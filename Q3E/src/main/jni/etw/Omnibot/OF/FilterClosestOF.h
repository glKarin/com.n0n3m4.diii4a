////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __FilterClosestOF_H__
#define __FilterClosestOF_H__

#include "FilterClosestTF.h"

class FilterClosestOF : public FilterClosestTF
{
public:
	
	virtual bool CheckEx(const GameEntity _ent, const MemoryRecord &_record);

	FilterClosestOF(Client *_client, AiState::SensoryMemory::Type _type);
	virtual ~FilterClosestOF() {}
protected:

	FilterClosestOF();
};

#endif
