/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "Objectives/MissionData.h"
#include "TimerManager.h"
#include "MeleeWeapon.h"
#include "AbsenceMarker.h"
#include "StdString.h"

// #include "logmgr.h"
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
	delete thread;
}

/*
=====================
idAnimState::Save
=====================
*/
void idAnimState::Save( idSaveGame *savefile ) const {

	savefile->WriteObject( self );

	// Save the entity owner of the animator
	savefile->WriteObject( animator->GetEntity() );

	savefile->WriteObject( thread );

	savefile->WriteString( state );

	savefile->WriteInt( animBlendFrames );
	savefile->WriteInt( lastAnimBlendFrames );
	savefile->WriteInt( channel );
	savefile->WriteBool( idleAnim );
	savefile->WriteBool( disabled );
}

/*
=====================
idAnimState::Restore
=====================
*/
void idAnimState::Restore( idRestoreGame *savefile ) {
	savefile->ReadObject( reinterpret_cast<idClass *&>( self ) );

	idEntity *animowner;
	savefile->ReadObject( reinterpret_cast<idClass *&>( animowner ) );
	if ( animowner ) {
		animator = animowner->GetAnimator();
	}

	savefile->ReadObject( reinterpret_cast<idClass *&>( thread ) );

	savefile->ReadString( state );

	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( lastAnimBlendFrames );
	savefile->ReadInt( channel );
	savefile->ReadBool( idleAnim );
	savefile->ReadBool( disabled );
}

/*
=====================
idAnimState::Init
=====================
*/
void idAnimState::Init( idActor *owner, idAnimator *_animator, int animchannel ) {
	assert( owner );
	assert( _animator );
	self = owner;
	animator = _animator;
	channel = animchannel;

	if ( !thread ) {
		thread = new idThread();
		thread->ManualDelete();
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
	delete thread;
	thread = NULL;
}

/*
=====================
idAnimState::SetState
=====================
*/
void idAnimState::SetState( const char *statename, int blendFrames ) {
	const function_t *func;

	func = self->scriptObject.GetFunction( statename );
	if ( !func ) {
		assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, self->scriptObject.GetTypeName() );
	}

	if (cv_ai_debug_anims.GetBool() && self != gameLocal.GetLocalPlayer())
	{
		gameLocal.Printf("Frame %d: New animstate %s (%s)\n", gameLocal.framenum, statename, self->name.c_str());
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Frame %d: New animstate %s (%s)\r", gameLocal.framenum, statename, self->name.c_str());
	}

	state = statename;
	disabled = false;
	animBlendFrames = blendFrames;
	lastAnimBlendFrames = blendFrames;
	thread->CallFunction( self, func, true );

	animBlendFrames = blendFrames;
	lastAnimBlendFrames = blendFrames;
	disabled = false;
	idleAnim = false;

	if ( ai_debugScript.GetInteger() == self->entityNumber ) {
		gameLocal.Printf( "%d: idAnimState::SetState channel %d %s\n", gameLocal.time, channel, state.c_str() ); // #4057
	}
}

/*
=====================
idAnimState::SetFrame
=====================
*/

