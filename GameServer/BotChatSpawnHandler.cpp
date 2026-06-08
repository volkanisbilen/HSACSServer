#include "StdAfx.h"
#include "GameServerDlg.h"
#include "User.h"
#include "BotHandler.h"
#include "Npc.h"
#include "Map.h"
#include "MagicInstance.h"
#include "MagicProcess.h"
#include "Knights.h"
#include "KnightsManager.h"
#include "ChatHandler.h"

COMMAND_HANDLER(CUser::HandleBotDisconnected)
{
	if (!isGM())
		return false;

	if (vargs.size() > 0)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +botkill");
		return true;
	}

	CBot* pBot = g_pMain->GetBotPtr(GetTargetID());
	if (pBot == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Such a bots does not exist in the game.");
		return true;
	}

	pBot->UserInOut(INOUT_OUT);
	g_pMain->RemoveMapBotList(pBot->GetID(), pBot->GetName());
	return true;
}

COMMAND_HANDLER(CUser::HandleBotAfkSystem)
{
	if (!isGM())
		return false;

	if (vargs.size() < 2)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +afkbotspawn count minute level");
		return true;
	}

	int sTime = 0, sLevel = 0, sCount = 0;
	sCount = atoi(vargs.front().c_str());
	vargs.pop_front();
	sTime = atoi(vargs.front().c_str());
	vargs.pop_front();
	sLevel = atoi(vargs.front().c_str());

	if (sCount > 100)
		sCount = 100;

	for (int i = 0; i < sCount; i++)
	{
		float BonX = myrand(1, 5) * 1.0f;
		float BonZ = myrand(1, 5) * 1.0f;
		g_pMain->SpawnEventAfkBotHandler(sTime, GetZoneID(), GetX() + BonX, GetY(), GetZ() + BonZ, sLevel);
	}
	return true;
}

COMMAND_HANDLER(CUser::HandleBotAllDisconnected)
{
	if (!isGM())
		return false;

	std::vector<uint32> HandleBotAllDisconnected;
	if (vargs.size() > 0)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +allbotkill");
		return true;
	}
	
	g_pMain->m_sMapBotListArray.m_lock.lock();
	auto m_sMapBotListArray = g_pMain->m_sMapBotListArray.m_UserTypeMap;
	g_pMain->m_sMapBotListArray.m_lock.unlock();

	foreach(itr, m_sMapBotListArray)
	{
		CBot* pBot = g_pMain->GetBotPtr(itr->first);
		if (pBot == nullptr)
			continue;

		HandleBotAllDisconnected.push_back(pBot->GetID());
	}

	if (HandleBotAllDisconnected.size() > 0)
	{
		foreach(itr, HandleBotAllDisconnected)
		{
			CBot* pBot = g_pMain->GetBotPtr(*itr);
			if (pBot == nullptr)
				continue;

			pBot->UserInOut(INOUT_OUT);
			g_pMain->RemoveMapBotList(pBot->GetID(), pBot->GetName());
		}
	}
	return true;
}

COMMAND_HANDLER(CUser::HandleBotSpawnMining)
{
	if (!isGM())
		return false;

	if (vargs.size() < 3)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +miningbotspawn Count Minute MinLevel");
		return true;
	}

	int sTime = 0, sLevel = 0, sCount = 0;
	sCount = atoi(vargs.front().c_str());
	vargs.pop_front();
	sTime = atoi(vargs.front().c_str());
	vargs.pop_front();
	sLevel = atoi(vargs.front().c_str());

	if (sCount > 100)
		sCount = 100;

	for (int i = 0; i < sCount; i++)
	{
		float BonX = myrand(1, 5) * 1.0f;
		float BonZ = myrand(1, 5) * 1.0f;
		g_pMain->SpawnEventBotMining(sTime, GetZoneID(), GetX() + BonX, GetY(), GetZ() + BonZ, sLevel);
	}
	return true;
}

COMMAND_HANDLER(CUser::HandleBotSpawnMerchant)
{
	if (!isGM())
		return false;

	if (vargs.size() < 1)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +merchantbotspawn sIndex");
		return true;
	}

	DateTime time;
	int sIndex = 0;
	sIndex = atoi(vargs.front().c_str());

	uint16 m_sSocketID = g_pMain->SpawnEventBotMerchant(3600, GetZoneID(), GetX(), GetY(), GetZ(), 0, MIN_LEVEL_ARDREAM);
	CBot* pBot = g_pMain->GetBotPtr(m_sSocketID);
	if (pBot == nullptr)
		return true;

	_BOT_MERCHANT_ITEM* pBotMerchantTable = g_pMain->m_ArtificialMerchantArray.GetData(sIndex);
	if (pBotMerchantTable == nullptr)
	{
		pBot->UserInOut(INOUT_OUT);
		g_pMain->RemoveMapBotList(pBot->GetID(), pBot->GetName());
		return true;
	}

	pBot->m_bPremiumMerchant = (bool)pBotMerchantTable->BotMerchantType;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_INSERT));
	result << uint16(1) << pBotMerchantTable->BotMerchantMessage << uint32(pBot->GetID())
		<< pBot->m_bPremiumMerchant;

	if (!pBotMerchantTable->BotMerchantMessage.empty())
		pBot->MerchantChat = string_format("%s(Location:%d,%d)", pBotMerchantTable->BotMerchantMessage.c_str(), pBot->GetSPosX() / 10, pBot->GetSPosZ() / 10);
	else
		pBot->MerchantChat.clear();

	uint16 totalMerchItems = 0;
	for (int t = 0; t < MAX_MERCH_ITEMS; t++)
	{
		pBot->m_arMerchantItems[t].bCount = pBotMerchantTable->m_MerchantItems[t].bCount;
		pBot->m_arMerchantItems[t].bOriginalSlot = pBotMerchantTable->m_MerchantItems[t].bOriginalSlot;
		pBot->m_arMerchantItems[t].IsSoldOut = pBotMerchantTable->m_MerchantItems[t].IsSoldOut;
		pBot->m_arMerchantItems[t].nNum = pBotMerchantTable->m_MerchantItems[t].nNum;
		pBot->m_arMerchantItems[t].nPrice = pBotMerchantTable->m_MerchantItems[t].nPrice;
		pBot->m_arMerchantItems[t].nSerialNum = pBotMerchantTable->m_MerchantItems[t].nSerialNum;
		pBot->m_arMerchantItems[t].sCount = pBotMerchantTable->m_MerchantItems[t].sCount;
		pBot->m_arMerchantItems[t].sDuration = pBotMerchantTable->m_MerchantItems[t].sDuration;

		if (pBot->m_arMerchantItems[t].nNum != 0 &&
			(pBot->m_arMerchantItems[t].bCount == 0
				|| pBot->m_arMerchantItems[t].bCount < pBotMerchantTable->m_MerchantItems[t].sCount))
			continue;

		result << pBot->m_arMerchantItems[t].nNum;

		if (pBot->m_arMerchantItems[t].nNum > 0)
			totalMerchItems++;
	}

	if (totalMerchItems == 0)
	{
		result.clear();
		result.Initialize(WIZ_MERCHANT);
		result << uint8(MERCHANT_CLOSE) << uint32(pBot->GetID());
		pBot->SendToRegion(&result);

		pBot->UserInOut(INOUT_OUT);
		g_pMain->RemoveMapBotList(pBot->GetID(), pBot->GetName());
		return true;
	}

	pBot->m_bMerchantState = MERCHANT_STATE_SELLING;
	pBot->SendToRegion(&result);
	MerchantClose();
	return true;
}

COMMAND_HANDLER(CUser::HandleBotSpawnFishing)
{
	if (!isGM())
		return false;

	if (vargs.size() < 3)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +fishingbotspawn Count Minute MinLevel");
		return true;
	}

	int sTime = 0, sLevel = 0, sCount = 0;
	sCount = atoi(vargs.front().c_str());
	vargs.pop_front();
	sTime = atoi(vargs.front().c_str());
	vargs.pop_front();
	sLevel = atoi(vargs.front().c_str());

	if (sCount > 100)
		sCount = 100;

	for (int i = 0; i < sCount; i++)
	{
		float BonX = myrand(1, 5) * 1.0f;
		float BonZ = myrand(1, 5) * 1.0f;
		g_pMain->SpawnEventBotFishing(sTime, GetZoneID(), GetX() + BonX, GetY(), GetZ() + BonZ, sLevel);
	}
	return true;
}

