#include "StdAfx.h"

void CUser::GetOfflineStatus(_choffstatus s, offcharactertype type)
{
	uint32 itemid = 0;

	if (type == offcharactertype::merchant)
	{
		if (GetItem(CFAIRY)->nNum == MERCHANT_AUTO_FISHING)
			itemid = MERCHANT_AUTO_FISHING;
		else if (GetItem(CFAIRY)->nNum == MERCHANT_AUTO_MANING)
			itemid = MERCHANT_AUTO_MANING;
		else
			itemid = 924041913;
	}
	else if (type == offcharactertype::genie)
		itemid = 824041931;
	else if (type == offcharactertype::minning)
		itemid = OFFLINE_MINNING;
	else if (type == offcharactertype::fishing)
		itemid = OFFLINE_FISHING;
	else
		return;

	if (s == _choffstatus::ACTIVE || s == _choffstatus::DEACTIVE) 
	{
		_ITEM_DATA * pItem = GetItem(CFAIRY);
		if (!pItem 
			|| pItem->nNum != itemid)
			return;

		if (s == _choffstatus::ACTIVE)
			m_bOfflineTimeCheck = 1400;
	}

	SetOfflineStatus(s);
	std::string message = "offline";

	if (type == offcharactertype::genie)
		message.append(" genie");
	else if(type == offcharactertype::merchant)
		message.append(" merchant");
	else if (type == offcharactertype::fishing)
		message.append(" fishing");
	else if (type == offcharactertype::minning)
		message.append(" minning");

	if (s == _choffstatus::ACTIVE) {
		m_bOfflineCheck = UNIXTIME + 60;
		message.append(" ready");
	}
	else
		message.append(" has been cancelled");
	SendAcsMessage(message);
}