void idAnimState::SetFrame( int anim, int frame )
{
	animator->SetFrame( channel, anim, frame, gameLocal.time, 0 );
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

void idAnimState::PauseAnim( int channel, bool bPause)
{
	animator->CurrentAnim( channel )->Pause( bPause );
}

bool idAnimState::AnimIsPaused( int channel )
{
	return animator->CurrentAnim( channel )->IsPaused();
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

idAnimator*	idAnimState::GetAnimator()
{
	return animator;
}

/*
=====================
idAnimState::Enable
=====================
*/
void idAnimState::Enable( int blendFrames ) 
{
	if ( disabled ) {
		disabled = false;
		animBlendFrames = blendFrames;
		lastAnimBlendFrames = blendFrames;
		if ( state.Length() ) {
			SetState( state.c_str(), blendFrames );
		}

		if ( ai_debugScript.GetInteger() == self->entityNumber ) // #4057
		{
			gameLocal.Printf( "%d: idAnimState::Enable channel %d\n", gameLocal.time, channel );
		}
	}
}

/*
=====================
idAnimState::Disable
=====================
*/
void idAnimState::Disable( void ) 
{
	if ( !disabled && ai_debugScript.GetInteger() == self->entityNumber ) // #4057
	{
		gameLocal.Printf( "%d: idAnimState::Disable channel %d\n", gameLocal.time, channel );
	}

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

	if ( ai_debugScript.GetInteger() == self->entityNumber ) {
		thread->EnableDebugInfo();
	} else {
		thread->DisableDebugInfo();
	}

	thread->Execute();

	return true;
}


/*
=====================
idAnimState::FinishAction
=====================
*/
void idAnimState::FinishAction(const idStr& actionname)
{
	if (waitState == actionname) {
		SetWaitState("");
	}
}

/*
=====================
idAnimState::SetWaitState
=====================
*/
void idAnimState::SetWaitState( const char *_waitstate )
{
	if ( ai_debugScript.GetInteger() == self->entityNumber ) // #4057
	{
		gameLocal.Printf( "%d: idAnimState::SetWaitState channel %d waitState \"%s\" --> \"%s\"\n", gameLocal.time, channel, waitState.c_str(), _waitstate );
	}
	
	waitState = _waitstate;
}

/*
=====================
idAnimState::WaitState
=====================
*/
const char *idAnimState::WaitState( void ) const 
{
	if ( waitState.Length() ) {
		return waitState;
	} else {
		return NULL;
	}
}


	


/***********************************************************************

	idActor

***********************************************************************/

const char *idActor::MeleeTypeNames[ NUM_MELEE_TYPES ] = {
	"Over", "LR", "RL", "Thrust", "General", "General"
};

const idEventDef AI_EnableEyeFocus( "enableEyeFocus", EventArgs(), EV_RETURNS_VOID, "Enables eye focus.");
const idEventDef AI_DisableEyeFocus( "disableEyeFocus", EventArgs(), EV_RETURNS_VOID, "Disables eye focus.");
const idEventDef EV_Footstep( "footstep", EventArgs(), EV_RETURNS_VOID, "Plays footstep sound.");
const idEventDef EV_FootstepLeft( "leftFoot", EventArgs(), EV_RETURNS_VOID, "Changes to left foot and plays footstep sound.");
const idEventDef EV_FootstepRight( "rightFoot", EventArgs(), EV_RETURNS_VOID, "Changes to right foot and plays footstep sound.");
const idEventDef EV_EnableWalkIK( "EnableWalkIK", EventArgs(), EV_RETURNS_VOID, "enables walk IK");
const idEventDef EV_DisableWalkIK( "DisableWalkIK", EventArgs(), EV_RETURNS_VOID, "disables walk IK");
const idEventDef EV_EnableLegIK( "EnableLegIK", EventArgs('d', "num", ""), EV_RETURNS_VOID, "enables leg IK");
const idEventDef EV_DisableLegIK( "DisableLegIK", EventArgs('d', "num", ""), EV_RETURNS_VOID, "disables leg IK");
const idEventDef AI_StopAnim( "stopAnim", EventArgs('d', "channel", "", 'd', "frames", ""), 
	EV_RETURNS_VOID, "Stops the animation currently playing on the given channel over the given number of frames.");
// NOTE: Id defines playanim here, but it is also overloaded in a roundabout way
// by idWeapon (maybe due to limited polymorphism in scripting?)
const idEventDef AI_PlayAnim( "playAnim", EventArgs('d', "channel", "", 's', "animName", ""), 'd', "Do not use, this is part of TDM's internal mechanics. Use playCustomAnim() on AI."
	"in scripts instead of this. Plays the given animation on the given channel. Returns false if anim doesn't exist.");
const idEventDef AI_PauseAnim( "pauseAnim", EventArgs('d', "channel", "", 'd', "bPause", "true = pause, false = unpause"), EV_RETURNS_VOID, 
	"Pause all animations playing on the given channel.\n" \
	"NOTE: Can also be used used by idWeapons");
const idEventDef AI_AnimIsPaused( "animIsPaused", EventArgs('d', "channel", ""), 'd', "Return whether the given anim channel is paused");
const idEventDef AI_PlayCycle( "playCycle", EventArgs('d', "channel", "", 's', "animName", ""), 'd', "Continuously repeats the given animation on the given channel.  Returns false if anim doesn't exist.");
const idEventDef AI_IdleAnim( "idleAnim", EventArgs('d', "channel", "", 's', "animName", ""), 'd', "Plays the given idle animation on the given channel.  Returns false if anim doesn't exist.");
const idEventDef AI_SetSyncedAnimWeight( "setSyncedAnimWeight", EventArgs('d', "channel", "", 'd', "animindex", "", 'f', "weight", ""), 
	EV_RETURNS_VOID, "Sets the blend amount on multi-point anims.");
const idEventDef AI_SyncAnimChannels("syncAnimChannels", EventArgs('d', "fromChannel", "", 'd', "toChannel", "", 'f', "blendFrames", ""), 
	EV_RETURNS_VOID, "Synchronises the channels");
const idEventDef AI_SetBlendFrames( "setBlendFrames", EventArgs('d', "channel", "", 'd', "blendFrame", ""), EV_RETURNS_VOID, "Sets the number of frames to blend between animations on the given channel.");
const idEventDef AI_GetBlendFrames( "getBlendFrames", EventArgs('d', "channel", ""), 'd', "Returns the number of frames to blend between animations on the given channel.");
const idEventDef AI_AnimState( "animState", EventArgs('d', "channel", "", 's', "stateFunction", "", 'd', "blendFrame", ""), 
	EV_RETURNS_VOID, "Sets a new animation state script function for the given channel.");
const idEventDef AI_GetAnimState( "getAnimState", EventArgs('d', "channel", ""), 's', "Returns the name of the current animation state script function used for the given channel.");
const idEventDef AI_InAnimState( "inAnimState", EventArgs('d', "channel", "", 's', "stateFunc", ""), 
	'd', "Returns true if the given animation state script function is currently used for the given channel.");
const idEventDef AI_FinishAction( "finishAction", EventArgs('s', "action", ""), EV_RETURNS_VOID, "Finishes the given wait action.");
const idEventDef AI_ReloadTorchReplacementAnims( "reloadTorchReplacementAnims", EventArgs(), EV_RETURNS_VOID, "If actor has a torch, reload the torch's replacement anims."); // grayman #3166
const idEventDef AI_FinishChannelAction( "finishChannelAction", EventArgs('d', "channel", "", 's', "animname", ""), 
	EV_RETURNS_VOID, "Overloaded finishAction function for setting the waitstate on each channel separately");
const idEventDef AI_AnimDone( "animDone", EventArgs('d', "channel", "", 'd', "blendOutFrames", ""), 'd', 
	"Returns true if the animation playing on the given channel\n" \
	"is completed considering a number of blend frames.");
const idEventDef AI_OverrideAnim( "overrideAnim", EventArgs('d', "channel", ""), EV_RETURNS_VOID, 
	"Disables the animation currently playing on the given channel and syncs\n" \
	"the animation with the animation of the nearest animating channel.");
const idEventDef AI_EnableAnim( "enableAnim", EventArgs('d', "channel", "", 'd', "blendFrames", ""), EV_RETURNS_VOID, "Enables animation on the given channel.");
const idEventDef AI_DisableAnimchannel( "disableAnimchannel", EventArgs('d', "channel", ""), EV_RETURNS_VOID, "Used to disable a certain animchannel (for example if the ai is dead)");
const idEventDef AI_PreventPain( "preventPain", EventArgs('f', "duration", "time in seconds"), EV_RETURNS_VOID, "Prevents any pain animation from being played for the given time in seconds.");
const idEventDef AI_DisablePain( "disablePain", EventArgs(), EV_RETURNS_VOID, "Disables pain animations.");
const idEventDef AI_EnablePain( "enablePain", EventArgs(), EV_RETURNS_VOID, "Enables pain animations.");
const idEventDef AI_GetPainAnim( "getPainAnim", EventArgs(), 's', "Returns the name of the pain animation.");
const idEventDef AI_SetAnimPrefix( "setAnimPrefix", EventArgs('s', "prefix", ""), EV_RETURNS_VOID, "Sets a string which is placed in front of any animation names.");
const idEventDef AI_HasAnim( "hasAnim", EventArgs('d', "channel", "", 's', "animName", ""), 'f', "Returns true when an entity has a specific animation.");
const idEventDef AI_CheckAnim( "checkAnim", EventArgs('d', "channel", "", 's', "animName", ""), EV_RETURNS_VOID, "Ensures that the animation exists and causes an error if it doesn't.");
const idEventDef AI_ChooseAnim( "chooseAnim", EventArgs('d', "channel", "", 's', "animName", ""), 's', "Chooses a random anim and returns the name. Useful for doing move tests on anims.");
const idEventDef AI_AnimLength( "animLength", EventArgs('d', "channel", "", 's', "animName", ""), 'f', 
	"Returns the length of the anim in seconds. If the entity has multiple anims with animName,\n" \
	"length may not match the anim that is played. Use chooseAnim to get a non-random anim\n" \
	"and pass that string into animLength.");
const idEventDef AI_AnimDistance( "animDistance", EventArgs('d', "channel", "", 's', "animName", ""), 'f', 
	"Returns the distance that the anim travels. If the entity has multiple anims with animName,\n" \
	"the distance may not match the anim that is played. Use chooseAnim to get a non-random anim\n" \
	"and pass that string into animDistance.");

const idEventDef AI_HasEnemies( "hasEnemies", EventArgs(), 'd', "Returns true if the actor has one or more enemies.");
const idEventDef AI_NextEnemy( "nextEnemy", EventArgs('E', "lastEnemy", ""), 'e', "Returns the next enemy the actor has acquired.");
const idEventDef AI_ClosestEnemyToPoint( "closestEnemyToPoint", EventArgs('v', "point", ""), 'e', "Returns the enemy closest to the given location.");
const idEventDef AI_SetNextState( "setNextState", EventArgs('s', "stateFunc", ""), EV_RETURNS_VOID, 
	"Sets the next state and waits until thread exits, or a frame delay before calling it.\n" \
	"Handy for setting the state in the constructor.");
const idEventDef AI_SetState( "setState", EventArgs('s', "stateFunc", ""), EV_RETURNS_VOID, "Sets the next state and goes to it immediately");
const idEventDef AI_GetState( "getState", EventArgs(), 's', "Gets the current state.");
const idEventDef AI_GetHead( "getHead", EventArgs(), 'e', "Returns the entity used for the character's head, if it has one.");
const idEventDef AI_GetEyePos( "getEyePos", EventArgs(), 'v', "Get eye position of the player and the AI");

// greebo: Moved these from ai_events.cpp to here
const idEventDef AI_SetHealth( "setHealth", EventArgs('f', "health", ""), EV_RETURNS_VOID, "Use these to set the health of AI or players (this also updates the AI_DEAD flag)");
const idEventDef AI_GetHealth( "getHealth", EventArgs(), 'f', "Returns the current health.");

const idEventDef AI_SetMaxHealth( "setMaxHealth", EventArgs('f', "maxHealth", ""), EV_RETURNS_VOID, "Use these to set the max health of AI or players. Note that the health bar GUI is limited to showing a max health of 100. If health exceeds max health after this, health will be set to be equal to max health (which would also update the AI_DEAD flag)");

// Attachment Related Events:
const idEventDef AI_Attach( "attach", EventArgs('e', "ent", "", 's', "attName", "the desired name of the attachment, e.g., 'melee_weapon'"), EV_RETURNS_VOID, 
	"Attach an entity to the AI.\n" \
	"Entity spawnArgs checked for attachments are:\n" \
	" - \"origin\", \"angles\", and \"joint\".\n" \
	"These must be set prior to calling attach.");
const idEventDef AI_AttachToPos( "attachToPos", EventArgs('e', "ent", "", 's', "position", "", 's', "attName", "the desired name of the attachment, e.g., 'melee_weapon'"), EV_RETURNS_VOID, 
	"Attach an entity to the AI, using a named attachment position");
const idEventDef AI_ReAttachToPos( "reAttachToPos", 
	EventArgs('s', "attName", "the name of the attachment we want to reattach.", 
			  's', "position", "the new position we want to attach it to."), 
	EV_RETURNS_VOID, 
	"Reattach an existing attachment to a named attachment position.\n" \
	"Example: reAttachToPos( melee_weapon, right_hand_held )");
const idEventDef AI_ReAttachToCoords( "reAttachToCoords", 
	EventArgs('s', "attName", "the name of the attachment we want to reattach. ", 
			  's', "joint", "the name of the joint to attach to", 
			  'v', "offset", "the translation offset from joint", 
			  'v', "angles", "a (pitch, yaw, roll) angle vector that defines the rotation \n" \
							"of the attachment relative to the joint's orientation"), 
	EV_RETURNS_VOID, 
	"Reattach an existing attachment");
const idEventDef AI_DropAttachment( "dropAttachment", EventArgs('s', "attName", ""), EV_RETURNS_VOID, "Drop the attachment for the given attachment name.");
const idEventDef AI_ShowAttachment( "showAttachment", EventArgs('s', "attName", "", 'd', "show", "1 shows attachment, 0 hides it."),
	EV_RETURNS_VOID, "Show or hide an attachment.");
const idEventDef AI_DropAttachmentInd( "dropAttachmentInd", EventArgs('d', "index", "starts at 0"), EV_RETURNS_VOID, "Drop the attachment for the given index.");
const idEventDef AI_ShowAttachmentInd( "showAttachmentInd", EventArgs('d', "index", "starts at 0", 'd', "show", "1 shows attachment, 0 hides it."), EV_RETURNS_VOID, "Show or hide an attachment by array index.");
const idEventDef AI_GetAttachment( "getAttachment", EventArgs('s', "attName", ""), 'e', 
	"Get the attached entity with the given attachment name\n" \
	"Will be NULL if the name is invalid or if the entity no longer exists");
const idEventDef AI_GetAttachmentInd( "getAttachmentInd", EventArgs('d', "index", "starts at 0"), 'e', 
	"Get the attached entity at the given index.\n" \
	"Will be NULL if the index is invalid or the entity no longer exists");
const idEventDef AI_GetNumAttachments( "getNumAttachments", EventArgs(), 'd', 
	"Return the number of attachments on an AI.\n" \
	"Used to iterate through the attachments if desired.");
// Weapon attachment related events
const idEventDef AI_GetNumRangedWeapons( "getNumRangedWeapons", EventArgs(), 'd', 
	"Returns the number of ranged weapons attached to an actor.\n" \
	"This refers to the TDM-style attachments with the spawnarg\n" \
	"'is_weapon_ranged' set to '1'.");
const idEventDef AI_GetNumMeleeWeapons( "getNumMeleeWeapons", EventArgs(), 'd', 
	"Returns the number of melee weapons attached to an actor.\n" \
	"This refers to the TDM-style attachments with the spawnarg\n" \
	"'is_weapon_melee' set to '1'.");

// Getting/setting attack flags
const idEventDef AI_GetAttackFlag( "getAttackFlag", EventArgs('d', "combatType", "see tdm_defs.script for possible enum values"), 'd', 
	"Returns 1 if the given attack flag is activated.");
const idEventDef AI_SetAttackFlag( "setAttackFlag", EventArgs('d', "combatType", "see tdm_defs.script for possible enum values", 'd', "enabled", ""), 
	EV_RETURNS_VOID, "Activate or deactivate the given attack flag");

// melee combat events
const idEventDef AI_MeleeAttackStarted( "meleeAttackStarted", EventArgs('d', "attType", ""), EV_RETURNS_VOID, 
	"Registers the start of a given melee attack\n" \
	"Intended to be called from a script that also starts the animation");
const idEventDef AI_MeleeParryStarted( "meleeParryStarted", EventArgs('d', "parType", ""), EV_RETURNS_VOID, 
	"Registers the start of a given melee parry\n" \
	"Intended to be called from a script that also starts the animation");
const idEventDef AI_MeleeActionHeld( "meleeActionHeld", EventArgs(), EV_RETURNS_VOID, "Called when the melee action reaches the \"hold\" point.");
const idEventDef AI_MeleeActionReleased( "meleeActionReleased", EventArgs(), EV_RETURNS_VOID, "Called when the melee action is released from the hold point.");
const idEventDef AI_MeleeActionFinished( "meleeActionFinished", EventArgs(), EV_RETURNS_VOID, "Called when the animation for the melee action has finished.");
const idEventDef AI_GetMeleeActionState( "getMeleeActState", EventArgs(), 'd', "Returns the current melee action state (attacking/defending).");
const idEventDef AI_GetMeleeActionPhase( "getMeleeActPhase", EventArgs(), 'd', "Returns the current melee action phase (holding,recovering,etc).");
const idEventDef AI_GetMeleeActionType( "getMeleeActType", EventArgs(), 'd', "Returns the current melee action type (overhead,thrust,etc.).");
const idEventDef AI_GetMeleeLastActTime( "getMeleeLastActTime", EventArgs(), 'd', 
	"Returns the melee type of the last attack to hit this actor.\n" \
	"Defaults to MELEETYPE_UNBLOCKABLE if we were not hit before.");
const idEventDef AI_GetMeleeResult( "getMeleeResult", EventArgs(), 'd', 
	"Get the result of the last melee action\n" \
	"Follows MELEERESULT_* enum defined in tdm_defs.script");
const idEventDef AI_GetMeleeLastHitByType( "getMeleeLastHitByType", EventArgs(), 'd', "Returns the game time that the most recent melee action ended (in ms)");
const idEventDef AI_MeleeBestParry( "meleeBestParry", EventArgs(), 'd', 
	"Returns the melee type integer of the best parry given the enemy attacks at the time\n" \
	"If no attacking enemy is found, returns default of MELEETYPE_RL\n" \
	"Follows MELEETYPE_* enum defined in tdm_defs.script");
const idEventDef AI_MeleeNameForNum( "meleeNameForNum", EventArgs('d', "num", ""), 's', 
	"Converts a melee type integer to a string name suffix\n" \
	"Used for finding the right animation for a given type, etc.\n" \
	"Possible names are, in order: \"Over, LR, RL, Thrust, General\"\n" \
	"Where general blocks all attacks types except unblockable");

// greebo: anim replacement script events
const idEventDef AI_SetReplacementAnim( "setReplacementAnim", EventArgs('s', "animToReplace", "", 's', "replacement", ""), EV_RETURNS_VOID, 
	"Replaces the animation \"animToReplace\" with \"replacement\"");
const idEventDef AI_LookupReplacementAnim( "lookupReplacementAnim", EventArgs('s', "anim", ""), 's', 
	"Returns the current replacement animation for \"anim\". Returns empty if no replacement anim ");
const idEventDef AI_RemoveReplacementAnim( "removeReplacementAnim", EventArgs('s', "anim", ""), EV_RETURNS_VOID, 
	"Removes the replacement for the given \"anim\"");
// Obsttorte: #0540
const idEventDef EV_getAnimRate("getAnimRate", EventArgs('d',"channel","",'s', "animName", "The name of the animation."),
	'f', "Returns the rate for the given animation. Returns -1 if animation cannot be found.");
const idEventDef EV_setAnimRate("setAnimRate", EventArgs('d',"channel","",'s', "animName", "The name of the animation.", 'f', "animRate", "The rate to set the animation to."),
	'd', "Sets the animation rate to the given value. Returns 1 if successful, otherwise -1");
const idEventDef EV_getAnimList("getAnimList", EventArgs('d',"channel",""), 's', "Returns a list of all animations and their anim rates.");


CLASS_DECLARATION( idAFEntity_Gibbable, idActor )
	EVENT( AI_EnableEyeFocus,			idActor::Event_EnableEyeFocus )
	EVENT( AI_DisableEyeFocus,			idActor::Event_DisableEyeFocus )
	EVENT( EV_Footstep,					idActor::Event_Footstep )
	EVENT( EV_FootstepLeft,				idActor::Event_Footstep )
	EVENT( EV_FootstepRight,			idActor::Event_Footstep )
	EVENT( EV_EnableWalkIK,				idActor::Event_EnableWalkIK )
	EVENT( EV_DisableWalkIK,			idActor::Event_DisableWalkIK )
	EVENT( EV_EnableLegIK,				idActor::Event_EnableLegIK )
	EVENT( EV_DisableLegIK,				idActor::Event_DisableLegIK )
	EVENT( AI_PreventPain,				idActor::Event_PreventPain )
	EVENT( AI_DisablePain,				idActor::Event_DisablePain )
	EVENT( AI_EnablePain,				idActor::Event_EnablePain )
	EVENT( AI_GetPainAnim,				idActor::Event_GetPainAnim )
	EVENT( AI_SetAnimPrefix,			idActor::Event_SetAnimPrefix )
	EVENT( AI_StopAnim,					idActor::Event_StopAnim )
	EVENT( AI_PlayAnim,					idActor::Event_PlayAnim )
	EVENT( AI_PauseAnim,				idActor::Event_PauseAnim )
	EVENT( AI_AnimIsPaused,				idActor::Event_AnimIsPaused )
	EVENT( AI_PlayCycle,				idActor::Event_PlayCycle )
	EVENT( AI_IdleAnim,					idActor::Event_IdleAnim )
	EVENT( AI_SetSyncedAnimWeight,		idActor::Event_SetSyncedAnimWeight )
	EVENT( AI_SyncAnimChannels,			idActor::Event_SyncAnimChannels)
	EVENT( AI_SetBlendFrames,			idActor::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,			idActor::Event_GetBlendFrames )
	EVENT( AI_AnimState,				idActor::Event_AnimState )
	EVENT( AI_GetAnimState,				idActor::Event_GetAnimState )
	EVENT( AI_InAnimState,				idActor::Event_InAnimState )
	EVENT( AI_FinishAction,				idActor::Event_FinishAction )
	EVENT( AI_ReloadTorchReplacementAnims, idActor::Event_ReloadTorchReplacementAnims) // grayman #3166
	EVENT( AI_FinishChannelAction,		idActor::Event_FinishChannelAction )
	EVENT( AI_AnimDone,					idActor::Event_AnimDone )
	EVENT( AI_OverrideAnim,				idActor::Event_OverrideAnim )
	EVENT( AI_EnableAnim,				idActor::Event_EnableAnim )
	EVENT( AI_DisableAnimchannel,		idActor::Event_DisableAnimchannel )
	EVENT( AI_HasAnim,					idActor::Event_HasAnim )
	EVENT( AI_CheckAnim,				idActor::Event_CheckAnim )
	EVENT( AI_ChooseAnim,				idActor::Event_ChooseAnim )
	EVENT( AI_AnimLength,				idActor::Event_AnimLength )
	EVENT( AI_AnimDistance,				idActor::Event_AnimDistance )
	EVENT( AI_HasEnemies,				idActor::Event_HasEnemies )
	EVENT( AI_NextEnemy,				idActor::Event_NextEnemy )
	EVENT( AI_ClosestEnemyToPoint,		idActor::Event_ClosestEnemyToPoint )
	EVENT( AI_MeleeAttackStarted,		idActor::Event_MeleeAttackStarted )
	EVENT( AI_MeleeParryStarted,		idActor::Event_MeleeParryStarted )
	EVENT( AI_MeleeActionHeld,			idActor::Event_MeleeActionHeld )
	EVENT( AI_MeleeActionReleased,		idActor::Event_MeleeActionReleased )
	EVENT( AI_MeleeActionFinished,		idActor::Event_MeleeActionFinished )
	EVENT( AI_GetMeleeActionState,		idActor::Event_GetMeleeActionState )
	EVENT( AI_GetMeleeActionPhase,		idActor::Event_GetMeleeActionPhase )
	EVENT( AI_GetMeleeActionType,		idActor::Event_GetMeleeActionType )
	EVENT( AI_GetMeleeLastActTime,		idActor::Event_GetMeleeLastActTime )
	EVENT( AI_GetMeleeResult,			idActor::Event_GetMeleeResult )
	EVENT( AI_GetMeleeLastHitByType,	idActor::Event_GetMeleeLastHitByType )
	EVENT( AI_MeleeBestParry,			idActor::Event_MeleeBestParry )
	EVENT( AI_MeleeNameForNum,			idActor::Event_MeleeNameForNum )
	EVENT( EV_StopSound,				idActor::Event_StopSound )
	EVENT( AI_SetNextState,				idActor::Event_SetNextState )
	EVENT( AI_SetState,					idActor::Event_SetState )
	EVENT( AI_GetState,					idActor::Event_GetState )
	EVENT( AI_GetHead,					idActor::Event_GetHead )
	EVENT( AI_GetEyePos,				idActor::Event_GetEyePos )
	EVENT( AI_SetHealth,				idActor::Event_SetHealth )
	EVENT( AI_GetHealth,				idActor::Event_GetHealth )
	EVENT( AI_SetMaxHealth,				idActor::Event_SetMaxHealth )

	// Obsttorte: #0540
	EVENT(EV_getAnimRate,				idActor::Event_getAnimRate)
	EVENT(EV_setAnimRate,				idActor::Event_setAnimRate)
	EVENT(EV_getAnimList,				idActor::Event_getAnimList)

	EVENT ( AI_Attach,					idActor::Event_Attach )
	EVENT ( AI_AttachToPos,				idActor::Event_AttachToPos )
	EVENT ( AI_ReAttachToPos,			idActor::ReAttachToPos )
	EVENT ( AI_ReAttachToCoords,		idActor::ReAttachToCoords )
	EVENT ( AI_DropAttachment,			idActor::Detach )
	EVENT ( AI_ShowAttachment,			idActor::ShowAttachment )
	EVENT ( AI_DropAttachmentInd,		idActor::DetachInd )
	EVENT ( AI_ShowAttachmentInd,		idActor::ShowAttachmentInd )
	EVENT ( AI_GetAttachment,			idActor::Event_GetAttachment )
	EVENT ( AI_GetAttachmentInd,		idActor::Event_GetAttachmentInd )
	EVENT ( AI_GetNumAttachments,		idActor::Event_GetNumAttachments )
	EVENT ( AI_GetNumRangedWeapons,		idActor::Event_GetNumRangedWeapons )
	EVENT ( AI_GetNumMeleeWeapons,		idActor::Event_GetNumMeleeWeapons )

	EVENT ( AI_SetReplacementAnim,		idActor::Event_SetReplacementAnim )
	EVENT ( AI_LookupReplacementAnim,	idActor::Event_LookupReplacementAnim )
	EVENT ( AI_RemoveReplacementAnim,	idActor::Event_RemoveReplacementAnim )

	EVENT ( AI_GetAttackFlag,			idActor::Event_GetAttackFlag )
	EVENT ( AI_SetAttackFlag,			idActor::Event_SetAttackFlag )
	
END_CLASS

/*
=====================
idActor::idActor
=====================
*/
idActor::idActor( void ) {
	viewAxis.Identity();

	scriptThread		= NULL;		// initialized by ConstructScriptObject, which is called by idEntity::Spawn

	use_combat_bbox		= false;
	head				= NULL;

	m_AItype			= 0;
	m_Innocent			= false;
	rank				= 0;

	greetingState		= ECannotGreet;

	m_fovDotHoriz		= 0.0f;
	m_fovDotVert		= 0.0f;
	m_SneakAttackMult	= 1.0f;
	m_SneakAttackThresh	= idMath::INFINITY;
	eyeOffset.Zero();
	pain_debounce_time	= 0;
	pain_delay			= 0;
	pain_threshold		= 0;

	melee_range_unarmed = 0.0f;
	melee_range			= 0.0f;
	melee_range_vert	= 0.0f;	// grayman #2655
	m_MeleePredictedAttTime				= 0.0f;
	m_MeleePredictedAttTimeUnarmed		= 0.0f;
	m_MeleeDamageMult					= 1.0f;
	m_MeleeHoldTimeMin					= 0;
	m_MeleeHoldTimeMax					= 0;
	m_MeleeCurrentHoldTime				= 0;
	m_MeleeAttackRecoveryMin			= 0;
	m_MeleeAttackRecoveryMax			= 0;
	m_MeleeCurrentAttackRecovery		= 0;
	m_MeleeAttackLongRecoveryMin		= 0;
	m_MeleeAttackLongRecoveryMax		= 0;
	m_MeleeCurrentAttackLongRecovery	= 0;
	m_MeleeParryRecoveryMin				= 0;
	m_MeleeParryRecoveryMax				= 0;
	m_MeleeCurrentParryRecovery			= 0;
	m_MeleeRiposteRecoveryMin			= 0;
	m_MeleeRiposteRecoveryMax			= 0;
	m_MeleeCurrentRiposteRecovery		= 0;
	m_MeleePreParryDelayMin				= 0;
	m_MeleePreParryDelayMax				= 0;
	m_MeleeCurrentPreParryDelay			= 0;
	m_MeleeRepeatedPreParryDelayMin		= 0;
	m_MeleeRepeatedPreParryDelayMax		= 0;
	m_MeleeCurrentRepeatedPreParryDelay	= 0;
	m_MeleeNumRepAttacks				= 0;
	m_MeleeRepAttackTime				= 0;
	m_MeleePostParryDelayMin			= 0;
	m_MeleePostParryDelayMax			= 0;
	m_MeleeCurrentPostParryDelay		= 0;
	m_MeleeRepeatedPostParryDelayMin	= 0;
	m_MeleeRepeatedPostParryDelayMax	= 0;
	m_MeleeCurrentRepeatedPostParryDelay	= 0;

	state				= NULL;
	idealState			= NULL;

	leftEyeJoint		= INVALID_JOINT;
	rightEyeJoint		= INVALID_JOINT;
	soundJoint			= INVALID_JOINT;

	mHeadModelOffset.Zero();
	modelOffset.Zero();
	deltaViewAngles.Zero();

	painTime			= 0;
	allowPain			= false;
	allowEyeFocus		= false;

	waitState			= "";
	
	blink_anim			= 0;
	blink_time			= 0;
	blink_min			= 0;
	blink_max			= 0;

	finalBoss			= false;

	m_timeFellDown		= 0;	 // grayman #3317

	m_isMute			= false; // grayman #3202

	m_MouthOffset		= vec3_zero; // grayman #1104

	m_EyeOffset			= vec3_zero; // grayman #3525

	m_knownSuspiciousEvents.Clear(); // grayman #3857

	m_warningEvents.Clear(); // grayman #3424

	m_victorHasKnealt = false; // grayman #3848

	m_Attachments.SetGranularity( 1 );

	enemyNode.SetOwner( this );
	enemyList.SetOwner( this );

	INIT_TIMER_HANDLE(actorGetObstaclesTimer);
	INIT_TIMER_HANDLE(actorGetPointOutsideObstaclesTimer);
	INIT_TIMER_HANDLE(actorGetWallEdgesTimer);
	INIT_TIMER_HANDLE(actorSortWallEdgesTimer);
	INIT_TIMER_HANDLE(actorBuildPathTreeTimer);
	INIT_TIMER_HANDLE(actorPrunePathTreeTimer);
	INIT_TIMER_HANDLE(actorFindOptimalPathTimer);
	INIT_TIMER_HANDLE(actorRouteToGoalTimer);
	INIT_TIMER_HANDLE(actorSubSampleWalkPathTimer);
	INIT_TIMER_HANDLE(actorWalkPathValidTimer);

}

/*
=====================
idActor::~idActor
=====================
*/
idActor::~idActor( void ) {
	int i;
	idEntity *ent;

	DeconstructScriptObject();
	scriptObject.Free();

	StopSound( SND_CHANNEL_ANY, false );

	delete combatModel;
	combatModel = NULL;

	if ( head.GetEntity() ) {
		head.GetEntity()->ClearBody();
		head.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}

	// remove any attached entities
	for( i = 0; i < m_Attachments.Num(); i++ ) {
		ent = m_Attachments[ i ].ent.GetEntity();
		if ( ent ) {
			ent->PostEventMS( &EV_SafeRemove, 0 );
		}
	}

	ShutdownThreads();
}

/*
=====================
idActor::Spawn
=====================
*/
void idActor::Spawn( void ) 
{
	idStr			jointName;
	float			fovDegHoriz, fovDegVert;
	copyJoints_t	copyJoint;
	const idKeyValue *kv = NULL;

	animPrefix	= "";
	state		= NULL;
	idealState	= NULL;

	spawnArgs.GetFloat( "collision_damage_threshold_hard", "25", m_damage_thresh_hard ); // greebo: dealing 50+ hit points is considered "hard"
	spawnArgs.GetFloat( "collision_damage_threshold_min", "5", m_damage_thresh_min ); // falling ~12 ft, g = 1066

	m_savedVelocity = vec3_zero; // grayman #3699

	spawnArgs.GetFloat( "collision_delta_scale",  "1.0", m_delta_scale );
	spawnArgs.GetInt( "rank", "0", rank );
	spawnArgs.GetInt( "type", "0", m_AItype );
	spawnArgs.GetBool( "innocent", "0", m_Innocent );
	spawnArgs.GetVector("offsetModel", "0 0 0", modelOffset);

	spawnArgs.GetBool( "use_combat_bbox", "0", use_combat_bbox );	

	viewAxis = GetPhysics()->GetAxis();

	spawnArgs.GetFloat( "fov", "150", fovDegHoriz );
	// If fov_vert is -1, it will be set the same as horizontal
	spawnArgs.GetFloat( "fov_vert", "-1", fovDegVert );
	SetFOV( fovDegHoriz, fovDegVert );

	pain_debounce_time	= 0;

	pain_delay		= SEC2MS( spawnArgs.GetFloat( "pain_delay" ) );
	pain_threshold	= spawnArgs.GetInt( "pain_threshold" );

	// load melee settings based on AI skill level + player difficulty
	LoadMeleeSet();
	// adjust health based on multiplier that may be in melee set
	health += spawnArgs.GetFloat("health_offset", "0.0");
	
	melee_range_unarmed					= spawnArgs.GetFloat( "melee_range","64");
	melee_range							= melee_range_unarmed;

	// grayman #2655 - if "melee_range_vert" has a value, use that,
	// otherwise set it to melee_range

	melee_range_vert					= spawnArgs.GetFloat( "melee_range_vert","0");
	if (melee_range_vert == 0)
	{
		melee_range_vert = melee_range;
	}

	m_MeleePredictedAttTimeUnarmed		= 0.001f * spawnArgs.GetFloat("melee_predicted_attack_time");
	m_MeleePredictedAttTime				= m_MeleePredictedAttTimeUnarmed;
	m_MeleeDamageMult					= spawnArgs.GetFloat("melee_damage_mod","1.0f");
	m_MeleeHoldTimeMin					= spawnArgs.GetInt("melee_hold_time_min");
	m_MeleeHoldTimeMax					= spawnArgs.GetInt("melee_hold_time_max");
	m_MeleeParryHoldMax					= spawnArgs.GetInt("melee_parry_hold_max");
	m_MeleeParryHoldMin					= spawnArgs.GetInt("melee_parry_hold_min");
	m_MeleeCurrentParryHold				= -1;
	m_MeleeAttackRecoveryMin			= spawnArgs.GetInt("melee_attack_recovery_min");
	m_MeleeAttackRecoveryMax			= spawnArgs.GetInt("melee_attack_recovery_max");
	m_MeleeAttackLongRecoveryMin		= spawnArgs.GetInt("melee_attack_long_recovery_min");
	m_MeleeAttackLongRecoveryMax		= spawnArgs.GetInt("melee_attack_long_recovery_max");
	m_MeleeParryRecoveryMin				= spawnArgs.GetInt("melee_parry_recovery_min");
	m_MeleeParryRecoveryMax				= spawnArgs.GetInt("melee_parry_recovery_max");
	m_MeleeRiposteRecoveryMin			= spawnArgs.GetInt("melee_riposte_recovery_min");
	m_MeleeRiposteRecoveryMax			= spawnArgs.GetInt("melee_riposte_recovery_max");
	m_MeleePreParryDelayMin				= spawnArgs.GetInt("melee_pre_parry_delay_min");
	m_MeleePreParryDelayMax				= spawnArgs.GetInt("melee_pre_parry_delay_max");
	m_MeleeRepeatedPreParryDelayMin		= spawnArgs.GetInt("melee_repeated_pre_parry_delay_min");
	m_MeleeRepeatedPreParryDelayMax		= spawnArgs.GetInt("melee_repeated_pre_parry_delay_max");	
	m_MeleePostParryDelayMin			= spawnArgs.GetInt("melee_post_parry_delay_min");
	m_MeleePostParryDelayMax			= spawnArgs.GetInt("melee_post_parry_delay_max");
	m_MeleeRepeatedPostParryDelayMin	= spawnArgs.GetInt("melee_repeated_post_parry_delay_min");
	m_MeleeRepeatedPostParryDelayMax	= spawnArgs.GetInt("melee_repeated_post_parry_delay_max");
	m_MeleeNumRepAttacks				= spawnArgs.GetInt("melee_num_rep_attacks");
	m_MeleeRepAttackTime				= spawnArgs.GetInt("melee_rep_attack_time");

	m_MouthOffset						= spawnArgs.GetVector("mouth_offset"); // grayman #1104

	m_EyeOffset							= spawnArgs.GetVector("eye_offset"); // grayman #3525

	LoadAF();

	walkIK.Init( this, IK_ANIM, modelOffset );

	// the animation used to be set to the IK_ANIM at this point, but that was fixed, resulting in
	// attachments not binding correctly, so we're stuck setting the IK_ANIM before attaching things.
	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.SetFrame( ANIMCHANNEL_ALL, animator.GetAnim( IK_ANIM ), 0, 0, 0 );

	SetupDamageGroups();
	ParseAttachmentsAF();
	SetupHead();

	// clear the bind anim
	animator.ClearAllAnims( gameLocal.time, 0 );

	idEntity *headEnt = head.GetEntity();
	idAnimator *headAnimator;
	if ( headEnt ) {
		headAnimator = headEnt->GetAnimator();
	} else {
		headAnimator = &animator;
	}

	if ( headEnt ) {
		// set up the list of joints to copy to the head
		for( kv = spawnArgs.MatchPrefix( "copy_joint", NULL ); kv != NULL; kv = spawnArgs.MatchPrefix( "copy_joint", kv ) ) {
			if ( kv->GetValue() == "" ) {
				// probably clearing out inherited key, so skip it
				continue;
			}

			jointName = kv->GetKey();
			if ( jointName.StripLeadingOnce( "copy_joint_world " ) ) {
				copyJoint.mod = JOINTMOD_WORLD_OVERRIDE;
			} else {
				jointName.StripLeadingOnce( "copy_joint " );
				copyJoint.mod = JOINTMOD_LOCAL_OVERRIDE;
			}

			copyJoint.from = animator.GetJointHandle( jointName );
			if ( copyJoint.from == INVALID_JOINT ) {
				gameLocal.Warning( "Unknown copy_joint '%s' on entity %s", jointName.c_str(), name.c_str() );
				continue;
			}

			jointName = kv->GetValue();
			copyJoint.to = headAnimator->GetJointHandle( jointName );
			if ( copyJoint.to == INVALID_JOINT ) {
				gameLocal.Warning( "Unknown copy_joint '%s' on head of entity %s", jointName.c_str(), name.c_str() );
				continue;
			}

			copyJoints.Append( copyJoint );
		}
	}

	greetingState = spawnArgs.GetBool("canGreet", "0") ? ENotGreetingAnybody : ECannotGreet;

	// set up blinking
	blink_anim = headAnimator->GetAnim( "blink" );
	blink_time = 0;	// it's ok to blink right away
	blink_min = SEC2MS( spawnArgs.GetFloat( "blink_min", "0.5" ) );
	blink_max = SEC2MS( spawnArgs.GetFloat( "blink_max", "8" ) );

	// set up the head anim if necessary
	int headAnim = headAnimator->GetAnim( "def_head" );
	if ( headAnim ) {
		if ( headEnt ) {
            headAnimator->CycleAnim( ANIMCHANNEL_ALL, headAnim, gameLocal.time, 0 );
		} else {
			headAnimator->CycleAnim( ANIMCHANNEL_HEAD, headAnim, gameLocal.time, 0 );
		}
	}

	if ( spawnArgs.GetString( "sound_bone", "", jointName ) ) {
		soundJoint = animator.GetJointHandle( jointName );
		if ( soundJoint == INVALID_JOINT ) {
			gameLocal.Warning( "idAnimated '%s' at (%s): cannot find joint '%s' for sound playback", name.c_str(), GetPhysics()->GetOrigin().ToString(0), jointName.c_str() );
		}
	}
	
	finalBoss = spawnArgs.GetBool( "finalBoss" );

	canUseElevators = spawnArgs.GetBool("canOperateElevators", "0");

	// greebo: Set up the melee flags for AI without weapons
	if (spawnArgs.GetBool("unarmed_melee"))
	{
		SetAttackFlag(COMBAT_MELEE, true);
		// add the general unarmed attack to possible melee attacks list
		// this will be overridden if they get a weapon attached
		// TODO: Add this back in if the weapon is later detached?
		m_MeleeStatus.m_attacks.Append(MELEETYPE_UNBLOCKABLE);
	}

	if (spawnArgs.GetBool("unarmed_ranged"))
	{
		SetAttackFlag(COMBAT_RANGED, true);
	}

	lowHealthThreshold = spawnArgs.GetInt("low_health_threshold", "-1");
	lowHealthScript = spawnArgs.GetString("low_health_script");

	LoadVocalSet();

	// Load replacement animations from our own spawnargs
	LoadReplacementAnims(spawnArgs,NULL); // grayman #3074 - load all replacements

	FinishSetup();

	CREATE_TIMER(actorGetObstaclesTimer, name, "GetObstacles");
	CREATE_TIMER(actorGetPointOutsideObstaclesTimer, name, "GetPointOutsideObstacles");
	CREATE_TIMER(actorGetWallEdgesTimer, name, "GetWallEdges");
	CREATE_TIMER(actorSortWallEdgesTimer, name, "SortWallEdges");
	CREATE_TIMER(actorBuildPathTreeTimer, name, "BuildPathTree");
	CREATE_TIMER(actorPrunePathTreeTimer, name, "PrunePathTree");
	CREATE_TIMER(actorFindOptimalPathTimer, name, "FindOptimalPath");
	CREATE_TIMER(actorRouteToGoalTimer, name, "RouteToGoal");
	CREATE_TIMER(actorSubSampleWalkPathTimer, name, "SubSampleWalkPath");
	CREATE_TIMER(actorWalkPathValidTimer, name, "WalkPathValid");

	m_pathRank = rank;	// grayman #2345 - rank for path-finding
	m_nextKickTime = 0;	// grayman #2728
}

/*
================
idActor::FinishSetup
================
*/
void idActor::FinishSetup( void ) {
	const char	*scriptObjectName;

	// setup script object
	if ( spawnArgs.GetString( "scriptobject", NULL, &scriptObjectName ) ) {
		if ( !scriptObject.SetType( scriptObjectName ) ) {
			gameLocal.Error( "Script object '%s' not found on entity '%s'.", scriptObjectName, name.c_str() );
		}

		ConstructScriptObject();
	}

	SetupBody();
}

/*
================
idActor::SetupHead
================
*/
void idActor::SetupHead()
{
	idStr headModelDefName = spawnArgs.GetString( "def_head", "" );

	if (!headModelDefName.IsEmpty())
	{
		// We look if the head model is defined as a key to have a specific offset.
		// If that is not the case, then we use the default value, if it exists, 
		// otherwise there is no offset at all.
		mHeadModelOffset = spawnArgs.GetVector(headModelDefName, "0 0 0");

		// greebo: Regardless what happens, the offsetHeadModel vector always gets added to the offset
		mHeadModelOffset += spawnArgs.GetVector("offsetHeadModel", "0 0 0");

		idStr jointName = spawnArgs.GetString( "head_joint" );
		jointHandle_t joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'head_joint' on '%s'", jointName.c_str(), name.c_str() );
		}

		// set the damage joint to be part of the head damage group (if possible)
		jointHandle_t damageJoint = joint;

		for (int i = 0; i < damageGroups.Num(); i++ )
		{
			if ( damageGroups[ i ] == "head" ) {
				damageJoint = static_cast<jointHandle_t>( i );
				break;
			}
		}

		// Setup the default spawnargs for all heads
		idDict args;

		const idDeclEntityDef* def = gameLocal.FindEntityDef(headModelDefName, false);

		if (def == NULL)
		{
			gameLocal.Warning("Could not find head entityDef %s!", headModelDefName.c_str());

			// Try to fallback on the default head entityDef
			def = gameLocal.FindEntityDef(TDM_HEAD_ENTITYDEF, false);
		}

		if (def != NULL)
		{
			// Make a copy of the default spawnargs
			args = def->dict;
		}
		else
		{
			gameLocal.Warning("Could not find head entityDef %s or %s!", headModelDefName.c_str(), TDM_HEAD_ENTITYDEF);
		}		
		
		// Copy any sounds in case we have frame commands on the head
		for (const idKeyValue* kv = spawnArgs.MatchPrefix("snd_", NULL); kv != NULL; kv = spawnArgs.MatchPrefix("snd_", kv)) 
		{
			args.Set(kv->GetKey(), kv->GetValue());
		}

		for (const idKeyValue* kv_set = spawnArgs.MatchPrefix("set ", NULL); kv_set != NULL; kv_set = spawnArgs.MatchPrefix("set ", kv_set))
		{
			// "set FOO on HEAD"
			idStr SpawnargName(kv_set->GetKey());

			// check whether this spawnarg should apply to the head
			if (SpawnargName.Right(4) == "head")
			{
				// "set FOO on HEAD" => "FOO on HEAD"
				SpawnargName = SpawnargName.Right(kv_set->GetKey().Length() - 4);

				// find position of first ' '	
				int PosSpace = SpawnargName.Find(' ', 0, -1);

				// "FOO on HEAD" => "FOO"
				SpawnargName = SpawnargName.Left(PosSpace);

				// add the spawnarg to the args list
				args.Set(SpawnargName, kv_set->GetValue());
			}
		}


		// Spawn the head entity
		idEntity* ent = gameLocal.SpawnEntityType(idAFAttachment::Type, &args);
		idAFAttachment* headEnt = static_cast<idAFAttachment*>(ent);
		headEnt->SetName( name + "_head" );

		// Retrieve the actual model from the head entityDef
		idStr headModel = args.GetString("model");
		if (headModel.IsEmpty())
		{
			gameLocal.Warning("No 'model' spawnarg on head entityDef: %s", headModelDefName.c_str());
		}
		headEnt->SetBody( this, headModel, damageJoint );
		headEnt->SetCombatModel();

		// Store the head locally
		head = headEnt;

		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("SETBODY: Actor %s : damage joint %d for attached head is part of damage group %s\r", name.c_str(), (int) damageJoint, GetDamageGroup( damageJoint ) );

		// Add the head as attachment
		idVec3 origin;
		idMat3 axis;
		CAttachInfo& attach = m_Attachments.Alloc();

		attach.channel = animator.GetChannelForJoint( joint );
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + ( origin + modelOffset + mHeadModelOffset ) * renderEntity.axis;
		attach.ent = headEnt;
		attach.posName = jointName; // grayman #2603

		headEnt->SetOrigin( origin );
		headEnt->SetAxis( renderEntity.axis );
		headEnt->BindToJoint( this, joint, true );

		// greebo: Setup the frob-peer relationship between head and body
		m_FrobPeers.AddUnique(headEnt->name);
	}
}

/*
================
idActor::CopyJointsFromBodyToHead
================
*/
void idActor::CopyJointsFromBodyToHead( void ) {
	idEntity	*headEnt = head.GetEntity();
	idAnimator	*headAnimator;
	int			i;
	idMat3		mat;
	idMat3		axis;
	idVec3		pos;

	if ( !headEnt ) {
		return;
	}

	headAnimator = headEnt->GetAnimator();

	// copy the animation from the body to the head
	for( i = 0; i < copyJoints.Num(); i++ ) {
		if ( copyJoints[ i ].mod == JOINTMOD_WORLD_OVERRIDE ) {
			mat = headEnt->GetPhysics()->GetAxis().Transpose();
			GetJointWorldTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
			pos -= headEnt->GetPhysics()->GetOrigin();
			headAnimator->SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos * mat );
			headAnimator->SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis * mat );
		} else {
			animator.GetJointLocalTransform( copyJoints[ i ].from, gameLocal.time, pos, axis );
			headAnimator->SetJointPos( copyJoints[ i ].to, copyJoints[ i ].mod, pos );
			headAnimator->SetJointAxis( copyJoints[ i ].to, copyJoints[ i ].mod, axis );
		}
	}
}

