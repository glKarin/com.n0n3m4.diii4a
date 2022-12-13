#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"
 
#define CAMERA_INTERP_LERP_DEBUG if( p_camInterpDebug.GetInteger() == 1 ) gameLocal.Printf
#define CAMERA_INTERP_SLERP_DEBUG if( p_camInterpDebug.GetInteger() == 2 ) gameLocal.Printf

/**********************************************************************

hhCameraInterpolator

**********************************************************************/

/*
================
NoInterpEvaluate
================
*/
float NoInterpEvaluate( float& interpVal, float deltaVal ) {
	return 1.0f;
}

/*
================
VariableMidPointSinusoidalEvaluate
================
*/
float VariableMidPointSinusoidalEvaluate( float& interpVal, float deltaVal ) {
	interpVal = hhMath::ClampFloat( 0.0f, 1.0f, interpVal + deltaVal );

	if( interpVal <= 0.0f || interpVal >= 1.0f ) {
		return interpVal;
	}

	float debug = hhMath::Sin( DEG2RAD(hhMath::MidPointLerp(0.0f, 60.0f, 90.0f, interpVal)) );
	return debug;
}

/*
================
LinearEvaluate
================
*/
float LinearEvaluate( float& interpVal, float deltaVal ) {
	interpVal = hhMath::ClampFloat( 0.0f, 1.0f, interpVal + deltaVal );

	if( interpVal <= 0.0f || interpVal >= 1.0f ) {
		return interpVal;
	}

	return interpVal;
}

/*
================
InverseEvaluate
================
*/
float InverseEvaluate( float& interpVal, float deltaVal ) {
	interpVal = hhMath::ClampFloat( 0.0f, 1.0f, interpVal + deltaVal );

	if( interpVal <= 0.0f || interpVal >= 1.0f ) {
		return interpVal;
	}
	
	const float minLerpVal = deltaVal;
	if( deltaVal <= 0.0f ) {
		return interpVal;
	}
	
	const float scale = (1.0f / minLerpVal) - 1.0f;
	float debug = ((1.0f / interpVal) - 1.0f) / scale;
	return debug;
}

/*
================
hhCameraInterpolator::hhCameraInterpolator
================
*/
hhCameraInterpolator::hhCameraInterpolator() : 
	clipBounds( idBounds(idVec3(-1, -1, 10), idVec3(1, 1, 12)) ) {

	ClearFuncList();
	RegisterFunc( NoInterpEvaluate, IT_None );
	RegisterFunc( VariableMidPointSinusoidalEvaluate, IT_VariableMidPointSinusoidal );
	RegisterFunc( LinearEvaluate, IT_Linear );
	RegisterFunc( InverseEvaluate, IT_Inverse );

	SetSelf( NULL );
	Setup( 0.0f, IT_None );
	Reset( vec3_origin, mat3_identity[2], 0.0f );
}

/*
================
hhCameraInterpolator::SetSelf
================
*/
void hhCameraInterpolator::SetSelf( hhPlayer* self ) {
	this->self = self;
	clipBounds.SetOwner( self );
}

/*
================
hhCameraInterpolator::GetCurrentEyeHeight
================
*/
float hhCameraInterpolator::GetCurrentEyeHeight() const {
	return eyeOffsetInfo.current;
}

/*
================
hhCameraInterpolator::GetIdealEyeHeight
================
*/
float hhCameraInterpolator::GetIdealEyeHeight() const {
	return eyeOffsetInfo.end;
}

/*
================
hhCameraInterpolator::GetCurrentEyeOffset
================
*/
idVec3 hhCameraInterpolator::GetCurrentEyeOffset() const {
	return GetCurrentUpVector() * GetCurrentEyeHeight();
}

/*
================
hhCameraInterpolator::GetEyePosition
================
*/
idVec3 hhCameraInterpolator::GetEyePosition() const {
	return GetCurrentPosition() + GetCurrentEyeOffset();
}

/*
================
hhCameraInterpolator::GetCurrentPosition
================
*/
idVec3 hhCameraInterpolator::GetCurrentPosition() const {
	return positionInfo.current;
}

/*
================
hhCameraInterpolator::GetIdealPosition
================
*/
idVec3 hhCameraInterpolator::GetIdealPosition() const {
	return positionInfo.end;
}

/*
================
hhCameraInterpolator::GetCurrentUpVector
================
*/
idVec3 hhCameraInterpolator::GetCurrentUpVector() const {
	return GetCurrentAxis()[2];
}

