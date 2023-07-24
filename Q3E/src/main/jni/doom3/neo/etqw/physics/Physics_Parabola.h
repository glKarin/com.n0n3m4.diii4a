// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __PHYSICS_PARABOLA_H__
#define __PHYSICS_PARABOLA_H__

#include "Physics_Base.h"

class sdPhysicsParabolaBroadcastData : public sdEntityStateNetworkData {
public:
							sdPhysicsParabolaBroadcastData( void ) { ; }

	virtual void			MakeDefault( void );

	virtual void			Write( idFile* file ) const;
	virtual void			Read( idFile* file );

	idCQuat					orientation;
	idVec3					position;
	idVec3					velocity;
	idVec3					acceleration;

	int						startTime;
	int						endTime;
};

class sdPhysics_Parabola : public idPhysics_Base {
public:
	typedef struct parabolaPState_s {
		int					time;
		idVec3				origin;
		idVec3				velocity;
	} parabolaPState_t;

	CLASS_PROTOTYPE( sdPhysics_Parabola );

							sdPhysics_Parabola( void );
	virtual					~sdPhysics_Parabola( void );

	void					Init( const idVec3& origin, const idVec3& velocity, const idVec3& acceleration, const idMat3& axes, int _startTime, int _endTime );

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;

	virtual int				GetNumClipModels( void ) const { return clipModel ? 1 : 0; }

	virtual idVec3			EvaluatePosition( void ) const;
	virtual bool			Evaluate( int timeStepMSec, int endTimeMSec );

	void					AdjustForMaster( idVec3& org, idMat3& axes );
	void					CalcProperties( idVec3& origin, idVec3& velocity, int time ) const;

	void					CheckWater( void );
	bool					CheckForCollisions( parabolaPState_t& next, trace_t& collision );
	bool					CollisionResponse( trace_t& collision );

	virtual void			SetClipModel( idClipModel* model, float density = 0.f, int id = 0, bool freeOld = true );
	virtual idClipModel*	GetClipModel( int id = 0 ) const;

	virtual void			SetAxis( const idMat3& newAxis, int id );
	virtual void			SetOrigin( const idVec3& newOrigin, int id );

	virtual void			LinkClip( void );

	virtual const idVec3&	GetLinearVelocity( int id ) const { return current.velocity; }
	virtual const idVec3&	GetOrigin( int id ) const { return current.origin; }
	virtual const idMat3&	GetAxis( int id ) const { return baseAxes; }

	virtual const idBounds&	GetBounds( int id = -1 ) const;
	virtual const idBounds&	GetAbsBounds( int id = -1 ) const;

	
	virtual void			SetContents( int mask, int id = -1 );

	virtual float			InWater( void ) const { return waterLevel; }

private:
	parabolaPState_t		current;

	idVec3					baseOrg;
	idVec3					baseVelocity;
	idVec3					baseAcceleration;
	idMat3					baseAxes;
	int						startTime;
	int						endTime;
	float					waterLevel;

	idClipModel*			clipModel;
};

#endif // __PHYSICS_PARABOLA_H__
