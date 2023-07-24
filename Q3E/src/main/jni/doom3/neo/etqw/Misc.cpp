// Copyright (C) 2007 Id Software, Inc.
//
/*

Various utility objects and functions.

*/

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Misc.h"
#include "Player.h"
#include "Camera.h"
//#include "ai/AI.h"
#include "Projectile.h"
#include "WorldSpawn.h"
#include "ContentMask.h"
#include "Moveable.h"
#include "../decllib/declTypeHolder.h"
#include "../decllib/declImposter.h"

/*
===============================================================================

idSpawnableEntity

A simple, spawnable entity with a model and no functionable ability of it's own.
For example, it can be used as a placeholder during development, for marking
locations on maps for script, or for simple placed models without any behavior
that can be bound to other entities.  Should not be subclassed.
===============================================================================
*/

CLASS_DECLARATION( idEntity, idSpawnableEntity )
END_CLASS

/*
======================
idSpawnableEntity::Spawn
======================
*/
void idSpawnableEntity::Spawn() {
	// this just holds dict information
}

/*
===============================================================================

sdModelStatic

A simple, spawnable entity with a collision model and no functionable ability of its own.

	NOTE: this entity is really a hack, and can go as soon as idEntity is cleaned up
	( that is, when it has no renderEntity anymore )

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdModelStatic )
END_CLASS

/*
======================
sdModelStatic::Spawn
======================
*/
void sdModelStatic::Spawn( void ) {
	Hide();
	memset( &renderEntity, 0, sizeof( renderEntity ) );
}

/*
===============
sdModelStatic::PostMapSpawn
===============
*/
void sdModelStatic::PostMapSpawn( void ) {
	idEntity::PostMapSpawn();

	sdInstanceCollector< sdLODEntity > lodEntity( false );
	if ( lodEntity.Num() < 1 ) {
		return;
	}
	sdLODEntity* lodEnt = lodEntity[ 0 ];

	lodEnt->AddClipModel( new idClipModel( GetPhysics()->GetClipModel() ), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	PostEventMS( &EV_Remove, 0 );
}

/*
===============
sdModelStatic::InhibitSpawn
================
*/
bool sdModelStatic::InhibitSpawn( const idDict& args ) {
	if ( args.GetBool( "noClipModel" ) ) {
		return true;
	}

	if ( args.GetBool( "mergecm" ) ) {
		assert( 0 );	// jrad - the map compiler isn't doing its job if we ever get here
		return true;
	}

	if ( args.GetBool( "inlineCollisionModel" ) ) {
		assert( 0 );	// jrad - the map compiler isn't doing its job if we ever get here
		return true;
	}

	return false;
}

/*
===============================================================================

	sdDynamicSpawnPoint

===============================================================================
*/

CLASS_DECLARATION( sdScriptEntity, sdDynamicSpawnPoint )
END_CLASS

/*
===============
sdDynamicSpawnPoint::sdDynamicSpawnPoint
================
*/
sdDynamicSpawnPoint::sdDynamicSpawnPoint( void ) : spawnPoint( NULL ) {
}

/*
===============
sdDynamicSpawnPoint::~sdDynamicSpawnPoint
================
*/
sdDynamicSpawnPoint::~sdDynamicSpawnPoint( void ) {
	gameLocal.UnRegisterSpawnPoint( spawnPoint );
}

/*
===============
sdDynamicSpawnPoint::Spawn
================
*/
void sdDynamicSpawnPoint::Spawn( void ) {
	spawnPoint = &gameLocal.RegisterSpawnPoint( this, vec3_origin, ang_zero );
	spawnPoint->GetRequirements().Load( spawnArgs, "require" );
}

/*
==============
sdDynamicSpawnPoint::CanCollide
==============
*/
bool sdDynamicSpawnPoint::CanCollide( const idEntity* other, int traceId ) const {
	if ( traceId == TM_THIRDPERSON_OFFSET ) {
		return false;
	}
	return idEntity::CanCollide( other, traceId );
}

/*
===============================================================================

	idPlayerStart

===============================================================================
*/

CLASS_DECLARATION( idEntity, idPlayerStart )
END_CLASS

/*
===============
idPlayerStart::idPlayerStart
================
*/
idPlayerStart::idPlayerStart( void ) {
}

/*
===============
idPlayerStart::InhibitSpawn
================
*/
bool idPlayerStart::InhibitSpawn( const idDict& args ) {
	return gameLocal.isClient && !gameLocal.serverIsRepeater;
}

/*
===============
idPlayerStart::PostMapSpawn
================
*/
void idPlayerStart::PostMapSpawn( void ) {
	sdSpawnPoint* spot;

	idAngles angles;

	const char* targetName = spawnArgs.GetString( "target" );
	idEntity* target = gameLocal.FindEntity( targetName );

	const char* ownerName = spawnArgs.GetString( "owner" );

	bool parachute = spawnArgs.GetBool( "parachute" );
	idVec3 origin = GetPhysics()->GetOrigin();
	if ( parachute ) {
		origin.z += spawnArgs.GetFloat( "parachute_height", "2048" );
	}

	if ( *ownerName ) {
		idEntity* owner = gameLocal.FindEntity( ownerName );
		if ( !owner ) {
			gameLocal.Error( "idPlayerStart::PostMapSpawn Could not find owner '%s'", ownerName );
		}

		idVec3 org = ( origin - owner->GetPhysics()->GetOrigin() ) * owner->GetPhysics()->GetAxis().Transpose();						
		if ( target ) {
			idVec3 vec = target->GetPhysics()->GetOrigin() - org;
			vec.Normalize();
			angles = vec.ToMat3().ToAngles();
		} else {
			angles = GetPhysics()->GetAxis().ToAngles();
		}
		spot = &gameLocal.RegisterSpawnPoint( owner, org, angles );

	} else {
		if( target ) {
			idVec3 vec = target->GetPhysics()->GetOrigin() - origin;
			vec.Normalize();
			angles = vec.ToMat3().ToAngles();
		} else {
			angles = GetPhysics()->GetAxis().ToAngles();
		}
		spot = &gameLocal.RegisterSpawnPoint( NULL, origin, angles );
	}

	spot->GetRequirements().Load( spawnArgs, "require" );
	spot->SetParachute( parachute );

	PostEventMS( &EV_Remove, 0 );
}

/*
===============================================================================

  idForceField
	
===============================================================================
*/

const idEventDef EV_Toggle( "toggle", '\0', DOC_TEXT( "Toggles the state of the force field." ), 0, "Calling $event:activate$ on a force field will toggle the state, then reset it after a set wait period, using $event:toggle$ will not reset." );

CLASS_DECLARATION( idEntity, idForceField )
	EVENT( EV_Activate,		idForceField::Event_Activate )
	EVENT( EV_Toggle,		idForceField::Event_Toggle )
	EVENT( EV_FindTargets,	idForceField::Event_FindTargets )
	EVENT( EV_GetMins,		idForceField::Event_GetMins )
	EVENT( EV_GetMaxs,		idForceField::Event_GetMaxs )
END_CLASS

idCVar g_debugForceFields( "g_debugForceFields", "0", CVAR_GAME | CVAR_BOOL, "" );

/*
===============
idForceField::Toggle
================
*/
void idForceField::Toggle( void ) {
	active = !active;
}

/*
================
idForceField::Think
================
*/
void idForceField::Think( void ) {
	if ( active ) {
		// evaluate force
		forceField.Evaluate( gameLocal.time );
	}
	Present();

	if ( g_debugForceFields.GetBool() ) {
		gameRenderWorld->DebugBounds( colorBlue, forceField.GetClipModel()->GetBounds(), forceField.GetClipModel()->GetOrigin() );
	}
}

/*
================
idForceField::Spawn
================
*/
void idForceField::Spawn( void ) {
	idVec3 uniform;
	float explosion, implosion, randomTorque;

	if ( spawnArgs.GetVector( "uniform", "0 0 0", uniform ) ) {
		forceField.Uniform( uniform );
	} else if ( spawnArgs.GetFloat( "explosion", "0", explosion ) ) {
		forceField.Explosion( explosion );
	} else if ( spawnArgs.GetFloat( "implosion", "0", implosion ) ) {
		forceField.Implosion( implosion );
	}

	if ( spawnArgs.GetFloat( "randomTorque", "0", randomTorque ) ) {
		forceField.RandomTorque( randomTorque );
	}

	if ( spawnArgs.GetBool( "applyForce", "0" ) ) {
		forceField.SetApplyType( FORCEFIELD_APPLY_FORCE );
	} else if ( spawnArgs.GetBool( "applyImpulse", "0" ) ) {
		forceField.SetApplyType( FORCEFIELD_APPLY_IMPULSE );
	} else {
		forceField.SetApplyType( FORCEFIELD_APPLY_VELOCITY );
	}

	forceField.SetPlayerOnly( spawnArgs.GetBool( "playerOnly", "0" ) );
	forceField.SetMonsterOnly( spawnArgs.GetBool( "monsterOnly", "0" ) );

	// set the collision model on the force field
	forceField.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ) );

	// remove the collision model from the physics object
	GetPhysics()->SetClipModel( NULL, 1.0f );

	active = spawnArgs.GetBool( "start_on" );

	PostEventMS( &EV_FindTargets, 0 );

	BecomeActive( TH_THINK );
}

