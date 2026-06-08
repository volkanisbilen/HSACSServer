#include "stdafx.h"

void CUser::isEventSoccerMember(uint8 TeamColours, float x, float z)
{
	bool bIsNeutralZone = (GetZoneID() >= ZONE_MORADON && GetZoneID() <= ZONE_MORADON5);

	_SOCCER_STATUS_INFO* pRoomInfo = g_pMain->m_TempleSoccerEventRoomList.GetData(GetZoneID());
	if (pRoomInfo == nullptr)
		return;

	if (pRoomInfo->m_SoccerRedColour > 11
		&& TeamColours == TeamColourRed)
		return;

	if (pRoomInfo->m_SoccerBlueColour > 11
		&& TeamColours == TeamColourBlue)
		return;

	if (TeamColours > TeamColourRed
		|| TeamColours < TeamColourBlue)
		return;

	if (!bIsNeutralZone)
		return;

	if (pRoomInfo->GetRoomTotalUserCount() > 22)
		return;

	_SOCCER_STARTED_EVENT_USER pEventUser;
	pEventUser.strUserID = GetName();
	pEventUser.isPrizeGiven = false;
	pEventUser.isLoqOut = false;

	pRoomInfo->SoccerUserListLock.lock();
	auto pInfo = pRoomInfo->m_SeccerUserList.find(pEventUser.strUserID);
	if (pInfo != pRoomInfo->m_SeccerUserList.end())
	{
		pRoomInfo->SoccerUserListLock.unlock();
		return;
	}
	else
	{
		pRoomInfo->m_SeccerUserList.insert(std::make_pair(pEventUser.strUserID, pEventUser));

		if (TeamColours == TeamColourBlue)
			pRoomInfo->m_SoccerBlueColour++;

		if (TeamColours == TeamColourRed)
			pRoomInfo->m_SoccerRedColour++;

		Packet result(WIZ_MINING);
		result << uint8(16) << uint8(2) << pRoomInfo->m_SoccerTime;
		Send(&result);

		if (x == 0.0f && z == 0.0f)
		{
			if (TeamColours == TeamColourBlue)
				x = 672.0f, z = 166.0f;
			else
				x = 672.0f, z = 154.0f;
		}

		ZoneChange(GetZoneID(), x, z);
		StateChangeServerDirect(11, uint32(TeamColours));
	}
	pRoomInfo->SoccerUserListLock.unlock();
}

void CUser::isEventSoccerStard()
{
	_SOCCER_STATUS_INFO* pRoomInfo = g_pMain->m_TempleSoccerEventRoomList.GetData(GetZoneID());
	if (pRoomInfo == nullptr)
		return;

	if (pRoomInfo->isSoccerAktive())
		return;

	if (!isSoccerEventUser())
		return;

	if (pRoomInfo->m_SoccerBlueColour == TeamColourNone)
		return;

	if (pRoomInfo->m_SoccerRedColour == TeamColourNone)
		return;

	if (pRoomInfo->m_SoccerBlueColour > TeamColourNone
		&& pRoomInfo->m_SoccerRedColour > TeamColourNone)
	{
		pRoomInfo->m_SoccerTime = 600;
		pRoomInfo->m_SoccerActive = true;
	}
}

void CUser::isEventSoccerEnd()
{
	if (!isSoccerEventUser())
		return;

	uint8 nWinnerTeam = TeamColourNone;
	Packet result(WIZ_MINING, uint8(16));

	_SOCCER_STATUS_INFO* pRoomInfo = g_pMain->m_TempleSoccerEventRoomList.GetData(GetZoneID());
	if (pRoomInfo == nullptr)
		return;

	if (pRoomInfo->m_SoccerRedGool > pRoomInfo->m_SoccerBlueGool)
		nWinnerTeam = TeamColourRed;
	else if (pRoomInfo->m_SoccerRedGool < pRoomInfo->m_SoccerBlueGool)
		nWinnerTeam = TeamColourBlue;
	else
		nWinnerTeam = TeamColourNone;

	result << uint8(4) << nWinnerTeam << pRoomInfo->m_SoccerBlueGool << pRoomInfo->m_SoccerRedGool;

	if (pRoomInfo->m_SoccerBlueColour > TeamColourNone
		&& m_teamColour == TeamColourBlue)
		pRoomInfo->m_SoccerBlueColour--;

	if (pRoomInfo->m_SoccerRedColour > TeamColourNone
		&& m_teamColour == TeamColourRed)
		pRoomInfo->m_SoccerRedColour--;

	StateChangeServerDirect(11, TeamColourNone);
	Send(&result);
}

