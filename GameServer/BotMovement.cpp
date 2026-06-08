#include "StdAfx.h"

void CBot::MoveProcess(float X, float Y, float Z, uint16 will_x, uint16 will_y, uint16 will_z, float sSpeed, uint8 echo)
{
	if (isMage()
		|| isPriest())
	{
		if (m_sSkillCoolDown[0] > (uint32)UNIXTIME)
			return;
	}

	if (!isInGame() || GetMap() == nullptr)
		return;

	if (isDead())
		return;

	if (!GetMap()->IsValidPosition(X, Z, Y))
		return;

	if (m_oldx != GetX()
		|| m_oldz != GetZ())
	{
		m_oldx = GetX();
		m_oldy = GetY();
		m_oldz = GetZ();
	}

	// TODO: Ensure this is checked properly to prevent speedhacking
	SetPosition(X, Y, Z); RegisterRegion();

	Packet result(WIZ_MOVE);
	result << uint32(GetID()) << will_x << will_z << will_y << int16(sSpeed) << echo;
	SendToRegion(&result);
}

void CBot::MoveRegionProcess(float X, float Y, float Z, uint16 will_x, uint16 will_y, uint16 will_z, float sSpeed, uint8 echo)
{
	if (!isInGame() || GetMap() == nullptr)
		return;

	if (isDead())
		return;

	if (!GetMap()->IsValidPosition(X, Z, Y))
		return;

	if (m_oldx != GetX()
		|| m_oldz != GetZ())
	{
		m_oldx = GetX();
		m_oldy = GetY();
		m_oldz = GetZ();
	}

	// TODO: Ensure this is checked properly to prevent speedhacking
	SetPosition(X, Y, Z); RegisterRegion();

	Packet result(WIZ_MOVE);
	result << uint32(GetID()) << will_x << will_z << will_y << int16(sSpeed) << echo;
	SendToRegion(&result);
}

void CBot::MoveProcessGoDeahTown()
{
	if (isMage()
		|| isPriest())
	{
		if (m_sSkillCoolDown[0] > (uint32)UNIXTIME)
			return;
	}

	switch (GetZoneID())
	{
	case ZONE_RONARK_LAND:
		MoveProcessRonarkLandTown();
		break;
	case ZONE_ARDREAM:
		MoveProcessArdreamLandTown();
		break;
	case ZONE_RONARK_LAND_BASE:
		break;
	}
}

void CBot::WalkRegionCordinat(int16 m_sSocketID, float x, float y, float z, uint16 Delay, bool isAttack)
{
	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	__Vector3 vBot, vUser, vDistance, vRealDistance;

	Mesafe = pow(x - GetX(), 2.0f) + pow(z - GetZ(), 2.0f);
	if (Mesafe == EnYakinMesafe)
	{
		if (WalkStep == 15)
			m_Reverse = true;

		if (WalkStep == 0)
			m_Reverse = false;

		if (m_Reverse)
			WalkStep--;
		else
			WalkStep++;

		float(x + myrand(1, 15) * 2.0f);
		float(z + myrand(1, 15) * 2.0f);
	}

	if (isAttack)
	{
		if (!isRegionTargetUp())
			m_sRegionAttack = true;
	}

	vBot.Set(GetX(), GetY(), GetZ());
	vUser.Set(x + ((myrand(0, 2000) - 1000.0f) / 500.0f), y, z + ((myrand(0, 2000) - 1000.0f) / 500.0f));

	if (m_sTargetID != m_sSocketID)
	{
		m_TargetChanged = true;
		m_sTargetID = m_sSocketID;
		m_oldx = vUser.x + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_oldz = vUser.z + ((myrand(0, 2000) - 1000.0f) / 500.0f);
		m_oldy = vUser.y;
	}

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = m_sSpeed;
	uint8 KosuSuresi = 1;
	bool sRunFinish = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 100.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 100.0f;
		KosuSuresi = 100;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		sRunFinish = true;
		vDistance = vRealDistance;
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

	uint16 will_x, will_z, will_y;
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);
	MoveRegionProcess((vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, speed, echo);
}

bool CBot::ZoneArdreamMoveType()
{
	switch (s_MoveProcess)
	{
	case 1:
		if (m_MoveState++ > 14)
			return false;
		break;
	case 2:
		if (m_MoveState++ > 14)
			return false;
		break;
	case 5:
		if (m_MoveState++ > 14)
			return false;
		break;
	case 6:
		if (m_MoveState++ > 14)
			return false;
		break;
	case 7:
		if (m_MoveState++ > 14)
			return false;
		break;
	case 8:
		if (m_MoveState++ > 14)
			return false;
		break;
	case 9:
		if (m_MoveState++ > 14)
			return false;
		break;
	case 10:
		if (m_MoveState++ > 14)
			return false;
		break;
	case 3:
		if (m_MoveState++ > 13)
			return false;
		break;
	case 4:
		if (m_MoveState++ > 13)
			return false;
		break;
	}
	return true;
}

