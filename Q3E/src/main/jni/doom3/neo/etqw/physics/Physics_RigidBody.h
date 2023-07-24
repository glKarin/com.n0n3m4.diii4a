// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_RIGIDBODY_H__
#define __PHYSICS_RIGIDBODY_H__

/*
===================================================================================

	Rigid body physics

	Employs an impulse based dynamic simulation which is not very accurate but
	relatively fast and still reliable due to the continuous collision detection.

===================================================================================
*/

#include "Physics_Base.h"

typedef struct rigidBodyIState_s {
	idVec3					position;					// position of trace model
	idMat3					orientation;				// orientation of trace model
	idVec3					linearMomentum;				// translational momentum relative to center of mass
	idVec3					angularMomentum;			// rotational momentum relative to center of mass
} rigidBodyIState_t;

typedef struct rigidBodyPState_s {
	int						atRest;						// set when simulation is suspended
	float					lastTimeStep;				// length of last time step
	idVec6					pushVelocity;				// push velocity
	idVec3					externalForce;				// external force relative to center of mass
	idVec3					externalTorque;				// external torque relative to center of mass
	rigidBodyIState_t		i;							// state used for integration
} rigidBodyPState_t;

class sdRigidBodyNetworkState : public sdEntityStateNetworkData {
public:
							sdRigidBodyNetworkState( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					position;
	idCQuat 				orientation;
	idVec3					linearVelocity;
	idVec3					angularVelocity;
};

class sdRigidBodyBroadcastState : public sdEntityStateNetworkData {
public:
							sdRigidBodyBroadcastState( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					localPosition;
	idCQuat					localOrientation;
	int						atRest;
};

class idPhysics_RigidBody : public idPhysics_Base {

public:

	CLASS_PROTOTYPE( idPhysics_RigidBody );

							idPhysics_RigidBody( void );
							~idPhysics_RigidBody( void );

							// initialisation
	void					SetFriction( const float linear, const float angular, const float contact );
	void					SetWaterFriction( const float linear, const float angular );
	void					SetBouncyness( const float b );
	void					SetBuoyancy( float b );
							// same as above but drop to the floor first
	void					DropToFloor( void );
							// no contact determination and contact friction
	void					NoContact( void );
							// enable/disable activation by impact
	virtual void			EnableImpact( void );
	virtual void			DisableImpact( void );

public:	// common physics interface
	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	virtual const idMat3&	GetInertiaTensor( int id = -1 ) const { return inertiaTensor; }

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const idBounds &		GetBounds( int id = -1 ) const;
	const idBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );
	void					AddForce( const int id, const idVec3 &point, const idVec3 &force );
	void					Activate( void );
	void					PutToRest( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const idVec3 &			GetOrigin( int id = 0 ) const;
	const idMat3 &			GetAxis( int id = 0 ) const;

	virtual const idVec3&	GetCenterOfMass() const { return centerOfMass; }

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;
	const idVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	void					UnlinkClip( void );
	void					LinkClip( void );
	void					DisableClip( bool activateContacting = true );
	void					EnableClip( void );

	bool					EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY );

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;
	const idVec3 &			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated );

	void					SetApplyImpulse( bool i ) { noApplyImpulse = !i; }

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual void			DrawDebugInfo( void ) { DebugDraw(); }

	virtual float			InWater( void ) const { return waterLevel; }

private:
	void					CheckWater( void );

private:
	// state of the rigid body
	rigidBodyPState_t		current;
	rigidBodyPState_t		saved;
	idVec3					localOrigin;				// origin relative to master
	idMat3					localAxis;					// axis relative to master

	// rigid body properties
	float					linearFriction;				// translational friction
	float					angularFriction;			// rotational friction
	float					bouncyness;					// bouncyness

	float					linearFrictionWater;		// translational friction when in water
	float					angularFrictionWater;		// rotational friction when in water

	float					contactFriction;			// friction with contact surfaces
	float					buoyancy;
	idClipModel *			clipModel;					// clip model used for collision detection
	idClipModel	*			centeredClipModel;			// clip model at the center of mass

	// derived properties
	float					mass;						// mass of body
	float					inverseMass;				// 1 / mass
	idVec3					centerOfMass;				// center of mass of trace model
	idMat3					inertiaTensor;				// mass distribution
	idMat3					inverseInertiaTensor;		// inverse inertia tensor

	idODE *					integrator;					// integrator
	bool					dropToFloor;				// true if dropping to the floor and putting to rest
	bool					testSolid;					// true if testing for solid when dropping to the floor
	bool					noImpact;					// if true do not activate when another object collides
	bool					noContact;					// if true do not determine contacts and no contact friction
	bool					noApplyImpulse;				// if true do not apply an impulse to another object when colliding

	// master
	bool					hasMaster;
	bool					isOrientated;
	float					waterLevel;

private:
	friend void				RigidBodyDerivatives( const float t, const void *clientData, const float *state, float *derivatives );
	void					Integrate( const float deltaTime, rigidBodyPState_t &next );
	bool					CheckForCollisions( const float deltaTime, rigidBodyPState_t &next, trace_t &collision );
	bool					CollisionImpulse( const trace_t &collision, idVec3 &impulse );
	void					ContactFriction( float deltaTime );
	void					DropToFloorAndRest( void );
	bool					TestIfAtRest( void ) const;
	void					Rest( void );
	void					DebugDraw( void );
};

#endif /* !__PHYSICS_RIGIDBODY_H__ */
