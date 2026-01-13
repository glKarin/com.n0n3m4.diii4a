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
#include "renderer/RenderSystem.h"

#include "framework/DeclEntityDef.h"
#include "gamesys/SysCvar.h"
#include "Player.h"
#include "Fx.h"
#include "SmokeParticles.h"

#include "Item.h"

/*
===============================================================================

  idItem

===============================================================================
*/

const idEventDef EV_DropToFloor( "<dropToFloor>" );
const idEventDef EV_RespawnItem( "respawn" );
const idEventDef EV_RespawnFx( "<respawnFx>" );
const idEventDef EV_GetPlayerPos( "<getplayerpos>" );
const idEventDef EV_HideObjective( "<hideobjective>", "e" );
const idEventDef EV_CamShot( "<camshot>" );
const idEventDef EV_SetNextState( "setNextState", "s" );
const idEventDef EV_SetState( "setState", "s" );
const idEventDef EV_GetState( "getState", NULL, 's' );
// HEXEN : Zeroth
const idEventDef EV_ArtifactStart( "ArtifactStart" );
const idEventDef EV_ArtifactCoolDown( "ArtifactCoolDown" );
const idEventDef EV_ArtifactDone( "ArtifactDone" );
const idEventDef EV_SetArtifactActive( "ArtifactActive", "f" );
const idEventDef EV_OwnerLaunchProjectiles( "OwnerLaunchProjectiles", "dffff" );
const idEventDef EV_OwnerCreateProjectile( "OwnerCreateProjectile", NULL, 'e' );
const idEventDef EV_GetOwner( "GetOwner", NULL, 'e' );
const idEventDef EV_HideMultiModel( "HideMultiModel" );

CLASS_DECLARATION( idEntity, idItem )
	EVENT( EV_DropToFloor,		idItem::Event_DropToFloor )
	EVENT( EV_Touch,			idItem::Event_Touch )
	EVENT( EV_Activate,			idItem::Event_Trigger )
	EVENT( EV_RespawnItem,		idItem::Event_Respawn )
	EVENT( EV_RespawnFx,		idItem::Event_RespawnFx )
//	EVENT( EV_SetNextState,		idItem::Event_SetNextState )
//	EVENT( EV_SetState,			idItem::Event_SetState )
//	EVENT( EV_GetState,			idItem::Event_GetState )
// HEXEN : Zeroth
	EVENT( EV_ArtifactStart,			idItem::Event_ArtifactStart )
	EVENT( EV_ArtifactDone,				idItem::Event_ArtifactDone )
	EVENT( EV_ArtifactCoolDown,			idItem::Event_ArtifactCoolDown )
	EVENT( EV_SetArtifactActive,		idItem::Event_SetArtifactActive )
	EVENT( EV_OwnerLaunchProjectiles,	idItem::Event_OwnerLaunchProjectiles )
	EVENT( EV_OwnerCreateProjectile,	idItem::Event_OwnerCreateProjectile )
	EVENT( EV_GetOwner,					idItem::Event_GetOwner )
	EVENT( EV_HideMultiModel,			idItem::Event_HideMultiModel )
END_CLASS


/*
================
idItem::idItem
================
*/
idItem::idItem() {
	spin = false;
	inView = false;
	inViewTime = 0;
	lastCycle = 0;
	lastRenderViewTime = -1;
	itemShellHandle = -1;
	shellMaterial = NULL;
	orgOrigin.Zero();
	canPickUp = true;
	fl.networkSync = true;

	owner = NULL;
	DeleteMe = false;
	Cooling = false;
	ArtifactActive = false;
	Processing = false;
}

/*
================
idItem::~idItem
================
*/
idItem::~idItem() {
	// remove the highlight shell
	if ( itemShellHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( itemShellHandle );
	}

	if ( multimodel ) {
		delete multimodel;
	}

	//z.todo: necessary???
	// 	delete scriptThread;
	// 	DeconstructScriptObject();
	// 	scriptObject.Free();
}

/*
================
HEXEN
idItem::Hide
================
*/
void idItem::Hide( void ) {
	if ( !idEntity::IsHidden() ) {
		idEntity::Hide();
		if ( multimodel ) {
			multimodel->Hide();
		}
	}
}

/*
================
HEXEN
idItem::Event_ArtifactStart
================
*/
void idItem::Event_ArtifactStart( void ) {
	if ( owner != NULL ) {
       	Processing = true;

		owner->RemoveInventoryItem(spawnArgs.GetString("inv_name"));
		owner->UpdateHudArtifacts();
	}
}

/*
================
HEXEN
idItem::Event_ArtifactDone
================
*/
void idItem::Event_ArtifactDone( void ) {
	if ( owner != NULL ) {
		Cooling = false;
		DeleteMe = true;
	}
}

/*
================
HEXEN
idItem::Event_ArtifactCoolDown
================
*/
void idItem::Event_ArtifactCoolDown( void ) {
	if ( owner != NULL ) {
		Cooling = true;
		owner->UpdateHudActiveArtifacts();
	}
}

/*
================
HEXEN
idItem::Event_SetArtifactActive
================
*/
void idItem::Event_SetArtifactActive( const float yesorno ) {
	if ( owner != NULL ) {
		if ( yesorno ) {
			ArtifactActive = true;
			owner->UpdateHudActiveArtifacts();
		} else {
			ArtifactActive = false;
			owner->UpdateHudActiveArtifacts();
		}
	}
}

/*
================
HEXEN
idItem::Event_GetOwner
================
*/
void idItem::Event_GetOwner( void ) {
	idThread::ReturnEntity( owner );
}

