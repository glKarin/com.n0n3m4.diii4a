//**************************************************************************
//**
//** PREY_ITEMS.CPP
//**
//** Game code for Prey-specific items
//**
//**************************************************************************

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

//==========================================================================
// hhItem
//==========================================================================
idEventDef EV_SetPickupState( "<setPickupState>", "dd" );

CLASS_DECLARATION( idItem, hhItem )
	EVENT( EV_SetPickupState,	hhItem::Event_SetPickupState )
	EVENT( EV_RespawnItem,		hhItem::Event_Respawn )
END_CLASS

/*
================
hhItem::Spawn
================
*/
void hhItem::Spawn() {
	// Logic to allow item cabinets to deny pickups until desired
	if( spawnArgs.GetBool("enablePickup", "1") ) {
		EnablePickup();
	} else {
		DisablePickup();
	}

	// Diversity for the blinking highlight shells
	SetShaderParm(SHADERPARM_DIVERSITY, gameLocal.random.RandomFloat());
}

/*
================
hhItem::EnablePickup
================
*/
void hhItem::EnablePickup() {
	GetPhysics()->EnableClip();
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	canPickUp = !spawnArgs.GetBool("triggerFirst");
}

/*
================
hhItem::DisablePickup
================
*/
void hhItem::DisablePickup() {
	GetPhysics()->DisableClip();
	GetPhysics()->SetContents( 0 );
	canPickUp = false;
}

/*
================
hhItem::Pickup
================
*/
bool hhItem::Pickup( idPlayer *player ) {
	if( gameLocal.isMultiplayer ) {
		if( gameLocal.IsCooperative() ) {
			CoopPickup( player );
		} else {
			if (MultiplayerPickup( player )) {
				//HUMANHEAD rww - see if the weapon has a dropped energy type on it
				const char *droppedEnergy = spawnArgs.GetString("def_droppedEnergyType");
				if (droppedEnergy && droppedEnergy[0]) { //if it does, copy it to the player's inventory
					const idDeclEntityDef *energyDecl = gameLocal.FindEntityDef(droppedEnergy, false);
					if (energyDecl) {
						const idDeclEntityDef *fireDecl = gameLocal.FindEntityDef(energyDecl->dict.GetString("def_fireInfo"), false);
						if (fireDecl) {
							hhPlayer *hhPl = static_cast<hhPlayer *>(player);
							int num = hhPl->GetWeaponNum("weaponobj_soulstripper");
							assert(num);
							hhPl->inventory.energyType = droppedEnergy;
							hhPl->weaponInfo[ num ].ammoMax = fireDecl->dict.GetInt("ammoAmount");
							hhPl->spawnArgs.SetInt( "max_ammo_energy", fireDecl->dict.GetInt("ammoAmount") );
							hhPl->inventory.ammo[hhPl->inventory.AmmoIndexForAmmoClass("ammo_energy")]=0;
							hhPl->Give( "ammo_energy", fireDecl->dict.GetString("ammoAmount") );
						}
					}
				}
				//HUMANHEAD END
			}
		}
	} else {
		SinglePlayerPickup( player );
	}
	return true;
}

/*
================
hhItem::ShouldRespawn
================
*/
bool hhItem::ShouldRespawn( float* respawnDelay ) const {
	float respawn = spawnArgs.GetFloat( "respawn", "5.0" );
	if (gameLocal.isMultiplayer) { //rww - override default in mp
		respawn *= 2.0f;
	}

	if( respawnDelay && respawn > 0.0f ) {
		*respawnDelay = respawn;
	}

	return (respawn > 0.0f) && !spawnArgs.GetBool( "dropped" ) && gameLocal.isMultiplayer;
}

/*
================
hhItem::PostRespawn
================
*/
void hhItem::PostRespawn( float delay ) {
	const char *sfx = spawnArgs.GetString( "fxRespawn" );
	if ( sfx && *sfx ) {
		PostEventSec( &EV_RespawnFx, delay - 0.5f );
	} 
	PostEventSec( &EV_RespawnItem, delay );
}

