#include "stdafx.h"

void CUser::SurroundingAllInfo(bool first, bool change)
{
	if (!m_bZoneChangeFlag)
	{
		if (first)
		{
			if (m_sBottonSign)
				return;
		}
	}

	if (!first)
	{
		if (!change)
		{
			if (m_bSurallInfoTime > 0)
			{
				if (m_bSurallInfoTime > UNIXTIME2)
					return;
			}
		}
	}

	if (!first)
		m_bSurallInfoTime = UNIXTIME2 + 5000;

	if (!m_sBottonSign)
		m_sBottonSign = true;
	
	{
		Packet result(WIZ_USER_INFORMATIN, uint8(1));
		result.SByte();
		result << uint8(1) << uint16(GetZoneID()) << uint8(0) << uint16(0);
		Send(&result);
	}

	struct CUserCBotInfo
	{
		CUser* pUser;
		CBot* pBot;
		float distance;
		CUserCBotInfo(CUser* pUser, CBot* pBot, float distance)
		{
			this->pUser = pUser;
			this->pBot = pBot;
			this->distance = distance;
		}
	};

	std::vector <CUserCBotInfo> pUserpBotList;

	uint16 sCount = 0;
	Packet result(WIZ_USER_INFORMATIN, uint8(first ? 1 : 3));
	result << uint8(1) << uint16(GetZoneID()) << uint8(0) << sCount;

	g_pMain->m_sClientSocket.GetLock().lock();
	SessionMap sessMap = g_pMain->m_sClientSocket.GetActiveSessionMap();
	g_pMain->m_sClientSocket.GetLock().unlock();
	foreach(itr, sessMap)
	{
		CUser* pUser = TO_USER(itr->second);
		if (pUser == nullptr
			|| !pUser->isInGame()
			|| pUser->GetZoneID() != GetZoneID()
			|| pUser->GetEventRoom() != GetEventRoom()
			|| (!isGM() && pUser->isGM()))
			continue;

		if (pUser->GetZoneID() == ZONE_CHAOS_DUNGEON
			|| pUser->GetZoneID() == ZONE_PRISON
			|| pUser->GetZoneID() == ZONE_KNIGHT_ROYALE
			|| pUser->GetZoneID() == ZONE_DUNGEON_DEFENCE)
			continue;

		float fDis = 0.0f;
		if (!isGM())
			fDis = Unit::GetDistanceSqrt(pUser);

		if (fDis > 0.0f && fDis > 300.0f)
			continue;

		pUserpBotList.push_back(CUserCBotInfo(pUser, nullptr, fDis));
	}

	g_pMain->m_sMapBotListArray.m_lock.lock();
	auto m_sMapBotListArray = g_pMain->m_sMapBotListArray.m_UserTypeMap;
	g_pMain->m_sMapBotListArray.m_lock.unlock();

	foreach(itr, m_sMapBotListArray)
	{
		CBot* pUser = itr->second;
		if (pUser == nullptr
			|| !pUser->isInGame()
			|| pUser->GetZoneID() != GetZoneID()
			|| pUser->GetEventRoom() != GetEventRoom()
			|| (!isGM() && pUser->isGM()))
			continue;

		if (pUser->GetZoneID() == ZONE_CHAOS_DUNGEON
			|| pUser->GetZoneID() == ZONE_PRISON
			|| pUser->GetZoneID() == ZONE_KNIGHT_ROYALE
			|| pUser->GetZoneID() == ZONE_DUNGEON_DEFENCE)
			continue;

		float fDis = 0.0f;
		if (!isGM())
			fDis = Unit::GetDistanceSqrt(pUser);

		if (fDis > 0.0f && fDis > 300.0f)
			continue;

		pUserpBotList.push_back(CUserCBotInfo(nullptr, pUser, fDis));
	}

	if (pUserpBotList.empty())
		goto fail_return;
		
	std::sort(pUserpBotList.begin(), pUserpBotList.end(), [](auto const& a, auto const& b) { return a.distance < b.distance; });

	foreach(itr, pUserpBotList)
	{
		if (sCount >= 800)
			break;

		CUser* pUser = itr->pUser;
		if (pUser == nullptr)
		{
			CBot* pUser = itr->pBot;
			if (!pUser) continue;

			result.SByte();
			result << pUser->GetName() << pUser->GetNation();
			result << uint16(1) << pUser->GetSPosX() << pUser->GetSPosZ() << uint32(pUser->GetClanID());

			CKnights* pKnights = nullptr;
			if (pUser->isInClan())
				pKnights = g_pMain->GetClanPtr(pUser->GetClanID());

			if (pKnights)
				result << pKnights->m_sMarkVersion;
			else
				result << uint16(0);

			result << uint16(1);
		}
		else
		{
			result.SByte();
			result << pUser->GetName() << pUser->GetNation();
			result << uint16(1) << pUser->GetSPosX() << pUser->GetSPosZ() << uint32(pUser->GetClanID());

			CKnights* pKnights = nullptr;
			if (pUser->isInClan())
				pKnights = g_pMain->GetClanPtr(pUser->GetClanID());

			if (pKnights)
				result << pKnights->m_sMarkVersion;
			else
				result << uint16(0);

			result << uint16(1);
		}
		sCount++;
	}
	result.put(5, sCount);
	SendCompressed(&result);
	return;
fail_return:
	Send(&result);
}

