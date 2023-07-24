// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_SIMPLE_H__
#define __PHYSICS_SIMPLE_H__

#include "Physics_Actor.h"

/*
===================================================================================

	Simple physics

	This is just an AABB - it has no angular info. 
	It can't be pushed, but it can push.
	Applying forces & impulses works appropriately.

===================================================================================
*/

typedef struct simplePState_s {
	idVec3					origin;
	idVec3					velocity;
	idVec3					pushVelocity;

	idMat3					axis;
	idVec3					angularVelocity;
} simplePState_t;

class sdSimplePhysicsNetworkData : public sdEntityStateNetworkData {
public:
							sdSimplePhysicsNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					origin;
	idVec3					velocity;
	idCQuat					orientation;
	idVec3					angularVelocity;
	int						atRest;
	bool					locked;
};

class sdSimplePhysicsBroadcastData : public sdEntityStateNetworkData {
public:
							sdSimplePhysicsBroadcastData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					pushVelocity;
	int						atRest;
	float					groundLevel;
	bool					locked;
};

class sdPhysics_Simple : public idPhysics_Actor {

public:
	CLASS_PROTOTYPE( sdPhysics_Simple );

							sdPhysics_Simple( void );
							~sdPhysics_Simple( void );


public:	// common physics interface
	bool					Evaluate( int timeStepMSec, int endTimeMSec );

	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	const idVec3 &			GetOrigin( int id = 0 ) const;

	void					SetAxis( const idMat3 &newAxis, int id = -1 );
	const idMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );
	const idVec3 &			GetLinearVelocity( int id = 0 ) const;

	void					SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 );
	const idVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					SaveState( void );
	void					RestoreState( void );

	void					SetPushed( int deltaTime );
	const idVec3 &			GetPushedLinearVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated = true );

	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					PutToRest( void );
	void					Rest( int time );
	bool					IsPushable( void ) const;

	virtual void			Activate( void );
	virtual bool			IsAtRest( void ) const;
	virtual void			SetComeToRest( bool value );

	void					SetGroundPosition( const idVec3& position );

	virtual void			EnableImpact( void );
	virtual void			DisableImpact( void );


	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
	
private:
	int						atRest;
	bool					rotates;
	bool					locked;

	// physics state
	simplePState_t			current;
	simplePState_t			saved;

	float					groundLevel;

	idVec3					centerOfMass;
	idMat3					inertiaTensor;
	idMat3					inverseInertiaTensor;
};

#endif /* !__PHYSICS_SIMPLE_H__ */
