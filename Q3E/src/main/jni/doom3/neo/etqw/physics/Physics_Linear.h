// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __PHYSICS_LINEAR_H__
#define __PHYSICS_LINEAR_H__

#include "Physics_Base.h"

typedef struct linearPState_s {
	int										time;					// physics time
	int										atRest;					// set when simulation is suspended
	idVec3									origin;					// world origin
	idVec3									localOrigin;			// local origin
	idExtrapolate< idVec3 >					linearExtrapolation;	// extrapolation based description of the position over time
} linearPState_t;

class sdPhysicsLinearBroadcastData : public sdEntityStateNetworkData {
public:
									sdPhysicsLinearBroadcastData( void ) { ; }

	virtual void					MakeDefault( void );

	virtual void					Write( idFile* file ) const;
	virtual void					Read( idFile* file );

	int								atRest;
	idVec3							localOrigin;
	extrapolation_t					extrapolationType;
	int								startTime;
	int								duration;
	idVec3							startValue;
	idVec3							speed;
	idVec3							baseSpeed;
};

class sdPhysics_Linear : public idPhysics_Base {
public:
	CLASS_PROTOTYPE( sdPhysics_Linear );

							sdPhysics_Linear( void );
							~sdPhysics_Linear( void );

	void					SetPusher( int flags );
	bool					IsPusher( void ) const;

	void					SetLinearExtrapolation( extrapolation_t type, int time, int duration, const idVec3 &base, const idVec3 &speed, const idVec3 &baseSpeed );
	extrapolation_t			GetLinearExtrapolationType( void ) const { return current.linearExtrapolation.GetExtrapolationType(); }

	void					GetLocalOrigin( idVec3 &curOrigin ) const;

public:	// common physics interface
	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const idBounds &		GetBounds( int id = -1 ) const;
	const idBounds &		GetAbsBounds( int id = -1 ) const;

	bool					Evaluate( int timeStepMSec, int endTimeMSec );
	void					UpdateTime( int endTimeMSec );
	int						GetTime( void ) const;

	void					Activate( void );
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

	void					SetLinearVelocity( const idVec3 &newLinearVelocity, int id = 0 );
	void					SetAngularVelocity( const idVec3 &newAngularVelocity, int id = 0 );

	const idVec3 &			GetLinearVelocity( int id = 0 ) const;
	const idVec3 &			GetAngularVelocity( int id = 0 ) const;

	void					UnlinkClip( void );
	void					LinkClip( void );
	void					DisableClip( bool activateContacting = true );
	void					EnableClip( void );

	void					SetMaster( idEntity *master, const bool orientated = true );

	const trace_t *			GetBlockingInfo( void ) const;
	idEntity *				GetBlockingEntity( void ) const;

	int						GetLinearEndTime( void ) const;

	virtual void			ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual bool			CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void			WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual void			ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

private:
	// parametric physics state
	linearPState_t			current;
	linearPState_t			saved;

	idMat3					axis;					// world axis

	// pusher
	bool					isPusher;
	idClipModel *			clipModel;
	int						pushFlags;

	// results of last evaluate
	trace_t					pushResults;
	bool					isBlocked;

	// master
	bool					hasMaster;
	bool					isOrientated;

private:
	bool					TestIfAtRest( void ) const;
	void					Rest( void );
};

#endif /* __PHYSICS_LINEAR_H__ */
