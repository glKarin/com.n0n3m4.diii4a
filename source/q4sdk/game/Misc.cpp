/*

Various utility objects and functions.

*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

// RAVEN BEGIN
// nmckenzie: added ai
#if !defined(__GAME_PROJECTILE_H__)
	#include "Projectile.h"
#endif
#include "ai/AI.h"
// RAVEN END

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

const idEventDef EV_TeleportStage( "<TeleportStage>", "e" );

CLASS_DECLARATION( idEntity, idPlayerStart )
	EVENT( EV_Activate,			idPlayerStart::Event_Teleport )
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
			float x = msg.ReadFloat();
			float y = msg.ReadFloat();
			float z = msg.ReadFloat();

			idVec3 prevOrigin( x, y, z );
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			if ( player != NULL && player->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
				Event_TeleportEntity( player, false, prevOrigin );
			}
			return true;
		}
		case EVENT_TELEPORTITEM: {
			entityNumber = msg.ReadBits( GENTITYNUM_BITS );
			idEntity* activator = gameLocal.entities[ entityNumber ];

			if ( activator != NULL ) {
				Event_TeleportEntity( activator, false );
			}
			return true;
		}

		default: {
			return idEntity::ClientReceiveEvent( event, time, msg );
		}
	}
//unreachable
//	return false;
}

/*
===============
idPlayerStart::Event_TeleportStage

FIXME: add functionality to fx system ( could be done with player scripting too )
================
*/
void idPlayerStart::Event_TeleportStage( idPlayer *player ) {
	float teleportDelay = spawnArgs.GetFloat( "teleportDelay" );
	switch ( teleportStage ) {
		case 0:
			player->playerView.Flash( colorWhite, 125 );
			player->SetInfluenceLevel( INFLUENCE_LEVEL3 );
			player->SetInfluenceView( spawnArgs.GetString( "mtr_teleportFx" ), NULL, 0.0f, NULL );
			soundSystem->FadeSoundClasses( SOUNDWORLD_GAME, 0, -20.0f, teleportDelay );
			teleportStage++;
			PostEventSec( &EV_TeleportStage, teleportDelay, player );
			break;
		case 1:
			soundSystem->FadeSoundClasses( SOUNDWORLD_GAME, 0, 0.0f, 0.25f );
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
	float pushVel = spawnArgs.GetFloat( "push", "50000" );
	float f = spawnArgs.GetFloat( "visualEffect", "0" );
	const char *viewName = spawnArgs.GetString( "visualView", "" );
	idEntity *ent = viewName ? gameLocal.FindEntity( viewName ) : NULL;

	if ( f && ent ) {
		// place in private camera view for some time
		// the entity needs to teleport to where the camera view is to have the PVS right
		player->Teleport( ent->GetPhysics()->GetOrigin(), ang_zero, this );

		player->SetPrivateCameraView( static_cast<idCamera*>(ent) );
		// the player entity knows where to spawn from the previous Teleport call
		if ( !gameLocal.isClient ) {
			player->PostEventSec( &EV_Player_ExitTeleporter, f );
		}
	} else {
		// direct to exit, Teleport will take care of the killbox
		player->Teleport( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis().ToAngles(), NULL );

		// multiplayer hijacked this entity, so only push the player in multiplayer
		if ( gameLocal.isMultiplayer ) {
			//push
			idVec3 impulse( pushVel, 0, 0 );
			impulse *= GetPhysics( )->GetAxis ( );
			player->ApplyImpulse( gameLocal.world, 0, player->GetPhysics( )->GetOrigin( ), impulse );
		}
	}
}

/*
===============
idPlayerStart::Teleport
For non-players
================
*/
void idPlayerStart::Teleport( idEntity* other ) {
	other->SetOrigin( GetPhysics()->GetOrigin() );
	idVec3 vel = other->GetPhysics()->GetLinearVelocity();
	vel *= GetPhysics()->GetAxis();
	other->GetPhysics()->SetLinearVelocity( vel );
}

void idPlayerStart::Event_TeleportEntity( idEntity* activator, bool predicted, idVec3& prevOrigin ) {
	idPlayer *player;

	if ( activator->IsType( idPlayer::GetClassType() ) ) {
		player = static_cast<idPlayer*>( activator );
	} else {
		player = NULL;

		if ( gameLocal.isServer ) {
			idBitMsg	msg;
			byte		msgBuf[MAX_EVENT_PARAM_SIZE];

			msg.Init( msgBuf, sizeof( msgBuf ) );
			msg.BeginWriting();
			msg.WriteBits( activator->entityNumber, GENTITYNUM_BITS );
			ServerSendInstanceEvent( EVENT_TELEPORTITEM, &msg, false, -1 );
		}

		idVec3 oldOrigin = activator->GetPhysics()->GetOrigin();
		Teleport( activator );

		if( gameLocal.isMultiplayer ) {
			if( gameLocal.isServer ) {
				gameLocal.PlayEffect( activator->spawnArgs, "fx_teleport_enter", oldOrigin, idVec3(0,0,1).ToMat3(), false, vec3_origin );
				gameLocal.PlayEffect( activator->spawnArgs, "fx_teleport", activator->GetPhysics()->GetOrigin(), idVec3(0,0,1).ToMat3(), false, vec3_origin );
			} else if( predicted ) {
				// only predict teleport enter
				gameLocal.PlayEffect( activator->spawnArgs, "fx_teleport_enter", oldOrigin, idVec3(0,0,1).ToMat3(), false, vec3_origin );		
			} else {
				gameLocal.PlayEffect( activator->spawnArgs, "fx_teleport", oldOrigin, idVec3(0,0,1).ToMat3(), false, vec3_origin );
			}
		}
	}

	if ( player ) {
		if ( spawnArgs.GetBool( "visualFx" ) ) {

			teleportStage = 0;
			Event_TeleportStage( player );

		} else {
			idVec3 oldOrigin;
			
			if( gameLocal.isServer || prevOrigin == vec3_zero ) {
				oldOrigin = activator->GetPhysics()->GetOrigin();
			} else {
				oldOrigin = prevOrigin;
			}

			if ( gameLocal.isServer ) {
				idBitMsg	msg;
				byte		msgBuf[MAX_EVENT_PARAM_SIZE];

				msg.Init( msgBuf, sizeof( msgBuf ) );
				msg.BeginWriting();
				msg.WriteBits( player->entityNumber, GENTITYNUM_BITS );
				msg.WriteFloat( oldOrigin.x );
				msg.WriteFloat( oldOrigin.y );
				msg.WriteFloat( oldOrigin.z );
				ServerSendInstanceEvent( EVENT_TELEPORTPLAYER, &msg, false, -1 );
			}

			TeleportPlayer( player );

			if( gameLocal.isMultiplayer ) {
				gameLocal.PlayEffect( player->spawnArgs, "fx_teleport_enter", oldOrigin, idVec3(0,0,1).ToMat3(), false, vec3_origin );
				gameLocal.PlayEffect( player->spawnArgs, "fx_teleport", player->GetPhysics()->GetOrigin(), idVec3(0,0,1).ToMat3(), false, vec3_origin );
			}
		}
		}
	}


/*
===============
idPlayerStart::Event_TeleportPlayer
================
*/
void idPlayerStart::Event_Teleport( idEntity *activator ) {
	Event_TeleportEntity( activator, true );
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

// RAVEN BEGIN
// bdube: optional clip model on activators
	const char	*temp;
	if ( spawnArgs.GetString( "clipmodel", "", &temp ) ) {
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_PUSH_HEAP_MEM(this);
// RAVEN END
		GetPhysics()->SetClipModel( new idClipModel(temp), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_POP_HEAP();
// RAVEN END
	} else {
		GetPhysics()->SetClipBox( idBounds( vec3_origin ).Expand( 4 ), 1.0f );
	}

	GetPhysics()->SetContents( 0 );
// RAVEN END

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
//	EVENT( AI_RandomPath,		idPathCorner::Event_RandomPath )
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
idPathCorner *idPathCorner::RandomPath( const idEntity *source, const idEntity *ignore ) {
	int	i;
	int	num;
	int which;
	idEntity *ent;
	idPathCorner *path[ MAX_GENTITIES ];

	num = 0;
	for( i = 0; i < source->targets.Num(); i++ ) {
		ent = source->targets[ i ].GetEntity();
		if ( ent && ( ent != ignore ) && ent->IsType( idPathCorner::Type ) ) {
			path[ num++ ] = static_cast<idPathCorner *>( ent );
			if ( num >= MAX_GENTITIES ) {
				break;
			}
		}
	}

	if ( !num ) {
		return NULL;
	}

	which = gameLocal.random.RandomInt( num );
	return path[ which ];
}

/*
=====================
idPathCorner::Event_RandomPath
=====================
*/
void idPathCorner::Event_RandomPath( void ) {
	idPathCorner *path;

	path = RandomPath( this, NULL );
	idThread::ReturnEntity( path );
}

/*
===============================================================================

  idDamagable
	
===============================================================================
*/

const idEventDef EV_RestoreDamagable( "<RestoreDamagable>" );
// RAVEN BEGIN
// kfuller: spawn other things
const idEventDef EV_SpawnForcefield( "<SpawnForcefield>" );
const idEventDef EV_SpawnTriggerHurt( "<SpawnTriggerHurt>" );
// RAVEN END

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
	invincibleTime = 0;
}

/*
================
idDamagable::Save
================
*/
void idDamagable::Save( idSaveGame *savefile ) const {

	savefile->WriteInt( invincibleTime );

	savefile->WriteInt( stage );
	savefile->WriteInt( stageNext );
	
	// cnicholson: Don't save the stageDict, its setup in the restore
	
	savefile->WriteInt( stageEndTime );
	savefile->WriteInt( stageEndHealth );
	savefile->WriteInt( stageEndSpeed );
	savefile->WriteBool( stageEndOnGround );
	savefile->WriteBool( activateStageOnTrigger );

	savefile->WriteInt( count );
	savefile->WriteInt( nextTriggerTime );
}

/*
================
idDamagable::Restore
================
*/
void idDamagable::Restore( idRestoreGame *savefile ) {

	const char* stageName;
	
	savefile->ReadInt( invincibleTime );

	savefile->ReadInt( stage );
	savefile->ReadInt( stageNext );
	savefile->ReadInt( stageEndTime );
	savefile->ReadInt( stageEndHealth );
	savefile->ReadInt( stageEndSpeed );
	savefile->ReadBool( stageEndOnGround );
	savefile->ReadBool( activateStageOnTrigger );

	savefile->ReadInt( count );
	savefile->ReadInt( nextTriggerTime );
	

	// Get the stage name, if there is none then there are no more stages
	stageDict = NULL;
	if ( spawnArgs.GetString ( va( "def_stage%d", stage ), "", &stageName ) && stageName ) {

		stageDict = gameLocal.FindEntityDefDict ( stageName, false );
	}
}

/*
================
idDamagable::Spawn
================
*/
void idDamagable::Spawn( void ) {
	idStr	broken;
	bool	keepContents;

// RAVEN BEGIN
// abahr: stage stuff
	stage				= 0;
	stageNext			= 1;
	stageDict			= NULL;
	stageEndTime		= 0;
	stageEndHealth		= 9999;
	stageEndSpeed		= 0;
//jshepard:
	stageEndOnGround    = false;
// RAVEN END

	health = spawnArgs.GetInt( "health", "5" );

// RAVEN BEGIN
// abahr: stage stuff
   stageEndHealth = health - 1;
	activateStageOnTrigger = spawnArgs.GetBool("activateOnTrigger");
// RAVEN END

	spawnArgs.GetInt( "count", "1", count );
	invincibleTime = gameLocal.GetTime() + SEC2MS( spawnArgs.GetFloat( "invincibleTime", "0" ) );
	nextTriggerTime = 0;
	
	// make sure the model gets cached
	spawnArgs.GetString( "broken", "", broken );
	if ( broken.Length() && !renderModelManager->CheckModel( broken ) ) {
		gameLocal.Error( "idDamagable '%s' at (%s): cannot load broken model '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), broken.c_str() );
	}

// RAVEN BEGIN
// bdube: turn take damage off if it has no health
	fl.takedamage = health > 0 ? true : false;
// RAVEN END

	keepContents = spawnArgs.GetBool( "KeepContents" );

	if ( keepContents ) {
		// do nothing, keep the contents from the model
	} else {
		GetPhysics()->SetContents( CONTENTS_SOLID );
	}
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
============
idMoveable::Damage
============
*/
// RAVEN BEGIN
// abahr
void idDamagable::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location ) {

	if ( invincibleTime > gameLocal.GetTime() )	{
		return;
	}

	// If there is a damage filter we need to check to see if the damage def meets the requirement
	const char* damageFilter;
	if ( spawnArgs.GetString ( "damage_filter", "", &damageFilter ) && *damageFilter ) {
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
		if ( !damageDef ) {
			gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
		}
		// If the filter isnt matched then ignore it
		if ( !damageDef->GetBool ( va("filter_%s", damageFilter ) ) ) {
			return;
		}
	}
	if ( spawnArgs.GetBool( "nosplash", "0" ) ) {
		//ignore splash damage
		const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, false );
		if ( !damageDef ) {
			gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
		}
		//if it has radius, then it's splash damage and ignore it
		if ( damageDef->GetFloat ( "radius", "0" ) ) {
			return;
		}
	}
	
	idEntity::Damage ( inflictor, attacker, dir, damageDefName, damageScale, location );
	
	// Force a stage update so impact effects will be correct
	UpdateStage();
}

/*
================
idDamagable::Killed
================
*/
void idDamagable::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	if ( gameLocal.time < nextTriggerTime ) {
		health += damage;
		return;
	}

	BecomeBroken( attacker );
}

/*
================
idDamagable::UpdateStage
================
*/
void idDamagable::UpdateStage ( void ) {
	
	// If there is a stage to go to then see if its time to do that
 	if ( stage != stageNext ) {
		int	 oldstage;

		// Check to see if the stage is complete
		oldstage = stage;
		while ( health < stageEndHealth || (stageEndTime != 0 && gameLocal.time > stageEndTime ) || (stageEndOnGround && GetPhysics()->HasGroundContacts()) || activateStageOnTrigger ) {
			stage = stageNext;
			
			//this only needs to happen once
			activateStageOnTrigger = false;

			// Get the stage name, if there is none then there are no more stages
			const char* stageName;
			if ( !spawnArgs.GetString ( va("def_stage%d", stage ), "", &stageName ) || !stageName ) {
				stage = 0;
				break;
			}

			// Get the stage dictionary
			stageDict = gameLocal.FindEntityDefDict ( stageName, false );
			if ( !stageDict ) {
				gameLocal.Warning ( "could not find stage '%s' for moveable '%s'", stageName, name.c_str() );
				stage = 0;
				break;
			}

			// Get the end conditions of the stage
			stageEndHealth = stageDict->GetInt ( "end_health", "-9999" );

			stageEndOnGround = stageDict->GetBool( "end_onGround" );

			// Timed?			
			stageEndTime = SEC2MS ( GetStageFloat ( "end_time" ) );
			if ( stageEndTime > 0 ) {
				stageEndTime += gameLocal.time;			
			}		
			
			// Set the next stage to move to when end conditions met
			stageNext = stage + 1;

			// Execute the new stage
			ExecuteStage ( );
		}
	}	
}

/*
================
idDamagable::ExecuteStage
================
*/
void idDamagable::ExecuteStage ( void ) {
	const idKeyValue* kv;
	bool			  remove;
	
	// Remove the entity this frame?
	remove = stageDict->GetBool ( "remove" );
	
	// Targets?
	if ( stageDict->GetBool( "triggerTargets" ) ) {
		ActivateTargets( this );
	}

	// Unbind targets
	if ( stageDict->GetBool( "unbindTargets" ) ) {
		UnbindTargets( this );
	}

	// Stop current looping effects
	StopAllEffects ( );

	// Play all effects (if they start with "fx_loop" then also loop them
	for ( kv = stageDict->MatchPrefix ( "fx_" ); kv; kv = stageDict->MatchPrefix("fx_",kv) ) {
		if ( !kv->GetKey().Icmpn ( "fx_loop", 7 ) ) {
			if ( !remove ) {
				PlayEffect ( gameLocal.GetEffect ( *stageDict, kv->GetKey() ), 
							 renderEntity.origin + GetStageVector ( va("offset_%s", kv->GetKey().c_str() ) ), 
							 GetStageVector ( va("dir_%s", kv->GetKey().c_str() ), "1 0 0" ).ToMat3() * renderEntity.axis, 
							 true );
			}
		} else {
			gameLocal.PlayEffect ( gameLocal.GetEffect ( *stageDict, "fx_explode" ), 
								   GetPhysics()->GetOrigin() + GetStageVector ( va("offset_%s", kv->GetKey().c_str() ) ) * GetPhysics()->GetAxis(), 
								   GetStageVector ( va("dir_%s", kv->GetKey().c_str() ), "1 0 0" ).ToMat3() );
		}
	}

	// Kick off debris
	for ( kv = stageDict->MatchPrefix ( "def_debris" ); kv; kv = stageDict->MatchPrefix("def_debris",kv) ) {		
		const idDict* args = gameLocal.FindEntityDefDict ( kv->GetValue(), false );
		if ( !args ) {
			continue;
		}

		rvClientMoveable* cent = NULL;
		// force spawnclass to rvClientMoveable
		gameLocal.SpawnClientEntityDef( *args, (rvClientEntity**)(&cent), false, "rvClientMoveable" );

		if( !cent ) {
			continue;
		}
		cent->SetOrigin ( GetPhysics()->GetOrigin() + GetStageVector ( va("offset_%s", kv->GetKey().c_str()) ) * GetPhysics()->GetAxis() );
		cent->SetAxis ( GetPhysics()->GetAxis ( ) );
		
		idVec3 vel;
		vel = GetStageVector ( va("vel_%s", kv->GetKey().c_str()) ) * GetPhysics()->GetAxis();
		vel += GetPhysics()->GetLinearVelocity ( );
		cent->GetPhysics()->SetLinearVelocity ( vel );
		cent->PostEventMS ( &CL_FadeOut, 2500, 2500 );
	}

    // Apply force to all objects nearby
	float fPush;
	stageDict->GetFloat("radiusPush","0",fPush);
	if (fPush > 0)	{
		gameLocal.RadiusPush( GetPhysics()->GetOrigin(), 128, fPush, this, this, 1.0f, true );
	}
	
	// Remove the entity now?
	if ( remove ) {
		Hide ( );
		PostEventMS ( &EV_Remove, 0 );
		return;
	} 
	
	// Switch model?
	const char* model;
	if ( stageDict->GetString ( "model", "", &model ) && *model ) {			
		SetModel( model );
	}

	// Skin?
	const char* skin;
	if ( stageDict->GetString ( "skin", "", &skin ) && *skin ) {
		renderEntity.customSkin = declManager->FindSkin ( skin, false );
	}
	
	// Velocities
	idVec3 vel;
	vel  = GetStageVector ( "vel_world" );
	vel += (GetStageVector ( "vel_local" ) * GetPhysics()->GetAxis() );
	vel += GetPhysics()->GetLinearVelocity ( );	
	GetPhysics()->SetLinearVelocity ( vel );
		
	// Enable thinking to ensure stages are run
	BecomeActive ( TH_THINK );
}

/*
================
idDamagable::GetStageVector
================
*/
idVec3 idDamagable::GetStageVector ( const char* key, const char* defaultString ) const {
	idVec3 mins;
	if ( stageDict->GetVector ( va("%s_min", key ), "", mins ) ) {
		idVec3 maxs;
		stageDict->GetVector ( va("%s_max", key), mins.ToString(), maxs );
		return mins + (maxs - mins) * gameLocal.random.RandomFloat ( );
	}
	
	return stageDict->GetVector ( key, defaultString );
}

/*
================
idDamagable::GetStageFloat
================
*/
float idDamagable::GetStageFloat	( const char* key, const char* defaultString ) const {
	float minValue;
	if ( stageDict->GetFloat ( va("%s_min", key ), "", minValue ) ) {
		float maxValue;
		stageDict->GetFloat ( va("%s_max", key), va("%g",minValue), maxValue );
		return minValue + (maxValue - minValue) * gameLocal.random.CRandomFloat ( );
	}
	
	return stageDict->GetFloat ( key, defaultString );
}

/*
================
idDamagable::GetStageInt
================
*/
int idDamagable::GetStageInt	( const char* key, const char* defaultString ) const {
	int minValue;
	if ( stageDict->GetInt ( va("%s_min", key ), "", minValue ) ) {
		int maxValue;
		stageDict->GetInt ( va("%s_max", key), va("%d",minValue), maxValue );
		return minValue + (maxValue - minValue) * gameLocal.random.CRandomFloat ( );
	}
	
	return stageDict->GetInt ( key, defaultString );
}

/*
================
idDamagable::Event_BecomeBroken
================
*/
void idDamagable::Event_BecomeBroken( idEntity *activator ) {
	BecomeBroken( activator );

// RAVEN BEGIN
// bdube: start stages
	UpdateStage ( );
// RAVEN END
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
		if ( ent1 && ent1->GetPhysics() ) {
			axis = ent1->GetPhysics()->GetAxis();
			origin = ent1->GetPhysics()->GetOrigin();
			start = origin + start * axis;
		}

		end = p2;
		if ( ent2 && ent2->GetPhysics() ) {
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
================
idSpring::Save
================
*/
void idSpring::Save( idSaveGame *savefile ) const {
	savefile->WriteInt ( id1 );
	savefile->WriteInt ( id2 );
	savefile->WriteVec3 ( p1 );
	savefile->WriteVec3 ( p2 );
	spring.Save ( savefile );
}

/*
================
idSpring::Restore
================
*/
void idSpring::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt ( id1 );
	savefile->ReadInt ( id2 );
	savefile->ReadVec3 ( p1 );
	savefile->ReadVec3 ( p2 );
	spring.Restore ( savefile );
	Event_LinkSpring ( );	
}

/*
===============================================================================

  idForceField
	
===============================================================================
*/

const idEventDef EV_Toggle( "Toggle", NULL );

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

	// set the collision model on the force field
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	forceField.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ) );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
	forceField.SetOwner( this );
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

