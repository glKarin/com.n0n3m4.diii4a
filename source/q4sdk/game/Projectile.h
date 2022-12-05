// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#ifndef __GAME_PROJECTILE_H__
#define __GAME_PROJECTILE_H__

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

	void					Create( idEntity *owner, const idVec3 &start, const idVec3 &dir, idEntity* ignore = NULL, idEntity* extraPassEntity = NULL );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float dmgPower = 1.0f );

	virtual void			FreeLightDef( void );

//RITUAL BEGIN
	void					SetOwner(idEntity* ent)	{ owner = ent;	}
// RITUAL END

	idEntity *				GetOwner( void ) const;

	virtual void			Think( void );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );

	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity, bool &hitTeleporter );
	virtual void			Explode( const trace_t *collision, const bool showExplodeFX, idEntity *ignore = NULL, const char *sndExplode = "snd_explode" );
	void					Fizzle( void );

	static idVec3			GetVelocity( const idDict *projectile );
	static idVec3			GetGravity( const idDict *projectile );

	void					SetSpeed		( float s, int accelTime = 0 );
	float					GetSpeed		( void ) const;

	virtual void			UpdateVisualAngles();

	// information about what kind of projectile we are, used for death messages
	int						methodOfDeath;

	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

	virtual bool			ClientStale( void );
	
protected:
	void					SpawnImpactEntities(const trace_t& collision, const idVec3 projectileDirection);


	idEntityPtr<idEntity>	owner;

	struct projectileFlags_s {
		bool				detonate_on_world			: 1;
		bool				detonate_on_actor			: 1;
		bool				detonate_on_bounce			: 1;		// Detonate if hit a bounce surface
		bool				randomShaderSpin			: 1;
		bool				isTracer					: 1;
	} projectileFlags;

	float					damagePower;

	renderLight_t			renderLight;
	qhandle_t				lightDefHandle;				// handle to renderer light def
	idVec3					lightOffset;
	int						lightStartTime;
	int						lightEndTime;
	idVec3					lightColor;

	idEntity*				impactedEntity;

	rvPhysics_Particle		physicsObj;
	idAngles				visualAngles;
	idAngles				angularVelocity;
	idInterpolate<float>	speed;
	bool					updateVelocity;

	rvSphericalInterpolate	rotation;

	rvClientEffectPtr		flyEffect;
	float					flyEffectAttenuateSpeed;

	int						bounceCount;
	bool					sticky;
	
	idStr					impactEntity;
	int						numImpactEntities;
	int						ieMinPitch;
	int						ieMaxPitch;
	float					ieSlicePercentage;

// RAVEN BEGIN
// ddynerman: hit count for stats
	int						hitCount;
// ddynerman: pre-prediction ( rocket jumping )
	int						prePredictTime;
// RAVEN END
	typedef enum {
		SPAWNED = 0,
		CREATED = 1,
		LAUNCHED = 2,
		FIZZLED = 3,
		EXPLODED = 4
	} projectileState_t;
	
	projectileState_t		state;

	void					PlayPainEffect		( idEntity* ent, int damage, const rvDeclMatType* materialType, const idVec3& origin, const idVec3& direction );
	virtual void			PlayDetonateEffect	( const idVec3& origin, const idMat3& axis );

private:
	void					DefaultDamageEffect	( const trace_t &collision, const idVec3 &velocity, const char *damageDefName );

	void					Event_Explode			( void );
	void					Event_Fizzle			( void );
	void					Event_RadiusDamage		( idEntity *ignore );
	void					Event_ResidualDamage	( idEntity *ignore );
	void					Event_Touch				( idEntity *other, trace_t *trace );

	bool					syncPhysics;

	// cheap linear client side projectiles
	// transmitted in snapshot
	int						launchTime;
	idVec3					launchOrig;
	idVec3					launchDir;
	// set from def file in :Launch on both client and server
	float					launchSpeed;
};

ID_INLINE float idProjectile::GetSpeed ( void ) const {
	return speed.GetCurrentValue( gameLocal.time );
}

/*
===============================================================================

idGuidedProjectile

===============================================================================
*/

extern const idEventDef EV_UpdateGuideTarget;
extern const idEventDef EV_GuideToEntity;
extern const idEventDef EV_GuideToPos;

class idGuidedProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idGuidedProjectile );

							idGuidedProjectile( void );
							~idGuidedProjectile( void );

	enum {
		GUIDE_NONE,
		GUIDE_ENTITY,
		GUIDE_POS,
		GUIDE_DIR,
		GUIDE_MAX
	};

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float dmgPower = 1.0f );

	void					GuideTo			( const idVec3& post, const idVec3& dir );
	void					GuideTo			( const idVec3& pos );
	void					GuideTo			( idEntity* ent, jointHandle_t guideJoint=INVALID_JOINT, const idVec3 &offset=vec3_origin );
	void					CancelGuide		( void );

	int						GetGuideType	( void ) const;

	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

protected:

	int						guideType;	
	idEntityPtr<idEntity>	guideEnt;
	idVec3					guideDir;
	idVec3					guidePos;
	jointHandle_t			guideJoint;
	float					guideMinDist;
	
	int						driftTime;
	int						driftRate;
	float					driftRange;
	float					driftRadius;
	float					driftDiversity;
	float					driftAngle;
	float					driftAngleStep;
	float					driftProjectRange;
	
	virtual bool			GetGuideDir		( idVec3 &outDir, float& outDist );

private:

	idInterpolate<float>	turn_max;
	int						launchTime;
	int						guideDelay;
	int						driftDelay;
};

ID_INLINE int idGuidedProjectile::GetGuideType ( void ) const {
	return guideType;
}

ID_INLINE void idGuidedProjectile::GuideTo ( const idVec3& pos, const idVec3& dir ) {
	guideType = GUIDE_DIR;
	guidePos  = pos;
	guideDir  = dir;
}

ID_INLINE void idGuidedProjectile::GuideTo ( const idVec3& pos ) {
	guideType = GUIDE_POS;
	guidePos  = pos;
}

ID_INLINE void idGuidedProjectile::GuideTo ( idEntity* ent, jointHandle_t joint, const idVec3 &offset ) {
	guideType = GUIDE_ENTITY;
	guideEnt  = ent;
	guideJoint = joint;
	guidePos = offset;

	if ( guideEnt.IsValid() ) {
		guideEnt->GuidedProjectileIncoming( this );
	}
}

ID_INLINE void idGuidedProjectile::CancelGuide ( void ) {
	guideType = GUIDE_NONE;

	// twhitaker: TEMP
	if ( guideEnt.IsValid() ) {
		guideEnt->GuidedProjectileIncoming( NULL );	
	}
	// </twhitaker>
}

/*
===============================================================================

rvDriftingProjectile

===============================================================================
*/

class rvDriftingProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( rvDriftingProjectile );

							rvDriftingProjectile ( void );
							~rvDriftingProjectile ( void );

	void					Save			( idSaveGame *savefile ) const;
	void					Restore			( idRestoreGame *savefile );

	virtual void			Think			( void );
	virtual void			Launch			( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float dmgPower = 1.0f );

protected:

	virtual void			UpdateVisualAngles	( void );

	idVec3					startDir;
	idVec3					startOrigin;
	idMat3					startAxis;
	float					startSpeed;
	
	idInterpolateAccelDecelLinear<float>	driftOffset[2];
	idInterpolateAccelDecelLinear<float>	driftSpeed;
	float									driftOffsetMax;
	float									driftSpeedMax;
	float									driftTime;
};

/*
===============================================================================

rvSpawnerProjectile

===============================================================================
*/

class rvSpawner;
class rvSpawnerProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( rvSpawnerProjectile );
	
							rvSpawnerProjectile ( void );
							~rvSpawnerProjectile ( void );

	void					Spawn			( void );
	virtual void			Think			( void );

	void					SetSpawner		( rvSpawner* spawner );

protected:

	idEntityPtr<rvSpawner>	spawner;

	enum {
		STATE_NONE,
		STATE_ADDED,
	} spawnState;	
	
private:

	void					Event_PostSpawn	( void );
};

/*
===============================================================================

rvMIRVProjectile

===============================================================================
*/

class rvMIRVProjectile : public idProjectile {
	CLASS_PROTOTYPE( rvMIRVProjectile );

						rvMIRVProjectile ( void );
						~rvMIRVProjectile (  void );


	void				Spawn				( void );

private:
		
	void				Event_LaunchWarheads ( void );
};

#endif /* !__GAME_PROJECTILE_H__ */

// RAVEN END
