#ifndef __MEMORYRECORD_H__
#define __MEMORYRECORD_H__

#include "TargetInfo.h"

// class: MemoryRecord
//		Represents an element in a bots memory system.
class MemoryRecord
{
public:	
	friend class AiState::SensoryMemory;

	// var: m_TargetInfo
	//		Latest info about this target entity
	TargetInfo	m_TargetInfo;

	inline GameEntity GetEntity() const { return m_Entity; }

	inline int GetTimeLastSensed() const { return m_TimeLastSensed; }
	inline int GetTimeBecameVisible() const { return m_TimeBecameVisible; }
	inline int GetTimeLastVisible() const { return m_TimeLastVisible; }

	inline int GetTimeTargetHasBeenVisible() const { return IsInFOV() ? IGame::GetTime() - GetTimeBecameVisible() : 0; }
	inline int GetTimeHasBeenOutOfView() const { return IGame::GetTime() - GetTimeLastVisible(); }
	inline int GetIgnoreTargetTime() const { return m_IgnoreForTargeting; }

	inline void IgnoreAsTarget(bool _ignore) { m_IgnoreAsTarget = _ignore; } 
	inline void IgnoreAsTargetForTime(int _milliseconds) { m_IgnoreForTargeting = IGame::GetTime() + _milliseconds; }
	inline bool ShouldIgnore() const { return m_IgnoreAsTarget || m_IgnoreForTargeting > IGame::GetTime(); }

	inline const Vector3f &GetLastSensedPosition() const { return m_TargetInfo.m_LastPosition; }
	inline const Vector3f &GetLastSensedVelocity() const { return m_TargetInfo.m_LastVelocity; }
	inline bool IsInFOV() const { return m_InFOV; }
	inline bool IsAllied() const { return m_IsAllied; }
	inline bool IsShootable() const { return m_IsShootable; }

	Vector3f PredictPosition(float _time) const;

	bool IsMovingTowards(const Vector3f &_pos) const;

	int GetAge() const { return IGame::GetTime() - m_TimeLastUpdated; }
	void MarkUpdated() { m_TimeLastUpdated = IGame::GetTime(); }

	void Reset(GameEntity _ent = GameEntity());

	obint16 GetSerial() const { return m_Serial; }

	MemoryRecord()
		: m_Serial(0)
	{
		Reset();
	}
	~MemoryRecord() {};
private:
	GameEntity		m_Entity;

	// int: m_TimeLastSensed
	//		Records the time the entity was last sensed (seen or heard). 
	//		Used to determine if a bot can 'remember' this record or not.
	int				m_TimeLastSensed;

	// int: m_TimeBecameVisible
	//		How long the opponent has been visible. Useful for implementing reaction time.
	int				m_TimeBecameVisible;

	// int: m_TimeLastVisible
	//		The last time this opponent was seen. "Freshness" of this sensed entity
	int				m_TimeLastVisible;

	int				m_TimeLastUpdated;

	// int: m_IgnoreForTargeting
	//		The time until which the target should not be considered for targeting.
	int				m_IgnoreForTargeting;

	obint16			m_Serial;

	// bool: m_InFOV
	//		Marks whether this entity is current in this bots field of view.
	bool			m_InFOV : 1;

	// bool: m_IsShootable
	//		Marks whether this entity can be shot(has clear line of sight)
	bool			m_IsShootable : 1;

	// bool: m_IsAllied
	//		Marks whether this entity is an ally of this bot.
	bool			m_IsAllied : 1;

	// bool: m_IgnoreAsTarget
	//		Ignore this entity as a valid target.
	bool			m_IgnoreAsTarget : 1;
};

class RecordHandle
{
public:
	obint16 GetIndex() const { return udata.m_Short[0]; }
	obint16 GetSerial() const { return udata.m_Short[1]; }

	obint32 AsInt() const { return udata.m_Int; }
	void FromInt(obint32 _n) { udata.m_Int = _n; }

	void Reset() { *this = RecordHandle(); }

	bool IsValid() const { return udata.m_Short[0] >= 0; }

	explicit RecordHandle(obint16 _index, obint16 _serial)
	{
		udata.m_Short[0] = _index;
		udata.m_Short[1] = _serial;
	}
	RecordHandle()
	{
		udata.m_Short[0] = -1;
		udata.m_Short[1] = 0;
	}
private:
	union udatatype
	{
		obint32			m_Int;
		obint16			m_Short[2];
	} udata;
};

// typedef: MemoryRecords
//		This type is defined as a vector of MemoryRecords for various usages
typedef std::vector<RecordHandle> MemoryRecords;

#endif
