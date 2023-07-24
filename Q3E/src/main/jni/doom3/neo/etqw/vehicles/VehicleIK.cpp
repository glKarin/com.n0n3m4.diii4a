// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "VehicleIK.h"
#include "TransportComponents.h"
#include "../Entity.h"
#include "../anim/Anim.h"
#include "../physics/Physics_RigidBodyMultiple.h"
#include "Transport.h"
#include "../Player.h"
#include "../ContentMask.h"
#include "VehicleSuspension.h"
#include "VehicleWeapon.h"

/*
===============================================================================

	sdIK_WheeledVehicle

===============================================================================
*/

CLASS_DECLARATION( idIK, sdIK_WheeledVehicle )
END_CLASS

/*
================
sdIK_WheeledVehicle::sdIK_WheeledVehicle
================
*/
sdIK_WheeledVehicle::sdIK_WheeledVehicle( void ) {
}

/*
================
sdIK_WheeledVehicle::~sdIK_WheeledVehicle
================
*/
sdIK_WheeledVehicle::~sdIK_WheeledVehicle( void ) {
}

/*
================
sdIK_WheeledVehicle::AddWheel
================
*/
void sdIK_WheeledVehicle::AddWheel( sdVehicleRigidBodyWheel& _wheel ) {
	wheels.Alloc() = &_wheel;
}

/*
================
sdIK_WheeledVehicle::ClearWheels
================
*/
void sdIK_WheeledVehicle::ClearWheels( void ) {
	wheels.Clear();
}

/*
================
sdIK_WheeledVehicle::ClearJointMods
================
*/
void sdIK_WheeledVehicle::ClearJointMods( void ) {
	if ( !self ) {
		return;
	}

	vehicleDriveObjectList_t& list = rbParent->GetDriveObjects();
	for ( int i = 0; i < list.Num(); i++ ) {
		sdVehicleDriveObject* object = list[ i ];
		object->ClearSuspensionIK();
	}
}


/*
================
sdIK_WheeledVehicle::Evaluate
================
*/
bool sdIK_WheeledVehicle::Evaluate( void ) {
	idAnimator* animator = self->GetAnimator();

	bool changed = false;

	vehicleDriveObjectList_t& list = rbParent->GetDriveObjects();

	for ( int i = 0; i < list.Num(); i++ ) {
		sdVehicleDriveObject* object = list[ i ];
		changed |= object->UpdateSuspensionIK();
	}

	for ( int i = 0; i < wheels.Num(); i++ ) {
		sdVehicleRigidBodyWheel* wheel = wheels[ i ];

		if ( !wheel->HasVisualStateChanged() ) {
			continue;
		}
		wheel->ResetVisualState();

		const idMat3& frictionAxes = wheel->GetFrictionAxes();
		const idMat3& baseAxes = wheel->GetBaseAxes();

		idRotation rotation;
		rotation.SetVec( wheel->GetRotationAxis() );
		rotation.SetAngle( wheel->GetWheelAngle() );

		animator->SetJointAxis( wheel->GetWheelJoint(), JOINTMOD_WORLD_OVERRIDE, baseAxes * rotation.ToMat3() * frictionAxes );

		changed = true;
	}

	return changed;
}

/*
================
sdIK_WheeledVehicle::Init
================
*/
bool sdIK_WheeledVehicle::Init( sdTransport_RB* self, const char *anim, const idVec3 &modelOffset ) {
	rbParent = self;

	if( !idIK::Init( self, anim, modelOffset ) ) {
		return false;
	}

	initialized = true;

	return true;
}




/*
===============================================================================

	sdIK_Walker

===============================================================================
*/

CLASS_DECLARATION( idIK, sdIK_Walker )
END_CLASS

/*
================
sdIK_Walker::sdIK_Walker
================
*/
sdIK_Walker::sdIK_Walker( void ) {
}

/*
================
sdIK_Walker::~sdIK_Walker
================
*/
sdIK_Walker::~sdIK_Walker( void ) {
	gameLocal.clip.DeleteClipModel( footModel );
}


