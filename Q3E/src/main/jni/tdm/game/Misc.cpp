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

// Copyright (C) 2004 Id Software, Inc.
//
/*

Various utility objects and functions.

*/

#include "precompiled.h"
#pragma hdrstop



#include "Game_local.h"
#include "SndProp.h"
#include "EscapePointManager.h"
#include "Objectives/MissionData.h"
#include "StimResponse/StimResponseCollection.h"

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

	idPlayerStart

===============================================================================
*/

const idEventDef EV_TeleportStage( "<TeleportStage>", EventArgs('e', "", ""), EV_RETURNS_VOID, "internal" );

CLASS_DECLARATION( idEntity, idPlayerStart )
	EVENT( EV_Activate,			idPlayerStart::Event_TeleportPlayer )
	EVENT( EV_TeleportStage,	idPlayerStart::Event_TeleportStage )
END_CLASS

/*
===============
idPlayerStart::idPlayerStart
================
*/
idPlayerStart::idPlayerStart( void ) {
	teleportStage = 0;
}

/*
===============
idPlayerStart::Spawn
================
*/
void idPlayerStart::Spawn( void ) {
	teleportStage = 0;
}

/*
================
idPlayerStart::Save
================
*/
void idPlayerStart::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( teleportStage );
}

/*
================
idPlayerStart::Restore
================
*/
void idPlayerStart::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( teleportStage );
}

/*
================
idPlayerStart::ClientReceiveEvent
================
*/
bool idPlayerStart::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	int entityNumber;

	switch( event ) {
		case EVENT_TELEPORTPLAYER: {
			entityNumber = msg.ReadBits( GENTITYNUM_BITS );
			idPlayer *player = static_cast<idPlayer *>( gameLocal.entities[entityNumber] );
			if ( player != NULL && player->IsType( idPlayer::Type ) ) {
				Event_TeleportPlayer( player );
			}
			return true;
		}
		default: {
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
//	return false;
}

/*
===============
idPlayerStart::Event_TeleportStage

FIXME: add functionality to fx system ( could be done with player scripting too )
================
*/
void idPlayerStart::Event_TeleportStage( idEntity *_player ) {
	idPlayer *player;

	if ( !_player->IsType( idPlayer::Type ) ) {

		common->Warning( "idPlayerStart::Event_TeleportStage: entity is not an idPlayer" );

		return;

	}

	player = static_cast<idPlayer*>(_player);

	float teleportDelay = spawnArgs.GetFloat( "teleportDelay" );
	switch ( teleportStage ) {
		case 0:
			player->playerView.Flash( colorWhite, 125 );
			player->SetInfluenceLevel( INFLUENCE_LEVEL3 );
			player->SetInfluenceView( spawnArgs.GetString( "mtr_teleportFx" ), NULL, 0.0f, NULL );
			gameSoundWorld->FadeSoundClasses( 0, -20.0f, teleportDelay );
			player->StartSound( "snd_teleport_start", SND_CHANNEL_BODY2, 0, false, NULL );
			teleportStage++;
			PostEventSec( &EV_TeleportStage, teleportDelay, player );
			break;
		case 1:
			gameSoundWorld->FadeSoundClasses( 0, 0.0f, 0.25f );
			teleportStage++;
			PostEventSec( &EV_TeleportStage, 0.25f, player );
			break;
		case 2:
			player->SetInfluenceView( NULL, NULL, 0.0f, NULL );
			TeleportPlayer( player );
			player->StopSound( SND_CHANNEL_BODY2, false );
			player->SetInfluenceLevel( INFLUENCE_NONE );
			teleportStage = 0;
			break;
		default:
			break;
	}
}

/*
===============
idPlayerStart::TeleportPlayer
================
*/
void idPlayerStart::TeleportPlayer( idPlayer *player ) {
	float pushVel = spawnArgs.GetFloat( "push", "300" );
	float f = spawnArgs.GetFloat( "visualEffect", "0" );
	const char *viewName = spawnArgs.GetString( "visualView", "" );
	idEntity *ent = viewName ? gameLocal.FindEntity( viewName ) : NULL;

	if ( f && ent ) {
		// place in private camera view for some time
		// the entity needs to teleport to where the camera view is to have the PVS right
		player->Teleport( ent->GetPhysics()->GetOrigin(), ang_zero, this );
		player->StartSound( "snd_teleport_enter", SND_CHANNEL_ANY, 0, false, NULL );
		player->SetPrivateCameraView( static_cast<idCamera*>(ent) );
		// the player entity knows where to spawn from the previous Teleport call
	} else {
		// direct to exit, Teleport will take care of the killbox
		player->Teleport( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis().ToAngles(), NULL );
	}
}

/*
===============
idPlayerStart::Event_TeleportPlayer
================
*/
void idPlayerStart::Event_TeleportPlayer( idEntity *activator ) {
	idPlayer *player;

	if ( activator->IsType( idPlayer::Type ) ) {
		player = static_cast<idPlayer*>( activator );
	} else {
		player = gameLocal.GetLocalPlayer();
	}
	if ( player ) {
		if ( spawnArgs.GetBool( "visualFx" ) ) {

			teleportStage = 0;
			Event_TeleportStage( player );

		} else {
			TeleportPlayer( player );
		}
	}
}

/*
===============================================================================

	idActivator

===============================================================================
*/

CLASS_DECLARATION( idEntity, idActivator )
	EVENT( EV_Activate,		idActivator::Event_Activate )
END_CLASS

/*
===============
idActivator::Save
================
*/
void idActivator::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( stay_on );
}

/*
===============
idActivator::Restore
================
*/
void idActivator::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( stay_on );

	if ( stay_on ) {
		BecomeActive( TH_THINK );
	}
}

/*
===============
idActivator::Spawn
================
*/
void idActivator::Spawn( void ) {
	bool start_off;

	spawnArgs.GetBool( "stay_on", "0", stay_on );
	spawnArgs.GetBool( "start_off", "0", start_off );

	GetPhysics()->SetClipBox( idBounds( vec3_origin ).Expand( 4 ), 1.0f );
	GetPhysics()->SetContents( 0 );

	if ( !start_off ) {
		BecomeActive( TH_THINK );
	}
}

/*
===============
idActivator::Think
================
*/
void idActivator::Think( void ) {
	RunPhysics();
	if ( thinkFlags & TH_THINK ) {
		if ( TouchTriggers() ) {
			if ( !stay_on ) {
				BecomeInactive( TH_THINK );
			}
		}
	}
	Present();
}

/*
===============
idActivator::Activate
================
*/
void idActivator::Event_Activate( idEntity *activator ) {
	if ( thinkFlags & TH_THINK ) {
		BecomeInactive( TH_THINK );
	} else {
		BecomeActive( TH_THINK );
	}
}


/*
===============================================================================

idPathCorner

===============================================================================
*/

CLASS_DECLARATION( idEntity, idPathCorner )
	EVENT( AI_RandomPath,		idPathCorner::Event_RandomPath )
END_CLASS

/*
=====================
idPathCorner::Spawn
=====================
*/
void idPathCorner::Spawn( void ) {
}

/*
=====================
idPathCorner::DrawDebugInfo
=====================
*/
void idPathCorner::DrawDebugInfo( void ) {
	idEntity *ent;
	idBounds bnds( idVec3( -4.0, -4.0f, -8.0f ), idVec3( 4.0, 4.0f, 64.0f ) );

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( !ent->IsType( idPathCorner::Type ) ) {
			continue;
		}

		idVec3 org = ent->GetPhysics()->GetOrigin();
		gameRenderWorld->DebugBounds( colorRed, bnds, org, 0 );
	}
}

/*
============
idPathCorner::RandomPath
============
*/
idPathCorner *idPathCorner::RandomPath( const idEntity *source, const idEntity *ignore, idAI* owner )
{
	if ( source == NULL ) // grayman #3405
	{
		return NULL;
	}

	idFlexList<idPathCorner*, 128> path;

	float rand(gameLocal.random.RandomFloat());
	float accumulatedChance(0);
	float maxChance(0);

	idEntity* candidate(NULL);

	for ( int i = 0 ; i < source->targets.Num() ; i++ )
	{
		idEntity* ent = source->targets[ i ].GetEntity();
		if ( ent && ( ent != ignore ) && ent->IsType( idPathCorner::Type ) ) 
		{
			if (owner)
			{
				if (owner->HasSeenEvidence() && ent->spawnArgs.GetBool("idle_only", "0"))
				{
					continue;
				}
				
				if (!owner->HasSeenEvidence() && ent->spawnArgs.GetBool("alert_idle_only", "0"))
				{
					continue;
				}
			}

			float chance = ent->spawnArgs.GetFloat("chance", "0");
			if (chance)
			{
				// angua: path has chance spawn arg set
				// sum of the probability of this path and of the previous ones
				accumulatedChance += chance;

				// if the random number is between the current and the previous sum, this is our next path
				if (rand < accumulatedChance)
				{
					return static_cast<idPathCorner *>( ent );
				}

				// store the path with the highest chance
				if (chance > maxChance)
				{
					maxChance = chance;
					candidate = ent;
				}
			}
			else
			{
				// path doesn't have chance spawn arg set
				// add to list
				path.AddGrow( static_cast<idPathCorner *>( ent ) );
			}
		}
	}

	// probability comparison didn't return a path

	// no path without chance spawn arg (chance sum is < 1)
	if ( path.Num() == 0 )
	{
		if (candidate)
		{
			// return the path with the highest chance
			return static_cast<idPathCorner *>(candidate);
		}
		return NULL;
	}

	// choose one from the list
	int which = gameLocal.random.RandomInt( path.Num() );
	return path[ which ];
}

/*
=====================
idPathCorner::Event_RandomPath
=====================
*/
void idPathCorner::Event_RandomPath( void ) {
	idPathCorner *path;

	path = RandomPath( this, NULL, NULL );
	idThread::ReturnEntity( path );
}

/*
===============================================================================

tdmPathFlee

===============================================================================
*/

CLASS_DECLARATION( idEntity, tdmPathFlee )
END_CLASS

tdmPathFlee::~tdmPathFlee()
{
	// Unregister self with the escape point manager
	gameLocal.m_EscapePointManager->RemoveEscapePoint(this);
}

/*
=====================
tdmPathFlee::Spawn
=====================
*/
void tdmPathFlee::Spawn( void ) {

	// Get the location of this entity within the AAS
	

	// Register this class with the escape point manager
	gameLocal.m_EscapePointManager->AddEscapePoint(this);
}

/*
===============================================================================

tdmPathGuard

===============================================================================
*/

CLASS_DECLARATION( idEntity, tdmPathGuard )
END_CLASS

tdmPathGuard::~tdmPathGuard()
{
}

/*
=====================
tdmPathGuard::Spawn
=====================
*/
void tdmPathGuard::Spawn( void )
{
	m_priority = spawnArgs.GetInt("priority", "1");

	// If the "angle" spawnarg is present, it's the angle the guard
	// should turn toward when he reaches the guard spot. If the
	// spawnarg isn't present, the default is to turn toward the
	// origin of the search.
	if (spawnArgs.FindKey("angle") != NULL)
	{
		m_angle = spawnArgs.GetFloat("angle");
	}
	else
	{
		m_angle = idMath::INFINITY;
	}
}

/*
================
tdmPathGuard::Save
================
*/
void tdmPathGuard::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( m_priority );
	savefile->WriteFloat( m_angle );
}

/*
================
tdmPathGuard::Restore
================
*/
void tdmPathGuard::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( m_priority );
	savefile->ReadFloat( m_angle );
}

/*
===============================================================================

  idDamagable
	
===============================================================================
*/

