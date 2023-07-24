// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Actor.h"
#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"
#include "Light.h"
#include "ContentMask.h"
#include "Moveable.h"
#include "Item.h"
#include "WorldSpawn.h"

/***********************************************************************

	idAnimState

***********************************************************************/

/*
=====================
idAnimState::idAnimState
=====================
*/
idAnimState::idAnimState() {
	self			= NULL;
	animator		= NULL;
	thread			= NULL;
	idleAnim		= true;
	disabled		= true;
	channel			= ANIMCHANNEL_ALL;
	animBlendFrames = 0;
	lastAnimBlendFrames = 0;
}

/*
=====================
idAnimState::~idAnimState
=====================
*/
idAnimState::~idAnimState() {
	Shutdown();
}

/*
=====================
idAnimState::Init
=====================
*/
void idAnimState::Init( idActor *owner, idAnimator *_animator, animChannel_t animchannel ) {
	assert( owner );
	assert( _animator );
	self = owner;
	animator = _animator;
	channel = animchannel;

	if ( thread == NULL ) {
		thread = gameLocal.program->CreateThread();
		thread->ManualDelete();
		thread->SetName( va( "%s_channel_%i", owner->GetName(), animchannel ) );
	}
	thread->EndThread();
	thread->ManualControl();
}

/*
=====================
idAnimState::Shutdown
=====================
*/
void idAnimState::Shutdown( void ) {
	if ( thread != NULL ) {
		gameLocal.program->FreeThread( thread );
		thread = NULL;
	}
}

/*
=====================
idAnimState::SetState
=====================
*/
void idAnimState::SetState( const char *statename, int blendFrames ) {
	const sdProgram::sdFunction* func = self->scriptObject->GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, self->scriptObject->GetTypeName() );
	}

	if ( ai_debugScript.GetInteger() == self->entityNumber || ai_debugAnimState.GetInteger() == self->entityNumber ) {
		gameLocal.Printf( "%d: %s: Animstate: %s -> %s\n", gameLocal.time, state.c_str(), self->name.c_str(), statename );
	}

	state = statename;
	disabled = false;
	animBlendFrames = blendFrames;
	lastAnimBlendFrames = blendFrames;

	animBlendFrames = blendFrames;
	lastAnimBlendFrames = blendFrames;
	disabled = false;
	idleAnim = false;

	thread->CallFunction( self->scriptObject, func );
}

/*
=====================
idAnimState::StopAnim
=====================
*/
void idAnimState::StopAnim( int frames ) {
	animBlendFrames = 0;
	animator->Clear( channel, gameLocal.time, FRAME2MS( frames ) );
}

/*
=====================
idAnimState::PlayAnim
=====================
*/
void idAnimState::PlayAnim( int anim ) {
	if ( anim ) {
		animator->PlayAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
	}
	animBlendFrames = 0;
}

/*
=====================
idAnimState::CycleAnim
=====================
*/
void idAnimState::CycleAnim( int anim ) {
	if ( anim ) {
		animator->CycleAnim( channel, anim, gameLocal.time, FRAME2MS( animBlendFrames ) );
	}
	animBlendFrames = 0;
}

/*
=====================
idAnimState::BecomeIdle
=====================
*/
void idAnimState::BecomeIdle( void ) {
	idleAnim = true;
}

/*
=====================
idAnimState::Disabled
=====================
*/
bool idAnimState::Disabled( void ) const {
	return disabled;
}

/*
=====================
idAnimState::AnimDone
=====================
*/
bool idAnimState::AnimDone( int blendFrames ) const {
	int animDoneTime;
	
	animDoneTime = animator->CurrentAnim( channel )->GetEndTime();
	if ( animDoneTime < 0 ) {
		// playing a cycle
		return false;
	} else if ( animDoneTime - FRAME2MS( blendFrames ) <= gameLocal.time ) {
		return true;
	} else {
		return false;
	}
}

/*
=====================
idAnimState::IsIdle
=====================
*/
bool idAnimState::IsIdle( void ) const {
	return disabled || idleAnim;
}

/*
=====================
idAnimState::GetAnimFlags
=====================
*/
animFlags_t idAnimState::GetAnimFlags( void ) const {
	animFlags_t flags;

	memset( &flags, 0, sizeof( flags ) );
	if ( !disabled && !AnimDone( 0 ) ) {
		flags = animator->GetAnimFlags( animator->CurrentAnim( channel )->AnimNum() );
	}

	return flags;
}

/*
=====================
idAnimState::Enable
=====================
*/
void idAnimState::Enable( int blendFrames ) {
	if ( disabled ) {
		disabled = false;
		animBlendFrames = blendFrames;
		lastAnimBlendFrames = blendFrames;
		if ( state.Length() ) {
			SetState( state.c_str(), blendFrames );
		}
	}
}

/*
=====================
idAnimState::Disable
=====================
*/
void idAnimState::Disable( void ) {
	disabled = true;
	idleAnim = false;
}

