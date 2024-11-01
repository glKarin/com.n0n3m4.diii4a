#ifndef __FILTERCLOSEST_H__
#define __FILTERCLOSEST_H__

#include "FilterSensory.h"

// class: FilterClosest
//		This filter finds the closest entity matching the 
//		requested type, category, and class
class FilterClosest : public FilterSensory
{
public:
	
	virtual void Check(int _index, const MemoryRecord &_record);
	
	virtual void Reset();

	inline float GetBestDistanceSq() { return m_BestDistanceSq; }

	FilterClosest(Client *_client, AiState::SensoryMemory::Type _type);
	virtual ~FilterClosest();
protected:
	float		m_BestDistanceSq;

	FilterClosest();
};

#endif
