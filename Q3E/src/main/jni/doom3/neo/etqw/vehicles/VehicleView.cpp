// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "VehicleView.h"
#include "VehicleWeapon.h"
#include "Transport.h"
#include "../ContentMask.h"

/*
===============================================================================

	sdVehicleView

===============================================================================
*/

sdVehicleViewFactory sdVehicleView::viewFactory;

/*
================
sdVehicleView::AllocView
================
*/
sdVehicleView* sdVehicleView::AllocView( const char* name ) {
	return viewFactory.CreateType( name );
}

/*
================
sdVehicleView::Startup
================
*/

void sdVehicleView::Startup( void ) {
	viewFactory.RegisterType( sdDampedVehicleView::TypeName(), sdVehicleViewFactory::Allocator< sdDampedVehicleView > );
	viewFactory.RegisterType( "", sdVehicleViewFactory::Allocator< sdDampedVehicleView > );
	viewFactory.RegisterType( sdDampedVehicleView_Pivot::TypeName(), sdVehicleViewFactory::Allocator< sdDampedVehicleView_Pivot > );
	viewFactory.RegisterType( sdDampedVehicleView_FreePivot::TypeName(), sdVehicleViewFactory::Allocator< sdDampedVehicleView_FreePivot > );
	viewFactory.RegisterType( sdDampedVehicleView_Player::TypeName(), sdVehicleViewFactory::Allocator< sdDampedVehicleView_Player > );
	viewFactory.RegisterType( sdSmoothVehicleView::TypeName(), sdVehicleViewFactory::Allocator< sdSmoothVehicleView > );
	viewFactory.RegisterType( sdSmoothVehicleView_Free::TypeName(), sdVehicleViewFactory::Allocator< sdSmoothVehicleView_Free > );
	viewFactory.RegisterType( sdSmoothVehicleView_Locked::TypeName(), sdVehicleViewFactory::Allocator< sdSmoothVehicleView_Locked > );
	viewFactory.RegisterType( sdIcarusVehicleView::TypeName(), sdVehicleViewFactory::Allocator< sdIcarusVehicleView > );
}

/*
================
sdVehicleView::Shutdown
================
*/

void sdVehicleView::Shutdown( void ) {
	viewFactory.Shutdown();
}

/*
================
sdVehicleView::Init
================
*/
void sdVehicleView::Init( sdVehiclePosition* _position, const positionViewMode_t& _viewMode ) {
	position			= _position;
	viewMode			= _viewMode;

	SetupEyes( position->GetTransport() );

	zoomTable			= gameLocal.declTableType[ viewMode.zoomTable ];

	sensitivityYaw		= cvarSystem->Find( viewMode.sensitivityYaw );
	sensitivityPitch	= cvarSystem->Find( viewMode.sensitivityPitch );
	sensitivityYawScale		= cvarSystem->Find( viewMode.sensitivityYawScale );
	sensitivityPitchScale	= cvarSystem->Find( viewMode.sensitivityPitchScale );

	thirdPersonViewOrigin.Zero();
	thirdPersonViewAxes.Identity();
}

/*
================
sdVehicleView::GetSensitivity
================
*/
bool sdVehicleView::GetSensitivity( float& x, float& y ) {
	bool changed = false;
	if ( sensitivityPitch != NULL ) {
		y = sensitivityPitch->GetFloat();
		changed = true;
	}
	if ( sensitivityYaw != NULL ) {
		x = sensitivityYaw->GetFloat();
		changed = true;
	}

	if ( sensitivityPitchScale != NULL ) {
		y *= sensitivityPitchScale->GetFloat();
		changed = true;
	}
	if ( sensitivityYawScale != NULL ) {
		x *= sensitivityYawScale->GetFloat();
		changed = true;
	}

	return changed;
}

/*
================
sdVehicleView::GetFov
================
*/
float sdVehicleView::GetFov( void ) const {
	if ( !zoomTable ) {
		return 90.0f;
	}

	idPlayer* player = position->GetPlayer();

	if ( player->vehicleViewCurrentZoom < 0 || player->vehicleViewCurrentZoom >= zoomTable->NumValues() ) {
		return 90.0f;
	}

	return zoomTable->GetValue( player->vehicleViewCurrentZoom );
}

/*
================
sdVehicleView::ZoomCycle
================
*/
void sdVehicleView::ZoomCycle( void ) const {
	if ( !zoomTable ) {
		return;
	}

	idPlayer* player = position->GetPlayer();
	player->vehicleViewCurrentZoom++;
	player->vehicleViewCurrentZoom %= zoomTable->NumValues() - 1;

	if ( player->vehicleViewCurrentZoom == 0 ) {
		gameSoundWorld->PlayShaderDirectly( viewMode.zoomOutSound, SND_VEHICLE_ZOOM );
	} else {
		gameSoundWorld->PlayShaderDirectly( viewMode.zoomInSound, SND_VEHICLE_ZOOM );
	}
}

/*
================
sdVehicleView::ClampViewAngles
================
*/
void sdVehicleView::ClampViewAngles( idAngles& viewAngles, const idAngles& oldViewAngles ) const {

	idAngles oldAngles = oldViewAngles;
	DoFreshEntryAngles( viewAngles, oldAngles );

	sdVehiclePosition::ClampAngle( viewAngles, oldAngles, viewMode.clampPitch, 0 );
	sdVehiclePosition::ClampAngle( viewAngles, oldAngles, viewMode.clampYaw, 1 );
}

/*
================
sdVehicleView::DoFreshEntryAngles
================
*/
void sdVehicleView::DoFreshEntryAngles( idAngles& viewAngles, idAngles& oldAngles ) const {
	if ( freshEntry ) {
		idAngles oldViewAngles = oldAngles;

		// calculate the angles needed to point in the same direction as pointed
		// when the player entered the vehicle
		freshEntry = false;
		CalcNewViewAngles( oldAngles );

		idAngles deltaAngles = viewAngles - oldViewAngles;
		viewAngles = oldAngles + deltaAngles;
		viewAngles.Normalize180();
	}
}

/*
================
sdVehicleView::ClipView
================
*/
void sdVehicleView::ClipView( idVec3& viewOrigin, const idVec3& pivotPoint ) {

	idEntity* owner = position->GetTransport();
	owner->DisableClip( false );

	// HACK: Disable clip of everything bound to it too
	for ( idEntity* ent = owner; ent != NULL; ent = ent->GetNextTeamEntity() ) {
		ent->DisableClip( false );
	}

	const idClipModel* thirdPersonModel = gameLocal.clip.GetThirdPersonOffsetModel();
	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
	trace_t trace;
	gameLocal.clip.Translation( CLIP_DEBUG_PARMS trace, pivotPoint, viewOrigin, thirdPersonModel, mat3_identity, MASK_SHOT_RENDERMODEL & ~CONTENTS_FORCEFIELD, owner );
	if ( trace.fraction != 1.0 ) {
		viewOrigin = trace.endpos;
		viewOrigin.z += ( 1.0f - trace.fraction ) * 32.0f;

		// try another trace to this position, because a tunnel may have the ceiling
		// close enough that this is poking out
		gameLocal.clip.Translation( CLIP_DEBUG_PARMS trace, pivotPoint, viewOrigin, thirdPersonModel, mat3_identity, MASK_SHOT_RENDERMODEL & ~CONTENTS_FORCEFIELD, owner );
		viewOrigin = trace.endpos;
	}

	owner->EnableClip();

	for ( idEntity* ent = owner; ent != NULL; ent = ent->GetNextTeamEntity() ) {
		ent->EnableClip();
	}
}

