/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"
#include "Item.h"
#include "Light.h"
#include "Projectile.h"
#include "WorldSpawn.h"
#include "Fx.h"
#include "Player.h"

#include "BrittleFracture.h"
#include "framework/DeclEntityDef.h"
#include "bc_meta.h"
#include "bc_fireattachment.h"
#include "Actor.h"


// blendo eric: debug helpers for anim states
const int DEBUG_Anim_State_Count = 2;
const int DEBUG_Anim_State_Channels[DEBUG_Anim_State_Count] = { ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS }; // limit to skindeep used channels
static idStr DEBUG_Anim_State_Changed[ANIM_NumAnimChannels] = {};
static idStr DEBUG_Anim_State_Prev[ANIM_NumAnimChannels] = {};

static int DEBUG_Anim_Channel_Executing = 0;


// find state names that mismatch their event channel parameters
// example:
//		void player:Torso_Walk(){ playAnim(ANIMCHANNEL_LEGS, "idle"); }
// The channel ANIMCHANNEL_LEGS being called in a state called Torso_Walk is likely a script bug
bool DEBUG_ANIM_State_Channel_Mismatch(const char * stateName, int channelChanging) {
	const char* chanName = ANIMCHANNEL_Names[channelChanging];

	if (idStr::FindText(stateName, chanName, false) < 0) { // if state already contains the channel name skip check
		for (int idx = 0; idx < DEBUG_Anim_State_Count; idx++) {
			const int otherChannelToCheck = DEBUG_Anim_State_Channels[idx]; // limit to skindeep used channels
			if (channelChanging != otherChannelToCheck)
			{
				if (idStr::FindText(stateName, chanName, false) >= 0) {
					return true;
				}
			}
		}
	}
	return false;
}

const int VISIONBOX_MAXLENGTH = 1024;
const int VISIONBOX_WIDTH = 160;
const int VISIONBOX_MINZ = -64;
const int VISIONBOX_MAXZ = 64;



const int ENERGYSHIELD_RECHARGEDELAY = 10000;

const int GIB_THRESHOLD = -20;




//Player throwable objects can only stun if thrown object is from SIDE or BEHIND. AI will ignore throwables thrown to the front of them.
//-1 = directly in front.
//0 = directly to side.
//1 = directly behind.
const float THROWABLE_DOT_THRESHOLD = -.2f;


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
	savefile->WriteBool( idleAnim ); // bool idleAnim
	savefile->WriteString( state ); // idString state
	savefile->WriteInt( animBlendFrames ); // int animBlendFrames
	savefile->WriteInt( lastAnimBlendFrames ); // int lastAnimBlendFrames

	savefile->WriteObject( self ); // idActor * self
	savefile->WriteAnimatorPtr( animator ); // idAnimator * animator
	savefile->WriteObject( thread ); // idThread * thread
	savefile->WriteInt( channel ); // int channel
	savefile->WriteBool( disabled ); // bool disabled
}
/*
=====================
idAnimState::Restore
=====================
*/
void idAnimState::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( idleAnim ); // bool idleAnim
	savefile->ReadString( state ); // idString state
	savefile->ReadInt( animBlendFrames ); // int animBlendFrames
	savefile->ReadInt( lastAnimBlendFrames ); // int lastAnimBlendFrames

	savefile->ReadObject( CastClassPtrRef(self) ); // idActor * self
	savefile->ReadAnimatorPtr( animator ); // idAnimator * animator
	savefile->ReadObject( CastClassPtrRef(thread) ); // idThread * thread
	savefile->ReadInt( channel ); // int channel
	savefile->ReadBool( disabled ); // bool disabled
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
		//assert( 0 );
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, self->scriptObject.GetTypeName() );
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
		gameLocal.Printf( "%d: %s: Animstate: %s\n", gameLocal.time, self->name.c_str(), state.c_str() );
	}
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

	if ( ai_debugScript.GetInteger() == self->entityNumber ) {
		thread->EnableDebugInfo();
	} else {
		thread->DisableDebugInfo();
	}

	thread->Execute();

	return true;
}

/***********************************************************************

	idActor

***********************************************************************/


const idEventDef AI_EnableEyeFocus( "enableEyeFocus" );
const idEventDef AI_DisableEyeFocus( "disableEyeFocus" );
const idEventDef EV_Footstep( "footstep" );
const idEventDef EV_EnableWalkIK( "EnableWalkIK" );
const idEventDef EV_DisableWalkIK( "DisableWalkIK" );
const idEventDef EV_EnableLegIK( "EnableLegIK", "d" );
const idEventDef EV_DisableLegIK( "DisableLegIK", "d" );
const idEventDef AI_StopAnim( "stopAnim", "dd" );
const idEventDef AI_PlayAnim( "playAnim", "ds", 'd' );
const idEventDef AI_PlayCycle( "playCycle", "ds", 'd' );
const idEventDef AI_IdleAnim( "idleAnim", "ds", 'd' );
const idEventDef AI_SetSyncedAnimWeight( "setSyncedAnimWeight", "ddf" );
const idEventDef AI_SetBlendFrames( "setBlendFrames", "dd" );
const idEventDef AI_GetBlendFrames( "getBlendFrames", "d", 'd' );
const idEventDef AI_AnimState( "animState", "dsd" );
const idEventDef AI_GetAnimState( "getAnimState", "d", 's' );
const idEventDef AI_InAnimState( "inAnimState", "ds", 'd' );
const idEventDef AI_FinishAction( "finishAction", "s" );
const idEventDef AI_AnimDone( "animDone", "dd", 'd' );
const idEventDef AI_OverrideAnim( "overrideAnim", "d" );
const idEventDef AI_EnableAnim( "enableAnim", "dd" );
const idEventDef AI_PreventPain( "preventPain", "f" );
const idEventDef AI_DisablePain( "disablePain" );
const idEventDef AI_EnablePain( "enablePain" );
const idEventDef AI_GetPainAnim( "getPainAnim", NULL, 's' );
const idEventDef AI_SetAnimPrefix( "setAnimPrefix", "s" );
const idEventDef AI_HasAnim( "hasAnim", "ds", 'f' );
const idEventDef AI_CheckAnim( "checkAnim", "ds" );
const idEventDef AI_ChooseAnim( "chooseAnim", "ds", 's' );
const idEventDef AI_AnimLength( "animLength", "ds", 'f' );
const idEventDef AI_AnimDistance( "animDistance", "ds", 'f' );
const idEventDef AI_HasEnemies( "hasEnemies", NULL, 'd' );
const idEventDef AI_NextEnemy( "nextEnemy", "E", 'e' );
const idEventDef AI_ClosestEnemyToPoint( "closestEnemyToPoint", "v", 'e' );
const idEventDef AI_SetNextState( "setNextState", "s" );
const idEventDef AI_SetState( "setState", "s" );
const idEventDef AI_GetState( "getState", NULL, 's' );
const idEventDef AI_GetHead( "getHead", NULL, 'e' );
#ifdef _D3XP
const idEventDef EV_SetDamageGroupScale( "setDamageGroupScale", "sf" );
const idEventDef EV_SetDamageGroupScaleAll( "setDamageGroupScaleAll", "f" );
const idEventDef EV_GetDamageGroupScale( "getDamageGroupScale", "s", 'f' );
const idEventDef EV_SetDamageCap( "setDamageCap", "f" );
const idEventDef EV_SetWaitState( "setWaitState" , "s" );
const idEventDef EV_GetWaitState( "getWaitState", NULL, 's' );
#endif

//BC
const idEventDef EV_FootstepLeft("leftFoot");
const idEventDef EV_FootstepRight("rightFoot");
const idEventDef EV_FootstepLeftRun("leftFootRun");
const idEventDef EV_FootstepRightRun("rightFootRun");
const idEventDef EV_Actor_getEyePos("getEyePos", NULL, 'v');
const idEventDef EV_Actor_getCenter("getActorCenter", NULL, 'v');
const idEventDef AI_GetCustomIdleAnim("getCustomIdleAnim", NULL, 's');
const idEventDef AI_GetStunAnim("getStunAnim", NULL, 's');

const idEventDef AI_ActorVO("actorVO", "sd", 'f');


CLASS_DECLARATION( idAFEntity_Gibbable, idActor )
	EVENT( AI_EnableEyeFocus,			idActor::Event_EnableEyeFocus )
	EVENT( AI_DisableEyeFocus,			idActor::Event_DisableEyeFocus )
	EVENT( EV_Footstep,					idActor::Event_Footstep )
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
	EVENT( AI_PlayCycle,				idActor::Event_PlayCycle )
	EVENT( AI_IdleAnim,					idActor::Event_IdleAnim )
	EVENT( AI_SetSyncedAnimWeight,		idActor::Event_SetSyncedAnimWeight )
	EVENT( AI_SetBlendFrames,			idActor::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,			idActor::Event_GetBlendFrames )
	EVENT( AI_AnimState,				idActor::Event_AnimState )
	EVENT( AI_GetAnimState,				idActor::Event_GetAnimState )
	EVENT( AI_InAnimState,				idActor::Event_InAnimState )
	EVENT( AI_FinishAction,				idActor::Event_FinishAction )
	EVENT( AI_AnimDone,					idActor::Event_AnimDone )
	EVENT( AI_OverrideAnim,				idActor::Event_OverrideAnim )
	EVENT( AI_EnableAnim,				idActor::Event_EnableAnim )
	EVENT( AI_HasAnim,					idActor::Event_HasAnim )
	EVENT( AI_CheckAnim,				idActor::Event_CheckAnim )
	EVENT( AI_ChooseAnim,				idActor::Event_ChooseAnim )
	EVENT( AI_AnimLength,				idActor::Event_AnimLength )
	EVENT( AI_AnimDistance,				idActor::Event_AnimDistance )
	EVENT( AI_HasEnemies,				idActor::Event_HasEnemies )
	EVENT( AI_NextEnemy,				idActor::Event_NextEnemy )
	EVENT( AI_ClosestEnemyToPoint,		idActor::Event_ClosestEnemyToPoint )
	EVENT( EV_StopSound,				idActor::Event_StopSound )
	EVENT( AI_SetNextState,				idActor::Event_SetNextState )
	EVENT( AI_SetState,					idActor::Event_SetState )
	EVENT( AI_GetState,					idActor::Event_GetState )
	EVENT( AI_GetHead,					idActor::Event_GetHead )
#ifdef _D3XP
	EVENT( EV_SetDamageGroupScale,		idActor::Event_SetDamageGroupScale )
	EVENT( EV_SetDamageGroupScaleAll,	idActor::Event_SetDamageGroupScaleAll )
	EVENT( EV_GetDamageGroupScale,		idActor::Event_GetDamageGroupScale )
	EVENT( EV_SetDamageCap,				idActor::Event_SetDamageCap )
	EVENT( EV_SetWaitState,				idActor::Event_SetWaitState )
	EVENT( EV_GetWaitState,				idActor::Event_GetWaitState )


	//BC
	EVENT(EV_FootstepLeft,				idActor::Event_FootstepLeft)
	EVENT(EV_FootstepRight,				idActor::Event_FootstepRight)
	EVENT(EV_FootstepLeftRun,			idActor::Event_FootstepLeftRun)
	EVENT(EV_FootstepRightRun,			idActor::Event_FootstepRightRun)
	EVENT(EV_Actor_getEyePos,			idActor::Event_getEyePos)
	EVENT(EV_Actor_getCenter,			idActor::Event_GetActorCenter)
	EVENT(EV_PostSpawn,					idActor::Event_PostSpawn)
	EVENT(AI_GetCustomIdleAnim,			idActor::Event_GetCustomIdleAnim)
	EVENT(AI_GetStunAnim,				idActor::Event_GetStunAnim)

	EVENT(AI_ActorVO,					idActor::Event_ActorVO)

	

#endif
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

	team				= 0;
	rank				= 0;
	fovDot				= 0.0f;
	eyeOffset.Zero();
	pain_debounce_time	= 0;
	pain_delay			= 0;
	pain_threshold		= 0;

	state				= NULL;
	idealState			= NULL;

	leftEyeJoint		= INVALID_JOINT;
	rightEyeJoint		= INVALID_JOINT;
	soundJoint			= INVALID_JOINT;

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

	attachments.SetGranularity( 1 );

	attachmentsToDrop.SetGranularity( 1 ); //bc

	enemyNode.SetOwner( this );
	enemyList.SetOwner( this );


	aimAssistNode.SetOwner(this);
	aimAssistNode.AddToEnd(gameLocal.aimAssistEntities);

