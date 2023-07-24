// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "IK.h"
#include "Entity.h"
#include "anim/Anim.h"
#include "Player.h"
#include "Mover.h"

/*
===============================================================================

  idIK

===============================================================================
*/

CLASS_DECLARATION( idClass, idIK )
END_CLASS

/*
================
idIK::idIK
================
*/
idIK::idIK( void ) {
	ik_activate = false;
	initialized = false;
	self = NULL;
	animator = NULL;
	modifiedAnim = 0;
	modelOffset.Zero();
}

/*
================
idIK::~idIK
================
*/
idIK::~idIK( void ) {
}

/*
================
idIK::IsInitialized
================
*/
bool idIK::IsInitialized( void ) const {
	return initialized && ik_enable.GetBool();
}

/*
================
idIK::IsInhibited
================
*/
bool idIK::IsInhibited( void ) const {
	return gameLocal.isClient && ( self->aorFlags & AOR_INHIBIT_IK );
}

/*
================
idIK::GetPhysics
================
*/
idPhysics* idIK::GetPhysics() {
	return self->GetPhysics();
}

/*
================
idIK::GetAnimator
================
*/
idAnimator* idIK::GetAnimator() {
	return animator;
}

/*
================
idIK::Init
================
*/
bool idIK::Init( idEntity *self, const char *anim, const idVec3 &modelOffset ) {
	idRenderModel *model;

	if ( self == NULL ) {
		return false;
	}

	this->self = self;

	animator = self->GetAnimator();
	if ( animator == NULL || animator->ModelDef() == NULL ) {
		gameLocal.Warning( "idIK::Init: IK for entity '%s' at (%s) has no model set.",
							self->name.c_str(), self->GetPhysics()->GetOrigin().ToString(0) );
		return false;
	}
	if ( animator->ModelDef()->ModelHandle() == NULL ) {
		gameLocal.Warning( "idIK::Init: IK for entity '%s' at (%s) uses default model.",
							self->name.c_str(), self->GetPhysics()->GetOrigin().ToString(0) );
		return false;
	}
	model = animator->ModelHandle();
	if ( model == NULL ) {
		gameLocal.Warning( "idIK::Init: IK for entity '%s' at (%s) has no model set.",
							self->name.c_str(), self->GetPhysics()->GetOrigin().ToString(0) );
		return false;
	}
	modifiedAnim = animator->GetAnim( anim );
	if ( modifiedAnim == 0 ) {
		gameLocal.Warning( "idIK::Init: IK for entity '%s' at (%s) has no modified animation.",
								self->name.c_str(), self->GetPhysics()->GetOrigin().ToString(0) );
		return false;
	}
	
	this->modelOffset = modelOffset;

	return true;
}

/*
================
idIK::Evaluate
================
*/
bool idIK::Evaluate( void ) {
	return false;
}

/*
================
idIK::ClearJointMods
================
*/
void idIK::ClearJointMods( void ) {
	ik_activate = false;
}

/*
================
idIK::SolveTwoBones
================
*/
bool idIK::SolveTwoBones( const idVec3 &startPos, const idVec3 &endPos, const idVec3 &dir, float len0, float len1, idVec3 &jointPos ) {
	float length, lengthSqr, lengthInv, x, y;
	idVec3 vec0, vec1;

	vec0 = endPos - startPos;
	lengthSqr = vec0.LengthSqr();
	lengthInv = idMath::InvSqrt( lengthSqr );
	length = lengthInv * lengthSqr;

	// if the start and end position are too far out or too close to each other
	if ( length > len0 + len1 || length < idMath::Fabs( len0 - len1 ) ) {
		jointPos = startPos + 0.5f * vec0;
		return false;
	}

	vec0 *= lengthInv;
	vec1 = dir - vec0 * dir * vec0;
	vec1.Normalize();

	x = ( length * length + len0 * len0 - len1 * len1 ) * ( 0.5f * lengthInv );
	y = idMath::Sqrt( len0 * len0 - x * x );

	jointPos = startPos + ( x * vec0 ) + ( y * vec1 );

	return true;
}

/*
================
idIK::GetBoneAxis
================
*/
float idIK::GetBoneAxis( const idVec3 &startPos, const idVec3 &endPos, const idVec3 &dir, idMat3 &axis ) {
	float length;
	
	axis[ 0 ] = endPos - startPos;
	length = axis[ 0 ].Normalize();

	axis[ 1 ] = dir - ( axis[ 0 ] * ( dir * axis[ 0 ] ) );
	axis[ 1 ].Normalize();

	axis[ 2 ].Cross( axis[ 1 ], axis[ 0 ] );

	return length;
}