/*
================
hhItem::DetermineRemoveOrRespawn
================
*/
void hhItem::DetermineRemoveOrRespawn( int removeDelay ) {
	bool keepThinking = false;

	// clear our contents so the object isn't picked up twice
	SetPickupState( 0, false );

	if (gameLocal.isMultiplayer && !IsType(idMoveableItem::Type)) { //check for a respawning skin in mp
		idStr respawningSkin;
		if (spawnArgs.GetString("skin_itemRespawning", "", respawningSkin)) {
			SetSkin(declManager->FindSkin(respawningSkin.c_str()));
			keepThinking = true;
		}
	}

	if (!keepThinking) {
		// hide the model
		Hide();
	}

	// add the highlight shell
	if ( itemShellHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( itemShellHandle );
		itemShellHandle = -1;
	}

	float respawnDelay = 0.0f;
	if ( ShouldRespawn(&respawnDelay) ) {
		PostRespawn( respawnDelay );
	} else {
		// give some time for the pickup sound to play
		// FIXME: Play on the owner
		PostEventMS( &EV_Remove, removeDelay );
	}

	if (!keepThinking) {
		BecomeInactive( TH_THINK );
	}
}

/*
================
hhItem::AnnouncePickup
================
*/
int hhItem::AnnouncePickup( idPlayer* player ) {
	ServerSendEvent( EVENT_PICKUP, NULL, false, -1 );

	// play pickup sound
	int soundLength = 0;
	StartSound( "snd_acquire", SND_CHANNEL_ITEM, 0, false, &soundLength );

	// trigger our targets
	ActivateTargets( player );

	return soundLength;
}

/*
================
hhItem::SinglePlayerPickup
================
*/
void hhItem::SinglePlayerPickup( idPlayer *player ) {
	if ( !GiveToPlayer( player ) ) {
		return;
	}

	DetermineRemoveOrRespawn( AnnouncePickup(player) );
}

/*
================
hhItem::CoopPickup
================
*/
void hhItem::CoopPickup( idPlayer* player ) {
	const char* weaponDef = spawnArgs.GetString( "def_weapon" );
	if( weaponDef && weaponDef[0] ) {
		CoopWeaponPickup( player );
	} else {
		CoopItemPickup( player );
	}
}

/*
================
hhItem::CoopWeaponPickup
================
*/
void hhItem::CoopWeaponPickup( idPlayer *player ) {
	float delay = 0.0f;

	if( !GiveToPlayer(player) ) {
		return;
	}

	AnnouncePickup( player );

	ShouldRespawn( &delay );
	SetPickupState( 0, false );
	PostEventSec( &EV_SetPickupState, delay, CONTENTS_TRIGGER, true );
}

/*
================
hhItem::CoopItemPickup
================
*/
void hhItem::CoopItemPickup( idPlayer *player ) {
	idEntity* entity = NULL;
	hhPlayer* castPlayer = NULL;

	for( int ix = 0; ix < gameLocal.numClients; ++ix ) {
		entity = gameLocal.entities[ix];
		if( !entity || !entity->IsType(hhPlayer::Type) ) {
			continue;
		}

		castPlayer = static_cast<hhPlayer*>( entity );
		castPlayer->GiveItem( this );
	}

	DetermineRemoveOrRespawn( AnnouncePickup(player) );
}

/*
================
hhItem::MultiplayerPickup
================
*/
bool hhItem::MultiplayerPickup( idPlayer *player ) {
	if( !GiveToPlayer(player) ) {
		return false;
	}

	DetermineRemoveOrRespawn( AnnouncePickup(player) );
	return true;
}

/*
================
hhItem::SetPickupState
================
*/
void hhItem::SetPickupState( int contents, bool allowPickup ) {
	GetPhysics()->SetContents( contents );
	canPickUp = allowPickup;
}

/*
================
hhItem::Event_SetPickupState
================
*/
void hhItem::Event_SetPickupState( int contents, bool allowPickup ) {
	SetPickupState( contents, allowPickup );
}