COMMAND_HANDLER(CUser::HandleBotSpawnFarm)
{
	if (!isGM())
		return false;

	if (vargs.size() < 6)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +farmbotspawn Count Minute MinLevel PartyLider Genie Class (0 for all classes)");
		return true;
	}

	int sTime = 0, sLevel = 0, sCount = 0, sPartyLider = 0, sGenie = 0, sClass = 0;
	sCount = atoi(vargs.front().c_str());
	vargs.pop_front();
	sTime = atoi(vargs.front().c_str());
	vargs.pop_front();
	sLevel = atoi(vargs.front().c_str());
	vargs.pop_front();
	sPartyLider = atoi(vargs.front().c_str());
	vargs.pop_front();
	sGenie = atoi(vargs.front().c_str());
	vargs.pop_front();
	sClass = atoi(vargs.front().c_str());

	if (sCount > 100)
		sCount = 100;

	for (int i = 0; i < sCount; i++)
	{
		float BonX = myrand(1, 5) * 1.0f;
		float BonZ = myrand(1, 5) * 1.0f;
		g_pMain->SpawnEventBotFarm(sTime, GetZoneID(), GetX() + BonX, GetY(), GetZ() + BonZ, sLevel, sPartyLider, sGenie, sClass);
	}
	return true;
}

COMMAND_HANDLER(CUser::HandleBotSpawnMerchantMove)
{
	if (!isGM())
		return false;

	if (vargs.size() < 3)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +merchantmovebotspawn Count Minute MinLevel");
		return true;
	}

	int sTime = 0, sLevel = 0, sCount = 0;
	sCount = atoi(vargs.front().c_str());
	vargs.pop_front();
	sTime = atoi(vargs.front().c_str());
	vargs.pop_front();
	sLevel = atoi(vargs.front().c_str());

	if (sCount > 100)
		sCount = 100;

	for (int i = 0; i < sCount; i++)
	{
		float BonX = myrand(1, 15) * 1.0f;
		float BonZ = myrand(1, 15) * 1.0f;
		g_pMain->SpawnEventBotMoveProcess(sTime, GetZoneID(), GetX() + BonX, GetY(), GetZ() + BonZ, sLevel);
	}
	return true;
}

COMMAND_HANDLER(CUser::HandleBotSpawnPk)
{
	if (!isGM())
		return false;

	if (vargs.size() < 5)
	{
		// send description
		g_pMain->SendHelpDescription(this, "Using Sample : +pkbotspawn Count Minute MinLevel Nation Class (0 for all classes)");
		return true;
	}

	int sTime = 0, sLevel = 0, sCount = 0, sNation = 0, sClass = 0;
	sCount = atoi(vargs.front().c_str());
	vargs.pop_front();
	sTime = atoi(vargs.front().c_str());
	vargs.pop_front();
	sLevel = atoi(vargs.front().c_str());
	vargs.pop_front();
	sNation = atoi(vargs.front().c_str());
	vargs.pop_front();
	sClass = atoi(vargs.front().c_str());

	if (sCount > 100)
		sCount = 100;

	for (int i = 0; i < sCount; i++)
	{
		float BonX = myrand(1, 15) * 1.0f;
		float BonZ = myrand(1, 15) * 1.0f;
		g_pMain->SpawnEventBotPk(sTime, GetZoneID(), GetX() + BonX, GetY(), GetZ() + BonZ, sLevel, sNation, sClass, this);
	}
	return true;
}

void CBot::SendDeathNotice(Unit* pKiller, DeathNoticeType noticeType, bool isToZone /*= true*/)
{
	if (pKiller == nullptr)
		return;

	Packet result(WIZ_CHAT, uint8(ChatType::DEATH_NOTICE));
	result.SByte();
	result << GetNation()
		<< pKiller->GetNation()
		<< uint8(0)
		<< uint8(noticeType)
		<< uint32(pKiller->GetID())
		<< pKiller->GetName()
		<< uint32(GetID())
		<< GetName()
		<< uint16(GetX()) 
		<< uint16(GetZ());

#if(GAME_SOURCE_VERSION != 2369)
	bool newnotice;

	if (GAME_SOURCE_VERSION == 1098)
		newnotice = GAME_SOURCE_VERSION == 1098 && noticeType != DeathNoticeType::DeathNoticeRival;
	else if (GAME_SOURCE_VERSION == 1534)
		newnotice = GAME_SOURCE_VERSION == 1534 && noticeType != DeathNoticeType::DeathNoticeRival;
	else
		newnotice = GAME_SOURCE_VERSION == 2369 && noticeType != DeathNoticeType::DeathNoticeRival;

	if (newnotice)
	{
		if (pKiller->isPlayer())
			SendNewDeathNotice(pKiller);
		else if (pKiller->isNPC())
		{
			if (isToZone)
				SendToZone(&result, (isInArena() ? RANGE_30M : 0.0f));
		}
		else if (pKiller->isBot())
			SendNewDeathNotice(pKiller);
	}
	else
	{
		if (isToZone)
			SendToZone(&result, (isInArena() ? RANGE_30M : 0.0f));
		else 
		{
			if (pKiller->isPlayer())
				TO_USER(pKiller)->Send(&result);
		}
	}
#else
	if (isToZone)
		SendToZone(&result, (isInArena() ? RANGE_30M : 0.0f));
	else
	{
		if (pKiller->isPlayer())
			TO_USER(pKiller)->Send(&result);
	}
#endif
}

void CBot::SendNewDeathNotice(Unit* pKiller)
{
#if(__VERSION < 2369)
	if (pKiller == nullptr 
		|| pKiller->isNPC())
		return;

	float fRange = 0.0f;
	uint8 killtype = 3;
	std::string color = "#f08080";

	if (pKiller->isBot())
	{
		CBot* pBot = TO_BOT(pKiller);
		if (pBot != nullptr)
		{
			if (!pBot->isInGame())
				return;

			for (uint16 i = 0; i < MAX_USER; i++)
			{
				CUser* pUser = g_pMain->GetUserPtr(i);
				if (pUser == nullptr)
					continue;

				if (pUser->GetZoneID() != GetZoneID() || pUser->GetEventRoom() != GetEventRoom())
					continue;

				if (pBot->GetID() == pUser->GetID())
					killtype = 1;
				else if (GetID() == pUser->GetID())
					killtype = 1;
				else if (pBot->isInParty()
					&& pUser->GetPartyID() == pBot->GetPartyID())
					killtype = 2;
				else
					killtype = 3;

				Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::DeathNotice));
				result.SByte();
				result << killtype << TO_USER(pKiller)->GetName().c_str() << GetName().c_str() << uint16(GetX()) << uint16(GetZ());
				pUser->Send(&result);
			}
		}
	}
	else
	{
		CUser* pKillerUser = TO_USER(pKiller);
		if (pKillerUser != nullptr)
		{
			if (!pKillerUser->isInGame())
				return;

			for (uint16 i = 0; i < MAX_USER; i++)
			{
				CUser* pUser = g_pMain->GetUserPtr(i);
				if (pUser == nullptr)
					continue;

				if (pUser->GetZoneID() != GetZoneID()
					|| pUser->GetEventRoom() != GetEventRoom())
					continue;

				if (pKillerUser->GetID() == pUser->GetID())
					killtype = 1;
				else if (GetID() == pUser->GetID())
					killtype = 1;
				else if (pKillerUser->isInParty()
					&& pUser->GetPartyID() == pKillerUser->GetPartyID())
					killtype = 2;
				else
					killtype = 3;

				Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::DeathNotice));
				result.SByte();
				result << killtype << TO_USER(pKiller)->GetName().c_str() << GetName().c_str() << uint16(GetX()) << uint16(GetZ());
				pUser->Send(&result);
			}
		}
	}
#endif
}

uint16 CGameServerDlg::SpawnEventAfkBotHandler(int Minute, uint8 byZone, float fX, float fY, float fZ, uint8 minlevel)
{
	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		if (byZone <= ZONE_ELMORAD && byZone != pBotInfo->m_bNation
			|| (byZone >= ZONE_KARUS_ESLANT
				&& byZone <= ZONE_ELMORAD_ESLANT
				&& byZone != (pBotInfo->m_bNation + 10)))
			continue;

		if (pBotInfo->m_bLevel < minlevel)
			continue;

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		int Random = myrand(0, 10000);
		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (Minute * 60);
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;
		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->SetBotAbility();
		pBot->m_BotState = BOT_AFK;
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_IN);
		pBot->StateChangeServerDirect(1, Random > 5000 ? USER_STANDING : USER_SITDOWN);
		return pBot->GetID();
	}
	return 0;
}

