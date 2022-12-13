#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../Prey/prey_local.h"

const float c_fTraceOriginOffset = 2.0f;
const float c_fGroundTraceDistance = 50.0f + c_fTraceOriginOffset;

#define PLAYER_DEBUG if(p_playerPhysicsDebug.GetInteger() >= 3) gameLocal.Printf
extern int c_pmove;
#define PLAYER_CONTACT_EPSILON		1.25f		// The default contact epsilon wasn't big enough to recognize
												// ground contacts when walking on the ultra-convex planets
#define PLAYER_INWARD_CONTACT_EPSILON	16.0f	//rww - for extra inward gravity ground check

/**********************************************************************

hhPhysics_Player

**********************************************************************/
CLASS_DECLARATION( idPhysics_Player, hhPhysics_Player )
END_CLASS

/*
================
hhPhysics_Player::hhPhysics_Player
================
*/
hhPhysics_Player::hhPhysics_Player( void ): idPhysics_Player() {
	current.axis.Identity();
	current.localAxis.Identity();
	saved.axis.Identity();
	saved.localAxis.Identity();

	ClipModelRotationOrigin.Zero();

	IsWallWalking( false );
	WasWallWalking( false );
	ShouldRemainAlignedToAxial( true );
	OrientToGravity( false );
	SetSlopeCheck( true );
	iInwardGravity = 0; //rww

	castSelf = NULL;
	castSelfGeneric = NULL;
	camera = NULL;

	bMoveNextFrame = false; //HUMANHEAD rww

	for( int iIndex = 0; iIndex < c_iNumWallwalkTraces; ++iIndex ) {
		wallwalkTraceOriginTable[iIndex].Zero();
	}

	for( int iIndex = 0; iIndex < c_iNumRotationTraces; ++iIndex ) {
		rotationTraceDirectionTable[iIndex].Zero();
	}

	oldOrigin.Zero();
	oldAxis.Identity();

	//HUMANHEAD PCF rww 05/11/06 - hack fix for stuck bugs
	stuckCount = 0;
	stuckOrigin.Zero();
	//HUMANHEAD END
}

/*
================
hhPhysics_Player::SetSelf
================
*/
void hhPhysics_Player::SetSelf( idEntity *e ) {
	idPhysics_Player::SetSelf( e );

	if( e && e->IsType( hhPlayer::Type ) ) {
		castSelf = static_cast<hhPlayer*>(e);
		camera = &(castSelf->cameraInterpolator);
	}
	else { //rww
		castSelfGeneric = e;
	}
}

/*
================
hhPhysics_Player::SaveState
================
*/
void hhPhysics_Player::SaveState( void ) {
	saved = current;
	saved.axis = clipModelAxis;
}

/*
================
hhPhysics_Player::RestoreState
================
*/
void hhPhysics_Player::RestoreState( void ) {
	current = saved;

	SetOrigin( current.origin );
	SetAxis( current.axis );

	EvaluateContacts();
}

/*
================
hhPhysics_Player::EvaluateOwnerCamera
================
*/
void hhPhysics_Player::EvaluateOwnerCamera( const int timeStep ) {
	if( camera ) {
		camera->Evaluate( MS2SEC(timeStep) );
	}
}

/*
================
hhPhysics_Player::SetOwnerCameraTarget
================
*/
void hhPhysics_Player::SetOwnerCameraTarget( const idVec3& Origin, const idMat3& Axis, int iInterpFlags ) {
	if( camera && castSelf ) {
		camera->UpdateTarget( Origin, Axis, castSelf->EyeHeightIdeal(), iInterpFlags );
	}
	else if (castSelfGeneric && castSelfGeneric->IsType(idActor::Type)) { //rww - update the view axis for non-player actors
		idActor *actor = static_cast<idActor *>(castSelfGeneric);
		idVec3 fwd = actor->viewAxis[0];
		idVec3 up = Axis[2];
		fwd.ProjectOntoPlane(up);
		fwd.Normalize();
		idVec3 right = up.Cross(fwd);
		right.Normalize();

		idMat3 view(fwd, right, up);
		actor->viewAxis = view;
	}
}

/*
================
hhPhysics_Player::LinkClip
================
*/
void hhPhysics_Player::LinkClip( const idVec3& Origin, const idMat3& Axis ) {
	if( clipModel ) {
		clipModel->Link( gameLocal.clip, self, 0, Origin, Axis );
	}
}

/*
================
hhPhysics_Player::Translate
================
*/
void hhPhysics_Player::Translate( const idVec3 &translation, int id ) {
	//SetOrigin( current.origin + translation );
	current.localOrigin += translation;
	current.origin += translation;

	LinkClip( current.origin, clipModelAxis );
}

