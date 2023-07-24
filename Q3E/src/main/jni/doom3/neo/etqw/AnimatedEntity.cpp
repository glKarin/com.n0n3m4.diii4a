// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "AnimatedEntity.h"
#include "physics/Clip.h"

/*
===============================================================================

	idAnimatedEntity

===============================================================================
*/

extern const idEventDef EV_PlayAnim;
extern const idEventDef EV_PlayCycle;
extern const idEventDef EV_PlayAnimBlended;
extern const idEventDef EV_GetAnimatingOnChannel;
extern const idEventDef EV_SetJointPos;
extern const idEventDef EV_SetJointAngle;
extern const idEventDef EV_GetJointPos;
extern const idEventDef EV_SetAnimFrame;
extern const idEventDef EV_GetNumFrames;

const idEventDef EV_ClearAllJoints( "clearAllJoints", '\0', DOC_TEXT( "Clears all joint modifications." ), 0, "Joint modifications are added using $event:setJointPos$ or $event:setJointAngle$." );
const idEventDef EV_ClearJoint( "clearJoint", '\0', DOC_TEXT( "Clears joint modifications on a specific joint." ), 1, "You can use $event:clearAllJoints$ to clear all modifications.", "d", "joint", "Index of the joint to clear." );
const idEventDef EV_SetJointPos( "setJointPos", '\0', DOC_TEXT( "Adds an origin joint modification." ), 3, "Valid values for mode are JOINTMOD_NONE, JOINTMOD_LOCAL, JOINTMOD_LOCAL_OVERRIDE, JOINTMOD_WORLD, or JOINTMOD_WORLD_OVERRIDE.\nNONE: Clears any modification.\nLOCAL: Offset in joint space.\nLOCAL_OVERRIDE: Set in joint space.\nWORLD: Offset in model space.\nWORLD_OFFSET: Set in model space.", "d", "joint", "Index of the joint.", "d", "mode", "Type of modification.", "v", "offset", "Offset to apply." );
const idEventDef EV_SetJointAngle( "setJointAngle", '\0', DOC_TEXT( "Adds an axes joint modification." ), 3, "Valid values for mode are JOINTMOD_NONE, JOINTMOD_LOCAL, JOINTMOD_LOCAL_OVERRIDE, JOINTMOD_WORLD, or JOINTMOD_WORLD_OVERRIDE.\nNONE: Clears any modification.\nLOCAL: Offset in joint space.\nLOCAL_OVERRIDE: Set in joint space.\nWORLD: Offset in model space.\nWORLD_OFFSET: Set in model space.", "d", "joint", "Index of the joint.", "d", "mode", "Type of modification.", "v", "offset", "Offset angles to apply." );
const idEventDef EV_GetJointPos( "getJointPos", 'v', DOC_TEXT( "Returns the world space position of a joint." ), 1, "If the lookup fails, '0 0 0' will be returned.", "d", "joint", "Index of the joint." );
const idEventDef EV_GetLocalJointPos( "getLocalJointPos", 'v', DOC_TEXT( "Returns the model space position of a joint." ), 1, "If the lookup fails, '0 0 0' will be returned.", "d", "joint", "Index of the joint." );
const idEventDef EV_GetJointAxis( "getJointAxis", 'v', DOC_TEXT( "Returns the single axis of a joint in world space." ), 2, "If the lookup fails, the axis of an identity matrix will be returned.\nThe axis index must be in the range 0-2 otherwise the game will likely crash/return garbage data.", "d", "joint", "Index of the joint.", "d", "axis", "Index of the axis." );
const idEventDef EV_GetJointAngle( "getJointAngle", 'v', DOC_TEXT( "Returns the angles of a joint in world space." ), 1, "If the lookup fails, '0 0 0' will be returned.", "d", "joint", "Index of the joint." );
const idEventDef EV_SetAnimFrame( "setAnimFrame", '\0', DOC_TEXT( "Sets the specified channel to a fixed frame in an animation." ), 3, NULL, "s", "anim", "Name of the animation to play.", "d", "channel", "Channel to set the fixed frame on.", "f", "frame", "Fixed frame to use." );
const idEventDef EV_PlayAnim( "playAnim", 'f', DOC_TEXT( "Plays the given animation once on the specified channel, and returns the length of the animation." ), 2, "If the animation isn't found, the result will be 0.", "d", "channel", "Channel to play the animation on.", "s", "anim", "Name of the animation to play." );
const idEventDef EV_PlayCycle( "playCycle", 'f', DOC_TEXT( "Loops the given animation on the specified channel, and returns the length of the animation." ), 2, "If the animation isn't found, the result will be 0.", "d", "channel", "Channel to play the animation on.", "s", "anim", "Name of the animation to play." );
const idEventDef EV_GetAnimLength( "getAnimLength", 'f', DOC_TEXT( "Returns the length of the animation in seconds." ), 1, "If the animation isn't found, the result will be 0.", "s", "anim", "Name of the animation." );
const idEventDef EV_PlayAnimBlended( "playAnimBlended", 'f', DOC_TEXT( "Plays the given animation once on the specified channel using the blend time to blend from the current pose, and returns the length of the animation." ), 3, "If the animation isn't found, the result will be 0.", "d", "channel", "Channel to play the animation on.", "s", "anim", "Name of the animation to play.", "f", "blend", "Length of time is seconds to blend from the current pose." );
const idEventDef EV_IsAnimating( "isAnimating", 'b', DOC_TEXT( "Returns whether there are any currently active animations." ), 0, NULL );
const idEventDef EV_AnimName( "animationName", 's', DOC_TEXT( "Returns the name of the animation at the specified index." ), 1, "If the index is invalid an empty string will be returned.", "d", "index", "Index of the animation." );
const idEventDef EV_IsAnimatingOnChannel( "isAnimatingOnChannel", 'b', DOC_TEXT( "Returns whether there are any currently active animations on the given channel." ), 1, NULL, "d", "channel", "Channel to check." );
const idEventDef EV_GetAnimatingOnChannel( "getAnimatingOnChannel", 's', DOC_TEXT( "Returns the name of the primary animation playing on the specified channel." ), 1, "If there is no animation playing on the channel, the result will be an empty string.", "d", "channel", "Channel to check." );
const idEventDef EV_JointToWorldSpace( "jointToWorldSpace", 'v', DOC_TEXT( "Converts a vector from joint space into world space." ), 2, "If the joint lookup fails, the input vector will be returned.", "d", "joint", "Joint to convert with.", "v", "value", "Vector to convert." );
const idEventDef EV_WorldToModelSpace( "worldToModelSpace", 'v', DOC_TEXT( "Converts a vector from world space to model space." ), 1, NULL, "v", "value", "Vector to convert." );
const idEventDef EV_HideSurface( "hideSurface", '\0', DOC_TEXT( "Hides the surface at the given index." ), 1, "Index must be in the range 0-63.\nSee also $event:showSurface$ and $event:getSurfaceId$.", "d", "index", "Index of the surface to hide." );
const idEventDef EV_ShowSurface( "showSurface", '\0', DOC_TEXT( "Shows the surface at the given index." ), 1, "Index must be in the range 0-63.\nSee also $event:hideSurface$ and $event:getSurfaceId$.", "d", "index", "Index of the surface to show." );
const idEventDef EV_GetSurfaceId( "getSurfaceId", 'f', DOC_TEXT( "Returns the index of the surface with the given name." ), 1, "If the surface cannot be found, the result will be -1.", "s", "name", "Name of the surface to look up." );
const idEventDef EV_IsSurfaceHidden( "isSurfaceHidden", 'b', DOC_TEXT( "Returns whether the surface at the given index is hidden or not." ), 1, "Index must be in the range 0-63.", "d", "index", "Index of the surface." );
const idEventDef EV_GetNumFrames( "getNumFrames", 'f', DOC_TEXT( "Returns the number of frames in the given animation." ), 1, "If the animation cannot be found, the result will be 0.", "s", "name", "Name of the animation." );