void CUser::BottomLeftRegionUserList(Packet& pkt)
{
	uint8 opcode; pkt >> opcode;
	if (opcode == (uint8)BottomUserListOpcode::UserInfoDetail && (isTrading() || isMerchanting()
		|| isFishing() || isMining() || isSellingMerchantingPreparing()))
		return;

#if(GAME_SOURCE_VERSION == 1098 || GAME_SOURCE_VERSION == 1534)
	if (opcode != (uint8)BottomUserListOpcode::UserInfoDetail
		&& opcode != (uint8)BottomUserListOpcode::unknow) return;
#endif

	switch ((BottomUserListOpcode)opcode)
	{
	case BottomUserListOpcode::Sign:
		SurroundingAllInfo(true);
		break;
	case BottomUserListOpcode::UserInfoDetail:
		HandleBottomUserInfoDetail(pkt);
		break;
	case BottomUserListOpcode::UserList:
		SurroundingAllInfo();
		break;
	case BottomUserListOpcode::RegionDelete:
		BottomUserLogOut();
		break;
	case BottomUserListOpcode::unknow:
		HandleGetUserInfoView(pkt);
		break;
	}
}

void CUser::BottomUserLogOut()
{
#if(GAME_SOURCE_VERSION == 1098 || GAME_SOURCE_VERSION == 1534)
	return;
#endif
	Packet result(WIZ_USER_INFORMATIN, uint8(BottomUserListOpcode::RegionDelete));
	result.SByte(); result << GetName();
	g_pMain->Send_Zone(&result, GetZoneID(), this, (uint8)Nation::ALL, GetEventRoom());
}