/*
=====================
idAnimState::UpdateState
=====================
*/
bool idAnimState::UpdateState( void ) {
	if ( disabled ) {
		return false;
	}

	thread->Execute();

	return true;
}

/***********************************************************************

	idActor

***********************************************************************/

extern const idEventDef EV_SetState;
extern const idEventDef EV_PlayCycle;
extern const idEventDef EV_SetAnimFrame;
extern const idEventDef EV_PlayAnim;

const idEventDef AI_StopAnim( "stopAnim", '\0', DOC_TEXT( "Stops the animation currently playing on the given channel over the specified number of frames." ), 2, NULL, "d", "channel", "Channel to stop animating on.", "d", "frames", "Number of frames to stop the animation over." );
const idEventDef AI_IdleAnim( "idleAnim", 'b', DOC_TEXT( "Attempts to cycle the specified animation on the given channel, and returns whether it succeeded or not." ), 2, "If the opposing channel is already in idle state with the same animation, both channels will start the animation, and the other channel will sync to the one specified.\nIf the opposing channel is cycling the same animation, this channel will sync to the other channel.", "d", "channel", "Channel to play the animation on.", "s", "anim", "Name of the animation to play." );
const idEventDef AI_SetBlendFrames( "setBlendFrames", '\0', DOC_TEXT( "Sets the number of frames to blend the next animation over, on the specified channel." ), 2, NULL, "d", "channel", "Channel to set the blend frames on.", "d", "count", "Number of blend frames to set." );
const idEventDef AI_GetBlendFrames( "getBlendFrames", 'd', DOC_TEXT( "Returns the number of blend frames that will be used for the next animation on the specified channel." ), 1, NULL, "d", "channel", "Channel to look up." );
const idEventDef AI_AnimState( "animState", '\0', DOC_TEXT( "Specifies the new state function to switch to for the given animation channel thread, as well as setting the next blend frames value." ), 3, "An error will be thrown if the state function does not exist.\nThis works in similar fashion to $event:setState$.", "d", "channel", "Channel to change the state of.", "s", "state", "State to change to.", "d", "fames", "Number of blend frames to set." );
const idEventDef AI_GetAnimState( "getAnimState", 's', DOC_TEXT( "Returns the last state function set on the given channel." ), 1, "See also $event:animState$.", "d", "channel", "Channel to look up." );
const idEventDef AI_InAnimState( "inAnimState", 'b', DOC_TEXT( "Returns whether the state for the specified channel is the same as the given state." ), 2, NULL, "d", "channel", "Channel to look up.", "s", "state", "State to compare." );
const idEventDef AI_AnimDone( "animDone", 'b', DOC_TEXT( "Returns whether any animations on the given channel will be finished within the specified number of frames." ), 2, NULL, "d", "channel", "Channel to look up.", "d", "frames", "Number of frames to check." );
const idEventDef AI_OverrideAnim( "overrideAnim", '\0', DOC_TEXT( "Syncs the specified channel to the opposing channel, and saves the original animation state for this channel." ), 1, NULL, "d", "channel", "Channel to sync." );
const idEventDef AI_EnableAnim( "enableAnim", '\0', DOC_TEXT( "Restores the old state of this channel after a call to $event:overrideAnim$." ), 2, NULL, "d", "channel", "Channel to restore the state of.", "d", "frames", "Number of frames to blend the animation back in over." );
const idEventDef AI_SetPrefix( "setPrefix", '\0', DOC_TEXT( "Sets the given prefix for a specified channel. " ), 3, "Prefixes are used to construct animation names.\nPrefix should be AP_WEAPON, AP_WEAPON_CLASS, AP_STANCE, AP_STANCE_ACTION, or AP_CHANNEL_NAME.", "d", "channel", "Channel to set the prefix on.", "d", "prefix", "Prefix to set.", "s", "value", "Value to set the prefix to." );
const idEventDef AI_HasAnim( "hasAnim", 'b', DOC_TEXT( "Performs an animation lookup for the specified channel, and returns whether an animation with the given name was found." ), 2, "This lookup will include using the channel's specific set of prefixes.", "d", "channel", "Channel to look up.", "s", "anim", "Name of the animation to look up." );
const idEventDef AI_GetState( "getState", 's', DOC_TEXT( "Returns the last state function set using $event:setState$." ), 0, NULL );
const idEventDef AI_SyncAnim( "syncAnim", '\0', DOC_TEXT( "Syncs the two specified channels, using the given number of blend frames." ), 3, NULL, "d", "from", "Channel to sync.", "d", "to", "Channel to sync to.", "d", "frames", "Number of frames to blend over." );
const idEventDef EV_SetState( "setState", '\0', DOC_TEXT( "Terminates the current execution path, and moves execution to a different function." ), 1, "The function must be on the same script object.\nAn error will be thrown if the function does not exist.", "s", "name", "Name of the function to switching execution to." );