bool CBot::ZoneRonarkLandMoveType()
{
	switch (s_MoveProcess)
	{
	case 1:
		if (m_MoveState++ > 19)
			return false;
		break;
	case 2:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 34)
				return false;
		}
		else
		{
			if (m_MoveState++ > 29)
				return false;
		}
	}break;
	case 3:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 26)
				return false;
		}
		else
		{
			if (m_MoveState++ > 31)
				return false;
		}
	}break;
	case 4:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 19)
				return false;
		}
		else
		{
			if (m_MoveState++ > 34)
				return false;
		}
	}break;
	case 5:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 21)
				return false;
		}
		else
		{
			if (m_MoveState++ > 39)
				return false;
		}
	}break;
	case 6:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 17)
				return false;
		}
		else
		{
			if (m_MoveState++ > 34)
				return false;
		}
	}break;
	case 7:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 22)
				return false;
		}
		else
		{
			if (m_MoveState++ > 25)
				return false;
		}
	}break;
	case 8:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 16)
				return false;
		}
		else
		{
			if (m_MoveState++ > 24)
				return false;
		}
	}break;
	case 9:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 21)
				return false;
		}
		else
		{
			if (m_MoveState++ > 24)
				return false;
		}
	}break;
	case 10:
	{
		if (GetNation() == KARUS)
		{
			if (m_MoveState++ > 23)
				return false;
		}
		else
		{
			if (m_MoveState++ > 30)
				return false;
		}
	}break;
	}
	return true;
}

void CBot::WalkCordinat(float x, float y, float z, uint16 Delay, bool isAttack)
{
	float Mesafe = 0.0f, EnYakinMesafe = 0.0f;
	__Vector3 vBot, vUser, vDistance, vRealDistance;

	Mesafe = pow(x - GetX(), 2.0f) + pow(z - GetZ(), 2.0f);
	if (Mesafe == EnYakinMesafe)
	{
		if (WalkStep == 15)
			m_Reverse = true;

		if (WalkStep == 0)
			m_Reverse = false;

		if (m_Reverse)
			WalkStep--;
		else
			WalkStep++;

		float(x + myrand(1, 15) * 2.0f);
		float(z + myrand(1, 15) * 2.0f);

		bool m_sMoveTypeStatus = false;

		switch (GetZoneID())
		{
		case ZONE_RONARK_LAND:
			if (ZoneRonarkLandMoveType() == false)
				m_sMoveTypeStatus = true;
			break;
		case ZONE_ARDREAM:
			if (ZoneArdreamMoveType() == false)
				m_sMoveTypeStatus = true;
			break;
		default:
			break;
		}
		
		if (m_sMoveTypeStatus)
		{
			isReset(false);
			Regene(INOUT_IN, isInPKZone() ? 0 : 112754);
			return;
		}
	}

	if (isAttack)
	{
		if (!isRegionTargetUp())
			m_sRegionAttack = true;
	}

	vBot.Set(GetX(), GetY(), GetZ());
	vUser.Set(x, y, z);

	vDistance = vUser - vBot;
	vRealDistance = vDistance;
	vDistance.Normalize();

	float speed = m_sSpeed;
	uint8 KosuSuresi = 1;
	bool KosuBitirme = false;
	vDistance *= speed / 10.0f;

	if (echo == uint8(0)
		&& vDistance.Magnitude() < vRealDistance.Magnitude()
		&& (vDistance * 100.0f).Magnitude() < vRealDistance.Magnitude())
	{
		vDistance *= 100.0f;
		KosuSuresi = 100;
	}
	else if (vDistance.Magnitude() > vRealDistance.Magnitude()
		|| vDistance.Magnitude() == vRealDistance.Magnitude())
	{
		KosuBitirme = true;
		vDistance = vRealDistance;
	}

	uint16 will_x, will_z, will_y;
	echo = uint8(3);
	will_x = uint16((vBot + vDistance).x * 10.0f);
	will_y = uint16(vUser.y * 10.0f);
	will_z = uint16((vBot + vDistance).z * 10.0f);
	MoveRegionProcess((vBot + vDistance).x, vUser.y, (vBot + vDistance).z, will_x, will_y, will_z, speed, echo);
}

