#ifndef __THREATZONE_H__
#define __THREATZONE_H__

// class: ThreatZone
//		This class marks an area as dangerous.
class ThreatZone
{
public:

	static const obuint32 MAX_WEAPONS = 32;
	static const obuint32 MAX_CLASSES = 16;

	ThreatZone(obuint32 _navId, obint32 _weaponId, obint32 _classId);
	~ThreatZone();
private:
	ThreatZone();

	int		m_NavId;

	obuint16	m_WeaponKills[MAX_WEAPONS];
	obuint16	m_ClassKills[MAX_CLASSES];
};

// typedef: ThreatZonePtr
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<ThreatZone> ThreatZonePtr;
#else
typedef boost::shared_ptr<ThreatZone> ThreatZonePtr;
#endif

#endif