CLASS_DECLARATION( idAnimatedEntity, idActor )
	EVENT( AI_SetPrefix,				idActor::Event_SetPrefix )
	EVENT( AI_StopAnim,					idActor::Event_StopAnim )
	EVENT( EV_PlayAnim,					idActor::Event_PlayAnim )
	EVENT( EV_SetAnimFrame,				idActor::Event_SetAnimFrame )
	EVENT( EV_PlayCycle,				idActor::Event_PlayCycle )
	EVENT( AI_IdleAnim,					idActor::Event_IdleAnim )
	EVENT( AI_SetBlendFrames,			idActor::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,			idActor::Event_GetBlendFrames )
	EVENT( AI_AnimState,				idActor::Event_AnimState )
	EVENT( AI_GetAnimState,				idActor::Event_GetAnimState )
	EVENT( AI_InAnimState,				idActor::Event_InAnimState )
	EVENT( AI_AnimDone,					idActor::Event_AnimDone )
	EVENT( AI_OverrideAnim,				idActor::Event_OverrideAnim )
	EVENT( AI_EnableAnim,				idActor::Event_EnableAnim )
	EVENT( AI_HasAnim,					idActor::Event_HasAnim )
	EVENT( EV_StopSound,				idActor::Event_StopSound )
	EVENT( EV_SetState,					idActor::Event_SetState )
	EVENT( AI_GetState,					idActor::Event_GetState )
	EVENT( AI_SyncAnim,					idActor::Event_SyncAnim )
END_CLASS

/*
=====================
idActor::idActor
=====================
*/
idActor::idActor( void ) {
	viewAxis.Identity();
	viewAxisOrientation.Identity();
	viewAxisOrientator = viewAxisOrientation[ 2 ];

	scriptThread		= NULL;		// initialized by ConstructScriptObject, which is called by idEntity::Spawn

	team				= NULL;
	eyeOffset.Zero();
	painDebounceTime	= 0;
	painDelay			= 0;

	state				= NULL;
	idealState			= NULL;

	soundJoint			= INVALID_JOINT;

	deltaViewAngles.Zero();
}

/*
=====================
idActor::~idActor
=====================
*/
idActor::~idActor( void ) {
	DeconstructScriptObject();

	RemoveCombatModel();

	ShutdownThreads();
}

/*
=====================
idActor::Spawn
=====================
*/
void idActor::Spawn( void ) {
	idStr			jointName;

	for( int i = 0; i < ANIM_NumAnimChannels; i++ ) {
		prefixes[ i ].AssureSize( AP_MAX );
	}
	

	state		= NULL;
	idealState	= NULL;

	viewAxis = GetPhysics()->GetAxis();

	painDebounceTime	= 0;
	painDelay			= SEC2MS( spawnArgs.GetFloat( "pain_delay" ) );

	// clear the bind anim
	animator.ClearAllAnims( gameLocal.time, 0 );

	if ( spawnArgs.GetString( "sound_bone", "", jointName ) ) {
		soundJoint = animator.GetJointHandle( jointName );
		if ( soundJoint == INVALID_JOINT ) {
			gameLocal.Warning( "idAnimated '%s' at (%s): cannot find joint '%s' for sound playback", name.c_str(), GetPhysics()->GetOrigin().ToString(0), jointName.c_str() );
		}
	}

	FinishSetup();
}

/*
================
idActor::FinishSetup
================
*/
void idActor::FinishSetup( void ) {
	// setup script object
	scriptObject = gameLocal.program->AllocScriptObject( this, spawnArgs.GetString( "scriptobject", "default" ) );
	ConstructScriptObject();

	SetupBody();
}

/*
================
idActor::Restart
================
*/
void idActor::Restart( void ) {
	FinishSetup();
}

/*
=====================
idActor::SetupBody
=====================
*/
void idActor::SetupBody( void ) {
	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.ClearAllJoints();

	torsoAnim.Init( this, &animator, ANIMCHANNEL_TORSO );
	legsAnim.Init( this, &animator, ANIMCHANNEL_LEGS );
}

/*
=====================
idActor::SetAxis
=====================
*/
void idActor::SetAxis( const idMat3& axis ) {
	viewAxis = axis;
	UpdateVisuals();
}

/*
=====================
idActor::SetPosition
=====================
*/
void idActor::SetPosition( const idVec3 &org, const idMat3 &axis ) {
	GetPhysics()->SetOrigin( org );
	viewAxis = axis;
	UpdateVisuals();
}

/*
================
idActor::GetPhysicsToVisualTransform
================
*/
bool idActor::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	origin = vec3_origin;
	axis = viewAxis;
	return true;
}

