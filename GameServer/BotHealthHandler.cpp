#include "StdAfx.h"

void CBot::HpChange(int amount, Unit* pAttacker /*= nullptr*/, bool isDOT /*= false*/)
{
	uint16 tid = (pAttacker != nullptr ? pAttacker->GetID() : -1);
	int16 oldHP = m_sHp;
	int originalAmount = amount;
	int mirrorDamage = 0;

	// No cheats allowed
	if (pAttacker && pAttacker->GetZoneID() != GetZoneID())
		return;

	// Implement damage/HP cap.
	if (amount < -MAX_DAMAGE)
		amount = -MAX_DAMAGE;
	else if (amount > MAX_DAMAGE)
		amount = MAX_DAMAGE;

	// If we're taking damage...
	if (amount < 0)
	{
		/*if (!isDOT)
			RemoveStealth();*/

		bool NotUseZone = (GetZoneID() == ZONE_CHAOS_DUNGEON || GetZoneID() == ZONE_KNIGHT_ROYALE);

		// Handle the mirroring of damage.
		if (m_bMirrorDamage && !NotUseZone)
		{
			if (m_bMirrorDamageType)
			{
				CUser* pUserAttacker = g_pMain->GetUserPtr(pAttacker->GetID());;
				if (pUserAttacker != nullptr)
				{
					mirrorDamage = (m_byMirrorAmount * amount) / 100;
					amount -= mirrorDamage;
					pUserAttacker->HpChange(mirrorDamage);
				}
			}
		}

		// Handle mastery passives
		if (isMastered() && !NotUseZone)
		{
			// Matchless: [Passive]Decreases all damages received by 15%
			if (CheckSkillPoint(SkillPointMaster, 10, g_pMain->m_byMaxLevel))
				amount = (85 * amount) / 100;
			// Absoluteness: [Passive]Decrease 10 % demage of all attacks
			else if (CheckSkillPoint(SkillPointMaster, 5, 9))
				amount = (90 * amount) / 100;
		}

		if (m_bManaAbsorb > 0 && !NotUseZone)
		{
			int toBeAbsorbed = 0, absortedmana = 0;
			toBeAbsorbed = (originalAmount * m_bManaAbsorb) / 100;
			amount -= toBeAbsorbed;

			if (amount > 0)
				amount = 0;

			absortedmana = toBeAbsorbed;
			MSpChange(absortedmana);
		}
	}
	// If we're receiving HP and we're undead, all healing must become damage.
	else if (m_bIsUndead)
	{
		amount = -amount;
		originalAmount = amount;
	}

	if (amount < 0 && -amount >= m_sHp)
		m_sHp = 0;
	else if (amount >= 0 && m_sHp + amount > m_MaxHp)
		m_sHp = m_MaxHp;
	else
		m_sHp += amount;

	bool NotUseZone2 = (GetZoneID() == ZONE_CHAOS_DUNGEON || GetZoneID() == ZONE_KNIGHT_ROYALE);

	if (pAttacker != nullptr
		&& pAttacker->isPlayer()
		&& isDevil())
	{
		if (amount < 0)
		{
			int32 Receive = int32(amount / 3.1);
			AbsorbedAmmount += Receive;

			if (m_sHp > 0)
				m_sHp -= int16(Receive);

			if (AbsorbedAmmount <= ABSORBED_TOTAL)
				CMagicProcess::RemoveType4Buff(BUFF_TYPE_DEVIL_TRANSFORM, this);
		}
	}

	if (pAttacker != nullptr
		&& pAttacker->isPlayer()
		&& m_sHp > 0 && amount < 0
		&& !NotUseZone2)
	{
		if (isWarrior() && isMastered())
		{
			if (CheckSkillPoint(PRO_SKILL4, 10, 23))
			{
				int16 NewHP = oldHP - m_sHp;
				m_sHp += (15 * NewHP) / 100;
			}
		}
		else if ((isRogue() || isMage() || isPriest()) && isMastered())
		{
			if (CheckSkillPoint(PRO_SKILL4, 5, 9))
			{
				int16 NewHP = oldHP - m_sHp;
				m_sHp += (10 * NewHP) / 100;
			}
			else if (CheckSkillPoint(PRO_SKILL4, 10, 23))
			{
				int16 NewHP = oldHP - m_sHp;
				m_sHp += (15 * NewHP) / 100;
			}
		}
	}

	if (GetHealth() > 0
		&& isMastered()
		&& !isMage() && !NotUseZone2)
	{
		const uint16 hp30Percent = (30 * GetMaxHealth()) / 100;
		if ((oldHP >= hp30Percent && m_sHp < hp30Percent)
			|| (m_sHp > hp30Percent))
		{
			SetBotAbility();

			if (m_sHp < hp30Percent)
				ShowEffect(106800); // skill ID for "Boldness", shown when a player takes damage.
		}
	}

	// Ensure we send the original damage (prior to passives) amount to the attacker 
	// as it appears to behave that way officially.
	if (pAttacker != nullptr
		&& pAttacker->isPlayer())
		TO_USER(pAttacker)->SendTargetHP(0, GetID(), originalAmount, false);

	if (m_sHp <= 0)
		OnDeath(pAttacker);
}