CLASS_DECLARATION( idEntity, idAnimatedEntity )
	EVENT( EV_ClearAllJoints,		idAnimatedEntity::Event_ClearAllJoints )
	EVENT( EV_ClearJoint,			idAnimatedEntity::Event_ClearJoint )
	EVENT( EV_SetJointPos,			idAnimatedEntity::Event_SetJointPos )
	EVENT( EV_SetJointAngle,		idAnimatedEntity::Event_SetJointAngle )
	EVENT( EV_GetJointPos,			idAnimatedEntity::Event_GetJointPos )
	EVENT( EV_GetLocalJointPos,		idAnimatedEntity::Event_GetLocalJointPos )
	EVENT( EV_GetJointAxis,			idAnimatedEntity::Event_GetJointAxis )
	EVENT( EV_GetJointAngle,		idAnimatedEntity::Event_GetJointAngle )
	EVENT( EV_PlayAnim,				idAnimatedEntity::Event_PlayAnim )
	EVENT( EV_SetAnimFrame,			idAnimatedEntity::Event_SetAnimFrame )
	EVENT( EV_PlayCycle,			idAnimatedEntity::Event_PlayCycle )
	EVENT( EV_PlayAnimBlended,		idAnimatedEntity::Event_PlayAnimBlended )
	EVENT( EV_IsAnimating,			idAnimatedEntity::Event_IsAnimating )
	EVENT( EV_IsAnimatingOnChannel,	idAnimatedEntity::Event_IsAnimatingOnChannel )
	EVENT( EV_GetAnimatingOnChannel,idAnimatedEntity::Event_GetAnimatingOnChannel )
	EVENT( EV_JointToWorldSpace,	idAnimatedEntity::Event_JointToWorldTransform )
	EVENT( EV_WorldToModelSpace,	idAnimatedEntity::Event_WorldToModelSpace )
	EVENT( EV_GetAnimLength,		idAnimatedEntity::Event_AnimLength )
	EVENT( EV_AnimName,				idAnimatedEntity::Event_AnimName )
	EVENT( EV_HideSurface,			idAnimatedEntity::Event_HideSurface )
	EVENT( EV_ShowSurface,			idAnimatedEntity::Event_ShowSurface )
	EVENT( EV_GetSurfaceId,			idAnimatedEntity::Event_GetSurfaceId )
	EVENT( EV_IsSurfaceHidden,		idAnimatedEntity::Event_IsSurfaceHidden )
	EVENT( EV_GetNumFrames,			idAnimatedEntity::Event_GetNumFrames )