/*
================
hhCameraInterpolator::GetCurrentAxis
================
*/
idMat3 hhCameraInterpolator::GetCurrentAxis() const {
	return GetCurrentRotation().ToMat3();
}

/*
================
hhCameraInterpolator::GetCurrentAngles
================
*/
idAngles hhCameraInterpolator::GetCurrentAngles() const {
	return GetCurrentRotation().ToAngles();
}

/*
================
hhCameraInterpolator::GetCurrentRotation
================
*/
idQuat hhCameraInterpolator::GetCurrentRotation() const {
	return rotationInfo.current;
}

/*
================
hhCameraInterpolator::GetIdealUpVector
================
*/
idVec3 hhCameraInterpolator::GetIdealUpVector() const {
	return GetIdealAxis()[2];
}

/*
================
hhCameraInterpolator::GetIdealAxis
================
*/
idMat3 hhCameraInterpolator::GetIdealAxis() const {
	return GetIdealRotation().ToMat3();
}

/*
================
hhCameraInterpolator::GetIdealAngles
================
*/
idAngles hhCameraInterpolator::GetIdealAngles() const {
	return GetIdealRotation().ToAngles();
}

/*
================
hhCameraInterpolator::GetIdealRotation
================
*/
idQuat hhCameraInterpolator::GetIdealRotation() const {
	return rotationInfo.end;
}

/*
================
hhCameraInterpolator::UpdateViewAngles
================
*/
idAngles hhCameraInterpolator::UpdateViewAngles( const idAngles& viewAngles ) {
	return (viewAngles.ToMat3() * GetCurrentAxis()).ToAngles();
}

/*
================
hhCameraInterpolator::UpdateTarget
================
*/
void hhCameraInterpolator::UpdateTarget( const idVec3& idealPos, const idMat3& idealAxis, float eyeOffset, int interpFlags ) {
	if( !positionInfo.end.Compare(idealPos, VECTOR_EPSILON) ) {
		SetTargetPosition( idealPos, interpFlags );
	}

	if( !idealAxis[2].Compare(GetIdealUpVector(), VECTOR_EPSILON) ) {
		SetTargetAxis( idealAxis, interpFlags );
	}

	if( hhMath::Fabs(eyeOffset - hhMath::Fabs(eyeOffsetInfo.end)) >= VECTOR_EPSILON ) {
		SetTargetEyeOffset( eyeOffset, interpFlags );
	}
}

/*
================
hhCameraInterpolator::SetTargetPosition
================
*/
void hhCameraInterpolator::SetTargetPosition( const idVec3& idealPos, int interpFlags ) {
	if( interpFlags & INTERPOLATE_POSITION ) {
		positionInfo.Set( idealPos );
	} else {
		positionInfo.start = idealPos - (positionInfo.end - positionInfo.start);
		positionInfo.current = idealPos - (positionInfo.end - positionInfo.current);
		positionInfo.end = idealPos;
	}
}

/*
================
hhCameraInterpolator::SetTargetAxis
================
*/
void hhCameraInterpolator::SetTargetAxis( const idMat3& idealAxis, int interpFlags ) {
	idQuat cachedRotation;
	idQuat idealRotation = DetermineIdealRotation( idealAxis[2] );
	if( interpFlags & INTERPOLATE_ROTATION ) {
		rotationInfo.Set( idealRotation );
	} else {
		cachedRotation = idealRotation * rotationInfo.end.Inverse();
		rotationInfo.start = cachedRotation * rotationInfo.start;
		rotationInfo.current = cachedRotation * rotationInfo.current;
		rotationInfo.end = idealRotation;
	}
}

/*
================
hhCameraInterpolator::SetTargetEyeOffset
================
*/
void hhCameraInterpolator::SetTargetEyeOffset( float idealEyeOffset, int interpFlags ) {
	if( interpFlags & INTERPOLATE_EYEOFFSET ) {
		eyeOffsetInfo.Set( idealEyeOffset );
	}
}

/*
================
hhCameraInterpolator::SetInterpolationType
================
*/
InterpolationType hhCameraInterpolator::SetInterpolationType( InterpolationType type ) {
	InterpolationType cachedType = interpType;

	interpType = type;

	return cachedType;
}

/*
================
hhCameraInterpolator::Setup
================
*/
void hhCameraInterpolator::Setup( const float lerpScale, const InterpolationType type ) {
	this->lerpScale = hhMath::hhMax( 0.01f, lerpScale );
	SetInterpolationType( type );
}

