#include "stdafx.h"

_FORGETTEN_TEMPLE_STAGES CGameServerDlg::ForgettenTempleGetStage()
{
	g_pMain->m_ForgettenTempleStagesArray.m_lock.lock();
	auto m_sForgettenTempleStagesArray = g_pMain->m_ForgettenTempleStagesArray.m_UserTypeMap;
	g_pMain->m_ForgettenTempleStagesArray.m_lock.unlock();

	foreach(ita, m_sForgettenTempleStagesArray)
	{
		auto* pStage = ita->second;
		if (pStage == nullptr 
			|| pStage->Type != pForgettenTemple.Type 
			|| pStage->Stage != pForgettenTemple.Stage)
			continue;

		return *pStage;
	}
	return _FORGETTEN_TEMPLE_STAGES();
}

std::vector< _FORGETTEN_TEMPLE_SUMMON> CGameServerDlg::ForgettenTempleLoadSpawn()
{
	std::vector< _FORGETTEN_TEMPLE_SUMMON> mVector;
	g_pMain->m_ForgettenTempleMonsterArray.m_lock.lock();
	auto m_sForgettenTempleMonsterArray = g_pMain->m_ForgettenTempleMonsterArray.m_UserTypeMap;
	g_pMain->m_ForgettenTempleMonsterArray.m_lock.unlock();

	foreach(ita, m_sForgettenTempleMonsterArray)
	{
		auto* pSummon = ita->second;
		if (pSummon == nullptr 
			|| pSummon->Type != pForgettenTemple.Type)
			continue;

		if (pSummon->Stage != pForgettenTemple.Stage)
			continue;

		mVector.push_back(*pSummon);
	}
	return mVector;
}

void CGameServerDlg::ForgettenTempleReset()
{
	RemoveAllEventNpc(ZONE_FORGOTTEN_TEMPLE);
	pForgettenTemple.Initialize(true);
}

void CGameServerDlg::ForgettenTempleStart(uint8 Type, uint8 MinLevel, uint8 MaxLevel)
{
	if (m_ForgettenTempleMonsterArray.IsEmpty() || m_ForgettenTempleStagesArray.IsEmpty() || pForgettenTemple.isActive) return;
	pForgettenTemple.isActive = true;
	pForgettenTemple.isJoin = true;
	pForgettenTemple.MinLevel = pForgettenTemple.ptimeopt.MinLevel;
	pForgettenTemple.MaxLevel = pForgettenTemple.ptimeopt.MaxLevel;
	pForgettenTemple.Stage = 1;
	pForgettenTemple.Type = Type;
	pForgettenTemple.StartTime = UNIXTIME;
	pForgettenTemple.FinishTime = UNIXTIME + (pForgettenTemple.ptimeopt.PlayingTime * MINUTE);
	Announcement(IDS_MONSTER_CHALLENGE_OPEN);
}

void CGameServerDlg::ForgettenTempleSendItem()
{
#define MIN_DAMAGE 50000

	std::vector<_EVENT_REWARD> mreward;
	g_pMain->m_EventRewardArray.m_lock.lock();
	auto m_sEventRewardArray = g_pMain->m_EventRewardArray.m_UserTypeMap;
	g_pMain->m_EventRewardArray.m_lock.unlock();

	foreach(itr, m_sEventRewardArray)
		if (itr->second && 13 == itr->second->local_id && itr->second->status)
			mreward.push_back(*itr->second);

	pForgettenTemple.UserListLock.lock();
	std::map<uint16, uint64> copymap = pForgettenTemple.UserList;
	pForgettenTemple.UserListLock.unlock();

	foreach(itr, copymap) 
	{
		auto* pUser = g_pMain->GetUserPtr(itr->first);
		if (pUser == nullptr 
			|| !pUser->isInGame() 
			|| pUser->isDead() 
			|| pUser->GetZoneID() != ZONE_FORGOTTEN_TEMPLE)
			continue;

		foreach(itr2, mreward) 
		{

			for (int i = 0; i < 3; i++)
				if(itr2->itemid[i])
					pUser->GiveItem("FT", itr2->itemid[i], itr2->itemcount[i], true, itr2->itemexpiration[i]);

			if (itr2->cash)
				pUser->GiveBalance(itr2->cash);

			if (itr2->experience)
				pUser->ExpChange("FT", itr2->experience, true);

			if (itr2->loyalty)
				pUser->SendLoyaltyChange("FT", itr2->loyalty);

			if (itr2->noah)
				pUser->GoldGain(itr2->noah, true, false);
		}
	}
}

