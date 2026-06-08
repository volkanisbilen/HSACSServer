#include "stdafx.h"
#include "KingSystem.h"
#include "DBAgent.h"
#include <iostream>
#include "MagicInstance.h"

uint32 CGameServerDlg::Timer_UpdateGameTime(void* lpParam)
{
	while (g_bRunning)
	{
		g_pMain->UpdateGameTime();
#if 0
		if (!g_pMain->pCollectionRaceEvent.m_sCollectionStatus && UNIXTIME > g_pMain->autocrchecktime)
		{
			g_pMain->m_CollectionRaceListArray.m_lock.lock();
			auto copylist = g_pMain->m_CollectionRaceListArray.m_UserTypeMap;
			g_pMain->m_CollectionRaceListArray.m_lock.unlock();

			foreach(itr, copylist)
			{
				auto* pCollection = itr->second;
				if (!pCollection) 
					continue;

				if (!pCollection->m_bCollectionAutoStatus
					|| pCollection->m_bCollectionAutoHour < 0
					|| pCollection->m_bCollectionAutoMinute < 0)
					continue;

				if (pCollection->m_bCollectionAutoHour == g_pMain->m_sHour && pCollection->m_bCollectionAutoMinute == g_pMain->m_sMin && g_pMain->m_sSec == 0)
				{
					g_pMain->autocrchecktime = UNIXTIME + 5;
					g_pMain->CollectionRaceStart(pCollection, itr->first, nullptr);
				}
			}
		}
#endif
		sleep(1 * SECOND);
	}
	return 0;
}

void CGameServerDlg::UpdateGameTime()
{
	DateTime now(&g_localTime);
	// g_pMain->LicenseSystem(); // 18.10.2020 Belirlenen Tarihe Lisanslama
	Packet result;

	if (m_sSec != now.GetSecond())
	{
		// Check timed King events.
		m_KingSystemArray.m_lock.lock();
		auto m_bKingSystemArray = m_KingSystemArray.m_UserTypeMap;
		m_KingSystemArray.m_lock.unlock();

		foreach(itr, m_bKingSystemArray)
		{
			if (itr->second == nullptr)
				continue;

			itr->second->CheckKingTimer();
		}

		m_AutomaticCommandArray.m_lock.lock();
		auto m_sAutomaticCommandArray = m_AutomaticCommandArray.m_UserTypeMap;
		m_AutomaticCommandArray.m_lock.unlock();

		foreach(itr, m_sAutomaticCommandArray)
		{
			auto* pAutoCommand = itr->second;
			if (!pAutoCommand
				|| pAutoCommand->command.empty()) 
				continue;

			if (pAutoCommand->hour != m_sHour 
				|| pAutoCommand->minute != m_sMin)
				continue;

			if (pAutoCommand->iDay != 7
				&& pAutoCommand->iDay != m_sDate)
				continue;

			ProcessServerCommand(pAutoCommand->command);
		}
	}

	if (m_sMin != now.GetMinute())
	{
		m_ReloadKnightAndUserRanksMinute++;
		if (m_ReloadKnightAndUserRanksMinute == RELOAD_KNIGHTS_AND_USER_RATING)
		{
			m_ReloadKnightAndUserRanksMinute = 0;
			ReloadKnightAndUserRanks(false);
		}

		// Player Ranking Rewards
		std::list<std::string> vargs = StrSplit(m_sPlayerRankingsRewardZones, ",");
		uint8 nZones = (uint8)vargs.size();
		if (nZones > 0)
		{
			uint8 nZoneID = 0;
			for (int i = 0; i < nZones; i++)
			{
				nZoneID = atoi(vargs.front().c_str());
				SetPlayerRankingRewards(nZoneID);
				vargs.pop_front();
			}
		}
	}

	// Every hour
	if (m_sHour != now.GetHour())
	{
		ResetPlayerRankings();
		UpdateWeather();

		if (m_bSantaOrAngel)
			SendFlyingSantaOrAngel();

		result.clear();
		result.Initialize(WIZ_DB_SAVE);
		result << uint8(ProcDbServerType::UpdateKnights);
		g_pMain->AddDatabaseRequest(result);
	}

	// Every day
	if (m_sDate != now.GetDay())
	{
		EventTimerSet();
		g_pMain->UpdateFlagAndCape();

		result.clear();
		result.Initialize(WIZ_DB_SAVE);
		result << uint8(ProcDbServerType::UpdateSiegeWarfareDb);
		g_pMain->AddDatabaseRequest(result);

		m_KingSystemArray.m_lock.lock();
		auto m_sKingSystemArray = m_KingSystemArray.m_UserTypeMap;
		m_KingSystemArray.m_lock.unlock();

		foreach(itr, m_sKingSystemArray)
		{
			if (itr->second == nullptr)
				continue;

			result.clear();
			result.Initialize(WIZ_DB_SAVE);
			result << uint8(ProcDbServerType::UpdateKingSystemDb);
			result << itr->second->m_byNation << itr->second->m_nNationalTreasury << itr->second->m_nTerritoryTax;
			g_pMain->AddDatabaseRequest(result);
		}
	}

	// Every month
	if (m_sMonth != now.GetMonth())
	{
		// Reset monthly NP.
		result.clear();
		result.Initialize(WIZ_DB_SAVE);
		result << uint8(ProcDbServerType::ResetLoyalty);
		g_pMain->AddDatabaseRequest(result);
	}

	// Update the server time
	m_sYear = now.GetYear();
	m_sMonth = now.GetMonth();
	m_sDate = now.GetDay();
	m_sHour = now.GetHour();
	m_sMin = now.GetMinute();
	m_sSec = now.GetSecond();

}

