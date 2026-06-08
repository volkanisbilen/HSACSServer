#include "stdafx.h"

/**
* @brief	Handles packets related to the mining system.
* 			Also handles soccer-related packets (yuck).
*
* @param	pkt	The packet.
*/
void CUser::HandleMiningSystem(Packet & pkt)
{
	uint8 opcode;
	pkt >> opcode;

	switch (opcode)
	{
	case MiningStart:
		HandleMiningStart(pkt);
		break;

	case MiningAttempt:
		HandleMiningAttempt(pkt);
		break;

	case MiningStop:
		HandleMiningStop();
		break;

	case BettingGame:
		HandleBettingGame(pkt);
		break;

	case FishingStart:
		HandleFishingStart(pkt);
		break;

	case FishingAttempt:
		HandleFishingAttempt(pkt);
		break;

	case FishingStop:
		HandleFishingStop(pkt);
		break;

	case MiningSoccer:
		HandleSoccer(pkt);
		break;

	default:
		TRACE("[SID=%d] Unknown packet %X\n", GetSocketID(), opcode);
		printf("[SID=%d] Unknown packet %X\n", GetSocketID(), opcode);
		return;
	}
}

/**
* @brief	Handles users Betting Game for 10 k coins.
* @param	pkt	The packet.
*/
void CUser::HandleBettingGame(Packet & pkt)
{
	if (isDead() || !isInGame())
		return;

	enum class ResultMessages
	{
		Won = 1, // %s won 10000
		EndinginaTie = 2, // Received 5000 Noah for ending in a tie
		Lose = 3, // %s lost Noah
		EnougCoins = 4, // %s You don't Enough coins.
	};

	Packet result(WIZ_MINING, uint8(BettingGame));
	ResultMessages resultsopcode = ResultMessages::EnougCoins;

	if (!hasCoins(5000))
	{
		resultsopcode = ResultMessages::EnougCoins;
		goto fail_return;
	}

	GoldLose(5000);

	uint8 PlayerRand = 0;  uint8 NpcRand = 0;
	PlayerRand = myrand(1, 5); NpcRand = myrand(1, 5);

	if (PlayerRand > NpcRand)
		resultsopcode = ResultMessages::Won;
	else if (PlayerRand < NpcRand)
		resultsopcode = ResultMessages::Lose;
	else if (PlayerRand == NpcRand)
		resultsopcode = ResultMessages::EndinginaTie;

	if (resultsopcode == ResultMessages::Won)
		GoldGain(10000);

fail_return:
	result << uint16(resultsopcode) << uint16(0) << uint8(0) << uint8(0) << uint8(PlayerRand) << uint8(NpcRand) << uint16(0);
	SendToRegion(&result);
}

bool CUser::MiningFishingAreaCheck(uint8 Type) {
	switch (Type)
	{
		//mining
	case 1:
	{
		C3DMap* pMap = GetMap();
		if (pMap == nullptr || !pMap->m_bMiningZone)
			return false;
		
		
		if (isInMoradon())
		{
			if (GetX() >= 600 && GetX() <= 666 && GetZ() >= 348 && GetZ() <= 399)
				return true;
		}
		else if (isInElmoradCastle())
		{
			if (GetX() >= 1408 && GetX() <= 1488 && GetZ() >= 354 && GetZ() <= 440)
				return true;

			if (GetX() >= 1653 && GetX() <= 1733 && GetZ() >= 526 && GetZ() <= 625)
				return true;
		}
		else if (isInLufersonCastle())
		{
			if (GetX() >= 597 && GetX() <= 720 && GetZ() >= 1625 && GetZ() <= 1705)
				return true;

			if (GetX() >= 315 && GetX() <= 415 && GetZ() >= 1435 && GetZ() <= 1500)
				return true;
		}
	}
	break;
	//fishing
	case 2:
	{
		if (isInElmoradCastle())
		{
			if (GetX() >= 850 && GetX() <= 935 && GetZ() >= 1080 && GetZ() <= 1115)
				return true;
		}
		else if (isInLufersonCastle())
		{
			if (GetX() >= 1106 && GetX() <= 1209 && GetZ() >= 915 && GetZ() <= 944)
				return true;
		}
	}
	break;
	}
	return false;
}

