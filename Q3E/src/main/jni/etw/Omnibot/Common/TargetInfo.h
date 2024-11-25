#ifndef __TARGETINFO_H__
#define __TARGETINFO_H__

class gmUserObject;

// class: TargetInfo
//		Holds the last known information regarding this target.
class TargetInfo
{
public:
	// float: m_DistanceTo
	//		Distance to this target
	float		m_DistanceTo;

	// int: m_EntityClass
	//		The Classification for this entity
	int			m_EntityClass;

	// int: m_CurrentWeapon
	//		The currently equipped weapon
	int			m_CurrentWeapon;

	// BitFlag64: m_EntityFlags
	//		Bit flags for this entity representing extra info
	BitFlag64	m_EntityFlags;

	// BitFlag64: m_EntityPowerups
	//		Current power-ups of this entity, see <Powerups>
	BitFlag64	m_EntityPowerups;

	// int: m_EntityCategory
	//		The category of entities this belongs to.
	BitFlag32	m_EntityCategory;

	// var: m_LastPosition
	//		The last position observed.
	Vector3f	m_LastPosition;

	// var: m_LastVelocity
	//		The last velocity observed.
	Vector3f	m_LastVelocity;

	// var: m_LastFacing
	//		The last facing direction observed.
	Vector3f	m_LastFacing;

	// function: IsA
	//		Quick easy access to check if this target matches a
	//		specific entity class
	bool IsA(int _class) { return (m_EntityClass == _class); }

	bool Update();

	gmUserObject *GetScriptObject(gmMachine *_machine) const;

	TargetInfo();
	~TargetInfo();
private:

	// var: m_ScriptObject
	//		This objects script instance, so that the object can clear its script
	//		references when deleted.
	mutable gmGCRoot<gmUserObject> m_ScriptObject;
};

#endif
