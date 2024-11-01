#include "PrecompCommon.h"
#include "MemoryRecord.h"

Vector3f MemoryRecord::PredictPosition(float _time) const
{
	float fTimeOutOfView = (float)GetTimeHasBeenOutOfView() / 1000.f;
	return Utils::PredictFuturePositionOfTarget(
		m_TargetInfo.m_LastPosition, m_TargetInfo.m_LastVelocity, fTimeOutOfView+_time);
}

bool MemoryRecord::IsMovingTowards(const Vector3f &_pos) const
{
	Vector3f vToTarget = _pos - GetLastSensedPosition();
	return GetLastSensedVelocity().Dot(vToTarget) > 0.f;
}

void MemoryRecord::Reset(GameEntity _ent)
{
	m_Entity = _ent;
	m_TargetInfo.m_EntityClass = 0;
	m_TargetInfo.m_EntityCategory.ClearAll();
	m_TimeLastSensed = -999;
	m_TimeBecameVisible = -999;
	m_IgnoreForTargeting = -999;
	m_TimeLastVisible = 0;
	m_TimeLastUpdated = 0;
	m_InFOV = false;
	m_IsAllied = false;
	m_IsShootable = false;
	m_IgnoreAsTarget = false;
	m_Serial++;
}
