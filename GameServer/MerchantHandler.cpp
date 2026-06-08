#include "stdafx.h"
#include "../shared/DateTime.h"
using std::string;
enum MerchantOpenResponseCodes
{
	MERCHANT_OPEN_SUCCESS = 1,
	MERCHANT_OPEN_NO_SESSION = -1,
	MERCHANT_OPEN_DEAD = -2,
	MERCHANT_OPEN_TRADING = -3,
	MERCHANT_OPEN_MERCHANTING = -4,
	MERCHANT_OPEN_INVALID_ZONE = -5,
	MERCHANT_OPEN_SHOPPING = -6,
	MERCHANT_OPEN_UNDERLEVELED = 30
};

#define ITEM_MENICIAS_LIST 810166000

void CUser::MerchantProcess(Packet& pkt)
{
	if (!isInGame())
		return;

	if (pCindWar.isEventUser() && g_pMain->isCindirellaZone(GetZoneID()))
		return;

	uint8 opcode = pkt.read<uint8>();
	switch (opcode)
	{
		// Regular merchants
	case MERCHANT_OPEN:
		MerchantOpen();
		break;

	case MERCHANT_CLOSE:
		MerchantClose();
		break;

	case MERCHANT_ITEM_ADD:
#if (!HOOKACTIVE)
		MerchantItemAdd(pkt);
#endif
		break;

	case MERCHANT_ITEM_CANCEL:
		MerchantItemCancel(pkt);
		break;

	case MERCHANT_ITEM_LIST_NEW:
	case MERCHANT_ITEM_LIST:  // 6805F9FF
		MerchantItemList(pkt);
		break;

	case MERCHANT_ITEM_BUY:
		MerchantItemBuy(pkt);
		break;

	case MERCHANT_INSERT:
		MerchantInsert(pkt);
		break;

	case MERCHANT_TRADE_CANCEL:
		CancelMerchant();
		break;

		// Buying merchants
	case MERCHANT_BUY_OPEN:
		BuyingMerchantOpen(pkt);
		break;

	case MERCHANT_BUY_CLOSE:
		BuyingMerchantClose();
		break;

	case MERCHANT_BUY_LIST_NEW:
	case MERCHANT_BUY_LIST:
		BuyingMerchantList(pkt);
		break;

	case MERCHANT_BUY_INSERT:
		BuyingMerchantInsert(pkt);
		break;

	case MERCHANT_BUY_BUY: // seeya!
		BuyingMerchantBuy(pkt);
		break;

	case MERCHANT_MENISIA_LIST:
		MerchantOfficialList(pkt);
		break;

	case MERCHANT_LIST:
		MerchantList(pkt);
		break;
	default:
		printf("Merchant unhandled packets %d \n", opcode);
		TRACE("Merchant unhandled packets %d \n", opcode);
		break;
	}
}

void CUser::MerchantOpen()
{
	if (isBuyingMerchantingPreparing())
		return;

	int16 errorCode = 0;
	if (isDead())
		errorCode = MERCHANT_OPEN_DEAD;
	else if (isStoreOpen())
		errorCode = MERCHANT_OPEN_SHOPPING;
	else if (isTrading())
		errorCode = MERCHANT_OPEN_TRADING;
	/*else if (GetZoneID() > ZONE_MORADON)
		errorCode = MERCHANT_OPEN_INVALID_ZONE;*/
	else if (GetLevel() < g_pMain->pServerSetting.MerchantLevel)
		errorCode = MERCHANT_OPEN_UNDERLEVELED;
	else if (isMerchanting())
		errorCode = MERCHANT_OPEN_MERCHANTING;
	else
		errorCode = MERCHANT_OPEN_SUCCESS;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_OPEN));
	result << errorCode;
	Send(&result);

	// If we're already merchanting, user may be desynced
	// so we need to close our current merchant first.
	if (errorCode == MERCHANT_OPEN_MERCHANTING)
		MerchantClose();

	if (errorCode == MERCHANT_OPEN_SUCCESS)
		m_bSellingMerchantPreparing = true;
}

// kanka enes bot yazdý orda gitmiþ olabilir mi? bilmiyorum þuanda merchant sistemi en son benim yaptýðým deðil
void CUser::MerchantClose()
{
	bool m_sMerchantStatus = isSellingMerchantingPreparing() || isSellingMerchant();
	if (!m_sMerchantStatus)
		return;

	bool m_sMerchantPreparing = m_bSellingMerchantPreparing;

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		auto* pItem = GetItem(i);
		if (pItem && pItem->MerchItem) pItem->MerchItem = false;
	}

	m_sLevelMerchantTime = 0;
	m_sPlookerSocketID = -1;
	m_bSellingMerchantPreparing = false;
	m_bMerchantState = MERCHANT_STATE_NONE;
	memset(m_arMerchantItems, 0, sizeof(m_arMerchantItems));
#if(GAME_SOURCE_VERSION == 2369)
	HandleHShieldSoftwareNewItemMoveLoadderHandler();
#endif
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_CLOSE));
	result << uint32(GetSocketID());

	if (!m_sMerchantPreparing)
	{
		SendToRegion(&result, nullptr, GetEventRoom());
		GetOfflineStatus(_choffstatus::DEACTIVE, offcharactertype::merchant);
	}
	else Send(&result);
}

void CUser::MerchantItemAdd(Packet& pkt)
{
	if (!isSellingMerchantingPreparing() || isBuyingMerchantingPreparing())
		return;

	_ITEM_TABLE pTable = _ITEM_TABLE();
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_ADD));
	uint32 nGold, nItemID;
	uint16 sCount;
	uint8 bSrcPos, bDstPos, bMode, isKC;

	pkt >> nItemID >> sCount >> nGold >> bSrcPos >> bDstPos >> bMode >> isKC;
	// TODO: Implement the possible error codes for these various error cases.
	if (!nItemID || !sCount || bSrcPos >= HAVE_MAX || bDstPos >= MAX_MERCH_ITEMS) goto fail_return;

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
		if (m_arMerchantItems[i].nNum && m_arMerchantItems[i].bOriginalSlot == bSrcPos)
			goto fail_return;

	_ITEM_DATA* pSrcItem = GetItem(bSrcPos + SLOT_MAX);
	pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable.isnull() || !pSrcItem)
		goto fail_return;

	if (nItemID >= ITEM_NO_TRADE_MIN && nItemID <= ITEM_NO_TRADE_MAX // Cannot be traded, sold or stored.
		|| pTable.m_bRace == RACE_UNTRADEABLE // Cannot be traded or sold.
		|| pTable.m_bCountable == 2
		|| (pTable.m_bKind == 255 && sCount != 1)) // Cannot be traded or sold.
		goto fail_return;

	if (pTable.m_bCountable == 1 && sCount > MAX_ITEM_COUNT) goto fail_return;

	if (pSrcItem->nNum != nItemID
		|| pSrcItem->sCount == 0
		|| sCount == 0
		|| pSrcItem->sCount < sCount
		|| pSrcItem->isRented()
		|| pSrcItem->isSealed()
		|| pSrcItem->isBound()
		|| pSrcItem->isDuplicate()
		|| pSrcItem->isExpirationTime())
		goto fail_return;

	_MERCH_DATA* pMerch = &m_arMerchantItems[bDstPos];
	if (pMerch == nullptr || pMerch->nNum != 0)
		goto fail_return;

	pSrcItem->MerchItem = true;

	pMerch->nNum = nItemID;
	pMerch->nPrice = nGold;
	pMerch->sCount = sCount; // Selling Count
	pMerch->bCount = pSrcItem->sCount;

	pMerch->sDuration = pSrcItem->sDuration;
	pMerch->nSerialNum = pSrcItem->nSerialNum; // NOTE: Stackable items will have an issue with this.

#if (!HOOKACTIVE)
	pMerch->bOriginalSlot = bSrcPos + SLOT_MAX;
#else
	pMerch->bOriginalSlot = bSrcPos;
#endif
	
	pMerch->IsSoldOut = false;
	pMerch->isKC = isKC;

	if (pTable.m_bKind == 255 && !pTable.m_bCountable) pMerch->sCount = 1;

	result << uint16(1) << nItemID << sCount << pMerch->sDuration << nGold << bSrcPos << bDstPos << isKC;
	Send(&result);
	return;

fail_return:
	result << uint16(0) << nItemID << sCount << (uint16)bSrcPos + bDstPos << nGold << bSrcPos << bDstPos << isKC;
	Send(&result);
}

void CUser::MerchantItemCancel(Packet& pkt)
{
	if (!isSellingMerchantingPreparing() || isBuyingMerchantingPreparing())
		return;

	uint8 bSrcPos = pkt.read<uint8>();

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_CANCEL));

	if (bSrcPos >= MAX_MERCH_ITEMS)
	{
		result << int16(-2);
		Send(&result);
		return;
	}

	_MERCH_DATA* pMerch = &m_arMerchantItems[bSrcPos];
	if (pMerch == nullptr || !pMerch->nNum
		|| !pMerch->sCount || pMerch->sCount > ITEMCOUNT_MAX
		|| pMerch->bOriginalSlot >= HAVE_MAX)
	{
		result << int16(-3);
		Send(&result);
		return;
	}

	_ITEM_DATA* pItem = GetItem(pMerch->bOriginalSlot);
	if (pItem == nullptr || pItem->nNum != pMerch->nNum
		|| pItem->sCount != pMerch->bCount
		|| !pItem->isMerchantItem())
	{
		result << int16(-3);
		Send(&result);
		return;
	}

	pItem->MerchItem = false;
	memset(pMerch, 0, sizeof(_MERCH_DATA));
	result << int16(1) << bSrcPos;
	Send(&result);
}

