
#ifndef __GAME_MISC_H__
#define __GAME_MISC_H__

/*
===============================================================================

idSpawnableEntity

A simple, spawnable entity with a model and no functionable ability of it's own.
For example, it can be used as a placeholder during development, for marking
locations on maps for script, or for simple placed models without any behavior
that can be bound to other entities.  Should not be subclassed.
===============================================================================
*/

class idSpawnableEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idSpawnableEntity );

	void				Spawn( void );

private:
};

/*
===============================================================================

  Potential spawning position for players.
  The first time a player enters the game, they will be at an 'initial' spot.
  Targets will be fired when someone spawns in on them.

  When triggered, will cause player to be teleported to spawn spot.

===============================================================================
*/

class idPlayerStart : public idEntity {
public:
	CLASS_PROTOTYPE( idPlayerStart );

	enum {
		EVENT_TELEPORTPLAYER = idEntity::EVENT_MAXEVENTS,
		EVENT_TELEPORTITEM,
		EVENT_MAXEVENTS
	};

						idPlayerStart( void );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual bool		ClientReceiveEvent( int event, int time, const idBitMsg &msg );

private:
	int					teleportStage;

	void				Event_TeleportEntity( idEntity *activator, bool predicted, idVec3& prevOrigin = vec3_origin );
	void				Event_Teleport( idEntity *activator );
	void				Teleport( idEntity* other );
	void				Event_TeleportStage( idPlayer *player );
	void				Event_ResetCamera( idPlayer *player );
	void				TeleportPlayer( idPlayer *player );
};


/*
===============================================================================

  Non-displayed entity used to activate triggers when it touches them.
  Bind to a mover to have the mover activate a trigger as it moves.
  When target by triggers, activating the trigger will toggle the
  activator on and off. Check "start_off" to have it spawn disabled.
	
===============================================================================
*/

class idActivator : public idEntity {
public:
	CLASS_PROTOTYPE( idActivator );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think( void );

private:
	bool				stay_on;

	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

  Path entities for monsters to follow.

===============================================================================
*/
class idPathCorner : public idEntity {
public:
	CLASS_PROTOTYPE( idPathCorner );

	void				Spawn( void );

	static void			DrawDebugInfo( void );

	static idPathCorner *RandomPath( const idEntity *source, const idEntity *ignore );

private:
	void				Event_RandomPath( void );
};

// RAVEN BEGIN
// bdube: jump points
/*
===============================================================================

  Debug Jump Point

===============================================================================
*/

class rvDebugJumpPoint : public idEntity {
public:

	CLASS_PROTOTYPE( rvDebugJumpPoint );

	void				Spawn();
};

/*
===============================================================================

  Object that fires targets and changes shader parms when damaged.

===============================================================================
*/

class idDamagable : public idEntity {
public:
	CLASS_PROTOTYPE( idDamagable );

						idDamagable( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	void				Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

// RAVEN BEGIN
// abahr:
	virtual void		Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );

	int					invincibleTime;
// RAVEN BEGIN
// abahr: changed to protected
protected:
	int					stage;
	int					stageNext;
	const idDict*		stageDict;
	int					stageEndTime;
	int					stageEndHealth;
	int					stageEndSpeed;
//jshepard: used to end a stage if a moveable is on the ground (for falling objects)
	bool				stageEndOnGround;
//jshepard: we want to activate certain objects when triggered-- falling blocks yes, barrels no.
	bool				activateStageOnTrigger;
		
	virtual void		ExecuteStage	( void );
	void				UpdateStage		( void );
	idVec3				GetStageVector	( const char* key, const char* defaultString = "" ) const;
	float				GetStageFloat	( const char* key, const char* defaultString = "" ) const;
	int					GetStageInt		( const char* key, const char* defaultString = "" ) const;
// RAVEN END

	int					count;
	int					nextTriggerTime;

	void				BecomeBroken( idEntity *activator );
	void				Event_BecomeBroken( idEntity *activator );
	void				Event_RestoreDamagable( void );
};

/*
===============================================================================

  Hidden object that explodes when activated

===============================================================================
*/

class idExplodable : public idEntity {
public:
	CLASS_PROTOTYPE( idExplodable );

