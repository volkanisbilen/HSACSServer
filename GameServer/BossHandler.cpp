#include "stdafx.h"

#pragma region CGameServerDlg::GetRandomIndex(uint16 Stage, uint16 MonsterID, uint16 MonsterZone)
_MONSTER_BOSS_RANDOM_SPAWN CGameServerDlg::GetRandomIndex(uint16 Stage, uint16 MonsterID, uint16 MonsterZone)
{
	std::vector<_MONSTER_BOSS_RANDOM_SPAWN> m_list;

	g_pMain->m_MonsterBossRandom.m_lock.lock();
	auto m_sMonsterBossRandom = g_pMain->m_MonsterBossRandom.m_UserTypeMap;
	g_pMain->m_MonsterBossRandom.m_lock.unlock();

	foreach(itr, m_sMonsterBossRandom)
	{
		if (itr->second 
			&& itr->second->MonsterID == MonsterID
			&& itr->second->MonsterZone == MonsterZone
			&& itr->second->Stage == Stage) 
			m_list.emplace_back(*itr->second);
	}

	return m_list.size() ? m_list[myrand(0, (int32)m_list.size() - 1)] : _MONSTER_BOSS_RANDOM_SPAWN();
}
#pragma endregion

bool CGameServerDlg::RandomBossSystemLoad()
{
	std::vector< _MONSTER_BOSS_RANDOM_STAGES> mlist;

	g_pMain->m_MonsterBossStage.m_lock.lock();
	auto m_sMonsterBossStage = g_pMain->m_MonsterBossStage.m_UserTypeMap;
	g_pMain->m_MonsterBossStage.m_lock.unlock();

	foreach(itr, m_sMonsterBossStage)
		if (itr->second)
			mlist.push_back(*itr->second);

	foreach(itr, mlist) 
	{
		auto& p = GetRandomIndex(itr->Stage, itr->MonsterID, itr->MonsterZone);
		if (p.isnull()) 
			continue;

		SpawnEventNpc(p.MonsterID, true, p.MonsterZone, (float)p.PosX, (float)0, (float)p.PosZ, 1, p.Range, 0, 0, -1, 0, 0, 1, 0, (uint16)SpawnEventType::MonsterBossSummon, p.nIndex);
	}
	return true;
}