#include "stdafx.h"
#include "BotHandler.h"

CBot::CBot() :Unit(UnitType::UnitBot)
{
	Initialize();
}

void CBot::Initialize()
{
	Unit::Initialize();
	ReplyChat = "";
	ReplyStatus = 0;
	ReplyTime = 0;
	ReplyID = 0;
	m_sTargetID = -1;
	m_TargetChanged = false;
	echo = -1;
	m_sRegionAttack = false;
	/*m_sMoveTime = */m_sMoveTimeCode = t_timeLastPotionUse = 0;
	MerchantChat.clear();
	LastWarpTime = 0;
	WalkStep = 0;
	/*m_sRegionAttackTime = */m_sMoveRegionAttackTime = 0;
	m_strUserID.clear();
	m_Reverse = false;
	memset(&m_byAPClassBonusAmount, 0, sizeof(m_byAPClassBonusAmount));
	memset(&m_byAcClassBonusAmount, 0, sizeof(m_byAcClassBonusAmount));
	memset(&m_bStats, 0, sizeof(m_bStats));
	memset(&m_sStatItemBonuses, 0, sizeof(m_sStatItemBonuses));
	memset(&m_sStatAchieveBonuses, 0, sizeof(m_sStatAchieveBonuses));
	memset(&m_bStatBuffs, 0, sizeof(m_bStatBuffs));
	memset(&m_bRebStats, 0, sizeof(m_bRebStats));
	memset(&m_bstrSkill, 0, sizeof(m_bstrSkill));
	memset(&m_arMerchantItems, 0, sizeof(m_arMerchantItems));
	memset(&m_sSkillCoolDown, 0, sizeof(m_sSkillCoolDown));

	m_sAchieveCoverTitle = 0;
	m_maxStop = 5000;
	LastMiningCheck = m_sMoveMerchantProcess = 0;
	m_fHPChangeTime = m_fHPType4CheckTime = m_fType4ChangeTime = getMSTime();

	m_bKnightsRank = -1;
	m_bPersonalRank = -1;
	m_bNPGainAmount = 100;

	m_bMerchantState = MERCHANT_STATE_NONE;
	m_sMerchantAreaType = 0;
	m_bMerchantIndex = 0;
	m_bAuthority = 1;
	m_sBind = -1;
	m_state = GameState::GAME_STATE_CONNECTED;
	m_bPartyLeader = false;

	m_bGenieStatus = 0;
	m_sLoginBotType = m_BotState = ChickenStatus = 0;
	m_bIsChicken = m_bWeaponsDisabled = false;
	m_bIsHidingHelmet = false;
	m_bIsHidingCospre = false;
	m_bIsHidingWings = false;

	m_bPremiumMerchant = m_bSellingMerchantPreparing = m_bBuyingMerchantPreparing = false;
	m_bInParty = false;

	m_PlayerKillingLoyaltyDaily = 0;
	m_PlayerKillingLoyaltyPremiumBonus = 0;

	m_bInvisibilityType = m_bRegeneType = INVIS_NONE;
	m_sDirection = 0;

	m_bAuthority = AUTHORITY_PLAYER;
	m_bLevel = m_MoveState = 1;
	m_iLoyalty = 100;
	m_iLoyaltyMonthly = 0;
	m_sHp = m_sMp = m_sSp = MAX_PLAYER_HP;

	m_sRivalID = -1;
	m_tRivalExpiryTime = 0;
	m_byAngerGauge = 0;

	m_bAddWeaponDamage = 0;
	m_bPctArmourAc = m_bPlayerAttackAmount = 100;
	m_sAddArmourAc = 0;
	m_sPartyIndex = -1;

	m_MaxHp = 0;
	m_MaxMp = 1;
	m_MaxSp = 250;
	m_bMerchantViewer = -1;
	m_bResHpType = USER_STANDING;
	m_bBlockPrivateChat = false;
	m_bNeedParty = 0x01;
	m_bAbnormalType = ABNORMAL_NORMAL;	// User starts out in normal size.
	m_nOldAbnormalType = m_bAbnormalType;
	m_teamColour = TeamColourNone;
	m_sSpeed = 0.0f;

	m_tBlinkExpiryTime = m_StartMoveTime = 0;
}

void CBot::GetInOut(Packet& result, uint8 bType)
{
	result.Initialize(WIZ_USER_INOUT);
	result << uint8(bType) << uint8(0) << uint32(GetID());
	if (bType != INOUT_OUT)
		GetUserInfo(result);
}

void CBot::AddToRegion(int16 new_region_x, int16 new_region_z)
{
	if (GetRegion())
		GetRegion()->Remove(this);

	SetRegion(new_region_x, new_region_z);

	if (GetRegion())
		GetRegion()->Add(this);
}

void CBot::BotInOut(uint8 bType)
{
	if (GetRegion() == nullptr)
		return;

	Packet result;
	GetInOut(result, bType);

	if (bType == INOUT_OUT)
		GetRegion()->Remove(this);
	else
		GetRegion()->Add(this);

	SendToRegion(&result);
}

void CBot::UserInOut(uint8 bType)
{
	Packet result;
	GetInOut(result, bType);

	if (bType == INOUT_RESPAWN)
		bType = INOUT_IN;
	else
		m_sLoginBotType = 0;

	if (bType == INOUT_OUT)
	{
		C3DMap* pMap = g_pMain->GetZoneByID(GetZoneID());
		if (pMap == nullptr)
			return;

		if (pMap->GetID() == ZONE_ARDREAM
			|| pMap->GetID() == ZONE_RONARK_LAND
			|| pMap->GetID() == ZONE_BORDER_DEFENSE_WAR)
		{
			m_iLoyaltyDaily = 0; m_iLoyaltyPremiumBonus = 0;

			for (int nation = KARUS_ARRAY; nation <= ELMORAD_ARRAY; nation++)
				g_pMain->m_UserPlayerKillingZoneRankingArray[nation].DeleteData(GetID());
		}
		else if (pMap->GetID() == ZONE_CHAOS_DUNGEON)
		{
			g_pMain->m_UserChaosExpansionRankingArray.DeleteData(GetID());
		}

		TRACE("%s Bot has Out the Game RemovePlayer Ranked ZoneID %d\n", GetName().c_str(), GetZoneID());
		BotsSurroundingUserRegionUpdate();

		if (GetRegion())
			GetRegion()->Remove(this);

		m_state = GameState::GAME_STATE_CONNECTED;
		g_DBAgent.RemoveCurrentUser(GetName());
		g_DBAgent.UpdateBotUser(this);
	}
	else
	{
		if (GetRegion())
			GetRegion()->Add(this);

		m_state = GameState::GAME_STATE_INGAME;
		g_DBAgent.InsertCurrentUser(GetName(), GetName());
		g_DBAgent.GetLoadBotUser(this);
		g_pMain->GetBotRank(this);
		m_tGameStartTimeSavedMagic = UNIXTIME;
	}
	SendToRegion(&result);
}

int8 CBot::GetLoyaltySymbolRank()
{
	if (m_bPersonalRank > 100 && m_bPersonalRank <= 200
		|| m_bKnightsRank > 100 && m_bKnightsRank <= 200)
		return -1;

	return m_bKnightsRank <= m_bPersonalRank ? m_bKnightsRank : m_bPersonalRank <= m_bKnightsRank ? m_bPersonalRank : -1;
}



uint8 CBot::GetSymbol()
{
	uint8 bRaking = 0, m_sPersonalRank = m_bPersonalRank, m_sKnightsRank = m_bKnightsRank;
	if (m_sPersonalRank < 1 && m_sKnightsRank < 1)
		return 0;

	if (m_sPersonalRank < m_sKnightsRank) {
		if (m_sPersonalRank == 1)
			bRaking = 30;
		else if (m_sPersonalRank > 1 && m_sPersonalRank <= 4)
			bRaking = 31;
		else if (m_sPersonalRank > 4 && m_sPersonalRank <= 10)
			bRaking = 32;
		else if (m_sPersonalRank > 10 && m_sPersonalRank <= 40)
			bRaking = 33;
		else if (m_sPersonalRank > 40 && m_sPersonalRank <= 100)
			bRaking = 34;
		else if (m_sPersonalRank > 100 && m_sPersonalRank <= 200)
			bRaking = 35;
	}
	else if (m_sPersonalRank > m_sKnightsRank) {
		if (m_sKnightsRank == 1)
			bRaking = 20;
		else if (m_sKnightsRank > 1 && m_sKnightsRank <= 4)
			bRaking = 21;
		else if (m_sKnightsRank > 4 && m_sKnightsRank <= 10)
			bRaking = 22;
		else if (m_sKnightsRank > 10 && m_sKnightsRank <= 40)
			bRaking = 23;
		else if (m_sKnightsRank > 40 && m_sKnightsRank <= 100)
			bRaking = 24;
		else if (m_sKnightsRank > 100 && m_sKnightsRank <= 200)
			bRaking = 25;
	}
	else if (m_sPersonalRank == m_sKnightsRank) {
		if (m_sKnightsRank == 1)
			bRaking = 20;
		else if (m_sKnightsRank > 1 && m_sKnightsRank <= 4)
			bRaking = 21;
		else if (m_sKnightsRank > 4 && m_sKnightsRank <= 10)
			bRaking = 22;
		else if (m_sKnightsRank > 10 && m_sKnightsRank <= 40)
			bRaking = 23;
		else if (m_sKnightsRank > 40 && m_sKnightsRank <= 100)
			bRaking = 24;
		else if (m_sKnightsRank > 100 && m_sKnightsRank <= 200)
			bRaking = 25;
	}

	return bRaking;
}