void CUser::HandleBottomUserInfoDetail(Packet& pkt)
{
#if(__VERSION > 2369)
	return;
#endif
	Packet result(WIZ_USER_INFORMATIN, uint8(UserInfoDetail));
	Packet resultPlayer(WIZ_ITEM_UPGRADE, uint8_t(9));
	std::string strCharName;
	pkt.SByte();
	pkt >> strCharName;
	CKnights* pKnights = nullptr;

	CUser* pUser = g_pMain->GetUserPtr(strCharName, NameType::TYPE_CHARACTER);
	if (pUser != nullptr)
	{
		if (!isGM() && pUser->isGM())
			return;

		resultPlayer.SByte();
		resultPlayer << uint8_t(4) << uint8_t(1)
			<< pUser->GetName() << pUser->GetNation() << pUser->GetRace() << pUser->GetClass() << pUser->GetLevel() << uint32_t(pUser->GetLoyalty())
			<< pUser->GetStat(StatType::STAT_STR) << pUser->GetStat(StatType::STAT_STA) << pUser->GetStat(StatType::STAT_DEX) << pUser->GetStat(StatType::STAT_INT) << pUser->GetStat(StatType::STAT_CHA)
			<< pUser->GetCoins()
			<< pUser->m_sPoints
			<< uint8_t(0) << uint16_t(0)
			<< pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat1] << pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat2] << pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat3] << pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointMaster];

		for (int i = 0; i < SLOT_MAX + HAVE_MAX; i++)
		{
			_ITEM_DATA* pItem = pUser->GetItem(i);
			resultPlayer << pItem->nNum << pItem->sDuration << pItem->sCount << pItem->bFlag;
		}

		resultPlayer << pUser->GetRebirthLevel();
		Send(&resultPlayer);

		/*
		Warrior -> 334,359,365
		Rogue -> 335,347,360,366
		Mage -> 336,348,361,367,516
		Priest -> 337,349,357,362,363,364,368
		*/
		if (true)
		{
#if(__VERSION < 2369)
			Packet result(WIZ_HSACS_HOOK, uint8_t(HSACSXOpCodes::ShowQuestList));
			result << uint16(334) << (pUser->GetQuestStatus(334) == 2 ? uint8(2) : uint8(1));
			result << uint16(359) << (pUser->GetQuestStatus(359) == 2 ? uint8(2) : uint8(1));
			result << uint16(365) << (pUser->GetQuestStatus(365) == 2 ? uint8(2) : uint8(1));
			result << uint16(335) << (pUser->GetQuestStatus(335) == 2 ? uint8(2) : uint8(1));
			result << uint16(347) << (pUser->GetQuestStatus(347) == 2 ? uint8(2) : uint8(1));
			result << uint16(360) << (pUser->GetQuestStatus(360) == 2 ? uint8(2) : uint8(1));
			result << uint16(366) << (pUser->GetQuestStatus(366) == 2 ? uint8(2) : uint8(1));
			result << uint16(336) << (pUser->GetQuestStatus(336) == 2 ? uint8(2) : uint8(1));
			result << uint16(348) << (pUser->GetQuestStatus(348) == 2 ? uint8(2) : uint8(1));
			result << uint16(361) << (pUser->GetQuestStatus(361) == 2 ? uint8(2) : uint8(1));
			result << uint16(367) << (pUser->GetQuestStatus(367) == 2 ? uint8(2) : uint8(1));
			result << uint16(516) << (pUser->GetQuestStatus(516) == 2 ? uint8(2) : uint8(1));
			result << uint16(337) << (pUser->GetQuestStatus(337) == 2 ? uint8(2) : uint8(1));
			result << uint16(349) << (pUser->GetQuestStatus(349) == 2 ? uint8(2) : uint8(1));
			result << uint16(357) << (pUser->GetQuestStatus(357) == 2 ? uint8(2) : uint8(1));
			result << uint16(362) << (pUser->GetQuestStatus(362) == 2 ? uint8(2) : uint8(1));
			result << uint16(363) << (pUser->GetQuestStatus(363) == 2 ? uint8(2) : uint8(1));
			result << uint16(364) << (pUser->GetQuestStatus(364) == 2 ? uint8(2) : uint8(1));
			result << uint16(368) << (pUser->GetQuestStatus(368) == 2 ? uint8(2) : uint8(1));
			for (int i = 0; i < PERK_COUNT; i++)
				result << pUser->pPerks.perkType[i];
			Send(&result);
#endif
		}
		// YOLLAT
	}
	else
	{
		CBot* pBot = g_pMain->GetBotPtr(strCharName, NameType::TYPE_CHARACTER);
		if (pBot != nullptr)
		{
			resultPlayer.SByte();
			resultPlayer << uint8_t(4) << uint8_t(1)
				<< pBot->GetName() << pBot->GetNation() << pBot->GetRace() << pBot->GetClass() << pBot->GetLevel() << pBot->GetLoyalty()
				<< pBot->GetStat(StatType::STAT_STR) << pBot->GetStat(StatType::STAT_STA) << pBot->GetStat(StatType::STAT_DEX) << pBot->GetStat(StatType::STAT_INT) << pBot->GetStat(StatType::STAT_CHA)
				<< pBot->GetCoins()
				<< pBot->m_sPoints
				<< uint8_t(0) << uint16_t(0)
				<< pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat1] << pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat2] << pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat3] << pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointMaster];

			for (int i = 0; i < SLOT_MAX + HAVE_MAX; i++)
			{
				_ITEM_DATA* pItem = pBot->GetItem(i);
				resultPlayer << pItem->nNum << pItem->sDuration << pItem->sCount << pItem->bFlag;
			}

			resultPlayer << pBot->GetRebirthLevel();
			Send(&resultPlayer);


		}
	}
	//Arya v1 End View Info
}

