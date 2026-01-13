/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GAME_PROJECTILE_H__
#define __GAME_PROJECTILE_H__

#include "physics/Physics_RigidBody.h"
#include "physics/Force_Constant.h"
#include "physics/Force_Field.h"
#include "Entity.h"

#ifdef _DENTONMOD
#include "tracer.h"
#ifndef _DENTONMOD_PROJECTILE_CPP
#define _DENTONMOD_PROJECTILE_CPP
#endif
#endif



/*
===============================================================================

  idProjectile

===============================================================================
*/

extern const idEventDef EV_Explode;

class idProjectile : public idEntity {
public :
	CLASS_PROTOTYPE( idProjectile );

							idProjectile();
	virtual					~idProjectile();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Create( idEntity *owner, const idVec3 &start, const idVec3 &dir, int hitCountGroupId = 0 );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	virtual void			FreeLightDef( void );

	idEntity *				GetOwner( void ) const;
#ifdef _D3XP
	virtual void			CatchProjectile( idEntity* o, const char* reflectName ); //ff1.3 - virtual addded
	int						GetProjectileState( void );
	void					Event_CreateProjectile( idEntity *owner, const idVec3 &start, const idVec3 &dir );
	void					Event_LaunchProjectile( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity );
	void					Event_SetGravity( float gravity );
#endif

	//ff start
	void					Event_SetOwner( idEntity *owner );
	void					Event_GetOwner( void );
	//ff end

	virtual void			Think( void );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void			Explode( const trace_t &collision, idEntity *ignore );
	virtual void			Fizzle( void ); //ff1.3 - virtual added

	static idVec3			GetVelocity( const idDict *projectile );
	static idVec3			GetGravity( const idDict *projectile );

	enum {
		EVENT_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	static void				DefaultDamageEffect( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity );
	static bool				ClientPredictionCollide( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity, bool addDamageEffect );
	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

#ifdef _DENTONMOD
	void					setTracerEffect( dnTracerEffect *effect) { tracerEffect = effect; }
#endif

	//ff1.3
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
	
	bool					IgnoresForceField( void ){ return projectileFlags.ignoreForceField; }
	bool					FiredByFriend( void ){ return projectileFlags.firedByFriend; }
	int						GetHitCountGroupId( void ){ return hitCountGroupId; }

protected:
	const idDeclEntityDef	*damageDef; // stores Damage Def -- By Clone JCD
	idEntityPtr<idEntity>	owner;

	struct projectileFlags_s {
		bool				detonate_on_world			: 1;
		bool				detonate_on_actor			: 1;
		bool				randomShaderSpin			: 1;
//		bool				isTracer					: 1;		
		bool				noSplashDamage				: 1;
		bool				impact_fx_played			: 1; // keeps track of fx played on collided body - BY JCD
		//ff1.3 start
		//bool				detonate_on_moveables		: 1; //works, but disabled atm
		//bool				detonate_on_ragdoll			: 1; //instead of a new key, just use detonate_on_actor also for ragdolls
		bool				ignoreForceField			: 1; 
		//bool				continuousSmoke				: 1;
		bool				firedByFriend				: 1; //true if fired by a friendly AI
		//bool				railgun						: 1; //railgun mode
		//ff1.3 end
	} projectileFlags;

#ifdef _DENTONMOD
	dnTracerEffect *tracerEffect;
#endif

	float					thrust;
	int						thrust_end;
	float					damagePower;

	renderLight_t			renderLight;
	qhandle_t				lightDefHandle;				// handle to renderer light def
	idVec3					lightOffset;
	int						lightStartTime;
	int						lightEndTime;
	idVec3					lightColor;

	idForce_Constant		thruster;
	idPhysics_RigidBody		physicsObj;

	const idDeclParticle *	smokeFly;
	int						smokeFlyTime;

#ifdef _D3XP
	int						originalTimeGroup;
#endif

	typedef enum {
		// must update these in script/doom_defs.script if changed
		SPAWNED = 0,
		CREATED = 1,
		LAUNCHED = 2,
		FIZZLED = 3,
		EXPLODED = 4
	} projectileState_t;

