#include "stdafx.h"
#include "DBAgent.h"

#pragma region CUser::HandleCollectionRaceStart
COMMAND_HANDLER(CUser::HandleCollectionRaceStart)
{
	if (!isGM())
		return false;

	if (vargs.empty() || vargs.size() < 1)
	{
		g_pMain->SendHelpDescription(this, "+cropen sEventIndex");
		return false;
	}

	uint16 sIndexID = atoi(vargs.front().c_str());
	vargs.pop_front();

	_COLLECTION_RACE_EVENT_LIST * pCollectEventList = g_pMain->m_CollectionRaceListArray.GetData(sIndexID);
	if (pCollectEventList == nullptr)
	{
		g_pMain->SendHelpDescription(this, "CollectionRace sEventIndex is nullptr");
		return false;
	}

	g_pMain->CollectionRaceStart(pCollectEventList, sIndexID, this);

	return true;
}
#pragma endregion

COMMAND_HANDLER(CUser::HandleCollectionRaceClose)
{
	if (!isGM()) 
		return false;

	if (!g_pMain->pCollectionRaceEvent.m_sCollectionStatus) 
	{
		g_pMain->SendHelpDescription(this, "CR event is already closed.");
		return false;
	} 
	
	g_pMain->CollectionRaceEnd();
	return true;
}

void CGameServerDlg::ReqCollectionRaceStart(Packet &pkt) {

	pCollectionRaceEvent.m_bCollectionRequest = false;

	uint16 index = pkt.read<uint16>();

	_COLLECTION_RACE_EVENT_LIST* pCollectEventList = g_pMain->m_CollectionRaceListArray.GetData(index);
	if (pCollectEventList == nullptr)
		return;

	bool RaceEventZone = true;
	for (int i = 0; i < 3; i++)
	{
		if (pCollectEventList->RewardItemID[i]) 
		{
			_ITEM_TABLE pTable = g_pMain->GetItemPtr(pCollectEventList->RewardItemID[i]);
			if (pTable.isnull())
				return;
		}
		if (pCollectEventList->RewardItemCount[i]) 
		{
			if (pCollectEventList->RewardItemCount[i] > ZONEITEM_MAX && (pCollectEventList->RewardItemID[i] == ITEM_GOLD || pCollectEventList->RewardItemID[i] == ITEM_EXP))
				return;
			else if (pCollectEventList->RewardItemCount[i] > 9999 && pCollectEventList->RewardItemID[i] != ITEM_GOLD && pCollectEventList->RewardItemID[i] != ITEM_EXP)
				return;
		}
	}

	for (int i = 0; i < 3; i++)
	{
		pCollectionRaceEvent.m_bProtoID[i] = pCollectEventList->m_bProtoID[i];
		pCollectionRaceEvent.m_bKillCount[i] = pCollectEventList->m_bKillCount[i];
		pCollectionRaceEvent.RewardItemID[i] = 0;
		pCollectionRaceEvent.RewardItemCount[i] = 0;
		pCollectionRaceEvent.RewardItemTime[i] = 0;
	}

	g_DBAgent.LoadCollectionReward(pCollectEventList->m_bEventID);
	pCollectionRaceEvent.m_bMinLevel = pCollectEventList->m_bMinLevel;
	pCollectionRaceEvent.m_bMaxLevel = pCollectEventList->m_bMaxLevel;
	pCollectionRaceEvent.m_bZoneID = pCollectEventList->m_bZoneID;
	pCollectionRaceEvent.m_bEventTime = (uint32)UNIXTIME + (pCollectEventList->m_bEventTime * 60);
	pCollectionRaceEvent.m_bUserLimit = pCollectEventList->m_bUserLimit;
	pCollectionRaceEvent.m_bEventName = pCollectEventList->m_bEventName;
	pCollectionRaceEvent.m_bTotalCount = 0;
	pCollectionRaceEvent.m_sCollectionStatus = true;
	pCollectionRaceEvent.m_bRankBug = myrand(g_pMain->pRankBug.CrMinComp > 0 ? g_pMain->pRankBug.CrMinComp : 1, g_pMain->pRankBug.CrMaxComp > 0 ? g_pMain->pRankBug.CrMaxComp : 0);
	pCollectionRaceEvent.m_bCollectionEventListStatus = pCollectEventList->m_bCollectionEventListStatus;
	pCollectionRaceEvent.m_bUserList.DeleteAllData();

	std::string descp1 = "", descp2 = "Quest : ", descp3 = "Reward : ", descp4 = "", title = "CollectionRace";
	descp1.append(string_format("Mission : %s", pCollectionRaceEvent.m_bEventName.c_str()));

	for (int i = 0; i < 3; i++) 
	{
		if (pCollectionRaceEvent.m_bProtoID[i])
		{
			auto* pMonster = m_arMonTable.GetData(pCollectionRaceEvent.m_bProtoID[i]);
			if (pMonster)
			{
				if (i == 2)
					descp2.append(string_format("%dx : %s", pCollectionRaceEvent.m_bKillCount[i], pMonster->m_strName.c_str()));
				else 
					descp2.append(string_format("%dx : %s -", pCollectionRaceEvent.m_bKillCount[i], pMonster->m_strName.c_str()));
			}
		}
	}
	for (int i = 0; i < 3; i++) 
	{
		if (pCollectionRaceEvent.RewardItemID[i]) 
		{
			auto pItem = GetItemPtr(pCollectionRaceEvent.RewardItemID[i]);
			if (!pItem.isnull()) 
			{
				if (i == 2)
					descp3.append(string_format("%dx : %s", pCollectionRaceEvent.RewardItemCount[i], pItem.m_sName.c_str()));
				else
					descp3.append(string_format("%dx : %s -", pCollectionRaceEvent.RewardItemCount[i], pItem.m_sName.c_str()));
			}
		}
	}

	std::string m_bZoneName = "-";
	auto* pzone = g_pMain->m_ZoneArray.GetData(pCollectionRaceEvent.m_bZoneID);
	if (pzone) 
		m_bZoneName = pzone->m_nZoneName;

	descp4.append(string_format("Zone: %s - level: (%d-%d)", m_bZoneName.c_str(), pCollectionRaceEvent.m_bMinLevel, pCollectionRaceEvent.m_bMaxLevel));

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr 
			|| !pUser->isInGame())
			continue;

		pUser->CollectionRace.m_bCheckFinish = false;

		for (int j = 0; j < 3; j++)
			pUser->CollectionRace.KillCount[j] = 0;

		if (pUser->GetLevel() < pCollectionRaceEvent.m_bMinLevel
			|| pUser->GetLevel() > pCollectionRaceEvent.m_bMaxLevel)
			continue;
