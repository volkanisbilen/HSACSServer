#include "stdafx.h"

#pragma region CGameServerDlg::Send_CommandChat(Packet *pkt, int nation, CUser* pExceptUser)

void CGameServerDlg::Send_CommandChat(Packet* pkt, int nation, CUser* pExceptUser)
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		if (nation == 0 || nation == pUser->GetNation())
			pUser->Send(pkt);
	}
}
#pragma endregion

#pragma region CUser::RegionUserInOutForMe()
void CUser::RegionUserInOutForMe()
{
	C3DMap* pMap = GetMap();

	if (pMap == nullptr)
		return;

	Packet result;
	result.Initialize(WIZ_REGIONCHANGE);
	result << uint8(0);
	Send(&result);

	uint16 user_count = 0;
	result.clear();
	result.Initialize(WIZ_REGIONCHANGE);
	result << uint8(1) << uint16(0); // placeholder for the user count

	int16 rx = GetRegionX(), rz = GetRegionZ();
	foreach_region(x, z)
	{
		GetRegionUserList(pMap, rx + x, rz + z, result, user_count, GetEventRoom());
		GetRegionBotList(pMap, rx + x, rz + z, result, user_count, GetEventRoom());
	}

	result.put(1, user_count);
	SendCompressed(&result);

	result.clear();
	result.Initialize(WIZ_REGIONCHANGE);
	result << uint8(2);
	Send(&result);
}
#pragma endregion

#pragma region  CUser::GetRegionUserList(KOMap* pMap, uint16 region_x, uint16 region_z, Packet& pkt, uint16& t_count, uint16 nEventRoom)
void CUser::GetRegionUserList(KOMap* pMap, uint16 region_x, uint16 region_z, Packet& pkt, uint16& t_count, uint16 nEventRoom)
{
	if (pMap == nullptr)
		return;

	CRegion* pRegion = pMap->GetRegion(region_x, region_z);
	if (pRegion == nullptr)
		return;

	std::lock_guard<std::recursive_mutex> lock(pRegion->m_lockUserArray);
	ZoneUserArray cm_RegionUserArray = pRegion->m_RegionUserArray;
	foreach(itr, cm_RegionUserArray)
	{
		CUser* pUser = g_pMain->GetUserPtr(*itr);
		if (pUser == nullptr
			|| !pUser->isInGame()
			|| pUser->GetZoneID() != GetZoneID())
			continue;

		if (GetEventRoom() != pUser->GetEventRoom())
			continue;

		if (pUser->m_bAbnormalType == ABNORMAL_INVISIBLE)
			continue;

		pkt << uint32(pUser->GetSocketID());
		t_count++;
	}
}
#pragma endregion

#pragma region  CUser::GetRegionBotList(KOMap* pMap, uint16 region_x, uint16 region_z, Packet& pkt, uint16& t_count, uint16 nEventRoom)
void CUser::GetRegionBotList(KOMap* pMap, uint16 region_x, uint16 region_z, Packet& pkt, uint16& t_count, uint16 nEventRoom)
{
	if (pMap == nullptr)
		return;

	CRegion* pRegion = pMap->GetRegion(region_x, region_z);
	if (pRegion == nullptr)
		return;

	std::lock_guard<std::recursive_mutex> lock(pRegion->m_sBotArrayLock);
	ZoneUserArray cm_RegionUserArray = pRegion->m_sRegionBotArray;
	foreach(itr, cm_RegionUserArray)
	{
		CBot* pBot = g_pMain->GetBotPtr(*itr);
		if (pBot == nullptr)
			continue;

		if (!pBot->isInGame()
			|| pBot->GetZoneID() != GetZoneID())
			continue;

		if (pBot->LastWarpTime > 0)
		{
			if (pBot->LastWarpTime < UNIXTIME)
				continue;
		}
		
		if (pBot->m_bAbnormalType == ABNORMAL_INVISIBLE)
			continue;

		pkt << uint32(pBot->GetID());
		t_count++;
	}
}
#pragma endregion