/*
================
sdIK_Walker::Init
================
*/
bool sdIK_Walker::Init( idEntity *self, const char *anim, const idVec3 &modelOffset ) {
	float footSize;
	idVec3 verts[ 4 ];
	idTraceModel trm;

	static idVec3 footWinding[ 4 ] = {
		idVec3(  1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f, -1.0f, 0.0f ),
		idVec3(  1.0f, -1.0f, 0.0f )
	};

	waistOffset.Zero();
	oldWaistHeight	= 0;
	oldHeightsValid = false;
	isStable = true;

	assert( self );

	const idDict& spawnArgs = self->spawnArgs;

	int numLegs = spawnArgs.GetInt( "ik_numLegs" );
	if ( numLegs == 0 ) {
		return true;
	}

	if ( !idIK::Init( self, anim, modelOffset ) ) {
		return false;
	}
	
	float tweakFactor = spawnArgs.GetFloat( "ik_tweakFactor", "0.5" );
	compressionInterpolate.Init( MS2SEC( gameLocal.time ), 1.f, tweakFactor, tweakFactor );

	// get all the joints
	for ( int i = 0; i < numLegs; i++ ) {
		leg_t& leg = legs.Alloc();

		const char* jointName;

		jointName = spawnArgs.GetString( va( "ik_foot%d", i + 1 ) );
		leg.footJoint = animator->GetJointHandle( jointName );
		if ( leg.footJoint == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid foot joint '%s'", jointName );
		}

		jointName = spawnArgs.GetString( va( "ik_ankle%d", i + 1 ) );
		leg.ankleJoint = animator->GetJointHandle( jointName );
		if ( leg.ankleJoint == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid ankle joint '%s'", jointName );
		}

		jointName = spawnArgs.GetString( va( "ik_knee%d", i + 1 ) );
		leg.kneeJoint = animator->GetJointHandle( jointName );
		if ( leg.kneeJoint == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid knee joint '%s'", jointName );
		}

		jointName = spawnArgs.GetString( va( "ik_mid%d", i + 1 ) );
		leg.midJoint = animator->GetJointHandle( jointName );
		if ( leg.midJoint == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid mid joint '%s'", jointName );
		}

		jointName = spawnArgs.GetString( va( "ik_hip%d", i + 1 ) );
		leg.hipJoint = animator->GetJointHandle( jointName );
		if ( leg.hipJoint == INVALID_JOINT ) {
			gameLocal.Error( "idIK_Walk::Init: invalid hip joint '%s'", jointName );
		}
	}

	const char* jointName = spawnArgs.GetString( "ik_waist" );
	waistJoint = animator->GetJointHandle( jointName );
	if ( waistJoint == INVALID_JOINT ) {
		gameLocal.Error( "idIK_Walk::Init: invalid waist joint '%s'", jointName );
	}

	// get the leg bone lengths and rotation matrices
	for ( int i = 0; i < legs.Num(); i++ ) {
		leg_t& leg = legs[ i ];

		idVec3 dir, ankleOrigin, kneeOrigin, midOrigin, hipOrigin, footOrigin, dirOrigin;
		idVec3 dir2;
		idMat3 axis, ankleAxis, kneeAxis, midAxis, hipAxis, footAxis;

		leg.oldAnkleHeight = 0.0f;

		animator->GetJointTransform( leg.ankleJoint, gameLocal.time, ankleOrigin, ankleAxis );
		animator->GetJointTransform( leg.kneeJoint, gameLocal.time, kneeOrigin, kneeAxis );
		animator->GetJointTransform( leg.midJoint, gameLocal.time, midOrigin, midAxis );		
		animator->GetJointTransform( leg.hipJoint, gameLocal.time, hipOrigin, hipAxis );
		animator->GetJointTransform( leg.footJoint, gameLocal.time, footOrigin, footAxis );

		// get the IK direction
		dir = spawnArgs.GetVector( "ik_direction", "1 0 0" );
		dir.Normalize();

		dir2 = spawnArgs.GetVector( "ik_direction2", "0 1 0" );
		dir2.Normalize();

		leg.hipForward = dir * hipAxis.Transpose();
		leg.midForward = dir * midAxis.Transpose();
		leg.kneeForward = dir * kneeAxis.Transpose();

		leg.midSide = dir2 * midAxis.Transpose();

		// conversion from upper leg bone axis to hip joint axis
		leg.upperLegLength		= GetBoneAxis( hipOrigin, midOrigin, dir, axis );
		leg.upperLegToHipJoint	= hipAxis * axis.Transpose();

		// conversion from upper leg bone axis to hip joint axis
		leg.midLegLength		= GetBoneAxis( midOrigin, kneeOrigin, dir2, axis );
		leg.midToUpperLegJoint	= midAxis * axis.Transpose();

		// conversion from lower leg bone axis to knee joint axis
		leg.lowerLegLength		= GetBoneAxis( kneeOrigin, ankleOrigin, dir, axis );
		leg.lowerLegToKneeJoint	= kneeAxis * axis.Transpose();

		leg.lastAnkleAxes.Identity();
	}

	smoothing			= spawnArgs.GetFloat( "ik_smoothing", "0.75" );
	downsmoothing		= spawnArgs.GetFloat( "ik_downsmoothing", "0.25" );
	waistSmoothing		= spawnArgs.GetFloat( "ik_waistSmoothing", "0.75" );
	footShift			= spawnArgs.GetFloat( "ik_footShift", "0" );
	waistShift			= spawnArgs.GetFloat( "ik_waistShift", "0" );
	minWaistFloorDist	= spawnArgs.GetFloat( "ik_minWaistFloorDist", "0" );
	minWaistAnkleDist	= spawnArgs.GetFloat( "ik_minWaistAnkleDist", "0" );
	footUpTrace			= spawnArgs.GetFloat( "ik_footUpTrace", "32" );
	footDownTrace		= spawnArgs.GetFloat( "ik_footDownTrace", "32" );
	tiltWaist			= spawnArgs.GetBool( "ik_tiltWaist", "0" );
	usePivot			= spawnArgs.GetBool( "ik_usePivot", "0" );
	invertedLegDirOffset= spawnArgs.GetFloat( "ik_invertlegoffset", "-2" ); // Gordon: Hack to tweak the inverted dir for the back section of the legs

	// setup a clip model for the feet
	footSize = spawnArgs.GetFloat( "ik_footSize", "4" ) * 0.5f;
	if ( footSize > 0.0f ) {
		for ( int i = 0; i < 4; i++ ) {
			verts[ i ] = footWinding[ i ] * footSize;
		}
		trm.SetupPolygon( verts, 4 );
		footModel = new idClipModel( trm, false );
	}

	initialized = true;

	return true;
}

/*
================
sdIK_Walker::SetCompressionScale
================
*/
void sdIK_Walker::SetCompressionScale( float scale, float length ) {
	float current = compressionInterpolate.GetCurrentValue( MS2SEC( gameLocal.time ) );
	compressionInterpolate.Init( MS2SEC( gameLocal.time ), length, current, scale );
}