/*
================
hhPhysics_Player::Rotate
================
*/
void hhPhysics_Player::Rotate( const idRotation& rotation, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.origin *= rotation;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localOrigin = ( current.origin - masterOrigin ) * masterAxis.Transpose();
	}
	else {
		current.localOrigin = current.origin;
	}


	clipModelAxis *= rotation.ToMat3();
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.localAxis = clipModelAxis * masterAxis.Transpose();
	}
	else {
		current.localAxis = clipModelAxis;
	}

	idPhysics* physics = (groundTrace.fraction < 1.0f) ? gameLocal.entities[groundTrace.c.entityNum]->GetPhysics() : NULL;
	if( IsWallWalking() && physics && (physics->IsType(idPhysics_Parametric::Type) || physics->IsType(idPhysics_Actor::Type)) ) {
		SetOwnerCameraTarget( GetOrigin(), GetAxis(), INTERPOLATE_POSITION );
	}
}

/*
================
hhPhysics_Player::SetOrigin
================
*/
void hhPhysics_Player::SetOrigin( const idVec3 &newOrigin, int id ) {
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localOrigin = newOrigin;
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + newOrigin * masterAxis;
	}
	else {
		current.origin = newOrigin;
	}

	assert(!FLOAT_IS_NAN(current.origin[0])); //HUMANHEAD rww
	assert(!FLOAT_IS_NAN(current.origin[1])); //HUMANHEAD rww
	assert(!FLOAT_IS_NAN(current.origin[2])); //HUMANHEAD rww

	LinkClip( newOrigin, GetAxis() );
}

/*
================
hhPhysics_Player::GetOrigin
================
*/
const idVec3 &hhPhysics_Player::GetOrigin( int id ) const {
	return current.origin;
}

/*
================
hhPhysics_Player::SetAxis
================
*/
void hhPhysics_Player::SetAxis( const idMat3 &newAxis, int id ) {
#if 1 //Player rotation around master test
	clipModelAxis = newAxis;
#else
	idVec3 masterOrigin;
	idMat3 masterAxis;

	current.localAxis = newAxis;
	if ( masterEntity /*&& isOrientated*/ ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		clipModelAxis = current.localAxis * masterAxis;
	}
	else {
		clipModelAxis = newAxis;
	}
#endif
	LinkClip( GetOrigin(), newAxis );
}

/*
================
hhPhysics_Player::GetAxis
================
*/
const idMat3& hhPhysics_Player::GetAxis( int id ) const {
	return clipModelAxis;
}

/*
================
hhPhysics_Player::OrientToGravity
================
*/
void hhPhysics_Player::OrientToGravity( bool orientToGravity ) {
	//Assumption:  hhGravityZone is the only thing that will call this function
	this->orientToGravity = IsWallWalking() ? true : orientToGravity;
}

/*
================
hhPhysics_Player::OrientToGravity
================
*/
bool hhPhysics_Player::OrientToGravity() const {
	return orientToGravity;
}

/*
================
hhPhysics_Player::ClipTranslation
================
*/
void hhPhysics_Player::ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.TranslationModel( results, current.origin, current.origin + translation,
								clipModel, clipModelAxis, clipMask,
								model->Handle(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		if (self && self->IsType(hhSpiritProxy::Type)) { //rww - spirit prox checks for collision exceptions
			gameLocal.clip.TranslationWithExceptions( results, current.origin, current.origin + translation, self,
									clipModel, clipModelAxis, clipMask, NULL );
		}
		else {
			gameLocal.clip.Translation( results, current.origin, current.origin + translation,
									clipModel, clipModelAxis, clipMask, NULL );
		}
	}
}

/*
================
hhPhysics_Player::ClipRotation
================
*/
void hhPhysics_Player::ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const {
	if ( model ) {
		gameLocal.clip.RotationModel( results, current.origin, rotation,
								clipModel, clipModelAxis, clipMask,
								model->Handle(), model->GetOrigin(), model->GetAxis() );
	}
	else {
		gameLocal.clip.Rotation( results, current.origin, rotation,
								clipModel, clipModelAxis, clipMask, NULL );
	}
}

/*
================
hhPhysics_Player::IsGroundEntity
================
*/
bool hhPhysics_Player::IsGroundEntity( int entityNum ) const {
	int i;

	for ( i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].entityNum == entityNum && (contacts[i].normal * clipModelAxis[2]) > 0.0f ) {
			return true;
		}
	}
	return false;
}

/*
================
hhPhysics_Player::IsGroundClipModel
================
*/
bool hhPhysics_Player::IsGroundClipModel( int entityNum, int id ) const {
	int i;

	for ( i = 0; i < contacts.Num(); i++ ) {
		if ( contacts[i].entityNum == entityNum && contacts[i].id == id && (contacts[i].normal * clipModelAxis[2]) > 0.0f ) {
			return true;
		}
	}
	return false;
}

/*
================
hhPhysics_Player::HasGroundContacts
================
*/
bool hhPhysics_Player::HasGroundContacts( void ) const {
	int i;

	for ( i = 0; i < contacts.Num(); i++ ) {
		if ( (contacts[i].normal * clipModelAxis[2]) > 0.0f ) {
			return true;
		}
	}
	return false;
}