const idEventDef EV_RestoreDamagable( "<RestoreDamagable>", EventArgs(), EV_RETURNS_VOID, "internal" );

CLASS_DECLARATION( idEntity, idDamagable )
	EVENT( EV_Activate,			idDamagable::Event_BecomeBroken )
	EVENT( EV_RestoreDamagable,	idDamagable::Event_RestoreDamagable )
END_CLASS

/*
================
idDamagable::idDamagable
================
*/
idDamagable::idDamagable( void ) {
	count = 0;
	nextTriggerTime = 0;
}

/*
================
idDamagable::Save
================
*/
void idDamagable::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( count );
	savefile->WriteInt( nextTriggerTime );
}

/*
================
idDamagable::Restore
================
*/
void idDamagable::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( count );
	savefile->ReadInt( nextTriggerTime );
}

/*
================
idDamagable::Spawn
================
*/
void idDamagable::Spawn( void ) 
{
	idStr broken;

	health = spawnArgs.GetInt( "health", "5" );
	spawnArgs.GetInt( "count", "1", count );	
	nextTriggerTime = 0;
	
	// make sure the model gets cached
	spawnArgs.GetString( "broken", "", broken );
	if ( broken.Length() && !renderModelManager->CheckModel( broken ) ) {
		gameLocal.Error( "idDamagable '%s' at (%s): cannot load broken model '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), broken.c_str() );
	}

	fl.takedamage = true;
	GetPhysics()->SetContents( CONTENTS_SOLID );
	if( m_CustomContents != -1 )
		GetPhysics()->SetContents( m_CustomContents );

	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );
}

/*
================
idDamagable::BecomeBroken
================
*/
void idDamagable::BecomeBroken( idEntity *activator ) {
	float	forceState;
	int		numStates;
	int		cycle;
	float	wait;
	
	if ( gameLocal.time < nextTriggerTime ) {
		return;
	}

	spawnArgs.GetFloat( "wait", "0.1", wait );
	nextTriggerTime = gameLocal.time + SEC2MS( wait );
	if ( count > 0 ) {
		count--;
		if ( !count ) {
			fl.takedamage = false;
		} else {
			health = spawnArgs.GetInt( "health", "5" );
		}
	}

	idStr	broken;

	spawnArgs.GetString( "broken", "", broken );
	if ( broken.Length() ) {
		SetModel( broken );
	}

	// offset the start time of the shader to sync it to the gameLocal time
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	spawnArgs.GetInt( "numstates", "1", numStates );
	spawnArgs.GetInt( "cycle", "0", cycle );
	spawnArgs.GetFloat( "forcestate", "0", forceState );

	// set the state parm
	if ( cycle ) {
		renderEntity.shaderParms[ SHADERPARM_MODE ]++;
		if ( renderEntity.shaderParms[ SHADERPARM_MODE ] > numStates ) {
			renderEntity.shaderParms[ SHADERPARM_MODE ] = 0;
		}
	} else if ( forceState ) {
		renderEntity.shaderParms[ SHADERPARM_MODE ] = forceState;
	} else {
		renderEntity.shaderParms[ SHADERPARM_MODE ] = gameLocal.random.RandomInt( numStates ) + 1;
	}

	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );

	ActivateTargets( activator );

	if ( spawnArgs.GetBool( "hideWhenBroken" ) ) {
		Hide();
		PostEventMS( &EV_RestoreDamagable, nextTriggerTime - gameLocal.time );
		BecomeActive( TH_THINK );
	}
}

/*
================
idDamagable::Killed
================
*/
void idDamagable::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) 
{
	bool bPlayerResponsible(false);

	if ( gameLocal.time < nextTriggerTime ) 
	{
		health += damage;
		return;
	}

	if ( attacker && attacker->IsType( idPlayer::Type ) )
		bPlayerResponsible = ( attacker == gameLocal.GetLocalPlayer() );
	else if( attacker && attacker->m_SetInMotionByActor.GetEntity() )
		bPlayerResponsible = ( attacker->m_SetInMotionByActor.GetEntity() == gameLocal.GetLocalPlayer() );

	gameLocal.m_MissionData->MissionEvent( COMP_DESTROY, this, bPlayerResponsible );
	BecomeBroken( attacker );
}

/*
================
idDamagable::Event_BecomeBroken
================
*/
void idDamagable::Event_BecomeBroken( idEntity *activator ) {
	BecomeBroken( activator );
}

/*
================
idDamagable::Event_RestoreDamagable
================
*/
void idDamagable::Event_RestoreDamagable( void ) {
	health = spawnArgs.GetInt( "health", "5" );
	Show();
}


/*
===============================================================================

  idExplodable
	
===============================================================================
*/

CLASS_DECLARATION( idEntity, idExplodable )
	EVENT( EV_Activate,	idExplodable::Event_Explode )
END_CLASS

/*
================
idExplodable::Spawn
================
*/
void idExplodable::Spawn( void ) {
	Hide();
}

/*
================
idExplodable::Event_Explode
================
*/
void idExplodable::Event_Explode( idEntity *activator ) {
	const char *temp;

	if ( spawnArgs.GetString( "def_damage", "damage_explosion", &temp ) ) {
		gameLocal.RadiusDamage( GetPhysics()->GetOrigin(), activator, activator, this, this, temp );
	}

	StartSound( "snd_explode", SND_CHANNEL_ANY, 0, false, NULL );

	// Show() calls UpdateVisuals, so we don't need to call it ourselves after setting the shaderParms
	renderEntity.shaderParms[SHADERPARM_RED]		= 1.0f;
	renderEntity.shaderParms[SHADERPARM_GREEN]		= 1.0f;
	renderEntity.shaderParms[SHADERPARM_BLUE]		= 1.0f;
	renderEntity.shaderParms[SHADERPARM_ALPHA]		= 1.0f;
	renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
	renderEntity.shaderParms[SHADERPARM_DIVERSITY]	= 0.0f;
	Show();

	PostEventMS( &EV_Remove, 2000 );

	ActivateTargets( activator );
}


/*
===============================================================================

  idSpring
	
===============================================================================
*/

CLASS_DECLARATION( idEntity, idSpring )
	EVENT( EV_PostSpawn,	idSpring::Event_LinkSpring )
END_CLASS

/*
================
idSpring::Think
================
*/
void idSpring::Think( void ) {
	idVec3 start, end, origin;
	idMat3 axis;

	// run physics
	RunPhysics();

	if ( thinkFlags & TH_THINK ) {
		// evaluate force
		spring.Evaluate( gameLocal.time );

		start = p1;
		if ( ent1->GetPhysics() ) {
			axis = ent1->GetPhysics()->GetAxis();
			origin = ent1->GetPhysics()->GetOrigin();
			start = origin + start * axis;
		}

		end = p2;
		if ( ent2->GetPhysics() ) {
			axis = ent2->GetPhysics()->GetAxis();
			origin = ent2->GetPhysics()->GetOrigin();
			end = origin + p2 * axis;
		}
		
		gameRenderWorld->DebugLine( idVec4(1, 1, 0, 1), start, end, 0, true );
	}

	Present();
}

/*
================
idSpring::Event_LinkSpring
================
*/
void idSpring::Event_LinkSpring( void ) {
	idStr name1, name2;

	spawnArgs.GetString( "ent1", "", name1 );
	spawnArgs.GetString( "ent2", "", name2 );

	if ( name1.Length() ) {
		ent1 = gameLocal.FindEntity( name1 );
		if ( !ent1 ) {
			gameLocal.Error( "idSpring '%s' at (%s): cannot find first entity '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), name1.c_str() );
		}
	}
	else {
		ent1 = gameLocal.entities[ENTITYNUM_WORLD];
	}

	if ( name2.Length() ) {
		ent2 = gameLocal.FindEntity( name2 );
		if ( !ent2 ) {
			gameLocal.Error( "idSpring '%s' at (%s): cannot find second entity '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), name2.c_str() );
		}
	}
	else {
		ent2 = gameLocal.entities[ENTITYNUM_WORLD];
	}
	spring.SetPosition( ent1->GetPhysics(), id1, p1, ent2->GetPhysics(), id2, p2 );
	BecomeActive( TH_THINK );
}

/*
================
idSpring::Spawn
================
*/
void idSpring::Spawn( void ) {
	float Kstretch, damping, restLength;

	spawnArgs.GetInt( "id1", "0", id1 );
	spawnArgs.GetInt( "id2", "0", id2 );
	spawnArgs.GetVector( "point1", "0 0 0", p1 );
	spawnArgs.GetVector( "point2", "0 0 0", p2 );
	spawnArgs.GetFloat( "constant", "100.0f", Kstretch );
	spawnArgs.GetFloat( "damping", "10.0f", damping );
	spawnArgs.GetFloat( "restlength", "0.0f", restLength );

	spring.InitSpring( Kstretch, 0.0f, damping, restLength );

	ent1 = ent2 = NULL;

	PostEventMS( &EV_PostSpawn, 0 );
}

/*
===============================================================================

  idForceField
	
===============================================================================
*/

const idEventDef EV_Toggle( "Toggle", EventArgs(), EV_RETURNS_VOID, "Turns the forcefield on and off." );

CLASS_DECLARATION( idEntity, idForceField )
	EVENT( EV_Activate,		idForceField::Event_Activate )
	EVENT( EV_Toggle,		idForceField::Event_Toggle )
	EVENT( EV_FindTargets,	idForceField::Event_FindTargets )
END_CLASS

/*
===============
idForceField::Toggle
================
*/
void idForceField::Toggle( void ) {
	if ( thinkFlags & TH_THINK ) {
		BecomeInactive( TH_THINK );
	} else {
		BecomeActive( TH_THINK );
	}
}

/*
================
idForceField::Think
================
*/
void idForceField::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		// evaluate force
		forceField.Evaluate( gameLocal.time );
	}
	Present();
}

/*
================
idForceField::Save
================
*/
void idForceField::Save( idSaveGame *savefile ) const {
	savefile->WriteStaticObject( forceField );
}

/*
================
idForceField::Restore
================
*/
void idForceField::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( forceField );
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

	// grayman #2975 - for scaleImpulse == 1, use the player mass in a calculation
	// that reduces force on small objects. Provides more realistic behavior
	// when the same force field is used on small objects and the player.

	idPlayer *player = gameLocal.GetLocalPlayer();
	float mass = 70;
	if ( player )
	{
		mass = player->spawnArgs.GetFloat("mass","70");
	}

	forceField.SetPlayerMass( mass );
	forceField.SetScale( spawnArgs.GetBool("scale_impulse","0") ); // identify when to apply scaled force

	// set the collision model on the force field
	forceField.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ) );

	// remove the collision model from the physics object
	GetPhysics()->SetClipModel( NULL, 1.0f );

	if ( spawnArgs.GetBool( "start_on" ) ) {
		BecomeActive( TH_THINK );
	}
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
===============================================================================

	idAnimated

===============================================================================
*/

const idEventDef EV_Animated_Start( "<start>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_LaunchMissiles( "launchMissiles", 
	EventArgs('s', "projectilename", "",
			  's', "sound", "",
			  's', "launchbone", "",
			  's', "targetbone", "",
			  'd', "numshots", "",
			  'f', "framedelay", ""),
	EV_RETURNS_VOID, 
	"Launches a projectile.");
const idEventDef EV_LaunchMissilesUpdate( "<launchMissiles>", EventArgs('d', "", "", 'd', "", "", 'd', "", "", 'd', "", ""), EV_RETURNS_VOID, "internal" );
const idEventDef EV_AnimDone( "<AnimDone>", EventArgs('d', "", ""), EV_RETURNS_VOID, "internal" );
const idEventDef EV_StartRagdoll( "startRagdoll", EventArgs(), EV_RETURNS_VOID, "Switches to a ragdoll taking over the animation." );