void CUser::BotUsingSkill(uint16 sTargetID, uint32 nSkillID)
{
	MagicInstance instance;
	instance.bIsRunProc = true;
	instance.sCasterID = sTargetID;
	instance.sTargetID = GetSocketID();
	instance.nSkillID = nSkillID;
	instance.sSkillCasterZoneID = GetZoneID();
	instance.Run();
}

uint32 CGameServerDlg::Timer_t_1(void* lpParam)
{
#if(SKILLTEST)

	return 0;

	while (g_bRunning)
	{
		for (uint16 i = 0; i < MAX_USER; i++)
		{
			CUser* pUser = g_pMain->GetUserPtr(i);
			if (pUser == nullptr || !pUser->isInGame()) continue;

			CNpcThread* zoneitrThread = g_pMain->m_arNpcThread.GetData(pUser->GetZoneID());
			if (!zoneitrThread)
				continue;

			zoneitrThread->m_arNpcArray.m_lock.lock();
			auto copymap = zoneitrThread->m_arNpcArray.m_UserTypeMap;
			zoneitrThread->m_arNpcArray.m_lock.unlock();

			foreach(ax, copymap)
			{
				if (ax->second && ax->second->GetProtoID() == 13013)
					pUser->ClientEvent(ax->second->GetID());
			}
		}

		sleep(1000);
	}
#endif
	return (uint32)0;
}

