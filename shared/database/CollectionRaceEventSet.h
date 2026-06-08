#pragma once

class CollectionRaceEventListSet : public OdbcRecordset
{
public:
	CollectionRaceEventListSet(OdbcConnection * dbConnection, CollectionRaceEventListArray * pMap): OdbcRecordset(dbConnection), m_pMap(pMap) {}

	virtual tstring GetTableName() { return _T("COLLECTION_RACE_EVENT_SETTINGS"); }
	virtual tstring GetColumns() { 
		return _T("sEventIndex,EventName, Unit1,UnitCount1,Unit2,UnitCount2,Unit3,UnitCount3,MinLevel,Maxlevel,EventZone,EventTime,UserLimit,isRepeatStatus,"
		"AutoStart,AutoHour,AutoMinute"); 
	}

	virtual bool Fetch()
	{
		_COLLECTION_RACE_EVENT_LIST * pData = new _COLLECTION_RACE_EVENT_LIST;

		auto i = 1, sEventIndex = 0;

		_dbCommand->FetchUInt32(i++, pData->m_bEventID);
		_dbCommand->FetchString(i++, pData->m_bEventName);

		for (uint32 j = 0; j < 3; j++) 
		{
			_dbCommand->FetchUInt16(i++, pData->m_bProtoID[j]);
			_dbCommand->FetchUInt16(i++, pData->m_bKillCount[j]);
			
		}

		_dbCommand->FetchByte(i++, pData->m_bMinLevel);
		_dbCommand->FetchByte(i++, pData->m_bMaxLevel);
		_dbCommand->FetchByte(i++, pData->m_bZoneID);
		_dbCommand->FetchUInt32(i++, pData->m_bEventTime);
		_dbCommand->FetchUInt16(i++, pData->m_bUserLimit);
		_dbCommand->FetchByte(i++, pData->m_bCollectionEventListStatus);
		_dbCommand->Fetchtbool(i++, pData->m_bCollectionAutoStatus);
		_dbCommand->FetchInt32(i++, pData->m_bCollectionAutoHour);
		_dbCommand->FetchInt32(i++, pData->m_bCollectionAutoMinute);
		if (!m_pMap->PutData(pData->m_bEventID, pData))
			delete pData;

		return true;
	}

	CollectionRaceEventListArray * m_pMap;
};