CLASS_DECLARATION( idAFEntity_Gibbable, idAnimated )
	EVENT( EV_Activate,				idAnimated::Event_Activate )
	EVENT( EV_Animated_Start,		idAnimated::Event_Start )
	EVENT( EV_StartRagdoll,			idAnimated::Event_StartRagdoll )
	EVENT( EV_AnimDone,				idAnimated::Event_AnimDone )
	EVENT( EV_Footstep,				idAnimated::Event_Footstep )
	EVENT( EV_FootstepLeft,			idAnimated::Event_Footstep )
	EVENT( EV_FootstepRight,		idAnimated::Event_Footstep )
	EVENT( EV_LaunchMissiles,		idAnimated::Event_LaunchMissiles )
	EVENT( EV_LaunchMissilesUpdate,	idAnimated::Event_LaunchMissilesUpdate )
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
	delete combatModel;
	combatModel = NULL;
}

/*
===============
idAnimated::Save
================
*/
void idAnimated::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( current_anim_index );
	savefile->WriteInt( num_anims );
	savefile->WriteInt( anim );
	savefile->WriteInt( blendFrames );
	savefile->WriteJoint( soundJoint );
	activator.Save( savefile );
	savefile->WriteBool( activated );
}

/*
===============
idAnimated::Restore
================
*/
void idAnimated::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( current_anim_index );
	savefile->ReadInt( num_anims );
	savefile->ReadInt( anim );
	savefile->ReadInt( blendFrames );
	savefile->ReadJoint( soundJoint );
	activator.Restore( savefile );
	savefile->ReadBool( activated );
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
bool idAnimated::LoadAF( void ) 
{
	bool bReturnVal = false;
	idStr fileName;

	if ( !spawnArgs.GetString( "ragdoll", "*unknown*", fileName ) ) 
	{
		goto Quit;
	}
	af.SetAnimator( GetAnimator() );
	bReturnVal = af.Load( this, fileName );
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
===============
idAnimated::Think
===============
*/
void idAnimated::Think( void )
{
	// SteveL #3770: Allow idAnimateds to use LOD and support switching between a non-animated model and an animated one.
	idAFEntity_Gibbable::Think();
}

/*
===============
idAnimated::SwapLODModel
===============
*/
void idAnimated::SwapLODModel( const char *modelname )
{
	// idAnimateds are the only class allowed to simply still their md5mesh as a LOD 
	// optimization, which they do by setting "model_lod_N" to the special value "be_still".
	// They can also swap in another animated model, or a static FS model.
	bool stopAnims = ( idStr::Icmp( modelname, "be_still" ) == 0 );

	if ( stopAnims )
	{
		animator.Clear( ANIMCHANNEL_ALL, gameLocal.time, FRAME2MS( blendFrames ) );
	}
	else if ( animator.ModelDef() && idStr::Cmp( modelname, animator.ModelDef()->GetName() ) == 0 )
	{
		// We already have the right model, so we don't need to switch
	}
	else
	{
		idAnimatedEntity::SwapLODModel( modelname );
	}

	if ( !stopAnims )
	{
		// If switching back to an animated model from a non-moving one, we can't rely on anims being 
		// preserved through the switch like other idAnimatedEntities can. So restart our anims here. 
		// Only restart the start_anim -- leave triggered anims to whatever scripts or setup controls them.

		bool newModelIsAnimated = animator.ModelDef() ? true : false;
		bool currentlyMoving = animator.IsAnimating( gameLocal.time );
		bool usingOtherAnim = ( anim > 0 );

		if ( newModelIsAnimated && !currentlyMoving && !usingOtherAnim )
		{
			idStr startAnimname;
			if ( spawnArgs.GetString( "start_anim", "", startAnimname ) )
			{
				int startAnimnum = animator.GetAnim( startAnimname );
				if ( startAnimnum )
				{
					animator.CycleAnim( ANIMCHANNEL_ALL, startAnimnum, gameLocal.time, 0 );
				}
			}
		}
		else if ( newModelIsAnimated && !currentlyMoving && usingOtherAnim )
		{
			// If we get to this point, we have a new animated mesh that doesn't support any ongoing
			// animation and that isn't using its start animation. It won't be visible uness we set up its joints 
			// to the first frame of the animation, so do that
			animator.SetFrame( ANIMCHANNEL_ALL, anim, 1, gameLocal.time, 0 );
		}
	}
}

/*
================
idStaticEntity::ReapplyDecals

Replace decals on func statics after a LOD switch or savegame load -- SteveL #3817
================
*/
void idStaticEntity::ReapplyDecals()
{
	if ( modelDefHandle == -1 )
	{
		return;	// Might happen if model gets hidden again immediately after being shown, before the next Think().
	}

	gameRenderWorld->RemoveDecals( modelDefHandle );
	std::list<SDecalInfo>::iterator di = decals_list.begin();

	while ( di != decals_list.end() )
	{
		const idMaterial* material = declManager->FindMaterial( di->decal, false );
		if ( material && di->overlay_joint == INVALID_JOINT ) // Otherwise, this isn't a valid static decal.
		{
			int duration = material->GetDecalInfo().stayTime + material->GetDecalInfo().fadeTime;
			if ( di->decal_starttime + duration < gameLocal.time )
			{
				// Decal is already timed out. Delete it and move on.
				di = decals_list.erase( di );
				continue;
			}
			gameLocal.ProjectDecal( di->origin, di->dir, di->decal_depth, di->decal_parallel, di->size, di->decal, di->decal_angle, this, false, di->decal_starttime );
		}
		++di;
	}
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
	GetPhysics()->DisableClip();

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

	ActivateTargets( activator.GetEntity() );
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
idAnimated::Event_Footstep
===============
*/
void idAnimated::Event_Footstep( void ) {
	StartSound( "snd_footstep", SND_CHANNEL_BODY, 0, false, NULL );
}

/*
=====================
idAnimated::Event_LaunchMissilesUpdate
=====================
*/
void idAnimated::Event_LaunchMissilesUpdate( int launchjoint, int targetjoint, int numshots, int framedelay ) {
	idVec3			launchPos;
	idVec3			targetPos;
	idMat3			axis;
	idVec3			dir;
	idEntity *		ent;
	idProjectile *	projectile;
	const idDict *	projectileDef;
	const char *	projectilename;

	projectilename = spawnArgs.GetString( "projectilename" );
	projectileDef = gameLocal.FindEntityDefDict( projectilename, false );
	if ( !projectileDef ) {
		gameLocal.Warning( "idAnimated '%s' at (%s): 'launchMissiles' called with unknown projectile '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), projectilename );
		return;
	}

	StartSound( "snd_missile", SND_CHANNEL_WEAPON, 0, false, NULL );

	animator.GetJointTransform( ( jointHandle_t )launchjoint, gameLocal.time, launchPos, axis );
	launchPos = renderEntity.origin + launchPos * renderEntity.axis;
	
	animator.GetJointTransform( ( jointHandle_t )targetjoint, gameLocal.time, targetPos, axis );
	targetPos = renderEntity.origin + targetPos * renderEntity.axis;

	dir = targetPos - launchPos;
	dir.Normalize();

	gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
	if ( !ent || !ent->IsType( idProjectile::Type ) ) {
		gameLocal.Error( "idAnimated '%s' at (%s): in 'launchMissiles' call '%s' is not an idProjectile", name.c_str(), GetPhysics()->GetOrigin().ToString(0), projectilename );
	}
	projectile = ( idProjectile * )ent;
	projectile->Create( this, launchPos, dir );
	projectile->Launch( launchPos, dir, vec3_origin );

	if ( numshots > 0 ) {
		PostEventMS( &EV_LaunchMissilesUpdate, FRAME2MS( framedelay ), launchjoint, targetjoint, numshots - 1, framedelay );
	}
}

/*
=====================
idAnimated::Event_LaunchMissiles
=====================
*/
void idAnimated::Event_LaunchMissiles( const char *projectilename, const char *sound, const char *launchjoint, const char *targetjoint, int numshots, int framedelay ) {
	const idDict *	projectileDef;
	jointHandle_t	launch;
	jointHandle_t	target;

	projectileDef = gameLocal.FindEntityDefDict( projectilename, false );
	if ( !projectileDef ) {
		gameLocal.Warning( "idAnimated '%s' at (%s): unknown projectile '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), projectilename );
		return;
	}

	launch = animator.GetJointHandle( launchjoint );
	if ( launch == INVALID_JOINT ) {
		gameLocal.Warning( "idAnimated '%s' at (%s): unknown launch joint '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), launchjoint );
		gameLocal.Error( "Unknown joint '%s'", launchjoint );
	}

	target = animator.GetJointHandle( targetjoint );
	if ( target == INVALID_JOINT ) {
		gameLocal.Warning( "idAnimated '%s' at (%s): unknown target joint '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), targetjoint );
	}

	spawnArgs.Set( "projectilename", projectilename );
	spawnArgs.Set( "missilesound", sound );

	CancelEvents( &EV_LaunchMissilesUpdate );
	ProcessEvent( &EV_LaunchMissilesUpdate, launch, target, numshots - 1, framedelay );
}


/*
===============================================================================

	idStaticEntity

	Some static entities may be optimized into inline geometry by dmap

===============================================================================
*/

CLASS_DECLARATION( idEntity, idStaticEntity )
	EVENT( EV_Activate,				idStaticEntity::Event_Activate )
END_CLASS

/*
===============
idStaticEntity::idStaticEntity
===============
*/
idStaticEntity::idStaticEntity( void ) {
	spawnTime = 0;
	active = false;
	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart = 0;
	fadeEnd	= 0;
	runGui = false;
}

/*
===============
idStaticEntity::Save
===============
*/
void idStaticEntity::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( spawnTime );
	savefile->WriteBool( active );
	savefile->WriteVec4( fadeFrom );
	savefile->WriteVec4( fadeTo );
	savefile->WriteInt( fadeStart );
	savefile->WriteInt( fadeEnd );
	savefile->WriteBool( runGui );
}

/*
===============
idStaticEntity::Restore
===============
*/
void idStaticEntity::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( spawnTime );
	savefile->ReadBool( active );
	savefile->ReadVec4( fadeFrom );
	savefile->ReadVec4( fadeTo );
	savefile->ReadInt( fadeStart );
	savefile->ReadInt( fadeEnd );
	savefile->ReadBool( runGui );
}

/*
===============
idStaticEntity::Spawn
===============
*/
void idStaticEntity::Spawn( void ) {
	bool solid;

	// an inline static model will not do anything at all
	if ( spawnArgs.GetBool( "inline" ) || gameLocal.world->spawnArgs.GetBool( "inlineAllStatics" ) ) {
		Hide();
		return;
	}

	solid = spawnArgs.GetBool( "solid" );

	// ishtvan fix : Let clearing contents happen naturally on Hide instead of
	// checking hidden here and clearing contents prematurely
	if ( solid ) 
	{
		GetPhysics()->SetContents( CONTENTS_SOLID | CONTENTS_OPAQUE );
		if( m_CustomContents != -1 )
			GetPhysics()->SetContents( m_CustomContents );
	} 
	else
	{
		GetPhysics()->SetContents( 0 );
	}
	// SR CONTENTS_RESONSE FIX
	if( m_StimResponseColl->HasResponse() )
		GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );

	spawnTime = gameLocal.time;
	active = false;

	idStr model = spawnArgs.GetString( "model" );
	if ( model.Find( ".prt" ) >= 0 ) {

		// we want the parametric particles out of sync with each other
		renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = gameLocal.random.RandomInt( 32767 );
	}

	fadeFrom.Set( 1, 1, 1, 1 );
	fadeTo.Set( 1, 1, 1, 1 );
	fadeStart = 0;
	fadeEnd	= 0;

	// NOTE: this should be used very rarely because it is expensive
	runGui = spawnArgs.GetBool( "runGui" );
	if ( runGui ) {
		BecomeActive( TH_THINK );
	}
}

/*
================
idStaticEntity::ShowEditingDialog
================
*/
void idStaticEntity::ShowEditingDialog( void ) {
	common->InitTool( EDITOR_PARTICLE, &spawnArgs );
}

/*
================
idStaticEntity::Think
================
*/
void idStaticEntity::Think( void ) 
{
	// will also do LOD thinking:
	idEntity::Think();

	if ( thinkFlags & TH_THINK ) 
	{
		if ( runGui && renderEntity.gui[0] ) 
		{
			idPlayer *player = gameLocal.GetLocalPlayer();
			if ( player ) 
			{
				renderEntity.gui[0]->StateChanged( gameLocal.time, true );
				if ( renderEntity.gui[1] ) {
					renderEntity.gui[1]->StateChanged( gameLocal.time, true );
				}
				if ( renderEntity.gui[2] ) {
					renderEntity.gui[2]->StateChanged( gameLocal.time, true );
				}
			}
		}
		if ( fadeEnd > 0 ) 
		{
			idVec4 color;
			if ( gameLocal.time < fadeEnd ) {
				color.Lerp( fadeFrom, fadeTo, ( float )( gameLocal.time - fadeStart ) / ( float )( fadeEnd - fadeStart ) );
			} else {
				color = fadeTo;
				fadeEnd = 0;
				BecomeInactive( TH_THINK );
			}
			SetColor( color );
		}
	}
}

/*
================
idStaticEntity::Fade
================
*/
void idStaticEntity::Fade( const idVec4 &to, float fadeTime ) {
	GetColor( fadeFrom );
	fadeTo = to;
	fadeStart = gameLocal.time;
	fadeEnd = gameLocal.time + SEC2MS( fadeTime );
	BecomeActive( TH_THINK );
}

/*
================
idStaticEntity::Hide
================
*/
void idStaticEntity::Hide( void ) {
	idEntity::Hide();
	GetPhysics()->SetContents( 0 );
}

/*
================
idStaticEntity::Show
================
*/
void idStaticEntity::Show( void ) {
	idEntity::Show();
	GetPhysics()->SetContents( m_preHideContents );
}

/*
================
idStaticEntity::Event_Activate
================
*/
void idStaticEntity::Event_Activate( idEntity *activator ) {

	spawnTime = gameLocal.time;
	active = !active;

	const idKeyValue *kv = spawnArgs.FindKey( "hide" );
	if ( kv ) {
		if ( IsHidden() ) {
			Show();
		} else {
			Hide();
		}
	}

	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( spawnTime );
	renderEntity.shaderParms[5] = active;
	// this change should be a good thing, it will automatically turn on 
	// lights etc.. when triggered so that does not have to be specifically done
	// with trigger parms.. it MIGHT break things so need to keep an eye on it
	renderEntity.shaderParms[ SHADERPARM_MODE ] = ( renderEntity.shaderParms[ SHADERPARM_MODE ] ) ?  0.0f : 1.0f;
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idStaticEntity::WriteToSnapshot
================
*/
void idStaticEntity::WriteToSnapshot( idBitMsgDelta &msg ) const {
	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
	WriteColorToSnapshot( msg );
	WriteGUIToSnapshot( msg );
	msg.WriteBits( IsHidden()?1:0, 1 );
}

/*
================
idStaticEntity::ReadFromSnapshot
================
*/
void idStaticEntity::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	bool hidden;

	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );
	ReadColorFromSnapshot( msg );
	ReadGUIFromSnapshot( msg );
	hidden = msg.ReadBits( 1 ) == 1;
	if ( hidden != IsHidden() ) {
		if ( hidden ) {
			Hide();
		} else {
			Show();
		}
	}
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}


/*
===============================================================================

idFuncSmoke

===============================================================================
*/

const idEventDef EV_SetSmoke( "setSmoke", EventArgs('s', "particleDef", ""), EV_RETURNS_VOID, "Changes the smoke particle of a func_smoke." );

CLASS_DECLARATION( idEntity, idFuncSmoke )
EVENT( EV_Activate,				idFuncSmoke::Event_Activate )
EVENT( EV_SetSmoke,				idFuncSmoke::Event_SetSmoke )
END_CLASS

/*
===============
idFuncSmoke::idFuncSmoke
===============
*/
idFuncSmoke::idFuncSmoke() {
	smokeTime = 0;
	smoke = NULL;
	restart = false;
}

/*
===============
idFuncSmoke::Save
===============
*/
void idFuncSmoke::Save(	idSaveGame *savefile ) const {
	savefile->WriteInt( smokeTime );
	savefile->WriteParticle( smoke );
	savefile->WriteBool( restart );
}

/*
===============
idFuncSmoke::Restore
===============
*/
void idFuncSmoke::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( smokeTime );
	savefile->ReadParticle( smoke );
	savefile->ReadBool( restart );
}

/*
===============
idFuncSmoke::Spawn
===============
*/
void idFuncSmoke::Spawn( void ) {
	const char *smokeName = spawnArgs.GetString( "smoke" );
	if ( *smokeName != '\0' ) {
		smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
	} else {
		smoke = NULL;
	}
	if ( spawnArgs.GetBool( "start_off" ) ) {
		smokeTime = 0;
		restart = false;
	} else if ( smoke ) {
		smokeTime = gameLocal.time;
		BecomeActive( TH_UPDATEPARTICLES );
		restart = true;
	}
	GetPhysics()->SetContents( 0 );
	
}

/*
================
idFuncSmoke::Event_Activate
================
*/
void idFuncSmoke::Event_Activate( idEntity *activator ) {
	if ( thinkFlags & TH_UPDATEPARTICLES ) {
		restart = false;
		return;
	} else {
		BecomeActive( TH_UPDATEPARTICLES );
		restart = true;
		smokeTime = gameLocal.time;
	}
}

/*
================
idFuncSmoke::Event_SetSmoke
================
*/
void idFuncSmoke::Event_SetSmoke( const char *particleDef ) {
	if ( *particleDef != '\0' ) {
		smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particleDef ) );
	} else {
		smoke = NULL;
	}
}