/*
================
idActor::Restart
================
*/
void idActor::Restart( void ) {
	assert( !head.GetEntity() );
	SetupHead();
	FinishSetup();
}

/*
================
idActor::Save

archive object for savegame file
================
*/
void idActor::Save( idSaveGame *savefile ) const {
	idActor *ent;
	int i;

	savefile->WriteInt( rank );
	savefile->WriteInt( m_AItype );
	savefile->WriteBool( m_Innocent );
	savefile->WriteMat3( viewAxis );

	savefile->WriteInt( enemyList.Num() );
	for ( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		savefile->WriteObject( ent );
	}

	savefile->WriteInt(static_cast<int>(greetingState));

	// melee stuff
	m_MeleeStatus.Save( savefile );
	savefile->WriteFloat( melee_range_unarmed );
	savefile->WriteFloat( melee_range );
	savefile->WriteFloat( melee_range_vert ); // grayman #2655
	savefile->WriteFloat( m_MeleePredictedAttTimeUnarmed );
	savefile->WriteFloat( m_MeleePredictedAttTime );
	savefile->WriteFloat( m_MeleeDamageMult );
	savefile->WriteInt( m_MeleeHoldTimeMin );
	savefile->WriteInt( m_MeleeHoldTimeMax );
	savefile->WriteInt( m_MeleeCurrentHoldTime );
	savefile->WriteInt( m_MeleeAttackRecoveryMin );
	savefile->WriteInt( m_MeleeAttackRecoveryMax );
	savefile->WriteInt( m_MeleeCurrentAttackRecovery );
	savefile->WriteInt( m_MeleeAttackLongRecoveryMin );
	savefile->WriteInt( m_MeleeAttackLongRecoveryMax );
	savefile->WriteInt( m_MeleeCurrentAttackLongRecovery );
	savefile->WriteInt( m_MeleeParryRecoveryMin );
	savefile->WriteInt( m_MeleeParryRecoveryMax );
	savefile->WriteInt( m_MeleeCurrentParryRecovery );
	savefile->WriteInt( m_MeleeRiposteRecoveryMin );
	savefile->WriteInt( m_MeleeRiposteRecoveryMax );
	savefile->WriteInt( m_MeleeCurrentRiposteRecovery );
	savefile->WriteInt( m_MeleePreParryDelayMin );
	savefile->WriteInt( m_MeleePreParryDelayMax );
	savefile->WriteInt( m_MeleeCurrentPreParryDelay );
	savefile->WriteInt( m_MeleeRepeatedPreParryDelayMin );
	savefile->WriteInt( m_MeleeRepeatedPreParryDelayMax );
	savefile->WriteInt( m_MeleeCurrentRepeatedPreParryDelay );
	savefile->WriteInt( m_MeleeNumRepAttacks );
	savefile->WriteInt( m_MeleeRepAttackTime );
	savefile->WriteInt( m_MeleePostParryDelayMin );
	savefile->WriteInt( m_MeleePostParryDelayMax );
	savefile->WriteInt( m_MeleeCurrentPostParryDelay );
	savefile->WriteInt( m_MeleeRepeatedPostParryDelayMin );
	savefile->WriteInt( m_MeleeRepeatedPostParryDelayMax );
	savefile->WriteInt( m_MeleeCurrentRepeatedPostParryDelay );
	savefile->WriteInt( m_pathRank );		// grayman #2345
	savefile->WriteInt( m_nextKickTime );	// grayman #2728
	savefile->WriteVec3( m_MouthOffset );	// grayman #1104
	savefile->WriteVec3( m_EyeOffset );	// grayman #3525

	// grayman #3857

	savefile->WriteInt(m_knownSuspiciousEvents.Num());
	for ( int i = 0 ; i < m_knownSuspiciousEvents.Num() ; i++ )
	{
		savefile->WriteInt(m_knownSuspiciousEvents[i].eventID);
		savefile->WriteBool(m_knownSuspiciousEvents[i].searched);
	}

	// grayman #3424
	savefile->WriteInt(m_warningEvents.Num());
	for ( int i = 0 ; i < m_warningEvents.Num() ; i++ )
	{
		savefile->WriteInt(m_warningEvents[i].eventID);
		m_warningEvents[i].entity.Save(savefile);
	}

	savefile->WriteBool(m_victorHasKnealt); // grayman #3848

	m_killedBy.Save(savefile); // grayman #3848

	savefile->WriteFloat( m_fovDotHoriz );
	savefile->WriteFloat( m_fovDotVert );
	savefile->WriteVec3( eyeOffset );
	savefile->WriteVec3( modelOffset );
	savefile->WriteVec3(mHeadModelOffset);
	savefile->WriteAngles( deltaViewAngles );

	savefile->WriteInt( pain_debounce_time );
	savefile->WriteInt( pain_delay );
	savefile->WriteInt( pain_threshold );

	savefile->WriteInt( damageGroups.Num() );
	for( i = 0; i < damageGroups.Num(); i++ ) {
		savefile->WriteString( damageGroups[ i ] );
	}

	savefile->WriteInt( damageScale.Num() );
	for( i = 0; i < damageScale.Num(); i++ ) {
		savefile->WriteFloat( damageScale[ i ] );
	}

	savefile->WriteInt(lowHealthThreshold);
	savefile->WriteString(lowHealthScript);

	savefile->WriteFloat( m_SneakAttackThresh );
	savefile->WriteFloat( m_SneakAttackMult );

	savefile->WriteBool( use_combat_bbox );
	head.Save( savefile );

	savefile->WriteInt( copyJoints.Num() );
	for( i = 0; i < copyJoints.Num(); i++ ) {
		savefile->WriteInt( copyJoints[i].mod );
		savefile->WriteJoint( copyJoints[i].from );
		savefile->WriteJoint( copyJoints[i].to );
	}

	savefile->WriteJoint( leftEyeJoint );
	savefile->WriteJoint( rightEyeJoint );
	savefile->WriteJoint( soundJoint );

	walkIK.Save( savefile );

	savefile->WriteString( animPrefix );
	savefile->WriteString( painAnim );

	savefile->WriteInt( blink_anim );
	savefile->WriteInt( blink_time );
	savefile->WriteInt( blink_min );
	savefile->WriteInt( blink_max );

	// script variables
	savefile->WriteObject( scriptThread );

	savefile->WriteString( waitState );

	headAnim.Save( savefile );
	torsoAnim.Save( savefile );
	legsAnim.Save( savefile );

	savefile->WriteBool( allowPain );
	savefile->WriteBool( allowEyeFocus );

	savefile->WriteInt( painTime );

	savefile->WriteBool(canUseElevators);

	savefile->WriteBool( finalBoss );
	savefile->WriteFloat( m_damage_thresh_hard );
	savefile->WriteFloat( m_delta_scale );
	savefile->WriteFloat( m_damage_thresh_min );
	savefile->WriteVec3(m_savedVelocity); // grayman #3699

	idToken token;

	//FIXME: this is unneccesary
	if ( state ) {
		idLexer src( state->Name(), idStr::Length( state->Name() ), "idAI::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

	if ( idealState ) {
		idLexer src( idealState->Name(), idStr::Length( idealState->Name() ), "idAI::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}
	
	savefile->WriteFloat(m_stepvol_walk);
	savefile->WriteFloat(m_stepvol_run);
	savefile->WriteFloat(m_stepvol_creep);

	savefile->WriteFloat(m_stepvol_crouch_walk);
	savefile->WriteFloat(m_stepvol_crouch_creep);
	savefile->WriteFloat(m_stepvol_crouch_run);

	savefile->WriteDict(&m_replacementAnims);

	savefile->WriteInt(static_cast<int>(m_AttackFlags.size()));
	for (std::set<int>::const_iterator i = 	m_AttackFlags.begin(); i != m_AttackFlags.end(); ++i)
	{
		savefile->WriteInt(static_cast<int>(*i));
	}

	savefile->WriteInt(m_timeFellDown); // grayman #3317

	savefile->WriteBool(m_isMute);		// grayman #3202

	SAVE_TIMER_HANDLE(actorGetObstaclesTimer, savefile);
	SAVE_TIMER_HANDLE(actorGetPointOutsideObstaclesTimer, savefile);
	SAVE_TIMER_HANDLE(actorGetWallEdgesTimer, savefile);
	SAVE_TIMER_HANDLE(actorSortWallEdgesTimer, savefile);
	SAVE_TIMER_HANDLE(actorBuildPathTreeTimer, savefile);
	SAVE_TIMER_HANDLE(actorPrunePathTreeTimer, savefile);
	SAVE_TIMER_HANDLE(actorFindOptimalPathTimer, savefile);
	SAVE_TIMER_HANDLE(actorRouteToGoalTimer, savefile);
	SAVE_TIMER_HANDLE(actorSubSampleWalkPathTimer, savefile);
	SAVE_TIMER_HANDLE(actorWalkPathValidTimer, savefile);
}

/*
================
idActor::Restore

unarchives object from save game file
================
*/
void idActor::Restore( idRestoreGame *savefile ) {
	int i, num;
	idActor *ent;

	savefile->ReadInt( rank );
	savefile->ReadInt( m_AItype );
	savefile->ReadBool( m_Innocent );
	savefile->ReadMat3( viewAxis );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->enemyNode.AddToEnd( enemyList );
		}
	}

	savefile->ReadInt(i);
	assert(i >= ECannotGreet && i < ENumAIGreetingStates);
	greetingState = static_cast<GreetingState>(i);

	// melee stuff
	m_MeleeStatus.Restore( savefile );
	savefile->ReadFloat( melee_range_unarmed );
	savefile->ReadFloat( melee_range );
	savefile->ReadFloat( melee_range_vert ); // grayman #2655
	savefile->ReadFloat( m_MeleePredictedAttTimeUnarmed );
	savefile->ReadFloat( m_MeleePredictedAttTime );
	savefile->ReadFloat( m_MeleeDamageMult );
	savefile->ReadInt( m_MeleeHoldTimeMin );
	savefile->ReadInt( m_MeleeHoldTimeMax );
	savefile->ReadInt( m_MeleeCurrentHoldTime );
	savefile->ReadInt( m_MeleeAttackRecoveryMin );
	savefile->ReadInt( m_MeleeAttackRecoveryMax );
	savefile->ReadInt( m_MeleeCurrentAttackRecovery );
	savefile->ReadInt( m_MeleeAttackLongRecoveryMin );
	savefile->ReadInt( m_MeleeAttackLongRecoveryMax );
	savefile->ReadInt( m_MeleeCurrentAttackLongRecovery );
	savefile->ReadInt( m_MeleeParryRecoveryMin );
	savefile->ReadInt( m_MeleeParryRecoveryMax );
	savefile->ReadInt( m_MeleeCurrentParryRecovery );
	savefile->ReadInt( m_MeleeRiposteRecoveryMin );
	savefile->ReadInt( m_MeleeRiposteRecoveryMax );
	savefile->ReadInt( m_MeleeCurrentRiposteRecovery );
	savefile->ReadInt( m_MeleePreParryDelayMin );
	savefile->ReadInt( m_MeleePreParryDelayMax );
	savefile->ReadInt( m_MeleeCurrentPreParryDelay );
	savefile->ReadInt( m_MeleeRepeatedPreParryDelayMin );
	savefile->ReadInt( m_MeleeRepeatedPreParryDelayMax );
	savefile->ReadInt( m_MeleeCurrentRepeatedPreParryDelay );
	savefile->ReadInt( m_MeleeNumRepAttacks );
	savefile->ReadInt( m_MeleeRepAttackTime );
	savefile->ReadInt( m_MeleePostParryDelayMin );
	savefile->ReadInt( m_MeleePostParryDelayMax );
	savefile->ReadInt( m_MeleeCurrentPostParryDelay );
	savefile->ReadInt( m_MeleeRepeatedPostParryDelayMin );
	savefile->ReadInt( m_MeleeRepeatedPostParryDelayMax );
	savefile->ReadInt( m_MeleeCurrentRepeatedPostParryDelay );
	savefile->ReadInt( m_pathRank );	 // grayman #2345
	savefile->ReadInt( m_nextKickTime ); // grayman #2728
	savefile->ReadVec3( m_MouthOffset ); // grayman #1104
	savefile->ReadVec3( m_EyeOffset );	// grayman #3525

	// grayman #3424
	savefile->ReadInt(num);
	m_knownSuspiciousEvents.Clear();
	m_knownSuspiciousEvents.SetNum(num);
	for ( int i = 0 ; i < num ; i++ )
	{
		savefile->ReadInt(m_knownSuspiciousEvents[i].eventID);
		savefile->ReadBool(m_knownSuspiciousEvents[i].searched);
	}

	// grayman #3424
	savefile->ReadInt(num);
	m_warningEvents.Clear();
	m_warningEvents.SetNum(num);
	for ( int i = 0 ; i < num ; i++ )
	{
		savefile->ReadInt(m_warningEvents[i].eventID);
		m_warningEvents[i].entity.Restore(savefile);
	}

	savefile->ReadBool(m_victorHasKnealt); // grayman #3848

	m_killedBy.Restore(savefile); // grayman #3848

	savefile->ReadFloat( m_fovDotHoriz );
	savefile->ReadFloat( m_fovDotVert );
	savefile->ReadVec3( eyeOffset );
	savefile->ReadVec3( modelOffset );
	savefile->ReadVec3(mHeadModelOffset);
	savefile->ReadAngles( deltaViewAngles );

	savefile->ReadInt( pain_debounce_time );
	savefile->ReadInt( pain_delay );
	savefile->ReadInt( pain_threshold );

	savefile->ReadInt( num );
	damageGroups.SetGranularity( 1 );
	damageGroups.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadString( damageGroups[ i ] );
	}

	savefile->ReadInt( num );
	damageScale.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadFloat( damageScale[ i ] );
	}

	savefile->ReadInt(lowHealthThreshold);
	savefile->ReadString(lowHealthScript);

	savefile->ReadFloat( m_SneakAttackThresh );
	savefile->ReadFloat( m_SneakAttackMult );

	savefile->ReadBool( use_combat_bbox );
	head.Restore( savefile );

	savefile->ReadInt( num );
	copyJoints.SetNum( num );
	for( i = 0; i < num; i++ ) {
		int val;
		savefile->ReadInt( val );
		copyJoints[i].mod = static_cast<jointModTransform_t>( val );
		savefile->ReadJoint( copyJoints[i].from );
		savefile->ReadJoint( copyJoints[i].to );
	}

	savefile->ReadJoint( leftEyeJoint );
	savefile->ReadJoint( rightEyeJoint );
	savefile->ReadJoint( soundJoint );

	walkIK.Restore( savefile );

	savefile->ReadString( animPrefix );
	savefile->ReadString( painAnim );

	savefile->ReadInt( blink_anim );
	savefile->ReadInt( blink_time );
	savefile->ReadInt( blink_min );
	savefile->ReadInt( blink_max );

	savefile->ReadObject( reinterpret_cast<idClass *&>( scriptThread ) );

	savefile->ReadString( waitState );

	headAnim.Restore( savefile );
	torsoAnim.Restore( savefile );
	legsAnim.Restore( savefile );

	savefile->ReadBool( allowPain );
	savefile->ReadBool( allowEyeFocus );

	savefile->ReadInt( painTime );
	savefile->ReadBool(canUseElevators);

	savefile->ReadBool( finalBoss );
	savefile->ReadFloat( m_damage_thresh_hard );
	savefile->ReadFloat( m_delta_scale );
	savefile->ReadFloat( m_damage_thresh_min );
	savefile->ReadVec3(m_savedVelocity); // grayman #3699

	idStr statename;

	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		state = GetScriptFunction( statename );
	}

	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		idealState = GetScriptFunction( statename );
	}
	
	savefile->ReadFloat(m_stepvol_walk);
	savefile->ReadFloat(m_stepvol_run);
	savefile->ReadFloat(m_stepvol_creep);

	savefile->ReadFloat(m_stepvol_crouch_walk);
	savefile->ReadFloat(m_stepvol_crouch_creep);
	savefile->ReadFloat(m_stepvol_crouch_run);

	savefile->ReadDict(&m_replacementAnims);

	m_AttackFlags.clear();
	savefile->ReadInt(num);
	for (int i = 0; i < num; i++)
	{
		int temp;
		savefile->ReadInt(temp);
		assert(static_cast<ECombatType>(temp) >= COMBAT_NONE && static_cast<ECombatType>(temp) <= COMBAT_RANGED);
		m_AttackFlags.insert(temp);
	}

	savefile->ReadInt(m_timeFellDown);	// grayman #3317

	savefile->ReadBool(m_isMute);		// grayman #3202

	RESTORE_TIMER_HANDLE(actorGetObstaclesTimer, savefile);
	RESTORE_TIMER_HANDLE(actorGetPointOutsideObstaclesTimer, savefile);
	RESTORE_TIMER_HANDLE(actorGetWallEdgesTimer, savefile);
	RESTORE_TIMER_HANDLE(actorSortWallEdgesTimer, savefile);
	RESTORE_TIMER_HANDLE(actorBuildPathTreeTimer, savefile);
	RESTORE_TIMER_HANDLE(actorPrunePathTreeTimer, savefile);
	RESTORE_TIMER_HANDLE(actorFindOptimalPathTimer, savefile);
	RESTORE_TIMER_HANDLE(actorRouteToGoalTimer, savefile);
	RESTORE_TIMER_HANDLE(actorSubSampleWalkPathTimer, savefile);
	RESTORE_TIMER_HANDLE(actorWalkPathValidTimer, savefile);

}