/*
================
hhCameraInterpolator::Reset
================
*/
void hhCameraInterpolator::Reset( const idVec3& position, const idVec3& idealUpVector, float eyeOffset ) {
	positionInfo.Reset( position );
	rotationInfo.Reset( DetermineIdealRotation(idealUpVector) );
	eyeOffsetInfo.Reset( eyeOffset );
}

/*
================
hhCameraInterpolator::DetermineIdealRotation
================
*/
idQuat hhCameraInterpolator::DetermineIdealRotation( const idVec3& idealUpVector, const idVec3& viewDir, const idMat3& untransformedViewAxis ) {
	idMat3 mat;
	idVec3 newViewVector( viewDir );

	newViewVector.ProjectOntoPlane( idealUpVector );
	if( newViewVector.LengthSqr() < VECTOR_EPSILON ) {
		newViewVector = -Sign( newViewVector * idealUpVector );
	}

	newViewVector.Normalize();
	mat[0] = newViewVector;
	mat[1] = idealUpVector.Cross( newViewVector );
	mat[2] = idealUpVector;

	mat = untransformedViewAxis.Transpose() * mat;
	return mat.ToQuat();
}

/*
================
hhCameraInterpolator::DetermineIdealRotation
================
*/
idQuat hhCameraInterpolator::DetermineIdealRotation( const idVec3& idealUpVector ) {
	if( !self ) {
		return mat3_identity.ToQuat();
	}

	return DetermineIdealRotation( idealUpVector, self->GetAxis()[0], self->GetUntransformedViewAxis() );
}

/*
================
hhCameraInterpolator::ClearFuncList
================
*/
void hhCameraInterpolator::ClearFuncList() {
	funcList.SetNum( IT_NumTypes );

	for( int ix = 0; ix < funcList.Num(); ++ix ) {
		funcList[ix] = NULL;
	}
}

/*
================
hhCameraInterpolator::RegisterFunc
================
*/
void hhCameraInterpolator::RegisterFunc( InterpFunc func, InterpolationType type ) {
	int index = (int)type;

	if( funcList[index] ) {
		gameLocal.Warning( "Function already registered for interpolation type %d", type );
	}
		
	funcList[index] = func;
}

/*
================
hhCameraInterpolator::DetermineFunc
================
*/
InterpFunc hhCameraInterpolator::DetermineFunc( InterpolationType type ) {
	int index = (int)( p_disableCamInterp.GetBool() ? IT_None : type );
	return funcList[ index ];
}

/*
================
hhCameraInterpolator::Evaluate
================
*/
void hhCameraInterpolator::Evaluate( float deltaTime ) {
	float baseTime = deltaTime * gameLocal.GetTimeScale();
	float scaledDeltaTime = baseTime * lerpScale;
	float lenFactor = (positionInfo.current-positionInfo.end).Length()*0.2f;
	if (lenFactor < 1.0f) {
		lenFactor = 1.0f;
	}
	float scaledPosDeltaTime = baseTime * (lerpScale*lenFactor);
	float scaledEyeDeltaTime = baseTime * (lerpScale*4.0f);

	InterpFunc func = DetermineFunc( interpType );
	if( !func ) {
		return;
	}

	EvaluatePosition( func(positionInfo.interpVal, scaledPosDeltaTime) );
	EvaluateRotation( func(rotationInfo.interpVal, scaledDeltaTime) );
	EvaluateEyeOffset( func(eyeOffsetInfo.interpVal, scaledEyeDeltaTime) );

	VerifyEyeOffset( eyeOffsetInfo.current );
}

/*
================
hhCameraInterpolator::EvaluatePosition
================
*/
void hhCameraInterpolator::EvaluatePosition( float interpVal ) {
	if( interpVal >= 1.0f ) {
		positionInfo.Reset( positionInfo.end );
		return;
	}

	positionInfo.current.Lerp( positionInfo.start, positionInfo.end, interpVal );
}

/*
================
hhCameraInterpolator::EvaluateRotation
================
*/
void hhCameraInterpolator::EvaluateRotation( float interpVal ) {
	if( interpVal >= 1.0f ) {
		rotationInfo.Reset( rotationInfo.end );
		return;
	}

	rotationInfo.current.Slerp( rotationInfo.start, rotationInfo.end, interpVal );
}

/*
================
hhCameraInterpolator::EvaluateEyeOffset
================
*/
void hhCameraInterpolator::EvaluateEyeOffset( float interpVal ) {
	if( interpVal >= 1.0f ) {
		eyeOffsetInfo.Reset( eyeOffsetInfo.end );
		return;
	}
	
	eyeOffsetInfo.current = hhMath::Lerp( eyeOffsetInfo.start, eyeOffsetInfo.end, interpVal );
}