/*
===============
idFuncSmoke::Think
================
*/
void idFuncSmoke::Think( void ) {

	// if we are completely closed off from the player, don't do anything at all
	if ( CheckDormant() || smoke == NULL || smokeTime == -1 ) {
		return;
	}

	if ( ( thinkFlags & TH_UPDATEPARTICLES) && !fl.hidden ) {
		if ( !gameLocal.smokeParticles->EmitSmoke( smoke, smokeTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() ) ) {
			if ( restart ) {
				smokeTime = gameLocal.time;
			} else {
				smokeTime = 0;
				BecomeInactive( TH_UPDATEPARTICLES );
			}
		}
	}
}


/*
===============================================================================

	idTextEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, idTextEntity )
END_CLASS

/*
================
idTextEntity::Spawn
================
*/
void idTextEntity::Spawn( void )
{
	// these are cached as they are used each frame
	text = spawnArgs.GetString( "text" );
	playerOriented = spawnArgs.GetBool( "playerOriented" );

	// stgatilov: fix \n to make it possible to show multiline text
	text.Replace("\\n", "\n");

	// grayman #3042 - this used to only start thinking if the "developer"
	// boolean was set. I couldn't get that to work at map start, and changing
	// it while playing wouldn't get the text to show, so I changed how it was done.
	force = spawnArgs.GetBool( "force" );
	BecomeActive(TH_THINK);
}

/*
================
idTextEntity::Save
================
*/
void idTextEntity::Save( idSaveGame *savefile ) const
{
	savefile->WriteString( text );
	savefile->WriteBool( playerOriented );
	savefile->WriteBool( force ); // grayman #3042
}

/*
================
idTextEntity::Restore
================
*/
void idTextEntity::Restore( idRestoreGame *savefile )
{
	savefile->ReadString( text );
	savefile->ReadBool( playerOriented );
	savefile->ReadBool( force ); // grayman #3042
}

/*
================
idTextEntity::Think
================
*/
void idTextEntity::Think( void )
{
	if ( force || com_developer.GetBool() ) // grayman #3042
	{
		gameRenderWorld->DebugText( text, GetPhysics()->GetOrigin(), 0.25, colorWhite, playerOriented ? gameLocal.GetLocalPlayer()->viewAngles.ToMat3() : GetPhysics()->GetAxis().Transpose(), 1 );
		for ( int i = 0 ; i < targets.Num() ; i++ )
		{
			if ( targets[i].GetEntity() )
			{
				gameRenderWorld->DebugArrow( colorBlue, GetPhysics()->GetOrigin(), targets[i].GetEntity()->GetPhysics()->GetOrigin(), 1 );
			}
		}
	}
}

/*
===============================================================================

	idVacuumSeperatorEntity

	Can be triggered to let vacuum through a portal (blown out window)

===============================================================================
*/

CLASS_DECLARATION( idEntity, idVacuumSeparatorEntity )
	EVENT( EV_Activate,		idVacuumSeparatorEntity::Event_Activate )
END_CLASS


/*
================
idVacuumSeparatorEntity::idVacuumSeparatorEntity
================
*/
idVacuumSeparatorEntity::idVacuumSeparatorEntity( void ) {
	portal = 0;
}

/*
================
idVacuumSeparatorEntity::Save
================
*/
void idVacuumSeparatorEntity::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( (int)portal );
	savefile->WriteInt( gameRenderWorld->GetPortalState( portal ) );
}

/*
================
idVacuumSeparatorEntity::Restore
================
*/
void idVacuumSeparatorEntity::Restore( idRestoreGame *savefile ) {
	int state;

	savefile->ReadInt( (int &)portal );
	savefile->ReadInt( state );

	gameLocal.SetPortalState( portal, state );
}

/*
================
idVacuumSeparatorEntity::Spawn
================
*/
void idVacuumSeparatorEntity::Spawn() {
	idBounds b;

	b = idPortalEntity::GetBounds( spawnArgs.GetVector( "origin" ) );
	portal = gameRenderWorld->FindPortal( b );
	if ( !portal ) {
		gameLocal.Warning( "VacuumSeparator '%s' didn't contact a portal", spawnArgs.GetString( "name" ) );
		return;
	}
}

/*
================
idVacuumSeparatorEntity::Event_Activate
================
*/
void idVacuumSeparatorEntity::Event_Activate( idEntity *activator ) {
	if ( !portal ) {
		return;
	}
	gameLocal.SetPortalState( portal, PS_BLOCK_NONE );
}


/*
===============================================================================

idPortalEntity

===============================================================================
*/

const idEventDef EV_GetPortalHandle( "getPortalHandle", EventArgs(), 'f', "Returns the portal handle." );
const idEventDef EV_GetSoundLoss( "getSoundLoss", EventArgs(), 'f', "Returns the sound loss value (dB)." ); // grayman #3042
const idEventDef EV_SetSoundLoss( "setSoundLoss", EventArgs('f',"loss",""), EV_RETURNS_VOID, "Sets the sound loss value (dB)." ); // grayman #3042

CLASS_DECLARATION( idEntity, idPortalEntity )
	EVENT( EV_GetPortalHandle,		idPortalEntity::Event_GetPortalHandle )
	EVENT( EV_GetSoundLoss,			idPortalEntity::Event_GetSoundLoss )	// grayman #3042
	EVENT( EV_SetSoundLoss,			idPortalEntity::Event_SetSoundLoss )	// grayman #3042
	EVENT( EV_PostSpawn,			idPortalEntity::Event_PostSpawn )		// grayman #3042