#if(__VERSION < 2369)
		if (RaceEventZone)
		{

			Packet result(WIZ_HSACS_HOOK);
			result << uint8(HSACSXOpCodes::CR) << uint8(0x00);

			for (int i = 0; i < 3; i++)
				result << pCollectionRaceEvent.m_bProtoID[i] << pCollectionRaceEvent.m_bKillCount[i];

			for (int i = 0; i < 3; i++)
				result << pCollectionRaceEvent.RewardItemID[i] << pCollectionRaceEvent.RewardItemCount[i] << pCollectionRaceEvent.RewardItemRate[i];

			result << (uint32)(pCollectEventList->m_bEventTime * 60) << uint16(g_pMain->pCollectionRaceEvent.m_bTotalCount * g_pMain->pCollectionRaceEvent.m_bRankBug) << pCollectionRaceEvent.m_bUserLimit << pUser->GetNation();
			result << pCollectionRaceEvent.m_bEventName << pCollectionRaceEvent.m_bZoneID;
			pUser->Send(&result);
		}
#endif
		_CR_USER_LIST* pCollectionUserList = pCollectionRaceEvent.m_bUserList.GetData(pUser->GetName());
		if (pCollectionUserList != nullptr) 
		{
			pCollectEventList->m_bKillCount[0] = 0;
			pCollectEventList->m_bKillCount[1] = 0;
			pCollectEventList->m_bKillCount[2] = 0;
			pCollectionUserList->m_bFinish = false;
		}
		else 
		{
			_CR_USER_LIST* pList = new _CR_USER_LIST();
			pList->m_bKillCount[0] = 0;
			pList->m_bKillCount[1] = 0;
			pList->m_bKillCount[2] = 0;
			pList->m_bFinish = false;
			if (!pCollectionRaceEvent.m_bUserList.PutData(pUser->GetName(), pList)) {
				delete pList;
				continue;
			}
		}

		{
			Packet newpm1, newpm2, newpm3, newpm4;
			ChatPacket::Construct(&newpm1, (uint8)ChatType::PRIVATE_CHAT, descp1.c_str(), title.c_str(), pUser->GetNation());
			ChatPacket::Construct(&newpm2, (uint8)ChatType::PRIVATE_CHAT, descp2.c_str(), title.c_str(), pUser->GetNation());
			ChatPacket::Construct(&newpm3, (uint8)ChatType::PRIVATE_CHAT, descp3.c_str(), title.c_str(), pUser->GetNation());
			ChatPacket::Construct(&newpm4, (uint8)ChatType::PRIVATE_CHAT, descp4.c_str(), title.c_str(), pUser->GetNation());
			pUser->Send(&newpm1);
			pUser->Send(&newpm2);
			pUser->Send(&newpm3);
			pUser->Send(&newpm4);
		}
	}

	std::string buffer = "Collection Race Event started.";
	SendChat<ChatType::PUBLIC_CHAT>(buffer.c_str(), (uint8)Nation::ALL, true);
}