// RAVEN BEGIN
// bdube: jump pads

/*
===============================================================================

  idForceField
	
===============================================================================
*/

const int JUMPPAD_EFFECT_DELAY	= 100;

CLASS_DECLARATION( idForceField, rvJumpPad )
	EVENT( EV_FindTargets,	rvJumpPad::Event_FindTargets )
END_CLASS

/*
================
rvJumpPad::rvJumpPad
================
*/
rvJumpPad::rvJumpPad ( void ) {
	lastEffectTime = -1;
}

/*
================
rvJumpPad::Think
================
*/
void rvJumpPad::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		// evaluate force
		forceField.Evaluate( gameLocal.time );
		
		// If force has been applied to an entity and jump pad effect hasnt been played for a bit
		// then play it now.
		if ( forceField.GetLastApplyTime ( ) - lastEffectTime > JUMPPAD_EFFECT_DELAY ) {
			// start locally
			StartSound( "snd_jump", SND_CHANNEL_ITEM, 0, false, NULL );
			if ( spawnArgs.GetString( "fx_jump" )[ 0 ] ) {
				PlayEffect( "fx_jump", renderEntity.origin, effectAxis, false, vec3_origin, false );
			}

			// send through unreliable
			if ( gameLocal.isServer ) {
				idBitMsg	msg;
				byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];

				msg.Init( msgBuf, sizeof( msgBuf ) );
				msg.WriteByte( GAME_UNRELIABLE_MESSAGE_EVENT );
				msg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
				msg.WriteByte( EVENT_JUMPFX );
				gameLocal.SendUnreliableMessagePVS( msg, this, gameLocal.pvs.GetPVSArea( renderEntity.origin ) );
			}
			
			lastEffectTime = forceField.GetLastApplyTime( );
		}
	}
	Present();
}