void CBot::BotsSurroundingUserRegionUpdate()
{
	Packet result(WIZ_USER_INFORMATIN, uint8(UserInfoShow));
	result.SByte();
	result << GetName();
	g_pMain->Send_Zone(&result, GetZoneID());
}
#if 0
void CBot::GetUserInfo(Packet& pkt)
{
	pkt.SByte();
	pkt << GetName() << GetNation();
	pkt << uint8(0) << uint8(0);
	pkt << GetClanID() << uint8(isInPKZone() == true ? uint8(0) : GetFame());

	CKnights* pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		if (isKing())
			pkt << uint32(0) << uint16(0) << uint8(0) << uint16(GetNation() == KARUS ? 97 : 98) << uint32(0) << uint8(0);
		else
			pkt << uint32(0) << uint16(0) << uint8(0) << uint16(-1) << uint32(0) << uint8(0);
	}
	else
	{
		pkt << pKnights->GetAllianceID() << pKnights->GetName() << pKnights->m_byGrade << pKnights->m_byRanking << pKnights->m_sMarkVersion; // symbol/mark version

		CKnights* pMainClan = g_pMain->GetClanPtr(pKnights->GetAllianceID());
		_KNIGHTS_ALLIANCE* pAlliance = g_pMain->GetAlliancePtr(pKnights->GetAllianceID());

		if (pKnights->isInAlliance()
			&& pMainClan != nullptr
			&& pAlliance != nullptr)
		{
			if (!isKing())
			{
				if (pMainClan->isCastellanCape() == false)
				{
					if (pAlliance->sMainAllianceKnights == pKnights->GetID())
						pkt << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << uint8(0);
					else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
						pkt << pMainClan->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0);
					else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
						pkt << pMainClan->GetCapeID() << uint32(0); // only the cape will be present
					else
						pkt << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << uint8(0); // this is stored in 4 bytes after all.
				}
				else
				{
					if (pMainClan->m_sCastTime >= uint32(UNIXTIME))
					{
						if (pAlliance->sMainAllianceKnights == pKnights->GetID())
							pkt << pMainClan->m_sCastCapeID << pMainClan->m_bCastCapeR << pMainClan->m_bCastCapeG << pMainClan->m_bCastCapeB << uint8(0);
						else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
							pkt << pMainClan->m_sCastCapeID << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0);
						else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
							pkt << pMainClan->m_sCastCapeID << uint32(0); // only the cape will be present
						else
							pkt << pMainClan->m_sCastCapeID << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << uint8(0); // this is stored in 4 bytes after all.
					}
					else
					{
						if (pAlliance->sMainAllianceKnights == pKnights->GetID())
							pkt << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << uint8(0);
						else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
							pkt << pMainClan->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0);
						else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
							pkt << pMainClan->GetCapeID() << uint32(0); // only the cape will be present
						else
							pkt << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << uint8(0); // this is stored in 4 bytes after all.
					}
				}
			}
			else
				pkt << uint16(GetNation() == KARUS ? 97 : 98) << uint32(0); // cape ID

			// not sure what this is, but it (just?) enables the clan symbol on the cape 
			// value in dump was 9, but everything tested seems to behave as equally well...
			// we'll probably have to implement logic to respect requirements.
			pkt << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
		}
		else
		{
			if (!isKing())
			{
				if (pKnights->isCastellanCape() == false)
					pkt << pKnights->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0); // this is stored in 4 bytes after all.
				else
				{
					if (pKnights->m_sCastTime >= uint32(UNIXTIME))
						pkt << pKnights->m_sCastCapeID << pKnights->m_bCastCapeR << pKnights->m_bCastCapeG << pKnights->m_bCastCapeB << uint8(0); // this is stored in 4 bytes after all.
					else
						pkt << pKnights->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0); // this is stored in 4 bytes after all.
				}
			}
			else
				pkt << uint16(GetNation() == KARUS ? 97 : 98) << uint32(0); // cape ID

			// not sure what this is, but it (just?) enables the clan symbol on the cape 
			// value in dump was 9, but everything tested seems to behave as equally well...
			// we'll probably have to implement logic to respect requirements.
			pkt << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
		}
	}

	// There are two event-driven invisibility states; dispel on attack, and dispel on move.
	// These are handled primarily server-side; from memory the client only cares about value 1 (which we class as 'dispel on move').
	// As this is the only place where this flag is actually sent to the client, we'll just convert 'dispel on attack' 
	// back to 'dispel on move' as the client expects.
	uint8 bInvisibilityType = m_bInvisibilityType;
	if (bInvisibilityType != (uint8)InvisibilityType::INVIS_NONE)
		bInvisibilityType = (uint8)InvisibilityType::INVIS_DISPEL_ON_MOVE;

	bool sAsistTagSystem = false;
	pkt << GetLevel() << m_bRace << m_sClass
		<< GetSPosX() << GetSPosZ() << GetSPosY()
		<< m_bFace << m_nHair
		<< m_bResHpType << uint32(m_bAbnormalType)
		<< m_bNeedParty
		<< m_bAuthority
		<< m_bPartyLeader
		<< bInvisibilityType
		<< uint8(m_teamColour)
		<< m_bIsDevil
		<< uint8_t(0x00)
		<< m_sDirection
		<< m_bIsChicken
		<< m_bRank;

	if (sAsistTagSystem)
		pkt << uint8(0) << uint8(0);
	else
		pkt << uint8(0) << uint8(0);

	if (GetLevel() < 30 && !ChickenStatus && GAME_SOURCE_VERSION == 1098)		// V2 için burasý aktif olucak.
		pkt << int8(50) << int8(250);
	else if (GetLevel() < 30 && !ChickenStatus && GAME_SOURCE_VERSION == 1534)		// V2 için burasý aktif olucak.
		pkt << int8(50) << int8(250);
	else
		pkt << (m_bKnightsRank <= m_bPersonalRank ? m_bKnightsRank : int8(-1)) << (m_bPersonalRank <= m_bKnightsRank ? m_bPersonalRank : int8(-1));

	uint8 equippedItems[] =
	{
		BREAST, LEG, HEAD, GLOVE, FOOT, SHOULDER, RIGHTHAND, LEFTHAND,
		CWING, CHELMET, CLEFT, CRIGHT, CTOP, CFAIRY, CTATTOO
	};

	bool isWarOpen = (g_pMain->m_byBattleOpen == NATION_BATTLE && g_pMain->m_byBattleZone + ZONE_BATTLE_BASE != ZONE_BATTLE3);
	_ITEM_DATA* pItem = nullptr;

	foreach_array(i, equippedItems)
	{
		pItem = GetItem(equippedItems[i]);
		if (pItem == nullptr)
			continue;

		if (isWarOpen)
		{
			if (isWarrior())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isRogue())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isMage())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)MAGE_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)MAGE_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)MAGE_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)MAGE_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)MAGE_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isPriest())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isPortuKurian())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
		}
		else
		{
			pItem = GetItem(equippedItems[i]);
			if (pItem == nullptr)
				continue;

			pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
		}
	}
	pkt << GetZoneID() << int16_t(-1) << uint8_t(0x00) << uint32_t(-1) << m_bIsHidingHelmet << m_bIsHidingCospre << uint8_t(0x00) << GetRebirthLevel() << uint16_t(0x00) << uint32_t(0x00) << uint8_t(0x00) << uint32_t(0x00/*FaceID*/) << uint8_t(0x00);
}
#else
void CBot::GetUserInfo(Packet& pkt)
{
	pkt.SByte();
	pkt << GetName() << GetNation();
	pkt << uint8(0) << uint8(0) << uint8(0);
	pkt << GetClanID() << uint8(isInPKZone() == true ? uint8(0) : GetFame());

	CKnights* pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		if (isKing())
			pkt << uint32(0) << uint16(0) << uint8(0) << uint16(GetNation() == (uint8)Nation::KARUS ? 97 : 98) << uint32(0) << uint8(0);
		else
			pkt << uint32(0) << uint16(0) << uint8(0) << uint16(-1) << uint32(0) << uint8(0);
	}
	else
	{
		pkt << pKnights->GetAllianceID() << pKnights->GetName() << pKnights->m_byGrade << pKnights->m_byRanking << pKnights->m_sMarkVersion; // symbol/mark version

		CKnights* pMainClan = g_pMain->GetClanPtr(pKnights->GetAllianceID());
		_KNIGHTS_ALLIANCE* pAlliance = g_pMain->GetAlliancePtr(pKnights->GetAllianceID());

		if (pKnights->isInAlliance() && pMainClan != nullptr && pAlliance != nullptr)
		{
			if (!isKing())
			{
				if (pAlliance->sMainAllianceKnights == pKnights->GetID())
					pkt << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << uint8(0);
				else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
					pkt << pMainClan->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0);
				else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
					pkt << pMainClan->GetCapeID() << uint32(0); // only the cape will be present
				else
					pkt << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << uint8(0); // this is stored in 4 bytes after all.
			}
			else
				pkt << uint16(GetNation() == (uint8)Nation::KARUS ? 97 : 98) << uint32(0); // cape ID

			// not sure what this is, but it (just?) enables the clan symbol on the cape 
			// value in dump was 9, but everything tested seems to behave as equally well...
			// we'll probably have to implement logic to respect requirements.
			pkt << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
		}
		else
		{
			if (!isKing())
				pkt << pKnights->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << uint8(0); // this is stored in 4 bytes after all.
			else
				pkt << uint16(GetNation() == (uint8)Nation::KARUS ? 97 : 98) << uint32(0); // cape ID

			// not sure what this is, but it (just?) enables the clan symbol on the cape 
			// value in dump was 9, but everything tested seems to behave as equally well...
			// we'll probably have to implement logic to respect requirements.
			pkt << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
		}
	}

	// There are two event-driven invisibility states; dispel on attack, and dispel on move.
	// These are handled primarily server-side; from memory the client only cares about value 1 (which we class as 'dispel on move').
	// As this is the only place where this flag is actually sent to the client, we'll just convert 'dispel on attack' 
	// back to 'dispel on move' as the client expects.
	uint8 bInvisibilityType = m_bInvisibilityType;
	if (bInvisibilityType != (uint8)InvisibilityType::INVIS_NONE)
		bInvisibilityType = (uint8)InvisibilityType::INVIS_DISPEL_ON_MOVE;

	bool sAsistTagSystem = false;
	pkt << GetLevel() 
		<< m_bRace 
		<< m_sClass
		<< GetSPosX() 
		<< GetSPosZ() 
		<< GetSPosY()
		<< m_bFace 
		<< m_nHair
		<< m_bResHpType
		<< uint32(m_bAbnormalType)
		<< m_bNeedParty
		<< m_bAuthority
		<< m_bPartyLeader
		<< bInvisibilityType
		<< uint8(m_teamColour)
		<< m_bIsDevil
		<< uint8_t(0x00)
		<< m_sDirection
		<< m_bIsChicken
		<< m_bRank;

	if (sAsistTagSystem)
		pkt << uint8(0) << uint8(0);
	else
		pkt << uint8(0) << uint8(0);

	if (GetLevel() < 30 && !ChickenStatus && GAME_SOURCE_VERSION == 1098)		// V2 için burasý aktif olucak.
		pkt << int8(50) << int8(250);
	else if (GetLevel() < 30 && !ChickenStatus && GAME_SOURCE_VERSION == 1534)		// V2 için burasý aktif olucak.
		pkt << int8(50) << int8(250);
	else
		pkt << (m_bKnightsRank <= m_bPersonalRank ? m_bKnightsRank : int8(-1)) << (m_bPersonalRank <= m_bKnightsRank ? m_bPersonalRank : int8(-1));

	uint8 equippedItems[] =
	{
		BREAST, LEG, HEAD, GLOVE, FOOT, SHOULDER, RIGHTHAND, LEFTHAND,
		CWING, CHELMET, CLEFT, CRIGHT, CTOP, CTATTOO, CFAIRY, CEMBLEM, CTALISMAN
	};

	bool isWarOpen = (g_pMain->m_byBattleOpen == NATION_BATTLE && g_pMain->m_byBattleZone + ZONE_BATTLE_BASE != ZONE_BATTLE3);
	_ITEM_DATA* pItem = nullptr;

	foreach_array(i, equippedItems)
	{
		pItem = GetItem(equippedItems[i]);
		if (pItem == nullptr)
			continue;

		if (isWarOpen)
		{
			if (isWarrior())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isRogue())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)ROGUE_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isMage())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)MAGE_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)MAGE_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)MAGE_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)MAGE_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)MAGE_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isPriest())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)PRIEST_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
			else if (isPortuKurian())
			{
				if (i == RIGHTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAULDRON << pItem->sDuration << pItem->bFlag;
				else if (i == HEAD)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_PAD << pItem->sDuration << pItem->bFlag;
				else if (i == LEFTEAR)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_HELMET << pItem->sDuration << pItem->bFlag;
				else if (i == NECK)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_GAUNTLET << pItem->sDuration << pItem->bFlag;
				else if (i == BREAST)
					pkt << (uint32)WARRIOR_DRAGON_ARMOR_BOOTS << pItem->sDuration << pItem->bFlag;
				else
					pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
			}
		}
		else
		{
			pItem = GetItem(equippedItems[i]);
			if (pItem == nullptr)
				continue;

			pkt << pItem->nNum << pItem->sDuration << pItem->bFlag;
		}
	}
	pkt << uint16(GetZoneID()) << int32(-1) << uint8(0) << uint32(0) << m_bIsHidingHelmet << m_bIsHidingCospre << uint8(0) << GetRebirthLevel() << uint16(0) << uint32(0) << uint8(0) << uint32(0) << uint8(0);
}
#endif