END_CLASS

/*
================
idAnimatedEntity::idAnimatedEntity
================
*/
idAnimatedEntity::idAnimatedEntity() {
	animator.SetEntity( this );
	combatModel = NULL;
	lastServiceTime = 0;
	selectionBounds.Clear();
}

/*
================
idAnimatedEntity::~idAnimatedEntity
================
*/
idAnimatedEntity::~idAnimatedEntity() {
	gameLocal.clip.DeleteClipModel( combatModel );
}


/*
================
idAnimatedEntity::Event_IsAnimating
================
*/
void idAnimatedEntity::Event_IsAnimating( void ) {
	sdProgram::ReturnBoolean( animator.IsAnimating( gameLocal.time ) );
}

/*
================
idAnimatedEntity::Event_IsAnimatingOnChannel
================
*/
void idAnimatedEntity::Event_IsAnimatingOnChannel( animChannel_t channel ) {
	sdProgram::ReturnBoolean( animator.IsAnimatingOnChannel( channel, gameLocal.time ) );
}

/*
================
idAnimatedEntity::Event_SetAnimFrame
================
*/
void idAnimatedEntity::Event_SetAnimFrame( const char* animname, animChannel_t channel, float frame ) {
	int anim = animator.GetAnim( animname );

	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) { 
			gameLocal.Warning( "idAnimatedEntity::Event_SetAnimFrame missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		}
		animator.Clear( channel, gameLocal.time, 0 );
	} else {
		animator.SetFrame( channel, anim, frame, gameLocal.time, 0 );
	}
}

/*
================
idAnimatedEntity::Event_PlayAnim
================
*/
void idAnimatedEntity::Event_PlayAnim( animChannel_t channel, const char *animname ) {
	int anim = animator.GetAnim( animname );

	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) { 
			gameLocal.Warning( "idAnimatedEntity::Event_PlayAnim missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		}		
		animator.Clear( channel, gameLocal.time, 0 );
		sdProgram::ReturnFloat( 0 );
	} else {
		animator.PlayAnim( channel, anim, gameLocal.time, 0 );
		sdProgram::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
	}
}