/*
================
idActor::Hide
================
*/
void idActor::Hide( void ) 
{
	idAFEntity_Base::Hide();
	if ( head.GetEntity() ) 
	{
		head.GetEntity()->Hide();
	}
	UnlinkCombat();
}

/*
================
idActor::Show
================
*/
void idActor::Show( void ) 
{
	idAFEntity_Base::Show();
	if ( head.GetEntity() ) {
		head.GetEntity()->Show();
	}

	LinkCombat();
}

/*
==============
idActor::GetDefaultSurfaceType
==============
*/
int	idActor::GetDefaultSurfaceType( void ) const {
	return SURFTYPE_FLESH;
}

/*
================
idActor::ProjectOverlay
================
*/
void idActor::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material, bool save ) {
	idEntity *ent;
	idEntity *next;

	idEntity::ProjectOverlay( origin, dir, size, material, save );

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			if ( ent->fl.takedamage && ent->spawnArgs.GetBool( "bleed" ) ) {
				ent->ProjectOverlay( origin, dir, size, material, save );
			}
		}
	}
}

/*
================
idActor::LoadAF
================
*/
bool idActor::LoadAF( void ) 
{
	bool bReturnVal = false;
	idStr fileName;

	if ( !spawnArgs.GetString( "ragdoll", "*unknown*", fileName ) || !fileName.Length() ) 
	{
		goto Quit;
	}
	af.SetAnimator( GetAnimator() );
	bReturnVal =  af.Load( this, fileName );
	SetUpGroundingVars();

	if( m_bAFPushMoveables )
	{
		af.SetupPose( this, gameLocal.time );
		af.GetPhysics()->EnableClip();
	}

Quit:
	return bReturnVal;
}

/*
=====================
idActor::SetupBody
=====================
*/
void idActor::SetupBody( void ) {
	const char *jointname;

	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.ClearAllJoints();

	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		jointname = spawnArgs.GetString( "bone_leftEye" );
		leftEyeJoint = headEnt->GetAnimator()->GetJointHandle( jointname );

		jointname = spawnArgs.GetString( "bone_rightEye" );
		rightEyeJoint = headEnt->GetAnimator()->GetJointHandle( jointname );

		// set up the eye height.  check if it's specified in the def.
		if ( !spawnArgs.GetFloat( "eye_height", "0", eyeOffset.z ) ) {
			// if not in the def, then try to base it off the idle animation
			int anim = headEnt->GetAnimator()->GetAnim( "idle" );
			if ( anim && ( leftEyeJoint != INVALID_JOINT ) ) {
				idVec3 pos;
				idMat3 axis;
				headEnt->GetAnimator()->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
				headEnt->GetAnimator()->GetJointTransform( leftEyeJoint, gameLocal.time, pos, axis );
				headEnt->GetAnimator()->ClearAllAnims( gameLocal.time, 0 );
				headEnt->GetAnimator()->ForceUpdate();
				pos += headEnt->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
				eyeOffset = pos + modelOffset;
			} else {
				// just base it off the bounding box size
				eyeOffset.z = GetPhysics()->GetBounds()[ 1 ].z - 6;
			}
		}
		headAnim.Init( this, headEnt->GetAnimator(), ANIMCHANNEL_ALL );
	} else {
		jointname = spawnArgs.GetString( "bone_leftEye" );
		leftEyeJoint = animator.GetJointHandle( jointname );

		jointname = spawnArgs.GetString( "bone_rightEye" );
		rightEyeJoint = animator.GetJointHandle( jointname );

		// set up the eye height.  check if it's specified in the def.
		if ( !spawnArgs.GetFloat( "eye_height", "0", eyeOffset.z ) ) {
			// if not in the def, then try to base it off the idle animation
			int anim = animator.GetAnim( "idle" );
			if ( anim && ( leftEyeJoint != INVALID_JOINT ) ) {
				idVec3 pos;
				idMat3 axis;
				animator.PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, 0 );
				animator.GetJointTransform( leftEyeJoint, gameLocal.time, pos, axis );
				animator.ClearAllAnims( gameLocal.time, 0 );
				animator.ForceUpdate();
				eyeOffset = pos + modelOffset;
			} else {
				// just base it off the bounding box size
				eyeOffset.z = GetPhysics()->GetBounds()[ 1 ].z - 6;
			}
		}
		headAnim.Init( this, &animator, ANIMCHANNEL_HEAD );
	}

	waitState = "";

	torsoAnim.Init( this, &animator, ANIMCHANNEL_TORSO );
	legsAnim.Init( this, &animator, ANIMCHANNEL_LEGS );
}

/*
=====================
idActor::CheckBlink
=====================
*/
void idActor::CheckBlink( void ) {
	// check if it's time to blink
	if ( !blink_anim || ( health <= 0 ) || !allowEyeFocus || ( blink_time > gameLocal.time ) ) {
		return;
	}

	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->GetAnimator()->PlayAnim( ANIMCHANNEL_EYELIDS, blink_anim, gameLocal.time, 1 );
	} else {
		animator.PlayAnim( ANIMCHANNEL_EYELIDS, blink_anim, gameLocal.time, 1 );
	}

	// set the next blink time
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomInt( blink_max - blink_min );
}

/*
================
idActor::GetPhysicsToVisualTransform
================
*/
bool idActor::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}
	origin = modelOffset;
	axis = viewAxis;
	return true;
}

/*
================
idActor::GetPhysicsToSoundTransform
================
*/
bool idActor::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis )
{
	if ( soundJoint != INVALID_JOINT )
	{
		animator.GetJointTransform( soundJoint, gameLocal.time, origin, axis );
		origin += modelOffset;
		axis = viewAxis;
	}
	else
	{
		// grayman #3340 - this was giving the location of standing
		// eyes whether the actor was standing or sleeping.
		// This causes a problem when sleeping in a low-ceilinged
		// room, because it pushes the sound origin up into the void,
		// or possibly even the room above. If sleeping, use the AI's
		// origin, which is not technically where his mouth is, but it's
		// good enough for what we're doing here, and certainly better
		// than being in the room above.
		if ( IsType(idAI::Type) && ( static_cast<idAI*>(this)->GetMoveType() == MOVETYPE_SLEEP ) )
		{
			origin.Zero();
		}
		else
		{
			origin = GetPhysics()->GetGravityNormal() * -eyeOffset.z;
		}
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
	headAnim.Shutdown();
	torsoAnim.Shutdown();
	legsAnim.Shutdown();

	if ( scriptThread ) {
		scriptThread->EndThread();
		scriptThread->PostEventMS( &EV_Remove, 0 );
		delete scriptThread;
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
idThread *idActor::ConstructScriptObject( void ) {
	const function_t *constructor;

	// make sure we have a scriptObject
	if ( !scriptObject.HasObject() ) {
		gameLocal.Error( "No scriptobject set on '%s'.  Check the '%s' entityDef.", name.c_str(), GetEntityDefName() );
	}

	if ( !scriptThread ) {
		// create script thread
		scriptThread = new idThread();
		scriptThread->ManualDelete();
		scriptThread->ManualControl();
		scriptThread->SetThreadName( name.c_str() );
	} else {
		scriptThread->EndThread();
	}
	
	// call script object's constructor
	constructor = scriptObject.GetConstructor();
	if ( !constructor ) {
		gameLocal.Error( "Missing constructor on '%s' for entity '%s'", scriptObject.GetTypeName(), name.c_str() );
	}

	// init the script object's data
	scriptObject.ClearObject();

	// just set the current function on the script.  we'll execute in the subclasses.
	scriptThread->CallFunction( this, constructor, true );

	return scriptThread;
}

/*
=====================
idActor::GetScriptFunction
=====================
*/
const function_t *idActor::GetScriptFunction( const char *funcname ) {
	const function_t *func;

	func = scriptObject.GetFunction( funcname );
	if ( !func ) {
		scriptThread->Error( "Unknown function '%s' in '%s'", funcname, scriptObject.GetTypeName() );
	}

	return func;
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const function_t *newState ) {
	if ( !newState ) {
		gameLocal.Error( "idActor::SetState: Null state" );
	}

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		gameLocal.Printf( "%d: %s: State: %s\n", gameLocal.time, name.c_str(), newState->Name() );
	}

	state = newState;
	idealState = state;
	scriptThread->CallFunction( this, state, true );
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const char *statename ) {
	const function_t *newState;

	newState = GetScriptFunction( statename );
	SetState( newState );
}

/*
=====================
idActor::UpdateScript
=====================
*/
void idActor::UpdateScript( void ) {
	int	i;

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		scriptThread->EnableDebugInfo();
	} else {
		scriptThread->DisableDebugInfo();
	}

	// a series of state changes can happen in a single frame.
	// this loop limits them in case we've entered an infinite loop.
	for( i = 0; i < 20; i++ ) {
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

	vision

***********************************************************************/

/*
=====================
idActor::setFov
=====================
*/
void idActor::SetFOV( float fovHoriz, float fovVert ) 
{
	m_fovDotHoriz = (float) cos( DEG2RAD( fovHoriz * 0.5f ) );

	// if fovVert not specified (default val of -1), make same as horizontal
	if( fovVert == -1 )
		m_fovDotVert = m_fovDotHoriz;
	else
		m_fovDotVert = (float) cos( DEG2RAD( fovVert * 0.5f) );
}

/*
=====================
idActor::SetEyeHeight
=====================
*/
void idActor::SetEyeHeight( float height ) {
	eyeOffset.z = height;
}

/*
=====================
idActor::EyeHeight
=====================
*/
float idActor::EyeHeight( void ) const {
	return eyeOffset.z;
}

/*
=====================
idActor::EyeOffset
=====================
*/
idVec3 idActor::EyeOffset( void ) const {
	return GetPhysics()->GetGravityNormal() * -eyeOffset.z;
}

/*
=====================
idActor::GetEyePosition
=====================
*/
idVec3 idActor::GetEyePosition( void ) const
{
	return GetPhysics()->GetOrigin() + ( GetPhysics()->GetGravityNormal() * -eyeOffset.z );
}

/*
=====================
idActor::GetViewPos
=====================
*/
void idActor::GetViewPos( idVec3 &origin, idMat3 &axis ) const {
	origin = GetEyePosition();
	axis = viewAxis;
}

/*
=====================
idActor::CheckFOV
=====================
*/
bool idActor::CheckFOV( const idVec3 &pos ) const 
{
	//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("idActor::CheckFOV was called\r");

	if ( m_fovDotHoriz == 1.0f ) {
		return true;
	}

	float	dot;
	idVec3	delta;
	
	delta = pos - GetEyePosition();

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	dot = viewAxis[ 0 ] * delta;

	return ( dot >= m_fovDotHoriz );
}

/*
=====================
idActor::CanSee
=====================
*/
bool idActor::CanSee( idEntity *ent, bool useFov ) const
{
	idVec3 origin; // The entity's origin
	trace_t result; // results of the traces
	idVec3 eye(GetEyePosition()); // eye position of the AI

	// angua: If the target entity is an idActor,
	// use its eye position, its origin and its shoulders

	// grayman #3992 - problem: if an AI has been KO'ed or killed,
	// it's in ragdoll form, and GetViewPos() returns the angle the
	// AI was facing before it became a ragdoll, which is useless here.

	if (ent->IsType(idActor::Type)) 
	{
		// grayman #3643 - shouldn't be able to see the actor if he's marked 'notarget'
		// grayman #3857 - or if marked 'invisible'
		if ((ent->fl.notarget) || (ent->fl.invisible))
		{
			return false;
		}

		bool fovEyeOK;
		bool fovOriginOK;
		bool fovShoulder1OK;
		bool fovShoulder2OK;

		idActor* actor = static_cast<idActor*>(ent);

		// Check eyes

		const idVec3& actorEyePos = actor->GetEyePosition();
		fovEyeOK = useFov ? CheckFOV(actorEyePos) : true;
		if ( fovEyeOK )
		{
			if ( !gameLocal.clip.TracePoint(result, eye, actorEyePos, MASK_OPAQUE, this) ||
				gameLocal.GetTraceEntity(result) == actor )
			{
				// Eye to eye trace succeeded
				// gameRenderWorld->DebugArrow(colorGreen,eye, actorEyePos, 1, 32);
				return true;
			}
		}

		// Check origin

		const idVec3& actorOrigin = actor->GetPhysics()->GetOrigin();
		fovOriginOK = useFov ? CheckFOV(actorOrigin) : true;

		if ( fovOriginOK )
		{
			if ( !gameLocal.clip.TracePoint(result, eye, actorOrigin, MASK_OPAQUE, this) ||
				gameLocal.GetTraceEntity(result) == actor )
			{
				// Eye to origin trace succeeded
				// gameRenderWorld->DebugArrow(colorGreen,eye, actorOrigin, 1, 32);
				return true;
			}
		}

		// Check one shoulder

		idVec3 dir;
		if ( actor->AI_DEAD || actor->IsKnockedOut() )
		{
			const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();
			idVec3 bodyAxis = actorEyePos - actorOrigin;
			bodyAxis.NormalizeFast();
			dir = bodyAxis.Cross(gravityDir);
		}
		else
		{
			idVec3 origin;
			idMat3 viewaxis;
			actor->GetViewPos(origin, viewaxis);

			const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();
			dir = (viewaxis[0] - gravityDir * ( gravityDir * viewaxis[0] )).Cross(gravityDir);
		}

		float dist = 8;

		const idVec3& shoulder1 = actorOrigin + (actorEyePos - actorOrigin)*0.7f + dir * dist;
		fovShoulder1OK = useFov ? CheckFOV(shoulder1) : true;

		if ( fovShoulder1OK )
		{
			if ( !gameLocal.clip.TracePoint(result, eye, shoulder1, MASK_OPAQUE, this) ||
				gameLocal.GetTraceEntity(result) == actor )
			{
				// Eye to shoulder1 trace succeeded
				// gameRenderWorld->DebugArrow(colorGreen,eye, shoulder1, 1, 32);
				return true;
			}
		}

		// Check other shoulder

		const idVec3& shoulder2 = actorOrigin + (actorEyePos - actorOrigin)*0.7f - dir * dist;
		fovShoulder2OK = useFov ? CheckFOV(shoulder2) : true;

		if ( fovShoulder2OK )
		{
			if ( !gameLocal.clip.TracePoint(result, eye, shoulder2, MASK_OPAQUE, this) ||
				gameLocal.GetTraceEntity(result) == actor )
			{
				// Eye to shoulder2 trace succeeded
				// gameRenderWorld->DebugArrow(colorGreen,eye, shoulder2, 1, 32);
				return true;
			}
		}

		return false;
	}

	if ( ent->IsType(CBinaryFrobMover::Type) )	// grayman #2861 - in the case of doors, use the 'closed origin' and not the 'origin'
	{
		CBinaryFrobMover* door = static_cast<CBinaryFrobMover*>(ent);
		origin = door->GetClosedOrigin();
	}
	else
	{
		origin = ent->GetPhysics()->GetOrigin();
	}

	const idVec3& entityOrigin = origin;

	// Check the field of view if specified

	if ( useFov )
	{
		if ( !CheckFOV(entityOrigin) )
		{
			// FOV check failed
			return false;
		}
	}

	// Use the origin for general entities.
	// Perform a trace from the eye position to the target entity.
	// TracePoint will return FALSE, when the trace.result is >= 1

	if (!gameLocal.clip.TracePoint(result, eye, entityOrigin, MASK_OPAQUE, this) || 
			gameLocal.GetTraceEntity(result) == ent) 
	{
		// Trace succeeded or hit the target entity itself
		return true;
	}

	// grayman #2603 - We can't see the entity itself. If we're trying to see a light source,
	// however, it might be embedded in a candle and/or a candle holder.
	// So we have to look up the chain of bindmasters, and if we can see any of them, we'll take it
	// that we can see the light source itself.

	if (ent->IsType(idLight::Type))
	{
		idEntity* entHit = gameLocal.GetTraceEntity(result); // grayman #2603
		idEntity* bindMaster = ent->GetBindMaster();
		while (bindMaster != NULL) // exit when bindMaster == NULL or we hit one of them
		{
			if ( entHit == bindMaster ) // grayman #2603
			{
				return true;
			}
			bindMaster = bindMaster->GetBindMaster(); // go up the hierarchy
		}

		// grayman #4290

		// It's possible that the origin of the light is embedded inside
		// another object, and that the light is a light holder whose origin
		// is the same as the light's. (Perhaps a chandelier.) Do one last
		// check at the true source of the light, using the light_center spawnarg.

		idVec3 lightCenter;
		idLight *light = static_cast<idLight*>(ent);
		light->spawnArgs.GetVector("light_center", "0 0 0", lightCenter);
		idVec3 truelight = entityOrigin + lightCenter;
		if ( lightCenter.LengthFast() > 0.01 )
		{
			if (!gameLocal.clip.TracePoint(result, eye, truelight, MASK_OPAQUE, this) || 
					gameLocal.GetTraceEntity(result) == ent) 
			{
				// Trace succeeded
				return true;
			}
		}

		// Check the light's bindMaster again

		entHit = gameLocal.GetTraceEntity(result);
		bindMaster = ent->GetBindMaster();
		while (bindMaster != NULL) // exit when bindMaster == NULL or we hit one of them
		{
			if ( entHit == bindMaster )
			{
				return true;
			}
			bindMaster = bindMaster->GetBindMaster(); // go up the hierarchy
		}
	}
	else if ( ent->IsType(CAbsenceMarker::Type) ) // grayman #2860
	{
		// We're trying to see an absence marker and can't. Check for the case where the missing item
		// was replaced by another item. For example, a lootable painting replaced by an empty frame.
		// If a replacement item is present, is that what the trace hit? If so, the trace was successful.

		CAbsenceMarker* marker = static_cast<CAbsenceMarker*>(ent);
		const idDict& refSpawnargs = marker->GetRefSpawnargs();
		idStr replacedByClass = refSpawnargs.GetString("replace");
		if ( !replacedByClass.IsEmpty() )
		{
			idEntity* entHit = gameLocal.GetTraceEntity(result); // the trace hit this
			if ( entHit != NULL )
			{
				if ( idStr::Icmp( entHit->spawnArgs.GetString("classname"),replacedByClass ) == 0 )
				{
					return true; // we hit the replacement item, which means a successful trace
				}
			}
		}
	}

	return false;
}

/*
=====================
idActor::PointVisible
=====================
*/
bool idActor::PointVisible( const idVec3 &point ) const {
	trace_t results;
	idVec3 start, end;

	start = GetEyePosition();
	end = point;
	end[2] += 1.0f;

	gameLocal.clip.TracePoint( results, start, end, MASK_OPAQUE, this );
	return ( results.fraction >= 1.0f );
}

/*
=====================
idActor::GetAIAimTargets

Returns positions for the AI to aim at.
=====================
*/
void idActor::GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos ) {
	headPos = lastSightPos + EyeOffset();
	chestPos = ( headPos + lastSightPos + GetPhysics()->GetBounds().GetCenter() ) * 0.5f;
}

/*
=====================
idActor::GetRenderView
=====================
*/
renderView_t *idActor::GetRenderView() {
	renderView_t *rv = idEntity::GetRenderView();
	rv->viewaxis = viewAxis;
	rv->vieworg = GetEyePosition();
	return rv;
}

/***********************************************************************

	Model/Ragdoll

***********************************************************************/

/*
================
idActor::SetCombatModel
================
*/
void idActor::SetCombatModel( void ) {
	idAFAttachment *headEnt;

	if ( !use_combat_bbox ) {
		if ( combatModel ) {
			combatModel->Unlink();
			combatModel->LoadModel( modelDefHandle );
		} else {
			combatModel = new idClipModel( modelDefHandle );
		}

		headEnt = head.GetEntity();
		if ( headEnt ) {
			headEnt->SetCombatModel();
		}
	}
}

/*
================
idActor::GetCombatModel
================
*/
idClipModel *idActor::GetCombatModel( void ) const {
	return combatModel;
}

/*
================
idActor::LinkCombat
================
*/
void idActor::LinkCombat( void ) {
	idAFAttachment *headEnt;

	if ( fl.hidden || use_combat_bbox ) {
		return;
	}

	if ( combatModel ) {
		combatModel->Link( gameLocal.clip, this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->LinkCombat();
	}
}

/*
================
idActor::UnlinkCombat
================
*/
void idActor::UnlinkCombat( void ) {
	idAFAttachment *headEnt;

	if ( combatModel != NULL ) {
		combatModel->Unlink();
	}
	headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->UnlinkCombat();
	}
}

/*
================
idActor::StartRagdoll
================
*/
bool idActor::StartRagdoll( void ) {
	float slomoStart, slomoEnd;
	float jointFrictionDent, jointFrictionDentStart, jointFrictionDentEnd;
	float contactFrictionDent, contactFrictionDentStart, contactFrictionDentEnd;

	// if no AF loaded
	if ( !af.IsLoaded() ) {
		return false;
	}

	// if the AF is already active
	if ( af.IsActive() ) {
		return true;
	}

	// disable the monster bounding box
	if (spawnArgs.GetBool("nonsolid_on_ragdoll", "1"))
	{
		GetPhysics()->DisableClip();
	}

	// ishtvan: Establish AF constraints for any AF bodies of bound entities
	UpdateAddedEntConstraints();

	// start using the AF
	af.StartFromCurrentPose( spawnArgs.GetInt( "velocityTime", "0" ) );

	slomoStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_slomoStart", "-1.6" );
	slomoEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_slomoEnd", "0.8" );

	// do the first part of the death in slow motion
	af.GetPhysics()->SetTimeScaleRamp( slomoStart, slomoEnd );

	jointFrictionDent = spawnArgs.GetFloat( "ragdoll_jointFrictionDent", "0.1" );
	jointFrictionDentStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_jointFrictionStart", "0.2" );
	jointFrictionDentEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_jointFrictionEnd", "1.2" );

	// set joint friction dent
	af.GetPhysics()->SetJointFrictionDent( jointFrictionDent, jointFrictionDentStart, jointFrictionDentEnd );

	contactFrictionDent = spawnArgs.GetFloat( "ragdoll_contactFrictionDent", "0.1" );
	contactFrictionDentStart = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_contactFrictionStart", "1.0" );
	contactFrictionDentEnd = MS2SEC( gameLocal.time ) + spawnArgs.GetFloat( "ragdoll_contactFrictionEnd", "2.0" );

	// set contact friction dent
	af.GetPhysics()->SetContactFrictionDent( contactFrictionDent, contactFrictionDentStart, contactFrictionDentEnd );

	// drop any items the actor is holding
	idMoveableItem::DropItems( this, "death", NULL );

	// drop any articulated figures the actor is holding
	idAFEntity_Base::DropAFs( this, "death", NULL );

	RemoveAttachments();

	return true;
}

/*
================
idActor::StopRagdoll
================
*/
void idActor::StopRagdoll( void ) {
	if ( af.IsActive() ) {
		af.Stop();
	}
}

/*
================
idActor::UpdateAnimationControllers
================
*/
bool idActor::UpdateAnimationControllers( void ) {

	if ( af.IsActive() ) {
		return idAFEntity_Base::UpdateAnimationControllers();
	} else {
		animator.ClearAFPose();
	}

	if ( walkIK.IsInitialized() ) {
		walkIK.Evaluate();
		return true;
	}

	return false;
}

/*
================
idActor::RemoveAttachments
================
*/
void idActor::RemoveAttachments( void ) 
{
	int i;
	idEntity *ent;

	// remove any attached entities
	for( i = 0; i < m_Attachments.Num(); i++ ) {
		ent = m_Attachments[ i ].ent.GetEntity();
		if ( ent && ent->spawnArgs.GetBool( "remove" ) ) {
			ent->PostEventMS( &EV_SafeRemove, 0 );
		}
	}
}

/*
================
idActor::Attach
================
*/
void idActor::Attach( idEntity *ent, const char *PosName, const char *AttName )
{
	idAnimatedEntity::Attach( ent, PosName, AttName );

	// If the ent we're attaching is an AFAttachment, call SetBody to set up damage propagation, physics propagation, etc.
	// NOTE: We read ent->GetBindJoint here, assuming the bind went okay.
	if( ent->IsType(idAFAttachment::Type) )
	{
		// TODO: Is this line correct?  Won't know until we test.
		idStr modelName = ent->spawnArgs.GetString("model","");
		static_cast<idAFAttachment *>(ent)->SetBody( this, modelName.c_str(), ent->GetBindJoint() );
	}

	// check various spawnargs for special behaviors on attaching (not frobable, contents corpse, etc)
	if( ent->spawnArgs.GetBool("on_attach_contents_corpse") )
	{
		// clear solid contents, set corpse contents
		int oldContents = ent->GetPhysics()->GetContents();
		ent->GetPhysics()->SetContents( (oldContents & ~CONTENTS_SOLID) | CONTENTS_CORPSE );
	}
	if( ent->spawnArgs.GetBool("on_attach_nonsolid") )
	{
		// clear solid and corpse contents
		int oldContents = ent->GetPhysics()->GetContents();
		ent->GetPhysics()->SetContents( (oldContents & ~CONTENTS_SOLID) & ~CONTENTS_CORPSE );
	}
	if( ent->spawnArgs.GetBool("on_attach_not_frobable") )
		ent->SetFrobable(false);


	if( ent->IsType(CMeleeWeapon::Type) )
	{
		static_cast<CMeleeWeapon *>(ent)->AttachedToActor( this );
	}
}

/*
================
idActor::BindNotify
================
*/
void idActor::BindNotify( idEntity *ent , const char *jointName) // grayman #3074
{
	idAFEntity_Base::BindNotify(ent, jointName); // grayman #3074

	// Override our animations based on the bound entity's replace_anim_* spawnargs
	LoadReplacementAnims(ent->spawnArgs,jointName); // grayman #3074
}

/*
================
idActor::UnbindNotify
================
*/
void idActor::UnbindNotify( idEntity *ent )
{
	idAFEntity_Base::UnbindNotify(ent);

	// Remove animation overrides
	const idKeyValue *KeyVal = ent->spawnArgs.MatchPrefix( "replace_anim_", NULL );
	while ( KeyVal )
	{
		idStr key = KeyVal->GetKey();
		key.StripLeadingOnce("replace_anim_");

		// grayman #3166
		// key holds "<replacedAnimation>[ jointName]"
		// Separate the substrings before and after the space.

		// Does key contain a space, which indicates two substrings?

		idStr replacedAnimation = key;
		idStr replacingAnimation = KeyVal->GetValue();

		int index = idStr::FindChar(key.c_str(),' ');
		if ( index > 0 )
		{
			std::string keyString = key.c_str();
			std::vector<std::string> substrings; // will hold the separated substrings
			stdext::split(substrings, keyString, " ");
			replacedAnimation = idStr(substrings[0].c_str());
			// ignore substrings[1] because we don't need it
		}

		if (strcmp(m_replacementAnims.GetString( replacedAnimation ), replacingAnimation.c_str()) == 0 )
		{
			// This animation override is present, so remove it
			//gameLocal.Warning( "idActor: Removing replacement animation %s", KeyVal->GetValue().c_str() );
			m_replacementAnims.Delete( replacedAnimation );
		}

		KeyVal = ent->spawnArgs.MatchPrefix( "replace_anim_", KeyVal );
	}

	// angua: remove from attachments
	for (int i = 0; i < m_Attachments.Num(); i++)
	{
		idEntity* attachment = m_Attachments[i].ent.GetEntity();

		if (attachment != NULL && attachment->name == ent->name)
		{
			m_Attachments[i].ent = NULL;
		}
	}
}

/*
================
idActor::Teleport
================
*/
void idActor::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {

	GetPhysics()->SetLinearVelocity( vec3_origin );
	if (destination == NULL)
	{
		GetPhysics()->SetOrigin( origin + idVec3( 0, 0, CM_CLIP_EPSILON ) );
		viewAxis = angles.ToMat3();
	}
	else
	{
		GetPhysics()->SetOrigin( destination->GetPhysics()->GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );
		viewAxis = destination->GetPhysics()->GetAxis();
	}

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
idActor::HasEnemies
================
*/
bool idActor::HasEnemies( void ) const {
	idActor *ent;

	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( !ent->fl.hidden ) {
			return true;
		}
	}

	return false;
}

/*
================
idActor::ClosestEnemyToPoint
================
*/
idActor *idActor::ClosestEnemyToPoint( const idVec3 &pos ) {
	idActor		*ent;
	idActor		*bestEnt;
	float		bestDistSquared;
	float		distSquared;
	idVec3		delta;

	bestDistSquared = idMath::INFINITY;
	bestEnt = NULL;
	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( ent->fl.hidden ) {
			continue;
		}
		delta = ent->GetPhysics()->GetOrigin() - pos;
		distSquared = delta.LengthSqr();
		if ( distSquared < bestDistSquared ) {
			bestEnt = ent;
			bestDistSquared = distSquared;
		}
	}

	return bestEnt;
}

/*
================
idActor::EnemyWithMostHealth
================
*/
idActor *idActor::EnemyWithMostHealth() {
	idActor		*ent;
	idActor		*bestEnt;

	int most = -9999;
	bestEnt = NULL;
	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( !ent->fl.hidden && ( ent->health > most ) ) {
			bestEnt = ent;
			most = ent->health;
		}
	}
	return bestEnt;
}

