#include "stdafx.h"
#include "stdafx.h"
#include "DBAgent.h"
#include <vector>
#include "md5.h"

using namespace std;
#define HSACSX_ACTIVE			1
#define HSACSX_VERSION			0x51				// Anticheat derlendiðinde burasý deðiþtirilmeli. 
#define HSACSX_ALIVE_TIMEOUT	60
#define HSACSX_SUPPORT_CHEK		30

time_t HSACSX_LASTSUPPORT = 0;

struct ProcessInfo
{
	int id;
	string name;
	vector<string> windows;
	ProcessInfo(int _id, string _name, vector<string> _windows)
	{
		id = _id;
		name = _name;
		windows = _windows;
	}
};

enum class PUS_CATEGORY
{
	SPECIAL = 1,
	POWER_UP = 2,
	COSTUME = 3,
	BUNDLE = 4
};

struct PUSItem
{
	uint32 sID;
	string Name;
	string Desc1;
	string Desc2;
	uint32 Price;
	uint32 Count;
	PUS_CATEGORY Category;
	PUSItem(uint32 sid, string name, string desc1, string desc2, uint32 price, uint32 count, PUS_CATEGORY category)
	{
		sID = sid;
		Name = name;
		Desc1 = desc1;
		Desc2 = desc2;
		Price = price;
		Count = count;
		Category = category;
	}
};

struct _TempItem
{
	uint32 nItemID;
	uint32 time;
	uint8 pos, slot;
};

void CUser::HSACSX_SendLifeSkills()
{
	LF_VEC2 m_War = GetLifeSkillCurrent(LIFE_SKILL::WAR);
	LF_VEC2 m_Hunting = GetLifeSkillCurrent(LIFE_SKILL::HUNTING);
	LF_VEC2 m_Smithery = GetLifeSkillCurrent(LIFE_SKILL::SMITHERY);
	LF_VEC2 m_Karma = GetLifeSkillCurrent(LIFE_SKILL::KARMA);
#if(__VERSION < 2369)
	Packet pkt(WIZ_HSACS_HOOK);
	pkt << uint8(HSACSXOpCodes::BANSYSTEM)
		<< GetLifeSkillLevel(LIFE_SKILL::WAR) 
		<< m_War.EXP 
		<< m_War.TargetEXP
		<< GetLifeSkillLevel(LIFE_SKILL::HUNTING) 
		<< m_Hunting.EXP
		<< m_Hunting.TargetEXP
		<< GetLifeSkillLevel(LIFE_SKILL::SMITHERY) 
		<< m_Smithery.EXP 
		<< m_Smithery.TargetEXP
		<< GetLifeSkillLevel(LIFE_SKILL::KARMA) 
		<< m_Karma.EXP 
		<< m_Karma.TargetEXP;
	Send(&pkt);
#endif
}

void CUser::HSACSX_HandleLifeSkill(Packet& pkt)
{
	HSACSX_SendLifeSkills();
}

void CUser::HSACSX_ItemNotice(uint8 type, uint32 nItemID)
{
#if(__VERSION < 2369)
	Packet pkt(WIZ_HSACS_HOOK);
	pkt << uint8_t(HSACSXOpCodes::AUTODROP) << type << nItemID;
	Send(&pkt);
#endif
}

void CUser::HSACSX_SendMessageBox(string title, string message)
{
#if(__VERSION < 2369)
	Packet pkt(WIZ_HSACS_HOOK);
	pkt << uint8_t(HSACSXOpCodes::MESSAGE) << title << message;
	Send(&pkt);
#endif
}

void CUser::HSACSX_ReqMerchantSystem(Packet& pkt)
{
	uint8 m_sOpCode;
	pkt >> m_sOpCode;

	Packet result(WIZ_MERCHANT, uint8(MERCHANT_MENISIA_LIST));
	if (!CheckExistItem(810166000))
	{
		result << uint8(1) << uint8(0) << uint8(3);
		Send(&result);
		return;
	}

	if (GetMap() == nullptr
		|| GetMap()->m_bMenissiahList != 1)
	{
		result << uint8(1) << uint8(0) << uint8(3);
		Send(&result);
		return;
	}

	switch (m_sOpCode)
	{
	case 0:
		HSACSX_ReqMerchantListSend(pkt);
		break;
	case 1:
		HSACSX_ReqMerchantListGo(pkt);
		break;
	case 2:
		HSACSX_ReqMerchantListMessage(pkt);
		break;
	default:
		break;
	}
}

void CUser::HSACSX_ReqMerchantListMessage(Packet& pkt)
{
	string m_sCharackterName;
	pkt >> m_sCharackterName;

	if (m_sCharackterName.empty())
		return;

	CUser* pMerchantUser = g_pMain->GetUserPtr(m_sCharackterName, NameType::TYPE_CHARACTER);
	CBot* pMerchantBot = g_pMain->GetBotPtr(m_sCharackterName, NameType::TYPE_CHARACTER);

	if (pMerchantUser == nullptr)
	{
		CBot* pMerchantUser = g_pMain->GetBotPtr(m_sCharackterName, NameType::TYPE_CHARACTER);
		if (pMerchantUser == nullptr
			|| !pMerchantUser->isMerchanting())
			return;

		Packet result;
		string m_sMessage = " ";
		ChatPacket::Construct(&result, (uint8)ChatType::PRIVATE_CHAT, &m_sMessage, &m_sCharackterName);
		Send(&result);
	}
	else
	{
		if (!pMerchantUser->isMerchanting())
			return;

		Packet result;
		string m_sMessage = " ";
		ChatPacket::Construct(&result, (uint8)ChatType::PRIVATE_CHAT, &m_sMessage, &m_sCharackterName);
		Send(&result);
	}
}

void CUser::HSACSX_ReqMerchantListGo(Packet& pkt)
{
	string m_sCharackterName;
	pkt >> m_sCharackterName;

	if (m_sCharackterName.empty())
		return;

	CUser* pMerchantUser = g_pMain->GetUserPtr(m_sCharackterName, NameType::TYPE_CHARACTER);
	if (pMerchantUser == nullptr)
	{
		CBot* pMerchantUser = g_pMain->GetBotPtr(m_sCharackterName, NameType::TYPE_CHARACTER);
		if (pMerchantUser == nullptr
			|| !pMerchantUser->isMerchanting()
			|| pMerchantUser->GetMap() == nullptr
			|| pMerchantUser->GetMap()->m_bMenissiahList != 1)
			return;

		ZoneChange(pMerchantUser->GetZoneID(), pMerchantUser->GetX(), pMerchantUser->GetZ());
	}
	else
	{
		if (!pMerchantUser->isMerchanting()
			|| pMerchantUser->GetMap() == nullptr
			|| pMerchantUser->GetMap()->m_bMenissiahList != 1)
			return;
		
		ZoneChange(pMerchantUser->GetZoneID(), pMerchantUser->GetX(), pMerchantUser->GetZ());
	}
}

void CUser::HSACSX_ReqMerchantListSend(Packet& pkt)
{
	std::vector<uint16_t> MerchantList;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::MERCHANTLIST);
#endif
	uint32 m_sMerchantCount = 0;

	for (uint16 i = 0; i < NPC_BAND; i++)
	{
		if (i < MAX_USER)
		{
			CUser* pUserMerchant = g_pMain->GetUserPtr(i);
			if (pUserMerchant == nullptr)
				continue;

			if (!pUserMerchant->isMerchanting())
				continue;

			for (int j = 0; j < MAX_MERCH_ITEMS; j++)
			{
				_MERCH_DATA* pMerch = &pUserMerchant->m_arMerchantItems[j];
				if (pMerch->nNum == 0 
					|| pMerch->nPrice == 0)
					continue;

				m_sMerchantCount++;
			}
			MerchantList.push_back(pUserMerchant->GetID());
		}
		else
		{
			CBot* pBotMerchant = g_pMain->GetBotPtr(i);
			if (pBotMerchant == nullptr)
				continue;

			if (!pBotMerchant->isMerchanting())
				continue;

			for (int j = 0; j < MAX_MERCH_ITEMS; j++)
			{
				_MERCH_DATA* pMerch = &pBotMerchant->m_arMerchantItems[j];
				if (pMerch->nNum == 0 
					|| pMerch->nPrice == 0)
					continue;

				m_sMerchantCount++;
			}
			MerchantList.push_back(pBotMerchant->GetID());
		}
	}

	if (m_sMerchantCount > 0)
	{
		if (MerchantList.size() > 0)
		{
#if(__VERSION < 2369)
			result << m_sMerchantCount;
#endif
			foreach(itr, MerchantList)
			{
				CUser* pMerchantUser = g_pMain->GetUserPtr((*itr));
				if (pMerchantUser == nullptr)
				{
					CBot* pMerchantBot = g_pMain->GetBotPtr((*itr));
					if (pMerchantBot == nullptr
						|| pMerchantBot->GetMap() == nullptr
						|| pMerchantBot->GetMap()->m_bMenissiahList != 1)
						continue;

					for (int j = 0; j < MAX_MERCH_ITEMS; j++)
					{
						_MERCH_DATA* pMerchbot = &pMerchantBot->m_arMerchantItems[j];
						if (pMerchbot->nNum == 0
							|| pMerchbot->nPrice == 0)
							continue;
#if(__VERSION < 2369)
						result << pMerchantBot->GetID() 
							<< uint32(pMerchantBot->GetID())
							<< pMerchantBot->GetName()
							<< uint8((pMerchantBot->isSellingMerchant() ? uint8(0) : uint8(1)))
							<< pMerchbot->nNum 
							<< pMerchbot->sCount 
							<< pMerchbot->nPrice
							<< pMerchbot->isKC
							<< pMerchantBot->GetX() 
							<< pMerchantBot->GetY() 
							<< pMerchantBot->GetZ();
#endif
					}
				}
				else
				{
					if (pMerchantUser->GetMap() == nullptr
						|| pMerchantUser->GetMap()->m_bMenissiahList != 1)
						continue;

					for (int j = 0; j < MAX_MERCH_ITEMS; j++)
					{
						_MERCH_DATA* pMerch = &pMerchantUser->m_arMerchantItems[j];
						if (pMerch->nNum == 0 
							|| pMerch->nPrice == 0)
							continue;
#if(__VERSION < 2369)
						result << pMerchantUser->GetID() 
							<< uint32(pMerchantUser->GetID()) 
							<< pMerchantUser->GetName()
							<< uint8((pMerchantUser->isSellingMerchant() ? uint8(0) : uint8(1)))
							<< pMerch->nNum 
							<< pMerch->sCount 
							<< pMerch->nPrice 
							<< pMerch->isKC
							<< pMerchantUser->GetX() 
							<< pMerchantUser->GetY()
							<< pMerchantUser->GetZ();
#endif
					}
				}
			}
		}
#if(__VERSION < 2369)
		Send(&result);
#endif
	}
}

