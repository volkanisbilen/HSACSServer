#include "stdafx.h"
#include <sstream>
#include <iostream>

#include <time.h>
#include <fstream>

#include "../shared/Ini.h"
#include "../shared/DateTime.h"

extern bool g_bRunning;
std::vector<Thread *> g_LogintimerThreads;

LoginServer::LoginServer() { Initialize(); }

void LoginServer::Initialize()
{
	m_sLastVersion = __VERSION;
	m_fpLoginServer = false;
}

bool LoginServer::Startup()
{
	DateTime time;
	//GetInfoFromIni();
	//// Server Start
	//printf("Server started on %04d-%02d-%02d at %02d:%02d\n\n", time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute());
	//if (!m_DBProcess.Connect(m_ODBCName, m_ODBCLogin, m_ODBCPwd))
	//{
	//	printf("LoginServer ODBC Connect Error\n");
	//	return false;
	//}

	//m_DBProcess.LoadVersionList();
	//m_DBProcess.LoadServerList();
	//m_DBProcess.LoadKingNotice();
	//InitPacketHandlers();
	//UpdateServerList();

	//g_LogintimerThreads.push_back(new Thread(Timer_UpdateUserCount));
	//g_LogintimerThreads.push_back(new Thread(Timer_UpdateKingNotice));
	
	
	/*// License System Stard System..
	m_HardwareIDArray.push_back(628452953932025);
	m_HardwareIDArray.push_back(254442953937629);
	m_HardwareIDArray.push_back(2517529539244);
	m_HardwareIDArray.push_back(459122953948522);
	m_HardwareIDArray.push_back(25175295392441);
	if (!g_HardwareInformation.IsValidHardwareID(m_HardwareIDArray))
	{
		printf("Server is shutdown :(\n");
		return false;
	}
	// License System End System..*/

	GetInfoFromIni();

	CreateDirectory("Logs",NULL);

	m_fpLoginServer = fopen("./Logs/LoginServer.log", "a");
	if (m_fpLoginServer == nullptr)
	{
		printf("ERROR: Unable to open log file.\n");
		return false;
	}

	m_fpUser = fopen(string_format("./Logs/Login_%d_%d_%d.log",time.GetDay(),time.GetMonth(),time.GetYear()).c_str(), "a");
	if (m_fpUser == nullptr)
	{
		printf("ERROR: Unable to open user log file.\n");
		return false;
	}

	if (!m_DBProcess.Connect(m_ODBCName, m_ODBCLogin, m_ODBCPwd)) 
	{
		printf("HATA: ODBC ile Yapilan ayarlar nedeniyle veritabanina baglanilamiyor.\n");
		return false;
	}

	printf("Database connection started successfully.\n");

	m_DBProcess.LoadKingNotice();

	if (!m_DBProcess.LoadVersionList())
	{
		printf("HATA: Version listesi yuklenemiyor.\n");
		return false;
	}

	printf("Latest version in database : %d\n", GetVersion());
	if (!m_DBProcess.LoadServerList())
	{
		printf("HATA: Version listesi yuklenemiyor.\n");
		return false;
	}

	InitPacketHandlers();

	for (int i = 0; i < 10; i++)
		if (!m_sClientSocket[i].Listen(m_LoginServerPort + i, MAX_USER))
		{
			printf("ERROR: Failed to listen on server port.\n");
			return false;
		}

	for (int i = 0; i < 10; i++)
		m_sClientSocket[i].RunServer(true);

	if (!m_GameServerSocketMgr.Listen(m_GameServerSocketPort, 10))
	{
		printf("ERROR: Failed to listen on server port %d.\n", m_GameServerSocketPort);
		return false;
	}

	m_GameServerSocketMgr.RunServer(false);

	g_LogintimerThreads.push_back(new Thread(Timer_UpdateUserCount));
	g_LogintimerThreads.push_back(new Thread(Timer_UpdateKingNotice));
	printf("Login Server Startup\n");
	return true;
}

uint32 LoginServer::Timer_UpdateUserCount(void * lpParam)
{
	while (true)
	{
		g_pMain->UpdateServerList();
		sleep(60 * SECOND);
	}
	return 0;
}

uint32 LoginServer::Timer_UpdateKingNotice(void * lpParam)
{
	while (true)
	{
		sleep(600 * SECOND);
		
		g_pMain->m_DBProcess.LoadKingNotice();
	}
	return 0;
}

void LoginServer::GetServerList(Packet & result)
{
	Guard lock(m_serverListLock);
	result.append(m_serverListPacket.contents(), m_serverListPacket.size());
}

