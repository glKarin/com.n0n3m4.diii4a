// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


#if !defined(__GAME_PROJECTILE_H__)
	#include "Projectile.h"
#endif
#if !defined(__GAME_VEHICLE_H__)
	#include "Vehicle/Vehicle.h"
#endif

#include "ai/AI.h"
#include "ai/AI_Manager.h"

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
}

/*
=====================
idAnimState::Save
=====================
*/
void idAnimState::Save( idSaveGame *savefile ) const {

	savefile->WriteBool( idleAnim );
	savefile->WriteInt( animBlendFrames );
	savefile->WriteInt( lastAnimBlendFrames );

	savefile->WriteObject( self );

	// Save the entity owner of the animator
	savefile->WriteObject( animator->GetEntity() );

	savefile->WriteInt( channel );
	savefile->WriteBool( disabled );

// RAVEN BEGIN
// abahr:
	stateThread.Save( savefile );
// RAVEN END
}

/*
=====================
idAnimState::Restore
=====================
*/
void idAnimState::Restore( idRestoreGame *savefile ) {

	savefile->ReadBool( idleAnim );
	savefile->ReadInt( animBlendFrames );
	savefile->ReadInt( lastAnimBlendFrames );

	savefile->ReadObject( reinterpret_cast<idClass *&>( self ) );

	idEntity *animowner;
	savefile->ReadObject( reinterpret_cast<idClass *&>( animowner ) );
	if ( animowner ) {
		animator = animowner->GetAnimator();
	}

	savefile->ReadInt( channel );
	savefile->ReadBool( disabled );

// RAVEN BEGIN
// abahr:
	stateThread.Restore( savefile, self );
// RAVEN END
}

/*
=====================
idAnimState::Init
=====================
*/
// RAVEN BEGIN
// bdube: converted self to entity ptr so any entity can use it
void idAnimState::Init( idEntity *owner, idAnimator *_animator, int animchannel ) {
// RAVEN BEGIN
	assert( owner );
	assert( _animator );
	self = owner;
	animator = _animator;
	channel = animchannel;

	stateThread.SetName ( va("%s_anim_%d", owner->GetName(), animchannel ) );
	stateThread.SetOwner ( owner );	
}

/*
=====================
idAnimState::Shutdown
=====================
*/
void idAnimState::Shutdown( void ) {
	stateThread.Clear ( true );
}

/*
=====================
idAnimState::PostState
=====================
*/
void idAnimState::PostState ( const char* statename, int blendFrames, int delay, int flags ) {
	if ( SRESULT_OK != stateThread.PostState ( statename, blendFrames, delay, flags ) ) {
		gameLocal.Error ( "Could not find state function '%s' for entity '%s'", statename, self->GetName() );
	}
	disabled = false;
}

/*
=====================
idAnimState::SetState
=====================
*/
void idAnimState::SetState( const char *statename, int blendFrames, int flags ) {
	if ( SRESULT_OK != stateThread.SetState ( statename, blendFrames, 0, flags ) ) {
		gameLocal.Error ( "Could not find state function '%s' for entity '%s'", statename, self->GetName() );
	}

	animBlendFrames = blendFrames;
	lastAnimBlendFrames = blendFrames;
	disabled = false;
	idleAnim = false;
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

	stateThread.Execute ( );

	return true;
}

/***********************************************************************

	idActor

***********************************************************************/

const idEventDef AI_EnableEyeFocus( "enableEyeFocus" );
const idEventDef AI_DisableEyeFocus( "disableEyeFocus" );
const idEventDef AI_EnableBlink( "enableBlinking" );
const idEventDef AI_DisableBlink( "disableBlinking" );
const idEventDef EV_Footstep( "footstep" );
const idEventDef EV_FootstepLeft( "leftFoot" );
const idEventDef EV_FootstepRight( "rightFoot" );
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
const idEventDef AI_AnimDone( "animDone", "dd", 'd' );
const idEventDef AI_OverrideAnim( "overrideAnim", "d" );
const idEventDef AI_EnableAnim( "enableAnim", "dd" );
const idEventDef AI_PreventPain( "preventPain", "f" );
const idEventDef AI_DisablePain( "disablePain" );
const idEventDef AI_EnablePain( "enablePain" );
const idEventDef AI_SetAnimPrefix( "setAnimPrefix", "s" );
const idEventDef AI_HasEnemies( "hasEnemies", NULL, 'd' );
const idEventDef AI_NextEnemy( "nextEnemy", "E", 'e' );
const idEventDef AI_ClosestEnemyToPoint( "closestEnemyToPoint", "v", 'e' );
const idEventDef AI_GetHead( "getHead", NULL, 'e' );


// RAVEN BEGIN
// bdube: added
const idEventDef AI_Flashlight("flashlight", "d" );
const idEventDef AI_Teleport("teleport", "vv");
const idEventDef AI_EnterVehicle ( "enterVehicle", "e" );
const idEventDef AI_ExitVehicle ( "exitVehicle", "d" );
const idEventDef AI_PostExitVehicle ( "<exitVehicle>", "d" );

//jshepard: change animation rate
const idEventDef AI_SetAnimRate ( "setAnimRate","f");
//MCG: damage over time
const idEventDef EV_DamageOverTime ( "damageOverTime","ddEEvsfd" );
const idEventDef EV_DamageOverTimeEffect ( "damageOverTimeEffect","dds" );
// MCG: script-callable joint crawl effect
const idEventDef EV_JointCrawlEffect ( "jointCrawlEffect","sf" );

// RAVEN END

CLASS_DECLARATION( idAFEntity_Gibbable, idActor )
	EVENT( AI_EnableEyeFocus,			idActor::Event_EnableEyeFocus )
	EVENT( AI_DisableEyeFocus,			idActor::Event_DisableEyeFocus )
	EVENT( AI_EnableBlink,				idActor::Event_EnableBlink )
	EVENT( AI_DisableBlink,				idActor::Event_DisableBlink )
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
	EVENT( AI_SetAnimPrefix,			idActor::Event_SetAnimPrefix )
	EVENT( AI_SetSyncedAnimWeight,		idActor::Event_SetSyncedAnimWeight )
	EVENT( AI_SetBlendFrames,			idActor::Event_SetBlendFrames )
	EVENT( AI_GetBlendFrames,			idActor::Event_GetBlendFrames )
	EVENT( AI_OverrideAnim,				idActor::Event_OverrideAnim )
	EVENT( AI_EnableAnim,				idActor::Event_EnableAnim )
	EVENT( AI_HasEnemies,				idActor::Event_HasEnemies )
	EVENT( AI_NextEnemy,				idActor::Event_NextEnemy )
	EVENT( AI_ClosestEnemyToPoint,		idActor::Event_ClosestEnemyToPoint )
	EVENT( EV_StopSound,				idActor::Event_StopSound )
	EVENT( AI_GetHead,					idActor::Event_GetHead )

// RAVEN BEGIN
// bdube: added
	EVENT( AI_Flashlight,				idActor::Event_Flashlight )
	EVENT( AI_Teleport,					idActor::Event_Teleport )
	EVENT( AI_EnterVehicle,				idActor::Event_EnterVehicle )

	// twhitaker: Yeah... this just got confusing.
	// basically, I need a delay in between the time the space bar is hit and the time the person actually get's ejected.
	// this is mostly for things such as screen fades when exiting a vehicle.  This was the least obtrusive way I could think of.
	EVENT( AI_ExitVehicle,				idActor::Event_PreExitVehicle )
	EVENT( AI_PostExitVehicle,			idActor::Event_ExitVehicle )

// jshepard: added
	EVENT( AI_SetAnimRate,				idActor::Event_SetAnimRate )

// twhitaker: added animation support (mostly for vehicle purposes)
	EVENT( AI_PlayAnim,					idActor::Event_PlayAnim )

// MCG: added recurring damage
	EVENT( EV_DamageOverTime,			idActor::Event_DamageOverTime )
	EVENT( EV_DamageOverTimeEffect,		idActor::Event_DamageOverTimeEffect )
// MCG: script-callable joint crawl effect
	EVENT( EV_JointCrawlEffect,			idActor::Event_JointCrawlEffect )
// RAVEN END

END_CLASS

CLASS_STATES_DECLARATION ( idActor )
	STATE ( "Wait_Frame",				idActor::State_Wait_Frame )
	STATE ( "Wait_LegsAnim",			idActor::State_Wait_LegsAnim )
	STATE ( "Wait_TorsoAnim",			idActor::State_Wait_TorsoAnim )
END_CLASS_STATES

// RAVEN END

/*
=====================
idActor::idActor
=====================
*/
idActor::idActor( void )
{
	viewAxis.Identity();

	use_combat_bbox		= false;
	head				= NULL;

	eyeOffset.Zero();
	chestOffset.Zero();
	modelOffset.Zero();

	team				= 0;
	rank				= 0;
	fovDot				= 0.0f;
	pain_debounce_time	= 0;
	pain_delay			= 0;

	leftEyeJoint		= INVALID_JOINT;
	rightEyeJoint		= INVALID_JOINT;
	soundJoint			= INVALID_JOINT;
	eyeOffsetJoint		= INVALID_JOINT;
	chestOffsetJoint	= INVALID_JOINT;
	neckJoint			= INVALID_JOINT;
	headJoint			= INVALID_JOINT;

	deltaViewAngles.Zero();

	painTime			= 0;
	inDamageEvent		= false;
	disablePain			= true;
	allowEyeFocus		= false;
	
	blink_anim			= NULL;
	blink_time			= 0;
	blink_min			= 0;
	blink_max			= 0;

	finalBoss			= false;

	attachments.SetGranularity( 1 );

	enemyNode.SetOwner( this );
	enemyList.SetOwner( this );

	teamNode.SetOwner ( this );

	memset( &flashlight, 0, sizeof( flashlight ) );
	flashlightHandle = -1;

	deathPushTime	= 0;
	
	lightningEffects = 0;
	lightningNextTime = 0;
}