uint16 CGameServerDlg::SpawnEventBotMining(int Minute, uint8 byZone, float fX, float fY, float fZ, uint8 minlevel)
{
	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		if (byZone <= ZONE_ELMORAD && byZone != pBotInfo->m_bNation
			|| (byZone >= ZONE_KARUS_ESLANT
				&& byZone <= ZONE_ELMORAD_ESLANT
				&& byZone != (pBotInfo->m_bNation + 10)))
			continue;

		if (pBotInfo->m_bLevel < minlevel)
			continue;

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (Minute * 60);
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;

		_ITEM_DATA* pItem = pBot->GetItem(RIGHTHAND);
		if (pItem == nullptr)
		{
			_ITEM_TABLE pTable = GetItemPtr(MATTOCK);
			if (pTable.isnull())
				continue;

			_ITEM_DATA* pTItem = pBot->GetItem(LEFTHAND);
			if (pTItem != nullptr)
				memset(pTItem, 0x00, sizeof(_ITEM_DATA));

			memset(pItem, 0x00, sizeof(_ITEM_DATA));
			pItem->nNum = pTable.m_iNum;
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
			pItem->sCount = 1;
			pItem->sDuration = pTable.m_sDuration;
		}
		else
		{
			_ITEM_TABLE pTable = GetItemPtr(MATTOCK);
			if (pTable.isnull())
				continue;

			_ITEM_DATA* pTItem = pBot->GetItem(LEFTHAND);
			if (pTItem != nullptr)
				memset(pTItem, 0x00, sizeof(_ITEM_DATA));

			memset(pItem, 0x00, sizeof(_ITEM_DATA));
			pItem->nNum = pTable.m_iNum;
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
			pItem->sCount = 1;
			pItem->sDuration = pTable.m_sDuration;
		}

		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->m_bResHpType = USER_MINING;
		pBot->SetBotAbility();
		pBot->m_BotState = BOT_MINING;
		pBot->StateChangeServerDirect(1, USER_MINING);
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_IN);
		return pBot->GetID();
	}
	return true;
}

uint16 CGameServerDlg::SpawnEventBotMerchant(int Minute, uint8 byZone, float fX, float fY, float fZ, int16 nDir, uint8 minlevel)
{
	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		if (byZone <= ZONE_ELMORAD && byZone != pBotInfo->m_bNation
			|| (byZone >= ZONE_KARUS_ESLANT
				&& byZone <= ZONE_ELMORAD_ESLANT
				&& byZone != (pBotInfo->m_bNation + 10)))
			continue;

		if (pBotInfo->m_bLevel < minlevel)
			continue;

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		int Random = myrand(0, 10000);
		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (Minute * 60);
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;
		pBot->m_sDirection = nDir;
		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->SetBotAbility();
		pBot->m_BotState = BOT_MERCHANT;
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_IN);
		pBot->StateChangeServerDirect(1, Random > 5000 ? USER_STANDING : USER_SITDOWN);
		return pBot->GetID();
	}
	return 0;
}

uint16 CGameServerDlg::SpawnEventBotFishing(int Minute, uint8 byZone, float fX, float fY, float fZ, uint8 minlevel)
{
	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		if (byZone <= ZONE_ELMORAD && byZone != pBotInfo->m_bNation
			|| (byZone >= ZONE_KARUS_ESLANT
				&& byZone <= ZONE_ELMORAD_ESLANT
				&& byZone != (pBotInfo->m_bNation + 10)))
			continue;

		if (pBotInfo->m_bLevel < minlevel)
			continue;

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (Minute * 60);
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;

		_ITEM_DATA* pItem = pBot->GetItem(RIGHTHAND);
		if (pItem == nullptr)
		{
			_ITEM_TABLE pTable = GetItemPtr(FISHING);
			if (pTable.isnull())
				continue;

			_ITEM_DATA* pTItem = pBot->GetItem(LEFTHAND);
			if (pTItem != nullptr)
				memset(pTItem, 0x00, sizeof(_ITEM_DATA));

			memset(pItem, 0x00, sizeof(_ITEM_DATA));
			pItem->nNum = pTable.m_iNum;
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
			pItem->sCount = 1;
			pItem->sDuration = pTable.m_sDuration;
		}
		else
		{
			_ITEM_TABLE pTable = GetItemPtr(FISHING);
			if (pTable.isnull())
				continue;

			_ITEM_DATA* pTItem = pBot->GetItem(LEFTHAND);
			if (pTItem != nullptr)
				memset(pTItem, 0x00, sizeof(_ITEM_DATA));

			memset(pItem, 0x00, sizeof(_ITEM_DATA));
			pItem->nNum = pTable.m_iNum;
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
			pItem->sCount = 1;
			pItem->sDuration = pTable.m_sDuration;
		}

		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->m_bResHpType = USER_FLASHING;
		pBot->SetBotAbility();
		pBot->m_BotState = BOT_FISHING;
		pBot->StateChangeServerDirect(1, USER_FLASHING);
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_IN);

		return pBot->GetID();
	}
	return true;
}

uint16 CGameServerDlg::SpawnEventBotFarm(int Minute, uint8 byZone, float fX, float fY, float fZ, uint8 minlevel, uint8 sPartyLider, uint8 sGenie, uint8 sClass)
{
	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		if (byZone <= ZONE_ELMORAD && byZone != pBotInfo->m_bNation
			|| (byZone >= ZONE_KARUS_ESLANT
				&& byZone <= ZONE_ELMORAD_ESLANT
				&& byZone != (pBotInfo->m_bNation + 10)))
			continue;

		if (pBotInfo->m_bLevel < minlevel)
			continue;

		_ITEM_DATA* pItem = pBotInfo->GetItem(RIGHTHAND);
		if (pItem == nullptr)
			continue;

		_ITEM_TABLE pTable = GetItemPtr(pItem->nNum);
		if (pTable.isnull())
			continue;

		if (pTable.isShield()
			|| pTable.isPickaxe()
			|| pTable.isFishing())
			continue;

		// Class check - if sClass is 0, allow all classes, otherwise check specific class
		if (sClass > 0 && sClass < 15)
		{
			if (sClass == 1 && !pBotInfo->isWarrior())
				continue;

			if (sClass == 2 && !pBotInfo->isRogue())
				continue;

			if (sClass == 3 && !pBotInfo->isMage())
				continue;

			if (sClass == 4 && !pBotInfo->isPriest())
				continue;

			if (sClass == 14 && !pBotInfo->isPortuKurian())
				continue;

			if (sClass > 4 && sClass < 14)
				continue;
		}
		// If sClass is 0, allow all classes to spawn

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;		

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		pBot->m_PlayerKillingLoyaltyDaily = 0;
		pBot->m_PlayerKillingLoyaltyPremiumBonus = 0;

		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (Minute * MINUTE);
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;
		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->SetBotAbility();
		pBot->m_BotState = BOT_FARMER;
		pBot->m_bPartyLeader = sPartyLider == 1 ? true : false;
		pBot->m_bGenieStatus = sGenie == 1 ? true : false;
		pBot->SetPosition(fX, fY, fZ);
		pBot->m_oldx = pBot->m_curx;
		pBot->m_oldy = pBot->m_cury;
		pBot->m_oldz = pBot->m_curz;
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		pBot->StateChangeServerDirect(1, USER_STANDING);

		if (byZone == ZONE_RONARK_LAND
			|| byZone == ZONE_RONARK_LAND_BASE
			|| byZone == ZONE_ARDREAM)
		{
			C3DMap* pMap = g_pMain->GetZoneByID(byZone);
			if (pMap == nullptr)
				continue;

			TRACE("%s In the Game Insert Player Ranking ZoneID %d\n", pBot->GetName().c_str(), pBot->GetZoneID());
			pBot->AddBotRank(pMap);
		}

		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_IN);
		return pBot->GetID();
	}
	return true;
}

uint16 CGameServerDlg::SpawnEventBotMoveProcess(int Minute, uint8 byZone, float fX, float fY, float fZ, uint8 minlevel)
{
	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		if (byZone <= ZONE_ELMORAD && byZone != pBotInfo->m_bNation
			|| (byZone >= ZONE_KARUS_ESLANT
				&& byZone <= ZONE_ELMORAD_ESLANT
				&& byZone != (pBotInfo->m_bNation + 10)))
			continue;

		if (pBotInfo->m_bLevel < minlevel)
			continue;

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (Minute * MINUTE);
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;
		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->SetBotAbility();
		pBot->m_BotState = BOT_MERCHANT_MOVE;
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		pBot->StateChangeServerDirect(1, USER_STANDING);
		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_IN);
		return pBot->GetID();
	}
	return true;
}