#pragma region CUser::MerchantUserInOutForMe()
void CUser::MerchantUserInOutForMe()
{
	Packet result;
	C3DMap* pMap = GetMap();
	if (pMap == nullptr)
		return;

	uint16 user_count = 0;
	result.clear();
	result.Initialize(WIZ_MERCHANT_INOUT);
	result << uint8(1) << uint16(0);

	int16 rx = GetRegionX(), rz = GetRegionZ();
	foreach_region(x, z)
	{
		GetRegionUserMerchantUserIn(pMap, rx + x, rz + z, result, user_count, GetEventRoom());
		GetRegionBotMerchantUserIn(pMap, rx + x, rz + z, result, user_count, GetEventRoom());
	}

	result.put(1, uint16(user_count));
	SendCompressed(&result);
}
#pragma endregion

#pragma region  CUser::GetRegionUserMerchantUserIn(KOMap *pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count, uint16 nEventRoom)
void CUser::GetRegionUserMerchantUserIn(KOMap* pMap, uint16 region_x, uint16 region_z, Packet& pkt, uint16& t_count, uint16 nEventRoom)
{
	if (pMap == nullptr)
		return;

	CRegion* pRegion = pMap->GetRegion(region_x, region_z);
	if (pRegion == nullptr)
		return;

	std::lock_guard<std::recursive_mutex> lock(pRegion->m_lockUserArray);
	ZoneUserArray cm_RegionUserArray = pRegion->m_RegionUserArray;
	foreach(itr, cm_RegionUserArray)
	{
		CUser* pUser = g_pMain->GetUserPtr(*itr);
		if (pUser == nullptr
			|| !pUser->isInGame()
			|| !pUser->isMerchanting())
			continue;

		if (nEventRoom > 0
			&& nEventRoom != pUser->GetEventRoom())
			continue;

		if (pUser->m_bAbnormalType == ABNORMAL_INVISIBLE)
			continue;

		pkt << uint32(pUser->GetSocketID())
			<< pUser->GetMerchantState() // 0 is selling, 1 is buying
			<< (pUser->GetMerchantState() == 1 ? false : pUser->m_bPremiumMerchant); // Type of merchant [normal - gold] // bool

		t_count++;
	}
}
#pragma endregion

#pragma region  CUser::GetRegionBotMerchantUserIn(KOMap *pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count, uint16 nEventRoom)
void CUser::GetRegionBotMerchantUserIn(KOMap* pMap, uint16 region_x, uint16 region_z, Packet& pkt, uint16& t_count, uint16 nEventRoom)
{
	if (pMap == nullptr)
		return;

	CRegion* pRegion = pMap->GetRegion(region_x, region_z);
	if (pRegion == nullptr)
		return;

	std::lock_guard<std::recursive_mutex> lock(pRegion->m_sBotArrayLock);
	ZoneBotArray cm_sRegionBotArray = pRegion->m_sRegionBotArray;
	foreach(itr, cm_sRegionBotArray)
	{
		CBot* pBot = g_pMain->GetBotPtr(*itr);
		if (pBot == nullptr)
			continue;

		if (!pBot->isInGame()
			|| !pBot->isMerchanting())
			continue;

		if (pBot->LastWarpTime > 0)
		{
			if (pBot->LastWarpTime < UNIXTIME)
				continue;
		}

		pkt << uint32(pBot->GetID())
			<< pBot->GetMerchantState() // 0 is selling, 1 is buying
			<< (pBot->GetMerchantState() == 1 ? false : pBot->m_bPremiumMerchant); // Type of merchant [normal - gold] // bool

		t_count++;
	}
}
#pragma endregion

#pragma region CUser::RegionNpcInfoForMe()
void CUser::RegionNpcInfoForMe()
{
	Packet bResult;
	KOMap* pMap = GetMap();
	if (pMap == nullptr)
		return;

	uint16 npc_count = 0;
	bResult.clear();
	bResult.Initialize(WIZ_NPC_REGION);
	bResult << uint16(0);

	int16 rx = GetRegionX(), rz = GetRegionZ();
	foreach_region(x, z)
		GetRegionNpcList(pMap, rx + x, rz + z, bResult, npc_count, GetEventRoom());

	bResult.put(0, uint16(npc_count));
	SendCompressed(&bResult);
}
#pragma endregion