/*
================
idItem::Save
================
*/
void idItem::Save( idSaveGame *savefile ) const {

	savefile->WriteVec3( orgOrigin );
	savefile->WriteBool( spin );
	savefile->WriteBool( pulse );
	savefile->WriteBool( canPickUp );

	savefile->WriteMaterial( shellMaterial );

	savefile->WriteBool( inView );
	savefile->WriteInt( inViewTime );
	savefile->WriteInt( lastCycle );
	savefile->WriteInt( lastRenderViewTime );

	// HEXEN : Zeroth
	savefile->WriteObject( scriptThread );
	savefile->WriteString( waitState );

	//FIXME: this is unneccesary
	idToken token;
	// HEXEN : Zeroth
	if ( state ) {
		idLexer src( state->Name(), idStr::Length( state->Name() ), "idItem::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

	// HEXEN : Zeroth
	if ( idealState ) {
		idLexer src( idealState->Name(), idStr::Length( idealState->Name() ), "idItem::Save" );

		src.ReadTokenOnLine( &token );
		src.ExpectTokenString( "::" );
		src.ReadTokenOnLine( &token );

		savefile->WriteString( token );
	} else {
		savefile->WriteString( "" );
	}

	// HEXEN : Zeroth
	savefile->WriteObject( projectileEnt );
	savefile->WriteObject( multimodel );
}

/*
================
idItem::Restore
================
*/
void idItem::Restore( idRestoreGame *savefile ) {

	savefile->ReadVec3( orgOrigin );
	savefile->ReadBool( spin );
	savefile->ReadBool( pulse );
	savefile->ReadBool( canPickUp );

	savefile->ReadMaterial( shellMaterial );

	savefile->ReadBool( inView );
	savefile->ReadInt( inViewTime );
	savefile->ReadInt( lastCycle );
	savefile->ReadInt( lastRenderViewTime );

	itemShellHandle = -1;

	// HEXEN : Zeroth
	savefile->ReadObject( reinterpret_cast<idClass *&>( scriptThread ) );
	savefile->ReadString( waitState );
	idStr statename;

	// HEXEN : Zeroth
	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		state = GetScriptFunction( statename );
	}

	// HEXEN : Zeroth
	savefile->ReadString( statename );
	if ( statename.Length() > 0 ) {
		idealState = GetScriptFunction( statename );
	}

	// HEXEN : Zeroth
	const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( spawnArgs.GetString( "def_projectile" ), false );
	if ( projectileDef ) {
		projectileDict = projectileDef->dict;
	} else {
		projectileDict.Clear();
	}

	// HEXEN : Zeroth
	savefile->ReadObject( reinterpret_cast<idClass *&>( projectileEnt ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( multimodel ) );
}

/*
================
idItem::UpdateRenderEntity
================
*/
bool idItem::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const {

	if ( lastRenderViewTime == renderView->time ) {
		return false;
	}

	lastRenderViewTime = renderView->time;

	// check for glow highlighting if near the center of the view
	idVec3 dir = renderEntity->origin - renderView->vieworg;
	dir.Normalize();
	float d = dir * renderView->viewaxis[0];

	// two second pulse cycle
	float cycle = ( renderView->time - inViewTime ) / 2000.0f;

	if ( d > 0.94f ) {
		if ( !inView ) {
			inView = true;
			if ( cycle > lastCycle ) {
				// restart at the beginning
				inViewTime = renderView->time;
				cycle = 0.0f;
			}
		}
	} else {
		if ( inView ) {
			inView = false;
			lastCycle = ceil( cycle );
		}
	}

	// fade down after the last pulse finishes
	if ( !inView && cycle > lastCycle ) {
		renderEntity->shaderParms[4] = 0.0f;
	} else {
		// pulse up in 1/4 second
		cycle -= (int)cycle;
		if ( cycle < 0.1f ) {
			renderEntity->shaderParms[4] = cycle * 10.0f;
		} else if ( cycle < 0.2f ) {
			renderEntity->shaderParms[4] = 1.0f;
		} else if ( cycle < 0.3f ) {
			renderEntity->shaderParms[4] = 1.0f - ( cycle - 0.2f ) * 10.0f;
		} else {
			// stay off between pulses
			renderEntity->shaderParms[4] = 0.0f;
		}
	}

	// update every single time this is in view
	return true;
}

/*
================
idItem::ModelCallback
================
*/
bool idItem::ModelCallback( renderEntity_t *renderEntity, const renderView_t *renderView ) {
	const idItem *ent;

	// this may be triggered by a model trace or other non-view related source
	if ( !renderView ) {
		return false;
	}

	ent = static_cast<idItem *>(gameLocal.entities[ renderEntity->entityNum ]);
	if ( !ent ) {
		gameLocal.Error( "idItem::ModelCallback: callback with NULL game entity" );
	}

	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
idItem::Think
================
*/
void idItem::Think( void ) {
	if ( thinkFlags & TH_THINK ) {
		if ( spin ) {
			idAngles	ang;
			idVec3		org;

			ang.pitch = ang.roll = 0.0f;
			ang.yaw = ( gameLocal.time & 4095 ) * 360.0f / -4096.0f;
			SetAngles( ang );

			float scale = 0.005f;

			org = orgOrigin;
			org.z += 4.0f + cos( ( gameLocal.time + 2000 ) * scale ) * 4.0f;
			SetOrigin( org );
		}
	}

	Present();

	if ( !IsHidden() && multimodel ) {
		multimodel->GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() ); // bob up and down in unison
		multimodel->SetAngles( -GetAngles() ); // make it spin the opposite direction
	}
}

/*
================
idItem::Present
================
*/
void idItem::Present( void ) {
	idEntity::Present();

	if ( !fl.hidden && pulse ) {
		// also add a highlight shell model
		renderEntity_t	shell;

		shell = renderEntity;

		// we will mess with shader parms when the item is in view
		// to give the "item pulse" effect
		shell.callback = idItem::ModelCallback;
		shell.entityNum = entityNumber;
		shell.customShader = shellMaterial;
		if ( itemShellHandle == -1 ) {
			itemShellHandle = gameRenderWorld->AddEntityDef( &shell );
		} else {
			gameRenderWorld->UpdateEntityDef( itemShellHandle, &shell );
		}

	}
}

/*
================
idItem::Spawn
================
*/
void idItem::Spawn( void ) {
	idStr		giveTo;
	idEntity *	ent;
	float		tsize;
	const char	*projectileName;

	state		= NULL;
	idealState	= NULL;

	// HEXEN : Zeroth
	// get the projectile
	projectileDict.Clear();

	projectileName = spawnArgs.GetString( "def_projectile" );
	if ( projectileName[0] != '\0' ) {
		const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( projectileName, false );
		if ( !projectileDef ) {
			gameLocal.Warning( "Unknown projectile '%s' in item '%s'", projectileName, spawnArgs.GetString("inv_name") );
		} else {
			const char *spawnclass = projectileDef->dict.GetString( "spawnclass" );
			idTypeInfo *cls = idClass::GetClass( spawnclass );
			if ( !cls || !cls->IsType( idProjectile::Type ) ) {
				gameLocal.Warning( "Invalid spawnclass '%s' on projectile '%s' (used by item '%s')", spawnclass, projectileName, spawnArgs.GetString("inv_name") );
			} else {
				projectileDict = projectileDef->dict;
			}
		}
	}

	if ( spawnArgs.GetBool( "dropToFloor" ) ) {
		PostEventMS( &EV_DropToFloor, 0 );
	}

	if ( spawnArgs.GetFloat( "triggersize", "0", tsize ) ) {
		GetPhysics()->GetClipModel()->LoadModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ) );
		GetPhysics()->GetClipModel()->Link( gameLocal.clip );
	}

	if ( spawnArgs.GetBool( "start_off" ) ) {
		GetPhysics()->SetContents( 0 );
		Hide();
	} else {
		GetPhysics()->SetContents( CONTENTS_TRIGGER );
	}

	giveTo = spawnArgs.GetString( "owner" );
	if ( giveTo.Length() ) {
		ent = gameLocal.FindEntity( giveTo );
		if ( !ent ) {
			gameLocal.Error( "Item couldn't find owner '%s'", giveTo.c_str() );
		}
		PostEventMS( &EV_Touch, 0, ent, 0 );
	}

	if ( spawnArgs.GetBool( "spin" ) || gameLocal.isMultiplayer ) {
		spin = true;
		BecomeActive( TH_THINK );
	}

	//pulse = !spawnArgs.GetBool( "nopulse" );
	//temp hack for tim
	pulse = false;
	orgOrigin = GetPhysics()->GetOrigin();

	canPickUp = !( spawnArgs.GetBool( "triggerFirst" ) || spawnArgs.GetBool( "no_touch" ) );

	inViewTime = -1000;
	lastCycle = -1;
	itemShellHandle = -1;
	shellMaterial = declManager->FindMaterial( "itemHighlightShell" );

	multimodel=NULL;
	idStr mstr=spawnArgs.GetString("multimodel");

	if ( mstr != "" ) {
		const idDict *multimodeldef = gameLocal.FindEntityDefDict( mstr.c_str() );
		if ( gameLocal.SpawnEntityDef( *multimodeldef, &multimodel ) && multimodel ) {
			multimodel->GetPhysics()->SetOrigin( GetPhysics()->GetOrigin() ); // bob up and down in unison
			multimodel->SetAngles( -GetAngles() ); // make it spin the opposite direction
			spawnArgs.Set( "multimodel_name", multimodel->GetName() );
		}
	}
}

