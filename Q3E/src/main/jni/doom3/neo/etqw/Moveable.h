// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_MOVEABLE_H__
#define __GAME_MOVEABLE_H__

/*
===============================================================================

  Entity using rigid body physics.

===============================================================================
*/

#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "effects/WaterEffects.h"

extern const idEventDef EV_BecomeNonSolid;

class sdMoveableNetworkData : public sdEntityStateNetworkData {
public:
								sdMoveableNetworkData( void ) : physicsData( NULL ) { ; }
	virtual						~sdMoveableNetworkData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	sdEntityStateNetworkData*	physicsData;
};

class sdMoveableBroadcastData : public sdEntityStateNetworkData {
public:
								sdMoveableBroadcastData( void ) : physicsData( NULL ) { ; }
	virtual						~sdMoveableBroadcastData( void );

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	sdEntityStateNetworkData*	physicsData;
	bool						hidden;
};

class idMoveable : public idEntity {
public:
	CLASS_PROTOTYPE( idMoveable );

							idMoveable( void );
							~idMoveable( void );

	void					Spawn( void );

	virtual void			Hide( void );
	virtual void			Show( void );

	bool					AllowStep( void ) const;
	void					EnableDamage( bool enable, float duration );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity, int bodyId );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );
	virtual void			CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );

protected:
	idPhysics_RigidBody		physicsObj;				// physics object
	int						nextCollideFxTime;		// next time it is ok to spawn collision fx
	float					minDamageVelocity;		// minimum velocity before moveable applies damage
	float					maxDamageVelocity;		// velocity at which the maximum damage is applied
	bool					explode;				// entity explodes when health drops down to or below zero
	bool					unbindOnDeath;			// unbind from master when health drops down to or below zero
	bool					allowStep;				// allow monsters to step on the object
	bool					canDamage;				// only apply damage when this is set
	const sdDeclDamage*		damageDecl;
	int						nextDamageTime;			// next time the movable can hurt the player
	int						nextSoundTime;			// next time the moveable can make a sound
	sdWaterEffects			*waterEffects;

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	const idMaterial *		GetRenderModelMaterial( void ) const;
	void					BecomeNonSolid( void );

	void					Event_Activate( idEntity *activator );
	void					Event_BecomeNonSolid( void );
	void					Event_SetOwnerFromSpawnArgs( void );
	void					Event_IsAtRest( void );
	void					Event_EnableDamage( bool enable );
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

	void					BarrelThink( void );
	virtual void			Think( void );
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );

	virtual bool			StartSynced( void ) const { return true; }

private:
	float					radius;					// radius of barrel
	int						barrelAxis;				// one of the coordinate axes the barrel cylinder is parallel to
	idVec3					lastOrigin;				// origin of the barrel the last think frame
	idMat3					lastAxis;				// axis of the barrel the last think frame
	float					additionalRotation;		// additional rotation of the barrel about it's axis
	idMat3					additionalAxis;			// additional rotation axis
};

#endif /* !__GAME_MOVEABLE_H__ */