/*
================
hhItem::Event_Respawn
================
*/
void hhItem::Event_Respawn( void ) {
	if ( !gameLocal.isClient ) {
		ServerSendEvent( EVENT_RESPAWN, NULL, false, -1 );
	}
	BecomeActive( TH_THINK );
	if (gameLocal.isMultiplayer && renderEntity.customSkin != GetNonRespawnSkin()) {
		SetSkin(GetNonRespawnSkin()); //restore skin
	}
	Show();
	inViewTime = -1000;
	lastCycle = -1;

	//HUMANHEAD: aob
	SetPickupState( CONTENTS_TRIGGER, true );
	//HUMANHEAD END

	SetOrigin( orgOrigin );
	StartSound( "snd_respawn", SND_CHANNEL_ITEM );
}


//=============================================================================
// hhItemSoul
//=============================================================================

idEventDef EV_PlaySpiritSound( "playSpiritSound", NULL );
idEventDef EV_Broadcast_AssignFx_Spirit( "<assignFx_Spirit>", "e" );
idEventDef EV_Broadcast_AssignFx_Physical( "<assignFx_Physical>", "e" );

CLASS_DECLARATION( hhItem, hhItemSoul )
	EVENT( EV_TalonAction,			hhItemSoul::Event_TalonAction )
	EVENT( EV_PostSpawn,			hhItemSoul::Event_PostSpawn )
	EVENT( EV_PlaySpiritSound,		hhItemSoul::Event_PlaySpiritSound )
	EVENT( EV_Broadcast_AssignFx_Spirit,	hhItemSoul::Event_AssignFxSoul_Spirit )
	EVENT( EV_Broadcast_AssignFx_Physical,	hhItemSoul::Event_AssignFxSoul_Physical )
END_CLASS

//--------------------------------------------------------------------------
//
// hhItemSoul::Spawn
//
//--------------------------------------------------------------------------
void hhItemSoul::Spawn() {
	idDict		args;
	idVec3		offset;

	fl.networkSync = true; //rww
	fl.clientEvents = true; //rww

	spawnArgs.SetBool("dropped", true); //rww - don't respawn

	bFollowTriggered = false;

	// Only allow this item to be spawned if we can spirit walk
	if (!gameLocal.isMultiplayer) {
		hhPlayer *player = static_cast<hhPlayer *>( gameLocal.GetLocalPlayer() );
		if ( player ) {
			if ( !player->inventory.requirements.bCanSpiritWalk ) {
				Hide();
				PostEventMS( &EV_Remove, 0 ); // Remove this soul before it can be spawned
				return;
			}
		}
	}

	BecomeActive( TH_THINK | TH_PHYSICS );

	// Remove the lifeforce after 60 seconds if it hasn't been picked up
	if (gameLocal.isMultiplayer) { //rww
		PostEventSec( &EV_Remove, spawnArgs.GetFloat( "lifeTime_mp", "30" ) );
	}
	else {
		PostEventSec( &EV_Remove, spawnArgs.GetFloat( "lifeTime", "60" ) );
	}

	// Play the spirit sound, after a 10 second delay
	PostEventSec( &EV_PlaySpiritSound, spawnArgs.GetFloat( "spiritSoundDelay", "10" ) );

	renderEntity.onlyVisibleInSpirit = true;

	velocity = vec3_origin;
	acceleration = vec3_origin;
	surge = 0.0f;
	parentMonster = NULL;

	PostEventMS( &EV_PostSpawn, 0 );
}

//--------------------------------------------------------------------------
//
// hhItemSoul::Event_PostSpawn
//
//--------------------------------------------------------------------------