uint32 CGameServerDlg::Timer_t_2(void* lpParam)
{
#if(SKILLTEST)


	while (g_bRunning)
	{
		/*g_pMain->m_arNpcThread.m_lock.lock();
		foreach_stlmap_nolock(iyt, g_pMain->m_arNpcThread) {
			if (!iyt->second) continue;
			iyt->second->m_arNpcArray.m_lock.lock();
			foreach_stlmap_nolock(a, iyt->second->m_arNpcArray) {
				if (a->second) {
					a->second->IsNoPathFind(10.0f);
					a->second->Type4Duration();
				}
			}
			iyt->second->m_arNpcArray.m_lock.unlock();
		}
		g_pMain->m_arNpcThread.m_lock.unlock();*/


		for (uint16 i = 0; i < MAX_USER; i++)
		{
			CUser* pUser = g_pMain->GetUserPtr(i);
			if (pUser == nullptr || !pUser->isInGame())
				continue;

			/*pUser->SendPremiumInfo();

			pUser->InitType4();
			pUser->Type4Duration();
			pUser->Update();
			pUser->m_bInvisibilityType = (uint8)InvisibilityType::INVIS_DISPEL_ON_MOVE;
			pUser->Type9Duration(true);
			pUser->RemoveStealth();*/

			if (pUser->isWarrior())
			{
				if (pUser->isNoviceWarrior())
				{
					pUser->BotUsingSkill(pUser->GetSocketID(), 105002); //sprint

				}
				else if (pUser->isMasteredWarrior())
				{
					if (pUser->GetNation() == 1)
					{
						pUser->BotUsingSkill(pUser->GetSocketID(), 106002); //sprint
					}
					else
					{
						pUser->BotUsingSkill(pUser->GetSocketID(), 206002); //sprint
					}
				}
			}

			if (UNIXTIME > pUser->testskillusetime)
			{
				pUser->BotUsingSkill(pUser->GetSocketID(), 500354);//814678000
				pUser->BotUsingSkill(pUser->GetSocketID(), 500512);//800220000
				pUser->BotUsingSkill(pUser->GetSocketID(), 500508);//800130000
				pUser->BotUsingSkill(pUser->GetSocketID(), 490160);//800127000

				if (!pUser->CheckExistItem(814678000)) pUser->GiveItem("tst", 814678000);
				if (!pUser->CheckExistItem(800220000)) pUser->GiveItem("tst", 800220000);
				if (!pUser->CheckExistItem(800130000)) pUser->GiveItem("tst", 800130000);
				if (!pUser->CheckExistItem(800127000)) pUser->GiveItem("tst", 800127000);
			}

			if (UNIXTIME > pUser->testskillusetime2)
			{
				std::vector<uint8> willDel;
				pUser->m_buffLock.lock();
				foreach(itr, pUser->m_buffMap) willDel.push_back(itr->first);
				pUser->m_buffLock.unlock();
				foreach(itr, willDel)
					CMagicProcess::RemoveType4Buff((*itr), pUser, true, pUser->isLockableScroll((*itr)));
				pUser->testskillusetime2 = UNIXTIME + myrand(120, 240);
			}
		}
		sleep(1 * SECOND);
	}
#endif
	return (uint32)0;
}

uint32 CGameServerDlg::Timer_t_3(void* lpParam)
{
#if(SKILLTEST)
	return 0;
	while (g_bRunning)
	{
		for (uint16 i = 0; i < MAX_USER; i++)
		{
			CUser* pUser = g_pMain->GetUserPtr(i);
			if (pUser == nullptr || !pUser->isInGame())
				continue;

			CNpcThread* zoneitrThread = g_pMain->m_arNpcThread.GetData(pUser->GetZoneID());
			if (!zoneitrThread)
				continue;

			zoneitrThread->m_arNpcArray.m_lock.lock();
			auto copymap = zoneitrThread->m_arNpcArray.m_UserTypeMap;
			zoneitrThread->m_arNpcArray.m_lock.unlock();

			foreach(ax, copymap)
			{
				if (ax->second && ax->second->GetProtoID() == 13013)
					pUser->ClientEvent(ax->second->GetID());
			}

		}
		sleep(myrand(1250, 2000));
	}
#endif
	return (uint32)0;
}

#pragma region CGameServerDlg::Timer_UpdateSessions(void * lpParam)
uint32 CGameServerDlg::Timer_UpdateSessions(void* lpParam)
{
	while (g_bRunning)
	{
		g_pMain->m_sClientSocket.GetLock().lock();
		SessionMap sessMap = g_pMain->m_sClientSocket.GetActiveSessionMap();
		g_pMain->m_sClientSocket.GetLock().unlock();
		foreach(itr, sessMap)
		{
			CUser* pUser = TO_USER(itr->second);
			if (pUser == nullptr)
				continue;

			if (!pUser->m_strAccountID.empty() && !pUser->isInGame())
			{
				ULONGLONG timeout = KOSOCKET_LOADING_TIMEOUT, nDifference = (UNIXTIME2 - pUser->GetLastResponseTime());
				if (nDifference >= timeout)
				{
					pUser->goDisconnect("time out", __FUNCTION__);
					continue;
				}
			}

			if (pUser->isInGame())
			{
				// Update the player, and hence any skill effects while we're here.
				pUser->Update();
				pUser->PetMonAttack();
				pUser->CheckDelayedTime();
			}
		}
		sleep(1 * SECOND);
	}
	return uint32(0);
}
#pragma endregion 

