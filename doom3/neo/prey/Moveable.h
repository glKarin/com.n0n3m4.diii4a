// Copyright (C) 2004 Id Software, Inc.
//

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
							~idMoveable( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );

	bool					AllowStep( void ) const;
	void					EnableDamage( bool enable, float duration );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );

protected:
	idPhysics_RigidBody		physicsObj;				// physics object
//	idStr					brokenModel;			// model set when health drops down to or below zero
	idStr					damage;					// if > 0 apply damage to hit entities
	idStr					fxCollide;				// fx system to start when collides with something
	int						nextCollideFxTime;		// next time it is ok to spawn collision fx
	float					minDamageVelocity;		// minimum velocity before moveable applies damage
	float					maxDamageVelocity;		// velocity at which the maximum damage is applied
	idCurve_Spline<idVec3> *initialSpline;			// initial spline path the moveable follows
	idVec3					initialSplineDir;		// initial relative direction along the spline path
// HUMANHEAD pdm: unused
//	bool					explode;				// entity explodes when health drops down to or below zero
	bool					unbindOnDeath;			// unbind from master when health drops down to or below zero
	bool					allowStep;				// allow monsters to step on the object
	bool					canDamage;				// only apply damage when this is set
	int						nextDamageTime;			// next time the movable can hurt the player
	int						nextSoundTime;			// next time the moveable can make a sound

	const idMaterial *		GetRenderModelMaterial( void ) const;
	void					BecomeNonSolid( void );
	void					InitInitialSpline( int startTime );
	bool					FollowInitialSplinePath( void );

	virtual	// HUMANHEAD: made virtual
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

  HUMANHEAD pdm: removed, re-inherited from hhMoveable

===============================================================================
*/


/*
===============================================================================

  A barrel using rigid body physics and special handling of the view model
  orientation to make it look like it rolls instead of slides. The barrel
  can burn and explode when damaged.

  HUMANHEAD pdm: removed, unused

===============================================================================
*/


#endif /* !__GAME_MOVEABLE_H__ */