/**
* @brief	Handles users requesting to start mining.
* 			NOTE: This is a mock-up, so be warned that it does not
* 			handle checks such as identifying if the user is allowed
* 			to mine in this area.
*
* @param	pkt	The packet.
*/

void CUser::HandleMiningStart(Packet & pkt)
{
	Packet result(WIZ_MINING, uint8(MiningStart));
	uint16 resultCode = MiningResultSuccess;

	// Are we mining already?
	if (isMining())
		resultCode = MiningResultMiningAlready;

	if (!MiningFishingAreaCheck(1))
		resultCode = MiningResultNotMiningArea;

	// Do we have a pickaxe? Is it worn?
	_ITEM_DATA * pItem;
	_ITEM_TABLE pTable = GetItemPrototype(RIGHTHAND, pItem);
	if (pItem == nullptr || pTable.isnull()
		|| pItem->sDuration <= 0
		|| !pTable.isPickaxe())
		resultCode = MiningResultNotPickaxe;

	result << resultCode;

	// If nothing went wrong, allow the user to start mining.
	// Be sure to let everyone know we're mining.
	if (resultCode == MiningResultSuccess)
	{
		GetOfflineStatus(_choffstatus::ACTIVE, offcharactertype::minning);
		m_bResHpType = USER_MINING;
		m_bMining = true;
		result << uint32(GetID());
		SendToRegion(&result);
	}
	else
	{
		Send(&result);
	}
}


std::vector<_MINING_FISHING_ITEM> CUser::MiningItemList(_ITEM_TABLE pTable) {
	std::vector<_MINING_FISHING_ITEM> pMiningList;
	if (pTable.isnull()) 
		return pMiningList;

	g_pMain->m_MiningFishingItemArray.m_lock.lock();
	auto m_sMiningFishingItemArray = g_pMain->m_MiningFishingItemArray.m_UserTypeMap;
	g_pMain->m_MiningFishingItemArray.m_lock.unlock();

	foreach(itr, m_sMiningFishingItemArray)
	{
		auto& pMining = (*itr->second);
		if (pMining.isnull() 
			|| pMining.nTableType != 0) 
			continue;

		if (pTable.m_iNum == GOLDEN_MATTOCK && pMining.UseItemType == 1) 
		{
			if (g_pMain->m_byBattleOpen == NATION_BATTLE) 
			{
				if (GetZoneID() == ZONE_KARUS) 
				{
					if (GetWarVictory() == (uint8)Nation::ELMORAD && GetNation() == (uint8)Nation::ELMORAD && pMining.nWarStatus == 2) 
						pMiningList.push_back(pMining);
					else if (GetWarVictory() == (uint8)Nation::ELMORAD && GetNation() == (uint8)Nation::KARUS && pMining.nWarStatus == 1) 
						pMiningList.push_back(pMining);
					else if (pMining.nWarStatus == 0)
						pMiningList.push_back(pMining);
				}
				else if (GetZoneID() == ZONE_ELMORAD) 
				{
					if (GetWarVictory() == (uint8)Nation::KARUS && GetNation() == (uint8)Nation::KARUS && pMining.nWarStatus == 2) 
						pMiningList.push_back(pMining);
					else if (GetWarVictory() == (uint8)Nation::KARUS && GetNation() == (uint8)Nation::ELMORAD && pMining.nWarStatus == 1) 
						pMiningList.push_back(pMining);
					else if (pMining.nWarStatus == 0) 
						pMiningList.push_back(pMining);
				}
				else if (pMining.nWarStatus == 0) 
					pMiningList.push_back(pMining);
			}
			else if (pMining.nWarStatus == 0) 
				pMiningList.push_back(pMining);
		}
		else if (pTable.m_iNum == MATTOCK && pMining.UseItemType == 0) 
		{
			if (g_pMain->m_byBattleOpen == NATION_BATTLE) 
			{
				if (GetZoneID() == ZONE_KARUS) 
				{
					if (GetWarVictory() == (uint8)Nation::ELMORAD && GetNation() == (uint8)Nation::ELMORAD && pMining.nWarStatus == 2) 
						pMiningList.push_back(pMining);
					else if (GetWarVictory() == (uint8)Nation::ELMORAD && GetNation() == (uint8)Nation::KARUS && pMining.nWarStatus == 1) 
						pMiningList.push_back(pMining);
					else if (pMining.nWarStatus == 0) 
						pMiningList.push_back(pMining);
				}
				else if (GetZoneID() == ZONE_ELMORAD) 
				{
					if (GetWarVictory() == (uint8)Nation::KARUS && GetNation() == (uint8)Nation::KARUS && pMining.nWarStatus) 
						pMiningList.push_back(pMining);
					else if (GetWarVictory() == (uint8)Nation::KARUS && GetNation() == (uint8)Nation::ELMORAD && pMining.nWarStatus == 1) 
						pMiningList.push_back(pMining);
					else if (pMining.nWarStatus == 0) 
						pMiningList.push_back(pMining);
				}
				else if (pMining.nWarStatus == 0) 
					pMiningList.push_back(pMining);
			}
			else if (pMining.nWarStatus == 0) 
				pMiningList.push_back(pMining);
		}
	}
	return pMiningList;
}