uint16 CGameServerDlg::SpawnEventBotPk(int Minute, uint8 byZone, float fX, float fY, float fZ, uint8 minlevel, uint8 sNation, uint8 sClass, CUser* pUser)
{
	// If pUser is null, we'll skip map level requirements check
	bool skipMapCheck = (pUser == nullptr);

	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		// Only check map level requirements if we have a valid user
		if (!skipMapCheck)
		{
			switch (byZone)
			{
			case ZONE_RONARK_LAND:
				if (pBotInfo->m_bLevel < pUser->GetMap()->GetMinLevelReq())
					continue;
				break;
			case ZONE_ARDREAM:
				if (pBotInfo->m_bLevel < pUser->GetMap()->GetMinLevelReq()
					|| pBotInfo->m_bLevel > pUser->GetMap()->GetMaxLevelReq())
					continue;
				break;
			case ZONE_RONARK_LAND_BASE:
				if (pBotInfo->m_bLevel < pUser->GetMap()->GetMinLevelReq()
					|| pBotInfo->m_bLevel > pUser->GetMap()->GetMaxLevelReq())
					continue;
				break;
			default:
				continue;
				break;
			}
		}

		if (pBotInfo->m_bLevel < minlevel
			|| pBotInfo->m_bNation != sNation)
			continue;

		_ITEM_DATA* pItem = pBotInfo->GetItem(RIGHTHAND);
		if (pItem == nullptr)
			continue;

		_ITEM_TABLE pTable = GetItemPtr(pItem->nNum);
		if (pTable.isnull())
			continue;

		if (pTable.isShield()
			|| pTable.isPickaxe()
			|| pTable.isFishing())
			continue;

		// Class check - if sClass is 0, allow all classes, otherwise check specific class
		if (sClass > 0 && sClass < 15)
		{
			if (sClass == 1 && !pBotInfo->isWarrior())
				continue;

			if (sClass == 2 && !pBotInfo->isRogue())
				continue;

			if (sClass == 3 && !pBotInfo->isMage())
				continue;

			if (sClass == 4 && !pBotInfo->isPriest())
				continue;

			if (sClass == 14 && !pBotInfo->isPortuKurian())
				continue;

			if (sClass > 4 && sClass < 14)
				continue;
		}
		// If sClass is 0, allow all classes to spawn

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		pBot->m_PlayerKillingLoyaltyDaily = 0;
		pBot->m_PlayerKillingLoyaltyPremiumBonus = 0;

		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (Minute * MINUTE);
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;
		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->m_BotState = BOT_MOVE;
		pBot->m_bPartyLeader = false;
		pBot->m_bGenieStatus = false;

		short sx, sz;
		pBot->GetStartPosition(sx, sz);
		fX = sx;
		fZ = sz;

		pBot->isReset(false);
		pBot->SetBotAbility();
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		pBot->StateChangeServerDirect(1, USER_STANDING);

		if (byZone == ZONE_RONARK_LAND
			|| byZone == ZONE_RONARK_LAND_BASE
			|| byZone == ZONE_ARDREAM)
		{
			C3DMap* pMap = g_pMain->GetZoneByID(byZone);
			if (pMap == nullptr)
				continue;

			TRACE("%s In the Game Insert Player Ranking ZoneID %d\n", pBot->GetName().c_str(), pBot->GetZoneID());
			pBot->AddBotRank(pMap);
		}

		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_IN);
		return pBot->GetID();
	}
	return true;
}

void CGameServerDlg::BotAutoHandlerMainTimer()
{
	std::set<CBot*> mlist;
	try
	{
		g_pMain->m_sMapBotListArray.m_lock.lock();
		auto m_sMapBotListArray = g_pMain->m_sMapBotListArray.m_UserTypeMap;
		g_pMain->m_sMapBotListArray.m_lock.unlock();

		foreach(itr, m_sMapBotListArray)
		{
			CBot* pBot = itr->second;
			if (pBot == nullptr)
				continue;

			if (!pBot->isInGame()
				|| pBot->m_sLoginBotType == 0)
				continue;

			mlist.insert(pBot);
		}

		foreach(itr, mlist)
		{
			auto* pBot = *itr;
			if (!pBot) continue;

			if (pBot->m_sLoginBotType == BOT_NPC_MOVE_TOWN && pBot->m_StartMoveTime > 0 && (UNIXTIME > pBot->m_StartMoveTime))
			{
				ULONGLONG time = GetTickCount64();

				if (time > pBot->m_sMoveTimeCode)
					pBot->BotTownGo();
			}
			else if (pBot->m_sLoginBotType == BOT_NPC_MOVE_HELMES && pBot->m_StartMoveTime > 0 && (UNIXTIME > pBot->m_StartMoveTime))
			{
				ULONGLONG time = GetTickCount64();

				if (time > pBot->m_sMoveTimeCode)
					pBot->BotHemesGo();
			}
			else if (pBot->m_sLoginBotType == BOT_NPC_MOVE_UNW && pBot->m_StartMoveTime > 0 && (UNIXTIME > pBot->m_StartMoveTime))
			{
				ULONGLONG time = GetTickCount64();

				if (time > pBot->m_sMoveTimeCode)
					pBot->BotHemesGoStop();
			}

			if (pBot->m_tBlinkExpiryTime > 0 && UNIXTIME > pBot->m_tBlinkExpiryTime)
			{
				pBot->m_bRegeneType = REGENE_NORMAL;
				pBot->StateChangeServerDirect(3, ABNORMAL_NORMAL);
				pBot->m_tBlinkExpiryTime = 0;
			}
		}
	}
	catch (std::exception& ex)
	{
#ifdef _DEBUG
		std::string information = string_format("Fixed a Critical Error with Bot Theard system: %s\n", ex.what());
		printf(information.c_str());
		ASSERT(0); /* fix me */
#endif
	}
}

void CGameServerDlg::BotHandlerMainTimer()
{
	try
	{
		DWORD currentTime = GetTickCount();
		time_t fTime2 = getMSTime();

		std::vector<CBot*> botsToRemove;

		g_pMain->m_sMapBotListArray.m_lock.lock();
		auto m_sMapBotListArray = g_pMain->m_sMapBotListArray.m_UserTypeMap;
		g_pMain->m_sMapBotListArray.m_lock.unlock();

		foreach(itr, m_sMapBotListArray)
		{
			CBot* pBot = itr->second;
			if (!pBot || !pBot->isInGame())
				continue;

			if (HandleWarpTime(pBot, currentTime))
			{
				botsToRemove.push_back(pBot);
				continue;
			}

			if (pBot->m_sLoginBotType <= 0)
				HandleNormalBot(pBot, fTime2, currentTime);
			else
				HandleLoginBot(pBot, fTime2, currentTime);
		}

		RemoveBots(botsToRemove);
	}
	catch (const std::exception& ex)
	{
#ifdef _DEBUG
		std::string information = string_format("Fixed a Critical Error with Bot Thread system: %s\n", ex.what());
		printf(information.c_str());
		ASSERT(0); // Fix me
#endif
	}
}

bool CGameServerDlg::HandleWarpTime(CBot* pBot, DWORD currentTime)
{
	if (pBot->LastWarpTime > 0 && pBot->LastWarpTime < UNIXTIME)
		return true;
	return false;
}

void CGameServerDlg::HandleNormalBot(CBot* pBot, time_t fTime2, DWORD currentTime)
{
	HandleGameStartMagic(pBot);
	HandleBotActions(pBot, fTime2, currentTime);
	SetBotSpeed(pBot);
}

void CGameServerDlg::HandleLoginBot(CBot* pBot, time_t fTime2, DWORD currentTime)
{
	HandleGameStartMagic(pBot);
	HandleBotActions(pBot, fTime2, currentTime);
	if (pBot->m_sLoginBotType == BOT_FARMER)
		pBot->FindNewMonsterAttackSlot();
	SetBotSpeed(pBot);
}

void CGameServerDlg::HandleGameStartMagic(CBot* pBot)
{
	if (pBot->m_tGameStartTimeSavedMagic != 0 && (UNIXTIME - pBot->m_tGameStartTimeSavedMagic) >= 2)
	{
		pBot->m_tGameStartTimeSavedMagic = 0;
		pBot->InitType4();
		pBot->RecastSavedMagic();
		if (pBot->isInPKZone())
			pBot->Type4Change();
	}
}

void CGameServerDlg::HandleBotActions(CBot* pBot, time_t fTime2, DWORD currentTime)
{
	if (pBot->isRegionTargetUp())
		pBot->RegionFindAttackProcess();

	time_t dwTickTime = fTime2 - pBot->m_fHPChangeTime;
	if (2 * SECOND < dwTickTime)
		pBot->HpMpChange();

	if (pBot->hasRival() && pBot->hasRivalryExpired())
		pBot->RemoveRival();

	if (pBot->ReplyStatus == 1 && currentTime > pBot->ReplyTime)
		HandleBotReply(pBot, currentTime);

	HandleBotState(pBot);
	HandleBotSpeedCheck(pBot, fTime2);
}