void CBot::MoveProcessRonarkLandTown()
{
	float UnitX = 0, UnitY = 0, UnitZ = 0;
	float sRange = 45.0f;

	switch (s_MoveProcess)
	{
	case 1:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1375) : float(623);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1099) : float(902);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1276) : float(668);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1056) : float(917);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1212) : float(731);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(901) : float(938);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1088) : float(770);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(771) : float(1007);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(966) : float(820);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(876) : float(1078);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(745) : float(863);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(937) : float(1140);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(726) : float(910);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(921) : float(1166);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(734) : float(939);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1016) : float(1201);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(574) : float(1019);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1056) : float(1207);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(463) : float(1067);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(910) : float(1206);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(517) : float(1098);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(880) : float(1215);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(538) : float(1127);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(792) : float(1182);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(693) : float(1201);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(794) : float(1145);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(772) : float(1229);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(941) : float(1094);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(725) : float(1267);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(946) : float(1059);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(717) : float(1275);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(918) : float(1034);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(775) : float(1243);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(944) : float(1041);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(735) : float(1227);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(956) : float(1100);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(718) : float(1267);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(928) : float(978);
			break;
		}
	}break;
	case 2:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1377) : float(623);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1100) : float(902);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1350) : float(647);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1087) : float(920);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1311) : float(689);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1035) : float(960);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1205) : float(728);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(916) : float(989);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(1164) : float(776);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(968) : float(1014);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(994) : float(801);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(973) : float(1080);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(949) : float(818);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1047) : float(1142);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(871) : float(876);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1049) : float(1196);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(843) : float(922);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1103) : float(1214);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(613) : float(952);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1055) : float(1244);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(432) : float(1024);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1118) : float(1251);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(374) : float(1010);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1042) : float(1207);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(392) : float(1054);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(957) : float(1200);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(322) : float(1138);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(896) : float(1233);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(143) : float(1155);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(887) : float(1174);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(282) : float(1204);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(695) : float(1145);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(364) : float(1226);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(604) : float(1097);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(279) : float(1247);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(518) : float(1057);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(193) : float(1286);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(590) : float(1066);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(129) : float(1246);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(674) : float(1055);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(182) : float(1267);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(595) : float(1044);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(303) : float(1309);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(513) : float(989);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(403) : float(1316);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(645) : float(942);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(506) : float(1381);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(735) : float(952);
			break;
		case 25:
			UnitX = GetNation() == KARUS ? float(586) : float(1435);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(782) : float(968);
			break;
		case 26:
			UnitX = GetNation() == KARUS ? float(709) : float(1466);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(811) : float(988);
			break;
		case 27:
			UnitX = GetNation() == KARUS ? float(764) : float(1484);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(933) : float(1066);
			break;
		case 28:
			UnitX = GetNation() == KARUS ? float(726) : float(1508);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(945) : float(1133);
			break;
		case 29:
			UnitX = GetNation() == KARUS ? float(732) : float(1460);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(923) : float(1108);
			break;
		case 30:
			UnitX = GetNation() == KARUS ? float(776) : float(0);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(951) : float(0);
			break;
		case 31:
			UnitX = GetNation() == KARUS ? float(712) : float(0);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(937) : float(0);
			break;
		case 32:
			UnitX = GetNation() == KARUS ? float(746) : float(0);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(928) : float(0);
			break;
		case 33:
			UnitX = GetNation() == KARUS ? float(703) : float(0);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(949) : float(0);
			break;
		case 34:
			UnitX = GetNation() == KARUS ? float(776) : float(0);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(937) : float(0);
			break;
		}
	}break;
	case 3:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1380) : float(624);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1102) : float(901);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1269) : float(661);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1058) : float(906);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1165) : float(685);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1173) : float(899);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1034) : float(710);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1196) : float(917);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(952) : float(747);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1209) : float(923);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(799) : float(770);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1067) : float(952);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(738) : float(795);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(935) : float(1022);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(715) : float(836);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(915) : float(1091);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(740) : float(862);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(955) : float(1077);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(776) : float(876);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(944) : float(1052);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(726) : float(897);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(946) : float(1079);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(768) : float(927);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(934) : float(1063);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(717) : float(963);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(919) : float(1081);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(749) : float(992);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(966) : float(1112);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(708) : float(1031);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1022) : float(1144);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(573) : float(1075);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1065) : float(1151);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(469) : float(1117);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(909) : float(1160);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(541) : float(1137);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(807) : float(1158);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(688) : float(1179);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(795) : float(1150);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(773) : float(1209);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(930) : float(1125);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(731) : float(1223);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(941) : float(1088);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(723) : float(1244);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(923) : float(1059);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(742) : float(1251);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(947) : float(984);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(724) : float(1257);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(938) : float(942);
			break;
		case 25:
			UnitX = GetNation() == KARUS ? float(722) : float(1301);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(921) : float(936);
			break;
		case 26:
			UnitX = GetNation() == KARUS ? float(750) : float(1344);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(934) : float(931);
			break;
		case 27:
			UnitX = GetNation() == KARUS ? float(0) : float(1308);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(972);
			break;
		case 28:
			UnitX = GetNation() == KARUS ? float(0) : float(1270);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1052);
			break;
		case 29:
			UnitX = GetNation() == KARUS ? float(0) : float(1282);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1070);
			break;
		case 30:
			UnitX = GetNation() == KARUS ? float(0) : float(1217);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1100);
			break;
		case 31:
			UnitX = GetNation() == KARUS ? float(0) : float(1266);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1062);
			break;
		}
	}break;
	case 4:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1380) : float(626);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1103) : float(901);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1460) : float(638);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1081) : float(912);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1589) : float(653);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1067) : float(919);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1626) : float(675);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1011) : float(949);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(1794) : float(710);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1185) : float(930);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(1521) : float(762);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1573) : float(919);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(1368) : float(795);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1483) : float(912);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(1136) : float(830);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1265) : float(843);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(1060) : float(869);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1201) : float(819);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(869) : float(892);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1210) : float(806);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(793) : float(929);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1069) : float(806);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(764) : float(989);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(937) : float(795);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(737) : float(1060);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(923) : float(798);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(714) : float(1142);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(934) : float(815);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(758) : float(1186);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(960) : float(843);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(742) : float(1194);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(923) : float(888);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(726) : float(1223);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(955) : float(906);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(802) : float(1227);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(903) : float(933);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(732) : float(1250);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(935) : float(982);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(0) : float(1274);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1007);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(0) : float(1266);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1060);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(0) : float(1204);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1129);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(0) : float(1178);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1170);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(0) : float(1119);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1185);
			break;
		case 25:
			UnitX = GetNation() == KARUS ? float(0) : float(1102);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1216);
			break;
		case 26:
			UnitX = GetNation() == KARUS ? float(0) : float(1154);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1240);
			break;
		case 27:
			UnitX = GetNation() == KARUS ? float(0) : float(1176);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1301);
			break;
		case 28:
			UnitX = GetNation() == KARUS ? float(0) : float(1221);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1349);
			break;
		case 29:
			UnitX = GetNation() == KARUS ? float(0) : float(1264);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1307);
			break;
		case 30:
			UnitX = GetNation() == KARUS ? float(0) : float(1347);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1449);
			break;
		case 31:
			UnitX = GetNation() == KARUS ? float(0) : float(1381);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1488);
			break;
		case 32:
			UnitX = GetNation() == KARUS ? float(0) : float(1428);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1530);
			break;
		case 33:
			UnitX = GetNation() == KARUS ? float(0) : float(1491);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1532);
			break;
		case 34:
			UnitX = GetNation() == KARUS ? float(0) : float(1425);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1572);
			break;

		}
	}break;
	case 5:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1375) : float(627);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1102) : float(903);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1257) : float(636);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1045) : float(913);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1218) : float(623);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(897) : float(893);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1166) : float(628);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(965) : float(896);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(1030) : float(634);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(972) : float(925);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(1020) : float(647);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1023) : float(925);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(866) : float(672);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1062) : float(921);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(838) : float(713);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1098) : float(927);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(721) : float(728);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(925) : float(910);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(773) : float(777);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(931) : float(915);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(723) : float(802);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(953) : float(892);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(918) : float(833);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(767) : float(844);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(1017) : float(861);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(805) : float(831);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(1056) : float(886);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(923) : float(803);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(1001) : float(906);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(972) : float(776);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(944) : float(952);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1051) : float(764);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(872) : float(996);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1056) : float(769);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(836) : float(1036);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1096) : float(750);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(726) : float(1088);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(943) : float(744);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(771) : float(1118);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(938) : float(751);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(716) : float(1125);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(928) : float(763);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(0) : float(1032);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(793);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(0) : float(1177);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(840);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(0) : float(1233);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(865);
			break;
		case 25:
			UnitX = GetNation() == KARUS ? float(0) : float(1301);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(918);
			break;
		case 26:
			UnitX = GetNation() == KARUS ? float(0) : float(1270);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(970);
			break;
		case 27:
			UnitX = GetNation() == KARUS ? float(0) : float(1257);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1011);
			break;
		case 28:
			UnitX = GetNation() == KARUS ? float(0) : float(1272);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1059);
			break;
		case 29:
			UnitX = GetNation() == KARUS ? float(0) : float(1282);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1076);
			break;
		case 30:
			UnitX = GetNation() == KARUS ? float(0) : float(1269);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1119);
			break;
		case 31:
			UnitX = GetNation() == KARUS ? float(0) : float(1281);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1165);
			break;
		case 32:
			UnitX = GetNation() == KARUS ? float(0) : float(1316);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1180);
			break;
		case 33:
			UnitX = GetNation() == KARUS ? float(0) : float(1353);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1198);
			break;
		case 34:
			UnitX = GetNation() == KARUS ? float(0) : float(1417);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1196);
			break;
		case 35:
			UnitX = GetNation() == KARUS ? float(0) : float(1469);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1187);
			break;
		case 36:
			UnitX = GetNation() == KARUS ? float(0) : float(1499);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1143);
			break;
		case 37:
			UnitX = GetNation() == KARUS ? float(0) : float(1505);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1085);
			break;
		case 38:
			UnitX = GetNation() == KARUS ? float(0) : float(1460);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1105);
			break;
		case 39:
			UnitX = GetNation() == KARUS ? float(0) : float(1380);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1091);
			break;

		}
	}break;
	case 6:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1375) : float(625);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1102) : float(895);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1389) : float(605);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1069) : float(896);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1245) : float(605);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1081) : float(925);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1100) : float(586);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1209) : float(898);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(1024) : float(511);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1189) : float(880);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(1043) : float(485);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1032) : float(902);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(1050) : float(485);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(971) : float(933);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(979) : float(533);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(961) : float(1010);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(935) : float(558);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1045) : float(1046);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(868) : float(630);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1066) : float(1052);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(838) : float(703);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1054) : float(1073);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(733) : float(769);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(953) : float(1083);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(745) : float(819);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(920) : float(1086);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(730) : float(842);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(939) : float(1104);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(706) : float(874);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(938) : float(1054);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(762) : float(932);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(958) : float(1050);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(736) : float(972);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(969) : float(1037);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(0) : float(972);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1037);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(0) : float(1059);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(986);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(0) : float(1099);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(959);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(0) : float(1146);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(965);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(0) : float(1175);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(971);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(0) : float(1190);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(927);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(0) : float(1215);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(915);
			break;
		case 25:
			UnitX = GetNation() == KARUS ? float(0) : float(1243);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(959);
			break;
		case 26:
			UnitX = GetNation() == KARUS ? float(0) : float(1248);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1003);
			break;
		case 27:
			UnitX = GetNation() == KARUS ? float(0) : float(1290);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1065);
			break;
		case 28:
			UnitX = GetNation() == KARUS ? float(0) : float(1257);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1060);
			break;
		case 29:
			UnitX = GetNation() == KARUS ? float(0) : float(1264);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1055);
			break;
		case 31:
			UnitX = GetNation() == KARUS ? float(0) : float(1286);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1081);
			break;
		case 32:
			UnitX = GetNation() == KARUS ? float(0) : float(1254);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1045);
			break;
		case 33:
			UnitX = GetNation() == KARUS ? float(0) : float(1300);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1054);
			break;
		case 34:
			UnitX = GetNation() == KARUS ? float(0) : float(1267);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1060);
			break;

		}
	}break;
	case 7:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1376) : float(626);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1102) : float(890);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1272) : float(634);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1048) : float(923);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1206) : float(635);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(896) : float(911);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1124) : float(693);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(777) : float(927);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(982) : float(741);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(794) : float(911);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(832) : float(791);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(728) : float(910);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(794) : float(844);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(923) : float(840);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(708) : float(890);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1023) : float(795);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(578) : float(911);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1065) : float(782);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(526) : float(960);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1006) : float(783);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(439) : float(978);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1013) : float(798);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(334) : float(1015);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(900) : float(809);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(175) : float(1037);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(895) : float(780);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(288) : float(1180);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(689) : float(838);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(464) : float(1230);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(514) : float(867);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(611) : float(1253);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(400) : float(937);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(706) : float(1251);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(541) : float(990);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(843) : float(1269);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(667) : float(1024);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(774) : float(1269);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(943) : float(1060);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(729) : float(1253);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(954) : float(1067);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(770) : float(1250);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(996) : float(1055);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(729) : float(1275);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(936) : float(1047);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(0) : float(1254);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1035);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(0) : float(1291);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1073);
			break;
		case 25:
			UnitX = GetNation() == KARUS ? float(0) : float(1242);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1055);
			break;
		}
	}break;

	case 8:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1375) : float(622);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1091) : float(901);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1487) : float(626);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1111) : float(902);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1463) : float(641);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(970) : float(916);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1267) : float(708);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(901) : float(923);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(1129) : float(751);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(803) : float(997);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(836) : float(800);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(773) : float(1044);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(728) : float(827);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(839) : float(1080);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(754) : float(842);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(930) : float(1092);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(728) : float(860);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(942) : float(1073);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(768) : float(887);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(930) : float(1040);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(735) : float(936);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(951) : float(1042);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(765) : float(971);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(977) : float(1043);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(768) : float(993);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(936) : float(1014);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(730) : float(1023);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(947) : float(986);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(783) : float(1055);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(946) : float(972);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(731) : float(1130);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(950) : float(958);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(0) : float(1172);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(974);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(0) : float(1186);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(916);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(0) : float(1128);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(907);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(0) : float(1253);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(925);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(0) : float(1247);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1005);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(0) : float(1268);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1025);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(0) : float(1260);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1061);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(0) : float(1244);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1053);
			break;
		}
	}break;

	case 9:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1376) : float(625);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1103) : float(902);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1255) : float(654);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1040) : float(915);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1241) : float(692);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(897) : float(886);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(1190) : float(709);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(930) : float(877);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(1177) : float(750);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(966) : float(863);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(1070) : float(768);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(968) : float(855);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(980) : float(809);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1045) : float(845);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(885) : float(875);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1035) : float(818);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(847) : float(893);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1095) : float(810);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(726) : float(931);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(939) : float(803);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(805) : float(979);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(914) : float(795);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(761) : float(1012);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(949) : float(813);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(736) : float(1025);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(920) : float(850);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(743) : float(1052);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(980) : float(887);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(776) : float(1037);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(951) : float(914);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(753) : float(1060);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(946) : float(928);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(720) : float(1102);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(971) : float(951);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(769) : float(1138);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(974) : float(962);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(758) : float(1173);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(941) : float(965);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(712) : float(1189);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(800) : float(940);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(769) : float(1199);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(939) : float(922);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(0) : float(1219);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(917);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(0) : float(1256);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(961);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(0) : float(1271);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1059);
			break;
		}
	}break;

	case 10:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(1378) : float(626);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1102) : float(903);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(1340) : float(597);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1159) : float(915);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(1137) : float(563);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1220) : float(890);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(940) : float(480);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1197) : float(877);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(800) : float(473);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1063) : float(921);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(508) : float(524);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1084) : float(994);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(377) : float(567);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1066) : float(1047);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(389) : float(691);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(956) : float(1070);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(337) : float(777);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(900) : float(1085);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(180) : float(787);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(888) : float(1121);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(287) : float(825);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(694) : float(1148);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(359) : float(894);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(610) : float(1198);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(468) : float(949);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(693) : float(1204);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(547) : float(994);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(794) : float(1201);
			break;
		case 15:
			UnitX = GetNation() == KARUS ? float(694) : float(1026);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(789) : float(1165);
			break;
		case 16:
			UnitX = GetNation() == KARUS ? float(777) : float(1047);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(940) : float(1118);
			break;
		case 17:
			UnitX = GetNation() == KARUS ? float(738) : float(1062);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(925) : float(1096);
			break;
		case 18:
			UnitX = GetNation() == KARUS ? float(770) : float(1106);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(956) : float(1113);
			break;
		case 19:
			UnitX = GetNation() == KARUS ? float(740) : float(1144);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(962) : float(1139);
			break;
		case 20:
			UnitX = GetNation() == KARUS ? float(751) : float(1161);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(935) : float(1151);
			break;
		case 21:
			UnitX = GetNation() == KARUS ? float(651) : float(1183);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(1061) : float(1130);
			break;
		case 22:
			UnitX = GetNation() == KARUS ? float(772) : float(1208);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(969) : float(1121);
			break;
		case 23:
			UnitX = GetNation() == KARUS ? float(765) : float(1248);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(940) : float(1076);
			break;
		case 24:
			UnitX = GetNation() == KARUS ? float(0) : float(1241);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1060);
			break;
		case 25:
			UnitX = GetNation() == KARUS ? float(0) : float(1255);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1045);
			break;
		case 26:
			UnitX = GetNation() == KARUS ? float(0) : float(1250);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1065);
			break;
		case 27:
			UnitX = GetNation() == KARUS ? float(0) : float(1264);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1051);
			break;
		case 28:
			UnitX = GetNation() == KARUS ? float(0) : float(1266);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1060);
			break;
		case 29:
			UnitX = GetNation() == KARUS ? float(0) : float(1257);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1057);
			break;
		case 30:
			UnitX = GetNation() == KARUS ? float(0) : float(1272);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(0) : float(1062);
			break;
		}
	}break;
	}

	Unit* pUnit = g_pMain->GetUnitPtr(m_sTargetID, GetZoneID());
	if (pUnit != nullptr)
	{
		if (!pUnit->isDead())
		{
			float fDis = GetDistanceSqrt(pUnit);
			if (fDis < sRange)
			{
				WalkRegionCordinat(pUnit->GetID(), pUnit->GetX(), pUnit->GetY(), pUnit->GetZ(), 5, true);
				return;
			}
		}
	}

	if (UnitX == 0 && UnitY == 0 && UnitZ == 0)
		return;

	if (UnitX == 0 && UnitZ == 0)
		return;

	WalkCordinat(UnitX, UnitY, UnitZ, 5, true);
}

