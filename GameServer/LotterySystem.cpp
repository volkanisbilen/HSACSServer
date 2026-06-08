#include "StdAfx.h"
#include "DBAgent.h"

#define MAX_GIFT_SEND 4

void CGameServerDlg::LotterySystemStart(uint32 ID)
{
	_RIMA_LOTTERY_DB *pLottery = g_pMain->m_RimaLotteryArray.GetData(ID);
	if (pLottery == nullptr 
		|| pLotteryProc.LotteryStart)
	{
		if (pLottery == nullptr)
			return;

		if (pLotteryProc.LotteryStart) 
			return;
	}

	bool m_sItemReward = false, m_sRewardPlus = false;
	for (int i = 0; i < 5; i++) 
	{
		if (pLottery->nReqItem[i] > 0) 
		{
			if (pLottery->nReqItemCount[i] == 0) 
				return;
			
			_ITEM_TABLE pTable = g_pMain->GetItemPtr(pLottery->nReqItem[i]);

			if (pTable.isnull()) 
				return;

			m_sItemReward = true;
		}
	}

	for (int i = 0; i < 4; i++) 
	{
		if (pLottery->nRewardItem[i] > 0) 
		{
			_ITEM_TABLE pTable = g_pMain->GetItemPtr(pLottery->nRewardItem[i]);
			if (pTable.isnull())
				return;

			m_sRewardPlus = true;
		}
	}

	if (!m_sItemReward 
		|| !m_sRewardPlus)
		return;
	
	pLotteryProc.UserLimit = pLottery->UserLimit;

	memcpy(pLotteryProc.nReqItem, pLottery->nReqItem, sizeof(pLottery->nReqItem));
	memcpy(pLotteryProc.nReqItemCount, pLottery->nReqItemCount, sizeof(pLottery->nReqItemCount));
	memcpy(pLotteryProc.nRewardItem, pLottery->nRewardItem, sizeof(pLottery->nRewardItem));

	pLotteryProc.EventTime = pLottery->EventTime;
	pLotteryProc.EventProcessTime = (uint32)UNIXTIME + pLottery->EventTime;
	pLotteryProc.EventStartTime = (uint32)UNIXTIME;
	pLotteryProc.TimerControl = true;
	pLotteryProc.LotteryStart = true;


	uint32 m_sRemaingTime = 0;
	if (g_pMain->pLotteryProc.EventProcessTime > (uint32)UNIXTIME)
		m_sRemaingTime = g_pMain->pLotteryProc.EventProcessTime - (uint32)UNIXTIME;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::LOTTERY));
	result << uint8(1); // baþlat

	for (int i = 0; i < 5; i++)
		result << pLotteryProc.nReqItem[i] << pLotteryProc.nReqItemCount[i];

	for (int i = 0; i < 4; i++) 
		result << pLotteryProc.nRewardItem[i];
#endif
	uint32 j_count = pLotteryProc.UserJoinCounter;
	if (j_count && g_pMain->pRankBug.LotteryJoin) j_count *= g_pMain->pRankBug.LotteryJoin;
#if(__VERSION < 2369)
	result << pLotteryProc.UserLimit << m_sRemaingTime << j_count << uint32(0);
	Send_All(&result);
#endif
	//printf("Lottery Time %d \n", m_sRemaingTime);
	std::string Notice = "Lottery Event started.";
	SendChat<ChatType::WAR_SYSTEM_CHAT>(Notice.c_str(), (uint8)Nation::ALL, true);
	SendChat<ChatType::PUBLIC_CHAT>(Notice.c_str(), (uint8)Nation::ALL, true);
}

void CUser::LotteryGameStartSend()
{
	if (!g_pMain->pLotteryProc.LotteryStart 
		|| g_pMain->pLotteryProc.EventTime == 0)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::LOTTERY));
	result << uint8(1); // baþlat

	for (int c = 0; c < 5; c++) 
		result << g_pMain->pLotteryProc.nReqItem[c] << g_pMain->pLotteryProc.nReqItemCount[c];
		
	for (int v = 0; v < 4; v++)
		result << g_pMain->pLotteryProc.nRewardItem[v];
#endif
	uint32 m_sTicketCount = 0;

	std::string strUserID = GetName();
	STRTOUPPER(strUserID);

	_RIMA_LOOTERY_USER_INFO *pLotInfo = g_pMain->m_RimaLotteryUserInfo.GetData(strUserID);
	if (pLotInfo != nullptr)
		m_sTicketCount = pLotInfo->TicketCount;

	uint32 m_sRemaingTime = 0;
	if (g_pMain->pLotteryProc.EventProcessTime > (uint32)UNIXTIME)
		m_sRemaingTime = g_pMain->pLotteryProc.EventProcessTime - (uint32)UNIXTIME;

	uint32 m_sCount = g_pMain->pLotteryProc.UserJoinCounter;
	if (m_sCount && g_pMain->pRankBug.LotteryJoin) 
		m_sCount *= g_pMain->pRankBug.LotteryJoin;
