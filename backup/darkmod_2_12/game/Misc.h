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
		EVENT_MAXEVENTS
	};

						idPlayerStart( void );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	virtual bool		ClientReceiveEvent( int event, int time, const idBitMsg &msg ) override;

private:
	int					teleportStage;

	void				Event_TeleportPlayer( idEntity *activator );
	void				Event_TeleportStage( idEntity *player );
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

	virtual void		Think( void ) override;

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

	static idPathCorner *RandomPath( const idEntity *source, const idEntity *ignore, idAI* owner );

private:
	void				Event_RandomPath( void );
};

/*
===============================================================================

  Path entities for AI to flee to.

===============================================================================
*/
class tdmPathFlee : public idEntity {
public:
	CLASS_PROTOTYPE( tdmPathFlee );

	virtual ~tdmPathFlee() override;

	void				Spawn( void );

	static void		DrawDebugInfo( void );
};

/*
===============================================================================

  Path entities for AI to guard during a search.

===============================================================================
*/
class tdmPathGuard : public idEntity
{
public:
	CLASS_PROTOTYPE( tdmPathGuard );

	virtual			~tdmPathGuard() override;

	int				m_priority; // 1 = lowest
	float			m_angle;	// yaw to face

	void			Spawn( void );

	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	static void		DrawDebugInfo( void );
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
	virtual void		Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;

private:
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

	virtual void		Think( void ) override;

private:
	idEntity *			ent1;
	idEntity *			ent2;
	int					id1;
	int					id2;
	idVec3				p1;
	idVec3				p2;
	idForce_Spring		spring;

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

	virtual void		Think( void ) override;

private:
	idForce_Field		forceField;

	void				Toggle( void );

	void				Event_Activate( idEntity *activator );
	void				Event_Toggle( void );
	void				Event_FindTargets( void );
};


/*
===============================================================================

  idAnimated

===============================================================================
*/

class idAnimated : public idAFEntity_Gibbable {
public:
	CLASS_PROTOTYPE( idAnimated );

	idAnimated();
	virtual ~idAnimated();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual bool			LoadAF( void ) override;
	bool					StartRagdoll( void );
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) override;

	virtual void			Think( void ) override;								//	\ SteveL #3770: to enable LOD
	virtual void			SwapLODModel( const char *modelname ) override;		//	/	
	
private:
	int						num_anims;
	int						current_anim_index;
	int						anim;
	int						blendFrames;
	jointHandle_t			soundJoint;
	idEntityPtr<idEntity>	activator;
	bool					activated;

	void					PlayNextAnim( void );

	void					Event_Activate( idEntity *activator );	
	void					Event_Start( void );
	void					Event_StartRagdoll( void );
	void					Event_AnimDone( int animIndex );
	void					Event_Footstep( void );
	void					Event_LaunchMissiles( const char *projectilename, const char *sound, const char *launchjoint, const char *targetjoint, int numshots, int framedelay );
	void					Event_LaunchMissilesUpdate( int launchjoint, int targetjoint, int numshots, int framedelay );
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
	virtual void		ShowEditingDialog( void ) override;
	virtual void		Hide( void ) override;
	virtual void		Show( void ) override;
	void				Fade( const idVec4 &to, float fadeTime );
	virtual void		Think( void ) override;

	virtual void		ReapplyDecals() override; // #3817

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg ) override;

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

	virtual void			Think( void ) override;
	void					Event_Activate( idEntity *activator );
	void					Event_SetSmoke( const char *particleDef );

private:
	int						smokeTime;
	const idDeclParticle *	smoke;
	bool					restart;
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

	virtual void		Think( void ) override;

private:
	idStr				text;
	bool				playerOriented;
	bool				force; // grayman #3042
};


/*
===============================================================================

idLocationEntity

===============================================================================
*/

class idLocationEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idLocationEntity );

	idLocationEntity( void );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	const char *		GetLocation( void ) const;

public:
	/**
	* Soundprop: Loss multiplier for atmospheric attenuation
	**/
	float				m_SndLossMult;
	/**
	* Soundprop: Volume offset for sounds originating in location
	**/
	float				m_SndVolMod;
	/**
	* Objective system: Location's objective group name for objective checks
	**/
	idStr				m_ObjectiveGroup;

private:
};

/*
===============================================================================

idPortalEntity

===============================================================================
*/

class idPortalEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idPortalEntity );
	idPortalEntity();
	virtual ~idPortalEntity() override;

	static idBounds		GetBounds( const idVec3 &origin );

	void				Spawn( void );

	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );

	qhandle_t			GetPortalHandle( void ) const;

	// Returns a factor (0..1.0) that says how  much light the portal lets through
	float				GetLightLoss( void ) const;
	float				GetSoundLoss( void ) const; // grayman #3042
	void				SetSoundLoss( const float loss ); // grayman #3042
	void				Event_GetPortalHandle( void );
	void				Event_GetSoundLoss( void ); // grayman #3042
	void				Event_SetSoundLoss( const float loss ); // grayman #3042
	/**
	* grayman #3042 - The post-spawn event looks for touching doors and brittle fractures
	* that might not be available at spawn time.
	**/
	void				Event_PostSpawn( void );

public:

	/**
	* Soundprop: Volume loss for sounds traveling through this portal, in
	* addition to a potential door or brittle fracture on this portal.
	**/
	float				m_SoundLoss;

	/**
	* Tels: Handle of the portal this entity touches.
	**/
	qhandle_t			m_Portal;

private:
	/**
	* grayman #3042 - Save a pointer to a door or brittle fracture that shares the portal with this portal entity
	**/
	idEntity*			m_Entity;
	bool				m_EntityLocationDone;
};

class idLocationSeparatorEntity : public idPortalEntity
{
public:
	CLASS_PROTOTYPE( idLocationSeparatorEntity );

	void				Spawn( void );

public:
};

// grayman #3042 - entity to provide settings on a portal w/o splitting locations

class idPortalSettingsEntity : public idPortalEntity
{
public:
	CLASS_PROTOTYPE( idPortalSettingsEntity );

	void				Spawn( void );
};

/*
===============================================================================

idVacuumSeparatorEntity

===============================================================================
*/

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

	virtual void		Think( void ) override;

	void				SetMaster( idBeam *masterbeam );
	void				SetBeamTarget( const idVec3 &origin );

	virtual void		Show( void ) override;

	virtual void		WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void		ReadFromSnapshot( const idBitMsgDelta &msg ) override;

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

#ifndef MOD_WATERPHYSICS
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
#endif

/*
===============================================================================

  idShaking

===============================================================================
*/

class idShaking : public idEntity {
public:
	CLASS_PROTOTYPE( idShaking );

							idShaking();

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

	virtual void		Think( void ) override;

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
	virtual void		Think( void ) override;

	void				OpenPortal( void );
	void				ClosePortal( void );

private:
	qhandle_t			portal;
	bool				state;

	/**
	* Set to true if the portal state depends on distance from player
	**/
	bool				m_bDistDependent;
	
	/**
	* Timestamp and interval between checks, in milliseconds
	**/
	int					m_TimeStamp;
	int					m_Interval;
	/**
	* Distance at which the portal shuts off, if it is distance dependent
	**/
	float				m_Distance;

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

	// greebo: Public function to set the state directly
	// Note: Passing TRUE means that the AAS area is DISABLED
	void				SetAASState(bool newState);

private:
	bool				state;

	void				Event_Activate( idEntity *activator );
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

	virtual void		Think( void ) override;

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

idPortalSky

===============================================================================
*/

class idPortalSky : public idEntity {

public:

	CLASS_PROTOTYPE( idPortalSky );

	idPortalSky();

	virtual ~idPortalSky() override;

	void				Spawn( void );
	void				Event_PostSpawn();
	void				Event_Activate( idEntity *activator );
};

/*
===============================================================================

  CVine

===============================================================================
*/

class tdmVine: public idStaticEntity
{
public:
	CLASS_PROTOTYPE( tdmVine);
	// Constructor
	tdmVine();

	// Needed on game save/load
	void	Save( idSaveGame *savefile ) const;
	void	Restore( idRestoreGame *savefile );

	// Gets called when this entity is actually being spawned
	void	Spawn();

private:
	bool	_watered;	// true if a vine piece was watered during this watering event
	tdmVine* _prime;	// initial, or prime, vine piece
	idList< idEntityPtr<tdmVine> >_descendants;	// a list of all descendants (kept by prime only)

protected:
	void	Event_SetPrime( tdmVine* newPrime );
	void	Event_GetPrime();
	void	Event_AddDescendant( tdmVine* descendant );
	void	Event_ClearWatered();
	void	Event_SetWatered();
	void	Event_CanWater();
	void	Event_ScaleVine(float factor);
};

/*
===============================================================================

idPeek

===============================================================================
*/

class idPeek : public idStaticEntity
{
public:
	CLASS_PROTOTYPE(idPeek);
	
	idPeek(); // Constructor
	virtual ~idPeek() override; // Destructor

	void	Spawn();
};

#endif /* !__GAME_MISC_H__ */

