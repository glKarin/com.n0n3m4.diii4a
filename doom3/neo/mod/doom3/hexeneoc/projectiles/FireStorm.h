#ifndef __AI_FIRESTORM_H__
#define __AI_FIRESTORM_H__

// one flame to go on one joint. idProj_Incinerator creates these entities.
class idProj_IncineratorFlame : public idEntity {
public:
	CLASS_PROTOTYPE( idProj_IncineratorFlame );
	void				Spawn( void );
};

// base class for projetiles that incinerate things
class idProj_Incinerator : public idProjectile {
public:
	CLASS_PROTOTYPE( idProj_Incinerator );
	void				Spawn( void );
	void				Incinerate( void );
	bool				Collide( const trace_t &collision, const idVec3 &velocity );

private:
	bool				incinerate;
	idEntity *			ent;
};

class idProj_FireBeam : public idProj_Incinerator {
public:
	CLASS_PROTOTYPE( idProj_FireBeam );
	void				Think( void );
	void				Spawn( void );
	void				Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

private:
	float				gravTime;

private:
	int					meteorNum;

private:
	void				Event_Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire, const float launchPower, const float dmgPower );
};

#endif // __AI_FIRESTORM_H__
