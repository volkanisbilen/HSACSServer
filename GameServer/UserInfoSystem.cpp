#include "stdafx.h"
#include "KingSystem.h"
#include "DBAgent.h"

using namespace std;

/**
* @brief	Sends the player's information on initial login.
*/
void CUser::SendMyInfo(bool after /*after = false*/)
{
	C3DMap* pMap = GetMap();
	if (pMap == nullptr)
		return;

	CKnights* pKnights = nullptr;

	if (!pMap->IsValidPosition(GetX(), GetZ(), 0.0f))
	{
		short x = 0, z = 0;
		GetStartPosition(x, z);

		m_curx = (float)x;
		m_curz = (float)z;
	}

	if (!after)
		AchieveNormalCountAdd(UserAchieveNormalTypes::AchieveBecomeKing, 0, nullptr);

	Packet result(WIZ_MYINFO);

	// Load up our user rankings (for our NP symbols).
	g_pMain->GetUserRank(this);

	// Are we the King? Let's see, shall we?
	CKingSystem * pData = g_pMain->m_KingSystemArray.GetData(GetNation());
	if (pData != nullptr
		&& STRCASECMP(pData->m_strKingName.c_str(), m_strUserID.c_str()) == 0)
		m_bRank = 1; // We're da King, man.
	else
		m_bRank = 0; // totally not da King.

	result.SByte(); // character name has a single byte length
	result << uint32(GetSocketID())
		<< GetName()
		<< GetSPosX()
		<< GetSPosZ()
		<< GetSPosY()
		<< GetNation()
		<< m_bRace
		<< m_sClass
		<< m_bFace
		<< m_nHair
		<< m_bRank
		<< m_bTitle
		<< uint8(0) << uint8(0)
		<< GetLevel()
		<< m_sPoints
		<< m_iMaxExp
		<< m_iExp
		<< GetLoyalty()
		<< GetMonthlyLoyalty()
		<< GetClanID()
		<< GetFame();

	if (isInClan())
		pKnights = g_pMain->GetClanPtr(GetClanID());

	if (pKnights == nullptr)
	{
		if (isKing())
			result << uint64(0) << uint16(GetNation() == KARUS ? 97 : 98) << uint32(0);
		else
			result << uint64(0) << uint16(-1) << uint32(0);
	}
	else
	{
		result << pKnights->GetAllianceID() << pKnights->m_byFlag << pKnights->GetName() << pKnights->m_byGrade << pKnights->m_byRanking << pKnights->m_sMarkVersion;

		CKnights* pMainClan = g_pMain->GetClanPtr(pKnights->GetAllianceID());
		_KNIGHTS_ALLIANCE* pAlliance = g_pMain->GetAlliancePtr(pKnights->GetAllianceID());

		if (pKnights->isInAlliance()
			&& pMainClan != nullptr
			&& pAlliance != nullptr)
		{
			if (isKing())
				result << uint16(GetNation() == KARUS ? 97 : 98) << uint32(0);
			else
			{
				if (pMainClan->isCastellanCape() == false)
				{
					if (pAlliance->sMainAllianceKnights == pKnights->GetID())
						result << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
					else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
						result << pMainClan->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
					else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
						result << pMainClan->GetCapeID() << uint32(0); // only the cape will be present
				}
				else
				{
					if (pMainClan->m_sCastTime >= uint32(UNIXTIME))
					{
						if (pAlliance->sMainAllianceKnights == pKnights->GetID())
							result << pMainClan->m_sCastCapeID << pMainClan->m_bCastCapeR << pMainClan->m_bCastCapeG << pMainClan->m_bCastCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
						else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
							result << pMainClan->m_sCastCapeID << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
						else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
							result << pMainClan->m_sCastCapeID << uint32(0); // only the cape will be present
					}
					else
					{
						if (pAlliance->sMainAllianceKnights == pKnights->GetID())
							result << pMainClan->GetCapeID() << pMainClan->m_bCapeR << pMainClan->m_bCapeG << pMainClan->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
						else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
							result << pMainClan->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
						else if (pAlliance->sMercenaryClan_1 == pKnights->GetID() || pAlliance->sMercenaryClan_2 == pKnights->GetID())
							result << pMainClan->GetCapeID() << uint32(0); // only the cape will be present
					}
				}
			}
		}
		else
		{
			pKnights->OnLogin(this);
			if (isKing())
				result << uint16(GetNation() == KARUS ? 97 : 98) << uint32(0);
			else
			{
				if (pKnights->isCastellanCape() == false)
					result << pKnights->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
				else
				{
					if (pKnights->m_sCastTime >= uint32(UNIXTIME))
						result << pKnights->m_sCastCapeID << pKnights->m_bCastCapeR << pKnights->m_bCastCapeG << pKnights->m_bCastCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
					else
						result << pKnights->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(1) : uint8(2));
				}
			}
		}
	}

	result << uint8(0) << uint8(0) << uint8(0) << uint8(0) // unknown
		<< uint8(0) << uint8(0) << uint8(0) << uint8(0) // unknown
		<< m_MaxHp << m_sHp
		<< m_MaxMp << m_sMp
		<< m_sMaxWeight << m_sItemWeight
		<< GetStat(StatType::STAT_STR) << uint8(GetStatItemBonus(StatType::STAT_STR))
		<< GetStat(StatType::STAT_STA) << uint8(GetStatItemBonus(StatType::STAT_STA))
		<< GetStat(StatType::STAT_DEX) << uint8(GetStatItemBonus(StatType::STAT_DEX))
		<< GetStat(StatType::STAT_INT) << uint8(GetStatItemBonus(StatType::STAT_INT))
		<< GetStat(StatType::STAT_CHA) << uint8(GetStatItemBonus(StatType::STAT_CHA))
		<< m_sTotalHit << m_sTotalAc
		<< uint8(m_sFireR) << uint8(m_sColdR) << uint8(m_sLightningR)
		<< uint8(m_sMagicR) << uint8(m_sDiseaseR) << uint8(m_sPoisonR)
		<< m_iGold
		<< m_bAuthority;

	if (GetLevel() < 30 && !ChickenStatus && GAME_SOURCE_VERSION == 1098)		// V2 için burasý aktif olucak.
		result << int8(50) << int8(250);
	else if (GetLevel() < 30 && !ChickenStatus && GAME_SOURCE_VERSION == 1534)		// V2 için burasý aktif olucak.
		result << int8(50) << int8(250);
	else
		result << (m_bKnightsRank <= m_bPersonalRank ? m_bKnightsRank : int8(-1)) << (m_bPersonalRank <= m_bKnightsRank ? m_bPersonalRank : int8(-1));

	result.append(m_bstrSkill, 9);

	for (int i = 0; i < INVENTORY_TOTAL; i++)
	{
		_ITEM_DATA *pItem = GetItem(i);
		if (pItem == nullptr)
			continue;

		result << pItem->nNum
			<< pItem->sDuration
			<< pItem->sCount
			<< pItem->bFlag
			<< pItem->sRemainingRentalTime;	// remaining time

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
		result << pItem->nExpirationTime; // expiration date in unix time
	}
	
	m_PremiumMap.m_lock.lock();
	auto m_sPremiumMap = m_PremiumMap.m_UserTypeMap;
	m_PremiumMap.m_lock.unlock();

	result.SByte();
	m_bIsChicken = GetLevel() < 30 ? true : false;// CheckExistEvent(50, 1);
	result << uint8(m_bAccountStatus)	// account status (0 = none, 1 = normal prem with expiry in hours, 2 = pc room)
		<< uint8(m_sPremiumMap.size());

	foreach(itr, m_sPremiumMap)
	{
		_PREMIUM_DATA * pPreData = itr->second;
		if (pPreData == nullptr 
			|| pPreData->iPremiumTime == 0)
			continue;

		uint32 TimeRest;
		uint16 TimeShow;
		TimeRest = uint32(pPreData->iPremiumTime - UNIXTIME);

		if (TimeRest >= 1 && TimeRest <= 3600)
			TimeShow = 1;
		else
			TimeShow = TimeRest / 3600;

		result << pPreData->bPremiumType << TimeShow;
	}

	result << m_bPremiumInUse
		<< uint8(m_bIsChicken)	// chicken /beginner flag
		<< m_iMannerPoint
		<< g_pMain->m_sKarusMilitary
		<< g_pMain->m_sHumanMilitary
		<< g_pMain->m_sKarusEslantMilitary
		<< g_pMain->m_sHumanEslantMilitary
		<< g_pMain->m_sMoradonMilitary
		<< uint8(1)
		<< uint16(GetGenieTime())
		<< GetRebirthLevel()
		<< GetRebStatBuff(StatType::STAT_STR)
		<< GetRebStatBuff(StatType::STAT_STA)
		<< GetRebStatBuff(StatType::STAT_DEX)
		<< GetRebStatBuff(StatType::STAT_INT)
		<< GetRebStatBuff(StatType::STAT_CHA)
		<< uint64(m_iSealedExp)
		<< uint16(m_sCoverTitle)
		<< uint16(m_sSkillTitle)
		<< ReturnSymbolisOK
		<< uint32(0) << uint16(1);

	SendCompressed(&result);
	SetZoneAbilityChange(GetZoneID());

	if (g_pMain->ClanPrePazar)
	{
		if (sClanPremStatus)
			m_bPremiumMerchant = true;
	}

	if (isGM())
		m_bAuthorityColor = 20;
	else if (m_bRank == 1)
		m_bAuthorityColor = 22;
	else
		m_bAuthorityColor = 1;

	m_bGearSkills = true;
}