void LoginServer::UpdateServerList()
{
	// Update the user counts first
	m_DBProcess.LoadUserCountList();

	Guard lock(m_serverListLock);
	Packet & result = m_serverListPacket;

	m_ServerList.m_lock.lock();
	auto m_bServerList = m_ServerList.m_UserTypeMap;
	m_ServerList.m_lock.unlock();

	result.clear();
	result << uint8(m_bServerList.size());
	foreach(itr, m_bServerList)
	{		
		_SERVER_INFO * pServer = itr->second;
		if (pServer == nullptr)
			continue;

		result << pServer->strLanIP << pServer->strServerIP<< pServer->strServerName;

		if (pServer->sUserCount <= pServer->sPlayerCap)
			result << pServer->sUserCount;
		else
			result << int16(-1);

		result << pServer->sServerID << pServer->sGroupID << pServer->sPlayerCap << pServer->sFreePlayerCap << uint8(0) << pServer->strScreenType;

		// we read all this stuff from ini, TODO: make this more versatile.
		result << pServer->strKarusKingName << pServer->strKarusNotice << pServer->strElMoradKingName << pServer->strElMoradNotice;
	}
}

void LoginServer::GetInfoFromIni()
{
	CIni ini(CONF_LOGIN_SERVER);

	ini.GetString("DOWNLOAD", "URL", "ftp.yoursite.net", m_strFtpUrl, false);
	ini.GetString("DOWNLOAD", "PATH", "/", m_strFilePath, false);

	ini.GetString("ODBC", "DSN", "KO_LOGIN", m_ODBCName, false);
	ini.GetString("ODBC", "UID", "username", m_ODBCLogin, false);
	ini.GetString("ODBC", "PWD", "password", m_ODBCPwd, false);

	m_LoginServerPort = ini.GetInt("SETTINGS", "LOGIN_PORT", 15100);

	char key[20];

	// Read news from INI (max 3 blocks)
#define BOX_START '#' << uint8(0) << '\n'
#define BOX_END '#'

	m_news.Size = 0;
	std::stringstream ss;
	for (int i = 0; i < 3; i++)
	{
		std::string title, message;

		_snprintf(key, sizeof(key), "TITLE_%02d", i);
		ini.GetString("NEWS", key, "", title);
		if (title.empty())
			continue;

		_snprintf(key, sizeof(key), "MESSAGE_%02d", i);
		ini.GetString("NEWS", key, "", message);
		if (message.empty())
			continue;

		size_t oldPos = 0, pos = 0;
		ss << title << BOX_START << message << BOX_END;
	}

	m_news.Size = ss.str().size();
	if (m_news.Size)
		memcpy(&m_news.Content, ss.str().c_str(), m_news.Size);
}

void LoginServer::WriteLogFile(std::string & logMessage)
{
	m_lock.lock();
	fwrite(logMessage.c_str(), logMessage.length(), 1, m_fpLoginServer);
	fflush(m_fpLoginServer);
	m_lock.unlock();
}

void LoginServer::WriteUserLogFile(std::string & logMessage)
{
	m_lock.lock();
	fwrite(logMessage.c_str(), logMessage.length(), 1, m_fpUser);
	fflush(m_fpUser);
	m_lock.unlock();
}

void LoginServer::ReportSQLError(OdbcError *pError)
{
	if (pError == nullptr)
		return;

	// This is *very* temporary.
	std::string errorMessage = string_format(_T("ODBC error occurred.\r\nSource: %s\r\nError: %s\r\nDescription: %s\n"),
		pError->Source.c_str(), pError->ExtendedErrorMessage.c_str(), pError->ErrorMessage.c_str());

	TRACE("%s", errorMessage.c_str());
	WriteLogFile(errorMessage);
	delete pError;
}

LoginServer::~LoginServer()
{
	printf("Waiting for timer threads to exit...");
	foreach(itr, g_LogintimerThreads)
	{
		(*itr)->waitForExit();
		delete (*itr);
	}
	printf(" exited.\n");
	Sleep(1 * SECOND);

	m_ServerList.DeleteAllData();

	foreach(itr, m_VersionList)
		delete itr->second;
	m_VersionList.clear();

	if (m_fpLoginServer != nullptr)
		fclose(m_fpLoginServer);

	if (m_fpUser != nullptr)
		fclose(m_fpUser);

	printf("Shutting down socket system...");

	for (int i = 0; i < 10; i++)
		m_sClientSocket[i].Shutdown();

	printf(" done.\n");
	Sleep(1 * SECOND);
}