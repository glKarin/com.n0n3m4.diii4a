#ifndef __BLACKBOARD_H__
#define __BLACKBOARD_H__

#include "Omni-Bot_Events.h"

class bbItem;
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<bbItem> BBRecordPtr;
#else
typedef boost::shared_ptr<bbItem> BBRecordPtr;
#endif

// class: Blackboard
//		The blackboard acts as an area where various information can be 
//		posted and later retrieved. It is a high level storage area for all
//		kinds of information.
class BlackBoard
{
public:

	static int MakeKey();
	static BBRecordPtr AllocRecord(int _type);
	
	//////////////////////////////////////////////////////////////////////////

	bool PostBBRecord(BBRecordPtr _item);
	int GetBBRecords(int _type, BBRecordPtr *_items, int _maxitems);
	int GetNumBBRecords(int _type, int _target);

	bool RecordExistsOwner(int _type, int _owner);
	bool RecordExistsTarget(int _type, int _target);

	int RemoveBBRecordByPoster(int _poster, int _type = bbk_All);
	int RemoveBBRecordByTarget(int _target, int _type = bbk_All);
	int RemoveAllBBRecords(int _type = bbk_All);

	void PurgeExpiredRecords();
	void DumpBlackBoardContentsToGame(int _type = bbk_All);

	static void Bind(gmMachine *a_machine);

	BlackBoard();
	~BlackBoard();
protected:

	// typedef: BlackBoardDatabase
	//	std::multimap provides easy means to store information by 
	//	<BlackBoard_Key>. By using a multi-map it supports more than 
	//	one entry per key.
	typedef std::multimap<int, BBRecordPtr> BlackBoardDatabase;

	BlackBoardDatabase m_DB;
};

#endif