#ifdef _D3XP
	damageCap = -1;
#endif

	visionBox = NULL;
	eyeJoint = INVALID_JOINT;

	customIdleAnim = "";
	stunAnimationName = "";
	energyShieldModel = NULL;
	hasAttachedBeltItems = false;
	stunTime = 0;
	stunStartTime = 0;
	helmetModel = NULL;
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

	// blendo eric: TODO need to come back to this, forgot if this fixed anything? probably for missing ent bug?
	// but i don't think we should unbind/remove on inactive ents anyhow
#if 1
	// remove any attached entities
	for( i = 0; i < attachments.Num(); i++ ) {
		ent = attachments[ i ].ent.GetEntity();
		if ( ent ) {
			ent->Unbind();
			ent->PostEventMS( &EV_Remove, 0 );
		}
	}

	for (i = 0; i < attachmentsToDrop.Num(); i++) {
		ent = attachmentsToDrop[i].ent.GetEntity();
		if ( ent ) {
			ent->Unbind();
			ent->PostEventMS(&EV_Remove, 0);
		}
	}
#else
	// remove any attached entities
	for( i = 0; i < attachments.Num(); i++ ) {
		ent = attachments[ i ].ent.GetEntity();
		if ( ent && ent->IsActive() ) {
			ent->Unbind();
			ent->PostEventMS( &EV_Remove, 0 );
		}
	}

	for (i = 0; i < attachmentsToDrop.Num(); i++) {
		ent = attachmentsToDrop[i].ent.GetEntity();
		if (ent && ent->IsActive()) {
			ent->Unbind();
			ent->PostEventMS(&EV_Remove, 0);
		}
	}
#endif

	//DOOM BFG
	aimAssistNode.Remove();


	if (energyShieldModel != NULL)
	{
		energyShieldModel->PostEventMS(&EV_Remove, 0);
		energyShieldModel = nullptr;
	}
	

	ShutdownThreads();
}

/*
=====================
idActor::Spawn
=====================
*/
void idActor::Spawn( void ) {
	idEntity		*ent;
	idStr			jointName;
	float			fovDegrees;
	copyJoints_t	copyJoint;	


	animPrefix	= "";
	state		= NULL;
	idealState	= NULL;

	spawnArgs.GetInt( "rank", "0", rank );
	spawnArgs.GetInt( "team", "0", team );
	spawnArgs.GetVector( "offsetModel", "0 0 0", modelOffset );

	spawnArgs.GetBool( "use_combat_bbox", "0", use_combat_bbox );

	viewAxis = GetPhysics()->GetAxis();

	spawnArgs.GetFloat( "fov", "90", fovDegrees );
	SetFOV( fovDegrees );

	pain_debounce_time	= 0;

	pain_delay		= SEC2MS( spawnArgs.GetFloat( "pain_delay" ) );
	pain_threshold	= spawnArgs.GetInt( "pain_threshold" );

	LoadAF();

	walkIK.Init( this, IK_ANIM, modelOffset );

	// the animation used to be set to the IK_ANIM at this point, but that was fixed, resulting in
	// attachments not binding correctly, so we're stuck setting the IK_ANIM before attaching things.
	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.SetFrame( ANIMCHANNEL_ALL, animator.GetAnim( IK_ANIM ), 0, 0, 0 );

	// spawn any attachments we might have
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_attach", NULL );
	while ( kv ) {
		idDict args;

		if (kv->GetValue().Length() <= 0)
		{
			gameLocal.Printf("def_attach on entity '%s' is empty field.\n", kv->GetValue().c_str(), name.c_str());
		}
		else
		{
			args.Set("classname", kv->GetValue().c_str());

			// make items non-touchable so the player can't take them out of the character's hands
			args.Set("no_touch", "1");

			// don't let them drop to the floor
			args.Set("dropToFloor", "0");

			gameLocal.SpawnEntityDef(args, &ent);
			if (!ent) {
				gameLocal.Error("Couldn't spawn '%s' to attach to entity '%s'", kv->GetValue().c_str(), name.c_str());
			}
			else {
				Attach(ent);
			}
		}
		kv = spawnArgs.MatchPrefix( "def_attach", kv );
	}

	SetupDamageGroups();
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

	finalBoss = spawnArgs.GetBool( "finalboss" );



	//BC
	if (spawnArgs.GetBool("hasvisionbox", "0"))
	{
		SetupVisionbox();
	}

	
	

	
	
	energyShieldMax = spawnArgs.GetInt("energyshield", "0");
	energyShieldCurrent = 0;
	energyShieldTimer = 0;
	energyShieldState = ENERGYSHIELDSTATE_STOWED;
	energyShieldModel = NULL;


	lastDamageOrigin = vec3_zero;
	lastAttacker = NULL;
	lastDamagedTime = 0;

	lastHealth = 0;
	lastHealthTimer = 0;

	helmetModel = NULL;
	hasHelmet = false;	

	vo_lastSpeakTime = 0;
	vo_lastCategory = 0;

	pointdefenseAmount = spawnArgs.GetInt("pointdefense_amount");



	FinishSetup();

	PostEventMS(&EV_PostSpawn, 0);
}

void idActor::Event_PostSpawn()
{
	if (spawnArgs.GetBool("do_beltattachments"))
	{
		AttachBeltattachments();
	}
}


idVec3 idActor::GetBeltAttachmentVec3(idStr entityName, idStr keyname)
{
	//Get the attach info from the entity that's being attached.
	//This is so that we don't have to add values to the AI entity every time.
	

	//First see if the entity definition exists.
	const idDeclEntityDef* itemDef;
	itemDef = gameLocal.FindEntityDef(entityName, false);
	if (itemDef != nullptr)
	{
		idVec3 newvec = itemDef->dict.GetVector(keyname.c_str());
		if (newvec != vec3_zero)
		{
			return newvec;
		}
	}

	//cannot find entity definition, so fall back to searching for entity in the world.
	idEntity *ent = gameLocal.FindEntity(entityName.c_str());
	if (ent != nullptr)
	{
		idVec3 newvec = ent->spawnArgs.GetVector(keyname.c_str());
		if (newvec != vec3_zero)
		{
			return newvec;
		}
	}

	gameLocal.Error("GetBeltAttachmentVec3: '%s' failed to get attachment info: '%s'", entityName.c_str(), keyname.c_str());
	return vec3_zero;
}

void idActor::AttachBeltattachments()
{
	if (hasAttachedBeltItems)
		return;

	hasAttachedBeltItems = true;

	//BC belt attachments.
	#define STRINGLENGTH_BELTATTACH 14 //how many characters are in 'def_beltattach'
	const idKeyValue *bv;
	bv = this->spawnArgs.MatchPrefix("def_beltattach", NULL);
	while (bv)
	{
		//Get the suffix. This is so we can have multiple belt attachments.
		idStr suffix = bv->GetKey();
		suffix = suffix.Right(suffix.Length() - STRINGLENGTH_BELTATTACH);

		//Attachment name.
		idStr attachDefname = spawnArgs.GetString(idStr::Format("def_beltattach%s", suffix.c_str()));

		//Strip any leading/trailing spaces, in case any were accidentally included in radiant.
		attachDefname.StripLeading(' ');
		attachDefname.StripTrailingWhitespace();

		if (attachDefname.Length() <= 0 || attachDefname.IsEmpty()) //If empty slot, then skip.
		{
			bv = this->spawnArgs.MatchPrefix("def_beltattach", bv);
			continue;
		}

		idStr beltAttachAngleKeyname = idStr::Format("beltattachangle%s", suffix.c_str());
		idStr beltAttachOffsetKeyname = idStr::Format("beltattachOffset%s", suffix.c_str());

		// SW: use alternative angle/offset for heavy pirates, since they have a different model
		if (spawnArgs.GetBool("beltattach_heavy", "0"))
		{
			beltAttachAngleKeyname = idStr::Format("beltattachangle_heavy%s", suffix.c_str());
			beltAttachOffsetKeyname = idStr::Format("beltattachOffset_heavy%s", suffix.c_str());
		}

		//Attachment angle.
		idVec3 attachAngleVec = spawnArgs.GetVector(beltAttachAngleKeyname.c_str()); //First check value on the actor.
		if (attachAngleVec == vec3_zero)
		{
			//if no value, then fall back to the attached item's entity definition.
			attachAngleVec = GetBeltAttachmentVec3(attachDefname, beltAttachAngleKeyname);
		}
		idAngles attachAng = idAngles(attachAngleVec.x, attachAngleVec.y, attachAngleVec.z);

		//Attachment offset.
		idVec3 attachOffset = spawnArgs.GetVector(beltAttachOffsetKeyname.c_str()); //First check value on actor.
		if (attachOffset == vec3_zero)
		{
			//if no value, then fall back to the attached item's entity definition.
			attachOffset = GetBeltAttachmentVec3(attachDefname, beltAttachOffsetKeyname);
		}

		//Attachment bone. Default to 'body'
		idStr attachJoint = spawnArgs.GetString(idStr::Format("beltattachJoint%s", suffix.c_str()), "body");

		//Attach it.
		AttachItem(attachDefname, attachJoint.c_str(), attachOffset, attachAng);

		//Iterate to next item, if any.
		bv = this->spawnArgs.MatchPrefix("def_beltattach", bv);
	}
}

idEntity *idActor::FindBeltattachEnt(idStr _name, idVec3 _itemPos, idMat3 _itemAxis)
{
	idEntity *ent = NULL;

	//See if entity definition exists.
	const idDeclEntityDef *itemDef;
	itemDef = gameLocal.FindEntityDef(_name, false);
	if (itemDef != NULL)
	{
		idDict args;
		args.Clear();
		args.Set("classname", _name.c_str());
		args.SetInt("solid", 0);
		args.SetBool("noclipmodel", true);
		args.SetVector("origin", _itemPos);
		args.SetMatrix("rotation", _itemAxis);
		gameLocal.SpawnEntityDef(args, &ent);
		if (ent != NULL)
		{
			return ent;
		}

		gameLocal.Warning("FindBeltattachEnt(): failed to spawn/find entitydef '%s'", _name.c_str());
	}

	//Try to find object in world via name.
	ent = gameLocal.FindEntity(_name.c_str());
	if (ent != NULL)
	{
		ent->SetOrigin(_itemPos); //Move the key in the world to the correct position on belt, so that we can bind it.
		return ent;
	}

	gameLocal.Warning("FindBeltattachEnt(): failed to find entity in world '%s'", _name.c_str());
	return NULL;
}

//This is used to attach things to actor. i.e. keycards
void idActor::AttachItem(idStr itemDefName, idStr jointName, idVec3 originOffset, idAngles angleOffset)
{
	if (itemDefName.Length() <= 0) //if empty, then skip.
		return;

	idVec3			origin;
	idMat3			axis;
	jointHandle_t	joint;
	idAttachInfo	&attach = attachmentsToDrop.Alloc();

	//Find the joint.
	joint = animator.GetJointHandle(jointName);
	if (joint == INVALID_JOINT)
	{
		gameLocal.Error("Joint '%s' not found for attaching '%s' on '%s'", jointName.c_str(), itemDefName.c_str(), name.c_str());
	}
	
	//Find position.
	GetJointWorldTransform(joint, gameLocal.time, origin, axis);
	idVec3 attachmentPos = origin + originOffset * renderEntity.axis;

	//Find angle.
	idMat3 rotate = angleOffset.ToMat3();
	idMat3 newAxis = rotate * axis;

	//Spawn the item.	
	idEntity *ent = FindBeltattachEnt(itemDefName, attachmentPos, newAxis);
	if (ent == NULL)
	{
		gameLocal.Error("Actor '%s' failed to spawn attachitem '%s'", GetName(), jointName.c_str());
	}

	//BC 3-11-2025: fixed bug where belt attachment angles were bad. Force it to change angle here.
	ent->SetAxis(newAxis);

	attach.channel = animator.GetChannelForJoint(joint);
	attach.ent = ent;	
	
	ent->BindToJoint(this, joint, true);
	ent->cinematic = cinematic;
	ent->fl.takedamage = false;
}