/*
================
sdIK_Walker::FindSmallest
================
*/
float sdIK_Walker::FindSmallest( float min1, float max1, float min2, float max2, float& sizeMin1, float& sizeMax1, float& sizeMin2, float& sizeMax2 ) {
	if ( max1 < min2 ) {
		sizeMin1 = sizeMax1 = max1;
		sizeMin2 = sizeMax2 = min2;
		return min2 - max1;
	}

	if ( max2 < min1 ) {
		sizeMin2 = sizeMin2 = max2;
		sizeMin1 = sizeMax1 = min1;
		return min1 - max2;
	}

	if ( max2 > max1 ) {
		sizeMax1 = sizeMax2 = max1;
	} else {
		sizeMax1 = sizeMax2 = max2;
	}

	if ( min2 < min1 ) {
		sizeMin1 = sizeMin2 = min1;
	} else {
		sizeMin1 = sizeMin2 = min2;
	}

	return 0;
}

/*
================
sdIK_Walker::FindSmallest
================
*/
float sdIK_Walker::FindSmallest( float size1, float min2, float max2, float& sizeMin2, float& sizeMax2 ) {
	if ( size1 < min2 ) {
		sizeMin2 = sizeMax2 = min2;
		return min2 - size1;
	}

	if ( max2 < size1 ) {
		sizeMin2 = sizeMin2 = max2;
		return size1 - max2;
	}

	if ( max2 > size1 ) {
		sizeMax2 = size1;
	} else {
		sizeMax2 = max2;
	}

	if ( min2 < size1 ) {
		sizeMin2 = size1;
	} else {
		sizeMin2 = min2;
	}

	return 0;
}

