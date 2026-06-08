#include "stdafx.h"
#include "DBAgent.h"

enum class ResultOpCodes
{
	NotAccess = 0,
	OpenWarehouse = 1,
	RequiredMoney = 2,
	InvalidPassword = 3
};

void CUser::WarehouseSendError(uint8 opcode, WarehouseError e) {
	Packet result(WIZ_WAREHOUSE);
	result << opcode << uint8(e);
	Send(&result);
}

#pragma region CUser::WarehouseProcess(Packet & pkt)
void CUser::WarehouseProcess(Packet& pkt)
{
	Packet result(WIZ_WAREHOUSE);
	uint32 nItemID, nCount, m_sNpcId;
	uint16 reference_pos;
	uint8 page, bSrcPos, bDstPos;
	CNpc* pNpc = nullptr;
	_ITEM_TABLE  pTable = _ITEM_TABLE();
	_ITEM_DATA* pSrcItem = nullptr, * pDstItem = nullptr;
	uint8 opcode;
	ResultOpCodes resultOpCode = ResultOpCodes::NotAccess;

	if (pCindWar.isEventUser()
		&& g_pMain->isCindirellaZone(GetZoneID()))
		return;

	if (isDead() || !isInGame())
		return;

	if (isTrading()
		|| isStoreOpen()
		|| isMerchanting()
		|| isSellingMerchant()
		|| isBuyingMerchant()
		|| isMining()
		|| isFishing())
		goto fail_return;

	pkt >> opcode;

	if (isInGenie())
	{
		if (opcode == WAREHOUSE_OPEN)
			return WarehouseSendError(WAREHOUSE_OPEN, WarehouseError::NoAccess);
		else
			return WarehouseSendError(opcode, WarehouseError::NoAccess);
	}

	if (opcode == WAREHOUSE_OPEN)
	{
		if (isInPKZone())
		{
			if (hasCoins(10000))
				GoldLose(10000);
			else
			{
				resultOpCode = ResultOpCodes::RequiredMoney;
				goto fail_return;
			}
		}
		result << opcode << uint8(1) << GetInnCoins();
		for (int i = 0; i < WAREHOUSE_MAX; i++)
		{
			_ITEM_DATA* pItem = &m_sWarehouseArray[i];

			if (pItem == nullptr)
				continue;

			result << pItem->nNum
				<< pItem->sDuration
				<< pItem->sCount
				<< pItem->bFlag;

			_ITEM_TABLE pItemTable = g_pMain->GetItemPtr(pItem->nNum);
			if (!pItemTable.isnull())
			{
				if (pItemTable.isPetItem())
					ShowPetItemInfo(result, pItem->nSerialNum);
				else if (pItemTable.GetNum() == ITEM_CYPHER_RING)
					ShowCyperRingItemInfo(result, pItem->nSerialNum);
				else
					result << uint32(0); // Item Unique ID
			}
			else
				result << uint32(0); // Item Unique ID*/
			result << pItem->nExpirationTime;
		}
		Send(&result);
		return;
	}
	pkt >> m_sNpcId >> nItemID >> page >> bSrcPos >> bDstPos;

	pNpc = g_pMain->GetNpcPtr(m_sNpcId, GetZoneID());
	if (pNpc == nullptr
		|| pNpc->GetType() != NPC_WAREHOUSE
		|| !isInRange(pNpc, MAX_NPC_RANGE))
		goto fail_return;

	pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable.isnull())
		goto fail_return;

	reference_pos = 24 * page;

	switch (opcode)
	{
		// Inventory -> inn
	case WAREHOUSE_INPUT:
	{
		pkt >> nCount;

		if (!nCount)goto fail_return;

		// Handle coin input.
		if (nItemID == ITEM_GOLD)
		{
			if (!hasCoins(nCount)
				|| GetInnCoins() + nCount > COIN_MAX)
				goto fail_return;

			m_iBank += nCount;
			m_iGold -= nCount;
			break;
		}

		pSrcItem = GetItem(SLOT_MAX + bSrcPos);

		// Check for invalid slot IDs.
		if (!pSrcItem
			|| bSrcPos > HAVE_MAX
			|| reference_pos + bDstPos > WAREHOUSE_MAX
			// Cannot be traded, sold or stored (note: don't check the race, as these items CAN be stored).
			|| (nItemID >= ITEM_NO_TRADE_MIN && nItemID <= ITEM_NO_TRADE_MAX)
			// Check that the source item we're moving is what the client says it is.
			|| pSrcItem->nNum != nItemID
			// Rented items cannot be placed in the inn.
			|| pSrcItem->isRented()
			|| pSrcItem->isDuplicate()
			|| pSrcItem->isExpirationTime() 
			|| pSrcItem->isSealed())
			goto fail_return;

		pDstItem = &m_sWarehouseArray[reference_pos + bDstPos];
		// Forbid users from moving non-stackable items into a slot already occupied by an item.
		if ((!pTable.isStackable() && pDstItem->nNum != 0)
			// Forbid users from moving stackable items into a slot already occupied by a different item.
			|| (pTable.isStackable()
				&& pDstItem->nNum != 0 // slot in use
				&& pDstItem->nNum != pSrcItem->nNum) // ... by a different item.
			|| pSrcItem->sCount < nCount) // Ensure users have enough of the specified item to move.
			goto fail_return;

		if (pTable.isStackable())
			pDstItem->sCount += (uint16)nCount;
		else
			pDstItem->sCount = (uint16)nCount;

		if (pTable.isStackable())
			pSrcItem->sCount -= nCount;
		else
			pSrcItem->sCount = 0;

		uint64 serial = pSrcItem->nSerialNum;
		if (!serial) serial = g_pMain->GenerateItemSerial();

		if (pTable.isStackable())
		{
			if (!pSrcItem->sCount && !pDstItem->nNum)
				pDstItem->nSerialNum = serial;
			else if (!pDstItem->nNum)
				pDstItem->nSerialNum = g_pMain->GenerateItemSerial();
		}
		else pDstItem->nSerialNum = serial;

		pDstItem->sDuration = pSrcItem->sDuration;
		pDstItem->bFlag = pSrcItem->bFlag;
		pDstItem->sRemainingRentalTime = pSrcItem->sRemainingRentalTime;
		pDstItem->nExpirationTime = pSrcItem->nExpirationTime;
		pDstItem->nNum = pSrcItem->nNum;

		if (pDstItem->sCount > MAX_ITEM_COUNT)
			pDstItem->sCount = MAX_ITEM_COUNT;

		if ((!pSrcItem->sCount) || (!pTable.m_bCountable) || (pTable.m_bKind == 255 && !pTable.m_bCountable))
			memset(pSrcItem, 0, sizeof(_ITEM_DATA));

		SetUserAbility(false);
		SendItemWeight();

#if(GAME_SOURCE_VERSION == 2369)
		HandleHShieldSoftwareNewItemMoveLoadderHandler();
#endif
	}
	break;
	case WAREHOUSE_OUTPUT:
	{
		pkt >> nCount;

		if (!nCount)goto fail_return;

		if (nItemID == ITEM_GOLD)
		{
			if (!hasInnCoins(nCount)
				|| GetCoins() + nCount > COIN_MAX)
				goto fail_return;

			m_iGold += nCount;
			m_iBank -= nCount;
			break;
		}

		// Ensure we're not being given an invalid slot ID.
		if (reference_pos + bSrcPos > WAREHOUSE_MAX
			|| bDstPos > HAVE_MAX
			// Check that the source item we're moving is what the client says it is.
			|| (pSrcItem = &m_sWarehouseArray[reference_pos + bSrcPos])->nNum != nItemID
			// Does the player have enough room in their inventory?
			|| !CheckWeight(pTable, nItemID, (uint16)nCount))
			goto fail_return;

		pDstItem = GetItem(SLOT_MAX + bDstPos);
		// Forbid users from moving non-stackable items into a slot already occupied by an item.
		if ((!pTable.isStackable() && pDstItem->nNum != 0)
			// Forbid users from moving stackable items into a slot already occupied by a different item.
			|| (pTable.isStackable()
				&& pDstItem->nNum != 0 // slot in use
				&& pDstItem->nNum != pSrcItem->nNum) // ... by a different item.
			|| pSrcItem->sCount < nCount) // Ensure users have enough of the specified item to move.
			goto fail_return;

		if (pTable.isStackable())
			pDstItem->sCount += (uint16)nCount;
		else
			pDstItem->sCount = (uint16)nCount;

		if (pTable.isStackable())
			pSrcItem->sCount -= nCount;
		else
			pSrcItem->sCount = 0;

		uint64 serial = pSrcItem->nSerialNum;
		if (!serial) serial = g_pMain->GenerateItemSerial();

		if (pTable.isStackable())
		{
			if (!pSrcItem->sCount && !pDstItem->nNum)
				pDstItem->nSerialNum = serial;
			else if (!pDstItem->nNum)
				pDstItem->nSerialNum = g_pMain->GenerateItemSerial();
		}
		else pDstItem->nSerialNum = serial;

		pDstItem->sDuration = pSrcItem->sDuration;
		pDstItem->bFlag = pSrcItem->bFlag;
		pDstItem->sRemainingRentalTime = pSrcItem->sRemainingRentalTime;
		pDstItem->nExpirationTime = pSrcItem->nExpirationTime;
		pDstItem->nNum = pSrcItem->nNum;

		if (pDstItem->sCount > MAX_ITEM_COUNT)
			pDstItem->sCount = MAX_ITEM_COUNT;

		if (!pSrcItem->sCount || !pTable.m_bCountable || (pTable.m_bKind == 255 && !pTable.m_bCountable))
			memset(pSrcItem, 0, sizeof(_ITEM_DATA));

		SetUserAbility(false);
		SendItemWeight();

#if(GAME_SOURCE_VERSION == 2369)
		HandleHShieldSoftwareNewItemMoveLoadderHandler();
#endif
	}
	break;
	case WAREHOUSE_MOVE:
	{
		// Ensure we're not being given an invalid slot ID.
		if (reference_pos + bSrcPos > WAREHOUSE_MAX
			|| reference_pos + bDstPos > WAREHOUSE_MAX)
			goto fail_return;

		pSrcItem = &m_sWarehouseArray[reference_pos + bSrcPos];
		pDstItem = &m_sWarehouseArray[reference_pos + bDstPos];

		// Check that the source item we're moving is what the client says it is.
		if (pSrcItem->nNum != nItemID
			// You can't move a partial stack in the inn (the whole stack is moved).
			|| pDstItem->nNum != 0)
			goto fail_return;

		memcpy(pDstItem, pSrcItem, sizeof(_ITEM_DATA));
		memset(pSrcItem, 0, sizeof(_ITEM_DATA));
	}
	break;
	case WAREHOUSE_INVENMOVE:
	{
		// Ensure we're not being given an invalid slot ID.
		if (bSrcPos > HAVE_MAX
			|| bDstPos > HAVE_MAX)
			goto fail_return;

		pSrcItem = GetItem(SLOT_MAX + bSrcPos);
		pDstItem = GetItem(SLOT_MAX + bDstPos);

		// Check that the source item we're moving is what the client says it is.
		if (pSrcItem->nNum != nItemID
			// You can't move a partial stack in the inventory (the whole stack is moved).
			|| pDstItem->nNum != 0)
			goto fail_return;

		memcpy(pDstItem, pSrcItem, sizeof(_ITEM_DATA));
		memset(pSrcItem, 0, sizeof(_ITEM_DATA));

#if(GAME_SOURCE_VERSION == 2369)
		HandleHShieldSoftwareNewItemMoveLoadderHandler();
#endif
	}
	break;
	}

	resultOpCode = ResultOpCodes::OpenWarehouse;

fail_return:
	result << opcode << (uint8)resultOpCode;
	Send(&result);
}
#pragma endregion