void hhItemSoul::Event_PostSpawn() {
	hhFxInfo	fxInfo;

	if (!gameLocal.isMultiplayer) {
		const char *monsterName = spawnArgs.GetString("monsterSpawnedBy", NULL);
		if (monsterName) {
			idEntity *ent = gameLocal.FindEntity(monsterName);
			if (ent && ent->IsType(idAI::Type)) {
				parentMonster = ent;
			}
		}
	}

	// Spawn the spirit-only fx (this fx is only seen when spiritwalking)
	fxInfo.SetNormal( GetAxis()[2] );
	fxInfo.RemoveWhenDone( false );
	fxInfo.OnlyVisibleInSpirit( true );
	BroadcastFxInfo( spawnArgs.GetString("fx_lifeforce"), GetOrigin(), GetAxis(), &fxInfo, &EV_Broadcast_AssignFx_Spirit, false ); //rww - local

	// Spawn the physical realm fx
	fxInfo.SetNormal( GetAxis()[2] );
	fxInfo.RemoveWhenDone( false );
	fxInfo.OnlyVisibleInSpirit( false );
	BroadcastFxInfo( spawnArgs.GetString("fx_lifeforcePhysical"), GetOrigin(), GetAxis(), &fxInfo, &EV_Broadcast_AssignFx_Physical, false ); //rww - local
}

//--------------------------------------------------------------------------
//
// hhItemSoul::Event_AssignFxSoul
//
//--------------------------------------------------------------------------

void hhItemSoul::Event_AssignFxSoul_Spirit( hhEntityFx* fx ) {
	soulFx = fx;
}

void hhItemSoul::Event_AssignFxSoul_Physical( hhEntityFx* fx ) {
	physicalSoulFx = fx;
}

//--------------------------------------------------------------------------
//
// hhItemSoul::Event_PlaySpiritSound
//
//--------------------------------------------------------------------------

void hhItemSoul::Event_PlaySpiritSound() {
	StartSound( "snd_spiritItemNear", SND_CHANNEL_SPIRITWALK, 0, true );

	// Randomly start the sound over again (random delay between 12 and 22 seconds)
	PostEventSec( &EV_PlaySpiritSound, 12.0f + spawnArgs.GetFloat( "spiritSoundDelay", "10.0" ) * gameLocal.random.RandomFloat() );
}

//--------------------------------------------------------------------------
//
// hhItemSoul::~hhItemSoul
//
//--------------------------------------------------------------------------

hhItemSoul::~hhItemSoul() {
	StopSound( SND_CHANNEL_SPIRITWALK, false ); //stop now, do not broadcast on purpose
	SAFE_REMOVE( soulFx );
	SAFE_REMOVE( physicalSoulFx );
}

//--------------------------------------------------------------------------
//
// hhItemSoul::Ticker
//
//--------------------------------------------------------------------------

void hhItemSoul::Think() {
	idVec3	playerOrigin;
	idMat3	playerAxis;
	// Move the soul towards the nearest spirit player
	//COOP FIXME: Get the NEAREST spirit player for this to support multiplayer and coop.

	hhPlayer *player = NULL;
	if (!gameLocal.isMultiplayer) {
		player = static_cast<hhPlayer *>( gameLocal.GetLocalPlayer() );
	}
	else {
		orgOrigin = GetOrigin();
		spin = false;
	}
	float followSpeed = spawnArgs.GetFloat( "followSpeed", "20" );

	if ( !player || player->noclip ) {
		velocity = vec3_origin; // mdl: If player is noclip, don't move an inch
	} else {
		player->GetViewPos( playerOrigin, playerAxis );

		if ( !bFollowTriggered && player && player->IsSpiritWalking() ) {
			acceleration	= ( playerOrigin - playerAxis[2] * 20 ) - GetOrigin();
			float dot		= playerAxis[0] * -acceleration;

			if ( dot > 0.95f ) { // Accelerate only if the player is looking at the soul
				bFollowTriggered = true;
				lastPlayerOrigin = playerOrigin;
			}
		}

		if ( bFollowTriggered ) {
			//HUMANHEAD PCF mdl 04/28/06 - Changed factor from 0.01 to 0.05 for stationary players
			float factor = 0.05f; // Keep acceleration slow enough for the player to see
			if ( ( lastPlayerOrigin - playerOrigin ).LengthSqr() > 75.0f ) {
				velocity = vec3_origin; // Player has moved, so clear the old velocity
				lastPlayerOrigin = playerOrigin;
				//HUMANHEAD PCF mdl 04/28/06 - Changed factor from 5 to 3.5 for moving players
				factor = 3.5f; // Give acceleration a little boost so the soul doesn't seem slow when the player is moving around.
			}
 
			acceleration = ( playerOrigin - playerAxis[2] * 20 ) - GetOrigin();
			velocity += (factor * acceleration * ( spawnArgs.GetFloat( "acceleration", "0.5" ) * spawnArgs.GetFloat( "surge", "0.01" ) ) * (60.0f * USERCMD_ONE_OVER_HZ));

			SetOrigin( GetOrigin() + velocity );
			UpdateVisuals();

			if ( soulFx.IsValid() ) {
				soulFx->SetOrigin( GetOrigin() );
				soulFx->UpdateVisuals();
			}
			if ( physicalSoulFx.IsValid() ) {
				physicalSoulFx->SetOrigin( GetOrigin() );
				physicalSoulFx->UpdateVisuals();
			}
		}
	}

	// Must be done after this movement for proper relinking
	hhItem::Think();
}