void CBot::MoveProcessArdreamLandTown()
{
	float UnitX = 0, UnitY = 0, UnitZ = 0;
	float sRange = 45.0f;
	
	switch (s_MoveProcess)
	{
	case 1:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(856) : float(195);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(138) : float(901);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(809) : float(186);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(156) : float(875);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(739) : float(214);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(297) : float(762);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(828) : float(230);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(533) : float(692);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(908) : float(216);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(638) : float(581);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(864) : float(292);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(777) : float(544);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(772) : float(469);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(838) : float(534);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(712) : float(524);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(889) : float(545);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(548) : float(614);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(895) : float(507);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(467) : float(801);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(928) : float(447);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(327) : float(775);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(883) : float(197);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(234) : float(686);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(824) : float(238);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(194) : float(767);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(816) : float(247);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(205) : float(816);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(796) : float(176);
			break;
		}
	}break;
	case 2:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(853) : float(188);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(141) : float(899);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(768) : float(203);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(223) : float(732);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(668) : float(235);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(172) : float(626);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(490) : float(132);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(148) : float(447);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(368) : float(215);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(100) : float(335);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(325) : float(218);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(144) : float(195);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(212) : float(336);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(190) : float(132);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(206) : float(359);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(270) : float(114);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(219) : float(519);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(337) : float(154);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(100) : float(637);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(474) : float(211);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(210) : float(760);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(595) : float(249);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(227) : float(768);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(779) : float(203);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(189) : float(799);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(803) : float(220);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(212) : float(750);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(816) : float(205);
			break;
		}
	}break;
	case 3:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(856) : float(191);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(140) : float(900);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(800) : float(126);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(136) : float(876);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(757) : float(135);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(207) : float(766);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(753) : float(117);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(463) : float(694);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(626) : float(267);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(539) : float(540);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(550) : float(396);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(621) : float(491);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(394) : float(534);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(560) : float(378);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(249) : float(715);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(542) : float(497);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(221) : float(761);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(744) : float(379);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(195) : float(769);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(807) : float(206);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(250) : float(788);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(817) : float(220);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(208) : float(716);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(784) : float(197);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(192) : float(791);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(849) : float(206);
			break;
		}
	}break;
	case 4:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(852) : float(192);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(141) : float(899);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(831) : float(186);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(181) : float(821);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(770) : float(272);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(226) : float(842);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(810) : float(396);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(453) : float(915);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(645) : float(557);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(524) : float(915);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(537) : float(720);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(436) : float(883);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(478) : float(789);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(423) : float(822);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(314) : float(864);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(542) : float(788);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(221) : float(875);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(577) : float(596);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(216) : float(812);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(697) : float(502);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(193) : float(772);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(804) : float(212);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(239) : float(772);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(823) : float(241);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(198) : float(789);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(819) : float(215);
			break;
		}
	}break;
	case 5:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(855) : float(195);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(140) : float(901);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(806) : float(210);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(157) : float(888);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(762) : float(223);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(209) : float(813);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(593) : float(333);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(161) : float(885);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(513) : float(460);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(155) : float(919);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(371) : float(723);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(106) : float(880);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(305) : float(823);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(153) : float(801);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(128) : float(871);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(253) : float(651);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(160) : float(756);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(391) : float(453);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(134) : float(767);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(585) : float(219);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(238) : float(806);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(716) : float(241);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(225) : float(669);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(808) : float(168);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(187) : float(722);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(816) : float(246);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(223) : float(778);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(833) : float(226);
			break;
		}
	}break;
	case 6:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(853) : float(189);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(139) : float(902);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(807) : float(202);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(158) : float(735);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(798) : float(241);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(269) : float(624);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(824) : float(376);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(400) : float(586);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(819) : float(464);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(554) : float(589);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(874) : float(546);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(659) : float(620);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(852) : float(702);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(784) : float(491);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(768) : float(779);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(842) : float(419);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(727) : float(747);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(877) : float(306);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(467) : float(786);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(931) : float(200);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(323) : float(743);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(879) : float(214);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(200) : float(794);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(795) : float(225);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(209) : float(779);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(819) : float(188);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(188) : float(791);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(818) : float(197);
			break;
		}
	}break;
	case 7:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(856) : float(189);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(137) : float(907);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(834) : float(189);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(144) : float(839);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(770) : float(220);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(232) : float(741);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(780) : float(217);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(344) : float(639);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(730) : float(100);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(484) : float(478);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(576) : float(135);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(520) : float(331);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(539) : float(190);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(538) : float(197);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(463) : float(325);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(534) : float(145);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(346) : float(360);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(604) : float(110);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(243) : float(525);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(609) : float(156);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(208) : float(606);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(740) : float(154);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(194) : float(776);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(821) : float(227);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(235) : float(755);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(823) : float(189);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(195) : float(802);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(792) : float(210);
			break;
		}
	}break;
	case 8:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(856) : float(176);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(137) : float(938);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(812) : float(182);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(121) : float(910);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(768) : float(192);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(184) : float(853);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(673) : float(249);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(178) : float(824);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(554) : float(407);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(177) : float(925);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(367) : float(598);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(97) : float(908);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(316) : float(719);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(157) : float(877);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(215) : float(780);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(275) : float(832);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(238) : float(863);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(383) : float(790);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(199) : float(862);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(476) : float(579);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(209) : float(795);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(528) : float(472);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(205) : float(779);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(691) : float(265);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(231) : float(769);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(774) : float(188);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(188) : float(794);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(815) : float(213);
			break;
		}
	}break;
	case 9:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(853) : float(194);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(140) : float(899);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(808) : float(185);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(157) : float(842);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(750) : float(230);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(329) : float(710);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(828) : float(198);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(544) : float(561);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(896) : float(133);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(611) : float(445);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(889) : float(211);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(782) : float(341);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(783) : float(209);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(825) : float(197);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(720) : float(323);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(884) : float(143);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(452) : float(363);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(927) : float(105);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(337) : float(519);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(885) : float(154);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(194) : float(627);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(787) : float(215);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(208) : float(765);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(813) : float(242);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(240) : float(799);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(770) : float(209);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(197) : float(770);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(826) : float(194);
			break;
		}
	}break;
	case 10:
	{
		switch (m_MoveState)
		{
		case 1:
			UnitX = GetNation() == KARUS ? float(858) : float(166);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(117) : float(899);
			break;
		case 2:
			UnitX = GetNation() == KARUS ? float(784) : float(169);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(122) : float(856);
			break;
		case 3:
			UnitX = GetNation() == KARUS ? float(757) : float(208);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(253) : float(782);
			break;
		case 4:
			UnitX = GetNation() == KARUS ? float(755) : float(225);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(457) : float(698);
			break;
		case 5:
			UnitX = GetNation() == KARUS ? float(668) : float(239);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(504) : float(622);
			break;
		case 6:
			UnitX = GetNation() == KARUS ? float(548) : float(367);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(365) : float(602);
			break;
		case 7:
			UnitX = GetNation() == KARUS ? float(401) : float(455);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(478) : float(465);
			break;
		case 8:
			UnitX = GetNation() == KARUS ? float(328) : float(529);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(541) : float(457);
			break;
		case 9:
			UnitX = GetNation() == KARUS ? float(250) : float(694);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(562) : float(500);
			break;
		case 10:
			UnitX = GetNation() == KARUS ? float(212) : float(752);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(633) : float(426);
			break;
		case 11:
			UnitX = GetNation() == KARUS ? float(227) : float(773);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(777) : float(196);
			break;
		case 12:
			UnitX = GetNation() == KARUS ? float(190) : float(769);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(823) : float(197);
			break;
		case 13:
			UnitX = GetNation() == KARUS ? float(289) : float(797);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(841) : float(209);
			break;
		case 14:
			UnitX = GetNation() == KARUS ? float(208) : float(783);
			UnitY = float(GetY());
			UnitZ = GetNation() == KARUS ? float(798) : float(182);
			break;
		}
	}break;
	}

	Unit* pUnit = g_pMain->GetUnitPtr(m_sTargetID, GetZoneID());
	if (pUnit != nullptr)
	{
		if (!pUnit->isDead())
		{
			float fDis = GetDistanceSqrt(pUnit);
			if (fDis < sRange)
			{
				WalkRegionCordinat(pUnit->GetID(), pUnit->GetX(), pUnit->GetY(), pUnit->GetZ(), 5, true);
				return;
			}
		}
	}

	if (UnitX == 0 && UnitY == 0 && UnitZ == 0)
		return;

	if (UnitX == 0 && UnitZ == 0)
		return;

	WalkCordinat(UnitX, UnitY, UnitZ, 5, true);
}