/*
=====================
idActor::~idActor
=====================
*/
idActor::~idActor( void ) {
// RAVEN BEGIN
// bdube: flashlights
	if ( flashlightHandle != -1 )	{
		gameRenderWorld->FreeLightDef( flashlightHandle );
		flashlightHandle = -1;
	}
// RAVEN END

	int i;
	idEntity *ent;

	StopSound( SND_CHANNEL_ANY, false );

	delete combatModel;
	combatModel = NULL;

	if ( head.GetEntity() ) {
		head.GetEntity()->ClearBody();
		head.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}

	// remove any attached entities
	for( i = 0; i < attachments.Num(); i++ ) {
		ent = attachments[ i ].ent.GetEntity();
		if ( ent ) {
			ent->PostEventMS( &EV_Remove, 0 );
		}
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
	float			fovDegreesClose;

	animPrefix	= "";

	spawnArgs.GetInt( "rank", "0", rank );
	spawnArgs.GetInt( "team", "0", team );
	spawnArgs.GetVector( "offsetModel", "0 0 0", modelOffset );

	spawnArgs.GetBool( "use_combat_bbox", "0", use_combat_bbox );	

	viewAxis = GetPhysics()->GetAxis();

	spawnArgs.GetFloat( "fov", "90", fovDegrees );
	spawnArgs.GetFloat( "fovClose", "200", fovDegreesClose );
	spawnArgs.GetFloat( "fovCloseRange", "180", fovCloseRange );
	SetFOV( fovDegrees, fovDegreesClose );

	pain_debounce_time	= 0;

	pain_delay = SEC2MS( spawnArgs.GetFloat( "pain_delay" ) );

	LoadAF ( );

	walkIK.Init( this, IK_ANIM, modelOffset );

	// the animation used to be set to the IK_ANIM at this point, but that was fixed, resulting in
	// attachments not binding correctly, so we're stuck setting the IK_ANIM before attaching things.
	animator.ClearAllAnims( gameLocal.time, 0 );
// RAVEN BEGIN
// jscott: new setframe stuff
	frameBlend_t frameBlend = { 0, 0, 0, 1.0f, 0 };
	animator.SetFrame( ANIMCHANNEL_ALL, animator.GetAnim( IK_ANIM ), frameBlend );
// RAVEN END

	// spawn any attachments we might have
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_attach", NULL );
	while ( kv ) {
		idDict args;

		args.Set( "classname", kv->GetValue().c_str() );

		// make items non-touchable so the player can't take them out of the character's hands
		args.Set( "no_touch", "1" );

		// don't let them drop to the floor
		args.Set( "dropToFloor", "0" );
		
		gameLocal.SpawnEntityDef( args, &ent );
		if ( !ent ) {
			gameLocal.Error( "Couldn't spawn '%s' to attach to entity '%s'", kv->GetValue().c_str(), name.c_str() );
		} else {
			Attach( ent );
		}
		kv = spawnArgs.MatchPrefix( "def_attach", kv );
	}

	SetupDamageGroups();

	// MP sets up heads on players from UpdateModelSetup()
	if( !gameLocal.isMultiplayer || !IsType( idPlayer::GetClassType() ) ) {
		SetupHead ( );
	}

	// clear the bind anim
	animator.ClearAllAnims( gameLocal.time, 0 );

	idEntity *headEnt = head.GetEntity();
	idAnimator *headAnimator;
	if ( headEnt ) {
		headAnimator = headEnt->GetAnimator();
	} else {
		headAnimator = &animator;
	}

	// set up blinking
	blink_anim = headAnimator->GetAnim( "blink" );
	blink_time = 0;	// it's ok to blink right away
	blink_min = SEC2MS( spawnArgs.GetFloat( "blink_min", "0.5" ) );
	blink_max = SEC2MS( spawnArgs.GetFloat( "blink_max", "8" ) );
	fl.allowAutoBlink	= spawnArgs.GetBool( "allowAutoBlink", "1" );

	if ( spawnArgs.GetString( "sound_bone", "", jointName ) ) {
		soundJoint = animator.GetJointHandle( jointName );
		if ( soundJoint == INVALID_JOINT ) {
			gameLocal.Warning( "idAnimated '%s' at (%s): cannot find joint '%s' for sound playback", name.c_str(), GetPhysics()->GetOrigin().ToString(0), jointName.c_str() );
		}
	}

	finalBoss = spawnArgs.GetBool( "finalBoss" );

// RAVEN BEGIN
// bdube: flashlight
	flashlightJoint = animator.GetJointHandle( spawnArgs.GetString ( "joint_flashlight", "flashlight" ) );

	memset( &flashlight, 0, sizeof( flashlight ) );
	flashlight.suppressLightInViewID = entityNumber + 1;
	flashlight.allowLightInViewID = 0;
	flashlight.lightId = 1 + entityNumber;	

	flashlight.allowLightInViewID = 1;

	idVec3	color;
	spawnArgs.GetVector ( "flashlightColor", "1 1 1", color );
	
	flashlight.pointLight							= spawnArgs.GetBool( "flashlightPointLight", "1" );
	flashlight.shader								= declManager->FindMaterial( spawnArgs.GetString( "mtr_flashlight", "muzzleflash" ), false );
	flashlight.shaderParms[ SHADERPARM_RED ]		= color[0];
	flashlight.shaderParms[ SHADERPARM_GREEN ]		= color[1];
	flashlight.shaderParms[ SHADERPARM_BLUE ]		= color[2];
	flashlight.shaderParms[ SHADERPARM_TIMESCALE ]	= 1.0f;

// RAVEN BEGIN
// dluetscher: added a default detail level to each render light
	flashlight.detailLevel = DEFAULT_LIGHT_DETAIL_LEVEL;
// RAVEN END

	flashlight.lightRadius[0] = flashlight.lightRadius[1] = 
		flashlight.lightRadius[2] = spawnArgs.GetFloat ( "flashlightRadius" );

	if ( !flashlight.pointLight ) {
		flashlight.target	= spawnArgs.GetVector( "flashlightTarget" );
		flashlight.up		= spawnArgs.GetVector( "flashlightUp" );
		flashlight.right	= spawnArgs.GetVector( "flashlightRight" );
		flashlight.end		= spawnArgs.GetVector( "flashlightTarget" );
	}
	
	spawnArgs.GetVector ( "flashlightOffset", "0 0 0", flashlightOffset );

	if ( spawnArgs.GetString( "flashlight_flaresurf", NULL ) ) {
		HideSurface( spawnArgs.GetString( "flashlight_flaresurf", NULL ) );
	}

// RAVEN END

	stateThread.SetName ( GetName() );
	stateThread.SetOwner ( this );

// RAVEN BEGIN
// cdr: Obstacle Avoidance
	fl.isAIObstacle = true;
// RAVEN END

	FinishSetup();
}

/*
================
idActor::FinishSetup
================
*/
void idActor::FinishSetup( void ) {
	if ( spawnArgs.GetBool ( "flashlight", "0" ) ) {
		FlashlightUpdate ( true );
	}

	SetupBody();
}

/*
================
idActor::SetupHead
================
*/
void idActor::SetupHead( const char* headDefName, idVec3 headOffset ) {
	idAFAttachment		*headEnt;
	idStr				jointName;
	jointHandle_t		joint;
	const idKeyValue	*sndKV;

	if ( gameLocal.isClient && head.GetEntity() == NULL ) {
		return;
	}

	// If we don't pass in a specific head model, try looking it up
	if( !headDefName[ 0 ] ) {
		headDefName = spawnArgs.GetString( "def_head", "" );
// jshepard: allow for heads to override persona defs
		headDefName = spawnArgs.GetString( "override_head", headDefName );		
	} 

	if ( headDefName[ 0 ] ) {
		// free the old head if we want a new one
		if( gameLocal.isServer ) {
			if( head && idStr::Icmp( head->spawnArgs.GetString( "classname" ), headDefName ) ) {
				head->SetName( va( "%s_oldhead", name.c_str() ) );
				head->PostEventMS( &EV_Remove, 0 );
				head = NULL;
			} else if( head ) {
				// the current head is OK
				return;
			}
		}

		jointName = spawnArgs.GetString( "joint_head" );
		joint = animator.GetJointHandle( jointName );
		if ( joint == INVALID_JOINT ) {
			gameLocal.Error( "Joint '%s' not found for 'joint_head' on '%s'", jointName.c_str(), name.c_str() );
		}

		// copy any sounds in case we have frame commands on the head
		idDict	args;
		sndKV = spawnArgs.MatchPrefix( "snd_", NULL );
		while( sndKV ) {
			args.Set( sndKV->GetKey(), sndKV->GetValue() );
			sndKV = spawnArgs.MatchPrefix( "snd_", sndKV );
		}

		if ( !gameLocal.isClient ) {
			args.Set( "classname", headDefName );
			if( !gameLocal.SpawnEntityDef( args, ( idEntity ** )&headEnt ) ) {
				gameLocal.Warning( "idActor::SetupHead() - Unknown head model '%s'\n", headDefName );
				return;
			}
			headEnt->spawnArgs.Set( "classname", headDefName );

			headEnt->SetName( va( "%s_head", name.c_str() ) );
			headEnt->SetBody ( this, headEnt->spawnArgs.GetString ( "model" ), joint );
			head = headEnt;
		} else {
			// we got our spawnid from the server
			headEnt = head.GetEntity();
			headEnt->SetBody ( this, headEnt->spawnArgs.GetString ( "model" ), joint );
			headEnt->GetRenderEntity()->suppressSurfaceInViewID = entityNumber + 1;
		}

		headEnt->BindToJoint( this, joint, true );
		headEnt->GetPhysics()->SetOrigin( vec3_origin + headOffset );		
		headEnt->GetPhysics()->SetAxis( mat3_identity );
	} else if ( head ) {
		head->PostEventMS( &EV_Remove, 0 );
		head = NULL;
	}

	if ( head ) {
		int i;
		// set the damage joint to be part of the head damage group
		for( i = 0; i < damageGroups.Num(); i++ ) {
			if ( damageGroups[ i ] == "head" ) {
				head->SetDamageJoint ( static_cast<jointHandle_t>( i ) );
				break;
			}
		}
		
		head->InitCopyJoints ( );
		head->SetInstance( instance );
	}
}

/*
================
idActor::Restart
================
*/
void idActor::Restart( void ) {
	assert( !head.GetEntity() );
	// MP sets up heads from UpdateModelSetup()
	if( !gameLocal.isMultiplayer ) {
		SetupHead();
	}
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

	savefile->WriteInt( team );

	// cnicholson: 	This line was already commented out, so we aint changing it.
	// No need to write/read teamNode
	// idLinkList<idActor>		teamNode;

	savefile->WriteInt( rank );
	savefile->WriteMat3( viewAxis );

// twhitaker: this confuses me... should we be writing out enemyList.Num() or enemyList.Next()->enemyNode->Num().  I'm not sure what these variables represent.
// cnicholson: bdube said to do it how id does it, so here goes:
    savefile->WriteInt( enemyList.Num() );
	for ( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		savefile->WriteObject( ent );
	}

	savefile->WriteInt( lightningNextTime );// cnicholson: Added unwritten var
	savefile->WriteInt( lightningEffects );	// cnicholson: Added unwritten var

	savefile->WriteFloat( fovDot );
	savefile->WriteFloat( fovCloseDot );
	savefile->WriteFloat( fovCloseRange );
	savefile->WriteVec3( eyeOffset );
	savefile->WriteVec3( chestOffset );
	savefile->WriteVec3( modelOffset );
	savefile->WriteAngles( deltaViewAngles );

	savefile->WriteInt( pain_debounce_time );
	savefile->WriteInt( pain_delay );

	savefile->WriteInt( damageGroups.Num() );
	for( i = 0; i < damageGroups.Num(); i++ ) {
		savefile->WriteString( damageGroups[ i ] );
	}

	savefile->WriteInt( damageScale.Num() );
	for( i = 0; i < damageScale.Num(); i++ ) {
		savefile->WriteFloat( damageScale[ i ] );
	}
//MCG
	savefile->WriteBool( inDamageEvent );

	savefile->WriteBool( use_combat_bbox );

	savefile->WriteJoint( leftEyeJoint );
	savefile->WriteJoint( rightEyeJoint );
	savefile->WriteJoint( soundJoint );
	savefile->WriteJoint( eyeOffsetJoint );	
	savefile->WriteJoint( chestOffsetJoint );
	savefile->WriteJoint( neckJoint );
	savefile->WriteJoint( headJoint );

	walkIK.Save( savefile );

	savefile->WriteString( animPrefix );
	savefile->WriteString( painType );
	savefile->WriteString( painAnim );
	
	savefile->WriteInt( blink_anim );
	savefile->WriteInt( blink_time );
	savefile->WriteInt( blink_min );
	savefile->WriteInt( blink_max );

	headAnim.Save( savefile );
	torsoAnim.Save( savefile );
	legsAnim.Save( savefile );

	stateThread.Save( savefile );

	// idEntityPtr<idAFAttachment>	head;
	head.Save( savefile );	// cnicholson: Added unwritten var

	savefile->WriteBool( disablePain );
	savefile->WriteBool( allowEyeFocus );
	savefile->WriteBool( finalBoss );

	savefile->WriteInt( painTime );

	savefile->WriteInt( attachments.Num() );
	for ( i = 0; i < attachments.Num(); i++ ) {
		attachments[i].ent.Save( savefile );
		savefile->WriteInt( attachments[i].channel );
	}

	vehicleController.Save ( savefile );

	// These aren't saved in the same order as they're declared, due to the dependency (I didn't see the need to change the order of declaration)
	savefile->WriteInt ( flashlightHandle );
	savefile->WriteJoint ( flashlightJoint );
	savefile->WriteVec3 ( flashlightOffset );
	savefile->WriteRenderLight ( flashlight );

	savefile->WriteInt( deathPushTime );
	savefile->WriteVec3( deathPushForce );
	savefile->WriteJoint( deathPushJoint );
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

	savefile->ReadInt( team );

// cnicholson: 	This line was already commented out, so we aint changing it.
// No need to write/read teamNode
	// idLinkList<idActor>		teamNode;

	savefile->ReadInt( rank );
	savefile->ReadMat3( viewAxis );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		savefile->ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->enemyNode.AddToEnd( enemyList );
		}
	}

	savefile->ReadInt( lightningEffects );
	savefile->ReadInt( lightningNextTime );

	savefile->ReadFloat( fovDot );
	savefile->ReadFloat( fovCloseDot );
	savefile->ReadFloat( fovCloseRange );
	savefile->ReadVec3( eyeOffset );
	savefile->ReadVec3( chestOffset );
	savefile->ReadVec3( modelOffset );
	savefile->ReadAngles( deltaViewAngles );

	savefile->ReadInt( pain_debounce_time );
	savefile->ReadInt( pain_delay );

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
//MCG
	savefile->ReadBool( inDamageEvent );

	savefile->ReadBool( use_combat_bbox );

	savefile->ReadJoint( leftEyeJoint );
	savefile->ReadJoint( rightEyeJoint );
	savefile->ReadJoint( soundJoint );
	savefile->ReadJoint( eyeOffsetJoint );	
	savefile->ReadJoint( chestOffsetJoint );
	savefile->ReadJoint( neckJoint );
	savefile->ReadJoint( headJoint );

	walkIK.Restore( savefile );

	savefile->ReadString( animPrefix );
	savefile->ReadString( painType );
	savefile->ReadString( painAnim );
	
	savefile->ReadInt( blink_anim );
	savefile->ReadInt( blink_time );
	savefile->ReadInt( blink_min );
	savefile->ReadInt( blink_max );

	headAnim.Restore( savefile );
	torsoAnim.Restore( savefile );
	legsAnim.Restore( savefile );

	stateThread.Restore( savefile, this );