END_CLASS

idPortalEntity::idPortalEntity() {
	m_SoundLoss = 0.0f;
	m_Portal = -1;
	m_Entity = nullptr;
	m_EntityLocationDone = false;
}
idPortalEntity::~idPortalEntity() {}

/*
================
idPortalEntity::GetBounds
================
*/
idBounds idPortalEntity::GetBounds( const idVec3 &origin ) {
	return idBounds( origin ).Expand( 16 );
}

/*
================
idPortalEntity::Spawn
================
*/
void idPortalEntity::Spawn()
{
	idBounds b = GetBounds( spawnArgs.GetVector( "origin" ) );
	m_Portal = gameRenderWorld->FindPortal( b );

	if ( !m_Portal ) 
	{
		gameLocal.Warning( "idPortalEntity '%s' didn't contact a portal", GetName() );
		return;
	}

	m_Entity = NULL;
	m_EntityLocationDone = false;

	// store the sound loss for the associated portal
	m_SoundLoss = idMath::Fabs( spawnArgs.GetFloat("sound_loss", "0.0") );
}

// grayman #3042 - Doors and brittle fractures need to know about any portal entities they share a portal with.
// This is for use with sound loss, since portal entity sound loss needs to be added to
// sound losses defined by the door or brittle fracture.

void idPortalEntity::Event_PostSpawn( void )
{
	// Find a door or brittle fracture touching me and give it my sound_loss value

	if ( !m_Portal )
	{
		return; // no portal, so there's nothing to do
	}

	// While it might be tempting to skip the rest of this if m_SoundLoss is zero,
	// we can't, because the loss value can be changed dynamically, and we need to
	// handle when it's set to zero that way.

	// Search for a door or brittle fracture that shares my portal. After the first
	// time this is attempted, set m_EntityLocationDone to TRUE to indicate
	// we don't need to search again if the sound loss value is dynamically set
	// by a script later.

	// grayman #3042 - Store booleans for whether the sound loss
	// applies only to AI, only to the Player, or both, or neither.
	// grayman #3455 - Read these spawnargs as needed, instead of storing
	// them in the entity.
	bool applyToAI = spawnArgs.GetBool("apply_loss_to_AI", "1");
	bool applyToPlayer = spawnArgs.GetBool("apply_loss_to_Player", "1");

	if ( !m_EntityLocationDone )
	{
		idBounds b = GetBounds( spawnArgs.GetVector( "origin" ) );
		idClip_ClipModelList clipModelList;
		int numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( b, CONTENTS_SOLID, clipModelList );

		for ( int i = 0 ; i < numListedClipModels ; i++ ) 
		{
			idClipModel* clipModel = clipModelList[i];
			idEntity* obEnt = clipModel->GetEntity();

			if ( obEnt == NULL )
			{
				continue;
			}

			if ( obEnt == this ) // skip myself
			{
				continue;
			}

			if ( obEnt->IsType(CFrobDoor::Type) || obEnt->IsType(idBrittleFracture::Type) )
			{
				// Check the visportal touching the found entity. If it's the same as ours, we're done.
				int testPortal = gameRenderWorld->FindPortal(obEnt->GetPhysics()->GetAbsBounds());

				if ( ( testPortal == m_Portal ) && ( testPortal != 0 ) )
				{
					DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("idPortalEntity::Event_PostSpawn - touching %s\r", obEnt->name.c_str());
					m_Entity = obEnt;
					break; // Assume only one door or brittle fracture
				}
			}
		}

		m_EntityLocationDone = true;
	}

	// If we have an entity we share the portal with, set its base sound loss level to ours

	if ( m_Entity != NULL )
	{
		float m_SoundLossAI = applyToAI ? m_SoundLoss : 0.0f;
		float m_SoundLossPlayer = applyToPlayer ? m_SoundLoss : 0.0f;

		if ( m_Entity->IsType(CFrobDoor::Type) )
		{
			CFrobDoor *door = static_cast<CFrobDoor*>(m_Entity);
			door->SetLossBase( m_SoundLossAI, m_SoundLossPlayer );
			door->UpdateSoundLoss(); // tell the door to pass the total loss to the portal

			// If this door is part of a double door, tell the other door about the portal entity's sound loss.
			// He won't know about the portal entity if it's not touching him.

			CFrobDoor* doubleDoor = door->GetDoubleDoor();
			if ( doubleDoor )
			{
				doubleDoor->SetLossBase( m_SoundLossAI, m_SoundLossPlayer );
			}
		}
		else if ( m_Entity->IsType(idBrittleFracture::Type) )
		{
			idBrittleFracture *bf = static_cast<idBrittleFracture*>(m_Entity);
			bf->SetLossBase( m_SoundLossAI, m_SoundLossPlayer );
			bf->UpdateSoundLoss(); // tell the brittle fracture to pass the total loss to the portal
		}
	}
	else // place our loss value on the portal directly
	{
		if ( applyToAI )
		{
			gameLocal.m_sndProp->SetPortalAILoss( m_Portal, m_SoundLoss );
		}
		if ( applyToPlayer )
		{
			gameLocal.m_sndProp->SetPortalPlayerLoss( m_Portal, m_SoundLoss );
		}
	}
}

/*
================
idPortalEntity::Save

Tels: Each idPortalEntity contains the handle of the portal it
	  is connected to, so we can let it return the handle.
================
*/
void idPortalEntity::Save( idSaveGame *savefile ) const
{
	savefile->WriteFloat( m_SoundLoss );
	savefile->WriteInt( m_Portal );
	savefile->WriteObject(m_Entity);				// grayman #3042
	savefile->WriteBool(m_EntityLocationDone);	// grayman #3042
}

void idPortalEntity::Restore( idRestoreGame *savefile )
{
	savefile->ReadFloat( m_SoundLoss );
	savefile->ReadInt( m_Portal );
	savefile->ReadObject(reinterpret_cast<idClass*&>(m_Entity));		// grayman #3042
	savefile->ReadBool(m_EntityLocationDone);						// grayman #3042
}

qhandle_t idPortalEntity::GetPortalHandle( void ) const
{
	return m_Portal;
}

// grayman #3042 - retrieve sound loss

float idPortalEntity::GetSoundLoss( void ) const
{
	return m_SoundLoss;
}

void idPortalEntity::SetSoundLoss( const float loss )
{
	m_SoundLoss = loss;
	Event_PostSpawn();
}

void idPortalEntity::Event_GetPortalHandle( void )
{
	if ( !m_Portal )
	{
		return idThread::ReturnFloat( -1.0f );
	}
	idThread::ReturnFloat( (float) m_Portal );
}

void idPortalEntity::Event_GetSoundLoss( void ) // grayman #3042
{
	idThread::ReturnFloat( m_SoundLoss );
}

void idPortalEntity::Event_SetSoundLoss( const float loss ) // grayman #3042
{
	SetSoundLoss(loss);
}

/*
===============================================================================

idLocationSeparatorEntity

===============================================================================
*/

CLASS_DECLARATION( idPortalEntity, idLocationSeparatorEntity )
END_CLASS

/*
================
idLocationSeparatorEntity::Spawn
================
*/
void idLocationSeparatorEntity::Spawn() 
{
	if ( !m_Portal ) 
	{
		return;
	}

	// grayman #3399 - OR in the PS_BLOCK_LOCATION bit so other bits aren't affected
	int blockingBits = gameRenderWorld->GetPortalState( m_Portal );
	blockingBits |= PS_BLOCK_LOCATION;

	gameLocal.SetPortalState( m_Portal, blockingBits );

	// grayman #3042 - Schedule a post-spawn event to search for touching doors or brittle fractures.
	// This event has to occur after the CFrobDoor PostSpawn() event, because
	// that's where double doors learn about each other. That delay is 16,
	// so make this one 18.
	PostEventMS( &EV_PostSpawn, 18 );
}

// grayman #3042 - add idPortalSettingsEntity. This entity is like a location
// separator, except that it doesn't mark location boundaries.

/*
===============================================================================

idPortalSettingsEntity

===============================================================================
*/

CLASS_DECLARATION( idPortalEntity, idPortalSettingsEntity )
END_CLASS

/*
================
idPortalSettingsEntity::Spawn
================
*/
void idPortalSettingsEntity::Spawn() 
{
	if ( !m_Portal ) 
	{
		return;
	}

	// Schedule a post-spawn event to search for touching doors and brittle fractures.
	// This event has to occur after the CFrobDoor PostSpawn() event, because
	// that's where double doors learn about each other. That delay is 16,
	// so make this one 18.
	PostEventMS( &EV_PostSpawn, 18 );
}

/*
===============================================================================

	idVacuumEntity

	Levels should only have a single vacuum entity.

===============================================================================
*/

CLASS_DECLARATION( idEntity, idVacuumEntity )
END_CLASS

/*
================
idVacuumEntity::Spawn
================
*/
void idVacuumEntity::Spawn() {
	if ( gameLocal.vacuumAreaNum != -1 ) {
		gameLocal.Warning( "idVacuumEntity::Spawn: multiple idVacuumEntity in level" );
		return;
	}

	idVec3 org = spawnArgs.GetVector( "origin" );

	gameLocal.vacuumAreaNum = gameRenderWorld->GetAreaAtPoint( org );
}


/*
===============================================================================

idLocationEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, idLocationEntity )
END_CLASS

/*
======================
idLocationEntity::idLocationEntity
======================
*/
idLocationEntity::idLocationEntity( void )
{
	m_SndLossMult = 1.0;
	m_SndVolMod = 0.0;
	m_ObjectiveGroup.Clear();
}

/*
======================
idLocationEntity::Spawn
======================
*/
void idLocationEntity::Spawn() 
{
	idStr realName;

	// this just holds dict information

	// if "location" not already set, use the entity name.
	if ( !spawnArgs.GetString( "location", "", realName ) ) 
	{
		spawnArgs.Set( "location", name );
	}

	m_SndLossMult = idMath::Fabs( spawnArgs.GetFloat("sound_loss_mult", "1.0") );
	m_SndVolMod = spawnArgs.GetFloat( "sound_vol_offset", "0.0" );
	m_ObjectiveGroup = spawnArgs.GetString( "objective_group", "" );
}

/*
======================
idLocationEntity::Save
======================
*/
void idLocationEntity::Save( idSaveGame *savefile ) const
{
	savefile->WriteFloat( m_SndLossMult );
	savefile->WriteFloat( m_SndVolMod );
	savefile->WriteString( m_ObjectiveGroup );
}

/*
======================
idLocationEntity::Restore
======================
*/
void idLocationEntity::Restore( idRestoreGame *savefile )
{
	savefile->ReadFloat( m_SndLossMult );
	savefile->ReadFloat( m_SndVolMod );
	savefile->ReadString( m_ObjectiveGroup );
}

/*
======================
idLocationEntity::GetLocation
======================
*/
const char *idLocationEntity::GetLocation( void ) const {
	return spawnArgs.GetString( "location" );
}

/*
===============================================================================

	idBeam

===============================================================================
*/

CLASS_DECLARATION( idEntity, idBeam )
	EVENT( EV_PostSpawn,			idBeam::Event_MatchTarget )
	EVENT( EV_Activate,				idBeam::Event_Activate )
END_CLASS

/*
===============
idBeam::idBeam
===============
*/
idBeam::idBeam() {
	target = NULL;
	master = NULL;
}

/*
===============
idBeam::Save
===============
*/
void idBeam::Save( idSaveGame *savefile ) const {
	target.Save( savefile );
	master.Save( savefile );
}

/*
===============
idBeam::Restore
===============
*/
void idBeam::Restore( idRestoreGame *savefile ) {
	target.Restore( savefile );
	master.Restore( savefile );
}