	void				Spawn( void );

private:
	void				Event_Explode( idEntity *activator );
};


/*
===============================================================================

  idSpring

===============================================================================
*/

class idSpring : public idEntity {
public:
	CLASS_PROTOTYPE( idSpring );

	void				Spawn( void );
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think( void );

private:
	idEntityPtr<idEntity>	ent1;
	idEntityPtr<idEntity>	ent2;
	int						id1;
	int						id2;
	idVec3					p1;
	idVec3					p2;
	idForce_Spring			spring;

	void				Event_LinkSpring( void );
};


/*
===============================================================================

  idForceField

===============================================================================
*/

class idForceField : public idEntity {
public:
	CLASS_PROTOTYPE( idForceField );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );

	virtual void		Think( void );

// RAVEN BEGIN
// kfuller: idDamagable may want to change some things on the fly
	void				SetExplosion(float force) { forceField.Explosion(force); }
// RAVEN END


// RAVEN BEGIN
// bdube: made force field protected
protected:

	idForce_Field		forceField;
	
private:
// RAVEN END
	void				Toggle( void );

	void				Event_Activate( idEntity *activator );
	void				Event_Toggle( void );
	void				Event_FindTargets( void );
};

// RAVEN BEGIN
// bdube: jump pads
/*
===============================================================================

  rvJumpPad

===============================================================================
*/

class rvJumpPad : public idForceField {
public:
	CLASS_PROTOTYPE( rvJumpPad );

	rvJumpPad ( void );

	void				Spawn( void );
	void				Think( void );

private:

	int					lastEffectTime;

	void				Event_FindTargets( void );

	enum {
		EVENT_JUMPFX = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};
	bool				ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	idMat3				effectAxis;
};
// RAVEN END

/*
===============================================================================

  idAnimated

===============================================================================
*/

class idAnimated : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idAnimated );

							idAnimated();
							~idAnimated();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual bool			LoadAF( const char* keyname );
	bool					StartRagdoll( void );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

// RAVEN BEGIN
// bdube: script 
	void					Think ( void );
	
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
 	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
// RAVEN END

private:
	int						num_anims;
	int						current_anim_index;
	int						anim;
	int						blendFrames;
	jointHandle_t			soundJoint;
	idEntityPtr<idEntity>	activator;
	bool					activated;

// RAVEN BEGIN
// bdube: script variables
	// script control
	idThread *				scriptThread;
	idStr					state;
	idStr					idealState;
	int						animDoneTime[ANIM_NumAnimChannels];

	// Script state management
	void					UpdateScript( void );
	void					SetState( const char *statename, int blend );
	void					CallHandler ( const char* handler );
// RAVEN END

	void					PlayNextAnim( void );

	void					Event_Activate( idEntity *activator );	
	void					Event_Start( void );
	void					Event_StartRagdoll( void );
	void					Event_AnimDone( int animIndex );
	void					Event_Footstep( void );
	void					Event_LaunchMissiles( const char *projectilename, const char *sound, const char *launchjoint, const char *targetjoint, int numshots, int framedelay );
	void					Event_LaunchMissilesUpdate( int launchjoint, int targetjoint, int numshots, int framedelay );

// RAVEN BEGIN
// kfuller: added
	void					Event_SetAnimState( const char* state, int blendframes );
	void					Event_PlayAnim( int channel, const char *animname );
	void					Event_PlayCycle( int channel, const char *animname );
	void					Event_AnimDone2( int channel, int blendFrames );
// RAVEN END	
};

/*
===============================================================================

  idStaticEntity

===============================================================================
*/

class idStaticEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idStaticEntity );

						idStaticEntity( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	void				ShowEditingDialog( void );
	virtual void		Hide( void );
	virtual void		Show( void );
	void				Fade( const idVec4 &to, float fadeTime );
	virtual void		Think( void );

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	void				Event_Activate( idEntity *activator );

	int					spawnTime;
	bool				active;
	idVec4				fadeFrom;
	idVec4				fadeTo;
	int					fadeStart;
	int					fadeEnd;
	bool				runGui;
};


/*
===============================================================================

idFuncEmitter

===============================================================================
*/

class idFuncEmitter : public idStaticEntity {
public:
	CLASS_PROTOTYPE( idFuncEmitter );