/*
================
idActor::GetPhysicsToSoundTransform
================
*/
bool idActor::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	if ( soundJoint != INVALID_JOINT ) {
		animator.GetJointTransform( soundJoint, gameLocal.time, origin, axis );
		axis = viewAxis;
	} else {
		origin = viewAxis * eyeOffset;
		axis.Identity();
	}
	return true;
}

/***********************************************************************

	script state management

***********************************************************************/

/*
================
idActor::ShutdownThreads
================
*/
void idActor::ShutdownThreads( void ) {
	torsoAnim.Shutdown();
	legsAnim.Shutdown();

	if ( scriptThread ) {
		gameLocal.program->FreeThread( scriptThread );
		scriptThread = NULL;
	}
}

/*
================
idActor::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idActor::ShouldConstructScriptObjectAtSpawn( void ) const {
	return false;
}

/*
================
idActor::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
sdProgramThread* idActor::ConstructScriptObject( void ) {
	if ( !scriptThread ) {
		// create script thread
		scriptThread = gameLocal.program->CreateThread();
		scriptThread->ManualDelete();
		scriptThread->ManualControl();
		scriptThread->SetName( name.c_str() );
	} else {
		scriptThread->EndThread();
	}
	
	// init the script object's data
	scriptObject->ClearObject();

	sdScriptHelper h1, h2;
	CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h1 );
	CallNonBlockingScriptEvent( scriptObject->GetSyncFunc(), h2 );

	// call script object's constructor
	const sdProgram::sdFunction* constructor = scriptObject->GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for entity '%s'", scriptObject->GetTypeName(), name.c_str() );
	}

	// just set the current function on the script.  we'll execute in the subclasses.
	scriptThread->CallFunction( scriptObject, constructor );

	return scriptThread;
}

/*
=====================
idActor::GetScriptFunction
=====================
*/
const sdProgram::sdFunction* idActor::GetScriptFunction( const char *funcname ) {
	const sdProgram::sdFunction* func = scriptObject->GetFunction( funcname );
	if ( !func ) {
		gameLocal.Error( "Unknown function '%s' in '%s'", funcname, scriptObject->GetTypeName() );
	}
	return func;
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const sdProgram::sdFunction* newState ) {
	if ( !newState ) {
		gameLocal.Error( "idActor::SetState: Null state" );
	}

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		gameLocal.Printf( "%d: %s: State: %s\n", gameLocal.time, name.c_str(), newState->GetName() );
	}

	state = newState;
	idealState = state;
	scriptThread->CallFunction( scriptObject, state );
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const char *statename ) {
	SetState( GetScriptFunction( statename ) );
}

/*
=====================
idActor::UpdateScript
=====================
*/
void idActor::UpdateScript( void ) {
	if ( gameLocal.IsPaused() ) {
		return;
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	int i;
	for ( i = 0; i < 20; i++ ) {
		if ( idealState != state ) {
			SetState( idealState );
		}

		// don't call script until it's done waiting
		if ( scriptThread->IsWaiting() ) {
			break;
		}
        
		scriptThread->Execute();
		if ( idealState == state ) {
			break;
		}
	}

	if ( i == 20 ) {
		scriptThread->Warning( "idActor::UpdateScript: exited loop to prevent lockup" );
	}
}

/***********************************************************************

	Model

***********************************************************************/

/*
================
idActor::SetCombatModel
================
*/
void idActor::SetCombatModel( void ) {
	gameLocal.clip.DeleteClipModel( combatModel );
	combatModel = new idClipModel( modelDefHandle );
}

/*
================
idActor::RemoveCombatModel
================
*/
void idActor::RemoveCombatModel( void ) {
	gameLocal.clip.DeleteClipModel( combatModel );
	combatModel = NULL;
}

/*
================
idActor::LinkCombat
================
*/
void idActor::LinkCombat( void ) {
	if ( fl.hidden ) { // || GetHealth() > 0 ) {
		return;
	}

	if ( combatModel != NULL ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
}

/*
================
idActor::UnLinkCombat
================
*/
void idActor::UnLinkCombat( void ) {
	if ( combatModel != NULL ) {
		combatModel->Unlink( gameLocal.clip );
	}
}

/*
================
idActor::EnableCombat
================
*/
void idActor::EnableCombat( void ) {
	if ( combatModel != NULL ) {
		combatModel->Enable();
	}
}

/*
================
idActor::DisableCombat
================
*/
void idActor::DisableCombat( void ) {
	if ( combatModel != NULL ) {
		combatModel->Disable();
	}
}

/*
================
idActor::UpdateAnimationControllers
================
*/
bool idActor::UpdateAnimationControllers( void ) {
	return false;
}

/*
================
idActor::Teleport
================
*/
void idActor::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	GetPhysics()->SetOrigin( origin + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	GetPhysics()->SetLinearVelocity( vec3_origin );

	viewAxis = angles.ToMat3();

	UpdateVisuals();

	if ( !IsHidden() ) {
		// kill anything at the new position
		gameLocal.KillBox( this );
	}
}

/*
================
idActor::GetDeltaViewAngles
================
*/
const idAngles &idActor::GetDeltaViewAngles( void ) const {
	return deltaViewAngles;
}

/*
================
idActor::SetDeltaViewAngles
================
*/
void idActor::SetDeltaViewAngles( const idAngles &delta ) {
	deltaViewAngles = delta;
}

/*
================
idActor::OnLadder
================
*/
bool idActor::OnLadder( void ) const {
	return false;
}

/***********************************************************************

	animation state

***********************************************************************/

/*
=====================
idActor::SetAnimState
=====================
*/
void idActor::SetAnimState( int channel, const char *statename, int blendFrames ) {
	const sdProgram::sdFunction* func = scriptObject->GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject->GetTypeName() );
	}

	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		torsoAnim.SetState( statename, blendFrames );
		legsAnim.Enable( blendFrames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.SetState( statename, blendFrames );
		torsoAnim.Enable( blendFrames );
		break;

	default:
		gameLocal.Error( "idActor::SetAnimState: Unknown anim group" );
		break;
	}
}