void CUser::HandleGetUserInfoView(Packet& pkt)
{
	uint16 charid;
	pkt >> charid;

	CBot* pBot = g_pMain->GetBotPtr(charid);
	if (pBot != nullptr)
	{
		Packet result(WIZ_ITEM_UPGRADE);
		result << uint8(ITEM_CHARACTER_SEAL) << uint8(CharacterSealOpcodes::Preview) << uint8(0x01);
		result.SByte();
		result << pBot->GetName()
			<< pBot->GetNation()
			<< pBot->GetRace()
			<< pBot->GetClass()
			<< pBot->GetLevel()
			<< pBot->GetLoyalty()
			<< pBot->m_bStats[(uint8)StatType::STAT_STR] << pBot->m_bStats[(uint8)StatType::STAT_STA]
			<< pBot->m_bStats[(uint8)StatType::STAT_DEX] << pBot->m_bStats[(uint8)StatType::STAT_INT] << pBot->m_bStats[(uint8)StatType::STAT_CHA]
			<< pBot->m_iGold
			<< pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointFree]
			<< uint32(0x01)
			<< pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat1] << pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat2]
			<< pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat3] << pBot->m_bstrSkill[(uint8)SkillPointCategory::SkillPointMaster];

		for (int i = 0; i < INVENTORY_COSP; i++)
		{
			auto* pItem = pBot->GetItem(i);
			if (pItem == nullptr)
				continue;

			result << pItem->nNum << pItem->sDuration << pItem->sCount << pItem->bFlag;
		}

		result << pBot->GetRebirthLevel();

		Send(&result);
	}

	auto* pUser = g_pMain->GetUserPtr(charid);
	if (pUser == nullptr)
		return;

	Packet result(WIZ_ITEM_UPGRADE);
	result << uint8(ITEM_CHARACTER_SEAL) << uint8(CharacterSealOpcodes::Preview) << uint8(0x01);
	result.SByte();
	result << pUser->GetName()
		<< pUser->GetNation()
		<< pUser->GetRace()
		<< pUser->GetClass()
		<< pUser->GetLevel()
		<< pUser->GetLoyalty()
		<< pUser->m_bStats[(uint8)StatType::STAT_STR] << pUser->m_bStats[(uint8)StatType::STAT_STA]
		<< pUser->m_bStats[(uint8)StatType::STAT_DEX] << pUser->m_bStats[(uint8)StatType::STAT_INT] << pUser->m_bStats[(uint8)StatType::STAT_CHA]
		<< pUser->m_iGold
		<< pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointFree]
		<< uint32(0x01)
		<< pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat1] << pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat2]
		<< pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointCat3] << pUser->m_bstrSkill[(uint8)SkillPointCategory::SkillPointMaster];

	for (int i = 0; i < INVENTORY_COSP; i++)
	{
		auto* pItem = pUser->GetItem(i);
		if (pItem == nullptr)
			continue;

		result << pItem->nNum << pItem->sDuration << pItem->sCount << pItem->bFlag;
	}

	result << pUser->GetRebirthLevel();

	Send(&result);
}