void CGameServerDlg::HandleBotReply(CBot* pBot, DWORD currentTime)
{
	CUser* pUser = g_pMain->GetUserPtr(pBot->ReplyID);
	if (pUser != nullptr)
	{
		std::string strUserID;
		std::string PMdetay = string_format("%s", pBot->ReplyChat.c_str());
		strUserID = pBot->GetName();
		Packet result1;
		ChatPacket::Construct(&result1, PRIVATE_CHAT, &PMdetay, &strUserID, pUser->GetNation());
		pUser->Send(&result1);
	}

	pBot->ReplyTime = 0;
	pBot->ReplyStatus = 0;
	pBot->ReplyID = 0;
	pBot->ReplyChat = "";
}

void CGameServerDlg::HandleBotState(CBot* pBot)
{
	switch (pBot->GetBotState())
	{
	case BOT_MINING:
		pBot->BotMining();
		break;
	case BOT_FISHING:
		pBot->BotFishing();
		break;
	case BOT_MERCHANT:
		pBot->BotMerchant();
		break;
	case BOT_DEAD:
		pBot->Regene(INOUT_IN, pBot->isInPKZone() ? 0 : 112754);
		break;
	case BOT_FARMER:
	case BOT_FARMERS:
		pBot->FindMonsterAttackSlot();
		break;
	case BOT_MOVE:
		pBot->MoveProcessGoDeahTown();
		break;
	case BOT_MERCHANT_MOVE:
		pBot->MerchantMoveProcess();
		break;
	case BOT_AFK:
		break;
	}

	if (pBot->GetBotState() != BOT_DEAD)
		pBot->m_fDelayTime = getMSTime();
}

void CGameServerDlg::HandleBotSpeedCheck(CBot* pBot, time_t fTime2)
{
	time_t dwTickTimeType4 = fTime2 - pBot->m_fHPType4CheckTime;
	if (500 < dwTickTimeType4 && pBot->isAlive())
	{
		pBot->HPTimeChangeType3();
		pBot->Type4Duration();
		pBot->CheckSavedMagic();
		pBot->m_fHPType4CheckTime = getMSTime();
	}
}

void CGameServerDlg::SetBotSpeed(CBot* pBot)
{
	float nMaxSpeed = 45.0f;

	if (pBot->isInPKZone())
	{
		if (pBot->GetFame() == COMMAND_CAPTAIN || pBot->isRogue())
			nMaxSpeed = 90.0f;
		else if (pBot->isWarrior() || pBot->isMage() || pBot->isPriest())
			nMaxSpeed = 67.0f;
		else if (pBot->isPortuKurian())
			nMaxSpeed = 67.0f;
	}

	pBot->m_sSpeed = nMaxSpeed;
}

void CGameServerDlg::RemoveBots(const std::vector<CBot*>& botsToRemove)
{
	for (auto& bot : botsToRemove)
	{
		bot->UserInOut(INOUT_OUT);
		g_pMain->RemoveMapBotList(bot->GetID(), bot->GetName());
	}
}

COMMAND_HANDLER(CUser::HandleMerchantBotCommand)
{
	if (!isGM())
		return true;

	if (g_pMain->m_bMerchantBotStatus) {
		g_pMain->SendHelpDescription(this, "Please reloading table.");
		return true;
	}

	if (vargs.size() < 3) {
		g_pMain->SendHelpDescription(this, "Using Sample : /merchantbot Count Time AreaType.");
		return true;
	}

	uint16 botCount = 0;
	if (!vargs.empty()) { botCount = atoi(vargs.front().c_str()); vargs.pop_front(); }

	if (!botCount || botCount > 100) {
		g_pMain->SendHelpDescription(this, "The maximum number of robots you can buy cannot exceed 100.");
		return true;
	}

	uint32 botTime = 0;
	if (!vargs.empty()) { botTime = atoi(vargs.front().c_str()); vargs.pop_front(); }

	botTime = botTime * SECOND;

	uint32 botArea = 0;
	if (!vargs.empty()) { botArea = atoi(vargs.front().c_str()); vargs.pop_front(); }
	if (!botArea) {
		g_pMain->SendHelpDescription(this, "Bot Area Error.");
		return true;
	}

	BotMerchantAdd(botCount, botTime, botArea);
	return true;
}

void CUser::BotMerchantAdd(uint16 count, uint32 bTime, uint16 type)
{
	struct _list {
		uint32 index;
		_MERCHANT_BOT_INFO pInfo;
		_list(uint32 index, _MERCHANT_BOT_INFO pInfo) {
			this->index = index;
			this->pInfo = pInfo;
		}
	};
	std::vector<_list> mList;

	uint16 counter = 0; uint32 index = 0;
	g_pMain->pBotInfo.mCoordinate.m_lock.lock();
	auto m_bMerchantCoordinateStatus = g_pMain->pBotInfo.mCoordinate.m_UserTypeMap;
	g_pMain->pBotInfo.mCoordinate.m_lock.unlock();

	foreach(itr, m_bMerchantCoordinateStatus) 
	{
		auto* pCoordinat = itr->second;
		if (!pCoordinat 
			|| pCoordinat->used 
			|| pCoordinat->type != type)
			continue;

		if (counter >= count)
			break;

		counter++;
		index = itr->first;
		pCoordinat->used = true;
		mList.push_back(_list(itr->first, *pCoordinat));
	}

	if (mList.empty())
	{
		if (index) {
			auto* pCoord = g_pMain->pBotInfo.mCoordinate.GetData(index);
			if (pCoord)
				pCoord->used = false;
		}
		g_pMain->SendHelpDescription(this, "List is empty!");
		return;
	}

	int atime = 1; ULONGLONG btime = UNIXTIME2 + (ULONGLONG)(atime * bTime);
	g_pMain->m_addbotlistLock.lock();
	for (auto& itr : mList) {
		g_pMain->m_addbotlist.insert(std::make_pair(itr.index, _botadd(itr.index, btime, (uint8)type)));
		btime += (ULONGLONG)(atime * bTime);
	}
	g_pMain->m_addbotlistLock.unlock();
}

void CGameServerDlg::BotHandlerMainMerchantTimer()
{
	{
		std::set<uint32> removelist;  removelist.clear();
		g_pMain->m_addbotlistLock.lock();
		foreach(itr, g_pMain->m_addbotlist) 
		{
			if (itr->second.addtime > UNIXTIME2)
				continue;

			g_pMain->MerchantBotAddSet(itr->first, itr->second.type);
			removelist.insert(itr->first);
		}
		foreach(itr, removelist) g_pMain->m_addbotlist.erase(*itr);
		g_pMain->m_addbotlistLock.unlock();
	}

	if (g_pMain->m_sMaxSpawnCount > 0 && g_pMain->m_sMaxBotFinish >= g_pMain->m_sMaxSpawnCount)
	{
		g_pMain->m_sMaxSpawnCount = 0;
		g_pMain->m_sBotStepCount = 0;
		g_pMain->m_sBotRespawnTick = 0;
		g_pMain->m_sBotTimeNext = 0;
		g_pMain->m_sMaxBotFinish = 0;
	}

	if (g_pMain->m_sMaxSpawnCount > 0)
	{
		if (GetTickCount64() > g_pMain->m_sBotRespawnTick)
		{
			for (size_t i = 0; i < g_pMain->m_sBotStepCount; i++)
			{
				if (g_pMain->m_sMaxBotFinish >= g_pMain->m_sMaxSpawnCount)
				{
					g_pMain->m_sMaxSpawnCount = 0;
					g_pMain->m_sBotStepCount = 0;
					g_pMain->m_sBotRespawnTick = 0;
					g_pMain->m_sBotTimeNext = 0;
					g_pMain->m_sMaxBotFinish = 0;
					break;
				}

				if (g_pMain->SpawnLoginBot((float)g_pMain->m_sBotAutoX, float(4), (float)g_pMain->m_sBotAutoZ, g_pMain->m_sBotAutoType, g_pMain->m_byZoneID) > 0)
					g_pMain->m_sMaxBotFinish++;
			}
			g_pMain->m_sBotRespawnTick = GetTickCount64() + g_pMain->m_sBotTimeNext;
		}
	}
}