/*
================
idActor::ClosestAttackingEnemy
================
*/
idActor *idActor::ClosestAttackingEnemy( bool bUseFOV )
{
	idActor		*ent(NULL);
	idActor		*bestEnt(NULL);
	float		bestDistSquared(idMath::INFINITY);
	float		distSquared(0.0f);
	idVec3		delta;

	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) 
	{
		if ( ent->fl.hidden )
			continue;

		CMeleeStatus *pStatus = &ent->m_MeleeStatus;
		// TODO: Differentiate between phases of the action state, holding attack, etc?
		if ( !(pStatus->m_ActionState == MELEEACTION_ATTACK) )
			continue;

		idVec3 entOrigin = ent->GetPhysics()->GetOrigin();
		delta = entOrigin - GetPhysics()->GetOrigin();
		distSquared = delta.LengthSqr();

		// check FOV, using idActor version which only checks horizontal angle
		if( bUseFOV && !idActor::CheckFOV( entOrigin) )
		{
			continue;
		}

		if ( distSquared < bestDistSquared ) 
		{
			bestEnt = ent;
			bestDistSquared = distSquared;
		}
	}

	return bestEnt;
}

/*
================
idActor::GetBestParry
================
*/
EMeleeType idActor::GetBestParry( void )
{
	idActor *AttEnemy;
	EMeleeType ParryType;

	if( m_MeleeStatus.m_bCanParryAll )
		ParryType = MELEETYPE_BLOCKALL;
	else if( (AttEnemy = ClosestAttackingEnemy( true )) != NULL
			&& AttEnemy->m_MeleeStatus.m_ActionType != MELEETYPE_UNBLOCKABLE )
	{
		ParryType = AttEnemy->m_MeleeStatus.m_ActionType;
	}
	else
		ParryType = MELEETYPE_RL;

	return ParryType;
}

/*
================
idActor::OnLadder
================
*/
bool idActor::OnLadder( void ) const {
	return false;
}

CMultiStateMover* idActor::OnElevator(bool mustBeMoving) const
{
	return NULL;
}

/*
==============
idActor::GetAASLocation
==============
*/
void idActor::GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const {
	idVec3		size;
	idBounds	bounds;

	GetFloorPos( 64.0f, pos );
	if ( !aas ) {
		areaNum = 0;
		return;
	}
	
	size = aas->GetSettings()->boundingBoxes[0][1];
	bounds[0] = -size;
	size.z = 32.0f;
	bounds[1] = size;

	areaNum = aas->PointReachableAreaNum( pos, bounds, AREA_REACHABLE_WALK );
	if ( areaNum ) {
		aas->PushPointIntoAreaNum( areaNum, pos );
	}
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
	const function_t *func;

// greebo: Try to lookup the script function of this animstate
	func = scriptObject.GetFunction( statename );
	if ( !func ) {
//		assert( 0 ); // greebo: don't just crash, a missing script function can happen...
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.SetState( statename, blendFrames );
		allowEyeFocus = true;
		break;
		
	case ANIMCHANNEL_TORSO :
		torsoAnim.SetState( statename, blendFrames );
		legsAnim.Enable( blendFrames );
		allowPain = true;
		allowEyeFocus = true;
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.SetState( statename, blendFrames );
		torsoAnim.Enable( blendFrames );
		allowPain = true;
		allowEyeFocus = true;
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
	case ANIMCHANNEL_HEAD :
		return headAnim.state;
		break;

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
	case ANIMCHANNEL_HEAD :
		if ( headAnim.state == statename ) {
			return true;
		}
		break;

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
idActor::WaitState
=====================
*/
const char *idActor::WaitState( void ) const {
	if ( waitState.Length() ) {
		return waitState;
	} else {
		return NULL;
	}
}


const char *idActor::WaitState( int channel ) const 
{
	switch( channel ) 
	{
		case ANIMCHANNEL_HEAD :
			return headAnim.WaitState();
			break;

		case ANIMCHANNEL_TORSO :
			return torsoAnim.WaitState();			
			break;

		case ANIMCHANNEL_LEGS :
			return legsAnim.WaitState();
			break;

		default:
			gameLocal.Error( "idActor::WaitState: Unknown anim group" );
			break;
	}

	return NULL;
}



/*
=====================
idActor::SetWaitState
=====================
*/
void idActor::SetWaitState( const char *_waitstate ) 
{
	if ( ai_debugScript.GetInteger() == entityNumber ) // #4057
	{
		gameLocal.Printf( "%d: idActor::SetWaitState \"%s\" --> \"%s\"\n", gameLocal.time, waitState.c_str(), _waitstate );
	}

	waitState = _waitstate;
}

void idActor::SetWaitState(int channel, const char *_waitstate)
{
	switch( channel ) 
	{
		case ANIMCHANNEL_HEAD :
			headAnim.SetWaitState(_waitstate);
			break;

		case ANIMCHANNEL_TORSO :
			torsoAnim.SetWaitState(_waitstate);
			break;

		case ANIMCHANNEL_LEGS :
			legsAnim.SetWaitState(_waitstate);
			break;

		default:
			gameLocal.Error( "idActor::SetWaitState: Unknown anim group" );
			break;
	}

}

/*
=====================
idActor::UpdateAnimState
=====================
*/
void idActor::UpdateAnimState( void ) {
	headAnim.UpdateState();
	torsoAnim.UpdateState();
	legsAnim.UpdateState();
}

/*
=====================
idActor::GetAnim
=====================
*/
int idActor::GetAnim( int channel, const char *animname ) {
	int			anim;
	const char *temp;
	idAnimator *animatorPtr;

	if ( channel == ANIMCHANNEL_HEAD ) {
		if ( !head.GetEntity() ) {
			return 0;
		}
		animatorPtr = head.GetEntity()->GetAnimator();
	} else {
		animatorPtr = &animator;
	}

	if ( animPrefix.Length() )
	{
		temp = va( "%s_%s", animPrefix.c_str(), animname );

		const char* replacement = LookupReplacementAnim( temp );

		if (cv_ai_debug_anims.GetBool() && this != gameLocal.GetLocalPlayer() && idStr::Cmp(replacement, temp) != 0)
		{
			gameLocal.Printf("Frame: %d - replacing %s with %s\n", gameLocal.framenum, animname, replacement);
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Frame: %d - replacing %s with %s\r", gameLocal.framenum, animname, replacement);
		}

		anim = animatorPtr->GetAnim( replacement );
		if (!anim) anim = animatorPtr->GetAnim( temp );
		if (anim) {
			return anim;
		}
	}

	const char* replacement = LookupReplacementAnim( animname );

	if (cv_ai_debug_anims.GetBool() && this != gameLocal.GetLocalPlayer() && idStr::Cmp(replacement, animname) != 0)
	{
		gameLocal.Printf("Frame: %d - replacing %s with %s\n", gameLocal.framenum, animname, replacement);
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Frame: %d - replacing %s with %s\r", gameLocal.framenum, animname, replacement);
	}

	anim = animatorPtr->GetAnim( replacement );
	if (!anim)
	{
		anim = animatorPtr->GetAnim( animname );
	}

	return anim;
}

idAnimator*	idActor::GetAnimatorForChannel(int channel)
{
	switch (channel) 
	{
		case ANIMCHANNEL_HEAD :
			if (head.GetEntity() == NULL)
			{
				return headAnim.GetAnimator();
			}
			return head.GetEntity()->GetAnimator();
			break;

		case ANIMCHANNEL_TORSO :
			return torsoAnim.GetAnimator();
			break;

		case ANIMCHANNEL_LEGS :
			return legsAnim.GetAnimator();
			break;

		default:
			gameLocal.Error("GetAnimatorForChannel: Unknown anim group");
			break;
	};

	return NULL;
}

/*
=====================
idActor::LookupReplacementAnim
(TDM)
=====================
*/
const char* idActor::LookupReplacementAnim( const char *animname )
{
	// Recursively lookup the animation to find its replacement animation
	const char* replacement = animname;
	int tries = 0; // Infinite loop prevention counter

	while ( m_replacementAnims.FindKey( replacement ) )
	{
		replacement = m_replacementAnims.GetString( replacement );
		
		// Avoid infinite loops
		if (++tries > 50)
		{
			gameLocal.Warning("Infinite loop detected in replacements for animation '%s' applied to actor '%s'\n",
				animname, this->name.c_str());
			break;
		}
	}

	return replacement;
}

void idActor::LoadReplacementAnims(const idDict& spawnArgs, const char *attached2Joint) // grayman #3074
{
	// grayman #3074 - Change to syntax of replacement animations.
	//
	// "replace_anim_<replacedAnimation>[ jointName]" "<replacingAnimation>"
	//
	// Only replace the animation if the joint being attached to matches
	// the "jointName" defined in the replace_anim, or if jointName being
	// attached to is NULL.

	for (const idKeyValue* kv = spawnArgs.MatchPrefix("replace_anim_", NULL);
		 kv != NULL; kv = spawnArgs.MatchPrefix("replace_anim_", kv))
	{
		idStr key = kv->GetKey();
		key.StripLeadingOnce("replace_anim_");

		// key holds "<replacedAnimation>[ jointName]"
		// Separate the substrings before and after the space.

		// Does key contain a space, which indicates two substrings?

		int index = idStr::FindChar(key.c_str(),' ');
		if ( index > 0 )
		{
			std::string keyString = key.c_str();
			std::vector<std::string> substrings; // will hold the separated substrings
			stdext::split(substrings, keyString, " ");
			idStr replacedAnimation = idStr(substrings[0].c_str());
			const char *jointName = substrings[1].c_str();

			// make the animation replacement if the joint listed for this replacement
			// matches the joint the entity was attached to (attached2Joint), or if
			// attached2Joint is NULL

			if ( ( attached2Joint == NULL ) || ( idStr::Icmp(jointName, attached2Joint) == 0 ) )
			{
				SetReplacementAnim(replacedAnimation, kv->GetValue());
			}
		}
		else
		{
			SetReplacementAnim(key, kv->GetValue());
		}
	}
}

void idActor::SetReplacementAnim(const idStr& animToReplace, const idStr& replacementAnim)
{
	m_replacementAnims.Set(animToReplace, replacementAnim);
}

void idActor::RemoveReplacementAnim(const idStr& replacedAnim)
{
	m_replacementAnims.Delete(replacedAnim);
}

void idActor::StopAnim(int channel, int frames) 
{
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.StopAnim( frames );
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.StopAnim( frames );
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.StopAnim( frames );
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
		break;
	}
}

/*
===============
idActor::SyncAnimChannels
===============
*/
void idActor::SyncAnimChannels( int channel, int syncToChannel, int blendFrames ) {
	idAnimator		*headAnimator;
	idAFAttachment	*headEnt;
	int				anim;
	idAnimBlend		*syncAnim;
	int				starttime;
	int				blendTime;
	int				cycle;

	blendTime = FRAME2MS( blendFrames );
	if ( channel == ANIMCHANNEL_HEAD ) {
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnimator = headEnt->GetAnimator();
			syncAnim = animator.CurrentAnim( syncToChannel );
			if ( syncAnim ) {
				anim = headAnimator->GetAnim( syncAnim->AnimFullName() );
				if ( !anim ) {
					anim = headAnimator->GetAnim( syncAnim->AnimName() );
				}
				if ( anim ) {
					cycle = animator.CurrentAnim( syncToChannel )->GetCycleCount();
					starttime = animator.CurrentAnim( syncToChannel )->GetStartTime();
					headAnimator->PlayAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, blendTime );
					headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( cycle );
					headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->SetStartTime( starttime );
				} else {
					headEnt->PlayIdleAnim( blendTime );
				}
			}
		}
	} else if ( syncToChannel == ANIMCHANNEL_HEAD ) {
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnimator = headEnt->GetAnimator();
			syncAnim = headAnimator->CurrentAnim( ANIMCHANNEL_ALL );
			if ( syncAnim ) {
				anim = GetAnim( channel, syncAnim->AnimFullName() );
				if ( !anim ) {
					anim = GetAnim( channel, syncAnim->AnimName() );
				}
				if ( anim ) {
					cycle = headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->GetCycleCount();
					starttime = headAnimator->CurrentAnim( ANIMCHANNEL_ALL )->GetStartTime();
					animator.PlayAnim( channel, anim, gameLocal.time, blendTime );
					animator.CurrentAnim( channel )->SetCycleCount( cycle );
					animator.CurrentAnim( channel )->SetStartTime( starttime );
				}
			}
		}
	} else {
		animator.SyncAnimChannels( channel, syncToChannel, gameLocal.time, blendTime );
	}
}

/***********************************************************************

	Damage

***********************************************************************/

/*
============
idActor::Gib
============
*/
void idActor::Gib( const idVec3 &dir, const char *damageDefName ) {
	// only gib once
	if ( gibbed ) {
		return;
	}
	idAFEntity_Gibbable::Gib( dir, damageDefName );
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}
	StopSound( SND_CHANNEL_VOICE, false );
}