void CUser::MyInfo()
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

	AchieveNormalCountAdd(UserAchieveNormalTypes::AchieveBecomeKing, 0, nullptr);

	Packet result(WIZ_MYINFO);

	// Load up our user rankings (for our NP symbols).
	g_pMain->GetUserRank(this);

	// Are we the King? Let's see, shall we?
	CKingSystem* pData = g_pMain->m_KingSystemArray.GetData(GetNation());
	if (pData != nullptr && STRCASECMP(pData->m_strKingName.c_str(), m_strUserID.c_str()) == 0)
		m_bRank = 1; // We're da King, man.
	else
		m_bRank = 0; // totally not da King.

	result.SByte(); // character name has a single byte length
	result << GetSocketID()
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
		<< m_bIsHidingHelmet
		<< m_bIsHidingCospre
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
		{
			if (GetNation() == (uint8)Nation::ELMORAD)
				result << uint64(0) << uint16(KingCapeType::KNIGHTS_HUMAN_KING_CAPE) << uint32(0);
			else
				result << uint64(0) << uint16(KingCapeType::KNIGHTS_KARUS_KING_CAPE) << uint32(0);
		}
		else
			result << uint64(0) << uint16(-1) << uint32(0);
	}
	else
	{
		result << pKnights->GetAllianceID() << pKnights->m_byFlag << pKnights->GetName() << pKnights->m_byGrade << pKnights->m_byRanking << pKnights->m_sMarkVersion;

		CKnights* pMainClan = g_pMain->GetClanPtr(pKnights->GetAllianceID());
		_KNIGHTS_ALLIANCE* pAlliance = g_pMain->GetAlliancePtr(pKnights->GetAllianceID());

		if (pKnights->isInAlliance() && pMainClan != nullptr && pAlliance != nullptr)
		{
			if (isKing())
			{
				if (GetNation() == (uint8)Nation::ELMORAD)
					result << uint16(KingCapeType::KNIGHTS_HUMAN_KING_CAPE) << uint32(0);
				else
					result << uint16(KingCapeType::KNIGHTS_KARUS_KING_CAPE) << uint32(0);
			}
			else
			{
				if (pMainClan->isCastellanCape())
				{
					if (pAlliance->sMainAllianceKnights == pKnights->GetID())
						result << pMainClan->m_sCastCapeID << pMainClan->m_bCastCapeR << pMainClan->m_bCastCapeG << pMainClan->m_bCastCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
					else if (pAlliance->sSubAllianceKnights == pKnights->GetID())
						result << pMainClan->m_sCastCapeID << pKnights->m_bCastCapeR << pKnights->m_bCastCapeG << pKnights->m_bCastCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
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
		else
		{
			if (isKing())
			{
				if (GetNation() == (uint8)Nation::ELMORAD)
					result << uint16(KingCapeType::KNIGHTS_HUMAN_KING_CAPE) << uint32(0);
				else
					result << uint16(KingCapeType::KNIGHTS_KARUS_KING_CAPE) << uint32(0);
			}
			else
			{
				if (pKnights->isCastellanCape())
					result << pKnights->m_sCastCapeID << pKnights->m_bCastCapeR << pKnights->m_bCastCapeG << pKnights->m_bCastCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
				else
					result << pKnights->GetCapeID() << pKnights->m_bCapeR << pKnights->m_bCapeG << pKnights->m_bCapeB << ((pKnights->m_byFlag > 1 && pKnights->m_byGrade < 3) ? uint8(2) : uint8(0));
			}
		}
	}
	result << uint8(0) << uint8(0) << uint8(0) << uint8(0) // unknown
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
		_ITEM_DATA* pItem = GetItem(i);
		if (pItem == nullptr) continue;

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
		_PREMIUM_DATA* pPreData = itr->second;
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

		result << pPreData->bPremiumType
			<< TimeShow;
	}
	result << m_bPremiumInUse
		<< uint8(m_bIsChicken) // chicken/beginner flag
		<< m_iMannerPoint
		<< g_pMain->m_sKarusMilitary
		<< g_pMain->m_sHumanMilitary
		<< g_pMain->m_sKarusEslantMilitary
		<< g_pMain->m_sHumanEslantMilitary
		<< g_pMain->m_sMoradonMilitary
		<< uint8(0)
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
		<< m_bIsHidingWings;

	SendCompressed(&result);
	SetZoneAbilityChange(GetZoneID());

	if (g_pMain->ClanPrePazar && sClanPremStatus)
		m_bPremiumMerchant = true;
}

void CUser::HSACSX_Reset(Packet& pkt)
{
	if (!isInGame() 
		|| isMerchanting()
		|| isMining()
		|| isDead() 
		|| isFishing()
		|| GetZoneID() == ZONE_CHAOS_DUNGEON
		|| GetZoneID() == ZONE_KNIGHT_ROYALE
		|| GetZoneID() == ZONE_DUNGEON_DEFENCE
		|| GetZoneID() == ZONE_PRISON)
		return;

	uint8 subCode = pkt.read<uint8>();

	switch (subCode)
	{
	case 1:
		AllSkillPointChange();
		break;
	case 2:
		AllPointChange();
		break;
	default:
		break;
	}
}

void CUser::HSACSX_StayAlive(Packet& pkt)
{
	if (isCheckDecated)
		return;

	uint8 ischeckdecated2;
	uint32 clock1, clock2, clock3;
	std::string public_key, uPublic_key;

	std::string accountid = GetAccountName();

	pkt.DByte();
	pkt >> clock1 >> clock2 >> uPublic_key >> ischeckdecated2 >> clock3;
	if (accountid.size()) STRTOUPPER(accountid);

	if (ischeckdecated2)
	{
		isCheckDecated = true;
		Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::CheatLog));
		g_pMain->AddDatabaseRequest(newpkt, this);
		return goDisconnect("checkdecated1", __FUNCTION__);
	}

	public_key = md5("1X" + std::to_string(HSACSX_VERSION) + "50001" + std::to_string(clock1) + std::to_string(ischeckdecated2) + accountid);

	if (public_key.empty() || uPublic_key.empty())
		return goDisconnect("heart beat md5 encrypt error", __FUNCTION__);

	if (m_bSelectedCharacter && public_key != uPublic_key)
	{
		g_pMain->SendHelpDescription(this, "Version mismatch. Please update your game or reinstall.");
		return goDisconnect("heart beat md5 encrypt2 error", __FUNCTION__);
	}

	if (isInGame())
	{
		if (lastTickTime == clock1)
		{
			Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::CheatLog));
			g_pMain->AddDatabaseRequest(newpkt, this);
			return goDisconnect("lastticktime1", __FUNCTION__);

			if (lastTickTime2 == clock1)
			{
				Packet newpkt(WIZ_DB_SAVE_USER, uint8(ProcDbType::CheatLog));
				g_pMain->AddDatabaseRequest(newpkt, this);
				return goDisconnect("lastticktime2", __FUNCTION__);
			}
			lastTickTime2 = lastTickTime;
		}
		else lastTickTime = clock1;
	}
	HSACSX_LASTCHECK = UNIXTIME;
}

void CUser::HSACSX_ReqUserInfo(Packet& pkt)
{
	uint32 moneyReq = 0;
	if (GetPremium() == 12)
		moneyReq = 0;
	else
	{
		moneyReq = (int)pow((GetLevel() * 2.0f), 3.4f);
		if (GetLevel() < 30)
			moneyReq = (int)(moneyReq * 0.4f);
		else if (GetLevel() >= 60)
			moneyReq = (int)(moneyReq * 1.5f);

		if ((g_pMain->m_sDiscount == 1
			&& g_pMain->m_byOldVictory == GetNation())
			|| g_pMain->m_sDiscount == 2)
			moneyReq /= 2;
	}
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::USERINFO) << uint32(m_nKnightCash) << m_nTLBalance << uint16(m_sDaggerR) << uint16(m_sAxeR) << uint16(m_sSwordR) << uint16(m_sClubR) << uint16(m_sSpearR) << uint16(m_sBowR) << uint16(m_sJamadarR) << uint32(moneyReq);
	result << m_bFlamelevel;
	Send(&result);
#endif
}

void CUser::HSACSX_ClanBank(Packet& pkt)
{
	return;
	Packet result(WIZ_WAREHOUSE);
	result << uint8_t(ClanBankOpcodes::ClanBankOpen);

	uint8_t ReturnValue = 1;

	if (!isInGame() 
		|| isDead() 
		|| isTrading() 
		|| isStoreOpen()
		|| isMerchanting()
		|| isMining()
		|| isFishing())
	{
		ReturnValue = 0;
		goto fail_return;
	}

	uint16 targetid = 0;
	for (int i = 0; i < 50; i++)
	{
		if (g_pMain->ClanBankGetZone[i] == GetZoneID())
		{
			targetid = g_pMain->ClanBankGetID[i];
			break;
		}
	}

	if (!targetid)
	{
		ReturnValue = 0; goto fail_return;
	}

	m_targetID = targetid;

	auto* pNpc = g_pMain->GetNpcPtr(GetTargetID(), GetZoneID());
	if (pNpc == nullptr || pNpc->GetProtoID() != 19999)
	{
		ReturnValue = 0;
		goto fail_return;
	}

	//Clan Bankasý Premiumlu veya Premiumsuz Acilsin 15.07.2020 start
	{
		if (g_pMain->pServerSetting.ClanBankWithPremium)
		{
			if (m_bIsLoggingOut 
				|| !isInClan() 
				|| !sClanPremStatus
				|| !g_pMain->ClanBankStatus)
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
		else
		{
			if (m_bIsLoggingOut || !isInClan() || !g_pMain->ClanBankStatus)
			{
				ReturnValue = 0;
				goto fail_return;
			}
		}
	}

	auto* pKnights = g_pMain->GetClanPtr(GetClanID());
	if (pKnights == nullptr)
	{
		g_pMain->SendHelpDescription(this, "Clan Bank : Couldn't Identify Clan.");
		ReturnValue = 0;
		goto fail_return;
	}

	result << ReturnValue << pKnights->GetClanInnCoins();

	for (int32_t i = 0; i < WAREHOUSE_MAX; i++)
	{
		auto* pItem = &pKnights->m_sClanWarehouseArray[i];
		if (pItem == nullptr)
			continue;

		if (pItem->nExpirationTime != 0 && pItem->nExpirationTime < (uint32)UNIXTIME)
			memset(pItem, 0, sizeof(_ITEM_DATA));

		result << pItem->nNum << pItem->sDuration << pItem->sCount << pItem->bFlag << uint32(0) << pItem->nExpirationTime;
	}
	Send(&result);

	g_pMain->SendHelpDescription(this, "Clan Bank : Your clan bank is opened.");
	return;

fail_return:
	result << ReturnValue;
	Send(&result);
}

void CUser::HSACSX_AuthInfo(Packet& pkt)
{
	uint8 u_auth;
	pkt >> u_auth;
	if (&u_auth != nullptr)
	{
		if (u_auth == (uint8)AuthorityTypes::AUTHORITY_GAME_MASTER && !isGM())
		{
			string graphicCards, processor, ip;
			pkt >> graphicCards >> processor;
			ip = GetRemoteIP();
			printf("%s is currently disconnect due to editing memory.", GetName().c_str());
			g_pMain->SendFormattedNotice("%s is currently disconnect due to editing memory.", (uint8)Nation::ALL, GetName().c_str());
#if(__VERSION < 2369)
			Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::BANSYSTEM));
			result << uint8(1);
			Send(&result);
#endif
			return goDisconnect(string_format("Wallhack %s %s %s %s %s", GetAccountName().c_str(), GetName().c_str(), graphicCards.c_str(), processor.c_str(), ip.c_str()), __FUNCTION__);
		}
	}
	else
		printf("%s is currently disconnect due to editing memory2.", GetName().c_str());
}

void CUser::HSACSX_ProcInfo(Packet& pkt)
{
	bool m_sPacket = false;
	int16 m_sSocketID = -1;
	pkt >> m_sSocketID;
	CUser* pTUser = nullptr;

	pTUser = g_pMain->GetUserPtr(m_sSocketID);
	if (pTUser == nullptr)
		return;

	if (pTUser->isGM())
		m_sPacket = true;

	vector<ProcessInfo> process;
	uint32 size;
	pkt >> size;
	for (uint32 i = 0; i < size; i++)
	{
		int id, windowsize;
		string name;
		vector<string> windows;
		pkt >> id >> name >> windowsize;

		for (int j = 0; j < windowsize; j++)
		{
			string windowtitle;
			pkt >> windowtitle;
			windows.push_back(windowtitle);
		}
		process.push_back(ProcessInfo(id, name, windows));
	}

	if (m_sPacket)
	{
		g_pMain->SendHelpDescription(pTUser, string_format("----- [%s] Processes ----", GetName().c_str()));
		for (ProcessInfo proc : process)
		{
			for (string window : proc.windows)
				g_pMain->SendHelpDescription(pTUser, string_format("    -- %s", window.c_str()));
		}
		g_pMain->SendHelpDescription(pTUser, "----- End of processes ----");
	}
}

void CUser::HSACSX_SendProcessInfoRequest(CUser* toWHO)
{
#if(__VERSION < 2369)
	Packet pkt(WIZ_HSACS_HOOK);
	pkt << uint8(HSACSXOpCodes::PROCINFO);

	if (toWHO != nullptr)
		pkt << uint16(toWHO->GetID());
	else
		pkt << uint16(0);

	Send(&pkt);
#endif
}

void CUser::HSACSX_Log(Packet& pkt)
{
	string log, graphicCards, processor, ip;
	pkt >> log >> graphicCards >> processor;
	ip = GetRemoteIP();
	if (&log != nullptr)
	{
		/*LOG* pLog = new LOG();
		pLog->table = "PEARL_LOG";
		pLog->sql = string_format("'%s', '%s', '%s', '%s', '%s', '%s', GetDate()", GetAccountName().c_str(), GetName().c_str(), log.c_str(), graphicCards.c_str(), processor.c_str(), ip.c_str());
		LogThread::AddRequest(pLog);*/
	}
}

void CUser::HSACSX_Support(Packet& pkt)
{
	if (HSACSX_LASTSUPPORT + (HSACSX_SUPPORT_CHEK) >= UNIXTIME)
		return;

	uint8 subCode;
	pkt >> subCode;

	string subject, message, ReportType;
	pkt >> subject >> message;
	if (subject.size() > 25 || message.size() > 40)
		return;

	if (subCode == 0x00) ReportType = "Bug";
	else if (subCode == 0x01) ReportType = "Koxp";

	HSACSX_LASTSUPPORT = UNIXTIME;
	Packet report(WIZ_DB_SAVE_USER, uint8(ProcDbType::UserReportDbSave));
	report << GetName() << ReportType << subject << message;
	g_pMain->AddDatabaseRequest(report, this);
}

void CUser::HSACSX_UIRequest(Packet& pkt)
{
	int moneyReq = 0;
	if (GetPremium() == 12)
		moneyReq = 0;
	else
	{
		moneyReq = (int)pow((GetLevel() * 2.0f), 3.4f);
		if (GetLevel() < 30)
			moneyReq = (int)(moneyReq * 0.4f);
		else if (GetLevel() >= 60)
			moneyReq = (int)(moneyReq * 1.5f);

		if ((g_pMain->m_sDiscount == 1 && g_pMain->m_byOldVictory == GetNation()) || g_pMain->m_sDiscount == 2)
			moneyReq /= 2;
	}
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK);
	result << uint8(HSACSXOpCodes::UIINFO) << uint32(m_nKnightCash) << m_nTLBalance << uint16(m_sDaggerR) << uint16(m_sAxeR) << uint16(m_sSwordR) << uint16(m_sClubR) << uint16(m_sSpearR) << uint16(m_sBowR) << uint16(m_sJamadarR) << uint32(moneyReq);
	result << m_iExp << m_iMaxExp << GetSocketID() << GetName() << GetClass() << GetRace() << GetLevel();
	for (uint8 i = 0; i < (uint8)StatType::STAT_COUNT; i++) result << m_bStats[i];

	uint8 r = GetRValue(GetTagNameColour()), g = GetGValue(GetTagNameColour()), b = GetBValue(GetTagNameColour());
	result << mytagname << r << g << b << GetZoneID() << m_bFlamelevel;
	Send(&result);

	Packet pus(HSACSXOpCodes::PUS);
	pus << uint8(0);
	HSACSX_PusRequest(pus);

	HSACSX_SendTempItems();
#endif
}

