#include "stdafx.h"
#include "DBAgent.h"

void CUser::WheelOfFun(Packet& pkt)
{
	if (!isInGame() || isMerchanting() || isTrading())
		return;

	//if (m_nKnightCash < 80)
	if (m_nKnightCash < 350)
	{
		g_pMain->SendHelpDescription(this, "Not enought KC for wheel of fun.");
		return;
	}

	int offset = 0;
	uint32 bRandArray[10000];
	memset(bRandArray, 0, sizeof(bRandArray));
	uint8 sItemSlot = 0;

	g_pMain->m_ItemWheelArray.m_lock.lock();
	auto m_sItemWheelArray = g_pMain->m_ItemWheelArray.m_UserTypeMap;
	g_pMain->m_ItemWheelArray.m_lock.unlock();

	foreach(itr, m_sItemWheelArray)
	{
		_WHEEL_DATA* pData = itr->second;
		if (!pData)
			continue;

		if (offset >= 9999) 
			break;

		for (int i = 0; i < int(pData->ExchangeRate / 5); i++)
		{
			if (i + offset >= 9999) 
				break;
			bRandArray[offset + i] = pData->nIndex;
		}
		offset += int(pData->ExchangeRate / 5);
	}

	if (offset > 9999) offset = 9999;

	uint32 bRandSlot = myrand(0, offset);
	uint32 nIndex = bRandArray[bRandSlot];

	auto* pData = g_pMain->m_ItemWheelArray.GetData(nIndex);
	if (!pData)
		return;

	uint32 nItemID = pData->nItemID, nCountID = pData->nItemCount;

	uint8 bFreeSlots = 0;
	for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
	{
		if (GetItem(i)->nNum == 0 && ++bFreeSlots >= ITEMS_IN_EXCHANGE_GROUP)
			break;
	}

	if (bFreeSlots <= 1)
		return g_pMain->SendHelpDescription(this, "Wheel Of Fun : Uzerinizde yer yok");

	//if (!CashLose(80)) 
	if (!CashLose(350))
		return g_pMain->SendHelpDescription(this, "Wheel Of Fun : error 1");

	if (!GiveItem(std::string("Wheel Of Fun Item"), nItemID, nCountID))
		return g_pMain->SendHelpDescription(this, "Wheel Of Fun : error 2");

	HSACSX_ItemNotice(3, nItemID);
}

#pragma region CUser::SendWheelData()
void CUser::SendWheelData() 
{
	g_pMain->m_ItemWheelArray.m_lock.lock();
	auto m_sItemWheelArray = g_pMain->m_ItemWheelArray.m_UserTypeMap;
	g_pMain->m_ItemWheelArray.m_lock.unlock();

	if (m_sItemWheelArray.size() == 0) 
		return;

	if (m_sItemWheelArray.size() > 25)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::WheelData));
	result << uint16(m_sItemWheelArray.size());

	foreach(itr, m_sItemWheelArray)
		result << itr->second->nItemID;

	Send(&result);
#endif
}
#pragma endregion