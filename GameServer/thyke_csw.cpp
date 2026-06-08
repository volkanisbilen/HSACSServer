#include "stdafx.h"

#pragma region CGameServerDlg::csw_maintimer
void CGameServerDlg::SiegeWarfareMainTimer() {
	if (!isCswActive())
		return;

	uint32 r_time = 0;
	if (pCswEvent.CswTime > UNIXTIME)
		r_time = uint32(pCswEvent.CswTime - UNIXTIME);

	if (pCswEvent.Status == CswOpStatus::Preparation && !pCswEvent.prepare_check) {
		if (r_time) {
			if (r_time == 10 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::Preparation, 10);
			else if (r_time == 5 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::Preparation, 5);
			else if (r_time == 4 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::Preparation, 4);
			else if (r_time == 3 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::Preparation, 3);
			else if (r_time == 2 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::Preparation, 2);
			else if (r_time == 1 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::Preparation, 1);
		}
		else {
			pCswEvent.prepare_check = true;
			CastleSiegeWarfareWarOpen();
		}
	}
	else if (pCswEvent.Status == CswOpStatus::War && !pCswEvent.war_check) {
		if (r_time) {
			if (r_time == 30 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::War, 30);
			else if (r_time == 10 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::War, 10);
			else if (r_time == 5 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::War, 5);
			else if (r_time == 4 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::War, 4);
			else if (r_time == 3 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::War, 3);
			else if (r_time == 2 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::War, 2);
			else if (r_time == 1 * MINUTE) CastleSiegeWarfareRawNotice((uint8)CswNotice::War, 1);
		}
		else {
			pCswEvent.war_check = true;
			CastleSiegeWarfareClose();
		}
	}
}
#pragma endregion

#pragma region CGameServerDlg::CastleSiegeWarfareClose()
void CGameServerDlg::CastleSiegeWarfareClose() {

	if (!isCswActive())
		return;

	m_byBattleOpen = m_byOldBattleOpen = NO_BATTLE;
	CastleSiegeWarfareReset();
	CastleSiegeWarfareTools(true, CswNotice::CswFinish, true, false, false, true, true);
	ResetAllEventObject(ZONE_DELOS);
}
#pragma endregion

#pragma region CGameServerDlg::CastleSiegeWarfareReset()
void CGameServerDlg::CastleSiegeWarfareReset() {
	pCswEvent.Status = CswOpStatus::NotOperation;
	pCswEvent.CswTime = 0;
	pCswEvent.Started = false;
	pCswEvent.MonumentTime = 0;
	pCswEvent.war_check = pCswEvent.prepare_check = false;
}
#pragma endregion

#pragma region CGameServerDlg::CastleSiegeWarfareRawNotice(uint8 NoticeType,int32 Time)
void CGameServerDlg::CastleSiegeWarfareRawNotice(uint8 NoticeType, int32 Time)
{
	uint32 a = 0;
	 if ((CswNotice)NoticeType == CswNotice::Preparation)
		a = IDS_SIEGE_WAR_READY_TIME_NOTICE;
	else if ((CswNotice)NoticeType == CswNotice::War)
		a = IDS_SIEGE_WAR_TIME_NOTICE;

	if (!a)
		return;

	std::string notice = "";

	g_pMain->GetServerResource(a, &notice, Time);

	Packet pkt;
	g_pMain->GetServerResource(IDP_ANNOUNCEMENT, &notice, notice.c_str());
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &notice);

	for (int i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		pUser->Send(&pkt);
	}
}
#pragma endregion

#pragma region CGameServerDlg::CastleSiegeWarfarePrepaOpen
void CGameServerDlg::CastleSiegeWarfarePrepaOpen() {

	if (isCswActive())
		return;

	ResetAllEventObject(ZONE_DELOS);
	g_pMain->pCswEvent.mClanListLock.lock();
	g_pMain->pCswEvent.mClanList.clear();
	g_pMain->pCswEvent.mClanListLock.unlock();
	pCswEvent.Status = CswOpStatus::Preparation;
	pCswEvent.CswTime = UNIXTIME + (pCswEvent.poptions.Preparing * MINUTE);
	pCswEvent.Started = true;
	CastleSiegeWarfareTools(true, CswNotice::Preparation, true, true, false, false);
}
#pragma endregion