						idFuncEmitter( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Spawn( void );
	void				Event_Activate( idEntity *activator );

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	bool				hidden;

};


// RAVEN BEGIN
// bdube: not using
#if 0
// RAVEN END

/*
===============================================================================

idFuncSmoke

===============================================================================
*/

class idFuncSmoke : public idEntity {
public:
	CLASS_PROTOTYPE( idFuncSmoke );

							idFuncSmoke();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );
	void					Event_Activate( idEntity *activator );

private:
	int						smokeTime;
	const idDeclParticle *	smoke;
	bool					restart;
};

// RAVEN BEGIN
// bdube: not using
#endif
// RAVEN END

/*
===============================================================================

idFuncSplat

===============================================================================
*/

class idFuncSplat : public idFuncEmitter {
public:
	CLASS_PROTOTYPE( idFuncSplat );

	idFuncSplat( void );

	void				Spawn( void );

private:
	void				Event_Activate( idEntity *activator );
	void				Event_Splat();
};


/*
===============================================================================

idTextEntity

===============================================================================
*/

class idTextEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idTextEntity );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think( void );
	virtual void		ClientPredictionThink( void );

private:
	idStr				text;
	bool				playerOriented;
};


/*
===============================================================================

idLocationEntity

===============================================================================
*/

class idLocationEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idLocationEntity );

	void				Spawn( void );

	const char *		GetLocation( void ) const;

private:
};

class idLocationSeparatorEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idLocationSeparatorEntity );

	void				Spawn( void );

private:
};

class idVacuumSeparatorEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idVacuumSeparatorEntity );

						idVacuumSeparatorEntity( void );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Event_Activate( idEntity *activator );	

private:
	qhandle_t			portal;
};

class idVacuumEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idVacuumEntity );

	void				Spawn( void );

private:
};

// RAVEN BEGIN
// abahr
class rvGravitySeparatorEntity : public idEntity {
public:
	CLASS_PROTOTYPE( rvGravitySeparatorEntity );

						rvGravitySeparatorEntity( void );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				Event_Activate( idEntity *activator );	

private:
	qhandle_t			portal;
};

class rvGravityArea : public idEntity {
public:
	ABSTRACT_PROTOTYPE( rvGravityArea );

	void					Spawn( void );

	virtual int				GetArea() const { return area; }
	virtual const idVec3	GetGravity( const idVec3& origin, const idMat3& axis, int clipMask, idEntity* passEntity ) const = 0;
	virtual const idVec3	GetGravity( const idEntity* ent ) const = 0;
	virtual const idVec3	GetGravity( const rvClientEntity* ent ) const = 0;

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	bool					IsEqualTo( const rvGravityArea* area ) const;
	bool					operator==( const rvGravityArea* area ) const;
	bool					operator==( const rvGravityArea& area ) const;
	bool					operator!=( const rvGravityArea* area ) const;
	bool					operator!=( const rvGravityArea& area ) const;

protected:
	int						area;
};

class rvGravityArea_Static : public rvGravityArea {
public:
	CLASS_PROTOTYPE( rvGravityArea_Static );

	void					Spawn( void );

	virtual const idVec3	GetGravity( const idVec3& origin, const idMat3& axis, int clipMask, idEntity* passEntity ) const { return gravity; }
	virtual const idVec3	GetGravity( const idEntity* ent ) const { return gravity; }
	virtual const idVec3	GetGravity( const rvClientEntity* ent ) const { return gravity; }

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

protected:
	idVec3					gravity;
};

class rvGravityArea_SurfaceNormal : public rvGravityArea {
public:
	CLASS_PROTOTYPE( rvGravityArea_SurfaceNormal );

	virtual const idVec3	GetGravity( const idVec3& origin, const idMat3& axis, int clipMask, idEntity* passEntity ) const;
	virtual const idVec3	GetGravity( const idEntity* ent ) const;
	virtual const idVec3	GetGravity( const rvClientEntity* ent ) const;

protected:
	virtual const idVec3	GetGravity( const idPhysics* physics ) const;
};
// RAVEN END

/*
===============================================================================

  idBeam

===============================================================================
*/