	projectileState_t		state;
	int						hitCountGroupId; //ff1.3

	void					AddDefaultDamageEffect( const trace_t &collision, const idVec3 &velocity );

private:
	bool					netSyncPhysics;

	void					Event_Explode( void );
	void					Event_Fizzle( void );
	void					Event_RadiusDamage( idEntity *ignore );
	//void					Event_Touch( idEntity *other, trace_t *trace ); //promoted to protected
	void					Event_GetProjectileState( void );
	
	//ff1.3 start
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					DropEntities( const trace_t &collision );
	//ff1.3 end
};

class idGuidedProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idGuidedProjectile );

							idGuidedProjectile( void );
							~idGuidedProjectile( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
#ifdef _D3XP
	void					SetEnemy( idEntity *ent );
	void					Event_SetEnemy(idEntity *ent);
#endif
	void					SetForceFieldVelocity( const idVec3 &velocity ); //ff1.3

protected:
	idAngles				angles;
	float					speed;
	idEntityPtr<idEntity>	enemy;
	virtual void			GetSeekPos( idVec3 &out );

private:
	idAngles				rndScale;
	idAngles				rndAng;
	//idAngles				angles; //ff1.3 - promoted to protected
	int						rndUpdateTime;
	float					turn_max;
	float					clamp_dist;
	bool					unGuided;
	bool					burstMode;
	float					burstDist;
	float					burstVelocity;
	//ff1.3 start
	idVec3					forceFieldVelocity;
	int						forceFieldVelocityTime;
	//ff1.3 end
};

//ff1.3 start
class idPossessionProjectile : public idGuidedProjectile {
public :
	CLASS_PROTOTYPE( idPossessionProjectile );

							idPossessionProjectile( void );
							~idPossessionProjectile( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	virtual void			Explode( const trace_t &collision, idEntity *ignore );

	void					Event_Remove( void );

protected:
	virtual void			GetSeekPos( idVec3 &out );

private:
	//bool					cameraActive;
	idCamera *				cameraView;
	idVec3					cameraOffset;
	renderEntity_t			secondModel;
	qhandle_t				secondModelDefHandle;

	idVec3					startingVelocity;
	idVec3					endingVelocity;
	float					accelTime;
	int						launchTime;
	
	void					EnableCamera( bool on );
	void					UpdateCamera( void );
	void					StartRide( idEntity* hitEnt );
};
//ff1.3 end

class idSoulCubeMissile : public idGuidedProjectile {
public:
	CLASS_PROTOTYPE ( idSoulCubeMissile );
	~idSoulCubeMissile();
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float power = 1.0f, const float dmgPower = 1.0f );

protected:
	virtual void			GetSeekPos( idVec3 &out );
	void					ReturnToOwner( void );
	void					KillTarget( const idVec3 &dir );

private:
	idVec3					startingVelocity;
	idVec3					endingVelocity;
	float					accelTime;
	int						launchTime;
	bool					killPhase;
	bool					returnPhase;
	idVec3					destOrg;
	idVec3					orbitOrg;
	int						orbitTime;
	int						smokeKillTime;
	const idDeclParticle *	smokeKill;
};

struct beamTarget_t {
	idEntityPtr<idEntity>	target;
	renderEntity_t			renderEntity;
	qhandle_t				modelDefHandle;
	bool					alreadyDamaged; //ff1.3
};

class idBFGProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idBFGProjectile );

							idBFGProjectile();
							~idBFGProjectile();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	virtual void			Explode( const trace_t &collision, idEntity *ignore );

//ff1.3 start
protected:
	float					damageRadius;
	
	virtual bool			UpdateBeamTarget( beamTarget_t &beamTarget, bool doDamage );
	virtual bool			AcceptBeamTarget(idEntity *ent, idVec3& damagePoint);
//ff1.3 end

