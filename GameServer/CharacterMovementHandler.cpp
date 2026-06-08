#include "stdafx.h"

#pragma region CUser::MoveProcess(Packet & pkt)
void CUser::MoveProcess(Packet & pkt)
{
	if(!isInGame() || GetMap() == nullptr)
		return;

	if (m_bWarp || isDead()) 
		return;

	uint16 will_x, will_z, will_y;
	int16 speed = 0;
	float real_x, real_z, real_y;
	uint8 echo;
	uint16 fWillX = m_oldwillx, fWillZ = m_oldwillz, fWillY = m_oldwilly;

	pkt >> will_x >> will_z >> will_y >> speed >> echo >> curX1 >> curZ1 >> curY1;
	//printf("MoveProcess %d %d %d %d %d %d\n", will_x, will_z, will_y, curX1, curZ1, curY1);
	moveop e = (moveop)echo;
	if (e != moveop::start && e != moveop::move && e != moveop::finish)
		return goDisconnect("moveprocess diff echo", __FUNCTION__);

	pMove.status = e;

	bool stable = will_x == curX1 && will_z == curZ1 && will_y == curY1;
	if (!isGM() && !stable && ((echo != 0 && speed == 0) || (pMove.oldecho == 0 && echo == 0))) {
		if (pMove.caughttime > UNIXTIME2) pMove.caughtcount++;
		else pMove.caughtcount = 1;

		if (pMove.caughtcount >= 3) {
			std::string dclog = string_format("echo is not 0 but speed is 0 struserid=%s\n", GetName().c_str());
			
			if (pMove.oldecho == 0 && echo == 0)
				dclog = string_format("successively echo 0 came\n", GetName().c_str());
			
			printf("%s\n", dclog.c_str());
			return Home();
		}
		pMove.caughttime = UNIXTIME2 + 1100;
	}
	pMove.oldecho = echo; pMove.oldspeed = speed;

	if (fWillX == 0)
		fWillX = will_x;

	if (fWillY == 0)
		fWillY = will_y;

	if (fWillZ == 0)
		fWillZ = will_z;

	if (m_sSpeed == 0 && echo == 1)
	{
		will_x = (will_x + curX1) / 2;
		will_y = (will_y + curY1) / 2;
		will_z = (will_z + curZ1) / 2;
	}
	else if (speed)
	{
		if (GetDistance(fWillX / 10.0f, fWillZ / 10.0f, will_x / 10.0f, will_z / 10.0f) / speed > 8.0f && GetDistance(fWillX / 10.0f, fWillZ / 10.0f, will_x / 10.0f, will_z / 10.0f) / speed < 10.0f)
		{
			will_x = (will_x + curX1) / 2;
			will_y = (will_y + curY1) / 2;
			will_z = (will_z + curZ1) / 2;
		}
		else if (GetDistance(fWillX / 10.0f, fWillZ / 10.0f, will_x / 10.0f, will_z / 10.0f) / speed >= 12.0f)
		{
			will_x = curX1;
			will_y = curY1;
			will_z = curZ1;
		}
	}

	real_x = will_x / 10.0f; real_z = will_z / 10.0f; real_y = will_y / 10.0f;
	
	if (isSellingMerchant() || isSellingMerchantingPreparing())
		MerchantClose();
	
	m_sSpeed = speed;
	SpeedHackUser();

	m_oldwillx = will_x; m_oldwillz = will_z; m_oldwilly = will_y;

	if (!GetMap()->IsValidPosition(real_x, real_z, real_y)) 
		return;

	if (m_oldx != GetX()
		|| m_oldy != GetY()
		|| m_oldz != GetZ())
	{
		m_oldx = GetX();
		m_oldy = GetY();
		m_oldz = GetZ();
	}

	// TODO: Ensure this is checked properly to prevent speedhacking
	SetPosition(real_x, real_y, real_z);
	 
	if (RegisterRegion())
	{
		RegionNpcInfoForMe();
		RegionUserInOutForMe();
		MerchantUserInOutForMe();
	}

	if (m_PettingOn)
	{
		CNpc* pPet = g_pMain->GetPetPtr(GetSocketID(), GetZoneID());
		if (pPet) {
			if ((pPet->GetState() == (uint8)NpcState::NPC_STANDING
				|| pPet->GetState() == (uint8)NpcState::NPC_MOVING)
				&& (speed == 0
					|| GetDistanceSqrt(pPet) >= 10))
			{
				float	warp_x = pPet->GetX() - GetX(),
					warp_z = pPet->GetZ() - GetZ();

				// Unable to work out orientation, so we'll just fail (won't be necessary with m_sDirection).
				float	distance = sqrtf(warp_x * warp_x + warp_z * warp_z);
				if (distance == 0.0f)
					goto fail_return;

				if (m_PettingOn->sStateChange == MODE_ATTACK)
				{
					if (m_PettingOn->isFamilyAttack())
						m_PettingOn->isFamilyAttackEnd();
				}
				warp_x /= distance; warp_z /= distance;
				warp_x *= 2; warp_z *= 2;
				warp_x += m_oldx; warp_z += m_oldz;

				pPet->SendMoveResult(warp_x, 0, warp_z, distance);
			}
		}
	}
fail_return:
	if (m_bInvisibilityType == (uint8)InvisibilityType::INVIS_DISPEL_ON_MOVE)
		CMagicProcess::RemoveStealth(this, InvisibilityType::INVIS_DISPEL_ON_MOVE);

	if (isMining())
		HandleMiningStop();

	if (isFishing())
		HandleFishingStop((Packet)(WIZ_MINING, FishingStop));

	Packet result;

	if (isGM())
	{
		if (m_bAbnormalType != ABNORMAL_INVISIBLE)
		{
			Packet x(WIZ_MOVE);
			x << uint32(GetSocketID()) << will_x << will_z << will_y << speed << echo;
			SendToRegion(&x, nullptr, GetEventRoom());
		}
	}
	else
	{
		Packet x(WIZ_MOVE);
		x << uint32(GetSocketID()) << will_x << will_z << will_y << speed << echo;
		SendToRegion(&x, nullptr, GetEventRoom());
	}

	if (m_bCheckWarpZoneChange && speed)
		m_bCheckWarpZoneChange = false;

	GetMap()->CheckEvent(real_x, real_z, this);

	if(isEventUser() && isInTempleEventZone())
		m_event_afkcheck = UNIXTIME;

	EventTrapProcess(real_x, real_z, this);
	OreadsZoneTerrainEvent();
	BDWMonumentPointProcess();
}
#pragma endregion 

