#include "PrecompCommon.h"
#include "BlackBoard.h"

static int g_NextScriptItem;

BlackBoard::BlackBoard()
{
}

BlackBoard::~BlackBoard()
{
}

bool BlackBoard::PostBBRecord(BBRecordPtr _item)
{
	m_DB.insert(std::make_pair(_item->GetType(), _item));
	return true;
}

int BlackBoard::GetBBRecords(int _type, BBRecordPtr *_items, int _maxitems)
{
	BlackBoardDatabase::iterator it = m_DB.lower_bound(_type);
	BlackBoardDatabase::iterator itEnd = m_DB.upper_bound(_type);

	int iNum = 0;
	while(it != itEnd && iNum < _maxitems)
	{
		if(it->second->GetType() == _type)
			_items[iNum++] = it->second;
		++it;
	}

	return iNum;
}

int BlackBoard::GetNumBBRecords(int _type, int _target)
{
	BlackBoardDatabase::iterator it = m_DB.lower_bound(_type);
	BlackBoardDatabase::iterator itEnd = m_DB.upper_bound(_type);
	int iNum = 0;
	while(it != itEnd)
	{
		if(it->second->GetType() == _type)
		{
			if(_target == bbk_All || it->second->m_Target == _target)
				++iNum;
		}
		++it;
	}
	return iNum;
}

bool BlackBoard::RecordExistsOwner(int _type, int _owner)
{
	BlackBoardDatabase::iterator it = m_DB.lower_bound(_type);
	BlackBoardDatabase::iterator itEnd = m_DB.upper_bound(_type);
	while(it != itEnd)
	{
		if(it->second->GetType() == _type && it->second->m_Owner == _owner)
			return true;
		++it;
	}
	return false;
}

bool BlackBoard::RecordExistsTarget(int _type, int _target)
{
	BlackBoardDatabase::iterator it = m_DB.lower_bound(_type);
	BlackBoardDatabase::iterator itEnd = m_DB.upper_bound(_type);
	while(it != itEnd)
	{
		if(it->second->GetType() == _type && it->second->m_Target == _target)
			return true;
		++it;
	}
	return false;
}

int BlackBoard::RemoveBBRecordByPoster(int _poster, int _type /*= bbk_All*/)
{
	int iCount = 0;

	BlackBoardDatabase::iterator it;
	BlackBoardDatabase::iterator itEnd;
	if(_type == bbk_All)
	{
		it = m_DB.begin();
		itEnd = m_DB.end();
	}
	else
	{
		it = m_DB.lower_bound(_type);
		itEnd = m_DB.upper_bound(_type);		
	}

	while(it != itEnd)
	{
		if(it->second->m_Owner == _poster)
		{
			m_DB.erase(it++);
			++iCount;
		} 
		else
		{
			++it;
		}
	}

	return iCount;
}

int BlackBoard::RemoveBBRecordByTarget(int _target, int _type /*= bbk_All*/)
{
	int iCount = 0;

	BlackBoardDatabase::iterator it;
	BlackBoardDatabase::iterator itEnd;
	if(_type == bbk_All)
	{
		it = m_DB.begin();
		itEnd = m_DB.end();
	}
	else
	{
		it = m_DB.lower_bound(_type);
		itEnd = m_DB.upper_bound(_type);		
	}

	while(it != itEnd)
	{
		if(it->second->m_Target == _target)
		{
			m_DB.erase(it++);
			++iCount;
		} 
		else
		{
			++it;
		}
	}

	return iCount;
}

int BlackBoard::RemoveAllBBRecords(int _type /*= bbk_All*/)
{
	int iCount = (int)m_DB.size();
	if(_type == bbk_All)
	{
		m_DB.clear();
	}
	else
	{
		m_DB.erase(_type);
	}
	return iCount;
}