void CGameServerDlg::CollectionRaceStart(_COLLECTION_RACE_EVENT_LIST * pList, uint16 sIndex, CUser *pUser)
{
	if (pList == nullptr)
		return;

	if (pCollectionRaceEvent.m_sCollectionStatus)
		CollectionRaceEnd();

	if (pCollectionRaceEvent.m_bCollectionRequest) 
	{
		if(pUser)
			SendHelpDescription(pUser, "Daha Once Collection Race acilma istegi yollandý. Lütfen bekleyiniz");
		return;
	}

	if (pUser)
		SendHelpDescription(pUser, "Collection Race acilma istegi yollandý. Lütfen bekleyiniz");

	pCollectionRaceEvent.m_bCollectionRequest = true;
	Packet newpkt(WIZ_DB_SAVE, uint8(ProcDbServerType::CollectionRaceStart));
	newpkt << sIndex;
	g_pMain->AddDatabaseRequest(newpkt);
}

void CUser::CollectionRaceFirstLoad()
{
	if (!g_pMain->pCollectionRaceEvent.m_sCollectionStatus)
		return;

	_CR_USER_LIST* pCollectionUserList = g_pMain->pCollectionRaceEvent.m_bUserList.GetData(GetName());
	if (pCollectionUserList == nullptr)
	{
		pCollectionUserList = new _CR_USER_LIST();
		pCollectionUserList->m_bKillCount[0] = 0;
		pCollectionUserList->m_bKillCount[1] = 0;
		pCollectionUserList->m_bKillCount[2] = 0;
		pCollectionUserList->m_bFinish = false;
		pCollectionUserList->m_bUserStatus = 0;
		if (!g_pMain->pCollectionRaceEvent.m_bUserList.PutData(GetName(), pCollectionUserList))
		{
			delete pCollectionUserList;
			return;
		}
	}

	if (pCollectionUserList->m_bFinish)
		return;

	if (GetLevel() < g_pMain->pCollectionRaceEvent.m_bMinLevel
		|| GetLevel() > g_pMain->pCollectionRaceEvent.m_bMaxLevel)
		return;

	uint32 RemainingTime = g_pMain->pCollectionRaceEvent.m_bEventTime - (uint32)UNIXTIME;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::CR) << uint8(0x01);