/*
================
sdVehicleView::Update
================
*/
void sdVehicleView::Update( sdVehicleWeapon* weapon ) {
}

/*
================
sdVehicleView::GetRequiredViewAngles
================
*/
const idAngles sdVehicleView::GetRequiredViewAngles( const idVec3& aimPosition ) const {
	return idAngles( vec3_origin );
}

/*
================
sdVehicleView::OnPlayerEntered
================
*/
void sdVehicleView::OnPlayerEntered( idPlayer* player ) {
	GetInitialViewAxis( player->firstPersonViewAxis );

	OnPlayerSwitched( player, true );
}

/*
================
sdVehicleView::OnPlayerSwitched
================
*/
void sdVehicleView::OnPlayerSwitched( idPlayer* player, bool newPosition ) {
	freshEntry = true;
	if ( GetViewParms().matchPrevious || !newPosition ) {
		// aim in the same direction as we were previously aiming
		entryAim = position->GetPlayer()->firstPersonViewAxis;
	} else {
		GetInitialViewAxis( entryAim );
	}
}

/*
================
sdVehicleView::GetInitialViewAxis
================
*/
void sdVehicleView::GetInitialViewAxis( idMat3& aim ) const {
	aim = position->GetTransport()->GetAxis();
}

/*
================
sdVehicleView::OnTeleport
================
*/
void sdVehicleView::OnTeleport( void ) {
	idPlayer* player = position->GetPlayer();
	OnPlayerEntered( player );
}

/*
================
sdVehicleView::ClampFinalAxis
================
*/
void sdVehicleView::ClampFinalAxis( idMat3& axis, const idMat3& baseAxis, idMat3& dampedAxis ) const {
	// clamp this final axis to the damped clamp values
	// need the fovs of the final rendering screen to do it
	const renderView_t& view = gameLocal.playerView.GetCurrentView();

	// calculate the clamps necessary to prevent the edge of the view exceeding the maximum
	angleClamp_t pitchClamp = viewMode.clampDampedPitch;
	angleClamp_t yawClamp = viewMode.clampDampedYaw;
	pitchClamp.extents[ 0 ] += view.fov_y * 0.5f;
	pitchClamp.extents[ 1 ] -= view.fov_y * 0.5f;
	yawClamp.extents[ 0 ] += view.fov_x * 0.5f;
	yawClamp.extents[ 1 ] -= view.fov_x * 0.5f;

	idMat3 dampedAxisTransform = axis * dampedAxis.Transpose();
	idMat3 localAxis = axis * baseAxis.Transpose();
	idAngles localAngles = localAxis.ToAngles();

	idAngles tempAngles = localAngles;
	if ( !sdVehiclePosition::ClampAngle( localAngles, tempAngles, pitchClamp, 0, 0.00001f )
		|| !sdVehiclePosition::ClampAngle( localAngles, tempAngles, yawClamp, 1, 0.00001f ) ) {

		axis = localAngles.ToMat3() * baseAxis;

		// now we have the final clamped axis, we can reverse engineer what the damped axis
		// should have been to achieve that... hopefully..
		dampedAxis = dampedAxisTransform.TransposeMultiply( axis );
	}
}

/*
===============================================================================

	sdDampedVehicleView

===============================================================================
*/


/*
================
sdDampedVehicleView::SetupEyes
================
*/
void sdDampedVehicleView::SetupEyes( idAnimatedEntity* other ) {
	idAnimator* animator	= other->GetAnimator();

	eyeJoint = animator->GetJointHandle( viewMode.eyes );

	eyeBaseAxis.Identity();

	if ( eyeJoint == INVALID_JOINT ) {
		gameLocal.Warning( "sdVehicleView::Init can't find eye joint '%s'", viewMode.eyes.c_str() );
		eyeBaseJoint = INVALID_JOINT;
	} else {
		eyeBaseJoint = animator->GetJointHandle( viewMode.eyePivot );
		if ( eyeBaseJoint == INVALID_JOINT ) {
			eyeBaseJoint = animator->GetJointParent( eyeJoint );
		}

		if ( eyeBaseJoint != INVALID_JOINT ) {
			idVec3 eyePos;
			idMat3 eyeAxis;

			other->GetJointTransformForAnim( eyeJoint, 1, gameLocal.time, eyePos, eyeAxis );
			other->GetJointTransformForAnim( eyeBaseJoint, 1, gameLocal.time, eyeBaseOffset, eyeBaseAxis );
			idMat3 eyeBaseAxisT = eyeBaseAxis.Transpose();

			eyeOffset = ( eyePos - eyeBaseOffset ) * eyeBaseAxisT;
			eyeAxisOffset = eyeBaseAxisT * eyeAxis;
		}
	}

	lastReturnedAxis = eyeBaseAxis;
}

/*
================
sdDampedVehicleView::GetDampingPos
================
*/
void sdDampedVehicleView::GetDampingPos( idVec3& pos ) const {
	position->GetTransport()->GetWorldOrigin( eyeJoint, pos );
}

/*
================
sdDampedVehicleView::GetDampingAxis
================
*/
void sdDampedVehicleView::GetDampingAxis( idMat3& axis ) const {
	position->GetTransport()->GetWorldAxis( eyeJoint, axis );
}

/*
================
sdDampedVehicleView::GetInitialViewAxis
================
*/
void sdDampedVehicleView::GetInitialViewAxis( idMat3& axis ) const {
	position->GetTransport()->GetWorldAxis( eyeJoint, axis );
}

/*
================
sdDampedVehicleView::DampEyeAxes
================
*/
void sdDampedVehicleView::DampEyeAxes( const idMat3& oldAxes, idMat3& outAxes ) {
	idMat3 vAxes;
	GetDampingAxis( vAxes );

	if ( !GetViewParms().allowDamping ) {
		outAxes = vAxes;
		return;
	}

	for ( int i = 0; i < 3; i++ ) {
		outAxes[ i ] = Lerp( oldAxes[ i ], vAxes[ i ], viewMode.dampCopyFactor[ i ] );
	}

	idQuat a = outAxes.ToQuat();
	idQuat b = vAxes.ToQuat();
	idQuat c;
	c.Slerp( a, b, MS2SEC( gameLocal.msec ) / viewMode.dampSpeed );
	c.Normalize();

	outAxes = c.ToMat3();
	outAxes.FixDenormals();
}

/*
================
sdDampedVehicleView::CalcViewOrigin
================
*/
void sdDampedVehicleView::CalcViewOrigin( const idMat3& dampedAxis, const idVec3& posIn, idVec3& posOut, const idAngles& angles ) {
	posOut = posIn;
}

/*
================
sdDampedVehicleView::CalcViewAxes
================
*/
void sdDampedVehicleView::CalcViewAxes( const idMat3& axisIn, idMat3& axisOut, const idAngles& angles ) {
	axisOut = angles.ToMat3() * axisIn;

	if ( !viewMode.thirdPerson ) {
		sdTransport* transport = position->GetTransport();

		idRotation rotation;
		rotation.SetVec( transport->GetPhysics()->GetAxis()[ 2 ] );
		rotation.SetAngle( position->GetCurrentViewOffset() );

		axisOut *= rotation.ToMat3();
	}
}

