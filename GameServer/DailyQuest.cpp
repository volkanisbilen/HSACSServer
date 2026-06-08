#include "StdAfx.h"
#include "DBAgent.h"

int32 GetRemeaningtime(uint32 time) {return time - (uint32)UNIXTIME;}
void CUser::UpdateDailyQuestState(_DAILY_USERQUEST* pquest, uint8 newstate) {if(pquest)pquest->status = newstate;}

void CUser::DailyQuestFinished(_DAILY_USERQUEST* puserq, _DAILY_QUEST* pquest) {
	if (!puserq || !pquest) return;

	if (pquest->replaytime) puserq->replaytime = (uint32)UNIXTIME + (pquest->replaytime * HOUR);
	if (pquest->timetype == (uint8)DailyQuesttimetype::repeat) UpdateDailyQuestState(puserq, (uint8)DailyQuestStatus::ongoing);
	else if (pquest->timetype == (uint8)DailyQuesttimetype::single) UpdateDailyQuestState(puserq, (uint8)DailyQuestStatus::comp);
	else if (pquest->timetype == (uint8)DailyQuesttimetype::time) UpdateDailyQuestState(puserq, (uint8)DailyQuestStatus::timewait);
	puserq->kcount = 0;

	Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::DailyQuestReward));
	newpkt << pquest->index;
	g_pMain->AddDatabaseRequest(newpkt, this);
}


void CUser::ReqDailyQuestSendReward(Packet &pkt) {
	uint8 index = pkt.read<uint8>();
	_DAILY_QUEST* pquest = g_pMain->m_DailyQuestArray.GetData(index);
	if (!pquest) return;

	struct list { uint32 itemid; uint32 itemcount; list(uint32 a, uint32 b){itemid = a; itemcount = b;}};

	std::map <uint8, list> mlist; uint8 counter = 0;
	std::vector<uint8> mdeleted;

	bool randitem = false;
	for (uint8 i = 0; i < 4; i++) {
		if (pquest->rewaditemid[i] && pquest->rewarditemcount[i]) {

			if (pquest->rewaditemid[i] == 900004000) {
				if (pquest->randomid) 
					randitem = true;
			}
			else
				mlist.insert(std::make_pair(counter++, list(pquest->rewaditemid[i], pquest->rewarditemcount[i])));
		}
	}

	if (randitem) {
		std::vector<uint32> m_randlist;
		foreach(itr, g_pMain->m_RandomItemArray) {
			auto* pRandom = *itr;
			if (!pRandom || pRandom->SessionID != pquest->randomid) continue;
			m_randlist.push_back(pRandom->ItemID);
		}
		if (m_randlist.size()) {
			uint32 n_randitem = m_randlist[myrand(0, (int32)m_randlist.size() - 1)];
			if (n_randitem) mlist.insert(std::make_pair(counter++, list(n_randitem, 1)));
		}
	}

	if (mlist.empty()) return;

	QuestV2ShowGiveItem(pquest->rewaditemid[0], pquest->rewarditemcount[0],
		pquest->rewaditemid[1], pquest->rewarditemcount[1],
		pquest->rewaditemid[2], pquest->rewarditemcount[2],
		pquest->rewaditemid[3], pquest->rewarditemcount[3]);

	bool lettercheck = false;
	foreach(itr, mlist) {
		_ITEM_TABLE pTable = g_pMain->GetItemPtr(itr->second.itemid);
		if (pTable.isnull()) continue;

		if (pTable.GetNum() == ITEM_EXP) {
			ExpChange("daily quest", itr->second.itemcount, true);
			continue;
		}
		else if (pTable.GetNum() == ITEM_GOLD) {
			GoldGain(itr->second.itemcount, true, false);
			continue;
		}
		else if (pTable.GetNum() == ITEM_COUNT || pTable.GetNum() == ITEM_LADDERPOINT) {
			SendLoyaltyChange("Daily Quest",itr->second.itemcount);
			continue;
		}

		int8 pos = FindSlotForItem(itr->second.itemid, itr->second.itemcount);
		if (pos > 0) {
			GiveItem("Daily Quest", itr->second.itemid, itr->second.itemcount,true);
			continue;
		}

		std::string SenderName = "ADMIN", Subject = "REWARD", Message = "REWARD";
		_ITEM_DATA pItem{};
		pItem.nNum = itr->second.itemid;
		pItem.nSerialNum = g_pMain->GenerateItemSerial();
		pItem.sCount = itr->second.itemcount;
		pItem.sDuration = pTable.m_sDuration;
		pItem.nExpirationTime = 0;
		g_DBAgent.SendLetter(SenderName, GetName(), Subject, Message, 2, &pItem, 0);
		if (!lettercheck) lettercheck = true;
	}
	if (lettercheck) ReqLetterUnread();
}

