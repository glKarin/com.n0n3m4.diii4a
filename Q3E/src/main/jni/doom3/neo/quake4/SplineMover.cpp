#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "spawner.h"

const idEventDef EV_OnAcceleration( "<onAcceleration>" );
const idEventDef EV_OnDeceleration( "<onDeceleration>" );
const idEventDef EV_OnCruising( "<onCruising>" );

const idEventDef EV_OnStartMoving( "<onStartMoving>" );
const idEventDef EV_OnStopMoving( "<onStopMoving>" );

//=======================================================
//
//	rvPhysics_Spline
//
//=======================================================
CLASS_DECLARATION( idPhysics_Base, rvPhysics_Spline )
	EVENT( EV_PostRestore,			rvPhysics_Spline::Event_PostRestore )
END_CLASS

void splinePState_t::ApplyAccelerationDelta( float timeStepSec ) {
	speed = SignZero(idealSpeed) * Min( idMath::Fabs(idealSpeed), idMath::Fabs(speed) + acceleration * timeStepSec );
}

void splinePState_t::ApplyDecelerationDelta( float timeStepSec ) {
	speed = SignZero(speed) * Max( idMath::Fabs(idealSpeed), idMath::Fabs(speed) - deceleration * timeStepSec );
}

void splinePState_t::UpdateDist( float timeStepSec ) {
	dist += speed * timeStepSec;
}

bool splinePState_t::ShouldAccelerate() const {
	if( Sign(idealSpeed) == Sign(speed) ) {
		return idMath::Fabs(speed) < idMath::Fabs(idealSpeed);
	} else if( !Sign(speed) ) {
		return true;
	}
	
	return false;
}

bool splinePState_t::ShouldDecelerate() const {
	if( Sign(speed) == Sign(idealSpeed) ) {
		return idMath::Fabs(speed) > idMath::Fabs(idealSpeed);
	} else if( !Sign(idealSpeed) ) {
		return true;
	}

	return false;
}

void splinePState_t::Clear() {
	origin.Zero();
	localOrigin.Zero();
	axis.Identity();
	localAxis.Identity();
	speed = 0.0f;
	idealSpeed = 0.0f;
	dist = 0.0f;
	acceleration = 0.0f;
	deceleration = 0.0f;
}

void splinePState_t::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteDeltaVec3( vec3_zero, origin );
	msg.WriteDeltaVec3( vec3_zero, localOrigin );
	msg.WriteDeltaMat3( mat3_identity, axis );
	msg.WriteDeltaMat3( mat3_identity, localAxis );
	msg.WriteDeltaFloat( 0.0f, speed );
	msg.WriteDeltaFloat( 0.0f, idealSpeed );
	msg.WriteDeltaFloat( 0.0f, dist );
	msg.WriteDeltaFloat( 0.0f, acceleration );
	msg.WriteDeltaFloat( 0.0f, deceleration );
}

void splinePState_t::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	origin = msg.ReadDeltaVec3( vec3_zero );
	localOrigin = msg.ReadDeltaVec3( vec3_zero );
	axis = msg.ReadDeltaMat3( mat3_identity );
	localAxis = msg.ReadDeltaMat3( mat3_identity );
	speed = msg.ReadDeltaFloat( 0.0f );
	idealSpeed = msg.ReadDeltaFloat( 0.0f );
	dist = msg.ReadDeltaFloat( 0.0f );
	acceleration = msg.ReadDeltaFloat( 0.0f );
	deceleration = msg.ReadDeltaFloat( 0.0f );
}

void splinePState_t::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( origin );
	savefile->WriteVec3( localOrigin );
	savefile->WriteMat3( axis );
	savefile->WriteMat3( localAxis );
	savefile->WriteFloat( speed );
	savefile->WriteFloat( idealSpeed );
	savefile->WriteFloat( dist );
	savefile->WriteFloat( acceleration );
	savefile->WriteFloat( deceleration );
}

void splinePState_t::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( origin );
	savefile->ReadVec3( localOrigin );
	savefile->ReadMat3( axis );
	savefile->ReadMat3( localAxis );
	savefile->ReadFloat( speed );
	savefile->ReadFloat( idealSpeed );
	savefile->ReadFloat( dist );
	savefile->ReadFloat( acceleration );
	savefile->ReadFloat( deceleration );
}

splinePState_t&	splinePState_t::Assign( const splinePState_t* state ) {
	SIMDProcessor->Memcpy( this, state, sizeof(splinePState_t) );
	return *this;
}

splinePState_t&	splinePState_t::operator=( const splinePState_t& state ) {
	return Assign( &state );
}

splinePState_t&	splinePState_t::operator=( const splinePState_t* state ) {
	return Assign( state );
}

/*
================
rvPhysics_Spline::rvPhysics_Spline
================
*/
rvPhysics_Spline::rvPhysics_Spline( void ) {
	accelDecelStateThread.SetName( "AccelDecel" );
	accelDecelStateThread.SetOwner( this );
	accelDecelStateThread.SetState( "Cruising" );

	clipModel = NULL;

	spline = NULL;
	SetSplineEntity( NULL );

	memset( &pushResults, 0, sizeof(trace_t) );
	pushResults.fraction = 1.0f;

	current.Clear();
	SaveState();
}

/*
================
rvPhysics_Spline::~rvPhysics_Spline
================
*/
rvPhysics_Spline::~rvPhysics_Spline( void ) {
	SAFE_DELETE_PTR( clipModel );

	SAFE_DELETE_PTR( spline );
}

/*
================
rvPhysics_Spline::Save
================
*/
void rvPhysics_Spline::Save( idSaveGame *savefile ) const {

	current.Save( savefile );
	saved.Save( savefile );

	savefile->WriteFloat( splineLength );
	// This spline was retored as NULL, so there's no reason to save it.
	//savefile->WriteInt(spline != NULL ? spline->GetTime( 0 ) : -1 );	// cnicholson: Added unsaved var
	splineEntity.Save( savefile );

	savefile->WriteTrace( pushResults );

	savefile->WriteClipModel( clipModel );

	accelDecelStateThread.Save( savefile );
}

/*
================
rvPhysics_Spline::Restore
================
*/
void rvPhysics_Spline::Event_PostRestore( void ) {

	if( splineEntity.IsValid() ) {
		spline = splineEntity->GetSpline();
	}
}

void rvPhysics_Spline::Restore( idRestoreGame *savefile ) {

	current.Restore( savefile );
	saved.Restore( savefile );

	savefile->ReadFloat( splineLength );
	SAFE_DELETE_PTR( spline );
	splineEntity.Restore( savefile );

	savefile->ReadTrace( pushResults );
	
	savefile->ReadClipModel( clipModel );

	accelDecelStateThread.Restore( savefile, this );
}

/*
================
rvPhysics_Spline::SetClipModel
================
*/
void rvPhysics_Spline::SetClipModel( idClipModel *model, const float density, int id, bool freeOld ) {

	assert( self );
	assert( model );					// we need a clip model

	if ( clipModel && clipModel != model && freeOld ) {
		delete clipModel;
	}
	
	clipModel = model;

	LinkClip();
}

/*
================
rvPhysics_Spline::GetClipModel
================
*/
idClipModel *rvPhysics_Spline::GetClipModel( int id ) const {
	return clipModel;
}

/*
================
rvPhysics_Spline::SetContents
================
*/
void rvPhysics_Spline::SetContents( int contents, int id ) {
	clipModel->SetContents( contents );
}

/*
================
rvPhysics_Spline::GetContents
================
*/
int rvPhysics_Spline::GetContents( int id ) const {
	return clipModel->GetContents();
}

/*
================
rvPhysics_Spline::GetBounds
================
*/
const idBounds &rvPhysics_Spline::GetBounds( int id ) const {
	return clipModel->GetBounds();
}

/*
================
rvPhysics_Spline::GetAbsBounds
================
*/
const idBounds &rvPhysics_Spline::GetAbsBounds( int id ) const {
	return clipModel->GetAbsBounds();
}

/*
================
rvPhysics_Spline::SetSpline
================
*/
void rvPhysics_Spline::SetSpline( idCurve_Spline<idVec3>* spline ) {
	SAFE_DELETE_PTR( this->spline );

	//Keep any left over dist from last spline to minimize hitches
	if( GetSpeed() >= 0.0f ) {
		current.dist = Max( 0.0f, current.dist - splineLength );
	}

	if( !spline ) {
		splineLength = 0.0f;
		return;
	}

	this->spline = spline;
 
	splineLength = spline->GetLengthForTime( spline->GetTime(spline->GetNumValues() - 1) );
	if( GetSpeed() < 0.0f ) {
		current.dist = splineLength - current.dist;
	}

	Activate();
}

/*
================
rvPhysics_Spline::SetSplineEntity
================
*/
void rvPhysics_Spline::SetSplineEntity( idSplinePath* spline ) {
	splineEntity = spline;
	SetSpline( (spline) ? spline->GetSpline() : NULL );
}

/*
================
rvPhysics_Spline::ComputeDecelFromSpline
================
*/
float rvPhysics_Spline::ComputeDecelFromSpline() const {
	// FIXME: merge this in better.  It seems very special case
	float numerator = GetSpeed() * GetSpeed();
	float denomonator = 2.0f * ((GetSpeed() >= 0.0f) ? (splineLength - current.dist) : current.dist);

	assert( denomonator > VECTOR_EPSILON );

	return numerator / denomonator;
}

/*
================
rvPhysics_Spline::SetLinearAcceleration
================
*/
void rvPhysics_Spline::SetLinearAcceleration( const float accel ) {
	current.acceleration = accel;
}

/*
================
rvPhysics_Spline::SetLinearDeceleration
================
*/
void rvPhysics_Spline::SetLinearDeceleration( const float decel ) {
	current.deceleration = decel;
}

/*
================
rvPhysics_Spline::SetSpeed
================
*/
void rvPhysics_Spline::SetSpeed( float speed ) {
	if( IsAtRest() || StoppedMoving() ) {
		current.dist = (speed < 0.0f) ? splineLength - current.dist : current.dist;
	}

	current.idealSpeed = speed;
	Activate();
}

/*
================
rvPhysics_Spline::GetSpeed
================
*/
float rvPhysics_Spline::GetSpeed() const {
	return current.speed;
}

/*
================
rvPhysics_Spline::Evaluate
================
*/
bool rvPhysics_Spline::Evaluate( int timeStepMSec, int endTimeMSec ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	splinePState_t previous = current;

	if( HasValidSpline() ) {
		if( StoppedMoving() ) {
			Rest();
			return false;
		}

		accelDecelStateThread.Execute();

		// FIXME: clean this up
		if( IsAtBeginningOfSpline() || IsAtEndOfSpline() ) {
			current = previous;
			Rest();
			self->ProcessEvent( &EV_DoneMoving );
			
			if( gameLocal.program.GetReturnedBool() ) {
				current.speed = 0.0f;
				return false;
			} else {
				return true;
			}
		}
	
		float currentTime = splineEntity->GetSampledTime ( current.dist );
		if (  currentTime ==  -1.0f ) {
			currentTime = spline->GetTimeForLength( Min(current.dist, splineLength), 0.01f );
		}

		current.axis = spline->GetCurrentFirstDerivative(currentTime).ToAngles().Normalize360().ToMat3();
		current.origin = spline->GetCurrentValue( currentTime );
		current.localOrigin = current.origin;
		current.localAxis = current.axis;
	} else if( self->IsBound() ) {	
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.axis = current.localAxis * masterAxis;
	    current.origin = masterOrigin + current.localOrigin * masterAxis;
	} else {
		Rest();
		return false;
	}

	gameLocal.push.ClipPush( pushResults, self, 0, previous.origin, previous.axis, current.origin, current.axis );
	if( pushResults.fraction < 1.0f ) {
		current = previous;
		LinkClip();
		current.speed = 0.0f;
		return false;
	}

	LinkClip();

	if( StoppedMoving() && !self->IsBound() ) {
		Rest();
		self->ProcessEvent( &EV_DoneMoving );
		return !gameLocal.program.GetReturnedBool();
	}

	return true;
}