void CBot::MSpChange(int amount)
{
	int16 oldMP = m_sMp;

	// TODO: Make this behave unsigned.
	m_sMp += amount;
	if (m_sMp < 0)
		m_sMp = 0;
	else if (m_sMp > m_MaxMp)
		m_sMp = m_MaxMp;

	if (isMasteredMage())
	{
		const uint16 mp30Percent = (30 * GetMaxMana()) / 100;
		if (oldMP >= mp30Percent
			&& GetMana() < mp30Percent)
			ShowEffect(106800); // skill ID for "Boldness", shown when a player loses mana.
	}
}

void CBot::SpChange(int amount)
{
	Packet result(WIZ_KURIAN_SP_CHANGE);

	if (isBeginnerKurianPortu())
		m_MaxSp = 100;
	else if (isNoviceKurianPortu())
		m_MaxSp = 150;
	else if (isMasteredKurianPortu())
	{
		if (CheckSkillPoint(PRO_SKILL4, 0, 2))
			m_MaxSp = 200;
		else if (CheckSkillPoint(PRO_SKILL4, 3, 23))
			m_MaxSp = 250;
		else
			m_MaxSp = 200;
	}
	else
		m_MaxSp = 200;

	m_sSp += amount;

	if (m_sSp < 0)
		m_sSp = 0;
	else if (m_sSp >= m_MaxSp)
		m_sSp = m_MaxSp;
}

void CBot::SetMaxHp(int iFlag)
{
	_CLASS_COEFFICIENT* p_TableCoefficient = nullptr;
	p_TableCoefficient = g_pMain->m_CoefficientArray.GetData(GetClass());

	if (!p_TableCoefficient)
		return;

	int temp_sta = GetStatTotal(STAT_STA);

	if (GetZoneID() == ZONE_SNOW_BATTLE && iFlag == 0)
	{
		if (GetFame() == COMMAND_CAPTAIN || isKing())
			m_MaxHp = 300;
		else
			m_MaxHp = 100;
	}
	else if (GetZoneID() == ZONE_CHAOS_DUNGEON && iFlag == 0
		|| (GetZoneID() == ZONE_DUNGEON_DEFENCE && iFlag == 0))
		m_MaxHp = 10000 / 10;
	else
	{
		m_MaxHp = (short)(((p_TableCoefficient->HP * GetLevel() * GetLevel() * temp_sta)
			+ 0.1 * (GetLevel() * temp_sta) + (temp_sta / 5)) + m_sMaxHPAmount + m_sItemMaxHp + 20);

		if (iFlag == 1)
		{
			m_MaxHp = MAX_PLAYER_HP;
			HpChange(m_MaxHp);
		}
		else if (iFlag == 2)
			m_MaxHp = 100;
	}
	// Awakening Max Healt %20 Arttýrma.
	if (isMasteredKurianPortu())
	{
		if (GetZoneID() != ZONE_KNIGHT_ROYALE
			&& GetZoneID() != ZONE_CHAOS_DUNGEON)
		{
			if (CheckSkillPoint(PRO_SKILL4, 2, 23))
				m_MaxHp += m_MaxHp * 20 / 100;
		}
	}

	//Transformation stats need to be applied here
	if (GetZoneID() == ZONE_DELOS && isSiegeTransformation())
	{
		_MAGIC_TYPE6* pType = g_pMain->m_Magictype6Array.GetData(m_sTransformSkillID);

		if (pType != nullptr)
			m_MaxHp = (short)pType->sMaxHp;

		if (m_MaxHp > 0)
		{
			if (m_sTransformHpchange)
			{
				m_sTransformHpchange = false;
				return;
			}
		}
	}
	if (m_MaxHp < m_sHp)
	{
		m_sHp = m_MaxHp;
		HpChange(m_sHp);
	}
}

