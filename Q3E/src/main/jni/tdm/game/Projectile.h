/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __GAME_PROJECTILE_H__
#define __GAME_PROJECTILE_H__

#include "PickableLock.h"
#include "Inventory/InventoryItem.h"
#include "Inventory/Category.h"

/**
* SFinalProjData: Structure storing the final projectile data at impact
* Passed on to CProjectileResult object
**/
typedef struct SFinalProjData_s
{
	/**
	 * greebo: The entity which fired the projectile
	 */
	idEntityPtr<idEntity> Owner;

	/**
	* Final world position of the origin of the projectile on impact
	**/
	idVec3	FinalOrigin;

	/**
	* Final orientation of the projectile on impact
	**/
	idMat3	FinalAxis;

	/**
	* Final linear velocity of the projectile just before impact
	**/
	idVec3	LinVelocity;

	/**
	* Final angular velocity of the projectile just before impact
	**/
	idVec3	AngVelocity;

	/**
	* Direction vector for the axis of the arrow.  Needed for pushing it in.
	**/
	idVec3	AxialDir;

	/**
	* Max Angle of incidence when projectile hits surface
	* (Calculated in CProjectileResult::Init )
	**/
	float IncidenceAngle;

	/**
	* Mass of the projectile (might be useful)
	**/
	float mass;

	/**
	* Name of the surface that was struck
	**/
	idStr	SurfaceType;

} SFinalProjData;

/*
===============================================================================

  idProjectile
	
===============================================================================
*/

extern const idEventDef EV_Explode;
// greebo: Exposed projectile launch method to scripts
extern const idEventDef EV_Launch;

class idProjectile : public idEntity {
public :
	CLASS_PROTOTYPE( idProjectile );

							idProjectile();
	virtual					~idProjectile() override;

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Create( idEntity *owner, const idVec3 &start, const idVec3 &dir );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	virtual void			FreeLightDef( void ) override;

	idEntity *				GetOwner( void ) const;
	void					SetReplaced(); // grayman #2908

	virtual void			Think( void ) override;
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity ) override;
	virtual void			Explode( const trace_t &collision, idEntity *ignore );
	virtual void			Bounced( const trace_t &collision, const idVec3 &velocity, idEntity *bounceEnt );
	void					Fizzle( void );

	static idVec3			GetVelocity( const idDict *projectile );
	static idVec3			GetGravity( const idDict *projectile );

	enum {
		EVENT_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	static void				DefaultDamageEffect( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity );
	static bool				ClientPredictionCollide( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity, bool addDamageEffect );
	virtual void			ClientPredictionThink( void ) override;
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg ) override;
	void					MineExplode( int entityNumber ); // grayman #2478
	bool					IsMine();	// grayman #2478
	bool					IsArmed();	// grayman #2906
	bool					DetonateOnWater(); // grayman #1104
	void					SetNoSplashDamage(bool setting); // grayman #1104
	virtual void			AttackAction(idPlayer* player) override; // grayman #2934
	void					Event_ActivateProjectile();

protected:
	idEntityPtr<idEntity>	owner;

	struct projectileFlags_s {
		bool				detonate_on_world			: 1;
		bool				detonate_on_actor			: 1;
		bool				randomShaderSpin			: 1;
		bool				isTracer					: 1;
		bool				noSplashDamage				: 1;
		bool				detonate_on_water			: 1; // grayman #1104
	} projectileFlags;

	float					thrust;
	int						thrust_end;
	float					damagePower;
	int						nextDamageTime;

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

	typedef enum {
		// must update these in script/tdm_defs.script if changed
		SPAWNED = 0,
		CREATED = 1,
		LAUNCHED = 2,
		FIZZLED = 3,
		EXPLODED = 4,
		INACTIVE = 5	// greebo: this applies to mines that haven't become active yet
	} projectileState_t;
	
	projectileState_t		state;
	
	PickableLock*			m_Lock;		// grayman #2478 - A lock implementation for this mover
	bool					isMine;		// grayman #2478 - true if this is a mine
	bool					replaced;	// grayman #2908 - true if this is a projectile mine that replaced a map author-placed armed mine
	bool					hasBounced;	// whether the projectile has bounced at least once already

protected:
	/**
	* Determine whether the projectile "activates" based on the surface type argument
	* This checks a space-delimited spawnarg list of activating materials and
	* returns true if the struck material activates this projectile
	**/
	bool TestActivated( const char *typeName );

private:
	void					AddDefaultDamageEffect( const trace_t &collision, const idVec3 &velocity );

	virtual void			AddObjectsToSaveGame(idSaveGame* savefile) override; // grayman #2478
	bool					CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) override; // grayman #2478
	bool					UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item) override; // grayman #2478
	bool					IsLocked(); // grayman #2478
	void					Event_ClearPlayerImmobilization(idEntity* player); // grayman #2478

	void					Event_Explode( void );
	void					Event_Fizzle( void );
	void					Event_RadiusDamage( idEntity *ignore );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Event_GetProjectileState( void );
	void					Event_Launch( idVec3 const &origin, idVec3 const &direction, idVec3 const &velocity );
	void					Event_Lock_OnLockPicked();	// grayman #2478
	void					Event_Mine_Replace();		// grayman #2478
	float					AngleAdjust(float angle);	// grayman #2478
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) override; // grayman #2478
};

class idGuidedProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idGuidedProjectile );

							idGuidedProjectile( void );
	virtual					~idGuidedProjectile( void ) override;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void ) override;
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f ) override;

protected:
	float					speed;
	idEntityPtr<idEntity>	enemy;
	virtual void			GetSeekPos( idVec3 &out );

private:
	void					Event_Launch( idVec3 const &origin, idVec3 const &direction, idVec3 const &velocity, idEntity *target );

	idAngles				rndScale;
	idAngles				rndAng;
	idAngles				angles;
	int						rndUpdateTime;
	float					turn_max;
	float					clamp_dist;
	bool					burstMode;
	bool					unGuided;
	float					burstDist;
	float					burstVelocity;
};


/*
===============================================================================

  idDebris
	
===============================================================================
*/

class idDebris : public idEntity {
public :
	CLASS_PROTOTYPE( idDebris );

							idDebris();
	virtual					~idDebris() override;

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	void					Spawn( void );

	void					Create( idEntity *owner, const idVec3 &start, const idMat3 &axis );
	void					Launch( void );
	virtual void			Think( void ) override;
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	void					Explode( void );
	void					Fizzle( void );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity ) override;


private:
	idEntityPtr<idEntity>	owner;
	idPhysics_RigidBody		physicsObj;
	const idDeclParticle *	smokeFly;
	int						smokeFlyTime;
	const idSoundShader *	sndBounce;

	void					Event_Explode( void );
	void					Event_Fizzle( void );
};

#endif /* !__GAME_PROJECTILE_H__ */