/*
================
hhPhysics_Player::ExtraGroundCheck
rww - kind of ugly, do an extra ground check to avoid triggering land anim in inward gravity
returns true if our high epsilon check is touching anything
================
*/
bool hhPhysics_Player::ExtraGroundCheck(idClipModel *customClipModel) {
	idVec6 dir;
	contactInfo_t contact;
	int num;
	if (!customClipModel) {
		customClipModel = clipModel;
	}

	dir.SubVec3(0) = gravityNormal;
	dir.SubVec3(1) = vec3_origin;
	num = gameLocal.clip.Contacts( &contact, 1, current.origin,
					dir, PLAYER_INWARD_CONTACT_EPSILON, clipModel, clipModelAxis, clipMask, self );

	return (num > 0);
}


/*
================
hhPhysics_Player::AddGroundContacts
================
*/
void hhPhysics_Player::AddGroundContacts( const idClipModel *clipModel ) {
	idVec6 dir;
	int index, num;

	index = contacts.Num();
	contacts.SetNum( index + 10, false );

	dir.SubVec3(0) = gravityNormal;
	dir.SubVec3(1) = vec3_origin;
	// HUMANHEAD mdl: changed CONTACT_EPSILON to GetContactEpsilon() below
	num = gameLocal.clip.Contacts( &contacts[index], 10, current.origin,
					dir, GetContactEpsilon(), clipModel, clipModelAxis, clipMask, self );
	contacts.SetNum( index + num, false );
}

/*
================
hhPhysics_Player::EvaluateContacts
================
*/
bool hhPhysics_Player::EvaluateContacts( void ) {

	// get all the contacts
	ClearContacts();

	AddGroundContacts( clipModel );

	return ( contacts.Num() != 0 );
}

/*
================
hhPhysics_Player::SetKnockBack
================
*/
void hhPhysics_Player::SetKnockBack( const int knockBackTime ) {
	if( IsWallWalking() || current.movementTime ) {
		return;
	}
	current.movementFlags |= PMF_TIME_KNOCKBACK;
	current.movementTime = knockBackTime;
}

/*
================
hhPhysics_Player::ApplyImpulse
================
*/
void hhPhysics_Player::ApplyImpulse( const int id, const idVec3 &point, const idVec3 &impulse ) {
	//HUMANHEAD: aob - so we can use forces on player (crane)
	//FIXME: aob - impulses are actually time dependent.  See about fixing this.
	if ( self && self->IsType( hhSpiritProxy::Type ) ) { // HUMANHEAD cjr:  Don't apply impulses if the actor is a spirit/death proxy
		return;
	}

	current.velocity += (impulse / GetMass());// * MS2SEC( gameLocal.msec );
	//HUMANHEAD END
}

/*
================
hhPhysics_Player::ForceCrouching
================
*/
void hhPhysics_Player::ForceCrouching() {
	idBounds bounds;
	float maxZ = 0.0f;

	if( IsCrouching() ) {
		return;
	}

	current.movementFlags |= PMF_DUCKED;

	playerSpeed = crouchSpeed;
	maxZ = pm_crouchheight.GetFloat();
		
	// if the clipModel height should change
	if ( clipModel->GetBounds()[1][2] != maxZ ) {
		bounds = clipModel->GetBounds();
		bounds[1][2] = maxZ;
		clipModel->LoadModel( idTraceModel( bounds ) );
	}
}

/*
================
hhPhysics_Player::PerformGroundTraces
================
*/
void hhPhysics_Player::PerformGroundTrace( trace_t& TraceInfo, const idVec3& Start ) {
	idVec3 End = Start + (-clipModelAxis[2]) * c_fGroundTraceDistance;

	gameLocal.clip.TracePoint(TraceInfo, Start, End, clipMask, self);

	if(p_playerPhysicsDebug.GetInteger() == 1) {
		gameRenderWorld->DebugLine(colorRed, Start, Start + (-clipModelAxis[2]) * c_fGroundTraceDistance * TraceInfo.fraction);
	}
}