bool CUser::isSoccerEventUser()
{
	_SOCCER_STATUS_INFO* pRoomInfo = g_pMain->m_TempleSoccerEventRoomList.GetData(GetZoneID());
	if (pRoomInfo == nullptr)
		return false;

	pRoomInfo->SoccerUserListLock.lock();
	auto pInfo = pRoomInfo->m_SeccerUserList.find(GetName());
	if (pInfo != pRoomInfo->m_SeccerUserList.end())
	{
		pRoomInfo->SoccerUserListLock.unlock();
		return true;
	}
	pRoomInfo->SoccerUserListLock.unlock();
	return false;
}

void CUser::isEventSoccerUserRemoved()
{
	_SOCCER_STATUS_INFO* pRoomInfo = g_pMain->m_TempleSoccerEventRoomList.GetData(GetZoneID());
	if (pRoomInfo == nullptr)
		return;

	pRoomInfo->SoccerUserListLock.lock();
	pRoomInfo->m_SeccerUserList.erase(GetName());
	pRoomInfo->SoccerUserListLock.unlock();
}

bool CUser::isInSoccerEvent()
{
	bool bIsNeutralZone = (GetZoneID() >= ZONE_MORADON && GetZoneID() <= ZONE_MORADON5);

	if (!bIsNeutralZone)
		return false;

	if (!isSoccerEventUser())
		return false;

	return ((GetX() > 644.0f && GetX() < 699.0f)
		&& ((GetZ() > 120.0f && GetZ() < 200.0f)));
}

uint8 CNpc::isInSoccerEvent()
{
	bool bIsNeutralZone = (GetZoneID() >= ZONE_MORADON && GetZoneID() <= ZONE_MORADON5);

	bool b_isSoccerOutside = ((GetX() > 644.0f && GetX() < 699.0f) && ((GetZ() > 120.0f && GetZ() < 200.0f)));

	bool b_isSoccerRedside = ((GetX() > 661.0f && GetX() < 681.0f) && ((GetZ() > 108.0f && GetZ() < 120.0f)));

	bool b_isSoccerBlueside = ((GetX() > 661.0f && GetX() < 681.0f) && ((GetZ() > 199.0f && GetZ() < 208.0f)));

	if (!bIsNeutralZone)
		return TeamColourMap;

	if (b_isSoccerBlueside)
		return TeamColourBlue;

	if (b_isSoccerRedside)
		return TeamColourRed;

	if (!b_isSoccerOutside)
		return TeamColourOutside;

	return TeamColourNone;
}

#pragma region CGameServerDlg::TempleSoccerEventTimer()

void CGameServerDlg::TempleSoccerEventTimer()
{
	for (int i = ZONE_MORADON; i <= ZONE_MORADON5; i++)
	{
		_SOCCER_STATUS_INFO* pRoomInfo = g_pMain->m_TempleSoccerEventRoomList.GetData(i);
		if (pRoomInfo == nullptr)
			continue;

		if (pRoomInfo->isSoccerAktive())
		{
			if (pRoomInfo->m_SoccerTime == 600)
			{
				Packet result(WIZ_MINING, uint8(0x10));
				pRoomInfo->SoccerUserListLock.lock();
				auto sUserList = pRoomInfo->m_SeccerUserList;
				pRoomInfo->SoccerUserListLock.unlock();

				foreach(itr, sUserList)
				{
					auto& pEventUser = itr->second;
					if (pEventUser.strUserID.empty()
						|| pEventUser.isLoqOut)
						continue;

					CUser* pUser = g_pMain->GetUserPtr(pEventUser.strUserID, NameType::TYPE_CHARACTER);
					if (pUser == nullptr
						|| !pUser->isInGame()
						|| !pUser->isSoccerEventUser())
						continue;

					result << uint8(0x02) << pRoomInfo->m_SoccerTime;
					pUser->Send(&result);
				}
			}
			else if (pRoomInfo->m_SoccerTime > 1 && pRoomInfo->m_SoccerTime < 600)
			{
				CNpc* pNpc = g_pMain->GetNpcPtr(pRoomInfo->m_sNpcID, i);
				if (pNpc == nullptr
					|| pNpc->isDead())
					continue;

				if (pNpc->isInSoccerEvent() >= TeamColourBlue
					&& pNpc->isInSoccerEvent() <= TeamColourOutside)
				{
					if (pNpc->isInSoccerEvent() == TeamColourRed
						|| pNpc->isInSoccerEvent() == TeamColourBlue)
					{
						if (pNpc->isInSoccerEvent() == TeamColourRed)
							pRoomInfo->m_SoccerBlueGool++;

						if (pNpc->isInSoccerEvent() == TeamColourBlue)
							pRoomInfo->m_SoccerRedGool++;

						TempleSoccerEventGool(pNpc->isInSoccerEvent(), pNpc->GetZoneID());
					}

					float x, z;
					pNpc->SendInOut(INOUT_OUT, pNpc->GetX(), pNpc->GetZ(), 0.0f);
					x = 672.0f; z = 160.0f;
					pNpc->SendInOut(INOUT_IN, x, z, 0.0f);
				}
			}
			else if (pRoomInfo->m_SoccerTime == 1)
			{
				float x = 0.0f, z = 0.0f;
				pRoomInfo->SoccerUserListLock.lock();
				auto sUserList = pRoomInfo->m_SeccerUserList;
				pRoomInfo->SoccerUserListLock.unlock();

				foreach(itr, sUserList)
				{
					auto& pEventUser = itr->second;
					if (pEventUser.strUserID.empty()
						|| pEventUser.isLoqOut)
						continue;

					CUser* pUser = g_pMain->GetUserPtr(pEventUser.strUserID, NameType::TYPE_CHARACTER);
					if (pUser == nullptr
						|| !pUser->isInGame()
						|| !pUser->isSoccerEventUser())
						continue;

					if (pUser->m_teamColour == TeamColourBlue)
					{
						x = 639.0f;
						z = 194.0f;
					}

					if (pUser->m_teamColour == TeamColourRed)
					{
						x = 703.0f;
						z = 127.0f;
					}

					pUser->ZoneChange(pUser->GetZoneID(), x, z);
					pUser->isEventSoccerEnd();
				}

				CNpc* pNpc = g_pMain->GetNpcPtr(pRoomInfo->m_sNpcID, i);
				if (pNpc == nullptr
					|| pNpc->isDead())
					continue;

				pNpc->SendInOut(INOUT_OUT, pNpc->GetX(), pNpc->GetZ(), 0.0f);
				x = 672.0f; z = 160.0f;
				pNpc->SendInOut(INOUT_IN, x, z, 0.0f);

				if (!pRoomInfo->isSoccerTime())
				{
					pRoomInfo->m_SoccerTimer = true;
					pRoomInfo->m_SoccerActive = false;
					pRoomInfo->m_SoccerTimers = 10;
				}
			}

			if (pRoomInfo->m_SoccerTime > 0)
				pRoomInfo->m_SoccerTime--;
		}
		else if (pRoomInfo->isSoccerAktive() == false)
		{
			if (pRoomInfo->isSoccerTime())
			{
				if (pRoomInfo->m_SoccerTimers == 1)
					TempleSoccerEventEnd();

				if (pRoomInfo->m_SoccerTimers > 0)
					pRoomInfo->m_SoccerTimers--;
			}
		}
	}
}