#endif
	for (int i = 0; i < 3; i++)
	{
		CollectionRace.KillCount[i] = pCollectionUserList->m_bKillCount[i];
		uint16 KillCount = g_pMain->pCollectionRaceEvent.m_bKillCount[i];

		if (pCollectionUserList->m_bUserStatus > 0)
			KillCount *= uint16(2.5);
#if(__VERSION < 2369)
		result << g_pMain->pCollectionRaceEvent.m_bProtoID[i] << KillCount << pCollectionUserList->m_bKillCount[i];
#endif
	}
#if(__VERSION < 2369)
	for (int i = 0; i < 3; i++)
		result << g_pMain->pCollectionRaceEvent.RewardItemID[i] << g_pMain->pCollectionRaceEvent.RewardItemCount[i] << g_pMain->pCollectionRaceEvent.RewardItemRate[i];

	result << RemainingTime << uint16(g_pMain->pCollectionRaceEvent.m_bTotalCount * g_pMain->pCollectionRaceEvent.m_bRankBug) << g_pMain->pCollectionRaceEvent.m_bUserLimit << GetNation();
	result << g_pMain->pCollectionRaceEvent.m_bEventName << g_pMain->pCollectionRaceEvent.m_bZoneID;
	Send(&result);
#endif
}

void CUser::CollectionGetActiveTime()
{
	if (!g_pMain->pCollectionRaceEvent.m_sCollectionStatus)
		return;

	_CR_USER_LIST * pCollectionUserList = g_pMain->pCollectionRaceEvent.m_bUserList.GetData(GetName());
	if (pCollectionUserList == nullptr 
		|| pCollectionUserList->m_bFinish)
		return;

	if (GetLevel() < g_pMain->pCollectionRaceEvent.m_bMinLevel || GetLevel() > g_pMain->pCollectionRaceEvent.m_bMaxLevel)
		return;

	uint32 RemainingTime = g_pMain->pCollectionRaceEvent.m_bEventTime - (uint32)UNIXTIME;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::CR) << uint8(0x01);
#endif
	for (int i = 0; i < 3; i++)
	{
		uint16 KillCount = g_pMain->pCollectionRaceEvent.m_bKillCount[i];

		if (pCollectionUserList->m_bUserStatus > 0)
			KillCount *= uint16(2.5);
#if(__VERSION < 2369)
		result << g_pMain->pCollectionRaceEvent.m_bProtoID[i] << KillCount << pCollectionUserList->m_bKillCount[i];
#endif
	}
#if(__VERSION < 2369)
	for (int i = 0; i < 3; i++)
		result << g_pMain->pCollectionRaceEvent.RewardItemID[i] << g_pMain->pCollectionRaceEvent.RewardItemCount[i] << g_pMain->pCollectionRaceEvent.RewardItemRate[i];

	result << RemainingTime << uint16(g_pMain->pCollectionRaceEvent.m_bTotalCount* g_pMain->pCollectionRaceEvent.m_bRankBug) << g_pMain->pCollectionRaceEvent.m_bUserLimit << GetNation();
	result << g_pMain->pCollectionRaceEvent.m_bEventName << g_pMain->pCollectionRaceEvent.m_bZoneID;
	Send(&result);
#endif
}

void CUser::CollectionRaceHide()
{
	if (!g_pMain->pCollectionRaceEvent.m_sCollectionStatus)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::CR) << uint8(0x05);
	Send(&result);
#endif
}

void CGameServerDlg::CollectionRaceTimer()
{
	if (pCollectionRaceEvent.m_sCollectionStatus)
	{
		if (pCollectionRaceEvent.m_bEventTime > 0)
		{
			uint32 RemainingTime = pCollectionRaceEvent.m_bEventTime - (uint32)UNIXTIME;
			if (RemainingTime == 900)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 15), 1, 240, 1);
			else if (RemainingTime == 600)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 10), 1, 240, 1);
			else if (RemainingTime == 300)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 5), 1, 240, 1);
			else if (RemainingTime == 180)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 3), 1, 240, 1);
			else if (RemainingTime == 120)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 2), 1, 240, 1);
			else if (RemainingTime == 60)
				LogosYolla("[Collection Race]", string_format("Remaining Minute %d", 1), 1, 240, 1);

			if (!RemainingTime)
				CollectionRaceEnd();
		}
		else
			CollectionRaceEnd();
	}
}