/*
===============
sdDampedVehicleView::CalcThirdPersonView
===============
*/
void sdDampedVehicleView::CalcThirdPersonView( const idVec3& fpOrigin, const idMat3& fpAxis ) {

	idEntity* owner = position->GetTransport();

	idVec3 origin			= fpOrigin;
	thirdPersonViewAxes		= fpAxis;

	idVec3 focusPoint	= origin + thirdPersonViewAxes[ 0 ] * viewMode.cameraFocus + owner->GetAxis()[ 2 ] * viewMode.cameraFocusHeight;
	idVec3 view			= origin;
	view.z += 8 + viewMode.cameraHeight;

	idAngles angles		= thirdPersonViewAxes.ToAngles();

	view -= viewMode.cameraDistance * thirdPersonViewAxes[ 0 ];

	ClipView( view, origin );

	// select pitch to look at focus point from vieworg
	focusPoint -= view;
	float focusDist = idMath::Sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;	// should never happen
	}

	angles.pitch = - RAD2DEG( atan2( focusPoint.z, focusDist ) );

	thirdPersonViewOrigin = view;
	thirdPersonViewAxes	= angles.ToMat3();
}

/*
================
sdDampedVehicleView::CalculateViewPos
================
*/
void sdDampedVehicleView::CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) {
	if ( fullUpdate ) {
		if ( freshEntry ) {
			// ensure that the angles have been clamped to the valid range
			idAngles tempAngles = player->clientViewAngles;
			ClampViewAngles( player->clientViewAngles, tempAngles );
			freshEntry = true;
		}


		// damping
		DampEyeAxes( player->vehicleEyeAxis, player->vehicleEyeAxis );
		GetDampingPos( player->vehicleEyeOrigin );
	}

	// axis
	CalcViewAxes( player->vehicleEyeAxis, axis, player->clientViewAngles );

	if ( fullUpdate ) {
		// clamp the final result
		idMat3 baseAxis;
		GetDampingAxis( baseAxis );
		ClampFinalAxis( axis, baseAxis, player->vehicleEyeAxis );
	}

	// origin
	CalcViewOrigin( player->vehicleEyeAxis, player->vehicleEyeOrigin, origin, player->clientViewAngles );


	if ( viewMode.thirdPerson ) {
		CalcThirdPersonView( origin, axis );
	}

	lastReturnedAxis = axis;
}

/*
================
sdDampedVehicleView::OnPlayerSwitched
================
*/
void sdDampedVehicleView::OnPlayerSwitched( idPlayer* player, bool newPosition ) {
	GetDampingAxis( player->vehicleEyeAxis );
	
	sdVehicleView::OnPlayerSwitched( player, newPosition );

	lastReturnedAxis = player->firstPersonViewAxis;
}

/*
================
sdDampedVehicleView::GetRequiredViewAngles
================
*/
const idAngles sdDampedVehicleView::GetRequiredViewAngles( const idVec3& aimPosition ) const {
	idVec3 eyePos;
	GetDampingPos( eyePos );

	idVec3 deltaPos = aimPosition - eyePos;
	const idMat3& ownerAxis = position->GetTransport()->GetAxis();

	// find the delta in car space
	idVec3 deltaDirection = deltaPos;
	deltaDirection.Normalize();
	deltaDirection = deltaDirection * ownerAxis.Transpose();

	idAngles result = deltaDirection.ToAngles();
	result.Normalize180();
	return result;
}

/*
================
sdDampedVehicleView::CalcNewViewAngles
================
*/
void sdDampedVehicleView::CalcNewViewAngles( idAngles& angles ) const {
	idMat3 dampAxis;
	GetDampingAxis( dampAxis );

	idMat3 angleMat = entryAim * dampAxis.Transpose();
	angles = angleMat[ 0 ].ToAngles();
}

/*
===============================================================================

	sdDampedVehicleView_Pivot

===============================================================================
*/

/*
================
sdDampedVehicleView_Pivot::GetDampingAxis
================
*/
void sdDampedVehicleView_Pivot::GetDampingAxis( idMat3& axis ) const {
	axis = position->GetTransport()->GetRenderEntity()->axis;
}

/*
================
sdDampedVehicleView_Pivot::GetDampingPos
================
*/
void sdDampedVehicleView_Pivot::GetDampingPos( idVec3& pos ) const {
	position->GetTransport()->GetWorldOrigin( eyeBaseJoint, pos );
}

/*
================
sdDampedVehicleView_Pivot::CalcViewOrigin
================
*/
void sdDampedVehicleView_Pivot::CalcViewOrigin( const idMat3& dampedAxis, const idVec3& posIn, idVec3& posOut, const idAngles& angles ) {
	idMat3 gunRotation = mat3_identity;

	idMat3 temp;

	if ( viewMode.followPitch ) {
		idAngles::PitchToMat3( angles.pitch, temp );
		gunRotation *= temp;
	}

	if ( viewMode.followYaw ) {
		idAngles::YawToMat3( angles.yaw, temp );
		gunRotation *= temp;
	}

	posOut = posIn + ( eyeOffset * eyeBaseAxis ) * gunRotation * dampedAxis;
}

/*
================
sdDampedVehicleView_Pivot::GetRequiredViewAngles
================
*/
const idAngles sdDampedVehicleView_Pivot::GetRequiredViewAngles( const idVec3& aimPosition ) const {
// FIXME: This isn't quite correct, it doesn't take the arm into account properly. 
	return sdDampedVehicleView::GetRequiredViewAngles( aimPosition );
}

/*
================
sdDampedVehicleView_Pivot::CalcNewViewAngles
================
*/
void sdDampedVehicleView_Pivot::CalcNewViewAngles( idAngles& angles ) const {
	sdDampedVehicleView::CalcNewViewAngles( angles );
}

/*
===============================================================================

	sdDampedVehicleView_FreePivot

===============================================================================
*/

/*
================
sdDampedVehicleView_FreePivot::GetDampingAxis
================
*/
void sdDampedVehicleView_FreePivot::GetDampingAxis( idMat3& axis ) const {
	axis = mat3_identity;
}

/*
================
sdDampedVehicleView_FreePivot::ClampViewAngles
================
*/
void sdDampedVehicleView_FreePivot::ClampViewAngles( idAngles& viewAngles, const idAngles& oldViewAngles ) const {
	if ( !freshEntry ) {
		// convert angle delta into one in screen space
		idAngles oldFinalViewAngles = lastReturnedAxis.ToAngles();

		idMat2 rotMat;
		rotMat.Rotation( DEG2RAD( oldFinalViewAngles.roll ) );
		idAngles deltaAngles = ( viewAngles - oldViewAngles ).Normalize180();
		idVec2 deltaVec( deltaAngles.yaw, deltaAngles.pitch );
		deltaVec *= rotMat;

		deltaAngles.yaw = deltaVec[ 0 ];
		deltaAngles.pitch = deltaVec[ 1 ];

		viewAngles = oldViewAngles + deltaAngles;
	}

	idAngles oldAngles = oldViewAngles;
	DoFreshEntryAngles( viewAngles, oldAngles );

	// do a clamp to prevent it going crazy when the player looks all the way up or all the way down
	angleClamp_t flipClamp;
	flipClamp.extents.x = -80.0f;
	flipClamp.extents.y = 80.0f;
	flipClamp.flags.enabled = true;
	flipClamp.flags.limitRate = false;

	sdVehiclePosition::ClampAngle( viewAngles, oldAngles, flipClamp, 0 );

	// move angles back into "clamping space" using the damping axis to find a transform
	idMat3 ownerAxis = position->GetTransport()->GetRenderEntity()->axis;
	idMat3 dampingAxis;
	GetDampingAxis( dampingAxis );

	idMat3 dampingSpaceToClampSpace = ownerAxis.TransposeMultiply( dampingAxis );

	// transform forward vectors into clamping space & convert back to angles
	idAngles ownerSpaceOldAngles = ( oldAngles.ToForward() * dampingSpaceToClampSpace ).ToAngles();
	idAngles ownerSpaceAngles = ( viewAngles.ToForward() * dampingSpaceToClampSpace ).ToAngles();
	ownerSpaceAngles.Normalize180();
	ownerSpaceOldAngles.Normalize180();

	sdVehiclePosition::ClampAngle( ownerSpaceAngles, ownerSpaceOldAngles, viewMode.clampPitch, 0 );
	sdVehiclePosition::ClampAngle( ownerSpaceAngles, ownerSpaceOldAngles, viewMode.clampYaw, 1 );

	// now transform the clamped angle back into clamping space
	viewAngles = ( dampingSpaceToClampSpace.TransposeMultiply( ownerSpaceAngles.ToForward() ) ).ToAngles();
	viewAngles.Normalize180();
}

