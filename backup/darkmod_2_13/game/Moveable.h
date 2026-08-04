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

#ifndef __GAME_MOVEABLE_H__
#define __GAME_MOVEABLE_H__

/*
===============================================================================

  Entity using rigid body physics.

===============================================================================
*/

extern const idEventDef EV_BecomeNonSolid;
extern const idEventDef EV_IsAtRest;

class idMoveable : public idEntity {
public:
	CLASS_PROTOTYPE( idMoveable );

							idMoveable( void );
	virtual					~idMoveable( void ) override;

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void ) override;

	virtual void			Hide( void ) override;
	virtual void			Show( void ) override;

	bool					AllowStep( void ) const;
	void					EnableDamage( bool enable, float duration );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity ) override;
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;

	// Update the "pushed" state of this entity
	virtual void			SetIsPushed(bool isPushed, const idVec3& pushDirection);

	// Returns true if the entity is pushed by something or someone
	virtual bool			IsPushed();

protected:
	idPhysics_RigidBody		physicsObj;				// physics object
	idStr					damage;					// if > 0 apply damage to hit entities
	idStr					fxCollide;				// fx system to start when collides with something
	int						nextCollideFxTime;		// next time it is ok to spawn collision fx

	/**
	* TDM Collision scripts
	**/
	idStr					m_scriptCollide;		// script function to call when collides with something
	int						m_nextCollideScriptTime;// next time it is ok to call collision script
	int						m_collideScriptCounter;	// how often to call the collision script
													// 0 => never, -1 => always, +X => X times
	float					m_minScriptVelocity;	// minimum velocity before calling collide script

	float					minDamageVelocity;		// minimum velocity before moveable applies damage
	float					maxDamageVelocity;		// velocity at which the maximum damage is applied
	idCurve_Spline<idVec3> *initialSpline;			// initial spline path the moveable follows
	idVec3					initialSplineDir;		// initial relative direction along the spline path
	bool					explode;				// entity explodes when health drops down to or below zero
	bool					unbindOnDeath;			// unbind from master when health drops down to or below zero
	bool					allowStep;				// allow monsters to step on the object
	bool					canDamage;				// only apply damage when this is set
	int						nextDamageTime;			// next time the movable can hurt the player
	int						nextSoundTime;			// next time the moveable can make a sound

	// greebo: Stores the last collision info to avoid constant playing of the collision sound when stuck
	trace_t					lastCollision;

	bool					isPushed;				// true if the entity is pushed by something/someone
	bool					wasPushedLastFrame;		// true if the entity was pushed the last frame
	idVec3					pushDirection;			// the direction the moveable is pushed in
	idVec3					lastPushOrigin;			// the old origin during pushing to compare whether we have actually moved somewhere

	const idMaterial *		GetRenderModelMaterial( void ) const;
	void					BecomeNonSolid( void );
	void					InitInitialSpline( int startTime );
	bool					FollowInitialSplinePath( void );

	// greebo: Updates the sliding sounds according to the "pushed" state
	void					UpdateSlidingSounds();

	void					Event_Activate( idEntity *activator );
	void					Event_BecomeNonSolid( void );
	void					Event_SetOwnerFromSpawnArgs( void );
	void					Event_IsAtRest( void );
	void					Event_EnableDamage( float enable );
};


/*
===============================================================================

  A barrel using rigid body physics. The barrel has special handling of
  the view model orientation to make it look like it rolls instead of slides.

===============================================================================
*/

class idBarrel : public idMoveable {

public:
	CLASS_PROTOTYPE( idBarrel );
							idBarrel();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					BarrelThink( void );
	virtual void			Think( void ) override;
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) override;
	virtual void			ClientPredictionThink( void ) override;

private:
	float					radius;					// radius of barrel
	int						barrelAxis;				// one of the coordinate axes the barrel cylinder is parallel to
	idVec3					lastOrigin;				// origin of the barrel the last think frame
	idMat3					lastAxis;				// axis of the barrel the last think frame
	float					additionalRotation;		// additional rotation of the barrel about it's axis
	idMat3					additionalAxis;			// additional rotation axis
};


/*
===============================================================================

  A barrel using rigid body physics and special handling of the view model
  orientation to make it look like it rolls instead of slides. The barrel
  can burn and explode when damaged.

===============================================================================
*/

class idExplodingBarrel : public idBarrel {
public:
	CLASS_PROTOTYPE( idExplodingBarrel );

							idExplodingBarrel();
	virtual					~idExplodingBarrel() override;

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void ) override;
	virtual void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
								const char *damageDefName,const float damageScale,
								const int location, trace_t *tr = NULL ) override;
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) override;

	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const override;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ) override;
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg ) override;

	enum {
		EVENT_EXPLODE = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

private:
	typedef enum {
		NORMAL = 0,
		BURNING,
		BURNEXPIRED,
		EXPLODING
	} explode_state_t;
	explode_state_t			state;

	idVec3					spawnOrigin;
	idMat3					spawnAxis;
	qhandle_t				particleModelDefHandle;
	qhandle_t				lightDefHandle;
	renderEntity_t			particleRenderEntity;
	renderLight_t			light;
	int						particleTime;
	int						lightTime;
	float					time;

	void					AddParticles( const char *name, bool burn );
	void					AddLight( const char *name , bool burn );
	void					ExplodingEffects( void );

	void					Event_Activate( idEntity *activator );
	void					Event_Respawn();
	void					Event_Explode();
	void					Event_TriggerTargets();
};

#endif /* !__GAME_MOVEABLE_H__ */