#pragma region CUser::GetRegionNpcList(C3DMap *pMap, uint16 region_x, uint16 region_z, Packet & pkt, uint16 & t_count, uint16 nEventRoom)
void CUser::GetRegionNpcList(C3DMap* pMap, uint16 region_x, uint16 region_z, Packet& pkt, uint16& t_count, uint16 nEventRoom)
{
	if (pMap == nullptr)
		return;

	CRegion* pRegion = pMap->GetRegion(region_x, region_z);
	if (pRegion == nullptr)
		return;

	std::lock_guard<std::recursive_mutex> lock(pRegion->m_lockNpcArray);
	ZoneNpcArray cm_RegionNpcArray = pRegion->m_RegionNpcArray;
	foreach(itr, cm_RegionNpcArray)
	{
		CNpc* pNpc = g_pMain->GetNpcPtr(*itr, pMap->GetID());
		if (pNpc == nullptr
			|| pNpc->isDead())
			continue;

		if (nEventRoom > 0 && nEventRoom != pNpc->GetEventRoom()
			&& pNpc->GetProtoID() != SAW_BLADE_SSID)
			continue;

		pkt << uint32(pNpc->GetID());
		t_count++;
	}
}
#pragma endregion

#pragma region CGameServerDlg::GetUnitListFromSurroundingRegions(Unit * pOwner, std::vector<uint16> * pList)

void CGameServerDlg::GetUnitListFromSurroundingRegions(Unit* pOwner, std::vector<uint16>* pList)
{
	if (pOwner == nullptr)
		return;

	C3DMap* pMap = pOwner->GetMap();
	if (pMap == nullptr)
		return;

	int16 rx = pOwner->GetRegionX(), rz = pOwner->GetRegionZ();

	foreach_region(x, z)
	{
		uint16 region_x = rx + x, region_z = rz + z;
		CRegion* pRegion = pMap->GetRegion(region_x, region_z);
		if (pRegion == nullptr)
			continue;

		pRegion->m_lockNpcArray.lock();
		if (pRegion->m_RegionNpcArray.size() <= 0)
		{
			pRegion->m_lockNpcArray.unlock();
			continue;
		}

		ZoneNpcArray cm_RegionNpcArray = pRegion->m_RegionNpcArray;
		pRegion->m_lockNpcArray.unlock();
		foreach(itr, cm_RegionNpcArray)
		{
			CNpc* pNpc = GetNpcPtr(*itr, pMap->GetID());
			if (pNpc == nullptr || pNpc->isDead())
				continue;

			if (pOwner->GetEventRoom() >= 0 && pOwner->GetEventRoom() != pNpc->GetEventRoom())
				continue;

			pList->push_back(*itr);
		}
	}

	foreach_region(x, z)
	{
		uint16 region_x = rx + x, region_z = rz + z;
		CRegion* pRegion = pMap->GetRegion(region_x, region_z);
		if (pRegion == nullptr)
			continue;

		// Add all potential users to list
		pRegion->m_lockUserArray.lock();
		if (pRegion->m_RegionUserArray.size() <= 0)
		{
			pRegion->m_lockUserArray.unlock();
			continue;
		}

		ZoneUserArray cm_RegionUserArray = pRegion->m_RegionUserArray;
		pRegion->m_lockUserArray.unlock();
		foreach(itr, cm_RegionUserArray)
		{
			CUser* pUser = GetUserPtr(*itr);
			if (pUser == nullptr
				|| !pUser->isInGame())
				continue;

			if (pOwner->GetEventRoom() >= 0 && pOwner->GetEventRoom() != pUser->GetEventRoom())
				continue;

			pList->push_back(*itr);

		}
	}

	foreach_region(x, z)
	{
		uint16 region_x = rx + x, region_z = rz + z;
		CRegion* pRegion = pMap->GetRegion(region_x, region_z);
		if (pRegion == nullptr)
			continue;

		// Add all potential users to list
		pRegion->m_sBotArrayLock.lock();
		if (pRegion->m_sRegionBotArray.size() <= 0)
		{
			pRegion->m_sBotArrayLock.unlock();
			continue;
		}

		ZoneUserArray cm_sRegionBotArray = pRegion->m_sRegionBotArray;
		pRegion->m_sBotArrayLock.unlock();
		foreach(itr, cm_sRegionBotArray)
		{
			CBot* pBot = GetBotPtr(*itr);
			if (pBot == nullptr
				|| !pBot->isInGame()
				|| pBot->isDead())
				continue;

			if (pOwner->GetEventRoom() >= 0 && pOwner->GetEventRoom() != pBot->GetEventRoom())
				continue;

			pList->push_back(*itr);
		}
	}
}