void CUser::MerchantInsert(Packet& pkt)
{
	if (isGM())
		MerchantGameMasterInsert(pkt);
	else
		MerchantUserInsert(pkt);
}

void CUser::MerchantUserInsert(Packet& pkt)
{
	if (!isSellingMerchantingPreparing()
		|| isBuyingMerchantingPreparing())
		return;

	uint16 bResult = 0;
	bResult = 1;
	_MERCH_DATA	m_arNewItems[MAX_MERCH_ITEMS]{}; //What is this person selling? Stored in "_MERCH_DATA" structure.

	DateTime time;
	string advertMessage; // check here maybe to make sure they're not using it otherwise?
	pkt >> advertMessage;
	if (advertMessage.size() > MAX_MERCH_MESSAGE)
		return;

	Packet result;
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_INSERT) << uint16(0x01) << advertMessage << uint32(GetSocketID()) << m_bPremiumMerchant;

	uint16 m_sTotalMerchant = 0;

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		result << m_arMerchantItems[i].nNum;
		if (m_arMerchantItems[i].nNum > 0)  
			m_sTotalMerchant++;
	}

	if (!m_sTotalMerchant || m_sTotalMerchant > 12)
		return goDisconnect(string_format("There is a problem in the number of parts put on the market. itemcount=%d", m_sTotalMerchant), __FUNCTION__);

	if (g_pMain->pLevMercInfo.status)
		m_sLevelMerchantTime = UNIXTIME + (g_pMain->pLevMercInfo.rewardtime * MINUTE);

	m_bMerchantState = MERCHANT_STATE_SELLING;
	m_bSellingMerchantPreparing = false;
	SendToRegion(&result, nullptr, GetEventRoom());
	GetOfflineStatus(_choffstatus::ACTIVE, offcharactertype::merchant);
	MerchantCreationInsertLog(1);
	MerchantSlipRefList(this, true);
}

void CUser::MerchantGameMasterInsert(Packet& pkt)
{
	if (!isSellingMerchantingPreparing()
		|| isBuyingMerchantingPreparing())
		return;

	uint16 bResult = 0;
	bResult = 1;
	_MERCH_DATA	m_arNewItems[MAX_MERCH_ITEMS]{}; //What is this person selling? Stored in "_MERCH_DATA" structure.

	DateTime time;
	string advertMessage; // check here maybe to make sure they're not using it otherwise?
	pkt >> advertMessage;
	if (advertMessage.size() > MAX_MERCH_MESSAGE)
		return;

	Packet result;
	result.Initialize(WIZ_MERCHANT);

	_MERCH_DATA pBotMerchant[MAX_MERCH_ITEMS];
	memcpy(pBotMerchant, &m_arMerchantItems, sizeof(m_arMerchantItems));

	m_sPlookerSocketID = -1;
	m_bSellingMerchantPreparing = false;
	m_bMerchantState = MERCHANT_STATE_NONE;
	memset(m_arMerchantItems, 0, sizeof(m_arMerchantItems));

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		auto* pItem = GetItem(i);
		if (pItem && pItem->MerchItem)
			pItem->MerchItem = false;
	}

	uint8 nRandom = 2;
	float fX = GetX();
	float fZ = GetZ();

	fX = fX + myrand(0, nRandom);
	fZ = fZ + myrand(0, nRandom);

	CBot* pBorMerchant;
	uint16 m_bSocketID = g_pMain->SpawnEventBotMerchant(3600, GetZoneID(), fX, GetY(), fZ, NULL, MIN_LEVEL_ARDREAM);
	if (m_bSocketID)
	{
		pBorMerchant = g_pMain->GetBotPtr(m_bSocketID);
		if (pBorMerchant == nullptr)
			return;

		if (pBorMerchant->GetRegion() == nullptr)
			return;

		std::string asdasd = advertMessage;

		if (!asdasd.empty())
			pBorMerchant->MerchantChat = string_format("%s(Location:%d,%d)", asdasd.c_str(), pBorMerchant->GetSPosX() / 10, pBorMerchant->GetSPosZ() / 10);
		else
			pBorMerchant->MerchantChat.clear();

		pBorMerchant->m_iLoyalty = myrand(100, 5000);
		memset(pBorMerchant->m_arMerchantItems, 0, sizeof(pBorMerchant->m_arMerchantItems));
		result << uint8(MERCHANT_INSERT) << bResult << advertMessage << uint32(pBorMerchant->GetID()) << m_bPremiumMerchant;

		uint8 nreqcount = 0;
		for (int i = 0; i < MAX_MERCH_ITEMS; i++)
		{
			pBorMerchant->m_arMerchantItems[i] = pBotMerchant[i];

			if (pBorMerchant->m_arMerchantItems[i].nNum != 0 &&
				(pBorMerchant->m_arMerchantItems[i].sCount == 0
					|| pBorMerchant->m_arMerchantItems[i].sCount < pBotMerchant[i].sCount))
				return;

			result << pBorMerchant->m_arMerchantItems[i].nNum;

			if (pBorMerchant->m_arMerchantItems[i].nNum > 0)
				nreqcount++;
		}

		if (!nreqcount)
			return;

		pBorMerchant->m_bPremiumMerchant = m_bPremiumMerchant;
		pBorMerchant->m_bMerchantState = MERCHANT_STATE_SELLING;
		pBorMerchant->m_bSellingMerchantPreparing = false;
	}

	if (pBorMerchant && m_bSocketID)
		pBorMerchant->SendToRegion(&result);

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_CLOSE) << uint32(GetSocketID());
	Send(&result);
}

void CUser::MerchantItemList(Packet& pkt)
{
	uint32 m_dSocketID = pkt.read<uint32>();
	int16 test = (int16)m_dSocketID;
	if (test < NPC_BAND)
	{
		if (test < MAX_USER)
			MerchantItemUserList(pkt, test);
		else
			MerchantItemBotList(pkt, test);
	}
}

void CUser::MerchantItemBotList(Packet& pkt, uint16 m_dSocketID)
{
	CBot* pUserBot = g_pMain->GetBotPtr(m_dSocketID);
	if (pUserBot == nullptr)
		return;

	if (!pUserBot->isInGame()
		|| !pUserBot->isMerchanting())
		return;

	Packet result;
	if (pUserBot->m_bMerchantViewer >= 0)
	{
		auto* pLooker = g_pMain->GetUserPtr(pUserBot->m_bMerchantViewer);
		if (pLooker && pLooker->isInGame())
		{
			result.Initialize(WIZ_MERCHANT);
			result << uint8(MERCHANT_ITEM_LIST);
			result.SByte();
			result << int16(-7) << pLooker->GetName();
			Send(&result);
			return;
		}
		else pUserBot->m_bMerchantViewer = -1;
	}

	pUserBot->m_bMerchantViewer = GetID();
	m_sMerchantsSocketID = m_dSocketID;

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_ITEM_LIST) << uint16(0x01) << uint32(m_dSocketID);

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		_MERCH_DATA* pMerch = &pUserBot->m_arMerchantItems[i];
		if (pMerch == nullptr)
			continue;

		result << pMerch->nNum << pMerch->sCount << pMerch->sDuration << pMerch->nPrice;

		if (pMerch->nNum == ITEM_CYPHER_RING)
			ShowCyperRingItemInfo(result, pMerch->nSerialNum);
		else
			result << uint32(0); // Item Unique ID
	}

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		_MERCH_DATA* pMerch = &pUserBot->m_arMerchantItems[i];
		uint8 isKC = pMerch->isKC ? 1 : 0;
		result << isKC;
	}

	Send(&result);
}

void CUser::MerchantItemUserList(Packet& pkt, uint16 m_dSocketID)
{
	CUser* pUserMerchant = g_pMain->GetUserPtr(m_dSocketID);
	if (pUserMerchant == nullptr)
		return;

	if (!isInRange(pUserMerchant, MAX_NPC_RANGE)
		|| !pUserMerchant->isMerchanting()
		|| pUserMerchant->isSellingMerchantingPreparing()
		|| pUserMerchant->isBuyingMerchantingPreparing())
		return;

	Packet result;
	if (pUserMerchant->m_sPlookerSocketID >= 0)
	{
		CUser* pLooker = g_pMain->GetUserPtr(pUserMerchant->m_sPlookerSocketID);
		if (pLooker && pLooker->isInGame())
		{
			result.Initialize(WIZ_MERCHANT);
			result << uint8(MERCHANT_ITEM_LIST);
			result.SByte();
			result << int16(-7) << pLooker->GetName();
			Send(&result);
			return;
		}
		else pUserMerchant->m_sPlookerSocketID = -1;
	}

	pUserMerchant->m_sPlookerSocketID = GetID();
	m_sMerchantsSocketID = m_dSocketID;

	if (g_pMain->pServerSetting.MerchantView)
	{
		if (pUserMerchant != nullptr)
		{
#if(__VERSION < 2369)
			DateTime timemachine;
			result.clear();
			result.Initialize(WIZ_HSACS_HOOK);
			result << uint8(HSACSXOpCodes::MERC_WIEWER_INFO) << uint8(1);
			result.SByte();
			result << GetName() << timemachine.GetHour() << timemachine.GetMinute();
			pUserMerchant->Send(&result);
#endif
		}
	}

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_ITEM_LIST);
	result << uint16(1) << uint32(m_dSocketID);

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		_MERCH_DATA* pMerch = &pUserMerchant->m_arMerchantItems[i];
		if (pMerch == nullptr)
			continue;

		result << pMerch->nNum
			<< pMerch->sCount
			<< pMerch->sDuration
			<< pMerch->nPrice;

		_ITEM_TABLE pItemTable = g_pMain->GetItemPtr(pMerch->nNum);
		if (!pItemTable.isnull())
		{
			if (pItemTable.isPetItem())
				ShowPetItemInfo(result, pMerch->nSerialNum);
			else if (pItemTable.GetNum() == ITEM_CYPHER_RING)
				ShowCyperRingItemInfo(result, pMerch->nSerialNum);
			else
				result << uint32(0); // Item Unique ID
		}
		else
			result << uint32(0); // Item Unique ID*/
	}

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		_MERCH_DATA* pMerch = &pUserMerchant->m_arMerchantItems[i];
		if (!pMerch) 
			continue;

		uint8 isKC = pMerch->isKC ? 1 : 0;
		result << isKC;
	}

	Send(&result);
}