bool CBot::JobGroupCheck(short jobgroupid)
{
	if (jobgroupid > 100)
		return GetClass() == jobgroupid;

	ClassType subClass = GetBaseClassType();
	switch (jobgroupid)
	{
	case GROUP_WARRIOR: return (subClass == ClassWarrior || subClass == ClassWarriorNovice || subClass == ClassWarriorMaster);
	case GROUP_PORTU_KURIAN: return (subClass == ClassPortuKurian || subClass == ClassPortuKurianNovice || subClass == ClassPortuKurianMaster);
	case GROUP_ROGUE: return (subClass == ClassRogue || subClass == ClassRogueNovice || subClass == ClassRogueMaster);
	case GROUP_MAGE: return (subClass == ClassMage || subClass == ClassMageNovice || subClass == ClassMageMaster);
	case GROUP_CLERIC: return (subClass == ClassPriest || subClass == ClassPriestNovice || subClass == ClassPriestMaster);
	}
	return (subClass == jobgroupid);
}

void CBot::ShowEffect(uint32 nSkillID)
{
	Packet result(WIZ_EFFECT);
	result << uint32(GetID()) << nSkillID;
	SendToRegion(&result);
}

/**
* @brief	Recasts any lockable scrolls on debuff.
*/
void CBot::RecastLockableScrolls(uint8 buffType)
{
	InitType4(false, buffType);
	RecastSavedMagic(buffType);
}

void CBot::RecastSavedMagic(uint8 buffType /* = 0*/)
{
	m_savedMagicLock.lock();
	UserSavedMagicMap castSet;
	foreach(itr, m_savedMagicMap)
	{
		if (itr->first != 0 || itr->second != 0)
			castSet.insert(std::make_pair(itr->first, itr->second));
	}
	m_savedMagicLock.unlock();

	if (castSet.empty())
		return;

	foreach(itr, castSet)
	{
		if (buffType > 0)
		{
			_MAGIC_TYPE4* pType = g_pMain->m_Magictype4Array.GetData(itr->first);

			if (pType == nullptr)
				continue;

			if (pType->bBuffType != buffType)
				continue;
		}

		if (isSiegeTransformation())
			continue;

		MagicInstance instance;
		instance.sCasterID = GetID();
		instance.sTargetID = GetID();
		instance.nSkillID = itr->first;
		instance.sSkillCasterZoneID = GetZoneID();
		instance.bIsRecastingSavedMagic = true;
		instance.Run();
	}
}

void CBot::CheckSavedMagic()
{
	Guard lock(m_savedMagicLock);
	if (m_savedMagicMap.empty()) return;
	std::set<uint32> deleteSet;
	foreach(itr, m_savedMagicMap)
	{
		if (itr->second <= UNIXTIME2) deleteSet.insert(itr->first);
	} foreach(itr, deleteSet) m_savedMagicMap.erase(*itr);
}

void CBot::Type4Duration()
{
	std::vector<uint8> willDel;
	m_buffLock.lock();
	foreach(itr, m_buffMap) {
		if (itr->second.m_tEndTime > UNIXTIME)
			continue;

		willDel.push_back(itr->first);
	}
	m_buffLock.unlock();
	foreach(itr, willDel)
		CMagicProcess::RemoveType4Buff((*itr), this, true, isLockableScroll((*itr)));

	if (!isDebuffed() && !m_bType3Flag)
		SendUserStatusUpdate(UserStatus::USER_STATUS_POISON, UserStatusBehaviour::USER_STATUS_CURE);
}