void CGameServerDlg::CollectionRaceCounter()
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr 
			|| !pUser->isInGame())
			continue;

		if (pUser->GetLevel() < pCollectionRaceEvent.m_bMinLevel 
			|| pUser->GetLevel() > pCollectionRaceEvent.m_bMaxLevel)
			continue;
#if(__VERSION < 2369)
		Packet result(WIZ_HSACS_HOOK);
		result << uint8(HSACSXOpCodes::CR) 
			<< uint8(0x03) 
			<< uint16(pCollectionRaceEvent.m_bTotalCount * pCollectionRaceEvent.m_bRankBug)
			<< pCollectionRaceEvent.m_bUserLimit;
		pUser->Send(&result);
#endif
	}
}

void CGameServerDlg::CollectionRaceSendDead(Unit* pKiller, uint16 ProtoID)
{
	if (pKiller == nullptr
		|| !pKiller->isPlayer())
		return;

	auto* pKillerUser = TO_USER(pKiller);

	if (pKillerUser->GetLevel() < g_pMain->pCollectionRaceEvent.m_bMinLevel
		|| pKillerUser->GetLevel() > g_pMain->pCollectionRaceEvent.m_bMaxLevel)
		return;

	if (pKillerUser->CollectionRace.m_bCheckFinish)
		return;

	_CR_USER_LIST* pCollectionUserList = pCollectionRaceEvent.m_bUserList.GetData(pKiller->GetName());
	if (!pCollectionUserList)
		return;

	bool m_bCheckFinish = true;
	for (int i = 0; i < 3; i++)
	{
		uint16 KillCount = g_pMain->pCollectionRaceEvent.m_bKillCount[i];

		if (pCollectionUserList->m_bUserStatus > 0)
			KillCount *= uint16(2.5);

		if (g_pMain->pCollectionRaceEvent.m_bProtoID[i] == ProtoID)
		{
			if (pKillerUser->CollectionRace.KillCount[i] < KillCount && KillCount) 
			{
				pKillerUser->CollectionRace.KillCount[i]++;
				pCollectionUserList->m_bKillCount[i] = pKillerUser->CollectionRace.KillCount[i];
			}
		}

		if (pKillerUser->CollectionRace.KillCount[i] != KillCount && KillCount)
			m_bCheckFinish = false;
	}

	if (m_bCheckFinish)
	{
		bool isSave = false;
		if (g_pMain->pCollectionRaceEvent.m_bCollectionEventListStatus == 0)
			isSave = true;
		else if (g_pMain->pCollectionRaceEvent.m_bCollectionEventListStatus == 1)
		{
			pCollectionUserList->m_bUserStatus++;

			if (pCollectionUserList->m_bUserStatus > 1)
				isSave = true;
		}

		if (isSave)
		{
			pKillerUser->CollectionRace.m_bCheckFinish = m_bCheckFinish;
			pCollectionUserList->m_bFinish = m_bCheckFinish;
		}
	}
	if (m_bCheckFinish)
	{
		bool kapat = true;

		if (g_pMain->pCollectionRaceEvent.m_bCollectionEventListStatus == 2)
			kapat = false;

		if (g_pMain->pCollectionRaceEvent.m_bCollectionEventListStatus == 1 && pCollectionUserList->m_bUserStatus < 2)
			kapat = false;
#if(__VERSION < 2369)
		if (kapat)
		{
			Packet result(WIZ_HSACS_HOOK);
			result << uint8(HSACSXOpCodes::CR) << uint8(0x04);
			pKillerUser->Send(&result);
		}
#endif
		if (!kapat)
		{
			for (int i = 0; i < 3;i++)
				pCollectionUserList->m_bKillCount[i] = 0;

			pKillerUser->CollectionRaceFirstLoad();
		}

		if (kapat)
			pCollectionUserList->m_bFinish = m_bCheckFinish;

		g_pMain->pCollectionRaceEvent.m_bTotalCount++;

		if (g_pMain->pCollectionRaceEvent.m_bTotalCount <= g_pMain->pCollectionRaceEvent.m_bUserLimit)
		{
			pKillerUser->CollectionRaceFinish();
			CollectionRaceCounter();
		}

		if (g_pMain->pCollectionRaceEvent.m_bTotalCount >= g_pMain->pCollectionRaceEvent.m_bUserLimit)
			CollectionRaceEnd();
	}
#if(__VERSION < 2369)
	Packet Dead(WIZ_HSACS_HOOK);
	Dead << uint8(HSACSXOpCodes::CR) 
		<< uint8(0x02) 
		<< ProtoID 
		<< pKillerUser->CollectionRace.KillCount[0] 
		<< pKillerUser->CollectionRace.KillCount[1] 
		<< pKillerUser->CollectionRace.KillCount[2];
	TO_USER(pKiller)->Send(&Dead);
#endif
}
void CUser::GiveRandomItem(uint32& nItemID, uint32& nCount, uint8 bySession)
{
	std::map <uint16, _RANDOM_ITEM*> nRandom;
	int say = 0;

	foreach(itr, g_pMain->m_RandomItemArray)
	{
		_RANDOM_ITEM* pRandom = *itr;
		if (pRandom->SessionID != bySession)
			continue;

		nRandom.insert(std::make_pair(say, pRandom));
		say++;
	}

	int thisrand = (int)nRandom.size();
	if (thisrand == 0)
		return;

	thisrand -= 1;
	int rand = myrand(0, thisrand);

	auto it = nRandom.find(rand);
	if (it != nRandom.end())
	{
		_RANDOM_ITEM* pRandom = it->second;
		nItemID = pRandom->ItemID;
		nCount = pRandom->ItemCount;
	}
}