/*
================
idItem::GetAttributes
================
*/
void idItem::GetAttributes( idDict &attributes ) {
	int					i;
	const idKeyValue	*arg;

	for( i = 0; i < spawnArgs.GetNumKeyVals(); i++ ) {
		arg = spawnArgs.GetKeyVal( i );
		if ( arg->GetKey().Left( 4 ) == "inv_" ) {
			attributes.Set( arg->GetKey().Right( arg->GetKey().Length() - 4 ), arg->GetValue() );
		}
	}
}

/*
================
idItem::GiveToPlayer
================
*/
bool idItem::GiveToPlayer( idPlayer *player ) {
	if ( player == NULL ) {
		return false;
	}

	bool val = false;

	if ( spawnArgs.GetBool( "inv_carry" ) ) {
		val = player->GiveInventoryItem( this );
	} else {
		val = player->GiveItem( this );
	}

	if ( val ) {
		if ( spawnArgs.GetString( "scriptobject" ) != "" ) {
			if ( !g_noPickupNotification.GetBool() && !spawnArgs.GetBool( "dontNotifyOnPickup" ) ) {
				// CallFunc( "pickup_message" ); // doesn't work in dhewm3, crashes
				player->ShowHudMessage( va("You got the %s\n", spawnArgs.GetString( "inv_name" )) );
			}
			// CallFunc( "pickup_effect" ); // doesn't work in dhewm3, crashes
		}

		gameLocal.SetPersistentRemove( name.c_str() );
	}

	return val;
}

/*
================
idItem::Pickup
================
*/
bool idItem::Pickup( idPlayer *player ) {

	if ( !GiveToPlayer( player ) ) {
		return false;
	}

	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_PICKUP, NULL, false, -1 );
	}

	// play pickup sound
	if ( !g_noPickupNotification.GetBool() ) {
		StartSound( "snd_acquire", SND_CHANNEL_ITEM, 0, false, NULL );
	}

	// trigger our targets
	ActivateTargets( player );

	// clear our contents so the object isn't picked up twice
	GetPhysics()->SetContents( 0 );

	// hide the model
	Hide();

	// add the highlight shell
	if ( itemShellHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( itemShellHandle );
		itemShellHandle = -1;
	}

	float respawn = spawnArgs.GetFloat( "respawn" );
	bool dropped = spawnArgs.GetBool( "dropped" );
	bool no_respawn = spawnArgs.GetBool( "no_respawn" );

	if ( gameLocal.isMultiplayer && respawn == 0.0f ) {
		respawn = 20.0f;
	}

	if ( respawn && !dropped && !no_respawn ) {
		const char *sfx = spawnArgs.GetString( "fxRespawn" );
		if ( sfx && *sfx ) {
			PostEventSec( &EV_RespawnFx, respawn - 0.5f );
		}
		PostEventSec( &EV_RespawnItem, respawn );
	} else if ( !spawnArgs.GetBool( "inv_objective" ) && !no_respawn ) {
		// give some time for the pickup sound to play
		// FIXME: Play on the owner
		if ( !spawnArgs.GetBool( "inv_carry" ) ) {
			PostEventMS( &EV_Remove, 5000 );
		}
	}

	BecomeInactive( TH_THINK );
	return true;
}

/*
================
idItem::ClientPredictionThink
================
*/
void idItem::ClientPredictionThink( void ) {
	// only think forward because the state is not synced through snapshots
	if ( !gameLocal.isNewFrame ) {
		return;
	}
	Think();
}

/*
================
idItem::WriteFromSnapshot
================
*/
void idItem::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteBits( IsHidden(), 1 );
}

/*
================
idItem::ReadFromSnapshot
================
*/
void idItem::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	if ( msg.ReadBits( 1 ) ) {
		Hide();
	} else {
		Show();
	}
}