/*
=====================
idActor::GetAnimState
=====================
*/
const char *idActor::GetAnimState( int channel ) const {
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		return torsoAnim.state;
		break;

	case ANIMCHANNEL_LEGS :
		return legsAnim.state;
		break;

	default:
		gameLocal.Error( "idActor::GetAnimState: Unknown anim group" );
		return NULL;
		break;
	}
}

/*
=====================
idActor::InAnimState
=====================
*/
bool idActor::InAnimState( int channel, const char *statename ) const {
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		if ( torsoAnim.state == statename ) {
			return true;
		}
		break;

	case ANIMCHANNEL_LEGS :
		if ( legsAnim.state == statename ) {
			return true;
		}
		break;

	default:
		gameLocal.Error( "idActor::InAnimState: Unknown anim group" );
		break;
	}

	return false;
}

/*
=====================
idActor::UpdateAnimState
=====================
*/
void idActor::UpdateAnimState( void ) {
	torsoAnim.UpdateState();
	legsAnim.UpdateState();
}

/*
=====================
idActor::GetAnim
=====================
*/
idCVar g_debugAnimLookups( "g_debugAnimLookups", "-1", CVAR_GAME | CVAR_INTEGER, "prints successful animation lookups" );
int idActor::GetAnim( int channel, const char *animname ) {
	static unsigned int animCombinations[] = {
												APF_WEAPON			| APF_STANCE			| APF_CHANNEL_NAME,						// pliers_crouch_torso_<anim>
												APF_WEAPON			| APF_STANCE			| APF_STANCE_ACTION,					// pliers_crouch_forward_<anim>
												APF_WEAPON			| APF_STANCE_ACTION,											// pliers_forward_<anim>
												APF_WEAPON			| APF_STANCE,													// pliers_crouch_<anim>
												APF_WEAPON			| APF_CHANNEL_NAME,												// pliers_torso_<anim>
												
												APF_WEAPON_CLASS	| APF_STANCE			| APF_CHANNEL_NAME,						// pliers_crouch_torso_<anim>
												APF_WEAPON_CLASS	| APF_STANCE			| APF_STANCE_ACTION,					// pliers_crouch_forward_<anim>
												APF_WEAPON_CLASS	| APF_STANCE_ACTION,											// pliers_forward_<anim>
												APF_WEAPON_CLASS	| APF_STANCE,													// pliers_crouch_<anim>
												APF_WEAPON_CLASS	| APF_CHANNEL_NAME,												// pliers_torso_<anim>

												APF_WEAPON,																			// pliers_<anim>
												APF_WEAPON_CLASS,																	// tool_<anim>

												APF_CHANNEL_NAME,																	// torso_<anim>

												APF_STANCE			| APF_CHANNEL_NAME,												// crouch_torso_<anim>
												APF_STANCE			| APF_STANCE_ACTION,											// crouch_forward_<anim>
												APF_STANCE,																			// crouch_<anim>
												0																					// <anim>
	};

	static unsigned int numAnimCombinations = sizeof( animCombinations ) / sizeof( animCombinations[ 0 ] );

	int anim = 0;

	for( unsigned int i = 0; i < numAnimCombinations; i++ ) {
		AssembleAnimName( static_cast< animChannel_t >( channel ), animname, animCombinations[ i ] );
		anim = animator.GetAnim( completeAnim.c_str() );
		if( anim ) {
			if( g_debugAnimLookups.GetInteger() == entityNumber ) {
				gameLocal.Printf( "%s: Found anim '%s' for '%s'\n", name.c_str(), completeAnim.c_str(), animname );
			}
			break;
		}
	}
	return anim;
}