#pragma region CGameServerDlg::CastleSiegeWarfareWarOpen
void CGameServerDlg::CastleSiegeWarfareWarOpen() {

	if (!isCswActive())
		return;

	m_byBattleOpen = m_byOldBattleOpen = SIEGE_BATTLE;
	pCswEvent.Status = CswOpStatus::War;
	pCswEvent.CswTime = UNIXTIME + (pCswEvent.poptions.wartime * MINUTE);
	CastleSiegeWarfareTools(true, CswNotice::War, true, false, true, true);
}
#pragma endregion

#pragma region CGameServerDlg::CastleSiegeWarfareTools(bool Notice, CswNotice s, bool Flag, bool KickOutPreapre, bool KickOutWar, bool town, bool reward)
void CGameServerDlg::CastleSiegeWarfareTools(bool Notice, CswNotice s, bool Flag, bool KickOutPreapre, bool KickOutWar, bool town, bool reward)
{
	CKnights* pOwner = nullptr;
	if (Flag && g_pMain->pSiegeWar.sMasterKnights)
		pOwner = g_pMain->GetClanPtr(g_pMain->pSiegeWar.sMasterKnights);

	for (int i = 0; i < MAX_USER; i++)
	{
		CUser* pUser = GetUserPtr(i);
		if (pUser == nullptr || !pUser->isInGame())
			continue;

		if (reward && pUser->GetZoneID() == ZONE_DELOS)
		{
			if (g_pMain->pSiegeWar.sMasterKnights == pUser->GetClanID())
			{
				for (uint8 i = 0; i < 3; i++) {
					if (!pCswEvent.poptions.itemid[i] || !pCswEvent.poptions.itemcount[i])
						continue;

					pUser->GiveItem("Csw winner knight", pCswEvent.poptions.itemid[i],
						pCswEvent.poptions.itemcount[i], true, pCswEvent.poptions.itemtime[i]);
				}

				if (pCswEvent.poptions.cash || pCswEvent.poptions.tl)
					pUser->GiveBalance(pCswEvent.poptions.cash, pCswEvent.poptions.tl);

				if (pCswEvent.poptions.money)
					pUser->GoldGain(pCswEvent.poptions.money, true);

				if (pCswEvent.poptions.loyalty)
					pUser->SendLoyaltyChange("csw winner knight", pCswEvent.poptions.loyalty, false, false, true);
			}
		}

		if (Notice && s == CswNotice::MonumentKilled && pUser->GetZoneID() == ZONE_DELOS)
			pUser->CastleSiegeWarfareNotice(s);
		else if (Notice && s == CswNotice::Preparation)
			pUser->CastleSiegeWarfareNotice(s);
		else if (Notice && s == CswNotice::War && pUser->GetZoneID() == ZONE_DELOS)
			pUser->CastleSiegeWarfareNotice(s);
		else if (Notice && s == CswNotice::CswFinish)
			pUser->CastleSiegeWarfareNotice(s);

		if (Flag && pUser->GetZoneID() == ZONE_DELOS) 
			pUser->CastleSiegeWarfareFlag(pOwner);

		if (KickOutPreapre
			&& (pUser->GetZoneID() == ZONE_DELOS || pUser->GetZoneID() == ZONE_HELL_ABYSS || pUser->GetZoneID() == ZONE_DESPERATION_ABYSS))
			pUser->ZoneChange(ZONE_MORADON, 0.0f, 0.0f);

		if (KickOutWar && pUser->isInPKZone())
			pUser->ZoneChange(ZONE_MORADON, 0.0f, 0.0f);

		if (town && pUser->GetZoneID() == ZONE_DELOS)
			pUser->CastleSiegeWarfareTown();
	}
}
#pragma endregion