/*
================
idItem::ClientReceiveEvent
================
*/
bool idItem::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {

	switch( event ) {
		case EVENT_PICKUP: {

			// play pickup sound
			StartSound( "snd_acquire", SND_CHANNEL_ITEM, 0, false, NULL );

			// hide the model
			Hide();

			// remove the highlight shell
			if ( itemShellHandle != -1 ) {
				gameRenderWorld->FreeEntityDef( itemShellHandle );
				itemShellHandle = -1;
			}
			return true;
		}
		case EVENT_RESPAWN: {
			Event_Respawn();
			return true;
		}
		case EVENT_RESPAWNFX: {
			Event_RespawnFx();
			return true;
		}
		default:
			break;
	}

	return idEntity::ClientReceiveEvent( event, time, msg );
}

/*
================
idItem::Event_DropToFloor
================
*/
void idItem::Event_DropToFloor( void ) {
	trace_t trace;

	// don't drop the floor if bound to another entity
	if ( GetBindMaster() != NULL && GetBindMaster() != this ) {
		return;
	}

	gameLocal.clip.TraceBounds( trace, renderEntity.origin, renderEntity.origin - idVec3( 0, 0, 64 ), renderEntity.bounds, MASK_SOLID | CONTENTS_CORPSE, this );
	SetOrigin( trace.endpos );
}

/*
================
idItem::Event_Touch
================
*/
void idItem::Event_Touch( idEntity *other, trace_t *trace ) {
	idPlayer *player;

	if ( !other->IsType( idPlayer::Type ) ) {
		return;
	}

	player = static_cast<idPlayer *>( other );

	if ( !canPickUp ) {
		return;
	}


	player->CleanupArtifactItems();

	// pickup delay for this player, to prevent instant pickup after drop.

	if ( player == lastOwner && spawnArgs.GetBool( "eoc_dropped" ) ) {
		if ( PickupDelayTime < MS2SEC( gameLocal.realClientTime ) ) {
			return;
		}
	}

	// I don't know why this happens, but it does. seems the player can hit an item multiple times and trigger this pickupevent before he has a chance to disappear after picking it up the first time.
	if ( player == owner ) {
		return;
	}

	if ( spawnArgs.GetBool( "instantEffect" ) && player->ActiveArtifact( spawnArgs.GetString( "inv_name" ) ) ) {
		return;
	}

	// don't pickup if we're full on 'em
	if ( spawnArgs.FindKey("artifact") ) {
		if ( spawnArgs.GetInt( "max_inventory" ) > 0 && player->InventoryItemQty(spawnArgs.GetString( "inv_name" )) >= spawnArgs.GetInt( "max_inventory" ) ) {
			return;
		}
	}

	Pickup( static_cast<idPlayer *>( other ) );
}

/*
================
idItem::Event_Trigger
================
*/
void idItem::Event_Trigger( idEntity *activator ) {

	if ( !canPickUp && spawnArgs.GetBool( "triggerFirst" ) ) {
		canPickUp = true;
		return;
	}

	if ( activator && activator->IsType( idPlayer::Type ) ) {
		Pickup( static_cast<idPlayer *>( activator ) );
	}
}

/*
================
idItem::Event_Respawn
================
*/
void idItem::Event_Respawn( void ) {
	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_RESPAWN, NULL, false, -1 );
	}
	BecomeActive( TH_THINK );
	Show();
	inViewTime = -1000;
	lastCycle = -1;
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	SetOrigin( orgOrigin );
	StartSound( "snd_respawn", SND_CHANNEL_ITEM, 0, false, NULL );
	CancelEvents( &EV_RespawnItem ); // don't double respawn
}

/*
================
idItem::Event_RespawnFx
================
*/
void idItem::Event_RespawnFx( void ) {
	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_RESPAWNFX, NULL, false, -1 );
	}
	const char *sfx = spawnArgs.GetString( "fxRespawn" );
	if ( sfx && *sfx ) {
		idEntityFx::StartFx( sfx, NULL, NULL, this, true );
	}
}

/*
===============================================================================

  idItemPowerup

===============================================================================
*/

/*
===============
idItemPowerup
===============
*/

CLASS_DECLARATION( idItem, idItemPowerup )
END_CLASS

/*
================
idItemPowerup::idItemPowerup
================
*/
idItemPowerup::idItemPowerup() {
	time = 0;
	type = 0;
}

/*
================
idItemPowerup::Save
================
*/
void idItemPowerup::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( time );
	savefile->WriteInt( type );
}

/*
================
idItemPowerup::Restore
================
*/
void idItemPowerup::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( time );
	savefile->ReadInt( type );
}

/*
================
idItemPowerup::Spawn
================
*/
void idItemPowerup::Spawn( void ) {
	time = spawnArgs.GetInt( "time", "30" );
	type = spawnArgs.GetInt( "type", "0" );
}

/*
================
idItemPowerup::GiveToPlayer
================
*/
bool idItemPowerup::GiveToPlayer( idPlayer *player ) {
	if ( player->spectating ) {
		return false;
	}
	player->GivePowerUp( type, time * 1000 );
	gameLocal.SetPersistentRemove( name.c_str() );
	return true;
}