#pragma endregion

#pragma region CGameServerDlg::Send_Zone_Matched_Class(Packet *pkt, uint8 bZoneID, CUser* pExceptUser, uint8 nation, uint8 seekingPartyOptions, uint16 nEventRoom)

/**
* @brief	Sends a packet to all users in the zone matching the specified class types.
*
* @param	pkt				   	The packet.
* @param	bZoneID			   	Identifier for the zone.
* @param	pExceptUser		   	The except user.
* @param	nation			   	The nation.
* @param	seekingPartyOptions	Bitmask of classes to send to.
*/
void CGameServerDlg::Send_Zone_Matched_Class(Packet* pkt, uint8 bZoneID, CUser* pExceptUser, uint8 nation, uint8 seekingPartyOptions, uint16 nEventRoom)
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr)
			continue;

		if (pUser == pExceptUser
			|| !pUser->isInGame()
			|| pUser->GetZoneID() != bZoneID
			|| pUser->isInParty()) // looking for users to join the party
			continue;

		if (nEventRoom >= 0 && nEventRoom != pUser->GetEventRoom())
			continue;

		// If we're in the neutral zone (Moradon), it doesn't matter which nation we party with.
		// For all other zones, we must party with a player of the same nation.
		if (pUser->isInMoradon()
			|| pUser->GetNation() == nation)
		{
			if (((seekingPartyOptions & 1) && pUser->JobGroupCheck((short)ClassType::ClassWarrior))
				|| ((seekingPartyOptions & 2) && pUser->JobGroupCheck((short)ClassType::ClassRogue))
				|| ((seekingPartyOptions & 4) && pUser->JobGroupCheck((short)ClassType::ClassMage))
				|| ((seekingPartyOptions & 8) && pUser->JobGroupCheck((short)ClassType::ClassPriest))
				|| ((seekingPartyOptions & 10) && pUser->JobGroupCheck((short)ClassType::ClassPortuKurian)))
				pUser->Send(pkt);
		}
	}
}

#pragma endregion

#pragma region CGameServerDlg::Send_Zone(Packet *pkt, uint8 bZoneID, CUser* pExceptUser /*= nullptr*/, uint8 nation /*= 0*/, uint16 nEventRoom /*= 0*/, float fRange /*= 0.0f*/)

void CGameServerDlg::Send_Zone(Packet* pkt, uint8 bZoneID, CUser* pExceptUser /*= nullptr*/, uint8 nation /*= 0*/, uint16 nEventRoom /*= 0*/, float fRange /*= 0.0f*/)
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr)
			continue;

		if (!pUser->isInGame()
			|| pUser->GetZoneID() != bZoneID
			|| (nation != (uint8)Nation::ALL && nation != pUser->GetNation()))
		{
			if (pExceptUser != nullptr)
			{
				if (pUser == pExceptUser
					|| (fRange > 0.0f && pUser->isInRange(pExceptUser, fRange)))
					continue;
			}
			continue;
		}

		if (nEventRoom >= 0 && nEventRoom != pUser->GetEventRoom())
			continue;

		pUser->Send(pkt);
	}
}

#pragma endregion

void CGameServerDlg::Send_Merchant(Packet* pkt, uint8 bZoneID, CUser* pExceptUser /*= nullptr*/, uint8 nation /*= 0*/, uint16 nEventRoom /*= 0*/, float fRange /*= 0.0f*/)
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);

		if (pUser == nullptr)
			continue;

		if (!pUser->isInGame() || pUser->GetZoneID() != bZoneID || (nation != (uint8)Nation::ALL && nation != pUser->GetNation()))
		{
			if (pExceptUser != nullptr)
			{
				if (pUser == pExceptUser)
					continue;
			}
			continue;
		}

		pUser->Send(pkt);
	}
}


#pragma region CGameServerDlg::Send_GM

void CGameServerDlg::Send_GM(Packet* pkt)
{
	foreach(itr, m_GMNameMap)
	{
		CUser* pUser = TO_USER(itr->second);
		if (pUser == nullptr
			|| !pUser->isInGame()
			|| !pUser->isGM())
			continue;

		pUser->Send(pkt);
	}
}
#pragma endregion
// end

