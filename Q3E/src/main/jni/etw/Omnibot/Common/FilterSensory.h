#ifndef __FILTERSENSORY_H__
#define __FILTERSENSORY_H__

class Client;
#include "BotSensoryMemory.h"

// class: FilterSensory
class FilterSensory
{
public:

	static int ANYPLAYERCLASS;

	enum SortType { Sort_None, Sort_NearToFar, Sort_FarToNear, };
	enum { MaxClassType = 8, MaxIgnoreEntity = 8, MaxPositions=8 };

	void AddClass(int _class);
	void AddCategory(int _category);
	void AddIgnoreEntity(GameEntity _ent);

	bool IsBeingIgnored(GameEntity _ent);

	void ResetClasses();
	void ResetCategory();
	void ResetIgnoreEntity();

	inline bool PassesFilter(int _class) const 
	{
		if(m_ClassType[0]==0)
			return true;

		for(int i = 0; i < MaxClassType; ++i)
		{
			if(m_ClassType[i] && m_ClassType[i] == _class) 
				return true; 
		}
		return false; 
	}

	inline void SetSortType(SortType _sort) { m_SortType = _sort; }

	void AddPosition(const Vector3f &_pos);
	void ClearPositions();
	const Vector3f &GetPosition(int i) const { return m_Position[i]; }
	int GetNumPositions() const { return m_NumPositions; }
	const Vector3f &GetTriggerPosition() const { return m_Position[m_ClosestPosition]; }
	
	obReal GetMaxDistance() const { return m_MaxDistance; }
	void SetMaxDistance(obReal _rad) { m_MaxDistance = _rad; }

	void MemorySpan(int _time) { m_MemorySpan = _time; }
	void MaxMemorySpan();

	virtual void Check(int _index, const MemoryRecord &_record) = 0;
	virtual bool CheckEx(const MemoryRecord &_record) { return true; }

	inline GameEntity GetBestEntity() const { return m_BestEntity; }

	virtual bool DetectedSomething() const { return GetBestEntity().IsValid(); }

	virtual void Reset() { m_BestEntity.Reset(); }

	virtual void PostQuery() {}

	FilterSensory(Client *_client, AiState::SensoryMemory::Type _type);
	virtual ~FilterSensory() {};
protected:
	Client								*m_Client;

	Vector3f							m_Position[MaxPositions];
	int									m_NumPositions;
	int									m_ClosestPosition;

	obReal								m_MaxDistance;

	AiState::SensoryMemory::Type		m_Type;

	int									m_MemorySpan;

	int									m_ClassType[MaxClassType];
	BitFlag32							m_Category;
	GameEntity							m_IgnoreEntity[MaxIgnoreEntity];
	
	GameEntity							m_BestEntity;

	SortType							m_SortType;
	bool								m_AnyPlayerClass : 1;

	FilterSensory();
private:
};

#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<FilterSensory> FilterPtr;
#else
typedef boost::shared_ptr<FilterSensory> FilterPtr;
#endif

#endif
