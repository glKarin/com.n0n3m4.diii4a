// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PHYSICS_STATIC_H__
#define __PHYSICS_STATIC_H__

/*
===============================================================================

	Physics for a non moving object using at most one collision model.

===============================================================================
*/

#include "Physics.h"

typedef struct staticPState_s {
	idVec3					origin;
	idMat3					axis;
} staticPState_t;

class sdPhysicsStaticNetworkData : public sdEntityStateNetworkData {
public:
							sdPhysicsStaticNetworkData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					position;
	idCQuat					orientation;
};

class sdPhysicsStaticBroadcastData : public sdEntityStateNetworkData {
public:
							sdPhysicsStaticBroadcastData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idVec3					localPosition;
	idCQuat					localOrientation;
};

// Gordon: FIXME: Half of this stuff is virtual, but isn't marked as virtual, which i hate.
class idPhysics_Static : public idPhysics {
public:
	CLASS_PROTOTYPE( idPhysics_Static );

							idPhysics_Static( void );
							~idPhysics_Static( void );

	void					SetNoPrediction( void ) { flags.noPrediction = true; }

public:	// common physics interface
	void					SetSelf( idEntity *e );

	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;
	virtual const idMat3&	GetInertiaTensor( int id = -1 ) const { return mat3_zero; }
	virtual const idVec3&	GetCenterOfMass( void ) const { return vec3_origin; }

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	void					SetClipMask( int mask, int id = -1 );
	int						GetClipMask( int id = -1 ) const;

	const idBounds &		GetBounds( int id = -1 ) const;
	const idBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					GetImpactInfo( const int id, const idVec3 &point, impactInfo_t *info ) const;
	void					ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse );
	void					AddForce( const int id, const idVec3 &point, const idVec3 &force );
	virtual void			AddForce( const idVec3& force );
	virtual void			AddTorque( const idVec3& torque );
	void					Activate( void );
	void					PutToRest( void );
	bool					IsAtRest( void ) const;
	int						GetRestStartTime( void ) const;
	bool					IsPushable( void ) const;

	virtual void			EnableImpact( void ) { }
	virtual void			DisableImpact( void ) { }

	void					SaveState( void );
	void					RestoreState( void );

	void					SetOrigin( const idVec3 &newOrigin, int id = -1 );
	void					SetAxis( const idMat3 &newAxis, int id = -1 );

	void					Translate( const idVec3 &translation, int id = -1 );
	void					Rotate( const idRotation &rotation, int id = -1 );

	const idVec3 &			GetOrigin( int id = 0 ) const;
	const idMat3 &			GetAxis( int id = 0 ) const;

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;
	const idVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					SetGravity( const idVec3 &newGravity );
	const idVec3 &			GetGravity( void ) const;
	const idVec3 &			GetGravityNormal( void ) const;

	void					ClipTranslation( trace_t& results, const idVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t& results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	void					UnlinkClip( void );
	void					LinkClip( void );
	void					DisableClip( bool activateContacting = true );
	void					EnableClip( void );

	bool					EvaluateContacts( CLIP_DEBUG_PARMS_DECLARATION_ONLY );
	int						GetNumContacts( void ) const;
	const contactInfo_t&	GetContact( int num ) const;
	void					ClearContacts( void );
	void					AddContactEntity( idEntity *e );
	void					RemoveContactEntity( idEntity *e );

	bool					HasGroundContacts( void ) const;
	bool					IsGroundEntity( int entityNum ) const;
	bool					IsGroundClipModel( int entityNum, int id ) const;

	void					SetPushed( int deltaTime );
	const idVec3&			GetPushedLinearVelocity( const int id = 0 ) const;
	const idVec3&			GetPushedAngularVelocity( const int id = 0 ) const;

	void					SetMaster( idEntity *master, const bool orientated = true );

	const trace_t*			GetBlockingInfo( void ) const;
	idEntity*				GetBlockingEntity( void ) const;

	int						GetLinearEndTime( void ) const;
	int						GetAngularEndTime( void ) const;

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual void			DrawDebugInfo( void ) { }

	virtual void			SetComeToRest( bool ) { }

	virtual float			InWater( void ) const { return 0.0f; }

	virtual bool			AllowInhibit( void ) const { return true; }

protected:
	idEntity *				self;					// entity using this physics object
	staticPState_t			current;				// physics state
	idClipModel *			clipModel;				// collision model
	idVec3					localOrigin;
	idMat3					localAxis;

	// master
	struct flags_t {
		bool				hasMaster		: 1;
		bool				isOrientated	: 1;
		bool				noPrediction	: 1;
	};

	flags_t					flags;
};

#endif /* !__PHYSICS_STATIC_H__ */