/*
================
idAnimatedEntity::Event_PlayCycle
================
*/
void idAnimatedEntity::Event_PlayCycle( animChannel_t channel, const char *animname ) {
	int anim = animator.GetAnim( animname );

	if ( anim == 0 ) {
		if( anim_showMissingAnims.GetBool() ) { 
			gameLocal.Warning( "idAnimatedEntity::Event_PlayCycle missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		}
		animator.Clear( channel, gameLocal.time, 0 );
		sdProgram::ReturnFloat( 0.0f );
	} else {
		animator.CycleAnim( channel, anim, gameLocal.time, 0 );
		sdProgram::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
	}
}

/*
================
idAnimatedEntity::Event_PlayAnimBlended
================
*/
void idAnimatedEntity::Event_PlayAnimBlended( animChannel_t channel, const char *animname, float blendTime ) {
	int anim = animator.GetAnim( animname );

	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) { 
			gameLocal.Warning( "idAnimatedEntity::Event_PlayAnim missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		}
		animator.Clear( channel, gameLocal.time, 0 );
		sdProgram::ReturnFloat( 0 );
	} else {
		animator.PlayAnim( channel, anim, gameLocal.time, SEC2MS( blendTime ) );
		sdProgram::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
	}
}

/*
================
idAnimatedEntity::Event_GetAnimatingOnChannel
================
*/
void idAnimatedEntity::Event_GetAnimatingOnChannel( animChannel_t channel ) {
	idAnimBlend *blend = animator.CurrentAnim( channel );
	if ( !blend ) {
		sdProgram::ReturnString( "" );
		return;
	}

	int index = blend->AnimNum();
	const idAnim* anim = animator.GetAnim( index );
	if ( !anim ) {
		sdProgram::ReturnString( "" );
		return;
	}

	sdProgram::ReturnString( anim->FullName() );
}

/*
================
idAnimatedEntity::Think
================
*/
void idAnimatedEntity::Think( void ) {
	RunPhysics();
	UpdateAnimation();
	Present();
}

/*
================
idAnimatedEntity::UpdateAnimation
================
*/
void idAnimatedEntity::UpdateAnimation( void ) {
	// don't do animations if they're not enabled
	if ( !( thinkFlags & TH_ANIMATE ) ) {
		return;
	}

	// is the model an MD5?
	if ( !animator.ModelHandle() ) {
		// no, so nothing to do
		return;
	}

	// call any frame commands that have happened in the past frame
	if ( !fl.hidden ) {
		if ( gameLocal.time > lastServiceTime ) {
			animator.ServiceAnims( lastServiceTime, gameLocal.time );
			lastServiceTime = gameLocal.time;
		}
	}

	// if the model is animating then we have to update it
	if ( !animator.FrameHasChanged( gameLocal.time ) ) {
		if ( fl.hidden ) {
			BecomeInactive( TH_ANIMATE );
		}
		// still fine the way it was
		return;
	}

	if ( gameLocal.isNewFrame ) {

		// get the latest frame bounds
		animator.GetBounds( gameLocal.time, renderEntity.bounds );
		if ( renderEntity.bounds.IsCleared() && !fl.hidden ) {
			gameLocal.DPrintf( "%d: inside out bounds\n", gameLocal.time );
		}
	}

	// update the renderEntity
	UpdateVisuals();

	// the animation is updated
	animator.ClearForceUpdate();
}

/*
================
idAnimatedEntity::GetAnimator
================
*/
idAnimator *idAnimatedEntity::GetAnimator( void ) {
	return &animator;
}

/*
================
idAnimatedEntity::SetModel
================
*/
void idAnimatedEntity::SetModel( const char *modelname ) {
	FreeModelDef();

	renderEntity.hModel = animator.SetModel( modelname );
	if ( !renderEntity.hModel ) {
		idEntity::SetModel( modelname );
		return;
	}

	if ( !renderEntity.customSkin ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}

	// set the callback to update the joints
	renderEntity.callback = idEntity::ModelCallback;
	animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	animator.GetBounds( gameLocal.time, renderEntity.bounds );

	UpdateVisuals();
}

/*
=====================
idAnimatedEntity::GetJointWorldTransform
=====================
*/
bool idAnimatedEntity::GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( !animator.GetJointTransform( jointHandle, currentTime, offset, axis ) ) {
		offset.Zero();
		axis.Identity();
		return false;
	}

	ConvertLocalToWorldTransform( offset, axis );
	return true;
}

/*
==============
idAnimatedEntity::GetJointTransformForAnim
==============
*/
bool idAnimatedEntity::GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idMat3 &axis ) const {
	const idAnim	*anim;
	int				numJoints;
	idJointMat		*frame;

	anim = animator.GetAnim( animNum );
	if ( !anim ) {
		assert( false );
		axis.Identity();
		return false;
	}

	numJoints = animator.NumJoints();
	if ( ( jointHandle < 0 ) || ( jointHandle >= numJoints ) ) {
		assert( false );
		axis.Identity();
		return false;
	}

	frame = ( idJointMat * )_alloca16( numJoints * sizeof( idJointMat ) );
	gameEdit->ANIM_CreateAnimFrame( animator.ModelHandle(), anim->MD5Anim( 0 ), renderEntity.numJoints, frame, frameTime, animator.ModelDef()->GetVisualOffset(), animator.RemoveOrigin() );

	axis = frame[ jointHandle ].ToMat3();

	return true;
}