void CUser::MerchantItemBuy(Packet& pkt)
{
	if (m_sMerchantsSocketID < NPC_BAND)
	{
		if (m_sMerchantsSocketID < MAX_USER)
			MerchantItemUserBuy(pkt);
		else
			MerchantItemBotBuy(pkt);
	}
}

void CUser::MerchantItemBotBuy(Packet& pkt)
{
	uint32 itemid, req_gold;
	uint16 item_count, leftover_count;
	uint8 item_slot, dest_slot;
	Packet result(WIZ_MERCHANT);
	CBot* pUserBot = g_pMain->GetBotPtr(m_sMerchantsSocketID);
	if (pUserBot == nullptr)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	if (pUserBot->GetMerchantState() != MERCHANT_STATE_SELLING)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	pkt >> itemid >> item_count >> item_slot >> dest_slot;

	// Make sure the slots are correct and that we're actually buying at least 1 item.
	if (item_slot >= MAX_MERCH_ITEMS 
		|| dest_slot >= HAVE_MAX || !item_count)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	// Grab pointers to the items.
	_MERCH_DATA* pMerch = &pUserBot->m_arMerchantItems[item_slot];
	if (pMerch == nullptr 
		|| pMerch->IsSoldOut 
		|| !pMerch->sCount 
		|| !pMerch->nPrice)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	_ITEM_DATA* pBotBuy = GetItem(SLOT_MAX + dest_slot);
	if (pBotBuy == nullptr)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	if (pMerch->nNum != itemid 
		|| pMerch->sCount < item_count
		|| pMerch->bCount < item_count)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	auto pSellingItem = g_pMain->GetItemPtr(itemid);
	if (pSellingItem.isnull() 
		|| pSellingItem.m_bCountable == 2 
		|| (!pSellingItem.m_bCountable && item_count != 1))
	{
		result << uint16(-18);
		Send(&result);
		return;
	}

	if (pSellingItem.m_bKind == 255 
		&& item_count != 1 && !pSellingItem.m_bCountable)
	{
		result << uint16(-18);
		Send(&result);
		return;
	}

	uint32 nReqWeight = pSellingItem.m_sWeight * item_count;
	if (nReqWeight + m_sItemWeight > m_sMaxWeight)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	req_gold = pMerch->nPrice * item_count;

	if (pMerch->isKC)
	{
		if (m_nKnightCash < req_gold)
		{
			result << uint8(6) << uint16(-18);
			Send(&result);
			return;
		}
	}
	else
	{
		if (m_iGold < req_gold)
		{
			result << uint8(6) << uint16(-18);
			Send(&result);
			return;
		}
	}

	if (pBotBuy->nNum != 0 
		&& (pBotBuy->nNum != itemid 
			|| !pSellingItem.m_bCountable))
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	/*if (!pMerch->isKC 
		&& pUserBot->m_iGold + req_gold > COIN_MAX)
		return;*/

	if (pMerch->isKC)
	{
		if (!CashLose(req_gold))
		{
			result << uint8(6) << uint16(-18);
			Send(&result);
			return;
		}
	}
	else
	{
		if (!GoldLose(req_gold))
		{
			result << uint8(6) << uint16(-18);
			Send(&result);
			return;
		}

		if (pUserBot->m_iGold + req_gold > COIN_MAX)
			pUserBot->m_iGold = COIN_MAX;
		else
			pUserBot->m_iGold += req_gold;
	}

	leftover_count = pMerch->sCount - item_count;
	pBotBuy->sCount += item_count;
	pMerch->sCount -= item_count;
	pBotBuy->nSerialNum = g_pMain->GenerateItemSerial();
	pBotBuy->nNum = pMerch->nNum;
	pBotBuy->sDuration = pMerch->sDuration;

	if (!pSellingItem.isStackable() 
		|| item_count == pMerch->sCount) 
		pBotBuy->nSerialNum = g_pMain->GenerateItemSerial();

	if (!pSellingItem.isStackable()
		&& pBotBuy->nSerialNum == 0) 
		pBotBuy->nSerialNum = g_pMain->GenerateItemSerial();

	pBotBuy->MerchItem = false;

	SendStackChange(itemid, pBotBuy->sCount, pBotBuy->sDuration, dest_slot, (pBotBuy->sCount == item_count)); // is it a new item?
	MerchantShoppingDetailInsertLog(true, 1, itemid, item_count, pMerch->nPrice, nullptr);
#if(GAME_SOURCE_VERSION == 2369)
	HandleHShieldSoftwareNewItemMoveLoadderHandler();
#endif
	if (!pMerch->sCount 
		|| (pSellingItem.m_bCountable == 0 && pSellingItem.m_bKind == 255))
	{
		memset(pMerch, 0, sizeof(_MERCH_DATA));
		pMerch->IsSoldOut = true;
	}

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_ITEM_PURCHASED) << itemid << GetName();

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_ITEM_BUY) << uint16(1) << itemid << leftover_count << item_slot << dest_slot;
	Send(&result);

	if (item_slot < 4 && leftover_count == 0)
	{
		result.clear();
		result.Initialize(WIZ_MERCHANT_INOUT);
		result << uint8(2) << uint32(m_sMerchantsSocketID) << uint8(1) << uint8(0) << item_slot;
		pUserBot->SendToRegion(&result);
	}

	int nItemsRemaining = 0;
	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		if (pUserBot->m_arMerchantItems[i].nNum != 0 && !pUserBot->m_arMerchantItems[i].IsSoldOut)
			nItemsRemaining++;
	}

	/*if (nItemsRemaining > 0)
		MerchantSlipRefList(pUserBot);*/

	if (nItemsRemaining == 0)
	{
		pUserBot->LastWarpTime = UNIXTIME + 10;

		pUserBot->m_bMerchantViewer = -1;
		pUserBot->m_bSellingMerchantPreparing = false;
		pUserBot->m_bMerchantState = MERCHANT_STATE_NONE;

		Packet res(WIZ_MERCHANT, uint8(MERCHANT_CLOSE));
		res << uint32(pUserBot->GetID());
		pUserBot->SendToRegion(&res);
		return;
	}
}