// cnicholson: Restore unread var
	// idEntityPtr<idAFAttachment>	head;
	head.Restore(savefile);

	savefile->ReadBool( disablePain );
	savefile->ReadBool( allowEyeFocus );
	savefile->ReadBool( finalBoss );

	savefile->ReadInt( painTime );

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idAttachInfo &attach = attachments.Alloc();
		attach.ent.Restore( savefile );
		savefile->ReadInt( attach.channel );
	}

// RAVEN BEGIN
// bdube: added
	vehicleController.Restore ( savefile );

	savefile->ReadInt ( flashlightHandle );
	savefile->ReadJoint ( flashlightJoint );
	savefile->ReadVec3 ( flashlightOffset );
	savefile->ReadRenderLight ( flashlight );
	if ( flashlightHandle != -1 ) {		
		flashlightHandle = gameRenderWorld->AddLightDef( &flashlight );
	}

	savefile->ReadInt( deathPushTime );
	savefile->ReadVec3( deathPushForce );
	savefile->ReadJoint( deathPushJoint );

// mekberg: update this
	FlashlightUpdate( );
// RAVEN END
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			if ( ent->IsType( idLight::GetClassType() ) ) {
// RAVEN END
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			if ( ent->IsType( idLight::GetClassType() ) ) {
// RAVEN END
				static_cast<idLight *>( ent )->On();
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
bool idActor::LoadAF( const char* keyname, bool purgeAF /* = false */ ) {
	idStr fileName;

	if ( !keyname || !*keyname ) {
		keyname = "ragdoll";
	}

	if ( !spawnArgs.GetString( keyname, "*unknown*", fileName ) || !fileName.Length() ) {
		return false;
	}
	af.SetAnimator( GetAnimator() );
	return af.Load( this, fileName, purgeAF );
}

/*
=====================
idActor::SetupBody
=====================
*/
void idActor::SetupBody( void ) {
	const char*		jointname;
	idAnimator*		headAnimator;
	float			height;

	animator.ClearAllAnims( gameLocal.time, 0 );
	animator.ClearAllJoints();

	// Cache the head entity pointer and determine which animator to use
	idAFAttachment *headEnt = head.GetEntity();
	if ( headEnt ) {
		headAnimator = headEnt->GetAnimator();
	} else {
		headAnimator = GetAnimator( );
	}
	
	// Get left eye joint
	if ( !headEnt || !headEnt->spawnArgs.GetString ( "joint_leftEye", "", &jointname ) ) {		
		jointname = spawnArgs.GetString( "joint_leftEye" );
	}
	leftEyeJoint = headAnimator->GetJointHandle( jointname );

	// Get right eye joint
	if ( !headEnt || !headEnt->spawnArgs.GetString ( "joint_rightEye", "", &jointname ) ) {		
		jointname = spawnArgs.GetString( "joint_rightEye" );
	}
	rightEyeJoint = headAnimator->GetJointHandle( jointname );
				
	// If head height is specified, just use that
	if ( spawnArgs.GetFloat( "eye_height", "0", height ) ) {
		SetEyeHeight( height );
	} else { 
		// See if there is an eye offset joint specified, if not just use the left eye joint
		if ( !headEnt || !headEnt->spawnArgs.GetString( "joint_eyeOffset", "", &jointname ) ) {		
			jointname = spawnArgs.GetString( "joint_eyeOffset" );
		}
		// Get the eye offset joint
		eyeOffsetJoint = headAnimator->GetJointHandle( jointname );
	}

	// If eye height is specified, just use that
	if ( spawnArgs.GetFloat( "chest_height", "0", height ) ) {
		SetChestHeight( height );
	} else { 
		// See if there is an eye offset joint specified, if not just use the left eye joint
		spawnArgs.GetString( "joint_chestOffset", "", &jointname );
		// Get the chest offset joint
		chestOffsetJoint = animator.GetJointHandle( jointname );
	}

	// Get the neck joint
	spawnArgs.GetString( "joint_look_neck", "", &jointname );
	neckJoint = animator.GetJointHandle( jointname );

	// Get the head joint
	spawnArgs.GetString( "joint_look_head", "", &jointname );
	headJoint = animator.GetJointHandle( jointname );

	headAnim.Init( this, &animator, ANIMCHANNEL_HEAD );
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
	if ( !blink_anim || ( health <= 0 ) || ( blink_time > gameLocal.time ) || !fl.allowAutoBlink ) {
		return;
	}

	idAnimator *animator = head.GetEntity() ? head->GetAnimator() : &this->animator;
	animator->PlayAnim( ANIMCHANNEL_EYELIDS, blink_anim, gameLocal.time, 1 );

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
// RAVEN BEGIN
// bdube: position player in seat (nmckenzie: copy and paste from the player version of this call)
	if ( vehicleController.IsDriving ( ) ) {
		vehicleController.GetDriverPosition ( origin, axis );
		return true;
	}
// RAVEN END

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
}

/*
=====================
idActor::OnStateThreadClear
=====================
*/
void idActor::OnStateThreadClear( const char *statename, int flags ) {
}

/*
=====================
idActor::SetState
=====================
*/
void idActor::SetState( const char *statename, int flags ) {
	OnStateThreadClear( statename, flags );
	stateThread.SetState ( statename, 0, 0, flags );
}

/*
=====================
idActor::PostState
=====================
*/
void idActor::PostState ( const char* statename, int delay, int flags ) {
	if ( SRESULT_OK != stateThread.PostState ( statename, 0, delay, flags ) ) {
		gameLocal.Error ( "unknown state '%s' on entity '%s'", statename, GetName() );
	}
}

/*
=====================
idActor::InterruptState
=====================
*/
void idActor::InterruptState ( const char* statename, int delay, int flags ) {
	if ( SRESULT_OK != stateThread.InterruptState ( statename, 0, delay, flags ) ) {
		gameLocal.Error ( "unknown state '%s' on entity '%s'", statename, GetName() );
	}
}

/*
=====================
idActor::UpdateState
=====================
*/
void idActor::UpdateState ( void ) {
	stateThread.Execute ( );
}

/***********************************************************************

	vision

***********************************************************************/

/*
=====================
idActor::setFov
=====================
*/
void idActor::SetFOV( float fov, float fovClose ) {
	fovDot = idMath::Cos( DEG2RAD( fov * 0.5f ) );
	fovCloseDot = idMath::Cos( DEG2RAD( fovClose * 0.5f ) );
}

/*
=====================
idActor::SetEyeHeight
=====================
*/
void idActor::SetEyeHeight( float height ) {
	eyeOffset = GetPhysics()->GetGravityNormal() * -height;
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
idActor::SetChestHeight
=====================
*/
void idActor::SetChestHeight( float height ) {
	chestOffset = GetPhysics()->GetGravityNormal() * -height;
}

/*
=====================
idActor::GetEyePosition
=====================
*/
idVec3 idActor::GetEyePosition( void ) const {
	return GetPhysics()->GetOrigin() + eyeOffset * viewAxis;
}

/*
=====================
idActor::GetChestPosition
=====================
*/
idVec3 idActor::GetChestPosition( void ) const {
	return GetPhysics()->GetOrigin() + chestOffset * viewAxis;
}

// RAVEN BEGIN
// bdube: flashlights

/*
=====================
idActor::GetGroundEntity
=====================
*/
idEntity* idActor::GetGroundEntity( void ) const {
	return static_cast<idPhysics_Actor*>(GetPhysics())->GetGroundEntity();
}

/*
=====================
idActor::Present
=====================
*/
void idActor::Present( void ) {
	idAFEntity_Gibbable::Present();

	FlashlightUpdate();
}

/*
=====================
idActor::Event_Teleport
=====================
*/
void idActor::Event_Teleport( idVec3 &newPos, idVec3 &newAngles ) {
	Teleport ( newPos, newAngles.ToAngles(), NULL );
}

/*
=====================
idActor::Event_EnterVehicle
=====================
*/
void idActor::Event_EnterVehicle ( idEntity* vehicle ) {
	if ( IsInVehicle ( ) ) {
		return;
	}
	EnterVehicle ( vehicle );
}

/*
=====================
idActor::Event_ExitVehicle
=====================
*/
void idActor::Event_ExitVehicle ( bool force ) {
	if ( !IsInVehicle ( ) ) {
		return;
	}
	ExitVehicle ( force );
}

/*
=====================
idActor::Event_PreExitVehicle
=====================
*/
void idActor::Event_PreExitVehicle ( bool force ) {
	if ( !IsInVehicle ( ) ) {
		return;
	}

	// call the script func regardless of the fact that we may not be getting out just yet.
	// this allows things in the script to happen before ejection actually occurs (such as screen fades).
	vehicleController.GetVehicle()->OnExit();

	// this is done because having an exit delay when the player is dead was causing bustedness if you died in the walker
	//	specifically the restart menu would not appear if you were still in the walker
	if ( health > 0 ) {
		PostEventMS( &AI_PostExitVehicle, vehicleController.GetVehicle()->spawnArgs.GetInt( "exit_vehicle_delay" ), force );
	}
	else {
		ExitVehicle(true);
	}
}

/*
=====================
idActor::Event_Flashlight
=====================
*/
void idActor::Event_Flashlight( bool on ) {	
	if ( on ) {
		FlashlightUpdate(true);
	} else {
		if ( flashlightHandle != -1 ) {
			gameRenderWorld->FreeLightDef ( flashlightHandle );
			flashlightHandle = -1;
		}

		if ( spawnArgs.GetString( "flashlight_flaresurf", NULL ) ) {
			HideSurface( spawnArgs.GetString( "flashlight_flaresurf", NULL ) );
		}
		if ( spawnArgs.GetString( "fx_flashlight", NULL ) ) {
			StopEffect( "fx_flashlight", true );
		}
	}
}

/*
=====================
idActor::FlashlightUpdate
=====================
*/
void idActor::FlashlightUpdate ( bool forceOn ) {

	// Dont do anything if flashlight is off and its not being forced on
	if ( !forceOn && flashlightHandle == -1 ) {
		return;
	}

	if ( !flashlight.lightRadius[0] || flashlightJoint == INVALID_JOINT ) {
		return;
	}

	if ( forceOn && flashlightHandle == -1 ) {
		//first time turning it on
		if ( spawnArgs.GetString( "flashlight_flaresurf", NULL ) ) {
			ShowSurface( spawnArgs.GetString( "flashlight_flaresurf", NULL ) );
		}
		if ( spawnArgs.GetString( "fx_flashlight", NULL ) ) {
			PlayEffect( "fx_flashlight", flashlightJoint, true );
		}
	}

	// the flash has an explicit joint for locating it
	GetJointWorldTransform ( flashlightJoint, gameLocal.time, flashlight.origin, flashlight.axis );
	flashlight.origin += flashlightOffset * flashlight.axis;
	
	if ( flashlightHandle != -1 ) {
		gameRenderWorld->UpdateLightDef( flashlightHandle, &flashlight );
	} else {
		flashlightHandle = gameRenderWorld->AddLightDef( &flashlight );
	}

}

// RAVEN END

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
bool idActor::CheckFOV( const idVec3 &pos, float ang ) const {
	float testAng = ( ang != -1.0f ) ? idMath::Cos( DEG2RAD( ang * 0.5f ) ) : fovDot;
	if ( testAng == 1.0f ) {
		return true;
	}

	if(!GetPhysics()) {
		return false;
	}
	
	float	dot;
	float	dist;
	idVec3	delta;
	
	delta = pos - GetEyePosition();
	dist = delta.LengthFast();

	//NOTE!!!
	//This logic is BACKWARDS, but it's too late in the project
	//for me to feel comfortable fixing this.  It SHOULD be:
	//if ( ang == -1.0f )
	// - MCG
	if ( ang != -1.0f )
	{//not overriding dot test value
		if (dist<fovCloseRange) {
			testAng = fovCloseDot;	// allow a wider FOV if close enough
		}
	}

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	dot = viewAxis[ 0 ] * delta;

	return ( dot >= testAng );
}

/*
=====================
idActor::HasFOV
=====================
*/

bool idActor::HasFOV( idEntity *ent )
{
	// Fixme: Make this do something, anything.
	return true;
}
// RAVEN END

/*
=====================
idActor::CanSee
=====================
*/
bool idActor::CanSee( const idEntity *ent, bool useFov ) const {
	return CanSeeFrom ( GetEyePosition ( ), ent, useFov );
}

/*
=====================
idActor::CanSeeFrom
=====================
*/
bool idActor::CanSeeFrom ( const idVec3& from, const idEntity *ent, bool useFov ) const {
	idVec3 toPos;

	if ( !ent || ent->IsHidden() ) {
		return false;
	}

	if ( ent->IsType( idActor::Type ) ) {
		toPos  = ((idActor*)ent)->GetEyePosition();
	} else {
		toPos = ent->GetPhysics()->GetAbsBounds().GetCenter ( );
	}

	return CanSeeFrom ( from, toPos, useFov );
}

bool idActor::CanSeeFrom ( const idVec3& from, const idVec3& toPos, bool useFov ) const {
	trace_t tr;

	if ( useFov && !CheckFOV( toPos ) ) {
		return false;
	}
	if ( g_perfTest_aiNoVisTrace.GetBool() ) {
		return true;
	}
	gameLocal.TracePoint( this, tr, from, toPos, MASK_OPAQUE, this );
	if ( tr.fraction >= 1.0f ) { // || ( gameLocal.GetTraceEntity( tr ) == ent ) ) {
		return true;
	}

	return false;
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

// RAVEN BEGIN
// bdube: set the combat model reguardless
	if ( 1 ) { // !use_combat_bbox ) {
// RAVEN END
		if ( combatModel ) {
			combatModel->Unlink();
			combatModel->LoadModel( modelDefHandle );
		} else {
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
			RV_PUSH_HEAP_MEM(this);
// RAVEN END
			combatModel = new idClipModel( modelDefHandle );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
			RV_POP_HEAP();
// RAVEN END
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
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		combatModel->Link( this, 0, renderEntity.origin, renderEntity.axis, modelDefHandle );
// RAVEN END
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

	// Raise the origin up 5 units to help ensure the ragdoll doesnt start in the ground
	GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() + GetPhysics()->GetGravityNormal() * -5.0f );
	UpdateModelTransform();

	// disable the monster bounding box
	GetPhysics()->DisableClip();

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
	idList<idEntity *> list;
	idMoveableItem::DropItems( this, "death", &list );
	for ( int i = 0; i < list.Num(); i++ ) {
		if ( list[i] && list[i]->GetPhysics() )
		{
			idVec3 velocity;
			float pitchDir = gameLocal.random.CRandomFloat()>0.0f?1.0f:-1.0f;
			float yawDir = gameLocal.random.CRandomFloat()>0.0f?1.0f:-1.0f;
			float rollDir = gameLocal.random.CRandomFloat()>0.0f?1.0f:-1.0f;
			velocity.Set( pitchDir*((gameLocal.random.RandomFloat() * 200.0f) + 50.0f),
							yawDir*((gameLocal.random.RandomFloat() * 200.0f) + 50.0f),
							(gameLocal.random.RandomFloat() * 300.0f) + 100.0f );
			list[i]->GetPhysics()->SetAngularVelocity( idVec3( pitchDir*((gameLocal.random.RandomFloat() * 6.0f) + 2.0f),
															yawDir*((gameLocal.random.RandomFloat() * 6.0f) + 2.0f),
															rollDir*((gameLocal.random.RandomFloat() * 10.0f) + 3.0f)));
			if ( gibbed ) {
				//only throw them if we end up gibbed?
				list[i]->GetPhysics()->SetLinearVelocity( velocity );
			}
		}
	}

	// drop any articulated figures the actor is holding
	idAFEntity_Base::DropAFs( this, "death", NULL );

	RemoveAttachments();

// RAVEN BEGIN
// bdube: evaluate one ragdoll frame
	RunPhysics();
// RAVEN END

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
		return idAFEntity_Gibbable::UpdateAnimationControllers();
	} else {
		animator.ClearAFPose();
	}

	if ( walkIK.IsInitialized() ) {
		walkIK.Evaluate();
		return true;
	}

	idMat3			  axis;
	idAnimatedEntity* headEnt = head.GetEntity( );
	if ( !headEnt ) {
		headEnt = this;
	}
	
	// Dynamically update the eye offset if a joint was specified
	if ( eyeOffsetJoint != INVALID_JOINT ) {
		headEnt->GetJointWorldTransform( eyeOffsetJoint, gameLocal.time, eyeOffset, axis );
		eyeOffset = (eyeOffset - GetPhysics()->GetOrigin()) * viewAxis.Transpose( );
	}

	if ( DebugFilter( ai_debugMove ) ) { // RED = Eye Pos & orientation
		gameRenderWorld->DebugArrow( colorRed, GetEyePosition(), GetEyePosition() + viewAxis[ 0 ] * 32.0f, 4, gameLocal.msec );
	}

	// Dynamically update the chest offset if a joint was specified
	if ( chestOffsetJoint != INVALID_JOINT ) {
		GetJointWorldTransform( chestOffsetJoint, gameLocal.time, chestOffset, axis );
		chestOffset = ( chestOffset - GetPhysics()->GetOrigin() ) * viewAxis.Transpose();
	}

	if ( DebugFilter( ai_debugMove ) ) { // RED = Eye Pos & orientation
		gameRenderWorld->DebugArrow( colorPink, GetChestPosition(), GetChestPosition() + viewAxis[ 0 ] * 32.0f, 4, gameLocal.msec );
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
	ent->BindToJoint( this, joint, true );
	ent->cinematic = cinematic;
}

idEntity* idActor::FindAttachment( const char* attachmentName )
{
	idEntity *ent = NULL;
	const char* fullName = va("idAFAttachment_%s",attachmentName);
	// find the specified attachment
	for( int i = 0; i < attachments.Num(); i++ ) {
		ent = attachments[ i ].ent.GetEntity();
		if ( ent && !ent->name.CmpPrefix(fullName) ) {
			return ent;
		}
	}
	return NULL;
}

void idActor::HideAttachment( const char* attachmentName )
{
	idEntity *ent = FindAttachment( attachmentName );
	if ( ent )
	{
		ent->Hide();
	}
}

void idActor::ShowAttachment( const char* attachmentName )
{
	idEntity *ent = FindAttachment( attachmentName );
	if ( ent )
	{
		ent->Show();
	}
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
idActor *idActor::ClosestEnemyToPoint( const idVec3 &pos, float maxRange, bool returnFirst, bool checkPVS ) {
	idActor		*ent;
	idActor		*bestEnt;
	float		bestDistSquared;
	float		distSquared;
	idVec3		delta;
	pvsHandle_t pvs;
	
	//just to supress the compiler warning
    pvs.i = 0;

	if ( checkPVS ) {
		// Setup our local variables used in the search
		pvs	 = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );
	}

	bestDistSquared = maxRange?(maxRange*maxRange):idMath::INFINITY;
	bestEnt = NULL;
	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( ent->fl.hidden ) {
			continue;
		}
		delta = ent->GetPhysics()->GetOrigin() - pos;
		distSquared = delta.LengthSqr();
		if ( distSquared < bestDistSquared ) {
			if ( checkPVS ) {
				// If this enemy isnt in the same pvps then use them as a backup
				if ( pvs.i > 0 
					&& pvs.i < MAX_CURRENT_PVS
					&& !gameLocal.pvs.InCurrentPVS( pvs, ent->GetPVSAreas(), ent->GetNumPVSAreas() ) ) {
					continue;
				}
			}
			bestEnt = ent;
			bestDistSquared = distSquared;
			if ( returnFirst ) {
				break;
			}
		}
	}

	if ( checkPVS ) {
		gameLocal.pvs.FreeCurrentPVS( pvs );
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
idActor::StopAnimState
=====================
*/
void idActor::StopAnimState( int channel ) {
	GetAnimState( channel ).Shutdown ( );
}

/*
=====================
idActor::PostAnimState
=====================
*/
void idActor::PostAnimState( int channel, const char* statename, int blendFrames, int delay, int flags ) {
	GetAnimState( channel ).PostState( statename, blendFrames, delay, flags );
}

/*
=====================
idActor::SetAnimState
=====================
*/
void idActor::SetAnimState( int channel, const char *statename, int blendFrames, int flags ) {
	switch ( channel ) {
		case ANIMCHANNEL_HEAD :
			headAnim.GetStateThread().Clear();
			break;
			
		case ANIMCHANNEL_TORSO :
			torsoAnim.GetStateThread().Clear();
			legsAnim.Enable( blendFrames );
			break;

		case ANIMCHANNEL_LEGS :
			legsAnim.GetStateThread().Clear();
			torsoAnim.Enable( blendFrames );
			break;
	}		

	OnStateChange( channel );
	
	PostAnimState( channel, statename, blendFrames, flags );
}

/*
=====================
idActor::OnStateChange
=====================
*/
void idActor::OnStateChange ( int channel ) {
	allowEyeFocus = true;

	// Only clear eye focus on head channel change
	if ( channel == ANIMCHANNEL_HEAD ) {
		return;
	}

	disablePain = false;
}

/*
=====================
idActor::OnFriendlyFire
=====================
*/
void idActor::OnFriendlyFire ( idActor* attacker ) {
}

/*
=====================
idActor::UpdateAnimState
=====================
*/
void idActor::UpdateAnimState( void ) {
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_ANIM);
// RAVEN END
	headAnim.UpdateState();
	torsoAnim.UpdateState();
	legsAnim.UpdateState();
}

/*
=====================
idActor::GetAnim
=====================
*/
int idActor::GetAnim( int channel, const char *animname, bool forcePrefix ) {
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

	// Allow for anim substitution
	animname = spawnArgs.GetString ( va("anim %s", animname ), animname );

	if ( animPrefix.Length() ) {
		temp = va( "%s_%s", animPrefix.c_str(), animname );
		anim = animatorPtr->GetAnim( temp );
		if ( anim ) {
			return anim;
		} else if ( forcePrefix ) {
			return NULL;
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
					headAnimator->PlayAnim( ANIMCHANNEL_LEGS, anim, gameLocal.time, blendTime );
					headAnimator->CurrentAnim( ANIMCHANNEL_LEGS )->SetCycleCount( cycle );
					headAnimator->CurrentAnim( ANIMCHANNEL_LEGS )->SetStartTime( starttime );
				} else {
					headEnt->PlayIdleAnim( ANIMCHANNEL_LEGS, blendTime );
				}
			}
		}
	} else if ( syncToChannel == ANIMCHANNEL_HEAD ) {
		headEnt = head.GetEntity();
		if ( headEnt ) {
			headAnimator = headEnt->GetAnimator();
			syncAnim = headAnimator->CurrentAnim( ANIMCHANNEL_LEGS );
			if ( syncAnim ) {
				anim = GetAnim( channel, syncAnim->AnimFullName() );
				if ( !anim ) {
					anim = GetAnim( channel, syncAnim->AnimName() );
				}
				if ( anim ) {
					cycle = headAnimator->CurrentAnim( ANIMCHANNEL_LEGS )->GetCycleCount();
					starttime = headAnimator->CurrentAnim( ANIMCHANNEL_LEGS )->GetStartTime();
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
	// for multiplayer we use client-side models
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
	
	gameLocal.PlayEffect ( spawnArgs, "fx_gib", GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
}

void idActor::CheckDeathObjectives( void )
{
	idPlayer *player = gameLocal.GetLocalPlayer();

	if ( !player || !player->GetObjectiveHud() ) {
		return;
	}

	if ( spawnArgs.GetString( "objectivetitle_failed", NULL ) ) {
		player->GetObjectiveHud()->SetStateString( "objective", "2" );
		player->GetObjectiveHud()->SetStateString( "objectivetext", common->GetLocalizedString( spawnArgs.GetString( "objectivetext_failed" ) ) );
		player->GetObjectiveHud()->SetStateString( "objectivetitle", common->GetLocalizedString( spawnArgs.GetString( "objectivetitle_failed" ) ) );

		player->FailObjective( spawnArgs.GetString( "objectivetitle_failed" ) );
	}

	if ( spawnArgs.GetString( "objectivetitle_completed", NULL ) ) {
		player->GetObjectiveHud()->SetStateString( "objective", "2" );
		player->GetObjectiveHud()->SetStateString( "objectivetext", common->GetLocalizedString( spawnArgs.GetString( "objectivetext_completed" ) ) );
		player->GetObjectiveHud()->SetStateString( "objectivetitle", common->GetLocalizedString( spawnArgs.GetString( "objectivetitle_completed" ) ) );

		player->CompleteObjective( spawnArgs.GetString( "objectivetitle_completed" ) );
	}
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
void idActor::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, const int location ) {
	if ( !fl.takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Error( "Unknown damageDef '%s'", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" ) * damageScale;
	damage = GetDamageForLocation( damage, location );

	// friendly fire damage
	bool noDmgFeedback = false;
	if ( attacker->IsType ( idActor::Type ) && static_cast<idActor*>(attacker)->team == team ) {
		
		OnFriendlyFire ( static_cast<idActor*>(attacker) );
		
		// jshepard:
		// if the player deals friendly fire damage it is reduced to 0. If the damage is splash damage,
		// then the victim should use a pain anim.
		if( static_cast<idPlayer*>( attacker ) == gameLocal.GetLocalPlayer() )	{
			
			//play pain (maybe one day a special anim?) for damages that have the cower keyword
			if ( damageDef->GetBool( "cower" ))	{
				Pain( inflictor, attacker, damage, dir, location );
			}
			
			//reduce the damage
			damage = 0;
			noDmgFeedback = true;
		}		

		// reduce friendly fire damage by the teamscale
		damage = floor( damage * damageDef->GetFloat ( "teamScale", "0.5" ) );


	}

	if ( !IsType( idPlayer::GetClassType() ) && attacker->IsType( idAI::GetClassType() ) ) {
		if ( ((idAI*)attacker)->aifl.killerGuard ) {
			//Hard-coded to do lots of damage
			damage = 100;
		}
	}

	if ( !noDmgFeedback ) {
		// inform the attacker that they hit someone
		attacker->DamageFeedback( this, inflictor, damage );
	}

// RAVEN BEGIN
// jnewquist: FIXME - Was this removed from Xenon intentionally?
#ifdef _XENON
	// singleplayer stat reporting.
	if(!gameLocal.isMultiplayer) {
		int methodOfDeath = -1;
		// jnewquist: Use accessor for static class type 
		if ( inflictor->IsType( idProjectile::GetClassType() ) ) {
		// RAVEN END
			methodOfDeath = static_cast<idProjectile*>(inflictor)->methodOfDeath;
		} else if ( inflictor->IsType( idPlayer::GetClassType() ) ) {
			// hitscan weapon
			methodOfDeath = static_cast<idPlayer*>(inflictor)->GetCurrentWeapon();
		}
		if( methodOfDeath != -1 && attacker && attacker->IsType( idActor::Type ) ) {
// jnewquist: Fix Xenon compile warning
			statManager->WeaponHit( static_cast<idActor*> (attacker) , this, methodOfDeath, !!damage );
		}
	}
#endif
// RAVEN END

// RAVEN BEGIN
// MCG - added damage over time
	if ( !inDamageEvent ) {
		if ( damageDef->GetFloat( "dot_duration" ) ) {
			int endTime;
			if ( damageDef->GetFloat( "dot_duration" ) == -1 ) {
				endTime = -1;
			} else {
				endTime = gameLocal.GetTime() + SEC2MS(damageDef->GetFloat( "dot_duration" ));
			}
			int interval = SEC2MS(damageDef->GetFloat( "dot_interval", "0" ));
			if ( endTime == -1 || gameLocal.GetTime() + interval <= endTime ) {//post it again
				PostEventMS( &EV_DamageOverTime, interval, endTime, interval, inflictor, attacker, dir, damageDefName, damageScale, location );
			}
			if ( damageDef->GetString( "fx_dot", NULL ) ) {
				ProcessEvent( &EV_DamageOverTimeEffect, endTime, interval, damageDefName );
			}
			if ( damageDef->GetString( "snd_dot_start", NULL ) ) {
				StartSound ( "snd_dot_start", SND_CHANNEL_ANY, 0, false, NULL );
			}
		}
	}
// RAVEN END

	if ( damage > 0 ) {
		int oldHealth = health;
		AdjustHealthByDamage ( damage );
		if ( health <= 0 ) {

			//allow for quick burning
			if (damageDef->GetFloat( "quickburn", "0" ) && !spawnArgs.GetFloat("no_quickburn", "0"))	{
				fl.quickBurn = true;
			}

			if ( health < -999 ) {
				health = -999;
			}
			//annoying hack for StartRagdoll
			bool saveGibbed = gibbed;
			bool canDMG_Gib = (spawnArgs.GetBool( "gib" ) | spawnArgs.GetBool( "DMG_gib" ));
			if ( health < -20 )
			{
				if ( (spawnArgs.GetBool( "gib" ) && damageDef->GetBool( "gib" )) || 
					 (canDMG_Gib && damageDef->GetBool( "DMG_gib"))) 
				{
					gibbed = true;
				}
			}
			Killed( inflictor, attacker, damage, dir, location );
			gibbed = saveGibbed;
			if ( health < -20 )
			{
				if ( (spawnArgs.GetBool( "gib" ) && damageDef->GetBool( "gib" )) || 
					 (canDMG_Gib && damageDef->GetBool( "DMG_gib"))) 
				{
					Gib( dir, damageDefName );
				}
			}

			if ( oldHealth > 0 && !gibbed && !fl.quickBurn) {	
  				float pushScale = 1.0f;
  				if ( inflictor && inflictor->IsType ( idPlayer::GetClassType() ) ) {
  					pushScale = static_cast<idPlayer*>(inflictor)->PowerUpModifier ( PMOD_PROJECTILE_DEATHPUSH );
  				}
  				InitDeathPush ( dir, location, damageDef, pushScale );
  			}
		} else {
			painType = damageDef->GetString ( "pain" );
			Pain( inflictor, attacker, damage, dir, location );
		}
	} else {
		// don't accumulate knockback
		/*
		if ( af.IsLoaded() ) {
			// clear impacts
			af.Rest();

			// physics is turned off by calli/ng af.Rest()
			BecomeActive( TH_PHYSICS );
		}
		*/
	}
}

/*
=====================
idActor::InitDeathPush
=====================
*/
void idActor::InitDeathPush ( const idVec3& dir, int location, const idDict* damageDict, float pushScale ) {
	idVec2	forceMin;
	idVec2	forceMax;

	if( !af.IsActive() ) {
		return;
	}
	
	if ( deathPushTime > gameLocal.time ) { 
		return;
	}
	
	if ( !damageDict->GetInt ( "deathPush", "0", deathPushTime ) || deathPushTime <= 0 ) {
		return;
	}
	
	damageDict->GetVec2( "deathPushMin", "", forceMin );	
	damageDict->GetVec2( "deathPushMax", "", forceMax );	

/*
	forceMin *= (pushScale * GetPhysics()->GetMass());
	forceMax *= (pushScale * GetPhysics()->GetMass());
*/
	forceMin *= (pushScale);
	forceMax *= (pushScale);

	deathPushForce = dir;
	deathPushForce.Normalize ( );
	deathPushForce = rvRandom::flrand ( forceMin.x, forceMax.x ) * deathPushForce + 
		    -rvRandom::flrand ( forceMin.y, forceMax.y ) * GetPhysics()->GetGravityNormal ( );
	
	deathPushTime += gameLocal.time;
	deathPushJoint = (jointHandle_t) location;
}

/*
=====================
idActor::DeathPush
=====================
*/
void idActor::DeathPush ( void ) {
	if ( deathPushTime <= gameLocal.time ) {
		return;
	}

	idVec3 center;
	center = GetPhysics()->GetAbsBounds ( ).GetCenter();

	GetPhysics()->ApplyImpulse ( 0, center, -0.5f * GetPhysics()->GetMass () * MS2SEC(gameLocal.GetMSec()) * GetPhysics()->GetGravity ( ) );
	
	if ( deathPushJoint != INVALID_JOINT ) {
		idVec3 origin;
		idMat3 axis;
		GetJointWorldTransform ( deathPushJoint, gameLocal.time, origin, axis );
		GetPhysics()->ApplyImpulse ( 0, origin, deathPushForce );
	} else {
		GetPhysics()->ApplyImpulse ( 0, center, deathPushForce );
	}
}

/*
=====================
idActor::SkipImpulse
=====================
*/
bool idActor::SkipImpulse( idEntity* ent, int id ) {	
	return idAFEntity_Gibbable::SkipImpulse( ent, id ) || health <= 0 || gibbed || ent->IsType( idActor::GetClassType() ) || ent->IsType( idProjectile::GetClassType() );
}

/*
=====================
idActor::AddDamageEffect
=====================
*/
void idActor::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor ) {
	if ( !gameLocal.isMultiplayer && inflictor && inflictor->IsType ( idActor::GetClassType ( ) ) ) {
		if ( static_cast<idActor*>(inflictor)->team == team ) {
			return;
		}
	}
	idAFEntity_Gibbable::AddDamageEffect( collision, velocity, damageDefName, inflictor );
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
bool idActor::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( af.IsLoaded() ) {
		// clear impacts
		af.Rest();

		// physics is turned off by calling af.Rest()
		BecomeActive( TH_PHYSICS );
	}

	if ( gameLocal.time < pain_debounce_time ) {
		return false;
	}
	
	// No pain if being hit by a friendly target
	// jshepard: friendly targets can now cause pain
/*
	if ( attacker && attacker->IsType ( idActor::GetClassType ( ) ) ) {
		if ( static_cast<idActor*>( attacker )->team == team ) {
			return false;
		}
	}
*/

	// don't play pain sounds more than necessary
	pain_debounce_time = gameLocal.time + pain_delay;

	float f;
// RAVEN BEGIN
// mekberg: fixed divide by zero
	float spawnHealth = spawnArgs.GetFloat ( "health", "1" );
	if (spawnHealth<1.0f) {
		spawnHealth = 1.0f;		// more devide by zero nonsense
	}
	f = (float)damage / spawnHealth;
// RAVEN END
	if( gameLocal.isMultiplayer && IsType( idPlayer::GetClassType() ) && (health < 0.25f * ((idPlayer*)this)->inventory.maxHealth) ) {
		StartSound( "snd_pain_low_health", SND_CHANNEL_VOICE, 0, false, NULL );
	} else {
		if ( f > 0.75f ) {
			StartSound( "snd_pain_huge", SND_CHANNEL_VOICE, 0, false, NULL );
		} else if ( f > 0.5f ) {
			StartSound( "snd_pain_large", SND_CHANNEL_VOICE, 0, false, NULL );
		} else if ( f > 0.25f ) {
			StartSound( "snd_pain_medium", SND_CHANNEL_VOICE, 0, false, NULL );
		} else {
			StartSound( "snd_pain_small", SND_CHANNEL_VOICE, 0, false, NULL );
		}
	}

	if ( disablePain || ( gameLocal.time < painTime ) ) {
		// don't play a pain anim
		return false;
	}

	// set the pain anim
	idStr damageGroup = GetDamageGroup( location );

	painAnim.Clear ( );

	// If we have both a damage group and a pain type then check that combination first
	if ( damageGroup.Length ( ) && painType.Length ( ) ) {
		painAnim = va ( "pain_%s_%s", painType.c_str(), damageGroup.c_str() );
		if ( !animator.HasAnim ( painAnim ) ) {
			painAnim.Clear ( );
		}
	}
	
	// Do we have a pain anim for just the pain type?
	if ( !painAnim.Length ( ) && painType.Length ( ) ) {
		painAnim = va ( "pain_%s", painType.c_str() );
		if ( !animator.HasAnim ( painAnim ) ) {
			painAnim.Clear ( );
		}
	}

	// Do we have a pain anim for just the damage group?
	if ( !painAnim.Length ( ) && damageGroup.Length ( ) ) {
		painAnim = va ( "pain_%s", damageGroup.c_str() );
		if ( !animator.HasAnim ( painAnim ) ) {
			painAnim.Clear ( );
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


// RAVEN BEGIN
// bdube: added for vehicle
/*
==============
idActor::ExitVehicle
==============
*/
bool idActor::ExitVehicle ( bool force ) {
	idMat3	axis;			
	idVec3	origin;

	if ( !IsInVehicle ( ) ) {
		return false;
	}

	if ( vehicleController.GetVehicle()->IsLocked() ) {
		if ( force ) {
			vehicleController.GetVehicle()->Unlock();
		} else {
			return false;
		}
	}

	if( !vehicleController.FindClearExitPoint(origin, axis) ) {
		if ( force ) {
			origin = GetPhysics()->GetOrigin() + idVec3( spawnArgs.GetVector( "forced_exit_offset", "-100 0 0" ) );
			axis = GetPhysics()->GetAxis();
		} else {
			return false;
		}
	}

	vehicleController.Eject ( force );

	GetPhysics()->SetOrigin( origin );
	viewAxis = axis[0].ToMat3();
	GetPhysics()->SetAxis( mat3_identity );
	GetPhysics()->SetLinearVelocity( vec3_origin );
	
	return true;
}

/*
=====================
idActor::EnterVehicle
=====================
*/
bool idActor::EnterVehicle ( idEntity* ent ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( IsInVehicle ( ) || !ent->IsType ( rvVehicle::GetClassType() ) ) {
// RAVEN END
		return false ;
	}	

	// Get in the vehicle	
	if ( !vehicleController.Drive ( static_cast<rvVehicle*>(ent), this ) ) {
		return false;
	}
	
	return true;
}

// RAVEN END


/***********************************************************************

	Events

***********************************************************************/

/*
=====================
idActor::FootStep
=====================
*/
void idActor::FootStep( void ) {
	const char*				sound;
	const rvDeclMatType*	materialType;

	if ( !GetPhysics()->HasGroundContacts() ) {
		return;
	}

	// start footstep sound based on material type
	materialType = GetPhysics()->GetContact( 0 ).materialType;
	sound		 = NULL;
	
	// Sound based on material type?
	if ( materialType ) {
		sound = spawnArgs.GetString( va( "snd_footstep_%s", materialType->GetName() ) );
	}
	if ( !sound || !*sound ) {
		sound = spawnArgs.GetString( "snd_footstep" );
	}
	
	// If we have a sound then play it
	if ( sound && *sound ) {
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
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
=====================
idActor::Event_EnableBlink
=====================
*/
void idActor::Event_EnableBlink( void ) {
	blink_time = gameLocal.time + blink_min + gameLocal.random.RandomFloat() * ( blink_max - blink_min );
}

/*
=====================
idActor::Event_DisableBlink
=====================
*/
void idActor::Event_DisableBlink( void ) {
	blink_time = 0x7FFFFFFF;
}

/*
===============
idActor::Event_Footstep
===============
*/
void idActor::Event_Footstep( void ) {
	FootStep ( );
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
// RAVEN BEGIN
// bdube: reversed var
	disablePain = true;
// RAVEN END
}

/*
===============
idActor::Event_EnablePain
===============
*/
void idActor::Event_EnablePain( void ) {
// RAVEN BEGIN
// bdube: reversed var
	disablePain = false;
// RAVEN END
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
	idThread::ReturnFloat( MS2SEC(PlayAnim(channel, animname, -1)) );
}

/*
===============
idActor::Event_PlayCycle
===============
*/
void idActor::Event_PlayCycle( int channel, const char *animname ) {
	PlayCycle ( channel, animname, -1 );
	idThread::ReturnInt( true );
}

/*
=====================
idAI::DebugFilter
=====================
*/
bool idActor::DebugFilter ( const idCVar& test )  const {
	return ( health>0 && (test.GetBool() || test.GetInteger()>0) && (!ai_debugFilterString.GetString()[0] || !stricmp( name.c_str(), ai_debugFilterString.GetString() )));
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
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
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
			animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
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
			headEnt->GetAnimator()->CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
		}
		break;

	case ANIMCHANNEL_LEGS :
		animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
		if ( torsoAnim.IsIdle() ) {
			animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( anim, weight );
			if ( headEnt && headAnim.IsIdle() ) {
				headEnt->GetAnimator()->CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( anim, weight );
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
idActor::Event_GetHead
=====================
*/
void idActor::Event_GetHead( void ) {
	idThread::ReturnEntity( head.GetEntity() );
}

// RAVEN BEGIN
// jshepard: added
/*
=====================
idActor::Event_SetAnimRate
=====================
*/
void idActor::Event_SetAnimRate( float multiplier ) {
	animator.SetPlaybackRate(multiplier);
}


/*
===============================================================================

	Wait States 

===============================================================================
*/

/*
================
idActor::State_Wait_Frame

Stop a state thread for a single frame
================
*/
stateResult_t idActor::State_Wait_Frame ( const stateParms_t& parms ) {
	return SRESULT_DONE_WAIT;
}

/*
================
idActor::State_Wait_LegsAnim

Stop a state thread until the animation running on the legs channel is finished
================
*/
stateResult_t idActor::State_Wait_LegsAnim ( const stateParms_t& parms ) {
	if ( !AnimDone ( ANIMCHANNEL_LEGS, parms.blendFrames ) ) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idActor::State_Wait_TorsoAnim

Stop a state thread until the animation running on the torso channel is finished
================
*/
stateResult_t idActor::State_Wait_TorsoAnim ( const stateParms_t& parms ) {
	if ( !AnimDone ( ANIMCHANNEL_TORSO, parms.blendFrames ) ) {
		return SRESULT_WAIT;
	}
	return SRESULT_DONE;
}

/*
================
idActor::PlayAnim
================
*/
int idActor::PlayAnim ( int channel, const char *animname, int blendFrames ) {
	animFlags_t	flags;
	idEntity *headEnt;
	int	anim;

	if ( blendFrames != -1 ) {
		Event_SetBlendFrames ( channel, blendFrames );
	}
	
	anim = GetAnim( channel, animname );

	if( ai_animShow.GetBool() ){
		gameLocal.DPrintf( "Playing animation '%s' on '%s' (%s)\n", animname, name.c_str(), spawnArgs.GetString( "head", "" ) );
	}

	if ( !anim ) {
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		}
		return 0;
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
	
	return animator.CurrentAnim( channel )->Length();
}

/*
================
idActor::PlayCycle
================
*/
bool idActor::PlayCycle ( int channel, const char *animname, int blendFrames ) {
	animFlags_t	flags;
	int			anim;

	if ( blendFrames != -1 ) {
		Event_SetBlendFrames ( channel, blendFrames );
	}
	
	anim = GetAnim( channel, animname );
	if ( !anim ) {
		if ( ( channel == ANIMCHANNEL_HEAD ) && head.GetEntity() ) {
			gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), spawnArgs.GetString( "def_head", "" ) );
		} else {
			gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		}
		return false;
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
	
	return true;
}

void idActor::IdleAnim ( int channel, const char *name, int blendFrames ) {
	Event_SetBlendFrames ( channel, blendFrames );
	Event_IdleAnim ( channel, name );
}

void idActor::OverrideAnim ( int channel ) {
	Event_OverrideAnim ( channel );
}

idAnimState& idActor::GetAnimState ( int channel ) {
	switch ( channel ) {
		case ANIMCHANNEL_LEGS:		return legsAnim;
		case ANIMCHANNEL_TORSO:		return torsoAnim;
		case ANIMCHANNEL_HEAD:		return headAnim;
		default:
			gameLocal.Error( "idActor::GetAnimState: Unknown anim channel" );
			return torsoAnim;
	}
}

void idActor::DisableAnimState ( int channel ) {
	Event_OverrideAnim ( channel );
//	GetAnimState ( channel ).Disable ( );
}

void idActor::EnableAnimState ( int channel ) {
	GetAnimState ( channel ).Enable ( 4 );
}

bool idActor::HasAnim ( int channel, const char* animname, bool forcePrefix ) {
	return GetAnim( channel, animname, forcePrefix ) != NULL;
}

bool idActor::AnimDone ( int channel, int blendFrames ) {
	return GetAnimState( channel ).AnimDone ( blendFrames );
}

/*
=====================
idActor::GetDebugInfo
=====================
*/
void idActor::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	// Base class first
	idAFEntity_Gibbable::GetDebugInfo ( proc, userData );

	proc ( "idActor", "state",			stateThread.GetState()?stateThread.GetState()->state->name : "<none>", userData );

	proc ( "idActor", "legs_state",		legsAnim.GetStateThread().GetState()?legsAnim.GetStateThread().GetState()->state->name:"<none>", userData );
	proc ( "idActor", "legs_disable",	legsAnim.Disabled()?"true":"false", userData );
	proc ( "idActor", "legs_anim",		GetAnimator()->CurrentAnim ( ANIMCHANNEL_LEGS ) ? GetAnimator()->CurrentAnim ( ANIMCHANNEL_LEGS )->AnimName ( ) : "<none>", userData );

	proc ( "idActor", "torso_state",	torsoAnim.GetStateThread().GetState()?torsoAnim.GetStateThread().GetState()->state->name:"<none>", userData );
	proc ( "idActor", "torso_disabled",	torsoAnim.Disabled()?"true":"false", userData );
	proc ( "idActor", "torso_anim",		GetAnimator()->CurrentAnim ( ANIMCHANNEL_TORSO ) ? GetAnimator()->CurrentAnim ( ANIMCHANNEL_TORSO )->AnimName ( ) : "<none>", userData );

	proc ( "idActor", "head_state",		headAnim.GetStateThread().GetState()?headAnim.GetStateThread().GetState()->state->name:"<none>", userData );
	proc ( "idActor", "head_disabled",	headAnim.Disabled()?"true":"false", userData );
	proc ( "idActor", "head_anim",		GetAnimator()->CurrentAnim ( ANIMCHANNEL_HEAD ) ? GetAnimator()->CurrentAnim ( ANIMCHANNEL_HEAD )->AnimName ( ) : "<none>", userData );	

	proc ( "idActor", "painAnim",		painAnim.c_str(), userData );
	proc ( "idActor", "animPrefix",		animPrefix.c_str(), userData );
}

//MCG: damage over time
void idActor::Event_DamageOverTime ( int endTime, int interval, idEntity *inflictor, idEntity *attacker, idVec3 &dir,
					   const char *damageDefName, const float damageScale, int location ) {
	const idDeclEntityDef* damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if ( damageDef ) {
		inDamageEvent = true;
		Damage( inflictor, attacker, dir, damageDefName, damageScale, location );
		inDamageEvent = false;
		if ( endTime == -1 || gameLocal.GetTime() + interval <= endTime ) {
			//post it again
			PostEventMS( &EV_DamageOverTime, interval, endTime, interval, inflictor, attacker, dir, damageDefName, damageScale, location );
		}
	}
}

void idActor::Event_DamageOverTimeEffect ( int endTime, int interval, const char *damageDefName ) {
	const idDeclEntityDef* damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if ( damageDef ) {
		rvClientCrawlEffect* effect;
		effect = new rvClientCrawlEffect ( gameLocal.GetEffect ( damageDef->dict, "fx_dot" ), this, interval );
		effect->Play ( gameLocal.time, false );
		if ( endTime == -1 || gameLocal.GetTime() + interval <= endTime ) {
			//post it again
			PostEventMS( &EV_DamageOverTimeEffect, interval, endTime, interval, damageDefName );
		}
	}
}

// MCG: script-callable joint crawl effect
void idActor::Event_JointCrawlEffect ( const char *effectKeyName, float crawlSecs ) {
	if ( effectKeyName ) {
		rvClientCrawlEffect* effect;
		effect = new rvClientCrawlEffect( gameLocal.GetEffect ( spawnArgs, effectKeyName ), this, 100 );
		effect->Play ( gameLocal.GetTime(), false );
		crawlSecs -= 0.1f;
		if ( crawlSecs >= 0.1f ) {
			PostEventMS( &EV_JointCrawlEffect, 100, effectKeyName, crawlSecs );
		}
	}
}

idEntity* idActor::GetGroundElevator( idEntity* testElevator ) const {
	idEntity* groundEnt = GetGroundEntity();
	if ( !groundEnt ) {
		return NULL;
	}
	while ( groundEnt->GetBindMaster() ) {
		groundEnt = groundEnt->GetBindMaster();
	}

	if ( !groundEnt->IsType( idElevator::GetClassType() ) ) {
		return NULL;
	}

	if ( testElevator && groundEnt != testElevator ) {
		return groundEnt;
	}

	idEntity* traceEnt;
	idVec3 testPoint = GetPhysics()->GetOrigin();
	idVec3 testBottom;
	testPoint.z += 1.0f;

	for ( int x = 0; x < 2; x++ ) {
		testPoint.x = GetPhysics()->GetAbsBounds()[x].x;
		for ( int y = 0; y < 2; y++ ) {
			testPoint.y = GetPhysics()->GetAbsBounds()[y].y;
			testBottom = testPoint;
			testBottom.z -= 65.0f;

			trace_t tr;
			gameLocal.TracePoint( this, tr, testPoint, testBottom, GetPhysics()->GetClipMask(), this );
			traceEnt = gameLocal.FindEntity( tr.c.entityNum );
			if ( !traceEnt ) {
				return NULL;
			}
			while ( traceEnt->GetBindMaster() ) {
				traceEnt = traceEnt->GetBindMaster();
			}
			if ( traceEnt != groundEnt ) {
				return traceEnt;
			}
			if ( testElevator && traceEnt != testElevator ) {
				return traceEnt;
			}
		}
	}

	return groundEnt;
}

void idActor::GuidedProjectileIncoming( idGuidedProjectile *projectile )
{
	if ( IsInVehicle() )
	{
		if ( vehicleController.GetVehicle() )
		{
			vehicleController.GetVehicle()->GuidedProjectileIncoming( projectile );
		}
	}
}
// RAVEN END