/*
============
idActor::Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted

collision	trace info for the collision that caused the damage.  Defaults to NULL.

inflictor, attacker, dir, and point can be NULL for environmental effects

Bleeding wounds and surface overlays are applied in the collision code that calls Damage()
============
*/
void idActor::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, const int location,
					  trace_t *collision ) 
{
	if (collision != NULL)
	{
		int bodID = BodyForClipModelId( collision->c.id );
		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Fun is trying to call getBody with bodyID %d\r", bodID );
		idAFBody* StruckBody = GetAFPhysics()->GetBody( bodID );
		
		if ( StruckBody != NULL )
		{
			DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Struck body %s\r",StruckBody->GetName().c_str());
			idEntity* reroute = StruckBody->GetRerouteEnt();
			if (reroute != NULL) 
			{
				DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Rerouting damage from the AF of entity %s to bound entity %s\r",name.c_str(), reroute->name.c_str());
				// TODO: Technically location is wrong here, it's a joint that's not on the reroute entity (not sure if it will matter)
				reroute->Damage( inflictor, attacker, dir, damageDefName, damageScale, location, collision );
				return;
			}
		}
	}

	if ( !fl.takedamage )
	{
		return;
	}

	// grayman #2478 - why take more damage if you're already dead?

	if (health <= 0)
	{
		return;
	}

	if ( !inflictor )
	{
		inflictor = gameLocal.world;
	}

	if ( !attacker )
	{
		attacker = gameLocal.world;
	}

	// grayman #3679 - if I'm an AI, remember who attacked me (could be used by death scripts)

	if ( IsType(idAI::Type) )
	{
		static_cast<idAI*>(this)->GetMemory().attacker = attacker;
	}

	// Try to find the damage entityDef
	const idDict* damageDef = gameLocal.FindEntityDefDict( damageDefName, true ); // grayman #3391 - don't create a default 'damageDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if ( !damageDef )
	{
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	// grayman #2816 - hit by melee weapon or moveable?

	bool hitByMelee = false;
	bool hitByMoveable = false;
	float mass = 1.0f;
	if ( inflictor->IsType(CMeleeWeapon::Type) && ( attacker != gameLocal.world ) && ( inflictor->GetBindMaster() != NULL ) )
	{
		hitByMelee = true;
	}
	else if ( inflictor->IsType(idMoveable::Type) )
	{
		mass = inflictor->spawnArgs.GetFloat("mass","1");
		hitByMoveable = true;
	}
	
	int damage;

	// grayman #2816 - scale damage?

	if ( hitByMoveable ) // Moveable
	{
		// grayman #3358 - if the moveable has a "damage" spawnarg, use that.
		// This lets mappers override the damage def value for damage.
		damage = inflictor->spawnArgs.GetInt("damage","0");
		if ( damage == 0 )
		{
			// If not, use the "damage" value from the damage def
			damage = static_cast<int>(damageDef->GetInt("damage"));
		}
		damage *= static_cast<int>(mass / 5.0f); // grayman #3358
	}
	else
	{
		damage = static_cast<int>(damageDef->GetInt("damage")); // grayman #3358
	}

	damage *= damageScale;

	damage = GetDamageForLocation( damage, location );

	// apply stealth damage multiplier - only active for derived AI class
	damage *= StealthDamageMult();

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );

	// DarkMod: check for KO damage type and knockout AI if appropriate
	bool bKO = damageDef->GetBool( "knockout" );
	bool bKOPowerBlow = damageDef->GetBool( "knockout_power" );

	if ( ( bKO || bKOPowerBlow ) && collision )
	{
		// Objects with enough mass and traveling fast enough will force a KO.

		if ( hitByMoveable )
		{
			idVec3 velocity = inflictor->GetPhysics()->GetLinearVelocity();

			// The normal of the struck surface and the velocity of
			// the striking object can sometimes indicate that the
			// object was moving AWAY from the struck surface at the
			// time of collision. In those cases, normalSpeed will
			// be negative, so we'll just flip its sign so we get a
			// result that's usable when trying to determine if this
			// is a potential KO blow. After all, the object DID strike.

			float normalSpeed = -collision->c.normal*velocity;
			bool canKO = ( mass*normalSpeed >= 1000 );

			// If my mass is small enough, and canKO is TRUE, the object
			// should kill me, regardless of how much damage I'm about to
			// receive, and what my health is.

			if ( canKO && ( GetPhysics()->GetMass() <= MIN_MASS_FOR_KO ) )
			{
				damage = health; // kill me
				canKO = false;	 // don't bother with the KO
			}

			if ( canKO )
			{
				// If the object is falling, we can force a KO if it hit an actor's head.
				// If it's not falling, we should test if the blow hit the right place
				// w/in the actor's KO zone, as we normally would with a blackjack blow.

				idVec2 horzVelocity = velocity.ToVec2();
				float horzSpeed = horzVelocity.LengthFast();
				float vertDelta = collision->endpos.z - GetEyePosition().z;

				if ( ( velocity.z < 0 ) && ( vertDelta > 0 ) && ( abs(velocity.z) > horzSpeed ) ) // falling?
				{
					// Bypass "determined immunity"/alertness/helmet checks and force a KO.
					// Two situations can prevent a forced KO:
					// 1 - m_bCanBeKnockedOut is true ("immune_ko" is set in the *.def file)
					// 2 - m_KoZone is "" ("ko_zone" is set to "" in the *.def file)

					if ( IsType(idAI::Type) ) // should always be true, since the player processes his own damage, but follow convention ...
					{
						idAI* meAI = static_cast<idAI*>(this);

						if ( meAI->m_bCanBeKnockedOut && !meAI->m_KoZone.IsEmpty() )
						{
							meAI->Event_KO_Knockout( attacker );
							damage = 0; // KO does no health damage.
						}
						else
						{
							// If there's enough impulse to force a KO, but the *.def file
							// says we're immune to KOs, apply at least a small amount of damage
							// equivalent to what we'd get from a blackjack blow

							if ( damage == 0 )
							{
								damage += 2;
							}
						}
					}
				}
				else // object is coming in from the side
				{
					if ( TestKnockoutBlow( attacker, dir, collision, location, bKOPowerBlow ) )
					{
						// For now, first KO blow does no health damage
						damage = 0;
					}
				}
			}
		}
		else if ( hitByMelee && !inflictor->m_droppedByAI ) // grayman #2816
		{
			if ( TestKnockoutBlow( attacker, dir, collision, location, bKOPowerBlow ) )
			{
				// For now, first KO blow does no health damage
				damage = 0;
			}
		}
	}

	// grayman #2816 - when hit by a moveable, either thrown or dropped,
	// don't let the AI pass away unless it has a small mass.

	// grayman #3358 - if "kill_all" is "1", bypass this adjustment to the damage

	if ( damage > 0 )
	{
		if ( hitByMoveable &&
			 !( damageDef->GetBool( "kill_all", "0" ) ) &&
			 ( damage >= health ) &&
			 ( GetPhysics()->GetMass() > MIN_MASS_FOR_KO ) )
		{
			damage = health / 2; // damage falls to 0 when health = 1, otherwise take half their health
		}
	}
	
	if ( damage > 0 )
	{
		// Apply the damage

		health -= damage; // ouch!!

		DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Actor %s received damage %d at joint %d, corresponding to damage group %s\r", name.c_str(), damage, (int) location, GetDamageGroup(location) );
		if ( ( lowHealthThreshold != -1 ) && ( health <= lowHealthThreshold ) )
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Actor %s's fell below health threshold %d, firing script %s\r", name.c_str(), lowHealthThreshold, lowHealthScript.c_str());

			if ( !lowHealthScript.IsEmpty() )
			{
				CallScriptFunctionArgs(lowHealthScript, true, 0, "e", this);
			}
		}

		if ( health <= 0 ) 
		{
			if ( health < -999 ) 
			{
				health = -999;
			}
			Killed( inflictor, attacker, damage, dir, location );
			if ( ( health < -20 ) && spawnArgs.GetBool( "gib" ) && damageDef->GetBool( "gib" ) ) 
			{
				Gib( dir, damageDefName );
			}
		} 
		else
		{
			Pain( inflictor, attacker, damage, dir, location, damageDef );

			// FIX: if drowning, stop pain SFX and play drown SFX on voice channel
			if ( damageDef->GetBool( "no_air" ) ) 
			{
				StopSound( SND_CHANNEL_VOICE, false );
				SetSoundVolume(-10); // grayman #1488 - less volume underwater
				StartSound( "snd_drowning", SND_CHANNEL_VOICE, 0, false, NULL ); // grayman #3291
			}
		}
	}
	// Ishtvan: Try commenting this out, it doesn't make sense to ignore nonzero "push" in the DmgDef just
	// because the attack happens to hit armor and do no damage...
	// Cleary Id was trying to fix something here, but I'm not sure what
/*
	else 
	{
		// Ishtvan: THIS IS WHAT'S CAUSING PLATE ARMOR HITS NOT TO MOVE AI... WHY DID ID DO THIS?

		// don't accumulate knockback
		if ( af.IsLoaded() ) 
		{
			// clear impacts
			af.Rest();

			// physics is turned off by calling af.Rest()
			BecomeActive( TH_PHYSICS );
		}
	}
*/
}

/*
=====================
idActor::ClearPain
=====================
*/
void idActor::ClearPain( void ) {
	pain_debounce_time = 0;
}

/*
=====================
idActor::Pain
=====================
*/
bool idActor::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const idDict* damageDef ) {
	if ( af.IsLoaded() && !IsKnockedOut() )
	{
		// clear impacts
		af.Rest();

		// physics is turned off by calling af.Rest()
		BecomeActive( TH_PHYSICS );
	}

	if ( gameLocal.time < pain_debounce_time )
	{
		return false;
	}

	// grayman #2816 - no pain sounds or animation if knocked out

	if ( IsKnockedOut() )
	{
		return false;
	}

	// don't play pain sounds more than necessary
	pain_debounce_time = gameLocal.time + pain_delay;

	if (damageDef != NULL && damageDef->FindKey("snd_damage") != NULL)
	{
		// The damage def defines a special damage sound, use that one
		// Copy it into our own spawnargs for use with StartSound() routine
		spawnArgs.Set("snd_damage_internal___", damageDef->GetString("snd_damage"));

		StartSound( "snd_damage_internal___", SND_CHANNEL_VOICE, 0, false, NULL );
	}
	else
	{
		// Ordinary health-based pain sound
		if ( health > 75  ) {
			StartSound( "snd_pain_small", SND_CHANNEL_VOICE, 0, false, NULL );
		} else if ( health > 50 ) {
			StartSound( "snd_pain_medium", SND_CHANNEL_VOICE, 0, false, NULL );
		} else if ( health > 25 ) {
			StartSound( "snd_pain_large", SND_CHANNEL_VOICE, 0, false, NULL );
		} else {
			StartSound( "snd_pain_huge", SND_CHANNEL_VOICE, 0, false, NULL );
		}
	}

	if ( !allowPain || ( gameLocal.time < painTime ) ) {
		// don't play a pain anim
		return false;
	}

	if ( pain_threshold && ( damage < pain_threshold ) ) {
		return false;
	}

	// set the pain anim
	idStr damageGroup = GetDamageGroup( location );

	painAnim = "";
	if ( animPrefix.Length() ) {
		if ( damageGroup.Length() && ( damageGroup != "legs" ) ) {
			sprintf( painAnim, "%s_pain_%s", animPrefix.c_str(), damageGroup.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				sprintf( painAnim, "pain_%s", damageGroup.c_str() );
				if ( !animator.HasAnim( painAnim ) ) {
					painAnim = "";
				}
			}
		}

		if ( !painAnim.Length() ) {
			sprintf( painAnim, "%s_pain", animPrefix.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				painAnim = "";
			}
		}
	} else if ( damageGroup.Length() && ( damageGroup != "legs" ) ) {
		sprintf( painAnim, "pain_%s", damageGroup.c_str() );
		if ( !animator.HasAnim( painAnim ) ) {
			sprintf( painAnim, "pain_%s", damageGroup.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				painAnim = "";
			}
		}
	}

	if ( !painAnim.Length() ) {
		painAnim = "pain";
	}

	if ( g_debugDamage.GetBool() ) {
		gameLocal.Printf( "Damage: joint: '%s', zone '%s', anim '%s'\n", animator.GetJointName( ( jointHandle_t )location ), 
			damageGroup.c_str(), painAnim.c_str() );
	}

	return true;
}

void idActor::SetIsPushing(bool isPushing)
{
	// do nothing, is handled by the subclasses
}

bool idActor::IsPushing()
{
	return false;
}

/*
=====================
idActor::SpawnGibs
=====================
*/
void idActor::SpawnGibs( const idVec3 &dir, const char *damageDefName ) {
	idAFEntity_Gibbable::SpawnGibs( dir, damageDefName );
	RemoveAttachments();
}

/*
=====================
idActor::SetupDamageGroups

FIXME: only store group names once and store an index for each joint
=====================
*/
void idActor::SetupDamageGroups( void ) {
	int						i;
	const idKeyValue		*arg;
	idStr					groupname;
	idList<jointHandle_t>	jointList;
	int						jointnum;
	float					scale;

	// create damage zones
	damageGroups.SetNum( animator.NumJoints() );
	arg = spawnArgs.MatchPrefix( "damage_zone ", NULL );
	while ( arg ) {
		groupname = arg->GetKey();
		groupname.Strip( "damage_zone " );
		animator.GetJointList( arg->GetValue(), jointList );
		for( i = 0; i < jointList.Num(); i++ ) {
			jointnum = jointList[ i ];
			damageGroups[ jointnum ] = groupname;
		}
		jointList.Clear();
		arg = spawnArgs.MatchPrefix( "damage_zone ", arg );
	}

	// initilize the damage zones to normal damage
	damageScale.SetNum( animator.NumJoints() );
	for( i = 0; i < damageScale.Num(); i++ ) {
		damageScale[ i ] = 1.0f;
	}

	// set the percentage on damage zones
	arg = spawnArgs.MatchPrefix( "damage_scale ", NULL );
	while ( arg ) {
		scale = atof( arg->GetValue() );
		groupname = arg->GetKey();
		groupname.Strip( "damage_scale " );
		for( i = 0; i < damageScale.Num(); i++ ) {
			if ( damageGroups[ i ] == groupname ) {
				damageScale[ i ] = scale;
			}
		}
		arg = spawnArgs.MatchPrefix( "damage_scale ", arg );
	}
}

/*
=====================
idActor::GetDamageForLocation
=====================
*/
int idActor::GetDamageForLocation( int damage, int location ) {
	if ( ( location < 0 ) || ( location >= damageScale.Num() ) ) {
		return damage;
	}

	return (int)ceil( damage * damageScale[ location ] );
}

/*
=====================
idActor::GetDamageGroup
=====================
*/
const char *idActor::GetDamageGroup( int location ) {
	if ( ( location < 0 ) || ( location >= damageGroups.Num() ) ) {
		return "";
	}

	return damageGroups[ location ];
}

/*
====================
idActor::GetDamageLocation
====================
*/
int idActor::GetDamageLocation(idStr name)
{
	for (int i = 0; i < damageGroups.Num(); i++)
	{
		if (damageGroups[i] == name)
		{
			return i;
		}
	}
	return -1;
}
/*
=====================
idActor::PlayFootStepSound
=====================
*/
void idActor::PlayFootStepSound( void ) 
{
	// empty, override this in the subclasses
}

void idActor::LinkScriptVariables()
{
	// Link the script variables to our script object
	AI_DEAD.LinkTo(scriptObject, "AI_DEAD");
}

bool idActor::ReEvaluateArea(int areaNum)
{
	// Default implementation for actors: return positive
	return true;
}

void idActor::LoadVocalSet()
{
	// Try to look up the entityDef
	idStr vocalSet = spawnArgs.GetString("def_vocal_set");

	// #6507 Amadeus - Check if AI is drunk to change vocal sets
	if (spawnArgs.GetBool("drunk", "0"))
	{
		vocalSet = (spawnArgs.GetString("def_vocal_set_drunk"));
	}

	if (vocalSet.IsEmpty())
	{
		m_isMute = true; // grayman #3202
		return;
	}

	const idDeclEntityDef* def = gameLocal.FindEntityDef(vocalSet, false);

	if (def == NULL)
	{
		gameLocal.Warning("%s - Could not find def_vocal_set %s!", name.c_str(),vocalSet.c_str());
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("%s - Could not find def_vocal_set %s!", name.c_str(),vocalSet.c_str());
		m_isMute = true; // grayman #3202
		return;
	}

	if ( idStr::FindText(vocalSet,"ai_vocal_set_mute") >= 0 ) // grayman #3202
	{
		m_isMute = true;
	}

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Copying vocal set %s to actor %s\r", vocalSet.c_str(), name.c_str());

	int i = 0;

	// Copy all snd_* spawnargs over to this entity
	for (const idKeyValue* kv = def->dict.MatchPrefix("snd_"); kv != NULL; kv = def->dict.MatchPrefix("snd_", kv), i++)
	{
		spawnArgs.Set(kv->GetKey(), kv->GetValue());
	}
	
	// Copy all sound prop "spr*" spawnargs over to this entity
	for (const idKeyValue* kv = def->dict.MatchPrefix("spr"); kv != NULL; kv = def->dict.MatchPrefix("spr", kv), i++)
	{
		spawnArgs.Set(kv->GetKey(), kv->GetValue());
	}

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Copied %d vocal set spawnargs to actor %s\r", i, name.c_str());
}

void idActor::LoadMeleeSet()
{
	idStr MeleeSet = spawnArgs.GetString("def_melee_set");
	if (MeleeSet.IsEmpty()) return; // nothing to do

	// tack on difficulty string
	// if we do it this way, def will not be precached, but that's okay for just some numbers?
	MeleeSet += va("_%s", cv_melee_difficulty.GetString());

	const idDeclEntityDef* def = gameLocal.FindEntityDef(MeleeSet, false);

	if (def == NULL)
	{
		gameLocal.Warning("%s - Could not find def_melee_set %s!", name.c_str(),MeleeSet.c_str());
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("%s - Could not find def_melee_set %s!\r", name.c_str(), MeleeSet.c_str());
		return;
	}

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Copying melee set %s to actor %s\r", MeleeSet.c_str(), name.c_str());

	// Copy ALL spawnargs from melee set over to this entity
	spawnArgs.Copy( def->dict );

	// re-cache the anim rates in case they changed
	CacheAnimRates();
}

/***********************************************************************

	Events

***********************************************************************/

/*
=====================
idActor::Event_EnableEyeFocus
=====================
*/
void idActor::Event_EnableEyeFocus( void ) {
	allowEyeFocus = true;
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomInt( blink_max - blink_min );
}

/*
=====================
idActor::Event_DisableEyeFocus
=====================
*/
void idActor::Event_DisableEyeFocus( void ) {
	allowEyeFocus = false;
	
	idEntity *headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->GetAnimator()->Clear( ANIMCHANNEL_EYELIDS, gameLocal.time, FRAME2MS( 2 ) );
	} else {
		animator.Clear( ANIMCHANNEL_EYELIDS, gameLocal.time, FRAME2MS( 2 ) );
	}
}

/*
===============
idActor::Event_Footstep
===============
*/
void idActor::Event_Footstep( void ) {
	PlayFootStepSound();
}

/*
=====================
idActor::Event_EnableWalkIK
=====================
*/
void idActor::Event_EnableWalkIK( void ) {
	walkIK.EnableAll();
}

/*
=====================
idActor::Event_DisableWalkIK
=====================
*/
void idActor::Event_DisableWalkIK( void ) {
	walkIK.DisableAll();
}

/*
=====================
idActor::Event_EnableLegIK
=====================
*/
void idActor::Event_EnableLegIK( int num ) {
	walkIK.EnableLeg( num );
}

/*
=====================
idActor::Event_DisableLegIK
=====================
*/
void idActor::Event_DisableLegIK( int num ) {
	walkIK.DisableLeg( num );
}

/*
=====================
idActor::Event_PreventPain
=====================
*/
void idActor::Event_PreventPain( float duration ) {
	painTime = gameLocal.time + SEC2MS( duration );
}

/*
===============
idActor::Event_DisablePain
===============
*/
void idActor::Event_DisablePain( void ) {
	allowPain = false;
}

/*
===============
idActor::Event_EnablePain
===============
*/
void idActor::Event_EnablePain( void ) {
	allowPain = true;
}

/*
=====================
idActor::Event_GetPainAnim
=====================
*/
void idActor::Event_GetPainAnim( void ) {
	if ( !painAnim.Length() ) {
		idThread::ReturnString( "pain" );
	} else {
		idThread::ReturnString( painAnim );
	}
}

/*
=====================
idActor::Event_SetAnimPrefix
=====================
*/
void idActor::Event_SetAnimPrefix( const char *prefix ) {
	animPrefix = prefix;
}

/*
===============
idActor::Event_StopAnim
===============
*/
void idActor::Event_StopAnim( int channel, int frames ) {
	StopAnim(channel, frames);
}