#if(__VERSION < 2369)
	result << g_pMain->pLotteryProc.UserLimit << m_sRemaingTime << m_sCount << m_sTicketCount;
	Send(&result);
#endif
}

void CUser::HSACSX_LotteryJoinFunction(Packet &pkt)
{
	uint8 subcode;
	pkt >> subcode;

	switch (subcode)
	{
	case 3:
		LotteryJoinFunction();
		break;
	default:
		break;
	}
}

bool CUser::LotteryItemCheck(uint32 ItemID1, uint32 ItemCount1, 
	uint32 ItemID2, uint32 ItemCount2,
	uint32 ItemID3, uint32 ItemCount3,
	uint32 ItemID4, uint32 ItemCount4,
	uint32 ItemID5, uint32 ItemCount5)
{
	if (ItemID1 > 0 && !CheckExistItem(ItemID1, ItemCount1))
		return false;

	if (ItemID2 > 0 && !CheckExistItem(ItemID2, ItemCount2))
		return false;

	if (ItemID3 > 0 && !CheckExistItem(ItemID3, ItemCount3))
		return false;

	if (ItemID4 > 0 && !CheckExistItem(ItemID4, ItemCount4))
		return false;

	if (ItemID5 > 0 && !CheckExistItem(ItemID5, ItemCount5))
		return false;

	return true;
}

void CUser::LotteryJoinFunction()
{
#if(__VERSION < 2369)
	Packet pkt;
#endif
	if (!g_pMain->pLotteryProc.LotteryStart)
	{
#if(__VERSION < 2369)
		pkt.clear();
		pkt.Initialize(WIZ_HSACS_HOOK);
		pkt << uint8(HSACSXOpCodes::LOTTERY) << uint8(3) << uint8(0) << std::string("The Lottery Event has not started.");
		Send(&pkt);
#endif
		return;
	}

	if (g_pMain->pLotteryProc.UserJoinCounter >= g_pMain->pLotteryProc.UserLimit)
	{
#if(__VERSION < 2369)
		pkt.clear();
		pkt.Initialize(WIZ_HSACS_HOOK);
		pkt << uint8(HSACSXOpCodes::LOTTERY) << uint8(3) << uint8(0) << std::string("Limit is insufficient.");
		Send(&pkt);
#endif
		return;
	}

	if (!LotteryItemCheck(g_pMain->pLotteryProc.nReqItem[0], g_pMain->pLotteryProc.nReqItemCount[0],
		g_pMain->pLotteryProc.nReqItem[1], g_pMain->pLotteryProc.nReqItemCount[1],
		g_pMain->pLotteryProc.nReqItem[2], g_pMain->pLotteryProc.nReqItemCount[2],
		g_pMain->pLotteryProc.nReqItem[3], g_pMain->pLotteryProc.nReqItemCount[3],
		g_pMain->pLotteryProc.nReqItem[4], g_pMain->pLotteryProc.nReqItemCount[4])) 
	{
#if(__VERSION < 2369)
		pkt.clear();
		pkt.Initialize(WIZ_HSACS_HOOK);
		pkt << uint8(HSACSXOpCodes::LOTTERY) << uint8(3) << uint8(0) << std::string("No items");
		Send(&pkt);
#endif
		return;
	}

	int nReqGold = 0;
	for (int i = 0; i < 5; i++) 
	{
		uint32 ItemID = g_pMain->pLotteryProc.nReqItem[i];
		uint32 ItemCount = g_pMain->pLotteryProc.nReqItemCount[i];

		if (ItemID <= 0 
			|| ItemCount <= 0) 
			continue;

		if (ItemID == ITEM_GOLD)
			nReqGold += ItemCount;
	}

	if (nReqGold > 0 
		&& !GoldLose(nReqGold)) 
	{
#if(__VERSION < 2369)
		pkt.clear();
		pkt.Initialize(WIZ_HSACS_HOOK);
		pkt << uint8(HSACSXOpCodes::LOTTERY) << uint8(3) << uint8(0) << std::string("You don't have enough money for the lottery event.");
		Send(&pkt);
#endif
		return;
	}

	for (int i = 0; i < 5; i++) 
	{
		uint32 ItemID = g_pMain->pLotteryProc.nReqItem[i];
		uint32 ItemCount = g_pMain->pLotteryProc.nReqItem[i];
		if (!ItemID || !ItemCount)
			continue;

		_ITEM_TABLE pItem = g_pMain->GetItemPtr(ItemID);
		if (pItem.isnull() 
			|| pItem.m_bCountable == 2) 
			continue;

		if (!RobItem(ItemID, ItemCount))
			continue;
	}

	uint32 TicketCount = 0;

	std::string strUserID = GetName();
	STRTOUPPER(strUserID);

	_RIMA_LOOTERY_USER_INFO *pUserInfo = g_pMain->m_RimaLotteryUserInfo.GetData(strUserID);
	if (pUserInfo != nullptr) 
	{
		pUserInfo->TicketCount++;
		TicketCount = pUserInfo->TicketCount;
	}
	else 
	{
		_RIMA_LOOTERY_USER_INFO* pList = new _RIMA_LOOTERY_USER_INFO;
		pList->GetID = GetID();
		pList->Name = strUserID;
		pList->TicketCount = 1;
		pList->isGift = false;
		TicketCount = pList->TicketCount;
		if (!g_pMain->m_RimaLotteryUserInfo.PutData(strUserID, pList)) { // strname üzerinden yapýcak
#if(__VERSION < 2369)
			pkt.clear();
			pkt.Initialize(WIZ_HSACS_HOOK);
			pkt << uint8(HSACSXOpCodes::LOTTERY) << uint8(3) << uint8(0) << std::string("You don't have enough money for the event.");
			Send(&pkt);
#endif
			return;
		}
	}

	g_pMain->pLotteryProc.UserJoinCounter++;
#if(__VERSION < 2369)
	pkt.clear();
	pkt.Initialize(WIZ_HSACS_HOOK);
	pkt << uint8(HSACSXOpCodes::LOTTERY) << uint8(3) << uint8(1) << TicketCount;
	Send(&pkt);

	pkt.clear();
	pkt.Initialize(WIZ_HSACS_HOOK);
	pkt << uint8(HSACSXOpCodes::LOTTERY) << uint8(2);
	g_pMain->Send_All(&pkt);
#endif
}