/*
================
rvPhysics_Spline::Activate
================
*/
void rvPhysics_Spline::Activate( void ) {
	assert( self );
	self->BecomeActive( TH_PHYSICS );
}

/*
================
rvPhysics_Spline::Rest
================
*/
void rvPhysics_Spline::Rest( void ) {
	assert( self );
	self->BecomeInactive( TH_PHYSICS );
}

/*
================
rvPhysics_Spline::IsAtRest
================
*/
bool rvPhysics_Spline::IsAtRest( void ) const {
	assert( self );
	return !self->IsActive( TH_PHYSICS );
}

/*
================
rvPhysics_Spline::IsAtEndOfSpline
================
*/
bool rvPhysics_Spline::IsAtEndOfSpline( void ) const {
	return current.dist >= splineLength;
}

/*
================
rvPhysics_Spline::IsAtBeginningOfSpline
================
*/
bool rvPhysics_Spline::IsAtBeginningOfSpline( void ) const {
	return current.dist <= 0.0f;
}

/*
================
rvPhysics_Spline::IsPushable
================
*/
bool rvPhysics_Spline::IsPushable( void ) const {
	return !HasValidSpline() && idPhysics_Base::IsPushable();
}

/*
================
rvPhysics_Spline::StartingToMove
================
*/
bool rvPhysics_Spline::StartingToMove( void ) const {
	float firstDeltaSpeed = current.acceleration * MS2SEC(gameLocal.GetMSec());
	return idMath::Fabs(current.idealSpeed) > VECTOR_EPSILON && idMath::Fabs(current.speed) <= firstDeltaSpeed;
}

/*
================
rvPhysics_Spline::StoppedMoving
================
*/
bool rvPhysics_Spline::StoppedMoving( void ) const {
	return idMath::Fabs(current.idealSpeed) < VECTOR_EPSILON && idMath::Fabs(current.speed) < VECTOR_EPSILON;
}

/*
================
rvPhysics_Spline::HasValidSpline
================
*/
bool rvPhysics_Spline::HasValidSpline() const {
	return spline && splineLength > VECTOR_EPSILON;
}

/*
================
rvPhysics_Spline::SaveState
================
*/
void rvPhysics_Spline::SaveState( void ) {
	saved = current;
}

/*
================
rvPhysics_Spline::RestoreState
================
*/
void rvPhysics_Spline::RestoreState( void ) {
	current = saved;

	LinkClip();
}

/*
================
idPhysics::SetOrigin
================
*/
void rvPhysics_Spline::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localOrigin = newOrigin;
	if( self->IsBound() ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;
	}
	else {
		current.origin = current.localOrigin;
	}

	LinkClip();
	Activate();
}

/*
================
idPhysics::SetAxis
================
*/
void rvPhysics_Spline::SetAxis( const idMat3 &newAxis, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localAxis = newAxis;
	if ( self->IsBound() ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.axis = newAxis * masterAxis;
	}
	else {
		current.axis = newAxis;
	}

	LinkClip();
	Activate();
}

/*
================
rvPhysics_Spline::Translate
================
*/
void rvPhysics_Spline::Translate( const idVec3 &translation, int id ) {
	SetOrigin( GetLocalOrigin() + translation );
}

/*
================
rvPhysics_Spline::Rotate
================
*/
void rvPhysics_Spline::Rotate( const idRotation &rotation, int id ) {
	SetAxis( GetLocalAxis() * rotation.ToMat3() );
	SetOrigin( GetLocalOrigin() * rotation );
}

/*
================
rvPhysics_Spline::GetOrigin
================
*/
const idVec3 &rvPhysics_Spline::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
rvPhysics_Spline::GetAxis
================
*/
const idMat3 &rvPhysics_Spline::GetAxis( int id ) const {
	return current.axis;
}

/*
================
rvPhysics_Spline::GetOrigin
================
*/
idVec3 &rvPhysics_Spline::GetOrigin( int id ) {
	return current.origin;
}

/*
================
rvPhysics_Spline::GetAxis
================
*/
idMat3 &rvPhysics_Spline::GetAxis( int id ) {
	return current.axis;
}

/*
================
rvPhysics_Spline::GetLocalOrigin
================
*/
const idVec3 &rvPhysics_Spline::GetLocalOrigin( int id ) const {
	return current.localOrigin;
}

/*
================
rvPhysics_Spline::GetLocalAxis
================
*/
const idMat3 &rvPhysics_Spline::GetLocalAxis( int id ) const {
	return current.localAxis;
}

/*
================
rvPhysics_Spline::GetLocalOrigin
================
*/
idVec3 &rvPhysics_Spline::GetLocalOrigin( int id ) {
	return current.localOrigin;
}

/*
================
rvPhysics_Spline::GetLocalAxis
================
*/
idMat3 &rvPhysics_Spline::GetLocalAxis( int id ) {
	return current.localAxis;
}

/*
================
rvPhysics_Spline::SetMaster
================
*/
void rvPhysics_Spline::SetMaster( idEntity *master, const bool orientated ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	if( master ) {
		if( self->IsBound() ) {
			// transform from world space to master space
			self->GetMasterPosition( masterOrigin, masterAxis );
			current.localOrigin = ( GetOrigin() - masterOrigin ) * masterAxis.Transpose();
			current.localAxis = GetAxis() * masterAxis.Transpose();
		}
	}
}

/*
================
rvPhysics_Spline::ClipTranslation
================
*/
void rvPhysics_Spline::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.TranslationModel( self, results, GetOrigin(), GetOrigin() + translation,
											clipModel, GetAxis(), clipMask,
											model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.Translation( self, results, GetOrigin(), GetOrigin() + translation,
											clipModel, GetAxis(), clipMask, self );
	}
}

/*
================
rvPhysics_Spline::ClipRotation
================
*/
void rvPhysics_Spline::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.RotationModel( self, results, GetOrigin(), rotation,
											clipModel, GetAxis(), clipMask,
											model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.Rotation( self, results, GetOrigin(), rotation,
											clipModel, GetAxis(), clipMask, self );
	}
}