/*
==============
idAnimatedEntity::GetJointTransformForAnim
==============
*/
bool idAnimatedEntity::GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idVec3 &offset, idMat3 &axis ) const {
	const idAnim	*anim;
	int				numJoints;
	idJointMat		*frame;

	anim = animator.GetAnim( animNum );
	if ( !anim ) {
		assert( false );
		offset.Zero();
		axis.Identity();
		return false;
	}

	numJoints = animator.NumJoints();
	if ( ( jointHandle < 0 ) || ( jointHandle >= numJoints ) ) {
		assert( false );
		offset.Zero();
		axis.Identity();
		return false;
	}

	frame = ( idJointMat * )_alloca16( numJoints * sizeof( idJointMat ) );
	gameEdit->ANIM_CreateAnimFrame( animator.ModelHandle(), anim->MD5Anim( 0 ), renderEntity.numJoints, frame, frameTime, animator.ModelDef()->GetVisualOffset(), animator.RemoveOrigin() );

	offset = frame[ jointHandle ].ToVec3();
	axis = frame[ jointHandle ].ToMat3();

	return true;
}

/*
==============
idAnimatedEntity::GetJointTransformForAnim
==============
*/
bool idAnimatedEntity::GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idVec3 &offset ) const {
	const idAnim	*anim;
	int				numJoints;
	idJointMat		*frame;

	anim = animator.GetAnim( animNum );
	if ( !anim ) {
		assert( false );
		offset.Zero();
		return false;
	}

	numJoints = animator.NumJoints();
	if ( ( jointHandle < 0 ) || ( jointHandle >= numJoints ) ) {
		assert( false );
		offset.Zero();
		return false;
	}

	frame = ( idJointMat * )_alloca16( numJoints * sizeof( idJointMat ) );
	gameEdit->ANIM_CreateAnimFrame( animator.ModelHandle(), anim->MD5Anim( 0 ), renderEntity.numJoints, frame, frameTime, animator.ModelDef()->GetVisualOffset(), animator.RemoveOrigin() );

	offset = frame[ jointHandle ].ToVec3();

	return true;
}

/*
================
idAnimatedEntity::LinkCombat
================
*/
void idAnimatedEntity::LinkCombat( void ) {
	if ( !fl.hidden ) {
		if ( combatModel && modelDefHandle != -1 ) {
			combatModel->Link( gameLocal.clip, this, 999, renderEntity.origin, renderEntity.axis, modelDefHandle );
		}
	}
}

/*
================
idAnimatedEntity::FreeModelDef
================
*/
void idAnimatedEntity::FreeModelDef( void ) {
	idEntity::FreeModelDef();
	UnLinkCombat();
}

/*
================
idAnimatedEntity::OnModelDefCreated
================
*/
void idAnimatedEntity::OnModelDefCreated( void ) {
	idEntity::OnModelDefCreated();
	LinkCombat();
}