/*
===============
idBeam::Spawn
===============
*/
void idBeam::Spawn( void ) {
	float width;

	if ( spawnArgs.GetFloat( "width", "0", width ) ) {
		renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ] = width;
	}

	SetModel( "_BEAM" );
	Hide();
	PostEventMS( &EV_PostSpawn, 0 );
}

/*
================
idBeam::Think
================
*/
void idBeam::Think( void ) {
	idBeam *masterEnt;

	if ( !IsHidden() && !target.GetEntity() ) {
		// hide if our target is removed
		Hide();
	}

	RunPhysics();

	masterEnt = master.GetEntity();
	if ( masterEnt ) {
		const idVec3 &origin = GetPhysics()->GetOrigin();
		masterEnt->SetBeamTarget( origin );
	}
	Present();
}

/*
================
idBeam::SetMaster
================
*/
void idBeam::SetMaster( idBeam *masterbeam ) {
	master = masterbeam;
}

/*
================
idBeam::SetBeamTarget
================
*/
void idBeam::SetBeamTarget( const idVec3 &origin ) {
	if ( ( renderEntity.shaderParms[ SHADERPARM_BEAM_END_X ] != origin.x ) || ( renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ] != origin.y ) || ( renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ] != origin.z ) ) {
		renderEntity.shaderParms[ SHADERPARM_BEAM_END_X ] = origin.x;
		renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ] = origin.y;
		renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ] = origin.z;
		UpdateVisuals();
	}
}

/*
================
idBeam::Show
================
*/
void idBeam::Show( void ) {
	idBeam *targetEnt;

	idEntity::Show();

	targetEnt = target.GetEntity();
	if ( targetEnt ) {
		const idVec3 &origin = targetEnt->GetPhysics()->GetOrigin();
		SetBeamTarget( origin );
	}
}

/*
================
idBeam::Event_MatchTarget
================
*/
void idBeam::Event_MatchTarget( void ) {
	int i;
	idEntity *targetEnt;
	idBeam *targetBeam;

	if ( !targets.Num() ) {
		return;
	}

	targetBeam = NULL;
	for( i = 0; i < targets.Num(); i++ ) {
		targetEnt = targets[ i ].GetEntity();
		if ( targetEnt && targetEnt->IsType( idBeam::Type ) ) {
			targetBeam = static_cast<idBeam *>( targetEnt );
			break;
		}
	}

	if ( !targetBeam ) {
		gameLocal.Error( "Could not find valid beam target for '%s'", name.c_str() );
	}

	target = targetBeam;
	targetBeam->SetMaster( this );
	if ( !spawnArgs.GetBool( "start_off" ) ) {
		Show();
	}
}

/*
================
idBeam::Event_Activate
================
*/
void idBeam::Event_Activate( idEntity *activator ) {
	if ( IsHidden() ) {
		Show();
	} else {
		Hide();		
	}
}

/*
================
idBeam::WriteToSnapshot
================
*/
void idBeam::WriteToSnapshot( idBitMsgDelta &msg ) const {
	GetPhysics()->WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
	WriteColorToSnapshot( msg );
	msg.WriteFloat( renderEntity.shaderParms[SHADERPARM_BEAM_END_X] );
	msg.WriteFloat( renderEntity.shaderParms[SHADERPARM_BEAM_END_Y] );
	msg.WriteFloat( renderEntity.shaderParms[SHADERPARM_BEAM_END_Z] );
}

/*
================
idBeam::ReadFromSnapshot
================
*/
void idBeam::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	GetPhysics()->ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );
	ReadColorFromSnapshot( msg );
	renderEntity.shaderParms[SHADERPARM_BEAM_END_X] = msg.ReadFloat();
	renderEntity.shaderParms[SHADERPARM_BEAM_END_Y] = msg.ReadFloat();
	renderEntity.shaderParms[SHADERPARM_BEAM_END_Z] = msg.ReadFloat();
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}


/*
===============================================================================

	idLiquid

===============================================================================
*/

#ifndef MOD_WATERPHYSICS
CLASS_DECLARATION( idEntity, idLiquid )
	EVENT( EV_Touch,			idLiquid::Event_Touch )
END_CLASS

/*
================
idLiquid::Save
================
*/
void idLiquid::Save( idSaveGame *savefile ) const {
	// Nothing to save
}

/*
================
idLiquid::Restore
================
*/
void idLiquid::Restore( idRestoreGame *savefile ) {
	//FIXME: NO!
	Spawn();
}

/*
================
idLiquid::Spawn
================
*/
void idLiquid::Spawn() {
/*
	model = dynamic_cast<idRenderModelLiquid *>( renderEntity.hModel );
	if ( !model ) {
		gameLocal.Error( "Entity '%s' must have liquid model", name.c_str() );
	}
	model->Reset();
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
*/
}

/*
================
idLiquid::Event_Touch
================
*/
void idLiquid::Event_Touch( idEntity *other, trace_t *trace ) {
	// FIXME: for QuakeCon
/*
	idVec3 pos;

	pos = other->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	model->IntersectBounds( other->GetPhysics()->GetBounds().Translate( pos ), -10.0f );
*/
}
#endif

/*
===============================================================================

	idShaking

===============================================================================
*/

CLASS_DECLARATION( idEntity, idShaking )
	EVENT( EV_Activate,				idShaking::Event_Activate )
END_CLASS

/*
===============
idShaking::idShaking
===============
*/
idShaking::idShaking() {
	active = false;
}

/*
===============
idShaking::Save
===============
*/
void idShaking::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( active );
	savefile->WriteStaticObject( physicsObj );
}

/*
===============
idShaking::Restore
===============
*/
void idShaking::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( active );
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );
}

/*
===============
idShaking::Spawn
===============
*/
void idShaking::Spawn( void ) {
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetClipMask( MASK_SOLID );
	SetPhysics( &physicsObj );
	
	active = false;
	if ( !spawnArgs.GetBool( "start_off" ) ) {
		BeginShaking();
	}
}

/*
================
idShaking::BeginShaking
================
*/
void idShaking::BeginShaking( void ) {
	int			phase;
	idAngles	shake;
	int			period;

	active = true;
	phase = gameLocal.random.RandomInt( 1000 );
	shake = spawnArgs.GetAngles( "shake", "0.5 0.5 0.5" );
	period = static_cast<int>(spawnArgs.GetFloat( "period", "0.05" ) * 1000);
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), phase, static_cast<int>(period * 0.25f), GetPhysics()->GetAxis().ToAngles(), shake, ang_zero );
}

/*
================
idShaking::Event_Activate
================
*/
void idShaking::Event_Activate( idEntity *activator ) {
	if ( !active ) {
		BeginShaking();
	} else {
		active = false;
		physicsObj.SetAngularExtrapolation( EXTRAPOLATION_NONE, 0, 0, physicsObj.GetAxis().ToAngles(), ang_zero, ang_zero );
	}
}

/*
===============================================================================

	idEarthQuake

===============================================================================
*/

CLASS_DECLARATION( idEntity, idEarthQuake )
	EVENT( EV_Activate,				idEarthQuake::Event_Activate )
END_CLASS

/*
===============
idEarthQuake::idEarthQuake
===============
*/
idEarthQuake::idEarthQuake() {
	wait = 0.0f;
	random = 0.0f;
	nextTriggerTime = 0;
	shakeStopTime = 0;
	triggered = false;
	playerOriented = false;
	disabled = false;
	shakeTime = 0.0f;
}

/*
===============
idEarthQuake::Save
===============
*/
void idEarthQuake::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( nextTriggerTime );
	savefile->WriteInt( shakeStopTime );
	savefile->WriteFloat( wait );
	savefile->WriteFloat( random );
	savefile->WriteBool( triggered );
	savefile->WriteBool( playerOriented );
	savefile->WriteBool( disabled );
	savefile->WriteFloat( shakeTime );
}

/*
===============
idEarthQuake::Restore
===============
*/
void idEarthQuake::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( nextTriggerTime );
	savefile->ReadInt( shakeStopTime );
	savefile->ReadFloat( wait );
	savefile->ReadFloat( random );
	savefile->ReadBool( triggered );
	savefile->ReadBool( playerOriented );
	savefile->ReadBool( disabled );
	savefile->ReadFloat( shakeTime );

	if ( shakeStopTime > gameLocal.time ) {
		BecomeActive( TH_THINK );
	}
}

/*
===============
idEarthQuake::Spawn
===============
*/
void idEarthQuake::Spawn( void ) {
	nextTriggerTime = 0;
	shakeStopTime = 0;
	wait = spawnArgs.GetFloat( "wait", "15" );
	random = spawnArgs.GetFloat( "random", "5" );
	triggered = spawnArgs.GetBool( "triggered" );
	playerOriented = spawnArgs.GetBool( "playerOriented" );
	disabled = false;
	shakeTime = spawnArgs.GetFloat( "shakeTime", "0" );

	if ( !triggered ){
		PostEventSec( &EV_Activate, spawnArgs.GetFloat( "wait" ), this );
	}
	BecomeInactive( TH_THINK );
}

/*
================
idEarthQuake::Event_Activate
================
*/
void idEarthQuake::Event_Activate( idEntity *activator ) {
	
	if ( nextTriggerTime > gameLocal.time ) {
		return;
	}

	if ( disabled && activator == this ) {
		return;
	}

	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		return;
	}

	nextTriggerTime = 0;

	if ( !triggered && activator != this ){
		// if we are not triggered ( i.e. random ), disable or enable
		disabled ^= 1;
		if (disabled) {
			return;
		} else {
			PostEventSec( &EV_Activate, wait + random * gameLocal.random.CRandomFloat(), this );
		}
	}

	ActivateTargets( activator );

	const idSoundShader *shader = declManager->FindSound( spawnArgs.GetString( "snd_quake" ) );
	if ( playerOriented ) {
		player->StartSoundShader( shader, SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
	} else {
		StartSoundShader( shader, SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
	}

	if ( shakeTime > 0.0f ) {
		shakeStopTime = gameLocal.time + SEC2MS( shakeTime );
		BecomeActive( TH_THINK );
	}

	if ( wait > 0.0f ) {
		if ( !triggered ) {
			PostEventSec( &EV_Activate, wait + random * gameLocal.random.CRandomFloat(), this );
		} else {
			nextTriggerTime = gameLocal.time + SEC2MS( wait + random * gameLocal.random.CRandomFloat() );
		}
	} else if ( shakeTime == 0.0f ) {
		PostEventMS( &EV_Remove, 0 );
	}
}


/*
===============
idEarthQuake::Think
================
*/
void idEarthQuake::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		if ( gameLocal.time > shakeStopTime ) {
			BecomeInactive( TH_THINK );
			if ( wait <= 0.0f ) {
				PostEventMS( &EV_Remove, 0 );
			}
			return;
		}
		float shakeVolume = gameSoundWorld->CurrentShakeAmplitudeForPosition( gameLocal.time, gameLocal.GetLocalPlayer()->firstPersonViewOrigin );
		gameLocal.RadiusPush( GetPhysics()->GetOrigin(), 256, 1500 * shakeVolume, this, this, 1.0f, true );
	}
	BecomeInactive( TH_UPDATEVISUALS );
}

/*
===============================================================================

	idFuncPortal

===============================================================================
*/

CLASS_DECLARATION( idEntity, idFuncPortal )
	EVENT( EV_Activate,				idFuncPortal::Event_Activate )
END_CLASS

/*
===============
idFuncPortal::idFuncPortal
===============
*/
idFuncPortal::idFuncPortal() 
{
	portal = 0;
	state = false;
	m_bDistDependent = false;
	m_Distance = 0;

	m_TimeStamp = 0;
	m_Interval = 1000;
}