/*
===============================================================================

  idIK_Walk

===============================================================================
*/

CLASS_DECLARATION( idIK, idIK_Walk )
END_CLASS

/*
================
idIK_Walk::idIK_Walk
================
*/
idIK_Walk::idIK_Walk( void ) {
	int i;

	initialized = false;
	footModel = NULL;
	numLegs = 0;
	enabledLegs = 0;
	for ( i = 0; i < MAX_LEGS; i++ ) {
		footJoints[i] = INVALID_JOINT;
		ankleJoints[i] = INVALID_JOINT;
		kneeJoints[i] = INVALID_JOINT;
		hipJoints[i] = INVALID_JOINT;
		dirJoints[i] = INVALID_JOINT;
		hipForward[i].Zero();
		kneeForward[i].Zero();
		upperLegLength[i] = 0.0f;
		lowerLegLength[i] = 0.0f;
		upperLegToHipJoint[i].Identity();
		lowerLegToKneeJoint[i].Identity();
		oldAnkleHeights[i] = 0.0f;
	}
	waistJoint = INVALID_JOINT;

	smoothing = 0.75f;
	waistSmoothing = 0.5f;
	footShift = 0.0f;
	waistShift = 0.0f;
	minWaistFloorDist = 0.0f;
	minWaistAnkleDist = 0.0f;
	footUpTrace = 32.0f;
	footDownTrace = 32.0f;
	tiltWaist = false;
	usePivot = false;

	pivotFoot = -1;
	pivotYaw = 0.0f;
	pivotPos.Zero();

	oldHeightsValid = false;
	oldWaistHeight = 0.0f;
	waistOffset.Zero();
}

/*
================
idIK_Walk::~idIK_Walk
================
*/
idIK_Walk::~idIK_Walk( void ) {
	gameLocal.clip.DeleteClipModel( footModel );
}


