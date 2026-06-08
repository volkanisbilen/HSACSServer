#include "stdafx.h"

void CUser::VersionCheck(Packet & pkt)
{
	if (m_bSelectedCharacter || m_bIsLoggingOut)
		return;

	Packet SendPacket;
	DWORD PrivateKey[4] = { m_Aes.GenerateIV(),m_Aes.GenerateIV(),m_Aes.GenerateIV(),m_Aes.GenerateIV() };
	SendPacket.clear();
	SendPacket.Initialize(WIZ_VERSION_CHECK);
	SendPacket
		<< uint8(0)
		<< uint16(__VERSION)
		<< uint8(0)
		<< uint32(0)
		<< uint32(0)
		<< uint32(0)
		<< uint32(0)
		<< uint8(0); // 0 = success, 1 = prem error 
	Send(&SendPacket);
	//EnableAes(PrivateKey);
}

void CUser::KickOutProcess(Packet & pkt)
{
	std::string account;
	pkt >> account;
	if (account.empty() || account.size() > MAX_ID_SIZE)
		return;

	CUser* pUser = g_pMain->GetUserPtr(account, NameType::TYPE_ACCOUNT);
	if (pUser != nullptr)
	{
		if (pUser->isOfflineStatus())
		{
			pUser->SetOfflineStatus(_choffstatus::DEACTIVE);
			return pUser->goDisconnect("Offline Merchant Disconnect.", __FUNCTION__);
		}
		else
			return pUser->goDisconnect("Disconnect.", __FUNCTION__);
	}
	return;
}

void CUser::ReqKickOutProcess(Packet& pkt)
{
	std::string accountid, passid;
	pkt >> accountid >> passid;

	if (accountid.empty() || accountid.size() > MAX_ID_SIZE
		|| passid.empty())
		return;

	std::string db_password = g_DBAgent.getmypassword(accountid);
	if (db_password.empty() || db_password != passid)
		return;

	CUser* pUser = g_pMain->GetUserPtr(accountid, NameType::TYPE_ACCOUNT);
	if (pUser)
		pUser->goDisconnect();
}

void CUser::SendLoginFailed(int8 errorid) {

	Packet result(WIZ_LOGIN);
	result << int8(-1);
	Send(&result);
}

void CUser::LoginProcess(Packet & pkt)
{
	if (!m_strAccountID.empty() 
		|| !string_is_valid(m_strAccountID))
		return;

	if (m_bSelectedCharacter 
		|| m_bIsLoggingOut 
		|| !m_strAccountID.empty())
		return SendLoginFailed(-1);

	std::string strAccountID, strPasswd;
	//uint8 tes, tes1, tes2, tes3;
	
	//pkt.DByte();
	pkt >> strAccountID >> strPasswd;
	//printf("LoginProcess %d %s %s\n", tes, strAccountID.c_str(), strPasswd.c_str());
	if (strAccountID.empty() 
		|| strAccountID.size() > MAX_ID_SIZE
		|| strPasswd.empty() 
		|| strPasswd.size() > MAX_PW_SIZE)
		return SendLoginFailed(-1);
	
	bool disconnect = false;
	CUser* pUser = g_pMain->GetUserPtr(strAccountID, NameType::TYPE_ACCOUNT);
	if (pUser != nullptr)
	{
		if (pUser->IsDeleted() && !pUser->isOfflineStatus())
			return SendLoginFailed(-1);
		else if (pUser->isOfflineStatus())
		{
			pUser->SetOfflineStatus(_choffstatus::DEACTIVE);
			return pUser->goDisconnect("Offline Merchant Disconnect.", __FUNCTION__);
		}
	}

	if (pUser)
		disconnect = true;

	Packet result(WIZ_LOGIN);
	result << strPasswd << strAccountID << disconnect;
	g_pMain->AddDatabaseRequest(result, this);
}