/*
===============
idForceField::Event_Toggle
================
*/
void idForceField::Event_Toggle( void ) {
	Toggle();
}

/*
================
idForceField::Event_Activate
================
*/
void idForceField::Event_Activate( idEntity *activator ) {
	float wait;

	Toggle();
	if ( spawnArgs.GetFloat( "wait", "0.01", wait ) ) {
		PostEventSec( &EV_Toggle, wait );
	}
}

/*
================
idForceField::Event_FindTargets
================
*/
void idForceField::Event_FindTargets( void ) {
	FindTargets();
	RemoveNullTargets();
	if ( targets.Num() ) {
		forceField.Uniform( targets[0].GetEntity()->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin() );
	}
}

/*
================
idForceField::Event_GetMins
================
*/
void idForceField::Event_GetMins( void ) {
	sdProgram::ReturnVector( forceField.GetClipModel()->GetBounds()[ 0 ] );
}

/*
================
idForceField::Event_GetMaxs
================
*/
void idForceField::Event_GetMaxs( void ) {
	sdProgram::ReturnVector( forceField.GetClipModel()->GetBounds()[ 1 ] );
}


/*
===============================================================================

	idAnimated

===============================================================================
*/

const idEventDefInternal EV_Animated_Start( "internal_start" );
const idEventDefInternal EV_AnimDone( "internal_animDone", "d" );
const idEventDef EV_StartRagdoll( "startRagdoll", '\0', DOC_TEXT( "Switches the entity into ragdoll mode from its current animation pose." ), 0, "This will do nothing if the entity does not have an $decl:articulatedFigure$ set up." );

CLASS_DECLARATION( idAFEntity_Base, idAnimated )
	EVENT( EV_Activate,				idAnimated::Event_Activate )
	EVENT( EV_Animated_Start,		idAnimated::Event_Start )
	EVENT( EV_StartRagdoll,			idAnimated::Event_StartRagdoll )
	EVENT( EV_AnimDone,				idAnimated::Event_AnimDone )
END_CLASS

/*
===============
idAnimated::idAnimated
================
*/
idAnimated::idAnimated() {
	anim = 0;
	blendFrames = 0;
	soundJoint = INVALID_JOINT;
	activated = false;
	combatModel = NULL;
	activator = NULL;
	current_anim_index = 0;
	num_anims = 0;

}

/*
===============
idAnimated::idAnimated
================
*/
idAnimated::~idAnimated() {
	gameLocal.clip.DeleteClipModel( combatModel );
	combatModel = NULL;
}