void CGameServerDlg::TempleSoccerEventEnd()
{
	for (int i = ZONE_MORADON; i <= ZONE_MORADON5; i++)
	{
		_SOCCER_STATUS_INFO* pRoomInfo = g_pMain->m_TempleSoccerEventRoomList.GetData(i);
		if (pRoomInfo == nullptr
			|| !pRoomInfo->isSoccerAktive())
			continue;

		pRoomInfo->Clean();
	}
}

void CGameServerDlg::TempleSoccerEventGool(int8 nType, int8 sZone)
{
	Packet result(WIZ_MINING, uint8(0x10));
	for (int i = ZONE_MORADON; i <= ZONE_MORADON5; i++)
	{
		_SOCCER_STATUS_INFO* pRoomInfo = g_pMain->m_TempleSoccerEventRoomList.GetData(i);
		if (pRoomInfo == nullptr)
			continue;

		pRoomInfo->SoccerUserListLock.lock();
		auto sUserList = pRoomInfo->m_SeccerUserList;
		pRoomInfo->SoccerUserListLock.unlock();

		foreach(itr, sUserList)
		{
			auto& pEventUser = itr->second;
			if (pEventUser.strUserID.empty() 
				|| pEventUser.isLoqOut)
				continue;

			CUser* pUser = g_pMain->GetUserPtr(pEventUser.strUserID, NameType::TYPE_CHARACTER);
			if (pUser == nullptr
				|| !pUser->isInGame()
				|| !pUser->isSoccerEventUser()
				|| pUser->GetZoneID() != sZone)
				continue;

			result << uint8(0x01)
				<< uint32(pRoomInfo->m_SoccerSocketID)
				<< nType << nType
				<< pRoomInfo->m_SoccerBlueGool
				<< pRoomInfo->m_SoccerRedGool;
			pUser->Send(&result);
		}
	}
}

#pragma endregion

void CUser::HandleSoccer(Packet& pkt)
{
	Packet result(WIZ_MINING, uint8(MiningSoccer));
	uint16 resultCode = MiningResultSuccess;

	// Are we mining already?
	if (isMining())
		resultCode = MiningResultMiningAlready;

	result << resultCode;

	// If nothing went wrong, allow the user to start mining.
	// Be sure to let everyone know we're mining.
	if (resultCode == MiningResultSuccess)
	{
		m_bMining = true;
		result << uint32(GetID());
		SendToRegion(&result);
	}
	else
	{
		Send(&result);
	}
}