/*
================
sdDampedVehicleView_FreePivot::CalcViewAxes
================
*/
void sdDampedVehicleView_FreePivot::CalcViewAxes( const idMat3& axisIn, idMat3& axisOut, const idAngles& angles ) {
	axisOut = angles.ToMat3() * axisIn;

	idMat3 ownerAxes = position->GetTransport()->GetRenderEntity()->axis;

	// stick the up axis of the vehicle in as the z axis and renormalize
	axisOut[ 2 ] = ownerAxes[ 2 ];

	axisOut[ 1 ].Cross( axisOut[ 2 ], axisOut[ 0 ] );
	axisOut[ 1 ].Normalize();

	axisOut[ 2 ].Cross( axisOut[ 0 ], axisOut[ 1 ] );
	axisOut[ 2 ].Normalize();
}

/*
================
sdDampedVehicleView_FreePivot::CalculateViewPos
================
*/
void sdDampedVehicleView_FreePivot::CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) {
	if ( fullUpdate ) {
		if ( freshEntry ) {
			// ensure that the angles have been clamped to the valid range
			idAngles tempAngles = player->clientViewAngles;
			ClampViewAngles( player->clientViewAngles, tempAngles );
			freshEntry = true;
		}

		// damping
		DampEyeAxes( player->vehicleEyeAxis, player->vehicleEyeAxis );
		GetDampingPos( player->vehicleEyeOrigin );
	}

	// axis
	CalcViewAxes( player->vehicleEyeAxis, axis, player->clientViewAngles );

	if ( fullUpdate ) {
		ClampFinalAxis( axis, mat3_identity, player->vehicleEyeAxis );
	}

	// origin
	CalcViewOrigin( axis, player->vehicleEyeOrigin, origin, ang_zero );

	if ( viewMode.thirdPerson ) {
		CalcThirdPersonView( origin, axis );
	}

	lastReturnedAxis = axis;
}

/*
================
sdDampedVehicleView_FreePivot::GetRequiredViewAngles
================
*/
const idAngles sdDampedVehicleView_FreePivot::GetRequiredViewAngles( const idVec3& aimPosition ) const {

// FIXME: This isn't quite correct, it doesn't take the arm into account properly. 

	idVec3 eyePos;
//	position->GetTransport()->GetWorldOrigin( eyeJoint, eyePos );
	GetDampingPos( eyePos );

	idVec3 deltaDirection = aimPosition - eyePos;
	deltaDirection.Normalize();

	deltaDirection *= eyeAxisOffset.Transpose();

	idAngles result = deltaDirection.ToAngles();
	result.Normalize180();
	return result;
}

/*
================
sdDampedVehicleView_FreePivot::CalcNewViewAngles
================
*/
void sdDampedVehicleView_FreePivot::CalcNewViewAngles( idAngles& angles ) const {
	angles = entryAim[ 0 ].ToAngles();
}


/*
===============================================================================

	sdDampedVehicleView_Player

===============================================================================
*/

/*
================
sdDampedVehicleView_Player::Init
================
*/
void sdDampedVehicleView_Player::Init( sdVehiclePosition* _position, const positionViewMode_t& _viewMode ) {
	position			= _position;
	viewMode			= _viewMode;

	zoomTable			= NULL;

	sensitivityYaw		= NULL;
	sensitivityPitch	= NULL;
	sensitivityYawScale		= NULL;
	sensitivityPitchScale	= NULL;
}

/*
================
sdDampedVehicleView_Player::CalcViewAxes
================
*/
void sdDampedVehicleView_Player::CalcViewAxes( const idMat3& axisIn, idMat3& axisOut, const idAngles& angles ) {
	axisOut = position->GetTransport()->GetAxis() * axisIn;
}

/*
================
sdDampedVehicleView_Player::CalcViewOrigin
================
*/
void sdDampedVehicleView_Player::CalcViewOrigin( const idMat3& dampedAxis, const idVec3& posIn, idVec3& posOut, const idAngles& angles ) {
	posOut = posIn;
}

/*
================
sdDampedVehicleView_Player::GetDampingPos
================
*/
void sdDampedVehicleView_Player::GetDampingPos( idVec3& pos ) const {
	renderEntity_t* renderEnt = position->GetTransport()->GetRenderEntity();
	pos = renderEnt->origin + idVec3( 0.f, 0.f, 72.f );
}

/*
================
sdDampedVehicleView_Player::GetDampingAxis
================
*/
void sdDampedVehicleView_Player::GetDampingAxis( idMat3& axis ) const {
	axis = mat3_identity;
}

/*
================
sdDampedVehicleView_Player::GetRequiredViewAngles
================
*/
const idAngles sdDampedVehicleView_Player::GetRequiredViewAngles( const idVec3& aimPosition ) const {
	return sdDampedVehicleView::GetRequiredViewAngles( aimPosition );
}

/*
================
sdDampedVehicleView_Player::CalcNewViewAngles
================
*/
void sdDampedVehicleView_Player::CalcNewViewAngles( idAngles& angles ) const {
}


/*
===============================================================================

	sdSmoothVehicleView

===============================================================================
*/

/*
================
sdSmoothVehicleView::Init
================
*/
void sdSmoothVehicleView::Init( sdVehiclePosition* _position, const positionViewMode_t& _viewMode ) {
	previousAimMatrix.Identity();
	previousRawAimMatrix.Identity();
	previousCameraDistance = _viewMode.cameraDistance;
	previousOwnerOrigin.Zero();
	previousCameraHeightDelta = 0.0f;

	sdVehicleView::Init( _position, _viewMode );
}