/*
================
rvPhysics_Spline::ClipContents
================
*/
int rvPhysics_Spline::ClipContents( const idClipModel *model ) const {
	if ( model ) {
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		return gameLocal.ContentsModel( self, GetOrigin(), clipModel, GetAxis(), -1,
									model->GetCollisionModel(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		return gameLocal.Contents( self, GetOrigin(), clipModel, GetAxis(), -1, NULL );
// RAVEN END
	}
}

/*
================
rvPhysics_Spline::DisableClip
================
*/
void rvPhysics_Spline::DisableClip( void ) {
	if( clipModel ) {
		clipModel->Disable();
	}
}

/*
================
rvPhysics_Spline::EnableClip
================
*/
void rvPhysics_Spline::EnableClip( void ) {
	if( clipModel ) {
		clipModel->Enable();
	}
}

/*
================
rvPhysics_Spline::UnlinkClip
================
*/
void rvPhysics_Spline::UnlinkClip( void ) {
	if( clipModel ) {
		clipModel->Unlink();
	}
}

/*
================
rvPhysics_Spline::LinkClip
================
*/
void rvPhysics_Spline::LinkClip( void ) {
	if( clipModel ) {
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		clipModel->Link( self, clipModel->GetId(), GetOrigin(), GetAxis() );
// RAVEN END
	}
}

/*
================
rvPhysics_Spline::GetBlockingInfo
================
*/
const trace_t* rvPhysics_Spline::GetBlockingInfo( void ) const {
	return (pushResults.fraction < 1.0f) ? &pushResults : NULL;
}

/*
================
rvPhysics_Spline::GetBlockingEntity
================
*/
idEntity* rvPhysics_Spline::GetBlockingEntity( void ) const {
	return (pushResults.fraction < 1.0f) ? gameLocal.entities[ pushResults.c.entityNum ] : NULL;
}

/*
================
rvPhysics_Spline::WriteToSnapshot
================
*/
void rvPhysics_Spline::WriteToSnapshot( idBitMsgDelta &msg ) const {
	current.WriteToSnapshot( msg );
}

/*
================
rvPhysics_Spline::ReadFromSnapshot
================
*/
void rvPhysics_Spline::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	current.ReadFromSnapshot( msg );

	LinkClip();
}

CLASS_STATES_DECLARATION( rvPhysics_Spline )
	STATE( "Accelerating",		rvPhysics_Spline::State_Accelerating )
	STATE( "Decelerating",		rvPhysics_Spline::State_Decelerating )
	STATE( "Cruising",			rvPhysics_Spline::State_Cruising )
END_CLASS_STATES

/*
================
rvPhysics_Spline::State_Accelerating
================
*/
stateResult_t rvPhysics_Spline::State_Accelerating( const stateParms_t& parms ) {
	stateResult_t returnResult = SRESULT_WAIT;

	if( !current.ShouldAccelerate() ) {
		accelDecelStateThread.SetState( current.ShouldDecelerate() ? "Decelerating" : "Cruising" );
		return SRESULT_DONE;
	}

	if( !parms.stage ) {
		if( StartingToMove() ) {
			self->ProcessEvent( &EV_OnStartMoving );
		}
		self->ProcessEvent( &EV_OnAcceleration );

		returnResult = SRESULT_STAGE( parms.stage + 1 );
	}

	float timeStepSec = MS2SEC( gameLocal.GetMSec() );
	current.ApplyAccelerationDelta( timeStepSec );
	current.UpdateDist( timeStepSec );

	return returnResult;
}

/*
================
rvPhysics_Spline::State_Decelerating
================
*/
stateResult_t rvPhysics_Spline::State_Decelerating( const stateParms_t& parms ) {
	if( !current.ShouldDecelerate() ) {
		accelDecelStateThread.SetState( current.ShouldAccelerate() ? "Accelerating" : "Cruising" );
		return SRESULT_DONE;
	}

	float timeStepSec = MS2SEC( gameLocal.GetMSec() );
	current.ApplyDecelerationDelta( timeStepSec );
	current.UpdateDist( timeStepSec );

	if( !parms.stage ) {
		self->ProcessEvent( &EV_OnDeceleration );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	if( StoppedMoving() ) {
		self->ProcessEvent( &EV_OnStopMoving );
	}

	return SRESULT_WAIT;
}

/*
================
rvPhysics_Spline::State_Cruising
================
*/
stateResult_t rvPhysics_Spline::State_Cruising( const stateParms_t& parms ) {
	if( current.ShouldAccelerate() ) {
		accelDecelStateThread.SetState( "Accelerating" );
		return SRESULT_DONE;
	} else if( current.ShouldDecelerate() ) {
		accelDecelStateThread.SetState( "Decelerating" );
		return SRESULT_DONE;
	}

	current.UpdateDist( MS2SEC(gameLocal.GetMSec()) );

	if( !parms.stage ) {
		self->ProcessEvent( &EV_OnCruising );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	return SRESULT_WAIT;
}



const idEventDef EV_SetSpline( "setSpline", "E" );

const idEventDef EV_SetAccel( "setAccel", "f" );
const idEventDef EV_SetDecel( "setDecel", "f" );

const idEventDef EV_SetSpeed( "setSpeed", "f" );
const idEventDef EV_GetSpeed( "getSpeed", "", 'f' );

const idEventDef EV_TramCar_SetIdealSpeed( "setIdealSpeed", "f" );
const idEventDef EV_TramCar_GetIdealSpeed( "getIdealSpeed", "", 'f' );

const idEventDef EV_TramCar_ApplySpeedScale( "applySpeedScale", "f" );

const idEventDef EV_GetCurrentTrackInfo( "getCurrentTrackInfo", "", 's' );
const idEventDef EV_GetTrackInfo( "getTrackInfo", "e", 's' );

const idEventDef EV_DoneMoving( "<doneMoving>", "", 'd' );
const idEventDef EV_StartSoundPeriodic( "<startSoundPeriodic>", "sddd" );

idLinkList<rvSplineMover> rvSplineMover::splineMovers;

//=======================================================
//
//	rvSplineMover
//
//=======================================================
CLASS_DECLARATION( idAnimatedEntity, rvSplineMover )
	EVENT( EV_PostSpawn,				rvSplineMover::Event_PostSpawn )
	EVENT( EV_Activate,					rvSplineMover::Event_Activate )
	EVENT( EV_SetSpline,				rvSplineMover::Event_SetSpline )
	EVENT( EV_SetAccel,					rvSplineMover::Event_SetAcceleration )
	EVENT( EV_SetDecel,					rvSplineMover::Event_SetDeceleration )
	EVENT( EV_SetSpeed,					rvSplineMover::Event_SetSpeed )
	EVENT( EV_GetSpeed,					rvSplineMover::Event_GetSpeed )
	EVENT( EV_Thread_SetCallback,		rvSplineMover::Event_SetCallBack )
	EVENT( EV_DoneMoving,				rvSplineMover::Event_DoneMoving )
	EVENT( EV_GetSplineEntity,			rvSplineMover::Event_GetSpline )
	EVENT( EV_GetCurrentTrackInfo,		rvSplineMover::Event_GetCurrentTrackInfo )
	EVENT( EV_GetTrackInfo,				rvSplineMover::Event_GetTrackInfo )
	EVENT( EV_TramCar_SetIdealSpeed,	rvSplineMover::Event_SetIdealSpeed )
	EVENT( EV_TramCar_GetIdealSpeed,	rvSplineMover::Event_GetIdealSpeed )
	EVENT( EV_TramCar_ApplySpeedScale,	rvSplineMover::Event_ApplySpeedScale )
	EVENT( EV_OnAcceleration,			rvSplineMover::Event_OnAcceleration )
	EVENT( EV_OnDeceleration,			rvSplineMover::Event_OnDeceleration )
	EVENT( EV_OnCruising,				rvSplineMover::Event_OnCruising )
	EVENT( EV_OnStartMoving,			rvSplineMover::Event_OnStartMoving )
	EVENT( EV_OnStopMoving,				rvSplineMover::Event_OnStopMoving )
	EVENT( EV_StartSoundPeriodic,		rvSplineMover::Event_StartSoundPeriodic )
	EVENT( EV_PartBlocked,				rvSplineMover::Event_PartBlocked )
END_CLASS

/*
================
rvSplineMover::Spawn
================
*/
void rvSplineMover::Spawn() {
	waitThreadId = -1;

	physicsObj.SetSelf( this );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	physicsObj.SetClipModel( new idClipModel(GetPhysics()->GetClipModel()), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
	physicsObj.SetContents( spawnArgs.GetBool("solid", "1") ? CONTENTS_SOLID : 0 );
	physicsObj.SetClipMask( spawnArgs.GetBool("solidClip") ? CONTENTS_SOLID : 0 );
	physicsObj.SetLinearVelocity( GetPhysics()->GetLinearVelocity() );
	physicsObj.SetLinearAcceleration( spawnArgs.GetFloat("accel", "50") );
	physicsObj.SetLinearDeceleration( spawnArgs.GetFloat("decel", "50") );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );

	SetPhysics( &physicsObj );

	AddSelfToGlobalList();

	// This is needed so we get sorted correctly
	BecomeInactive( TH_PHYSICS );
	BecomeActive( TH_PHYSICS );

	PlayAnim( ANIMCHANNEL_ALL, "idle", 0 );

	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
rvSplineMover::~rvSplineMover
================
*/
rvSplineMover::~rvSplineMover() {
	RemoveSelfFromGlobalList();
	SetPhysics( NULL );
}

/*
================
rvSplineMover::SetSpeed
================
*/
void rvSplineMover::SetSpeed( float newSpeed ) {
	physicsObj.SetSpeed( newSpeed );
}

/*
================
rvSplineMover::GetSpeed
================
*/
float rvSplineMover::GetSpeed() const {
	return physicsObj.GetSpeed();
}

/*
================
rvSplineMover::SetIdealSpeed
================
*/
void rvSplineMover::SetIdealSpeed( float newIdealSpeed ) {
	idealSpeed = newIdealSpeed;
	SetSpeed( newIdealSpeed );
}

/*
================
rvSplineMover::GetIdealSpeed
================
*/
float rvSplineMover::GetIdealSpeed() const {
	return idealSpeed;
}

/*
================
rvSplineMover::SetSpline
================
*/
void rvSplineMover::SetSpline( idSplinePath* spline ) {
	physicsObj.SetSplineEntity( spline );
	CheckSplineForOverrides( physicsObj.GetSpline(), &spline->spawnArgs );
}

/*
================
rvSplineMover::GetSpline
================
*/
const idSplinePath* rvSplineMover::GetSpline() const {
	return physicsObj.GetSplineEntity();
}

/*
================
rvSplineMover::GetSpline
================
*/
idSplinePath* rvSplineMover::GetSpline() {
	return physicsObj.GetSplineEntity();
}

/*
================
rvSplineMover::SetAcceleration
================
*/
void rvSplineMover::SetAcceleration( float accel ) {
	physicsObj.SetLinearAcceleration( accel );
}

/*
================
rvSplineMover::SetDeceleration
================
*/
void rvSplineMover::SetDeceleration( float decel ) {
	physicsObj.SetLinearDeceleration( decel );
}

/*
================
rvSplineMover::CheckSplineForOverrides
================
*/
void rvSplineMover::CheckSplineForOverrides( const idCurve_Spline<idVec3>* spline, const idDict* args ) {
	if( !spline || !args ) {
		return;
	}

	int endSpline = args->GetInt( "end_spline" );
	if( endSpline && Sign(endSpline) == Sign(GetSpeed()) ) {
		physicsObj.SetLinearDeceleration( physicsObj.ComputeDecelFromSpline() );
		SetIdealSpeed( 0.0f );
	}
}

/*
================
rvSplineMover::RestoreFromOverrides
================
*/
void rvSplineMover::RestoreFromOverrides( const idDict* args ) {
	if( !args ) {
		return;
	}

	physicsObj.SetLinearDeceleration( args->GetFloat("decel") );
}

/*
================
rvSplineMover::PlayAnim
================
*/
int rvSplineMover::PlayAnim( int channel, const char* animName, int blendFrames ) {
	int animIndex = GetAnimator()->GetAnim( animName );
	if( !animIndex ) {
		return 0;
	}

	GetAnimator()->PlayAnim( channel, animIndex, gameLocal.GetTime(), FRAME2MS(blendFrames) );
	return GetAnimator()->CurrentAnim( channel )->Length();
}

/*
================
rvSplineMover::CycleAnim
================
*/
void rvSplineMover::CycleAnim( int channel, const char* animName, int blendFrames ) {
	int animIndex = GetAnimator()->GetAnim( animName );
	if( !animIndex ) {
		return;
	}

	GetAnimator()->CycleAnim( channel, animIndex, gameLocal.GetTime(), FRAME2MS(blendFrames) );
}

/*
================
rvSplineMover::ClearChannel
================
*/
void rvSplineMover::ClearChannel( int channel, int clearFrame ) {
	GetAnimator()->Clear( channel, gameLocal.GetTime(), FRAME2MS(clearFrame) );
}

/*
================
rvSplineMover::PreBind
================
*/
void rvSplineMover::PreBind() {
	idAnimatedEntity::PreBind();

	SetSpline( NULL );
}

/*
================
rvSplineMover::Save
================
*/
void rvSplineMover::Save( idSaveGame *savefile ) const {
	savefile->WriteStaticObject( physicsObj );
	savefile->WriteFloat( idealSpeed );
	savefile->WriteInt( waitThreadId );
}

/*
================
rvSplineMover::Restore
================
*/
void rvSplineMover::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadFloat( idealSpeed );
	savefile->ReadInt( waitThreadId );

	AddSelfToGlobalList();
}

/*
================
rvSplineMover::WriteToSnapshot
================
*/
void rvSplineMover::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
}

/*
================
rvSplineMover::ReadFromSnapshot
================
*/
void rvSplineMover::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
}

/*
================
rvSplineMover::AddSelfToGlobalList
================
*/
void rvSplineMover::AddSelfToGlobalList() {
	splineMoverNode.SetOwner( this );

	if( !InGlobalList() ) {
		splineMoverNode.AddToEnd( splineMovers );
	}
}

/*
================
rvSplineMover::RemoveSelfFromGlobalList
================
*/
void rvSplineMover::RemoveSelfFromGlobalList() {
	splineMoverNode.Remove();
}

/*
================
rvSplineMover::InGlobalList
================
*/
bool rvSplineMover::InGlobalList() const {
	return splineMoverNode.InList();
}

/*
================
rvSplineMover::WhosVisible
================
*/
bool rvSplineMover::WhosVisible( const idFrustum& frustum, idList<rvSplineMover*>& list ) const {
	list.Clear();

	if( !frustum.IsValid() ) {
		return false;
	}

	for( rvSplineMover* node = splineMovers.Next(); node; node = node->splineMoverNode.Next() ) {
		if( node == this ) {
			continue;
		}

		if( frustum.IntersectsBounds(node->GetPhysics()->GetAbsBounds()) ) {
			list.AddUnique( node );
		}
	}

	return list.Num() > 0;
}

/*
================
rvSplineMover::GetTrackInfo
================
*/
idStr rvSplineMover::GetTrackInfo( const idSplinePath* track ) const {
	if( !track ) {
		return idStr( "" );
	}

	idStr info( track->GetName() );
	return info.Mid( info.Last('_') - 1, 1 );
}

/*
================
rvSplineMover::ConvertToMover
================
*/
rvSplineMover* rvSplineMover::ConvertToMover( idEntity* mover ) const {
	return mover && mover->IsType(rvSplineMover::Type) ? static_cast<rvSplineMover*>(mover) : NULL;
}

/*
================
rvSplineMover::ConvertToSplinePath
================
*/
idSplinePath* rvSplineMover::ConvertToSplinePath( idEntity* spline ) const {
	return (spline && spline->IsType(idSplinePath::GetClassType())) ? static_cast<idSplinePath*>(spline) : NULL;
}

/*
================
rvSplineMover::PreDoneMoving
================
*/
void rvSplineMover::PreDoneMoving() {
	if( waitThreadId >= 0 ) {
		idThread::ObjectMoveDone( waitThreadId, this );
		waitThreadId = -1;
	}

	RestoreFromOverrides( &spawnArgs );
}

/*
================
rvSplineMover::PostDoneMoving
================
*/
void rvSplineMover::PostDoneMoving() {
	CallScriptEvents( physicsObj.GetSplineEntity(), "call_doneMoving", this );
}

/*
==============
rvSplineMover::CallScriptEvents
==============
*/
// FIXME: very similier code is in the spawner...if possible try and make one function for both to call
void rvSplineMover::CallScriptEvents( const idSplinePath* spline, const char* prefixKey, idEntity* parm ) {
	if( !spline || !prefixKey || !prefixKey[0] ) {
		return;
	}

	rvScriptFuncUtility func;
	for( const idKeyValue* kv = spline->spawnArgs.MatchPrefix(prefixKey); kv; kv = spline->spawnArgs.MatchPrefix(prefixKey, kv) ) {
		if( !kv->GetValue().Length() ) {
			continue;
		}

		if( func.Init(kv->GetValue()) <= SFU_ERROR ) {
			continue;
		}

		func.InsertEntity( spline, 0 );
		func.InsertEntity( parm, 1 );
		func.CallFunc( &spawnArgs );
	}
}

/*
================
rvSplineMover::Event_PostSpawn
================
*/
void rvSplineMover::Event_PostSpawn() {
	idEntityPtr<idEntity> target;
	for( int ix = targets.Num() - 1; ix >= 0; --ix ) {
		target = targets[ix];

		if( target.IsValid() && target->IsType(idSplinePath::GetClassType()) ) {
			SetSpline( static_cast<idSplinePath*>(target.GetEntity()) );
			break;
		}
	}

	SetIdealSpeed( spawnArgs.GetBool("waitForTrigger") ? 0.0f : spawnArgs.GetFloat("speed", "50") );
}

/*
===============
rvSplineMover::Event_PartBlocked
===============
*/
void rvSplineMover::Event_PartBlocked( idEntity *blockingEntity ) {
	assert( blockingEntity );

	float damage = spawnArgs.GetFloat( "damage" );
	if( damage > 0.0f ) {
		blockingEntity->Damage( this, this, vec3_origin, "damage_moverCrush", damage, INVALID_JOINT );
	}
	if( g_debugMover.GetBool() ) {
		gameLocal.Printf( "%d: '%s' blocked by '%s'\n", gameLocal.GetTime(), GetName(), blockingEntity->GetName() );
	}
}

/*
================
rvSplineMover::Event_SetSpline
================
*/
void rvSplineMover::Event_SetSpline( idEntity* spline ) {
	SetSpline( ConvertToSplinePath(spline) );
}

/*
================
rvSplineMover::Event_GetSpline
================
*/
void rvSplineMover::Event_GetSpline() {
	idThread::ReturnEntity( GetSpline() );
}

/*
================
rvSplineMover::Event_SetAcceleration
================
*/
void rvSplineMover::Event_SetAcceleration( float accel ) {
	SetAcceleration( accel );
}

/*
================
rvSplineMover::Event_SetDeceleration
================
*/
void rvSplineMover::Event_SetDeceleration( float decel ) {
	SetDeceleration( decel );
}

/*
================
rvSplineMover::Event_SetSpeed
================
*/
void rvSplineMover::Event_SetSpeed( float speed ) {
	SetIdealSpeed( speed );
	SetSpeed( speed );
}

/*
================
rvSplineMover::Event_GetSpeed
================
*/
void rvSplineMover::Event_GetSpeed() {
	idThread::ReturnFloat( GetSpeed() );
}

/*
================
rvSplineMover::Event_SetIdealSpeed
================
*/
void rvSplineMover::Event_SetIdealSpeed( float speed ) {
	SetIdealSpeed( speed );
}

/*
================
rvSplineMover::Event_GetIdealSpeed
================
*/
void rvSplineMover::Event_GetIdealSpeed() {
	idThread::ReturnFloat( GetIdealSpeed() );
}

/*
================
rvSplineMover::Event_ApplySpeedScale
================
*/
void rvSplineMover::Event_ApplySpeedScale( float scale ) {
	SetIdealSpeed( spawnArgs.GetFloat("speed", "50") * scale );
}

/*
================
rvSplineMover::Event_SetCallBack
================
*/
void rvSplineMover::Event_SetCallBack() {
	if( waitThreadId >= 0 ) {
		idThread::ReturnInt( false );
	}

	waitThreadId = idThread::CurrentThreadNum();
	idThread::ReturnInt( true );
}

/*
================
rvSplineMover::Event_DoneMoving
================
*/
void rvSplineMover::Event_DoneMoving() {
	PreDoneMoving();
	PostDoneMoving();

	idThread::ReturnInt( !physicsObj.HasValidSpline() );
}

/*
================
rvSplineMover::Event_GetCurrentTrackInfo
================
*/
void rvSplineMover::Event_GetCurrentTrackInfo() {
	Event_GetTrackInfo( physicsObj.GetSplineEntity() );
}

/*
================
rvSplineMover::Event_GetTrackInfo
================
*/
void rvSplineMover::Event_GetTrackInfo( idEntity* track ) {
	idThread::ReturnString( GetTrackInfo(ConvertToSplinePath(track)) );
}

/*
================
rvSplineMover::Event_Activate
================
*/
void rvSplineMover::Event_Activate( idEntity* activator ) {
	// This is for my special case in tram1b

	//if( physicsObj.StoppedMoving() ) {
	//	SetIdealSpeed( spawnArgs.GetFloat("speed", "50") );
	//}
}

/*
================
rvSplineMover::Event_OnAcceleration
================
*/
void rvSplineMover::Event_OnAcceleration() {
	StartSound( "snd_accel", SND_CHANNEL_ANY, 0, false, NULL );
}

/*
================
rvSplineMover::Event_OnDeceleration
================
*/
void rvSplineMover::Event_OnDeceleration() {
	StartSound( "snd_decel", SND_CHANNEL_ANY, 0, false, NULL );
}

/*
================
rvSplineMover::Event_OnCruising
================
*/
void rvSplineMover::Event_OnCruising() {
	idVec2 range( spawnArgs.GetVec2("noisePeriodRange") * idMath::M_SEC2MS );
	if( !EventIsPosted(&EV_StartSoundPeriodic) && range.Length() > VECTOR_EPSILON ) {
		ProcessEvent( &EV_StartSoundPeriodic, "snd_noise", (int)SND_CHANNEL_ANY, (int)range[0], (int)range[1] );
	}
}

/*
================
rvSplineMover::Event_OnStopMoving
================
*/
void rvSplineMover::Event_OnStopMoving() {
	StopSound( SND_CHANNEL_ANY, false );
	CancelEvents( &EV_StartSoundPeriodic );
}

/*
================
rvSplineMover::Event_OnStartMoving
================
*/
void rvSplineMover::Event_OnStartMoving() {
}

/*
================
rvSplineMover::Event_StartSoundPeriodic
================
*/
void rvSplineMover::Event_StartSoundPeriodic( const char* sndKey, const s_channelType channel, int minDelay, int maxDelay ) {
	CancelEvents( &EV_StartSoundPeriodic );

	if( physicsObj.StoppedMoving() ) {
		return;
	}

	int length;
	StartSound( sndKey, channel, 0, false, &length );

	PostEventMS( &EV_StartSoundPeriodic, Max(rvRandom::irand(minDelay, maxDelay), length), sndKey, (int)channel, minDelay, maxDelay );
}


const idEventDef EV_TramCar_RadiusDamage( "<tramCar_radiusDamage>", "vs" );
const idEventDef EV_TramCar_SetIdealTrack( "setIdealTrack", "s" );
const idEventDef EV_TramCar_DriverSpeak( "driverSpeak", "s", 'e' );
const idEventDef EV_TramCar_GetDriver( "getDriver", "", 'E' );

const idEventDef EV_TramCar_OpenDoors( "openDoors" );
const idEventDef EV_TramCar_CloseDoors( "closeDoors" );

//=======================================================
//
//	rvTramCar
//
//=======================================================
CLASS_DECLARATION( rvSplineMover, rvTramCar )
	EVENT( EV_TramCar_DriverSpeak,		rvTramCar::Event_DriverSpeak )
	EVENT( EV_TramCar_GetDriver,		rvTramCar::Event_GetDriver )
	EVENT( EV_Activate,					rvTramCar::Event_Activate )
	EVENT( EV_TramCar_RadiusDamage,		rvTramCar::Event_RadiusDamage )
	EVENT( EV_TramCar_SetIdealTrack,	rvTramCar::Event_SetIdealTrack )
	EVENT( EV_OnStartMoving,			rvTramCar::Event_OnStartMoving )
	EVENT( EV_OnStopMoving,				rvTramCar::Event_OnStopMoving )
	EVENT( EV_TramCar_OpenDoors,		rvTramCar::Event_OpenDoors )
	EVENT( EV_TramCar_CloseDoors,		rvTramCar::Event_CloseDoors )
	EVENT( AI_SetHealth,				rvTramCar::Event_SetHealth )
END_CLASS

/*
================
rvTramCar::Spawn
================
*/
void rvTramCar::Spawn() {
	RegisterStateThread( idealTrackStateThread, "IdealTrack" );
	RegisterStateThread( speedSoundEffectsStateThread, "SpeedSoundEffects" );

	numTracksOnMap = gameLocal.world->spawnArgs.GetInt( "numTramCarTracks", "1" );

	idStr track;
	if ( numTracksOnMap == 1 ) {
		track += ConvertToTrackLetter( numTracksOnMap - 1 );
	} else {
		track = spawnArgs.RandomPrefix( "idealTrack", gameLocal.random, "0" );
	}
	Event_SetIdealTrack( track.c_str() );
	idealTrackStateThread.SetState( GetIdealTrack() < 0 ? "RandomTrack" : "AssignedTrack" );

	SpawnDriver( "def_driver" );
	SpawnWeapons( "def_weapon" );
	SpawnOccupants( "def_occupant" );
	SpawnDoors();

	float dNear = 0.0f, dFar = 0.0f, dLeft = 0.0f, dUp = 0.0f;
	const char* frustumKey = spawnArgs.GetString( "collisionFov" );
	if( frustumKey[0] ) {
		sscanf( frustumKey, "%f %f %f %f", &dNear, &dFar, &dLeft, &dUp );
		collisionFov.SetSize( dNear, dFar, dLeft, dUp );
	}

	fl.takedamage = health > 0;

	BecomeActive( TH_THINK );

	SetSpline( NULL );
}

/*
================
rvTramCar::SpawnDriver
================
*/
rvTramCar::~rvTramCar() {
	SAFE_REMOVE( driver );
	occupants.RemoveContents( true );
	weapons.RemoveContents( true );

	SAFE_REMOVE( leftDoor );
	SAFE_REMOVE( rightDoor );
}

/*
================
rvTramCar::SpawnDriver
================
*/
void rvTramCar::SpawnDriver( const char* driverKey ) {
	driver = (idAI*)SpawnPart( spawnArgs.GetString(driverKey), "def_occupant" );
}

/*
================
rvTramCar::SpawnWeapons
================
*/
void rvTramCar::SpawnWeapons( const char* partKey ) {
	idEntityPtr<rvVehicle>	weapon;

	for( const idKeyValue* kv = spawnArgs.MatchPrefix(partKey); kv; kv = spawnArgs.MatchPrefix(partKey, kv) ) {
		if( !kv->GetValue().Length() ) {
			continue;
		}

		weapon = (rvVehicle*)SpawnPart( kv->GetValue().c_str(), partKey );
		weapon->IdleAnim( ANIMCHANNEL_LEGS, "idle", 0 );
		//if( weapon->GetAnimator()->GetAnim("toFire") && weapon->GetAnimator()->GetAnim("toIdle") ) {
			weapons.AddUnique( weapon );
		//}
	}
}

/*
================
rvTramCar::SpawnOccupants
================
*/
void rvTramCar::SpawnOccupants( const char* partKey ) {
	idEntityPtr<idAI> occupant;

	for( const idKeyValue* kv = spawnArgs.MatchPrefix(partKey); kv; kv = spawnArgs.MatchPrefix(partKey, kv) ) {
		if( !kv->GetValue().Length() ) {
			continue;
		}
		
		occupant = (idAI*)SpawnPart( kv->GetValue().c_str(), partKey );
		occupants.AddUnique( occupant );
	}
}

/*
================
rvTramCar::SpawnPart
================
*/
idEntity* rvTramCar::SpawnPart( const char* partDefName, const char* subPartDefName ) {
	idEntity*		part = NULL;
	idDict			entityDef;
	const idDict*	info = gameLocal.FindEntityDefDict( partDefName, false );
	if( !info ) {
		return NULL;
	}

	const idDict*	def = gameLocal.FindEntityDefDict( info->GetString(subPartDefName), false );
	if( !def ) {
		return NULL;
	}

	entityDef.Copy( *def );

	entityDef.SetBool( "trigger", spawnArgs.GetBool("waitForTrigger") );
	entityDef.SetVector( "origin", GetPhysics()->GetOrigin() + info->GetVector("spawn_offset") * GetPhysics()->GetAxis() );
	entityDef.SetFloat( "angle", info->GetInt("spawn_facing_offset") + spawnArgs.GetFloat("angle") );
	entityDef.Set( "bind", GetName() );
	gameLocal.SpawnEntityDef( entityDef, &part ); 

	return part;
}

/*
================
rvTramCar::SpawnDoors
================
*/
void rvTramCar::SpawnDoors() {
	leftDoor = SpawnDoor( "clipModel_doorLeft" );
	rightDoor = SpawnDoor( "clipModel_doorRight" );
}

/*
================
rvTramCar::SpawnDoors
================
*/
idMover* rvTramCar::SpawnDoor( const char* key ) {
	idDict				args;
	const idDict*		dict = NULL;
	idMover*			outer = NULL;
	idMover*			inner = NULL;

	dict = gameLocal.FindEntityDefDict( spawnArgs.GetString(key), false );
	if( !dict ) {
		return NULL;
	}

	args.Set( "open_rotation", dict->GetString("open_rotation") );
	args.Set( "close_rotation", dict->GetString("close_rotation") );

	args.Set( "open_dir", dict->GetString("open_dir") );
	args.Set( "close_dir", dict->GetString("close_dir") );

	args.Set( "distExt", dict->GetString("distExt") );

	args.SetVector( "origin", GetPhysics()->GetOrigin() + dict->GetVector("offset") * GetPhysics()->GetAxis() );
	args.SetMatrix( "rotation", GetPhysics()->GetAxis() );
	args.Set( "clipModel", dict->GetString("clipModel") );
	args.Set( "bind", GetName() );
	outer = gameLocal.SpawnSafeEntityDef<idMover>( "func_mover", &args );
	assert( outer );

	args.Set( "clipModel", dict->GetString("clipModelExt") );
	args.Set( "bind", outer->GetName() );
	inner = gameLocal.SpawnSafeEntityDef<idMover>( "func_mover", &args );

	return inner;
}

/*
================
rvTramCar::Think
================
*/
void rvTramCar::Think() {
	RunPhysics();

	MidThink();

	Present();
}

/*
================
rvTramCar::MidThink
================
*/
void rvTramCar::MidThink() {
	if( !physicsObj.StoppedMoving() ) {
		LookAround();
		TouchTriggers();
		speedSoundEffectsStateThread.Execute();
	}
}

/*
================
rvTramCar::RegisterStateThread
================
*/
void rvTramCar::RegisterStateThread( rvStateThread& stateThread, const char* name ) {
	stateThread.SetName( name );
	stateThread.SetOwner( this );
}

/*
================
rvTramCar::SetIdealTrack
================
*/
void rvTramCar::SetIdealTrack( int track ) {
	idealTrack = track;
}

/*
================
rvTramCar::GetIdealTrack
================
*/
int rvTramCar::GetIdealTrack() const {
	return idealTrack;
}

/*
================
rvTramCar::AddDamageEffect
================
*/
void rvTramCar::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor ) {
	rvSplineMover::AddDamageEffect( collision, velocity, damageDefName, inflictor );

	if( GetDamageScale() >= 0.5f && rvRandom::flrand() <= spawnArgs.GetFloat("damageEffectFreq", "0.25") ) {
		idVec3 center = GetPhysics()->GetAbsBounds().GetCenter();
		idVec3 v = (center - collision.endpos).ToNormal();
		float dot = v * collision.c.normal;
		PlayEffect( (dot > 0.0f) ? "fx_damage_internal" : "fx_damage_external", collision.endpos, collision.c.normal.ToMat3(2) );
	}
}