void CUser::MerchantItemUserBuy(Packet& pkt)
{
	uint32 itemid, req_gold;
	uint16 item_count, leftover_count;
	uint8 item_slot, dest_slot;
	Packet result(WIZ_MERCHANT);

	CUser* pMerchUser = g_pMain->GetUserPtr(m_sMerchantsSocketID);
	if (pMerchUser == nullptr)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	if (pMerchUser->GetSocketID() == GetSocketID() || pMerchUser->GetAccountName() == GetAccountName())
		return goDisconnect("trying to buy parts from its own market.", __FUNCTION__);

	if (!isInGame() 
		|| isDead() 
		|| isMining() 
		|| isFishing()
		|| isMerchanting()
		|| isSellingMerchantingPreparing()
		|| isBuyingMerchant()
		|| m_sMerchantsSocketID < 0
		|| m_sMerchantsSocketID > MAX_USER 
		|| isTrading())
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	if (!isInRange(pMerchUser, 35.0f) 
		|| !pMerchUser->isMerchanting() 
		|| pMerchUser->isSellingMerchantingPreparing()
		|| pMerchUser->GetMerchantState() != MERCHANT_STATE_SELLING)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	pkt >> itemid >> item_count >> item_slot >> dest_slot;
	if (item_slot >= MAX_MERCH_ITEMS || dest_slot >= HAVE_MAX || !item_count)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	// Grab pointers to the items.
	_MERCH_DATA* pMerch = &pMerchUser->m_arMerchantItems[item_slot];
	if (pMerch == nullptr
		|| pMerch->IsSoldOut
		|| !pMerch->sCount
		|| !pMerch->nPrice)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	_ITEM_DATA* pUserBuy = GetItem(SLOT_MAX + dest_slot);
	if (pUserBuy == nullptr)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	if (pMerch->nNum != itemid || pMerch->sCount < item_count || pMerch->bCount < item_count)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	_ITEM_TABLE pSellingItem = g_pMain->GetItemPtr(itemid);
	if (pSellingItem.isnull() || (!pSellingItem.m_bCountable && item_count != 1))
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;

	}

	if (pSellingItem.m_bKind == 255 && item_count != 1 && !pSellingItem.m_bCountable)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	uint32 nReqWeight = pSellingItem.m_sWeight * item_count;
	if ((nReqWeight + m_sItemWeight > m_sMaxWeight))
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	if (pUserBuy->nNum != 0 && (pUserBuy->nNum != itemid || !pSellingItem.m_bCountable))
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	_ITEM_DATA* pSellerItem = pMerchUser->GetItem(pMerch->bOriginalSlot);
	if (pSellerItem == nullptr 
		|| pSellerItem->nNum != pMerch->nNum
		|| !pSellerItem->isMerchantItem() 
		|| pSellerItem->sCount < item_count
		|| !pSellerItem->sCount)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	int8 PosXZ = FindSlotForItem(pMerch->nNum, pMerch->sCount);
	if (PosXZ < 0)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	// Do we have enough coins?
	req_gold = pMerch->nPrice * item_count;
	if (pMerch->isKC)
	{
		if (m_nKnightCash < req_gold)
		{
			result << uint8(6) << uint16(-18);
			Send(&result);
			return;
		}
	}
	else
	{
		if (m_iGold < req_gold)
		{
			result << uint8(6) << uint16(-18);
			Send(&result);
			return;
		}
	}

	if (pMerchUser && pMerchUser->m_iGold + req_gold > COIN_MAX)
	{
		result << uint8(6) << uint16(-18);
		Send(&result);
		return;
	}

	if (pMerch->isKC)
	{
		if (!CashLose(req_gold))
		{
			result << uint8(6) << uint16(-18);
			Send(&result);
			return;
		}
		if (pMerchUser) pMerchUser->CashGain(req_gold);
	}
	else
	{
		if (!GoldLose(req_gold))
		{
			result << uint8(6) << uint16(-18);
			Send(&result);
			return;
		}
		if (pMerchUser) pMerchUser->GoldGain(req_gold);
	}

	if (!pMerch->isKC)
		pMerchUser->pUserDailyRank.GMTotalSold += req_gold;

	leftover_count = pMerch->sCount - item_count;
	pUserBuy->sCount += item_count;

	SendStackChange(itemid, pUserBuy->sCount, pUserBuy->sDuration, dest_slot, (pUserBuy->sCount == item_count)); // is it a new item?

	pMerch->sCount -= item_count;
	pSellerItem->sCount -= item_count;

	uint64 serial = pSellerItem->nSerialNum;
	if (!serial) serial = g_pMain->GenerateItemSerial();

	if (pSellingItem.isStackable())
	{
		if (!pSellerItem->sCount && !pUserBuy->nNum)
			pUserBuy->nSerialNum = serial;
		else if (!pUserBuy->nNum)
			pUserBuy->nSerialNum = g_pMain->GenerateItemSerial();
	}
	else pUserBuy->nSerialNum = serial;

	pUserBuy->nNum = pMerch->nNum;
	pUserBuy->sDuration = pMerch->sDuration;

	if (!pSellingItem.isStackable() 
		|| item_count == pMerch->sCount) 
		pUserBuy->nSerialNum = pSellerItem->nSerialNum;

	if (!pSellingItem.isStackable() 
		&& pUserBuy->nSerialNum == 0) 
		pUserBuy->nSerialNum = g_pMain->GenerateItemSerial();

	pUserBuy->MerchItem = false;

	MerchantShoppingDetailInsertLog(false, 1, itemid, item_count, pMerch->nPrice, pMerchUser);
#if(GAME_SOURCE_VERSION == 2369)
	HandleHShieldSoftwareNewItemMoveLoadderHandler();
#endif
	if (!pSellerItem->sCount
		|| (pSellingItem.m_bKind == 255 
			&& pSellingItem.m_bCountable == 0)) 
		memset(pSellerItem, 0, sizeof(_ITEM_DATA));

	pMerchUser->SendStackChange(pSellerItem->nNum, pSellerItem->sCount, pSellerItem->sDuration, pMerch->bOriginalSlot - SLOT_MAX);

	if (!pMerch->sCount
		|| (pSellingItem.m_bCountable == 0 && pSellingItem.m_bKind == 255)) 
	{ 
		memset(pMerch, 0, sizeof(_MERCH_DATA)); 
		pMerch->IsSoldOut = true; 
	}

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_ITEM_PURCHASED) << itemid << GetName();
	pMerchUser->Send(&result);

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_ITEM_BUY) 
		<< uint16(1)
		<< itemid 
		<< leftover_count
		<< item_slot 
		<< dest_slot;
	Send(&result);

	if (item_slot < 4 && leftover_count == 0)
	{
		result.clear();
		result.Initialize(WIZ_MERCHANT_INOUT);
		result << uint8(2) 
			<< uint32(m_sMerchantsSocketID)
			<< uint8(1) 
			<< uint8(0) 
			<< item_slot;
		pMerchUser->SendToRegion(&result, nullptr, GetEventRoom());
	}

	int nItemsRemaining = 0;
	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		if (pMerchUser->m_arMerchantItems[i].nNum != 0 
			&& !pMerchUser->m_arMerchantItems[i].IsSoldOut)
			nItemsRemaining++;
	}

	if (nItemsRemaining == 0
		&& pMerchUser) 
		pMerchUser->MerchantClose();

	if (nItemsRemaining > 0) 
		MerchantSlipRefList(pMerchUser);
}

void CUser::CancelMerchant()
{
	if (m_sMerchantsSocketID < 0) return;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_TRADE_CANCEL));
	result << uint16(1);
	Send(&result);
	RemoveFromMerchantLookers();
}

void CUser::BuyingMerchantOpen(Packet& pkt)
{
	if (isSellingMerchantingPreparing())
		return;

	int16 errorCode = 0;
	if (isDead())
		errorCode = MERCHANT_OPEN_DEAD;
	else if (isStoreOpen())
		errorCode = MERCHANT_OPEN_SHOPPING;
	else if (isTrading())
		errorCode = MERCHANT_OPEN_TRADING;
	/*else if (GetZoneID() > ZONE_MORADON || GetZoneID() <= ELMORAD)
		errorCode = MERCHANT_OPEN_INVALID_ZONE;*/
	else if (GetLevel() < g_pMain->pServerSetting.MerchantLevel)
		errorCode = MERCHANT_OPEN_UNDERLEVELED;
	else if (isMerchanting())
		errorCode = MERCHANT_OPEN_MERCHANTING;
	else
		errorCode = MERCHANT_OPEN_SUCCESS;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_OPEN));
	result << errorCode;
	Send(&result);

	if (errorCode == MERCHANT_OPEN_MERCHANTING)
		BuyingMerchantClose();

	if (errorCode == MERCHANT_OPEN_SUCCESS)
		m_bBuyingMerchantPreparing = true;

	memset(&m_arMerchantItems, 0, sizeof(m_arMerchantItems));
	DateTime time;
}

void CUser::BuyingMerchantClose()
{
	if (isSellingMerchantingPreparing() || isSellingMerchant())
		return;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_CLOSE));
	result << uint32(GetSocketID());

	if (isBuyingMerchant())
	{
		if (m_sPlookerSocketID >= 0)
		{
			if (CUser* pUser = g_pMain->GetUserPtr(m_sPlookerSocketID))
			{
				pUser->m_sMerchantsSocketID = -1;
				Packet newpkt(WIZ_MERCHANT, uint8(29));
				newpkt << uint16(1);
				pUser->Send(&newpkt);
			}
		}
		m_sLevelMerchantTime = 0;
		m_sPlookerSocketID = -1;
		m_bBuyingMerchantPreparing = false;
		m_bMerchantState = MERCHANT_STATE_NONE;
		memset(m_arMerchantItems, 0, sizeof(m_arMerchantItems));
		Send(&result);
		SendToRegion(&result, nullptr, GetEventRoom());
		GetOfflineStatus(_choffstatus::DEACTIVE, offcharactertype::merchant);
	}
	else if (m_sMerchantsSocketID >= 0)
	{
		RemoveFromMerchantLookers();
		Send(&result);
	}
}

void CUser::BuyingMerchantInsert(Packet& pkt)
{
	if (isGM())
		BuyingMerchantBotInsert(pkt);
	else
		BuyingMerchantUserInsert(pkt);
}