void CBot::SetMaxMp()
{
	_CLASS_COEFFICIENT* p_TableCoefficient = nullptr;
	p_TableCoefficient = g_pMain->m_CoefficientArray.GetData(GetClass());
	if (!p_TableCoefficient) return;

	int temp_intel = 0, temp_sta = 0;
	temp_intel = GetStatTotal(STAT_INT) + 30;
	temp_sta = GetStatTotal(STAT_STA);

	if (p_TableCoefficient->MP != 0)
	{
		m_MaxMp = (short)((p_TableCoefficient->MP * GetLevel() * GetLevel() * temp_intel)
			+ (0.1f * GetLevel() * 2 * temp_intel) + (temp_intel / 5) + m_sMaxMPAmount + m_sItemMaxMp + 20);
	}
	else if (p_TableCoefficient->SP != 0)
	{
		m_MaxMp = (short)((p_TableCoefficient->SP * GetLevel() * GetLevel() * temp_sta)
			+ (0.1f * GetLevel() * temp_sta) + (temp_sta / 5) + m_sMaxMPAmount + m_sItemMaxMp);
	}

	//Transformation stats need to be applied here
	if (GetZoneID() == ZONE_DELOS && isSiegeTransformation())
	{
		_MAGIC_TYPE6* pType = g_pMain->m_Magictype6Array.GetData(m_sTransformSkillID);

		if (pType != nullptr)
			m_MaxMp = (short)pType->sMaxMp;

		if (m_MaxMp > 0)
		{
			if (m_sTransformMpchange)
			{
				m_sTransformMpchange = false;
				MSpChange(m_MaxMp);
				return;
			}
		}
	}

	if (m_MaxMp < m_sMp)
	{
		m_sMp = m_MaxMp;
		MSpChange(m_sMp);
	}
}

void CBot::SetMaxSp()
{
	if (isBeginnerKurianPortu())
		m_MaxSp = 100;
	else if (isNoviceKurianPortu())
		m_MaxSp = 150;
	else if (isMasteredKurianPortu())
	{
		if (CheckSkillPoint(PRO_SKILL4, 0, 2))
			m_MaxSp = 200;
		else if (CheckSkillPoint(PRO_SKILL4, 3, 23))
			m_MaxSp = 250;
		else
			m_MaxSp = 200;
	}
	else
		m_MaxSp = 200;

	if (m_MaxSp < m_sSp)
	{
		m_sSp = m_MaxSp;
		SpChange(m_sSp);
	}
}

void CBot::HpMpChange()
{
	m_fHPChangeTime = getMSTime();

	if (isDead())
		return;

	const uint16 hp30Percent = (90 * GetMaxHealth()) / 100;
	if (uint16(GetHealth()) > hp30Percent)
		return;

	int32 sSkillID = 0;
	if (isRogue())
		sSkillID = 490014;
	else if (isMage())
		sSkillID = 490014;
	else if (isPortuKurian())
		sSkillID = 490014;
	else if (isPriest())
		sSkillID = 112545;
	else
		sSkillID = 490014;

	if (sSkillID <= 0)
		return;

	if (isPriest())
	{
		if (GetNation() == ELMORAD)
			sSkillID += 100000;

		m_sSpeed = NULL;
	}

	if (sSkillID == 112545
		|| sSkillID == 212545)
		MagicPacket(MAGIC_CASTING, sSkillID, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());

	MagicPacket(MAGIC_EFFECTING, sSkillID, GetID(), GetID(), (uint16)GetX(), (uint16)GetY(), (uint16)GetZ());
}