/*
===============
idAnimated::Spawn
================
*/
void idAnimated::Spawn( void ) {
	idStr		animname;
	int			anim2;
	float		wait;
	const char	*joint;

	joint = spawnArgs.GetString( "sound_bone", "origin" ); 
	soundJoint = animator.GetJointHandle( joint );
	if ( soundJoint == INVALID_JOINT ) {
		gameLocal.Warning( "idAnimated '%s' at (%s): cannot find joint '%s' for sound playback", name.c_str(), GetPhysics()->GetOrigin().ToString(0), joint );
	}

	LoadAF();

	// allow bullets to collide with a combat model
	if ( spawnArgs.GetBool( "combatModel", "0" ) ) {
		combatModel = new idClipModel( modelDefHandle );
	}

	// allow the entity to take damage
	if ( spawnArgs.GetBool( "takeDamage", "0" ) ) {
		fl.takedamage = true;
	}

	blendFrames = 0;

	current_anim_index = 0;
	spawnArgs.GetInt( "num_anims", "0", num_anims );

	blendFrames = spawnArgs.GetInt( "blend_in" );

	animname = spawnArgs.GetString( num_anims ? "anim1" : "anim" );
	if ( !animname.Length() ) {
		anim = 0;
	} else {
		anim = animator.GetAnim( animname );
		if ( !anim ) {
			gameLocal.Error( "idAnimated '%s' at (%s): cannot find anim '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), animname.c_str() );
		}
	}

	if ( spawnArgs.GetBool( "hide" ) ) {
		Hide();

		if ( !num_anims ) {
			blendFrames = 0;
		}
	} else if ( spawnArgs.GetString( "start_anim", "", animname ) ) {
		anim2 = animator.GetAnim( animname );
		if ( !anim2 ) {
			gameLocal.Error( "idAnimated '%s' at (%s): cannot find anim '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), animname.c_str() );
		}
		animator.CycleAnim( ANIMCHANNEL_ALL, anim2, gameLocal.time, 0 );
	} else if ( anim ) {
		// init joints to the first frame of the animation
		animator.SetFrame( ANIMCHANNEL_ALL, anim, 1, gameLocal.time, 0 );		

		if ( !num_anims ) {
			blendFrames = 0;
		}
	}

	spawnArgs.GetFloat( "wait", "-1", wait );

	if ( wait >= 0 ) {
		PostEventSec( &EV_Activate, wait, this );
	}
}

/*
===============
idAnimated::LoadAF
===============
*/
bool idAnimated::LoadAF( void ) {
	idStr fileName;

	if ( !spawnArgs.GetString( "ragdoll", "*unknown*", fileName ) ) {
		return false;
	}
	af.SetAnimator( GetAnimator() );
	return af.Load( this, fileName );
}

/*
===============
idAnimated::GetPhysicsToSoundTransform
===============
*/
bool idAnimated::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	animator.GetJointTransform( soundJoint, gameLocal.time, origin, axis );
	axis = renderEntity.axis;
	return true;
}

/*
================
idAnimated::StartRagdoll
================
*/
bool idAnimated::StartRagdoll( void ) {
	// if no AF loaded
	if ( !af.IsLoaded() ) {
		return false;
	}

	// if the AF is already active
	if ( af.IsActive() ) {
		return true;
	}

	// disable any collision model used
	GetPhysics()->UnlinkClip();

	// start using the AF
	af.StartFromCurrentPose( spawnArgs.GetInt( "velocityTime", "0" ) );
	
	return true;
}

/*
=====================
idAnimated::PlayNextAnim
=====================
*/
void idAnimated::PlayNextAnim( void ) {
	const char *animname;
	int len;
	int cycle;

	if ( current_anim_index >= num_anims ) {
		Hide();
		if ( spawnArgs.GetBool( "remove" ) ) {
			PostEventMS( &EV_Remove, 0 );
		} else {
			current_anim_index = 0;
		}
		return;
	}

	Show();
	current_anim_index++;

	spawnArgs.GetString( va( "anim%d", current_anim_index ), NULL, &animname );
	if ( !animname ) {
		anim = 0;
		animator.Clear( ANIMCHANNEL_ALL, gameLocal.time, FRAME2MS( blendFrames ) );
		return;
	}

	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing anim '%s' on %s", animname, name.c_str() );
		return;
	}

	if ( g_debugCinematic.GetBool() ) {
		gameLocal.Printf( "%d: '%s' start anim '%s'\n", gameLocal.framenum, GetName(), animname );
	}
		
	spawnArgs.GetInt( "cycle", "1", cycle );
	if ( ( current_anim_index == num_anims ) && spawnArgs.GetBool( "loop_last_anim" ) ) {
		cycle = -1;
	}

	animator.CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( blendFrames ) );
	animator.CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( cycle );

	len = animator.CurrentAnim( ANIMCHANNEL_ALL )->PlayLength();
	if ( len >= 0 ) {
		PostEventMS( &EV_AnimDone, len, current_anim_index );
	}

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	animator.ForceUpdate();
	UpdateAnimation();
	UpdateVisuals();
	Present();
}

/*
===============
idAnimated::Event_StartRagdoll
================
*/
void idAnimated::Event_StartRagdoll( void ) {
	StartRagdoll();
}

/*
===============
idAnimated::Event_AnimDone
================
*/
void idAnimated::Event_AnimDone( int animindex ) {
	if ( g_debugCinematic.GetBool() ) {
		const idAnim *animPtr = animator.GetAnim( anim );
		gameLocal.Printf( "%d: '%s' end anim '%s'\n", gameLocal.framenum, GetName(), animPtr ? animPtr->Name() : "" );
	}

	if ( ( animindex >= num_anims ) && spawnArgs.GetBool( "remove" ) ) {
		Hide();
		PostEventMS( &EV_Remove, 0 );
	} else if ( spawnArgs.GetBool( "auto_advance" ) ) {
		PlayNextAnim();
	} else {
		activated = false;
	}
}

/*
===============
idAnimated::Event_Activate
================
*/
void idAnimated::Event_Activate( idEntity *_activator ) {
	if ( num_anims ) {
		PlayNextAnim();
		activator = _activator;
		return;
	}

	if ( activated ) {
		// already activated
		return;
	}

	activated = true;
	activator = _activator;
	ProcessEvent( &EV_Animated_Start );
}

/*
===============
idAnimated::Event_Start
================
*/
void idAnimated::Event_Start( void ) {
	int cycle;
	int len;

	Show();

	if ( num_anims ) {
		PlayNextAnim();
		return;
	}

	if ( anim ) {
		if ( g_debugCinematic.GetBool() ) {
			const idAnim *animPtr = animator.GetAnim( anim );
			gameLocal.Printf( "%d: '%s' start anim '%s'\n", gameLocal.framenum, GetName(), animPtr ? animPtr->Name() : "" );
		}
		spawnArgs.GetInt( "cycle", "1", cycle );
		animator.CycleAnim( ANIMCHANNEL_ALL, anim, gameLocal.time, FRAME2MS( blendFrames ) );
		animator.CurrentAnim( ANIMCHANNEL_ALL )->SetCycleCount( cycle );

		len = animator.CurrentAnim( ANIMCHANNEL_ALL )->PlayLength();
		if ( len >= 0 ) {
			PostEventMS( &EV_AnimDone, len, 1 );
		}
	}

	// offset the start time of the shader to sync it to the game time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	animator.ForceUpdate();
	UpdateAnimation();
	UpdateVisuals();
	Present();
}

/*
===============
sdStaticEntityNetworkData::~sdStaticEntityNetworkData
===============
*/
sdStaticEntityNetworkData::~sdStaticEntityNetworkData( void ) {
	delete physicsData;
}

/*
===============
sdStaticEntityNetworkData::MakeDefault
===============
*/
void sdStaticEntityNetworkData::MakeDefault( void ) {
	if ( physicsData != NULL ) {
		physicsData->MakeDefault();
	}
}

/*
===============
sdStaticEntityNetworkData::Write
===============
*/
void sdStaticEntityNetworkData::Write( idFile* file ) const {
	if ( physicsData != NULL ) {
		physicsData->Write( file );
	}
}

/*
===============
sdStaticEntityNetworkData::Read
===============
*/
void sdStaticEntityNetworkData::Read( idFile* file ) {
	if ( physicsData != NULL ) {
		physicsData->Read( file );
	}
}

/*
===============
sdStaticEntityNetworkData::MakeDefault
===============
*/
void sdStaticEntityBroadcastData::MakeDefault( void ) {
	if ( physicsData != NULL ) {
		physicsData->MakeDefault();
	}
	hidden = -1;
	forceDisableClip = -1;
}

/*
===============
sdStaticEntityBroadcastData::~sdStaticEntityBroadcastData
===============
*/
sdStaticEntityBroadcastData::~sdStaticEntityBroadcastData( void ) {
	delete physicsData;
}

/*
===============
sdStaticEntityBroadcastData::Write
===============
*/
void sdStaticEntityBroadcastData::Write( idFile* file ) const {
	if ( physicsData != NULL ) {
		physicsData->Write( file );
	}
	
	file->WriteBool( hidden > 0 );
	file->WriteBool( forceDisableClip > 0 );
}