void CGameServerDlg::FtFinish() {
	pForgettenTemple.isActive = false;
	KickOutZoneUsers(ZONE_FORGOTTEN_TEMPLE, ZONE_MORADON);
	ForgettenTempleReset();
	Announcement(IDS_MONSTER_CHALLENGE_CLOSE);
}

void CGameServerDlg::ForgettenTempleTimerProc()
{
	if (!pForgettenTemple.isActive)
		return;

	if (pForgettenTemple.isFinished && pForgettenTemple.isWaiting && UNIXTIME > pForgettenTemple.WaitingTime)
		return FtFinish();

	if (pForgettenTemple.isFinished)
		return;

	if (UNIXTIME > pForgettenTemple.FinishTime) {
		if (!pForgettenTemple.FinishTime) printf("Forgetten Temple Starting Error | Info:Finish time is zero. ! \n");
		return FtFinish();
	}

	if (!pForgettenTemple.isSummonCheck && UNIXTIME > (pForgettenTemple.StartTime + pForgettenTemple.ptimeopt.SummonTime)) 
	{
		pForgettenTemple.isSummonCheck = true;
		pForgettenTemple.Stage = 1;
		pForgettenTemple.isLastSummonTime = UNIXTIME + 30;
		pForgettenTemple.isJoin = false;
		pForgettenTemple.isSummon = true;
		Announcement(IDS_MONSTER_CHALLENGE_START, (uint8)Nation::ALL, (uint8)ChatType::PUBLIC_CHAT);
	}

	if (pForgettenTemple.isSummon && !pForgettenTemple.isLastSummon) 
	{
		_FORGETTEN_TEMPLE_STAGES pStages = ForgettenTempleGetStage();
		if (pStages.nIndex != -1) 
		{
			if (!pForgettenTemple.isLastSummon)
			{
				uint32 ntime = (uint32)pForgettenTemple.StartTime + (pStages.Time + pForgettenTemple.ptimeopt.SummonTime);
				if (UNIXTIME > ntime)
				{
					auto summonlist = ForgettenTempleLoadSpawn();
					if (summonlist.size())
					{
						foreach(pSummon, summonlist)
							SpawnEventNpc(pSummon->SidID, true, ZONE_FORGOTTEN_TEMPLE, (float)pSummon->PosX, 0, (float)pSummon->PosZ, pSummon->SidCount, pSummon->Range, 0, 0, -1, 0, 0, 1, 0, (uint16)SpawnEventType::ForgettenTempleSummon);

						pForgettenTemple.Stage++;
						pForgettenTemple.isLastSummonTime = UNIXTIME;
					}
					else
					{
						pForgettenTemple.isLastSummon = true;
						pForgettenTemple.isSummon = false;
					}
				}
			}
		}
		else 
		{
			pForgettenTemple.isLastSummon = true;
			pForgettenTemple.isSummon = false;
		}
	}

	if (pForgettenTemple.isLastSummon && !pForgettenTemple.isSummon && !g_pMain->pForgettenTemple.monstercount) 
	{
		pForgettenTemple.WaitingTime = UNIXTIME + pForgettenTemple.ptimeopt.WaitingTime;
		pForgettenTemple.isWaiting = true;
		pForgettenTemple.isFinished = true;
		ForgettenTempleSendItem();
		Announcement(IDS_MONSTER_CHALLENGE_VICTORY);
	}
}

void CNpc::ForgettenTempleMonsterDead() 
{
	if (!g_pMain->isForgettenTempleActive() || GetZoneID() != ZONE_FORGOTTEN_TEMPLE || e_stype != e_summontype::m_ftmonster)
		return;

	if (g_pMain->pForgettenTemple.monstercount > 0) g_pMain->pForgettenTemple.monstercount--;

	uint32 nSkillID = 0;
	if (GetProtoID() == 9816) nSkillID = 492059;
	else if (GetProtoID() == 9817) nSkillID = 492060;
	else if (GetProtoID() == 9818) nSkillID = 492061;
	else if (GetProtoID() == 9819) nSkillID = 492060;
	else if (GetProtoID() == 9820) nSkillID = 492062;
	if (!nSkillID)
		return;

	bool r_result = false;

	MagicInstance instance;
	instance.bIsRunProc = true;
	instance.bIsItemProc = true;
	instance.sCasterID = GetID();
	instance.sTargetID = -1;
	instance.nSkillID = nSkillID;
	instance.sSkillCasterZoneID = GetZoneID();
	instance.sData[0] = (uint16)127;
	instance.sData[2] = (uint16)127;
	instance.Run();
}