/*
================
rvJumpPad::Spawn
================
*/
void rvJumpPad::Spawn( void ) {
	forceField.SetApplyType( FORCEFIELD_APPLY_VELOCITY );
	forceField.SetOwner( this );
}

/*
================
rvJumpPad::Event_FindTargets
================
*/
void rvJumpPad::Event_FindTargets( void ) {
	FindTargets();
	RemoveNullTargets();
	if ( targets.Num() ) {
		idEntity* ent;
		ent = targets[0].GetEntity();
		assert( ent );
		
		idVec3 vert;
		idVec3 diff;
		idVec3 vel;
		idVec3 localGravity( gameLocal.GetCurrentGravity(this) );
		idVec3 localGravityNormal( localGravity.ToNormal() );
		float  time;
		float  dist;
		
		// Find the distance along the gravity vector between the jump pad and its target
		diff = (ent->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin());
		vert = (diff * localGravityNormal) * localGravityNormal;
		
		// Determine how long it would take to cover the distance along the gravity vector
		time = idMath::Sqrt( vert.Length() / (0.5f * localGravity.Length()) );
		if ( !time ) {
			PostEventMS( &EV_Remove, 0 );
			return;
		}

		// The final velocity is the direction between the jump pad and its target using
		// the travel time to determine the forward vector and adding in an inverse gravity vector.
		vel  = diff - vert;
		dist = vel.Normalize();
		
		vel = vel * (dist / time);
		vel += (localGravity * -time);

		forceField.Uniform( vel );

		// calculate a coordinate axis where the vector to the jumppad target is forward
		// use this to play the fx_jump fx
		diff.Normalize();
		effectAxis = diff.ToMat3();
	}
}

