// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_JETPACK_H__
#define __PHYSICS_JETPACK_H__

#include "Physics_Actor.h"

class sdJetPack;

/*
===================================================================================

	Jet Pack physics

===================================================================================
*/


// movement flags
// NOTE: remember to update JETPACK_MOVEMENT_FLAGS_BITS if you change these!
const int JPF_JUMPED		= 0x00000001;
const int JPF_JUMP_HELD		= 0x00000002;
const int JPF_STEPPED_UP	= 0x00000004;
const int JPF_STEPPED_DOWN	= 0x00000008;

typedef struct jetPackPState_s {
	bool					onGround;
	bool					onWater;
	idVec3					origin;
	idVec3					velocity;
	idVec3					pushVelocity;
	int						movementFlags;
	float					stepUp;
} jetPackPState_t;

class sdJetPackPhysicsNetworkData : public sdEntityStateNetworkData {
public:
							sdJetPackPhysicsNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					origin;
	idVec3					velocity;
	int						movementFlags;
};

class sdJetPackPhysicsBroadcastData : public sdEntityStateNetworkData {
public:
							sdJetPackPhysicsBroadcastData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					pushVelocity;
};

class sdPhysics_JetPack : public idPhysics_Actor {

public:
	CLASS_PROTOTYPE( sdPhysics_JetPack );

							sdPhysics_JetPack( void );
	virtual					~sdPhysics_JetPack();

							// maximum step up that we can take
	void					SetMaxStepHeight( const float newMaxStepHeight );
	float					GetMaxStepHeight( void ) const;
							// returns true if on the ground
	bool					OnGround( void ) const;
							// enable/disable activation by impact
	virtual void			EnableImpact( void );
	virtual void			DisableImpact( void );

	float					GetBoost( void ) const { return boost; }
	idVec3 const &			GetFanForce( void ) const { return fanForce; }
	bool					HasJumped( void ) const { return ( ( current.movementFlags & JPF_JUMPED ) != 0 ); }

	void					SetPlayerInput( const usercmd_t& cmd, const idAngles &newViewAngles, bool allowMovement );
	void					SetBoost( float newBoost ) { boost = newBoost; }
	inline const idVec3&	GetCurrentOrigin() { return current.origin; } 

	virtual void			Activate( void );

	inline void				SetMaxSpeed( float value )			{ maxSpeed = value; }
	inline void				SetMaxBoostSpeed( float value )		{ maxBoostSpeed = value; }
	inline void				SetWalkForceScale( float value )	{ walkForceScale = value; }
	inline void				SetKineticFriction( float value )	{ kineticFriction = value; }
	inline void				SetJumpForce( float value )			{ jumpForce = value; }
	inline void				SetBoostForce( float value )		{ boostForce = value; }

	virtual int				VehiclePush( bool stuck, float timeDelta, idVec3& move, idClipModel* pusher, int pushCount );

	virtual void			SetSelf( idEntity *e );

	virtual void			UnlinkClip( void );
	virtual void			LinkClip( void );
	virtual void			DisableClip( bool activateContacting = true );
	virtual void			EnableClip( void );

	virtual void			SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );

public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	bool					EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY );

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );

	void					SaveState( void );
	void					RestoreState( void );

	virtual const idVec3&	GetOrigin( int id = -1 ) const;

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
	virtual void						ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );

	virtual void			SetComeToRest( bool enabled ) { disableBoost = enabled; }

	virtual float			InWater( void ) const { return waterFraction; }

private:
	idVec3					GroundMoveForce( bool skiing, const idVec3& desiredMove, const idVec3& forceSoFar, float boost, float timeStep );
	idVec3					FrictionForce( bool skiing, const idVec3& desiredMove, const idVec3& forceSoFar, float boost, float timeStep );
	idVec3					JumpForce( bool skiing, float upMove, float boost, float timeStep );
	idVec3					BoostForce( bool skiing, const idVec3& desiredMove, float boost, float timeStep );
	void					CalcWaterFraction( void );

	void					LinkShotModel( void );

	sdJetPack*				jetPackSelf;

	idClipModel*			shotClipModel;

	// physics state
	jetPackPState_t			current;
	jetPackPState_t			saved;
	idVec3					addedVelocity;
	float					waterFraction;

	// player input
	usercmd_t				command;
	idAngles				viewAngles;
	bool					movementAllowed;
	float					boost;

	// properties
	float					maxStepHeight;		// maximum step height
	bool					noImpact;			// if true do not activate when another object collides
	bool					disableBoost;

	float					maxSpeed;
	float					maxBoostSpeed;
	float					walkForceScale;
	float					kineticFriction;
	float					jumpForce;
	float					boostForce;	
	idVec3					fanForce;

	// results of last evaluate
	trace_t					groundTrace;
	bool					groundTraceValid;

private:
	void					CheckGround( void );
	bool					SlideMove( bool stepUp, bool stepDown, bool push, int vehiclePush, float frametime );
};

#endif /* !__PHYSICS_JETPACK_H__ */
