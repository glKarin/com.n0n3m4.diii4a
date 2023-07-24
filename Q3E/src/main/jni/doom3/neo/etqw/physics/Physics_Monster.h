// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_MONSTER_H__
#define __PHYSICS_MONSTER_H__

#include "Physics_Actor.h"

/*
===================================================================================

	Monster physics

	Simulates the motion of a monster through the environment. The monster motion
	is typically driven by animations.

===================================================================================
*/

typedef enum {
	MM_OK,
	MM_SLIDING,
	MM_BLOCKED,
	MM_STEPPED,
	MM_FALLING
} monsterMoveResult_t;

typedef struct monsterPState_s {
	bool					onGround;
	float					stepUp;
	idVec3					origin;
	idVec3					velocity;
	idVec3					pushVelocity;
} monsterPState_t;

class sdMonsterPhysicsNetworkData : public sdEntityStateNetworkData {
public:
							sdMonsterPhysicsNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					origin;
	idVec3					velocity;
};

class sdMonsterPhysicsBroadcastData : public sdEntityStateNetworkData {
public:
							sdMonsterPhysicsBroadcastData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					pushVelocity;
	int						atRest;
};

class idPhysics_Monster : public idPhysics_Actor {

public:
	CLASS_PROTOTYPE( idPhysics_Monster );

							idPhysics_Monster( void );

							// maximum step up the monster can take, default 18 units
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight( void ) const;
							// set delta for next move
	void					SetDelta( const idVec3 &d );
							// returns true if monster is standing on the ground
	bool					OnGround( void ) const;
							// overrides any velocity for pure delta movement
	void					ForceDeltaMove( bool force );
							// don't use delta movement
	void					UseVelocityMove( bool force );
							// get entity blocking the move
	idEntity *				GetSlideMoveEntity( void ) const;
							// enable/disable activation by impact
	virtual void			EnableImpact( void );
	virtual void			DisableImpact( void );

public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;

	virtual void			Activate( void );
	virtual void			PutToRest( void );
	virtual bool			IsAtRest( void );

	virtual int				VehiclePush( bool stuck, float timeDelta, idVec3& move, idClipModel* pusher, int pushCount );

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
	
	virtual float			InWater( void ) const { return waterLevel; }

	const idVec3&			GetGroundNormal( void ) const { return groundTrace.c.normal; }

	virtual void			DebugDraw( void );

	void					SetStability( bool value ) { isStable = value; }

	// Currently the monster physics won't handle being pushed gracefully, so setting this to false for now!
	virtual bool			IsPushable( void ) const { return false; }

private:
	// monster physics state
	monsterPState_t			current;
	monsterPState_t			saved;
	int						atRest;

	int						framemsec;
	float					frametime;

	// properties
	float					maxStepHeight;		// maximum step height
	idVec3					delta;				// delta for next move

	bool					useVelocityMove;
	bool					noImpact;			// if true do not activate when another object collides
	bool					walking;

	trace_t					groundTrace;

	// results of last evaluate
	idEntity *				blockingEntity;

	float					waterLevel;

	bool					isStable;

private:
	void					CheckGround( void );
	bool					SlideMove( bool gravity, bool stepUp, bool stepDown, bool push, int vehiclePush );
	void					CheckWater( void );
	void					WalkMove( void );
	void					AirMove( void );
};

#endif /* !__PHYSICS_MONSTER_H__ */