void BlackBoard::PurgeExpiredRecords()
{
	BlackBoardDatabase::iterator it = m_DB.begin();
	BlackBoardDatabase::iterator itEnd = m_DB.end();
	while(it != itEnd)
	{
		if(it->second->m_DeleteOnExpire && it->second->m_ExpireTime <= IGame::GetTime())
		{
			m_DB.erase(it++);
		}
		else if(it->second->m_DeleteOnRefCount1 && it->second.use_count() <= 1)
		{
			m_DB.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

void BlackBoard::DumpBlackBoardContentsToGame(int _type /*= bbk_All*/)
{
	BlackBoardDatabase::iterator it = m_DB.begin();
	BlackBoardDatabase::iterator itEnd = m_DB.end();
	
	EngineFuncs::ConsoleMessage("-= Global Blackboard =-");
	for( ; it != itEnd; ++it)
	{
		if(it->first == bbk_All || it->first == _type)
		{
			EngineFuncs::ConsoleMessage(va("Type: %d, Poster: %d, Target: %d", 
				it->first, it->second->m_Owner, it->second->m_Target));
		}
	}
	EngineFuncs::ConsoleMessage("-= End Global Blackboard =-");
}

//////////////////////////////////////////////////////////////////////////

int BlackBoard::MakeKey()
{
	return g_NextScriptItem++;
}

BBRecordPtr BlackBoard::AllocRecord(int _type)
{
	BBRecordPtr ptr;
	switch(_type)
	{
	case bbk_DelayGoal:
		ptr.reset(new bbDelayGoal);
		break;
	case bbk_IsTaken:
		ptr.reset(new bbIsTaken);
		break;
	
		//////////////////////////////////////////////////////////////////////////
	case bbk_All:
	default:
		if(_type >= bbk_FirstScript && _type < g_NextScriptItem)
			ptr.reset(new bbScriptItem(_type));
		else
			OBASSERT(0, "Invalid Blackboard Item Type: %d", _type);
	}
	return ptr;
}

//////////////////////////////////////////////////////////////////////////
static int GM_CDECL gmfPostRecord(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(type, 0);
	GM_CHECK_TABLE_PARAM(props, 1);

	BBRecordPtr bbr = BlackBoard::AllocRecord(type);
	if(bbr)
	{
		bbr->FromScriptTable(a_thread->GetMachine(), props);
		g_Blackboard.PostBBRecord(bbr);
	}
	else
	{
		GM_EXCEPTION_MSG("Invalid Blackboard Item Type");
		return GM_EXCEPTION;
	}	
	return GM_OK;
}

static int GM_CDECL gmfGetRecords(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(type, 0);

	gmTableObject *pRecords = NULL;

	enum { MaxRecords = 64 };
	BBRecordPtr records[MaxRecords];
	int c = g_Blackboard.GetBBRecords(type,records,MaxRecords);
	if(c > 0)
	{
		DisableGCInScope gcEn(a_thread->GetMachine());

		pRecords = a_thread->GetMachine()->AllocTableObject();
		for(int i = 0; i < c; ++i)
		{
			gmTableObject *pTbl = a_thread->GetMachine()->AllocTableObject();
			if(records[i]->ToScriptTable(a_thread->GetMachine(), pTbl))
				pRecords->Set(a_thread->GetMachine(), i, gmVariable(pTbl));
		}
		a_thread->PushTable(pRecords);
	}
	else 
		a_thread->PushNull();
	return GM_OK;
}
static int GM_CDECL gmfGetNumRecords(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(1);
	GM_CHECK_INT_PARAM(type, 0);
	GM_INT_PARAM(target, 1, bbk_All);
	a_thread->PushInt(g_Blackboard.GetNumBBRecords(type, target));
	return GM_OK;
}
static int GM_CDECL gmfRemoveByPoster(gmThread *a_thread)
{
	GM_CHECK_INT_PARAM(poster, 0);
	GM_INT_PARAM(type, 1, bbk_All);
	a_thread->PushInt(g_Blackboard.RemoveBBRecordByPoster(poster, type));
	return GM_OK;
}

static int GM_CDECL gmfRemoveByTarget(gmThread *a_thread)
{
	GM_CHECK_INT_PARAM(target, 0);
	GM_INT_PARAM(type, 1, bbk_All);
	a_thread->PushInt(g_Blackboard.RemoveBBRecordByTarget(target, type));
	return GM_OK;
}
static int GM_CDECL gmfRecordExistsOwner(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(type, 0);
	GM_CHECK_INT_PARAM(owner, 1);
	g_Blackboard.RecordExistsOwner(type, owner);
	return GM_OK;
}

static int GM_CDECL gmfRecordExistsTarget(gmThread *a_thread)
{
	GM_CHECK_NUM_PARAMS(2);
	GM_CHECK_INT_PARAM(type, 0);
	GM_CHECK_INT_PARAM(target, 1);
	g_Blackboard.RecordExistsTarget(type, target);
	return GM_OK;
}

static int GM_CDECL gmfPrint(gmThread *a_thread)
{
	GM_INT_PARAM(type, 0, bbk_All);
	g_Blackboard.DumpBlackBoardContentsToGame(type);
	return GM_OK;
}

void BlackBoard::Bind(gmMachine *a_machine)
{
	g_NextScriptItem = bbk_FirstScript;

	gmBind2::Global(a_machine,"Blackboard")
		.func(BlackBoard::MakeKey,"MakeKey"/*,"Creates a new unique key for blackboard data."*/)
		.func(gmfRecordExistsOwner,"RecordExistsOwner"/*,"Checks if a record of a certain type exists for a specific owner id."*/)
		.func(gmfRecordExistsTarget,"RecordExistsOwner"/*,"Checks if a record of a certain type exists for a specific target id."*/)
		.func(gmfPostRecord,"PostRecord"/*,"Post a record to the blackboard of a given type."*/)
		.func(gmfGetRecords,"GetRecords"/*,"Gets all the records of a certain type."*/)
		.func(gmfGetNumRecords,"GetNumRecords"/*,"Gets the number of records of a certain type on an optional target id."*/)
		.func(gmfRemoveByPoster,"RemoveByPoster"/*, "Remove records by a certain owner and optional message type"*/)
		.func(gmfRemoveByTarget,"RemoveByTarget"/*, "Remove records by a certain target and optional message type"*/)
		.func(gmfPrint,"PrintBlackboard"/*,"Prints all blackboard records, filtering by an optional type."*/)
		;
}