void CUser::HSACSX_SendTempItems()
{
	if (!TempItemsControl)
	{
#if(__VERSION < 2369)
		Packet result(WIZ_HSACS_HOOK);
		result << uint8(HSACSXOpCodes::TEMPITEMS);
#endif
		vector<_TempItem> items;

		for (int Inventory = SLOT_MAX; Inventory < INVENTORY_COSP; Inventory++)
		{
			auto* pData = GetItem(Inventory);
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 0;
			item.nItemID = pData->nNum;
			item.pos = Inventory;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		for (int Cospre = INVENTORY_COSP; Cospre < INVENTORY_MBAG; Cospre++)
		{
			auto* pData = GetItem(Cospre);
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 1;
			item.nItemID = pData->nNum;
			item.pos = Cospre;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		for (int MagicBag = INVENTORY_MBAG1; MagicBag < INVENTORY_MBAG2; MagicBag++)
		{
			auto* pData = GetItem(MagicBag);
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 2;
			item.nItemID = pData->nNum;
			item.pos = MagicBag;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		for (int Warehouse = 0; Warehouse < WAREHOUSE_MAX; Warehouse++)
		{
			auto* pData = &m_sWarehouseArray[Warehouse];
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 3;
			item.nItemID = pData->nNum;
			item.pos = Warehouse;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}

		for (int Vip = 0; Vip < VIPWAREHOUSE_MAX; Vip++)
		{
			auto* pData = &m_sVIPWarehouseArray[Vip];
			if (pData == nullptr || pData->nNum == 0 || pData->nExpirationTime <= 0)
				continue;

			auto pTable = g_pMain->GetItemPtr(pData->nNum);
			if (pTable.isnull())
				continue;

			_TempItem item;
			item.slot = 4;
			item.nItemID = pData->nNum;
			item.pos = Vip;
			item.time = pData->nExpirationTime;
			items.push_back(item);
		}
#if(__VERSION < 2369)
		result << uint8(items.size());
		foreach(itr, items)
			result << itr->slot << itr->nItemID << itr->pos << itr->time;

		Send(&result);
#endif
		TempItemsControl = true;
	}
}

void CUser::HSACSX_SaveLootSettings(Packet& pkt)
{
	uint8 iWeapon, iArmor, iAccessory, iNormal, iUpgrade, iCraft, iRare, iMagic, iUnique, iConsumable;
	uint32 iPrice = 0;

	pkt >> iWeapon >> iArmor >> iAccessory >> iNormal >> iUpgrade >> iCraft >> iRare >> iMagic >> iUnique >> iConsumable >> iPrice;

	if (iWeapon > 1 || iWeapon < 0) iWeapon = 0;
	if (iArmor > 1 || iArmor < 0) iArmor = 0;
	if (iAccessory > 1 || iAccessory < 0) iAccessory = 0;

	if (iNormal > 1 || iNormal < 0) iNormal = 0;
	if (iUpgrade > 1 || iUpgrade < 0) iUpgrade = 0;
	if (iCraft > 1 || iCraft < 0) iCraft = 0;
	if (iRare > 1 || iRare < 0) iRare = 0;
	if (iMagic > 1 || iMagic < 0) iMagic = 0;
	if (iUnique > 1 || iUnique < 0) iUnique = 0;

	if (iConsumable > 1 || iConsumable < 0) iConsumable = 0;

	if (iPrice > 999999999) iPrice = 999999999;

	pUserLootSetting.iWeapon = iWeapon;
	pUserLootSetting.iArmor = iArmor;
	pUserLootSetting.iAccessory = iAccessory;
	pUserLootSetting.iNormal = iNormal;
	pUserLootSetting.iUpgrade = iUpgrade;
	pUserLootSetting.iCraft = iCraft;
	pUserLootSetting.iRare = iRare;
	pUserLootSetting.iMagic = iMagic;
	pUserLootSetting.iUnique = iUnique;
	pUserLootSetting.iConsumable = iConsumable;
	pUserLootSetting.iPrice = iPrice;
}

void CUser::HSACSX_ReqUserData()
{
#if(__VERSION < 2369)
	Packet ret(WIZ_HSACS_HOOK);
	ret << uint8(HSACSXOpCodes::USERDATA) << GetSocketID() << GetName() << GetClass() << GetRace() << GetLevel() << m_bStats[0] << m_bStats[1] << m_bStats[2] << m_bStats[3] << m_bStats[4];
	Send(&ret);
#endif
}

void CUser::HSACSX_Merchant(Packet& pkt)
{
	if (!isSellingMerchantingPreparing() || isBuyingMerchantingPreparing())
		return;

	_ITEM_TABLE pTable = _ITEM_TABLE();
	uint8 subCode;
	pkt >> subCode;

	if (subCode == MERCHANT_ITEM_ADD)
	{
		Packet result(WIZ_MERCHANT, uint8(MERCHANT_ITEM_ADD));
		uint32 nGold, nItemID;
		uint16 sCount;
		uint8 bSrcPos, bDstPos, bMode, isKC;

		pkt >> nItemID >> sCount >> nGold >> bSrcPos >> bDstPos >> bMode >> isKC;
		// TODO: Implement the possible error codes for these various error cases.
		if (!nItemID || !sCount || bSrcPos >= HAVE_MAX || bDstPos >= MAX_MERCH_ITEMS) goto fail_return;

		for (int i = 0; i < MAX_MERCH_ITEMS; i++)
			if (m_arMerchantItems[i].nNum && m_arMerchantItems[i].bOriginalSlot == (bSrcPos + SLOT_MAX))
				goto fail_return;

		uint16 minreqcash = g_pMain->pServerSetting.MinKnightCash;
		if (isKC)
		{
			if (minreqcash)
			{
				if (nGold < minreqcash)
				{
					g_pMain->SendHelpDescription(this, string_format("You must enter a minimum of %d Knight Cash to establish a market.", minreqcash).c_str());
					goto fail_return;
				}
			}
			/*Max KC Kaldýrma*/
			/*if (nGold > 5000)
			{
				g_pMain->SendHelpDescription(this, string_format("You must enter a maximum of 5000 Knight Cash to establish a market."));
				goto fail_return;
			}*/
		}

		_ITEM_DATA* pSrcItem = GetItem(bSrcPos + SLOT_MAX);
		pTable = g_pMain->GetItemPtr(nItemID);

		if (pTable.isnull() || !pSrcItem)
			goto fail_return;

		if (nItemID >= ITEM_NO_TRADE_MIN && nItemID <= ITEM_NO_TRADE_MAX // Cannot be traded, sold or stored.
			|| pTable.m_bRace == RACE_UNTRADEABLE // Cannot be traded or sold.
			|| pTable.m_bCountable == 2
			|| (pTable.m_bKind == 255 && sCount != 1)) // Cannot be traded or sold.
			goto fail_return;

		if (pTable.m_bCountable == 1 && sCount > MAX_ITEM_COUNT) goto fail_return;

		if (pSrcItem->nNum != nItemID
			|| pSrcItem->sCount == 0
			|| sCount == 0
			|| pSrcItem->sCount < sCount
			|| pSrcItem->isRented()
			|| pSrcItem->isSealed()
			|| pSrcItem->isBound()
			|| pSrcItem->isDuplicate()
			|| pSrcItem->isExpirationTime())
			goto fail_return;

		_MERCH_DATA* pMerch = &m_arMerchantItems[bDstPos];
		if (pMerch == nullptr || pMerch->nNum != 0)
			goto fail_return;

		pSrcItem->MerchItem = true;

		pMerch->nNum = nItemID;
		pMerch->nPrice = nGold;
		pMerch->sCount = sCount; // Selling Count
		pMerch->bCount = pSrcItem->sCount;

		pMerch->sDuration = pSrcItem->sDuration;
		pMerch->nSerialNum = pSrcItem->nSerialNum; // NOTE: Stackable items will have an issue with this.
		pMerch->bOriginalSlot = bSrcPos + SLOT_MAX;
		pMerch->IsSoldOut = false;
		pMerch->isKC = isKC;

		if (pTable.m_bKind == 255 && !pTable.m_bCountable) pMerch->sCount = 1;

		result << uint16(1) << nItemID << sCount << pMerch->sDuration << nGold << bSrcPos << bDstPos;
		Send(&result);
		return;

	fail_return:
		result << uint16(0) << nItemID << sCount << (uint16)bSrcPos + bDstPos << nGold << bSrcPos << bDstPos;
		Send(&result);
	}
}

void CUser::SendChaoticResult(uint8 result)
{
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::CHAOTIC_EXCHANGE));
	newpkt << result;
	Send(&newpkt);
#endif
}

void CUser::HSACSX_ChaoticExchange(Packet& pkt)
{
	uint16 sNpcID; uint32 nExchangeItemID; uint8 bSrcPos, errorcode = 2, bank = 0, sell = 0, count = 0;
	pkt >> sNpcID >> nExchangeItemID >> bSrcPos >> bank >> sell >> count;

	uint32 coinsreq = g_pMain->pServerSetting.chaoticcoins;
	if (coinsreq && !hasCoins(coinsreq))
	{
		g_pMain->SendHelpDescription(this, string_format("You need %d coins to do the exchange process.", coinsreq));
		return BifrostPieceSendFail(errorcode);
	}

	if (count > 100)
	{
		g_pMain->SendHelpDescription(this, "maximum number of pieces 100.");
		return BifrostPieceSendFail(errorcode);
	}

	bool multiple = count > 1 ? true : false;

	if (!CheckExistItem(nExchangeItemID, count))
	{
		g_pMain->SendHelpDescription(this, "You are not equal to the sufficient number of items.");
		return BifrostPieceSendFail(errorcode);
	}

	for (int i = 0; i < count; i++)
	{
		Packet result(WIZ_ITEM_UPGRADE, (uint8)ItemUpgradeOpcodes::ITEM_BIFROST_EXCHANGE);

		if (!multiple)
		{
			if (m_BeefExchangeTime > UNIXTIME2 || m_sItemWeight >= m_sMaxWeight)
				return BifrostPieceSendFail(errorcode);
			m_BeefExchangeTime = UNIXTIME2 + 750;
		}

		CNpc* pNpc = g_pMain->GetNpcPtr(sNpcID, GetZoneID());
		if (pNpc == nullptr || !isInGame()
			|| !isInRange(pNpc, MAX_NPC_RANGE)
			|| isTrading() || isMerchanting()
			|| isSellingMerchantingPreparing() || isMining()
			|| isFishing() || isDead() || !isInMoradon())
			return BifrostPieceSendFail(errorcode);

		if (pNpc->GetType() != NPC_CHAOTIC_GENERATOR && pNpc->GetType() != NPC_CHAOTIC_GENERATOR2)
			return BifrostPieceSendFail(errorcode);

		auto pTable = g_pMain->GetItemPtr(nExchangeItemID);
		if (pTable.isnull() || !pTable.m_bCountable || pTable.m_iEffect2 != 251)
			return BifrostPieceSendFail(errorcode);

		auto* pItem = GetItem(SLOT_MAX + bSrcPos);
		if (pItem == nullptr || bSrcPos >= HAVE_MAX
			|| pItem->nNum != nExchangeItemID
			|| !pItem->sCount || pItem->isRented()
			|| pItem->isSealed() || pItem->isDuplicate())
			return BifrostPieceSendFail(errorcode);

		uint8 bFreeSlots = 0;
		for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
			if (GetItem(i)->nNum == 0 && ++bFreeSlots >= ITEMS_IN_EXCHANGE_GROUP) break;

		if (bFreeSlots < 1)
		{
			g_pMain->SendHelpDescription(this, "there is not enough space left.");
			return BifrostPieceSendFail(errorcode);
		}

		std::vector<_ITEM_EXCHANGE> mlist;
		g_pMain->m_ItemExchangeArray.m_lock.lock();
		auto m_sItemExchangeArray = g_pMain->m_ItemExchangeArray.m_UserTypeMap;
		g_pMain->m_ItemExchangeArray.m_lock.unlock();

		foreach(itr, m_sItemExchangeArray)
		{
			if (itr->second == nullptr) 
				continue;

			if (itr->second->bRandomFlag != 1 
				&& itr->second->bRandomFlag != 2 
				&& itr->second->bRandomFlag != 3)
				continue;

			if (itr->second->nOriginItemNum[0] == nExchangeItemID) 
				mlist.push_back(*itr->second);
		}

		if (mlist.empty())
			return BifrostPieceSendFail(errorcode);

		int bRandArray[10000]{ 0 }; int offset = 0;
		memset(&bRandArray, 0, sizeof(bRandArray));
		foreach(itr, mlist)
		{
			if (!BifrostCheckExchange(itr->nIndex))
				return BifrostPieceSendFail(errorcode);

			if (itr->bRandomFlag >= 101
				|| !CheckExistItemAnd(itr->nOriginItemNum[0], itr->sOriginItemCount[0], 0, 0, 0, 0, 0, 0, 0, 0))
				continue;

			if (offset >= 9999) break;
			for (int i = 0; i < int(itr->sExchangeItemCount[0] / 5); i++)
			{
				if (i + offset >= 9999) break;
				bRandArray[offset + i] = itr->nExchangeItemNum[0];
			}
			offset += int(itr->sExchangeItemCount[0] / 5);
		}

		if (offset > 9999) offset = 9999;
		uint32 bRandSlot = myrand(0, offset);
		uint32 nItemID = bRandArray[bRandSlot];

		auto pGiveTable = g_pMain->GetItemPtr(nItemID);
		if (pGiveTable.isnull())
			return BifrostPieceSendFail(errorcode);

		bool seslling = false;
		if (sell && pGiveTable.Gettype() != 4) seslling = true;

		if (!seslling && !bank && pGiveTable.m_sWeight + m_sItemWeight >= m_sMaxWeight)
		{
			g_pMain->SendHelpDescription(this, "too much weight in your inventory.");
			return BifrostPieceSendFail(errorcode);
		}

		int8 sItemSlot = FindSlotForItem(nItemID, 1);
		if (sItemSlot < 1)
			return BifrostPieceSendFail(errorcode);

		if (!RobItem(SLOT_MAX + bSrcPos, pTable, 1))
			return BifrostPieceSendFail(errorcode);

		if (seslling)
		{
			uint32 transactionPrice = 0;
			if (pGiveTable.m_iSellNpcType == 1)
				transactionPrice = pGiveTable.m_iSellNpcPrice;
			else if (GetPremium() == 0 && pGiveTable.m_iSellPrice == SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice;
			else if (GetPremium() > 0 && pGiveTable.m_iSellPrice == SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice;
			else if (GetPremium() == 0 && pGiveTable.m_iSellPrice != SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice / 6;
			else if (GetPremium() > 0 && pGiveTable.m_iSellPrice != SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice / 4;

			if (pGiveTable.GetKind() == 255) transactionPrice = 1;

			if (GetCoins() + transactionPrice > COIN_MAX)
			{
				g_pMain->SendHelpDescription(this, "You have too many coins in your inventory.");
				return BifrostPieceSendFail(errorcode);
			}

			GoldGain(transactionPrice);
			SendChaoticResult(2);
			BifrostPieceSendFail(1);
			continue;
		}

		if (bank)
		{
			if (!CheckEmptyWareHouseSlot())
			{
				g_pMain->SendHelpDescription(this, "There is not enough space in your bank.");
				return BifrostPieceSendFail(errorcode);
			}

			WareHouseAddItemProcess(nItemID, 1);
			SendChaoticResult(2);
			BifrostPieceSendFail(1);

			std::string message = string_format("%s has been stored.", pGiveTable.m_sName.c_str());
			SendAcsMessage(message);
			continue;
		}

		if (!GiveItem("Bifrost Exchange", nItemID, 1))
		{
			errorcode = 0;
			return BifrostPieceSendFail(errorcode);
		}

		BeefEffectType beefEffectType = BeefEffectType::EffectNone;

		if (pGiveTable.m_ItemType == 4) beefEffectType = BeefEffectType::EffectWhite;
		else if (pGiveTable.m_ItemType == 5) beefEffectType = BeefEffectType::EffectGreen;
		else beefEffectType = BeefEffectType::EffectRed;

		result << uint8(1) << nItemID << int8(sItemSlot - SLOT_MAX) << nExchangeItemID << bSrcPos << (uint8)beefEffectType;
		Send(&result);

		SendChaoticResult(2);
		if (!multiple)
		{
			Packet newpkt(WIZ_OBJECT_EVENT, (uint8)OBJECT_ARTIFACT);
			newpkt << (uint8)beefEffectType << sNpcID;
			SendToRegion(&newpkt, nullptr, GetEventRoom());
		}
		if (pGiveTable.m_ItemType == 4 || pGiveTable.GetNum() == 379068000) LogosItemNotice(pGiveTable);

		if (coinsreq)
			GoldLose(coinsreq);

		BifrostPieceSendFail(2);
	}
}

void CUser::HSACSX_PusRequest(Packet& pkt)
{
	uint8 process;
	pkt >> process;
	switch (process)
	{
	case 0:
		HSACSX_SendPUS();
		break;
	case 1:
		HSACSX_PUSPurchase(pkt);
		break;
	case 2:
		break;
	case 3:
		PUSGiftPurchase(pkt);
		break;
	default:
		break;
	}
#if 0
	if (process == 0)
	{
		HSACSX_SendPUS();
		return;
	}
	else if (process == 1)
	{
		HSACSX_PUSPurchase(pkt);
		return;
	}
	else if (process == 2)
	{
		return;
	}
#endif
}

void CUser::HSACSX_PUSPurchase(Packet& pkt)
{
	if (pCindWar.isEventUser() && g_pMain->isCindirellaZone(GetZoneID()))
	{
		g_pMain->SendHelpDescription(this, "You can't use power-up-store while Fun Class Event.");
		return;
	}
	if (isMerchanting())
	{
		g_pMain->SendHelpDescription(this, "You can't use power-up-store while merchanting.");
		return;
	}

	if (isTrading())
	{
		g_pMain->SendHelpDescription(this, "You can't use power-up-store while trading.");
		return;
	}

	if (isDead())
		return;

	uint32 item_id;
	uint8 count;
	pkt >> item_id >> count;

	if (count > 28)
		return;

	_PUS_ITEM* item = g_pMain->m_PusItemArray.GetData(item_id);
	if (item == nullptr)
		return;

	uint32 totalCash = item->Price * count;

	if (!item->PriceType)
	{
		if (m_nKnightCash >= totalCash)
		{
			_ITEM_TABLE itemdata = g_pMain->GetItemPtr(item->ItemID);
			if (itemdata.isnull())
				return;

			uint32 sFreeSlot = 0;
			for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
			{
				if (GetItem(i) && GetItem(i)->nNum == 0)
					sFreeSlot++;
			}

			if (sFreeSlot <= count)
				return;

			bool notice = true;
			for (size_t i = 1; i <= count; i++)
			{
				if (!GiveItem("Pus Repurchase", item->ItemID, item->BuyCount, true, 0, _giveitempusinfo(true, item->PriceType, item->Price)))
					continue;

				m_nKnightCash -= item->Price;

				if (notice)
					g_pMain->SendHelpDescription(this, string_format("Congratulations! You have purchased %s for %d Knight Cash.", itemdata.m_sName.c_str(), item->Price).c_str());
#if(__VERSION < 2369)
				Packet result(WIZ_HSACS_HOOK);
				result << uint8(HSACSXOpCodes::CASHCHANGE) << uint32(m_nKnightCash) << uint32(m_nTLBalance);
				Send(&result);
#endif
				Packet Save(WIZ_DB_SAVE_USER, uint8(ProcDbType::UpdateKnightCash));
				g_pMain->AddDatabaseRequest(Save, this);
				notice = false;
			}
		}
	}
	else if (item->PriceType)
	{
		if (m_nTLBalance >= totalCash)
		{
			_ITEM_TABLE itemdata = g_pMain->GetItemPtr(item->ItemID);
			if (itemdata.isnull())
				return;

			uint32 sFreeSlot = 0;
			for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
			{
				if (GetItem(i)->nNum == 0)
					sFreeSlot++;
			}

			if (sFreeSlot <= count)
				return;

			bool notice = true;
			for (size_t i = 1; i <= count; i++)
			{
				if (item->ItemID >= 489500000 && item->ItemID <= 489600000)
				{
					m_nTLBalance -= item->Price;
					GiveBalance(item->BuyCount);
#if(__VERSION < 2369)
					Packet result(WIZ_HSACS_HOOK);
					result << uint8(HSACSXOpCodes::CASHCHANGE) << uint32(m_nKnightCash) << uint32(m_nTLBalance);
					Send(&result);
#endif
					g_pMain->SendHelpDescription(this, string_format("Congratulations! You have purchased Change %d tl balance to %d knight cash", item->Price, item->BuyCount).c_str());
					return;
				}

				if (!GiveItem("Pus Repurchase", item->ItemID, item->BuyCount, true, 0, _giveitempusinfo(true, item->PriceType, item->Price)))
					continue;

				m_nTLBalance -= item->Price;
				if (notice)
					g_pMain->SendHelpDescription(this, string_format("Congratulations! You have purchased %s for %d Tl Balance Cash.", itemdata.m_sName.c_str(), item->Price).c_str());
#if(__VERSION < 2369)
				Packet result(WIZ_HSACS_HOOK);
				result << uint8(HSACSXOpCodes::CASHCHANGE) << uint32(m_nKnightCash) << uint32(m_nTLBalance);
				Send(&result);
#endif
				Packet Save(WIZ_DB_SAVE_USER, uint8(ProcDbType::UpdateKnightCash));
				g_pMain->AddDatabaseRequest(Save, this);
				notice = false;
			}
		}
	}
}

void CUser::HSACSX_SendPUS()
{
	if (!m_sPowerUpStoreControle)
	{
#if(__VERSION < 2369)
		Packet result(WIZ_HSACS_HOOK);
		result << uint8(HSACSXOpCodes::PUS);
#endif
		uint32 PusItemCount = g_pMain->m_PusItemArray.GetSize();
#if(__VERSION < 2369)
		result << PusItemCount;
#endif
		g_pMain->m_PusItemArray.m_lock.lock();
		auto m_sPusItemArray = g_pMain->m_PusItemArray.m_UserTypeMap;
		g_pMain->m_PusItemArray.m_lock.unlock();

		foreach(itr, m_sPusItemArray)
		{
			if (itr->second == nullptr)
				continue;
#if(__VERSION < 2369)
			result << itr->second->ID
				<< itr->second->ItemID
				<< itr->second->Price
				<< itr->second->Cat 
				<< itr->second->BuyCount
				<< itr->second->PriceType;
#endif
		}
#if(__VERSION < 2369)
		Send(&result);
#endif
		if (true)
		{
			g_pMain->m_PusCategoryArray.m_lock.lock();
			auto m_sPusCategoryArray = g_pMain->m_PusCategoryArray.m_UserTypeMap;
			g_pMain->m_PusCategoryArray.m_lock.unlock();
#if(__VERSION < 2369)
			Packet result(WIZ_HSACS_HOOK);
			result << uint8(HSACSXOpCodes::PusCat);
			result << uint32(m_sPusCategoryArray.size());
#endif
			foreach(itr, m_sPusCategoryArray)
			{
				if (itr->second == nullptr)
					continue;
#if(__VERSION < 2369)
				result << uint32(itr->second->ID) 
					<< itr->second->CategoryName 
					<< itr->second->Status;
#endif
			}
#if(__VERSION < 2369)
			Send(&result);
#endif
		}
		m_sPowerUpStoreControle = true;
	}
	else
	{
		m_sPowerUpStoreControle = true;
		return;
	}
	m_sPowerUpStoreControle = true;
}

void CUser::HSACSX_SendAliveRequest()
{
#if(__VERSION < 2369)
	Packet pkt(WIZ_HSACS_HOOK);
	pkt << uint8(HSACSXOpCodes::xALIVE) << uint8(1);
	Send(&pkt);
#endif
}

void CUser::HSACSX_OpenWeb(std::string url)
{
#if(__VERSION < 2369)
	Packet pkt(WIZ_HSACS_HOOK);
	pkt << uint8(HSACSXOpCodes::OPEN) << url;
	Send(&pkt);
#endif
}

void CUser::HSACSX_DropRequest(Packet& pkt)
{
#if (GAME_SOURCE_VERSION == 2369)
	struct DropItem
	{
		uint32 nItemID;
		uint16 sPercent;

		DropItem(uint32 ItemID, uint16 sPer)
		{
			nItemID = ItemID;
			sPercent = sPer;
		}
	};
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK);
#endif
	uint8 command;
	pkt >> command;

	if (command == 1)
	{
		uint8 iasMonster = 0;
#if(__VERSION < 2369)
		result << uint8_t(HSACSXOpCodes::DROP_REQUEST) << uint8(1);
#endif
		uint32 target;
		uint8 isRandom = 0;
		pkt >> target;

		CNpc* pNpc = g_pMain->GetNpcPtr(m_targetID, GetZoneID());
		if (pNpc == nullptr)
			return;

		if (pNpc->isMonster()) iasMonster = 1;

		if (pNpc->isMonster())
		{
			iasMonster = 1;
			_K_MONSTER_ITEM* pItem = g_pMain->m_MonsterItemArray.GetData(pNpc->GetProtoID());
			if (pItem != nullptr)
			{
#if(__VERSION < 2369)
				result << pNpc->GetProtoID();
				for (int j = 0; j < LOOT_DROP_ITEMS; j++)
					result << uint32(pItem->iItem[j]) << uint16(pItem->sPercent[j]);
#endif
			}
		}
		else
		{
			_K_NPC_ITEM* pItem = g_pMain->m_NpcItemArray.GetData(pNpc->GetProtoID());
			if (pItem != nullptr)
			{
#if(__VERSION < 2369)
				result << pNpc->GetProtoID();
				for (int j = 0; j < LOOT_DROP_ITEMS; j++)
					result << uint32(pItem->iItem[j]) << uint16(pItem->sPercent[j]);
#endif
			}
		}
#if(__VERSION < 2369)
		result << iasMonster;
		Send(&result);
#endif
	}
	else if (command == 2)
	{
#if(__VERSION < 2369)
		result << uint8_t(HSACSXOpCodes::DROP_LIST) << uint8(2);
#endif
		uint32 groupID;
		pkt >> groupID;

		_MAKE_ITEM_GROUP* itemGroup = g_pMain->m_MakeItemGroupArray.GetData(groupID);
		if (itemGroup != nullptr)
		{
#if(__VERSION < 2369)
			result << uint8(itemGroup->iItems.size());
			for (uint8 i = 0; i < (uint8)itemGroup->iItems.size(); i++)
				result << itemGroup->iItems[i];

			Send(&result);
#endif
		}
	}
	else if (command == 3)
	{
#if(__VERSION < 2369)
		result << uint8_t(HSACSXOpCodes::DROP_REQUEST) << uint8(1);
#endif
		uint16 mob;
		pkt >> mob;

		std::list<DropItem> mlist;
#if(__VERSION < 2369)
		result << mob;
#endif
		_K_MONSTER_ITEM* pItem = g_pMain->m_MonsterItemArray.GetData(mob);
		if (pItem)
		{
			bool added_random = false;
			for (int j = 0; j < 12; j++)
			{
				if (!pItem->iItem[j])
					continue;

				if (pItem->iItem[j] < MIN_ITEM_ID)
				{
					if (!added_random)
					{
						g_pMain->m_MakeItemGroupRandomArray.m_lock.lock();
						auto m_sMakeItemGroupRandomArray = g_pMain->m_MakeItemGroupRandomArray.m_UserTypeMap;
						g_pMain->m_MakeItemGroupRandomArray.m_lock.unlock();

						foreach(itr, m_sMakeItemGroupRandomArray)
						{
							if (itr->second && itr->second->GroupNo == pItem->iItem[j])
							{
								added_random = true;
								mlist.push_back(DropItem(900004000, 10000));
								break;
							}
						}
					}

					auto* pGrupItems = g_pMain->m_MakeItemGroupArray.GetData(pItem->iItem[j]);
					if (!pGrupItems
						|| pGrupItems->iItems.empty())
						continue;

					for (int i = 0; i < (int)pGrupItems->iItems.size(); i++)
						mlist.push_back(DropItem(pGrupItems->iItems[i], pItem->sPercent[j]));
				}
				else
				{
					mlist.push_back(DropItem(pItem->iItem[j], pItem->sPercent[j]));
				}
			}
		}
#if(__VERSION < 2369)
		result << (uint32)mlist.size();

		for (auto it : mlist)
			result << it.nItemID << it.sPercent;

		result << uint8(1);
		Send(&result);
#endif
	}
	else if (command == 4)
	{
		uint8 crSelect;
		pkt >> crSelect;
#if(__VERSION < 2369)
		result << uint8_t(HSACSXOpCodes::DROP_REQUEST) << uint8(1);
#endif
		if (g_pMain->pCollectionRaceEvent.RewardItemID[crSelect] == 0)
			return;

		std::list<DropItem> mlist;
#if(__VERSION < 2369)
		result << uint16(2);
#endif
		if (g_pMain->m_RandomItemArray.empty())
			return;

		foreach(itr, g_pMain->m_RandomItemArray)
		{
			_RANDOM_ITEM* pRandom = *itr;
			if (pRandom->SessionID != g_pMain->pCollectionRaceEvent.RewardSession[crSelect])
				continue;

			mlist.push_back(DropItem(pRandom->ItemID, 10000));
		}

		if (mlist.empty()) return;
#if(__VERSION < 2369)
		result << (uint32)mlist.size();
		for (auto it : mlist) result << it.nItemID << it.sPercent;
		result << uint8(2);
		Send(&result);
#endif
	}
#else
	Packet result(WIZ_HSACS_HOOK);

	uint8 command;
	pkt >> command;

	if (command == 1)
	{
		uint8 iasMonster = 0;
		result << uint8_t(HSACSXOpCodes::DROP_REQUEST) << uint8(1);
		uint32 target;
		uint8 isRandom = 0;
		pkt >> target;

		CNpc* pNpc = g_pMain->GetNpcPtr(m_targetID, GetZoneID());
		if (pNpc == nullptr)
			return;

		if (pNpc->isMonster()) iasMonster = 1;

		if (pNpc->isMonster())
		{
			iasMonster = 1;
			_K_MONSTER_ITEM* pItem = g_pMain->m_MonsterItemArray.GetData(pNpc->GetProtoID());
			if (pItem != nullptr)
			{
				result << pNpc->GetProtoID();
				for (int j = 0; j < LOOT_DROP_ITEMS; j++)
					result << uint32(pItem->iItem[j]) << uint16(pItem->sPercent[j]);
			}
		}
		else
		{
			_K_NPC_ITEM* pItem = g_pMain->m_NpcItemArray.GetData(pNpc->GetProtoID());
			if (pItem != nullptr)
			{
				result << pNpc->GetProtoID();
				for (int j = 0; j < LOOT_DROP_ITEMS; j++)
					result << uint32(pItem->iItem[j]) << uint16(pItem->sPercent[j]);
			}
		}

		result << iasMonster;
		Send(&result);
	}
	else if (command == 2)
	{
		result << uint8_t(HSACSXOpCodes::DROP_LIST) << uint8(2);

		uint32 groupID;
		pkt >> groupID;

		_MAKE_ITEM_GROUP* itemGroup = g_pMain->m_MakeItemGroupArray.GetData(groupID);
		if (itemGroup != nullptr)
		{
			result << uint8(itemGroup->iItems.size());
			for (uint8 i = 0; i < (uint8)itemGroup->iItems.size(); i++)
				result << itemGroup->iItems[i];

			Send(&result);
		}
	}
	else if (command == 3)
	{
		result << uint8_t(HSACSXOpCodes::DROP_REQUEST) << uint8(1);
		uint16 mob;
		pkt >> mob;

		std::list<uint32> mlist;
		result << mob;

		_K_MONSTER_ITEM* pItem = g_pMain->m_MonsterItemArray.GetData(mob);
		if (pItem)
		{

			bool added_random = false;
			for (int j = 0; j < 12; j++)
			{
				if (!pItem->iItem[j])
					continue;

				if (pItem->iItem[j] < MIN_ITEM_ID)
				{

					if (!added_random)
					{
						g_pMain->m_MakeItemGroupRandomArray.m_lock.lock();
						auto m_sMakeItemGroupRandomArray = g_pMain->m_MakeItemGroupRandomArray.m_UserTypeMap;
						g_pMain->m_MakeItemGroupRandomArray.m_lock.unlock();

						foreach(itr, m_sMakeItemGroupRandomArray)
						{
							if (itr->second && itr->second->GroupNo == pItem->iItem[j])
							{
								added_random = true;
								mlist.push_back(900004000);
								break;
							}
						}
					}

					auto* itemGroup = g_pMain->m_MakeItemGroupArray.GetData(pItem->iItem[j]);
					if (!itemGroup
						|| itemGroup->iItems.empty())
						continue;

					for (int i = 0; i < (int)itemGroup->iItems.size(); i++)
					{
						if (std::find(mlist.begin(), mlist.end(), itemGroup->iItems[i]) == mlist.end())
							mlist.push_back(itemGroup->iItems[i]);
					}
				}
				else
				{
					if (std::find(mlist.begin(), mlist.end(), pItem->iItem[j]) == mlist.end())
						mlist.push_back(pItem->iItem[j]);
				}
			}
		}

		result << (uint32)mlist.size();

		for (auto it : mlist)
			result << it;

		result << uint8(1);
		Send(&result);
	}
	else if (command == 4)
	{
		uint8 crSelect;
		pkt >> crSelect;
		result << uint8_t(HSACSXOpCodes::DROP_REQUEST) << uint8(1);
		if (g_pMain->pCollectionRaceEvent.RewardItemID[crSelect] == 0)
			return;

		std::list<uint32> mlist;
		result << uint16(2);
		foreach(itr, g_pMain->m_RandomItemArray)
		{

			_RANDOM_ITEM* pRandom = *itr;
			if (pRandom->SessionID != g_pMain->pCollectionRaceEvent.RewardSession[crSelect])
				continue;
			mlist.push_back(pRandom->ItemID);

		}
		if (mlist.empty()) return;

		result << (uint32)mlist.size();
		for (auto it : mlist) result << it;
		result << uint8(2);
		Send(&result);
	}
#endif
}

void CUser::HSACSX_Main()
{
	if (!isInGame() || isOfflineStatus()) return;

	if (UNIXTIME - HSACSX_LASTCHECK > HSACSX_ALIVE_TIMEOUT)
	{
		if (HSACSX_ACTIVE == 1)
		{
			//g_pMain->SendHelpDescription(this, "Couldn't connect to ACS server.");
			//Disconnect();
		}
	}
}

void CUser::HSACSX_HandlePacket(Packet& pkt)
{
	uint8 SubOpCode;
	pkt >> SubOpCode;

	switch (SubOpCode)
	{
	case HSACSXOpCodes::PERKS:
		HandlePerks(pkt);
		break;
	case HSACSXOpCodes::CINDIRELLA:
		CindirillaHandler(pkt);
		break;
	case HSACSXOpCodes::xALIVE:
		HSACSX_StayAlive(pkt);
		break;
	case HSACSXOpCodes::AUTHINFO:
		HSACSX_AuthInfo(pkt);
		break;
	case HSACSXOpCodes::PROCINFO:
		HSACSX_ProcInfo(pkt);
		break;
	case HSACSXOpCodes::LOG:
		HSACSX_Log(pkt);
		break;
	case HSACSXOpCodes::UIINFO:
		HSACSX_UIRequest(pkt);
		break;
	case HSACSXOpCodes::PUS:
		HSACSX_PusRequest(pkt);
		break;
	case HSACSXOpCodes::RESET:
		HSACSX_Reset(pkt);
		break;
	case HSACSXOpCodes::DROP_LIST:
		HSACSX_DropRequest(pkt);
		break;
	case HSACSXOpCodes::DROP_REQUEST:
		HSACSX_DropRequest(pkt);
		break;
	case HSACSXOpCodes::CLANBANK:
		HSACSX_ClanBank(pkt);
		break;
	case HSACSXOpCodes::USERINFO:
		HSACSX_ReqUserInfo(pkt);
		break;
	case HSACSXOpCodes::LOOT_SETTINS:
		HSACSX_SaveLootSettings(pkt);
		break;
	case HSACSXOpCodes::CHAOTIC_EXCHANGE:
		HSACSX_ChaoticExchange(pkt);
		break;
	case HSACSXOpCodes::MERCHANT:
		HSACSX_Merchant(pkt);
		break;
	case HSACSXOpCodes::TEMPITEMS:
		HSACSX_SendTempItems();
		break;
	case HSACSXOpCodes::SUPPORT:
		HSACSX_Support(pkt);
		break;
	case HSACSXOpCodes::MERCHANTLIST:
		HSACSX_ReqMerchantSystem(pkt);
		break;
	/*case HSACSXOpCodes::BANSYSTEM:
		HSACSX_HandleLifeSkill(pkt);
		break;*/
	case 22:
		HSACSX_General(pkt);
		break;
	case HSACSXOpCodes::LOTTERY:
		HSACSX_LotteryJoinFunction(pkt);
		break;
	case HSACSXOpCodes::RESETREBSTAT:
	{
		int32_t menuButtonText[MAX_MESSAGE_EVENT], menuButtonEvents[MAX_MESSAGE_EVENT];
		m_sEventNid = 14446;
		m_sEventSid = 18004;
		m_nQuestHelperID = 4324;
		foreach_array_n(i, menuButtonText, MAX_MESSAGE_EVENT)
		{
			menuButtonText[i] = -1;
		}

		foreach_array_n(i, menuButtonEvents, MAX_MESSAGE_EVENT)
		{
			menuButtonEvents[i] = -1;
		}

		SelectMsg(52, -1, -1, menuButtonText, menuButtonEvents);
	}
	break;
	case HSACSXOpCodes::ACCOUNT_INFO_SAVE:
		HSACSX_AccountInfoSave(pkt);
		break;
	case HSACSXOpCodes::CHAT_LASTSEEN:
		HSACSX_LastSeenProcess(pkt);
		break;
	case HSACSXOpCodes::SendRepurchaseMsg:
		HSACSX_SendRepurchaseMsg(pkt);
		break;
	case HSACSXOpCodes::Send1299SkillAndStatReset:
		HSACSX_Send1299SkillAndStatReset(pkt);
		break;
	case HSACSXOpCodes::TagInfo:
		HandleTagChange(pkt);
		break;
	case HSACSXOpCodes::PusRefund:
		HandleItemReturn(pkt);
		break;
	case HSACSXOpCodes::WheelData:
		WheelOfFun(pkt);
		break;
	case HSACSXOpCodes::CHEST_BLOCKITEM:
		HandleChestBlock(pkt);
		break;
#if(GAME_SOURCE_VERSION == 2369 || GAME_SOURCE_VERSION == 1098)
	case HSACSXOpCodes::WIZ_ITEM_EXCHANGE_INFO:
		HandleRightClickExchange(pkt);
		break;
#if(GAME_SOURCE_VERSION == 2369 || GAME_SOURCE_VERSION == 1098)
	case HSACSXOpCodes::WIZ_DAILY_REWARD:
		HandleDailyRewardProcess(pkt);
		break;
#endif
#endif
	default:
		printf("HSACSX_HandlePacket unhandled opcode %d\n", SubOpCode);
		break;
	}
}

#if(GAME_SOURCE_VERSION == 2369 || GAME_SOURCE_VERSION == 1098)
#if(GAME_SOURCE_VERSION == 2369 || GAME_SOURCE_VERSION == 1098)
void CUser::HandleDailyRewardProcess(Packet& pkt)
{
	uint8 subcode = pkt.read<uint8>();
	switch (subcode)
	{
	case 1:
		HandleDailyRewardGive(pkt);
		break;
	default:
		break;
	}
}

void CUser::HandleDailyRewardGive(Packet& pkt)
{
	DateTime strDateTime;
	uint32 sItemID;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_DAILY_REWARD));
#endif
	uint32 m_sItemID[25] = { 0 };
	uint32 sItemCumulativeID[3] = { 0 };
	uint8 sbType[25] = { 0 };
	uint8 sGetDay[25] = { 0 };
	pkt >> sItemID;

	std::vector<ByteBuffer> sItemList;
	g_DBAgent.LoadDailyReward(sItemList);
	if (sItemList.size() > 0)
	{
		for (int i = 0; i < 25; i++)
			sItemList.at(0) >> m_sItemID[i];

		std::vector<ByteBuffer> sItemCumulativeList;
		g_DBAgent.LoadDailyCumulativeReward(sItemCumulativeList);
		if (sItemCumulativeList.size() > 0)
		{
			for (int i = 0; i < 3; i++)
				sItemCumulativeList.at(0) >> sItemCumulativeID[i];

			std::vector<ByteBuffer> bItemList;
			g_DBAgent.LoadDailyRewardUser(GetID(), bItemList);
			if (bItemList.size() > 0)
			{
				for (int i = 0; i < 25; i++)
					bItemList.at(0) >> sbType[i] >> sGetDay[i];

				for (int i = 0; i < 25; i++)
				{
					if (m_sItemID[i] == sItemID && sbType[i] == 0)
					{
						if (i > 0 && sbType[i - 1] == 0)
						{
#if(__VERSION < 2369)
							result << uint8(3);
							Send(&result);
#endif
							return;
						}
						else if (i > 0 && sbType[i - 1] > 0)
						{
							if (sGetDay[i - 1] == strDateTime.GetDay())
							{
#if(__VERSION < 2369)
								result << uint8(3);
								Send(&result);
#endif
								return;
							}

							sbType[i] = 1;
							sGetDay[i] = strDateTime.GetDay();
#if(__VERSION < 2369)
							result << uint8(2);

							for (int i = 0; i < 25; i++)
								result << m_sItemID[i] << sbType[i] << sGetDay[i];

							for (int i = 0; i < 3; i++)
								result << sItemCumulativeID[i];

							Send(&result);
#endif
							GiveItem("DailyReward", sItemID, 1);
							g_DBAgent.UpdateDailyRewardUser(GetID(), sbType, sGetDay);
						}
						else if (i == 0 && sbType[i] == 0)
						{
#if(__VERSION < 2369)
							result << uint8(2);
#endif
							sbType[i] = 1;
							sGetDay[i] = strDateTime.GetDay();
#if(__VERSION < 2369)
							for (int i = 0; i < 25; i++)
								result << m_sItemID[i] << sbType[i] << sGetDay[i];

							for (int i = 0; i < 3; i++)
								result << sItemCumulativeID[i];

							Send(&result);
#endif
							GiveItem("DailyReward", sItemID, 1);
							g_DBAgent.UpdateDailyRewardUser(GetID(), sbType, sGetDay);
						}
					}
				}
			}
		}
	}
}
#endif
void CUser::HandleRightClickExchange(Packet& pkt)
{
	uint8 subcode = pkt.read<uint8>();
	switch (subcode)
	{
	case 1:
		HandleRightClickExchangeSend(pkt);
		break;
	case 2:
		HandleNewRightClickExchangeSend(pkt);
		break;
	case 3:
		HandleNewRightClickExchange(pkt);
		break;
	case 4:
		HandleNewRightClickGiveExchange(pkt);
		break;
	case 5:
		HandleNewRightClickGeneratorExchange(pkt);
		break;
	default:
		printf("HandleRightClickExchange unhandled opcode %d\n", subcode);
		break;
	}
}

#if(GAME_SOURCE_VERSION == 1098)
void CUser::HandleNewRightClickGeneratorSystem(Packet& pkt)
{
	uint32 nItemID = pkt.read<uint32>();
	uint8 sSize = RightExchangeInventoySlot(nItemID);
	_ITEM_DATA* pItem = GetItem(sSize);
	if (pItem == nullptr
		|| pItem->GetNum() != nItemID)
		return;

	Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(1) << uint8(2) << sSize << pItem->GetNum();
	Send(&result);
}
#endif

void CUser::HandleNewRightClickExchangeSend(Packet& pkt)
{
	uint8 sSize = pkt.read<uint8>();
	_ITEM_DATA* pItem = GetItem(sSize);

	if (pItem == nullptr)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(1) << uint8(2) << sSize << pItem->GetNum();
	Send(&result);
#endif
}

void CUser::HandleRightClickExchangeSend(Packet& pkt)
{
	uint8 sSize = pkt.read<uint8>();
	_ITEM_DATA* pItem = GetItem(sSize);

	if (pItem == nullptr)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(1) << uint8(1) << pItem->GetNum();
	Send(&result);
#endif
}

void CUser::HandleNewRightClickExchange(Packet& pkt)
{
	uint8 subcode = pkt.read<uint8>();
	switch (subcode)
	{
	case 1:
		HandleNewRightClickExchangeReward(pkt);
		break;
	case 2:
		HandleNewRightClickExchangeAll(pkt);
		break;
	case 3:
		HandleNewRightClickExchangePremium(pkt);
		break;
	case 4:
		HandleNewRightClickExchangeKnightCash(pkt);
		break;
#if(GAME_SOURCE_VERSION == 1098)
	case 5:
		HandleNewRightClickGeneratorSystem(pkt);
		break;
#endif
	case 6:
		HandleNewRightClickExchangeGenie(pkt);
		break;
	case 7:
		HandleNewRightClickExchangeKnightTL(pkt);
		break;
	default:
		TRACE("HandleNewRightClickExchange unhandled opcode %d\n", subcode);
		break;
	}
}
void CUser::HandleNewRightClickGeneratorExchange(Packet& pkt)
{
	uint32 nExchangeItemID; uint8 bSrcPos, errorcode = 2, bank = 0, sell = 0, count = 0;
	pkt >> nExchangeItemID >> bSrcPos >> bank >> sell >> count;

	uint32 coinsreq = g_pMain->pServerSetting.chaoticcoins;
	if (coinsreq && !hasCoins(coinsreq))
	{
		g_pMain->SendHelpDescription(this, string_format("You need %d coins to do the exchange process.", coinsreq));
		return;
	}

	if (count > 100)
	{
		g_pMain->SendHelpDescription(this, "maximum number of pieces 100.");
		return;
	}

	bool multiple = count > 1 ? true : false;

	if (!CheckExistItem(nExchangeItemID, count))
	{
		g_pMain->SendHelpDescription(this, "You are not equal to the sufficient number of items.");
		return;
	}

	for (int i = 0; i < count; i++)
	{
		if (!multiple)
		{
			if (m_BeefExchangeTime > UNIXTIME2 
				|| m_sItemWeight >= m_sMaxWeight)
				return;

			m_BeefExchangeTime = UNIXTIME2 + 750;
		}

		auto pTable = g_pMain->GetItemPtr(nExchangeItemID);
		if (pTable.isnull() 
			|| !pTable.m_bCountable)
			return;
#if(GAME_SOURCE_VERSION == 2369 || GAME_SOURCE_VERSION == 1098)

#if (GAME_SOURCE_VERSION == 1098)
		if (bSrcPos == 0)
			bSrcPos = RightExchangeInventoySlot(nExchangeItemID);
#endif
		auto* pItem = GetItem(bSrcPos);
		if (pItem == nullptr
			|| pItem->nNum != nExchangeItemID
			|| !pItem->sCount
			|| pItem->isRented()
			|| pItem->isSealed()
			|| pItem->isDuplicate())
			return;
#endif
		uint8 bFreeSlots = 0;
		for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
			if (GetItem(i)->nNum == 0 && ++bFreeSlots >= ITEMS_IN_EXCHANGE_GROUP) break;

		if (bFreeSlots < 1)
		{
			g_pMain->SendHelpDescription(this, "there is not enough space left.");
			return;
		}

		std::vector<_ITEM_EXCHANGE> mlist;
		g_pMain->m_ItemExchangeArray.m_lock.lock();
		auto m_sItemExchangeArray = g_pMain->m_ItemExchangeArray.m_UserTypeMap;
		g_pMain->m_ItemExchangeArray.m_lock.unlock();

		foreach(itr, m_sItemExchangeArray)
		{
			if (itr->second == nullptr)
				continue;

			if (itr->second->bRandomFlag != 1
				&& itr->second->bRandomFlag != 2
				&& itr->second->bRandomFlag != 3
				&& itr->second->bRandomFlag != 101)
				continue;

			if (itr->second->nOriginItemNum[0] == nExchangeItemID)
				mlist.push_back(*itr->second);
		}

		if (mlist.empty())
			return;

		int bRandArray[10000]{ 0 }; int offset = 0;
		memset(&bRandArray, 0, sizeof(bRandArray));
		foreach(itr, mlist)
		{
			if (!BifrostCheckExchange(itr->nIndex))
				return;

			if (itr->bRandomFlag >= 102
				|| !CheckExistItemAnd(itr->nOriginItemNum[0], itr->sOriginItemCount[0], 0, 0, 0, 0, 0, 0, 0, 0))
				continue;

			if (itr->bRandomFlag == 101)
			{
				if (offset >= 9999)
					break;

				for (int i = 0; i < ITEMS_IN_ORIGIN_GROUP; i++)
				{
					for (int n = 0; n < int(itr->sExchangeItemCount[i] / 900); n++)
					{
						if (n + offset >= 9999)
							break;

						bRandArray[offset + n] = itr->nExchangeItemNum[i];
					}
					offset += int(itr->sExchangeItemCount[i] / 900);
				}
			}
			else
			{
				if (offset >= 9999) 
					break;

				for (int i = 0; i < int(itr->sExchangeItemCount[0] / 5); i++)
				{
					if (i + offset >= 9999) break;
					bRandArray[offset + i] = itr->nExchangeItemNum[0];
				}
				offset += int(itr->sExchangeItemCount[0] / 5);
			}
		}

		if (offset > 9999) offset = 9999;
		uint32 bRandSlot = myrand(0, offset);
		uint32 nItemID = bRandArray[bRandSlot];

		auto pGiveTable = g_pMain->GetItemPtr(nItemID);
		if (pGiveTable.isnull())
			return;

		bool seslling = false;
		if (sell && pGiveTable.Gettype() != 4) seslling = true;

		if (!seslling 
			&& !bank 
			&& pGiveTable.m_sWeight + m_sItemWeight >= m_sMaxWeight)
			g_pMain->SendHelpDescription(this, "too much weight in your inventory.");

		int8 sItemSlot = FindSlotForItem(nItemID, 1);
		if (sItemSlot < 1)
			return;

		if (!RobItem(bSrcPos, pTable, 1))
			return;

		if (seslling)
		{
			uint32 transactionPrice = 0;
			if (pGiveTable.m_iSellNpcType == 1)
				transactionPrice = pGiveTable.m_iSellNpcPrice;
			else if (GetPremium() == 0 && pGiveTable.m_iSellPrice == SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice;
			else if (GetPremium() > 0 && pGiveTable.m_iSellPrice == SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice;
			else if (GetPremium() == 0 && pGiveTable.m_iSellPrice != SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice / 6;
			else if (GetPremium() > 0 && pGiveTable.m_iSellPrice != SellTypeFullPrice)
				transactionPrice = pGiveTable.m_iBuyPrice / 4;

			if (pGiveTable.GetKind() == 255) transactionPrice = 1;

			if (GetCoins() + transactionPrice > COIN_MAX)
			{
				g_pMain->SendHelpDescription(this, "You have too many coins in your inventory.");
				return BifrostPieceSendFail(errorcode);
			}

			GoldGain(transactionPrice);
			continue;
		}

		if (bank)
		{
			if (!CheckEmptyWareHouseSlot())
			{
				g_pMain->SendHelpDescription(this, "There is not enough space in your bank.");
				return;
			}

			WareHouseAddItemProcess(nItemID, 1);
			std::string message = string_format("%s has been stored.", pGiveTable.m_sName.c_str());
			SendAcsMessage(message);
			continue;
		}

		if (!GiveItem("Right Exchange No Packet Exchange", nItemID, 1))
			return;

		BeefEffectType beefEffectType = BeefEffectType::EffectNone;

		if (pGiveTable.m_ItemType == 4) 
			beefEffectType = BeefEffectType::EffectWhite;
		else if (pGiveTable.m_ItemType == 5) 
			beefEffectType = BeefEffectType::EffectGreen;
		else 
			beefEffectType = BeefEffectType::EffectRed;

		if (pGiveTable.m_ItemType == 4 
			|| pGiveTable.GetNum() == 379068000)
			LogosItemNotice(pGiveTable);

		if (coinsreq)
			GoldLose(coinsreq);
	}
}

#if(GAME_SOURCE_VERSION == 1098)
uint8 CUser::RightExchangeInventoySlot(uint32 sItemID)
{
	uint8 bFreeSlots;
	for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
	{
		if (GetItem(i)->GetNum() != 0 
			&& GetItem(i)->GetNum() == sItemID)
		{
			bFreeSlots = i;
			return bFreeSlots;
		}
	}
	return 0;
}
#endif
void CUser::HandleNewRightClickGiveExchange(Packet& pkt)
{
	uint8 sType; uint32 sItemID;
	pkt >> sType >> sItemID;

	_ITEM_RIGHT_EXCHANGE* pRightExchange = g_pMain->s_HShieldSoftwareRightExchangeArray.GetData(sItemID);

	if (pRightExchange == nullptr)
		return;

	Packet result;

	if (sType == 1)
	{
		uint32 sSelectItem;
		pkt >> sSelectItem;

		if (pRightExchange->sType != sType)
			return;

		uint8 bFreeSlots = 0;
		for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
			if (GetItem(i)->nNum == 0 && ++bFreeSlots >= ITEMS_IN_EXCHANGE_GROUP) break;

		if (bFreeSlots < 1) {
			g_pMain->SendHelpDescription(this, "There is not enough space left.");
			return;
		}

		if (sSelectItem != 0)
		{
			if (RobItem(pRightExchange->sItemID, 1))
			{
				int8 bSelectIndex = -1;
				for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
				{
					if (pRightExchange->nItemID[i] == sSelectItem)
					{
						bSelectIndex = i;
						break;
					}
				}

				uint32 SelectItem = pRightExchange->nItemID[bSelectIndex];
				uint32 SelectItemCount = pRightExchange->nCount[bSelectIndex];
				uint32 SelectRentalTime = pRightExchange->nExpirationTime[bSelectIndex];

				GiveItem("Right Click New Exchange", SelectItem, SelectItemCount, true, SelectRentalTime);
#if(__VERSION < 2369)
				result.clear();
				result.Initialize(WIZ_HSACS_HOOK);
				result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(3) << uint8(5);
				Send(&result);
#endif
			}
		}
	}
	else if (sType == 2)
	{
		if (pRightExchange->sType != sType)
			return;

		uint8 bFreeSlots = 0;
		for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			if (pRightExchange->nItemID[i] > 0)
				bFreeSlots++;

		if (LuaCheckGiveSlot(bFreeSlots) == false) {
			g_pMain->SendHelpDescription(this, "There is not enough space left.");
			return;
		}

		if (RobItem(pRightExchange->sItemID, 1))
		{
			int8 bSelectIndex = -1;
			for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			{
				if (pRightExchange->nItemID[i] > 0)
					GiveItem("Right Click New Exchange", pRightExchange->nItemID[i], pRightExchange->nCount[i], true, pRightExchange->nExpirationTime[i]);
			}
#if(__VERSION < 2369)
			result.clear();
			result.Initialize(WIZ_HSACS_HOOK);
			result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(3) << uint8(5);
			Send(&result);
#endif
		}
	}
	else if (sType == 3)
	{
		if (pRightExchange->sType != sType)
			return;

		uint8 bFreeSlots = 0;
		for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			if (pRightExchange->nItemID[i] > 0)
				bFreeSlots++;

		if (LuaCheckGiveSlot(bFreeSlots) == false) {
			g_pMain->SendHelpDescription(this, "There is not enough space left.");
			return;
		}

		if (pRightExchange->sCount == 14)
		{
			if (!isClanLeader())
				return;
		}

		if (RobItem(pRightExchange->sItemID, 1))
		{
			if (pRightExchange->sCount == 13)
			{
				GiveSwitchPremium(10, 30);
				GiveSwitchPremium(11, 30);
				GiveSwitchPremium(12, 30);
			}
			else if (pRightExchange->sCount == 14)
				GiveClanPremium(2, 30);
			else
				GivePremium((uint8)pRightExchange->sCount, 30);

			int8 bSelectIndex = -1;
			for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			{
				if (pRightExchange->nItemID[i] > 0)
					GiveItem("Right Click New Exchange", pRightExchange->nItemID[i], pRightExchange->nCount[i], true, pRightExchange->nExpirationTime[i]);
			}
#if(__VERSION < 2369)
			result.clear();
			result.Initialize(WIZ_HSACS_HOOK);
			result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(3) << uint8(5);
			Send(&result);
#endif
		}
	}
	else if (sType == 4)
	{
		if (pRightExchange->sType != sType)
			return;

		uint8 bFreeSlots = 0;
		for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			if (pRightExchange->nItemID[i] > 0)
				bFreeSlots++;

		if (LuaCheckGiveSlot(bFreeSlots) == false) {
			g_pMain->SendHelpDescription(this, "There is not enough space left.");
			return;
		}

		if (RobItem(pRightExchange->sItemID, 1))
		{
			if (pRightExchange->sCount > 0)
				GiveBalance(pRightExchange->sCount);

			int8 bSelectIndex = -1;
			for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			{
				if (pRightExchange->nItemID[i] > 0)
					GiveItem("Right Click New Exchange", pRightExchange->nItemID[i], pRightExchange->nCount[i], true, pRightExchange->nExpirationTime[i]);
			}
#if(__VERSION < 2369)
			result.clear();
			result.Initialize(WIZ_HSACS_HOOK);
			result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(3) << uint8(5);
			Send(&result);
#endif
		}
	}
	else if (sType == 6)
	{
		if (pRightExchange->sType != sType)
			return;

		uint8 bFreeSlots = 0;
		for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			if (pRightExchange->nItemID[i] > 0)
				bFreeSlots++;

		if (LuaCheckGiveSlot(bFreeSlots) == false) {
			g_pMain->SendHelpDescription(this, "There is not enough space left.");
			return;
		}

		if (RobItem(pRightExchange->sItemID, 1))
		{
			if (pRightExchange->sCount > 0)
			{
				if (!m_sFirstUsingGenie)
					m_sFirstUsingGenie = 1;

				int remtime = int(m_1098GenieTime > UNIXTIME ? m_1098GenieTime - UNIXTIME : 0);
				m_1098GenieTime = UNIXTIME + (pRightExchange->sCount * HOUR) + (remtime > 0 ? remtime : 0);

				result.clear();
				result.Initialize(WIZ_GENIE);
				result << uint8(GenieUseSpiringPotion) << uint8(GenieUseSpiringPotion) << GetGenieTime();
				Send(&result);
			}

			int8 bSelectIndex = -1;
			for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			{
				if (pRightExchange->nItemID[i] > 0)
					GiveItem("Right Click New Exchange", pRightExchange->nItemID[i], pRightExchange->nCount[i], true, pRightExchange->nExpirationTime[i]);
			}
#if(__VERSION < 2369)
			result.clear();
			result.Initialize(WIZ_HSACS_HOOK);
			result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(3) << uint8(5);
			Send(&result);
#endif
		}
	}
	else if (sType == 7)
	{
		if (pRightExchange->sType != sType)
			return;

		uint8 bFreeSlots = 0;
		for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			if (pRightExchange->nItemID[i] > 0)
				bFreeSlots++;

		if (LuaCheckGiveSlot(bFreeSlots) == false) {
			g_pMain->SendHelpDescription(this, "There is not enough space left.");
			return;
		}

		if (RobItem(pRightExchange->sItemID, 1))
		{
			if (pRightExchange->sCount > 0)
				GiveBalance(0, pRightExchange->sCount);

			int8 bSelectIndex = -1;
			for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
			{
				if (pRightExchange->nItemID[i] > 0)
					GiveItem("Right Click New Exchange", pRightExchange->nItemID[i], pRightExchange->nCount[i], true, pRightExchange->nExpirationTime[i]);
			}
#if(__VERSION < 2369)
			result.clear();
			result.Initialize(WIZ_HSACS_HOOK);
			result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(3) << uint8(5);
			Send(&result);
#endif
		}
	}
}

void CUser::HandleNewRightClickExchangeReward(Packet& pkt)
{
	uint32 nItemID = pkt.read<uint32>();

	_ITEM_RIGHT_EXCHANGE* pRightExchange = g_pMain->s_HShieldSoftwareRightExchangeArray.GetData(nItemID);

	if (pRightExchange == nullptr)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(3) << pRightExchange->sType << pRightExchange->sType << pRightExchange->sItemID;

	for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
	{
		if (pRightExchange->nItemID[i] > MIN_ITEM_ID && pRightExchange->nItemID[i] < MAX_ITEM_ID)
			result << pRightExchange->nItemID[i] << pRightExchange->nExpirationTime[i];
		else if (pRightExchange->nItemID[i] == 0xFFFFFFFF)
			result << uint32(0) << uint32(0);
		else
			result << uint32(0) << uint32(0);
	}

	Send(&result);
#endif
}

void CUser::HandleNewRightClickExchangeAll(Packet& pkt)
{
	uint32 nItemID = pkt.read<uint32>();

	_ITEM_RIGHT_EXCHANGE* pRightExchange = g_pMain->s_HShieldSoftwareRightExchangeArray.GetData(nItemID);

	if (pRightExchange == nullptr)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(3) << pRightExchange->sType << pRightExchange->sType << pRightExchange->sItemID;

	for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
	{
		if (pRightExchange->nItemID[i] > MIN_ITEM_ID && pRightExchange->nItemID[i] < MAX_ITEM_ID)
			result << pRightExchange->nItemID[i] << pRightExchange->nExpirationTime[i];
		else if (pRightExchange->nItemID[i] == 0xFFFFFFFF)
			result << uint32(0) << uint32(0);
		else
			result << uint32(0) << uint32(0);
	}

	Send(&result);
#endif
}

void CUser::HandleNewRightClickExchangePremium(Packet& pkt)
{
	uint32 nItemID = pkt.read<uint32>();

	_ITEM_RIGHT_EXCHANGE* pRightExchange = g_pMain->s_HShieldSoftwareRightExchangeArray.GetData(nItemID);

	if (pRightExchange == nullptr)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(3) << pRightExchange->sType << pRightExchange->sType << pRightExchange->sItemID;

	for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
	{
		if (pRightExchange->nItemID[i] > MIN_ITEM_ID && pRightExchange->nItemID[i] < MAX_ITEM_ID)
			result << pRightExchange->nItemID[i] << pRightExchange->nExpirationTime[i];
		else if (pRightExchange->nItemID[i] == 0xFFFFFFFF)
			result << uint32(0) << uint32(0);
		else
			result << uint32(0) << uint32(0);
	}

	Send(&result);
#endif
}

void CUser::HandleNewRightClickExchangeKnightCash(Packet& pkt)
{
	uint32 nItemID = pkt.read<uint32>();

	_ITEM_RIGHT_EXCHANGE* pRightExchange = g_pMain->s_HShieldSoftwareRightExchangeArray.GetData(nItemID);

	if (pRightExchange == nullptr)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(3) << pRightExchange->sType << pRightExchange->sType << pRightExchange->sItemID;

	for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
	{
		if (pRightExchange->nItemID[i] > MIN_ITEM_ID && pRightExchange->nItemID[i] < MAX_ITEM_ID)
			result << pRightExchange->nItemID[i] << pRightExchange->nExpirationTime[i];
		else if (pRightExchange->nItemID[i] == 0xFFFFFFFF)
			result << uint32(0) << uint32(0);
		else
			result << uint32(0) << uint32(0);
	}

	Send(&result);
#endif
}

void CUser::HandleNewRightClickExchangeGenie(Packet& pkt)
{
	uint32 nItemID = pkt.read<uint32>();

	_ITEM_RIGHT_EXCHANGE* pRightExchange = g_pMain->s_HShieldSoftwareRightExchangeArray.GetData(nItemID);

	if (pRightExchange == nullptr)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(3) << pRightExchange->sType << pRightExchange->sType << pRightExchange->sItemID;

	for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
	{
		if (pRightExchange->nItemID[i] > MIN_ITEM_ID && pRightExchange->nItemID[i] < MAX_ITEM_ID)
			result << pRightExchange->nItemID[i] << pRightExchange->nExpirationTime[i];
		else if (pRightExchange->nItemID[i] == 0xFFFFFFFF)
			result << uint32(0) << uint32(0);
		else
			result << uint32(0) << uint32(0);
	}

	Send(&result);
#endif
}

void CUser::HandleNewRightClickExchangeKnightTL(Packet& pkt)
{
	uint32 nItemID = pkt.read<uint32>();

	_ITEM_RIGHT_EXCHANGE* pRightExchange = g_pMain->s_HShieldSoftwareRightExchangeArray.GetData(nItemID);

	if (pRightExchange == nullptr)
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(3) << pRightExchange->sType << pRightExchange->sType << pRightExchange->sItemID;

	for (int i = 0; i < ITEMS_RIGHT_CLICK_EXCHANGE_GROUP; i++)
	{
		if (pRightExchange->nItemID[i] > MIN_ITEM_ID && pRightExchange->nItemID[i] < MAX_ITEM_ID)
			result << pRightExchange->nItemID[i] << pRightExchange->nExpirationTime[i];
		else if (pRightExchange->nItemID[i] == 0xFFFFFFFF)
			result << uint32(0) << uint32(0);
		else
			result << uint32(0) << uint32(0);
	}

	Send(&result);
#endif
}
#if(GAME_SOURCE_VERSION == 2369)
uint8 CUser::HandleHShieldSoftwareItemExchangeHandler(uint32 sItemID)
{
	auto pRightClick = g_pMain->s_HShieldSoftwareRightClickExchangeArray.GetData(sItemID);
	if (pRightClick == nullptr)
		return 0;

	if (pRightClick->sItemID == sItemID)
		return pRightClick->sSobCode;
	return 0;
}

uint8 CUser::HandleHShieldRightClickExchangeHandler(uint32 sItemID)
{
	auto pRightClick = g_pMain->s_HShieldSoftwareRightClickExchangeArray.GetData(sItemID);
	if (pRightClick == nullptr)
		return 0;

	if (pRightClick->sItemID == sItemID)
		return 1;
	return 0;
}

void CUser::HandleHShieldSoftwareNewItemMoveLoadderHandler()
{
	std::vector<_ITEM_DATA> InventoryItems;
	uint8 strInventory[28];
	uint8 sSubCode[28];
	uint32 sItemID[28];
#if(__VERSION < 2369)
	Packet result;
	result.Initialize(WIZ_HSACS_HOOK);
#endif
	for (int i = SLOT_MAX; i < SLOT_MAX + HAVE_MAX; i++)
		InventoryItems.push_back(m_sItemArray[i]);

	for (int k = SLOT_MAX; k < SLOT_MAX + HAVE_MAX; k++)
	{
		m_sItemArray[k] = InventoryItems[k - SLOT_MAX];
		strInventory[k - SLOT_MAX] = HandleHShieldRightClickExchangeHandler(m_sItemArray[k].nNum);
		sSubCode[k - SLOT_MAX] = HandleHShieldSoftwareItemExchangeHandler(m_sItemArray[k].nNum);
		sItemID[k - SLOT_MAX] = m_sItemArray[k].nNum;
	}
#if(__VERSION < 2369)
	result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(0);

	for (int k = 0; k < HAVE_MAX; k++)
		result << strInventory[k] << sSubCode[k] << sItemID[k];

	Send(&result);
#endif
}
#endif
void CUser::HandleHShieldSoftwareRightExchangeLoadderHandler()
{
	g_pMain->s_HShieldSoftwareRightExchangeArray.m_lock.lock();
	auto s_HShieldSoftwareRightExchangeArray = g_pMain->s_HShieldSoftwareRightExchangeArray.m_UserTypeMap;
	g_pMain->s_HShieldSoftwareRightExchangeArray.m_lock.unlock();

	std::vector <uint32> pRightClickExchangeReward;
	std::vector <uint32> pRightClickExchangeAll;
	std::vector <uint32> pRightClickExchangeKnightCash;
	std::vector <uint32> pRightClickExchangePremium;
	std::vector <uint32> pRightClickExchangeGenie;
	std::vector <uint32> pRightClickExchangeKnightTL;
	std::vector <uint32> pRightClickGeneratorExchange;

	foreach(itr, s_HShieldSoftwareRightExchangeArray)
	{
		if (itr->second->sType == 1)
			pRightClickExchangeReward.push_back(itr->second->sItemID);
		else if (itr->second->sType == 2)
			pRightClickExchangeAll.push_back(itr->second->sItemID);
		else if (itr->second->sType == 3)
			pRightClickExchangePremium.push_back(itr->second->sItemID);
		else if (itr->second->sType == 4)
			pRightClickExchangeKnightCash.push_back(itr->second->sItemID);
		else if (itr->second->sType == 6)
			pRightClickExchangeGenie.push_back(itr->second->sItemID);
		else if (itr->second->sType == 7)
			pRightClickExchangeKnightTL.push_back(itr->second->sItemID);
		else
			continue;
	}
#if(GAME_SOURCE_VERSION == 1098)
	g_pMain->s_HShieldSoftwareRightClickExchangeArray.m_lock.lock();
	auto s_HShieldSoftwareRightClickExchangeArray = g_pMain->s_HShieldSoftwareRightClickExchangeArray.m_UserTypeMap;
	g_pMain->s_HShieldSoftwareRightClickExchangeArray.m_lock.unlock();

	foreach(itr, s_HShieldSoftwareRightClickExchangeArray)
	{
		if (itr->second->sSobCode == 1)
			pRightClickGeneratorExchange.push_back(itr->second->sItemID);
		else
			continue;
	}
#endif
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_ITEM_EXCHANGE_INFO));
	result << uint8(2) << uint8(1) << int16(pRightClickExchangeReward.size());

	foreach(itr, pRightClickExchangeReward)
		result << uint32(*itr);
	Send(&result);

	result.clear();
	result.Initialize(WIZ_HSACS_HOOK);
	result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(2) << uint8(2) << uint16(pRightClickExchangeAll.size());
	foreach(itr, pRightClickExchangeAll)
		result << uint32(*itr);
	Send(&result);

	result.clear();
	result.Initialize(WIZ_HSACS_HOOK);
	result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(2) << uint8(3) << uint16(pRightClickExchangePremium.size());
	foreach(itr, pRightClickExchangePremium)
		result << uint32(*itr);
	Send(&result);

	result.clear();
	result.Initialize(WIZ_HSACS_HOOK);
	result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(2) << uint8(4) << uint16(pRightClickExchangeKnightCash.size());
	foreach(itr, pRightClickExchangeKnightCash)
		result << uint32(*itr);
	Send(&result);

	result.clear();
	result.Initialize(WIZ_HSACS_HOOK);
	result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(2) << uint8(6) << uint16(pRightClickExchangeGenie.size());
	foreach(itr, pRightClickExchangeGenie)
		result << uint32(*itr);
	Send(&result);

	result.clear();
	result.Initialize(WIZ_HSACS_HOOK);
	result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(2) << uint8(7) << uint16(pRightClickExchangeKnightTL.size());
	foreach(itr, pRightClickExchangeKnightTL)
		result << uint32(*itr);
	Send(&result);
#if(GAME_SOURCE_VERSION == 1098)
	result.clear();
	result.Initialize(WIZ_HSACS_HOOK);
	result << uint8(WIZ_ITEM_EXCHANGE_INFO) << uint8(2) << uint8(5) << uint16(pRightClickGeneratorExchange.size());
	foreach(itr, pRightClickGeneratorExchange)
		result << uint32(*itr);
	Send(&result);
#endif
#endif
}
#if(GAME_SOURCE_VERSION == 2369 || GAME_SOURCE_VERSION == 1098)
void CUser::HandleHShieldSoftwareDailyReward()
{
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_DAILY_REWARD));
#endif
	uint32 sItemID[25] = { 0 };
	uint32 sItemCumulativeID[3] = { 0 };
	std::vector<ByteBuffer> sItemIDType, sByType, sItemCumulative;
	g_DBAgent.LoadDailyReward(sItemIDType);
	g_DBAgent.LoadDailyRewardUser(GetID(), sByType);
	g_DBAgent.LoadDailyCumulativeReward(sItemCumulative);

	if (sItemIDType.size() > 0)
	{
		if (sByType.size() > 0)
		{
			if (sItemCumulative.size() > 0)
			{
				for (int i = 0; i < 25; i++)
					sItemIDType.at(0) >> sItemID[i];

				for (int i = 0; i < 3; i++)
					sItemCumulative.at(0) >> sItemCumulativeID[i];

				uint8 sGetDay[25] = { 0 };
				uint8 sNewGetDay[25] = { 0 };

				for (int i = 0; i < 25; i++)
					sByType.at(0) >> sGetDay[i] >> sNewGetDay[i];
#if(__VERSION < 2369)
				result << uint8(0);

				for (int i = 0; i < 25; i++)
					result << sItemID[i] << sGetDay[i] << sNewGetDay[i];

				for (int i = 0; i < 3; i++)
					result << sItemCumulativeID[i];

				Send(&result);
#endif
			}
		}
	}
}
#endif
#endif

bool CUser::CheckChestBlockItem(uint32 itemid)
{
	if (m_reloadChestBlock)
		return false;

	Guard lock(mChestBlockItemLock);
	return std::find(mChestBlockItem.begin(), mChestBlockItem.end(), itemid) != mChestBlockItem.end();
}

void CUser::HandleChestBlock(Packet& pkt)
{
	uint8 subcode = pkt.read<uint8>();

	m_reloadChestBlock = true;
	mChestBlockItemLock.lock();
	mChestBlockItem.clear();
	if (subcode == 0)
	{
		uint16 size;
		pkt >> size;
		if (size > 100) size = 100;
		for (uint16 i = 0; i < size; i++)
		{
			uint32 itemid = pkt.read<uint32>();

			auto pItem = g_pMain->GetItemPtr(itemid);
			if (pItem.isnull() || pItem.m_bCountable == 2)
				continue;

			if (itemid)
				mChestBlockItem.push_back(itemid);
		}
	}
	mChestBlockItemLock.unlock();
	m_reloadChestBlock = false;
	if (subcode != 1)
		return;
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::CHEST_BLOCKITEM));
	Send(&newpkt);