/*
================
idAnimatedEntity::SetCombatModel
================
*/
void idAnimatedEntity::SetCombatModel( void ) {
	gameLocal.clip.DeleteClipModel( combatModel );
	combatModel = NULL;

	UpdateVisuals();
	Present();

	if ( modelDefHandle != -1 ) {
		combatModel = new idClipModel( modelDefHandle );
		LinkCombat();
	} else {
		gameLocal.Printf( "Failed to create combat model on %s\n", GetName() );
	}
}

/*
================
idAnimatedEntity::SetSelectionCombatModel
================
*/
void idAnimatedEntity::SetSelectionCombatModel( void ) {
	selectionBounds = renderEntity.bounds.Expand( 64.f );
}

/*
================
idAnimatedEntity::FreeSelectionCombatModel
================
*/
void idAnimatedEntity::FreeSelectionCombatModel( void ) {
	selectionBounds.Clear();
}

/*
================
idAnimatedEntity::SetSelectionCombatModel
================
*/
const idBounds*	idAnimatedEntity::GetSelectionBounds( void ) const {
	if ( !selectionBounds.IsCleared() ) {
		return &selectionBounds;
	}
	return NULL;
}


/*
================
idAnimatedEntity::OnUpdateVisuals
================
*/
void idAnimatedEntity::OnUpdateVisuals( void ) {
	LinkCombat();
}

/*
================
idAnimatedEntity::DisableCombat
================
*/
void idAnimatedEntity::DisableCombat( void ) {
	if ( combatModel ) {
		combatModel->Disable();
	}
}

/*
================
idAnimatedEntity::EnableCombat
================
*/
void idAnimatedEntity::EnableCombat( void ) {
	if ( combatModel ) {
		combatModel->Enable();
	}
}

/*
================
idAnimatedEntity::UnLinkCombat
================
*/
void idAnimatedEntity::UnLinkCombat( void ) {
	if( combatModel != NULL ) {
		combatModel->Unlink( gameLocal.clip );
	}
}

/*
================
idAnimatedEntity::GetCombatModel
================
*/
idClipModel *idAnimatedEntity::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
idAnimatedEntity::Event_ClearAllJoints

removes any custom transforms on all joints
================
*/
void idAnimatedEntity::Event_ClearAllJoints( void ) {
	animator.ClearAllJoints();
}

/*
================
idAnimatedEntity::Event_ClearJoint

removes any custom transforms on the specified joint
================
*/
void idAnimatedEntity::Event_ClearJoint( jointHandle_t jointnum ) {
	animator.ClearJoint( jointnum );
}

/*
================
idAnimatedEntity::Event_SetJointPos

modifies the position of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos ) {
	animator.SetJointPos( jointnum, transform_type, pos );
}

/*
================
idAnimatedEntity::Event_SetJointAngle

modifies the orientation of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles &angles ) {
	idMat3 mat;

	mat = angles.ToMat3();
	animator.SetJointAxis( jointnum, transform_type, mat );
}

/*
================
idAnimatedEntity::Event_GetJointPos

returns the position of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointPos( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		offset.Zero();
		gameLocal.Warning( "Joint # %d out of range on entity '%s'", jointnum, name.c_str() );
	}

	sdProgram::ReturnVector( offset );
}

/*
================
idAnimatedEntity::Event_GetJointPos

returns the position of the joint in model space
================
*/
void idAnimatedEntity::Event_GetLocalJointPos( jointHandle_t jointnum ) {
	idVec3 offset;

	if ( !animator.GetJointTransform( jointnum, gameLocal.time, offset ) ) {
		offset.Zero();
		gameLocal.Warning( "Joint # %d out of range on entity '%s'", jointnum, name.c_str() );
	}

	sdProgram::ReturnVector( offset );
}

/*
================
idAnimatedEntity::Event_GetJointAxis
================
*/
void idAnimatedEntity::Event_GetJointAxis( jointHandle_t jointnum, int index ) {
	idMat3 axis;
	if ( !GetWorldAxis( jointnum, axis ) ) {
		axis.Identity();
		gameLocal.Warning( "Joint # %d out of range on entity '%s'", jointnum, name.c_str() );
	}

	sdProgram::ReturnVector( axis[ index ] );
}