void CGameServerDlg::MerchantBotAddSet(uint32 index, uint8 type) {

	auto* pCoor = pBotInfo.mCoordinate.GetData(index);
	if (!pCoor || !pCoor->used || pCoor->type != type)
		return;

	bool isBuy = pCoor->isBuy;
	uint8 bZone = pCoor->bZoneID;
	uint16 setX = pCoor->setX, setZ = pCoor->setZ;
	int32 direction = pCoor->direction;
	int16 setY = pCoor->setY;

	g_pMain->m_sBotAutoX = setX;
	g_pMain->m_sBotAutoZ = setZ;
	g_pMain->m_byZoneID = bZone;

	std::map<uint32, _MERCHANT_BOT_ITEM> m_pBotItemInfo;

	pBotInfo.mItem.m_lock.lock();
	auto m_sBotInfoItems = pBotInfo.mItem.m_UserTypeMap;
	pBotInfo.mItem.m_lock.unlock();

	foreach(itr, m_sBotInfoItems) {
		auto* pBotInfoItem = itr->second;
		if (!pBotInfoItem 
			|| pBotInfoItem->type != type)
			continue;

		if (m_pBotItemInfo.find(itr->first) != m_pBotItemInfo.end())
			continue;

		m_pBotItemInfo.insert(std::make_pair(itr->first, *pBotInfoItem));
	}

	if (m_pBotItemInfo.empty()) {
		pCoor->used = false;
		return;
	}

	_bot_merchant _merchant{};
	_merchant.isBuy = isBuy;
	_merchant.index = index;
	_merchant.areaType = type;
	memset(_merchant.merc, 0, sizeof(_merchant.merc));

	uint8 max_itemcount = myrand(4, 10);
	uint64 totalprice = 0;
	int my_index = 0;

	for (int i = 0; i < max_itemcount; i++) {

		std::vector<uint32> mlist;
		foreach(itr, m_pBotItemInfo)
			mlist.push_back(itr->second.itemid);

		if (mlist.empty())
			continue;

		uint32 itemid = mlist[myrand(0, (int32)mlist.size() - 1)];
		auto pBaz = m_pBotItemInfo.find(itemid);
		if (pBaz == m_pBotItemInfo.end())
			continue;

		auto pTable = g_pMain->GetItemPtr(itemid);
		if (pTable.isnull())
			continue;

		bool iskc = myrand(0, 100) < 50;
		uint32 min_price = 0; // iskc ? pBaz->second.minKc : pBaz->second.minPrice;
		uint32 max_price = 0; // iskc ? pBaz->second.maxKc : pBaz->second.maxPrice;

		uint8 money_type = 0;
		if (pBaz->second.moneytype == 0) { //0: random - 1:coins  - 2:kc
			if (myrand(0, 100) < 50) money_type = 2;
			else money_type = 1;
		}
		else if (pBaz->second.moneytype == 1)
			money_type = 1;
		else if (pBaz->second.moneytype == 2)
			money_type = 2;

		if (money_type == 1) {
			min_price = pBaz->second.minPrice;
			max_price = pBaz->second.maxPrice;
		}
		else {
			min_price = pBaz->second.minKc;
			max_price = pBaz->second.maxKc;
		}

		if (!min_price && !max_price)
			continue;

		if (min_price > max_price)
			min_price = max_price;

		long long price = myrand(min_price, max_price);
		if (price > COIN_MAX)
			price = COIN_MAX;

		std::string real_price = std::to_string(price);
		if (real_price.empty()) continue;

		if (real_price.size() > 3 && real_price.size() < 12)
		{
			std::string final_price = "";
			for (int i = 0; i < real_price.size(); i++)
				final_price.append("0");

			int z = (int)real_price.size();
			if (real_price.size() == 4 || real_price.size() == 5 || real_price.size() == 11) z -= 2;
			else if (real_price.size() == 6 || real_price.size() == 7) z -= 3;
			else if (real_price.size() == 8 || real_price.size() == 9) z -= 3;
			else if (real_price.size() == 10) z -= 4;

			if (z > 0 && real_price.size() > z)
			{
				for (int i = 0; i < real_price.size() - z; i++)
					final_price[i] = real_price[i];

				price = std::stoull(final_price);

				if (myrand(0, 100) < 15)
					price -= myrand(1, 5);
			}
		}

		if (totalprice + price >= COIN_MAX)
			break;

		totalprice += price;

		uint32 minItemCount = pBaz->second.minItemCount, maxItemCount = pBaz->second.maxItemCount;
		if (minItemCount > maxItemCount)
			minItemCount = maxItemCount;

		if (!minItemCount) minItemCount = 1;
		if (!maxItemCount) maxItemCount = 1;

		_merchant.merc[my_index].itemid = itemid;
		_merchant.merc[my_index].price = (uint32)price;
		_merchant.merc[my_index].iskc = money_type == 2 ? true : false;
		_merchant.merc[my_index].count = myrand(minItemCount, maxItemCount);
		_merchant.merc[my_index].pTable = pTable;
		my_index++;
		m_pBotItemInfo.erase(pBaz->second.itemid);
	}

	uint8 m_bzaa_count = 0;
	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
		if (_merchant.merc[i].itemid)
			m_bzaa_count++;

	if (!m_bzaa_count)
		return;

	g_pMain->SpawnUserBot(0, bZone, (float)setX, (float)setY, (float)setZ, 6, 1, direction, 0, 0, _merchant);
}