/*
================
rvTramCar::Damage
================
*/
void rvTramCar::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	if( attacker && GetPhysics()->GetAbsBounds().Contains(attacker->GetPhysics()->GetAbsBounds()) ) {
		return;
	}

	if( attacker && g_debugVehicle.GetInteger() == 3 ) {
		gameLocal.Printf( "Was damaged by %s.  Current damage scale: %f\n", attacker->GetName(), GetDamageScale() );
	}

	rvSplineMover::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
}

/*
================
rvTramCar::Killed
================
*/
void rvTramCar::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	idVec3 center = GetPhysics()->GetAbsBounds().GetCenter();

	fl.takedamage = false;

	StopSound( SND_CHANNEL_ANY, false );

	//Calling gameLocal's version because we don't want to get bound and hidden
	gameLocal.PlayEffect( gameLocal.GetEffect( spawnArgs, "fx_explode" ), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	PostEventMS( &EV_TramCar_RadiusDamage, 0, center, spawnArgs.GetString("def_damage_explode") );

	//TODO: think about giving rigid body physics and have it fall from track
	Hide();
	PostEventMS( &EV_Remove, 0 );
}

/*
================
rvTramCar::GetDamageScale
================
*/
float rvTramCar::GetDamageScale() const {
	return 1.0f - GetHealthScale();
}