/*
===============
rvJumpPad::ClientReceiveEvent
===============
*/
bool rvJumpPad::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	switch ( event ) {
	case EVENT_JUMPFX: {
		if ( spawnArgs.GetString( "fx_jump" )[ 0 ] ) {
			PlayEffect( "fx_jump", renderEntity.origin, effectAxis, false, vec3_origin, false );
		}
		StartSound( "snd_jump", SND_CHANNEL_ITEM, 0, false, NULL );
		return true;
	}
	default:
		return idEntity::ClientReceiveEvent( event, time, msg );
	}
}

// RAVEN END

/*
===============================================================================

	idAnimated

===============================================================================
*/

const idEventDef EV_Animated_Start( "<start>" );
const idEventDef EV_LaunchMissiles( "launchMissiles", "ssssdf" );
const idEventDef EV_LaunchMissilesUpdate( "<launchMissiles>", "dddd" );
const idEventDef EV_AnimDone( "<AnimDone>", "d" );
const idEventDef EV_StartRagdoll( "startRagdoll" );

// RAVEN BEGIN
// bdube: script object
const idEventDef EV_SetAnimState ( "setAnimState", "sd" );
// RAVEN END

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

// RAVEN BEGIN
// bdube: script object
	EVENT( EV_SetAnimState,			idAnimated::Event_SetAnimState )
	EVENT( AI_PlayAnim,				idAnimated::Event_PlayAnim )
	EVENT( AI_PlayCycle,			idAnimated::Event_PlayCycle )
	EVENT( AI_AnimDone,				idAnimated::Event_AnimDone2 )
// RAVEN END

END_CLASS

/*
===============
idAnimated::idAnimated
================
*/
idAnimated::idAnimated() {
	int		i;

	anim = 0;
	blendFrames = 0;
	soundJoint = INVALID_JOINT;
	activated = false;
	combatModel = NULL;
	activator = NULL;
	current_anim_index = 0;
	num_anims = 0;
// RAVEN BEGIN
// bdube: script control
	scriptThread = NULL;
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		animDoneTime[i] = 0;
	}
// RAVEN END
}

/*
===============
idAnimated::idAnimated
================
*/
idAnimated::~idAnimated() {
	delete combatModel;
	combatModel = NULL;
	
// RAVEN BEGIN
// bdube: kill script object
	delete scriptThread;
// RAVEN END		
}

/*
===============
idAnimated::Save
================
*/
void idAnimated::Save( idSaveGame *savefile ) const {
	int		i;

	savefile->WriteInt( num_anims );
	savefile->WriteInt( current_anim_index );
	savefile->WriteInt( anim );
	savefile->WriteInt( blendFrames );
	savefile->WriteJoint( soundJoint );
	activator.Save( savefile );
	savefile->WriteBool( activated );

// RAVEN BEGIN
	savefile->WriteObject( scriptThread );
	savefile->WriteString( state );
	savefile->WriteString( idealState );
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		savefile->WriteInt( animDoneTime[i] );
	}
// RAVEN END
}

