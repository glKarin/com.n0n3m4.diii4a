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
#include "idlib/LangDict.h"
#include "framework/async/NetworkSystem.h"
#include "framework/DeclEntityDef.h"
#include "renderer/RenderSystem.h"

#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"
#include "ai/AI.h"
#include "WorldSpawn.h"
#include "Player.h"
#include "Camera.h"
#include "Fx.h"
#include "Misc.h"

const int ASYNC_PLAYER_INV_AMMO_BITS = idMath::BitsForInteger( 999 );	// 9 bits to cover the range [0, 999]
const int ASYNC_PLAYER_INV_CLIP_BITS = -7;								// -7 bits to cover the range [-1, 60]
/*
===============================================================================

	Player control of the Doom Marine.
	This object handles all player movement and world interaction.

===============================================================================
*/

// distance between ladder rungs (actually is half that distance, but this sounds better)
const int LADDER_RUNG_DISTANCE = 32;

// amount of health per dose from the health station
const int HEALTH_PER_DOSE = 10;

// time before a weapon dropped to the floor disappears
const int WEAPON_DROP_TIME = 20 * 1000;

// time before a next or prev weapon switch happens
const int WEAPON_SWITCH_DELAY = 150;

// how many units to raise spectator above default view height so it's in the head of someone
const int SPECTATE_RAISE = 25;

const int HEALTHPULSE_TIME = 333;

// minimum speed to bob and play run/walk animations at
const float MIN_BOB_SPEED = 5.0f;

//ff start

//cursor states
const int CURSOR_STATE_HIDDEN	= 0;
const int CURSOR_STATE_TALK		= 1;

//stamina given by medals
const int MASSACRE_STAMINA		= 10;
const int MULTIKILL_STAMINA		= 7;
const int DOUBLE_STAMINA		= 5;
const int SKILLSHOT_STAMINA		= 5;
const int CHAINSAW_STAMINA		= 3;

//health
const int HP_EXTRA				= 1;

//helltime
const float HELLTIME_SPEED_MULT = 1.2f;
const float HELLTIME_GRAVITY_MULT = 0.5f;

//vehicle
const int VEHICLE_ENTER_DELAY = 1700; //ms

//skip cin
const int SKIP_CIN_TIP_TIME = 2000; //must match gui time
const int SKIP_CIN_MIN_WAIT_TIME = 700; //don't lower this value. The game may not call HandleEsc for a while.
const int ESC_PRESSED_TIMEOUT = 100;

//ff1.3 - hud events names similar to gui ones, with extra parameter: 0 = cursor, 1 = hud, 2 = info
const idEventDef EV_Player_HudEvent( "hudNamedEvent", "ds" );
const idEventDef EV_Player_SetHudParm( "setHudParm", "dss" );
const idEventDef EV_Player_SetHudFloat( "setHudFloat", "dsf" );
const idEventDef EV_Player_GetHudParm( "getHudParm", "ds", 's');
const idEventDef EV_Player_GetHudFloat( "getHudFloat", "ds", 'f');
const idEventDef EV_Player_ForceUpdateNpcStatus( "forceUpdateNpcStatus" ); //ff1.1
const idEventDef EV_Player_SetStamina( "setStamina", "d" );
const idEventDef EV_Player_GetStamina( "getStamina", NULL, 'd' );
const idEventDef EV_Player_EnableStamina( "enableStamina", "d" );
const idEventDef EV_Player_EnableMedals( "enableMedals", "d" );
const idEventDef EV_Player_IsOnVehicle( "isOnVehicle", NULL, 'd' );
const idEventDef EV_Player_IsOnPossession( "isOnPossession", NULL, 'd' );
const idEventDef EV_Player_GetAlliesDistanceOffset( "getAlliesDistanceOffset", NULL, 'f' );
const idEventDef EV_Player_ShowStats( "showStats", "sd", 'd' );
const idEventDef EV_Player_HideStats( "hideStats" );
const idEventDef EV_Player_GetAmmoForWeapon( "getAmmoForWeapon", "s", 'd' );
//ff end

const idEventDef EV_Player_GetButtons( "getButtons", NULL, 'd' );
const idEventDef EV_Player_GetMove( "getMove", NULL, 'v' );
const idEventDef EV_Player_GetViewAngles( "getViewAngles", NULL, 'v' );
const idEventDef EV_Player_StopFxFov( "stopFxFov" );
const idEventDef EV_Player_EnableWeapon( "enableWeapon" );
const idEventDef EV_Player_DisableWeapon( "disableWeapon" );
const idEventDef EV_Player_GetCurrentWeapon( "getCurrentWeapon", NULL, 's' );
const idEventDef EV_Player_GetPreviousWeapon( "getPreviousWeapon", NULL, 's' );
const idEventDef EV_Player_SelectWeapon( "selectWeapon", "s" );
const idEventDef EV_Player_GetWeaponEntity( "getWeaponEntity", NULL, 'e' );
const idEventDef EV_Player_OpenPDA( "openPDA" );
const idEventDef EV_Player_InPDA( "inPDA", NULL, 'd' );
const idEventDef EV_Player_ExitTeleporter( "exitTeleporter" );
const idEventDef EV_Player_StopAudioLog( "stopAudioLog" );
const idEventDef EV_Player_HideTip( "hideTip" );
const idEventDef EV_Player_LevelTrigger( "levelTrigger" );
const idEventDef EV_SpectatorTouch( "spectatorTouch", "et" );
#ifdef _D3XP
const idEventDef EV_Player_GiveInventoryItem( "giveInventoryItem", "s" );
const idEventDef EV_Player_RemoveInventoryItem( "removeInventoryItem", "s" );
const idEventDef EV_Player_GetIdealWeapon( "getIdealWeapon", NULL, 's' );
const idEventDef EV_Player_SetPowerupTime( "setPowerupTime", "dd" );
const idEventDef EV_Player_IsPowerupActive( "isPowerupActive", "d", 'd' );
const idEventDef EV_Player_WeaponAvailable( "weaponAvailable", "s", 'd');
const idEventDef EV_Player_StartWarp( "startWarp" );
const idEventDef EV_Player_StopHelltime( "stopHelltime", "d" );
const idEventDef EV_Player_ToggleBloom( "toggleBloom", "d" );
const idEventDef EV_Player_SetBloomParms( "setBloomParms", "ff" );
#endif

CLASS_DECLARATION( idActor, idPlayer )
	//ff start
	EVENT( EV_Player_HudEvent,				idPlayer::Event_HudEvent )
	EVENT( EV_Player_SetHudParm,			idPlayer::Event_SetHudParm )
	EVENT( EV_Player_SetHudFloat,			idPlayer::Event_SetHudFloat )
	EVENT( EV_Player_GetHudParm,			idPlayer::Event_GetHudParm )
	EVENT( EV_Player_GetHudFloat,			idPlayer::Event_GetHudFloat )
	EVENT( EV_Player_ForceUpdateNpcStatus,	idPlayer::Event_ForceUpdateNpcStatus ) //ff1.1
	EVENT( EV_Player_SetStamina,			idPlayer::Event_SetStamina )
	EVENT( EV_Player_GetStamina,			idPlayer::Event_GetStamina )
	EVENT( EV_Player_EnableStamina,			idPlayer::Event_EnableStamina )
	EVENT( EV_Player_EnableMedals,			idPlayer::Event_EnableMedals )
	EVENT( EV_Player_IsOnVehicle,			idPlayer::Event_IsOnVehicle )
	EVENT( EV_Player_IsOnPossession,		idPlayer::Event_IsOnPossession )
	EVENT( EV_Player_GetAlliesDistanceOffset,	idPlayer::Event_GetAlliesDistanceOffset )
	EVENT( EV_Player_ShowStats,				idPlayer::Event_ShowStats )
	EVENT( EV_Player_HideStats,				idPlayer::Event_HideStats )
	EVENT( EV_Player_GetAmmoForWeapon,		idPlayer::Event_GetAmmoForWeapon )
	//ff end

	EVENT( EV_Player_GetButtons,			idPlayer::Event_GetButtons )
	EVENT( EV_Player_GetMove,				idPlayer::Event_GetMove )
	EVENT( EV_Player_GetViewAngles,			idPlayer::Event_GetViewAngles )
	EVENT( EV_Player_StopFxFov,				idPlayer::Event_StopFxFov )
	EVENT( EV_Player_EnableWeapon,			idPlayer::Event_EnableWeapon )
	EVENT( EV_Player_DisableWeapon,			idPlayer::Event_DisableWeapon )
	EVENT( EV_Player_GetCurrentWeapon,		idPlayer::Event_GetCurrentWeapon )
	EVENT( EV_Player_GetPreviousWeapon,		idPlayer::Event_GetPreviousWeapon )
	EVENT( EV_Player_SelectWeapon,			idPlayer::Event_SelectWeapon )
	EVENT( EV_Player_GetWeaponEntity,		idPlayer::Event_GetWeaponEntity )
	EVENT( EV_Player_OpenPDA,				idPlayer::Event_OpenPDA )
	EVENT( EV_Player_InPDA,					idPlayer::Event_InPDA )
	EVENT( EV_Player_ExitTeleporter,		idPlayer::Event_ExitTeleporter )
	EVENT( EV_Player_StopAudioLog,			idPlayer::Event_StopAudioLog )
	EVENT( EV_Player_HideTip,				idPlayer::Event_HideTip )
	EVENT( EV_Player_LevelTrigger,			idPlayer::Event_LevelTrigger )
	EVENT( EV_Gibbed,						idPlayer::Event_Gibbed )
#ifdef _D3XP
	EVENT( EV_Player_GiveInventoryItem,		idPlayer::Event_GiveInventoryItem )
	EVENT( EV_Player_RemoveInventoryItem,	idPlayer::Event_RemoveInventoryItem )
	EVENT( EV_Player_GetIdealWeapon,		idPlayer::Event_GetIdealWeapon )
	EVENT( EV_Player_WeaponAvailable,		idPlayer::Event_WeaponAvailable )
	EVENT( EV_Player_SetPowerupTime,		idPlayer::Event_SetPowerupTime )
	EVENT( EV_Player_IsPowerupActive,		idPlayer::Event_IsPowerupActive )
	EVENT( EV_Player_StartWarp,				idPlayer::Event_StartWarp )
	EVENT( EV_Player_StopHelltime,			idPlayer::Event_StopHelltime )
	EVENT( EV_Player_ToggleBloom,			idPlayer::Event_ToggleBloom )
	EVENT( EV_Player_SetBloomParms,			idPlayer::Event_SetBloomParms )
#endif
END_CLASS

const int MAX_RESPAWN_TIME = 10000;
const int RAGDOLL_DEATH_TIME = 3000;
const int MAX_PDAS = 64;
const int MAX_PDA_ITEMS = 128;
const int STEPUP_TIME = 200;
const int MAX_INVENTORY_ITEMS = 20;


#ifdef _D3XP
idVec3 idPlayer::colorBarTable[ 8 ] = {
#else
idVec3 idPlayer::colorBarTable[ 5 ] = {
#endif
	idVec3( 0.25f, 0.25f, 0.25f ),
	idVec3( 1.00f, 0.00f, 0.00f ),
	idVec3( 0.00f, 0.80f, 0.10f ),
	idVec3( 0.20f, 0.50f, 0.80f ),
	idVec3( 1.00f, 0.80f, 0.10f )
#ifdef _D3XP
	,idVec3( 0.425f, 0.484f, 0.445f ),
	idVec3( 0.39f, 0.199f, 0.3f ),
	idVec3( 0.484f, 0.312f, 0.074f)
#endif
};


/*
==============
idInventory::Clear
==============
*/
void idInventory::Clear( void ) {
	maxHealth		= 0;
	weapons			= 0;
	powerups		= 0;
	armor			= 0;
	maxarmor		= 0;
	deplete_armor	= 0;
	deplete_rate	= 0.0f;
	deplete_ammount	= 0;
	nextArmorDepleteTime = 0;

	memset( ammo, 0, sizeof( ammo ) );

	ClearPowerUps();

	// set to -1 so that the gun knows to have a full clip the first time we get it and at the start of the level
	memset( clip, -1, sizeof( clip ) );

	memset( pw_ammo, 0, sizeof( pw_ammo ) ); //ff1.1
	memset( skullSpecialAmmo, 0, sizeof( skullSpecialAmmo ) ); //ff1.3
	skullMode = SKULL_MODE_NORMAL; //ff1.3

	items.DeleteContents( true );
	memset(pdasViewed, 0, 4 * sizeof( pdasViewed[0] ) );
	pdas.Clear();
	videos.Clear();
	emails.Clear();
	selVideo = 0;
	selEMail = 0;
	selPDA = 0;
	selAudio = 0;
	pdaOpened = false;
	turkeyScore = false;

	levelTriggers.Clear();

	nextItemPickup = 0;
	nextItemNum = 1;
	onePickupTime = 0;
	pickupItemNames.Clear();
	objectiveNames.Clear();

	//ff1.3 start
	//lastItemNum				= 0;
	lastShownItem.name		= "";
	lastShownItem.icon		= "";
	lastShownItem.amount	= 0;
	lastShownItemTime		= 0;
	lastFullItemName.Empty();
	gameCovers.Clear();
	//ff1.3 end

	ammoPredictTime = 0;

	lastGiveTime = 0;

	ammoPulse	= false;
	weaponPulse	= false;
	armorPulse	= false;
}

/*
==============
idInventory::GivePowerUp
==============
*/
void idInventory::GivePowerUp( idPlayer *player, int powerup, int msec ) {
	if ( !msec ) {
		// get the duration from the .def files
		const idDeclEntityDef *def = NULL;
		switch ( powerup ) {
			case BERSERK:
				def = gameLocal.FindEntityDef( "powerup_berserk", false );
				break;
			case INVISIBILITY:
				def = gameLocal.FindEntityDef( "powerup_invisibility", false );
				break;
			case MEGAHEALTH:
				def = gameLocal.FindEntityDef( "powerup_megahealth", false );
				break;
			case ADRENALINE:
				def = gameLocal.FindEntityDef( "powerup_adrenaline", false );
				break;
#ifdef _D3XP
			case INVULNERABILITY:
				def = gameLocal.FindEntityDef( "powerup_invulnerability", false );
				break;
			/*case HASTE:
				def = gameLocal.FindEntityDef( "powerup_haste", false );
				break;*/
#endif
		}
		assert( def );
		msec = def->dict.GetInt( "time" ) * 1000;
	}
	powerups |= 1 << powerup;

	//ff1.3 start
	//was: powerupEndTime[ powerup ] = gameLocal.time + msec;
	if ( powerupEndTime[ powerup ] > gameLocal.slow.time ) {
		powerupEndTime[ powerup ] += msec;
	} else {
		powerupEndTime[ powerup ] = gameLocal.slow.time + msec;
	}
	//ff1.3 end
}

/*
==============
idInventory::ClearPowerUps
==============
*/
void idInventory::ClearPowerUps( void ) {
	int i;
	for ( i = 0; i < MAX_POWERUPS; i++ ) {
		powerupEndTime[ i ] = 0;
	}
	powerups = 0;
}

/*
==============
idInventory::GetPersistantData
==============
*/
void idInventory::GetPersistantData( idDict &dict ) {
	int		i;
	int		num;
	idDict	*item;
	idStr	key;
	const idKeyValue *kv;
	const char *name;

	// armor
	dict.SetInt( "armor", armor );

	// don't bother with powerups, maxhealth, maxarmor, or the clip

	//ff1.3 start - we do bother with maxhealth :)
	dict.SetInt( "maxhealth", maxHealth );
	//ff1.3 end

	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if ( name ) {
			dict.SetInt( name, ammo[ i ] );
		}
	}

#ifdef _D3XP
	//Save the clip data
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		dict.SetInt( va("clip%i", i), clip[ i ] );
		dict.SetInt( va("pw_ammo%i", i), pw_ammo[ i ] ); //ff1.1
	}
#endif

	//Save the skull ammo data //ff1.3
	dict.SetInt( "skullMode", skullMode );
	for( i = 0; i < MAX_SKULL_MODES; i++ ) {
		dict.SetInt( va("skullSpecialAmmo%i", i), skullSpecialAmmo[ i ] );
	}

	// items
	num = 0;
	for( i = 0; i < items.Num(); i++ ) {
		item = items[ i ];

		// copy all keys with "inv_"
		kv = item->MatchPrefix( "inv_" );
		if ( kv ) {
			while( kv ) {
				sprintf( key, "item_%i %s", num, kv->GetKey().c_str() );
				dict.Set( key, kv->GetValue() );
				kv = item->MatchPrefix( "inv_", kv );
			}
			num++;
		}
	}
	dict.SetInt( "items", num );

	// pdas viewed
	for ( i = 0; i < 4; i++ ) {
		dict.SetInt( va("pdasViewed_%i", i), pdasViewed[i] );
	}

	dict.SetInt( "selPDA", selPDA );
	dict.SetInt( "selVideo", selVideo );
	dict.SetInt( "selEmail", selEMail );
	dict.SetInt( "selAudio", selAudio );
	dict.SetInt( "pdaOpened", pdaOpened );
	dict.SetInt( "turkeyScore", turkeyScore );

	// pdas
	for ( i = 0; i < pdas.Num(); i++ ) {
		sprintf( key, "pda_%i", i );
		dict.Set( key, pdas[ i ] );
	}
	dict.SetInt( "pdas", pdas.Num() );

	// video cds
	for ( i = 0; i < videos.Num(); i++ ) {
		sprintf( key, "video_%i", i );
		dict.Set( key, videos[ i ].c_str() );
	}
	dict.SetInt( "videos", videos.Num() );

	// emails
	for ( i = 0; i < emails.Num(); i++ ) {
		sprintf( key, "email_%i", i );
		dict.Set( key, emails[ i ].c_str() );
	}
	dict.SetInt( "emails", emails.Num() );

	// weapons
	dict.SetInt( "weapon_bits", weapons );

	// levelTriggers
	dict.SetInt( "levelTriggers", levelTriggers.Num() );
	for ( i = 0; i < levelTriggers.Num(); i++ ) {
		sprintf( key, "levelTrigger_Level_%i", i );
		dict.Set( key, levelTriggers[i].levelName );
		sprintf( key, "levelTrigger_Trigger_%i", i );
		dict.Set( key, levelTriggers[i].triggerName );
	}

	//gameCovers
	dict.SetInt( "gameCovers", gameCovers.Num() );
	for( i = 0; i < gameCovers.Num(); i++ ) {
		sprintf( key, "gameCovers_DefName_%i", i );
		dict.Set( key, gameCovers[i].defName );
		//sprintf( key, "gameCovers_Amount_%i", i );
		//dict.SetInt( key, gameCovers[i].amount );
	}
}

/*
==============
idInventory::RestoreInventory
==============
*/
void idInventory::RestoreInventory( idPlayer *owner, const idDict &dict ) {
	int			i;
	int			num;
	idDict		*item;
	idStr		key;
	idStr		itemname;
	const idKeyValue *kv;
	const char	*name;

	Clear();

	// health/armor
	maxHealth		= dict.GetInt( "maxhealth", "100" );
	armor			= dict.GetInt( "armor", "50" );
	maxarmor		= dict.GetInt( "maxarmor", "100" );
	deplete_armor	= dict.GetInt( "deplete_armor", "0" );
	deplete_rate	= dict.GetFloat( "deplete_rate", "2.0" );
	deplete_ammount	= dict.GetInt( "deplete_ammount", "1" );

	// the clip and powerups aren't restored

	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if ( name ) {
			ammo[ i ] = dict.GetInt( name );
		}
	}

#ifdef _D3XP
	//Restore the clip data
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		clip[i] = dict.GetInt(va("clip%i", i), "-1");
		pw_ammo[i] = dict.GetInt(va("pw_ammo%i", i), "0"); //ff1.1
	}
#endif

	//Restore the skull ammo data //ff1.3
	skullMode = dict.GetInt( "skullMode", "-1" );
	for( i = 0; i < MAX_SKULL_MODES; i++ ) {
		skullSpecialAmmo[i] = dict.GetInt(va("skullSpecialAmmo%i", i), "0");
	}

	// items
	num = dict.GetInt( "items" );
	items.SetNum( num );
	for( i = 0; i < num; i++ ) {
		item = new idDict();
		items[ i ] = item;
		sprintf( itemname, "item_%i ", i );
		kv = dict.MatchPrefix( itemname );
		while( kv ) {
			key = kv->GetKey();
			key.Strip( itemname );
			item->Set( key, kv->GetValue() );
			kv = dict.MatchPrefix( itemname, kv );
		}
	}

	// pdas viewed
	for ( i = 0; i < 4; i++ ) {
		pdasViewed[i] = dict.GetInt(va("pdasViewed_%i", i));
	}

	selPDA = dict.GetInt( "selPDA" );
	selEMail = dict.GetInt( "selEmail" );
	selVideo = dict.GetInt( "selVideo" );
	selAudio = dict.GetInt( "selAudio" );
	pdaOpened = dict.GetBool( "pdaOpened" );
	turkeyScore = dict.GetBool( "turkeyScore" );

	// pdas
	num = dict.GetInt( "pdas" );
	pdas.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "pda_%i", i );
		pdas[i] = dict.GetString( itemname, "default" );
	}

	// videos
	num = dict.GetInt( "videos" );
	videos.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "video_%i", i );
		videos[i] = dict.GetString( itemname, "default" );
	}

	// emails
	num = dict.GetInt( "emails" );
	emails.SetNum( num );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "email_%i", i );
		emails[i] = dict.GetString( itemname, "default" );
	}

	// weapons are stored as a number for persistant data, but as strings in the entityDef
	weapons	= dict.GetInt( "weapon_bits", "0" );

	if ( g_skill.GetInteger() >= 3 ) {
		Give( owner, dict, "weapon", dict.GetString( "weapon_nightmare" ), NULL, false );
	} else {
		Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false );
	}

	// levelTriggers
	num = dict.GetInt( "levelTriggers" );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "levelTrigger_Level_%i", i );
		idLevelTriggerInfo lti;
		lti.levelName = dict.GetString( itemname );
		sprintf( itemname, "levelTrigger_Trigger_%i", i );
		lti.triggerName = dict.GetString( itemname );
		levelTriggers.Append( lti );
	}

	// gameCovers
	num = dict.GetInt( "gameCovers" );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "gameCovers_DefName_%i", i );
		idGameCoverInfo gameCover;
		gameCover.defName = dict.GetString( itemname );
		//sprintf( itemname, "gameCovers_Amount_%i", i );
		//gameCover.amount = dict.GetInt( itemname );
		gameCovers.Append( gameCover );
	}
}

/*
==============
idInventory::Save
==============
*/
void idInventory::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteInt( maxHealth );
	savefile->WriteInt( weapons );
	savefile->WriteInt( powerups );
	savefile->WriteInt( armor );
	savefile->WriteInt( maxarmor );
	savefile->WriteInt( ammoPredictTime );
	savefile->WriteInt( deplete_armor );
	savefile->WriteFloat( deplete_rate );
	savefile->WriteInt( deplete_ammount );
	savefile->WriteInt( nextArmorDepleteTime );

	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		savefile->WriteInt( ammo[ i ] );
	}
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		savefile->WriteInt( clip[ i ] );
		savefile->WriteInt( pw_ammo[ i ] ); //ff1.1
	}
	//ff1.3
	savefile->WriteInt( skullMode );
	for( i = 0; i < MAX_SKULL_MODES; i++ ) {
		savefile->WriteInt( skullSpecialAmmo[ i ] );
	}
	for( i = 0; i < MAX_POWERUPS; i++ ) {
		savefile->WriteInt( powerupEndTime[ i ] );
	}

	savefile->WriteInt( items.Num() );
	for( i = 0; i < items.Num(); i++ ) {
		savefile->WriteDict( items[ i ] );
	}

	savefile->WriteInt( pdasViewed[0] );
	savefile->WriteInt( pdasViewed[1] );
	savefile->WriteInt( pdasViewed[2] );
	savefile->WriteInt( pdasViewed[3] );

	savefile->WriteInt( selPDA );
	savefile->WriteInt( selVideo );
	savefile->WriteInt( selEMail );
	savefile->WriteInt( selAudio );
	savefile->WriteBool( pdaOpened );
	savefile->WriteBool( turkeyScore );

	savefile->WriteInt( pdas.Num() );
	for( i = 0; i < pdas.Num(); i++ ) {
		savefile->WriteString( pdas[ i ] );
	}

	savefile->WriteInt( pdaSecurity.Num() );
	for( i=0; i < pdaSecurity.Num(); i++ ) {
		savefile->WriteString( pdaSecurity[ i ] );
	}

	savefile->WriteInt( videos.Num() );
	for( i = 0; i < videos.Num(); i++ ) {
		savefile->WriteString( videos[ i ] );
	}

	savefile->WriteInt( emails.Num() );
	for ( i = 0; i < emails.Num(); i++ ) {
		savefile->WriteString( emails[ i ] );
	}

	savefile->WriteInt( nextItemPickup );
	savefile->WriteInt( nextItemNum );
	savefile->WriteInt( onePickupTime );

	savefile->WriteInt( pickupItemNames.Num() );
	for( i = 0; i < pickupItemNames.Num(); i++ ) {
		savefile->WriteString( pickupItemNames[i].icon );
		savefile->WriteString( pickupItemNames[i].name );
		savefile->WriteInt( pickupItemNames[i].amount ); //ff1.3
	}

	//ff1.3 start
	savefile->WriteString( lastShownItem.icon );
	savefile->WriteString( lastShownItem.name );
	savefile->WriteInt( lastShownItem.amount );
	savefile->WriteInt( lastShownItemTime );
	//lastFullItemName not saved

	savefile->WriteInt( gameCovers.Num() );
	for ( i = 0; i < gameCovers.Num(); i++ ) {
		savefile->WriteString( gameCovers[i].defName );
		//savefile->WriteInt( gameCovers[i].amount );
	}
	//ff1.3 end

	savefile->WriteInt( objectiveNames.Num() );
	for( i = 0; i < objectiveNames.Num(); i++ ) {
		savefile->WriteString( objectiveNames[i].screenshot );
		savefile->WriteString( objectiveNames[i].text );
		savefile->WriteString( objectiveNames[i].title );
	}

	savefile->WriteInt( levelTriggers.Num() );
	for ( i = 0; i < levelTriggers.Num(); i++ ) {
		savefile->WriteString( levelTriggers[i].levelName );
		savefile->WriteString( levelTriggers[i].triggerName );
	}

	savefile->WriteBool( ammoPulse );
	savefile->WriteBool( weaponPulse );
	savefile->WriteBool( armorPulse );

	savefile->WriteInt( lastGiveTime );

	//ff1.3 start
	/* was:
#ifdef _D3XP
	for(i = 0; i < AMMO_NUMTYPES; i++) {
		savefile->WriteInt(rechargeAmmo[i].ammo);
		savefile->WriteInt(rechargeAmmo[i].rechargeTime);
		savefile->WriteString(rechargeAmmo[i].ammoName);
	}
#endif
	*/

	savefile->WriteInt( skullAmmoType );
	//ff1.3 end
}

/*
==============
idInventory::Restore
==============
*/
void idInventory::Restore( idRestoreGame *savefile ) {
	int i, num;

	savefile->ReadInt( maxHealth );
	savefile->ReadInt( weapons );
	savefile->ReadInt( powerups );
	savefile->ReadInt( armor );
	savefile->ReadInt( maxarmor );
	savefile->ReadInt( ammoPredictTime );
	savefile->ReadInt( deplete_armor );
	savefile->ReadFloat( deplete_rate );
	savefile->ReadInt( deplete_ammount );
	savefile->ReadInt( nextArmorDepleteTime );

	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		savefile->ReadInt( ammo[ i ] );
	}
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		savefile->ReadInt( clip[ i ] );
		savefile->ReadInt( pw_ammo[ i ] ); //ff1.1
	}
	//ff1.3
	savefile->ReadInt( skullMode );
	for( i = 0; i < MAX_SKULL_MODES; i++ ) {
		savefile->ReadInt( skullSpecialAmmo[ i ] );
	}
	for( i = 0; i < MAX_POWERUPS; i++ ) {
		savefile->ReadInt( powerupEndTime[ i ] );
	}

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idDict *itemdict = new idDict;

		savefile->ReadDict( itemdict );
		items.Append( itemdict );
	}

	// pdas
	savefile->ReadInt( pdasViewed[0] );
	savefile->ReadInt( pdasViewed[1] );
	savefile->ReadInt( pdasViewed[2] );
	savefile->ReadInt( pdasViewed[3] );

	savefile->ReadInt( selPDA );
	savefile->ReadInt( selVideo );
	savefile->ReadInt( selEMail );
	savefile->ReadInt( selAudio );
	savefile->ReadBool( pdaOpened );
	savefile->ReadBool( turkeyScore );

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idStr strPda;
		savefile->ReadString( strPda );
		pdas.Append( strPda );
	}

	// pda security clearances
	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idStr invName;
		savefile->ReadString( invName );
		pdaSecurity.Append( invName );
	}

	// videos
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idStr strVideo;
		savefile->ReadString( strVideo );
		videos.Append( strVideo );
	}

	// email
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idStr strEmail;
		savefile->ReadString( strEmail );
		emails.Append( strEmail );
	}

	savefile->ReadInt( nextItemPickup );
	savefile->ReadInt( nextItemNum );
	savefile->ReadInt( onePickupTime );
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idItemInfo info;

		savefile->ReadString( info.icon );
		savefile->ReadString( info.name );
		savefile->ReadInt( info.amount ); //ff1.3

		pickupItemNames.Append( info );
	}

	//ff1.3 start
	savefile->ReadString( lastShownItem.icon );
	savefile->ReadString( lastShownItem.name );
	savefile->ReadInt( lastShownItem.amount );
	savefile->ReadInt( lastShownItemTime );
	lastFullItemName.Empty(); //not saved

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idGameCoverInfo gameCover;
		savefile->ReadString( gameCover.defName );
		//savefile->ReadInt( gameCover.amount );
		gameCovers.Append( gameCover );
	}
	//ff1.3 end

	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idObjectiveInfo obj;

		savefile->ReadString( obj.screenshot );
		savefile->ReadString( obj.text );
		savefile->ReadString( obj.title );

		objectiveNames.Append( obj );
	}

	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idLevelTriggerInfo lti;
		savefile->ReadString( lti.levelName );
		savefile->ReadString( lti.triggerName );
		levelTriggers.Append( lti );
	}

	savefile->ReadBool( ammoPulse );
	savefile->ReadBool( weaponPulse );
	savefile->ReadBool( armorPulse );

	savefile->ReadInt( lastGiveTime );

	//ff1.3 start
	/* was:
#ifdef _D3XP
	for(i = 0; i < AMMO_NUMTYPES; i++) {
		savefile->ReadInt(rechargeAmmo[i].ammo);
		savefile->ReadInt(rechargeAmmo[i].rechargeTime);

		idStr name;
		savefile->ReadString(name);
		strcpy(rechargeAmmo[i].ammoName, name);
	}
#endif
	*/

	savefile->ReadInt( (int &) skullAmmoType );
	//ff1.3 end
}

/*
==============
idInventory::AmmoIndexForAmmoClass
==============
*/
ammo_t idInventory::AmmoIndexForAmmoClass( const char *ammo_classname ) const {
	return idWeapon::GetAmmoNumForName( ammo_classname );
}

/*
==============
idInventory::MaxAmmoForAmmoClass
==============
*/
int idInventory::MaxAmmoForAmmoClass( idPlayer *owner, const char *ammo_classname ) const {
	return owner->spawnArgs.GetInt( va( "max_%s", ammo_classname ), "0" );
}

//ff1.3 start
/*
==============
idInventory::MaxPwAmmoForWeaponName
==============
*/
int idInventory::MaxPwAmmoForWeaponName( idPlayer *owner, const char *weapon_name ) const {
	return owner->spawnArgs.GetInt( va( "max_pw_%s", weapon_name ), "0" );
}

/*
==============
idInventory::PwPickupNameForWeaponName
==============
*/
const char *idInventory::PwPickupNameForWeaponName( const char *weapon_name ) const {
	const idDict *weaponDict = gameLocal.FindEntityDefDict( weapon_name, false );
	if ( weaponDict ) {
		return va( "%s Powerup", common->GetLanguageDict()->GetString( weaponDict->GetString( "inv_name" ) ) );
	} else {
		gameLocal.Warning("Invalid weapon name '%s'", weapon_name );
		return "";
	}
}
//ff1.3 end

/*
==============
idInventory::AmmoPickupNameForIndex
==============
*/
const char *idInventory::AmmoPickupNameForIndex( ammo_t ammonum ) const {
	return idWeapon::GetAmmoPickupNameForNum( ammonum );
}

/*
==============
idInventory::WeaponIndexForAmmoClass
mapping could be prepared in the constructor
==============
*/
int idInventory::WeaponIndexForAmmoClass( const idDict & spawnArgs, const char *ammo_classname ) const {
	int i;
	const char *weapon_classname;
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		weapon_classname = spawnArgs.GetString( va( "def_weapon%d", i ) );
		if ( !weapon_classname ) {
			continue;
		}
		const idDeclEntityDef *decl = gameLocal.FindEntityDef( weapon_classname, false );
		if ( !decl ) {
			continue;
		}
		if ( !idStr::Icmp( ammo_classname, decl->dict.GetString( "ammoType" ) ) ) {
			return i;
		}
	}
	return -1;
}

/*
==============
idInventory::AmmoIndexForWeaponClass
==============
*/
ammo_t idInventory::AmmoIndexForWeaponClass( const char *weapon_classname, int *ammoRequired ) {
	const idDeclEntityDef *decl = gameLocal.FindEntityDef( weapon_classname, false );
	if ( !decl ) {
		gameLocal.Error( "Unknown weapon in decl '%s'", weapon_classname );
	}
	if ( ammoRequired ) {
		*ammoRequired = decl->dict.GetInt( "ammoRequired" );
	}
	ammo_t ammo_i = AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
	return ammo_i;
}

/*
==============
idInventory::AddPickupName
==============
*/
void idInventory::AddPickupName( const char *name, const char *icon, int amount, idPlayer* owner ) { //_D3XP
	int num;

	num = pickupItemNames.Num();

	//ff1.3 start
	if ( ( num > 0 ) && ( pickupItemNames[ num - 1 ].name.Icmp( name ) == 0 ) ) {
		pickupItemNames[ num - 1 ].amount += amount;
	} else {
	//if ( ( num == 0 ) || ( pickupItemNames[ num - 1 ].name.Icmp( name ) != 0 ) ) {
	//ff1.3 end
		idItemInfo &info = pickupItemNames.Alloc();

		if ( idStr::Cmpn( name, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
			info.name = common->GetLanguageDict()->GetString( name );
		} else {
			info.name = name;
		}
		info.icon = icon;
		info.amount = amount; //ff1.3

#ifdef _D3XP
		if ( gameLocal.isServer ) {
			idBitMsg	msg;
			byte		msgBuf[MAX_EVENT_PARAM_SIZE];

			msg.Init( msgBuf, sizeof( msgBuf ) );
			msg.WriteString( name, MAX_EVENT_PARAM_SIZE );
			owner->ServerSendEvent( idPlayer::EVENT_PICKUPNAME, &msg, false, -1 );
		}
	}
#endif
}

/*
==============
idInventory::Give
==============
*/
bool idInventory::Give( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon, bool updateHud ) {
	int						i;
	const char				*pos;
	const char				*end;
	int						len;
	int						max;
	const idDeclEntityDef	*weaponDecl;
	bool					tookWeapon;
	int						amount;
	idItemInfo				info;
	const char				*name;


	//ff start
	if ( !idStr::Icmpn( statname, "pw_", 3 ) ) {
		amount = atoi( value );
		if ( !amount ) {
			return false;
		}

		if ( !idStr::Icmp( statname, "pw_remote_grenade" ) ) {
			end = "weapon_handgrenade";
			i = owner->SlotForWeapon( end );
			if ( i < 0 ) {
				gameLocal.Warning( "Invalid weapon name 'weapon_handgrenade'" );
				return false;
			}
		} else {

			end = idStr( statname ).Right( idStr::Length(statname) - 3 ).c_str(); //weapon name
			i = owner->SlotForWeapon( end );
			if ( i < 0 ) {
				gameLocal.Warning( "Invalid weapon name for '%s'", statname );
				return false;
			}

			max = MaxPwAmmoForWeaponName( owner, end );
			if ( pw_ammo[ i ] >= max ) {
				if ( lastFullItemName.IsEmpty() ){
					lastFullItemName = PwPickupNameForWeaponName( end );
				}
				return false;
			}

			pw_ammo[ i ] += amount;
			if ( pw_ammo[ i ] > max ) {
				pw_ammo[ i ] = max;
			}
			ammoPulse = true; //upd ammo count
		}

		//select weapon if we already have it
		if ( (weapons & ( 1 << i )) && idealWeapon ) {
			*idealWeapon = i;
			weaponPulse = true; //upd weapon selection
		}

		name = PwPickupNameForWeaponName( end );
		if ( idStr::Length( name ) ) {
			AddPickupName( name, "", amount, owner ); //_D3XP
		}
	} else if ( !idStr::Icmpn( statname, "ammo_skulls", 11 ) ) { //fix for skulls
		if ( !idStr::Icmp( statname, "ammo_skulls" ) ) {
			len = SKULL_MODE_NORMAL;
		} else if ( !idStr::Icmp( statname, "ammo_skulls_berserk" ) ) {
			len = SKULL_MODE_BERSERK;
		} else if ( !idStr::Icmp( statname, "ammo_skulls_invuln" ) ) {
			len = SKULL_MODE_INVULN;
		} else {
			gameLocal.Warning( "Unknown special ammo skulls type '%s'", statname );
			return false;
		}

		amount = atoi( value );
		if ( !amount ) {
			return false;
		}
		i = skullAmmoType;
		max = MaxAmmoForAmmoClass( owner, "ammo_skulls" );

		if ( ammo[ i ] >= max ) {
			if ( lastFullItemName.IsEmpty() && amount > 0 ){
				lastFullItemName = AmmoPickupNameForIndex( i );
			}
			return false;
		}

		//hud feedback
		name = AmmoPickupNameForIndex( i );
		if ( idStr::Length( name ) ) {
			AddPickupName( name, "", amount, owner ); //_D3XP
		}

		if ( ammo[ i ] + amount > max ) {
			amount = max - ammo[ i ];
		}
		ammo[ i ] += amount; //upd standard ammo count
		if ( len != SKULL_MODE_NORMAL ) {
			skullSpecialAmmo[len] += amount; //upd special ammo count
			//gameLocal.Printf( "adding %d skull ammo -> ammo[ %d ] = %d, skullSpecialAmmo[ %d ] = %d\n", amount, i, ammo[ i ], len, skullSpecialAmmo[len] );
		}
		ammoPulse = true;

		//change mode
		skullMode = len;

		//select skull weapon
		if ( (weapons & ( 1 << owner->weapon_skull )) && idealWeapon ) {
			*idealWeapon = owner->weapon_skull;
			weaponPulse = true; //upd weapon selection
		}
	} else
	//ff end
/*
#ifdef _D3XP
	if ( !idStr::Icmp( statname, "ammo_bloodstone" ) ) {
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );

		if(max <= 0) {
			//No Max
			ammo[ i ] += atoi( value );
		} else {
			//Already at or above the max so don't allow the give
			if(ammo[ i ] >= max) {
				ammo[ i ] = max;
				return false;
			}
			//We were below the max so accept the give but cap it at the max
			ammo[ i ] += atoi( value );
			if(ammo[ i ] > max) {
				ammo[ i ] = max;
			}
		}
	} else
#endif
*/
	if ( !idStr::Icmpn( statname, "ammo_", 5 ) ) {
		amount = atoi( value );
		if( !amount ) {
			return false; //ff1.3 - powerups have 0 ammo
		}
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );

		if ( ammo[ i ] >= max ) {
			//ff1.3 start
			if ( lastFullItemName.IsEmpty() ){
				lastFullItemName = AmmoPickupNameForIndex( i );
			}
			//ff1.3 end
			return false;
		}

		//ff1.3 start - same logic but calc
		/*
		if ( ( max > 0 ) && ( ammo[ i ] + amount > max ) ) {
			amount = max - ammo[ i ];
			ammo[ i ] = max;
		} else {
			ammo[ i ] += amount;
		}
		*/
		ammo[ i ] += amount;
		if ( ( max > 0 ) && ( ammo[ i ] > max ) ) {
			ammo[ i ] = max;
		}
		//ff1.3 end
		ammoPulse = true;

		name = AmmoPickupNameForIndex( i );
		if ( idStr::Length( name ) ) {
			AddPickupName( name, "", amount, owner ); //_D3XP
		}

		//ff1.3 start
		if ( !idStr::Icmp( statname, "ammo_souls" ) ) {
			//select soul2cube weapon
			if ( (weapons & ( 1 << owner->weapon_soul2cube )) && idealWeapon ) {
				*idealWeapon = owner->weapon_soul2cube;
				weaponPulse = true; //upd weapon selection
			}
		} else if ( !idStr::Icmp( statname, "ammo_possession" ) ) {
			//select soul2cube weapon
			if ( (weapons & ( 1 << owner->weapon_possession )) && idealWeapon ) {
				*idealWeapon = owner->weapon_possession;
				weaponPulse = true; //upd weapon selection
			}
		}
		//ff1.3 end
	} else if ( !idStr::Icmpn( statname, "armor", 5 ) ) { //was: !idStr::Icmp( statname, "armor" )
		if ( idStr::Icmp( statname, "armor" ) ) {
			switch( g_skill.GetInteger() ) {
				case 0: {
					if( idStr::Icmp( statname, "armor_easy" ) ){
						return false;
					}
					break;
				}
				case 1: {
					if( idStr::Icmp( statname, "armor_medium" ) ){
						return false;
					}
					break;
				}
				case 2: {
					if( idStr::Icmp( statname, "armor_hard" ) ){
						return false;
					}
					break;
				}
				case 3: {
					if( idStr::Icmp( statname, "armor_nightmare" ) ){
						return false;
					}
					break;
				}
				default: {
					return false;
				}
			}
		}

		if ( armor >= maxarmor ) {
			//ff1.3 start
			if ( lastFullItemName.IsEmpty() && atoi( value ) > 0 ){
				lastFullItemName = "Armor";
			}
			//ff1.3 end
			return false;	// can't hold any more, so leave the item
		}
		amount = atoi( value );
		if ( amount ) {
			armor += amount;
			if ( armor > maxarmor ) {
				armor = maxarmor;
			}
			nextArmorDepleteTime = 0;
			armorPulse = true;
			AddPickupName( "Armor", "", amount, owner ); //ff1.3 - TODO get name from a def?
		}
	} else if ( idStr::FindText( statname, "inclip_" ) == 0 ) {
#ifdef _D3XP
		idStr temp = statname;
		i = atoi(temp.Mid(7, 2));
#else
		i = WeaponIndexForAmmoClass( spawnArgs, statname + 7 );
#endif
		if ( i != -1 ) {
			// set, don't add. not going over the clip size limit.
#ifndef _D3XP
			clip[ i ] = atoi( value );
#endif
		}
#ifdef _D3XP
	} else if ( !idStr::Icmp( statname, "invulnerability" ) ) {
		owner->GivePowerUp( INVULNERABILITY, SEC2MS( atof( value ) ) );
	} else if ( !idStr::Icmp( statname, "helltime" ) ) {
		owner->GivePowerUp( HELLTIME, SEC2MS( atof( value ) ) );
	} else if ( !idStr::Icmp( statname, "envirosuit" ) ) {
		owner->GivePowerUp( ENVIROSUIT, SEC2MS( atof( value ) ) );
		owner->GivePowerUp( ENVIROTIME, SEC2MS( atof( value ) ) );
	} else if ( !idStr::Icmp( statname, "berserk" ) ) {
		owner->GivePowerUp( BERSERK, SEC2MS( atof( value ) ) );
	//} else if ( !idStr::Icmp( statname, "haste" ) ) {
	//	owner->GivePowerUp( HASTE, SEC2MS( atof( value ) ) );
#else
	} else if ( !idStr::Icmp( statname, "berserk" ) ) {
		GivePowerUp( owner, BERSERK, SEC2MS( atof( value ) ) );
#endif
	} else if ( !idStr::Icmp( statname, "mega" ) ) {
		GivePowerUp( owner, MEGAHEALTH, SEC2MS( atof( value ) ) );
	} else if ( !idStr::Icmp( statname, "weapon" ) ) {
		tookWeapon = false;
		for( pos = value; pos != NULL; pos = end ) {
			end = strchr( pos, ',' );
			if ( end ) {
				len = end - pos;
				end++;
			} else {
				len = strlen( pos );
			}

			idStr weaponName( pos, 0, len );

			// find the number of the matching weapon name
			for( i = 0; i < MAX_WEAPONS; i++ ) {
				if ( weaponName == spawnArgs.GetString( va( "def_weapon%d", i ) ) ) {
					break;
				}
			}

			if ( i >= MAX_WEAPONS ) {
#ifdef _D3XP
				gameLocal.Warning( "Unknown weapon '%s'", weaponName.c_str() );
				continue;
#else
				gameLocal.Error( "Unknown weapon '%s'", weaponName.c_str() );
#endif
			}

			// cache the media for this weapon
			weaponDecl = gameLocal.FindEntityDef( weaponName, false );

			// don't pickup "no ammo" weapon types twice
			// not for D3 SP .. there is only one case in the game where you can get a no ammo
			// weapon when you might already have it, in that case it is more conistent to pick it up
			if ( gameLocal.isMultiplayer && weaponDecl && ( weapons & ( 1 << i ) ) && !weaponDecl->dict.GetInt( "ammoRequired" ) ) {
				continue;
			}

			if ( !gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || ( weaponName == "weapon_fists" ) /*|| ( weaponName == "weapon_linker" )*/ /*|| ( weaponName == "weapon_soulcube" ) */ ) {
				if ( ( weapons & ( 1 << i ) ) == 0 || gameLocal.isMultiplayer ) {
					if ( owner->GetUserInfo()->GetBool( "ui_autoSwitch" ) && idealWeapon ) { //&& i != owner->weapon_bloodstone_active1 && i != owner->weapon_bloodstone_active2 && i != owner->weapon_bloodstone_active3
						assert( !gameLocal.isClient );
						*idealWeapon = i;
					}
					if ( owner->hud && updateHud && lastGiveTime + 1000 < gameLocal.time ) {
						owner->hud->SetStateInt( "newWeapon", i );
						owner->hud->HandleNamedEvent( "newWeapon" );
						lastGiveTime = gameLocal.time;
					}
					weaponPulse = true;
					weapons |= ( 1 << i );
					tookWeapon = true;
				}
			}
		}
		return tookWeapon;
	} else if ( !idStr::Icmp( statname, "item" ) || !idStr::Icmp( statname, "icon" ) || !idStr::Icmp( statname, "name" ) ) {
		// ignore these as they're handled elsewhere
		return false;
	} else {
		// unknown item
		gameLocal.Warning( "Unknown stat '%s' added to player's inventory", statname );
		return false;
	}

	return true;
}

/*
===============
idInventoy::Drop
===============
*/
int idInventory::Drop( const idDict &spawnArgs, const char *weapon_classname, int weapon_index ) {
	// remove the weapon bit
	// also remove the ammo associated with the weapon as we pushed it in the item
	assert( weapon_index != -1 || weapon_classname );
	if ( weapon_index == -1 ) {
		for( weapon_index = 0; weapon_index < MAX_WEAPONS; weapon_index++ ) {
			if ( !idStr::Icmp( weapon_classname, spawnArgs.GetString( va( "def_weapon%d", weapon_index ) ) ) ) {
				break;
			}
		}
		if ( weapon_index >= MAX_WEAPONS ) {
			gameLocal.Error( "Unknown weapon '%s'", weapon_classname );
		}
	} else if ( !weapon_classname ) {
		weapon_classname = spawnArgs.GetString( va( "def_weapon%d", weapon_index ) );
	}
	weapons &= ( 0xffffffff ^ ( 1 << weapon_index ) );
	ammo_t ammo_i = AmmoIndexForWeaponClass( weapon_classname, NULL );
	if ( ammo_i ) {
		//gameLocal.Printf( "clip[ %d ] = -1\n", weapon_index ); //temp
		clip[ weapon_index ] = -1;
		ammo[ ammo_i ] = 0;

		//ff1.3 - weapon_skull fix
		if( ammo_i == skullAmmoType ){
			for( int i = 0; i < MAX_SKULL_MODES; i++ ) {
				skullSpecialAmmo[i] = 0;
			}
		}
	}
	pw_ammo[ weapon_index ] = 0; //ff1.1
	return weapon_index; //ff1.3
}

/*
===============
idInventory::HasAmmo
===============
*/
int idInventory::HasAmmo( ammo_t type, int amount ) {
	if ( ( type == 0 ) || !amount ) {
		// always allow weapons that don't use ammo to fire
		return -1;
	}

	// check if we have infinite ammo
	if ( ammo[ type ] < 0 ) {
		return -1;
	}

	// return how many shots we can fire
	return ammo[ type ] / amount;

}

/*
===============
idInventory::HasAmmo
===============
*/
int idInventory::HasAmmo( const char *weapon_classname, bool includeClip, idPlayer* owner ) {		//_D3XP
	int ammoRequired;
	ammo_t ammo_i = AmmoIndexForWeaponClass( weapon_classname, &ammoRequired );

#ifdef _D3XP
	int ammoCount = HasAmmo( ammo_i, ammoRequired );
	if(includeClip && owner) {
		//ff1.3 start - fix for new weapons having clip -1
		//was: ammoCount += clip[owner->SlotForWeapon(weapon_classname)];
		ammoRequired = clip[owner->SlotForWeapon(weapon_classname)];
		if ( ammoRequired > 0 ) {
			ammoCount += ammoRequired;
		}
		//ff1.3 end
	}
	return ammoCount;
#else
	return HasAmmo( ammo_i, ammoRequired );
#endif

}

#ifdef _D3XP
/*
===============
idInventory::HasEmptyClipCannotRefill
===============
*/
bool idInventory::HasEmptyClipCannotRefill(const char *weapon_classname, idPlayer* owner) {

	int clipSize = clip[owner->SlotForWeapon(weapon_classname)];
	if(clipSize) {
		return false;
	}

	const idDeclEntityDef *decl = gameLocal.FindEntityDef( weapon_classname, false );
	if ( !decl ) {
		gameLocal.Error( "Unknown weapon in decl '%s'", weapon_classname );
	}
	int minclip = decl->dict.GetInt("minclipsize");
	if(!minclip) {
		return false;
	}

	ammo_t ammo_i = AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
	int ammoRequired = decl->dict.GetInt( "ammoRequired" );
	int ammoCount = HasAmmo( ammo_i, ammoRequired );
	if(ammoCount < minclip) {
		return true;
	}
	return false;
}
#endif

/*
===============
idInventory::UseAmmo
===============
*/
bool idInventory::UseAmmo( ammo_t type, int amount ) {
	if ( !HasAmmo( type, amount ) ) {
		return false;
	}

	// take an ammo away if not infinite
	if ( ammo[ type ] >= 0 ) {
		ammo[ type ] -= amount;
		ammoPredictTime = gameLocal.time; // mp client: we predict this. mark time so we're not confused by snapshots
	}

	return true;
}

/*
===============
idInventory::UpdateArmor
===============
*/
void idInventory::UpdateArmor( void ) {
	if ( deplete_armor != 0.0f && deplete_armor < armor ) {
		if ( !nextArmorDepleteTime ) {
			nextArmorDepleteTime = gameLocal.time + deplete_rate * 1000;
		} else if ( gameLocal.time > nextArmorDepleteTime ) {
			armor -= deplete_ammount;
			if ( armor < deplete_armor ) {
				armor = deplete_armor;
			}
			nextArmorDepleteTime = gameLocal.time + deplete_rate * 1000;
		}
	}
}

#ifdef _D3XP

//ff1.3 start - RechargeAmmo removed

/*
===============
idInventory::InitRechargeAmmo
===============
* Loads any recharge ammo definitions from the ammo_types entity definitions.

void idInventory::InitRechargeAmmo(idPlayer *owner) {

	memset (rechargeAmmo, 0, sizeof(rechargeAmmo));

	const idKeyValue *kv = owner->spawnArgs.MatchPrefix( "ammorecharge_" );
	while ( kv ) {
		idStr key = kv->GetKey();
		idStr ammoname = key.Right(key.Length()- strlen("ammorecharge_"));
		int ammoType = AmmoIndexForAmmoClass(ammoname);
		rechargeAmmo[ammoType].ammo = (atof(kv->GetValue().c_str())*1000);
		strcpy(rechargeAmmo[ammoType].ammoName, ammoname);
		kv = owner->spawnArgs.MatchPrefix( "ammorecharge_", kv );
	}
}
*/

/*
===============
idInventory::RechargeAmmo
===============
* Called once per frame to update any ammo amount for ammo types that recharge.

void idInventory::RechargeAmmo(idPlayer *owner) {

	for(int i = 0; i < AMMO_NUMTYPES; i++) {
		if (rechargeAmmo[i].ammo > 0) {
			if (!rechargeAmmo[i].rechargeTime)  {
				//Initialize the recharge timer.
				rechargeAmmo[i].rechargeTime = gameLocal.time;
			}
			int elapsed = gameLocal.time - rechargeAmmo[i].rechargeTime;
			if (elapsed >= rechargeAmmo[i].ammo) {
				int intervals = (gameLocal.time - rechargeAmmo[i].rechargeTime)/rechargeAmmo[i].ammo;
				ammo[i] += intervals;

				int max = MaxAmmoForAmmoClass(owner, rechargeAmmo[i].ammoName);
				if (max > 0) {
					if (ammo[i] > max) {
						ammo[i] = max;
					}
				}
				rechargeAmmo[i].rechargeTime += intervals*rechargeAmmo[i].ammo;
			}
		}
	}
}
*/
//ff1.3 end

/*
===============
idInventory::CanGive
===============
*/
bool idInventory::CanGive( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon ) {
	/* ff1-3 - removed
	if ( !idStr::Icmp( statname, "ammo_bloodstone" ) ) {
		int max = MaxAmmoForAmmoClass(owner, statname);
		int i = AmmoIndexForAmmoClass(statname);

		if(max <= 0) {
			//No Max
			return true;
		} else {
			//Already at or above the max so don't allow the give
			if(ammo[ i ] >= max) {
				ammo[ i ] = max;
				return false;
			}
			return true;
		}
	} else
	*/
	if ( !idStr::Icmp( statname, "item" ) || !idStr::Icmp( statname, "icon" ) || !idStr::Icmp( statname, "name" ) ) {
		// ignore these as they're handled elsewhere
		//These items should not be considered as succesful gives because it messes up the max ammo items
		return false;
	}
	return true;
}
#endif

//ff1.1 start

/*
===============
idInventory::UseAvailAmmo
===============
*/
bool idInventory::UseAvailAmmo( ammo_t type, int amount ) {
	if ( ( type == 0 ) || !amount ) {
		return false;
	}

	// check if we have infinite ammo
	if ( ammo[ type ] < 0 ) {
		return false;
	}

	// take an ammo away if not infinite
	if ( ammo[ type ] >= 0 ) {
		ammo[ type ] -= amount;
		if ( ammo[ type ] < 0 ) {
			ammo[ type ] = 0;
		}
		ammoPredictTime = gameLocal.time; // mp client: we predict this. mark time so we're not confused by snapshots
	}
	return true;
}
//ff1.1 end

//ff1.3 start

/*
===============
idInventory::HasPwAmmo
===============
*/
int idInventory::HasPwAmmo( int weaponIndex ) {
	if ( weaponIndex < 0 || weaponIndex >= MAX_WEAPONS ) {
		return -1;
	}
	return pw_ammo[weaponIndex];
}


/*
===============
idInventory::UsePwAmmo
===============
*/
bool idInventory::UsePwAmmo( int weaponIndex, int amount ) {
	if ( weaponIndex < 0 || weaponIndex >= MAX_WEAPONS ) {
		return false;
	}
	pw_ammo[weaponIndex] -= amount;
	if ( pw_ammo[weaponIndex] < 0 ) {
		pw_ammo[weaponIndex] = 0;
	}

	return true;
}

/*
===============
idInventory::AddPwAmmo
===============
*/
void idInventory::AddPwAmmo( int weaponIndex, int amount ) {
	if ( weaponIndex < 0 || weaponIndex >= MAX_WEAPONS ) {
		return;
	}
	pw_ammo[weaponIndex] += amount;
}


/*
===============
idInventory::UseSkullAmmo
===============
*/
bool idInventory::UseSkullAmmo( int mode, int amount ) {
	if ( !UseAmmo( skullAmmoType, amount ) ) { //use normal ammo
		return false;
	}

	if ( mode != SKULL_MODE_NORMAL ) { //use special ammo too
		if ( mode > SKULL_MODE_NORMAL && mode < MAX_SKULL_MODES ) {
			skullSpecialAmmo[mode] -= amount;
			//gameLocal.Printf( "SkullSpecialAmmo mode %d now is %d\n", mode, skullSpecialAmmo[mode] );
		} else {
			//gameLocal.Warning( "Invalid skull mode: %d\n", mode );
			return false;
		}
	}
	return true;
}

/*
===============
idInventory::HasSkullAmmo
===============
*/
int idInventory::HasSkullAmmo( int mode ) {
	int ammo;
	if ( mode == SKULL_MODE_NORMAL ) {
		ammo = HasAmmo( skullAmmoType, 1 );
		int i;
		for( i = 0; i < MAX_SKULL_MODES; i++ ) {
			ammo -= skullSpecialAmmo[i];
		}
	} else if ( mode > SKULL_MODE_NORMAL && mode < MAX_SKULL_MODES ) {
		ammo = skullSpecialAmmo[mode];
	} else {
		ammo = -1;
	}
	return ammo;
}

/*
===============
idInventory::Init
===============
*/
void idInventory::Init( void ) {
	//init skull ammo
	skullAmmoType = AmmoIndexForAmmoClass( "ammo_skulls" );
	//gameLocal.Printf( "skullAmmoType is %d\n", (int)skullAmmoType );
}

//ff1.3 end


/*
==============
idPlayer::idPlayer
==============
*/
idPlayer::idPlayer() {
	memset( &usercmd, 0, sizeof( usercmd ) );

	noclip					= false;
	godmode					= false;

	spawnAnglesSet			= false;
	spawnAngles				= ang_zero;
	viewAngles				= ang_zero;
	cmdAngles				= ang_zero;

	oldButtons				= 0;
	buttonMask				= 0;
	oldFlags				= 0;

	lastHitTime				= 0;
	lastSndHitTime			= 0;
	lastSavingThrowTime		= 0;

	weapon					= NULL;

	hud						= NULL;
	objectiveSystem			= NULL;
	objectiveSystemOpen		= false;

	//ivan start
	infoGui					= NULL;
	//infoGuiOpen				= false;
	//ivan end

#ifdef _D3XP
	mountedObject			= NULL;
	enviroSuitLight			= NULL;
#endif

	heartRate				= BASE_HEARTRATE;
	heartInfo.Init( 0, 0, 0, 0 );
	lastHeartAdjust			= 0;
	lastHeartBeat			= 0;
	lastDmgTime				= 0;
	deathClearContentsTime	= 0;
	lastArmorPulse			= -10000;
	//ff1.3 start
	//stamina					= 0.0f;
	staminaHelltime			= 0;
	staminaEnabled			= true;
	nextStaminaDrop			= 0;
	nextHealthDrop			= 0;
	healthAdded				= 0;
	nextHealthAddedReset	= 0;
	memset( &lastKillTimes, -MASSACRE_TIME_DELTA, sizeof( lastKillTimes ) );
	tempKillCount			= 0;
	nextKillUpdate			= 0;
	hudPulseFlags			= 0;
	//ff1.3 end

	healthPool				= 0.0f;
	nextHealthPulse			= 0;
	healthPulse				= false;
	//nextHealthTake			= 0;
	//healthTake				= false;

	scoreBoardOpen			= false;
	forceScoreBoard			= false;
	forceRespawn			= false;
	spectating				= false;
	spectator				= 0;
	colorBar				= vec3_zero;
	colorBarIndex			= 0;
	forcedReady				= false;
	wantSpectate			= false;

#ifdef CTF
	carryingFlag			= false;
#endif

	lastHitToggle			= false;

	minRespawnTime			= 0;
	maxRespawnTime			= 0;

	firstPersonViewOrigin	= vec3_zero;
	firstPersonViewAxis		= mat3_identity;

	hipJoint				= INVALID_JOINT;
	chestJoint				= INVALID_JOINT;
	headJoint				= INVALID_JOINT;

	bobFoot					= 0;
	bobFrac					= 0.0f;
	bobfracsin				= 0.0f;
	bobCycle				= 0;
	xyspeed					= 0.0f;
	stepUpTime				= 0;
	stepUpDelta				= 0.0f;
	idealLegsYaw			= 0.0f;
	legsYaw					= 0.0f;
	legsForward				= true;
	oldViewYaw				= 0.0f;
	viewBobAngles			= ang_zero;
	viewBob					= vec3_zero;
	landChange				= 0;
	landTime				= 0;

	currentWeapon			= -1;
	idealWeapon				= -1;
	previousWeapon			= -1;
	weaponSwitchTime		=  0;
	weaponEnabled			= true;
	weaponEnabledOnRideExit	= true;
	//weapon_soulcube			= -1;
	weapon_possession		= -1;
	weapon_soul2cube		= -1;
	weapon_skull			= -1;
	weapon_pda				= -1;
	//weapon_linker			= -1;
	weapon_shockrifle		= -1;
	weapon_fists			= -1;
#ifdef _D3XP
	/*
	weapon_bloodstone		= -1;
	weapon_bloodstone_active1 = -1;
	weapon_bloodstone_active2 = -1;
	weapon_bloodstone_active3 = -1;
	*/
	harvest_lock			= false;
	/*
	hudPowerup				= -1;
	lastHudPowerup			= -1;
	hudPowerupDuration		= 0;
	*/
#endif
	showWeaponViewModel		= true;

	skin					= NULL;
	powerUpSkin				= NULL;
	baseSkinName			= "";
	/*
	projFired		= 0;
	projHits		= 0;
	*/
	//airless					= false;
	//airTics					= 0;
	//lastAirDamage			= 0;

	gibDeath				= false;
	gibsLaunched			= false;
	gibsDir					= vec3_zero;

	zoomFov.Init( 0, 0, 0, 0 );
	centerView.Init( 0, 0, 0, 0 );
	fxFov					= false;

	influenceFov			= 0;
	influenceActive			= 0;
	influenceRadius			= 0.0f;
	influenceEntity			= NULL;
	influenceMaterial		= NULL;
	influenceSkin			= NULL;

	privateCameraView		= NULL;

	memset( loggedViewAngles, 0, sizeof( loggedViewAngles ) );
	memset( loggedAccel, 0, sizeof( loggedAccel ) );
	currentLoggedAccel	= 0;

	focusTime				= 0;
	focusGUIent				= NULL;
	focusUI					= NULL;
	focusCharacter			= NULL;
	focusGameCover			= NULL; //ff1.3
	talkCursor				= CURSOR_STATE_HIDDEN;
	focusVehicle			= NULL;
	cursor					= NULL;

    //ff1.3 start
    //currentVehicle			= NULL;  //moved to actor
    currentRiddenAI			= NULL;
    nextVehicleTime			= 0;
    advancedWeaponZoomTime	= 0;
    advancedWeaponZooming	= false;
    quickZoomFov			= 0.0f;
    memset( &hitCountGroupIds, 0, sizeof( hitCountGroupIds ) );
    nextHitCountGroupIndex	= 0;
    lastEscPressedTime		= 0;
    skipCinematicTipStartTime	= 0;
    selectedGameCoverIndex	= 0;
    selectedStatsIndex		= 0;
    maxMapStatsIndex		= 0;
    //ff1.3 end

	oldMouseX				= 0;
	oldMouseY				= 0;

	pdaAudio				= "";
	pdaVideo				= "";
	pdaVideoWave			= "";

	lastDamageDef			= 0;
	lastDamageDir			= vec3_zero;
	lastDamageLocation		= 0;
	smoothedFrame			= 0;
	smoothedOriginUpdated	= false;
	smoothedOrigin			= vec3_zero;
	smoothedAngles			= ang_zero;

	fl.networkSync			= true;

	latchedTeam				= -1;
	doingDeathSkin			= false;
	weaponGone				= false;
	useInitialSpawns		= false;
	tourneyRank				= 0;
	lastSpectateTeleport	= 0;
	tourneyLine				= 0;
	hiddenWeapon			= false;
	tipUp					= false;
	objectiveUp				= false;
	teleportEntity			= NULL;
	teleportKiller			= -1;
	respawning				= false;
	ready					= false;
	leader					= false;
	lastSpectateChange		= 0;
	lastTeleFX				= -9999;
	weaponCatchup			= false;
	lastSnapshotSequence	= 0;

	MPAim					= -1;
	lastMPAim				= -1;
	lastMPAimTime			= 0;
	MPAimFadeTime			= 0;
	MPAimHighlight			= false;

	spawnedTime				= 0;
	lastManOver				= false;
	lastManPlayAgain		= false;
	lastManPresent			= false;

	isTelefragged			= false;

	isLagged				= false;
	isChatting				= false;

	selfSmooth				= false;

#ifdef _DENTONMOD
	memset( &weaponZoom, 0, sizeof( weaponZoom ) );
	memset(	projectileType, 0, sizeof(projectileType) );
#endif

	//ff1.3 start
	memset( &playerStats, 0, sizeof( playerStats ) );
	campaignSoulsCount		= 0;
	currentStatsIndex		= 0;
	//medalsQueue.Clear();
	nextMedalTime			= 0;
	medalsEnabled			= true;
	inGameStatsVisible		= false;
	inGameStatsCurrentMap	= false;
	nextInvulnWarningTime	= 0;
	//ff1.3 end
}

/*
==============
idPlayer::LinkScriptVariables

set up conditions for animation
==============
*/
void idPlayer::LinkScriptVariables( void ) {
	AI_FORWARD.LinkTo(			scriptObject, "AI_FORWARD" );
	AI_BACKWARD.LinkTo(			scriptObject, "AI_BACKWARD" );
	AI_STRAFE_LEFT.LinkTo(		scriptObject, "AI_STRAFE_LEFT" );
	AI_STRAFE_RIGHT.LinkTo(		scriptObject, "AI_STRAFE_RIGHT" );
	AI_ATTACK_HELD.LinkTo(		scriptObject, "AI_ATTACK_HELD" );
	AI_WEAPON_FIRED.LinkTo(		scriptObject, "AI_WEAPON_FIRED" );
	AI_JUMP.LinkTo(				scriptObject, "AI_JUMP" );
	AI_DEAD.LinkTo(				scriptObject, "AI_DEAD" );
	AI_CROUCH.LinkTo(			scriptObject, "AI_CROUCH" );
	AI_ONGROUND.LinkTo(			scriptObject, "AI_ONGROUND" );
	AI_ONLADDER.LinkTo(			scriptObject, "AI_ONLADDER" );
	AI_HARDLANDING.LinkTo(		scriptObject, "AI_HARDLANDING" );
	AI_SOFTLANDING.LinkTo(		scriptObject, "AI_SOFTLANDING" );
	AI_RUN.LinkTo(				scriptObject, "AI_RUN" );
	AI_PAIN.LinkTo(				scriptObject, "AI_PAIN" );
	AI_RELOAD.LinkTo(			scriptObject, "AI_RELOAD" );
	AI_TELEPORT.LinkTo(			scriptObject, "AI_TELEPORT" );
	AI_TURN_LEFT.LinkTo(		scriptObject, "AI_TURN_LEFT" );
	AI_TURN_RIGHT.LinkTo(		scriptObject, "AI_TURN_RIGHT" );
	AI_DODGE.LinkTo(			scriptObject, "AI_DODGE" ); //ff1.3
}

/*
==============
idPlayer::SetupWeaponEntity
==============
*/
void idPlayer::SetupWeaponEntity( void ) {
	int w;
	const char *weap;

	if ( weapon.GetEntity() ) {
		// get rid of old weapon
		weapon.GetEntity()->Clear();
		currentWeapon = -1;
	} else if ( !gameLocal.isClient ) {
		weapon = static_cast<idWeapon *>( gameLocal.SpawnEntityType( idWeapon::Type, NULL ) );
		weapon.GetEntity()->SetOwner( this );
		currentWeapon = -1;
	}

	for( w = 0; w < MAX_WEAPONS; w++ ) {
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( weap && *weap ) {
			idWeapon::CacheWeapon( weap );
		}
	}

	UpdateEveryAmmoOnHud(); //ff1.1
}

/*
==============
idPlayer::Init
==============
*/
void idPlayer::Init( void ) {
	const char			*value;
	const idKeyValue	*kv;

	inventory.Init(); //ff1.3

	noclip					= false;
	godmode					= false;

	oldButtons				= 0;
	oldFlags				= 0;

#ifdef _DENTONMOD
	memset( &weaponZoom, 0, sizeof( weaponZoom ) ); // New
#endif

	currentWeapon			= -1;
	idealWeapon				= -1;
	previousWeapon			= -1;
	weaponSwitchTime		= 0;
	weaponEnabled			= true;
	weaponEnabledOnRideExit	= true;
	//weapon_soulcube			= SlotForWeapon( "weapon_soulcube" ); //ff1.3 - code removed
	weapon_possession		= SlotForWeapon( "weapon_bloodstone_possession" ); //ff1.3
	weapon_soul2cube		= SlotForWeapon( "weapon_soul2cube" ); //ff1.3
	weapon_skull			= SlotForWeapon( "weapon_bloodstone_skull" ); //ff1.3
	weapon_pda				= SlotForWeapon( "weapon_pda" );
	//weapon_linker			= SlotForWeapon( "weapon_linker" );
	weapon_shockrifle		= SlotForWeapon( "weapon_shockrifle" ); //ff1.3
	weapon_fists			= SlotForWeapon( "weapon_fists" ); //ff1.3

#ifdef _D3XP
	/*
	weapon_bloodstone		= SlotForWeapon( "weapon_bloodstone_passive" );
	weapon_bloodstone_active1 = SlotForWeapon( "weapon_bloodstone_active1" );
	weapon_bloodstone_active2 = SlotForWeapon( "weapon_bloodstone_active2" );
	weapon_bloodstone_active3 = SlotForWeapon( "weapon_bloodstone_active3" );
	*/
	harvest_lock			= false;
#endif
	showWeaponViewModel		= GetUserInfo()->GetBool( "ui_showGun" );


	lastDmgTime				= 0;
	lastArmorPulse			= -10000;
	lastHeartAdjust			= 0;
	lastHeartBeat			= 0;
	heartInfo.Init( 0, 0, 0, 0 );

	bobCycle				= 0;
	bobFrac					= 0.0f;
	landChange				= 0;
	landTime				= 0;
	zoomFov.Init( 0, 0, 0, 0 );
	centerView.Init( 0, 0, 0, 0 );
	fxFov					= false;

	influenceFov			= 0;
	influenceActive			= 0;
	influenceRadius			= 0.0f;
	influenceEntity			= NULL;
	influenceMaterial		= NULL;
	influenceSkin			= NULL;

#ifdef _D3XP
	mountedObject			= NULL;
	if( enviroSuitLight.IsValid() ) {
		enviroSuitLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}
	enviroSuitLight			= NULL;
	healthRecharge			= false;
	lastHealthRechargeTime	= 0;
	rechargeSpeed			= 500;
	new_g_damageScale		= 1.f;
	bloomEnabled			= false;
	bloomSpeed				= 1.f;
	bloomIntensity			= -0.01f;
	/*
	//inventory.InitRechargeAmmo(this); //ff1.3 - removed
	hudPowerup				= -1;
	lastHudPowerup			= -1;
	hudPowerupDuration		= 0;
	*/
#endif

	currentLoggedAccel		= 0;

	focusTime				= 0;
	focusGUIent				= NULL;
	focusUI					= NULL;
	focusCharacter			= NULL;
	focusGameCover			= NULL; //ff1.3
	talkCursor				= CURSOR_STATE_HIDDEN;
	focusVehicle			= NULL;

	//ff1.3 start
	currentVehicle			= NULL;
	currentRiddenAI			= NULL;
	nextVehicleTime			= 0;
	advancedWeaponZoomTime	= 0;
	advancedWeaponZooming	= false;
	quickZoomFov			= 0.0f;
	memset( &hitCountGroupIds, 0, sizeof( hitCountGroupIds ) );
	nextHitCountGroupIndex	= 0;

	//stats - must be done before RestorePersistantInfo();
	memset( &playerStats, 0, sizeof( playerStats ) );
	campaignSoulsCount = 0;
	currentStatsIndex = 0;

	//ff1.3 end

	// remove any damage effects
	playerView.ClearEffects();

	// damage values
	fl.takedamage			= true;
	ClearPain();

	// restore persistent data
	RestorePersistantInfo();

	bobCycle				= 0;
	//ff1.3 start
	//stamina			= 0.0f;
	//staminaHelltime			= 0;   //set by RestorePersistantInfo
	staminaEnabled			= true;
	nextStaminaDrop			= 0;
	nextHealthDrop			= 0;
	healthAdded				= 0;
	nextHealthAddedReset	= 0;
	memset( &lastKillTimes, -MASSACRE_TIME_DELTA, sizeof( lastKillTimes ) );
	tempKillCount			= 0;
	nextKillUpdate			= 0;
	hudPulseFlags			= 0;
	//ff1.3 end

	healthPool				= 0.0f;
	nextHealthPulse			= 0;
	healthPulse				= false;
	//nextHealthTake			= 0;
	//healthTake				= false;

	SetupWeaponEntity();
	currentWeapon = -1;
	previousWeapon = -1;

	heartRate = BASE_HEARTRATE;
	AdjustHeartRate( BASE_HEARTRATE, 0.0f, 0.0f, true );

	idealLegsYaw = 0.0f;
	legsYaw = 0.0f;
	legsForward	= true;
	oldViewYaw = 0.0f;

	// set the pm_ cvars
	if ( !gameLocal.isMultiplayer || gameLocal.isServer ) {
		kv = spawnArgs.MatchPrefix( "pm_", NULL );
		while( kv ) {
			cvarSystem->SetCVarString( kv->GetKey(), kv->GetValue() );
			kv = spawnArgs.MatchPrefix( "pm_", kv );
		}
	}

	//ff1.3 start - disable stamina
	/* was:
	// disable stamina on hell levels
	if ( gameLocal.world && gameLocal.world->spawnArgs.GetBool( "no_stamina" ) ) {
		pm_stamina.SetFloat( 0.0f );
	}

	// stamina always initialized to maximum
	stamina = pm_stamina.GetFloat();
	*/

	//medals
	medalsQueue.Clear();
	nextMedalTime = 0;
	medalsEnabled = true;
	inGameStatsVisible = false;
	inGameStatsCurrentMap = false;
	nextInvulnWarningTime = 0;
	//ff1.3 end

	// air always initialized to maximum too
	//airTics = pm_airTics.GetFloat();
	//airless = false;

	gibDeath = false;
	gibsLaunched = false;
	gibsDir.Zero();

	// set the gravity
	physicsObj.SetGravity( gameLocal.GetGravity() );

	// start out standing
	SetEyeHeight( pm_normalviewheight.GetFloat() );

	stepUpTime = 0;
	stepUpDelta = 0.0f;
	viewBobAngles.Zero();
	viewBob.Zero();

	if ( !spawnArgs.GetString( "spawn_model", NULL, &value ) ) { //ff1.3 - add support for custom model
		value = spawnArgs.GetString( "model" );
	}
	if ( value && ( *value != 0 ) ) {
		SetModel( value );
	}

	//ff1.3 start
	if ( spawnArgs.GetString( "spawn_def_head", NULL, &value ) && head.GetEntity() ) {
		head.GetEntity()->SetModel(value);
	}
	//ff1.3 end

	if ( cursor ) {
		cursor->SetStateInt( "talkcursor", CURSOR_STATE_HIDDEN );
		cursor->SetStateString( "combatcursor", "1" );
		cursor->SetStateString( "itemcursor", "0" );
		cursor->SetStateString( "guicursor", "0" );
#ifdef _D3XP
		cursor->SetStateString( "grabbercursor", "0" );
#endif
	}

	if ( ( gameLocal.isMultiplayer || g_testDeath.GetBool() ) && skin ) {
		SetSkin( skin );
		renderEntity.shaderParms[6] = 0.0f;
	} else if ( spawnArgs.GetString( "spawn_skin", NULL, &value ) ) {
		skin = declManager->FindSkin( value );
		SetSkin( skin );
		renderEntity.shaderParms[6] = 0.0f;
	}

	value = spawnArgs.GetString( "bone_hips", "" );
	hipJoint = animator.GetJointHandle( value );
	if ( hipJoint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for 'bone_hips' on '%s'", value, name.c_str() );
	}

	value = spawnArgs.GetString( "bone_chest", "" );
	chestJoint = animator.GetJointHandle( value );
	if ( chestJoint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for 'bone_chest' on '%s'", value, name.c_str() );
	}

	value = spawnArgs.GetString( "bone_head", "" );
	headJoint = animator.GetJointHandle( value );
	if ( headJoint == INVALID_JOINT ) {
		gameLocal.Error( "Joint '%s' not found for 'bone_head' on '%s'", value, name.c_str() );
	}

	// initialize the script variables
	AI_FORWARD		= false;
	AI_BACKWARD		= false;
	AI_STRAFE_LEFT	= false;
	AI_STRAFE_RIGHT	= false;
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED	= false;
	AI_JUMP			= false;
	AI_DEAD			= false;
	AI_CROUCH		= false;
	AI_ONGROUND		= true;
	AI_ONLADDER		= false;
	AI_HARDLANDING	= false;
	AI_SOFTLANDING	= false;
	AI_RUN			= false;
	AI_PAIN			= false;
	AI_RELOAD		= false;
	AI_TELEPORT		= false;
	AI_TURN_LEFT	= false;
	AI_TURN_RIGHT	= false;
	AI_DODGE		= false; //ff1.3

	// reset the script object
	ConstructScriptObject();

	// execute the script so the script object's constructor takes effect immediately
	scriptThread->Execute();

	forceScoreBoard		= false;
	forcedReady			= false;

	privateCameraView	= NULL;

	lastSpectateChange	= 0;
	lastTeleFX			= -9999;

	hiddenWeapon		= false;
	tipUp				= false;
	objectiveUp			= false;
	teleportEntity		= NULL;
	teleportKiller		= -1;
	leader				= false;

	SetPrivateCameraView( NULL );

	lastSnapshotSequence	= 0;

	MPAim				= -1;
	lastMPAim			= -1;
	lastMPAimTime		= 0;
	MPAimFadeTime		= 0;
	MPAimHighlight		= false;

	if ( hud ) {
		hud->HandleNamedEvent( "aim_clear" );
	}

	//isChatting = false;
	cvarSystem->SetCVarBool("ui_chat", false);

}

/*
==============
idPlayer::Spawn

Prepare any resources used by the player.
==============
*/
void idPlayer::Spawn( void ) {
	idStr		temp;
	idBounds	bounds;

	if ( entityNumber >= MAX_CLIENTS ) {
		gameLocal.Error( "entityNum > MAX_CLIENTS for player.  Player may only be spawned with a client." );
	}

	// allow thinking during cinematics
	cinematic = true;

	if ( gameLocal.isMultiplayer ) {
		// always start in spectating state waiting to be spawned in
		// do this before SetClipModel to get the right bounding box
		spectating = true;
	}

	// set our collision model
	physicsObj.SetSelf( this );
	SetClipModel();
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );
	physicsObj.SetContents( CONTENTS_BODY );
	physicsObj.SetClipMask( MASK_PLAYERSOLID );
	SetPhysics( &physicsObj );
	InitAASLocation();

	skin = renderEntity.customSkin;

	// only the local player needs guis
	if ( !gameLocal.isMultiplayer || entityNumber == gameLocal.localClientNum ) {

		// load HUD
		if ( gameLocal.isMultiplayer ) {
			hud = uiManager->FindGui( "guis/mphud.gui", true, false, true );
		} else if ( spawnArgs.GetString( "hud", "", temp ) ) {
			hud = uiManager->FindGui( temp, true, false, true );
		}
		if ( hud ) {
			hud->Activate( true, gameLocal.time );
#ifdef CTF
			if ( gameLocal.mpGame.IsGametypeFlagBased() ) {
				hud->SetStateInt( "red_team_score", gameLocal.mpGame.GetFlagPoints(0) );
				hud->SetStateInt( "blue_team_score", gameLocal.mpGame.GetFlagPoints(1) );
			}
#endif
		}

		// load cursor
		if ( spawnArgs.GetString( "cursor", "", temp ) ) {
			cursor = uiManager->FindGui( temp, true, gameLocal.isMultiplayer, gameLocal.isMultiplayer );
		}
		if ( cursor ) {
			cursor->Activate( true, gameLocal.time );
		}

		objectiveSystem = uiManager->FindGui( "guis/pda.gui", true, false, true );
		objectiveSystemOpen = false;

		//ivan start
		if ( objectiveSystem ) { //default primary objective
			objectiveSystem->SetStateString( "missionobjective", "Find the exit." );
		}

		infoGui = uiManager->FindGui( "guis/info.gui", true, false, true );
		//infoGuiOpen = false;
		//todo: activate only when visible, see objectiveSystem
		if ( infoGui ) {
			infoGui->Activate( true, gameLocal.time );
		}
		//ivan end
	}

	SetLastHitTime( 0 );

	// load the armor sound feedback
	declManager->FindSound( "player_sounds_hitArmor" );

	// set up conditions for animation
	LinkScriptVariables();

	animator.RemoveOriginOffset( true );

	// initialize user info related settings
	// on server, we wait for the userinfo broadcast, as this controls when the player is initially spawned in game
	if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
		UserInfoChanged( false );
	}

	// create combat collision hull for exact collision detection
	SetCombatModel();

	// init the damage effects
	playerView.SetPlayerEntity( this );

	// supress model in non-player views, but allow it in mirrors and remote views
	renderEntity.suppressSurfaceInViewID = entityNumber+1;

	// don't project shadow on self or weapon
	renderEntity.noSelfShadow = true;

	idAFAttachment *headEnt = head.GetEntity();
	if ( headEnt ) {
		headEnt->GetRenderEntity()->suppressSurfaceInViewID = entityNumber+1;
		headEnt->GetRenderEntity()->noSelfShadow = true;
	}

	if ( gameLocal.isMultiplayer ) {
		Init();
		Hide();	// properly hidden if starting as a spectator
		if ( !gameLocal.isClient ) {
			// set yourself ready to spawn. idMultiplayerGame will decide when/if appropriate and call SpawnFromSpawnSpot
			SetupWeaponEntity();
			SpawnFromSpawnSpot();
			forceRespawn = true;
			assert( spectating );
		}
	} else {
		SetupWeaponEntity();
		SpawnFromSpawnSpot();
	}

	// trigger playtesting item gives, if we didn't get here from a previous level
	// the devmap key will be set on the first devmap, but cleared on any level
	// transitions
	if ( !gameLocal.isMultiplayer && gameLocal.serverInfo.FindKey( "devmap" ) ) {
		// fire a trigger with the name "devmap"
		idEntity *ent = gameLocal.FindEntity( "devmap" );
		if ( ent ) {
			ent->ActivateTargets( this );
		}
	}
	if ( hud ) {
		// We can spawn with a full soul cube, so we need to make sure the hud knows this
/*
#ifndef _D3XP
		if ( weapon_soulcube > 0 && ( inventory.weapons & ( 1 << weapon_soulcube ) ) ) {
			int max_souls = inventory.MaxAmmoForAmmoClass( this, "ammo_souls" );
			if ( inventory.ammo[ idWeapon::GetAmmoNumForName( "ammo_souls" ) ] >= max_souls ) {
				hud->HandleNamedEvent( "soulCubeReady" );
			}
		}
#endif
#ifdef _D3XP
		//We can spawn with a full bloodstone, so make sure the hud knows
		if ( weapon_bloodstone > 0 && ( inventory.weapons & ( 1 << weapon_bloodstone ) ) ) {
			//int max_blood = inventory.MaxAmmoForAmmoClass( this, "ammo_bloodstone" );
			//if ( inventory.ammo[ idWeapon::GetAmmoNumForName( "ammo_bloodstone" ) ] >= max_blood ) {
				hud->HandleNamedEvent( "bloodstoneReady" );
			//}
		}
#endif
*/
		hud->HandleNamedEvent( "itemPickup" );
	}

	if( !gameLocal.mapInfo.noDefaultPda && !GetPDA() ) {
		GivePDA(NULL, NULL); //give default (personal) pda
	}

	if ( GetPDA() ) {
		// Add any emails from the inventory
		for ( int i = 0; i < inventory.emails.Num(); i++ ) {
			GetPDA()->AddEmail( inventory.emails[i] );
		}
		GetPDA()->SetSecurity( common->GetLanguageDict()->GetString( "#str_00066" ) );
	}

	if ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) {
		hiddenWeapon = true;
		if ( weapon.GetEntity() ) {
			weapon.GetEntity()->LowerWeapon();
		}
		idealWeapon = 0;
	} else {
		hiddenWeapon = false;
	}

	if ( hud ) {
		UpdateHudWeapon();
		hud->StateChanged( gameLocal.time );
	}

	tipUp = false;
	objectiveUp = false;

	if ( inventory.levelTriggers.Num() ) {
		PostEventMS( &EV_Player_LevelTrigger, 0 );
	}

	inventory.pdaOpened = false;
	inventory.selPDA = 0;

	if ( !gameLocal.isMultiplayer ) {
		//ff start
		//g_armorProtection.SetFloat( 0.4f ); //?
		//ff end

		//ff1.4 start
		int startingHealth = gameLocal.world->spawnArgs.GetInt( "startingHealth", "0");
		if ( (startingHealth > 0) && (health > startingHealth) ) {
			health = startingHealth;
		}
		//ff1.4 end

		if ( g_skill.GetInteger() < 2 ) {
			if ( health < 25 ) {
				health = 25;
			}
			if ( g_useDynamicProtection.GetBool() ) {
#ifdef _D3XP
				new_g_damageScale = 1.0f;
#else
				g_damageScale.SetFloat( 1.0f );
#endif
			}
		} else {
#ifdef _D3XP
			new_g_damageScale = 1.0f;
#else
			g_damageScale.SetFloat( 1.0f );
#endif
			//ff start
			//g_armorProtection.SetFloat( ( g_skill.GetInteger() < 2 ) ? 0.4f : 0.2f );
			//ff end
			/*
			if ( g_skill.GetInteger() == 3 ) {
				healthTake = true;
				nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
			}
			*/
		}
	}

#ifdef _D3XP
	//Setup the weapon toggle lists
	const idKeyValue *kv;
	kv = spawnArgs.MatchPrefix( "weapontoggle", NULL );
	while( kv ) {
		WeaponToggle_t newToggle;
		strcpy(newToggle.name, kv->GetKey().c_str());
		//newToggle.lastSelected = -1; //ff1.3

		idStr toggleData = kv->GetValue();

		idLexer src;
		idToken token;
		src.LoadMemory(toggleData, toggleData.Length(), "toggleData");
		while(1) {
			if(!src.ReadToken(&token)) {
				break;
			}
			int index = atoi(token.c_str());
			newToggle.toggleList.Append(index);

			//Skip the ,
			src.ReadToken(&token);
		}
		weaponToggles.Set(newToggle.name, newToggle);

		kv = spawnArgs.MatchPrefix( "weapontoggle", kv );
	}
#endif

#ifdef _DENTONMOD
	memset(	projectileType, 0, sizeof(projectileType) );
#endif

#ifdef _D3XP
	/* ff - commented out
	if(g_skill.GetInteger() >= 3) {
		if(!WeaponAvailable("weapon_bloodstone_passive")) {
			GiveInventoryItem("weapon_bloodstone_passive");
		}
		if(!WeaponAvailable("weapon_bloodstone_active1")) {
			GiveInventoryItem("weapon_bloodstone_active1");
		}
		if(!WeaponAvailable("weapon_bloodstone_active2")) {
			GiveInventoryItem("weapon_bloodstone_active2");
		}
		if(!WeaponAvailable("weapon_bloodstone_active3")) {
			GiveInventoryItem("weapon_bloodstone_active3");
		}
	}
	*/

	bloomEnabled			= false;
	bloomSpeed				= 1;
	bloomIntensity			= -0.01f;
#endif

	//ff1.3 start
	quickZoomFov = spawnArgs.GetFloat( "zoomFov", "75" );
	//ff1.3 end
}

/*
==============
idPlayer::~idPlayer()

Release any resources used by the player.
==============
*/
idPlayer::~idPlayer() {
	delete weapon.GetEntity();
	weapon = NULL;
#ifdef CTF
	if ( enviroSuitLight.IsValid() ) {
		enviroSuitLight.GetEntity()->ProcessEvent( &EV_Remove );
	}
	// have to do this here, idMultiplayerGame::DisconnectClient() is too late
	if ( gameLocal.isMultiplayer && gameLocal.mpGame.IsGametypeFlagBased() ) {
		ReturnFlag();
	}
#endif
}

/*
===========
idPlayer::Save
===========
*/
void idPlayer::Save( idSaveGame *savefile ) const {
	int i;

	savefile->WriteUsercmd( usercmd );
	playerView.Save( savefile );

	savefile->WriteBool( noclip );
	savefile->WriteBool( godmode );

	// don't save spawnAnglesSet, since we'll have to reset them after loading the savegame
	savefile->WriteAngles( spawnAngles );
	savefile->WriteAngles( viewAngles );
	savefile->WriteAngles( cmdAngles );

	savefile->WriteInt( buttonMask );
	savefile->WriteInt( oldButtons );
	savefile->WriteInt( oldFlags );

	savefile->WriteInt( lastHitTime );
	savefile->WriteInt( lastSndHitTime );
	savefile->WriteInt( lastSavingThrowTime );

	// idBoolFields don't need to be saved, just re-linked in Restore

	inventory.Save( savefile );
	weapon.Save( savefile );

	savefile->WriteUserInterface( hud, false );
	savefile->WriteUserInterface( objectiveSystem, false );
	savefile->WriteBool( objectiveSystemOpen );

	//ivan start
	savefile->WriteUserInterface( infoGui, false );
	//savefile->WriteBool( infoGuiOpen );
	//ivan end

	//savefile->WriteInt( weapon_soulcube );
	savefile->WriteInt( weapon_possession );
	savefile->WriteInt( weapon_soul2cube );
	savefile->WriteInt( weapon_skull );
	savefile->WriteInt( weapon_pda );
	//savefile->WriteInt( weapon_linker );
	savefile->WriteInt( weapon_shockrifle );
	savefile->WriteInt( weapon_fists );
#ifdef _D3XP
	/*
	savefile->WriteInt( weapon_bloodstone );
	savefile->WriteInt( weapon_bloodstone_active1 );
	savefile->WriteInt( weapon_bloodstone_active2 );
	savefile->WriteInt( weapon_bloodstone_active3 );
	*/
	savefile->WriteBool( harvest_lock );
	/*
	savefile->WriteInt( hudPowerup );
	savefile->WriteInt( lastHudPowerup );
	savefile->WriteInt( hudPowerupDuration );
    */

#endif

	savefile->WriteInt( heartRate );

	savefile->WriteFloat( heartInfo.GetStartTime() );
	savefile->WriteFloat( heartInfo.GetDuration() );
	savefile->WriteFloat( heartInfo.GetStartValue() );
	savefile->WriteFloat( heartInfo.GetEndValue() );

	savefile->WriteInt( lastHeartAdjust );
	savefile->WriteInt( lastHeartBeat );
	savefile->WriteInt( lastDmgTime );
	savefile->WriteInt( deathClearContentsTime );
	savefile->WriteBool( doingDeathSkin );
	savefile->WriteInt( lastArmorPulse );
	//ff1.3 start
	//savefile->WriteFloat( stamina );
	savefile->WriteInt( staminaHelltime );
	savefile->WriteBool( staminaEnabled );
	savefile->WriteInt( nextStaminaDrop );
	savefile->WriteInt( nextHealthDrop );
	savefile->WriteInt( healthAdded );
	savefile->WriteInt( nextHealthAddedReset );
	for( i = 0; i < MASSACRE_KILLS; i++ ) {
		savefile->WriteInt( lastKillTimes[ i ] );
	}
	savefile->WriteInt( tempKillCount );
	savefile->WriteInt( nextKillUpdate );
	savefile->WriteInt( hudPulseFlags );
	//ff1.3 end
	savefile->WriteFloat( healthPool );
	savefile->WriteInt( nextHealthPulse );
	savefile->WriteBool( healthPulse );
	//savefile->WriteInt( nextHealthTake );
	//savefile->WriteBool( healthTake );

	savefile->WriteBool( hiddenWeapon );
	//soulCubeProjectile.Save( savefile ); //ff1.3 - removed
	//possessionProjectile.Save( savefile ); //ff1.3
	painKillerProjectile.Save( savefile ); //ff1.3
	remoteGrenadeProjectile.Save( savefile ); //ff1.3

	savefile->WriteInt( spectator );
	savefile->WriteVec3( colorBar );
	savefile->WriteInt( colorBarIndex );
	savefile->WriteBool( scoreBoardOpen );
	savefile->WriteBool( forceScoreBoard );
	savefile->WriteBool( forceRespawn );
	savefile->WriteBool( spectating );
	savefile->WriteInt( lastSpectateTeleport );
	savefile->WriteBool( lastHitToggle );
	savefile->WriteBool( forcedReady );
	savefile->WriteBool( wantSpectate );
	savefile->WriteBool( weaponGone );
	savefile->WriteBool( useInitialSpawns );
	savefile->WriteInt( latchedTeam );
	savefile->WriteInt( tourneyRank );
	savefile->WriteInt( tourneyLine );

	teleportEntity.Save( savefile );
	savefile->WriteInt( teleportKiller );

	savefile->WriteInt( minRespawnTime );
	savefile->WriteInt( maxRespawnTime );

	savefile->WriteVec3( firstPersonViewOrigin );
	savefile->WriteMat3( firstPersonViewAxis );

	// don't bother saving dragEntity since it's a dev tool

	savefile->WriteJoint( hipJoint );
	savefile->WriteJoint( chestJoint );
	savefile->WriteJoint( headJoint );

	savefile->WriteStaticObject( physicsObj );

	savefile->WriteInt( aasLocation.Num() );
	for( i = 0; i < aasLocation.Num(); i++ ) {
		savefile->WriteInt( aasLocation[ i ].areaNum );
		savefile->WriteVec3( aasLocation[ i ].pos );
	}

	savefile->WriteInt( bobFoot );
	savefile->WriteFloat( bobFrac );
	savefile->WriteFloat( bobfracsin );
	savefile->WriteInt( bobCycle );
	savefile->WriteFloat( xyspeed );
	savefile->WriteInt( stepUpTime );
	savefile->WriteFloat( stepUpDelta );
	savefile->WriteFloat( idealLegsYaw );
	savefile->WriteFloat( legsYaw );
	savefile->WriteBool( legsForward );
	savefile->WriteFloat( oldViewYaw );
	savefile->WriteAngles( viewBobAngles );
	savefile->WriteVec3( viewBob );
	savefile->WriteInt( landChange );
	savefile->WriteInt( landTime );

	savefile->WriteInt( currentWeapon );
	savefile->WriteInt( idealWeapon );
	savefile->WriteInt( previousWeapon );
	savefile->WriteInt( weaponSwitchTime );
	savefile->WriteBool( weaponEnabled );
	savefile->WriteBool( weaponEnabledOnRideExit ); //ff1.3
	savefile->WriteBool( showWeaponViewModel );

	savefile->WriteSkin( skin );
	savefile->WriteSkin( powerUpSkin );
	savefile->WriteString( baseSkinName );
	/*
	savefile->WriteInt( projFired );
	savefile->WriteInt( projHits );

	savefile->WriteBool( airless );
	savefile->WriteInt( airTics );
	savefile->WriteInt( lastAirDamage );
	*/

	savefile->WriteBool( gibDeath );
	savefile->WriteBool( gibsLaunched );
	savefile->WriteVec3( gibsDir );

	//ff1.3 start

	//stats
	savefile->WriteInt( playerStats.killCount );
	savefile->WriteInt( playerStats.soulsCount );
	savefile->WriteInt( playerStats.multikill );
	savefile->WriteInt( playerStats.skillshot );
	savefile->WriteInt( playerStats.doublekill );
	savefile->WriteInt( playerStats.massacre );
	savefile->WriteInt( playerStats.chainkill );
	savefile->WriteInt( playerStats.secretsFound );
	savefile->WriteInt( playerStats.gameCoversFound );
	savefile->WriteInt( playerStats.damage );
	savefile->WriteInt( playerStats.projFired );
	savefile->WriteInt( playerStats.projHits );

	savefile->WriteInt( campaignSoulsCount );
	savefile->WriteInt( currentStatsIndex );

	//medals
	savefile->WriteInt( medalsQueue.Num() );
	for( i = 0; i < medalsQueue.Num(); i++ ) {
		savefile->WriteInt( medalsQueue[ i ] );
	}
	savefile->WriteInt( nextMedalTime );
	savefile->WriteBool( medalsEnabled );
	savefile->WriteBool( inGameStatsVisible );
	savefile->WriteBool( inGameStatsCurrentMap );
	savefile->WriteInt( nextInvulnWarningTime );
	//ff1.3 end

#ifdef _DENTONMOD
	weaponZoom_s flags = weaponZoom;			// Save the weapon Zoom Info
	LittleBitField( &flags, sizeof( flags ) );
	savefile->Write( &flags, sizeof( flags ) );
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		savefile->WriteByte( projectileType[ i ] );
	}
#endif

	savefile->WriteFloat( zoomFov.GetStartTime() );
	savefile->WriteFloat( zoomFov.GetDuration() );
	savefile->WriteFloat( zoomFov.GetStartValue() );
	savefile->WriteFloat( zoomFov.GetEndValue() );

	savefile->WriteFloat( centerView.GetStartTime() );
	savefile->WriteFloat( centerView.GetDuration() );
	savefile->WriteFloat( centerView.GetStartValue() );
	savefile->WriteFloat( centerView.GetEndValue() );

	savefile->WriteBool( fxFov );

	savefile->WriteFloat( influenceFov );
	savefile->WriteInt( influenceActive );
	savefile->WriteFloat( influenceRadius );
	savefile->WriteObject( influenceEntity );
	savefile->WriteMaterial( influenceMaterial );
	savefile->WriteSkin( influenceSkin );

	savefile->WriteObject( privateCameraView );

	for( i = 0; i < NUM_LOGGED_VIEW_ANGLES; i++ ) {
		savefile->WriteAngles( loggedViewAngles[ i ] );
	}
	for( i = 0; i < NUM_LOGGED_ACCELS; i++ ) {
		savefile->WriteInt( loggedAccel[ i ].time );
		savefile->WriteVec3( loggedAccel[ i ].dir );
	}
	savefile->WriteInt( currentLoggedAccel );

	savefile->WriteObject( focusGUIent );
	// can't save focusUI
	savefile->WriteObject( focusCharacter );
	savefile->WriteInt( talkCursor );
	savefile->WriteInt( focusTime );
	savefile->WriteObject( focusVehicle );
	savefile->WriteObject( focusGameCover ); //ff1.3
	savefile->WriteUserInterface( cursor, false );

	//ff1.3 start
	//savefile->WriteObject( currentVehicle );  //moved to actor
	savefile->WriteObject( currentRiddenAI );
	savefile->WriteInt( nextVehicleTime );
	savefile->WriteInt( advancedWeaponZoomTime );
	savefile->WriteBool( advancedWeaponZooming );
	savefile->WriteFloat( quickZoomFov );
	for( i = 0; i < NUM_HIT_COUNT_GROUP_IDS; i++ ) {
		savefile->WriteInt( hitCountGroupIds[ i ] );
	}
	savefile->WriteInt( nextHitCountGroupIndex );
	savefile->WriteInt( lastEscPressedTime );
	savefile->WriteInt( skipCinematicTipStartTime );
	savefile->WriteInt( selectedGameCoverIndex );
	savefile->WriteInt( selectedStatsIndex );
	savefile->WriteInt( maxMapStatsIndex );
	//ff1.3 end

	savefile->WriteInt( oldMouseX );
	savefile->WriteInt( oldMouseY );

	savefile->WriteString( pdaAudio );
	savefile->WriteString( pdaVideo );
	savefile->WriteString( pdaVideoWave );

	savefile->WriteBool( tipUp );
	savefile->WriteBool( objectiveUp );

	savefile->WriteInt( lastDamageDef );
	savefile->WriteVec3( lastDamageDir );
	savefile->WriteInt( lastDamageLocation );
	savefile->WriteInt( smoothedFrame );
	savefile->WriteBool( smoothedOriginUpdated );
	savefile->WriteVec3( smoothedOrigin );
	savefile->WriteAngles( smoothedAngles );

	savefile->WriteBool( ready );
	savefile->WriteBool( respawning );
	savefile->WriteBool( leader );
	savefile->WriteInt( lastSpectateChange );
	savefile->WriteInt( lastTeleFX );

	//ff1.3 start
	//savefile->WriteFloat( pm_stamina.GetFloat() );
	if ( infoGui && gameLocal.time > 500 ) { //don't show on map start (it looks bad in intros)
		infoGui->SetStateString( "message", (gameLocal.IsAutoSaving() ? "Autosave..." : "Game Saved...") );
		infoGui->HandleNamedEvent( "Message" );
	}
	/* was:
	if ( hud ) {
		hud->SetStateString( "message", common->GetLanguageDict()->GetString( "#str_02916" ) );
		hud->HandleNamedEvent( "Message" );
	}
	*/
	//ff1.3 end

#ifdef _D3XP
	savefile->WriteInt(weaponToggles.Num());
	for(i = 0; i < weaponToggles.Num(); i++) {
		WeaponToggle_t* weaponToggle = weaponToggles.GetIndex(i);
		savefile->WriteString(weaponToggle->name);
		//savefile->WriteInt(weaponToggle->lastSelected); //ff1.3
		savefile->WriteInt(weaponToggle->toggleList.Num());
		for(int j = 0; j < weaponToggle->toggleList.Num(); j++) {
			savefile->WriteInt(weaponToggle->toggleList[j]);
		}
	}
	savefile->WriteObject( mountedObject );
	enviroSuitLight.Save( savefile );
	savefile->WriteBool( healthRecharge );
	savefile->WriteInt( lastHealthRechargeTime );
	savefile->WriteInt( rechargeSpeed );
	savefile->WriteFloat( new_g_damageScale );

	savefile->WriteBool( bloomEnabled );
	savefile->WriteFloat( bloomSpeed );
	savefile->WriteFloat( bloomIntensity );

#endif
}

/*
===========
idPlayer::Restore
===========
*/
void idPlayer::Restore( idRestoreGame *savefile ) {
	int	  i;
	int	  num;
	float set;

	savefile->ReadUsercmd( usercmd );
	playerView.Restore( savefile );

	savefile->ReadBool( noclip );
	savefile->ReadBool( godmode );

	savefile->ReadAngles( spawnAngles );
	savefile->ReadAngles( viewAngles );
	savefile->ReadAngles( cmdAngles );

	memset( usercmd.angles, 0, sizeof( usercmd.angles ) );
	SetViewAngles( viewAngles );
	spawnAnglesSet = true;

	savefile->ReadInt( buttonMask );
	savefile->ReadInt( oldButtons );
	savefile->ReadInt( oldFlags );

	usercmd.flags = 0;
	oldFlags = 0;

	savefile->ReadInt( lastHitTime );
	savefile->ReadInt( lastSndHitTime );
	savefile->ReadInt( lastSavingThrowTime );

	// Re-link idBoolFields to the scriptObject, values will be restored in scriptObject's restore
	LinkScriptVariables();

	inventory.Restore( savefile );
	weapon.Restore( savefile );

	for ( i = 0; i < inventory.emails.Num(); i++ ) {
		GetPDA()->AddEmail( inventory.emails[i] );
	}

	savefile->ReadUserInterface( hud );
	savefile->ReadUserInterface( objectiveSystem );
	savefile->ReadBool( objectiveSystemOpen );

	//ivan start
	savefile->ReadUserInterface( infoGui );
	//savefile->ReadBool( infoGuiOpen );
	//ivan end

	//savefile->ReadInt( weapon_soulcube );
	savefile->ReadInt( weapon_possession );
	savefile->ReadInt( weapon_soul2cube );
	savefile->ReadInt( weapon_skull );
	savefile->ReadInt( weapon_pda );
	//savefile->ReadInt( weapon_linker );
	savefile->ReadInt( weapon_shockrifle );
	savefile->ReadInt( weapon_fists );
#ifdef _D3XP
	/*
	savefile->ReadInt( weapon_bloodstone );
	savefile->ReadInt( weapon_bloodstone_active1 );
	savefile->ReadInt( weapon_bloodstone_active2 );
	savefile->ReadInt( weapon_bloodstone_active3 );
	*/
	savefile->ReadBool( harvest_lock );
	/*
	savefile->ReadInt( hudPowerup );
	savefile->ReadInt( lastHudPowerup );
	savefile->ReadInt( hudPowerupDuration );
    */

#endif

	savefile->ReadInt( heartRate );

	savefile->ReadFloat( set );
	heartInfo.SetStartTime( set );
	savefile->ReadFloat( set );
	heartInfo.SetDuration( set );
	savefile->ReadFloat( set );
	heartInfo.SetStartValue( set );
	savefile->ReadFloat( set );
	heartInfo.SetEndValue( set );

	savefile->ReadInt( lastHeartAdjust );
	savefile->ReadInt( lastHeartBeat );
	savefile->ReadInt( lastDmgTime );
	savefile->ReadInt( deathClearContentsTime );
	savefile->ReadBool( doingDeathSkin );
	savefile->ReadInt( lastArmorPulse );
	//ff1.3 start
	//savefile->ReadFloat( stamina );
	savefile->ReadInt( staminaHelltime );
	savefile->ReadBool( staminaEnabled );
	savefile->ReadInt( nextStaminaDrop );
	savefile->ReadInt( nextHealthDrop );
	savefile->ReadInt( healthAdded );
	savefile->ReadInt( nextHealthAddedReset );
	for( i = 0; i < MASSACRE_KILLS; i++ ) {
		savefile->ReadInt( lastKillTimes[ i ] );
	}
	savefile->ReadInt( tempKillCount );
	savefile->ReadInt( nextKillUpdate );
	savefile->ReadInt( hudPulseFlags );
	//ff1.3 end
	savefile->ReadFloat( healthPool );
	savefile->ReadInt( nextHealthPulse );
	savefile->ReadBool( healthPulse );
	//savefile->ReadInt( nextHealthTake );
	//savefile->ReadBool( healthTake );

	savefile->ReadBool( hiddenWeapon );
	//soulCubeProjectile.Restore( savefile ); //ff1.3 - removed
	//possessionProjectile.Restore( savefile ); //ff1.3
	painKillerProjectile.Restore( savefile ); //ff1.3
	remoteGrenadeProjectile.Restore( savefile ); //ff1.3

	savefile->ReadInt( spectator );
	savefile->ReadVec3( colorBar );
	savefile->ReadInt( colorBarIndex );
	savefile->ReadBool( scoreBoardOpen );
	savefile->ReadBool( forceScoreBoard );
	savefile->ReadBool( forceRespawn );
	savefile->ReadBool( spectating );
	savefile->ReadInt( lastSpectateTeleport );
	savefile->ReadBool( lastHitToggle );
	savefile->ReadBool( forcedReady );
	savefile->ReadBool( wantSpectate );
	savefile->ReadBool( weaponGone );
	savefile->ReadBool( useInitialSpawns );
	savefile->ReadInt( latchedTeam );
	savefile->ReadInt( tourneyRank );
	savefile->ReadInt( tourneyLine );

	teleportEntity.Restore( savefile );
	savefile->ReadInt( teleportKiller );

	savefile->ReadInt( minRespawnTime );
	savefile->ReadInt( maxRespawnTime );

	savefile->ReadVec3( firstPersonViewOrigin );
	savefile->ReadMat3( firstPersonViewAxis );

	// don't bother saving dragEntity since it's a dev tool
	dragEntity.Clear();

	savefile->ReadJoint( hipJoint );
	savefile->ReadJoint( chestJoint );
	savefile->ReadJoint( headJoint );

	savefile->ReadStaticObject( physicsObj );
	RestorePhysics( &physicsObj );

	savefile->ReadInt( num );
	aasLocation.SetGranularity( 1 );
	aasLocation.SetNum( num );
	for( i = 0; i < num; i++ ) {
		savefile->ReadInt( aasLocation[ i ].areaNum );
		savefile->ReadVec3( aasLocation[ i ].pos );
	}

	savefile->ReadInt( bobFoot );
	savefile->ReadFloat( bobFrac );
	savefile->ReadFloat( bobfracsin );
	savefile->ReadInt( bobCycle );
	savefile->ReadFloat( xyspeed );
	savefile->ReadInt( stepUpTime );
	savefile->ReadFloat( stepUpDelta );
	savefile->ReadFloat( idealLegsYaw );
	savefile->ReadFloat( legsYaw );
	savefile->ReadBool( legsForward );
	savefile->ReadFloat( oldViewYaw );
	savefile->ReadAngles( viewBobAngles );
	savefile->ReadVec3( viewBob );
	savefile->ReadInt( landChange );
	savefile->ReadInt( landTime );

	savefile->ReadInt( currentWeapon );
	savefile->ReadInt( idealWeapon );
	savefile->ReadInt( previousWeapon );
	savefile->ReadInt( weaponSwitchTime );
	savefile->ReadBool( weaponEnabled );
	savefile->ReadBool( weaponEnabledOnRideExit ); //ff1.3
	savefile->ReadBool( showWeaponViewModel );

	savefile->ReadSkin( skin );
	savefile->ReadSkin( powerUpSkin );
	savefile->ReadString( baseSkinName );
	/*
	savefile->ReadInt( projFired );
	savefile->ReadInt( projHits );

	savefile->ReadBool( airless );
	savefile->ReadInt( airTics );
	savefile->ReadInt( lastAirDamage );
	*/

	savefile->ReadBool( gibDeath );
	savefile->ReadBool( gibsLaunched );
	savefile->ReadVec3( gibsDir );

	//ff1.3 start

	//stats
	savefile->ReadInt( playerStats.killCount );
	savefile->ReadInt( playerStats.soulsCount );
	savefile->ReadInt( playerStats.multikill );
	savefile->ReadInt( playerStats.skillshot );
	savefile->ReadInt( playerStats.doublekill );
	savefile->ReadInt( playerStats.massacre );
	savefile->ReadInt( playerStats.chainkill );
	savefile->ReadInt( playerStats.secretsFound );
	savefile->ReadInt( playerStats.gameCoversFound );
	savefile->ReadInt( playerStats.damage );
	savefile->ReadInt( playerStats.projFired );
	savefile->ReadInt( playerStats.projHits );

	savefile->ReadInt( campaignSoulsCount );
	savefile->ReadInt( currentStatsIndex );

	//medals
	medalsQueue.Clear();
	savefile->ReadInt( num );
	medalsQueue.SetNum( num );
	for( i = 0; i < medalsQueue.Num(); i++ ) {
		savefile->ReadInt( medalsQueue[ i ] );
	}
	savefile->ReadInt( nextMedalTime );
	savefile->ReadBool( medalsEnabled );
	savefile->ReadBool( inGameStatsVisible );
	savefile->ReadBool( inGameStatsCurrentMap );
	savefile->ReadInt( nextInvulnWarningTime );
	//ff1.3 end

#ifdef _DENTONMOD
	// Remember the order of saving this info...
	savefile->Read( &weaponZoom, sizeof( weaponZoom ) );
	LittleBitField( &weaponZoom, sizeof( weaponZoom ) );
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		savefile->ReadByte( projectileType[ i ] );
	}
#endif

	savefile->ReadFloat( set );
	zoomFov.SetStartTime( set );
	savefile->ReadFloat( set );
	zoomFov.SetDuration( set );
	savefile->ReadFloat( set );
	zoomFov.SetStartValue( set );
	savefile->ReadFloat( set );
	zoomFov.SetEndValue( set );

	savefile->ReadFloat( set );
	centerView.SetStartTime( set );
	savefile->ReadFloat( set );
	centerView.SetDuration( set );
	savefile->ReadFloat( set );
	centerView.SetStartValue( set );
	savefile->ReadFloat( set );
	centerView.SetEndValue( set );

	savefile->ReadBool( fxFov );

	savefile->ReadFloat( influenceFov );
	savefile->ReadInt( influenceActive );
	savefile->ReadFloat( influenceRadius );
	savefile->ReadObject( reinterpret_cast<idClass *&>( influenceEntity ) );
	savefile->ReadMaterial( influenceMaterial );
	savefile->ReadSkin( influenceSkin );

	savefile->ReadObject( reinterpret_cast<idClass *&>( privateCameraView ) );

	for( i = 0; i < NUM_LOGGED_VIEW_ANGLES; i++ ) {
		savefile->ReadAngles( loggedViewAngles[ i ] );
	}
	for( i = 0; i < NUM_LOGGED_ACCELS; i++ ) {
		savefile->ReadInt( loggedAccel[ i ].time );
		savefile->ReadVec3( loggedAccel[ i ].dir );
	}
	savefile->ReadInt( currentLoggedAccel );

	savefile->ReadObject( reinterpret_cast<idClass *&>( focusGUIent ) );
	// can't save focusUI
	focusUI = NULL;
	savefile->ReadObject( reinterpret_cast<idClass *&>( focusCharacter ) );
	savefile->ReadInt( talkCursor );
	savefile->ReadInt( focusTime );
	savefile->ReadObject( reinterpret_cast<idClass *&>( focusVehicle ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( focusGameCover ) );
	savefile->ReadUserInterface( cursor );

	//ff1.3 start
	//savefile->ReadObject( reinterpret_cast<idClass *&>( currentVehicle ) ); //moved to actor
	savefile->ReadObject( reinterpret_cast<idClass *&>( currentRiddenAI ) );
	savefile->ReadInt( nextVehicleTime );
	savefile->ReadInt( advancedWeaponZoomTime );
	savefile->ReadBool( advancedWeaponZooming );
	savefile->ReadFloat( quickZoomFov );
	for( i = 0; i < NUM_HIT_COUNT_GROUP_IDS; i++ ) {
		savefile->ReadInt( hitCountGroupIds[ i ] );
	}
	savefile->ReadInt( nextHitCountGroupIndex );
	savefile->ReadInt( lastEscPressedTime );
	savefile->ReadInt( skipCinematicTipStartTime );
	savefile->ReadInt( selectedGameCoverIndex );
	savefile->ReadInt( selectedStatsIndex );
	savefile->ReadInt( maxMapStatsIndex );
	//ff1.3 end

	savefile->ReadInt( oldMouseX );
	savefile->ReadInt( oldMouseY );

	savefile->ReadString( pdaAudio );
	savefile->ReadString( pdaVideo );
	savefile->ReadString( pdaVideoWave );

	savefile->ReadBool( tipUp );
	savefile->ReadBool( objectiveUp );

	savefile->ReadInt( lastDamageDef );
	savefile->ReadVec3( lastDamageDir );
	savefile->ReadInt( lastDamageLocation );
	savefile->ReadInt( smoothedFrame );
	savefile->ReadBool( smoothedOriginUpdated );
	savefile->ReadVec3( smoothedOrigin );
	savefile->ReadAngles( smoothedAngles );

	savefile->ReadBool( ready );
	savefile->ReadBool( respawning );
	savefile->ReadBool( leader );
	savefile->ReadInt( lastSpectateChange );
	savefile->ReadInt( lastTeleFX );

	// set the pm_ cvars
	const idKeyValue	*kv;
	kv = spawnArgs.MatchPrefix( "pm_", NULL );
	while( kv ) {
		cvarSystem->SetCVarString( kv->GetKey(), kv->GetValue() );
		kv = spawnArgs.MatchPrefix( "pm_", kv );
	}

	//savefile->ReadFloat( set );
	//pm_stamina.SetFloat( set );

	// create combat collision hull for exact collision detection
	SetCombatModel();

#ifdef _D3XP
	int weaponToggleCount;
	savefile->ReadInt(weaponToggleCount);
	for(i = 0; i < weaponToggleCount; i++) {
		WeaponToggle_t newToggle;
		memset(&newToggle, 0, sizeof(newToggle));

		idStr name;
		savefile->ReadString(name);
		strcpy(newToggle.name, name.c_str());
		//savefile->ReadInt(newToggle.lastSelected); //ff1.3

		int indexCount;
		savefile->ReadInt(indexCount);
		for(int j = 0; j < indexCount; j++) {
			int temp;
			savefile->ReadInt(temp);
			newToggle.toggleList.Append(temp);
		}
		weaponToggles.Set(newToggle.name, newToggle);
	}
	savefile->ReadObject(reinterpret_cast<idClass *&>(mountedObject));
	enviroSuitLight.Restore( savefile );
	savefile->ReadBool( healthRecharge );
	savefile->ReadInt( lastHealthRechargeTime );
	savefile->ReadInt( rechargeSpeed );
	savefile->ReadFloat( new_g_damageScale );

	savefile->ReadBool( bloomEnabled );
	savefile->ReadFloat( bloomSpeed );
	savefile->ReadFloat( bloomIntensity );
#endif

	// DG: workaround for lingering messages that are shown forever after loading a savegame
	//     (one way to get them is saving again, while the message from first save is still
	//      shown, and then load)
	if ( hud ) {
		hud->SetStateString( "message", "" );
	}
}

/*
===============
idPlayer::PrepareForRestart
================
*/
void idPlayer::PrepareForRestart( void ) {
	ClearPowerUps();
	Spectate( true );
	forceRespawn = true;

#ifdef CTF
	// Confirm reset hud states
	DropFlag();

	if ( hud ) {
		hud->SetStateInt( "red_flagstatus", 0 );
		hud->SetStateInt( "blue_flagstatus", 0 );
	}
#endif

	// we will be restarting program, clear the client entities from program-related things first
	ShutdownThreads();

	// the sound world is going to be cleared, don't keep references to emitters
	FreeSoundEmitter( false );
}

/*
===============
idPlayer::Restart
================
*/
void idPlayer::Restart( void ) {
	idActor::Restart();

	// client needs to setup the animation script object again
	if ( gameLocal.isClient ) {
		Init();
	} else {
		// choose a random spot and prepare the point of view in case player is left spectating
		assert( spectating );
		SpawnFromSpawnSpot();
	}

	useInitialSpawns = true;
	UpdateSkinSetup( true );
}

/*
===============
idPlayer::ServerSpectate
================
*/
void idPlayer::ServerSpectate( bool spectate ) {
	assert( !gameLocal.isClient );

	if ( spectating != spectate ) {
		Spectate( spectate );
		if ( spectate ) {
			SetSpectateOrigin();
		} else {
			if ( gameLocal.gameType == GAME_DM ) {
				// make sure the scores are reset so you can't exploit by spectating and entering the game back
				// other game types don't matter, as you either can't join back, or it's team scores
				gameLocal.mpGame.ClearFrags( entityNumber );
			}
		}
	}
	if ( !spectate ) {
		SpawnFromSpawnSpot();
	}
#ifdef CTF
	// drop the flag if player was carrying it
	if ( spectate && gameLocal.isMultiplayer && gameLocal.mpGame.IsGametypeFlagBased() &&
		 carryingFlag )
	{
		DropFlag();
	}
#endif
}

/*
===========
idPlayer::SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', otherwise
use normal spawn selection.
============
*/
void idPlayer::SelectInitialSpawnPoint( idVec3 &origin, idAngles &angles ) {
	idEntity *spot;
	idStr value;

	spot = gameLocal.SelectInitialSpawnPoint( this );

	// set the player skin from the spawn location
	if ( spot->spawnArgs.GetString( "skin", NULL, value ) ) {
		spawnArgs.Set( "spawn_skin", value );
	}

	//ff1.3 start
	if ( spot->spawnArgs.GetString( "model_player", NULL, value ) ) {
		spawnArgs.Set( "spawn_model", value );
	}

	if ( spot->spawnArgs.GetString( "def_head", NULL, value ) ) {
		spawnArgs.Set( "spawn_def_head", value );
	}

	if ( spot->spawnArgs.GetString( "pda_name_personal", NULL, value ) ) {
		spawnArgs.Set( "pda_name_personal", value );
	}
	//ff1.3 end

	// activate the spawn locations targets
	spot->PostEventMS( &EV_ActivateTargets, 0, this );

	origin = spot->GetPhysics()->GetOrigin();
	origin[2] += 4.0f + CM_BOX_EPSILON;		// move up to make sure the player is at least an epsilon above the floor
	angles = spot->GetPhysics()->GetAxis().ToAngles();
}

/*
===========
idPlayer::SpawnFromSpawnSpot

Chooses a spawn location and spawns the player
============
*/
void idPlayer::SpawnFromSpawnSpot( void ) {
	idVec3		spawn_origin;
	idAngles	spawn_angles;

	SelectInitialSpawnPoint( spawn_origin, spawn_angles );
	SpawnToPoint( spawn_origin, spawn_angles );
}

/*
===========
idPlayer::SpawnToPoint

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState

when called here with spectating set to true, just place yourself and init
============
*/
void idPlayer::SpawnToPoint( const idVec3 &spawn_origin, const idAngles &spawn_angles ) {
	idVec3 spec_origin;

	assert( !gameLocal.isClient );

	respawning = true;

	Init();

	fl.noknockback = false;

	// stop any ragdolls being used
	StopRagdoll();

	// set back the player physics
	SetPhysics( &physicsObj );

	physicsObj.SetClipModelAxis();
	physicsObj.EnableClip();

	if ( !spectating ) {
		SetCombatContents( true );
	}

	physicsObj.SetLinearVelocity( vec3_origin );

	// setup our initial view
	if ( !spectating ) {
		SetOrigin( spawn_origin );
	} else {
		spec_origin = spawn_origin;
		spec_origin[ 2 ] += pm_normalheight.GetFloat();
		spec_origin[ 2 ] += SPECTATE_RAISE;
		SetOrigin( spec_origin );
	}

	// if this is the first spawn of the map, we don't have a usercmd yet,
	// so the delta angles won't be correct.  This will be fixed on the first think.
	viewAngles = ang_zero;
	SetDeltaViewAngles( ang_zero );
	SetViewAngles( spawn_angles );
	spawnAngles = spawn_angles;
	spawnAnglesSet = false;

	legsForward = true;
	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;

	if ( spectating ) {
		Hide();
	} else {
		Show();
	}

	if ( gameLocal.isMultiplayer ) {
		if ( !spectating ) {
			// we may be called twice in a row in some situations. avoid a double fx and 'fly to the roof'
			if ( lastTeleFX < gameLocal.time - 1000 ) {
				idEntityFx::StartFx( spawnArgs.GetString( "fx_spawn" ), &spawn_origin, NULL, this, true );
				lastTeleFX = gameLocal.time;
			}
		}
		AI_TELEPORT = true;
	} else {
		AI_TELEPORT = false;
	}

	// kill anything at the new position
	if ( !spectating ) {
		physicsObj.SetClipMask( MASK_PLAYERSOLID ); // the clip mask is usually maintained in Move(), but KillBox requires it
		gameLocal.KillBox( this );
	}

	// don't allow full run speed for a bit
	physicsObj.SetKnockBack( 100 );

	// set our respawn time and buttons so that if we're killed we don't respawn immediately
	minRespawnTime = gameLocal.time;
	maxRespawnTime = gameLocal.time;
	if ( !spectating ) {
		forceRespawn = false;
	}

	privateCameraView = NULL;

	BecomeActive( TH_THINK );

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	Think();

	respawning			= false;
	lastManOver			= false;
	lastManPlayAgain	= false;
	isTelefragged		= false;
}

/*
===============
idPlayer::SavePersistantInfo

Saves any inventory and player stats when changing levels.
NOTE: also called on savegame. Must not change the state!
===============
*/
void idPlayer::SavePersistantInfo( void ) {
	idDict &playerInfo = gameLocal.persistentPlayerInfo[entityNumber];

	playerInfo.Clear();
	inventory.GetPersistantData( playerInfo );
	playerInfo.SetInt( "health", health );
	playerInfo.SetInt( "current_weapon", currentWeapon );

	//ff1.3 start
	playerInfo.SetInt( "staminaHelltime", staminaHelltime );
	playerInfo.SetInt( "campaignSoulsCount", campaignSoulsCount );

	//add current level stats for campaign maps

	//copy old stats
	const idKeyValue *kv = spawnArgs.MatchPrefix( "stat_", NULL );
	while ( kv ) {
		playerInfo.Set( kv->GetKey().c_str(), kv->GetValue().c_str() );
		//gameLocal.Printf("Copying %s (%s) in PersistantInfo\n", kv->GetKey().c_str(), kv->GetValue().c_str());
		kv = spawnArgs.MatchPrefix( "stat_", kv );
	}

	//store map index for scripts
	playerInfo.SetInt( "prev_map_index", currentStatsIndex ); //also used by scripts

	//for some reason this method is called also when map starts, but we want to save stats only at the end...

	//store stats for scripts and PDA
	if ( !gameLocal.mapInfo.noStats ) {
		//gameLocal.Printf("Saving stat_m%d_* in PersistantInfo\n", currentStatsIndex);
		playerInfo.Set( va("stat_file_%d"				, currentStatsIndex),		gameLocal.GetMapName() );
		playerInfo.SetInt( va("stat_m%d_skill"			, currentStatsIndex),		gameLocal.mapStats.skill );
		playerInfo.SetInt( va("stat_m%d_time"			, currentStatsIndex),		gameLocal.mapStats.time );
		if ( gameLocal.mapStats.skillChanged ) {
			playerInfo.SetBool( va("stat_m%d_skillChanged"	, currentStatsIndex),	gameLocal.mapStats.skillChanged );
		}
		if ( gameLocal.mapStats.numSecrets ) {
			playerInfo.SetInt( va("stat_m%d_numSecrets"		, currentStatsIndex),	gameLocal.mapStats.numSecrets );
		}
		if ( gameLocal.mapStats.numGameCovers ) {
			playerInfo.SetInt( va("stat_m%d_numGameCovers"	, currentStatsIndex),	gameLocal.mapStats.numGameCovers );
		}
		if ( playerStats.killCount ) {
			playerInfo.SetInt( va("stat_m%d_killCount"		, currentStatsIndex),	playerStats.killCount );
		}
		if ( playerStats.soulsCount ) {
			playerInfo.SetInt( va("stat_m%d_souls"			, currentStatsIndex),	playerStats.soulsCount );
		}
		if ( playerStats.multikill ) {
			playerInfo.SetInt( va("stat_m%d_multikill"		, currentStatsIndex),	playerStats.multikill );
		}
		if ( playerStats.skillshot ) {
			playerInfo.SetInt( va("stat_m%d_skillshot"		, currentStatsIndex),	playerStats.skillshot );
		}
		if ( playerStats.doublekill ) {
			playerInfo.SetInt( va("stat_m%d_doublekill"		, currentStatsIndex),	playerStats.doublekill );
		}
		if ( playerStats.massacre ) {
			playerInfo.SetInt( va("stat_m%d_massacre"		, currentStatsIndex),	playerStats.massacre );
		}
		if ( playerStats.chainkill ) {
			playerInfo.SetInt( va("stat_m%d_chainkill"		, currentStatsIndex),	playerStats.chainkill );
		}
		if ( playerStats.secretsFound ) {
			playerInfo.SetInt( va("stat_m%d_secrets"		, currentStatsIndex),	playerStats.secretsFound );
		}
		if ( playerStats.gameCoversFound ) {
			playerInfo.SetInt( va("stat_m%d_gameCovers"		, currentStatsIndex),	playerStats.gameCoversFound );
		}
		if ( playerStats.damage ) {
			playerInfo.SetInt( va("stat_m%d_damage"			, currentStatsIndex),	playerStats.damage );
		}
		if ( playerStats.projFired ) {
			playerInfo.SetInt( va("stat_m%d_projFired"		, currentStatsIndex),	playerStats.projFired );
		}
		if ( playerStats.projHits ) {
			playerInfo.SetInt( va("stat_m%d_projHits"		, currentStatsIndex),	playerStats.projHits );
		}
		//playerInfo.SetInt( "currentStatsIndex", currentStatsIndex ); //store it only if stats are also saved
	}

	//gameLocal.Printf("SavePersistantInfo\n");
	//playerInfo.Print();

	//ff1.3 end
}

/*
===============
idPlayer::RestorePersistantInfo

Restores any inventory and player stats when changing levels.
===============
*/
void idPlayer::RestorePersistantInfo( void ) {
	if ( gameLocal.isMultiplayer ) {
		gameLocal.persistentPlayerInfo[entityNumber].Clear();
	}

	spawnArgs.Copy( gameLocal.persistentPlayerInfo[entityNumber] );

	inventory.RestoreInventory( this, spawnArgs );
	health = spawnArgs.GetInt( "health", "100" );
	if ( !gameLocal.isClient ) {
		idealWeapon = spawnArgs.GetInt( "current_weapon", "0" ); //ff1.3 - was 0
	}

	//ff1.3 start
	staminaHelltime = spawnArgs.GetInt( "staminaHelltime" );
	campaignSoulsCount = spawnArgs.GetInt( "campaignSoulsCount" );

	//find index for current map
	if ( gameLocal.mapInfo.noStats ) {
		currentStatsIndex = -1;
	} else if ( !FindPersistedStatsIndex( gameLocal.GetMapName(), currentStatsIndex ) ) {
		currentStatsIndex = FirstFreeStatsIndex();
	}

	//NOTE: if first map is noStats max index will be -1
	maxMapStatsIndex = Max( currentStatsIndex, spawnArgs.GetInt( "prev_map_index", "-1" ) );

	//select map in PDA
	if ( gameLocal.mapInfo.noStats ) {
		selectedStatsIndex = CampaignStatsIndex(); //set selection to campaign
	} else {
		selectedStatsIndex = currentStatsIndex; //set selection to current map
	}

	/*
	gameLocal.Printf("**********\ncurrentStatsIndex: %d\nselectedStatsIndex: %d\nmaxMapStatsIndex: %d\n**********\n",
		currentStatsIndex, selectedStatsIndex, maxMapStatsIndex);
	*/
	//ff1.3 end
}

/*
================
idPlayer::FindPersistedStatsIndex
================
*/
bool idPlayer::FindPersistedStatsIndex( const char *mapName, int &statsIndex ) {
	const idKeyValue *kv = spawnArgs.MatchPrefix( "stat_file_", NULL );
	while ( kv ) {
		if ( !idStr::Icmp( kv->GetValue().c_str(), mapName ) ) {
			idStr key = kv->GetKey().c_str();
			if( key.StripLeadingOnce( "stat_file_" ) ) {
				statsIndex = atoi( key );
				return true;
			}
		}
		kv = spawnArgs.MatchPrefix( "stat_file_", kv );
	}
	return false;
}

/*
================
idPlayer::FirstFreeStatsIndex
================
*/
int idPlayer::FirstFreeStatsIndex( void ) {
	int statsIndex = 0;
	while ( spawnArgs.FindKey( va("stat_file_%d", statsIndex) ) != NULL ) {
		statsIndex++;
	}
	return statsIndex;
}

/*
================
idPlayer::CampaignStatsIndex
================
*/
int idPlayer::CampaignStatsIndex( void ) {
	return maxMapStatsIndex + (( gameLocal.campaignInfo.maps > 1 ) ? 1 : 0);
}


/*
================
idPlayer::LoadStatsForStatsIndex
================
*/
void idPlayer::LoadStatsForStatsIndex( int statsIndex, playerStats_t &playerStats, mapStats_t &mapStats ) {
	playerStats.killCount		= spawnArgs.GetInt( va("stat_m%d_killCount", statsIndex) );
	playerStats.soulsCount		= spawnArgs.GetInt( va("stat_m%d_souls", statsIndex) );
	playerStats.multikill		= spawnArgs.GetInt( va("stat_m%d_multikill", statsIndex) );
	playerStats.skillshot		= spawnArgs.GetInt( va("stat_m%d_skillshot", statsIndex) );
	playerStats.doublekill		= spawnArgs.GetInt( va("stat_m%d_doublekill", statsIndex) );
	playerStats.massacre		= spawnArgs.GetInt( va("stat_m%d_massacre", statsIndex) );
	playerStats.chainkill		= spawnArgs.GetInt( va("stat_m%d_chainkill", statsIndex) );
	playerStats.secretsFound	= spawnArgs.GetInt( va("stat_m%d_secrets", statsIndex) );
	playerStats.gameCoversFound	= spawnArgs.GetInt( va("stat_m%d_gameCovers", statsIndex) );
	playerStats.damage			= spawnArgs.GetInt( va("stat_m%d_damage", statsIndex) );
	playerStats.projFired		= spawnArgs.GetInt( va("stat_m%d_projFired", statsIndex) );
	playerStats.projHits		= spawnArgs.GetInt( va("stat_m%d_projHits", statsIndex) );
	mapStats.time				= spawnArgs.GetInt( va("stat_m%d_time", statsIndex) );
	mapStats.skill				= spawnArgs.GetInt( va("stat_m%d_skill", statsIndex) );
	mapStats.skillChanged		= spawnArgs.GetBool( va("stat_m%d_skillChanged", statsIndex) );
	mapStats.numSecrets			= spawnArgs.GetInt( va("stat_m%d_numSecrets", statsIndex) );
	mapStats.numGameCovers		= spawnArgs.GetInt( va("stat_m%d_numGameCovers", statsIndex) );
}

/*
================
idPlayer::LoadMapInfoForStatsIndex
================
*/
void idPlayer::LoadMapInfoForStatsIndex( int statsIndex, mapInfo_t &mapInfo ) {
	gameLocal.LoadMapInfo( spawnArgs.GetString( va("stat_file_%d", statsIndex) ), mapInfo );
}

/*
================
idPlayer::CalculateCampaignStats
================
*/
void idPlayer::CalculateCampaignStats( playerStats_t &campaignPlayerStats, mapStats_t &campaignMapStats ) {
	bool extraSecretFound;
	playerStats_t tempPlayerStats;
	mapStats_t tempMapStats;

	memset( &campaignPlayerStats, 0, sizeof( campaignPlayerStats ) );
	memset( &campaignMapStats, 0, sizeof( campaignMapStats ) );
	campaignMapStats.skill = g_skill.GetInteger();

	for ( int i = 0; i <= maxMapStatsIndex; i ++ ) {
		if ( i == currentStatsIndex ) {
			tempPlayerStats = playerStats;
			tempMapStats = gameLocal.mapStats;
			if ( !tempMapStats.time ) { //map stats not yet "committed"
				tempMapStats.time = gameLocal.GetTimeStat();
			}
		} else {
			LoadStatsForStatsIndex( i, tempPlayerStats, tempMapStats );
		}
		campaignPlayerStats.killCount		+= tempPlayerStats.killCount;
		campaignPlayerStats.soulsCount		+= tempPlayerStats.soulsCount;
		campaignPlayerStats.multikill		+= tempPlayerStats.multikill;
		campaignPlayerStats.skillshot		+= tempPlayerStats.skillshot;
		campaignPlayerStats.doublekill		+= tempPlayerStats.doublekill;
		campaignPlayerStats.massacre		+= tempPlayerStats.massacre;
		campaignPlayerStats.chainkill		+= tempPlayerStats.chainkill;
		campaignPlayerStats.secretsFound	+= tempPlayerStats.secretsFound;
		campaignPlayerStats.gameCoversFound	+= tempPlayerStats.gameCoversFound;
		campaignPlayerStats.damage			+= tempPlayerStats.damage;
		campaignPlayerStats.projFired		+= tempPlayerStats.projFired;
		campaignPlayerStats.projHits		+= tempPlayerStats.projHits;
		campaignMapStats.time				+= tempMapStats.time;
		campaignMapStats.numSecrets			+= tempMapStats.numSecrets;
		campaignMapStats.numGameCovers		+= tempMapStats.numGameCovers;

		if ( tempMapStats.skillChanged || campaignMapStats.skill != tempMapStats.skill ) {
			campaignMapStats.skillChanged = true;
		}
	}

	if ( spawnArgs.GetBool( "stat_extra_secret", "0", extraSecretFound ) ){
		campaignMapStats.numSecrets++; //the secret exists if the key has been set
		if ( extraSecretFound ) {
			campaignPlayerStats.secretsFound++;
		}
	}
}


/*
================
idPlayer::Event_ShowStats
================
*/
void idPlayer::Event_ShowStats( const char *mapName, int showEvent ) {
	int statsIndex;

	if ( !infoGui || inGameStatsVisible ) {
		idThread::ReturnInt( -1 );
		return;
	}

	if ( !idStr::Cmp( mapName, "campaign" ) ) { //campaign
		playerStats_t campaignPlayerStats;
		mapStats_t campaignStats;

		statsIndex = CampaignStatsIndex();
		if ( showEvent && !gameLocal.mapStats.time ) { //"commit" map time if needed
			gameLocal.mapStats.time = gameLocal.GetTimeStat();
		}
		CalculateCampaignStats( campaignPlayerStats, campaignStats );
		DisplayCampaignStats( infoGui, campaignPlayerStats, campaignStats, gameLocal.campaignInfo, true );
		//gameLocal.PersistCampaignStats( campaignPlayerStats, campaignStats );
		inGameStatsCurrentMap = false;
		if ( showEvent ) {
			inGameStatsVisible = true;
			infoGui->HandleNamedEvent( "showMapEnding" );
		}
	} else if ( !idStr::Cmp( mapName, "map" ) || !idStr::Cmp( mapName, gameLocal.GetMapName() ) ) { //current map
		statsIndex = currentStatsIndex;
		if ( showEvent && !gameLocal.mapStats.time ) { //"commit" map time if needed
			gameLocal.mapStats.time = gameLocal.GetTimeStat();
		}
		DisplayMapStats( infoGui, playerStats, gameLocal.mapStats, gameLocal.mapInfo, true );
		inGameStatsCurrentMap = true;
		if ( showEvent ) {
			inGameStatsVisible = true;
			infoGui->HandleNamedEvent( "showMapEnding" );
		}
	} else if ( FindPersistedStatsIndex( mapName, statsIndex ) ) { //completed maps
		mapInfo_t completedMapInfo;
		playerStats_t completedPlayerStats;
		mapStats_t completedMapStats;

		LoadStatsForStatsIndex( statsIndex, completedPlayerStats, completedMapStats );
		LoadMapInfoForStatsIndex( statsIndex, completedMapInfo );
		DisplayMapStats( infoGui, completedPlayerStats, completedMapStats, completedMapInfo, true );
		inGameStatsCurrentMap = false;
		if ( showEvent ) {
			inGameStatsVisible = true;
			infoGui->HandleNamedEvent( "showMapSummary" );
		}
	} else {
		mapInfo_t newMapInfo;
		playerStats_t newPlayerStats;
		mapStats_t newMapStats;

		memset( &newPlayerStats, 0, sizeof( newPlayerStats ) );
		memset( &newMapStats, 0, sizeof( newMapStats ) );
		gameLocal.LoadMapInfo( mapName, newMapInfo );

		DisplayMapStats( infoGui, newPlayerStats, newMapStats, newMapInfo, false );
		statsIndex = -1; //not yet completed
		inGameStatsCurrentMap = false;
		if ( showEvent ) {
			inGameStatsVisible = true;
			infoGui->HandleNamedEvent( "showMapPreview" );
		}
	}

	idThread::ReturnInt( statsIndex );
}

/*
================
idPlayer::Event_HideStats
================
*/
void idPlayer::Event_HideStats( void ) {
	if ( infoGui && inGameStatsVisible ) {
		inGameStatsVisible = false;
		inGameStatsCurrentMap = false;
		infoGui->HandleNamedEvent( "hideMapInfo" );

		//reset time so it'll be updated again, but not during final cinematic
		if ( !gameLocal.inCinematic ) {
			gameLocal.mapStats.time = 0;
		}
	}
}

/*
================
idPlayer::Event_GetAmmoForWeapon
================
*/
void idPlayer::Event_GetAmmoForWeapon( const char *weaponClassName ) {
	idThread::ReturnInt( inventory.HasAmmo( weaponClassName, true, this ) );
}

/*
================
idPlayer::GetUserInfo
================
*/
idDict *idPlayer::GetUserInfo( void ) {
	return &gameLocal.userInfo[ entityNumber ];
}

/*
==============
idPlayer::UpdateSkinSetup
==============
*/
void idPlayer::UpdateSkinSetup( bool restart ) {
	if ( restart ) {
		team = ( idStr::Icmp( GetUserInfo()->GetString( "ui_team" ), "Blue" ) == 0 );
	}
	if ( gameLocal.mpGame.IsGametypeTeamBased() ) { /* CTF */
		if ( team ) {
			baseSkinName = "skins/characters/player/marine_mp_blue";
		} else {
			baseSkinName = "skins/characters/player/marine_mp_red";
		}
		if ( !gameLocal.isClient && team != latchedTeam ) {
			gameLocal.mpGame.SwitchToTeam( entityNumber, latchedTeam, team );
		}
		latchedTeam = team;
	} else {
		baseSkinName = GetUserInfo()->GetString( "ui_skin" );
	}
	if ( !baseSkinName.Length() ) {
		baseSkinName = "skins/characters/player/marine_mp";
	}
	skin = declManager->FindSkin( baseSkinName, false );
	assert( skin );
	// match the skin to a color band for scoreboard
	if ( baseSkinName.Find( "red" ) != -1 ) {
		colorBarIndex = 1;
	} else if ( baseSkinName.Find( "green" ) != -1 ) {
		colorBarIndex = 2;
	} else if ( baseSkinName.Find( "blue" ) != -1 ) {
		colorBarIndex = 3;
	} else if ( baseSkinName.Find( "yellow" ) != -1 ) {
		colorBarIndex = 4;
	} else if ( baseSkinName.Find( "grey" ) != -1 ) {
		colorBarIndex = 5;
	} else if ( baseSkinName.Find( "purple" ) != -1 ) {
		colorBarIndex = 6;
	} else if ( baseSkinName.Find( "orange" ) != -1 ) {
		colorBarIndex = 7;
	} else {
		colorBarIndex = 0;
	}
	colorBar = colorBarTable[ colorBarIndex ];
	if ( PowerUpActive( BERSERK ) ) {
		powerUpSkin = declManager->FindSkin( baseSkinName + "_berserk" );
	}
#ifdef _D3XP
	else if ( PowerUpActive( INVULNERABILITY ) ) {
		powerUpSkin = declManager->FindSkin( baseSkinName + "_invuln" );
	//} else if ( PowerUpActive( HASTE ) ) {
	//	powerUpSkin = declManager->FindSkin( baseSkinName + "_haste" );
	}
#endif
}

/*
==============
idPlayer::BalanceTDM
==============
*/
bool idPlayer::BalanceTDM( void ) {
	int			i, balanceTeam, teamCount[2];
	idEntity	*ent;

	teamCount[ 0 ] = teamCount[ 1 ] = 0;
	for( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPlayer::Type ) ) {
			teamCount[ static_cast< idPlayer * >( ent )->team ]++;
		}
	}
	balanceTeam = -1;
	if ( teamCount[ 0 ] < teamCount[ 1 ] ) {
		balanceTeam = 0;
	} else if ( teamCount[ 0 ] > teamCount[ 1 ] ) {
		balanceTeam = 1;
	}
	if ( balanceTeam != -1 && team != balanceTeam ) {
		common->DPrintf( "team balance: forcing player %d to %s team\n", entityNumber, balanceTeam ? "blue" : "red" );
		team = balanceTeam;
		GetUserInfo()->Set( "ui_team", team ? "Blue" : "Red" );
		return true;
	}
	return false;
}

/*
==============
idPlayer::UserInfoChanged
==============
*/
bool idPlayer::UserInfoChanged( bool canModify ) {
	idDict	*userInfo;
	bool	modifiedInfo;
	bool	spec;
	bool	newready;

	userInfo = GetUserInfo();
	showWeaponViewModel = userInfo->GetBool( "ui_showGun" );

	if ( !gameLocal.isMultiplayer ) {
		return false;
	}

	modifiedInfo = false;

	spec = ( idStr::Icmp( userInfo->GetString( "ui_spectate" ), "Spectate" ) == 0 );
	if ( gameLocal.serverInfo.GetBool( "si_spectators" ) ) {
		// never let spectators go back to game while sudden death is on
		if ( canModify && gameLocal.mpGame.GetGameState() == idMultiplayerGame::SUDDENDEATH && !spec && wantSpectate == true ) {
			userInfo->Set( "ui_spectate", "Spectate" );
			modifiedInfo |= true;
		} else {
			if ( spec != wantSpectate && !spec ) {
				// returning from spectate, set forceRespawn so we don't get stuck in spectate forever
				forceRespawn = true;
			}
			wantSpectate = spec;
		}
	} else {
		if ( canModify && spec ) {
			userInfo->Set( "ui_spectate", "Play" );
			modifiedInfo |= true;
		} else if ( spectating ) {
			// allow player to leaving spectator mode if they were in it when si_spectators got turned off
			forceRespawn = true;
		}
		wantSpectate = false;
	}

	newready = ( idStr::Icmp( userInfo->GetString( "ui_ready" ), "Ready" ) == 0 );
	if ( ready != newready && gameLocal.mpGame.GetGameState() == idMultiplayerGame::WARMUP && !wantSpectate ) {
		gameLocal.mpGame.AddChatLine( common->GetLanguageDict()->GetString( "#str_07180" ), userInfo->GetString( "ui_name" ), newready ? common->GetLanguageDict()->GetString( "#str_04300" ) : common->GetLanguageDict()->GetString( "#str_04301" ) );
	}
	ready = newready;
	team = ( idStr::Icmp( userInfo->GetString( "ui_team" ), "Blue" ) == 0 );
	// server maintains TDM balance
	if ( canModify && gameLocal.mpGame.IsGametypeTeamBased() && !gameLocal.mpGame.IsInGame( entityNumber ) && g_balanceTDM.GetBool() ) { /* CTF */
		modifiedInfo |= BalanceTDM( );
	}
	UpdateSkinSetup( false );

	isChatting = userInfo->GetBool( "ui_chat", "0" );
	if ( canModify && isChatting && AI_DEAD ) {
		// if dead, always force chat icon off.
		isChatting = false;
		userInfo->SetBool( "ui_chat", false );
		modifiedInfo |= true;
	}

	return modifiedInfo;
}

/*
===============
idPlayer::UpdateHudAmmo
===============
*/
void idPlayer::UpdateHudAmmo( idUserInterface *_hud ) {
	int inclip;
	int ammoamount;
	int clipSize;
	int pwAmmo;
	int i;

	assert( weapon.GetEntity() );
	assert( _hud );

	inclip		= weapon.GetEntity()->AmmoInClip();
	ammoamount	= weapon.GetEntity()->AmmoAvailable();
	clipSize	= weapon.GetEntity()->ClipSize();
	pwAmmo		= weapon.GetEntity()->PwAmmoAvailable(); //GetPwAmmoCurrentWeapon();

	if ( (!pwAmmo && ammoamount < 0 ) || !weapon.GetEntity()->IsReady() ) {
		// show infinite ammo
		_hud->SetStateString( "player_allammo", "" );
		_hud->SetStateString( "weapicon","" );
		_hud->SetStateBool( "show_skull_ammo", false );
	} else if( currentWeapon == weapon_skull ){ //fix for skulls weapon
		pwAmmo = 0; //use 'pwAmmo' var for special skull ammo
		for( i = 0; i < MAX_SKULL_MODES; i++ ) {
			_hud->SetStateInt( va( "special_skull_ammo%d",i ), inventory.skullSpecialAmmo[i] );
			pwAmmo += inventory.skullSpecialAmmo[i];
		}
		inclip =  ammoamount - pwAmmo;
		_hud->SetStateInt( "default_skull_ammo",  inclip );
		_hud->SetStateString( "player_allammo", va( "%i", inclip) );
		_hud->SetStateString( "weapicon", weapon.GetEntity()->Icon() );
		_hud->SetStateBool( "show_skull_ammo", true );
	} else if ( pwAmmo || (currentWeapon == weapon_shockrifle && painKillerProjectile.GetEntity() ) ){
		_hud->SetStateString( "player_allammo", va( "%i", pwAmmo ) );
		_hud->SetStateString( "weapicon", weapon.GetEntity()->IconPw() );
		_hud->SetStateBool( "show_skull_ammo", false );
	} else {
		_hud->SetStateString( "player_allammo", clipSize ? va( "%i / %i", inclip, ammoamount ) :  va( "%i", ammoamount ) );
		_hud->SetStateString( "weapicon", weapon.GetEntity()->Icon() );
		_hud->SetStateBool( "show_skull_ammo", false );
	}

	//ff1.3

	/* ff1.3 - removed:
	_hud->SetStateBool( "player_ammo_empty", ( ammoamount == 0 ) );
	_hud->SetStateBool( "player_clip_empty", ( clipSize ? inclip == 0 : false ) );
	_hud->SetStateBool( "player_clip_low", ( clipSize ? inclip <= weapon.GetEntity()->LowAmmo() : false ) );
	*/

	/*
#ifdef _D3XP
	//Hack to stop the bloodstone ammo to display when it is being activated
	if(currentWeapon == weapon_bloodstone) {
		_hud->SetStateBool( "player_ammo_empty", false );
		_hud->SetStateBool( "player_clip_empty", false );
		_hud->SetStateBool( "player_clip_low", false );
	}
#endif


#ifdef _D3XP
	//Let the HUD know the total amount of ammo regardless of the ammo required value
	_hud->SetStateString( "player_ammo_count", va("%i", weapon.GetEntity()->AmmoCount()));
#endif
*/

	/*
#ifdef _D3XP
	//Make sure the hud always knows how many bloodstone charges there are
	int ammoRequired;
	ammo_t ammo_i = inventory.AmmoIndexForWeaponClass( "weapon_bloodstone_passive", &ammoRequired );
	int bloodstoneAmmo = inventory.HasAmmo( ammo_i, ammoRequired );
	_hud->SetStateString("player_bloodstone_ammo", va("%i", bloodstoneAmmo));
	_hud->HandleNamedEvent( "bloodstoneAmmoUpdate" );
#endif
	*/

	_hud->HandleNamedEvent( "updateAmmo" );
}

/*
===============
idPlayer::UpdateHudStats
===============
*/

#define HEALTH_ADDER_SHOW_TIME	 200	//must match gui time

void idPlayer::UpdateHudStats( idUserInterface *_hud ) {
	/* ff1.3
	int staminapercentage;
	float max_stamina;
	*/

	assert( _hud );

	/* ff1.3
	max_stamina = pm_stamina.GetFloat();
	if ( !max_stamina ) {
		// stamina disabled, so show full stamina bar
		staminapercentage = 100.0f;
	} else {
		staminapercentage = idMath::FtoiFast( 100.0f * stamina / max_stamina );
	}
	*/

	//medals moved to own method...

	if ( currentVehicle ) {
		currentVehicle->UpdateHudStats( _hud );
	} else if ( currentRiddenAI ) {
		currentRiddenAI->UpdateHudStats( _hud );
	}

	//ff1.3 start
	if ( advancedWeaponZooming && zoomFov.GetStartValue() - zoomFov.GetEndValue() > 0.0f ) {
		_hud->SetStateInt( "player_weapzoom", 100.0f * (zoomFov.GetStartValue() - zoomFov.GetCurrentValue(advancedWeaponZoomTime ? advancedWeaponZoomTime : gameLocal.time))/(zoomFov.GetStartValue() - zoomFov.GetEndValue()) );
	} else {
		_hud->SetStateInt( "player_weapzoom", 0.0f);
	}
	//ff1.3 end

	_hud->SetStateInt( "player_health", health );
	//_hud->SetStateInt( "player_stamina", staminapercentage );
	_hud->SetStateInt( "player_stamina", staminaHelltime );
	_hud->SetStateInt("player_helltime_enabled", staminaEnabled ? 1 : 0 );

	_hud->SetStateInt( "player_armor", inventory.armor );
	_hud->SetStateInt( "player_hr", heartRate );

	//_hud->SetStateInt( "player_nostamina", ( max_stamina == 0 ) ? 1 : 0 );

	_hud->HandleNamedEvent( "updateArmorHealthAir" );

#ifdef _D3XP
	_hud->HandleNamedEvent( "updatePowerup" );
#endif

	if ( hudPulseFlags ) {
		if( hudPulseFlags & HUD_PULSE_SOUL ){
			_hud->SetStateInt( "campaign_souls", campaignSoulsCount );
			_hud->HandleNamedEvent( "soulPulse" );
		}

		if( hudPulseFlags & HUD_PULSE_HEALTH_ADDER ){
			if( nextHealthAddedReset < gameLocal.time ){ //too much time elapsed
				_hud->SetStateString("health_added", va( "+%d", healthAdded ) );
				_hud->HandleNamedEvent( "add_new_energy" );
			} else {
				healthAdded += _hud->GetStateInt( "health_added" );
				_hud->SetStateString("health_added", va( "+%d", healthAdded ) );
				_hud->HandleNamedEvent( "upd_last_energy" );
			}
			healthAdded = 0;
			nextHealthAddedReset = gameLocal.time + HEALTH_ADDER_SHOW_TIME;
		}

		if( hudPulseFlags & HUD_PULSE_HEALTH_ICON ){
			_hud->HandleNamedEvent( "extra_energy" );
		}

		hudPulseFlags = 0;
	}

	if ( healthPulse ) {
		_hud->HandleNamedEvent( "healthPulse" );
		StartSound( "snd_healthpulse", SND_CHANNEL_ITEM, 0, false, NULL );
		healthPulse = false;
	}

	/*
	if ( healthTake ) {
		_hud->HandleNamedEvent( "healthPulse" );
		StartSound( "snd_healthtake", SND_CHANNEL_ITEM, 0, false, NULL );
		healthTake = false;
	}
	*/

	if ( inventory.ammoPulse ) {
		_hud->HandleNamedEvent( "ammoPulse" );
		inventory.ammoPulse = false;
		UpdateEveryAmmoOnHud(); //ff1.1
	}
	if ( inventory.weaponPulse ) {
		// We need to update the weapon hud manually, but not
		// the armor/ammo/health because they are updated every
		// frame no matter what
		UpdateHudWeapon();
		_hud->HandleNamedEvent( "weaponPulse" );
		inventory.weaponPulse = false;
	}
	if ( inventory.armorPulse ) {
		_hud->HandleNamedEvent( "armorPulse" );
		inventory.armorPulse = false;
	}

#ifdef CTF
	if ( gameLocal.mpGame.IsGametypeFlagBased() && _hud )
	{
		_hud->SetStateInt( "red_flagstatus", gameLocal.mpGame.GetFlagStatus( 0 ) );
		_hud->SetStateInt( "blue_flagstatus", gameLocal.mpGame.GetFlagStatus( 1 ) );

		_hud->SetStateInt( "red_team_score",  gameLocal.mpGame.GetFlagPoints( 0 ) );
		_hud->SetStateInt( "blue_team_score", gameLocal.mpGame.GetFlagPoints( 1 ) );

		_hud->HandleNamedEvent( "RedFlagStatusChange" );
		_hud->HandleNamedEvent( "BlueFlagStatusChange" );
	}

	//ff1.3 line removed - was:  _hud->HandleNamedEvent( "selfTeam" );

#endif

    UpdateHudAmmo( _hud );
}
//ff1.3 start

/*
===============
idPlayer::DisplayCampaignStats
===============
*/
void idPlayer::DisplayCampaignStats( idUserInterface *_gui, playerStats_t &campaignPlayerStats, mapStats_t &campaignStats, campaignInfo_t &campaignInfo, bool completed ) {
	DisplayStats( _gui, campaignPlayerStats, campaignStats, "Campaign Totals", campaignInfo.img, completed, completed ); //show totals only when completed
}


/*
===============
idPlayer::DisplayMapStats
===============
*/
void idPlayer::DisplayMapStats( idUserInterface *_gui, playerStats_t &playerStats, mapStats_t &mapStats, mapInfo_t &mapInfo, bool completed ) {
	DisplayStats( _gui, playerStats, mapStats, mapInfo.title.c_str(), mapInfo.img.c_str(), completed, true );
}

/*
===============
idPlayer::DisplayStats
===============
*/
void idPlayer::DisplayStats( idUserInterface *_gui, playerStats_t &playerStats, mapStats_t &mapStats, const char *statsTitle, const char *statsImg, bool completed, bool showTotals ) {
	const char *skill;
	int time;

	if ( mapStats.time ) {
		time = mapStats.time;
	} else { //map time not yet "committed"
		time = gameLocal.GetTimeStat();
	}

	_gui->SetStateInt( "stat_souls",			playerStats.soulsCount );
	_gui->SetStateInt( "stat_killCount",		playerStats.killCount );
	_gui->SetStateInt( "stat_doublekill",		playerStats.doublekill );
	_gui->SetStateInt( "stat_multikill",		playerStats.multikill );
	_gui->SetStateInt( "stat_skillshot",		playerStats.skillshot );
	_gui->SetStateInt( "stat_chainkill",		playerStats.chainkill );
	_gui->SetStateInt( "stat_massacre",			playerStats.massacre );

	if ( playerStats.projFired ) {
		_gui->SetStateString( "stat_accuracy", va( "%.1f%%", ( (float) playerStats.projHits / playerStats.projFired ) * 100 ) );
	} else {
		_gui->SetStateString( "stat_accuracy", "0.0%" );
	}

	if ( showTotals ) {
		_gui->SetStateString( "stat_secrets", va( "%d / %d", playerStats.secretsFound, mapStats.numSecrets ) );
		_gui->SetStateString( "stat_gameCovers", va( "%d / %d", playerStats.gameCoversFound, mapStats.numGameCovers ) );
	} else {
		_gui->SetStateString( "stat_secrets", va( "%d", playerStats.secretsFound ) );
		_gui->SetStateString( "stat_gameCovers", va( "%d", playerStats.gameCoversFound ) );
	}

	_gui->SetStateInt( "stat_damage", playerStats.damage );

	switch( mapStats.skill ) {
		case 0: {
			skill = "Rookie";
			break;
		}
		case 1: {
			skill = "Veteran";
			break;
		}
		case 2: {
			skill = "Pro";
			break;
		}
		case 3: {
			skill = "Master";
			break;
		}
		default: {
			skill =  "Unknown";
		}
	}
	_gui->SetStateString( "stat_skill", mapStats.skillChanged ? va( "%s (changed)", skill ) : skill );
	_gui->SetStateBool( "stat_completed", completed );
	_gui->SetStateString( "stat_title", statsTitle );
	_gui->SetStateString( "stat_img", statsImg );
	_gui->SetStateString( "stat_time", va( "%d:%02d:%02d", time / (3600 * 1000), (time / (60 * 1000)) % 60, (time / 1000) % 60 ));
}

/*
===============
idPlayer::UpdatePDAStats
===============
*/
void idPlayer::UpdatePDAStats( idUserInterface *_gui ) {
	if ( selectedStatsIndex > maxMapStatsIndex ) { //campaign
		playerStats_t campaignPlayerStats;
		mapStats_t campaignStats;
		CalculateCampaignStats( campaignPlayerStats, campaignStats );
		DisplayCampaignStats( _gui, campaignPlayerStats, campaignStats, gameLocal.campaignInfo, false );
	} else if ( selectedStatsIndex == currentStatsIndex ) { //current map
		DisplayMapStats( _gui, playerStats, gameLocal.mapStats, gameLocal.mapInfo, false );
	} else { //completed maps
		mapInfo_t completedMapInfo;
		playerStats_t completedPlayerStats;
		mapStats_t completedMapStats;
		LoadStatsForStatsIndex( selectedStatsIndex, completedPlayerStats, completedMapStats );
		LoadMapInfoForStatsIndex( selectedStatsIndex, completedMapInfo );
		DisplayMapStats( _gui, completedPlayerStats, completedMapStats, completedMapInfo, true );
	}

	_gui->SetStateBool( "has_prev_stats", (selectedStatsIndex > 0) );
	_gui->SetStateBool( "has_next_stats", (selectedStatsIndex < CampaignStatsIndex()) ); //reserve 1 for Campaign stats

	//UpdateChallanges( _gui, playerStats, mapStats, (selectedStatsIndex < currentStatsIndex) );
}



/*
===============
idPlayer::UpdateChallanges
===============

void idPlayer::UpdateChallanges( idUserInterface *_gui, playerStats_t &playerStats, mapStats_t &mapStats, bool failedIfNotCompleted ) {
	_gui->SetStateInt( "chal_damage", ( playerStats.damage == 0 ) ? 1 : ( failedIfNotCompleted ? -1 : 0 ) );
	_gui->SetStateInt( "chal_secrets", ( playerStats.secretsFound == mapStats.numSecrets ) ? 1 : ( failedIfNotCompleted ? -1 : 0 ) );
	_gui->SetStateInt( "chal_gameCovers", ( playerStats.gameCoversFound == mapStats.numGameCovers ) ? 1 : ( failedIfNotCompleted ? -1 : 0 ) );
}
*/

//ff1.3 end

/*
===============
idPlayer::UpdateHudWeapon
===============
*/
void idPlayer::UpdateHudWeapon( bool flashWeapon ) {
	idUserInterface *hud = idPlayer::hud;

	// if updating the hud of a followed client
	if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
		idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
		if ( p->spectating && p->spectator == entityNumber ) {
			assert( p->hud );
			hud = p->hud;
		}
	}

	if ( !hud ) {
		return;
	}

	int lastWeaponSlotIndex = -1; //ff1.3
	for ( int i = 0; i < MAX_WEAPONS; i++ ) {
		const char *weapnum = va( "def_weapon%d", i );
		const char *hudWeap = va( "weapon%d", i );
		int weapstate = 0;
		if ( inventory.weapons & ( 1 << i ) ) {
			const char *weap = spawnArgs.GetString( weapnum );
			if ( weap && *weap ) {
				weapstate++;
			}
			if ( idealWeapon == i ) {
				weapstate++;
			}
		}
		hud->SetStateInt( hudWeap, weapstate );

		//ff1.3 start
		const char *hudWeapSlot = va( "slot_weapon%d", i );
		int weaponSlotIndex;
		if (weapstate > 0 && i != weapon_fists ) {
			weaponSlotIndex = ++lastWeaponSlotIndex;
		} else {
			weaponSlotIndex = -1;
		}
		hud->SetStateInt( hudWeapSlot, weaponSlotIndex );
		//ff1.3 end
	}

	//ff1.3 start
	hud->SetStateInt( "weapon_last_slot", lastWeaponSlotIndex );
	hud->SetStateInt( "skull_mode", inventory.skullMode );
	//ff1.3 end

	if ( flashWeapon ) {

/*#ifdef _D3XP
		//Clear all hud weapon varaibles for the weapon change
		hud->SetStateString( "player_ammo", "" );
		hud->SetStateString( "player_totalammo", "" );
		hud->SetStateString( "player_clips", "" );
		hud->SetStateString( "player_allammo", "" );
		hud->SetStateBool( "player_ammo_empty", false );
		hud->SetStateBool( "player_clip_empty", false );
		hud->SetStateBool( "player_clip_low", false );
		hud->SetStateString( "player_ammo_count", "");
#endif*/

		hud->HandleNamedEvent( "weaponChange" );
	}
}

/*
===============
idPlayer::DrawHUD
===============
*/
void idPlayer::DrawHUD( idUserInterface *_hud ) {
	/* ff1.3 was:
	if ( !weapon.GetEntity() || influenceActive != INFLUENCE_NONE || privateCameraView || gameLocal.GetCamera() || !_hud || !g_showHud.GetBool() ) {
		return;
	}
	*/
	if( !g_showHud.GetBool() ) {
		return;
	}
	if ( weapon.GetEntity() && influenceActive == INFLUENCE_NONE && !privateCameraView && !gameLocal.GetCamera() && _hud ) {

		UpdateHudStats( _hud );

		//_hud->SetStateString( "weapicon", weapon.GetEntity()->Icon() );

		// FIXME: this is temp to allow the sound meter to show up in the hud
		// it should be commented out before shipping but the code can remain
		// for mod developers to enable for the same functionality
		_hud->SetStateInt( "s_debug", cvarSystem->GetCVarInteger( "s_showLevelMeter" ) );

		weapon.GetEntity()->UpdateGUI();

		_hud->Redraw( gameLocal.realClientTime );

		// weapon targeting crosshair
		if ( !GuiActive() && !inGameStatsVisible ) { //ff1.3 inGameStatsVisible added
			if ( cursor && weapon.GetEntity()->ShowCrosshair() ) {

#ifdef _D3XP
				if ( weapon.GetEntity()->GetGrabberState() == 1 || weapon.GetEntity()->GetGrabberState() == 2 || weapon.GetEntity()->GetGrabberState() == 4) { //ff added 4 = vehicle
					cursor->SetStateString( "grabbercursor", "1" );
					cursor->SetStateString( "combatcursor", "0" );
				} else {
					cursor->SetStateString( "grabbercursor", "0" );
					cursor->SetStateString( "combatcursor", "1" );
				}
#endif

				cursor->Redraw( gameLocal.realClientTime );
			}
		}
	}

	//ff start
	if ( /*infoGuiOpen &&*/ infoGui ) {
		if ( inGameStatsCurrentMap ) {
			DisplayMapStats( infoGui, playerStats, gameLocal.mapStats, gameLocal.mapInfo, true );
		}
		infoGui->Redraw( gameLocal.realClientTime );
	}
	//ff end
}

/*
===============
idPlayer::EnterCinematic
===============
*/
void idPlayer::EnterCinematic( void ) {
#ifdef _D3XP
	if ( PowerUpActive( HELLTIME ) ) {
		ClearPowerup( HELLTIME );
	}
#endif

	//ff1.3 start
	//Hide();
	if ( currentVehicle ) {
		currentVehicle->PostEventMS( &EV_Hide, 0 );
	} else if ( currentRiddenAI ) {
		currentRiddenAI->PostEventMS( &EV_Hide, 0 );
	} else {
		Hide();
	}
	//ff1.3 end

	StopAudioLog();
	StopSound( SND_CHANNEL_PDA, false );
	if ( hud ) {
		hud->HandleNamedEvent( "radioChatterDown" );
	}

	physicsObj.SetLinearVelocity( vec3_origin );

	SetState( "EnterCinematic" );
	UpdateScript();

	if ( weaponEnabled && weapon.GetEntity() ) {
		weapon.GetEntity()->EnterCinematic();
	}

	AI_FORWARD		= false;
	AI_BACKWARD		= false;
	AI_STRAFE_LEFT	= false;
	AI_STRAFE_RIGHT	= false;
	AI_RUN			= false;
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED	= false;
	AI_JUMP			= false;
	AI_CROUCH		= false;
	AI_ONGROUND		= true;
	AI_ONLADDER		= false;
	AI_DEAD			= ( health <= 0 );
	AI_RUN			= false;
	AI_PAIN			= false;
	AI_HARDLANDING	= false;
	AI_SOFTLANDING	= false;
	AI_RELOAD		= false;
	AI_TELEPORT		= false;
	AI_TURN_LEFT	= false;
	AI_TURN_RIGHT	= false;
	AI_DODGE		= false; //ff1.3
}

/*
===============
idPlayer::ExitCinematic
===============
*/
void idPlayer::ExitCinematic( void ) {

	//ff1.3 start
	//Show();
	if ( currentVehicle ) {
		currentVehicle->PostEventMS( &EV_Show, 0 );
	} else if ( currentRiddenAI ) {
		currentRiddenAI->PostEventMS( &EV_Show, 0 );
	} else {
		Show();
	}
	//ff1.3 end

	if ( weaponEnabled && weapon.GetEntity() ) {
		weapon.GetEntity()->ExitCinematic();
	}

	SetState( "ExitCinematic" );
	UpdateScript();
}

/*
=====================
idPlayer::UpdateConditions
=====================
*/
void idPlayer::UpdateConditions( void ) {
	idVec3	velocity;
	float	forwardspeed;
	float	sidespeed;

	// minus the push velocity to avoid playing the walking animation and sounds when riding a mover
	velocity = physicsObj.GetLinearVelocity() - physicsObj.GetPushedLinearVelocity();

	if ( influenceActive ) {
		AI_FORWARD		= false;
		AI_BACKWARD		= false;
		AI_STRAFE_LEFT	= false;
		AI_STRAFE_RIGHT	= false;
	} else if ( gameLocal.time - lastDmgTime < 500 ) {
		forwardspeed = velocity * viewAxis[ 0 ];
		sidespeed = velocity * viewAxis[ 1 ];
		AI_FORWARD		= AI_ONGROUND && ( forwardspeed > 20.01f );
		AI_BACKWARD		= AI_ONGROUND && ( forwardspeed < -20.01f );
		AI_STRAFE_LEFT	= AI_ONGROUND && ( sidespeed > 20.01f );
		AI_STRAFE_RIGHT	= AI_ONGROUND && ( sidespeed < -20.01f );
	} else if ( xyspeed > MIN_BOB_SPEED ) {
		AI_FORWARD		= AI_ONGROUND && ( usercmd.forwardmove > 0 );
		AI_BACKWARD		= AI_ONGROUND && ( usercmd.forwardmove < 0 );
		AI_STRAFE_LEFT	= AI_ONGROUND && ( usercmd.rightmove < 0 );
		AI_STRAFE_RIGHT	= AI_ONGROUND && ( usercmd.rightmove > 0 );
	} else {
		AI_FORWARD		= false;
		AI_BACKWARD		= false;
		AI_STRAFE_LEFT	= false;
		AI_STRAFE_RIGHT	= false;
	}

	//ff1.3 start - always run! I mean... walk :)
	//AI_RUN			= ( usercmd.buttons & BUTTON_RUN ) && ( ( !pm_stamina.GetFloat() ) || ( stamina > pm_staminathreshold.GetFloat() ) );
	//ff1.3 end
	AI_DEAD			= ( health <= 0 );
}

/*
==================
WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayer::WeaponFireFeedback( const idDict *weaponDef ) {
	// force a blink
	blink_time = 0;

	// play the fire animation
	AI_WEAPON_FIRED = true;

	// update view feedback
	playerView.WeaponFireFeedback( weaponDef );
}

/*
===============
idPlayer::StopFiring
===============
*/
void idPlayer::StopFiring( void ) {
	AI_ATTACK_HELD	= false;
	AI_WEAPON_FIRED = false;
	AI_RELOAD		= false;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->EndAttack();
		weapon.GetEntity()->EndSpecialFunction(); //ivan - force also the end of secondary fire.
	}
}

/*
===============
idPlayer::FireWeapon
===============
*/
void idPlayer::FireWeapon( bool keyTapped ) { //parameter added
	idMat3 axis;
	idVec3 muzzle;

	if ( privateCameraView ) {
		return;
	}

	if ( g_editEntityMode.GetInteger() ) {
		GetViewPos( muzzle, axis );
		if ( gameLocal.editEntities->SelectEntity( muzzle, axis[0], this ) ) {
			return;
		}
	}

	if ( !hiddenWeapon && weapon.GetEntity()->IsReady() ) {
		//ff1.3 start
		if ( painKillerProjectile.GetEntity() != NULL ) {
			if ( keyTapped ) {
				painKillerProjectile.GetEntity()->StartReturnPhase();
			}
		} else
		//ff1.3 end

		if ( weapon.GetEntity()->AmmoInClip() || weapon.GetEntity()->AmmoAvailable() || weapon.GetEntity()->PwAmmoAvailable() ) { //ff1.1 - pw added
			AI_ATTACK_HELD = true;
			weapon.GetEntity()->BeginAttack();
			/*
			if ( ( weapon_soulcube >= 0 ) && ( currentWeapon == weapon_soulcube ) ) {
				if ( hud ) {
					hud->HandleNamedEvent( "soulCubeNotReady" );
				}
				SelectWeapon( previousWeapon, false );
			}
			*/
/*
#ifdef _D3XP
			if( (weapon_bloodstone >= 0) && (currentWeapon == weapon_bloodstone) && inventory.weapons & ( 1 << weapon_bloodstone_active1 ) && weapon.GetEntity()->GetStatus() == WP_READY) {
				// tell it to switch to the previous weapon. Only do this once to prevent
				// weapon toggling messing up the previous weapon
				if(idealWeapon == weapon_bloodstone) {
					if(previousWeapon == weapon_bloodstone || previousWeapon == -1) {
						NextBestWeapon();
					} else {
						//Since this is a toggle weapon just select itself and it will toggle to the last weapon
						SelectWeapon( weapon_bloodstone, false );
					}
				}
			}
#endif
*/
		} else /*if ( keyTapped )*/ { //ff1.3 if added
			NextBestWeapon();
		}
	}

	if ( hud ) {
		/* ivan - removed
		if ( tipUp ) {
			HideTip();
		}

		// may want to track with with a bool as well
		// keep from looking up named events so often
		if ( objectiveUp ) {
			HideObjective();
		}
		*/
	}
}

/*
===============
idPlayer::CacheWeapons
===============
*/
void idPlayer::CacheWeapons( void ) {
	idStr	weap;
	int		w;

	// check if we have any weapons
	if ( !inventory.weapons ) {
		return;
	}

	for( w = 0; w < MAX_WEAPONS; w++ ) {
		if ( inventory.weapons & ( 1 << w ) ) {
			weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
			if ( weap != "" ) {
				idWeapon::CacheWeapon( weap );
			} else {
				inventory.weapons &= ~( 1 << w );
			}
		}
	}
}

/*
===============
idPlayer::Give
===============
*/
bool idPlayer::Give( const char *statname, const char *value ) {
	int amount;


	if ( AI_DEAD ) {
		return false;
	}

	//ff start
	if ( !idStr::Icmpn( statname, "health", 6 ) ) {

		if ( idStr::Icmp( statname, "health" ) ) {
			switch( g_skill.GetInteger() ) {
				case 0: {
					if ( idStr::Icmp( statname, "health_easy" ) ) {
						return false;
					}
					break;
				}
				case 1: {
					if ( idStr::Icmp( statname, "health_medium" ) ) {
						return false;
					}
					break;
				}
				case 2: {
					if ( idStr::Icmp( statname, "health_hard" ) ) {
						return false;
					}
					break;
				}
				case 3: {
					if ( idStr::Icmp( statname, "health_nightmare" ) ) {
						return false;
					}
					break;
				}
				default: {
					return false;
				}
			}
		}

		if ( health >= inventory.maxHealth ) {
			if ( inventory.lastFullItemName.IsEmpty() && atoi( value ) > 0 ){
				inventory.lastFullItemName = "Health";
			}
			return false;
		}

		amount = atoi( value );
		if ( amount ) {
			health += amount;
			if ( health > inventory.maxHealth ) {
				health = inventory.maxHealth;
			}
			if ( hud ) {
				hud->HandleNamedEvent( "healthPulse" );
			}

			inventory.AddPickupName( "Health", "", amount, this );
			//hud
			//healthAdded += amount;
			//hudPulseFlags |= HUD_PULSE_HEALTH_ADDER;
		}
	}
	/*
	if ( !idStr::Icmp( statname, "health" ) ) {
		if ( health >= inventory.maxHealth ) {
			return false;
		}
		amount = atoi( value );
		if ( amount ) {
			health += amount;
			if ( health > inventory.maxHealth ) {
				health = inventory.maxHealth;
			}
			if ( hud ) {
				hud->HandleNamedEvent( "healthPulse" );
			}
		}

	}
	*/
	else if ( !idStr::Icmp( statname, "takeme" ) ) { // jetpack and possession
		return true;
	}
	//ff end

	//ff1.3 start - disable stamina
	/*
    else if ( !idStr::Icmp( statname, "stamina" ) ) {
		if ( stamina >= 100 ) {
			return false;
		}
		stamina += atof( value );
		if ( stamina > 100 ) {
			stamina = 100;
		}
	}
	*/
	else if ( !idStr::Icmp( statname, "soul" ) ) {
		playerStats.soulsCount++;
		campaignSoulsCount++;
		hudPulseFlags |= HUD_PULSE_SOUL;
		if ( campaignSoulsCount % 100 == 0 ) {
			GiveMedal( MEDAL_100SOULS );
		}
	} else if ( !idStr::Icmp( statname, "monsterHealth" ) ) {
		if ( currentRiddenAI && currentRiddenAI->health > 0 ) {
			currentRiddenAI->health += atoi( value );
		}
	} else if ( !idStr::Icmp( statname, "staminaHelltime" ) ) {
		amount = atoi( value );

		staminaHelltime += amount;
		if ( staminaHelltime > HELLTIME_STAMINA_MAX ) {
			staminaHelltime = HELLTIME_STAMINA_MAX;
		}
	}
	//ff1.3 end
	else if ( !idStr::Icmp( statname, "heartRate" ) ) {
		heartRate += atoi( value );
		if ( heartRate > MAX_HEARTRATE ) {
			heartRate = MAX_HEARTRATE;
		}
	} else if ( !idStr::Icmp( statname, "air" ) ) {
		/* removed by ff1.3 - keep this "if" for backwark compaibility
		if ( airTics >= pm_airTics.GetInteger() ) {
			return false;
		}
		airTics += atoi( value ) / 100.0 * pm_airTics.GetInteger();
		if ( airTics > pm_airTics.GetInteger() ) {
			airTics = pm_airTics.GetInteger();
		}
		*/
#ifdef _D3XP
	} else if ( !idStr::Icmp( statname, "enviroTime" ) ) {
		if ( PowerUpActive( ENVIROTIME ) ) {
			inventory.powerupEndTime[ ENVIROTIME ] += (atof(value) * 1000);
		} else {
			GivePowerUp( ENVIROTIME, atoi(value)*1000 );
		}
	} else {
		/*
		bool ret = inventory.Give( this, spawnArgs, statname, value, &idealWeapon, true );
		if(!idStr::Icmp( statname, "ammo_bloodstone" ) ) {
			//int i = inventory.AmmoIndexForAmmoClass( statname );
			//int max = inventory.MaxAmmoForAmmoClass( this, statname );
			//if(hud && inventory.ammo[ i ] >= max) {

			if(hud) {

				//Force an update of the bloodstone ammount
				int ammoRequired;
				ammo_t ammo_i = inventory.AmmoIndexForWeaponClass( "weapon_bloodstone_passive", &ammoRequired );
				int bloodstoneAmmo = inventory.HasAmmo( ammo_i, ammoRequired );
				hud->SetStateString("player_bloodstone_ammo", va("%i", bloodstoneAmmo));

				hud->HandleNamedEvent("bloodstoneReady");
				//Make sure we unlock the ability to harvest
				harvest_lock = false;
			}
		}
		return ret;
		*/
		return inventory.Give( this, spawnArgs, statname, value, &idealWeapon, true );
#else
		return inventory.Give( this, spawnArgs, statname, value, &idealWeapon, true );
#endif
	}
	return true;
}


/*
===============
idPlayer::GiveHealthPool

adds health to the player health pool
===============
*/
void idPlayer::GiveHealthPool( float amt ) {

	if ( AI_DEAD ) {
		return;
	}

	if ( health > 0 ) {
		healthPool += amt;
		if ( healthPool > inventory.maxHealth - health ) {
			healthPool = inventory.maxHealth - health;
		}
		nextHealthPulse = gameLocal.time;
	}
}

/*
===============
idPlayer::GiveItem

Returns false if the item shouldn't be picked up
===============
*/
bool idPlayer::GiveItem( idItem *item ) {
	int					i;
	const idKeyValue	*arg;
	idDict				attr;
	bool				gave;
	int					numPickup;
	int					amountPickup; //ff1.3

	if ( gameLocal.isMultiplayer && spectating ) {
		return false;
	}

	item->GetAttributes( attr );

	gave = false;
	numPickup = inventory.pickupItemNames.Num();
	amountPickup = numPickup > 0 ? inventory.pickupItemNames[numPickup-1].amount : 0; //ff1.3
	for( i = 0; i < attr.GetNumKeyVals(); i++ ) {
		arg = attr.GetKeyVal( i );
		if ( Give( arg->GetKey(), arg->GetValue() ) ) {
			gave = true;
		}
	}

	arg = item->spawnArgs.MatchPrefix( "inv_weapon", NULL );
	if ( arg && hud ) {
		// We need to update the weapon hud manually, but not
		// the armor/ammo/health because they are updated every
		// frame no matter what
		UpdateHudWeapon( false );
		hud->HandleNamedEvent( "weaponPulse" );
	}

	// display the pickup feedback on the hud
	//if ( gave && ( numPickup == inventory.pickupItemNames.Num() ) ) {
	if ( gave && ( numPickup == inventory.pickupItemNames.Num() ) && ( numPickup == 0 || amountPickup == inventory.pickupItemNames[numPickup-1].amount ) ) { //ff1.3 - add case where only the amount has changed
		inventory.AddPickupName( item->spawnArgs.GetString( "inv_name" ), item->spawnArgs.GetString( "inv_icon" ), 0, this ); //_D3XP
	}

	return gave;
}

/*
===============
idPlayer::PowerUpModifier
===============
*/
float idPlayer::PowerUpModifier( int type ) {
	float mod = 1.0f;

	if ( PowerUpActive( BERSERK ) ) {
		switch( type ) {
			case SPEED: {
				mod *= 1.7f;
				break;
			}
			case PROJECTILE_DAMAGE: {
				mod *= 2.0f;
				break;
			}
			case MELEE_DAMAGE: {
				mod *= 30.0f;
				break;
			}
			case MELEE_DISTANCE: {
				mod *= 2.0f;
				break;
			}
		}
	}

	//ff1.3 start
	if ( PowerUpActive( ADRENALINE ) ) {
		switch( type ) {
			case SPEED: {
				mod *= 2.0f;
				break;
			}
		}
	}
	//ff1.3 end

	if ( gameLocal.isMultiplayer && !gameLocal.isClient ) {
		if ( PowerUpActive( MEGAHEALTH ) ) {
			if ( healthPool <= 0 ) {
				GiveHealthPool( 100 );
			}
		} else {
			healthPool = 0;
		}

#ifdef _D3XP
		/*if( PowerUpActive( HASTE ) ) {
			switch( type ) {
			case SPEED: {
				mod = 1.7f;
				break;
						}
			}
		}*/
#endif
	}

	return mod;
}

/*
===============
idPlayer::PowerUpActive
===============
*/
bool idPlayer::PowerUpActive( int powerup ) const {
	return ( inventory.powerups & ( 1 << powerup ) ) != 0;
}

/*
===============
idPlayer::GivePowerUp
===============
*/
bool idPlayer::GivePowerUp( int powerup, int time ) {
	const char *sound;
	const char *skin;

	if ( powerup >= 0 && powerup < MAX_POWERUPS ) {

		//ff1.3 start - check preconditions before adding it to the inventory
		switch( powerup ) {
			case HELLTIME: {
				if ( staminaHelltime < 5 || !staminaEnabled || IsRiding() ) {
					return false; //not allowed
				}
				break;
			}
		}
		//ff1.3 end

		if ( gameLocal.isServer ) {
			idBitMsg	msg;
			byte		msgBuf[MAX_EVENT_PARAM_SIZE];

			msg.Init( msgBuf, sizeof( msgBuf ) );
			msg.WriteShort( powerup );
			msg.WriteBits( 1, 1 );
			ServerSendEvent( EVENT_POWERUP, &msg, false, -1 );
		}

		if ( powerup != MEGAHEALTH ) {
			inventory.GivePowerUp( this, powerup, time );
		}

		const idDeclEntityDef *def = NULL;

		switch( powerup ) {
			case BERSERK: {
				if(gameLocal.isMultiplayer && !gameLocal.isClient) {
					inventory.AddPickupName("#str_00100627", "", 0, this);
				}

				if(gameLocal.isMultiplayer) {
					if ( spawnArgs.GetString( "snd_berserk_third", "", &sound ) ) {
						StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_DEMONIC, 0, false, NULL );
					}
				}


				if ( baseSkinName.Length() ) {
					powerUpSkin = declManager->FindSkin( baseSkinName + "_berserk" );
				}
				if ( !gameLocal.isClient ) {
#ifdef _D3XP
					if( !gameLocal.isMultiplayer ) {
						// Trying it out without the health boost (1/3/05)
						// Give the player full health in single-player
						// health = 100;
					} else {
						// Switch to fists in multiplayer
						idealWeapon = 1;
					}
#else
					idealWeapon = 0;
#endif
				}
				break;
			}
			case INVISIBILITY: {
				if(gameLocal.isMultiplayer && !gameLocal.isClient) {
					inventory.AddPickupName("#str_00100628", "", 0, this);
				}
				spawnArgs.GetString( "skin_invisibility", "", &skin );
				powerUpSkin = declManager->FindSkin( skin );
				// remove any decals from the model
				if ( modelDefHandle != -1 ) {
					gameRenderWorld->RemoveDecals( modelDefHandle );
				}
				if ( weapon.GetEntity() ) {
					weapon.GetEntity()->UpdateSkin();
				}
/*				if ( spawnArgs.GetString( "snd_invisibility", "", &sound ) ) {
					StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_ANY, 0, false, NULL );
				} */
				break;
			}
			case ADRENALINE: {
#ifdef _D3XP
				inventory.AddPickupName(va("Speed +%ds", time/1000), "", 0, this); //ff1.3 - was "#str_00100799"
#endif
				//stamina = 100.0f;
				break;
			 }
			case MEGAHEALTH: {
				if(gameLocal.isMultiplayer && !gameLocal.isClient) {
					inventory.AddPickupName("#str_00100629", "", 0, this);
				}
				if ( spawnArgs.GetString( "snd_megahealth", "", &sound ) ) {
					StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_ANY, 0, false, NULL );
				}
				def = gameLocal.FindEntityDef( "powerup_megahealth", false );
				if ( def ) {
					health = def->dict.GetInt( "inv_health" );
				}
				break;
			 }
#ifdef _D3XP
			case HELLTIME: {
				if ( spawnArgs.GetString( "snd_helltime_start", "", &sound ) ) {
					PostEventMS( &EV_StartSoundShader, 0, sound, SND_CHANNEL_ANY );
				}
				if ( spawnArgs.GetString( "snd_helltime_loop", "", &sound ) ) {
					PostEventMS( &EV_StartSoundShader, 0, sound, SND_CHANNEL_DEMONIC );
				}
				//ff1.3 start
				//influenceMaterial = declManager->FindMaterial( "textures/decals/bloodyfilm1" );
				bloomEnabled = ff_slowmoBloom.GetBool(); //( g_testBloomNumPasses.GetInteger() > 0 );
				physicsObj.SetGravityMultiplier( HELLTIME_GRAVITY_MULT );
				/*
				if ( weaponEnabled ) {
					Event_DisableWeapon();
				}
				*/
				//ff1.3 end
				break;
			}
			case ENVIROSUIT: {
				// Turn on the envirosuit sound
				if ( gameSoundWorld ) {
					gameSoundWorld->SetEnviroSuit( true );
				}

				// Put the helmet and lights on the player
				idDict	args;

				// Light
				const idDict *lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );
				if ( lightDef ) {
					idEntity *temp;
					gameLocal.SpawnEntityDef( *lightDef, &temp, false );

					idLight *eLight = static_cast<idLight *>(temp);
					eLight->GetPhysics()->SetOrigin( firstPersonViewOrigin );
					eLight->UpdateVisuals();
					eLight->Present();

					enviroSuitLight = eLight;
				}
				break;
			}
			case ENVIROTIME: {
				/* ff1.3 - not supported
				hudPowerup = ENVIROTIME;
				// The HUD display bar is fixed at 60 seconds
				hudPowerupDuration = 60000;
				*/
				break;
			}
			case INVULNERABILITY: {

				if(gameLocal.isMultiplayer) {
					if(!gameLocal.isClient) {
						inventory.AddPickupName("#str_00100630", "", 0, this);
					}
					/*if ( spawnArgs.GetString( "snd_invulnerable", "", &sound ) ) {
						StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_DEMONIC, 0, false, NULL );
					}*/
					if ( baseSkinName.Length() ) {
						powerUpSkin = declManager->FindSkin( baseSkinName + "_invuln" );
					}
				}
				//ff1.3 start
				else {
					inventory.AddPickupName( va("Invulnerability +%ds", time/1000), "", 0, this);
					if ( spawnArgs.GetString( "snd_invuln_start", "", &sound ) ) {
						StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_PDA, 0, false, NULL );
					}
					nextInvulnWarningTime = inventory.powerupEndTime[ INVULNERABILITY ] - 2500;
				}
				//ff1.3 end
				break;
			}
			/*case HASTE: {
				if(gameLocal.isMultiplayer && !gameLocal.isClient) {
					inventory.AddPickupName("#str_00100631", "", this);
				}

				if ( baseSkinName.Length() ) {
					powerUpSkin = declManager->FindSkin( baseSkinName + "_haste" );
				}
				break;
			}*/
#endif
		}

		if ( hud ) {
			hud->HandleNamedEvent( "itemPickup" );
		}

		return true;
	} else {
		gameLocal.Warning( "Player given power up %i\n which is out of range", powerup );
	}
	return false;
}

/*
==============
idPlayer::ClearPowerup
==============
*/
void idPlayer::ClearPowerup( int i ) {
	const char* sound; //ff1.3

	if ( gameLocal.isServer ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteShort( i );
		msg.WriteBits( 0, 1 );
		ServerSendEvent( EVENT_POWERUP, &msg, false, -1 );
	}

	powerUpSkin = NULL;
	inventory.powerups &= ~( 1 << i );
	inventory.powerupEndTime[ i ] = 0;
	switch( i ) {
		case BERSERK: {
			if(gameLocal.isMultiplayer) {
				StopSound( SND_CHANNEL_DEMONIC, false );
			}
#ifdef _D3XP
			if(!gameLocal.isMultiplayer) {
				StopHealthRecharge();
			}
#endif
			break;
		}
		case INVISIBILITY: {
			if ( weapon.GetEntity() ) {
				weapon.GetEntity()->UpdateSkin();
			}
			break;
		}
#ifdef _D3XP
		case HELLTIME: {
			//ff1.3 start
			//was: StopSound( SND_CHANNEL_DEMONIC, false );
			if ( spawnArgs.GetString( "snd_helltime_stop", "", &sound ) ) {
				StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_DEMONIC, 0, false, NULL );
			}

			//influenceMaterial = NULL;
			bloomEnabled = false;
			physicsObj.SetGravityMultiplier( 1.0f );
			gameLocal.QuickSlowmoReset();
			//ff1.3 end
			break;
		}
		case ENVIROSUIT: {

			//hudPowerup = -1;

			// Turn off the envirosuit sound
			if ( gameSoundWorld ) {
				gameSoundWorld->SetEnviroSuit( false );
			}

			// Take off the helmet and lights
			if ( enviroSuitLight.IsValid() ) {
				enviroSuitLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
			}
			enviroSuitLight = NULL;
			break;
		}
		case INVULNERABILITY: {
			if(gameLocal.isMultiplayer) {
				StopSound( SND_CHANNEL_DEMONIC, false );
			}
		}
		/*case HASTE: {
			if(gameLocal.isMultiplayer) {
				StopSound( SND_CHANNEL_DEMONIC, false );
			}
		}*/
#endif
	}
}

//ff1.3 start
/*
===============
idPlayer::UpdateMedalsQueue
===============
*/
#define MEDAL_SHOW_TIME			2500

void idPlayer::UpdateMedalsQueue( void ) {
	if ( medalsQueue.Num() > 0 && nextMedalTime < gameLocal.slow.time ) {
		int medalType = medalsQueue[0];
		switch( medalType ) {
			case MEDAL_SKILLSHOT: {
				playerStats.skillshot++;
				if ( hud ) {
					hud->HandleNamedEvent( "skillshot" );
				}
				AddExtraHealth();
				break;
			}
			case MEDAL_DOUBLEKILL: {
				playerStats.doublekill++;
				if ( hud ) {
					hud->HandleNamedEvent( "doublekill" );
				}
				AddExtraHealth();
				break;
			}
			case MEDAL_MULTIKILL: {
				playerStats.multikill++;
				if ( hud ) {
					hud->HandleNamedEvent( "multikill" );
				}
				AddExtraHealth();
				break;
			}
			case MEDAL_CHAINKILL: {
				playerStats.chainkill++;
				if ( hud ) {
					hud->HandleNamedEvent( "chainkill" );
				}
				AddExtraHealth();
				break;
			}
			case MEDAL_MASSACRE: {
				playerStats.massacre++;
				if ( hud ) {
					hud->HandleNamedEvent( "massacre" );
				}
				AddExtraHealth();
				break;
			}
			case MEDAL_100SOULS: {
				if ( hud ) {
					hud->HandleNamedEvent( "soulsMedal" );
				}
				IncreaseMaxHealth();
				AddExtraHealth();
				break;
			}
			case MEDAL_SECRET: {
				playerStats.secretsFound++;
				if ( hud ) {
					hud->HandleNamedEvent( "secretFound" );
				}
				//AddExtraHealth();
				break;
			}
			default: {
				gameLocal.Warning("Invalid medalType %i", medalType);
				return;
			}
		}
		medalsQueue.RemoveIndex(0);
		nextMedalTime = gameLocal.slow.time + MEDAL_SHOW_TIME;
	}
}



/*
==============
idPlayer::UpdateKillsMedals
==============
*/
void idPlayer::UpdateKillsMedals( void ) {
	if ( nextKillUpdate < gameLocal.time ) {
		if ( tempKillCount > 0 ) {
			if ( tempKillCount == 2 ) {
				//gameLocal.Printf("Doublekill\n");
				GiveMedal(MEDAL_DOUBLEKILL);
			}
			else if ( tempKillCount > 2 ) {
				//gameLocal.Printf("Multikill\n");
				GiveMedal(MEDAL_MULTIKILL);
			}

			//this should always be the last medal check
			if ( lastKillTimes[ 0 ] - lastKillTimes[ MASSACRE_KILLS-1 ] < MASSACRE_TIME_DELTA ) {
				//gameLocal.Printf("Massacre\n");
				GiveMedal(MEDAL_MASSACRE);
				for( int i = 0; i < MASSACRE_KILLS; i++ ) {
					lastKillTimes[ i ] = 0;
				}
			}
		}
		//prepare for next time
		nextKillUpdate = gameLocal.time + 100;
		tempKillCount = 0;
	}
}
//ff1.3 end

/*
==============
idPlayer::UpdatePowerUps
==============
*/
void idPlayer::GiveMedal( int medalType ) {
	if( !medalsEnabled ){
		return;
	}
	medalsQueue.Append(medalType);
}

/*
==============
idPlayer::UpdatePowerUps
==============
*/
void idPlayer::UpdatePowerUps( void ) {
	int i;

	if ( !gameLocal.isClient ) {
		for ( i = 0; i < MAX_POWERUPS; i++ ) {
			//ff1.3 start
			if ( PowerUpActive( i ) ){
				if ( inventory.powerupEndTime[i] <= gameLocal.slow.time ) {
					ClearPowerup( i );
				} else {
#ifdef _D3XP
			//if ( ( inventory.powerups & ( 1 << i ) ) && inventory.powerupEndTime[i] > gameLocal.time ) {
					switch( i ) {
						//ff1.3 start
						case HELLTIME: {
							//NOTE: don't decrease stamina while possession Projectile is active
							if ( !privateCameraView && nextStaminaDrop < gameLocal.time ) {
								nextStaminaDrop = gameLocal.time + pm_staminaHelltimeRate.GetInteger();
								staminaHelltime--;
							}
							if ( staminaHelltime <= 0 ) {
								staminaHelltime = 0;
								ClearPowerup( i );
							}
							break;
						}
						case INVULNERABILITY: {
							//NOTE: don't decrease stamina while possession Projectile is active
							if ( nextInvulnWarningTime > 0 && nextInvulnWarningTime < gameLocal.time ) {
								nextInvulnWarningTime = 0;

								const char* sound;
								if ( spawnArgs.GetString( "snd_invuln_warn", "", &sound ) ) {
									StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_PDA, 0, false, NULL );
								}
							}
							break;
						}
						//ff1.3 end
						case ENVIROSUIT: {
							if ( enviroSuitLight.IsValid() ) {
								idAngles lightAng = firstPersonViewAxis.ToAngles();
								idVec3 lightOrg = firstPersonViewOrigin;
								const idDict *lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );

								idVec3 enviroOffset = lightDef->GetVector( "enviro_offset" );
								idVec3 enviroAngleOffset = lightDef->GetVector( "enviro_angle_offset" );

								lightOrg += (enviroOffset.x * firstPersonViewAxis[0]);
								lightOrg += (enviroOffset.y * firstPersonViewAxis[1]);
								lightOrg += (enviroOffset.z * firstPersonViewAxis[2]);
								lightAng.pitch += enviroAngleOffset.x;
								lightAng.yaw += enviroAngleOffset.y;
								lightAng.roll += enviroAngleOffset.z;

								enviroSuitLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
								enviroSuitLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
								enviroSuitLight.GetEntity()->UpdateVisuals();
								enviroSuitLight.GetEntity()->Present();
							}
							break;
						}
						default: {
							break;
						}
					}
				}
			} //ff1.3 end
#endif
			/* was:
			if ( PowerUpActive( i ) && inventory.powerupEndTime[i] <= gameLocal.time ) {
				ClearPowerup( i );
			}
			*/
		}
	}

	if ( health > 0 ) {
		if ( powerUpSkin ) {
			renderEntity.customSkin = powerUpSkin;
		} else {
			renderEntity.customSkin = skin;
		}
	}

	if ( healthPool && gameLocal.time > nextHealthPulse && !AI_DEAD && health > 0 ) {
		assert( !gameLocal.isClient );	// healthPool never be set on client
		int amt = ( healthPool > 5 ) ? 5 : healthPool;
		health += amt;
		if ( health > inventory.maxHealth ) {
			//health = inventory.maxHealth; //ff1.3 removed
			healthPool = 0;
		} else {
			healthPool -= amt;
		}
		nextHealthPulse = gameLocal.time + HEALTHPULSE_TIME;
		healthPulse = true;
	}
    /* //ff1.3 - don't take health
	if ( !gameLocal.inCinematic && influenceActive == 0 && g_skill.GetInteger() == 3 && gameLocal.time > nextHealthTake && !AI_DEAD && health > g_healthTakeLimit.GetInteger() ) {
		assert( !gameLocal.isClient );	// healthPool never be set on client

#ifdef _D3XP
		if(!PowerUpActive(INVULNERABILITY)) {
#endif
		health -= g_healthTakeAmt.GetInteger();
		if ( health < g_healthTakeLimit.GetInteger() ) {
			health = g_healthTakeLimit.GetInteger();
		}
#ifdef _D3XP
		}
#endif
		nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
		healthTake = true;
	}
	*/
}

/*
===============
idPlayer::ClearPowerUps
===============
*/
void idPlayer::ClearPowerUps( void ) {
	int i;
	for ( i = 0; i < MAX_POWERUPS; i++ ) {
		if ( PowerUpActive( i ) ) {
			ClearPowerup( i );
		}
	}
	inventory.ClearPowerUps();

#ifdef _D3XP
	if ( gameLocal.isMultiplayer ) {
		if ( enviroSuitLight.IsValid() ) {
			enviroSuitLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
		}
	}
#endif
}

/*
===============
idPlayer::GiveInventoryItem
===============
*/
bool idPlayer::GiveInventoryItem( idDict *item ) {
	if ( gameLocal.isMultiplayer && spectating ) {
		return false;
	}
	inventory.items.Append( new idDict( *item ) );
	idItemInfo info;
	const char* itemName = item->GetString( "inv_name" );
	if ( idStr::Cmpn( itemName, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
		info.name = common->GetLanguageDict()->GetString( itemName );
	} else {
		info.name = itemName;
	}
	info.icon = item->GetString( "inv_icon" );
	info.amount = 0;
	inventory.pickupItemNames.Append( info );
	if ( hud ) {
		hud->SetStateString( "itemicon", info.icon );
		hud->HandleNamedEvent( "invPickup" );
	}

#ifdef _D3XP //Added to support powercells
	if(item->GetInt("inv_powercell") && focusUI) {
		//Reset the powercell count
		int powerCellCount = 0;
		for ( int j = 0; j < inventory.items.Num(); j++ ) {
			idDict *item = inventory.items[ j ];
			if(item->GetInt("inv_powercell")) {
				powerCellCount++;
			}
		}
		focusUI->SetStateInt( "powercell_count", powerCellCount );
	}
#endif

	return true;
}

#ifdef _D3XP //BSM: Implementing this defined function for scripted give inventory items
/*
==============
idPlayer::GiveInventoryItem
==============
*/
bool idPlayer::GiveInventoryItem( const char *name ) {
	idDict args;

	args.Set( "classname", name );
	args.Set( "owner", this->name.c_str() );
	gameLocal.SpawnEntityDef( args);
	return true;
}
#endif

/*
==============
idPlayer::UpdateObjectiveInfo
==============
 */
void idPlayer::UpdateObjectiveInfo( void ) {
	if ( objectiveSystem == NULL ) {
		return;
	}
	objectiveSystem->SetStateString( "objective1", "" );
	objectiveSystem->SetStateString( "objective2", "" );
	objectiveSystem->SetStateString( "objective3", "" );
	for ( int i = 0; i < inventory.objectiveNames.Num(); i++ ) {
		objectiveSystem->SetStateString( va( "objective%i", i+1 ), "1" );
		objectiveSystem->SetStateString( va( "objectivetitle%i", i+1 ), inventory.objectiveNames[i].title.c_str() );
		objectiveSystem->SetStateString( va( "objectivetext%i", i+1 ), inventory.objectiveNames[i].text.c_str() );
		objectiveSystem->SetStateString( va( "objectiveshot%i", i+1 ), inventory.objectiveNames[i].screenshot.c_str() );
	}
	objectiveSystem->StateChanged( gameLocal.time );
}

/*
===============
idPlayer::GiveObjective
===============
*/
void idPlayer::GiveObjective( const char *title, const char *text, const char *screenshot ) {
	idObjectiveInfo info;
	info.title = title;
	info.text = text;
	info.screenshot = screenshot;
	inventory.objectiveNames.Append( info );
	ShowObjective( "newObjective" );
	if ( hud ) {
		hud->HandleNamedEvent( "newObjective" );

		/*
		//ff1.3 - replace default obj with new obj
		hud->SetStateString( "current_levelshot", screenshot );
		hud->SetStateString( "current_leveltext", text );
		*/
	}
}

/*
===============
idPlayer::CompleteObjective
===============
*/
void idPlayer::CompleteObjective( const char *title ) {
	int c = inventory.objectiveNames.Num();
	for ( int i = 0;  i < c; i++ ) {
		if ( idStr::Icmp(inventory.objectiveNames[i].title, title) == 0 ) {
			inventory.objectiveNames.RemoveIndex( i );
			break;
		}
	}
	ShowObjective( "newObjectiveComplete" );

	if ( hud ) {
		hud->HandleNamedEvent( "newObjectiveComplete" );
	}
}

//ff1.3 start

/*
===============
idPlayer::GiveGameCover
===============
*/
void idPlayer::GiveGameCover( const char *coverDefName ) {
	int i;

	const idDeclEntityDef *coverDef = gameLocal.FindEntityDef( coverDefName, false );
	if ( coverDefName == NULL || !coverDef ) {
		return;
	}

	for ( i = inventory.gameCovers.Num() - 1; i >= 0; i-- ) {
		if ( idStr::Icmp( coverDefName, inventory.gameCovers[i].defName ) == 0 ){
			//inventory.gameCovers[i].amount++;
			selectedGameCoverIndex = i;
			//gameLocal.Printf("Amount: %d\n", inventory.gameCovers[i].amount);
			break;
		}
	}
	if ( i < 0 ) {
		idGameCoverInfo gameCover;
		gameCover.defName = coverDefName;
		//gameCover.amount = 1;
		selectedGameCoverIndex = inventory.gameCovers.Append( gameCover );
	}

	if ( hud ) {
		//hud->SetStateString( "cover_name", coverDef->dict.GetString("name") );
		hud->SetStateString( "cover_icon", coverDef->dict.GetString("mtr_cover") );
		hud->HandleNamedEvent( "coverPickup" );
	}
	playerStats.gameCoversFound++;
}
//ff1.3 end

/*
===============
idPlayer::GiveVideo
===============
*/
void idPlayer::GiveVideo( const char *videoName, idDict *item ) {

	if ( videoName == NULL || *videoName == 0 ) {
		return;
	}

	inventory.videos.AddUnique( videoName );

	if ( item ) {
		idItemInfo info;
		info.name = item->GetString( "inv_name" );
		info.icon = item->GetString( "inv_icon" );
		info.amount = 0; //ff1.3
		inventory.pickupItemNames.Append( info );
	}
	if ( hud ) {
		hud->HandleNamedEvent( "videoPickup" );
	}
}

/*
===============
idPlayer::GiveSecurity
===============
*/
void idPlayer::GiveSecurity( const char *security ) {
	GetPDA()->SetSecurity( security );
	if ( hud ) {
		hud->SetStateString( "pda_security", "1" );
		hud->HandleNamedEvent( "securityPickup" );
	}
}

/*
===============
idPlayer::GiveEmail
===============
*/
void idPlayer::GiveEmail( const char *emailName ) {

	if ( emailName == NULL || *emailName == 0 ) {
		return;
	}

	inventory.emails.AddUnique( emailName );
	GetPDA()->AddEmail( emailName );

	if ( hud ) {
		hud->HandleNamedEvent( "emailPickup" );
	}
}

/*
===============
idPlayer::GivePDA
===============
*/
void idPlayer::GivePDA( const char *pdaName, idDict *item )
{
	if ( gameLocal.isMultiplayer && spectating ) {
		return;
	}

	if ( item ) {
		inventory.pdaSecurity.AddUnique( item->GetString( "inv_name" ) );
	}

	if ( pdaName == NULL || *pdaName == 0 ) {
		pdaName = spawnArgs.GetString( "pda_name_personal", "ff_personal" ); //ff1.3 - was: "personal";
	}

	const idDeclPDA *pda = static_cast< const idDeclPDA* >( declManager->FindType( DECL_PDA, pdaName ) );

	inventory.pdas.AddUnique( pdaName );

	// Copy any videos over
	for ( int i = 0; i < pda->GetNumVideos(); i++ ) {
		const idDeclVideo *video = pda->GetVideoByIndex( i );
		if ( video ) {
			inventory.videos.AddUnique( video->GetName() );
		}
	}

	// This is kind of a hack, but it works nicely
	// We don't want to display the 'you got a new pda' message during a map load
	if ( gameLocal.GetFrameNum() > 10 ) {
		if ( pda && hud ) {
			idStr pdaName = pda->GetPdaName();
			pdaName.RemoveColors();
			hud->SetStateString( "pda", "1" );
			hud->SetStateString( "pda_text", pdaName );
			const char *sec = pda->GetSecurity();
			hud->SetStateString( "pda_security", ( sec && *sec ) ? "1" : "0" );
			hud->HandleNamedEvent( "pdaPickup" );
		}

		if ( inventory.pdas.Num() == 1 ) {
			GetPDA()->RemoveAddedEmailsAndVideos();
			if ( !objectiveSystemOpen ) {
				TogglePDA();
			}
			objectiveSystem->HandleNamedEvent( "showPDATip" );
		}

		if ( inventory.pdas.Num() > 1 && pda->GetNumVideos() > 0 && hud ) {
			hud->HandleNamedEvent( "videoPickup" );
		}
	}
}

/*
===============
idPlayer::FindInventoryItem
===============
*/
idDict *idPlayer::FindInventoryItem( const char *name ) {
	for ( int i = 0; i < inventory.items.Num(); i++ ) {
		const char *iname = inventory.items[i]->GetString( "inv_name" );
		if ( iname && *iname ) {
			if ( idStr::Icmp( name, iname ) == 0 ) {
				return inventory.items[i];
			}
		}
	}
	return NULL;
}

/*
===============
idPlayer::RemoveInventoryItem
===============
*/
void idPlayer::RemoveInventoryItem( const char *name ) {
	//Hack for localization
	if(!idStr::Icmp(name, "Pwr Cell")) {
		name = common->GetLanguageDict()->GetString( "#str_00101056" );
	}
	idDict *item = FindInventoryItem(name);
	if ( item ) {
		RemoveInventoryItem( item );
	}
}

/*
===============
idPlayer::RemoveInventoryItem
===============
*/
void idPlayer::RemoveInventoryItem( idDict *item ) {
	inventory.items.Remove( item );

#ifdef _D3XP //Added to support powercells
	if(item->GetInt("inv_powercell") && focusUI) {
		//Reset the powercell count
		int powerCellCount = 0;
		for ( int j = 0; j < inventory.items.Num(); j++ ) {
			idDict *item = inventory.items[ j ];
			if(item->GetInt("inv_powercell")) {
				powerCellCount++;
			}
		}
		focusUI->SetStateInt( "powercell_count", powerCellCount );
	}
#endif

	delete item;
}

/*
===============
idPlayer::GiveItem
===============
*/
void idPlayer::GiveItem( const char *itemname ) {
	idDict args;

	args.Set( "classname", itemname );
	args.Set( "owner", name.c_str() );
	gameLocal.SpawnEntityDef( args );
	if ( hud ) {
		hud->HandleNamedEvent( "itemPickup" );
	}
}

/*
==================
idPlayer::SlotForWeapon
==================
*/
int idPlayer::SlotForWeapon( const char *weaponName ) {
	int i;

	for( i = 0; i < MAX_WEAPONS; i++ ) {
		const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
		if ( !idStr::Cmp( weap, weaponName ) ) {
			return i;
		}
	}

	// not found
	return -1;
}

/*
===============
idPlayer::Reload
===============
*/
void idPlayer::Reload( void ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( spectating || gameLocal.inCinematic || influenceActive ) {
		return;
	}

	if ( weapon.GetEntity() && weapon.GetEntity()->IsLinked() ) {
		weapon.GetEntity()->Reload();
	}
}

//ff1.3 start

/*
===============
idPlayer::WeaponSpecialFunction

Weapon special function- Added by Clone JC Denton
===============
*/
void idPlayer::WeaponSpecialFunction( bool keyTapped ) {

	/* 	old Denton code
	if ( gameLocal.isClient ) {
		return;
	}

	if ( spectating || gameLocal.inCinematic || influenceActive ) {
		return;
	}

	if ( !hiddenWeapon && weapon.GetEntity() && weapon.GetEntity()->IsLinked() ) {
		weapon.GetEntity()->BeginSpecialFunction( keyTapped );
	}
	*/

	idMat3 axis;
	idVec3 muzzle;

	if ( privateCameraView ) {
		return;
	}

	if ( !hiddenWeapon && weapon.GetEntity()->IsReady() ) {
		if ( weapon.GetEntity()->AmmoInClip() || weapon.GetEntity()->AmmoAvailable() || weapon.GetEntity()->PwAmmoAvailable() ) { //ff1.1 - pw added
			//AI_ATTACK_HELD = true;
			//weapon.GetEntity()->BeginAttack();
			weapon.GetEntity()->BeginSpecialFunction( keyTapped );
		} else /*if ( keyTapped ) */ {
			NextBestWeapon();
		}
	}

}



/*
===============
idPlayer::NextBestWeapon
===============
*/
void idPlayer::NextBestWeapon( void ) {
	const char *weap;
	int w;
	bool ascendingPhase;

	if ( gameLocal.isClient || !weaponEnabled ) {
		return;
	}

	if ( currentWeapon != idealWeapon ) { //this in necessary because this method is invoked many times for a single weapon switch!
		//gameLocal.Printf("next weapon already selected: abort NextBestWeapon\n");
		return;
	}

	ascendingPhase = true;
	w = idealWeapon;
	//gameLocal.Printf("NextBestWeapon\n");

	while ( w > 0 ) {
		if ( ascendingPhase ) { //search next best weapon first
			w++;
			if ( w >= MAX_WEAPONS ) {
				w = idealWeapon;
				ascendingPhase = false;
				continue;
			}
		} else { //then search previous best weapon
			w--;
			if ( w <= 0 ) {
				//gameLocal.Warning("could not find valid weapon!");
				w = weapon_fists;
				break;
			}
		}

		//gameLocal.Printf("Test weapon %d\n", w );

		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
#ifdef _D3XP
		if ( !weap[ 0 ] || (( inventory.weapons & ( 1 << w ) ) == 0) ){ //ff1.1 not present
			continue;
		}

		//if no pw && not enough ammo
		if ( !inventory.HasPwAmmo(w) && ( !inventory.HasAmmo( weap, true, this ) || (inventory.HasEmptyClipCannotRefill(weap, this)) ) ) {  //ff1.1
#else
		if ( !weap[ 0 ] || ( ( inventory.weapons & ( 1 << w ) ) == 0 ) || ( !(inventory.HasAmmo( weap )) ) ) {
#endif
			continue;
		}
		if ( !spawnArgs.GetBool( va( "weapon%d_best", w ) ) ) {
			continue;
		}

		//gameLocal.Printf("break on weapon %d\n", w );
		break;
	}
	idealWeapon = w;
	weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
	UpdateHudWeapon();
}

/* OLD D3 LOGIC
void idPlayer::NextBestWeapon( void ) {
	const char *weap;
	int w = MAX_WEAPONS;

	if ( gameLocal.isClient || !weaponEnabled ) {
		return;
	}

	while ( w > 0 ) {
		w--;
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
#ifdef _D3XP
		if ( !weap[ 0 ] || (( inventory.weapons & ( 1 << w ) ) == 0) ){ //ff1.1 not present
			continue;
		}

		//if no pw && not enough ammo
		if ( !inventory.HasPwAmmo(w) && ( !inventory.HasAmmo( weap, true, this ) || (inventory.HasEmptyClipCannotRefill(weap, this)) ) ) {  //ff1.1
#else
		if ( !weap[ 0 ] || ( ( inventory.weapons & ( 1 << w ) ) == 0 ) || ( !(inventory.HasAmmo( weap )) ) ) {
#endif
			continue;
		}
		if ( !spawnArgs.GetBool( va( "weapon%d_best", w ) ) ) {
			continue;
		}
// ff1.1 - this has been moved above
//#ifdef _D3XP
//		//Some weapons will report having ammo but the clip is empty and
//		//will not have enough to fill the clip (i.e. Double Barrel Shotgun with 1 round left)
//		//We need to skip these weapons because they cannot be used
//		if (inventory.HasEmptyClipCannotRefill(weap, this)) {
//			continue;
//		}
//#endif

		break;
	}
	idealWeapon = w;
	weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
	UpdateHudWeapon();
}
*/
//ff1.3 end

/*
===============
idPlayer::NextWeapon
===============
*/
void idPlayer::NextWeapon( void ) {

	const char *weap;
	int w;

	if ( !weaponEnabled || spectating || hiddenWeapon || gameLocal.inCinematic || gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	// check if we have any weapons
	if ( !inventory.weapons ) {
		return;
	}

	w = idealWeapon;
	while( 1 ) {
		w++;
		if ( w >= MAX_WEAPONS ) {
			w = 0;
		}
        if( w == idealWeapon ){
            return;
        }
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( !spawnArgs.GetBool( va( "weapon%d_cycle", w ) ) ) {
			continue;
		}
		if ( !weap[ 0 ] ) {
			continue;
		}
		if ( ( inventory.weapons & ( 1 << w ) ) == 0 ) {
			continue;
		}

#ifdef _D3XP
		if ( inventory.HasAmmo( weap, true, this ) || inventory.HasPwAmmo(w) ) { //ff1.1 was w == weapon_bloodstone
#else
		if ( inventory.HasAmmo( weap ) ) {
#endif
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		/* ff1.3 - was:
		UpdateHudWeapon();
		ShowWeaponOverview(); //ff1.3
		*/
	}
	//ff1.3 start
	UpdateHudWeapon();
	if ( w != weapon_pda ) {
		ShowWeaponOverview();
	}
	//ff1.3 end
}

/*
===============
idPlayer::PrevWeapon
===============
*/
void idPlayer::PrevWeapon( void ) {

	const char *weap;
	int w;

	if ( !weaponEnabled || spectating || hiddenWeapon || gameLocal.inCinematic || gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || health < 0 ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	// check if we have any weapons
	if ( !inventory.weapons ) {
		return;
	}

	w = idealWeapon;
	while( 1 ) {
		w--;
		if ( w < 0 ) {
			w = MAX_WEAPONS - 1;
		}
		if( w == idealWeapon ){
			return;
		}
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( !spawnArgs.GetBool( va( "weapon%d_cycle", w ) ) ) {
			continue;
		}
		if ( !weap[ 0 ] ) {
			continue;
		}
		if ( ( inventory.weapons & ( 1 << w ) ) == 0 ) {
			continue;
		}
#ifdef _D3XP
		if ( inventory.HasAmmo( weap, true, this ) || inventory.HasPwAmmo(w) ) { //ff1.1 was w == weapon_bloodstone
#else
		if ( inventory.HasAmmo( weap ) ) {
#endif
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		/* ff1.3 - was:
		UpdateHudWeapon();
		ShowWeaponOverview(); //ff1.3
		*/
	}
	//ff1.3 start
	UpdateHudWeapon();
	if ( w != weapon_pda ) {
		ShowWeaponOverview();
	}
	//ff1.3 end
}

/*
===============
idPlayer::SelectWeapon
===============
*/
void idPlayer::SelectWeapon( int num, bool force ) {
	const char *weap;

	if ( !weaponEnabled || spectating || gameLocal.inCinematic || health < 0 ) {
		return;
	}

	if ( ( num < 0 ) || ( num >= MAX_WEAPONS ) ) {
		return;
	}

	if ( gameLocal.isClient ) {
		return;
	}

	if ( ( num != weapon_pda ) && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) {
		num = weapon_fists;
		hiddenWeapon ^= 1;
		if ( hiddenWeapon && weapon.GetEntity() ) {
			weapon.GetEntity()->LowerWeapon();
		} else {
			weapon.GetEntity()->RaiseWeapon();
		}
	}

	weap = spawnArgs.GetString( va( "def_weapon%d", num ) );
	if ( !weap[ 0 ] ) {
		gameLocal.Printf( "Invalid weapon\n" );
		return;
	}

#ifdef _D3XP
	//Is the weapon a toggle weapon
	WeaponToggle_t* weaponToggle;
	if(weaponToggles.Get(va("weapontoggle%d", num), &weaponToggle)) {

		int weaponToggleIndex = 0;

		//Find the current Weapon in the list
		int currentIndex = -1;
		for(int i = 0; i < weaponToggle->toggleList.Num(); i++) {
			if(weaponToggle->toggleList[i] == idealWeapon) {
				currentIndex = i;
				break;
			}
		}
		/*
		//ff1.3 start
		if (currentIndex == -1 && weaponToggle->lastSelected != -1) {
			//Didn't find the current weapon so select the last selected item in this group
			num = weaponToggle->lastSelected;
			gameLocal.Printf("weaponToggleIndex = weaponToggle->lastSelected\n");
		} else {
		//ff1.3 end
		*/
			if(currentIndex == -1) {
				//Didn't find the current weapon so select the first item
				weaponToggleIndex = 0;
			} else {
				//Roll to the next available item in the list
				weaponToggleIndex = currentIndex;
				weaponToggleIndex++;
				if(weaponToggleIndex >= weaponToggle->toggleList.Num()) {
					weaponToggleIndex = 0;
				}
			}

			for(int i = 0; i < weaponToggle->toggleList.Num(); i++) {

				//Is it available
				if(inventory.weapons & ( 1 << weaponToggle->toggleList[weaponToggleIndex])) {
					break;
				}

				weaponToggleIndex++;
				if(weaponToggleIndex >= weaponToggle->toggleList.Num()) {
					weaponToggleIndex = 0;
				}
			}

			num = weaponToggle->toggleList[weaponToggleIndex];
		//}//ff1.3
	}
#endif

	if ( force || ( inventory.weapons & ( 1 << num ) ) ) {
#ifdef _D3XP
		if ( !inventory.HasAmmo( weap, true, this )
			&& /*ff1.1*/ !inventory.HasPwAmmo(num)
			&& !spawnArgs.GetBool( va( "weapon%d_allowempty", num ) ) ) {
#else
		if ( !inventory.HasAmmo( weap ) && !spawnArgs.GetBool( va( "weapon%d_allowempty", num ) ) ) {
#endif
			return;
		}
		if ( ( previousWeapon >= 0 ) && ( idealWeapon == num ) && ( spawnArgs.GetBool( va( "weapon%d_toggle", num ) ) ) ) {
			weap = spawnArgs.GetString( va( "def_weapon%d", previousWeapon ) );
#ifdef _D3XP
			if ( !inventory.HasAmmo( weap, true, this )
				&& /*ff1.1*/ !inventory.HasPwAmmo(previousWeapon)
				&& !spawnArgs.GetBool( va( "weapon%d_allowempty", previousWeapon ) ) ) {
#else
			if ( !inventory.HasAmmo( weap ) && !spawnArgs.GetBool( va( "weapon%d_allowempty", previousWeapon ) ) ) {
#endif
				return;
			}
			//ff1.3 start - previousWeapon may have been removed!
			if ( !(inventory.weapons & ( 1 << previousWeapon )) ){
				return;
			}
			//ff1.3 end
			idealWeapon = previousWeapon;
		} else if ( ( weapon_pda >= 0 ) && ( num == weapon_pda ) && ( inventory.pdas.Num() == 0 ) ) {
			ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_noPDA" ), true );
			return;
		} else {
			idealWeapon = num;
		}
		UpdateHudWeapon();
	}

	//ff1.3 start
	/*
	if ( weaponToggle != NULL ) {
		weaponToggle->lastSelected = num;
	}
	*/
	if ( num != weapon_pda ) {
		ShowWeaponOverview();
	}
	//ff1.3 end
}

/*
=================
idPlayer::DropWeapon
=================
*/
void idPlayer::DropWeapon( bool died ) {
	idVec3 forward, up;
	int inclip, ammoavailable;

	assert( !gameLocal.isClient );

	if ( spectating || weaponGone || weapon.GetEntity() == NULL ) {
		return;
	}

	if ( ( !died && !weapon.GetEntity()->IsReady() ) || weapon.GetEntity()->IsReloading() ) {
		return;
	}
	// ammoavailable is how many shots we can fire
	// inclip is which amount is in clip right now
	ammoavailable = weapon.GetEntity()->AmmoAvailable();
	inclip = weapon.GetEntity()->AmmoInClip();

	// don't drop a grenade if we have none left
	if ( !idStr::Icmp( idWeapon::GetAmmoNameForNum( weapon.GetEntity()->GetAmmoType() ), "ammo_grenades" ) && ( ammoavailable - inclip <= 0 ) ) {
		return;
	}

#ifdef _D3XP
	ammoavailable += inclip;
#endif

	// expect an ammo setup that makes sense before doing any dropping
	// ammoavailable is -1 for infinite ammo, and weapons like chainsaw
	// a bad ammo config usually indicates a bad weapon state, so we should not drop
	// used to be an assertion check, but it still happens in edge cases

	if ( ( ammoavailable != -1 ) && ( ammoavailable < 0 ) ) {
		common->DPrintf( "idPlayer::DropWeapon: bad ammo setup\n" );
		return;
	}
	idEntity *item = NULL;
	if ( died ) {
		// ain't gonna throw you no weapon if I'm dead
		item = weapon.GetEntity()->DropItem( vec3_origin, 0, WEAPON_DROP_TIME, died );
	} else {
		viewAngles.ToVectors( &forward, NULL, &up );
		item = weapon.GetEntity()->DropItem( 250.0f * forward + 150.0f * up, 500, WEAPON_DROP_TIME, died );
	}
	if ( !item ) {
		return;
	}
	// set the appropriate ammo in the dropped object
	const idKeyValue * keyval = item->spawnArgs.MatchPrefix( "inv_ammo_" );
	if ( keyval ) {
		item->spawnArgs.SetInt( keyval->GetKey(), ammoavailable );
		idStr inclipKey = keyval->GetKey();
		inclipKey.Insert( "inclip_", 4 );
#ifdef _D3XP
		inclipKey.Insert( va("%.2d", currentWeapon), 11);
#endif
		item->spawnArgs.SetInt( inclipKey, inclip );
	}
	if ( !died ) {
		// remove from our local inventory completely
		inventory.Drop( spawnArgs, item->spawnArgs.GetString( "inv_weapon" ), -1 );
		weapon.GetEntity()->ResetAmmoClip();
		NextWeapon();
		weapon.GetEntity()->WeaponStolen();
		weaponGone = true;
	}
}

/*
=================
idPlayer::StealWeapon
steal the target player's current weapon
=================
*/
void idPlayer::StealWeapon( idPlayer *player ) {
	assert( !gameLocal.isClient );

	// make sure there's something to steal
	idWeapon *player_weapon = static_cast< idWeapon * >( player->weapon.GetEntity() );
	if ( !player_weapon || !player_weapon->CanDrop() || weaponGone ) {
		return;
	}
	// steal - we need to effectively force the other player to abandon his weapon
	int newweap = player->currentWeapon;
	if ( newweap == -1 ) {
		return;
	}
	// might be just dropped - check inventory
	if ( ! ( player->inventory.weapons & ( 1 << newweap ) ) ) {
		return;
	}
	const char *weapon_classname = spawnArgs.GetString( va( "def_weapon%d", newweap ) );
	assert( weapon_classname );
	int ammoavailable = player->weapon.GetEntity()->AmmoAvailable();
	int inclip = player->weapon.GetEntity()->AmmoInClip();

#ifdef _D3XP
	ammoavailable += inclip;
#endif

	if ( ( ammoavailable != -1 ) && ( ammoavailable < 0 ) ) {
		// see DropWeapon
		common->DPrintf( "idPlayer::StealWeapon: bad ammo setup\n" );
		// we still steal the weapon, so let's use the default ammo levels
		inclip = -1;
		const idDeclEntityDef *decl = gameLocal.FindEntityDef( weapon_classname );
		assert( decl );
		const idKeyValue *keypair = decl->dict.MatchPrefix( "inv_ammo_" );
		assert( keypair );
		ammoavailable = atoi( keypair->GetValue() );
	}

	player->weapon.GetEntity()->WeaponStolen();
	player->inventory.Drop( player->spawnArgs, NULL, newweap );
	player->SelectWeapon( weapon_fists, false );
	// in case the robbed player is firing rounds with a continuous fire weapon like the chaingun/plasma etc.
	// this will ensure the firing actually stops
	player->weaponGone = true;

	// give weapon, setup the ammo count
	Give( "weapon", weapon_classname );
	ammo_t ammo_i = player->inventory.AmmoIndexForWeaponClass( weapon_classname, NULL );
	idealWeapon = newweap;
	inventory.ammo[ ammo_i ] += ammoavailable;

#ifndef _D3XP
	inventory.clip[ newweap ] = inclip;
#endif
}

/*
===============
idPlayer::ActiveGui
===============
*/
idUserInterface *idPlayer::ActiveGui( void ) {
	if ( objectiveSystemOpen ) {
		return objectiveSystem;
	}

	if ( IsRiding() ) { //ff1.1 - disable guis
		return NULL;
	}

	return focusUI;
}

/*
===============
idPlayer::Weapon_Combat
===============
*/
void idPlayer::Weapon_Combat( void ) {
	if ( influenceActive || !weaponEnabled || gameLocal.inCinematic || privateCameraView ) {
		return;
	}

	weapon.GetEntity()->RaiseWeapon();
	if ( weapon.GetEntity()->IsReloading() ) {
		if ( !AI_RELOAD ) {
			AI_RELOAD = true;
			SetState( "ReloadWeapon" );
			UpdateScript();
		}
	} else {
		AI_RELOAD = false;
	}

	//ff1.3 start
	/*
	if ( idealWeapon == weapon_soulcube && soulCubeProjectile.GetEntity() != NULL ) {
		idealWeapon = currentWeapon;
	}
	*/
	//ff1.3 end

	if ( idealWeapon != currentWeapon ) {
		//ff1.3 start
		if ( idealWeapon != weapon_shockrifle && painKillerProjectile.GetEntity() != NULL ) {
			 painKillerProjectile.GetEntity()->StartReturnPhase();
		}
		//ff1.3 end

		if ( weaponCatchup ) {
			assert( gameLocal.isClient );

			currentWeapon = idealWeapon;
			weaponGone = false;
			animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
			weapon.GetEntity()->GetWeaponDef( animPrefix, inventory.clip[ currentWeapon ] );
			animPrefix.Strip( "weapon_" );

			weapon.GetEntity()->NetCatchup();
			const function_t *newstate = GetScriptFunction( "NetCatchup" );
			if ( newstate ) {
				SetState( newstate );
				UpdateScript();
			}
			weaponCatchup = false;
		} else {
			if ( weapon.GetEntity()->IsReady() ) {
				weapon.GetEntity()->PutAway();
			}

			if ( weapon.GetEntity()->IsHolstered() ) {
				assert( idealWeapon >= 0 );
				assert( idealWeapon < MAX_WEAPONS );

				if ( currentWeapon != weapon_pda && !spawnArgs.GetBool( va( "weapon%d_toggle", currentWeapon ) ) ) {
					previousWeapon = currentWeapon;
				}
				currentWeapon = idealWeapon;
				weaponGone = false;
				animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
				weapon.GetEntity()->GetWeaponDef( animPrefix, inventory.clip[ currentWeapon ] );
				animPrefix.Strip( "weapon_" );

				weapon.GetEntity()->Raise();
			}
		}
	} else {
		weaponGone = false;	// if you drop and re-get weap, you may miss the = false above
		if ( weapon.GetEntity()->IsHolstered() ) {
			if ( !weapon.GetEntity()->AmmoAvailable() && !weapon.GetEntity()->PwAmmoAvailable() ) { //ff1.1
				// weapons can switch automatically if they have no more ammo
				NextBestWeapon();
			} else {
				weapon.GetEntity()->Raise();
				state = GetScriptFunction( "RaiseWeapon" );
				if ( state ) {
					SetState( state );
				}
			}
		}
	}

	// check for attack
	AI_WEAPON_FIRED = false;

	if ( !influenceActive ) {
		if ( ( usercmd.buttons & BUTTON_ATTACK ) && !weaponGone ) {
			FireWeapon( !(oldButtons & BUTTON_ATTACK) ); // The condition holds True when key is being tapped rather than held
		} else if ( oldButtons & BUTTON_ATTACK ) {
			AI_ATTACK_HELD = false;
			weapon.GetEntity()->EndAttack();
		}

		//ff1.3 start
		// check for Weapon special function, new
		if ( ( usercmd.buttons & BUTTON_5 ) && !weaponGone ) {  // BUTTON_5 is being used for weapon special function
			WeaponSpecialFunction( !(oldButtons & BUTTON_5) ); // The condition holds True when key is being tapped rather than held
		} else if ( oldButtons & BUTTON_5 ) {
			weapon.GetEntity()->EndSpecialFunction();
		}
		//ff1.3 end
	}

	// update our ammo clip in our inventory
	if ( ( currentWeapon >= 0 ) && ( currentWeapon < MAX_WEAPONS ) ) {
		//gameLocal.Printf( "inventory.clip[ %d ] = %d\n", currentWeapon, weapon.GetEntity()->AmmoInClip() ); //temp
		inventory.clip[ currentWeapon ] = weapon.GetEntity()->AmmoInClip();
		if ( hud && ( currentWeapon == idealWeapon ) ) {
			UpdateHudAmmo( hud );
		}
	}
}

/*
===============
idPlayer::Weapon_NPC
===============
*/
void idPlayer::Weapon_NPC( void ) {
	if ( idealWeapon != currentWeapon ) {
		Weapon_Combat();
	}
	StopFiring();
	weapon.GetEntity()->LowerWeapon();

	if ( ( usercmd.buttons & BUTTON_ATTACK ) && !( oldButtons & BUTTON_ATTACK ) ) {
		buttonMask |= BUTTON_ATTACK;
		focusCharacter->TalkTo( this );
	}
}

//ff1.3 start

/*
===============
idPlayer::Weapon_GameCover
===============
*/
void idPlayer::Weapon_GameCover( void ) {
	if ( idealWeapon != currentWeapon ) {
		Weapon_Combat();
	}
	StopFiring();
	weapon.GetEntity()->LowerWeapon();

	if ( ( usercmd.buttons & BUTTON_ATTACK ) && !( oldButtons & BUTTON_ATTACK ) ) {
		buttonMask |= BUTTON_ATTACK;
		focusGameCover->GiveTo( this );
	}
}

/*
===============
idPlayer::LanchPossessionProjectile
===============

idProjectile * idPlayer::LanchPossessionProjectile( idActor *targetEnt ){
	idPossessionProjectile	*proj;
	idEntity		*ent;
	idDict			projectileDict;

	gameLocal.Printf("idPlayer::LanchPossessionProjectile\n");

	const idDeclEntityDef *projectileDef = gameLocal.FindEntityDef( "projectile_soulpossession", false );
	if ( !projectileDef ) {
		gameLocal.Warning( "No def 'projectile_soulpossession' found");
		return NULL;
	}

	gameLocal.SpawnEntityDef( projectileDef->dict, &ent, false );

	if ( !ent || !ent->IsType( idPossessionProjectile::Type ) ) {
		gameLocal.Error( "'projectile_soulpossession' is not an idPossessionProjectile" );
	}

	StartSound( "snd_ride_fire", SND_CHANNEL_ANY, 0, false, NULL );

	idVec3 pos = GetEyePosition();
	idVec3 dir = viewAngles.ToForward();
	proj = static_cast<idPossessionProjectile *>(ent);
	proj->Create( this, pos, dir );
	proj->SetEnemy( targetEnt );
	proj->Launch( pos, dir, vec3_origin, 0.0f, 1.0f, 1.0f );
	return proj;
}
*/

//ff1.3 end

/*
===============
idPlayer::LowerWeapon
===============
*/
void idPlayer::LowerWeapon( void ) {
	if ( weapon.GetEntity() && !weapon.GetEntity()->IsHidden() ) {
		weapon.GetEntity()->LowerWeapon();
	}
}

/*
===============
idPlayer::RaiseWeapon
===============
*/
void idPlayer::RaiseWeapon( void ) {
	if ( weapon.GetEntity() && weapon.GetEntity()->IsHidden() ) {
		weapon.GetEntity()->RaiseWeapon();
	}
}

/*
===============
idPlayer::WeaponLoweringCallback
===============
*/
void idPlayer::WeaponLoweringCallback( void ) {
	SetState( "LowerWeapon" );
	UpdateScript();
}

/*
===============
idPlayer::WeaponRisingCallback
===============
*/
void idPlayer::WeaponRisingCallback( void ) {
	SetState( "RaiseWeapon" );
	UpdateScript();
}

/*
===============
idPlayer::Weapon_GUI
===============
*/
void idPlayer::Weapon_GUI( void ) {

	if ( !objectiveSystemOpen ) {
		if ( idealWeapon != currentWeapon ) {
			Weapon_Combat();
		}
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
	}

	// disable click prediction for the GUIs. handy to check the state sync does the right thing
	if ( gameLocal.isClient && !net_clientPredictGUI.GetBool() ) {
		return;
	}

	if ( ( oldButtons ^ usercmd.buttons ) & BUTTON_ATTACK ) {
		sysEvent_t ev;
		const char *command = NULL;
		bool updateVisuals = false;

		idUserInterface *ui = ActiveGui();
		if ( ui ) {
			ev = sys->GenerateMouseButtonEvent( 1, ( usercmd.buttons & BUTTON_ATTACK ) != 0 );
			command = ui->HandleEvent( &ev, gameLocal.time, &updateVisuals );
			if ( updateVisuals && focusGUIent && ui == focusUI ) {
				focusGUIent->UpdateVisuals();
			}
		}
		if ( gameLocal.isClient ) {
			// we predict enough, but don't want to execute commands
			return;
		}
		if ( focusGUIent ) {
			HandleGuiCommands( focusGUIent, command );
		} else {
			HandleGuiCommands( this, command );
		}
	}
}

/*
===============
idPlayer::UpdateWeapon
===============
*/
void idPlayer::UpdateWeapon( void ) {
	if ( health <= 0 ) {
		return;
	}

	assert( !spectating );

	if ( gameLocal.isClient ) {
		// clients need to wait till the weapon and it's world model entity
		// are present and synchronized ( weapon.worldModel idEntityPtr to idAnimatedEntity )
		if ( !weapon.GetEntity()->IsWorldModelReady() ) {
			return;
		}
	}

	// always make sure the weapon is correctly setup before accessing it
	if ( !weapon.GetEntity()->IsLinked() ) {
		if ( idealWeapon != -1 ) {
			animPrefix = spawnArgs.GetString( va( "def_weapon%d", idealWeapon ) );
			weapon.GetEntity()->GetWeaponDef( animPrefix, inventory.clip[ idealWeapon ] );
			assert( weapon.GetEntity()->IsLinked() );
		} else {
			return;
		}
	}

	/* ivan - removed
	if ( hiddenWeapon && tipUp && usercmd.buttons & BUTTON_ATTACK ) {
		HideTip();
	}
    */
	if ( g_dragEntity.GetBool() ) {
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
		dragEntity.Update( this );
	} else if ( ActiveGui() ) {
		// gui handling overrides weapon use
		Weapon_GUI();
	} else	if ( focusCharacter && ( focusCharacter->health > 0 ) ) {
		Weapon_NPC();
	} else if ( focusGameCover ) {
		Weapon_GameCover();
	} else if ( inGameStatsVisible ) {
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
	} else {
		Weapon_Combat();
	}

	if ( hiddenWeapon ) {
		weapon.GetEntity()->LowerWeapon();
	}

	// update weapon state, particles, dlights, etc
	weapon.GetEntity()->PresentWeapon( showWeaponViewModel );
}

/*
===============
idPlayer::SpectateFreeFly
===============
*/
void idPlayer::SpectateFreeFly( bool force ) {
	idPlayer	*player;
	idVec3		newOrig;
	idVec3		spawn_origin;
	idAngles	spawn_angles;

	player = gameLocal.GetClientByNum( spectator );
	if ( force || gameLocal.time > lastSpectateChange ) {
		spectator = entityNumber;
		if ( player && player != this && !player->spectating && !player->IsInTeleport() ) {
			newOrig = player->GetPhysics()->GetOrigin();
			if ( player->physicsObj.IsCrouching() ) {
				newOrig[ 2 ] += pm_crouchviewheight.GetFloat();
			} else {
				newOrig[ 2 ] += pm_normalviewheight.GetFloat();
			}
			newOrig[ 2 ] += SPECTATE_RAISE;
			idBounds b = idBounds( vec3_origin ).Expand( pm_spectatebbox.GetFloat() * 0.5f );
			idVec3 start = player->GetPhysics()->GetOrigin();
			start[2] += pm_spectatebbox.GetFloat() * 0.5f;
			trace_t t;
			// assuming spectate bbox is inside stand or crouch box
			gameLocal.clip.TraceBounds( t, start, newOrig, b, MASK_PLAYERSOLID, player );
			newOrig.Lerp( start, newOrig, t.fraction );
			SetOrigin( newOrig );
			idAngles angle = player->viewAngles;
			angle[ 2 ] = 0;
			SetViewAngles( angle );
		} else {
			SelectInitialSpawnPoint( spawn_origin, spawn_angles );
			spawn_origin[ 2 ] += pm_normalviewheight.GetFloat();
			spawn_origin[ 2 ] += SPECTATE_RAISE;
			SetOrigin( spawn_origin );
			SetViewAngles( spawn_angles );
		}
		lastSpectateChange = gameLocal.time + 500;
	}
}

/*
===============
idPlayer::SpectateCycle
===============
*/
void idPlayer::SpectateCycle( void ) {
	idPlayer *player;

	if ( gameLocal.time > lastSpectateChange ) {
		int latchedSpectator = spectator;
		spectator = gameLocal.GetNextClientNum( spectator );
		player = gameLocal.GetClientByNum( spectator );
		assert( player ); // never call here when the current spectator is wrong
		// ignore other spectators
		while ( latchedSpectator != spectator && player->spectating ) {
			spectator = gameLocal.GetNextClientNum( spectator );
			player = gameLocal.GetClientByNum( spectator );
		}
		lastSpectateChange = gameLocal.time + 500;
	}
}

/*
===============
idPlayer::UpdateSpectating
===============
*/
void idPlayer::UpdateSpectating( void ) {
	assert( spectating );
	assert( !gameLocal.isClient );
	assert( IsHidden() );
	idPlayer *player;
	if ( !gameLocal.isMultiplayer ) {
		return;
	}
	player = gameLocal.GetClientByNum( spectator );
	if ( !player || ( player->spectating && player != this ) ) {
		SpectateFreeFly( true );
	} else if ( usercmd.upmove > 0 ) {
		SpectateFreeFly( false );
	} else if ( usercmd.buttons & BUTTON_ATTACK ) {
		SpectateCycle();
	}
}

/*
===============
idPlayer::HandleSingleGuiCommand
===============
*/
bool idPlayer::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token;

	if ( !src->ReadToken( &token ) ) {
		return false;
	}

	if ( token == ";" ) {
		return false;
	}

	if ( token.Icmp( "addhealth" ) == 0 ) {
		//ff1.3 start
		if ( entityGui && health < inventory.maxHealth ) {
			int healthNeeded = inventory.maxHealth - health;
			if ( healthNeeded > 0 ) {
				int _health = entityGui->spawnArgs.GetInt( "gui_parm1" );
				int amt = ( _health >= HEALTH_PER_DOSE ) ? HEALTH_PER_DOSE : _health;
				if ( amt > healthNeeded ) {
					amt = healthNeeded;
				}
				_health -= amt;
				entityGui->spawnArgs.SetInt( "gui_parm1", _health );
				if ( entityGui->GetRenderEntity() && entityGui->GetRenderEntity()->gui[ 0 ] ) {
					entityGui->GetRenderEntity()->gui[ 0 ]->SetStateInt( "gui_parm1", _health );
				}
				health += amt;
				if ( health > inventory.maxHealth ) {
					health = inventory.maxHealth;
				}
			}
		}

		/* was:
		if ( entityGui && health < 100 ) {
			int healthNeeded = inventory.maxHealth - health;
			if ( healthNeeded > 0 ) {
				int _health = entityGui->spawnArgs.GetInt( "gui_parm1" );
				int amt = ( _health >= HEALTH_PER_DOSE ) ? HEALTH_PER_DOSE : _health;
				_health -= amt;
				entityGui->spawnArgs.SetInt( "gui_parm1", _health );
				if ( entityGui->GetRenderEntity() && entityGui->GetRenderEntity()->gui[ 0 ] ) {
					entityGui->GetRenderEntity()->gui[ 0 ]->SetStateInt( "gui_parm1", _health );
				}
				health += amt;
				if ( health > 100 ) {
					health = 100;
				}
			}
		}
		*/
		//ff1.3 end
		return true;
	}

	if ( token.Icmp( "ready" ) == 0 ) {
		PerformImpulse( IMPULSE_17 );
		return true;
	}

	if ( token.Icmp( "updatepda" ) == 0 ) {
		UpdatePDAInfo( true );
		return true;
	}

	if ( token.Icmp( "updatepda2" ) == 0 ) {
		UpdatePDAInfo( false );
		return true;
	}

	if ( token.Icmp( "stoppdavideo" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaVideoWave.Length() > 0 ) {
			StopSound( SND_CHANNEL_PDA, false );
		}
		return true;
	}

	if ( token.Icmp( "close" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen ) {
			TogglePDA();
		}
	}

	if ( token.Icmp( "playpdavideo" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaVideo.Length() > 0 ) {
			const idMaterial *mat = declManager->FindMaterial( pdaVideo );
			if ( mat ) {
				int c = mat->GetNumStages();
				for ( int i = 0; i < c; i++ ) {
					const shaderStage_t *stage = mat->GetStage(i);
					if ( stage && stage->texture.cinematic ) {
						stage->texture.cinematic->ResetTime( gameLocal.time );
					}
				}
				if ( pdaVideoWave.Length() ) {
					const idSoundShader *shader = declManager->FindSound( pdaVideoWave );
					StartSoundShader( shader, SND_CHANNEL_PDA, 0, false, NULL );
				}
			}
		}
	}

	if ( token.Icmp( "playpdaaudio" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaAudio.Length() > 0 ) {
			const idSoundShader *shader = declManager->FindSound( pdaAudio );
			int ms;
			StartSoundShader( shader, SND_CHANNEL_PDA, 0, false, &ms );
			StartAudioLog();
			CancelEvents( &EV_Player_StopAudioLog );
			PostEventMS( &EV_Player_StopAudioLog, ms + 150 );
		}
		return true;
	}

	if ( token.Icmp( "stoppdaaudio" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaAudio.Length() > 0 ) {
			// idSoundShader *shader = declManager->FindSound( pdaAudio );
			StopAudioLog();
			StopSound( SND_CHANNEL_PDA, false );
		}
		return true;
	}

	//ff1.3 start
	if ( token.Icmp( "prevcover" ) == 0 ) {
		if ( selectedGameCoverIndex > 0 ) {
			selectedGameCoverIndex--;
			UpdatePDAInfo( false );
		}
		return true;
	}

	if ( token.Icmp( "nextcover" ) == 0 ) {
		if ( selectedGameCoverIndex < inventory.gameCovers.Num()-1 ) {
			selectedGameCoverIndex++;
			UpdatePDAInfo( false );
		}
		return true;
	}

	if ( token.Icmp( "prevstats" ) == 0 ) {
		if ( selectedStatsIndex > 0 ) {
			selectedStatsIndex--;
			UpdatePDAInfo( false );
		}
		return true;
	}

	if ( token.Icmp( "nextstats" ) == 0 ) {
		if ( selectedStatsIndex < CampaignStatsIndex() ) { //reserve 1 extra for campaign stats
			selectedStatsIndex++;
			UpdatePDAInfo( false );
		}
		return true;
	}
	//ff1.3 end

	src->UnreadToken( &token );
	return false;
}

/*
==============
idPlayer::Collide
==============
*/
bool idPlayer::Collide( const trace_t &collision, const idVec3 &velocity ) {
	idEntity *other;

	if ( gameLocal.isClient ) {
		return false;
	}

	other = gameLocal.entities[ collision.c.entityNum ];
	if ( other ) {
		other->Signal( SIG_TOUCH );
		if ( !spectating ) {
			if ( other->RespondsTo( EV_Touch ) ) {
				other->ProcessEvent( &EV_Touch, this, &collision );
			}
		} else {
			if ( other->RespondsTo( EV_SpectatorTouch ) ) {
				other->ProcessEvent( &EV_SpectatorTouch, this, &collision );
			}
		}
	}
	return false;
}


/*
================
idPlayer::UpdateLocation

Searches nearby locations
================
*/
void idPlayer::UpdateLocation( void ) {
	if ( hud ) {
		idLocationEntity *locationEntity = gameLocal.LocationForPoint( GetEyePosition() );
		if ( locationEntity ) {
			hud->SetStateString( "location", locationEntity->GetLocation() );
		} else {
			hud->SetStateString( "location", common->GetLanguageDict()->GetString( "#str_02911" ) );
		}
	}
}

//ff1.3 start
/*
================
idPlayer::GetAimTarget
================
*/
idEntity *idPlayer::GetAimTarget( float range ) {
	trace_t	trace;
	idVec3 start, end;
	idEntity *ent;
	idEntity *master;

	start = firstPersonViewOrigin;
	end = start + firstPersonViewAxis[0] * range ;

	idBounds bounds;
	bounds.Zero();
	bounds.ExpandSelf( 10.0f );

	gameLocal.clip.TraceBounds( trace, start, end, bounds, MASK_SHOT_RENDERMODEL, this );

	if ( trace.fraction < 1.0f ) {
		ent = gameLocal.entities[ trace.c.entityNum ];
		if ( ent && !ent->IsType( idAI::Type ) ) {
			master = ent->GetBindMaster();
			if ( master ) {
				ent = master;
			}
		}
	} else {
		ent = NULL;
	}

	if ( g_debugWeapon.GetBool() ) {
		gameRenderWorld->DebugLine( colorYellow, start, end, 100 );
		if ( ent ) {
			gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds(), ent->GetPhysics()->GetOrigin(), 100 );
		}
	}

	return ent;
}

//ff1-3 end

/*
================
idPlayer::ClearFocus

Clears the focus cursor
================
*/
void idPlayer::ClearFocus( void ) {
	focusCharacter	= NULL;
	focusGUIent		= NULL;
	focusUI			= NULL;
	focusVehicle	= NULL;
	focusGameCover	= NULL; //ff1.3
	talkCursor		= CURSOR_STATE_HIDDEN;
}

/*
================
idPlayer::UpdateFocus

Searches nearby entities for interactive guis, possibly making one of them
the focus and sending it a mouse move event
================
*/
void idPlayer::UpdateFocus( void ) {
	idClipModel *clipModelList[ MAX_GENTITIES ];
	idClipModel *clip;
	int			listedClipModels;
	idEntity	*oldFocus;
	idEntity	*ent;
	idUserInterface *oldUI;
	idAI		*oldChar;
	int			oldTalkCursor;
	idGameCover *oldGameCover;
	int			i, j;
	idVec3		start, end;
	bool		allowFocus;
	const char *command;
	trace_t		trace;
	guiPoint_t	pt;
	const idKeyValue *kv;
	sysEvent_t	ev;
	idUserInterface *ui;

	if ( gameLocal.inCinematic ) {
		return;
	}

	// only update the focus character when attack button isn't pressed so players
	// can still chainsaw NPC's
	if ( gameLocal.isMultiplayer || ( !focusCharacter && ( usercmd.buttons & BUTTON_ATTACK ) ) ) {
		allowFocus = false;
	} else {
		allowFocus = true;
	}

	oldFocus		= focusGUIent;
	oldUI			= focusUI;
	oldChar			= focusCharacter;
	oldTalkCursor	= talkCursor;
    oldGameCover	= focusGameCover;

    if ( privateCameraView || IsRiding() ) { //ff1.3
        ClearFocus(); //clear current focus entity
    } else {
		if ( focusTime <= gameLocal.time ) {
			ClearFocus();
		}

		// don't let spectators interact with GUIs
		if ( spectating ) {
			return;
		}

		start = GetEyePosition();
		end = start + viewAngles.ToForward() * 80.0f;

		// player identification -> names to the hud
		if ( gameLocal.isMultiplayer && entityNumber == gameLocal.localClientNum ) {
			idVec3 end = start + viewAngles.ToForward() * 768.0f;
			gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
			int iclient = -1;
			if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum < MAX_CLIENTS ) ) {
				iclient = trace.c.entityNum;
			}
			if ( MPAim != iclient ) {
				lastMPAim = MPAim;
				MPAim = iclient;
				lastMPAimTime = gameLocal.realClientTime;
			}
		}

		idBounds bounds( start );
		bounds.AddPoint( end );

		listedClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

		// no pretense at sorting here, just assume that there will only be one active
		// gui within range along the trace
		for ( i = 0; i < listedClipModels; i++ ) {
			clip = clipModelList[ i ];
			ent = clip->GetEntity();

			if ( ent->IsHidden() ) {
				continue;
			}

			if ( allowFocus ) {
				if ( ent->IsType( idAFAttachment::Type ) ) {
					idEntity *body = static_cast<idAFAttachment *>( ent )->GetBody();
					if ( body && body->IsType( idAI::Type ) && ( static_cast<idAI *>( body )->GetTalkState() >= TALK_OK ) ) {
						gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
						if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
							ClearFocus();
							focusCharacter = static_cast<idAI *>( body );
							talkCursor = CURSOR_STATE_TALK;
							focusTime = gameLocal.time + FOCUS_TIME;
							break;
						}
					}
					continue;
				}

				if ( ent->IsType( idAI::Type ) ) {
					if ( static_cast<idAI *>( ent )->GetTalkState() >= TALK_OK ) {
						gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
						if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
							ClearFocus();
							focusCharacter = static_cast<idAI *>( ent );
							talkCursor = CURSOR_STATE_TALK;
							focusTime = gameLocal.time + FOCUS_TIME;
							break;
						}
					}
					continue;
				}

				if ( ent->IsType( idAFEntity_Vehicle::Type ) ) {
					//ff1.3 start
					//gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
					idBounds bounds;
					bounds.Zero();
					bounds.ExpandSelf( 20.0f );
					gameLocal.clip.TraceBounds( trace, start, end, bounds, MASK_SHOT_RENDERMODEL, this );
					//ff1.3 end
					if ( ( trace.fraction < 0.2f /*1.0f*/ ) && ( trace.c.entityNum == ent->entityNumber ) ) {
						ClearFocus();
						focusVehicle = static_cast<idAFEntity_Vehicle *>( ent );
						focusTime = gameLocal.time + FOCUS_TIME;
						break;
					}
					continue;
				}

				//ff1.3 start
				if ( ent->IsType( idGameCover::Type ) ) {
					gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
					if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
						ClearFocus();
						focusGameCover = static_cast<idGameCover *>( ent );
						talkCursor = CURSOR_STATE_TALK;
						focusTime = gameLocal.time + FOCUS_TIME;
						break;
					}
					continue;
				}
				//ff1.3 end
			}

			if ( !ent->GetRenderEntity() || !ent->GetRenderEntity()->gui[ 0 ] || !ent->GetRenderEntity()->gui[ 0 ]->IsInteractive() ) {
				continue;
			}

			if ( ent->spawnArgs.GetBool( "inv_item" ) ) {
				// don't allow guis on pickup items focus
				continue;
			}

			pt = gameRenderWorld->GuiTrace( ent->GetModelDefHandle(), start, end );
			if ( pt.x != -1 ) {
				// we have a hit
				renderEntity_t *focusGUIrenderEntity = ent->GetRenderEntity();
				if ( !focusGUIrenderEntity ) {
					continue;
				}

				if ( pt.guiId == 1 ) {
					ui = focusGUIrenderEntity->gui[ 0 ];
				} else if ( pt.guiId == 2 ) {
					ui = focusGUIrenderEntity->gui[ 1 ];
				} else {
					ui = focusGUIrenderEntity->gui[ 2 ];
				}

				if ( ui == NULL ) {
					continue;
				}

				ClearFocus();
				focusGUIent = ent;
				focusUI = ui;

				if ( oldFocus != ent ) {
					// new activation
					// going to see if we have anything in inventory a gui might be interested in
					// need to enumerate inventory items
					focusUI->SetStateInt( "inv_count", inventory.items.Num() );
					for ( j = 0; j < inventory.items.Num(); j++ ) {
						idDict *item = inventory.items[ j ];
						const char *iname = item->GetString( "inv_name" );
						const char *iicon = item->GetString( "inv_icon" );
						const char *itext = item->GetString( "inv_text" );

						focusUI->SetStateString( va( "inv_name_%i", j), iname );
						focusUI->SetStateString( va( "inv_icon_%i", j), iicon );
						focusUI->SetStateString( va( "inv_text_%i", j), itext );
						kv = item->MatchPrefix("inv_id", NULL);
						if ( kv ) {
							focusUI->SetStateString( va( "inv_id_%i", j ), kv->GetValue() );
						}
						focusUI->SetStateInt( iname, 1 );
					}


					for( j = 0; j < inventory.pdaSecurity.Num(); j++ ) {
						const char *p = inventory.pdaSecurity[ j ];
						if ( p && *p ) {
							focusUI->SetStateInt( p, 1 );
						}
					}

	#ifdef _D3XP		//BSM: Added for powercells
					int powerCellCount = 0;
					for ( j = 0; j < inventory.items.Num(); j++ ) {
						idDict *item = inventory.items[ j ];
						if (item->GetInt("inv_powercell")) {
							powerCellCount++;
						}
					}
					focusUI->SetStateInt( "powercell_count", powerCellCount );
	#endif

					//int staminapercentage = ( int )( 100.0f * stamina / pm_stamina.GetFloat() );
					focusUI->SetStateString( "player_health", va("%i", health ) );
					//focusUI->SetStateString( "player_stamina", va( "%i%%", staminapercentage ) );
					focusUI->SetStateString( "player_armor", va( "%i%%", inventory.armor ) );

					kv = focusGUIent->spawnArgs.MatchPrefix( "gui_parm", NULL );
					while ( kv ) {
						focusUI->SetStateString( kv->GetKey(), kv->GetValue() );
						kv = focusGUIent->spawnArgs.MatchPrefix( "gui_parm", kv );
					}
				}

				// clamp the mouse to the corner
				ev = sys->GenerateMouseMoveEvent( -2000, -2000 );
				command = focusUI->HandleEvent( &ev, gameLocal.time );
 				HandleGuiCommands( focusGUIent, command );

				// move to an absolute position
				ev = sys->GenerateMouseMoveEvent( pt.x * SCREEN_WIDTH, pt.y * SCREEN_HEIGHT );
				command = focusUI->HandleEvent( &ev, gameLocal.time );
				HandleGuiCommands( focusGUIent, command );
				focusTime = gameLocal.time + FOCUS_GUI_TIME;
				break;
			}
		}
	}

	if ( focusGUIent && focusUI ) {
		if ( !oldFocus || oldFocus != focusGUIent ) {
			command = focusUI->Activate( true, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			StartSound( "snd_guienter", SND_CHANNEL_ANY, 0, false, NULL );
			// HideTip();
			// HideObjective();
		}
	} else if ( oldFocus && oldUI ) {
		command = oldUI->Activate( false, gameLocal.time );
		HandleGuiCommands( oldFocus, command );
		StartSound( "snd_guiexit", SND_CHANNEL_ANY, 0, false, NULL );
	}

	if ( cursor && ( oldTalkCursor != talkCursor ) ) {
		cursor->SetStateInt( "talkcursor", talkCursor );
	}

	if ( oldChar != focusCharacter && hud ) {
		if ( focusCharacter ) {
			if ( focusCharacter->spawnArgs.GetBool( "showStatus", "0" ) ) { //ff1.1 - friends
				hud->SetStateString( "npc", "Status:" );
				hud->SetStateString( "npc_action", focusCharacter->spawnArgs.GetString( "shownState", "Inactive" ) );
			} else {
				hud->SetStateString( "npc", focusCharacter->spawnArgs.GetString( "npc_name", "Joe" ) );
#ifdef _D3XP
				//Use to code to update the npc action string to fix bug 1159
				hud->SetStateString( "npc_action", common->GetLanguageDict()->GetString( "#str_02036" ) );
#endif
			}
			hud->HandleNamedEvent( "showNPC" );
			// HideTip();
			// HideObjective();
		} else {
			hud->SetStateString( "npc", "" );
#ifdef _D3XP
			hud->SetStateString( "npc_action", "" );
#endif
			hud->HandleNamedEvent( "hideNPC" );
		}
	}
	//ff1.3 start
	else if ( oldGameCover != focusGameCover && hud ) {
		if ( focusGameCover ) {
			const idDict * cover = gameLocal.FindEntityDefDict( focusGameCover->spawnArgs.GetString( "def_cover" ), false );
			if ( cover ) {
				hud->SetStateString( "npc", cover->GetString("name") );
			} else {
				hud->SetStateString( "npc", "" );
			}
			hud->SetStateString( "npc_action", "Collect" );
			hud->HandleNamedEvent( "showNPC" );
		} else {
			hud->SetStateString( "npc", "" );
			hud->SetStateString( "npc_action", "" );
			hud->HandleNamedEvent( "hideNPC" );
		}
	}
	//ff1.3 end
}


/*
=================
idPlayer::CrashLand

Check for hard landings that generate sound events
=================
*/
void idPlayer::CrashLand( const idVec3 &oldOrigin, const idVec3 &oldVelocity ) {
	idVec3		origin, velocity;
	idVec3		gravityVector, gravityNormal;
	float		delta;
	float		hardDelta, fatalDelta;
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;
	waterLevel_t waterLevel;
	bool		noDamage;

	AI_SOFTLANDING = false;
	AI_HARDLANDING = false;

	// if the player is not on the ground
	if ( !physicsObj.HasGroundContacts() ) {
		return;
	}

	gravityNormal = physicsObj.GetGravityNormal();

	// if the player wasn't going down
	if ( ( oldVelocity * -gravityNormal ) >= 0.0f ) {
		return;
	}

	waterLevel = physicsObj.GetWaterLevel();

	// never take falling damage if completely underwater
	if ( waterLevel == WATERLEVEL_HEAD ) {
		return;
	}

	// no falling damage if touching a nodamage surface
	noDamage = false;
	for ( int i = 0; i < physicsObj.GetNumContacts(); i++ ) {
		const contactInfo_t &contact = physicsObj.GetContact( i );
		if ( contact.material->GetSurfaceFlags() & SURF_NODAMAGE ) {
			noDamage = true;
			StartSound( "snd_land_hard", SND_CHANNEL_ANY, 0, false, NULL );
			break;
		}
	}

	origin = GetPhysics()->GetOrigin();
	gravityVector = physicsObj.GetGravity();

	// calculate the exact velocity on landing
	dist = ( origin - oldOrigin ) * -gravityNormal;
	vel = oldVelocity * -gravityNormal;
	acc = -gravityVector.Length();

	a = acc / 2.0f;
	b = vel;
	c = -dist;

	den = b * b - 4.0f * a * c;
	if ( den < 0 ) {
		return;
	}
	t = ( -b - idMath::Sqrt( den ) ) / ( 2.0f * a );

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

	// reduce falling damage if there is standing water
	if ( waterLevel == WATERLEVEL_WAIST ) {
		delta *= 0.25f;
	}
	if ( waterLevel == WATERLEVEL_FEET ) {
		delta *= 0.5f;
	}

	if ( delta < 1.0f ) {
		return;
	}

	// allow falling a bit further for multiplayer
	if ( gameLocal.isMultiplayer ) {
		fatalDelta	= 75.0f;
		hardDelta	= 50.0f;
	} else {
		fatalDelta	= 65.0f;
		hardDelta	= 45.0f;
	}

	if ( delta > fatalDelta ) {
		AI_HARDLANDING = true;
		landChange = -32;
		landTime = gameLocal.time;
		if ( !noDamage ) {
			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_fatalfall", 1.0f, 0 );
		}
	} else if ( delta > hardDelta ) {
		AI_HARDLANDING = true;
		landChange	= -24;
		landTime	= gameLocal.time;
		if ( !noDamage ) {
			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_hardfall", 1.0f, 0 );
		}
	} else if ( delta > 30 ) {
		AI_HARDLANDING = true;
		landChange	= -16;
		landTime	= gameLocal.time;
		if ( !noDamage ) {
			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_softfall", 1.0f, 0 );
		}
	} else if ( delta > 7 ) {
		AI_SOFTLANDING = true;
		landChange	= -8;
		landTime	= gameLocal.time;
	} else if ( delta > 3 ) {
		// just walk on
	}
}

/*
===============
idPlayer::BobCycle
===============
*/
void idPlayer::BobCycle( const idVec3 &pushVelocity ) {
	float		bobmove;
	int			old, deltaTime;
	idVec3		vel, gravityDir, velocity;
	idMat3		viewaxis;
	float		bob;
	float		delta;
	float		speed;
	float		f;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	velocity = physicsObj.GetLinearVelocity() - pushVelocity;

	gravityDir = physicsObj.GetGravityNormal();
	vel = velocity - ( velocity * gravityDir ) * gravityDir;
	xyspeed = vel.LengthFast();

	// do not evaluate the bob for other clients
	// when doing a spectate follow, don't do any weapon bobbing
	if ( gameLocal.isClient && entityNumber != gameLocal.localClientNum ) {
		viewBobAngles.Zero();
		viewBob.Zero();
		return;
	}

	if ( !physicsObj.HasGroundContacts() || influenceActive == INFLUENCE_LEVEL2 || ( gameLocal.isMultiplayer && spectating ) ) {
		// airborne
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	} else if ( ( !usercmd.forwardmove && !usercmd.rightmove ) || ( xyspeed <= MIN_BOB_SPEED ) ) {
		// start at beginning of cycle again
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;
	} else {
		if ( physicsObj.IsCrouching() ) {
			bobmove = pm_crouchbob.GetFloat();
			// ducked characters never play footsteps
		} else {
			// vary the bobbing based on the speed of the player
			bobmove = pm_walkbob.GetFloat() * ( 1.0f - bobFrac ) + pm_runbob.GetFloat() * bobFrac;
		}

		// check for footstep / splash sounds
		old = bobCycle;
		bobCycle = (int)( old + bobmove * gameLocal.msec ) & 255;
		bobFoot = ( bobCycle & 128 ) >> 7;
		bobfracsin = idMath::Fabs( sin( ( bobCycle & 127 ) / 127.0 * idMath::PI ) );
	}

	// calculate angles for view bobbing
	viewBobAngles.Zero();

	viewaxis = viewAngles.ToMat3() * physicsObj.GetGravityAxis();

	// add angles based on velocity
	delta = velocity * viewaxis[0];
	viewBobAngles.pitch += delta * pm_runpitch.GetFloat();

	delta = velocity * viewaxis[1];
	viewBobAngles.roll -= delta * pm_runroll.GetFloat();

	// add angles based on bob
	// make sure the bob is visible even at low speeds
	speed = xyspeed > 200 ? xyspeed : 200;

	delta = bobfracsin * pm_bobpitch.GetFloat() * speed;
	if ( physicsObj.IsCrouching() ) {
		delta *= 3;		// crouching
	}
	viewBobAngles.pitch += delta;
	delta = bobfracsin * pm_bobroll.GetFloat() * speed;
	if ( physicsObj.IsCrouching() ) {
		delta *= 3;		// crouching accentuates roll
	}
	if ( bobFoot & 1 ) {
		delta = -delta;
	}
	viewBobAngles.roll += delta;

	// calculate position for view bobbing
	viewBob.Zero();

	if ( physicsObj.HasSteppedUp() ) {

		// check for stepping up before a previous step is completed
		deltaTime = gameLocal.time - stepUpTime;
		if ( deltaTime < STEPUP_TIME ) {
			stepUpDelta = stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME + physicsObj.GetStepUp();
		} else {
			stepUpDelta = physicsObj.GetStepUp();
		}
		if ( stepUpDelta > 2.0f * pm_stepsize.GetFloat() ) {
			stepUpDelta = 2.0f * pm_stepsize.GetFloat();
		}
		stepUpTime = gameLocal.time;
	}

	idVec3 gravity = physicsObj.GetGravityNormal();

	// if the player stepped up recently
	deltaTime = gameLocal.time - stepUpTime;
	if ( deltaTime < STEPUP_TIME ) {
		viewBob += gravity * ( stepUpDelta * ( STEPUP_TIME - deltaTime ) / STEPUP_TIME );
	}

	// add bob height after any movement smoothing
	bob = bobfracsin * xyspeed * pm_bobup.GetFloat();
	if ( bob > 6 ) {
		bob = 6;
	}
	viewBob[2] += bob;

	// add fall height
	delta = gameLocal.time - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		viewBob -= gravity * ( landChange * f );
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		viewBob -= gravity * ( landChange * f );
	}
}

/*
================
idPlayer::UpdateDeltaViewAngles
================
*/
void idPlayer::UpdateDeltaViewAngles( const idAngles &angles ) {
	// set the delta angle
	idAngles delta;
	for( int i = 0; i < 3; i++ ) {
		delta[ i ] = angles[ i ] - SHORT2ANGLE( usercmd.angles[ i ] );
	}
	SetDeltaViewAngles( delta );
}

/*
================
idPlayer::SetViewAngles
================
*/
void idPlayer::SetViewAngles( const idAngles &angles ) {
	UpdateDeltaViewAngles( angles );
	viewAngles = angles;
}

/*
================
idPlayer::UpdateViewAngles
================
*/
void idPlayer::UpdateViewAngles( void ) {
	int i;
	idAngles delta;

	if ( !noclip && ( gameLocal.inCinematic || privateCameraView || gameLocal.GetCamera() || influenceActive == INFLUENCE_LEVEL2 || objectiveSystemOpen ) ) {
		// no view changes at all, but we still want to update the deltas or else when
		// we get out of this mode, our view will snap to a kind of random angle
		UpdateDeltaViewAngles( viewAngles );
		return;
	}

	// if dead
	if ( health <= 0 ) {
		if ( pm_thirdPersonDeath.GetBool() ) {
			viewAngles.roll = 0.0f;
			viewAngles.pitch = 30.0f;
		} else {
			viewAngles.roll = 40.0f;
			viewAngles.pitch = -15.0f;
		}
		return;
	}

	// circularly clamp the angles with deltas
	for ( i = 0; i < 3; i++ ) {
		cmdAngles[i] = SHORT2ANGLE( usercmd.angles[i] );
		if ( influenceActive == INFLUENCE_LEVEL3 ) {
			viewAngles[i] += idMath::ClampFloat( -1.0f, 1.0f, idMath::AngleDelta( idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i]) + deltaViewAngles[i] ) , viewAngles[i] ) );
		}
		//ff1.3 start - apply only a percentage of the movement inversely proportional to the zoom percentage
		else if ( advancedWeaponZoomTime && weapon.GetEntity() ) {
			viewAngles[i] += (1.0f - pm_zoomSensitReduction.GetFloat() * (weapon.GetEntity()->GetZoomFov() / CalcFov(true)) )*idMath::AngleDelta( idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i]) + deltaViewAngles[i] ) , viewAngles[i] );
		}
		//ff1.3 end
		else {
			viewAngles[i] = idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i]) + deltaViewAngles[i] );
		}
	}
	if ( !centerView.IsDone( gameLocal.time ) ) {
		viewAngles.pitch = centerView.GetCurrentValue(gameLocal.time);
	}

	// clamp the pitch
	if ( noclip ) {
		if ( viewAngles.pitch > 89.0f ) {
			// don't let the player look down more than 89 degrees while noclipping
			viewAngles.pitch = 89.0f;
		} else if ( viewAngles.pitch < -89.0f ) {
			// don't let the player look up more than 89 degrees while noclipping
			viewAngles.pitch = -89.0f;
		}
#ifdef _D3XP
	} else if ( mountedObject ) {
		int yaw_min, yaw_max, varc;

		mountedObject->GetAngleRestrictions( yaw_min, yaw_max, varc );

		if ( yaw_min < yaw_max ) {
			viewAngles.yaw = idMath::ClampFloat( yaw_min, yaw_max, viewAngles.yaw );
		} else {
			if ( viewAngles.yaw < 0 ) {
				viewAngles.yaw = idMath::ClampFloat( -180.f, yaw_max, viewAngles.yaw );
			} else {
				viewAngles.yaw = idMath::ClampFloat( yaw_min, 180.f, viewAngles.yaw );
			}
		}
		viewAngles.pitch = idMath::ClampFloat( -varc, varc, viewAngles.pitch );
#endif
	} else {
		if ( viewAngles.pitch > pm_maxviewpitch.GetFloat() ) {
			// don't let the player look down enough to see the shadow of his (non-existant) feet
			viewAngles.pitch = pm_maxviewpitch.GetFloat();
		} else if ( viewAngles.pitch < pm_minviewpitch.GetFloat() ) {
			// don't let the player look up more than 89 degrees
			viewAngles.pitch = pm_minviewpitch.GetFloat();
		}
	}

	UpdateDeltaViewAngles( viewAngles );

	// orient the model towards the direction we're looking
	SetAngles( idAngles( 0, viewAngles.yaw, 0 ) );

	// save in the log for analyzing weapon angle offsets
	loggedViewAngles[ gameLocal.framenum & (NUM_LOGGED_VIEW_ANGLES-1) ] = viewAngles;
}

/*
==============
idPlayer::AdjustHeartRate

Player heartrate works as follows

DEF_HEARTRATE is resting heartrate

Taking damage when health is above 75 adjusts heart rate by 1 beat per second
Taking damage when health is below 75 adjusts heart rate by 5 beats per second
Maximum heartrate from damage is MAX_HEARTRATE

Firing a weapon adds 1 beat per second up to a maximum of COMBAT_HEARTRATE

Being at less than 25% stamina adds 5 beats per second up to ZEROSTAMINA_HEARTRATE

All heartrates are target rates.. the heart rate will start falling as soon as there have been no adjustments for 5 seconds
Once it starts falling it always tries to get to DEF_HEARTRATE

The exception to the above rule is upon death at which point the rate is set to DYING_HEARTRATE and starts falling
immediately to zero

Heart rate volumes go from zero ( -40 db for DEF_HEARTRATE to 5 db for MAX_HEARTRATE ) the volume is
scaled linearly based on the actual rate

Exception to the above rule is once the player is dead, the dying heart rate starts at either the current volume if
it is audible or -10db and scales to 8db on the last few beats
==============
*/
void idPlayer::AdjustHeartRate( int target, float timeInSecs, float delay, bool force ) {

	if ( heartInfo.GetEndValue() == target ) {
		return;
	}

	if ( AI_DEAD && !force ) {
		return;
	}

	lastHeartAdjust = gameLocal.time;

	heartInfo.Init( gameLocal.time + delay * 1000, timeInSecs * 1000, heartRate, target );
}

/*
==============
idPlayer::GetBaseHeartRate
==============
*/
int idPlayer::GetBaseHeartRate( void ) {
	//ff1.3 start - disable stamina
	/* was:
	int base = idMath::FtoiFast( ( BASE_HEARTRATE + LOWHEALTH_HEARTRATE_ADJ ) - ( (float)health / 100.0f ) * LOWHEALTH_HEARTRATE_ADJ );
	int rate = idMath::FtoiFast( base + ( ZEROSTAMINA_HEARTRATE - base ) * ( 1.0f - stamina / pm_stamina.GetFloat() ) );
	*/
	int rate = idMath::FtoiFast( ( BASE_HEARTRATE + LOWHEALTH_HEARTRATE_ADJ ) - ( (float)health / 100.0f ) * LOWHEALTH_HEARTRATE_ADJ );
	//ff1.3 end
	int diff = ( lastDmgTime ) ? gameLocal.time - lastDmgTime : 99999;
	rate += ( diff < 5000 ) ? ( diff < 2500 ) ? ( diff < 1000 ) ? 15 : 10 : 5 : 0;
	return rate;
}

/*
==============
idPlayer::SetCurrentHeartRate
==============
*/
void idPlayer::SetCurrentHeartRate( void ) {

	int base = idMath::FtoiFast( ( BASE_HEARTRATE + LOWHEALTH_HEARTRATE_ADJ ) - ( (float) health / 100.0f ) * LOWHEALTH_HEARTRATE_ADJ );

	if ( PowerUpActive( ADRENALINE )) {
		heartRate = 115; //ff1.3 - was: 135
	} else {
		heartRate = idMath::FtoiFast( heartInfo.GetCurrentValue( gameLocal.time ) );
		int currentRate = GetBaseHeartRate();
		if ( health >= 0 && gameLocal.time > lastHeartAdjust + 2500 ) {
			AdjustHeartRate( currentRate, 2.5f, 0.0f, false );
		}
	}

	int bps = idMath::FtoiFast( 60.0f / heartRate * 1000.0f );
	if ( gameLocal.time - lastHeartBeat > bps ) {
		int dmgVol = DMG_VOLUME;
		int deathVol = DEATH_VOLUME;
		int zeroVol = ZERO_VOLUME;
		float pct = 0.0;
		if ( heartRate > BASE_HEARTRATE && health > 0 ) {
			pct = (float)(heartRate - base) / (MAX_HEARTRATE - base);
			pct *= ((float)dmgVol - (float)zeroVol);
		} else if ( health <= 0 ) {
			pct = (float)(heartRate - DYING_HEARTRATE) / (BASE_HEARTRATE - DYING_HEARTRATE);
			if ( pct > 1.0f ) {
				pct = 1.0f;
			} else if (pct < 0.0f) {
				pct = 0.0f;
			}
			pct *= ((float)deathVol - (float)zeroVol);
		}

		pct += (float)zeroVol;

		if ( pct != zeroVol ) {
			StartSound( "snd_heartbeat", SND_CHANNEL_HEART, SSF_PRIVATE_SOUND, false, NULL );
			// modify just this channel to a custom volume
			soundShaderParms_t	parms;
			memset( &parms, 0, sizeof( parms ) );
			parms.volume = pct;
			refSound.referenceSound->ModifySound( SND_CHANNEL_HEART, &parms );
		}

		lastHeartBeat = gameLocal.time;
	}
}

//ff1.3 start
/*
==============
idPlayer::UpdateAir
==============

void idPlayer::UpdateAir( void ) { //removed
	if ( health <= 0 ) {
		return;
	}

	// see if the player is connected to the info_vacuum
	bool	newAirless = false;

	if ( gameLocal.vacuumAreaNum != -1 ) {
		int	num = GetNumPVSAreas();
		if ( num > 0 ) {
			int		areaNum;

			// if the player box spans multiple areas, get the area from the origin point instead,
			// otherwise a rotating player box may poke into an outside area
			if ( num == 1 ) {
				const int	*pvsAreas = GetPVSAreas();
				areaNum = pvsAreas[0];
			} else {
				areaNum = gameRenderWorld->PointInArea( this->GetPhysics()->GetOrigin() );
			}
			newAirless = gameRenderWorld->AreasAreConnected( gameLocal.vacuumAreaNum, areaNum, PS_BLOCK_AIR );
		}
	}

#ifdef _D3XP
	if ( PowerUpActive( ENVIROTIME ) ) {
		newAirless = false;
	}
#endif

	if ( newAirless ) {
		if ( !airless ) {
			StartSound( "snd_decompress", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
			StartSound( "snd_noAir", SND_CHANNEL_BODY2, 0, false, NULL );
			if ( hud ) {
				hud->HandleNamedEvent( "noAir" );
			}
		}
		airTics--;
		if ( airTics < 0 ) {
			airTics = 0;
			// check for damage
			const idDict *damageDef = gameLocal.FindEntityDefDict( "damage_noair", false );
			int dmgTiming = 1000 * ((damageDef) ? damageDef->GetFloat( "delay", "3.0" ) : 3.0f );
			if ( gameLocal.time > lastAirDamage + dmgTiming ) {
				Damage( NULL, NULL, vec3_origin, "damage_noair", 1.0f, 0 );
				lastAirDamage = gameLocal.time;
			}
		}

	} else {
		if ( airless ) {
			StartSound( "snd_recompress", SND_CHANNEL_ANY, SSF_GLOBAL, false, NULL );
			StopSound( SND_CHANNEL_BODY2, false );
			if ( hud ) {
				hud->HandleNamedEvent( "Air" );
			}
		}
		airTics+=2;	// regain twice as fast as lose
		if ( airTics > pm_airTics.GetInteger() ) {
			airTics = pm_airTics.GetInteger();
		}
	}

	airless = newAirless;

	if ( hud ) {
		hud->SetStateInt( "player_air", 100 * airTics / pm_airTics.GetInteger() );
	}
}

void idPlayer::UpdatePowerupHud() { //removed

	if ( health <= 0 ) {
		return;
	}

	if (lastHudPowerup != hudPowerup) {

		if (hudPowerup == -1) {
			//The powerup hud should be turned off
			if ( hud ) {
				hud->HandleNamedEvent( "noPowerup" );
			}
		} else {
			//Turn the pwoerup hud on
			if ( hud ) {
				hud->HandleNamedEvent( "Powerup" );
			}
		}

		lastHudPowerup = hudPowerup;
	}

	if (hudPowerup != -1) {
		if (PowerUpActive(hudPowerup)) {
			int remaining = inventory.powerupEndTime[ hudPowerup ] - gameLocal.time;
			int filledbar = idMath::ClampInt( 0, hudPowerupDuration, remaining );

			if ( hud ) {
				hud->SetStateInt( "player_powerup", 100 * filledbar / hudPowerupDuration );
				hud->SetStateInt( "player_poweruptime", remaining / 1000 );
			}
		}
	}
}
*/

int idPlayer::ComputePowerupHudPct( int powerup, int maxHudDuration ) {
	int remaining = inventory.powerupEndTime[ powerup ] - gameLocal.slow.time;
	int filledbar = idMath::ClampInt( 0, maxHudDuration, remaining );
	return 100 * filledbar / maxHudDuration;
}

void idPlayer::UpdatePowerupHud() {
	if ( health <= 0 || !hud ) {
		return;
	}

	if ( PowerUpActive( INVULNERABILITY ) ) {
		hud->SetStateInt( "powerup_invuln_pct", ComputePowerupHudPct( INVULNERABILITY, 30000 ) );
	} else {
		hud->SetStateInt( "powerup_invuln_pct", 0 );
	}

	if ( PowerUpActive( ADRENALINE ) ) {
		hud->SetStateInt( "powerup_speed_pct", ComputePowerupHudPct( ADRENALINE, 45000 ) );
	} else {
		hud->SetStateInt( "powerup_speed_pct", 0 );
	}
}
//ff1.3 end

/*
==============
idPlayer::AddGuiPDAData
==============
 */
int idPlayer::AddGuiPDAData( const declType_t dataType, const char *listName, const idDeclPDA *src, idUserInterface *gui ) {
	int c, i;
	idStr work;
	if ( dataType == DECL_EMAIL ) {
		c = src->GetNumEmails();
		for ( i = 0; i < c; i++ ) {
			const idDeclEmail *email = src->GetEmailByIndex( i );
			if ( email == NULL ) {
				work = va( "-\tEmail %d not found\t-", i );
			} else {
				work = email->GetFrom();
				work += "\t";
				work += email->GetSubject();
				work += "\t";
				work += email->GetDate();
			}
			gui->SetStateString( va( "%s_item_%i", listName, i ), work );
		}
		return c;
	} else if ( dataType == DECL_AUDIO ) {
		c = src->GetNumAudios();
		for ( i = 0; i < c; i++ ) {
			const idDeclAudio *audio = src->GetAudioByIndex( i );
			if ( audio == NULL ) {
				work = va( "Audio Log %d not found", i );
			} else {
				work = audio->GetAudioName();
			}
			gui->SetStateString( va( "%s_item_%i", listName, i ), work );
		}
		return c;
	} else if ( dataType == DECL_VIDEO ) {
		c = inventory.videos.Num();
		for ( i = 0; i < c; i++ ) {
			const idDeclVideo *video = GetVideo( i );
			if ( video == NULL ) {
				work = va( "Video CD %s not found", inventory.videos[i].c_str() );
			} else {
				work = video->GetVideoName();
			}
			gui->SetStateString( va( "%s_item_%i", listName, i ), work );
		}
		return c;
	}
	return 0;
}

/*
==============
idPlayer::GetPDA
==============
 */
const idDeclPDA *idPlayer::GetPDA( void ) const {
	if ( inventory.pdas.Num() ) {
		return static_cast< const idDeclPDA* >( declManager->FindType( DECL_PDA, inventory.pdas[ 0 ] ) );
	} else {
		return NULL;
	}
}


/*
==============
idPlayer::GetVideo
==============
*/
const idDeclVideo *idPlayer::GetVideo( int index ) {
	if ( index >= 0 && index < inventory.videos.Num() ) {
		return static_cast< const idDeclVideo* >( declManager->FindType( DECL_VIDEO, inventory.videos[index], false ) );
	}
	return NULL;
}


/*
==============
idPlayer::UpdatePDAInfo
==============
*/
void idPlayer::UpdatePDAInfo( bool updatePDASel ) {
	int j, sel;

	if ( objectiveSystem == NULL ) {
		return;
	}

	assert( hud );

	int currentPDA = objectiveSystem->State().GetInt( "listPDA_sel_0", "0" );
	if ( currentPDA == -1 ) {
		currentPDA = 0;
	}

	if ( updatePDASel ) {
		objectiveSystem->SetStateInt( "listPDAVideo_sel_0", 0 );
		objectiveSystem->SetStateInt( "listPDAEmail_sel_0", 0 );
		objectiveSystem->SetStateInt( "listPDAAudio_sel_0", 0 );
	}

	if ( currentPDA > 0 ) {
		currentPDA = inventory.pdas.Num() - currentPDA;
	}

	// Mark in the bit array that this pda has been read
	if ( currentPDA < 128 ) {
		inventory.pdasViewed[currentPDA >> 5] |= 1 << (currentPDA & 31);
	}

	pdaAudio = "";
	pdaVideo = "";
	pdaVideoWave = "";
	idStr name, data, preview, info, wave;
	for ( j = 0; j < MAX_PDAS; j++ ) {
		objectiveSystem->SetStateString( va( "listPDA_item_%i", j ), "" );
	}
	for ( j = 0; j < MAX_PDA_ITEMS; j++ ) {
		objectiveSystem->SetStateString( va( "listPDAVideo_item_%i", j ), "" );
		objectiveSystem->SetStateString( va( "listPDAAudio_item_%i", j ), "" );
		objectiveSystem->SetStateString( va( "listPDAEmail_item_%i", j ), "" );
		objectiveSystem->SetStateString( va( "listPDASecurity_item_%i", j ), "" );
	}
	for ( j = 0; j < inventory.pdas.Num(); j++ ) {

		const idDeclPDA *pda = static_cast< const idDeclPDA* >( declManager->FindType( DECL_PDA, inventory.pdas[j], false ) );

		if ( pda == NULL ) {
			continue;
		}

		int index = inventory.pdas.Num() - j;
		if ( j == 0 ) {
			// Special case for the first PDA
			index = 0;
		}

		if ( j != currentPDA && j < 128 && inventory.pdasViewed[j >> 5] & (1 << (j & 31)) ) {
			// This pda has been read already, mark in gray
			objectiveSystem->SetStateString( va( "listPDA_item_%i", index), va(S_COLOR_GRAY "%s", pda->GetPdaName()) );
		} else {
			// This pda has not been read yet
			objectiveSystem->SetStateString( va( "listPDA_item_%i", index), pda->GetPdaName() );
		}

		const char *security = pda->GetSecurity();
		if ( j == currentPDA || (currentPDA == 0 && security && *security ) ) {
			if ( *security == 0 ) {
				security = common->GetLanguageDict()->GetString( "#str_00066" );
			}
			objectiveSystem->SetStateString( "PDASecurityClearance", security );
		}

		if ( j == currentPDA ) {

			objectiveSystem->SetStateString( "pda_icon", pda->GetIcon() );
			objectiveSystem->SetStateString( "pda_id", pda->GetID() );
			objectiveSystem->SetStateString( "pda_title", pda->GetTitle() );

			if ( j == 0 ) {
				// Selected, personal pda
				// Add videos
				if ( updatePDASel || !inventory.pdaOpened ) {
					objectiveSystem->HandleNamedEvent( "playerPDAActive" );
					objectiveSystem->SetStateString( "pda_personal", "1" );
					inventory.pdaOpened = true;
				}
				objectiveSystem->SetStateString( "pda_location", hud->State().GetString("location") );
				objectiveSystem->SetStateString( "pda_name", cvarSystem->GetCVarString( "ui_name") );
				int videoCount = AddGuiPDAData( DECL_VIDEO, "listPDAVideo", pda, objectiveSystem );
				objectiveSystem->SetStateInt( "videoCount", videoCount ); //ff1.3
				sel = objectiveSystem->State().GetInt( "listPDAVideo_sel_0", "0" );
				const idDeclVideo *vid = NULL;
				if ( sel >= 0 && sel < inventory.videos.Num() ) {
					vid = static_cast< const idDeclVideo * >( declManager->FindType( DECL_VIDEO, inventory.videos[ sel ], false ) );
				}
				if ( vid ) {
					pdaVideo = vid->GetRoq();
					pdaVideoWave = vid->GetWave();
					objectiveSystem->SetStateString( "PDAVideoTitle", vid->GetVideoName() );
					objectiveSystem->SetStateString( "PDAVideoVid", vid->GetRoq() );
					objectiveSystem->SetStateString( "PDAVideoIcon", vid->GetPreview() );
					objectiveSystem->SetStateString( "PDAVideoInfo", vid->GetInfo() );
				} else {
					//FIXME: need to precache these in the player def
					objectiveSystem->SetStateString( "PDAVideoVid", "" ); //sound/vo/video/welcome.tga
					objectiveSystem->SetStateString( "PDAVideoIcon", "" ); //sound/vo/video/welcome.tga
					objectiveSystem->SetStateString( "PDAVideoTitle", "" );
					objectiveSystem->SetStateString( "PDAVideoInfo", "" );
				}
			} else {
				// Selected, non-personal pda
				// Add audio logs
				if ( updatePDASel ) {
					objectiveSystem->HandleNamedEvent( "playerPDANotActive" );
					objectiveSystem->SetStateString( "pda_personal", "0" );
					inventory.pdaOpened = true;
				}
				objectiveSystem->SetStateString( "pda_location", pda->GetPost() );
				objectiveSystem->SetStateString( "pda_name", pda->GetFullName() );
				int audioCount = AddGuiPDAData( DECL_AUDIO, "listPDAAudio", pda, objectiveSystem );
				objectiveSystem->SetStateInt( "audioLogCount", audioCount );
				sel = objectiveSystem->State().GetInt( "listPDAAudio_sel_0", "0" );
				const idDeclAudio *aud = NULL;
				if ( sel >= 0 ) {
					aud = pda->GetAudioByIndex( sel );
				}
				if ( aud ) {
					pdaAudio = aud->GetWave();
					objectiveSystem->SetStateString( "PDAAudioTitle", aud->GetAudioName() );
					objectiveSystem->SetStateString( "PDAAudioIcon", aud->GetPreview() );
					objectiveSystem->SetStateString( "PDAAudioInfo", aud->GetInfo() );
				} else {
					objectiveSystem->SetStateString( "PDAAudioIcon", "sound/vo/video/welcome.tga" );
					objectiveSystem->SetStateString( "PDAAutioTitle", "" );
					objectiveSystem->SetStateString( "PDAAudioInfo", "" );
				}
			}
			// add emails
			name = "";
			data = "";
			int numEmails = pda->GetNumEmails();
			if ( numEmails > 0 ) {
				AddGuiPDAData( DECL_EMAIL, "listPDAEmail", pda, objectiveSystem );
				sel = objectiveSystem->State().GetInt( "listPDAEmail_sel_0", "-1" );
				if ( sel >= 0 && sel < numEmails ) {
					const idDeclEmail *email = pda->GetEmailByIndex( sel );
					name = email->GetSubject();
					data = email->GetBody();
				}
			}
			objectiveSystem->SetStateInt( "emailCount", numEmails ); //ff1.3
			objectiveSystem->SetStateString( "PDAEmailTitle", name );
			objectiveSystem->SetStateString( "PDAEmailText", data );
		}
	}
	if ( objectiveSystem->State().GetInt( "listPDA_sel_0", "-1" ) == -1 ) {
		objectiveSystem->SetStateInt( "listPDA_sel_0", 0 );
	}

	//ff1.3 start
	objectiveSystem->SetStateInt( "coverCount", inventory.gameCovers.Num() );
	for ( j = -2; j <= 2; j++ ) {
		sel = selectedGameCoverIndex + j;
		if ( sel >= 0 && sel < inventory.gameCovers.Num() ) {
			const idDict * cover = gameLocal.FindEntityDefDict( inventory.gameCovers[sel].defName, false );
			if ( cover ) {
				objectiveSystem->SetStateString( va( "cover_name_%i", j+2), cover->GetString("name") );
				objectiveSystem->SetStateString( va( "cover_icon_%i", j+2), cover->GetString("mtr_cover") );
				objectiveSystem->SetStateInt( va( "cover_amount_%i", j+2), 1 );
				//objectiveSystem->SetStateInt( va( "cover_amount_%i", j+2), inventory.gameCovers[sel].amount );
				continue;
			}
		}

		objectiveSystem->SetStateString( va( "cover_name_%i", j+2), "" );
		objectiveSystem->SetStateString( va( "cover_icon_%i", j+2), "" );
		objectiveSystem->SetStateInt( va( "cover_amount_%i", j+2), 0 );
	}

	UpdatePDAStats( objectiveSystem );
	//ff1.3 end

	objectiveSystem->StateChanged( gameLocal.time );
}

/*
==============
idPlayer::TogglePDA
==============
*/
void idPlayer::TogglePDA( void ) {
	if ( objectiveSystem == NULL ) {
		return;
	}

	if ( inventory.pdas.Num() == 0 ) {
		ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_noPDA" ), true );
		return;
	}

	assert( hud );

	if ( !objectiveSystemOpen ) {
		int j, c = inventory.items.Num();
		objectiveSystem->SetStateInt( "inv_count", c );
		for ( j = 0; j < MAX_INVENTORY_ITEMS; j++ ) {
			objectiveSystem->SetStateString( va( "inv_name_%i", j ), "" );
			objectiveSystem->SetStateString( va( "inv_icon_%i", j ), "" );
			objectiveSystem->SetStateString( va( "inv_text_%i", j ), "" );
		}
		for ( j = 0; j < c; j++ ) {
			idDict *item = inventory.items[j];
			if ( !item->GetBool( "inv_pda" ) ) {
				const char *iname = item->GetString( "inv_name" );
				const char *iicon = item->GetString( "inv_icon" );
				const char *itext = item->GetString( "inv_text" );
				objectiveSystem->SetStateString( va( "inv_name_%i", j ), iname );
				objectiveSystem->SetStateString( va( "inv_icon_%i", j ), iicon );
				objectiveSystem->SetStateString( va( "inv_text_%i", j ), itext );
				const idKeyValue *kv = item->MatchPrefix( "inv_id", NULL );
				if ( kv ) {
					objectiveSystem->SetStateString( va( "inv_id_%i", j ), kv->GetValue() );
				}
			}
		}

		for ( j = 0; j < MAX_WEAPONS; j++ ) {
			const char *weapnum = va( "def_weapon%d", j );
			const char *hudWeap = va( "weapon%d", j );
			int weapstate = 0;
			if ( inventory.weapons & ( 1 << j ) ) {
				const char *weap = spawnArgs.GetString( weapnum );
				if ( weap && *weap ) {
					weapstate++;
				}
			}
			objectiveSystem->SetStateInt( hudWeap, weapstate );
		}

		objectiveSystem->SetStateInt( "listPDA_sel_0", inventory.selPDA );
		objectiveSystem->SetStateInt( "listPDAVideo_sel_0", inventory.selVideo );
		objectiveSystem->SetStateInt( "listPDAAudio_sel_0", inventory.selAudio );
		objectiveSystem->SetStateInt( "listPDAEmail_sel_0", inventory.selEMail );
		UpdatePDAInfo( false );
		UpdateObjectiveInfo();
		objectiveSystem->Activate( true, gameLocal.time );
		hud->HandleNamedEvent( "pdaPickupHide" );
		hud->HandleNamedEvent( "videoPickupHide" );
	} else {
		inventory.selPDA = objectiveSystem->State().GetInt( "listPDA_sel_0" );
		inventory.selVideo = objectiveSystem->State().GetInt( "listPDAVideo_sel_0" );
		inventory.selAudio = objectiveSystem->State().GetInt( "listPDAAudio_sel_0" );
		inventory.selEMail = objectiveSystem->State().GetInt( "listPDAEmail_sel_0" );
		objectiveSystem->Activate( false, gameLocal.time );
	}
	objectiveSystemOpen ^= 1;
}

/*
==============
idPlayer::ToggleScoreboard
==============
*/
void idPlayer::ToggleScoreboard( void ) {
	scoreBoardOpen ^= 1;
}

/*
==============
idPlayer::Spectate
==============
*/
void idPlayer::Spectate( bool spectate ) {
	idBitMsg	msg;
	byte		msgBuf[MAX_EVENT_PARAM_SIZE];

	// track invisible player bug
	// all hiding and showing should be performed through Spectate calls
	// except for the private camera view, which is used for teleports
	assert( ( teleportEntity.GetEntity() != NULL ) || ( IsHidden() == spectating ) );

	if ( spectating == spectate ) {
		return;
	}

	spectating = spectate;

	if ( gameLocal.isServer ) {
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteBits( spectating, 1 );
		ServerSendEvent( EVENT_SPECTATE, &msg, false, -1 );
	}

	if ( spectating ) {
		// join the spectators
		ClearPowerUps();
		spectator = this->entityNumber;
		Init();
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.DisableClip();
		Hide();
		Event_DisableWeapon();
		if ( hud ) {
			hud->HandleNamedEvent( "aim_clear" );
			MPAimFadeTime = 0;
		}
	} else {
		// put everything back together again
		currentWeapon = -1;	// to make sure the def will be loaded if necessary
		Show();
		Event_EnableWeapon();
	}
	SetClipModel();
}

/*
==============
idPlayer::SetClipModel
==============
*/
void idPlayer::SetClipModel( void ) {
	idBounds bounds;

	if ( spectating ) {
		bounds = idBounds( vec3_origin ).Expand( pm_spectatebbox.GetFloat() * 0.5f );
	} else {
		bounds[0].Set( -pm_bboxwidth.GetFloat() * 0.5f, -pm_bboxwidth.GetFloat() * 0.5f, 0 );
		bounds[1].Set( pm_bboxwidth.GetFloat() * 0.5f, pm_bboxwidth.GetFloat() * 0.5f, pm_normalheight.GetFloat() );
	}
	// the origin of the clip model needs to be set before calling SetClipModel
	// otherwise our physics object's current origin value gets reset to 0
	idClipModel *newClip;
	if ( pm_usecylinder.GetBool() ) {
		newClip = new idClipModel( idTraceModel( bounds, 8 ) );
		newClip->Translate( physicsObj.PlayerGetOrigin() );
		physicsObj.SetClipModel( newClip, 1.0f );
	} else {
		newClip = new idClipModel( idTraceModel( bounds ) );
		newClip->Translate( physicsObj.PlayerGetOrigin() );
		physicsObj.SetClipModel( newClip, 1.0f );
	}
}

//ivan start

/*
==============
idPlayer::UseUseThirdPersonCamera
==============
*/
bool idPlayer::UseThirdPersonCamera( void ) {
	return pm_thirdPerson.GetBool() || IsRiding();
}

/*
==============
idPlayer::SetupForRiding
==============
*/
void idPlayer::CommonRidingSetup( idEntity *riddenEnt ){
	physicsObj.DisableClip();
	weaponEnabledOnRideExit = weaponEnabled;
	weaponEnabled = false;
	if ( !gameLocal.inCinematic ) {
		Hide();
		if ( weapon.GetEntity() ) {
			weapon.GetEntity()->EnterCinematic();
		}
	}
	if ( PowerUpActive( HELLTIME ) ) {
		ClearPowerup( HELLTIME );
	}
	if ( painKillerProjectile.GetEntity() != NULL ) {
		painKillerProjectile.GetEntity()->StartReturnPhase();
	}
}

/*
==============
idPlayer::SetupForRiding
==============
*/
void idPlayer::CommonRidingCleanUp( bool killed ){
	currentVehicle = NULL;
	currentRiddenAI = NULL;
	if ( !gameLocal.inCinematic ) {
		Show();
	}
	physicsObj.EnableClip();
	if ( !killed ) {
		weaponEnabled = weaponEnabledOnRideExit;
		if ( weaponEnabled && weapon.GetEntity() && !gameLocal.inCinematic ) {
			weapon.GetEntity()->ExitCinematic();
		}

		//restore player hud
		if ( hud ) {
			UpdateEveryAmmoOnHud(); //make sure ammo are up to date, especially possession pw
			hud->HandleNamedEvent( "stop_riding" );
		}
		if ( cursor ) {
			cursor->HandleNamedEvent( "enableDefaultCursor" );
		}
	}
}

/*
==============
idPlayer::CanEnterAI
==============
*/
bool idPlayer::CanEnterAI( void ){
	return ( !IsRiding() && health > 0 && !IsBound() );
}

/*
==============
idPlayer::EnterAI
called by idAI_Rideable
==============
*/
void idPlayer::EnterAI( idAI_Rideable* aiEnt ){
	currentRiddenAI = aiEnt;
	CommonRidingSetup( aiEnt );
	aiEnt->InitHudStats( hud, cursor );
	aiEnt->UpdateHudStats( hud );

	//clear player enemies
	idActor *ent;
	for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
		if ( ent->IsType( idAI::Type ) ) {
			static_cast<idAI*>(ent)->PostEventMS( &AI_ClearEnemy, 0 );
		}
	}

	//fadein
	playerView.Fade( vec4_zero, SEC2MS( 1.0f ) );
}

/*
==============
idPlayer::ExitAI
called by idAI_Rideable
==============
*/
void idPlayer::ExitAI( bool killed ){
	if ( currentRiddenAI ) {
		if ( killed && hud ) { //update the health one last time to see the negative value
			currentRiddenAI->UpdateHudStats( hud );
		}
		//currentRiddenAI->UnbindDriver();
		CommonRidingCleanUp( killed );

		//fadein
		playerView.Fade( vec4_zero, SEC2MS( 1.0f ) );
	}
}

/*
==============
idPlayer::EnterVehicle
==============
*/
void idPlayer::EnterVehicle( idAFEntity_Vehicle *vehicle ){
	if ( !IsRiding() && vehicle && vehicle->StartDriving( this ) ) {
		currentVehicle = vehicle;
		CommonRidingSetup( vehicle );
		vehicle->InitHudStats( hud, cursor );
		vehicle->UpdateHudStats( hud );
	}
}

/*
==============
idPlayer::ExitVehicle
==============
*/
void idPlayer::ExitVehicle( void ){
	if ( currentVehicle ) {
		currentVehicle->StopDriving();
		CommonRidingCleanUp(false);
		nextVehicleTime = gameLocal.time + VEHICLE_ENTER_DELAY;
	}
}

//ivan end

/*
==============
idPlayer::PerformImpulse
==============
*/
void idPlayer::PerformImpulse( int impulse ) {

	if ( gameLocal.isClient ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		assert( entityNumber == gameLocal.localClientNum );
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( impulse, 6 );
		ClientSendEvent( EVENT_IMPULSE, &msg );
	}

	//ff1.3 start
	/*
	if ( currentVehicle ) {
		currentVehicle->PerformImpulse( impulse ); //TODO
		//return;
	}
	*/
	if ( currentRiddenAI && currentRiddenAI->PerformImpulse( impulse ) ) {
		return;
	}
	//ff1.3 end

	if ( impulse >= IMPULSE_0 && impulse <= IMPULSE_12 ) {
		SelectWeapon( impulse, false );
		return;
	}

	//ff impulse workaround for weapon selection
	if ( impulse >= IMPULSE_23 && impulse <= IMPULSE_28 ) {
		SelectWeapon( impulse - 10, false );
		return;
	}

	switch( impulse ) {
		case IMPULSE_13: {
			Reload();
			break;
		}
		case IMPULSE_14: {
			NextWeapon();
			break;
		}
		case IMPULSE_15: {
			PrevWeapon();
			break;
		}
		//ff start
		//case IMPULSE_16: { } //was ZOOM in ff1.2
		//ff end
		case IMPULSE_17: {
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
				gameLocal.mpGame.ToggleReady();
			}
			break;
		}
		case IMPULSE_18: {
			centerView.Init(gameLocal.time, 200, viewAngles.pitch, 0);
			break;
		}
		case IMPULSE_19: {
			// when we're not in single player, IMPULSE_19 is used for showScores
			// otherwise it opens the pda
			if ( !gameLocal.isMultiplayer ) {
				//ff start
				if ( objectiveSystemOpen || (!spectating && !gameLocal.inCinematic && !privateCameraView && health > 0) ) {
					TogglePDA();
				}
				//ff end
			}
			break;
		}
		case IMPULSE_20: {
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
				gameLocal.mpGame.ToggleTeam();
			}
			break;
		}
		//ff start
		case IMPULSE_21: {
			if ( PowerUpActive( HELLTIME ) ){
				ClearPowerup( HELLTIME );
			} else if ( !gameLocal.inCinematic && !privateCameraView ){
				GivePowerUp( HELLTIME, 3600 * 1000 ); //1h
			}
			break;
		}
		//ff end

		case IMPULSE_22: {
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
				gameLocal.mpGame.ToggleSpectate();
			}
			break;
		}

		/* ff: now it's used for weapon selelection
		case IMPULSE_25: {
			if ( gameLocal.isServer && gameLocal.mpGame.IsGametypeFlagBased() && (gameLocal.serverInfo.GetInt( "si_midnight" ) == 2) ) {
				if ( enviroSuitLight.IsValid() ) {
					enviroSuitLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
					enviroSuitLight = NULL;
				} else {
					const idDict *lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );
					if ( lightDef ) {
						idEntity *temp = static_cast<idEntity *>(enviroSuitLight.GetEntity());
						idAngles lightAng = firstPersonViewAxis.ToAngles();
						idVec3 lightOrg = firstPersonViewOrigin;

						idVec3 enviroOffset = lightDef->GetVector( "enviro_offset" );
						idVec3 enviroAngleOffset = lightDef->GetVector( "enviro_angle_offset" );

						gameLocal.SpawnEntityDef( *lightDef, &temp, false );
						enviroSuitLight = static_cast<idLight *>(temp);

						enviroSuitLight.GetEntity()->fl.networkSync = true;

						lightOrg += (enviroOffset.x * firstPersonViewAxis[0]);
						lightOrg += (enviroOffset.y * firstPersonViewAxis[1]);
						lightOrg += (enviroOffset.z * firstPersonViewAxis[2]);
						lightAng.pitch += enviroAngleOffset.x;
						lightAng.yaw += enviroAngleOffset.y;
						lightAng.roll += enviroAngleOffset.z;

						enviroSuitLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
						enviroSuitLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );

						enviroSuitLight.GetEntity()->UpdateVisuals();
						enviroSuitLight.GetEntity()->Present();
					}
				}
			}
			break;
		} */

		/*
                case IMPULSE_28: {
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
				gameLocal.mpGame.CastVote( gameLocal.localClientNum, true );
			}
			break;
		}
		case IMPULSE_29: {
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
				gameLocal.mpGame.CastVote( gameLocal.localClientNum, false );
			}
			break;
		}
                */

/*
		//ff1.1 start - pda
		case IMPULSE_29: {
			// opens the pda
			if ( !gameLocal.isMultiplayer ) {
				if ( objectiveSystemOpen ) {
					TogglePDA();
				} else if ( weapon_pda >= 0 ) {
					SelectWeapon( weapon_pda, true );
				}
			}
			break;
		}


		case IMPULSE_40: {
			UseVehicle();
			break;
		}
		//ff1.1 end
	*/

 /* commented out by ivan
#ifdef _D3XP
		 //Hack so the chainsaw will work in MP
		case IMPULSE_27: {
			SelectWeapon(18, false);
			break;
		}
#endif
*/
	}
}

bool idPlayer::HandleESC( void ) { //true = ignore, false = go to main menu
	//ff1.3 start
	bool escHeld = ( gameLocal.fast.time < lastEscPressedTime + ESC_PRESSED_TIMEOUT );
	lastEscPressedTime = gameLocal.fast.time;
	//gameLocal.Printf("%d Esc pressed\n", gameLocal.fast.time);

	if ( gameLocal.inCinematic ) {
		if ( skipCinematicTipStartTime > 0 ) {
			if ( gameLocal.fast.time > skipCinematicTipStartTime + SKIP_CIN_TIP_TIME ) {
				//gameLocal.Printf("%d Try to skip\n", gameLocal.fast.time);
				return SkipCinematic() || escHeld;
			} else {
				return true;
			}
		} else {
			//gameLocal.Printf("%d Begin skip\n", gameLocal.fast.time);
			skipCinematicTipStartTime = gameLocal.fast.time;
			if ( infoGui ) {
				infoGui->HandleNamedEvent( "tipSkipCinematicUp" );
			}
			return true;
		}
	}

	/* was:
	if ( gameLocal.inCinematic ) {
		return SkipCinematic();
	}
	*/
	//ff1.3 end

	if ( objectiveSystemOpen ) {
		TogglePDA();
		return true;
	}

	return escHeld; //ff1.3 - was: false
}

bool idPlayer::SkipCinematic( void ) {
	StartSound( "snd_skipcinematic", SND_CHANNEL_ANY, 0, false, NULL );
	if ( skipCinematicTipStartTime > 0 ) {
		HideSkipCinematicTip();
	}
	return gameLocal.SkipCinematic();
}

/*
==============
idPlayer::HideSkipCinematicTip
==============
*/
void idPlayer::HideSkipCinematicTip( void ) {
	skipCinematicTipStartTime = 0;
	//gameLocal.Printf("%d reset skipCinematicTipStartTime\n", gameLocal.fast.time);
	if ( infoGui ) {
		infoGui->HandleNamedEvent( "tipSkipCinematicDown" );
	}
}

/*
==============
idPlayer::EvaluateControls
==============
*/
void idPlayer::EvaluateControls( void ) {
	// check for respawning
	if ( health <= 0 ) {
		if ( ( gameLocal.time > minRespawnTime ) && ( usercmd.buttons & BUTTON_ATTACK ) ) {
			forceRespawn = true;
		} else if ( gameLocal.time > maxRespawnTime ) {
			forceRespawn = true;
		}
	}

	// in MP, idMultiplayerGame decides spawns
	if ( forceRespawn && !gameLocal.isMultiplayer && !g_testDeath.GetBool() ) {
		// in single player, we let the session handle restarting the level or loading a game
		gameLocal.sessionCommand = "died";
	}

	if ( ( usercmd.flags & UCF_IMPULSE_SEQUENCE ) != ( oldFlags & UCF_IMPULSE_SEQUENCE ) ) {
		PerformImpulse( usercmd.impulse );
	}

	scoreBoardOpen = ( ( usercmd.buttons & BUTTON_SCORES ) != 0 || forceScoreBoard );

	oldFlags = usercmd.flags;

	AdjustSpeed();

	// update the viewangles
	UpdateViewAngles();
}

/*
==============
idPlayer::AdjustSpeed
==============
*/
void idPlayer::AdjustSpeed( void ) {
	float speed;
	//float rate;

	if ( IsRiding() ) { //ff1.1
		speed = 0.0f;
		bobFrac = 0.0f;
	} else if ( spectating ) {
		speed = pm_spectatespeed.GetFloat();
		bobFrac = 0.0f;
	} else if ( noclip ) {
		speed = pm_noclipspeed.GetFloat();
		bobFrac = 0.0f;
	}
	//ff1.3 start - always run! I mean... walk :)
	/*
	else if ( !physicsObj.OnLadder() && ( usercmd.buttons & BUTTON_RUN ) && ( usercmd.forwardmove || usercmd.rightmove ) && ( usercmd.upmove >= 0 ) ) {
		if ( !gameLocal.isMultiplayer && !physicsObj.IsCrouching() && !PowerUpActive( ADRENALINE ) ) {
			stamina -= MS2SEC( gameLocal.msec );
		}
		if ( stamina < 0 ) {
			stamina = 0;
		}
		if ( ( !pm_stamina.GetFloat() ) || ( stamina > pm_staminathreshold.GetFloat() ) ) {
			bobFrac = 1.0f;
		} else if ( pm_staminathreshold.GetFloat() <= 0.0001f ) {
			bobFrac = 0.0f;
		} else {
			bobFrac = stamina / pm_staminathreshold.GetFloat();
		}
		speed = pm_walkspeed.GetFloat() * ( 1.0f - bobFrac ) + pm_runspeed.GetFloat() * bobFrac;
	}
	*/
	//ff1.3 end
	else {
		//ff1.3 start - disable stamina
		/* was:
		rate = pm_staminarate.GetFloat();

		// increase 25% faster when not moving
		if ( ( usercmd.forwardmove == 0 ) && ( usercmd.rightmove == 0 ) && ( !physicsObj.OnLadder() || ( usercmd.upmove == 0 ) ) ) {
			 rate *= 1.25f;
		}

		stamina += rate * MS2SEC( gameLocal.msec );
		if ( stamina > pm_stamina.GetFloat() ) {
			stamina = pm_stamina.GetFloat();
		}
		*/
		//ff1.3 end

		speed = pm_walkspeed.GetFloat();

		//ff1.3 start
		if ( PowerUpActive(HELLTIME) ) {
			speed *= HELLTIME_SPEED_MULT;
		}
		//ff1.3 end

		bobFrac = 0.0f;
	}

	speed *= PowerUpModifier(SPEED);

	if ( influenceActive == INFLUENCE_LEVEL3 ) {
		speed *= 0.33f;
	}

	physicsObj.SetSpeed( speed, pm_crouchspeed.GetFloat() );
}

/*
==============
idPlayer::AdjustBodyAngles
==============
*/
void idPlayer::AdjustBodyAngles( void ) {
	idMat3	lookAxis;
	idMat3	legsAxis;
	bool	blend;
	float	diff;
	float	frac;
	float	upBlend;
	float	forwardBlend;
	float	downBlend;

	if ( health < 0 ) {
		return;
	}

	blend = true;

	if ( !physicsObj.HasGroundContacts() ) {
		idealLegsYaw = 0.0f;
		legsForward = true;
	} else if ( usercmd.forwardmove < 0 ) {
		idealLegsYaw = idMath::AngleNormalize180( idVec3( -usercmd.forwardmove, usercmd.rightmove, 0.0f ).ToYaw() );
		legsForward = false;
	} else if ( usercmd.forwardmove > 0 ) {
		idealLegsYaw = idMath::AngleNormalize180( idVec3( usercmd.forwardmove, -usercmd.rightmove, 0.0f ).ToYaw() );
		legsForward = true;
	} else if ( ( usercmd.rightmove != 0 ) && physicsObj.IsCrouching() ) {
		if ( !legsForward ) {
			idealLegsYaw = idMath::AngleNormalize180( idVec3( idMath::Abs( usercmd.rightmove ), usercmd.rightmove, 0.0f ).ToYaw() );
		} else {
			idealLegsYaw = idMath::AngleNormalize180( idVec3( idMath::Abs( usercmd.rightmove ), -usercmd.rightmove, 0.0f ).ToYaw() );
		}
	} else if ( usercmd.rightmove != 0 ) {
		idealLegsYaw = 0.0f;
		legsForward = true;
	} else {
		legsForward = true;
		diff = idMath::Fabs( idealLegsYaw - legsYaw );
		idealLegsYaw = idealLegsYaw - idMath::AngleNormalize180( viewAngles.yaw - oldViewYaw );
		if ( diff < 0.1f ) {
			legsYaw = idealLegsYaw;
			blend = false;
		}
	}

	if ( !physicsObj.IsCrouching() ) {
		legsForward = true;
	}

	oldViewYaw = viewAngles.yaw;

	AI_TURN_LEFT = false;
	AI_TURN_RIGHT = false;
	if ( idealLegsYaw < -45.0f ) {
		idealLegsYaw = 0;
		AI_TURN_RIGHT = true;
		blend = true;
	} else if ( idealLegsYaw > 45.0f ) {
		idealLegsYaw = 0;
		AI_TURN_LEFT = true;
		blend = true;
	}

	if ( blend ) {
		legsYaw = legsYaw * 0.9f + idealLegsYaw * 0.1f;
	}
	legsAxis = idAngles( 0.0f, legsYaw, 0.0f ).ToMat3();
	animator.SetJointAxis( hipJoint, JOINTMOD_WORLD, legsAxis );

	// calculate the blending between down, straight, and up
	frac = viewAngles.pitch / 90.0f;
	if ( frac > 0.0f ) {
		downBlend		= frac;
		forwardBlend	= 1.0f - frac;
		upBlend			= 0.0f;
	} else {
		downBlend		= 0.0f;
		forwardBlend	= 1.0f + frac;
		upBlend			= -frac;
	}

	animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 0, downBlend );
	animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 1, forwardBlend );
	animator.CurrentAnim( ANIMCHANNEL_TORSO )->SetSyncedAnimWeight( 2, upBlend );

	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 0, downBlend );
	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 1, forwardBlend );
	animator.CurrentAnim( ANIMCHANNEL_LEGS )->SetSyncedAnimWeight( 2, upBlend );
}

/*
==============
idPlayer::InitAASLocation
==============
*/
void idPlayer::InitAASLocation( void ) {
	int		i;
	int		num;
	idVec3	size;
	idBounds bounds;
	idAAS	*aas;
	idVec3	origin;

	GetFloorPos( 64.0f, origin );

	num = gameLocal.NumAAS();
	aasLocation.SetGranularity( 1 );
	aasLocation.SetNum( num );
	for( i = 0; i < aasLocation.Num(); i++ ) {
		aasLocation[ i ].areaNum = 0;
		aasLocation[ i ].pos = origin;
		aas = gameLocal.GetAAS( i );
		if ( aas && aas->GetSettings() ) {
			size = aas->GetSettings()->boundingBoxes[0][1];
			bounds[0] = -size;
			size.z = 32.0f;
			bounds[1] = size;

			aasLocation[ i ].areaNum = aas->PointReachableAreaNum( origin, bounds, AREA_REACHABLE_WALK );
		}
	}
}

/*
==============
idPlayer::SetAASLocation
==============
*/
void idPlayer::SetAASLocation( void ) {
	int		i;
	int		areaNum;
	idVec3	size;
	idBounds bounds;
	idAAS	*aas;
	idVec3	origin;

	if ( !GetFloorPos( 64.0f, origin ) ) {
		return;
	}

	for( i = 0; i < aasLocation.Num(); i++ ) {
		aas = gameLocal.GetAAS( i );
		if ( !aas ) {
			continue;
		}

		size = aas->GetSettings()->boundingBoxes[0][1];
		bounds[0] = -size;
		size.z = 32.0f;
		bounds[1] = size;

		areaNum = aas->PointReachableAreaNum( origin, bounds, AREA_REACHABLE_WALK );
		if ( areaNum ) {
			aasLocation[ i ].pos = origin;
			aasLocation[ i ].areaNum = areaNum;
		}
	}
}

/*
==============
idPlayer::GetAASLocation
==============
*/
void idPlayer::GetAASLocation( idAAS *aas, idVec3 &pos, int &areaNum ) const {
	int i;

	if ( aas != NULL ) {
		for( i = 0; i < aasLocation.Num(); i++ ) {
			if ( aas == gameLocal.GetAAS( i ) ) {
				areaNum = aasLocation[ i ].areaNum;
				pos = aasLocation[ i ].pos;
				return;
			}
		}
	}

	areaNum = 0;
	pos = physicsObj.GetOrigin();
}

/*
==============
idPlayer::Move
==============
*/
void idPlayer::Move( void ) {
	float newEyeOffset;
	idVec3 oldOrigin;
	idVec3 oldVelocity;
	idVec3 pushVelocity;

	// save old origin and velocity for crashlanding
	oldOrigin = physicsObj.GetOrigin();
	oldVelocity = physicsObj.GetLinearVelocity();
	pushVelocity = physicsObj.GetPushedLinearVelocity();

	// set physics variables
	physicsObj.SetMaxStepHeight( pm_stepsize.GetFloat() );
	physicsObj.SetMaxJumpHeight( pm_jumpheight.GetFloat() );

	if ( noclip || IsRiding() ) { //ff1.1
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_NOCLIP );
	} else if ( spectating ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_SPECTATOR );
	} else if ( health <= 0 ) {
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
		physicsObj.SetMovementType( PM_DEAD );
	} else if ( gameLocal.inCinematic || gameLocal.GetCamera() || privateCameraView || ( influenceActive == INFLUENCE_LEVEL2 ) ) {
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_FREEZE );
#ifdef _D3XP
	} else if ( mountedObject ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_FREEZE );
#endif
	} else {
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_NORMAL );
	}

	if ( spectating ) {
		physicsObj.SetClipMask( MASK_DEADSOLID );
	} else if ( health <= 0 ) {
		physicsObj.SetClipMask( MASK_DEADSOLID );
	} else {
		physicsObj.SetClipMask( MASK_PLAYERSOLID );
	}

	physicsObj.SetDebugLevel( g_debugMove.GetBool() );
	physicsObj.SetPlayerInput( usercmd, viewAngles );

	// FIXME: physics gets disabled somehow
	BecomeActive( TH_PHYSICS );
	RunPhysics();

	// update our last valid AAS location for the AI
	SetAASLocation();

	if ( spectating ) {
		newEyeOffset = 0.0f;
	} else if ( health <= 0 ) {
		newEyeOffset = pm_deadviewheight.GetFloat();
	} else if ( physicsObj.IsCrouching() ) {
		newEyeOffset = pm_crouchviewheight.GetFloat();
	} else if ( GetBindMaster() && GetBindMaster()->IsType( idAFEntity_Vehicle::Type ) ) {
		newEyeOffset = 0.0f;
	} else {
		newEyeOffset = pm_normalviewheight.GetFloat();
	}

	if ( EyeHeight() != newEyeOffset ) {
		if ( spectating ) {
			SetEyeHeight( newEyeOffset );
		} else {
			// smooth out duck height changes
			SetEyeHeight( EyeHeight() * pm_crouchrate.GetFloat() + newEyeOffset * ( 1.0f - pm_crouchrate.GetFloat() ) );
		}
	}

	if ( noclip || gameLocal.inCinematic || ( influenceActive == INFLUENCE_LEVEL2 ) || IsRiding() ) { //ff1.1
		AI_CROUCH	= false;
		AI_ONGROUND	= ( influenceActive == INFLUENCE_LEVEL2 );
		AI_ONLADDER	= false;
		AI_JUMP		= false;
		AI_DODGE	= false;
	} else {
		AI_CROUCH	= physicsObj.IsCrouching();
		AI_ONGROUND	= physicsObj.HasGroundContacts();
		AI_ONLADDER	= physicsObj.OnLadder();
		AI_JUMP		= physicsObj.HasJumped();
		AI_DODGE	= physicsObj.HasDodged();

		//ff1.3 start - don't do this
		/*
		// check if we're standing on top of a monster and give a push if we are
		idEntity *groundEnt = physicsObj.GetGroundEntity();
		if ( groundEnt && groundEnt->IsType( idAI::Type ) ) {
			idVec3 vel = physicsObj.GetLinearVelocity();
			if ( vel.ToVec2().LengthSqr() < 0.1f ) {
				vel.ToVec2() = physicsObj.GetOrigin().ToVec2() - groundEnt->GetPhysics()->GetAbsBounds().GetCenter().ToVec2();
				vel.ToVec2().NormalizeFast();
				vel.ToVec2() *= pm_walkspeed.GetFloat();
			} else {
				// give em a push in the direction they're going
				vel *= 1.1f;
			}
			physicsObj.SetLinearVelocity( vel );
		}
		*/
		//ff1.3 end
	}

	if ( AI_JUMP ) {
		// bounce the view weapon
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[2] = 200;
		acc->dir[0] = acc->dir[1] = 0;
	}

	if ( AI_ONLADDER ) {
		int old_rung = oldOrigin.z / LADDER_RUNG_DISTANCE;
		int new_rung = physicsObj.GetOrigin().z / LADDER_RUNG_DISTANCE;

		if ( old_rung != new_rung ) {
			StartSound( "snd_stepladder", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}

	if ( AI_DODGE ) {
		StartSound( "snd_dodge", SND_CHANNEL_ANY, 0, false, NULL );
	}

	BobCycle( pushVelocity );
	CrashLand( oldOrigin, oldVelocity );
}

#define PICKUP_ITEM_SLOTS 5

/*
==============
idPlayer::UpdateHud
==============
*/
void idPlayer::UpdateHud( void ) {
	idPlayer *aimed;

	if ( !hud ) {
		return;
	}

	if ( entityNumber != gameLocal.localClientNum ) {
		return;
	}

	//ff1.3 start
	if ( !inventory.lastFullItemName.IsEmpty() ) {
		hud->SetStateString("itemFullText", va("%s Full", inventory.lastFullItemName.c_str()) );
		hud->HandleNamedEvent("itemFull");
		inventory.lastFullItemName.Empty();
	}
	//ff1.3 end

	int c = inventory.pickupItemNames.Num();
	if ( c > 0 ) {
		if ( gameLocal.time > inventory.nextItemPickup ) {
			//ff1.3 start
			if ( inventory.nextItemPickup && gameLocal.time - inventory.nextItemPickup > 2000 ) { //restart from 1 after an empty period
				inventory.nextItemNum = 1;
				//gameLocal.Printf("reset to 1\n");
			}
			else if ( ( gameLocal.time - inventory.lastShownItemTime < 1500 ) //must match visible time in gui
					&& ( inventory.lastShownItem.amount > 0 )
					&& ( inventory.lastShownItem.name.Icmp( inventory.pickupItemNames[0].name ) == 0 ) ) {
				inventory.pickupItemNames[0].amount += inventory.lastShownItem.amount;
				inventory.nextItemNum--; //back to the last used slot
				if ( inventory.nextItemNum < 1 ) {
					inventory.nextItemNum = PICKUP_ITEM_SLOTS;
				}
				//gameLocal.Printf("use the same slot\n");
			}
			inventory.lastShownItem = inventory.pickupItemNames[c-1];
			inventory.lastShownItemTime = gameLocal.time;

			int i;

			for ( i = 0; i < c && i < 5; i++ ) {
				if (inventory.nextItemNum == 1 ) {
					inventory.onePickupTime = gameLocal.time;
					//gameLocal.Printf("onePickupTime\n");
				}
				//gameLocal.Printf("nextItemNum %i\n", inventory.nextItemNum);
				if ( gameLocal.time > 1000 ) { //ff1.3 - just like weapons, don't show ammo given at map start
					if ( inventory.pickupItemNames[0].amount > 0 ) {
						hud->SetStateString( va( "itemtext%i", inventory.nextItemNum ), va("%s +%i", inventory.pickupItemNames[0].name.c_str(), inventory.pickupItemNames[0].amount) );
					} else {
						hud->SetStateString( va( "itemtext%i", inventory.nextItemNum ), inventory.pickupItemNames[0].name );
					}
					//hud->SetStateString( va( "itemamount%i", inventory.nextItemNum ),
						//( inventory.pickupItemNames[0].amount > 0 ) ? va("+ %i", inventory.pickupItemNames[0].amount) : ""); //ff1.3

					hud->SetStateString( va( "itemicon%i", inventory.nextItemNum ), inventory.pickupItemNames[0].icon );
					hud->HandleNamedEvent( va( "itemPickup%i", inventory.nextItemNum++ ) );
				}
				inventory.pickupItemNames.RemoveIndex( 0 );

				if ( inventory.nextItemNum > PICKUP_ITEM_SLOTS ) {
					inventory.nextItemNum = 1;
					inventory.nextItemPickup = inventory.onePickupTime + 2000;
				} else {
					inventory.nextItemPickup = gameLocal.time + 400;
				}
			}


			/* was:
			if ( inventory.nextItemPickup && gameLocal.time - inventory.nextItemPickup > 2000 ) {
				inventory.nextItemNum = 1;
			}
			int i, count = 5;
#ifdef _D3XP
			if (gameLocal.isMultiplayer) {
				count = 3;
			}

			if (count < c)
				c = count;
#endif
			for ( i = 0; i < c; i++ ) { //_D3XP
				hud->SetStateString( va( "itemtext%i", inventory.nextItemNum ), inventory.pickupItemNames[0].name );
				hud->SetStateString( va( "itemicon%i", inventory.nextItemNum ), inventory.pickupItemNames[0].icon );
				hud->HandleNamedEvent( va( "itemPickup%i", inventory.nextItemNum++ ) );
				inventory.pickupItemNames.RemoveIndex( 0 );

				if (inventory.nextItemNum == 1 ) {
					inventory.onePickupTime = gameLocal.time;
				} else	if ( inventory.nextItemNum > count ) { //_D3XP
					inventory.nextItemNum = 1;
					inventory.nextItemPickup = inventory.onePickupTime + 2000;
				} else {
					inventory.nextItemPickup = gameLocal.time + 400;
				}
			}
			*/
			//ff1.3 end
		}
	}

	if ( gameLocal.realClientTime == lastMPAimTime ) {
		if ( MPAim != -1 && gameLocal.mpGame.IsGametypeTeamBased() /* CTF */
			&& gameLocal.entities[ MPAim ] && gameLocal.entities[ MPAim ]->IsType( idPlayer::Type )
			&& static_cast< idPlayer * >( gameLocal.entities[ MPAim ] )->team == team ) {
				aimed = static_cast< idPlayer * >( gameLocal.entities[ MPAim ] );
				hud->SetStateString( "aim_text", gameLocal.userInfo[ MPAim ].GetString( "ui_name" ) );
				hud->SetStateFloat( "aim_color", aimed->colorBarIndex );
				hud->HandleNamedEvent( "aim_flash" );
				MPAimHighlight = true;
				MPAimFadeTime = 0;	// no fade till loosing focus
		} else if ( MPAimHighlight ) {
			hud->HandleNamedEvent( "aim_fade" );
			MPAimFadeTime = gameLocal.realClientTime;
			MPAimHighlight = false;
		}
	}
	if ( MPAimFadeTime ) {
		assert( !MPAimHighlight );
		if ( gameLocal.realClientTime - MPAimFadeTime > 2000 ) {
			MPAimFadeTime = 0;
		}
	}

	/* ff1.3 - Moved to stats
	hud->SetStateInt( "g_showProjectilePct", g_showProjectilePct.GetInteger() );
	if ( projFired ) {
		hud->SetStateString( "projectilepct", va( "Hit %% %.1f", ( (float) projHits / projFired ) * 100 ) );
	} else {
		hud->SetStateString( "projectilepct", "Hit % 0.0" );
	}
	*/

	if ( isLagged && gameLocal.isMultiplayer && gameLocal.localClientNum == entityNumber ) {
		hud->SetStateString( "hudLag", "1" );
	} else {
		hud->SetStateString( "hudLag", "0" );
	}
}

/*
==============
idPlayer::UpdateDeathSkin
==============
*/
void idPlayer::UpdateDeathSkin( bool state_hitch ) {
	if ( !( gameLocal.isMultiplayer || g_testDeath.GetBool() ) ) {
		return;
	}
	if ( health <= 0 ) {
		if ( !doingDeathSkin ) {
			deathClearContentsTime = spawnArgs.GetInt( "deathSkinTime" );
			doingDeathSkin = true;
			renderEntity.noShadow = true;
			if ( state_hitch ) {
				renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f - 2.0f;
			} else {
				renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
			}
			UpdateVisuals();
		}

		// wait a bit before switching off the content
		if ( deathClearContentsTime && gameLocal.time > deathClearContentsTime ) {
			SetCombatContents( false );
			deathClearContentsTime = 0;
		}
	} else {
		renderEntity.noShadow = false;
		renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = 0.0f;
		UpdateVisuals();
		doingDeathSkin = false;
	}
}

/*
==============
idPlayer::StartFxOnBone
==============
*/
void idPlayer::StartFxOnBone( const char *fx, const char *bone ) {
	idVec3 offset;
	idMat3 axis;
	jointHandle_t jointHandle = GetAnimator()->GetJointHandle( bone );

	if ( jointHandle == INVALID_JOINT ) {
		gameLocal.Printf( "Cannot find bone %s\n", bone );
		return;
	}

	if ( GetAnimator()->GetJointTransform( jointHandle, gameLocal.time, offset, axis ) ) {
		offset = GetPhysics()->GetOrigin() + offset * GetPhysics()->GetAxis();
		axis = axis * GetPhysics()->GetAxis();
	}

	idEntityFx::StartFx( fx, &offset, &axis, this, true );
}

void idPlayer::StartAdvancedWeaponZoom( void ) {
	advancedWeaponZooming = true; //will go back to false when zoom-out if fully completed
	StartSound( "snd_zoomin", SND_CHANNEL_ANY, 0, false, NULL );
	zoomFov.Init( gameLocal.time, 1000.0f, DefaultZoomFov(), weapon.GetEntity()->GetZoomFov() );
	advancedWeaponZoomTime = gameLocal.time;
	if ( hud ) {
		hud->HandleNamedEvent( "weaponZoomOn" );
	}
}

void idPlayer::StopAdvancedWeaponZoom( void ) {
	StartSound( "snd_zoomout", SND_CHANNEL_ANY, 0, false, NULL );
	zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( advancedWeaponZoomTime ), DefaultFov() );
	advancedWeaponZoomTime = 0;
}

void idPlayer::CompleteAdvancedWeaponZoom( void ) {
	advancedWeaponZooming = false;
	if ( hud ) {
		hud->HandleNamedEvent( "weaponZoomOff" );
	}
}


/*
==============
idPlayer::Think

Called every tic for each player
==============
*/
void idPlayer::Think( void ) {
	renderEntity_t *headRenderEnt;

	UpdatePlayerIcons();

	// latch button actions
	oldButtons = usercmd.buttons;

	// grab out usercmd
	usercmd_t oldCmd = usercmd;
	usercmd = gameLocal.usercmds[ entityNumber ];
	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;

	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		lastEscPressedTime = gameLocal.time; //like if ESC kept pressed
		return;
	}

	if ( skipCinematicTipStartTime > 0
		&& gameLocal.fast.time > lastEscPressedTime + ESC_PRESSED_TIMEOUT //not pressed anymore
		&& gameLocal.fast.time > skipCinematicTipStartTime + SKIP_CIN_MIN_WAIT_TIME ){ //above min threshold

		HideSkipCinematicTip();
	}

	// clear the ik before we do anything else so the skeleton doesn't get updated twice
	walkIK.ClearJointMods();

	// if this is the very first frame of the map, set the delta view angles
	// based on the usercmd angles
	if ( !spawnAnglesSet && ( gameLocal.GameState() != GAMESTATE_STARTUP ) ) {
		spawnAnglesSet = true;
		SetViewAngles( spawnAngles );
		oldFlags = usercmd.flags;
	}

#ifdef _D3XP
	if ( mountedObject ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}
#endif

	if ( objectiveSystemOpen || gameLocal.inCinematic || influenceActive ) {
		if ( objectiveSystemOpen && AI_PAIN ) {
			TogglePDA();
		}
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;

		//ff1.3 start
		if ( objectiveSystemOpen && objectiveSystem != NULL ) {
			UpdatePDAStats( objectiveSystem );
		}
		//ff1.3 end
	}

	// log movement changes for weapon bobbing effects
	if ( usercmd.forwardmove != oldCmd.forwardmove ) {
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[0] = usercmd.forwardmove - oldCmd.forwardmove;
		acc->dir[1] = acc->dir[2] = 0;
	}

	if ( usercmd.rightmove != oldCmd.rightmove ) {
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[1] = usercmd.rightmove - oldCmd.rightmove;
		acc->dir[0] = acc->dir[2] = 0;
	}

	// freelook centering
	if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_MLOOK ) {
		centerView.Init( gameLocal.time, 200, viewAngles.pitch, 0 );
	}

	//ff1.3 start
	if ( currentVehicle ) {
		if ( usercmd.upmove > 0 || noclip ) {
			ExitVehicle();
		}
	} else {
		if ( focusVehicle && !noclip && nextVehicleTime < gameLocal.time ) {
			EnterVehicle( focusVehicle );
		}
	}

	//vehicle zooming - use secondary fire as zoom
	if ( currentVehicle && usercmd.buttons & BUTTON_5 ) {
		usercmd.buttons |= BUTTON_ZOOM;
	}

	// weapon zooming
	if ( weaponZoom.startZoom && !weaponZoom.quickMode && weapon.GetEntity() && weaponEnabled && !gameLocal.inCinematic ) {
		if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_5 ) {
			if ( usercmd.buttons & BUTTON_5 ) { //down
				if ( advancedWeaponZoomTime ) { //stop
					StopAdvancedWeaponZoom();
				} else { //start
					StartAdvancedWeaponZoom();
				}
			}
			/*
			else if ( advancedWeaponZoomTime ) { //up
				gameLocal.Printf("zoom wait\n");
			}
			*/
		}else if ( advancedWeaponZoomTime && ( usercmd.buttons & BUTTON_5 ) ){ //holding
			advancedWeaponZoomTime = gameLocal.time;
		}
	} else if ( advancedWeaponZoomTime ) { //stop
		StopAdvancedWeaponZoom();
	}

	if ( advancedWeaponZooming && !advancedWeaponZoomTime && zoomFov.IsDone( gameLocal.time ) ) { //weapon zoom-out fully completed
		CompleteAdvancedWeaponZoom();
		if ( usercmd.buttons & BUTTON_ZOOM ) { //fix for standard zoom button being pressed while weapon zoom ends
			zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), DefaultZoomFov() );
		}
	} else if ( !advancedWeaponZooming ) {
		// player zooming
		if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_ZOOM) {
			if ( usercmd.buttons & BUTTON_ZOOM ) {
				zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), DefaultZoomFov() );
			} else {
				zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
			}
		}

#ifdef _DENTONMOD
		// simple weapon zooming, initiated by weapon script
		if ( weaponZoom.quickMode && ( weaponZoom.oldZoomStatus ^ weaponZoom.startZoom ) ) {
			if ( weaponZoom.startZoom && weapon.GetEntity() ) {
				weaponZoom.oldZoomStatus = true;
				zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), weapon.GetEntity()->GetZoomFov() );
			} else {
				weaponZoom.oldZoomStatus = false;
				zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
			}
		}
#endif //_DENTONMOD_PLAYER_CPP

	}

	/*
	// zooming
	if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_ZOOM ) {
		if ( ( usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() ) {
			zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), weapon.GetEntity()->GetZoomFov() );
		} else {
			zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
		}
	}

#ifdef _DENTONMOD
	// zooming, initiated by weapon script
	if ( ( weaponZoom.oldZoomStatus ^ weaponZoom.startZoom ) ) {
		if ( weaponZoom.startZoom && weapon.GetEntity() ) {
			weaponZoom.oldZoomStatus = true;
			zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), weapon.GetEntity()->GetZoomFov() );
		} else {
			weaponZoom.oldZoomStatus = false;
			zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
		}
	}
#endif //_DENTONMOD_PLAYER_CPP
	*/
	//ff1.3 end

	// if we have an active gui, we will unrotate the view angles as
	// we turn the mouse movements into gui events
	idUserInterface *gui = ActiveGui();
	if ( gui && gui != focusUI ) {
		RouteGuiMouse( gui );
	}

	// set the push velocity on the weapon before running the physics
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->SetPushVelocity( physicsObj.GetPushedLinearVelocity() );
	}

	EvaluateControls();

	if ( !af.IsActive() ) {
		AdjustBodyAngles();
		CopyJointsFromBodyToHead();
	}

	Move();

	if ( !g_stopTime.GetBool() ) {

		if ( !noclip && !spectating && ( health > 0 ) && !IsHidden() ) {
			TouchTriggers();
		}

		// not done on clients for various reasons. don't do it on server and save the sound channel for other things
		if ( !gameLocal.isMultiplayer ) {
			SetCurrentHeartRate();
#ifdef _D3XP
			float scale = new_g_damageScale;
#else
			float scale = g_damageScale.GetFloat();
#endif
			if ( g_useDynamicProtection.GetBool() && scale < 1.0f && gameLocal.time - lastDmgTime > 500 ) {
				if ( scale < 1.0f ) {
					scale += 0.05f;
				}
				if ( scale > 1.0f ) {
					scale = 1.0f;
				}
#ifdef _D3XP
				new_g_damageScale = scale;
#else
				g_damageScale.SetFloat( scale );
#endif
			}
		}

		// update GUIs, Items, and character interactions
		UpdateFocus();

		UpdateLocation();

		// update player script
		UpdateScript();

		// service animations
		if ( !spectating && !af.IsActive() && !gameLocal.inCinematic ) {
			UpdateConditions();
			UpdateAnimState();
			CheckBlink();
		}

		// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
		AI_PAIN = false;
	}

	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();

	// this may use firstPersonView, or a thirdPeroson / camera view
	CalculateRenderView();

	inventory.UpdateArmor();

	if ( spectating ) {
		UpdateSpectating();
	} else if ( health > 0 ) {
		UpdateWeapon();
	}

	//UpdateAir(); //ff1.3 - removed

#ifdef _D3XP
	UpdatePowerupHud();
#endif

    if ( health > inventory.maxHealth && nextHealthDrop < gameLocal.time) {
        health--;
        nextHealthDrop = gameLocal.time + pm_exceedingHealthRate.GetInteger();
    }

    UpdateKillsMedals(); //ff1.3
    UpdateMedalsQueue(); //ff1.3

	UpdateHud();

	UpdatePowerUps();

	UpdateDeathSkin( false );

	if ( gameLocal.isMultiplayer ) {
		DrawPlayerIcons();

#ifdef _D3XP
		if ( enviroSuitLight.IsValid() ) {
			idAngles lightAng = firstPersonViewAxis.ToAngles();
			idVec3 lightOrg = firstPersonViewOrigin;
			const idDict *lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );

			idVec3 enviroOffset = lightDef->GetVector( "enviro_offset" );
			idVec3 enviroAngleOffset = lightDef->GetVector( "enviro_angle_offset" );

			lightOrg += (enviroOffset.x * firstPersonViewAxis[0]);
			lightOrg += (enviroOffset.y * firstPersonViewAxis[1]);
			lightOrg += (enviroOffset.z * firstPersonViewAxis[2]);
			lightAng.pitch += enviroAngleOffset.x;
			lightAng.yaw += enviroAngleOffset.y;
			lightAng.roll += enviroAngleOffset.z;

			enviroSuitLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
			enviroSuitLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
			enviroSuitLight.GetEntity()->UpdateVisuals();
			enviroSuitLight.GetEntity()->Present();
		}
#endif
	}

	if ( head.GetEntity() ) {
		headRenderEnt = head.GetEntity()->GetRenderEntity();
	} else {
		headRenderEnt = NULL;
	}

	if ( headRenderEnt ) {
		if ( influenceSkin ) {
			headRenderEnt->customSkin = influenceSkin;
		} else {
			headRenderEnt->customSkin = NULL;
		}
	}

	if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() ) {
		renderEntity.suppressShadowInViewID	= 0;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = 0;
		}
	} else {
		renderEntity.suppressShadowInViewID	= entityNumber+1;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = entityNumber+1;
		}
	}
	// never cast shadows from our first-person muzzle flashes
	renderEntity.suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	if ( headRenderEnt ) {
		headRenderEnt->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	}

	if ( !g_stopTime.GetBool() ) {
		UpdateAnimation();

		Present();

		UpdateDamageEffects();

		LinkCombat();

		playerView.CalculateShake();
	}

	if ( !( thinkFlags & TH_THINK ) ) {
		gameLocal.Printf( "player %d not thinking?\n", entityNumber );
	}

	if ( g_showEnemies.GetBool() ) {
		idActor *ent;
		int num = 0;
		for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
			gameLocal.Printf( "enemy (%d)'%s'\n", ent->entityNumber, ent->name.c_str() );
			gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds().Expand( 2 ), ent->GetPhysics()->GetOrigin() );
			num++;
		}
		gameLocal.Printf( "%d: enemies\n", num );
	}

#ifdef _D3XP
	//inventory.RechargeAmmo(this); //ff1.3 - removed

	if (healthRecharge) {
		int elapsed = gameLocal.time - lastHealthRechargeTime;
		if (elapsed >= rechargeSpeed) {
			int intervals = (gameLocal.time - lastHealthRechargeTime)/rechargeSpeed;
			Give("health", va("%d", intervals));
			lastHealthRechargeTime += intervals*rechargeSpeed;
		}
	}

	// determine if portal sky is in pvs
	gameLocal.portalSkyActive = gameLocal.pvs.CheckAreasForPortalSky( gameLocal.GetPlayerPVS(), GetPhysics()->GetOrigin() );
#endif
}

#ifdef _D3XP
/*
=================
idPlayer::StartHealthRecharge
=================
*/
void idPlayer::StartHealthRecharge(int speed) {
	lastHealthRechargeTime = gameLocal.time;
	healthRecharge = true;
	rechargeSpeed = speed;
}

/*
=================
idPlayer::StopHealthRecharge
=================
*/
void idPlayer::StopHealthRecharge() {
	healthRecharge = false;
}

/*
=================
idPlayer::GetCurrentWeapon
=================
*/
idStr idPlayer::GetCurrentWeapon() {
	const char *weapon;

	if ( currentWeapon >= 0 ) {
		weapon = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
		return weapon;
	} else {
		return "";
	}
}

/*
=================
idPlayer::CanGive
=================
*/
bool idPlayer::CanGive( const char *statname, const char *value ) {
	if ( AI_DEAD ) {
		return false;
	}

	//ff1.3 - was: if ( !idStr::Icmp( statname, "health" ) ) {
	if ( !idStr::Icmpn( statname, "health", 6 ) ) {
		if ( health >= inventory.maxHealth ) {
			return false;
		}
		return true;
	}
	else if ( !idStr::Icmp( statname, "takeme" )){ // jetpack and possession
		return true;
	}
	//ff1.1 end

	//ff1.3 start - disable stamina
	/* was:
	else if ( !idStr::Icmp( statname, "stamina" ) ) {
		if ( stamina >= 100 ) {
			return false;
		}
		return true;

	}
	*/
	else if ( !idStr::Icmp( statname, "soul" )){
		return true;
	}
	else if ( !idStr::Icmp( statname, "staminaHelltime" )){
		return true;
	}
	else if ( !idStr::Icmp( statname, "monsterHealth" )){
		return (currentRiddenAI != NULL);
	}
	//ff1.3 end
	else if ( !idStr::Icmp( statname, "heartRate" ) ) {
		return true;
	/* removed by ff1.3
	} else if ( !idStr::Icmp( statname, "air" ) ) {
		if ( airTics >= pm_airTics.GetInteger() ) {
			return false;
		}
		return true;
	*/
	}

	return inventory.CanGive( this, spawnArgs, statname, value, &idealWeapon );
}

/*
=================
idPlayer::StopHelltime

provides a quick non-ramping way of stopping helltime
=================

void idPlayer::StopHelltime( bool quick ) {
	if ( !PowerUpActive( HELLTIME ) ) {
		return;
	}

	if ( PowerUpActive( HELLTIME ) ) {
		ClearPowerup( HELLTIME );
	}

	//ff1.3 start - commented out: loop sound is now stopped by exit sound when pw is disabled
	// stop the looping sound
	//StopSound( SND_CHANNEL_DEMONIC, false );
	//ff1.3 end

	// reset the game vars
	if ( quick ) {
		gameLocal.QuickSlowmoReset();
	}
}
*/

/*
=================
idPlayer::Event_ToggleBloom
=================
*/
void idPlayer::Event_ToggleBloom( int on ) {
	EnableBloom( on > 0 );
}

/*
=================
idPlayer::EnableBloom
=================
*/
void idPlayer::EnableBloom( bool on ) {
	bloomEnabled = on;
}


/*
=================
idPlayer::Event_SetBloomParms
=================
*/
void idPlayer::Event_SetBloomParms( float speed, float intensity ) {
	bloomSpeed = speed;
	bloomIntensity = intensity;
}

#endif

/*
=================
idPlayer::RouteGuiMouse
=================
*/
void idPlayer::RouteGuiMouse( idUserInterface *gui ) {
	sysEvent_t ev;

	if ( usercmd.mx != oldMouseX || usercmd.my != oldMouseY ) {
		ev = sys->GenerateMouseMoveEvent( usercmd.mx - oldMouseX, usercmd.my - oldMouseY );
		gui->HandleEvent( &ev, gameLocal.time );
		oldMouseX = usercmd.mx;
		oldMouseY = usercmd.my;
	}
}

/*
==================
idPlayer::LookAtKiller
==================
*/
void idPlayer::LookAtKiller( idEntity *inflictor, idEntity *attacker ) {
	idVec3 dir;

	if ( attacker && attacker != this ) {
		dir = attacker->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	} else if ( inflictor && inflictor != this ) {
		dir = inflictor->GetPhysics()->GetOrigin() - GetPhysics()->GetOrigin();
	} else {
		dir = viewAxis[ 0 ];
	}

	idAngles ang( 0, dir.ToYaw(), 0 );
	SetViewAngles( ang );
}

/*
==============
idPlayer::Kill
==============
*/
void idPlayer::Kill( bool delayRespawn, bool nodamage ) {
	if ( spectating ) {
		SpectateFreeFly( false );
	} else if ( health > 0 ) {
		godmode = false;
		if ( nodamage ) {
			ServerSpectate( true );
			forceRespawn = true;
		} else {
			//ff1.3 start
			if ( currentRiddenAI ) {
				currentRiddenAI->UnbindDriver(true);
			}
			if ( currentVehicle ) {
				ExitVehicle();
			}
			//ff1.3 end
			Damage( this, this, vec3_origin, "damage_suicide", 1.0f, INVALID_JOINT );
			if ( delayRespawn ) {
				forceRespawn = false;
				int delay = spawnArgs.GetFloat( "respawn_delay" );
				minRespawnTime = gameLocal.time + SEC2MS( delay );
				maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
			}
		}
	}
}

/*
==================
idPlayer::Killed
==================
*/
void idPlayer::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
	float delay;

	assert( !gameLocal.isClient );

	//ff1.3 start - stop dmgfx
	idActor::Killed(inflictor, attacker, damage, dir, location);
	//ff1.3 end

	// stop taking knockback once dead
	fl.noknockback = true;
	if ( health < -999 ) {
		health = -999;
	}

	if ( AI_DEAD ) {
		AI_PAIN = true;
		return;
	}

	heartInfo.Init( 0, 0, 0, BASE_HEARTRATE );
	AdjustHeartRate( DEAD_HEARTRATE, 10.0f, 0.0f, true );

	if ( !g_testDeath.GetBool() ) {
		playerView.Fade( colorBlack, 12000 );
	}

	AI_DEAD = true;
	SetAnimState( ANIMCHANNEL_LEGS, "Legs_Death", 4 );
	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Death", 4 );
	SetWaitState( "" );

	animator.ClearAllJoints();

	if ( StartRagdoll() ) {
		pm_modelView.SetInteger( 0 );
		minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME;
		maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
	} else {
		// don't allow respawn until the death anim is done
		// g_forcerespawn may force spawning at some later time
		delay = spawnArgs.GetFloat( "respawn_delay" );
		minRespawnTime = gameLocal.time + SEC2MS( delay );
		maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
	}

	physicsObj.SetMovementType( PM_DEAD );
	StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
	StopSound( SND_CHANNEL_BODY2, false );

	fl.takedamage = true;		// can still be gibbed

	// get rid of weapon
	weapon.GetEntity()->OwnerDied();

	// drop the weapon as an item
	DropWeapon( true );

#ifdef CTF
	// drop the flag if player was carrying it
	if ( gameLocal.isMultiplayer && gameLocal.mpGame.IsGametypeFlagBased() &&
		 carryingFlag )
	{
		DropFlag();
	}
#endif

	if ( !g_testDeath.GetBool() ) {
		LookAtKiller( inflictor, attacker );
	}

	if ( gameLocal.isMultiplayer || g_testDeath.GetBool() ) {
		idPlayer *killer = NULL;
		// no gibbing in MP. Event_Gib will early out in MP
		if ( attacker->IsType( idPlayer::Type ) ) {
			killer = static_cast<idPlayer*>(attacker);
			if ( health < -20 || killer->PowerUpActive( BERSERK ) ) {
				gibDeath = true;
				gibsDir = dir;
				gibsLaunched = false;
			}
		}
		gameLocal.mpGame.PlayerDeath( this, killer, isTelefragged );
	} else {
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
	}

	ClearPowerUps();

	UpdateVisuals();

	isChatting = false;
}

/*
=====================
idPlayer::GetAIAimTargets

Returns positions for the AI to aim at.
=====================
*/
void idPlayer::GetAIAimTargets( const idVec3 &lastSightPos, idVec3 &headPos, idVec3 &chestPos ) {
	idVec3 offset;
	idMat3 axis;
	idVec3 origin;

	origin = lastSightPos - physicsObj.GetOrigin();

	GetJointWorldTransform( chestJoint, gameLocal.time, offset, axis );
	headPos = offset + origin;

	GetJointWorldTransform( headJoint, gameLocal.time, offset, axis );
	chestPos = offset + origin;
}

/*
================
idPlayer::DamageFeedback

callback function for when another entity received damage from this entity.  damage can be adjusted and returned to the caller.
================
*/
void idPlayer::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	assert( !gameLocal.isClient );
	damage *= PowerUpModifier( BERSERK );
	if ( damage && ( victim != this ) && ( victim->IsType( idActor::Type ) || victim->IsType( idDamagable::Type ) ) ) {

		// ff1.4 - generalize old code to work on actors

		/* was:
		idPlayer *victimPlayer = NULL;

		//No damage feedback sound for hitting friendlies in CTF
		if ( victim->IsType( idPlayer::Type ) ) {
			victimPlayer = static_cast<idPlayer*>(victim);
		}

		if ( gameLocal.mpGame.IsGametypeFlagBased() && victimPlayer && this->team == victimPlayer->team ) {
			// Do nothing
		}
		*/

		idActor *victimActor = NULL;

		//No damage feedback sound for hitting friendlies in CTF
		if ( victim->IsType( idActor::Type ) ) {
			victimActor = static_cast<idActor*>(victim);
		}

		if ( gameLocal.mpGame.IsGametypeFlagBased() && victimActor && this->team == victimActor->team ) {
			// Do nothing
		}
		else if ( inflictor && inflictor->IsType( idDamagingFx::Type ) && this->team == victimActor->team ) {
			 // No damage feedback for dmgFx on friendlies
		}
		// ff1.4  end
		else {
			SetLastHitTime( gameLocal.time );
		}
	}
}

/*
=================
idPlayer::CalcDamagePoints

Calculates how many health and armor points will be inflicted, but
doesn't actually do anything with them.  This is used to tell when an attack
would have killed the player, possibly allowing a "saving throw"
=================
*/
void idPlayer::CalcDamagePoints( idEntity *inflictor, idEntity *attacker, const idDict *damageDef,
							   const float damageScale, const int location, int *health, int *armor ) {
	int		damage;
	int		armorSave;

	damageDef->GetInt( "damage", "0", damage );
	damage = GetDamageForLocation( damage, location );
	//gameLocal.Printf("damage start %d\n", damage);

	//ff1.3 start
	//half damage while bullet time is active
	if ( PowerUpActive( HELLTIME ) ) {
		damage *= 0.50f;
		if ( damage < 1 ) {
			damage = 1;
		}
	}
	//ff1.3 end

	idPlayer *player = attacker->IsType( idPlayer::Type ) ? static_cast<idPlayer*>(attacker) : NULL;
	if ( !gameLocal.isMultiplayer ) {
		if ( inflictor != gameLocal.world ) {
			switch ( g_skill.GetInteger() ) {
				case 0:
					damage *= 0.90f; //ff1.3 - was: 0.8
					if ( damage < 1 ) {
						damage = 1;
					}
					break;
				case 1: //ff1.3
					damage *= 1.10f;
				case 2:
					damage *= 1.70f;
					break;
				case 3:
					damage *= 3.5f;
					break;
				default:
					break;
			}
		}
	}

	//gameLocal.Printf("damage %d, damageScale %f, playerDamageScale %f\n", damage, damageScale, damageDef->GetFloat( "playerDamageScale", "1" ));
	damage = (int)ceil( damage * damageScale * damageDef->GetFloat( "playerDamageScale", "1" ) ); //ff1.3 - extra scale for players
	//gameLocal.Printf("damage -> %d\n", damage);

	// always give half damage if hurting self
	if ( attacker == this ) {
		if ( gameLocal.isMultiplayer ) {
			// only do this in mp so single player plasma and rocket splash is very dangerous in close quarters
			damage *= damageDef->GetFloat( "selfDamageScale", "0.5" );
		} else {
			damage *= damageDef->GetFloat( "selfDamageScale", "1" );
		}
	}

	// check for completely getting out of the damage
	if ( !damageDef->GetBool( "noGod" ) ) {
		// check for godmode
		if ( godmode ) {
			damage = 0;
		}
#ifdef _D3XP
		//Invulnerability is just like god mode
		if( PowerUpActive( INVULNERABILITY ) ) {
			damage = 0;
		}
#endif
	}

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );

	// save some from armor
	if ( !damageDef->GetBool( "noArmor" ) ) {
		float armor_protection;

		armor_protection = ( gameLocal.isMultiplayer ) ? g_armorProtectionMP.GetFloat() : g_armorProtection.GetFloat();

		armorSave = ceil( damage * armor_protection );
		if ( armorSave >= inventory.armor ) {
			armorSave = inventory.armor;
		}

		if ( !damage ) {
			armorSave = 0;
		} else if ( armorSave >= damage && damage > 1 ) { //ff1.3: "damage > 1" added - fix for damage = 1 not removing armor (eg: bfg)
			armorSave = damage - 1;
			damage = 1;
		} else {
			damage -= armorSave;
		}
	} else {
		armorSave = 0;
	}
	//gameLocal.Printf("damage after armor -> %d\n", damage);

	// check for team damage
	if ( gameLocal.mpGame.IsGametypeTeamBased() /* CTF */
		&& !gameLocal.serverInfo.GetBool( "si_teamDamage" )
		&& !damageDef->GetBool( "noTeam" )
		&& player
		&& player != this		// you get self damage no matter what
		&& player->team == team ) {
			damage = 0;
	}

	*health = damage;
	*armor = armorSave;
}

/*
============
Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space

damageDef	an idDict with all the options for damage effects

inflictor, attacker, dir, and point can be NULL for environmental effects
============
*/
void idPlayer::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir,
					   const char *damageDefName, const float damageScale, const int location ) {
	idVec3		kick;
	int			damage;
	int			armorSave;
	int			knockback;
	idVec3		damage_from;
	idVec3		localDamageVector;
	float		attackerPushScale;
#ifdef _D3XP
	SetTimeState ts( timeGroup );
#endif

	// damage is only processed on server
	if ( gameLocal.isClient ) {
		return;
	}

	if ( !fl.takedamage || noclip || spectating || gameLocal.inCinematic
        || IsRiding() || privateCameraView ) { //v1.3 - no damage to player while riding or teleporting
        return;
    }

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	if ( attacker->IsType( idAI::Type ) ) {
#ifndef _D3XP
		if ( PowerUpActive( BERSERK ) ) {
			return;
		}
#endif

		//ff start - always ignore damage from friends
		if ( static_cast<idAI *>(attacker)->team == 0 ) {
			return;
		}
		//ff end

		// don't take damage from monsters during influences
		if ( influenceActive != 0 ) {
			return;
		}
	}
	//ff1.3 start -  handle projectiles fired by already-removed friends (rare, but could happen)
	else if ( inflictor->IsType( idProjectile::Type ) && static_cast<idProjectile *>(inflictor)->FiredByFriend() ) {
		gameLocal.Warning( "Ignored damage for projectile fired by removed friend" );
		return;
	}
	//ff1.3 end

	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}

	if ( damageDef->dict.GetBool( "ignore_player" ) ) {
		return;
	}

	//ivan start
	if ( allowDmgfxs && !gibbed ) {
		CheckDamageFx( &damageDef->dict, attacker );
	}
	//ivan end

	CalcDamagePoints( inflictor, attacker, &damageDef->dict, damageScale, location, &damage, &armorSave );

	// determine knockback
	damageDef->dict.GetInt( "knockback", "20", knockback );

/*#ifdef _D3XP
	idPlayer *player = attacker->IsType( idPlayer::Type ) ? static_cast<idPlayer*>(attacker) : NULL;

	if ( gameLocal.mpGame.IsGametypeTeamBased()
		&& !gameLocal.serverInfo.GetBool( "si_teamDamage" )
		&& !damageDef->dict.GetBool( "noTeam" )
		&& player
		&& player != this		// you get self damage no matter what
		&& player->team == team ) {
			knockback = 0;
		}
#endif*/

	if ( knockback != 0 && !fl.noknockback ) {
		if ( attacker == this ) {
			damageDef->dict.GetFloat( "attackerPushScale", "0", attackerPushScale );
		} else {
			attackerPushScale = 1.0f;
		}

		kick = dir;
		kick.Normalize();
		kick *= g_knockback.GetFloat() * knockback * attackerPushScale / 200.0f;
		physicsObj.SetLinearVelocity( physicsObj.GetLinearVelocity() + kick );

		// set the timer so that the player can't cancel out the movement immediately
		physicsObj.SetKnockBack( idMath::ClampInt( 50, 200, knockback * 2 ) );
	}


	// give feedback on the player view and audibly when armor is helping
	if ( armorSave ) {
		inventory.armor -= armorSave;

		if ( gameLocal.time > lastArmorPulse + 200 ) {
			StartSound( "snd_hitArmor", SND_CHANNEL_ITEM, 0, false, NULL );
		}
		lastArmorPulse = gameLocal.time;
	}

	if ( damageDef->dict.GetBool( "burn" ) ) {
		StartSound( "snd_burn", SND_CHANNEL_BODY3, 0, false, NULL );
	} else if ( damageDef->dict.GetBool( "no_air" ) ) {
		if ( !armorSave && health > 0 ) {
			StartSound( "snd_airGasp", SND_CHANNEL_ITEM, 0, false, NULL );
		}
	}

	if ( g_debugDamage.GetInteger() ) {
		gameLocal.Printf( "client:%i health:%i damage:%i armor:%i\n",
			entityNumber, health, damage, armorSave );
	}

	// move the world direction vector to local coordinates
	damage_from = dir;
	damage_from.Normalize();

	viewAxis.ProjectVector( damage_from, localDamageVector );

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( health > 0 ) {
		playerView.DamageImpulse( localDamageVector, &damageDef->dict );
	}

	// do the damage
	if ( damage > 0 ) {

		if ( !gameLocal.isMultiplayer ) {
#ifdef _D3XP
			float scale = new_g_damageScale;
#else
			float scale = g_damageScale.GetFloat();
#endif
			if ( g_useDynamicProtection.GetBool() && g_skill.GetInteger() < 2 ) {
				//ff1.3 start
				//was: if ( gameLocal.time > lastDmgTime + 500 && scale > 0.25f ) {
				if ( gameLocal.time < lastDmgTime + 500 && scale > 0.25f ) {
				//ff1.3 end
					scale -= 0.05f;

					//gameLocal.Printf("scale down to -> %f\n", scale);
#ifdef _D3XP
					new_g_damageScale = scale;
#else
					g_damageScale.SetFloat( scale );
#endif
				}
			}

			if ( scale > 0.0f ) {
				damage *= scale;
			}

			//gameLocal.Printf("damage after new_g_damageScale -> %d, scale %f\n", damage, scale);
		}

		if ( damage < 1 ) {
			damage = 1;
		}

		//ff1.3 start - upd stats
        playerStats.damage += damage;
        //ff1.3 end

		health -= damage;

		if ( health <= 0 ) {

			if ( health < -999 ) {
				health = -999;
			}

			isTelefragged = damageDef->dict.GetBool( "telefrag" );

			lastDmgTime = gameLocal.time;
			Killed( inflictor, attacker, damage, dir, location );

		} else {
			// force a blink
			blink_time = 0;

			// let the anim script know we took damage
			AI_PAIN = Pain( inflictor, attacker, damage, dir, location );
			if ( !g_testDeath.GetBool() ) {
				lastDmgTime = gameLocal.time;
			}
		}
	} else {
		// don't accumulate impulses
		if ( af.IsLoaded() ) {
			// clear impacts
			af.Rest();

			// physics is turned off by calling af.Rest()
			BecomeActive( TH_PHYSICS );
		}
	}

	lastDamageDef = damageDef->Index();
	lastDamageDir = damage_from;
	lastDamageLocation = location;
}

/*
===========
idPlayer::Teleport
============
*/
void idPlayer::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	idVec3 org;

	//ff1.3 start
	teleportEntity = destination;

	if ( currentRiddenAI ) {
		currentRiddenAI->Teleport(origin, angles, destination);
		return;
	}
	else if ( currentVehicle ) {
		currentVehicle->Teleport(origin, angles, destination);
		return;
	}
	//ff1.3 end

	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->LowerWeapon();
	}

	SetOrigin( origin + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	if ( !gameLocal.isMultiplayer && GetFloorPos( 16.0f, org ) ) {
		SetOrigin( org );
	}

	// clear the ik heights so model doesn't appear in the wrong place
	walkIK.EnableAll();

	GetPhysics()->SetLinearVelocity( vec3_origin );

	SetViewAngles( angles );

	legsYaw = 0.0f;
	idealLegsYaw = 0.0f;
	oldViewYaw = viewAngles.yaw;

	if ( gameLocal.isMultiplayer ) {
		playerView.Flash( colorWhite, 140 );
	}

	UpdateVisuals();

	//teleportEntity = destination; //ff1.3 - moved up

	if ( !gameLocal.isClient && !noclip ) {
		if ( gameLocal.isMultiplayer ) {
			// kill anything at the new position or mark for kill depending on immediate or delayed teleport
			gameLocal.KillBox( this, destination != NULL );
		} else {
			// kill anything at the new position
			gameLocal.KillBox( this, true );
		}
	}

#ifdef _D3XP
	if ( PowerUpActive( HELLTIME ) ) {
		ClearPowerup( HELLTIME );
	}
#endif
}

/*
====================
idPlayer::SetPrivateCameraView
====================
*/
void idPlayer::SetPrivateCameraView( idCamera *camView, bool changeWeaponCinematicState ) {
	privateCameraView = camView;
	if ( camView ) {
		if( currentVehicle ){
			currentVehicle->PostEventMS( &EV_Hide, 0 );
		} else if( currentRiddenAI ){
			currentRiddenAI->PostEventMS( &EV_Hide, 0 );
		} else {
			StopFiring();
			Hide();
		}

		if ( changeWeaponCinematicState && weaponEnabled && weapon.GetEntity() && !gameLocal.inCinematic && !influenceActive ) { //ff1.3 - changeWeaponCinematicState, ff1.4: !gameLocal.inCinematic && !influenceActive
			weapon.GetEntity()->EnterCinematic();
		}
	} else {
		if ( !spectating ) {
			if( currentVehicle ){
				currentVehicle->PostEventMS( &EV_Show, 0 );
			} else if( currentRiddenAI ){
				currentRiddenAI->PostEventMS( &EV_Show, 0 );
			} else {
				Show();
			}

			if ( changeWeaponCinematicState && weaponEnabled && weapon.GetEntity() && !gameLocal.inCinematic && !influenceActive ) { //ff1.3 - changeWeaponCinematicState, ff1.4: !gameLocal.inCinematic && !influenceActive
				weapon.GetEntity()->ExitCinematic();
			}
		}
	}
}

/*
====================
idPlayer::DefaultFov

Returns the base FOV
====================
*/
float idPlayer::DefaultFov( void ) const {
	float fov;

	fov = g_fov.GetFloat();
	if ( gameLocal.isMultiplayer ) {
		if ( fov < 90.0f ) {
			return 90.0f;
		} else if ( fov > 110.0f ) {
			return 110.0f;
		}
	}

	return fov;
}

/*
====================
idPlayer::CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
float idPlayer::CalcFov( bool honorZoom ) {
	float fov;

	if ( fxFov ) {
		return DefaultFov() + 10.0f + cos( ( gameLocal.time + 2000 ) * 0.01 ) * 10.0f;
	}

	if ( influenceFov ) {
		return influenceFov;
	}

	//ff1.3 start
	if ( advancedWeaponZoomTime ) {
		if ( zoomFov.IsDone( advancedWeaponZoomTime ) ) {
			fov = ( honorZoom && weapon.GetEntity() ) ? weapon.GetEntity()->GetZoomFov() : DefaultFov();
		} else {
			fov = zoomFov.GetCurrentValue( advancedWeaponZoomTime );
		}
	} else { //old
		if ( zoomFov.IsDone( gameLocal.time ) ) {
#ifdef _DENTONMOD
			if ( honorZoom ) {
				if ( weaponZoom.quickMode && weaponZoom.startZoom && weapon.GetEntity() ) {
					fov = weapon.GetEntity()->GetZoomFov();
				} else if ( usercmd.buttons & BUTTON_ZOOM ) {
					fov = DefaultZoomFov();
				} else {
					fov = DefaultFov();
				}
			} else {
				fov = DefaultFov();
			}
			//fov = ( honorZoom && ((usercmd.buttons & BUTTON_ZOOM) || weaponZoom.startZoom )) && weapon.GetEntity() ? weapon.GetEntity()->GetZoomFov() : DefaultFov(); // Updated By Clone JCD
			//fov = ( honorZoom && (usercmd.buttons & BUTTON_ZOOM) ) ? DefaultZoomFov() : DefaultFov(); //ff1.3
#else
			fov = ( honorZoom && (usercmd.buttons & BUTTON_ZOOM)) && weapon.GetEntity() ? weapon.GetEntity()->GetZoomFov() : DefaultFov(); // Updated By Clone JCD
#endif // _DENTONMOD

		} else {
			fov = zoomFov.GetCurrentValue( gameLocal.time );
		}
	}
	//ff1.3 end


	// bound normal viewsize
	if ( fov < 1 ) {
		fov = 1;
	} else if ( fov > 179 ) {
		fov = 179;
	}

	return fov;
}

/*
==============
idPlayer::GunTurningOffset

generate a rotational offset for the gun based on the view angle
history in loggedViewAngles
==============
*/
idAngles idPlayer::GunTurningOffset( void ) {
	idAngles	a;

	a.Zero();

	if ( gameLocal.framenum < NUM_LOGGED_VIEW_ANGLES ) {
		return a;
	}

	idAngles current = loggedViewAngles[ gameLocal.framenum & (NUM_LOGGED_VIEW_ANGLES-1) ];

	idAngles	av, base;
	int weaponAngleOffsetAverages;
	float weaponAngleOffsetScale, weaponAngleOffsetMax;

	weapon.GetEntity()->GetWeaponAngleOffsets( &weaponAngleOffsetAverages, &weaponAngleOffsetScale, &weaponAngleOffsetMax );

	av = current;

	// calcualte this so the wrap arounds work properly
	for ( int j = 1 ; j < weaponAngleOffsetAverages ; j++ ) {
		idAngles a2 = loggedViewAngles[ ( gameLocal.framenum - j ) & (NUM_LOGGED_VIEW_ANGLES-1) ];

		idAngles delta = a2 - current;

		if ( delta[1] > 180 ) {
			delta[1] -= 360;
		} else if ( delta[1] < -180 ) {
			delta[1] += 360;
		}

		av += delta * ( 1.0f / weaponAngleOffsetAverages );
	}

	a = ( av - current ) * weaponAngleOffsetScale;

	for ( int i = 0 ; i < 3 ; i++ ) {
		if ( a[i] < -weaponAngleOffsetMax ) {
			a[i] = -weaponAngleOffsetMax;
		} else if ( a[i] > weaponAngleOffsetMax ) {
			a[i] = weaponAngleOffsetMax;
		}
	}

	return a;
}

/*
==============
idPlayer::GunAcceleratingOffset

generate a positional offset for the gun based on the movement
history in loggedAccelerations
==============
*/
idVec3	idPlayer::GunAcceleratingOffset( void ) {
	idVec3	ofs;

	float weaponOffsetTime, weaponOffsetScale;

	ofs.Zero();

	weapon.GetEntity()->GetWeaponTimeOffsets( &weaponOffsetTime, &weaponOffsetScale );

	int stop = currentLoggedAccel - NUM_LOGGED_ACCELS;
	if ( stop < 0 ) {
		stop = 0;
	}
	for ( int i = currentLoggedAccel-1 ; i > stop ; i-- ) {
		loggedAccel_t	*acc = &loggedAccel[i&(NUM_LOGGED_ACCELS-1)];

		float	f;
		float	t = gameLocal.time - acc->time;
		if ( t >= weaponOffsetTime ) {
			break;	// remainder are too old to care about
		}

		f = t / weaponOffsetTime;
		f = ( cos( f * 2.0f * idMath::PI ) - 1.0f ) * 0.5f;
		ofs += f * weaponOffsetScale * acc->dir;
	}

	return ofs;
}

/*
==============
idPlayer::CalculateViewWeaponPos

Calculate the bobbing position of the view weapon
==============
*/
void idPlayer::CalculateViewWeaponPos( idVec3 &origin, idMat3 &axis ) {
	float		scale;
	float		fracsin;
	idAngles	angles;
	int			delta;

	// CalculateRenderView must have been called first
	const idVec3 &viewOrigin = firstPersonViewOrigin;
	const idMat3 &viewAxis = firstPersonViewAxis;

	// these cvars are just for hand tweaking before moving a value to the weapon def
	idVec3	gunpos( g_gun_x.GetFloat(), g_gun_y.GetFloat(), g_gun_z.GetFloat() );

	// as the player changes direction, the gun will take a small lag
	idVec3	gunOfs = GunAcceleratingOffset();
	origin = viewOrigin + ( gunpos + gunOfs ) * viewAxis;

	// on odd legs, invert some angles
	if ( bobCycle & 128 ) {
		scale = -xyspeed;
	} else {
		scale = xyspeed;
	}

	// gun angles from bobbing
	angles.roll		= scale * bobfracsin * 0.005f;
	angles.yaw		= scale * bobfracsin * 0.01f;
	angles.pitch	= xyspeed * bobfracsin * 0.005f;

	// gun angles from turning
	if ( gameLocal.isMultiplayer ) {
		idAngles offset = GunTurningOffset();
		offset *= g_mpWeaponAngleScale.GetFloat();
		angles += offset;
	} else {
		angles += GunTurningOffset();
	}

	idVec3 gravity = physicsObj.GetGravityNormal();

	// drop the weapon when landing after a jump / fall
	delta = gameLocal.time - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin -= gravity * ( landChange*0.25f * delta / LAND_DEFLECT_TIME );
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin -= gravity * ( landChange*0.25f * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME );
	}

	// speed sensitive idle drift
	scale = xyspeed + 40.0f;
	fracsin = scale * sin( MS2SEC( gameLocal.time ) ) * 0.01f;
	angles.roll		+= fracsin;
	angles.yaw		+= fracsin;
	angles.pitch	+= fracsin;

	axis = angles.ToMat3() * viewAxis;
}

/*
===============
idPlayer::OffsetThirdPersonView
===============
*/
void idPlayer::OffsetThirdPersonView( float angle, float range, float height, bool clip ) {
	idVec3			view;
	idVec3			focusAngles;
	trace_t			trace;
	idVec3			focusPoint;
	float			focusDist;
	float			forwardScale, sideScale;
	idVec3			origin;
	idAngles		angles;
	idMat3			axis; //not used
	idBounds		bounds;
	idEntity *		passEntity; //ff1.3

	angles = viewAngles;

	//ivan start
	//was:	GetViewPos( origin, axis );
	if ( currentRiddenAI ) {
		currentRiddenAI->GetCameraPos( origin, axis );
		passEntity = currentRiddenAI;
	} else if ( currentVehicle ) {
		currentVehicle->GetCameraPos( origin, axis );
		passEntity = currentVehicle;
	} else {
		GetViewPos( origin, axis );
		passEntity = this;
	}
	//ivan end

	if ( angle ) {
		angles.pitch = 0.0f;
	}

	if ( angles.pitch > 45.0f ) {
		angles.pitch = 45.0f;		// don't go too far overhead
	}

	focusPoint = origin + angles.ToForward() * THIRD_PERSON_FOCUS_DISTANCE;
	focusPoint.z += height;
	view = origin;
	view.z += 8 + height;

	//ff1.3 start
	//gameRenderWorld->DebugArrow( colorRed, origin, view, 12 );
	//ff1.3 end

	angles.pitch *= 0.5f;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();

	idMath::SinCos( DEG2RAD( angle ), sideScale, forwardScale );
	view -= range * forwardScale * renderView->viewaxis[ 0 ];
	view += range * sideScale * renderView->viewaxis[ 1 ];

	if ( clip ) {
		// trace a ray from the origin to the viewpoint to make sure the view isn't
		// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
		bounds = idBounds( idVec3( -4, -4, -4 ), idVec3( 4, 4, 4 ) );
		gameLocal.clip.TraceBounds( trace, origin, view, bounds, MASK_SOLID, passEntity /*this*/ );
		if ( trace.fraction != 1.0f ) {
			view = trace.endpos;
			view.z += ( 1.0f - trace.fraction ) * 32.0f;

			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out
			gameLocal.clip.TraceBounds( trace, origin, view, bounds, MASK_SOLID, passEntity /*this*/ );
			view = trace.endpos;
		}
	}

	// select pitch to look at focus point from vieword
	focusPoint -= view;
	focusDist = idMath::Sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1.0f ) {
		focusDist = 1.0f;	// should never happen
	}

	angles.pitch = - RAD2DEG( atan2( focusPoint.z, focusDist ) );
	angles.yaw -= angle;

	renderView->vieworg = view;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();
	renderView->viewID = 0;
}

/*
===============
idPlayer::GetEyePosition
===============
*/
idVec3 idPlayer::GetEyePosition( void ) const {
	idVec3 org;

	// use the smoothed origin if spectating another player in multiplayer
	if ( gameLocal.isClient && entityNumber != gameLocal.localClientNum ) {
		org = smoothedOrigin;
	} else {
		org = GetPhysics()->GetOrigin();
	}
	return org + ( GetPhysics()->GetGravityNormal() * -eyeOffset.z );
}

/*
===============
idPlayer::GetViewPos
===============
*/
void idPlayer::GetViewPos( idVec3 &origin, idMat3 &axis ) const {
	idAngles angles;

	// if dead, fix the angle and don't add any kick
	if ( health <= 0 ) {
		angles.yaw = viewAngles.yaw;
		angles.roll = 40;
		angles.pitch = -15;
		axis = angles.ToMat3();
		origin = GetEyePosition();
	} else {
		origin = GetEyePosition() + viewBob;
		angles = viewAngles + viewBobAngles + playerView.AngleOffset();

		axis = angles.ToMat3() * physicsObj.GetGravityAxis();

		// adjust the origin based on the camera nodal distance (eye distance from neck)
		origin += physicsObj.GetGravityNormal() * g_viewNodalZ.GetFloat();
		origin += axis[0] * g_viewNodalX.GetFloat() + axis[2] * g_viewNodalZ.GetFloat();
	}
}

/*
===============
idPlayer::CalculateFirstPersonView
===============
*/
void idPlayer::CalculateFirstPersonView( void ) {
	if ( ( pm_modelView.GetInteger() == 1 ) || ( ( pm_modelView.GetInteger() == 2 ) && ( health <= 0 ) ) ) {
		//	Displays the view from the point of view of the "camera" joint in the player model

		idMat3 axis;
		idVec3 origin;
		idAngles ang;

		ang = viewBobAngles + playerView.AngleOffset();
		ang.yaw += viewAxis[ 0 ].ToYaw();

		jointHandle_t joint = animator.GetJointHandle( "camera" );
		animator.GetJointTransform( joint, gameLocal.time, origin, axis );
		firstPersonViewOrigin = ( origin + modelOffset ) * ( viewAxis * physicsObj.GetGravityAxis() ) + physicsObj.GetOrigin() + viewBob;
		firstPersonViewAxis = axis * ang.ToMat3() * physicsObj.GetGravityAxis();
	} else {
		// offset for local bobbing and kicks
		GetViewPos( firstPersonViewOrigin, firstPersonViewAxis );
#if 0
		// shakefrom sound stuff only happens in first person
		firstPersonViewAxis = firstPersonViewAxis * playerView.ShakeAxis();
#endif
	}
}

/*
==================
idPlayer::GetRenderView

Returns the renderView that was calculated for this tic
==================
*/
renderView_t *idPlayer::GetRenderView( void ) {
	return renderView;
}

/*
==================
idPlayer::CalculateRenderView

create the renderView for the current tic
==================
*/
void idPlayer::CalculateRenderView( void ) {
	int i;
	float range;

	if ( !renderView ) {
		renderView = new renderView_t;
	}
	memset( renderView, 0, sizeof( *renderView ) );

	// copy global shader parms
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView->shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}
	renderView->globalMaterial = gameLocal.GetGlobalMaterial();

#ifdef _D3XP
	renderView->time = gameLocal.slow.time;
#endif

	// calculate size of 3D view
	renderView->x = 0;
	renderView->y = 0;
	renderView->width = SCREEN_WIDTH;
	renderView->height = SCREEN_HEIGHT;
	renderView->viewID = 0;

	// check if we should be drawing from a camera's POV
	if ( !noclip && (gameLocal.GetCamera() || privateCameraView) ) {
		// get origin, axis, and fov
		if ( privateCameraView ) {
			privateCameraView->GetViewParms( renderView );
		} else {
			gameLocal.GetCamera()->GetViewParms( renderView );
		}
	} else {
		if ( g_stopTime.GetBool() ) {
			renderView->vieworg = firstPersonViewOrigin;
			renderView->viewaxis = firstPersonViewAxis;

			//was: if ( !pm_thirdPerson.GetBool() ) { //ivan
			if ( !UseThirdPersonCamera() ) { //ivan
				// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
				// allow the right player view weapons
				renderView->viewID = entityNumber + 1;
			}
		//ff1.3 start
/*
was:
		} else if ( pm_thirdPerson.GetBool() ) {
			OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), pm_thirdPersonRange.GetFloat(), pm_thirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool() );
		}
*/
		} else if ( UseThirdPersonCamera() ) { //ivan
			/*
			if ( ffdebug.GetBool() ) {
				OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), pm_thirdPersonRange.GetFloat(), pm_thirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool() );
			}else
			*/
			if ( currentRiddenAI ) {
				OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), currentRiddenAI->GetThirdPersonRange(), currentRiddenAI->GetThirdPersonHeight(), pm_thirdPersonClip.GetBool() );
			} else if ( currentVehicle ) {
				OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), currentVehicle->GetThirdPersonRange(), currentVehicle->GetThirdPersonHeight(), pm_thirdPersonClip.GetBool() );
			} else {
				OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), pm_thirdPersonRange.GetFloat(), pm_thirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool() );
			}
		}
		//ff1.3 end
		else if ( pm_thirdPersonDeath.GetBool() ) {
			range = gameLocal.time < minRespawnTime ? ( gameLocal.time + RAGDOLL_DEATH_TIME - minRespawnTime ) * ( 120.0f / RAGDOLL_DEATH_TIME ) : 120.0f;
			OffsetThirdPersonView( 0.0f, 20.0f + range, 0.0f, false );
		} else {
			renderView->vieworg = firstPersonViewOrigin;
			renderView->viewaxis = firstPersonViewAxis;

			// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
			// allow the right player view weapons
			renderView->viewID = entityNumber + 1;
		}

		// field of view
		gameLocal.CalcFov( CalcFov( true ), renderView->fov_x, renderView->fov_y );
	}

	if ( renderView->fov_y == 0 ) {
		common->Error( "renderView->fov_y == 0" );
	}

	if ( g_showviewpos.GetBool() ) {
		gameLocal.Printf( "%s : %s\n", renderView->vieworg.ToString(), renderView->viewaxis.ToAngles().ToString() );
	}
}

/*
=============
idPlayer::AddAIKill
=============
*/
void idPlayer::AddAIKill( idActor *victim, idEntity *inflictor, const idDict *damageDef ) {
	//ff1.3 start
	/* was:
#ifndef _D3XP

	int max_souls;
	int ammo_souls;

	if ( ( weapon_soulcube < 0 ) || ( inventory.weapons & ( 1 << weapon_soulcube ) ) == 0 ) {
		return;
	}

	assert( hud );


	ammo_souls = idWeapon::GetAmmoNumForName( "ammo_souls" );
	max_souls = inventory.MaxAmmoForAmmoClass( this, "ammo_souls" );
	if ( inventory.ammo[ ammo_souls ] < max_souls ) {
		inventory.ammo[ ammo_souls ]++;
		if ( inventory.ammo[ ammo_souls ] >= max_souls ) {
			hud->HandleNamedEvent( "soulCubeReady" );
			StartSound( "snd_soulcube_ready", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
#endif
	*/

	if ( gameLocal.inCinematic ) {
		return;
	}

	playerStats.killCount++;

	//give hp for bullet-time kills
	if ( PowerUpActive( HELLTIME ) ) {
		AddExtraHealth();
	}

	//log kill times for massacre check
	for( int i = MASSACRE_KILLS - 1; i > 0; i-- ) {
		lastKillTimes[ i ] = lastKillTimes[ i-1 ];
	}
	lastKillTimes[ 0 ] = gameLocal.fast.time;

	//medals for special weapons
	if ( inflictor->spawnArgs.GetBool( "grabbed" ) ) {
		//gameLocal.Printf("GrabberMedal\n");
		GiveMedal(MEDAL_SKILLSHOT);
	}
	else if ( damageDef->GetBool( "chainsaw" ) ) {
		//gameLocal.Printf("ChainsawMedal\n");
		GiveMedal(MEDAL_CHAINKILL);
	}
	else if ( damageDef->GetBool( "railgun" ) ) {
		//multiple rail kills happens in the same frame
		if ( lastKillTimes[ 1 ] == gameLocal.fast.time ) {
			if ( lastKillTimes[ 2 ] != gameLocal.fast.time ) { //trigger the medal only at second kill
				//gameLocal.Printf("RailMedal\n");
				GiveMedal(MEDAL_SKILLSHOT);
			}
			return; //don't update tempKillCount so that we don't trigger double or multi kill
		}
	}

	//temporary kill count for double/multi kill
	tempKillCount++;
	//ff1.3 end
}

/*
=============
idPlayer::AddExtraHealth
=============
*/
void idPlayer::AddExtraHealth( void ) {
	if( health <= 0 ){
		return; //don't add health if dead
	}
	health += HP_EXTRA;
	healthAdded += HP_EXTRA;
	hudPulseFlags |= HUD_PULSE_HEALTH_ADDER;

	//wait a little before decreasing if we are above maxHealth
	nextHealthDrop = gameLocal.fast.time + pm_exceedingHealthRate.GetInteger();
}

/*
=============
idPlayer::AddExtraHealth
=============
*/
void idPlayer::IncreaseMaxHealth( void ) {
	inventory.maxHealth++;
	//hudPulseFlags |= HUD_PULSE_MAXHEALTH_ADDER;
}


//ff1.3 start
/*
=============
idPlayer::SetSoulCubeProjectile
=============
void idPlayer::SetSoulCubeProjectile( idProjectile *projectile ) {
	soulCubeProjectile = projectile;
}
*/

/*
================
idPlayer::GetPainKillerBeamData
================
*/
void idPlayer::GetPainKillerBeamData( idVec3 &returnPos, bool &beamEnabled ) {
	if ( currentRiddenAI ) {
		returnPos = currentRiddenAI->GetPhysics()->GetAbsBounds().GetCenter();
		beamEnabled = false;
	} else if ( currentVehicle ) {
		returnPos = currentVehicle->GetPhysics()->GetAbsBounds().GetCenter();
		beamEnabled = false;
	} else if ( currentWeapon == weapon_shockrifle && weapon.GetEntity() ) {
		weapon.GetEntity()->GetMuzzlePos(returnPos);
		beamEnabled = !IsHidden();
	} else {
		returnPos = physicsObj.GetAbsBounds().GetCenter();
		beamEnabled = false;
	}
}

/*
=============
idPlayer::PainKillerReturnedCallback
=============
*/
void idPlayer::PainKillerReturnedCallback( bool hit ) {
	painKillerProjectile = NULL;
	//gameLocal.Printf("PainKillerReturnedCallback\n");
	if ( !hit ) {
		//gameLocal.Printf("PainKillerReturnedCallback !hit \n");
		AddPwAmmoWeapon( weapon_shockrifle, 1 );
		//hud->HandleNamedEvent( "painKillerReady" );
		playerStats.projFired--;
	}

	if ( weaponEnabled && currentWeapon == weapon_shockrifle && weapon.GetEntity() ) {
		StartSound( "snd_painkiller_returned", SND_CHANNEL_ANY, 0, false, NULL );
		weapon.GetEntity()->MuzzleKick();
	}
}

//ff1.3 end

/*
=============
idPlayer::AddProjectilesFired
=============
*/
void idPlayer::AddProjectilesFired( int count ) {
	playerStats.projFired += count;
}

/*
=============
idPlayer::AddProjectileHits
=============
*/
void idPlayer::AddProjectileHits( int count, int hitCountGroupId ) {
	//ff1.3 start
	int i;

	if ( hitCountGroupId > 0 ) {
		for( i = 0; i < NUM_HIT_COUNT_GROUP_IDS; i++ ) {
			if ( hitCountGroupIds[ i ] == hitCountGroupId ) {
				//gameLocal.Printf("Not added: %d found in %d\n", hitCountGroupId, i);
				return;
			}
		}
		//gameLocal.Printf("Insert %d in %d\n", hitCountGroupId, nextHitCountGroupIndex);
		hitCountGroupIds[ nextHitCountGroupIndex++ ] = hitCountGroupId;
		if ( nextHitCountGroupIndex >= NUM_HIT_COUNT_GROUP_IDS ) {
			nextHitCountGroupIndex = 0;
		}
	}
	//ff1.3  end

	playerStats.projHits += count;
}

/*
=============
idPlayer::SetLastHitTime
=============
*/
void idPlayer::SetLastHitTime( int time ) {
	idPlayer *aimed = NULL;

	if ( time && lastHitTime != time ) {
		lastHitToggle ^= 1;
	}
	lastHitTime = time;
	if ( !time ) {
		// level start and inits
		return;
	}
	if ( gameLocal.isMultiplayer && ( time - lastSndHitTime ) > 10 ) {
		lastSndHitTime = time;
		StartSound( "snd_hit_feedback", SND_CHANNEL_ANY, SSF_PRIVATE_SOUND, false, NULL );
	}
	if ( cursor ) {
		cursor->HandleNamedEvent( "hitTime" );
	}
	if ( hud ) {
		if ( MPAim != -1 ) {
			if ( gameLocal.entities[ MPAim ] && gameLocal.entities[ MPAim ]->IsType( idPlayer::Type ) ) {
				aimed = static_cast< idPlayer * >( gameLocal.entities[ MPAim ] );
			}
			assert( aimed );
			// full highlight, no fade till loosing aim
			hud->SetStateString( "aim_text", gameLocal.userInfo[ MPAim ].GetString( "ui_name" ) );
			if ( aimed ) {
				hud->SetStateFloat( "aim_color", aimed->colorBarIndex );
			}
			hud->HandleNamedEvent( "aim_flash" );
			MPAimHighlight = true;
			MPAimFadeTime = 0;
		} else if ( lastMPAim != -1 ) {
			if ( gameLocal.entities[ lastMPAim ] && gameLocal.entities[ lastMPAim ]->IsType( idPlayer::Type ) ) {
				aimed = static_cast< idPlayer * >( gameLocal.entities[ lastMPAim ] );
			}
			assert( aimed );
			// start fading right away
			hud->SetStateString( "aim_text", gameLocal.userInfo[ lastMPAim ].GetString( "ui_name" ) );
			if ( aimed ) {
				hud->SetStateFloat( "aim_color", aimed->colorBarIndex );
			}
			hud->HandleNamedEvent( "aim_flash" );
			hud->HandleNamedEvent( "aim_fade" );
			MPAimHighlight = false;
			MPAimFadeTime = gameLocal.realClientTime;
		}
	}
}

/*
=============
idPlayer::SetInfluenceLevel
=============
*/
void idPlayer::SetInfluenceLevel( int level ) {
	if ( level != influenceActive ) {
		if ( level ) {
			for ( idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
				if ( ent->IsType( idProjectile::Type ) ) {
					// remove all projectiles
					ent->PostEventMS( &EV_Remove, 0 );
				}
			}
			if ( weaponEnabled && weapon.GetEntity() && !gameLocal.inCinematic && !privateCameraView ) { //ff1.4 - skip if already in cinematic or private camera
				weapon.GetEntity()->EnterCinematic();
			}
		} else {
			physicsObj.SetLinearVelocity( vec3_origin );
			if ( weaponEnabled && weapon.GetEntity() && !gameLocal.inCinematic && !privateCameraView ) { //ff1.4 - skip if already in cinematic or private camera
				weapon.GetEntity()->ExitCinematic();
			}
		}
		influenceActive = level;
	}
}

/*
=============
idPlayer::SetInfluenceView
=============
*/
void idPlayer::SetInfluenceView( const char *mtr, const char *skinname, float radius, idEntity *ent ) {
	influenceMaterial = NULL;
	influenceEntity = NULL;
	influenceSkin = NULL;
	if ( mtr && *mtr ) {
		influenceMaterial = declManager->FindMaterial( mtr );
	}
	if ( skinname && *skinname ) {
		influenceSkin = declManager->FindSkin( skinname );
		if ( head.GetEntity() ) {
			head.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC( gameLocal.time );
		}
		UpdateVisuals();
	}
	influenceRadius = radius;
	if ( radius > 0.0f ) {
		influenceEntity = ent;
	}
}

/*
=============
idPlayer::SetInfluenceFov
=============
*/
void idPlayer::SetInfluenceFov( float fov ) {
	influenceFov = fov;
}

/*
================
idPlayer::OnLadder
================
*/
bool idPlayer::OnLadder( void ) const {
	return physicsObj.OnLadder();
}

/*
==================
idPlayer::Event_GetButtons
==================
*/
void idPlayer::Event_GetButtons( void ) {
	idThread::ReturnInt( usercmd.buttons );
}

/*
==================
idPlayer::Event_GetMove
==================
*/
void idPlayer::Event_GetMove( void ) {
	idVec3 move( usercmd.forwardmove, usercmd.rightmove, usercmd.upmove );
	idThread::ReturnVector( move );
}

/*
================
idPlayer::Event_GetViewAngles
================
*/
void idPlayer::Event_GetViewAngles( void ) {
	idThread::ReturnVector( idVec3( viewAngles[0], viewAngles[1], viewAngles[2] ) );
}

//ff1.3 start
/*
=================
idPlayer::GetAimPos
=================

idVec3 idPlayer::GetAimPos( idEntity* ignoreEnt ) {
	idVec3 end = renderView->vieworg + renderView->viewaxis[ 0 ] * 3000.0f;
	trace_t trace;
	gameLocal.clip.TracePoint( trace, renderView->vieworg, end, MASK_SOLID|CONTENTS_RENDERMODEL, ignoreEnt ? ignoreEnt : this );
	if ( trace.fraction < 1.0f ) {
		return trace.endpos;
	}
	return end;
}
*/

/*
=================
idPlayer::GetAimAngles
=================

idAngles idPlayer::GetAimAngles( const idVec3 &firePos, idEntity* ignoreEnt ) {
	idVec3 end = GetAimPos( ignoreEnt );
	if ( g_debugVehicle.GetBool() ) {
		gameRenderWorld->DebugArrow( colorRed, firePos, end, 2, 10000 );
		gameRenderWorld->DebugBounds( colorBlue, idBounds( vec3_origin ).Expand( 5.0f ), firePos, 10000 );
	}
	return (end - firePos).ToAngles();
}
*/


/*
=================
idPlayer::Event_GetAimPos
=================

void idPlayer::Event_GetAimPos( idEntity* ignoreEnt ) {
	idVec3 end = GetAimPos( ignoreEnt );
	idThread::ReturnVector( idVec3( end[0], end[1], end[2] ) );
}
*/
/*
=================
idPlayer::Event_GetAimAngles
=================

void idPlayer::Event_GetAimAngles( idVec3 firePos, idEntity* ignoreEnt ) {
	idAngles angles = GetAimAngles( firePos, ignoreEnt );
	idThread::ReturnVector( idVec3( angles[0], angles[1], angles[2] ) );
}
*/
//ff1.3 end

/*
==================
idPlayer::Event_StopFxFov
==================
*/
void idPlayer::Event_StopFxFov( void ) {
	fxFov = false;
}

/*
==================
idPlayer::StartFxFov
==================
*/
void idPlayer::StartFxFov( float duration ) {
	fxFov = true;
	PostEventSec( &EV_Player_StopFxFov, duration );
}

/*
==================
idPlayer::Event_EnableWeapon
==================
*/
void idPlayer::Event_EnableWeapon( void ) {
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );

	//ff1.3 start
	if ( IsRiding() ) {
		weaponEnabledOnRideExit = true;
	} else {
		weaponEnabled = true;
		if ( weapon.GetEntity() ) {
			weapon.GetEntity()->ExitCinematic();
		}
	}
	/* was:
	weaponEnabled = true;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->ExitCinematic();
	}
	*/
	//ff1.3 end
}

/*
==================
idPlayer::Event_DisableWeapon
==================
*/
void idPlayer::Event_DisableWeapon( void ) {
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );

	//ff1.3 start
	if ( IsRiding() ) {
		weaponEnabledOnRideExit = false;
	} else {
		weaponEnabled = false;
		if ( weapon.GetEntity() ) {
			weapon.GetEntity()->EnterCinematic();
		}
	}
	/* was:
	weaponEnabled = false;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->EnterCinematic();
	}
	*/
	//ff1.3 end
}

#ifdef _D3XP
/*
==================
idPlayer::Event_GiveInventoryItem
==================
*/
void idPlayer::Event_GiveInventoryItem( const char* name ) {
	GiveInventoryItem(name);
}

/*
==================
idPlayer::Event_RemoveInventoryItem
==================
*/
void idPlayer::Event_RemoveInventoryItem( const char* name ) {
	RemoveInventoryItem(name);
}


/*
==================
idPlayer::Event_GetIdealWeapon
==================
*/
void idPlayer::Event_GetIdealWeapon( void ) {
	const char *weapon;

	if ( idealWeapon >= 0 ) {
		weapon = spawnArgs.GetString( va( "def_weapon%d", idealWeapon ) );
		idThread::ReturnString( weapon );
	} else {
		idThread::ReturnString( "" );
	}
}

/*
==================
idPlayer::Event_SetPowerupTime
==================
*/
void idPlayer::Event_SetPowerupTime( int powerup, int time ) {
	if ( time > 0 ) {
		GivePowerUp( powerup, time );
	} else {
		ClearPowerup( powerup );
	}
}

/*
==================
idPlayer::Event_IsPowerupActive
==================
*/
void idPlayer::Event_IsPowerupActive( int powerup ) {
	idThread::ReturnInt(this->PowerUpActive(powerup) ? 1 : 0);
}

/*
==================
idPlayer::Event_StartWarp
==================
*/
void idPlayer::Event_StartWarp() {
	playerView.AddWarp( idVec3( 0, 0, 0 ), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100, 1000 );
}

/*
==================
idPlayer::Event_StopHelltime
==================
*/
void idPlayer::Event_StopHelltime( int mode ) {
	if ( PowerUpActive( HELLTIME ) ) {
		ClearPowerup( HELLTIME );
	}
	/*
	if ( mode == 1 ) {
		StopHelltime( true );
	}
	else {
		StopHelltime( false );
	}
	*/
}

/*
==================
idPlayer::Event_WeaponAvailable
==================
*/
void idPlayer::Event_WeaponAvailable( const char* name ) {
	idThread::ReturnInt( WeaponAvailable(name) ? 1 : 0 );
}

/*
==================
idPlayer::WeaponAvailable
==================
*/
bool idPlayer::WeaponAvailable( const char* name ) {
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		if ( inventory.weapons & ( 1 << i ) ) {
			const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if ( !idStr::Cmp( weap, name ) ) {
				return true;
			}
		}
	}
	return false;
}

#endif

/*
==================
idPlayer::Event_GetCurrentWeapon
==================
*/
void idPlayer::Event_GetCurrentWeapon( void ) {
	const char *weapon;

	if ( currentWeapon >= 0 ) {
		weapon = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );
		idThread::ReturnString( weapon );
	} else {
		idThread::ReturnString( "" );
	}
}

/*
==================
idPlayer::Event_GetPreviousWeapon
==================
*/
void idPlayer::Event_GetPreviousWeapon( void ) {
	const char *weapon;

	if ( previousWeapon >= 0 ) {
		int pw = ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) ? 0 : previousWeapon;
		weapon = spawnArgs.GetString( va( "def_weapon%d", pw) );
		idThread::ReturnString( weapon );
	} else {
		idThread::ReturnString( spawnArgs.GetString( "def_weapon0" ) );
	}
}

/*
==================
idPlayer::Event_SelectWeapon
==================
*/
void idPlayer::Event_SelectWeapon( const char *weaponName ) {
	int i;
	int weaponNum;

	if ( gameLocal.isClient ) {
		gameLocal.Warning( "Cannot switch weapons from script in multiplayer" );
		return;
	}

	if ( hiddenWeapon && gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) {
		idealWeapon = weapon_fists;
		weapon.GetEntity()->HideWeapon();
		return;
	}

	weaponNum = -1;
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		if ( inventory.weapons & ( 1 << i ) ) {
			const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if ( !idStr::Cmp( weap, weaponName ) ) {
				weaponNum = i;
				break;
			}
		}
	}

	if ( weaponNum < 0 ) {
		gameLocal.Warning( "%s is not carrying weapon '%s'", name.c_str(), weaponName );
		return;
	}

	hiddenWeapon = false;
	idealWeapon = weaponNum;

	UpdateHudWeapon();
}

/*
==================
idPlayer::Event_GetWeaponEntity
==================
*/
void idPlayer::Event_GetWeaponEntity( void ) {
	idThread::ReturnEntity( weapon.GetEntity() );
}

/*
==================
idPlayer::Event_OpenPDA
==================
*/
void idPlayer::Event_OpenPDA( void ) {
	if ( !gameLocal.isMultiplayer ) {
		TogglePDA();
	}
}

/*
==================
idPlayer::Event_InPDA
==================
*/
void idPlayer::Event_InPDA( void ) {
	idThread::ReturnInt( objectiveSystemOpen );
}

/*
==================
idPlayer::TeleportDeath
==================
*/
void idPlayer::TeleportDeath( int killer ) {
	teleportKiller = killer;
}

/*
==================
idPlayer::Event_ExitTeleporter
==================
*/
void idPlayer::Event_ExitTeleporter( void ) {
	idEntity	*exitEnt;
	float		pushVel;

	// verify and setup
	exitEnt = teleportEntity.GetEntity();
	if ( !exitEnt ) {
		common->DPrintf( "Event_ExitTeleporter player %d while not being teleported\n", entityNumber );
		return;
	}

	pushVel = exitEnt->spawnArgs.GetFloat( "push", "300" );

	if ( gameLocal.isServer ) {
		ServerSendEvent( EVENT_EXIT_TELEPORTER, NULL, false, -1 );
	}

	SetPrivateCameraView( NULL, true ); //ff1.3 - enable weapon particles

	const idVec3 &teleportPos = exitEnt->GetPhysics()->GetOrigin();
	const idAngles &teleportAngles = exitEnt->GetPhysics()->GetAxis().ToAngles();

	if( currentRiddenAI ){
		currentRiddenAI->Teleport(teleportPos, teleportAngles, exitEnt);
	} else if( currentVehicle ){
		currentVehicle->Teleport(teleportPos, teleportAngles, exitEnt);
	} else {
		// setup origin and push according to the exit target
		SetOrigin( teleportPos + idVec3( 0, 0, CM_CLIP_EPSILON ) );
		SetViewAngles( teleportAngles );
		physicsObj.SetLinearVelocity( exitEnt->GetPhysics()->GetAxis()[ 0 ] * pushVel );
		physicsObj.ClearPushedVelocity();

		// clear the ik heights so model doesn't appear in the wrong place
		walkIK.EnableAll();

		UpdateVisuals();

		if ( teleportKiller != -1 ) {
			// we got killed while being teleported
			Damage( gameLocal.entities[ teleportKiller ], gameLocal.entities[ teleportKiller ], vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT );
			teleportKiller = -1;
		} else {
			// kill anything that would have waited at teleport exit
			gameLocal.KillBox( this );
		}
	}

	// teleport fx
	playerView.Flash( colorWhite, 120 );

	StartSound( "snd_teleport_exit", SND_CHANNEL_ANY, 0, false, NULL );

	teleportEntity = NULL;
}

/*
================
idPlayer::ClientPredictionThink
================
*/
void idPlayer::ClientPredictionThink( void ) {
	renderEntity_t *headRenderEnt;

	oldFlags = usercmd.flags;
	oldButtons = usercmd.buttons;

	usercmd = gameLocal.usercmds[ entityNumber ];

	if ( entityNumber != gameLocal.localClientNum ) {
		// ignore attack button of other clients. that's no good for predictions
		usercmd.buttons &= ~BUTTON_ATTACK;
	}

	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;

#ifdef _D3XP
	if ( mountedObject ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}
#endif

	if ( objectiveSystemOpen ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}

	// clear the ik before we do anything else so the skeleton doesn't get updated twice
	walkIK.ClearJointMods();

	if ( gameLocal.isNewFrame ) {
		if ( ( usercmd.flags & UCF_IMPULSE_SEQUENCE ) != ( oldFlags & UCF_IMPULSE_SEQUENCE ) ) {
			PerformImpulse( usercmd.impulse );
		}
	}

	scoreBoardOpen = ( ( usercmd.buttons & BUTTON_SCORES ) != 0 || forceScoreBoard );

	AdjustSpeed();

	UpdateViewAngles();

	// update the smoothed view angles
	if ( gameLocal.framenum >= smoothedFrame && entityNumber != gameLocal.localClientNum ) {
		idAngles anglesDiff = viewAngles - smoothedAngles;
		anglesDiff.Normalize180();
		if ( idMath::Fabs( anglesDiff.yaw ) < 90.0f && idMath::Fabs( anglesDiff.pitch ) < 90.0f ) {
			// smoothen by pushing back to the previous angles
			viewAngles -= gameLocal.clientSmoothing * anglesDiff;
			viewAngles.Normalize180();
		}
		smoothedAngles = viewAngles;
	}
	smoothedOriginUpdated = false;

	if ( !af.IsActive() ) {
		AdjustBodyAngles();
	}

	if ( !isLagged ) {
		// don't allow client to move when lagged
		Move();
	}

	// update GUIs, Items, and character interactions
	UpdateFocus();

	// service animations
	if ( !spectating && !af.IsActive() ) {
		UpdateConditions();
		UpdateAnimState();
		CheckBlink();
	}

	// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
	AI_PAIN = false;

	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();

	// this may use firstPersonView, or a thirdPerson / camera view
	CalculateRenderView();

	if ( !gameLocal.inCinematic && weapon.GetEntity() && ( health > 0 ) && !( gameLocal.isMultiplayer && spectating ) ) {
		UpdateWeapon();
	}

	UpdateKillsMedals(); //ff1.3
	UpdateMedalsQueue(); //ff1.3

	UpdateHud();

	if ( gameLocal.isNewFrame ) {
		UpdatePowerUps();
	}

	UpdateDeathSkin( false );

	if ( head.GetEntity() ) {
		headRenderEnt = head.GetEntity()->GetRenderEntity();
	} else {
		headRenderEnt = NULL;
	}

	if ( headRenderEnt ) {
		if ( influenceSkin ) {
			headRenderEnt->customSkin = influenceSkin;
		} else {
			headRenderEnt->customSkin = NULL;
		}
	}

	if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() ) {
		renderEntity.suppressShadowInViewID	= 0;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = 0;
		}
	} else {
		renderEntity.suppressShadowInViewID	= entityNumber+1;
		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = entityNumber+1;
		}
	}
	// never cast shadows from our first-person muzzle flashes
	renderEntity.suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	if ( headRenderEnt ) {
		headRenderEnt->suppressShadowInLightID = LIGHTID_VIEW_MUZZLE_FLASH + entityNumber;
	}

	if ( !gameLocal.inCinematic ) {
		UpdateAnimation();
	}

#ifdef _D3XP
	if ( enviroSuitLight.IsValid() ) {
		idAngles lightAng = firstPersonViewAxis.ToAngles();
		idVec3 lightOrg = firstPersonViewOrigin;
		const idDict *lightDef = gameLocal.FindEntityDefDict( "envirosuit_light", false );

		idVec3 enviroOffset = lightDef->GetVector( "enviro_offset" );
		idVec3 enviroAngleOffset = lightDef->GetVector( "enviro_angle_offset" );

		lightOrg += (enviroOffset.x * firstPersonViewAxis[0]);
		lightOrg += (enviroOffset.y * firstPersonViewAxis[1]);
		lightOrg += (enviroOffset.z * firstPersonViewAxis[2]);
		lightAng.pitch += enviroAngleOffset.x;
		lightAng.yaw += enviroAngleOffset.y;
		lightAng.roll += enviroAngleOffset.z;

		enviroSuitLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
		enviroSuitLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
		enviroSuitLight.GetEntity()->UpdateVisuals();
		enviroSuitLight.GetEntity()->Present();
	}
#endif

	if ( gameLocal.isMultiplayer ) {
		DrawPlayerIcons();
	}

	Present();

	UpdateDamageEffects();

	LinkCombat();

	if ( gameLocal.isNewFrame && entityNumber == gameLocal.localClientNum ) {
		playerView.CalculateShake();
	}

#ifdef _D3XP
	// determine if portal sky is in pvs
	pvsHandle_t	clientPVS = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );
	gameLocal.portalSkyActive = gameLocal.pvs.CheckAreasForPortalSky( clientPVS, GetPhysics()->GetOrigin() );
	gameLocal.pvs.FreeCurrentPVS( clientPVS );
#endif
}

/*
================
idPlayer::GetPhysicsToVisualTransform
================
*/
bool idPlayer::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	if ( af.IsActive() ) {
		af.GetPhysicsToVisualTransform( origin, axis );
		return true;
	}

	// smoothen the rendered origin and angles of other clients
	// smooth self origin if snapshots are telling us prediction is off
	if ( gameLocal.isClient && gameLocal.framenum >= smoothedFrame && ( entityNumber != gameLocal.localClientNum || selfSmooth ) ) {
		// render origin and axis
		idMat3 renderAxis = viewAxis * GetPhysics()->GetAxis();
		idVec3 renderOrigin = GetPhysics()->GetOrigin() + modelOffset * renderAxis;

		// update the smoothed origin
		if ( !smoothedOriginUpdated ) {
			idVec2 originDiff = renderOrigin.ToVec2() - smoothedOrigin.ToVec2();
			if ( originDiff.LengthSqr() < Square( 100.0f ) ) {
				// smoothen by pushing back to the previous position
				if ( selfSmooth ) {
					assert( entityNumber == gameLocal.localClientNum );
					renderOrigin.ToVec2() -= net_clientSelfSmoothing.GetFloat() * originDiff;
				} else {
					renderOrigin.ToVec2() -= gameLocal.clientSmoothing * originDiff;
				}
			}
			smoothedOrigin = renderOrigin;

			smoothedFrame = gameLocal.framenum;
			smoothedOriginUpdated = true;
		}

		axis = idAngles( 0.0f, smoothedAngles.yaw, 0.0f ).ToMat3();
		origin = ( smoothedOrigin - GetPhysics()->GetOrigin() ) * axis.Transpose();

	} else {

		axis = viewAxis;
		origin = modelOffset;
	}
	return true;
}

/*
================
idPlayer::GetPhysicsToSoundTransform
================
*/
bool idPlayer::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	idCamera *camera;

	if ( privateCameraView ) {
		camera = privateCameraView;
	} else {
		camera = gameLocal.GetCamera();
	}

	if ( camera ) {
		renderView_t view;

		memset( &view, 0, sizeof( view ) );
		camera->GetViewParms( &view );
		origin = view.vieworg;
		axis = view.viewaxis;
		return true;
	} else {
		return idActor::GetPhysicsToSoundTransform( origin, axis );
	}
}

/*
================
idPlayer::WriteToSnapshot
================
*/
void idPlayer::WriteToSnapshot( idBitMsgDelta &msg ) const {
	physicsObj.WriteToSnapshot( msg );
	WriteBindToSnapshot( msg );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[0] );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[1] );
	msg.WriteDeltaFloat( 0.0f, deltaViewAngles[2] );
	msg.WriteShort( health );
	msg.WriteBits( gameLocal.ServerRemapDecl( -1, DECL_ENTITYDEF, lastDamageDef ), gameLocal.entityDefBits );
	msg.WriteDir( lastDamageDir, 9 );
	msg.WriteShort( lastDamageLocation );
	msg.WriteBits( idealWeapon, idMath::BitsForInteger( MAX_WEAPONS ) );
	msg.WriteBits( inventory.weapons, MAX_WEAPONS );
	msg.WriteBits( weapon.GetSpawnId(), 32 );
	msg.WriteBits( spectator, idMath::BitsForInteger( MAX_CLIENTS ) );
	msg.WriteBits( lastHitToggle, 1 );
	msg.WriteBits( weaponGone, 1 );
	msg.WriteBits( isLagged, 1 );
	msg.WriteBits( isChatting, 1 );
#ifdef CTF
	/* Needed for the scoreboard */
	msg.WriteBits( carryingFlag, 1 );
#endif
#ifdef _D3XP
	msg.WriteBits( enviroSuitLight.GetSpawnId(), 32 );
#endif
}

/*
================
idPlayer::ReadFromSnapshot
================
*/
void idPlayer::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	int		i, oldHealth, newIdealWeapon, weaponSpawnId;
	bool	newHitToggle, stateHitch;

	if ( snapshotSequence - lastSnapshotSequence > 1 ) {
		stateHitch = true;
	} else {
		stateHitch = false;
	}
	lastSnapshotSequence = snapshotSequence;

	oldHealth = health;

	physicsObj.ReadFromSnapshot( msg );
	ReadBindFromSnapshot( msg );
	deltaViewAngles[0] = msg.ReadDeltaFloat( 0.0f );
	deltaViewAngles[1] = msg.ReadDeltaFloat( 0.0f );
	deltaViewAngles[2] = msg.ReadDeltaFloat( 0.0f );
	health = msg.ReadShort();
	lastDamageDef = gameLocal.ClientRemapDecl( DECL_ENTITYDEF, msg.ReadBits( gameLocal.entityDefBits ) );
	lastDamageDir = msg.ReadDir( 9 );
	lastDamageLocation = msg.ReadShort();
	newIdealWeapon = msg.ReadBits( idMath::BitsForInteger( MAX_WEAPONS ) );
	inventory.weapons = msg.ReadBits( MAX_WEAPONS );
	weaponSpawnId = msg.ReadBits( 32 );
	spectator = msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) );
	newHitToggle = msg.ReadBits( 1 ) != 0;
	weaponGone = msg.ReadBits( 1 ) != 0;
	isLagged = msg.ReadBits( 1 ) != 0;
	isChatting = msg.ReadBits( 1 ) != 0;
#ifdef CTF
	carryingFlag = msg.ReadBits( 1 ) != 0;
#endif
#ifdef _D3XP
	int enviroSpawnId;
	enviroSpawnId = msg.ReadBits( 32 );
	enviroSuitLight.SetSpawnId( enviroSpawnId );
#endif

	// no msg reading below this

	if ( weapon.SetSpawnId( weaponSpawnId ) ) {
		if ( weapon.GetEntity() ) {
			// maintain ownership locally
			weapon.GetEntity()->SetOwner( this );
		}
		currentWeapon = -1;
	}
	// if not a local client assume the client has all ammo types
	if ( entityNumber != gameLocal.localClientNum ) {
		for( i = 0; i < AMMO_NUMTYPES; i++ ) {
			inventory.ammo[ i ] = 999;
		}
	}

	if ( oldHealth > 0 && health <= 0 ) {
		if ( stateHitch ) {
			// so we just hide and don't show a death skin
			UpdateDeathSkin( true );
		}
		// die
		AI_DEAD = true;
		ClearPowerUps();
		SetAnimState( ANIMCHANNEL_LEGS, "Legs_Death", 4 );
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_Death", 4 );
		SetWaitState( "" );
		animator.ClearAllJoints();
		if ( entityNumber == gameLocal.localClientNum ) {
			playerView.Fade( colorBlack, 12000 );
		}
		StartRagdoll();
		physicsObj.SetMovementType( PM_DEAD );
		if ( !stateHitch ) {
			StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
		}
		if ( weapon.GetEntity() ) {
			weapon.GetEntity()->OwnerDied();
		}
	} else if ( oldHealth <= 0 && health > 0 ) {
		// respawn
		Init();
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.EnableClip();
		SetCombatContents( true );
	} else if ( health < oldHealth && health > 0 ) {
		if ( stateHitch ) {
			lastDmgTime = gameLocal.time;
		} else {
			// damage feedback
			const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>( declManager->DeclByIndex( DECL_ENTITYDEF, lastDamageDef, false ) );
			if ( def ) {
				playerView.DamageImpulse( lastDamageDir * viewAxis.Transpose(), &def->dict );
				AI_PAIN = Pain( NULL, NULL, oldHealth - health, lastDamageDir, lastDamageLocation );
				lastDmgTime = gameLocal.time;
			} else {
				common->Warning( "NET: no damage def for damage feedback '%d'\n", lastDamageDef );
			}
		}
	} else if ( health > oldHealth && PowerUpActive( MEGAHEALTH ) && !stateHitch ) {
		// just pulse, for any health raise
		healthPulse = true;
	}

#ifdef _D3XP
	// If the player is alive, restore proper physics object
	if ( health > 0 && IsActiveAF() ) {
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.EnableClip();
		SetCombatContents( true );
	}
#endif

	if ( idealWeapon != newIdealWeapon ) {
		if ( stateHitch ) {
			weaponCatchup = true;
		}
		idealWeapon = newIdealWeapon;
		UpdateHudWeapon();
	}

	if ( lastHitToggle != newHitToggle ) {
		SetLastHitTime( gameLocal.realClientTime );
	}

	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}
}

/*
================
idPlayer::WritePlayerStateToSnapshot
================
*/
void idPlayer::WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const {
	int i;

	msg.WriteByte( bobCycle );
	msg.WriteInt( stepUpTime );
	msg.WriteFloat( stepUpDelta );
#ifdef _D3XP
	msg.WriteInt( inventory.weapons );
#else
	msg.WriteShort( inventory.weapons );
#endif
	msg.WriteByte( inventory.armor );

	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		msg.WriteBits( inventory.ammo[i], ASYNC_PLAYER_INV_AMMO_BITS );
	}
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		msg.WriteBits( inventory.clip[i], ASYNC_PLAYER_INV_CLIP_BITS );
	}
}

/*
================
idPlayer::ReadPlayerStateFromSnapshot
================
*/
void idPlayer::ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg ) {
	int i, ammo;

	bobCycle = msg.ReadByte();
	stepUpTime = msg.ReadInt();
	stepUpDelta = msg.ReadFloat();
#ifdef _D3XP
	inventory.weapons = msg.ReadInt();
#else
	inventory.weapons = msg.ReadShort();
#endif
	inventory.armor = msg.ReadByte();

	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		ammo = msg.ReadBits( ASYNC_PLAYER_INV_AMMO_BITS );
		if ( gameLocal.time >= inventory.ammoPredictTime ) {
			inventory.ammo[ i ] = ammo;
		}
	}
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		inventory.clip[i] = msg.ReadBits( ASYNC_PLAYER_INV_CLIP_BITS );
	}
}

/*
================
idPlayer::ServerReceiveEvent
================
*/
bool idPlayer::ServerReceiveEvent( int event, int time, const idBitMsg &msg ) {

	if ( idEntity::ServerReceiveEvent( event, time, msg ) ) {
		return true;
	}

	// client->server events
	switch( event ) {
		case EVENT_IMPULSE: {
			PerformImpulse( msg.ReadBits( 6 ) );
			return true;
		}
		default: {
			return false;
		}
	}
}

/*
================
idPlayer::ClientReceiveEvent
================
*/
bool idPlayer::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	int powerup;
	bool start;

	switch ( event ) {
		case EVENT_EXIT_TELEPORTER:
			Event_ExitTeleporter();
			return true;
		case EVENT_ABORT_TELEPORTER:
			SetPrivateCameraView( NULL );
			return true;
		case EVENT_POWERUP: {
			powerup = msg.ReadShort();
			start = msg.ReadBits( 1 ) != 0;
			if ( start ) {
				GivePowerUp( powerup, 0 );
			} else {
				ClearPowerup( powerup );
			}
			return true;
		}
#ifdef _D3XP
		case EVENT_PICKUPNAME: {
			char buf[MAX_EVENT_PARAM_SIZE];
			msg.ReadString(buf, MAX_EVENT_PARAM_SIZE);
			inventory.AddPickupName(buf, "", 0, this); //_D3XP
			return true;
		}
#endif
		case EVENT_SPECTATE: {
			bool spectate = ( msg.ReadBits( 1 ) != 0 );
			Spectate( spectate );
			return true;
		}
		case EVENT_ADD_DAMAGE_EFFECT: {
			if ( spectating ) {
				// if we're spectating, ignore
				// happens if the event and the spectate change are written on the server during the same frame (fraglimit)
				return true;
			}
			break;
		}
		default:
			break;
	}

	return idActor::ClientReceiveEvent( event, time, msg );
}

/*
================
idPlayer::Hide
================
*/
void idPlayer::Hide( void ) {
	idWeapon *weap;

	idActor::Hide();
	weap = weapon.GetEntity();
	if ( weap ) {
		weap->HideWorldModel();
	}
}

/*
================
idPlayer::Show
================
*/
void idPlayer::Show( void ) {
	idWeapon *weap;

	idActor::Show();
	weap = weapon.GetEntity();
	if ( weap ) {
		weap->ShowWorldModel();
	}
}

/*
===============
idPlayer::StartAudioLog
===============
*/
void idPlayer::StartAudioLog( void ) {
	if ( hud ) {
		hud->HandleNamedEvent( "audioLogUp" );
	}
}

/*
===============
idPlayer::StopAudioLog
===============
*/
void idPlayer::StopAudioLog( void ) {
	if ( hud ) {
		hud->HandleNamedEvent( "audioLogDown" );
	}
}

/*
===============
idPlayer::ShowTip
===============
*/
void idPlayer::ShowTip( const char *title, const char *tip, bool autoHide ) {
	if ( tipUp ) {
		return;
	}
	hud->SetStateString( "tip", tip );
	hud->SetStateString( "tiptitle", title );
	hud->HandleNamedEvent( "tipWindowUp" );
	if ( autoHide ) {
		CancelEvents( &EV_Player_HideTip ); //ivan - make sure there are no other events that could hide this too soon.
		PostEventSec( &EV_Player_HideTip, 10.0f ); //ivan - was 5.0f
	}
	tipUp = true;
}

/*
===============
idPlayer::HideTip
===============
*/
void idPlayer::HideTip( void ) {
	hud->HandleNamedEvent( "tipWindowDown" );
	tipUp = false;
}

/*
===============
idPlayer::Event_HideTip
===============
*/
void idPlayer::Event_HideTip( void ) {
	HideTip();
}

/*
===============
idPlayer::ShowObjective
===============
*/
void idPlayer::ShowObjective( const char *obj ) {
	hud->HandleNamedEvent( obj );
	objectiveUp = true;
}

/*
===============
idPlayer::HideObjective
===============
*/
void idPlayer::HideObjective( void ) {
	hud->HandleNamedEvent( "closeObjective" );
	objectiveUp = false;
}

/*
===============
idPlayer::Event_StopAudioLog
===============
*/
void idPlayer::Event_StopAudioLog( void ) {
	StopAudioLog();
}

/*
===============
idPlayer::SetSpectateOrigin
===============
*/
void idPlayer::SetSpectateOrigin( void ) {
	idVec3 neworig;

	neworig = GetPhysics()->GetOrigin();
	neworig[ 2 ] += EyeHeight();
	neworig[ 2 ] += 25;
	SetOrigin( neworig );
}

/*
===============
idPlayer::RemoveWeapon
===============
*/
void idPlayer::RemoveWeapon( const char *weap ) {
	if ( weap && *weap ) {
		//ff1.3 start
		//was: inventory.Drop( spawnArgs, spawnArgs.GetString( weap ), -1 );
		int weapon_index = inventory.Drop( spawnArgs, spawnArgs.GetString( weap ), -1 );

		//we must reset ammo clip on weapon entity, otherwise they'll be copied back to the inventory while the weapon is lowering
		if ( weapon.GetEntity() && weapon_index == currentWeapon ) {
			weapon.GetEntity()->ResetAmmoClip();
		}
		//ff1.3 end
	}
}

/*
===============
idPlayer::CanShowWeaponViewmodel
===============
*/
bool idPlayer::CanShowWeaponViewmodel( void ) const {
	return showWeaponViewModel;
}

/*
===============
idPlayer::SetLevelTrigger
===============
*/
void idPlayer::SetLevelTrigger( const char *levelName, const char *triggerName ) {
	if ( levelName && *levelName && triggerName && *triggerName ) {
		idLevelTriggerInfo lti;
		lti.levelName = levelName;
		lti.triggerName = triggerName;
		inventory.levelTriggers.Append( lti );
	}
}

/*
===============
idPlayer::Event_LevelTrigger
===============
*/
void idPlayer::Event_LevelTrigger( void ) {
	idStr mapName = gameLocal.GetMapName();
	mapName.StripPath();
	mapName.StripFileExtension();
	for ( int i = inventory.levelTriggers.Num() - 1; i >= 0; i-- ) {
		if ( idStr::Icmp( mapName, inventory.levelTriggers[i].levelName) == 0 ){
			idEntity *ent = gameLocal.FindEntity( inventory.levelTriggers[i].triggerName );
			if ( ent ) {
				ent->PostEventMS( &EV_Activate, 1, this );
			}
		}
	}
}

/*
===============
idPlayer::Event_Gibbed
===============
*/
void idPlayer::Event_Gibbed( void ) {
	// do nothing
}

/*
===============
idPlayer::UpdatePlayerIcons
===============
*/
void idPlayer::UpdatePlayerIcons( void ) {
	int time = networkSystem->ServerGetClientTimeSinceLastPacket( entityNumber );
	if ( time > cvarSystem->GetCVarInteger( "net_clientMaxPrediction" ) ) {
		isLagged = true;
	} else {
		isLagged = false;
	}
	// TODO: chatting, PDA, etc?
}

/*
===============
idPlayer::DrawPlayerIcons
===============
*/
void idPlayer::DrawPlayerIcons( void ) {
	if ( !NeedsIcon() ) {
		playerIcon.FreeIcon();
		return;
	}

#ifdef CTF
	// Never draw icons for hidden players.
	if ( this->IsHidden() )
		return;
#endif

	playerIcon.Draw( this, headJoint );
}

/*
===============
idPlayer::HidePlayerIcons
===============
*/
void idPlayer::HidePlayerIcons( void ) {
	playerIcon.FreeIcon();
}

/*
===============
idPlayer::NeedsIcon
==============
*/
bool idPlayer::NeedsIcon( void ) {
	// local clients don't render their own icons... they're only info for other clients
#ifdef CTF
	// always draw icons in CTF games
	return entityNumber != gameLocal.localClientNum && ( ( g_CTFArrows.GetBool() && gameLocal.mpGame.IsGametypeFlagBased() && !IsHidden() && !AI_DEAD ) || ( isLagged || isChatting ) );
#else
	return entityNumber != gameLocal.localClientNum && ( isLagged || isChatting );
#endif
}

#ifdef CTF
/*
===============
idPlayer::DropFlag()
==============
*/
void idPlayer::DropFlag( void ) {
	if ( !carryingFlag || !gameLocal.isMultiplayer || !gameLocal.mpGame.IsGametypeFlagBased() ) /* CTF */
		return;

	idEntity * entity = gameLocal.mpGame.GetTeamFlag( 1 - this->latchedTeam );
	if ( entity ) {
		idItemTeam * item = static_cast<idItemTeam*>(entity);

		if ( item->carried && !item->dropped ) {
			item->Drop( health <= 0 );
			carryingFlag = false;
		}
	}

}

void idPlayer::ReturnFlag() {

	if ( !carryingFlag || !gameLocal.isMultiplayer || !gameLocal.mpGame.IsGametypeFlagBased() ) /* CTF */
		return;

	idEntity * entity = gameLocal.mpGame.GetTeamFlag( 1 - this->latchedTeam );
	if ( entity ) {
		idItemTeam * item = static_cast<idItemTeam*>(entity);

		if ( item->carried && !item->dropped ) {
			item->Return();
			carryingFlag = false;
		}
	}
}

void idPlayer::FreeModelDef( void ) {
	idAFEntity_Base::FreeModelDef();
	if ( gameLocal.isMultiplayer && gameLocal.mpGame.IsGametypeFlagBased() )
		playerIcon.FreeIcon();
}

#endif

//ff1.3 start

/*
===============
idPlayer::Event_HudEvent
==============
*/
void idPlayer::Event_HudEvent( int guiType, const char* name ) {
	idUserInterface * gui = GetGuiByType(guiType);
	if ( gui ) {
		gui->HandleNamedEvent( name );
	}
}

/*
===============
idPlayer::Event_SetHudParm
==============
*/
void idPlayer::Event_SetHudParm(int guiType, const char *key, const char *val ) {
	idUserInterface * gui = GetGuiByType(guiType);
	if ( gui ) {
		gui->SetStateString( key, val );
	}
}

/*
===============
idPlayer::Event_SetHudFloat
==============
*/
void idPlayer::Event_SetHudFloat(int guiType, const char *key, float val ) {
	idUserInterface * gui = GetGuiByType(guiType);
	if ( gui ) {
		gui->SetStateString( key, va( "%f", val ) );
	}
}

/*
===============
idPlayer::Event_GetHudParm
==============
*/
void idPlayer::Event_GetHudParm(int guiType, const char *key) {
	idUserInterface * gui = GetGuiByType(guiType);
	idThread::ReturnString( gui ? gui->GetStateString(key) : "" );
}

/*
===============
idPlayer::Event_GetHudFloat
==============
*/
void idPlayer::Event_GetHudFloat(int guiType, const char *key) {
	idUserInterface * gui = GetGuiByType(guiType);
	idThread::ReturnFloat( gui ? gui->GetStateFloat(key) : 0.0f );
}


/*
===============
idPlayer::GetGuiByType
==============
*/
idUserInterface * idPlayer::GetGuiByType(int guiType) {
	switch( guiType ) {
		case GUI_CURSOR: {
			return cursor;
		}
		case GUI_HUD: {
			return hud;
		}
		case GUI_INFO: {
			return infoGui;
		}
	}
	return NULL;
}

/*
===============
idPlayer::Event_ForceUpdateNpcStatus
==============
*/
void idPlayer::Event_ForceUpdateNpcStatus( void ) {
	if ( focusCharacter && hud ) {
		if ( focusCharacter->spawnArgs.GetBool( "showStatus", "0" ) ) {
			hud->SetStateString( "npc", "Status:" );
			hud->SetStateString( "npc_action", focusCharacter->spawnArgs.GetString( "shownState", "Inactive" ) );
		}
	}
}

/*
===============
idPlayer::SecretFound
==============
*/
void idPlayer::SecretFound( void ) {
	GiveMedal( MEDAL_SECRET );
}

/*
===============
idPlayer::ShowWeaponOverview
===============
*/
void idPlayer::ShowWeaponOverview( void ) {
	if ( hud && ff_showWeaponOverview.GetBool() ) {
		hud->HandleNamedEvent( "weaponScroll" );
	}
}

/*
=================
idPlayer::Event_SetStamina
=================
*/
void idPlayer::Event_SetStamina( int stamina ) {
	staminaHelltime = stamina;
}

/*
=================
idPlayer::Event_GetStamina
=================
*/
void idPlayer::Event_GetStamina( void ) {
	idThread::ReturnInt( staminaHelltime );
}

/*
=================
idPlayer::Event_EnableStamina
=================
*/
void idPlayer::Event_EnableStamina( int on ) {
	staminaEnabled = ( on ) ? true : false;
}

/*
=================
idPlayer::Event_EnableMedals
=================
*/
void idPlayer::Event_EnableMedals( int on ) {
	medalsEnabled = ( on ) ? true : false;
}

/*
===============
idPlayer::UpdHudAmmoCurrentWeapon
==============
*/
void idPlayer::UpdHudAmmoCurrentWeapon( ammo_t type, int ammoinclip ) { //ff1.1 - called from the current weapon
	if ( currentWeapon < 0 ) {
		return;
	}
	if ( hud ) {

		//fix start: update the OTHER shotgun
		if (currentWeapon == 4) { //quick fix for shotguns - TODO: do this in a better way
			hud->SetStateInt( "ammo_weapon5" , (inventory.ammo[(int) type] + inventory.clip[5] ) );
		} else if (currentWeapon == 5) { //quick fix for shotguns - TODO: do this in a better way
			hud->SetStateInt( "ammo_weapon4" , (inventory.ammo[(int) type] + inventory.clip[4] ) );
		}
		/*
		if (currentWeapon == weapon_skull) { //quick fix for skulls
			hud->SetStateInt( "normal_skull_ammo" , (inventory.ammo[(int) type] - inventory.pw_ammo[currentWeapon] ) );
		}
		//fix end
		*/

		//gameLocal.Printf("ammo_weapon%d\n", currentWeapon);
		hud->SetStateInt( va( "ammo_weapon%d", currentWeapon ), (inventory.ammo[(int) type] + ammoinclip ) );
		hud->SetStateInt( va( "pw_ammo_weapon%d", currentWeapon ), (inventory.pw_ammo[currentWeapon] ) );
	}
}

/*
===============
idPlayer::GetPwAmmoCurrentWeapon
==============

int idPlayer::GetPwAmmoCurrentWeapon( void ) { //ff1.1 - called from the current weapon
	if ( currentWeapon < 0 ) { return 0; }
	return inventory.pw_ammo[currentWeapon];
}
*/
/*
===============
idPlayer::UsePwAmmoCurrentWeapon
==============

void idPlayer::UsePwAmmoCurrentWeapon( int amount ) { //ff1.1 - called from the current weapon
	if ( currentWeapon < 0 ) { return; }
	inventory.pw_ammo[currentWeapon] -= amount;
	if (inventory.pw_ammo[currentWeapon] < 0) {
		inventory.pw_ammo[currentWeapon] = 0;
	}
}
*/

/*
===============
idPlayer::AddPwAmmoWeapon
==============
*/
void idPlayer::AddPwAmmoWeapon( int weaponNum, int amount ) { //ff1.3
	inventory.AddPwAmmo(weaponNum, amount);
	if ( hud ) {
		hud->SetStateInt( va( "pw_ammo_weapon%d", weaponNum ), (inventory.HasPwAmmo(weaponNum) ) );
	}
}

/*
===============
idPlayer::UpdateEveryAmmoOnHud
==============
*/
void idPlayer::UpdateEveryAmmoOnHud( void ) { //ff1.1 - done at map start and on ammoPulse
	const char *weap;
	int w;
	int ammoType;

	if ( !hud ){
		return;
	}

	for( w = 0; w < MAX_WEAPONS; w++ ) {
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( !weap[ 0 ] ) {
			continue;
		}

		//get ammo type
		ammoType = (int) inventory.AmmoIndexForWeaponClass( weap, NULL );
		if ( ammoType == 0 ) {
			continue;
		}

		//upd hud
		//gameLocal.Printf("ammo_val: %d\n",temp_res);
		if ( w == weapon_skull ) { //fix for skulls weapon
			int skullSpecialAmmoTotal = 0;
			for( int i = 0; i < MAX_SKULL_MODES; i++ ) {
				hud->SetStateInt( va( "special_skull_ammo%d", i ), inventory.skullSpecialAmmo[i] );
				skullSpecialAmmoTotal += inventory.skullSpecialAmmo[i];
			}
			hud->SetStateInt( "default_skull_ammo", inventory.ammo[ammoType] - skullSpecialAmmoTotal );
			//gameLocal.Printf("default_skull_ammo: %d\n", inventory.ammo[ammoType] - skullSpecialAmmoTotal);
		}

		hud->SetStateInt( va( "ammo_weapon%d",w ), (inventory.clip[w] > 0) ? inventory.clip[w] + inventory.ammo[ammoType] : inventory.ammo[ammoType] );
		hud->SetStateInt( va( "pw_ammo_weapon%d",w ), inventory.pw_ammo[w] );
	}
}

/*
================
idPlayer::GetGuiCoordinates
================
*/
void idPlayer::GetGuiCoordinates( idEntity* target, idVec2 &guiPos ) {
	idVec3 ndc;
	idVec3 targetPos;

	if ( target != NULL ) {
		if ( target->IsType( idActor::Type ) ) {
			targetPos = static_cast<idActor *>(target)->GetEyePosition();
			//targetPos.z -= 12.0f; //same logic as guided proj
		} else {
			targetPos = target->GetPhysics()->GetAbsBounds().GetCenter();
		}
		if ( CheckFOV( targetPos, (float)cos( DEG2RAD( CalcFov(false) /** 0.5f*/ ) ) ) ) {
			renderSystem->GlobalToNormalizedDeviceCoordinates( targetPos, ndc );
			guiPos.x = idMath::ClampFloat( 0.0f, 640.0f, ( ndc.x * 0.5 + 0.5 ) * 640.0f );
			guiPos.y = idMath::ClampFloat( 0.0f, 480.0f, 480.0f - ( ndc.y * 0.5 + 0.5 ) * 480.0f );
		} else { //out of fov
			guiPos.x = -320.0f;
			guiPos.y = -240.0f;
		}
	} else { //centered by default
		guiPos.x = 320.0f;
		guiPos.y = 240.0f;
	}
	//gameLocal.Printf( "Lock pos %s\n", guiPos.ToString() );
}

/*
================
idPlayer::UpdateLockCursor
================
*/
void idPlayer::UpdateLockCursor( idEntity* target ) {
	if ( cursor != NULL ) {
		idVec2 guiPos;
		GetGuiCoordinates(target, guiPos);
		cursor->SetStateInt( "lock_y", (int) guiPos.y );
		cursor->SetStateInt( "lock_x", (int) guiPos.x );
	}
}

/*
================
idPlayer::Event_StartRiding
================

void idPlayer::Event_StartRiding( idEntity* aiEnt ){
	//gameLocal.Printf("idPlayer::Event_StartRiding\n");
	if ( aiEnt && aiEnt->IsType( idAI_Rideable::Type ) ) {
		EnterAI( static_cast< idAI_Rideable * >( aiEnt ) );
		//static_cast< idAI_Rideable * >( aiEnt )->PreStartRiding( this );
	}
}
*/
/*
================
idPlayer::Event_StopRiding
================

void idPlayer::Event_StopRiding( void ){
	if ( currentRiddenAI ) {
		currentRiddenAI->PreStopRiding();
	}
}
*/

/*
================
idPlayer::Event_IsOnVehicle
================
*/
void idPlayer::Event_IsOnVehicle( void ) {
	idThread::ReturnInt( (currentVehicle ? 1 : 0));
}

/*
================
idPlayer::Event_IsOnPossession
================
*/
void idPlayer::Event_IsOnPossession( void ) {
	idThread::ReturnInt( (currentRiddenAI ? 1 : 0));
}

/*
================
idPlayer::Event_GetAlliesDistanceOffset
================
*/
void idPlayer::Event_GetAlliesDistanceOffset( void ) {
	if ( currentRiddenAI ) {
		const idBounds &myBounds = GetPhysics()->GetBounds();
		const idBounds &aiBounds = currentRiddenAI->GetPhysics()->GetBounds();
		idThread::ReturnFloat( Max( 0.0f, aiBounds[1][0] - myBounds[1][0] ) ); //NOTE: use only max X. This relies on the fact that BBox is symmetrical for AI and player.
	} else {
		idThread::ReturnFloat( 0.0f );
	}
}

/*
=====================
idActor::CheckFOV
=====================
*/
bool idPlayer::CheckFOV( const idVec3 &pos, float targetFovDot ) const { //ff1.3
	//float targetFovDot = (float)cos( DEG2RAD( CalcFov(false) /** 0.5f*/ ) );
	if ( targetFovDot == 1.0f ) {
		return true;
	}

	float	dot;
	idVec3	delta;

	delta = pos - renderView->vieworg; //GetEyePosition();

	// get our gravity normal
	const idVec3 &gravityDir = GetPhysics()->GetGravityNormal();

	// infinite vertical vision, so project it onto our orientation plane
	delta -= gravityDir * ( gravityDir * delta );

	delta.Normalize();
	dot = renderView->viewaxis[ 0 ] * delta;

	return ( dot >= targetFovDot );
}

/*
================
idPlayer::ComboCallback
================
*/
void idPlayer::ComboCallback( idProjectile* comboProj ){
	ammo_t comboAmmoType = inventory.AmmoIndexForAmmoClass( comboProj->spawnArgs.GetString("combo_ammoType")  );
	if ( comboAmmoType ) {
		inventory.UseAvailAmmo(comboAmmoType, comboProj->spawnArgs.GetInt("combo_ammoAmount", "1"));
		UpdHudAmmoCurrentWeapon(comboAmmoType, 0);
	}
}

/*
===============
idPlayer::PossessionProjectileHitCallback
===============
*/
void idPlayer::PossessionProjectileHitCallback( idProjectile* possessionProj ) {
	inventory.UsePwAmmo( weapon_possession, 1 );
}


/*
================
idPlayer::AnyFireButtonPressed
================
*/
bool idPlayer::AnyFireButtonPressed( void ){
	return ( usercmd.buttons & BUTTON_ATTACK || usercmd.buttons & BUTTON_5 );
}

//ff1.3 end