/*
===============
idFuncPortal::Save
===============
*/
void idFuncPortal::Save( idSaveGame *savefile ) const 
{
	savefile->WriteInt( (int)portal );
	savefile->WriteBool( state );

	savefile->WriteBool( m_bDistDependent );
	savefile->WriteFloat( m_Distance );
	savefile->WriteInt( m_TimeStamp );
	savefile->WriteInt( m_Interval );
}

/*
===============
idFuncPortal::Restore
===============
*/
void idFuncPortal::Restore( idRestoreGame *savefile ) 
{
	savefile->ReadInt( (int &)portal );
	savefile->ReadBool( state );

	// grayman #3399 - change only the PS_BLOCK_VIEW bit so other bits aren't affected
	int blockingBits = gameRenderWorld->GetPortalState( portal );

	if ( state )
	{
		blockingBits |= PS_BLOCK_VIEW; // turn PS_BLOCK_VIEW on
	}
	else
	{
		blockingBits &= ~PS_BLOCK_VIEW; // turn PS_BLOCK_VIEW off
	}
	gameLocal.SetPortalState( portal, blockingBits );

	savefile->ReadBool( m_bDistDependent );
	savefile->ReadFloat( m_Distance );
	savefile->ReadInt( m_TimeStamp );
	savefile->ReadInt( m_Interval );
}

/*
===============
idFuncPortal::Spawn
===============
*/
void idFuncPortal::Spawn( void ) 
{
	portal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds().Expand( 32.0f ) );
	if ( portal > 0 ) 
	{
		// grayman #3399 - change only the PS_BLOCK_VIEW bit so other bits aren't affected
		int blockingBits = gameRenderWorld->GetPortalState( portal );

		state = spawnArgs.GetBool( "start_on" );
		if ( state )
		{
			blockingBits |= PS_BLOCK_VIEW; // turn PS_BLOCK_VIEW on
		}
		else
		{
			blockingBits &= ~PS_BLOCK_VIEW; // turn PS_BLOCK_VIEW off
		}
		gameLocal.SetPortalState( portal, blockingBits );
	}
	if( (m_Distance = spawnArgs.GetFloat( "portal_dist", "0.0" )) <= 0 )
	{
		return;
	}

	// distance dependent portals from this point on:
	m_bDistDependent = true;
	m_Distance *= m_Distance;
	m_Interval = (int) (1000.0f * spawnArgs.GetFloat( "distcheck_period", "1.0" ));

	// add some phase diversity to the checks so that they don't all run in one frame
	// make sure they all run on the first frame though, by initializing m_TimeStamp to
	// be at least one interval early.
	m_TimeStamp = gameLocal.time - (int) (m_Interval * (1.0f + 0.5f*gameLocal.random.RandomFloat()) );

	// only start thinking if it's distance dependent.
	BecomeActive( TH_THINK );

	return;
}

/*
================
idFuncPortal::Event_Activate
================
*/
void idFuncPortal::Event_Activate( idEntity *activator ) 
{
	if ( portal > 0 ) 
	{
		state = !state;
		// grayman #3399 - change only the PS_BLOCK_VIEW bit so other bits aren't affected
		int blockingBits = gameRenderWorld->GetPortalState( portal );

		if ( state )
		{
			blockingBits |= PS_BLOCK_VIEW; // turn PS_BLOCK_VIEW on
		}
		else
		{
			blockingBits &= ~PS_BLOCK_VIEW; // turn PS_BLOCK_VIEW off
		}
		gameLocal.SetPortalState( portal, blockingBits );
	}

	// activate our targets
	PostEventMS( &EV_ActivateTargets, 0, activator );
}

/*
================
idFuncPortal::ClosePortal
================
*/
void idFuncPortal::ClosePortal( void )
{
	if ( portal > 0 ) 
	{
		state = true;
		// grayman #3399 - change only the PS_BLOCK_VIEW bit so other bits aren't affected
		int blockingBits = gameRenderWorld->GetPortalState( portal );

		blockingBits |= PS_BLOCK_VIEW; // turn PS_BLOCK_VIEW on
		gameLocal.SetPortalState( portal, blockingBits );
	}
}

/*
================
idFuncPortal::OpenPortal
================
*/
void idFuncPortal::OpenPortal( void ) 
{
	if ( portal > 0 ) 
	{
		state = false;
		// grayman #3399 - change only the PS_BLOCK_VIEW bit so other bits aren't affected
		int blockingBits = gameRenderWorld->GetPortalState( portal );

		blockingBits &= ~PS_BLOCK_VIEW; // turn PS_BLOCK_VIEW off
		gameLocal.SetPortalState( portal, blockingBits );
	}
}

void idFuncPortal::Think( void )
{
	extern idCVar r_lockView;
	idVec3 delta;
	bool bWithinDist;

	if( !m_bDistDependent )
		goto Quit;

	if( (gameLocal.time - m_TimeStamp) < m_Interval )
		goto Quit;

	if ( r_lockView.GetInteger() != 0 )
		goto Quit;

	m_TimeStamp = gameLocal.time;
	bWithinDist = false;

	delta = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	delta -= GetPhysics()->GetOrigin();

	bWithinDist = (delta.LengthSqr() < m_Distance);

	if( (!state && !bWithinDist) || (state && bWithinDist) )
	{
		// toggle portal and trigger targets
		Event_Activate( gameLocal.GetLocalPlayer() );
	}

Quit:
	idEntity::Think();
	return;
}

/*
===============================================================================

	idFuncAASPortal

===============================================================================
*/

CLASS_DECLARATION( idEntity, idFuncAASPortal )
	EVENT( EV_Activate,				idFuncAASPortal::Event_Activate )
END_CLASS

/*
===============
idFuncAASPortal::idFuncAASPortal
===============
*/
idFuncAASPortal::idFuncAASPortal() {
	state = false;
}

/*
===============
idFuncAASPortal::Save
===============
*/
void idFuncAASPortal::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( state );
}

/*
===============
idFuncAASPortal::Restore
===============
*/
void idFuncAASPortal::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( state );
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_CLUSTERPORTAL, state );
}

/*
===============
idFuncAASPortal::Spawn
===============
*/
void idFuncAASPortal::Spawn( void ) {
	state = spawnArgs.GetBool( "start_on" );
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_CLUSTERPORTAL, state );
}

/*
================
idFuncAASPortal::Event_Activate
================
*/
void idFuncAASPortal::Event_Activate( idEntity *activator ) {
	state ^= 1;
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_CLUSTERPORTAL, state );
}

/*
===============================================================================

	idFuncAASObstacle

===============================================================================
*/

CLASS_DECLARATION( idEntity, idFuncAASObstacle )
	EVENT( EV_Activate,				idFuncAASObstacle::Event_Activate )
END_CLASS

/*
===============
idFuncAASObstacle::idFuncAASObstacle
===============
*/
idFuncAASObstacle::idFuncAASObstacle() {
	state = false;
}

/*
===============
idFuncAASObstacle::Save
===============
*/
void idFuncAASObstacle::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( state );
}

/*
===============
idFuncAASObstacle::Restore
===============
*/
void idFuncAASObstacle::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( state );
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_OBSTACLE, state );
}

/*
===============
idFuncAASObstacle::Spawn
===============
*/
void idFuncAASObstacle::Spawn( void ) {
	state = spawnArgs.GetBool( "start_on" );
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_OBSTACLE, state );
	if (cv_ai_show_aasfuncobstacle_state.GetBool())
	{
		gameRenderWorld->DebugBounds(state ? colorRed : colorGreen, GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), 15000);
	}
}

/*
================
idFuncAASObstacle::Event_Activate
================
*/
void idFuncAASObstacle::Event_Activate( idEntity *activator ) {
	state ^= 1;
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_OBSTACLE, state );
	if (cv_ai_show_aasfuncobstacle_state.GetBool())
	{
		gameRenderWorld->DebugBounds(state ? colorRed : colorGreen, GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), 2000);
	}
}

void idFuncAASObstacle::SetAASState(bool newState)
{
	state = newState;
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_OBSTACLE, state );
	if (cv_ai_show_aasfuncobstacle_state.GetBool())
	{
		gameRenderWorld->DebugBounds(state ? colorRed : colorGreen, GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), 2000);
	}
}


/*
===============================================================================

	idPhantomObjects

===============================================================================
*/

CLASS_DECLARATION( idEntity, idPhantomObjects )
	EVENT( EV_Activate,				idPhantomObjects::Event_Activate )
END_CLASS

/*
===============
idPhantomObjects::idPhantomObjects
===============
*/
idPhantomObjects::idPhantomObjects() {
	target			= NULL;
	end_time		= 0;
	throw_time 		= 0.0f;
	shake_time 		= 0.0f;
	shake_ang.Zero();
	speed			= 0.0f;
	min_wait		= 0;
	max_wait		= 0;
	fl.neverDormant	= false;
}

/*
===============
idPhantomObjects::Save
===============
*/
void idPhantomObjects::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteInt( end_time );
	savefile->WriteFloat( throw_time );
	savefile->WriteFloat( shake_time );
	savefile->WriteVec3( shake_ang );
	savefile->WriteFloat( speed );
	savefile->WriteInt( min_wait );
	savefile->WriteInt( max_wait );
	target.Save( savefile );
	savefile->WriteInt( targetTime.Num() );
	for( i = 0; i < targetTime.Num(); i++ ) {
		savefile->WriteInt( targetTime[ i ] );
	}

	for( i = 0; i < lastTargetPos.Num(); i++ ) {
		savefile->WriteVec3( lastTargetPos[ i ] );
	}
}

/*
===============
idPhantomObjects::Restore
===============
*/
void idPhantomObjects::Restore( idRestoreGame *savefile ) {
	int num;
	int i;

	savefile->ReadInt( end_time );
	savefile->ReadFloat( throw_time );
	savefile->ReadFloat( shake_time );
	savefile->ReadVec3( shake_ang );
	savefile->ReadFloat( speed );
	savefile->ReadInt( min_wait );
	savefile->ReadInt( max_wait );
	target.Restore( savefile );
	
	savefile->ReadInt( num );	
	targetTime.SetGranularity( 1 );
	targetTime.SetNum( num );
	lastTargetPos.SetGranularity( 1 );
	lastTargetPos.SetNum( num );

	for( i = 0; i < num; i++ ) {
		savefile->ReadInt( targetTime[ i ] );
	}

	if ( savefile->GetBuildNumber() == INITIAL_RELEASE_BUILD_NUMBER ) {
		// these weren't saved out in the first release
		for( i = 0; i < num; i++ ) {
			lastTargetPos[ i ].Zero();
		}
	} else {
		for( i = 0; i < num; i++ ) {
			savefile->ReadVec3( lastTargetPos[ i ] );
		}
	}
}

/*
===============
idPhantomObjects::Spawn
===============
*/
void idPhantomObjects::Spawn( void ) {
	throw_time = spawnArgs.GetFloat( "time", "5" );
	speed = spawnArgs.GetFloat( "speed", "1200" );
	shake_time = spawnArgs.GetFloat( "shake_time", "1" );
	throw_time -= shake_time;
	if ( throw_time < 0.0f ) {
		throw_time = 0.0f;
	}
	min_wait = SEC2MS( spawnArgs.GetFloat( "min_wait", "1" ) );
	max_wait = SEC2MS( spawnArgs.GetFloat( "max_wait", "3" ) );

	shake_ang = spawnArgs.GetVector( "shake_ang", "65 65 65" );
	Hide();
	GetPhysics()->SetContents( 0 );
}