/*
================
hhCameraInterpolator::VerifyEyeOffset
================
*/
void hhCameraInterpolator::VerifyEyeOffset( float& eyeOffset ) {
	idPhysics* selfPhysics = NULL;

	if( !self ) {
		return;
	}

	selfPhysics = self->GetPhysics();
	if( !selfPhysics ) {
		return;
	}

	if( clipBounds.GetBounds().Translate(GetCurrentUpVector() * eyeOffset).IntersectsBounds(selfPhysics->GetBounds()) ) {
		return;
	}

	trace_t trace;
	gameLocal.clip.Translation( trace, GetCurrentPosition(), GetCurrentPosition() + GetCurrentUpVector() * eyeOffset, &clipBounds, GetCurrentAxis(), selfPhysics->GetClipMask(), NULL );
	eyeOffset *= trace.fraction;
}

//================
//hhCameraInterpolator::Save
//================
void hhCameraInterpolator::Save( idSaveGame *savefile ) const {
	savefile->WriteQuat( rotationInfo.start );
	savefile->WriteQuat( rotationInfo.end );
	savefile->WriteQuat( rotationInfo.current );
	savefile->WriteFloat( rotationInfo.interpVal );

	savefile->WriteVec3( positionInfo.start );
	savefile->WriteVec3( positionInfo.end );
	savefile->WriteVec3( positionInfo.current );
	savefile->WriteFloat( positionInfo.interpVal );

	savefile->WriteFloat( eyeOffsetInfo.start );
	savefile->WriteFloat( eyeOffsetInfo.end );
	savefile->WriteFloat( eyeOffsetInfo.current );
	savefile->WriteFloat( eyeOffsetInfo.interpVal );

	savefile->WriteInt( reinterpret_cast<const int &> ( interpType ) );
	savefile->WriteFloat( lerpScale );
	savefile->WriteObject( self );

	clipBounds.Save( savefile );
}

//================
//hhCameraInterpolator::Restore
//================
void hhCameraInterpolator::Restore( idRestoreGame *savefile ) {
	savefile->ReadQuat( rotationInfo.start );
	savefile->ReadQuat( rotationInfo.end );
	savefile->ReadQuat( rotationInfo.current );
	savefile->ReadFloat( rotationInfo.interpVal );

	savefile->ReadVec3( positionInfo.start );
	savefile->ReadVec3( positionInfo.end );
	savefile->ReadVec3( positionInfo.current );
	savefile->ReadFloat( positionInfo.interpVal );

	savefile->ReadFloat( eyeOffsetInfo.start );
	savefile->ReadFloat( eyeOffsetInfo.end );
	savefile->ReadFloat( eyeOffsetInfo.current );
	savefile->ReadFloat( eyeOffsetInfo.interpVal );

	savefile->ReadInt( reinterpret_cast<int &> ( interpType ) );
	savefile->ReadFloat( lerpScale );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( self ) );

	clipBounds.Restore( savefile );
}

//================
//hhCameraInterpolator::WriteToSnapshot
//================
void hhCameraInterpolator::WriteToSnapshot( idBitMsgDelta &msg, const hhPlayer *pl ) const
{
	idCQuat sq, q;

	idVec3 plPos = ((idPlayer *)pl)->GetPlayerPhysics()->GetOrigin();
#if 1
	sq = rotationInfo.start.ToCQuat();
	msg.WriteFloat(sq.x);
	msg.WriteFloat(sq.y);
	msg.WriteFloat(sq.z);
	q = rotationInfo.current.ToCQuat();
	msg.WriteDeltaFloat(sq.x, q.x);
	msg.WriteDeltaFloat(sq.y, q.y);
	msg.WriteDeltaFloat(sq.z, q.z);
	q = rotationInfo.end.ToCQuat();
	msg.WriteDeltaFloat(sq.x, q.x);
	msg.WriteDeltaFloat(sq.y, q.y);
	msg.WriteDeltaFloat(sq.z, q.z);
	msg.WriteFloat(rotationInfo.interpVal, 4, 4);

	msg.WriteDeltaFloat(plPos[0], positionInfo.start[0]);
	msg.WriteDeltaFloat(plPos[1], positionInfo.start[1]);
	msg.WriteDeltaFloat(plPos[2], positionInfo.start[2]);
	msg.WriteDeltaFloat(positionInfo.start[0], positionInfo.current[0]);
	msg.WriteDeltaFloat(positionInfo.start[1], positionInfo.current[1]);
	msg.WriteDeltaFloat(positionInfo.start[2], positionInfo.current[2]);
	msg.WriteDeltaFloat(positionInfo.start[0], positionInfo.end[0]);
	msg.WriteDeltaFloat(positionInfo.start[1], positionInfo.end[1]);
	msg.WriteDeltaFloat(positionInfo.start[2], positionInfo.end[2]);
	msg.WriteFloat(positionInfo.interpVal, 4, 4);

	msg.WriteFloat(eyeOffsetInfo.start);
	msg.WriteDeltaFloat(eyeOffsetInfo.start, eyeOffsetInfo.current);
	msg.WriteDeltaFloat(eyeOffsetInfo.start, eyeOffsetInfo.end);
	msg.WriteFloat(eyeOffsetInfo.interpVal, 4, 4);
#else
	q = rotationInfo.current.ToCQuat();
	msg.WriteFloat(q.x);
	msg.WriteFloat(q.y);
	msg.WriteFloat(q.z);

	msg.WriteDeltaFloat(plPos[0], positionInfo.current[0]);
	msg.WriteDeltaFloat(plPos[1], positionInfo.current[1]);
	msg.WriteDeltaFloat(plPos[2], positionInfo.current[2]);

	msg.WriteFloat(eyeOffsetInfo.current);
#endif

	//msg.WriteFloat(lerpScale);
}