#pragma region CGameServerDlg::Send_All(Packet *pkt, CUser* pExceptUser /*= nullptr*/, uint8 nation /*= 0*/,uint8 ZoneID /*= 0*/, bool isSendEventUsers /* false */, uint16 nEventRoom /*= -1*/)
void CGameServerDlg::Send_All(Packet* pkt, CUser* pExceptUser /*= nullptr*/, uint8 nation /*= 0*/,
	uint8 ZoneID /*= 0*/, bool isSendEventUsers /* false */, uint16 nEventRoom /*= -1*/)
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr
			|| pUser == pExceptUser
			|| !pUser->isInGame())
			continue;

		if ((nation != (uint8)Nation::ALL && nation != pUser->GetNation()))
			continue;

		if (ZoneID != 0 && pUser->GetZoneID() != ZoneID)
			continue;

		if (nEventRoom && nEventRoom != pUser->GetEventRoom())
			continue;

		if (isSendEventUsers && !pUser->isEventUser())
			continue;

		pUser->Send(pkt);
	}
}
#pragma endregion

#pragma region  CGameServerDlg::Send_Region(Packet *pkt, C3DMap *pMap, int x, int z, CUser* pExceptUser, uint16 nEventRoom)

void CGameServerDlg::Send_Region(Packet* pkt, C3DMap* pMap, int x, int z, CUser* pExceptUser, uint16 nEventRoom)
{
	foreach_region(rx, rz)
		Send_UnitRegion(pkt, pMap, rx + x, rz + z, pExceptUser, nEventRoom);
}

#pragma endregion

void CGameServerDlg::SendNoticeWindAll(std::string notice, uint32 color, uint8 ZoneID)
{
	Packet result(WIZ_ADD_MSG, uint8(2));
	result << notice << color << color;

	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame() || pUser && ZoneID > 0 && ZoneID != pUser->GetZoneID())
			continue;

		pUser->Send(&result);
	}
}

#pragma region CGameServerDlg::Send_UnitRegion(Packet *pkt, C3DMap *pMap, int x, int z, CUser *pExceptUser, uint16 nEventRoom)

void CGameServerDlg::Send_UnitRegion(Packet* pkt, C3DMap* pMap, int x, int z, CUser* pExceptUser, uint16 nEventRoom)
{
	if (pMap == nullptr || pMap->m_nZoneNumber == 0)
		return;

	CRegion* pRegion = pMap->GetRegion(x, z);

	if (pRegion == nullptr)
		return;

	pRegion->m_lockUserArray.lock();

	if (pRegion->m_RegionUserArray.size() <= 0)
	{
		pRegion->m_lockUserArray.unlock();
		return;
	}

	ZoneUserArray cm_RegionUserArray = pRegion->m_RegionUserArray;
	pRegion->m_lockUserArray.unlock();
	foreach(itr, cm_RegionUserArray)
	{
		if (cm_RegionUserArray.size() <= 0)
		{
			printf("cm_RegionUserArray [10] size = 0\n");
			break;
		}
		CUser* pUser = GetUserPtr(*itr);
		if (pUser == nullptr
			|| pUser == pExceptUser
			|| !pUser->isInGame())
			continue;

		if (nEventRoom >= 0 && nEventRoom != pUser->GetEventRoom())
			continue;

		pUser->Send(pkt);
	}
}

#pragma endregion

#pragma region CGameServerDlg::Send_OldRegions(Packet *pkt, int old_x, int old_z, C3DMap *pMap, int x, int z, CUser* pExceptUser, uint16 nEventRoom)

