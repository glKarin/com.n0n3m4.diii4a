#include "PrecompCommon.h"
#include "ThreatZone.h"

ThreatZone::ThreatZone(obuint32 _navId, obint32 _weaponId, obint32 _classId) :
	m_NavId		(_navId)
{
	memset(m_WeaponKills,0,sizeof(m_WeaponKills));
	memset(m_ClassKills,0,sizeof(m_ClassKills));
}

ThreatZone::~ThreatZone()
{
}