/*
===============
sdStaticEntityBroadcastData::Read
===============
*/
void sdStaticEntityBroadcastData::Read( idFile* file ) {
	if ( physicsData != NULL ) {
		physicsData->Read( file );
	}
	
	bool temp;
	file->ReadBool( temp );
	hidden = temp ? 1 : 0;
	file->ReadBool( temp );
	forceDisableClip = temp ? 1 : 0;
}

/*
===============================================================================

	idStaticEntity

	Some static entities may be optimized into inline geometry by dmap

===============================================================================
*/

CLASS_DECLARATION( idEntity, idStaticEntity )
END_CLASS

/*
===============
idStaticEntity::idStaticEntity
===============
*/
idStaticEntity::idStaticEntity( void ) {
}

/*
================
idStaticEntity::~idStaticEntity
================
*/
idStaticEntity::~idStaticEntity( void ) {
	delete []renderEntity.areas;
}

/*
===============
idStaticEntity::Spawn
===============
*/
void idStaticEntity::Spawn( void ) {
	bool solid	= spawnArgs.GetBool( "solid" );
	bool hidden = spawnArgs.GetBool( "hide" );
	bool disableClip = spawnArgs.GetBool( "disableClip" );
	
	const char *areas;
	if ( spawnArgs.GetString( "areas", "", &areas ) ) {
		idStrList areaList;
		idSplitStringIntoList( areaList, areas, " " );
		if ( areaList.Num() ) {
			renderEntity.numAreas = areaList.Num();
			renderEntity.areas = new int[ areaList.Num() ];
			for (int i=0; i<areaList.Num(); i++ ) {
				renderEntity.areas[i] = atoi( areaList[i].c_str() );
			}
		}
	}

	if ( solid ) {
		GetPhysics()->SetContents( CONTENTS_SOLID );
	} else {
		GetPhysics()->SetContents( 0 );
	}

	if ( hidden ) {
		Hide();
	}
	if ( disableClip ) {
		ForceDisableClip();
	}
}

/*
===============
idStaticEntity::PostMapSpawn
===============
*/
void idStaticEntity::PostMapSpawn( void ) {
	idEntity::PostMapSpawn();

	if ( !IsNetSynced() && !spawnArgs.GetBool( "dynamic" ) ) {
		sdInstanceCollector< sdLODEntity > lodEntity( false );
		if ( lodEntity.Num() < 1 ) {
			return;
		}
		sdLODEntity* lodEnt = lodEntity[ 0 ];

		if ( GetPhysics()->GetNumClipModels() > 0 ) {
			lodEnt->AddClipModel( new idClipModel( GetPhysics()->GetClipModel() ), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
		}

		if ( !IsHidden() && renderEntity.hModel != NULL ) {
			lodEnt->AddRenderEntity( renderEntity, -1 );
		}

		PostEventMS( &EV_Remove, 0 );
	}
}

/*
================
idStaticEntity::InhibitSpawn
================
*/
bool idStaticEntity::InhibitSpawn( const idDict& args ) {
	assert( gameLocal.world != NULL );

	// an inline static model will not do anything at all
	if ( args.GetBool( "inline" ) || gameLocal.world->spawnArgs.GetBool( "inlineAllStatics" ) ) {
		return true;
	}

	return false;
}

/*
================
idStaticEntity::Think
================
*/
void idStaticEntity::Think( void ) {
	idEntity::Think();
}

/*
================
idStaticEntity::Hide
================
*/
void idStaticEntity::Hide( void ) {
	idEntity::Hide();
	fl.forceDisableClip = true;
	DisableClip();
}

/*
================
idStaticEntity::Show
================
*/
void idStaticEntity::Show( void ) {
	idEntity::Show();
	fl.forceDisableClip = false;
	EnableClip();
}

/*
================
idStaticEntity::ApplyNetworkState
================
*/
void idStaticEntity::ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_NEW( sdStaticEntityNetworkData );
		NET_APPLY_STATE_PHYSICS;
	} else if ( mode == NSM_BROADCAST ) {
		NET_GET_NEW( sdStaticEntityBroadcastData );
		NET_APPLY_STATE_PHYSICS;

		if ( newData.hidden > 0 ) {
			Hide();
		} else {
			Show();
		}

		if ( newData.forceDisableClip > 0 ) {
			ForceDisableClip();
		} else {
			ForceEnableClip();
		}
	}
}

/*
================
idStaticEntity::ReadNetworkState
================
*/
void idStaticEntity::ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdStaticEntityNetworkData );
		NET_READ_STATE_PHYSICS

		return;
	}
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdStaticEntityBroadcastData );
		NET_READ_STATE_PHYSICS;

		newData.hidden = msg.ReadBool() ? 1 : 0;
		newData.forceDisableClip = msg.ReadBool() ? 1 : 0;
		return;
	}
	return;
}

/*
================
idStaticEntity::WriteNetworkState
================
*/
void idStaticEntity::WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_STATES( sdStaticEntityNetworkData );
		NET_WRITE_STATE_PHYSICS

		return;
	}
	if ( mode == NSM_BROADCAST ) {
		NET_GET_STATES( sdStaticEntityBroadcastData );
		NET_WRITE_STATE_PHYSICS;

		newData.hidden = fl.hidden ? 1 : 0;
		newData.forceDisableClip = fl.forceDisableClip ? 1 : 0;

		msg.WriteBool( fl.hidden );
		msg.WriteBool( fl.forceDisableClip );

		return;
	}
	return;
}

/*
================
idStaticEntity::CheckNetworkStateChanges
================
*/
bool idStaticEntity::CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const {
	if ( mode == NSM_VISIBLE ) {
		NET_GET_BASE( sdStaticEntityNetworkData );
		NET_CHECK_STATE_PHYSICS

		return false;
	}
	if ( mode == NSM_BROADCAST ) {
		NET_GET_BASE( sdStaticEntityBroadcastData );
		NET_CHECK_STATE_PHYSICS

		if ( ( fl.hidden ? 1 : 0 ) != baseData.hidden ) {
			return true;
		}
		if ( ( fl.forceDisableClip ? 1 : 0 ) != baseData.forceDisableClip ) {
			return true;
		}

		return false;
	}
	return false;
}

/*
================
idStaticEntity::CreateNetworkStructure
================
*/
sdEntityStateNetworkData* idStaticEntity::CreateNetworkStructure( networkStateMode_t mode ) const {
	if ( mode == NSM_VISIBLE ) {
		sdStaticEntityNetworkData* newData = new sdStaticEntityNetworkData();

		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );

		return newData;
	}
	if ( mode == NSM_BROADCAST ) {
		sdStaticEntityBroadcastData* newData = new sdStaticEntityBroadcastData();

		newData->physicsData = GetPhysics()->CreateNetworkStructure( mode );

		return newData;
	}
	return NULL;
}

/*
===============================================================================

	sdEnvDefinition

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdEnvDefinitionEntity )
END_CLASS

/*
================
sdEnvDefinition::Spawn
================
*/
void sdEnvDefinitionEntity::Spawn( void ) {

	sdEnvDefinition env;
	spawnArgs.GetVector( "origin", "0 0 0", env.origin );
	spawnArgs.GetString( "env_name", "", env.name );
	spawnArgs.GetInt( "env_size", "128", env.size );

	if ( env.name.Length() ) {
		gameLocal.AddEnvDefinition( env );
	} else {
		gameLocal.Warning( "No env_name field specified on environment definition, skipped" );
	}
	PostEventMS( &EV_Remove, 0 );

}