void CBot::Type4Change()
{
	m_fType4ChangeTime = getMSTime();

	if (isDead())
		return;

	MagicPacket(MAGIC_EFFECTING, 500048, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
	MagicPacket(MAGIC_EFFECTING, 500049, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
	MagicPacket(MAGIC_EFFECTING, 500050, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
	MagicPacket(MAGIC_EFFECTING, 500054, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
	MagicPacket(MAGIC_EFFECTING, 492018, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
	MagicPacket(MAGIC_EFFECTING, 491014, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());

	if (isRogue())
		MagicPacket(MAGIC_EFFECTING, 500042, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
	else if (isMage())
		MagicPacket(MAGIC_EFFECTING, 500040, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
	else if (isPriest())
	{
		MagicPacket(MAGIC_EFFECTING, 500040, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
		MagicPacket(MAGIC_EFFECTING, 212020, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
	}
	else
		MagicPacket(MAGIC_EFFECTING, 500040, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
}

void CBot::BotMining()
{
	if (LastMiningCheck + (2 * MINUTE) > UNIXTIME)
		return;

	LastMiningCheck = UNIXTIME;

	Packet result(WIZ_MINING, uint8(MiningAttempt));
	uint16 resultCode = MiningResultSuccess, Random = myrand(0, 10000);
	uint16 sEffect = 0;

	if (Random > 4000)
		sEffect = 13082; // "XP" effect
	else
		sEffect = 13081; // "Item" effect

	result << resultCode << uint32(GetID()) << sEffect;
	SendToRegion(&result);
}

void CBot::BotFishing()
{
	if (LastMiningCheck + (2 * MINUTE) > UNIXTIME)
		return;

	LastMiningCheck = UNIXTIME;

	Packet result(WIZ_MINING, uint8(FishingAttempt));
	uint16 resultCode = MiningResultSuccess, Random = myrand(0, 10000);
	uint16 sEffect = 0;

	if (Random > 4000)
		sEffect = 13082; // "XP" effect
	else
		sEffect = 13081; // "Item" effect

	result << resultCode << uint32(GetID()) << sEffect;
	SendToRegion(&result);
}

void CBot::BotMerchant()
{
	if (LastMiningCheck + (1 * MINUTE) > UNIXTIME)
		return;

	LastMiningCheck = UNIXTIME;

	if (MerchantChat.empty())
		return;

	Packet result(WIZ_CHAT);
	ChatPacket::Construct(&result, MERCHANT_CHAT, &MerchantChat, &GetName(), GetNation(), GetID());
	SendToRegion(&result);
}

void CBot::FindMonsterAttackSlot()
{
	if (isDead())
		return;

	CNpc* pNpc = nullptr;
	KOMap* pMap = GetMap();
	if (pMap == nullptr)
		return;

	foreach_region(rx, rz)
		FindMonsterAttack(rx + GetRegionX(), rz + GetRegionZ(), pMap);
}

void CBot::FindNewMonsterAttackSlot()
{
	if (isDead())
		return;

	CNpc* pNpc = nullptr;
	KOMap* pMap = GetMap();
	if (pMap == nullptr)
		return;

	foreach_region(rx, rz)
		FindNewMonsterAttack(rx + GetRegionX(), rz + GetRegionZ(), pMap);
}

void CBot::FindNewMonsterAttack(int x, int z, C3DMap* pMap)
{
	if (x < 0 
		|| z < 0 
		|| x > pMap->GetXRegionMax() 
		|| z > pMap->GetZRegionMax())
		return;

	std::vector<Unit*> casted_member;
	std::vector<uint16> unitList;
	g_pMain->GetUnitListFromSurroundingRegions(this, &unitList);

	foreach(itr, unitList)
	{
		Unit* pTarget = g_pMain->GetUnitPtr(*itr, GetZoneID());

		if (pTarget == nullptr)
			continue;

		if (this != pTarget
			&& !pTarget->isDead()
			&& !pTarget->isBlinking()
			&& pTarget->isAttackable())
			casted_member.push_back(pTarget);
	}

	if (pMap == nullptr)
		return;

	//int iValue = 0;
	//float UnitX, UnitY, UnitZ;

	//CRegion* pRegion = pMap->GetRegion(x, z);
	//if (pRegion == nullptr)
	//	return;

	//int16 sTargetID = -1;
	//std::vector<uint16> willDel;
	//__Vector3 vBot, vpUnit, vDistance, vRealDistance;

	//foreach(itr, casted_member)
	//{
	//	Unit* pTarget = *itr; // it's checked above, not much need to check it again
	//	float fSearchRange = 25.0f;

	//	if (pTarget == nullptr)
	//		continue;

	//	if (pTarget->isDead()
	//		|| pTarget->isPlayer() && TO_USER(pTarget)->GetNation() == GetNation()
	//		|| pTarget->isPlayer() && !TO_USER(pTarget)->isInPKZone()
	//		|| pTarget->GetZoneID() != GetZoneID()
	//		|| pTarget->isNPC() && !TO_NPC(pTarget)->isMonster()
	//		|| pTarget->isPlayer() && TO_USER(pTarget)->isGM()
	//		|| pTarget->isBot() && TO_BOT(pTarget)->GetNation() == GetNation()
	//		|| pTarget->isBot() && !TO_BOT(pTarget)->isInPKZone())
	//		continue;

	//	float fDis = GetDistanceSqrt(pTarget);
	//	if (fDis > fSearchRange)
	//		continue;

	//	if (sTargetID > -1)
	//		continue;

	//	vBot.Set(GetX(), pTarget->GetY(), GetZ());
	//	vpUnit.Set(pTarget->GetX() + ((myrand(0, 2000) - 1000.0f) / 500.0f), pTarget->GetY(), pTarget->GetZ() + ((myrand(0, 2000) - 1000.0f) / 500.0f));
	//	sTargetID = pTarget->GetID();
	//	UnitX = pTarget->GetX();
	//	UnitZ = pTarget->GetZ();
	//	UnitY = pTarget->GetY();
	//}

	//if (sTargetID == int16(-1)
	//	|| UnitX == 0
	//	//|| UnitY == 0
	//	|| UnitZ == 0)
	//{
	//	//m_sRegionAttackTime = ULONGLONG(UNIXTIME2 + (5 * SECOND));
	//	return;
	//}

	//if (m_sTargetID != sTargetID)
	//{
	//	m_TargetChanged = true;
	//	m_sTargetID = sTargetID;
	//	m_oldx = vpUnit.x + ((myrand(0, 2000) - 1000.0f) / 500.0f);
	//	m_oldz = vpUnit.z + ((myrand(0, 2000) - 1000.0f) / 500.0f);
	//	m_oldy = vpUnit.y;
	//}

	//vDistance = vpUnit - vBot;
	//vRealDistance = vDistance;
	//vDistance.Normalize();

	//float sSpeed = m_sSpeed;
	//uint8 sRunTime = 1;
	//bool sRunFinish = false;
	//vDistance *= sSpeed / 10.0f;

	//if (echo == uint8(0)
	//	&& vDistance.Magnitude() < vRealDistance.Magnitude()
	//	&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	//{
	//	vDistance *= 2.0f;
	//	sRunTime = 2;
	//}
	//else if (vDistance.Magnitude() > vRealDistance.Magnitude()
	//	|| vDistance.Magnitude() == vRealDistance.Magnitude())
	//{
	//	sRunFinish = true;
	//	vDistance = vRealDistance;
	//}

	//if (m_TargetChanged)
	//{
	//	m_TargetChanged = false;
	//	echo = uint8(1);
	//	//m_sRegionAttackTime = ULONGLONG(UNIXTIME2 + (sRunTime * SECOND));
	//}
	//else if (sRunFinish)
	//{
	//	echo = uint8(0);
	//	//m_sRegionAttackTime = ULONGLONG(UNIXTIME2 + (myrand(1, m_maxStop) * SECOND));
	//}
	//else
	//{
	//	echo = uint8(3);
	//	//m_sRegionAttackTime = ULONGLONG(UNIXTIME2 + (sRunTime * SECOND));
	//}

	//uint16 will_x, will_z, will_y;
	//will_x = uint16((vBot + vDistance).x * 10.0f);
	//will_y = uint16(vpUnit.y * 10.0f);
	//will_z = uint16((vBot + vDistance).z * 10.0f);
	////m_sRegionAttackTime = ULONGLONG(UNIXTIME2 + (2 * SECOND));
	float fDis = 0.0f, fSearchRange = 30;
	int iValue = 0;

	float monster_x, monster_y, monster_z;

	CRegion* pRegion = pMap->GetRegion(x, z);
	if (pRegion == nullptr)
	return;

	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	uint16 BestTargetID = 0;
	__Vector3 vBot, vUser, vDistance, vRealDistance;
	std::vector<uint16> willDel;
	foreach(itr, casted_member)
	{
		Unit* pTarget = *itr; // it's checked above, not much need to check it again

		if (pTarget == nullptr)
			continue;

		if (pTarget->isDead()
			|| pTarget->isPlayer() && TO_USER(pTarget)->GetNation() == GetNation()
			|| pTarget->isPlayer() && !TO_USER(pTarget)->isInPKZone()
			|| pTarget->GetZoneID() != GetZoneID()
			|| pTarget->isNPC() && !TO_NPC(pTarget)->isMonster()
			|| pTarget->isPlayer() && TO_USER(pTarget)->isGM()
			|| pTarget->isBot() && TO_BOT(pTarget)->GetNation() == GetNation()
			|| pTarget->isBot() && !TO_BOT(pTarget)->isInPKZone())
			continue;

		if (BestTargetID > 0)
			continue;

		if (pow(GetX() - pTarget->GetX(), 2.0f) + pow(GetZ() - pTarget->GetZ(), 2.0f) > 400)
			continue;

		Mesafe = pow(pTarget->GetX() + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(pTarget->GetZ() + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);

		if (EnYakinMesafe != 0.0f && Mesafe > EnYakinMesafe)
			continue;

		monster_x = pTarget->GetX();
		monster_z = pTarget->GetZ();
		monster_y = pTarget->GetY();

		vBot.Set(GetX(), pTarget->GetY(), GetZ());
		vUser.Set(pTarget->GetX() + ((myrand(0, 2000) - 1000.0f) / 500.0f), pTarget->GetY(), pTarget->GetZ() + ((myrand(0, 2000) - 1000.0f) / 500.0f));

		BestTargetID = pTarget->GetID();
		EnYakinMesafe = Mesafe;
	}

	if (BestTargetID == uint16(0))
		return;

	if (m_sTargetID != BestTargetID)
	{
		m_TargetChanged = true;
		m_sTargetID = BestTargetID;
		/*m_Tcurx = vUser.x + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_Tcurz = vUser.z + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_Tcury = vUser.y;*/
	}

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float sSpeed = 45.0f;
	uint8 KosuSuresi = 1;
	bool KosuBitirme = false;
	vDistance *= sSpeed / 10.0f;

	if (echo == uint8(0) // Duruyorsa
		&& vDistance.Magnitude() < vRealDistance.Magnitude() // Gerçek mesafe tek saniyelik koþu mesafesinden büyükse
		&& (vDistance * EnYakinMesafe).Magnitude() < vRealDistance.Magnitude())// Gerçek mesafe EnYakinMesafe koþu mesafesinden büyükse
	{
		vDistance *= EnYakinMesafe;// EnYakinMesafe koþu yaptýralým
		KosuSuresi = (uint8)EnYakinMesafe;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude() // Gerçek mesafe koþu mesafesinden küçükse
		|| vDistance.Magnitude() == vRealDistance.Magnitude())// Gerçek mesafe koþu mesafesine eþitse
	{
		KosuBitirme = true;
		vDistance = vRealDistance;
	}

	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);

	if (isRogue())
		GetAssasinDamageMagic(m_sTargetID, (vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, sSpeed, echo);
	else if (isWarrior())
		GetWarriorDamageMagic(m_sTargetID, (vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, sSpeed, echo);
	else if (isMage())
	{
		int nRandom = myrand(1, 3);
		switch (nRandom)
		{
		case 1:
			GetFlameMageDamageMagic(m_sTargetID, (vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, sSpeed, echo);
			break;
		case 2:
			GetGlacierMageDamageMagic(m_sTargetID, (vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, sSpeed, echo);
			break;
		default:
			GetLightningMageDamageMagic(m_sTargetID, (vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, sSpeed, echo);
			break;
		}
	}
	else if (isPriest())
		GetPriestDamageMagic(m_sTargetID, (vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, sSpeed, echo);
}

void CBot::FindMonsterAttack(int x, int z, C3DMap* pMap)
{
	std::vector<Unit*> casted_member;
	std::vector<uint16> unitList;
	g_pMain->GetUnitListFromSurroundingRegions(this, &unitList);

	FilterValidTargets(unitList, casted_member);

	if (pMap == nullptr)
		return;

	Unit* pSelectedTarget = SelectTarget(casted_member);

	if (pSelectedTarget == nullptr)
		return;

	AttackTarget(pSelectedTarget);
}

void CBot::FilterValidTargets(const std::vector<uint16>& unitList, std::vector<Unit*>& casted_member)
{
	foreach(itr, unitList)
	{
		Unit* pTarget = g_pMain->GetUnitPtr(*itr, GetZoneID());

		if (IsFarmerValidTarget(pTarget))
			casted_member.push_back(pTarget);
	}
}

bool CBot::IsFarmerValidTarget(Unit* pTarget)
{
	if (!pTarget)
		return false;

	if (pTarget->isDead() 
		|| (pTarget->isPlayer() && (TO_USER(pTarget)->GetNation() == GetNation() || !TO_USER(pTarget)->isInPKZone())) 
		|| (pTarget->GetZoneID() != GetZoneID()) 
		|| (pTarget->isNPC() && !TO_NPC(pTarget)->isMonster()) 
		|| (pTarget->isPlayer() && TO_USER(pTarget)->isGM()) 
		|| (pTarget->isBot() && (TO_BOT(pTarget)->GetNation() == GetNation() || !TO_BOT(pTarget)->isInPKZone())))
		return false;
	return true;
}

Unit* CBot::SelectTarget(const std::vector<Unit*>& casted_member)
{
	float fSearchRange = 25.0f;
	int16 sTargetID = -1;

	foreach(itr, casted_member)
	{
		Unit* pTarget = *itr;
		float fDis = GetDistanceSqrt(pTarget);

		if (fDis <= fSearchRange && sTargetID == -1)
		{
			sTargetID = pTarget->GetID();
		}
	}

	return (sTargetID != int16(-1)) ? g_pMain->GetUnitPtr(sTargetID, GetZoneID()) : nullptr;
}

void CBot::AttackTarget(Unit* pTarget)
{
	__Vector3 vBot, vpUnit, vDistance, vRealDistance;
	float sSpeed = 45.0f;
	uint8 sRunTime = 1;
	bool sRunFinish = false;

	SetTargetVariables(pTarget, vBot, vpUnit);

	ComputeDistance(vBot, vpUnit, vDistance, vRealDistance, sSpeed, sRunTime, sRunFinish);

	HandleAttackLogic(pTarget, vBot, vpUnit, vDistance, vRealDistance, sSpeed, sRunTime, sRunFinish);
}

void CBot::SetTargetVariables(Unit* pTarget, __Vector3& vBot, __Vector3& vpUnit)
{
	vBot.Set(GetX(), pTarget->GetY(), GetZ());
	vpUnit.Set(pTarget->GetX() + ((myrand(0, 2000) - 1000.0f) / 500.0f), pTarget->GetY(), pTarget->GetZ() + ((myrand(0, 2000) - 1000.0f) / 500.0f));
}

void CBot::ComputeDistance(const __Vector3& vBot, const __Vector3& vpUnit, __Vector3& vDistance, __Vector3& vRealDistance, float sSpeed, uint8& sRunTime, bool& sRunFinish)
{
	vDistance = vpUnit - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	vDistance *= sSpeed / 10.0f;
	
	if (echo == uint8(0) && vDistance.Magnitude() < vRealDistance.Magnitude() && (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 2.0f;
		sRunTime = 2;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude() || vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		sRunFinish = true;
		vDistance = vRealDistance;
	}
}

void CBot::HandleAttackLogic(Unit* pTarget, const __Vector3& vBot, const __Vector3& vpUnit, const __Vector3& vDistance, const __Vector3& vRealDistance, float sSpeed, uint8 sRunTime, bool sRunFinish)
{
	if (m_sTargetID != pTarget->GetID())
	{
		m_TargetChanged = true;
		m_sTargetID = pTarget->GetID();
		/*m_oldx = vpUnit.x + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_oldz = vpUnit.z + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_oldy = vpUnit.y;*/
	}

	if (m_TargetChanged)
	{
		m_TargetChanged = false;
		echo = uint8(1);
	}
	else if (sRunFinish)
		echo = uint8(0);
	else
		echo = uint8(3);

	/*uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vpUnit.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);

	DispatchAttack(pTarget, (vBot + vDistance).x, vpUnit.y, (vBot + vDistance).z, will_x, will_y, will_z, 3.0f, echo);*/

	Point target = { (vBot + vDistance).x, (vBot + vDistance).z };  // Örnek hedef koordinatlarý
	Point currentLocation = { GetX(), GetZ()};  // Þu anki konum
	double speed = 1.0;  // Hareket hýzý

	//std::cout << "Mevcut Konum: (" << currentLocation.x << ", " << currentLocation.y << ")\n";

	// Yeni konumu hesapla ve ekrana yazdýr
	currentLocation = moveTowardsTarget(currentLocation, target, speed);

	uint16 will_x, will_z, will_y;
	will_x = uint16(currentLocation.x * 10.0f);
	will_y = uint16(vpUnit.y * 10.0f);
	will_z = uint16(currentLocation.y * 10.0f);

	DispatchAttack(pTarget, (float)currentLocation.x, vpUnit.y, (float)currentLocation.y, will_x, will_y, will_z, m_sSpeed, echo);
}

void CBot::DispatchAttack(Unit* pTarget, float x, float y, float z, uint16 will_x, uint16 will_y, uint16 will_z, float sSpeed, uint8 echo)
{
	if (isRogue())
		GetAssasinDamageMagic(pTarget->GetID(), x, y, z, will_x, will_y, will_z, sSpeed, echo);
	else if (isWarrior())
		GetWarriorDamageMagic(pTarget->GetID(), x, y, z, will_x, will_y, will_z, sSpeed, echo);
	else if (isMage())
	{
		int nRandom = myrand(1, 3);
		switch (nRandom)
		{
		case 1:
			GetFlameMageDamageMagic(pTarget->GetID(), x, y, z, will_x, will_y, will_z, sSpeed, echo);
			break;
		case 2:
			GetGlacierMageDamageMagic(pTarget->GetID(), x, y, z, will_x, will_y, will_z, sSpeed, echo);
			break;
		default:
			GetLightningMageDamageMagic(pTarget->GetID(), x, y, z, will_x, will_y, will_z, sSpeed, echo);
			break;
		}
	}
	else if (isPriest())
		GetPriestDamageMagic(pTarget->GetID(), x, y, z, will_x, will_y, will_z, sSpeed, echo);
}


void CBot::MerchantMoveProcess()
{
	if (m_sMoveMerchantProcess > UNIXTIME)
		return;

	std::vector<Unit*> casted_member;
	std::vector<uint16> unitList;
	g_pMain->GetUnitListFromSurroundingRegions(this, &unitList);

	foreach(itr, unitList)
	{
		Unit* pTarget = g_pMain->GetUnitPtr(*itr, GetZoneID());

		if (pTarget == nullptr)
			continue;

		if (this != pTarget
			&& !pTarget->isDead()
			&& !pTarget->isBlinking()
			&& pTarget->isAttackable())
			casted_member.push_back(pTarget);
	}

	int iValue = 0;
	int16 sTargetID = -1;
	float sRange = 0.0f, sRangeSlow = 0.0f;
	__Vector3 vBot, vpUnit, vDistance, vRealDistance;
	foreach(itr, casted_member)
	{
		Unit* pTarget = *itr; // it's checked above, not much need to check it again
		if (pTarget == nullptr)
			continue;

		if (pTarget->isDead()
			|| pTarget->GetZoneID() != GetZoneID()
			|| pTarget->isNPC()
			|| pTarget->isPlayer() && !TO_USER(pTarget)->isMerchanting()
			|| pTarget->isBot() && !TO_BOT(pTarget)->isMerchanting())
			continue;

		if ((echo == uint8(1)
			&& pTarget->GetID() != m_sTargetID)
			|| (echo == 0
				&& m_sTargetID == pTarget->GetID()))
			continue;

		sRange = pow(pTarget->GetX() + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(pTarget->GetZ() + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);
		if (sRangeSlow != 0.0f && sRange > sRangeSlow)
			continue;

		sRangeSlow = sRange;
		vBot.Set(GetX(), pTarget->GetY(), GetZ());
		vpUnit.Set(pTarget->GetX() + ((myrand(0, 2000) - 1000.0f) / 500.0f), pTarget->GetY(), pTarget->GetZ() + ((myrand(0, 2000) - 1000.0f) / 500.0f));
		sTargetID = pTarget->GetID();
	}

	if (sTargetID == -1)
		return;

	if (m_sTargetID != sTargetID)
	{
		m_TargetChanged = true;
		m_sTargetID = sTargetID;
		m_oldx = vpUnit.x + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_oldz = vpUnit.z + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_oldy = vpUnit.y;
	}

	vDistance = vpUnit - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float sSpeed = m_sSpeed;
	uint8 sRuntime = 1;
	bool sRunTimeFinish = false;
	vDistance *= sSpeed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 2.0f;
		sRuntime = 2;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		sRunTimeFinish = true;
		vDistance = vRealDistance;
	}

	if (m_TargetChanged)
	{
		m_TargetChanged = false;
		echo = uint8(1);
		m_sMoveMerchantProcess = UNIXTIME + (sRuntime);
	}
	else if (sRunTimeFinish)
	{
		echo = uint8(0);
		m_sMoveMerchantProcess = UNIXTIME + (myrand(7, 17));
	}
	else
	{
		echo = uint8(3);
		m_sMoveMerchantProcess = UNIXTIME + (sRuntime);
	}

	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vpUnit.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);
	MoveRegionProcess((vBot + vDistance).x, vpUnit.y, (vBot + vDistance).z, will_x, will_y, will_z, sSpeed, echo);
}

void CBot::HPTimeChangeType3()
{
	if (isDead()
		|| !m_bType3Flag)
		return;

	uint16	totalActiveDurationalSkills = 0,
		totalActiveDOTSkills = 0;

	bool bIsDOT = false;
	for (int i = 0; i < MAX_TYPE3_REPEAT; i++)
	{
		MagicType3* pEffect = &m_durationalSkills[i];
		if (pEffect == nullptr || !pEffect->m_byUsed)
			continue;

		// Has the required interval elapsed before using this skill?
		if ((UNIXTIME - pEffect->m_tHPLastTime) >= pEffect->m_bHPInterval)
		{
			Unit* pUnit = g_pMain->GetUnitPtr(pEffect->m_sSourceID, GetZoneID());
			if (pUnit == nullptr)
				continue;

			// Reduce the HP 
			HpChange(pEffect->m_sHPAmount, pUnit); // do we need to specify the source of the DOT?
			pEffect->m_tHPLastTime = UNIXTIME;

			if (pEffect->m_sHPAmount < 0)
				bIsDOT = true;

			// Has the skill expired yet?
			if (++pEffect->m_bTickCount == pEffect->m_bTickLimit)
			{
				Packet result(WIZ_MAGIC_PROCESS, uint8(MagicOpcode::MAGIC_DURATION_EXPIRED));

				// Healing-over-time skills require the type 100
				if (pEffect->m_sHPAmount > 0)
					result << uint8(100);
				else // Damage-over-time requires 200.
					result << uint8(200);

				SendToRegion(&result);

				// If the skill inflicts poison damage, remember to remove the poison indicator!
				if (pEffect->m_byAttribute == (uint8)ResistanceTypes::POISON_R)
					SendUserStatusUpdate(UserStatus::USER_STATUS_POISON, UserStatusBehaviour::USER_STATUS_CURE);

				pEffect->Reset();
			}
		}

		if (pEffect->m_byUsed)
		{
			totalActiveDurationalSkills++;
			if (pEffect->m_sHPAmount < 0)
				totalActiveDOTSkills++;
		}
	}

	// Have all the skills expired?
	if (totalActiveDurationalSkills == 0)
		m_bType3Flag = false;

	// If there was DOT skills when we started, but none anymore... revert the HP bar.
	if (bIsDOT && totalActiveDOTSkills == 0)
		SendUserStatusUpdate(UserStatus::USER_STATUS_DOT, UserStatusBehaviour::USER_STATUS_CURE);
}

void CBot::SendUserStatusUpdate(UserStatus type, UserStatusBehaviour status)
{
	Packet result(WIZ_ZONEABILITY, uint8(2));
	result << uint8(type) << uint8(status);
	/*
	1				, 0 = Cure damage over time
	1				, 1 = Damage over time
	2				, 0 = Cure poison
	2				, 1 = poison (purple)
	3				, 0 = Cure disease
	3				, 1 = disease (green)
	4				, 1 = blind
	5				, 0 = Cure grey HP
	5				, 1 = HP is grey (not sure what this is)
	*/
	SendToZone(&result);
}

void CBot::SendToZone(Packet* result, float fRange)
{
	g_pMain->Send_Zone(result, GetZoneID(), nullptr, 0, 0, fRange);
}

void CBot::AddBotRank(C3DMap* pMap)
{
	if (isGM())
		return;

	m_PlayerKillingLoyaltyDaily = 0; m_PlayerKillingLoyaltyPremiumBonus = 0;

	if (GetNation() == (uint8)Nation::KARUS)
	{
		_PLAYER_KILLING_ZONE_RANKING* pKarusRanking = g_pMain->m_UserPlayerKillingZoneRankingArray[0].GetData(GetID());
		if (pKarusRanking == nullptr)
		{
			_PLAYER_KILLING_ZONE_RANKING* pKillinZoneRank = new _PLAYER_KILLING_ZONE_RANKING;
			pKillinZoneRank->p_SocketID = GetID();
			pKillinZoneRank->P_Nation = GetNation();
			pKillinZoneRank->p_Zone = GetZoneID();
			pKillinZoneRank->P_LoyaltyDaily = m_PlayerKillingLoyaltyDaily;
			pKillinZoneRank->P_LoyaltyPremiumBonus = m_PlayerKillingLoyaltyPremiumBonus;
			if (!g_pMain->m_UserPlayerKillingZoneRankingArray[0].PutData(pKillinZoneRank->p_SocketID, pKillinZoneRank))
				delete pKillinZoneRank;
		}
		else
		{
			pKarusRanking->p_SocketID = GetID();
			pKarusRanking->P_Nation = GetNation();
			pKarusRanking->p_Zone = GetZoneID();
			pKarusRanking->P_LoyaltyDaily = m_PlayerKillingLoyaltyDaily;
			pKarusRanking->P_LoyaltyPremiumBonus = m_PlayerKillingLoyaltyPremiumBonus;
		}
		g_pMain->m_UserPlayerKillingZoneRankingArray[1].DeleteData(GetID());
	}
	else
	{
		_PLAYER_KILLING_ZONE_RANKING* pHumanRanking = g_pMain->m_UserPlayerKillingZoneRankingArray[1].GetData(GetID());
		if (pHumanRanking == nullptr)
		{
			_PLAYER_KILLING_ZONE_RANKING* pKillinZoneRank = new _PLAYER_KILLING_ZONE_RANKING;
			pKillinZoneRank->p_SocketID = GetID();
			pKillinZoneRank->P_Nation = GetNation();
			pKillinZoneRank->p_Zone = GetZoneID();
			pKillinZoneRank->P_LoyaltyDaily = m_PlayerKillingLoyaltyDaily;
			pKillinZoneRank->P_LoyaltyPremiumBonus = m_PlayerKillingLoyaltyPremiumBonus;
			if (!g_pMain->m_UserPlayerKillingZoneRankingArray[1].PutData(pKillinZoneRank->p_SocketID, pKillinZoneRank))
				delete pKillinZoneRank;
		}
		else
		{
			pHumanRanking->p_SocketID = GetID();
			pHumanRanking->P_Nation = GetNation();
			pHumanRanking->p_Zone = GetZoneID();
			pHumanRanking->P_LoyaltyDaily = m_PlayerKillingLoyaltyDaily;
			pHumanRanking->P_LoyaltyPremiumBonus = m_PlayerKillingLoyaltyPremiumBonus;
		}
		g_pMain->m_UserPlayerKillingZoneRankingArray[0].DeleteData(GetID());
	}
}

void CBot::StateChangeServerDirect(uint8 bType, uint32 nBuff)
{
	uint8 buff = *(uint8*)&nBuff; // don't ask
	switch (bType)
	{
	case 1:
		m_bResHpType = buff;
		break;

	case 2:
		m_bNeedParty = buff;
		break;

	case 3:
		m_nOldAbnormalType = m_bAbnormalType;
		m_bAbnormalType = nBuff;
		break;

	case 5:
		m_bAbnormalType = nBuff;
		break;

	case 6:
		nBuff = m_bPartyLeader; // we don't set this here.
		break;

	case 7:
		break;

	case 8: // beginner quest
		break;

	case 14:
		break;
	}

	Packet result(WIZ_STATE_CHANGE);
	result << uint32(GetID()) << bType << uint64(nBuff);
	SendToRegion(&result);
}

void CBot::OnDeath(Unit* pKiller)
{
	if (m_bResHpType == USER_DEAD)
		return;

	m_bResHpType = USER_DEAD;
	m_BotState = BOT_DEAD;

	if (isInPKZone())
		m_sSkillCoolDown[1] = (uint32)UNIXTIME + 5;

	isReset(false);

	if (pKiller != nullptr)
	{
		if (pKiller->isPlayer())
		{
			CUser* pUserKiller = TO_USER(pKiller);
			if (pUserKiller != nullptr)
				OnDeathKilledPlayer(pUserKiller);
		}
		else if (pKiller->isNPC())
		{
			CNpc* pNpcKiller = TO_NPC(pKiller);
			if (pNpcKiller != nullptr)
				OnDeathKilledNpc(pNpcKiller);
		}
		else if (pKiller->isBot())
		{
			CBot* pBotKiller = TO_BOT(pKiller);
			if (pBotKiller != nullptr)
				OnDeathKilledBot(pBotKiller);
		}
		else
		{
			printf("OnDeath warning \n");
			TRACE("OnDeath warning \n");
		}
	}

	InitOnDeath(pKiller);
}

void CBot::InitOnDeath(Unit* pKiller)
{
	Unit::OnDeath(pKiller);

	// Player is dead stop other process.
	InitType3();
	InitType4();

	if (pKiller->isBot())
		TO_BOT(pKiller)->isReset(true);
}

/**
* @brief	Sets various zone flags to control how
* 			the client handles other players/NPCs.
* 			Also sends the zone's current tax rate.
*/
void CBot::SetZoneAbilityChange(uint16 sNewZone)
{
	C3DMap* pMap = g_pMain->GetZoneByID(sNewZone);
	CKingSystem* pKingSystem = g_pMain->m_KingSystemArray.GetData(GetNation());

	if (pMap == nullptr)
		return;

	switch (sNewZone)
	{
	case ZONE_KARUS:
	case ZONE_KARUS2:
	case ZONE_KARUS3:
	case ZONE_ELMORAD:
	case ZONE_ELMORAD2:
	case ZONE_ELMORAD3:
	case ZONE_KARUS_ESLANT:
	case ZONE_KARUS_ESLANT2:
	case ZONE_KARUS_ESLANT3:
	case ZONE_ELMORAD_ESLANT:
	case ZONE_ELMORAD_ESLANT2:
	case ZONE_ELMORAD_ESLANT3:
	case ZONE_BIFROST:
	case ZONE_BATTLE:
	case ZONE_BATTLE2:
	case ZONE_BATTLE3:
	case ZONE_BATTLE4:
	case ZONE_BATTLE5:
	case ZONE_BATTLE6:
	case ZONE_SNOW_BATTLE:
	case ZONE_RONARK_LAND:
	case ZONE_ARDREAM:
	case ZONE_RONARK_LAND_BASE:
	case ZONE_KROWAZ_DOMINION:
	case ZONE_STONE1:
	case ZONE_STONE2:
	case ZONE_STONE3:
	case ZONE_BORDER_DEFENSE_WAR:
	case ZONE_UNDER_CASTLE:
	case ZONE_JURAID_MOUNTAIN:
	case ZONE_PARTY_VS_1:
	case ZONE_PARTY_VS_2:
	case ZONE_PARTY_VS_3:
	case ZONE_PARTY_VS_4:
	case ZONE_CLAN_WAR_ARDREAM:
	case ZONE_CLAN_WAR_RONARK:
	case ZONE_KNIGHT_ROYALE:
	case ZONE_CHAOS_DUNGEON:
		if (pKingSystem != nullptr)
			pMap->SetTariff(10 + pKingSystem->m_nTerritoryTariff);
		else
			pMap->SetTariff(10);
		break;
	case ZONE_MORADON:
	case ZONE_MORADON2:
	case ZONE_MORADON3:
	case ZONE_MORADON4:
	case ZONE_MORADON5:
	case ZONE_ARENA:
		pMap->SetTariff((uint8)g_pMain->pSiegeWar.sMoradonTariff);
		break;
	case ZONE_DELOS:
	case ZONE_DESPERATION_ABYSS:
	case ZONE_HELL_ABYSS:
	case ZONE_DELOS_CASTELLAN:
		pMap->SetTariff((uint8)g_pMain->pSiegeWar.sDellosTariff);
		break;
	default:
		//printf("King and Deos Tariff unhandled zone %d \n", sNewZone);
		TRACE("King and Deos Tariff unhandled zone %d \n", sNewZone);
		break;
	}

	//m_sCoolDownListLock.lock();
	//m_sCoolDownList.clear();
	//m_sCoolDownListLock.unlock();

	//m_sSkillCastListLock.lock();
	//m_sSkillCastList.clear();
	//m_sSkillCastListLock.unlock();

	//m_sMagicTypeCooldownListLock.lock();
	//m_sMagicTypeCooldownList.clear();
	//m_sMagicTypeCooldownListLock.unlock();
}

void CBot::OnDeathKilledPlayer(CUser* pKiller)
{
	if (!pKiller)
		return;

	int16 m_sWhoKilledMe;
	if (pKiller->GetName() != GetName())
	{
		bool m_party_check = false;
		bool removerival = false;
		bool specialevent = pKiller->isInSpecialEventZone() && isInSpecialEventZone() && g_pMain->pSpecialEvent.opened;
		bool cindireallaw = g_pMain->isCindirellaZone(pKiller->GetZoneID()) && g_pMain->pCindWar.isStarted();

		uint16 bonusNP = 0;
		switch (pKiller->GetZoneID())
		{
		case ZONE_CHAOS_DUNGEON:
			if (pKiller->isInTempleEventZone(pKiller->GetZoneID()))
			{
				if (pKiller->isEventUser())
				{
					pKiller->m_ChaosExpansionKillCount++;
					pKiller->UpdateChaosExpansionRank();
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				}
			}
			break;
		case ZONE_BORDER_DEFENSE_WAR:
			if (pKiller->isInTempleEventZone(pKiller->GetZoneID()))
			{
				if (pKiller->isEventUser())
				{
					pKiller->BDWUpdateRoomKillCount();
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				}
			}
			break;
		case ZONE_JURAID_MOUNTAIN:
			if (pKiller->isInTempleEventZone(pKiller->GetZoneID()))
			{
				if (pKiller->isEventUser())
				{
					pKiller->JRUpdateRoomKillCount();
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				}
			}
			break;
		case ZONE_DELOS:
		{
			if (g_pMain->isCswActive() && pKiller->isInClan() && g_pMain->isCswWarActive())
			{
				pKiller->CastleSiegeWarfarerankKillUpdate();
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				m_party_check = true;
			}
		}
		break;
		case ZONE_MORADON:
		case ZONE_MORADON2:
		case ZONE_MORADON3:
		case ZONE_MORADON4:
		case ZONE_MORADON5:
			SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
			break;
		case ZONE_CLAN_WAR_RONARK:
		case ZONE_CLAN_WAR_ARDREAM:
		case ZONE_PARTY_VS_1:
		case ZONE_PARTY_VS_2:
		case ZONE_PARTY_VS_3:
		case ZONE_PARTY_VS_4:
			if (pKiller->isClanTournamentinZone() || pKiller->isPartyTournamentinZone())
			{
				g_pMain->UpdateClanTournamentScoreBoard(pKiller);
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
			}
			break;
		case ZONE_ARDREAM:
		case ZONE_RONARK_LAND:
		case ZONE_RONARK_LAND_BASE:
		{
			m_party_check = true;
			pKiller->KA_KillUpdate();
			KA_AssistDebufUpdate(pKiller);
			bool bKilledByRival = false;

			if (!pKiller->isInParty())
			{
				pKiller->killmoney++;
				pKiller->killuser++;

				if (pKiller->killmoney >= 3)
					pKiller->killmoney = 0;

				if (pKiller->killuser >= 5)
					pKiller->killuser = 0;
			}

			// If the killer has us set as their rival, reward them & remove the rivalry.
			bKilledByRival = (!pKiller->hasRivalryExpired() && pKiller->GetRivalID() == GetID());

			if (!cindireallaw && !specialevent && !bKilledByRival)
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);

			if (bKilledByRival)
			{
				if (!cindireallaw && !specialevent)
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeRival, true);

				// Apply bonus NP for rival kills
				bonusNP += RIVALRY_NP_BONUS;

				removerival = true;
			}

			if (!hasFullAngerGauge())
				UpdateAngerGauge(++m_byAngerGauge);

			// If we don't have a rival, this player is now our rival for 3 minutes.
			if (!hasRival()) SetRival(pKiller);
		}
		break;
		case ZONE_ELMORAD:
		case ZONE_KARUS:
			if (g_pMain->isWarOpen())
			{
				m_party_check = true;
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, false);
			}
			break;
		case ZONE_BATTLE:
		case ZONE_BATTLE2:
		case ZONE_BATTLE3:
		case ZONE_BATTLE4:
		case ZONE_BATTLE5:
		case ZONE_BATTLE6:
			if (g_pMain->isWarOpen())
			{
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, false);
				m_party_check = true;
			}
			break;
		case ZONE_BIFROST:
			m_party_check = true;
			SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, false);
			break;
		case ZONE_SNOW_BATTLE:
			if (g_pMain->m_byBattleOpen == SNOW_BATTLE)
			{
				pKiller->GoldGain(SNOW_EVENT_MONEY);
				GetNation() == (uint8)Nation::KARUS ? g_pMain->m_sKarusDead++ : g_pMain->m_sElmoradDead++;;
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
			}
			break;
		}

		if (pKiller->isInSpecialEventZone()
			&& isInSpecialEventZone())
			m_party_check = true;

		if (m_party_check)
		{
			_PARTY_GROUP* pParty = nullptr;
			if (pKiller->isInParty() && (pParty = g_pMain->GetPartyPtr(pKiller->GetPartyID())) != nullptr)
			{

				for (int i = 0; i < MAX_PARTY_USERS; i++)
				{
					CUser* PartyUser = g_pMain->GetUserPtr(pParty->uid[i]);
					if (PartyUser == nullptr || !isInRangeSlow(PartyUser, RANGE_50M)
						|| PartyUser->isDead() || !PartyUser->isInGame() || !PartyUser->isPlayer())
						continue;

					PartyUser->killmoney++;
					PartyUser->killuser++;

					if (PartyUser->killmoney >= 3)
						PartyUser->killmoney = 0;

					if (PartyUser->killuser >= 5)
						PartyUser->killuser = 0;

					if (PartyUser->GetNation() == (uint8)Nation::KARUS)
						PartyUser->QuestV2MonsterCountAdd((uint8)Nation::KARUS);
					else if (PartyUser->GetNation() == (uint8)Nation::ELMORAD)
						PartyUser->QuestV2MonsterCountAdd((uint8)Nation::ELMORAD);
#if 0
					PartyUser->UpdateDailyQuestCount(1);

					if (g_pMain->pCollectionRaceEvent.m_sCollectionStatus)
						g_pMain->CollectionRaceSendDead(PartyUser, 0x01);
#endif
				}
			}
			else
			{
				if (GetNation() == (uint8)Nation::KARUS)
					pKiller->QuestV2MonsterCountAdd((uint8)Nation::KARUS);
				else if (GetNation() == (uint8)Nation::ELMORAD)
					pKiller->QuestV2MonsterCountAdd((uint8)Nation::ELMORAD);
#if 0
				pKiller->UpdateDailyQuestCount(1);

				if (g_pMain->pCollectionRaceEvent.m_sCollectionStatus)
					g_pMain->CollectionRaceSendDead(pKiller, 0x01);
#endif
			}
		}

		if (specialevent || cindireallaw)
		{
			SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);

			if (specialevent)
				pKiller->ZindanWarUpdateScore();
		}

		if (pKiller->isInPKZone())
		{
			pKiller->m_KillCount++;
			pKiller->GiveKillReward();
		}

		if (pKiller->GetMap())
		{
			if (pKiller->GetMap()->m_bGiveLoyalty)
			{
				if (pKiller->isInParty()) 
					pKiller->LoyaltyDivide("user kill", this, bonusNP);
				else 
					pKiller->LoyaltyChange(this, bonusNP);
			}

			if (pKiller->GetMap()->m_bGoldLose && !cindireallaw)
				pKiller->GoldChange(GetID(), 0);
		}

		if (removerival 
			&& pKiller->isInGame() 
			&& pKiller->hasRival())
			pKiller->RemoveRival();

		m_sWhoKilledMe = pKiller->GetID();
	}
	else
		m_sWhoKilledMe = -1;
}

void CBot::OnDeathKilledBot(CBot* pKiller)
{
	int16 m_sWhoKilledMe;
	if (pKiller != nullptr)
	{
		if (pKiller->GetName() != GetName())
		{
			bool m_sRivalStatus = false;

			uint16 bonusNP = 0;
			switch (pKiller->GetZoneID())
			{
			case ZONE_CHAOS_DUNGEON:
				if (pKiller->isInTempleEventZone(pKiller->GetZoneID()))
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				break;
			case ZONE_BORDER_DEFENSE_WAR:
				if (pKiller->isInTempleEventZone(pKiller->GetZoneID()))
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				break;
			case ZONE_JURAID_MOUNTAIN:
				if (pKiller->isInTempleEventZone(pKiller->GetZoneID()))
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				break;
			case ZONE_DELOS:
			{
				if (g_pMain->isCswActive() 
					&& pKiller->isInClan() 
					&& g_pMain->isCswWarActive())
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
			}
			break;
			case ZONE_MORADON:
			case ZONE_MORADON2:
			case ZONE_MORADON3:
			case ZONE_MORADON4:
			case ZONE_MORADON5:
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				break;
			case ZONE_CLAN_WAR_RONARK:
			case ZONE_CLAN_WAR_ARDREAM:
			case ZONE_PARTY_VS_1:
			case ZONE_PARTY_VS_2:
			case ZONE_PARTY_VS_3:
			case ZONE_PARTY_VS_4:
				if (pKiller->isClanTournamentinZone() 
					|| pKiller->isPartyTournamentinZone())
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				break;
			case ZONE_ARDREAM:
			case ZONE_RONARK_LAND:
			case ZONE_RONARK_LAND_BASE:
			{
				bool bKilledByRival = false;

				// If the killer has us set as their rival, reward them & remove the rivalry.
				bKilledByRival = (!pKiller->hasRivalryExpired() && pKiller->GetRivalID() == GetID());

				if (!bKilledByRival)
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);

				if (bKilledByRival)
				{
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeRival, true);
					// Apply bonus NP for rival kills
					bonusNP += RIVALRY_NP_BONUS;
					m_sRivalStatus = true;
				}

				if (!hasFullAngerGauge())
					UpdateAngerGauge(++m_byAngerGauge);

				// If we don't have a rival, this player is now our rival for 3 minutes.
				if (!hasRival()) SetRival(pKiller);
			}
			break;
			case ZONE_ELMORAD:
			case ZONE_KARUS:
				if (g_pMain->isWarOpen())
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, false);
				break;
			case ZONE_BATTLE:
			case ZONE_BATTLE2:
			case ZONE_BATTLE3:
			case ZONE_BATTLE4:
			case ZONE_BATTLE5:
			case ZONE_BATTLE6:
				if (g_pMain->isWarOpen())
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, false);
				break;
			case ZONE_BIFROST:
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, false);
				break;
			case ZONE_SNOW_BATTLE:
				if (g_pMain->m_byBattleOpen == SNOW_BATTLE)
				{
					GetNation() == (uint8)Nation::KARUS ? g_pMain->m_sKarusDead++ : g_pMain->m_sElmoradDead++;;
					SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
				}
				break;
			}

			if (pKiller->GetMap())
			{
				if (pKiller->GetMap()->m_bGiveLoyalty)
				{
					if (pKiller->isInParty()) 
						pKiller->LoyaltyBotDivide(GetID(), bonusNP);
					else 
						pKiller->LoyaltyBotChange(GetID(), bonusNP);
				}
			}

			if (m_sRivalStatus
				&& pKiller->isInGame() 
				&& pKiller->hasRival())
				pKiller->RemoveRival();

			m_sWhoKilledMe = pKiller->GetID();
		}
		else
			m_sWhoKilledMe = -1;
	}
}

void CBot::OnDeathKilledNpc(CNpc* pKiller)
{
	if (pKiller != nullptr)
	{
		int64 nExpLost = 0;

		switch (pKiller->GetZoneID())
		{
		case ZONE_ARDREAM:
		case ZONE_RONARK_LAND:
		case ZONE_RONARK_LAND_BASE:
			if (pKiller->GetType() == NPC_GUARD_TOWER1 || pKiller->GetType() == NPC_GUARD_TOWER2)
				SendDeathNotice(pKiller, DeathNoticeType::DeathNoticeCoordinates, true);
			break;
		}
	}
}

uint8 CBot::GetPVPMonumentNation() { return g_pMain->m_nPVPMonumentNation[GetZoneID()]; }

void CBot::UpdatePlayerKillingRank()
{
	if (GetNation() < KARUS 
		|| GetNation() > ELMORAD)
		return;

	uint8 bnation = GetNation() - 1;

	if (isInSpecialEventZone()) {
		auto* pRank = g_pMain->m_ZindanWarZoneRankingArray[bnation].GetData(GetID());
		if (pRank != nullptr) {
			pRank->z_LoyaltyDaily = m_PlayerKillingLoyaltyDaily;
			pRank->z_LoyaltyPremiumBonus = m_PlayerKillingLoyaltyPremiumBonus;
		}
	}
	else {
		auto* pRank = g_pMain->m_UserPlayerKillingZoneRankingArray[bnation].GetData(GetID());
		if (pRank != nullptr) {
			pRank->P_LoyaltyDaily = m_PlayerKillingLoyaltyDaily;
			pRank->P_LoyaltyPremiumBonus = m_PlayerKillingLoyaltyPremiumBonus;
		}
	}
}

float CBot::getplusdamage()
{
	auto pLeftHand = GetItemPrototype(LEFTHAND);
	if (!pLeftHand.isnull())
	{
		if (pLeftHand.m_ItemType == 4 
			|| pLeftHand.m_ItemType == 12) 
			return g_pMain->pDamageSetting.uniqueitem;
		else if (pLeftHand.m_ItemType == 2) 
			return g_pMain->pDamageSetting.rareitem;
		else if (pLeftHand.m_ItemType == 1)
			return g_pMain->pDamageSetting.magicitem;
		else if (pLeftHand.m_ItemType == 5 
			&& (pLeftHand.ItemClass == 0 
				|| pLeftHand.ItemClass == 1)) 
			return g_pMain->pDamageSetting.lowclassitem;
		else if (pLeftHand.m_ItemType == 5 
			&& (pLeftHand.ItemClass == 2 
				|| pLeftHand.ItemClass == 7 
				|| pLeftHand.ItemClass == 33)) 
			return g_pMain->pDamageSetting.middleclassitem;
		else if (pLeftHand.m_ItemType == 5 
			&& (pLeftHand.ItemClass == 3 
				|| pLeftHand.ItemClass == 4 
				|| pLeftHand.ItemClass == 8 
				|| pLeftHand.ItemClass == 34)) 
			return g_pMain->pDamageSetting.highclassitem;
		else if (pLeftHand.m_ItemType == 11)
			return g_pMain->pDamageSetting.highclassitem;
	}
	else
	{
		auto pRightHand = GetItemPrototype(RIGHTHAND);
		if (!pRightHand.isnull())
		{
			if (pRightHand.m_ItemType == 4 
				|| pRightHand.m_ItemType == 12) 
				return g_pMain->pDamageSetting.uniqueitem;
			else if (pRightHand.m_ItemType == 2)
				return g_pMain->pDamageSetting.rareitem;
			else if (pRightHand.m_ItemType == 1) 
				return g_pMain->pDamageSetting.magicitem;
			else if (pRightHand.m_ItemType == 5 
				&& (pRightHand.ItemClass == 0 
					|| pRightHand.ItemClass == 1))
				return g_pMain->pDamageSetting.lowclassitem;
			else if (pRightHand.m_ItemType == 5 
				&& (pRightHand.ItemClass == 2 
					|| pRightHand.ItemClass == 7 
					|| pRightHand.ItemClass == 33))
				return g_pMain->pDamageSetting.middleclassitem;
			else if (pRightHand.m_ItemType == 5 
				&& (pRightHand.ItemClass == 3 
					|| pRightHand.ItemClass == 4 
					|| pRightHand.ItemClass == 8 
					|| pRightHand.ItemClass == 34))
				return g_pMain->pDamageSetting.highclassitem;
			else if (pRightHand.m_ItemType == 11)
				return g_pMain->pDamageSetting.highclassitem;
		}
	}
	return 1.0f;
}

void CBot::KA_AssistDebufUpdate(CUser* pkiller)
{
	return;
	if (!pkiller || !pkiller->isInParty()) return;

	std::vector<CUser*> mpriestlist;

	m_buffLock.lock();
	foreach(sk, m_buffMap) { if (sk->second.isDebuff() && sk->second.pownskill) mpriestlist.push_back(sk->second.pownskill); }
	m_buffLock.unlock();

	foreach(itr, mpriestlist)
	{
		if (!(*itr) 
			|| !(*itr)->isInParty() 
			|| !(*itr)->isPriest() 
			|| (*itr)->GetPartyID() != pkiller->GetPartyID())
			continue;

		(*itr)->KA_AssistScreenSend(1);
	}
}

int CBot::FindSlotForItem(uint32 nItemID, uint16 sCount /*= 1*/)
{
	int result = -1;
	_ITEM_TABLE pTable = g_pMain->GetItemPtr(nItemID);
	if (pTable.isnull())
		return result;

	_ITEM_DATA* pItem = nullptr;
	if (pTable.m_bCountable > 0) {
		for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++) {
			pItem = GetItem(i);
			if (pItem == nullptr)
				continue;

			if (pItem->nNum == nItemID && pItem->sCount + sCount <= ITEMCOUNT_MAX)
				return i;

			// Found a free slot, we'd prefer to stack it though
			// so store the first free slot, and ignore it.
			if (pItem->nNum == 0 && result < 0)
				result = i;
		}
		// If we didn't find a slot countaining our stackable item, it's possible we found
		// an empty slot. So return that (or -1 if it none was found; no point searching again).
		return result;
	}

	// If it's not stackable, don't need any additional logic.
	// Just find the first free slot.
	return GetEmptySlot();
}

int CBot::GetEmptySlot()
{
	_ITEM_DATA* pItem = nullptr;
	for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
	{
		pItem = GetItem(i);
		if (pItem == nullptr)
			continue;

		if (pItem->nNum == 0)
			return i;
	}
	return -1;
}

void CBot::BotHemesGo()
{
	uint8 echo = 0;
	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	uint16 BestTargetID = uint16(-1);
	__Vector3 vBot, vUser, vDistance, vRealDistance;

	uint8 RandomX = myrand(1, 2);
	float x, z;

	if (RandomX == 1)
	{
		x = 327.0f;
		z = 367.0f;
	}
	else
	{
		x = 330.0f;
		z = 365.0f;
	}

	Mesafe = pow(x + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(z + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);
	if (EnYakinMesafe != 0.0f && Mesafe > EnYakinMesafe)
		return;


	if (RandomX == 1)
		vBot.Set(GetX() + 2, 0, GetZ());
	else
		vBot.Set(GetX() - 1, 0, GetZ());

	vUser.Set(x + ((myrand(0, 2000) - 1000.0f) / 500.0f), 0, z + ((myrand(0, 2000) - 1000.0f) / 500.0f));

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = 40.0f;
	uint8 KosuSuresi = 1;
	bool Finished = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 2.0f;
		KosuSuresi = 1;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		Finished = true;
		vDistance = vRealDistance;
	}

	if (Finished)
	{
		m_sLoginBotType = BOT_NPC_MOVE_UNW;
		echo = uint8(3);
		m_sMoveTimeCode = 0;
	}
	else
	{
		echo = uint8(3);
		m_sMoveTimeCode = GetTickCount64() + 1700;
	}

	//uint16 will_x, will_z, will_y;
	//will_x = uint16((vBot + vDistance).x * 10.0f);
	//will_y = uint16(vUser.y * 10.0f);
	//will_z = uint16((vBot + vDistance).z * 10.0f);
	//MoveRegionProcess((vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, 40/*sSpeed*/, echo);
	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);

	Packet result(WIZ_MOVE);
	result << uint32(GetID()) << will_x << will_z << will_y << uint16(40) << uint8(echo);
	SendToRegion(&result);

	m_curx = (vBot + vDistance).x;
	m_curz = (vBot + vDistance).z;
	m_cury = vUser.y;
	RegisterRegion();
}

void CBot::BotHemesGoStop()
{
	uint8 echo = 0;
	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	uint16 BestTargetID = uint16(-1);
	__Vector3 vBot, vUser, vDistance, vRealDistance;
	uint8 RandomX = myrand(1, 13);
	float CX = 0.0f, CZ = 0.0f;

	if (RandomX == 1)
	{
		CX = 309.0f;
		CZ = 404.0f;
	}
	else if (RandomX == 2)
	{
		CX = 305.0f;
		CZ = 403.0f;
	}
	else if (RandomX == 3)
	{
		CX = 305.0f;
		CZ = 399.0f;
	}
	else if (RandomX == 4)
	{
		CX = 306.0f;
		CZ = 397.0f;
	}
	else if (RandomX == 5)
	{
		CX = 310.0f;
		CZ = 396.0f;
	}
	else if (RandomX == 6)
	{
		CX = 312.0f;
		CZ = 397.0f;
	}
	else if (RandomX == 7)
	{
		CX = 310.0f;
		CZ = 399.0f;
	}
	else if (RandomX == 8)
	{
		CX = 309.0f;
		CZ = 401.0f;
	}
	else if (RandomX == 9)
	{
		CX = 308.0f;
		CZ = 399.0f;
	}
	else if (RandomX == 10)
	{
		CX = 313.0f;
		CZ = 401.0f;
	}
	else if (RandomX == 11)
	{
		CX = 314.0f;
		CZ = 398.0f;
	}
	else if (RandomX == 12)
	{
		CX = 316.0f;
		CZ = 394.0f;
	}
	else if (RandomX == 13)
	{
		CX = 309.0f;
		CZ = 400.0f;
	}

	Mesafe = pow(CX + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(CZ + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);
	if (EnYakinMesafe != 0.0f && Mesafe > EnYakinMesafe)
		return;

	vBot.Set(GetX(), 0, GetZ());
	vUser.Set(CX + ((myrand(0, 2000) - 1000.0f) / 500.0f), 0, CZ + ((myrand(0, 2000) - 1000.0f) / 500.0f));

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = 40.0f;
	uint8 KosuSuresi = 1;
	bool Finished = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 2.0f;
		KosuSuresi = 1;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		Finished = true;
		vDistance = vRealDistance;
	}

	if (Finished)
	{
		m_StartMoveTime = 0;
		m_sLoginBotType = 0;
		echo = uint8(0);
		m_sMoveTimeCode = GetTickCount64() + 1700;
	}
	else
	{
		echo = uint8(3);
		m_sMoveTimeCode = GetTickCount64() + 1700;
	}

	//uint16 will_x, will_z, will_y;
	//will_x = uint16((vBot + vDistance).x * 10.0f);
	//will_y = uint16(vUser.y * 10.0f);
	//will_z = uint16((vBot + vDistance).z * 10.0f);
	//MoveRegionProcess((vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, 40/*sSpeed*/, echo);
	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);

	Packet result(WIZ_MOVE);
	result << uint32(GetID()) << will_x << will_z << will_y << uint16(40) << uint8(echo);
	SendToRegion(&result);

	m_curx = (vBot + vDistance).x;
	m_curz = (vBot + vDistance).z;
	m_cury = vUser.y;
	RegisterRegion();
}

void CBot::BotTownGo()
{
	uint8 echo = 0;
	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	uint16 BestTargetID = uint16(-1);
	__Vector3 vBot, vUser, vDistance, vRealDistance;
	float CX = 0.0f, CZ = 0.0f;
	uint32 RandomX = myrand(248, 268);//town kordinat 1.
	uint32 RandomZ = myrand(286, 309);// town kordinat 2.

	CX = (float)RandomX;
	CZ = (float)RandomZ;

	Mesafe = pow(CX + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetX(), 2.0f) + pow(CZ + ((myrand(0, 2000) - 1000.0f) / 500.0f) - GetZ(), 2.0f);
	if (EnYakinMesafe != 0.0f && Mesafe > EnYakinMesafe)
		return;

	vBot.Set(GetX(), 0, GetZ());
	vUser.Set(CX + ((myrand(0, 2000) - 1000.0f) / 500.0f), 0, CZ + ((myrand(0, 2000) - 1000.0f) / 500.0f));

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = 40.0f;
	uint8 KosuSuresi = 1;
	bool Finished = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 2.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 2.0f;
		KosuSuresi = 1;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		Finished = true;
		vDistance = vRealDistance;
	}

	if (Finished)
	{
		m_StartMoveTime = 0;
		m_sLoginBotType = 0;
		echo = uint8(0);
		m_sMoveTimeCode = GetTickCount64() + 1700;
	}
	else
	{
		echo = uint8(3);
		m_sMoveTimeCode = GetTickCount64() + 1700;
	}

	//uint16 will_x, will_z, will_y;
	//will_x = uint16((vBot + vDistance).x * 10.0f);
	//will_y = uint16(vUser.y * 10.0f);
	//will_z = uint16((vBot + vDistance).z * 10.0f);
	//MoveRegionProcess((vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, 40/*sSpeed*/, echo);
	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);

	Packet result(WIZ_MOVE);
	result << uint32(GetID()) << will_x << will_z << will_y << uint16(40) << uint8(echo);
	SendToRegion(&result);

	m_curx = (vBot + vDistance).x;
	m_curz = (vBot + vDistance).z;
	m_cury = vUser.y;
	RegisterRegion();
}