/*
================
HEXEN
idItem::Event_OwnerLaunchProjectiles
================
*/
void idItem::Event_OwnerLaunchProjectiles( int num_projectiles, float spread, float fuseOffset, float launchPower, float dmgPower ) {
	idProjectile	*proj;
	idEntity		*ent;
	int				i;
	idVec3			dir;
	float			ang;
	float			spin;
	float			distance;
	trace_t			tr;
	idVec3			start;
	idVec3			muzzle_pos;
	idBounds		ownerBounds, projBounds;

	idVec3			playerViewOrigin;
	idMat3			playerViewAxis;
	idVec3			zzero;

	zzero.Zero();

	playerViewOrigin.Zero();
	playerViewAxis.Zero();

	playerViewOrigin = owner->firstPersonViewOrigin;
	playerViewAxis = owner->firstPersonViewAxis;

	if ( !projectileDict.GetNumKeyVals() ) {
		const char *classname = this->spawnArgs.GetString("inv_name");
		gameLocal.Warning( "No projectile defined on '%s'", classname );
		return;
	}

	if ( gameLocal.isClient ) {
		// predict instant hit projectiles
		if ( projectileDict.GetBool( "net_instanthit" ) ) {
			float spreadRad = DEG2RAD( spread );
			muzzle_pos = playerViewOrigin + playerViewAxis[ 0 ] * 2.0f;
			for( i = 0; i < num_projectiles; i++ ) {
				ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
				spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
				dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
				dir.Normalize();
				gameLocal.clip.Translation( tr, muzzle_pos, muzzle_pos + dir * 4096.0f, NULL, mat3_identity, MASK_SHOT_RENDERMODEL, owner );
				if ( tr.fraction < 1.0f ) {
					idProjectile::ClientPredictionCollide( this, projectileDict, tr, vec3_origin, true );
				}
			}
		}

	} else {

		ownerBounds = owner->GetPhysics()->GetAbsBounds();

		owner->AddProjectilesFired( num_projectiles );

		float spreadRad = DEG2RAD( spread );
		for( i = 0; i < num_projectiles; i++ ) {
			ang = idMath::Sin( spreadRad * gameLocal.random.RandomFloat() );
			spin = (float)DEG2RAD( 360.0f ) * gameLocal.random.RandomFloat();
			dir = playerViewAxis[ 0 ] + playerViewAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - playerViewAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
			dir.Normalize();

			if ( projectileEnt ) {
				ent = projectileEnt;
				ent->Show();
				ent->Unbind();
				projectileEnt = NULL;
			} else {
				gameLocal.SpawnEntityDef( projectileDict, &ent, false );
			}

			if ( !ent || !ent->IsType( idProjectile::Type ) ) {
				const char *projectileName = this->spawnArgs.GetString( "def_projectile" );
				gameLocal.Error( "'%s' is not an idProjectile", projectileName );
			}

			if ( projectileDict.GetBool( "net_instanthit" ) ) {
				// don't synchronize this on top of the already predicted effect
				ent->fl.networkSync = false;
			}

			proj = static_cast<idProjectile *>(ent);
			proj->Create( owner, playerViewOrigin, dir );

			projBounds = proj->GetPhysics()->GetBounds().Rotate( proj->GetPhysics()->GetAxis() );

			// make sure the projectile starts inside the bounding box of the owner
			if ( i == 0 ) {
				muzzle_pos = playerViewOrigin + playerViewAxis[ 0 ] * 2.0f;
				if ( ( ownerBounds - projBounds).RayIntersection( muzzle_pos, playerViewAxis[0], distance ) ) {
					start = muzzle_pos + distance * playerViewAxis[0];
				} else {
					start = ownerBounds.GetCenter();
				}
				gameLocal.clip.Translation( tr, start, muzzle_pos, proj->GetPhysics()->GetClipModel(), proj->GetPhysics()->GetClipModel()->GetAxis(), MASK_SHOT_RENDERMODEL, owner );
				muzzle_pos = tr.endpos;
			}

			proj->Launch( muzzle_pos, dir, zzero, fuseOffset, launchPower, dmgPower );
		}

	}
}

/*
================
HEXEN
idItem::Event_OwnerCreateProjectile
================
*/
void idItem::Event_OwnerCreateProjectile( void ) {
	if ( !gameLocal.isClient ) {
		projectileEnt = NULL;
		gameLocal.SpawnEntityDef( projectileDict, &projectileEnt, false );
		if ( projectileEnt ) {
			projectileEnt->SetOrigin( GetPhysics()->GetOrigin() );
			projectileEnt->Bind( owner, false );
			projectileEnt->Hide();
		}
		idThread::ReturnEntity( projectileEnt );
	} else {
		idThread::ReturnEntity( NULL );
	}
}


/*
===============================================================================

  idObjective

===============================================================================
*/

CLASS_DECLARATION( idItem, idObjective )
	EVENT( EV_Activate,			idObjective::Event_Trigger )
	EVENT( EV_HideObjective,	idObjective::Event_HideObjective )
	EVENT( EV_GetPlayerPos,		idObjective::Event_GetPlayerPos )
	EVENT( EV_CamShot,			idObjective::Event_CamShot )
END_CLASS

/*
================
idObjective::idObjective
================
*/
idObjective::idObjective() {
	playerPos.Zero();
}

/*
================
idObjective::Save
================
*/
void idObjective::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( playerPos );
}

/*
================
idObjective::Restore
================
*/
void idObjective::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( playerPos );
	PostEventMS( &EV_CamShot, 250 );
}

/*
================
idObjective::Spawn
================
*/
void idObjective::Spawn( void ) {
	Hide();
	PostEventMS( &EV_CamShot, 250 );
}

/*
================
idObjective::Event_Screenshot
================
*/
void idObjective::Event_CamShot( ) {
	const char *camName;
	idStr shotName = gameLocal.GetMapName();
	shotName.StripFileExtension();
	shotName += "/";
	shotName += spawnArgs.GetString( "screenshot" );
	shotName.SetFileExtension( ".tga" );
	if ( spawnArgs.GetString( "camShot", "", &camName ) ) {
		idEntity *ent = gameLocal.FindEntity( camName );
		if ( ent && ent->cameraTarget ) {
			const renderView_t *view = ent->cameraTarget->GetRenderView();
			renderView_t fullView = *view;
			fullView.width = SCREEN_WIDTH;
			fullView.height = SCREEN_HEIGHT;

			// HEXEN : Zeroth
			// HACK : always draw sky-portal view if there is one in the map, this isn't real-time
			if ( gameLocal.portalSkyEnt.GetEntity() && g_enablePortalSky.GetBool() ) {
				renderView_t	portalView = fullView;
				portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();

				// setup global fixup projection vars
				#if 1
					int vidWidth, vidHeight;
					idVec2 shiftScale;

					renderSystem->GetGLSettings( vidWidth, vidHeight );

					float pot;
					int temp;

					int	 w = vidWidth;
					for (temp = 1 ; temp < w ; temp<<=1) {
					}
					pot = (float)temp;
					shiftScale.x = (float)w / pot;

					int	 h = vidHeight;
					for (temp = 1 ; temp < h ; temp<<=1) {
					}
					pot = (float)temp;
					shiftScale.y = (float)h / pot;

					fullView.shaderParms[4] = shiftScale.x;
					fullView.shaderParms[5] = shiftScale.y;
				#endif

				gameRenderWorld->RenderScene( &portalView );
				renderSystem->CaptureRenderToImage( "_currentRender" );
			}

			// draw a view to a texture
			renderSystem->CropRenderSize( 256, 256, true );
			gameRenderWorld->RenderScene( &fullView );
			renderSystem->CaptureRenderToFile( shotName );
			renderSystem->UnCrop();
		}
	}
}