/*
================
idIK_Walk::Init
================
*/
bool idIK_Walk::Init( idEntity *self, const char *anim, const idVec3 &modelOffset ) {
	int i;
	float footSize;
	idVec3 verts[4];
	idTraceModel trm;
	const char *jointName;
	idVec3 dir, ankleOrigin, kneeOrigin, hipOrigin, dirOrigin;
	idMat3 axis, ankleAxis, kneeAxis, hipAxis;

	static idVec3 footWinding[4] = {
		idVec3(  1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f, -1.0f, 0.0f ),
		idVec3(  1.0f, -1.0f, 0.0f )
	};

	if ( !self ) {
		return false;
	}

	numLegs = Min( self->spawnArgs.GetInt( "ik_numLegs", "0" ), MAX_LEGS );
	if ( numLegs == 0 ) {
		return true;
	}

	if ( !idIK::Init( self, anim, modelOffset ) ) {
		return false;
	}

	int numJoints = animator->NumJoints();
	idJointMat *joints = ( idJointMat * )_alloca16( numJoints * sizeof( joints[0] ) );

	// create the animation frame used to setup the IK
	gameEdit->ANIM_CreateAnimFrame( animator->ModelHandle(), animator->GetAnim( modifiedAnim )->MD5Anim( 0 ), numJoints, joints, 1, animator->ModelDef()->GetVisualOffset() + modelOffset, animator->RemoveOrigin() );

	enabledLegs = 0;

	// get all the joints
	for ( i = 0; i < numLegs; i++ ) {

		jointName = self->spawnArgs.GetString( va( "ik_foot%d", i+1 ) );
		footJoints[i] = animator->GetJointHandle( jointName );
		if ( footJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid foot joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_ankle%d", i+1 ) );
		ankleJoints[i] = animator->GetJointHandle( jointName );
		if ( ankleJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid ankle joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_knee%d", i+1 ) );
		kneeJoints[i] = animator->GetJointHandle( jointName );
		if ( kneeJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid knee joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_hip%d", i+1 ) );
		hipJoints[i] = animator->GetJointHandle( jointName );
		if ( hipJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid hip joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_dir%d", i+1 ) );
		dirJoints[i] = animator->GetJointHandle( jointName );

		enabledLegs |= 1 << i;
	}

	jointName = self->spawnArgs.GetString( "ik_waist" );
	waistJoint = animator->GetJointHandle( jointName );
	if ( waistJoint == INVALID_JOINT ) {
		gameLocal.Error( "idIK_Walk::Init: invalid waist joint '%s'", jointName );
	}

	// get the leg bone lengths and rotation matrices
	for ( i = 0; i < numLegs; i++ ) {
		oldAnkleHeights[i] = 0.0f;

		ankleAxis = joints[ ankleJoints[ i ] ].ToMat3();
		ankleOrigin = joints[ ankleJoints[ i ] ].ToVec3();

		kneeAxis = joints[ kneeJoints[ i ] ].ToMat3();
		kneeOrigin = joints[ kneeJoints[ i ] ].ToVec3();

		hipAxis = joints[ hipJoints[ i ] ].ToMat3();
		hipOrigin = joints[ hipJoints[ i ] ].ToVec3();

		// get the IK direction
		if ( dirJoints[i] != INVALID_JOINT ) {
			dirOrigin = joints[ dirJoints[ i ] ].ToVec3();
			dir = dirOrigin - kneeOrigin;
		} else {
			dir.Set( 1.0f, 0.0f, 0.0f );
		}

		hipForward[i] = dir * hipAxis.Transpose();
		kneeForward[i] = dir * kneeAxis.Transpose();

		// conversion from upper leg bone axis to hip joint axis
		upperLegLength[i] = GetBoneAxis( hipOrigin, kneeOrigin, dir, axis );
		upperLegToHipJoint[i] = hipAxis * axis.Transpose();

		// conversion from lower leg bone axis to knee joint axis
		lowerLegLength[i] = GetBoneAxis( kneeOrigin, ankleOrigin, dir, axis );
		lowerLegToKneeJoint[i] = kneeAxis * axis.Transpose();
	}

	smoothing = self->spawnArgs.GetFloat( "ik_smoothing", "0.75" );
	waistSmoothing = self->spawnArgs.GetFloat( "ik_waistSmoothing", "0.75" );
	footShift = self->spawnArgs.GetFloat( "ik_footShift", "0" );
	waistShift = self->spawnArgs.GetFloat( "ik_waistShift", "0" );
	minWaistFloorDist = self->spawnArgs.GetFloat( "ik_minWaistFloorDist", "0" );
	minWaistAnkleDist = self->spawnArgs.GetFloat( "ik_minWaistAnkleDist", "0" );
	footUpTrace = self->spawnArgs.GetFloat( "ik_footUpTrace", "32" );
	footDownTrace = self->spawnArgs.GetFloat( "ik_footDownTrace", "32" );
	tiltWaist = self->spawnArgs.GetBool( "ik_tiltWaist", "0" );
	usePivot = self->spawnArgs.GetBool( "ik_usePivot", "0" );

	// setup a clip model for the feet
	footSize = self->spawnArgs.GetFloat( "ik_footSize", "4" ) * 0.5f;
	if ( footSize > 0.0f ) {
		for ( i = 0; i < 4; i++ ) {
			verts[i] = footWinding[i] * footSize;
		}
		trm.SetupPolygon( verts, 4 );
		footModel = new idClipModel( trm, false );
	}

	initialized = true;

	return true;
}

/*
================
idIK_Walk::Evaluate
================
*/
bool idIK_Walk::Evaluate( void ) {
	int i, newPivotFoot;
	float modelHeight, jointHeight, lowestHeight, floorHeights[MAX_LEGS];
	float shift, smallestShift, newHeight, step, newPivotYaw, height, largestAnkleHeight;
	idVec3 modelOrigin, normal, hipDir, kneeDir, start, end, jointOrigins[MAX_LEGS];
	idVec3 footOrigin, ankleOrigin, kneeOrigin, hipOrigin, waistOrigin;
	idMat3 modelAxis, waistAxis, axis;
	idMat3 hipAxis[MAX_LEGS], kneeAxis[MAX_LEGS], ankleAxis[MAX_LEGS];
	trace_t results;

	if ( !self || !gameLocal.isNewFrame ) {
		return false;
	}

	// if no IK enabled on any legs
	if ( !enabledLegs ) {
		return false;
	}

	normal = - self->GetPhysics()->GetGravityNormal();
	modelOrigin = self->GetPhysics()->GetOrigin();
	modelAxis = self->GetRenderEntity()->axis;
	modelHeight = modelOrigin * normal;

	modelOrigin += modelOffset * modelAxis;

	// create frame without joint mods
	animator->CreateFrame( gameLocal.time, false );

	// get the joint positions for the feet
	lowestHeight = idMath::INFINITY;
	for ( i = 0; i < numLegs; i++ ) {
		animator->GetJointTransform( footJoints[i], gameLocal.time, footOrigin, axis );
		jointOrigins[i] = modelOrigin + footOrigin * modelAxis;
		jointHeight = jointOrigins[i] * normal;
		if ( jointHeight < lowestHeight ) {
			lowestHeight = jointHeight;
			newPivotFoot = i;
		}
	}

	if ( usePivot ) {

		newPivotYaw = modelAxis[0].ToYaw();

		// change pivot foot
		if ( newPivotFoot != pivotFoot || idMath::Fabs( idMath::AngleNormalize180( newPivotYaw - pivotYaw ) ) > 30.0f ) {
			pivotFoot = newPivotFoot;
			pivotYaw = newPivotYaw;
			animator->GetJointTransform( footJoints[pivotFoot], gameLocal.time, footOrigin, axis );
			pivotPos = modelOrigin + footOrigin * modelAxis;
		}

		// keep pivot foot in place
		jointOrigins[pivotFoot] = pivotPos;
	}

	// get the floor heights for the feet
	for ( i = 0; i < numLegs; i++ ) {

		if ( !( enabledLegs & ( 1 << i ) ) ) {
			continue;
		}

		start = jointOrigins[i] + normal * footUpTrace;
		end = jointOrigins[i] - normal * footDownTrace;
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS results, start, end, footModel, mat3_identity, CONTENTS_SOLID|CONTENTS_IKCLIP, self );
		floorHeights[i] = results.endpos * normal;

		if ( ik_debug.GetBool() && footModel ) {
			idFixedWinding w;
			for ( int j = 0; j < footModel->GetTraceModel()->numVerts; j++ ) {
				w += footModel->GetTraceModel()->verts[j];
			}
			gameRenderWorld->DebugWinding( colorRed, w, results.endpos, results.endAxis );
		}
	}

	const idPhysics *phys = self->GetPhysics();

	// test whether or not the character standing on the ground
	bool onGround = phys->HasGroundContacts();

	// test whether or not the character is standing on a plat
	bool onPlat = false;
	for ( i = 0; i < phys->GetNumContacts(); i++ ) {
		idEntity *ent = gameLocal.entities[ phys->GetContact( i ).entityNum ];
		if ( ent != NULL && ent->IsType( idPlat::Type ) ) {
			onPlat = true;
			break;
		}
	}

	// adjust heights of the ankles
	smallestShift = idMath::INFINITY;
	largestAnkleHeight = -idMath::INFINITY;
	for ( i = 0; i < numLegs; i++ ) {

		if ( onGround && ( enabledLegs & ( 1 << i ) ) ) {
			shift = floorHeights[i] - modelHeight + footShift;
		} else {
			shift = 0.0f;
		}

		if ( shift < smallestShift ) {
			smallestShift = shift;
		}

		animator->GetJointTransform( ankleJoints[i], gameLocal.time, ankleOrigin, ankleAxis[i] );
		jointOrigins[i] = modelOrigin + ankleOrigin * modelAxis;

		height = jointOrigins[i] * normal;

		if ( oldHeightsValid && !onPlat ) {
			step = height + shift - oldAnkleHeights[i];
			shift -= smoothing * step;
		}

		newHeight = height + shift;
		if ( newHeight > largestAnkleHeight ) {
			largestAnkleHeight = newHeight;
		}

		oldAnkleHeights[i] = newHeight;

		jointOrigins[i] += shift * normal;
	}

	animator->GetJointTransform( waistJoint, gameLocal.time, waistOrigin, waistAxis );
	waistOrigin = modelOrigin + waistOrigin * modelAxis;

	// adjust position of the waist
	waistOffset = ( smallestShift + waistShift ) * normal;

	// if the waist should be at least a certain distance above the floor
	if ( minWaistFloorDist > 0.0f && waistOffset * normal < 0.0f ) {
		start = waistOrigin;
		end = waistOrigin + waistOffset - normal * minWaistFloorDist;
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS results, start, end, footModel, modelAxis, CONTENTS_SOLID|CONTENTS_IKCLIP, self );
		height = ( waistOrigin + waistOffset - results.endpos ) * normal;
		if ( height < minWaistFloorDist ) {
			waistOffset += ( minWaistFloorDist - height ) * normal;
		}
	}

	// if the waist should be at least a certain distance above the ankles
	if ( minWaistAnkleDist > 0.0f ) {
		height = ( waistOrigin + waistOffset ) * normal;
		if ( height - largestAnkleHeight < minWaistAnkleDist ) {
			waistOffset += ( minWaistAnkleDist - ( height - largestAnkleHeight ) ) * normal;
		}
	}

	if ( oldHeightsValid ) {
		// smoothly adjust height of waist
		newHeight = ( waistOrigin + waistOffset ) * normal;
		step = newHeight - oldWaistHeight;
		waistOffset -= waistSmoothing * step * normal;
	}

	// save height of waist for smoothing
	oldWaistHeight = ( waistOrigin + waistOffset ) * normal;

	if ( !oldHeightsValid ) {
		oldHeightsValid = true;
		return false;
	}

	// solve IK
	for ( i = 0; i < numLegs; i++ ) {

		// get the position of the hip in world space
		animator->GetJointTransform( hipJoints[i], gameLocal.time, hipOrigin, axis );
		hipOrigin = modelOrigin + waistOffset + hipOrigin * modelAxis;
		hipDir = hipForward[i] * axis * modelAxis;

		// get the IK bend direction
		animator->GetJointTransform( kneeJoints[i], gameLocal.time, kneeOrigin, axis );
		kneeDir = kneeForward[i] * axis * modelAxis;

		// solve IK and calculate knee position
		SolveTwoBones( hipOrigin, jointOrigins[i], kneeDir, upperLegLength[i], lowerLegLength[i], kneeOrigin );

		if ( ik_debug.GetBool() ) {
			gameRenderWorld->DebugLine( colorCyan, hipOrigin, kneeOrigin );
			gameRenderWorld->DebugLine( colorRed, kneeOrigin, jointOrigins[i] );
			gameRenderWorld->DebugLine( colorYellow, kneeOrigin, kneeOrigin + hipDir );
			gameRenderWorld->DebugLine( colorGreen, kneeOrigin, kneeOrigin + kneeDir );
		}

		// get the axis for the hip joint
		GetBoneAxis( hipOrigin, kneeOrigin, hipDir, axis );
		hipAxis[i] = upperLegToHipJoint[i] * ( axis * modelAxis.Transpose() );

		// get the axis for the knee joint
		GetBoneAxis( kneeOrigin, jointOrigins[i], kneeDir, axis );
		kneeAxis[i] = lowerLegToKneeJoint[i] * ( axis * modelAxis.Transpose() );
	}

	// set the joint mods
	animator->SetJointAxis( waistJoint, JOINTMOD_WORLD_OVERRIDE, waistAxis );
	animator->SetJointPos( waistJoint, JOINTMOD_WORLD_OVERRIDE, ( waistOrigin + waistOffset - modelOrigin ) * modelAxis.Transpose() );
	for ( i = 0; i < numLegs; i++ ) {
		animator->SetJointAxis( hipJoints[i], JOINTMOD_WORLD_OVERRIDE, hipAxis[i] );
		animator->SetJointAxis( kneeJoints[i], JOINTMOD_WORLD_OVERRIDE, kneeAxis[i] );
		animator->SetJointAxis( ankleJoints[i], JOINTMOD_WORLD_OVERRIDE, ankleAxis[i] );
	}

	ik_activate = true;

	return true;
}

/*
================
idIK_Walk::ClearJointMods
================
*/
void idIK_Walk::ClearJointMods( void ) {
	int i;

	if ( !self || !ik_activate ) {
		return;
	}

	animator->SetJointAxis( waistJoint, JOINTMOD_NONE, mat3_identity );
	animator->SetJointPos( waistJoint, JOINTMOD_NONE, vec3_origin );
	for ( i = 0; i < numLegs; i++ ) {
		animator->SetJointAxis( hipJoints[i], JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( kneeJoints[i], JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( ankleJoints[i], JOINTMOD_NONE, mat3_identity );
	}

	ik_activate = false;
}

/*
================
idIK_Walk::EnableAll
================
*/
void idIK_Walk::EnableAll( void ) {
	enabledLegs = ( 1 << numLegs ) - 1;
	oldHeightsValid = false;
}

/*
================
idIK_Walk::DisableAll
================
*/
void idIK_Walk::DisableAll( void ) {
	enabledLegs = 0;
	oldHeightsValid = false;
}

/*
================
idIK_Walk::EnableLeg
================
*/
void idIK_Walk::EnableLeg( int num ) {
	enabledLegs |= 1 << num;
}

/*
================
idIK_Walk::DisableLeg
================
*/
void idIK_Walk::DisableLeg( int num ) {
	enabledLegs &= ~( 1 << num );
}


/*
===============================================================================

  idIK_Reach

===============================================================================
*/

CLASS_DECLARATION( idIK, idIK_Reach )
END_CLASS

/*
================
idIK_Reach::idIK_Reach
================
*/
idIK_Reach::idIK_Reach() {
	int i;

	initialized = false;
	numArms = 0;
	enabledArms = 0;
	for ( i = 0; i < MAX_ARMS; i++ ) {
		handJoints[i] = INVALID_JOINT;
		elbowJoints[i] = INVALID_JOINT;
		shoulderJoints[i] = INVALID_JOINT;
		dirJoints[i] = INVALID_JOINT;
		shoulderForward[i].Zero();
		elbowForward[i].Zero();
		upperArmLength[i] = 0.0f;
		lowerArmLength[i] = 0.0f;
		upperArmToShoulderJoint[i].Identity();
		lowerArmToElbowJoint[i].Identity();
	}
}

/*
================
idIK_Reach::~idIK_Reach
================
*/
idIK_Reach::~idIK_Reach() {
}

/*
================
idIK_Reach::Init
================
*/
bool idIK_Reach::Init( idEntity *self, const char *anim, const idVec3 &modelOffset ) {
	int i;
	const char *jointName;
	idTraceModel trm;
	idVec3 dir, handOrigin, elbowOrigin, shoulderOrigin, dirOrigin;
	idMat3 axis, handAxis, elbowAxis, shoulderAxis;

	if ( !self ) {
		return false;
	}

	numArms = Min( self->spawnArgs.GetInt( "ik_numArms", "0" ), MAX_ARMS );
	if ( numArms == 0 ) {
		return true;
	}

	if ( !idIK::Init( self, anim, modelOffset ) ) {
		return false;
	}

	int numJoints = animator->NumJoints();
	idJointMat *joints = ( idJointMat * )_alloca16( numJoints * sizeof( joints[0] ) );

	// create the animation frame used to setup the IK
	gameEdit->ANIM_CreateAnimFrame( animator->ModelHandle(), animator->GetAnim( modifiedAnim )->MD5Anim( 0 ), numJoints, joints, 1, animator->ModelDef()->GetVisualOffset() + modelOffset, animator->RemoveOrigin() );

	enabledArms = 0;

	// get all the joints
	for ( i = 0; i < numArms; i++ ) {

		jointName = self->spawnArgs.GetString( va( "ik_hand%d", i+1 ) );
		handJoints[i] = animator->GetJointHandle( jointName );
		if ( handJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Reach::Init: invalid hand joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_elbow%d", i+1 ) );
		elbowJoints[i] = animator->GetJointHandle( jointName );
		if ( elbowJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Reach::Init: invalid elbow joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_shoulder%d", i+1 ) );
		shoulderJoints[i] = animator->GetJointHandle( jointName );
		if ( shoulderJoints[i] == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Reach::Init: invalid shoulder joint '%s'", jointName );
		}

		jointName = self->spawnArgs.GetString( va( "ik_elbowDir%d", i+1 ) );
		dirJoints[i] = animator->GetJointHandle( jointName );

		enabledArms |= 1 << i;
	}

	// get the arm bone lengths and rotation matrices
	for ( i = 0; i < numArms; i++ ) {

		handAxis = joints[ handJoints[ i ] ].ToMat3();
		handOrigin = joints[ handJoints[ i ] ].ToVec3();

		elbowAxis = joints[ elbowJoints[ i ] ].ToMat3();
		elbowOrigin = joints[ elbowJoints[ i ] ].ToVec3();

		shoulderAxis = joints[ shoulderJoints[ i ] ].ToMat3();
		shoulderOrigin = joints[ shoulderJoints[ i ] ].ToVec3();

		// get the IK direction
		if ( dirJoints[i] != INVALID_JOINT ) {
			dirOrigin = joints[ dirJoints[ i ] ].ToVec3();
			dir = dirOrigin - elbowOrigin;
		} else {
			dir.Set( -1.0f, 0.0f, 0.0f );
		}

		shoulderForward[i] = dir * shoulderAxis.Transpose();
		elbowForward[i] = dir * elbowAxis.Transpose();

		// conversion from upper arm bone axis to should joint axis
		upperArmLength[i] = GetBoneAxis( shoulderOrigin, elbowOrigin, dir, axis );
		upperArmToShoulderJoint[i] = shoulderAxis * axis.Transpose();

		// conversion from lower arm bone axis to elbow joint axis
		lowerArmLength[i] = GetBoneAxis( elbowOrigin, handOrigin, dir, axis );
		lowerArmToElbowJoint[i] = elbowAxis * axis.Transpose();
	}

	initialized = true;

	return true;
}

/*
================
idIK_Reach::Evaluate
================
*/
bool idIK_Reach::Evaluate( void ) {
	return false;
	int i;
	idVec3 modelOrigin, shoulderOrigin, elbowOrigin, handOrigin, shoulderDir, elbowDir;
	idMat3 modelAxis, axis;
	idMat3 shoulderAxis[MAX_ARMS], elbowAxis[MAX_ARMS];
	trace_t trace;

	modelOrigin = self->GetRenderEntity()->origin;
	modelAxis = self->GetRenderEntity()->axis;

	// solve IK
	for ( i = 0; i < numArms; i++ ) {

		// get the position of the shoulder in world space
		animator->GetJointTransform( shoulderJoints[i], gameLocal.time, shoulderOrigin, axis );
		shoulderOrigin = modelOrigin + shoulderOrigin * modelAxis;
		shoulderDir = shoulderForward[i] * axis * modelAxis;

		// get the position of the hand in world space
		animator->GetJointTransform( handJoints[i], gameLocal.time, handOrigin, axis );
		handOrigin = modelOrigin + handOrigin * modelAxis;

		// get first collision going from shoulder to hand
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, shoulderOrigin, handOrigin, CONTENTS_SOLID, self );
		handOrigin = trace.endpos;

		// get the IK bend direction
		animator->GetJointTransform( elbowJoints[i], gameLocal.time, elbowOrigin, axis );
		elbowDir = elbowForward[i] * axis * modelAxis;

		// solve IK and calculate elbow position
		SolveTwoBones( shoulderOrigin, handOrigin, elbowDir, upperArmLength[i], lowerArmLength[i], elbowOrigin );

		if ( ik_debug.GetBool() ) {
			gameRenderWorld->DebugLine( colorCyan, shoulderOrigin, elbowOrigin );
			gameRenderWorld->DebugLine( colorRed, elbowOrigin, handOrigin );
			gameRenderWorld->DebugLine( colorYellow, elbowOrigin, elbowOrigin + elbowDir );
			gameRenderWorld->DebugLine( colorGreen, elbowOrigin, elbowOrigin + shoulderDir );
		}

		// get the axis for the shoulder joint
		GetBoneAxis( shoulderOrigin, elbowOrigin, shoulderDir, axis );
		shoulderAxis[i] = upperArmToShoulderJoint[i] * ( axis * modelAxis.Transpose() );

		// get the axis for the elbow joint
		GetBoneAxis( elbowOrigin, handOrigin, elbowDir, axis );
		elbowAxis[i] = lowerArmToElbowJoint[i] * ( axis * modelAxis.Transpose() );
	}

	for ( i = 0; i < numArms; i++ ) {
		animator->SetJointAxis( shoulderJoints[i], JOINTMOD_WORLD_OVERRIDE, shoulderAxis[i] );
		animator->SetJointAxis( elbowJoints[i], JOINTMOD_WORLD_OVERRIDE, elbowAxis[i] );
	}

	ik_activate = true;
}

/*
================
idIK_Reach::ClearJointMods
================
*/
void idIK_Reach::ClearJointMods( void ) {
	int i;

	if ( !self || !ik_activate ) {
		return;
	}

	for ( i = 0; i < numArms; i++ ) {
		animator->SetJointAxis( shoulderJoints[i], JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( elbowJoints[i], JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( handJoints[i], JOINTMOD_NONE, mat3_identity );
	}

	ik_activate = false;
}


/*
===============================================================================

	sdIK_Aim

===============================================================================
*/

CLASS_DECLARATION( idIK, sdIK_Aim )
END_CLASS

/*
================
sdIK_Aim::sdIK_Aim
================
*/
sdIK_Aim::sdIK_Aim( void ) {
}

/*
================
sdIK_Aim::~sdIK_Aim
================
*/
sdIK_Aim::~sdIK_Aim( void ) {
}

/*
================
sdIK_Aim::Init
================
*/
bool sdIK_Aim::Init( idEntity *self, const char *anim, const idVec3& modelOffset ) {
	if ( !idIK::Init( self, anim, modelOffset ) ) {
		return false;
	}

	int count = self->spawnArgs.GetInt( "ik_numsets", "0" );

	int i;
	for ( i = 0; i < count; i++ ) {
		const char* jointname1		= self->spawnArgs.GetString( va( "ik_set%i_joint1", i ) );
		const char* jointname2		= self->spawnArgs.GetString( va( "ik_set%i_joint2", i ) );

		jointGroup_t& group = jointGroups.Alloc();

		group.joint1 = self->GetAnimator()->GetJointHandle( jointname1 );
		group.joint2 = self->GetAnimator()->GetJointHandle( jointname2 );
	}

	initialized = true;

	return true;
}

extern idCVar r_debugAxisLength;

/*
================
sdIK_Aim::Evaluate
================
*/
bool sdIK_Aim::Evaluate( void ) {
	int i;
	for ( i = 0; i < jointGroups.Num(); i++ ) {
		jointGroup_t& group = jointGroups[ i ];

		idVec3 org1, org2;

		self->GetAnimator()->GetJointTransform( group.joint1, gameLocal.time, org1 );
		self->GetAnimator()->GetJointTransform( group.joint2, gameLocal.time, org2 );

		idVec3 dir = org2 - org1;
		dir.Normalize();

		float dot = dir * group.lastDir;

		if ( dot < ( 1.f - 1e-4f ) ) {
			break;
		}
	}

	if ( i == jointGroups.Num() ) {
		return false;
	}

	ClearJointMods();

	for ( i = 0; i < jointGroups.Num(); i++ ) {
		jointGroup_t& group = jointGroups[ i ];

		idVec3 org1, org2;
		idMat3 axis1, axis2;

		self->GetAnimator()->GetJointTransform( group.joint1, gameLocal.time, org1, axis1 );
		self->GetAnimator()->GetJointTransform( group.joint2, gameLocal.time, org2, axis2 );

		idMat3 axes1, axes2;

		group.lastDir = org2 - org1;
		group.lastDir.Normalize();
		
		
		axes1[ 0 ] = org2 - org1;
		axes1[ 0 ] *= axis1.Transpose();
		axes1[ 0 ].Normalize();

		axes1[ 2 ].Set( 0.f, 0.f, 1.f );
		axes1[ 2 ] -= ( axes1[ 0 ] * axes1[ 2 ] ) * axes1[ 0 ];
		axes1[ 2 ].Normalize();

		axes1[ 1 ] = axes1[ 2 ].Cross( axes1[ 0 ] );



		axes2[ 0 ] = org1 - org2;
		axes2[ 0 ] *= axis2.Transpose();
		axes2[ 0 ].Normalize();

		axes2[ 2 ].Set( 0.f, 0.f, 1.f );
		axes2[ 2 ] -= ( axes2[ 0 ] * axes2[ 2 ] ) * axes2[ 0 ];
		axes2[ 2 ].Normalize();

		axes2[ 1 ] = axes2[ 2 ].Cross( axes2[ 0 ] );		

		self->GetAnimator()->SetJointAxis( group.joint1, JOINTMOD_LOCAL, axes1 );
		self->GetAnimator()->SetJointAxis( group.joint2, JOINTMOD_LOCAL, axes2 );
	}

	return true;
}

/*
================
sdIK_Aim::ClearJointMods
================
*/
void sdIK_Aim::ClearJointMods( void ) {
	if ( !self ) {
		return;
	}

	int i;
	for ( i = 0; i < jointGroups.Num(); i++ ) {
		jointGroup_t& group = jointGroups[ i ];

		self->GetAnimator()->SetJointAxis( group.joint1, JOINTMOD_NONE, mat3_identity );
		self->GetAnimator()->SetJointAxis( group.joint2, JOINTMOD_NONE, mat3_identity );
	}
}