/*
===============================================================================

	sdSpawnController

===============================================================================
*/

CLASS_DECLARATION( sdScriptEntity, sdSpawnController )
END_CLASS

/*
================
sdSpawnController::Spawn
================
*/
void sdSpawnController::Spawn( void ) {
	spawnRequirements.Load( spawnArgs, "require_spawn" );
}


/*
===============================================================================

	sdLiquid

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdLiquid )
END_CLASS

/*
================
sdLiquid::sdLiquid
================
*/
sdLiquid::sdLiquid( void ) {
}

/*
================
sdLiquid::Spawn
================
*/
void sdLiquid::Spawn( void ) {
	current = spawnArgs.GetVector( "current" );
}

/*
===============================================================================

	sdLODEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdLODEntity )
END_CLASS

/*
================
sdLODEntity::sdLODEntity
================
*/
sdLODEntity::sdLODEntity() {
}

/*
================
sdLODEntity::~sdLODEntity
================
*/
sdLODEntity::~sdLODEntity() {
	FreeModelDefs();
}

/*
================
sdLODEntity::FreeModelDefs
================
*/
void sdLODEntity::FreeModelDefs() {
	for ( int i = 0; i < modelDefHandles.Num(); i++ ) {
		int& modelDefHandle = modelDefHandles[ i ];
			
		if ( modelDefHandle != -1 ) {
			renderEntity_t *re = gameRenderWorld->GetRenderEntity( modelDefHandle );
			delete []re->dummies;

			gameRenderWorld->FreeEntityDef( modelDefHandle );
			modelDefHandle = -1;
		}
	}
}

/*
================
sdLODEntity::AddRenderEntity
================
*/
void sdLODEntity::AddRenderEntity( const renderEntity_t& entity, int ID ) {
	modelDefHandles.Alloc() = gameRenderWorld->AddEntityDef( &entity );
	modelID.Alloc() = ID;
}

/*
================
sdLODEntity::AddClipModel
================
*/
void sdLODEntity::AddClipModel( idClipModel* clipModel, const idVec3& origin, const idMat3& axes ) {
	int				numClipModels = physicsObj.GetNumClipModels();

	physicsObj.SetClipModel( clipModel, 1.0f, numClipModels );
	physicsObj.SetOrigin( origin, numClipModels );
	physicsObj.SetAxis( axes, numClipModels );
}

/*
================
sdLODEntity::Spawn
================
*/
void sdLODEntity::Spawn() {
	physicsObj.SetSelf( this );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin(), 0 );
	physicsObj.SetAxis( GetPhysics()->GetAxis(), 0 );

	// add models
	const idKeyValue*	arg;
	renderEntity_t		renderEntity;

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.spawnID = gameLocal.GetSpawnId( this );//renderEntity.entityNum = entityNumber;
	renderEntity.axis.Identity();
	renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;

	const char* temp = NULL;

	for ( int i = 0; i < spawnArgs.GetNumKeyVals(); i++ ) {
		arg = spawnArgs.GetKeyVal( i );

		if ( !arg->GetKey().Icmpn( "model", idStr::Length( "model" ) ) ) {
			const char* model = arg->GetValue();

			if ( *model != '\0' ) {
				renderEntity.hModel = renderModelManager->FindModel( model );
			}

			idStr modelID = ( arg->GetKey().c_str() + idStr::Length( "model" ) );

			if ( renderEntity.hModel != NULL && !renderEntity.hModel->IsDefaultModel() ) {
				renderEntity.bounds		= renderEntity.hModel->Bounds();
				renderEntity.origin		= spawnArgs.GetVector( "origin" + modelID );
				renderEntity.shadowVisDistMult = spawnArgs.GetFloat( "shadowVisDistMult" + modelID, "0" );
				renderEntity.maxVisDist = spawnArgs.GetInt( "maxvisdist" + modelID );
				renderEntity.minVisDist = spawnArgs.GetInt( "minvisdist" + modelID );
				renderEntity.visDistFalloff = spawnArgs.GetFloat( "visDistFalloff" + modelID, "0.25" );
				renderEntity.mapId		= spawnArgs.GetInt( "mapid" + modelID );
				renderEntity.flags.pushByCenter = spawnArgs.GetBool( "pushByOrigin" + modelID );
				renderEntity.flags.occlusionTest = spawnArgs.GetBool( "occlusionTest" + modelID );
				renderEntity.flags.noShadow = spawnArgs.GetBool( "noShadows" + modelID );
				renderEntity.flags.noSelfShadow = spawnArgs.GetBool( "noSelfShadows" + modelID );
				renderEntity.flags.dontCastFromAtmosLight = spawnArgs.GetBool( "dontCastFromAtmosLight" + modelID );
				renderEntity.dummies = NULL;
				renderEntity.numVisDummies = 0;

				idStr gpuSpecParam = spawnArgs.GetString( "drawSpec" + modelID, "low" );
				if ( gpuSpecParam.Icmp( "high" ) == 0 ) {
					renderEntity.drawSpec = 2;
				} else if ( gpuSpecParam.Icmp( "med" ) == 0 || gpuSpecParam.Icmp( "medium" ) == 0 ) {
					renderEntity.drawSpec = 1;
				} else if ( gpuSpecParam.Icmp( "low" ) == 0 ) {
					renderEntity.drawSpec = 0;
				} else {
					renderEntity.drawSpec = 0;
				}

				idStr shadowSpec = spawnArgs.GetString( "shadowSpec" + modelID , "low" );
				if ( shadowSpec.Icmp( "high" ) == 0 ) {
					renderEntity.shadowSpec = 2;
				} else if ( shadowSpec.Icmp( "med" ) == 0 || shadowSpec.Icmp( "medium" ) == 0 ) {
					renderEntity.shadowSpec = 1;
				} else if ( shadowSpec.Icmp( "low" ) == 0 ) {
					renderEntity.shadowSpec = 0;
				} else {
					renderEntity.shadowSpec = 0;
				}

				temp = spawnArgs.GetString( "ambientCubeMap" + modelID );
				if ( *temp ) {
					renderEntity.ambientCubeMap = declHolder.FindAmbientCubeMap( temp );
				} else {
					renderEntity.ambientCubeMap = NULL;
				}

				// add to renderer
				AddRenderEntity( renderEntity, atoi( modelID ) );

				// find inlineCollisionModel value
				if ( !spawnArgs.GetBool( "inlineCollisionModel" + modelID, "1" ) ) {
					// hook up collision model
					AddClipModel( new idClipModel( model ), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
				}

				// find instanced collision models
				int id = 0;
				const idKeyValue* kv = spawnArgs.FindKey( "cm_model" + modelID + "_0" );

				while( kv != NULL ) {
					idVec3 origin = spawnArgs.GetVector( va( "cmodel%s_%d_origin", modelID.c_str(), id ), "0 0 0" );
					idMat3 axis = spawnArgs.GetMatrix( va( "cmodel%s_%d_axis", modelID.c_str(), id ), "1 0 0 0 1 0 0 0 1" );
					AddClipModel( new idClipModel( kv->GetValue().c_str() ), origin, axis );
					id++;
					kv = spawnArgs.FindKey( va( "cm_model%s_%d", modelID.c_str(), id ) );
				}
			}
		}
	}

	physicsObj.SetContents( CONTENTS_SOLID );
	SetPhysics( &physicsObj );
}