/*
============
idActor::AssembleAnimName
============
*/
void idActor::AssembleAnimName( animChannel_t channel, const char* action, unsigned int prefixToUse ) {
	completeAnim.Clear();
	
	const idStrList& prefixesLocal = prefixes[ channel ];
	if( ( prefixToUse & APF_WEAPON_CLASS ) && prefixesLocal[ AP_WEAPON_CLASS ].Length() ) {
		completeAnim.Append( prefixesLocal[ AP_WEAPON_CLASS ] );
		completeAnim.Append( '_' );
	}
	if( ( prefixToUse & APF_WEAPON ) && prefixesLocal[ AP_WEAPON ].Length() ) {
		completeAnim.Append( prefixesLocal[ AP_WEAPON ] );
		completeAnim.Append( '_' );
	}
	if( ( prefixToUse & APF_STANCE ) && prefixesLocal[ AP_STANCE ].Length() ) {
		completeAnim.Append( prefixesLocal[ AP_STANCE ] );
		completeAnim.Append( '_' );
	}
	if( ( prefixToUse & APF_STANCE_ACTION ) && prefixesLocal[ AP_STANCE_ACTION ].Length() ) {
		completeAnim.Append( prefixesLocal[ AP_STANCE_ACTION ] );
		completeAnim.Append( '_' );
	}
	if( ( prefixToUse & APF_CHANNEL_NAME ) && prefixesLocal[ AP_CHANNEL_NAME ].Length() ) {
		completeAnim.Append( prefixesLocal[ AP_CHANNEL_NAME ] );
		completeAnim.Append( '_' );
	}

	completeAnim.Append( action );
}

/*
===============
idActor::SyncAnimChannels
===============
*/
void idActor::SyncAnimChannels( animChannel_t channel, animChannel_t syncToChannel, int blendFrames ) {
	animator.SyncAnimChannels( channel, syncToChannel, gameLocal.time, FRAME2MS( blendFrames ) );
}

/***********************************************************************

	Damage

***********************************************************************/

/*
=====================
idActor::ClearPain
=====================
*/
void idActor::ClearPain( void ) {
	painDebounceTime = 0;
}

/*
=====================
idActor::Pain
=====================
*/
bool idActor::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
	if ( gameLocal.time < painDebounceTime ) {
		return false;
	}

	// don't play pain sounds more than necessary
	painDebounceTime = gameLocal.time + painDelay;

	int health = GetHealth();
	if ( health > 75  ) {
		PlayPain( "small" );
	} else if ( health > 50 ) {
		PlayPain( "medium" );
	} else if ( health > 25 ) {
		PlayPain( "large" );
	} else {
		PlayPain( "huge" );
	}

	return true;
}

/*
=====================
idActor::PlayPain
=====================
*/
void idActor::PlayPain( const char* strength ) {
	StartSound( va( "snd_pain_%s", strength ), SND_VOICE, 0, NULL );
}

/***********************************************************************

	Events

***********************************************************************/

/*
=====================
idActor::Event_SetPrefix
=====================
*/
void idActor::Event_SetPrefix( animChannel_t channel, ePrefixes prefix, const char *prefixValue ) {
	SetPrefix( channel, prefix, prefixValue );
}

/*
===============
idActor::Event_StopAnim
===============
*/
void idActor::Event_StopAnim( int channel, int frames ) {
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		torsoAnim.StopAnim( frames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.StopAnim( frames );
		break;

	default:
		gameLocal.Error( "Unknown anim group '%i'", channel );
		break;
	}
}


/*
============
idActor::AnimationMissing
============
*/
void idActor::AnimationMissing( animChannel_t channel, const char* animation ) {
	if( anim_showMissingAnims.GetBool() == false ) {
		return;
	}

	switch( channel ) {
		case ANIMCHANNEL_TORSO :
			gameLocal.Warning( "%d: %s : missing '%s' animation on '%s' (%s)", gameLocal.time, torsoAnim.state.c_str(), animation, name.c_str(), GetEntityDefName() );
			break;
		case ANIMCHANNEL_LEGS :
			gameLocal.Warning( "%d: %s : missing '%s' animation on '%s' (%s)", gameLocal.time, legsAnim.state.c_str(), animation, name.c_str(), GetEntityDefName() );
			break;
	}
}