uint16 CGameServerDlg::SpawnUserBot(int Minute, uint8 byZone, float fX, float fY, float fZ, uint8 Restipi, uint8 minlevel /* = 1*/, int16 direction, uint32 SaveID, uint8 Class, _bot_merchant _merchant)
{
	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		if (byZone <= ZONE_ELMORAD && byZone != pBotInfo->m_bNation
			|| (byZone >= ZONE_KARUS_ESLANT
				&& byZone <= ZONE_ELMORAD_ESLANT
				&& byZone != (pBotInfo->m_bNation + 10)))
			continue;

		if (pBotInfo->m_bLevel < minlevel)
			continue;

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		pBot->m_bMerchantState = (SaveID > 0 ? MERCHANT_STATE_SELLING : MERCHANT_STATE_NONE);
		pBot->LastWarpTime = 0;
		pBot->m_sMerchantAreaType = 0;

		if (Minute > 0)
			pBot->LastWarpTime = UNIXTIME + (Minute * 60);

		pBot->m_sDirection = direction;
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;
		pBot->m_bMerchantIndex = 0;
		pBot->m_iGold = myrand(1000, 5000000);

		if (Restipi == 13 || Restipi == 14)
		{
			_ITEM_DATA* pItem = &pBot->m_sItemArray[RIGHTHAND];

			_ITEM_TABLE pTable = GetItemPtr(pItem->nNum);
			if (pTable.isnull())
				continue;

			// Class check - if Class is 0, allow all classes, otherwise check specific class
			if (Class > 0)
			{
				if (Class == 1 && !pBot->isWarrior())
					continue;

				if (Class == 2 && !pBot->isRogue())
					continue;

				if (Class == 3 && !pBot->isMage())
					continue;

				if (Class == 4 && !pBot->isPriest())
					continue;
			}

			if (pBot->isRogue() && !pTable.isBow())
				continue;

			if (pBot->isWarrior() 
				|| pTable.isShield() 
				|| pTable.isPickaxe() 
				|| pTable.isFishing())
				continue;

			pBot->m_bGenieStatus = 1;

			if (Restipi == 14)
				pBot->m_bPartyLeader = true;
			else
				pBot->m_bPartyLeader = false;
		}
		else if (Restipi == 50)
		{
			_BOT_SAVE_DATA* pAuto = g_pMain->m_bBotSaveDataArray.GetData(SaveID);
			if (pAuto == nullptr)
				return 0;

			std::string advertMessage = "test";

			uint16 bResult = 1;
			uint8 MerchantItemleri = 0;
			for (int i = 0; i < MAX_MERCH_ITEMS; i++) { if (pAuto->nNum[i] != 0)					MerchantItemleri++; }

			if (MerchantItemleri == 0)
				return false;

			_MERCH_DATA	m_arNewItems[MAX_MERCH_ITEMS]{};

			if (pAuto->sMerchanType == 0)
			{
				Packet result(WIZ_MERCHANT, uint8(MERCHANT_INSERT));

				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				{
					int8 sItemSlot = pBot->FindSlotForItem(pAuto->nNum[i], pAuto->sCount[i]);
					if (sItemSlot < 0)
						continue;

					auto* pData = pBot->GetItem(sItemSlot);
					if (!pData 
						|| pData->nNum != 0)
						continue;

					pData->nNum = pAuto->nNum[i];
					pData->sCount = pAuto->sCount[i];
					pData->sDuration = pAuto->sDuration[i];
					pData->nSerialNum = pAuto->nSerialNum[i];
					pData->MerchItem = true;

					m_arNewItems[i].sCount = pAuto->sCount[i];
					m_arNewItems[i].bCount = pAuto->sCount[i];
					m_arNewItems[i].nNum = pAuto->nNum[i];
					m_arNewItems[i].IsSoldOut = pAuto->IsSoldOut[i];
					m_arNewItems[i].sDuration = pAuto->sDuration[i];
					m_arNewItems[i].nPrice = pAuto->nPrice[i];
					m_arNewItems[i].nSerialNum = pAuto->nSerialNum[i];
					m_arNewItems[i].bOriginalSlot = sItemSlot;
					m_arNewItems[i].isKC = pAuto->isKc[i];
				}

				uint8 reqcount = 0;
				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					if (m_arNewItems[i].nNum)
						reqcount++;

				if (!reqcount)
					return false;

				uint8 nRandom = 3;
				std::string asdasd = "";
				if (!asdasd.empty())
					pBot->MerchantChat = string_format("%s(Location:%d,%d)", asdasd.c_str(), pBot->GetSPosX() / 10, pBot->GetSPosZ() / 10);

				pBot->MerchantChat.clear();
				pBot->m_iLoyalty = myrand(3000, 5000);
				pBot->m_bPremiumMerchant = 0;
				pBot->m_bMerchantState = MERCHANT_STATE_SELLING;
				pBot->m_BotState = BOT_MERCHANT;

				result << bResult << advertMessage << uint32(pBot->GetID())
					<< pBot->m_bPremiumMerchant;

				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					pBot->m_arMerchantItems[i] = m_arNewItems[i];

				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					result << pBot->m_arMerchantItems[i].nNum;

				pBot->SendToRegion(&result);
			}

			if (pAuto->sMerchanType == 1)
			{
				Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_REGION_INSERT));

				for (int i = 0; i < MAX_MERCH_ITEMS; i++) { pBot->m_arMerchantItems[i].nNum = pAuto->nNum[i];					pBot->m_arMerchantItems[i].sCount = pAuto->sCount[i];					pBot->m_arMerchantItems[i].nPrice = pAuto->nPrice[i];					pBot->m_arMerchantItems[i].sDuration = pAuto->sDuration[i];					pBot->m_arMerchantItems[i].isKC = pAuto->isKc[i]; }
				pBot->m_bMerchantState = MERCHANT_STATE_BUYING;
				pBot->m_BotState = BOT_MERCHANT;
				result << uint32(pBot->GetID());

				for (int i = 0; i < 4; i++)
					result << pBot->m_arMerchantItems[i].nNum;

				pBot->SendToRegion(&result);
			}
		}
		else if (Restipi == 1)
		{
			_ITEM_DATA* pItem = &pBot->m_sItemArray[RIGHTHAND];
			if (pItem)
			{
				auto pTable = GetItemPtr(myrand(0, 100) > 50 ? GOLDEN_MATTOCK : MATTOCK);
				if (pTable.isnull())
					continue;

				_ITEM_DATA* pTItem = &pBot->m_sItemArray[LEFTHAND];
				if (pTItem) memset(pTItem, 0x00, sizeof(_ITEM_DATA));

				memset(pItem, 0x00, sizeof(_ITEM_DATA));
				pItem->nNum = pTable.m_iNum;
				pItem->nSerialNum = GenerateItemSerial();
				pItem->sCount = 1;
				pItem->sDuration = pTable.m_sDuration;
				pBot->m_bResHpType = USER_MINING;
			}
		}
		else if (Restipi == 2)
		{
			_ITEM_DATA* pItem = &pBot->m_sItemArray[RIGHTHAND];
			if (pItem)
			{
				auto pTable = GetItemPtr(myrand(0, 100) > 50 ? GOLDEN_FISHING : FISHING);
				if (pTable.isnull())
					continue;

				auto* pTItem = &pBot->m_sItemArray[LEFTHAND];
				if (pTItem != nullptr)
					memset(pTItem, 0x00, sizeof(_ITEM_DATA));

				memset(pItem, 0x00, sizeof(_ITEM_DATA));
				pItem->nNum = pTable.m_iNum;
				pItem->nSerialNum = g_pMain->GenerateItemSerial();
				pItem->sCount = 1;
				pItem->sDuration = pTable.m_sDuration;
				pBot->m_bResHpType = USER_FLASHING;
			}
		}
		else if (Restipi == 3 || Restipi == 4)
			pBot->m_bResHpType = Restipi == 3 ? USER_STANDING : USER_SITDOWN;
		else if (Restipi == 5)
			pBot->m_bResHpType = USER_STANDING;// Random > 5000 ? USER_STANDING : USER_SITDOWN;
		else if (Restipi == 6) {}
		else continue;

		if (Restipi == 6)
		{
			pBot->m_bMerchantIndex = _merchant.index;
			pBot->m_sMerchantAreaType = _merchant.areaType;
			pBot->m_BotState = BOT_MERCHANT;

			uint8 itemcount = 0;
			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
			{
				if (_merchant.merc[i].itemid)
					itemcount++;
			}

			_MERCH_DATA	m_arNewItems[MAX_MERCH_ITEMS]{};
			memset(m_arNewItems, 0, sizeof(m_arNewItems));

			if (!_merchant.isBuy) {

				for (int i = 0; i < MAX_MERCH_ITEMS; i++) {
					if (!_merchant.merc[i].itemid)
						continue;

					int8 sItemSlot = pBot->FindSlotForItem(_merchant.merc[i].itemid, _merchant.merc[i].count);
					if (sItemSlot < 0)
						continue;

					auto pItem = g_pMain->GetItemPtr(_merchant.merc[i].itemid);
					if (pItem.isnull())
						continue;

					auto* pData = pBot->GetItem(sItemSlot);
					if (!pData || (pData->nNum != 0 && !pItem.m_bCountable))
						continue;

					pData->nNum = _merchant.merc[i].itemid;
					pData->sCount += _merchant.merc[i].count;
					pData->sDuration = _merchant.merc[i].pTable.m_sDuration;
					pData->nSerialNum = g_pMain->GenerateItemSerial();
					pData->MerchItem = true;

					m_arNewItems[i].sCount = pData->sCount;
					m_arNewItems[i].bCount = pData->sCount;
					m_arNewItems[i].nNum = pData->nNum;
					m_arNewItems[i].IsSoldOut = false;
					m_arNewItems[i].sDuration = pData->sDuration;
					m_arNewItems[i].nPrice = _merchant.merc[i].price;
					m_arNewItems[i].nSerialNum = pData->nSerialNum;
					m_arNewItems[i].bOriginalSlot = sItemSlot;
					m_arNewItems[i].isKC = _merchant.merc[i].iskc;
				}

				uint8 reqcount = 0;
				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					if (m_arNewItems[i].nNum)
						reqcount++;

				if (!reqcount)
				{
					if (_merchant.index) {
						auto* pCoord = g_pMain->pBotInfo.mCoordinate.GetData(_merchant.index);
						if (pCoord)
							pCoord->used = false;
					}
					return 0;
				}

				pBot->m_bPremiumMerchant = myrand(0, 100) < 15;
				pBot->m_bMerchantState = MERCHANT_STATE_SELLING;
			}
			else {

				uint32 total_price = 0;
				for (int i = 0; i < MAX_MERCH_ITEMS; i++) {
					m_arNewItems[i].sCount = _merchant.merc[i].count;
					m_arNewItems[i].bCount = _merchant.merc[i].count;
					m_arNewItems[i].nNum = _merchant.merc[i].itemid;
					m_arNewItems[i].sDuration = _merchant.merc[i].pTable.m_sDuration;
					m_arNewItems[i].nPrice = _merchant.merc[i].price;
					m_arNewItems[i].isKC = _merchant.merc[i].iskc;
					m_arNewItems[i].bOriginalSlot = i;
					total_price += _merchant.merc[i].price;
				}

				uint8 reqcount = 0;
				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
					if (m_arNewItems[i].nNum)
						reqcount++;

				if (!reqcount)
				{
					if (_merchant.index) {
						auto* pCoord = g_pMain->pBotInfo.mCoordinate.GetData(_merchant.index);
						if (pCoord)
							pCoord->used = false;
					}
					return 0;
				}

				pBot->m_bPremiumMerchant = myrand(0, 100) < 15;
				pBot->m_bMerchantState = MERCHANT_STATE_BUYING;

				if (pBot->m_iGold < total_price)
					pBot->m_iGold = myrand(total_price, total_price + 5000000);
			}

			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				pBot->m_arMerchantItems[i] = m_arNewItems[i];
		}

		int Random = myrand(0, 10000);
		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->m_BotState = BOT_AFK;
		pBot->SetBotAbility();
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_IN);
		pBot->StateChangeServerDirect(1, Random > 5000 ? USER_STANDING : USER_SITDOWN);
		return pBot->GetID();
	}
	return 0;
}