/*
===============
idAnimated::Restore
================
*/
void idAnimated::Restore( idRestoreGame *savefile ) {
	int		i;

	savefile->ReadInt( num_anims );
	savefile->ReadInt( current_anim_index );
	savefile->ReadInt( anim );
	savefile->ReadInt( blendFrames );
	savefile->ReadJoint( soundJoint );
	activator.Restore( savefile );
	savefile->ReadBool( activated );

// RAVEN BEGIN
	savefile->ReadObject( reinterpret_cast<idClass *&>( scriptThread ) );
	savefile->ReadString( state );
	savefile->ReadString( idealState );
	for( i = 0; i < ANIM_NumAnimChannels; i++ ) {
		savefile->ReadInt( animDoneTime[i] );
	}
// RAVEN END
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

	LoadAF( NULL );

	// allow bullets to collide with a combat model
	if ( spawnArgs.GetBool( "combatModel", "0" ) ) {
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

	// allow the entity to take damage
	if ( spawnArgs.GetBool( "takeDamage", "0" ) ) {
		fl.takedamage = true;
	}

	blendFrames = 0;

// RAVEN BEGIN
// bdube: script control
	// setup script object
	const char* scriptObjectName;
	if ( spawnArgs.GetString( "scriptobject", NULL, &scriptObjectName ) ) {
		if ( !scriptObject.SetType( scriptObjectName ) ) {
			gameLocal.Error( "Script object '%s' not found on entity '%s'.", scriptObjectName, name.c_str() );
		}

		// init the script object's data
		scriptObject.ClearObject();

		// call script object's constructor
		const function_t *constructor;
		constructor = scriptObject.GetConstructor();
		if ( constructor ) {
			// start a thread that will initialize after Spawn is done being called
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
			RV_PUSH_HEAP_MEM(this);
// RAVEN END
			scriptThread = new idThread();
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
			RV_POP_HEAP();
// RAVEN END
			scriptThread->ManualDelete();
			scriptThread->ManualControl();
			scriptThread->SetThreadName( name.c_str() );
			scriptThread->CallFunction( this, constructor, true );
			scriptThread->Execute ( );
		}
	
		fl.takedamage = true;

		return;
	}	
// RAVEN END

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
// RAVEN BEGIN
		frameBlend_t frameBlend = { 0, 0, 0, 1.0f, 0 };
		animator.SetFrame( ANIMCHANNEL_ALL, anim, frameBlend );		
// RAVEN END

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
bool idAnimated::LoadAF( const char* keyname ) {
	idStr fileName;

	if ( !keyname || !*keyname ) {
		keyname = "ragdoll";
	}

	if ( !spawnArgs.GetString( keyname, "*unknown*", fileName ) ) {
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
// RAVEN BEGIN
// bdube: script object support
	if ( scriptThread ) {
		CallHandler ( "onActivate" );
		return;
	}
// RAVEN END

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
	
// RAVEN BEGIN
// bdube: with no target bone it will just shoot out the direction of the bones orientation
	if ( targetjoint != INVALID_JOINT ) {
		animator.GetJointTransform( ( jointHandle_t )targetjoint, gameLocal.time, targetPos, axis );
		targetPos = renderEntity.origin + targetPos * renderEntity.axis;
		dir = targetPos - launchPos;
	} else {
		axis *= renderEntity.axis;
		dir = axis[0];
	}
// RAVEN END

	dir.Normalize();

	gameLocal.SpawnEntityDef( *projectileDef, &ent, false );
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !ent || !ent->IsType( idProjectile::GetClassType() ) ) {
// RAVEN END
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
// RAVEN BEGIN
// bdube: invalid is ok now, it means shoot out the direction of the bone
//	if ( target == INVALID_JOINT ) {
//		gameLocal.Warning( "idAnimated '%s' at (%s): unknown target joint '%s'", name.c_str(), GetPhysics()->GetOrigin().ToString(0), targetjoint );
//	}
// RAVEN END

	spawnArgs.Set( "projectilename", projectilename );
	spawnArgs.Set( "missilesound", sound );

	CancelEvents( &EV_LaunchMissilesUpdate );
// RAVEN BEGIN
// nmckenzie: Do this now.  No delays.  No event loops or updates.  Just a straightforward DO THIS.  Tacky.
	if ( numshots == 1 && framedelay == 0 ){
		Event_LaunchMissilesUpdate ( launch, target, 1, 0 );
	}
	else{
		ProcessEvent( &EV_LaunchMissilesUpdate, launch, target, numshots - 1, framedelay );
	}
// RAVEN END
}

// RAVEN BEGIN
// bdube: added
/*
=====================
idAnimated::ShouldConstructScriptObjectAtSpawn
=====================
*/
bool idAnimated::ShouldConstructScriptObjectAtSpawn( void ) const {
	return false;
}

/*
=====================
idAnimated::Think
=====================
*/
void idAnimated::Think ( void ) {
	UpdateScript ( );
	idAFEntity_Gibbable::Think ( );
}

/*
=====================
idAnimated::Think
=====================
*/
void idAnimated::Damage ( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location ) {
	if ( !scriptThread ) {
		return;
	}

	CallHandler ( "onDamage" );	
}

/*
=====================
idAnimated::Event_PlayAnim
=====================
*/
void idAnimated::Event_PlayAnim( int channel, const char *animname ) {
	int anim;
	
	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( blendFrames ) );
		animDoneTime[channel] = 0;
		idThread::ReturnFloat( false );
	} else {
		animator.PlayAnim( channel, anim, gameLocal.time, FRAME2MS( blendFrames ) );
		animDoneTime[channel] = animator.CurrentAnim( channel )->GetEndTime();
		idThread::ReturnFloat( MS2SEC( animDoneTime[channel] - gameLocal.time ) );
	}
	blendFrames = 0;
}

/*
===============
idAnimated::Event_PlayCycle
===============
*/
void idAnimated::Event_PlayCycle( int channel, const char *animname ) {
	int anim;

	anim = animator.GetAnim( animname );
	if ( !anim ) {
		gameLocal.Warning( "missing '%s' animation on '%s' (%s)", animname, name.c_str(), GetEntityDefName() );
		animator.Clear( channel, gameLocal.time, FRAME2MS( blendFrames ) );
		animDoneTime[channel] = 0;
	} else {
		animator.CycleAnim( channel, anim, gameLocal.time, FRAME2MS( blendFrames ) );
		animDoneTime[channel] = animator.CurrentAnim( channel )->GetEndTime();
	}
	blendFrames = 0;
}

/*
===============
idAnimated::Event_AnimDone2
===============
*/
void idAnimated::Event_AnimDone2( int channel, int blend ) {
	if ( animDoneTime[channel] - FRAME2MS( blend ) <= gameLocal.time ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
===============
idAnimated::Event_SetAnimState 
===============
*/
void idAnimated::Event_SetAnimState  ( const char* statename, int blend ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	idealState = statename;
	blendFrames = blend;
	scriptThread->DoneProcessing();
}

/*
================
idAnimated::UpdateScript
================
*/
void idAnimated::UpdateScript( void ) {
	int	count;

	if ( !scriptThread || !gameLocal.isNewFrame  ) {
		return;
	}

	if ( idealState.Length() ) {
		SetState( idealState, blendFrames );
	}
	
	// If no state has been set then dont execute nothing
	if ( !state.Length ( ) || scriptThread->IsWaiting() ) {
		return;
	}

	// update script state
	count = 10;
	while( ( scriptThread->Execute() || idealState.Length() ) && count-- ) {
		if ( idealState.Length() ) {
			SetState( idealState, blendFrames );
		}
	}
}

/*
=====================
idAnimated::CallHandler
=====================
*/
void idAnimated::CallHandler ( const char* handler ) {
	const function_t *func;
	func = scriptObject.GetFunction( handler );
	if ( !func ) {
		return;
	}

	scriptThread->CallFunction( this, func, false );
	scriptThread->Execute ( );
}

/*
=====================
idAnimated::SetState
=====================
*/
void idAnimated::SetState( const char *statename, int frames ) {
	const function_t *func;

	func = scriptObject.GetFunction( statename );
	if ( !func ) {
		gameLocal.Error( "Can't find function '%s' in object '%s'", statename, scriptObject.GetTypeName() );
	}

	scriptThread->CallFunction( this, func, true );
	state = statename;
	blendFrames = frames;
	idealState = "";
}

// RAVEN END

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
	bool hidden;
	bool keepContents;

// RAVEN BEGIN
// rjohnson: inline models can be solid, since the entities hang around anyway (don't know why it was done that way)
	solid = spawnArgs.GetBool( "solid" );

	// an inline static model will not do anything at all
	if ( spawnArgs.GetBool( "inline" ) || gameLocal.world->spawnArgs.GetBool( "inlineAllStatics" ) ) {
		Hide();
		if ( solid ) {
			GetPhysics()->SetContents( CONTENTS_SOLID );
		}
		return;
	}
// RAVEN END

	hidden = spawnArgs.GetBool( "hide" );
	keepContents = spawnArgs.GetBool( "KeepContents" );

	if ( keepContents ) {
		// do nothing, keep the contents from the model
	} else if ( solid && !hidden ) {
		GetPhysics()->SetContents( CONTENTS_SOLID );
	} else {
		GetPhysics()->SetContents( 0 );
	}

	spawnTime = gameLocal.time;
	active = false;

	idStr model = spawnArgs.GetString( "model" );
	// FIXME: temp also catch obsolete .ips extension
	if ( model.Find( ".ips" ) >= 0 || model.Find( ".prt" ) >= 0 ) {
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
// RAVEN BEGIN
// bdube: not using
//	common->InitTool( EDITOR_PARTICLE, &spawnArgs );
// RAVEN END
}
/*
================
idStaticEntity::Think
================
*/
void idStaticEntity::Think( void ) {
	idEntity::Think();
	if ( thinkFlags & TH_THINK ) {
		if ( runGui && renderEntity.gui[0] ) {
			idPlayer *player = gameLocal.GetLocalPlayer();
			if ( player ) {
				if ( !player->objectiveSystemOpen ) {
					for( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
						if ( renderEntity.gui[ i ] ) {
							renderEntity.gui[ i ]->StateChanged( gameLocal.time, true );
						}
					}
				}
			}
		}
		if ( fadeEnd > 0 ) {
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
	if ( spawnArgs.GetBool( "solid" ) ) {
		GetPhysics()->SetContents( CONTENTS_SOLID );
	}
}

/*
================
idStaticEntity::Event_Activate
================
*/
void idStaticEntity::Event_Activate( idEntity *activator ) {
	idStr activateGui;

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

idFuncEmitter

===============================================================================
*/


CLASS_DECLARATION( idStaticEntity, idFuncEmitter )
EVENT( EV_Activate,				idFuncEmitter::Event_Activate )
END_CLASS

/*
===============
idFuncEmitter::idFuncEmitter
===============
*/
idFuncEmitter::idFuncEmitter( void ) {
	hidden = false;
}

/*
===============
idFuncEmitter::Spawn
===============
*/
void idFuncEmitter::Spawn( void ) {
	if ( spawnArgs.GetBool( "start_off" ) ) {
		hidden = true;
		renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC( 1 );
		UpdateVisuals();
	} else {
		hidden = false;
	}
}

/*
===============
idFuncEmitter::Save
===============
*/
void idFuncEmitter::Save( idSaveGame *savefile ) const {
	savefile->WriteBool( hidden );
}

/*
===============
idFuncEmitter::Restore
===============
*/
void idFuncEmitter::Restore( idRestoreGame *savefile ) {
	savefile->ReadBool( hidden );
}

/*
================
idFuncEmitter::Event_Activate
================
*/
void idFuncEmitter::Event_Activate( idEntity *activator ) {
	if ( hidden || spawnArgs.GetBool( "cycleTrigger" ) ) {
		renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = 0;
		renderEntity.shaderParms[SHADERPARM_TIMEOFFSET] = -MS2SEC( gameLocal.time );
		hidden = false;
	} else {
		renderEntity.shaderParms[SHADERPARM_PARTICLE_STOPTIME] = MS2SEC( gameLocal.time );
		hidden = true;
	}
	UpdateVisuals();
}

/*
================
idFuncEmitter::WriteToSnapshot
================
*/
void idFuncEmitter::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( hidden ? 1 : 0, 1 );
	msg.WriteFloat( renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] );
	msg.WriteFloat( renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] );
}

/*
================
idFuncEmitter::ReadFromSnapshot
================
*/
void idFuncEmitter::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	hidden = msg.ReadBits( 1 ) != 0;
	renderEntity.shaderParms[ SHADERPARM_PARTICLE_STOPTIME ] = msg.ReadFloat();
	renderEntity.shaderParms[ SHADERPARM_TIMEOFFSET ] = msg.ReadFloat();
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}


/*
===============================================================================

idFuncSplat

===============================================================================
*/


const idEventDef EV_Splat( "<Splat>" );
CLASS_DECLARATION( idFuncEmitter, idFuncSplat )
EVENT( EV_Activate,		idFuncSplat::Event_Activate )
EVENT( EV_Splat,		idFuncSplat::Event_Splat )
END_CLASS

/*
===============
idFuncSplat::idFuncSplat
===============
*/
idFuncSplat::idFuncSplat( void ) {
}

/*
===============
idFuncSplat::Spawn
===============
*/
void idFuncSplat::Spawn( void ) {
}

/*
================
idFuncSplat::Event_Splat
================
*/
void idFuncSplat::Event_Splat( void ) {
	const char *splat = NULL;
	int count = spawnArgs.GetInt( "splatCount", "1" );
	for ( int i = 0; i < count; i++ ) {
		splat = spawnArgs.RandomPrefix( "mtr_splat", gameLocal.random );
		if ( splat && *splat ) {
			float size = spawnArgs.GetFloat( "splatSize", "128" );
			float dist = spawnArgs.GetFloat( "splatDistance", "128" );
			float angle = spawnArgs.GetFloat( "splatAngle", "0" );
			gameLocal.ProjectDecal( GetPhysics()->GetOrigin(), GetPhysics()->GetAxis()[2], dist, true, size, splat, angle );
		}
	}
	StartSound( "snd_splat", SND_CHANNEL_ANY, 0, false, NULL );
}

/*
================
idFuncSplat::Event_Activate
================
*/
void idFuncSplat::Event_Activate( idEntity *activator ) {
	idFuncEmitter::Event_Activate( activator );
	PostEventSec( &EV_Splat, spawnArgs.GetFloat( "splatDelay", "0.25" ) );
	StartSound( "snd_spurt", SND_CHANNEL_ANY, 0, false, NULL );
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
void idTextEntity::Spawn( void ) {
	// these are cached as the are used each frame
	text = spawnArgs.GetString( "text" );
	playerOriented = spawnArgs.GetBool( "playerOriented" );
	bool force = spawnArgs.GetBool( "force" );
	if ( developer.GetBool() || force ) {
		BecomeActive(TH_THINK);
	}
}

/*
================
idTextEntity::Save
================
*/
void idTextEntity::Save( idSaveGame *savefile ) const {
	savefile->WriteString( text );
	savefile->WriteBool( playerOriented );
}

/*
================
idTextEntity::Restore
================
*/
void idTextEntity::Restore( idRestoreGame *savefile ) {
	savefile->ReadString( text );
	savefile->ReadBool( playerOriented );
}

/*
================
idTextEntity::Think
================
*/
void idTextEntity::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		gameRenderWorld->DrawText( text, GetPhysics()->GetOrigin(), 0.25, colorWhite, playerOriented ? gameLocal.GetLocalPlayer()->viewAngles.ToMat3() : GetPhysics()->GetAxis().Transpose(), 1 );
		for ( int i = 0; i < targets.Num(); i++ ) {
			if ( targets[i].GetEntity() ) {
				gameRenderWorld->DebugArrow( colorBlue, GetPhysics()->GetOrigin(), targets[i].GetEntity()->GetPhysics()->GetOrigin(), 1 );
			}
		}
	} else {
		BecomeInactive( TH_ALL );
	}
}

/*
================
idTextEntity::Think
================
*/
void idTextEntity::ClientPredictionThink( void ) {
	if ( thinkFlags & TH_THINK ) {
		gameRenderWorld->DrawText( text, GetPhysics()->GetOrigin(), 0.25, colorWhite, playerOriented ? gameLocal.GetLocalPlayer()->viewAngles.ToMat3() : GetPhysics()->GetAxis().Transpose(), 1 );
		for ( int i = 0; i < targets.Num(); i++ ) {
			if ( targets[i].GetEntity() ) {
				gameRenderWorld->DebugArrow( colorBlue, GetPhysics()->GetOrigin(), targets[i].GetEntity()->GetPhysics()->GetOrigin(), 1 );
			}
		}
	} else {
		BecomeInactive( TH_ALL );
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

	b = idBounds( spawnArgs.GetVector( "origin" ) ).Expand( 16 );
	portal = gameRenderWorld->FindPortal( b );
	if ( !portal ) {
		gameLocal.Warning( "VacuumSeparator '%s' didn't contact a portal", spawnArgs.GetString( "name" ) );
		return;
	}
	gameLocal.SetPortalState( portal, PS_BLOCK_AIR | PS_BLOCK_LOCATION );
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

idLocationSeparatorEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, idLocationSeparatorEntity )
END_CLASS

/*
================
idLocationSeparatorEntity::Spawn
================
*/
void idLocationSeparatorEntity::Spawn() {
	idBounds b;

	b = idBounds( spawnArgs.GetVector( "origin" ) ).Expand( 16 );
	qhandle_t portal = gameRenderWorld->FindPortal( b );
	if ( !portal ) {
		gameLocal.Warning( "LocationSeparator '%s' didn't contact a portal", spawnArgs.GetString( "name" ) );
	}
	gameLocal.SetPortalState( portal, PS_BLOCK_LOCATION );
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

	gameLocal.vacuumAreaNum = gameRenderWorld->PointInArea( org );
}

// RAVEN BEGIN
// abahr:
/*
===============================================================================

	rvGravitySeparatorEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, rvGravitySeparatorEntity )
	EVENT( EV_Activate,		rvGravitySeparatorEntity::Event_Activate )
END_CLASS


/*
================
rvGravitySeparatorEntity::idVacuumSeparatorEntity
================
*/
rvGravitySeparatorEntity::rvGravitySeparatorEntity( void ) {
	portal = 0;
}

/*
================
rvGravitySeparatorEntity::Save
================
*/
void rvGravitySeparatorEntity::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( (int)portal );
	savefile->WriteInt( gameRenderWorld->GetPortalState( portal ) );
}

/*
================
rvGravitySeparatorEntity::Restore
================
*/
void rvGravitySeparatorEntity::Restore( idRestoreGame *savefile ) {
	int state;

	savefile->ReadInt( (int &)portal );
	savefile->ReadInt( state );

	gameLocal.SetPortalState( portal, state );
}

/*
================
rvGravitySeparatorEntity::Spawn
================
*/
void rvGravitySeparatorEntity::Spawn() {
	idBounds b;

	b = idBounds( spawnArgs.GetVector( "origin" ) ).Expand( 16 );
	portal = gameRenderWorld->FindPortal( b );
	if ( !portal ) {
		gameLocal.Warning( "GravitySeparator '%s' didn't contact a portal", spawnArgs.GetString( "name" ) );
		return;
	}
	gameLocal.SetPortalState( portal, gameRenderWorld->GetPortalState(portal) | PS_BLOCK_GRAVITY );
}

/*
================
rvGravitySeparatorEntity::Event_Activate
================
*/
void rvGravitySeparatorEntity::Event_Activate( idEntity *activator ) {
	if ( !portal ) {
		return;
	}

	int portalState = gameRenderWorld->GetPortalState( portal );
	gameLocal.SetPortalState( portal, (portalState & PS_BLOCK_GRAVITY) > 0 ? (portalState & ~PS_BLOCK_GRAVITY) : portalState | PS_BLOCK_GRAVITY );
}

/*
===============================================================================

	rvGravityEntity

===============================================================================
*/
ABSTRACT_DECLARATION( idEntity, rvGravityArea )
END_CLASS

/*
================
rvGravityArea::Spawn
================
*/
void rvGravityArea::Spawn() {
	area = gameRenderWorld->PointInArea( spawnArgs.GetVector("origin") );
	gameLocal.AddUniqueGravityInfo( this );
}

/*
================
rvGravityArea::Save
================
*/
void rvGravityArea::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( area );
}