void CGameServerDlg::ReqLotteryReward(Packet&pkt) {

	std::string Name; uint32 nItemID, nItemCount;
	pkt >> Name >> nItemID >> nItemCount;

	_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable.isnull()) 
		return;
	
	if (!pTable.m_bCountable && nItemCount != 1) 
		nItemCount = 1;

	std::string SenderName = "LOTTERY", Subject = "REWARD", Message = "REWARD";

	_ITEM_DATA pItem{};
	pItem.nNum = nItemID;
	pItem.nSerialNum = g_pMain->GenerateItemSerial();
	pItem.sCount = nItemCount;
	pItem.sDuration = pTable.m_sDuration;
	pItem.nExpirationTime = 0;
	g_DBAgent.SendLetter(SenderName, Name, Subject, Message, 2, &pItem, 0);
}

void CGameServerDlg::LotteryEventLetterProcess(std::string Name, uint16 index)
{
	if (index > 3)
		return;

	auto* pInf = m_RimaLotteryUserInfo.GetData(Name);
	if (pInf == nullptr)
		return;

	if (pLotteryProc.nRewardItem[index] > 0) {
		_ITEM_TABLE pTable = g_pMain->GetItemPtr(pLotteryProc.nRewardItem[index]);
		if (pTable.isnull())
			return;

		Packet dbreq(WIZ_DB_SAVE, uint8(ProcDbServerType::LotteryReward));
		dbreq << pInf->Name << pLotteryProc.nRewardItem[index] << uint32(1);
		AddDatabaseRequest(dbreq);
	}
}