#pragma region CUser::CastleSiegeWarfareFlag(CKnights* pOwner,bool finish)
void CUser::CastleSiegeWarfareFlag(CKnights* pKnights, bool finish)
{
	CKnights* pOwner = pKnights;
	if (g_pMain->pSiegeWar.sMasterKnights)
		pOwner = g_pMain->GetClanPtr(g_pMain->pSiegeWar.sMasterKnights);

	Packet result(WIZ_SIEGE, uint8(2));
	result.SByte();
	result << uint8(0);
	if (pOwner)
		result << pOwner->GetID() << pOwner->m_sMarkVersion << pOwner->m_byFlag << pOwner->m_byGrade;
	else
		result << uint32(0) << uint16(0);
	Send(&result);

	uint32 sTimeCount = 0;
	if (g_pMain->pCswEvent.CswTime > UNIXTIME)
		sTimeCount = uint32(g_pMain->pCswEvent.CswTime - UNIXTIME);

	enum class CastleSiegeWarfareOpcode
	{
		WIZ_TIMER,
		WIZ_FINISH
	};
#if(__VERSION < 2369)
	//Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::CSW));
	result.clear();
	result.Initialize(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::CSW);
	result.SByte();
#endif
	if (!finish)
	{
		if (sTimeCount)
		{
#if(__VERSION < 2369)
			result << uint8(CastleSiegeWarfareOpcode::WIZ_TIMER) << sTimeCount << std::string(pOwner ? pOwner->GetName() : "") << (uint8)g_pMain->pCswEvent.Status;
			if (g_pMain->pCswEvent.Status == CswOpStatus::Preparation)
				result << g_pMain->pCswEvent.poptions.Preparing;
			else
				result << g_pMain->pCswEvent.poptions.wartime;

			Send(&result);
#endif
			return;
		}
	}
#if(__VERSION < 2369)
	result << uint8(CastleSiegeWarfareOpcode::WIZ_FINISH);
	Send(&result);
#endif
}
#pragma endregion

#pragma region CUser::CastleSiegeWarfareNotice(CswNotice p)
void CUser::CastleSiegeWarfareNotice(CswNotice p)
{
	std::string notice = ""; Packet pkt;

	CKnights* pKnights = nullptr;
	if ((p == CswNotice::MonumentKilled || p == CswNotice::CswFinish) && g_pMain->pSiegeWar.sMasterKnights)
		pKnights = g_pMain->GetClanPtr(g_pMain->pSiegeWar.sMasterKnights);

	switch (p)
	{
	case CswNotice::Preparation:
		g_pMain->GetServerResource(IDS_SIEGE_WAR_READY_TIME_NOTICE, &notice, g_pMain->pCswEvent.poptions.Preparing);
		break;
	case CswNotice::MonumentKilled:
		g_pMain->GetServerResource(IDS_NPC_GUIDON_DESTORY, &notice, pKnights == nullptr ? "***" : pKnights->GetName().c_str());
		break;
	case CswNotice::War:
		g_pMain->GetServerResource(IDS_SIEGE_WAR_TIME_NOTICE, &notice, g_pMain->pCswEvent.poptions.wartime);
		break;
	case CswNotice::CswFinish:
		if (pKnights)
			g_pMain->GetServerResource(IDS_SIEGE_WAR_VICTORY, &notice, pKnights->GetName().c_str());
		else
			g_pMain->GetServerResource(IDS_SIEGE_WAR_END, &notice);
		break;
	}
	
	if (notice.empty()) 
		return;

	g_pMain->GetServerResource(IDP_ANNOUNCEMENT, &notice, notice.c_str());
	ChatPacket::Construct(&pkt, (uint8)ChatType::WAR_SYSTEM_CHAT, &notice);
	Send(&pkt);
}
#pragma endregion