/*
================
sdIK_Walker::Evaluate
================
*/
bool sdIK_Walker::Evaluate( void ) {
	trace_t results;

	// clear joint mods
	ClearJointMods();
	animator->CreateFrame( gameLocal.time, true );

	const renderEntity_t* renderEnt = self->GetRenderEntity();

	idVec3 normal					= -self->GetPhysics()->GetGravityNormal();
	const idVec3& modelOrigin		= renderEnt->origin;
	const idMat3& modelAxis			= renderEnt->axis;
	float modelHeight				= modelOrigin * normal;

	isStable = true;

	// get the joint positions for the feet
	for ( int i = 0; i < legs.Num(); i++ ) {
		leg_t& leg = legs[ i ];

		idVec3 footOrigin;
		animator->GetJointTransform( leg.footJoint, gameLocal.time, footOrigin );
		
		leg.current.jointWorldOrigin = modelOrigin + footOrigin * modelAxis;

		idVec3 start	= leg.current.jointWorldOrigin + ( normal * footUpTrace );
		idVec3 end		= leg.current.jointWorldOrigin - ( normal * footDownTrace );

//		gameLocal.clip.Translation( results, start, end, footModel, modelAxis, CONTENTS_SOLID, self );
		if ( !gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS results, start, end, MASK_VEHICLESOLID | CONTENTS_MONSTER, self ) ) {
			isStable = false;
		}

		leg.current.floorHeight = results.endpos * normal;

		idMat3 newAxes;

		if ( results.fraction != 1.f ) {
			idVec3 normal = results.c.normal * modelAxis.Transpose();

			idVec3 vec3_forward( 1.f, 0.f, 0.f );

			newAxes[ 0 ] = vec3_forward - ( normal * ( vec3_forward * normal ) );
			newAxes[ 0 ].Normalize();

			newAxes[ 2 ] = normal;

			newAxes[ 1 ] = newAxes[ 2 ].Cross( newAxes[ 0 ] );
		} else {
			newAxes.Identity();
		}

		idQuat newQuat;
		newQuat.Slerp( leg.lastAnkleAxes.ToQuat(), newAxes.ToQuat(), 0.1f );
		leg.lastAnkleAxes = newQuat.ToMat3();
		leg.lastAnkleAxes.FixDenormals();

		if ( ik_debug.GetBool() && footModel ) {
			idFixedWinding w;
			for ( int j = 0; j < footModel->GetTraceModel()->numVerts; j++ ) {
				w += footModel->GetTraceModel()->verts[j];
			}
			gameRenderWorld->DebugWinding( colorRed, w, results.endpos, results.endAxis );
		}
	}

	// adjust heights of the ankles
	float smallestShift = idMath::INFINITY;
	float largestAnkleHeight = -idMath::INFINITY;
	for ( int i = 0; i < legs.Num(); i++ ) {
		leg_t& leg = legs[ i ];

		idVec3 ankleOrigin;
		idVec3 footOrigin;

		animator->GetJointTransform( leg.ankleJoint, gameLocal.time, ankleOrigin, leg.current.ankleAxis );
		animator->GetJointTransform( leg.footJoint, gameLocal.time, footOrigin );

		float shift = leg.current.floorHeight - modelHeight + footShift;

		if ( shift < smallestShift ) {
			smallestShift = shift;
		}

		leg.current.jointWorldOrigin = modelOrigin + ankleOrigin * modelAxis;

		float height = leg.current.jointWorldOrigin * normal;

		if ( oldHeightsValid ) {
			float step = height + shift - leg.oldAnkleHeight;
			if ( step < 0 ) {
				shift -= smoothing * step;
			} else {
				shift -= downsmoothing * step;
			}
		}

		float newHeight = height + shift;
		if ( newHeight > largestAnkleHeight ) {
			largestAnkleHeight = newHeight;
		}

		leg.oldAnkleHeight = newHeight;

		leg.current.jointWorldOrigin += shift * normal;
	}

	idVec3 waistOrigin;
	animator->GetJointTransform( waistJoint, gameLocal.time, waistOrigin );
	waistOrigin = modelOrigin + waistOrigin * modelAxis;

	// adjust position of the waist
	waistOffset = ( smallestShift + waistShift ) * normal;

	// if the waist should be at least a certain distance above the floor
	if ( minWaistFloorDist > 0.0f && waistOffset * normal < 0.0f ) {
		idVec3 start	= waistOrigin;
		idVec3 end		= waistOrigin + waistOffset - normal * minWaistFloorDist;
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS results, start, end, footModel, modelAxis, CONTENTS_SOLID, self );
		float height = ( waistOrigin + waistOffset - results.endpos ) * normal;
		if ( height < minWaistFloorDist ) {
			waistOffset += ( minWaistFloorDist - height ) * normal;
		}
	}

	// if the waist should be at least a certain distance above the ankles
	if ( minWaistAnkleDist > 0.0f ) {
		float height = ( waistOrigin + waistOffset ) * normal;
		if ( height - largestAnkleHeight < minWaistAnkleDist ) {
			waistOffset += ( minWaistAnkleDist - ( height - largestAnkleHeight ) ) * normal;
		}
	}

	if ( oldHeightsValid ) {
		// smoothly adjust height of waist
		float newHeight = ( waistOrigin + waistOffset ) * normal;
		float step = newHeight - oldWaistHeight;
		waistOffset -= waistSmoothing * step * normal;
	}

	// save height of waist for smoothing
	oldWaistHeight = ( waistOrigin + waistOffset ) * normal;

	if ( !oldHeightsValid ) {
		oldHeightsValid = true;
		return false;
	}

	// solve IK
	for ( int i = 0; i < legs.Num(); i++ ) {
		leg_t& leg = legs[ i ];

		idVec3 hipOrigin;

		idMat3 axis;
		// get the position of the hip in world space
		animator->GetJointTransform( leg.hipJoint, gameLocal.time, hipOrigin, axis );

		hipOrigin = modelOrigin + waistOffset + hipOrigin * modelAxis;

//		DebugAxis( hipOrigin, axis * modelAxis );

		idVec3 hipDir = leg.hipForward * axis * modelAxis;

		idVec3 midOrigin;

		// get the IK bend direction
		animator->GetJointTransform( leg.midJoint, gameLocal.time, midOrigin, axis );

		midOrigin = modelOrigin + waistOffset + midOrigin * modelAxis;

//		DebugAxis( midOrigin, axis * modelAxis );

		idVec3 midDir		= leg.midForward * axis * modelAxis;
		idVec3 midSideDir	= leg.midSide * axis * modelAxis;

		idVec3 kneeOrigin;

		// get the IK bend direction
		animator->GetJointTransform( leg.kneeJoint, gameLocal.time, kneeOrigin, axis );

		kneeOrigin = modelOrigin + waistOffset + kneeOrigin * modelAxis;
		
//		DebugAxis( kneeOrigin, axis * modelAxis );

		idVec3 kneeDir = leg.kneeForward * axis * modelAxis;

		idVec3 ankleOrigin;
		
		animator->GetJointTransform( leg.ankleJoint, gameLocal.time, ankleOrigin, axis );

		ankleOrigin = modelOrigin + waistOffset + ankleOrigin * modelAxis;




		float len1 = leg.upperLegLength;
		float minLen2 = fabs( leg.midLegLength - leg.lowerLegLength );
		float maxLen2 = leg.midLegLength + leg.lowerLegLength;
		float wantLen = ( hipOrigin - leg.current.jointWorldOrigin ).Length();
		float constrainedMaxLen2, constrainedMinLen2;

		float lenTotal	= 0;
		if( minLen2 < fabs( wantLen - len1 ) ) {
			minLen2 = fabs( wantLen - len1 );
		}
		float minTotal	= FindSmallest( len1, minLen2, maxLen2, constrainedMaxLen2, constrainedMinLen2 );
		float maxTotal	= len1 + maxLen2;

		if ( maxTotal < wantLen ) {
			lenTotal = maxTotal;
		} else if ( minTotal > wantLen ) {
			lenTotal = constrainedMaxLen2;
		} else {
			float scale = compressionInterpolate.GetCurrentValue( MS2SEC( gameLocal.time ) );
			lenTotal = Lerp( minLen2, maxLen2, scale );
		}

		// solve IK and calculate upper knee position
		SolveTwoBones( hipOrigin, leg.current.jointWorldOrigin, midDir, leg.upperLegLength, lenTotal, midOrigin );

		float d1 = ( hipOrigin - midOrigin ).Length();

		idVec3 kneeDirTest = kneeDir;
		kneeDirTest.z += invertedLegDirOffset;
		kneeDirTest.NormalizeFast();

		// solve IK and calculate lower knee position, using -kneeDir, as lower leg is inverted
		SolveTwoBones( midOrigin, leg.current.jointWorldOrigin, -kneeDirTest, leg.midLegLength, leg.lowerLegLength, kneeOrigin );

		float d2 = ( midOrigin - kneeOrigin ).Length();

		if ( ik_debug.GetBool() ) {
			gameRenderWorld->DebugLine( colorCyan, hipOrigin, midOrigin );
			gameRenderWorld->DebugLine( colorRed, midOrigin, kneeOrigin );
			gameRenderWorld->DebugLine( colorBlue, kneeOrigin, leg.current.jointWorldOrigin );

			gameRenderWorld->DebugLine( colorYellow, midOrigin, midOrigin + ( midSideDir * 32 ) );

			gameRenderWorld->DebugLine( colorGreen, hipOrigin, hipOrigin + ( hipDir * 32 ) );
			gameRenderWorld->DebugLine( colorGreen, midOrigin, midOrigin + ( midDir * 32 ) );
			gameRenderWorld->DebugLine( colorGreen, kneeOrigin, kneeOrigin + ( -kneeDirTest * 32 ) );
		}

		// get the axis for the hip joint
		GetBoneAxis( hipOrigin, midOrigin, hipDir, axis );
		leg.current.hipAxis = leg.upperLegToHipJoint * ( axis * modelAxis.Transpose() );

		// get the axis for the knee joint
		GetBoneAxis( midOrigin, kneeOrigin, midSideDir, axis );
		leg.current.midAxis = leg.midToUpperLegJoint * ( axis * modelAxis.Transpose() );

		// get the axis for the knee joint
		GetBoneAxis( kneeOrigin, leg.current.jointWorldOrigin, kneeDir, axis );
		leg.current.kneeAxis = leg.lowerLegToKneeJoint * ( axis * modelAxis.Transpose() );
	}

	// set the joint mods
	animator->SetJointPos( waistJoint, JOINTMOD_WORLD_OVERRIDE, ( waistOrigin + waistOffset - modelOrigin ) * modelAxis.Transpose() );
	for ( int i = 0; i < legs.Num(); i++ ) {
		leg_t& leg = legs[ i ];

		animator->SetJointAxis( leg.hipJoint, JOINTMOD_WORLD_OVERRIDE, leg.current.hipAxis );
		animator->SetJointAxis( leg.midJoint, JOINTMOD_WORLD_OVERRIDE, leg.current.midAxis );
		animator->SetJointAxis( leg.kneeJoint, JOINTMOD_WORLD_OVERRIDE, leg.current.kneeAxis );
		animator->SetJointAxis( leg.ankleJoint, JOINTMOD_WORLD_OVERRIDE, leg.current.ankleAxis * leg.lastAnkleAxes );
	}

	return true;
}