/*
================
rvTramCar::GetDamageScale
================
*/
float rvTramCar::GetHealthScale() const {
	float spawnHealth = spawnArgs.GetFloat( "health" );
	return ( idMath::Fabs(spawnHealth) <= VECTOR_EPSILON ) ? 1.0f : (health / spawnHealth);
}

/*
================
rvTramCar::Save
================
*/
void rvTramCar::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( idealTrack );
//	savefile->WriteString( idealTrackTag.c_str() ); // cnicholson: FIXME: This has a comment of HACK in the .h file... Not sure how to write a idStr yet

	savefile->WriteFrustum( collisionFov );

	idealTrackStateThread.Save( savefile );
	speedSoundEffectsStateThread.Save( savefile );

	driver.Save( savefile );

	savefile->WriteInt( occupants.Num() );
	for( int ix = occupants.Num() - 1; ix >= 0; --ix ) {
		occupants[ix].Save( savefile );
	}

	savefile->WriteInt( weapons.Num() );
	for( int ix = weapons.Num() - 1; ix >= 0; --ix ) {
		weapons[ix].Save( savefile );
	}

	savefile->WriteInt( numTracksOnMap );

	leftDoor.Save ( savefile );
	rightDoor.Save ( savefile );
}

/*
================
rvTramCar::Restore
================
*/
void rvTramCar::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( idealTrack );

	savefile->ReadFrustum( collisionFov );

	idealTrackStateThread.Restore( savefile, this );
	speedSoundEffectsStateThread.Restore( savefile, this );

	driver.Restore( savefile );

	int num = 0;
	savefile->ReadInt( num );
	occupants.SetNum( num );
	for( int ix = occupants.Num() - 1; ix >= 0; --ix ) {
		occupants[ix].Restore( savefile );
	}

	savefile->ReadInt( num );
	weapons.SetNum( num );
	for( int ix = weapons.Num() - 1; ix >= 0; --ix ) {
		weapons[ix].Restore( savefile );
	}

	savefile->ReadInt( numTracksOnMap );

	leftDoor.Restore ( savefile );
	rightDoor.Restore ( savefile );
}

/*
================
rvTramCar::WriteToSnapshot
================
*/
void rvTramCar::WriteToSnapshot( idBitMsgDelta &msg ) const {
}

/*
================
rvTramCar::ReadFromSnapshot
================
*/
void rvTramCar::ReadFromSnapshot( const idBitMsgDelta &msg ) {
}

/*
================
rvTramCar::HeadTowardsIdealTrack
================
*/
void rvTramCar::HeadTowardsIdealTrack() {
	idSplinePath*	ideal = FindSplineToIdealTrack( GetSpline() );
	if( !ideal ) {
		ideal = GetRandomSpline(GetSpline());
	}

	SetSpline( ideal );
}

/*
================
rvTramCar::FindSplineToTrack
================
*/
enum {
	LOOK_LEFT = -1,
	LOOK_RIGHT = 1
};
idSplinePath* rvTramCar::FindSplineToTrack( idSplinePath* spline, const idStr& track ) const {
	idSplinePath*	target = NULL;
	idEntity*		ent = NULL;
	idStr			trackInfo;
	idList<rvSplineMover*> list;

	if( !spline ) {
		return NULL;
	}

	for( int ix = SortSplineTargets(spline) - 1; ix >= 0; --ix ) {
		ent = GetSplineTarget( spline, ix );
		target = static_cast<idSplinePath*>( ent );
		assert( target->IsActive() );

		trackInfo = GetTrackInfo( target );
		if( -1 >= trackInfo.Find(track) ) {
			continue;
		}

		// HACK: I hate switch statements
		switch( ConvertToTrackNumber(trackInfo) - GetCurrentTrack() ) {
			case LOOK_LEFT: {
				if( !LookLeft(list) ) {
					return target; 
				}
				break;
			}

			case LOOK_RIGHT: {
				if( !LookRight(list) ) {
					return target; 
				}
				break;
			}

			default: {
				return target;
			}
		}
		// HACK
	}

	return NULL;
}