uint32 CUser::GetMiningExpCount() {
	if (GetLevel() >= 1 && GetLevel() <= 34) return 50;
	else if (GetLevel() >= 35 && GetLevel() <= 59) return 100;
	else if (GetLevel() >= 60 && GetLevel() <= 69) return 200;
	else if (GetLevel() >= 70 && GetLevel() <= 83) return 300;
	return 0;
}

void CUser::HandleMiningAttempt(Packet & pkt)
{
	if (!isMining() 
		|| isFishing() 
		|| isTrading() 
		|| isMerchanting() 
		|| isSellingMerchantingPreparing())
		return;
	
	if (m_tLastMiningAttempt > UNIXTIME2)
		return;

	_ITEM_TABLE pGiveItem = _ITEM_TABLE();
	std::vector<_MINING_FISHING_ITEM> pMiningList;
	m_tLastMiningAttempt = UNIXTIME2 + MINING_DELAY;
	MiningErrors bResult = MiningErrors::MiningResultError;
	Packet result(WIZ_MINING, uint8(MiningAttempt));
	_ITEM_DATA* pItem;
	auto pTable = GetItemPrototype(RIGHTHAND, pItem);
	if (pItem == nullptr 
		|| pTable.isnull()
		|| (pTable.m_iNum != GOLDEN_MATTOCK && pTable.m_iNum != MATTOCK)
		|| pItem->sDuration <= 0 
		|| pItem->isDuplicate()
		|| pItem->isRented() 
		|| pItem->isSealed() 
		|| !pTable.isPickaxe()) {
		bResult = MiningErrors::MiningResultNotPickaxe;
		goto fail_return;
	}

	if (!isInMoradon()) {
		if (g_pMain->m_byBattleOpen == NATION_BATTLE && GetZoneID() != ZONE_KARUS && GetZoneID() != ZONE_ELMORAD) 
			goto fail_return;
		
		if (g_pMain->m_byBattleOpen != NATION_BATTLE && (GetNation() == (uint8)Nation::ELMORAD && !isInElmoradCastle()) || (GetNation() == (uint8)Nation::KARUS && !isInLufersonCastle())) 
			goto fail_return;
	}

	pMiningList = MiningItemList(pTable);
	if (pMiningList.empty()) 
		goto fail_return;

	uint32 bRandArray[10000], offset = 0; uint8 sItemSlot = 0;
	memset(bRandArray, 0, sizeof(bRandArray));
	foreach(itr, pMiningList) {
		if (itr->isnull()) 
			continue;
		if (offset >= 9999)
			break;
		
		for (int i = 0; i < int(itr->SuccessRate); i++) {
			if (i + offset >= 9999) 
				break;
			
			bRandArray[offset + i] = itr->nGiveItemID;
		}
		offset += int(itr->SuccessRate);
	}

	if (offset > 9999) offset = 9999;
	uint32 bRandSlot = myrand(0, offset);
	uint32 nItemID = bRandArray[bRandSlot];

	pGiveItem = g_pMain->GetItemPtr(nItemID);
	if (pGiveItem.isnull() || m_sItemWeight + pGiveItem.m_sWeight > m_sMaxWeight)
		return;

	int SlotForItem = FindSlotForItem(pGiveItem.m_iNum, 1);
	if (SlotForItem < 0)
		goto fail_return;

	uint16 sEffect = 0;
	if (nItemID == ITEM_EXP) {
		uint32 expcount = GetMiningExpCount();
		if (expcount) ExpChange("Mining",expcount);
		sEffect = 13082;
	}
	else {
		sItemSlot = GetEmptySlot() - SLOT_MAX;
		GiveItem("Mining Item", nItemID, 1);
		sEffect = 13081;
	}

	ItemWoreOut(ATTACK, 150);
	result << uint16(MiningErrors::MiningResultSuccess) << uint32(GetSocketID()) << sEffect;
	SendToRegion(&result, nullptr, GetEventRoom());

	return;
fail_return:
	result << uint16(MiningErrors::MiningResultError);
	Send(&result);
	HandleMiningStop();
}