//================
//hhCameraInterpolator::ReadFromSnapshot
//================
void hhCameraInterpolator::ReadFromSnapshot( const idBitMsgDelta &msg, hhPlayer *pl )
{
	idCQuat sq, q;

	idVec3 plPos = pl->GetPlayerPhysics()->GetOrigin();

#if 1
	sq.x = msg.ReadFloat();
	sq.y = msg.ReadFloat();
	sq.z = msg.ReadFloat();
	rotationInfo.start = sq.ToQuat();
	q.x = msg.ReadDeltaFloat(sq.x);
	q.y = msg.ReadDeltaFloat(sq.y);
	q.z = msg.ReadDeltaFloat(sq.z);
	rotationInfo.current = q.ToQuat();
	q.x = msg.ReadDeltaFloat(sq.x);
	q.y = msg.ReadDeltaFloat(sq.y);
	q.z = msg.ReadDeltaFloat(sq.z);
	rotationInfo.end = q.ToQuat();
	rotationInfo.interpVal = msg.ReadFloat(4, 4);

	positionInfo.start[0] = msg.ReadDeltaFloat(plPos[0]);
	positionInfo.start[1] = msg.ReadDeltaFloat(plPos[1]);
	positionInfo.start[2] = msg.ReadDeltaFloat(plPos[2]);
	positionInfo.current[0] = msg.ReadDeltaFloat(positionInfo.start[0]);
	positionInfo.current[1] = msg.ReadDeltaFloat(positionInfo.start[1]);
	positionInfo.current[2] = msg.ReadDeltaFloat(positionInfo.start[2]);
	positionInfo.end[0] = msg.ReadDeltaFloat(positionInfo.start[0]);
	positionInfo.end[1] = msg.ReadDeltaFloat(positionInfo.start[1]);
	positionInfo.end[2] = msg.ReadDeltaFloat(positionInfo.start[2]);
	positionInfo.interpVal = msg.ReadFloat(4, 4);

	eyeOffsetInfo.start = msg.ReadFloat();
	eyeOffsetInfo.current = msg.ReadDeltaFloat(eyeOffsetInfo.start);
	eyeOffsetInfo.end = msg.ReadDeltaFloat(eyeOffsetInfo.start);
	eyeOffsetInfo.interpVal = msg.ReadFloat(4, 4);
#else
	q.x = msg.ReadFloat();
	q.y = msg.ReadFloat();
	q.z = msg.ReadFloat();
	rotationInfo.current = q.ToQuat();

	positionInfo.current[0] = msg.ReadDeltaFloat(plPos[0]);
	positionInfo.current[1] = msg.ReadDeltaFloat(plPos[1]);
	positionInfo.current[2] = msg.ReadDeltaFloat(plPos[2]);

	eyeOffsetInfo.current = msg.ReadFloat();

	rotationInfo.start = rotationInfo.current;
	rotationInfo.end = rotationInfo.current;
	positionInfo.start = positionInfo.current;
	positionInfo.end = positionInfo.current;
	eyeOffsetInfo.start = eyeOffsetInfo.current;
	eyeOffsetInfo.end = eyeOffsetInfo.current;
#endif
	//lerpScale = msg.ReadFloat();
}