void CUser::BuyingMerchantUserInsert(Packet& pkt)
{
	if (isDead() 
		|| isTrading() 
		|| isMining() 
		|| isFishing())
		return;

	if (!isBuyingMerchantingPreparing() 
		|| isSellingMerchantingPreparing() 
		|| isMerchanting())
		return;

	if (GetLevel() < g_pMain->pServerSetting.MerchantLevel)
		return;

	uint32 totalamount = 0;
	uint8 t_itemcount = pkt.read<uint8>(), n_itemcount = 0;
	if (!t_itemcount 
		|| t_itemcount > MAX_MERCH_ITEMS)
		return;

	for (int i = 0; i < t_itemcount; i++)
	{
		uint32 itemid, buying_price; uint16 item_count;
		pkt >> itemid >> item_count >> buying_price;
		auto pItem = g_pMain->GetItemPtr(itemid);
		if (pItem.isnull())
			return;

		if (!item_count 
			|| !buying_price
			|| (!pItem.m_bCountable && item_count != 1)
			|| (itemid >= ITEM_NO_TRADE_MIN && itemid <= ITEM_NO_TRADE_MAX)
			|| pItem.m_bRace == RACE_UNTRADEABLE || pItem.m_bCountable == 2)
			return;

		if (pItem.m_bKind == 255 
			&& !pItem.m_bCountable 
			&& item_count != 1)
			return;

		m_arMerchantItems[i].nNum = itemid;
		m_arMerchantItems[i].sCount = item_count;
		m_arMerchantItems[i].nPrice = buying_price;
		m_arMerchantItems[i].sDuration = pItem.m_sDuration;
		totalamount += buying_price;
		n_itemcount++;
	}

	if (!hasCoins(totalamount) || n_itemcount != t_itemcount)
		return;

	if (g_pMain->pLevMercInfo.status)
		m_sLevelMerchantTime = UNIXTIME + (g_pMain->pLevMercInfo.rewardtime * MINUTE);

	m_bMerchantState = MERCHANT_STATE_BUYING;
	m_bBuyingMerchantPreparing = false;
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_INSERT));
	result << uint8(1);
	Send(&result);

	BuyingMerchantInsertRegion();
	GetOfflineStatus(_choffstatus::ACTIVE, offcharactertype::merchant);
	MerchantCreationInsertLog(2);
}

void CUser::BuyingMerchantBotInsert(Packet& pkt)
{
	if (isDead()
		|| isTrading()
		|| isMining()
		|| isFishing())
		return;

	if (!isBuyingMerchantingPreparing()
		|| isSellingMerchantingPreparing()
		|| isMerchanting())
		return;

	if (GetLevel() < g_pMain->pServerSetting.MerchantLevel)
		return;

	uint32 totalamount = 0;
	uint8 t_itemcount = pkt.read<uint8>(), n_itemcount = 0;
	if (!t_itemcount
		|| t_itemcount > MAX_MERCH_ITEMS)
		return;

	for (int i = 0; i < t_itemcount; i++)
	{
		uint32 itemid, buying_price; uint16 item_count;
		pkt >> itemid >> item_count >> buying_price;
		auto pItem = g_pMain->GetItemPtr(itemid);
		if (pItem.isnull())
			return;

		if (!item_count
			|| !buying_price
			|| (!pItem.m_bCountable && item_count != 1)
			|| (itemid >= ITEM_NO_TRADE_MIN && itemid <= ITEM_NO_TRADE_MAX)
			|| pItem.m_bRace == RACE_UNTRADEABLE || pItem.m_bCountable == 2)
			return;

		if (pItem.m_bKind == 255
			&& !pItem.m_bCountable
			&& item_count != 1)
			return;

		m_arMerchantItems[i].nNum = itemid;
		m_arMerchantItems[i].sCount = item_count;
		m_arMerchantItems[i].nPrice = buying_price;
		m_arMerchantItems[i].sDuration = pItem.m_sDuration;
		totalamount += buying_price;
		n_itemcount++;
	}

	CBot* pUser = nullptr;
	uint16 sCount = myrand(1, 10);
	for (int i = 0; i < 1; i++)
	{
		float Bonc = myrand(1, 15) * 1.0f;
		float Bonc2 = myrand(1, 15) * 1.0f;

		uint16 m_bSocketID = g_pMain->SpawnEventBotMerchant(3600, GetZoneID(), GetX(), GetY(), GetZ(), NULL, MIN_LEVEL_ARDREAM);
		if (m_bSocketID)
		{
			pUser = g_pMain->GetBotPtr(m_bSocketID);
			if (pUser == nullptr)
				return;

			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				pUser->m_arMerchantItems[i] = m_arMerchantItems[i];

			if (!hasCoins(totalamount))
				return;

			int count = 0, count2 = 0;
			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				if (pUser->m_arMerchantItems[i].nNum)
					count++;

			for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
				if (!pUser->GetItem(i)->nNum)
					count2++;

			if (count > count2)
			{
				count2 = 0;
				for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
				{
					auto* pItem = pUser->GetItem(i);
					if (pItem)
					{
						memset(pItem, 0, sizeof(_ITEM_DATA));
						count2++;
					}
					if (count2 >= count)
						break;
				}
			}

			pUser->m_iLoyalty = myrand(100, 5000);
			pUser->m_bMerchantState = MERCHANT_STATE_BUYING;

			Packet result;
			result.clear();
			result.Initialize(WIZ_MERCHANT);
			result << uint8(MERCHANT_BUY_REGION_INSERT) << uint32(pUser->GetID());

			for (int i = 0; i < 4; i++)
				result << pUser->m_arMerchantItems[i].nNum;

			pUser->SendToRegion(&result);

			if (pUser->m_iGold < totalamount)
				pUser->m_iGold = myrand(totalamount, totalamount + 5000000);
		}
	}

	m_sPlookerSocketID = -1;
	m_bBuyingMerchantPreparing = false;
	m_bMerchantState = MERCHANT_STATE_NONE;
	memset(m_arMerchantItems, 0, sizeof(m_arMerchantItems));
	Packet result;
	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_BUY_CLOSE) << uint32(GetSocketID());
	SendToRegion(&result, nullptr, GetEventRoom());
}

void CUser::BuyingMerchantInsertRegion()
{
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_REGION_INSERT));
	result << uint32(GetSocketID());
	DateTime time;

	for (int i = 0; i < 4; i++)
		result << m_arMerchantItems[i].nNum;

	SendToRegion(&result, nullptr, GetEventRoom());
}

void CUser::BuyingMerchantList(Packet& pkt)
{
	uint32 m_dSocketID;
	pkt >> m_dSocketID;
	int16 test = m_dSocketID;
	if (test < NPC_BAND)
	{
		if (test < MAX_USER)
			BuyingMerchantUserList(pkt, test);
		else
			BuyingMerchantBotList(pkt, test);
	}
}

void CUser::BuyingMerchantBotList(Packet& pkt, uint16 m_bSocketID)
{
	CBot* pUser = g_pMain->GetBotPtr(m_bSocketID);

	if (pUser == nullptr
		|| !pUser->isMerchanting()
		|| !pUser->isBuyingMerchant())
		return;

	if (pUser->m_bMerchantViewer >= 0)
	{
		CUser* pLooker = g_pMain->GetUserPtr(pUser->m_bMerchantViewer);
		if (pLooker && pLooker->isInGame())
		{
			Packet newpkt(WIZ_MERCHANT, uint8(MERCHANT_ITEM_LIST));
			newpkt.SByte();
			newpkt << int16(-7) << pLooker->GetName();
			Send(&newpkt);
			return;
		}
		else pUser->m_bMerchantViewer = -1;
	}

	pUser->m_bMerchantViewer = GetID();
	m_sMerchantsSocketID = m_bSocketID;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_LIST));
	result << uint8(1) << uint32(m_bSocketID);

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		_MERCH_DATA* pMerch = &pUser->m_arMerchantItems[i];
		if (!pMerch) 
			continue;

		result << pMerch->nNum << pMerch->sCount << pMerch->sDuration << pMerch->nPrice;
	}
	Send(&result);
}

void CUser::BuyingMerchantUserList(Packet& pkt, uint16 m_bSocketID)
{
	CUser* pUser = g_pMain->GetUserPtr(m_bSocketID);
	if (pUser == nullptr 
		|| !pUser->isMerchanting() 
		|| pUser->isSellingMerchantingPreparing() 
		|| !pUser->isBuyingMerchant())
		return;

	if (pUser->m_sPlookerSocketID >= 0)
	{
		CUser* pLooker = g_pMain->GetUserPtr(pUser->m_sPlookerSocketID);
		if (pLooker && pLooker->isInGame())
		{
			Packet newpkt(WIZ_MERCHANT, uint8(MERCHANT_ITEM_LIST));
			newpkt.SByte();
			newpkt << int16(-7) << pLooker->GetName();
			Send(&newpkt);
		}
		else pUser->m_sPlookerSocketID = -1;
	}

	pUser->m_sPlookerSocketID = GetID();
	m_sMerchantsSocketID = m_bSocketID;

	if (g_pMain->pServerSetting.MerchantView)
	{
		if (pUser != nullptr)
		{
			DateTime timemachine;
#if(__VERSION < 2369)
			Packet merchanwiew(WIZ_HSACS_HOOK);
			merchanwiew << uint8(HSACSXOpCodes::MERC_WIEWER_INFO) << uint8(1);
			merchanwiew.SByte();
			merchanwiew << GetName() << timemachine.GetHour() << timemachine.GetMinute();
			pUser->Send(&merchanwiew);
#endif
		}
	}
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_LIST));
	result << uint8(1) << uint32(m_bSocketID);

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		_MERCH_DATA* pMerch = &pUser->m_arMerchantItems[i];
		if (pMerch == nullptr)
			continue;

		result << pMerch->nNum << pMerch->sCount << pMerch->sDuration << pMerch->nPrice;
	}
	Send(&result);
}

void CUser::BuyingMerchantBuy(Packet& pkt)
{
	if (m_sMerchantsSocketID < NPC_BAND)
	{
		if (m_sMerchantsSocketID < MAX_USER)
			BuyingMerchantUserBuy(pkt);
		else
			BuyingMerchantBotBuy(pkt);
	}
}

