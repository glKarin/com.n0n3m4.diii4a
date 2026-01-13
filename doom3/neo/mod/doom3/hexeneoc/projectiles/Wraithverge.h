#ifndef __AI_WRAITHVERGE_H__
#define __AI_WRAITHVERGE_H__

/*************************************************************************

WraithVerge Projectile (spawns other wraith types)

*************************************************************************/

class idProj_Wraith : public idProjectile {
public:
	CLASS_PROTOTYPE( idProj_Wraith );
	void				Think( void );
	void				Spawn( void );
	void				Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );

private:
	void				SpawnSubWraiths( int num );

private:
	bool			tome;
	bool			wraithSpun;
	idVec3			forw;
	float			defVel;
	float			dieTime;
	bool			wraithInitialized;
	float			spinTime;
	idAngles		ang;
};


class idProj_HomingWraith : public idProjectile {
public:
	CLASS_PROTOTYPE( idProj_HomingWraith );

private:
	void				Think( void );
	void				Explode( idEntity *ignore );
	void				Spawn( void );
	void				DirectDamage( const char *meleeDefName, idEntity *ent );

public:
	bool			ready;

private:
	void                findTarget();
	void                homingWraith();
	void                pullingWraith();

private:
	float	    	gravTime;
	idVec3          dir;
	float           defVel;
	int             dieTime;
	bool		    tome;
	idAI            *target;
	idVec3  		targOrigin;
	idVec3          targSize;
	float           nextSearch;
	float           nextWallCollideCheck;
	int             hits;
};

#endif // __AI_WRAITHVERGE_H__