/*
================
sdSmoothVehicleView::ClampViewAngles
================
*/
void sdSmoothVehicleView::ClampViewAngles( idAngles& viewAngles, const idAngles& oldViewAngles ) const {

	idAngles oldAngles = oldViewAngles;
	DoFreshEntryAngles( viewAngles, oldAngles );

	idPhysics* ownerPhysics = position->GetTransport()->GetPhysics();
	const idMat3& ownerAxes = ownerPhysics->GetAxis();


	// transform the angles from world space into local space
	const idMat3 ownerAxesT = ownerAxes.Transpose();

	// pitch is not relative to the vehicle but yaw is!
	idAngles ownerAngles = ownerAxes.ToAngles();
	viewAngles.yaw += ownerAngles.yaw;
	oldAngles.yaw += ownerAngles.yaw;

	idAngles localSpaceViewAngles = ( viewAngles.ToMat3() * ownerAxesT ).ToAngles();
	idAngles localSpaceOldAngles = ( oldAngles.ToMat3() * ownerAxesT ).ToAngles();

	sdVehiclePosition::ClampAngle( localSpaceViewAngles, localSpaceOldAngles, viewMode.clampPitch, 0 );
	sdVehiclePosition::ClampAngle( localSpaceViewAngles, localSpaceOldAngles, viewMode.clampYaw, 1 );

	// transform the clamped angles back into world space
	viewAngles = ( localSpaceViewAngles.ToMat3() * ownerAxes )[ 0 ].ToAngles();

	viewAngles.yaw -= ownerAngles.yaw;

	// clean out the roll, it has a habit of accumulating little values
	viewAngles.roll = 0.0f;
}

/*
================
sdSmoothVehicleView::CalcNewViewAngles
================
*/
void sdSmoothVehicleView::CalcNewViewAngles( idAngles& angles ) const {
	// yaw relative to body
	const idMat3& ownerAxes = position->GetTransport()->GetPhysics()->GetAxis();
	idAngles ownerAngles = ownerAxes[ 0 ].ToAngles();

	angles = entryAim[ 0 ].ToAngles();
	angles.yaw -= ownerAngles.yaw;
	angles.Normalize180();
}

/*
================
sdSmoothVehicleView::CalculateAimMatrix
================
*/
void sdSmoothVehicleView::CalculateAimMatrix(  const viewEvalProperties_t& state ) {
	// in this mode yaw is based off the owner, but pitch is free to smooth out bumps :)
	idMat3 yawMat = ( idRotation( vec3_origin, idVec3( 0.0f, 0.0f, 1.0f ), -state.viewAngles.yaw - state.ownerAngles.yaw ) ).ToMat3();
	idMat3 pitchMat = ( idRotation( vec3_origin, idVec3( 0.0f, 1.0f, 0.0f ), -state.viewAngles.pitch ) ).ToMat3();

	evalState.aimMatrix = pitchMat * yawMat;
}

/*
================
sdSmoothVehicleView::ClampToViewConstraints
================
*/
void sdSmoothVehicleView::ClampToViewConstraints( const viewEvalProperties_t& state ) {
	// clamp the resultant angles
	idAngles localAxisAngles = ( evalState.cameraAxis * state.ownerAxesT ).ToAngles();
	idAngles clampedAxisAngles = localAxisAngles;
	bool changed = false;
	changed = !sdVehiclePosition::ClampAngle( clampedAxisAngles, localAxisAngles, viewMode.clampPitch, 0 );
	changed |= !sdVehiclePosition::ClampAngle( clampedAxisAngles, localAxisAngles, viewMode.clampYaw, 1 );
//	clampedAxisAngles.roll = 0.0f;

	idMat3 ownerSpaceAxis = clampedAxisAngles.ToMat3();
	if ( changed ) {
		evalState.cameraAxis = ownerSpaceAxis * state.ownerAxes;
	}
}

/*
================
sdSmoothVehicleView::ClampToWorld
================
*/
void sdSmoothVehicleView::ClampToWorld( const viewEvalProperties_t& state ) {
	ClipView( evalState.cameraOrigin, state.ownerCenter );
	evalState.newCameraDistance = ( state.ownerCenter - evalState.cameraOrigin ).Length();
}

/*
================
sdSmoothVehicleView::CalculateCameraDelta
================
*/
idVec3 sdSmoothVehicleView::CalculateCameraDelta( const viewEvalProperties_t& state ) {
	float cameraDist = viewMode.cameraDistance;
	float cameraHeight = viewMode.cameraHeight;

	idVec3 cameraDelta( -cameraDist, 0.0f, cameraHeight );
	cameraDelta = cameraDelta * evalState.aimMatrix;
	
	return cameraDelta;
}

/*
================
sdSmoothVehicleView::DampenMotion
================
*/
void sdSmoothVehicleView::DampenMotion( const viewEvalProperties_t& state ) {
	if ( !firstFrame ) {
		// find the difference in aiming matrices
		idMat3 aimingDiff = previousAimMatrix.Transpose() * ( evalState.ownerYawAxis.Transpose() * evalState.aimMatrix );

		// find the difference in result matrices
		idMat3 resultDiff = state.oldAxis.Transpose() * evalState.cameraAxis;

		// remove the result contribution by the view angle changes
		resultDiff = resultDiff * aimingDiff.Transpose();

		// now! this result is the change in viewing axis caused by car movement only!
		// damp out the yaw & pitch caused by the car movement
		idAngles angleDiff = resultDiff.ToAngles();
		angleDiff.yaw = DampenYaw( angleDiff.yaw );
		angleDiff.pitch = DampenPitch( angleDiff.pitch );

		resultDiff = angleDiff.ToMat3();
		evalState.cameraAxis = state.oldAxis * resultDiff * aimingDiff;

		// ok, this is kinda dodgy isn't it? but it smooths out the tiny
		// fluctuations that creep in at really low level
		idAngles tempAngles = evalState.cameraAxis.ToAngles();
		tempAngles.yaw = idMath::Floor( tempAngles.yaw * 100000.0f ) / 100000.0f;
		tempAngles.pitch = idMath::Floor( tempAngles.pitch * 100000.0f ) / 100000.0f;
		tempAngles.roll = idMath::Floor( tempAngles.roll * 100000.0f ) / 100000.0f;
		evalState.cameraAxis = tempAngles.ToMat3();

		// FINALLY - eliminate any roll component
		idVec3 newUp( 0.0f, 0.0f, 1.0f );
		idVec3 newRight = newUp.Cross( evalState.cameraAxis[ 0 ] );
		newRight.Normalize();
		newUp = evalState.cameraAxis[ 0 ].Cross( newRight );
		newUp.Normalize();

		evalState.cameraAxis[ 1 ] = newRight;
		evalState.cameraAxis[ 2 ] = newUp;
	}
}

/*
================
sdSmoothVehicleView::DoTeleporting
================
*/
void sdSmoothVehicleView::DoTeleporting( viewEvalProperties_t& state ) const {
	state.viewAngles.Zero();
}