/*
================
rvGravityArea::Restore
================
*/
void rvGravityArea::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( area );
}

/*
================
rvGravityArea::IsEqualTo
================
*/
bool rvGravityArea::IsEqualTo( const rvGravityArea* area ) const {
	assert( area );
	return !idStr::Icmp( GetName(), area->GetName() );
}

/*
================
rvGravityArea::operator==
================
*/
bool rvGravityArea::operator==( const rvGravityArea* area ) const {
	return IsEqualTo( area );
}

/*
================
rvGravityArea::operator==
================
*/
bool rvGravityArea::operator==( const rvGravityArea& area ) const {
	return IsEqualTo( &area );
}

/*
================
rvGravityArea::operator!=
================
*/
bool rvGravityArea::operator!=( const rvGravityArea* area ) const {
	return !IsEqualTo( area );
}

/*
================
rvGravityArea::operator!=
================
*/
bool rvGravityArea::operator!=( const rvGravityArea& area ) const {
	return !IsEqualTo( &area );
}

/*
===============================================================================

	rvGravityEntity_Static

===============================================================================
*/
CLASS_DECLARATION( rvGravityArea, rvGravityArea_Static )
END_CLASS

/*
================
rvGravityArea_Static::Spawn
================
*/
void rvGravityArea_Static::Spawn() {
	gravity = spawnArgs.GetVector( "gravity" );// * gameLocal.GetGravity().Length();
}