void CGameServerDlg::LotterySendGift()
{
	if (!pLotteryProc.LotteryStart 
		|| !pLotteryProc.SendGitfActivate)
		return;

	if (m_RimaLotteryUserInfo.IsEmpty())
		return;

	uint16 Index = 0;
	struct JoinList { uint16 nIndex; int16 UserGetID; std::string Name; };

	std::vector<JoinList> m_JoinList;

	m_RimaLotteryUserInfo.m_lock.lock();
	auto m_bRimaLotteryUserInfo = m_RimaLotteryUserInfo.m_UserTypeMap;
	m_RimaLotteryUserInfo.m_lock.unlock();

	foreach(itr, m_bRimaLotteryUserInfo) 
	{
		auto* pInfo = itr->second;
		if (pInfo == nullptr)
			continue;

		m_JoinList.push_back({ ++Index, pInfo->GetID, pInfo->Name });
	}

	if (m_JoinList.empty())
		return;

	int x = 4;
	int y = (int)m_JoinList.size() > x ? x : (int)m_JoinList.size();

	int SelectingTotal[10000];
	memset(&SelectingTotal, -1, sizeof(SelectingTotal));

	for (int i = 0; i < y; i++) 
	{
		if (Index < y && Index <= i)
			break;

		SelectingTotal[i] = myrand(1, Index);

		for (int kontrol = 0; kontrol < i; kontrol++) 
		{
			if (SelectingTotal[kontrol] == SelectingTotal[i]) 
			{
				i--;
				break;
			}
		}
	}

	uint16 Counter = 0;
	std::vector<std::string> mWinnerList;

	foreach(itr, m_JoinList) 
	{
		auto* pInf = m_RimaLotteryUserInfo.GetData(itr->Name);
		if (pInf == nullptr 
			|| pInf->isGift)
			continue;

		for (int i = 0; i < y; i++)
		{
			if (SelectingTotal[i] == itr->nIndex)
			{
				pInf->isGift = true;
				LotteryEventLetterProcess(pInf->Name, Counter);
				mWinnerList.push_back(itr->Name);
				Counter++;
			}
		}
	}

	Counter = 1;
	std::string Command = "------Lottery Event Winners------";
	SendChat<ChatType::PUBLIC_CHAT>(Command.c_str(), (uint8)Nation::ALL, false);
	if (!mWinnerList.empty()) 
	{	
		foreach(itr, mWinnerList) 
		{
			Command.clear();
			switch (Counter)
			{
			case 1: Command = string_format("%dst Winner Player : %s ", Counter, (*itr).c_str()); break;
			case 2: Command = string_format("%dnd Winner Player : %s ", Counter, (*itr).c_str()); break;
			case 3: Command = string_format("%drd Winner Player : %s ", Counter, (*itr).c_str()); break;
			case 4: Command = string_format("%dth Winner Player : %s ", Counter, (*itr).c_str()); break;
			}

			if(Command.size())
				SendChat<ChatType::PUBLIC_CHAT>(Command.c_str(), (uint8)Nation::ALL, false);
			Counter++;
		}
		Command = "---------------------------------";
		SendChat<ChatType::PUBLIC_CHAT>(Command.c_str(), (uint8)Nation::ALL, false);
	}
	else 
	{
		Command = "No Winner";
		SendChat<ChatType::PUBLIC_CHAT>(Command.c_str(), (uint8)Nation::ALL, false);
	}
}

void CGameServerDlg::LotteryEventTimer()
{
	if (!pLotteryProc.LotteryStart)
		return;

	if (pLotteryProc.EventTime != 0 && pLotteryProc.TimerControl)
	{
		uint32 RemainingTime = pLotteryProc.EventProcessTime - (uint32)UNIXTIME;
		if (RemainingTime > 0)
		{
			if (RemainingTime == 900)
				LogosYolla("[Lottery Event]", string_format("Remaining Minute %d", 15), 1, 240, 1);
			else if (RemainingTime == 600)
				LogosYolla("[Lottery Event]", string_format("Remaining Minute %d", 10), 1, 240, 1);
			else if (RemainingTime == 300)
				LogosYolla("[Lottery Event]", string_format("Remaining Minute %d", 5), 1, 240, 1);
			else if (RemainingTime == 180)
				LogosYolla("[Lottery Event]", string_format("Remaining Minute %d", 3), 1, 240, 1);
			else if (RemainingTime == 120)
				LogosYolla("[Lottery Event]", string_format("Remaining Minute %d", 2), 1, 240, 1);
			else if (RemainingTime == 60)
				LogosYolla("[Lottery Event]", string_format("Remaining Minute %d", 1), 1, 240, 1);
		}

		if (uint32(UNIXTIME) >= pLotteryProc.EventProcessTime)
		{
			pLotteryProc.TimerControl = false;
			pLotteryProc.SendGitfActivate = true;
			LotterySendGift();

			LotterySystemReset();
			m_RimaLotteryUserInfo.DeleteAllData();
		}
	}
}

void CGameServerDlg::LotterySystemReset()
{
	pLotteryProc.Initialize();
	pLotteryProc.LotteryStart = false;
	pLotteryProc.TimerControl = false;
	pLotteryProc.SendGitfActivate = false;
	pLotteryProc.EventTime = 0;
	pLotteryProc.UserJoinCounter = 0;

	memset(pLotteryProc.nReqItem, 0, sizeof(pLotteryProc.nReqItem));
	memset(pLotteryProc.nRewardItem, 0, sizeof(pLotteryProc.nRewardItem));

	pLotteryProc.UserLimit = 0;
	pLotteryProc.EventStartTime = 0;
	pLotteryProc.EventProcessTime = 0;
#if(__VERSION < 2369)
	Packet pkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::LOTTERY));
	pkt << uint8(4);
	Send_All(&pkt);
#endif
	std::string Notice = "Lottery Event has finished.";
	SendChat<ChatType::WAR_SYSTEM_CHAT>(Notice.c_str(), (uint8)Nation::ALL, true);
	SendChat<ChatType::PUBLIC_CHAT>(Notice.c_str(), (uint8)Nation::ALL, true);
}