void CUser::CollectionRaceFinish()
{
	_CR_USER_LIST * pCollectionUser = g_pMain->pCollectionRaceEvent.m_bUserList.GetData(GetName());

	if (pCollectionUser != nullptr)
		pCollectionUser->m_bFinish = CollectionRace.m_bCheckFinish;

	uint32 tmp_RewardItemID[3]{};
	uint32 tmp_RewardItemCount[3]{};
	for (int i = 0; i < 3; i++)
	{
		CollectionRace.KillCount[i] = 0;
		tmp_RewardItemID[i] = 0;
		tmp_RewardItemCount[i] = 0;
	}

	bool isCind = pCindWar.isEventUser() && g_pMain->isCindirellaZone(GetZoneID());

	bool lettercheck = false;
	for (int i = 0; i < 3; i++)
	{
		if (g_pMain->pCollectionRaceEvent.RewardItemID[i] > 0)
		{
			if (isCind)
			{
				if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_GOLD)
				{
					pCindWar.gainnoah += g_pMain->pCollectionRaceEvent.RewardItemCount[i];
					continue;
				}

				if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_EXP)
				{
					pCindWar.gainexp += g_pMain->pCollectionRaceEvent.RewardItemCount[i];
					continue;
				}
			}
			else
			{
				if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_GOLD)
				{
					GoldGain(g_pMain->pCollectionRaceEvent.RewardItemCount[i]);
					continue;
				}

				if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_EXP)
				{
					ExpChange("collection race", g_pMain->pCollectionRaceEvent.RewardItemCount[i], true);
					continue;
				}
			}

			if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == ITEM_COUNT)
			{
				SendLoyaltyChange("collection race", g_pMain->pCollectionRaceEvent.RewardItemCount[i]);
				continue;
			}
			
			uint32 xItemID = g_pMain->pCollectionRaceEvent.RewardItemID[i];
			uint32 xItemTime = g_pMain->pCollectionRaceEvent.RewardItemTime[i];
			uint32 xCount = g_pMain->pCollectionRaceEvent.RewardItemCount[i];
			uint8 xRand = g_pMain->pCollectionRaceEvent.RewardItemRate[i];

			if (g_pMain->pCollectionRaceEvent.RewardItemID[i] == 900004000)
				GiveRandomItem(xItemID, xCount, g_pMain->pCollectionRaceEvent.RewardSession[i]);

			tmp_RewardItemID[i] = xItemID;
			tmp_RewardItemCount[i] = xCount;

			_ITEM_TABLE pTable = g_pMain->GetItemPtr(xItemID);
			if (!pTable.isnull()) 
			{
				int nRate = 0;

				if (xRand > 100) 
					xRand = 100;

				if (xRand) 
					nRate = myrand(0, 10000);

				if (nRate && (xRand * 100) < nRate) 
				{
					tmp_RewardItemID[i] = 0;
					tmp_RewardItemCount[i] = 0;
					continue;
				}

				int8 pos = FindSlotForItem(xItemID, xCount);
				if ((pCindWar.isEventUser() && g_pMain->isCindirellaZone(GetZoneID())) || pos < 0)
				{
					Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::CollectionRaceReward));
					newpkt << xItemID << xCount;
					g_pMain->AddDatabaseRequest(newpkt, this);
				}
				else
					GiveItem("Collection Race", xItemID, xCount, true, xItemTime);
			}
		}
	}
	
	Packet result(WIZ_QUEST);
	result << uint8(0x0A);
	for (int i = 0; i < 3; i++)
		result << tmp_RewardItemID[i] << tmp_RewardItemCount[i];
		
	result << uint32_t(0x00) << uint32_t(0x00) << uint32_t(0x00) << uint32_t(0x00);
	Send(&result);
}