/*
===============
idActor::Event_PlayAnim
===============
*/
void idActor::Event_PlayAnim( animChannel_t channel, const char *animname ) {
	int anim = GetAnim( channel, animname );

	if ( animator.IsCyclingAnim( channel, anim, gameLocal.time ) ) {
		sdProgram::ReturnFloat( 1.f );
		return;
	}

	if ( !anim ) {
		AnimationMissing( channel, animname );
		sdProgram::ReturnFloat( 0.f );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		torsoAnim.idleAnim = false;
		torsoAnim.PlayAnim( anim );
		if ( !torsoAnim.GetAnimFlags().prevent_idle_override && animator.IsPlayingAnimPrimary( ANIMCHANNEL_LEGS, anim, gameLocal.time ) ) {
			//if ( legsAnim.IsIdle() ) {
				legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			//}
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.idleAnim = false;
		legsAnim.PlayAnim( anim );
		if ( !legsAnim.GetAnimFlags().prevent_idle_override && animator.IsPlayingAnimPrimary( ANIMCHANNEL_TORSO, anim, gameLocal.time ) ) {
			//if ( torsoAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			//}
		}
		break;

	default :
		gameLocal.Error( "Unknown anim group" );
		break;
	}
	sdProgram::ReturnFloat( 1.f );
}

/*
===============
idActor::Event_PlayCycle
===============
*/
void idActor::Event_PlayCycle( animChannel_t channel, const char *animname ) {
	int anim = GetAnim( channel, animname );

	if ( animator.IsCyclingAnim( channel, anim, gameLocal.time ) ) {
		sdProgram::ReturnFloat( 1.f );
		return;
	}

	if ( !anim ) {
		AnimationMissing( channel, animname );
		sdProgram::ReturnFloat( 0.f );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		torsoAnim.idleAnim = false;
		torsoAnim.CycleAnim( anim );
		if ( !torsoAnim.GetAnimFlags().prevent_idle_override && animator.IsPlayingAnimPrimary( ANIMCHANNEL_LEGS, anim, gameLocal.time ) ) {
			//if ( legsAnim.IsIdle() ) {
				legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			//}
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.idleAnim = false;
		legsAnim.CycleAnim( anim );
		if ( !legsAnim.GetAnimFlags().prevent_idle_override && animator.IsPlayingAnimPrimary( ANIMCHANNEL_TORSO, anim, gameLocal.time ) ) {
			//if ( torsoAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			//}
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}

	sdProgram::ReturnFloat( 1.f );
}

/*
===============
idActor::Event_IdleAnim
===============
*/
void idActor::Event_IdleAnim( animChannel_t channel, const char *animname ) {
	int anim = GetAnim( channel, animname );

	if ( animator.IsCyclingAnim( channel, anim, gameLocal.time ) ) {
		sdProgram::ReturnBoolean( true );
		return;
	}

	if ( !anim ) {
		AnimationMissing( channel, animname );

		switch( channel ) {
		case ANIMCHANNEL_TORSO :
			torsoAnim.BecomeIdle();
			break;

		case ANIMCHANNEL_LEGS :
			legsAnim.BecomeIdle();
			break;

		default:
			gameLocal.Error( "Unknown anim group" );
		}

		sdProgram::ReturnBoolean( false );
		return;
	}

	
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		torsoAnim.BecomeIdle();
		if ( legsAnim.GetAnimFlags().prevent_idle_override || !animator.IsPlayingAnimPrimary( ANIMCHANNEL_LEGS, anim, gameLocal.time ) ) {
			// don't sync to legs if legs anim doesn't override idle anims
			torsoAnim.CycleAnim( anim );
		} else if ( legsAnim.IsIdle() ) {
			// play the anim in both legs and torso
			torsoAnim.CycleAnim( anim );
			legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		} else {
			// sync the anim to the legs
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, torsoAnim.animBlendFrames );
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.BecomeIdle();
		if ( torsoAnim.GetAnimFlags().prevent_idle_override || !animator.IsPlayingAnimPrimary( ANIMCHANNEL_TORSO, anim, gameLocal.time ) ) {
			// don't sync to torso if torso anim doesn't override idle anims
			legsAnim.CycleAnim( anim );
		} else if ( torsoAnim.IsIdle() ) {
			// play the anim in both legs and torso
			legsAnim.CycleAnim( anim );
			torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		} else {
			// sync the anim to the torso
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, legsAnim.animBlendFrames );
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}

	sdProgram::ReturnBoolean( true );
}

/*
===============
idActor::Event_SetAnimFrame
===============
*/
void idActor::Event_SetAnimFrame( const char *animname, animChannel_t channel, float frame ) {
	int anim = GetAnim( channel, animname );
	if ( !anim ) {
		AnimationMissing( channel, animname );

		switch( channel ) {
			case ANIMCHANNEL_TORSO :
				torsoAnim.BecomeIdle();
				break;

			case ANIMCHANNEL_LEGS :
				legsAnim.BecomeIdle();
				break;

			default:
				gameLocal.Error( "Unknown anim group" );
		}

		return;
	}

	animator.SetFrame( channel, anim, frame, gameLocal.time, 0 );
}

/*
===============
idActor::Event_OverrideAnim
===============
*/
void idActor::Event_OverrideAnim( int channel ) {
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		torsoAnim.Disable();
		SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.Disable();
		SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_EnableAnim
===============
*/
void idActor::Event_EnableAnim( int channel, int blendFrames ) {
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		torsoAnim.Enable( blendFrames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.Enable( blendFrames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_SetBlendFrames
===============
*/
void idActor::Event_SetBlendFrames( int channel, int blendFrames ) {
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		torsoAnim.animBlendFrames = blendFrames;
		torsoAnim.lastAnimBlendFrames = blendFrames;
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.animBlendFrames = blendFrames;
		legsAnim.lastAnimBlendFrames = blendFrames;
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_GetBlendFrames
===============
*/
void idActor::Event_GetBlendFrames( int channel ) {
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		sdProgram::ReturnInteger( torsoAnim.animBlendFrames );
		break;

	case ANIMCHANNEL_LEGS :
		sdProgram::ReturnInteger( legsAnim.animBlendFrames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::Event_AnimState
===============
*/
void idActor::Event_AnimState( int channel, const char *statename, int blendFrames ) {
	SetAnimState( channel, statename, blendFrames );
}

/*
===============
idActor::Event_GetAnimState
===============
*/
void idActor::Event_GetAnimState( int channel ) {
	sdProgram::ReturnString( GetAnimState( channel ) );
}

/*
===============
idActor::Event_InAnimState
===============
*/
void idActor::Event_InAnimState( int channel, const char *statename ) {
	sdProgram::ReturnBoolean( InAnimState( channel, statename ) );
}

/*
===============
idActor::Event_AnimDone
===============
*/
void idActor::Event_AnimDone( int channel, int blendFrames ) {
	switch( channel ) {
	case ANIMCHANNEL_TORSO :
		sdProgram::ReturnBoolean( torsoAnim.AnimDone( blendFrames ) );
		break;

	case ANIMCHANNEL_LEGS :
		sdProgram::ReturnBoolean( legsAnim.AnimDone( blendFrames ) );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}
}

/*
================
idActor::Event_HasAnim
================
*/
void idActor::Event_HasAnim( int channel, const char *animname ) {
	if ( GetAnim( channel, animname ) != 0 ) {
		sdProgram::ReturnBoolean( true );
	}
	sdProgram::ReturnBoolean( false );
}

/*
================
idActor::Event_StopSound
================
*/
void idActor::Event_StopSound( int channel ) {
	StopSound( channel );
}

/*
=====================
idActor::Event_SetState
=====================
*/
void idActor::Event_SetState( const char *name ) {
	idealState = GetScriptFunction( name );
	if ( idealState == state ) {
		state = NULL;
	}
	scriptThread->DoneProcessing();
}

/*
=====================
idActor::Event_GetState
=====================
*/
void idActor::Event_GetState( void ) {
	if ( state ) {
		sdProgram::ReturnString( state->GetName() );
	} else {
		sdProgram::ReturnString( "" );
	}
}

/*
=====================
idActor::SetPrefix
=====================
*/
void idActor::SetPrefix( animChannel_t channel, ePrefixes prefix, const char *prefixValue ) {
	if( prefix < 0 || prefix > AP_MAX ) {
		gameLocal.Error( "SetPrefix: Animation prefix '%i' out of bounds", prefix );
	}

	static const char* stanceNames[] = {
		"Weapon",
		"Weapon Class",
		"Stance",
		"Stance Action",
		"Channel Name"
	};

	bool changed = false;
	if( channel != ANIMCHANNEL_ALL ) {
		if( g_debugAnimStance.GetInteger() == entityNumber ) {
			changed = prefixes[ channel ][ prefix ].Icmp( prefixValue ) != 0;
		}
		prefixes[ channel ][ prefix ] = prefixValue;
	} else {
		for( int i = ANIMCHANNEL_ALL + 1; i < ANIM_NumAnimChannels; i++ ) {
			if( g_debugAnimStance.GetInteger() == entityNumber ) {
				changed |= prefixes[ channel ][ prefix ].Icmp( prefixValue ) != 0;
			}
			prefixes[ i ][ prefix ] = prefixValue;
		}
	}
		
	if( changed && g_debugAnimStance.GetInteger() == entityNumber ) {
		gameLocal.Printf( "%s, channel %i: set prefix '%s' to '%s'\n", name.c_str(), channel, stanceNames[ prefix ], prefixValue );
	}
}


/*
============
idActor::ReportCurrentState
============
*/
void idActor::ReportCurrentState_f( const idCmdArgs& args ) {
	if( args.Argc() < 2 ) {
		return;
	}

	int entityNum = atoi( args.Argv( 1 ));
	if( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		idEntity* entity = gameLocal.entities[ entityNum ];
	
		if( entity ) {
			if( idActor* actor = entity->Cast< idActor >() ) {
				gameLocal.Printf( "%s: Legs: '%s'   Torso: '%s'\n", actor->name.c_str(), actor->legsAnim.state.c_str(), actor->torsoAnim.state.c_str() );
			}
		}
	}	
}

/*
============
idActor::Event_SyncAnim
============
*/
void idActor::Event_SyncAnim( animChannel_t channel, animChannel_t syncToChannel, int blendFrames ) {
	SyncAnimChannels( channel, syncToChannel, blendFrames );
}