void sdLODEntity::PostMapSpawn() {
	idEntity::PostMapSpawn();
	for ( int i = 0; i < modelID.Num(); i++ ) {
		int ID = modelID[i];
		if ( ID != -1 ) {
			renderEntity_t *re = gameRenderWorld->GetRenderEntity( modelDefHandles[i] );

			idStr value = spawnArgs.GetString( va( "visDummies%d", ID ) );

			if ( !value.IsEmpty() ) {

				idStrList strlist;
				idSplitStringIntoList( strlist, value, "," );

				idList< idEntityPtr<idEntity> > dummies;

				idList< int > areas;
				for (int j=0; j<strlist.Num(); j++) {
					idEntity *ent = gameLocal.FindEntity( strlist[j] );
					if ( ent ) {
						idVec3 org = ent->GetPhysics()->GetOrigin();
						int areaNum = gameRenderWorld->PointInArea( org );
						if ( areaNum >= 0 ) {
							bool found = areas.FindIndex( areaNum ) != -1;

							if ( !found ) {
								idEntityPtr<idEntity> &entityPtr = dummies.Alloc();
								entityPtr = ent;
								areas.Alloc() = areaNum;
							}
						}
					}
				}

				re->numVisDummies = dummies.Num();

				if ( re->numVisDummies ) {
					int validcount = 0;
					int c = 0;
					re->dummies = new idVec3[ re->numVisDummies ];
					for (int j=0; j<re->numVisDummies; j++) {
						if ( dummies[j].IsValid() ) {
							re->dummies[c++] = dummies[j].GetEntity()->GetPhysics()->GetOrigin();
						}
					}
					gameRenderWorld->UpdateEntityDef( modelDefHandles[i], re );
				}
			}
		}
	}
}

/*
===============================================================================

sdImposterEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdImposterEntity )
END_CLASS


sdImposterEntity::sdImposterEntity() {
}

sdImposterEntity::~sdImposterEntity() {
	FreeModelDefs();
}

/*
================
sdImposterEntity::FreeModelDefs
================
*/
void sdImposterEntity::FreeModelDefs() {
	for ( int i = 0; i < modelDefHandles.Num(); i++ ) {
		int& modelDefHandle = modelDefHandles[ i ];

		if ( modelDefHandle != -1 ) {
			renderEntity_t *re = gameRenderWorld->GetRenderEntity( modelDefHandle );
			if ( re->insts != NULL ) {
				delete[] re->insts;
			}
			re->insts = NULL;
			gameRenderWorld->FreeEntityDef( modelDefHandle );
			modelDefHandle = -1;
		}
	}
}

/*
================
sdImposterEntity::Spawn
================
*/
struct imposterGather_s {
	idStr name;
	int count;
};
#if 1
void sdImposterEntity::Spawn() {
	const char* defaultMaxVisDist = spawnArgs.GetString( "maxvisdist", "0" );	
	const int imposterStrLen = idStr::Length( "imposter" );
	idList< imposterGather_s > imposterList;
	for( int i = 0; i < spawnArgs.GetNumKeyVals(); i++ )	{
		const idKeyValue* imposterKey = spawnArgs.GetKeyVal( i );
		if( imposterKey->GetKey().Icmpn( "imposter", imposterStrLen ) != 0 ) {
			continue;
		}
		bool found = false;
		for (int j=0; j<imposterList.Num(); j++) {
			if ( imposterKey->GetValue().Icmp( imposterList[j].name.c_str() ) == 0 ) {
				imposterList[j].count++;
				found = true;
			}
		}
		if ( !found ) {
			imposterGather_s entry;
			entry.name = imposterKey->GetValue();
			entry.count = 1;
			imposterList.Alloc() = entry;
		}
	}

	bool errors = false;
	renderEntity_t *renderEntity = new renderEntity_t[ imposterList.Num() ];
	for ( int i=0; i<imposterList.Num(); i++) {
		idDict tempDict;
		tempDict.Set( "forceimposter", "1" );
		tempDict.Set( "model", "_default" );
		tempDict.Set( "imposter", imposterList[i].name );
		gameEdit->ParseSpawnArgsToRenderEntity( tempDict, renderEntity[i] );
		if ( renderEntity[i].imposter == NULL ) {
			common->Warning( "Imposter '%s' not found", imposterList[i].name.c_str() );
			errors = true;
		}

		renderEntity[i].numInsts = 0;
		renderEntity[i].insts = new sdInstInfo[ imposterList[i].count ];
		renderEntity[i].bounds.Clear();
		renderEntity[i].flags.overridenBounds = true;
		renderEntity[i].flags.pushByInstances = true;
	}
	if ( errors ) {
		common->Error( "sdImposterEntity: imposters not found" );
	}

	for( int i = 0; i < spawnArgs.GetNumKeyVals(); i++ ) {
		const idKeyValue* imposterKey = spawnArgs.GetKeyVal( i );
		if( imposterKey->GetKey().Icmpn( "imposter", imposterStrLen ) != 0 ) {
			continue;
		}

		int found = -1;
		for (int j=0; j<imposterList.Num(); j++) {
			if ( imposterKey->GetValue().Icmp( imposterList[j].name.c_str() ) == 0 ) {
				imposterList[j].count++;
				found = j;
			}
		}

		if ( found != -1 ) {
			const char* imposterId = imposterKey->GetKey().c_str() + imposterStrLen;
			sdInstInfo &inst = renderEntity[found].insts[ renderEntity[found].numInsts++ ];

			inst.inst.origin = spawnArgs.GetVector( va( "origin%s", imposterId ), "0 0 0" );
			inst.fadeOrigin = inst.inst.origin;
			inst.inst.axis = spawnArgs.GetMatrix(va( "rotation%s", imposterId ), "1 0 0 0 1 0 0 0 1" );
			inst.maxVisDist = spawnArgs.GetFloat( va( "maxvisdist%s", imposterId ), defaultMaxVisDist );
			inst.minVisDist = 0.f;
//			inst.maxVisDist *= inst.maxVisDist;

			float scalex = renderEntity[found].imposter->GetScaleX();
			float scaley = renderEntity[found].imposter->GetScaleY();
			idBounds bb;
			bb.Clear();
			bb.AddPoint( idVec3( -scalex, -scalex, -scaley ) );
			bb.AddPoint( idVec3(  scalex,  scalex,  scaley ) );
			bb.RotateSelf( inst.inst.axis );
			bb.TranslateSelf( inst.inst.origin );
			renderEntity[found].bounds.AddBounds( bb );

			idVec3 fadeOrigin;
			if ( spawnArgs.GetVector( va( "fadeOrigin%s", imposterId ), "0 0 0", fadeOrigin ) ) {
				inst.fadeOrigin = fadeOrigin;
			}
		}
	}

	for ( int i=0; i<imposterList.Num(); i++) {
		modelDefHandles.Alloc() = gameRenderWorld->AddEntityDef( &renderEntity[i] );
	}

	delete []renderEntity;
}
#else
void sdImposterEntity::Spawn() {	
	renderEntity_t renderEntity;
	idDict tempDict;
	tempDict.Set( "forceimposter", "1" );
	tempDict.Set( "model", "_default" );

	const char* defaultMaxVisDist = spawnArgs.GetString( "maxvisdist", "0" );
	
	const int imposterStrLen = idStr::Length( "imposter" );

	for( int i = 0; i < spawnArgs.GetNumKeyVals(); i++ )	{
		const idKeyValue* imposterKey = spawnArgs.GetKeyVal( i );
		if( imposterKey->GetKey().Icmpn( "imposter", imposterStrLen ) != 0 ) {
			continue;
		}

		const char* imposterId = imposterKey->GetKey().c_str() + imposterStrLen;
		tempDict.Set( "imposter",	imposterKey->GetValue() );
		tempDict.Set( "origin",		spawnArgs.GetString( va( "origin%s", imposterId ), "0 0 0" ) );
		tempDict.Set( "rotation",	spawnArgs.GetString( va( "rotation%s", imposterId ), "1 0 0 0 1 0 0 0 1" ) );
		tempDict.Set( "maxvisdist",	spawnArgs.GetString( va( "maxvisdist%s", imposterId ), defaultMaxVisDist ) );

		idStr fadeOrigin;
		if ( spawnArgs.GetString( va( "fadeOrigin%s", imposterId ), "0 0 0", fadeOrigin ) ) {
			tempDict.Set( "fadeOrigin",	fadeOrigin );
		}

		gameEdit->ParseSpawnArgsToRenderEntity( tempDict, renderEntity );
		modelDefHandles.Alloc() = gameRenderWorld->AddEntityDef( &renderEntity );
	}

	PostEventMS( &EV_Remove, 0 );
}
#endif
/*
===============================================================================

sdJumpPad

===============================================================================
*/