/*
=============
hhPhysics_Player::BuildWallwalkTraceOriginTable
	Currently does 5 traces, one from each corner of the clip model and one from the center
=============
*/
void hhPhysics_Player::BuildWallwalkTraceOriginTable( const idMat3& Axis ) {
	assert( clipModel );

#if !GOLD //rww - this happened on a single client during a listen server mp test on dmwallwalk2, need more info on it (stack was not available)
	if (!clipModel) {
		int ownerNum = -1;
		int localNum = -1;
		if (self) {
			ownerNum = self->entityNumber;
		}
		if (gameLocal.GetLocalPlayer()) {
			localNum = gameLocal.GetLocalPlayer()->entityNumber;
		}
		gameLocal.Error("hhPhysics_Player::BuildWallwalkTraceOriginTable NULL clipModel for entity %i (local %i).", ownerNum, localNum);
	}
#endif

	idVec3 UpVector = Axis[2];
	idBounds ClipModelBounds( clipModel->GetBounds() );

	wallwalkTraceOriginTable[1].Set(ClipModelBounds[0].x, ClipModelBounds[0].y, ClipModelBounds[0].z);
	wallwalkTraceOriginTable[2].Set(ClipModelBounds[0].x, ClipModelBounds[1].y, ClipModelBounds[0].z);
	wallwalkTraceOriginTable[3].Set(ClipModelBounds[1].x, ClipModelBounds[1].y, ClipModelBounds[0].z);
	wallwalkTraceOriginTable[4].Set(ClipModelBounds[1].x, ClipModelBounds[0].y, ClipModelBounds[0].z);

	for(int iIndex = 1; iIndex < c_iNumWallwalkTraces; ++iIndex) {
		wallwalkTraceOriginTable[iIndex] = current.origin + wallwalkTraceOriginTable[iIndex].x * clipModelAxis[0] + wallwalkTraceOriginTable[iIndex].y * clipModelAxis[1] + wallwalkTraceOriginTable[iIndex].z * clipModelAxis[2]; 
		wallwalkTraceOriginTable[iIndex] = wallwalkTraceOriginTable[iIndex] + (UpVector * c_fTraceOriginOffset);
	}
	wallwalkTraceOriginTable[0] = current.origin + (UpVector * c_fTraceOriginOffset);
}

/*
=============
hhPhysics_Player::EvaluateGroundTrace
=============
*/
bool hhPhysics_Player::EvaluateGroundTrace( const trace_t& TraceInfo ) {
	surfTypes_t surfType;
	if (g_debugMatter.GetInteger() >= 2) {
		surfType = gameLocal.GetMatterType(TraceInfo, "hhPhysics_Player::EvaluateGroundTrace");
	}
	else {
		surfType = gameLocal.GetMatterType(TraceInfo, NULL);
	}
	return (surfType == SURFTYPE_WALLWALK);
}

/*
=============
hhPhysics_Player::TranslateMove
=============
*/
void hhPhysics_Player::TranslateMove( const idVec3& TranslationVector, const float fDist ) {
	trace_t TraceInfo;

	gameLocal.clip.Translation(TraceInfo, current.origin, current.origin + TranslationVector * fDist, clipModel, clipModelAxis, clipMask, self);
	PLAYER_DEBUG("TranslationTrace.fraction: %f\n", TraceInfo.fraction);

	SetOrigin( TraceInfo.endpos );
	SetOwnerCameraTarget( GetOrigin(), GetAxis(), INTERPMASK_WALLWALK );
}

/*
=============
hhPhysics_Player::RotateMove
=============
*/
bool hhPhysics_Player::RotateMove( const idVec3& UpVector, const idVec3& IdealUpVector, const idVec3& RotationOrigin, const idVec3& RotationCheckOrigin ) {
	idRotation Rotator;
	idVec3 RotationVector;
	trace_t TraceInfo;

	if( IdealUpVector.Compare(vec3_origin, VECTOR_EPSILON) ) {// For Zero G
		return false;
	}

	if( UpVector.Compare(IdealUpVector, VECTOR_EPSILON) ) {
		return false;
	}

	Rotator.SetVec( DetermineRotationVector(UpVector, IdealUpVector, RotationCheckOrigin) );
	Rotator.SetOrigin( RotationOrigin );
	Rotator.SetAngle( RAD2DEG(idMath::ACos(UpVector * IdealUpVector)) );
	Rotator.ToMat3();

	gameLocal.clip.Rotation( TraceInfo, current.origin, Rotator, clipModel, clipModelAxis, clipMask, self );

	if( TraceInfo.fraction == 0.0f ) {
		//Try to move ourselves till we fit
		IterativeRotateMove( UpVector, IdealUpVector, GetOrigin(), RotationCheckOrigin, p_iterRotMoveNumIterations.GetInteger() );
		return false;
	}

	PLAYER_DEBUG( "UpVector: %s, IdealUpVector: %s\n", UpVector.ToString(), IdealUpVector.ToString() );

	SetOrigin( TraceInfo.endpos );
	SetAxis( TraceInfo.endAxis );

	LinkClip( current.origin, clipModelAxis );

	PLAYER_DEBUG( "RotationTrace.fraction: %f\n", TraceInfo.fraction );

	SetOwnerCameraTarget( GetOrigin(), GetAxis(), INTERPMASK_WALLWALK );

	return true;
}

/*
================
hhPhysics_Player::InterativeRotateMove
================
*/
bool hhPhysics_Player::IterativeRotateMove( const idVec3& UpVector, const idVec3& IdealUpVector, const idVec3& RotationOrigin, const idVec3& RotationCheckOrigin, int iNumIterations) {
	if( !orientToGravity ) {
		return false;
	}

	if( !idPhysics_Player::IterativeRotateMove(UpVector, IdealUpVector, RotationOrigin, RotationCheckOrigin, iNumIterations) ) {
		return false;
	}

	SetOwnerCameraTarget( GetOrigin(), GetAxis(), INTERPMASK_ALL );

	return true;
}