/*
================
idObjective::Event_Trigger
================
*/
void idObjective::Event_Trigger( idEntity *activator ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {

		//Pickup( player );

		if ( spawnArgs.GetString( "inv_objective", NULL ) ) {
			if ( player && player->hud ) {
				idStr shotName = gameLocal.GetMapName();
				shotName.StripFileExtension();
				shotName += "/";
				shotName += spawnArgs.GetString( "screenshot" );
				shotName.SetFileExtension( ".tga" );
				player->hud->SetStateString( "screenshot", shotName );
				player->hud->SetStateString( "objective", "1" );
				player->hud->SetStateString( "objectivetext", spawnArgs.GetString( "objectivetext" ) );
				player->hud->SetStateString( "objectivetitle", spawnArgs.GetString( "objectivetitle" ) );
				player->GiveObjective( spawnArgs.GetString( "objectivetitle" ), spawnArgs.GetString( "objectivetext" ), shotName );

				// a tad slow but keeps from having to update all objectives in all maps with a name ptr
				for( int i = 0; i < gameLocal.num_entities; i++ ) {
					if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idObjectiveComplete::Type ) ) {
						if ( idStr::Icmp( spawnArgs.GetString( "objectivetitle" ), gameLocal.entities[ i ]->spawnArgs.GetString( "objectivetitle" ) ) == 0 ){
							gameLocal.entities[ i ]->spawnArgs.SetBool( "objEnabled", true );
							break;
						}
					}
				}

				PostEventMS( &EV_GetPlayerPos, 2000 );
			}
		}
	}
}

/*
================
idObjective::Event_GetPlayerPos
================
*/
void idObjective::Event_GetPlayerPos() {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		playerPos = player->GetPhysics()->GetOrigin();
		PostEventMS( &EV_HideObjective, 100, player );
	}
}

/*
================
idObjective::Event_HideObjective
================
*/
void idObjective::Event_HideObjective(idEntity *e) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		idVec3 v = player->GetPhysics()->GetOrigin() - playerPos;
		if ( v.Length() > 64.0f ) {
			player->HideObjective();
			PostEventMS( &EV_Remove, 0 );
		} else {
			PostEventMS( &EV_HideObjective, 100, player );
		}
	}
}

/*
===============================================================================

  idVideoCDItem

===============================================================================
*/

CLASS_DECLARATION( idItem, idVideoCDItem )
END_CLASS

/*
================
idVideoCDItem::Spawn
================
*/
void idVideoCDItem::Spawn( void ) {
}

/*
================
idVideoCDItem::GiveToPlayer
================
*/
bool idVideoCDItem::GiveToPlayer( idPlayer *player ) {
	idStr str = spawnArgs.GetString( "video" );
	if ( player && str.Length() ) {
		player->GiveVideo( str, &spawnArgs );
	}
	gameLocal.SetPersistentRemove( name.c_str() );
	return true;
}

/*
===============================================================================

  idPDAItem

===============================================================================
*/

CLASS_DECLARATION( idItem, idPDAItem )
END_CLASS

/*
================
idPDAItem::GiveToPlayer
================
*/
bool idPDAItem::GiveToPlayer(idPlayer *player) {
	const char *str = spawnArgs.GetString( "pda_name" );
	if ( player ) {
		player->GivePDA( str, &spawnArgs );
	}
	gameLocal.SetPersistentRemove( name.c_str() );
	return true;
}

/*
===============================================================================

  idMoveableItem

===============================================================================
*/

CLASS_DECLARATION( idItem, idMoveableItem )
	EVENT( EV_DropToFloor,	idMoveableItem::Event_DropToFloor )
	EVENT( EV_Gib,			idMoveableItem::Event_Gib )
END_CLASS

/*
================
idMoveableItem::idMoveableItem
================
*/
idMoveableItem::idMoveableItem() {
	trigger = NULL;
	smoke = NULL;
	smokeTime = 0;
}

/*
================
idMoveableItem::~idMoveableItem
================
*/
idMoveableItem::~idMoveableItem() {
	if ( trigger ) {
		delete trigger;
	}
}

/*
================
idMoveableItem::Save
================
*/
void idMoveableItem::Save( idSaveGame *savefile ) const {
	savefile->WriteStaticObject( physicsObj );

	savefile->WriteClipModel( trigger );

	savefile->WriteParticle( smoke );
	savefile->WriteInt( smokeTime );
}

/*
================
idMoveableItem::Restore
================
*/
void idMoveableItem::Restore( idRestoreGame *savefile ) {
	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadClipModel( trigger );

	savefile->ReadParticle( smoke );
	savefile->ReadInt( smokeTime );
}

/*
================
idMoveableItem::Spawn
================
*/
void idMoveableItem::Spawn( void ) {
	idTraceModel trm;
	float density, friction, bouncyness, tsize;
	idStr clipModelName;
	idBounds bounds;

	// create a trigger for item pickup
	spawnArgs.GetFloat( "triggersize", "16.0", tsize );
	trigger = new idClipModel( idTraceModel( idBounds( vec3_origin ).Expand( tsize ) ) );
	trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	trigger->SetContents( CONTENTS_TRIGGER );

	// check if a clip model is set
	spawnArgs.GetString( "clipmodel", "", clipModelName );
	if ( !clipModelName[0] ) {
		clipModelName = spawnArgs.GetString( "model" );		// use the visual model
	}

	// load the trace model
	if ( !collisionModelManager->TrmFromModel( clipModelName, trm ) ) {
		gameLocal.Error( "idMoveableItem '%s': cannot load collision model %s", name.c_str(), clipModelName.c_str() );
		return;
	}

	// if the model should be shrinked
	if ( spawnArgs.GetBool( "clipshrink" ) ) {
		trm.Shrink( CM_CLIP_EPSILON );
	}

	// get rigid body properties
	spawnArgs.GetFloat( "density", "0.5", density );
	density = idMath::ClampFloat( 0.001f, 1000.0f, density );
	spawnArgs.GetFloat( "friction", "0.05", friction );
	friction = idMath::ClampFloat( 0.0f, 1.0f, friction );
	spawnArgs.GetFloat( "bouncyness", "0.6", bouncyness );
	bouncyness = idMath::ClampFloat( 0.0f, 1.0f, bouncyness );

	// setup the physics
	physicsObj.SetSelf( this );
	physicsObj.SetClipModel( new idClipModel( trm ), density );
	physicsObj.SetOrigin( GetPhysics()->GetOrigin() );
	physicsObj.SetAxis( GetPhysics()->GetAxis() );
	physicsObj.SetBouncyness( bouncyness );
	physicsObj.SetFriction( 0.6f, 0.6f, friction );
	physicsObj.SetGravity( gameLocal.GetGravity() );
	physicsObj.SetContents( CONTENTS_RENDERMODEL );
	physicsObj.SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );
	SetPhysics( &physicsObj );

	smoke = NULL;
	smokeTime = 0;
	const char *smokeName = spawnArgs.GetString( "smoke_trail" );
	if ( *smokeName != '\0' ) {
		smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		smokeTime = gameLocal.time;
		BecomeActive( TH_UPDATEPARTICLES );
	}
}