class idBeam : public idEntity {
public:
	CLASS_PROTOTYPE( idBeam );

						idBeam();

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think( void );

	void				SetMaster( idBeam *masterbeam );
	void				SetBeamTarget( const idVec3 &origin );

	virtual void		Show( void );

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg );

private:
	void				Event_MatchTarget( void );
	void				Event_Activate( idEntity *activator );

	idEntityPtr<idBeam>	target;
	idEntityPtr<idBeam>	master;
};


/*
===============================================================================

  idLiquid

===============================================================================
*/

class idRenderModelLiquid;

class idLiquid : public idEntity {
public:
	CLASS_PROTOTYPE( idLiquid );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

private:
	void				Event_Touch( idEntity *other, trace_t *trace );


	idRenderModelLiquid *model;
};


/*
===============================================================================

  idShaking

===============================================================================
*/

class idShaking : public idEntity {
public:
	CLASS_PROTOTYPE( idShaking );

							idShaking();
							~idShaking();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

private:
	idPhysics_Parametric	physicsObj;
	bool					active;

	void					BeginShaking( void );
	void					Event_Activate( idEntity *activator );
};


/*
===============================================================================

  idEarthQuake

===============================================================================
*/

class idEarthQuake : public idEntity {
public:
	CLASS_PROTOTYPE( idEarthQuake );
			
						idEarthQuake();

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think( void );

// RAVEN BEGIN
// kfuller: look for fx entities and the like that may want to be triggered when a mortar round (aka earthquake) goes off
protected:
	void				AffectNearbyEntities(float affectRadius);
// RAVEN END

private:
	int					nextTriggerTime;
	int					shakeStopTime;
	float				wait;
	float				random;
	bool				triggered;
	bool				playerOriented;
	bool				disabled;
	float				shakeTime;

	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

  idFuncPortal

===============================================================================
*/

class idFuncPortal : public idEntity {
public:
	CLASS_PROTOTYPE( idFuncPortal );
			
						idFuncPortal();

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

private:
	qhandle_t			portal;
	bool				state;

	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

  idFuncAASPortal

===============================================================================
*/

class idFuncAASPortal : public idEntity {
public:
	CLASS_PROTOTYPE( idFuncAASPortal );
			
						idFuncAASPortal();

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

private:
	bool				state;

	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

  idFuncAASObstacle

===============================================================================
*/

class idFuncAASObstacle : public idEntity {
public:
	CLASS_PROTOTYPE( idFuncAASObstacle );
			
						idFuncAASObstacle();

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	void				SetState ( bool _state );

private:
	bool				state;

	void				Event_Activate( idEntity *activator );
};


/*
===============================================================================

idFuncRadioChatter

===============================================================================
*/

class idFuncRadioChatter : public idEntity {
public:
	CLASS_PROTOTYPE( idFuncRadioChatter );

						idFuncRadioChatter();

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	static void			RepeatLast ( void );

private:
	static idEntityPtr<idFuncRadioChatter> lastRadioChatter;
	float				time;
	bool				isActive;
	void				Event_Activate( idEntity *activator );
	void				Event_ResetRadioHud( idEntity *activator );
	void				Event_IsActive( void );
};


/*
===============================================================================

  idPhantomObjects

===============================================================================
*/

class idPhantomObjects : public idEntity {
public:
	CLASS_PROTOTYPE( idPhantomObjects );
			
						idPhantomObjects();

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual void		Think( void );

private:
	void				Event_Activate( idEntity *activator );
	void				Event_Throw( void );
	void				Event_ShakeObject( idEntity *object, int starttime );

	int					end_time;
	float				throw_time;
	float				shake_time;
	idVec3				shake_ang;
	float				speed;
	int					min_wait;
	int					max_wait;
	idEntityPtr<idActor>target;
	idList<int>			targetTime;
	idList<idVec3>		lastTargetPos;
};


/*
===============================================================================

rvFuncSaveGame

===============================================================================
*/

class rvFuncSaveGame : public idEntity {
public:
	CLASS_PROTOTYPE( rvFuncSaveGame );

	void				Spawn( void );

	void				Event_Activate		( idEntity *activator );

private:
};


#endif /* !__GAME_MISC_H__ */
