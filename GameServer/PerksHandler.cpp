#include "StdAfx.h"

void CUser::Send_myPerks()
{
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::PERKS));
	newpkt << uint8(perksSub::info) << pPerks.remPerk << g_pMain->pServerSetting.m_perkCoins;
#endif
	g_pMain->m_PerksArray.m_lock.lock();
	auto m_PerksArray = g_pMain->m_PerksArray.m_UserTypeMap;
	g_pMain->m_PerksArray.m_lock.unlock();
#if(__VERSION < 2369)
	newpkt << (uint16)m_PerksArray.size();
#endif
	foreach(itr, m_PerksArray)
	{
		auto* p = itr->second;
		if (!p) continue;
#if(__VERSION < 2369)
		newpkt.DByte();
		newpkt << p->pIndex << p->status << p->perkCount << p->maxPerkCount << p->strDescp << p->percentage;
#endif
	}
#if(__VERSION < 2369)
	for (int i = 0; i < PERK_COUNT; i++)
		newpkt << pPerks.perkType[i];
	Send(&newpkt);
#endif
}

void CUser::HandlePerks(Packet& pkt)
{
	uint8 subcode = pkt.read<uint8>();
	switch ((perksSub)subcode)
	{
	case perksSub::perkPlus:
		PerkPlus(pkt);
		break;
	case perksSub::perkReset:
		PerkReset(pkt);
		break;
	case perksSub::perkTargetInfo:
		PerkTargetInfo(pkt);
		break;
	}
}

void CUser::SendPerkError(perksError error)
{
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::PERKS));
	newpkt << uint8(perksSub::perkPlus) << uint8(error);
	Send(&newpkt);
#endif
}

void CUser::PerkPlus(Packet& pkt)
{
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::PERKS));
	newpkt << uint8(perksSub::perkPlus);

	uint32 index = pkt.read<uint32>();
	if (index >= PERK_COUNT)
		return;

	if (!pPerks.remPerk)
		return SendPerkError(perksError::remPerks);

	auto* perks = g_pMain->m_PerksArray.GetData(index);
	if (!perks)
		return SendPerkError(perksError::notFound);

	if (pPerks.perkType[index] >= perks->maxPerkCount)
		return SendPerkError(perksError::maxPerkCount);

	pPerks.remPerk--;
	pPerks.perkType[index]++;
	newpkt << uint8(perksError::success) << index << pPerks.perkType[index] << pPerks.remPerk;
	Send(&newpkt);
#endif
	SetUserAbility(true);
}

void CUser::PerkReset(Packet& pkt)
{
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::PERKS));
	newpkt << uint8(perksSub::perkReset);
#endif
	uint32 point = 0;
	for (int i = 0; i < PERK_COUNT; i++)
		point += pPerks.perkType[i];

	if (point == 0)
		return SendBoxMessage(0, "", "There are no Perk points to reset.", 0, messagecolour::red);

	uint32 coins = g_pMain->pServerSetting.m_perkCoins;

	if (!GoldLose(coins, true))
		return SendBoxMessage(0, "", "You do not have enough money to reset the perk points.", 0, messagecolour::red);

	pPerks.remPerk += point;
	memset(pPerks.perkType, 0, sizeof(pPerks.perkType));
#if(__VERSION < 2369)
	newpkt << pPerks.remPerk;
	Send(&newpkt);
#endif
	SetUserAbility(true);
}

bool CUser::PerkUseItem(uint32 itemid, uint32 itemcount, uint16 perk)
{
	uint16 plus = perk;
	if (plus < 1) plus = 1;
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::PERKS));
	newpkt << uint8(perksSub::perkUseItem);
#endif
	if (itemid)
	{
		if (!CheckExistItem(itemid, itemcount)) {
			SendBoxMessage(0, "", "You also don't have perk item inventory. please check.", 0, messagecolour::red);
			return false;
		}
			
		if (!RobItem(itemid, itemcount, true)) {
			SendBoxMessage(0, "", "You also don't have perk item inventory. please check.", 0, messagecolour::red);
			return false;
		}
	}

	pPerks.remPerk += plus;
#if(__VERSION < 2369)
	newpkt << pPerks.remPerk;
	Send(&newpkt);
#endif
	return true;
}

void CUser::PerkTargetInfo(Packet& pkt)
{
	uint16 targetID = pkt.read<uint16>();
	if (!targetID)
		return;

	CUser* pUser = g_pMain->GetUserPtr(targetID);
	if (!pUser || !pUser->isInGame())
		return SendBoxMessage(0, "", "No player found or no such player.", 0, messagecolour::red);
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, uint8(HSACSXOpCodes::PERKS));
	newpkt << uint8(perksSub::perkTargetInfo) << targetID;
	for (int i = 0; i < PERK_COUNT; i++)
		newpkt << pUser->pPerks.perkType[i];
	Send(&newpkt);
#endif
}

void CUser::SendBoxMessage(uint8 type, std::string title, std::string message, uint8 time, messagecolour a) {
#if(__VERSION < 2369)
	Packet newpkt(WIZ_HSACS_HOOK, (uint8)HSACSXOpCodes::MESSAGE2);
	newpkt.DByte();

	newpkt << type;
	if (type == 0)
		newpkt << message << uint32(a);
	else
		newpkt << title << message << time;

	Send(&newpkt);
#endif
}