/*
================
hhPhysics_Player::WallWalkIsAllowed
================
*/
bool hhPhysics_Player::WallWalkIsAllowed() const {
	return	HasGroundContacts() && 
			castSelf && !castSelf->IsSpiritWalking() &&
			castSelf->inventory.requirements.bCanWallwalk;
}

/*
================
hhPhysics_Player::CheckWallWalk
================
*/
void hhPhysics_Player::CheckWallWalk( bool bForce ) {
	trace_t TraceInfo;

	assert( self );

	if( bForce || WallWalkIsAllowed() ) {
		BuildWallwalkTraceOriginTable( clipModelAxis );
	
		for( int iIndex = 0; iIndex < c_iNumWallwalkTraces; ++iIndex ) {
			PerformGroundTrace( TraceInfo, wallwalkTraceOriginTable[iIndex] );

			if( EvaluateGroundTrace(TraceInfo) ) {
				SetGravity( -TraceInfo.c.normal * hhUtils::GetLocalGravity(GetOrigin(), GetBounds(), gameLocal.GetGravity()).Length() );
				ClipModelRotationOrigin = wallwalkTraceOriginTable[iIndex];
				IsWallWalking( true );
				ShouldRemainAlignedToAxial( false );
				OrientToGravity( true );
				return;
			}
		}
	}

	if( IsWallWalking() ) {
		SetGravity( hhUtils::GetLocalGravity(GetOrigin(), GetBounds(), gameLocal.GetGravity()) );
		ClipModelRotationOrigin = GetOrigin();
		IsWallWalking( false );
		ShouldRemainAlignedToAxial( true );
	}
}

/*
=============
hhPhysics_Player::DetermineJumpVelocity
=============
*/
idVec3 hhPhysics_Player::DetermineJumpVelocity( void ) {
	idVec3 addVelocity;

	addVelocity = (IsWallWalking() ? 0.20f : 2.0f) * maxJumpHeight * -gravityVector;
	addVelocity *= idMath::Sqrt( addVelocity.Normalize() );

	return addVelocity;
}

/*
================
hhPhysics_Player::SetPushed
================
*/
void hhPhysics_Player::SetPushed( int deltaTime ) {
	//Stopping pushVelocity from being set while on wallwalking mover
	idPhysics* physics = (groundTrace.fraction < 1.0f) ? gameLocal.entities[groundTrace.c.entityNum]->GetPhysics() : NULL;
	bool parametricPhysics = (physics) ? physics->IsType(idPhysics_Parametric::Type) : false;
	if( parametricPhysics && IsWallWalking() ) {
        return;		
	}

	idPhysics_Player::SetPushed( deltaTime );
}

/*
===================
hhPhysics_Player::WalkMove
===================
*/
void hhPhysics_Player::WalkMove( void ) {
	idPhysics_Player::WalkMove();

	//Used to smooth out steps
	SetOwnerCameraTarget( GetOrigin(), GetAxis(), (hhMath::Fabs(GetStepUp()) >= (pm_stepsize.GetFloat() * 0.5f)) ? INTERPMASK_WALLWALK : INTERPOLATE_ROTATION );

	//Force wallwalk check for spiritProxy
	CheckWallWalk( !castSelf );
}

/*
===================
hhPhysics_Player::AirMove
===================
*/
void hhPhysics_Player::AirMove( void ) {
	idPhysics_Player::AirMove();

	if( IsWallWalking() && groundTrace.fraction == 1.0f ) {
		PLAYER_DEBUG("Align with gameLocal.gravity\n");
		SetGravity( hhUtils::GetLocalGravity(GetOrigin(), GetBounds(), gameLocal.GetGravity()) );
		ClipModelRotationOrigin = current.origin;
		IsWallWalking( false );
		ShouldRemainAlignedToAxial( true );
	}

	SetOwnerCameraTarget( GetOrigin(), GetAxis(), INTERPOLATE_ROTATION );
}