/*
===============
idActor::Event_PlayAnim
===============
*/
void idActor::Event_PlayAnim( int channel, const char *animname ) {
	animFlags_t	flags;
	idEntity *headEnt;
	int	anim;

	if (cv_ai_debug_anims.GetBool() && this != gameLocal.GetLocalPlayer())
	{
		gameLocal.Printf("Frame: %d - playing anim %s (%s)\n", gameLocal.framenum, animname, name.c_str());
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Frame: %d - playing anim %s (%s)\r", gameLocal.framenum, animname, name.c_str());
	}

	anim = GetAnim( channel, animname );
	if ( !anim ) {
		
#ifndef SUPPRESS_CONSOLE_WARNINGS
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.Printf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			//gameLocal.Printf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
#endif

		idThread::ReturnInt( 0 );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnim.idleAnim = false;
			headAnim.PlayAnim( anim );
			flags = headAnim.GetAnimFlags();
			if ( !flags.prevent_idle_override ) {
				if ( torsoAnim.IsIdle() ) {
					torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
					if ( legsAnim.IsIdle() ) {
						legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
						SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
					}
				}
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.idleAnim = false;
		torsoAnim.PlayAnim( anim );
		flags = torsoAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( headAnim.IsIdle() ) {
				headAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			if ( legsAnim.IsIdle() ) {
				legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.idleAnim = false;
		legsAnim.PlayAnim( anim );
		flags = legsAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				if ( headAnim.IsIdle() ) {
					headAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				}
			}
		}
		break;

	default :
		gameLocal.Error( "Unknown anim group" );
		break;
	}
	idThread::ReturnInt( 1 );
}

/*
===============
idActor::Event_PlayCycle
===============
*/
void idActor::Event_PlayCycle( int channel, const char *animname ) {
	animFlags_t	flags;
	int			anim;
	
	anim = GetAnim( channel, animname );
	if ( !anim ) {
		
#ifndef SUPPRESS_CONSOLE_WARNINGS
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.Printf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.Printf( "missing '%s' animation on '%s' (%s), channel %d\n", animname, name.c_str(), GetEntityDefName(),channel );
		}
#endif

		idThread::ReturnInt( false );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.idleAnim = false;
		headAnim.CycleAnim( anim );
		flags = headAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() && legsAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
				legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.idleAnim = false;
		torsoAnim.CycleAnim( anim );
		flags = torsoAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( headAnim.IsIdle() ) {
				headAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			if ( legsAnim.IsIdle() ) {
				legsAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.idleAnim = false;
		legsAnim.CycleAnim( anim );
		flags = legsAnim.GetAnimFlags();
		if ( !flags.prevent_idle_override ) {
			if ( torsoAnim.IsIdle() ) {
				torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				if ( headAnim.IsIdle() ) {
					headAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
				}
			}
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}

	idThread::ReturnInt( true );
}

/*
===============
idActor::Event_PauseAnim
===============
*/
void idActor::Event_PauseAnim( int channel, bool bPause )
{
	animator.CurrentAnim( channel )->Pause( bPause );
}

/*
===============
idActor::Event_AnimIsPaused
===============
*/
void idActor::Event_AnimIsPaused( int channel )
{
	idThread::ReturnInt( animator.CurrentAnim( channel )->IsPaused() );
}

/*
===============
idActor::Event_IdleAnim
===============
*/
void idActor::Event_IdleAnim( int channel, const char *animname ) {
	int anim;
	
	anim = GetAnim( channel, animname );	
	if ( !anim ) {

#ifndef SUPPRESS_CONSOLE_WARNINGS
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.Printf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.Printf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
#endif

		switch( channel ) {
		case ANIMCHANNEL_HEAD :
			headAnim.BecomeIdle();
			break;

		case ANIMCHANNEL_TORSO :
			torsoAnim.BecomeIdle();
			break;

		case ANIMCHANNEL_LEGS :
			legsAnim.BecomeIdle();
			break;

		default:
			gameLocal.Error( "Unknown anim group" );
		}

		idThread::ReturnInt( false );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.BecomeIdle();
		if ( torsoAnim.GetAnimFlags().prevent_idle_override ) {
			// don't sync to torso body if it doesn't override idle anims
			headAnim.CycleAnim( anim );
		} else if ( torsoAnim.IsIdle() && legsAnim.IsIdle() ) {
			// everything is idle, so play the anim on the head and copy it to the torso and legs
			headAnim.CycleAnim( anim );
			torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
			legsAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
		} else if ( torsoAnim.IsIdle() ) {
			// sync the head and torso to the legs
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, headAnim.animBlendFrames );
			torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, torsoAnim.animBlendFrames );
		} else {
			// sync the head to the torso
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, headAnim.animBlendFrames );
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.BecomeIdle();
		if ( legsAnim.GetAnimFlags().prevent_idle_override ) {
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

		if ( headAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.BecomeIdle();
		if ( torsoAnim.GetAnimFlags().prevent_idle_override ) {
			// don't sync to torso if torso anim doesn't override idle anims
			legsAnim.CycleAnim( anim );
		} else if ( torsoAnim.IsIdle() ) {
			// play the anim in both legs and torso
			legsAnim.CycleAnim( anim );
			torsoAnim.animBlendFrames = legsAnim.lastAnimBlendFrames;
			SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			if ( headAnim.IsIdle() ) {
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
			}
		} else {
			// sync the anim to the torso
			SyncAnimChannels( ANIMCHANNEL_LEGS, ANIMCHANNEL_TORSO, legsAnim.animBlendFrames );
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}

	idThread::ReturnInt( true );
}

/*
================
idActor::Event_SetSyncedAnimWeight
================
*/
void idActor::Event_SetSyncedAnimWeight( int channel, int anim, float weight ) {
	idEntity *headEnt;

	headEnt = head.GetEntity();
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		if ( headEnt ) {
			animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
		} else {
			animator.CurrentAnim( ANIMCHANNEL_HEAD )->SetSyncedAnimWeight( anim, weight );
		}
		if ( torsoAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
			if ( legsAnim.IsIdle() ) {
				animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
			}
		}
		break;

	case ANIMCHANNEL_TORSO :
		animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
		if ( legsAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
		}
		if ( headEnt && headAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
		}
		break;

	case ANIMCHANNEL_LEGS :
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
		if ( torsoAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
			if ( headEnt && headAnim.IsIdle() ) {
				animator.CurrentAnim( ANIMCHANNEL_ALL )->SetSyncedAnimWeight( anim, weight );
			}
		}
		break;

	default:
		gameLocal.Error( "Unknown anim group" );
	}
}

void idActor::Event_SyncAnimChannels(int fromChannel, int toChannel, float blendFrames)
{
	SyncAnimChannels(fromChannel, toChannel, static_cast<int>(blendFrames));
}

/*
===============
idActor::Event_OverrideAnim
===============
*/
void idActor::Event_OverrideAnim( int channel ) {
	switch ( channel ) {
	case ANIMCHANNEL_HEAD:
		headAnim.Disable();
		if ( !torsoAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		}
		else {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_TORSO:
		torsoAnim.Disable();
		legsAnim.Enable( legsAnim.lastAnimBlendFrames );  // #4063: make sure the channel we are sync'ing to is enabled.
		SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		if ( headAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_LEGS:
		legsAnim.Disable();
		torsoAnim.Enable( torsoAnim.lastAnimBlendFrames ); // #4063: make sure the channel we are sync'ing to is enabled.
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
	case ANIMCHANNEL_HEAD :
		headAnim.Enable( blendFrames );
		break;

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
idActor::Event_DisableAnimchannel
===============
*/
void idActor::Event_DisableAnimchannel( int channel ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.Disable();
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.Disable();
		break;

	case ANIMCHANNEL_LEGS :
		legsAnim.Disable();
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
	case ANIMCHANNEL_HEAD :
		headAnim.animBlendFrames = blendFrames;
		headAnim.lastAnimBlendFrames = blendFrames;
		break;

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
	case ANIMCHANNEL_HEAD :
		idThread::ReturnInt( headAnim.animBlendFrames );
		break;

	case ANIMCHANNEL_TORSO :
		idThread::ReturnInt( torsoAnim.animBlendFrames );
		break;

	case ANIMCHANNEL_LEGS :
		idThread::ReturnInt( legsAnim.animBlendFrames );
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
	const char *state;

	state = GetAnimState( channel );
	idThread::ReturnString( state );
}

/*
===============
idActor::Event_InAnimState
===============
*/
void idActor::Event_InAnimState( int channel, const char *statename ) {
	bool instate;

	instate = InAnimState( channel, statename );
	idThread::ReturnInt( instate );
}

/*
===============
idActor::Event_FinishAction
===============
*/
void idActor::Event_FinishAction( const char *actionname ) {

	if ( waitState == actionname ) {
		SetWaitState("");
	}
}

void idActor::Event_FinishChannelAction( int channel, const char *actionname)
{
	switch( channel ) 
	{
		case ANIMCHANNEL_HEAD :
			headAnim.FinishAction( actionname);
			break;

		case ANIMCHANNEL_TORSO :
			torsoAnim.FinishAction( actionname);
			break;

		case ANIMCHANNEL_LEGS :
			legsAnim.FinishAction( actionname );
			break;

		default:
			gameLocal.Error( "Unknown anim group" );
	}
}

bool idActor::AnimDone( int channel, int blendFrames ) const
{
	bool result = false;

	switch( channel ) {
		case ANIMCHANNEL_HEAD :
			result = headAnim.AnimDone( blendFrames );
			break;

		case ANIMCHANNEL_TORSO :
			result = torsoAnim.AnimDone( blendFrames );
			break;

		case ANIMCHANNEL_LEGS :
			result = legsAnim.AnimDone( blendFrames );
			break;

		default:
			gameLocal.Error( "Unknown anim group" );
	}

	return result;
}

/*
===============
idActor::Event_AnimDone
===============
*/
void idActor::Event_AnimDone( int channel, int blendFrames ) {
	idThread::ReturnInt(AnimDone(channel, blendFrames));
}

/*
================
idActor::Event_HasAnim
================
*/
void idActor::Event_HasAnim( int channel, const char *animname ) {
	if ( GetAnim( channel, animname ) ) {
		idThread::ReturnFloat( 1.0f );
	} else {
		idThread::ReturnFloat( 0.0f );
	}
}

/*
================
idActor::Event_CheckAnim
================
*/
void idActor::Event_CheckAnim( int channel, const char *animname ) {
	if ( !GetAnim( channel, animname ) ) {
		if ( animPrefix.Length() ) {
			gameLocal.Error( "Can't find anim '%s_%s' for '%s'", animPrefix.c_str(), animname, name.c_str() );
		} else {
			gameLocal.Error( "Can't find anim '%s' for '%s'", animname, name.c_str() );
		}
	}
}

/*
================
idActor::Event_ChooseAnim
================
*/
void idActor::Event_ChooseAnim( int channel, const char *animname ) {
	int anim;

	anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				idThread::ReturnString( head.GetEntity()->GetAnimator()->AnimFullName( anim ) );
				return;
			}
		} else {
			idThread::ReturnString( animator.AnimFullName( anim ) );
			return;
		}
	}

	idThread::ReturnString( "" );
}

/*
================
idActor::Event_AnimLength
================
*/
void idActor::Event_AnimLength( int channel, const char *animname ) {
	int anim;

	anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				idThread::ReturnFloat( MS2SEC( head.GetEntity()->GetAnimator()->AnimLength( anim ) ) );
				return;
			}
		} else {
			idThread::ReturnFloat( MS2SEC( animator.AnimLength( anim ) ) );
			return;
		}		
	}
	
	idThread::ReturnFloat( 0.0f );
}

/*
================
idActor::Event_AnimDistance
================
*/
void idActor::Event_AnimDistance( int channel, const char *animname ) {
	int anim;

	anim = GetAnim( channel, animname );
	if ( anim ) {
		if ( channel == ANIMCHANNEL_HEAD ) {
			if ( head.GetEntity() ) {
				idThread::ReturnFloat( head.GetEntity()->GetAnimator()->TotalMovementDelta( anim ).Length() );
				return;
			}
		} else {
			idThread::ReturnFloat( animator.TotalMovementDelta( anim ).Length() );
			return;
		}
	}
	
	idThread::ReturnFloat( 0.0f );
}

// grayman #3166 - reload replacement animations from a torch
// if the actor is carrying one

void idActor::Event_ReloadTorchReplacementAnims( void )
{
	if ( IsType(idAI::Type) )
	{
		idAI* me = static_cast<idAI*>(this);
		idEntity* torch = me->GetTorch();
		if ( torch )
		{
			LoadReplacementAnims(torch->spawnArgs,NULL); // grayman #3074
		}
	}
}

// Obsttorte: #0540
void idActor::Event_getAnimRate(int channel, const char* animName)
{
	const int index = GetAnim(channel, animName);
	if (index == 0)
	{
		idThread::ReturnFloat(-1);
		return;
	}
	idThread::ReturnFloat( m_animRates[index]);
}

void idActor::Event_setAnimRate(int channel, const char* animName, float animRate)
{
	const int index = GetAnim(channel, animName);
	if (index == 0 || animRate < 0)
	{
		idThread::ReturnFloat(-1);
		return;
	}
	m_animRates[index] = animRate;
	animator.CurrentAnim(channel)->UpdatePlaybackRate(index, this);
	idThread::ReturnFloat(1);
}

void idActor::Event_getAnimList(int channel)
{
	const idAnimator* anima = GetAnimatorForChannel(channel);
	if (anima == NULL) idThread::ReturnString("");
	const int n = anima->NumAnims();
	idStr res_list;
	for (int i = 0; i < n; i++)
	{
		res_list += idStr(i) + ": " + idStr(anima->AnimName(i)) + "####";
	}
	idThread::ReturnString(res_list);
}


/*
================
idActor::Event_HasEnemies
================
*/
void idActor::Event_HasEnemies( void ) {
	bool hasEnemy;

	hasEnemy = HasEnemies();
	idThread::ReturnInt( hasEnemy );
}

/*
================
idActor::Event_NextEnemy
================
*/
void idActor::Event_NextEnemy( idEntity *ent ) {
	idActor *actor;

	if ( !ent || ( ent == this ) ) {
		actor = enemyList.Next();
	} else {
		if ( !ent->IsType( idActor::Type ) ) {
			gameLocal.Error( "'%s' cannot be an enemy", ent->name.c_str() );
		}

		actor = static_cast<idActor *>( ent );
		if ( actor->enemyNode.ListHead() != &enemyList ) {
			gameLocal.Error( "'%s' is not in '%s' enemy list", actor->name.c_str(), name.c_str() );
		}
	}

	for( ; actor != NULL; actor = actor->enemyNode.Next() ) {
		if ( !actor->fl.hidden ) {
			idThread::ReturnEntity( actor );
			return;
		}
	}

    idThread::ReturnEntity( NULL );
}

/*
================
idActor::Event_ClosestEnemyToPoint
================
*/
void idActor::Event_ClosestEnemyToPoint( const idVec3 &pos ) {
	idActor *bestEnt = ClosestEnemyToPoint( pos );
	idThread::ReturnEntity( bestEnt );
}

/*
================
idActor::Event_MeleeBestParry
================
*/
void idActor::Event_MeleeBestParry()
{
	idThread::ReturnInt( GetBestParry() );
}

/*
================
idActor::Event_MeleeNameForNum
================
*/
void idActor::Event_MeleeNameForNum( int num )
{
	if( num >= 0 && num < NUM_MELEE_TYPES )
		idThread::ReturnString( MeleeTypeNames[num] );
	else
	{
		gameLocal.Warning("Actor %s attempted to look up bad melee type number %d", name.c_str(), num );
		idThread::ReturnString( "" );
	}
}

/*
================
idActor::Event_StopSound
================
*/
void idActor::Event_StopSound( int channel, int netSync ) {
	if ( channel == SND_CHANNEL_VOICE ) {
		idEntity *headEnt = head.GetEntity();
		if ( headEnt ) {
			headEnt->StopSound( channel, ( netSync != 0 ) );
		}
	}
	StopSound( channel, ( netSync != 0 ) );
}

/*
=====================
idActor::Event_SetNextState
=====================
*/
void idActor::Event_SetNextState( const char *name ) {
	idealState = GetScriptFunction( name );
	if ( idealState == state ) {
		state = NULL;
	}
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
		idThread::ReturnString( state->Name() );
	} else {
		idThread::ReturnString( "" );
	}
}

/*
=====================
idActor::Event_GetHead
=====================
*/
void idActor::Event_GetHead( void ) {
	idThread::ReturnEntity( head.GetEntity() );
}

/*
=====================
idActor::Event_GetEyePos
=====================
*/
void idActor::Event_GetEyePos( void )
{
	idThread::ReturnVector( GetEyePosition() );
}

/*
=====================
idActor::Event_SetHealth
=====================
*/
void idActor::Event_SetHealth( float newHealth ) {
	health = static_cast<int>(newHealth);
	fl.takedamage = true;
	if ( health > 0 ) {
		AI_DEAD = false;
	} else {
		AI_DEAD = true;
	}
}

/*
=====================
idActor::Event_GetHealth
=====================
*/
void idActor::Event_GetHealth( void ) {
	idThread::ReturnFloat( health );
}

/*
=====================
idActor::Event_SetMaxHealth
=====================
*/
void idActor::Event_SetMaxHealth( float newMaxHealth ) {
	maxHealth = static_cast<int>(newMaxHealth);
	fl.takedamage = true;

	if( health > newMaxHealth )
	{
		Event_SetHealth(newMaxHealth);
	}
}

/*
=====================
idActor::Event_Attach
=====================
*/
void idActor::Event_Attach( idEntity *ent, const char *AttName )
{
	Attach( ent, NULL, AttName );
}

/*
=====================
idActor::Event_AttachToPos
=====================
*/
void idActor::Event_AttachToPos( idEntity *ent, const char *PosName, const char *AttName )
{
	Attach( ent, PosName, AttName );
}

/*
=====================
idActor::Event_GetAttachment
=====================
*/
void idActor::Event_GetAttachment( const char *AttName )
{
	idEntity *ent = GetAttachment( AttName );
	idThread::ReturnEntity( ent );
}

/*
=====================
idActor::Event_GetAttachmentInd
=====================
*/
void idActor::Event_GetAttachmentInd( int ind )
{
	idEntity *ent = GetAttachment( ind );
	idThread::ReturnEntity( ent );
}

/*
=====================
idActor::Event_GetNumAttachments
=====================
*/
void idActor::Event_GetNumAttachments( void )
{
	idThread::ReturnInt( m_Attachments.Num() );
}

/*
=====================
idActor::Event_GetNumMeleeWeapons
=====================
*/
void idActor::Event_GetNumMeleeWeapons()
{
	idThread::ReturnInt(GetNumMeleeWeapons());
}

/*
=====================
idActor::Event_GetNumRangedWeapons
=====================
*/
void idActor::Event_GetNumRangedWeapons()
{
	idThread::ReturnInt(GetNumRangedWeapons());
}

int idActor::GetNumMeleeWeapons()
{
	int numMeleeWeapons(0);

	// greebo: Always return 1 if this type of actor doesn't need weapons to fight
	if (spawnArgs.GetBool("unarmed_melee", "0")) {
		return 1;
	}

	for (int i = 0; i < m_Attachments.Num(); i++)
	{
		idEntity* ent = m_Attachments[i].ent.GetEntity();

		if (ent == NULL || !m_Attachments[i].ent.IsValid())
		{
			continue;
		}

		if (ent->spawnArgs.GetBool("is_weapon_melee"))
		{
			numMeleeWeapons++;
		}
	}

	return numMeleeWeapons;
}

int idActor::GetNumRangedWeapons()
{
	int numRangedWeapons(0);
	
	if (spawnArgs.GetBool("unarmed_ranged", "0")) {
		return 1;
	}

	for (int i = 0; i < m_Attachments.Num(); i++)
	{
		idEntity* ent = m_Attachments[i].ent.GetEntity();

		if (ent == NULL || !m_Attachments[i].ent.IsValid())
		{
			continue;
		}

		if (ent->spawnArgs.GetBool("is_weapon_ranged"))
		{
			numRangedWeapons++;
		}
	}

	return numRangedWeapons;
}

bool idActor::GetAttackFlag(ECombatType type) const
{
	return m_AttackFlags.find(static_cast<int>(type)) != m_AttackFlags.end();
}

void idActor::SetAttackFlag(ECombatType type, bool enabled)
{
	if (enabled)
	{
		m_AttackFlags.insert(type);
	}
	else
	{
		m_AttackFlags.erase(type);
	}
}

bool idActor::CanGreet() // grayman #3338
{
	// Player and AI provide their own responses
	return true;
}

idAFAttachment* idActor::GetHead() // grayman #1104
{
	return head.GetEntity();
}

// grayman #3424
void idActor::MarkEventAsSearched( int eventID )
{
	if ( eventID < 0 )
	{
		return;
	}

	// If this event involves an entity that visually stims AI to get their
	// attention, we have to turn the stim off so that the AI doesn't
	// end up searching the same thing twice. (I.e. join a search as an
	// observer for a corpse, leave the search, then "see" the corpse
	// for yourself, starting up a new search.) There's no need to check
	// whether the entity has a visual stim; the IgnoreResponse() method
	// handles that case.

	SuspiciousEvent* se = gameLocal.FindSuspiciousEvent(eventID);
	if (se->entity.GetEntity())
	{
		se->entity.GetEntity()->IgnoreResponse(ST_VISUAL, this);

		// mark the flags that HasSeenEvidence() uses
		static_cast<idAI*>(this)->HasEvidence(se->type);
	}

	// grayman #3857 - Do I know about this event?

	for ( int i = 0 ; i < m_knownSuspiciousEvents.Num() ; i++ )
	{
		if ( m_knownSuspiciousEvents[i].eventID == eventID )
		{
			// I know about it. Mark it 'searched'.
			m_knownSuspiciousEvents[i].searched = true;
			return;
		}
	}

	// Didn't already know about this event. Since I'm
	// marking it as searched, add it to
	// my list of known events.

	// grayman #3857
	KnownSuspiciousEvent kse;
	kse.eventID = eventID;
	kse.searched = true;
	m_knownSuspiciousEvents.Append(kse);
}

// grayman #3424
bool idActor::HasSearchedEvent( int eventID )
{
	if ( eventID < 0 )
	{
		return false;
	}

	// Do I know about this event?

	for ( int i = 0 ; i < m_knownSuspiciousEvents.Num() ; i++ )
	{
		if ( m_knownSuspiciousEvents[i].eventID == eventID )
		{
			// I know about it. Have I already searched it?
			return m_knownSuspiciousEvents[i].searched;
		}
	}

	return false;
}