/*
================
idPhantomObjects::Event_Activate
================
*/
void idPhantomObjects::Event_Activate( idEntity *activator ) {
	int i;
	float time;
	float frac;
	float scale;

	if ( thinkFlags & TH_THINK ) {
		BecomeInactive( TH_THINK );
		return;
	}

	RemoveNullTargets();
	if ( !targets.Num() ) {
		return;
	}

	if ( !activator || !activator->IsType( idActor::Type ) ) {
		target = gameLocal.GetLocalPlayer();
	} else {
		target = static_cast<idActor *>( activator );
	}
	
	end_time = gameLocal.time + SEC2MS( spawnArgs.GetFloat( "end_time", "0" ) );

	targetTime.SetNum( targets.Num() );
	lastTargetPos.SetNum( targets.Num() );

	const idVec3 &toPos = target.GetEntity()->GetEyePosition();

    // calculate the relative times of all the objects
	time = 0.0f;
	for( i = 0; i < targetTime.Num(); i++ ) {
		targetTime[ i ] = SEC2MS( time );
		lastTargetPos[ i ] = toPos;

		frac = 1.0f - ( float )i / ( float )targetTime.Num();
		time += ( gameLocal.random.RandomFloat() + 1.0f ) * 0.5f * frac + 0.1f;
	}

	// scale up the times to fit within throw_time
	scale = throw_time / time;
	for( i = 0; i < targetTime.Num(); i++ ) {
		targetTime[ i ] = static_cast<int>(gameLocal.time + SEC2MS( shake_time )+ targetTime[ i ] * scale);
	}

	BecomeActive( TH_THINK );
}

/*
===============
idPhantomObjects::Think
================
*/
void idPhantomObjects::Think( void ) {
	int			i;
	int			num;
	float		time;
	idVec3		vel;
	idVec3		ang;
	idEntity	*ent;
	idActor		*targetEnt;
	idPhysics	*entPhys;
	trace_t		tr;

	// if we are completely closed off from the player, don't do anything at all
	if ( CheckDormant() ) {
		return;
	}

	if ( !( thinkFlags & TH_THINK ) ) {
		BecomeInactive( thinkFlags & ~TH_THINK );
		return;
	}

	targetEnt = target.GetEntity();
	if ( !targetEnt || ( targetEnt->health <= 0 ) || ( end_time && ( gameLocal.time > end_time ) ) || gameLocal.inCinematic ) {
		BecomeInactive( TH_THINK );
	}

	const idVec3 &toPos = targetEnt->GetEyePosition();

	num = 0;
	for ( i = 0; i < targets.Num(); i++ ) {
		ent = targets[ i ].GetEntity();
		if ( !ent ) {
			continue;
		}
		
		if ( ent->fl.hidden ) {
			// don't throw hidden objects
			continue;
		}

		if ( !targetTime[ i ] ) {
			// already threw this object
			continue;
		}

		num++;

		time = MS2SEC( targetTime[ i ] - gameLocal.time );
		if ( time > shake_time ) {
			continue;
		}

		entPhys = ent->GetPhysics();
		const idVec3 &entOrg = entPhys->GetOrigin();

		gameLocal.clip.TracePoint( tr, entOrg, toPos, MASK_OPAQUE, ent );
		if ( tr.fraction >= 1.0f || ( gameLocal.GetTraceEntity( tr ) == targetEnt ) ) {
			lastTargetPos[ i ] = toPos;
		}

		if ( time < 0.0f ) {
			idAI::PredictTrajectory( entPhys->GetOrigin(), lastTargetPos[ i ], speed, entPhys->GetGravity(), 
				entPhys->GetClipModel(), entPhys->GetClipMask(), 256.0f, ent, targetEnt, ai_debugTrajectory.GetBool() ? 1 : 0, vel );
			vel *= speed;
			entPhys->SetLinearVelocity( vel );
			if ( !end_time ) {
				targetTime[ i ] = 0;
			} else {
				targetTime[ i ] = gameLocal.time + gameLocal.random.RandomInt( max_wait - min_wait ) + min_wait;
			}
			if ( ent->IsType( idMoveable::Type ) ) {
				idMoveable *ment = static_cast<idMoveable*>( ent );
				ment->EnableDamage( true, 2.5f );
			}
		} else {
			// this is not the right way to set the angular velocity, but the effect is nice, so I'm keeping it. :)
			ang.x = gameLocal.random.CRandomFloat() * shake_ang.x;
			ang.y = gameLocal.random.CRandomFloat() * shake_ang.y;
			ang.z = gameLocal.random.CRandomFloat() * shake_ang.z;
			ang *= ( 1.0f - time / shake_time );
			entPhys->SetAngularVelocity( ang );
		}
	}

	if ( !num ) {
		BecomeInactive( TH_THINK );
	}
}

/*
===============================================================================
idPortalSky
===============================================================================
*/

CLASS_DECLARATION( idEntity, idPortalSky )
	EVENT( EV_PostSpawn,			idPortalSky::Event_PostSpawn )
	EVENT( EV_Activate,				idPortalSky::Event_Activate )
END_CLASS

/*
===============
idPortalSky::idPortalSky
===============
*/

idPortalSky::idPortalSky( void ) {
}

/*
===============
idPortalSky::~idPortalSky
===============
*/

idPortalSky::~idPortalSky( void ) {
}

/*
===============
idPortalSky::Spawn
===============
*/

void idPortalSky::Spawn( void )
{
	// grayman #3108 - contributed by neuro & 7318
	if ( spawnArgs.GetInt( "type" ) == PORTALSKY_GLOBAL )
	{
		gameLocal.SetGlobalPortalSky( spawnArgs.GetString( "name" ) );
		gameLocal.portalSkyGlobalOrigin = GetPhysics()->GetOrigin();
	}

	if ( !spawnArgs.GetBool( "triggered" ) )
	{
		gameLocal.portalSkyScale = spawnArgs.GetInt( "scale", "16" );	
		PostEventMS( &EV_PostSpawn, 1 );
	}
	// end neuro & 7318
}

/*
================
idPortalSky::Event_PostSpawn
================
*/

void idPortalSky::Event_PostSpawn()
{
	// grayman #3108 - contributed by neuro & 7318
	gameLocal.SetCurrentPortalSkyType( spawnArgs.GetInt( "type", "0" ) );

	if ( gameLocal.GetCurrentPortalSkyType() != PORTALSKY_GLOBAL )
	{
		// Standard and local portalSky share the origin. It's in the execution that things change.
		gameLocal.portalSkyOrigin = GetPhysics()->GetOrigin();
	}

	gameLocal.SetPortalSkyEnt( this );
	// end neuro & 7318
}

/*
================
idPortalSky::Event_Activate
================
*/

void idPortalSky::Event_Activate( idEntity *activator )
{
	// grayman #3108 - contributed by neuro & 7318
	gameLocal.SetCurrentPortalSkyType( spawnArgs.GetInt( "type", "0" ) );

	if ( gameLocal.GetCurrentPortalSkyType() != PORTALSKY_GLOBAL )
	{
		// Standard and local portalSky share the origin. It's in the execution that things change.
		gameLocal.portalSkyOrigin = GetPhysics()->GetOrigin();
	}	

	gameLocal.portalSkyScale = spawnArgs.GetInt( "scale", "16" );
	gameLocal.SetPortalSkyEnt( this );
	// end neuro & 7318
}

/*
===============================================================================

  tdmVine - climbable vine piece (grayman #2787)

===============================================================================
*/

const idEventDef EV_Vine_SetPrime( "setPrime", EventArgs('e', "vine", ""), EV_RETURNS_VOID, "Event called using vine.*()" );
const idEventDef EV_Vine_GetPrime( "getPrime", EventArgs(), 'e', "Event called using vine.*()" );
const idEventDef EV_Vine_AddDescendant( "addDescendant", EventArgs('e', "vine", ""), EV_RETURNS_VOID, "Event called using vine.*()" );
const idEventDef EV_Vine_CanWater( "canWater", EventArgs(), 'f', "Event called using vine.*()" );
const idEventDef EV_Vine_SetWatered( "setWatered", EventArgs(), EV_RETURNS_VOID, "Event called using vine.*()" );
const idEventDef EV_Vine_ClearWatered( "clearWatered", EventArgs(), EV_RETURNS_VOID, "Event called using vine.*()" );
const idEventDef EV_Vine_ScaleVine( "scaleVine", EventArgs('f', "factor", ""), EV_RETURNS_VOID, "Event called using vine.*()" );

CLASS_DECLARATION( idStaticEntity, tdmVine )
	EVENT( EV_Vine_SetPrime, 		tdmVine::Event_SetPrime)
	EVENT( EV_Vine_GetPrime, 		tdmVine::Event_GetPrime)
	EVENT( EV_Vine_AddDescendant, 	tdmVine::Event_AddDescendant)
	EVENT( EV_Vine_CanWater, 		tdmVine::Event_CanWater)
	EVENT( EV_Vine_SetWatered, 		tdmVine::Event_SetWatered)
	EVENT( EV_Vine_ClearWatered, 	tdmVine::Event_ClearWatered )
	EVENT( EV_Vine_ScaleVine,		tdmVine::Event_ScaleVine )
END_CLASS

tdmVine::tdmVine( void )
{
	_watered = false;
	_prime = NULL;
	_descendants.Clear();
}

void tdmVine::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool( _watered );
	savefile->WriteObject( _prime );
	savefile->WriteInt( _descendants.Num() );
	for ( int i = 0 ; i < _descendants.Num() ; i++)
	{
		_descendants[i].Save( savefile );
	}
}

void tdmVine::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool( _watered );
	savefile->ReadObject( reinterpret_cast<idClass*&>( _prime ) );
	int num;
	savefile->ReadInt( num );
	_descendants.SetNum( num );
	for ( int i = 0 ; i < num ; i++ )
	{
		_descendants[i].Restore( savefile );
	}
}

void tdmVine::Spawn()
{
}

void tdmVine::Event_SetPrime( tdmVine* newPrime )
{
	_prime = newPrime;
}

void tdmVine::Event_GetPrime()
{
	idThread::ReturnEntity( _prime );
}

void tdmVine::Event_AddDescendant( tdmVine* descendant )
{
	idEntityPtr< tdmVine > tdmVinePtr;
	tdmVinePtr = descendant;
	_descendants.Append( tdmVinePtr );
}

void tdmVine::Event_ClearWatered()
{
	_watered = false;
}

void tdmVine::Event_SetWatered()
{
	_watered = true;
}

void tdmVine::Event_CanWater()
{
	// For a given vine family, only allow two pieces to be
	// watered by a water stim. Otherwise, growth can be
	// rampant as the stim falls through the family.

	float canWater = 1;
	int wateredCount = 0;
	if ( _watered ) // the prime vine should check itself first
	{
		wateredCount++;
	}
	for ( int i = 0 ; i < _descendants.Num() ; i++ )
	{
		idEntityPtr< tdmVine > tdmVinePtr = _descendants[i];
		tdmVine* vine = tdmVinePtr.GetEntity();
		if ( vine && vine->_watered )
		{
			if ( ++wateredCount >= 2 )
			{
				canWater = 0;
				break;
			}
		}
	}
	idThread::ReturnFloat( canWater );
}

void tdmVine::Event_ScaleVine(float factor)
{
	idMat3 axis = GetPhysics()->GetAxis();
	axis *= factor;
	GetPhysics()->SetAxis( axis );
	UpdateVisuals();
}

/*
===============================================================================

idPeek - a peek entity you can look through

===============================================================================
*/
CLASS_DECLARATION(idStaticEntity, idPeek)
END_CLASS

idPeek::idPeek(void) {
}

/*
===============
idPeek::~idPeek
===============
*/

idPeek::~idPeek(void) {
}

/*
===============
idPeek::Spawn
===============
*/

void idPeek::Spawn(void)
{
}