/*
================
sdSmoothVehicleView::CalculateViewPos
================
*/
void sdSmoothVehicleView::CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) {
	if ( !fullUpdate ) {
		origin = thirdPersonViewOrigin;
		axis = thirdPersonViewAxes;
		return;
	}

	if ( freshEntry ) {
		// ensure that the angles have been clamped to the valid range
		idAngles tempAngles = player->clientViewAngles;
		ClampViewAngles( player->clientViewAngles, tempAngles );
		freshEntry = true;
	}

	// Harvest data
	viewEvalProperties_t state;
	state.driver = player;
	state.oldOrigin = origin;
	state.oldAxis = axis;
	state.oldAxisAngles = axis.ToAngles();
	state.viewAngles = player->clientViewAngles;
	state.owner = position->GetTransport();
	state.ownerPhysics = state.owner->GetPhysics();
	state.ownerOrigin = state.owner->GetLastPushedOrigin();
	state.ownerAxes = state.owner->GetLastPushedAxis();
	state.ownerAxesT = state.ownerAxes.Transpose();
	state.ownerAngles = state.ownerAxes.ToAngles(); 
	idAngles::YawToMat3( state.ownerAngles.yaw, evalState.ownerYawAxis );
	state.ownerVelocity = state.ownerPhysics->GetLinearVelocity();
	state.ownerDirection = state.ownerVelocity;
	state.ownerSpeed = state.ownerDirection.NormalizeFast();

	if ( position->GetTransport()->IsTeleporting() ) {
		DoTeleporting( state );
	}

	state.timeStep = MS2SEC( gameLocal.msec );

	evalState.cameraAxis = axis;

	// TODO: store this joint handle
	jointHandle_t eyeJoint = position->GetTransport()->GetAnimator()->GetJointHandle( viewMode.eyes );
	if ( eyeJoint != INVALID_JOINT ) {
		idMat3 tempAxes;
		state.owner->GetWorldOriginAxisNoUpdate( eyeJoint, state.ownerOrigin, tempAxes );
	}

	state.ownerCenter = state.ownerPhysics->GetBounds().GetCenter() * state.ownerAxes + state.ownerOrigin;
	previousOwnerOrigin = state.ownerOrigin;


	//
	// Aiming
	//
	CalculateAimMatrix( state );
	idVec3 cameraDelta = CalculateCameraDelta( state );	
	evalState.cameraOrigin = cameraDelta + state.ownerOrigin;
	
	previousRawAimMatrix = evalState.aimMatrix;

	//
	// Focusing on the target
	//

	// calculate the axes so its looking at the focus point
	float focusPointHeight = viewMode.cameraHeight;
	evalState.focusPoint = state.ownerOrigin + idVec3( 0.0f, 0.0f, focusPointHeight );

	idVec3 deltaToCar = evalState.focusPoint - evalState.cameraOrigin;
	deltaToCar.Normalize();
	evalState.cameraAxis = deltaToCar.ToMat3();

	//
	// Clamping to the view constraints
	//
	ClampToViewConstraints( state );

	// remove roll
	evalState.cameraAxis = ( evalState.cameraAxis[ 0 ].ToAngles() ).ToMat3();

	//
	// Prevent it clipping through the vehicle
	//
	float focusLength = ( evalState.focusPoint - evalState.cameraOrigin ).Length();

	//
	// Damping
	//

	// do some dampening of the camera movement based on the vehicle movement
	DampenMotion( state );
	previousAimMatrix = evalState.ownerYawAxis.Transpose() * evalState.aimMatrix;

	// fix up the camera origin (as the axis may have changed)
	evalState.cameraOrigin = evalState.focusPoint - evalState.cameraAxis[ 0 ] * focusLength;

	//
	// Prevent it clipping through the world
	//
	idVec3 distanceClampDirection = state.ownerCenter - evalState.cameraOrigin;
	evalState.newCameraDistance = distanceClampDirection.Normalize();
	evalState.newCameraHeightDelta = 0.0f;

	ClampToWorld( state );


	//
	// Prevent it clipping through the vehicle
	//
	idClipModel* combatModel = state.owner->GetCombatModel();
	if ( combatModel != NULL ) {
		idBounds bounds = combatModel->GetBounds();
		idBounds absBounds = combatModel->GetAbsBounds();

		idVec3 traceDownStart = evalState.cameraOrigin;
		idVec3 traceDownEnd = evalState.cameraOrigin;

		float traceLength = absBounds.GetSize().z;
		traceDownStart.z += traceLength;

		if ( absBounds.LineIntersection( traceDownStart, traceDownEnd ) ) {
			// it intersects the abs bounds so theres a chance that it'll collide with the combat model
			float traceRadius = 32.0f;
			traceDownStart.z -= traceRadius;
			traceDownEnd.z -= traceRadius;

			// check against the combat model
			trace_t trace;
			trace.fraction = 1.0f;
			gameLocal.clip.TraceRenderModel( trace, traceDownStart, traceDownEnd, traceRadius, mat3_identity, -1, combatModel );
			float tracedDistance = trace.fraction * traceLength;
			if ( trace.fraction < 1.0f ) {
				evalState.newCameraHeightDelta = trace.endpos.z + 8.0f - evalState.cameraOrigin.z;
			}
		}
	}

	//
	// Damp out the camera height when lowering rapidly
	//
	if ( evalState.newCameraHeightDelta < previousCameraHeightDelta ) {
		evalState.newCameraHeightDelta -= ( evalState.newCameraHeightDelta - previousCameraHeightDelta ) * ( 1.0f - viewMode.dampSpeed );
		if ( evalState.newCameraHeightDelta < 0.001f ) {
			evalState.newCameraHeightDelta = 0.001f;
		}
	}
	if ( evalState.newCameraHeightDelta > 0.0f ) {
		evalState.cameraOrigin.z += evalState.newCameraHeightDelta;

		// Re-clamp to the world to make sure it doesn't go through anything as a result
		ClampToWorld( state );
	}

	//
	// Damp out the camera distance when going out rapidly
	//
	distanceClampDirection = state.ownerCenter - evalState.cameraOrigin;
	evalState.newCameraDistance = distanceClampDirection.Normalize();
	if ( evalState.newCameraDistance > previousCameraDistance ) {
		evalState.newCameraDistance -= ( evalState.newCameraDistance - previousCameraDistance ) * ( 1.0f - viewMode.dampSpeed );
	}
	evalState.cameraOrigin = state.ownerCenter - distanceClampDirection * evalState.newCameraDistance;

	previousCameraDistance = evalState.newCameraDistance;
	previousCameraHeightDelta = evalState.newCameraHeightDelta;

	origin = evalState.cameraOrigin;
	axis = evalState.cameraAxis;

	// clamp the final result
	idMat3 temp = mat3_identity;
	ClampFinalAxis( axis, mat3_identity, temp );

	thirdPersonViewOrigin = origin;
	thirdPersonViewAxes = axis;

	firstFrame = false;
}

/*
================
sdSmoothVehicleView::SetupEyes
================
*/
void sdSmoothVehicleView::SetupEyes( idAnimatedEntity* other ) {
}

/*
================
sdSmoothVehicleView::GetRequiredViewAngles
================
*/
const idAngles sdSmoothVehicleView::GetRequiredViewAngles( const idVec3& aimPosition ) const {
	return idAngles( vec3_origin );
}

/*
================
sdSmoothVehicleView::OnPlayerSwitched
================
*/
void sdSmoothVehicleView::OnPlayerSwitched( idPlayer* player, bool newPosition ) {
	sdVehicleView::OnPlayerSwitched( player, newPosition );

	firstFrame = true;
	previousAimMatrix.Identity();
	previousRawAimMatrix.Identity();
	previousCameraDistance = viewMode.cameraDistance;
	previousOwnerOrigin.Zero();
	previousCameraHeightDelta = 0.0f;
}


/*
===============================================================================

	sdSmoothVehicleView_Free

===============================================================================
*/