void idActor::SetupVisionbox(void)
{
	idDict			args;
	idStr			headjointname;
	jointHandle_t	headjoint;
	idVec3			headPos;
	idMat3			headAxis;
	//idAngles		spawnAngle(0, 90, 0);
	

	if (this->IsType(idPlayer::Type))
		return;

	headjointname = spawnArgs.GetString("vision_joint");
	headjoint = animator.GetJointHandle(headjointname);
	if (headjoint == INVALID_JOINT)
	{
		gameLocal.Error("SetupVisionbox: joint '%s' not found for 'vision_joint' on '%s'", headjointname.c_str(), name.c_str());
		return;
	}
	else
	{
		animator.GetJointTransform(headjoint, gameLocal.time, headPos, headAxis);
	}

	//BC vision box setup
	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin() + headPos);
	args.SetVector("mins", idVec3(0, -VISIONBOX_WIDTH, VISIONBOX_MINZ)); //make visionbox reach the floor so AI can see crawling enemies.
	args.SetVector("maxs", idVec3(VISIONBOX_MAXLENGTH, VISIONBOX_WIDTH, VISIONBOX_MAXZ));
	args.SetFloat("delay", .3f);
	visionBox = (idTrigger *)gameLocal.SpawnEntityType(idTrigger::Type, &args);

	//spawnAngle = GetPhysics()->GetAxis().ToAngles();
	//spawnAngle.yaw -= 90;
	
	idAngles myAngle = idAngles(0, viewAxis.ToAngles().yaw, 0);
	visionBox->SetAxis(myAngle.ToMat3());

	visionBox->BindToJoint(this, headjoint, true);
	


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
void idActor::SetupHead( void ) {
	idAFAttachment		*headEnt;
	idStr				jointName;
	const char			*headModel;
	jointHandle_t		joint;
	jointHandle_t		damageJoint;
	int					i;
	const idKeyValue	*sndKV;

	if ( gameLocal.isClient ) {
		return;
	}

	headModel = spawnArgs.GetString( "def_head", "" );
	if ( headModel[ 0 ] ) {
		jointName = spawnArgs.GetString( "head_joint" );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'head_joint' on '%s'", jointName.c_str(), name.c_str() );
		}

		// set the damage joint to be part of the head damage group
		damageJoint = joint;
		for( i = 0; i < damageGroups.Num(); i++ ) {
			if ( damageGroups[ i ] == "head" ) {
				damageJoint = static_cast<jointHandle_t>( i );
				break;
			}
		}

		// copy any sounds in case we have frame commands on the head
		idDict	args;
		sndKV = spawnArgs.MatchPrefix( "snd_", NULL );
		while( sndKV ) {
			args.Set( sndKV->GetKey(), sndKV->GetValue() );
			sndKV = spawnArgs.MatchPrefix( "snd_", sndKV );
		}

#ifdef _D3XP
		// copy slowmo param to the head
		args.SetBool( "slowmo", spawnArgs.GetBool("slowmo", "1") );
#endif


		headEnt = static_cast<idAFAttachment *>( gameLocal.SpawnEntityType( idAFAttachment::Type, &args ) );
		headEnt->SetName( va( "%s_head", name.c_str() ) );
		headEnt->SetBody( this, headModel, damageJoint );
		head = headEnt;

#ifdef _D3XP
		idStr xSkin;
		if ( spawnArgs.GetString( "skin_head_xray", "", xSkin ) ) {
			headEnt->xraySkin = declManager->FindSkin( xSkin.c_str() );
			headEnt->UpdateModel();
		}
#endif

		idVec3		origin;
		idMat3		axis;
		idAttachInfo &attach = attachments.Alloc();
		attach.channel = animator.GetChannelForJoint( joint );
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		origin = renderEntity.origin + ( origin + modelOffset ) * renderEntity.axis;
		attach.ent = headEnt;
		headEnt->SetOrigin( origin );
		headEnt->SetAxis( renderEntity.axis );
		headEnt->BindToJoint( this, joint, true );
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

	savefile->WriteInt( rank ); //  int rank
	savefile->WriteMat3( viewAxis ); //  idMat3 viewAxis

	//  idLinkList<idActor> enemyNode
	savefile->WriteInt( enemyList.Num() ); //  idLinkList<idActor> enemyList
	for ( idActor *ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		savefile->WriteObject( ent );
	}

	savefile->WriteInt( energyShieldCurrent ); //  int energyShieldCurrent
	savefile->WriteInt( energyShieldMax ); //  int energyShieldMax


	savefile->WriteVec3( lastDamageOrigin ); //  idVec3 lastDamageOrigin
	savefile->WriteVec3( lastAttackerDamageOrigin ); //  idVec3 lastAttackerDamageOrigin
	savefile->WriteObject( lastAttacker ); // idEntityPtr<idEntity> lastAttacker
	savefile->WriteInt( lastDamagedTime ); //  int lastDamagedTime


	savefile->WriteInt( stunStartTime ); //  int stunStartTime
	savefile->WriteInt( stunTime ); //  int stunTime
	savefile->WriteString( stunAnimationName ); //  idString stunAnimationName


	savefile->WriteInt( vo_lastSpeakTime ); //  int vo_lastSpeakTime
	savefile->WriteInt( vo_lastCategory ); //  int vo_lastCategory

	savefile->WriteInt( pointdefenseAmount ); //  int pointdefenseAmount

	savefile->WriteInt( attachmentsToDrop.Num() ); //  idList<idAttachInfo> attachments
	for ( int i = 0; i < attachmentsToDrop.Num(); i++ ) {
		attachmentsToDrop[i].ent.Save( savefile );
		savefile->WriteInt( attachmentsToDrop[i].channel );
	}

	savefile->WriteBool( hasHelmet ); //  bool hasHelmet

	savefile->WriteFloat( fovDot ); //  float fovDot
	savefile->WriteVec3( eyeOffset ); //  idVec3 eyeOffset
	savefile->WriteVec3( modelOffset ); //  idVec3 modelOffset

	savefile->WriteAngles( deltaViewAngles ); //  idAngles deltaViewAngles

	savefile->WriteInt( pain_debounce_time ); //  int pain_debounce_time
	savefile->WriteInt( pain_delay ); //  int pain_delay
	savefile->WriteInt( pain_threshold ); //  int pain_threshold

	savefile->WriteInt( damageGroups.Num() );  //  idStrList damageGroups
	for( int i = 0; i < damageGroups.Num(); i++ ) {
		savefile->WriteString( damageGroups[ i ] );
	}
	savefile->WriteInt( damageScale.Num() ); //  idList<float> damageScale
	for( int i = 0; i < damageScale.Num(); i++ ) {
		savefile->WriteFloat( damageScale[ i ] );
	}
	savefile->WriteInt( damageBonus.Num() ); //  idList<int> damageBonus
	for( int i = 0; i < damageBonus.Num(); i++ ) {
		savefile->WriteInt( damageBonus[ i ] );
	}

	savefile->WriteBool( use_combat_bbox ); //  bool use_combat_bbox

	head.Save( savefile ); //  idEntityPtr<idAFAttachment> head

	savefile->WriteInt( copyJoints.Num() ); //  idList<copyJoints_t> copyJoints
	for( int i = 0; i < copyJoints.Num(); i++ ) {
		savefile->WriteInt( copyJoints[i].mod );
		savefile->WriteJoint( copyJoints[i].from );
		savefile->WriteJoint( copyJoints[i].to );
	}

	idToken token;
	//FIXME: this is unneccesary
	if ( state ) { // const function_t		*state;
		idLexer src( state->Name(), idStr::Length( state->Name() ), "idAI::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

	if ( idealState ) { // const function_t		*idealState;
		idLexer src( idealState->Name(), idStr::Length( idealState->Name() ), "idAI::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

	savefile->WriteJoint( leftEyeJoint ); //  saveJoint_t leftEyeJoint
	savefile->WriteJoint( rightEyeJoint ); //  saveJoint_t rightEyeJoint
	savefile->WriteJoint( soundJoint ); //  saveJoint_t soundJoint

	walkIK.Save(savefile); //  idIK_Walk walkIK

	savefile->WriteString( animPrefix ); //  idString animPrefix
	savefile->WriteString( painAnim ); //  idString painAnim

	savefile->WriteInt( blink_anim ); //  int blink_anim
	savefile->WriteInt( blink_time ); //  int blink_time
	savefile->WriteInt( blink_min ); //  int blink_min
	savefile->WriteInt( blink_max ); //  int blink_max

	// script variables
	savefile->WriteObject( scriptThread ); //  idThread * scriptThread
	savefile->WriteString( waitState ); //  idString waitState

	headAnim.Save( savefile ); //  idAnimState headAnim
	torsoAnim.Save( savefile ); //  idAnimState torsoAnim
	legsAnim.Save( savefile ); //  idAnimState legsAnim

	savefile->WriteBool( allowPain ); //  bool allowPain
	savefile->WriteBool( allowEyeFocus ); //  bool allowEyeFocus
	savefile->WriteBool( finalBoss ); //  bool finalBoss

	savefile->WriteInt( painTime ); //  int painTime

	savefile->WriteInt( attachments.Num() ); //  idList<idAttachInfo> attachments
	for ( int i = 0; i < attachments.Num(); i++ ) {
		attachments[i].ent.Save( savefile );
		savefile->WriteInt( attachments[i].channel );
	}
	savefile->WriteCheckSizeMarker();

	savefile->WriteInt( damageCap ); //  int damageCap

	savefile->WriteJoint( leftFootJoint ); //  saveJoint_t leftFootJoint
	savefile->WriteJoint( rightFootJoint ); //  saveJoint_t rightFootJoint
	savefile->WriteObject( visionBox ); //  idTrigger * visionBox
	savefile->WriteJoint( eyeJoint ); //  saveJoint_t eyeJoint

	savefile->WriteInt( energyShieldTimer ); //  int energyShieldTimer
	savefile->WriteInt( energyShieldState ); //  int energyShieldState
	savefile->WriteObject( energyShieldModel ); //  idEntity * energyShieldModel

	savefile->WriteString( customIdleAnim ); //  idString customIdleAnim

	savefile->WriteObject( helmetModel ); //  idEntity * helmetModel

	savefile->WriteBool( hasAttachedBeltItems ); //  bool hasAttachedBeltItems
}

/*
================
idActor::Restore

unarchives object from save game file
================
*/
void idActor::Restore( idRestoreGame *savefile ) {
	int num;

	savefile->ReadInt( rank ); //  int rank
	savefile->ReadMat3( viewAxis ); //  idMat3 viewAxis

	//  idLinkList<idActor> enemyNode
	savefile->ReadInt( num ); //  idLinkList<idActor> enemyList
	for ( int i = 0; i < num; i++ ) {
		idActor* ent = nullptr;
		savefile->ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->enemyNode.AddToEnd( enemyList );
		}
	}

	savefile->ReadInt( energyShieldCurrent ); //  int energyShieldCurrent
	savefile->ReadInt( energyShieldMax ); //  int energyShieldMax

	savefile->ReadVec3( lastDamageOrigin ); //  idVec3 lastDamageOrigin
	savefile->ReadVec3( lastAttackerDamageOrigin ); //  idVec3 lastAttackerDamageOrigin
	savefile->ReadObject( lastAttacker ); // idEntityPtr<idEntity> lastAttacker
	savefile->ReadInt( lastDamagedTime ); //  int lastDamagedTime

	savefile->ReadInt( stunStartTime ); //  int stunStartTime
	savefile->ReadInt( stunTime ); //  int stunTime
	savefile->ReadString( stunAnimationName ); //  idString stunAnimationName


	savefile->ReadInt( vo_lastSpeakTime ); //  int vo_lastSpeakTime
	savefile->ReadInt( vo_lastCategory ); //  int vo_lastCategory

	savefile->ReadInt( pointdefenseAmount ); //  int pointdefenseAmount

	savefile->ReadInt( num ); //  idList<idAttachInfo> attachments
	attachmentsToDrop.SetNum( num );
	for ( int i = 0; i < attachmentsToDrop.Num(); i++ ) {
		attachmentsToDrop[i].ent.Restore( savefile );
		savefile->ReadInt( attachmentsToDrop[i].channel );
	}

	savefile->ReadBool( hasHelmet ); //  bool hasHelmet

	savefile->ReadFloat( fovDot ); //  float fovDot
	savefile->ReadVec3( eyeOffset ); //  idVec3 eyeOffset
	savefile->ReadVec3( modelOffset ); //  idVec3 modelOffset

	savefile->ReadAngles( deltaViewAngles ); //  idAngles deltaViewAngles

	savefile->ReadInt( pain_debounce_time ); //  int pain_debounce_time
	savefile->ReadInt( pain_delay ); //  int pain_delay
	savefile->ReadInt( pain_threshold ); //  int pain_threshold

	savefile->ReadInt( num );  //  idStrList damageGroups
	damageGroups.SetNum( num );
	damageGroups.SetGranularity(1);
	for( int i = 0; i < damageGroups.Num(); i++ ) {
		savefile->ReadString( damageGroups[ i ] );
	}
	savefile->ReadInt( num); //  idList<float> damageScale
	damageScale.SetNum( num );
	for( int i = 0; i < damageScale.Num(); i++ ) {
		savefile->ReadFloat( damageScale[ i ] );
	}
	savefile->ReadInt( num ); //  idList<int> damageBonus
	damageBonus.SetNum( num );
	for( int i = 0; i < damageBonus.Num(); i++ ) {
		savefile->ReadInt( damageBonus[ i ] );
	}

	savefile->ReadBool( use_combat_bbox ); //  bool use_combat_bbox

	head.Restore( savefile ); //  idEntityPtr<idAFAttachment> head

	savefile->ReadInt( num ); //  idList<copyJoints_t> copyJoints
	copyJoints.SetNum( num );
	for( int i = 0; i < copyJoints.Num(); i++ ) {
		int val;
		savefile->ReadInt( val );
		copyJoints[i].mod = (jointModTransform_t)val;
		savefile->ReadJoint( copyJoints[i].from );
		savefile->ReadJoint( copyJoints[i].to );
	}

	idStr statename;
	savefile->ReadString( statename );  // const function_t *state
	if ( statename.Length() > 0 ) {
		state = GetScriptFunction( statename );
	}

	savefile->ReadString( statename ); // const function_t *idealState;
	if ( statename.Length() > 0 ) {
		idealState = GetScriptFunction( statename );
	}

	savefile->ReadJoint( leftEyeJoint ); //  saveJoint_t leftEyeJoint
	savefile->ReadJoint( rightEyeJoint ); //  saveJoint_t rightEyeJoint
	savefile->ReadJoint( soundJoint ); //  saveJoint_t soundJoint

	walkIK.Restore(savefile); //  idIK_Walk walkIK

	savefile->ReadString( animPrefix ); //  idString animPrefix
	savefile->ReadString( painAnim ); //  idString painAnim

	savefile->ReadInt( blink_anim ); //  int blink_anim
	savefile->ReadInt( blink_time ); //  int blink_time
	savefile->ReadInt( blink_min ); //  int blink_min
	savefile->ReadInt( blink_max ); //  int blink_max

	// script variables
	savefile->ReadObject( reinterpret_cast<idClass *&>( scriptThread ) ); //  idThread * scriptThread
	savefile->ReadString( waitState ); //  idString waitState

	headAnim.Restore( savefile ); //  idAnimState headAnim
	torsoAnim.Restore( savefile ); //  idAnimState torsoAnim
	legsAnim.Restore( savefile ); //  idAnimState legsAnim

	savefile->ReadBool( allowPain ); //  bool allowPain
	savefile->ReadBool( allowEyeFocus ); //  bool allowEyeFocus
	savefile->ReadBool( finalBoss ); //  bool finalBoss

	savefile->ReadInt( painTime ); //  int painTime

	savefile->ReadInt( num ); //  idList<idAttachInfo> attachments
	for ( int i = 0; i < num; i++ ) {
		idAttachInfo &attach = attachments.Alloc();
		attach.ent.Restore( savefile );
		savefile->ReadInt( attach.channel );
	}
	savefile->ReadCheckSizeMarker();

	savefile->ReadInt( damageCap ); //  int damageCap

	savefile->ReadJoint( leftFootJoint ); //  saveJoint_t leftFootJoint
	savefile->ReadJoint( rightFootJoint ); //  saveJoint_t rightFootJoint
	savefile->ReadObject( reinterpret_cast<idClass *&>( visionBox ) ); //  idTrigger * visionBox
	savefile->ReadJoint( eyeJoint ); //  saveJoint_t eyeJoint

	savefile->ReadInt( energyShieldTimer ); //  int energyShieldTimer
	savefile->ReadInt( energyShieldState ); //  int energyShieldState
	savefile->ReadObject( energyShieldModel ); //  idEntity * energyShieldModel

	savefile->ReadString( customIdleAnim ); //  idString customIdleAnim

	savefile->ReadObject( helmetModel ); //  idEntity * helmetModel

	savefile->ReadBool( hasAttachedBeltItems ); //  bool hasAttachedBeltItems

	//af.SetAnimator( GetAnimator() );
}

/*
================
idActor::Hide
================
*/
void idActor::Hide( void ) {
	idEntity *ent;
	idEntity *next;

	idAFEntity_Base::Hide();
	if ( head.GetEntity() ) {
		head.GetEntity()->Hide();
	}

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			ent->Hide();
			if ( ent->IsType( idLight::Type ) ) {
				static_cast<idLight *>( ent )->Off();
			}
		}
	}
	UnlinkCombat();
}

/*
================
idActor::Show
================
*/
void idActor::Show( void ) {
	idEntity *ent;
	idEntity *next;

	idAFEntity_Base::Show();
	if ( head.GetEntity() ) {
		head.GetEntity()->Show();
	}
	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			ent->Show();
			if ( ent->IsType( idLight::Type ) ) {
#ifdef _D3XP
				if(!spawnArgs.GetBool("lights_off", "0")) {
					static_cast<idLight *>( ent )->On();
				}
#endif


			}
		}
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
void idActor::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {
	idEntity *ent;
	idEntity *next;

	idEntity::ProjectOverlay( origin, dir, size, material );

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) {
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) {
			if ( ent->fl.takedamage && ent->spawnArgs.GetBool( "bleed" ) ) {
				ent->ProjectOverlay( origin, dir, size, material );
			}
		}
	}
}