/*
================
sdIK_Walker::ClearJointMods
================
*/
void sdIK_Walker::ClearJointMods( void ) {
	animator->SetJointPos( waistJoint, JOINTMOD_NONE, vec3_origin );
	for ( int i = 0; i < legs.Num(); i++ ) {
		leg_t& leg = legs[ i ];

		animator->SetJointAxis( leg.hipJoint, JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( leg.midJoint, JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( leg.kneeJoint, JOINTMOD_NONE, mat3_identity );
		animator->SetJointAxis( leg.ankleJoint, JOINTMOD_NONE, mat3_identity );
	}
}







/*
===============================================================================

	sdVehicleIKSystem

===============================================================================
*/

ABSTRACT_DECLARATION( idClass, sdVehicleIKSystem )
END_CLASS

/*
================
sdVehicleIKSystem::Setup
================
*/
bool sdVehicleIKSystem::Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms ) {
	vehicle = _vehicle;

	clampYaw = yaw;
	clampPitch = pitch;

	const char* weaponName = ikParms.GetString( "weapon" );
	if ( *weaponName ) {
		weapon = _vehicle->GetWeapon( weaponName );
		if ( !weapon ) {
			gameLocal.Warning( "sdVehicleIKSystem::Setup Invalid Weapon '%s'", weaponName );
			return false;
		}
	} else {
		weapon = NULL;
	}
	return true;
}

/*
================
sdVehicleIKSystem::GetPlayer
================
*/
idPlayer* sdVehicleIKSystem::GetPlayer( void ) {	
	return weapon ? weapon->GetPlayer() : position->GetPlayer();
}

/*
===============================================================================

	sdVehicleIKArms

===============================================================================
*/

CLASS_DECLARATION( sdVehicleIKSystem, sdVehicleIKArms )
END_CLASS

/*
================
sdVehicleIKArms::Setup
================
*/
bool sdVehicleIKArms::Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms ) {
	for ( int i = 0; i < ARM_JOINT_NUM_JOINTS; i++ ) {
		ikJoints[ i ] = INVALID_JOINT;
	}

	if ( !sdVehicleIKSystem::Setup( _vehicle, yaw, pitch, ikParms ) ) {
		return false;
	}

	yawSound = NULL;
	if ( clampYaw.sound != NULL ) {
		yawSound = vehicle->GetMotorSounds().Alloc();
		yawSound->Start( clampYaw.sound );
	}

	pitchSound = NULL;
	if ( clampPitch.sound != NULL ) {
		pitchSound = vehicle->GetMotorSounds().Alloc();
		pitchSound->Start( clampPitch.sound );
	}

	jointAngles.Zero();

	idAnimator* animator = vehicle->GetAnimator();
	const idDict& dict = ikParms;

	const char* joint;


	joint = dict.GetString( "jointWrist" );
	if ( *joint ) {
		ikJoints[ ARM_JOINT_INDEX_WRIST ] = animator->GetJointHandle( joint );
	}

	joint = dict.GetString( "jointMuzzle" );
	if ( *joint ) {
		ikJoints[ ARM_JOINT_INDEX_MUZZLE ] = animator->GetJointHandle( joint );
	}

	joint = dict.GetString( "jointElbow" );
	if ( *joint ) {
		ikJoints[ ARM_JOINT_INDEX_ELBOW ] = animator->GetJointHandle( joint );
	}

	ikJoints[ ARM_JOINT_INDEX_SHOULDER ] = animator->GetJointParent( ikJoints[ ARM_JOINT_INDEX_ELBOW ] );

	for ( int i = 0; i < ARM_JOINT_NUM_JOINTS; i++ ) {
		animator->GetJointTransform( ikJoints[ i ], gameLocal.time, baseJointPositions[ i ], baseJointAxes[ i ] );
	}

	pitchAxis = dict.GetInt( "pitchAxis", "2" );	
	requireTophat = dict.GetBool( "require_tophat" );

	oldParentAxis = mat3_identity;

	return true;
}