COMMAND_HANDLER(CUser::HandleLoginBotCommand)
{
	if (!isGM())
		return false;

	if (isInPKZone())
		return true;

	if (vargs.size() < 5)
	{
		g_pMain->SendHelpDescription(this, "Town [1] - Hemes [2] - Count - Zone - StepCount - Time");
		return true;
	}

	uint8 sType = atoi(vargs.front().c_str());
	vargs.pop_front();

	uint16 sCount = atoi(vargs.front().c_str());
	vargs.pop_front();

	uint8 sZoneID = atoi(vargs.front().c_str());
	vargs.pop_front();

	uint16 sStep = atoi(vargs.front().c_str());
	vargs.pop_front();

	uint16 sTime = atoi(vargs.front().c_str());
	vargs.pop_front();

	if (sType > 3 || sType <= 0 || sZoneID == 0 || sZoneID < 0)
	{
		g_pMain->SendHelpDescription(this, "Login bot type not found!");
		return true;
	}

	if (sType == 1)
		sType = BOT_NPC_MOVE_TOWN;
	else if (sType == 2)
		sType = BOT_NPC_MOVE_HELMES;
	else
		sType = BOT_FARMER;

	uint8 RandomX = myrand(1, 2);

	if (sCount > 1 && sStep > 0 && sTime > 0 && sZoneID == ZONE_MORADON)
	{
		if (g_pMain->m_sMaxSpawnCount > 0)
		{
			g_pMain->SendHelpDescription(this, "Login Bot Start Active.");
			return true;
		}

		g_pMain->m_sMaxSpawnCount = sCount;
		g_pMain->m_sBotStepCount = sStep;
		g_pMain->m_sBotTimeNext = sTime;
		g_pMain->m_sBotAutoType = sType;

		if (RandomX == 1)
		{
			g_pMain->m_sBotAutoX = 264;
			g_pMain->m_sBotAutoZ = 302;
			g_pMain->m_byZoneID = sZoneID;
		}
		else
		{
			g_pMain->m_sBotAutoX = 264;
			g_pMain->m_sBotAutoZ = 302;
			g_pMain->m_byZoneID = sZoneID;
		}

		g_pMain->SendHelpDescription(this, "Login Bot Started.");
		return true;
	}

	g_pMain->m_byZoneID = sZoneID;

	if (RandomX == 1)
	{
		if (sCount > 1 && sStep == 0 && sTime == 0)
		{
			for (size_t i = 0; i < sCount; i++)
			{
				if (sType == BOT_FARMER)
				{
					float Bonc = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;
					float Bonc2 = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;

					if (g_pMain->SpawnLoginBot(GetX(), GetY() + Bonc, GetZ() + Bonc2, sType, g_pMain->m_byZoneID) > 0)
						g_pMain->SendHelpDescription(this, "Login bot added");
				}
				else
				{
					if (g_pMain->SpawnLoginBot(264, 4, 302, sType, g_pMain->m_byZoneID) > 0)
						g_pMain->SendHelpDescription(this, "Login bot added");
				}
			}
		}
		else
		{
			if (sType == BOT_FARMER)
			{
				float Bonc = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;
				float Bonc2 = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;

				if (g_pMain->SpawnLoginBot(GetX(), GetY() + Bonc, GetZ() + Bonc2, sType, g_pMain->m_byZoneID) > 0)
					g_pMain->SendHelpDescription(this, "Login bot added");
			}
			else
			{
				if (g_pMain->SpawnLoginBot(264, 4, 302, sType, g_pMain->m_byZoneID) > 0)
					g_pMain->SendHelpDescription(this, "Login bot added");
			}
		}
	}
	else
	{
		if (sCount > 1 && sStep == 0 && sTime == 0)
		{
			for (size_t i = 0; i < sCount; i++)
			{
				if (sType == BOT_FARMER)
				{
					float Bonc = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;
					float Bonc2 = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;

					if (g_pMain->SpawnLoginBot(GetX(), GetY() + Bonc, GetZ() + Bonc2, sType, g_pMain->m_byZoneID) > 0)
						g_pMain->SendHelpDescription(this, "Login bot added");
				}
				else
				{
					if (g_pMain->SpawnLoginBot(264, 4, 302, sType, g_pMain->m_byZoneID) > 0)
						g_pMain->SendHelpDescription(this, "Login bot added");
				}
			}
		}
		else
		{
			if (sType == BOT_FARMER)
			{
				float Bonc = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;
				float Bonc2 = sCount > 1 ? myrand(1, 6) * 1.0f : 0.0f;

				if (g_pMain->SpawnLoginBot(GetX(), GetY() + Bonc, GetZ() + Bonc2, sType, g_pMain->m_byZoneID) > 0)
					g_pMain->SendHelpDescription(this, "Login bot added");
			}
			else
			{
				if (g_pMain->SpawnLoginBot(264, 4, 302, sType, g_pMain->m_byZoneID) > 0)
					g_pMain->SendHelpDescription(this, "Login bot added");
			}
		}
	}

	return true;
}

uint16 CGameServerDlg::SpawnLoginBot(float fX, float fY, float fZ, uint8 type, uint8 byZone)
{
	m_ArtificialIntelligenceArray.m_lock.lock();
	auto m_sArtificialIntelligenceArray = m_ArtificialIntelligenceArray.m_UserTypeMap;
	m_ArtificialIntelligenceArray.m_lock.unlock();

	foreach(itr, m_sArtificialIntelligenceArray)
	{
		_BOT_DATA* pBotInfo = itr->second;
		if (pBotInfo == nullptr)
			continue;

		if (byZone <= ZONE_ELMORAD && byZone != pBotInfo->m_bNation
			|| (byZone >= ZONE_KARUS_ESLANT
				&& byZone <= ZONE_ELMORAD_ESLANT
				&& byZone != (pBotInfo->m_bNation + 10)))
			continue;

		if (type == BOT_FARMER)
		{
			_ITEM_DATA* pItem = pBotInfo->GetItem(RIGHTHAND);
			if (pItem == nullptr)
				continue;

			_ITEM_TABLE pTable = GetItemPtr(pItem->nNum);
			if (pTable.isnull())
				continue;

			if (pTable.isShield()
				|| pTable.isPickaxe()
				|| pTable.isFishing())
				continue;

			if (pBotInfo->m_bLevel < MIN_LEVEL_NATION_BASE)
				continue;
		}
		else 
		{
			if (pBotInfo->m_bLevel > MIN_LEVEL_NATION_BASE)
				continue;
		}

		// Check if bot already exist
		CBot* pBotCheck = GetBotPtr(pBotInfo->m_sSid);
		if (pBotCheck != nullptr)
			continue;

		CBot* pBot = new CBot();

		pBot->m_strUserID = pBotInfo->m_strUserID;
		pBot->m_bNation = pBotInfo->m_bNation;
		pBot->m_bRace = pBotInfo->m_bRace;
		pBot->m_sClass = pBotInfo->m_sClass;
		pBot->m_nHair = pBotInfo->m_nHair;
		pBot->m_bLevel = pBotInfo->m_bLevel;
		pBot->m_bFace = pBotInfo->m_bFace;
		pBot->m_bKnights = pBotInfo->m_bKnights;
		pBot->m_bFame = pBotInfo->m_bFame;

		memcpy(pBot->m_sItemArray, pBotInfo->m_sItemArray, sizeof(pBot->m_sItemArray));
		memcpy(pBot->m_bstrSkill, pBotInfo->m_bstrSkill, sizeof(pBot->m_bstrSkill));
		memcpy(pBot->m_bStats, pBotInfo->m_bStats, sizeof(pBot->m_bStats));

		pBot->m_sSid = pBotInfo->m_sSid;
		pBot->m_sAchieveCoverTitle = pBotInfo->m_sAchieveCoverTitle;
		pBot->m_reblvl = pBotInfo->m_reblvl;
		pBot->m_iGold = pBotInfo->m_iGold;
		pBot->m_sPoints = pBotInfo->m_sPoints;
		pBot->m_iLoyalty = pBotInfo->m_iLoyalty;
		pBot->m_iLoyaltyMonthly = pBotInfo->m_iLoyaltyMonthly;

		int Random = myrand(0, 10000);
		pBot->m_bMerchantState = MERCHANT_STATE_NONE;
		pBot->LastWarpTime = UNIXTIME + (10 * 60);
		pBot->m_pMap = GetZoneByID(byZone);
		pBot->m_bZone = byZone;
		pBot->m_sTargetID = int16(-1);
		pBot->m_TargetChanged = false;
		pBot->m_bBlockPrivateChat = false;
		pBot->SetBotAbility();
		pBot->m_sLoginBotType = type;
		pBot->m_bAbnormalType = ABNORMAL_BLINKING;
		pBot->m_tBlinkExpiryTime = UNIXTIME + BLINK_TIME;
		uint8 randtime = myrand(3, 10);
		pBot->m_StartMoveTime = UNIXTIME + randtime;
		pBot->m_bRegeneType = REGENE_ZONECHANGE;
		pBot->SetPosition(fX, fY, fZ);
		pBot->SetRegion(pBot->GetNewRegionX(), pBot->GetNewRegionZ());
		pBot->SetZoneAbilityChange(pBot->GetZoneID());
		AddMapBotList(pBot);
		pBot->UserInOut(INOUT_RESPAWN);
		pBot->StateChangeServerDirect(3, ABNORMAL_BLINKING);
		return pBot->GetID();
	}
	return 0;
}