/*
================
rvGravityArea_Static::Save
================
*/
void rvGravityArea_Static::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( gravity );
}

/*
================
rvGravityArea_Static::Restore
================
*/
void rvGravityArea_Static::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( gravity );
}

/*
===============================================================================

	rvGravityArea_SurfaceNormal

===============================================================================
*/
CLASS_DECLARATION( rvGravityArea, rvGravityArea_SurfaceNormal )
END_CLASS

/*
================
rvGravityArea_SurfaceNormal::GetGravity
================
*/
const idVec3 rvGravityArea_SurfaceNormal::GetGravity( const idVec3& origin, const idMat3& axis, int clipMask, idEntity* passEntity ) const {
	trace_t results;

// RAVEN BEGIN
// ddynerman: multiple clip worlds
	if( !gameLocal.TracePoint( this, results, origin, origin + -axis[2] * 50.0f, clipMask, passEntity ) ) {
// RAVEN END
		return gameLocal.GetGravity();
	}

	return -results.c.normal * gameLocal.GetGravity().Length();
}

/*
================
rvGravityArea_SurfaceNormal::GetGravity
================
*/
const idVec3 rvGravityArea_SurfaceNormal::GetGravity( const idEntity* ent ) const {
	return GetGravity( ent->GetPhysics() );
}

/*
================
rvGravityArea_SurfaceNormal::GetGravity
================
*/
const idVec3 rvGravityArea_SurfaceNormal::GetGravity( const rvClientEntity* ent ) const {
	return GetGravity( ent->GetPhysics() );
}

/*
================
rvGravityArea_SurfaceNormal::GetGravity
================
*/
const idVec3 rvGravityArea_SurfaceNormal::GetGravity( const idPhysics* physics ) const {
	return GetGravity( physics->GetOrigin(), physics->GetAxis(), physics->GetClipMask(), physics->GetSelf() );
}
// RAVEN END


/*
===============================================================================

idLocationEntity

===============================================================================
*/

CLASS_DECLARATION( idEntity, idLocationEntity )
END_CLASS

/*
======================
idLocationEntity::Spawn
======================
*/
void idLocationEntity::Spawn() {
	idStr realName;

	// this just holds dict information

	// if "location" not already set, use the entity name.
	if ( !spawnArgs.GetString( "location", "", realName ) ) {
		spawnArgs.Set( "location", name );
	}
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
	// cnicholson: It says nothing to save here, no mention of *model
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


idShaking::~idShaking() { 
	SetPhysics( NULL );
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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	physicsObj.SetClipModel( new idClipModel( GetPhysics()->GetClipModel() ), 1.0f );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END
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
	period = spawnArgs.GetFloat( "period", "0.05" ) * 1000;
	physicsObj.SetAngularExtrapolation( extrapolation_t(EXTRAPOLATION_DECELSINE|EXTRAPOLATION_NOSTOP), phase, period * 0.25f, GetPhysics()->GetAxis().ToAngles(), shake, ang_zero );
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
idCVar g_earthquake( "g_earthquake", "1", 0, "controls earthquake effect" );

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

// RAVEN BEGIN
// kfuller: look for fx entities and the like that may want to be triggered when a mortar round (aka earthquake) goes off
	AffectNearbyEntities(spawnArgs.GetFloat("affectEntities", "0"));
// RAVEN END

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

// RAVEN BEGIN
// kfuller: look for fx entities and the like that may want to be triggered when a mortar round, earthquake, etc goes off
void idEarthQuake::AffectNearbyEntities(float affectRadius)
{
	if( !g_earthquake.GetBool() ) 
	{
		return;
	}

	if (affectRadius == 0)
	{
		return;
	}
	idVec3		affectOrigin(GetPhysics()->GetOrigin());
	
	if (playerOriented)
	{
		idPlayer *player = gameLocal.GetLocalPlayer();
		if ( player == NULL )
		{
			return;
		}
		affectOrigin = player->GetPhysics()->GetOrigin();
	}

	idEntity *	entityList[ MAX_GENTITIES ];
	int			numListedEntities = 0;
	idEntity	*curEnt = NULL;
	bool		requiresLOS = spawnArgs.GetBool("requiresLOS", "false");

	// get all entities touching the bounds
	numListedEntities = gameLocal.EntitiesWithinRadius(affectOrigin, affectRadius, entityList, MAX_GENTITIES );
	for (int i = 0; i < numListedEntities; i++)
	{
		// only affect certain entities. first check if they have the "quakeChance" key/value, then send them an earthquake event
		curEnt = entityList[i];
		if (curEnt && curEnt->spawnArgs.GetFloat("quakeChance"))
		{
			curEnt->PostEventMS(&EV_Earthquake, 0, (float)requiresLOS);
		}
	}
}
// RAVEN END

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
		float shakeVolume = soundSystem->CurrentShakeAmplitudeForPosition( SOUNDWORLD_GAME, gameLocal.time, gameLocal.GetLocalPlayer()->firstPersonViewOrigin );
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
idFuncPortal::idFuncPortal() {
	portal = 0;
	state = false;
}

/*
===============
idFuncPortal::Save
===============
*/
void idFuncPortal::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( (int)portal );
	savefile->WriteBool( state );
}

/*
===============
idFuncPortal::Restore
===============
*/
void idFuncPortal::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( (int &)portal );
	savefile->ReadBool( state );
	gameLocal.SetPortalState( portal, state ? PS_BLOCK_ALL : PS_BLOCK_NONE );
}