/**
* @brief	Handles when a user stops mining.
*
* @param	pkt	The packet.
*/
void CUser::HandleMiningStop()
{
	if (!isMining())
		return;

	GetOfflineStatus(_choffstatus::DEACTIVE, offcharactertype::minning);
	Packet result(WIZ_MINING, uint8(MiningStop));
	result << uint16(1) << uint32(GetID());
	m_bMining = false;
	m_bResHpType = USER_STANDING;
	SendToRegion(&result);
	result.clear();
	result.SetOpcode(WIZ_MINING);
	result << uint8(MiningStop) << uint16(2);
	Send(&result);
}

/// Fish Stard
void CUser::HandleFishingStart(Packet & pkt)
{
	if (isBlinking()
		|| isTrading()
		|| isMerchanting()
		|| isDead()
		|| !isInGame()
		|| isBuyingMerchantingPreparing()
		|| isSellingMerchantingPreparing())
		return;

	Packet result(WIZ_MINING, uint8(FishingStart));
	uint16 resultCode = MiningResultSuccess;

	// Are we mining already?
	if (isFishing())
		resultCode = MiningResultMiningAlready;

	if (!MiningFishingAreaCheck(2)) {
		resultCode = MiningResultNotMiningArea;
	}

	// Do we have a pickaxe? Is it worn?
	_ITEM_DATA * pItem;
	_ITEM_TABLE pTable = GetItemPrototype(RIGHTHAND, pItem);
	if (pItem == nullptr || pTable.isnull()
		|| pItem->sDuration <= 0
		|| !pTable.isFishing())
		resultCode = MiningResultNotPickaxe;
	
	auto item = GetItem(RIGHTHAND);
	if (!item) return;

	if (item->nNum != GOLDEN_FISHING) //[] 06.05.2020 Golden Fishing Rod Disindakilerde RainWorm kontrolu yapacak.
	{
		if (GetItemCount(RAINWORM) <= 0)
			resultCode = MiningResultNoEarthWorm;
	}

	result << resultCode;

	// If nothing went wrong, allow the user to start mining.
	// Be sure to let everyone know we're mining.
	if (resultCode == MiningResultSuccess)
	{
		GetOfflineStatus(_choffstatus::ACTIVE, offcharactertype::fishing);
		m_bResHpType = USER_FLASHING;
		m_bFishing = true;
		result << uint32(GetID());
		SendToRegion(&result);
	}
	else
	{
		Send(&result);
	}
}

