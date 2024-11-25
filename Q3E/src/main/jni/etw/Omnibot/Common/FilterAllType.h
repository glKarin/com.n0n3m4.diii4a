#ifndef __FILTERALLTYPE_H__
#define __FILTERALLTYPE_H__

#include "FilterSensory.h"

// class: FilterAllType
//		This filter finds all the entities matching the 
//		requested type, category, and class
class FilterAllType : public FilterSensory
{
public:
	
	virtual void Check(int _index, const MemoryRecord &_record);

	inline MemoryRecords &GetList() { return m_List; }

	FilterAllType(Client *_client, AiState::SensoryMemory::Type _type, MemoryRecords &_list);
	virtual ~FilterAllType() {}
protected:
	MemoryRecords	&m_List;

	FilterAllType();
};

#endif
