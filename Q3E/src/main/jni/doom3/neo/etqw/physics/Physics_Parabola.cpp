// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Physics_Parabola.h"

CLASS_DECLARATION( idPhysics_Base, sdPhysics_Parabola )
END_CLASS

/*
================
sdPhysicsParabolaBroadcastData::MakeDefault
================
*/
void sdPhysicsParabolaBroadcastData::MakeDefault( void ) {
	orientation.x	= 0.f;
	orientation.y	= 0.f;
	orientation.z	= 0.f;

	position		= vec3_origin;
	velocity		= vec3_zero;
	acceleration	= vec3_zero;

	startTime		= 0;
	endTime			= 0;
}

/*
================
sdPhysicsParabolaBroadcastData::Write
================
*/
void sdPhysicsParabolaBroadcastData::Write( idFile* file ) const {
	file->WriteCQuat( orientation );
	file->WriteVec3( position );
	file->WriteVec3( velocity );
	file->WriteVec3( acceleration );

	file->WriteInt( startTime );
	file->WriteInt( endTime );
}

/*
================
sdPhysicsParabolaBroadcastData::Read
================
*/
void sdPhysicsParabolaBroadcastData::Read( idFile* file ) {
	file->ReadCQuat( orientation );
	file->ReadVec3( position );
	file->ReadVec3( velocity );
	file->ReadVec3( acceleration );

	file->ReadInt( startTime );
	file->ReadInt( endTime );
}


/*
================
sdPhysics_Parabola::sdPhysics_Parabola
================
*/
sdPhysics_Parabola::sdPhysics_Parabola( void ) {
	current.origin.Zero();
	current.velocity.Zero();
	current.time = 0;

	clipModel	= NULL;

	baseOrg.Zero();
	baseVelocity.Zero();
	baseAcceleration.Zero();
	baseAxes.Identity();}

/*
================
sdPhysics_Parabola::~sdPhysics_Parabola
================
*/
sdPhysics_Parabola::~sdPhysics_Parabola( void ) {
	gameLocal.clip.DeleteClipModel( clipModel );
}

/*
================
sdPhysics_Parabola::CalcProperties
================
*/
void sdPhysics_Parabola::CalcProperties( idVec3& origin, idVec3& velocity, int time ) const {
	if ( time < startTime ) {
		time = startTime;
	}

	if ( endTime != -1 ) {
		if ( time > endTime ) {
			time = endTime;
		}
	}

	float elapsed = MS2SEC( time - startTime );

	idVec3 acc = baseAcceleration + gravityVector;

	velocity		= baseVelocity + ( acc * elapsed );
	origin			= baseOrg + ( baseVelocity * elapsed ) + ( 0.5f * acc * Square( elapsed ) );
}

/*
================
sdPhysics_Parabola::Init
================
*/
void sdPhysics_Parabola::Init( const idVec3& origin, const idVec3& velocity, const idVec3& acceleration, const idMat3& axes, int _startTime, int _endTime ) {
	baseAxes			= axes;
	baseOrg				= origin;
	baseVelocity		= velocity;
	baseAcceleration	= acceleration;
	startTime			= _startTime;
	endTime				= _endTime;

	CalcProperties( current.origin, current.velocity, gameLocal.time );
	current.time = gameLocal.time;
}

/*
================
sdPhysics_Parabola::EvaluatePosition
================
*/
idVec3 sdPhysics_Parabola::EvaluatePosition( void ) const {
	idVec3 org, vel;
	CalcProperties( org, vel, gameLocal.time );
	return org;
}

/*
================
sdPhysics_Parabola::Evaluate
================
*/
bool sdPhysics_Parabola::Evaluate( int timeStepMSec, int endTimeMSec ) {
	parabolaPState_t next;

	CalcProperties( current.origin, current.velocity, endTimeMSec - timeStepMSec );
	current.time = endTimeMSec - timeStepMSec;
	CalcProperties( next.origin, next.velocity, endTimeMSec );
	next.time = endTimeMSec;

	CheckWater();

	trace_t collision;
	bool collided = CheckForCollisions( next, collision );

	current = next;

	if ( collided ) {
		if ( CollisionResponse( collision ) ) {
			startTime		= endTimeMSec;
			endTime			= endTimeMSec;
			baseOrg			= current.origin;
			current.velocity.Zero();
			baseVelocity.Zero();
			baseAcceleration.Zero();
		}
	}

	LinkClip();

	return true;
}