CLASS_DECLARATION( idTrigger_Multi, sdJumpPad )
END_CLASS

/*
=========================
sdJumpPad::Spawn
=========================
*/
void sdJumpPad::Spawn( void ) {
	forceField.SetApplyType( FORCEFIELD_APPLY_VELOCITY );
	forceField.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ) );

	spawnArgs.GetInt( "trigger_wait", "0", triggerWait );

	nextTriggerTime = 0;
}

/*
=========================
sdJumpPad::OnTouch
=========================
*/
void sdJumpPad::OnTouch( idEntity *other, const trace_t& trace ) {
	if ( gameLocal.time < nextTriggerTime ) {
		return;
	}

	if ( GetEntityAllegiance( other ) == TA_ENEMY ) {
		return;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( player != NULL && !player->IsSpectator() && player->GetHealth() <= 0 ) {
		return;
	}

	nextTriggerTime = gameLocal.time + triggerWait;
	forceField.Evaluate( gameLocal.time );

	StartSound( "snd_jump", SND_MOVER_MOVE, 0, NULL );
	PlayEffect( "fx_jump", idVec3( 1.f, 1.f, 1.f ), NULL, GetPhysics()->GetOrigin(), effectAxis );
}

/*
=========================
sdJumpPad::PostMapSpawn
=========================
*/
void sdJumpPad::PostMapSpawn( void ) {
	FindTargets();
	RemoveNullTargets();

	if ( targets.Num() != 1 ) {
		gameLocal.Warning( "sdJumpPad::PostMapSpawn: jumppad %s should have only 1 target", name.c_str() );
		return;
	}

	idEntity* target = targets[ 0 ];
	if ( target == NULL ) {
		assert( target != NULL );
		gameLocal.Warning( "sdJumpPad::PostMapSpawn: jumppad %s has a NULL target entity", name.c_str() );
		return;
	}

	idVec3 diff = target->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	idVec3 gravity( GetPhysics()->GetGravity() );
	idVec3 gravityNormal( 0, 0, -1.f );
	idVec3 vertical( 0.f, 0.f, diff.z );

	float time = idMath::Sqrt( vertical.Length() / ( 0.5f * gravity.Length() ) );
	if ( time <= idMath::FLT_EPSILON ) {
		// no point in having the jumppad
		gameLocal.Warning( "sdJumpPad::PostMapSpawn: removing jumppad %s as target too close to trigger", name.c_str() );
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	// calc the non-vertical velocity
	idVec3 velocity( diff - vertical );
	float distance = velocity.Normalize();
	velocity = velocity * ( distance / time );

	// add the vertical velocity
	velocity += (gravity * -time);

	forceField.Uniform( velocity );

	// calc the effect axis for playing the effect
	diff.Normalize();
	effectAxis = diff.ToMat3();
}


/*
===============================================================================

	sdInstStatic

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdInstStatic )
END_CLASS

/*
================
sdInstStatic::sdInstStatic
================
*/
sdInstStatic::sdInstStatic() {
}

/*
================
sdInstStatic::~sdInstStatic
================
*/
sdInstStatic::~sdInstStatic() {
	FreeModelDefs();
}

/*
================
sdInstStatic::FreeModelDefs
================
*/
void sdInstStatic::FreeModelDefs() {
	for ( int i = 0; i < modelDefHandles.Num(); i++ ) {
		int& modelDefHandle = modelDefHandles[ i ];

		if ( modelDefHandle != -1 ) {
			renderEntity_t *re = gameRenderWorld->GetRenderEntity( modelDefHandle );
			if ( re->insts != NULL ) {
				delete[] re->insts;
			}
			re->insts = NULL;
			gameRenderWorld->FreeEntityDef( modelDefHandle );
			modelDefHandle = -1;
		}
	}
}

/*
================
sdInstStatic::AddRenderEntity
================
*/
void sdInstStatic::AddRenderEntity( const renderEntity_t& entity ) {
	modelDefHandles.Alloc() = gameRenderWorld->AddEntityDef( &entity );
}

/*
================
sdInstStatic::AddClipModel
================
*/
void sdInstStatic::AddClipModel( idClipModel* clipModel, const idVec3& origin, const idMat3& axes ) {
	int				numClipModels = physicsObj.GetNumClipModels();

	physicsObj.SetClipModel( clipModel, 1.0f, numClipModels );
	physicsObj.SetOrigin( origin, numClipModels );
	physicsObj.SetAxis( axes, numClipModels );
}

/*
================
sdInstStatic::Spawn
================
*/
void sdInstStatic::Spawn() {
	physicsObj.SetSelf( this );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin(), 0 );
	physicsObj.SetAxis( GetPhysics()->GetAxis(), 0 );

	// add models
	const idKeyValue*	arg;
	renderEntity_t		renderEntity;
	const char* modelInstance = spawnArgs.GetString( "model_instance" );

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	renderEntity.spawnID = gameLocal.GetSpawnId( this );//	renderEntity.entityNum = entityNumber;
	renderEntity.axis.Identity();
	renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
	renderEntity.hModel = renderModelManager->FindModel( modelInstance );
	renderEntity.flags.overridenBounds = true;
	renderEntity.flags.noShadow = true;
	renderEntity.flags.pushByInstances = true;
	renderEntity.flags.pushByCenter = spawnArgs.GetBool( "pushByOrigin" );

	if ( renderEntity.hModel == NULL ) {
		gameLocal.Warning( "sdInstStatic::Spawn : no model for entity '%s'", GetName() );
		PostEventMS( &EV_Remove, 0 );
		return;
	}

	const char* temp;
	temp = spawnArgs.GetString( "imposter_instance" );
	if ( *temp != '\0' ) {
		renderEntity.imposter = declHolder.FindImposter( temp );
	}

	int count = 0;
	for ( int i = 0; i < spawnArgs.GetNumKeyVals(); i++ ) {
		arg = spawnArgs.GetKeyVal( i );
		char prefix[32];
		sprintf( prefix, "%d ", i );
		if ( spawnArgs.MatchPrefix( prefix ) ) {
			count++;
		} else {
			break;
		}
	}

	renderEntity.bounds.Clear();
	renderEntity.numInsts = count;
	renderEntity.insts = new sdInstInfo[ count ];

	int index = 0;
	const idKeyValue*	lastArg = NULL;
	for ( int i = 0; i < count; i++ ) {
		char cprefix[32];
		sprintf(cprefix, "%d ", i );
		idStr prefix;
		prefix = cprefix;
		arg = spawnArgs.MatchPrefix( prefix, lastArg );
		if ( arg ) {
			lastArg = arg;
			arg = spawnArgs.MatchPrefix( prefix, lastArg );

			sdInstInfo &info = renderEntity.insts[ i ];

			info.inst.origin = spawnArgs.GetVector( prefix + "origin" );
			info.fadeOrigin = info.inst.origin;
			info.maxVisDist = spawnArgs.GetInt( prefix + "maxVisDist" );
			info.minVisDist = spawnArgs.GetInt( prefix + "minVisDist" );
			info.inst.axis = spawnArgs.GetMatrix( prefix + "rotation", "1 0 0 0 1 0 0 0 1" );
			info.inst.color[0] = 255;
			info.inst.color[1] = 255;
			info.inst.color[2] = 255;
			info.inst.color[3] = 255;

			idBounds bb;
			bb.FromTransformedBounds( renderEntity.hModel->Bounds(), info.inst.origin, info.inst.axis );
			renderEntity.bounds.AddBounds( bb );

			idStr cm;
			if ( spawnArgs.GetString( prefix + "cm_model", "", cm ) ) {
				AddClipModel( new idClipModel( cm ), info.inst.origin, info.inst.axis );
			}
		}
	}
				
	AddRenderEntity( renderEntity );

	physicsObj.SetContents( CONTENTS_SOLID );
	SetPhysics( &physicsObj );
}

/*
===============================================================================

	sdEnvBounds

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdEnvBoundsEntity )
END_CLASS

/*
================
sdEnvDefinition::Spawn
================
*/
void sdEnvBoundsEntity::Spawn( void ) {

	idVec3 origin, size;
	idStr name;
	spawnArgs.GetVector( "origin", "0 0 0", origin );
	spawnArgs.GetVector( "size", "8 8 8", size );
	spawnArgs.GetString( "env_name", "", name );

	if ( name.Length() ) {
		gameRenderWorld->AddEnvBounds( origin, size, name );
	} else {
		gameLocal.Warning( "No env_name field specified on environment definition, skipped" );
	}
	PostEventMS( &EV_Remove, 0 );

}





/*
===============================================================================

	sdLadderEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, sdLadderEntity )
END_CLASS

/*
================
sdLadderEntity::Spawn
================
*/
void sdLadderEntity::Spawn( void ) {
	ladderModel = NULL;

	idClipModel* model = GetPhysics()->GetClipModel();
	if ( model == NULL ) {
		gameLocal.Error( "sdLadderEntity::Spawn No Collision Model" );
	}

	bool surfaceFound = false;
	for ( int i = 0; i < model->GetNumCollisionModels(); i++ ) {
		idCollisionModel* cm = model->GetCollisionModel( i );
		for ( int j = 0; j < cm->GetNumPolygons(); j++ ) {
			const idMaterial* material = cm->GetPolygonMaterial( j );
			if ( material == NULL ) {
				continue;
			}

			if ( !( material->GetSurfaceFlags() & SURF_LADDER ) ) {
				continue;
			}

			if ( surfaceFound ) {
				gameLocal.Error( "sdLadderEntity::Spawn Multiple Ladder Surfaces" );
			}
			surfaceFound = true;

			ladderNormal = cm->GetPolygonPlane( j ).Normal();

			idFixedWinding ladderWinding;
			cm->GetPolygon( j, ladderWinding );

			idTraceModel trm;
			trm.SetupPolygonPrism( ladderWinding, 16.f );

			ladderModel = new idClipModel( trm, true );
		}
	}
	if ( !surfaceFound ) {
		gameLocal.Error( "sdLadderEntity::Spawn No Ladder Surface Found" );
	}

//	BecomeActive( TH_THINK );
}

/*
================
sdLadderEntity::~sdLadderEntity
================
*/
sdLadderEntity::~sdLadderEntity( void ) {
	gameLocal.clip.DeleteClipModel( ladderModel );
}

/*
================
sdLadderEntity::Think
================
*/
void sdLadderEntity::Think( void ) {
	idEntity::Think();

//	ladderModel->Draw( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
}

/*
================
sdLadderEntity::GetLadderNormal
================
*/
idVec3 sdLadderEntity::GetLadderNormal( void ) const {
	return GetPhysics()->GetAxis() * ladderNormal;
}

#include "botai/BotThreadData.h"

CLASS_DECLARATION( idEntity, idAASObstacleEntity )
	EVENT( EV_Activate,		idAASObstacleEntity::Event_Activate )
END_CLASS

/*
===============
idAASObstacleEntity::idAASObstacleEntity
================
*/
idAASObstacleEntity::idAASObstacleEntity( void ) {
	enabled = false;
	team = 2;
}

/*
===============
idAASObstacleEntity::Spawn
================
*/
void idAASObstacleEntity::Spawn( ) {
	enabled = !spawnArgs.GetBool( "start_on", "1" );
	team = spawnArgs.GetInt( "team", "2" );
	ChangeAreaState();
}

/*
===============
idAASObstacleEntity::Event_Activate
================
*/
void idAASObstacleEntity::Event_Activate( idEntity *activator ) {
	enabled = !enabled;
	ChangeAreaState();
}

/*
===============
idAASObstacleEntity::ChangeAreaState
================
*/
void idAASObstacleEntity::ChangeAreaState( ) {
	botThreadData.EnableArea( GetPhysics()->GetAbsBounds(), AAS_AREA_CONTENTS_OBSTACLE, team, enabled );
}