/*
===============
idFuncPortal::Spawn
===============
*/
void idFuncPortal::Spawn( void ) {
	portal = gameRenderWorld->FindPortal( GetPhysics()->GetAbsBounds().Expand( 32.0f ) );
	if ( portal > 0 ) {
		state = spawnArgs.GetBool( "start_on" );
		gameLocal.SetPortalState( portal, state ? PS_BLOCK_ALL : PS_BLOCK_NONE );
	}
}

/*
================
idFuncPortal::Event_Activate
================
*/
void idFuncPortal::Event_Activate( idEntity *activator ) {
	if ( portal > 0 ) {
		state = !state;
		gameLocal.SetPortalState( portal, state ? PS_BLOCK_ALL : PS_BLOCK_NONE );
	}
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
}

/*
================
idFuncAASObstacle::Event_Activate
================
*/
void idFuncAASObstacle::Event_Activate( idEntity *activator ) {
	state ^= 1;
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_OBSTACLE, state );
}

/*
================
idFuncAASObstacle::SetState
================
*/
void idFuncAASObstacle::SetState ( bool _state ) {
	state = _state;
	gameLocal.SetAASAreaState( GetPhysics()->GetAbsBounds(), AREACONTENTS_OBSTACLE, state );
}

/*
===============================================================================

idFuncRadioChatter

===============================================================================
*/

const idEventDef EV_ResetRadioHud( "<resetradiohud>", "e" );
idEntityPtr<idFuncRadioChatter> idFuncRadioChatter::lastRadioChatter = 0;

CLASS_DECLARATION( idEntity, idFuncRadioChatter )
EVENT( EV_Activate,				idFuncRadioChatter::Event_Activate )
EVENT( EV_ResetRadioHud,		idFuncRadioChatter::Event_ResetRadioHud )
EVENT( EV_IsActive,				idFuncRadioChatter::Event_IsActive )
END_CLASS

/*
===============
idFuncRadioChatter::idFuncRadioChatter
===============
*/
idFuncRadioChatter::idFuncRadioChatter() {
	time = 0.0f;
	isActive = false;
}

/*
===============
idFuncRadioChatter::Save
===============
*/
void idFuncRadioChatter::Save( idSaveGame *savefile ) const {
	savefile->WriteFloat( time );
	savefile->WriteBool( isActive ); // cnicholson: Added unsaved var
	lastRadioChatter.Save ( savefile );
}

/*
===============
idFuncRadioChatter::Restore
===============
*/
void idFuncRadioChatter::Restore( idRestoreGame *savefile ) {
	savefile->ReadFloat( time );
	savefile->ReadBool( isActive ); // cnicholson: Added unrestored var
	lastRadioChatter.Restore ( savefile );
}

/*
===============
idFuncRadioChatter::Spawn
===============
*/
void idFuncRadioChatter::Spawn( void ) {
	time = spawnArgs.GetFloat( "time", "5.0" );
}

/*
================
idFuncRadioChatter::Event_Activate
================
*/
void idFuncRadioChatter::Event_Activate( idEntity *activator ) {
	idPlayer *player;
	const char	*sound;
	const idSoundShader *shader;
	int length;
	bool isDirectedAtPlayer = false;	

	lastRadioChatter = this;

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( activator && activator->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
		player = static_cast<idPlayer *>( activator );
	} else {
		player = gameLocal.GetLocalPlayer();
	}

// RAVEN BEGIN
// bdube: replaced named event with method
	player->StartRadioChatter();
// twhitaker: for isActive() script event
	isActive = true;
// RAVEN END

	sound = spawnArgs.GetString( "snd_radiochatter", "" );
	if ( sound && *sound ) {
		shader = declManager->FindSound( sound );
		if ( shader && shader->IsVO_ForPlayer()){
			isDirectedAtPlayer = true;
		}
		player->StartSoundShader( shader, SND_CHANNEL_RADIO, SSF_GLOBAL, false, &length );
		time = MS2SEC( length + 150 );
	}

	if ( player && player->GetHud() ) {
		player->GetHud()->SetStateBool( "rhinochatter", isDirectedAtPlayer );
	}

	// we still put the hud up because this is used with no sound on 
	// certain frame commands when the chatter is triggered
	PostEventSec( &EV_ResetRadioHud, time, player );
}

/*
================
idFuncRadioChatter::Event_ResetRadioHud
================
*/
void idFuncRadioChatter::Event_ResetRadioHud( idEntity *activator ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	idPlayer *player = ( activator->IsType( idPlayer::GetClassType() ) ) ? static_cast<idPlayer *>( activator ) : gameLocal.GetLocalPlayer();
// RAVEN END
// RAVEN BEGIN
// bdube: replaced named event with method
	player->StopRadioChatter();
// twhitaker: for isActive() script event
	isActive = false;
// RAVEN END
	ActivateTargets( activator );
}

/*
================
idFuncRadioChatter::Event_ResetRadioHud
================
*/
void idFuncRadioChatter::Event_IsActive( void ) {
	idThread::ReturnFloat( static_cast< float >( isActive ) );
}

/*
================
idFuncRadioChatter::RepeatLast
================
*/
void idFuncRadioChatter::RepeatLast( void ) {
	if ( lastRadioChatter ) {
		lastRadioChatter->Event_Activate( gameLocal.GetLocalPlayer() );
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

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !activator || !activator->IsType( idActor::GetClassType() ) ) {
// RAVEN END
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
		targetTime[ i ] = gameLocal.time + SEC2MS( shake_time )+ targetTime[ i ] * scale;
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

// RAVEN BEGIN
// ddynerman: multiple clip worlds
		gameLocal.TracePoint( this, tr, entOrg, toPos, MASK_OPAQUE, ent );
// RAVEN END
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			if ( ent->IsType( idMoveable::GetClassType() ) ) {
// RAVEN END
				idMoveable *ment = static_cast<idMoveable*>( ent );
				ment->EnableDamage( true, 2.5f );
			}
		} else {
			// this is not the right way to set the angular velocity, but the effect is nice, so I'm keeping it. :)
			ang.Set( gameLocal.random.CRandomFloat() * shake_ang.x, gameLocal.random.CRandomFloat() * shake_ang.y, gameLocal.random.CRandomFloat() * shake_ang.z );
			ang *= ( 1.0f - time / shake_time );
			entPhys->SetAngularVelocity( ang );
		}
	}

	if ( !num ) {
		BecomeInactive( TH_THINK );
	}
}

// RAVEN BEGIN
// bdube: jump points

/*
===============================================================================

rvDebugJumpPoint

===============================================================================
*/

CLASS_DECLARATION( idEntity, rvDebugJumpPoint )
END_CLASS

/*
=====================
rvDebugJumpPoint::Spawn
=====================
*/
void rvDebugJumpPoint::Spawn() {
	// Add the jump point
	gameDebug.JumpAdd ( name, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis().ToAngles ( ) );
	
	// Dont need the entity any more
	PostEventMS( &EV_Remove, 0 );
}


/*
===============================================================================

rvFuncSaveGame

===============================================================================
*/

CLASS_DECLARATION( idEntity, rvFuncSaveGame )
	EVENT( EV_Activate,		rvFuncSaveGame::Event_Activate )
END_CLASS

void rvFuncSaveGame::Spawn( void ) {

}

void rvFuncSaveGame::Event_Activate( idEntity *activator ) {
	// TODO: Xenon - request the player to save the game.
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "saveGame checkPoint" );
}
