#include "stdafx.h"
#include "../shared/database/OdbcConnection.h"
#include "DBAgent.h"

using std::string;
using std::unique_ptr;


bool CDBAgent::LoadQuestData(std::string & strCharID, CUser *pUser)
{
	unique_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == nullptr)
		return false;

	if (pUser == nullptr
		|| strCharID.length() > MAX_ID_SIZE
		|| pUser->m_sQuestMap.GetSize() > 0)
		return false;

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());

	if (!dbCommand->Execute(_T("{CALL LOAD_USER_QUEST_DATA(?)}")))
		ReportSQLError(m_GameDB->GetError());

	//OPEN SQL
	if (!dbCommand->hasData())
		return false;

	pUser->m_sDailyQuestMap.DeleteAllData();

	char strQuest[QUEST_ARRAY_SIZE]{ }, strQuestDaily[QUEST_DAILYARRAY_SIZE]{ };

	uint16 sQuestCount = 0, sQuestDailyCount = 0;
	int field = 1;

	dbCommand->FetchUInt16(field++, sQuestCount);
	dbCommand->FetchBinary(field++, strQuest, sizeof(strQuest));

	dbCommand->FetchUInt16(field++, sQuestDailyCount);
	dbCommand->FetchBinary(field++, strQuestDaily, sizeof(strQuestDaily));

	pUser->m_sQuestMap.DeleteAllData();
	// Convert the old quest storage format to the new one.
	if (sQuestCount > QUEST_LIMIT) sQuestCount = QUEST_LIMIT;
	if (sQuestDailyCount > QUEST_DAILYLIMIT) sQuestDailyCount = QUEST_DAILYLIMIT;

	//UPDATE SQL PROSEDUR tinyint to smallint

	for (int i = 0, index = 0; i < sQuestCount; i++, index += 7)
	{
		_USER_QUEST_INFO *pUserQuest = new _USER_QUEST_INFO();
		uint16	sQuestID = *(uint16 *)(strQuest + index);
		pUserQuest->QuestState = *(uint8  *)(strQuest + index + 2);
		pUserQuest->m_bKillCounts[0] = *(uint8  *)(strQuest + index + 3);
		pUserQuest->m_bKillCounts[1] = *(uint8  *)(strQuest + index + 4);
		pUserQuest->m_bKillCounts[2] = *(uint8  *)(strQuest + index + 5);
		pUserQuest->m_bKillCounts[3] = *(uint8  *)(strQuest + index + 6);
		if (!pUser->m_sQuestMap.PutData(sQuestID, pUserQuest)) {
			delete pUserQuest;
			return false;
		}
	}

	for (int i = 0, index = 0; i < sQuestDailyCount; i++, index += 8) {
		_DAILY_USERQUEST *pUserQuest = new _DAILY_USERQUEST();
		uint8 sQuestID = *(uint8 *)(strQuestDaily + index);
		pUserQuest->status = *(uint8  *)(strQuestDaily + index + 1);
		pUserQuest->kcount = *(uint16  *)(strQuestDaily + index + 2);
		pUserQuest->replaytime = *(uint32  *)(strQuestDaily + index + 4);
		if (!pUser->m_sDailyQuestMap.PutData(sQuestID, pUserQuest))
			return false;
	}

	std::vector<uint8> mlist;
	g_pMain->m_DailyQuestArray.m_lock.lock();
	auto m_sDailyQuestArray = g_pMain->m_DailyQuestArray.m_UserTypeMap;
	g_pMain->m_DailyQuestArray.m_lock.unlock();

	foreach(itr, m_sDailyQuestArray) {
		if (itr->second)
			mlist.push_back(itr->second->index);
	}

	foreach(itr, mlist) 
	{
		auto* pDailyQuestUser = pUser->m_sDailyQuestMap.GetData(*itr);
		if (pDailyQuestUser) 
			continue;

		_DAILY_USERQUEST* pnew = new _DAILY_USERQUEST();
		pnew->kcount = 0;
		pnew->status = (uint8)DailyQuestStatus::ongoing;
		pnew->replaytime = 0;
		if (!pUser->m_sDailyQuestMap.PutData(*itr, pnew))
			delete pnew;
	}

	if (!pUser->CheckExistEvent(STARTER_SEED_QUEST, 2))
		pUser->SaveEvent(STARTER_SEED_QUEST, 2);

	return true;
}

bool CDBAgent::UpdateQuestData(std::string & strCharID, CUser *pUser)
{
	if (pUser == nullptr) return false;

	unique_ptr<OdbcCommand> dbCommand(m_GameDB->CreateCommand());
	if (dbCommand.get() == nullptr || strCharID != pUser->GetName()) return false;

	char strQuest[QUEST_ARRAY_SIZE];
	memset(strQuest, 0, sizeof(strQuest));

	char strQuestDaily[QUEST_DAILYARRAY_SIZE];
	memset(strQuestDaily, 0, sizeof(strQuestDaily));

	int index = 0;
	pUser->m_sQuestMap.m_lock.lock();
	auto m_sQuestMap = pUser->m_sQuestMap.m_UserTypeMap;
	pUser->m_sQuestMap.m_lock.unlock();

	foreach(itr, m_sQuestMap)
	{
		if (itr->second == nullptr) 
			continue;

		*(uint16 *)(strQuest + index) = itr->first;
		*(uint8  *)(strQuest + index + 2) = itr->second->QuestState;
		*(uint8  *)(strQuest + index + 3) = itr->second->m_bKillCounts[0];
		*(uint8  *)(strQuest + index + 4) = itr->second->m_bKillCounts[1];
		*(uint8  *)(strQuest + index + 5) = itr->second->m_bKillCounts[2];
		*(uint8  *)(strQuest + index + 6) = itr->second->m_bKillCounts[3];
		index += 7;
	}

	index = 0;
	pUser->m_sDailyQuestMap.m_lock.lock();
	auto m_sDailyQuestMap = pUser->m_sDailyQuestMap.m_UserTypeMap;
	pUser->m_sDailyQuestMap.m_lock.unlock();

	foreach(itr, m_sDailyQuestMap)
	{
		if (itr->second == nullptr) 
			continue;

		*(uint8 *)(strQuestDaily + index) = itr->first;
		*(uint8 *)(strQuestDaily + index + 1) = itr->second->status;
		*(uint16 *)(strQuestDaily + index + 2) = itr->second->kcount;
		*(uint32 *)(strQuestDaily + index + 4) = itr->second->replaytime;
		index += 8;
	}

	dbCommand->AddParameter(SQL_PARAM_INPUT, strCharID.c_str(), strCharID.length());
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strQuest, sizeof(strQuest), SQL_BINARY);
	dbCommand->AddParameter(SQL_PARAM_INPUT, (char *)strQuestDaily, sizeof(strQuestDaily), SQL_BINARY);
	if (!dbCommand->Execute(string_format(_T("{CALL UPDATE_USER_QUEST_DATA(?, ?, ?, %d, %d)}"), pUser->m_sQuestMap.GetSize(), pUser->m_sDailyQuestMap.GetSize()))) {
		ReportSQLError(m_GameDB->GetError());
		return false;
	}
	return true;
}