// TODO: Move the following two methods into a base CUser/CNpc class
void CGameServerDlg::Send_OldRegions(Packet* pkt, int old_x, int old_z, C3DMap* pMap, int x, int z, CUser* pExceptUser, uint16 nEventRoom)
{
	if (old_x != 0)
	{
		Send_UnitRegion(pkt, pMap, x + old_x * 2, z + old_z - 1, pExceptUser, nEventRoom);
		Send_UnitRegion(pkt, pMap, x + old_x * 2, z + old_z, pExceptUser, nEventRoom);
		Send_UnitRegion(pkt, pMap, x + old_x * 2, z + old_z + 1, pExceptUser, nEventRoom);
	}

	if (old_z != 0)
	{
		Send_UnitRegion(pkt, pMap, x + old_x, z + old_z * 2, pExceptUser, nEventRoom);
		if (old_x < 0)
			Send_UnitRegion(pkt, pMap, x + old_x + 1, z + old_z * 2, pExceptUser, nEventRoom);
		else if (old_x > 0)
			Send_UnitRegion(pkt, pMap, x + old_x - 1, z + old_z * 2, pExceptUser, nEventRoom);
		else
		{
			Send_UnitRegion(pkt, pMap, x + old_x - 1, z + old_z * 2, pExceptUser, nEventRoom);
			Send_UnitRegion(pkt, pMap, x + old_x + 1, z + old_z * 2, pExceptUser, nEventRoom);
		}
	}
}

#pragma endregion

#pragma region CGameServerDlg::Send_NewRegions(Packet *pkt, int new_x, int new_z, C3DMap *pMap, int x, int z, CUser* pExceptUser, uint16 nEventRoom)

void CGameServerDlg::Send_NewRegions(Packet* pkt, int new_x, int new_z, C3DMap* pMap, int x, int z, CUser* pExceptUser, uint16 nEventRoom)
{
	if (new_x != 0)
	{
		Send_UnitRegion(pkt, pMap, x + new_x, z - 1, pExceptUser, nEventRoom);
		Send_UnitRegion(pkt, pMap, x + new_x, z, pExceptUser, nEventRoom);
		Send_UnitRegion(pkt, pMap, x + new_x, z + 1, pExceptUser, nEventRoom);
	}

	if (new_z != 0)
	{
		Send_UnitRegion(pkt, pMap, x, z + new_z, pExceptUser, nEventRoom);

		if (new_x < 0)
			Send_UnitRegion(pkt, pMap, x + 1, z + new_z, pExceptUser, nEventRoom);
		else if (new_x > 0)
			Send_UnitRegion(pkt, pMap, x - 1, z + new_z, pExceptUser, nEventRoom);
		else
		{
			Send_UnitRegion(pkt, pMap, x - 1, z + new_z, pExceptUser, nEventRoom);
			Send_UnitRegion(pkt, pMap, x + 1, z + new_z, pExceptUser, nEventRoom);
		}
	}
}

#pragma endregion

#pragma region CGameServerDlg::Send_NearRegion(Packet *pkt, C3DMap *pMap, int region_x, int region_z, float curx, float curz, CUser* pExceptUser, uint16 nEventRoom)

void CGameServerDlg::Send_NearRegion(Packet* pkt, C3DMap* pMap, int region_x, int region_z, float curx, float curz, CUser* pExceptUser, uint16 nEventRoom)
{
	int left_border = region_x * VIEW_DISTANCE, top_border = region_z * VIEW_DISTANCE;
	Send_FilterUnitRegion(pkt, pMap, region_x, region_z, curx, curz, pExceptUser, nEventRoom);
	if (((curx - left_border) > (VIEW_DISTANCE / 2.0f)))
	{			// RIGHT
		if (((curz - top_border) > (VIEW_DISTANCE / 2.0f)))
		{	// BOTTOM
			Send_FilterUnitRegion(pkt, pMap, region_x + 1, region_z, curx, curz, pExceptUser, nEventRoom);
			Send_FilterUnitRegion(pkt, pMap, region_x, region_z + 1, curx, curz, pExceptUser, nEventRoom);
			Send_FilterUnitRegion(pkt, pMap, region_x + 1, region_z + 1, curx, curz, pExceptUser, nEventRoom);
		}
		else
		{													// TOP
			Send_FilterUnitRegion(pkt, pMap, region_x + 1, region_z, curx, curz, pExceptUser, nEventRoom);
			Send_FilterUnitRegion(pkt, pMap, region_x, region_z - 1, curx, curz, pExceptUser, nEventRoom);
			Send_FilterUnitRegion(pkt, pMap, region_x + 1, region_z - 1, curx, curz, pExceptUser, nEventRoom);
		}
	}
	else
	{														// LEFT
		if (((curz - top_border) > (VIEW_DISTANCE / 2.0f)))
		{	// BOTTOM
			Send_FilterUnitRegion(pkt, pMap, region_x - 1, region_z, curx, curz, pExceptUser, nEventRoom);
			Send_FilterUnitRegion(pkt, pMap, region_x, region_z + 1, curx, curz, pExceptUser, nEventRoom);
			Send_FilterUnitRegion(pkt, pMap, region_x - 1, region_z + 1, curx, curz, pExceptUser, nEventRoom);
		}
		else
		{													// TOP
			Send_FilterUnitRegion(pkt, pMap, region_x - 1, region_z, curx, curz, pExceptUser, nEventRoom);
			Send_FilterUnitRegion(pkt, pMap, region_x, region_z - 1, curx, curz, pExceptUser, nEventRoom);
			Send_FilterUnitRegion(pkt, pMap, region_x - 1, region_z - 1, curx, curz, pExceptUser, nEventRoom);
		}
	}
}