/*
================
sdSmoothVehicleView_Free::ClampViewAngles
================
*/
void sdSmoothVehicleView_Free::ClampViewAngles( idAngles& viewAngles, const idAngles& oldViewAngles ) const {

	idAngles oldAngles = oldViewAngles;
	DoFreshEntryAngles( viewAngles, oldAngles );

	idPhysics* ownerPhysics = position->GetTransport()->GetPhysics();
	const idMat3& ownerAxes = ownerPhysics->GetAxis();

	// transform the angles from world space into local space
	const idMat3 ownerAxesT = ownerAxes.Transpose();

	idAngles localSpaceViewAngles = ( viewAngles.ToMat3() * ownerAxesT ).ToAngles();
	idAngles localSpaceOldAngles = ( oldAngles.ToMat3() * ownerAxesT ).ToAngles();

	sdVehiclePosition::ClampAngle( localSpaceViewAngles, localSpaceOldAngles, viewMode.clampPitch, 0 );
	sdVehiclePosition::ClampAngle( localSpaceViewAngles, localSpaceOldAngles, viewMode.clampYaw, 1 );

	// transform the clamped angles back into world space
	viewAngles = ( localSpaceViewAngles.ToMat3() * ownerAxes ).ToAngles();

	// clean out the roll, it has a habit of accumulating little values
	viewAngles.roll = 0.0f;
}

/*
================
sdSmoothVehicleView_Free::CalcNewViewAngles
================
*/
void sdSmoothVehicleView_Free::CalcNewViewAngles( idAngles& angles ) const {
	// angles relative to world
	angles = entryAim[ 0 ].ToAngles();
}

/*
================
sdSmoothVehicleView_Free::DoTeleporting
================
*/
void sdSmoothVehicleView_Free::DoTeleporting( viewEvalProperties_t& state ) const {
	state.viewAngles = state.ownerAngles;
}

/*
================
sdSmoothVehicleView_Free::CalculateAimMatrix
================
*/
void sdSmoothVehicleView_Free::CalculateAimMatrix( const viewEvalProperties_t& state ) {
	evalState.aimMatrix = state.viewAngles.ToMat3();
}

/*
===============================================================================

	sdSmoothVehicleView_Locked

===============================================================================
*/

/*
================
sdSmoothVehicleView_Locked::InTophat
================
*/
bool sdSmoothVehicleView_Locked::InTophat( const idPlayer* player ) const {
	if ( player == NULL ) {
		return false;
	}

	if ( player->GetUserInfo().drivingCameraFreelook ) {
		return !player->usercmd.buttons.btn.tophat;
	} else {
		return player->usercmd.buttons.btn.tophat;
	}
}

/*
================
sdSmoothVehicleView_Locked::ClampViewAngles
================
*/
void sdSmoothVehicleView_Locked::ClampViewAngles( idAngles& viewAngles, const idAngles& oldViewAngles ) const {
	idPlayer* player = position->GetPlayer();
	assert( player != NULL );

	if ( !InTophat( player ) ) {
		const idMat3& ownerAxes = position->GetTransport()->GetPhysics()->GetAxis();
		idAngles ownerAngles = ownerAxes[ 0 ].ToAngles();

		viewAngles = player->firstPersonViewAxis[ 0 ].ToAngles();
		viewAngles.yaw -= ownerAngles.yaw;
		viewAngles.roll = 0.0f;
		viewAngles.Normalize180();
	} else {
		idAngles oldAngles = oldViewAngles;
		DoFreshEntryAngles( viewAngles, oldAngles );

		viewAngles.Normalize180();
		viewAngles.pitch = idMath::ClampFloat( -40.0f, 40.0f, viewAngles.pitch );
	}
}

/*
================
sdSmoothVehicleView_Locked::CalculateViewPos
================
*/
void sdSmoothVehicleView_Locked::CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) {
	if ( !fullUpdate ) {
		origin = thirdPersonViewOrigin;
		axis = thirdPersonViewAxes;
		return;
	}

	// update transitioning in and out of tophat
	if ( InTophat( player )) {
		topHatTransition += MS2SEC( gameLocal.msec )*2.0f;
	} else {
		topHatTransition -= MS2SEC( gameLocal.msec )*2.0f;
	}
	topHatTransition = idMath::ClampFloat( 0.0f, 1.0f, topHatTransition );

	sdSmoothVehicleView::CalculateViewPos( player, origin, axis, fullUpdate );
}

/*
================
sdSmoothVehicleView_Locked::OnPlayerSwitched
================
*/
void sdSmoothVehicleView_Locked::OnPlayerSwitched( idPlayer* player, bool newPosition ) {
	sdSmoothVehicleView::OnPlayerSwitched( player, newPosition );
	topHatTransition = 0.0f;
	previousViewAngles = player->clientViewAngles;
}

/*
================
sdSmoothVehicleView_Locked::CalcNewViewAngles
================
*/
void sdSmoothVehicleView_Locked::CalcNewViewAngles( idAngles& angles ) const {
	// yaw relative to body
	const idMat3& ownerAxes = position->GetTransport()->GetPhysics()->GetAxis();
	idAngles ownerAngles = ownerAxes[ 0 ].ToAngles();

	angles = entryAim[ 0 ].ToAngles();
	angles.yaw -= ownerAngles.yaw;
	angles.Normalize180();
}

/*
================
sdSmoothVehicleView_Locked::DoTeleporting
================
*/
void sdSmoothVehicleView_Locked::DoTeleporting( viewEvalProperties_t& state ) const {
	state.viewAngles.Zero();
}

/*
================
sdSmoothVehicleView_Locked::CalculateAimMatrix
================
*/
void sdSmoothVehicleView_Locked::CalculateAimMatrix( const viewEvalProperties_t& state ) {
	// aggressively dampen the pitch
	idAngles idealAngles( state.ownerAngles.pitch * 0.5f, state.ownerAngles.yaw, 0.0f );

	// don't let it tilt up too far, it looks silly
	if ( idealAngles.pitch < -25.0f ) {
		idealAngles.pitch = -25.0f;
	}

	if ( !firstFrame ) {
		idAngles fromAngles = ( previousRawAimMatrix ).ToAngles();
		float pitchDiff = idMath::AngleDelta( idealAngles.pitch, fromAngles.pitch );
		float pitchDampSpeed = viewMode.dampSpeed * viewMode.dampSpeed;
		float ownerSpeedSqr = InchesToMetres( state.ownerSpeed*state.ownerSpeed );
		// hooray for magic numbers!
		// TODO: make this configurable, or at least a #define
		if ( ownerSpeedSqr < 7500.0f ) {
			float targetPitchDampSpeed = viewMode.dampSpeed;
			float distToGo = targetPitchDampSpeed - pitchDampSpeed;
			pitchDampSpeed += distToGo * ( 1.0f - ( ownerSpeedSqr / 7500.0f ) );
		}

		idealAngles.pitch = idMath::AngleNormalize180( fromAngles.pitch + pitchDiff * pitchDampSpeed );
	}

	evalState.aimMatrix = idealAngles.ToMat3();
}