/*
================
sdVehicleIKArms::Update
================
*/
void sdVehicleIKArms::Update( void ) {
	idEntity* vehicleEnt = vehicle;
	if ( !vehicleEnt ) {
		return;
	}

	idVec3 shoulderPos;
	idMat3 temp;

	idAnimator* animator = vehicleEnt->GetAnimator();
	animator->GetJointTransform( ikJoints[ ARM_JOINT_INDEX_SHOULDER ], gameLocal.time, shoulderPos, temp );

	idVec3 shoulderToElbow	= baseJointPositions[ ARM_JOINT_INDEX_ELBOW ] - baseJointPositions[ ARM_JOINT_INDEX_SHOULDER ];
	idVec3 elbowToWrist		= baseJointPositions[ ARM_JOINT_INDEX_WRIST ] - baseJointPositions[ ARM_JOINT_INDEX_ELBOW ];
	idVec3 wristToMuzzle	= baseJointPositions[ ARM_JOINT_INDEX_MUZZLE ] - baseJointPositions[ ARM_JOINT_INDEX_WRIST ];
	idVec3 elbowToMuzzle	= baseJointPositions[ ARM_JOINT_INDEX_MUZZLE ] - baseJointPositions[ ARM_JOINT_INDEX_ELBOW ];

	idMat3 shoulderAxis;
	TransposeMultiply( baseJointAxes[ ARM_JOINT_INDEX_SHOULDER ], temp, shoulderAxis );
	idMat3 transposedShoulderAxis = shoulderAxis.Transpose();

	idPlayer* player = GetPlayer();
	if ( player && requireTophat ) {
		if ( !gameLocal.usercmds[ player->entityNumber ].buttons.btn.tophat ) {
			player = NULL;
		}
	}

	bool changed = false;

	changed |= !oldParentAxis.Compare( temp, 0.005f );

	idAngles newAngles;

	renderView_t* view = player ? player->GetRenderView() : NULL;
	renderEntity_t* renderEnt = vehicleEnt->GetRenderEntity();

	trace_t trace;
	idVec3 modelTarget;
	if( view ) {
		idVec3 end = view->vieworg + ( 8192 * view->viewaxis[ 0 ] );
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, view->vieworg, end, CONTENTS_SOLID | CONTENTS_OPAQUE, player );

		modelTarget = trace.endpos;

		modelTarget -= renderEnt->origin;
		modelTarget *= renderEnt->axis.Transpose();

		modelTarget -= shoulderPos;
		modelTarget *= transposedShoulderAxis;

		modelTarget -= shoulderToElbow;
	}


	if ( view ) {
		idVec3 target = modelTarget;

		const idVec3& dir = baseJointAxes[ ARM_JOINT_INDEX_MUZZLE ][ 0 ];
		
		target -= elbowToMuzzle - ( ( dir * elbowToMuzzle ) * dir );
		target *= baseJointAxes[ ARM_JOINT_INDEX_MUZZLE ].Transpose();

		newAngles.yaw = RAD2DEG( atan2( target[ 1 ], target[ 0 ] ) );
	} else {
		newAngles.yaw = 0;
	}

	bool yawChanged = !sdVehiclePosition::ClampAngle( newAngles, jointAngles, clampYaw, 1, 0.1f );
	if ( yawSound != NULL ) {
		yawSound->Update( yawChanged );
	}
	changed |= yawChanged;

	idMat3 yawMat;
	idAngles::YawToMat3( newAngles.yaw, yawMat );

	if ( view ) {
		idVec3 target = modelTarget;

		idVec3 newElbowToWrist	= elbowToWrist * yawMat;
		idVec3 newWristToMuzzle	= wristToMuzzle * yawMat;

		idMat3 muzzleAxis = baseJointAxes[ ARM_JOINT_INDEX_MUZZLE ] * yawMat;

		target -= newElbowToWrist;
		target -= newWristToMuzzle - ( ( muzzleAxis[ 0 ] * newWristToMuzzle ) * muzzleAxis[ 0 ] );
		target *= muzzleAxis.Transpose();

		newAngles.pitch = -RAD2DEG( atan2( target[ 2 ], target[ 0 ] ) );
	} else {
		newAngles.pitch = 0;
	}

	bool pitchChanged = !sdVehiclePosition::ClampAngle( newAngles, jointAngles, clampPitch,	0, 0.1f );
	if ( pitchSound != NULL ) {
		pitchSound->Update( pitchChanged );
	}
	changed |= pitchChanged;

	// configurable pitching axis - to support vertically oriented arms (eg badger, bumblebee)
	// as well as horizontally oriented arms (eg goliath)
	int truePitchAxis = pitchAxis;
	if ( truePitchAxis < 0 ) {
		newAngles.pitch = -newAngles.pitch;
		truePitchAxis = -truePitchAxis;
	}

	idAngles pitchAngles( 0.0f, 0.0f, 0.0f );
	if ( truePitchAxis == 1 ) {					// x-axis
		pitchAngles.roll = newAngles.pitch;
	} else if ( truePitchAxis == 3 ) {			// z-axis
		pitchAngles.yaw = newAngles.pitch;
	} else {									// y-axis
		pitchAngles.pitch = newAngles.pitch;
	}

	idMat3 pitchMat = pitchAngles.ToMat3();

	if( changed ) {
		oldParentAxis = temp;
		jointAngles = newAngles;

		animator->SetJointAxis( ikJoints[ ARM_JOINT_INDEX_ELBOW ], JOINTMOD_WORLD_OVERRIDE, baseJointAxes[ ARM_JOINT_INDEX_ELBOW ] * yawMat * shoulderAxis );
		animator->SetJointAxis( ikJoints[ ARM_JOINT_INDEX_WRIST ], JOINTMOD_WORLD_OVERRIDE, pitchMat * baseJointAxes[ ARM_JOINT_INDEX_WRIST ] * yawMat * shoulderAxis );
	}
}