/*
=============
hhPhysics_Player::CheckGround
=============
*/
void hhPhysics_Player::CheckGround( void ) {
	int i, contents;
	idVec3 point;
	bool hadGroundContacts;

	hadGroundContacts = HasGroundContacts();

	// set the clip model origin before getting the contacts
	clipModel->SetPosition( current.origin, clipModelAxis );

	EvaluateContacts();

	// setup a ground trace from the contacts
	groundTrace.endpos = current.origin;
	groundTrace.endAxis = clipModelAxis;
	if ( contacts.Num() ) {
		groundTrace.fraction = 0.0f;
		groundTrace.c = contacts[0];
		for ( i = 1; i < contacts.Num(); i++ ) {
			groundTrace.c.normal += contacts[i].normal;
		}
		groundTrace.c.normal.Normalize();
	}
	else {
		groundTrace.fraction = 1.0f;
	}

	contents = gameLocal.clip.Contents( current.origin, clipModel, clipModelAxis, GetClipMask(), self );
	if ( contents & GetClipMask() ) { // HUMANHEAD cjr:  added GetClipMask() instead of hardcoded MASK_SOLID - was causing problems with spiritwalk
		// do something corrective if stuck in solid
		CorrectAllSolid( groundTrace, contents );
	}

	// if the trace didn't hit anything, we are in free fall
	if ( groundTrace.fraction == 1.0f ) {
		groundPlane = false;
		walking = false;
		groundMaterial = NULL;// HUMANHEAD: aob - if freefalling then no groundMaterial
		//HUMANHEAD PCF rww 05/11/06 - hack fix for stuck bugs
		stuckCount = 0;
		//HUMANHEAD END
		return;
	}

	groundMaterial = groundTrace.c.material;

	// check if getting thrown off the ground
	float fVelGravNormDot = (current.velocity * -gravityNormal);
	float fVelTrcNormDot = ( current.velocity * groundTrace.c.normal );
	if ( fVelGravNormDot > 0.0f && fVelTrcNormDot > 10.0f && ShouldSlopeCheck()) {
		if ( debugLevel ) {
			gameLocal.Printf( "%i:kickoff (gravdot:%.2f, tracedot:%.2f\n", c_pmove, fVelGravNormDot, fVelTrcNormDot );
		}

		groundPlane = false;
		walking = false;
		//HUMANHEAD PCF rww 05/11/06 - hack fix for stuck bugs
		stuckCount = 0;
		//HUMANHEAD END
		return;
	}
	
	// slopes that are too steep will not be considered onground
	if ( ( groundTrace.c.normal * -gravityNormal ) < MIN_WALK_NORMAL && ShouldSlopeCheck() ) {
		if ( debugLevel ) {
			gameLocal.Printf( "%i:steep\n", c_pmove );
		}
		// FIXME: if they can't slide down the slope, let them walk (sharp crevices)
		//HUMANHEAD PCF rww 05/11/06 - hack fix for stuck bugs
		stuckCount++;
		if ((current.origin-stuckOrigin).Length() > 1.0f) {
			stuckCount = 0;
			stuckOrigin = current.origin;
		}
		else if (stuckCount > 256) {
			stuckCount = 0;
			current.velocity = groundTrace.c.normal*512.0f;
		}
		//HUMANHEAD END
		groundPlane = true;
		walking = false;
		return;
	}

	//HUMANHEAD PCF rww 05/11/06 - hack fix for stuck bugs
	stuckCount = 0;
	//HUMANHEAD END

	groundPlane = true;
	walking = true;

	// hitting solid ground will end a waterjump
	if ( current.movementFlags & PMF_TIME_WATERJUMP ) {
		current.movementFlags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		current.movementTime = 0;
	}

	// if the player didn't have ground contacts the previous frame
	if ( !hadGroundContacts ) {

		// don't do landing time if we were just going down a slope
		if ( (current.velocity * -gravityNormal) < -200 ) {
			// don't allow another jump for a little while
			current.movementFlags |= PMF_TIME_LAND;
			current.movementTime = 250;
		}
#if VENOM
		//HUMANHEAD PCF rww 05/16/06 - negate velocity on the gravity normal,
		//if the velocity is going against the ground normal
		idVec3 gravVel = (current.velocity * gravityNormal) * gravityNormal;
		idVec3 nGravVel = gravVel;
		nGravVel.Normalize();
		if (nGravVel*groundTrace.c.normal < 0.0f) {
			current.velocity -= gravVel;
		}
		//HUMANHEAD END
#endif
	}

	// let the entity know about the collision
	self->Collide( groundTrace, vec3_origin );
}