void CUser::UpdateDailyQuestCount(uint16 m_sMonsteID)
{
	if (m_sDailyQuestMap.IsEmpty() 
		|| !m_sMonsteID)
		return;

	m_sDailyQuestMap.m_lock.lock();
	auto m_bDailyQuestMap = m_sDailyQuestMap.m_UserTypeMap;
	m_sDailyQuestMap.m_lock.unlock();

	foreach(itr, m_bDailyQuestMap)
	{
		if (!itr->second
			|| itr->second->status == (uint8)DailyQuestStatus::comp) 
			continue;

		if (itr->second->status == (uint8)DailyQuestStatus::timewait 
			&& GetRemeaningtime(itr->second->replaytime) > 0) 
			continue;

		auto *pDailyQuest = g_pMain->m_DailyQuestArray.GetData(itr->first);
		if (!pDailyQuest) 
			continue;

		if (pDailyQuest->zoneid) 
		{
			if (pDailyQuest->zoneid == ZONE_MORADON && !isInMoradon()) 
				continue;
			else if (pDailyQuest->zoneid == ZONE_KARUS && !isInLufersonCastle()) 
				continue;
			else if (pDailyQuest->zoneid == ZONE_ELMORAD && !isInElmoradCastle()) 
				continue;
			else if (pDailyQuest->zoneid == ZONE_KARUS_ESLANT && !isInKarusEslant()) 
				continue;
			else if (pDailyQuest->zoneid == ZONE_ELMORAD_ESLANT && !isInKarusEslant()) 
				continue;
			else if (pDailyQuest->zoneid != GetZoneID()) 
				continue;
		}

		if (itr->second->kcount + 1 > pDailyQuest->kcount) 
			continue;

		if ((pDailyQuest->killtype == 0 && isInParty()) 
			|| (pDailyQuest->killtype == 1 && !isInParty())) 
			continue;

		bool killcheck = false;
		for (uint8 i = 0; i < 4; i++) 
		{
			if (m_sMonsteID != pDailyQuest->Mobid[i])
				continue;

			killcheck = true;
		}

		if (!killcheck) 
			continue;

		itr->second->kcount++;
#if(__VERSION < 2369)
		Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::DailyQuest));
		result << uint8(DailyQuestOp::killupdate) 
			<< itr->first << m_sMonsteID;
		Send(&result);

		result.clear();
		result.Initialize(WIZ_HSACS_HOOK);
		result << uint8(0xDC);
		result.SByte();
		result << pDailyQuest->strQuestName 
			<< uint16(itr->second->kcount) 
			<< uint16(pDailyQuest->kcount) 
			<< m_sMonsteID;
		Send(&result);
#endif
		if (itr->second->kcount >= pDailyQuest->kcount)
			DailyQuestFinished(itr->second, pDailyQuest);
	}
}

void CUser::DailyQuestSendList() 
{
	uint16 sCount = 0;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::DailyQuest));
	result << uint8(DailyQuestOp::sendlist) << sCount;
#endif
	g_pMain->m_DailyQuestArray.m_lock.lock();
	auto m_sDailyQuestArray = g_pMain->m_DailyQuestArray.m_UserTypeMap;
	g_pMain->m_DailyQuestArray.m_lock.unlock();

	foreach(itr, m_sDailyQuestArray)
	{
		auto* pDailyQuestList = itr->second;
		if (!pDailyQuestList) 
			continue;
#if(__VERSION < 2369)
		result << pDailyQuestList->index << pDailyQuestList->timetype << pDailyQuestList->killtype;

		for (uint8 i = 0; i < 4; i++) 
			result << pDailyQuestList->Mobid[i] << pDailyQuestList->rewaditemid[i] << pDailyQuestList->rewarditemcount[i];

		result << pDailyQuestList->kcount << pDailyQuestList->zoneid << pDailyQuestList->replaytime << pDailyQuestList->minlevel << pDailyQuestList->maxlevel;
#endif
		sCount++;
	}
#if(__VERSION < 2369)
	result.put(2, sCount);
	Send(&result);
#endif
	sCount = 0;
#if(__VERSION < 2369)
	result.clear();
	result.Initialize(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::DailyQuest) << uint8(DailyQuestOp::userinfo) << sCount;
#endif
	m_sDailyQuestMap.m_lock.lock();
	auto m_bDailyQuestMap = m_sDailyQuestMap.m_UserTypeMap;
	m_sDailyQuestMap.m_lock.unlock();

	foreach(itr, m_bDailyQuestMap)
	{
		if (!itr->second) 
			continue;

		int sTime = GetRemeaningtime(itr->second->replaytime);
		if (sTime < 0)
		{
			sTime = 0;

			if(itr->second->status == (uint8)DailyQuestStatus::timewait) 
				itr->second->status = (uint8)DailyQuestStatus::ongoing;
		}
#if(__VERSION < 2369)
		result << itr->first << itr->second->status << itr->second->kcount << (uint32)sTime;
#endif
		sCount++;
	}
#if(__VERSION < 2369)
	result.put(2, sCount);
	Send(&result);
#endif
}