/*
================
idActor::LoadAF
================
*/
bool idActor::LoadAF( void ) {
	idStr fileName;

	if ( !spawnArgs.GetString( "ragdoll", "*unknown*", fileName ) || !fileName.Length() ) {
		return false;
	}
	af.SetAnimator( GetAnimator() );
	return af.Load( this, fileName );
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


	//BC
	jointname = spawnArgs.GetString("bone_leftFoot");
	if (*jointname != '\0')
	{
		leftFootJoint = animator.GetJointHandle(jointname);
	}


	jointname = spawnArgs.GetString("bone_rightFoot");
	if (*jointname != '\0')
	{
		rightFootJoint = animator.GetJointHandle(jointname);
	}

	jointname = spawnArgs.GetString("bone_focus");
	if (*jointname != '\0')
	{
		eyeJoint = animator.GetJointHandle(jointname);
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
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomFloat() * ( blink_max - blink_min );
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
bool idActor::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	if ( soundJoint != INVALID_JOINT ) {
		animator.GetJointTransform( soundJoint, gameLocal.time, origin, axis );
		origin += modelOffset;
		axis = viewAxis;
	} else {
		origin = GetPhysics()->GetGravityNormal() * -eyeOffset.z;
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
void idActor::SetState( const function_t *newState, bool updateScriptImmediate ) {

	if (ai_showPlayerState.GetBool() && this->IsType(idPlayer::Type)) { // blendo eric: collect start debug states
		for (int chidx = 0; chidx < DEBUG_Anim_State_Count; chidx++) {
			int channel = DEBUG_Anim_State_Channels[chidx];
			DEBUG_Anim_State_Changed[channel].Append(GetAnimState(channel));
		}
	}

	if ( !newState ) {
		gameLocal.Error( "idActor::SetState: Null state" );
	}

	if ( ai_debugScript.GetInteger() == entityNumber ) {
		gameLocal.Printf( "%d: %s: State: %s\n", gameLocal.time, name.c_str(), newState->Name() );
	}

	state = newState;
	idealState = state;
	scriptThread->CallFunction( this, state, true );

	if (updateScriptImmediate) { // blendo eric
		UpdateScript();
	}
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const char *statename, bool updateScriptImmediate ) {
	const function_t *newState;

	newState = GetScriptFunction( statename );
	SetState( newState, updateScriptImmediate);
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


	if (ai_showPlayerState.GetInteger() == 1 && this->IsType(idPlayer::Type)) {  // blendo eric: print state changes post update
		for (int chidx = 0; chidx < DEBUG_Anim_State_Count; chidx++) {
			int channel = DEBUG_Anim_State_Channels[chidx];
			if (DEBUG_Anim_State_Changed[channel].Find(">") >= 0) {
				DEBUG_Anim_State_Changed[channel].Replace(">", "^7>^6");
				common->Printf("%05d Code call state ^4%s ^6%s ^2[%s]\n", gameLocal.time, ANIMCHANNEL_Names[channel], DEBUG_Anim_State_Changed[channel].c_str(), GetAnimName(channel));
			}
			DEBUG_Anim_State_Changed[channel].Clear();
		}
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
void idActor::SetFOV( float fov ) {
	fovDot = (float)cos( DEG2RAD( fov * 0.5f ) );
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
idVec3 idActor::GetEyePosition( void ) const {
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

bool idActor::IsTargetImmediatelyFrontOfMe(const idVec3 &pos) const
{
	#define VERTCHECK_I_AM_ABOVE 32
	#define VERTCHECK_I_AM_BELOW 128

	//Do a verticality check.
	float deltaDist = idMath::Fabs(this->GetPhysics()->GetOrigin().z - pos.z);
	if (pos.z < this->GetPhysics()->GetOrigin().z && deltaDist > VERTCHECK_I_AM_ABOVE) //if I am above my target...
	{		
		return false;
	}
	else if (pos.z > this->GetPhysics()->GetOrigin().z && deltaDist > VERTCHECK_I_AM_BELOW) //if I am above my target...
	{
		return false;
	}

	//Check if target is in front of me.
	idAngles myAngle = idAngles(0, viewAxis.ToAngles().yaw, 0);
	idVec3 dirToEnemy;
	idAngles angToEnemy;
	dirToEnemy = pos - GetPhysics()->GetOrigin();
	angToEnemy = dirToEnemy.ToAngles();
	angToEnemy.pitch = 0;
	angToEnemy.roll = 0;
	float vdot = DotProduct(myAngle.ToForward(), angToEnemy.ToForward());
	if (vdot < 0)
		return false; //target is BEHIND me. Exit here.

	//Do distance check.
	float distanceToPos = (this->GetPhysics()->GetOrigin() - pos).Length();
	if (distanceToPos <= DARKNESS_VIEWRANGE)
		return true;

	return false;
}

/*
=====================
idActor::CheckFOV
=====================
*/
//Return TRUE if can see 'pos' point.
bool idActor::CheckFOV( const idVec3 &pos ) const
{
	//BC the FOV is set via spawnargs "fov" and "fov_combat"

	if ( fovDot == 1.0f )
	{
		return true;
	}

	idVec3	delta;

	if (visionBox != NULL)
	{
		//We need to handle situations where the actor is looking straight down, thus making the target not technically
		//inside the visionbox. So, we do an extra check to see if the target is directly in front of actor.
		bool targetImmediatelyInFrontOfMe = IsTargetImmediatelyFrontOfMe(pos);

		if (visionBox->GetPhysics()->GetAbsBounds().ContainsPoint(pos) || targetImmediatelyInFrontOfMe) //BC This is where the visionbox gets checked.
		{
			//IS in visionbox.
			//int verticalHeightDelta;
			float vdot;
			idVec3 dirToEnemy;
			idAngles angToEnemy;

			//Blindspot check. Get Angle to Enemy & Angle of Visionbox and do dotproduct on them. The blindspot is to the left and right side of the actor.
			//We want the AI to NOT see things immediately to their left and right.
			//So basically the vision box ends up looking like:
			//          -------------
			//			|			|
			//			|			|
			//			|			|
			//			|			|
			//			|			|
			//			|			|
			//			|			|
			//			|			|
			//			|			|
			//			\           /
			//			 \		   /
			//			  \       /
			//			   \     /
			//              \   /
			//               \ /
			//              actor
			//
			dirToEnemy = pos - GetPhysics()->GetOrigin(); //Get angle from my target candidate to me.
			angToEnemy = dirToEnemy.ToAngles();
			angToEnemy.pitch = 0;
			angToEnemy.roll = 0;
			vdot = DotProduct(idAngles(0, visionBox->GetPhysics()->GetAxis().ToAngles().yaw, 0).ToForward(), angToEnemy.ToForward());

			//gameRenderWorld->DebugArrow(colorGreen, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + idAngles(0, visionBox->GetPhysics()->GetAxis().ToAngles().yaw, 0).ToForward() * 128, 4, 1000);
			//gameRenderWorld->DebugArrow(colorOrange, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + angToEnemy.ToForward() * 128, 4, 1000);
			//common->Printf("vdot: %f   %f\n", vdot, fovDot);

			float blindspotThreshold = fovDot;
			if (vdot < blindspotThreshold)
			{
				//The target is inside the blind spot... do NOT see this thing.				
				return false;
			}

			if (ai_debugPerception.GetInteger() == 1)
			{
				idVec3 midpoint = (GetEyePosition() + pos) / 2.0f;

				gameRenderWorld->DebugArrow(colorGreen, GetEyePosition(), pos, 2, 100);
				gameRenderWorld->DrawText("IN VISIONBOX", midpoint, .4f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 100);
			}

			//vertical height check.
			//verticalHeightDelta = pos.z - this->GetPhysics()->GetOrigin().z;
			//if (verticalHeightDelta >= VERTICALHEIGHT_SIGHTTHRESHOLD)
			//{
			//	//enemy is in vision cone but the enemy is ABOVE me beyond the vertical threshold, so ignore the enemy.
			//	if (ai_debugPerception.GetBool())
			//	{
			//		idVec3 midpoint = (GetEyePosition() + pos) / 2.0f;
			//		gameRenderWorld->DebugArrow(colorRed, GetEyePosition(), pos, 2, 1000);
			//		gameRenderWorld->DrawTextA("Ignoring enemy: beyond vertical height.", midpoint, 1.0f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 1000);
			//	}
			//
			//	return false;
			//}

			return true;
		}
		else if (ai_debugPerception.GetInteger() == 1)
		{
			//Is NOT in visionbox.

			idVec3 midpoint = (GetEyePosition() + pos) / 2.0f;

			gameRenderWorld->DebugArrow(colorRed, GetEyePosition(), pos, 2, 100);
			gameRenderWorld->DrawText("NOT IN VISIONBOX", midpoint, .4f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 100);
		}
	}



	return false;

	//bc default vision cone code:
	/*
	delta = pos - GetEyePosition();

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	dot = viewAxis[ 0 ] * delta;

	return ( dot >= fovDot );*/
}

/*
=====================
idActor::CanSee
=====================
*/

bool idActor::CanSee( idEntity *ent, bool useFov ) {
	trace_t		tr;
	idVec3		myEye;
	idVec3		toPos;

	if ( ent->IsHidden() )
	{
		return false;
	}

	//BC ignore player in noclip or notarget.
	if (ent->IsType( idPlayer::Type))
	{
		if (static_cast<idPlayer *>(ent)->noclip || static_cast<idPlayer *>(ent)->fl.notarget)
		{
			return false;
		}
	}

	if ( ent->IsType( idActor::Type ) )
	{
		//Do a traceline check to the target's EYES.
		toPos = ( ( idActor * )ent )->GetEyePosition();
	}
	else
	{
		//If cannot do that, then just traceline to the origin point.
		toPos = ent->GetPhysics()->GetOrigin();
	}

	//if (useFov)
	//{
	//	//useFov = is the thing in a 180 degree cone in front of me.
	//	float dot;
	//	idVec3 delta;
	//
	//	delta = toPos - GetEyePosition();
	//
	//	// get our gravity normal
	//	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();
	//
	//	// infinite vertical vision, so project it onto our orientation plane
	//	delta -= gravityDir * (gravityDir * delta);
	//
	//	delta.Normalize();
	//	dot = viewAxis[0] * delta;
	//
	//	if (dot >= fovDot)
	//	{
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	//BC do not do feet check... only check if we can see enemy eyeballs
	//if ( useFov && !CheckFOV( toPos ) )
	//{
	//	if (ent->IsType(idActor::Type) )
	//	{
	//		//BC if actor and cannot see their eyes, then check their feet position.
	//		if (!CheckFOV(ent->GetPhysics()->GetOrigin()))
	//		{
	//			return false;
	//		}
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	
	myEye = GetEyePosition();
	//myEye = GetEyeBonePosition();//BC for lean animations, we need to know the exact position of the eye bone.
	//gameRenderWorld->DebugArrowSimple(myEye, 500);


	

	//gameLocal.clip.TracePoint( tr, myEye, toPos, MASK_SOLID | MASK_OPAQUE, this );
	gameLocal.clip.TracePoint(tr, myEye, toPos, MASK_SOLID, this);
	if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == ent ) )
	{
		//gameRenderWorld->DebugArrow(colorGreen, myEye, tr.endpos, 4, 5000);
		return true;
	}


	//check if we're looking through glass.
	int surfaceType;
	surfaceType = tr.c.material != NULL ? tr.c.material->GetSurfaceType() : SURFTYPE_NONE;

	bool isBrittleFracture = false;
	if (tr.c.entityNum != ENTITYNUM_WORLD && tr.c.entityNum != ENTITYNUM_NONE)
	{
		if (gameLocal.entities[tr.c.entityNum]->IsType(idBrittleFracture::Type))
		{
			isBrittleFracture = true;
		}
	}

	if (surfaceType == SURFTYPE_GLASS || isBrittleFracture)
	{
		//Looking through a glass surface.
		//Try to "poke" tracelines through the glass.

		idVec3 glassTrDir = toPos - myEye;
		glassTrDir.Normalize();
		idVec3 glassTrStart = tr.endpos + glassTrDir * 1.0f; //Poke a little past the glass.

		#define MAX_PANES_OF_GLASS 4 //the amount of glass panes we can "poke" through.
		for (int i = 0; i < MAX_PANES_OF_GLASS; i++)
		{
			trace_t glassTr;
			gameLocal.clip.TracePoint(glassTr, glassTrStart, toPos, MASK_SOLID, this);
			if (glassTr.fraction >= 1.0f || (gameLocal.GetTraceEntity(glassTr) == ent))
			{
				return true; //I was able to make a tracepoint to the enemy target. Return true.
			}

			int glassSurfCheck;
			glassSurfCheck = glassTr.c.material != NULL ? glassTr.c.material->GetSurfaceType() : SURFTYPE_NONE;
			if (glassSurfCheck == SURFTYPE_GLASS)
			{
				//if hit another pane of glass, then continue....
				continue;
			}
			else
			{
				return false;
			}
		}
	}




	//Couldn't see actor eye. Now do a check to see if I have LOS to the sides of the head.
	//This check is to handle if the player is behind a mesh grate or something; the tracepoint
	//checks a few locations on the head.
	idMat3 enemyViewaxis;
	idVec3 enemyDir;
	idVec3 gravityDir;

	enemyViewaxis = (toPos - myEye).ToAngles().ToForward().ToMat3(); //Get angle of enemy to me.
	gravityDir = gameLocal.GetLocalPlayer()->GetPhysics()->GetGravityNormal();
	enemyDir = (enemyViewaxis[0] - gravityDir * (gravityDir * enemyViewaxis[0])).Cross(gravityDir); //Get angle perpendicular to 'playerViewaxis'.
	
	//BC 3-11-2025: make more offset checks to handle more types of geometry, such as grate bars.
	#define PERCEPTIONOFFSET 1.5f
	idVec2 distanceArrays[] = { idVec2(PERCEPTIONOFFSET, 0), idVec2(-PERCEPTIONOFFSET, 0), idVec2(0, PERCEPTIONOFFSET), idVec2(0, -PERCEPTIONOFFSET) };
	for (int i = 0; i < 4; i++)
	{
		idVec3 headPos = toPos + (enemyDir * distanceArrays[i].x) + (idVec3(0,0,distanceArrays[i].y));
		gameLocal.clip.TracePoint(tr, myEye, headPos, MASK_SOLID, this);		
		if (tr.fraction >= 1.0f || (gameLocal.GetTraceEntity(tr) == ent))
		{
			return true;
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

	if ( combatModel ) {
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
	GetPhysics()->DisableClip();

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

	// drop any items the actor is holding. This is what gets called when a monster gets killed.
	idMoveableItem::DropItems( this, "death", NULL );

	// drop any articulated figures the actor is holding
	idAFEntity_Base::DropAFs( this, "death", NULL );

	RemoveAttachments();
	DropBeltAttachments();//BC

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
void idActor::RemoveAttachments( void ) {
	int i;
	idEntity *ent;

	// remove any attached entities
	for( i = 0; i < attachments.Num(); i++ ) {
		ent = attachments[ i ].ent.GetEntity();
		if ( ent && ent->spawnArgs.GetBool( "remove" ) ) {
			ent->PostEventMS( &EV_Remove, 0 );
		}
	}
}

void idActor::DropBeltAttachments()
{
	int i;
	idEntity *ent;

	// remove any attached entities
	for (i = 0; i < attachmentsToDrop.Num(); i++)
	{
		if (!attachmentsToDrop[i].ent.IsValid())
			continue;

		ent = attachmentsToDrop[i].ent.GetEntity();
		if (ent)
		{
			ent->Unbind(); //Detach item from actor.

			//Throw it a little.
			ent->GetPhysics()->SetLinearVelocity(idVec3(gameLocal.random.RandomInt(-4, 4), gameLocal.random.RandomInt(-4, 4), gameLocal.random.RandomInt(12, 16)));
			ent->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.RandomInt(-4, 4), gameLocal.random.RandomInt(-4, 4), 0));

			// SW: don't override takedamage on items with it explicitly specified in spawnargs
			if (ent->spawnArgs.GetBool("takedamage", "1"))
			{
				ent->fl.takedamage = true;
			}
			

			if (ent->IsType(idItem::Type))
			{
				static_cast<idItem*>(ent)->SetDropInvincible();
			}
		}
	}

	attachmentsToDrop.Clear();
}

/*
================
idActor::Attach
================
*/
void idActor::Attach( idEntity *ent ) {
	idVec3			origin;
	idMat3			axis;
	jointHandle_t	joint;
	idStr			jointName;
	idAttachInfo	&attach = attachments.Alloc();
	idAngles		angleOffset;
	idVec3			originOffset;

	jointName = ent->spawnArgs.GetString( "joint" );
	joint = animator.GetJointHandle( jointName );
	if ( joint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for attaching '%s' on '%s'", jointName.c_str(), ent->GetClassname(), name.c_str() );
	}

	angleOffset = ent->spawnArgs.GetAngles( "angles" );
	originOffset = ent->spawnArgs.GetVector( "origin" );

	attach.channel = animator.GetChannelForJoint( joint );
	GetJointWorldTransform( joint, gameLocal.time, origin, axis );
	attach.ent = ent;

	ent->SetOrigin( origin + originOffset * renderEntity.axis );
	idMat3 rotate = angleOffset.ToMat3();
	idMat3 newAxis = rotate * axis;
	ent->SetAxis( newAxis );
	ent->BindToJoint( this, joint, ent->spawnArgs.GetBool("orientated", "true") );
	ent->cinematic = cinematic;
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

	if (!gameLocal.InPlayerConnectedArea(this))
	{
		//if I am no longer in PVS, then hide my healthbar.
		healthbarDisplaytime = 0;
	}

	spawnArgs.SetVector("origin", origin);

	if (this != gameLocal.GetLocalPlayer()) //Don't set player gravity to zero, cuz it messes up the player movement in outer space (player physics gravityNormal gets all messed up).
	{
		UpdateGravity();
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
idActor::OnLadder
================
*/
bool idActor::OnLadder( void ) const {
	return false;
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

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		//assert( 0 );
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
idActor::GetAnimName
=====================
// blendo eric
*/
const char* idActor::GetAnimName(int channel) {
	return animator.CurrentAnim(channel)->AnimFullName();
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

/*
=====================
idActor::SetWaitState
=====================
*/
void idActor::SetWaitState( const char *_waitstate ) {
	waitState = _waitstate;
}

/*
=====================
idActor::UpdateAnimState
=====================
*/
void idActor::UpdateAnimState(void) {

	if (ai_showPlayerState.GetBool() && this->IsType(idPlayer::Type)) { // blendo eric: collect start debug states
		for (int chidx = 0; chidx < DEBUG_Anim_State_Count; chidx++) {
			int channel = DEBUG_Anim_State_Channels[chidx];
			DEBUG_Anim_State_Changed[channel].Append(GetAnimState(channel));
		}
	}

	headAnim.UpdateState();

	DEBUG_Anim_Channel_Executing = ANIMCHANNEL_TORSO; // blendo eric: set executing anim channel, to check for channel mismatches
	torsoAnim.UpdateState();

	DEBUG_Anim_Channel_Executing = ANIMCHANNEL_LEGS;
	legsAnim.UpdateState();

	DEBUG_Anim_Channel_Executing = ANIMCHANNEL_ALL;

	if (ai_showPlayerState.GetBool() && this->IsType(idPlayer::Type)) {
		if (ai_showPlayerState.GetInteger() == 1) {
			for (int chidx = 0; chidx < DEBUG_Anim_State_Count; chidx++) {  // blendo eric: print changes to states
				int channel = DEBUG_Anim_State_Channels[chidx];
				if (DEBUG_Anim_State_Changed[channel].Find(">") >= 0) {
					DEBUG_Anim_State_Changed[channel].Replace(">", "^7>^6");
					common->Printf("%05d New state ^4%s ^6%s ^2[%s]\n", gameLocal.time, ANIMCHANNEL_Names[channel], DEBUG_Anim_State_Changed[channel].c_str(), GetAnimName(channel));
				}
				DEBUG_Anim_State_Changed[channel].Clear();
			}
		}


		idAngles dir;
		idVec3 drawpos, forward;

		dir.yaw = viewAxis.ToAngles().yaw;
		dir.pitch = 0;
		dir.roll = 0;
		dir.ToVectors(&forward, NULL, NULL);

		drawpos = GetEyePosition() + (forward * 64);


		if (ai_showPlayerState.GetInteger() == 1) {
			gameRenderWorld->DrawText(torsoAnim.state,
				drawpos,
				0.2f, colorWhite, viewAxis, 100);

			gameRenderWorld->DrawText(legsAnim.state,
				drawpos + idVec3(0, 0, -8),
				0.2f, colorWhite, viewAxis, 100);
		} else {
			gameRenderWorld->DrawText( idStr(torsoAnim.state) + " [" + idStr(GetAnimName(ANIMCHANNEL_TORSO)) + "]",
				drawpos,
				0.2f, colorWhite, viewAxis, 100);

			gameRenderWorld->DrawText( idStr(legsAnim.state) + " [" + idStr(GetAnimName(ANIMCHANNEL_LEGS)) + "]",
				drawpos + idVec3(0, 0, -8),
				0.2f, colorWhite, viewAxis, 100);
		}
	}
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

	if ( animPrefix.Length() ) {
		temp = va( "%s_%s", animPrefix.c_str(), animname );
		anim = animatorPtr->GetAnim( temp );
		if ( anim ) {
			return anim;
		}
	}

	anim = animatorPtr->GetAnim( animname );

	return anim;
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
	// no gibbing in multiplayer - by self damage or by moving objects
	if ( gameLocal.isMultiplayer ) {
		return;
	}
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

inflictor, attacker, dir, and point can be NULL for environmental effects

Bleeding wounds and surface overlays are applied in the collision code that
calls Damage()
============
*/
void idActor::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	int pointdefenseIndex;

	if (!fl.takedamage) {
		return;
	}

	if (!inflictor) {
		inflictor = gameLocal.world;
	}
	if (!attacker) {
		attacker = gameLocal.world;
	}


	ShowHealthbar();



#ifdef _D3XP
	SetTimeState ts(timeGroup);

	// Helltime boss is immune to all projectiles except the helltime killer
	//if ( finalBoss && idStr::Icmp(inflictor->GetEntityDefName(), "projectile_helltime_killer") ) {
	//	return;
	//}
	//
	//// Maledict is immume to the falling asteroids
	//if ( !idStr::Icmp( GetEntityDefName(), "monster_boss_d3xp_maledict" ) &&
	//	(!idStr::Icmp( damageDefName, "damage_maledict_asteroid" ) || !idStr::Icmp( damageDefName, "damage_maledict_asteroid_splash" ) ) ) {
	//	return;
	//}
#else
	//if ( finalBoss && !inflictor->IsType( idSoulCubeMissile::Type ) ) {
	//	return;
	//}
#endif




	const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefName);
	if (!damageDef) {
		gameLocal.Error("Unknown damageDef '%s'", damageDefName);
	}


	//If Bullet damage and has point defense, then ignore.
	pointdefenseIndex = GetPointdefenseStatus();
	if (pointdefenseIndex >= 0 && (damageDef->GetBool("isbullet", "0") || damageDef->GetBool("blocked_by_shield", "0")) && pointdefenseAmount > 0  /*&& inflictor->IsType(idProjectile::Type)*/)
	{
		//Point defense robot has intercepted the bullet.

		idVec3 robotpos;
		idMat3 robotaxis;
		jointHandle_t robotJoint = attachments[pointdefenseIndex].ent.GetEntity()->GetAnimator()->GetJointHandle("robot");

		attachments[pointdefenseIndex].ent.GetEntity()->GetAnimator()->GetJointTransform(robotJoint, gameLocal.time, robotpos, robotaxis);
		robotpos = attachments[pointdefenseIndex].ent.GetEntity()->GetPhysics()->GetOrigin() + robotpos;
		//gameRenderWorld->DebugLine(colorYellow, robotpos, inflictor->GetPhysics()->GetOrigin(), 100, true);

		if (inflictor->IsType(idProjectile::Type))
			((idProjectile*)inflictor)->neutralized = true;

		//gameRenderWorld->DebugArrow(colorRed, inflictor->GetPhysics()->GetOrigin(), inflictor->GetPhysics()->GetOrigin() + idVec3(0, 0, 64), 8, 10000);
		//idEntityFx::StartFx("fx/pointdefense_activate", &robotpos, &robotaxis, attachments[pointdefenseIndex].ent.GetEntity(), true);

		//deplete the point defense charge.
		pointdefenseAmount--;

		if (pointdefenseAmount <= 0)
		{
			//no more point defense charges. make the point defense robot disappear.
			StartSound("snd_energyshield_break", SND_CHANNEL_ANY);
			DetachPointdefense();

			idEntityFx::StartFx(spawnArgs.GetString("fx_pointdefense_break"), &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);
		}
		else
		{
			//still have more charges left.
			StartSound("snd_energyshield_repel", SND_CHANNEL_ANY);

			//make the repel effect appear XX units away from body.
			//TODO: make this bind correctly. the ai is able to kinda run right through the fx right now.
			idVec3 inflictPoint = inflictor->GetPhysics()->GetOrigin() - idVec3(GetPhysics()->GetOrigin().x, GetPhysics()->GetOrigin().y, inflictor->GetPhysics()->GetOrigin().z);
			inflictPoint.Normalize();
			inflictPoint = idVec3(GetPhysics()->GetOrigin().x, GetPhysics()->GetOrigin().y, inflictor->GetPhysics()->GetOrigin().z) + (inflictPoint * 24);
			idEntityFx *repelFX = idEntityFx::StartFx(spawnArgs.GetString("fx_pointdefense_block"), &inflictPoint, &robotaxis, NULL, false);
			if (repelFX)
			{
				repelFX->Bind(this, false);
			}

			//when damage is repelled by shield, we still mark it as an attack so the monster knows who to attack.
			lastDamagedTime = gameLocal.time;
			if (attacker != nullptr)
			{
				lastAttacker = attacker;
			}

			OnShieldHit();
		}

		//Do no damage.
		return;
	}


	if (damageDef->GetBool("isbullet", "0") && inflictor->IsType(idProjectile::Type))
	{
		//Check if me, the monster, can be damaged by bullets.
		if (spawnArgs.GetBool("damaged_by_bullets", "1"))
		{
			//Can be damaged by bullets........ do nothing here
		}
		else
		{
			//I am NOT damaged by bullets. exit here.
			return;
		}
	}


	//BC we want to ignore throwable damage that comes from the front.
	if (1)
	{
		idStr damageGroup = GetDamageGroup(location);
		const char *forceLocation = damageDef->GetString("forcelocation"); //damage type that forces itself to be a specific damage group. This is used for the death from above jump.
		if (forceLocation[0] != '\0')
		{
			damageGroup = forceLocation;
		}

		if (damageDef->GetBool("stundamage"))
		{
			//Check if this damage def only applies to a specific zone.
			const char *stun_zone = damageDef->GetString("stun_zone");
			if (stun_zone[0] == '\0')
			{
				//No specific zone.
				//do the special Stun Damage.
				StartStunState(damageDefName);
			}
			else
			{
				if (idStr::Icmp(damageGroup, stun_zone) >= 0)
				{
					//damagedef stun_zone matches the hit position on me. Helmet pop off absorbs the damage. Only do stun if helmet has been popped off.
					if (idStr::Icmp(damageGroup, "head") >= 0 && hasHelmet)
					{
						//if I have a helmet and it hit me on head, then ignore....
					}
					else
					{
						//AI has had an item clonk them on the head.

						//We want the AI to take stun damage ONLY IF: the ai is not in combat/search state, or if thrown object is behind/sideways to AI.
						if (IsStundamageValid(dir))
						{
							//do the special Stun Damage.
							StartStunState(damageDefName);
						}
						else
						{
							return; //do NOT take damage.
						}
					}


					//BC changed how this works; previously, the stun was ignored if actor is wearing a helmet. Now: the stun happens regardless if a helmet is attached or not.
					//StartStunState(damageDef);
				}
			}
		}
	}



	int	damage = damageDef->GetInt("damage") * damageScale;
	int originalDamageAmount = damage;
	damage = max(damage, GetDamageForLocation(damage, location)); //BC this was meant to handle items that bop the head to not inflict damage; but this isn't working and needs to be fixed

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );

	if ( damage > 0 || originalDamageAmount > 0)
	{
		if (g_onehitkill.GetBool() && team == TEAM_ENEMY)
		{
			//accessibility: apply enough damage to kill the entity.
			damage = originalDamageAmount = health;
		}


		bool diedOnThisFrame = false;
		
		lastAttacker = attacker;
		lastDamagedTime = gameLocal.time;

		//Note: electrical damage circumvents the energyshield.
		if (energyShieldMax > 0 && energyShieldCurrent > 0 && health > 0 && !damageDef->GetBool("iselectrical"))
		{
			if (energyShieldCurrent - damage < 0)
			{
				//Player has defeated shield AND some health. Shield has been fully depleted on this frame.
				damage = damage - energyShieldCurrent; //Damage exceeds remaining shield; determine much damage to inflict on my HEALTH.
				energyShieldCurrent = 0;
				energyShieldState = ENERGYSHIELDSTATE_DESTROYED;
				energyShieldTimer = gameLocal.time + ENERGYSHIELD_RECHARGEDELAY;
				StartSound("snd_energyshield_break", SND_CHANNEL_BODY3, 0, false, NULL);

				OnShieldDestroyed();

				if (damage >= maxHealth && spawnArgs.GetBool("finalboss"))
				{
					//special code for final boss. TODO: make this more generic.
					damage = 1; //Don't kill boss with one hit. Make shield absorb all the damage.
				}
			}
			else
			{
				//Shield has absorbed 100% of the damage and still has some remaining shield.
				energyShieldCurrent -= damage;
				energyShieldState = ENERGYSHIELDSTATE_REGENDELAY;
				energyShieldTimer = gameLocal.time + ENERGYSHIELD_RECHARGEDELAY;				
				idEntityFx::StartFx("fx/pointdefense_bullet", inflictor->GetPhysics()->GetAbsBounds().GetCenter() + dir * -32, mat3_identity);				

				if (energyShieldCurrent <= 0)
				{
					energyShieldState = ENERGYSHIELDSTATE_DESTROYED;
					StartSound("snd_energyshield_break", SND_CHANNEL_BODY3, 0, false, NULL);
					OnShieldDestroyed();
				}
				else
				{
					StartSound("snd_energyshield_repel", SND_CHANNEL_BODY3, 0, false, NULL);
					OnShieldHit();
				}

				return;
			}
		}



		if (health > 0 && health - damage <= 0)
			diedOnThisFrame = true;

		if (gameLocal.time > lastHealthTimer) //if take multiple damage quickly, batch them all up together here to make it easier to read for the player.
			lastHealth = health;

		lastHealthTimer = gameLocal.time + LASTHEALTH_MAXTIME;

		health -= damage;

		gameLocal.AddEventlogDamage(this, damage, inflictor, attacker, damageDefName);

		if (damage > originalDamageAmount)
		{
			//check if headshot.
			idVec3 hitOrigin;
			if (idStr::Icmp(GetDamageGroup(location), "head") >= 0)
			{
				//snap it to the head bone. TODO: we can make this more data driven if we need to... make keyvalue damage_zone_wordjoint or something
				idMat3 hitAxis;
				jointHandle_t hitJoint = animator.GetJointHandle("head");
				animator.GetJointTransform(hitJoint, gameLocal.time, hitOrigin, hitAxis);
				hitOrigin = renderEntity.origin + (hitOrigin + modelOffset) * renderEntity.axis;
				hitOrigin.z += 12;
			}
			else
			{
				jointHandle_t hitJoint = (jointHandle_t)location;
				idMat3 hitAxis;
				animator.GetJointTransform(hitJoint, gameLocal.time, hitOrigin, hitAxis);
				hitOrigin = renderEntity.origin + (hitOrigin + modelOffset) * renderEntity.axis;
			}
			
			if (!gameLocal.GetLocalPlayer()->isInVignette)
			{
				gameLocal.GetLocalPlayer()->SetFlytextEvent(hitOrigin, "CRIT!"); //headshots, etc.
			}
			
		}



		//Check the health against any damage cap that is currently set
		if(damageCap >= 0 && health < damageCap) {
			health = damageCap;
		}


		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}

			if ((health < GIB_THRESHOLD) && spawnArgs.GetBool("gib") && damageDef->GetBool("gib")) {
				Gib(dir, damageDefName);
			}

			Killed( inflictor, attacker, damage, dir, location );
			

			//bc death UI feedback.
			if (attacker->IsType(idPlayer::Type) && diedOnThisFrame)
			{
				static_cast<idPlayer *>(attacker)->KillFeedback();
			}
		}
		else
		{
			Pain( inflictor, attacker, damage, dir, location, true, damageDefName);
		}

		if (damageDef->GetBool("isfire") && health > 0)
		{
			//receive fire damage. set it on fire.

			idEntity *damageAttachment;
			idDict args;
			args.Set("classname", "func_fireattachment");
			gameLocal.SpawnEntityDef(args, &damageAttachment);

			if (damageAttachment->IsType(idFireAttachment::Type))
			{
				static_cast<idFireAttachment *>(damageAttachment)->AttachTo(this);
			}
		}

	}

	// SW 24th Feb 2025:
	// Removing this so that zero-damage stimuli (e.g. gas jets) don't cause ragdolls to freeze in mid-air
	// I don't know exactly why Doom 3 needed this, so let's hope we don't miss it!
	//else
	//{
	//	// don't accumulate knockback
	//	if ( af.IsLoaded() ) {
	//		// clear impacts
	//		af.Rest();

	//		// physics is turned off by calling af.Rest()
	//		BecomeActive( TH_PHYSICS );
	//	}
	//}


	if (health > 0 && team == TEAM_ENEMY && attacker != NULL)
	{
		//BC 2-21-2025: do not do the player inflict damage vo during jockey
		//BC 3-19-2025: also don't do this if player is currently in defib state.
		if (attacker == gameLocal.GetLocalPlayer() && !gameLocal.GetLocalPlayer()->IsJockeying() && !gameLocal.GetLocalPlayer()->GetDefibState())
		{
			//Play player VO: "I damaged an enemy"
			bool isWorldCombatstate = (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetCombatState() == COMBATSTATE_IDLE);
			gameLocal.GetLocalPlayer()->SayVO_WithIntervalDelay_msDelayed(isWorldCombatstate ? "snd_vo_inflictdamage_unseen" : "snd_vo_inflictdamage_loud", 300);			
		}
	}
}

bool idActor::IsStundamageValid(idVec3 _damageDir)
{
	//idVec3 facingDirection = viewAxis[0];
	//
	//float facingResult = DotProduct(_damageDir, facingDirection);
	//
	//if (facingResult > THROWABLE_DOT_THRESHOLD)
	//{
	//	return true; //I'm being attacked from either my SIDE or BEHIND me. So, allow this throwable attack.
	//}
	//
	//return false; //Attack is coming from in front of me. Attack is no good, I can't be attacked from the front.

	return true;
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
bool idActor::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, bool playPainSound, const char *damageDef) {
	if ( af.IsLoaded() ) {
		// clear impacts
		af.Rest();

		// physics is turned off by calling af.Rest()
		BecomeActive( TH_PHYSICS );
	}

	if ( gameLocal.time < pain_debounce_time ) {
		return false;
	}

	// don't play pain sounds more than necessary
	pain_debounce_time = gameLocal.time + pain_delay;

	//BC allow the damage definition to suppress pain sounds.
	const idDict *damageDefinition = gameLocal.FindEntityDefDict(damageDef);
	if (damageDefinition)
	{
		if (!damageDefinition->GetBool("playpainsound", "1"))
		{
			playPainSound = false;
		}
	}


	if (playPainSound)
	{
		if (health > 75) {
			StartSound("snd_pain_small", SND_CHANNEL_VOICE, 0, false, NULL);
		}
		else if (health > 50) {
			StartSound("snd_pain_medium", SND_CHANNEL_VOICE, 0, false, NULL);
		}
		else if (health > 25) {
			StartSound("snd_pain_large", SND_CHANNEL_VOICE, 0, false, NULL);
		}
		else {
			StartSound("snd_pain_huge", SND_CHANNEL_VOICE, 0, false, NULL);
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
	
	//const idDict *damageDefinition = gameLocal.FindEntityDefDict(damageDef);
	//if (damageDefinition)
	//{
	//	if (damageDefinition->GetBool("stundamage"))
	//	{
	//		//Check if this damage def only applies to a specific zone.
	//		const char *stun_zone = damageDefinition->GetString("stun_zone");
	//		if (stun_zone[0] == '\0')
	//		{
	//			//No specific zone.
	//			//do the special Stun Damage.
	//			sprintf(painAnim, "pain_stun");
	//		}
	//		else
	//		{
	//			//ok , this damagedef specifies a specific zone.
	//			if (idStr::Icmp(damageGroup, stun_zone) >= 0)
	//			{
	//				//it's a match.
	//				if (idStr::Icmp(damageGroup, "head") >= 0 && hasHelmet)
	//				{
	//					//if stun zone is head and I have helmet, then ignore.
	//				}
	//				else
	//				{
	//
	//				}
	//			}
	//		}			
	//	}
	//}




	
	if ( animPrefix.Length() )
	{
		if ( damageGroup.Length() && ( damageGroup != "legs" ) )
		{
			sprintf( painAnim, "%s_pain_%s", animPrefix.c_str(), damageGroup.c_str() );
			if ( !animator.HasAnim( painAnim ) )
			{
				sprintf( painAnim, "pain_%s", damageGroup.c_str() );
				if ( !animator.HasAnim( painAnim ) )
				{
					painAnim = "";
				}
			}
		}

		if ( !painAnim.Length() )
		{
			sprintf( painAnim, "%s_pain", animPrefix.c_str() );
			if ( !animator.HasAnim( painAnim ) )
			{
				painAnim = "";
			}
		}
	}
	else if ( damageGroup.Length() && ( damageGroup != "legs" ) && !painAnim.Length())
	{
		sprintf( painAnim, "pain_%s", damageGroup.c_str() );
		if ( !animator.HasAnim( painAnim ) ) {
			sprintf( painAnim, "pain_%s", damageGroup.c_str() );
			if ( !animator.HasAnim( painAnim ) ) {
				painAnim = "";
			}
		}
	}
	else if (!painAnim.Length())
	{
		//bc directional pain anims.
		float painDot;

		painDot = DotProduct(viewAxis[0], dir);

		if (painDot > .5f)
		{
			//From back.
			sprintf(painAnim, "pain_fromback");
		}
		else if (painDot < -.5f)
		{
			//From front.
			sprintf(painAnim, "pain_fromfront");
		}
		else
		{
			painDot = DotProduct(viewAxis[1], dir);

			if (painDot > 0.5)
			{
				//From right.
				sprintf(painAnim, "pain_fromright");
			}
			else
			{
				//From left.
				sprintf(painAnim, "pain_fromleft");
			}
		}

		if (!animator.HasAnim(painAnim))
		{
			painAnim = "";
		}
	}



	if ( !painAnim.Length() ) {
		painAnim = "pain";
	}

	if ( g_debugDamage.GetBool() ) {
		gameLocal.Printf( "Damage: joint: '%s', zone '%s', anim '%s'\n", animator.GetJointName( ( jointHandle_t )location ), damageGroup.c_str(), painAnim.c_str() );
	}

	return true;
}

/*
=====================
idActor::SpawnGibs
=====================
*/
void idActor::SpawnGibs( const idVec3 &dir, const char *damageDefName ) {
	idAFEntity_Gibbable::SpawnGibs( dir, damageDefName );
	RemoveAttachments();
	DropBeltAttachments(); //BC
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


	//BC bonus damage groups.
	damageBonus.SetNum(animator.NumJoints());
	for (i = 0; i < damageBonus.Num(); i++) {
		damageBonus[i] = 0;
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


	//BC bonus damage. This is similar to damage_scale but is not a multiplier, this is an absolute value.
	arg = spawnArgs.MatchPrefix("damage_bonus", NULL);
	while (arg)
	{
		int bonusDmg = atof(arg->GetValue());
		groupname = arg->GetKey();
		groupname.Strip("damage_bonus ");
		for (i = 0; i < damageBonus.Num(); i++)
		{
			if (damageGroups[i] == groupname)
			{
				damageBonus[i] = bonusDmg;
			}
		}
		arg = spawnArgs.MatchPrefix("damage_bonus", arg);
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

	//BC special check if actor is wearing a helmet.
	if (hasHelmet)
	{
		if (idStr::Icmp(GetDamageGroup(location), "head") >= 0)
		{
			//hit on head AND is wearing helmet. Don't scale the damage, as the helmet 'absorbs' the damage.
			return 0; //Helmet takes all the damage.
			//return damage;
		}
	}

	return (int)ceil( (damage * damageScale[ location ]) + damageBonus[location]);
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


/***********************************************************************

	Events

***********************************************************************/

/*
=====================
idActor::Event_EnableEyeFocus
=====================
*/
void idActor::PlayFootStepSound( bool isRunning )
{
	const char *sound = NULL;
	const idMaterial *material;

	if ( !GetPhysics()->HasGroundContacts() )
	{
		return;
	}

	// start footstep sound based on material type
	material = GetPhysics()->GetContact( 0 ).material;
	if ( material != NULL )
	{
		//const char *cc = gameLocal.sufaceTypeNames[material->GetSurfaceType()];
		sound = spawnArgs.GetString( va( isRunning ? "snd_footstep_%s_run" : "snd_footstep_%s", gameLocal.sufaceTypeNames[ material->GetSurfaceType() ] ) );
	}

	if (*sound == '\0' && isRunning)
	{
		sound = spawnArgs.GetString("snd_footstep_run");
	}
	
	//If that fails then play a generic footstep.
	if ( *sound == '\0' )
	{
		sound = spawnArgs.GetString( "snd_footstep" );
	}

	if (*sound != '\0')
	{
		StartSoundShader(declManager->FindSound(sound), SND_CHANNEL_BODY, 0, false, NULL);
	}
}

/*
=====================
idActor::Event_EnableEyeFocus
=====================
*/
void idActor::Event_EnableEyeFocus( void ) {
	allowEyeFocus = true;
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomFloat() * ( blink_max - blink_min );
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
void idActor::Event_Footstep( void )
{
	PlayFootStepSound(false);
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

void idActor::Event_GetCustomIdleAnim()
{
	if (customIdleAnim.Length() <= 0)
	{
		
#if _DEBUG
		assert( 0 ); //This shouldn't happen. Try to track down what is causing this to happen.
#endif

		idThread::ReturnString("");
	}
	else
	{
		idThread::ReturnString(customIdleAnim);
	}
}

void idActor::Event_GetStunAnim()
{
	if (!stunAnimationName.Length())
	{
		//default.
		idThread::ReturnString("pain_stun");
	}
	else
	{
		idThread::ReturnString(stunAnimationName);
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
idActor::Event_PlayAnim
===============
*/
void idActor::Event_PlayAnim( int channel, const char *animname ) {

	if (ai_showPlayerState.GetInteger() >= 2 && this->IsType(idPlayer::Type)) { // blendo eric: print anim change with their "respective" state change
		common->Printf("%05d ^2Play anim ^4%s ^6%s^7>^6%s ^2[%s]\n", gameLocal.time, ANIMCHANNEL_Names[channel], DEBUG_Anim_State_Prev[channel].c_str(), GetAnimState(channel), animname);
	}

#if defined(_DEBUG)
	if ( channel != 0 && DEBUG_Anim_Channel_Executing != 0 && channel != DEBUG_Anim_Channel_Executing ) { // blendo eric: check for channel mismatches
		gameLocal.Warning("Event call playAnim( ANIMCHANNEL_%s, '%s') in %s is being called from the %s animState, kinda weird, no? \n",
			ANIMCHANNEL_Names[channel], animname, GetAnimState(DEBUG_Anim_Channel_Executing), ANIMCHANNEL_Names[DEBUG_Anim_Channel_Executing]);
	}
#endif

	animFlags_t	flags;
	idEntity *headEnt;
	int	anim;

	anim = GetAnim( channel, animname );

	if ( !anim )
	{
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() )
		{
			gameLocal.Error( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		}
		else
		{
			gameLocal.Error( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}

		idThread::ReturnInt( 0 );
		return;
	}

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headEnt = head.GetEntity();
		if ( headEnt )
		{
			headAnim.idleAnim = false;
			headAnim.PlayAnim( anim );
			flags = headAnim.GetAnimFlags();
			if ( !flags.prevent_idle_override )
			{
				if ( torsoAnim.IsIdle() )
				{
					torsoAnim.animBlendFrames = headAnim.lastAnimBlendFrames;
					SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_HEAD, headAnim.lastAnimBlendFrames );
					if ( legsAnim.IsIdle() )
					{
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
		if ( !flags.prevent_idle_override )
		{
			if ( headAnim.IsIdle() )
			{
				headAnim.animBlendFrames = torsoAnim.lastAnimBlendFrames;
				SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
			}
			if ( legsAnim.IsIdle() )
			{
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

	if (ai_showPlayerState.GetInteger() >= 2 && this->IsType(idPlayer::Type)) { // blendo eric: print anim change with their "respective" state change
		common->Printf("%05d ^2Cycle anim ^4%s ^6%s^7>^6%s ^2[%s]\n", gameLocal.time, ANIMCHANNEL_Names[channel], DEBUG_Anim_State_Prev[channel].c_str(), GetAnimState(channel), animname);
	}

#if defined(_DEBUG)
	if (channel != 0 && DEBUG_Anim_Channel_Executing != 0 && channel != DEBUG_Anim_Channel_Executing) { // blendo eric: check for channel mismatches
		gameLocal.Warning("Event call playCycle( ANIMCHANNEL_%s, '%s') in %s is being called from the %s animState, kinda weird, no? \n",
			ANIMCHANNEL_Names[channel], animname, GetAnimState(DEBUG_Anim_Channel_Executing), ANIMCHANNEL_Names[DEBUG_Anim_Channel_Executing]);
	}
#endif

	animFlags_t	flags;
	int			anim;

	anim = GetAnim( channel, animname );
	if ( !anim ) {
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}
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
idActor::Event_IdleAnim
===============
*/
void idActor::Event_IdleAnim( int channel, const char *animname ) {

	if (ai_showPlayerState.GetInteger() >= 2 && this->IsType(idPlayer::Type)) { // blendo eric: print anim change with their "respective" state change
		common->Printf("%05d ^2Idle anim ^4%s ^6%s^7>^6%s ^2[%s]\n", gameLocal.time, ANIMCHANNEL_Names[channel], DEBUG_Anim_State_Prev[channel].c_str(), GetAnimState(channel), animname);
	}

#if defined(_DEBUG)
	if (channel != 0 && DEBUG_Anim_Channel_Executing != 0 && channel != DEBUG_Anim_Channel_Executing) { // blendo eric: check for channel mismatches
		gameLocal.Warning("Event call idleAnim( ANIMCHANNEL_%s, '%s') in %s is being called from the %s animState, kinda weird, no? \n",
			ANIMCHANNEL_Names[channel], animname, GetAnimState(DEBUG_Anim_Channel_Executing), ANIMCHANNEL_Names[DEBUG_Anim_Channel_Executing]);
	}
#endif

	int anim;

	anim = GetAnim( channel, animname );
	if ( !anim ) {
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.DPrintf( "missing '%s' animation on '%s' (%s)\n", animname, name.c_str(), GetEntityDefName() );
		}

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

/*
===============
idActor::Event_OverrideAnim
===============
*/
void idActor::Event_OverrideAnim( int channel ) {
	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		headAnim.Disable();
		if ( !torsoAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		} else {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		}
		break;

	case ANIMCHANNEL_TORSO :
		torsoAnim.Disable();
		SyncAnimChannels( ANIMCHANNEL_TORSO, ANIMCHANNEL_LEGS, legsAnim.lastAnimBlendFrames );
		if ( headAnim.IsIdle() ) {
			SyncAnimChannels( ANIMCHANNEL_HEAD, ANIMCHANNEL_TORSO, torsoAnim.lastAnimBlendFrames );
		}
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
#if _DEBUG
	if (ai_showPlayerState.GetBool() && this->IsType(idPlayer::Type)) {
		const char* curState = GetAnimState(channel);
		if ( idStr::Cmp( statename, curState) != 0 ) {
			DEBUG_Anim_State_Changed[channel].Append(">");
			DEBUG_Anim_State_Changed[channel].Append(statename);
			if ( DEBUG_Anim_State_Prev[channel] != curState ) {
				DEBUG_Anim_State_Prev[channel]  = curState;
			}
		}
	}

	if (DEBUG_ANIM_State_Channel_Mismatch(statename, channel)) {
		gameLocal.Warning("Event call animState( ANIMCHANNEL_%s, '%s') might be have mismatched channels?\n", ANIMCHANNEL_Names[channel], statename);
    }
#endif

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
		SetWaitState( "" );
	}
}

/*
===============
idActor::Event_AnimDone
===============
*/
void idActor::Event_AnimDone( int channel, int blendFrames ) {

#if defined(_DEBUG)
	if (channel != 0 && DEBUG_Anim_Channel_Executing != 0 && channel != DEBUG_Anim_Channel_Executing) { // blendo eric: check for channel mismatches
		gameLocal.Warning("Event call animDone( ANIMCHANNEL_%s ) is being called from animstate %s thread, kinda weird, no? \n", ANIMCHANNEL_Names[channel], ANIMCHANNEL_Names[DEBUG_Anim_Channel_Executing]);
	}
#endif

	bool result;

	switch( channel ) {
	case ANIMCHANNEL_HEAD :
		result = headAnim.AnimDone( blendFrames );
		idThread::ReturnInt( result );
		break;

	case ANIMCHANNEL_TORSO :
		result = torsoAnim.AnimDone( blendFrames );
		idThread::ReturnInt( result );
		break;

	case ANIMCHANNEL_LEGS :
		result = legsAnim.AnimDone( blendFrames );
		idThread::ReturnInt( result );
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

#ifdef _D3XP
/*
================
idActor::Event_SetDamageGroupScale
================
*/
void idActor::Event_SetDamageGroupScale( const char* groupName, float scale) {

	for( int i = 0; i < damageScale.Num(); i++ ) {
		if ( damageGroups[ i ] == groupName ) {
			damageScale[ i ] = scale;
		}
	}
}

/*
================
idActor::Event_SetDamageGroupScaleAll
================
*/
void idActor::Event_SetDamageGroupScaleAll( float scale ) {

	for( int i = 0; i < damageScale.Num(); i++ ) {
		damageScale[ i ] = scale;
	}
}

void idActor::Event_GetDamageGroupScale( const char* groupName ) {

	for( int i = 0; i < damageScale.Num(); i++ ) {
		if ( damageGroups[ i ] == groupName ) {
			idThread::ReturnFloat(damageScale[i]);
			return;
		}
	}

	idThread::ReturnFloat(0);
}

void idActor::Event_SetDamageCap( float _damageCap ) {
	damageCap = _damageCap;
}

void idActor::Event_SetWaitState( const char* waitState) {
	SetWaitState(waitState);
}

void idActor::Event_GetWaitState() {
	if(WaitState()) {
		idThread::ReturnString(WaitState());
	} else {
		idThread::ReturnString("");
	}
}



void idActor::Event_FootstepLeft(void)
{
	Event_FootstepGeneric(0, false);
}

void idActor::Event_FootstepRight(void)
{
	Event_FootstepGeneric(1, false);
}

void idActor::Event_FootstepLeftRun(void)
{
	Event_FootstepGeneric(0, true);
}

void idActor::Event_FootstepRightRun(void)
{
	Event_FootstepGeneric(1, true);
}

void idActor::Event_FootstepGeneric(int side, bool isRunning)
{
	if (gameLocal.GetLocalPlayer()->listenmodeVisualizerTime > gameLocal.time)
	{
		idVec3 jointpos;
		idMat3 axis;
		animator.GetJointTransform(side == 0 ? leftFootJoint : rightFootJoint, gameLocal.time, jointpos, axis);
		gameLocal.GetLocalPlayer()->SetNoiseEvent(this->GetPhysics()->GetOrigin() + jointpos, NOISETYPE_FOOTSTEP);
	}

	PlayFootStepSound(isRunning);
}

void idActor::Event_getEyePos(void)
{
	idThread::ReturnVector(GetEyePosition());
}

int idActor::GetPointdefenseStatus(void)
{
	int i;
	idEntity *ent;

	if (attachments.Num() <= 0)
		return -1;

	for (i = 0; i < attachments.Num(); i++)
	{
		ent = attachments[i].ent.GetEntity();
		if (ent)
		{
			if (ent->spawnArgs.GetBool("pointdefense", "false"))
			{
				return i;
			}
		}
	}

	return -1;
}

// unbind, unset owner, and remove attachments
void idActor::PlayerTookCarryable( idEntity* item )
{
	if (!item) return;

	if (item->GetBindMaster() == this)
	{
		item->Unbind();
	}

	if (item->GetPhysics()->GetClipModel()
		&& item->GetPhysics()->GetClipModel()->GetOwner() == this)
	{
		item->GetPhysics()->GetClipModel()->SetOwner(NULL);
	}

	for ( int i = 0; i < attachmentsToDrop.Num(); i++ )
	{
		if ( attachmentsToDrop[i].ent.GetEntity() == item )
		{
			attachmentsToDrop.RemoveIndex(i);
			break;
		}
	}

	for ( int i = 0; i < attachments.Num(); i++ )
	{
		if ( attachments[i].ent.GetEntity() == item )
		{
			attachments.RemoveIndex(i);
			break;
		}
	}
}

void idActor::Event_GetActorCenter()
{
	idThread::ReturnVector(GetPhysics()->GetAbsBounds().GetCenter());
}




idVec3 idActor::GetEyeBonePosition(void)
{
	idVec3		_eyepos;
	idMat3		_axis;

	if (eyeJoint != INVALID_JOINT)
	{
		if (GetJointWorldTransform(eyeJoint, gameLocal.time, _eyepos, _axis))
		{
			return _eyepos;
		}
	}

	return GetEyePosition();
}



void idActor::OnShieldHit()
{	
}

void idActor::OnShieldDestroyed()
{
}

void idActor::StartStunState(const char *damageDefName)
{
}

//Is the shield currently up and active
bool idActor::GetEnergyshieldActive()
{
	return (energyShieldState == ENERGYSHIELDSTATE_IDLE || energyShieldState == ENERGYSHIELDSTATE_REGENDELAY || energyShieldState == ENERGYSHIELDSTATE_REGENERATING);
}

//Attach helmet to head.
void idActor::SetHelmet(bool value)
{
	if (value)
	{
		//Add helmet.
		jointHandle_t headJoint;
		idVec3 jointPos;
		idMat3 jointAxis;

		headJoint = animator.GetJointHandle("head");
		this->GetJointWorldTransform(headJoint, gameLocal.time, jointPos, jointAxis);
		idVec3 helmetPos = jointPos;
		helmetPos.z += 2.5f;

		idDict args;
		args.Clear();
		args.SetVector("origin", helmetPos);
		args.SetMatrix("rotation", viewAxis);
		args.Set("model", "models/objects/helmet/helmet.ase");
		args.SetInt("solid", 0);
		args.SetBool("noclipmodel", true);
		helmetModel = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
		helmetModel->BindToJoint(this, headJoint, true);
	}
	else
	{
		//remove helmet.
		if (helmetModel != NULL)
		{
			helmetModel->PostEventMS(&EV_Remove, 0);
			helmetModel = nullptr;
		}
	}

	hasHelmet = value;
}

void idActor::SetMaxHealthDelta(int delta)
{
	if (health <= 0)
		return;

	maxHealth += delta;
	health += delta;

	if (health > maxHealth)
		health = maxHealth;	
}



//Remove the point defense robot.
void idActor::DetachPointdefense()
{
	if (attachments.Num() <= 0)
		return;

	int i;
	for (i = attachments.Num() - 1; i >= 0; i--)
	{
		idEntity *ent = attachments[i].ent.GetEntity();
		if (!ent)
			continue;
		
		if (ent->spawnArgs.GetBool("pointdefense", "false"))
		{
			ent->PostEventMS(&EV_Remove, 0);
		}		
	}
}

//Returns length of string.
void idActor::Event_ActorVO(const char *sndName, int voCategory)
{
	int duration = gameLocal.voManager.SayVO(this, sndName, voCategory);

	idThread::ReturnFloat((float)(duration) / 1000);
}

#endif