/*
================
idAnimatedEntity::Event_GetJointAngle

returns the orientation of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointAngle( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		axis.Identity();
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	idAngles ang = axis.ToAngles();
	idVec3 vec( ang[ 0 ], ang[ 1 ], ang[ 2 ] );
	sdProgram::ReturnVector( vec );
}

/*
================
idAnimatedEntity::Event_JointToWorldTransform
================
*/
void idAnimatedEntity::Event_JointToWorldTransform( jointHandle_t jointnum, const idVec3& vector ) {
	idMat3 axes;
	if ( !GetWorldAxis( jointnum, axes ) ) {
		sdProgram::ReturnVector( vector );
		return;
	}
	sdProgram::ReturnVector( axes * vector );
}

/*
================
idAnimatedEntity::Event_WorldToModelSpace
================
*/
void idAnimatedEntity::Event_WorldToModelSpace( const idVec3& vector ) {
	sdProgram::ReturnVector( vector * renderEntity.axis.Transpose() );
}

/*
================
idAnimatedEntity::Event_AnimLength
================
*/
void idAnimatedEntity::Event_AnimLength( const char* animName ) {
	int anim = animator.GetAnim( animName );

	if ( !anim ) {
		if( anim_showMissingAnims.GetBool() ) { 
			gameLocal.Warning( "idAnimatedEntity::Event_AnimLength missing '%s' animation on '%s' (%s)", animName, name.c_str(), GetEntityDefName() );
		}
		sdProgram::ReturnFloat( 0 );
	} else {		
		sdProgram::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
	}
}

/*
================
idAnimatedEntity::Event_AnimName
================
*/
void idAnimatedEntity::Event_AnimName( int index ) {
	const idAnim* anim = animator.GetAnim( index );
	if ( !anim ) {
		sdProgram::ReturnString( "" );
		return;
	}

	sdProgram::ReturnString( anim->Name() );
}

/*
================
idAnimatedEntity::Event_HideSurface
================
*/
void idAnimatedEntity::Event_HideSurface( int index ) {
	if ( index < 0 || index >= MAX_SURFACE_BITS ) {
		gameLocal.Warning( "idAnimatedEntity::Event_HideSurface Index Out of Range '%d'", index );
		return;
	}

	renderEntity.hideSurfaceMask.Set( index );
	UpdateVisuals();
	Present();
}

/*
================
idAnimatedEntity::Event_ShowSurface
================
*/
void idAnimatedEntity::Event_ShowSurface( int index ) {
	if ( index < 0 || index >= MAX_SURFACE_BITS ) {
		gameLocal.Warning( "idAnimatedEntity::Event_HideSurface Index Out of Range '%d'", index );
		return;
	}

	renderEntity.hideSurfaceMask.Clear( index );
	UpdateVisuals();
	Present();
}

/*
================
idAnimatedEntity::Event_GetSurfaceId
================
*/
void idAnimatedEntity::Event_GetSurfaceId( const char* surfaceName ) {
	if ( renderEntity.hModel == NULL ) {
		gameLocal.Warning( "idAnimatedEntity::Event_GetSurfaceId No Model Set" );
		sdProgram::ReturnInteger( -1 );
		return;
	}

	sdProgram::ReturnInteger( renderEntity.hModel->FindSurfaceId( surfaceName ) );
}

/*
================
idAnimatedEntity::Event_IsSurfaceHidden
================
*/
void idAnimatedEntity::Event_IsSurfaceHidden( int index ) {
	if ( index < 0 || index >= MAX_SURFACE_BITS ) {
		gameLocal.Warning( "idAnimatedEntity::Event_HideSurface Index Out of Range '%d'", index );
		sdProgram::ReturnBoolean( false );
		return;
	}

	sdProgram::ReturnBoolean( renderEntity.hideSurfaceMask.Get( index ) != 0 );
}

/*
================
idAnimatedEntity::Event_GetNumFrames
================
*/
void idAnimatedEntity::Event_GetNumFrames( const char* animName ) {
	int anim = animator.GetAnim( animName );
	sdProgram::ReturnInteger( animator.NumFrames( anim ) );
}