/*
================
sdPhysics_Parabola::LinkClip
================
*/
void sdPhysics_Parabola::LinkClip( void ) {
	if ( !clipModel ) {
		return;
	}

	clipModel->Link( gameLocal.clip, self, 0, current.origin, baseAxes );
}

/*
================
sdPhysics_Parabola::SetClipMask
================
*/
void sdPhysics_Parabola::SetContents( int contents, int id ) {
	if ( clipModel ) {
		clipModel->SetContents( contents );
	}
}

/*
================
sdPhysics_Parabola::Evaluate
================
*/
bool sdPhysics_Parabola::CollisionResponse( trace_t& collision ) {
	idEntity* ent = gameLocal.entities[ collision.c.entityNum ];
	if ( !ent ) {
		gameLocal.Warning( "sdPhysics_Parabola::CollisionResponse: collision against an unknown entity" );
		return false;
	}

	impactInfo_t info;
	ent->GetImpactInfo( self, collision.c.id, collision.c.point, &info );

	idVec3 velocity = current.velocity - info.velocity;

	ent->Hit( collision, velocity, self );
	return self->Collide( collision, velocity, -1 );
}

/*
==============
sdPhysics_Parabola::CheckWater
==============
*/
void sdPhysics_Parabola::CheckWater( void ) {
	waterLevel = 0.0f;

	const idBounds& absBounds = GetAbsBounds( -1 );

	const idClipModel* clipModel;
	int count = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS absBounds, CONTENTS_WATER, &clipModel, 1, NULL );
	if ( !count ) {
		return;
	}

	if ( !clipModel->GetNumCollisionModels() ) {
		return;
	}

	idCollisionModel* model = clipModel->GetCollisionModel( 0 );
	int numPlanes = model->GetNumBrushPlanes();
	if ( !numPlanes ) {
		return;
	}

	self->CheckWater( clipModel->GetOrigin(), clipModel->GetAxis(), model );

	const idBounds& modelBounds = model->GetBounds();
	idBounds worldbb;
	worldbb.FromTransformedBounds( GetBounds(), GetOrigin( 0 ), GetAxis( 0 ) );
	bool submerged = worldbb.GetMaxs()[2] < (modelBounds.GetMaxs()[2] + clipModel->GetOrigin().z);

	if ( submerged ) {
		waterLevel = 1.f;
	}
}

/*
================
sdPhysics_Parabola::CheckForCollisions
================
*/
bool sdPhysics_Parabola::CheckForCollisions( parabolaPState_t& next, trace_t& collision ) {
	if ( gameLocal.clip.Translation( CLIP_DEBUG_PARMS collision, current.origin, next.origin, clipModel, baseAxes, clipMask, self ) ) {
		next.origin		= collision.endpos;
		next.velocity	= current.velocity;
		return true;
	}

	return false;
}

/*
================
sdPhysics_Parabola::SetClipModel
================
*/
void sdPhysics_Parabola::SetClipModel( idClipModel* model, float density, int id, bool freeOld ) {
	assert( self );
	assert( model );					// we need a clip model
	assert( model->IsTraceModel() );	// and it should be a trace model

	if ( clipModel != NULL && clipModel != model && freeOld ) {
		gameLocal.clip.DeleteClipModel( clipModel );
	}
	clipModel = model;
	LinkClip();
}

/*
================
sdPhysics_Parabola::GetClipModel
================
*/
idClipModel* sdPhysics_Parabola::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
sdPhysics_Parabola::SetAxis
================
*/
void sdPhysics_Parabola::SetAxis( const idMat3& newAxis, int id ) {
	baseAxes = newAxis;

	LinkClip();
}

/*
================
sdPhysics_Parabola::SetOrigin
================
*/
void sdPhysics_Parabola::SetOrigin( const idVec3& newOrigin, int id ) {
	current.origin = newOrigin;

	LinkClip();
}

/*
================
sdPhysics_Parabola::ApplyNetworkState
================
*/
void sdPhysics_Parabola::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	traceCollection.ForceNextUpdate();
	if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdPhysicsParabolaBroadcastData );

		// update state
		baseAxes				= newData.orientation.ToMat3();
		baseOrg					= newData.position;
		baseVelocity			= newData.velocity;
		baseAcceleration		= newData.acceleration;
		startTime				= newData.startTime;
		endTime					= newData.endTime;

		CalcProperties( current.origin, current.velocity, gameLocal.time );
		LinkClip();
		self->UpdateVisuals();

		return;
	}
}