/*
================
rvTramCar::SortSplineTargets
================
*/
int	rvTramCar::SortSplineTargets( idSplinePath* spline ) const {
	assert( spline );
	return (SignZero(GetSpeed()) >= 0) ? spline->SortTargets() : spline->SortBackwardsTargets();
}

/*
================
rvTramCar::GetSplineTarget
================
*/
idEntity* rvTramCar::GetSplineTarget( idSplinePath* spline, int index ) const {
	assert( spline );
	return (SignZero(GetSpeed()) >= 0) ? spline->targets[index].GetEntity() : spline->backwardPathTargets[index].GetEntity();
}

/*
================
rvTramCar::FindSplineToIdealTrack
================
*/
idSplinePath* rvTramCar::FindSplineToIdealTrack( idSplinePath* spline ) const {
	// HACK
	int				trackDelta = idMath::ClampInt( -1, 1, GetIdealTrack() - GetCurrentTrack() );
	idStr			trackLetter( ConvertToTrackLetter(GetCurrentTrack() + trackDelta) );
	idSplinePath*	s = NULL;

	if( idealTrackTag.Length() && !trackDelta ) {// On ideal track
		s = FindSplineToTrack( spline, idealTrackTag + trackLetter );
	}
	if( !s ) {
		s = FindSplineToTrack( spline, trackLetter );
	}
	return s;
}

/*
================
rvTramCar::GetRandomSpline
================
*/
idSplinePath* rvTramCar::GetRandomSpline( idSplinePath* spline ) const {
	if( !spline ) {
		return NULL;
	}

	int numActiveTargets = SortSplineTargets( spline );
	if( !numActiveTargets ) {
		return NULL;
	}

	idEntity* target = GetSplineTarget( spline, rvRandom::irand(0, numActiveTargets - 1) );
	return (target && target->IsType(idSplinePath::GetClassType())) ? static_cast<idSplinePath*>(target) : NULL;
}

/*
================
rvTramCar::GetCurrentTrack
================
*/
int rvTramCar::GetCurrentTrack() const {
	return ConvertToTrackNumber( GetTrackInfo(GetSpline()) );
}

/*
================
rvTramCar::UpdateChannel
================
*/
void rvTramCar::UpdateChannel( const s_channelType channel, const soundShaderParms_t& parms ) {
	idSoundEmitter *emitter = soundSystem->EmitterForIndex( SOUNDWORLD_GAME, refSound.referenceSoundHandle );
	if( emitter ) {
		emitter->ModifySound( channel, &parms );
	}
}

/*
================
rvTramCar::AttenuateTrackChannel
================
*/
void rvTramCar::AttenuateTrackChannel( float attenuation ) {
	soundShaderParms_t parms = refSound.parms;

	parms.frequencyShift = attenuation;//idMath::MidPointLerp( 0.0f, 1.0f, 1.1f, attenuation );

	UpdateChannel( SND_CHANNEL_BODY, parms );
}

/*
================
rvTramCar::AttenuateTramCarChannel
================
*/
void rvTramCar::AttenuateTramCarChannel( float attenuation ) {
	soundShaderParms_t parms = refSound.parms;

	parms.volume = (attenuation + 1.0f) * 0.5f;
	parms.frequencyShift = Max( 0.4f, attenuation );

	UpdateChannel( SND_CHANNEL_BODY2, parms );
}

/*
================
rvTramCar::LookForward
================
*/
bool rvTramCar::LookForward( idList<rvSplineMover*>& list ) const {
	collisionFov.SetOrigin( GetPhysics()->GetOrigin() );
	collisionFov.SetAxis( GetPhysics()->GetAxis() );

	Look( collisionFov, list );

	for( int ix = list.Num() - 1; ix >= 0; --ix ) {
		if( !OnSameTrackAs(list[ix]) ) {
			list.Remove( list[ix] );
		}
	}

	return list.Num() > 0;
}

/*
================
rvTramCar::LookLeft
================
*/
bool rvTramCar::LookLeft( idList<rvSplineMover*>& list ) const {
	collisionFov.SetOrigin( GetPhysics()->GetOrigin() );
	collisionFov.SetAxis( idAngles(0.0f, 60.0f, 0.0f).ToMat3() * GetPhysics()->GetAxis() );

	return Look( collisionFov, list );
}

/*
================
rvTramCar::LookRight
================
*/
bool rvTramCar::LookRight( idList<rvSplineMover*>& list ) const {
	collisionFov.SetOrigin( GetPhysics()->GetOrigin() );
	collisionFov.SetAxis( idAngles(0.0f, -60.0f, 0.0f).ToMat3() * GetPhysics()->GetAxis() );

	return Look( collisionFov, list );
}

/*
================
rvTramCar::LookLeftForTrackChange
================
*/
bool rvTramCar::LookLeftForTrackChange( idList<rvSplineMover*>& list ) const {
	collisionFov.SetOrigin( GetPhysics()->GetOrigin() );
	collisionFov.SetAxis( idAngles(0.0f, 45.0f, 0.0f).ToMat3() * GetPhysics()->GetAxis() );

	return Look( collisionFov, list );
}

/*
================
rvTramCar::LookRightForTrackChange
================
*/
bool rvTramCar::LookRightForTrackChange( idList<rvSplineMover*>& list ) const {
	collisionFov.SetOrigin( GetPhysics()->GetOrigin() );
	collisionFov.SetAxis( idAngles(0.0f, -45.0f, 0.0f).ToMat3() * GetPhysics()->GetAxis() );

	return Look( collisionFov, list );
}

/*
================
rvTramCar::Look
================
*/
int rvSortByDist( const void* left, const void* right ) {
	rvSplineMover* leftMover = *(rvSplineMover**)left;
	rvSplineMover* rightMover = *(rvSplineMover**)right;

	return rightMover->spawnArgs.GetFloat("distAway") - leftMover->spawnArgs.GetFloat("distAway");
}
bool rvTramCar::Look( const idFrustum& fov, idList<rvSplineMover*>& list ) const {
	bool result = WhosVisible( fov, list );

	if( g_debugVehicle.GetInteger() == 3 ) {
		gameRenderWorld->DebugFrustum( colorRed, fov );
	}

	for( int ix = list.Num() - 1; ix >= 0; --ix ) {
		list[ix]->spawnArgs.SetFloat( "distAway", (list[ix]->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).Length() );
	}

	qsort( list.Ptr(), list.Num(), list.TypeSize(), rvSortByDist );

	// Do we need to get rid of these keyvalues?

	return result;
}

/*
================
rvTramCar::OnSameTrackAs
================
*/
bool rvTramCar::OnSameTrackAs( const rvSplineMover* tram ) const {
	return tram && !GetTrackInfo(GetSpline()).Icmp( GetTrackInfo(tram->GetSpline()) );
}

/*
================
rvTramCar::SameIdealTrackAs
================
*/
bool rvTramCar::SameIdealTrackAs( const rvSplineMover* tram ) const {
	if( !tram ) {
		return false;
	}
	return GetIdealTrack() == static_cast<const rvTramCar*>(tram)->GetIdealTrack();
}

/*
================
rvTramCar::LookAround
================
*/
void rvTramCar::LookAround() {
	idList<rvSplineMover*> moverList;

	if( !AdjustSpeed(moverList) ) {
		SetSpeed( GetIdealSpeed() );
	}
}

/*
================
rvTramCar::AdjustSpeed
================
*/
bool rvTramCar::AdjustSpeed( idList<rvSplineMover*>& moverList ) {
	if( LookForward(moverList) ) {
		if( g_debugVehicle.GetInteger() ) {
			gameRenderWorld->DebugLine( colorRed, GetPhysics()->GetOrigin(), moverList[0]->GetPhysics()->GetOrigin() );
		}

		//Slow down and try to get onto other track if allowed
		SetSpeed( moverList[0]->GetSpeed() * rvRandom::flrand(0.5f, 0.75f) );

		// Is this safe if we are on a specified track
		SetIdealTrack( (GetIdealTrack() + rvRandom::irand(1, 2)) % numTracksOnMap );
		return true;
	}

	return false;
}

/*
================
rvTramCar::DriverSpeak
================
*/
idEntity* rvTramCar::DriverSpeak( const char* speechDecl, bool random ) {
	if( !driver.IsValid() || driver->IsSpeaking() ) {
		return NULL;
	}

	driver->Speak( speechDecl, random );
	return driver;
}

/*
================
rvTramCar::OccupantSpeak
================
*/
idEntity* rvTramCar::OccupantSpeak( const char *speechDecl, bool random ) {
	idEntityPtr<idAI> occupant;

	if( !occupants.Num() ) {
		return NULL;
	}

	occupant = occupants[ rvRandom::irand(0, occupants.Num() - 1) ];
	if( !occupant.IsValid() || occupant->IsSpeaking() ) {
		return NULL;
	}

	occupant->Speak( speechDecl, random );
	return occupant;
}

/*
================
rvTramCar::PostDoneMoving
================
*/
void rvTramCar::PostDoneMoving() {
	rvSplineMover::PostDoneMoving();

	HeadTowardsIdealTrack();
}

/*
================
rvTramCar::DeployRamp
================
*/
void rvTramCar::DeployRamp() {
	OperateRamp( "open" );
}

/*
================
rvTramCar::RetractRamp
================
*/
void rvTramCar::RetractRamp() {
	OperateRamp( "close" );
}

/*
================
rvTramCar::OperateRamp
================
*/
void rvTramCar::OperateRamp( const char* operation ) {
	if( !operation || !operation[0] ) {
		return;
	}
		
	PlayAnim( ANIMCHANNEL_ALL, operation, 0 );

	OperateRamp( operation, leftDoor );
	OperateRamp( operation, rightDoor );
}

/*
================
rvTramCar::OperateRamp
================
*/
void rvTramCar::OperateRamp( const char* operation, idMover* door ) {
	if( !operation || !operation[0] ) {
		return;
	}

	idAngles	ang;
	idVec3		vec;
	if( !door ) {
		return;
	}

	if( !door->IsBound() ) {
		return;
	}

	ang = door->spawnArgs.GetAngles( va("%s%s", operation, "_rotation") );
	vec.Set( ang[0], ang[1], ang[2] );
	door->GetBindMaster()->ProcessEvent( &EV_RotateOnce, vec );

	vec = door->spawnArgs.GetVector(va("%s%s", operation, "_dir")).ToNormal() * door->spawnArgs.GetFloat("distExt");
	door->PostEventSec( &EV_MoveAlongVector, 0.5f, vec );
}

/*
================
rvTramCar::Event_OnStopMoving
================
*/
void rvTramCar::Event_OnStopMoving() {
	rvSplineMover::Event_OnStopMoving();

	speedSoundEffectsStateThread.SetState( "IdleSpeed" );
}

/*
================
rvTramCar::Event_OnStartMoving
================
*/
void rvTramCar::Event_OnStartMoving() {
	rvSplineMover::Event_OnStartMoving();

	speedSoundEffectsStateThread.SetState( "NormalSpeed" );
}