// grayman #3857
bool idActor::HasSearchedEvent( int eventID, EventType type, idVec3 location ) // grayman #3857 - TODO: apply where applicable
{
	if ( eventID < 0 )
	{
		return false;
	}

	// Do I know about this event?

	for ( int i = 0 ; i < m_knownSuspiciousEvents.Num() ; i++ )
	{
		if ( m_knownSuspiciousEvents[i].eventID == eventID )
		{
			// I know about it. Have I already searched it?
			if (m_knownSuspiciousEvents[i].searched)
			{
				return true;
			}
			break;
		}
	}

	// Have I searched a similar event in the same location?

	for ( int i = 0 ; i < m_knownSuspiciousEvents.Num() ; i++ )
	{
		if (m_knownSuspiciousEvents[i].searched)
		{
			SuspiciousEvent* se = gameLocal.FindSuspiciousEvent(m_knownSuspiciousEvents[i].eventID);
			if (se->type == type)
			{
				if ((se->location - location).LengthSqr() <= 90000) // 300*300
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool idActor::HasBeenWarned( idActor* other, int eventID )
{
	// Go through my list of warnings and see if I've already been warned
	// by 'other' about this event.

	for ( int i = 0 ; i < m_warningEvents.Num() ; i++ )
	{
		if ( ( m_warningEvents[i].eventID == eventID ) && ( m_warningEvents[i].entity.GetEntity() == other ) )
		{
			return true;
		}
	}

	return false; // No warning on file
}

bool idActor::KnowsAboutSuspiciousEvent( int eventID ) // grayman #3424
{
	if ( eventID < 0 )
	{
		return false;
	}

	// grayman #3857

	for ( int i = 0 ; i < m_knownSuspiciousEvents.Num() ; i++ )
	{
		if ( m_knownSuspiciousEvents[i].eventID == eventID )
		{
			return true;
		}
	}
	return false;
}

void idActor::AddSuspiciousEvent( int eventID ) // grayman #3424
{
	if ( eventID < 0 )
	{
		return;
	}

	// If I already know about this event, no need to add it.
	if ( KnowsAboutSuspiciousEvent( eventID ) )
	{
		return;
	}

	// grayman #3857
	KnownSuspiciousEvent kse;
	kse.eventID = eventID;
	kse.searched = false;
	m_knownSuspiciousEvents.Append(kse);
}

// grayman #3424 - Add a warning event. This represents a warning
// having passed from 'other' to 'this'. Since we don't want 'this'
// warning 'other' about the same event in the future, log a
// "reverse warning".

void idActor::AddWarningEvent( idActor* other, int eventID)
{
	// No need to do this if it's already been done.

	if (HasBeenWarned(other,eventID))
	{
		return;
	}

	WarningEvent we;
	we.entity = other;
	we.eventID = eventID;

	m_warningEvents.Append(we);

	// Back at ya

	WarningEvent we2;
	we2.entity = this;
	we2.eventID = eventID;

	other->m_warningEvents.Append(we2);
}

// grayman #3424 - log a suspicious event

int idActor::LogSuspiciousEvent( EventType type, idVec3 loc, idEntity* entity, bool forceLog ) // grayman #3857
{
	SuspiciousEvent se;
	se.type = type;
	se.location = loc;
	se.entity = entity;
	se.time = gameLocal.time; // grayman #3857

	int index = gameLocal.LogSuspiciousEvent(se,forceLog);
	AddSuspiciousEvent(index); // I know about this event
	return index;
}

int idActor::GetDoorSide(CFrobDoor* frobDoor, idVec3 pos) // grayman #4882
{
	int doorSide = 0;
	// determine which side of the door we're on
	idVec3 sm0 = frobDoor->GetDoorPosition(DOOR_SIDE_FRONT, DOOR_POS_SIDEMARKER);
	idVec3 sm1 = frobDoor->GetDoorPosition(DOOR_SIDE_BACK, DOOR_POS_SIDEMARKER);

	if ( (sm0 - pos).LengthSqr() < (sm1 - pos).LengthSqr() )
	{
		doorSide = DOOR_SIDE_FRONT;
	}
	else
	{
		doorSide = DOOR_SIDE_BACK;
	}

	return doorSide;
}



/****************************************************************************************
	=====================
	idActor::CrashLand
	handle collision (Falling) damage to AI/Players
	Added by Richard Day
	=====================
****************************************************************************************/
CrashLandResult idActor::CrashLand( const idPhysics_Actor& physicsObj, const idVec3 &savedOrigin, const idVec3 &savedVelocity )
{
	CrashLandResult result;
	result.damageDealt = 0;
	result.hasLanded = false;

	if (GetPhysics() == NULL)
	{
		return result;
	}

	idPhysics& physics = *GetPhysics(); // shortcut

	idVec3 curVelocity = physics.GetLinearVelocity();

	// different math to determine whether they've landed, and how much
	// damage they should accrue. So we'll differentiate between them in certain
	// code sections. isRagDoll will be true if the AI is a ragdoll using ragdoll physics.

	bool isRagDoll = physics.IsType(idPhysics_AF::Type);

	// grayman #3699 - Test AI changing to ragdoll when falling, which gives a more
	// realistic fall. Just KO the AI, since he can't come back from being a ragdoll.
	if (IsType(idAI::Type) && !isRagDoll)
	{
		// When an AI in freefall falls ~20 units, he's traveling
		// a vertical downward speed of ~200, so that's where we'll
		// see what awaits him below. If he's going to fall > 200 units,
		// we can change him to a ragdoll now. If not, we'll leave him alone.
		if ((curVelocity.z < -200.0f))
		{
			// trace down
			trace_t result;
			idVec3 startPoint = physics.GetOrigin();
			idVec3 bottomPoint = startPoint;
			bottomPoint.z -= 200;
			if ( !gameLocal.clip.TracePoint(result, startPoint, bottomPoint, MASK_OPAQUE, this) )
			{
				// no floor soon, so knock him out
				static_cast<idAI*>(this)->Fall_Knockout( m_SetInMotionByActor.GetEntity() );
				isRagDoll = true; // immediate change, for the rest of this method
			}
		}
	}

	// grayman #3699 - Various comparisons of current velocity with previous velocity

	float angleChange = 0.0f;

	if (isRagDoll)
	{
		// determine the angle between the current velocity and some previous saved velocity

		idVec3 cv = curVelocity;
		cv.NormalizeFast();
		idVec3 v = (m_savedVelocity == vec3_zero) ? savedVelocity : m_savedVelocity;
		bool sameDirection = (curVelocity * v >= 0);
		v.NormalizeFast();
		angleChange = RAD2DEG(idMath::ACos(cv * v));

		if (!sameDirection)
		{
			angleChange = 180.0f - angleChange;
		}

		if ( (curVelocity.LengthSqr() >= savedVelocity.LengthSqr()) && sameDirection )
		{
			// speeding up in the same direction, no crashing
			m_savedVelocity = vec3_zero;
			return result;
		}

		if (m_savedVelocity == vec3_zero)
		{
			// only save this once per crash
			m_savedVelocity = savedVelocity;
		}
	}
	else // idPhysics_Actor
	{
		if ( curVelocity.LengthSqr() >= savedVelocity.LengthSqr() )
		{
			return result;
		}
	}

	// no falling damage if touching a nodamage surface
	// We do this here since the sound won't be played otherwise
	// as we do no damage if this is true.
	int numContacts = physics.GetNumContacts();
	for ( int i = 0 ; i < numContacts ; i++ )
	{
		const contactInfo_t &contact = physics.GetContact( i );
		if ( contact.material->GetSurfaceFlags() & SURF_NODAMAGE )
		{
			StartSound( "snd_land_hard", SND_CHANNEL_ANY, 0, false, NULL );
			result.hasLanded = true;
			return result;
		}
	}

	const idVec3& vGravNorm = physics.GetGravityNormal();

	// grayman #0578 - velocity for the purposes of this method needs to be the
	// relative velocity of the actor to the entity he's landing on.

	// grayman #3608 - There are two cases:
	//
	// 1 - you're falling
	//
	// Your velocity will have become very small as you come
	// to a stop. Determining damage will need to factor in the velocity of the object
	// you're landing on. I.e. you're at velocity 0, but the object you landed on
	// is traveling downward at some speed. You're not really going to maintain a
	// velocity of 0 because as the object falls, you'll follow it. But for now, your
	// velocity is 0. Your change in velocity from the previous frame will be:
	//
	// relative velocity = old velocity - current velocity - object velocity
	//
	// 2 - you're being pushed by something underneath you
	//
	// In this case, your velocity is rising. If it starts to fall from some large
	// number, and you're still in contact with the pushing object, your change in
	// velocity from the previous frame will be:
	//
	// relative velocity = old velocity - current velocity
	//

	idVec3 landedOnVelocity(0,0,0); // the velocity of any object you're on

	if ( savedOrigin.z > physics.GetOrigin().z ) // only consider the speed of the object you're landing on if you're falling
	{
		for ( int i = 0 ; i < numContacts ; i++ )
		{
			const contactInfo_t &contact = physics.GetContact(i);
			if ( contact.entityNum == ENTITYNUM_WORLD )
			{
				// we assume the world does not move
				break;
			}
			idEntity* ent = gameLocal.entities[contact.entityNum];
			if ( !ent )
			{
				continue;
			}

			// grayman #3370 - for some reason, moveables can attain high,
			// short-lived velocities. Since the fix to #0578 was meant
			// for jumping onto moving elevators (movers), we'll ignore
			// moveables, which tend to be smaller items that get kicked around

			// Tels: That would mean you could not jump on a stack of crates traveling
			//	 on a large elavator, though?

			// grayman: Yes, that's right. TDM rules forbid the stacking of crates on a large fast elevator.

			if ( ent->IsType(idMoveable::Type) )
			{
				continue;
			}

			landedOnVelocity = ent->GetPhysics()->GetLinearVelocity();
			break;
		}
	}

	curVelocity = curVelocity + landedOnVelocity; // velocity relative to the object you're on, if falling

	// The current speed parallel to gravity
	idVec3 curGravVelocity = (curVelocity*vGravNorm) * vGravNorm;

	// Get the vdelta (how much the velocity has changed in this frame)
	idVec3 deltaVec;
	if (isRagDoll) // grayman #3699
	{
		deltaVec = (m_savedVelocity - curVelocity);
	}
	else // idPhysics_Actor
	{
		deltaVec = (savedVelocity - curVelocity);
	}

	// greebo: Get the vertical portion of the velocity 
	idVec3 deltaVecVert = (deltaVec * vGravNorm) * vGravNorm;
	// Get the horizontal portion by subtracting the vertical one from the velocity
	idVec3 deltaVecHoriz = deltaVec - deltaVecVert;

	float deltaHoriz = deltaVecHoriz.LengthSqr();
	float deltaVert = deltaVec.LengthSqr() - deltaHoriz;

	// conversion factor to 10s of MJ/kg, horizontal and vertical weighted differently
	double delta = cv_collision_damage_scale_vert.GetFloat() * deltaVert;
	delta += cv_collision_damage_scale_horiz.GetFloat() * deltaHoriz;

	// damage scale per actor
	delta *= m_delta_scale;

	// grayman #3699 - physicsObj is the wrong physics to query when a ragdoll.
	// It's correct when using actor physics. Ragdoll physics
	// has no GetWaterLevel(), so the following code provides one.
	waterLevel_t waterLevel = WATERLEVEL_NONE;
	if (isRagDoll)
	{
		idBounds abb = physics.GetAbsBounds();
		// check at bottom of bounding box
		idVec3 point = abb[0];
		point.z++;
		int contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, this );
		if ( contents & MASK_WATER )
		{
			waterLevel = WATERLEVEL_FEET;

			// check at center
			point = abb.GetCenter();
			contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, this );
			if ( contents & MASK_WATER )
			{
				waterLevel = WATERLEVEL_WAIST;

				// check at top
				point = abb[1];
				point.z--;
				contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, this );
				if ( contents & MASK_WATER )
				{
					waterLevel = WATERLEVEL_HEAD;
				}
			}
		}
	}
	else // idPhysics_Actor
	{
		waterLevel = static_cast<idPhysics_Actor&>(physics).GetWaterLevel();
	}

	// reduce falling damage if there is standing water
	switch (waterLevel)
	{
		case WATERLEVEL_NONE:
			break;
		case WATERLEVEL_FEET:	delta *= 0.8f;	// -20% for shallow water
			break; 
		case WATERLEVEL_WAIST:	delta *= 0.5f;	// -50% for medium water
			break; 
		case WATERLEVEL_HEAD:	delta *= 0.25f;	// -75% for deep water
			break;
		default: 
			break;
	};

	// We've been moving downwards with a certain velocity, set the flag 

	// grayman #2978 - Setting result.hasLanded to true
	// in the next segment is based on the actor having lost almost
	// all vertical velocity. But when landing on a sloped surface, the actor hits, then
	// begins to slide for a few frames, so he still has some downward vertical
	// velocity here. Even so, for all intents and purposes, he's landed, and we
	// should tell the calling routine.

	// Even though the ( curGravVelocity.LengthFast() < 1 ) check appears below
	// to not be necessary, I'm leaving it in in case someone in the future
	// wants to distinguish between flat and sloped landings.

	// For the purpose of establishing 'hasLanded', we compare the velocity
	// frame-to-frame

	if (isRagDoll) // grayman #3699
	{
		if ( waterLevel != WATERLEVEL_NONE )
		{
			result.hasLanded = true; // crashed into water
		}

		if (!result.hasLanded && (angleChange > 10) && (numContacts > 0) )
		{
			result.hasLanded = true; // crashed into a solid, changing your velocity
		}
	
		if (!result.hasLanded) // no damage until you land
		{
			return result;
		}

		m_savedVelocity = vec3_zero;
	}
	else // idPhysics_Actor
	{
		if ( ( curGravVelocity.LengthFast() < 1 ) && ( deltaVecVert*vGravNorm > 100 ) )
		{
			result.hasLanded = true; // flat landing
		}

		if ( !result.hasLanded && ( deltaVecVert*vGravNorm > 100 ) )
		{
			result.hasLanded = true; // sloped landing, because there's still some vertical velocity,
									 // but the change in velocity is large enough to indicate
									 // having landed
		}
	}

	if (delta < 390000)
	{
		// Below this threshold, nothing happens
		return result;
	}

	// greebo: Now calibrate the damage using the sixth power of the velocity (=square^3)
	// The damage has a linear relationship to the sixth power of vdelta
	delta = delta*delta*delta;
	int damage = static_cast<int>(1.4E-16 * delta - 3);

	// gameRenderWorld->DebugText(idStr(damage), GetPhysics()->GetOrigin(), 0.15, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 16);

	// Check if the damage is above our threshold, ignore otherwise
	if (damage >= m_damage_thresh_min)
	{
		//gameRenderWorld->DebugText(idStr(deltaVert), GetPhysics()->GetOrigin(), 0.15, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 50000);

		//gameRenderWorld->DebugText(idStr(damage), GetPhysics()->GetOrigin(), 0.15, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 5000);

		// grayman - print data re: being damaged
		gameLocal.Printf("'%s' dealt %d landing damage at [%s]\n", GetName(),damage,GetPhysics()->GetOrigin().ToString());

		pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim

		// Update our return value
		result.damageDealt = damage;

		if (damage > m_damage_thresh_hard)
		{
			// greebo: the damage_fall_hard entityDef has a damage value of 1, which is scaled by the calculated damage integer
			StartSound("snd_damage_land_hard", SND_CHANNEL_VOICE, 0, false, NULL);
			Damage(NULL, m_SetInMotionByActor.GetEntity(), vGravNorm, "damage_fall_hard", damage, 0); // grayman #3699
		}
		else
		{
			// We are below the "hard" threshold, just deal the "soft" damage
			StartSound("snd_damage_land_soft", SND_CHANNEL_VOICE, 0, false, NULL);
			Damage(NULL, m_SetInMotionByActor.GetEntity(), vGravNorm, "damage_fall_soft", damage, 0); // grayman #3699
		}
	}

	return result;
}

void idActor::Event_MeleeAttackStarted( int num )
{
	m_MeleeStatus.m_ActionResult = MELEERESULT_IN_PROGRESS;
	m_MeleeStatus.m_ActionState = MELEEACTION_ATTACK;
	m_MeleeStatus.m_ActionType = (EMeleeType) num;
	m_MeleeStatus.m_ActionPhase = MELEEPHASE_PREPARING;
	m_MeleeStatus.m_PhaseChangeTime = gameLocal.time;

	// randomize minimum times to events after this one
	// TODO: Rewrite this so we only set the times we need according to the result?
	float fRand = gameLocal.random.RandomFloat();
	m_MeleeCurrentHoldTime = m_MeleeHoldTimeMin + fRand*(m_MeleeHoldTimeMax - m_MeleeHoldTimeMin);
	m_MeleeCurrentAttackRecovery = m_MeleeAttackRecoveryMin + fRand*(m_MeleeAttackRecoveryMax - m_MeleeAttackRecoveryMin);
	m_MeleeCurrentAttackLongRecovery = m_MeleeAttackLongRecoveryMin + fRand*(m_MeleeAttackLongRecoveryMax - m_MeleeAttackLongRecoveryMin);
	m_MeleeCurrentParryRecovery = m_MeleeParryRecoveryMin + fRand*(m_MeleeParryRecoveryMax - m_MeleeParryRecoveryMin);
}

void idActor::Event_MeleeParryStarted( int num )
{
	m_MeleeStatus.m_ActionResult = MELEERESULT_IN_PROGRESS;
	m_MeleeStatus.m_ActionState = MELEEACTION_PARRY;
	m_MeleeStatus.m_ActionType = (EMeleeType) num;
	m_MeleeStatus.m_ActionPhase = MELEEPHASE_PREPARING;
	m_MeleeStatus.m_PhaseChangeTime = gameLocal.time;

	// randomize minimum times to events after this one
	// TODO: Rewrite this so we only set what we need based on result?
	float fRand = gameLocal.random.RandomFloat();
	m_MeleeCurrentParryHold = m_MeleeParryHoldMin + fRand*(m_MeleeParryHoldMax - m_MeleeParryHoldMin);
	m_MeleeCurrentAttackLongRecovery = m_MeleeAttackLongRecoveryMin + fRand*(m_MeleeAttackLongRecoveryMax - m_MeleeAttackLongRecoveryMin);
	m_MeleeCurrentParryRecovery = m_MeleeParryRecoveryMin + fRand*(m_MeleeParryRecoveryMax - m_MeleeParryRecoveryMin);
	m_MeleeCurrentRiposteRecovery = m_MeleeRiposteRecoveryMin + fRand*(m_MeleeRiposteRecoveryMax - m_MeleeRiposteRecoveryMin);
	m_MeleeCurrentPreParryDelay = m_MeleePreParryDelayMin + fRand*(m_MeleePreParryDelayMax - m_MeleePreParryDelayMin);
	m_MeleeCurrentRepeatedPreParryDelay = m_MeleeRepeatedPreParryDelayMin + fRand*(m_MeleeRepeatedPreParryDelayMax - m_MeleeRepeatedPreParryDelayMin);
	m_MeleeCurrentPostParryDelay = m_MeleePostParryDelayMin + fRand*(m_MeleePostParryDelayMax - m_MeleePostParryDelayMin);
	m_MeleeCurrentRepeatedPostParryDelay = m_MeleeRepeatedPostParryDelayMin + fRand*(m_MeleeRepeatedPostParryDelayMax - m_MeleeRepeatedPostParryDelayMin);
}

void idActor::Event_MeleeActionHeld()
{
	m_MeleeStatus.m_ActionPhase = MELEEPHASE_HOLDING;
	m_MeleeStatus.m_PhaseChangeTime = gameLocal.time;
}

void idActor::Event_MeleeActionReleased()
{
	// attacks go to executing phase, parries jump straight to recovering
	// CMeleeWeapon handles toggling attacks from executing to recovering
	if( m_MeleeStatus.m_ActionState == MELEEACTION_ATTACK )
	{
		m_MeleeStatus.m_ActionPhase = MELEEPHASE_EXECUTING;
	}
	else
	{
		m_MeleeStatus.m_ActionPhase = MELEEPHASE_RECOVERING;
	}

	m_MeleeStatus.m_PhaseChangeTime = gameLocal.time;
}

void idActor::Event_MeleeActionFinished()
{
	m_MeleeStatus.m_ActionState = MELEEACTION_READY;
	m_MeleeStatus.m_ActionPhase = MELEEPHASE_PREPARING;
	m_MeleeStatus.m_LastActTime = gameLocal.time;
}

void idActor::Event_GetMeleeActionState()
{
	idThread::ReturnInt( m_MeleeStatus.m_ActionState );
}

void idActor::Event_GetMeleeActionPhase()
{
	idThread::ReturnInt( m_MeleeStatus.m_ActionPhase );
}

void idActor::Event_GetMeleeActionType()
{
	idThread::ReturnInt( m_MeleeStatus.m_ActionType );
}

void idActor::Event_GetMeleeLastActTime()
{
	idThread::ReturnInt( m_MeleeStatus.m_LastActTime );
}

void idActor::Event_GetMeleeResult()
{
	idThread::ReturnInt( m_MeleeStatus.m_ActionResult );
}

void idActor::Event_GetMeleeLastHitByType()
{
	idThread::ReturnInt( m_MeleeStatus.m_LastHitByType );
}

void idActor::Event_SetReplacementAnim(const char* animToReplace, const char* replacementAnim)
{
	SetReplacementAnim(animToReplace, replacementAnim);
}

void idActor::Event_RemoveReplacementAnim(const char* animName)
{
	RemoveReplacementAnim(animName);
}

void idActor::Event_LookupReplacementAnim(const char* animName)
{
	idThread::ReturnString(LookupReplacementAnim(animName));
}

void idActor::Event_GetAttackFlag(int combatType)
{
	if (combatType < COMBAT_NONE || combatType >= NUM_COMBAT_TYPES) 
	{
		idThread::ReturnInt(0);
	}

	idThread::ReturnInt(GetAttackFlag(static_cast<ECombatType>(combatType)) ? 1 : 0);
}

void idActor::Event_SetAttackFlag(int combatType, int enabled)
{
	if (combatType < COMBAT_NONE || combatType >= NUM_COMBAT_TYPES) 
	{
		// do nothing
		gameLocal.Warning("%s - Script is trying to set invalid combatType %d", name.c_str(),combatType);
		return;
	}

	SetAttackFlag(static_cast<ECombatType>(combatType), enabled == 1);
}

// ========== CMeleeStatus implementation =========
CMeleeStatus::CMeleeStatus( void )
{
	m_ActionState	= MELEEACTION_READY;
	m_ActionPhase	= MELEEPHASE_PREPARING;
	m_ActionType	= MELEETYPE_OVER;

	m_PhaseChangeTime	= 0;
	m_LastActTime		= 0;

	m_ActionResult	= MELEERESULT_IN_PROGRESS;

	m_bWasHit		= false; // NYI
	m_LastHitByType = MELEETYPE_UNBLOCKABLE;

	m_bCanParry		= false;
	m_bCanParryAll	= false;
	m_attacks.Clear();
}

CMeleeStatus::~CMeleeStatus( void )
{
	m_attacks.Clear();
}

void CMeleeStatus::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( m_ActionState );
	savefile->WriteInt( m_ActionPhase );
	savefile->WriteInt( m_ActionType );
	savefile->WriteInt( m_PhaseChangeTime );
	savefile->WriteInt( m_LastActTime );
	savefile->WriteInt( m_ActionResult );

	savefile->WriteBool( m_bWasHit );
	savefile->WriteInt( m_LastHitByType );

	savefile->WriteBool( m_bCanParry );
	savefile->WriteBool( m_bCanParryAll );

	int num = m_attacks.Num();
	savefile->WriteInt( num );
	for( int i =0; i < num; i++ )
	{
		savefile->WriteInt( m_attacks[i] );
	}
}

void CMeleeStatus::Restore( idRestoreGame *savefile )
{
	int i = 0;
	savefile->ReadInt( i );
	m_ActionState = (EMeleeActState) i; 
	savefile->ReadInt( i );
	m_ActionPhase = (EMeleeActPhase) i;
	savefile->ReadInt( i );
	m_ActionType = (EMeleeType) i;
	savefile->ReadInt( m_PhaseChangeTime );
	savefile->ReadInt( m_LastActTime );
	savefile->ReadInt( i );
	m_ActionResult = (EMeleeResult) i;

	savefile->ReadBool( m_bWasHit );
	savefile->ReadInt( i );
	m_LastHitByType = (EMeleeType) i;

	savefile->ReadBool( m_bCanParry );
	savefile->ReadBool( m_bCanParryAll );

	int num;
	savefile->ReadInt( num );
	m_attacks.SetNum( num );
	for( int j =0; j < num; j++ )
	{
		savefile->ReadInt( i );
		m_attacks[j] = (EMeleeType) i;
	}
}