/*
================
sdSmoothVehicleView_Locked::CalculateCameraDelta
================
*/
idVec3 sdSmoothVehicleView_Locked::CalculateCameraDelta( const viewEvalProperties_t& state ) {
	float cameraDist = viewMode.cameraDistance;
	float cameraHeight = viewMode.cameraHeight;

	idVec3 cameraDelta( -cameraDist, 0.0f, cameraHeight );
	cameraDelta = cameraDelta * evalState.aimMatrix;

	idVec3 noTopHatFutureDelta = cameraDelta;
	idVec3 currentDelta = state.oldOrigin - state.ownerOrigin;
	if ( !firstFrame && topHatTransition < 1.0f ) {
		// world space
		idVec3 futureDelta = cameraDelta;
		idVec3 futurePredictedPosition = state.ownerOrigin + /*state.ownerVelocity * 0.5f +*/ state.ownerAxes[ 0 ] * 128.0f;
		idVec3 turningPredict = 15.0f * ( state.ownerPhysics->GetAngularVelocity() * state.ownerAxes[ 2 ] ) * state.ownerAxes[ 1 ];
		if ( state.ownerAxes[ 0 ] * state.ownerVelocity < 0.0f ) {
			turningPredict = -turningPredict;
		}
	
		futurePredictedPosition += turningPredict;
		idVec3 directionToFuture = futurePredictedPosition - state.ownerOrigin;
		float distanceFromHere = directionToFuture.Normalize();
		if ( distanceFromHere < 32.0f ) {
			futureDelta = currentDelta;
		} else {
			directionToFuture.z = cameraHeight;
			directionToFuture.x *= -cameraDist;
			directionToFuture.y *= -cameraDist;
			futureDelta = directionToFuture;
		}

		// soften the blow - don't let the delta rotate too fast
		float currentYawAngle = RAD2DEG( idMath::ATan( currentDelta.y, currentDelta.x ) );
		float newYawAngle = RAD2DEG( idMath::ATan( futureDelta.y, futureDelta.x ) );
		float yawDiff = idMath::AngleDelta( newYawAngle, currentYawAngle );
		float maxRotateSpeedMin = 360.0f * MS2SEC( gameLocal.msec );
		float maxRotateSpeedMax = 2000.0f * MS2SEC( gameLocal.msec );
		float maxRotateSpeed = Lerp( maxRotateSpeedMin, maxRotateSpeedMax, idMath::Fabs( yawDiff ) / 180.0f );
		yawDiff = idMath::ClampFloat( -maxRotateSpeed, maxRotateSpeed, yawDiff );
		newYawAngle = currentYawAngle + yawDiff;
		futureDelta.x = cameraDist * idMath::Cos( DEG2RAD( newYawAngle ) );
		futureDelta.y = cameraDist * idMath::Sin( DEG2RAD( newYawAngle ) );
		
		// modify the aim matrix to fit what all this has done
		noTopHatFutureDelta = cameraDelta = futureDelta;
	} 

	if ( topHatTransition > 0.0f ) {
		idAngles topHatViewAngles = state.viewAngles;
		topHatViewAngles.yaw += state.ownerAngles.yaw;

		// dampen the vehicle's motion a little
		if ( topHatTransition >= 1.0f ) {
			float aimingDiff = idMath::AngleDelta( state.viewAngles.yaw, previousViewAngles.yaw );
			float resultDiff = idMath::AngleDelta( topHatViewAngles.yaw, state.oldAxisAngles.yaw );
			resultDiff = idMath::AngleDelta( resultDiff, aimingDiff );
			topHatViewAngles.yaw -= resultDiff * 0.8f;
		}

		idVec3 topHatDelta = topHatViewAngles.ToMat3()[ 0 ] * -cameraDist;
		topHatDelta.z += cameraHeight;

		// blend in the delta it'd have without top hat pressed
		if ( topHatTransition < 1.0f ) {
			cameraDelta = Lerp( noTopHatFutureDelta, topHatDelta, topHatTransition );
			idVec3 deltaDirection = cameraDelta;
			float deltaLength = deltaDirection.Normalize();
			if ( deltaLength > idMath::FLT_EPSILON ) { 
				float idealLength = topHatDelta.Length();
				cameraDelta = deltaDirection * idealLength;
			} else {
				cameraDelta = currentDelta;
			}
		} else {
			cameraDelta = topHatDelta;
		}
	}

	previousViewAngles = state.viewAngles;

	return cameraDelta;
}

/*
================
sdSmoothVehicleView_Locked::ClampToViewConstraints
================
*/
void sdSmoothVehicleView_Locked::ClampToViewConstraints( const viewEvalProperties_t& state ) {
	if ( InTophat( state.driver ) ) {
		// clamp the resultant angles
		idAngles localAxisAngles = ( evalState.cameraAxis * state.ownerAxesT ).ToAngles();
		localAxisAngles.roll = 0.0f;
		idAngles clampedAxisAngles = localAxisAngles;
		clampedAxisAngles.pitch = idMath::ClampFloat( -40.0f, 40.0f, clampedAxisAngles.pitch );

		if ( localAxisAngles != clampedAxisAngles ) {
			idMat3 ownerSpaceAxis = clampedAxisAngles.ToMat3();
			evalState.cameraAxis = ownerSpaceAxis * state.ownerAxes;
		}
	}
}

/*
================
sdSmoothVehicleView_Locked::DampenMotion
================
*/
void sdSmoothVehicleView_Locked::DampenMotion( const viewEvalProperties_t& state ) {
	bool topHatOverride = false;
	if ( topHatTransition == 1.0f ) {
		topHatOverride = true;
	}

	if ( !topHatOverride ) {
		sdSmoothVehicleView::DampenMotion( state );
	}
}

/*
================
sdSmoothVehicleView_Locked::DampenYaw
================
*/
float sdSmoothVehicleView_Locked::DampenYaw( float input ) const {
	float dampSpeed = Lerp( viewMode.dampSpeed, 1.0f, topHatTransition );
	return input * dampSpeed;
}

/*
================
sdSmoothVehicleView_Locked::DampenPitch
================
*/
float sdSmoothVehicleView_Locked::DampenPitch( float input ) const {
	float dampSpeed = Lerp( viewMode.dampSpeed, 1.0f, topHatTransition );
	return input * dampSpeed;
}

/*
===============================================================================

	sdIcarusVehicleView

===============================================================================
*/

/*
================
sdIcarusVehicleView::CalculateViewPos
================
*/
void sdIcarusVehicleView::CalculateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) {
	if ( freshEntry && fullUpdate ) {
		// ensure that the angles have been clamped to the valid range
		idAngles tempAngles = player->clientViewAngles;
		ClampViewAngles( player->clientViewAngles, tempAngles );
		freshEntry = true;
	}

	origin = position->GetTransport()->GetPhysics()->GetOrigin();
	axis = player->clientViewAngles.ToMat3();

	sdTeleporter* teleportEnt = position->GetTransport()->GetTeleportEntity();
	if ( teleportEnt != NULL ) {
		idPlayer* player = gameLocal.GetLocalViewPlayer();
		if ( player != NULL && player->GetProxyEntity() == position->GetTransport() ) {
			idEntity* viewer = teleportEnt->GetViewEntity();
			if ( viewer != NULL ) {
				origin	= viewer->GetPhysics()->GetOrigin();
				axis	= viewer->GetPhysics()->GetAxis();
			}
		}
	}
	
	origin += player->GetEyeOffset( idPlayer::EP_NORMAL );
	thirdPersonViewOrigin = origin;
	thirdPersonViewAxes = axis;
}

/*
================
sdIcarusVehicleView::GetRequiredViewAngles
================
*/
const idAngles sdIcarusVehicleView::GetRequiredViewAngles( const idVec3& aimPosition ) const {
	idVec3 deltaToTarget = aimPosition - position->GetTransport()->GetPhysics()->GetOrigin();
	deltaToTarget.Normalize();
	return deltaToTarget.ToAngles();
}

/*
================
sdIcarusVehicleView::SetupEyes
================
*/
void sdIcarusVehicleView::SetupEyes( idAnimatedEntity* other ) {
}

/*
================
sdIcarusVehicleView::CalcNewViewAngles
================
*/
void sdIcarusVehicleView::CalcNewViewAngles( idAngles& angles ) const {
}