void CUser::BuyingMerchantUserBuy(Packet& pkt)
{
	DateTime time;
	uint32 nPrice;
	uint16 sStackSize, sRemainingStackSize;
	uint8 bSellerSrcSlot, bMerchantListSlot;

	CUser* pUser = g_pMain->GetUserPtr(m_sMerchantsSocketID);

	if (pUser == nullptr
		|| pUser->GetMerchantState() != MERCHANT_STATE_BUYING 
		|| !pUser->isBuyingMerchant() 
		|| pUser->isSellingMerchantingPreparing())
		return;

	pkt >> bSellerSrcSlot >> bMerchantListSlot >> sStackSize;

	if (sStackSize == 0
		|| bSellerSrcSlot >= HAVE_MAX
		|| bMerchantListSlot >= MAX_MERCH_ITEMS)
		return;

	_MERCH_DATA* pWantedItem = &pUser->m_arMerchantItems[bMerchantListSlot];
	_ITEM_DATA* pSellerItem = GetItem(SLOT_MAX + bSellerSrcSlot);

	if (pWantedItem == nullptr || pSellerItem == nullptr)
		return;

	if (pWantedItem->nNum != pSellerItem->nNum
		|| pWantedItem->sCount < sStackSize
		|| pSellerItem->sCount < sStackSize
		// For scrolls, this will ensure you can only sell a full stack of scrolls.
		// For everything else, this will ensure you cannot sell items that need repair.
		|| pSellerItem->sDuration != pWantedItem->sDuration
		|| pSellerItem->isDuplicate()
		|| pSellerItem->isSealed()
		|| pSellerItem->isBound()
		|| pSellerItem->isRented())
		return;

	// If it's not stackable, and we're specifying something other than 1
	// we really don't care to handle this request...
	_ITEM_TABLE proto = g_pMain->GetItemPtr(pWantedItem->nNum);
	if (proto.isnull() || (!proto.m_bCountable && sStackSize != 1))
		return;

	// Do they have enough coins?
	nPrice = pWantedItem->nPrice * sStackSize;
	if (pWantedItem->isKC && pUser->m_nKnightCash < nPrice)
		return;
	else if (!pUser->hasCoins(nPrice))
		return;

	// Now find the buyer a home for their item
	int8 bDstPos = pUser->FindSlotForItem(pWantedItem->nNum, sStackSize);
	if (bDstPos < 0)
		return;

	_ITEM_DATA* pMerchantItem = pUser->GetItem(bDstPos);
	if (pMerchantItem == nullptr)
		return;

	// Take coins off the buying merchant
	if (pWantedItem->isKC)
	{
		if (!pUser->CashLose(nPrice))
			return;

		CashGain(nPrice);
	}
	else
	{
		if (!pUser->GoldLose(nPrice))
			return;

		GoldGain(nPrice);
		pUserDailyRank.GMTotalSold += nPrice;
	}

	// Get the remaining stack size after purchase.
	sRemainingStackSize = pSellerItem->sCount - sStackSize;

	// Now we give the buying merchant their wares.
	pMerchantItem->nNum = pSellerItem->nNum;
	pMerchantItem->sDuration = pSellerItem->sDuration;
	pSellerItem->sCount -= sStackSize;
	pMerchantItem->sCount += sStackSize;

	// Update how many items the buyer still needs.
	pWantedItem->sCount -= sStackSize;

	// If the buyer needs no more, remove this item from the wanted list.
	if (pWantedItem->sCount == 0)
		memset(pWantedItem, 0, sizeof(_MERCH_DATA));

	// If the seller's all out, remove their item.
	if (pSellerItem->sCount <= 0)
		memset(pSellerItem, 0, sizeof(_ITEM_DATA));

	// TODO : Proper checks for the removal of the items in the array, we're now assuming everything gets bought

	// Update players
	SendStackChange(pSellerItem->nNum, pSellerItem->sCount, pSellerItem->sDuration, bSellerSrcSlot);
	pUser->SendStackChange(pMerchantItem->nNum, pMerchantItem->sCount, pMerchantItem->sDuration, bDstPos - SLOT_MAX,
		pMerchantItem->sCount == sStackSize); 	// if the buying merchant only has what they wanted, it's a new item.
	// (otherwise it was a stackable item that was merged into an existing slot)

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_BOUGHT));
	result << bMerchantListSlot << uint16(0) << GetName();
	pUser->Send(&result);

	result.clear();
	result << uint8(MERCHANT_BUY_SOLD) << uint8(1) << bMerchantListSlot << pWantedItem->sCount << bSellerSrcSlot << pSellerItem->sCount;
	Send(&result);

	result.clear();
	result << uint8(MERCHANT_BUY_BUY) << uint8(1);
	Send(&result);

	if (bMerchantListSlot < 4 && pWantedItem->sCount == 0)
	{
		result.Initialize(WIZ_MERCHANT_INOUT);
		result << uint8(2) << uint32(m_sMerchantsSocketID) << uint8(1) << uint8(0) << bMerchantListSlot;
		pUser->SendToRegion(&result, nullptr, GetEventRoom());
	}

	int nItemsRemaining = 0;
	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		if (pUser->m_arMerchantItems[i].nNum != 0)
			nItemsRemaining++;
	}

	MerchantShoppingDetailInsertLog(false, 2, pWantedItem->nNum, pWantedItem->sCount, pWantedItem->nPrice, pUser);
	if (nItemsRemaining == 0) pUser->BuyingMerchantClose();
}

void CUser::BuyingMerchantBotBuy(Packet& pkt)
{
	DateTime time;
	uint32 nPrice;
	uint16 sStackSize, sRemainingStackSize;
	uint8 bSellerSrcSlot, bMerchantListSlot;

	CBot* pUser = g_pMain->GetBotPtr(m_sMerchantsSocketID);
	if (pUser == nullptr 
		|| pUser->GetMerchantState() != MERCHANT_STATE_BUYING 
		|| !pUser->isBuyingMerchant())
		return;

	pkt >> bSellerSrcSlot >> bMerchantListSlot >> sStackSize;

	if (sStackSize == 0
		|| bSellerSrcSlot >= HAVE_MAX
		|| bMerchantListSlot >= MAX_MERCH_ITEMS)
		return;

	_MERCH_DATA* pWantedItem = &pUser->m_arMerchantItems[bMerchantListSlot];
	_ITEM_DATA* pSellerItem = GetItem(SLOT_MAX + bSellerSrcSlot);

	// Make sure the merchant actually has that item in that slot
	// and that they want enough, and the selling user has enough
	if (pWantedItem == nullptr
		|| pSellerItem == nullptr
		|| pWantedItem->nNum != pSellerItem->nNum
		|| pWantedItem->sCount < sStackSize
		|| pSellerItem->sCount < sStackSize
		// For scrolls, this will ensure you can only sell a full stack of scrolls.
		// For everything else, this will ensure you cannot sell items that need repair.
		|| pSellerItem->sDuration != pWantedItem->sDuration
		|| pSellerItem->isDuplicate()
		|| pSellerItem->isSealed()
		|| pSellerItem->isBound()
		|| pSellerItem->isRented())
		return;

	// If it's not stackable, and we're specifying something other than 1
	// we really don't care to handle this request...
	_ITEM_TABLE proto = g_pMain->GetItemPtr(pWantedItem->nNum);
	if (proto.isnull() || (!proto.m_bCountable && sStackSize != 1))
		return;

	// Do they have enough coins?
	nPrice = pWantedItem->nPrice * sStackSize;

	// and give them all to me, me, me!
	GoldGain(nPrice);

	// Get the remaining stack size after purchase.
	sRemainingStackSize = pSellerItem->sCount - sStackSize;

	// Update how many items the buyer still needs.
	pSellerItem->sCount -= sStackSize;
	pWantedItem->sCount -= sStackSize;

	// If the buyer needs no more, remove this item from the wanted list.
	if (pWantedItem->sCount == 0)
		memset(pWantedItem, 0, sizeof(_MERCH_DATA));

	// If the seller's all out, remove their item.
	if (pSellerItem->sCount <= 0)
		memset(pSellerItem, 0, sizeof(_ITEM_DATA));

	// Update players
	SendStackChange(pSellerItem->nNum, pSellerItem->sCount, pSellerItem->sDuration, bSellerSrcSlot);

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_BOUGHT));
	result << bMerchantListSlot << uint16(0) << GetName();

	result.clear();
	result << uint8(MERCHANT_BUY_SOLD) << uint8(1) << bMerchantListSlot << pWantedItem->sCount << bSellerSrcSlot << pSellerItem->sCount;
	Send(&result);

	result.clear();
	result << uint8(MERCHANT_BUY_BUY) << uint8(1);
	Send(&result);

	if (bMerchantListSlot < 4 && pWantedItem->sCount == 0)
	{
		result.Initialize(WIZ_MERCHANT_INOUT);
		result << uint8(2) << uint32(m_sMerchantsSocketID) << uint8(1) << uint8(0) << bMerchantListSlot;
		pUser->SendToRegion(&result);
	}

	int nItemsRemaining = 0;
	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		if (pUser->m_arMerchantItems[i].nNum != 0)
			nItemsRemaining++;
	}

	if (nItemsRemaining == 0)
	{
		pUser->LastWarpTime = UNIXTIME + 10;
		pUser->m_bMerchantViewer = -1;
		pUser->m_bBuyingMerchantPreparing = false;
		pUser->m_bMerchantState = MERCHANT_STATE_NONE;
		memset(pUser->m_arMerchantItems, 0, sizeof(pUser->m_arMerchantItems));

		Packet result(WIZ_MERCHANT, uint8(MERCHANT_BUY_CLOSE));
		result << uint32(pUser->GetID());
		pUser->SendToRegion(&result);
	}
	MerchantShoppingDetailInsertLog(true, 2, pWantedItem->nNum, pWantedItem->sCount, pWantedItem->nPrice, nullptr);
}