//rww - netcode
void hhItemSoul::WriteToSnapshot( idBitMsgDelta &msg ) const {
	GetPhysics()->WriteToSnapshot(msg);
	msg.WriteBits(IsHidden() || !soulFx.IsValid() || !soulFx->IsActive(TH_THINK), 1);
}

void hhItemSoul::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	GetPhysics()->ReadFromSnapshot(msg);
	bool hidden = !!msg.ReadBits(1);
	if (hidden != IsHidden()) {
		if (hidden) {
			if( soulFx.IsValid() ) {
				soulFx->Nozzle( false );
			}
			if( physicalSoulFx.IsValid() ) {
				physicalSoulFx->Nozzle( false );
			}
			Hide();
		}
		else {
			Show();
		}
	}
}

void hhItemSoul::ClientPredictionThink( void ) {
	Think();
}


//--------------------------------------------------------------------------
//
// hhItemSoul::Pickup
//
//--------------------------------------------------------------------------

bool hhItemSoul::Pickup( idPlayer *player ) {

	CancelEvents( &EV_Remove ); // Remove the 60-second cancel event from when this item was spawned
	CancelEvents(&EV_PlaySpiritSound); //rww - also cancel pending spirit sound events

	StopSound( SND_CHANNEL_SPIRITWALK, true );

	bool result = hhItem::Pickup( player );
	if( soulFx.IsValid() ) {
		soulFx->Nozzle( false );
	}
	if( physicalSoulFx.IsValid() ) {
		physicalSoulFx->Nozzle( false );
	}

	return result;
}

//--------------------------------------------------------------------------
//
// hhItemSoul::Event_TalonAction
//
//--------------------------------------------------------------------------

void hhItemSoul::Event_TalonAction( idEntity *talon, bool landed ) {
	idPlayer *player;

	if( !talon ) {
		return;
	}

	player = (idPlayer *)(((hhTalon *)talon)->GetOwner());
	Pickup( player );
}

//--------------------------------------------------------------------------
//
// hhItemSoul::Event_Remove
//
//--------------------------------------------------------------------------
void hhItemSoul::Event_Remove() {
	// Now that spirit is gone, start the disposal countdown
	if (parentMonster.IsValid()) {
		parentMonster->StartDisposeCountdown();
	}

	hhItem::Event_Remove();
}

//================
//hhItemSoul::Save
//================
void hhItemSoul::Save( idSaveGame *savefile ) const {
	soulFx.Save( savefile );
	physicalSoulFx.Save( savefile );
	parentMonster.Save( savefile );
	savefile->WriteVec3( velocity );
	savefile->WriteVec3( acceleration );
	savefile->WriteFloat( surge );
	savefile->WriteBool( bFollowTriggered );
	savefile->WriteVec3( lastPlayerOrigin );
}

//================
//hhItemSoul::Restore
//================
void hhItemSoul::Restore( idRestoreGame *savefile ) {
	soulFx.Restore( savefile );
	physicalSoulFx.Restore( savefile );
	parentMonster.Restore( savefile );
	savefile->ReadVec3( velocity );
	savefile->ReadVec3( acceleration );
	savefile->ReadFloat( surge );
	savefile->ReadBool( bFollowTriggered );
	savefile->ReadVec3( lastPlayerOrigin );
}