void CGameServerDlg::CollectionRaceEnd()
{
	if (pCollectionRaceEvent.m_sCollectionStatus)
	{
		std::string buffer = "Collection Race Event has end.";
		SendChat<ChatType::PUBLIC_CHAT>(buffer.c_str(), (uint8)Nation::ALL, true);
	}
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::CR) << uint8(0x04);
	CollectionRaceSend(&result);
#endif
	CollectionRaceDataReset();
}

void CGameServerDlg::CollectionRaceDataReset()
{
	pCollectionRaceEvent.m_bUserList.DeleteAllData();
	pCollectionRaceEvent.m_sCollectionStatus = false;
	pCollectionRaceEvent.m_bEventTime = 0;
	memset(&pCollectionRaceEvent.RewardItemID, 0, sizeof(pCollectionRaceEvent.RewardItemID));
	memset(&pCollectionRaceEvent.RewardItemCount, 0, sizeof(pCollectionRaceEvent.RewardItemCount));
	memset(&pCollectionRaceEvent.m_bProtoID, 0, sizeof(pCollectionRaceEvent.m_bProtoID));
	memset(&pCollectionRaceEvent.m_bKillCount, 0, sizeof(pCollectionRaceEvent.m_bKillCount));
	pCollectionRaceEvent.m_bUserLimit = pCollectionRaceEvent.m_bTotalCount = 0;
	pCollectionRaceEvent.m_bZoneID = pCollectionRaceEvent.m_bMinLevel = pCollectionRaceEvent.m_bMaxLevel = 0;
	pCollectionRaceEvent.m_bEventName = "";
}

void CGameServerDlg::CollectionRaceSend(Packet * pkt)
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr 
			|| !pUser->isInGame())
			continue;

		if (pUser->GetLevel() < pCollectionRaceEvent.m_bMinLevel
			|| pUser->GetLevel() > pCollectionRaceEvent.m_bMaxLevel)
			continue;

		pUser->CollectionRace.m_bCheckFinish = false;

		for (int j = 0; j < 3; j++)
			pUser->CollectionRace.KillCount[j] = 0;

		pUser->Send(pkt);
	}
}

void CGameServerDlg::LogosYolla(std::string LogosName, std::string LogosMessage, uint8 R, uint8 G, uint8 B)
{
	Packet Logos(WIZ_LOGOSSHOUT);
	std::string Birlestir = LogosName + " : " + LogosMessage;
	Logos.SByte();
	Logos << uint8(2) << uint8(1) << R << G << B << uint8(0) << Birlestir;
	g_pMain->Send_All(&Logos);
}