/*
================
hhPhysics_Player::Evaluate
================
*/
bool hhPhysics_Player::Evaluate( int timeStepMSec, int endTimeMSec ) {
	PROFILE_SCOPE("Player", PROFMASK_PHYSICS);
	idVec3 masterOrigin;
	idMat3 masterAxis;
	//HUMANHEAD: aob
	bool bPositionChanged = false;
	//HUMANHEAD END

	waterLevel = WATERLEVEL_NONE;
	waterType = 0;
	oldOrigin = current.origin;
	//HUMANHEAD: aob
	oldAxis = GetAxis();
	HadGroundContacts( HasGroundContacts() );
	//rww - the way this is done is kind of dumb. as a hack, i'm moving this line below the only (as of current) place that actually checks waswallwalking.
	//(because right now it's possible to teleport off of a wallwalk and miss the transition)
	//WasWallWalking( IsWallWalking() );
	//HUMANHEAD END

	clipModel->Unlink();

	// if bound to a master
	if ( masterEntity ) {
		self->GetMasterPosition( masterOrigin, masterAxis );
		current.origin = masterOrigin + current.localOrigin * masterAxis;

		assert(!FLOAT_IS_NAN(current.origin[0])); //HUMANHEAD rww
		assert(!FLOAT_IS_NAN(current.origin[1])); //HUMANHEAD rww
		assert(!FLOAT_IS_NAN(current.origin[2])); //HUMANHEAD rww

		if ( castSelf && castSelf->InVehicle() ) { // Added castSelf check in case of player proxy -mdl
			clipModelAxis = current.localAxis * masterAxis;
		}
		current.velocity = ( current.origin - oldOrigin ) / ( timeStepMSec * 0.001f );

		assert(!FLOAT_IS_NAN(current.velocity[0])); //HUMANHEAD rww
		assert(!FLOAT_IS_NAN(current.velocity[1])); //HUMANHEAD rww
		assert(!FLOAT_IS_NAN(current.velocity[2])); //HUMANHEAD rww

		masterDeltaYaw = masterYaw;
		masterYaw = masterAxis[0].ToYaw();
		masterDeltaYaw = masterYaw - masterDeltaYaw;
	
		bPositionChanged = true;
	} else {
		ActivateContactEntities();

		MovePlayer( timeStepMSec );

		idVec3 RotationCheckOrigin = GetOrigin() + GetAxis()[2] * GetBounds()[1].z;
		if( !IsWallWalking() ) {
			IterativeRotateMove( GetAxis()[2], -GetGravityNormal(), GetOrigin(), RotationCheckOrigin, p_iterRotMoveNumIterations.GetInteger() );
		} else {
			if( RotateMove(GetAxis()[2], -GetGravityNormal(), ClipModelRotationOrigin, RotationCheckOrigin) && groundTrace.fraction < 1.0f ) {
				TranslateMove( GetGravityNormal(), c_fGroundTraceDistance );	
			}
		}

		bPositionChanged = (GetOrigin() != oldOrigin) || (GetAxis() != oldAxis);
	}

	LinkClip( GetOrigin(), GetAxis() );

	//Needed to update in other movement types like noclip and spectator
	SetOwnerCameraTarget( GetOrigin(), GetAxis(), INTERPOLATE_NONE );
	EvaluateOwnerCamera( timeStepMSec );

	if( p_playerPhysicsDebug.GetInteger() == 2 ) {
		gameRenderWorld->DebugLine(colorRed, GetOrigin(), GetOrigin() + wishdir * 48.0f);
	} 
	
	if( p_playerPhysicsDebug.GetInteger() == 1 ) {
		if(clipModel) {
			gameRenderWorld->DebugBox( colorBlue, idBox(clipModel->GetBounds(), GetOrigin(), GetAxis()) );
		}

		if( camera ) {
			gameRenderWorld->DebugLine( colorRed, GetOrigin(), camera->GetEyePosition() );
		}
	}

	 //HUMANHEAD rww - check bMoveNextFrame for movers
	if (bMoveNextFrame) {
		bMoveNextFrame = false;
		return true;
	}
	//HUMANHEAD END

	return bPositionChanged;
}

/*
================
hhPhysics_Player::SetInwardGravity
================
*/
void hhPhysics_Player::SetInwardGravity(int inwardGravity) {
	//this has gone through lots of revisions!
	if (inwardGravity <= 0) {
		iInwardGravity = 0;
	}
	else {
		iInwardGravity = 1;
	}
}

/*
================
hhPhysics_Player::ShouldRemainAlignedToAxial
================
*/
void hhPhysics_Player::ShouldRemainAlignedToAxial( const bool remainAligned ) {
	shouldRemainAlignedToAxial = IsWallWalking() ? false : remainAligned;
}

/*
================
hhPhysics_Player::ShouldRemainAlignedToAxial
================
*/
bool hhPhysics_Player::ShouldRemainAlignedToAxial() const {
	//Should check to see if wallwalking is on and that we are on a wallwalking surface
	return shouldRemainAlignedToAxial;
}

void hhPhysics_Player::WriteToSnapshot( idBitMsgDelta &msg, bool inVehicle ) const {
	if (!inVehicle) {
		idPhysics_Player::WriteToSnapshot( msg );
	}
	else { //only sync the local origin when in a vehicle -rww
		msg.WriteFloat( current.localOrigin[0] );
		msg.WriteFloat( current.localOrigin[1] );
		msg.WriteFloat( current.localOrigin[2] );
		return;
	}
	
	msg.WriteBits( orientToGravity, 1 );

	msg.WriteFloat( gravityVector.x );
	msg.WriteFloat( gravityVector.y );
	msg.WriteFloat( gravityVector.z );

	msg.WriteBits(shouldRemainAlignedToAxial, 1);
	msg.WriteBits(isWallWalking, 1);
	msg.WriteBits(wasWallWalking, 1);
	//bDoSlopeCheck isn't actually used, so don't bother sending
	//msg.WriteBits(bDoSlopeCheck, 1);
	msg.WriteBits(iInwardGravity, 1);

	msg.WriteFloat(ClipModelRotationOrigin.x);
	msg.WriteFloat(ClipModelRotationOrigin.y);
	msg.WriteFloat(ClipModelRotationOrigin.z);
	/*
	int i = 0;
	while (i < c_iNumWallwalkTraces)
	{
		msg.WriteFloat(wallwalkTraceOriginTable[i].x);
		msg.WriteFloat(wallwalkTraceOriginTable[i].y);
		msg.WriteFloat(wallwalkTraceOriginTable[i].z);

		i++;
	}
	*/

	//rww - axis should be sync'd now, because it is not constant
	idQuat q = clipModelAxis.ToQuat();
	msg.WriteFloat(q.w);
	msg.WriteFloat(q.x);
	msg.WriteFloat(q.y);
	msg.WriteFloat(q.z);
}