private:
	idList<beamTarget_t>	beamTargets;
	renderEntity_t			secondModel;
	qhandle_t				secondModelDefHandle;
	int						nextDamageTime;
	idStr					damageFreq;
	//int						damageFreqMs;

	//ff1.3 start
	bool					actorDamaged;
	bool					mainDamageFirst;
	bool					ignoreDeadTargets;
	int						nextBeamTargetsSearchTime;
	int						ownerTeam;
	
	bool					IsBeamTarget(idEntity *ent);
	void					FindBeamTargets();
	//ff1.3 end

	void					FreeBeams();
	void					Event_RemoveBeams();
};

#define _FORCEPROJ

//ff1.3 start
#ifdef _FORCEPROJ
class idForcefieldProjectile : public idBFGProjectile {
public :
	CLASS_PROTOTYPE( idForcefieldProjectile );

							idForcefieldProjectile();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );

protected:
	virtual bool			UpdateBeamTarget( beamTarget_t &beamTarget, bool doDamage );
	virtual bool			AcceptBeamTarget(idEntity *ent, idVec3& damagePoint);

private:
	idStr					fxTeleport;				// fx system to start when entity is teleported
	float					teleportTolerance;		// target's radius multiplier (0..1)
	idForce_Field			forceField;
};
#endif


class idRailGunProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idRailGunProjectile );

							idRailGunProjectile();
							~idRailGunProjectile();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

protected:
	idVec3					launchPos;

	void					FireRailTracer( const idVec3 &startPos, const idVec3 &endPos, const idVec3 &velocity/*, const char * damageDefName */);
	
};


struct painkillerBeam_t {
	renderEntity_t			renderEntity;
	qhandle_t				modelDefHandle;
};

class idPainkillerProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idPainkillerProjectile );

							idPainkillerProjectile();
							~idPainkillerProjectile();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	virtual void			Explode( const trace_t &collision, idEntity *ignore );
	virtual void			Fizzle( void );

	void					StartReturnPhase( void );

private:
	float					speed;
	bool					returnPhase;
	bool					returned;
	int						nextDamageTime;
	int						smokeBeamTime;
	const idDeclParticle *	smokeDamage; 
	painkillerBeam_t		beam;
	const idDeclEntityDef *	damageBeamDef;

	void					GetSeekPos( idVec3 &out );
	void					Event_PushEntity( idEntity *ent, const idVec3 &hitPos );
	void					PushEntity( idEntity *ent, const idVec3 &hitPos );
	void					SchedulePush( idEntity* ent, idVec3 pos );
	bool					ShouldPush( idEntity* ent );
	bool					ShouldReturnOn( idEntity* ent );
	void					FreeBeam();
};

class idRemoteGrenadeProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idRemoteGrenadeProjectile );

							idRemoteGrenadeProjectile();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	virtual void			Explode( const trace_t &collision, idEntity *ignore );
	virtual void			CatchProjectile( idEntity* o, const char* reflectName );

private:
	int						allowPickupTime;
	int						allowGrabTime;
};


class idVagaryProjectile : public idGuidedProjectile {
public :
	CLASS_PROTOTYPE( idVagaryProjectile );

							idVagaryProjectile();

	virtual void			Think( void );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

private :
	void					PullTarget( const trace_t &collision );
	void					FreeBeam();
};

//ff1.3 end

/*
===============================================================================

  idDebris

===============================================================================
*/

class idDebris : public idEntity {
public :
	CLASS_PROTOTYPE( idDebris );

							idDebris();
							~idDebris();

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	void					Spawn( void );

	void					Create( idEntity *owner, const idVec3 &start, const idMat3 &axis );
	void					Launch( void );
	void					Think( void );
	void					Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void					Explode( void );
	void					Fizzle( void );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );


private:
	idEntityPtr<idEntity>	owner;
	idPhysics_RigidBody		physicsObj;
	const idDeclParticle *	smokeFly; 
	int						smokeFlyTime;
	int						nextSoundTime;		// BY Clone JCD 
	int						soundTimeDifference;	// BY Clone JCD 
	bool					continuousSmoke;		//By Clone JCD
	const idSoundShader *	sndBounce;
	const idSoundShader *	sndRest;
	void					Event_Explode( void );
	void					Event_Fizzle( void );
};

#endif /* !__GAME_PROJECTILE_H__ */