#pragma endregion

#pragma region CGameServerDlg::Send_FilterUnitRegion(Packet *pkt, C3DMap *pMap, int x, int z, float ref_x, float ref_z, CUser *pExceptUser, uint16 nEventRoom)

void CGameServerDlg::Send_FilterUnitRegion(Packet* pkt, C3DMap* pMap, int x, int z, float ref_x, float ref_z, CUser* pExceptUser, uint16 nEventRoom)
{
	if (pMap == nullptr || pMap->m_nZoneNumber == 0)
		return;

	CRegion* pRegion = pMap->GetRegion(x, z);
	if (pRegion == nullptr)
		return;

	pRegion->m_lockUserArray.lock();

	if (pRegion->m_RegionUserArray.size() <= 0)
	{
		pRegion->m_lockUserArray.unlock();
		return;
	}

	ZoneUserArray cm_RegionUserArray = pRegion->m_RegionUserArray;
	pRegion->m_lockUserArray.unlock();
	foreach(itr, cm_RegionUserArray)
	{
		CUser* pUser = GetUserPtr(*itr);
		if (pUser == nullptr
			|| pUser == pExceptUser
			|| !pUser->isInGame())
			continue;

		if (nEventRoom >= 0 && nEventRoom != pUser->GetEventRoom())
			continue;

		if (sqrt(pow((pUser->m_curx - ref_x), 2) + pow((pUser->m_curz - ref_z), 2)) < 32)
			pUser->Send(pkt);
	}
}

#pragma endregion

#pragma region CGameServerDlg::Send_PartyMember(int party, Packet *result)

void CGameServerDlg::Send_PartyMember(int party, Packet* result)
{
	_PARTY_GROUP* pParty = GetPartyPtr(party);
	if (pParty == nullptr)
		return;

	for (int i = 0; i < MAX_PARTY_USERS; i++)
	{
		CUser* pUser = GetUserPtr(pParty->uid[i]);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		pUser->Send(result);
	}
}

#pragma endregion

#pragma region CGameServerDlg::Send_KnightsMember(int index, Packet *pkt)

void CGameServerDlg::Send_KnightsMember(int index, Packet* pkt)
{
	CKnights* pKnights = GetClanPtr(index);
	if (pKnights == nullptr)
		return;

	pKnights->Send(pkt);
}

#pragma endregion

#pragma region CGameServerDlg::Send_KnightsAlliance(uint16 sAllianceID, Packet *pkt)

void CGameServerDlg::Send_KnightsAlliance(uint16 sAllianceID, Packet* pkt)
{
	_KNIGHTS_ALLIANCE* pAlliance = GetAlliancePtr(sAllianceID);
	if (pAlliance == nullptr)
		return;

	Send_KnightsMember(pAlliance->sMainAllianceKnights, pkt);
	Send_KnightsMember(pAlliance->sSubAllianceKnights, pkt);
	Send_KnightsMember(pAlliance->sMercenaryClan_1, pkt);
	Send_KnightsMember(pAlliance->sMercenaryClan_2, pkt);
}
#pragma endregion

#pragma region CGameServerDlg::Send_Noah_Knights(Packet *pkt)
/**
* @brief Sends Noah Chat Packets to all Noah Knights.
*/
void CGameServerDlg::Send_Noah_Knights(Packet* pkt)
{
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (pUser == nullptr
			|| !pUser->isInGame()
			|| pUser->GetLevel() > 50)
			continue;

		pUser->Send(pkt);
	}
}
#pragma endregion