/*
================
rvTramCar::Event_DriverSpeak
================
*/
void rvTramCar::Event_DriverSpeak( const char* voKey ) {
	idThread::ReturnEntity( DriverSpeak(voKey) );
}

/*
================
rvTramCar::Event_GetDriver
================
*/
void rvTramCar::Event_GetDriver() {
	idThread::ReturnEntity( driver );
}

/*
================
rvTramCar::Event_Activate
================
*/
void rvTramCar::Event_Activate( idEntity* activator ) {
	rvSplineMover::Event_Activate( activator );

	// If being activated by a spawner we need to attach to it
	if( activator->IsType(rvSpawner::GetClassType()) ) {
		static_cast<rvSpawner*>(activator)->Attach( this );
	} else {
		if( driver.IsValid() ) {
			driver->ProcessEvent( &EV_Activate, activator );
		}

		occupants.RemoveNull();
		for( int ix = occupants.Num() - 1; ix >= 0; --ix ) {
			occupants[ix]->ProcessEvent( &EV_Activate, activator );
		}
	}
}

/*
================
rvTramCar::Event_RadiusDamage
================
*/
void rvTramCar::Event_RadiusDamage( const idVec3& origin, const char* damageDefName ) {
	gameLocal.RadiusDamage( origin, this, this, this, this, damageDefName );
}

/*
================
rvTramCar::Event_SetIdealTrack
================
*/
void rvTramCar::Event_SetIdealTrack( const char* track ) {
	idStr ideal = track;

	if( ideal.Length() > 1 ) {
		idealTrackTag = ideal.Left( ideal.Length() - 1 );
	}

	idealTrackStateThread.SetState( "AssignedTrack" );
	SetIdealTrack( ConvertToTrackNumber(ideal.Right(1)) );
}

/*
================
rvTramCar::Event_OpenDoors
================
*/
void rvTramCar::Event_OpenDoors() {
	DeployRamp();
}

/*
================
rvTramCar::Event_CloseDoors
================
*/
void rvTramCar::Event_CloseDoors() {
	RetractRamp();
}

/*
================
rvTramCar::Event_SetHealth
================
*/
void rvTramCar::Event_SetHealth	( float health ) {
	this->health = health;
}

CLASS_STATES_DECLARATION( rvTramCar )
	STATE( "IdleSpeed",			rvTramCar::State_Idle )
	STATE( "NormalSpeed",		rvTramCar::State_NormalSpeed )
	STATE( "ExcessiveSpeed",	rvTramCar::State_ExcessiveSpeed )
	STATE( "RandomTrack",		rvTramCar::State_RandomTrack )
	STATE( "AssignedTrack",		rvTramCar::State_AssignedTrack )
END_CLASS_STATES

/*
================
rvTramCar::State_Idle
================
*/
stateResult_t rvTramCar::State_Idle( const stateParms_t& parms ) {
	if( !parms.stage ) {
		StartSound( "snd_speed_idle_track", SND_CHANNEL_BODY, 0, false, NULL );
		StartSound( "snd_speed_idle_tram", SND_CHANNEL_BODY2, 0, false, NULL );
		return SRESULT_STAGE( parms.stage + 1 );
	}
	return SRESULT_WAIT; 
}

/*
================
rvTramCar::State_NormalSpeed
================
*/
stateResult_t rvTramCar::State_NormalSpeed( const stateParms_t& parms ) {
	if( !parms.stage ) {
		StartSound( "snd_speed_normal_track", SND_CHANNEL_BODY, 0, false, NULL );
		StartSound( "snd_speed_normal_tram", SND_CHANNEL_BODY2, 0, false, NULL );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	float speedScale = 0.8f + 0.2f * ( GetSpeed() / GetNormalSpeed() );

	if( speedScale >= 1.0f ) {
		speedSoundEffectsStateThread.SetState( "ExcessiveSpeed" );
		return SRESULT_DONE;
	}

	AttenuateTrackChannel( speedScale );
	AttenuateTramCarChannel( speedScale );
	return SRESULT_WAIT;
}

/*
================
rvTramCar::State_ExcessiveSpeed
================
*/
stateResult_t rvTramCar::State_ExcessiveSpeed( const stateParms_t& parms ) {
	if( !parms.stage ) {
		StartSound( "snd_speed_excessive_track", SND_CHANNEL_BODY, 0, false, NULL );
		StartSound( "snd_speed_excessive_tram", SND_CHANNEL_BODY2, 0, false, NULL );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	float speedScale = GetSpeed() / GetNormalSpeed();

	if( speedScale < 1.0f ) {
		speedSoundEffectsStateThread.SetState( "NormalSpeed" );
		return SRESULT_DONE;	
	}

	AttenuateTrackChannel( speedScale );
	AttenuateTramCarChannel( speedScale );
	return SRESULT_WAIT;
}

/*
================
rvTramCar::State_RandomTrack
================
*/
stateResult_t rvTramCar::State_RandomTrack( const stateParms_t& parms ) {
	SetIdealTrack( rvRandom::irand(0, numTracksOnMap - 1) );
	return SRESULT_WAIT;
}

/*
================
rvTramCar::State_AssignedTrack
================
*/
stateResult_t rvTramCar::State_AssignedTrack( const stateParms_t& parms ) {
	return SRESULT_WAIT;
}

//=======================================================
//
//	rvTramCar_Marine
//
//=======================================================
const idEventDef EV_TramCar_UseMountedGun( "useMountedGun", "e" );
const idEventDef EV_TramCar_SetPlayerDamageEnt( "setPlayerDamageEnt", "f" );

CLASS_DECLARATION( rvTramCar, rvTramCar_Marine )
	EVENT( EV_TramCar_UseMountedGun,	rvTramCar_Marine::Event_UseMountedGun )
	EVENT( EV_TramCar_SetPlayerDamageEnt,	rvTramCar_Marine::Event_SetPlayerDamageEntity )
END_CLASS

/*
================
rvTramCar_Marine::Spawn
================
*/
void rvTramCar_Marine::Spawn() {
	RegisterStateThread( playerOccupationStateThread, "PlayerOccupation" );
	playerOccupationStateThread.SetState( "NotOccupied" );

	RegisterStateThread( playerUsingMountedGunStateThread, "MountedGunInUse" );
	playerUsingMountedGunStateThread.SetState( "NotUsingMountedGun" );

	maxHealth	= spawnArgs.GetInt( "health", "0" );
	lastHeal	= 0;
	healDelay	= spawnArgs.GetInt( "heal_delay", "15" );
	healAmount	= spawnArgs.GetInt( "heal_amount", "2" );
}

/*
================
rvTramCar_Marine::MidThink
================
*/
void rvTramCar_Marine::MidThink() {
	rvTramCar::MidThink();

	if ( healDelay > 0 && lastHeal + healDelay < gameLocal.time && health < maxHealth ) {
		health += healAmount;

		if ( health > maxHealth ) {
			health = maxHealth;
		}
	}

	playerOccupationStateThread.Execute();
}

/*
================
rvTramCar_Marine::ActivateTramHud
================
*/
void rvTramCar_Marine::ActivateTramHud( idPlayer* player ) {
	if( !player ) {
		return;
	}

	idUserInterface* hud = player->GetHud();
	if( !hud ) {
		return;
	}

	hud->HandleNamedEvent( "enterTram" );
}

/*
================
rvTramCar_Marine::DeactivateTramHud
================
*/
void rvTramCar_Marine::DeactivateTramHud( idPlayer* player ) {
	if( !player ) {
		return;
	}

	idUserInterface* hud = player->GetHud();
	if( !hud ) {
		return;
	}

	hud->HandleNamedEvent( "leaveTram" );
}

/*
================
rvTramCar_Marine::UpdateTramHud
================
*/
void rvTramCar_Marine::UpdateTramHud( idPlayer* player ) {
	if( !player ) {
		return;
	}

	idUserInterface* hud = player->GetHud();
	if( !hud ) {
		return;
	}

	hud->SetStateFloat( "tram_healthpct", GetHealthScale() );
}

/*
============
rvTramCar_Marine::Damage
============
*/
void rvTramCar_Marine::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {
	rvTramCar::Damage( inflictor, attacker, dir, damageDefName, damageScale, location );

	if( attacker == gameLocal.GetLocalPlayer() ) {
		return;
	}

	if( fl.takedamage && GetDamageScale() < 0.35f ) {
		return;
	}

	if( rvRandom::flrand() > spawnArgs.GetFloat("damageWarningFreq", "0.2") ) {
		return;
	}

	DriverSpeak( "lipsync_damageWarning", true );
}

/*
================
rvTramCar_Marine::Killed
================
*/
void rvTramCar_Marine::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	playerOccupationStateThread.Execute();

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player && EntityIsInside(player) ) {
		player->SetDamageEntity( NULL );// This should fix the damage from tram car propogation issue
		player->ExitVehicle( true );// This should fix the issue that was causing the player to get removed
	}

	rvTramCar::Killed( inflictor, attacker, damage, dir, location );
}

/*
================
rvTramCar_Marine::Save
================
*/
void rvTramCar_Marine::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( visibleEnemies.Num() );
	for( int ix = visibleEnemies.Num() - 1; ix >= 0; --ix ) {
		visibleEnemies[ix].Save( savefile );
	}

	playerOccupationStateThread.Save( savefile );
	playerUsingMountedGunStateThread.Save( savefile );

	// no need to save this since it's set in the restore
	//savefile->WriteInt( maxHealth );	// cnicholson: added unsaved var
	savefile->WriteInt( lastHeal );
	savefile->WriteInt( healDelay );
	savefile->WriteInt( healAmount );
}

/*
================
rvTramCar_Marine::Restore
================
*/
void rvTramCar_Marine::Restore( idRestoreGame *savefile ) {
	int num = 0;
	savefile->ReadInt( num );
	visibleEnemies.SetNum( num );
	for( int ix = visibleEnemies.Num() - 1; ix >= 0; --ix ) {
		visibleEnemies[ix].Restore( savefile );
	}

	playerOccupationStateThread.Restore( savefile, this );
	playerUsingMountedGunStateThread.Restore( savefile, this );

	savefile->ReadInt( lastHeal );
	savefile->ReadInt( healDelay );
	savefile->ReadInt( healAmount );

	maxHealth = spawnArgs.GetInt( "health", "0" );
}

/*
================
rvTramCar_Marine::LookAround
================
*/
void rvTramCar_Marine::LookAround() {
	 idList<rvSplineMover*> moverList;

	rvTramCar::LookAround();

	if( !driver.IsValid() || driver->IsSpeaking() ) {
		return;
	}

	if( LookOverLeftShoulder(moverList)) {
		DriverSpeak( "lipsync_warningLeft", true );
		driver->ScriptedAnim( "point_left", 0, false, true );
	} else if( LookOverRightShoulder(moverList)) {
		DriverSpeak( "lipsync_warningRight", true );
		driver->ScriptedAnim( "point_right", 0, false, true );
	}
}