void CUser::RemoveFromMerchantLookers()
{
	int16 psockid = m_sMerchantsSocketID;

	m_sMerchantsSocketID = -1;

	CBot* pBot = g_pMain->GetBotPtr(psockid);
	if (pBot)
	{
		pBot->m_bMerchantViewer = -1;
		return;
	}

	CUser* pUser = g_pMain->GetUserPtr(psockid);
	if (!pUser) return;

	pUser->m_sPlookerSocketID = -1;

	if (g_pMain->pServerSetting.MerchantView)
	{
#if(__VERSION < 2369)
		DateTime timemachine;
		Packet merchanwiew(WIZ_HSACS_HOOK);
		merchanwiew << uint8(HSACSXOpCodes::MERC_WIEWER_INFO) << uint8(2); // Merchant Kapatildi Yazisi Düzeltildi.
		merchanwiew.SByte();
		merchanwiew << GetName() << timemachine.GetHour() << timemachine.GetMinute();
		pUser->Send(&merchanwiew);
#endif
	}
}

void CUser::MerchantOfficialList(Packet& pkt)
{

#if 1
	return;
#endif

	if (!isInGame() 
		|| isDead() 
		|| isTrading()
		|| isMerchanting()
		|| !isInMoradon()
		|| isMining() 
		|| isFishing())
		return;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_MENISIA_LIST));
	if (!GetMap() || GetMap()->m_bMenissiahList != 1)
		return;

	uint8 OpCode = pkt.read<uint8>();
	switch (OpCode)
	{
	case 1:
		MerchantListSend(pkt);
		break;
	case 2:
		MerchantListMoveProcess(pkt);
		break;
	default:
		//printf("MerchantOfficialList unhandled packets %d \n", OpCode);
		break;
	}
}

#define MAX_MERCHANT_RANGE  (35.0f)

void CUser::MerchantList(Packet& pkt)  //2369 Merchant list paket void
{
	uint32 m_bSocketID = pkt.read<uint32>();
	int16 test = (int16)m_bSocketID;
	if (test < NPC_BAND)
	{
		if (test < MAX_USER)
			MerchantUserList(pkt, test);
		else
			MerchantBotList(pkt, test);
	}
}

void CUser::MerchantUserList(Packet& pkt, uint16 m_bSocketID)  //2369 Merchant list paket void
{
	Packet result;
	CUser* pMerchUser = g_pMain->GetUserPtr(m_bSocketID);
	if (pMerchUser == nullptr || !pMerchUser->isInGame()
		|| pMerchUser->GetEventRoom() != GetEventRoom()
		|| pMerchUser->GetZoneID() != GetZoneID()
		|| !pMerchUser->isMerchanting())
		goto fail_result;

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_LIST);

	_MERCH_DATA pnewlist[MAX_MERCH_ITEMS] = {};
	memset(pnewlist, 0, sizeof(pnewlist));
	if (pMerchUser->isSellingMerchant())
	{
		uint8 PremiumState = pMerchUser->GetMerchantPremiumState() ? 1 : 0;
		result << uint8(1) << uint32(pMerchUser->GetID()) << pMerchUser->GetMerchantState() << PremiumState;

		GetMerchantSlipList(pnewlist, pMerchUser);
		for (int i = 0, x = PremiumState == 1 ? 8 : 4; i < x; i++)
		{
			if (!pnewlist[i].IsSoldOut)
				result << pnewlist[i].nNum;
			else
				result << uint32(0);
		}
	}
	else
	{
		result << uint8(1)
			<< uint32(pMerchUser->GetID())
			<< pMerchUser->GetMerchantState()
			<< uint8(0);

		for (int i = 0; i < 4; i++)
			result << pMerchUser->m_arMerchantItems[i].nNum;
	}
	Send(&result);
#if 0
	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_EYE_LIST)
		<< uint8(1)
		<< uint32(pMerchUser->GetID())
		<< uint8(pMerchUser->GetMerchantState() == MERCHANT_STATE_SELLING ? false : true)
		<< (pMerchUser->GetMerchantState() == 1 ? false : pMerchUser->m_bPremiumMerchant); // Type of merchant [normal - gold] // bool*/

	for (int i = 0, listCount = (pMerchUser->GetMerchantState() == 1 ? 4 : (pMerchUser->m_bPremiumMerchant ? 8 : 4)); i < listCount; i++)
		result << (pMerchUser->GetMerchantState() == MERCHANT_STATE_SELLING ? pnewlist[i].nPrice : pMerchUser->m_arMerchantItems[i].nPrice)
		<< (pMerchUser->GetMerchantState() == MERCHANT_STATE_SELLING ? uint8(pnewlist[i].isKC) : uint8(pMerchUser->m_arMerchantItems[i].isKC));

	Send(&result);
#endif
	return;
fail_result:
	return;
#if 0
	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_LIST) << uint8(4);
	Send(&result);
#endif
}

void CUser::MerchantBotList(Packet& pkt, uint16 m_bSocketID)  //2369 Merchant list paket void
{
	Packet result;
	CBot* pMerchUser = g_pMain->GetBotPtr(m_bSocketID);
	if (pMerchUser == nullptr || !pMerchUser->isInGame()
		|| pMerchUser->GetEventRoom() != GetEventRoom()
		|| pMerchUser->GetZoneID() != GetZoneID()
		|| !pMerchUser->isMerchanting())
		goto fail_result;

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_LIST);

	_MERCH_DATA pnewlist[MAX_MERCH_ITEMS] = {};
	memset(pnewlist, 0, sizeof(pnewlist));
	if (pMerchUser->isSellingMerchant())
	{
		uint8 PremiumState = pMerchUser->GetMerchantPremiumState() ? 1 : 0;
		result << uint8(1) << uint32(pMerchUser->GetID()) << pMerchUser->GetMerchantState() << PremiumState;

		GetMerchantSlipList(pnewlist, pMerchUser);
		for (int i = 0, x = PremiumState == 1 ? 8 : 4; i < x; i++)
		{
			if (!pnewlist[i].IsSoldOut)
				result << pnewlist[i].nNum;
			else
				result << uint32(0);
		}
	}
	else
	{
		result << uint8(1)
			<< uint32(pMerchUser->GetID())
			<< pMerchUser->GetMerchantState()
			<< uint8(0);

		for (int i = 0; i < 4; i++)
			result << pMerchUser->m_arMerchantItems[i].nNum;
	}
	Send(&result);
#if 0
	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_EYE_LIST)
		<< uint8(1)
		<< uint32(pMerchUser->GetID())
		<< uint8(pMerchUser->GetMerchantState() == MERCHANT_STATE_SELLING ? false : true)
		<< (pMerchUser->GetMerchantState() == 1 ? false : pMerchUser->m_bPremiumMerchant); // Type of merchant [normal - gold] // bool*/

	for (int i = 0, listCount = (pMerchUser->GetMerchantState() == 1 ? 4 : (pMerchUser->m_bPremiumMerchant ? 8 : 4)); i < listCount; i++)
		result << (pMerchUser->GetMerchantState() == MERCHANT_STATE_SELLING ? pnewlist[i].nPrice : pMerchUser->m_arMerchantItems[i].nPrice)
		<< (pMerchUser->GetMerchantState() == MERCHANT_STATE_SELLING ? uint8(pnewlist[i].isKC) : uint8(pMerchUser->m_arMerchantItems[i].isKC));

	Send(&result);
#endif
	return;
fail_result:
	return;
#if 0
	result.clear();
	result.Initialize(WIZ_MERCHANT);
	result << uint8(MERCHANT_LIST) << uint8(4);
	Send(&result);
#endif
}