#endif
}

void CUser::HSACSX_SendAccountRegister()
{
	/*if (isGM())
		return;*/

	Packet test(WIZ_DB_SAVE_USER, uint8(ProcDbType::AccountInfoShow));
	g_pMain->AddDatabaseRequest(test, this);
}

bool NumberValid(const std::string& str2)
{
	std::string str = str2;
	STRTOLOWER(str);
	char a[10] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };

	int size = (int)str.length();
	for (int i = 0; i < size; i++)
	{
		if (str.at(i) != a[0]
			&& str.at(i) != a[1]
			&& str.at(i) != a[2]
			&& str.at(i) != a[3]
			&& str.at(i) != a[4]
			&& str.at(i) != a[5]
			&& str.at(i) != a[6]
			&& str.at(i) != a[7]
			&& str.at(i) != a[8]
			&& str.at(i) != a[9])
			return false;
	}
	return true;
}

void CUser::HSACSX_AccountInfoSave(Packet& pkt)
{
	uint8 Opcode;
	pkt >> Opcode;

	switch (Opcode)
	{
	case 1:
	{
		std::string txt_email, txt_phone, txt_seal, txt_otp;
		pkt >> txt_email >> txt_phone >> txt_seal >> txt_otp;
#if(__VERSION < 2369)
		Packet test(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::ACCOUNT_INFO_SAVE));
#endif
		if (txt_email.empty() || txt_email.size() > 250)
		{
#if(__VERSION < 2369)
			test << uint8(2) << uint8(0);
			Send(&test);
#endif
			return;
		}

		if (txt_phone.empty() || txt_phone.size() != 11 || !NumberValid(txt_phone))
		{
#if(__VERSION < 2369)
			test << uint8(2) << uint8(0);
			Send(&test);
#endif
			return;
		}

		if (txt_seal.empty() || txt_seal.size() != 8 || !NumberValid(txt_seal))
		{
#if(__VERSION < 2369)
			test << uint8(2) << uint8(0);
			Send(&test);
#endif
			return;
		}

		if (txt_otp.empty() || txt_otp.size() != 6 || !NumberValid(txt_otp))
		{
#if(__VERSION < 2369)
			test << uint8(2) << uint8(0);
			Send(&test);
#endif
			return;
		}

		Packet Datasave(WIZ_DB_SAVE_USER, uint8(ProcDbType::AccountInfoSave));
		Datasave.DByte();
		Datasave << txt_email << txt_phone << txt_seal << txt_otp;
		g_pMain->AddDatabaseRequest(Datasave, this);
	}
	break;
	//case 2:
	//	Disconnect();
	//	break;
	default:
		break;
	}
}

