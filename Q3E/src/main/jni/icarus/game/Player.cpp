/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

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

// boyette bit - I am trying to get this to work. - SUCCESS - it's in the header.
//#include <bitset>
//std::bitset<1024>		willbeweapons();
//std::bitset<1024>		willbeweapons; - this does work and so does the above.
// boyette bit - This works, now you just have to figure out how to get it into the header file. - SUCCESS


#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
// boyette bit - I am trying to get this to work. -- so it doesn't work down here - but it works above.
//#include <bitset>

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
const idEventDef EV_Player_GiveInventoryItem( "giveInventoryItem", "s" );
const idEventDef EV_Player_RemoveInventoryItem( "removeInventoryItem", "s" );
const idEventDef EV_Player_GetIdealWeapon( "getIdealWeapon", NULL, 's' );
const idEventDef EV_Player_SetPowerupTime( "setPowerupTime", "dd" );
const idEventDef EV_Player_IsPowerupActive( "isPowerupActive", "d", 'd' );
const idEventDef EV_Player_WeaponAvailable( "weaponAvailable", "s", 'd');
const idEventDef EV_Player_StartWarp( "startWarp" );
const idEventDef EV_Player_StopWarpEffects( "stopWarpEffects" );
const idEventDef EV_Player_StopSlowMo( "stopSlowMo" );
const idEventDef EV_Player_TransitionNumBloomPassesToZero( "TransitionNumBloomPassesToZero" );
const idEventDef EV_Player_TransitionNumBloomPassesToThirty( "TransitionNumBloomPassesToThirty" );
const idEventDef EV_Player_UpdateSomeThingsAftersbShipIsRemoved( "UpdateSomeThingsAftersbShipIsRemoved" );
const idEventDef EV_Player_AllowPlayerFiring( "<allowPlayerFiring>" );
const idEventDef EV_Player_StopHelltime( "stopHelltime", "d" );
const idEventDef EV_Player_ToggleBloom( "toggleBloom", "d" );
const idEventDef EV_Player_SetBloomParms( "setBloomParms", "ff" );
// BOYETTE MUSIC BEGIN
const idEventDef EV_Player_MonitorMusic( "<monitorMusic>" );
const idEventDef EV_Player_CancelMonitorMusic( "cancelMonitorMusic" );
// BOYETTE MUSIC END

// BOYETTE WEIRD TEXTURE THRASHING FIX BEGIN
const idEventDef EV_Player_UpdateWeirdTextureThrashFix( "<updateWeirdTextureThrashFix>" );
// BOYETTE WEIRD TEXTURE THRASHING FIX END

// BOYETTE OVERLAY GUI SCRIPT BEGIN
const idEventDef EV_Player_OpenOverlayGUI( "openOverlayGUI", "s" );
// BOYETTE OVERLAY GUI SCRIPT END

// BOYETTE OUTRO INVENTORY ITEM SCRIPTS BEGIN
const idEventDef EV_Player_HasInventoryItem( "hasInventoryItem", "s", 'd' );
// BOYETTE OUTRO INVENTORY ITEM SCRIPTS END

// BOYETTE DELAY NONINTERACTIVE ALERTS BEGIN
const idEventDef EV_Player_DisplayNonInteractiveAlertMessage( "displayNonInteractiveAlertMessage", "s" );
// BOYETTE DELAY NONINTERACTIVE ALERTS BEGIN

// BOYETTE TUTORIAL MODE BEGIN
const idEventDef EV_Player_ToggleTutorialMode( "toggleTutorialMode", "d" );
const idEventDef EV_Player_IncreaseTutorialModeStep( "increaseTutorialModeStep" );
const idEventDef EV_Player_GetTutorialModeStep( "getTutorialModeStep", NULL, 'd' );
const idEventDef EV_Player_WaitForImpulse( "waitForImpulse", "d" );
// BOYETTE TUTORIAL MODE END

// BOYETTE STEAM INTEGRATION BEGIN
const idEventDef EV_Player_IncreaseSteamAchievementStat( "increaseSteamAchievementStat", "d" );
// BOYETTE STEAM INTEGRATION END


CLASS_DECLARATION( idActor, idPlayer )
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
	EVENT( EV_Player_GiveInventoryItem,		idPlayer::Event_GiveInventoryItem )
	EVENT( EV_Player_RemoveInventoryItem,	idPlayer::Event_RemoveInventoryItem )
	EVENT( EV_Player_GetIdealWeapon,		idPlayer::Event_GetIdealWeapon )
	EVENT( EV_Player_WeaponAvailable,		idPlayer::Event_WeaponAvailable )
	EVENT( EV_Player_SetPowerupTime,		idPlayer::Event_SetPowerupTime )
	EVENT( EV_Player_IsPowerupActive,		idPlayer::Event_IsPowerupActive )
	EVENT( EV_Player_StartWarp,				idPlayer::Event_StartWarp )
	EVENT( EV_Player_StopWarpEffects,		idPlayer::Event_StopWarpEffects )
	EVENT( EV_Player_StopSlowMo,			idPlayer::Event_StopSlowMo )
	EVENT( EV_Player_TransitionNumBloomPassesToZero,			idPlayer::Event_TransitionNumBloomPassesToZero )
	EVENT( EV_Player_TransitionNumBloomPassesToThirty,			idPlayer::Event_TransitionNumBloomPassesToThirty )
	EVENT( EV_Player_UpdateSomeThingsAftersbShipIsRemoved,			idPlayer::Event_UpdateSomeThingsAftersbShipIsRemoved )
	EVENT( EV_Player_AllowPlayerFiring,		idPlayer::Event_AllowPlayerFiring )
	EVENT( EV_Player_StopHelltime,			idPlayer::Event_StopHelltime )
	EVENT( EV_Player_ToggleBloom,			idPlayer::Event_ToggleBloom )
	EVENT( EV_Player_SetBloomParms,			idPlayer::Event_SetBloomParms )
	// BOYETTE MUSIC BEGIN
	EVENT( EV_Player_MonitorMusic,			idPlayer::Event_MonitorMusic )
	EVENT( EV_Player_CancelMonitorMusic,	idPlayer::Event_CancelMonitorMusic )
	// BOYETTE MUSIC END

	// BOYETTE WEIRD TEXTURE THRASHING FIX BEGIN
	EVENT( EV_Player_UpdateWeirdTextureThrashFix,			idPlayer::Event_UpdateWeirdTextureThrashFix )
	// BOYETTE WEIRD TEXTURE THRASHING FIX END

	// BOYETTE OVERLAY GUI SCRIPT BEGIN
	EVENT( EV_Player_OpenOverlayGUI,			idPlayer::Event_OpenOverlayGUI )
	// BOYETTE OVERLAY GUI SCRIPT END

	// BOYETTE OUTRO INVENTORY ITEM SCRIPTS BEGIN
	EVENT( EV_Player_HasInventoryItem,			idPlayer::Event_HasInventoryItem )
	// BOYETTE OUTRO INVENTORY ITEM SCRIPTS END

	// BOYETTE DELAY NONINTERACTIVE ALERTS BEGIN
	EVENT( EV_Player_DisplayNonInteractiveAlertMessage,			idPlayer::Event_DisplayNonInteractiveAlertMessage )
	// BOYETTE DELAY NONINTERACTIVE ALERTS BEGIN

	// BOYETTE TUTORIAL MODE BEGIN
	EVENT( EV_Player_ToggleTutorialMode,			idPlayer::Event_ToggleTutorialMode )
	EVENT( EV_Player_IncreaseTutorialModeStep,		idPlayer::Event_IncreaseTutorialModeStep )
	EVENT( EV_Player_GetTutorialModeStep,			idPlayer::Event_GetTutorialModeStep )
	EVENT( EV_Player_WaitForImpulse,				idPlayer::Event_WaitForImpulse )
	// BOYETTE TUTORIAL MODE END

	// BOYETTE STEAM INTEGRATION BEGIN
	EVENT( EV_Player_IncreaseSteamAchievementStat,	idPlayer::Event_IncreaseSteamAchievementStat )
	// BOYETTE STEAM INTEGRATION END
END_CLASS

const int MAX_RESPAWN_TIME = 10000;
const int RAGDOLL_DEATH_TIME = 3000;
const int MAX_PDAS = 64;
const int MAX_PDA_ITEMS = 128;
const int STEPUP_TIME = 200;
const int MAX_INVENTORY_ITEMS = 20;


idVec3 idPlayer::colorBarTable[ 8 ] = { 
	idVec3( 0.25f, 0.25f, 0.25f ),
	idVec3( 1.00f, 0.00f, 0.00f ),
	idVec3( 0.00f, 0.80f, 0.10f ),
	idVec3( 0.20f, 0.50f, 0.80f ),
	idVec3( 1.00f, 0.80f, 0.10f )
	,idVec3( 0.425f, 0.484f, 0.445f ),
	idVec3( 0.39f, 0.199f, 0.3f ),
	idVec3( 0.484f, 0.312f, 0.074f)
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
	// boyette bit test begin
	//weaponsBinaryString = "0101010101010101010101010101010101010101010101010101010101010101";
	//weaponsBinaryString = weapons.to_string<char,std::char_traits<char>,std::allocator<char> >();
	//weapons = std::bitset<MAX_WEAPONS>(weaponsBinaryString);
	// boyette bit test end

	memset( ammo, 0, sizeof( ammo ) );

	ClearPowerUps();

	// set to -1 so that the gun knows to have a full clip the first time we get it and at the start of the level
	memset( clip, -1, sizeof( clip ) );
	
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
			case INVULNERABILITY:
				def = gameLocal.FindEntityDef( "powerup_invulnerability", false );
				break;
			/*case HASTE:
				def = gameLocal.FindEntityDef( "powerup_haste", false );
				break;*/
		}
		assert( def );
		msec = def->dict.GetInt( "time" ) * 1000;
	}
	powerups |= 1 << powerup;
	powerupEndTime[ powerup ] = gameLocal.time + msec;
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
	// boyette bit begin
	bool	weaponsBitArray[MAX_WEAPONS];
	// boyette bit end

	// armor
	dict.SetInt( "armor", armor );

    // don't bother with powerups, maxhealth, maxarmor, or the clip

	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if ( name ) {
			dict.SetInt( name, ammo[ i ] );
		}
	}

	//Save the clip data
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		dict.SetInt( va("clip%i", i), clip[ i ] );
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
	// boyette bit- this is the original doom one - dict.SetInt( "weapon_bits", weapons );
	// boyette bit begin
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		weaponsBitArray[ i ] = weapons.test(i);
		dict.SetBool( va("weapon_bits_%i", i), weaponsBitArray[i] );
	}
	// boyette bit begin

	dict.SetInt( "levelTriggers", levelTriggers.Num() );
	for ( i = 0; i < levelTriggers.Num(); i++ ) {
		sprintf( key, "levelTrigger_Level_%i", i );
		dict.Set( key, levelTriggers[i].levelName );
		sprintf( key, "levelTrigger_Trigger_%i", i );
		dict.Set( key, levelTriggers[i].triggerName );
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
	// boyette bit begin
	bool	weaponsBitArray[MAX_WEAPONS];
	// boyette bit end

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

	//Restore the clip data
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		clip[i] = dict.GetInt(va("clip%i", i), "-1");
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
	// boyette bit - weapons	= dict.GetInt( "weapon_bits", "0" );
	// boyette bit begin
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		weaponsBitArray[i] = dict.GetBool(va("weapon_bits_%i", i), "0" );
		weapons.set(i,weaponsBitArray[i]);
	}
	// boyette bit end

#ifdef ID_DEMO_BUILD
		Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false );
#else
	if ( g_skill.GetInteger() >= 3 ) {
		Give( owner, dict, "weapon", dict.GetString( "weapon_nightmare" ), NULL, false );
	} else {
		Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false );
	}
#endif

	num = dict.GetInt( "levelTriggers" );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "levelTrigger_Level_%i", i );
		idLevelTriggerInfo lti;
		lti.levelName = dict.GetString( itemname );
		sprintf( itemname, "levelTrigger_Trigger_%i", i );
		lti.triggerName = dict.GetString( itemname );
		levelTriggers.Append( lti );
	}

}

/*
==============
idInventory::Save
==============
*/
void idInventory::Save( idSaveGame *savefile ) const {
	int i;
	// boyette bit begin
	bool	weaponsBitArray[MAX_WEAPONS];
	// boyette bit end


	savefile->WriteInt( maxHealth );
	//weaponsBinaryString = "0101010101010101010101010101010101010101010101010101010101010101";
	//willbeweapons = std::bitset<MAX_WEAPONS>(weaponsBinaryString);
	//weaponsBinaryString = weapons.to_string<char,std::char_traits<char>,std::allocator<char>>();
	// boyette bit - savefile->WriteInt( weapons );
	// boyette bit begin
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		weaponsBitArray[ i ] = weapons.test(i);
		savefile->WriteBool( weaponsBitArray[i] );
	}
	// boyette bit end
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
	}

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

	for(i = 0; i < AMMO_NUMTYPES; i++) {
		savefile->WriteInt(rechargeAmmo[i].ammo);
		savefile->WriteInt(rechargeAmmo[i].rechargeTime);
		savefile->WriteString(rechargeAmmo[i].ammoName);
	}
}

/*
==============
idInventory::Restore
==============
*/
void idInventory::Restore( idRestoreGame *savefile ) {
	int i, num;
	bool	weaponsBitArray[MAX_WEAPONS];

	savefile->ReadInt( maxHealth );
// boyette bit - this is the original doom one - savefile->ReadInt( weapons );
	// boyette bit begin
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		savefile->ReadBool(weaponsBitArray[ i ]);
		weapons.set(i,weaponsBitArray[ i ]);
	}
	// boyette bit end

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

		pickupItemNames.Append( info );
	}

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

	for(i = 0; i < AMMO_NUMTYPES; i++) {
		savefile->ReadInt(rechargeAmmo[i].ammo);
		savefile->ReadInt(rechargeAmmo[i].rechargeTime);
		
		idStr name;
		savefile->ReadString(name);
		strcpy(rechargeAmmo[i].ammoName, name);
	}
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
idInventory::AmmoIndexForAmmoClass
==============
*/
int idInventory::MaxAmmoForAmmoClass( idPlayer *owner, const char *ammo_classname ) const {
	return owner->spawnArgs.GetInt( va( "max_%s", ammo_classname ), "0" );
}

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
void idInventory::AddPickupName( const char *name, const char *icon, idPlayer* owner ) { //_D3XP
	int num;

	num = pickupItemNames.Num();
	if ( ( num == 0 ) || ( pickupItemNames[ num - 1 ].name.Icmp( name ) != 0 ) ) {
		idItemInfo &info = pickupItemNames.Alloc();

		if ( idStr::Cmpn( name, STRTABLE_ID, STRTABLE_ID_LENGTH ) == 0 ) {
			info.name = common->GetLanguageDict()->GetString( name );
		} else {
			info.name = name;
		}
		info.icon = icon;

		if ( gameLocal.isServer ) {
			idBitMsg	msg;
			byte		msgBuf[MAX_EVENT_PARAM_SIZE];

			msg.Init( msgBuf, sizeof( msgBuf ) );
			msg.WriteString( name, MAX_EVENT_PARAM_SIZE );
			owner->ServerSendEvent( idPlayer::EVENT_PICKUPNAME, &msg, false, -1 );
		}
	} 
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
	idStr					weaponString;
	int						max;
	const idDeclEntityDef	*weaponDecl;
	bool					tookWeapon;
	int						amount;
	idItemInfo				info;
	const char				*name;

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

	if ( !idStr::Icmpn( statname, "ammo_", 5 ) ) {
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );
		if ( ammo[ i ] >= max ) {
			return false;
		}
		amount = atoi( value );
		if ( amount ) {			
			ammo[ i ] += amount;
			if ( ( max > 0 ) && ( ammo[ i ] > max ) ) {
				ammo[ i ] = max;
			}
			ammoPulse = true;

			name = AmmoPickupNameForIndex( i );
			if ( idStr::Length( name ) ) {
				AddPickupName( name, "", owner ); //_D3XP
			}
		}
	} else if ( !idStr::Icmp( statname, "armor" ) ) {
		if ( armor >= maxarmor ) {
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
		}
	} else if ( idStr::FindText( statname, "inclip_" ) == 0 ) {
		idStr temp = statname;
		i = atoi(temp.Mid(7, 2));
		if ( i != -1 ) {
			// set, don't add. not going over the clip size limit.
			clip[ i ] = atoi( value );
		}
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
				gameLocal.Warning( "Unknown weapon '%s'", weaponName.c_str() );
				continue;
			}

			// cache the media for this weapon
			weaponDecl = gameLocal.FindEntityDef( weaponName, false );

			// don't pickup "no ammo" weapon types twice
			// not for D3 SP .. there is only one case in the game where you can get a no ammo
			// weapon when you might already have it, in that case it is more conistent to pick it up
			if ( gameLocal.isMultiplayer && weaponDecl && ( weapons.test(i) ) && !weaponDecl->dict.GetInt( "ammoRequired" ) ) {
				continue;
			}

			if ( !gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || ( weaponName == "weapon_space_command_fists" ) || ( weaponName == "weapon_soulcube" ) ) {
				if ( ( weapons.test(i) ) == 0 || gameLocal.isMultiplayer ) { // boyette note- so if they do not have the weapon it will be given to them farther down with: weapons |= ( 1 << i );      +the gameLocal.isMultiplayer must be for network purposes - the give console command(above this function) is disabled in multiplayer
					if ( owner->GetUserInfo()->GetBool( "ui_autoSwitch" ) && idealWeapon && i != owner->weapon_bloodstone_active1 && i != owner->weapon_bloodstone_active2 && i != owner->weapon_bloodstone_active3) {
						assert( !gameLocal.isClient );
						*idealWeapon = i;
					} 
					if ( owner->hud && updateHud && lastGiveTime + 1000 < gameLocal.time ) {
						owner->hud->SetStateInt( "newWeapon", i );
						owner->hud->HandleNamedEvent( "newWeapon" );
						lastGiveTime = gameLocal.time;
					}
					weaponPulse = true;
					weapons.set(i);
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
void idInventory::Drop( const idDict &spawnArgs, const char *weapon_classname, int weapon_index ) {
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
	weapons.reset(weapon_index);
	ammo_t ammo_i = AmmoIndexForWeaponClass( weapon_classname, NULL );
	if ( ammo_i ) {
		clip[ weapon_index ] = -1;
		ammo[ ammo_i ] = 0;
	}
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

	int ammoCount = HasAmmo( ammo_i, ammoRequired );
	if(includeClip && owner) {
		ammoCount += clip[owner->SlotForWeapon(weapon_classname)];
	}
	return ammoCount;

}
	
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
idInventory::GiveAmmo
===============
*/
bool idInventory::GiveAmmo( ammo_t type, int amount ) {
	//if ( !HasAmmo( type, amount ) ) {
	//	return false;
	//}

	if ( ammo[ type ] >= 0 ) {
		ammo[ type ] += amount;
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

/*
===============
idInventory::InitRechargeAmmo
===============
* Loads any recharge ammo definitions from the ammo_types entity definitions.
*/
void idInventory::InitRechargeAmmo(idPlayer *owner) {

	memset (rechargeAmmo, 0, sizeof(rechargeAmmo));

	const idKeyValue *kv = owner->spawnArgs.MatchPrefix( "ammorecharge_" );
	while( kv ) {
		idStr key = kv->GetKey();
		idStr ammoname = key.Right(key.Length()- strlen("ammorecharge_"));
		int ammoType = AmmoIndexForAmmoClass(ammoname);
		rechargeAmmo[ammoType].ammo = (atof(kv->GetValue().c_str())*1000);
		strcpy(rechargeAmmo[ammoType].ammoName, ammoname);
		kv = owner->spawnArgs.MatchPrefix( "ammorecharge_", kv );
	}
}

/*
===============
idInventory::RechargeAmmo
===============
* Called once per frame to update any ammo amount for ammo types that recharge.
*/
void idInventory::RechargeAmmo(idPlayer *owner) {

	for(int i = 0; i < AMMO_NUMTYPES; i++) {
		if(rechargeAmmo[i].ammo > 0) {
			if(!rechargeAmmo[i].rechargeTime)  {
				//Initialize the recharge timer.
				rechargeAmmo[i].rechargeTime = gameLocal.time;
			}
			int elapsed = gameLocal.time - rechargeAmmo[i].rechargeTime;
			if(elapsed >= rechargeAmmo[i].ammo) {
				int intervals = (gameLocal.time - rechargeAmmo[i].rechargeTime)/rechargeAmmo[i].ammo;
				ammo[i] += intervals;

				int max = MaxAmmoForAmmoClass(owner, rechargeAmmo[i].ammoName);
				if(max > 0) {
					if(ammo[i] > max) {
						ammo[i] = max;
					}
				}
				rechargeAmmo[i].rechargeTime += intervals*rechargeAmmo[i].ammo;
			}
		}
	}
}

/*
===============
idInventory::CanGive
===============
*/
bool idInventory::CanGive( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon ) {

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
	} else if ( !idStr::Icmp( statname, "item" ) || !idStr::Icmp( statname, "icon" ) || !idStr::Icmp( statname, "name" ) ) {
		// ignore these as they're handled elsewhere
		//These items should not be considered as succesful gives because it messes up the max ammo items
		return false;
	}
	return true;
}

/*
==============
idPlayer::idPlayer
==============
*/
idPlayer::idPlayer() {
	memset( &usercmd, 0, sizeof( usercmd ) );

	noclip					= false;
	godmode					= false;

	// boyette mod begin
	CaptainGui				= NULL;
	HailGui					= NULL;
	AIDialogueGui			= NULL;
	PlayerShip				= NULL;
	ShipOnBoard				= NULL;
	// boyette mod end

	// boyett list mod begin
	shipList			= NULL;

	SelectedEntityInSpace = NULL;

	notificationList_counter = 0;

	non_interactive_alert_message_counter = 0;
	last_non_interactive_alert_message_time = 0;

	currently_in_story_gui = false;

	allow_overlay_captain_gui = true;

	CaptainChairSeatedIn = NULL;
	ConsoleSeatedIn	=	NULL;

	disable_crosshairs = false;

	hud_map_visible = true;

	grabbed_reserve_crew_dict = NULL;
	selected_reserve_crew_dict = NULL;

	grabbed_crew_member = NULL;
	grabbed_crew_member_id = 0;
	confirm_removal_crew_member_id = 0;

	grabbed_module_exists = false;
	grabbed_module_id = 0;

	DialogueAI = NULL;
	currently_in_dialogue_with_ai = false;

	delay_selectedship_dialogue_branch_hostile_visible = false;

	tutorial_mode_on = false;
	tutorial_mode_current_step = 0;

	wait_impulse = -1;
	// boyette list mod end

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

	mountedObject			= NULL;
	playerHeadLight			= NULL;

	heartRate				= BASE_HEARTRATE;
	heartInfo.Init( 0, 0, 0, 0 );
	lastHeartAdjust			= 0;
	lastHeartBeat			= 0;
	lastDmgTime				= 0;
	deathClearContentsTime	= 0;
	lastArmorPulse			= -10000;
	stamina					= 0.0f;
	healthPool				= 0.0f;
	nextHealthPulse			= 0;
	healthPulse				= false;
	nextHealthTake			= 0;
	healthTake				= false;

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
	weapon_soulcube			= -1;
	weapon_pda				= -1;
	weapon_fists			= -1;
	weapon_bloodstone		= -1;
	weapon_bloodstone_active1 = -1;
	weapon_bloodstone_active2 = -1;
	weapon_bloodstone_active3 = -1;
	harvest_lock			= false;

	hudPowerup				= -1;
	lastHudPowerup			= -1;
	hudPowerupDuration		= 0;
	showWeaponViewModel		= true;

	skin					= NULL;
	powerUpSkin				= NULL;
	baseSkinName			= "";

	numProjectilesFired		= 0;
	numProjectileHits		= 0;

	airless					= false;
	airTics					= 0;
	lastAirDamage			= 0;

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
	talkCursor				= 0;
	focusVehicle			= NULL;
	cursor					= NULL;

	// boyette begin
	prevent_weapon_firing = false;
	// boyette end

	// boyette space command tablet begin
	tabletfocusEnt			= NULL;
	using_space_command_tablet = false;
	space_command_tablet_beam_active = false;
	player_just_transported = false;
	// boyette space command tablet end

	// BOYETTE MUSIC BEGIN
	music_shader_is_playing						= false;
	currently_playing_music_shader				= "";
	currently_playing_music_shader_begin_time	= 0;
	currently_playing_music_shader_end_time		= 0;
	// BOYETTE MUSIC END

	// BOYETTE WEIRD TEXTURE THRASHING FIX BEGIN
	thrash_fix_frame_begin = 0;
	thrash_fix_frames_to_wait = 0;
	before_thrash_fix_origin			= vec3_zero;
	before_thrash_fix_view_angles		= vec3_zero;
	currently_doing_weird_texture_thrash_fix = false;
	// BOYETTE WEIRD TEXTURE THRASHING FIX END

	// boyette space command headlight begin
	playerHeadLightOffset			= vec3_zero;
	playerHeadLightAngleOffset		= vec3_zero;
	// boyette space command headlight end
	
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
}

/*
==============
idPlayer::Init
==============
*/
void idPlayer::Init( void ) {
	const char			*value;
	const idKeyValue	*kv;

	noclip					= false;
	godmode					= false;

	// boyette mod begin
	guiOverlay			= NULL;		// - current overlay gui
	// boyette mod end

	oldButtons				= 0;
	oldFlags				= 0;

	currentWeapon			= -1;
	idealWeapon				= -1;
	previousWeapon			= -1;
	weaponSwitchTime		= 0;
	weaponEnabled			= true;
	weapon_soulcube			= SlotForWeapon( "weapon_soulcube" );
	weapon_pda				= SlotForWeapon( "weapon_pda" );
	weapon_fists			= SlotForWeapon( "weapon_space_command_fists" );
	weapon_bloodstone		= SlotForWeapon( "weapon_bloodstone_passive" );
	weapon_bloodstone_active1 = SlotForWeapon( "weapon_bloodstone_active1" );
	weapon_bloodstone_active2 = SlotForWeapon( "weapon_bloodstone_active2" );
	weapon_bloodstone_active3 = SlotForWeapon( "weapon_bloodstone_active3" );
	harvest_lock			= false;
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

	mountedObject			= NULL;
	if( playerHeadLight.IsValid() ) {
		playerHeadLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
	}
	playerHeadLight			= NULL;
	healthRecharge			= false;
	lastHealthRechargeTime	= 0;
	rechargeSpeed			= 500;
	new_g_damageScale		= 1.f;

	bloomEnabled			= false;
	bloomSpeed				= 1.f;
	bloomIntensity			= -0.01f;

	blurEnabled				= false;
	desaturateEnabled		= false;

	inventory.InitRechargeAmmo(this);
	hudPowerup				= -1;
	lastHudPowerup			= -1;
	hudPowerupDuration		= 0;

	currentLoggedAccel		= 0;

	focusTime				= 0;
	focusGUIent				= NULL;
	focusUI					= NULL;
	focusCharacter			= NULL;
	talkCursor				= 0;
	focusVehicle			= NULL;

	// boyette space command tablet begin
	tabletfocusEnt			= NULL;
	using_space_command_tablet = false;
	space_command_tablet_beam_active = false;
	// boyette space command tablet end

	// remove any damage effects
	playerView.ClearEffects();

	// damage values
	fl.takedamage			= true;
	ClearPain();

	// restore persistent data
	RestorePersistantInfo();

	bobCycle		= 0;
	stamina			= 0.0f;
	healthPool		= 0.0f;
	nextHealthPulse = 0;
	healthPulse		= false;
	nextHealthTake	= 0;
	healthTake		= false;

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

	// disable stamina on hell levels
	if ( gameLocal.world && gameLocal.world->spawnArgs.GetBool( "no_stamina" ) ) {
		pm_stamina.SetFloat( 0.0f );
	}

	// stamina always initialized to maximum
	stamina = pm_stamina.GetFloat();

	// air always initialized to maximum too
	airTics = pm_airTics.GetFloat();
	airless = false;

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

	value = spawnArgs.GetString( "model" );
	if ( value && ( *value != 0 ) ) {
		SetModel( value );
	}

	if ( cursor ) {
		cursor->SetStateInt( "talkcursor", 0 );
		cursor->SetStateString( "combatcursor", "1" );
		cursor->SetStateString( "itemcursor", "0" );
		cursor->SetStateString( "guicursor", "0" );
		cursor->SetStateString( "grabbercursor", "0" );
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

// boyette space command begin
	SetPlayerShipBasedOnShipSpawnArgs();
	ShipOnBoard = PlayerShip; // the game will bomb without a ShipOnBoard for some reason(only if there is a playership) - should be caught with static analysis.

	if ( PlayerShip ) {
		PlayerShip->current_currency_reserves += 500;
		PlayerShip->current_materials_reserves += 500;
		/* // BOYETTE NOTE: this happens on gameLocal now with: GetLocalPlayer()->PlayerShip->Event_GetATargetShipInSpace();
		if ( gameLocal.FindEntityUsingDef(NULL, "entity_sbship") ) {
			PlayerShip->SetTargetEntityInSpace( dynamic_cast<sbShip*>( gameLocal.FindEntityUsingDef(NULL, "entity_sbship") ) );
			SelectedEntityInSpace = PlayerShip->TargetEntityInSpace;
			if ( PlayerShip->TargetEntityInSpace && (PlayerShip->TargetEntityInSpace->stargridpositionx != PlayerShip->stargridpositionx || PlayerShip->TargetEntityInSpace->stargridpositiony != PlayerShip->stargridpositiony) ) {
				PlayerShip->TargetEntityInSpace	= NULL;
				SelectedEntityInSpace = NULL;
			}
			if ( PlayerShip->TargetEntityInSpace == PlayerShip ) {
				PlayerShip->TargetEntityInSpace	= NULL;
				SelectedEntityInSpace = NULL;
			}
		} else {
			PlayerShip->TargetEntityInSpace	= NULL;
		}
		*/
		PlayerShip->TargetEntityInSpace	= NULL;
		SelectedEntityInSpace = NULL;

		//stargrid_positions_visited.push_back( idVec2(PlayerShip->stargridpositionx,PlayerShip->stargridpositiony) );
	}
// boyette space command end

	// boyette mod begin
	CaptainGui = uiManager->FindGui("guis/CaptainDisplay.gui", true, false, true );
	CaptainGui->SetStateBool( "gameDraw", true );
	HailGui = uiManager->FindGui("guis/steve_space_command/hail_guis/communications_were_not_successful_notice.gui", true, false, true );
	HailGui->SetStateBool( "gameDraw", true );
	AIDialogueGui = uiManager->FindGui("guis/steve_space_command/ai_dialogue_guis/you_cannot_speak_to_this_ai.gui", true, false, true );
	AIDialogueGui->SetStateBool( "gameDraw", true );
	// boyette mod end

	// boyette CaptainGui initialization list begin
	CaptainGui->SetStateString( "captain_target", "steve" );
	CaptainGui->SetStateInt( "captain_number", 0 );
	// boyette CaptainGui initialization list end

	// boyette list mod begin
	if ( PlayerShip ) {
		PopulateShipList();
		UpdateStarGridShipPositions();
	}
	else{
		gameLocal.Warning( "There is no PlayerShip entity. The ICARUS gui lists will not work." );
	}
	// boyette list mod end

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
	}

	SetLastHitTime( 0 );

	// load the armor sound feedback
	declManager->FindSound( "source_default_player_sounds_hit_armor_01" );

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
		if ( weapon_soulcube > 0 && ( inventory.weapons.test(weapon_soulcube) ) ) {
			int max_souls = inventory.MaxAmmoForAmmoClass( this, "ammo_souls" );
			if ( inventory.ammo[ idWeapon::GetAmmoNumForName( "ammo_souls" ) ] >= max_souls ) {
				hud->HandleNamedEvent( "soulCubeReady" );
			}
		}
		//We can spawn with a full bloodstone, so make sure the hud knows
		if ( weapon_bloodstone > 0 && ( inventory.weapons.test(weapon_bloodstone) ) ) {
			//int max_blood = inventory.MaxAmmoForAmmoClass( this, "ammo_bloodstone" );
			//if ( inventory.ammo[ idWeapon::GetAmmoNumForName( "ammo_bloodstone" ) ] >= max_blood ) {
				hud->HandleNamedEvent( "bloodstoneReady" );
			//}
		}
		hud->HandleNamedEvent( "itemPickup" );
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
		if ( g_skill.GetInteger() < 2 ) {
			if ( health < 25 ) {
				health = 25;
			}
			if ( g_useDynamicProtection.GetBool() ) {
				new_g_damageScale = 1.0f;
			}
		} else {
			new_g_damageScale = 1.0f;
			g_armorProtection.SetFloat( ( g_skill.GetInteger() < 2 ) ? 0.4f : 0.2f );
#ifndef ID_DEMO_BUILD
			if ( g_skill.GetInteger() == 3 ) {
				healthTake = true;
				nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
			}
#endif
		}
	}

	//Setup the weapon toggle lists
	const idKeyValue *kv;
	kv = spawnArgs.MatchPrefix( "weapontoggle", NULL );
	while( kv ) {
		WeaponToggle_t newToggle;
		strcpy(newToggle.name, kv->GetKey().c_str());

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

	bloomEnabled			= false;
	bloomSpeed				= 1;
	bloomIntensity			= -0.01f;

	blurEnabled				= false;
	desaturateEnabled		= false;

	UpdateCaptainMenu();
	UpdateSelectedEntityInSpaceOnGuis();
	UpdateWeaponsAndTorpedosQueuesOnCaptainGui(); // maybe put this inside UpdateCaptainMenu() some day.
	UpdateModulesPowerQueueOnCaptainGui(); // maybe put this inside UpdateCaptainMenu() some day.
// boyette slow motion and warp effects reset begin
	//DetermineTimeGroup( spawnArgs.GetBool( "slowmo", "1" ) );
	//DetermineTimeGroup( true );
	g_enableSlowmo.SetBool( false );
	DetermineTimeGroup( true ); // this seems to solve all our gui time problems
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->DetermineTimeGroup( true );
	}	
	playerView.FreeWarp(0);
	//playerView.Fade(idVec4(0,0,0,0),100); // This is not necessary - it gets reset correctly somwhere esle already. And seems to screw up the view anyways - it just ends up black. Maybe because the playerView object is not ready at this point.
	//weapon.GetEntity()->DetermineTimeGroup( false );
	//DetermineTimeGroup( false );
	//g_enableSlowmo.SetBool(false); // need to either make a seperate cvar or set this to false when the gamelocal spawns. Because this doesn't get reset currently when you reload the level.
	currently_in_dialogue_with_ai = false;
// boyette slow motion and warp effects reset end

	// BOYETTE SPACE COMMAND BEGIN - if there is no focus character, don't show the talkcursor info on spawn
	if ( !focusCharacter && hud ) {
		hud->SetStateString( "npc", "" );
		hud->SetStateString( "npc_action", "" );
		hud->HandleNamedEvent( "hideNPC" );
	}

	playerView.Fade(idVec4(0,0,0,0),10000);
	// BOYETTE SPACE COMMAND END
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

	// BOYETTE SPACE COMMAND BEGIN - these became necessary here after was changed the way the in-game menus work.
	CloseOverlayGui();
	CloseOverlayCaptainGui();
	CloseOverlayHailGui();
	CloseOverlayAIDialogeGui();
	// BOYETTE SPACE COMMAND END

#ifdef CTF
	if ( playerHeadLight.IsValid() ) {
		playerHeadLight.GetEntity()->ProcessEvent( &EV_Remove );
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

	savefile->WriteInt( weapon_soulcube );
	savefile->WriteInt( weapon_pda );
	savefile->WriteInt( weapon_fists );
	savefile->WriteInt( weapon_bloodstone );
	savefile->WriteInt( weapon_bloodstone_active1 );
	savefile->WriteInt( weapon_bloodstone_active2 );
	savefile->WriteInt( weapon_bloodstone_active3 );
	savefile->WriteBool( harvest_lock );
	savefile->WriteInt( hudPowerup );
	savefile->WriteInt( lastHudPowerup );
	savefile->WriteInt( hudPowerupDuration );
	
	

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
	savefile->WriteFloat( stamina );
	savefile->WriteFloat( healthPool );
	savefile->WriteInt( nextHealthPulse );
	savefile->WriteBool( healthPulse );
	savefile->WriteInt( nextHealthTake );
	savefile->WriteBool( healthTake );

	savefile->WriteBool( hiddenWeapon );
	soulCubeProjectile.Save( savefile );

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
	savefile->WriteBool( showWeaponViewModel );

	savefile->WriteSkin( skin );
	savefile->WriteSkin( powerUpSkin );
	savefile->WriteString( baseSkinName );

	savefile->WriteInt( numProjectilesFired );
	savefile->WriteInt( numProjectileHits );

	savefile->WriteBool( airless );
	savefile->WriteInt( airTics );
	savefile->WriteInt( lastAirDamage );

	savefile->WriteBool( gibDeath );
	savefile->WriteBool( gibsLaunched );
	savefile->WriteVec3( gibsDir );

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
	savefile->WriteUserInterface( cursor, false );

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

	savefile->WriteFloat( pm_stamina.GetFloat() );

	if ( hud ) {
		hud->SetStateString( "message", common->GetLanguageDict()->GetString( "#str_02916" ) );
		hud->HandleNamedEvent( "Message" );
	}

	savefile->WriteInt(weaponToggles.Num());
	for(i = 0; i < weaponToggles.Num(); i++) {
		WeaponToggle_t* weaponToggle = weaponToggles.GetIndex(i);
		savefile->WriteString(weaponToggle->name);
		savefile->WriteInt(weaponToggle->toggleList.Num());
		for(int j = 0; j < weaponToggle->toggleList.Num(); j++) {
			savefile->WriteInt(weaponToggle->toggleList[j]);
		}
	}
	savefile->WriteObject( mountedObject );
	playerHeadLight.Save( savefile );
	savefile->WriteBool( healthRecharge );
	savefile->WriteInt( lastHealthRechargeTime );
	savefile->WriteInt( rechargeSpeed );
	savefile->WriteFloat( new_g_damageScale );

	savefile->WriteBool( bloomEnabled );
	savefile->WriteFloat( bloomSpeed );
	savefile->WriteFloat( bloomIntensity );

	// BOYETTE SAVE BEGIN
	savefile->WriteUserInterface( CaptainGui, false );
	savefile->WriteUserInterface( HailGui, false );
	savefile->WriteUserInterface( AIDialogueGui, false );

	if ( guiOverlay == CaptainGui ) {
		savefile->WriteInt(GUIOVERLAYISCAPTAINGUI);
	} else if ( guiOverlay == HailGui ) {
		savefile->WriteInt(GUIOVERLAYISHAILGUI);
	} else if ( guiOverlay == AIDialogueGui ) {
		savefile->WriteInt(GUIOVERLAYISAIDIALOGUEGUI);
	} else {
		savefile->WriteInt(GUIOVERLAYISOTHERGUI);
		savefile->WriteUserInterface( guiOverlay, false );
	}

	savefile->WriteObject( PlayerShip );
	savefile->WriteObject( ShipOnBoard );

	//shipList	= NULL;

	savefile->WriteObject( SelectedEntityInSpace );

	savefile->WriteInt( notificationList_counter );

	savefile->WriteInt( non_interactive_alert_message_counter );
	savefile->WriteInt( last_non_interactive_alert_message_time );

	savefile->WriteBool( currently_in_story_gui );

	savefile->WriteBool( allow_overlay_captain_gui );

	savefile->WriteObject( CaptainChairSeatedIn );
	savefile->WriteObject( ConsoleSeatedIn );

	savefile->WriteBool( disable_crosshairs );

	savefile->WriteBool( hud_map_visible );

	savefile->WriteDict( grabbed_reserve_crew_dict );
	savefile->WriteDict( selected_reserve_crew_dict );

	savefile->WriteObject( grabbed_crew_member );
	savefile->WriteInt( grabbed_crew_member_id );
	savefile->WriteInt( confirm_removal_crew_member_id );

	savefile->WriteBool( grabbed_module_exists );
	savefile->WriteInt( grabbed_module_id );

	savefile->WriteObject( DialogueAI );
	savefile->WriteBool( currently_in_dialogue_with_ai );

	savefile->WriteBool( delay_selectedship_dialogue_branch_hostile_visible );

	savefile->WriteInt( stargrid_positions_visited.size() );
	for ( int i = 0; i < stargrid_positions_visited.size(); i++ ) {
		savefile->WriteVec2( stargrid_positions_visited[i] );
	}

	savefile->WriteBool( tutorial_mode_on );
	savefile->WriteInt( tutorial_mode_current_step );

	savefile->WriteInt( wait_impulse );

	savefile->WriteBool( prevent_weapon_firing );

	tabletfocusEnt.Save( savefile );
	savefile->WriteBool( using_space_command_tablet );

	if ( SpaceCommandTabletBeam.target.GetEntity() && SpaceCommandTabletBeam.modelDefHandle >= 0 ) {
		savefile->WriteBool( true );
		SpaceCommandTabletBeam.target.Save( savefile );
		savefile->WriteRenderEntity( SpaceCommandTabletBeam.renderEntity );
	} else {
		savefile->WriteBool( false );
	}
	savefile->WriteBool( space_command_tablet_beam_active );

	savefile->WriteBool( player_just_transported );

	savefile->WriteBool( music_shader_is_playing );
	savefile->WriteString( currently_playing_music_shader );
	savefile->WriteInt( currently_playing_music_shader_begin_time );
	savefile->WriteInt( currently_playing_music_shader_end_time );

	savefile->WriteInt( thrash_fix_frame_begin );
	savefile->WriteInt( thrash_fix_frames_to_wait );
	savefile->WriteVec3( before_thrash_fix_origin );
	savefile->WriteVec3( before_thrash_fix_view_angles );
	savefile->WriteBool( currently_doing_weird_texture_thrash_fix );

	savefile->WriteVec3( playerHeadLightOffset );
	savefile->WriteVec3( playerHeadLightAngleOffset );

	savefile->WriteBool( blurEnabled );
	savefile->WriteBool( desaturateEnabled );

	savefile->WriteBool( g_inIronManMode.GetBool() );
	// BOYETTE SAVE END
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

	savefile->ReadInt( weapon_soulcube );
	savefile->ReadInt( weapon_pda );
	savefile->ReadInt( weapon_fists );
	savefile->ReadInt( weapon_bloodstone );
	savefile->ReadInt( weapon_bloodstone_active1 );
	savefile->ReadInt( weapon_bloodstone_active2 );
	savefile->ReadInt( weapon_bloodstone_active3 );

	savefile->ReadBool( harvest_lock );
	savefile->ReadInt( hudPowerup );
	savefile->ReadInt( lastHudPowerup );
	savefile->ReadInt( hudPowerupDuration );

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
	savefile->ReadFloat( stamina );
	savefile->ReadFloat( healthPool );
	savefile->ReadInt( nextHealthPulse );
	savefile->ReadBool( healthPulse );
	savefile->ReadInt( nextHealthTake );
	savefile->ReadBool( healthTake );

	savefile->ReadBool( hiddenWeapon );
	soulCubeProjectile.Restore( savefile );

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
	savefile->ReadBool( showWeaponViewModel );

	savefile->ReadSkin( skin );
	savefile->ReadSkin( powerUpSkin );
	savefile->ReadString( baseSkinName );

	savefile->ReadInt( numProjectilesFired );
	savefile->ReadInt( numProjectileHits );

	savefile->ReadBool( airless );
	savefile->ReadInt( airTics );
	savefile->ReadInt( lastAirDamage );

	savefile->ReadBool( gibDeath );
	savefile->ReadBool( gibsLaunched );
	savefile->ReadVec3( gibsDir );

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
	savefile->ReadUserInterface( cursor );

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

	savefile->ReadFloat( set );
	pm_stamina.SetFloat( set );

	// create combat collision hull for exact collision detection
	SetCombatModel();

	int weaponToggleCount;
	savefile->ReadInt(weaponToggleCount);
	for(i = 0; i < weaponToggleCount; i++) {
		WeaponToggle_t newToggle;
		memset(&newToggle, 0, sizeof(newToggle));

		idStr name;
		savefile->ReadString(name);
		strcpy(newToggle.name, name.c_str());

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
	playerHeadLight.Restore( savefile );
	savefile->ReadBool( healthRecharge );
	savefile->ReadInt( lastHealthRechargeTime );
	savefile->ReadInt( rechargeSpeed );
	savefile->ReadFloat( new_g_damageScale );

	savefile->ReadBool( bloomEnabled );
	savefile->ReadFloat( bloomSpeed );
	savefile->ReadFloat( bloomIntensity );

	// BOYETTE RESTORE BEGIN
	bool hasObject;

	savefile->ReadUserInterface( CaptainGui );
	savefile->ReadUserInterface( HailGui );
	savefile->ReadUserInterface( AIDialogueGui );

	savefile->ReadInt( num );
	if ( num == GUIOVERLAYISCAPTAINGUI ) {
		guiOverlay = CaptainGui;
	} else if ( num == GUIOVERLAYISHAILGUI ) {
		guiOverlay = HailGui;
	} else if ( num == GUIOVERLAYISAIDIALOGUEGUI ) {
		guiOverlay = AIDialogueGui;
	} else if ( num == GUIOVERLAYISOTHERGUI ) {
		savefile->ReadUserInterface( guiOverlay );
	}
	//savefile->ReadUserInterface( guiOverlay );

	savefile->ReadObject( reinterpret_cast<idClass *&>( PlayerShip ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( ShipOnBoard ) );

	//shipList	= NULL;

	savefile->ReadObject( reinterpret_cast<idClass *&>( SelectedEntityInSpace ) );

	savefile->ReadInt( notificationList_counter );

	savefile->ReadInt( non_interactive_alert_message_counter );
	savefile->ReadInt( last_non_interactive_alert_message_time );

	savefile->ReadBool( currently_in_story_gui );

	savefile->ReadBool( allow_overlay_captain_gui );

	savefile->ReadObject( reinterpret_cast<idClass *&>( CaptainChairSeatedIn ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( ConsoleSeatedIn ) );

	savefile->ReadBool( disable_crosshairs );

	savefile->ReadBool( hud_map_visible );

	savefile->ReadDict( grabbed_reserve_crew_dict );
	savefile->ReadDict( selected_reserve_crew_dict );

	savefile->ReadObject( reinterpret_cast<idClass *&>( grabbed_crew_member ) );
	savefile->ReadInt( grabbed_crew_member_id );
	savefile->ReadInt( confirm_removal_crew_member_id );

	savefile->ReadBool( grabbed_module_exists );
	savefile->ReadInt( grabbed_module_id );

	savefile->ReadObject( reinterpret_cast<idClass *&>( DialogueAI ) );
	savefile->ReadBool( currently_in_dialogue_with_ai );

	savefile->ReadBool( delay_selectedship_dialogue_branch_hostile_visible );

	savefile->ReadInt( num );
	stargrid_positions_visited.resize(num);
	for ( int i = 0; i < stargrid_positions_visited.size(); i++ ) {
		savefile->ReadVec2( stargrid_positions_visited[i] );
	}

	savefile->ReadBool( tutorial_mode_on );
	savefile->ReadInt( tutorial_mode_current_step );

	savefile->ReadInt( wait_impulse );

	savefile->ReadBool( prevent_weapon_firing );

	tabletfocusEnt.Restore( savefile );
	savefile->ReadBool( using_space_command_tablet );

	savefile->ReadBool( hasObject );
	if ( hasObject ) {
		SpaceCommandTabletBeam.target.Restore( savefile );
		savefile->ReadRenderEntity( SpaceCommandTabletBeam.renderEntity );
		SpaceCommandTabletBeam.modelDefHandle = gameRenderWorld->AddEntityDef( &SpaceCommandTabletBeam.renderEntity );
	} else {
		SpaceCommandTabletBeam.target = NULL;
		memset( &SpaceCommandTabletBeam.renderEntity, 0, sizeof( renderEntity_t ) );
		SpaceCommandTabletBeam.modelDefHandle = -1;
	}
	savefile->ReadBool( space_command_tablet_beam_active );

	savefile->ReadBool( player_just_transported );

	savefile->ReadBool( music_shader_is_playing );
	savefile->ReadString( currently_playing_music_shader );
	savefile->ReadInt( currently_playing_music_shader_begin_time );
	savefile->ReadInt( currently_playing_music_shader_end_time );

	savefile->ReadInt( thrash_fix_frame_begin );
	savefile->ReadInt( thrash_fix_frames_to_wait );
	savefile->ReadVec3( before_thrash_fix_origin );
	savefile->ReadVec3( before_thrash_fix_view_angles );
	savefile->ReadBool( currently_doing_weird_texture_thrash_fix );

	savefile->ReadVec3( playerHeadLightOffset );
	savefile->ReadVec3( playerHeadLightAngleOffset );

	savefile->ReadBool( blurEnabled );
	savefile->ReadBool( desaturateEnabled );

	bool restore_bool = false;
	savefile->ReadBool( restore_bool );
	g_inIronManMode.SetBool(restore_bool);
	// BOYETTE RESTORE END
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
	idStr skin;

	spot = gameLocal.SelectInitialSpawnPoint( this );

	// set the player skin from the spawn location
	if ( spot->spawnArgs.GetString( "skin", NULL, skin ) ) {
		spawnArgs.Set( "spawn_skin", skin );
	}

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
	// put the cvar here
	if ( g_enableWeirdTextureThrashingFix.GetBool() && !cvarSystem->GetCVarInteger( "image_downSize" ) ) { // made this always false instead of the cvar on 07 05 2016 because it was causing memory crashes every once in a while //if ( false ) { //
		currently_doing_weird_texture_thrash_fix = true;
		SpawnToPoint( idVec3(-177777,0,0), spawn_angles );
		StartWeirdTextureThrashFix( spawn_origin, 30 );
	} else {
	//PostEventMS( &EV_Player_UpdateWeirdTextureThrashFix, 100, spawn_origin, 15 );
	// put the cvar end here
		SpawnToPoint( spawn_origin, spawn_angles );
	}
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
===============
*/
void idPlayer::SavePersistantInfo( void ) {
	idDict &playerInfo = gameLocal.persistentPlayerInfo[entityNumber];

	playerInfo.Clear();
	inventory.GetPersistantData( playerInfo );
	playerInfo.SetInt( "health", health );
	playerInfo.SetInt( "current_weapon", currentWeapon );
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
		idealWeapon = spawnArgs.GetInt( "current_weapon", "1" );
	}
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
			baseSkinName = "skins/skins_used_in_source/player/officer_mp_blue";
		} else {
			baseSkinName = "skins/skins_used_in_source/player/officer_mp_red";
		}
		if ( !gameLocal.isClient && team != latchedTeam ) {
			gameLocal.mpGame.SwitchToTeam( entityNumber, latchedTeam, team );
		}
		latchedTeam = team;
	} else {
		baseSkinName = GetUserInfo()->GetString( "ui_skin" );
	}
	if ( !baseSkinName.Length() ) {
		baseSkinName = "skins/skins_used_in_source/player/officer_mp";
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
	else if ( PowerUpActive( INVULNERABILITY ) ) {
		powerUpSkin = declManager->FindSkin( baseSkinName + "_invuln" );
	//} else if ( PowerUpActive( HASTE ) ) {
	//	powerUpSkin = declManager->FindSkin( baseSkinName + "_haste" );
	}
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

	assert( weapon.GetEntity() );
	assert( _hud );

	inclip		= weapon.GetEntity()->AmmoInClip();
	ammoamount	= weapon.GetEntity()->AmmoAvailable();

	//Hack to stop the bloodstone ammo to display when it is being activated
	if ( ammoamount < 0 || !weapon.GetEntity()->IsReady() || currentWeapon == weapon_bloodstone) {
		// show infinite ammo
		_hud->SetStateString( "player_ammo", "" );
		_hud->SetStateString( "player_totalammo", "" );
	} else { 
		// show remaining ammo
		_hud->SetStateString( "player_totalammo", va( "%i", ammoamount ) );
		_hud->SetStateString( "player_ammo", weapon.GetEntity()->ClipSize() ? va( "%i", inclip ) : "--" );		// how much in the current clip
		_hud->SetStateString( "player_clips", weapon.GetEntity()->ClipSize() ? va( "%i", ammoamount / weapon.GetEntity()->ClipSize() ) : "--" );

		_hud->SetStateString( "player_allammo", va( "%i/%i", inclip, ammoamount ) );
	} 

	_hud->SetStateBool( "player_ammo_empty", ( ammoamount == 0 ) );
	_hud->SetStateBool( "player_clip_empty", ( weapon.GetEntity()->ClipSize() ? inclip == 0 : false ) );
	_hud->SetStateBool( "player_clip_low", ( weapon.GetEntity()->ClipSize() ? inclip <= weapon.GetEntity()->LowAmmo() : false ) );

	//Hack to stop the bloodstone ammo to display when it is being activated
	if(currentWeapon == weapon_bloodstone) {
		_hud->SetStateBool( "player_ammo_empty", false );
		_hud->SetStateBool( "player_clip_empty", false );
		_hud->SetStateBool( "player_clip_low", false );
	}

	//Let the HUD know the total amount of ammo regardless of the ammo required value
	_hud->SetStateString( "player_ammo_count", va("%i", weapon.GetEntity()->AmmoCount()));

	//Make sure the hud always knows how many bloodstone charges there are
	int ammoRequired;
	ammo_t ammo_i = inventory.AmmoIndexForWeaponClass( "weapon_bloodstone_passive", &ammoRequired );
	int bloodstoneAmmo = inventory.HasAmmo( ammo_i, ammoRequired );
	_hud->SetStateString("player_bloodstone_ammo", va("%i", bloodstoneAmmo));
	_hud->HandleNamedEvent( "bloodstoneAmmoUpdate" );

	_hud->HandleNamedEvent( "updateAmmo" );
}

/*
===============
idPlayer::UpdateHudStats
===============
*/
void idPlayer::UpdateHudStats( idUserInterface *_hud ) {
	int staminapercentage;
	float max_stamina;

	assert( _hud );

	max_stamina = pm_stamina.GetFloat();
	if ( !max_stamina ) {
		// stamina disabled, so show full stamina bar
		staminapercentage = 100.0f;
	} else {
		staminapercentage = idMath::FtoiFast( 100.0f * stamina / max_stamina );
	}

	_hud->SetStateInt( "player_health", health );
	_hud->SetStateInt( "player_stamina", staminapercentage );
	_hud->SetStateInt( "player_armor", inventory.armor );
	_hud->SetStateInt( "player_hr", heartRate );
	
	_hud->SetStateInt( "player_nostamina", ( max_stamina == 0 ) ? 1 : 0 );

	_hud->HandleNamedEvent( "updateArmorHealthAir" );

	_hud->HandleNamedEvent( "updatePowerup" );

	// Boyette space command begin
	// boyette mod update very frame
	//UpdateCaptainMenuEveryFrame(); // NOT NECESSARY BECAUSE IT ALREADY GETS CALLED CONSTANTLY ON Think();
	// Boyette space command end

	if ( healthPulse ) {
		_hud->HandleNamedEvent( "healthPulse" );
		StartSound( "snd_healthpulse", SND_CHANNEL_ITEM, 0, false, NULL );
		healthPulse = false;
	}

	if ( healthTake ) {
		_hud->HandleNamedEvent( "healthPulse" );
		StartSound( "snd_healthtake", SND_CHANNEL_ITEM, 0, false, NULL );
		healthTake = false;
	}

	if ( inventory.ammoPulse ) { 
		_hud->HandleNamedEvent( "ammoPulse" );
		inventory.ammoPulse = false;
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
    
    _hud->HandleNamedEvent( "selfTeam" );
    
#endif
    

	UpdateHudAmmo( _hud );
}

/*
===============
idPlayer::UpdateHudWeapon
===============
*/
void idPlayer::UpdateHudWeapon( bool flashWeapon ) {
	idUserInterface *hud = idPlayer::hud;

// boyette weapon hud icon begin
	if ( hud ) {
		idStr weaponIcon;
		weaponIcon = weapon.GetEntity()->spawnArgs.GetString( "hud_weapon_icon", "textures/images_used_in_source/default_weapon_hud_icon.tga" );
		hud->SetStateString("current_weapon_icon",weaponIcon);

		idStr weaponName;
		weaponName = weapon.GetEntity()->spawnArgs.GetString( "inv_name", "Unknown Weapon" );
		hud->SetStateString("current_weapon_name",weaponName);

		bool showClipAmmoAmounts;
		showClipAmmoAmounts = weapon.GetEntity()->spawnArgs.GetBool( "hud_weapon_show_clip_ammo_amounts", "1" );
		hud->SetStateBool("weapon_show_clip_ammo_amounts",showClipAmmoAmounts);

		bool showTotalAmmoAmounts;
		showTotalAmmoAmounts = weapon.GetEntity()->spawnArgs.GetBool( "hud_weapon_show_total_ammo_amounts", "1" );
		hud->SetStateBool("weapon_show_total_ammo_amounts",showTotalAmmoAmounts);
	}
// boyette weapon hud icon end

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

	for ( int i = 0; i < MAX_WEAPONS; i++ ) {
		const char *weapnum = va( "def_weapon%d", i );
		const char *hudWeap = va( "weapon%d", i );
		int weapstate = 0;
		if ( inventory.weapons.test(i) ) {
			const char *weap = spawnArgs.GetString( weapnum );
			if ( weap && *weap ) {
				weapstate++;
			}
			if ( idealWeapon == i ) {
				weapstate++;
			}
		}
		hud->SetStateInt( hudWeap, weapstate );
	}
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
	idClipModel *clipModelList[ MAX_GENTITIES ];
	idClipModel *clip;
	int			listedClipModels;
	idEntity	*oldFocus;
	idEntity	*ent;
	idUserInterface *oldUI;
	int			i, j;
	idVec3		start, end;
	const char *command;
	trace_t		trace;
	guiPoint_t	pt;
	const idKeyValue *kv;
	sysEvent_t	ev;
	idUserInterface *ui;
	/*
	if ( guiOverlay ) {
		guiOverlay->Redraw(gameLocal.time);
			//gameLocal.Printf( "gameLocal.fast.time is " + idStr(gameLocal.fast.time) + ".\n" );
			//gameLocal.Printf( "gameLocal.slow.time is " + idStr(gameLocal.slow.time) + ".\n" );
			//gameLocal.Printf( "gameLocal.time is " + idStr(gameLocal.time) + ".\n" );
			//gameLocal.Printf( "guiOverlay->GetTime() is " + idStr(guiOverlay->GetTime()) + ".\n" );
		//sysEvent_t ev; // I don't think this is necessary
		//const char *command; // I don't think this is necessary
		//command = guiOverlay->HandleEvent( &ev, gameLocal.time ); // I don't think this is necessary
		focusUI = guiOverlay;
		RouteGuiMouse( guiOverlay );
 		//HandleGuiCommands( focusGUIent, command ); // I don't think this is necessary
//		guiOverlay->Activate( true, gameLocal.time ); // screws it up
		//RouteGuiMouseWheel( guiOverlay );
	

		if ( gameLocal.inCinematic ) {
			return;
		}

		oldFocus		= focusGUIent;
		oldUI			= focusUI;

		if ( focusTime <= gameLocal.time ) {
	// boyette this fixed it //	ClearFocus();
		}

		// don't let spectators interact with GUIs
		if ( spectating ) {
			return;
		}

		start = GetEyePosition();

		//HH rww - the actual viewAngles do not seem to be properly transformed to at this point,
		//so just get a transformed direction now.
	//	idVec3 viewDir = (untransformedViewAngles.ToMat3()*GetEyeAxis())[0];

	//	end = start + viewDir * 70.0f;	// was 50, doom=80, prey=70

		idBounds bounds( start );
		bounds.AddPoint( end );

		// no pretense at sorting here, just assume that there will only be one active
		// gui within range along the trace
		if ( guiOverlay ) {
			
	//		focusGUIent = ent;
			// boyette mod

			ui = guiOverlay;

			focusUI = ui;

			// boyette mod;

		}
		if ( focusGUIent && focusUI ) {
			if ( !oldFocus || oldFocus != focusGUIent ) {
				command = focusUI->Activate( true, gameLocal.time );
				HandleGuiCommands( focusGUIent, command );
				//StartSound( "snd_guienter", SND_CHANNEL_ANY, 0, false, NULL );
			}
		} else if ( oldFocus && oldUI ) {
			command = oldUI->Activate( false, gameLocal.time );
			HandleGuiCommands( oldFocus, command );
			//StartSound( "snd_guiexit", SND_CHANNEL_ANY, 0, false, NULL );
		}

		return;
	}
	*/
	if ( guiOverlay ) {
		//HandleGuiCommands( guiOverlay, 
		return;
	}
	// HH pdm: removed weapon ptr check here since ours is invalid when in a vehicle
	// Also removed privateCameraView, since we want subtitles when they are on
	if ( gameLocal.GetCamera() || !_hud || !g_showHud.GetBool() || pm_thirdPerson.GetBool() ) {
		return;
	}

	UpdateHudStats( _hud );


	if(weapon.IsValid()) { // HH
		bool allowGuiUpdate = true;
		//rww - update the weapon gui only if the owner is being spectated by this client, or is this client
		if ( gameLocal.localClientNum != entityNumber ) {
			// if updating the hud for a followed client
			if ( gameLocal.localClientNum >= 0 && gameLocal.entities[ gameLocal.localClientNum ] && gameLocal.entities[ gameLocal.localClientNum ]->IsType( idPlayer::Type ) ) {
				idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ gameLocal.localClientNum ] );
				if ( !p->spectating || p->spectator != entityNumber ) {
					allowGuiUpdate = false;
				}
			} else {
				allowGuiUpdate = false;
			}
		}

		if (allowGuiUpdate) {
//			weapon->UpdateGUI();
		}
	}

	_hud->SetStateInt( "s_debug", cvarSystem->GetCVarInteger( "s_showLevelMeter" ) );

	_hud->Redraw( gameLocal.realClientTime );

	// weapon targeting crosshair
	if ( !GuiActive() ) {
		if ( cursor && weapon.GetEntity()->ShowCrosshair() ) {

			if ( disable_crosshairs ) {
				cursor->SetStateString( "grabbercursor", "0" );
				cursor->SetStateString( "combatcursor", "0" );
			} else {
				// original
				if ( weapon.GetEntity()->GetGrabberState() == 1 || weapon.GetEntity()->GetGrabberState() == 2 ) {
					cursor->SetStateString( "grabbercursor", "1" );
					cursor->SetStateString( "combatcursor", "0" );
				} else {
					cursor->SetStateString( "grabbercursor", "0" );
					cursor->SetStateString( "combatcursor", "1" );
				}
			}

			cursor->Redraw( gameLocal.realClientTime );
		}
	}
}

/*
===============
idPlayer::EnterCinematic
===============
*/
void idPlayer::EnterCinematic( void ) {
	if ( PowerUpActive( HELLTIME ) ) {
		StopHelltime();
	}

	Hide();
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
}

/*
===============
idPlayer::ExitCinematic
===============
*/
void idPlayer::ExitCinematic( void ) {
	Show();

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
	float	fallspeed;
	float	forwardspeed;
	float	sidespeed;

	// minus the push velocity to avoid playing the walking animation and sounds when riding a mover
	velocity = physicsObj.GetLinearVelocity() - physicsObj.GetPushedLinearVelocity();
	fallspeed = velocity * physicsObj.GetGravityNormal();

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

	AI_RUN			= ( usercmd.buttons & BUTTON_RUN ) && ( ( !pm_stamina.GetFloat() ) || ( stamina > pm_staminathreshold.GetFloat() ) );
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
	}
}

/*
===============
idPlayer::FireWeapon
===============
*/
void idPlayer::FireWeapon( void ) {
	if ( !prevent_weapon_firing ) {
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
			if ( weapon.GetEntity()->AmmoInClip() || weapon.GetEntity()->AmmoAvailable() ) {
				AI_ATTACK_HELD = true;
				weapon.GetEntity()->BeginAttack();
				if ( ( weapon_soulcube >= 0 ) && ( currentWeapon == weapon_soulcube ) ) {
					if ( hud ) {
						hud->HandleNamedEvent( "soulCubeNotReady" );
					}
					SelectWeapon( previousWeapon, false );
				}
				if( (weapon_bloodstone >= 0) && (currentWeapon == weapon_bloodstone) && inventory.weapons.test(weapon_bloodstone_active1) && weapon.GetEntity()->GetStatus() == WP_READY) {
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
			} else {
				NextBestWeapon();
			}
		}

		if ( hud ) {
			if ( tipUp ) {
				HideTip();
			}
			// may want to track with with a bool as well
			// keep from looking up named events so often
			if ( objectiveUp ) {
				HideObjective();
			}
		}
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
	if ( inventory.weapons.count() == 0 ) {
		return;
	}
	
	for( w = 0; w < MAX_WEAPONS; w++ ) {
		if ( inventory.weapons.test(w) ) {
			weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
			if ( weap != "" ) {
				idWeapon::CacheWeapon( weap );
			} else {
				inventory.weapons.reset(w);
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

	} else if ( !idStr::Icmp( statname, "stamina" ) ) {
		if ( stamina >= 100 ) {
			return false;
		}
		stamina += atof( value );
		if ( stamina > 100 ) {
			stamina = 100;
		}

	} else if ( !idStr::Icmp( statname, "heartRate" ) ) {
		heartRate += atoi( value );
		if ( heartRate > MAX_HEARTRATE ) {
			heartRate = MAX_HEARTRATE;
		}

	} else if ( !idStr::Icmp( statname, "air" ) ) {
		if ( airTics >= pm_airTics.GetInteger() ) {
			return false;
		}
		airTics += atoi( value ) / 100.0 * pm_airTics.GetInteger();
		if ( airTics > pm_airTics.GetInteger() ) {
			airTics = pm_airTics.GetInteger();
		}
	} else if ( !idStr::Icmp( statname, "enviroTime" ) ) {
		if ( PowerUpActive( ENVIROTIME ) ) {
			inventory.powerupEndTime[ ENVIROTIME ] += (atof(value) * 1000);
		} else {
			GivePowerUp( ENVIROTIME, atoi(value)*1000 );
		}
// BOYETTE SPACE COMMAND BEGIN
	} else if ( !idStr::Icmp( statname, "currency" ) ) {
		// nothing as of right now - this is just to make sure this function returns true
	} else if ( !idStr::Icmp( statname, "materials" ) ) {
		// nothing as of right now - this is just to make sure this function returns true
// BOYETTE SPACE COMMAND END
	} else {
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

	if ( gameLocal.isMultiplayer && spectating ) {
		return false;
	}

	item->GetAttributes( attr );
	
	gave = false;
	numPickup = inventory.pickupItemNames.Num();
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
	if ( gave && ( numPickup == inventory.pickupItemNames.Num() ) ) {
		inventory.AddPickupName( item->spawnArgs.GetString( "inv_name" ), item->spawnArgs.GetString( "inv_icon" ), this ); //_D3XP
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

	if ( gameLocal.isMultiplayer && !gameLocal.isClient ) {
		if ( PowerUpActive( MEGAHEALTH ) ) {
			if ( healthPool <= 0 ) {
				GiveHealthPool( 100 );
			}
		} else {
			healthPool = 0;
		}

		/*if( PowerUpActive( HASTE ) ) {
			switch( type ) {
			case SPEED: {
				mod = 1.7f;
				break;
						}
			}
		}*/
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
					inventory.AddPickupName("#str_00100627", "", this);
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
					if( !gameLocal.isMultiplayer ) {
						// Trying it out without the health boost (1/3/05)
						// Give the player full health in single-player
						// health = 100;
					} else {
						// Switch to fists in multiplayer
						idealWeapon = 1;
					}
				}
				break;
			}
			case INVISIBILITY: {
				if(gameLocal.isMultiplayer && !gameLocal.isClient) {
					inventory.AddPickupName("#str_00100628", "", this);
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
				inventory.AddPickupName("#str_00100799", "", this);
				stamina = 100.0f;
				break;
			 }
			case MEGAHEALTH: {
				if(gameLocal.isMultiplayer && !gameLocal.isClient) {
					inventory.AddPickupName("#str_00100629", "", this);
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
			case HELLTIME: {
				if ( spawnArgs.GetString( "snd_helltime_start", "", &sound ) ) {
					PostEventMS( &EV_StartSoundShader, 0, sound, SND_CHANNEL_ANY );
				}
				if ( spawnArgs.GetString( "snd_helltime_loop", "", &sound ) ) {
					PostEventMS( &EV_StartSoundShader, 0, sound, SND_CHANNEL_DEMONIC );
				}
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
				const idDict *lightDef = gameLocal.FindEntityDefDict( "player_head_light", false );
				if ( lightDef ) {
					idEntity *temp;
					gameLocal.SpawnEntityDef( *lightDef, &temp, false );

					idLight *eLight = static_cast<idLight *>(temp);
					eLight->GetPhysics()->SetOrigin( firstPersonViewOrigin );
					eLight->UpdateVisuals();
					eLight->Present();

					playerHeadLight = eLight;
				}
				break;
			}
			case ENVIROTIME: {
				hudPowerup = ENVIROTIME;
				// The HUD display bar is fixed at 60 seconds
				hudPowerupDuration = 60000;
				break;
			}
			case INVULNERABILITY: {
				if(gameLocal.isMultiplayer && !gameLocal.isClient) {
					inventory.AddPickupName("#str_00100630", "", this);
				}
				if(gameLocal.isMultiplayer) {
					/*if ( spawnArgs.GetString( "snd_invulnerable", "", &sound ) ) {
						StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_DEMONIC, 0, false, NULL );
					}*/
					if ( baseSkinName.Length() ) {
						powerUpSkin = declManager->FindSkin( baseSkinName + "_invuln" );
					}
				}
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
			if(!gameLocal.isMultiplayer) {
				StopHealthRecharge();
			}
			break;
		}
		case INVISIBILITY: {
			if ( weapon.GetEntity() ) {
				weapon.GetEntity()->UpdateSkin();
			}
			break;
		}
		case HELLTIME: {
			StopSound( SND_CHANNEL_DEMONIC, false );
			break;
		}
		case ENVIROSUIT: {
			
			hudPowerup = -1;

			// Turn off the envirosuit sound
			if ( gameSoundWorld ) {
				gameSoundWorld->SetEnviroSuit( false );
			}

			// Take off the helmet and lights
			if ( playerHeadLight.IsValid() ) {
				playerHeadLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
			}
			playerHeadLight = NULL;
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
	}
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
			if ( ( inventory.powerups & ( 1 << i ) ) && inventory.powerupEndTime[i] > gameLocal.time ) {
				switch( i ) {
					case ENVIROSUIT: {
						if ( playerHeadLight.IsValid() ) {
							idAngles lightAng = firstPersonViewAxis.ToAngles();
							idVec3 lightOrg = firstPersonViewOrigin;
							const idDict *lightDef = gameLocal.FindEntityDefDict( "player_head_light", false );

							playerHeadLightOffset = lightDef->GetVector( "player_head_light_offset" );
							playerHeadLightAngleOffset = lightDef->GetVector( "player_head_light_angle_offset" );

							lightOrg += (playerHeadLightOffset.x * firstPersonViewAxis[0]);
							lightOrg += (playerHeadLightOffset.y * firstPersonViewAxis[1]);
							lightOrg += (playerHeadLightOffset.z * firstPersonViewAxis[2]);
							lightAng.pitch += playerHeadLightAngleOffset.x;
							lightAng.yaw += playerHeadLightAngleOffset.y;
							lightAng.roll += playerHeadLightAngleOffset.z;

							playerHeadLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
							playerHeadLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
							playerHeadLight.GetEntity()->UpdateVisuals();
							playerHeadLight.GetEntity()->Present();
						}
						break;
					}
					default: {
						break;
					}
				}
			}
			if ( PowerUpActive( i ) && inventory.powerupEndTime[i] <= gameLocal.time ) {
				ClearPowerup( i );
			}
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
			health = inventory.maxHealth;
			healthPool = 0;
		} else {
			healthPool -= amt;
		}
		nextHealthPulse = gameLocal.time + HEALTHPULSE_TIME;
		healthPulse = true;
	}
#ifndef ID_DEMO_BUILD
	if ( !gameLocal.inCinematic && influenceActive == 0 && g_skill.GetInteger() == 3 && gameLocal.time > nextHealthTake && !AI_DEAD && health > g_healthTakeLimit.GetInteger() ) {
		assert( !gameLocal.isClient );	// healthPool never be set on client
		
		if(!PowerUpActive(INVULNERABILITY)) {
		health -= g_healthTakeAmt.GetInteger();
		if ( health < g_healthTakeLimit.GetInteger() ) {
			health = g_healthTakeLimit.GetInteger();
		}
		}
		nextHealthTake = gameLocal.time + g_healthTakeTime.GetInteger() * 1000;
		healthTake = true;
	}
#endif
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

	if ( gameLocal.isMultiplayer ) {
		if ( playerHeadLight.IsValid() ) {
			playerHeadLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
		}
	}
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
	inventory.pickupItemNames.Append( info );
	if ( hud ) {
		hud->SetStateString( "itemicon", info.icon );
		hud->HandleNamedEvent( "invPickup" );
	}
//Added to support powercells
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

	return true;
}
//BSM: Implementing this defined function for scripted give inventory items
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
	objectiveSystem->SetStateString( "objective4", "" );
	objectiveSystem->SetStateString( "objective5", "" );
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
	ShowObjective( "ShowNewObjective" );
	if ( hud ) {
		hud->HandleNamedEvent( "ShowNewObjective" );
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
	ShowObjective( "ShowObjectiveComplete" );

	if ( hud ) {
		hud->HandleNamedEvent( "ShowObjectiveComplete" );
	}
}

/*
===============
idPlayer::GiveVideo
===============
*/
void idPlayer::GiveVideo( const char *videoName, idDict *item ) {

	if ( videoName == NULL || *videoName == '\0' ) {
		return;
	}

	inventory.videos.AddUnique( videoName );

	if ( item ) {
		idItemInfo info;
		info.name = item->GetString( "inv_name" );
		info.icon = item->GetString( "inv_icon" );
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
	if ( GetPDA() ) {
		GetPDA()->SetSecurity( security );
		if ( hud ) {
			hud->SetStateString( "pda_security", "1" );
			hud->HandleNamedEvent( "securityPickup" );
		}
	} else {
		ShowTip( spawnArgs.GetString( "text_infoTitle" ), "You need a tablet in order to update your security clearance.", true );
	}
}

/*
===============
idPlayer::GiveEmail
===============
*/
void idPlayer::GiveEmail( const char *emailName ) {

	if ( emailName == NULL || *emailName == '\0' ) {
		return;
	}

	inventory.emails.AddUnique( emailName );
	if ( GetPDA() ) {
		GetPDA()->AddEmail( emailName );
	}

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

	if ( pdaName == NULL || *pdaName == '\0' ) {
		pdaName = "personal";
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
			if ( inventory.pdas.Num() > 1 ) { // BOYETTE NOTE IMPORTANT: we don't want the hud to do any animations for the standard pda the first time it is acquired.
				hud->HandleNamedEvent( "pdaPickup" );
			}
		}

		if ( inventory.pdas.Num() == 1 ) {
			GetPDA()->RemoveAddedEmailsAndVideos();
			if ( !objectiveSystemOpen ) {
				TogglePDA();
			}
			objectiveSystem->HandleNamedEvent( "showPDATip" );
			//ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_firstPDA" ), true );
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
//Added to support powercells
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

/*
===============
idPlayer::NextBestWeapon
===============
*/
void idPlayer::NextBestWeapon( void ) {
	const char *weap;
	int w = MAX_WEAPONS;

	if ( gameLocal.isClient || !weaponEnabled ) {
		return;
	}

	while ( w > 0 ) {
		w--;
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( !weap[ 0 ] || ( ( inventory.weapons.test(w) ) == false ) || ( !inventory.HasAmmo( weap, true, this ) ) ) {
			continue;
		}
		if ( !spawnArgs.GetBool( va( "weapon%d_best", w ) ) ) {
			continue;
		}

		//Some weapons will report having ammo but the clip is empty and 
		//will not have enough to fill the clip (i.e. Double Barrel Shotgun with 1 round left)
		//We need to skip these weapons because they cannot be used
		if(inventory.HasEmptyClipCannotRefill(weap, this)) {
			continue;
		}

		break;
	}
	idealWeapon = w;
	weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
	UpdateHudWeapon();
}

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
	if ( inventory.weapons.count() == 0 ) {
		return;
	}
	
	w = idealWeapon;
	while( 1 ) {
		w++;
		if ( w >= MAX_WEAPONS ) {
			w = 0;
		} 
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( !spawnArgs.GetBool( va( "weapon%d_cycle", w ) ) ) {
			continue;
		}
		if ( !weap[ 0 ] ) {
			continue;
		}
		if ( ( inventory.weapons.test(w) ) == false ) {
			continue;
		}

		if ( inventory.HasAmmo( weap, true, this ) || w == weapon_bloodstone ) {
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}
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
	if ( inventory.weapons.count() == 0 ) {
		return;
	}

	w = idealWeapon;
	while( 1 ) {
		w--;
		if ( w < 0 ) {
			w = MAX_WEAPONS - 1;
		}
		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
		if ( !spawnArgs.GetBool( va( "weapon%d_cycle", w ) ) ) {
			continue;
		}
		if ( !weap[ 0 ] ) {
			continue;
		}
		if ( ( inventory.weapons.test(w) ) == false ) {
			continue;
		}
		if ( inventory.HasAmmo( weap, true, this ) || w == weapon_bloodstone ) {
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}
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
			if( inventory.weapons.test(weaponToggle->toggleList[weaponToggleIndex]) ) {
				break;
			}

			weaponToggleIndex++;
			if(weaponToggleIndex >= weaponToggle->toggleList.Num()) {
				weaponToggleIndex = 0;
			}
		}

		num = weaponToggle->toggleList[weaponToggleIndex];
	}

	if ( force || ( inventory.weapons.test(num) ) ) {
		if ( !inventory.HasAmmo( weap, true, this ) && !spawnArgs.GetBool( va( "weapon%d_allowempty", num ) ) ) {
			return;
		}
		if ( ( previousWeapon >= 0 ) && ( idealWeapon == num ) && ( spawnArgs.GetBool( va( "weapon%d_toggle", num ) ) ) ) {
			weap = spawnArgs.GetString( va( "def_weapon%d", previousWeapon ) );
			if ( !inventory.HasAmmo( weap, true, this ) && !spawnArgs.GetBool( va( "weapon%d_allowempty", previousWeapon ) ) ) {
				return;
			}
			idealWeapon = previousWeapon;
		} else if ( ( weapon_pda >= 0 ) && ( num == weapon_pda ) && ( inventory.pdas.Num() == 0 ) ) {
			ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_noPDA" ), true );
			return;
		} else {
			idealWeapon = num;
		}
		UpdateHudWeapon();
	}
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

	ammoavailable += inclip;

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
		inclipKey.Insert( va("%.2d", currentWeapon), 11);
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
	if ( ! ( player->inventory.weapons.test(newweap) ) ) {
		return;
	}
	const char *weapon_classname = spawnArgs.GetString( va( "def_weapon%d", newweap ) );
	assert( weapon_classname );
	int ammoavailable = player->weapon.GetEntity()->AmmoAvailable();
	int inclip = player->weapon.GetEntity()->AmmoInClip();

	ammoavailable += inclip;

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

	inventory.clip[ newweap ] = inclip;
}

/*
===============
idPlayer::ActiveGui
===============
*/
idUserInterface *idPlayer::ActiveGui( void ) {
	// boyette begin
	if (guiOverlay) {
		return guiOverlay;
	}
	// boyette end

	if ( objectiveSystemOpen ) {
		return objectiveSystem;
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

	if ( idealWeapon == weapon_soulcube && soulCubeProjectile.GetEntity() != NULL ) {
		idealWeapon = currentWeapon;
	}

	if ( idealWeapon != currentWeapon ) {
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
			if ( !weapon.GetEntity()->AmmoAvailable() ) {
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
			FireWeapon();
		} else if ( oldButtons & BUTTON_ATTACK ) {
			AI_ATTACK_HELD = false;
			weapon.GetEntity()->EndAttack();
		}
	}

	// update our ammo clip in our inventory
	if ( ( currentWeapon >= 0 ) && ( currentWeapon < MAX_WEAPONS ) ) {
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
		// BOYETTE SPACE COMMAND BEGIN
		if ( focusCharacter->uses_space_command_dialogue_behavior ) {
			// will open up the captain dialogue gui and set the focusCharacter as the DialogueAI
			//DialogueAI = focusCharacter; // not necessary anymore because it is in SetOverlayAIDialogeGui();
			SetOverlayAIDialogeGui(); // BOYETTE NOTE TODO: maybe instead of just opening it we will tell the script to open it.
			focusCharacter->TalkTo( this ); // maybe we will have some greeting play when we begin a dialogue
		//} else {
		//if ( DialogueAI && DialogueAI->currently_in_dialogue_with_player ) {
			// do nothing
		} else {
		//if ( !guiOverlay ) {
		//if ( talkCursor ) {
			focusCharacter->TalkTo( this ); // original
		}
		//}
		//}
		//}
		// BOYETTE SPACE COMMAND END
	}
}

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

	if ( hiddenWeapon && tipUp && usercmd.buttons & BUTTON_ATTACK ) {
		HideTip();
	}
	
	if ( g_dragEntity.GetBool() ) {
		StopFiring();
		weapon.GetEntity()->LowerWeapon();
		dragEntity.Update( this );
	} else if ( ActiveGui() ) {
		// gui handling overrides weapon use
		Weapon_GUI();
	} else 	if ( focusCharacter && ( focusCharacter->health > 0 ) && focusCharacter->GetTalkState() == TALK_OK && (!focusCharacter->uses_space_command_dialogue_behavior || (focusCharacter->uses_space_command_dialogue_behavior && (focusCharacter->team == team || focusCharacter->waiting_until_player_talks_to_me))) ) {
		Weapon_NPC();
	} else {
		Weapon_Combat();
	}
	
	if ( hiddenWeapon ) {
		weapon.GetEntity()->LowerWeapon();
	}

	// update weapon state, particles, dlights, etc
	weapon.GetEntity()->PresentWeapon( showWeaponViewModel );
// boyette weapon icon begin
	UpdateHudWeapon();
// boyette weapon icon end

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

void ParellelJobsListPerformanceTestFunction() {
   int NUM_ELEMENTS = 5000000;
   int i, copy_vec;
   std::vector<int> vec;

   // Push values to vector
   for(i = 0; i < NUM_ELEMENTS; i++)
   {
      vec.push_back(i);
   } // for

   // Access and assignment (using [] operator)
   for(i = 0; i < NUM_ELEMENTS; i++)
   {
      copy_vec = vec[i];
   }

   return;
}
void ParellelJobsListPerformanceTest() {
	std::clock_t begin_time,end_time;
	float time_difference;

	// WITH PARALLEL PROCESSING BEGIN
	begin_time = clock();

	//put processing here
	for ( int i = 0; i < 10; i++ ) {
		common->AddJobToGameLogicJobList((jobRun_t)ParellelJobsListPerformanceTestFunction,NULL);
	}
	common->SubmitGameLogicJobList();
	common->WaitGameLogicJobList();

	end_time = clock();
	time_difference = (float)end_time - (float)begin_time;

	gameLocal.Printf( "With Parallel Processing: " + idStr(time_difference) + "\n" );
	// WITH PARALLEL PROCESSING END

	// WITHOUT PARALLEL PROCESSING BEGIN
	begin_time = clock();

	//put processing here
	for ( int i = 0; i < 10; i++ ) {
		ParellelJobsListPerformanceTestFunction();
	}

	end_time = clock();
	time_difference = (float)end_time - (float)begin_time;

	gameLocal.Printf( "Without Parallel Processing: " + idStr(time_difference) + "\n" );
	// WITHOUT PARALLEL PROCESSING END
}

/*
===============
idPlayer::HandleSingleGuiCommand
===============
*/
bool idPlayer::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	idToken token, token2, token3, token4;

	if ( !src->ReadToken( &token ) ) {
		return false;
	}

	if ( token == ";" ) {
		return false;
	}

	if ( token.Icmp( "addhealth" ) == 0 ) {
		if ( entityGui && health < 100 ) {
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
					EndMusic();
					const idSoundShader *shader = declManager->FindSound( pdaVideoWave );
					StartSoundShader( shader, SND_CHANNEL_PDA, 0, false, NULL );
				}
			}
		}
	}

	if ( token.Icmp( "playpdaaudio" ) == 0 ) {
		if ( objectiveSystem && objectiveSystemOpen && pdaAudio.Length() > 0 ) {
			EndMusic();
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

	if (token.Icmp("click_quit_game_and_return_to_main_menu") == 0) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
		return true;
	}


		// boyette mod begin
//	idEntity		*TargetEntity;

//	TargetEntity = gameLocal.FindEntity( "entity_sbplayership_1" ); // not sure if this is necessary

	if (token.Icmp("click_initiate_test") == 0) {
		ParellelJobsListPerformanceTest();
		/*
		if ( ShipOnBoard ) {
			for( int i = 0; i < ShipOnBoard->shiplights.Num(); i++ ) {
				for( int ix = 0; ix < ShipOnBoard->shiplights[ i ].GetEntity()->targets.Num(); ix++ ) {
					if ( ShipOnBoard->shiplights[ i ].GetEntity()->targets[ix].GetEntity()->IsType( idLight::Type ) ) {
						ShipOnBoard->shiplights[ i ].GetEntity()->targets[ix].GetEntity()->SetOrigin( ShipOnBoard->shiplights[ i ].GetEntity()->targets[ix].GetEntity()->GetPhysics()->GetOrigin() + idVec3(0,0,1));
					}
				}
			}
		}
		*/
		/*
		for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) {
			for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) {
				if ( gameLocal.stargridPositionsOffLimitsToShipAI[x][y] ) {
					gameLocal.Printf( "X: " + idStr(x) + ", Y: " + idStr(y) + "\n" );
				}
			}
		}
		*/
		if ( ShipOnBoard ) {
			ShipOnBoard->DimRandomShipLightsOff();
		}
		//if ( ShipOnBoard ) {
		//	ShipOnBoard->DimShipLightsOff();
		//}
		/*
		int max_x = 0;
		int max_y = 0;
		int min_x = 200;
		int min_y = 200;
		sbShip* ships_with_max_mins[4];
		ships_with_max_mins[0] = NULL;
		ships_with_max_mins[1] = NULL;
		ships_with_max_mins[2] = NULL;
		ships_with_max_mins[3] = NULL;
		for ( int i = 0; i < gameLocal.num_entities ; i++ ) {
			idEntity	*ent;
			ent = gameLocal.entities[ i ];
			if ( ent && ( ent->IsType(sbShip::Type) ) ) { // || ent->IsType(sbStationarySpaceEntity::Type) ) ) {
				sbShip* ship_to_test = static_cast<sbShip*>(ent);

				gameLocal.Printf( ent->name + ": " + idStr(ship_to_test->stargridpositionx) + "," + idStr(ship_to_test->stargridpositiony) + "\n" );
				if ( ship_to_test->stargridpositionx > max_x ) { max_x = ship_to_test->stargridpositionx; ships_with_max_mins[0] = ship_to_test; }
				if ( ship_to_test->stargridpositiony > max_y ) { max_y = ship_to_test->stargridpositiony; ships_with_max_mins[1] = ship_to_test; }
				if ( ship_to_test->stargridpositionx < min_x ) { min_x = ship_to_test->stargridpositionx; ships_with_max_mins[2] = ship_to_test; }
				if ( ship_to_test->stargridpositiony < min_y ) { min_y = ship_to_test->stargridpositiony; ships_with_max_mins[3] = ship_to_test; }
				if ( !ship_to_test->fl.isDormant ) {
				//if ( ship_is_dormant ) {
					//gameLocal.Printf( ent->name + "\n" );
				}
			}
		}
		gameLocal.Printf( "The max X is: " + idStr(max_x) + "\n" );
		gameLocal.Printf( "The max Y is: " + idStr(max_y) + "\n" );
		gameLocal.Printf( "The min X is: " + idStr(min_x) + "\n" );
		gameLocal.Printf( "The min Y is: " + idStr(min_y) + "\n" );
		if ( ships_with_max_mins[0] && ships_with_max_mins[1] && ships_with_max_mins[2] && ships_with_max_mins[3] ) {
			gameLocal.Printf( "The ship: " + ships_with_max_mins[0]->name + " has the maximum X: " + idStr(max_x) + "\n" );
			gameLocal.Printf( "The ship: " + ships_with_max_mins[1]->name + " has the maximum Y: " + idStr(max_y) + "\n" );
			gameLocal.Printf( "The ship: " + ships_with_max_mins[2]->name + " has the minimum X: " + idStr(min_x) + "\n" );
			gameLocal.Printf( "The ship: " + ships_with_max_mins[3]->name + " has the minimum Y: " + idStr(min_y) + "\n" );
		}
		*/

		//bool ship_is_dormant = true;
		/*
		gameLocal.Printf( "Aboard the " + ship_to_test->name + ": \n" );
		for ( int ix = 0; ix < ship_to_test->AIsOnBoard.size() ; ix++ ) {
			if ( ship_to_test->AIsOnBoard[ix] ) { //&& ship_to_test->AIsOnBoard[ix]->fl.neverDormant == true ) {
				gameLocal.Printf( "     " + ship_to_test->AIsOnBoard[ix]->name + "\n" );
				//ship_is_dormant = false;
			}
		}
		*/
		/*
		gameLocal.Printf( "The neutral teams of the " + ship_to_test->name + ": \n" );
		for ( int ix = 0; ix < ship_to_test->neutral_teams.size() ; ix++ ) {
			if ( ship_to_test->neutral_teams[ix]  ) {
				gameLocal.Printf( "     " + idStr(ship_to_test->neutral_teams[ix]) + "\n" );
				//ship_is_dormant = false;
			}
		}
		*/

		if ( PlayerShip ) {
			sbShip* ship_to_list = NULL;
			gameLocal.Printf( "In the PlayerShip  ships_at_my_stargrid_position: \n" );
			for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size() ; i++ ) {
				ship_to_list = PlayerShip->ships_at_my_stargrid_position[i];
				if ( ship_to_list ) {
					gameLocal.Printf( "     " + ship_to_list->name + "\n" );
				}
			}
		}

		/*
		if ( PlayerShip->TargetEntityInSpace ) {

			gameLocal.Printf( idStr(gameLocal.random.RandomInt(idRandom::MAX_RAND)) + ".\n" );
			gameLocal.Printf( idStr(gameLocal.random.RandomInt(idRandom::MAX_RAND)) + ".\n" );
			gameLocal.Printf( idStr(gameLocal.random.RandomInt(idRandom::MAX_RAND)) + ".\n" );
			gameLocal.Printf( idStr(gameLocal.random.RandomInt(idRandom::MAX_RAND)) + ".\n" );
			gameLocal.Printf( idStr("My Team Is: ") + idStr(team) + ".\n" );
			gameLocal.Printf( idStr("Player Yaw Is: ") + idStr(viewAngles.yaw) + ".\n" );

			if ( CaptainChairSeatedIn ) {

					gameLocal.Printf( idStr("The Captain Chair Min Yaw Is: ") + idStr(CaptainChairSeatedIn->min_view_angles.yaw) + ".\n" );
					gameLocal.Printf( idStr("The Captain Chair Max Yaw Is: ") + idStr(CaptainChairSeatedIn->max_view_angles.yaw) + ".\n" );
			}

			PlayerShip->TargetEntityInSpace->RecieveVolley();
			PlayerShip->RecieveVolley();
			TriggerHealFX(PlayerShip->spawnArgs.GetString("heal_actor_emitter_def", "heal_actor_emitter_def_default"));
			TriggerTransporterFX(PlayerShip->spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));

			idStr test1 = "This should be yellow. ";
			idStr test2 = "This should be red.";
			UpdateNotificationList("^3" + test1 + "^0" "Test");

			//SetOverlayHailGui();
			//return true;
		}
//		UpdateHudStats(guiOverlay);

		if ( gameLocal.SpaceCommandViewscreenCamera && gameLocal.SpaceCommandViewscreenCamera->IsType( idSecurityCamera::Type ) ) {
			if ( PlayerShip ) {
				for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size(); i++ ) {
					if ( PlayerShip->ships_at_my_stargrid_position[i] ) {
						if ( static_cast<idSecurityCamera*>(gameLocal.SpaceCommandViewscreenCamera)->CanSeeAllEntityBounds(PlayerShip->ships_at_my_stargrid_position[i]) ) {
							gameLocal.Printf( PlayerShip->ships_at_my_stargrid_position[i]->name + " : can see bounds\n" );
						}
						if ( static_cast<idSecurityCamera*>(gameLocal.SpaceCommandViewscreenCamera)->CanSeeEntity(PlayerShip->ships_at_my_stargrid_position[i]) ) {
							gameLocal.Printf( PlayerShip->ships_at_my_stargrid_position[i]->name + " : can see origin\n" );
						}
						if ( static_cast<idSecurityCamera*>(gameLocal.SpaceCommandViewscreenCamera)->CanSeeAnyEntityBounds(PlayerShip->ships_at_my_stargrid_position[i]) ) {
							gameLocal.Printf( PlayerShip->ships_at_my_stargrid_position[i]->name + " : can see any bounds\n" );
						}
					}
					//if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType(sbShip::Type) ) {
					//	gameLocal.Printf( "The " + gameLocal.entities[ i ]->name + " has the following ships at the same SG pos:\n" );
					//	for ( int x = 0; x < dynamic_cast<sbShip*>( gameLocal.entities[ i ] )->ships_at_my_stargrid_position.size(); x++ ) {
					//		gameLocal.Printf( dynamic_cast<sbShip*>( gameLocal.entities[ i ] )->ships_at_my_stargrid_position[x]->name + "\n" );
					//	}
					//}
				}
			}
			return true;
		}

		if ( PlayerShip && PlayerShip->TargetEntityInSpace ) {
			//PlayerShip->TargetEntityInSpace->TargetEntityInSpace = PlayerShip;


			//for (int i=0; i < PlayerShip->AIsOnBoard.size(); i++) {
			//	gameLocal.Printf( PlayerShip->AIsOnBoard[i]->name + ".\n" );
			//}




			return true;
		}
		*/
		for (int i=0; i < PlayerShip->AIsOnBoard.size(); i++) {
			gameLocal.Printf( PlayerShip->AIsOnBoard[i]->name + ".\n" );
		}
		return true;
	}
	if (token.Icmp("click_start_captaingui_slowmo") == 0) {
		if ( PlayerShip && guiOverlay ) {
			g_enableCaptainGuiSlowmo.SetBool(true);
			return true;
		}
	}
	if (token.Icmp("click_stop_captaingui_slowmo") == 0) {
		if ( PlayerShip && guiOverlay ) {
			g_enableCaptainGuiSlowmo.SetBool(false);
			return true;
		}
	}
	if (token.Icmp("click_initiate_ship_repair_mode") == 0) {
		if ( PlayerShip ) {
			if ( PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition() ) {
				DisplayNonInteractiveAlertMessage("You cannot repair your ship when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( PlayerShip->CheckForHostileEntitiesOnBoard() ) {
				DisplayNonInteractiveAlertMessage("You cannot repair your ship when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
			PlayerShip->InitiateShipRepairMode();
			StartSoundShader( declManager->FindSound("space_command_guisounds_computer_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_cancel_ship_repair_mode") == 0) {
		if ( PlayerShip ) {
			PlayerShip->CancelShipRepairMode();
			StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_toggle_playership_repair_mode") == 0) {
		if ( PlayerShip ) {
			if ( PlayerShip->in_repair_mode ) {
				PlayerShip->CancelShipRepairMode();
				StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else {
				if ( PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition() ) {
					DisplayNonInteractiveAlertMessage("You cannot repair your ship when there is a hostile entity in range.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else if ( PlayerShip->CheckForHostileEntitiesOnBoard() ) {
					DisplayNonInteractiveAlertMessage("You cannot repair your ship when there are hostile entities on board.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
				PlayerShip->InitiateShipRepairMode();
				StartSoundShader( declManager->FindSound("space_command_guisounds_computer_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
	if (token.Icmp("click_playership_dim_lights") == 0) {
		if ( PlayerShip ) {
			PlayerShip->DimShipLightsOff();
			StartSoundShader( declManager->FindSound("player_sounds_skip_cinematic_01") , SND_CHANNEL_ANY, 0, false, NULL );
		}
	}

	if (token.Icmp("click_test_target_ship_warp") == 0) {
		if ( PlayerShip && PlayerShip->TargetEntityInSpace ) {
			//PlayerShip->TargetEntityInSpace->EngageWarp(gameLocal.random.RandomInt(14),gameLocal.random.RandomInt(11));
			//PlayerShip->TargetEntityInSpace->EngageWarp(3,4);
			PlayerShip->TargetEntityInSpace->is_attempting_warp = PlayerShip->TargetEntityInSpace->AttemptWarp(3,4);
			return true;
		}
	}

	if (token.Icmp("click_send_crew_to_battlestations") == 0) {

			// Bloom test but I think it still needs a bloom.vfp file. Although it looks OK - Not great  - BEGIN
			//Event_ToggleBloom(1);

			//Event_SetBloomParms(1,-0.01f);
			//cvarSystem->SetCVarFloat( "g_testBloomSpeed" , 1 );
			//cvarSystem->SetCVarFloat( "g_testBloomIntensity", -0.01f );
			//cvarSystem->SetCVarInteger( "g_testBloomNumPasses", 10 );
			// Bloom test but I think it still needs a bloom.vfp file. Although it looks OK - Not great - END
		if ( PlayerShip ) {
			PlayerShip->SendCrewToBattlestations();
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}

	// DECONSTRUCT TARGETSHIP BUTTON
	//if ( PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->IsType( sbShip::Type ) && !PlayerShip->TargetEntityInSpace->IsType( sbStationarySpaceEntity::Type ) && PlayerShip->TargetEntityInSpace->is_derelict && !PlayerShip->TargetEntityInSpace->ship_deconstruction_sequence_initiated && !PlayerShip->ship_beam_active ) {
	if ( PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->can_be_deconstructed && ( (PlayerShip->TargetEntityInSpace->is_derelict && PlayerShip->TargetEntityInSpace->IsType( sbShip::Type )) || PlayerShip->TargetEntityInSpace->IsType( sbStationarySpaceEntity::Type ) ) && !PlayerShip->TargetEntityInSpace->ship_deconstruction_sequence_initiated && !PlayerShip->ship_beam_active && ShipOnBoard != PlayerShip->TargetEntityInSpace ) {
		if (token.Icmp("click_deconstruct_targetship") == 0) {
			if ( PlayerShip->TargetEntityInSpace->can_be_deconstructed ) {
				PlayerShip->TargetEntityInSpace->ship_deconstruction_sequence_initiated = true;
				PlayerShip->CreateBeamToEnt( PlayerShip->TargetEntityInSpace );

				// add some amount of materials to the player based on the remaining hullstrength of the targetship - display a message saying you salvaged this many materials from the targetship.
				PlayerShip->current_materials_reserves = PlayerShip->current_materials_reserves + PlayerShip->TargetEntityInSpace->hullStrength;
				DisplayNonInteractiveAlertMessage( "You recovered " + idStr(PlayerShip->TargetEntityInSpace->hullStrength) + " materials from the " + PlayerShip->TargetEntityInSpace->original_name );
				UpdateNotificationList ( "The " + PlayerShip->name + " recovered " + idStr(PlayerShip->TargetEntityInSpace->hullStrength) + " materials from the " + PlayerShip->TargetEntityInSpace->original_name );

				//PlayerShip->TargetEntityInSpace->BeginTransporterMaterialShaderEffect(PlayerShip->fx_color_theme); // I moved this back into PlayerShip->TargetEntityInSpace->BeginShipDestructionSequence(); below for various reasons
				idEntityFx::StartFx( PlayerShip->spawnArgs.GetString("deconstruction_fx", "fx/ship_deconstruct_default"), &(PlayerShip->TargetEntityInSpace->GetPhysics()->GetOrigin()), 0, PlayerShip->TargetEntityInSpace, true ); // BOYETTE NOTE TODO: might want to make this a spawnarg - then the mapper would just have to match up the fx with the destruction time.

				PlayerShip->CeaseFiringWeaponsAndTorpedos();
				// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
				if ( common->m_pStatsAndAchievements ) {
					if ( idStr::Icmp(PlayerShip->TargetEntityInSpace->spawnArgs.GetString("faction_name", "Unknown"), "Space Insect" ) == 0 ) {
						common->m_pStatsAndAchievements->m_nPlayerSpaceInsectKills++;
						common->StoreSteamStats();
					} else if ( !PlayerShip->TargetEntityInSpace->IsType( sbStationarySpaceEntity::Type ) ) {
						common->m_pStatsAndAchievements->m_nPlayerStarshipKills++;
						common->StoreSteamStats();
					}
				}
#endif
				// BOYETTE STEAM INTEGRATION END
				PlayerShip->TargetEntityInSpace->BeginShipDestructionSequence();
				//PlayerShip->TargetEntityInSpace = NULL; // this happens in BeginShipDestructionSequence() so this is just to be sure.
				UpdateCaptainMenu();
				if ( SelectedEntityInSpace && SelectedEntityInSpace == PlayerShip->TargetEntityInSpace ) {
					if ( guiOverlay == HailGui ) {
						HailGui->HandleNamedEvent("HailedSelectedShipHasBeenDestroyed");
					}
					SelectedEntityInSpace->currently_in_hail = false;
					SelectedEntityInSpace = NULL;
				}

				//PlayerShip->ReduceAllModuleChargesToZero(); // maybe display a message letting them know that this was because of the deconstruction.
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) { //- need to exclude engines
					if ( PlayerShip->consoles[i] && PlayerShip->consoles[i]->ControlledModule && i != ENGINESMODULEID ) {
						PlayerShip->consoles[i]->ControlledModule->current_charge_amount = 0;
					}
				}
				StartSoundShader( declManager->FindSound("space_command_guisounds_item_pickup_01") , SND_CHANNEL_ANY, 0, false, NULL );
			} else {
				UpdateNotificationList( "The " + PlayerShip->TargetEntityInSpace->original_name + " cannot be deconstructed because of interference" );
				DisplayNonInteractiveAlertMessage( "Interference from the " + PlayerShip->TargetEntityInSpace->original_name + " is preventing its deconstruction" );
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
			}
			return true;
		}
	}

	if (token.Icmp("click_scan_selectedship") == 0) {
		if ( PlayerShip && this->SelectedEntityInSpace && PlayerShip->consoles[SENSORSMODULEID] && PlayerShip->consoles[SENSORSMODULEID]->ControlledModule && PlayerShip->consoles[SENSORSMODULEID]->ControlledModule->current_charge_percentage >= 1.0f ) {
			PlayerShip->consoles[SENSORSMODULEID]->ControlledModule->current_charge_amount = 0;
			SelectedEntityInSpace->was_sensor_scanned = true;
			UpdateSelectedEntityInSpaceOnGuis();
			//StartSoundShader( declManager->FindSound("spaceship_sensor_scan_entity_default") , SND_CHANNEL_ANY, 0, false, NULL );
			StartSoundShader( declManager->FindSound("space_command_guisounds_static_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_scan_targetship") == 0) {
		if ( PlayerShip && this->SelectedEntityInSpace && PlayerShip->consoles[SENSORSMODULEID] && PlayerShip->consoles[SENSORSMODULEID]->ControlledModule && PlayerShip->consoles[SENSORSMODULEID]->ControlledModule->current_charge_percentage >= 1.0f ) {
			if ( PlayerShip->TargetEntityInSpace ) {
				SelectedEntityInSpace = PlayerShip->TargetEntityInSpace;
			}
			PlayerShip->consoles[SENSORSMODULEID]->ControlledModule->current_charge_amount = 0;
			SelectedEntityInSpace->was_sensor_scanned = true;
			UpdateSelectedEntityInSpaceOnGuis();
			StartSoundShader( declManager->FindSound("space_command_guisounds_static_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}

	// TOGGLES
	if (token.Icmp("click_playership_go_to_red_alert") == 0) {	
		if ( PlayerShip ) {
			PlayerShip->GoToRedAlert();
			StartSoundShader( declManager->FindSound("space_command_guisounds_warning_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}

	if (token.Icmp("click_cancel_playership_red_alert") == 0) {	
		if ( PlayerShip ) {
			PlayerShip->CancelRedAlert();
			StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_toggle_playership_red_alert") == 0) {
		if ( PlayerShip && PlayerShip->red_alert ) {
			PlayerShip->CancelRedAlert();
			StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
			//PlayerShip->LowerShields();
			return true;
		} else if ( PlayerShip ) {
			PlayerShip->GoToRedAlert();
			StartSoundShader( declManager->FindSound("space_command_guisounds_warning_01") , SND_CHANNEL_ANY, 0, false, NULL );
			//PlayerShip->RaiseShields();
			return true;
		}
	}
	if (token.Icmp("click_toggle_playership_shields_raised") == 0) {
		if ( PlayerShip ) {
			if ( PlayerShip->shields_raised ) {
				PlayerShip->LowerShields();
				StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else {
				PlayerShip->RaiseShields();
				StartSoundShader( declManager->FindSound("space_command_guisounds_ping_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
	if (token.Icmp("click_toggle_playership_self_destruct") == 0) {
		if ( PlayerShip && PlayerShip->ship_self_destruct_sequence_initiated ) {
			PlayerShip->CancelSelfDestructSequence();
			StartSoundShader( declManager->FindSound("space_command_guisounds_beepdown_02") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		} else if ( PlayerShip ) {
			PlayerShip->InitiateSelfDestructSequence();
			StartSoundShader( declManager->FindSound("source_default_arcade_sadsound") , SND_CHANNEL_ANY, 0, false, NULL );
			if ( PlayerShip && !PlayerShip->red_alert ) {
				PlayerShip->GoToRedAlert();
			}
			return true;
		}
	}
	if (token.Icmp("click_turn_playership_shiplights_off") == 0) {
		if ( PlayerShip ) {
			PlayerShip->TurnShipLightsOff();
			StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_turn_playership_shiplights_on") == 0) {
		if ( PlayerShip ) {
			PlayerShip->TurnShipLightsOn();
			StartSoundShader( declManager->FindSound("space_command_guisounds_ping_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}

// GIVE MOVE COMMAND TO SELECTED CREW MEMBERS
	// On PlayerShip
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
			if (token.Icmp( "click_give_crew_move_command_" + room_description[i] + "_room_node" ) == 0) {
				if ( PlayerShip && PlayerShip->room_node[i] ) {
					PlayerShip->GiveCrewMoveCommand(PlayerShip->room_node[i],PlayerShip);
					StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
	}
	// On TargetShip
	if ( PlayerShip && PlayerShip->TargetEntityInSpace ) {
		for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
			if (token.Icmp( "click_give_crew_move_command_" + room_description[i] + "_room_node_on_targetship" ) == 0) {
				if ( PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->room_node[i] ) {
					PlayerShip->GiveCrewMoveCommand(PlayerShip->TargetEntityInSpace->room_node[i],PlayerShip->TargetEntityInSpace);
					StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
	}




	// On PlayerShip
	if ( PlayerShip ) {
		int misc_crew_counter = 0;
		for ( int i = 0; i < PlayerShip->AIsOnBoard.size(); i++ ) {
			if ( PlayerShip->AIsOnBoard[i] && PlayerShip->AIsOnBoard[i]->ShipOnBoard && PlayerShip->AIsOnBoard[i]->ShipOnBoard == PlayerShip && !PlayerShip->AIsOnBoard[i]->was_killed && !PlayerShip->IsThisAIACrewmember(PlayerShip->AIsOnBoard[i]) ) {
				if (token.Icmp( "click_give_crew_move_command_to_misc_crew_" + idStr(misc_crew_counter) + "_on_playership" ) == 0) {
					PlayerShip->GiveCrewMoveCommand(PlayerShip->AIsOnBoard[i],PlayerShip);
					StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
				misc_crew_counter++;
			}
		}
	}
	// On TargetShip
	if ( PlayerShip && PlayerShip->TargetEntityInSpace ) {
		int misc_crew_counter = 0;
		for ( int i = 0; i < PlayerShip->TargetEntityInSpace->AIsOnBoard.size(); i++ ) {
			if ( PlayerShip->TargetEntityInSpace->AIsOnBoard[i] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->ShipOnBoard && PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->ShipOnBoard == PlayerShip->TargetEntityInSpace && !PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->was_killed && !PlayerShip->IsThisAIACrewmember(PlayerShip->TargetEntityInSpace->AIsOnBoard[i]) ) {
				if (token.Icmp( "click_give_crew_move_command_to_misc_crew_" + idStr(misc_crew_counter) + "_on_targetship" ) == 0) {
					PlayerShip->GiveCrewMoveCommand(PlayerShip->TargetEntityInSpace->AIsOnBoard[i],PlayerShip->TargetEntityInSpace);
					StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
				misc_crew_counter++;
			}
		}
	}





	// On PlayerShip
	if (token.Icmp("click_give_crew_move_command_transporter") == 0) {
		if ( PlayerShip && PlayerShip->TransporterBounds ) {
			PlayerShip->GiveCrewMoveCommand(PlayerShip->TransporterBounds,PlayerShip);
			StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
	if (token.Icmp("click_give_crew_move_command_captain_chair") == 0) {
		if ( PlayerShip && PlayerShip->CaptainChair ) {
			PlayerShip->GiveCrewMoveCommand(PlayerShip->CaptainChair,PlayerShip);
			StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
	// On TargetShip
	if (token.Icmp("click_give_crew_move_command_transporter_on_targetship") == 0) {
		if ( PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->TransporterBounds ) {
			PlayerShip->GiveCrewMoveCommand(PlayerShip->TargetEntityInSpace->TransporterBounds,PlayerShip->TargetEntityInSpace);
			StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
	if (token.Icmp("click_give_crew_move_command_captain_chair_on_targetship") == 0) {
		if ( PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->CaptainChair ) {
			PlayerShip->GiveCrewMoveCommand(PlayerShip->TargetEntityInSpace->CaptainChair,PlayerShip->TargetEntityInSpace);
			StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
// GRAB MODULES
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if (token.Icmp("grab_" + role_description[i] + "_module") == 0) {
				grabbed_module_exists = true;
				grabbed_module_id = i;
				UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				UpdateModulesPowerQueueOnCaptainGui();
				if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
					CaptainGui->multiselect_box_active = false;
					CaptainGui->EndMultiselectBox();
				}
				StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if (token.Icmp("move_grabbed_module_to_" + role_description[i] + "_module_position_in_weapons_queue") == 0) {
				if ( grabbed_module_exists ) {
					int move_to_pos;
					int move_from_pos;
					for ( move_to_pos = 0; move_to_pos < MAX_MODULES_ON_SHIPS; move_to_pos++ ) {
						if ( i == PlayerShip->WeaponsTargetQueue[move_to_pos] ) {
							break;
						}
					}
					for ( move_from_pos = 0; move_from_pos < MAX_MODULES_ON_SHIPS; move_from_pos++ ) {
						if ( grabbed_module_id == PlayerShip->WeaponsTargetQueue[move_from_pos] ) {
							break;
						}
					}
					while ( move_from_pos > move_to_pos ) {
						PlayerShip->MoveUpModuleInWeaponsTargetQueue(grabbed_module_id);
						move_from_pos--;
					}
					while ( move_from_pos < move_to_pos ) {
						PlayerShip->MoveDownModuleInWeaponsTargetQueue(grabbed_module_id);
						move_from_pos++;
					}
					if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
						CaptainGui->multiselect_box_active = false;
						CaptainGui->EndMultiselectBox();
					}
					grabbed_module_exists = false;
					grabbed_module_id = 0;
					UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
					StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if (token.Icmp("move_grabbed_module_to_" + role_description[i] + "_module_position_in_torpedos_queue") == 0) {
				if ( grabbed_module_exists ) {
					int move_to_pos;
					int move_from_pos;
					for ( move_to_pos = 0; move_to_pos < MAX_MODULES_ON_SHIPS; move_to_pos++ ) {
						if ( i == PlayerShip->TorpedosTargetQueue[move_to_pos] ) {
							break;
						}
					}
					for ( move_from_pos = 0; move_from_pos < MAX_MODULES_ON_SHIPS; move_from_pos++ ) {
						if ( grabbed_module_id == PlayerShip->TorpedosTargetQueue[move_from_pos] ) {
							break;
						}
					}
					while ( move_from_pos > move_to_pos ) {
						PlayerShip->MoveUpModuleInTorpedosTargetQueue(grabbed_module_id);
						move_from_pos--;
					}
					while ( move_from_pos < move_to_pos ) {
						PlayerShip->MoveDownModuleInTorpedosTargetQueue(grabbed_module_id);
						move_from_pos++;
					}
					if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
						CaptainGui->multiselect_box_active = false;
						CaptainGui->EndMultiselectBox();
					}
					grabbed_module_exists = false;
					grabbed_module_id = 0;
					UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
					StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if (token.Icmp("move_grabbed_module_to_" + role_description[i] + "_module_position_in_autopower_queue") == 0) {
				if ( grabbed_module_exists ) {
					int move_to_pos;
					int move_from_pos;
					for ( move_to_pos = 0; move_to_pos < MAX_MODULES_ON_SHIPS; move_to_pos++ ) {
						if ( i == PlayerShip->ModulesPowerQueue[move_to_pos] ) {
							break;
						}
					}
					for ( move_from_pos = 0; move_from_pos < MAX_MODULES_ON_SHIPS; move_from_pos++ ) {
						if ( grabbed_module_id == PlayerShip->ModulesPowerQueue[move_from_pos] ) {
							break;
						}
					}
					while ( move_from_pos > move_to_pos ) {
						PlayerShip->MoveUpModuleInModulesPowerQueue(grabbed_module_id);
						move_from_pos--;
					}
					while ( move_from_pos < move_to_pos ) {
						PlayerShip->MoveDownModuleInModulesPowerQueue(grabbed_module_id);
						move_from_pos++;
					}
					if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
						CaptainGui->multiselect_box_active = false;
						CaptainGui->EndMultiselectBox();
					}
					grabbed_module_exists = false;
					grabbed_module_id = 0;
					PlayerShip->AutoManageModulePowerlevels();
					UpdateModulesPowerQueueOnCaptainGui();
					StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
		if (token.Icmp("clear_grabbed_module") == 0) {
			if ( grabbed_module_exists ) {
				grabbed_module_exists = false;
				grabbed_module_id = 0;
				UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				UpdateModulesPowerQueueOnCaptainGui();
				StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
				//gameLocal.Printf( "clear grabbed module\n" );
			}
			return true;
		}
	}
// GRAB RESERVE CREW MEMBERS
	if ( PlayerShip ) {
		for ( int i = 0; i < PlayerShip->reserve_Crew.size(); i++ ) {
			if (token.Icmp( va("grab_reserve_crew_%i",i) ) == 0) {
				CheckForPlayerShipCrewMemberNameChanges();
				grabbed_reserve_crew_dict = &PlayerShip->reserve_Crew[i];
				selected_reserve_crew_dict = &PlayerShip->reserve_Crew[i];
				PlayerShip->ClearCrewMemberSelection();
				PlayerShip->ClearCrewMemberMultiSelection();
				if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
					CaptainGui->multiselect_box_active = false;
					CaptainGui->EndMultiselectBox();
				}
				StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
// GRAB CREW MEMBERS
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if (token.Icmp("grab_" + role_description[i] + "_officer") == 0) {
				CheckForPlayerShipCrewMemberNameChanges();
				if ( PlayerShip->crew[i] ) {
					grabbed_crew_member = PlayerShip->crew[i];
					grabbed_crew_member_id = i;
					selected_reserve_crew_dict = NULL;
					if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
						CaptainGui->multiselect_box_active = false;
						CaptainGui->EndMultiselectBox();
					}
					StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
	}
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if (token.Icmp("swap_grabbed_officer_with_" + role_description[i] + "_officer") == 0) {
				CheckForPlayerShipCrewMemberNameChanges();
				if ( /*PlayerShip->crew[i] &&*/ grabbed_crew_member ) {
					idAI* temp = PlayerShip->crew[i];
					PlayerShip->crew[i] = grabbed_crew_member;
					PlayerShip->crew[grabbed_crew_member_id] = temp;
					SyncUpPlayerShipNameCVars();
					grabbed_crew_member = NULL;
					grabbed_crew_member_id = 0;
					if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
						CaptainGui->multiselect_box_active = false;
						CaptainGui->EndMultiselectBox();
					}
					StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else if ( grabbed_reserve_crew_dict ) {
					bool hostile_entities_present = PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition();
					bool hostile_entities_on_board = PlayerShip->CheckForHostileEntitiesOnBoard();
					if ( PlayerShip->crew[i] == NULL && !hostile_entities_present && !hostile_entities_on_board ) {
						if ( grabbed_reserve_crew_dict->GetNumKeyVals() > 0 ) {
							if ( PlayerShip->TransporterBounds && PlayerShip->TransporterBounds->GetPhysics() ) {
								idVec3 default_spawn_point = PlayerShip->TransporterBounds->GetPhysics()->GetAbsBounds().GetCenter();
								default_spawn_point.z = PlayerShip->TransporterBounds->GetPhysics()->GetAbsBounds()[0][2];
								grabbed_reserve_crew_dict->Set( "origin", default_spawn_point.ToString() );
								PlayerShip->TriggerShipTransporterPadFX();
							} else if ( i < MAX_ROOMS_ON_SHIPS && PlayerShip->room_node[i] ) {
								grabbed_reserve_crew_dict->Set( "origin", PlayerShip->room_node[i]->GetPhysics()->GetOrigin().ToString() );
							} else {
								return false;
							}
							grabbed_reserve_crew_dict->SetInt( "team", team );
							PlayerShip->crew[i] = (idAI*)PlayerShip->SpawnShipInteriorEntityFromAdjustedidDict( *grabbed_reserve_crew_dict );
							SyncUpPlayerShipNameCVars();
							PlayerShip->AIsOnBoard.push_back(PlayerShip->crew[i]); // add the ai to the list of AI's on board this ship
							PlayerShip->crew[i]->ShipOnBoard = PlayerShip;
							PlayerShip->crew[i]->ParentShip = PlayerShip;

							if ( PlayerShip->TransporterBounds && PlayerShip->TransporterBounds->GetPhysics() ) {
								// KEEP TRYING RANDOM ORIGINS WITH THE TRANSPORTER BOUNDS UNTIL WE ARE NOT WITHIN THE BOUNDS OF ANOTHER AI
								gameLocal.GetSuitableTransporterPositionWithinBounds(PlayerShip->crew[i],&PlayerShip->TransporterBounds->GetPhysics()->GetAbsBounds());
							}

							PlayerShip->crew[i]->TriggerTransporterFX(PlayerShip->spawnArgs.GetString("transporter_actor_emitter_def", "transporter_actor_emitter_def_default"));
							//PlayerShip->crew[i]->BeginTransporterMaterialShaderEffect(PlayerShip->fx_color_theme); // BOYETTE NOTE TODO IMPORTANT: we would need to make a function called BeginREVERSETransporterMaterialShaderEffect - it is not necessary right now - maybe later

							if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
								CaptainGui->multiselect_box_active = false;
								CaptainGui->EndMultiselectBox();
							}

							//PlayerShip->reserve_Crew.erase(std::remove(PlayerShip->reserve_Crew.begin(), PlayerShip->reserve_Crew.end(), &grabbed_reserve_crew_dict), PlayerShip->reserve_Crew.end());
							for ( int vec_index = 0; vec_index < PlayerShip->reserve_Crew.size(); vec_index++ ) {
								if ( &PlayerShip->reserve_Crew[vec_index] == grabbed_reserve_crew_dict ) {
									PlayerShip->reserve_Crew.erase(PlayerShip->reserve_Crew.begin()+vec_index);
								}
							}
							UpdateReserveCrewMemberPortraits();

							grabbed_reserve_crew_dict = NULL;
							selected_reserve_crew_dict = NULL;
							PlayerShip->ClearCrewMemberSelection();
							PlayerShip->ClearCrewMemberMultiSelection();
							PlayerShip->SetSelectedCrewMember(PlayerShip->crew[i]);

							DisplayNonInteractiveAlertMessage("Crew station assigned. Transport Incoming.");
							StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
							return true;
						}
					} else {
						grabbed_reserve_crew_dict = NULL;
						selected_reserve_crew_dict = NULL;
						PlayerShip->ClearCrewMemberSelection();
						PlayerShip->ClearCrewMemberMultiSelection();
						if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
							CaptainGui->multiselect_box_active = false;
							CaptainGui->EndMultiselectBox();
						}
						if ( PlayerShip->crew[i] != NULL ) {
							DisplayNonInteractiveAlertMessage("There is already crew assigned to this position.");
						} else if ( hostile_entities_present ) {
							DisplayNonInteractiveAlertMessage("You cannot utilize reserve crew during battle."); //when there is a hostile entity in range.");
						} else if ( hostile_entities_on_board ) {
							DisplayNonInteractiveAlertMessage("You cannot utilize reserve when there are hostiles on board."); //when there is a hostile entity in range.");
						}
						StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				}
			}
		}
	}
	if (token.Icmp("clear_grabbed_crew") == 0) {
		CheckForPlayerShipCrewMemberNameChanges();
		grabbed_reserve_crew_dict = NULL;
		grabbed_crew_member = NULL;
		grabbed_crew_member_id = 0;
		StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
		return true;
	}
// REMOVE CREW MEMBERS
	if ( PlayerShip ) {
		if (token.Icmp("playership_remove_selected_reserve_crew") == 0) {
			CheckForPlayerShipCrewMemberNameChanges();
			if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
				CaptainGui->multiselect_box_active = false;
				CaptainGui->EndMultiselectBox();
			}
			if ( selected_reserve_crew_dict && selected_reserve_crew_dict->GetNumKeyVals() > 0 ) {
				DisplayNonInteractiveAlertMessage( selected_reserve_crew_dict->GetString( "npc_name", "Joe" ) + idStr(" was removed from reserve crew.") );
				for ( int vec_index = 0; vec_index < PlayerShip->reserve_Crew.size(); vec_index++ ) {
					if ( &PlayerShip->reserve_Crew[vec_index] == selected_reserve_crew_dict ) {
						PlayerShip->reserve_Crew.erase(PlayerShip->reserve_Crew.begin()+vec_index);
					}
				}
				grabbed_reserve_crew_dict = NULL;
				selected_reserve_crew_dict = NULL;
				UpdateReserveCrewMemberPortraits();
				StartSoundShader( declManager->FindSound("space_command_guisounds_close_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else {
				DisplayNonInteractiveAlertMessage( "No reserve crew selected." );
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
			}
		}
	}
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if (token.Icmp("playership_rename_" + role_description[i] + "_officer") == 0) {
				if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
					CaptainGui->multiselect_box_active = false;
					CaptainGui->EndMultiselectBox();
				}
				//CheckForPlayerShipCrewMemberNameChanges();
				if ( PlayerShip->crew[i] ) {
					//if ( guiOverlay ) {
					//	guiOverlay->HandleNamedEvent( "cvar read " );
					//}

					//PlayerShip->crew[i]->spawnArgs.Set( "npc_name", sc_medical_officer_name.GetString() );
					//gameLocal.Printf( role_description[i] + "_officer name changed \n" );

					//PlayerShip->crew[i]->npc_name_locked = true;
						//.GetString("npc_name", "This being")

					//if ( guiOverlay ) {
					//	guiOverlay->HandleNamedEvent( "cvar write " );
					//}
					return true;
				}
			}
		}
	}
// REMOVE CREW MEMBERS
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if (token.Icmp("playership_remove_" + role_description[i] + "_officer") == 0) {
				CheckForPlayerShipCrewMemberNameChanges();
				bool hostile_entities_present = PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition();
				bool hostile_entities_on_board = PlayerShip->CheckForHostileEntitiesOnBoard();
				if ( PlayerShip->crew[i] && !hostile_entities_present && !hostile_entities_on_board ) {
					//PlayerShip->crew[i]->Event_Remove();
					//PlayerShip->crew[i] = NULL;
					confirm_removal_crew_member_id = i;
					grabbed_crew_member = NULL; // just in case it is the grabbed crew member
					grabbed_crew_member_id = 0; // just in case it is the grabbed crew member
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForRemovingCrewMember");
					}
					StartSoundShader( declManager->FindSound("space_command_guisounds_close_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else {
					if ( !PlayerShip->crew[i] ) {
						DisplayNonInteractiveAlertMessage("There is no crew assigned to this position.");
					} else if ( hostile_entities_present ) {
						DisplayNonInteractiveAlertMessage("You cannot remove crew during battle."); //when there is a hostile entity in range.");
					} else if ( hostile_entities_on_board ) {
						DisplayNonInteractiveAlertMessage("You cannot remove crew when there are hostiles on board."); //when there is a hostile entity in range.");
					}
				}
			}
		}
		if (token.Icmp("playership_confirm_removal_of_officer") == 0) {
			CheckForPlayerShipCrewMemberNameChanges();
			bool hostile_entities_present = PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition();
			bool hostile_entities_on_board = PlayerShip->CheckForHostileEntitiesOnBoard();
			if ( PlayerShip->crew[confirm_removal_crew_member_id] ) {
				PlayerShip->crew[confirm_removal_crew_member_id]->Event_Remove();
				PlayerShip->crew[confirm_removal_crew_member_id] = NULL;
				grabbed_crew_member = NULL; // just in case it is the grabbed crew member
				grabbed_crew_member_id = 0; // just in case it is the grabbed crew member
				StartSoundShader( declManager->FindSound("space_command_guisounds_warning_02") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else {
				if ( !PlayerShip->crew[confirm_removal_crew_member_id] ) {
					DisplayNonInteractiveAlertMessage("There is no crew assigned to this position.");
				} else if ( hostile_entities_present ) {
					DisplayNonInteractiveAlertMessage("You cannot remove crew during battle."); //when there is a hostile entity in range.");
				} else if ( hostile_entities_on_board ) {
					DisplayNonInteractiveAlertMessage("You cannot remove crew when there are hostiles on board."); //when there is a hostile entity in range.");
				}
			}
		}
	}
// SELECT RESERVE CREW MEMBERS - we just do it in grab

// SELECT CREW MEMBERS
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if (token.Icmp("select_" + role_description[i] + "_officer") == 0) {
				CheckForPlayerShipCrewMemberNameChanges();
				if ( PlayerShip->crew[i] ) {
					if ( common->ShiftKeyIsDown() && PlayerShip->SelectedCrewMember ) {
						PlayerShip->AddCrewMemberToSelection(PlayerShip->SelectedCrewMember);
						PlayerShip->SelectedCrewMember = NULL;
						PlayerShip->AddCrewMemberToSelection(PlayerShip->crew[i]);
						selected_reserve_crew_dict = NULL;
						StartSoundShader( declManager->FindSound("space_command_guisounds_beep_01") , SND_CHANNEL_ANY, 0, false, NULL );
					} else if ( common->ShiftKeyIsDown() && PlayerShip->SelectedCrewMembers.size() > 0 ) {
						PlayerShip->AddCrewMemberToSelection(PlayerShip->crew[i]);
						selected_reserve_crew_dict = NULL;
						StartSoundShader( declManager->FindSound("space_command_guisounds_beep_01") , SND_CHANNEL_ANY, 0, false, NULL );
					} else {
						PlayerShip->SetSelectedCrewMember(PlayerShip->crew[i]);
						selected_reserve_crew_dict = NULL;
						StartSoundShader( declManager->FindSound("space_command_guisounds_beep_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				}
			}
		}
	}
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if (token.Icmp("multiselect_add_" + role_description[i] + "_officer_to_selection") == 0) {
				CheckForPlayerShipCrewMemberNameChanges();
				if ( PlayerShip->crew[i] ) {
					PlayerShip->AddCrewMemberToSelection(PlayerShip->crew[i]);
					selected_reserve_crew_dict = NULL;
					StartSoundShader( declManager->FindSound("space_command_guisounds_beep_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
	}

// CLEAR CREW MEMBER SELECTION
	if (token.Icmp("clear_crew_selection") == 0) {
		if ( common->ShiftKeyIsDown() ) {
		} else {
			if ( PlayerShip ) {
				CheckForPlayerShipCrewMemberNameChanges();
				PlayerShip->ClearCrewMemberSelection();
				selected_reserve_crew_dict = NULL;
				StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
			}
		}
	}
	if (token.Icmp("clear_crew_multiselection") == 0) {
		if ( common->ShiftKeyIsDown() ) {
		} else {
			if ( PlayerShip ) {
				CheckForPlayerShipCrewMemberNameChanges();
				PlayerShip->ClearCrewMemberMultiSelection();
				selected_reserve_crew_dict = NULL;
				StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
			}
		}
	}

// CREW AUTO_MODE AND PLAYER_FOLLOW_MODE TOGGLES
	if ( PlayerShip ) {
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if (token.Icmp("click_toggle_" + role_description[i] + "_officer_auto_mode") == 0) {
				if ( PlayerShip->crew[i] && PlayerShip->crew[i]->crew_auto_mode_activated ) {
					PlayerShip->crew[i]->crew_auto_mode_activated = false;
					PlayerShip->crew[i]->player_follow_mode_activated = false;
					StartSoundShader( declManager->FindSound("space_command_guisounds_static_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else if ( PlayerShip->crew[i] ) {
					PlayerShip->crew[i]->crew_auto_mode_activated = true;
					PlayerShip->crew[i]->handling_emergency_oxygen_situation = false;
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_02") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}

		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if (token.Icmp("click_toggle_" + role_description[i] + "_officer_player_follow_mode") == 0) {
				if ( PlayerShip->crew[i] && PlayerShip->crew[i]->player_follow_mode_activated ) {
					PlayerShip->crew[i]->player_follow_mode_activated = false;
					StartSoundShader( declManager->FindSound("space_command_guisounds_static_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else if ( PlayerShip->crew[i] ) {
					PlayerShip->crew[i]->player_follow_mode_activated = true;
					PlayerShip->crew[i]->crew_auto_mode_activated = true;
					PlayerShip->crew[i]->handling_emergency_oxygen_situation = false;
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_02") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
	}

// SELECT SHIP MODULES
	if (token.Icmp("select_medical_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[MEDICALMODULEID] && PlayerShip->consoles[MEDICALMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[MEDICALMODULEID]->ControlledModule);
			CaptainGui->SetCursorImage(0);
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("select_engines_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[ENGINESMODULEID] && PlayerShip->consoles[ENGINESMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[ENGINESMODULEID]->ControlledModule);
			CaptainGui->SetCursorImage(0);
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("select_weapons_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[WEAPONSMODULEID] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule ) {
			if ( PlayerShip->TargetEntityInSpace && team == PlayerShip->TargetEntityInSpace->team ) {
				if ( CaptainGui ) {
					CaptainGui->HandleNamedEvent("GetConfirmationForSelectingWeaponsModuleAgainstFriendlyEntity");
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			} else if ( PlayerShip->TargetEntityInSpace && PlayerShip->HasNeutralityWithShip( PlayerShip->TargetEntityInSpace ) && !PlayerShip->TargetEntityInSpace->is_derelict ) {
				if ( CaptainGui ) {
					CaptainGui->HandleNamedEvent("GetConfirmationForSelectingWeaponsModuleAgainstNonHostileEntity");
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			} else {
				PlayerShip->SetSelectedModule(PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule);
				// maybe turn weapons auto fire off when the weapons module is selected.
				PlayerShip->weapons_autofire_on = false; // BOYETTE NOTE TODO: send an event log out so that the player knows it has been turned off.
				UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				// change cursor to weapon targeting reticule
				StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
				CaptainGui->SetCursorImage(2);
				return true;
			}
		}
	}
	if (token.Icmp("confirm_select_weapons_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[WEAPONSMODULEID] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule);
			// maybe turn weapons auto fire off when the weapons module is selected.
			PlayerShip->weapons_autofire_on = false; // BOYETTE NOTE TODO: send an event log out so that the player knows it has been turned off.
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
			// change cursor to weapon targeting reticule
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			CaptainGui->SetCursorImage(2);
			return true;
		}
	}
	if (token.Icmp("select_torpedos_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[TORPEDOSMODULEID] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule ) {
			if ( PlayerShip->TargetEntityInSpace && team == PlayerShip->TargetEntityInSpace->team ) {
				if ( CaptainGui ) {
					CaptainGui->HandleNamedEvent("GetConfirmationForSelectingTorpedosModuleAgainstFriendlyEntity");
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			} else if ( PlayerShip->TargetEntityInSpace && PlayerShip->HasNeutralityWithShip( PlayerShip->TargetEntityInSpace ) && !PlayerShip->TargetEntityInSpace->is_derelict ) {
				if ( CaptainGui ) {
					CaptainGui->HandleNamedEvent("GetConfirmationForSelectingTorpedosModuleAgainstNonHostileEntity");
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			} else {
				PlayerShip->SetSelectedModule(PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule);
				// maybe turn torpedos auto fire off when the torpedos module is selected.
				PlayerShip->torpedos_autofire_on = false; // BOYETTE NOTE TODO:  send an event log out so that the player knows it has been turned off.
				UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				// change cursor to torpedo targeting reticule
				CaptainGui->SetCursorImage(3);
				StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
	if (token.Icmp("confirm_select_torpedos_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[TORPEDOSMODULEID] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule);
			// maybe turn torpedos auto fire off when the torpedos module is selected.
			PlayerShip->torpedos_autofire_on = false; // BOYETTE NOTE TODO:  send an event log out so that the player knows it has been turned off.
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
			// change cursor to torpedo targeting reticule
			CaptainGui->SetCursorImage(3);
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("select_shields_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[SHIELDSMODULEID] && PlayerShip->consoles[SHIELDSMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[SHIELDSMODULEID]->ControlledModule);
			CaptainGui->SetCursorImage(0);
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("select_sensors_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[SENSORSMODULEID] && PlayerShip->consoles[SENSORSMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[SENSORSMODULEID]->ControlledModule);
			CaptainGui->SetCursorImage(0);
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("select_environment_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[ENVIRONMENTMODULEID] && PlayerShip->consoles[ENVIRONMENTMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[ENVIRONMENTMODULEID]->ControlledModule);
			CaptainGui->SetCursorImage(0);
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("select_computer_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[COMPUTERMODULEID] && PlayerShip->consoles[COMPUTERMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[COMPUTERMODULEID]->ControlledModule);
			CaptainGui->SetCursorImage(0);
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("select_security_module") == 0) {
		if ( PlayerShip && PlayerShip->consoles[SECURITYMODULEID] && PlayerShip->consoles[SECURITYMODULEID]->ControlledModule ) {
			PlayerShip->SetSelectedModule(PlayerShip->consoles[SECURITYMODULEID]->ControlledModule);
			CaptainGui->SetCursorImage(0);
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}

// CLEAR SHIP MODULE SELECTION
	if (token.Icmp("clear_ship_module_selection") == 0) {
		if ( PlayerShip ) {
			PlayerShip->ClearModuleSelection();
			if ( CaptainGui ) {
				CaptainGui->SetCursorImage(1);
			}
			StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
		}
	}

/* // We could do this, but I think it would be better to just have an auto mode on the weapons and torpedos modules. This way if they really do just want to focus fire on a module they will be able to do that.
// Idea I like: maybe the best thing to do would be to say: if it is set to auto. move the module to the top of the queue. If it is not set to auto, then just make it the target module and forget about the queue.
// Idea I like: or we will always move it to the top of the queue but only follow the queue if it is set to auto.
if ( PlayerShip->WeaponsConsole && PlayerShip->WeaponsConsole->ControlledModule && PlayerShip->SelectedModule == PlayerShip->WeaponsConsole->ControlledModule ) {
	// move medical module up in the weapons queue until it is first in the queue.
} else if ( PlayerShip->TorpedosConsole && PlayerShip->TorpedosConsole->ControlledModule &&  PlayerShip->SelectedModule == PlayerShip->TorpedosConsole->ControlledModule
	// move medical module up in the torpedos queue until it is first in the queue.
}
*/
// SET CURRENT TARGET MODULE ON TARGETSHIP // still need to reset the cursor after one of these are clicked on.
	if ( PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->SelectedModule ) {
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if (token.Icmp("click_set_targetship_" + module_description[i] + "_module_as_target") == 0) {
				if ( PlayerShip->TargetEntityInSpace->consoles[i] && PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule ) {
					PlayerShip->SelectedModule->CurrentTargetModule = PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule;
					PlayerShip->SelectedModule = NULL; // this way we won't accidently give a module another command after we have already given it one.
					CaptainGui->SetCursorImage(0);
					StartSoundShader( declManager->FindSound("space_command_guisounds_ping_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
	}

// CLEAR CURRENTLY TARGET MODULE ON TARGETSHIP
	if (token.Icmp("clear_currently_selected_ship_module_target") == 0) {
		if ( PlayerShip && PlayerShip->SelectedModule ) {
			PlayerShip->SelectedModule->CurrentTargetModule = NULL;
			CaptainGui->SetCursorImage(0);
			StartSoundShader( declManager->FindSound("space_command_guisounds_ping_low_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}

// CEASE FIRING WEAPONS AND TORPEDOS
	if (token.Icmp("click_cease_firing_weapons_and_torpedos") == 0) {
		if ( PlayerShip ) {
			PlayerShip->CeaseFiringWeaponsAndTorpedos();
			if ( PlayerShip->TargetEntityInSpace ) {
				PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship = true;
			}
			StartSoundShader( declManager->FindSound("space_command_guisounds_disconnect_01") , SND_CHANNEL_ANY, 0, false, NULL );
		}
	}

	if (token.Icmp("click_shiplist") == 0) {
		SetSelectedEntityFromList();
		UpdateSelectedEntityInSpaceOnGuis();
		StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
		return true;
	}
	if (token.Icmp("click_set_selected_ship_as_targetship") == 0) {
		if ( SelectedEntityInSpace && SelectedEntityInSpace->IsType(sbShip::Type) && !SelectedEntityInSpace->IsType(sbStationarySpaceEntity::Type) ) {
			PlayerShip->SetTargetEntityInSpace( dynamic_cast<sbShip*>( SelectedEntityInSpace ) );
		} else if ( SelectedEntityInSpace && SelectedEntityInSpace->IsType(sbStationarySpaceEntity::Type) ) {
			PlayerShip->SetTargetEntityInSpace( dynamic_cast<sbStationarySpaceEntity*>( SelectedEntityInSpace ) );
		}
		// boyette note TODO: should clear all module targets when a new ship or planet is targeted. This should help prevent us from accidentally firing on a friendly ship or planet.
		// ClearAllModuleTargets();
		UpdateCaptainMenu();
		StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
		return true;
	}
	if (token.Icmp("click_hail_selected_ship") == 0) {
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( !SelectedEntityInSpace->is_derelict ) { // if the ship is not derelict
				//SetOverlayHailGui();
				SelectedEntityInSpace->OpenHailWithLocalPlayer();
				StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else {
				DisplayNonInteractiveAlertMessage("The ship is derelict. There is no one to talk to.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
//		UpdateHudStats(guiOverlay);
	}

// ENGINEERING TAB UPGRADES BEGIN:

	if ( PlayerShip ) {

		// TODO MUST: here need to check for other hostile ships at the same stargrid position here before doing anything
		bool hostile_ships_present = false;
		bool hostile_entities_on_board = false;
		if ( PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition() ) {
			hostile_ships_present = true;
		} else if ( PlayerShip->CheckForHostileEntitiesOnBoard() ) {
			hostile_entities_on_board = true;
		}

		if ( token.Icmp("click_buy_phaedrus_pistol") == 0 ) {
			if ( hostile_ships_present ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( hostile_entities_on_board ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( PlayerShip->current_materials_reserves >= 1000 && PlayerShip->current_currency_reserves >= 1000 ) {
				Give( "weapon", "weapon_phaedrus_pistol" );
				PlayerShip->current_materials_reserves -= 1000;
				PlayerShip->current_currency_reserves -= 1000;
				StartSoundShader( declManager->FindSound("sound_weapon_acquire_01") , SND_CHANNEL_ANY, 0, false, NULL );
				UpdateCaptainMenu();
				return true;
			} else {
				DisplayNonInteractiveAlertMessage("You do not have enough resources to create a Phaedrus Pistol.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
		if ( token.Icmp("click_buy_laser_shotgun") == 0 ) {
			if ( hostile_ships_present ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( hostile_entities_on_board ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( PlayerShip->current_materials_reserves >= 2000 && PlayerShip->current_currency_reserves >= 2000 ) {
				Give( "weapon", "weapon_laser_shotgun" );
				PlayerShip->current_materials_reserves -= 2000;
				PlayerShip->current_currency_reserves -= 2000;
				StartSoundShader( declManager->FindSound("sound_weapon_acquire_01") , SND_CHANNEL_ANY, 0, false, NULL );
				UpdateCaptainMenu();
				return true;
			} else {
				DisplayNonInteractiveAlertMessage("You do not have enough resources to create a Laser Shotgun.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
		if ( token.Icmp("click_buy_laser_rifle") == 0 ) {
			if ( hostile_ships_present ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( hostile_entities_on_board ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( PlayerShip->current_materials_reserves >= 4000 && PlayerShip->current_currency_reserves >= 4000 ) {
				Give( "weapon", "weapon_laser_rifle" );
				PlayerShip->current_materials_reserves -= 4000;
				PlayerShip->current_currency_reserves -= 4000;
				StartSoundShader( declManager->FindSound("sound_weapon_acquire_01") , SND_CHANNEL_ANY, 0, false, NULL );
				UpdateCaptainMenu();
				return true;
			} else {
				DisplayNonInteractiveAlertMessage("You do not have enough resources to create a Laser Rifle.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
		if ( token.Icmp("click_buy_energy_cells") == 0 ) {
			if ( hostile_ships_present ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( hostile_entities_on_board ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( PlayerShip->current_materials_reserves >= 500 && PlayerShip->current_currency_reserves >= 500 ) {
				if ( Give( "ammo_bullets", "50" ) ) {
					PlayerShip->current_materials_reserves -= 500;
					PlayerShip->current_currency_reserves -= 500;
					StartSoundShader( declManager->FindSound("sound_weapon_acquire_01") , SND_CHANNEL_ANY, 0, false, NULL );
					UpdateCaptainMenu();
					return true;
				} else {
					DisplayNonInteractiveAlertMessage("You do not have enough space in your inventory for more Energy Cells.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			} else {
				DisplayNonInteractiveAlertMessage("You cannot do not have enough resources to create Energy Cells.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
		if ( token.Icmp("click_buy_body_armor") == 0 ) {
			if ( hostile_ships_present ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( hostile_entities_on_board ) {
				DisplayNonInteractiveAlertMessage("You cannot use the workbench when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( PlayerShip->current_materials_reserves >= 500 && PlayerShip->current_currency_reserves >= 500 ) {
				if ( Give( "armor", "20" ) ) {
					PlayerShip->current_materials_reserves -= 500;
					PlayerShip->current_currency_reserves -= 500;
					StartSoundShader( declManager->FindSound("sound_acquire_armor") , SND_CHANNEL_ANY, 0, false, NULL );
					UpdateCaptainMenu();
					return true;
				} else {
					DisplayNonInteractiveAlertMessage("You cannot wear any more armor.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			} else {
				DisplayNonInteractiveAlertMessage("You do not have enough resources to create Armor.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}

		if ( token.Icmp("click_repair_hull") == 0 ) {
			if ( hostile_ships_present ) {
				DisplayNonInteractiveAlertMessage("You cannot repair your ship when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( hostile_entities_on_board ) {
				DisplayNonInteractiveAlertMessage("You cannot repair your ship when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else {
				if ( PlayerShip->hullStrength >= PlayerShip->max_hullStrength ) {
					DisplayNonInteractiveAlertMessage("The ship hull is fully repaired.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else if ( PlayerShip->current_materials_reserves >= 100 ) {
					PlayerShip->RepairShipWithMaterials(100);
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_02") , SND_CHANNEL_ANY, 0, false, NULL );
					UpdateCaptainHudOnce();
					UpdateCaptainMenu();
					UpdateHailGui();
					return true;
				} else {
					DisplayNonInteractiveAlertMessage("You do not have enough materials to repair the hull.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
		if (token.Icmp("click_upgrade_hull") == 0) {
			if ( hostile_ships_present ) {
				DisplayNonInteractiveAlertMessage("You cannot upgrade your ship when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( hostile_entities_on_board ) {
				DisplayNonInteractiveAlertMessage("You cannot upgrade your ship when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else {
				if ( PlayerShip->max_hullStrength + 100 > MAX_MAX_HULLSTRENGTH ) {
					DisplayNonInteractiveAlertMessage("The ship hull is fully upgraded.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else if ( PlayerShip->current_materials_reserves >= 500 ) {
					PlayerShip->current_materials_reserves -= 500;
					PlayerShip->max_hullStrength = PlayerShip->max_hullStrength + 100;
					PlayerShip->hullStrength = PlayerShip->hullStrength + 100;
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_02") , SND_CHANNEL_ANY, 0, false, NULL );
					UpdateCaptainHudOnce();
					UpdateCaptainMenu();
					UpdateHailGui();
					return true;
				} else {
					DisplayNonInteractiveAlertMessage("You do not have enough materials to upgrade the hull.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
		if (token.Icmp("click_upgrade_power_reserve") == 0) {
			if ( hostile_ships_present ) {
				DisplayNonInteractiveAlertMessage("You cannot upgrade your ship when there is a hostile entity in range.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else if ( hostile_entities_on_board ) {
				DisplayNonInteractiveAlertMessage("You cannot upgrade your ship when there are hostile entities on board.");
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			} else {
				if ( PlayerShip->maximum_power_reserve + 1 > MAX_MAX_POWER_RESERVE ) {
					DisplayNonInteractiveAlertMessage("The power reserve is fully upgraded.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else if ( PlayerShip->current_materials_reserves >= 800 ) {
					PlayerShip->current_materials_reserves -= 800;
					PlayerShip->maximum_power_reserve = PlayerShip->maximum_power_reserve + 1;
					PlayerShip->current_power_reserve = PlayerShip->current_power_reserve + 1;
					PlayerShip->current_automanage_power_reserve = PlayerShip->current_automanage_power_reserve + 1;
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_02") , SND_CHANNEL_ANY, 0, false, NULL );
					if ( PlayerShip->modules_power_automanage_on ) {
						PlayerShip->AutoManageModulePowerlevels();
						UpdateModulesPowerQueueOnCaptainGui();
					}
					// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
					if ( PlayerShip->maximum_power_reserve == MAX_MAX_POWER_RESERVE ) {
						if ( common->m_pStatsAndAchievements ) {
							if ( !common->m_pStatsAndAchievements->m_nTimesUpgradedShipToMaxPowerReserve ) {
								common->m_pStatsAndAchievements->m_nTimesUpgradedShipToMaxPowerReserve++;
								common->StoreSteamStats();
							}
						}
					}
#endif
					// BOYETTE STEAM INTEGRATION END
					UpdateCaptainHudOnce();
					UpdateCaptainMenu();
					UpdateHailGui();
					return true;
				} else {
					DisplayNonInteractiveAlertMessage("You do not have enough materials to upgrade the power reserve.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}

		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			if (token.Icmp("click_upgrade_" + module_description[i] + "_module_power") == 0) {
				if ( hostile_ships_present ) {
					DisplayNonInteractiveAlertMessage("You cannot upgrade your ship when there is a hostile entity in range.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else if ( hostile_entities_on_board ) {
					DisplayNonInteractiveAlertMessage("You cannot upgrade your ship when there are hostile entities on board.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				} else {
					if ( PlayerShip->consoles[i] && PlayerShip->consoles[i]->ControlledModule ) {
						int module_upgrade_cost = 100;
						if ( i == MEDICALMODULEID ) {
							module_upgrade_cost = 900;
						} else if ( i == ENGINESMODULEID ) {
							module_upgrade_cost = 1000;
						} else if ( i == WEAPONSMODULEID ) {
							module_upgrade_cost = 2000;
						} else if ( i == TORPEDOSMODULEID ) {
							module_upgrade_cost = 1500;
						} else if ( i == SHIELDSMODULEID ) {
							module_upgrade_cost = 2200;
						} else if ( i == SENSORSMODULEID ) {
							module_upgrade_cost = 400;
						} else if ( i == ENVIRONMENTMODULEID ) {
							module_upgrade_cost = 700;
						} else if ( i == COMPUTERMODULEID ) {
							module_upgrade_cost = 1400;
						} else if ( i == SECURITYMODULEID ) {
							module_upgrade_cost = 800;
						}
						if ( PlayerShip->consoles[i]->ControlledModule->max_power + 1 > MAX_MAX_MODULE_POWER ) {
							DisplayNonInteractiveAlertMessage("This module is fully upgraded.");
							StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
							return true;
						} else if ( PlayerShip->current_materials_reserves >= module_upgrade_cost ) {
							PlayerShip->current_materials_reserves -= module_upgrade_cost;
							PlayerShip->consoles[i]->ControlledModule->max_power = PlayerShip->consoles[i]->ControlledModule->max_power + 1;
							PlayerShip->module_max_powers[i] = PlayerShip->consoles[i]->ControlledModule->max_power;
							PlayerShip->spawnArgs.SetInt( module_description[i] + "_module_max_power", PlayerShip->module_max_powers[i] );
							PlayerShip->UpdateShipAttributes();
							StartSoundShader( declManager->FindSound("space_command_guisounds_click_02") , SND_CHANNEL_ANY, 0, false, NULL );
							if ( PlayerShip->modules_power_automanage_on ) {
								PlayerShip->AutoManageModulePowerlevels();
								UpdateModulesPowerQueueOnCaptainGui();
							}
							UpdateCaptainHudOnce();
							UpdateCaptainMenu();
							UpdateHailGui();
							return true;
						} else {
							DisplayNonInteractiveAlertMessage("You do not have enough materials to upgrade this module.");
							StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
							return true;
						}
					}
				}
			}
		}


	}

// ENGINEERING TAB UPGRADES END

// INCREASE/DECREASE MODULES POWER LEVELS
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_increase_" + module_description[i] + "_module_power") == 0) {
			if ( PlayerShip && PlayerShip->consoles[i] ) {
				if ( PlayerShip->consoles[i]->ControlledModule->power_allocated < PlayerShip->consoles[i]->ControlledModule->max_power && PlayerShip->current_power_reserve > 0 ) {
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
				} else {
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				}
				PlayerShip->IncreaseModulePower(PlayerShip->consoles[i]->ControlledModule);
				PlayerShip->modules_power_automanage_on = false;
				UpdateModulesPowerQueueOnCaptainGui();
				return true; // this makes it so that the HandleSingleGuiCommand only gets called once - otherwise it will get called twice and power will increase double.
			}
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_decrease_" + module_description[i] + "_module_power") == 0) {
			if ( PlayerShip && PlayerShip->consoles[i] ) {
				if ( PlayerShip->consoles[i]->ControlledModule->power_allocated == 0 ) {
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				} else {
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
				}
				PlayerShip->DecreaseModulePower(PlayerShip->consoles[i]->ControlledModule);
				PlayerShip->modules_power_automanage_on = false;
				UpdateModulesPowerQueueOnCaptainGui();
				return true; // this makes it so that the HandleSingleGuiCommand only gets called once - otherwise it will get called twice and power will increase double.
			}
		}
	}

// TRANSPORTER MISCELLANEOUS
	if (token.Icmp("click_initiate_transporter") == 0) {
		if ( PlayerShip && PlayerShip->TransporterBounds && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->TransporterBounds ) {
			PlayerShip->BeginTransporterSequence();
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true; // this makes it so that the HandleSingleGuiCommand only gets called once - otherwise it will get called twice and power will increase double.
		}
	}

	if (token.Icmp("click_initiate_retrieval_transport") == 0) {
		if ( PlayerShip && PlayerShip->TransporterBounds && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->TransporterBounds ) {
			PlayerShip->BeginRetrievalTransportSequence();
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true; // this makes it so that the HandleSingleGuiCommand only gets called once - otherwise it will get called twice and power will increase double.
		}
	}
// INCREASE/DECREASE MODULES IN WEAPONS QUEUE
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_increase_" + module_description[i] + "_module_priority_in_weapons_queue") == 0) {
			if ( PlayerShip ) {
				PlayerShip->MoveUpModuleInWeaponsTargetQueue(i);
				UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				StartSoundShader( declManager->FindSound("space_command_guisounds_beep_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_decrease_" + module_description[i] + "_module_priority_in_weapons_queue") == 0) {
			if ( PlayerShip ) {
				PlayerShip->MoveDownModuleInWeaponsTargetQueue(i);
				UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
// INCREASE/DECREASE MODULES IN TORPEDOS QUEUE
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_increase_" + module_description[i] + "_module_priority_in_torpedos_queue") == 0) {
			if ( PlayerShip ) {
				PlayerShip->MoveUpModuleInTorpedosTargetQueue(i);
				UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				StartSoundShader( declManager->FindSound("space_command_guisounds_beep_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_decrease_" + module_description[i] + "_module_priority_in_torpedos_queue") == 0) {
			if ( PlayerShip ) {
				PlayerShip->MoveDownModuleInTorpedosTargetQueue(i);
				UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
				StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}

// DISABLE/ENABLE AUTOFIRE MODES ON WEAPONS AND TORPEDOS MODULES.
	if (token.Icmp("click_toggle_weapons_module_autofire") == 0) {
		if ( PlayerShip ) {
			if ( PlayerShip->weapons_autofire_on == false ) {
				if ( PlayerShip->TargetEntityInSpace && team == PlayerShip->TargetEntityInSpace->team ) {
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForAutoFireAttackingFriendlyEntity");
						StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				} else if ( PlayerShip->TargetEntityInSpace && PlayerShip->HasNeutralityWithShip( PlayerShip->TargetEntityInSpace ) && !PlayerShip->TargetEntityInSpace->is_derelict ) {
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForAutoFireAttackingNonHostileEntity");
						StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				} else {
					PlayerShip->weapons_autofire_on = true;
					PlayerShip->CheckWeaponsTargetQueue();
				}
			} else {
				PlayerShip->weapons_autofire_on = false;
				if ( PlayerShip->TargetEntityInSpace ) {
					PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship = true;
				}
				if ( PlayerShip->consoles[WEAPONSMODULEID] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule ) {
					PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
				}
			}
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_toggle_torpedos_module_autofire") == 0) {
		if ( PlayerShip ) {
			if ( PlayerShip->torpedos_autofire_on == false ) {
				if ( PlayerShip->TargetEntityInSpace && team == PlayerShip->TargetEntityInSpace->team ) {
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForAutoFireAttackingFriendlyEntity");
						StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				} else if ( PlayerShip->TargetEntityInSpace && PlayerShip->HasNeutralityWithShip( PlayerShip->TargetEntityInSpace ) && !PlayerShip->TargetEntityInSpace->is_derelict ) {
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForAutoFireAttackingNonHostileEntity");
						StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				} else {
					PlayerShip->torpedos_autofire_on = true;
					PlayerShip->CheckTorpedosTargetQueue();
				}
			} else {
				PlayerShip->torpedos_autofire_on = false;
				if ( PlayerShip->TargetEntityInSpace ) {
					PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship = true;
				}
				if ( PlayerShip->consoles[TORPEDOSMODULEID] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule ) {
					PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
				}
			}
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_toggle_weapons_and_torpedos_modules_autofire") == 0) {
		if ( PlayerShip ) {
			if ( PlayerShip->weapons_autofire_on == false ) {
				if ( PlayerShip->TargetEntityInSpace && team == PlayerShip->TargetEntityInSpace->team ) {
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForAutoFireAttackingFriendlyEntity");
						StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				} else if ( PlayerShip->TargetEntityInSpace && PlayerShip->HasNeutralityWithShip( PlayerShip->TargetEntityInSpace ) && !PlayerShip->TargetEntityInSpace->is_derelict ) {
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForAutoFireAttackingNonHostileEntity");
						StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				} else {
					PlayerShip->weapons_autofire_on = true;
					PlayerShip->CheckWeaponsTargetQueue();
				}
			} else {
				PlayerShip->weapons_autofire_on = false;
				if ( PlayerShip->TargetEntityInSpace ) {
					PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship = true;
				}
				if ( PlayerShip->consoles[WEAPONSMODULEID] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule ) {
					PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
				}
			}
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
		}
		if ( PlayerShip ) {
			if ( PlayerShip->torpedos_autofire_on == false ) {
				if ( PlayerShip->TargetEntityInSpace && team == PlayerShip->TargetEntityInSpace->team ) {
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForAutoFireAttackingFriendlyEntity");
						StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				} else if ( PlayerShip->TargetEntityInSpace && PlayerShip->HasNeutralityWithShip( PlayerShip->TargetEntityInSpace ) && !PlayerShip->TargetEntityInSpace->is_derelict ) {
					if ( CaptainGui ) {
						CaptainGui->HandleNamedEvent("GetConfirmationForAutoFireAttackingNonHostileEntity");
						StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					}
				} else {
					PlayerShip->torpedos_autofire_on = true;
					PlayerShip->CheckTorpedosTargetQueue();
				}
			} else {
				PlayerShip->torpedos_autofire_on = false;
				if ( PlayerShip->TargetEntityInSpace ) {
					PlayerShip->TargetEntityInSpace->player_ceased_firing_on_this_ship = true;
				}
				if ( PlayerShip->consoles[TORPEDOSMODULEID] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule ) {
					PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule = NULL;
				}
			}
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
		}
		StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
		return true;
	}
	// confirmations of ENABLE AUTOFIRE MODES ON WEAPONS AND TORPEDOS MODULES.
	if (token.Icmp("click_confirm_enable_weapons_module_autofire") == 0) {
		if ( PlayerShip ) {
			PlayerShip->weapons_autofire_on = true;
			PlayerShip->CheckWeaponsTargetQueue();
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_confirm_enable_torpedos_module_autofire") == 0) {
		if ( PlayerShip ) {
			PlayerShip->torpedos_autofire_on = true;
			PlayerShip->CheckTorpedosTargetQueue();
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
			StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
	if (token.Icmp("click_confirm_enable_weapons_and_torpedos_modules_autofire") == 0) {
		if ( PlayerShip ) {
			PlayerShip->weapons_autofire_on = true;
			PlayerShip->CheckWeaponsTargetQueue();
			PlayerShip->torpedos_autofire_on = true;
			PlayerShip->CheckTorpedosTargetQueue();
			UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
		}
		StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
		return true;
	}

// INCREASE/DECREASE MODULES IN POWER LEVELS QUEUE
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_increase_" + module_description[i] + "_module_priority_in_power_queue") == 0) {
			if ( PlayerShip ) {
				PlayerShip->modules_power_automanage_on = true;
				PlayerShip->MoveUpModuleInModulesPowerQueue(i);
				UpdateModulesPowerQueueOnCaptainGui();
				PlayerShip->AutoManageModulePowerlevels();
				StartSoundShader( declManager->FindSound("space_command_guisounds_beep_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_decrease_" + module_description[i] + "_module_priority_in_power_queue") == 0) {
			if ( PlayerShip ) {
				PlayerShip->modules_power_automanage_on = true;
				PlayerShip->MoveDownModuleInModulesPowerQueue(i);
				UpdateModulesPowerQueueOnCaptainGui();
				PlayerShip->AutoManageModulePowerlevels();
				StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
	}

// DISABLE/ENABLE AUTOMANAGE MODE FOR MODULE POWER LEVELS.
	if (token.Icmp("click_toggle_modules_power_automanage") == 0) {
		if ( PlayerShip ) {
			if ( PlayerShip->modules_power_automanage_on == false && g_allowPlayerAutoPowerMode.GetBool() ) {
				PlayerShip->modules_power_automanage_on = true;
				PlayerShip->AutoManageModulePowerlevels();
			} else {
				PlayerShip->modules_power_automanage_on = false;
			}
			UpdateModulesPowerQueueOnCaptainGui();
			StartSoundShader( declManager->FindSound("space_command_guisounds_computer_up_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
// INCREASE/DECREASE MODULES TARGET AUTOMANAGE POWER LEVELS
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_increase_" + module_description[i] + "_automanage_target_module_power") == 0) {
			if ( PlayerShip && PlayerShip->consoles[i] ) {
				if ( PlayerShip->consoles[i]->ControlledModule->automanage_target_power_level < PlayerShip->consoles[i]->ControlledModule->max_power && PlayerShip->current_automanage_power_reserve > 0 ) {
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
				} else {
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				}
				PlayerShip->IncreaseAutomanageTargetModulePower(PlayerShip->consoles[i]->ControlledModule);
				PlayerShip->modules_power_automanage_on = true;
				PlayerShip->AutoManageModulePowerlevels();
				UpdateModulesPowerQueueOnCaptainGui();
				return true; // this makes it so that the HandleSingleGuiCommand only gets called once - otherwise it will get called twice and power will increase double.
			}
		}
	}
	for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		if (token.Icmp("click_decrease_" + module_description[i] + "_automanage_target_module_power") == 0) {
			if ( PlayerShip && PlayerShip->consoles[i] ) {
				if ( PlayerShip->consoles[i]->ControlledModule->automanage_target_power_level == 0 ) {
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				} else {
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
				}
				PlayerShip->DecreaseAutomanageTargetModulePower(PlayerShip->consoles[i]->ControlledModule);
				PlayerShip->modules_power_automanage_on = true;
				PlayerShip->AutoManageModulePowerlevels();
				UpdateModulesPowerQueueOnCaptainGui();
				return true; // this makes it so that the HandleSingleGuiCommand only gets called once - otherwise it will get called twice and power will increase double.
			}
		}
	}
// AI DIALOGUE SYSTEM BEGIN
	if ( guiOverlay == AIDialogueGui ) {
		if ( DialogueAI ) {
			if ( token.Icmp( "activate_or_trigger_this_ai" ) == 0 ) {
				DialogueAI->Activate( this );
				// BOYETTE NOTE TODO IMPORTANT: put the functions under the "click_close_ai_dialogue_gui" command here
				return true;
			}
		}
		if ( DialogueAI ) {
			for ( int i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
				if ( token.Icmp( va("click_toggle_ai_dialogue_branch_%i_visible", i ) ) == 0 ) {
					AIDialogueGui->HandleNamedEvent("AIDialogueGUIResponseChosen");
					ResetAIDialogueGuiBranchVisibilities();
					DialogueAI->AIDialogueBranchTracker.flip(i);
					UpdateAIDialogueGui();
					DialogueAI->ai_dialogue_anim_to_play = "ai_dialogue_standard";
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
		if ( DialogueAI ) {
			if ( token.Icmp("click_play_ai_dialogue_standard_animation") == 0 ) {
				// set the AIDialogueAnim to "standard_ai_dialogue_animation"
				DialogueAI->ai_dialogue_anim_to_play = "ai_dialogue_standard";
				//DialogueAI->GetHeadEntity()
				// when the source finds that there is a AIDialogueAnim it will play it.
				return true;
			}
		}
		if ( DialogueAI ) {
			if ( token.Icmp("click_play_ai_dialogue_angry_animation") == 0 ) {
				// set the AIDialogueAnim to "standard_ai_dialogue_animation"
				DialogueAI->ai_dialogue_anim_to_play = "ai_dialogue_angry";
				//DialogueAI->GetHeadEntity()
				// when the source finds that there is a AIDialogueAnim it will play it.
				return true;
			}
		}
		if ( DialogueAI ) {
			if ( token.Icmp("click_have_dialogue_ai_join_your_crew") == 0 ) {
				if ( PlayerShip ) {
					if ( DialogueAI->ParentShip && DialogueAI->ParentShip == PlayerShip ) {
						DisplayNonInteractiveAlertMessage( idStr(DialogueAI->spawnArgs.GetString("npc_name", "This being")) + " is already a member of your crew.");
						return true;
					} else {
						PlayerShip->InviteAIToJoinCrew(DialogueAI,false,true,true);
						return true;
					}
				}
			}
			if ( token.Icmp("click_have_dialogue_ai_join_your_crew_without_transport") == 0 ) {
				if ( PlayerShip ) {
					if ( DialogueAI->ParentShip && DialogueAI->ParentShip == PlayerShip ) {
						DisplayNonInteractiveAlertMessage( idStr(DialogueAI->spawnArgs.GetString("npc_name", "This being")) + " is already a member of your crew.");
						return true;
					} else {
						PlayerShip->InviteAIToJoinCrew(DialogueAI);
						return true;
					}
				}
			}
		}
		if (token.Icmp("click_close_ai_dialogue_gui") == 0) {
			if ( PlayerShip /*&& SelectedEntityInSpace*/ ) {
				if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
					CaptainGui->multiselect_box_active = false;
					CaptainGui->EndMultiselectBox();
				}
			}
			CloseOverlayAIDialogeGui();
			StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
			return true;
		}
	}
// AI DIALOGUE SYSTEM END

// HAIL SYSTEM/DIALOGUE SYSTEM BEGIN
	if ( guiOverlay == HailGui ) {
		if ( PlayerShip && SelectedEntityInSpace ) {
			for ( int i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
				if ( token.Icmp( va("click_toggle_selectedship_dialogue_branch_%i_visible", i ) ) == 0 ) {
					HailGui->HandleNamedEvent("HailGUIDialogueResponseChosen");
					ResetHailGuiDialogueBranchVisibilities();
					SelectedEntityInSpace->hailDialogueBranchTracker.flip(i);
					UpdateHailGui();
					StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("click_toggle_selectedship_dialogue_branch_hostile_visible") == 0 ) {
				//HailGui->HandleNamedEvent("HailGUIDialogueResponseChosen");
				ResetHailGuiDialogueBranchVisibilities();
				SelectedEntityInSpace->friendlinessWithPlayer = 0;

				PlayerShip->EndNeutralityWithTeam( SelectedEntityInSpace->team );
				SelectedEntityInSpace->EndNeutralityWithTeam( team );
				if ( PlayerShip->team == SelectedEntityInSpace->team ) PlayerShip->BecomeASpacePirateShip();

				PlayerShip->Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();
				SelectedEntityInSpace->Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();

				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("click_toggle_selectedship_dialogue_branch_forgive_visible") == 0 ) {
				//HailGui->HandleNamedEvent("HailGUIDialogueResponseChosen");
				ResetHailGuiDialogueBranchVisibilities();
				SelectedEntityInSpace->friendlinessWithPlayer++;
				SelectedEntityInSpace->has_forgiven_player = true;
				SelectedEntityInSpace->is_ignoring_player = false;
				//HailGui->SetStateBool("selectedship_dialogue_branch_forgive_visible", true );
				//for ( int i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
				//	HailGui->SetStateBool( va("selectedship_dialogue_branch_%i_visible", i ), false );
				//}
				PlayerShip->StartNeutralityWithTeam( SelectedEntityInSpace->team );
				SelectedEntityInSpace->StartNeutralityWithTeam( team );
				DisplayNonInteractiveAlertMessage("We are now neutral with the " + SelectedEntityInSpace->name + " and its allies.");
				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("click_toggle_selectedship_dialogue_branch_ignore_visible") == 0 ) {
				//HailGui->HandleNamedEvent("HailGUIDialogueResponseChosen");
				ResetHailGuiDialogueBranchVisibilities();
				SelectedEntityInSpace->has_forgiven_player = false;
				SelectedEntityInSpace->is_ignoring_player = true;
				//HailGui->SetStateBool("selectedship_dialogue_branch_ignore_visible", true );
				//for ( int i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
				//	HailGui->SetStateBool( va("selectedship_dialogue_branch_%i_visible", i ), false );
				//}
				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("click_force_dialogue_response_chosen_transition") == 0 ) {
				ResetHailGuiDialogueBranchVisibilities();
				HailGui->HandleNamedEvent("HailGUIDialogueResponseChosen");
				UpdateHailGui();
				StartSoundShader( declManager->FindSound("space_command_guisounds_click_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("click_become_neutral_with_selectedship") == 0 ) {
				SelectedEntityInSpace->friendlinessWithPlayer++;
				PlayerShip->StartNeutralityWithTeam( SelectedEntityInSpace->team );
				SelectedEntityInSpace->StartNeutralityWithTeam( team );
				DisplayNonInteractiveAlertMessage("We are now neutral with the " + SelectedEntityInSpace->name + " and its allies.");
				if ( PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->team == SelectedEntityInSpace->team ) {
					PlayerShip->CeaseFiringWeaponsAndTorpedos();
					PlayerShip->TargetEntityInSpace->CeaseFiringWeaponsAndTorpedos();
				}
				if ( SelectedEntityInSpace->TargetEntityInSpace && SelectedEntityInSpace->TargetEntityInSpace->team == PlayerShip->team ) {
					SelectedEntityInSpace->CeaseFiringWeaponsAndTorpedos();
					SelectedEntityInSpace->TargetEntityInSpace->CeaseFiringWeaponsAndTorpedos();
				}
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("gamble_50_50_for_neutrality") == 0 ) {
				int random_int = gameLocal.random.RandomInt( 2 );
				gameLocal.Printf( "The random 50/50 is " + idStr(random_int) + "\n");
				if ( random_int == 0 ) {
					SelectedEntityInSpace->friendlinessWithPlayer++;
					PlayerShip->StartNeutralityWithTeam( SelectedEntityInSpace->team );
					SelectedEntityInSpace->StartNeutralityWithTeam( team );
					DisplayNonInteractiveAlertMessage("We are now neutral with the " + SelectedEntityInSpace->name + " and its allies.");
					if ( PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->team == SelectedEntityInSpace->team ) {
						PlayerShip->CeaseFiringWeaponsAndTorpedos();
						PlayerShip->TargetEntityInSpace->CeaseFiringWeaponsAndTorpedos();
					}
					if ( SelectedEntityInSpace->TargetEntityInSpace && SelectedEntityInSpace->TargetEntityInSpace->team == PlayerShip->team ) {
						SelectedEntityInSpace->CeaseFiringWeaponsAndTorpedos();
						SelectedEntityInSpace->TargetEntityInSpace->CeaseFiringWeaponsAndTorpedos();
						SelectedEntityInSpace->TargetEntityInSpace = NULL;
						SelectedEntityInSpace->Event_SetMainGoal(SHIP_AI_IDLE,NULL);
					}
					StartSoundShader( declManager->FindSound("space_command_guisounds_beepdown_03") , SND_CHANNEL_ANY, 0, false, NULL ); // gamble was successful
				} else {
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL ); // gamble was NOT successful
				}
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("click_lower_shields_as_a_sign_of_trust") == 0 ) {
				PlayerShip->LowerShields();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("click_raise_shields_as_a_sign_of_distrust") == 0 ) {
				PlayerShip->RaiseShields();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("end_hail_conditionals_met_dialogue") == 0 ) {
				SelectedEntityInSpace->hail_conditionals_met = false;
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("selectedship_increase_friendliness_with_player") == 0 ) {
				SelectedEntityInSpace->friendlinessWithPlayer++;
				SelectedEntityInSpace->friendlinessWithPlayer = idMath::ClampInt(0,10,SelectedEntityInSpace->friendlinessWithPlayer);
				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("selectedship_decrease_friendliness_with_player") == 0 ) {
				SelectedEntityInSpace->friendlinessWithPlayer--;
				SelectedEntityInSpace->friendlinessWithPlayer = idMath::ClampInt(0,10,SelectedEntityInSpace->friendlinessWithPlayer);

				if ( SelectedEntityInSpace->friendlinessWithPlayer <= 0 ) {
					PlayerShip->EndNeutralityWithTeam( SelectedEntityInSpace->team );
					SelectedEntityInSpace->EndNeutralityWithTeam( team );
					if ( PlayerShip->team == SelectedEntityInSpace->team ) PlayerShip->BecomeASpacePirateShip();

					PlayerShip->Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();
					SelectedEntityInSpace->Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();
				}

				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("selectedship_repair_playership_one_cycle") == 0 ) {
				// here need to check for other ships at the same stargrid position here before repairing
				if ( PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition() ) {
					DisplayNonInteractiveAlertMessage("You cannot repair your ship when there is a hostile entity in range.");
					return true;
				} else if ( PlayerShip->CheckForHostileEntitiesOnBoard() ) {
					DisplayNonInteractiveAlertMessage("You cannot repair your ship when there are hostile entities on board.");
					return true;
				}
				PlayerShip->RepairShipWithMaterials(MAX_HULL_REPAIR_AMOUNT_PER_SHIP_REPAIR_CYCLE);
				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("selectedship_repair_playership_as_much_as_possible") == 0 ) {
				// here need to check for other ships at the same stargrid position here before repairing
				if ( PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition() ) {
					DisplayNonInteractiveAlertMessage("You cannot repair your ship when there is a hostile entity in range.");
					return true;
				} else if ( PlayerShip->CheckForHostileEntitiesOnBoard() ) {
					DisplayNonInteractiveAlertMessage("You cannot repair your ship when there are hostile entities on board.");
					return true;
				}
				PlayerShip->RepairShipWithMaterials(77777); // 77777 is just a large number
				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("playership_give_100_currency_to_selectedship") == 0 ) {
				if ( PlayerShip->current_currency_reserves >= 100 ) {
					PlayerShip->current_currency_reserves -= 100;
					SelectedEntityInSpace->current_currency_reserves += 100;
				} else {
					DisplayNonInteractiveAlertMessage("You do not have enough currency to give them.");
					return true;
				}
				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("playership_take_100_currency_from_selectedship") == 0 ) {
				if ( SelectedEntityInSpace->current_currency_reserves >= 100 ) {
					SelectedEntityInSpace->current_currency_reserves -= 100;
					PlayerShip->current_currency_reserves += 100;
				} else {
					DisplayNonInteractiveAlertMessage("They do not have enough currency to give you.");
					return true;
				}
				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("playership_give_100_materials_to_selectedship") == 0 ) {
				if ( PlayerShip->current_materials_reserves >= 100 ) {
					PlayerShip->current_materials_reserves -= 100;
					SelectedEntityInSpace->current_materials_reserves += 100;
				} else {
					DisplayNonInteractiveAlertMessage("You do not have enough materials to give them.");
					return true;
				}
				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("playership_take_100_materials_from_selectedship") == 0 ) {
				if ( SelectedEntityInSpace->current_materials_reserves >= 100 ) {
					SelectedEntityInSpace->current_materials_reserves -= 100;
					PlayerShip->current_materials_reserves += 100;
				} else {
					DisplayNonInteractiveAlertMessage("They do not have enough materials to give you.");
					return true;
				}
				UpdateHailGui();
				return true;
			}
		}
		// BOYETTE NOTE TODO BEGIN - need to iterate these 100 to 1000 in increments of 100
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("playership_give_100_materials_to_selectedship_for_100_currency") == 0 ) {
				if ( PlayerShip->current_materials_reserves < 100 ) {
					DisplayNonInteractiveAlertMessage("You do not have enough materials to make this trade.");
					return true;
				}
				if ( SelectedEntityInSpace->current_currency_reserves < 100 ) {
					DisplayNonInteractiveAlertMessage("They do not have enough credits to make this trade.");
					return true;
				}

				PlayerShip->current_materials_reserves -= 100;
				SelectedEntityInSpace->current_materials_reserves += 100;

				SelectedEntityInSpace->current_currency_reserves -= 100;
				PlayerShip->current_currency_reserves += 100;

				UpdateHailGui();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("playership_give_100_currency_to_selectedship_for_100_materials") == 0 ) {
				if ( PlayerShip->current_currency_reserves < 100 ) {
					DisplayNonInteractiveAlertMessage("You do not have enough currency to make this trade.");
					return true;
				}
				if ( SelectedEntityInSpace->current_materials_reserves < 100 ) {
					DisplayNonInteractiveAlertMessage("They do not have enough materials to make this trade.");
					return true;
				}

				PlayerShip->current_currency_reserves -= 100;
				SelectedEntityInSpace->current_currency_reserves += 100;

				SelectedEntityInSpace->current_materials_reserves -= 100;
				PlayerShip->current_materials_reserves += 100;

				UpdateHailGui();
				return true;
			}
		}
		/*
		// BOYETTE NOTE TODO END - need to iterate these 100 to 1000 in increments of 100
		if ( PlayerShip ) {
			if ( token.Icmp("playership_hire_human_male_officer_for_100_currency") == 0 ) {
				if ( PlayerShip->current_currency_reserves >= 100 ) {
					if ( PlayerShip->CanHireCrew() ) {
						PlayerShip->current_currency_reserves -= 100;
						SelectedEntityInSpace->current_currency_reserves += 100;

						int crew_id_hired = 0;
						crew_id_hired = PlayerShip->HireCrew( "spaceship_crewmember_default" );
						DisplayNonInteractiveAlertMessage("You have hired a human male as your " + role_description[crew_id_hired] + " officer.");
						UpdateHailGui();
						return true;
					} else {
						DisplayNonInteractiveAlertMessage("Your crew is already at full capacity.");
						return true;
					}
					UpdateHailGui();
					return true;
				} else {
					DisplayNonInteractiveAlertMessage("You do not have enough credits to hire crew.");
					return true;
				}
			}
		}
		*/
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("playership_hire") == 0 ) {
				bool hostile_entities_present = PlayerShip->CheckForHostileEntitiesAtCurrentStarGridPosition();
				bool hostile_entities_on_board = PlayerShip->CheckForHostileEntitiesOnBoard();
				if ( !hostile_entities_present && !hostile_entities_on_board ) {

					if ( src->ReadToken( &token2 ) ) {
						if ( token2 == ";" ) {
							src->UnreadToken( &token2 );
							gameLocal.Warning( "playership_hire gui command needs the 1st argument.", token2.c_str() );
							return true;
						} else {
							const idDict* entity_dict_to_spawn = gameLocal.FindEntityDefDict( token2 , false );
							if ( !entity_dict_to_spawn ) {
								gameLocal.Warning( "Tried to hire unknown classname: %s", token2.c_str() );
								return true;
							}
						}
					}

					int credits_cost = 0;
					if ( src->ReadToken( &token3 ) ) {
						if ( token3 == ";" ) {
							src->UnreadToken( &token3 );
						} else if ( !idStr(token3).IsNumeric() ) {
							gameLocal.Warning( "Tried to hire classname with non numeric currency cost argument: %s", token3.c_str() );
							return true;
						} else {
							credits_cost = atoi(token3);
						}
					}

					int materials_cost = 0;
					if ( src->ReadToken( &token4 ) ) {
						if ( token4 == ";" ) {
							src->UnreadToken( &token4 );
						} else if ( !idStr(token4).IsNumeric() ) {
							gameLocal.Warning( "Tried to hire classname with non numeric materials cost argument: %s", token4.c_str() );
							return true;
						} else {
							materials_cost = atoi(token4);
						}
					}


					if ( PlayerShip->current_currency_reserves >= credits_cost && PlayerShip->current_materials_reserves >= materials_cost ) {
						if ( PlayerShip->CanHireCrew() ) {
							PlayerShip->current_currency_reserves -= credits_cost;
							SelectedEntityInSpace->current_currency_reserves += credits_cost;
							PlayerShip->current_materials_reserves -= materials_cost;
							SelectedEntityInSpace->current_materials_reserves += materials_cost;

							int crew_id_hired = 0;
							crew_id_hired = PlayerShip->HireCrew( token2 );
							DisplayNonInteractiveAlertMessage(idStr("You have hired a ") + PlayerShip->crew[crew_id_hired]->spawnArgs.GetString( "space_command_vocation", "Explorer/Scientist" ) + " as your " + role_description[crew_id_hired] + " officer.");
							UpdateHailGui();
							return true;
						} else if ( PlayerShip->CanHireReserveCrew() ) {
							PlayerShip->current_currency_reserves -= credits_cost;
							SelectedEntityInSpace->current_currency_reserves += credits_cost;
							PlayerShip->current_materials_reserves -= materials_cost;
							SelectedEntityInSpace->current_materials_reserves += materials_cost;

							const idDict* entity_dict_to_hire = gameLocal.FindEntityDefDict( token2 , false );
							PlayerShip->reserve_Crew.push_back(*entity_dict_to_hire);
							DisplayNonInteractiveAlertMessage(idStr("You have hired a ") + entity_dict_to_hire->GetString( "space_command_vocation", "Explorer/Scientist" ) + " to your reserve crew.");
							UpdateHailGui();
							UpdateReserveCrewMemberPortraits();
							return true;
						} else {
							DisplayNonInteractiveAlertMessage("Your main crew and reserve crew are at full capacity.");
							return true;
						}
						UpdateHailGui();
						return true;
					} else {
						DisplayNonInteractiveAlertMessage("You do not have enough resources to hire crew.");
						return true;
					}
				} else {
					if ( hostile_entities_present ) {
						DisplayNonInteractiveAlertMessage("You cannot hire crew during battle."); //when there is a hostile entity in range.");
					} else if ( hostile_entities_on_board ) {
						DisplayNonInteractiveAlertMessage("You cannot hire crew when there are hostiles on board."); //when there is a hostile entity in range.");
					}
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					return true;
				}
			}
		}

		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("playership_invite_entire_crew_of_selectedship_to_join_your_crew") == 0 ) {
				bool all_invited_crew_joined = true;
				for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
					if ( SelectedEntityInSpace->crew[i] ) {
						if ( PlayerShip->InviteAIToJoinCrew(SelectedEntityInSpace->crew[i],false,true,true,0) == false ) {
							all_invited_crew_joined = false;
						}
					}
				}
				if ( all_invited_crew_joined ) {
					CloseOverlayHailGui();
				}
				if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
					CaptainGui->multiselect_box_active = false;
					CaptainGui->EndMultiselectBox();
				}
				StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
			if ( token.Icmp("playership_invite_entire_crew_of_selectedship_to_join_your_crew_force_transport") == 0 ) {
				bool all_invited_crew_joined = true;
				for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
					if ( SelectedEntityInSpace->crew[i] ) {
						if ( PlayerShip->InviteAIToJoinCrew(SelectedEntityInSpace->crew[i],true,true,true,0) == false ) {
							all_invited_crew_joined = false;
						}
					}
				}
				if ( all_invited_crew_joined ) {
					CloseOverlayHailGui();
				}
				if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
					CaptainGui->multiselect_box_active = false;
					CaptainGui->EndMultiselectBox();
				}
				StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
			if ( token.Icmp("playership_invite_entire_crew_of_selectedship_to_join_your_crew_without_transport") == 0 ) {
				bool all_invited_crew_joined = true;
				for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
					if ( SelectedEntityInSpace->crew[i] ) {
						if ( PlayerShip->InviteAIToJoinCrew(SelectedEntityInSpace->crew[i],false,false,false,0) == false ) {
							all_invited_crew_joined = false;
						}
					}
				}
				if ( all_invited_crew_joined ) {
					CloseOverlayHailGui();
				}
				if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
					CaptainGui->multiselect_box_active = false;
					CaptainGui->EndMultiselectBox();
				}
				StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}


			if ( token.Icmp("click_suffocate_all_ais_on_board_of_selectship") == 0 ) {
				SelectedEntityInSpace->infinite_oxygen = false;
				//SelectedEntityInSpace->current_oxygen_level = 0; // BOYETTE NOTE TODO: not sure if we should do this - probably not
				for ( int i = 0; i < SelectedEntityInSpace->AIsOnBoard.size() ; i++ ) {
					SelectedEntityInSpace->AIsOnBoard[i]->Damage(this,this,idVec3(0,0,0),"damage_triggerhurt_toxin",800,0);
				}
				if ( SelectedEntityInSpace->consoles[ENVIRONMENTMODULEID] && SelectedEntityInSpace->consoles[ENVIRONMENTMODULEID]->ControlledModule ) {
					SelectedEntityInSpace->consoles[ENVIRONMENTMODULEID]->Damage(this,this,idVec3(0,0,0),"damage_massive",800,0);
					SelectedEntityInSpace->consoles[ENVIRONMENTMODULEID]->ControlledModule->Damage(this,this,idVec3(0,0,0),"damage_massive",800,0);
				}
			}
			if ( token.Icmp("click_turn_off_infinite_oxygen_on_selectship") == 0 ) {
				SelectedEntityInSpace->infinite_oxygen = false;
			}
			if ( token.Icmp("click_all_ais_on_board_of_selectship_die") == 0 ) {
				for ( int i = 0; i < SelectedEntityInSpace->AIsOnBoard.size() ; i++ ) {
					SelectedEntityInSpace->AIsOnBoard[i]->Damage(this,this,idVec3(0,0,0),"damage_triggerhurt_toxin",800,0);
				}
			}
		}

		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("all_ships_at_this_sg_pos_exit_no_action_hail_mode") == 0 ) {
				PlayerShip->Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();
				SelectedEntityInSpace->Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("start_delay_selectedship_dialogue_branch_hostile_visible") == 0 ) {
				delay_selectedship_dialogue_branch_hostile_visible = true;
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("end_delay_selectedship_dialogue_branch_hostile_visible") == 0 ) {
				delay_selectedship_dialogue_branch_hostile_visible = false;
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("set_player_as_priority_target_of_selected_entity") == 0 ) {
				SelectedEntityInSpace->EndNeutralityWithShip(PlayerShip);
				SelectedEntityInSpace->spawnArgs.SetBool("prioritize_playership_as_space_entity_to_target","1");
				SelectedEntityInSpace->prioritize_playership_as_space_entity_to_target = true;
				SelectedEntityInSpace->Event_SetMainGoal(SHIP_AI_IDLE,NULL);
				return true;
			}
		}
		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("set_player_as_priority_protectee_of_selected_entity") == 0 ) {
				SelectedEntityInSpace->StartNeutralityWithTeam(PlayerShip->team);
				PlayerShip->StartNeutralityWithTeam(SelectedEntityInSpace->team);
				DisplayNonInteractiveAlertMessage("We are now neutral with the " + SelectedEntityInSpace->name + " and its allies.");
				SelectedEntityInSpace->spawnArgs.SetBool("prioritize_playership_as_space_entity_to_protect","1");
				SelectedEntityInSpace->prioritize_playership_as_space_entity_to_protect = true;
				SelectedEntityInSpace->Event_SetMainGoal(SHIP_AI_IDLE,NULL);
				return true;
			}
		}

		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("set_selected_entity_as_target") == 0 ) {
				PlayerShip->SetTargetEntityInSpace(SelectedEntityInSpace);
				return true;
			}
		}

		if ( PlayerShip && SelectedEntityInSpace ) {
			if ( token.Icmp("have_selected_entity_set_playership_as_target") == 0 ) {
				SelectedEntityInSpace->SetTargetEntityInSpace(PlayerShip);
				return true;
			}
		}

		if (token.Icmp("click_close_hailgui") == 0) {
			if ( PlayerShip /*&& SelectedEntityInSpace*/ ) {
				//SetOverlayHailGui();
				//BOYETTE NOTE TODO: might want to standardize the multiselect box deactivation whenever a gui is opened or activated or closed. - BEGIN
				if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
					CaptainGui->multiselect_box_active = false;
					CaptainGui->EndMultiselectBox();
				}
				//BOYETTE NOTE TODO: might want to standardize the multiselect box deactivation whenever a gui is opened or activated or closed. - END
				CloseOverlayHailGui();
				//SelectedEntityInSpace->currently_in_hail = false; // this should get handled already by this point.
				// STILL NOT SURE IF WE WANT TO STOP TIME DURING A HAIL.
				//g_stopTime.SetBool(false);
				//g_stopTimeForceFrameNumUpdateDuring.SetBool(false);
				//g_stopTimeForceRenderViewUpdateDuring.SetBool(false);
				StartSoundShader( declManager->FindSound("space_command_guisounds_menu_click_down_01") , SND_CHANNEL_ANY, 0, false, NULL );
				return true;
			}
			//if ( guiOverlay == HailGui ) {
			//	guiOverlay = NULL;
			//}
		}
	}
// HAIL SYSTEM/DIALOGUE SYSTEM END
	if (token.Icmp("end_multiselect_box") == 0) {
		if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
			CaptainGui->multiselect_box_active = false;
			CaptainGui->EndMultiselectBox();
		}
	}

	if (token.Icmp("click_close_guioverlay") == 0) {
		if ( guiOverlay ) {
			CloseOverlayCaptainGui();
		}
		if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
			CaptainGui->multiselect_box_active = false;
			CaptainGui->EndMultiselectBox();
		}
		return true;
	}



// STORY WINDOW OVERLAY SYSTEM BEGIN
	if ( guiOverlay && guiOverlay != HailGui && guiOverlay != AIDialogueGui && guiOverlay != CaptainGui ) {

		if ( PlayerShip ) {
			if ( token.Icmp("playership_hire") == 0 ) {

				if ( src->ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src->UnreadToken( &token2 );
						gameLocal.Warning( "playership_hire gui command needs the 1st argument.", token2.c_str() );
						return true;
					} else {
						const idDict* entity_dict_to_spawn = gameLocal.FindEntityDefDict( token2 , false );
						if ( !entity_dict_to_spawn ) {
							gameLocal.Warning( "Tried to hire unknown classname: %s", token2.c_str() );
							return true;
						}
					}
				}

				int credits_cost = 0;
				if ( src->ReadToken( &token3 ) ) {
					if ( token3 == ";" ) {
						src->UnreadToken( &token3 );
					} else if ( !idStr(token3).IsNumeric() ) {
						gameLocal.Warning( "Tried to hire classname with non numeric currency cost argument: %s", token3.c_str() );
						return true;
					} else {
						credits_cost = atoi(token3);
					}
				}

				int materials_cost = 0;
				if ( src->ReadToken( &token4 ) ) {
					if ( token4 == ";" ) {
						src->UnreadToken( &token4 );
					} else if ( !idStr(token4).IsNumeric() ) {
						gameLocal.Warning( "Tried to hire classname with non numeric materials cost argument: %s", token4.c_str() );
						return true;
					} else {
						materials_cost = atoi(token4);
					}
				}


				if ( PlayerShip->current_currency_reserves >= credits_cost && PlayerShip->current_materials_reserves >= materials_cost ) {
					if ( PlayerShip->CanHireCrew() ) {
						PlayerShip->current_currency_reserves -= credits_cost;
						PlayerShip->current_materials_reserves -= materials_cost;

						int crew_id_hired = 0;
						crew_id_hired = PlayerShip->HireCrew( token2 );
						DisplayNonInteractiveAlertMessage(idStr("You have hired a ") + PlayerShip->crew[crew_id_hired]->spawnArgs.GetString( "space_command_vocation", "Explorer/Scientist" ) + " as your " + role_description[crew_id_hired] + " officer.");
						UpdateHailGui();
						return true;
					} else if ( PlayerShip->CanHireReserveCrew() ) {
						PlayerShip->current_currency_reserves -= credits_cost;
						PlayerShip->current_materials_reserves -= materials_cost;

						const idDict* entity_dict_to_hire = gameLocal.FindEntityDefDict( token2 , false );
						PlayerShip->reserve_Crew.push_back(*entity_dict_to_hire);
						DisplayNonInteractiveAlertMessage(idStr("You have hired a ") + entity_dict_to_hire->GetString( "space_command_vocation", "Explorer/Scientist" ) + " to your reserve crew.");
						UpdateHailGui();
						UpdateReserveCrewMemberPortraits();
						return true;
					} else {
						DisplayNonInteractiveAlertMessage("Your main crew and reserve crew are at full capacity.");
						return true;
					}
					UpdateHailGui();
					return true;
				} else {
					DisplayNonInteractiveAlertMessage("You do not have enough resources to hire crew.");
					return true;
				}
			}
		}

		if ( PlayerShip ) {
			if ( token.Icmp("gamble_for_something") == 0 ) { // BOYETTE NOTE TODO IMPORTANT: this is just the template - will decide what to gamble for at some future point time
				int success_chance = 0;
				if ( src->ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src->UnreadToken( &token2 );
						gameLocal.Warning( "gamble_for_something gui command did not specify success chance as first argument.", token2.c_str() );
						return true;
					} else if ( !idStr(token2).IsNumeric() ) {
						gameLocal.Warning( "Tried to perform gamble with non numeric success chance argument: %s", token2.c_str() );
						return true;
					} else {
						success_chance = atoi(token2);
					}
				}

				int random_int = gameLocal.random.RandomInt( 100 );
				gameLocal.Printf( "The random 50/50 is " + idStr(random_int) + "\n");
				if ( random_int < success_chance ) {
					// BOYETTE NOTE TODO: need to give the player something
					DisplayNonInteractiveAlertMessage("Success.");
					StartSoundShader( declManager->FindSound("space_command_guisounds_beepdown_03") , SND_CHANNEL_ANY, 0, false, NULL ); // gamble was successful
				} else {
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL ); // gamble was NOT successful
				}
				return true;
			}
		}

		if ( PlayerShip ) {
			if ( token.Icmp("playership_recieve_currency_for_materials") == 0 ) {

				int credits_cost = 0;
				if ( src->ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src->UnreadToken( &token2 );
					} else if ( !idStr(token2).IsNumeric() ) {
						gameLocal.Warning( "playership_give_currency_for_materials specified non numeric currency cost argument: %s", token2.c_str() );
						return true;
					} else {
						credits_cost = atoi(token2);
					}
				}

				int materials_cost = 0;
				if ( src->ReadToken( &token3 ) ) {
					if ( token3 == ";" ) {
						src->UnreadToken( &token3 );
					} else if ( !idStr(token3).IsNumeric() ) {
						gameLocal.Warning( "playership_give_currency_for_materials specified non numeric materials cost argument: %s", token3.c_str() );
						return true;
					} else {
						materials_cost = atoi(token3);
					}
				}

				if ( PlayerShip->current_materials_reserves < materials_cost ) {
					DisplayNonInteractiveAlertMessage("You do not have enough materials to make this trade.");
					return true;
				}

				PlayerShip->current_currency_reserves += credits_cost;
				PlayerShip->current_materials_reserves -= materials_cost;

				UpdateGuiOverlay();
				return true;
			}
		}

		if ( PlayerShip ) {
			if ( token.Icmp("playership_give_currency_for_materials") == 0 ) {

				int credits_cost = 0;
				if ( src->ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src->UnreadToken( &token2 );
					} else if ( !idStr(token2).IsNumeric() ) {
						gameLocal.Warning( "playership_give_currency_for_materials specified non numeric currency cost argument: %s", token2.c_str() );
						return true;
					} else {
						credits_cost = atoi(token2);
					}
				}

				int materials_cost = 0;
				if ( src->ReadToken( &token3 ) ) {
					if ( token3 == ";" ) {
						src->UnreadToken( &token3 );
					} else if ( !idStr(token3).IsNumeric() ) {
						gameLocal.Warning( "playership_give_currency_for_materials specified non numeric materials cost argument: %s", token3.c_str() );
						return true;
					} else {
						materials_cost = atoi(token3);
					}
				}

				if ( PlayerShip->current_currency_reserves < credits_cost ) {
					DisplayNonInteractiveAlertMessage("You do not have enough credit to make this trade.");
					return true;
				}

				PlayerShip->current_currency_reserves -= credits_cost;
				PlayerShip->current_materials_reserves += materials_cost;

				UpdateGuiOverlay();
				return true;
			}
		}

		if (token.Icmp("satisfy_all_story_windows_at_this_stargrid_position") == 0) {
			for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size(); i++ ) {
				if ( PlayerShip->ships_at_my_stargrid_position[i] ) {
					PlayerShip->ships_at_my_stargrid_position[i]->story_window_satisfied = true;
				}
			}
			return true;
		}

		if (token.Icmp("click_close_story_window_guioverlay") == 0) {
			bool was_closed = false;
			if ( guiOverlay ) {
				CloseOverlayGui();
				was_closed = true;
			}
			if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
				CaptainGui->multiselect_box_active = false;
				CaptainGui->EndMultiselectBox();
			}
			gameSoundWorld->FadeSoundClasses(0,0.0f,0.1f); // BOYETTE NOTE: 0.0f is the default decibel level for all sounds. All entities have a soundclass of zero unless it is set otherwise.
			if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->music_shader_is_playing ) {
				gameSoundWorld->FadeSoundClasses(3,-17.0f,0.0f);
			} else {
				gameSoundWorld->FadeSoundClasses(3,0.0f,0.1f); // ship alarms are soundclass 3. 0.0f db is the default level.
			}
			gameSoundWorld->FadeSoundClasses(1,0.0f,1.5f); // BOYETTE NOTE: 0.0f is the default decibel level for all sounds. The space ship hum is set to soundclass 1.
			g_stopTime.SetBool(false);
			g_stopTimeForceFrameNumUpdateDuring.SetBool(false);
			g_stopTimeForceRenderViewUpdateDuring.SetBool(false);
			currently_in_story_gui = false;

			if ( was_closed && CaptainChairSeatedIn && PlayerShip && PlayerShip->TargetEntityInSpace ) {
				SetViewAngles( CaptainChairSeatedIn->GetPhysics()->GetAxis().ToAngles() );
				SetAngles( CaptainChairSeatedIn->GetPhysics()->GetAxis().ToAngles() );
				SetOverlayCaptainGui();
				CaptainGui->HandleNamedEvent( "GoToTacticalTab" );
			}

			if ( g_inIronManMode.GetBool() ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "saveGame IRONMAN\n" );
			}
			return true;
		}

	}
// STORY WINDOW OVERLAY SYSTEM END



	// boyette stargrid mod begin
	if (token.Icmp("click_engagewarp") == 0) {
		if ( PlayerShip ) {
			if ( (PlayerShip->CaptainChair && PlayerShip->CaptainChair->SeatedEntity.GetEntity() && PlayerShip->CaptainChair->SeatedEntity.GetEntity()) || (PlayerShip->ReadyRoomCaptainChair && PlayerShip->ReadyRoomCaptainChair->SeatedEntity.GetEntity() && PlayerShip->ReadyRoomCaptainChair->SeatedEntity.GetEntity()) ) {
				if ( (PlayerShip->stargriddestinationx > 0 && PlayerShip->stargriddestinationy > 0 && PlayerShip->stargriddestinationx <= MAX_STARGRID_X_POSITIONS && PlayerShip->stargriddestinationy <= MAX_STARGRID_Y_POSITIONS) && ((PlayerShip->stargridpositionx != PlayerShip->stargriddestinationx) || (PlayerShip->stargridpositiony != PlayerShip->stargriddestinationy)) ) {
					if ( PlayerShip->AttemptWarp(PlayerShip->stargriddestinationx,PlayerShip->stargriddestinationy) ) {
						PlayerShip->is_attempting_warp = true;
						// BOYETTE STEAM INTEGRATION BEGIN
						//if ( common->m_pStatsAndAchievements ) {
						//	common->m_pStatsAndAchievements->m_flGameFeetTraveled = 600.0f;
						//}
						// BOYETTE STEAM INTEGRATION END
						StartSoundShader( declManager->FindSound("space_command_guisounds_ping_01") , SND_CHANNEL_ANY, 0, false, NULL );
						return true;
					} else {
						PlayerShip->is_attempting_warp = false;
					}
				} else {
					DisplayNonInteractiveAlertMessage( "^1No ^1Destination ^1Selected" );
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
				}
			} else {
				DisplayNonInteractiveAlertMessage( "^1Cannot ^1Warp ^1- ^1Captain ^1Chair ^1Empty" );
				StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
			}
			return true;
		}
		return true;
	}
	
	if ( PlayerShip ) {
		// x coordinate
		for ( int i = 1; i <= MAX_STARGRID_X_POSITIONS; i++ ) {
			if (token.Icmp( va( "click_x%i", i) ) == 0) {
				PlayerShip->stargriddestinationx = i;
				return true;
			}
		}
		// y coordinate
		for ( int i = 1; i <= MAX_STARGRID_Y_POSITIONS; i++ ) {
			if (token.Icmp( va( "click_y%i", i) ) == 0) {
				PlayerShip->stargriddestinationy = i;
				float distance_to_destination = (float)idMath::Sqrt( (PlayerShip->stargridpositionx - PlayerShip->stargriddestinationx)*(PlayerShip->stargridpositionx - PlayerShip->stargriddestinationx) + (PlayerShip->stargridpositiony - PlayerShip->stargriddestinationy)*(PlayerShip->stargridpositiony - PlayerShip->stargriddestinationy) );
				if ( distance_to_destination > 1 || distance_to_destination == 0 ) { // BOYETTE NOTE IMPORTANT: Comment out/change the appropriate bools here to enable warping anywhere on the stargrid
					PlayerShip->stargriddestinationx = MAX_STARGRID_X_POSITIONS + 10;
					PlayerShip->stargriddestinationy = MAX_STARGRID_Y_POSITIONS + 10;
					StartSoundShader( declManager->FindSound("space_command_guisounds_error_01") , SND_CHANNEL_ANY, 0, false, NULL );
					// BOYETTE NOTE TODO: play a custom not valid soundshader here on any channel.
				} else {
					StartSoundShader( declManager->FindSound("space_command_guisounds_beep_02") , SND_CHANNEL_ANY, 0, false, NULL );
					// BOYETTE NOTE TODO: play a custom valid soundshader here on any channel.
				}
				return true;
			}
		}
	}


	// boyette stargrid mod end

	// boyette mod end

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
			if ( ShipOnBoard ) { // BOYETTE NOTE: added this for various reasons. The locationEntity only exist when we are in the same room - otherwise it will just say str_02911 which is "Unidentified"
				hud->SetStateString( "location", ShipOnBoard->original_name );
			} else {
				hud->SetStateString( "location", common->GetLanguageDict()->GetString( "#str_02911" ) );
			}
		}
	}
}

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
	talkCursor		= 0;
	tabletfocusEnt = NULL;
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
	idAFEntity_Vehicle *oldVehicle;
	int			i, j;
	idVec3		start, end;
	bool		allowFocus;
	const char *command;
	trace_t		trace;
	guiPoint_t	pt;
	const idKeyValue *kv;
	sysEvent_t	ev;
	idUserInterface *ui;

	if ( guiOverlay ) {		
		return;		
	}

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
	oldVehicle		= focusVehicle;
	// boyette space command begin
	idEntity* oldtabletfocusEnt = tabletfocusEnt.GetEntity();
	// boyette space command end

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
	// boyette space command tablet begin
	if ( using_space_command_tablet ) { // or maybe use tablet_beam_active
		bounds.AddPoint( start + viewAngles.ToForward() * 256.0f );
	} else {
	// boyette space command tablet end
		bounds.AddPoint( end );
	// boyette space command tablet begin
	}
	// boyette space command tablet end

	listedClipModels = gameLocal.clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	// no pretense at sorting here, just assume that there will only be one active
	// gui within range along the trace
	for ( i = 0; i < listedClipModels; i++ ) {
		clip = clipModelList[ i ];
		ent = clip->GetEntity();

		if ( ent->IsHidden() ) {
			continue;
		}

		// boyette space command tablet begin
		if ( using_space_command_tablet && !focusGUIent && !talkCursor ) { // and if weapon player is using the captain tablet (not this too - and if the captain tablet is firing. )
			if ( ent->IsType( idAFAttachment::Type ) ) {
				idEntity *body = static_cast<idAFAttachment *>( ent )->GetBody();
				if ( body && body->IsType( idAI::Type ) ) {
					gameLocal.clip.TracePoint( trace, start, start + viewAngles.ToForward() * 256.0f, MASK_SHOT_RENDERMODEL, this );
					if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
						ClearFocus();
						tabletfocusEnt = static_cast<idAI *>( body );
						focusTime = gameLocal.time + FOCUS_TIME;
						if ( tabletfocusEnt.GetEntity() && oldtabletfocusEnt != tabletfocusEnt.GetEntity() && weapon.GetEntity() ) {
							weapon.GetEntity()->HandleNamedEventOnGuis( "UpdateScannedEntity" );
							UpdateSpaceCommandTabletGUIOnce();
						}
						if ( tabletfocusEnt.GetEntity() && oldtabletfocusEnt && oldtabletfocusEnt != tabletfocusEnt.GetEntity() && weapon.GetEntity() ) {
							weapon.GetEntity()->HandleNamedEventOnGuis( "ChangeScannedEntity" );
						}
						if ( tabletfocusEnt.GetEntity() && !oldtabletfocusEnt && weapon.GetEntity() ) {
							weapon.GetEntity()->HandleNamedEventOnGuis( "BeginScanningEntity" );
						}
					}
				}
			}

			if ( ent->IsType( idMoveablePDAItem::Type ) && ent->GetBindMaster() ) {
				idEntity *master = ent->GetBindMaster();
				if ( master && master->IsType( idAI::Type ) ) {
					gameLocal.clip.TracePoint( trace, start, start + viewAngles.ToForward() * 256.0f, MASK_SHOT_RENDERMODEL, this );
					if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
						ClearFocus();
						tabletfocusEnt = static_cast<idAI *>( master );
						focusTime = gameLocal.time + FOCUS_TIME;
						if ( tabletfocusEnt.GetEntity() && oldtabletfocusEnt != tabletfocusEnt.GetEntity() && weapon.GetEntity() ) {
							weapon.GetEntity()->HandleNamedEventOnGuis( "UpdateScannedEntity" );
							UpdateSpaceCommandTabletGUIOnce();
						}
						if ( tabletfocusEnt.GetEntity() && oldtabletfocusEnt && oldtabletfocusEnt != tabletfocusEnt.GetEntity() && weapon.GetEntity() ) {
							weapon.GetEntity()->HandleNamedEventOnGuis( "ChangeScannedEntity" );
						}
						if ( tabletfocusEnt.GetEntity() && !oldtabletfocusEnt && weapon.GetEntity() ) {
							weapon.GetEntity()->HandleNamedEventOnGuis( "BeginScanningEntity" );
						}
					}
				}
			}

			if ( ent->IsType( idAI::Type ) || ent->IsType( sbModule::Type ) || ent->IsType( sbConsole::Type ) ) {
				gameLocal.clip.TracePoint( trace, start, start + viewAngles.ToForward() * 256.0f, MASK_SHOT_RENDERMODEL, this );
				if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
					ClearFocus();
					tabletfocusEnt = ent;
					focusTime = gameLocal.time + FOCUS_TIME;
					if ( tabletfocusEnt.GetEntity() && oldtabletfocusEnt != tabletfocusEnt.GetEntity() && weapon.GetEntity() ) {
						weapon.GetEntity()->HandleNamedEventOnGuis( "UpdateScannedEntity" );
						UpdateSpaceCommandTabletGUIOnce();
					}
					if ( tabletfocusEnt.GetEntity() && oldtabletfocusEnt && oldtabletfocusEnt != tabletfocusEnt.GetEntity() && weapon.GetEntity() ) {
						weapon.GetEntity()->HandleNamedEventOnGuis( "ChangeScannedEntity" );
					}
					if ( tabletfocusEnt.GetEntity() && !oldtabletfocusEnt && weapon.GetEntity() ) {
						weapon.GetEntity()->HandleNamedEventOnGuis( "BeginScanningEntity" );
					}
				}
			}
			if ( ent->IsType( idDoor::Type ) ) {
				gameLocal.clip.TracePoint( trace, start, start + viewAngles.ToForward() * 256.0f, MASK_SHOT_RENDERMODEL, this );
				if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
					ClearFocus();
					tabletfocusEnt = ent;
					focusTime = gameLocal.time + FOCUS_TIME;
					// BOYETTE NOTE: If the focus entity is an idDoor we don't want anything to happen on the tablet GUI
					if ( ( oldtabletfocusEnt && tabletfocusEnt.GetEntity() && weapon.GetEntity() && tabletfocusEnt.GetEntity() != oldtabletfocusEnt ) && !(tabletfocusEnt.GetEntity() && oldtabletfocusEnt && tabletfocusEnt.GetEntity()->IsType(idDoor::Type) && oldtabletfocusEnt->IsType(idDoor::Type)) ) {
						weapon.GetEntity()->HandleNamedEventOnGuis( "LostScannedEntity" );
						UpdateSpaceCommandTabletGUIOnce();
					}
				}
			}
			
																				// BOYETTE NOTE: If the focus entity is an idDoor we don't want anything to happen on the tablet GUI
			if ( oldtabletfocusEnt && !tabletfocusEnt.GetEntity() && weapon.GetEntity() && !(oldtabletfocusEnt && oldtabletfocusEnt->IsType(idDoor::Type)) ) {
				weapon.GetEntity()->HandleNamedEventOnGuis( "LostScannedEntity" );
				UpdateSpaceCommandTabletGUIOnce();
			}
			if ( (oldFocus && !focusGUIent) || (oldTalkCursor && !talkCursor) || player_just_transported ) {
				if ( (!tabletfocusEnt.GetEntity() || tabletfocusEnt.GetEntity()->IsType( idDoor::Type ) ) && weapon.GetEntity() && !(oldtabletfocusEnt && oldtabletfocusEnt->IsType(idDoor::Type)) ) {
					weapon.GetEntity()->HandleNamedEventOnGuis( "LostScannedEntity" );
					UpdateSpaceCommandTabletGUIOnce();
				}
			}
			if ( player_just_transported) { player_just_transported = false; };
		}
		// boyette space command tablet end

		if ( allowFocus ) {
			if ( ent->IsType( idAFAttachment::Type ) ) {
				idEntity *body = static_cast<idAFAttachment *>( ent )->GetBody();
				if ( body && body->IsType( idAI::Type ) && ( static_cast<idAI *>( body )->GetTalkState() >= TALK_OK ) ) {
					gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
					if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
						ClearFocus();
						focusCharacter = static_cast<idAI *>( body );
						// BOYETTE SPACE COMMAND BEGIN
						if ( focusCharacter->uses_space_command_dialogue_behavior ) {
							if ( (focusCharacter->team == team || focusCharacter->waiting_until_player_talks_to_me) && focusCharacter->GetTalkState() == TALK_OK ) {
								talkCursor = 1; // DONE: BOYETTE NOTE TODO MAYBE - I think this is where we want to make sure that the talk cursor only appears if they are on the same team.
							} else {
								talkCursor = 0;
							}
						} else {
							if ( focusCharacter->GetTalkState() == TALK_OK ) {
								talkCursor = 1; // original
							} else {
								talkCursor = 0;
							}
						}
						// BOYETTE SPACE COMMAND END
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
						// BOYETTE SPACE COMMAND BEGIN
						if ( focusCharacter->uses_space_command_dialogue_behavior ) {
							if ( (focusCharacter->team == team || focusCharacter->waiting_until_player_talks_to_me) && focusCharacter->GetTalkState() == TALK_OK ) {
								talkCursor = 1; // DONE: BOYETTE NOTE TODO MAYBE - I think this is where we want to make sure that the talk cursor only appears if they are on the same team.
							} else {
								talkCursor = 0;
							}
						} else {
							if ( focusCharacter->GetTalkState() == TALK_OK ) {
								talkCursor = 1; // original
							} else {
								talkCursor = 0;
							}
						}
						// BOYETTE SPACE COMMAND END
						focusTime = gameLocal.time + FOCUS_TIME;
						break;
					}
				}
				continue;
			}

			if ( ent->IsType( idAFEntity_Vehicle::Type ) ) {
				gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
				if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == ent->entityNumber ) ) {
					ClearFocus();
					focusVehicle = static_cast<idAFEntity_Vehicle *>( ent );
					focusTime = gameLocal.time + FOCUS_TIME;
					break;
				}
				continue;
			}
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
//BSM: Added for powercells
				int powerCellCount = 0;
				for ( j = 0; j < inventory.items.Num(); j++ ) {
					idDict *item = inventory.items[ j ];
					if(item->GetInt("inv_powercell")) {
						powerCellCount++;
					}
				}
				focusUI->SetStateInt( "powercell_count", powerCellCount );

				int staminapercentage = ( int )( 100.0f * stamina / pm_stamina.GetFloat() );
				focusUI->SetStateString( "player_health", va("%i", health ) );
				focusUI->SetStateString( "player_stamina", va( "%i%%", staminapercentage ) );
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

	// BOYETTE NOTE BEGIN: this was the original doom 3 version - it didn't always work properly
	/*
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
	*/
	// BOYETTE NOTE END

	// BOYETTE NOTE BEGIN: This is our improved version:
	if ( focusGUIent && focusUI ) {
		if ( oldUI && oldUI != focusUI ) {
			command = oldUI->Activate( false, gameLocal.time );
			HandleGuiCommands( oldFocus, command );
			StartSound( "snd_guiexit", SND_CHANNEL_ANY, 0, false, NULL );
		}
		if ( !oldFocus || oldFocus != focusGUIent ) {
			command = focusUI->Activate( true, gameLocal.time );
			HandleGuiCommands( focusGUIent, command );
			StartSound( "snd_guienter", SND_CHANNEL_ANY, 0, false, NULL );
			// HideTip();
			// HideObjective();
		} else if ( !oldUI || oldUI != focusUI ) {
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
	// BOYETTE NOTE END

	if ( cursor && ( oldTalkCursor != talkCursor ) ) {
		cursor->SetStateInt( "talkcursor", talkCursor );
	}

	if ( oldChar != focusCharacter && hud ) {
		if ( focusCharacter ) {
			hud->SetStateString( "npc", focusCharacter->spawnArgs.GetString( "npc_name", "Joe" ) );
			//Use to code to update the npc action string to fix bug 1159
			hud->SetStateString( "npc_action", common->GetLanguageDict()->GetString( "#str_02036" ));
			hud->HandleNamedEvent( "showNPC" );
			// HideTip();
			// HideObjective();
		} else {
			hud->SetStateString( "npc", "" );
			hud->SetStateString( "npc_action", "" );
			hud->HandleNamedEvent( "hideNPC" );
		}
	}
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

	if ( guiOverlay || (!noclip && ( gameLocal.inCinematic || privateCameraView || gameLocal.GetCamera() || influenceActive == INFLUENCE_LEVEL2 || objectiveSystemOpen ) ) ) {
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
		} else {
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
	} else {
		if ( viewAngles.pitch > pm_maxviewpitch.GetFloat() ) {
			// don't let the player look down enough to see the shadow of his (non-existant) feet
			viewAngles.pitch = pm_maxviewpitch.GetFloat();
		} else if ( viewAngles.pitch < pm_minviewpitch.GetFloat() ) {
			// don't let the player look up more than 89 degrees
			viewAngles.pitch = pm_minviewpitch.GetFloat();
		}
	}

	// BOYETTE SPACE COMMAND BEGIN
		// BOYETTE NOTE TODO IMPORTANT: 02/13/2016 - what we really need is a form of angle collision detection - we need to know the previous yaw from the last frame and the current yaw to know what we should set it to here - this is available with viewAngles and cmdAngles - but this seems to work fine for now - as long as the frame rate is not extremely low.
	if ( CaptainChairSeatedIn ) {

		viewAngles.pitch = idMath::ClampFloat( CaptainChairSeatedIn->min_view_angles.pitch, CaptainChairSeatedIn->max_view_angles.pitch, viewAngles.pitch );

		//gameLocal.Printf( "Captain Chair Yaw BEGIN: %f min %f max Curr: %f \n", CaptainChairSeatedIn->min_view_angles.yaw, CaptainChairSeatedIn->max_view_angles.yaw, viewAngles.yaw );

		if ( CaptainChairSeatedIn->min_view_angles.yaw < CaptainChairSeatedIn->max_view_angles.yaw ) {

			if ( (CaptainChairSeatedIn->min_view_angles.yaw < 0 && CaptainChairSeatedIn->max_view_angles.yaw < 0 && viewAngles.yaw > 0) || (CaptainChairSeatedIn->min_view_angles.yaw > 0 && CaptainChairSeatedIn->max_view_angles.yaw > 0 && viewAngles.yaw < 0) ) {
				if ( viewAngles.yaw < CaptainChairSeatedIn->min_view_angles.yaw || viewAngles.yaw > CaptainChairSeatedIn->max_view_angles.yaw ) {
					if ( idMath::Fabs((idMath::Fabs(viewAngles.yaw) - idMath::Fabs(CaptainChairSeatedIn->min_view_angles.yaw))) < idMath::Fabs((idMath::Fabs(viewAngles.yaw) - idMath::Fabs(CaptainChairSeatedIn->max_view_angles.yaw))) ) {
						viewAngles.yaw = CaptainChairSeatedIn->min_view_angles.yaw;
					} else {
						viewAngles.yaw = CaptainChairSeatedIn->max_view_angles.yaw;
					}
				}
			}

			viewAngles.yaw = idMath::ClampFloat( CaptainChairSeatedIn->min_view_angles.yaw, CaptainChairSeatedIn->max_view_angles.yaw, viewAngles.yaw );
		} else {
			if ( viewAngles.yaw < 0 ) {
				viewAngles.yaw = idMath::ClampFloat( -180.f, CaptainChairSeatedIn->max_view_angles.yaw, viewAngles.yaw );
			} else {
				viewAngles.yaw = idMath::ClampFloat( CaptainChairSeatedIn->min_view_angles.yaw, 180.f, viewAngles.yaw );
			}
		}
		//gameLocal.Printf( "Captain Chair Yaw END: %f min %f max Curr: %f \n", CaptainChairSeatedIn->min_view_angles.yaw, CaptainChairSeatedIn->max_view_angles.yaw, viewAngles.yaw );
	}

		// BOYETTE NOTE TODO IMPORTANT: 02/13/2016 - what we really need is a form of angle collision detection - we need to know the previous yaw from the last frame and the current yaw to know what we should set it to here - this is available with viewAngles and cmdAngles - but this seems to work fine for now - as long as the frame rate is not extremely low.
	if ( ConsoleSeatedIn ) {

		viewAngles.pitch = idMath::ClampFloat( ConsoleSeatedIn->min_view_angles.pitch, ConsoleSeatedIn->max_view_angles.pitch, viewAngles.pitch );

		//gameLocal.Printf( "Console Yaw BEGIN: %f min %f max Curr: %f \n", ConsoleSeatedIn->min_view_angles.yaw, ConsoleSeatedIn->max_view_angles.yaw, viewAngles.yaw );

		if ( ConsoleSeatedIn->min_view_angles.yaw < ConsoleSeatedIn->max_view_angles.yaw ) {

			if ( (ConsoleSeatedIn->min_view_angles.yaw < 0 && ConsoleSeatedIn->max_view_angles.yaw < 0 && viewAngles.yaw > 0) || (ConsoleSeatedIn->min_view_angles.yaw > 0 && ConsoleSeatedIn->max_view_angles.yaw > 0 && viewAngles.yaw < 0) ) {
				if ( viewAngles.yaw < ConsoleSeatedIn->min_view_angles.yaw || viewAngles.yaw > ConsoleSeatedIn->max_view_angles.yaw ) {
					if ( idMath::Fabs((idMath::Fabs(viewAngles.yaw) - idMath::Fabs(ConsoleSeatedIn->min_view_angles.yaw))) < idMath::Fabs((idMath::Fabs(viewAngles.yaw) - idMath::Fabs(ConsoleSeatedIn->max_view_angles.yaw))) ) {
						viewAngles.yaw = ConsoleSeatedIn->min_view_angles.yaw;
					} else {
						viewAngles.yaw = ConsoleSeatedIn->max_view_angles.yaw;
					}
				}
			}

			viewAngles.yaw = idMath::ClampFloat( ConsoleSeatedIn->min_view_angles.yaw, ConsoleSeatedIn->max_view_angles.yaw, viewAngles.yaw );
		} else {
			if ( viewAngles.yaw < 0 ) {
				viewAngles.yaw = idMath::ClampFloat( -180.f, ConsoleSeatedIn->max_view_angles.yaw, viewAngles.yaw );
			} else {
				viewAngles.yaw = idMath::ClampFloat( ConsoleSeatedIn->min_view_angles.yaw, 180.f, viewAngles.yaw );
			}
		}
		//gameLocal.Printf( "Console Yaw END: %f min %f max Curr: %f \n", ConsoleSeatedIn->min_view_angles.yaw, ConsoleSeatedIn->max_view_angles.yaw, viewAngles.yaw );
	}
	// BOYETTE SPACE COMMAND END

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
	int base = idMath::FtoiFast( ( BASE_HEARTRATE + LOWHEALTH_HEARTRATE_ADJ ) - ( (float)health / 100.0f ) * LOWHEALTH_HEARTRATE_ADJ );
	int rate = idMath::FtoiFast( base + ( ZEROSTAMINA_HEARTRATE - base ) * ( 1.0f - stamina / pm_stamina.GetFloat() ) );
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
		heartRate = 135;
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

/*
==============
idPlayer::UpdateAir
==============
*/
void idPlayer::UpdateAir( void ) {	
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

	if ( PowerUpActive( ENVIROTIME ) ) {
		newAirless = false;
	}

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

void idPlayer::UpdatePowerupHud() {
	
	if ( health <= 0 ) {
		return;
	}

	if(lastHudPowerup != hudPowerup) {
		
		if(hudPowerup == -1) {
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

	if(hudPowerup != -1) {
		if(PowerUpActive(hudPowerup)) {
			int remaining = inventory.powerupEndTime[ hudPowerup ] - gameLocal.time;
			int filledbar = idMath::ClampInt( 0, hudPowerupDuration, remaining );
			
			if ( hud ) {
				hud->SetStateInt( "player_powerup", 100 * filledbar / hudPowerupDuration );
				hud->SetStateInt( "player_poweruptime", remaining / 1000 );
			}
		}
	}
}

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
			if ( *security == '\0' ) {
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
				/* // BOYETTE SPACE COMMAND: MOVED BELOW - WE WANT VIDEOS TO BE VISIBLE/PLAYABLE NO MATTER WHICH PROFILE IS SELECTED
				AddGuiPDAData( DECL_VIDEO, "listPDAVideo", pda, objectiveSystem );
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
					objectiveSystem->SetStateString( "PDAVideoVid", "textures/images_used_in_source/welcome.tga" ); // BOYETTE NOTE: was originally sound/vo/video/welcome.tga - which never existed. - it is the image displayed if the video cannot be found.
					objectiveSystem->SetStateString( "PDAVideoIcon", "textures/images_used_in_source/welcome.tga" ); // BOYETTE NOTE: was originally sound/vo/video/welcome.tga - which never existed. - it is the image displayed if the video cannot be found.
					objectiveSystem->SetStateString( "PDAVideoTitle", "" );
					objectiveSystem->SetStateString( "PDAVideoInfo", "" );
				}
				*/
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
					objectiveSystem->SetStateString( "PDAAudioIcon", "textures/images_used_in_source/welcome.tga" ); // BOYETTE NOTE: was originally sound/vo/video/welcome.tga - which never existed. - must be part of something they left unfinished
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
			objectiveSystem->SetStateString( "PDAEmailTitle", name );
			objectiveSystem->SetStateString( "PDAEmailText", data );
		}
// BOYETTE SPACE COMMAND BEGIN
		if ( j == 0 ) {
			AddGuiPDAData( DECL_VIDEO, "listPDAVideo", pda, objectiveSystem );
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
				objectiveSystem->SetStateString( "PDAVideoVid", "textures/images_used_in_source/welcome.tga" ); // BOYETTE NOTE: was originally sound/vo/video/welcome.tga - which never existed. - it is the image displayed if the video cannot be found.
				objectiveSystem->SetStateString( "PDAVideoIcon", "textures/images_used_in_source/welcome.tga" ); // BOYETTE NOTE: was originally sound/vo/video/welcome.tga - which never existed. - it is the image displayed if the video cannot be found.
				objectiveSystem->SetStateString( "PDAVideoTitle", "" );
				objectiveSystem->SetStateString( "PDAVideoInfo", "" );
			}
		}
// BOYETTE SPACE COMMAND END
	}
	if ( objectiveSystem->State().GetInt( "listPDA_sel_0", "-1" ) == -1 ) {
		objectiveSystem->SetStateInt( "listPDA_sel_0", 0 );
	}
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
			if ( inventory.weapons.test(j) ) {
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

/*
==============
idPlayer::UseVehicle
==============
*/
void idPlayer::UseVehicle( void ) {
	trace_t	trace;
	idVec3 start, end;
	idEntity *ent;

	if ( GetBindMaster() && GetBindMaster()->IsType( idAFEntity_Vehicle::Type ) ) {
		Show();
		static_cast<idAFEntity_Vehicle*>(GetBindMaster())->Use( this );
	} else {
		start = GetEyePosition();
		end = start + viewAngles.ToForward() * 80.0f;
		gameLocal.clip.TracePoint( trace, start, end, MASK_SHOT_RENDERMODEL, this );
		if ( trace.fraction < 1.0f ) {
			ent = gameLocal.entities[ trace.c.entityNum ];
			if ( ent && ent->IsType( idAFEntity_Vehicle::Type ) ) {
				Hide();
				static_cast<idAFEntity_Vehicle*>(ent)->Use( this );
			}
		}
	}
}

/*
==============
idPlayer::PerformImpulse
==============
*/
void idPlayer::PerformImpulse( int impulse ) {

	// BOYETTE TUTORIAL MODE BEGIN
	if ( wait_impulse >= 0 ) {
		if ( impulse == wait_impulse ) {
			wait_impulse = -1;
			SetInfluenceLevel( INFLUENCE_NONE );
		} else {
			return;
		}
	}
	// BOYETTE TUTORIAL MODE END

	if ( gameLocal.isClient ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		assert( entityNumber == gameLocal.localClientNum );
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( impulse, 6 );
		ClientSendEvent( EVENT_IMPULSE, &msg );
	}

	if ( impulse >= IMPULSE_0 && impulse <= IMPULSE_12 ) {
		SelectWeapon( impulse, false );
		return;
	}

	switch( impulse ) {
		case IMPULSE_13: {
			Reload();
			break;
		}
		case IMPULSE_14: {
			// boyette begin
			//if ( guiOverlay ) {
			//	RouteGuiMouseWheel( guiOverlay);
			//} else {
			// boyette end
				NextWeapon();
			//}
			break;
		}
		case IMPULSE_15: {
			// boyette begin
			//if ( guiOverlay ) {
			//	RouteGuiMouseWheel( guiOverlay );
			//} else {
			// boyette end
				PrevWeapon();
			//}
			break;
		}
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
			if ( !guiOverlay ) {
				if ( !gameLocal.isMultiplayer && !common->ShiftKeyIsDown() ) {
					if ( objectiveSystemOpen ) {
						TogglePDA();
					} else if ( weapon_pda >= 0 ) {
						SelectWeapon( weapon_pda, true );
					}
				}
			}
			break;
		}
		case IMPULSE_20: {
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
				gameLocal.mpGame.ToggleTeam();
			}
			break;
		}
		case IMPULSE_22: {
			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
				gameLocal.mpGame.ToggleSpectate();
			}
			break;
		}

		case IMPULSE_25: {
			//if ( gameLocal.isServer && gameLocal.mpGame.IsGametypeFlagBased() && (gameLocal.serverInfo.GetInt( "si_midnight" ) == 2) ) {
			if ( g_showHud.GetBool() ) { // BOYETTE NOTE: this was added for the outro cinematic
				if ( playerHeadLight.IsValid() ) {
					playerHeadLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
					playerHeadLight = NULL;
					//gameLocal.Printf( "Envirosuit Light Deactivated" );
				} else {
					const idDict *lightDef = gameLocal.FindEntityDefDict( "player_head_light", false );
					if ( lightDef ) {
						idEntity *temp = static_cast<idEntity *>(playerHeadLight.GetEntity());
						idAngles lightAng = firstPersonViewAxis.ToAngles();
						idVec3 lightOrg = firstPersonViewOrigin;

						playerHeadLightOffset = lightDef->GetVector( "player_head_light_offset" );
						playerHeadLightAngleOffset = lightDef->GetVector( "player_head_light_angle_offset" );

						gameLocal.SpawnEntityDef( *lightDef, &temp, false );
						playerHeadLight = static_cast<idLight *>(temp);

						playerHeadLight.GetEntity()->fl.networkSync = true;

						lightOrg += (playerHeadLightOffset.x * firstPersonViewAxis[0]);
						lightOrg += (playerHeadLightOffset.y * firstPersonViewAxis[1]);
						lightOrg += (playerHeadLightOffset.z * firstPersonViewAxis[2]);
						lightAng.pitch += playerHeadLightAngleOffset.x;
						lightAng.yaw += playerHeadLightAngleOffset.y;
						lightAng.roll += playerHeadLightAngleOffset.z;

						playerHeadLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
						playerHeadLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );

						playerHeadLight.GetEntity()->UpdateVisuals();
						playerHeadLight.GetEntity()->Present();
					}
					//gameLocal.Printf( "Envirosuit Light Activated" );
				}
			}
			//}
			break;
		}

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
		case IMPULSE_40: {
			UseVehicle();
			break;
		}
// boyette begin
		case IMPULSE_41: {
			if ( g_showHud.GetBool() ) { // BOYETTE NOTE: this was added for the outro cinematic
				if ( PlayerShip ) {
					if ( guiOverlay ) {
						if ( guiOverlay == CaptainGui ) {
							CloseOverlayCaptainGui();
						}
					} else {
						if ( ShipOnBoard && !ShipOnBoard->is_attempting_warp && allow_overlay_captain_gui && !objectiveSystemOpen && currentWeapon != weapon_pda ) {
							if ( ShipOnBoard->stargridpositionx != PlayerShip->stargridpositionx || ShipOnBoard->stargridpositiony != PlayerShip->stargridpositiony ) {
								DisplayNonInteractiveAlertMessage( "Your ship the ^4(the " + PlayerShip->name + ")^0 is not in local space. You cannot command it. But you're crew will try to work their way towards you." );
							} else {
								if ( tutorial_mode_on && (tutorial_mode_current_step == 5 || tutorial_mode_current_step >= 42) ) {
									return;
								}
								SetOverlayCaptainGui();
								if ( tutorial_mode_on && tutorial_mode_current_step == 0 ) {
									Event_IncreaseTutorialModeStep();
								}
							}
						}
					}
				}
			}
			break;
		}
		case IMPULSE_42: {
			if ( hud_map_visible ) {
				hud_map_visible = false;
			} else {
				hud_map_visible = true;
			}
			//hud_map_visible = !hud_map_visible; // cleaner - not sure if it would work
			break;
		}
// boyette end
		 //Hack so the chainsaw will work in MP
		case IMPULSE_27: {
			SelectWeapon(18, false);
			break;
		}
	} 
}

bool idPlayer::HandleESC( void ) {
	if ( gameLocal.inCinematic ) {
		return SkipCinematic();
	}

	if ( objectiveSystemOpen ) {
		TogglePDA();
		return true;
	}

	if ( guiOverlay && guiOverlay == CaptainGui ) {
		if ( tutorial_mode_on && ( (tutorial_mode_current_step >= 1 && tutorial_mode_current_step <= 4) || (tutorial_mode_current_step >= 6 && tutorial_mode_current_step <= 26) || (tutorial_mode_current_step >= 35 && tutorial_mode_current_step <= 42) ) ) {
			return false;
		} else {
			CloseOverlayCaptainGui();

			if ( tutorial_mode_on && tutorial_mode_current_step == 27 ) {
				Event_IncreaseTutorialModeStep();
			}
		}
		return true;
	}

	// boyette space command begin
	if ( CaptainChairSeatedIn ) {
		CaptainChairSeatedIn->ReleasePlayerCaptain();

		if ( tutorial_mode_on && tutorial_mode_current_step == 28 ) {
			Event_IncreaseTutorialModeStep();
		}
		return true;
	}

	if ( ConsoleSeatedIn ) {
		ConsoleSeatedIn->ReleasePlayerCaptain();
		return true;
	}
	// boyette space command end

	return false;
}

bool idPlayer::SkipCinematic( void ) {
	StartSound( "snd_skipcinematic", SND_CHANNEL_ANY, 0, false, NULL );
	return gameLocal.SkipCinematic();
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
	float rate;

	if ( spectating ) {
		speed = pm_spectatespeed.GetFloat();
		bobFrac = 0.0f;
	} else if ( noclip ) {
		speed = pm_noclipspeed.GetFloat();
		bobFrac = 0.0f;
	} else if ( !physicsObj.OnLadder() && ( usercmd.buttons & BUTTON_RUN ) && ( usercmd.forwardmove || usercmd.rightmove ) && ( usercmd.upmove >= 0 ) ) {
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
	} else {
		rate = pm_staminarate.GetFloat();
		
		// increase 25% faster when not moving
		if ( ( usercmd.forwardmove == 0 ) && ( usercmd.rightmove == 0 ) && ( !physicsObj.OnLadder() || ( usercmd.upmove == 0 ) ) ) {
			 rate *= 1.25f;
		}

		stamina += rate * MS2SEC( gameLocal.msec );
		if ( stamina > pm_stamina.GetFloat() ) {
			stamina = pm_stamina.GetFloat();
		}
		speed = pm_walkspeed.GetFloat();
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

	if ( noclip ) {
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
	} else if ( guiOverlay && physicsObj.HasGroundContacts() ) {
		physicsObj.SetContents( CONTENTS_BODY );
		physicsObj.SetMovementType( PM_DEAD );
	} else if ( mountedObject ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_FREEZE );
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

	if ( guiOverlay || noclip || gameLocal.inCinematic || ( influenceActive == INFLUENCE_LEVEL2 ) ) {
		AI_CROUCH	= false;
		AI_ONGROUND	= ( influenceActive == INFLUENCE_LEVEL2 );
		AI_ONLADDER	= false;
		AI_JUMP		= false;
	} else {
		AI_CROUCH	= physicsObj.IsCrouching();
		AI_ONGROUND	= physicsObj.HasGroundContacts();
		AI_ONLADDER	= physicsObj.OnLadder();
		AI_JUMP		= physicsObj.HasJumped();

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

	BobCycle( pushVelocity );
	CrashLand( oldOrigin, oldVelocity );
}

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

	int c = inventory.pickupItemNames.Num();
	if ( c > 0 ) {
		if ( gameLocal.time > inventory.nextItemPickup ) {
			if ( inventory.nextItemPickup && gameLocal.time - inventory.nextItemPickup > 2000 ) {
				inventory.nextItemNum = 1;
			}
			int i;

			int count = 5;
			if(gameLocal.isMultiplayer) {
				count = 3;
			}
			for ( i = 0; i < count, i < c; i++ ) { //_D3XP
				hud->SetStateString( va( "itemtext%i", inventory.nextItemNum ), inventory.pickupItemNames[0].name );
				hud->SetStateString( va( "itemicon%i", inventory.nextItemNum ), inventory.pickupItemNames[0].icon );
				hud->HandleNamedEvent( va( "itemPickup%i", inventory.nextItemNum++ ) );
				inventory.pickupItemNames.RemoveIndex( 0 );
				if (inventory.nextItemNum == 1 ) {
					inventory.onePickupTime = gameLocal.time;
				} else 	if ( inventory.nextItemNum > count ) { //_D3XP
					inventory.nextItemNum = 1;
					inventory.nextItemPickup = inventory.onePickupTime + 2000;
				} else {
					inventory.nextItemPickup = gameLocal.time + 400;
				}
			}
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

	hud->SetStateInt( "g_showProjectilePct", g_showProjectilePct.GetInteger() );
	if ( numProjectilesFired ) {
		hud->SetStateString( "projectilepct", va( "Hit %% %.1f", ( (float) numProjectileHits / numProjectilesFired ) * 100 ) );
	} else {
		hud->SetStateString( "projectilepct", "Hit % 0.0" );
	}

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
		return;
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

	if ( mountedObject ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}

	if ( objectiveSystemOpen || gameLocal.inCinematic || influenceActive ) {
		if ( objectiveSystemOpen && AI_PAIN ) {
			TogglePDA();
		}
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
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

	// zooming
	if ( ( usercmd.buttons ^ oldCmd.buttons ) & BUTTON_ZOOM ) {
		if ( ( usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() ) {
			zoomFov.Init( gameLocal.time, 200.0f, CalcFov( false ), weapon.GetEntity()->GetZoomFov() );
		} else {
			zoomFov.Init( gameLocal.time, 200.0f, zoomFov.GetCurrentValue( gameLocal.time ), DefaultFov() );
		}
	}

	// if we have an active gui, we will unrotate the view angles as
	// we turn the mouse movements into gui events
	idUserInterface *gui = ActiveGui();
	if ( gui && gui != focusUI && !guiOverlay ) {
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
			float scale = new_g_damageScale;
			if ( g_useDynamicProtection.GetBool() && scale < 1.0f && gameLocal.time - lastDmgTime > 500 ) {
				if ( scale < 1.0f ) {
					scale += 0.05f;
				}
				if ( scale > 1.0f ) {
					scale = 1.0f;
				}
				new_g_damageScale = scale;
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

	UpdateAir();

	UpdatePowerupHud();
	
	UpdateHud();

	UpdatePowerUps();

	UpdateDeathSkin( false );

	if ( gameLocal.isMultiplayer ) {
		DrawPlayerIcons();

		if ( playerHeadLight.IsValid() ) {
			idAngles lightAng = firstPersonViewAxis.ToAngles();
			idVec3 lightOrg = firstPersonViewOrigin;
			const idDict *lightDef = gameLocal.FindEntityDefDict( "player_head_light", false );

			playerHeadLightOffset = lightDef->GetVector( "player_head_light_offset" );
			playerHeadLightAngleOffset = lightDef->GetVector( "player_head_light_angle_offset" );

			lightOrg += (playerHeadLightOffset.x * firstPersonViewAxis[0]);
			lightOrg += (playerHeadLightOffset.y * firstPersonViewAxis[1]);
			lightOrg += (playerHeadLightOffset.z * firstPersonViewAxis[2]);
			lightAng.pitch += playerHeadLightAngleOffset.x;
			lightAng.yaw += playerHeadLightAngleOffset.y;
			lightAng.roll += playerHeadLightAngleOffset.z;

			playerHeadLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
			playerHeadLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
			playerHeadLight.GetEntity()->UpdateVisuals();
			playerHeadLight.GetEntity()->Present();
		}
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

	inventory.RechargeAmmo(this);

	if(healthRecharge) {
		int elapsed = gameLocal.time - lastHealthRechargeTime;
		if(elapsed >= rechargeSpeed) {
			int intervals = (gameLocal.time - lastHealthRechargeTime)/rechargeSpeed;
			Give("health", va("%d", intervals));
			lastHealthRechargeTime += intervals*rechargeSpeed;
		}
	}

	// determine if portal sky is in pvs
	gameLocal.portalSkyActive = gameLocal.pvs.CheckAreasForPortalSky( gameLocal.GetPlayerPVS(), GetPhysics()->GetOrigin() );


	// Boyette space command begin
	// boyette mod update very frame
	UpdateCaptainMenuEveryFrame();
	UpdateCaptainHudEveryFrame();
	UpdateSpaceCommandTabletEveryFrame();

	UpdatePlayerHeadLightEveryFrame();
	// Boyette space command end
}

/*
=====================
idPlayer::UpdatePlayerHeadLightEveryFrame
=====================
*/
void idPlayer::UpdatePlayerHeadLightEveryFrame() {
	if ( playerHeadLight.IsValid() ) {
		idAngles lightAng = firstPersonViewAxis.ToAngles();
		idVec3 lightOrg = firstPersonViewOrigin;
		const idDict *lightDef = &playerHeadLight.GetEntity()->spawnArgs;

		//playerHeadLightOffset = lightDef->GetVector( "player_head_light_offset" ); // BOYETTE NOTE: this get set when the light is activated so it doesn't need to search for the spawnarg every frame
		//playerHeadLightAngleOffset = lightDef->GetVector( "player_head_light_angle_offset" ); // BOYETTE NOTE: this get set when the light is activated so it doesn't need to search for the spawnarg every frame

		lightOrg += (playerHeadLightOffset.x * firstPersonViewAxis[0]);
		lightOrg += (playerHeadLightOffset.y * firstPersonViewAxis[1]);
		lightOrg += (playerHeadLightOffset.z * firstPersonViewAxis[2]);
		lightAng.pitch += playerHeadLightAngleOffset.x;
		lightAng.yaw += playerHeadLightAngleOffset.y;
		lightAng.roll += playerHeadLightAngleOffset.z;

		playerHeadLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
		playerHeadLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
		playerHeadLight.GetEntity()->UpdateVisuals();
		playerHeadLight.GetEntity()->Present();
	}
}

/*
=====================
idPlayer::UpdateSpaceCommandTabletEveryFrame
=====================
*/
void idPlayer::UpdateSpaceCommandTabletEveryFrame() {
	if ( using_space_command_tablet ) {
		if ( weapon.GetEntity() ) {
			weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists", tabletfocusEnt.GetEntity() );
			weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_beam_active", space_command_tablet_beam_active );
			weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_hostile", tabletfocusEnt.GetEntity() && tabletfocusEnt.GetEntity()->team != team );

			if ( tabletfocusEnt.GetEntity() && !tabletfocusEnt.GetEntity()->IsType( idDoor::Type ) ) {
				weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_health", tabletfocusEnt.GetEntity()->health );
				weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_max_health", tabletfocusEnt.GetEntity()->entity_max_health );
				weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_health_info_text", idStr(tabletfocusEnt.GetEntity()->health) + " / " + idStr(tabletfocusEnt.GetEntity()->entity_max_health) );
				weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_health_percentage", (float)tabletfocusEnt.GetEntity()->health / (float)tabletfocusEnt.GetEntity()->entity_max_health );
				if ( tabletfocusEnt.GetEntity()->idEntity::team == team) {
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_hostility", "^0Non-Hostile" );
				} else {
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_hostility", "^1Hostile" );
				}

				if ( tabletfocusEnt.GetEntity()->IsType( sbModule::Type ) ) {
					sbModule* scan_Module = static_cast<sbModule*>( tabletfocusEnt.GetEntity() );

					if ( ShipOnBoard ) {
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", (float)scan_Module->power_allocated / MAX_POSSIBLE_MODULE_POWER );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", (float)scan_Module->max_power / MAX_POSSIBLE_MODULE_POWER );
						weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", scan_Module->module_efficiency );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_module_is_buffed", scan_Module->module_buffed_amount > 0 );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", scan_Module->current_charge_percentage );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_automanage_power_on", ShipOnBoard->modules_power_automanage_on );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", 0.0f );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", 0.0f );
						weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", 0 );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_module_is_buffed", false );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", 0.0f );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_automanage_power_on", false );
					}
				} else if ( tabletfocusEnt.GetEntity()->IsType( sbConsole::Type ) ) {
					sbConsole* scan_Console = static_cast<sbConsole*>( tabletfocusEnt.GetEntity() );
					if ( ShipOnBoard ) {
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_console_is_manned", scan_Console->console_occupied > 0 );
						if ( scan_Console->ControlledModule ) {
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", (float)scan_Console->ControlledModule->power_allocated / MAX_POSSIBLE_MODULE_POWER );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", (float)scan_Console->ControlledModule->max_power / MAX_POSSIBLE_MODULE_POWER );
							weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", scan_Console->ControlledModule->module_efficiency );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", scan_Console->ControlledModule->current_charge_percentage );
						} else {
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", 0 );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", 0 );
							weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", 0 );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", 0 );
						}
					} else {
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_console_is_manned", false );
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_console_module_name", "Unknown" );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", 0 );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", 0 );
						weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", 0 );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", 0 );
					}
				} else if ( tabletfocusEnt.GetEntity()->IsType( idAI::Type ) ) {
					idAI* scan_AI = static_cast<idAI*>( tabletfocusEnt.GetEntity() );
					if ( scan_AI->ParentShip ) {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", scan_AI->ParentShip->original_name );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", "Unknown" );
					}
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_vocation", scan_AI->spawnArgs.GetString( "space_command_vocation", "Explorer/Scientist" ) );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_tech_skill_rating", scan_AI->spawnArgs.GetString( "module_buff_amount", "20" ) );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_armament", scan_AI->spawnArgs.GetString( "space_command_armament", "Basic Plasma Weapon" ) );
				}

			} else {
				if ( tabletfocusEnt.GetEntity() && tabletfocusEnt.GetEntity()->IsType( idDoor::Type ) ) {
					weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_door_five_health_chits", idMath::ClampInt(0,5,idMath::Ceil( ((float)tabletfocusEnt.GetEntity()->health / (float)tabletfocusEnt.GetEntity()->entity_max_health ) * 5.0f)) );
					idDoor* scan_Door = static_cast<idDoor*>( tabletfocusEnt.GetEntity() );
				}

				if ( ShipOnBoard ) {
					weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_hullstrength", ShipOnBoard->hullStrength );
					weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_max_hullstrength", ShipOnBoard->max_hullStrength );
					weapon.GetEntity()->SetRenderEntityGuisFloats( "ship_on_board_hullstrength_percentage", (float)ShipOnBoard->hullStrength / (float)ShipOnBoard->max_hullStrength );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_hullstrength_text", idStr(ShipOnBoard->hullStrength) + " / " + idStr(ShipOnBoard->max_hullStrength) );

					weapon.GetEntity()->SetRenderEntityGuisBools( "ship_on_board_shields_raised", ShipOnBoard->shields_raised );
					weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_max_shieldstrength", ShipOnBoard->max_shieldStrength );
					if ( ShipOnBoard->shields_raised ) {
						weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_shieldstrength", ShipOnBoard->shieldStrength );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "ship_on_board_shieldstrength_percentage", (float)ShipOnBoard->shieldStrength / (float)ShipOnBoard->max_shieldStrength );
						weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_shieldstrength_text", idStr(ShipOnBoard->shieldStrength) + " / " + idStr(ShipOnBoard->max_shieldStrength) );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_shieldstrength_lowered", ShipOnBoard->shieldStrength_copy );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "ship_on_board_shieldstrength_percentage_lowered", (float)ShipOnBoard->shieldStrength_copy / (float)ShipOnBoard->max_shieldStrength );
						weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_shieldstrength_text_lowered", idStr(ShipOnBoard->shieldStrength_copy) + " / " + idStr(ShipOnBoard->max_shieldStrength) + " lwrd" );
					}

					weapon.GetEntity()->SetRenderEntityGuisBools( "ship_on_board_is_derelict", ShipOnBoard->is_derelict );

					if ( ShipOnBoard->idEntity::team == team) {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_diplomatic_status", "^0Non-Hostile" );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_diplomatic_status", "^1Hostile" );
					}

					if ( ShipOnBoard->current_oxygen_level >= 98 ) {
						weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_current_oxygen_level", 100.0f );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_current_oxygen_level", ( (float)ShipOnBoard->current_oxygen_level / (float)100 ) * 100.0f );
					}
					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_maximum_power_reserves", idStr(ShipOnBoard->maximum_power_reserve) + " terajoules" );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_number_of_ai_entities_detected_on_board", idStr((unsigned)ShipOnBoard->AIsOnBoard.size()) );
					weapon.GetEntity()->SetRenderEntityGuisBools( "ship_on_board_is_at_red_alert", ShipOnBoard->red_alert );
				}
			}
		}																																															// If the current beam target and the new beam target are the same OR the current beam target and the new beam target are both doors - don't deactivate the beam
		if ( space_command_tablet_beam_active && tabletfocusEnt.GetEntity() && SpaceCommandTabletBeam.target.GetEntity() != NULL && ( SpaceCommandTabletBeam.target.GetEntity() == tabletfocusEnt.GetEntity() || (SpaceCommandTabletBeam.target.GetEntity()->IsType(idDoor::Type) && tabletfocusEnt.GetEntity()->IsType(idDoor::Type) && static_cast<idDoor*>(SpaceCommandTabletBeam.target.GetEntity())->IsRelatedDoor(tabletfocusEnt.GetEntity()) ) ) ) {

			idVec3				org;
		
			if ( tabletfocusEnt.GetEntity()->IsType( idAI::Type ) ) {
				org = SpaceCommandTabletBeam.target.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter();
				org.z = SpaceCommandTabletBeam.target.GetEntity()->GetPhysics()->GetOrigin().z + EyeHeight();
			} else  if ( tabletfocusEnt.GetEntity()->IsType( sbModule::Type ) || tabletfocusEnt.GetEntity()->IsType( sbConsole::Type ) ) {
				org = SpaceCommandTabletBeam.target.GetEntity()->GetPhysics()->GetAbsBounds().GetCenter();
			} else if  ( tabletfocusEnt.GetEntity()->IsType( idDoor::Type ) ) {
				org = SpaceCommandTabletBeam.target.GetEntity()->GetPhysics()->GetOrigin();
				org.z = SpaceCommandTabletBeam.target.GetEntity()->GetPhysics()->GetOrigin().z + EyeHeight();
			}
		
			SpaceCommandTabletBeam.renderEntity.origin = GetPhysics()->GetOrigin() + (viewAngles.ToForward() * 32.0f) + idVec3(0,0,EyeHeight()-15.0f); // so it looks like it is coming from the back of the tablet.
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_X ] = org.x;
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_Y ] = org.y;
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_BEAM_END_Z ] = org.z;
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_RED ] = 
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_GREEN ] = 
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_BLUE ] = 
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
			gameRenderWorld->UpdateEntityDef( SpaceCommandTabletBeam.modelDefHandle, &SpaceCommandTabletBeam.renderEntity );

			UpdateVisuals();
		} else if ( space_command_tablet_beam_active ) {
			DeactivateSpaceCommandTabletBeam();
		}
	}
}

/*
=====================
idPlayer::UpdateSpaceCommandTabletGUIOnce
=====================
*/
void idPlayer::UpdateSpaceCommandTabletGUIOnce() {
	if ( using_space_command_tablet ) {
		if ( weapon.GetEntity() ) {

			weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists", tabletfocusEnt.GetEntity() );
			weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_beam_active", space_command_tablet_beam_active );
			weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_hostile", tabletfocusEnt.GetEntity() && tabletfocusEnt.GetEntity()->team != team );

			if ( tabletfocusEnt.GetEntity() && !tabletfocusEnt.GetEntity()->IsType( idDoor::Type ) ) {
				weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists_and_is_not_door", true );

				weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_health", tabletfocusEnt.GetEntity()->health );
				weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_max_health", tabletfocusEnt.GetEntity()->entity_max_health );
				weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_health_info_text", idStr(tabletfocusEnt.GetEntity()->health) + " / " + idStr(tabletfocusEnt.GetEntity()->entity_max_health) );
				weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_health_percentage", (float)tabletfocusEnt.GetEntity()->health / (float)tabletfocusEnt.GetEntity()->entity_max_health );
				if ( tabletfocusEnt.GetEntity()->idEntity::team == team) {
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_hostility", "^0Non-Hostile" );
				} else {
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_hostility", "^1Hostile" );
				}
				weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_space_command_description", tabletfocusEnt.GetEntity()->spawnArgs.GetString( "space_command_tablet_description", "NO INFO AVAILABLE" ) );
				// BOYETTE NOTE TODO: can have a description here from a spawnarg called "space_command_description" - which should be an appropriately sci fi type description of the entity that will show up on the tablet when it is scanned. also do "npc_name", "health", "max_health" which we can call "vitality", "faction", "species", "disposition", "armament", "parentship", "mass", "team_info", "accuracy", "ship_diagram_portrait", "ship_diagram_icon", computer skill which would be "module_buff_amount", entity type, if it is a console,module or ai show appropriate icon

				if ( tabletfocusEnt.GetEntity()->IsType( sbModule::Type ) ) {
					sbModule* scan_Module = static_cast<sbModule*>( tabletfocusEnt.GetEntity() );

					if ( ShipOnBoard ) {
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_module", true );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_console", false );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_ai", false );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_door", false );
						bool match_found = false;
						for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
							if ( ShipOnBoard->consoles[i] && ShipOnBoard->consoles[i]->ControlledModule && ShipOnBoard->consoles[i]->ControlledModule == scan_Module ) {
								weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", module_description_upper[i] + " Module" );
								weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_portrait", "guis/assets/steve_captain_display/" + module_description_upper[i] + "ModuleIconLarge.tga" );
								weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", ShipOnBoard->original_name );
								weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", (float)scan_Module->power_allocated / MAX_POSSIBLE_MODULE_POWER );
								weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", (float)scan_Module->max_power / MAX_POSSIBLE_MODULE_POWER );
								weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", scan_Module->module_efficiency );
								weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_module_is_buffed", scan_Module->module_buffed_amount > 0 );
								weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", scan_Module->current_charge_percentage );
								weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_automanage_power_on", ShipOnBoard->modules_power_automanage_on );

								match_found = true;
								break;
							}
						}
						if ( !match_found ) {
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", "Unknown Module" );
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_portrait", ""); 
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", "Unknown" );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", 0.0f );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", 0.0f );
							weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", 0 );
							weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_module_is_buffed", false );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", 0.0f );
							weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_automanage_power_on", false );
						}
					} else {
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", "Unknown Module" );
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_portrait", ""); 
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", "Unknown" );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", 0.0f );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", 0.0f );
							weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", 0 );
							weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_module_is_buffed", false );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", 0.0f );
							weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_automanage_power_on", false );
					}
				} else if ( tabletfocusEnt.GetEntity()->IsType( sbConsole::Type ) ) {
					sbConsole* scan_Console = static_cast<sbConsole*>( tabletfocusEnt.GetEntity() );
					if ( ShipOnBoard ) {
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_module", false );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_console", true );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_ai", false );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_door", false );
						bool match_found = false;
						for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
							if ( ShipOnBoard->consoles[i] && ShipOnBoard->consoles[i] == scan_Console ) {
								weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", module_description_upper[i] + " Console" );
								weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_portrait", "guis/assets/steve_captain_display/ConsoleIconLarge.tga" );
								declManager->FindMaterial("guis/assets/steve_captain_display/ConsoleIconLarge.tga")->SetSort(SS_GUI);
								weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", ShipOnBoard->original_name );
								weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_console_is_manned", scan_Console->console_occupied > 0 );

								if ( scan_Console->ControlledModule ) {
									weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_console_module_name", module_description_upper[i] + " Module" );
									weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", (float)scan_Console->ControlledModule->power_allocated / MAX_POSSIBLE_MODULE_POWER );
									weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", (float)scan_Console->ControlledModule->max_power / MAX_POSSIBLE_MODULE_POWER );
									weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", scan_Console->ControlledModule->module_efficiency );
									weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", scan_Console->ControlledModule->current_charge_percentage );
								} else {
									weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_console_module_name", "Unknown" );
									weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", 0 );
									weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", 0 );
									weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", 0 );
									weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", 0 );
								}
								match_found = true;
								break;
							}
						}
						if ( !match_found ) {
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", "Unknown Console" );
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_portrait", "guis/assets/steve_captain_display/ConsoleIconLarge.tga");
							declManager->FindMaterial("guis/assets/steve_captain_display/ConsoleIconLarge.tga")->SetSort(SS_GUI);
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", "Unknown" );
							weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_console_is_manned", false );
							weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_console_module_name", "Unknown" );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", 0 );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", 0 );
							weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", 0 );
							weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", 0 );
						}
					} else {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", "Unknown Console" );
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_portrait", "guis/assets/steve_captain_display/ConsoleIconLarge.tga");
						declManager->FindMaterial("guis/assets/steve_captain_display/ConsoleIconLarge.tga")->SetSort(SS_GUI);
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", "Unknown" );
						weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_console_is_manned", false );
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_console_module_name", "Unknown" );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_current_power_percentage", 0 );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_module_max_power_percentage", 0 );
						weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_module_efficiency", 0 );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "tablet_focus_entity_current_charge_percentage", 0 );
					}
				} else if ( tabletfocusEnt.GetEntity()->IsType( idAI::Type ) ) {
					weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_module", false );
					weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_console", false );
					weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_ai", true );
					weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_door", false );
					idAI* scan_AI = static_cast<idAI*>( tabletfocusEnt.GetEntity() );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_portrait", scan_AI->ship_diagram_portrait );
					declManager->FindMaterial(scan_AI->ship_diagram_portrait)->SetSort(SS_GUI);
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", scan_AI->spawnArgs.GetString( "npc_name", "Joe" ) );
					if ( scan_AI->ParentShip ) {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", scan_AI->ParentShip->original_name );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_parentship", "Unknown" );
					}
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_species", scan_AI->spawnArgs.GetString( "space_command_species", "Homo Sapiens" ) );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_mass", scan_AI->spawnArgs.GetString( "mass", "150" ) + idStr(" Kilograms") );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_vocation", scan_AI->spawnArgs.GetString( "space_command_vocation", "Explorer/Scientist" ) );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_tech_skill_rating", scan_AI->spawnArgs.GetString( "module_buff_amount", "20" ) );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_armament", scan_AI->spawnArgs.GetString( "space_command_armament", "Basic Plasma Weapon" ) );
				}

			} else {
				weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists_and_is_not_door", false );

				if ( tabletfocusEnt.GetEntity() && tabletfocusEnt.GetEntity()->IsType( idDoor::Type ) ) {
					weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_module", false );
					weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_console", false );
					weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_ai", false );
					weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_is_door", true );

					weapon.GetEntity()->SetRenderEntityGuisInts( "tablet_focus_entity_door_five_health_chits", idMath::ClampInt(0,5,idMath::Ceil( ((float)tabletfocusEnt.GetEntity()->health / (float)tabletfocusEnt.GetEntity()->entity_max_health ) * 5.0f)) );

					idDoor* scan_Door = static_cast<idDoor*>( tabletfocusEnt.GetEntity() );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_portrait", "guis/assets/steve_captain_display/ShipDoorIcon.tga");
					declManager->FindMaterial("guis/assets/steve_captain_display/ShipDoorIcon.tga")->SetSort(SS_GUI);
					if ( ShipOnBoard ) {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", ShipOnBoard->original_name + " Door" );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "tablet_focus_entity_name", "Door" );
					}
					// NOT NECESSARY - BOYETTE NOTE TODO - SHOW LOCKED STATUS - SAME TEAM AND HEALTH ABOVE ZERO IS SECURE - DIFFERENT TEAM AND HEALTH ABOVE ZERO IS LOCKED - HEALTH EQUAL TO OR BELOW ZERO IS ALWAYS DAMAGED
					// NOT NECESSARY - BOYETTE NOTE TODO - SHOW DAMAGE ADJUSTED MAX HEALTH ON THE DOORS
				}

				if ( ShipOnBoard ) {
					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_name", ShipOnBoard->original_name );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_portrait", ShipOnBoard->ShipImageVisual );

					weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_health", ShipOnBoard->health );
					weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_hullstrength", ShipOnBoard->hullStrength );
					weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_max_hullstrength", ShipOnBoard->max_hullStrength );
					weapon.GetEntity()->SetRenderEntityGuisFloats( "ship_on_board_hullstrength_percentage", (float)ShipOnBoard->hullStrength / (float)ShipOnBoard->max_hullStrength );
					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_hullstrength_text", idStr(ShipOnBoard->hullStrength) + " / " + idStr(ShipOnBoard->max_hullStrength) );

					weapon.GetEntity()->SetRenderEntityGuisBools( "ship_on_board_shields_raised", ShipOnBoard->shields_raised );
					weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_max_shieldstrength", ShipOnBoard->max_shieldStrength );
					if ( ShipOnBoard->shields_raised ) {
						weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_shieldstrength", ShipOnBoard->shieldStrength );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "ship_on_board_shieldstrength_percentage", (float)ShipOnBoard->shieldStrength / (float)ShipOnBoard->max_shieldStrength );
						weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_shieldstrength_text", idStr(ShipOnBoard->shieldStrength) + " / " + idStr(ShipOnBoard->max_shieldStrength) );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_shieldstrength_lowered", ShipOnBoard->shieldStrength_copy );
						weapon.GetEntity()->SetRenderEntityGuisFloats( "ship_on_board_shieldstrength_percentage_lowered", (float)ShipOnBoard->shieldStrength_copy / (float)ShipOnBoard->max_shieldStrength );
						weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_shieldstrength_text_lowered", idStr(ShipOnBoard->shieldStrength_copy) + " / " + idStr(ShipOnBoard->max_shieldStrength) + " lwrd" );
					}

					weapon.GetEntity()->SetRenderEntityGuisBools( "ship_on_board_is_derelict", ShipOnBoard->is_derelict );

					if ( ShipOnBoard->idEntity::team == team) {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_diplomatic_status", "^0Non-Hostile" );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_diplomatic_status", "^1Hostile" );
					}

					if ( ShipOnBoard->current_oxygen_level >= 98 ) {
						weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_current_oxygen_level", 100.0f );
					} else {
						weapon.GetEntity()->SetRenderEntityGuisInts( "ship_on_board_current_oxygen_level", ( (float)ShipOnBoard->current_oxygen_level / (float)100 ) * 100.0f );
					}

					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_maximum_power_reserves", idStr(ShipOnBoard->maximum_power_reserve) + " terajoules" );

					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_number_of_ai_entities_detected_on_board", idStr((unsigned)ShipOnBoard->AIsOnBoard.size()) );

					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_classification", ShipOnBoard->spawnArgs.GetString( "space_command_classification_description", "Unknown Classification") );

					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_atmosphere", ShipOnBoard->spawnArgs.GetString( "space_command_atmosphere_description", "78.09% nitrogen, 20.95% oxygen, 0.93% argon, 0.039% carbon dioxide") );

					weapon.GetEntity()->SetRenderEntityGuisBools( "ship_on_board_is_at_red_alert", ShipOnBoard->red_alert );

					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_space_command_description", ShipOnBoard->spawnArgs.GetString( "space_command_tablet_description", "NO INFO AVAILABLE" ) );

					// relationship
					// current_oxygen_level
					// maximum power reserves
					// number of crew - or maybe just if there is crew - or maybe use AIsOnBoard

					// number of modules
					// number of consoles
					// red alert status or maybe alarm status
					// ship_is_firing
					// shield can be transported trhough - use min_shields_percent_for_blocking_foreign_transporters
					// some sci fi descriptions of variable atmospheric descriptions and chemicals
					// some sci fi description of the enitity in general - the ship "class" e.g. Constellation Mark IV

					// figure out whether the portrait should be the ship image visual 	or the ShipStargridIcon - or maybe the ship diagram
				} else {
					weapon.GetEntity()->SetRenderEntityGuisStrings( "ship_on_board_name", "No Data Available" );
				}
			}
		}
	}
}

/*
=====================
idPlayer::ActivateSpaceCommandTabletBeam
=====================
*/
void idPlayer::ActivateSpaceCommandTabletBeam() {
	if ( tabletfocusEnt.GetEntity() && !tabletfocusEnt.GetEntity()->IsType(idAI::Type) ) { // added this AI check on 07 22 2016 so that the player can heal or damage AI's with the tablet
		// to create the beam
		if ( space_command_tablet_beam_active == false ) {
			memset( &SpaceCommandTabletBeam.renderEntity, 0, sizeof( renderEntity_t ) );
			//if ( DefAttach.GetEntity() ) {
			//	SpaceCommandTabletBeam.renderEntity.origin = DefAttach.GetEntity()->GetPhysics()->GetOrigin(); // if they have a tricorder
			//} else {
				SpaceCommandTabletBeam.renderEntity.origin = GetPhysics()->GetOrigin() + (viewAngles.ToForward() * 32.0f) + idVec3(0,0,EyeHeight()-15.0f);
			//}
			SpaceCommandTabletBeam.renderEntity.axis = GetPhysics()->GetAxis();
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_BEAM_WIDTH ] = spawnArgs.GetFloat( "beam_width", "24.0" );
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_RED ] = 1.0f;
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1.0f;
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_BLUE ] = 1.0f;
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_ALPHA ] = 1.0f;
			SpaceCommandTabletBeam.renderEntity.shaderParms[ SHADERPARM_DIVERSITY] = gameLocal.random.CRandomFloat() * 0.75;
			SpaceCommandTabletBeam.renderEntity.hModel = renderModelManager->FindModel( "_beam" );
			SpaceCommandTabletBeam.renderEntity.callback = NULL;
			SpaceCommandTabletBeam.renderEntity.numJoints = 0;
			SpaceCommandTabletBeam.renderEntity.joints = NULL;
			SpaceCommandTabletBeam.renderEntity.bounds.Clear();
			if ( tabletfocusEnt.GetEntity()->idEntity::team == team ) {
				SpaceCommandTabletBeam.renderEntity.customSkin = declManager->FindSkin( spawnArgs.GetString( "friendly_beam_skin", "skins/space_command/friendly_beam_default") );
				StartSoundShader( declManager->FindSound( spawnArgs.GetString("snd_repair_beam", "spaceship_crew_repair_beam_default") ), SND_CHANNEL_WEAPON, 0, false, NULL );
				if ( weapon.GetEntity() ) {
					weapon.GetEntity()->projectileDict.Set("def_damage",weapon.GetEntity()->projectileDict.GetString("def_damage_friendly",""));
				}
			} else {
				SpaceCommandTabletBeam.renderEntity.customSkin = declManager->FindSkin( spawnArgs.GetString( "hostile_beam_skin", "skins/space_command/hostile_beam_default") );
				StartSoundShader( declManager->FindSound( spawnArgs.GetString("snd_damage_beam", "spaceship_crew_damage_beam_default") ), SND_CHANNEL_WEAPON, 0, false, NULL );
				if ( weapon.GetEntity() ) {
					weapon.GetEntity()->projectileDict.Set("def_damage",weapon.GetEntity()->projectileDict.GetString("def_damage_hostile",""));
				}
			}
			SpaceCommandTabletBeam.modelDefHandle = gameRenderWorld->AddEntityDef( &SpaceCommandTabletBeam.renderEntity );
			space_command_tablet_beam_active = true;
			// to set the target of the beam
			SpaceCommandTabletBeam.target = tabletfocusEnt.GetEntity();//put module origin plus some height above the origin so the beam is not going into the ground.
		}
	}
}

/*
=====================
idPlayer::DeactivateSpaceCommandTabletBeam
=====================
*/
void idPlayer::DeactivateSpaceCommandTabletBeam() {
	// to remove the beam
	if ( space_command_tablet_beam_active ) {
		space_command_tablet_beam_active = false;
		gameRenderWorld->FreeEntityDef( SpaceCommandTabletBeam.modelDefHandle );
		SpaceCommandTabletBeam.modelDefHandle = -1;
		StopSound(SND_CHANNEL_WEAPON,false);
		UpdateVisuals();
		gameLocal.Printf( "TABLET BEAM DEACTIVATED DUE TO FOCUS ENT CHANGE\n" );
	}
}

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

	if ( !idStr::Icmp( statname, "health" ) ) {
		if ( health >= inventory.maxHealth ) {
			return false;
		}
		return true;
	} else if ( !idStr::Icmp( statname, "stamina" ) ) {
		if ( stamina >= 100 ) {
			return false;
		}
		return true;

	} else if ( !idStr::Icmp( statname, "heartRate" ) ) {
		return true;

	} else if ( !idStr::Icmp( statname, "air" ) ) {
		if ( airTics >= pm_airTics.GetInteger() ) {
			return false;
		}
		return true;
	} else {
		return inventory.CanGive( this, spawnArgs, statname, value, &idealWeapon );
	}

	return false;
}

/*
=================
idPlayer::StopHelltime

provides a quick non-ramping way of stopping helltime
=================
*/
void idPlayer::StopHelltime( bool quick ) {
	if ( !PowerUpActive( HELLTIME ) ) {
		return;
	}

	// take away the powerups
	if ( PowerUpActive( INVULNERABILITY ) ) {
		ClearPowerup( INVULNERABILITY );
	}

	if ( PowerUpActive( BERSERK ) ) {
		ClearPowerup( BERSERK );
	}

	if ( PowerUpActive( HELLTIME ) ) {
		ClearPowerup( HELLTIME );
	}

	// stop the looping sound
	StopSound( SND_CHANNEL_DEMONIC, false );

	// reset the game vars
	if ( quick ) {
		gameLocal.QuickSlowmoReset();
	}
}

/*
=================
idPlayer::Event_ToggleBloom
=================
*/
void idPlayer::Event_ToggleBloom( int on ) {
	if ( on ) {
		bloomEnabled = true;
	}
	else {
		bloomEnabled = false;
	}
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

/*
=================
idPlayer::PlayHelltimeStopSound
=================
*/
void idPlayer::PlayHelltimeStopSound() {
	const char* sound;

	if ( spawnArgs.GetString( "snd_helltime_stop", "", &sound ) ) {
		PostEventMS( &EV_StartSoundShader, 0, sound, SND_CHANNEL_ANY );
	}
}

/*
=================
idPlayer::RouteGuiMouse
=================
*/
void idPlayer::RouteGuiMouse( idUserInterface *gui ) {
	sysEvent_t ev;
	const char *command;

	if ( usercmd.mx != oldMouseX || usercmd.my != oldMouseY ) {
		ev = sys->GenerateMouseMoveEvent( usercmd.mx - oldMouseX, usercmd.my - oldMouseY );
		command = gui->HandleEvent( &ev, gameLocal.time );
		oldMouseX = usercmd.mx;
		oldMouseY = usercmd.my;

	}
}

// boyette begin
/*
=================
idPlayer::RouteGuiMouseWheel
=================
*/
void idPlayer::RouteGuiMouseWheel( idUserInterface *gui ) {
	sysEvent_t ev;
	const char *command;
	bool updateVisuals = false;

	if ( usercmd.impulse == IMPULSE_14 ) {
		ev = sys->GenerateMouseButtonEvent( 9, ( usercmd.impulse == IMPULSE_14 ) );
		command = gui->HandleEvent( &ev, gameLocal.time, &updateVisuals );
		if ( updateVisuals && focusGUIent && gui == focusUI ) {
			focusGUIent->UpdateVisuals();
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
		usercmd.impulse;
	}

	if ( usercmd.impulse == IMPULSE_15 ) {
		ev = sys->GenerateMouseButtonEvent( 10, ( usercmd.impulse == IMPULSE_15 ) );
		command = gui->HandleEvent( &ev, gameLocal.time, &updateVisuals );
		if ( updateVisuals && focusGUIent && gui == focusUI ) {
			focusGUIent->UpdateVisuals();
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
		usercmd.impulse;
	}

}
// boyette end

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

	// BOYETTE SPACE COMMAND BEGIN
	CloseOverlayGui();
	CloseOverlayCaptainGui();
	CloseOverlayHailGui();
	CloseOverlayAIDialogeGui();

	if ( g_inIronManMode.GetBool() ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "removeSaveGame IRONMAN\n" );
	}
	// BOYETTE SPACE COMMAND END

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

        idPlayer *victimPlayer = NULL;
        
        /* No damage feedback sound for hitting friendlies in CTF */
		if ( victim->IsType( idPlayer::Type ) ) {
            victimPlayer = static_cast<idPlayer*>(victim);
		}

        if ( gameLocal.mpGame.IsGametypeFlagBased() && victimPlayer && this->team == victimPlayer->team ) {
			/* Do nothing ... */ 
		}
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

	damageDef->GetInt( "damage", "20", damage );
	damage = GetDamageForLocation( damage, location );

	idPlayer *player = attacker->IsType( idPlayer::Type ) ? static_cast<idPlayer*>(attacker) : NULL;
	if ( !gameLocal.isMultiplayer ) {
		if ( inflictor != gameLocal.world ) {
			switch ( g_skill.GetInteger() ) {
				case 0: 
					damage *= 0.80f;
					if ( damage < 1 ) {
						damage = 1;
					}
					break;
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

	damage *= damageScale;

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
		//Invulnerability is just like god mode
		if( PowerUpActive( INVULNERABILITY ) ) {
			damage = 0;
		}
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
		} else if ( armorSave >= damage ) {
			armorSave = damage - 1;
			damage = 1;
		} else {
			damage -= armorSave;
		}
	} else {
		armorSave = 0;
	}

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
	SetTimeState ts( timeGroup );

	// damage is only processed on server
	if ( gameLocal.isClient ) {
		return;
	}
	
	if ( !fl.takedamage || noclip || spectating || gameLocal.inCinematic ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	if ( attacker->IsType( idAI::Type ) ) {
		if ( PowerUpActive( BERSERK ) ) {
			return;
		}
		// don't take damage from monsters during influences
		if ( influenceActive != 0 ) {
			return;
		}
	}

	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if ( !damageDef ) {
		gameLocal.Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}

	if ( damageDef->dict.GetBool( "ignore_player" ) ) {
		return;
	}

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
			float scale = new_g_damageScale;
			if ( g_useDynamicProtection.GetBool() && g_skill.GetInteger() < 2 ) {
				if ( gameLocal.time > lastDmgTime + 500 && scale > 0.25f ) {
					scale -= 0.05f;
					new_g_damageScale = scale;
				}
			}

			if ( scale > 0.0f ) {
				damage *= scale;
			}
		}

		if ( damage < 1 ) {
			damage = 1;
		}

		int oldHealth = health;
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

// boyette mode begin - this is to allow the player to be healed with negative damage defs.
		// boyette - heal begin
		health -= damage;
		idMath::ClampInt(-999,entity_max_health,health);
		// boyette - heal end
// boyette mod end - this is to allow the player to be healed with negative damage defs.

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

	teleportEntity = destination;

	if ( !gameLocal.isClient && !noclip ) {
		if ( gameLocal.isMultiplayer ) {
			// kill anything at the new position or mark for kill depending on immediate or delayed teleport
			gameLocal.KillBox( this, destination != NULL );
		} else {
			// kill anything at the new position
			gameLocal.KillBox( this, true );
		}
	}

	if ( PowerUpActive( HELLTIME ) ) {
		StopHelltime();
	}
}

/*
====================
idPlayer::SetPrivateCameraView
====================
*/
void idPlayer::SetPrivateCameraView( idCamera *camView ) {
	privateCameraView = camView;
	if ( camView ) {
		StopFiring();
		Hide();
	} else {
		if ( !spectating ) {
			Show();
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

	if ( zoomFov.IsDone( gameLocal.time ) ) {
		fov = ( honorZoom && usercmd.buttons & BUTTON_ZOOM ) && weapon.GetEntity() ? weapon.GetEntity()->GetZoomFov() : DefaultFov();
	} else {
		fov = zoomFov.GetCurrentValue( gameLocal.time );
	}

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
	idMat3			axis;
	idBounds		bounds;

	angles = viewAngles;
	GetViewPos( origin, axis );

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

	angles.pitch *= 0.5f;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();

	idMath::SinCos( DEG2RAD( angle ), sideScale, forwardScale );
	view -= range * forwardScale * renderView->viewaxis[ 0 ];
	view += range * sideScale * renderView->viewaxis[ 1 ];

	if ( clip ) {
		// trace a ray from the origin to the viewpoint to make sure the view isn't
		// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
		bounds = idBounds( idVec3( -4, -4, -4 ), idVec3( 4, 4, 4 ) );
		gameLocal.clip.TraceBounds( trace, origin, view, bounds, MASK_SOLID, this );
		if ( trace.fraction != 1.0f ) {
			view = trace.endpos;
			view.z += ( 1.0f - trace.fraction ) * 32.0f;

			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out
			gameLocal.clip.TraceBounds( trace, origin, view, bounds, MASK_SOLID, this );
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

	renderView->time = gameLocal.time;

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

			if ( !pm_thirdPerson.GetBool() ) {
				// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
				// allow the right player view weapons
				renderView->viewID = entityNumber + 1;
			}
		} else if ( pm_thirdPerson.GetBool() ) {
			OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), pm_thirdPersonRange.GetFloat(), pm_thirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool() );
		} else if ( pm_thirdPersonDeath.GetBool() ) {
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
void idPlayer::AddAIKill( void ) {


	int max_souls;
	int ammo_souls;

	if ( ( weapon_soulcube < 0 ) || ( inventory.weapons.test(weapon_soulcube) ) == false ) {
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
}

/*
=============
idPlayer::SetSoulCubeProjectile
=============
*/
void idPlayer::SetSoulCubeProjectile( idProjectile *projectile ) {
	soulCubeProjectile = projectile;
}

/*
=============
idPlayer::AddProjectilesFired
=============
*/
void idPlayer::AddProjectilesFired( int count ) {
	numProjectilesFired += count;
}

/*
=============
idPlayer::AddProjectileHites
=============
*/
void idPlayer::AddProjectileHits( int count ) {
	numProjectileHits += count;
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
			if ( weaponEnabled && weapon.GetEntity() ) {
				weapon.GetEntity()->EnterCinematic();
			}
			// BOYETTE BEGIN -  // BOYETTE NOTE: this was added for the outro cinematic
			if ( playerHeadLight.IsValid() ) {
				playerHeadLight.GetEntity()->PostEventMS( &EV_Remove, 0 );
				playerHeadLight = NULL;
			}
			// BOYETTE END
		} else {
			physicsObj.SetLinearVelocity( vec3_origin );
			if ( weaponEnabled && weapon.GetEntity() ) {
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
	weaponEnabled = true;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->ExitCinematic();
	}
}

/*
==================
idPlayer::Event_DisableWeapon
==================
*/
void idPlayer::Event_DisableWeapon( void ) {
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );
	weaponEnabled = false;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->EnterCinematic();
	}
}

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
	CancelEvents( &EV_Player_StopWarpEffects );
	playerView.AddWarp( idVec3( 0, 0, 0 ), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 100, 1000 );
}

/*
==================
idPlayer::Event_StopWarpEffects
==================
*/
void idPlayer::Event_StopWarpEffects() {
	playerView.FreeWarp(0);
	//gameLocal.GetLocalPlayer()->bloomEnabled = false;
}

/*
==================
idPlayer::Event_StopWarpSlowMo
==================
*/
void idPlayer::Event_StopSlowMo() {
	g_enableSlowmo.SetBool( false );
}

/*
==================
idPlayer::Event_TransitionNumBloomPassesToZero
==================
*/
void idPlayer::Event_TransitionNumBloomPassesToZero() {
	CancelEvents( &EV_Player_TransitionNumBloomPassesToZero );
	if ( g_testBloomNumPasses.GetInteger() > 0 ) {
		g_testBloomNumPasses.SetInteger( g_testBloomNumPasses.GetInteger() - 1 );
	}
	if ( g_testBloomNumPasses.GetInteger() > 0 ) {
		PostEventMS( &EV_Player_TransitionNumBloomPassesToZero, 20 );
	} else {
		bloomEnabled = false;
	}
}

/*
==================
idPlayer::Event_TransitionNumBloomPassesToThirty
==================
*/
void idPlayer::Event_TransitionNumBloomPassesToThirty() {
	CancelEvents( &EV_Player_TransitionNumBloomPassesToThirty );
	if ( g_testBloomNumPasses.GetInteger() < 30 ) {
		g_testBloomNumPasses.SetInteger( g_testBloomNumPasses.GetInteger() + 1 );
	}
	if ( g_testBloomNumPasses.GetInteger() < 30 ) {
		PostEventMS( &EV_Player_TransitionNumBloomPassesToThirty, 20 );
	}
}

/*
==================
idPlayer::Event_StopHelltime
==================
*/
void idPlayer::Event_StopHelltime( int mode ) {
	if ( mode == 1 ) {
		StopHelltime( true );
	}
	else {
		StopHelltime( false );
	}
}

/*
==================
idPlayer::Event_WeaponAvailable
==================
*/
void idPlayer::Event_WeaponAvailable( const char* name ) {

	idThread::ReturnInt( WeaponAvailable(name) ? 1 : 0 );
}

bool idPlayer::WeaponAvailable( const char* name ) {
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		if ( inventory.weapons.test(i) ) {
			const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if ( !idStr::Cmp( weap, name ) ) {
				return true;
			}
		}
	}
	return false;
}


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
		if ( inventory.weapons.test(i) ) {
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

	SetPrivateCameraView( NULL );
	// setup origin and push according to the exit target
	SetOrigin( exitEnt->GetPhysics()->GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	SetViewAngles( exitEnt->GetPhysics()->GetAxis().ToAngles() );
	physicsObj.SetLinearVelocity( exitEnt->GetPhysics()->GetAxis()[ 0 ] * pushVel );
	physicsObj.ClearPushedVelocity();
	// teleport fx
	playerView.Flash( colorWhite, 120 );

	// clear the ik heights so model doesn't appear in the wrong place
	walkIK.EnableAll();

	UpdateVisuals();

	StartSound( "snd_teleport_exit", SND_CHANNEL_ANY, 0, false, NULL );

	if ( teleportKiller != -1 ) {
		// we got killed while being teleported
		Damage( gameLocal.entities[ teleportKiller ], gameLocal.entities[ teleportKiller ], vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT );
		teleportKiller = -1;
	} else {
		// kill anything that would have waited at teleport exit
		gameLocal.KillBox( this );
	}
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

	if ( mountedObject ) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}

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

	if ( playerHeadLight.IsValid() ) {
		idAngles lightAng = firstPersonViewAxis.ToAngles();
		idVec3 lightOrg = firstPersonViewOrigin;
		const idDict *lightDef = gameLocal.FindEntityDefDict( "player_head_light", false );

		playerHeadLightOffset = lightDef->GetVector( "player_head_light_offset" );
		playerHeadLightAngleOffset = lightDef->GetVector( "player_head_light_angle_offset" );

		lightOrg += (playerHeadLightOffset.x * firstPersonViewAxis[0]);
		lightOrg += (playerHeadLightOffset.y * firstPersonViewAxis[1]);
		lightOrg += (playerHeadLightOffset.z * firstPersonViewAxis[2]);
		lightAng.pitch += playerHeadLightAngleOffset.x;
		lightAng.yaw += playerHeadLightAngleOffset.y;
		lightAng.roll += playerHeadLightAngleOffset.z;

		playerHeadLight.GetEntity()->GetPhysics()->SetOrigin( lightOrg );
		playerHeadLight.GetEntity()->GetPhysics()->SetAxis( lightAng.ToMat3() );
		playerHeadLight.GetEntity()->UpdateVisuals();
		playerHeadLight.GetEntity()->Present();
	}

	if ( gameLocal.isMultiplayer ) {
		DrawPlayerIcons();
	}

	Present();

	UpdateDamageEffects();

	LinkCombat();

	if ( gameLocal.isNewFrame && entityNumber == gameLocal.localClientNum ) {
		playerView.CalculateShake();
	}

	// determine if portal sky is in pvs
	pvsHandle_t	clientPVS = gameLocal.pvs.SetupCurrentPVS( GetPVSAreas(), GetNumPVSAreas() );
	gameLocal.portalSkyActive = gameLocal.pvs.CheckAreasForPortalSky( clientPVS, GetPhysics()->GetOrigin() );
	gameLocal.pvs.FreeCurrentPVS( clientPVS );
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
	// boyette bit begin
	bool weaponsBitArray[ MAX_WEAPONS ];
	// boyette bit end

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
	// boyette bit begin - if multiplayer is crashing - you should probably delete this entire boyette bit section - don't forget to delete the array too.
	//msg.WriteBits( inventory.weapons, MAX_WEAPONS );
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		weaponsBitArray[ i ] = inventory.weapons.test(i);
		msg.WriteBits( weaponsBitArray[i], 1 );
	}
	// boyette bit end
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
	msg.WriteBits( playerHeadLight.GetSpawnId(), 32 );
}

/*
================
idPlayer::ReadFromSnapshot
================
*/
void idPlayer::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	int		i, oldHealth, newIdealWeapon, weaponSpawnId;
	bool	newHitToggle, stateHitch;
	// boyette bit begin
	bool weaponsBitArray[ MAX_WEAPONS ];
	// boyette bit end

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
	// boyette bit begin - if multiplayer is crashing - you should probably delete this entire boyette bit section - don't forget to delete the array too.
	// boyette bit - this is the original doom version - inventory.weapons = msg.ReadBits( MAX_WEAPONS );
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		weaponsBitArray[ i ] = msg.ReadBits( 1 );
		inventory.weapons.set(i,weaponsBitArray[ i ]);
	}
	// boyette bit end
	weaponSpawnId = msg.ReadBits( 32 );
	spectator = msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) );
	newHitToggle = msg.ReadBits( 1 ) != 0;
	weaponGone = msg.ReadBits( 1 ) != 0;
	isLagged = msg.ReadBits( 1 ) != 0;
	isChatting = msg.ReadBits( 1 ) != 0;
#ifdef CTF
	carryingFlag = msg.ReadBits( 1 ) != 0;
#endif
	int enviroSpawnId;
	enviroSpawnId = msg.ReadBits( 32 );
	playerHeadLight.SetSpawnId( enviroSpawnId );

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

	// If the player is alive, restore proper physics object
	if ( health > 0 && IsActiveAF() ) {
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.EnableClip();
		SetCombatContents( true );
	}

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
	// boyette bit begin
	bool weaponsBitArray[ MAX_WEAPONS ];
	// boyette bit end

	msg.WriteByte( bobCycle );
	msg.WriteLong( stepUpTime );
	msg.WriteFloat( stepUpDelta );
	// boyette bit begin - if multiplayer is crashing - you should probably delete this entire boyette bit section - don't forget to delete the array too.
	// boyette bit - msg.WriteLong( inventory.weapons );
	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		weaponsBitArray[ i ] = inventory.weapons.test(i);
		msg.WriteBits( weaponsBitArray[i], 1 );
	}
	// boyette bit end
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
	// boyette bit begin
	bool weaponsBitArray[ MAX_WEAPONS ];
	// boyette bit end

	bobCycle = msg.ReadByte();
	stepUpTime = msg.ReadLong();
	stepUpDelta = msg.ReadFloat();
	// boyette bit begin - if multiplayer is crashing - you should probably delete this entire boyette bit section - don't forget to delete the array too.
	// boyette bit - this is the original doom version - inventory.weapons = msg.ReadLong();
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		weaponsBitArray[ i ] = msg.ReadBits( 1 );
		inventory.weapons.set(i,weaponsBitArray[ i ]);
	}
	// boyette bit end
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
		case EVENT_PICKUPNAME: {
			char buf[MAX_EVENT_PARAM_SIZE];
			msg.ReadString(buf, MAX_EVENT_PARAM_SIZE);
			inventory.AddPickupName(buf, "", this); //_D3XP
			return true;
		}
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
			return idActor::ClientReceiveEvent( event, time, msg );
		}
		default: {
			return idActor::ClientReceiveEvent( event, time, msg );
		}
	}
	return false;
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
		PostEventSec( &EV_Player_HideTip, 5.0f );
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
		inventory.Drop( spawnArgs, spawnArgs.GetString( weap ), -1 );
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

// boyette mod begin
void idPlayer::SetOverlayGui(const char *guiName) {
	CloseOverlayGui();
	CloseOverlayCaptainGui();
	CloseOverlayHailGui();
	CloseOverlayAIDialogeGui();

	if (guiName && *guiName && guiOverlay == NULL) {
		guiOverlay = uiManager->FindGui(guiName, true);
		if (guiOverlay) {
			//guiOverlay->Activate(true, gameLocal.time);
			guiOverlay->SetStateBool( "gameDraw", true );
			common->OpenFullScreenGUIOverlay();
		}
		if ( using_space_command_tablet && weapon.GetEntity() ) {
			tabletfocusEnt = NULL; // to reset events on tablet gui
			weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists_and_is_not_door", false ); // to reset events on tablet gui
			//weapon.GetEntity()->HandleNamedEventOnGuis( "StopUsingTablet" ); // to reset events on tablet gui
			UpdateSpaceCommandTabletGUIOnce();
		}
	} else {
		//guiOverlay = NULL;
		CloseOverlayGui();
	}
	if ( PlayerShip ) {
		PlayerShip->ClearModuleSelection();
	}
	if ( guiOverlay ) {
		guiOverlay->SetCursorImage(0); // get's rid of weapons/torpedos selection cursor.
	}
}
void idPlayer::CloseOverlayGui() {
	if ( guiOverlay ) {
		guiOverlay->multiselect_box_active = false;
		guiOverlay->EndMultiselectBox();
		guiOverlay->Activate(false, gameLocal.time);
		common->CloseFullScreenGUIOverlay();
		guiOverlay->SetCursorImage(0); // get's rid of weapons/torpedos selection cursor.
		guiOverlay = NULL;
		if ( CaptainGui && CaptainGui->multiselect_box_active == true ) {
			CaptainGui->multiselect_box_active = false;
			CaptainGui->EndMultiselectBox();
		}
		if ( using_space_command_tablet && weapon.GetEntity() ) {
			weapon.GetEntity()->Event_OwnerBeginsUsingSpaceCommandTablet();
		}
		PreventPlayerFiringForAWhile(400);
	}
	if ( PlayerShip ) {
		PlayerShip->ClearModuleSelection();
	}
}

void idPlayer::SetOverlayCaptainGui() {
	CloseOverlayGui();
	CloseOverlayCaptainGui();
	CloseOverlayHailGui();
	CloseOverlayAIDialogeGui();

	if (guiOverlay == NULL) {
		guiOverlay = CaptainGui;
		if ( !CaptainChairSeatedIn ) {
			//EnableBlur();
			EnableDesaturate();
		}
		if (guiOverlay) {
			//guiOverlay->Activate(true, gameLocal.time);
			guiOverlay->SetStateBool( "gameDraw", true );
			common->OpenFullScreenGUIOverlay();
		}
		if ( using_space_command_tablet && weapon.GetEntity() ) {
			tabletfocusEnt = NULL; // to reset events on tablet gui
			weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists_and_is_not_door", false ); // to reset events on tablet gui
			//weapon.GetEntity()->HandleNamedEventOnGuis( "StopUsingTablet" ); // to reset events on tablet gui
			UpdateSpaceCommandTabletGUIOnce();
		}

		if ( PlayerShip && CaptainGui ) {
			if ( PlayerShip->shields_raised ) {
				CaptainGui->HandleNamedEvent( "PlayerShipRaiseShields" );
			} else {
				CaptainGui->HandleNamedEvent( "PlayerShipLowerShields" );
			}
			if ( PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->shields_raised ) {
				CaptainGui->HandleNamedEvent( "TargetShipRaiseShields" );
			} else {
				CaptainGui->HandleNamedEvent( "TargetShipLowerShields" );
			}
			CheckForPlayerShipCrewMemberNameChanges();
			grabbed_reserve_crew_dict = NULL;
			grabbed_crew_member = NULL;
			grabbed_crew_member_id = 0;
		}
		UpdateCaptainMenu();
	} else {
		//guiOverlay = NULL;
		CloseOverlayCaptainGui();
	}
	if ( PlayerShip ) {
		PlayerShip->ClearModuleSelection();
	}
	if ( guiOverlay ) {
		guiOverlay->SetCursorImage(0); // get's rid of weapons/torpedos selection cursor.
	}
}
void idPlayer::CloseOverlayCaptainGui() {
	if ( guiOverlay && guiOverlay == CaptainGui ) {
		guiOverlay->multiselect_box_active = false;
		guiOverlay->EndMultiselectBox();
		guiOverlay->Activate(false,gameLocal.time); // THIS DEACTIVATES IT - runs any events in the onDeactivate bracket
		common->CloseFullScreenGUIOverlay();
		guiOverlay->SetCursorImage(0); // get's rid of weapons/torpedos selection cursor.
		guiOverlay = NULL;
		//DisableBlur();
		DisableDesaturate();
		g_enableCaptainGuiSlowmo.SetBool(false);
		if ( PlayerShip ) {
			PlayerShip->ClearModuleSelection();
		}
		if ( using_space_command_tablet && weapon.GetEntity() ) {
			weapon.GetEntity()->Event_OwnerBeginsUsingSpaceCommandTablet();
		}
		PreventPlayerFiringForAWhile(400);
	}
	if ( PlayerShip ) {
		PlayerShip->ClearModuleSelection();
	}
}

void idPlayer::SetOverlayHailGui() {
	CloseOverlayGui();
	CloseOverlayCaptainGui();
	CloseOverlayHailGui();
	CloseOverlayAIDialogeGui();

	if (guiOverlay == NULL || guiOverlay != HailGui) {
		if ( PlayerShip && SelectedEntityInSpace && SelectedEntityInSpace->hail_dialogue_gui_file ) {
			HailGui = uiManager->FindGui(SelectedEntityInSpace->hail_dialogue_gui_file, true, false, true );
			guiOverlay = HailGui;
			if (guiOverlay) {
				//guiOverlay->Activate(true, gameLocal.time);
				guiOverlay->SetStateBool( "gameDraw", true );
				common->OpenFullScreenGUIOverlay();
			}
			UpdateHailGui();
			if ( using_space_command_tablet && weapon.GetEntity() ) {
				tabletfocusEnt = NULL; // to reset events on tablet gui
				weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists_and_is_not_door", false ); // to reset events on tablet gui
				//weapon.GetEntity()->HandleNamedEventOnGuis( "StopUsingTablet" ); // to reset events on tablet gui
				UpdateSpaceCommandTabletGUIOnce();
			}
		} else {
			HailGui = uiManager->FindGui("guis/steve_space_command/hail_guis/communications_were_not_successful_notice.gui", true, false, true );
			guiOverlay = HailGui;
			if (guiOverlay) {
				//guiOverlay->Activate(true, gameLocal.time);
				guiOverlay->SetStateBool( "gameDraw", true );
				common->OpenFullScreenGUIOverlay();
			}
			UpdateHailGui();
			if ( using_space_command_tablet && weapon.GetEntity() ) {
				tabletfocusEnt = NULL; // to reset events on tablet gui
				weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists_and_is_not_door", false ); // to reset events on tablet gui
				//weapon.GetEntity()->HandleNamedEventOnGuis( "StopUsingTablet" ); // to reset events on tablet gui
				UpdateSpaceCommandTabletGUIOnce();
			}
		}
	} else {
		//guiOverlay = NULL;
		CloseOverlayHailGui();
	}
	if ( PlayerShip ) {
		PlayerShip->ClearModuleSelection();
	}
	if ( guiOverlay ) {
		guiOverlay->SetCursorImage(0); // get's rid of weapons/torpedos selection cursor.
	}
}
void idPlayer::CloseOverlayHailGui() {
	if ( guiOverlay && guiOverlay == HailGui ) {
		guiOverlay->multiselect_box_active = false;
		guiOverlay->EndMultiselectBox();
		guiOverlay->Activate(false, gameLocal.time);
		common->CloseFullScreenGUIOverlay();
		guiOverlay->SetCursorImage(0); // get's rid of weapons/torpedos selection cursor.
		guiOverlay = NULL;
		if ( PlayerShip ) {
			PlayerShip->ClearModuleSelection();
			PlayerShip->currently_in_hail = false;
			PlayerShip->Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();
			delay_selectedship_dialogue_branch_hostile_visible = false;
			if ( SelectedEntityInSpace ) {
				SelectedEntityInSpace->currently_in_hail = false;
				SelectedEntityInSpace->Event_ExitAllShipsAtTheSameSGPosFromNoActionHailMode();
			}
		}
		if ( using_space_command_tablet && weapon.GetEntity() ) {
			weapon.GetEntity()->Event_OwnerBeginsUsingSpaceCommandTablet();
		}
		// BOYETTE NOTE BEGIN: added 06 16 2016 to resolve various issues with it getting stuck in stop time
		gameSoundWorld->FadeSoundClasses(0,0.0f,0.1f); // BOYETTE NOTE: 0.0f is the default decibel level for all sounds. All entities have a soundclass of zero unless it is set otherwise.
		if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->music_shader_is_playing ) {
			gameSoundWorld->FadeSoundClasses(3,-17.0f,0.0f);
		} else {
			gameSoundWorld->FadeSoundClasses(3,0.0f,0.1f); // BOYETTE NOTE: ship alarms are soundclass 3. 0.0f db is the default level.
		}
		gameSoundWorld->FadeSoundClasses(1,0.0f,1.5f); // BOYETTE NOTE: 0.0f is the default decibel level for all sounds. The space ship hum is set to soundclass 1.
		g_stopTime.SetBool(false);
		g_stopTimeForceFrameNumUpdateDuring.SetBool(false);
		g_stopTimeForceRenderViewUpdateDuring.SetBool(false);
		// BOYETTE NOTE END
		PreventPlayerFiringForAWhile(400);
	}
}

void idPlayer::SetOverlayAIDialogeGui() {
	CloseOverlayGui();
	CloseOverlayCaptainGui();
	CloseOverlayHailGui();
	CloseOverlayAIDialogeGui();

	if ( focusCharacter ) {
		DialogueAI = focusCharacter;
		DialogueAI->currently_in_dialogue_with_player = true;
	}

	if (guiOverlay == NULL || guiOverlay != AIDialogueGui) {
		if ( DialogueAI && DialogueAI->ai_dialogue_gui_file ) {
			AIDialogueGui = uiManager->FindGui(DialogueAI->ai_dialogue_gui_file, true, false, true );
			guiOverlay = AIDialogueGui;
			guiOverlay->Activate(true, gameLocal.time);
			currently_in_dialogue_with_ai = false; // BOYETTE NOTE TODO
			LookAtKiller(NULL,DialogueAI);
			if (guiOverlay) {
				//guiOverlay->Activate(true, gameLocal.time);
				guiOverlay->SetStateBool( "gameDraw", true );
				common->OpenFullScreenGUIOverlay();
			}
			UpdateAIDialogueGui();
			if ( using_space_command_tablet && weapon.GetEntity() ) {
				tabletfocusEnt = NULL; // to reset events on tablet gui
				weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists_and_is_not_door", false ); // to reset events on tablet gui
				//weapon.GetEntity()->HandleNamedEventOnGuis( "StopUsingTablet" ); // to reset events on tablet gui
				UpdateSpaceCommandTabletGUIOnce();
			}
		} else {
			AIDialogueGui = uiManager->FindGui("guis/you_cannot_speak_to_this_ai.gui", true, false, true );
			guiOverlay = AIDialogueGui;
			if (guiOverlay) {
				//guiOverlay->Activate(true, gameLocal.time);
				guiOverlay->SetStateBool( "gameDraw", true );
				common->OpenFullScreenGUIOverlay();
			}
			UpdateAIDialogueGui();
			if ( using_space_command_tablet && weapon.GetEntity() ) {
				tabletfocusEnt = NULL; // to reset events on tablet gui
				weapon.GetEntity()->SetRenderEntityGuisBools( "tablet_focus_entity_exists_and_is_not_door", false ); // to reset events on tablet gui
				//weapon.GetEntity()->HandleNamedEventOnGuis( "StopUsingTablet" ); // to reset events on tablet gui
				UpdateSpaceCommandTabletGUIOnce();
			}
		}
	} else {
		//guiOverlay = NULL;
		CloseOverlayAIDialogeGui();
	}
	if ( PlayerShip ) {
		PlayerShip->ClearModuleSelection();
	}
	if ( guiOverlay ) {
		guiOverlay->SetCursorImage(0); // get's rid of weapons/torpedos selection cursor.
	}
}
void idPlayer::CloseOverlayAIDialogeGui() {
	if ( guiOverlay && guiOverlay == AIDialogueGui ) {
		guiOverlay->multiselect_box_active = false;
		guiOverlay->EndMultiselectBox();
		guiOverlay->Activate(false, gameLocal.time);
		common->CloseFullScreenGUIOverlay();
		guiOverlay->SetCursorImage(0); // get's rid of weapons/torpedos selection cursor.
		guiOverlay = NULL;
		if ( PlayerShip ) {
			PlayerShip->ClearModuleSelection();
			PlayerShip->currently_in_hail = false;
		}
		currently_in_dialogue_with_ai = false; // BOYETTE NOTE TODO
		if ( DialogueAI ) {
			DialogueAI->currently_in_dialogue_with_player = false;
		}
		if ( using_space_command_tablet && weapon.GetEntity() ) {
			weapon.GetEntity()->Event_OwnerBeginsUsingSpaceCommandTablet();
		}
		DialogueAI = NULL;
		PreventPlayerFiringForAWhile(400);
	}
}

void idPlayer::UpdateCaptainMenu() { // boyette note -  put all the materials that only need to be updated on targetship change or playership change - in here.
	if ( CaptainGui ) {
		if ( PlayerShip ) {
			idStr PlayerShipDiagramMaterialName;
			PlayerShipDiagramMaterialName = PlayerShip->spawnArgs.GetString( "ship_diagram_material", "textures/images_used_in_source/default_ship_diagram_material.tga" );
			CaptainGui->SetStateString("player_ship_diagram_material",PlayerShipDiagramMaterialName);
			declManager->FindMaterial(PlayerShipDiagramMaterialName)->SetSort(SS_GUI);
			idStr PlayerShipStargridIconMaterialName;
			PlayerShipStargridIconMaterialName = PlayerShip->spawnArgs.GetString( "ship_stargrid_icon", "textures/images_used_in_source/default_ship_stargrid_icon.tga" );
			CaptainGui->SetStateString("player_ship_stargrid_ship_icon",PlayerShipStargridIconMaterialName);
			declManager->FindMaterial(PlayerShipStargridIconMaterialName)->SetSort(SS_GUI);

			CaptainGui->SetStateInt("max_stargrid_y_positions", MAX_STARGRID_Y_POSITIONS );

			for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
				for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
					CaptainGui->SetStateFloat( "abs_stargrid_dist_to_playership_x" + idStr(x) + "y" + idStr(y), (float)idMath::Sqrt( (PlayerShip->stargridpositionx - x)*(PlayerShip->stargridpositionx - x) + (PlayerShip->stargridpositiony - y)*(PlayerShip->stargridpositiony - y) ) );
				}								// abs = absolute (that is to say: absolute stargrid distance to playership)
			}

			if (PlayerShip->TargetEntityInSpace){
				CaptainGui->SetStateString( "captain_target", PlayerShip->TargetEntityInSpace->name );
				CaptainGui->SetStateInt( "captain_number", PlayerShip->TargetEntityInSpace->captaintestnumber );

				idStr TargetShipDiagramMaterialName;
				TargetShipDiagramMaterialName = PlayerShip->TargetEntityInSpace->spawnArgs.GetString( "ship_diagram_material", "textures/images_used_in_source/default_ship_diagram_material.tga" );
				CaptainGui->SetStateString("target_ship_diagram_material",TargetShipDiagramMaterialName);
				declManager->FindMaterial(TargetShipDiagramMaterialName)->SetSort(SS_GUI);
				CaptainGui->SetStateString("target_ship_image_visual",PlayerShip->TargetEntityInSpace->ShipImageVisual);

				if ( PlayerShip->TargetEntityInSpace->team == team ) {
					CaptainGui->SetStateBool( "targetship_is_friendly", true );
					CaptainGui->SetStateBool( "targetship_is_neutral", false );
					CaptainGui->SetStateBool( "targetship_is_hostile", false );
				} else if ( PlayerShip->HasNeutralityWithShip(PlayerShip->TargetEntityInSpace) ) {
					CaptainGui->SetStateBool( "targetship_is_friendly", false );
					CaptainGui->SetStateBool( "targetship_is_neutral", true );
					CaptainGui->SetStateBool( "targetship_is_hostile", false );
				} else {
					CaptainGui->SetStateBool( "targetship_is_friendly", false );
					CaptainGui->SetStateBool( "targetship_is_neutral", false );
					CaptainGui->SetStateBool( "targetship_is_hostile", true );
				}

			} else {
				CaptainGui->SetStateString( "captain_target", "no_target" );
				CaptainGui->SetStateInt( "captain_number", 0 );

				idStr TargetShipDiagramMaterialName;
				TargetShipDiagramMaterialName = "guis/assets/steve_captain_display/NoTargetDiagramTest.tga";
				CaptainGui->SetStateString("target_ship_diagram_material",TargetShipDiagramMaterialName);
				declManager->FindMaterial(TargetShipDiagramMaterialName)->SetSort(SS_GUI);
				CaptainGui->SetStateString("target_ship_image_visual","guis/assets/steve_captain_display/NoTargetDiagramTest.tga");
			}
		} else {
			idStr PlayerShipDiagramMaterialName;
			PlayerShipDiagramMaterialName = "guis/assets/steve_captain_display/NoTargetDiagramTest.tga";
			CaptainGui->SetStateString("player_ship_diagram_material",PlayerShipDiagramMaterialName);
			declManager->FindMaterial(PlayerShipDiagramMaterialName)->SetSort(SS_GUI);
		}

		for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) {
			for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) {
				CaptainGui->SetStateBool( "stargrid_position_not_visited_x" + idStr(x) + "y" + idStr(y), true );
			}
		}
		for ( int i = 0; i < stargrid_positions_visited.size(); i++ ) {
			CaptainGui->SetStateBool( "stargrid_position_not_visited_x" + idStr(stargrid_positions_visited[i].x) + "y" + idStr(stargrid_positions_visited[i].y), false );
		}

		int weapon_index = 0;
		bool has_phaedrus_pistol = false;
		bool has_laser_shotgun = false;
		bool has_laser_rifle = false;
		for ( weapon_index = 0; weapon_index < MAX_WEAPONS; weapon_index++ ) {
			if ( inventory.weapons.test(weapon_index) ) {
				if ( !idStr::Icmp( "weapon_phaedrus_pistol", spawnArgs.GetString(va("def_weapon%d", weapon_index)) ) ) {
					has_phaedrus_pistol = true;
					continue;
				}
				if ( !idStr::Icmp( "weapon_laser_shotgun", spawnArgs.GetString(va("def_weapon%d", weapon_index)) ) ) {
					has_laser_shotgun = true;
					continue;
				}
				if ( !idStr::Icmp( "weapon_laser_rifle", spawnArgs.GetString(va("def_weapon%d", weapon_index)) ) ) {
					has_laser_rifle = true;
					continue;
				}
			}
		}
		CaptainGui->SetStateBool( "player_bought_phaedrus_pistol", has_phaedrus_pistol );
		CaptainGui->SetStateBool( "player_bought_laser_shotgun", has_laser_shotgun );
		CaptainGui->SetStateBool( "player_bought_laser_rifle", has_laser_rifle );
		//CaptainGui->SetStateBool( "stargrid_position_not_visited_x2y2", false );
	}

	UpdateDoorIconsOnShipDiagramsOnce();
	UpdateCaptainHudOnce();
}

void idPlayer::PopulateShipList() {
	if ( CaptainGui ) {
		sbShip	*ship_to_list = NULL;
		int list_counter = 0;

		// clear the list
		for ( int i = 0; i < gameLocal.num_entities; i++) { // BOYETTE NOTE TODO IMPORTANT: we could change this to the maximum number of entities that would be at a stargrid position and show up in the list - maximum possible number - currently it's probably 16 but we could make it 32 or 64 for good measure
			CaptainGui->DeleteStateVar( va( "shipList_item_%i", i) );
		}

		// populate the list
		if ( PlayerShip ) {
			for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size() ; i++ ) {
				ship_to_list = PlayerShip->ships_at_my_stargrid_position[i];
				if ( ship_to_list ) {
					if ( ship_to_list->team == team ) {
						CaptainGui->SetStateString( va("shipList_item_%i", list_counter ), "^4" + ship_to_list->name + "\t" "^4" + ship_to_list->spawnArgs.GetString("faction_name", "Unknown") );
					} else if ( dynamic_cast<sbShip*>( ship_to_list )->HasNeutralityWithTeam( team ) ) {
						CaptainGui->SetStateString( va("shipList_item_%i", list_counter ), "^8" + ship_to_list->name + "\t" "^8" + ship_to_list->spawnArgs.GetString("faction_name", "Unknown") );
					} else {
						CaptainGui->SetStateString( va("shipList_item_%i", list_counter ), "^1" + ship_to_list->name + "\t" "^1" + ship_to_list->spawnArgs.GetString("faction_name", "Unknown") );
					}
					list_counter++;
				}
			}
			// This is necessary to make sure the actual list display is updated.
			CaptainGui->StateChanged( gameLocal.time, false );
		}
		//CaptainGui->Redraw(gameLocal.realClientTime); // this seems to total screw up the captain gui on map load. I don't know why. It seems harmless. Maybe because it happens during spawn and you are already redrawing in StateChanged but it seems to work ok not during spawn(in click_engage_warp). In any case it is not necessary because we are already redrawing once in StateChanged() elsewhere - and that seems to work fine.
	}
	return;
}

void idPlayer::SetSelectedEntityFromList() {

	sbShip	*ship_in_list = NULL;
	int list_counter = 0;

//	shipList->SetSelection( 1 ); // boyette note - It seems like you can't do anything with shipList - ever. This, for example, will bomb the game - always.

	int Selected_Item_Num = CaptainGui->State().GetInt( "shipList_sel_0", "-1");

	if ( PlayerShip ) {
		for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size() ; i++ ) {
			ship_in_list = PlayerShip->ships_at_my_stargrid_position[i];
			if ( ship_in_list && ship_in_list != PlayerShip ) {
				if ( list_counter == Selected_Item_Num ) {
					SelectedEntityInSpace = ship_in_list;
					return;
				}
				list_counter++;
			}
		}
	}
	return;
}

// need to do a version of this that uses ShipOnBoard instead of PlayerShip because a ship that is not ours might warp away with us on it.
void idPlayer::ScheduleThingsToUpdateAftersbShipIsRemoved() {
	CancelEvents( &EV_Player_UpdateSomeThingsAftersbShipIsRemoved );
	PostEventMS( &EV_Player_UpdateSomeThingsAftersbShipIsRemoved, 1 );
}
/*
==================
idPlayer::Event_UpdateSomeThingsAftersbShipIsRemoved
==================
*/
void idPlayer::Event_UpdateSomeThingsAftersbShipIsRemoved() {

	gameLocal.GetLocalPlayer()->UpdateStarGridShipPositions();
	gameLocal.GetLocalPlayer()->PopulateShipList(); // needed to update the colors of the ship name text in the shiplist - and to remove ships in the list.
	gameLocal.GetLocalPlayer()->UpdateCaptainMenu();
	gameLocal.GetLocalPlayer()->UpdateSelectedEntityInSpaceOnGuis();
	gameLocal.UpdateSpaceEntityVisibilities();
	//gameLocal.UpdateSpaceCommandViewscreenCamera(); schedule for a slight delay so we get to see the ship explosion after destruction without interruption
	if ( ShipOnBoard ) {
		ShipOnBoard->ScheduleUpdateViewscreenCamera(5000);
	}
}

/*
==================
idPlayer::Event_AllowPlayerFiring
==================
*/
void idPlayer::Event_AllowPlayerFiring() {
	//StopFiring();
	StopFiring();
	prevent_weapon_firing = false;
}
/*
==================
idPlayer::PreventPlayerFiringForAWhile
==================
*/
void idPlayer::PreventPlayerFiringForAWhile( int prevent_firing_time ) {
	buttonMask |= BUTTON_ATTACK; // boyette note: masks firing button(s) for a bit
	prevent_weapon_firing = true;
	StopFiring();
	CancelEvents( &EV_Player_AllowPlayerFiring );
	PostEventMS( &EV_Player_AllowPlayerFiring, prevent_firing_time );
}

void idPlayer::UpdateViewScreenShip() {
	//gameLocal.UpdateSpaceEntityVisibilities();
}

/* // highly simplified version
void idPlayer::UpdateStarGridShipPositions() {

	idEntity	*ent;
	int i;
	int list_counter = 0;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) && !ent->IsType(sbStationarySpaceEntity::Type) && (ent != PlayerShip) ) {
			CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_x", list_counter ), ent->stargridpositionx );
			CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_y", list_counter ), ent->stargridpositiony );
			list_counter++;
		}
	}
	return;
}
*/
 // second attempt
void idPlayer::UpdateStarGridShipPositions() {

	 // BOYETTE NOTE TODO: only do this stuff if the sensors are not damaged maybe - still trying to figure out if ships should be unviewable if the sensors are disabled.
	if ( CaptainGui ) {
		idEntity	*ent;
		sbShip*		ship;
		int ships_counter = 0;
		int stations_counter = 0;

		//int ii;
		int offset_x = 0;
		int offset_y = 0; // still ned to do this.
		//std::vector<sbShip*>	previous_ships;
		//std::vector<sbShip*>	previous_stations;
		idEntity* previous_ships[MAX_SPACE_COMMAND_SHIPS]; // there will not be more than 64 ships.
		idEntity* previous_stations[MAX_SPACE_COMMAND_STATIONARY_ENTITIES]; // there will definitely not be more than 64 spacestations.

		// BOYETE NOTE TODO: need to do "stargrid_spacestation_%i_visible" windows and logic here - should be behind the windowdefs of the ships

		for ( int i = 0; i < gameLocal.num_entities ; i++ ) {
			ent = gameLocal.entities[ i ];
			if ( ent && ent->IsType(sbShip::Type) && (ent != PlayerShip) ) {
				if ( !ent->IsType(sbStationarySpaceEntity::Type) ) {
					CaptainGui->SetStateBool( va("stargrid_ship_%i_visible", ships_counter ), true );
					CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_x", ships_counter ), ent->stargridpositionx );
					CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_y", ships_counter ), ent->stargridpositiony );

					offset_x = 1; // the player ship always gets offset 0 so we can always offset any other ships at least 1.
					offset_y = 0; // the other ships are in the same row as the player ship unless there are more than 3 or 4 other ships in the same stargrid position - in which case it will offset 1.

					if ( ships_counter >= MAX_SPACE_COMMAND_SHIPS ) {
						continue; // BOYETTE NOTE: we shouldn't need to handle more than MAX_SPACE_COMMAND_SHIPS ships at the time (0 to MAX_SPACE_COMMAND_SHIPS)
					}
					for ( int x = 0; x < ships_counter ; x++ ) {
						// calculate the offset_x
						if ( ent->stargridpositionx == previous_ships[x]->stargridpositionx && ent->stargridpositiony == previous_ships[x]->stargridpositiony ) {
							offset_x++;
						}
					} // 1 2 3
					if ( offset_x > 3 && offset_x < 8 ) { // 4 5 6 7
						offset_x = offset_x - 4;
						offset_y = 1;
					} else if  ( offset_x > 7 && offset_x < 12 ) { // 8 9 10 11
						offset_x = offset_x - 8;
						offset_y = 2;
					} else if ( offset_x > 11 ) { // anything above this will just be in the same position - although there shouldn't be anything above this
						offset_x = 4;
						offset_y = 2;
					}

					CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_offset_x", ships_counter ), offset_x );
					CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_offset_y", ships_counter ), offset_y );

					previous_ships[ships_counter] = ent; // create a list of the ship grid positions to do the offsets on the stargrid so ship icons don't overlap.

					ship = dynamic_cast<sbShip*>( ent );
					if ( ship->discovered_by_player || g_showStargridAllEntityIcons.GetBool() ) {
						CaptainGui->SetStateString( va("stargrid_ship_%i_icon", ships_counter ), ship->ShipStargridIcon );
					} else if ( ship->has_artifact_aboard ) {
						CaptainGui->SetStateString( va("stargrid_ship_%i_icon", ships_counter ), ship->ShipStargridArtifactIcon );
					} else {
						if ( g_showStargridUndiscoveredEntityIcons.GetBool() ) {
							CaptainGui->SetStateString( va("stargrid_ship_%i_icon", ships_counter ), "guis/assets/steve_captain_display/StargridIcons/UndiscoveredEntityStarGridIcon.tga" ); // BOYETTE NOTE TODO: this needs to be more generic - maybe have it as a spawnarg def.
						} else {
							CaptainGui->SetStateBool( va("stargrid_ship_%i_visible", ships_counter ), false );
						}
					}
					ships_counter++;
				} else if ( ent->IsType(sbStationarySpaceEntity::Type) && static_cast<sbStationarySpaceEntity*>(ent)->track_on_stargrid ) {
					CaptainGui->SetStateBool( va("stargrid_spacestation_%i_visible", stations_counter ), true );
					CaptainGui->SetStateInt( va("stargrid_spacestation_%i_pos_x", stations_counter ), ent->stargridpositionx );
					CaptainGui->SetStateInt( va("stargrid_spacestation_%i_pos_y", stations_counter ), ent->stargridpositiony );

					offset_x = 0;
					offset_y = 0;

					if ( stations_counter >= MAX_SPACE_COMMAND_STATIONARY_ENTITIES ) {
						continue; // BOYETTE NOTE: we shouldn't need to handle more than MAX_SPACE_COMMAND_STATIONARY_ENTITIES stations at the time (0 to MAX_SPACE_COMMAND_STATIONARY_ENTITIES)
					}
					for ( int x = 0; x < stations_counter ; x++ ) {
						// calculate the offset_x
						if ( ent->stargridpositionx == previous_stations[x]->stargridpositionx && ent->stargridpositiony == previous_stations[x]->stargridpositiony ) {
							offset_x++;
						}
					}
					if ( offset_x == 0 ) {
						offset_y = 1; //
						offset_x = 0; // 1
					} else if ( offset_x == 1 ) {
						offset_y = 1; // 
						offset_x = 1; // 1 2
					} else if ( offset_x == 2 ) {
						offset_y = 0; //   3
						offset_x = 1; // 1 2
					} else if ( offset_x == 3 ) {
						offset_y = 0; // 4 3
						offset_x = 0; // 1 2
					} else if ( offset_x > 3 ) { // anything above this will just be in the same position - although there shouldn't be anything above this
						offset_y = 1;
						offset_x = 1;
					}
					CaptainGui->SetStateInt( va("stargrid_spacestation_%i_pos_offset_x", stations_counter ), offset_x );
					CaptainGui->SetStateInt( va("stargrid_spacestation_%i_pos_offset_y", stations_counter ), offset_y );

					previous_stations[stations_counter] = ent; // create a list of the ship grid positions to do the offsets on the stargrid so ship icons don't overlap.

					ship = dynamic_cast<sbShip*>( ent );
					if ( ship->discovered_by_player || g_showStargridAllEntityIcons.GetBool() ) {
						CaptainGui->SetStateString( va("stargrid_spacestation_%i_icon", stations_counter ), ship->ShipStargridIcon );
					} else if ( ship->has_artifact_aboard ) {
						CaptainGui->SetStateString( va("stargrid_spacestation_%i_icon", stations_counter ), ship->ShipStargridArtifactIcon );
					} else {
						if ( g_showStargridUndiscoveredEntityIcons.GetBool() ) {
							CaptainGui->SetStateString( va("stargrid_spacestation_%i_icon", stations_counter ), "guis/assets/steve_captain_display/StargridIcons/UndiscoveredStationaryEntityStarGridIcon.tga" ); // BOYETTE NOTE TODO: this needs to be more generic - maybe have it as a spawnarg def.
						} else {
							CaptainGui->SetStateBool( va("stargrid_spacestation_%i_visible", stations_counter ), false );
						}
					}
					stations_counter++;
				}
			}
		}
		for ( ships_counter; ships_counter < MAX_SPACE_COMMAND_SHIPS ; ships_counter++ ) { // BOYETTE NOTE TODO: we should make a constant called MAXIMUM_SBSHIPS_ON_MAP and set it to 64 - we might be able to do more than this but we'll try 64 first.
			CaptainGui->SetStateBool( va("stargrid_ship_%i_visible", ships_counter ), false );
		}
		for ( stations_counter; stations_counter < MAX_SPACE_COMMAND_STATIONARY_ENTITIES ; stations_counter++ ) { // BOYETTE NOTE TODO: we should make a constant called MAXIMUM_SBSTATIONARYSPACEENTITYS_ON_MAP and set it to 64 - we might be able to do more than this but we'll try 64 first.
			CaptainGui->SetStateBool( va("stargrid_spacestation_%i_visible", stations_counter ), false );
		}
	}

	return;
}

void idPlayer::SetPlayerShipBasedOnShipSpawnArgs() {

	idEntity	*ent;
	int i;
	sbShip*		ShipToCheck;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) /*&& !ent->IsType(sbStationarySpaceEntity::Type)*/ ) {
			ShipToCheck = dynamic_cast<sbShip*>( ent );
			if ( ShipToCheck->set_as_playership_on_player_spawn == true ) {
				PlayerShip = ShipToCheck;
				ShipOnBoard = PlayerShip;
				return;
			}
		}
	}
	PlayerShip = NULL;
	ShipOnBoard = NULL;
	return;
}

// eventually for performance reasons we might want to build an array of integers for the all the AI entities on the map - this will save on processing power.
// we could set the materials to update only when they actually need to and not every frame.(when any transport happens, when any space entity is targeted and when the game starts).
void idPlayer::UpdateCrewIconPositionsOnShipDiagrams() {
	if ( CaptainGui ) {
		idEntity	*ent;
		int i;
		int list_counter = 0;
		//idAI* CrewMemberToUpdate;

		if (PlayerShip) {
			for ( i = 0; i < PlayerShip->AIsOnBoard.size() ; i++ ) {
				if ( PlayerShip->AIsOnBoard[i] && PlayerShip->AIsOnBoard[i]->ShipOnBoard && PlayerShip->AIsOnBoard[i]->ShipOnBoard == PlayerShip && !PlayerShip->AIsOnBoard[i]->was_killed && (PlayerShip->AIsOnBoard[i] != PlayerShip->crew[MEDICALCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[ENGINESCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[WEAPONSCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[TORPEDOSCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[SHIELDSCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[SENSORSCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[ENVIRONMENTCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[COMPUTERCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[SECURITYCREWID] && PlayerShip->AIsOnBoard[i] != PlayerShip->crew[CAPTAINCREWID]) ) {
					CaptainGui->SetStateInt( va("playership_misc_crew_%i_pos_x", list_counter ), PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->AIsOnBoard[i]) );
					CaptainGui->SetStateInt( va("playership_misc_crew_%i_pos_y", list_counter ), -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->AIsOnBoard[i]) );
					CaptainGui->SetStateFloat( va("playership_misc_crew_%i_angle", list_counter ), PlayerShip->AIsOnBoard[i]->GetCurrentYaw() - 90 );
					CaptainGui->SetStateFloat( va("playership_misc_crew_%i_health_percentage", list_counter ), (float)PlayerShip->AIsOnBoard[i]->health / (float)PlayerShip->AIsOnBoard[i]->entity_max_health );
					CaptainGui->SetStateString( va("playership_misc_crew_%i_diagram_icon", list_counter ), PlayerShip->AIsOnBoard[i]->ship_diagram_icon );
					if ( PlayerShip->AIsOnBoard[i]->team != team && PlayerShip->HasNeutralityWithAI(PlayerShip->AIsOnBoard[i]) ) {
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_red", list_counter ), 0.4f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_green", list_counter ), 0.4f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_blue", list_counter ), 0.4f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
					} else if ( PlayerShip->AIsOnBoard[i]->team != team ) {
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_red", list_counter ), 0.75f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_green", list_counter ), 0.09f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_blue", list_counter ), 0.09f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
					} else if ( PlayerShip->AIsOnBoard[i]->team == team ) {
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_red", list_counter ), 0.2f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_green", list_counter ), 0.7f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_blue", list_counter ), 1.0f );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
					}
					CaptainGui->SetStateBool( va("playership_misc_crew_%i_visible", list_counter ), true );
					list_counter++;
				}
			}
			/*
			for ( i = 0; i < gameLocal.num_entities ; i++ ) {
				ent = gameLocal.entities[ i ];
				if ( ent && ent->IsType( idAI::Type ) && !ent->IsType( idPlayer::Type ) ) {
					CrewMemberToUpdate = dynamic_cast<idAI*>(ent);
					if (CrewMemberToUpdate->ShipOnBoard == PlayerShip && !CrewMemberToUpdate->was_killed && (CrewMemberToUpdate != PlayerShip->crew[MEDICALCREWID] && CrewMemberToUpdate != PlayerShip->crew[ENGINESCREWID] && CrewMemberToUpdate != PlayerShip->crew[WEAPONSCREWID] && CrewMemberToUpdate != PlayerShip->crew[TORPEDOSCREWID] && CrewMemberToUpdate != PlayerShip->crew[SHIELDSCREWID] && CrewMemberToUpdate != PlayerShip->crew[SENSORSCREWID] && CrewMemberToUpdate != PlayerShip->crew[ENVIRONMENTCREWID] && CrewMemberToUpdate != PlayerShip->crew[COMPUTERCREWID] && CrewMemberToUpdate != PlayerShip->crew[SECURITYCREWID] && CrewMemberToUpdate != PlayerShip->crew[CAPTAINCREWID]) ) {
						CaptainGui->SetStateInt( va("playership_misc_crew_%i_pos_x", list_counter ), PlayerShip->ReturnOnBoardEntityDiagramPositionX(CrewMemberToUpdate) );
						CaptainGui->SetStateInt( va("playership_misc_crew_%i_pos_y", list_counter ), -PlayerShip->ReturnOnBoardEntityDiagramPositionY(CrewMemberToUpdate) );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_angle", list_counter ), CrewMemberToUpdate->GetCurrentYaw() - 90 );
						CaptainGui->SetStateFloat( va("playership_misc_crew_%i_health_percentage", list_counter ), (float)CrewMemberToUpdate->health / (float)CrewMemberToUpdate->entity_max_health );
						CaptainGui->SetStateString( va("playership_misc_crew_%i_diagram_icon", list_counter ), CrewMemberToUpdate->ship_diagram_icon );
						CaptainGui->SetStateBool( va("playership_misc_crew_%i_visible", list_counter ), true );
						list_counter++;
					}
				}
			}
			*/
			// clear out the rest.
			for ( i = list_counter; i < 21 ; i++ ) { // There will not be more than 20 misc crew on the playership.
				CaptainGui->SetStateBool( va("playership_misc_crew_%i_visible", i ), false );
			}
			list_counter = 0;
			if ( PlayerShip->TargetEntityInSpace ) {
				for ( i = 0; i < PlayerShip->TargetEntityInSpace->AIsOnBoard.size() ; i++ ) {
					if ( PlayerShip->TargetEntityInSpace->AIsOnBoard[i] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->ShipOnBoard && PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->ShipOnBoard == PlayerShip->TargetEntityInSpace && !PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->was_killed && (PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[MEDICALCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[ENGINESCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[WEAPONSCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[TORPEDOSCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[SHIELDSCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[SENSORSCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[ENVIRONMENTCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[COMPUTERCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[SECURITYCREWID] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i] != PlayerShip->crew[CAPTAINCREWID]) ) {
						CaptainGui->SetStateInt( va("targetship_misc_crew_%i_pos_x", list_counter ), PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->AIsOnBoard[i]) );
						CaptainGui->SetStateInt( va("targetship_misc_crew_%i_pos_y", list_counter ), -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->AIsOnBoard[i]) );
						CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_angle", list_counter ), PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->GetCurrentYaw() - 90 );
						CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_health_percentage", list_counter ), (float)PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->health / (float)PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->entity_max_health );
						CaptainGui->SetStateString( va("targetship_misc_crew_%i_diagram_icon", list_counter ), PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->ship_diagram_icon );
						CaptainGui->SetStateBool( va("targetship_misc_crew_%i_visible", list_counter ), true );
						if ( PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->team != team && PlayerShip->HasNeutralityWithAI(PlayerShip->TargetEntityInSpace->AIsOnBoard[i]) ) {
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_red", list_counter ), 0.4f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_green", list_counter ), 0.4f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_blue", list_counter ), 0.4f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
						} else if ( PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->team != team ) {
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_red", list_counter ), 0.75f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_green", list_counter ), 0.09f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_blue", list_counter ), 0.09f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
						} else if ( PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->team == team ) {
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_red", list_counter ), 0.2f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_green", list_counter ), 0.7f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_blue", list_counter ), 1.0f );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
						}
						list_counter++;
					}
				}
				/*
				for ( i = 0; i < gameLocal.num_entities ; i++ ) {
					ent = gameLocal.entities[ i ];
					if ( ent && ent->IsType( idAI::Type ) && !ent->IsType( idPlayer::Type ) ) {
						CrewMemberToUpdate = dynamic_cast<idAI*>(ent);
						if (CrewMemberToUpdate->ShipOnBoard == PlayerShip->TargetEntityInSpace && !CrewMemberToUpdate->was_killed && (CrewMemberToUpdate != PlayerShip->crew[MEDICALCREWID] && CrewMemberToUpdate != PlayerShip->crew[ENGINESCREWID] && CrewMemberToUpdate != PlayerShip->crew[WEAPONSCREWID] && CrewMemberToUpdate != PlayerShip->crew[TORPEDOSCREWID] && CrewMemberToUpdate != PlayerShip->crew[SHIELDSCREWID] && CrewMemberToUpdate != PlayerShip->crew[SENSORSCREWID] && CrewMemberToUpdate != PlayerShip->crew[ENVIRONMENTCREWID] && CrewMemberToUpdate != PlayerShip->crew[COMPUTERCREWID] && CrewMemberToUpdate != PlayerShip->crew[SECURITYCREWID] && CrewMemberToUpdate != PlayerShip->crew[CAPTAINCREWID]) ) {
							CaptainGui->SetStateInt( va("targetship_misc_crew_%i_pos_x", list_counter ), PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(CrewMemberToUpdate) );
							CaptainGui->SetStateInt( va("targetship_misc_crew_%i_pos_y", list_counter ), -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(CrewMemberToUpdate) );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_angle", list_counter ), CrewMemberToUpdate->GetCurrentYaw() - 90 );
							CaptainGui->SetStateFloat( va("targetship_misc_crew_%i_health_percentage", list_counter ), (float)CrewMemberToUpdate->health / (float)CrewMemberToUpdate->entity_max_health );
							CaptainGui->SetStateString( va("targetship_misc_crew_%i_diagram_icon", list_counter ), CrewMemberToUpdate->ship_diagram_icon );
							CaptainGui->SetStateBool( va("targetship_misc_crew_%i_visible", list_counter ), true );
							list_counter++;
						}
					}
				}
				*/
				// clear out the rest.
				for ( i = list_counter; i < 21 ; i++ ) { // There will not be more than 20 misc crew on the targetship.
					CaptainGui->SetStateBool( va("targetship_misc_crew_%i_visible", i ), false );
				}
			} else {
				for ( i = 0; i < 21 ; i++ ) { // There will not be more than 20 misc crew on the targetship.
					CaptainGui->SetStateBool( va("targetship_misc_crew_%i_visible", i ), false );
				}
			}
		}
	}
	return;
}

// eventually for performance reasons we might want to build an array of integers for the all the AI entities on the map - this will save on processing power - but will cost more memory.
// we could set the materials to update only when they actually need to and not every frame.(when any transport happens, when any space entity is targeted and when the game starts).
void idPlayer::UpdateDoorIconsOnShipDiagramsEveryFrame() {
	if ( CaptainGui ) {
		int i;
		int ix;

		if (PlayerShip) {
			for( i = 0; i < PlayerShip->shipdoors.Num(); i++ ) {
				if ( PlayerShip->shipdoors[ i ].GetEntity() ) {
					CaptainGui->SetStateFloat( va("playership_door_%i_health_percentage", i ), (float)PlayerShip->shipdoors[ i ].GetEntity()->health / (float)PlayerShip->shipdoors[ i ].GetEntity()->entity_max_health );
					CaptainGui->SetStateBool( va("playership_door_%i_is_secure", i ), PlayerShip->shipdoors[ i ].GetEntity()->health > 0 );
					if ( PlayerShip->shipdoors[ i ].GetEntity()->was_just_damaged ) {
						CaptainGui->HandleNamedEvent(va("PlayerShipDoor_%i_Damaged", i ));
						PlayerShip->shipdoors[ i ].GetEntity()->was_just_damaged = false;
					}
					if ( PlayerShip->shipdoors[ i ].GetEntity()->IsType(idDoor::Type) ) {
						if ( dynamic_cast<idDoor*>(PlayerShip->shipdoors[ i ].GetEntity())->IsOpen() ) {
							CaptainGui->SetStateBool( va("playership_door_%i_is_open", i ), true );
						} else {
							CaptainGui->SetStateBool( va("playership_door_%i_is_open", i ), false );
						}	
					}
				}
			}
			if (PlayerShip->TargetEntityInSpace) {
				for( ix = 0; ix < PlayerShip->TargetEntityInSpace->shipdoors.Num(); ix++ ) {
					if ( PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity() ) {
						CaptainGui->SetStateFloat( va("targetship_door_%i_health_percentage", ix ), (float)PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()->health / (float)PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()->entity_max_health );
						CaptainGui->SetStateBool( va("targetship_door_%i_is_secure", ix ), PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()->health > 0 );
						if ( PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()->was_just_damaged ) {
							CaptainGui->HandleNamedEvent(va("TargetShipDoor_%i_Damaged", ix ));
							PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()->was_just_damaged = false;
						}
						if ( PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()->IsType(idDoor::Type) ) {
							if ( dynamic_cast<idDoor*>(PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity())->IsOpen() ) {
								CaptainGui->SetStateBool( va("targetship_door_%i_is_open", ix ), true );
							} else {
								CaptainGui->SetStateBool( va("targetship_door_%i_is_open", ix ), false );
							}	
						}
					}
				}
			}
		}
		// BOYETTE NOTE TODO - THIS MIGHT BE NECESSARY -  WE WILL HAVE TO PUT A FUNCTION HERE TO CLEAR OUT ANY OTHER EXTRA SHIP DOORS THAT ARE STILL VISIBLE AND SET THEM TO VISIBLE FALSE.
	}
	return;
}

void idPlayer::UpdateProjectileIconsOnShipDiagramsEveryFrame() {
	if ( CaptainGui ) {
		if(PlayerShip && ShipOnBoard){
			int projectile_counter = 0;
			// first do any AIs on board the player ship:
			for ( int i = 0; i < PlayerShip->AIsOnBoard.size() ; i++ ) {
				if ( PlayerShip->AIsOnBoard[i] && PlayerShip->AIsOnBoard[i]->ShipOnBoard && PlayerShip->AIsOnBoard[i]->ShipOnBoard == PlayerShip && !PlayerShip->AIsOnBoard[i]->was_killed ) {
					for ( int x = 0; x < PlayerShip->AIsOnBoard[i]->projectiles_to_track.size() ; x++ ) {
						if ( PlayerShip->AIsOnBoard[i]->projectiles_to_track[x] ) {
							CaptainGui->SetStateInt( va("crew_projectile_on_playership_%i_pos_x", projectile_counter ), PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->AIsOnBoard[i]->projectiles_to_track[x]) );
							CaptainGui->SetStateInt( va("crew_projectile_on_playership_%i_pos_y", projectile_counter ), -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->AIsOnBoard[i]->projectiles_to_track[x]) );
							CaptainGui->SetStateBool( va("crew_projectile_on_playership_%i_visible", projectile_counter ), true );
							projectile_counter++;
							if ( projectile_counter > 50 ) break; // don't try to track more than 50 projectiles.
						}
					}
				}
				if ( projectile_counter > 50 ) break; // don't try to track more than 51 projectiles.
			}
			// then do the player:
			if ( PlayerShip == ShipOnBoard ) {
				for ( int x = 0; x < projectiles_to_track.size() ; x++ ) {
					if ( projectiles_to_track[x] ) {
						CaptainGui->SetStateInt( va("crew_projectile_on_playership_%i_pos_x", projectile_counter ), PlayerShip->ReturnOnBoardEntityDiagramPositionX(projectiles_to_track[x]) );
						CaptainGui->SetStateInt( va("crew_projectile_on_playership_%i_pos_y", projectile_counter ), -PlayerShip->ReturnOnBoardEntityDiagramPositionY(projectiles_to_track[x]) );
						CaptainGui->SetStateBool( va("crew_projectile_on_playership_%i_visible", projectile_counter ), true );
						projectile_counter++;
						if ( projectile_counter > 50 ) break; // don't try to track more than 50 projectiles.
					}
				}
			}
			// clear out the rest.
			while ( projectile_counter <= 50 ) { // There will not be more than 20 misc crew on the playership.
				CaptainGui->SetStateBool( va("crew_projectile_on_playership_%i_visible", projectile_counter ), false );
				projectile_counter++;
			}
		}
		if ( PlayerShip && PlayerShip->TargetEntityInSpace ) {
			int projectile_counter = 0;
			// next do any AIs on board the target ship:
			for ( int i = 0; i < PlayerShip->TargetEntityInSpace->AIsOnBoard.size() ; i++ ) {
				if ( PlayerShip->TargetEntityInSpace->AIsOnBoard[i] && PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->ShipOnBoard && PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->ShipOnBoard == PlayerShip->TargetEntityInSpace && !PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->was_killed ) {
					for ( int x = 0; x < PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->projectiles_to_track.size() ; x++ ) {
						if ( PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->projectiles_to_track[x] ) {
							if ( PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->projectiles_to_track[x] ) {
								CaptainGui->SetStateInt( va("crew_projectile_on_targetship_%i_pos_x", projectile_counter ), PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->projectiles_to_track[x]) );
								CaptainGui->SetStateInt( va("crew_projectile_on_targetship_%i_pos_y", projectile_counter ), -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->AIsOnBoard[i]->projectiles_to_track[x]) );
								CaptainGui->SetStateBool( va("crew_projectile_on_targetship_%i_visible", projectile_counter ), true );
								projectile_counter++;
								if ( projectile_counter > 50 ) break; // don't try to track more than 51 projectiles.
							}
						}
					}
				}
				if ( projectile_counter > 50 ) break; // don't try to track more than 51 projectiles.
			}
			// then do the player:
			if ( ShipOnBoard && ShipOnBoard == PlayerShip->TargetEntityInSpace ) {
				for ( int x = 0; x < projectiles_to_track.size() ; x++ ) {
					if ( projectiles_to_track[x] ) {
						if ( projectiles_to_track[x] ) {
							CaptainGui->SetStateInt( va("crew_projectile_on_targetship_%i_pos_x", projectile_counter ), PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(projectiles_to_track[x]) );
							CaptainGui->SetStateInt( va("crew_projectile_on_targetship_%i_pos_y", projectile_counter ), -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(projectiles_to_track[x]) );
							CaptainGui->SetStateBool( va("crew_projectile_on_targetship_%i_visible", projectile_counter ), true );
							projectile_counter++;
							if ( projectile_counter > 50 ) break; // don't try to track more than 51 projectiles.
						}
					}
				}
			}
			// clear out the rest.
			while ( projectile_counter <= 50 ) { // don't try to track more than 51 projectiles.
				CaptainGui->SetStateBool( va("crew_projectile_on_targetship_%i_visible", projectile_counter ), false );
				projectile_counter++;
			}
		}
	}
}

void idPlayer::UpdateProjectileIconsOnHudEveryFrame() {
	if ( hud ) {
		if(ShipOnBoard){
			int projectile_counter = 0;
			// first do any AIs on board the ship on board:
			for ( int i = 0; i < ShipOnBoard->AIsOnBoard.size() ; i++ ) {
				if ( ShipOnBoard->AIsOnBoard[i] && ShipOnBoard->AIsOnBoard[i]->ShipOnBoard && ShipOnBoard->AIsOnBoard[i]->ShipOnBoard == ShipOnBoard && !ShipOnBoard->AIsOnBoard[i]->was_killed ) {
					for ( int x = 0; x < ShipOnBoard->AIsOnBoard[i]->projectiles_to_track.size() ; x++ ) {
						if ( ShipOnBoard->AIsOnBoard[i]->projectiles_to_track[x] ) {
							hud->SetStateInt( va("crew_projectile_on_shiponboard_%i_pos_x", projectile_counter ), ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(ShipOnBoard->AIsOnBoard[i]->projectiles_to_track[x]) );
							hud->SetStateInt( va("crew_projectile_on_shiponboard_%i_pos_y", projectile_counter ), -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(ShipOnBoard->AIsOnBoard[i]->projectiles_to_track[x]) );
							hud->SetStateBool( va("crew_projectile_on_shiponboard_%i_visible", projectile_counter ), true );
							projectile_counter++;
							if ( projectile_counter > 50 ) break; // don't try to track more than 50 projectiles.
						}
					}
				}
				if ( projectile_counter > 50 ) break; // don't try to track more than 51 projectiles.
			}
			// then do the player:
			for ( int x = 0; x < projectiles_to_track.size() ; x++ ) {
				if ( projectiles_to_track[x] ) {
					hud->SetStateInt( va("crew_projectile_on_shiponboard_%i_pos_x", projectile_counter ), ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(projectiles_to_track[x]) );
					hud->SetStateInt( va("crew_projectile_on_shiponboard_%i_pos_y", projectile_counter ), -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(projectiles_to_track[x]) );
					hud->SetStateBool( va("crew_projectile_on_shiponboard_%i_visible", projectile_counter ), true );
					projectile_counter++;
					if ( projectile_counter > 50 ) break; // don't try to track more than 50 projectiles.
				}
			}
			// clear out the rest.
			while ( projectile_counter <= 50 ) { // There will not be more than 20 misc crew on the shiponboard.
				hud->SetStateBool( va("crew_projectile_on_shiponboard_%i_visible", projectile_counter ), false );
				projectile_counter++;
			}
		}
	}
}

// we might need to make sure that it takes the spawnarg position instead of the current position - otherwise the doors could appeared offset if this function is run when they are opened or opening.
void idPlayer::UpdateDoorIconsOnShipDiagramsOnce() {
	int i;
	int ix;

	if ( PlayerShip && CaptainGui ) {
		for( i = 0; i < PlayerShip->shipdoors.Num(); i++ ) {
			CaptainGui->SetStateInt( va("playership_door_%i_pos_x", i ), PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->shipdoors[ i ].GetEntity()) );
			CaptainGui->SetStateInt( va("playership_door_%i_pos_y", i ), -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->shipdoors[ i ].GetEntity()) );
			//CaptainGui->SetStateFloat( va("playership_door_%i_angle", i ), PlayerShip->shipdoors[ i ].GetEntity()->GetPhysics()->GetAxis().ToAngles().Normalize180().yaw - 90.0f );
			CaptainGui->SetStateBool( va("playership_door_%i_visible", i ), true );
		}
		for ( i; i < MAX_SHIP_DOOR_ENTITIES; i++ ) {
				CaptainGui->SetStateBool( va("playership_door_%i_visible", i ), false );
		}
		if ( PlayerShip->TargetEntityInSpace ) {
			for( ix = 0; ix < PlayerShip->TargetEntityInSpace->shipdoors.Num(); ix++ ) {
				CaptainGui->SetStateInt( va("targetship_door_%i_pos_x", ix ), PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()) );
				CaptainGui->SetStateInt( va("targetship_door_%i_pos_y", ix ), -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()) );
				//CaptainGui->SetStateFloat( va("targetship_door_%i_angle", ix ), PlayerShip->TargetEntityInSpace->shipdoors[ ix ].GetEntity()->GetPhysics()->GetAxis().ToAngles().Normalize180().yaw - 90.0f );
				CaptainGui->SetStateBool( va("targetship_door_%i_visible", ix ), true );
			}
			for ( ix; ix < MAX_SHIP_DOOR_ENTITIES; ix++ ) {
				CaptainGui->SetStateBool( va("targetship_door_%i_visible", ix ), false );
			}
		} else {
			for ( ix = 0; ix < MAX_SHIP_DOOR_ENTITIES; ix++ ) {
				CaptainGui->SetStateBool( va("targetship_door_%i_visible", ix ), false );
			}
		}
	} else if ( CaptainGui ) {
		for ( i = 0; i < MAX_SHIP_DOOR_ENTITIES; i++ ) {
				CaptainGui->SetStateBool( va("playership_door_%i_visible", i ), false );
		}
	}
	// DONE: BOYETTE NOTE TODO - THIS MIGHT BE NECESSARY -  WE WILL HAVE TO PUT A FUNCTION HERE TO CLEAR OUT ANY OTHER EXTRA SHIP DOORS THAT ARE STILL VISIBLE AND SET THEM TO VISIBLE FALSE.
	return;
}

/*
void idPlayer::UpdateStarGridShipPositions() {

	idEntity	*ent;
	int i;
	int list_counter = 0;
	int ship_counter = 0;
	int offset = 0;
	idVec2 gridpositions[64]; // there will not be more than 64 ships.
	int shipIDlist[64]; // there will not be more than 64 ships.

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) && !ent->IsType(sbStationarySpaceEntity::Type) && (ent != PlayerShip) ) {
			CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_x", list_counter ), ent->stargridpositionx );
			CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_y", list_counter ), ent->stargridpositiony );
			shipIDlist[list_counter] = ent->entityNumber; // create a list of the ship entity id numbers to do the offsets on the stargrid so ship icons don't overlap.
			gridpositions[list_counter].Set(ent->stargridpositionx,ent->stargridpositiony); // create a list of the ship grid positions to do the offsets on the stargrid so ship icons don't overlap.
			list_counter++;
		}
	}

	ship_counter = list_counter;
	list_counter = 0;
	// do the offsets on the stargrid so ship icons don't overlap.
	for ( i = 0; i < ship_counter ; i++ ) {
		int offset = 0;
		ent = gameLocal.entities[ shipIDlist[i] ];
		for ( i = 0; i < ship_counter ; i++ ) {
			if ( idVec2(ent->stargridpositionx,ent->stargridpositiony) == gridpositions[i] ) {
				CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_offset_x", list_counter ), ent->stargridpositionx );
				CaptainGui->SetStateInt( va("stargrid_ship_%i_pos_offset_y", list_counter ), ent->stargridpositiony );
			}
		}
		list_counter++;
	}
	return;
}
*/

// we could set the materials to update only when they actually need to and not every frame.(when any transport happens, when any space entity is targeted and when the game starts).
void idPlayer::UpdateCrewMemberPortraits() {
	if ( CaptainGui ) {
		if ( PlayerShip ) {
			if ( grabbed_crew_member && !grabbed_crew_member->was_killed ) {
				CaptainGui->SetStateFloat( "playership_grabbed_officer_portrait_rect_pos_x", CaptainGui->CursorX() );
				CaptainGui->SetStateFloat( "playership_grabbed_officer_portrait_rect_pos_y", CaptainGui->CursorY() );

				CaptainGui->SetStateBool( "playership_grabbed_officer_portrait_visible", true );
				CaptainGui->SetStateString( "playership_grabbed_officer_portrait", grabbed_crew_member->ship_diagram_portrait );
				CaptainGui->SetStateBool( "playership_grabbed_officer_auto_mode_activated", grabbed_crew_member->crew_auto_mode_activated );
				CaptainGui->SetStateBool( "playership_grabbed_officer_player_follow_mode_activated", grabbed_crew_member->player_follow_mode_activated );
				CaptainGui->SetStateString( "playership_grabbed_officer_npc_name", grabbed_crew_member->spawnArgs.GetString( "npc_name", "Joe" ) );
				CaptainGui->SetStateString( "playership_grabbed_officer_insignia", grabbed_crew_member->ship_diagram_icon );
				CaptainGui->SetStateFloat( "playership_grabbed_officer_health_percentage", (float)grabbed_crew_member->health / (float)grabbed_crew_member->entity_max_health );

				CaptainGui->SetStateInt( "playership_grabbed_officer_space_command_level", grabbed_crew_member->space_command_level );

				if( grabbed_crew_member->ShipOnBoard == PlayerShip ) {
					CaptainGui->SetStateBool( "playership_grabbed_officer_off_ship_overlay_visible", false );
				} else {
					CaptainGui->SetStateBool( "playership_grabbed_officer_off_ship_overlay_visible", true );
				}
			} else if ( grabbed_reserve_crew_dict ) {
				CaptainGui->SetStateFloat( "playership_grabbed_officer_portrait_rect_pos_x", CaptainGui->CursorX() );
				CaptainGui->SetStateFloat( "playership_grabbed_officer_portrait_rect_pos_y", CaptainGui->CursorY() );

				CaptainGui->SetStateBool( "playership_grabbed_officer_portrait_visible", true );
				CaptainGui->SetStateString( "playership_grabbed_officer_portrait", grabbed_reserve_crew_dict->GetString("ship_diagram_portrait", "guis/assets/steve_captain_display/CrewPortraits/CrewPortraitDefault.tga") );
				CaptainGui->SetStateBool( "playership_grabbed_officer_auto_mode_activated", false );
				CaptainGui->SetStateBool( "playership_grabbed_officer_player_follow_mode_activated", false );
				CaptainGui->SetStateString( "playership_grabbed_officer_npc_name", grabbed_reserve_crew_dict->GetString( "npc_name", "Joe" ) );
				CaptainGui->SetStateString( "playership_grabbed_officer_insignia", grabbed_reserve_crew_dict->GetString(va("ship_diagram_icon%i",grabbed_reserve_crew_dict->GetInt( "space_command_level", "1" )), "guis/assets/steve_captain_display/CrewDiagramIcons/CrewDiagramIconDefault.tga") );
				CaptainGui->SetStateFloat( "playership_grabbed_officer_health_percentage", 1.0f );

				CaptainGui->SetStateInt( "playership_grabbed_officer_space_command_level", grabbed_reserve_crew_dict->GetInt( "space_command_level", "1" ) );

				CaptainGui->SetStateBool( "playership_grabbed_officer_off_ship_overlay_visible", false );
			} else {
				CaptainGui->SetStateBool( "playership_grabbed_officer_portrait_visible", false );
			}

			if ( PlayerShip->SelectedCrewMember && !PlayerShip->SelectedCrewMember->was_killed ) {
				CaptainGui->SetStateFloat( "playership_selected_officer_portrait_rect_pos_x", CaptainGui->CursorX() );
				CaptainGui->SetStateFloat( "playership_selected_officer_portrait_rect_pos_y", CaptainGui->CursorY() );

				CaptainGui->SetStateBool( "playership_selected_officer_portrait_visible", true );
				CaptainGui->SetStateString( "playership_selected_officer_portrait", PlayerShip->SelectedCrewMember->ship_diagram_portrait );
				CaptainGui->SetStateBool( "playership_selected_officer_auto_mode_activated", PlayerShip->SelectedCrewMember->crew_auto_mode_activated );
				CaptainGui->SetStateBool( "playership_selected_officer_player_follow_mode_activated", PlayerShip->SelectedCrewMember->player_follow_mode_activated );

				CaptainGui->SetStateString( "playership_selected_officer_npc_name", PlayerShip->SelectedCrewMember->spawnArgs.GetString( "npc_name", "Joe" ) );
				if ( PlayerShip->SelectedCrewMember->ParentShip ) {
					CaptainGui->SetStateString( "playership_selected_officer_parentship", PlayerShip->SelectedCrewMember->ParentShip->original_name );
				} else {
					CaptainGui->SetStateString( "playership_selected_officer_parentship", "Unknown" );
				}
				CaptainGui->SetStateString( "playership_selected_officer_species", PlayerShip->SelectedCrewMember->spawnArgs.GetString( "space_command_species", "Homo Sapiens" ) );
				CaptainGui->SetStateString( "playership_selected_officer_mass", PlayerShip->SelectedCrewMember->spawnArgs.GetString( "mass", "150" ) + idStr(" Kilograms") );
				CaptainGui->SetStateString( "playership_selected_officer_vocation", PlayerShip->SelectedCrewMember->spawnArgs.GetString( "space_command_vocation", "Explorer/Scientist" ) );
				CaptainGui->SetStateInt( "playership_selected_officer_tech_skill_rating", PlayerShip->SelectedCrewMember->module_buff_amount );
				CaptainGui->SetStateString( "playership_selected_officer_armament", PlayerShip->SelectedCrewMember->spawnArgs.GetString( "space_command_armament", "Basic Plasma Weapon" ) + idStr(" / Rank: ") + idStr(PlayerShip->SelectedCrewMember->spawnArgs.GetInt("attack_accuracy", "7")) );
				CaptainGui->SetStateString( "playership_selected_officer_space_command_description", PlayerShip->SelectedCrewMember->spawnArgs.GetString( "space_command_tablet_description", "NO INFO AVAILABLE" ) );

				if ( PlayerShip->SelectedCrewMember->module_console_damage_def_name ) {
					const idDict *damageDef = gameLocal.FindEntityDefDict( PlayerShip->SelectedCrewMember->module_console_damage_def_name );
					if ( !damageDef ) {
						gameLocal.Error( "Unknown damageDef '%s'\n", PlayerShip->SelectedCrewMember->module_console_damage_def_name.c_str() );
					}
					float engineer_skill_rating_damageScale = 1.0f;
					engineer_skill_rating_damageScale = idMath::Pow(SPACE_COMMAND_LEVEL_STAT_MODIFIER, PlayerShip->SelectedCrewMember->space_command_level - 1.0f );
					int	engineer_skill_rating = damageDef->GetInt( "damage" ) * engineer_skill_rating_damageScale;
					CaptainGui->SetStateInt( "playership_selected_officer_engineer_skill_rating", engineer_skill_rating );
				}
				if ( PlayerShip->SelectedCrewMember->door_damage_def_name ) {
					const idDict *damageDef = gameLocal.FindEntityDefDict( PlayerShip->SelectedCrewMember->door_damage_def_name );
					if ( !damageDef ) {
						gameLocal.Error( "Unknown damageDef '%s'\n", PlayerShip->SelectedCrewMember->door_damage_def_name.c_str() );
					}
					float door_jacking_rating_damageScale = 1.0f;
					door_jacking_rating_damageScale = idMath::Pow(SPACE_COMMAND_LEVEL_STAT_MODIFIER, PlayerShip->SelectedCrewMember->space_command_level - 1.0f );
					int	door_jacking_rating = damageDef->GetInt( "damage" ) * door_jacking_rating_damageScale;
					CaptainGui->SetStateInt( "playership_selected_officer_door_jacking_skill_rating", door_jacking_rating );
				}

				CaptainGui->SetStateInt( "playership_selected_officer_space_command_level", PlayerShip->SelectedCrewMember->space_command_level );

				CaptainGui->SetStateFloat( "playership_selected_officer_health_percentage", (float)PlayerShip->SelectedCrewMember->health / (float)PlayerShip->SelectedCrewMember->entity_max_health );
				if( PlayerShip->SelectedCrewMember->ShipOnBoard == PlayerShip ) {
					CaptainGui->SetStateBool( "playership_selected_officer_off_ship_overlay_visible", false );
				} else {
					CaptainGui->SetStateBool( "playership_selected_officer_off_ship_overlay_visible", true );
				}
			} else if ( selected_reserve_crew_dict ) {
				CaptainGui->SetStateFloat( "playership_selected_officer_portrait_rect_pos_x", CaptainGui->CursorX() );
				CaptainGui->SetStateFloat( "playership_selected_officer_portrait_rect_pos_y", CaptainGui->CursorY() );

				CaptainGui->SetStateBool( "playership_selected_officer_portrait_visible", true );
				CaptainGui->SetStateString( "playership_selected_officer_portrait",  selected_reserve_crew_dict->GetString("ship_diagram_portrait", "guis/assets/steve_captain_display/CrewPortraits/CrewPortraitDefault.tga") );
				CaptainGui->SetStateBool( "playership_selected_officer_auto_mode_activated", false );
				CaptainGui->SetStateBool( "playership_selected_officer_player_follow_mode_activated", false );

				CaptainGui->SetStateString( "playership_selected_officer_npc_name", selected_reserve_crew_dict->GetString( "npc_name", "Joe" ) );
				CaptainGui->SetStateString( "playership_selected_officer_parentship", PlayerShip->original_name );

				CaptainGui->SetStateString( "playership_selected_officer_species", selected_reserve_crew_dict->GetString( "space_command_species", "Homo Sapiens" ) );
				CaptainGui->SetStateString( "playership_selected_officer_mass", selected_reserve_crew_dict->GetString( "mass", "150" ) + idStr(" Kilograms") );
				CaptainGui->SetStateString( "playership_selected_officer_vocation", selected_reserve_crew_dict->GetString( "space_command_vocation", "Explorer/Scientist" ) );
				int temp_module_buff_amount = selected_reserve_crew_dict->GetInt( "module_buff_amount", "20" );// * idMath::Pow(SPACE_COMMAND_LEVEL_STAT_MODIFIER, selected_reserve_crew_dict->GetInt( "space_command_level", "1" ) - 1.0f );
				int temp_space_command_level = selected_reserve_crew_dict->GetInt( "space_command_level", "1" );
				for ( int i = 0; i < temp_space_command_level - 1; i++ ) {
					temp_module_buff_amount = temp_module_buff_amount * SPACE_COMMAND_LEVEL_STAT_MODIFIER;
				}
				CaptainGui->SetStateInt( "playership_selected_officer_tech_skill_rating", temp_module_buff_amount );
				float temp_attack_accuracy = selected_reserve_crew_dict->GetFloat("attack_accuracy", "7");// * idMath::Pow(SPACE_COMMAND_LEVEL_STAT_MODIFIER, temp_space_command_level - 1.0f );
				for ( int i = 0; i < temp_space_command_level - 1; i++ ) {
					temp_attack_accuracy = temp_attack_accuracy / SPACE_COMMAND_LEVEL_STAT_MODIFIER;
				}
				CaptainGui->SetStateString( "playership_selected_officer_armament", selected_reserve_crew_dict->GetString( "space_command_armament", "Basic Plasma Weapon" ) + idStr(" / Rank: ") + idStr((int)temp_attack_accuracy)); //selected_reserve_crew_dict->GetInt("attack_accuracy", "7")) );
				CaptainGui->SetStateString( "playership_selected_officer_space_command_description", selected_reserve_crew_dict->GetString( "space_command_tablet_description", "NO INFO AVAILABLE" ) );

				const char* temp_module_console_damage_def_name = selected_reserve_crew_dict->GetString("module_console_damage_def","damage_module_console_default");
				const char* temp_door_damage_def_name = selected_reserve_crew_dict->GetString("door_damage_def","damage_door_default");
				if ( temp_module_console_damage_def_name ) {
					const idDict *damageDef = gameLocal.FindEntityDefDict( temp_module_console_damage_def_name );
					if ( !damageDef ) {
						gameLocal.Error( "Unknown damageDef '%s'\n", temp_module_console_damage_def_name );
					}
					float engineer_skill_rating_damageScale = 1.0f;
					engineer_skill_rating_damageScale = idMath::Pow(SPACE_COMMAND_LEVEL_STAT_MODIFIER, selected_reserve_crew_dict->GetInt( "space_command_level", "1" ) - 1.0f );
					int	engineer_skill_rating = damageDef->GetInt( "damage" ) * engineer_skill_rating_damageScale;
					CaptainGui->SetStateInt( "playership_selected_officer_engineer_skill_rating", engineer_skill_rating );
				}
				if ( temp_door_damage_def_name ) {
					const idDict *damageDef = gameLocal.FindEntityDefDict( temp_door_damage_def_name );
					if ( !damageDef ) {
						gameLocal.Error( "Unknown damageDef '%s'\n", temp_door_damage_def_name );
					}
					float door_jacking_rating_damageScale = 1.0f;
					door_jacking_rating_damageScale = idMath::Pow(SPACE_COMMAND_LEVEL_STAT_MODIFIER, selected_reserve_crew_dict->GetInt( "space_command_level", "1" ) - 1.0f );
					int	door_jacking_rating = damageDef->GetInt( "damage" ) * door_jacking_rating_damageScale;
					CaptainGui->SetStateInt( "playership_selected_officer_door_jacking_skill_rating", door_jacking_rating );
				}

				CaptainGui->SetStateInt( "playership_selected_officer_space_command_level", selected_reserve_crew_dict->GetInt( "space_command_level", "1" ) );

				CaptainGui->SetStateFloat( "playership_selected_officer_health_percentage", 1.0f );
				CaptainGui->SetStateBool( "playership_selected_officer_off_ship_overlay_visible", false );
			} else {
				CaptainGui->SetStateBool( "playership_selected_officer_portrait_visible", false );
			}

			for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
				if ( PlayerShip->crew[i] && !PlayerShip->crew[i]->was_killed ) {
					CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_portrait_visible", true );
					CaptainGui->SetStateString( "playership_" + role_description[i] + "_officer_portrait", PlayerShip->crew[i]->ship_diagram_portrait );
					CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_auto_mode_activated", PlayerShip->crew[i]->crew_auto_mode_activated );
					CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_player_follow_mode_activated", PlayerShip->crew[i]->player_follow_mode_activated );
					CaptainGui->SetStateString( "playership_" + role_description[i] + "_officer_npc_name", PlayerShip->crew[i]->spawnArgs.GetString( "npc_name", "Joe" ) );
					CaptainGui->SetStateString( "playership_" + role_description[i] + "_officer_insignia", PlayerShip->crew[i]->ship_diagram_icon );
					CaptainGui->SetStateInt( "playership_" + role_description[i] + "_officer_space_command_level", PlayerShip->crew[i]->space_command_level );
					CaptainGui->SetStateInt( "playership_" + role_description[i] + "_officer_name_locked", PlayerShip->crew[i]->npc_name_locked );
					if( PlayerShip->crew[i]->ShipOnBoard == PlayerShip ) {
						CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_off_ship_overlay_visible", false );
					} else {
						CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_off_ship_overlay_visible", true );
					}
					CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_is_grabbed", grabbed_crew_member && grabbed_crew_member == PlayerShip->crew[i] );
				} else {
					CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_portrait_visible", false );
				}
			}

			if ( PlayerShip->TargetEntityInSpace ) {

				for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
					if ( PlayerShip->TargetEntityInSpace->crew[i] && !PlayerShip->TargetEntityInSpace->crew[i]->was_killed ) {
						CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_portrait_visible", true );
						CaptainGui->SetStateString( "targetship_" + role_description[i] + "_officer_portrait", PlayerShip->TargetEntityInSpace->crew[i]->ship_diagram_portrait );
						CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_auto_mode_activated", PlayerShip->TargetEntityInSpace->crew[i]->crew_auto_mode_activated );
						CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_player_follow_mode_activated", PlayerShip->TargetEntityInSpace->crew[i]->player_follow_mode_activated );
						if( PlayerShip->TargetEntityInSpace->crew[i]->ShipOnBoard == PlayerShip->TargetEntityInSpace ) {
							CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_off_ship_overlay_visible", false );
						} else {
							CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_off_ship_overlay_visible", true );
						}
					} else {
						CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_portrait_visible", false );
					}
				}
			} else {
				for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
					CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_portrait_visible", false );
				}
			}
		} else {
			for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
				CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_portrait_visible", false );
			}
		}
	}
}

void idPlayer::UpdateReserveCrewMemberPortraits() {
	if ( PlayerShip ) {
		struct sort_reserve_Crew {
			bool operator()(const idDict &left, const idDict &right) {
				return left.GetInt( "space_command_level", "1" ) > right.GetInt( "space_command_level", "1" );
			}
		};
		std::sort(PlayerShip->reserve_Crew.begin(), PlayerShip->reserve_Crew.end(), sort_reserve_Crew());

		if ( CaptainGui ) {
			CaptainGui->SetStateString( "playership_reserve_crew_title_text", "Reserve Crew: " + idStr((unsigned)PlayerShip->reserve_Crew.size()) + "/" + idStr(PlayerShip->max_reserve_crew) );
			CaptainGui->SetStateInt( "playership_reserve_crew_max_size", PlayerShip->max_reserve_crew );
			for ( int i = 0; i < MAX_RESERVE_CREW_ON_SHIPS; i++ ) {
				//gameLocal.Printf( "PlayerShip->reserve_Crew.size() = %i \n", PlayerShip->reserve_Crew.size() );
				if ( i < PlayerShip->reserve_Crew.size() ) {
					CaptainGui->SetStateBool( "playership_reserve_crew_portrait_" + idStr(i) + "_visible", true );
					CaptainGui->SetStateString( "playership_reserve_crew_portrait_" + idStr(i), PlayerShip->reserve_Crew[i].GetString("ship_diagram_portrait", "guis/assets/steve_captain_display/CrewPortraits/CrewPortraitDefault.tga") );
					CaptainGui->SetStateString( "playership_reserve_crew_portrait_" + idStr(i) + "_name", PlayerShip->reserve_Crew[i].GetString( "npc_name", "Joe" ) );
					CaptainGui->SetStateInt( "playership_reserve_crew_portrait_" + idStr(i) + "_text", PlayerShip->reserve_Crew[i].GetInt( "space_command_level", "1" ) );
					CaptainGui->SetStateString( "playership_reserve_crew_portrait_" + idStr(i) + "_insignia", PlayerShip->reserve_Crew[i].GetString(va("ship_diagram_icon%i",PlayerShip->reserve_Crew[i].GetInt( "space_command_level", "1" )), "guis/assets/steve_captain_display/CrewDiagramIcons/CrewDiagramIconDefault.tga") );
				} else {
					CaptainGui->SetStateBool( "playership_reserve_crew_portrait_" + idStr(i) + "_visible", false );
					continue;
				}
			}
		}
	}
}

void idPlayer::CheckForPlayerShipCrewMemberNameChanges() {
	if ( PlayerShip ) {
		if ( PlayerShip->crew[MEDICALCREWID] && !PlayerShip->crew[MEDICALCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[MEDICALCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_medical_officer_name.GetString(), old_name.c_str() ) ) { // returns false if strings are equal - so it will return true if the strings are not equal
				PlayerShip->crew[MEDICALCREWID]->spawnArgs.Set( "npc_name", sc_medical_officer_name.GetString() );
				PlayerShip->crew[MEDICALCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[ENGINESCREWID] && !PlayerShip->crew[ENGINESCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[ENGINESCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_engines_officer_name.GetString(), old_name.c_str()) ) {
				PlayerShip->crew[ENGINESCREWID]->spawnArgs.Set( "npc_name", sc_engines_officer_name.GetString() );
				PlayerShip->crew[ENGINESCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[WEAPONSCREWID] && !PlayerShip->crew[WEAPONSCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[WEAPONSCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_weapons_officer_name.GetString(),old_name.c_str()) ) {
				PlayerShip->crew[WEAPONSCREWID]->spawnArgs.Set( "npc_name", sc_weapons_officer_name.GetString() );
				PlayerShip->crew[WEAPONSCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[TORPEDOSCREWID] && !PlayerShip->crew[TORPEDOSCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[TORPEDOSCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_torpedos_officer_name.GetString(), old_name.c_str()) ) {
				PlayerShip->crew[TORPEDOSCREWID]->spawnArgs.Set( "npc_name", sc_torpedos_officer_name.GetString() );
				PlayerShip->crew[TORPEDOSCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[SHIELDSCREWID] && !PlayerShip->crew[SHIELDSCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[SHIELDSCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_shields_officer_name.GetString(), old_name.c_str()) ) {
				PlayerShip->crew[SHIELDSCREWID]->spawnArgs.Set( "npc_name", sc_shields_officer_name.GetString() );
				PlayerShip->crew[SHIELDSCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[SENSORSCREWID] && !PlayerShip->crew[SENSORSCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[SENSORSCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_sensors_officer_name.GetString(), old_name.c_str()) ) {
				PlayerShip->crew[SENSORSCREWID]->spawnArgs.Set( "npc_name", sc_sensors_officer_name.GetString() );
				PlayerShip->crew[SENSORSCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[ENVIRONMENTCREWID] && !PlayerShip->crew[ENVIRONMENTCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[ENVIRONMENTCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_environment_officer_name.GetString(), old_name.c_str()) ) {
				PlayerShip->crew[ENVIRONMENTCREWID]->spawnArgs.Set( "npc_name", sc_environment_officer_name.GetString() );
				PlayerShip->crew[ENVIRONMENTCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[COMPUTERCREWID] && !PlayerShip->crew[COMPUTERCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[COMPUTERCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_computer_officer_name.GetString(), old_name.c_str()) ) {
				PlayerShip->crew[COMPUTERCREWID]->spawnArgs.Set( "npc_name", sc_computer_officer_name.GetString() );
				PlayerShip->crew[COMPUTERCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[SECURITYCREWID] && !PlayerShip->crew[SECURITYCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[SECURITYCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_security_officer_name.GetString(), old_name.c_str()) ) {
				PlayerShip->crew[SECURITYCREWID]->spawnArgs.Set( "npc_name", sc_security_officer_name.GetString() );
				PlayerShip->crew[SECURITYCREWID]->npc_name_locked = true;
			}
		}
		if ( PlayerShip->crew[CAPTAINCREWID] && !PlayerShip->crew[CAPTAINCREWID]->npc_name_locked ) {
			idStr old_name = PlayerShip->crew[CAPTAINCREWID]->spawnArgs.GetString( "npc_name", "Joe" );
			if ( idStr::Icmp(sc_captain_officer_name.GetString(), old_name.c_str()) ) {
				PlayerShip->crew[CAPTAINCREWID]->spawnArgs.Set( "npc_name", sc_captain_officer_name.GetString() );
				PlayerShip->crew[CAPTAINCREWID]->npc_name_locked = true;
			}
		}
	}
}

void idPlayer::SyncUpPlayerShipNameCVars() {
	if ( PlayerShip ) {
		if ( PlayerShip->crew[MEDICALCREWID] ) {
			sc_medical_officer_name.SetString(PlayerShip->crew[MEDICALCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[ENGINESCREWID] ) {
			sc_engines_officer_name.SetString(PlayerShip->crew[ENGINESCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[WEAPONSCREWID] ) {
			sc_weapons_officer_name.SetString(PlayerShip->crew[WEAPONSCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[TORPEDOSCREWID] ) {
			sc_torpedos_officer_name.SetString(PlayerShip->crew[TORPEDOSCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[SHIELDSCREWID] ) {
			sc_shields_officer_name.SetString(PlayerShip->crew[SHIELDSCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[SENSORSCREWID] ) {
			sc_sensors_officer_name.SetString(PlayerShip->crew[SENSORSCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[ENVIRONMENTCREWID] ) {
			sc_environment_officer_name.SetString(PlayerShip->crew[ENVIRONMENTCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[COMPUTERCREWID] ) {
			sc_computer_officer_name.SetString(PlayerShip->crew[COMPUTERCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[SECURITYCREWID] ) {
			sc_security_officer_name.SetString(PlayerShip->crew[SECURITYCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
		if ( PlayerShip->crew[CAPTAINCREWID] ) {
			sc_captain_officer_name.SetString(PlayerShip->crew[CAPTAINCREWID]->spawnArgs.GetString( "npc_name", "Joe" ));
		}
	}
}

// we could set the materials to update only when they actually need to and not every frame.(when any transport happens, when any space entity is targeted and when the game starts).
void idPlayer::UpdateWeaponsAndTorpedosQueuesOnCaptainGui() {

	if ( CaptainGui ) {
		if ( PlayerShip ) { // might want to put a check in any function that accesses the CaptainGui to make sure it exists first - currently some functions are lacking this check.

			CaptainGui->SetStateBool( "playership_weapons_module_autofire_on", PlayerShip->weapons_autofire_on );
			CaptainGui->SetStateBool( "playership_torpedos_module_autofire_on", PlayerShip->torpedos_autofire_on );

			//if ( PlayerShip->weapons_autofire_on ) {
				if ( PlayerShip->consoles[WEAPONSMODULEID] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule ) {
					for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
						if ( PlayerShip->WeaponsTargetQueue[i] == MEDICALMODULEID ) {
							CaptainGui->SetStateInt( "playership_medical_module_position_in_weapons_queue", i );
						} else if ( PlayerShip->WeaponsTargetQueue[i] == ENGINESMODULEID ) {
							CaptainGui->SetStateInt( "playership_engines_module_position_in_weapons_queue", i );
						} else if ( PlayerShip->WeaponsTargetQueue[i] == WEAPONSMODULEID ) {
							CaptainGui->SetStateInt( "playership_weapons_module_position_in_weapons_queue", i );
						} else if ( PlayerShip->WeaponsTargetQueue[i] == TORPEDOSMODULEID ) {
							CaptainGui->SetStateInt( "playership_torpedos_module_position_in_weapons_queue", i );
						} else if ( PlayerShip->WeaponsTargetQueue[i] == SHIELDSMODULEID ) {
							CaptainGui->SetStateInt( "playership_shields_module_position_in_weapons_queue", i );
						} else if ( PlayerShip->WeaponsTargetQueue[i] == SENSORSMODULEID ) {
							CaptainGui->SetStateInt( "playership_sensors_module_position_in_weapons_queue", i );
						} else if ( PlayerShip->WeaponsTargetQueue[i] == ENVIRONMENTMODULEID ) {
							CaptainGui->SetStateInt( "playership_environment_module_position_in_weapons_queue", i );
						} else if ( PlayerShip->WeaponsTargetQueue[i] == COMPUTERMODULEID ) {
							CaptainGui->SetStateInt( "playership_computer_module_position_in_weapons_queue", i );
						} else if ( PlayerShip->WeaponsTargetQueue[i] == SECURITYMODULEID ) {
							CaptainGui->SetStateInt( "playership_security_module_position_in_weapons_queue", i );
						}
					}
				}
			/*} else {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_position_in_weapons_queue", i );
				}
			}*/
			//if ( PlayerShip->torpedos_autofire_on ) {
				if ( PlayerShip->consoles[TORPEDOSMODULEID] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule ) {
					for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
						if ( PlayerShip->TorpedosTargetQueue[i] == MEDICALMODULEID ) {
							CaptainGui->SetStateInt( "playership_medical_module_position_in_torpedos_queue", i );
						} else if ( PlayerShip->TorpedosTargetQueue[i] == ENGINESMODULEID ) {
							CaptainGui->SetStateInt( "playership_engines_module_position_in_torpedos_queue", i );
						} else if ( PlayerShip->TorpedosTargetQueue[i] == WEAPONSMODULEID ) {
							CaptainGui->SetStateInt( "playership_weapons_module_position_in_torpedos_queue", i );
						} else if ( PlayerShip->TorpedosTargetQueue[i] == TORPEDOSMODULEID ) {
							CaptainGui->SetStateInt( "playership_torpedos_module_position_in_torpedos_queue", i );
						} else if ( PlayerShip->TorpedosTargetQueue[i] == SHIELDSMODULEID ) {
							CaptainGui->SetStateInt( "playership_shields_module_position_in_torpedos_queue", i );
						} else if ( PlayerShip->TorpedosTargetQueue[i] == SENSORSMODULEID ) {
							CaptainGui->SetStateInt( "playership_sensors_module_position_in_torpedos_queue", i );
						} else if ( PlayerShip->TorpedosTargetQueue[i] == ENVIRONMENTMODULEID ) {
							CaptainGui->SetStateInt( "playership_environment_module_position_in_torpedos_queue", i );
						} else if ( PlayerShip->TorpedosTargetQueue[i] == COMPUTERMODULEID ) {
							CaptainGui->SetStateInt( "playership_computer_module_position_in_torpedos_queue", i );
						} else if ( PlayerShip->TorpedosTargetQueue[i] == SECURITYMODULEID ) {
							CaptainGui->SetStateInt( "playership_security_module_position_in_torpedos_queue", i );
						}
					}
				}
			/*} else {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_position_in_torpedos_queue", i );
				}
			}*/

			// BEGIN TargetShip details ordering based on weapons and torpedos queue position combination
			float	weapons_priority[MAX_MODULES_ON_SHIPS] = {}; //  = {}; ensures it will initialize to zero - although it might automatically
			float	torpedos_priority[MAX_MODULES_ON_SHIPS] = {}; //  = {}; ensures it will initialize to zero - although it might automatically
			float	overall_priority[MAX_MODULES_ON_SHIPS] = {}; //  = {}; ensures it will initialize to zero - although it might automatically
			if ( PlayerShip->weapons_autofire_on || PlayerShip->torpedos_autofire_on ) {

				if ( PlayerShip->weapons_autofire_on ) {
					if ( PlayerShip->consoles[WEAPONSMODULEID] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule ) {
						for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
							for ( int module_id_to_check = 0; module_id_to_check < MAX_MODULES_ON_SHIPS; module_id_to_check++ ) {
								if ( PlayerShip->WeaponsTargetQueue[i] == module_id_to_check ) {
									weapons_priority[module_id_to_check] = i;
								}
							}
						}
					}
				}

				if ( PlayerShip->torpedos_autofire_on ) {
					if ( PlayerShip->consoles[TORPEDOSMODULEID] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule ) {
						for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
							for ( int module_id_to_check = 0; module_id_to_check < MAX_MODULES_ON_SHIPS; module_id_to_check++ ) {
								if ( PlayerShip->TorpedosTargetQueue[i] == module_id_to_check ) {
									torpedos_priority[module_id_to_check] = i;
								}
							}
						}
					}
				}

				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					overall_priority[i] = weapons_priority[i] + torpedos_priority[i] + ( i * 0.0003 ); // this is to prevent duplicates.
				}

				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					int rank_counter = 0;
					for (  int x = 0; x < MAX_MODULES_ON_SHIPS; x++ ) {
						if ( overall_priority[i] > overall_priority[x] ) {
							rank_counter++;
						}
					}
					CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_overall_queue_position", rank_counter );
				}

			} else {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_overall_queue_position", i );
				}
			}
			// END TargetShip details ordering based on weapons and torpedos queue position combination
		}
	}
}

void idPlayer::UpdateWeaponsAndTorpedosTargetingOverlays() {
	if ( CaptainGui ) {
		if ( PlayerShip ) {
			if ( PlayerShip->TargetEntityInSpace ) {
				if ( PlayerShip->consoles[WEAPONSMODULEID] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule ) {
					CaptainGui->SetStateInt( "targetship_diagram_playership_weapons_target_overlay_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule) );
					CaptainGui->SetStateInt( "targetship_diagram_playership_weapons_target_overlay_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule) );
					CaptainGui->SetStateBool( "targetship_diagram_playership_weapons_target_overlay_visible", true );
				} else {
					CaptainGui->SetStateBool( "targetship_diagram_playership_weapons_target_overlay_visible", false );
				}
				if ( PlayerShip->consoles[TORPEDOSMODULEID] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule ) {
					CaptainGui->SetStateInt( "targetship_diagram_playership_torpedos_target_overlay_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule) );
					CaptainGui->SetStateInt( "targetship_diagram_playership_torpedos_target_overlay_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule) );
					CaptainGui->SetStateBool( "targetship_diagram_playership_torpedos_target_overlay_visible", true );
				} else {
					CaptainGui->SetStateBool( "targetship_diagram_playership_torpedos_target_overlay_visible", false );
				}

				if ( PlayerShip->TargetEntityInSpace->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->TargetEntityInSpace == PlayerShip ) {
					if ( PlayerShip->TargetEntityInSpace->consoles[WEAPONSMODULEID] && PlayerShip->TargetEntityInSpace->consoles[WEAPONSMODULEID]->ControlledModule && PlayerShip->TargetEntityInSpace->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule ) {
						CaptainGui->SetStateInt( "playership_diagram_targetship_weapons_target_overlay_pos_x", PlayerShip->TargetEntityInSpace->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule) );
						CaptainGui->SetStateInt( "playership_diagram_targetship_weapons_target_overlay_pos_y", -PlayerShip->TargetEntityInSpace->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule) );
						CaptainGui->SetStateBool( "playership_diagram_targetship_weapons_target_overlay_visible", true );
					} else {
						CaptainGui->SetStateBool( "playership_diagram_targetship_weapons_target_overlay_visible", false );
					}
					if ( PlayerShip->TargetEntityInSpace->consoles[TORPEDOSMODULEID] && PlayerShip->TargetEntityInSpace->consoles[TORPEDOSMODULEID]->ControlledModule && PlayerShip->TargetEntityInSpace->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule ) {
						CaptainGui->SetStateInt( "playership_diagram_targetship_torpedos_target_overlay_pos_x", PlayerShip->TargetEntityInSpace->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule) );
						CaptainGui->SetStateInt( "playership_diagram_targetship_torpedos_target_overlay_pos_y", -PlayerShip->TargetEntityInSpace->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule) );
						CaptainGui->SetStateBool( "playership_diagram_targetship_torpedos_target_overlay_visible", true );
					} else {
						CaptainGui->SetStateBool( "playership_diagram_targetship_torpedos_target_overlay_visible", false );
					}
				} else {
					CaptainGui->SetStateBool( "playership_diagram_targetship_weapons_target_overlay_visible", false );
					CaptainGui->SetStateBool( "playership_diagram_targetship_torpedos_target_overlay_visible", false );
				}
			} else {
				CaptainGui->SetStateBool( "targetship_diagram_playership_weapons_target_overlay_visible", false );
				CaptainGui->SetStateBool( "targetship_diagram_playership_torpedos_target_overlay_visible", false );

				CaptainGui->SetStateBool( "playership_diagram_targetship_weapons_target_overlay_visible", false );
				CaptainGui->SetStateBool( "playership_diagram_targetship_torpedos_target_overlay_visible", false );
			}

			if ( PlayerShip->consoles[WEAPONSMODULEID] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule && PlayerShip->TargetEntityInSpace ) {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					if ( PlayerShip->TargetEntityInSpace->consoles[i] && PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule->CurrentTargetModule == PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule ) {
						CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_is_current_weapons_target", true );
					} else {
						CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_is_current_weapons_target", false );
					}
				}
			} else {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_is_current_weapons_target", false );
				}
			}
			if ( PlayerShip->consoles[TORPEDOSMODULEID] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule && PlayerShip->TargetEntityInSpace ) {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					if ( PlayerShip->TargetEntityInSpace->consoles[i] && PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule->CurrentTargetModule == PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule ) {
						CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_is_current_torpedos_target", true );
					} else {
						CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_is_current_torpedos_target", false );
					}
				}
			} else {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_is_current_torpedos_target", false );
				}
			}
		}
	}
}

void idPlayer::UpdateModulesPowerQueueOnCaptainGui() {
	if ( CaptainGui ) {
		if ( PlayerShip ) {

			// if ( PlayerShip->WeaponsConsole && PlayerShip->WeaponsConsole->ControlledModule ) { // going to want to put a check here for the power core.

			CaptainGui->SetStateBool( "playership_modules_power_automanage_on", PlayerShip->modules_power_automanage_on );

			CaptainGui->SetStateInt( "playership_automanage_power_reserve", PlayerShip->current_automanage_power_reserve );

			if ( PlayerShip->modules_power_automanage_on ) {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					for ( int x = 0; x < MAX_MODULES_ON_SHIPS; x++ ) {
						if ( PlayerShip->ModulesPowerQueue[i] == MEDICALMODULEID ) {
							CaptainGui->SetStateInt( "playership_medical_module_position_in_power_queue", i );
						} else if ( PlayerShip->ModulesPowerQueue[i] == ENGINESMODULEID ) {
							CaptainGui->SetStateInt( "playership_engines_module_position_in_power_queue", i );
						} else if ( PlayerShip->ModulesPowerQueue[i] == WEAPONSMODULEID ) {
							CaptainGui->SetStateInt( "playership_weapons_module_position_in_power_queue", i );
						} else if ( PlayerShip->ModulesPowerQueue[i] == TORPEDOSMODULEID ) {
							CaptainGui->SetStateInt( "playership_torpedos_module_position_in_power_queue", i );
						} else if ( PlayerShip->ModulesPowerQueue[i] == SHIELDSMODULEID ) {
							CaptainGui->SetStateInt( "playership_shields_module_position_in_power_queue", i );
						} else if ( PlayerShip->ModulesPowerQueue[i] == SENSORSMODULEID ) {
							CaptainGui->SetStateInt( "playership_sensors_module_position_in_power_queue", i );
						} else if ( PlayerShip->ModulesPowerQueue[i] == ENVIRONMENTMODULEID ) {
							CaptainGui->SetStateInt( "playership_environment_module_position_in_power_queue", i );
						} else if ( PlayerShip->ModulesPowerQueue[i] == COMPUTERMODULEID ) {
							CaptainGui->SetStateInt( "playership_computer_module_position_in_power_queue", i );
						} else if ( PlayerShip->ModulesPowerQueue[i] == SECURITYMODULEID ) {
							CaptainGui->SetStateInt( "playership_security_module_position_in_power_queue", i );
						}
					}
				}
			} else {
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_position_in_power_queue", i );
				}
			}
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
				if ( PlayerShip->consoles[i] && PlayerShip->consoles[i]->ControlledModule ) {
					CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_automanage_target_power_level", PlayerShip->consoles[i]->ControlledModule->automanage_target_power_level );
				}
			}
		}
	}
}

void idPlayer::UpdateGuiOverlay() {
	if ( guiOverlay ) {

		if ( PlayerShip ) {
			if ( PlayerShip->CanHireCrew() || PlayerShip->CanHireReserveCrew() ) {
				int open_main_crew_positions = 0;
				for( int i = 0; i < MAX_CREW_ON_SHIPS - 1; i++ ) { // MAX_CREW_ON_SHIPS - 1 because we want to exclude the position of captain - captains will not be allowed to be hired. 
					if ( PlayerShip->crew[i] == NULL ) {
						open_main_crew_positions++;
					}
				}
				int open_reserve_crew_positions = PlayerShip->max_reserve_crew - PlayerShip->reserve_Crew.size();
				guiOverlay->SetStateInt("playership_open_crew_positions_num", open_main_crew_positions + open_reserve_crew_positions );
			} else {
				guiOverlay->SetStateInt("playership_open_crew_positions_num", 0 );
			}

			int PlayerPosX = PlayerShip->stargridpositionx;
			int PlayerPosY = PlayerShip->stargridpositiony;
			if ( ShipOnBoard && ShipOnBoard != PlayerShip ) {
				PlayerPosX = ShipOnBoard->stargridpositionx;
				PlayerPosY = ShipOnBoard->stargridpositiony;
			}
			guiOverlay->SetStateString("position_text", va("You have arrived at sector coordinates: ^5%i,%i^0",PlayerPosX ,PlayerPosY) );

			guiOverlay->SetStateInt("playership_currency_reserves", PlayerShip->current_currency_reserves );
			guiOverlay->SetStateInt("playership_materials_reserves", PlayerShip->current_materials_reserves );

			guiOverlay->SetStateString( "playership_name", "^4" + PlayerShip->name );
			guiOverlay->SetStateString( "playership_name_with_colon", "^4" + PlayerShip->name + ":" );

			int num_friendly_entities = 0;
			int num_neutral_entities = 0;
			int num_hostile_entities = 0;
			int num_derelict_entities = 0;

			int num_incoming_hostile_entities = 0;

			for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size(); i++ ) {
				sbShip* ShipToEvaluate = PlayerShip->ships_at_my_stargrid_position[i];
				if ( ShipToEvaluate->team == team ) {
					num_friendly_entities++;
				} else if ( ShipToEvaluate->team != team && ShipToEvaluate->HasNeutralityWithTeam(team) && !ShipToEvaluate->is_derelict ) {
					num_neutral_entities++;
				} else if ( !ShipToEvaluate->is_derelict ) {
					num_hostile_entities++;
					if ( ShipToEvaluate->should_warp_in_when_first_encountered ) {
						num_incoming_hostile_entities++;
					}
				} else if ( ShipToEvaluate->is_derelict ) {
					num_derelict_entities++;
				}
			}

			guiOverlay->SetStateInt("num_friendly_entities_in_local_space", num_friendly_entities );
			guiOverlay->SetStateInt("num_neutral_entities_in_local_space", num_neutral_entities );
			guiOverlay->SetStateInt("num_hostile_entities_in_local_space", num_hostile_entities );
			guiOverlay->SetStateInt("num_derelict_entities_in_local_space", num_derelict_entities );

			if ( num_hostile_entities == 1 ) {
				if ( num_incoming_hostile_entities > 0 ) {
					guiOverlay->SetStateString( "warning_text", "WARNING: INCOMING HOSTILE DETECTED" );
				} else {
					guiOverlay->SetStateString( "warning_text", "WARNING: Hostile entity detected in local space." );
				}
			} else if ( num_hostile_entities > 1 ) {
				if ( num_incoming_hostile_entities > 1 ) {
					guiOverlay->SetStateString( "warning_text", "WARNING: INCOMING HOSTILES DETECTED" );
				} else if ( num_incoming_hostile_entities == 1 ) {
					guiOverlay->SetStateString( "warning_text", "WARNING: INCOMING HOSTILE DETECTED" );
				} else {
					guiOverlay->SetStateString( "warning_text", "WARNING: Hostile entities detected in local space." );
				}
			} else {
				guiOverlay->SetStateString( "warning_text", "" );
			}
		}

	}
}

void idPlayer::UpdateHailGui() {
	int i;
	i = 0;

	if ( HailGui ) {
		if ( PlayerShip && SelectedEntityInSpace ) {
			HailGui->SetStateInt("selectedship_friendliness_with_player", SelectedEntityInSpace->friendlinessWithPlayer );
			HailGui->SetStateBool("selectedship_dialogue_branch_forgive_visible", SelectedEntityInSpace->has_forgiven_player );
			HailGui->SetStateBool("selectedship_dialogue_branch_ignore_visible", SelectedEntityInSpace->is_ignoring_player );

			HailGui->SetStateInt("selectedship_currency_reserves", SelectedEntityInSpace->current_currency_reserves );
			HailGui->SetStateInt("selectedship_materials_reserves", SelectedEntityInSpace->current_materials_reserves );
			HailGui->SetStateInt("playership_currency_reserves", PlayerShip->current_currency_reserves );
			HailGui->SetStateInt("playership_materials_reserves", PlayerShip->current_materials_reserves );


			HailGui->SetStateString( "playership_name", "^4" + PlayerShip->name );
			HailGui->SetStateString( "playership_name_with_colon", "^4" + PlayerShip->name + ":" );
			if ( SelectedEntityInSpace->team == team ) {
				HailGui->SetStateString( "selectedship_name", "^4" + SelectedEntityInSpace->original_name );
				HailGui->SetStateString( "selectedship_name_with_colon", "^4" + SelectedEntityInSpace->original_name + ":" );
			} else if ( PlayerShip->HasNeutralityWithShip(SelectedEntityInSpace) ) {
				HailGui->SetStateString( "selectedship_name", "^8" + SelectedEntityInSpace->original_name );
				HailGui->SetStateString( "selectedship_name_with_colon", "^8" + SelectedEntityInSpace->original_name + ":" );
			} else {
				HailGui->SetStateString( "selectedship_name", "^1" + SelectedEntityInSpace->original_name );
				HailGui->SetStateString( "selectedship_name_with_colon", "^1" + SelectedEntityInSpace->original_name + ":" );
			}
			HailGui->SetStateString( "selectedship_name_no_color", SelectedEntityInSpace->original_name );
			HailGui->SetStateString( "selectedship_name_with_colon_no_color", SelectedEntityInSpace->original_name + ":" );

			HailGui->SetStateString( "hailed_ship_image_visual", SelectedEntityInSpace->ShipImageVisual );
			// Find a crewmember aboard the selected ship to communicate with. Tryt eh captain first, if he/she is not available then any other crewmember.
			if ( SelectedEntityInSpace->crew[CAPTAINCREWID] && !SelectedEntityInSpace->crew[CAPTAINCREWID]->was_killed && SelectedEntityInSpace->crew[CAPTAINCREWID]->ShipOnBoard == SelectedEntityInSpace ) {
				HailGui->SetStateString( "hailed_ship_communicating_crewmember_portrait", SelectedEntityInSpace->crew[CAPTAINCREWID]->ship_diagram_portrait );
			} else {
				for( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
					if ( SelectedEntityInSpace->crew[i] && !SelectedEntityInSpace->crew[i]->was_killed && SelectedEntityInSpace->crew[i]->ShipOnBoard == SelectedEntityInSpace ) {
						HailGui->SetStateString( "hailed_ship_communicating_crewmember_portrait", SelectedEntityInSpace->crew[i]->ship_diagram_portrait );
						break;
					}
				}
				HailGui->SetStateString( "hailed_ship_communicating_crewmember_portrait", SelectedEntityInSpace->ShipImageVisual );
			}
		}

		if ( PlayerShip && SelectedEntityInSpace ) {
			HailGui->SetStateBool("selectedship_in_no_action_hail_mode", SelectedEntityInSpace->in_no_action_hail_mode );
			HailGui->SetStateBool("paused_in_no_action_hail_mode", g_stopTime.GetBool() );
		}

		// check the relationshipToPlayer int here to see if it is low enough to become hostile to the player. If it is, then set the last bit of hailDialogueBranchTracker with hailDialogueBranchTracker.reset(), and then hailDialogueBranchTracker.set(MAX_DIALOGUE_BRANCHES-1)
			HailGui->SetStateBool("selectedship_dialogue_branch_hostile_visible", false );
			HailGui->SetStateBool("selectedship_dialogue_branch_conditional_visible", false );
			//gameLocal.Printf( "The hailgui has been updated.\n" );
		if ( PlayerShip && SelectedEntityInSpace && SelectedEntityInSpace->hail_conditionals_met ) {
			HailGui->SetStateBool("selectedship_dialogue_branch_conditional_visible", true );
			//gameLocal.Printf( "The hail_conditionals_met bool is set to true.\n" );

			for ( i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
				HailGui->SetStateBool( va("selectedship_dialogue_branch_%i_visible", i ), false );
			}
			return;
		} else {
			if ( PlayerShip && SelectedEntityInSpace && SelectedEntityInSpace->IsHostileShip(PlayerShip) /*&& SelectedEntityInSpace->friendlinessWithPlayer <= 0*/ && !SelectedEntityInSpace->in_no_action_hail_mode && !delay_selectedship_dialogue_branch_hostile_visible ) {
				HailGui->SetStateBool("selectedship_dialogue_branch_hostile_visible", true );
				// need to put a function here for the selected ship that will change it's team to something different than the player and do the same for all entities with that same team on the entire map. Can just loop through all entities on the map with a team equal to the selectedship and pick a random number between 1 and a billion for their team. Or you could get the max of all entity teams and add one. Or you could cycle through numbers till you find one that no other entity has as it's team.
				for ( i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
					HailGui->SetStateBool( va("selectedship_dialogue_branch_%i_visible", i ), false );
				}
				return;
			}
			//delay_selectedship_dialogue_branch_hostile_visible = false;
			if ( PlayerShip && SelectedEntityInSpace ) {
				for ( i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
					HailGui->SetStateBool( va("selectedship_dialogue_branch_%i_visible", i ), SelectedEntityInSpace->hailDialogueBranchTracker.test(i) );
				}
			}
		}
	}
}

void idPlayer::ResetHailGuiDialogueBranchVisibilities() {

	if ( PlayerShip && SelectedEntityInSpace ) {
		SelectedEntityInSpace->hailDialogueBranchTracker.reset();
	}
}

void idPlayer::UpdateSelectedEntityInSpaceOnGuis() {
	if ( CaptainGui ) {
		if ( PlayerShip && SelectedEntityInSpace && SelectedEntityInSpace->ShipImageVisual ) {
			CaptainGui->SetStateString( "selectedship_ship_image_visual", SelectedEntityInSpace->ShipImageVisual );
		} else {
			CaptainGui->SetStateString( "selectedship_ship_image_visual", "guis/assets/steve_captain_display/ShipImages/NoEntitySelected.tga" );
		}

		if ( SelectedEntityInSpace ) {
			if ( SelectedEntityInSpace->was_sensor_scanned )	{
				CaptainGui->SetStateString("sensor_scan_description",SelectedEntityInSpace->spawnArgs.GetString("sensor_scan_description", "Sensors scans reveal that this is an entity that exists in space.") );
			} else {
				CaptainGui->SetStateString("sensor_scan_description", "THIS ENTITY REQUIRES A TARGETED SENSOR SCAN" ); //"THIS ENTITY HAS NOT BEEN SCANNED"
			}
		} else {
			CaptainGui->SetStateString("sensor_scan_description", "NO ENTITY SELECTED" );
		}
	}
}

void idPlayer::UpdateAIDialogueGui() {
	if ( DialogueAI && AIDialogueGui ) {
		for ( int i = 0; i < MAX_DIALOGUE_BRANCHES; i++ ) {
			AIDialogueGui->SetStateBool( va("ai_dialogue_branch_%i_visible", i ), DialogueAI->AIDialogueBranchTracker.test(i) );
		}
		AIDialogueGui->SetStateString("dialogue_ai_name", DialogueAI->spawnArgs.GetString( "npc_name", "Joe" ) );
	} else if ( AIDialogueGui ) {
		AIDialogueGui->SetStateString("dialogue_ai_name", "" );
	}
}
void idPlayer::ResetAIDialogueGuiBranchVisibilities() {
	if ( DialogueAI ) {
		DialogueAI->AIDialogueBranchTracker.reset();
	}
}

void idPlayer::PostWarpThingsToDoForThePlayer() {

		UpdateWeaponsAndTorpedosQueuesOnCaptainGui(); // I'm not sure if this is necessary here. Maybe it is just for the auto-fire setting.
		UpdateModulesPowerQueueOnCaptainGui(); // I'm 99% sure this isn't necessary here.

		// boyette note - This is how you clear a list(in this case shipList).
		// boyette begin - just to be safe we will delete the highest number of possible list items - but you could just pick a really big number.
		int i;
		for (i=0; i<gameLocal.num_entities; i++) {
			CaptainGui->DeleteStateVar( va( "shipList_item_%i", i) );
		}
		// boyette end - just to be safe we will delete the highest number of possible list items.

		PopulateShipList();

		CaptainGui->SetStateInt("shipList_sel_0",0); // this is how you set the selection of a list(in this case shipList). This will select the first item in the list(1 the second, 2 the third, etc). You cannot do anything it seems directly with shiplist. e.g. 'shipList->anything' - it will bomb. If the set to value is higher than the number of items in the list it will not select anything(which is good) so it will just be NULL since that is what we set it to last - two lines above.

		if ( SelectedEntityInSpace ) SelectedEntityInSpace->currently_in_hail = false;
		SelectedEntityInSpace = NULL;
		SetSelectedEntityFromList();
		/* BOYETTE NOTE: this is now done in Event_GetATargetShipInSpace(); at the end of engage warp
		if ( SelectedEntityInSpace && SelectedEntityInSpace->IsType(sbShip::Type) && !SelectedEntityInSpace->IsType(sbStationarySpaceEntity::Type) ) {
			PlayerShip->SetTargetEntityInSpace( dynamic_cast<sbShip*>( SelectedEntityInSpace ) );
		} else if ( SelectedEntityInSpace && SelectedEntityInSpace->IsType(sbStationarySpaceEntity::Type) ) {
			PlayerShip->SetTargetEntityInSpace( dynamic_cast<sbStationarySpaceEntity*>( SelectedEntityInSpace ) );
		}
		*/
		UpdateSelectedEntityInSpaceOnGuis();
		UpdateHailGui();

		UpdateCaptainMenu();

		//CaptainGui->StateChanged( gameLocal.time, false );

		//CaptainGui->Redraw(gameLocal.realClientTime);

		// I'm pretty sure this is unnecessary since we are already doing a state change and redraw(twice) on the CaptainGUI.
		////guiOverlay->StateChanged( gameLocal.time, true );

		// I'm pretty sure this is unnecessary since we are already doing a state change and redraw(twice) on the CaptainGUI.
		////guiOverlay->Redraw(gameLocal.realClientTime);

		if ( PlayerShip ) {
			PlayerShip->stargriddestinationx = MAX_STARGRID_X_POSITIONS + 10; // to get it out of the view of on the GUI grid
			PlayerShip->stargriddestinationy = MAX_STARGRID_Y_POSITIONS + 10; // to get it out of the view of on the GUI grid
		}

		return;
}

void idPlayer::UpdateNotificationList(idStr notification) {

//idWindow*	notificationList;

// Listdefs have a limit of 1024 items. So the list can go up to item # 1023. Then we will reset it. We might shift each item up by one when the limit is reached(and delete item 0) at some point but this should be good for now.

//for ( int i = 0; i < 128; i++) {
	//idEntity* ent;
	//CaptainGui->SetStateString( va("shipList_item_%i", list_counter ), "^3" + ent->name ); // this is how you would make the text yellow - other color codes are in the idStr/Str class file.
	if ( CaptainGui ) {

		CaptainGui->SetStateString( va("notificationList_item_%i", notificationList_counter ), notification );
		CaptainGui->SetStateInt("notificationList_sel_0",notificationList_counter);
		notificationList_counter++;

		if ( notificationList_counter > 1023 ) {
			for (int ix = 512; ix <= 1023; ix++) {
				CaptainGui->SetStateString( va( "notificationList_item_%i", ix - 512 ), CaptainGui->GetStateString( va( "notificationList_item_%i", ix),"") );
			}
			for (int ix = 512; ix <= 1023; ix++) {
				CaptainGui->DeleteStateVar( va( "notificationList_item_%i", ix) );
			}
			notificationList_counter = 512;
			CaptainGui->SetStateInt("notificationList_sel_0",notificationList_counter);
		}

	/* // alternative - just clears the entire list if the limit of 1024 items is reached.
		if ( notificationList_counter >= 1023 ) {
			for (int ix = 0; ix<=1023; ix++) {
				CaptainGui->DeleteStateVar( va( "notificationList_item_%i", ix) );
			}
			notificationList_counter = 0;
		}
	*/

	//}

		if ( guiOverlay && guiOverlay == CaptainGui ) {
			CaptainGui->StateChanged( gameLocal.time, false ); // BOYETTE NOTE: this if statement is trying to solve a performance drop issue. This seems to scroll the list all the way to the bottom. But there might be a performance issue.
		}

		CaptainGui->ScrollListGUIToBottom("notificationList");
	}

//CaptainGui->StateChanged( gameLocal.time, true );
//CaptainGui->Redraw(gameLocal.realClientTime);

return;

/*
// color escape string
//#define S_COLOR_DEFAULT				"^0"
//#define S_COLOR_RED					"^1"
//#define S_COLOR_GREEN				"^2"
//#define S_COLOR_YELLOW				"^3"
//#define S_COLOR_BLUE				"^4"
//#define S_COLOR_CYAN				"^5"
//define S_COLOR_MAGENTA				"^6"
//#define S_COLOR_WHITE				"^7"
//#define S_COLOR_GRAY				"^8"
//#define S_COLOR_BLACK				"^9"
*/
}

void idPlayer::ResetNotificationList() {

	if ( CaptainGui ) {
		for (int i = 0; i <= 1023; i++) {
			CaptainGui->SetStateString( va( "notificationList_item_%i", i ), "" );
		}
	}

}

void idPlayer::DisplayNonInteractiveAlertMessage(idStr alert_message) {
	if ( gameLocal.time - last_non_interactive_alert_message_time < 3400 ) {
		non_interactive_alert_message_counter++;
		if ( non_interactive_alert_message_counter > 4 ) {
			non_interactive_alert_message_counter = 0;
		}
	} else {
		non_interactive_alert_message_counter = 0;
	}
	last_non_interactive_alert_message_time = gameLocal.time;



	if ( CaptainGui ) {
		CaptainGui->SetStateString( va( "alert_message_text_%i", non_interactive_alert_message_counter ), alert_message );
		CaptainGui->HandleNamedEvent( va( "NonInteractiveAlertMessage%i", non_interactive_alert_message_counter ) );
	}
	if ( hud ) {
		hud->SetStateString( va( "alert_message_text_%i", non_interactive_alert_message_counter ), alert_message );
		hud->HandleNamedEvent( va( "NonInteractiveAlertMessage%i", non_interactive_alert_message_counter ) );
	}
	if ( HailGui ) {
		HailGui->SetStateString( va( "alert_message_text_%i", non_interactive_alert_message_counter ), alert_message );
		HailGui->HandleNamedEvent( va( "NonInteractiveAlertMessage%i", non_interactive_alert_message_counter ) );
	}
	if ( AIDialogueGui ) {
		AIDialogueGui->SetStateString( va( "alert_message_text_%i", non_interactive_alert_message_counter ), alert_message );
		AIDialogueGui->HandleNamedEvent( va( "NonInteractiveAlertMessage%i", non_interactive_alert_message_counter ) );
	}



	// OLD:
	// these are for if we have the old notification window system on any guis - but otherwise they will not be used.
	if ( CaptainGui ) {
		CaptainGui->SetStateString( "non_interactive_alert_message" , alert_message );
		CaptainGui->HandleNamedEvent("DisplayNonInteractiveAlertMessage");
	}
	if ( hud ) {
		hud->SetStateString( "non_interactive_alert_message" , alert_message );
		hud->HandleNamedEvent("DisplayNonInteractiveAlertMessage");
	}
	if ( HailGui ) {
		HailGui->SetStateString( "non_interactive_alert_message" , alert_message );
		HailGui->HandleNamedEvent("DisplayNonInteractiveAlertMessage");
	}
	if ( AIDialogueGui ) {
		AIDialogueGui->SetStateString( "non_interactive_alert_message" , alert_message );
		AIDialogueGui->HandleNamedEvent("DisplayNonInteractiveAlertMessage");
	}

	return;
}

void idPlayer::ScheduleDisplayNonInteractiveAlertMessage( int ms_from_now, idStr alert_message ) {
	PostEventMS( &EV_Player_DisplayNonInteractiveAlertMessage, ms_from_now, alert_message.c_str() );
}
void idPlayer::Event_DisplayNonInteractiveAlertMessage( const char *alert_message ) {
	DisplayNonInteractiveAlertMessage(alert_message);
}

void idPlayer::Event_ToggleTutorialMode( int on ) {
	if ( on ) {
		tutorial_mode_on = true;

		if ( hud ) {
			hud->SetStateString( "tutorial_text" , "Press -" + idStr(common->KeysFromBinding("_impulse41")) + "- to open the captain menu." );
			hud->StateChanged(gameLocal.time,false); // this fixes those weird timing issues on the hud - the issue might be caused by sys.setcvar( "g_showHud", "0" ); in the map script - when g_showHud is disbaled the hud seems to get the timing of events wrong
			hud->HandleNamedEvent("TutorialBeginStep0");
			//hud->StateChanged(gameLocal.time,false);
			SetInfluenceLevel( INFLUENCE_NONE );
			StopSound(SND_CHANNEL_RADIO,false);
			StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_00_2" ), SND_CHANNEL_RADIO, 0, false, NULL );
		}
	} else {
		tutorial_mode_on = false;
	}
	return;
}
void idPlayer::Event_IncreaseTutorialModeStep( void ) {
	//gameLocal.Printf( "Step: " + idStr(tutorial_mode_current_step) + " | Time: " + idStr(gameLocal.time) + "\n" );
	if ( tutorial_mode_current_step == 0 ) {
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep0");
		}
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialBeginStep1");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_01to02" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 1 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep1");
			CaptainGui->HandleNamedEvent("TutorialBeginStep2");
		}
	} else if ( tutorial_mode_current_step == 2 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep2");
			CaptainGui->HandleNamedEvent("TutorialBeginStep3");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_03to04" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 3 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep3");
			CaptainGui->HandleNamedEvent("TutorialBeginStep4");
		}
	} else if ( tutorial_mode_current_step == 4 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep4");

			CaptainGui->SetStateString( "tutorial_text" , "Press -" + idStr(common->KeysFromBinding("_impulse41")) + "- or -esc- to close the captain menu." );
			CaptainGui->HandleNamedEvent("TutorialBeginStep5");
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialBeginStep5");
			hud->SetStateString( "tutorial_text" , "Sit in the captain's chair." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_05" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 5 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep5");
			CaptainGui->HandleNamedEvent("TutorialBeginStep6");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_06" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 6 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep6");
			CaptainGui->HandleNamedEvent("TutorialBeginStep7");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_07" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 7 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep7");
			CaptainGui->HandleNamedEvent("TutorialBeginStep8");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_08to09" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 8 ) {
		if ( PlayerShip && PlayerShip->WeaponsTargetQueue[0] != SHIELDSMODULEID ) {
			return;
		}
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep8");
			CaptainGui->HandleNamedEvent("TutorialBeginStep9");
		}
	} else if ( tutorial_mode_current_step == 9 ) {
		if ( PlayerShip && PlayerShip->TorpedosTargetQueue[0] != SHIELDSMODULEID ) {
			return;
		}
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep9");
			CaptainGui->HandleNamedEvent("TutorialBeginStep10");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_10" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 10 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep10");
			CaptainGui->HandleNamedEvent("TutorialBeginStep11");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_11to15" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 11 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep11");
			CaptainGui->HandleNamedEvent("TutorialBeginStep12");
		}
	} else if ( tutorial_mode_current_step == 12 ) {
		if ( CaptainGui ) {
			CaptainGui->SetStateInt( "shipList_sel_0", 1 );
			SetSelectedEntityFromList();
			UpdateSelectedEntityInSpaceOnGuis();
			CaptainGui->StateChanged(gameLocal.time,true);

			CaptainGui->HandleNamedEvent("TutorialEndStep12");
			CaptainGui->HandleNamedEvent("TutorialBeginStep13");
		}
	} else if ( tutorial_mode_current_step == 13 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep13");
			CaptainGui->HandleNamedEvent("TutorialBeginStep14");
		}
	} else if ( tutorial_mode_current_step == 14 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep14");
			CaptainGui->HandleNamedEvent("TutorialBeginStep15");
		}
	} else if ( tutorial_mode_current_step == 15 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep15");
			CaptainGui->HandleNamedEvent("TutorialBeginStep16");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_16to17" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 16 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep16");
			CaptainGui->HandleNamedEvent("TutorialBeginStep17");
		}
	} else if ( tutorial_mode_current_step == 17 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep17");
			CaptainGui->HandleNamedEvent("TutorialBeginStep18");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_18to21" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 18 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep18");
			CaptainGui->HandleNamedEvent("TutorialBeginStep19");
		}
	} else if ( tutorial_mode_current_step == 19 ) {
		if ( CaptainGui ) {
			CaptainGui->SetStateInt( "shipList_sel_0", 3 );
			SetSelectedEntityFromList();
			UpdateSelectedEntityInSpaceOnGuis();
			CaptainGui->StateChanged(gameLocal.time,true);

			CaptainGui->HandleNamedEvent("TutorialEndStep19");
			CaptainGui->HandleNamedEvent("TutorialBeginStep20");
		}
	} else if ( tutorial_mode_current_step == 20 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep20");
			CaptainGui->HandleNamedEvent("TutorialBeginStep21");
		}
	} else if ( tutorial_mode_current_step == 21 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep21");
			CaptainGui->HandleNamedEvent("TutorialBeginStep22");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_22to23" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 22 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep22");
			CaptainGui->HandleNamedEvent("TutorialBeginStep23");
		}
	} else if ( tutorial_mode_current_step == 23 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep23");
			CaptainGui->HandleNamedEvent("TutorialBeginStep24");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_24" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 24 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep24");
			CaptainGui->HandleNamedEvent("TutorialBeginStep25");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_25to26" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 25 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep25");
			CaptainGui->HandleNamedEvent("TutorialBeginStep26");
		}
	} else if ( tutorial_mode_current_step == 26 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep26");
			CaptainGui->HandleNamedEvent("TutorialBeginStep27");
			CaptainGui->SetStateString( "tutorial_text" , "Press -" + idStr(common->KeysFromBinding("_impulse41")) + "- or -esc- to close the captain menu." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_27to28" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 27 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep27");
			CaptainGui->HandleNamedEvent("TutorialBeginStep28");
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialBeginStep28");
			hud->SetStateString( "tutorial_text" , "Press -Esc- to exit the captain's chair." );
		}
	} else if ( tutorial_mode_current_step == 28 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep28");
			CaptainGui->HandleNamedEvent("TutorialBeginStep29");
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep28");
			hud->HandleNamedEvent("TutorialBeginStep29");
			hud->SetStateString( "tutorial_text" , "Go to the transporter pad." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_29" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 29 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep29");
			CaptainGui->HandleNamedEvent("TutorialBeginStep30");
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep29");
			hud->HandleNamedEvent("TutorialBeginStep30");
			hud->SetStateString( "tutorial_text" , "Activate the transporter, either from the transporter console or the captain menu." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_30" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 30 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep30");
			CaptainGui->HandleNamedEvent("TutorialBeginStep31");
			CaptainGui->SetStateString( "tutorial_text" , "Close the captain menu and pick up the materials and currency." );
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep30");
			hud->HandleNamedEvent("TutorialBeginStep31");
			hud->SetStateString( "tutorial_text" , "Pick up the materials and currency." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_31" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 31 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep31");
			CaptainGui->HandleNamedEvent("TutorialBeginStep32");
			CaptainGui->SetStateString( "tutorial_text" , "Return to the transporter pad." );
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep31");
			hud->HandleNamedEvent("TutorialBeginStep32");
			hud->SetStateString( "tutorial_text" , "Return to the transporter pad." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_32" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 32 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep32");
			CaptainGui->HandleNamedEvent("TutorialBeginStep33");
			CaptainGui->SetStateString( "tutorial_text" , "Transport back to your ship." );
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep32");
			hud->HandleNamedEvent("TutorialBeginStep33");
			hud->SetStateString( "tutorial_text" , "Transport back to your ship." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_33" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 33 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep33");
			CaptainGui->HandleNamedEvent("TutorialBeginStep34");
			CaptainGui->SetStateString( "tutorial_text" , "Go back to the bridge and sit in the captain's chair." );
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep33");
			hud->HandleNamedEvent("TutorialBeginStep34");
			hud->SetStateString( "tutorial_text" , "Go back to the bridge and sit in the captain's chair." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_34" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 34 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep34");
			CaptainGui->HandleNamedEvent("TutorialBeginStep35");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_35" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 35 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep35");
			CaptainGui->HandleNamedEvent("TutorialBeginStep36");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_36" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 36 ) {
		if ( PlayerShip && PlayerShip->crew[MEDICALCREWID] ) {
			if ( CaptainGui ) {
				CaptainGui->HandleNamedEvent("TutorialEndStep36");
				CaptainGui->HandleNamedEvent("TutorialBeginStep37");
			}
			StopSound(SND_CHANNEL_RADIO,false);
			StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_37to38" ), SND_CHANNEL_RADIO, 0, false, NULL );
		} else {
			return;
		}
	} else if ( tutorial_mode_current_step == 37 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep37");
			CaptainGui->HandleNamedEvent("TutorialBeginStep38");
		}
	} else if ( tutorial_mode_current_step == 38 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep38");
			CaptainGui->HandleNamedEvent("TutorialBeginStep39");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_39" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 39 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep39");
			CaptainGui->HandleNamedEvent("TutorialBeginStep40");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_40" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 40 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep40");
			CaptainGui->HandleNamedEvent("TutorialBeginStep41");
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_41" ), SND_CHANNEL_RADIO, 0, false, NULL );
	} else if ( tutorial_mode_current_step == 41 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep41");
			CaptainGui->HandleNamedEvent("TutorialBeginStep42");
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep41");
			hud->HandleNamedEvent("TutorialBeginStep42");
			hud->SetStateString( "tutorial_text" , "YOU COMPLETED THE TUTORIAL." );
		}
		StopSound(SND_CHANNEL_RADIO,false);
		StartSoundShader( declManager->FindSound( "space_command_misc_tutorial_dialogue_step_42" ), SND_CHANNEL_RADIO, 0, false, NULL, false );
	} else if ( tutorial_mode_current_step == 42 ) {
		if ( CaptainGui ) {
			CaptainGui->HandleNamedEvent("TutorialEndStep42");
			CaptainGui->HandleNamedEvent("TutorialBeginStep43");
		}
		if ( hud ) {
			hud->StateChanged(gameLocal.time,false);
			hud->HandleNamedEvent("TutorialEndStep42");
			hud->HandleNamedEvent("TutorialBeginStep43");
		}
	}
	tutorial_mode_current_step++;
	return;
}
void idPlayer::Event_GetTutorialModeStep( void ) {
	idThread::ReturnInt( tutorial_mode_current_step );
}
void idPlayer::Event_WaitForImpulse( int impulse_to_wait_for ) {
	if ( impulse_to_wait_for >= 0 ) {
		wait_impulse = impulse_to_wait_for;
		SetInfluenceLevel( INFLUENCE_LEVEL2 );
	}
	return;
}

// BOYETTE STEAM INTEGRATION BEGIN
void idPlayer::Event_IncreaseSteamAchievementStat( int stat_id ) {
#ifdef STEAM_BUILD
	if ( common->m_pStatsAndAchievements ) {
		switch ( stat_id )
		{
		case GET_THE_SACRIFICE_ENDING:
			common->m_pStatsAndAchievements->m_nTimesBeatGameSacrificeEnding++;
			break;
		case GET_THE_SECRET_ENDING:
			common->m_pStatsAndAchievements->m_nTimesBeatGameSuperHappyFunTimeEnding++;
			break;
		case COMPLETE_THE_TUTORIAL:
			common->m_pStatsAndAchievements->m_nTimesTutorialCompleted++;
			break;
		case WATCH_THE_INTRO:
			common->m_pStatsAndAchievements->m_nTimesBarnabyFlewIntoSpace++;
			break;
		case GO_INSIDE_THE_CRYSTAL_ENTITY:
			common->m_pStatsAndAchievements->m_nTimesCrystalEntityEntered++;
			break;
		case COMPLETE_THE_ARTIFICIAN_LABYRINTH:
			common->m_pStatsAndAchievements->m_nTimesCompletedTheArtificianLabyrinth++;
			break;
		case BEAT_THE_GAME_IN_IRONMAN_MODE:
			if ( g_inIronManMode.GetBool() ) {
				common->m_pStatsAndAchievements->m_nTimesBeatTheGameInIronmanMode++;
			}
			break;
		}
		common->StoreSteamStats();
	}
#endif
}
// BOYETTE STEAM INTEGRATION END

void idPlayer::TakeCommandOfShip(sbShip* ShipToTakeCommandOf) {
	CloseOverlayGui();
	CloseOverlayCaptainGui();
	CloseOverlayHailGui();
	CloseOverlayAIDialogeGui();
	SelectedEntityInSpace = NULL;

	// BOYETTE NOTE TODO: need to clear all module targets as well.
	if ( PlayerShip ) {
		PlayerShip->ClearCrewMemberSelection();
		PlayerShip->ClearCrewMemberMultiSelection();
		PlayerShip->ClearModuleSelection();
	}
	ShipToTakeCommandOf->ClearCrewMemberSelection();
	ShipToTakeCommandOf->ClearCrewMemberMultiSelection();
	ShipToTakeCommandOf->ClearModuleSelection();


	// BOYETTE NTOE TODO: might need to make sure that we no longer have a torpedo that is still in transit - so we might have to make an event and keep checking until there is no torpedo in transit.
	if ( PlayerShip ) {
		PlayerShip->CeaseFiringWeaponsAndTorpedos();
	}
	ShipToTakeCommandOf->CeaseFiringWeaponsAndTorpedos();

	//PlayerShip->TargetEntityInSpace = NULL; // so we don't accidentally attack our new ship if we were targeting the ship that we are now taking over.
	//ShipToTakeCommandOf->TargetEntityInSpace = NULL; // so we don't accidentally attack the ship that the ship we are taking over was attacking.
	if ( PlayerShip ) {
		PlayerShip->TargetEntityInSpace = ShipToTakeCommandOf;
	}
	ShipToTakeCommandOf->TargetEntityInSpace = PlayerShip;
	if ( PlayerShip ) {
		PlayerShip->weapons_autofire_on = false;
		PlayerShip->torpedos_autofire_on = false;
	}
	ShipToTakeCommandOf->weapons_autofire_on = false;
	ShipToTakeCommandOf->torpedos_autofire_on = false;

	if ( PlayerShip ) {
		// need to transfer the crew pointers over.
		for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
			if ( PlayerShip->crew[i] ) ShipToTakeCommandOf->crew[i] = PlayerShip->crew[i]; PlayerShip->crew[i] = NULL; // BOYETTE INTERESTING NOTE: the second statement actually always runs - doesn't get checked by the if statement. Might want to avoid this syntax. But since we will always want to make it NULL as it doesn't matter here. Need {} to seperate statements.
			SyncUpPlayerShipNameCVars();
		}

		// need to transfer the weapons queues and torpedos queues and power queues over.
		// weapons and torpedos queues
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			ShipToTakeCommandOf->WeaponsTargetQueue[i] = PlayerShip->WeaponsTargetQueue[i];
		}
		for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			ShipToTakeCommandOf->TorpedosTargetQueue[i] = PlayerShip->TorpedosTargetQueue[i];
		}
		// power automanage queue
			for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
			ShipToTakeCommandOf->ModulesPowerQueue[i] = PlayerShip->ModulesPowerQueue[i];
		}
		// transfer the target power levels on the modules for the module automanage priorities
		ShipToTakeCommandOf->ResetAutomanagePowerReserves(); // changed to this on 07 05 2016
		ShipToTakeCommandOf->current_automanage_power_reserve = ShipToTakeCommandOf->maximum_power_reserve; // changed to this on 07 05 2016
		//for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
		//	if ( ShipToTakeCommandOf->consoles[i] && ShipToTakeCommandOf->consoles[i]->ControlledModule && PlayerShip->consoles[i] && PlayerShip->consoles[i]->ControlledModule ) ShipToTakeCommandOf->consoles[i]->ControlledModule->automanage_target_power_level = PlayerShip->consoles[i]->ControlledModule->automanage_target_power_level;
		//}

		// copy the neutral teams of the previous playership to the new playership
		ShipToTakeCommandOf->neutral_teams = PlayerShip->neutral_teams; // BOYETTE NOTE TODO: verify that this persists even after the old playership is destroyed.

		// currency and materials tranfer - BOYETTE NOTE TODO: DONE: might want to give the reserves of ship we are taking over to the player.
		ShipToTakeCommandOf->current_currency_reserves = PlayerShip->current_currency_reserves + ShipToTakeCommandOf->current_currency_reserves;
		ShipToTakeCommandOf->current_materials_reserves = PlayerShip->current_materials_reserves + ShipToTakeCommandOf->current_materials_reserves;
		PlayerShip->current_currency_reserves = 0;
		PlayerShip->current_materials_reserves = 0;

		// transfer the reserve crew
		ShipToTakeCommandOf->reserve_Crew.clear();
		ShipToTakeCommandOf->reserve_Crew = PlayerShip->reserve_Crew;
		PlayerShip->reserve_Crew.clear();
	}

	//not necessary - it is better below - ShipToTakeCommandOf->ChangeTeam(team); // this changes the crew members as well who have been checked to not exist or be off on another ship. This will make them stop attacking your ship and crew. Which we might not want - might need to look into this. We might be able to just do this after we transfer the crew members - that way the original wrewmembers will keep the same team.
	// BOYETTE NOTE TODO: might want to set the team of the playership to a unique team and then add that unique team to everyone's neutral teams - could call it - BecomeNeutralShip() - or could just put all this inside of BecomeDerelict();
	//PlayerShip->BecomeDerelict(); - this is not necessary because it has no crew so after this function is done it will become derelict automatically -  // the old player ship becomes derelict. // this was a problem as well because the player would become a pirate as well - so his/her team was now different from his/her crew's team.

	//ShipToTakeCommandOf->ChangeTeam(team); // BOYETTE NOTE TODO: not sure about this: the old ship should probably still technically be friendly so your crew doesn't attack it's modules/consoles.
	
	sbShip* oldShip = NULL;
	if ( PlayerShip ) {
		oldShip = PlayerShip;
	}

	for ( int i = 0; i < ShipToTakeCommandOf->ships_at_my_stargrid_position.size(); i++ ) {
		if ( ShipToTakeCommandOf->ships_at_my_stargrid_position[i] && ShipToTakeCommandOf->ships_at_my_stargrid_position[i]->TargetEntityInSpace && ShipToTakeCommandOf->ships_at_my_stargrid_position[i]->TargetEntityInSpace == ShipToTakeCommandOf && ShipToTakeCommandOf->team != ShipToTakeCommandOf->ships_at_my_stargrid_position[i]->TargetEntityInSpace->team && !ShipToTakeCommandOf->ships_at_my_stargrid_position[i]->HasNeutralityWithShip(ShipToTakeCommandOf) ) {
			ShipToTakeCommandOf->ships_at_my_stargrid_position[i]->CeaseFiringWeaponsAndTorpedos();
			ShipToTakeCommandOf->ships_at_my_stargrid_position[i]->TempTargetEntityInSpace = NULL;
			ShipToTakeCommandOf->ships_at_my_stargrid_position[i]->TargetEntityInSpace = NULL;
		}
	}

	if ( PlayerShip ) {
		PlayerShip->UpdateGuisOnTransporterPad();
	}
	PlayerShip = ShipToTakeCommandOf;
	PlayerShip->BecomeNonDerelict();
	PlayerShip->UpdateGuisOnTransporterPad();
	
	PlayerShip->ChangeTeam(team); // this is necessary to change the teams correctly for the player.

		// if there are friendlies on board without a ParentShip and we have open crew positions, then make them part of our crew
	if ( PlayerShip ) {
		for ( int i = 0; i < PlayerShip->AIsOnBoard.size(); i++ ) {
			if ( PlayerShip->AIsOnBoard[i] && PlayerShip->AIsOnBoard[i]->team == team && !PlayerShip->AIsOnBoard[i]->ParentShip ) {
				PlayerShip->InviteAIToJoinCrew(PlayerShip->AIsOnBoard[i],false,false,false,0);
			}
		}
	}

	PlayerShip->was_sensor_scanned = true;

	if ( oldShip ) {
		oldShip->BecomeDerelict();
		oldShip->ChangeTeam(team);
	}

	if ( PlayerShip && PlayerShip->ReadyRoomCaptainChair ) {
		PlayerShip->ReadyRoomCaptainChair->PopulateCaptainLaptop();
	}

	// BOYETTE NOTE BEGIN: added this 10/08/2016
	if ( !g_allowPlayerAutoPowerMode.GetBool() ) {
		PlayerShip->modules_power_automanage_on = false;
	}
	// BOYETTE NOTE END

	// Updating the captain menue - Some of these might not be necessary - can check which ones later - this works for now.
	UpdateCaptainMenu();
	UpdateStarGridShipPositions();
	UpdateModulesPowerQueueOnCaptainGui();
	UpdateWeaponsAndTorpedosTargetingOverlays();
	UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
	UpdateCrewMemberPortraits();
	UpdateReserveCrewMemberPortraits();
	PopulateShipList();

	// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
	if ( common->m_pStatsAndAchievements ) {
		common->m_pStatsAndAchievements->m_nTimesTookCommandOfAnotherShip++;
		common->StoreSteamStats();
	}
#endif
	// BOYETTE STEAM INTEGRATION END
}

/*
==================
idPlayer::EnableWeapon
==================
*/
void idPlayer::EnableWeapon() {
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );
	weaponEnabled = true;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->ExitCinematic();
	}
}

/*
==================
idPlayer::DisableWeapon
==================
*/
void idPlayer::DisableWeapon() {
	hiddenWeapon = gameLocal.world->spawnArgs.GetBool( "no_Weapons" );
	weaponEnabled = false;
	if ( weapon.GetEntity() ) {
		weapon.GetEntity()->EnterCinematic();
	}
}

void idPlayer::UpdateCaptainMenuEveryFrame() {
	if (gameLocal.isNewFrame) { //boyette - only on newframe
		// boyette mode begin
		if ( guiOverlay && CaptainGui && guiOverlay == CaptainGui ) { // Only need to udpate the captain gui if it is opened by the player(is the guioverlay).

			UpdateDoorIconsOnShipDiagramsEveryFrame();
			UpdateCrewIconPositionsOnShipDiagrams();
			UpdateCrewMemberPortraits();
			UpdateWeaponsAndTorpedosTargetingOverlays();

			UpdateProjectileIconsOnShipDiagramsEveryFrame();

			if( PlayerShip && PlayerShip->TargetEntityInSpace ){
				CaptainGui->SetStateBool( "playership_target_entity_exists", true );
			} else {
				CaptainGui->SetStateBool( "playership_target_entity_exists", false );
			}

			if( SelectedEntityInSpace ){
				CaptainGui->SetStateBool( "playership_selected_entity_exists", true );
			} else {
				CaptainGui->SetStateBool( "playership_selected_entity_exists", false );
			}

			if ( PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->was_sensor_scanned ) {
				CaptainGui->SetStateBool( "targetship_was_sensor_scanned", true );
			} else if ( PlayerShip && !PlayerShip->TargetEntityInSpace ) {
				CaptainGui->SetStateBool( "targetship_was_sensor_scanned", true );
			} else {
				CaptainGui->SetStateBool( "targetship_was_sensor_scanned", false );
			}

			CaptainGui->SetStateFloat( "cursor_pos_x", CaptainGui->CursorX() );
			CaptainGui->SetStateFloat( "cursor_pos_y", CaptainGui->CursorY() );

			CaptainGui->UpdateTooltip();
			CaptainGui->SetStateString( "tooltip_text", CaptainGui->tooltip_text );
			CaptainGui->SetStateBool( "tooltip_visible", CaptainGui->tooltip_visible );
			CaptainGui->SetStateFloat( "tooltip_width", CaptainGui->tooltip_width );
			CaptainGui->SetStateFloat( "tooltip_x_offset", CaptainGui->tooltip_x_offset );
			CaptainGui->SetStateFloat( "tooltip_y_offset", CaptainGui->tooltip_y_offset );

			CaptainGui->SetStateBool( "tutorial_mode_on", tutorial_mode_on );
			CaptainGui->SetStateInt( "tutorial_mode_current_step", tutorial_mode_current_step );

			if ( PlayerShip && ShipOnBoard ) {

				CaptainGui->SetStateString( "playership_name", PlayerShip->name );
				CaptainGui->SetStateString( "shiponboard_name", ShipOnBoard->name );

				CaptainGui->SetStateInt( "captain_shipxdes", PlayerShip->stargriddestinationx );
				CaptainGui->SetStateInt( "captain_shipydes", PlayerShip->stargriddestinationy );

				CaptainGui->SetStateInt( "captain_shipxpos", PlayerShip->stargridpositionx );
				CaptainGui->SetStateInt( "captain_shipypos", PlayerShip->stargridpositiony );

				CaptainGui->SetStateInt( "playership_max_power_reserve", PlayerShip->maximum_power_reserve );
				CaptainGui->SetStateInt( "playership_power_reserve", PlayerShip->current_power_reserve );

				// Normal power display
				idStr power_reserve_chits;
				for ( int i = 0; i < PlayerShip->current_power_reserve; i++ ) {
					power_reserve_chits = power_reserve_chits + "- ";
				}
				CaptainGui->SetStateString( "playership_power_reserve_chits", power_reserve_chits );
				// Auto power display
				if ( PlayerShip->modules_power_automanage_on ) {
					idStr autopower_reserve_chits;
					for ( int i = 0; i < PlayerShip->current_automanage_power_reserve; i++ ) {
						autopower_reserve_chits = autopower_reserve_chits + "- ";
					}
					CaptainGui->SetStateString( "playership_autopower_reserve_chits", autopower_reserve_chits );
				}
				if ( g_allowPlayerAutoPowerMode.GetBool() ) {
					CaptainGui->SetStateBool( "playership_autopower_mode_allowed", true );
				} else {
					CaptainGui->SetStateBool( "playership_autopower_mode_allowed", false );
				}

				CaptainGui->SetStateFloat( "player_health_percentage", (float)health / (float)entity_max_health );

				if ( PlayerShip == ShipOnBoard ) {
					CaptainGui->SetStateBool( "playership_is_ship_on_board", true );
					CaptainGui->SetStateBool( "targetship_is_ship_on_board", false );
				} else if ( PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace == ShipOnBoard ) {
					CaptainGui->SetStateBool( "playership_is_ship_on_board", false );
					CaptainGui->SetStateBool( "targetship_is_ship_on_board", true );
				} else {
					CaptainGui->SetStateBool( "playership_is_ship_on_board", false );
					CaptainGui->SetStateBool( "targetship_is_ship_on_board", false );
				}

				CaptainGui->SetStateInt( "playership_max_shieldstrength", PlayerShip->max_shieldStrength );
				CaptainGui->SetStateInt( "playership_current_shieldstrength", PlayerShip->shieldStrength );
				CaptainGui->SetStateInt( "playership_max_hullstrength", PlayerShip->max_hullStrength );
				CaptainGui->SetStateInt( "playership_current_hullstrength", PlayerShip->hullStrength );
				CaptainGui->SetStateInt( "playership_structure", PlayerShip->health );
				CaptainGui->SetStateInt( "playership_max_structure", PlayerShip->entity_max_health );

				CaptainGui->SetStateFloat( "playership_current_hullstrength_percentage", (float)PlayerShip->hullStrength / (float)PlayerShip->max_hullStrength );
				CaptainGui->SetStateFloat( "playership_current_shieldstrength_percentage", (float)PlayerShip->shieldStrength / (float)PlayerShip->max_shieldStrength );

				CaptainGui->SetStateInt( "playership_current_shieldstrength_lowered", PlayerShip->shieldStrength_copy );
				CaptainGui->SetStateFloat( "playership_current_shieldstrength_percentage_lowered", (float)PlayerShip->shieldStrength_copy / (float)PlayerShip->max_shieldStrength );

				CaptainGui->SetStateInt( "playership_current_materials_reserves", PlayerShip->current_materials_reserves );
				CaptainGui->SetStateBool( "playership_in_repair_mode", PlayerShip->in_repair_mode );

				CaptainGui->SetStateInt( "playership_current_currency_reserves", PlayerShip->current_currency_reserves );

				if ( grabbed_module_exists ) {
					CaptainGui->SetStateString( "playership_grabbed_module_background", "guis/assets/steve_captain_display/" + module_description_upper[grabbed_module_id] + "ModuleIconLarge.tga" );
					CaptainGui->SetStateFloat( "playership_grabbed_module_rect_pos_x", CaptainGui->CursorX() );
					CaptainGui->SetStateFloat( "playership_grabbed_module_rect_pos_y", CaptainGui->CursorY() );

					if ( PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->consoles[grabbed_module_id] && PlayerShip->TargetEntityInSpace->consoles[grabbed_module_id]->ControlledModule ) {
						CaptainGui->SetStateInt( "playership_targetship_grabbed_module_health", PlayerShip->TargetEntityInSpace->consoles[grabbed_module_id]->ControlledModule->health );
						CaptainGui->SetStateBool( "playership_targetship_grabbed_module_really_exists", true );
						CaptainGui->SetStateFloat( "playership_targetship_grabbed_module_health_percentage", (float)PlayerShip->TargetEntityInSpace->consoles[grabbed_module_id]->ControlledModule->health / (float)PlayerShip->TargetEntityInSpace->consoles[grabbed_module_id]->ControlledModule->entity_max_health );
					} else {
						CaptainGui->SetStateInt( "playership_targetship_grabbed_module_health", 100 );
						CaptainGui->SetStateBool( "playership_targetship_grabbed_module_really_exists", false );
						CaptainGui->SetStateFloat( "playership_targetship_grabbed_module_health_percentage", 1.0f );
					}

					if ( PlayerShip->consoles[grabbed_module_id] && PlayerShip->consoles[grabbed_module_id]->ControlledModule ) {
						CaptainGui->SetStateInt( "playership_grabbed_module_health", PlayerShip->consoles[grabbed_module_id]->ControlledModule->health );
						CaptainGui->SetStateBool( "playership_grabbed_module_really_exists", true );
						CaptainGui->SetStateFloat( "playership_grabbed_module_health_percentage", (float)PlayerShip->consoles[grabbed_module_id]->ControlledModule->health / (float)PlayerShip->consoles[grabbed_module_id]->ControlledModule->entity_max_health );
					} else {
						CaptainGui->SetStateInt( "playership_grabbed_module_health", 100 );
						CaptainGui->SetStateBool( "playership_grabbed_module_really_exists", false );
						CaptainGui->SetStateFloat( "playership_grabbed_module_health_percentage", 1.0f );
					}
				} else {
					CaptainGui->SetStateInt( "playership_targetship_grabbed_module_health", 100 );
					CaptainGui->SetStateInt( "playership_grabbed_module_health", 100 );
					CaptainGui->SetStateBool( "playership_targetship_grabbed_module_really_exists", false );
					CaptainGui->SetStateBool( "playership_grabbed_module_really_exists", false );
				}
				CaptainGui->SetStateBool( "playership_grabbed_module_exists", grabbed_module_exists );

				if ( PlayerShip->was_just_damaged ) {
					CaptainGui->HandleNamedEvent("PlayerShipDamaged");
					PlayerShip->was_just_damaged = false;
				}
				if ( PlayerShip->was_just_repaired ) {
					CaptainGui->HandleNamedEvent("PlayerShipRepaired");
					PlayerShip->was_just_repaired = false;
				}

				CaptainGui->SetStateInt( "playership_current_oxygen_level", PlayerShip->current_oxygen_level );

				CaptainGui->SetStateInt( "playership_player_pos_x", PlayerShip->ReturnOnBoardEntityDiagramPositionX(this) );
				CaptainGui->SetStateInt( "playership_player_pos_y", -PlayerShip->ReturnOnBoardEntityDiagramPositionY(this) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
				CaptainGui->SetStateInt( "playership_player_pos_z", PlayerShip->ReturnOnBoardEntityDiagramPositionZ(this) );
				CaptainGui->SetStateFloat( "playership_player_angle", viewAngles.yaw - 90 );

				CaptainGui->SetStateBool( "playership_red_alert", PlayerShip->red_alert );

				CaptainGui->SetStateBool( "playership_shields_raised", PlayerShip->shields_raised );

				CaptainGui->SetStateBool( "playership_ship_lights_on", PlayerShip->ship_lights_on );

				CaptainGui->SetStateBool( "playership_self_destruct_sequence_initiated", PlayerShip->ship_self_destruct_sequence_initiated );
				if ( PlayerShip->ship_self_destruct_sequence_initiated ) {
					CaptainGui->SetStateInt( "playership_self_destruct_sequence_time_remaining", 60 - ((gameLocal.time - PlayerShip->ship_self_destruct_sequence_timer)/1000) );
					CaptainGui->SetStateFloat( "playership_self_destruct_sequence_time_remaining_percentage", 1 - (((gameLocal.time - (float)PlayerShip->ship_self_destruct_sequence_timer)/1000.0f) / (float)60.0f)  );
				} else {
					CaptainGui->SetStateFloat( "playership_self_destruct_sequence_time_remaining_percentage", 0.0f  );
				}

				CaptainGui->SetStateFloat( "playership_min_shields_percent_for_blocking_foreign_transporters", PlayerShip->min_shields_percent_for_blocking_foreign_transporters );

				if ( PlayerShip->consoles[ENGINESMODULEID] && PlayerShip->consoles[ENGINESMODULEID]->ControlledModule ) {
					float engines_efficiency = (float)PlayerShip->consoles[ENGINESMODULEID]->ControlledModule->module_efficiency/100.00f;
					float max_hull_ratio = (float)PlayerShip->max_hullStrength/(float)MAX_MAX_HULLSTRENGTH;
					float miss_chance = (engines_efficiency * 0.10f) + ((engines_efficiency * (PlayerShip->consoles[ENGINESMODULEID]->ControlledModule->max_power * 0.03) ) * (1.0f - max_hull_ratio)); // OLD BEFORE BALANCING: float miss_chance = (engines_efficiency * 0.10f) + ((engines_efficiency * 0.30f) * (1.0f - max_hull_ratio));
					miss_chance = idMath::ClampFloat( 0.0f, 0.70f, miss_chance ) * 100.0f; // NOTE: multiplied by 100 for GUI appearance
					CaptainGui->SetStateString( "playership_evasion_chance_text", "EVASION: " + idStr((int)miss_chance) + "%" );
				} else {
					CaptainGui->SetStateString( "playership_evasion_chance_text", "EVASION: 0%" );
				}

				// Crew Members
				// boyette note - these will need to be updated every frame since the crew_member entities move around alot.
				for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
					if( PlayerShip->crew[i] && !PlayerShip->crew[i]->was_killed ) {
						CaptainGui->SetStateInt( "playership_" + role_description[i] + "_officer_pos_x", PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->crew[i]) );
						CaptainGui->SetStateInt( "playership_" + role_description[i] + "_officer_pos_y", -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->crew[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						CaptainGui->SetStateInt( "playership_" + role_description[i] + "_officer_pos_z", PlayerShip->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->crew[i]) );
						CaptainGui->SetStateFloat( "playership_" + role_description[i] + "_officer_angle", PlayerShip->crew[i]->GetCurrentYaw() - 90 );
						CaptainGui->SetStateFloat( "playership_" + role_description[i] + "_officer_health_percentage", (float)PlayerShip->crew[i]->health / (float)PlayerShip->crew[i]->entity_max_health );
						CaptainGui->SetStateString( "playership_" + role_description[i] + "_officer_diagram_icon", PlayerShip->crew[i]->ship_diagram_icon );
						CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_selected", PlayerShip->crew[i]->IsSelectedCrew );
						CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_visible", true );
						CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_on_playership_visible", PlayerShip->crew[i]->ShipOnBoard && PlayerShip->crew[i]->ShipOnBoard == PlayerShip );
						if ( PlayerShip->TargetEntityInSpace && PlayerShip->crew[i]->ShipOnBoard == PlayerShip->TargetEntityInSpace ) {
							CaptainGui->SetStateInt( "playership_" + role_description[i] + "_officer_on_targetship_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->crew[i]) );
							CaptainGui->SetStateInt( "playership_" + role_description[i] + "_officer_on_targetship_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->crew[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
							CaptainGui->SetStateInt( "playership_" + role_description[i] + "_officer_on_targetship_pos_z", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->crew[i]) );
							CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_visible", false );
							CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_on_targetship_visible", true );
						} else {
							CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_on_targetship_visible", false );
						}
					} else {
						CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_visible", false );
						CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_on_targetship_visible", false );
						CaptainGui->SetStateBool( "playership_" + role_description[i] + "_officer_on_playership_visible", false );
						PlayerShip->crew[i] = NULL;
					}
				}

				// Ship Consoles
				// boyette note - eventually will only need to do this when the playership is changed
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					if (PlayerShip->consoles[i]) {
						CaptainGui->SetStateInt( "playership_" + module_description[i] + "_console_pos_x", PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->consoles[i]) );
						CaptainGui->SetStateInt( "playership_" + module_description[i] + "_console_pos_y", -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->consoles[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						CaptainGui->SetStateInt( "playership_" + module_description[i] + "_console_pos_z", PlayerShip->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->consoles[i]) );
						CaptainGui->SetStateBool( "playership_" + module_description[i] + "_console_visible", true );
					} else {
						CaptainGui->SetStateBool( "playership_" + module_description[i] + "_console_visible", false );
						CaptainGui->SetStateBool( "playership_" + module_description[i] + "_module_visible", false );
					}
				}

				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					if ( PlayerShip->consoles[i] ) {
						CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_pos_x", PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->consoles[i]->ControlledModule) );
						CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_pos_y", -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->consoles[i]->ControlledModule) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_pos_z", PlayerShip->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->consoles[i]->ControlledModule) );
						CaptainGui->SetStateInt( "playership_" + module_description[i] + "_console_health", PlayerShip->consoles[i]->health );
						CaptainGui->SetStateFloat( "playership_" + module_description[i] + "_console_health_percentage", (float)PlayerShip->consoles[i]->health / (float)PlayerShip->consoles[i]->entity_max_health );

						if ( PlayerShip->consoles[i]->ControlledModule ) {
							CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_health", PlayerShip->consoles[i]->ControlledModule->health );
							CaptainGui->SetStateFloat( "playership_" + module_description[i] + "_module_health_percentage", (float)PlayerShip->consoles[i]->ControlledModule->health / (float)PlayerShip->consoles[i]->ControlledModule->entity_max_health );
							CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_efficiency", PlayerShip->consoles[i]->ControlledModule->module_efficiency );
							CaptainGui->SetStateFloat( "playership_" + module_description[i] + "_module_efficiency_percentage", (float)PlayerShip->consoles[i]->ControlledModule->module_efficiency / 100.0f );
							CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_buffed_amount", PlayerShip->consoles[i]->ControlledModule->module_buffed_amount );
							CaptainGui->SetStateFloat( "playership_" + module_description[i] + "_module_buffed_amount_modifier", PlayerShip->consoles[i]->ControlledModule->module_buffed_amount_modifier );
							CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_power_allocated", PlayerShip->consoles[i]->ControlledModule->power_allocated );
							CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_max_power", PlayerShip->consoles[i]->ControlledModule->max_power );
							CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_max_health", PlayerShip->consoles[i]->ControlledModule->entity_max_health );
							CaptainGui->SetStateInt( "playership_" + module_description[i] + "_module_damage_adjusted_max_power", PlayerShip->consoles[i]->ControlledModule->damage_adjusted_max_power );
							CaptainGui->SetStateFloat( "playership_" + module_description[i] + "_module_current_charge_percentage", PlayerShip->consoles[i]->ControlledModule->current_charge_percentage );
							CaptainGui->SetStateFloat( "playership_" + module_description[i] + "_module_current_charge_percentage_int", int(PlayerShip->consoles[i]->ControlledModule->current_charge_percentage * 100) );
							// NOT NECESSARY - THIS IS DONE ON MATERIALS NOW // CaptainGui->SetStateString( "playership_" + module_description[i] + "_module_current_charge_animation", idStr("guis/assets/steve_captain_display/CircularModuleChargeBar/100_images_circle/CircularChargeBar_") + idStr( int(PlayerShip->consoles[i]->ControlledModule->current_charge_percentage * 100) ) );
							CaptainGui->SetStateBool( "playership_" + module_description[i] + "_module_visible", true );
							if ( PlayerShip->consoles[i]->ControlledModule->was_just_damaged ) {
								CaptainGui->HandleNamedEvent("PlayerShip" + module_description_upper[i] + "ModuleDamaged");
								PlayerShip->consoles[i]->ControlledModule->was_just_damaged = false;
							}
							if ( PlayerShip->consoles[i]->ControlledModule->was_just_repaired ) {
								CaptainGui->HandleNamedEvent("PlayerShip" + module_description_upper[i] + "ModuleRepaired");
								PlayerShip->consoles[i]->ControlledModule->was_just_repaired = false;
							}
						} else {
							CaptainGui->SetStateBool( "playership_" + module_description[i] + "_module_visible", false );
						}
					}
				}

				for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
					if ( PlayerShip->room_node[i] ) {
						CaptainGui->SetStateInt( "playership_" + room_description[i] + "_room_node_pos_x", PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->room_node[i]) );
						CaptainGui->SetStateInt( "playership_" + room_description[i] + "_room_node_pos_y", -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->room_node[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						CaptainGui->SetStateInt( "playership_" + room_description[i] + "_room_node_pos_z", PlayerShip->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->room_node[i]) );
						CaptainGui->SetStateBool( "playership_" + room_description[i] + "_room_node_visible", true );
					} else {
						CaptainGui->SetStateBool( "playership_" + room_description[i] + "_room_node_visible", false );
					}
				}

				if ( PlayerShip->TransporterBounds ) {
					CaptainGui->SetStateInt( "playership_transporter_module_pos_x", PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TransporterBounds) );
					CaptainGui->SetStateInt( "playership_transporter_module_pos_y", -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TransporterBounds) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
					CaptainGui->SetStateInt( "playership_transporter_module_pos_z", PlayerShip->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->TransporterBounds) );
					CaptainGui->SetStateBool( "playership_transporter_module_visible", true );
				} else {
					CaptainGui->SetStateBool( "playership_transporter_module_visible", false );
				}

				if ( PlayerShip->CaptainChair ) {
					CaptainGui->SetStateInt( "playership_captain_chair_pos_x", PlayerShip->ReturnOnBoardEntityDiagramPositionX(PlayerShip->CaptainChair) );
					CaptainGui->SetStateInt( "playership_captain_chair_pos_y", -PlayerShip->ReturnOnBoardEntityDiagramPositionY(PlayerShip->CaptainChair) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
					CaptainGui->SetStateInt( "playership_captain_chair_pos_z", PlayerShip->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->CaptainChair) );
					CaptainGui->SetStateBool( "playership_captain_chair_visible", true );
					if ( PlayerShip->CaptainChair->SeatedEntity.GetEntity() || (PlayerShip->ReadyRoomCaptainChair && PlayerShip->ReadyRoomCaptainChair->SeatedEntity.GetEntity()) ) {
						CaptainGui->SetStateBool( "playership_captain_chair_has_seated_entity", true );
					} else {
						CaptainGui->SetStateBool( "playership_captain_chair_has_seated_entity", false );
					}

					if ( PlayerShip->CaptainChair->SeatedEntity.GetEntity() ) {
						CaptainGui->SetStateBool( "playership_captain_chair_only_has_seated_entity", true );
					} else {
						CaptainGui->SetStateBool( "playership_captain_chair_only_has_seated_entity", false );
					}
				} else {
					CaptainGui->SetStateBool( "playership_captain_chair_visible", false );
					CaptainGui->SetStateBool( "playership_captain_chair_has_seated_entity", false );
				}

				// TargetShip Entities
				if ( PlayerShip->TargetEntityInSpace ) {
					CaptainGui->SetStateString( "captain_target", PlayerShip->TargetEntityInSpace->name );
					CaptainGui->SetStateInt( "captain_number", PlayerShip->TargetEntityInSpace->captaintestnumber );
					CaptainGui->SetStateInt( "targetship_max_shieldstrength", PlayerShip->TargetEntityInSpace->max_shieldStrength );
					CaptainGui->SetStateInt( "targetship_current_shieldstrength", PlayerShip->TargetEntityInSpace->shieldStrength );
					CaptainGui->SetStateInt( "targetship_max_hullstrength", PlayerShip->TargetEntityInSpace->max_hullStrength );
					CaptainGui->SetStateInt( "targetship_current_hullstrength", PlayerShip->TargetEntityInSpace->hullStrength );
					CaptainGui->SetStateInt( "targetship_structure", PlayerShip->TargetEntityInSpace->health );
					CaptainGui->SetStateInt( "targetship_max_structure", PlayerShip->TargetEntityInSpace->entity_max_health );

					CaptainGui->SetStateFloat( "targetship_current_hullstrength_percentage", (float)PlayerShip->TargetEntityInSpace->hullStrength / (float)PlayerShip->TargetEntityInSpace->max_hullStrength );
					CaptainGui->SetStateFloat( "targetship_current_shieldstrength_percentage", (float)PlayerShip->TargetEntityInSpace->shieldStrength / (float)PlayerShip->TargetEntityInSpace->max_shieldStrength );

					CaptainGui->SetStateInt( "targetship_max_power_reserve", PlayerShip->TargetEntityInSpace->maximum_power_reserve );
					CaptainGui->SetStateInt( "targetship_power_reserve", PlayerShip->TargetEntityInSpace->current_power_reserve );
					CaptainGui->SetStateInt( "targetship_current_oxygen_level", PlayerShip->TargetEntityInSpace->current_oxygen_level );
					CaptainGui->SetStateBool( "targetship_red_alert", PlayerShip->TargetEntityInSpace->red_alert );

					CaptainGui->SetStateBool( "targetship_shields_raised", PlayerShip->TargetEntityInSpace->shields_raised );

					CaptainGui->SetStateFloat( "targetship_min_shields_percent_for_blocking_foreign_transporters", PlayerShip->TargetEntityInSpace->min_shields_percent_for_blocking_foreign_transporters );

					//CaptainGui->SetStateBool( "targetship_can_be_deconstructed", PlayerShip->TargetEntityInSpace->can_be_deconstructed && PlayerShip->TargetEntityInSpace->is_derelict && PlayerShip->TargetEntityInSpace->IsType( sbShip::Type ) && !PlayerShip->TargetEntityInSpace->IsType( sbStationarySpaceEntity::Type ) && !PlayerShip->TargetEntityInSpace->ship_deconstruction_sequence_initiated && !PlayerShip->ship_beam_active && ShipOnBoard != PlayerShip->TargetEntityInSpace );
					CaptainGui->SetStateBool( "targetship_can_be_deconstructed", PlayerShip->TargetEntityInSpace->can_be_deconstructed && ( (PlayerShip->TargetEntityInSpace->is_derelict && PlayerShip->TargetEntityInSpace->IsType( sbShip::Type )) || PlayerShip->TargetEntityInSpace->IsType( sbStationarySpaceEntity::Type ) ) && !PlayerShip->TargetEntityInSpace->ship_deconstruction_sequence_initiated && !PlayerShip->ship_beam_active && ShipOnBoard != PlayerShip->TargetEntityInSpace );

					if ( PlayerShip->TargetEntityInSpace->consoles[ENGINESMODULEID] && PlayerShip->TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule ) {
						float engines_efficiency = (float)PlayerShip->TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule->module_efficiency/100.00f;
						float max_hull_ratio = (float)PlayerShip->TargetEntityInSpace->max_hullStrength/(float)MAX_MAX_HULLSTRENGTH;
						float miss_chance = (engines_efficiency * 0.10f) + ((engines_efficiency * (PlayerShip->TargetEntityInSpace->consoles[ENGINESMODULEID]->ControlledModule->max_power * 0.03) ) * (1.0f - max_hull_ratio)); // OLD BEFORE BALANCING: float miss_chance = (engines_efficiency * 0.10f) + ((engines_efficiency * 0.30f) * (1.0f - max_hull_ratio));
						miss_chance = idMath::ClampFloat( 0.0f, 0.70f, miss_chance ) * 100.0f; // NOTE: multiplied by 100 for GUI appearance
						if ( PlayerShip->consoles[SENSORSMODULEID] && PlayerShip->consoles[SENSORSMODULEID]->ControlledModule && PlayerShip->consoles[SENSORSMODULEID]->ControlledModule->module_efficiency > 25 ) {
							CaptainGui->SetStateString( "targetship_evasion_chance_text", "EVASION: " + idStr((int)miss_chance) + "%" );
						} else {
							CaptainGui->SetStateString( "targetship_evasion_chance_text", "EVASION: ???%" );
						}
					} else {
						CaptainGui->SetStateString( "targetship_evasion_chance_text", "EVASION: 0%" );
					}

					if ( PlayerShip->TargetEntityInSpace->was_just_damaged ) {
						CaptainGui->HandleNamedEvent("TargetShipDamaged");
						PlayerShip->TargetEntityInSpace->was_just_damaged = false;
					}
					if ( PlayerShip->TargetEntityInSpace->was_just_repaired ) {
						CaptainGui->HandleNamedEvent("TargetShipRepaired");
						PlayerShip->TargetEntityInSpace->was_just_repaired = false;
					}

					CaptainGui->SetStateInt( "targetship_player_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(this) );
					CaptainGui->SetStateInt( "targetship_player_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(this) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
					CaptainGui->SetStateInt( "targetship_player_pos_z", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionZ(this) );
					CaptainGui->SetStateFloat( "targetship_player_angle", viewAngles.yaw - 90 );

					// TargetShip Crew Members
					// boyette note - these will need to be updated every frame since the crew_member entities move around alot.
					for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
						if( PlayerShip->TargetEntityInSpace->crew[i] && !PlayerShip->TargetEntityInSpace->crew[i]->was_killed ) {
							CaptainGui->SetStateInt( "targetship_" + role_description[i] + "_officer_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->crew[i]) );
							CaptainGui->SetStateInt( "targetship_" + role_description[i] + "_officer_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->crew[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
							CaptainGui->SetStateInt( "targetship_" + role_description[i] + "_officer_pos_z", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->TargetEntityInSpace->crew[i]) );
							CaptainGui->SetStateFloat( "targetship_" + role_description[i] + "_officer_angle", PlayerShip->TargetEntityInSpace->crew[i]->GetCurrentYaw() - 90 );
							CaptainGui->SetStateFloat( "targetship_" + role_description[i] + "_officer_health_percentage", (float)PlayerShip->TargetEntityInSpace->crew[i]->health / (float)PlayerShip->TargetEntityInSpace->crew[i]->entity_max_health );
							CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_visible", true );
							if ( !PlayerShip->TargetEntityInSpace->crew[i]) {
								CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_visible", false);
							}
						} else {
							CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_visible", false );
							PlayerShip->TargetEntityInSpace->crew[i] = NULL;
						}
					}

					// TargetShip Consoles
					// boyette note - eventually will only need to do this when the targetship is changed
					for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
						if (PlayerShip->TargetEntityInSpace->consoles[i]) {
							CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_console_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->consoles[i]) );
							CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_console_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->consoles[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
							CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_console_pos_z", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->TargetEntityInSpace->consoles[i]) );
							CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_console_visible", true );
						} else {
							CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_console_visible", false );
							CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_visible", false );
						}
					}

					// TargetShip Modules
					// boyette note - eventually will only need to do this when the targetship is changed
					for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
						if ( PlayerShip->TargetEntityInSpace->consoles[i] ) { // this If statement is necessary - the game will bomb if consoles[i] is not specified correctly in the defs and is NULL. C++ apparently doesn't know that ControlledModule is NULL if MedicalConsole is NULL - so we have to manually check it here. I will look into this.
							CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule) );
							CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
							CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_pos_z", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule) );
							CaptainGui->SetStateFloat( "targetship_" + module_description[i] + "_console_health_percentage", (float)PlayerShip->TargetEntityInSpace->consoles[i]->health / (float)PlayerShip->TargetEntityInSpace->consoles[i]->entity_max_health );
							if ( PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule ) {
								CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_health", PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->health );
								CaptainGui->SetStateFloat( "targetship_" + module_description[i] + "_module_health_percentage", (float)PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->health / (float)PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->entity_max_health );
								CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_max_health", PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->entity_max_health );
								CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_efficiency", PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->module_efficiency );
								CaptainGui->SetStateFloat( "targetship_" + module_description[i] + "_module_efficiency_percentage", (float)PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->module_efficiency / 100.0f );
								CaptainGui->SetStateInt( "targetship_" + module_description[i] + "_module_power_allocated", PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->power_allocated );
								CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_visible", true );
								if ( PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->was_just_damaged ) {
									CaptainGui->HandleNamedEvent("TargetShip" + module_description_upper[i] + "ModuleDamaged");
									PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->was_just_damaged = false;
								}
								if ( PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->was_just_repaired ) {
									CaptainGui->HandleNamedEvent("TargetShip" + module_description_upper[i] + "ModuleRepaired");
									PlayerShip->TargetEntityInSpace->consoles[i]->ControlledModule->was_just_repaired = false;
								}
							} else {
								CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_visible", false );
							}
						}
					}

					for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
						if ( PlayerShip->TargetEntityInSpace->room_node[i] ) {
							CaptainGui->SetStateInt( "targetship_" + room_description[i] + "_room_node_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->room_node[i]) );
							CaptainGui->SetStateInt( "targetship_" + room_description[i] + "_room_node_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->room_node[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
							CaptainGui->SetStateInt( "targetship_" + room_description[i] + "_room_node_pos_z", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->TargetEntityInSpace->room_node[i]) );
							CaptainGui->SetStateBool( "targetship_" + room_description[i] + "_room_node_visible", true );
						} else {
							CaptainGui->SetStateBool( "targetship_" + room_description[i] + "_room_node_visible", false );
						}
					}

					if ( PlayerShip->TargetEntityInSpace->TransporterBounds ) { // this If statement is necessary - the game will bomb if MedicalConsole is not specified correctly in the defs and is NULL. C++ apparently doesn't know that ControlledModule is NULL if MedicalConsole is NULL - so we have to manually check it here. I will look into this.
						CaptainGui->SetStateInt( "targetship_transporter_module_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->TransporterBounds) );
						CaptainGui->SetStateInt( "targetship_transporter_module_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->TransporterBounds) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						CaptainGui->SetStateInt( "targetship_transporter_module_pos_z", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->TargetEntityInSpace->TransporterBounds) );
						CaptainGui->SetStateBool( "targetship_transporter_module_visible", true );
					} else {
						CaptainGui->SetStateBool( "targetship_transporter_module_visible", false );
					}

					if ( PlayerShip->TargetEntityInSpace->CaptainChair ) { // this If statement is necessary - the game will bomb if MedicalConsole is not specified correctly in the defs and is NULL. C++ apparently doesn't know that ControlledModule is NULL if MedicalConsole is NULL - so we have to manually check it here. I will look into this.
						CaptainGui->SetStateInt( "targetship_captain_chair_pos_x", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionX(PlayerShip->TargetEntityInSpace->CaptainChair) );
						CaptainGui->SetStateInt( "targetship_captain_chair_pos_y", -PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionY(PlayerShip->TargetEntityInSpace->CaptainChair) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						CaptainGui->SetStateInt( "targetship_captain_chair_pos_z", PlayerShip->TargetEntityInSpace->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->TargetEntityInSpace->CaptainChair) );
						CaptainGui->SetStateBool( "targetship_captain_chair_visible", true );
						if ( PlayerShip->TargetEntityInSpace->CaptainChair->SeatedEntity.GetEntity() || (PlayerShip->TargetEntityInSpace->ReadyRoomCaptainChair && PlayerShip->TargetEntityInSpace->ReadyRoomCaptainChair->SeatedEntity.GetEntity()) ) {
							CaptainGui->SetStateBool( "targetship_captain_chair_has_seated_entity", true );
						} else {
							CaptainGui->SetStateBool( "targetship_captain_chair_has_seated_entity", false );
						}
					} else {
						CaptainGui->SetStateBool( "targetship_captain_chair_visible", false );
						CaptainGui->SetStateBool( "targetship_captain_chair_has_seated_entity", false );
					}
				} else {
					for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
						CaptainGui->SetStateBool( "targetship_" + role_description[i] + "_officer_visible", false );
					}

					CaptainGui->SetStateBool( "targetship_captain_chair_visible", false );

					for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
						CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_console_visible", false );
						CaptainGui->SetStateBool( "targetship_" + module_description[i] + "_module_visible", false );
					}

					for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
						CaptainGui->SetStateBool( "targetship_" + room_description[i] + "_room_node_visible", false );
					}

					CaptainGui->SetStateBool( "targetship_transporter_module_visible", false );

					CaptainGui->SetStateBool( "targetship_diagram_playership_weapons_target_overlay_visible", false );
					CaptainGui->SetStateBool( "targetship_diagram_playership_torpedos_target_overlay_visible", false );
					CaptainGui->SetStateFloat( "targetship_current_hullstrength_percentage", 0 );
					CaptainGui->SetStateFloat( "targetship_current_shieldstrength_percentage", 0 );

					CaptainGui->SetStateBool( "targetship_can_be_deconstructed", false );
				}
			}
		}
	}
}

void idPlayer::UpdateCaptainHudEveryFrame() {
	// BOYETTE NOTE TODO: have an update captain hud not every frame function - set ship name colors on the hud whenever a ship is targetted.
	if ( gameLocal.isNewFrame ) { //boyette - only on newframe
		if ( !guiOverlay && hud ) { // Only need to udpate the captain hud if there is not a guiOverlay).

			hud->SetStateBool( "tutorial_mode_on", tutorial_mode_on );
			hud->SetStateInt( "tutorial_mode_current_step", tutorial_mode_current_step );

			if( PlayerShip ){
				hud->SetStateBool( "playership_exists", true );

				if( PlayerShip && PlayerShip->TargetEntityInSpace ){
					hud->SetStateBool( "playership_target_entity_exists", true );
				} else {
					hud->SetStateBool( "playership_target_entity_exists", false );
				}
				hud->SetStateInt( "playership_current_materials_reserves", PlayerShip->current_materials_reserves );
				hud->SetStateInt( "playership_current_currency_reserves", PlayerShip->current_currency_reserves );
			} else {
				hud->SetStateBool( "playership_exists", false );
				hud->SetStateBool( "playership_target_entity_exists", false );
				hud->SetStateInt( "playership_current_materials_reserves", 0 );
				hud->SetStateInt( "playership_current_currency_reserves", 0 );
				hud->SetStateBool( "playership_self_destruct_sequence_initiated", false );
			}
			if( ShipOnBoard ){
				hud->SetStateBool( "shiponboard_exists", true );

				if( ( PlayerShip  && PlayerShip != ShipOnBoard && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace != ShipOnBoard ) || ( PlayerShip && !PlayerShip->TargetEntityInSpace && PlayerShip != ShipOnBoard ) || ( !PlayerShip ) ){
					hud->SetStateBool( "shiponboard_bars_visible", true );
				} else {
					hud->SetStateBool( "shiponboard_bars_visible", false );
				}
			} else {
				hud->SetStateBool( "shiponboard_bars_visible", false );
				hud->SetStateBool( "shiponboard_exists", false );
			}

			if ( PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->was_sensor_scanned ) {
				hud->SetStateBool( "targetship_was_sensor_scanned", true );
			} else if ( PlayerShip && !PlayerShip->TargetEntityInSpace ) {
				hud->SetStateBool( "targetship_was_sensor_scanned", true );
			} else {
				hud->SetStateBool( "targetship_was_sensor_scanned", false );
			}

			if ( PlayerShip && ShipOnBoard ) {

				hud->SetStateString( "playership_name", "^4" + PlayerShip->name );
				hud->SetStateString( "playership_name_with_colon", "^4" + PlayerShip->name + ":" );
				hud->SetStateString( "playership_name_with_colon_no_color", PlayerShip->original_name + ":" );

				hud->SetStateInt( "playership_max_shieldstrength", PlayerShip->max_shieldStrength );
				hud->SetStateInt( "playership_current_shieldstrength", PlayerShip->shieldStrength );
				hud->SetStateInt( "playership_max_hullstrength", PlayerShip->max_hullStrength );
				hud->SetStateInt( "playership_current_hullstrength", PlayerShip->hullStrength );
				hud->SetStateFloat( "playership_current_hullstrength_percentage", (float)PlayerShip->hullStrength / (float)PlayerShip->max_hullStrength );

				if ( PlayerShip->consoles[ENGINESMODULEID] && PlayerShip->consoles[ENGINESMODULEID]->ControlledModule ) {
					hud->SetStateFloat( "playership_" + module_description[ENGINESMODULEID] + "_module_current_charge_percentage", PlayerShip->consoles[ENGINESMODULEID]->ControlledModule->current_charge_percentage );
					hud->SetStateFloat( "playership_" + module_description[ENGINESMODULEID] + "_module_current_charge_percentage_int", int(PlayerShip->consoles[ENGINESMODULEID]->ControlledModule->current_charge_percentage * 100) );
					hud->SetStateFloat( "playership_" + module_description[ENGINESMODULEID] + "_module_health_percentage", (float)PlayerShip->consoles[ENGINESMODULEID]->ControlledModule->health / (float)PlayerShip->consoles[ENGINESMODULEID]->ControlledModule->entity_max_health );
					hud->SetStateBool( "playership_" + module_description[ENGINESMODULEID] + "_module_visible", true );
				} else {
					hud->SetStateBool( "playership_" + module_description[ENGINESMODULEID] + "_module_visible", false );
				}

				if ( PlayerShip->max_hullStrength > PlayerShip->max_shieldStrength ) {
					hud->SetStateInt( "playership_greater_of_max_hull_or_max_shields", PlayerShip->max_hullStrength );
				} else {
					hud->SetStateInt( "playership_greater_of_max_hull_or_max_shields", PlayerShip->max_shieldStrength );
				}

				if ( PlayerShip == ShipOnBoard ) {
					hud->SetStateBool( "playership_is_ship_on_board", true );
					hud->SetStateBool( "targetship_is_ship_on_board", false );
				} else if ( PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace == ShipOnBoard ) {
					hud->SetStateBool( "playership_is_ship_on_board", false );
					hud->SetStateBool( "targetship_is_ship_on_board", true );
				} else {
					hud->SetStateBool( "playership_is_ship_on_board", false );
					hud->SetStateBool( "targetship_is_ship_on_board", false );
				}

				hud->SetStateBool( "playership_shields_raised", PlayerShip->shields_raised );
				if ( !PlayerShip->shields_raised ) {
					hud->SetStateInt( "playership_current_shieldstrength_lowered", PlayerShip->shieldStrength_copy );
				}

				if ( PlayerShip->was_just_damaged ) {
					hud->HandleNamedEvent("PlayerShipDamaged");
					PlayerShip->was_just_damaged = false;
				}
				if ( PlayerShip->was_just_repaired ) {
					hud->HandleNamedEvent("PlayerShipRepaired");
					PlayerShip->was_just_repaired = false;
				}

				hud->SetStateFloat( "playership_min_shields_percent_for_blocking_foreign_transporters", PlayerShip->min_shields_percent_for_blocking_foreign_transporters );

				hud->SetStateBool( "playership_self_destruct_sequence_initiated", PlayerShip->ship_self_destruct_sequence_initiated );
				if ( PlayerShip->ship_self_destruct_sequence_initiated ) {
					hud->SetStateInt( "playership_self_destruct_sequence_time_remaining", 60 - ((gameLocal.time - PlayerShip->ship_self_destruct_sequence_timer)/1000) );
				}

				if ( PlayerShip->consoles[SENSORSMODULEID] ) {
					if ( PlayerShip->consoles[SENSORSMODULEID]->ControlledModule ) {
						hud->SetStateInt( "playership_sensors_module_efficiency", PlayerShip->consoles[SENSORSMODULEID]->ControlledModule->module_efficiency );
					} else {
						hud->SetStateInt( "playership_sensors_module_efficiency", 0 ); // for static and limiting information
					}
				} else {
					hud->SetStateInt( "playership_sensors_module_efficiency", 0 ); // for static and limiting information
				}






				hud->SetStateBool( "shiponboard_hud_map_visible", hud_map_visible );
				if ( hud_map_visible ) {

					hud->SetStateInt( "shiponboard_player_pos_x", ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(this) );
					hud->SetStateInt( "shiponboard_player_pos_y", -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(this) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
					hud->SetStateInt( "shiponboard_player_pos_z", ShipOnBoard->ReturnOnBoardEntityDiagramPositionZ(this) );

					hud->SetStateFloat( "shiponboard_player_angle", viewAngles.yaw - 90 );
				}
				UpdateCrewIconPositionsOnHudShipDiagram();
				UpdateDoorIconsOnHudShipDiagramEveryFrame();
				UpdateProjectileIconsOnHudEveryFrame();

				// Ship Consoles
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					if (ShipOnBoard->consoles[i]) {
						hud->SetStateInt( "shiponboard_" + module_description[i] + "_console_pos_x", ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(ShipOnBoard->consoles[i]) );
						hud->SetStateInt( "shiponboard_" + module_description[i] + "_console_pos_y", -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(ShipOnBoard->consoles[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						hud->SetStateInt( "shiponboard_" + module_description[i] + "_console_pos_z", ShipOnBoard->ReturnOnBoardEntityDiagramPositionZ(ShipOnBoard->consoles[i]) );
						//hud->SetStateFloat( "shiponboard_" + module_description[i] + "_console_angle", ShipOnBoard->consoles[i]->GetPhysics()->GetAxis().ToAngles().Normalize180().yaw - 90.0f );
						hud->SetStateBool( "shiponboard_" + module_description[i] + "_console_visible", true );
					} else {
						hud->SetStateBool( "shiponboard_" + module_description[i] + "_console_visible", false );
						hud->SetStateBool( "shiponboard_" + module_description[i] + "_module_visible", false );
					}
				}
				for ( int i = 0; i < MAX_MODULES_ON_SHIPS; i++ ) {
					if ( ShipOnBoard->consoles[i] ) {
						hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_pos_x", ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(ShipOnBoard->consoles[i]->ControlledModule) );
						hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_pos_y", -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(ShipOnBoard->consoles[i]->ControlledModule) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_pos_z", ShipOnBoard->ReturnOnBoardEntityDiagramPositionZ(ShipOnBoard->consoles[i]->ControlledModule) );
						hud->SetStateInt( "shiponboard_" + module_description[i] + "_console_health", ShipOnBoard->consoles[i]->health );
						hud->SetStateFloat( "shiponboard_" + module_description[i] + "_console_health_percentage", (float)ShipOnBoard->consoles[i]->health / (float)ShipOnBoard->consoles[i]->entity_max_health );

						if ( ShipOnBoard->consoles[i]->ControlledModule ) {
							hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_health", ShipOnBoard->consoles[i]->ControlledModule->health );
							hud->SetStateFloat( "shiponboard_" + module_description[i] + "_module_health_percentage", (float)ShipOnBoard->consoles[i]->ControlledModule->health / (float)ShipOnBoard->consoles[i]->ControlledModule->entity_max_health );
							hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_efficiency", ShipOnBoard->consoles[i]->ControlledModule->module_efficiency );
							hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_buffed_amount", ShipOnBoard->consoles[i]->ControlledModule->module_buffed_amount );
							hud->SetStateFloat( "shiponboard_" + module_description[i] + "_module_buffed_amount_modifier", ShipOnBoard->consoles[i]->ControlledModule->module_buffed_amount_modifier );
							hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_power_allocated", ShipOnBoard->consoles[i]->ControlledModule->power_allocated );
							hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_max_power", ShipOnBoard->consoles[i]->ControlledModule->max_power );
							hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_max_health", ShipOnBoard->consoles[i]->ControlledModule->entity_max_health );
							hud->SetStateInt( "shiponboard_" + module_description[i] + "_module_damage_adjusted_max_power", ShipOnBoard->consoles[i]->ControlledModule->damage_adjusted_max_power );
							hud->SetStateFloat( "shiponboard_" + module_description[i] + "_module_current_charge_percentage", ShipOnBoard->consoles[i]->ControlledModule->current_charge_percentage );
							hud->SetStateFloat( "shiponboard_" + module_description[i] + "_module_current_charge_percentage_int", int(ShipOnBoard->consoles[i]->ControlledModule->current_charge_percentage * 100) );
							// NOT NECESSARY - THIS IS DONE ON MATERIALS NOW // CaptainGui->SetStateString( "playership_" + module_description[i] + "_module_current_charge_animation", idStr("guis/assets/steve_captain_display/CircularModuleChargeBar/100_images_circle/CircularChargeBar_") + idStr( int(PlayerShip->consoles[i]->ControlledModule->current_charge_percentage * 100) ) );hud->SetStateString( "shiponboard_" + module_description[i] + "_module_current_charge_animation", idStr("guis/assets/steve_captain_display/CircularModuleChargeBar/100_images_circle/CircularChargeBar_") + idStr( int(ShipOnBoard->consoles[i]->ControlledModule->current_charge_percentage * 100) ) );
							hud->SetStateBool( "shiponboard_" + module_description[i] + "_module_visible", true );
							if ( ShipOnBoard->consoles[i]->ControlledModule->was_just_damaged ) {
								hud->HandleNamedEvent("ShipOnBoard" + module_description_upper[i] + "ModuleDamaged");
								ShipOnBoard->consoles[i]->ControlledModule->was_just_damaged = false;
							}
							if ( ShipOnBoard->consoles[i]->ControlledModule->was_just_repaired ) {
								hud->HandleNamedEvent("ShipOnBoard" + module_description_upper[i] + "ModuleRepaired");
								ShipOnBoard->consoles[i]->ControlledModule->was_just_repaired = false;
							}
						} else {
							hud->SetStateBool( "shiponboard_" + module_description[i] + "_module_visible", false );
						}
					}
				}

				for ( int i = 0; i < MAX_ROOMS_ON_SHIPS; i++ ) {
					if ( ShipOnBoard->room_node[i] ) {
						hud->SetStateInt( "shiponboard_" + room_description[i] + "_room_node_pos_x", ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(ShipOnBoard->room_node[i]) );
						hud->SetStateInt( "shiponboard_" + room_description[i] + "_room_node_pos_y", -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(ShipOnBoard->room_node[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
						hud->SetStateInt( "shiponboard_" + room_description[i] + "_room_node_pos_z", ShipOnBoard->ReturnOnBoardEntityDiagramPositionZ(ShipOnBoard->room_node[i]) );
						hud->SetStateBool( "shiponboard_" + room_description[i] + "_room_node_visible", true );
					} else {
						hud->SetStateBool( "shiponboard_" + room_description[i] + "_room_node_visible", false );
					}
				}

				// MISC

				if ( ShipOnBoard->TransporterBounds ) {
					hud->SetStateInt( "shiponboard_transporter_module_pos_x", ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(ShipOnBoard->TransporterBounds) );
					hud->SetStateInt( "shiponboard_transporter_module_pos_y", -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(ShipOnBoard->TransporterBounds) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
					hud->SetStateInt( "shiponboard_transporter_module_pos_z", ShipOnBoard->ReturnOnBoardEntityDiagramPositionZ(ShipOnBoard->TransporterBounds) );
					hud->SetStateBool( "shiponboard_transporter_module_visible", true );
				} else {
					hud->SetStateBool( "shiponboard_transporter_module_visible", false );
				}

				if ( ShipOnBoard->CaptainChair ) {
					hud->SetStateInt( "shiponboard_captain_chair_pos_x", ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(ShipOnBoard->CaptainChair) );
					hud->SetStateInt( "shiponboard_captain_chair_pos_y", -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(ShipOnBoard->CaptainChair) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
					hud->SetStateInt( "shiponboard_captain_chair_pos_z", ShipOnBoard->ReturnOnBoardEntityDiagramPositionZ(ShipOnBoard->CaptainChair) );
					hud->SetStateBool( "shiponboard_captain_chair_visible", true );
					if ( ShipOnBoard->CaptainChair->SeatedEntity.GetEntity() || (ShipOnBoard->ReadyRoomCaptainChair && ShipOnBoard->ReadyRoomCaptainChair->SeatedEntity.GetEntity()) ) {
						CaptainGui->SetStateBool( "shiponboard_captain_chair_has_seated_entity", true );
					} else {
						CaptainGui->SetStateBool( "shiponboard_captain_chair_has_seated_entity", false );
					}
				} else {
					hud->SetStateBool( "shiponboard_captain_chair_visible", false );
					CaptainGui->SetStateBool( "shiponboard_captain_chair_has_seated_entity", false );
				}








				if ( ShipOnBoard->team == team ) {
					hud->SetStateString( "shiponboard_name", "^4" + ShipOnBoard->original_name );
					hud->SetStateString( "shiponboard_name_with_colon", "^4" + ShipOnBoard->original_name + ":" );

					hud->SetStateBool( "shiponboard_is_friendly", true );
					hud->SetStateBool( "shiponboard_is_neutral", false );
					hud->SetStateBool( "shiponboard_is_hostile", false );
				} else if ( PlayerShip->HasNeutralityWithShip(ShipOnBoard) ) {
					hud->SetStateString( "shiponboard_name", "^8" + ShipOnBoard->original_name );
					hud->SetStateString( "shiponboard_name_with_colon", "^8" + ShipOnBoard->original_name + ":" );

					hud->SetStateBool( "shiponboard_is_friendly", false );
					hud->SetStateBool( "shiponboard_is_neutral", true );
					hud->SetStateBool( "shiponboard_is_hostile", false );
				} else {
					hud->SetStateString( "shiponboard_name", "^1" + ShipOnBoard->original_name );
					hud->SetStateString( "shiponboard_name_with_colon", "^1" + ShipOnBoard->original_name + ":" );

					hud->SetStateBool( "shiponboard_is_friendly", false );
					hud->SetStateBool( "shiponboard_is_neutral", false );
					hud->SetStateBool( "shiponboard_is_hostile", true );
				}
				hud->SetStateString( "shiponboard_name_with_colon_no_color", ShipOnBoard->original_name + ":" );

				hud->SetStateInt( "shiponboard_current_oxygen_level", ShipOnBoard->current_oxygen_level );

				hud->SetStateInt( "shiponboard_max_shieldstrength", ShipOnBoard->max_shieldStrength );
				hud->SetStateInt( "shiponboard_current_shieldstrength", ShipOnBoard->shieldStrength );
				hud->SetStateInt( "shiponboard_max_hullstrength", ShipOnBoard->max_hullStrength );
				hud->SetStateInt( "shiponboard_current_hullstrength", ShipOnBoard->hullStrength );
				hud->SetStateFloat( "shiponboard_current_hullstrength_percentage", (float)ShipOnBoard->hullStrength / (float)ShipOnBoard->max_hullStrength );

				if ( ShipOnBoard->max_hullStrength > ShipOnBoard->max_shieldStrength ) {
					hud->SetStateInt( "shiponboard_greater_of_max_hull_or_max_shields", ShipOnBoard->max_hullStrength );
				} else {
					hud->SetStateInt( "shiponboard_greater_of_max_hull_or_max_shields", ShipOnBoard->max_shieldStrength );
				}

				hud->SetStateFloat( "shiponboard_min_shields_percent_for_blocking_foreign_transporters", ShipOnBoard->min_shields_percent_for_blocking_foreign_transporters );

				hud->SetStateBool( "shiponboard_shields_raised", ShipOnBoard->shields_raised );

				if ( PlayerShip->TargetEntityInSpace ) {
					if ( PlayerShip->TargetEntityInSpace->team == team ) {
						hud->SetStateString( "targetship_name", "^4" + PlayerShip->TargetEntityInSpace->original_name );
						hud->SetStateString( "targetship_name_with_colon", "^4" + PlayerShip->TargetEntityInSpace->original_name + ":" );

						hud->SetStateBool( "targetship_is_friendly", true );
						hud->SetStateBool( "targetship_is_neutral", false );
						hud->SetStateBool( "targetship_is_hostile", false );
					} else if ( PlayerShip->HasNeutralityWithShip(PlayerShip->TargetEntityInSpace) ) {
						hud->SetStateString( "targetship_name", "^8" + PlayerShip->TargetEntityInSpace->original_name );
						hud->SetStateString( "targetship_name_with_colon", "^8" + PlayerShip->TargetEntityInSpace->original_name + ":" );

						hud->SetStateBool( "targetship_is_friendly", false );
						hud->SetStateBool( "targetship_is_neutral", true );
						hud->SetStateBool( "targetship_is_hostile", false );
					} else {
						hud->SetStateString( "targetship_name", "^1" + PlayerShip->TargetEntityInSpace->original_name );
						hud->SetStateString( "targetship_name_with_colon", "^1" + PlayerShip->TargetEntityInSpace->original_name + ":" );

						hud->SetStateBool( "targetship_is_friendly", false );
						hud->SetStateBool( "targetship_is_neutral", false );
						hud->SetStateBool( "targetship_is_hostile", true );
					}
					hud->SetStateString( "targetship_name_with_colon_no_color", PlayerShip->TargetEntityInSpace->original_name + ":" );

					hud->SetStateInt( "targetship_max_shieldstrength", PlayerShip->TargetEntityInSpace->max_shieldStrength );
					hud->SetStateInt( "targetship_current_shieldstrength", PlayerShip->TargetEntityInSpace->shieldStrength );
					hud->SetStateInt( "targetship_max_hullstrength", PlayerShip->TargetEntityInSpace->max_hullStrength );
					hud->SetStateInt( "targetship_current_hullstrength", PlayerShip->TargetEntityInSpace->hullStrength );
					hud->SetStateFloat( "targetship_current_hullstrength_percentage", (float)PlayerShip->TargetEntityInSpace->hullStrength / (float)PlayerShip->TargetEntityInSpace->max_hullStrength );

					if ( PlayerShip->TargetEntityInSpace->max_hullStrength > PlayerShip->TargetEntityInSpace->max_shieldStrength ) {
						hud->SetStateInt( "targetship_greater_of_max_hull_or_max_shields", PlayerShip->TargetEntityInSpace->max_hullStrength );
					} else {
						hud->SetStateInt( "targetship_greater_of_max_hull_or_max_shields", PlayerShip->TargetEntityInSpace->max_shieldStrength );
					}

					hud->SetStateBool( "targetship_shields_raised", PlayerShip->TargetEntityInSpace->shields_raised );

					if ( PlayerShip->TargetEntityInSpace->was_just_damaged ) {
						hud->HandleNamedEvent("TargetShipDamaged");
						PlayerShip->TargetEntityInSpace->was_just_damaged = false;
					}
					if ( PlayerShip->TargetEntityInSpace->was_just_repaired ) {
						hud->HandleNamedEvent("TargetShipRepaired");
						PlayerShip->TargetEntityInSpace->was_just_repaired = false;
					}

					hud->SetStateFloat( "targetship_min_shields_percent_for_blocking_foreign_transporters", PlayerShip->TargetEntityInSpace->min_shields_percent_for_blocking_foreign_transporters );
				}
			} else {
				hud->SetStateBool( "shiponboard_hud_map_visible", false );
			}
		}
	}
}

void idPlayer::UpdateCrewIconPositionsOnHudShipDiagram() {
	if ( hud ) {
		idEntity	*ent;
		int i;
		int list_counter = 0;
		idAI* CrewMemberToUpdate;

		if (ShipOnBoard) {
			for ( i = 0; i < ShipOnBoard->AIsOnBoard.size() ; i++ ) {
				if ( ShipOnBoard->AIsOnBoard[i] && ShipOnBoard->AIsOnBoard[i]->ShipOnBoard && ShipOnBoard->AIsOnBoard[i]->ShipOnBoard == ShipOnBoard && !ShipOnBoard->AIsOnBoard[i]->was_killed && ( !PlayerShip || ( (ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[MEDICALCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[ENGINESCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[WEAPONSCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[TORPEDOSCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[SHIELDSCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[SENSORSCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[ENVIRONMENTCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[COMPUTERCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[SECURITYCREWID] && ShipOnBoard->AIsOnBoard[i] != PlayerShip->crew[CAPTAINCREWID]) ) ) ) {
					hud->SetStateInt( va("shiponboard_misc_crew_%i_pos_x", list_counter ), ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(ShipOnBoard->AIsOnBoard[i]) );
					hud->SetStateInt( va("shiponboard_misc_crew_%i_pos_y", list_counter ), -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(ShipOnBoard->AIsOnBoard[i]) );
					hud->SetStateFloat( va("shiponboard_misc_crew_%i_angle", list_counter ), ShipOnBoard->AIsOnBoard[i]->GetCurrentYaw() - 90 );
					hud->SetStateFloat( va("shiponboard_misc_crew_%i_health_percentage", list_counter ), (float)ShipOnBoard->AIsOnBoard[i]->health / (float)ShipOnBoard->AIsOnBoard[i]->entity_max_health );
					hud->SetStateString( va("shiponboard_misc_crew_%i_diagram_icon", list_counter ), ShipOnBoard->AIsOnBoard[i]->ship_diagram_icon );
					hud->SetStateBool( va("shiponboard_misc_crew_%i_visible", list_counter ), true );
					if ( ShipOnBoard->AIsOnBoard[i]->team != team && PlayerShip && PlayerShip->HasNeutralityWithAI(ShipOnBoard->AIsOnBoard[i]) ) {
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_red", list_counter ), 0.4f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_green", list_counter ), 0.4f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_blue", list_counter ), 0.4f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
					} else if ( ShipOnBoard->AIsOnBoard[i]->team != team ) {
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_red", list_counter ), 0.75f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_green", list_counter ), 0.09f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_blue", list_counter ), 0.09f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
					} else if ( ShipOnBoard->AIsOnBoard[i]->team == team ) {
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_red", list_counter ), 0.2f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_green", list_counter ), 0.7f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_blue", list_counter ), 1.0f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
					} else {
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_red", list_counter ), 0.4f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_green", list_counter ), 0.4f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_blue", list_counter ), 0.4f );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_matcolor_alpha", list_counter ), 1.0f );
					}
					list_counter++;
				}
			}
			/*
			for ( i = 0; i < gameLocal.num_entities ; i++ ) {
				ent = gameLocal.entities[ i ];
				if ( ent && ent->IsType( idAI::Type ) && !ent->IsType( idPlayer::Type ) ) {
					CrewMemberToUpdate = dynamic_cast<idAI*>(ent);
					if (CrewMemberToUpdate->ShipOnBoard == ShipOnBoard && !CrewMemberToUpdate->was_killed && (CrewMemberToUpdate != PlayerShip->crew[MEDICALCREWID] && CrewMemberToUpdate != PlayerShip->crew[ENGINESCREWID] && CrewMemberToUpdate != PlayerShip->crew[WEAPONSCREWID] && CrewMemberToUpdate != PlayerShip->crew[TORPEDOSCREWID] && CrewMemberToUpdate != PlayerShip->crew[SHIELDSCREWID] && CrewMemberToUpdate != PlayerShip->crew[SENSORSCREWID] && CrewMemberToUpdate != PlayerShip->crew[ENVIRONMENTCREWID] && CrewMemberToUpdate != PlayerShip->crew[COMPUTERCREWID] && CrewMemberToUpdate != PlayerShip->crew[SECURITYCREWID] && CrewMemberToUpdate != PlayerShip->crew[CAPTAINCREWID]) ) {
						hud->SetStateInt( va("shiponboard_misc_crew_%i_pos_x", list_counter ), ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(CrewMemberToUpdate) );
						hud->SetStateInt( va("shiponboard_misc_crew_%i_pos_y", list_counter ), -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(CrewMemberToUpdate) );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_angle", list_counter ), CrewMemberToUpdate->GetCurrentYaw() - 90 );
						hud->SetStateFloat( va("shiponboard_misc_crew_%i_health_percentage", list_counter ), (float)CrewMemberToUpdate->health / (float)CrewMemberToUpdate->entity_max_health );
						hud->SetStateString( va("shiponboard_misc_crew_%i_diagram_icon", list_counter ), CrewMemberToUpdate->ship_diagram_icon );
						hud->SetStateBool( va("shiponboard_misc_crew_%i_visible", list_counter ), true );
						list_counter++;
					}
				}
			}
			*/
			// clear out the rest.
			for ( i = list_counter; i < 21 ; i++ ) { // There will not be more than 20 misc crew on the shiponboard.
				hud->SetStateBool( va("shiponboard_misc_crew_%i_visible", i ), false );
			}
		}

		if ( PlayerShip && ShipOnBoard ) {
			for ( int i = 0; i < MAX_CREW_ON_SHIPS; i++ ) {
				if( PlayerShip->crew[i] && !PlayerShip->crew[i]->was_killed && PlayerShip->crew[i]->ShipOnBoard && PlayerShip->crew[i]->ShipOnBoard == ShipOnBoard ) {
					hud->SetStateInt( "playership_" + role_description[i] + "_officer_pos_x", ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(PlayerShip->crew[i]) );
					hud->SetStateInt( "playership_" + role_description[i] + "_officer_pos_y", -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(PlayerShip->crew[i]) ); // boyette - notice the negative here - this is because the gui Y coordinate system is reversed.
					hud->SetStateInt( "playership_" + role_description[i] + "_officer_pos_z", ShipOnBoard->ReturnOnBoardEntityDiagramPositionZ(PlayerShip->crew[i]) );
					hud->SetStateFloat( "playership_" + role_description[i] + "_officer_angle", PlayerShip->crew[i]->GetCurrentYaw() - 90 );
					hud->SetStateFloat( "playership_" + role_description[i] + "_officer_health_percentage", (float)PlayerShip->crew[i]->health / (float)PlayerShip->crew[i]->entity_max_health );
					hud->SetStateString( "playership_" + role_description[i] + "_officer_diagram_icon", PlayerShip->crew[i]->ship_diagram_icon );
					hud->SetStateBool( "playership_" + role_description[i] + "_officer_selected", PlayerShip->crew[i]->IsSelectedCrew );
					hud->SetStateBool( "playership_" + role_description[i] + "_officer_visible", true );
				} else {
					hud->SetStateBool( "playership_" + role_description[i] + "_officer_visible", false );
				}
				if ( PlayerShip->crew[i] && !PlayerShip->crew[i]->was_killed ) {
				} else {
					PlayerShip->crew[i] = NULL;
				}
			}
		}
	}
	return;
}

void idPlayer::UpdateDoorIconsOnHudShipDiagramEveryFrame() {
	if ( hud ) {
		int i;
		int ix;

		if (ShipOnBoard) {
			for( i = 0; i < ShipOnBoard->shipdoors.Num(); i++ ) {
				if ( ShipOnBoard->shipdoors[ i ].GetEntity() ) {
					hud->SetStateFloat( va("shiponboard_door_%i_health_percentage", i ), (float)ShipOnBoard->shipdoors[ i ].GetEntity()->health / (float)ShipOnBoard->shipdoors[ i ].GetEntity()->entity_max_health );
					hud->SetStateBool( va("shiponboard_door_%i_is_secure", i ), ShipOnBoard->shipdoors[ i ].GetEntity()->health > 0 );
					if ( ShipOnBoard->shipdoors[ i ].GetEntity()->was_just_damaged ) {
						hud->HandleNamedEvent(va("ShipOnBoardDoor_%i_Damaged", i ));
						ShipOnBoard->shipdoors[ i ].GetEntity()->was_just_damaged = false;
					}
					if ( ShipOnBoard->shipdoors[ i ].GetEntity()->IsType(idDoor::Type) ) {
						if ( dynamic_cast<idDoor*>(ShipOnBoard->shipdoors[ i ].GetEntity())->IsOpen() ) {
							hud->SetStateBool( va("shiponboard_door_%i_is_open", i ), true );
						} else {
							hud->SetStateBool( va("shiponboard_door_%i_is_open", i ), false );
						}	
					}
				}
			}
		}
		// BOYETTE NOTE TODO - THIS MIGHT BE NECESSARY -  WE WILL HAVE TO PUT A FUNCTION HERE TO CLEAR OUT ANY OTHER EXTRA SHIP DOORS THAT ARE STILL VISIBLE AND SET THEM TO VISIBLE FALSE.
	}
	return;
}

void idPlayer::UpdateDoorIconsOnHudShipDiagramOnNewTarget() {
	int i;

	if ( ShipOnBoard && hud ) {
		for( i = 0; i < ShipOnBoard->shipdoors.Num(); i++ ) {
			hud->SetStateInt( va("shiponboard_door_%i_pos_x", i ), ShipOnBoard->ReturnOnBoardEntityDiagramPositionX(ShipOnBoard->shipdoors[ i ].GetEntity()) );
			hud->SetStateInt( va("shiponboard_door_%i_pos_y", i ), -ShipOnBoard->ReturnOnBoardEntityDiagramPositionY(ShipOnBoard->shipdoors[ i ].GetEntity()) );
			//hud->SetStateFloat( va("shiponboard_door_%i_angle", i ), ShipOnBoard->shipdoors[ i ].GetEntity()->GetPhysics()->GetAxis().ToAngles().Normalize180().yaw - 90.0f );
			hud->SetStateBool( va("shiponboard_door_%i_visible", i ), true );
		}
		for ( i; i < MAX_SHIP_DOOR_ENTITIES; i++ ) {
				hud->SetStateBool( va("shiponboard_door_%i_visible", i ), false );
		}
	}
	// DONE: BOYETTE NOTE TODO - THIS MIGHT BE NECESSARY -  WE WILL HAVE TO PUT A FUNCTION HERE TO CLEAR OUT ANY OTHER EXTRA SHIP DOORS THAT ARE STILL VISIBLE AND SET THEM TO VISIBLE FALSE.
	return;
}

void idPlayer::UpdateCaptainHudOnce() {
	if ( hud ) {
		if ( ShipOnBoard ) {
			idStr ShipOnBoardDiagramMaterialName;
			ShipOnBoardDiagramMaterialName = ShipOnBoard->spawnArgs.GetString( "ship_diagram_material", "textures/images_used_in_source/default_ship_diagram_material.tga" );
			if ( hud ) {
				hud->SetStateString("shiponboard_diagram_material",ShipOnBoardDiagramMaterialName);
				declManager->FindMaterial(ShipOnBoardDiagramMaterialName)->SetSort(SS_GUI);
			}

			if ( hud ) {
				hud->SetStateBool( "playership_" + module_description[SHIELDSMODULEID] + "_module_visible", PlayerShip && PlayerShip->consoles[SHIELDSMODULEID] && PlayerShip->consoles[SHIELDSMODULEID]->ControlledModule );
				hud->SetStateBool( "targetship_" + module_description[SHIELDSMODULEID] + "_module_visible", PlayerShip && PlayerShip->TargetEntityInSpace && PlayerShip->TargetEntityInSpace->consoles[SHIELDSMODULEID] && PlayerShip->TargetEntityInSpace->consoles[SHIELDSMODULEID]->ControlledModule );
				hud->SetStateBool( "shiponboard_" + module_description[SHIELDSMODULEID] + "_module_visible", ShipOnBoard && ShipOnBoard->consoles[SHIELDSMODULEID] && ShipOnBoard->consoles[SHIELDSMODULEID]->ControlledModule );
			}

			UpdateDoorIconsOnHudShipDiagramOnNewTarget();
		}
	}
}

/*
==================
idPlayer::ScheduleStopWarpEffects
==================
*/
void idPlayer::ScheduleStopWarpEffects( int ms_from_now ) {
	PostEventMS( &EV_Player_StopWarpEffects, ms_from_now );
}

/*
==================
idPlayer::ScheduleStopSlowMo
==================
*/
void idPlayer::ScheduleStopSlowMo( int ms_from_now ) {
	PostEventMS( &EV_Player_StopSlowMo, ms_from_now );
}

/*
==================
idPlayer::TransitionNumBloomPassesToZero
==================
*/
void idPlayer::TransitionNumBloomPassesToZero() {
	CancelEvents( &EV_Player_TransitionNumBloomPassesToZero );
	CancelEvents( &EV_Player_TransitionNumBloomPassesToThirty );
	if ( g_testBloomNumPasses.GetInteger() > 0 ) {
		g_testBloomNumPasses.SetInteger( g_testBloomNumPasses.GetInteger() - 1 );
	}
	if ( g_testBloomNumPasses.GetInteger() > 0 ) {
		PostEventMS( &EV_Player_TransitionNumBloomPassesToZero, 20 );
	} else {
		bloomEnabled = false;
	}
}

/*
==================
idPlayer::TransitionNumBloomPassesToThirty
==================
*/
void idPlayer::TransitionNumBloomPassesToThirty() {
	CancelEvents( &EV_Player_TransitionNumBloomPassesToThirty );
	CancelEvents( &EV_Player_TransitionNumBloomPassesToZero );
	if ( g_testBloomNumPasses.GetInteger() < 30 ) {
		g_testBloomNumPasses.SetInteger( g_testBloomNumPasses.GetInteger() + 1 );
	}
	if ( g_testBloomNumPasses.GetInteger() < 30) {
		PostEventMS( &EV_Player_TransitionNumBloomPassesToThirty, 20 );
	}
}

/*
==================
idPlayer::EnableBlur
==================
*/
void idPlayer::EnableBlur() {
	blurEnabled = true;
}
/*
==================
idPlayer::DisableBlur
==================
*/
void idPlayer::DisableBlur() {
	blurEnabled = false;
}

/*
==================
idPlayer::EnableDesaturate
==================
*/
void idPlayer::EnableDesaturate() {
	desaturateEnabled = true;
}
/*
==================
idPlayer::DisableDesaturate
==================
*/
void idPlayer::DisableDesaturate() {
	desaturateEnabled = false;
}

// BOYETTE MUSIC BEGIN
/*
==================
idPlayer::BeginMonitoringMusic
==================
*/
void idPlayer::BeginMonitoringMusic() {
	CancelEvents( &EV_Player_MonitorMusic );
	PostEventMS( &EV_Player_MonitorMusic, 100 );
}
/*
==================
idPlayer::Event_MonitorMusic
==================
*/
void idPlayer::Event_MonitorMusic( void ) {
	if ( music_shader_is_playing && gameLocal.time > currently_playing_music_shader_end_time ) {
		EndMusic();
	} else {
		PostEventMS( &EV_Player_MonitorMusic, 100 );
	}
}

/*
==================
idPlayer::EndMusic
==================
*/
void idPlayer::EndMusic( void ) {
	StopSound(SND_CHANNEL_MUSIC,false);
	gameSoundWorld->FadeSoundClasses(3,0.0f,5.0f); // return the alarms sounds back to normal after music is done playing
	music_shader_is_playing = false;
	currently_playing_music_shader.Empty();
	gameLocal.Printf( "RETURNING ALARMS VOLUME TO NORMAL\n" );
}

/*
==================
idPlayer::Event_CancelMonitorMusic
==================
*/
void idPlayer::Event_CancelMonitorMusic( void ) {
	CancelEvents( &EV_Player_MonitorMusic );
}

/*
==================
idPlayer::DeterminePlayerMusic
==================
*/
void idPlayer::DeterminePlayerMusic() {
	gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_MUSIC,false);
	if ( s_in_game_music_volume.GetFloat() < -35.0f ) {
		music_shader_is_playing = false;
		gameSoundWorld->FadeSoundClasses(2,-100.0f,1.5f); // music
		//gameSoundWorld->FadeSoundClasses(3,0.0f,1.5f); // alarms
		return;
	}
	// SONG SELECTION BEGIN
	if ( PlayerShip && ShipOnBoard && PlayerShip == ShipOnBoard ) {
		if ( PlayerShip->ships_at_my_stargrid_position.size() == 0 ) {
			currently_playing_music_shader = "default_music_long";
		} else if ( PlayerShip->ships_at_my_stargrid_position.size() == 1 && PlayerShip->ships_at_my_stargrid_position[0]->is_derelict ) {
			currently_playing_music_shader = "derelict_music_long";
		} else {

			bool hostile_entities_present = false;
			for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size(); i++ ) {
				if ( !PlayerShip->ships_at_my_stargrid_position[i]->is_derelict && PlayerShip->ships_at_my_stargrid_position[i]->team != team && !PlayerShip->ships_at_my_stargrid_position[i]->HasNeutralityWithTeam(team) ) {
					hostile_entities_present = true;
					break;
				}
			}

			if ( hostile_entities_present ) {
				std::vector<idStr>		potential_songs_to_play;
				int						most_common_song_count = 0;
				idStr					most_common_song;
				for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size(); i++ ) {
					if ( !PlayerShip->ships_at_my_stargrid_position[i]->is_derelict && PlayerShip->ships_at_my_stargrid_position[i]->team != team && !PlayerShip->ships_at_my_stargrid_position[i]->HasNeutralityWithTeam(team) ) {
						potential_songs_to_play.push_back(PlayerShip->ships_at_my_stargrid_position[i]->spawnArgs.GetString( "snd_ship_music_hostile", "default_music_long" ));
					}
				}
				for ( int i = 0; i < potential_songs_to_play.size(); i++ ) {
					int new_count = 0;
					new_count = std::count(potential_songs_to_play.begin(), potential_songs_to_play.end(), potential_songs_to_play[i]);
					if ( most_common_song_count < new_count && new_count > 1) {
						most_common_song_count = new_count;
						most_common_song = potential_songs_to_play[i];
					}
				}
				gameLocal.Printf( "\n" + most_common_song + "\n" );
				if ( most_common_song.IsEmpty() ) {
					currently_playing_music_shader = potential_songs_to_play[gameLocal.random.RandomInt(potential_songs_to_play.size()-1)];
				} else {
					currently_playing_music_shader = most_common_song;
				}
			} else {
				std::vector<idStr>		potential_songs_to_play;
				int						most_common_song_count = 0;
				idStr					most_common_song;
				for ( int i = 0; i < PlayerShip->ships_at_my_stargrid_position.size(); i++ ) {
					if ( PlayerShip->ships_at_my_stargrid_position[i]->is_derelict ) {
						potential_songs_to_play.push_back("derelict_music_long");
					} else {
						potential_songs_to_play.push_back(PlayerShip->ships_at_my_stargrid_position[i]->spawnArgs.GetString( "snd_ship_music_friendly", "default_music_long" ));
					}
				}
				for ( int i = 0; i < potential_songs_to_play.size(); i++ ) {
					int new_count = 0;
					new_count = std::count(potential_songs_to_play.begin(), potential_songs_to_play.end(), potential_songs_to_play[i]);
					if ( most_common_song_count < new_count && new_count > 1) {
						most_common_song_count = new_count;
						most_common_song = potential_songs_to_play[i];
					}
				}
				if ( most_common_song.IsEmpty() ) {
					gameLocal.GetLocalPlayer()->currently_playing_music_shader = potential_songs_to_play[gameLocal.random.RandomInt(potential_songs_to_play.size()-1)];
				} else {
					gameLocal.GetLocalPlayer()->currently_playing_music_shader = most_common_song;
				}
			}
		}
		// SONG SELECTION END
		int song_length = 0;
		StartSoundShader(declManager->FindSound(currently_playing_music_shader), SND_CHANNEL_MUSIC, 0, false, &song_length, false ); // channel defaults to -1 which shouldn't be important because we use soundclasses instead of channels for everything but maybe possibly it is
		currently_playing_music_shader_begin_time = gameLocal.time;
		currently_playing_music_shader_end_time = gameLocal.time + song_length;
		music_shader_is_playing = true;
		BeginMonitoringMusic();
		gameSoundWorld->FadeSoundClasses(2,s_in_game_music_volume.GetFloat(),5.0f); // BOYETTE NOTE: music sounds are soundclass 2. 0.0f db is the default level.
		gameSoundWorld->FadeSoundClasses(3,-17.0f,0.0f); // fade the alarm sounds down while music is playing
	}
}
// BOYETTE MUSIC END


// BOYETTE WEIRD TEXTURE THRASHING FIX BEGIN
/*
==================
idPlayer::StartWeirdTextureThrashFix
==================
*/
void idPlayer::StartWeirdTextureThrashFix( const idVec3& original_org, int frames_to_wait ) {
	//BOYETTE NOTE: commented this out 07 05 2016 because it might have been part of the reason we were having memory crashes  //
	if ( !cvarSystem->GetCVarInteger( "image_downSize" ) ) {
		cvarSystem->SetCVarBool( "r_forceLoadImages", true, CVAR_ARCHIVE ); // I believe this offloads some texture memory onto the GPU so that the game can use more than 4 gb worth of memory even though it is only 32 bit - but this understanding might not be accurate - but this works for now - will need to reduce most 1024x1024 images to 512x512 later so this is not necessary
	}
	//common->FlushOpenGL(); // this is a good idea - otherwise opengl is more likely to run out of memory and crash - actually I don't think this does anything here - but maybe

	currently_doing_weird_texture_thrash_fix = true;
	before_thrash_fix_origin = original_org;
	before_thrash_fix_view_angles = idVec3( viewAngles[0], viewAngles[1], viewAngles[2] );

	noclip = true;
	SetOrigin(idVec3(-177777,0,0));
	SetAngles( idAngles(0,0,0) );
	SetViewAngles( idAngles(0,0,0) );

	thrash_fix_frames_to_wait = frames_to_wait;
	thrash_fix_frame_begin = gameLocal.framenum;
	CancelEvents( &EV_Player_UpdateWeirdTextureThrashFix );
	PostEventMS( &EV_Player_UpdateWeirdTextureThrashFix, 1 );
}
/*
==================
idPlayer::Event_UpdateWeirdTextureThrashFix
==================
*/
void idPlayer::Event_UpdateWeirdTextureThrashFix( void ) {
	if ( (gameLocal.framenum - thrash_fix_frame_begin) > thrash_fix_frames_to_wait ) {
		noclip = false;
		SetOrigin(before_thrash_fix_origin);
		SetAngles( idAngles(before_thrash_fix_view_angles) );
		SetViewAngles( idAngles(before_thrash_fix_view_angles) );
		currently_doing_weird_texture_thrash_fix = false;
	} else {
		CancelEvents( &EV_Player_UpdateWeirdTextureThrashFix );
		PostEventMS( &EV_Player_UpdateWeirdTextureThrashFix, 1 );
	}
}
// BOYETTE WEIRD TEXTURE THRASHING FIX END


// BOYETTE OVERLAY GUI SCRIPT BEGIN
/*
==================
idPlayer::Event_OpenOverlayGUI
==================
*/
void idPlayer::Event_OpenOverlayGUI( const char *overlayGUI ) {
	SetOverlayGui(overlayGUI);
}
// BOYETTE OVERLAY GUI SCRIPT END

// BOYETTE OUTRO INVENTORY ITEM SCRIPTS BEGIN

/*
==================
idPlayer::Event_WeaponAvailable
==================
*/
void idPlayer::Event_HasInventoryItem( const char* item_name ) {
	if ( FindInventoryItem( item_name ) ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}
// BOYETTE OUTRO INVENTORY ITEM SCRIPTS END

// boyette mod end