void CUser::MerchantEye()  //2369 Merchant list paket void
{
	// ÝTEMKOTNROLU EKLENECEK
	struct pMerchantEyeData
	{
		uint8 nIndex;
		std::string strMerchantItem[12];
		pMerchantEyeData()
		{
			for (int i = 0; i < 12; i++)
				strMerchantItem[i] = "";
			nIndex = 0;
		}
	};


	if (!CheckExistItem(810168000))
		return;

	uint16 nCount = 2;
	std::map<uint32, _MERCHANT_LIST*> filteredDamageList;

	int16 nSize = 0;

	std::map<uint16, uint16> TestMerchant;
	std::map<uint16, pMerchantEyeData> pMerchantEyeList;

	for (uint16 i = 0; i < NPC_BAND; i++)
	{
		if (i < MAX_USER)
		{
			CUser* pUser = g_pMain->GetUserPtr(i);
			if (pUser == nullptr
				|| !pUser->isInGame()
				|| !pUser->isMerchanting()
				|| pUser->GetZoneID() != GetZoneID())
				continue;

			pMerchantEyeData pMerchantEye;
			pMerchantEye.nIndex = 1;

			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
			{
				if (pUser->m_arMerchantItems[i].nNum == 0)
					continue;

				_ITEM_TABLE proto = g_pMain->GetItemPtr(pUser->m_arMerchantItems[i].nNum);
				if (proto.isnull())
					continue;

				pMerchantEye.strMerchantItem[i] = proto.m_sName.c_str();
			}
			pMerchantEyeList.insert(std::pair<uint16, pMerchantEyeData>(pUser->GetID(), pMerchantEye));
		}
		else
		{
			CBot* pUser = g_pMain->GetBotPtr(i);
			if (pUser == nullptr
				|| !pUser->isInGame()
				|| !pUser->isMerchanting()
				|| pUser->GetZoneID() != GetZoneID())
				continue;

			pMerchantEyeData pMerchantEye;
			pMerchantEye.nIndex = 1;

			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
			{
				if (pUser->m_arMerchantItems[i].nNum == 0)
					continue;

				_ITEM_TABLE proto = g_pMain->GetItemPtr(pUser->m_arMerchantItems[i].nNum);
				if (proto.isnull())
					continue;

				pMerchantEye.strMerchantItem[i] = proto.m_sName.c_str();
			}
			pMerchantEyeList.insert(std::pair<uint16, pMerchantEyeData>(pUser->GetID(), pMerchantEye));
		}
	}

	if (pMerchantEyeList.size() > 0)
	{
#if(__VERSION < 2369)
		Packet error(WIZ_HSACS_HOOK, uint8(0xDD));
		error << uint16(pMerchantEyeList.size());
		error.SByte();
		foreach(itr, pMerchantEyeList)
		{
			error << uint16(itr->first);
			for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				error << itr->second.strMerchantItem[i].c_str();
		}
		SendCompressed(&error);
#endif
	}
}
void CUser::MerchantListSend(Packet& pkt)
{
	uint32 RecvItemID = pkt.read<uint32>();
	Packet result(WIZ_MERCHANT, uint8(MERCHANT_MENISIA_LIST));

	std::vector<uint32> MerchantOfficalListen;
	int8 nCount = 0;

	if (RecvItemID != ITEM_MENICIAS_LIST)
	{
		result << uint8(1) << uint8(0) << uint8(3);
		Send(&result);
		return;
	}

	if (!CheckExistItem(ITEM_MENICIAS_LIST))
	{
		result << uint8(1) << uint8(0) << uint8(3);
		Send(&result);
		return;
	}

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);

		if (!pUser)
			continue;

		if (!pUser->isInGame() || !pUser->isMerchanting())
			continue;

		if (pUser->GetZoneID() == GetZoneID())
			MerchantOfficalListen.push_back(pUser->GetSocketID());
	}

	if (MerchantOfficalListen.size() > 0)
	{
		int16 nSize = (int16)MerchantOfficalListen.size();
		while (nSize > 0)
		{
			nCount++;
			result << uint8(1) << uint8(2) << nCount;

			if (nSize > 50)
				result << uint8(50);
			else
				result << uint8(nSize);

			result.SByte();
			foreach(itr, MerchantOfficalListen)
			{
				CUser* pUser = g_pMain->GetUserPtr(*itr);

				if (pUser == nullptr)
					continue;

				if (!pUser->isInGame() || !pUser->isMerchanting())
					continue;


				result << uint32(pUser->GetID()) << pUser->GetName() << pUser->GetMerchantState();

				for (int i = 0; i < MAX_MERCH_ITEMS; i++)
				{
					if (pUser->m_arMerchantItems[i].nNum == 3452816845)
						result << uint32(0) << uint32(0);
					else
						result << pUser->m_arMerchantItems[i].nNum << pUser->m_arMerchantItems[i].nPrice;
				}
			}
			Send(&result);
			nSize = nSize - 50;
		}
	}
	else
	{
		result << uint8(1) << uint8(2) << int8(0);
		Send(&result);
	}
}

void CUser::MerchantListMoveProcess(Packet& pkt)
{
	uint16 TargetID = pkt.read<uint16>();

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_MENISIA_LIST));
	if (isDead()
		|| isTrading()
		|| isMerchanting()
		|| isStoreOpen()
		|| isMining()
		|| isFishing())
	{
		result << uint8(1) << uint8(0) << uint8(3);
		Send(&result);
		return;
	}

	CUser* pUser = g_pMain->GetUserPtr(TargetID);
	if (pUser == nullptr)
	{
		result << uint8(1) << uint8(0) << uint8(3);
		Send(&result);
		return;
	}

	if (!pUser->isInGame() || !pUser->isMerchanting() || pUser->GetZoneID() != GetZoneID())
	{
		result << uint8(1) << uint8(0) << uint8(3);
		Send(&result);
		return;
	}

	ZoneChange(pUser->GetZoneID(), pUser->GetX(), pUser->GetZ());
}

void CUser::GetMerchantSlipList(_MERCH_DATA list[MAX_MERCH_ITEMS], CUser* pUser)
{
	if (!pUser) 
		return;

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		if (pUser->m_arMerchantItems[i].nNum)
		{
			for (int x = 0; x < MAX_MERCH_ITEMS; x++)
			{
				if (!list[x].nNum) { list[x] = pUser->m_arMerchantItems[i]; break; }
			}
		}
	}
}

void CUser::GetMerchantSlipList(_MERCH_DATA list[MAX_MERCH_ITEMS], CBot* pUser)
{
	if (!pUser)
		return;

	for (int i = 0; i < MAX_MERCH_ITEMS; i++)
	{
		if (pUser->m_arMerchantItems[i].nNum)
		{
			for (int x = 0; x < MAX_MERCH_ITEMS; x++)
			{
				if (!list[x].nNum) { list[x] = pUser->m_arMerchantItems[i]; break; }
			}
		}
	}
}

void CUser::MerchantSlipRefList(CUser* pUser, bool m_sSend)
{
	if (!pUser) return;

	Packet result;
	if (!m_sSend)
	{
		result.Initialize(WIZ_MERCHANT_INOUT);
		result << uint8(1) << uint16(1) << uint32(pUser->GetSocketID()) << pUser->GetMerchantState() << (pUser->GetMerchantState() == 1 ? false : pUser->m_bPremiumMerchant);
		pUser->SendToRegion(&result, pUser, GetEventRoom());
	}

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	uint8 m_sPremiumMetchantType = pUser->GetMerchantPremiumState() ? 1 : 0;
	result << uint8(MERCHANT_LIST) << uint8(1) << uint32(pUser->GetID()) << pUser->GetMerchantState() << m_sPremiumMetchantType;

	_MERCH_DATA pMerchantList[MAX_MERCH_ITEMS] = {};
	memset(pMerchantList, 0, sizeof(pMerchantList));
	GetMerchantSlipList(pMerchantList, pUser);

	for (int i = 0, x = m_sPremiumMetchantType == 1 ? 8 : 4; i < x; i++)
		if (!pMerchantList[i].IsSoldOut)
			result << pMerchantList[i].nNum;

	pUser->Send(&result);

	for (int i = 0, x = m_sPremiumMetchantType == 1 ? 8 : 4; i < x; i++)
	{
		if (!pMerchantList[i].nNum)
			continue;

		result.clear();
		result.Initialize(WIZ_MERCHANT);
		result << uint8(MERCHANT_ITEM_ADD) << uint16(1) << pMerchantList[i].nNum << pMerchantList[i].sCount << pMerchantList[i].sDuration << pMerchantList[i].nPrice << pMerchantList[i].bOriginalSlot << uint8(i);
		pUser->Send(&result);
	}
}

void CUser::MerchantSlipRefList(CBot* pUser, bool m_sSend)
{
	if (!pUser) return;

	Packet result;
	if (!m_sSend)
	{
		result.Initialize(WIZ_MERCHANT_INOUT);
		result << uint8(1) << uint16(1) << uint32(pUser->GetID()) << pUser->GetMerchantState() << (pUser->GetMerchantState() == 1 ? false : pUser->m_bPremiumMerchant);
		pUser->SendToRegion(&result);
	}

	result.clear();
	result.Initialize(WIZ_MERCHANT);
	uint8 m_sPremiumMetchantType = pUser->GetMerchantPremiumState() ? 1 : 0;
	result << uint8(MERCHANT_LIST) << uint8(1) << uint32(pUser->GetID()) << pUser->GetMerchantState() << m_sPremiumMetchantType;

	_MERCH_DATA pMerchantList[MAX_MERCH_ITEMS] = {};
	memset(pMerchantList, 0, sizeof(pMerchantList));
	GetMerchantSlipList(pMerchantList, pUser);

	for (int i = 0, x = m_sPremiumMetchantType == 1 ? 8 : 4; i < x; i++)
		if (!pMerchantList[i].IsSoldOut)
			result << pMerchantList[i].nNum;

	pUser->SendToRegion(&result);

	for (int i = 0, x = m_sPremiumMetchantType == 1 ? 8 : 4; i < x; i++)
	{
		if (!pMerchantList[i].nNum)
			continue;

		result.clear();
		result.Initialize(WIZ_MERCHANT);
		result << uint8(MERCHANT_ITEM_ADD) << uint16(1) << pMerchantList[i].nNum << pMerchantList[i].sCount << pMerchantList[i].sDuration << pMerchantList[i].nPrice << pMerchantList[i].bOriginalSlot << uint8(i);
		pUser->SendToRegion(&result);
	}
}