#ifndef __RTCW_FILTERCLOSEST_H__
#define __RTCW_FILTERCLOSEST_H__

#include "FilterClosest.h"
#include "BotSensoryMemory.h"

// class: ET_FilterClosest
//		This filter finds the closest entity matching the 
//		requested type, category, and class. Also provides
//		additional functionality required in ET to take into
//		account whether the entity is disguised or feigned dead
class RTCW_FilterClosest : public FilterClosest
{
public:
	
	virtual bool CheckEx(const MemoryRecord &_record);

	RTCW_FilterClosest(Client *_client, AiState::SensoryMemory::Type _type);
	virtual ~RTCW_FilterClosest() {}
protected:
	float	m_BestDistance;

	RTCW_FilterClosest();
};

#endif