/*
===============================================================================

	sdVehicleSwivel

===============================================================================
*/

CLASS_DECLARATION( sdVehicleIKSystem, sdVehicleSwivel )
END_CLASS

/*
================
sdVehicleSwivel::Setup
================
*/
bool sdVehicleSwivel::Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms ) {
	joint = INVALID_JOINT;

	if ( !sdVehicleIKSystem::Setup( _vehicle, yaw, pitch, ikParms ) ) {
		return false;
	}

	angles.Zero();

	idAnimator* animator = vehicle->GetAnimator();
	joint = animator->GetJointHandle( ikParms.GetString( "joint" ) );

	animator->GetJointTransform( joint, gameLocal.time, baseAxis );

	yawSound = NULL;
	if ( clampYaw.sound != NULL ) {
		yawSound = vehicle->GetMotorSounds().Alloc();
		yawSound->Start( clampYaw.sound );
	}

	return true;
}

/*
================
sdVehicleSwivel::Update
================
*/
void sdVehicleSwivel::Update( void ) {
	idEntity* vehicleEnt = vehicle;
	if ( !vehicleEnt ) {
		return;
	}

	idAnimator* animator = vehicleEnt->GetAnimator();

	idPlayer* player = GetPlayer();
	idAngles newAngles;

	if ( player ) {
		float diff = idMath::AngleDelta( player->clientViewAngles.yaw, angles.yaw );
		newAngles.yaw = angles.yaw + diff * (1.f-clampYaw.filter);//clampYaw.filter * angles.yaw + (1.f - clampYaw.filter) * player->clientViewAngles.yaw;
	} else {
		newAngles.yaw = 0;
	}

	bool changed = !sdVehiclePosition::ClampAngle( newAngles, angles, clampYaw,	1, 0.1f );
	if ( yawSound != NULL ) {
		yawSound->Update( changed );
	}

	if ( changed ) {
		angles = newAngles;

		idMat3 yawAxis;
		idAngles::YawToMat3( angles.yaw, yawAxis );
		animator->SetJointAxis( joint, JOINTMOD_WORLD, yawAxis );
	}
}

/*
===============================================================================

	sdVehicleJointAimer

===============================================================================
*/

CLASS_DECLARATION( sdVehicleIKSystem, sdVehicleJointAimer )
END_CLASS

/*
================
sdVehicleJointAimer::Setup
================
*/
bool sdVehicleJointAimer::Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms ) {
	joint = INVALID_JOINT;

	if ( !sdVehicleIKSystem::Setup( _vehicle, yaw, pitch, ikParms ) ) {
		return false;
	}

	yawSound = NULL;
	if ( clampYaw.sound != NULL ) {
		yawSound = vehicle->GetMotorSounds().Alloc();
		yawSound->Start( clampYaw.sound );
	}

	pitchSound = NULL;
	if ( clampPitch.sound != NULL ) {
		pitchSound = vehicle->GetMotorSounds().Alloc();
		pitchSound->Start( clampPitch.sound );
	}

	idAnimator* animator = vehicle->GetAnimator();
	joint = animator->GetJointHandle( ikParms.GetString( "joint" ) );

	if ( joint == INVALID_JOINT ) {
		return false;
	}

	animator->GetJointTransform( joint, gameLocal.time, baseAxis );
	angles = baseAxis.ToAngles();

	const char* weapon2Name = ikParms.GetString( "weapon2" );
	if ( *weapon2Name ) {
		weapon2 = _vehicle->GetWeapon( weapon2Name );
		if ( !weapon2 ) {
			gameLocal.Warning( "sdVehicleIKSystem::Setup Invalid Weapon '%s'", weapon2Name );
			return false;
		}
	} else {
		weapon2 = NULL;
	}

	return true;
}

/*
================
sdVehicleIKSystem::GetPlayer
================
*/
idPlayer* sdVehicleJointAimer::GetPlayer( void ) {	
	idPlayer* player = weapon ? weapon->GetPlayer() : position->GetPlayer();
	if ( !player ) {
		return weapon2 ? weapon2->GetPlayer() : position->GetPlayer();
	}

	return player;
}