/*
================
rvTramCar_Marine::LookOverLeftShoulder
================
*/
bool rvTramCar_Marine::LookOverLeftShoulder( idList<rvSplineMover*>& list ) {
	collisionFov.SetOrigin( GetPhysics()->GetOrigin() );
	collisionFov.SetAxis( idAngles(0.0, -120.0f, 0.0f).ToMat3() * GetPhysics()->GetAxis() );

	bool newOneFound = false;
	Look( collisionFov, list );

	// now determine if we've spotted a new enemy
	for( int ix = list.Num() - 1; ix >= 0; --ix ) {
		if ( !visibleEnemies.Find( list[ix] )) {
			newOneFound = true;
			visibleEnemies.AddUnique( list[ix] );
		}
	}

	return newOneFound;
}

/*
================
rvTramCar_Marine::LookOverRightShoulder
================
*/
bool rvTramCar_Marine::LookOverRightShoulder( idList<rvSplineMover*>& list ) {
	collisionFov.SetOrigin( GetPhysics()->GetOrigin() );
	collisionFov.SetAxis( idAngles(0.0, 120.0f, 0.0f).ToMat3() * GetPhysics()->GetAxis() );

	bool newOneFound = false;
	Look( collisionFov, list );

	// now determine if we've spotted a new enemy
	for( int ix = list.Num() - 1; ix >= 0; --ix ) {
		if ( !visibleEnemies.Find( list[ix] )) {
			newOneFound = true;
			visibleEnemies.AddUnique( list[ix] );
		}
	}

	return newOneFound;
}

/*
================
rvTramCar_Marine::EntityIsInside
================
*/
bool rvTramCar_Marine::EntityIsInside( const idEntity* entity ) const {
	assert( entity );
	return GetPhysics()->GetAbsBounds().ContainsPoint( entity->GetPhysics()->GetAbsBounds().GetCenter() );
}

/*
================
rvTramCar_Marine::DeployRamp
================
*/
void rvTramCar_Marine::DeployRamp() {
	rvTramCar::DeployRamp();

	if( weapons.Num() ) {
		weapons[0]->Unlock();
		weapons[0]->EjectAllDrivers( true );
	}
}

/*
================
rvTramCar_Marine::RetractRamp
================
*/
void rvTramCar_Marine::RetractRamp() {
	rvTramCar::RetractRamp();

	UseMountedGun( gameLocal.GetLocalPlayer() );
}

/*
================
rvTramCar_Marine::UseMountedGun
================
*/
void rvTramCar_Marine::UseMountedGun( idPlayer* player ) {
	if( playerOccupationStateThread.CurrentStateIs("Occupied") && EntityIsInside(player) && weapons.Num() ) {
		player->EnterVehicle( weapons[0] );
		weapons[0]->Lock();
	}
}

/*
================
rvTramCar_Marine::Event_UseMountedGun
================
*/
void rvTramCar_Marine::Event_UseMountedGun( idEntity* ent ) {
	if( !ent || !ent->IsType(idPlayer::GetClassType()) ) {
		return;
	}

	UseMountedGun( static_cast<idPlayer*>(ent) );
}

/*
================
rvTramCar_Marine::Event_SetPlayerDamageEntity
================
*/
void rvTramCar_Marine::Event_SetPlayerDamageEntity(float f)	{
	
	gameLocal.GetLocalPlayer()->SetDamageEntity( (f? this : NULL) );
}

CLASS_STATES_DECLARATION( rvTramCar_Marine )
	STATE( "Occupied",			rvTramCar_Marine::State_Occupied )
	STATE( "NotOccupied",		rvTramCar_Marine::State_NotOccupied )
	STATE( "UsingMountedGun",	rvTramCar_Marine::State_UsingMountedGun )
	STATE( "NotUsingMountedGun",rvTramCar_Marine::State_NotUsingMountedGun )
END_CLASS_STATES


/*
================
rvTramCar_Marine::State_Occupied
================
*/
stateResult_t rvTramCar_Marine::State_Occupied( const stateParms_t& parms ) {
	idPlayer* player = gameLocal.GetLocalPlayer();

	if( !parms.stage ) {
		//player->ProcessEvent( &EV_Player_DisableWeapon );
		ActivateTramHud( player );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	if( !PlayerIsInside() ) {
		playerOccupationStateThread.SetState( "NotOccupied" );	
		//gameLocal.GetLocalPlayer()->SetDamageEntity( NULL );
		fl.takedamage = false;
		return SRESULT_DONE_WAIT;
	}

	playerUsingMountedGunStateThread.Execute();

	UpdateTramHud( player );

	return SRESULT_WAIT;
}



/*
================
rvTramCar_Marine::State_NotOccupied
================
*/
stateResult_t rvTramCar_Marine::State_NotOccupied( const stateParms_t& parms ) {
	if( !parms.stage ) {
		//player->ProcessEvent( &EV_Player_EnableWeapon );
		DeactivateTramHud( gameLocal.GetLocalPlayer() );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	if( PlayerIsInside() ) {
		playerOccupationStateThread.SetState( "Occupied" );
		//gameLocal.GetLocalPlayer()->SetDamageEntity( this );
		fl.takedamage = true;
		return SRESULT_DONE_WAIT;
	}

	return SRESULT_WAIT;
}

/*
================
rvTramCar_Marine::State_UsingMountedGun
================
*/
stateResult_t rvTramCar_Marine::State_UsingMountedGun( const stateParms_t& parms ) {
	if( !parms.stage ) {
		ActivateTramHud( gameLocal.GetLocalPlayer() );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player && !player->IsInVehicle() ) {
		playerUsingMountedGunStateThread.SetState( "NotUsingMountedGun" );
	}

	return SRESULT_WAIT;
}

/*
================
rvTramCar_Marine::State_NotUsingMountedGun
================
*/
stateResult_t rvTramCar_Marine::State_NotUsingMountedGun( const stateParms_t& parms ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	if( player && player->IsInVehicle() ) {
		playerUsingMountedGunStateThread.SetState( "UsingMountedGun" );
	}

	return SRESULT_WAIT;
}

//=======================================================
//
//	rvTramCar_Strogg
//
//=======================================================
CLASS_DECLARATION( rvTramCar, rvTramCar_Strogg )
	EVENT( EV_PostSpawn,		rvTramCar_Strogg::Event_PostSpawn )
END_CLASS

/*
================
rvTramCar_Strogg::Spawn
================
*/
void rvTramCar_Strogg::Spawn() {
	SetTarget( NULL );

	RegisterStateThread( targetSearchStateThread, "targetSearch" );
	targetSearchStateThread.SetState( "LookingForTarget" );
}

/*
================
rvTramCar_Strogg::SetTarget
================
*/
void rvTramCar_Strogg::SetTarget( idEntity* newTarget ) {
	target = ConvertToMover( newTarget );
}

/*
================
rvTramCar_Strogg::GetTarget
================
*/
const rvSplineMover* rvTramCar_Strogg::GetTarget() const {
	return target.GetEntity();
}

/*
================
rvTramCar_Strogg::Save
================
*/
void rvTramCar_Strogg::Save( idSaveGame *savefile ) const {
	target.Save( savefile );
	targetSearchStateThread.Save( savefile );
}

/*
================
rvTramCar_Strogg::Restore
================
*/
void rvTramCar_Strogg::Restore( idRestoreGame *savefile ) {
	target.Restore( savefile );
	targetSearchStateThread.Restore( savefile, this );
}

/*
================
rvTramCar_Strogg::WriteToSnapshot
================
*/
void rvTramCar_Strogg::WriteToSnapshot( idBitMsgDelta &msg ) const {
}

/*
================
rvTramCar_Strogg::ReadFromSnapshot
================
*/
void rvTramCar_Strogg::ReadFromSnapshot( const idBitMsgDelta &msg ) {
}

/*
================
rvTramCar_Strogg::Event_PostSpawn
================
*/
void rvTramCar_Strogg::Event_PostSpawn() {
	idEntity* enemy = gameLocal.FindEntity( spawnArgs.GetString("enemy") );

	SetTarget( enemy );

	occupants.RemoveNull();
	for( int ix = occupants.Num() - 1; ix >= 0; --ix ) {
		occupants[ix]->SetEnemy( enemy );
	}

	rvTramCar::Event_PostSpawn();
}

/*
================
rvTramCar_Strogg::TargetIsToLeft
================
*/
bool rvTramCar_Strogg::TargetIsToLeft() {
	idList<rvSplineMover*> list;

	if( LookLeft(list) ) {
		for( int ix = list.Num() - 1; ix >= 0; --ix ) {
			if( list[ix] == target ) {
				return true;
			}
		}
	}
	
	return false;
}

/*
================
rvTramCar_Strogg::TargetIsToRight
================
*/
bool rvTramCar_Strogg::TargetIsToRight() {
	idList<rvSplineMover*> list;

	if( LookRight(list) ) {
		for( int ix = list.Num() - 1; ix >= 0; --ix ) {
			if( list[ix] == target ) {
				return true;
			}
		}
	}
	
	return false;
}

/*
================
rvTramCar_Strogg::LookAround
================
*/
void rvTramCar_Strogg::LookAround() {
	targetSearchStateThread.Execute();
}

CLASS_STATES_DECLARATION( rvTramCar_Strogg )
	STATE( "LookingForTarget",		rvTramCar_Strogg::State_LookingForTarget )
	STATE( "TargetInSight",			rvTramCar_Strogg::State_TargetInSight )
END_CLASS_STATES

/*
================
rvTramCar_Strogg::State_LookingForTarget
================
*/
stateResult_t rvTramCar_Strogg::State_LookingForTarget( const stateParms_t& parms ) {
	static const int MSEC_DELAY = 100;
	idList<rvSplineMover*> list;

	if( !parms.stage ) {// Go back to random if we are supposed to be random
		//idealTrackStateThread.SetState( "RandomTrack" );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	if( AdjustSpeed(list) ) {
		return SRESULT_DELAY( MSEC_DELAY );
	}

	// Could optimise based on what track
	if( TargetIsToLeft() || TargetIsToRight() ) {
		targetSearchStateThread.SetState( "TargetInSight" );
		return SRESULT_DONE;
	}

	SetSpeed( GetIdealSpeed() );
	return SRESULT_DELAY( MSEC_DELAY );
}

/*
================
rvTramCar_Strogg::State_TargetInSight
================
*/
stateResult_t rvTramCar_Strogg::State_TargetInSight( const stateParms_t& parms ) {
	static const int MSEC_DELAY = 3000;

	if( !target.IsValid() || (!TargetIsToLeft() && !TargetIsToRight()) ) {
		targetSearchStateThread.SetState( "LookingForTarget" );
		return SRESULT_DONE;
	}

	if( !parms.stage ) {
		SetIdealTrack( GetCurrentTrack() );
		SetSpeed( target->GetSpeed() * 0.5f );
		idealTrackStateThread.SetState( "AssignedTrack" );
		StartSound( "snd_horn", SND_CHANNEL_ANY, 0, false, NULL );
		return SRESULT_STAGE( parms.stage + 1 );
	}

	idList<rvSplineMover*> list;
	if( AdjustSpeed(list) ) {
		return SRESULT_DELAY( MSEC_DELAY );
	}

	float dot = GetPhysics()->GetAxis()[0] * (target->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin()).ToNormal();
	float deltaSpeed = target->GetIdealSpeed() * rvRandom::flrand(0.0f, 0.1f) * SignZero(dot);
	SetSpeed( target->GetIdealSpeed() + deltaSpeed );

	return SRESULT_DELAY( MSEC_DELAY );
}