void hhPhysics_Player::ReadFromSnapshot( const idBitMsgDelta &msg, bool inVehicle ) {
	if (!inVehicle) {
		idPhysics_Player::ReadFromSnapshot( msg );
	}
	else { //only sync the local origin when in a vehicle -rww
		current.localOrigin[0] = msg.ReadFloat();
		current.localOrigin[1] = msg.ReadFloat();
		current.localOrigin[2] = msg.ReadFloat();

		if ( clipModel ) {
			clipModel->Link( gameLocal.clip, self, 0, current.origin, clipModel->GetAxis() );
		}
		return;
	}
	orientToGravity = ( msg.ReadBits( 1 ) != 0 );

	idVec3 newGrav;
	newGrav.x = msg.ReadFloat();
	newGrav.y = msg.ReadFloat();
	newGrav.z = msg.ReadFloat();

	SetGravity(newGrav);

	//FIXME - go through this and see what doesn't need to be sent
	shouldRemainAlignedToAxial = !!msg.ReadBits(1);
	isWallWalking = !!msg.ReadBits(1);
	wasWallWalking = !!msg.ReadBits(1);
	//bDoSlopeCheck = !!msg.ReadBits(1);
	iInwardGravity = msg.ReadBits(1);

	ClipModelRotationOrigin.x = msg.ReadFloat();
	ClipModelRotationOrigin.y = msg.ReadFloat();
	ClipModelRotationOrigin.z = msg.ReadFloat();
	/*
	int i = 0;
	while (i < c_iNumWallwalkTraces) {
		wallwalkTraceOriginTable[i].x = msg.ReadFloat();
		wallwalkTraceOriginTable[i].y = msg.ReadFloat();
		wallwalkTraceOriginTable[i].z = msg.ReadFloat();

		i++;
	}
	*/

	//rww - axis should be sync'd now, because it is not constant
	idQuat q;
	q.w = msg.ReadFloat();
	q.x = msg.ReadFloat();
	q.y = msg.ReadFloat();
	q.z = msg.ReadFloat();
	clipModelAxis = q.ToMat3();
}

/*
================
hhPhysics_Player::AlignWithGravity
================
*/
bool hhPhysics_Player::AlignedWithGravity() const {
	return clipModelAxis[2].Compare( -gravityNormal, VECTOR_EPSILON );
}

/*
================
hhPhysics_Player::Save
================
*/
void hhPhysics_Player::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( ClipModelRotationOrigin );
	savefile->WriteBool( shouldRemainAlignedToAxial );
	savefile->WriteBool( orientToGravity );
	savefile->WriteBool( isWallWalking );
	savefile->WriteBool( wasWallWalking );
	savefile->WriteVec3( oldOrigin );
	savefile->WriteMat3( oldAxis );
	savefile->WriteBool( bDoSlopeCheck );
	savefile->WriteInt( iInwardGravity );
	savefile->WriteObject( castSelf );
	savefile->WriteObject( castSelfGeneric );
	for( int i = 0; i < c_iNumWallwalkTraces; i++) {
		savefile->WriteVec3( wallwalkTraceOriginTable[i] );
	}
	savefile->WriteBool(bMoveNextFrame); //HUMANHEAD rww
}

/*
================
hhPhysics_Player::Restore
================
*/
void hhPhysics_Player::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( ClipModelRotationOrigin );
	savefile->ReadBool( shouldRemainAlignedToAxial );
	savefile->ReadBool( orientToGravity );
	savefile->ReadBool( isWallWalking );
	savefile->ReadBool( wasWallWalking );
	savefile->ReadVec3( oldOrigin );
	savefile->ReadMat3( oldAxis );
	savefile->ReadBool( bDoSlopeCheck );
	savefile->ReadInt( iInwardGravity );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( castSelf ) );
	savefile->ReadObject( reinterpret_cast<idClass *&> ( castSelfGeneric ) );
	for( int i = 0; i < c_iNumWallwalkTraces; i++) {
		savefile->ReadVec3( wallwalkTraceOriginTable[i] );
	}
	savefile->ReadBool(bMoveNextFrame); //HUMANHEAD rww

	if( castSelf ) {
		camera = &(castSelf->cameraInterpolator);
	} else {
		camera = NULL;
	}
}

float hhPhysics_Player::GetContactEpsilon(void) const {
	// If we're wallwalking (and not jumping off of wallwalk), use a higher epsilon to keep us firmly on the wallwalk.
	if (IsWallWalking() && command.upmove < 10) {
		return PLAYER_INWARD_CONTACT_EPSILON;
	} else {
		return PLAYER_CONTACT_EPSILON;
	}
}