std::vector<_MINING_FISHING_ITEM> CUser::FishingItemList(_ITEM_TABLE pTable) {
	std::vector<_MINING_FISHING_ITEM> pFishingList;
	if (pTable.isnull())
		return pFishingList;

	g_pMain->m_MiningFishingItemArray.m_lock.lock();
	auto m_sMiningFishingItemArray = g_pMain->m_MiningFishingItemArray.m_UserTypeMap;
	g_pMain->m_MiningFishingItemArray.m_lock.unlock();

	foreach(itr, m_sMiningFishingItemArray)
	{
		auto& pMining = (*itr->second);
		if (pMining.isnull()
			|| pMining.nTableType != 1)
			continue;

		if (pTable.m_iNum == GOLDEN_FISHING && pMining.UseItemType == 1)
		{
			if (g_pMain->m_byBattleOpen == NATION_BATTLE)
			{
				if (GetZoneID() == ZONE_KARUS)
				{
					if (GetWarVictory() == (uint8)Nation::ELMORAD && GetNation() == (uint8)Nation::ELMORAD && pMining.nWarStatus == 2)
						pFishingList.push_back(pMining);
					else if (GetWarVictory() == (uint8)Nation::ELMORAD && GetNation() == (uint8)Nation::KARUS && pMining.nWarStatus == 1)
						pFishingList.push_back(pMining);
					else if (pMining.nWarStatus == 0)
						pFishingList.push_back(pMining);
				}
				else if (GetZoneID() == ZONE_ELMORAD)
				{
					if (GetWarVictory() == (uint8)Nation::KARUS && GetNation() == (uint8)Nation::KARUS && pMining.nWarStatus == 2)
						pFishingList.push_back(pMining);
					else if (GetWarVictory() == (uint8)Nation::KARUS && GetNation() == (uint8)Nation::ELMORAD && pMining.nWarStatus == 1)
						pFishingList.push_back(pMining);
					else if (pMining.nWarStatus == 0)
						pFishingList.push_back(pMining);
				}
				else if (pMining.nWarStatus == 0)
					pFishingList.push_back(pMining);
			}
			else if (pMining.nWarStatus == 0)
				pFishingList.push_back(pMining);
		}
		else if (pTable.m_iNum == FISHING && pMining.UseItemType == 0)
		{
			if (g_pMain->m_byBattleOpen == NATION_BATTLE)
			{
				if (GetZoneID() == ZONE_KARUS)
				{
					if (GetWarVictory() == (uint8)Nation::ELMORAD && GetNation() == (uint8)Nation::ELMORAD && pMining.nWarStatus == 2)
						pFishingList.push_back(pMining);
					else if (GetWarVictory() == (uint8)Nation::ELMORAD && GetNation() == (uint8)Nation::KARUS && pMining.nWarStatus == 1)
						pFishingList.push_back(pMining);
					else if (pMining.nWarStatus == 0)
						pFishingList.push_back(pMining);
				}
				else if (GetZoneID() == ZONE_ELMORAD)
				{
					if (GetWarVictory() == (uint8)Nation::KARUS && GetNation() == (uint8)Nation::KARUS && pMining.nWarStatus)
						pFishingList.push_back(pMining);
					else if (GetWarVictory() == (uint8)Nation::KARUS && GetNation() == (uint8)Nation::ELMORAD && pMining.nWarStatus == 1)
						pFishingList.push_back(pMining);
					else if (pMining.nWarStatus == 0)
						pFishingList.push_back(pMining);
				}
				else if (pMining.nWarStatus == 0)
					pFishingList.push_back(pMining);
			}
			else if (pMining.nWarStatus == 0)
				pFishingList.push_back(pMining);
		}
	}
	return pFishingList;
}