#pragma region CUser::CastleSiegeWarfareCanenterDelos()
bool CUser::CastleSiegeWarfareCanenterDelos()
{
	if (g_pMain->isCswActive()) {
		if (!isInClan() || isInAutoClan())
			return false;

		auto* pClan = g_pMain->GetClanPtr(GetClanID());
		if (pClan == nullptr || pClan->m_byGrade > 3)
			return false;
	}

	if (GetLoyalty() == 0)
		return false;

	return true;
}
#pragma endregion

#pragma region CUser::CastleSiegeWarfareTown()
void CUser::CastleSiegeWarfareTown()
{
	Home(false);
}
#pragma endregion

#pragma region CUser::CastleSiegeWarfareRank()
void CUser::CastleSiegeWarfareRank()
{
	if (!isInClan() || !g_pMain->isCswActive())
		return;

	uint16 sCount = 0;
	Packet result(WIZ_SIEGE, uint8(5));
	result << uint8(2) << sCount;
	result.DByte();

	struct _list {
		uint16 clanid, killcount;
		_list(uint16 clanid, uint16 killcount) {
			this->clanid = clanid;
			this->killcount = killcount;
		}
	};


	std::vector<_list> mlist;
	g_pMain->pCswEvent.mClanListLock.lock();
	foreach(itr, g_pMain->pCswEvent.mClanList)
		mlist.push_back(_list(itr->first, itr->second));
	g_pMain->pCswEvent.mClanListLock.unlock();

	std::sort(mlist.begin(), mlist.end(), [](auto const& a, auto const& b) { return a.killcount > b.killcount; });

	uint8 sReaminCount = 0;
	foreach(itr, mlist) {
		CKnights* pknights = g_pMain->GetClanPtr(itr->clanid);
		if (!pknights)
			continue;

		sReaminCount++;
		result << itr->clanid << pknights->m_sMarkVersion << pknights->GetName() << uint8(0) << itr->killcount << uint8(1) << sReaminCount;
		sCount++;
		if (sCount >= 50)
			break;
	}
	result.ByteBuffer::put(2, sCount);
	Send(&result);
}
#pragma endregion

#pragma region CUser::CastleSiegeWarfareRankRegister()
void CUser::CastleSiegeWarfareRankRegister()
{
	if (!isInClan() || !g_pMain->isCswActive())
		return;

	Guard lock(g_pMain->pCswEvent.mClanListLock);
	auto itr = g_pMain->pCswEvent.mClanList.find(GetClanID());
	if (itr == g_pMain->pCswEvent.mClanList.end())
	{
		g_pMain->pCswEvent.mClanList.insert(std::make_pair(GetClanID(), 0));
	}
}
#pragma endregion

#pragma region CUser::CastleSiegeWarfarerankKillUpdate()
void CUser::CastleSiegeWarfarerankKillUpdate()
{
	if (!isInClan())
		return;

	Guard lock(g_pMain->pCswEvent.mClanListLock);
	auto itr = g_pMain->pCswEvent.mClanList.find(GetClanID());
	if (itr != g_pMain->pCswEvent.mClanList.end())
		itr->second++;
}
#pragma endregion

#pragma region CNpc::CastleSiegeWarfareMonumentProcess(CUser* pUser)
void CNpc::CastleSiegeWarfareMonumentProcess(CUser* pUser) {
	if (pUser == nullptr || !pUser->isInClan() || !g_pMain->isCswActive() || !g_pMain->isCswWarActive())
		return;

	auto* pknights = g_pMain->GetClanPtr(pUser->GetClanID());
	if (!pknights || pknights->m_byGrade > 3)
		return;

	g_pMain->pSiegeWar.sMasterKnights = pUser->GetClanID();
	g_pMain->UpdateSiege(g_pMain->pSiegeWar.sCastleIndex, g_pMain->pSiegeWar.sMasterKnights, g_pMain->pSiegeWar.bySiegeType, 0, 0, 0);
	g_pMain->CastleSiegeWarfareTools(true, CswNotice::MonumentKilled, true, false, false, true);
	g_pMain->ResetAllEventObject(ZONE_DELOS);
}
#pragma endregion