const float	PARABOLA_ORIGIN_MAX				= 32767.0f;
const int	PARABOLA_ORIGIN_TOTAL_BITS		= 24;
const int	PARABOLA_ORIGIN_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( PARABOLA_ORIGIN_MAX ) ) + 1;
const int	PARABOLA_ORIGIN_MANTISSA_BITS	= PARABOLA_ORIGIN_TOTAL_BITS - 1 - PARABOLA_ORIGIN_EXPONENT_BITS;

const float	PARABOLA_VELOCITY_MAX			= 4000;
const int	PARABOLA_VELOCITY_TOTAL_BITS	= 16;
const int	PARABOLA_VELOCITY_EXPONENT_BITS	= idMath::BitsForInteger( idMath::BitsForFloat( PARABOLA_VELOCITY_MAX ) ) + 1;
const int	PARABOLA_VELOCITY_MANTISSA_BITS	= PARABOLA_VELOCITY_TOTAL_BITS - 1 - PARABOLA_VELOCITY_EXPONENT_BITS;

/*
================
sdPhysics_Parabola::ReadNetworkState
================
*/
void sdPhysics_Parabola::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPhysicsParabolaBroadcastData );

		// write state
		newData.orientation		= msg.ReadDeltaCQuat( baseData.orientation );
		newData.position		= msg.ReadDeltaVector( baseData.position, PARABOLA_ORIGIN_EXPONENT_BITS, PARABOLA_ORIGIN_MANTISSA_BITS );
		newData.velocity		= msg.ReadDeltaVector( baseData.velocity, PARABOLA_VELOCITY_EXPONENT_BITS, PARABOLA_VELOCITY_MANTISSA_BITS );
		newData.acceleration	= msg.ReadDeltaVector( baseData.acceleration, PARABOLA_VELOCITY_EXPONENT_BITS, PARABOLA_VELOCITY_MANTISSA_BITS );
		newData.startTime		= msg.ReadDeltaLong( baseData.startTime );
		newData.endTime			= msg.ReadDeltaLong( baseData.endTime );

		return;
	}
}

/*
================
sdPhysics_Parabola::WriteNetworkState
================
*/
void sdPhysics_Parabola::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdPhysicsParabolaBroadcastData );

		// update state
		newData.orientation		= baseAxes.ToCQuat();
		newData.position		= baseOrg;
		newData.velocity		= baseVelocity;
		newData.acceleration	= baseAcceleration;
		newData.startTime		= startTime;
		newData.endTime			= endTime;

		// write state
		msg.WriteDeltaCQuat( baseData.orientation, newData.orientation );
		msg.WriteDeltaVector( baseData.position, newData.position, PARABOLA_ORIGIN_EXPONENT_BITS, PARABOLA_ORIGIN_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.velocity, newData.velocity, PARABOLA_VELOCITY_EXPONENT_BITS, PARABOLA_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaVector( baseData.acceleration, newData.acceleration, PARABOLA_VELOCITY_EXPONENT_BITS, PARABOLA_VELOCITY_MANTISSA_BITS );
		msg.WriteDeltaLong( baseData.startTime, newData.startTime );
		msg.WriteDeltaLong( baseData.endTime, newData.endTime );

		return;
	}
}

/*
================
sdPhysics_Parabola::CheckNetworkStateChanges
================
*/
bool sdPhysics_Parabola::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdPhysicsParabolaBroadcastData );

		NET_CHECK_FIELD( orientation,	baseAxes.ToCQuat() );
		NET_CHECK_FIELD( position,		baseOrg );
		NET_CHECK_FIELD( velocity,		baseVelocity );
		NET_CHECK_FIELD( acceleration,	baseAcceleration );
		NET_CHECK_FIELD( startTime,		startTime );
		NET_CHECK_FIELD( endTime,		endTime );

		return false;
	}

	return false;
}

/*
================
sdPhysics_Parabola::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* sdPhysics_Parabola::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_BROADCAST ) {
		return new sdPhysicsParabolaBroadcastData();
	}

	return NULL;
}

/*
================
sdPhysics_Parabola::GetBounds
================
*/
const idBounds& sdPhysics_Parabola::GetBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetBounds();
	}
	return idPhysics_Base::GetBounds();
}

/*
================
sdPhysics_Parabola::GetAbsBounds
================
*/
const idBounds& sdPhysics_Parabola::GetAbsBounds( int id ) const {
	if ( clipModel ) {
		return clipModel->GetAbsBounds();
	}
	return idPhysics_Base::GetAbsBounds();
}