/**
* @brief	Handles a user's mining attempt by finding a random reward (or none at all).
* 			This is sent automatically by the client every MINING_DELAY (5) seconds.
*
* @param	pkt	The packet.
*/
void CUser::HandleFishingAttempt(Packet & pkt)
{
	if (isMining()
		|| !isFishing()
		|| isTrading()
		|| isMerchanting()
		|| isSellingMerchantingPreparing())
		return;

	if (m_tLastMiningAttempt > UNIXTIME2)
		return;

	_ITEM_TABLE pGiveItem = _ITEM_TABLE();
	m_tLastMiningAttempt = UNIXTIME2 + MINING_DELAY;
	Packet result(WIZ_MINING, uint8(FishingAttempt));
	uint16 resultCode = MiningResultSuccess;
	// Do we have a pickaxe? Is it worn?
	_ITEM_DATA * pItem;
	_MINING_FISHING_ITEM * pFishing = nullptr;
	std::vector<_MINING_FISHING_ITEM> pFishingList;
	_ITEM_TABLE pTable = GetItemPrototype(RIGHTHAND, pItem);
	if (pItem == nullptr 
		|| pTable.isnull()
		|| pItem->sDuration <= 0 // are we supposed to wear the pickaxe on use? Need to verify.
		|| !pTable.isFishing())
		resultCode = MiningResultNotPickaxe;

	if (pItem->GetNum() != GOLDEN_FISHING)
	{
		if (GetItemCount(RAINWORM) <= 0)
			resultCode = MiningResultNoEarthWorm;
	}

	uint16 sEffect = 0; // Effect to show to clients

	// This is just a mock-up based on another codebase's implementation.
	// Need to log official data to get a proper idea of how it behaves, rate-wise,
	// so that we can then implement it more dynamically.
	if (!isInMoradon()) 
	{
		if (g_pMain->m_byBattleOpen == NATION_BATTLE
			&& GetZoneID() != ZONE_KARUS 
			&& GetZoneID() != ZONE_ELMORAD)
			return;

		if (g_pMain->m_byBattleOpen != NATION_BATTLE
			&& (GetNation() == (uint8)Nation::ELMORAD && !isInElmoradCastle()) 
			|| (GetNation() == (uint8)Nation::KARUS && !isInLufersonCastle()))
			return;
	}

	pFishingList = FishingItemList(pTable);
	if (pFishingList.empty())
		return;

	if (resultCode == MiningResultSuccess)
	{
		uint32 bRandArray[10000], offset = 0; uint8 sItemSlot = 0;
		memset(bRandArray, 0, sizeof(bRandArray));
		foreach(itr, pFishingList) {
			if (itr->isnull())
				continue;
			if (offset >= 9999)
				break;

			for (int i = 0; i < int(itr->SuccessRate); i++) {
				if (i + offset >= 9999)
					break;

				bRandArray[offset + i] = itr->nGiveItemID;
			}
			offset += int(itr->SuccessRate);
		}

		if (offset > 9999) offset = 9999;
		uint32 bRandSlot = myrand(0, offset);
		uint32 nItemID = bRandArray[bRandSlot];

		pGiveItem = g_pMain->GetItemPtr(nItemID);
		if (pGiveItem.isnull()
			|| m_sItemWeight + pGiveItem.m_sWeight > m_sMaxWeight)
			return;

		int SlotForItem = FindSlotForItem(pGiveItem.m_iNum, 1);
		if (SlotForItem < 0)
			return;

		uint16 sEffect = 0;
		if (nItemID == ITEM_EXP) {
			uint32 expcount = GetMiningExpCount();
			if (expcount) ExpChange("Fishing", expcount);
			sEffect = 13082;
		}
		else {
			sItemSlot = GetEmptySlot() - SLOT_MAX;
			GiveItem("Fishing Item", nItemID, 1);
			sEffect = 30730;
		}
	}
	
	result << resultCode << uint32(GetID()) << sEffect;

	if (GetItem(RIGHTHAND)->nNum != GOLDEN_FISHING)
		RobItem(RAINWORM, 1);

	ItemWoreOut(ATTACK, 100);

	if (resultCode != MiningResultSuccess
		&& resultCode != MiningResultNothingFound)
	{
		// Tell us the error first
		Send(&result);
		// and then tell the client to stop mining
		HandleFishingStop(pkt);
		return;
	}

	if (resultCode != MiningResultNothingFound)
		SendToRegion(&result);
	else if (resultCode == MiningResultNothingFound)
		Send(&result);
}

/**
* @brief	Handles when a user stops mining.
*
* @param	pkt	The packet.
*/
void CUser::HandleFishingStop(Packet & pkt)
{
	if (!isFishing())
		return;

	GetOfflineStatus(_choffstatus::DEACTIVE, offcharactertype::fishing);
	Packet result(WIZ_MINING, uint8(FishingStop));
	result << uint16(1) << uint32(GetID());
	m_bResHpType = USER_STANDING;
	m_bFishing = false;
	SendToRegion(&result);
	result.clear();
	result.SetOpcode(WIZ_MINING);
	result << uint8(FishingStop) << uint8(2);
	Send(&result);
}