#pragma region CGameServerDlg::Timer_UpdateConcurrent(void * lpParam)
uint32 CGameServerDlg::Timer_UpdateConcurrent(void* lpParam)
{
	while (true)
	{
		g_pMain->ReqUpdateConcurrent();
		sleep(120 * SECOND);
	}
	return 0;
}
#pragma endregion 

#pragma region CGameServerDlg::Timer_TimedNotice(void * lpParam)
uint32 CGameServerDlg::Timer_TimedNotice(void* lpParam)
{
	while (true)
	{
		g_pMain->m_TimedNoticeArray.m_lock.lock();
		auto m_sTimedNotice = g_pMain->m_TimedNoticeArray.m_UserTypeMap;
		g_pMain->m_TimedNoticeArray.m_lock.unlock();

		foreach(itr, m_sTimedNotice)
		{
			_TIMED_NOTICE* pTimedNotice = itr->second;
			if (pTimedNotice == nullptr
				|| pTimedNotice->usingtime > UNIXTIME)
				continue;

			if (pTimedNotice->time < 1)
				pTimedNotice->time = 1;

			pTimedNotice->usingtime = (uint32)UNIXTIME + pTimedNotice->time * MINUTE;
			Packet result;
			std::string notice = pTimedNotice->notice;
			g_pMain->GetServerResource(IDP_ANNOUNCEMENT, &notice, notice.c_str());
			ChatPacket::Construct(&result, (uint8)pTimedNotice->noticetype, &notice);

			if (pTimedNotice->zoneid != 0)
				g_pMain->Send_Zone(&result, (uint8)pTimedNotice->zoneid, nullptr, (uint8)Nation::ALL);
			else 
				g_pMain->Send_All(&result, nullptr, (uint8)Nation::ALL);
		} 
		sleep(60 * SECOND);
	} 
	return uint32(0);
}
#pragma endregion 

uint32 CGameServerDlg::Timer_BotMoving(void* lpParam)
{
	while (true)
	{
		g_pMain->BotHandlerMainTimer();
		Sleep(940);
	}

	return uint32(0);
}

#define BOT_MOVE_SLEEP 100
uint32 CGameServerDlg::Timer_AutoBotMoving(void* lpParam)
{
	while (true)
	{
		g_pMain->BotAutoHandlerMainTimer();
		Sleep(BOT_MOVE_SLEEP);
	}

	return uint32(0);
}
#if 0
uint32 CGameServerDlg::Timer_AutoBotSpawn(void* lpParam)
{
	while (true)
	{
		{
			std::set<uint32> removelist;  removelist.clear();
			g_pMain->m_addBotlistLock.lock();
			foreach(itr, g_pMain->m_addbotlist)
			{
				if (itr->second.addtime > UNIXTIME2)
					continue;

				g_pMain->MerchantBotAddSet(itr->first, itr->second.type);
				removelist.insert(itr->first);
			}
			foreach(itr, removelist) g_pMain->m_addbotlist.erase(*itr);
			g_pMain->m_addBotlistLock.unlock();
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

					if (g_pMain->SpawnLoginBot(g_pMain->m_sBotAutoX, 4, g_pMain->m_sBotAutoZ, g_pMain->m_sBotAutoType) > 0)
						g_pMain->m_sMaxBotFinish++;
				}

				g_pMain->m_sBotRespawnTick = GetTickCount64() + g_pMain->m_sBotTimeNext;
			}
		}

		Sleep(1 * SECOND);
	}

	return 0;
}
#endif
#pragma region CGameServerDlg::ReqUpdateConcurrent()
void CGameServerDlg::ReqUpdateConcurrent()
{
	uint32 sCount = 0;
	for (uint16 i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = g_pMain->GetUserPtr(i);
		if (!pUser || !pUser->isInGame())
			continue;

		sCount++;
	}
	sCount += (uint32)g_pMain->m_sBotCharackterNameArray.size();

	Packet result(WIZ_ZONE_CONCURRENT);
	result << uint32(m_nServerNo)
		<< sCount;
	AddDatabaseRequest(result);
}
#pragma endregion