void CUser::HSACSX_General(Packet& pkt)
{
	DateTime time;
	using std::string;

	std::string Account;

	pkt >> Account >> itemorg >> skillmagic >> zones >> itemsell_table >> srcversion; // dllversion uyumsuz ise dc eder
	if (!isGM() && !isGMUser())
	{
		if (itemorg != g_pMain->server_itemorg
			|| skillmagic != g_pMain->server_skillmagic
			|| zones != g_pMain->server_zones
			|| itemsell_table != g_pMain->server_itemsell_table)
			return goDisconnect("tbl data does not match.", __FUNCTION__);
	}

	if (srcversion != "f5h4y7r8d5v3sd1s696g9y7r5w5q1a2d23gf3e625q4") // dllversion uyumsuz ise dc eder
	{
		printf("Version Eslesmedi AccountID : %s Nick : %s\n", GetAccountName().c_str(), GetName().c_str());
		return goDisconnect("version did not match.", __FUNCTION__);
	}

	if (!Account.empty())
	{
		if (m_strAccountID != Account)
			return goDisconnect("AcountName match.", __FUNCTION__);
	}
}

uint8_t CUser::CheckEmptyWareHouseSlot()
{
	uint8_t EmptyCount = 0;

	for (uint8_t i = 0; i < WAREHOUSE_MAX; i++)
	{
		_ITEM_DATA* pItem = &m_sWarehouseArray[i];
		if (pItem == nullptr)
			continue;

		if (pItem->nNum == 0)
			EmptyCount++;

	}
	return EmptyCount;
}