/*
================
idMoveableItem::Think
================
*/
void idMoveableItem::Think( void ) {

	RunPhysics();

	if ( thinkFlags & TH_PHYSICS ) {
		// update trigger position
		trigger->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), mat3_identity );
	}

	if ( thinkFlags & TH_UPDATEPARTICLES ) {
		if ( !gameLocal.smokeParticles->EmitSmoke( smoke, smokeTime, gameLocal.random.CRandomFloat(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() ) ) {
			smokeTime = 0;
			BecomeInactive( TH_UPDATEPARTICLES );
		}
	}

	Present();
}

/*
================
idMoveableItem::Pickup
================
*/
bool idMoveableItem::Pickup( idPlayer *player ) {
	bool ret = idItem::Pickup( player );
	if ( ret ) {
		trigger->SetContents( 0 );
	}
	return ret;
}

/*
================
idMoveableItem::DropItem
================
*/
idEntity *idMoveableItem::DropItem( const char *classname, const idVec3 &origin, const idMat3 &axis, const idVec3 &velocity, int activateDelay, int removeDelay ) {
	idDict args;
	idEntity *item;

	args.Set( "classname", classname );
	args.Set( "dropped", "1" );

	// we sometimes drop idMoveables here, so set 'nodrop' to 1 so that it doesn't get put on the floor
	args.Set( "nodrop", "1" );

	if ( activateDelay ) {
		args.SetBool( "triggerFirst", true );
	}

	gameLocal.SpawnEntityDef( args, &item );
	if ( item ) {
		// set item position
		item->GetPhysics()->SetOrigin( origin );
		item->GetPhysics()->SetAxis( axis );
		item->GetPhysics()->SetLinearVelocity( velocity );
		item->UpdateVisuals();
		if ( activateDelay ) {
			item->PostEventMS( &EV_Activate, activateDelay, item );
		}
		if ( !removeDelay ) {
			removeDelay = 5 * 60 * 1000;
		}
		// always remove a dropped item after 5 minutes in case it dropped to an unreachable location
		item->PostEventMS( &EV_Remove, removeDelay );
	}
	return item;
}

/*
================
idMoveableItem::DropItems

  The entity should have the following key/value pairs set:
	"def_drop<type>Item"			"item def"
	"drop<type>ItemJoint"			"joint name"
	"drop<type>ItemRotation"		"pitch yaw roll"
	"drop<type>ItemOffset"			"x y z"
	"skin_drop<type>"				"skin name"
  To drop multiple items the following key/value pairs can be used:
	"def_drop<type>Item<X>"			"item def"
	"drop<type>Item<X>Joint"		"joint name"
	"drop<type>Item<X>Rotation"		"pitch yaw roll"
	"drop<type>Item<X>Offset"		"x y z"
  where <X> is an aribtrary string.
================
*/
void idMoveableItem::DropItems( idAnimatedEntity  *ent, const char *type, idList<idEntity *> *list ) {
	const idKeyValue *kv;
	const char *skinName, *c, *jointName;
	idStr key, key2;
	idVec3 origin;
	idMat3 axis;
	idAngles angles;
	const idDeclSkin *skin;
	jointHandle_t joint;
	idEntity *item;

	// drop all items
	kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sItem", type ), NULL );
	while ( kv ) {

		c = kv->GetKey().c_str() + kv->GetKey().Length();
		if ( idStr::Icmp( c - 5, "Joint" ) != 0 && idStr::Icmp( c - 8, "Rotation" ) != 0 ) {

			key = kv->GetKey().c_str() + 4;
			key2 = key;
			key += "Joint";
			key2 += "Offset";
			jointName = ent->spawnArgs.GetString( key );
			joint = ent->GetAnimator()->GetJointHandle( jointName );
			if ( !ent->GetJointWorldTransform( joint, gameLocal.time, origin, axis ) ) {
				gameLocal.Warning( "%s refers to invalid joint '%s' on entity '%s'\n", key.c_str(), jointName, ent->name.c_str() );
				origin = ent->GetPhysics()->GetOrigin();
				axis = ent->GetPhysics()->GetAxis();
			}
			if ( g_dropItemRotation.GetString()[0] ) {
				angles.Zero();
				sscanf( g_dropItemRotation.GetString(), "%f %f %f", &angles.pitch, &angles.yaw, &angles.roll );
			} else {
				key = kv->GetKey().c_str() + 4;
				key += "Rotation";
				ent->spawnArgs.GetAngles( key, "0 0 0", angles );
			}
			axis = angles.ToMat3() * axis;

			origin += ent->spawnArgs.GetVector( key2, "0 0 0" );

			item = DropItem( kv->GetValue(), origin, axis, vec3_origin, 0, 0 );
			if ( list && item ) {
				list->Append( item );
			}
		}

		kv = ent->spawnArgs.MatchPrefix( va( "def_drop%sItem", type ), kv );
	}

	// change the skin to hide all items
	skinName = ent->spawnArgs.GetString( va( "skin_drop%s", type ) );
	if ( skinName[0] ) {
		skin = declManager->FindSkin( skinName );
		ent->SetSkin( skin );
	}
}

/*
======================
idMoveableItem::WriteToSnapshot
======================
*/
void idMoveableItem::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
}

/*
======================
idMoveableItem::ReadFromSnapshot
======================
*/
void idMoveableItem::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	physicsObj.ReadFromSnapshot( msg );
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