void CUser::GetUserInfo(Packet & pkt)
{
	pkt.SByte();
	pkt << GetName() << GetNation();
	pkt << uint8(0) << uint8(0) << uint8(0);
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
		<< uint8(0x00)
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
	pkt << uint16(GetZoneID()) << int32(-1) << uint8(0) << uint32(0) << m_bIsHidingHelmet << m_bIsHidingCospre << isInGenie() << GetRebirthLevel() << uint16(m_sCoverTitle) << ReturnSymbolisOK << uint8(0) << uint32(0) << uint8(0);
}

void CUser::SendInfo()
{ 
	
}

void CUser::RightTopTitleMsg() 
{
	g_pMain->m_RightTopTitleArray.m_lock.lock();
	auto m_RightTopTitleArray = g_pMain->m_RightTopTitleArray.m_UserTypeMap;
	g_pMain->m_RightTopTitleArray.m_lock.unlock();

	foreach(itr, m_RightTopTitleArray)
	{
		_RIGHT_TOP_TITLE_MSG * pMessageShow = itr->second;
		if (pMessageShow == nullptr)
			continue;

		Packet result(WIZ_NOTICE);
		result.DByte();
		result << uint8(4)
			<< uint8(1) 
			<< pMessageShow->strTitle
			<< pMessageShow->strMessage;

		Send(&result);
	}
}

void CUser::RightTopTitleMsgDelete() 
{
	g_pMain->m_RightTopTitleArray.m_lock.lock();
	auto m_RightTopTitleArray = g_pMain->m_RightTopTitleArray.m_UserTypeMap;
	g_pMain->m_RightTopTitleArray.m_lock.unlock();

	foreach(itr, m_RightTopTitleArray)
	{
		_RIGHT_TOP_TITLE_MSG * pMessageShow = itr->second;
		if (pMessageShow == nullptr)
			continue;

		Packet result(WIZ_NOTICE);
		result.DByte();
		result << uint8(4)
			<< uint8(2) 
			<< pMessageShow->strTitle 
			<< pMessageShow->strMessage;

		Send(&result);
	}
}