void CUser::WareHouseAddItemProcess(uint32 ItemID, uint32 Count)
{
	//if (WareHouseOpen) // Banka Açýk Ýptal Ediyoruz
	//	return;

	/*if (!isAutoFishing() && !isAutoMining())
		return;*/

	_ITEM_TABLE pTable = g_pMain->GetItemPtr(ItemID);
	if (pTable.isnull() || (!pTable.isStackable() && Count > 1))
		return;

	for (int i = 0; i < WAREHOUSE_MAX; i++)
	{
		_ITEM_DATA* pItem = &m_sWarehouseArray[i];
		if (pItem == nullptr || (pItem->nNum != 0 && pItem->nNum != ItemID))
			continue;

		if (pItem->nNum == ItemID && pTable.m_bCountable != 0)
		{
			pItem->sCount++;
			break;
		}
		else if (pItem->nNum == ItemID && pTable.m_bCountable == 0)
			continue;
		else
		{
			pItem->nNum = ItemID;
			pItem->sDuration = pTable.m_sDuration;
			pItem->sCount = Count;
			pItem->nSerialNum = g_pMain->GenerateItemSerial();
			pItem->bFlag = ITEM_FLAG_NONE;	// ITEM_FLAG_NONE
			break;
		}
	}
}

void CUser::HSACSX_LastSeenProcess(Packet& pkt)
{
	uint8 opcode = pkt.read<uint8>();

	switch (opcode)
	{
	case 1:
	{
		DateTime dt;
		m_LastSeen[0] = dt.GetHour();
		m_LastSeen[1] = dt.GetMinute();

		//printf("Last Seen Updated %d:%d\n", m_LastSeen[0], m_LastSeen[1]);
	}
	break;
	case 2:
	{
		return;
		std::string targetUser = "";
		pkt >> targetUser;
		if (targetUser.empty() || targetUser == "")
			return;
		CUser* pTUser = g_pMain->GetUserPtr(targetUser, NameType::TYPE_CHARACTER);
		if (pTUser == nullptr || !pTUser->isInGame())
			return;
#if(__VERSION < 2369)
		Packet result(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::CHAT_LASTSEEN));
		result << pTUser->GetName() << uint8(pTUser->m_LastSeen[0]) << uint8(pTUser->m_LastSeen[1]);
		Send(&result);
#endif
	}
	default:
		break;
	}
}

void CUser::HSACSX_SendRepurchaseMsg(Packet& pkt)
{
	SendRepurchaseMsg();
}

void CUser::HSACSX_Send1299SkillAndStatReset(Packet& pkt)
{
	uint8 opcode = pkt.read<uint8>();

	switch (opcode)
	{
	case 1:
	{
		AllPointChange();
	}
	break;
	case 2:
	{
		AllSkillPointChange();
	}
	default:
		break;
	}
}

void CUser::SendAcsMessage(std::string message)
{
	if (message.empty())
		return;
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::INFOMESSAGE));
	newpkt << message; Send(&newpkt);
#endif
}

void CUser::SendVisibe(uint16 Offset1, uint16 Offset2)
{
	if (!isInGame()
		|| !isGM())
		return;
#if(__VERSION < 2369)
	Packet result(WIZ_HSACS_HOOK, uint8(WIZ_HOOK_VISIBLE));
	result << static_cast<uint16_t>(Offset1) << static_cast<uint16_t>(Offset2);
	Send(&result);
#endif
}