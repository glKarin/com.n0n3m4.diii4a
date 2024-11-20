////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __JA_FILTERCLOSEST_H__
#define __JA_FILTERCLOSEST_H__

#include "FilterClosest.h"
#include "BotSensoryMemory.h"

// class: JA_FilterClosest
//		This filter finds the closest entity matching the 
//		requested type, category, and class. Also provides
//		additional functionality required in JA to take into
//		account whether the entity is disguised or feigned dead
class JA_FilterClosest : public FilterClosest
{
public:
	
	virtual bool CheckEx(const GameEntity _ent, const MemoryRecord &_record);

	JA_FilterClosest(Client *_client, AiState::SensoryMemory::Type _type);
	virtual ~JA_FilterClosest() {}
protected:
	float	m_BestDistance;

	JA_FilterClosest();
};

#endif