/*
================
sdVehicleJointAimer::Update
================
*/
void sdVehicleJointAimer::Update( void ) {
	idEntity* vehicleEnt = vehicle;
	if ( !vehicleEnt ) {
		return;
	}

	if ( joint == INVALID_JOINT ) {
		return;
	}

	idAnimator* animator = vehicleEnt->GetAnimator();

	idPlayer* player = GetPlayer();
	idMat3 tempJointAxis;
	idVec3 jointPos;
	animator->GetJointTransform( joint, gameLocal.time, jointPos, tempJointAxis );
	renderView_t* view = player ? player->GetRenderView() : NULL;
	renderEntity_t* renderEnt = vehicleEnt->GetRenderEntity();

	trace_t trace;
	idVec3 modelTarget;
	if( view ) {
		// find what is being aimed at
		idVec3 end = view->vieworg + ( 8192 * view->viewaxis[ 0 ] );
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, view->vieworg, end, CONTENTS_SOLID | CONTENTS_OPAQUE, player );
		modelTarget = trace.endpos;

		// transform modelTarget into entity space
		modelTarget -= renderEnt->origin;
		modelTarget *= renderEnt->axis.Transpose();

		// make target relative to joint
		modelTarget -= jointPos;

		// calculate the vector to the target
		idVec3 direction = modelTarget;
		direction.Normalize();
		idMat3 newAxis = direction.ToMat3();
		idAngles newAngles = newAxis.ToAngles();
		
		// clamp the angles
		
		bool yawChanged = !sdVehiclePosition::ClampAngle( newAngles, angles, clampYaw, 1, 0.1f );
		if ( yawSound != NULL ) {
			yawSound->Update( yawChanged );
		}

		bool pitchChanged = !sdVehiclePosition::ClampAngle( newAngles, angles, clampPitch, 0, 0.1f );
		if ( pitchSound != NULL ) {
			pitchSound->Update( pitchChanged );
		}

		if ( yawChanged || pitchChanged ) {
			// set the angles
			angles = newAngles;
			animator->SetJointAxis( joint, JOINTMOD_WORLD_OVERRIDE, newAngles.ToMat3() );
		}
	}
}





/*
===============================================================================

  sdVehicleIK_Steering

===============================================================================
*/

CLASS_DECLARATION( sdVehicleIKSystem, sdVehicleIK_Steering )
END_CLASS

/*
================
sdVehicleIK_Steering::sdVehicleIK_Steering
================
*/
sdVehicleIK_Steering::sdVehicleIK_Steering( void ) {
}

/*
================
sdVehicleIK_Steering::~sdVehicleIK_Steering
================
*/
sdVehicleIK_Steering::~sdVehicleIK_Steering( void ) {
}

/*
================
sdVehicleIK_Steering::Init
================
*/
bool sdVehicleIK_Steering::Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms ) {
	if ( !sdVehicleIKSystem::Setup( _vehicle, yaw, pitch, ikParms ) ) {
		return false;
	}

	if ( !ik.Init( vehicle, ikParms ) ) {
		return false;
	}

	return true;
}

/*
================
sdVehicleIK_Steering::Update
================
*/
void sdVehicleIK_Steering::Update( void ) {
	idPlayer* player = GetPlayer();
	if ( player == NULL ) {
		return;
	}

	ik.Update( player, vehicle );
}





/*
===============================================================================

  sdVehicleWeaponAimer

===============================================================================
*/

CLASS_DECLARATION( sdVehicleIKSystem, sdVehicleWeaponAimer )
END_CLASS

/*
================
sdVehicleWeaponAimer::Update
================
*/
void sdVehicleWeaponAimer::Update( void ) {
	idPlayer* player = GetPlayer();
	if ( player ) {
		const renderView_t& renderView = player->renderView;

		trace_t trace;
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS trace, renderView.vieworg, renderView.vieworg + ( renderView.viewaxis[ 0 ] * 4096 ), CONTENTS_SOLID | CONTENTS_OPAQUE, player );

		aimer.SetTarget( trace.endpos );
	} else {
		aimer.ClearTarget();
	}

	aimer.Update();
}

/*
================
sdVehicleWeaponAimer::Setup
================
*/
bool sdVehicleWeaponAimer::Setup( sdTransport* _vehicle, const angleClamp_t& yaw, const angleClamp_t& pitch, const idDict& ikParms ) {
	if ( !sdVehicleIKSystem::Setup( _vehicle, yaw, pitch, ikParms ) ) {
		return false;
	}

	idAnimator* animator = vehicle->GetAnimator();

	jointHandle_t pitchJoint	= animator->GetJointHandle( ikParms.GetString( "jointWrist" ) );
	jointHandle_t yawJoint		= animator->GetJointHandle( ikParms.GetString( "jointElbow" ) );
	jointHandle_t muzzleJoint	= animator->GetJointHandle( ikParms.GetString( "jointMuzzle" ) );
	jointHandle_t shoulderJoint = animator->GetJointHandle( ikParms.GetString( "jointShoulder" ) );
	if ( shoulderJoint == INVALID_JOINT ) {
		shoulderJoint = animator->GetJointParent( yawJoint );
	}

	int anim = animator->GetAnim( ikParms.GetString( "deployed_anim" ) );

	aimer.Init( ikParms.GetBool( "fix_barrel" ), ikParms.GetBool( "invert_pitch" ), _vehicle, anim, yawJoint, pitchJoint, muzzleJoint, shoulderJoint, clampYaw, clampPitch );

	return true;
}