/*
============
idMoveableItem::Gib
============
*/
void idMoveableItem::Gib( const idVec3 &dir, const char *damageDefName ) {
	// spawn smoke puff
	const char *smokeName = spawnArgs.GetString( "smoke_gib" );
	if ( *smokeName != '\0' ) {
		const idDeclParticle *smoke = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, smokeName ) );
		gameLocal.smokeParticles->EmitSmoke( smoke, gameLocal.time, gameLocal.random.CRandomFloat(), renderEntity.origin, renderEntity.axis );
	}
	// remove the entity
	PostEventMS( &EV_Remove, 0 );
}

/*
================
idMoveableItem::Event_DropToFloor
================
*/
void idMoveableItem::Event_DropToFloor( void ) {
	// the physics will drop the moveable to the floor
}

/*
============
idMoveableItem::Event_Gib
============
*/
void idMoveableItem::Event_Gib( const char *damageDefName ) {
	Gib( idVec3( 0, 0, 1 ), damageDefName );
}

/*
===============================================================================

  idMoveablePDAItem

===============================================================================
*/

CLASS_DECLARATION( idMoveableItem, idMoveablePDAItem )
END_CLASS

/*
================
idMoveablePDAItem::GiveToPlayer
================
*/
bool idMoveablePDAItem::GiveToPlayer(idPlayer *player) {
	const char *str = spawnArgs.GetString( "pda_name" );
	if ( player ) {
		player->GivePDA( str, &spawnArgs );
	}
	gameLocal.SetPersistentRemove( name.c_str() );
	return true;
}

/*
===============================================================================

  idItemRemover

===============================================================================
*/

CLASS_DECLARATION( idEntity, idItemRemover )
	EVENT( EV_Activate,		idItemRemover::Event_Trigger )
END_CLASS

/*
================
idItemRemover::Spawn
================
*/
void idItemRemover::Spawn( void ) {
}

/*
================
idItemRemover::RemoveItem
================
*/
void idItemRemover::RemoveItem( idPlayer *player ) {
	const char *remove;

	remove = spawnArgs.GetString( "remove" );
	player->RemoveInventoryItem( remove );
}

/*
================
idItemRemover::Event_Trigger
================
*/
void idItemRemover::Event_Trigger( idEntity *activator ) {
	if ( activator->IsType( idPlayer::Type ) ) {
		RemoveItem( static_cast<idPlayer *>(activator) );
	}
}

/*
===============================================================================

  idObjectiveComplete

===============================================================================
*/

CLASS_DECLARATION( idItemRemover, idObjectiveComplete )
	EVENT( EV_Activate,			idObjectiveComplete::Event_Trigger )
	EVENT( EV_HideObjective,	idObjectiveComplete::Event_HideObjective )
	EVENT( EV_GetPlayerPos,		idObjectiveComplete::Event_GetPlayerPos )
END_CLASS

/*
================
idObjectiveComplete::idObjectiveComplete
================
*/
idObjectiveComplete::idObjectiveComplete() {
	playerPos.Zero();
}

/*
================
idObjectiveComplete::Save
================
*/
void idObjectiveComplete::Save( idSaveGame *savefile ) const {
	savefile->WriteVec3( playerPos );
}

/*
================
idObjectiveComplete::Restore
================
*/
void idObjectiveComplete::Restore( idRestoreGame *savefile ) {
	savefile->ReadVec3( playerPos );
}

/*
================
idObjectiveComplete::Spawn
================
*/
void idObjectiveComplete::Spawn( void ) {
	spawnArgs.SetBool( "objEnabled", false );
	Hide();
}

/*
================
idObjectiveComplete::Event_Trigger
================
*/
void idObjectiveComplete::Event_Trigger( idEntity *activator ) {
	if ( !spawnArgs.GetBool( "objEnabled" ) ) {
		return;
	}
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		RemoveItem( player );

		if ( spawnArgs.GetString( "inv_objective", NULL ) ) {
			if ( player->hud ) {
				player->hud->SetStateString( "objective", "2" );
				player->hud->SetStateString( "objectivetext", spawnArgs.GetString( "objectivetext" ) );
				player->hud->SetStateString( "objectivetitle", spawnArgs.GetString( "objectivetitle" ) );
				player->CompleteObjective( spawnArgs.GetString( "objectivetitle" ) );
				PostEventMS( &EV_GetPlayerPos, 2000 );
			}
		}
	}
}

/*
================
idObjectiveComplete::Event_GetPlayerPos
================
*/
void idObjectiveComplete::Event_GetPlayerPos() {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		playerPos = player->GetPhysics()->GetOrigin();
		PostEventMS( &EV_HideObjective, 100, player );
	}
}

/*
================
idObjectiveComplete::Event_HideObjective
================
*/
void idObjectiveComplete::Event_HideObjective( idEntity *e ) {
	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( player ) {
		idVec3 v = player->GetPhysics()->GetOrigin();
		v -= playerPos;
		if ( v.Length() > 64.0f ) {
			player->hud->HandleNamedEvent( "closeObjective" );
			PostEventMS( &EV_Remove, 0 );
		} else {
			PostEventMS( &EV_HideObjective, 100, player );
		}
	}
}

/*
================
HEXEN
idItem::SetOwner
================
*/
void idItem::SetOwner( idPlayer *_owner ) {
#ifdef _DEBUG && 0
	assert( !owner );
#endif
	owner = _owner;
	lastOwner = _owner;
	// SetName( va( "%s_weapon", owner->name.c_str() ) );

	// if ( worldModel.GetEntity() ) {
	//	worldModel.GetEntity()->SetName( va( "%s_weapon_worldmodel", owner->name.c_str() ) );
	// }
}

/*
================
HEXEN
idItem::CallFunc
================
*/
bool idItem::CallFunc( char *funcName ) {
	const function_t *func = GetScriptFunction( (const char*) funcName );
	if ( !func ) {
#ifdef _DEBUG && 0
		assert( 0 );
#endif
		gameLocal.Error( "Can't find function use' in object '%s'", scriptObject.GetTypeName() );
		return false;
	}

	SetState( func );
	UpdateScript();
	return true;
}

/*
================
HEXEN
idItem::Event_HideMultiModel
================
*/
void idItem::Event_HideMultiModel() {
	if ( multimodel ) {
		multimodel->Hide();
	}
}
