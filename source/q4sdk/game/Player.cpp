// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 09/30/2004

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#include "ai/AI.h"
#include "ai/AI_Manager.h"
#include "Weapon.h"
#include "Projectile.h"
#include "vehicle/Vehicle.h"
#include "client/ClientModel.h"
#include "ai/AAS_tactical.h"
#include "Healing_Station.h"
#include "ai/AI_Medic.h"

// RAVEN BEGIN
// nrausch: support for turning the weapon change ui on and off
#ifdef _XENON
#include "../ui/Window.h"

// nrausch: support for direct button input
#include "../sys/xenon/xen_input.h"
#endif
// RAVEN END

idCVar net_predictionErrorDecay( "net_predictionErrorDecay", "112", CVAR_FLOAT | CVAR_GAME | CVAR_NOCHEAT, "time in milliseconds it takes to fade away prediction errors", 0.0f, 200.0f );
idCVar net_showPredictionError( "net_showPredictionError", "-1", CVAR_INTEGER | CVAR_GAME | CVAR_NOCHEAT, "show prediction errors for the given client", -1, MAX_CLIENTS );


/*
===============================================================================

	Player control.
	This object handles all player movement and world interaction.

===============================================================================
*/

#ifdef _XENON
bool g_ObjectiveSystemOpen = false;
#endif

// distance between ladder rungs (actually is half that distance, but this sounds better)
const int LADDER_RUNG_DISTANCE = 32;

// amount of health per dose from the health station
const int HEALTH_PER_DOSE = 10;

// time before a weapon dropped to the floor disappears
const int WEAPON_DROP_TIME = 20 * 1000;

// time before a next or prev weapon switch happens
const int	WEAPON_SWITCH_DELAY		= 150;

const float	PLAYER_ITEM_DROP_SPEED	= 100.0f;

// how many units to raise spectator above default view height so it's in the head of someone
const int SPECTATE_RAISE = 25;

const int	HEALTH_PULSE		= 1000;			// Regen rate and heal leak rate (for health > 100)
const int	ARMOR_PULSE			= 1000;			// armor ticking down due to being higher than maxarmor
const int	AMMO_REGEN_PULSE	= 1000;			// ammo regen in Arena CTF
const int	POWERUP_BLINKS		= 5;			// Number of times the powerup wear off sound plays
const int	POWERUP_BLINK_TIME	= 1000;			// Time between powerup wear off sounds
const float MIN_BOB_SPEED		= 5.0f;			// minimum speed to bob and play run/walk animations at
const int	MAX_RESPAWN_TIME	= 10000;
const int	RAGDOLL_DEATH_TIME	= 3000;
#ifdef _XENON
	const int	RAGDOLL_DEATH_TIME_XEN_SP	= 1000;
	const int	MAX_RESPAWN_TIME_XEN_SP	= 3000;
#endif
const int	STEPUP_TIME			= 200;
const int	MAX_INVENTORY_ITEMS = 20;

const int	ARENA_POWERUP_MASK = ( 1 << POWERUP_AMMOREGEN ) | ( 1 << POWERUP_GUARD ) | ( 1 << POWERUP_DOUBLER ) | ( 1 << POWERUP_SCOUT );

//const idEventDef EV_Player_HideDatabaseEntry ( "<hidedatabaseentry>", NULL );
const idEventDef EV_Player_ZoomIn ( "<zoomin>" );
const idEventDef EV_Player_ZoomOut ( "<zoomout>" );

const idEventDef EV_Player_GetButtons( "getButtons", NULL, 'd' );
const idEventDef EV_Player_GetMove( "getMove", NULL, 'v' );
const idEventDef EV_Player_GetViewAngles( "getViewAngles", NULL, 'v' );
const idEventDef EV_Player_SetViewAngles( "setViewAngles", "v" );
const idEventDef EV_Player_StopFxFov( "stopFxFov" );
const idEventDef EV_Player_EnableWeapon( "enableWeapon" );
const idEventDef EV_Player_DisableWeapon( "disableWeapon" );
const idEventDef EV_Player_GetCurrentWeapon( "getCurrentWeapon", NULL, 's' );
const idEventDef EV_Player_GetPreviousWeapon( "getPreviousWeapon", NULL, 's' );
const idEventDef EV_Player_SelectWeapon( "selectWeapon", "s" );
const idEventDef EV_Player_GetWeaponEntity( "getWeaponEntity", NULL, 'e' );
const idEventDef EV_Player_ExitTeleporter( "exitTeleporter" );
const idEventDef EV_Player_HideTip( "hideTip" );
const idEventDef EV_Player_LevelTrigger( "levelTrigger" );
const idEventDef EV_SpectatorTouch( "spectatorTouch", "et" );
const idEventDef EV_Player_GetViewPos("getViewPos", NULL, 'v');
const idEventDef EV_Player_FinishHearingLoss ( "<finishHearingLoss>", "f" );
const idEventDef EV_Player_GetAmmoData( "getAmmoData", "s", 'v');
const idEventDef EV_Player_RefillAmmo( "refillAmmo" );
const idEventDef EV_Player_SetExtraProjPassEntity( "setExtraProjPassEntity", "E" );
const idEventDef EV_Player_SetArmor( "setArmor", "f" );
const idEventDef EV_Player_DamageEffect( "damageEffect", "sE" );
const idEventDef EV_Player_AllowFallDamage( "allowFallDamage", "d" );

// mekberg: allow enabling/disabling of objectives
const idEventDef EV_Player_EnableObjectives( "enableObjectives" );
const idEventDef EV_Player_DisableObjectives( "disableObjectives" );

// mekberg: don't suppress showing of new objectives anymore
const idEventDef EV_Player_AllowNewObjectives( "<allownewobjectives>" );

// RAVEN END

CLASS_DECLARATION( idActor, idPlayer )
//	EVENT( EV_Player_HideDatabaseEntry,		idPlayer::Event_HideDatabaseEntry )
	EVENT( EV_Player_ZoomIn,				idPlayer::Event_ZoomIn )
	EVENT( EV_Player_ZoomOut,				idPlayer::Event_ZoomOut )
	EVENT( EV_Player_GetButtons,			idPlayer::Event_GetButtons )
	EVENT( EV_Player_GetMove,				idPlayer::Event_GetMove )
	EVENT( EV_Player_GetViewAngles,			idPlayer::Event_GetViewAngles )
	EVENT( EV_Player_SetViewAngles,			idPlayer::Event_SetViewAngles )
	EVENT( EV_Player_StopFxFov,				idPlayer::Event_StopFxFov )
	EVENT( EV_Player_EnableWeapon,			idPlayer::Event_EnableWeapon )
	EVENT( EV_Player_DisableWeapon,			idPlayer::Event_DisableWeapon )
	EVENT( EV_Player_GetCurrentWeapon,		idPlayer::Event_GetCurrentWeapon )
	EVENT( EV_Player_GetPreviousWeapon,		idPlayer::Event_GetPreviousWeapon )
	EVENT( EV_Player_SelectWeapon,			idPlayer::Event_SelectWeapon )
	EVENT( EV_Player_GetWeaponEntity,		idPlayer::Event_GetWeaponEntity )
	EVENT( EV_Player_ExitTeleporter,		idPlayer::Event_ExitTeleporter )
	EVENT( EV_Player_HideTip,				idPlayer::Event_HideTip )
	EVENT( EV_Player_LevelTrigger,			idPlayer::Event_LevelTrigger )
	EVENT( EV_Player_GetViewPos,			idPlayer::Event_GetViewPos )
	EVENT( EV_Player_FinishHearingLoss,		idPlayer::Event_FinishHearingLoss )
	EVENT( EV_Player_GetAmmoData,			idPlayer::Event_GetAmmoData )
	EVENT( EV_Player_RefillAmmo,			idPlayer::Event_RefillAmmo )
	EVENT( EV_Player_AllowFallDamage,		idPlayer::Event_AllowFallDamage )


// mekberg: allow enabling/disabling of objectives
	EVENT ( EV_Player_EnableObjectives,		idPlayer::Event_EnableObjectives )
	EVENT ( EV_Player_DisableObjectives,	idPlayer::Event_DisableObjectives )

// mekberg: don't suppress showing of new objectives anymore
	EVENT ( EV_Player_AllowNewObjectives,	idPlayer::Event_AllowNewObjectives )
// RAVEN END
	
	EVENT( AI_EnableTarget,					idPlayer::Event_EnableTarget )
	EVENT( AI_DisableTarget,				idPlayer::Event_DisableTarget )

	EVENT( EV_ApplyImpulse,					idPlayer::Event_ApplyImpulse )

// RAVEN BEGIN
// mekberg: sethealth on player.
	EVENT( AI_SetHealth,					idPlayer::Event_SetHealth )
//MCG: setArmor
	EVENT( EV_Player_SetArmor,				idPlayer::Event_SetArmor )
// RAVEN END;
	EVENT( EV_Player_SetExtraProjPassEntity,idPlayer::Event_SetExtraProjPassEntity )
//MCG: direct damage
	EVENT( EV_Player_DamageEffect,			idPlayer::Event_DamageEffect )
END_CLASS

// RAVEN BEGIN
// asalmon: Xenon weapon combo system
#ifdef _XENON
nextWeaponCombo_t weaponComboChart[12] = {
	// up, down, left, right
	{1,3,2,4},	 // 0: empty slot (select none)
	{10,0,1,1},
	{2,2,5,0},
	{0,7,3,3},
	{4,4,0,9},
	{5,5,6,2},
	{6,6,6,5},
	{3,7,7,7},
	{8,8,9,8},
	{9,9,4,8},
	{10,1,10,10},
	{0,0,0,0} 
};
#endif
// RAVEN END

const idVec4 marineHitscanTint( 0.69f, 1.0f, 0.4f, 1.0f );
const idVec4 stroggHitscanTint( 1.0f, 0.5f, 0.0f, 1.0f );
const idVec4 defaultHitscanTint( 0.4f, 1.0f, 0.4f, 1.0f );

/*
==============
idInventory::Clear
==============
*/
void idInventory::Clear( void ) {
	maxHealth			= 0;
	weapons				= 0;
	carryOverWeapons	= 0;
	powerups			= 0;
	armor				= 0;
	maxarmor			= 0;
	secretAreasDiscovered = 0;

	memset( ammo, 0, sizeof( ammo ) );

	ClearPowerUps();

	memset( weaponMods, 0, sizeof(weaponMods) );

	// set to -1 so that the gun knows to have a full clip the first time we get it and at the start of the level
	memset( clip, -1, sizeof( clip ) );
	
	items.DeleteContents( true );
	pdas.Clear();
	videos.Clear();

	levelTriggers.Clear();

	nextItemPickup = 0;
	nextItemNum = 1;
	onePickupTime = 0;
	objectiveNames.Clear();

 	ammoPredictTime = 0;
 	lastGiveTime = 0;

	memset(	ammoRegenStep, -1, sizeof( int ) * MAX_WEAPONS );
	memset( ammoIndices, -1, sizeof( int ) * MAX_WEAPONS );
	memset( startingAmmo, -1, sizeof( int ) * MAX_WEAPONS );
	memset( ammoRegenTime, -1, sizeof( int ) * MAX_WEAPONS );
}

/*
==============
idInventory::GivePowerUp
==============
*/
void idInventory::GivePowerUp( idPlayer *player, int powerup, int msec ) {
	powerups |= 1 << powerup;
	powerupEndTime[ powerup ] = msec == -1 ? -1 : (gameLocal.time + msec);
}

/*
==============
idInventory::ClearPowerUps
==============
*/
void idInventory::ClearPowerUps( void ) {
	int i;
	for ( i = 0; i < POWERUP_MAX; i++ ) {
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

	// ammo
	for( i = 0; i < MAX_AMMOTYPES; i++ ) {
		name = rvWeapon::GetAmmoNameForIndex( i );
		if ( name ) {
			dict.SetInt( name, ammo[ i ] );
		}
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

	// weapons
	dict.SetInt( "weapon_bits", weapons );

	// weapon mods
	for ( i = 0; i < MAX_WEAPONS; i++ ) {
		dict.SetInt( va( "weapon_mods_%i", i ), weaponMods[ i ] );
	}

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

	//We might not need to clear it out.
	//Clear();

	// health/armor
	maxHealth		= dict.GetInt( "maxhealth", "100" );
	armor			= dict.GetInt( "armor", "50" );
	maxarmor		= dict.GetInt( "maxarmor", "100" );

	// ammo
	for( i = 0; i < MAX_AMMOTYPES; i++ ) {
		name = rvWeapon::GetAmmoNameForIndex ( i );
		if ( name ) {
			ammo[ i ] = dict.GetInt( name );
		}
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

	// weapons are stored as a number for persistant data, but as strings in the entityDef
	weapons	= dict.GetInt( "weapon_bits", "0" );

// RAVEN BEGIN
// mekberg: removed nightmare weapon check.
	Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false );
// RAVEN END

	// weapon mods
	for ( i = 0; i < MAX_WEAPONS; i++ ) {
		weaponMods[ i ] = dict.GetInt( va( "weapon_mods_%i", i ) );
	}
	// forcefully invalidate the weapon
	owner->GiveWeaponMods( 0 );

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

	savefile->WriteInt( maxHealth );
	savefile->WriteInt( weapons );
	savefile->WriteInt( powerups );
	savefile->WriteInt( armor );
	savefile->WriteInt( maxarmor );

	for( i = 0; i < MAX_AMMO; i++ ) {
		savefile->WriteInt( ammo[ i ] );
	}

	for( i = 0; i < MAX_WEAPONS; i++ ) {
		savefile->WriteInt( clip[ i ] );
		savefile->WriteInt( weaponMods[i] );
	}

	for( i = 0; i < POWERUP_MAX; i++ ) {
		savefile->WriteInt( powerupEndTime[ i ] );
	}

	savefile->WriteInt( ammoPredictTime );
 	savefile->WriteInt( lastGiveTime );

	// Save Items
	savefile->WriteInt( items.Num() );
	for( i = 0; i < items.Num(); i++ ) {
		savefile->WriteDict( items[ i ] );
	}

	// TOSAVE: idStrList				pdas;
	// TOSAVE: idStrList				pdaSecurity;
	// TOSAVE: idStrList				videos;

	// Save level triggers
	savefile->WriteInt( levelTriggers.Num() );
	for ( i = 0; i < levelTriggers.Num(); i++ ) {
		savefile->WriteString( levelTriggers[i].levelName );
		savefile->WriteString( levelTriggers[i].triggerName );
	}

	savefile->WriteInt( nextItemPickup );
	savefile->WriteInt( nextItemNum );
	savefile->WriteInt( onePickupTime );

	// Save pick up item names
	savefile->WriteInt( pickupItemNames.Num() );
	for( i = 0; i < pickupItemNames.Num(); i++ ) {
		savefile->WriteString( pickupItemNames[ i ].name );
		savefile->WriteString( pickupItemNames[ i ].icon );
	}

	// Save objectives
	savefile->WriteInt( objectiveNames.Num() );
	for( i = 0; i < objectiveNames.Num(); i++ ) {
		savefile->WriteString( objectiveNames[i].screenshot );
		savefile->WriteString( objectiveNames[i].text );
		savefile->WriteString( objectiveNames[i].title );
	}
/*
	// Save database
	savefile->WriteInt ( database.Num() );
	for ( i = 0; i < database.Num(); i ++ ) {
		savefile->WriteString ( database[i].title );
		savefile->WriteString ( database[i].text );
		savefile->WriteString ( database[i].image );
		savefile->WriteString ( database[i].filter );
	}
*/
	savefile->WriteInt( secretAreasDiscovered );

	savefile->WriteSyncId();
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

	for( i = 0; i < MAX_AMMO; i++ ) {
		savefile->ReadInt( ammo[ i ] );
	}

	for( i = 0; i < MAX_WEAPONS; i++ ) {
		savefile->ReadInt( clip[ i ] );
		savefile->ReadInt( weaponMods[i] );
	}

	for( i = 0; i < POWERUP_MAX; i++ ) {
		savefile->ReadInt( powerupEndTime[ i ] );
	}

	savefile->ReadInt( ammoPredictTime );
 	savefile->ReadInt( lastGiveTime );

	// Load Items
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idDict *itemdict = new idDict;

		savefile->ReadDict( itemdict );
		items.Append( itemdict );
	}

	// TORESTORE: idStrList				pdas;
	// TORESTORE: idStrList				pdaSecurity;
	// TORESTORE: idStrList				videos;

	// Load level triggers
	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idLevelTriggerInfo lti;
		savefile->ReadString( lti.levelName );
		savefile->ReadString( lti.triggerName );
		levelTriggers.Append( lti );
	}

	savefile->ReadInt( nextItemPickup );
	savefile->ReadInt( nextItemNum );
	savefile->ReadInt( onePickupTime );
	
	// Load pickup items
	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) {
		idItemInfo itemInfo;
		savefile->ReadString( itemInfo.name );
		savefile->ReadString( itemInfo.icon );
		pickupItemNames.Append( itemInfo );
	}

	// Load objectives
	savefile->ReadInt( num );
	for( i = 0; i < num; i++ ) {
		idObjectiveInfo obj;
		savefile->ReadString( obj.screenshot );
		savefile->ReadString( obj.text );
		savefile->ReadString( obj.title );
		objectiveNames.Append( obj );
	}
/*
	// Load database
	savefile->ReadInt ( num );
	for ( i = 0; i < num; i++ ) {
		rvDatabaseEntry entry;
		savefile->ReadString ( entry.title );
		savefile->ReadString ( entry.text );
		savefile->ReadString ( entry.image );
		savefile->ReadString ( entry.filter );
		database.Append ( entry );
	}
*/
	savefile->ReadInt( secretAreasDiscovered );

	savefile->ReadSyncId( "idInventory::Restore" );
}

/*
==============
idInventory::AmmoIndexForAmmoClass
==============
*/
int idInventory::AmmoIndexForAmmoClass( const char *ammo_classname ) const {
	return rvWeapon::GetAmmoIndexForName( ammo_classname );
}

/*
==============
idInventory::MaxAmmoForAmmoClass
==============
*/
int idInventory::MaxAmmoForAmmoClass( idPlayer *owner, const char *ammo_classname ) const {
	return owner->spawnArgs.GetInt( va( "max_%s", ammo_classname ), "0" );
}

/*
==============
idInventory::AmmoIndexForWeaponClass
==============
*/
int idInventory::AmmoIndexForWeaponClass( const char *weapon_classname, int *ammoRequired ) {
	const idDeclEntityDef *decl = gameLocal.FindEntityDef( weapon_classname, false );
	if ( !decl ) {
		gameLocal.Error( "Unknown weapon in decl '%s'", weapon_classname );
	}
	if ( ammoRequired ) {
		*ammoRequired = decl->dict.GetInt( "ammoRequired" );
	}
	return AmmoIndexForAmmoClass( decl->dict.GetString( "ammoType" ) );
}

/*
==============
idInventory::AmmoClassForWeaponClass
==============
*/
const char * idInventory::AmmoClassForWeaponClass( const char *weapon_classname ) {
	const idDeclEntityDef *decl = gameLocal.FindEntityDef( weapon_classname, false );
	if ( !decl ) {
		gameLocal.Error( "Unknown weapon in decl '%s'", weapon_classname );
	}

	return decl->dict.GetString( "ammoType" );
}

// RAVEN BEGIN
// mekberg: if the player can pick up ammo at this time
/*
==============
idInventory::DetermineAmmoAvailability
==============
*/
bool idInventory::DetermineAmmoAvailability( idPlayer* owner, const char *ammoName, int ammoIndex, int ammoAmount, int ammoMax ) {
	const idDeclEntityDef *weaponDecl = NULL;
	const idDict*	weaponDict	= NULL;
	const char*		mod			= NULL;
	const idDict*	modDict		= NULL;
	int				weaponIndex	= -1;
	int				clipSize	= 0;	
	int				modClipSize	= 0;	
	int				difference	= 0;
	idStr			realAmmoName( ammoName );

	// Early out
	if ( ammo[ ammoIndex ] == ammoMax ) {
		return false;
	}

	// Make sure the clip info is updated.
	if ( owner->weapon ) {
		clip[ owner->GetCurrentWeapon( ) ] = owner->weapon->AmmoInClip( );
	}

	 if ( !idStr::Icmpn( ammoName, "start_ammo_", 11 ) ) {
		realAmmoName.StripLeading( "start_" );
	}
	
	// Find the entityDef for the weapon that uses this ammo.
	for ( int i = 0; i < MAX_WEAPONS; i++ ) {

		if ( ! ( weapons & ( 1 << i ) ) ) {
			continue;
		}

		weaponDecl = owner->GetWeaponDef( i );

		if ( !weaponDecl ) {
			continue;
		}

		if ( !idStr::Icmp ( weaponDecl->dict.GetString( "ammoType" ), realAmmoName.c_str() ) ) {
			weaponDict = &( weaponDecl->dict );
			weaponIndex = i;
			break;
		}
	}

	// If we didn't find one.
	if ( weaponIndex == -1 ) {
		// If we are picking up ammo and we aren't currently full.
		if ( ammoAmount && ammo[ ammoIndex ] != ammoMax ) {
			ammo[ ammoIndex ] += ammoAmount;
			if ( ammo[ ammoIndex ] > ammoMax ) {
				ammo[ ammoIndex ] = ammoMax;
			}
			return true;
		}		
		return false;
	}

	clipSize = weaponDict->GetInt( "clipSize", "0" );

	// Find the weaponmods for this weapon and see if we have any clipsize mods.
	for ( int m = 0; m < MAX_WEAPONMODS; m ++ ) {		
		if ( ! ( weaponMods[ weaponIndex ] & ( 1 << m ) ) ) {
			continue;
		}

		mod = weaponDict->GetString ( va ( "def_mod%d" , m + 1 ) );
		if ( !mod || !*mod ) {
			break;
		}

		modDict = gameLocal.FindEntityDefDict ( mod, false );
		modClipSize = modDict->GetInt( "clipSize", "0" );

		if ( modClipSize > clipSize ) {
			clipSize = modClipSize;
		}					
	}
	
	// Don't bother with these checks if we don't have a clipsize
	if ( clipSize ) {
		difference = ( ammoMax - clipSize ) - ( ammo[ ammoIndex ] - clip[ weaponIndex ] );

		if (  difference  ) {
			if ( ammoAmount  > difference ) {			
				ammo[ ammoIndex ] += difference;
			} else {
				ammo[ ammoIndex ] += ammoAmount;
			}
			return true;
		} else {
			return false;
		}
	} else if ( ( ammo[ ammoIndex ] + ammoAmount ) > ammoMax ) {
		ammo[ ammoIndex ] = ammoMax;
	} else {
		ammo[ ammoIndex ] += ammoAmount;
	}	
	return true;
}
// RAVEN END


/*
==============
idInventory::AmmoIndexForWeaponIndex
==============
*/
int	idInventory::AmmoIndexForWeaponIndex( int weaponIndex ) {
	if( ammoIndices[ weaponIndex ] == -1 ) {
		const idDict* playerDict = gameLocal.FindEntityDefDict( "player_marine", false );
		if( !playerDict ) {
			gameLocal.Error( "idInventory::AmmoIndexForWeaponIndex() - Can't find player def\n" );
			return -1;
		}

		ammoIndices[ weaponIndex ] = AmmoIndexForWeaponClass( playerDict->GetString( va( "def_weapon%d", weaponIndex ) ) );
	}

	return ammoIndices[ weaponIndex ];
}

/*
==============
idInventory::StartingAmmoForWeaponIndex
==============
*/
int idInventory::StartingAmmoForWeaponIndex( int weaponIndex ) {
	if( startingAmmo[ weaponIndex ] == -1 ) {
		const idDict* playerDict = gameLocal.FindEntityDefDict( "player_marine", false );
		if( !playerDict ) {
			gameLocal.Error( "idInventory::StartingAmmoForWeaponIndex() - Can't find player def\n" );
			return -1;
		}

		const idDict* weaponDict = gameLocal.FindEntityDefDict( playerDict->GetString( va( "def_weapon%d", weaponIndex ) ), false );
		if( !weaponDict ) {
			gameLocal.Warning( "idInventory::StartingAmmoForWeaponIndex() - Unknown weapon '%d'\n", weaponIndex );
			return -1;
		}

		const idKeyValue* kv = weaponDict->MatchPrefix( "inv_start_ammo" );
		if( kv == NULL ) {
			startingAmmo[ weaponIndex ] = 1;
		} else {
			startingAmmo[ weaponIndex ] = atoi( kv->GetValue() );
			kv = weaponDict->MatchPrefix( "inv_start_ammo", kv );
			if( kv != NULL ) {
				gameLocal.Error( "idInventory::StartingAmmoForWeaponIndex() - Weapon dict for player's def_weapon%d has multiple inv_start_ammo entries\n", weaponIndex );
				return -1;
			}
		}
	}

	return startingAmmo[ weaponIndex ];
}

/*
==============
idInventory::AmmoRegenStepForWeaponIndex
==============
*/
int	idInventory::AmmoRegenStepForWeaponIndex( int weaponIndex ) {
	if( ammoRegenStep[ weaponIndex ] == -1 ) {
		const idDict* playerDict = gameLocal.FindEntityDefDict( "player_marine", false );
		if( !playerDict ) {
			gameLocal.Error( "idInventory::AmmoRegenStepForWeaponIndex() - Can't find player def\n" );
			return -1;
		}

		const idDict* weaponDict = gameLocal.FindEntityDefDict( playerDict->GetString( va( "def_weapon%d", weaponIndex ) ), false );
		if( !weaponDict ) {
			gameLocal.Warning( "idInventory::AmmoRegenStepForWeaponIndex() - Unknown weapon '%d'\n", weaponIndex );
			return -1;
		}

		ammoRegenStep[ weaponIndex ] = weaponDict->GetInt( "ammoRegenStep", "1" );
	}

	return ammoRegenStep[ weaponIndex ];
}

/*
==============
idInventory::AmmoRegenTimeForAmmoIndex
==============
*/
int	idInventory::AmmoRegenTimeForWeaponIndex( int weaponIndex ) {
	if ( ammoRegenTime[ weaponIndex ] == -1 ) {
		const idDict* playerDict = gameLocal.FindEntityDefDict( "player_marine", false );
		if( !playerDict ) {
			gameLocal.Error( "idInventory::AmmoRegenTimeForWeaponIndex() - Can't find player def\n" );
			return -1;
		}

		const idDict* weaponDict = gameLocal.FindEntityDefDict( playerDict->GetString( va( "def_weapon%d", weaponIndex ) ), false );
		if( !weaponDict ) {
			gameLocal.Warning( "idInventory::AmmoRegenTimeForWeaponIndex() - Unknown weapon '%d'\n", weaponIndex );
			return -1;
		}

		ammoRegenTime[ weaponIndex ] = weaponDict->GetInt( "ammoRegenTime", "1" );
	}

	return ammoRegenTime[ weaponIndex ];
}


/*
==============
idInventory::Give
If checkOnly is true, check only for possibility of adding to inventory, don't actually add
==============
*/
bool idInventory::Give( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon, bool updateHud, bool dropped, bool checkOnly ) {
	int						i;
	const char				*pos;
	const char				*end;
	int						len;
	idStr					weaponString;
	int						max;
	int						amount;

	if ( !idStr::Icmpn( statname, "ammo_", 5 ) ) {
		i = AmmoIndexForAmmoClass( statname );
		max = MaxAmmoForAmmoClass( owner, statname );
		amount = atoi( value );

// RAVEN BEGIN
// mekberg: check max ammo vs clipsize when picking up ammo
		if ( !gameLocal.IsMultiplayer ( ) ) {
			return DetermineAmmoAvailability ( owner, statname, i, amount, max );	
		} else if ( ammo[ i ] >= max ) {
			return false;
		}

		if ( amount && !checkOnly ) {			
			ammo[ i ] += amount;
			if ( ( max > 0 ) && ( ammo[ i ] > max ) ) {
				ammo[ i ] = max;
			}
		}
// RAVEN END
	} else if ( !idStr::Icmpn( statname, "start_ammo_", 11 ) ) {
		// starting ammo gives only if current ammo is below it
		idStr ammoname( statname );
		ammoname.StripLeading( "start_" );
		i = AmmoIndexForAmmoClass( ammoname.c_str() );
		max = MaxAmmoForAmmoClass( owner, ammoname.c_str() );
		amount = atoi( value );

// RAVEN BEGIN
// mekberg: check max ammo vs clipsize when picking up ammo
		if ( !gameLocal.IsMultiplayer ( ) ) {
			return DetermineAmmoAvailability ( owner, statname, i, amount, max );
		} else if ( amount ) {	
			if ( ammo[ i ] >= amount ) {
				amount = 1;
			} else {
				amount = amount - ammo[ i ];
			}
		}

		if ( amount && !checkOnly ) {			
			ammo[ i ] += amount;
			if ( ( max > 0 ) && ( ammo[ i ] > max ) ) {
				ammo[ i ] = max;
			}
		}
// RAVEN END
	} else if ( !idStr::Icmp( statname, "armor" ) ) {
		if ( armor >= maxarmor * 2 ) {
			return false;
		}
	} else 	if ( !idStr::Icmp( statname, "health" ) ) {
		if ( owner->health >= maxHealth ) {
			return false;
		}
	} else if ( idStr::FindText( statname, "inclip_" ) == 0 ) {
		i = owner->SlotForWeapon ( statname + 7 );
		if ( i != -1 && !checkOnly ) {
			// set, don't add. not going over the clip size limit.
			clip[ i ] = atoi( value );
		}
	} else if ( !idStr::Icmp( statname, "quad" ) && !checkOnly ) {
		GivePowerUp( owner, POWERUP_QUADDAMAGE, SEC2MS( atof( value ) ) );
	} else if ( !idStr::Icmp( statname, "regen" ) && !checkOnly ) {
		GivePowerUp( owner, POWERUP_REGENERATION, SEC2MS( atof( value ) ) );
	} else if ( !idStr::Icmp( statname, "haste" ) && !checkOnly ) {
		GivePowerUp( owner, POWERUP_HASTE, SEC2MS( atof( value ) ) );
	} else if( !idStr::Icmp( statname, "ammoregen" ) && !checkOnly ) {
		GivePowerUp( owner, POWERUP_AMMOREGEN, -1 );
	} else if ( !idStr::Icmp( statname, "weapon" ) ) {
		bool tookWeapon = false;
 		for( pos = value; pos != NULL; pos = end ) {
			end = strchr( pos, ',' );
			if ( end ) {
				len = end - pos;
				end++;
			} else {
				len = strlen( pos );
			}

			idStr weaponName( pos, 0, len );

			// find the number of the matching weapon names
			i = owner->SlotForWeapon ( weaponName );
// RAVEN BEGIN
// mekberg: check for not found weapons
			if ( i == -1 ) {
				gameLocal.Warning( "Unknown weapon '%s'", weaponName.c_str() );
				return false;
			}
// RAVEN END

 			if ( gameLocal.isMultiplayer 
				&& ( weapons & ( 1 << i ) ) ) {
				//already have this weapon
				if ( !dropped ) {
					//a placed weapon item
					if ( gameLocal.IsWeaponsStayOn() ) {
						//don't pick up weapons at all if you already have them...
						continue;
					}
				}
				// don't pickup "no ammo" weapon types twice
 				// not for singleplayer.. there is only one case in the game where you can get a no ammo
 				// weapon when you might already have it, in that case it is more consistent to pick it up
				// cache the media for this weapon
				const idDict* dict;
				dict = &owner->GetWeaponDef ( i )->dict;
				if ( dict && !dict->GetInt( "ammoRequired" ) ) {
					continue;
				}
			}

 			if ( !gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || ( weaponName == "weapon_fists" ) ) {
 				if ( ( weapons & ( 1 << i ) ) == 0 || gameLocal.isMultiplayer ) {
					if ( ( owner->GetUserInfo()->GetBool( "ui_autoSwitch" ) 
						|| ( gameLocal.isMultiplayer && gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() ) )
						&& idealWeapon && !checkOnly )
					{
						// client prediction should not get here
						assert( !gameLocal.isClient );
 						*idealWeapon = i;
 					} 
 					if ( owner->hud && updateHud && lastGiveTime + 1000 < gameLocal.time && !checkOnly ) {
 						owner->hud->SetStateInt( "newWeapon", i );
 						owner->hud->HandleNamedEvent( "newWeapon" );
 						lastGiveTime = gameLocal.time;
 					}
					if( !checkOnly ) {
 						weapons |= ( 1 << i );
					}
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
	weapons &= ( 0xffffffff ^ ( 1 << weapon_index ) );
	int ammo_i = AmmoIndexForWeaponClass( weapon_classname, NULL );
	if ( ammo_i ) {
		clip[ weapon_index ] = -1;
		ammo[ ammo_i ] = 0;
	}

	weaponMods[weapon_index] = 0;
}

/*
===============
idInventory::HasAmmo
===============
*/
int idInventory::HasAmmo( int index, int amount ) {
	if ( ( index == 0 ) || !amount ) {
		// always allow weapons that don't use ammo to fire
		return -1;
	}

	// check if we have infinite ammo
	if ( ammo[ index ] < 0 ) {
		return -1;
	}

	// return how many shots we can fire
	return ammo[ index ] / amount;
}

/*
===============
idInventory::HasAmmo
===============
*/
int idInventory::HasAmmo( const char *weapon_classname ) {
	int ammoRequired;
	int index;
	index = AmmoIndexForWeaponClass( weapon_classname, &ammoRequired );
	return HasAmmo( index, ammoRequired );
}

/*
===============
idInventory::UseAmmo
===============
*/
bool idInventory::UseAmmo( int index, int amount ) {
	if ( !HasAmmo( index, amount ) ) {
		return false;
	}

	// take an ammo away if not infinite
	if ( ammo[ index ] >= 0 ) {
		ammo[ index ] -= amount;
 		ammoPredictTime = gameLocal.time; // mp client: we predict this. mark time so we're not confused by snapshots
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

	alreadyDidTeamAnnouncerSound = false;

	doInitWeapon			= false;
	noclip					= false;
	godmode					= false;
	undying					= g_forceUndying.GetBool() ? !gameLocal.isMultiplayer : false;

	spawnAnglesSet			= false;
	spawnAngles				= ang_zero;
	viewAngles				= ang_zero;
	deltaViewAngles			= ang_zero;
	cmdAngles				= ang_zero;

	demoViewAngleTime		= 0;
	demoViewAngles			= ang_zero;

	oldButtons				= 0;
	buttonMask				= 0;
	oldFlags				= 0;

	lastHitTime				= 0;
	lastSavingThrowTime		= 0;

	weapon					= NULL;

	hud						= NULL;
	mphud					= NULL;
	objectiveSystem			= NULL;
	objectiveSystemOpen		= false;
	showNewObjectives		= false;
#ifdef _XENON
	g_ObjectiveSystemOpen	= false;
#endif
	objectiveButtonReleased = false;
	cinematicHud			= NULL;

	overlayHud				= NULL;
	overlayHudTime			= 0;

	lastDmgTime				= 0;
	deathClearContentsTime	= 0;
	nextHealthPulse			= 0;

	scoreBoardOpen			= false;
	forceScoreBoard			= false;
	forceScoreBoardTime		= 0;
	forceRespawn			= false;
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	allowedToRespawn		= true;
// squirrel: Mode-agnostic buymenus
	inBuyZone				= false;
	inBuyZonePrev			= false;
// RITUAL END
	spectating				= false;
	spectator				= 0;
	forcedReady				= false;
	wantSpectate			= false;

	lastHitToggle			= false;
	lastArmorHit			= false;

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

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	carryOverCurrentWeapon	= -1;
// RITUAL END
	currentWeapon			= -1;
	idealWeapon				= -1;
	previousWeapon			= -1;
	weaponSwitchTime		=  0;
	weaponEnabled			= true;
 	showWeaponViewModel		= true;
	oldInventoryWeapons		= 0;

// RAVEN BEGIN
// mekberg: allow disabling of objectives during non-cinematic time periods
	objectivesEnabled = true;
// RAVEN END

	skin					= NULL;
	weaponViewSkin			= NULL;
	headSkin				= NULL;
	powerUpSkin				= NULL;

	numProjectilesFired		= 0;
	numProjectileHits		= 0;

	airless					= false;
	airTics					= 0;
	lastAirDamage			= 0;

	gibDeath				= false;
	gibsLaunched			= false;
	gibDir					= vec3_zero;

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
	focusUI					= NULL;
	focusEnt				= NULL;
	focusType				= FOCUS_NONE;
	focusBrackets			= NULL;
	focusBracketsTime		= 0;

	talkingNPC				= NULL;

	cursor					= NULL;
 	talkCursor				= 0;
	
	oldMouseX				= 0;
	oldMouseY				= 0;

	lastDamageDef			= 0;
	lastDamageDir			= vec3_zero;
	lastDamageLocation		= 0;

	predictedFrame			= 0;
	predictedOrigin			= vec3_zero;
	predictedAngles			= ang_zero;
	predictedUpdated		= false;
	predictionOriginError	= vec3_zero;
	predictionAnglesError	= ang_zero;
	predictionErrorTime		= 0;

	fl.networkSync			= true;

	latchedTeam				= -1;
	hudTeam					= -1;
 	doingDeathSkin			= false;
 	weaponGone				= false;
 	useInitialSpawns		= false;
	lastSpectateTeleport	= 0;
	hiddenWeapon			= false;
	tipUp					= false;
	objectiveUp				= false;
 	teleportEntity			= NULL;
	teleportKiller			= -1;
	lastKiller				= NULL;

 	respawning				= false;
 	ready					= false;
 	leader					= false;
 	lastSpectateChange		= 0;
	lastArenaChange			= 0;
 	lastTeleFX				= -9999;

 	weaponCatchup			= false;
 	lastSnapshotSequence	= 0;
 
 	aimClientNum			= -1;
 
 	spawnedTime				= 0;
 
 	isTelefragged			= false;
	isLagged				= false;
 	isChatting				= false;

	intentDir.Zero();
	aasSensor = rvAASTacticalSensor::CREATE_SENSOR(this);

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	ResetCash();
// RITUAL END

	zoomFov.Init ( 0, 0, DefaultFov(), DefaultFov() );
	zoomed					= false;
	
	memset ( cachedWeaponDefs, 0, sizeof(cachedWeaponDefs) );
	memset ( cachedPowerupDefs, 0, sizeof(cachedPowerupDefs) );

	lastImpulsePlayer = NULL;
	lastImpulseTime = gameLocal.time;

	weaponChangeIconsUp = false;

	reloadModel = false;
	modelDecl = NULL;

	disableHud = false;

	mutedPlayers = 0;
	friendPlayers = 0;
	connectTime = 0;
	rank = -1;
	arena = 0;

	memset( nextAmmoRegenPulse, 0, sizeof( int ) * MAX_AMMO );

	spectator = 0;

	quadOverlay = NULL;
	hasteOverlay = NULL;
	regenerationOverlay = NULL;
	invisibilityOverlay = NULL;
	powerUpOverlay = NULL;

	tourneyStatus = PTS_UNKNOWN;
	
	vsMsgState = false;

	deathSkinTime = 0;

	jumpDuringHitch = false;

	lastPickupTime = 0;

	int		i;

	for( i = 0; i < MAX_CONCURRENT_VOICES; i++ ) {

		voiceDest[i] = -1;
		voiceDestTimes[i] = 0;
	}

	itemCosts = NULL;

	teamHealthRegen		= NULL;
	teamHealthRegenPending	= false;
	teamAmmoRegen			= NULL;
	teamAmmoRegenPending	= false;
	teamDoubler			= NULL;		
	teamDoublerPending		= false;
}

/*
==============
idPlayer::SetShowHud
==============
*/
void idPlayer::SetShowHud( bool showHud )	{
	disableHud = !showHud;
}

/*
==============
idPlayer::SetShowHud
==============
*/
bool idPlayer::GetShowHud( void )	{
	return !disableHud;
}

/*
==============
idPlayer::SetWeapon
==============
*/
void idPlayer::SetWeapon( int weaponIndex ) {
	if ( weapon && weaponIndex == currentWeapon ) {
		return;
	}
	
	// Clear the weapon entity
	delete weapon;
	weapon = NULL;

	previousWeapon	= currentWeapon;
	currentWeapon	= weaponIndex;
	weaponGone		= false;		

	if ( weaponIndex < 0 ) {
		weaponGone = true;
		return;
	}
	
	animPrefix = spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) );

	idTypeInfo*	typeInfo;
	weaponDef = GetWeaponDef( currentWeapon );
	if ( !weaponDef ) {
		gameLocal.Error( "Weapon definition not found for weapon %d", currentWeapon ) ;
	}
	typeInfo = idClass::GetClass( weaponDef->dict.GetString( "weaponclass", "rvWeapon" ) );
	if ( !typeInfo || !typeInfo->IsType( rvWeapon::GetClassType() ) ) {
		gameLocal.Error( "Invalid weapon class '%s' specified for weapon '%s'", animPrefix.c_str(), weaponDef->dict.GetString ( "weaponclass", "rvWeapon" ) );
	}
	weapon = static_cast<rvWeapon*>( typeInfo->CreateInstance() );
	weapon->Init( this, weaponDef, currentWeapon, isStrogg );
	weapon->CallSpawn( );

	// Reset the zoom fov on weapon change
	if ( zoomed ) {
		zoomFov.Init ( gameLocal.time, 100, CalcFov(true), DefaultFov() );
		zoomed = false;
	}				

	UpdateHudWeapon();

	// Remove the "weapon_" from the anim prefect for the player world anims
	animPrefix.Strip( "weapon_" );
	
	// Make sure weapon is hidden
	if ( !weaponEnabled ) {
		Event_DisableWeapon( );
	}
}
 
/*
==============
idPlayer::SetupWeaponEntity
==============
*/
void idPlayer::SetupWeaponEntity( void ) {
	int						w;
	const char				*weap;
	const idDeclEntityDef	*decl;
	idEntity				*spawn;
	
	// don't setup weapons for spectators
	if ( gameLocal.isClient || (weaponViewModel && weaponWorldModel) || spectating ) {
		return;
	}

	idDict					args;
	
	if ( !weaponViewModel ) {
		// setup the view model
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		decl = static_cast< const idDeclEntityDef * >( declManager->FindType( DECL_ENTITYDEF, "player_viewweapon", false, false ) );
		if ( !decl ) {
			gameLocal.Error( "entityDef not found: player_viewweapon" );
		}
		args.Set( "name", va( "%s_weapon", name.c_str() ) );
		args.SetInt( "instance", instance );
		args.Set( "classname", decl->GetName() );
		spawn = NULL;
		gameLocal.SpawnEntityDef( args, &spawn );
		if ( !spawn ) {
			gameLocal.Error( "idPlayer::SetupWeaponEntity: failed to spawn weaponViewModel" );
		}
		weaponViewModel = static_cast<rvViewWeapon*>(spawn);
		weaponViewModel->SetName( va("%s_weapon", name.c_str() ) );
		weaponViewModel->SetInstance( instance );
	}
	
			
	if ( !weaponWorldModel ) {
		// setup the world model
		decl = static_cast< const idDeclEntityDef * >( declManager->FindType( DECL_ENTITYDEF, "player_animatedentity", false, false ) );
		if ( !decl ) {
			gameLocal.Error( "entityDef not found: player_animatedentity" );
		}
		args.Set( "name", va( "%s_weapon_world", name.c_str() ) );
		args.SetInt( "instance", instance );
		args.Set( "classname", decl->GetName() );
		spawn = NULL;
		gameLocal.SpawnEntityDef( args, &spawn );
		if ( !spawn ) {
			gameLocal.Error( "idPlayer::SetupWeaponEntity: failed to spawn weaponWorldModel" );
		}
		weaponWorldModel = static_cast<idAnimatedEntity*>(spawn);
		weaponWorldModel->fl.networkSync = true;
		weaponWorldModel->SetName ( va("%s_weapon_world", name.c_str() ) );
		weaponWorldModel->SetInstance( instance );
	}
	
	currentWeapon = -1;

	weaponWorldModel->fl.persistAcrossInstances = true;
	weaponViewModel->fl.persistAcrossInstances = true;

 	for( w = 0; w < MAX_WEAPONS; w++ ) {
 		weap = spawnArgs.GetString( va( "def_weapon%d", w ) );
 		if ( weap && *weap ) {
 			rvWeapon::CacheWeapon( weap );
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
	
	noclip					= false;
	godmode					= false;
	godmodeDamage			= 0;
	undying					= g_forceUndying.GetBool() ? !gameLocal.isMultiplayer : false;

	oldButtons				= 0;
	oldFlags				= 0;

	currentWeapon			= -1;
	idealWeapon				= -1;
	previousWeapon			= -1;
	weaponSwitchTime		= 0;
	weaponEnabled			= true;
 	showWeaponViewModel		= GetUserInfo()->GetBool( "ui_showGun" );
	oldInventoryWeapons		= 0;

	lastDmgTime				= 0;
	
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

	currentLoggedAccel		= 0;

	focusTime				= 0;
	focusUI					= NULL;
	focusEnt				= NULL;
	focusType				= FOCUS_NONE;
	focusBrackets			= NULL;
	focusBracketsTime		= 0;
	
	talkingNPC				= NULL;
 	talkCursor				= 0;

	lightningEffects		= 0;
	lightningNextTime		= 0;

	modelName				= idStr();

	// Remove any hearing loss that may be set up from the last map
	soundSystem->FadeSoundClasses( SOUNDWORLD_GAME, 0, 0.0f, 0 );
	
	// remove any damage effects
	playerView.ClearEffects();

	// damage values
	fl.takedamage			= true;
	ClearPain();

	// restore persistent data
	RestorePersistantInfo();

	bobCycle	= 0;

	SetupWeaponEntity( );
	currentWeapon = -1;
	previousWeapon = -1;
	
	flashlightOn	  = false;

	idealLegsYaw = 0.0f;
	legsYaw = 0.0f;
	legsForward = true;
	oldViewYaw = 0.0f;

// RAVEN BEGIN
// abahr: need to init this
	vehicleCameraDist = 0.0f;

// mekberg: moved into function.
	SetPMCVars( );	
// RAVEN END

	// air always initialized to maximum too
	airTics = pm_airTics.GetFloat();
	airless = false;

	gibDeath = false;
	gibsLaunched = false;
	gibDir.Zero();

// RAVEN BEGIN
// abahr: changed to GetCurrentGravity
	// set the gravity
	physicsObj.SetGravity( gameLocal.GetCurrentGravity(this) );
// RAVEN END

	// start out standing
	SetEyeHeight( pm_normalviewheight.GetFloat() );

	stepUpTime = 0;
	stepUpDelta = 0.0f;
	viewBobAngles.Zero();
	viewBob.Zero();

	if( gameLocal.isMultiplayer && gameLocal.IsTeamGame() ) {
		value = spawnArgs.GetString( va( "model_%s", team ? "strogg" : "marine" ), NULL );
	}
	else {
		value = spawnArgs.GetString( "model" );
	}

	if( gameLocal.isMultiplayer ) {
		UpdateModelSetup( true );
	} else {
		if ( value && ( *value != 0 ) ) {
			SetModel( value );
		}
		// check head
		if( idStr::Icmp( head ? head->spawnArgs.GetString( "classname", "" ) : "", spawnArgs.GetString( "def_head", "" ) ) ) {
			SetupHead();
		}
	}

	if ( cursor ) {
 		cursor->SetStateInt( "talkcursor", 0 );
		cursor->HandleNamedEvent( "showCrossCombat" );
	}

	if ( !gameLocal.isMultiplayer ) {
		if ( g_testDeath.GetBool() && skin ) {
			SetSkin( skin );
			renderEntity.shaderParms[6] = 0.0f;
		} else if ( spawnArgs.GetString( "spawn_skin", NULL, &value ) ) {
			skin = declManager->FindSkin( value );
			SetSkin( skin );
			renderEntity.shaderParms[6] = 0.0f;
		}
	} else {
		SetSkin( skin );
		if( clientHead ) {
			clientHead->SetSkin( headSkin );
			if( clientHead->GetModelDefHandle() > 0) {
				gameRenderWorld->RemoveDecals( clientHead->GetModelDefHandle() );
			}
		}

		if( weaponViewModel ) {
			weaponViewModel->SetSkin( weaponViewSkin );
		}
	}

 	value = spawnArgs.GetString( "joint_hips", "" );
 	hipJoint = animator.GetJointHandle( value );
 	if ( hipJoint == INVALID_JOINT ) {
 		gameLocal.Error( "Joint '%s' not found for 'joint_hips' on '%s'", value, name.c_str() );
 	}
 
 	value = spawnArgs.GetString( "joint_chest", "" );
 	chestJoint = animator.GetJointHandle( value );
 	if ( chestJoint == INVALID_JOINT ) {
 		gameLocal.Error( "Joint '%s' not found for 'joint_chest' on '%s'", value, name.c_str() );
 	}
 
 	value = spawnArgs.GetString( "joint_head", "" );
 	headJoint = animator.GetJointHandle( value );
 	if ( headJoint == INVALID_JOINT ) {
 		gameLocal.Error( "Joint '%s' not found for 'joint_head' on '%s'", value, name.c_str() );
 	}

	// initialize the script variables
	memset ( &pfl, 0, sizeof( pfl ) );
	pfl.onGround = true;
	pfl.noFallingDamage = false;

	// Start in idle
	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 0 );
	SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );
	
	forceScoreBoard		= false;
	forceScoreBoardTime = 0;
	forcedReady			= false;

	privateCameraView	= NULL;

	lastSpectateChange	= 0;
	lastArenaChange		= 0;
 	lastTeleFX			= -9999;

	hiddenWeapon		= false;
	tipUp				= false;
	objectiveUp			= false;
 	teleportEntity		= NULL;
	lastKiller			= NULL;
	teleportKiller		= -1;
 	leader				= false;

 	SetPrivateCameraView( NULL );
 
 	lastSnapshotSequence	= 0;
 
	if ( !gameLocal.isMultiplayer ) {
		// in MP we set isStrogg in UpdateModelSetup()
		isStrogg = spawnArgs.GetBool ( "strogg", "0" );
	}

 
 	aimClientNum		= -1;
	if ( mphud ) {
		mphud->HandleNamedEvent( "aim_fade" );
	}

 	isChatting = false;

	SetInitialHud();

	emote = PE_NONE;

	powerupEffectTime   = 0;
	powerupEffect		= NULL;
	powerupEffectType	= 0;
	hasteEffect			= NULL;
	flagEffect			= NULL;
	arenaEffect			= NULL;

	quadOverlay			= declManager->FindMaterial( spawnArgs.GetString( "mtr_quaddamage_overlay" ), false );
	hasteOverlay		= declManager->FindMaterial( spawnArgs.GetString( "mtr_haste_overlay" ), false );
	regenerationOverlay = declManager->FindMaterial( spawnArgs.GetString( "mtr_regeneration_overlay" ), false );
	invisibilityOverlay = declManager->FindMaterial( spawnArgs.GetString( "mtr_invisibility_overlay" ), false );
	powerUpOverlay		= NULL;

	if ( gameLocal.isMultiplayer && entityNumber == gameLocal.localClientNum ) {
		if ( (gameLocal.mpGame.GetGameState()->GetMPGameState() != WARMUP) && gameLocal.mpGame.GetGameState()->GetMPGameState() != SUDDENDEATH ){
			if( gameLocal.gameType != GAME_TOURNEY || ((rvTourneyGameState*)(gameLocal.mpGame.GetGameState()))->GetArena( arena ).GetState() != AS_WARMUP && ((rvTourneyGameState*)(gameLocal.mpGame.GetGameState()))->GetArena( arena ).GetState() != AS_SUDDEN_DEATH )  {
				// don't clear notices while in warmup modes or sudden death
				GUIMainNotice( "" );
				GUIFragNotice( "" );
			}
		}

		if ( (gameLocal.mpGame.GetGameState()->GetMPGameState() == WARMUP) && vsMsgState ) {
			GUIMainNotice( "" );
			GUIFragNotice( "" );
		}
	}
	
	deathSkinTime		= 0;
	deathStateHitch		= false;
	jumpDuringHitch = false;

	lastPickupTime = 0;

	if ( teamHealthRegenPending ) {
		assert( teamHealthRegen == NULL );
		teamHealthRegenPending = false;
		teamHealthRegen = PlayEffect( "fx_guard", renderEntity.origin, renderEntity.axis, true );
	}
	if ( teamAmmoRegenPending ) {
		assert( teamAmmoRegen == NULL );
		teamAmmoRegenPending = false;
		teamAmmoRegen = PlayEffect( "fx_ammoregen", renderEntity.origin, renderEntity.axis, true );
	}
	if ( teamDoublerPending ) {
		assert( teamDoubler == NULL );
		teamDoublerPending = false;
		teamDoubler = PlayEffect( "fx_doubler", renderEntity.origin, renderEntity.axis, true );
	}
}

/*
===============
idPlayer::ProjectHeadOverlay
===============
*/
void idPlayer::ProjectHeadOverlay( const idVec3 &point, const idVec3 &dir, float size, const char *decal ) {

	if( clientHead ) {
		clientHead.GetEntity()->ProjectOverlay( point, dir, size, decal );
	}
}

/*
===============
idPlayer::GetCursorGUI
===============
*/
idUserInterface* idPlayer::GetCursorGUI( void ) {
	idStr temp;

	assert( !gameLocal.isMultiplayer || gameLocal.localClientNum == entityNumber );
	if ( cursor ) {
		return cursor;
	}
	if ( spawnArgs.GetString( "cursor", "", temp ) ) {
		cursor = uiManager->FindGui( temp, true, gameLocal.isMultiplayer, gameLocal.isMultiplayer );
	}
	return cursor;
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
	SetClipModel( );
	physicsObj.SetMass( spawnArgs.GetFloat( "mass", "100" ) );
	physicsObj.SetContents( CONTENTS_BODY | (use_combat_bbox?CONTENTS_SOLID:0) );
	physicsObj.SetClipMask( MASK_PLAYERSOLID );
	SetPhysics( &physicsObj );
	InitAASLocation();
	
	skin = renderEntity.customSkin;

	// only the local player needs guis
	// for server netdemos that have no local player, we use demo_* guis in idGameLocal
	if ( !gameLocal.isMultiplayer || entityNumber == gameLocal.localClientNum ) {
		
		// load HUD
		hud = NULL;
		mphud = NULL;
 		
		overlayHud = NULL;
		overlayHudTime = 0;
		
		objectiveSystem = NULL;

		if ( spawnArgs.GetString( "hud", "", temp ) ) {
			hud = uiManager->FindGui( temp, true, false, true );
		} else {
			gameLocal.Warning( "idPlayer::Spawn() - No hud for player." );
		}

		if ( gameLocal.isMultiplayer ) {
			if ( spawnArgs.GetString( "mphud", "", temp ) ) {
				mphud = uiManager->FindGui( temp, true, false, true );
			} else {
				gameLocal.Warning( "idPlayer::Spawn() - No MP hud overlay while in MP.");
			}
		}

		if ( hud ) {
			hud->Activate( true, gameLocal.time );
		}

		if ( mphud ) {
			mphud->Activate( true, gameLocal.time );
		}

		// load cursor
		GetCursorGUI();
		if ( cursor ) {
			cursor->Activate( true, gameLocal.time );
		}
		
		// Load 

		if ( spawnArgs.GetString ( "cinematicHud", "", temp ) ) {
			cinematicHud = uiManager->FindGui( temp, true, false, true );
		}

		if ( !gameLocal.isMultiplayer ) {
			objectiveSystem = uiManager->FindGui( spawnArgs.GetString( "wristcomm", "guis/wristcomm.gui" ), true, false, true );
			objectiveSystemOpen = false;
#ifdef _XENON
			g_ObjectiveSystemOpen = objectiveSystemOpen;
#endif
		}

		// clear votes
		// if we want to display current votes that were started before a player was connected
		// but are still being voted on, this should check the current vote and update the gui appropriately
		gameLocal.mpGame.ClearVote( entityNumber );
	}

	SetLastHitTime( 0, false );

	// load the armor sound feedback
	declManager->FindSound( "player_sounds_hitArmor" );

	animator.RemoveOriginOffset( true );

	// initialize user info related settings
	// on server, we wait for the userinfo broadcast, as this controls when the player is initially spawned in game
	// ocassionally, a race condition may mark a client in-game before he is spawned, if this is the case, parse the userinfo here
	if ( (gameLocal.isClient || entityNumber == gameLocal.localClientNum) || (gameLocal.isServer && gameLocal.mpGame.IsInGame( entityNumber ) ) ) {
		UserInfoChanged();
	}

	// create combat collision hull for exact collision detection
	SetCombatModel();

	// init the damage effects
	playerView.SetPlayerEntity( this );

	// supress model in non-player views, but allow it in mirrors and remote views
	renderEntity.suppressSurfaceInViewID = entityNumber+1;

	// don't project shadow on self or weapon
	renderEntity.noSelfShadow = true;

	if( gameLocal.isMultiplayer ) {
		if( clientHead ) {
			clientHead->GetRenderEntity()->suppressSurfaceInViewID = entityNumber + 1;
			clientHead->GetRenderEntity()->noSelfShadow = true;
		}
	} else {
		idAFAttachment *headEnt = head.GetEntity();
		if ( headEnt ) {
			headEnt->GetRenderEntity()->suppressSurfaceInViewID = entityNumber+1;
			headEnt->GetRenderEntity()->noSelfShadow = true;
		}
	}

	if ( gameLocal.isMultiplayer ) {
		Init();
 		Hide();	// properly hidden if starting as a spectator

		// Normally idPlayer::Move() gets called to set the contents to 0, but we don't call
		// move on players not in our snap, so we need to set it manually here.
		bool inOtherInstance = gameLocal.isClient && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance;
		if( inOtherInstance ) {
			physicsObj.SetContents( 0 );
			physicsObj.SetMovementType( PM_SPECTATOR );
			physicsObj.SetClipMask( MASK_DEADSOLID );
		}

		if ( !gameLocal.isClient ) {
			// set yourself ready to spawn. idMultiplayerGame will decide when/if appropriate and call SpawnFromSpawnSpot
			SetupWeaponEntity( );
			SpawnFromSpawnSpot( );
			spectator = entityNumber;
			forceRespawn = true;
			assert( spectating );
		}
	} else {
 		SetupWeaponEntity( );
		SpawnFromSpawnSpot( );
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
	
	if ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) {
		hiddenWeapon = true;
		if ( weapon ) {
			weapon->LowerWeapon();
		}
// RAVEN BEGIN
// mekberg: set to blaster now and disable the weapon.
		idealWeapon = SlotForWeapon ( "weapon_blaster" ); 
		Event_DisableWeapon( );
// RAVEN END
	} else {
		hiddenWeapon = false;
	}
	
	if ( hud ) {
		UpdateHudWeapon( );
		hud->StateChanged( gameLocal.time );
	}

	tipUp = false;
	objectiveUp = false;

	aiManager.AddTeammate ( this );

	if ( inventory.levelTriggers.Num() ) {
		PostEventMS( &EV_Player_LevelTrigger, 0 );
	}

	// ddynerman: defaults for these values are the single player fall deltas
	fatalFallDelta = spawnArgs.GetFloat("fatal_fall_delta", "65");
	hardFallDelta = spawnArgs.GetFloat("hard_fall_delta", "45");
	softFallDelta = spawnArgs.GetFloat("soft_fall_delta", "30");
	noFallDelta = spawnArgs.GetFloat("no_fall_delta", "7");

	// precache decls
	declManager->FindType( DECL_ENTITYDEF, "damage_fatalfall", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_hardfall", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_softfall", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_noair", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_suicide", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_telefrag", false, false );
	declManager->FindType( DECL_ENTITYDEF, "dmg_shellshock", false, false );
	declManager->FindType( DECL_ENTITYDEF, "dmg_shellshock_nohl", false, false );

	gibSkin = declManager->FindSkin( spawnArgs.GetString( "skin_gibskin" ) );

	// Skil levels
	dynamicProtectionScale = 1.0f;
	if ( !gameLocal.isMultiplayer ) {
		if ( g_skill.GetInteger() < 2 ) {
			if ( health < 25 ) {
				health = 25;
			}
		} else {
			//g_armorProtection.SetFloat( ( g_skill.GetInteger() < 2 ) ? 0.4f : 0.2f );
		}
	}
	
	// Powerup joints?
	if ( spawnArgs.GetString ( "powerup_effect_joints", "", temp ) ) {
		animator.GetJointList ( temp, powerupEffectJoints );
	}

	// RAVEN BEGIN
	// mekberg: allow disabling of objectives during non-cinematic time periods
	objectivesEnabled = true;

	// mekberg: new objectives are suppressed until this event is processed
	PostEventMS( &EV_Player_AllowNewObjectives, 5000 );
	tourneyStatus = PTS_UNKNOWN;

	predictionOriginError	= vec3_zero;
	predictionAnglesError	= ang_zero;

	// zero out view angles when we spawn ourselves in MP - the server will send down
	// the correct ones (only zero if our input is still zero'd)
	if( gameLocal.isClient && gameLocal.localClientNum == entityNumber && usercmd.angles[ 0 ] == 0 && usercmd.angles[ 1 ] == 0 && usercmd.angles[ 2 ] == 0 ) {
		deltaViewAngles = ang_zero;
	}
//RITUAL BEGIN
	carryOverCurrentWeapon = currentWeapon;
	inventory.carryOverWeapons = 0;
//RITUAL END

	itemCosts = static_cast< const idDeclEntityDef * >( declManager->FindType( DECL_ENTITYDEF, "ItemCostConstants", false ) );
}

/*
==============
idPlayer::~idPlayer()

Release any resources used by the player.
==============
*/
idPlayer::~idPlayer() {
	if( gameLocal.mpGame.GetGameState() ) {
		gameLocal.mpGame.GetGameState()->ClientDisconnect( this );
	}

	delete weaponViewModel;
	delete weaponWorldModel;
	delete weapon;
	delete aasSensor;
	
	SetPhysics( NULL );
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
	savefile->WriteInt ( godmodeDamage );	
	savefile->WriteBool( undying );

	// don't save spawnAnglesSet, since we'll have to reset them after loading the savegame
	savefile->WriteAngles( spawnAngles );
	savefile->WriteAngles( viewAngles );
	savefile->WriteAngles( cmdAngles );

	savefile->WriteInt( buttonMask );
	savefile->WriteInt( oldButtons );
 	savefile->WriteInt( oldFlags );

	savefile->WriteInt( lastHitTime );
 	savefile->WriteInt( 0 );
	savefile->WriteInt( lastSavingThrowTime );

	// idBoolFields don't need to be saved, just re-linked in Restore
	savefile->Write( &pfl, sizeof( pfl ) );

	inventory.Save( savefile );
	
	//weapon->Save( savefile );								// Don't save this

	weaponViewModel.Save( savefile );
	weaponWorldModel.Save ( savefile );
	// weaponDef restore = weaponDef = GetWeaponDef ( currentWeapon );

 	savefile->WriteUserInterface( hud, false );
//	savefile->WriteUserInterface( mphud, false );			// Don't save MP stuff
 	savefile->WriteUserInterface( objectiveSystem, false );
	savefile->WriteUserInterface( cinematicHud, false );
	savefile->WriteBool( objectiveSystemOpen );
	savefile->WriteBool( disableHud );

	savefile->WriteInt( lastDmgTime );
	savefile->WriteInt( deathClearContentsTime );
 	savefile->WriteBool( doingDeathSkin );
 	savefile->WriteInt( nextHealthPulse );
 	savefile->WriteInt( nextArmorPulse );
 	savefile->WriteBool( hiddenWeapon );

//	savefile->WriteInt( spectator );						// Don't save MP stuff

//	savefile->WriteBool( scoreBoardOpen );					// Don't save MP stuff
//	savefile->WriteBool( tourneyBracketsOpen );				// Don't save MP stuff
//	savefile->WriteBool( forceScoreBoard );					// Don't save MP stuff
//	savefile->WriteBool( forceRespawn );					// Don't save MP stuff

//	savefile->WriteBool( spectating );						// Don't save MP stuff
//	savefile->WriteBool( lastHitToggle );					// Don't save MP stuff
//	savefile->WriteBool( forcedReady );						// Don't save MP stuff
//	savefile->WriteBool( wantSpectate );					// Don't save MP stuff

// 	savefile->WriteBool( weaponGone );						// Don't save MP stuff
// 	savefile->WriteBool( useInitialSpawns );				// Don't save MP stuff
// 	savefile->WriteBool( isLagged );						// Don't save MP stuff
// 	savefile->WriteBool( isChatting );						// Don't save MP stuff

// 	savefile->WriteInt( lastSpectateTeleport );				// Don't save MP stuff
// 	savefile->WriteInt( latchedTeam );						// Don't save MP stuff
// 	savefile->WriteInt( spawnedTime );						// Don't save MP stuff

//	teleportEntity.Save( savefile );						// Don't save MP stuff
// 	savefile->WriteInt( teleportKiller );					// Don't save MP stuff

	savefile->WriteInt( minRespawnTime );
	savefile->WriteInt( maxRespawnTime );

	savefile->WriteVec3( firstPersonViewOrigin );
	savefile->WriteMat3( firstPersonViewAxis );

	// don't bother saving dragEntity or aasSensor since it's a dev tool
	savefile->WriteVec3( intentDir );

	savefile->WriteFloat ( vehicleCameraDist );

	savefile->WriteJoint( hipJoint );
	savefile->WriteJoint( chestJoint );
 	savefile->WriteJoint( headJoint );

	savefile->WriteStaticObject( physicsObj );

 	savefile->WriteInt( aasLocation.Num() );
 	for( i = 0; i < aasLocation.Num(); i++ ) {
 		savefile->WriteInt( aasLocation[ i ].areaNum );
 		savefile->WriteVec3( aasLocation[ i ].pos );
 	}

	savefile->WriteString( modelName );	// cnicholson: Added unsaved var
	// TOSAVE: const idDict*			modelDict
    
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

	savefile->WriteFloat( fatalFallDelta );
	savefile->WriteFloat( hardFallDelta );
	savefile->WriteFloat( softFallDelta );
	savefile->WriteFloat( noFallDelta );

	savefile->WriteInt( currentWeapon );
	savefile->WriteInt( idealWeapon );
	savefile->WriteInt( previousWeapon );
 	savefile->WriteInt( weaponSwitchTime );
	savefile->WriteBool( weaponEnabled );

	savefile->WriteBool ( flashlightOn);
	savefile->WriteBool ( zoomed );

	savefile->WriteBool( reloadModel );

	savefile->WriteSkin( skin );
	savefile->WriteSkin( powerUpSkin );
	savefile->WriteSkin( gibSkin );

	savefile->WriteInt( numProjectilesFired );
	savefile->WriteInt( numProjectileHits );

 	savefile->WriteBool( airless );
	savefile->WriteInt( airTics );
	savefile->WriteInt( lastAirDamage );

	savefile->WriteBool( gibDeath );
	savefile->WriteBool( gibsLaunched );
	savefile->WriteVec3( gibDir );

	savefile->WriteBool( isStrogg );

	savefile->WriteInterpolate( zoomFov );
	savefile->WriteInterpolate( centerView );
 	savefile->WriteBool( fxFov );

	savefile->WriteFloat( influenceFov );
	savefile->WriteInt( influenceActive );
	savefile->WriteObject( influenceEntity );
	savefile->WriteMaterial( influenceMaterial );
	savefile->WriteFloat( influenceRadius );
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

	savefile->WriteUserInterface( focusUI, false );
	savefile->WriteInt( focusTime );
	savefile->WriteInt ( focusType );
	focusEnt.Save ( savefile );
	savefile->WriteUserInterface( focusBrackets, false );
	savefile->WriteInt( focusBracketsTime );

 	talkingNPC.Save( savefile );

	extraProjPassEntity.Save( savefile );

	savefile->WriteInt( talkCursor );
	savefile->WriteUserInterface( cursor, false );

	savefile->WriteUserInterface( overlayHud, false );
	savefile->WriteInt ( overlayHudTime );

	savefile->WriteBool( targetFriendly );

	savefile->WriteInt( oldMouseX );
	savefile->WriteInt( oldMouseY );

 	savefile->WriteBool( tipUp );
 	savefile->WriteBool( objectiveUp );

	savefile->WriteFloat( dynamicProtectionScale );
	savefile->WriteInt( lastDamageDef );
	savefile->WriteVec3( lastDamageDir );
	savefile->WriteInt( lastDamageLocation );

	savefile->WriteInt( predictedFrame );
	savefile->WriteVec3( predictedOrigin );
	savefile->WriteAngles( predictedAngles );
	savefile->WriteBool( predictedUpdated );
	savefile->WriteVec3( predictionOriginError );
	savefile->WriteAngles( predictionAnglesError );
	savefile->WriteInt( predictionErrorTime );

//	savefile->WriteBool( ready );					// Don't save MP stuff
// 	savefile->WriteBool( respawning );				// Don't save MP stuff
// 	savefile->WriteBool( leader );					// Don't save MP stuff
// 	savefile->WriteBool( weaponCatchup );			// Don't save MP stuff
//	savefile->WriteBool( isTelefragged );			// Don't save MP stuff

//	savefile->WriteInt( lastSpectateChange );		// Don't save MP stuff
// 	savefile->WriteInt( lastTeleFX );				// Don't save MP stuff
//	savefile->WriteInt( lastSnapshotSequence );		// Don't save MP stuff

//	savefile->WriteInt( aimClientNum );				// Don't save MP stuff

//	lastImpulsePlayer->Save( savefile );			// Don't save MP stuff

//	savefile->WriteInt( arena );					// Don't save MP stuff

//	savefile->WriteInt( connectTime );				// Don't save MP stuff
//	savefile->WriteInt( mutedPlayers );				// Don't save MP stuff
//	savefile->WriteInt( friendPlayers );			// Don't save MP stuff

//	savefile->WriteInt( rank );						// Don't save MP stuff

	savefile->WriteInt( lastImpulseTime );
	bossEnemy.Save( savefile );						// cnicholson: Added unsaved var

	// TOSAVE: const idDeclEntityDef*	cachedWeaponDefs [ MAX_WEAPONS ];	// cnicholson: Save these?
	// TOSAVE: const idDeclEntityDef*	cachedPowerupDefs [ POWERUP_MAX ];

	savefile->WriteBool( weaponChangeIconsUp );		// cnicholson: Added unsaved var

	// mekberg: added
	savefile->WriteBool( showNewObjectives );
	savefile->WriteBool( objectivesEnabled );

	savefile->WriteBool( flagCanFire );
	
	// TOSAVE: const idDeclEntityDef*	cachedWeaponDefs [ MAX_WEAPONS ];	// cnicholson: Save these?
	// TOSAVE: const idDeclEntityDef*	cachedPowerupDefs [ POWERUP_MAX ];

#ifndef _XENON
 	if ( hud ) {
		hud->SetStateString( "message", common->GetLocalizedString( "#str_102916" ) );
		hud->HandleNamedEvent( "Message" );
 	}
#endif
}

/*
===========
idPlayer::Restore
===========
*/
void idPlayer::Restore( idRestoreGame *savefile ) {
	int	  i;
	int   num;

	savefile->ReadUsercmd( usercmd );

	playerView.Restore( savefile );

	savefile->ReadBool( noclip );
	savefile->ReadBool( godmode );
	savefile->ReadInt ( godmodeDamage );	
	savefile->ReadBool( undying );

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
	int foo;
 	savefile->ReadInt( foo );
	savefile->ReadInt( lastSavingThrowTime );

	savefile->Read( &pfl, sizeof( pfl ) );

	inventory.Restore( savefile );

	assert( !weapon );

	weaponViewModel.Restore( savefile );	
	weaponWorldModel.Restore( savefile );	

 	savefile->ReadUserInterface( hud, &spawnArgs );	
	assert( !mphud );									// Don't save MP stuff
	savefile->ReadUserInterface( objectiveSystem, &spawnArgs );
	savefile->ReadUserInterface( cinematicHud, &spawnArgs );
	savefile->ReadBool( objectiveSystemOpen );

#ifdef _XENON
	g_ObjectiveSystemOpen = objectiveSystemOpen;
#endif

	objectiveButtonReleased = false;
	savefile->ReadBool( disableHud );					// cnicholson: Added unrestored var

	savefile->ReadInt( lastDmgTime );
	savefile->ReadInt( deathClearContentsTime );
 	savefile->ReadBool( doingDeathSkin );
 	savefile->ReadInt( nextHealthPulse );
 	savefile->ReadInt( nextArmorPulse );
 	savefile->ReadBool( hiddenWeapon );

	assert( !spectator );								// Don't save MP stuff

	assert( !scoreBoardOpen );							// Don't save MP stuff
	assert( !forceScoreBoard );							// Don't save MP stuff
	assert( !forceRespawn );							// Don't save MP stuff

	assert( !spectating );								// Don't save MP stuff
	assert( !lastHitToggle );							// Don't save MP stuff
	assert( !forcedReady );								// Don't save MP stuff
	assert( !wantSpectate );							// Don't save MP stuff

 	assert( !weaponGone );								// Don't save MP stuff
 	assert( !useInitialSpawns );						// Don't save MP stuff
 	assert( !isLagged );								// Don't save MP stuff
 	assert( !isChatting );								// Don't save MP stuff

	assert( !lastSpectateTeleport );					// Don't save MP stuff
 	assert( latchedTeam == -1 );						// Don't save MP stuff
 	assert( !spawnedTime );								// Don't save MP stuff

	assert( !teleportEntity );							// Don't save MP stuff
 	assert( teleportKiller == -1 );						// Don't save MP stuff

	savefile->ReadInt( minRespawnTime );
	savefile->ReadInt( maxRespawnTime );

	savefile->ReadVec3( firstPersonViewOrigin );
	savefile->ReadMat3( firstPersonViewAxis );

	// don't bother restoring dragEntity since it's a dev tool
 	dragEntity.Clear();
	savefile->ReadVec3( intentDir );

	savefile->ReadFloat ( vehicleCameraDist );

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

	savefile->ReadString( modelName );	// cnicholson: Added unrestored var
	// TORESTORE: const idDict*			modelDict

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

	savefile->ReadFloat( fatalFallDelta );
	savefile->ReadFloat( hardFallDelta );
	savefile->ReadFloat( softFallDelta );
	savefile->ReadFloat( noFallDelta );

	savefile->ReadInt( currentWeapon );
	savefile->ReadInt( idealWeapon );
	savefile->ReadInt( previousWeapon );
 	savefile->ReadInt( weaponSwitchTime );
	savefile->ReadBool( weaponEnabled );

	savefile->ReadBool ( flashlightOn );
	savefile->ReadBool ( zoomed );

	savefile->ReadBool ( reloadModel );

	savefile->ReadSkin( skin );
	savefile->ReadSkin( powerUpSkin );
	savefile->ReadSkin( gibSkin );

	savefile->ReadInt( numProjectilesFired );
	savefile->ReadInt( numProjectileHits );

 	savefile->ReadBool( airless );
	savefile->ReadInt( airTics );
	savefile->ReadInt( lastAirDamage );

	savefile->ReadBool( gibDeath );
	savefile->ReadBool( gibsLaunched );
	savefile->ReadVec3( gibDir );

	savefile->ReadBool( isStrogg );

	savefile->ReadInterpolate( zoomFov );
	savefile->ReadInterpolate( centerView );
	savefile->ReadBool( fxFov );

	savefile->ReadFloat( influenceFov );
	savefile->ReadInt( influenceActive );
	savefile->ReadObject( reinterpret_cast<idClass *&>( influenceEntity ) );
	savefile->ReadMaterial( influenceMaterial );
	savefile->ReadFloat( influenceRadius );
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

	savefile->ReadUserInterface( focusUI, &spawnArgs ),
	savefile->ReadInt( focusTime );
	savefile->ReadInt( (int&)focusType );
	focusEnt.Restore ( savefile );
	savefile->ReadUserInterface( focusBrackets, &spawnArgs );
	savefile->ReadInt( focusBracketsTime );
	
	talkingNPC.Restore( savefile );

	extraProjPassEntity.Restore( savefile );

	savefile->ReadInt( talkCursor );
 	savefile->ReadUserInterface( cursor, &spawnArgs );

	savefile->ReadUserInterface( overlayHud, &spawnArgs );		// cnicholson: Added unrestored var
	savefile->ReadInt ( overlayHudTime );			// cnicholson: Added unrestored var

	savefile->ReadBool( targetFriendly );

	savefile->ReadInt( oldMouseX );
	savefile->ReadInt( oldMouseY );

 	savefile->ReadBool( tipUp );
 	savefile->ReadBool( objectiveUp );

	savefile->ReadFloat( dynamicProtectionScale );	// cnicholson: Added unrestored var
	savefile->ReadInt( lastDamageDef );
	savefile->ReadVec3( lastDamageDir );
	savefile->ReadInt( lastDamageLocation );

	savefile->ReadInt( predictedFrame );
	savefile->ReadVec3( predictedOrigin );
	savefile->ReadAngles( predictedAngles );
	savefile->ReadBool( predictedUpdated );
	savefile->ReadVec3( predictionOriginError );
	savefile->ReadAngles( predictionAnglesError );
	savefile->ReadInt( predictionErrorTime );

	assert( !ready );								// Don't save MP stuff
 	assert( !respawning );							// Don't save MP stuff
 	assert( !leader );								// Don't save MP stuff
 	assert( !weaponCatchup );						// Don't save MP stuff
	assert( !isTelefragged );						// Don't save MP stuff

	assert( !lastSpectateChange );					// Don't save MP stuff
 	assert( lastTeleFX == -9999 );					// Don't save MP stuff
 	assert( !lastSnapshotSequence );				// Don't save MP stuff

 	assert( aimClientNum == -1 );					// Don't save MP stuff

	assert( !lastImpulsePlayer );					// Don't save MP stuff

	assert( !arena );								// Don't save MP stuff

	assert( !connectTime );							// Don't save MP stuff
	assert( !mutedPlayers );						// Don't save MP stuff
	assert( !friendPlayers );						// Don't save MP stuff

	assert( rank == -1 );							// Don't save MP stuff

	savefile->ReadInt( lastImpulseTime );
	bossEnemy.Restore( savefile );					// cnicholson: Added unrestored var

	// TORESTORE: const idDeclEntityDef*	cachedWeaponDefs [ MAX_WEAPONS ];	// cnicholson: Save these?
	// TORESTORE: const idDeclEntityDef*	cachedPowerupDefs [ POWERUP_MAX ];

	savefile->ReadBool( weaponChangeIconsUp );		// cnicholson: Added unrestored var

// mekberg: added
	savefile->ReadBool( showNewObjectives );
	savefile->ReadBool( objectivesEnabled );

	savefile->ReadBool( flagCanFire );

	// set the pm_ cvars
	const idKeyValue	*kv;
	kv = spawnArgs.MatchPrefix( "pm_", NULL );
	while( kv ) {
		cvarSystem->SetCVarString( kv->GetKey(), kv->GetValue() );
		kv = spawnArgs.MatchPrefix( "pm_", kv );
	}

	// Loading a game on easy mode ensures you alwasy have 20% health when you load
	i = spawnArgs.GetInt ( va("minRestoreHealth%d", g_skill.GetInteger ( ) ) );
	if ( health < i ) {
		health = i;
	}

	//if there's hearing loss, make sure we post a finishing event
	if( pfl.hearingLoss )	{
		Event_FinishHearingLoss( 3.0f );
	} else {
		Event_FinishHearingLoss( 0.05f );
	}
	// create combat collision hull for exact collision detection
	SetCombatModel();	

// RAVEN BEGIN
// mekberg: Grab from user info.
 	showWeaponViewModel = GetUserInfo()->GetBool( "ui_showGun" );

	// precache decls
	declManager->FindType( DECL_ENTITYDEF, "damage_fatalfall", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_hardfall", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_softfall", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_noair", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_suicide", false, false );
	declManager->FindType( DECL_ENTITYDEF, "damage_telefrag", false, false );
	declManager->FindType( DECL_ENTITYDEF, "dmg_shellshock", false, false );
	declManager->FindType( DECL_ENTITYDEF, "dmg_shellshock_nohl", false, false );
// RAVEN END
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
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	allowedToRespawn = true;
// RITUAL END
	
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
		// clear the existing model to force a reload
		Init();
	} else {
		// choose a random spot and prepare the point of view in case player is left spectating
		assert( spectating );
		SpawnFromSpawnSpot();
	}

	lastKiller = NULL;
	useInitialSpawns = true;
}

/*
===============
idPlayer::ServerSpectate
================
*/
void idPlayer::ServerSpectate( bool spectate ) {
	assert( !gameLocal.isClient );

	if ( spectating != spectate ) {
		// if we select spectating on the client 
		// mekberg: drop and clear powerups from the player.
		DropPowerups();
		ClearPowerUps();
 		Spectate( spectate );
		gameLocal.mpGame.GetGameState()->Spectate( this );
   		if ( spectate ) {
  			SetSpectateOrigin( );
 		} 
	}
	if ( !spectate ) {
		SpawnFromSpawnSpot( );
	}
}

/*
===========
idPlayer::SelectSpawnPoint

Find a spawn point marked, otherwise use normal spawn selection.
============
*/
bool idPlayer::SelectSpawnPoint( idVec3 &origin, idAngles &angles ) {
	idEntity *spot;
	idStr skin;

	spot = gameLocal.SelectSpawnPoint( this );

	// no spot, try again next frame
	if( !spot ) {
		forceRespawn = true;
		return false;
	}

	// set the player skin from the spawn location
	if ( spot->spawnArgs.GetString( "skin", NULL, skin ) ) {
		spawnArgs.Set( "spawn_skin", skin );
	}

	if ( spot->spawnArgs.GetString( "spawn_model", NULL ) ) {
		spawnArgs.Set( "model", spot->spawnArgs.GetString( "spawn_model", NULL ) );
	}

	if ( spot->spawnArgs.GetString( "def_head", NULL ) ) {
		spawnArgs.Set( "def_head", spot->spawnArgs.GetString( "def_head", NULL ) );
	}

	// activate the spawn locations targets
	spot->PostEventMS( &EV_ActivateTargets, 0, this );

	origin = spot->GetPhysics()->GetOrigin();
	origin[2] += 4.0f + CM_BOX_EPSILON;		// move up to make sure the player is at least an epsilon above the floor
	angles = spot->GetPhysics()->GetAxis().ToAngles();

	return true;
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
	
	if( !SelectSpawnPoint( spawn_origin, spawn_angles ) ) {
		forceRespawn = true;
		return;
	}
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

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	if ( gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() ) {
		// Record previous weapons for later restoration
		inventory.carryOverWeapons &= ~CARRYOVER_WEAPONS_MASK;
		inventory.carryOverWeapons |= inventory.weapons;
	}
// RITUAL END

	respawning = true;

	Init();

	// Force players to use bounding boxes when in multiplayer
	if ( gameLocal.isMultiplayer ) {
		use_combat_bbox = true;

		// Make sure the combat model is unlinked
		if ( combatModel ) {
			combatModel->Unlink( );
		}
	}

	// Any health over max health will tick down
	if ( health > inventory.maxHealth ) {
		nextHealthPulse = gameLocal.time + HEALTH_PULSE;
	}
	
	if ( inventory.armor > inventory.maxarmor ) {
		nextArmorPulse = gameLocal.time + ARMOR_PULSE;
	}		

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
// RAVEN BEGIN
// abahr: taking into account gravity
		SetAxis( spawn_angles.ToMat3() );
// RAVEN END
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
				// currentThinkingEntity not set here (called out of Run())
				idEntity* thinker = gameLocal.currentThinkingEntity;
				gameLocal.currentThinkingEntity = this;
				gameLocal.PlayEffect( spawnArgs, "fx_spawn", renderEntity.origin, idVec3(0,0,1).ToMat3(), false, vec3_origin, true );
				lastTeleFX = gameLocal.time;
				gameLocal.currentThinkingEntity = thinker;
 			}
		}
//RAVEN BEGIN
//asalmon: Clear the respwan message
		if ( mphud ) {
					mphud->SetStateInt( "respawnNotice", 0 );
		}
//RAVEN END
		pfl.teleport = true;
	} else {
		pfl.teleport = false;
	}

	// kill anything at the new position
	if ( !spectating ) {
		physicsObj.SetClipMask( MASK_PLAYERSOLID ); // the clip mask is usually maintained in Move(), but KillBox requires it
// RAVEN BEGIN
// abahr: this is killing the tram car when spawning in.  Ooooops!
		if( gameLocal.isMultiplayer ) {
			gameLocal.KillBox( this );
		}
// RAVEN END
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

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
	if( gameLocal.isMultiplayer && gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() )
	{
		// restore previous weapons
		inventory.weapons |= inventory.carryOverWeapons & CARRYOVER_WEAPONS_MASK;
		for( int weaponIndex = 0; weaponIndex < MAX_WEAPONS; weaponIndex++ )
		{
			if( inventory.weapons & ( 1 << weaponIndex ) )
			{
				int ammoIndex	= inventory.AmmoIndexForWeaponIndex( weaponIndex );
				inventory.ammo[ ammoIndex ] = inventory.StartingAmmoForWeaponIndex( weaponIndex );
			}
		}

		/// Restore armor purchased while dead
		if( inventory.carryOverWeapons & CARRYOVER_FLAG_ARMOR_LIGHT )
		{
			inventory.carryOverWeapons &= ~CARRYOVER_FLAG_ARMOR_LIGHT;
			GiveItem( "item_armor_small" );
		}

		if( inventory.carryOverWeapons & CARRYOVER_FLAG_ARMOR_HEAVY )
		{
			inventory.carryOverWeapons &= ~CARRYOVER_FLAG_ARMOR_HEAVY;
			GiveItem( "item_armor_large" );
		}

		if( inventory.carryOverWeapons & CARRYOVER_FLAG_AMMO )
		{
			inventory.carryOverWeapons &= ~CARRYOVER_FLAG_AMMO;
			GiveItem( "ammorefill" );
		}

		// Reactivate team powerups
		gameLocal.mpGame.SetUpdateForTeamPowerups(team);
		UpdateTeamPowerups();
	}
// RITUAL END

	BecomeActive( TH_THINK );

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	Think();

 	respawning			= false;
 	isTelefragged		= false;
 	isLagged			= false;
 	isChatting			= false;

	lastImpulsePlayer = NULL;
	lastImpulseTime = 0;
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
 		idealWeapon = spawnArgs.GetInt( "current_weapon", "0" );
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
idPlayer::UpdateModelSetup
Updates the player's model setup.  Model setups are read from the player def, and contain
information about the player's model, the player's head model, a skin, and a team for the
composite model.
==============
*/
void idPlayer::UpdateModelSetup( bool forceReload ) {
	const idDict* dict;
	const char* uiKeyName = NULL;
	const char* defaultModel = NULL;
	const char* newModelName = NULL;

	if( !gameLocal.isMultiplayer || spectating ) {
		return;
	}

	if( gameLocal.IsTeamGame() ) {
		defaultModel = spawnArgs.GetString( va( "def_default_model_%s", idMultiplayerGame::teamNames[ team ] ), NULL );

		if( g_forceMarineModel.GetString()[ 0 ] && team == TEAM_MARINE ) {
			newModelName = g_forceMarineModel.GetString();
		} else if( g_forceStroggModel.GetString()[ 0 ] && team == TEAM_STROGG ) {
			newModelName = g_forceStroggModel.GetString();
		} else {
			uiKeyName = va( "ui_model_%s", idMultiplayerGame::teamNames[ team ] );
			newModelName = GetUserInfo()->GetString( uiKeyName );
		}
	} else {
		defaultModel = spawnArgs.GetString( "def_default_model" );

		if( g_forceModel.GetString()[ 0 ] ) {
			newModelName = g_forceModel.GetString();
		} else {
			uiKeyName = "ui_model";
			newModelName = GetUserInfo()->GetString( uiKeyName );
		}	
	}

	if( !idStr::Icmp( newModelName, "" ) ) {
		newModelName = defaultModel;
	}

	// model hasn't changed
	if( !modelName.Icmp( newModelName ) && !forceReload ) {
		return;
	}

	rvDeclPlayerModel* model = (rvDeclPlayerModel*)declManager->FindType( DECL_PLAYER_MODEL, newModelName, false );

	// validate that the model they've selected is OK for this team game
	if( gameLocal.IsTeamGame() && model ) {
		if( idStr::Icmp( model->team, idMultiplayerGame::teamNames[ team ] ) ) {
			gameLocal.Warning( "idPlayer::UpdateModelSetup() - Player %d (%s) set to model %s which is restricted to team %s (Player team: %s)\n", entityNumber, GetUserInfo()->GetString( "ui_name" ), newModelName, model->team.c_str(), idMultiplayerGame::teamNames[ team ] );
			if( uiKeyName ) {
				cvarSystem->SetCVarBool( uiKeyName, "" );
			}

			dict = NULL;
		}
	} 

	// check to see if the user-specified ui_model/ui_model_strogg/ui_model_marine is valid
	if( !model ) {
		newModelName = defaultModel;

		model = (rvDeclPlayerModel*)declManager->FindType( DECL_PLAYER_MODEL, newModelName, false );
		if( !model ) {
			gameLocal.Error( "idPlayer::UpdateModelSetup() - Can't find default model (%s)\n", defaultModel );
		} else {
			// If it's not valid, set the cvar to the default model
			if( uiKeyName ) {
				GetUserInfo()->Set( uiKeyName, defaultModel );

				if( entityNumber == gameLocal.localClientNum ) {
					cvarSystem->SetCVarString( uiKeyName, defaultModel );
				}

				if( gameLocal.isServer ) {
					cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "updateUI %d\n", entityNumber ) );
					return;
				}
			}

		}
	}

	modelName = newModelName;
	modelDecl = model;

	reloadModel = true;

	// if we have a strogg model, set the strogg flag here
	isStrogg = false;
	if( modelDecl && !modelDecl->team.Icmp( "strogg" ) ) {
		isStrogg = true;
	}
}

/*
==============
idPlayer::BalanceTeam
==============
*/
bool idPlayer::BalanceTeam( void ) {
	int			i, balanceTeam, teamCount[2];
	idEntity	*ent;

	teamCount[ 0 ] = teamCount[ 1 ] = 0;
	for( i = 0; i < gameLocal.numClients; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPlayer::Type ) && gameLocal.mpGame.IsInGame( i ) ) {
			if ( !static_cast< idPlayer * >( ent )->spectating ) {
				teamCount[ static_cast< idPlayer * >( ent )->team ]++;
			}
		}
	}
	
	balanceTeam = -1;
	if ( teamCount[ 0 ] < teamCount[ 1 ] ) {
		balanceTeam = 0;
	} else if ( teamCount[ 0 ] > teamCount[ 1 ] ) {
		balanceTeam = 1;
	}
	
	if ( balanceTeam != -1 && team != balanceTeam && teamCount[ balanceTeam ]+1 != teamCount[ !balanceTeam ] ) {
		common->DPrintf( "team balance: forcing player %d to %s team\n", entityNumber, balanceTeam ? "strogg" : "marine" );
		team = balanceTeam;
		GetUserInfo()->Set( "ui_team", team ? "Strogg" : "Marine" );
		return true;
	}
	return false;
}

void HSVtoRGB( float &r, float &g, float &b, float h, float s, float v ) {
	int i;
	float f, p, q, t;

	h = idMath::ClampFloat( 0.0f, 360.0f, h );
	s = idMath::ClampFloat( 0.0f, 1.0f, s );
	v = idMath::ClampFloat( 0.75f, 1.0f, v );

	if( s == 0 ) {
		// achromatic (grey)
		r = g = b = v;
		return;
	}

	h /= 60;			// sector 0 to 5
	i = floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );

	switch( i ) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;

		case 1:
			r = q;
			g = v;
			b = p;
			break;

		case 2:
			r = p;
			g = v;
			b = t;
			break;
			
		case 3:
			r = p;
			g = q;
			b = v;
			break;

		case 4:
			r = t;
			g = p;
			b = v;
			break;

		default:		// case 5:
			r = v;
			g = p;
			b = q;
			break;
	}
}

/*
==============
idPlayer::UserInfoChanged
==============
*/
bool idPlayer::UserInfoChanged( void ) {
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
 		if ( gameLocal.mpGame.GetGameState()->GetMPGameState() == SUDDENDEATH && !spec && wantSpectate == true ) {
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
 		if ( spec ) {
 			userInfo->Set( "ui_spectate", "Play" );
 			modifiedInfo |= true;
 		} else if ( spectating ) {  
 			// allow player to leaving spectator mode if they were in it when it was disallowed
 			forceRespawn = true;
 		}
		wantSpectate = false;
	}

	if ( gameLocal.serverInfo.GetBool( "si_useReady" ) ) {
		newready = ( idStr::Icmp( userInfo->GetString( "ui_ready" ), "Ready" ) == 0 );
		if ( ready != newready && gameLocal.mpGame.GetGameState()->GetMPGameState() == WARMUP && !wantSpectate ) {
 			gameLocal.mpGame.AddChatLine( common->GetLocalizedString( "#str_107180" ), userInfo->GetString( "ui_name" ), newready ? common->GetLocalizedString( "#str_104300" ) : common->GetLocalizedString( "#str_104301" ) );
		}
		ready = newready;
	}

	int newTeam = ( idStr::Icmp( userInfo->GetString( "ui_team" ), "Strogg" ) == 0 );

	if( hud && gameLocal.IsTeamGame() ) {
		hud->HandleNamedEvent( (team ? "setTeam_strogg" : "setTeam_marine") );
	} else if( hud ) {
		hud->HandleNamedEvent( "setTeam_marine" );
	}

	if ( gameLocal.IsTeamGame() && newTeam != latchedTeam ) {
		team = newTeam;

		if ( gameLocal.isServer ) {
			int verifyTeam = gameLocal.mpGame.VerifyTeamSwitch( newTeam, this );
			if( verifyTeam != newTeam ) {
				if( verifyTeam == TEAM_MARINE || verifyTeam == TEAM_STROGG ) {
					gameLocal.userInfo[ entityNumber ].Set( "ui_team", gameLocal.mpGame.teamNames[ verifyTeam ] );
					if( gameLocal.localClientNum == entityNumber ) {
						cvarSystem->SetCVarString( "ui_team", gameLocal.mpGame.teamNames[ verifyTeam ] );
					}
					modifiedInfo = true;
					team = verifyTeam;
				}
			}
		}

		// if still OK to change
		if( team != latchedTeam ) {
			if( gameLocal.isServer ) {
				gameLocal.mpGame.SwitchToTeam( entityNumber, latchedTeam, team );						
			}

			SetInitialHud();
			if( mphud ) {
				mphud->SetStateInt( "playerteam", team );
				mphud->HandleNamedEvent( "TeamChange" );
			}

			if( entityNumber == gameLocal.localClientNum ) {
				alreadyDidTeamAnnouncerSound = true;
				// the client might set its team to a value before the server corrects for team balance
				gameLocal.mpGame.RemoveAnnouncerSound( AS_TEAM_JOIN_MARINE );
				gameLocal.mpGame.RemoveAnnouncerSound( AS_TEAM_JOIN_STROGG );
				
				if ( !wantSpectate ) {				
					if( team == TEAM_STROGG ) {
						gameLocal.mpGame.ScheduleAnnouncerSound( AS_TEAM_JOIN_STROGG, gameLocal.time + 500 );
					} else if( team == TEAM_MARINE ) {
						gameLocal.mpGame.ScheduleAnnouncerSound( AS_TEAM_JOIN_MARINE, gameLocal.time + 500 );
					}
				}
			}

			// ATVI DevTrack #13224 - update on each team change
			iconManager->UpdateTeamIcons();

			latchedTeam = team;
		}
	}

	//if ( !gameLocal.isClient && gameLocal.serverInfo.GetBool( "si_autoBalance" ) && gameLocal.IsTeamGame() ) {
	//	bool teamsBalanced = BalanceTeam();
 	//	modifiedInfo |= teamsBalanced;
	//}

	UpdateModelSetup();

	if( (gameLocal.isServer && gameLocal.mpGame.IsInGame( entityNumber )) || (gameLocal.isClient && gameLocal.localClientNum == entityNumber) ) {
 		isChatting = userInfo->GetBool( "ui_chat", "0" );
 		if ( isChatting && pfl.dead ) {
 			// if dead, always force chat icon off.
 			isChatting = false;
 			userInfo->SetBool( "ui_chat", false );
 			modifiedInfo |= true;
		}
	}

	// grab hitscan tint
	hitscanTint = userInfo->GetVec4( "ui_hitscanTint", "81 1 1 1" );
	HSVtoRGB( hitscanTint.x, hitscanTint.y, hitscanTint.z, hitscanTint.x, hitscanTint.y, hitscanTint.z );
	// force alpha to 1
	hitscanTint.w = 1.0f;

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

	assert( weapon );
	assert( _hud );

	inclip		= weapon->AmmoInClip();
	ammoamount	= weapon->AmmoAvailable();

	if ( ammoamount < 0 ) {
		// show infinite ammo
		_hud->SetStateString( "player_ammo", "-1" );
		_hud->SetStateString( "player_totalammo", "-1" );
		_hud->SetStateFloat ( "player_ammopct", 1.0f );
	} else if ( weapon->ClipSize ( ) && !gameLocal.isMultiplayer ) {
		_hud->SetStateInt ( "player_clip_size", weapon->ClipSize() );
		_hud->SetStateFloat ( "player_ammopct", (float)inclip / (float)weapon->ClipSize ( ) );
		if ( weapon->ClipSize ( )==1) {
			_hud->SetStateInt ( "player_totalammo", ammoamount );
		}
		else {
			_hud->SetStateInt ( "player_totalammo", ammoamount - inclip );
		}
		_hud->SetStateInt ( "player_ammo", inclip );
	} else {
		_hud->SetStateFloat ( "player_ammopct", (float)ammoamount / (float)weapon->maxAmmo );
		_hud->SetStateInt ( "player_totalammo", ammoamount );
		_hud->SetStateInt ( "player_ammo", -1 );
	} 

	_hud->SetStateBool( "player_ammo_empty", ( ammoamount == 0 ) );
}

/*
===============
idPlayer::UpdateHudStats
===============
*/
void idPlayer::UpdateHudStats( idUserInterface *_hud ) {
	int temp;
	
	assert ( _hud );

	temp = _hud->State().GetInt ( "player_health", "-1" );
	if ( temp != health ) {		
		_hud->SetStateInt   ( "player_healthDelta", temp == -1 ? 0 : (temp - health) );
		_hud->SetStateInt	( "player_health", health < -100 ? -100 : health );
		_hud->SetStateFloat	( "player_healthpct", idMath::ClampFloat ( 0.0f, 1.0f, (float)health / (float)inventory.maxHealth ) );
		_hud->HandleNamedEvent ( "updateHealth" );
	}
		
	temp = _hud->State().GetInt ( "player_armor", "-1" );
	if ( temp != inventory.armor ) {
		_hud->SetStateInt ( "player_armorDelta", temp == -1 ? 0 : (temp - inventory.armor) );
		_hud->SetStateInt ( "player_armor", inventory.armor );
		_hud->SetStateFloat	( "player_armorpct", idMath::ClampFloat ( 0.0f, 1.0f, (float)inventory.armor / (float)inventory.maxarmor ) );
		_hud->HandleNamedEvent ( "updateArmor" );
	}
	
	// Boss bar
	if ( _hud->State().GetInt ( "boss_health", "-1" ) != (bossEnemy ? bossEnemy->health : -1) ) {
		if ( !bossEnemy || bossEnemy->health <= 0 ) {
			bossEnemy = NULL;
			_hud->SetStateInt ( "boss_health", -1 );
			_hud->HandleNamedEvent ( "hideBossBar" );			
 			_hud->HandleNamedEvent ( "hideBossShieldBar" ); // grrr, for boss buddy..but maybe other bosses will have shields?
		} else {			
			_hud->SetStateInt ( "boss_health", bossEnemy->health );
			_hud->HandleNamedEvent ( "updateBossBar" );
		}
	}
		
	// god mode information
	_hud->SetStateString( "player_god", va( "%i", (godmode && g_showGodDamage.GetBool()) ) );
	_hud->SetStateString( "player_god_damage", va( "%i", godmodeDamage ) );

	// Update the hit direction
	idVec3 localDir;
	viewAxis.ProjectVector( lastDamageDir, localDir );
	_hud->SetStateFloat( "hitdir", localDir.ToAngles()[YAW] + 180.0f );

	//_hud->HandleNamedEvent( "updateArmorHealthAir" );

	if ( weapon ) {
		UpdateHudAmmo( _hud );
	}
	
	_hud->StateChanged( gameLocal.time );
}

/*
===============
idPlayer::UpdateHudWeapon
===============
*/
void idPlayer::UpdateHudWeapon( int displayWeapon ) {
	
	if ( (displayWeapon < -1) || (displayWeapon >= MAX_WEAPONS) ) {
		common->DPrintf( "displayweapon was out of range" );
		return;
	}
	
	int index = 0;
	int idealIndex = 0;
	idUserInterface * hud = idPlayer::hud;
	idUserInterface * mphud = idPlayer::mphud;
	idUserInterface * cursor = idPlayer::cursor;

	if ( !gameLocal.GetLocalPlayer() ) {
		// server netdemo
		if ( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() && gameLocal.GetDemoFollowClient() == entityNumber ) {
			hud = gameLocal.GetDemoHud();
			mphud = gameLocal.GetDemoMphud();
			cursor = gameLocal.GetDemoCursor();
		}
	} else {
		// if updating the hud of a followed client
		idPlayer *p = gameLocal.GetLocalPlayer();
		if ( p->spectating && p->spectator == entityNumber ) {
			assert( p->hud && p->mphud );
			hud = p->hud;
			mphud = p->mphud;
			cursor = p->GetCursorGUI();
		}
	}

	if ( !hud || !weapon ) {
		return;
	}

	for ( int i = 0; i < MAX_WEAPONS; i++ ) {
		const char *weapnum = va( "weapon%d", i );
		int weapstate = 0;
		if ( inventory.weapons & ( 1 << i ) && spawnArgs.GetBool( va( "weapon%d_cycle", i ) ) ) {
			hud->SetStateBool( weapnum, true );
			const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if ( weap && *weap ) {
				weapstate++;

				if ( idealWeapon == i ) {
					idealIndex = index;
				}

				const char *weaponIcon = GetWeaponDef ( i )->dict.GetString ( "inv_icon" );
				//const idMaterial *material = declManager->FindMaterial( weaponIcon );
				//if ( material ) {
				//	material->SetSort( SS_GUI );
				//}			
				
				hud->SetStateInt	( va("weapon%d_index", i ), index++ );
				hud->SetStateString ( va("weapon%d_icon", i ), weaponIcon );
				hud->SetStateInt	( va("weapon%d_ammo", i ), inventory.ammo[inventory.AmmoIndexForWeaponClass ( weap ) ] );				
			}
		} else {
			hud->SetStateBool( weapnum, false );
		}				
	}

	hud->SetStateInt( "weaponcount", index );

	const idMaterial *material = declManager->FindMaterial( weapon->GetIcon() );
	if ( material ) {
		material->SetSort( SS_GUI );
	}			

	material = declManager->FindMaterial( weapon->spawnArgs.GetString ( "inv_icon" ) );
	//if ( material ) {
	//	material->SetSort( SS_GUI );
	//}			

	hud->SetStateString( "weapicon", weapon->GetIcon() );	
	hud->SetStateString( "ammoIcon", weapon->spawnArgs.GetString ( "inv_icon" ) );
 	hud->SetStateInt( "player_weapon", currentWeapon );
	hud->SetStateInt( "player_lastweapon", previousWeapon );
	hud->SetStateInt( "player_idealweapon", ((displayWeapon!=-1)?displayWeapon:idealWeapon) );
	hud->SetStateInt( "player_idealIndex", idealIndex );

	// Weapon name for weapon selection
	const idDeclEntityDef* w = GetWeaponDef( ( displayWeapon != -1 ) ? displayWeapon:idealWeapon );
	if ( w ) {
		idStr langToken = w->dict.GetString( "inv_name" );
		hud->SetStateString( "weaponname", common->GetLocalizedString( langToken ) );
	}

	UpdateHudAmmo( hud );
 			
 	if ( cursor ) {
 		weapon->UpdateCrosshairGUI( cursor );

		// mekberg: force a redraw so ON_INIT gets called and doesn't stomp all over
		//			the color values we set in weaponChange.
		cursor->Redraw( gameLocal.time );
		cursor->HandleNamedEvent( "weaponChange" );
	}

	hud->HandleNamedEvent( "weaponChange" );
	hud->StateChanged( gameLocal.time ); 			
	weaponChangeIconsUp = true;

	// if previousWeapon is -1, the weaponChange state won't update it, so manually reset it
#ifdef _XENON
	if( previousWeapon == -1 ) {
		ResetHUDWeaponSwitch();
	}
#endif
}

/*
===============
idPlayer::StartRadioChatter
===============
*/
void idPlayer::StartRadioChatter ( void ) {
	if ( hud ) {
		hud->HandleNamedEvent( "radioChatterUp" );
	}
	if ( vehicleController.IsDriving ( ) ) {
		vehicleController.StartRadioChatter ( );
	}
}

/*
===============
idPlayer::StopRadioChatter
===============
*/
void idPlayer::StopRadioChatter ( void ) {
	if ( hud ) {
		hud->HandleNamedEvent( "radioChatterDown" );
	}
	if ( vehicleController.IsDriving( ) ) {
		vehicleController.StopRadioChatter( );
	}
}

/*
===============
idPlayer::DrawShadow
===============
*/
void idPlayer::DrawShadow( renderEntity_t *headRenderEnt ) {
	if ( gameLocal.isMultiplayer && g_skipPlayerShadowsMP.GetBool() ) {
		// Disable all player shadows for the local client
		renderEntity.suppressShadowInViewID	= gameLocal.localClientNum+1;
 		if ( headRenderEnt ) {
 			headRenderEnt->suppressShadowInViewID = gameLocal.localClientNum+1;
   		}
	} else if ( gameLocal.isMultiplayer || g_showPlayerShadow.GetBool() || pm_thirdPerson.GetBool() ) {
		// Show all player shadows
		renderEntity.suppressShadowInViewID	= 0;
 		if ( headRenderEnt ) {
			headRenderEnt->suppressShadowInViewID = 0;
		}
	} else {
		// Only show player shadows for other clients
		renderEntity.suppressShadowInViewID	= entityNumber+1;
 		if ( headRenderEnt ) {
 			headRenderEnt->suppressShadowInViewID = entityNumber+1;
   		}
	}
}

/*
===============
idPlayer::DrawHUD
===============
*/
void idPlayer::DrawHUD( idUserInterface *_hud ) {
	idUserInterface * cursor = idPlayer::cursor;
 
	if ( !gameLocal.GetLocalPlayer() ) {
		// server netdemo
		if ( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() && gameLocal.GetDemoFollowClient() == entityNumber ) {
			cursor = gameLocal.GetDemoCursor();
			if ( cursor ) {
				cursor->HandleNamedEvent( "showCrossCombat" );
			}
		}		
	} else {

		if ( team != gameLocal.GetLocalPlayer()->hudTeam && _hud ) {
			_hud->HandleNamedEvent( (team ? "setTeam_strogg" : "setTeam_marine") );	
			gameLocal.GetLocalPlayer()->hudTeam = team;
		}
	
		// if updating the hud of a followed client
		idPlayer *p = gameLocal.GetLocalPlayer();
		if ( p->spectating && p->spectator == entityNumber ) {
			cursor = p->GetCursorGUI();
			if ( cursor ) {
				cursor->HandleNamedEvent( "showCrossCombat" );
			}
		}		
	}

	if ( disableHud || influenceActive != INFLUENCE_NONE || privateCameraView || !_hud || !g_showHud.GetBool() ) {
		return;
	}

	if ( objectiveSystemOpen ) {
		if ( !GuiActive() ) {
			// showing weapon zoom gui when objectives are open because that's the way I'z told to make it werkz
			if ( weapon && weapon->GetZoomGui( ) && zoomed ) {
				weapon->GetZoomGui( )->Redraw( gameLocal.time );
			}
		}
		return;
	}

	if ( gameLocal.isMultiplayer ) {
		_hud->SetStateBool( "mp", true );
	} else {
		_hud->SetStateBool( "mp", false );
	}

	// Draw the cinematic hud when in a cinematic
	if ( gameLocal.GetCamera( ) ) {
		if ( cinematicHud && !(gameLocal.editors & EDITOR_MODVIEW) ) {
			cinematicHud->Redraw( gameLocal.time );
		}
		return;
	}
	
	// Let the vehicle draw the hud instead
	if ( vehicleController.IsDriving( ) ) {
		if ( !gameDebug.IsHudActive( DBGHUD_ANY ) ) {
			vehicleController.DrawHUD( );
			if ( cursor && health > 0 ) {			
				// mekberg: adjustable crosshair size.
				int crossSize = cvarSystem->GetCVarInteger( "g_crosshairSize" );
				crossSize = crossSize - crossSize % 8;
				cvarSystem->SetCVarInteger( "g_crosshairSize", crossSize );
				cursor->SetStateInt( "g_crosshairSize", crossSize );
				cursor->SetStateBool( "vehiclecursor", true );

				vehicleController.UpdateCursorGUI( cursor );
				cursor->Redraw( gameLocal.time );
			}
		}
		_hud = GetHud();
		// Boss bar
		if ( _hud && _hud->State().GetInt( "boss_health", "-1" ) != (bossEnemy ? bossEnemy->health : -1) ) {
			if ( !bossEnemy || bossEnemy->health <= 0 ) {
				bossEnemy = NULL;
				_hud->SetStateInt( "boss_health", -1 );
				_hud->HandleNamedEvent( "hideBossBar" );			
 				_hud->HandleNamedEvent( "hideBossShieldBar" ); // grrr, for boss buddy..but maybe other bosses will have shields?
			} else {			
				_hud->SetStateInt( "boss_health", bossEnemy->health );
				_hud->HandleNamedEvent( "updateBossBar" );
			}
		}
		return;
	}
	
	if ( cursor ) {
		cursor->SetStateBool( "vehiclecursor", false );
	}
	
	// FIXME: this is temp to allow the sound meter to show up in the hud
	// it should be commented out before shipping but the code can remain
	// for mod developers to enable for the same functionality
	_hud->SetStateInt( "s_debug", cvarSystem->GetCVarInteger( "s_showLevelMeter" ) );

	// don't draw main hud in spectator (only mphud)
	if ( !spectating && !gameDebug.IsHudActive( DBGHUD_ANY ) ) {
		// weapon targeting crosshair
		if ( !GuiActive() ) {
			if ( weapon && weapon->GetZoomGui( ) && zoomed ) {
				weapon->GetZoomGui( )->Redraw( gameLocal.time );
			}
// RAVEN BEGIN
// mekberg: removed check for weapon->ShowCrosshair.
//			we want to show the crosshair regardless for NPC tags
			if ( cursor && health > 0 ) {		
				// Pass the current weapon to the cursor gui for custom crosshairs
// mekberg: adjustable crosshair size.
				int crossSize = cvarSystem->GetCVarInteger( "g_crosshairSize" );
				crossSize = crossSize - crossSize % 8;
				cvarSystem->SetCVarInteger( "g_crosshairSize", crossSize );
				cursor->SetStateInt( "g_crosshairSize", crossSize );
// RAVEN END
				cursor->Redraw( gameLocal.time );
			}
		}	

		UpdateHudStats( _hud );

		if ( focusBrackets ) {
			// If 2d_calc is still true then the gui didnt render so we can abandon it
			if ( focusBrackets->State().GetBool( "2d_calc" ) ) {
				focusBrackets->SetStateBool( "2d_calc", false );
				focusBrackets = NULL;
				focusBracketsTime = 0;
				_hud->HandleNamedEvent( "hideBrackets" );
			} else {
				_hud->SetStateString( "bracket_left", focusBrackets->State().GetString( "2d_min_x" ) );
				_hud->SetStateString( "bracket_top", focusBrackets->State().GetString( "2d_min_y" ) );
				_hud->SetStateFloat( "bracket_width", focusBrackets->State().GetFloat( "2d_max_x" ) - focusBrackets->State().GetFloat( "2d_min_x" ) );
				_hud->SetStateFloat( "bracket_height", focusBrackets->State().GetFloat( "2d_max_y" ) - focusBrackets->State().GetFloat( "2d_min_y" ) );
				// TODO: Find a way to get bracket text from gui to hud
			}
		}		
	 	_hud->Redraw( gameLocal.realClientTime );
	}

	if ( gameLocal.isMultiplayer ) {
		idUserInterface* _mphud = mphud;
		// server netdemos don't have a local player, grab the right mphud
		if ( !gameLocal.GetLocalPlayer() ) {
			assert( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() );
			assert( gameLocal.GetDemoFollowClient() >= 0 );
			assert( gameLocal.entities[ gameLocal.GetDemoFollowClient() ] && gameLocal.entities[ gameLocal.GetDemoFollowClient() ]->IsType( idPlayer::GetClassType() ) );
			_mphud = gameLocal.GetDemoMphud();
		} else if ( gameLocal.GetLocalPlayer() != this ) {
			// if we're spectating someone else, use our local hud
			_mphud = gameLocal.GetLocalPlayer()->mphud;
		}
		if ( _mphud ) {
			gameLocal.mpGame.UpdateHud( _mphud );
			_mphud->Redraw( gameLocal.time );
		}
		
		if ( overlayHud && overlayHudTime > gameLocal.time && overlayHudTime != 0 ) {
			overlayHud->Redraw( gameLocal.time );
		} else {
			overlayHud = NULL;
			overlayHudTime = 0;
		}
	}
}

/*
===============
idPlayer::EnterCinematic
===============
*/
void idPlayer::EnterCinematic( void ) {
	Hide();

// RAVEN BEGIN
// jnewquist: Cinematics are letterboxed, this auto-fixes on widescreens
	g_fixedHorizFOV.SetBool(true);
// RAVEN END

   	if ( hud ) {
   		hud->HandleNamedEvent( "radioChatterDown" );
   	}
   	
   	cinematicHud = NULL;
   	
	// See if camera has custom cinematic gui
   	if ( gameLocal.GetCamera ( ) ) {
   		const char* guiCinematic;
   		guiCinematic = gameLocal.GetCamera()->spawnArgs.GetString ( "guiCinematic", "" );
   		if ( *guiCinematic ) {
			cinematicHud = uiManager->FindGui( guiCinematic, true, false, true );   		
		}
	}

	// Load default cinematic gui?
	if ( !cinematicHud ) {	
		const char* temp;
		if ( spawnArgs.GetString ( "cinematicHud", "", &temp ) ) {
			cinematicHud = uiManager->FindGui( temp, true, false, true );
		}
	}
	
   	// Have the cinematic hud start
   	if ( cinematicHud ) {	
   		cinematicHud->Activate ( true, gameLocal.time );
   		cinematicHud->HandleNamedEvent ( "cinematicStart" );
// RAVEN BEGIN
// jnewquist: Option to adjust vertical fov instead of horizontal for non 4:3 modes
		if ( cvarSystem->GetCVarInteger( "r_aspectRatio" ) != 0 ) {
			cinematicHud->HandleNamedEvent ( "hideLetterbox" );
		}
// RAVEN END
   	}
   	
   	physicsObj.SetLinearVelocity( vec3_origin );
   	
 	if ( weaponEnabled && weapon ) {
		//this preSave kills all effects and sounds that we don't need lingering around.
   		weapon->PreSave();
		weapon->EnterCinematic();
   	}

	// Reset state flags   
	memset ( &pfl, 0, sizeof(pfl) );
	pfl.onGround = true;
	pfl.dead	 = (health <= 0);
}
   
/*
===============
idPlayer::ExitCinematic
===============
*/
void idPlayer::ExitCinematic( void ) {
  	Show();
   
// RAVEN BEGIN
// jnewquist: Cinematics are letterboxed, this auto-fixes on widescreens
	g_fixedHorizFOV.SetBool(false);
// RAVEN END

 	if ( weaponEnabled && weapon ) {
		//and this will restore them!
		weapon->PostSave();
   		weapon->ExitCinematic();
   	}
   
   	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 0 );
	SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );

   	UpdateState();
}

/*
===============
idPlayer::SkipCinematic
===============
*/
bool idPlayer::SkipCinematic( void ) {
	StartSound( "snd_skipcinematic", SND_CHANNEL_ANY, 0, false, NULL );
	return gameLocal.SkipCinematic();
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
 		pfl.forward		= false;
 		pfl.backward	= false;
 		pfl.strafeLeft	= false;
 		pfl.strafeRight	= false;
	} else if ( gameLocal.time - lastDmgTime < 500 ) {
		forwardspeed	= velocity * viewAxis[ 0 ];
		sidespeed		= velocity * viewAxis[ 1 ];
		pfl.forward		= pfl.onGround && ( forwardspeed > 20.01f );
		pfl.backward	= pfl.onGround && ( forwardspeed < -20.01f );
		pfl.strafeLeft	= pfl.onGround && ( sidespeed > 20.01f );
		pfl.strafeRight	= pfl.onGround && ( sidespeed < -20.01f );
 	} else if ( xyspeed > MIN_BOB_SPEED ) {
		pfl.forward		= pfl.onGround && ( usercmd.forwardmove > 0 );
		pfl.backward	= pfl.onGround && ( usercmd.forwardmove < 0 );
		pfl.strafeLeft	= pfl.onGround && ( usercmd.rightmove < 0 );
		pfl.strafeRight	= pfl.onGround && ( usercmd.rightmove > 0 );
 	} else {
 		pfl.forward		= false;
 		pfl.backward	= false;
 		pfl.strafeLeft	= false;
 		pfl.strafeRight	= false;
   	}

	pfl.run		= 1;
 	pfl.dead	= ( health <= 0 );
}

/*
==================
idPlayer::WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayer::WeaponFireFeedback( const idDict *weaponDef ) {
	// force a blink
	blink_time = 0;

	// play the fire animation
	pfl.weaponFired = true;

	// Bias the intent direction more heavily due to firing
	BiasIntentDir( viewAxis[0]*100.0f, 1.0f );

	// update view feedback
	playerView.WeaponFireFeedback( weaponDef );
}

/*
===============
idPlayer::StopFiring
===============
*/
void idPlayer::StopFiring( void ) {
	pfl.attackHeld	= false;
	pfl.weaponFired = false;
	pfl.reload		= false;
	if ( weapon ) {
		weapon->EndAttack();
	}
}

/*
===============
idPlayer::FireWeapon
===============
*/
void idPlayer::FireWeapon( void ) {
	idMat3 axis;
	idVec3 muzzle;

//RITUAL BEGIN
	if( gameLocal.GetIsFrozen() && gameLocal.gameType == GAME_DEADZONE )
	{
		return;
	}
//RITUAL END
	if ( privateCameraView ) {
		return;
	}

	if ( g_editEntityMode.GetInteger() ) {
		GetViewPos( muzzle, axis );
		gameLocal.editEntities->SelectEntity( muzzle, axis[0], this );	
		return;
	}

	if ( !hiddenWeapon && weapon->IsReady() ) {
		// cheap hack so in MP the LG isn't allowed to fire in the short lapse while it goes from Fire -> Idle before changing to another weapon
		// this gimps the weapon a lil bit but is consistent with the visual feedback clients are getting since 1.0
		bool noFireWhileSwitching = false;
		noFireWhileSwitching = ( gameLocal.isMultiplayer && idealWeapon != currentWeapon && weapon->NoFireWhileSwitching() );
		if ( !noFireWhileSwitching ) {
			if ( weapon->AmmoInClip() || weapon->AmmoAvailable() ) {
				pfl.attackHeld = true;
				weapon->BeginAttack();
			} else {
				pfl.attackHeld = false;
				pfl.weaponFired = false;
				StopFiring();
				NextBestWeapon();
			}
		} else {
			StopFiring();
		}
	}
	// If reloading when fire is hit cancel the reload
	else if ( weapon->IsReloading() ) {
		weapon->CancelReload();
	}
/* twhitaker: removed this at the request of Matt Vainio.
	if ( !gameLocal.isMultiplayer ) {
		if ( hud && tipUp ) {
			HideTip();
		}
		// may want to track with with a bool as well
		// keep from looking up named events so often
		if ( objectiveSystem && objectiveUp ) {
			HideObjective();
		}
	}
*/
	if( hud && weaponChangeIconsUp ) {
		hud->HandleNamedEvent( "weaponFire" );
		// nrausch: objectiveSystem does not necessarily exist (in mp it doesn't)
		if ( objectiveSystem ) {
			objectiveSystem->HandleNamedEvent( "weaponFire" );
		}
		weaponChangeIconsUp = false;
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
			if ( !GetWeaponDef ( w ) ) {
				inventory.weapons &= ~( 1 << w );
			} else {
				rvWeapon::CacheWeapon( spawnArgs.GetString( va( "def_weapon%d", w ) ) );
			}		
		}
	}
}

/*
===============
idPlayer::Give
===============
*/
bool idPlayer::Give( const char *statname, const char *value, bool dropped ) {
	int amount;

	if ( pfl.dead ) {
		return false;
	}

	if ( IsInVehicle ( ) ) {
		vehicleController.Give ( statname, value );
	}

	int boundaryHealth = inventory.maxHealth;
	int boundaryArmor = inventory.maxarmor;
	if( PowerUpActive( POWERUP_GUARD ) ) {
		boundaryHealth = inventory.maxHealth / 2;
		boundaryArmor = inventory.maxarmor / 2;
	}
	if( PowerUpActive( POWERUP_SCOUT ) ) {
		boundaryArmor = 0;
	}
	if ( gameLocal.isMultiplayer ) {
		//In MP, you can get twice your max from pickups
		boundaryArmor *= 2;
	}

	if ( !idStr::Icmp( statname, "health" ) ) {
		if ( health >= boundaryHealth ) {
			return false;
		}
 		amount = atoi( value );
 		if ( amount ) {
 			health += amount;
 			if ( health > boundaryHealth ) {
 				health = boundaryHealth;
 			}
		}
	} else if ( !idStr::Icmp( statname, "bonushealth" ) ) {
		// allow health over max health
		if ( health >= boundaryHealth * 2 ) {
			return false;
		}
		amount = atoi( value );
 		if ( amount ) {
 			health += amount;
 			if ( health > boundaryHealth * 2 ) {
 				health = boundaryHealth * 2;
 			}
		}
		nextHealthPulse = gameLocal.time + HEALTH_PULSE;
	} else if ( !idStr::Icmp( statname, "armor" ) ) {
		if ( inventory.armor >= boundaryArmor ) {
			return false;
		}
		amount = atoi( value );

		inventory.armor += amount;
		if ( inventory.armor > boundaryArmor ) {
			 inventory.armor = boundaryArmor;
		}
		nextArmorPulse = gameLocal.time + ARMOR_PULSE;
	} else if ( !idStr::Icmp( statname, "air" ) ) {
		if ( airTics >= pm_airTics.GetInteger() ) {
			return false;
		}
		airTics += atoi( value ) / 100.0 * pm_airTics.GetInteger();
		if ( airTics > pm_airTics.GetInteger() ) {
			airTics = pm_airTics.GetInteger();
		}
	} else if ( !idStr::Icmp ( statname, "weaponmod" ) ) {
		if( !idStr::Icmp( value, "all" ) ) {
			for( int i = 0; i < MAX_WEAPONS; i++ ) {
				if ( inventory.weapons & ( 1 << i ) ) {
					GiveWeaponMods( i, 0xFFFFFFFF );
				}
			}
		} else {
			const char* pos = value;

			while( pos != NULL ) {
				const char* end = strchr( pos, ',' );
				int			len;
				if ( end ) {
					len = end - pos;
					end++;
				} else {
					len = strlen( pos );
				}

				idStr weaponMod ( pos, 0, len );
				GiveWeaponMod ( weaponMod );

				pos = end;
			}
		}
	} else {
 		return inventory.Give( this, spawnArgs, statname, value, &idealWeapon, true, dropped );
	}
	return true;
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
	
	bool dropped = item->spawnArgs.GetBool( "dropped" );

	if ( gameLocal.isMultiplayer && spectating ) {
		return false;
	}

	item->GetAttributes( attr );

	if( gameLocal.isServer || !gameLocal.isMultiplayer ) {
		gave = false;
		bool skipWeaponKey = false;
		bool skipRestOfKeys = false;
		if ( gameLocal.IsMultiplayer() ) {
			dropped = item->spawnArgs.GetBool( "dropped" );
			if ( item->spawnArgs.FindKey( "weaponclass" ) ) {
				//this is really fucking lame, but
				//this is the only way we know we're trying
				//to pick up a weapon before we blindly start
				//processesing the attribute arguments in
				//whatever order they're in below.  We need
				//to not process any at all if we're not allowed
				//to pick up the weapon in the first place!
				arg = attr.FindKey( "weapon" );
				if ( arg ) {
					skipWeaponKey = true;
					if ( Give( arg->GetKey(), arg->GetValue(), dropped ) ) {
						gave = true;
					} else if ( !dropped//not a dropped weapon
						&& gameLocal.IsWeaponsStayOn() ) {
						//if failed to give weapon, don't give anything else with the weapon
						skipRestOfKeys = true;
					}
				}
			}
		}
		if ( !skipRestOfKeys ) {
			for( i = 0; i < attr.GetNumKeyVals(); i++ ) {
				arg = attr.GetKeyVal( i );
				if ( skipWeaponKey && arg->GetKey() == "weapon" ) {
					//already processed this above
					continue;
				}
				if ( Give( arg->GetKey(), arg->GetValue(), dropped ) ) {
					gave = true;
				}
			}
		}

		// hack - powerups call into this code to let them give stuff based on inv_ keywords
		// for powerups that don't have any ammo/etc to give to the player, we still want to
		// display the inv_name on the hud
		// since idItemPowerup::GiveToPlayer() handles whether or not a player gets a powerup,
		// we can override gave here for powerups
		if ( !gave && !item->IsType( idItemPowerup::GetClassType() ) ) {
			return false;
		}
	} else {
		gave = true;
	}

	arg = item->spawnArgs.MatchPrefix( "inv_ammo_", NULL );
	if ( arg && hud ) {
		hud->HandleNamedEvent( "ammoPulse" );
	}
	arg = item->spawnArgs.MatchPrefix( "inv_health", NULL );
	if ( arg && hud ) {
		hud->HandleNamedEvent( "healthPulse" );
	}
	arg = item->spawnArgs.MatchPrefix( "inv_weapon", NULL );
	if ( arg && hud ) {
		// We need to update the weapon hud manually, but not
		// the armor/ammo/health because they are updated every
		// frame no matter what
		if ( gameLocal.isMultiplayer ) {
			UpdateHudWeapon( );
		} else {
			//so weapon mods highlight the correct weapon when received
			int weapon = SlotForWeapon ( arg->GetValue() );
			UpdateHudWeapon( weapon );
		}
		hud->HandleNamedEvent( "weaponPulse" );
	}
	arg = item->spawnArgs.MatchPrefix( "inv_armor", NULL );
	if ( arg && hud ) {
		hud->HandleNamedEvent( "armorPulse" );
	}
	
//	GiveDatabaseEntry ( &item->spawnArgs );
	
	// Show the item pickup on the hud
	if ( hud ) {
		idStr langToken = item->spawnArgs.GetString( "inv_name" );
		hud->SetStateString ( "itemtext", common->GetLocalizedString( langToken ) );
		hud->SetStateString ( "itemicon", item->spawnArgs.GetString( "inv_icon" ) );
		hud->HandleNamedEvent ( "itemPickup" );
	}
//RITUAL BEGIN
	if ( gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() )
		gameLocal.mpGame.RedrawLocalBuyMenu();
//RITUAL END

	return gave;
}

/*
===============
idPlayer::PowerUpModifier
===============
*/
float idPlayer::PowerUpModifier( int type ) {
	float mod = 1.0f;

	if ( PowerUpActive( POWERUP_QUADDAMAGE ) ) {
		switch( type ) {
			case PMOD_PROJECTILE_DAMAGE: {
				mod *= 3.0f;
				break;
			}
			case PMOD_MELEE_DAMAGE: {
				mod *= 3.0f;
				break;
			}
			case PMOD_PROJECTILE_DEATHPUSH: {
				mod *= 2.0f;
				break;
			}
		}
	}

	if ( PowerUpActive( POWERUP_HASTE ) ) {
		switch ( type ) {
			case PMOD_SPEED:	
				mod *= 1.3f;
				break;

			case PMOD_FIRERATE:
				mod *= 0.7f;
				break;
		}
	}

	// Arena CTF powerups
	if( PowerUpActive( POWERUP_AMMOREGEN ) ) {
		switch( type ) {
			case PMOD_FIRERATE: {
				mod *= 0.7f;
				break;
			}
		}
	}

	if( PowerUpActive( POWERUP_DOUBLER ) ) {
		switch( type ) {
			case PMOD_PROJECTILE_DAMAGE: {
				mod *= 2.0f;
				break;
			}
			case PMOD_MELEE_DAMAGE: {
				mod *= 2.0f;
				break;
			}
		}
	}

//RITUAL BEGIN
	if( PowerUpActive( POWERUP_TEAM_DAMAGE_MOD ) ) {
		switch( type ) {
			case PMOD_PROJECTILE_DAMAGE: {
				mod *= 1.75f;
				break;
			}
			case PMOD_MELEE_DAMAGE: {
				mod *= 1.75f;
				break;
			}
			case PMOD_FIRERATE: {
				mod *= 0.80f;
				break;
			}
		}
	}
//RITUAL END	
	if( PowerUpActive( POWERUP_SCOUT ) ) {
		switch( type ) {
			case PMOD_FIRERATE: {
				mod *= (2.0f / 3.0f);
				break;
			}
			case PMOD_SPEED: {	
				mod *= 1.5f;
				break;
			}
		}
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
idPlayer::StartPowerUpEffect
===============
*/
void idPlayer::StartPowerUpEffect( int powerup ) {

	switch( powerup ) {
		case POWERUP_CTF_MARINEFLAG: {
			AddClientModel( "mp_ctf_flag_pole" );
			AddClientModel( "mp_ctf_marine_flag_world" );
			flagEffect = PlayEffect( "fx_ctf_marine_flag_world", animator.GetJointHandle( spawnArgs.GetString( "flagEffectJoint" ) ), spawnArgs.GetVector( "flagEffectOrigin" ), physicsObj.GetAxis(), true );
			break;
		}

		case POWERUP_CTF_STROGGFLAG: {
			AddClientModel( "mp_ctf_flag_pole" );
			AddClientModel( "mp_ctf_strogg_flag_world" );
			flagEffect = PlayEffect( "fx_ctf_strogg_flag_world", animator.GetJointHandle( spawnArgs.GetString( "flagEffectJoint" ) ), spawnArgs.GetVector( "flagEffectOrigin" ), physicsObj.GetAxis(), true );
			break;
		}

		case POWERUP_CTF_ONEFLAG: {
			AddClientModel( "mp_ctf_one_flag" );
			break;
		}
		case POWERUP_DEADZONE: {
			PlayEffect( "fx_deadzone", animator.GetJointHandle( "origin" ), true );		
			break;
		}
		case POWERUP_QUADDAMAGE: {
			powerUpOverlay = quadOverlay;

			StopEffect( "fx_regeneration" );
			PlayEffect( "fx_quaddamage", animator.GetJointHandle( "chest" ), true );			
			StartSound( "snd_quaddamage_idle", SND_CHANNEL_POWERUP_IDLE, 0, false, NULL );

			// Spawn quad effect
			powerupEffect = gameLocal.GetEffect( spawnArgs, "fx_quaddamage_crawl" );
			powerupEffectTime = gameLocal.time;
			powerupEffectType = POWERUP_QUADDAMAGE;

			break;
		}

		case POWERUP_REGENERATION: {

			// when buy mode is enabled, we use the guard effect for team powerup regen ( more readable than everyone going red )
			if ( gameLocal.IsTeamPowerups() ) {
				// don't setup the powerup on dead bodies, it will float up where the body is invisible and the orientation will be messed up
				if ( teamHealthRegen == NULL ) {
					if ( health <= 0 ) {
						// we can't start it now, it will be floating where the hidden dead body is
						teamHealthRegenPending = true;
					} else {
						teamHealthRegen = PlayEffect( "fx_guard", renderEntity.origin, renderEntity.axis, true );
					}
				}
			} else {
				powerUpOverlay = regenerationOverlay;

				StopEffect( "fx_quaddamage" );
				PlayEffect( "fx_regeneration", animator.GetJointHandle( "chest" ), true );

				// Spawn regen effect
				powerupEffect = gameLocal.GetEffect( spawnArgs, "fx_regeneration" );			
				powerupEffectTime = gameLocal.time;
				powerupEffectType = POWERUP_REGENERATION;
			}

			break;
		}

		case POWERUP_HASTE: {
			powerUpOverlay = hasteOverlay;

			hasteEffect = PlayEffect( "fx_haste", GetPhysics()->GetOrigin(), GetPhysics()->GetAxis(), true );
			break;
		}
		
		case POWERUP_INVISIBILITY: {
			powerUpOverlay = invisibilityOverlay;

			powerUpSkin = declManager->FindSkin( spawnArgs.GetString( "skin_invisibility" ), false );
			break;
		}

		case POWERUP_GUARD: {
			if ( arenaEffect != NULL ) {
				// don't accumulate. clear whatever was there
				arenaEffect->Stop( true );
			}
			arenaEffect = PlayEffect( "fx_guard", physicsObj.GetOrigin(), physicsObj.GetAxis(), true );
			break;
		}
		
		case POWERUP_SCOUT: {
			if ( arenaEffect != NULL ) {
				// don't accumulate. clear whatever was there
				arenaEffect->Stop( true );
			}
			arenaEffect = PlayEffect( "fx_scout", physicsObj.GetOrigin(), physicsObj.GetAxis(), true );
			break;
		}
		
		case POWERUP_AMMOREGEN: {
			if ( gameLocal.IsTeamPowerups() ) {
				if ( teamAmmoRegen == NULL ) {
					if ( health <= 0 ) {
						teamAmmoRegenPending = true;
					} else {
						teamAmmoRegen = PlayEffect( "fx_ammoregen", renderEntity.origin, renderEntity.axis, true );
					}
				}
			} else {
				assert( health > 0 );
				if ( arenaEffect != NULL ) {
					// don't accumulate. clear whatever was there
					arenaEffect->Stop( true );
				}
				arenaEffect = PlayEffect( "fx_ammoregen", renderEntity.origin, renderEntity.axis, true );
			}
			break;
		}
		
		case POWERUP_TEAM_DAMAGE_MOD: {
			assert( gameLocal.IsTeamPowerups() );
			if ( teamDoubler == NULL ) {
				if ( health <= 0 ) {
					teamDoublerPending = true;
				} else {
					teamDoubler = PlayEffect( "fx_doubler", renderEntity.origin, renderEntity.axis, true );				
				}
			}
			break;
		}

		case POWERUP_DOUBLER: {
			assert( health > 0 );
			if ( arenaEffect != NULL ) {
				// don't accumulate. clear whatever was there
				arenaEffect->Stop( true );
			}
			arenaEffect = PlayEffect( "fx_doubler", renderEntity.origin, renderEntity.axis, true );
			break;
		}
	}
}

/*
===============
idPlayer::StopPowerUpEffect
===============
*/
void idPlayer::StopPowerUpEffect( int powerup ) {
	//if the player doesn't have quad, regen, haste or invisibility remaining on him, remove the power up overlay.
	if( !( 
		(inventory.powerups & ( 1 << POWERUP_QUADDAMAGE ) ) || 
		(inventory.powerups & ( 1 << POWERUP_REGENERATION ) ) || 
		(inventory.powerups & ( 1 << POWERUP_HASTE ) ) || 
		(inventory.powerups & ( 1 << POWERUP_INVISIBILITY ) ) 
		) )	{

			powerUpOverlay = NULL;
			StopSound( SND_CHANNEL_POWERUP_IDLE, false );
		}

	switch( powerup ) {
		case POWERUP_QUADDAMAGE: {
			powerupEffect = NULL;
			powerupEffectTime = 0;
			powerupEffectType = 0;

			StopEffect( "fx_quaddamage" );
			break;
		}
		case POWERUP_REGENERATION: {
			if ( gameLocal.IsTeamPowerups() ) {
				teamHealthRegenPending = false;
				StopEffect( "fx_guard" );
			} else {
				powerupEffect = NULL;
				powerupEffectTime = 0;
				powerupEffectType = 0;

				StopEffect( "fx_regeneration" );
			}
			break;
		}
		case POWERUP_HASTE: {
			StopEffect( "fx_haste" );
			break;
		}
		case POWERUP_INVISIBILITY: {
			powerUpSkin = NULL;
			break;
		}
		case POWERUP_CTF_STROGGFLAG: {
			RemoveClientModel( "mp_ctf_flag_pole" );
			RemoveClientModel( "mp_ctf_strogg_flag_world" );
			StopEffect( "fx_ctf_strogg_flag_world" );
			break;
		}
		case POWERUP_CTF_MARINEFLAG: {
			RemoveClientModel( "mp_ctf_flag_pole" );
			RemoveClientModel( "mp_ctf_marine_flag_world" );
			StopEffect( "fx_ctf_marine_flag_world" );
			break;
		}
		case POWERUP_CTF_ONEFLAG: {
			RemoveClientModel ( "mp_ctf_one_flag" );
			break;
		}
	  case POWERUP_DEADZONE: {
			StopEffect( "fx_deadzone" );
			break;
		}
		case POWERUP_SCOUT: {
			StopEffect( "fx_scout" );
			break;
		}
		case POWERUP_GUARD: {
			StopEffect( "fx_guard" );
			break;
		}
		case POWERUP_TEAM_DAMAGE_MOD:
			teamDoublerPending = false;
			// fallthrough
		case POWERUP_DOUBLER: {
			StopEffect( "fx_doubler" );
			break;
		}
		case POWERUP_AMMOREGEN: {
			teamAmmoRegenPending = false;
			StopEffect( "fx_ammoregen" );
			break;
		}
	}
}

/*
===============
idPlayer::GivePowerUp
passiveEffectsOnly - GivePowerup() is used to restore effects on stale players coming
back into snapshot.  We don't want to announce powerups in this case 
(just re-start effects)
===============
*/
bool idPlayer::GivePowerUp( int powerup, int time, bool team ) {
	if ( powerup < 0 || powerup >= POWERUP_MAX ) {
		gameLocal.Warning( "Player given power up %i\n which is out of range", powerup );
		return false;
	}

	if ( gameLocal.isServer ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.WriteShort( powerup );
		msg.WriteBits( 1, 1 );
		// team flag only needed for POWERUP_AMMOREGEN
		msg.WriteBits( team, 1 );
		ServerSendEvent( EVENT_POWERUP, &msg, false, -1 );
	}

	inventory.GivePowerUp( this, powerup, time );

	// only start client effects in the same instance
	// play all stuff in instance 0 for server netdemo - atm other instances are not recorded
	bool playClientEffects = ( ( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() && instance == 0 ) ||
							   ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() == instance ) );

	switch( powerup ) {
		case POWERUP_CTF_MARINEFLAG: {
			// shouchard:  added notice for picking up the flag
			if ( playClientEffects && this == gameLocal.GetLocalPlayer() ) {
				if ( mphud ) {
					mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_104419" ) );
					mphud->HandleNamedEvent( "main_notice" );
				}
			}
			UpdateTeamPowerups();
			break;
		}

		case POWERUP_CTF_STROGGFLAG: {
			// shouchard:  added notice for picking up the flag
			if ( playClientEffects && this == gameLocal.GetLocalPlayer() ) {
				if ( mphud ) {
					mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_104419" ) );
					mphud->HandleNamedEvent( "main_notice" );
				}
			}
			UpdateTeamPowerups();
			break;
		}

		case POWERUP_CTF_ONEFLAG: {
			// shouchard:  added notice for picking up the flag
			if ( playClientEffects && this == gameLocal.GetLocalPlayer() ) {
				if ( mphud ) {
					mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_104419" ) );
					mphud->HandleNamedEvent( "main_notice" );
				}
			}
			UpdateTeamPowerups();
			break;
		}
		
		case POWERUP_QUADDAMAGE: {		
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_QUAD_DAMAGE, gameLocal.time, gameLocal.gameType == GAME_TOURNEY ? GetInstance() : -1 );
			break;
		}

		case POWERUP_REGENERATION: {
			nextHealthPulse = gameLocal.time + HEALTH_PULSE;

			// Have to test for this because buying the team regeneration powerup will cause
			// this to get hit multiple times as the server distributes the powerups to the clients.
			if ( gameLocal.GetLocalPlayer() == this ) {
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_REGENERATION, gameLocal.time, gameLocal.gameType == GAME_TOURNEY ? GetInstance() : -1 );
			}
			break;
		}
		case POWERUP_HASTE: {
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_HASTE, gameLocal.time, gameLocal.gameType == GAME_TOURNEY ? GetInstance() : -1 );
			break;
		}
		case POWERUP_INVISIBILITY: {
			gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_INVISIBILITY, gameLocal.time, gameLocal.gameType == GAME_TOURNEY ? GetInstance() : -1 );
			break;
		}
		case POWERUP_GUARD: {
			nextHealthPulse = gameLocal.time + HEALTH_PULSE;
			inventory.maxHealth = 200;
			inventory.maxarmor = 200;

			break;
		}
		case POWERUP_SCOUT: {
			inventory.armor = 0;

			break;
		}
		case POWERUP_AMMOREGEN: {
			if ( team && gameLocal.GetLocalPlayer() == this ) {
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_TEAM_AMMOREGEN, gameLocal.time, gameLocal.gameType == GAME_TOURNEY ? GetInstance() : -1 );
			}
			break;
		}
		case POWERUP_TEAM_DAMAGE_MOD: {
			if ( gameLocal.GetLocalPlayer() == this ) {
				gameLocal.mpGame.ScheduleAnnouncerSound( AS_GENERAL_TEAM_DOUBLER, gameLocal.time, gameLocal.gameType == GAME_TOURNEY ? GetInstance() : -1 );
			}
			break;
		}
//RITUAL BEGIN
		case POWERUP_DEADZONE: {
			if ( playClientEffects && this == gameLocal.GetLocalPlayer() ) {
				if ( mphud ) {
					mphud->SetStateString( "main_notice_text", common->GetLocalizedString( "#str_122000" ) ); // Squirrel@Ritual - Localized for 1.2 Patch
					mphud->HandleNamedEvent( "main_notice" );
				}
			}
			break;
		}
//RITUAL END
	}

	// only start effects if in our instances and snapshot
	if ( playClientEffects && !fl.networkStale ) {
		StartPowerUpEffect( powerup );
	}

	return true;
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

 	inventory.powerups &= ~( 1 << i );
	inventory.powerupEndTime[ i ] = 0;

	//if the player doesn't have quad, regen, haste or invisibility remaining on him, remove the power up overlay.
	if( !( 
			(inventory.powerups & ( 1 << POWERUP_TEAM_DAMAGE_MOD ) ) ||
			(inventory.powerups & ( 1 << POWERUP_QUADDAMAGE ) ) || 
			(inventory.powerups & ( 1 << POWERUP_REGENERATION ) ) || 
			(inventory.powerups & ( 1 << POWERUP_HASTE ) ) || 
			(inventory.powerups & ( 1 << POWERUP_INVISIBILITY ) ) ||
			(inventory.powerups & ( 1 << POWERUP_DEADZONE ) ) 
		) )	{

		powerUpOverlay = NULL;
		StopSound( SND_CHANNEL_POWERUP_IDLE, false );
	}
	
	StopPowerUpEffect( i );
}

/*
==============
idPlayer::GetArenaPowerupString
==============
*/
const char* idPlayer::GetArenaPowerupString ( void ) {
	if ( PowerUpActive( POWERUP_SCOUT ) ) {
		return "^isct";
	} else if ( PowerUpActive( POWERUP_GUARD ) ) {
		return "^igrd";
	} else if ( PowerUpActive( POWERUP_DOUBLER ) ) {
		return "^idbl";
	} else if ( PowerUpActive( POWERUP_AMMOREGEN ) ) {
		return "^irgn";
	} else {
		return "^ixxx";
	}
}

/*
==============
idPlayer::UpdatePowerUps
==============
*/
void idPlayer::UpdatePowerUps( void ) {
	int i;
	int index;
	int wearoff;

	idUserInterface *hud = idPlayer::hud;
	if ( !gameLocal.GetLocalPlayer() ) {
		// server netdemo
		if ( gameLocal.GetDemoState() == DEMO_PLAYING && gameLocal.IsServerDemo() && gameLocal.GetDemoFollowClient() == entityNumber ) {
			hud = gameLocal.GetDemoHud();
		}
	} else {
		// if updating the hud of a followed client
		idPlayer *p = gameLocal.GetLocalPlayer();
		if ( p->spectating && p->spectator == entityNumber ) {
			assert( p->hud && p->mphud );
			hud = p->hud;
		}
	}

	wearoff = -1;
	if ( hud ) {
		hud->HandleNamedEvent( "clearPowerups" );
	}

	for ( i = 0, index = 0; i < POWERUP_MAX; i++ ) {
		// Do we have this powerup?
		if ( !(inventory.powerups & ( 1 << i ) ) ) {
			continue;
		}
			
		if ( inventory.powerupEndTime[i] > gameLocal.time || inventory.powerupEndTime[i] == -1 ) {
			// If there is still time remaining on the powerup then update the hud		
			if ( hud ) {
				// Play the wearoff sound for the powerup that is closest to wearing off
				if ( ( wearoff == -1 || inventory.powerupEndTime[i] < inventory.powerupEndTime[wearoff] ) && inventory.powerupEndTime[i] != -1 ) {
					wearoff = i;
				}

				// for flags, set the powerup_flag_* variables, which give us a special pulsing flag display
				if( i == POWERUP_CTF_MARINEFLAG || i == POWERUP_CTF_STROGGFLAG || i == POWERUP_CTF_ONEFLAG ) {
					hud->SetStateInt( "powerup_flag_visible", 1 );
				} else {
					hud->SetStateString ( va("powerup%d_icon", index ), GetPowerupDef(i)->dict.GetString ( "inv_icon" ) );
					hud->SetStateString ( va("powerup%d_time", index ), inventory.powerupEndTime[i] == -1 ? "" : va( "%d" , (int)MS2SEC(inventory.powerupEndTime[i] - gameLocal.time) + 1 ) );
					hud->SetStateInt ( va( "powerup%d_visible", index ), 1 );
					index++;
				}
			}

			continue;
		} else if ( inventory.powerupEndTime[ i ] != -1 && gameLocal.isServer ) {
			// This particular powerup needs to respawn in a special way.
			if ( i == POWERUP_DEADZONE ) {
				gameLocal.mpGame.GetGameState()->SpawnDeadZonePowerup();
			}
			// Powerup time has run out so take it away from the player
			ClearPowerup( i );
		}
	}

	// PLay wear off sound?
	if ( gameLocal.isNewFrame && wearoff != -1 ) {
		if ( (inventory.powerupEndTime[wearoff] - gameLocal.time) < POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			if ( (inventory.powerupEndTime[wearoff] - gameLocal.time) / POWERUP_BLINK_TIME != ( inventory.powerupEndTime[wearoff] - gameLocal.previousTime ) / POWERUP_BLINK_TIME ) {
				StartSound ( "snd_powerup_wearoff", SND_CHANNEL_POWERUP, 0, false, NULL );
			}
		}
	}

	// Reneration regnerates faster when less than maxHealth and can regenerate up to maxHealth * 2
	if ( gameLocal.time > nextHealthPulse ) {
// RITUAL BEGIN
// squirrel: health regen only applies if you have positive health
		if( health > 0 ) {
			if ( PowerUpActive ( POWERUP_REGENERATION ) || PowerUpActive ( POWERUP_GUARD ) ) {
				int healthBoundary = inventory.maxHealth; // health will regen faster under this value, slower above
				int healthTic = 15;

				if( PowerUpActive ( POWERUP_GUARD ) ) {
					// guard max health == 200, so set the boundary back to 100
					healthBoundary = inventory.maxHealth / 2;
					if( PowerUpActive (POWERUP_REGENERATION) ) {
						healthTic = 30;
					}
				}

				if ( health < healthBoundary ) {
					// only actually give health on the server
					if( gameLocal.isServer ) {
						health += healthTic;
						if ( health > (healthBoundary * 1.1f) ) {
							health = healthBoundary * 1.1f;
						}
					}
					StartSound ( "snd_powerup_regen", SND_CHANNEL_POWERUP, 0, false, NULL );
					nextHealthPulse = gameLocal.time + HEALTH_PULSE;
				} else if ( health < (healthBoundary * 2) ) {
					if( gameLocal.isServer ) {
						health += healthTic / 3;
						if ( health > (healthBoundary * 2) ) {
							health = healthBoundary * 2;
						}
					}
					StartSound ( "snd_powerup_regen", SND_CHANNEL_POWERUP, 0, false, NULL );
					nextHealthPulse = gameLocal.time + HEALTH_PULSE;
				}	
			// Health above max technically isnt a powerup but functions as one so handle it here
			} else if ( health > inventory.maxHealth && gameLocal.isServer ) { 
				nextHealthPulse = gameLocal.time + HEALTH_PULSE;
				health--;
			}
		}
// RITUAL END
	}

	// Regenerate ammo
	if( gameLocal.isServer && PowerUpActive( POWERUP_AMMOREGEN ) ) {
		for( int i = 0; i < MAX_WEAPONS; i++ ) {
			if( inventory.weapons & ( 1 << i ) ) {
				int ammoIndex	= inventory.AmmoIndexForWeaponIndex( i );
				int max			= inventory.StartingAmmoForWeaponIndex( i );

				// only regen ammo if lower than starting
				if( gameLocal.time > nextAmmoRegenPulse[ ammoIndex ] && inventory.ammo[ ammoIndex ] < max ) {
					int step		= inventory.AmmoRegenStepForWeaponIndex( i );
					int time		= inventory.AmmoRegenTimeForWeaponIndex( i );

					if( inventory.ammo[ ammoIndex ] < max ) {
						inventory.ammo[ ammoIndex ] += step;
					}
					if( inventory.ammo[ ammoIndex ] >= max ) {
						inventory.ammo[ ammoIndex ] = max;
					}

					nextAmmoRegenPulse[ ammoIndex ] = gameLocal.time + time;
				}	
			}
		}
	}

	// Tick armor down if greater than max armor
	if ( !gameLocal.isClient && gameLocal.time > nextArmorPulse ) {
		if ( inventory.armor > inventory.maxarmor ) { 
			nextArmorPulse += ARMOR_PULSE;
			inventory.armor--;
		}		
	}
		
	// Assign the powerup skin as long as we are alive
 	if ( health > 0 ) {
 		if ( powerUpSkin ) {
 			renderEntity.customSkin = powerUpSkin;
			if( clientHead ) {
				clientHead->SetSkin( powerUpSkin );
			}

			if( weaponWorldModel ) {
				weaponWorldModel->SetSkin( powerUpSkin );
			}

			if( weaponViewModel ) {
				weaponViewModel->SetSkin( powerUpSkin );
			}
 		} else {
 			renderEntity.customSkin = skin;

			if( clientHead ) {
				clientHead->SetSkin( headSkin );
			}

			if( weaponViewModel ) {
				weaponViewModel->SetSkin( weaponViewSkin );
			}
 		}

		if( weaponViewModel ) {
			weaponViewModel->SetOverlayShader( powerUpOverlay );
		}

		if( clientHead ) {
			clientHead->GetRenderEntity()->overlayShader = powerUpOverlay;
		}

		if( weaponWorldModel ) {
			weaponWorldModel->GetRenderEntity()->overlayShader = powerUpOverlay;
		}

		renderEntity.overlayShader = powerUpOverlay;
	} else {
		renderEntity.overlayShader = NULL;
		powerUpOverlay = NULL;

		if( clientHead ) {
			clientHead->GetRenderEntity()->overlayShader = NULL;
		}

		if ( renderEntity.customSkin != gibSkin ) {
			if ( influenceSkin ) {
				renderEntity.customSkin = influenceSkin;
			} else {
				renderEntity.customSkin = skin;
			}
		}
	}

	// Spawn quad effect
	if( PowerUpActive( powerupEffectType ) && powerupEffect && gameLocal.time >= powerupEffectTime  ) {
		rvClientCrawlEffect* effect = new rvClientCrawlEffect( powerupEffect, this, 100, &powerupEffectJoints );
		effect->Play ( gameLocal.time, false );					
		effect->GetRenderEffect()->suppressSurfaceInViewID = entityNumber+1;
		powerupEffectTime = gameLocal.time + 400;
	}
	
	// Attenuate haste effect
	if ( hasteEffect ) {
		hasteEffect->Attenuate( idMath::ClampFloat( 0.0f, 1.0f, physicsObj.GetLinearVelocity().LengthSqr() / Square(100.0f) ) );
	}

	if ( flagEffect ) {
		flagEffect->Attenuate( idMath::ClampFloat( 0.0f, 1.0f, physicsObj.GetLinearVelocity().LengthSqr() / Square(100.0f) ) );
	}

	if( arenaEffect ) {
		arenaEffect->SetOrigin( vec3_zero );
	}
}

/*
===============
idPlayer::ClearPowerUps
===============
*/
void idPlayer::ClearPowerUps( void ) {
	int i;
 	for ( i = 0; i < POWERUP_MAX; i++ ) {
 		if ( PowerUpActive( i ) ) {
   			ClearPowerup( i );
   		}
   	}

	inventory.ClearPowerUps();
}

/*
===============
idPlayer::GiveWeaponMods
===============
*/
bool idPlayer::GiveWeaponMods( int mods ) {
	inventory.weaponMods[currentWeapon] |= mods;
	currentWeapon = -1;
	
	return true;
}

/*
===============
idPlayer::GiveWeaponMods
===============
*/
bool idPlayer::GiveWeaponMods( int weapon, int mods ) {
	inventory.weaponMods[weapon] |= mods;
	currentWeapon = -1;

	return true;
}

/*
==============
idPlayer::GiveWeaponMod
==============
*/
void idPlayer::GiveWeaponMod ( const char* weaponmod ) {
	const idDict* modDict;
	const idDict* weaponDict;
	const char*	  weaponClass;
	int			  m;
	int			  weaponIndex;

	// Grab the weapon mod dictionary
	modDict = gameLocal.FindEntityDefDict ( weaponmod, false );
	if ( !modDict ) {
		gameLocal.Warning ( "Invalid weapon modification def specified '%s'", weaponmod );
		return;
	}
		
	// Get the weapon it modifies
	weaponClass = modDict->GetString ( "weapon" );
	weaponDict  = gameLocal.FindEntityDefDict ( weaponClass, false );
	if ( !weaponDict ) {
		gameLocal.Warning ( "Invalid weapon classname '%s' specified on weapon modification '%s'", weaponClass, weaponmod );
		return;
	}
	
	weaponIndex = SlotForWeapon ( weaponClass );

	// Find the index of the weapon mod
	for ( m = 0; m < MAX_WEAPONMODS; m ++ ) {		
		const char* mod;
		mod = weaponDict->GetString ( va("def_mod%d",m+1) );
		if ( !mod || !*mod ) {
			break;
		}

		if ( !idStr::Icmp ( weaponmod, mod ) ) {
			if ( !(inventory.weaponMods[weaponIndex] & (1<<m)) ) {
				inventory.weaponMods[weaponIndex] |= (1<<m);
				// If the players current weapon is the modified one then we need
				// to force the weapon to switch so the mods dont just appear on it
				if ( currentWeapon == weaponIndex ) {
					currentWeapon = -1;
				}
			}
			return;
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

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM(this);
// RAVEN END
	inventory.items.Append( new idDict( *item ) );
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_POP_HEAP();
// RAVEN END

	if ( hud ) {
		const char *itemName = common->GetLocalizedString( item->GetString( "inv_name" ) );
		hud->SetStateString ( "itemtext", itemName );
		hud->SetStateString ( "itemicon", item->GetString( "inv_icon" ) );
		hud->HandleNamedEvent ( "itemPickup" );
	}
	
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

// RAVEN BEGIN
// mekberg: swap objective positions to allow for stack-like appearance.
	int objectiveCount = inventory.objectiveNames.Num();
	for ( int i = 0; i < inventory.objectiveNames.Num(); i++, objectiveCount-- ) {
		objectiveSystem->SetStateString( va( "objective%i", objectiveCount ), "1" );
		objectiveSystem->SetStateString( va( "objectivetitle%i", objectiveCount ), inventory.objectiveNames[i].title.c_str() );
		objectiveSystem->SetStateString( va( "objectivetext%i", objectiveCount), inventory.objectiveNames[i].text.c_str() );
		objectiveSystem->SetStateInt( va( "objectiveLength%i", objectiveCount), inventory.objectiveNames[i].text.Length() );
		objectiveSystem->SetStateString( va( "objectiveshot%i", objectiveCount), inventory.objectiveNames[i].screenshot.c_str() );
	}
	objectiveSystem->SetStateBool( "noObjective", !objectiveCount );
// RAVEN END

	objectiveSystem->StateChanged( gameLocal.time );
}

/*
===============
idPlayer::GiveObjective
===============
*/
void idPlayer::GiveObjective( const char *title, const char *text, const char *screenshot ) {
	idObjectiveInfo info;
// RAVEN BEGIN
	info.title = common->GetLocalizedString( title );
	info.text = common->GetLocalizedString( text );
// RAVEN END
	info.screenshot = screenshot;
	inventory.objectiveNames.Append( info );
	if ( showNewObjectives ) {
		ShowObjective( "newObjective" );
	}
	if ( objectiveSystem ) {
		if ( objectiveSystemOpen ) {
			objectiveSystemOpen = false;
			ToggleObjectives ( );
#ifdef _XENON
			g_ObjectiveSystemOpen = objectiveSystemOpen;
#endif
		}
	}
}

/*
===============
idPlayer::CompleteObjective
===============
*/
void idPlayer::CompleteObjective( const char *title ) {
// RAVEN BEGIN
	title = common->GetLocalizedString( title );
// RAVEN END
	int c = inventory.objectiveNames.Num();
	for ( int i = 0;  i < c; i++ ) {
		if ( idStr::Icmp(inventory.objectiveNames[i].title, title) == 0 ) {
			inventory.objectiveNames.RemoveIndex( i );
			break;
		}
	}
	ShowObjective( "newObjectiveComplete" );

	if ( objectiveSystem ) {
		objectiveSystem->HandleNamedEvent( "newObjectiveComplete" );
	}

	if ( objectiveSystemOpen ) {
		objectiveSystemOpen = false;
		ToggleObjectives ( );
#ifdef _XENON
		g_ObjectiveSystemOpen = objectiveSystemOpen;
#endif
	}
}

/*
===============
idPlayer::FailObjective
===============
*/
void idPlayer::FailObjective ( const char* title ) {
// RAVEN BEGIN
	title = common->GetLocalizedString( title );

// mekberg: prevent save games if objective failed.
	gameLocal.sessionCommand = "objectiveFailed ";
	gameLocal.sessionCommand += title;
// RAVEN END
	HideObjective ( );
	if ( objectiveSystem ) {
		objectiveSystem->HandleNamedEvent( "objectiveFailed" );
	}
	if( IsInVehicle() )	{
		 vehicleController.GetVehicle()->EjectAllDrivers(); 
	}
	fl.takedamage = true;
	pfl.objectiveFailed = true;
#ifdef _XENON
	playerView.Fade( colorBlack, MAX_RESPAWN_TIME_XEN_SP );
	minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME_XEN_SP;
	maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME_XEN_SP;
#else
	playerView.Fade( colorBlack, 12000 );
	minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME;
	maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
#endif
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
	delete item;
}

/*
===============
idPlayer::GiveItem
===============
*/
void idPlayer::GiveItem( const char *itemname ) {
	idDict	args;

	args.Set( "classname", itemname );
	args.Set( "owner", name.c_str() );
	args.Set( "givenToPlayer", va( "%d", entityNumber ) );

	if ( gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() ) {
		// check if this is a weapon
		if( !idStr::Icmpn( itemname, "weapon_", 7 ) ) {
			int weaponIndex = SlotForWeapon( itemname );
			if( weaponIndex >= 0 && weaponIndex < MAX_WEAPONS )
			{
				int weaponIndexBit = ( 1 << weaponIndex );
				inventory.weapons |= weaponIndexBit;
				inventory.carryOverWeapons |= weaponIndexBit;
				carryOverCurrentWeapon = weaponIndex;
			}
		}

		// if the player is dead, credit him with this armor or ammo purchase
		if ( health <= 0 ) {
			if( !idStr::Icmp( itemname, "item_armor_small" ) ) {
				inventory.carryOverWeapons |= CARRYOVER_FLAG_ARMOR_LIGHT;
			} else if( !idStr::Icmp( itemname, "item_armor_large" ) ) {
				inventory.carryOverWeapons |= CARRYOVER_FLAG_ARMOR_HEAVY;
			} else if( !idStr::Icmp( itemname, "ammorefill" ) ) {
				inventory.carryOverWeapons |= CARRYOVER_FLAG_AMMO;
			}
		} else {
			if ( !idStr::Icmp( itemname, "ammorefill" ) ) {
				int i;
				for ( i = 0 ; i < MAX_AMMOTYPES; i++ ) {
					int a = gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( rvWeapon::GetAmmoNameForIndex( i ), 0 );
					inventory.ammo[i] += a; 
					if ( inventory.ammo[i] > inventory.MaxAmmoForAmmoClass( this, rvWeapon::GetAmmoNameForIndex(i) ) ) {
						inventory.ammo[i] = inventory.MaxAmmoForAmmoClass( this, rvWeapon::GetAmmoNameForIndex(i) );
					}
				}
			}
		}
	}

	// spawn the item if the player is alive
	if ( health > 0 && idStr::Icmp( itemname, "ammorefill" ) ) {
		gameLocal.SpawnEntityDef( args );
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
 	if ( gameLocal.isClient || spectating || gameLocal.inCinematic || influenceActive || !weapon ) {
 		return;
 	}

	weapon->Reload();
}

#ifdef _XENON
/*
===============
idPlayer::ScheduleWeaponSwitch
===============
*/
void idPlayer::ScheduleWeaponSwitch(int weapon)
{
	CancelEvents(&EV_Player_SelectWeapon);
	hud->SetStateInt("player_selectedWeapon", weapon-1);
	hud->HandleNamedEvent( "weaponSelect" );
	
	// nrausch: support for turning the weapon change ui on and off
	idWindow *win = FindWindowByName( "p_weapswitch", hud->GetDesktop() );
	if ( win ) {
		win->SetVisible( false );
	}

	if ( weapon > 0 ) {
		const char *weap = spawnArgs.GetString( va( "def_weapon%d", weapon-1 ) );
		PostEventSec(&EV_Player_SelectWeapon, 0.25f, weap);
	}
}
#endif

/*
===============
idPlayer::ShowCrosshair
===============
*/
void idPlayer::ShowCrosshair( void ) {
	if ( !weaponEnabled ) {
		return;
	}

	if ( cursor ) {
		cursor->HandleNamedEvent( "showCrossCombat" );
	}
	UpdateHudWeapon();
}

/*
===============
idPlayer::HideCrosshair
===============
*/
void idPlayer::HideCrosshair( void ) {
	if ( cursor ) {
		cursor->HandleNamedEvent( "crossHide" );
	}
}

/*
===============
idPlayer::LastWeapon
===============
*/
void idPlayer::LastWeapon( void ) {
	// Dont bother if previousWeapon is invalid or the player is spectating
	if ( spectating || previousWeapon < 0 )	{
		return;
	}
	
	// Do we have the weapon still?
	if ( !(inventory.weapons & ( 1 << previousWeapon ) ) ) {
		return;
	}
	
	idealWeapon = previousWeapon;
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
 		if ( !weap[ 0 ] || ( ( inventory.weapons & ( 1 << w ) ) == 0 ) || ( !inventory.HasAmmo( weap ) ) ) {
			continue;
		}
		if ( !spawnArgs.GetBool( va( "weapon%d_best", w ) ) ) {
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
 
// RAVEN BEGIN
// nrausch: support for turning the weapon change ui on and off

	// check if we have any weapons
	if ( !inventory.weapons ) {
		return;
	}

#ifdef _XENON

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
		if ( ( inventory.weapons & ( 1 << w ) ) == 0 ) {
			continue;
		}
		if ( inventory.HasAmmo( weap ) ) {
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {		
		if ( entityNumber == gameLocal.localClientNum ) {
			idWindow *win = FindWindowByName( "p_weapswitch", hud->GetDesktop() );
			if ( win ) {
				win->SetVisible( true );
			}
		}
 		
 		if ( gameLocal.isClient ) {
			return;
		}

		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}

#else

 	if ( gameLocal.isClient ) {
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
		if ( ( inventory.weapons & ( 1 << w ) ) == 0 ) {
			continue;
		}
		if ( inventory.HasAmmo( weap ) ) {
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}

#endif
// RAVEN END
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
 
// RAVEN BEGIN
// nrausch: support for turning the weapon change ui on and off

	// check if we have any weapons
	if ( !inventory.weapons ) {
		return;
	}

#ifdef _XENON
	
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
		if ( ( inventory.weapons & ( 1 << w ) ) == 0 ) {
			continue;
		}
		if ( inventory.HasAmmo( weap ) ) {
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		if ( entityNumber == gameLocal.localClientNum ) {
			idWindow *win = FindWindowByName( "p_weapswitch", hud->GetDesktop() );
			if ( win ) {
				win->SetVisible( true );
			}
		}
 		
		if ( gameLocal.isClient ) {
			return;
		}

		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}

#else

 	if ( gameLocal.isClient ) {
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
		if ( ( inventory.weapons & ( 1 << w ) ) == 0 ) {
			continue;
		}
		if ( inventory.HasAmmo( weap ) ) {
			break;
		}
	}

	if ( ( w != currentWeapon ) && ( w != idealWeapon ) ) {
		idealWeapon = w;
		weaponSwitchTime = gameLocal.time + WEAPON_SWITCH_DELAY;
		UpdateHudWeapon();
	}
#endif
// RAVEN END
}

/*
===============
idPlayer::SelectWeapon
===============
*/
void idPlayer::SelectWeapon( const char *weapon_name ) {
	Event_SelectWeapon( weapon_name );
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

 	weap = spawnArgs.GetString( va( "def_weapon%d", num ) );
	if ( !weap[ 0 ] ) {
		gameLocal.Warning( "Invalid weapon def_weapon%d\n", num );
		return;
	}

	// cycle in-between weapons
	// if a weapon_def has a "def_weapon_swap" keyvalue pointing to another 
	// weapon, hitting that impulse twice will cycle to the target swap. 
	if( num == currentWeapon ) {
		const idDict* weapDict = gameLocal.FindEntityDefDict( weap, false );

		if( weapDict == NULL ) {
			gameLocal.Warning( "Invalid weapon entity %s\n", weap );
			return;
		}

// RAVEN BEGIN
// nrausch: we have no need for weapon swapping on the xenon, and it gets in the way of the quick weapon select ui
#ifndef _XENON
		const char* destWeapon = weapDict->GetString( "def_weapon_swap", NULL );

		if( destWeapon != NULL ) {
			int swapNum = SlotForWeapon( destWeapon );
			if( swapNum == -1 ) {
				gameLocal.Warning( "Swap weapon for %s (%s) is invalid", weap, destWeapon );
			} else {
				num = swapNum;
			}
		} 
#endif
// RAVEN END
	} 

	if ( force || ( inventory.weapons & ( 1 << num ) ) ) {
 		if ( !inventory.HasAmmo( weap ) && !spawnArgs.GetBool( va( "weapon%d_allowempty", num ) ) ) {
 			return;
 		}
		if ( ( previousWeapon >= 0 ) && ( idealWeapon == num ) && ( spawnArgs.GetBool( va( "weapon%d_toggle", num ) ) ) ) {
 			weap = spawnArgs.GetString( va( "def_weapon%d", previousWeapon ) );
 			if ( !inventory.HasAmmo( weap ) && !spawnArgs.GetBool( va( "weapon%d_allowempty", previousWeapon ) ) ) {
 				return;
 			}
			idealWeapon = previousWeapon;
/* NO PDA yet
		} else if ( ( weapon_pda >= 0 ) && ( num == weapon_pda ) && ( inventory.pdas.Num() == 0 ) ) {
			ShowTip( spawnArgs.GetString( "text_infoTitle" ), spawnArgs.GetString( "text_noPDA" ), true );
			return;
*/
		} else {
			idealWeapon = num;
		}
		UpdateHudWeapon();
	}
}

/*
=================
idPlayer::DropItem
=================
*/
idEntity* idPlayer::DropItem ( const char* itemClass, const idDict& customArgs, const idVec3& velocity ) const {
	idDict		args;
	idEntity*	ent;
	args.Set( "classname", itemClass );
	args.Set( "origin", GetPhysics()->GetAbsBounds().GetCenter().ToString ( ) );
	args.Set( "dropped", "1" );
	args.SetFloat ( "angle", 360.0f * gameLocal.random.RandomFloat ( ) );
	args.Copy ( customArgs );
	gameLocal.SpawnEntityDef ( args, &ent );
	if ( !ent ) {
		return NULL;
	}
	
	// If a velocity was given then just use that, otherwise randomly throw it around
	if ( velocity != vec3_origin ) {
		ent->GetPhysics()->SetLinearVelocity ( velocity );
	} else {
		idVec3 vel;
		float  ang;
		ang = idMath::TWO_PI * gameLocal.random.RandomFloat();
		vel[0] = PLAYER_ITEM_DROP_SPEED * idMath::Cos ( ang );
		vel[1] = PLAYER_ITEM_DROP_SPEED * idMath::Sin ( ang );
		vel[2] = PLAYER_ITEM_DROP_SPEED * 2;
		ent->GetPhysics()->SetLinearVelocity ( vel );
	}
	return ent;
}

/*
=================
idPlayer::DropPowerups
=================
*/
void idPlayer::DropPowerups( void ) {
	int			i;
	idEntity*	item;

	assert( !gameLocal.isClient );

	for ( i = 0; i < POWERUP_MAX; i++ ) {
		if ( !(inventory.powerups & ( 1 << i )) ) {
			continue;
		}		
		
		// These powerups aren't dropped
		if ( i >= POWERUP_TEAM_AMMO_REGEN && i <= POWERUP_TEAM_DAMAGE_MOD )
			continue;

		// Don't drop this either with buying enabled.
		if ( i == POWERUP_REGENERATION && gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() )
			continue;

		/// Don't drop arena rune powerups in non-Arena modes
		if( gameLocal.gameType != GAME_ARENA_CTF )
		{
			if( i == POWERUP_AMMOREGEN || 
				i == POWERUP_GUARD || 
				i == POWERUP_DOUBLER || 
				i == POWERUP_SCOUT )
			{
				continue;
			}
		}

		const idDeclEntityDef* def;
		def = GetPowerupDef ( i );
		if ( !def ) {
			continue;
		}

		if( def->dict.GetBool( "nodrop" ) ) {
			continue;
		}

		idDict args;		
		args.SetFloat ( "time", inventory.powerupEndTime[i] == -1 ? -1 : MS2SEC(inventory.powerupEndTime[i]-gameLocal.time) );
		args.SetInt( "instance", GetInstance() );
		item = DropItem ( def->dict.GetString ( "classname" ), args );
		if ( !item ) {
			gameLocal.Warning ( "Player %d failed to drop powerup '%s'", entityNumber, def->dict.GetString ( "classname" ) );
			return;
		}	
	}	
}

/*
=================
idPlayer::ResetFlag
=================
*/
idEntity* idPlayer::ResetFlag ( const char* itemClass, const idDict& customArgs ) const {
	idDict		args;
	idEntity*	ent;
	args.Set( "classname", itemClass );
	args.Set( "origin", GetPhysics()->GetAbsBounds().GetCenter().ToString ( ) );
	args.Set( "dropped", "1" );
	args.Set( "reset", "1" );
	args.Copy ( customArgs );
	gameLocal.SpawnEntityDef ( args, &ent );
	if ( !ent ) {
		return NULL;
	}
	
	return ent;
}

/*
=================
idPlayer::RespawnFlags
=================
*/
void idPlayer::RespawnFlags ( void ) {
	int			i;
	idEntity*	item;

	assert( !gameLocal.isClient );

	for ( i = POWERUP_CTF_MARINEFLAG; i < POWERUP_CTF_ONEFLAG; i++ ) {
		if ( !(inventory.powerups & ( 1 << i )) ) {
			continue;
		}		
		
		const idDeclEntityDef* def;
		def = GetPowerupDef ( i );
		if ( !def ) {
			continue;
		}

		idDict args;		
		args.SetFloat ( "time", inventory.powerupEndTime[i] == -1 ? -1 : MS2SEC(inventory.powerupEndTime[i]-gameLocal.time) );			
		item = ResetFlag ( def->dict.GetString ( "classname" ), args );
		if ( !item ) {
			gameLocal.Warning ( "Player %d failed to drop powerup '%s'", entityNumber, def->dict.GetString ( "classname" ) );
			return;
		}	
	}	
}

/*
=================
DropWeapon
=================
*/
void idPlayer::DropWeapon( void ) {
	idEntity*	item;
	idDict		args;
	const char*	itemClass;

	assert( !gameLocal.isClient );

	if( !gameLocal.isMultiplayer ) {
		return;
	}

// RITUAL BEGIN
// squirrel: don't drop weapons in Buying modes unless "always drop" is on
	if( gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() && !gameLocal.serverInfo.GetBool( "si_dropWeaponsInBuyingModes" ) ) {
		return;
	}
// RITUAL END

 	if ( spectating || weaponGone || !weapon ) {
		return;
	}

	// Make sure the weapon is droppable	
	itemClass = weapon->spawnArgs.GetString ( "def_dropItem" );
	if ( !itemClass || !*itemClass ) {
		return;
	}
		
	// If still alive then the weapon is being thrown so start it a bit in front of the player
	
	// copy over the instance
	args.SetInt( "instance", GetInstance() );

	if ( health > 0 ) {		
		idVec3 forward;
		idVec3 up;
		viewAngles.ToVectors( &forward, NULL, &up );
		args.SetBool( "triggerFirst", true );
		item = DropItem ( itemClass, args, 250.0f*forward + 150.0f*up );
	} else {
		item = DropItem ( itemClass, args );
	}
	
	// Drop the weapon
	if ( !item ) {
		gameLocal.Warning ( "Player %d failed to drop weapon '%s'", entityNumber, weapon->spawnArgs.GetString ( "def_dropItem" ) );
		return;
	}

	// Since this weapon was dropped, replace any starting ammo values with real ammo values
	const idKeyValue* keyval = item->spawnArgs.MatchPrefix( "inv_start_ammo_" );
	idDict newArgs;
	while( keyval ) {
		newArgs.Set( va( "inv_ammo_%s", keyval->GetKey().Right( keyval->GetKey().Length() - 15 ).c_str() ), keyval->GetValue().c_str() );
		item->spawnArgs.Set( keyval->GetKey(), "" );
		keyval = item->spawnArgs.MatchPrefix( "inv_start_ammo_", keyval );
	}

	item->spawnArgs.SetDefaults( &newArgs );

	// Set the appropriate mods on the dropped item
	int		i;
	int		mods;
	idStr	out;
	mods = weapon->GetMods ( );
	for ( i = 0; i < MAX_WEAPONMODS; i ++ ) {
		if ( mods & (1<<i) ) {
			if ( out.Length() ) {
				out += ",";
			}
			out += weapon->spawnArgs.GetString ( va("def_mod%d", i+1) );
		}
	} 
	if ( out.Length() ) {	
		item->spawnArgs.Set ( "inv_weaponmod", out );
	}

	// Make sure the weapon removes itself over time.
	item->PostEventMS ( &EV_Remove, WEAPON_DROP_TIME );

	// Delay aquire since the weapon is being thrown
	if ( health > 0 ) {		
		item->PostEventMS ( &EV_Activate, 500, item );
		inventory.Drop( spawnArgs, item->spawnArgs.GetString( "inv_weapon" ), -1 );
		NextWeapon();
	}
}

/*
===============
idPlayer::ActiveGui
===============
*/
idUserInterface *idPlayer::ActiveGui( void ) {
#ifdef _XENON
	if ( objectiveSystemOpen ) {
		return 0;
	}
#endif
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

	if ( weapon ) {
		weapon->RaiseWeapon();

 		if ( weapon->IsReloading() ) {
 			if ( !pfl.reload ) {
 				pfl.reload = true;
 				SetAnimState ( ANIMCHANNEL_TORSO, "Torso_Reload", 4 );
 				UpdateState();
			}
 		} else {
 			pfl.reload = false;
 		}
	}

	if ( idealWeapon != currentWeapon ) {
 		if ( !weapon || weaponCatchup ) {
 			assert( gameLocal.isClient );
   			weaponGone = false;
   			SetWeapon( idealWeapon );

			weapon->NetCatchup();
			
			SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 0 );
			SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );
			UpdateState();
		} else {
 			if ( weapon->IsReady() || weapon->IsReloading() ) {
 				weapon->PutAway();
 			}
 
 			if ( weapon->IsHolstered() && weaponViewModel ) {
				assert( idealWeapon >= 0 );
				assert( idealWeapon < MAX_WEAPONS );

				SetWeapon( idealWeapon );

				weapon->Raise();
			}
		}
	} else {
		weaponGone = false;
 		if ( weapon->IsHolstered() ) {
 			if ( !weapon->AmmoAvailable() ) {
 				// weapons can switch automatically if they have no more ammo
 				NextBestWeapon();
 			} else {
 				weapon->Raise();

				SetAnimState ( ANIMCHANNEL_TORSO, "Torso_RaiseWeapon", 3 );
   			}
   		}
	} 

	weaponCatchup = false;

	// check for attack
	pfl.weaponFired = false;
 	if ( !influenceActive ) {
 		if ( ( usercmd.buttons & BUTTON_ATTACK ) && !weaponGone ) {
 			FireWeapon();
 		} else if ( oldButtons & BUTTON_ATTACK ) {
 			pfl.attackHeld = false;
 			weapon->EndAttack();
 		}
 	}

	if ( gameLocal.isMultiplayer && spectating ) {
		UpdateHudWeapon();
	}

	// update our ammo clip in our inventory
	if ( gameLocal.GetLocalPlayer() == this && ( currentWeapon >= 0 ) && ( currentWeapon < MAX_WEAPONS ) ) {
		inventory.clip[ currentWeapon ] = weapon->AmmoInClip();
 		if ( hud && ( currentWeapon == idealWeapon ) ) {
 			UpdateHudAmmo( hud );
		}
	}
}

/*
===============
idPlayer::Weapon_Vehicle
===============
*/
void idPlayer::Weapon_Vehicle( void ) {
	StopFiring();
	weapon->LowerWeapon();

	if ( ( usercmd.buttons & BUTTON_ATTACK ) && !( oldButtons & BUTTON_ATTACK ) ) {
		ProcessEvent ( &AI_EnterVehicle, focusEnt.GetEntity() );
		
		ClearFocus ( );
	}
}

/*
===============
idPlayer::Weapon_Usable
===============
*/
void idPlayer::Weapon_Usable( void ) {
	StopFiring();
	weapon->LowerWeapon();

	if ( ( usercmd.buttons & BUTTON_ATTACK ) && !( oldButtons & BUTTON_ATTACK ) ) {
		focusEnt->ProcessEvent ( &EV_Activate, this  );

		ClearFocus ( );
	}
}

/*
===============
idPlayer::Weapon_NPC
===============
*/
void idPlayer::Weapon_NPC( void ) {

	flagCanFire = false;

	if ( idealWeapon != currentWeapon ) {
		Weapon_Combat();
	}

	if ( currentWeapon )	{
		StopFiring();
	}

	if ( !focusEnt || focusEnt->health <= 0 ) {
		ClearFocus ( );
		return;
	}

	if ( talkCursor && ( usercmd.buttons & BUTTON_ATTACK ) && !( oldButtons & BUTTON_ATTACK ) ) {
		buttonMask |= BUTTON_ATTACK;
		if ( !talkingNPC ) {
			idAI *focusAI = static_cast<idAI*>(focusEnt.GetEntity());
			if ( focusAI ) {
				focusAI->TalkTo( this );
				talkingNPC = focusAI;
			}
		}
	} else if ( currentWeapon == SlotForWeapon ( "weapon_blaster" ) ) {
		Weapon_Combat();
	}
}


/*
===============
idPlayer::LowerWeapon
===============
*/
void idPlayer::LowerWeapon( void ) {
	if ( weapon && !weapon->IsHidden() ) {
		weapon->LowerWeapon();
	}
}

/*
===============
idPlayer::RaiseWeapon
===============
*/
void idPlayer::RaiseWeapon( void ) {
	if ( weapon && weapon->IsHidden() ) {
		weapon->RaiseWeapon();
	}
}

/*
===============
idPlayer::WeaponLoweringCallback
===============
*/
void idPlayer::WeaponLoweringCallback( void ) {
	SetAnimState ( ANIMCHANNEL_TORSO, "Torso_LowerWeapon", 3 );
	UpdateState();
}

/*
===============
idPlayer::WeaponRisingCallback
===============
*/
void idPlayer::WeaponRisingCallback( void ) {
	SetAnimState ( ANIMCHANNEL_TORSO, "Torso_RaiseWeapon", 2 );
	UpdateState();
}

/*
===============
idPlayer::Weapon_GUI
===============
*/
void idPlayer::Weapon_GUI( void ) {

	flagCanFire = false;

	if ( !objectiveSystemOpen ) {
		if ( idealWeapon != currentWeapon ) {
			Weapon_Combat();
		}
		StopFiring();
		weapon->LowerWeapon();
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
			if ( updateVisuals && focusEnt && ui == focusUI ) {
				focusEnt->UpdateVisuals();
			}
		}
		if ( gameLocal.isClient ) {
			// we predict enough, but don't want to execute commands
			return;
		}
		if ( focusEnt ) {
			HandleGuiCommands( focusEnt, command );
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

	// clients need to wait till the weapon and it's world model entity
	// are present and synchronized ( weapon.worldModel idEntityPtr to idAnimatedEntity )
	if ( gameLocal.isClient && (!weaponViewModel || !weaponWorldModel) ) {
		return;
	}

	// always make sure the weapon is correctly setup before accessing it
	if ( !weapon ) {
		if ( idealWeapon != -1 ) {
			SetWeapon( idealWeapon );
			weaponCatchup = false;
			assert( weapon );
		} else {
			return;
		}
	}
	
	
	if ( hiddenWeapon && tipUp && usercmd.buttons & BUTTON_ATTACK ) {
		HideTip();
	}

	// Make sure the weapon is in a settled state before preventing thinking due
	// to drag entity.  This way things like hitting reload, zoom, etc, wont crash
	if ( g_dragEntity.GetInteger() ) {
		StopFiring();
		flagCanFire = false;
		if ( weapon ) {
 			weapon->LowerWeapon();
 		}
 		dragEntity.Update( this );
		return;
	} else if ( focusType == FOCUS_CHARACTER) {
		flagCanFire = false;
		Weapon_NPC();
	} else if ( focusType == FOCUS_VEHICLE ) {
		flagCanFire = false;
		Weapon_Vehicle();
	} else if ( focusType == FOCUS_USABLE || focusType == FOCUS_USABLE_VEHICLE ) {
		flagCanFire = false;
		Weapon_Usable();
	} else if ( ActiveGui() ) {
		flagCanFire = false;
		Weapon_GUI();
	} else if ( !hiddenWeapon ) { /* no pda yet || ( ( weapon_pda >= 0 ) && ( idealWeapon == weapon_pda ) ) ) { */
		flagCanFire = true;
		Weapon_Combat();
	}	

	// Range finder for debugging
	if ( g_showRange.GetBool ( ) ) {
		idVec3	start;
		idVec3	end;
		trace_t	tr;
		
		start = GetEyePosition();
		end = start + viewAngles.ToForward() * 50000.0f;
		gameLocal.TracePoint( this, tr, start, end, MASK_SHOT_BOUNDINGBOX, this );

		idVec3 forward;
		idVec3 right;
		idVec3 up;		
		viewAngles.ToVectors ( &forward, &right, &up );
		gameRenderWorld->DrawText( va( "%d qu", ( int )( tr.endpos - start ).Length() ), start + forward * 100.0f + right * 25.0f, .2f, colorCyan, viewAxis );
		gameRenderWorld->DrawText( va( "%d m", ( int )( tr.endpos - start ).Length() ), start + forward * 100.0f + right * 25.0f - up * 6.0f, .2f, colorCyan, viewAxis );
		gameRenderWorld->DrawText( va( "%d 2d", ( int )DistanceTo2d( tr.endpos ) ), start + forward * 100.0f + right * 25.0f - up * 12.0f, .2f, colorCyan, viewAxis );
	}

 	if ( hiddenWeapon ) {
 		weapon->LowerWeapon();
 	}

	// update weapon state, particles, dlights, etc
	weaponViewModel->PresentWeapon( showWeaponViewModel );
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
// RAVEN BEGIN
// ddynerman: multiple clip worlds
			gameLocal.TraceBounds( player, t, start, newOrig, b, MASK_PLAYERSOLID, player );
// RAVEN END
			newOrig.Lerp( start, newOrig, t.fraction );
			SetOrigin( newOrig );
			idAngles angle = player->viewAngles;
			angle[ 2 ] = 0;
			SetViewAngles( angle );
		} else {	
			if( !SelectSpawnPoint( spawn_origin, spawn_angles ) ) {
				return;
			}
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

		if ( player ) {
			UpdateHudWeapon( player->currentWeapon );
		}
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
	} else if ( usercmd.upmove > 0 && player && player != this ) {
		// following someone and hit jump? release.
		SpectateFreeFly( false );
	} else if ( usercmd.buttons & BUTTON_ATTACK && gameLocal.gameType != GAME_TOURNEY ) {
		// tourney mode uses seperate cycling
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

// RAVEN BEGIN
// twhitaker: no more database system
/*	if ( token.Icmp( "updateDB" ) == 0 ) {
		UpdateDatabaseInfo();
		return true;
	}
	
	if ( token.Icmp ( "filterDB" ) == 0 ) {
		if ( !src->ReadToken( &token ) ) {
			return false;
		}

		if ( objectiveSystem ) {
			objectiveSystem->SetStateString ( "dbFilter", token );
			UpdateDatabaseInfo ( );
		}
	}
*/
// RAVEN END

	if ( token.Icmp( "heal" ) == 0 && 
		entityGui->IsType( rvHealingStation::GetClassType() ) &&
		src->ReadToken( &token ) )
	{
		rvHealingStation * station = static_cast< rvHealingStation * >( entityGui );

		if ( token.Icmp( "begin" ) == 0 ) {
			station->BeginHealing( this );
		} else if ( token.Icmp( "end" ) == 0 ) {
			station->EndHealing( );
		} else {
			return false;
		}
		return true;
	}

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
	other = gameLocal.entities[ collision.c.entityNum ];

	// allow client-side prediction of item collisions for simple client effects
	if ( gameLocal.isClient && !other->IsType( idItem::GetClassType() ) ) {
		return false;
	}


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
// RAVEN BEGIN
// rjohnson: temp fix until id corrects slow downs created from constant string lookup
// 			hud->SetStateString( "location", common->GetLocalizedString( "#str_102911" ) );
 			hud->SetStateString( "location", "Unidentified" );
// RAVEN END
 		}
   	}
}

/*
================
idPlayer::UpdateFocus

Searches nearby entities for interactive guis, possibly making one of them
the focus and sending it a mouse move event
================
*/
void idPlayer::UpdateFocus( void ) {
	
	// These only need to be updated at the last tic
	if ( !gameLocal.isLastPredictFrame ) {
		return;
	}
	
	idClipModel*		clipModelList[ MAX_GENTITIES ];
	idClipModel*		clip;
	int					listedClipModels;
	idEntity*			ent;
	idUserInterface*	oldBrackets;
	int					oldTalkCursor;
	int					i;
	int					j;
	idVec3				start;
	idVec3				end;
	trace_t				renderTrace, bboxTrace, allTrace;
	guiPoint_t			pt;

// RAVEN BEGIN
	// mekberg: removed check to see if attack was held.
// RAVEN END

	// No focus during cinimatics
	if ( gameLocal.inCinematic ) {
		return;
	}

	// Focus has a limited time, make sure it hasnt expired
	if ( focusTime && gameLocal.time > focusTime ) {
		ClearFocus ( );
	}

 	if ( spectating ) {
 		return;
  	}

	if ( g_perfTest_noPlayerFocus.GetBool() ) {
		return;
	}

#ifndef _XENON
	cvarSystem->SetCVarInteger( "pm_isZoomed", zoomed ? pm_zoomedSlow.GetInteger() : 0 );
#endif

#ifdef _XENON
	if ( cursor ) {
		if ( weapon ) {
			cursor->SetStateInt( "autoaim", weapon->AllowAutoAim() ? 1 : 0 );
		} else {
			cursor->SetStateInt( "autoaim", 0 );
		}
	}
#endif
	
	// Kill the focus brackets when their time has elapsed
	oldBrackets = focusBrackets;	
	if ( focusBracketsTime && gameLocal.time > focusBracketsTime ) {
		focusBrackets = NULL;
	}

	oldTalkCursor = talkCursor;
	talkCursor = 0;
	start = GetEyePosition();
	end = start + viewAngles.ToForward() * 768.0f;

 	// player identification -> names to the hud
 	if ( gameLocal.isMultiplayer && entityNumber == gameLocal.localClientNum ) {
		trace_t trace;
 		idVec3 end = start + viewAngles.ToForward() * 768.0f;
		gameLocal.TracePoint( this, trace, start, end, MASK_SHOT_BOUNDINGBOX, this );
		// no aim text if player is invisible
 		if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum < MAX_CLIENTS ) && ( !((idPlayer*)gameLocal.entities[ trace.c.entityNum ])->PowerUpActive( POWERUP_INVISIBILITY ) ) ) {
			char* teammateHealth = "";
			idPlayer* p = static_cast<idPlayer*>(gameLocal.entities[ trace.c.entityNum ]);			
			
			if( trace.c.entityNum != aimClientNum ) {
				if( mphud ) {		
					mphud->SetStateString( "aim_text", va( "%s\n%s", gameLocal.userInfo[ trace.c.entityNum ].GetString( "ui_name" ), gameLocal.userInfo[ trace.c.entityNum ].GetString( "ui_clan" ) ) );
					if( gameLocal.IsTeamGame() ) {
						mphud->SetStateInt( "aim_player_team", p->team );
	
						if( p->team == team ) {
							teammateHealth = va( "^iteh %d / ^itea %d", p->health, p->inventory.armor );
						}

						// when looking at a friendly, color the crosshair
						if( cursor ) {
							if( p->team == team ) {
								cursor->HandleNamedEvent( "targetFriendly" );
							} else {
								cursor->HandleNamedEvent( "clearTarget" );
							}
						}
					}
					
					mphud->SetStateString( "aim_teammate_health", teammateHealth );
					

					mphud->HandleNamedEvent( "aim_text" );
					aimClientNum = trace.c.entityNum;
				}	
			} else {
				// update health
				if( gameLocal.IsTeamGame() && p->team == team ) {
					teammateHealth = va( "^iteh %d / ^itea %d", p->health, p->inventory.armor );
				}

				mphud->SetStateString( "aim_teammate_health", teammateHealth );
			}
		} else {
			if( mphud && aimClientNum != -1 ) {
				mphud->HandleNamedEvent( "aim_fade" );
				aimClientNum = -1;
			}
			if( cursor ) {
				cursor->HandleNamedEvent( "clearTarget" );
			}
		}
 	}
 
#ifdef _XENON

		bboxTrace.fraction = -1;
		
		bestEnemy = NULL;
		if ( gameLocal.isMultiplayer ) {
			
			if ( !weapon || !weapon->AllowAutoAim() ) {
				usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : 100 );
				if ( cursor ) {
					cursor->SetStateInt("enemyFocus", 0);
				}
				return;
			}
			
			idEntity *ent = NULL;
			idActor *actor = NULL;
			bool inside;
			bool showDebug = cvarSystem->GetCVarBool("pm_showAimAssist");
			float bDist = cvarSystem->GetCVarInteger("pm_AimAssistDistance");
			float dist;
			idFrustum aimArea;
			float dNear, dFar, size;
			renderView_t *rv = GetRenderView();
			float fovY, fovX;
			// field of view
 			gameLocal.CalcFov( CalcFov( true ), fovX, fovY );
			
		
			dNear = cvarSystem->GetCVarFloat( "r_znear" );
			dFar = cvarSystem->GetCVarInteger("pm_AimAssistDistance");
			
#ifndef _FINAL
			if ( cvarSystem->GetCVarInteger("pm_AimAssistTest") != 0 ) {
				size = dFar * idMath::Tan( DEG2RAD( fovY * 0.5f ) ) * pm_AimAssistFOV.GetFloat()/100.0;
			} else 
#endif
			{
				size = dFar * idMath::Tan( DEG2RAD( fovY * 0.5f ) ) * weapon->GetAutoAimFOV()/100.0;
			}		
			
			aimArea.SetOrigin( GetEyePosition() );
			aimArea.SetAxis( viewAngles.ToMat3() );
			aimArea.SetSize( dNear, dFar, size, size );
		

			if ( showDebug ) {
				gameRenderWorld->DebugFrustum(colorRed, aimArea, false, 20);
			}

			idLinkList<idEntity> *entities = &gameLocal.snapshotEntities;
			if ( gameLocal.isServer ) {
				entities = &gameLocal.spawnedEntities;
			}
			ent = entities->Next();
			while ( ent != NULL ) {

				bool isOk = true;
									
				if ( !ent->IsType(idPlayer::GetClassType()) ) {
					isOk = false;
				} else if ( gameLocal.IsTeamGame() && ((idPlayer*)ent)->team == team ) {
					isOk = false;
				} else if ( ((idPlayer*)ent)->instance != instance ) {
					isOk = false;
				} else if ( ((idPlayer*)ent)->spectating ) {
					isOk = false;
				} else if ( !idStr::Icmp(name.c_str(),ent->name.c_str()) ) {
					isOk = false;
				} else if ( ent->health <= 0 ) {
					isOk = false;
				}

				if ( !isOk ) {
					if ( gameLocal.isServer ) {
						ent = ent->spawnNode.Next();
					} else {
						ent = ent->snapshotNode.Next();
					}
					continue;
				}
				
				const idBounds &bounds = ent->GetPhysics()->GetAbsBounds();
				if ( showDebug ) {
					gameRenderWorld->DebugBounds(colorGreen, bounds, vec3_origin, 20);
				}
				inside = aimArea.IntersectsBounds(bounds);
				if ( inside ) {
					dist = bounds.ShortestDistance(GetEyePosition());
					if ( bDist > dist ) {
						if ( bboxTrace.fraction == -1 ) {
							gameLocal.TracePoint( this, bboxTrace, start, end, MASK_SHOT_BOUNDINGBOX, this );
						}
						if ( ( bboxTrace.fraction < 1.0f ) && ( bboxTrace.c.entityNum != ent->entityNumber ) ) {
							idVec3 v = end - start;
							if ( ((v.Length() * bboxTrace.fraction) + 5.0f) >= dist ) {
								// close enough to the bbox
								bDist = dist;
								bestEnemy = (idActor *)ent;
							}
						} else {
							// Didn't hit anything, so this must be in view
							bDist = dist;
							bestEnemy = (idActor *)ent;
						}
					}
				}					

				if ( gameLocal.isServer ) {
					ent = ent->spawnNode.Next();
				} else {
					ent = ent->snapshotNode.Next();
				}
			}
			
			if ( !gameLocal.isMultiplayer || (gameLocal.isMultiplayer && entityNumber == gameLocal.localClientNum) ) {
				if ( bestEnemy ) {
					if ( cursor ) {
						cursor->SetStateInt("enemyFocus", 1);
						idVec3 altEnd = bestEnemy->GetPhysics()->GetOrigin();
						altEnd[2]+=35.0f; // origin is always below the monster's feet for whatever reason, so aim at this
						trace_t trace;
					
						gameLocal.TracePoint( this, trace, start, altEnd, MASK_SHOT_BOUNDINGBOX, this );
						if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == bestEnemy->entityNumber ) ) {
							int slow = pm_AimAssistSlow.GetInteger();
							usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : slow );
						} else {
							usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : 100 );
						}
					} else {
						usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : 100 );
						if ( cursor ) {
							cursor->SetStateInt("enemyFocus", 0);
						}
					}
				} else {
					usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : 100 );
					if ( cursor ) {
						cursor->SetStateInt("enemyFocus", 0);
					}
				}
			}
			return;
		}

#endif

	idBounds bounds( start );
	bounds.AddPoint( end );
	
	listedClipModels = gameLocal.ClipModelsTouchingBounds( this, bounds, -1, clipModelList, MAX_GENTITIES );

	// Do autoaim
	
	
//RAVEN BEGIN
//asalmon: auto-aim for Xenon, finds a best enemy and changes the crosshairs.
#ifdef _XBOX
	
		bool doAimAssist = cvarSystem->GetCVarBool("pm_AimAssist") || gameLocal.isMultiplayer;
		
		bestEnemy = NULL;
		bool showDebug = cvarSystem->GetCVarBool("pm_showAimAssist");
		float bDist = cvarSystem->GetCVarInteger("pm_AimAssistDistance");
		float dist;
		idFrustum aimArea;
		float dNear, dFar, size;
		renderView_t *rv = GetRenderView();
		float fovY, fovX;
		
		if ( weapon && doAimAssist ) {
		
			// field of view
 			gameLocal.CalcFov( CalcFov( true ), fovX, fovY );
		
			dNear = cvarSystem->GetCVarFloat( "r_znear" );
			dFar = bDist;
#ifndef _FINAL
			if ( cvarSystem->GetCVarInteger("pm_AimAssistTest") != 0 ) {
				size = dFar * idMath::Tan( DEG2RAD( fovY * 0.5f ) ) * pm_AimAssistFOV.GetFloat()/100.0;
			} else 
#endif
			{
				size = dFar * idMath::Tan( DEG2RAD( fovY * 0.5f ) ) * weapon->GetAutoAimFOV()/100.0;
			}
			aimArea.SetOrigin( GetEyePosition() );
			aimArea.SetAxis( viewAngles.ToMat3() );
			aimArea.SetSize( dNear, dFar, size, size );
	
			if ( showDebug ) {
				gameRenderWorld->DebugFrustum( colorRed, aimArea, false, 20 );
			}
		}

#endif
//RAVEN END

	// dluetscher: added optimization to eliminate redundant traces
	renderTrace.fraction = -1;
	bboxTrace.fraction = -1;
	allTrace.fraction = -1;
	
	// no pretense at sorting here, just assume that there will only be one active
	// gui within range along the trace
	bool wasTargetFriendly = targetFriendly;
	targetFriendly = false;
	for ( i = 0; i < listedClipModels; i++ ) {
		clip = clipModelList[ i ];
		ent = clip->GetEntity();
//RITUAL BEGIN
//singlis: if ent is null, continue;
		if(ent == NULL || ent->IsHidden()) {
			continue;
		}
//RITUAL END

		float focusLength = (ent->GetPhysics()->GetOrigin() - start).LengthFast() - ent->GetPhysics()->GetBounds().GetRadius();
		//  SP only
		//	basically what was happening was that heads, which are an idAFAttachment, were being used for the focusLength
		//	calculations, but that generates a different focus length than the body would (when you scan the crosshair back 
		//	and forth between the head and body).  This ends up looking like a bug when you are right at the threshold where
		//	the body will display the name and rank, but doesn't when you pitch up to aim at the head.  Hence, using the body
		//	for this special case.
		if ( !gameLocal.isMultiplayer && ent->IsType( idAFAttachment::GetClassType() )) {
			idEntity *body = static_cast<idAFAttachment *>( ent )->GetBody();
			if ( body && body->IsType( idAI::GetClassType()) ) 	{
				focusLength = (body->GetPhysics()->GetOrigin() - start).LengthFast() - body->GetPhysics()->GetBounds().GetRadius();
			}
		}
		
		bool isAI = ent->IsType( idAI::GetClassType() );
		bool isFriendly = false;
		
		if ( isAI ) {
			isFriendly = (static_cast<idAI *>( ent )->team == team);
		}
		
		//change crosshair color if over a friendly
		if ( !gameLocal.isMultiplayer 
			&& focusType == FOCUS_NONE 
			&& !g_crosshairCharInfoFar.GetBool() ) {
			if ( focusLength < 512 ) {
				bool newTargetFriendly = false;
				if ( isAI && isFriendly ) {
					newTargetFriendly = true;
				} else if ( ent->IsType( idAFAttachment::GetClassType() ) ) {
					idEntity *body = static_cast<idAFAttachment *>( ent )->GetBody();
					if ( body && body->IsType( idAI::GetClassType() ) && ( static_cast<idAI *>( body )->team == team ) ) {
						newTargetFriendly = true;
					}
				}
				if ( newTargetFriendly ) {
				
					// dluetscher: added optimization to eliminate redundant traces
					if ( renderTrace.fraction == -1 ) {
						gameLocal.TracePoint( this, renderTrace, start, end, MASK_SHOT_RENDERMODEL, this );
					}
					if ( ( renderTrace.fraction < 1.0f ) && ( renderTrace.c.entityNum == ent->entityNumber ) ) {
						targetFriendly = true;
						if( cursor && !wasTargetFriendly ) {
							cursor->HandleNamedEvent( "showCrossBuddy" );
						}
					}
				}
			}
		}

// RAVEN BEGIN

#ifdef _XENON
		if ( doAimAssist ) {
			if ( isAI && !isFriendly && (ent->health > 0) ) {
				
				if ( idStr::Icmp( name.c_str(),ent->name.c_str() ) != 0 ) {
				
					const idBounds &bounds = ent->GetPhysics()->GetAbsBounds();
					//if ( showDebug ) {
						//gameRenderWorld->DebugBounds(colorGreen, bounds, vec3_origin, 20);
					//}
					bool inside = aimArea.IntersectsBounds(bounds);
					
					if ( inside ) {
						dist = bounds.ShortestDistance(GetEyePosition());
						if ( bDist > dist ) {
							if ( bboxTrace.fraction == -1 ) {
								gameLocal.TracePoint( this, bboxTrace, start, end, MASK_SHOT_BOUNDINGBOX, this );
							}
							if ( ( bboxTrace.fraction < 1.0f ) && ( bboxTrace.c.entityNum != ent->entityNumber ) ) {
								idVec3 v = end - start;
								if ( ((v.Length() * bboxTrace.fraction) + 5.0f) >= dist ) {
									// close enough to the bbox
									bDist = dist;
									bestEnemy = (idActor *)ent;
								}
							} else {
								// Didn't hit anything, so this must be in view
								bDist = dist;
								bestEnemy = (idActor *)ent;
							}
						}
					}					
				}
			}
		}
#endif

// mekberg: allowFocus removed
		if ( focusLength < (g_crosshairCharInfoFar.GetBool()?256.0f:80.0f) ) {
// RAVEN END
			if ( ent->IsType( idAFAttachment::GetClassType() ) ) {
				idEntity *body = static_cast<idAFAttachment *>( ent )->GetBody();
				if ( body && body->IsType( idAI::GetClassType() ) && ( static_cast<idAI *>( body )->GetTalkState() >= TALK_OK ) ) {

					// dluetscher: added optimization to eliminate redundant traces
					if ( renderTrace.fraction == -1 ) {
						gameLocal.TracePoint( this, renderTrace, start, end, MASK_SHOT_RENDERMODEL, this );
					}
					if ( ( renderTrace.fraction < 1.0f ) && ( renderTrace.c.entityNum == ent->entityNumber ) ) {
						SetFocus ( FOCUS_CHARACTER, FOCUS_TIME, body, NULL );
						if ( focusLength < 80.0f ) {
							talkCursor = 1;
						}
						break;
					}
				}
				continue;
			}

			if ( isAI ) {
 				if ( static_cast<idAI *>( ent )->GetTalkState() >= TALK_OK ) {

					// dluetscher: added optimization to eliminate redundant traces
					if ( renderTrace.fraction == -1 ) {
						gameLocal.TracePoint( this, renderTrace, start, end, MASK_SHOT_RENDERMODEL, this );
					}
					if ( ( renderTrace.fraction < 1.0f ) && ( renderTrace.c.entityNum == ent->entityNumber ) ) {
						SetFocus ( FOCUS_CHARACTER, FOCUS_TIME, ent, NULL );
						if ( focusLength < 80.0f ) {
							talkCursor = 1;
						}
						break;
					}
				}
				continue;
			}
		}

		if ( focusLength < 80.0f ) {
			if ( ent->IsType( rvVehicle::GetClassType() ) ) {
				rvVehicle* vehicle = static_cast<rvVehicle*>(ent);
					// dluetscher: added optimization to eliminate redundant traces
				if ( bboxTrace.fraction == -1 ) {
					gameLocal.TracePoint( this, bboxTrace, start, end, MASK_SHOT_BOUNDINGBOX, this );
				}
				if ( ( bboxTrace.fraction < 1.0f ) && ( bboxTrace.c.entityNum == ent->entityNumber ) && ((end - start).Length() * bboxTrace.fraction < vehicle->FocusLength()) ) {
					//jshepard: locked or unusable vehicles
					if ( !vehicle->IsLocked() && vehicle->HasOpenPositions() ) {
						SetFocus ( FOCUS_VEHICLE, FOCUS_TIME, ent, NULL );
					} else {
						SetFocus ( FOCUS_LOCKED_VEHICLE, FOCUS_TIME, ent, NULL );
					}
					break;
				}
			} 

			//jshepard: unusable vehicle
			if( ent->spawnArgs.GetBool( "unusableVehicle", "0" ) ) {
				if ( allTrace.fraction == -1.f ) {
					gameLocal.TracePoint( this, allTrace, start, end, MASK_ALL, this );
				}
				if ( ( allTrace.fraction < 1.0f ) && ( allTrace.c.entityNum == ent->entityNumber ) ) {
					SetFocus ( FOCUS_LOCKED_VEHICLE, FOCUS_TIME, ent, NULL );
					break;
				}
			}
			// Usable entities are last
			if ( ent->fl.usable ) {
			//jshepard: fake vehicles
				if ( allTrace.fraction == -1.f ) {
					gameLocal.TracePoint( this, allTrace, start, end, MASK_ALL, this );
				}
				if ( ( allTrace.fraction < 1.0f ) && ( allTrace.c.entityNum == ent->entityNumber ) ) {
					if( ent->spawnArgs.GetBool("crosshair_vehicle"))	{
						SetFocus ( FOCUS_USABLE_VEHICLE, FOCUS_TIME, ent, NULL );
					}	else	{
						SetFocus ( FOCUS_USABLE, FOCUS_TIME, ent, NULL );
					}
					break;
				}
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
			idUserInterface*	ui = NULL;			
			renderEntity_t*		focusGUIrenderEntity;
			
			focusGUIrenderEntity = ent->GetRenderEntity();
			if ( !focusGUIrenderEntity ) {
				continue;
			}

			if ( pt.guiId >= 1 && pt.guiId <= MAX_RENDERENTITY_GUI ) {
				ui = focusGUIrenderEntity->gui[ pt.guiId-1 ];
			}
			
			if ( ui == NULL || !ui->IsInteractive ( ) ) {
				continue;
			}

			// All focused guis get brackets
			focusBrackets = ui;

			// Any GUI that is too far away will just get bracket focus so the player can still shoot 
			// but still see which guis are interractive
			if ( focusLength > 300.0f ) {
				ClearFocus ( );
				break;
			} else if ( focusLength > 80.0f ) {
				ClearFocus ( );
				focusType = FOCUS_BRACKETS;
#ifdef _XENON
				// Hide the "press whatever" text
				hud->SetStateInt( "GUIIsNotUsingDPad", 3 );
				hud->SetStateInt( "hideInteractive", 1 );
#endif
				break;
			}

			// If this is the first time this gui was activated then set up some things
			if ( focusEnt != ent ) {
				const idKeyValue* kv;
				
				// new activation
				// going to see if we have anything in inventory a gui might be interested in
				// need to enumerate inventory items
				ui->SetStateInt( "inv_count", inventory.items.Num() );
				for ( j = 0; j < inventory.items.Num(); j++ ) {
					idDict *item = inventory.items[ j ];
					const char *iname = item->GetString( "inv_name" );
					iname = common->GetLocalizedString( iname );

					const char *iicon = item->GetString( "inv_icon" );
					const char *itext = item->GetString( "inv_text" );

					ui->SetStateString( va( "inv_name_%i", j), iname );
					ui->SetStateString( va( "inv_icon_%i", j), iicon );
					ui->SetStateString( va( "inv_text_%i", j), itext );
					kv = item->MatchPrefix("inv_id", NULL);
					if ( kv ) {
						ui->SetStateString( va( "inv_id_%i", j ), kv->GetValue() );
					}
					ui->SetStateInt( iname, 1 );
				}

				for( j = 0; j < inventory.pdaSecurity.Num(); j++ ) {
					const char *p = inventory.pdaSecurity[ j ];

					if ( p && *p ) {
						ui->SetStateInt( p, 1 );
					}
				}

				ui->SetStateString( "player_health", va("%i", health ) );
				ui->SetStateString( "player_armor", va( "%i%%", inventory.armor ) );

				kv = ent->spawnArgs.MatchPrefix( "gui_", NULL );
				while ( kv ) {
					ui->SetStateString( kv->GetKey(), common->GetLocalizedString( kv->GetValue() ) );
					kv = ent->spawnArgs.MatchPrefix( "gui_", kv );
				}
			}

			// clamp the mouse to the corner
			const char*	command;
			sysEvent_t	ev;
 			ev = sys->GenerateMouseMoveEvent( -2000, -2000 );
			command = ui->HandleEvent( &ev, gameLocal.time );
  			HandleGuiCommands( ent, command );

			// move to an absolute position
 			ev = sys->GenerateMouseMoveEvent( pt.x * SCREEN_WIDTH, pt.y * SCREEN_HEIGHT );
			command = ui->HandleEvent( &ev, gameLocal.time );
 			HandleGuiCommands( ent, command );
			
#ifdef _XENON
			int usepad = 0;
			if ( focusUI ) {
				idWindow *dwin = ui->GetDesktop();
				if ( dwin ) {
					idWinVar *wv = dwin->GetWinVarByName("dpadGUI");
					if ( wv ) {
						usepad = atoi(wv->c_str());
					}
				}
			}
			hud->SetStateInt( "GUIIsNotUsingDPad", (usepad) ? 0 : 1 );
			hud->SetStateInt( "hideInteractive", 0 );
#endif

			SetFocus ( FOCUS_GUI, FOCUS_GUI_TIME, ent, ui );
			break;
		}
	}

#ifdef _XENON
	if ( !gameLocal.isMultiplayer || (gameLocal.isMultiplayer && entityNumber == gameLocal.localClientNum) ) {
		
		if ( !weapon || !weapon->AllowAutoAim() ) {
			if ( cursor ) {
				cursor->SetStateInt("enemyFocus", 0);
			}
			usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : 100 );
		} else {		
		
			if ( bestEnemy && doAimAssist ) {
				if ( cursor ) {
					cursor->SetStateInt("enemyFocus", 1);
				}
				trace_t trace;
				idVec3 altEnd = bestEnemy->GetPhysics()->GetOrigin();
				altEnd[2]+=35.0f; // origin is always below the monster's feet for whatever reason, so aim at this
				gameLocal.TracePoint( this, trace, start, altEnd, MASK_SHOT_BOUNDINGBOX, this );
				if ( ( trace.fraction < 1.0f ) && ( trace.c.entityNum == bestEnemy->entityNumber ) ) {
					int slow = pm_AimAssistSlow.GetInteger();
					usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : slow );
				} else {
					usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : 100 );
				}
			} else {
				usercmdGen->SetSlowJoystick( zoomed ? pm_zoomedSlow.GetInteger() : 100 );
				if ( cursor ) {
					cursor->SetStateInt("enemyFocus", 0);
				}
			}
		}
	}
#endif

	if ( !gameLocal.isMultiplayer || (gameLocal.isMultiplayer && entityNumber == gameLocal.localClientNum) ) {
		if ( wasTargetFriendly && !targetFriendly && focusType == FOCUS_NONE ) {
			if ( cursor ) {
				cursor->HandleNamedEvent ( WeaponIsEnabled() ? "showCrossCombat" : "crossHide" );
			}
		}
	}

	// Update the focus brackets within the hud
	if ( focusBrackets && hud ) {
		if (focusType == FOCUS_BRACKETS || focusType == FOCUS_GUI ) {
			if ( !oldBrackets || focusBracketsTime ) {
				hud->HandleNamedEvent ( "showBrackets" );			
			}
			focusBracketsTime = 0;
		} else {
			if ( focusBracketsTime == 0 ) {
				hud->HandleNamedEvent ( "fadeBrackets" );
				focusBracketsTime = gameLocal.time + 2000;
			}
		}
		focusBrackets->SetStateBool ( "2d_calc", true );
	}	

 	if ( cursor && ( oldTalkCursor != talkCursor ) ) {
 		cursor->SetStateInt( "talkcursor", talkCursor );
 	}
 
}

/*
================
idPlayer::ClearFocus
================
*/
void idPlayer::ClearFocus ( void ) {
	SetFocus ( FOCUS_NONE, 0, NULL, NULL );
}

void idPlayer::UpdateFocusCharacter( idEntity* newEnt ) {
	if ( !cursor ) {
		return;
	}
	// Handle character interaction
	cursor->SetStateString( "npc", common->GetLocalizedString(newEnt->spawnArgs.GetString( "npc_name", "Joe" )) );
	cursor->SetStateString( "npcdesc", common->GetLocalizedString(newEnt->spawnArgs.GetString( "npc_description", "" )) );
	if ( newEnt->IsType( rvAIMedic::GetClassType() ) ) {
		if ( ((rvAIMedic*)newEnt)->isTech ) {
			cursor->SetStateInt( "npc_medictech", 2 );
		} else {
			cursor->SetStateInt( "npc_medictech", 1 );
		}
	} else {
		cursor->SetStateInt( "npc_medictech", 0 );
	}
}
/*
================
idPlayer::SetFocus
================
*/
void idPlayer::SetFocus ( playerFocus_t newType, int _focusTime, idEntity* newEnt, idUserInterface* newUI ) {
	const char* command;

	// Handle transitions from one user interface to another or to none
	if ( newUI != focusUI ) {
		if ( focusUI ) {		
			command = focusUI->Activate( false, gameLocal.time );
			HandleGuiCommands( focusEnt, command );
			StartSound( "snd_guiexit", SND_CHANNEL_ANY, 0, false, NULL );
		}			
		if ( newUI ) {
			command = newUI->Activate( true, gameLocal.time );
			HandleGuiCommands( newEnt, command );
			StartSound( "snd_guienter", SND_CHANNEL_ANY, 0, false, NULL );

			// Hide the weapon when a gui is being used
			/*
			if ( weapon ) {
				weapon->Hide();				
			}
			*/
		}/*
		 else if ( weapon ) {
			// Show the weapon since it was hidden when a gui first got focus
			weapon->Show();
		}		
		*/
	}

	//jshepard: the medic/tech crosshair won't update unless handleNamedEvent is called. I moved this outside
	//of the switch below because the focus type is the same when moving directly from marine to marine, but the
	//medic/tech status may change.
	if ( newType == FOCUS_CHARACTER ) {
		if ( newEnt != focusEnt ) {
			UpdateFocusCharacter( newEnt );
			cursor->HandleNamedEvent ( "showCrossTalk" );
		}
	}
	// Show the appropriate cursor for the current focus type
	if ( cursor && ( focusType != newType ) ) {
		switch ( newType ) {
			case FOCUS_VEHICLE:
			case FOCUS_USABLE_VEHICLE:
				cursor->HandleNamedEvent ( "showCrossVehicle" );
				break;
			case FOCUS_LOCKED_VEHICLE:
				cursor->HandleNamedEvent ( "showCrossVehicleLocked" );
				break;
			case FOCUS_USABLE:
				cursor->HandleNamedEvent ( "showCrossUsable" );
				break;
			case FOCUS_GUI:
				cursor->HandleNamedEvent ( "showCrossGui" );
				break;
			case FOCUS_CHARACTER:
				if ( newEnt != focusEnt ) {
					UpdateFocusCharacter( newEnt );
				}
				cursor->HandleNamedEvent ( "showCrossTalk" );
				break;
			default:
				// Make sure the weapon is shown in the default state
// RAVEN BEGIN
// abahr: don't do this if weapons are disabled
				cursor->HandleNamedEvent ( WeaponIsEnabled() ? "showCrossCombat" : "crossHide" );
// RAVEN END
				break;
		}
	}


	focusType = newType;
	focusEnt  = newEnt;
	focusUI	  = newUI;
	
	if ( focusType == FOCUS_NONE ) {
		focusTime = 0;
	} else {
		focusTime = gameLocal.time + _focusTime;
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
	float		dist;
	float		vel, acc;
	float		t;
	float		a, b, c, den;
	waterLevel_t waterLevel;
 	bool		noDamage;

	pfl.softLanding = false;
	pfl.hardLanding = false;

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
 			break;
		}
	}

	//jshepard: no falling damage if falling damage is disabled
	if( pfl.noFallingDamage )	{
		return;
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

	// Some bug in the player movement code causes double fall damage on the 2nd frame in certain situations, 
	// so hack around this.  Basically, if the next frame's velocity is going to be great enough to cause damage,
	// don't do the damage this frame. This feels a lot safer than messing with things that could break player 
	// movement in worse ways.
	if ( gameLocal.isMultiplayer ) {
		
		float vel2 = GetPhysics()->GetLinearVelocity() * -gravityNormal;

		a = acc / 2.0f;
		b = vel2;
		c = -dist;

		den = b * b - 4.0f * a * c;
		if ( den < 0 ) {
			return;
		}
		t = ( -b - idMath::Sqrt( den ) ) / ( 2.0f * a );

		float delta2 = vel2 + t * acc;
		delta2 = delta2 * delta2 * 0.0001;
		
		if ( delta2 > softFallDelta ) {
			return;
		}
	}
	

	// ddynerman: moved height delta selection to player def
	if ( delta > fatalFallDelta && fatalFallDelta > 0.0f ) {
		pfl.hardLanding = true;
		landChange = -32;
		landTime = gameLocal.time;
 		if ( !noDamage ) {
 			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
 			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_fatalfall", 1.0f, 0 );
 		}
	} else if ( delta > hardFallDelta && hardFallDelta > 0.0f ) {
		pfl.hardLanding = true;
		landChange	= -24;
		landTime	= gameLocal.time;
 		if ( !noDamage ) {
 			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
 			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_hardfall", 1.0f, 0 );
 		}
	} else if ( delta > softFallDelta && softFallDelta > 0.0f ) {
		pfl.softLanding = true;
 		landChange	= -16;
 		landTime	= gameLocal.time;
 		if ( !noDamage ) {
 			pain_debounce_time = gameLocal.time + pain_delay + 1;  // ignore pain since we'll play our landing anim
 			Damage( NULL, NULL, idVec3( 0, 0, -1 ), "damage_softfall", 1.0f, 0 );
		}
	} else if ( delta > noFallDelta && noFallDelta > 0.0f ) {
		pfl.softLanding = true;
		landChange	= -8;
		landTime	= gameLocal.time;
	}

	// ddynerman: sometimes the actual landing animation is pre-empted by another animation (i.e. sliding, moving forward)
	// so we play the landing sound here instead of relying on the anim
	if( pfl.hardLanding ) {
		StartSound ( "snd_land_hard", SND_CHANNEL_ANY, 0, false, NULL );		
		StartSound ( "snd_land_hard_pain", SND_CHANNEL_ANY, 0, false, NULL );	
	} else if ( pfl.softLanding ) {
		// todo - 2 different landing sounds for variety?
		StartSound ( "snd_land_soft", SND_CHANNEL_ANY, 0, false, NULL );				 
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

	if ( noclip ) {
		velocity.Zero ( );
	}
   
	gravityDir = physicsObj.GetGravityNormal();
	vel = velocity - ( velocity * gravityDir ) * gravityDir;
	xyspeed = vel.LengthFast();
	
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
		bobCycle = (int)( old + bobmove * gameLocal.GetMSec() ) & 255;
		bobFoot = ( bobCycle & 128 ) >> 7;
		bobfracsin = idMath::Fabs( idMath::Sin( ( bobCycle & 127 ) / 127.0 * idMath::PI ) );
	}

	// calculate angles for view bobbing
	viewBobAngles.Zero();

	// no view bob at all in MP while zoomed in
	if( gameLocal.isMultiplayer && IsZoomed() ) {
		bobCycle = 0;
		bobFoot = 0;
		bobfracsin = 0;	
		return;
	}

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
// RAVEN BEGIN
// abahr: added gravity
	viewBob += bob * -gravityDir;
// RAVEN END

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
	UpdateDeltaViewAngles(angles);
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

	if ( !noclip && ( gameLocal.inCinematic || privateCameraView || gameLocal.GetCamera() || influenceActive == INFLUENCE_LEVEL2 ) ) {
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
//	if( gameLocal.localClientNum == entityNumber ) {
//		gameLocal.Printf( "BEFORE VIEWANGLES: %s\n", viewAngles.ToString() );
//		gameLocal.Printf( "\tUSERCMD: <%d, %d, %d>\n", usercmd.angles[ 0 ], usercmd.angles[ 1 ], usercmd.angles[ 2 ] );
//		gameLocal.Printf( "\tDELTAVIEW: %s\n", deltaViewAngles.ToString() );
//	}
	for ( i = 0; i < 3; i++ ) {
		cmdAngles[i] = SHORT2ANGLE( usercmd.angles[i] );
		if ( influenceActive == INFLUENCE_LEVEL3 ) {
			viewAngles[i] += idMath::ClampFloat( -1.0f, 1.0f, idMath::AngleDelta( idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i]) + deltaViewAngles[i] ) , viewAngles[i] ) );
		} else {
			viewAngles[i] = idMath::AngleNormalize180( SHORT2ANGLE( usercmd.angles[i] ) + deltaViewAngles[i] );
		}
	}
	if ( !centerView.IsDone( gameLocal.time ) ) {
		viewAngles.pitch = centerView.GetCurrentValue( gameLocal.time );
	}
//	if( gameLocal.localClientNum == entityNumber ) {
//		gameLocal.Printf( "AFTER VIEWANGLES: %s\n", viewAngles.ToString() );
//	}

	// clamp the pitch
	if ( noclip ) {
		if ( viewAngles.pitch > 89.0f ) {
			// don't let the player look down more than 89 degrees while noclipping
			viewAngles.pitch = 89.0f;
		} else if ( viewAngles.pitch < -89.0f ) {
			// don't let the player look up more than 89 degrees while noclipping
			viewAngles.pitch = -89.0f;
		}
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

// RAVEN BEGIN
// abahr
/*
==============
idPlayer::UpdateGravity
==============
*/
void idPlayer::UpdateGravity( void ) {
	GetPhysics()->SetGravity( gameLocal.GetCurrentGravity(this) );
}
// RAVEN END

/*
==============
idPlayer::ToggleObjectives
==============
*/
void idPlayer::ToggleObjectives ( void ) {
// RAVEN BEGIN
// mekberg: allow disabling of objectives.
	if ( objectiveSystem == NULL || !objectivesEnabled ) {
		return;
	}
// RAVEN END

	if ( !objectiveSystemOpen ) {
		int j, c = inventory.items.Num();
		objectiveSystem->SetStateInt( "inv_count", c );
		for ( j = 0; j < c; j++ ) {
			idDict *item = inventory.items[j];
			if ( !item->GetBool( "inv_pda" ) ) {
				const char *iname = item->GetString( "inv_name" );
				iname = common->GetLocalizedString( iname );

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
			int weapstate = 0;
			if ( inventory.weapons & ( 1 << j ) ) {
				const char *weap = spawnArgs.GetString( weapnum );
				if ( weap && *weap ) {
					weapstate++;
				}
			}
			objectiveSystem->SetStateInt( weapnum, weapstate );
		}

		UpdateObjectiveInfo();
		objectiveSystem->Activate( true, gameLocal.time );
		objectiveSystem->HandleNamedEvent( "wristcommShow" );
	} else {
		objectiveSystem->Activate( false, gameLocal.time );
		objectiveSystem->HandleNamedEvent( "wristcommHide" );
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
void idPlayer::Spectate( bool spectate, bool force ) {
 	idBitMsg	msg;
 	byte		msgBuf[MAX_EVENT_PARAM_SIZE];
 
 	// track invisible player bug
 	// all hiding and showing should be performed through Spectate calls
 	// except for the private camera view, which is used for teleports
	// ok for players in other instances to be hidden
	assert( force || ( teleportEntity.GetEntity() != NULL ) || ( IsHidden() == spectating ) || ( IsHidden() && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance ) );

	if ( spectating == spectate && !force ) {
		return;
	}

	bool inOtherInstance = gameLocal.isClient && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance;

	spectating = spectate;

	// don't do any smoothing with this snapshot
	predictedFrame = gameLocal.framenum;
 
 	if ( gameLocal.isServer ) {
 		msg.Init( msgBuf, sizeof( msgBuf ) );
 		msg.WriteBits( spectating, 1 );
 		ServerSendEvent( EVENT_SPECTATE, &msg, false, -1 );
 	}
 
	// on the client, we'll get spectate messages about clients in other instances - always assume they are spectating
	if ( spectating || inOtherInstance ) {
		// join the spectators
		delete clientHead;
		clientHead = NULL;

		ClearPowerUps();
		spectator = this->entityNumber;
		Init();
		StopRagdoll();
		SetPhysics( &physicsObj );
		common->DPrintf( "idPlayer::Spectate() - Disabling clip for %d '%s' - spectate: %d force: %d\n", entityNumber, GetUserInfo()->GetString( "ui_name" ), spectate, force );
		physicsObj.DisableClip();
		Hide();
		Event_DisableWeapon();

		// remove the weapon
		delete weapon;
		weapon = NULL;
		if ( !gameLocal.isClient ) {
			delete weaponViewModel;
			weaponViewModel = NULL;
			delete weaponWorldModel;
			weaponWorldModel = NULL;
		}
	} else {
		// put everything back together again
		UpdateModelSetup( true );
		currentWeapon = -1;	// to make sure the def will be loaded if necessary
		Show();
		Event_EnableWeapon();
	}

	SetClipModel( inOtherInstance );

	if ( inOtherInstance ) {
		// Normally idPlayer::Move() gets called to set the contents to 0, but we don't call
		// move on players not in our snap, so we need to set it manually here.
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_SPECTATOR );
		physicsObj.SetClipMask( MASK_DEADSOLID );
	}
}

/*
==============
idPlayer::SetClipModel
==============
*/
void idPlayer::SetClipModel( bool forceSpectatorBBox ) {
	idBounds bounds;

	common->DPrintf( "idPlayer::SetClipModel() - Called on %d '%s' forceSpectatorBBox = %d spectate = %d instance = %d local instance = %d\n", entityNumber, GetUserInfo()->GetString( "ui_name" ), forceSpectatorBBox, spectating, instance, gameLocal.GetLocalPlayer() ? gameLocal.GetLocalPlayer()->GetInstance() : -1 );
	if ( spectating || forceSpectatorBBox ) {
 		bounds = idBounds( vec3_origin ).Expand( pm_spectatebbox.GetFloat() * 0.5f );
	} else {
		bounds[0].Set( -pm_bboxwidth.GetFloat() * 0.5f, -pm_bboxwidth.GetFloat() * 0.5f, 0 );
		bounds[1].Set( pm_bboxwidth.GetFloat() * 0.5f, pm_bboxwidth.GetFloat() * 0.5f, pm_normalheight.GetFloat() );
	}
	// the origin of the clip model needs to be set before calling SetClipModel
	// otherwise our physics object's current origin value gets reset to 0
	idClipModel *newClip;
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
	RV_PUSH_HEAP_MEM_AUTO(p0,this);
// RAVEN END

	if ( pm_usecylinder.GetBool() ) {
		newClip = new idClipModel( idTraceModel( bounds, 8 ), declManager->FindMaterial( "textures/flesh_boundingbox" ) );
		newClip->Translate( physicsObj.PlayerGetOrigin() );
		physicsObj.SetClipModel( newClip, 1.0f );
	} else {
		newClip = new idClipModel( idTraceModel( bounds ), declManager->FindMaterial( "textures/flesh_boundingbox" ) );
		newClip->Translate( physicsObj.PlayerGetOrigin() );
		physicsObj.SetClipModel( newClip, 1.0f );
	}
}

/*
==============
idPlayer::EnterVehicle
==============
*/
bool idPlayer::EnterVehicle( idEntity* vehicle ) {
	if ( !idActor::EnterVehicle ( vehicle ) ) {
		return false;
	}
	
// RAVEN BEGIN
// jshepard: safety first
	if( weapon)	{
	  	weapon->Hide();
	}

// abahr:
	//HideCrosshair();
// RAVEN END
  	
  	return true;
}

/*
==============
idPlayer::ExitVehicle
==============
*/
bool idPlayer::ExitVehicle ( bool force ) {
	if ( !idActor::ExitVehicle ( force ) ) {
		return false;
	}
	
	SetViewAngles( viewAxis[0].ToAngles() );

// RAVEN BEGIN
// jshepard: had this crash on me more than once :(
	if(weapon)	{
		weapon->Show();
	}
// abahr: Would like to call something more specific
	ShowCrosshair();
// RAVEN END
	
	return true;
}


// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus

/*
==============
GetItemCost
==============
*/
int idPlayer::GetItemCost( const char* itemName ) {
	if ( !itemCosts ) {
		assert( false );
		return 99999;
	}
	return itemCosts->dict.GetInt( itemName, "99999" );
}

/*
==============
GetItemBuyImpulse
==============
*/
int GetItemBuyImpulse( const char* itemName )
{
	struct ItemBuyImpulse
	{
		const char*	itemName;
		int			itemBuyImpulse;
	};

	ItemBuyImpulse itemBuyImpulseTable[] =
	{
		{ "weapon_shotgun",					IMPULSE_100, },
		{ "weapon_machinegun",				IMPULSE_101, },
		{ "weapon_hyperblaster",			IMPULSE_102, },
		{ "weapon_grenadelauncher",			IMPULSE_103, },
		{ "weapon_nailgun",					IMPULSE_104, },
		{ "weapon_rocketlauncher",			IMPULSE_105, },
		{ "weapon_railgun",					IMPULSE_106, },
		{ "weapon_lightninggun",			IMPULSE_107, },
		//									IMPULSE_108 - Unused
		{ "weapon_napalmgun",				IMPULSE_109, },
		//		{ "weapon_dmg",						IMPULSE_110, },
		//									IMPULSE_111 - Unused
		//									IMPULSE_112 - Unused
		//									IMPULSE_113 - Unused
		//									IMPULSE_114 - Unused
		//									IMPULSE_115 - Unused
		//									IMPULSE_116 - Unused
		//									IMPULSE_117 - Unused
		{ "item_armor_small",				IMPULSE_118, },
		{ "item_armor_large",				IMPULSE_119, },
		{ "ammorefill",						IMPULSE_120, },
		//									IMPULSE_121 - Unused
		//									IMPULSE_122 - Unused
		{ "ammo_regen",						IMPULSE_123, },
		{ "health_regen",					IMPULSE_124, },
		{ "damage_boost",					IMPULSE_125, },
		//									IMPULSE_126 - Unused
		//									IMPULSE_127 - Unused
	};
	const int itemBuyImpulseTableSize = sizeof(itemBuyImpulseTable) / sizeof(itemBuyImpulseTable[0]);

	for( int i = 0; i < itemBuyImpulseTableSize; ++ i )
	{
		if( !stricmp( itemBuyImpulseTable[ i ].itemName, itemName ) )
		{
			return itemBuyImpulseTable[ i ].itemBuyImpulse;
		}
	}

	return 0;
}


bool idPlayer::CanBuyItem( const char* itemName )
{
	itemBuyStatus_t buyStatus = ItemBuyStatus( itemName );
	return( buyStatus == IBS_CAN_BUY );
}


itemBuyStatus_t idPlayer::ItemBuyStatus( const char* itemName )
{
	idStr itemNameStr = itemName;
	if ( itemNameStr == "notimplemented" )
	{
		return IBS_NOT_ALLOWED;
	}
	else if( !idStr::Cmpn( itemName, "wpmod_", 6 ) )
	{
		return IBS_NOT_ALLOWED;
	}
	else if( itemNameStr == "item_armor_small" )
	{
		if( inventory.armor >= 190 )
			return IBS_ALREADY_HAVE;

		if( inventory.carryOverWeapons & CARRYOVER_FLAG_ARMOR_LIGHT )
			return IBS_ALREADY_HAVE;

		if( PowerUpActive( POWERUP_SCOUT ) )
			return IBS_NOT_ALLOWED;
	}
	else if( itemNameStr == "item_armor_large" )
	{
		if( inventory.armor >= 190 )
			return IBS_ALREADY_HAVE;

		if( inventory.carryOverWeapons & CARRYOVER_FLAG_ARMOR_HEAVY )
			return IBS_ALREADY_HAVE;

		if( PowerUpActive( POWERUP_SCOUT ) )
			return IBS_NOT_ALLOWED;
	}
	else if( itemNameStr == "ammorefill" )
	{
		if( inventory.carryOverWeapons & CARRYOVER_FLAG_AMMO )
			return IBS_ALREADY_HAVE;

		// If we are full of ammo for all weapons, you can't buy the ammo refill anymore.
		bool fullAmmo = true;
		for ( int i = 0 ; i < MAX_AMMOTYPES; i++ )
		{
			if ( inventory.ammo[i] != inventory.MaxAmmoForAmmoClass( this, rvWeapon::GetAmmoNameForIndex(i) ) )
				fullAmmo = false;
		}
		if ( fullAmmo )
			return IBS_NOT_ALLOWED;
	}
	else if ( itemNameStr == "fc_armor_regen" )
	{
		return IBS_NOT_ALLOWED;
	}

	if ( gameLocal.gameType == GAME_DM || gameLocal.gameType == GAME_TOURNEY || gameLocal.gameType == GAME_ARENA_CTF || gameLocal.gameType == GAME_1F_CTF || gameLocal.gameType == GAME_ARENA_1F_CTF ) {
		if ( itemNameStr == "ammo_regen" )
			return IBS_NOT_ALLOWED;
		if ( itemNameStr == "health_regen" )
			return IBS_NOT_ALLOWED;
		if ( itemNameStr == "damage_boost" )
			return IBS_NOT_ALLOWED;
	}

	if ( CanSelectWeapon(itemName) != -1 )
		return IBS_ALREADY_HAVE;

	int cost = GetItemCost(itemName);
	if ( cost > (int)buyMenuCash )
	{
		return IBS_CANNOT_AFFORD;
	}

	return IBS_CAN_BUY;
}

/*
==============
idPlayer::UpdateTeamPowerups
==============
*/
void idPlayer::UpdateTeamPowerups( bool isBuying ) {
	for ( int i=0; i<MAX_TEAM_POWERUPS; i++ )
	{
		// If the powerup needs updating...
		if ( gameLocal.mpGame.teamPowerups[team][i].powerup != 0 && gameLocal.mpGame.teamPowerups[team][i].update )
		{
			int powerup = gameLocal.mpGame.teamPowerups[team][i].powerup;
			int time = gameLocal.mpGame.teamPowerups[team][i].time;

			// Go through all the clients and update the status of this powerup.
			for( int j = 0; j < gameLocal.numClients; j++ ) {
				idEntity* ent = gameLocal.entities[ j ];

				if ( !ent )
					continue;

				if ( !ent->IsType( idPlayer::Type ) )
					continue;

				idPlayer* player = static_cast< idPlayer * >( ent );

				// Not my teammate
				if ( player->team != team )
					continue;
			
				// when a teammate respawns while a powerup is active, we don't want to go through other players which already have the powerup
				// otherwise they'll get multiple VO announces
				// ignore this when setting up powerups after a buying impulse, it only means someone is buying before expiration
				if ( !isBuying && ( ( player->inventory.powerups & ( 1 << powerup ) ) != 0 ) ) {
					continue;
				}

				player->GivePowerUp( powerup, time, true );
			}

			gameLocal.mpGame.teamPowerups[team][i].update = false;
		}
	}
}


/*
==============
idPlayer::AttemptToBuyTeamPowerup
==============
*/
bool idPlayer::AttemptToBuyTeamPowerup( const char* itemName )
{
	idStr itemNameStr = itemName;

	if ( itemNameStr == "ammo_regen" ) {
		gameLocal.mpGame.AddTeamPowerup(POWERUP_AMMOREGEN, SEC2MS(30), team);
		UpdateTeamPowerups( true );
		return true;
	}
	else if ( itemNameStr == "health_regen" ) {
		gameLocal.mpGame.AddTeamPowerup(POWERUP_REGENERATION, SEC2MS(30), team);
		UpdateTeamPowerups( true );
		return true;
	}
	else if ( itemNameStr == "damage_boost" ) {
		gameLocal.mpGame.AddTeamPowerup(POWERUP_TEAM_DAMAGE_MOD, SEC2MS(30), team);
		UpdateTeamPowerups( true );
		return true;
	}

	return false;
}

/*
==============
idPlayer::AttemptToBuyItem
==============
*/
bool idPlayer::AttemptToBuyItem( const char* itemName )
{
	if ( gameLocal.isClient ) {
		return false;
	}

	if( !itemName ) {
		return false;
	}

	int itemCost = GetItemCost( itemName );

	/// Check if the player is allowed to buy this item
	if( !CanBuyItem( itemName ) )
	{
		return false;
	}

	const char* playerName = GetUserInfo()->GetString( "ui_name" );
	common->DPrintf( "Player %s about to buy item %s; player has %d (%g) credits, cost is %d\n", playerName, itemName, (int)buyMenuCash, buyMenuCash, itemCost );

	buyMenuCash -= (float)itemCost;

	common->DPrintf( "Player %s just bought item %s; player now has %d (%g) credits, cost was %d\n", playerName, itemName, (int)buyMenuCash, buyMenuCash, itemCost );


	// Team-based effects
	idStr itemNameStr = itemName;

	if ( itemNameStr == "ammo_regen" || itemNameStr == "health_regen" || itemNameStr == "damage_boost" ) {
		return AttemptToBuyTeamPowerup(itemName);
	}

	GiveStuffToPlayer( this, itemName, NULL );
	gameLocal.mpGame.RedrawLocalBuyMenu();
	return true;
}

bool idPlayer::CanBuy( void ) {
	bool ret = gameLocal.mpGame.IsBuyingAllowedRightNow();
	if ( !ret ) {
		return false;
	}
	return !spectating;
}


void idPlayer::GenerateImpulseForBuyAttempt( const char* itemName ) {
	if ( !CanBuy() )
		return;

	int itemBuyImpulse = GetItemBuyImpulse( itemName );
	PerformImpulse( itemBuyImpulse );
}
// RITUAL END


/*
==============
idPlayer::PerformImpulse
==============
*/
void idPlayer::PerformImpulse( int impulse ) {

//RAVEN BEGIN
// nrausch: Don't send xenon dpad impulses over the network
#ifdef _XENON
	
	if ( objectiveSystemOpen ) {
		return;
	}
	
	if ( gameLocal.isClient && (impulse < IMPULSE_70) ) {
#else
	if ( gameLocal.isClient ) {
#endif
//RAVEN END
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

 		assert( entityNumber == gameLocal.localClientNum );
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteBits( impulse, IMPULSE_NUMBER_OF_BITS );
		ClientSendEvent( EVENT_IMPULSE, &msg );
	}

	if ( impulse >= IMPULSE_0 && impulse <= IMPULSE_12 ) {
		SelectWeapon( impulse, false );
		return;
	}

//RAVEN BEGIN
//asalmon: D-pad events for in game guis on Xenon
#ifdef _XBOX
	sysEvent_t ev;
	ev.evType = SE_KEY;
	ev.evValue2 = 1;
	ev.evPtrLength = 0;
	ev.evPtr = NULL;

	const char *command = NULL;
	idUserInterface *ui = ActiveGui();
	bool updateVisuals = false;
#endif
//RAVEN END

	switch( impulse ) {
		case IMPULSE_13: {
			Reload();
			break;
		}
		case IMPULSE_14: {
			NextWeapon();
			if( gameLocal.isServer && spectating && gameLocal.gameType == GAME_TOURNEY ) {	
				((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->SpectateCycleNext( this );
			}
			break;
		}
		case IMPULSE_15: {
			PrevWeapon();
			if( gameLocal.isServer && spectating && gameLocal.gameType == GAME_TOURNEY ) {	
				((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->SpectateCyclePrev( this );
			}
			break;
		}
		case IMPULSE_17: {
 			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
 				gameLocal.mpGame.ToggleReady( );
			}
			break;
		}
		case IMPULSE_18: {
			centerView.Init(gameLocal.time, 200, viewAngles.pitch, 0);
			break;
		}
		case IMPULSE_19: {
/*		
			// when we're not in single player, IMPULSE_19 is used for showScores
			// otherwise it does IMPULSE_12 (PDA)
			if ( !gameLocal.isMultiplayer ) {
				if ( !objectiveSystemOpen ) {
					if ( weapon ) {
						weapon->Hide ();
					}
				}
				ToggleMap();
			}
*/
			break;
		}
		case IMPULSE_20: {
 			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
 				gameLocal.mpGame.ToggleTeam( );
			}
			break;
		}
		case IMPULSE_21: {
			if( gameLocal.isServer && gameLocal.gameType == GAME_TOURNEY ) {
				// only allow a client to join the waiting arena if they are not currently assigned to an arena

				// removed waiting arena functionality for now
				/*rvTourneyArena& arena = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( GetArena() );

				if( this != arena.GetPlayers()[ 0 ] && this != arena.GetPlayers()[ 1 ] ) {
					if( instance == MAX_ARENAS && !spectating ) {
						ServerSpectate( true );
						JoinInstance( ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetNextActiveArena( 0 ) );
					} else if( spectating ) {
						JoinInstance( MAX_ARENAS );
						ServerSpectate( false );
					}
				}*/
			}
			break;
		}
		case IMPULSE_22: {
 			if ( gameLocal.isClient || entityNumber == gameLocal.localClientNum ) {
 				gameLocal.mpGame.ToggleSpectate( );
   			}
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
			idFuncRadioChatter::RepeatLast();
			break;
		}

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
		case IMPULSE_100:	AttemptToBuyItem( "weapon_shotgun" );				break;
		case IMPULSE_101:	AttemptToBuyItem( "weapon_machinegun" );			break;
		case IMPULSE_102:	AttemptToBuyItem( "weapon_hyperblaster" );			break;
		case IMPULSE_103:	AttemptToBuyItem( "weapon_grenadelauncher" );		break;
		case IMPULSE_104:	AttemptToBuyItem( "weapon_nailgun" );				break;
		case IMPULSE_105:	AttemptToBuyItem( "weapon_rocketlauncher" );		break;
		case IMPULSE_106:	AttemptToBuyItem( "weapon_railgun" );				break;
		case IMPULSE_107:	AttemptToBuyItem( "weapon_lightninggun" );			break;
		case IMPULSE_108:	break; // Unused
		case IMPULSE_109:	AttemptToBuyItem( "weapon_napalmgun" );				break;
		case IMPULSE_110:	/* AttemptToBuyItem( "weapon_dmg" );*/				break;
		case IMPULSE_111:	break; // Unused
		case IMPULSE_112:	break; // Unused
		case IMPULSE_113:	break; // Unused
		case IMPULSE_114:	break; // Unused
		case IMPULSE_115:	break; // Unused
		case IMPULSE_116:	break; // Unused
		case IMPULSE_117:	break; // Unused
		case IMPULSE_118:	AttemptToBuyItem( "item_armor_small" );				break;
		case IMPULSE_119:	AttemptToBuyItem( "item_armor_large" );				break;
		case IMPULSE_120:	AttemptToBuyItem( "ammorefill" );					break;
		case IMPULSE_121:	break; // Unused
		case IMPULSE_122:	break; // Unused
		case IMPULSE_123:	AttemptToBuyItem( "ammo_regen" );					break;
		case IMPULSE_124:	AttemptToBuyItem( "health_regen" );					break;
		case IMPULSE_125:	AttemptToBuyItem( "damage_boost" );					break;
		case IMPULSE_126:	break; // Unused
		case IMPULSE_127:	break; // Unused
// RITUAL END

		case IMPULSE_50: {
			ToggleFlashlight ( );
			break;
		}

 		case IMPULSE_51: {
 			LastWeapon();
 			break;
 		}
	} 

//RAVEN BEGIN
//asalmon: route d-pad input to the active gui.
#ifdef _XBOX
	if (ui && ev.evValue != 0 && !objectiveSystemOpen ) {
		command = ui->HandleEvent( &ev, gameLocal.time, &updateVisuals );
		if ( updateVisuals && focusEnt && ui == focusUI ) {
			focusEnt->UpdateVisuals();
		}

		if ( gameLocal.isClient ) {
			// we predict enough, but don't want to execute commands
			return;
		} 
		if ( focusEnt ) {
			HandleGuiCommands( focusEnt, command );
		} else {
			HandleGuiCommands( this, command );
		}
	}
#endif
//RAVEN END
}
   
/*
==============
idPlayer::HandleESC
==============
*/
bool idPlayer::HandleESC( void ) {

// jdischler: Straight from the top, cinematic skipping on xenon is OFFICIALLY OUT.  Too many problems with it and not enough time to properly address them.
#ifndef _XENON
	if ( gameLocal.inCinematic ) {
		return SkipCinematic();
	}
#endif
	return false;
}
 
/*
==============
idPlayer::HandleObjectiveInput
==============
*/
void idPlayer::HandleObjectiveInput() {
#ifdef _XENON
	if ( gameLocal.inCinematic ) {
		return;
	}
	if ( !objectiveButtonReleased ) {
		if ( ( usercmd.buttons &	BUTTON_SCORES ) == 0 ) {
			objectiveButtonReleased = true;
		}
	} else {
		if ( ( usercmd.buttons & BUTTON_SCORES ) != 0 ) {
			ToggleObjectives ( );
			g_ObjectiveSystemOpen = objectiveSystemOpen;
			objectiveButtonReleased = false;
		}
	}
#else
	if ( ( usercmd.buttons & BUTTON_SCORES ) != 0 && !objectiveSystemOpen && !gameLocal.inCinematic ) {
		ToggleObjectives ( );
	} else if ( ( usercmd.buttons & BUTTON_SCORES ) == 0 && objectiveSystemOpen ) {
		ToggleObjectives ( );
	} else if ( objectiveSystemOpen && gameLocal.inCinematic ) {
		ToggleObjectives ( );
	}
#endif
}

/*
==============
idPlayer::EvaluateControls
==============
*/
void idPlayer::EvaluateControls( void ) {
	// check for respawning
	if ( pfl.dead || pfl.objectiveFailed ) {
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
		if( allowedToRespawn ) {
		if ( ( gameLocal.time > minRespawnTime ) && ( usercmd.buttons & BUTTON_ATTACK ) ) {
			forceRespawn = true;
		} else if ( gameLocal.time > maxRespawnTime ) {
			forceRespawn = true;
		}
	}
		else
		{
			Spectate(true);
		}
// RITUAL END
	}

	// in MP, idMultiplayerGame decides spawns
	if ( forceRespawn && !gameLocal.isMultiplayer && !g_testDeath.GetBool() ) {
		// in single player, we let the session handle restarting the level or loading a game
		gameLocal.sessionCommand = "died";
	}

	if ( ( usercmd.flags & UCF_IMPULSE_SEQUENCE ) != ( oldFlags & UCF_IMPULSE_SEQUENCE ) )  {
		PerformImpulse( usercmd.impulse );
	}

	if( forceScoreBoard && forceScoreBoardTime && gameLocal.time > forceScoreBoardTime ) {
		forceScoreBoardTime = 0;
		forceScoreBoard = false;
	}
	scoreBoardOpen		= ( ( usercmd.buttons & BUTTON_SCORES ) != 0 || forceScoreBoard );

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

	if ( spectating ) {
		speed = pm_spectatespeed.GetFloat();
		bobFrac = 0.0f;
	} else if ( noclip ) {
		speed = pm_noclipspeed.GetFloat();
		bobFrac = 0.0f;
 	} else if ( !physicsObj.OnLadder() && ( usercmd.buttons & BUTTON_RUN ) && ( usercmd.forwardmove || usercmd.rightmove ) && ( usercmd.upmove >= 0 ) ) {
		bobFrac = 1.0f;
		speed = pm_speed.GetFloat();
	} else {
		speed = pm_walkspeed.GetFloat();
		bobFrac = 0.0f;
	}

	speed *= PowerUpModifier(PMOD_SPEED);

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
	idMat3		lookAxis;
	idMat3		legsAxis;
	bool		blend;
	float		diff;
	float		frac;
	float		upBlend;
	float		forwardBlend;
	float		downBlend;

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

	pfl.turnLeft = false;
	pfl.turnRight = false;
	if ( idealLegsYaw < -45.0f ) {
		idealLegsYaw = 0;
		pfl.turnRight = true;
		blend = true;
	} else if ( idealLegsYaw > 45.0f ) {
		idealLegsYaw = 0;
		pfl.turnLeft = true;
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
	} else if ( spectating || ( gameLocal.isClient && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance ) ) {
		physicsObj.SetContents( 0 );
		physicsObj.SetMovementType( PM_SPECTATOR );
	} else if ( health <= 0 ) {
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
		physicsObj.SetMovementType( PM_DEAD );
 	} else if ( gameLocal.inCinematic || pfl.objectiveFailed || gameLocal.GetCamera() || privateCameraView || ( influenceActive == INFLUENCE_LEVEL2 ) ) {
		physicsObj.SetContents( CONTENTS_BODY | (use_combat_bbox?CONTENTS_SOLID:0) );
		physicsObj.SetMovementType( PM_FREEZE );
	} else {
		physicsObj.SetContents( CONTENTS_BODY | (use_combat_bbox?CONTENTS_SOLID:0) );
		physicsObj.SetMovementType( PM_NORMAL );
	}

	if ( spectating || ( gameLocal.isClient && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() != instance ) ) {
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
	
	// If the player is dead then only run physics on new
	// frames since articulated figures are not synchronized over the network
	if ( health <= 0 ) {
		if ( gameLocal.isNewFrame ) {
			DeathPush();
			RunPhysics();
		}
	} else { 
		RunPhysics();
	}

 	// update our last valid AAS location for the AI
	if( !gameLocal.isMultiplayer ) {
		SetAASLocation(); 
	}

	if ( spectating ) {
		newEyeOffset = 0.0f;
	} else if ( health <= 0 ) {
		newEyeOffset = pm_deadviewheight.GetFloat();
	} else if ( physicsObj.IsCrouching() ) {
		newEyeOffset = pm_crouchviewheight.GetFloat();
	} else if ( IsInVehicle ( ) ) {
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

	if ( noclip || gameLocal.inCinematic || ( influenceActive == INFLUENCE_LEVEL2 ) ) {
		pfl.crouch		= false;
 		pfl.onGround	= ( influenceActive == INFLUENCE_LEVEL2 );
		pfl.onLadder	= false;
		pfl.jump		= false;
	} else {
		pfl.crouch	= physicsObj.IsCrouching();
		pfl.onGround	= physicsObj.HasGroundContacts();
		pfl.onLadder	= physicsObj.OnLadder();
		pfl.jump		= physicsObj.HasJumped();

 		// check if we're standing on top of a monster and give a push if we are
 		idEntity *groundEnt = physicsObj.GetGroundEntity();
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
 		if ( groundEnt && groundEnt->IsType( idAI::GetClassType() ) ) {
// RAVEN END
 			idVec3 vel = physicsObj.GetLinearVelocity();
 			if ( vel.ToVec2().LengthSqr() < 0.1f ) {
 				vel.ToVec2() = physicsObj.GetOrigin().ToVec2() - groundEnt->GetPhysics()->GetAbsBounds().GetCenter().ToVec2();
 				vel.ToVec2().NormalizeFast();
 				vel.ToVec2() *= pm_speed.GetFloat();
 			} else {
 				// give em a push in the direction they're going
 				vel *= 1.1f;
 			}
 			physicsObj.SetLinearVelocity( vel );
 		}
	}

	if ( pfl.jump ) {
		loggedAccel_t	*acc = &loggedAccel[currentLoggedAccel&(NUM_LOGGED_ACCELS-1)];
		currentLoggedAccel++;
		acc->time = gameLocal.time;
		acc->dir[2] = 200;
		acc->dir[0] = acc->dir[1] = 0;
	}

	if ( pfl.onLadder ) {
		int old_rung = oldOrigin.z / LADDER_RUNG_DISTANCE;
 		int new_rung = physicsObj.GetOrigin().z / LADDER_RUNG_DISTANCE;

		if ( old_rung != new_rung ) {
 			StartSound( "snd_stepladder", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}

	UpdateIntentDir( );

	BobCycle( pushVelocity );

// RAVEN BEGIN
// abahr: don't crashland while no clipping.
	if( !noclip ) {
		CrashLand( oldOrigin, oldVelocity );
	}
// RAVEN END
}

/*
==================
idPlayer::BiasIntentDir

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayer::BiasIntentDir( idVec3 newIntentDir, float prevBias ) {
	if ( !newIntentDir.Compare( vec3_origin ) )	{
		if ( intentDir.Compare( vec3_origin ) )	{
			//initialize it
			intentDir = newIntentDir;
		} else {
			intentDir = ((intentDir*prevBias)+newIntentDir)/(prevBias+1.0f);
			float iDirLen = idMath::ClampFloat(0.0f,1024.0f,intentDir.Normalize());
			intentDir *= iDirLen;
		}
	}
}

/*
==============
idPlayer::UpdateIntentDir
==============
*/
void idPlayer::UpdateIntentDir ( void ) {
	idVec3 newIntentDir;
	idVec3 viewDir = viewAxis[0];
	viewDir.z = 0;
	viewDir.Normalize();
	float prevBias = 199.0f;
	if ( intentDir.Compare( vec3_origin ) ) {
        newIntentDir = viewDir*50.0f;
	} else {
		newIntentDir = viewDir*intentDir.Length();
		if ( flashlightOn ) {
			//bias them more heavily to looking where I'm looking
			prevBias = 19.0f;
		}
	}
	if ( pfl.onGround )	{
		idVec3 moveDir = physicsObj.GetLinearVelocity();
		if ( moveDir.x || moveDir.y ) {
			// moving, too
			moveDir.z = 0;
			newIntentDir = moveDir;
			prevBias = 39.0f;
		}
	}
	BiasIntentDir( newIntentDir, prevBias );
	if ( ai_debugSquad.GetBool() ) {
		idVec4 color = colorCyan;
		if ( pfl.weaponFired ) {
			color = colorRed;
		}
		gameRenderWorld->DebugArrow( color, GetPhysics()->GetOrigin(), GetPhysics()->GetOrigin() + intentDir*4.0f, 8, 0 );
	}
}

/*
==============
idPlayer::UpdateHud
==============
*/
void idPlayer::UpdateHud( void ) {
	if ( !hud ) {
		return;
	}

	if ( entityNumber != gameLocal.localClientNum ) {
		return;
	}

	// FIXME: this is here for level ammo balancing to see pct of hits 
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
 		if ( !doingDeathSkin && !deathSkinTime ) {
			deathSkinTime = gameLocal.time + 1000;
			deathStateHitch = state_hitch;
		}

		// wait a bit before switching off the content
		if ( deathClearContentsTime && gameLocal.time > deathClearContentsTime ) {
			SetCombatContents( false );
			deathClearContentsTime = 0;
		}
	} else {
		renderEntity.noShadow = false;
		renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = 0.0f;
		if( gameLocal.isMultiplayer ) {
			if( clientHead ) {
				clientHead->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = 0.0f;
			}
		} else {
			if( head ) {
				head.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = 0.0f;
			}
		}
		UpdateVisuals();
 		doingDeathSkin = false;
		deathClearContentsTime = 0;
	}
}

/*
===============
idPlayer::LoadDeferredModel
===============
*/
void idPlayer::LoadDeferredModel( void ) {
	if( !modelDecl ) {
		gameLocal.Warning( "idPlayer::LoadDeferredModel() - reloadModel without vaid modelDict\n" );
		return;
	} 

	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Idle", 0 );
	SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 0 );
	UpdateState();

	if( weapon ) {
		weapon->NetCatchup();
	}

	if( !modelDecl->skin.Length() ) {
		skin = NULL;
	} else {
		skin = declManager->FindSkin( modelDecl->skin.c_str(), false );
	}

	SetModel( modelDecl->model );

	if( !modelDecl->head.Length() ) {
		if( clientHead ) {
			delete clientHead;
			clientHead = NULL;
		}
	} else {
		SetupHead( modelDecl->head.c_str(), modelDecl->headOffset );
	}

	if( clientHead && health <= 0 ) {
		// update death shader for new head
		clientHead.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ];
		clientHead.GetEntity()->GetRenderEntity()->noShadow = renderEntity.noShadow;
	}

	if( powerUpSkin != NULL ) {
		SetSkin( powerUpSkin );
		if( clientHead ) {
			clientHead->SetSkin( powerUpSkin );
		}
	} else {
		SetSkin( skin );
		if( clientHead ) {
			clientHead->SetSkin( headSkin );
		}
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
 
	if ( talkingNPC ) {
		if ( !talkingNPC.IsValid() ) {
			talkingNPC = NULL;
		} else {
			idAI *talkingNPCAI = (idAI*)(talkingNPC.GetEntity());
			if ( !talkingNPCAI ) {
				//wtf?
				talkingNPC = NULL;
			} else if ( talkingNPCAI->talkTarget != this || !talkingNPCAI->IsSpeaking() || DistanceTo( talkingNPCAI ) > 256.0f ) {
				//forget about them, okay to talk to someone else now
				talkingNPC = NULL;
			}
		}
	}

	if ( !gameLocal.usercmds ) {
		return;
	}

#ifdef _XENON
	// change the crosshair if it's modified
	if ( cursor && weapon && g_crosshairColor.IsModified() ) {
		weapon->UpdateCrosshairGUI( cursor );
		cursor->HandleNamedEvent( "weaponChange" );
		g_crosshairColor.ClearModified();
	}
#endif

 	// Dont do any thinking if we are in modview
	if ( gameLocal.editors & EDITOR_MODVIEW || gameEdit->PlayPlayback() ) {
		// calculate the exact bobbed view position, which is used to
		// position the view weapon, among other things
		CalculateFirstPersonView();

		// this may use firstPersonView, or a thirdPerson / camera view
		CalculateRenderView();

		FreeModelDef();
		
		if ( weapon ) {
			weapon->GetWorldModel()->FreeModelDef();
		}

 		if ( head.GetEntity() ) {
			head->FreeModelDef();
		}

		if ( clientHead ) {
			clientHead->FreeEntityDef();
		}

		return;
	}

	if( reloadModel ) {
		LoadDeferredModel(); 
		reloadModel = false;
	}

	gameEdit->RecordPlayback( usercmd, this );

	// latch button actions
	oldButtons = usercmd.buttons;

	// grab out usercmd
	usercmd_t oldCmd = usercmd;
	usercmd = gameLocal.usercmds[ entityNumber ];
	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;

	HandleObjectiveInput();
	if ( objectiveSystemOpen ) {
		HandleCheats();
	} else {
		ClearCheatState();
	}

	aasSensor->Update();

	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		// we need to let the camera think inside of this routine
		CalculateRenderView();
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

	if ( gameLocal.inCinematic || influenceActive 
#ifdef _XENON
		|| objectiveSystemOpen 
#endif
		) {
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}

	if( gameLocal.GetIsFrozen() && gameLocal.gameType == GAME_DEADZONE )
	{
		usercmd.forwardmove = 0;
		usercmd.rightmove = 0;
		usercmd.upmove = 0;
	}
	
	// zooming
	bool zoom = (usercmd.buttons & BUTTON_ZOOM) && CanZoom();
	if ( zoom != zoomed ) {
		if ( zoom ) {
			ProcessEvent ( &EV_Player_ZoomIn );
		} else {
			ProcessEvent ( &EV_Player_ZoomOut );
		}

		if ( vehicleController.IsDriving( ) ) {
#ifdef _XENON
			usercmdGen->SetSlowJoystick( zoom ? pm_zoomedSlow.GetInteger() : 100 );
#else
			cvarSystem->SetCVarInteger( "pm_isZoomed", zoom ? pm_zoomedSlow.GetInteger() : 0 );
#endif
		}

	}

	if ( IsInVehicle ( ) ) {	
		vehicleController.SetInput ( usercmd, viewAngles );
				
		// calculate the exact bobbed view position, which is used to
		// position the view weapon, among other things
		CalculateFirstPersonView();

		// this may use firstPersonView, or a thirdPeoson / camera view
		CalculateRenderView();

		thinkFlags |= TH_PHYSICS;
		RunPhysics();

		if ( health > 0 ) {
			TouchTriggers();
		}

		UpdateLocation();
		
		if ( !fl.hidden ) {
			UpdateAnimation();
			Present();
			LinkCombat();
		} else {
			UpdateModel();
		}

		// I don't want to have to add this but if you are in a locked vehicle and you fail your objective, you won't be 
		//	ejected from the vehicle (and I don't think we'd want that even though we do have the option of forcing it..)
		//	and since you are still in the vehicle, EvaluateControls (which covers the logic below for player usercmds)
		//	will never get called.
		if ( pfl.objectiveFailed ) 
		{
			if ( (	gameLocal.time > minRespawnTime && (usercmd.buttons & BUTTON_ATTACK)) ||
				gameLocal.time > maxRespawnTime )
			{
				gameLocal.sessionCommand = "died";
			}
		}

		return;
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

	// if we have an active gui, we will unrotate the view angles as
	// we turn the mouse movements into gui events
	idUserInterface *gui = ActiveGui();
	if ( gui && gui != focusUI ) {
		RouteGuiMouse( gui );
	}

	// set the push velocity on the weapon before running the physics
	if ( weapon ) {
		weapon->SetPushVelocity( physicsObj.GetPushedLinearVelocity() );
	}

	EvaluateControls();


// RAVEN BEGIN
// abahr
	if( !noclip && !spectating ) {
		UpdateGravity();
	}
// RAVEN END

	Move();

	if ( !g_stopTime.GetBool() ) {
 		if ( !noclip && !spectating && ( health > 0 ) && !IsHidden() ) {
 			TouchTriggers();
 		}

		// not done on clients for various reasons. don't do it on server and save the sound channel for other things
		if ( !gameLocal.isMultiplayer ) {
			if ( g_useDynamicProtection.GetBool() && dynamicProtectionScale < 1.0f && gameLocal.time - lastDmgTime > 500 ) {
				if ( dynamicProtectionScale < 1.0f ) {
					dynamicProtectionScale += 0.05f;
				}
				if ( dynamicProtectionScale > 1.0f ) {
					dynamicProtectionScale = 1.0f;
				}
			}
		}

 		// update GUIs, Items, and character interactions
		UpdateFocus();
 		
 		UpdateLocation();

	 	// update player script
 		UpdateState(); 

		// service animations
		if ( !spectating && !af.IsActive() ) {
    		UpdateConditions();
			UpdateAnimState();
			CheckBlink();
		}

		// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
		pfl.pain = false;
	}

	if ( !af.IsActive() ) {
		AdjustBodyAngles();
	}

	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();

	// this may use firstPersonView, or a thirdPeroson / camera view
	CalculateRenderView();

	if ( spectating ) {
		UpdateSpectating();
	} else if ( health > 0 && !gameLocal.inCinematic ) {
		UpdateWeapon();
	}

	UpdateAir();
	
	UpdateHud();

	UpdatePowerUps();

	UpdateDeathSkin( false );

	UpdateDeathShader( deathStateHitch );

	if( gameLocal.isMultiplayer ) {
		if( clientHead.GetEntity() ) {
			headRenderEnt = clientHead.GetEntity()->GetRenderEntity();
		} else {
			headRenderEnt = NULL;
		}
	} else {
 		if ( head.GetEntity() ) {
 			headRenderEnt = head.GetEntity()->GetRenderEntity();
 		} else {
 			headRenderEnt = NULL;
 		}
	}
 
 	if ( headRenderEnt ) {
		if ( powerUpSkin ) {
			headRenderEnt->customSkin = powerUpSkin;
		} else if ( influenceSkin ) {
 			headRenderEnt->customSkin = influenceSkin;
 		} else {
 			headRenderEnt->customSkin = headSkin;
 		}
 		headRenderEnt->suppressSurfaceInViewID = entityNumber + 1;
 	}

	// always show your own shadow
	if( entityNumber == gameLocal.localClientNum ) {
		renderEntity.suppressLOD = 1;
		if( headRenderEnt ) {
			headRenderEnt->suppressLOD = 1;
		}
	} else {
		renderEntity.suppressLOD = 0;
		if( headRenderEnt ) {
			headRenderEnt->suppressLOD = 0;
		}
	}

	DrawShadow( headRenderEnt );

	// never cast shadows from our first-person muzzle flashes
	// FIXME: get first person flashlight into this 
	renderEntity.suppressShadowInLightID = rvWeapon::WPLIGHT_MUZZLEFLASH * 100 + entityNumber;
 	if ( headRenderEnt ) {
 		headRenderEnt->suppressShadowInLightID = rvWeapon::WPLIGHT_MUZZLEFLASH * 100 + entityNumber;
   	}

 	if ( !g_stopTime.GetBool() ) {
		UpdateAnimation();

		Present();

		LinkCombat();
	}

 	if ( !( thinkFlags & TH_THINK ) ) {
		common->DPrintf( "player %d not thinking?\n", entityNumber );
	}

	if ( g_showEnemies.GetBool() ) {
		idActor *ent;
		int num = 0;
		for( ent = enemyList.Next(); ent != NULL; ent = ent->enemyNode.Next() ) {
			common->DPrintf( "enemy (%d)'%s'\n", ent->entityNumber, ent->name.c_str() );
			gameRenderWorld->DebugBounds( colorRed, ent->GetPhysics()->GetBounds().Expand( 2 ), ent->GetPhysics()->GetOrigin() );
			num++;
		}
		common->DPrintf( "%d: enemies\n", num );
	}

	if ( !inBuyZonePrev )
		inBuyZone = false;

	inBuyZonePrev = false;
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

bool idPlayer::CanZoom( void  )
{
	if ( vehicleController.IsDriving() ) {
		rvVehicle * vehicle				= vehicleController.GetVehicle();
		rvVehiclePosition * position	= vehicle ? vehicle->GetPosition( vehicleController.GetPosition() ) : 0;
		rvVehicleWeapon * weapon		= position ? position->GetActiveWeapon() : 0;

		return weapon && weapon->CanZoom();
	}

	return weapon && weapon->CanZoom() && !weapon->IsReloading ( );
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
		if ( !g_forceUndying.GetBool() ) {
			undying = false;
		}
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

	if ( pfl.dead ) {
		pfl.pain = true;
		return;
	}

// squirrel: Mode-agnostic buymenus
	if ( gameLocal.isMultiplayer ) {
		if( gameLocal.mpGame.IsBuyingAllowedInTheCurrentGameMode() )
		{
			if( gameLocal.mpGame.GetGameState()->GetMPGameState() != WARMUP )
			{
				/// Remove the player's armor
				inventory.armor = 0;

				/// Preserve this player's weapons at the state of his death, to be restored on respawn
				carryOverCurrentWeapon = currentWeapon;
				inventory.carryOverWeapons = inventory.weapons;

				if( attacker )
				{
					idPlayer* killer = NULL;
					if ( attacker->IsType( idPlayer::Type ) )
					{
						killer = static_cast<idPlayer*>(attacker);
						if( killer == this )
						{
							// Killed by self
							float cashAward = (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerCashAward_killingSelf", 0 );
							killer->GiveCash( cashAward );
						}
						else if( gameLocal.IsTeamGame() && killer->team == team )
						{
							// Killed by teammate
							float cashAward = (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerCashAward_killingTeammate", 0 );
							killer->GiveCash( cashAward );
						}
						else
						{
							// Killed by enemy
							float cashAward = (float) gameLocal.mpGame.mpBuyingManager.GetOpponentKillCashAward();
							killer->GiveCash( cashAward );
						}
					}
				}
			}
		}
	}
// RITUAL END

	bool noDrop = false;
	if ( inflictor 
		&& inflictor->IsType( idTrigger_Hurt::GetClassType() ) 
		&& inflictor->spawnArgs.GetBool( "nodrop" ) ) {
		//don't drop weapon or items here, flag auto-returns.
		noDrop = true;
	}

	if ( !g_testDeath.GetBool() && !gameLocal.isMultiplayer ) {
#ifdef _XENON
		playerView.Fade( colorBlack, MAX_RESPAWN_TIME_XEN_SP );
#else
		playerView.Fade( colorBlack, 12000 );
#endif
	}


	pfl.dead = true;
	SetAnimState( ANIMCHANNEL_LEGS, "Legs_Dead", 4 );
	SetAnimState( ANIMCHANNEL_TORSO, "Torso_Dead", 4 );

	animator.ClearAllJoints();

	if ( StartRagdoll() ) {
		pm_modelView.SetInteger( 0 );
#ifdef _XENON
		if ( gameLocal.isMultiplayer )
		{
			// didn't want to have any chance of affecting multiplayer...
			minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME;
			maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
		}
		else
		{
			minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME_XEN_SP;
			maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME_XEN_SP;
		}
#else
		minRespawnTime = gameLocal.time + RAGDOLL_DEATH_TIME;
		maxRespawnTime = minRespawnTime + MAX_RESPAWN_TIME;
#endif
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

	if (weapon) {					// cnicholson: Fix for crash if player dies while in vehicle
		weapon->OwnerDied();		// get rid of weapon
		if ( !noDrop ) {
			DropWeapon( );				// drop the weapon as an item 
		}
		delete weapon;
	}

	weapon = NULL;
	currentWeapon = -1;

	if ( !g_testDeath.GetBool() ) {
		LookAtKiller( inflictor, attacker );
	}

	if ( gameLocal.isMultiplayer || g_testDeath.GetBool() ) {
		idPlayer *killer = NULL;
		int methodOfDeath = MAX_WEAPONS + isTelefragged;
		
		if ( attacker->IsType( idPlayer::Type ) ) {
			killer = static_cast<idPlayer*>(attacker);

			lastKiller = killer;
 			if ( gameLocal.IsTeamGame() && killer->team == team ) {
				// don't worry about team killers
 				lastKiller = NULL;
 			}
			if ( killer == this ) {
				// don't worry about yourself
				lastKiller = NULL;
			}

			if ( health < -20 || killer->PowerUpActive( POWERUP_QUADDAMAGE ) ) {
				gibDeath = true;
				gibDir = dir;
				gibsLaunched = false;
				if( gameLocal.isMultiplayer && gameLocal.isListenServer && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->GetInstance() == instance ) {
					ClientGib( dir );
				}
			}

			if( !isTelefragged ) {
				if ( inflictor->IsType( idProjectile::GetClassType() ) ) {
					methodOfDeath = static_cast<idProjectile*>(inflictor)->methodOfDeath;
				} else if ( inflictor->IsType( idPlayer::Type ) ) {
					// hitscan weapon
					methodOfDeath = static_cast<idPlayer*>(inflictor)->GetCurrentWeapon();
				}
			}

			if( methodOfDeath == -1 ) {
				methodOfDeath = MAX_WEAPONS + isTelefragged;
			}

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		} else if ( attacker->IsType( idWorldspawn::GetClassType() ) ) {
// RAVEN END
			if( lastImpulseTime > gameLocal.time && lastImpulsePlayer ) {
				killer = lastImpulsePlayer;
			}
		}

		gameLocal.mpGame.PlayerDeath( this, killer, methodOfDeath );
	} else {
		physicsObj.SetContents( CONTENTS_CORPSE | CONTENTS_MONSTERCLIP );
	}

	if ( gameLocal.isMultiplayer && gameLocal.IsFlagGameType() ) {
		if ( PowerUpActive( POWERUP_CTF_MARINEFLAG ) ) {
			RemoveClientModel( "mp_ctf_flag_pole" );
			RemoveClientModel( "mp_ctf_marine_flag_world" );
			StopEffect( "fx_ctf_marine_flag_world" );
		} else if ( PowerUpActive( POWERUP_CTF_STROGGFLAG ) ) { 
			RemoveClientModel( "mp_ctf_flag_pole" );
			RemoveClientModel( "mp_ctf_strogg_flag_world" );
			StopEffect( "fx_ctf_strogg_flag_world" );
		} else if ( PowerUpActive( POWERUP_CTF_ONEFLAG ) ) {
			RemoveClientModel( "mp_ctf_one_flag" );
		}

		if ( PowerUpActive( POWERUP_CTF_STROGGFLAG ) || PowerUpActive( POWERUP_CTF_MARINEFLAG ) || PowerUpActive( POWERUP_CTF_ONEFLAG ) ) {
			idPlayer* killer = (idPlayer*)attacker;
			if ( killer != NULL && killer->IsType( idPlayer::GetClassType() ) && killer != this ) {
				if ( killer->team != team ) {
					// killing the flag carrier gives killer double cash
					killer->GiveCash( (float) gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerCashAward_killingOpponentFlagCarrier", 0 ) );
				}
			}
			statManager->FlagDropped( this, attacker );
		}
	}

	DropPowerups();	

	ClearPowerUps();

	UpdateVisuals();

	// AI sometimes needs to respond to having killed someone.
	// Note: Would it be better to make this a virtual funciton of... something?
	aiManager.RemoveTeammate( this );
	
	isChatting = false;
}

/*
=================
CalcDamagePoints

Calculates how many health and armor points will be inflicted, but
doesn't actually do anything with them.  This is used to tell when an attack
would have killed the player, possibly allowing a "saving throw"
=================
*/
void idPlayer::CalcDamagePoints( idEntity *inflictor, idEntity *attacker, const idDict *damageDef,
							   const float damageScale, const int location, int *health, int *armor ) {
	int		damage;
	int		armorSave;
	float	pDmgScale;

	damageDef->GetInt( "damage", "20", damage );
	damage = GetDamageForLocation( damage, location );

	// optional different damage in team games
	if( gameLocal.isMultiplayer && gameLocal.IsTeamGame() && damageDef->GetInt( "damage_team" ) ) {
		damage = damageDef->GetInt( "damage_team" );
	}

	idPlayer *player = attacker->IsType( idPlayer::Type ) ? static_cast<idPlayer*>(attacker) : NULL;
	if ( !gameLocal.isMultiplayer ) {
		if ( inflictor != gameLocal.world ) {
			switch ( g_skill.GetInteger() ) {
				case 0: 
					damage = ceil(0.80f*(float)damage);
					break;
				case 2:
					damage *= 1.7f;
					break;
				case 3:
 					damage *= 3.5f;
					break;
				default:
					//damage *= 1.1f;  reverted to 1.0 for default damage... as per Biessman's request.
					break;
			}	
		}
	}

	damage = ceil(damageScale*(float)damage);

	pDmgScale = damageDef->GetFloat( "playerScale", "1" );
	damage = ceil(pDmgScale*(float)damage);

	// check for completely getting out of the damage
	if ( !damageDef->GetBool( "noGod" ) ) {
		// check for godmode
		if ( godmode ) {
			godmodeDamage += damage;
			damage = 0;
		}
	}

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
 	if ( gameLocal.IsTeamGame()
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
					   const char *damageDefName, const float damageScale, int location ) {
 	idVec3		kick;
 	int			damage;
 	int			armorSave;
 	int			knockback;
 	idVec3		damage_from;
 	float		attackerPushScale;

	// RAVEN BEGIN
	// twhitaker: difficulty levels
	float modifiedDamageScale = damageScale;
	
	if ( !gameLocal.isMultiplayer ) {
		if ( inflictor != gameLocal.world ) {
			modifiedDamageScale *= ( 1.0f + gameLocal.GetDifficultyModifier() );
		}
	}
	// RAVEN END

	if ( forwardDamageEnt.IsValid() ) {
		forwardDamageEnt->Damage( inflictor, attacker, dir, damageDefName, modifiedDamageScale, location );
		return;
	}

	// damage is only processed on server
	if ( gameLocal.isClient ) {
		return;
	}
	
 	if ( !fl.takedamage || noclip || spectating || gameLocal.inCinematic ) {
		// If in vehicle let it know that something is trying to hurt the invisible player
		if ( IsInVehicle ( ) ) {
			const idDict *damageDict = gameLocal.FindEntityDefDict( damageDefName, false );
			if ( !damageDict ) {
				gameLocal.Warning( "Unknown damageDef '%s'", damageDefName );
				return;
			}
			
			// If the damage def is marked as a hazard then issue a warning to the vehicle 
			if ( damageDict->GetBool ( "hazard", "0" ) ) {
				vehicleController.GetVehicle()->IssueHazardWarning ( );
			}
		}
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}
	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	// MCG: player doesn't take friendly fire damage, except from self!
	if ( !gameLocal.isMultiplayer && attacker != this ) {
		if ( attacker->IsType ( idActor::GetClassType() ) && static_cast<idActor*>(attacker)->team == team ) {
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

	if ( damageDef->dict.GetBool( "lightning_damage_effect" ) )
	{
		lightningEffects = 0;
		lightningNextTime = gameLocal.GetTime();
	}

	// We pass in damageScale, because this function calculates a modified damageScale 
	// based on g_skill, and we don't want to compensate for skill level twice.
	CalcDamagePoints( inflictor, attacker, &damageDef->dict, damageScale, location, &damage, &armorSave );

	//
	// determine knockback
	//
	damageDef->dict.GetInt( "knockback", "0", knockback );
	if( gameLocal.isMultiplayer && gameLocal.IsTeamGame() ) {
		damageDef->dict.GetInt( "knockback_team", va( "%d", knockback ), knockback );
	}

	knockback *= damageScale;

	if ( knockback != 0 && !fl.noknockback ) {
		if ( !gameLocal.isMultiplayer && attacker == this ) {
			//In SP, no knockback from your own stuff
			knockback = 0;
		} else {
			if ( attacker != this ) {
				attackerPushScale = 1.0f;	
			} else {
				// since default attackerDamageScale is 0.5, default attackerPushScale should be 2
				damageDef->dict.GetFloat( "attackerPushScale", "2", attackerPushScale );
			}
		
			kick = dir;

			kick.Normalize();
 			kick *= g_knockback.GetFloat() * knockback * attackerPushScale / 200.0f;
			
			physicsObj.SetLinearVelocity( physicsObj.GetLinearVelocity() + kick );

			// set the timer so that the player can't cancel out the movement immediately
 			physicsObj.SetKnockBack( idMath::ClampInt( 50, 200, knockback * 2 ) );
		}
	}
	
	if ( damageDef->dict.GetBool( "burn" ) ) {
		StartSound( "snd_burn", SND_CHANNEL_BODY3, 0, false, NULL );
	} else if ( damageDef->dict.GetBool( "no_air" ) ) {
		if ( !armorSave && health > 0 ) {
			StartSound( "snd_airGasp", SND_CHANNEL_ITEM, 0, false, NULL );
		}
	}

	// give feedback on the player view and audibly when armor is helping
	inventory.armor -= armorSave;

	if ( g_debugDamage.GetInteger() ) {
		gameLocal.Printf( "client:%i health:%i damage:%i armor:%i\n", 
			entityNumber, health, damage, armorSave );
	}

	// move the world direction vector to local coordinates
	ClientDamageEffects ( damageDef->dict, dir, damage );

 	// inform the attacker that they hit someone
 	attacker->DamageFeedback( this, inflictor, damage );
	
//RAVEN BEGIN
//asalmon: Xenon needs stats in singleplayer
#ifndef _XENON
	if( gameLocal.isMultiplayer )
#endif 
	{
//RAVEN END
		idEntity* attacker = NULL;

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		int methodOfDeath = -1;
		if ( inflictor->IsType( idProjectile::GetClassType() ) ) {
// RAVEN END
			methodOfDeath = static_cast<idProjectile*>(inflictor)->methodOfDeath;
			attacker = static_cast<idProjectile*>(inflictor)->GetOwner();
		} else if ( inflictor->IsType( idPlayer::Type ) ) {
			// hitscan weapon
			methodOfDeath = static_cast<idPlayer*>(inflictor)->GetCurrentWeapon();
			attacker = inflictor;
		}

		statManager->Damage( attacker, this, methodOfDeath, damage );
	}
		
// RAVEN BEGIN
// MCG - added damage over time
	if ( !inDamageEvent ) {
		if ( damageDef->dict.GetFloat( "dot_duration" ) ) {
			int endTime;
			if ( damageDef->dict.GetFloat( "dot_duration" ) == -1 ) {
				endTime = -1;
			} else {
				endTime = gameLocal.GetTime() + SEC2MS(damageDef->dict.GetFloat( "dot_duration" ));
			}
			int interval = SEC2MS(damageDef->dict.GetFloat( "dot_interval", "0" ));
			if ( endTime == -1 || gameLocal.GetTime() + interval <= endTime ) {//post it again
				PostEventMS( &EV_DamageOverTime, interval, endTime, interval, inflictor, attacker, dir, damageDefName, damageScale, location );
			}
			if ( damageDef->dict.GetString( "fx_dot", NULL ) ) {
				ProcessEvent( &EV_DamageOverTimeEffect, endTime, interval, damageDefName );
			}
			if ( damageDef->dict.GetString( "snd_dot_start", NULL ) ) {
				StartSound ( "snd_dot_start", SND_CHANNEL_ANY, 0, false, NULL );
			}
		}
	}
// RAVEN END

	// do the damage
	if ( damage > 0 ) {
		if ( !gameLocal.isMultiplayer ) {
			if ( g_useDynamicProtection.GetBool() && g_skill.GetInteger() < 2 ) {
				if ( gameLocal.time > lastDmgTime + 500 && dynamicProtectionScale > 0.25f ) {
					dynamicProtectionScale -= 0.05f;
				}
			}

			if ( dynamicProtectionScale > 0.0f ) {
				damage *= dynamicProtectionScale;
			}
		}

		if ( damage < 1 ) {
			damage = 1;
		}

		int oldHealth = health;
		health -= damage;

		GAMELOG_ADD ( va("player%d_damage_taken", entityNumber ), damage );
		GAMELOG_ADD ( va("player%d_damage_%s", entityNumber, damageDefName), damage );

		// Check undying mode
		if ( !damageDef->dict.GetBool( "noGod" ) ) {
			if ( undying ) {
				if ( health < 1 ) {
					health = 1;
				}
			}
		}

		if ( health <= 0 ) {

			if ( health < -999 ) {
				health = -999;
			}

 			isTelefragged = damageDef->dict.GetBool( "telefrag" );
 
 			lastDmgTime = gameLocal.time;

			Killed( inflictor, attacker, damage, dir, location );

			if ( oldHealth > 0 ) {	
				float pushScale = 1.0f;
				if ( inflictor && inflictor->IsType ( idPlayer::Type ) ) {
					pushScale = static_cast<idPlayer*>(inflictor)->PowerUpModifier ( PMOD_PROJECTILE_DEATHPUSH );
				}
				InitDeathPush ( dir, location, &damageDef->dict, pushScale );
			}			
		} else {
			// force a blink
			blink_time = 0;

			// let the anim script know we took damage
 			pfl.pain = Pain( inflictor, attacker, damage, dir, location );
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

	lastDamageDir = dir;
  	lastDamageDir.Normalize();
	lastDamageDef = damageDef->Index();
	lastDamageLocation = location;
}

/*
=====================
idPlayer::CanPlayImpactEffect 
=====================
*/
bool idPlayer::CanPlayImpactEffect( idEntity* attacker, idEntity* target ) 
{
	if ( !gameLocal.isMultiplayer && attacker->IsType( idAI::GetClassType()) && target->IsType( idPlayer::GetClassType())) 
	{
		// don't display impact effects when marines on our team shoot us...
		idPlayer	*player = static_cast<idPlayer *>( target );
		idAI		*ai		= static_cast<idAI *>( attacker );
		if ( player->team == ai->team )
		{
			return false;
		}
	}

	return idAFEntity_Base::CanPlayImpactEffect( attacker, target );
}

/*
=====================
idPlayer::AddDamageEffect
=====================
*/
void idPlayer::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, idEntity* inflictor ) {
	if( gameLocal.isMultiplayer ) {
		if( ! cvarSystem->GetCVarBool("si_teamDamage") && inflictor && inflictor->IsType( idPlayer::GetClassType() ) && gameLocal.IsTeamGame() && ((idPlayer*)inflictor)->team == team ) {
			return;
		}
	}

	idActor::AddDamageEffect ( collision, velocity, damageDefName, inflictor );
}

/*
===========
idPlayer::Teleport
============
*/
void idPlayer::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
 	idVec3 org;
 
 	if ( weapon ) {
 		weapon->LowerWeapon();
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

	// don't do any smoothing with this snapshot
	predictedFrame = gameLocal.framenum;

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
		} else if ( fov > 175.0f ) {
			return 175.0f;
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
		return DefaultFov() + 10.0f + idMath::Cos( ( gameLocal.time + 2000 ) * 0.01 ) * 10.0f;
	}

	if ( influenceFov ) {
		return influenceFov;
	}

	if ( vehicleController.IsDriving() ) {
		rvVehicle * vehicle				= vehicleController.GetVehicle();
		rvVehiclePosition * position	= vehicle ? vehicle->GetPosition( vehicleController.GetPosition() ) : 0;
		rvVehicleWeapon * weapon		= position ? position->GetActiveWeapon() : 0;

		if ( zoomFov.IsDone( gameLocal.time ) ) {
 			fov = ( honorZoom && zoomed && weapon ) ? weapon->GetZoomFov() : DefaultFov();
		} else {
			fov = zoomFov.GetCurrentValue( gameLocal.time );
		}
	} else {
		if ( zoomFov.IsDone( gameLocal.time ) ) {
 			fov = ( honorZoom && zoomed && weapon ) ? weapon->GetZoomFov() : DefaultFov();
		} else {
			fov = zoomFov.GetCurrentValue( gameLocal.time );
		}
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

	weapon->GetAngleOffsets( &weaponAngleOffsetAverages, &weaponAngleOffsetScale, &weaponAngleOffsetMax );

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
	float	weaponOffsetTime;
	float	weaponOffsetScale;

	ofs.Zero();

	weapon->GetTimeOffsets( &weaponOffsetTime, &weaponOffsetScale );

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
		f = ( idMath::Cos( f * 2.0f * idMath::PI ) - 1.0f ) * 0.5f;
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
	gunpos += weapon->GetViewModelOffset ( );

	// as the player changes direction, the gun will take a small lag
	idVec3	gunOfs = GunAcceleratingOffset();
	origin = viewOrigin + ( gunpos + gunOfs ) * viewAxis;

	// on odd legs, invert some angles
	if ( noclip || 1 ) {
		scale = 0;
	} else if ( bobCycle & 128 ) {
		scale = -xyspeed;
	} else {
		scale = xyspeed;
	}

	// gun angles from bobbing
	angles.roll		= scale * bobfracsin * 0.005f + g_gun_roll.GetFloat();
	angles.yaw		= scale * bobfracsin * 0.01f + g_gun_yaw.GetFloat();
	angles.pitch	= xyspeed * bobfracsin * 0.005f + g_gun_pitch.GetFloat();
	
	angles += weapon->GetViewModelAngles();

	// gun angles from turning
 	if ( gameLocal.isMultiplayer ) {
 		idAngles offset = GunTurningOffset();
 		offset *= g_mpWeaponAngleScale.GetFloat();
 		angles += offset;
 	} else {
 		angles += GunTurningOffset();
 	}   

	idVec3 gravity = physicsObj.GetGravityNormal();

// RAVEN BEGIN
// abahr: when looking down, really large deflections cause back of weapons to show
	float landChangeFrac = idMath::Lerp( 0.25f, 0.05f, viewAngles.ToForward() * gravity);
// RAVEN ABAHR

	// drop the weapon when landing after a jump / fall
	delta = gameLocal.time - landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
// RAVEN BEGIN
// abahr: changed to use landChangeFrac
		origin -= gravity * ( landChange*landChangeFrac * delta / LAND_DEFLECT_TIME );
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin -= gravity * ( landChange*landChangeFrac * (LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME );
// RAVEN END
	}

	// speed sensitive idle drift
	if ( !noclip ) {
		scale = xyspeed * 0.5f + 40.0f;
		fracsin = scale * idMath::Sin( MS2SEC( gameLocal.time ) ) * 0.01f;
		angles.roll		+= fracsin;
		angles.yaw		+= fracsin;
		angles.pitch	+= fracsin;
	}

	axis = angles.ToMat3() * viewAxis;
}

/*
===============
idPlayer::OffsetThirdPersonVehicleView
===============
*/
// RAVEN BEGIN
// jnewquist: option to avoid clipping against world
void idPlayer::OffsetThirdPersonVehicleView( bool clip ) {
// RAVEN END
	idVec3			view;
	idVec3			focusAngles;
	trace_t			trace;
	idVec3			focusPoint;
	float			focusDist;
	idVec3			origin;
	idAngles		angles, angles2;
	idEntity*		vehicle;

	assert ( IsInVehicle ( ) );
	
	vehicle = vehicleController.GetVehicle();

	origin = vehicle->GetRenderEntity()->origin;
	angles = vehicle->GetRenderEntity()->axis.ToAngles();
	
	angles.yaw += pm_thirdPersonAngle.GetFloat();
	angles.pitch += 25.0f;

//	angles.pitch += viewAngles.pitch;
//	angles.yaw += viewAngles.yaw;

	focusPoint = origin + angles.ToForward() * THIRD_PERSON_FOCUS_DISTANCE;
	view = origin;
// RAVEN BEGIN
// abahr: taking into account gravity
	view += physicsObj.GetGravityAxis()[2] * 8.0f;
// RAVEN END

	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();

	float speed = vehicle->GetPhysics()->GetLinearVelocity() * 
				  vehicle->GetPhysics()->GetAxis()[0];

	speed = idMath::Fabs( speed );
	speed *= pm_vehicleCameraSpeedScale.GetFloat();
	if( speed > pm_vehicleCameraScaleMax.GetFloat() ) 
	{
		speed = pm_vehicleCameraScaleMax.GetFloat();
	}

	vehicleCameraDist += ( MS2SEC( gameLocal.GetMSec() ) * ( ( pm_vehicleCameraMinDist.GetFloat() + speed ) - vehicleCameraDist ) );

	view -= vehicleCameraDist * renderView->viewaxis[ 0 ];

// RAVEN BEGIN
// jnewquist: option to avoid clipping against world
 	if ( clip ) {
		// trace a ray from the origin to the viewpoint to make sure the view isn't
		// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
		const idVec3 clip_mins( -4.0f, -4.0f, -4.0f );
		const idVec3 clip_maxs( 4.0f, 4.0f, 4.0f );
		const idBounds clip_bounds( clip_mins, clip_maxs );
		idClipModel clipBounds( clip_bounds );// We clip when using a tram gun in the tram car
// ddynerman: multiple clip worlds
		gameLocal.Translation( this, trace, origin, view, &clipBounds, vehicle->GetPhysics()->GetAxis(), MASK_SOLID, vehicle, vehicle->GetBindMaster() ); 
		if ( trace.fraction != 1.0 ) 
		{
			view = trace.endpos;
// abahr: taking into account gravity
			view += physicsObj.GetGravityAxis()[2] * ( 1.0f - trace.fraction ) * 32;

			// try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out
// ddynerman: multiple clip worlds
			gameLocal.Translation( this, trace, origin, view, &clipBounds, vehicle->GetPhysics()->GetAxis(), MASK_SOLID, vehicle, vehicle->GetBindMaster() ); 
			view = trace.endpos;
		}
	}
// RAVEN END

	// select pitch to look at focus point from vieword
	focusPoint -= view;
	focusDist = idMath::Sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) 
	{
		focusDist = 1;	// should never happen
	}

	angles.pitch = - RAD2DEG( idMath::ATan( focusPoint.z, focusDist ) );

	renderView->vieworg = view;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();
	renderView->viewID = 0;
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
// RAVEN BEGIN
// abahr: taking into account gravity
	view += physicsObj.GetGravityAxis()[2] * (8.0f + height);
// RAVEN END

	angles.pitch *= 0.5f;
	renderView->viewaxis = angles.ToMat3() * physicsObj.GetGravityAxis();

	idMath::SinCos( DEG2RAD( angle ), sideScale, forwardScale );
	view -= range * forwardScale * renderView->viewaxis[ 0 ];
	view += range * sideScale * renderView->viewaxis[ 1 ];

 	if ( clip ) {
 		// trace a ray from the origin to the viewpoint to make sure the view isn't
 		// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
 		bounds = idBounds( idVec3( -4, -4, -4 ), idVec3( 4, 4, 4 ) );
// RAVEN BEGIN
// ddynerman: multiple clip worlds
		gameLocal.TraceBounds( this, trace, origin, view, bounds, MASK_SOLID, this );
// RAVEN END
 		if ( trace.fraction != 1.0f ) {
 			view = trace.endpos;
// RAVEN BEGIN
// abahr: taking into account gravity
 			view += physicsObj.GetGravityAxis()[2] * ( 1.0f - trace.fraction ) * 32.0f;
// RAVEN END
   
 			// try another trace to this position, because a tunnel may have the ceiling
 			// close enough that this is poking out
// RAVEN BEGIN
// ddynerman: multiple clip worlds
			gameLocal.TraceBounds( this, trace, origin, view, bounds, MASK_SOLID, this );
// RAVEN END
 			view = trace.endpos;
 		}
   	}
   

	// select pitch to look at focus point from vieword
	focusPoint -= view;
	focusDist = idMath::Sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1.0f ) {
		focusDist = 1.0f;	// should never happen
	}

	angles.pitch = - RAD2DEG( idMath::ATan( focusPoint.z, focusDist ) );
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
 
	if ( WantSmoothing() ) {
		org = predictedOrigin;
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
	} else if ( IsInVehicle ( ) ) {	
		vehicleController.GetEyePosition ( origin, axis );

  		idVec3		shakeOffset;
  		idAngles	shakeAngleOffset;
	   	idBounds	relBounds(idVec3(0, 0, 0), idVec3(0, 0, 0));
  		playerView.ShakeOffsets( shakeOffset, shakeAngleOffset, relBounds );

  		origin += shakeOffset;
  		axis = (shakeAngleOffset + playerView.AngleOffset()).ToMat3() * axis;
	} else {
  		idVec3		shakeOffset;
  		idAngles	shakeAngleOffset;
	   	idBounds	relBounds(idVec3(0, 0, 0), idVec3(0, 0, 0));

  		playerView.ShakeOffsets( shakeOffset, shakeAngleOffset, relBounds );
  		origin = GetEyePosition() + viewBob + shakeOffset;  		
		angles = viewAngles + viewBobAngles + shakeAngleOffset + playerView.AngleOffset();

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
idPlayer::SmoothenRenderView

various situations where the view angles need smoothing:

demo replay:
  On a slow client with low fps multiple game frames are run in quick succession
  inbetween rendered frames. As a result the usercmds are not recorded at fixed
  time intervals but in small bursts. This routine interpolates the view angles
  based on the real time at which the usercmds were recorded to make a demo
  recorded on a slow client play back smoothly on a fast client.

spectate follow?
==================
*/
void idPlayer::SmoothenRenderView( bool firstPerson ) {
	int d1, d2;
	idAngles angles, anglesDelta, newAngles;

	if ( gameLocal.GetDemoState() == DEMO_PLAYING ) {

		d1 = usercmd.gameTime - demoViewAngleTime;
		if ( d1 < 0 ) {
			return;
		}
		d2 = usercmd.realTime - demoViewAngleTime;
		if ( d2 <= 0 ) {
			return;
		}
		if ( d1 >= d2 ) {
			return;
		}

		angles = renderView->viewaxis.ToAngles();

		anglesDelta = angles - demoViewAngles;
		anglesDelta.Normalize180();
		newAngles = demoViewAngles + ( (float) d1 / d2 ) * anglesDelta;
		renderView->viewaxis = newAngles.ToMat3();

		if ( usercmd.gameTime + gameLocal.msec > usercmd.realTime ) {
			demoViewAngleTime = usercmd.realTime;
			demoViewAngles = angles;
		}

		if ( firstPerson ) {
			// make sure the view weapon moves smoothly
			firstPersonViewAxis = renderView->viewaxis;
		}
	}
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
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_PUSH_HEAP_MEM_AUTO(p0,this);
// RAVEN END
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
		bool cameraIsSet = false;

		// First try out any camera views that can possibly fail.
		if( !cameraIsSet ){
			if ( g_stopTime.GetBool() ) {
	 			renderView->vieworg = firstPersonViewOrigin;
	 			renderView->viewaxis = firstPersonViewAxis;
				SmoothenRenderView( true );
 
	 			if ( !pm_thirdPerson.GetBool() ) {
	 				// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
	 				// allow the right player view weapons
	 				renderView->viewID = entityNumber + 1;
	 			}
			} else if ( pm_thirdPerson.GetBool() && IsInVehicle ( ) ) {
// RAVEN BEGIN
// jnewquist: option to avoid clipping against world
				OffsetThirdPersonVehicleView( pm_thirdPersonClip.GetBool() );
// RAVEN END
				SmoothenRenderView( false );
			} else if ( pm_thirdPerson.GetBool() ) {
				OffsetThirdPersonView( pm_thirdPersonAngle.GetFloat(), pm_thirdPersonRange.GetFloat(), pm_thirdPersonHeight.GetFloat(), pm_thirdPersonClip.GetBool() );
				SmoothenRenderView( false );
			} else if ( pm_thirdPersonDeath.GetBool() ) {
				range = gameLocal.time < minRespawnTime ? ( gameLocal.time + RAGDOLL_DEATH_TIME - minRespawnTime ) * ( 120.0f / RAGDOLL_DEATH_TIME ) : 120.0f;
	 			OffsetThirdPersonView( 0.0f, 20.0f + range, 0.0f, false );
				SmoothenRenderView( false );
			} else {
				renderView->vieworg = firstPersonViewOrigin;
				renderView->viewaxis = firstPersonViewAxis;
				SmoothenRenderView( true );
				// set the viewID to the clientNum + 1, so we can suppress the right player bodies and
				// allow the right player view weapons
				renderView->viewID = entityNumber + 1;
			}
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
==================
idPlayer::Event_EnableTarget
==================
*/
void idPlayer::Event_EnableTarget ( void ) {
	fl.notarget = false;
}

/*
==================
idPlayer::Event_DisableTarget
==================
*/
void idPlayer::Event_DisableTarget ( void ) {
	fl.notarget = true;
}

/*
==================
idPlayer::Event_GetViewPos
==================
*/
void idPlayer::Event_GetViewPos( void ) {
	idVec3		viewOrigin;
	idMat3		viewAxis;

	GetViewPos(viewOrigin, viewAxis);
	idThread::ReturnVector( viewOrigin );
}

/*
==================
idPlayer::Event_FinishHearingLoss
==================
*/
void idPlayer::Event_FinishHearingLoss ( float fadeTime ) {
	if ( fadeTime <= 0.0f ) {
		StopSound ( SND_CHANNEL_DEMONIC, false );
		pfl.hearingLoss = false;
	} else {
		soundSystem->FadeSoundClasses( SOUNDWORLD_GAME, 0, 0.0f, fadeTime );
		PostEventSec ( &EV_Player_FinishHearingLoss, fadeTime, 0.0f );
	}
}

// RAVEN BEGIN
// twhitaker: added the event
/*
=============
idPlayer::Event_ApplyImpulse
=============
*/
void idPlayer::Event_ApplyImpulse ( idEntity* ent, idVec3 &point, idVec3 &impulse ) {
	GetPhysics()->ApplyImpulse( 0, point, impulse );
}

// mekberg: added Event_EnableObjectives
/*
=============
idPlayer::Event_EnableObjectives
=============
*/
void idPlayer::Event_EnableObjectives ( void ) {
	objectivesEnabled = true;
}

// mekberg: added Event_DisableObjectives
/*
=============
idPlayer::Event_DisableObjectives
=============
*/
void idPlayer::Event_DisableObjectives ( void ) {
	// if it's open, it should be closed
	if (objectiveSystemOpen ) {
		ToggleObjectives();
	}
	objectivesEnabled = false;
}

// mekberg: added Event_AllowNewObjectives
void idPlayer::Event_AllowNewObjectives ( void ) {
	showNewObjectives = true;
}

// mekberg: added sethealth
/*
=============
idPlayer::Event_SetHealth
=============
*/
void idPlayer::Event_SetHealth( float newHealth ) {
	health = idMath::ClampInt( 1 , inventory.maxHealth, newHealth );
}
/*
=============
idPlayer::Event_SetArmor
=============
*/
void idPlayer::Event_SetArmor( float newArmor ) {
	inventory.armor = idMath::ClampInt( 0 , inventory.maxarmor, newArmor );
}

/*
=============
idPlayer::Event_SetExtraProjPassEntity
=============
*/
void idPlayer::Event_SetExtraProjPassEntity( idEntity* _extraProjPassEntity ) {
	extraProjPassEntity = _extraProjPassEntity;
}
// RAVEN END

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
void idPlayer::SetLastHitTime( int time, bool armorHit ) {
 	if ( !time ) {
 		// level start and inits
 		return;
 	}

	idUserInterface *cursor		= idPlayer::cursor;
	bool spectated = false;
	if ( gameLocal.GetLocalPlayer() ) {
		idPlayer *p = gameLocal.GetLocalPlayer();
		if ( p->spectating && p->spectator == entityNumber ) {
			cursor = p->GetCursorGUI();
			spectated = true;
		}
	} else if ( gameLocal.IsServerDemo() ) {
		// server netdemo
		assert( gameLocal.GetDemoState() == DEMO_PLAYING );
		if ( gameLocal.GetDemoFollowClient() == entityNumber ) {
			cursor = gameLocal.GetDemoCursor();
			spectated = true;
		}
	}

	if ( lastHitTime != time ) {
		if ( cursor ) {
			cursor->HandleNamedEvent( "weaponHit" );
		}
		if ( gameLocal.isMultiplayer ) {			
			// spectated so we get sounds for a client we're following
			// localClientNum check so listen server plays only for local player
			if ( spectated || gameLocal.localClientNum == entityNumber ) {
				const char* sound;

				if ( armorHit ) {
					if ( spawnArgs.GetString ( "snd_armorHit", "", &sound ) ) {
						soundSystem->PlayShaderDirectly( SOUNDWORLD_GAME, sound );
					}
				} else {
					if ( spawnArgs.GetString ( "snd_weaponHit", "", &sound ) ) {
						soundSystem->PlayShaderDirectly( SOUNDWORLD_GAME, sound );
					}
				}
			}
		
			if ( aimClientNum != -1 ) {
				if ( mphud ) {
					mphud->HandleNamedEvent( "aim_hit" );
				}
			}

		}
		lastHitTime = time;
		lastArmorHit = armorHit;
		lastHitToggle ^= 1;
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
				if ( ent->IsType( idProjectile::GetClassType() ) ) {
// RAVEN END
					// remove all projectiles
					ent->PostEventMS( &EV_Remove, 0 );
				}
			}
			if ( weaponEnabled && weapon ) {
				weapon->EnterCinematic();
			}
		} else {
			physicsObj.SetLinearVelocity( vec3_origin );
			if ( weaponEnabled && weapon ) {
				weapon->ExitCinematic();
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
================
idPlayer::Event_SetViewAngles
================
*/
void idPlayer::Event_SetViewAngles( const idVec3 & vec ) {
	idAngles ang;
	ang.Set( vec.z, vec.y, vec.x );
	SetViewAngles( ang );
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
	gameLocal.world->spawnArgs.SetBool( "no_Weapons", 0 );
	Give( "weapon", spawnArgs.GetString( va( "def_weapon%d", 0 ) ) );
 	hiddenWeapon = false;
	weaponEnabled = true;
	if ( weapon ) {
 		weapon->ExitCinematic();
	}
	ShowCrosshair();
}

/*
==================
idPlayer::Event_DisableWeapon
==================
*/
void idPlayer::Event_DisableWeapon( void ) {
 	hiddenWeapon = true;
	weaponEnabled = false;
   	if ( weapon ) {
		weapon->EnterCinematic();
   	}
	HideCrosshair();
}

/*
==================
idPlayer::Event_GetCurrentWeapon
==================
*/
void idPlayer::Event_GetCurrentWeapon( void ) {
	if ( currentWeapon >= 0 ) {
		idThread::ReturnString( spawnArgs.GetString( va( "def_weapon%d", currentWeapon ) ) );
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
	if ( previousWeapon >= 0 ) {
		int pw = ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) ? 0 : previousWeapon;
		idThread::ReturnString( spawnArgs.GetString( va( "def_weapon%d", pw) ) );
	} else {
		idThread::ReturnString( "def_weapon0" );
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

	weaponNum = -1;
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		if ( inventory.weapons & ( 1 << i ) ) {
			const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if ( !idStr::Cmp( weap, weaponName ) ) {

				if ( !inventory.HasAmmo( weap ) ) {
					return;
				}
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
idPlayer::Event_GetAmmoData
==================
*/
void idPlayer::Event_GetAmmoData( const char *ammoClass ) {

	idVec3	weaponAmmo;

	//ammo vector is this: current ammo count, max ammo count, and %
	weaponAmmo.x = inventory.ammo[ inventory.AmmoIndexForAmmoClass( ammoClass) ];
	weaponAmmo.y = inventory.MaxAmmoForAmmoClass( this, ammoClass );

	if( weaponAmmo.y == 0)
		weaponAmmo.z = 0;
	else
		weaponAmmo.z = (float)(weaponAmmo.x / weaponAmmo.y);

	idThread::ReturnVector( weaponAmmo);
}

/*
==================
idPlayer::Event_RefillAmmo
==================
*/
void idPlayer::Event_RefillAmmo( void ) {
	int a;
	for ( int i = 0; i < MAX_WEAPONS; i++ ) {
		if ( inventory.weapons & ( 1 << i ) ) {
			const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if ( weap && *weap ) {
				a = inventory.AmmoIndexForWeaponIndex( i );
				inventory.ammo[ a ] = inventory.MaxAmmoForAmmoClass( this, rvWeapon::GetAmmoNameForIndex( a ) );
			}
		}
	}
}

/*
==================
idPlayer::Event_AllowFallDamage
==================
*/
void idPlayer::Event_AllowFallDamage( int toggle ) {
	if( toggle )	{
		pfl.noFallingDamage = false;
	} else {
		pfl.noFallingDamage = true;
	}

}
/*
==================
idPlayer::Event_GetWeaponEntity
==================
*/
void idPlayer::Event_GetWeaponEntity( void ) {
	idThread::ReturnEntity( weaponViewModel );
}

/*
==================
idPlayer::Event_HideDatabaseEntry
==================
*/
void idPlayer::Event_HideDatabaseEntry ( void ) {
	if ( hud ) {
		hud->HandleNamedEvent( "closeDatabaseEntry" );
	}
}

/*
==================
idPlayer::Event_ZoomIn
==================
*/
void idPlayer::Event_ZoomIn ( void ) {
	float currentFov;
	float t;
	
	if ( zoomed ) {
		return;
	}
	
	if ( vehicleController.IsDriving() ) {
		rvVehicle * vehicle				= vehicleController.GetVehicle();
		rvVehiclePosition * position	= vehicle ? vehicle->GetPosition( vehicleController.GetPosition() ) : 0;
		rvVehicleWeapon * weapon		= position ? position->GetActiveWeapon() : 0;

		if( !weapon ) {
			// this should only happen in fringe cases - zooming in while dead, etc
			zoomFov.Init( gameLocal.time, 0, DefaultFov(), DefaultFov() );
			zoomed = false;
			return;
		}

		currentFov = CalcFov ( true );
		t  = currentFov - weapon->GetZoomFov();
		t /= (DefaultFov() - weapon->GetZoomFov());
		t *= weapon->GetZoomTime();

		zoomFov.Init( gameLocal.time, SEC2MS(t), currentFov, weapon->GetZoomFov() );
				
		zoomed = true;
		if ( weapon->GetZoomGui() )	{
			weapon->GetZoomGui()->HandleNamedEvent ( "zoomIn" );
			weaponViewModel->StartSound ( "snd_zoomin", SND_CHANNEL_ANY, 0, false, NULL );
		}
	} else if ( weapon && this->weaponEnabled ) {
		currentFov = CalcFov ( true );
		t  = currentFov - weapon->GetZoomFov();
		t /= (DefaultFov() - weapon->GetZoomFov());
		t *= weapon->GetZoomTime();

		zoomFov.Init( gameLocal.time, SEC2MS(t), currentFov, weapon->GetZoomFov() );
				
		zoomed = true;
		if ( weapon->GetZoomGui() )	{
			weapon->GetZoomGui()->HandleNamedEvent ( "zoomIn" );
			weaponViewModel->StartSound ( "snd_zoomin", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
}

/*
==================
idPlayer::Event_ZoomOut
==================
*/
void idPlayer::Event_ZoomOut ( void ) {
	float t;
	float currentFov;

	if ( !zoomed ) {
		return;
	}
	
	if ( vehicleController.IsDriving() ) {
		rvVehicle * vehicle				= vehicleController.GetVehicle();
		rvVehiclePosition * position	= vehicle ? vehicle->GetPosition( vehicleController.GetPosition() ) : 0;
		rvVehicleWeapon * weapon		= position ? position->GetActiveWeapon() : 0;

		if( !weapon ) {
			// this should only happen in fringe cases - zooming out while dead, etc
			zoomFov.Init( gameLocal.time, 0, DefaultFov(), DefaultFov() );
			zoomed = false;
			return;
		}

		currentFov = CalcFov ( true );
		t  = currentFov - weapon->GetZoomFov();
		t /= (DefaultFov() - weapon->GetZoomFov());
		t  = (1.0f - t) * weapon->GetZoomTime();

		zoomFov.Init( gameLocal.time, SEC2MS(t), currentFov, DefaultFov() );
		zoomed = false;
		if ( weapon->GetZoomGui() )	{
			weaponViewModel->StartSound ( "snd_zoomout", SND_CHANNEL_ANY, 0, false, NULL );
		}
	} else {
		if( !weapon ) {
			// this should only happen in fringe cases - zooming out while dead, etc
			zoomFov.Init( gameLocal.time, 0, DefaultFov(), DefaultFov() );
			zoomed = false;
			return;
		}

		currentFov = CalcFov ( true );
		t  = currentFov - weapon->GetZoomFov();
		t /= (DefaultFov() - weapon->GetZoomFov());
		t  = (1.0f - t) * weapon->GetZoomTime();

		zoomFov.Init( gameLocal.time, SEC2MS(t), currentFov, DefaultFov() );
		zoomed = false;
		if ( weapon->GetZoomGui() )	{
			weaponViewModel->StartSound ( "snd_zoomout", SND_CHANNEL_ANY, 0, false, NULL );
		}
	}
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
 	exitEnt = teleportEntity;
 	if ( !exitEnt ) {
		common->DPrintf( "Event_ExitTeleporter player %d while not being teleported\n", entityNumber );
		return;
	}

	pushVel = exitEnt->spawnArgs.GetFloat( "push", "300" );

 	if ( gameLocal.isServer ) {
 		ServerSendInstanceEvent( EVENT_EXIT_TELEPORTER, NULL, false, -1 );
   	}

	SetPrivateCameraView( NULL );
	// setup origin and push according to the exit target
	SetOrigin( exitEnt->GetPhysics()->GetOrigin() + idVec3( 0, 0, CM_CLIP_EPSILON ) );
	SetViewAngles( exitEnt->GetPhysics()->GetAxis().ToAngles() );
	physicsObj.SetLinearVelocity( exitEnt->GetPhysics()->GetAxis()[ 0 ] * pushVel );
	physicsObj.ClearPushedVelocity( );
	// teleport fx
	playerView.Flash( colorWhite, 120 );

 	// clear the ik heights so model doesn't appear in the wrong place
 	walkIK.EnableAll();

	UpdateVisuals();

	gameLocal.PlayEffect( spawnArgs, "fx_teleport", GetPhysics()->GetOrigin(), idVec3(0,0,1).ToMat3(), false, vec3_origin );

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
===============
idPlayer::Event_DamageOverTimeEffect
===============
*/
void idPlayer::Event_DamageOverTimeEffect( int endTime, int interval, const char *damageDefName ) {
	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if ( damageDef ) {
		rvClientCrawlEffect* effect;

		// mwhitlock: Dynamic memory consolidation
		RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_MULTIPLE_FRAME);
		effect = new rvClientCrawlEffect( gameLocal.GetEffect ( damageDef->dict, "fx_dot" ), GetWeaponViewModel(), interval );
		RV_POP_HEAP();

		effect->Play ( gameLocal.time, false );
		if ( endTime == -1 || gameLocal.GetTime() + interval <= endTime ) {
			//post it again
			PostEventMS( &EV_DamageOverTimeEffect, interval, endTime, interval, damageDefName );
		}
	}
}

/*
===============
idPlayer::LocalClientPredictionThink
===============
*/
void idPlayer::LocalClientPredictionThink( void ) {
	renderEntity_t *headRenderEnt;

	oldFlags = usercmd.flags;
	oldButtons = usercmd.buttons;

	usercmd = gameLocal.usercmds[ entityNumber ];

	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;

	if ( idealWeapon != currentWeapon ) {
		usercmd.buttons &= ~BUTTON_ATTACK;		
	}

 	// clear the ik before we do anything else so the skeleton doesn't get updated twice
 	walkIK.ClearJointMods();

	if ( gameLocal.isNewFrame ) {
		if ( ( usercmd.flags & UCF_IMPULSE_SEQUENCE ) != ( oldFlags & UCF_IMPULSE_SEQUENCE ) ) {
			PerformImpulse( usercmd.impulse );
		}
	}

	if ( forceScoreBoard && forceScoreBoardTime && gameLocal.time > forceScoreBoardTime ) {
		forceScoreBoardTime = 0;
		forceScoreBoard = false;
	}
	scoreBoardOpen = ( ( usercmd.buttons & BUTTON_SCORES ) != 0 || forceScoreBoard );

	// zooming
	bool zoom = (usercmd.buttons & BUTTON_ZOOM) && CanZoom();
	if ( zoom != zoomed ) {
		if ( zoom ) {
			ProcessEvent( &EV_Player_ZoomIn );
		} else {
			ProcessEvent( &EV_Player_ZoomOut );
		}
	}

	if ( IsInVehicle( ) ) {	
		vehicleController.SetInput( usercmd, viewAngles );
				
		// calculate the exact bobbed view position, which is used to
		// position the view weapon, among other things
		CalculateFirstPersonView();

		// this may use firstPersonView, or a thirdPeoson / camera view
		CalculateRenderView();

		UpdateLocation();

		if ( !fl.hidden ) {
			UpdateAnimation();
			Present();
		}			
		
		return;
	}

	AdjustSpeed();

	UpdateViewAngles();

/*
// RAVEN BEGIN
// abahr
	if( !noclip && !spectating ) {
		UpdateGravity();
	}
// RAVEN END
*/

 	if ( !isLagged ) {
 		// don't allow client to move when lagged
		predictedUpdated = false;
 		Move();

		// predict collisions with items
		if ( !noclip && !spectating && ( health > 0 ) && !IsHidden() ) {
			TouchTriggers( &idItem::GetClassType() );
		}
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
	pfl.pain = false;

	if ( !af.IsActive() ) {
		AdjustBodyAngles();
	}

	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();

	// this may use firstPersonView, or a thirdPerson / camera view
	CalculateRenderView();

 	if ( !gameLocal.inCinematic && weaponViewModel && ( health > 0 ) && !( gameLocal.isMultiplayer && spectating ) ) {
		UpdateWeapon();
	}

	UpdateHud();

 	if ( gameLocal.isNewFrame ) {
 		UpdatePowerUps();
 	}

 	UpdateDeathSkin( false );

	UpdateDeathShader( deathStateHitch );

	if( gameLocal.isMultiplayer ) {
		if ( clientHead.GetEntity() ) {
			headRenderEnt = clientHead.GetEntity()->GetRenderEntity();
		} else {
			headRenderEnt = NULL;
		}
	} else {
		if ( head.GetEntity() ) {
			headRenderEnt = head.GetEntity()->GetRenderEntity();
		} else {
			headRenderEnt = NULL;
		}
	}

 	if ( headRenderEnt ) {
		// in MP, powerup skin overrides influence 	
		if ( powerUpSkin ) {
			headRenderEnt->customSkin = powerUpSkin;
		} else if ( influenceSkin ) {
			headRenderEnt->customSkin = influenceSkin;
		} else {
			headRenderEnt->customSkin = headSkin;
		}
 		
		headRenderEnt->suppressSurfaceInViewID = entityNumber + 1;
 	}

	// always show your own shadow
	renderEntity.suppressLOD = 1;
	if ( headRenderEnt ) {
		headRenderEnt->suppressLOD = 1;
	}

	DrawShadow( headRenderEnt );

	// never cast shadows from our first-person muzzle flashes
	// FIXME: flashlight too
	renderEntity.suppressShadowInLightID = rvWeapon::WPLIGHT_MUZZLEFLASH * 100 + entityNumber;
 	if ( headRenderEnt ) {
 		headRenderEnt->suppressShadowInLightID = renderEntity.suppressShadowInLightID;
   	}

 	if ( !gameLocal.inCinematic ) {
 		UpdateAnimation();
 	}

	Present();

 	LinkCombat();
}

/*
===============
idPlayer::NonLocalClientPredictionThink
===============
*/
#define LIMITED_PREDICTION		1

void idPlayer::NonLocalClientPredictionThink( void ) {
	renderEntity_t *headRenderEnt;

	oldFlags = usercmd.flags;
	oldButtons = usercmd.buttons;

	usercmd = gameLocal.usercmds[ entityNumber ];

	buttonMask &= usercmd.buttons;
	usercmd.buttons &= ~buttonMask;

	//jshepard: added this to make sure clients can see other clients and the host switching weapons
	if ( idealWeapon != currentWeapon )	{
		usercmd.buttons &= ~BUTTON_ATTACK;		
	}

 	// clear the ik before we do anything else so the skeleton doesn't get updated twice
 	walkIK.ClearJointMods();

	if ( gameLocal.isNewFrame ) {
		if ( ( usercmd.flags & UCF_IMPULSE_SEQUENCE ) != ( oldFlags & UCF_IMPULSE_SEQUENCE ) ) {
			PerformImpulse( usercmd.impulse );
		}
	}

	if ( forceScoreBoard && forceScoreBoardTime && gameLocal.time > forceScoreBoardTime ) {
		forceScoreBoardTime = 0;
		forceScoreBoard = false;
	}
	scoreBoardOpen = ( ( usercmd.buttons & BUTTON_SCORES ) != 0 || forceScoreBoard );

	// zooming
	bool zoom = (usercmd.buttons & BUTTON_ZOOM) && CanZoom();
	if ( zoom != zoomed ) {
		if ( zoom ) {
			ProcessEvent( &EV_Player_ZoomIn );
		} else {
			ProcessEvent( &EV_Player_ZoomOut );
		}
	}

#if !LIMITED_PREDICTION
	if ( IsInVehicle ( ) ) {	
		vehicleController.SetInput ( usercmd, viewAngles );

		// calculate the exact bobbed view position, which is used to
		// position the view weapon, among other things
		CalculateFirstPersonView();

		// this may use firstPersonView, or a thirdPeoson / camera view
		CalculateRenderView();

		UpdateLocation();

		if ( !fl.hidden ) {
			UpdateAnimation();
			Present();
		}			
		
		return;
	}
#endif

	AdjustSpeed();

	UpdateViewAngles();

	if ( gameLocal.isLastPredictFrame && jumpDuringHitch ) {
		// only play sound if still alive
		if ( health > 0 ) {
			StartSound( "snd_jump", (s_channelType)FC_SOUND, 0, false, NULL );
		}
		jumpDuringHitch = false;
	}

	if ( !isLagged ) {
 		// don't allow client to move when lagged
		predictedUpdated = false;
		// NOTE: only running on new frames causes prediction errors even when the input does not change!
		if ( gameLocal.isNewFrame ) {
 			Move();
		} else {
			PredictionErrorDecay();
		}
 	}

#if defined( _XENON ) || !LIMITED_PREDICTION
	// update GUIs, Items, and character interactions
	UpdateFocus();
#endif

	// service animations
 	if ( !spectating && !af.IsActive() ) {
    	UpdateConditions();
		UpdateAnimState();
		CheckBlink();
	}

	// clear out our pain flag so we can tell if we recieve any damage between now and the next time we think
	pfl.pain = false;

	if ( !af.IsActive() ) {
		AdjustBodyAngles();
	}

	// calculate the exact bobbed view position, which is used to
	// position the view weapon, among other things
	CalculateFirstPersonView();

 	if ( !gameLocal.inCinematic && weaponViewModel && ( health > 0 ) && !( gameLocal.isMultiplayer && spectating ) ) {
		UpdateWeapon();
	}

	if ( gameLocal.isLastPredictFrame ) {
		// this may use firstPersonView, or a thirdPerson / camera view
		CalculateRenderView();

		UpdateHud();
 		UpdatePowerUps();
 	}

//#if !LIMITED_PREDICTION
 	UpdateDeathSkin( false );

	UpdateDeathShader( deathStateHitch );
//#endif

	if( gameLocal.isMultiplayer ) {
		if ( clientHead.GetEntity() ) {
			headRenderEnt = clientHead.GetEntity()->GetRenderEntity();
		} else {
			headRenderEnt = NULL;
		}
	} else {
		if ( head.GetEntity() ) {
			headRenderEnt = head.GetEntity()->GetRenderEntity();
		} else {
			headRenderEnt = NULL;
		}
	}

	if ( headRenderEnt ) {
		// in MP, powerup skin overrides influence 	
		if ( powerUpSkin ) {
			headRenderEnt->customSkin = powerUpSkin;
		} else if ( influenceSkin ) {
 			headRenderEnt->customSkin = influenceSkin;
 		} else {
 			headRenderEnt->customSkin = headSkin;
 		}
 	}

	// always show your own shadow
	renderEntity.suppressLOD = 1;
	if ( headRenderEnt ) {
		headRenderEnt->suppressLOD = 1;
	}

	DrawShadow( headRenderEnt );

	// never cast shadows from our first-person muzzle flashes
	// FIXME: flashlight too
	renderEntity.suppressShadowInLightID = rvWeapon::WPLIGHT_MUZZLEFLASH * 100 + entityNumber;
 	if ( headRenderEnt ) {
 		headRenderEnt->suppressShadowInLightID = renderEntity.suppressShadowInLightID;
   	}

 	if ( !gameLocal.inCinematic ) {
 		UpdateAnimation();
 	}

	Present();

 	LinkCombat();
}

/*
================
idPlayer::ClientPredictionThink
================
*/
void idPlayer::ClientPredictionThink( void ) {
	
	if ( doInitWeapon ) {
		InitWeapon();
	}
	
	// common code for both the local & non local clients
	if ( reloadModel ) {
		LoadDeferredModel();
		reloadModel = false;
	}

	if ( entityNumber == gameLocal.GetDemoFollowClient() ) {
		LocalClientPredictionThink();
		return;
	}

	if ( entityNumber == gameLocal.localClientNum ) {
		LocalClientPredictionThink();
		return;
	}

	assert( gameLocal.localClientNum >= 0 );
	idPlayer *p = gameLocal.GetClientByNum( gameLocal.localClientNum );
	if ( p && p->spectating && p->spectator == entityNumber ) {
		LocalClientPredictionThink();
		return;
	}

	NonLocalClientPredictionThink();
}

/*
================
idPlayer::GetMasterPosition
================
*/
bool idPlayer::GetMasterPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const {
	if( !IsInVehicle() ) {
		return idActor::GetMasterPosition( masterOrigin, masterAxis );	
	}
	
	vehicleController.GetDriverPosition( masterOrigin, masterAxis );
	return true;
}

/*
===============
idPlayer::PredictionErrorDecay
===============
*/
void idPlayer::PredictionErrorDecay( void ) {
	if ( predictedUpdated ) {
		return;
	}

	if ( net_predictionErrorDecay.GetFloat() <= 0.0f ) {
		idMat3 renderAxis = viewAxis * GetPhysics()->GetAxis();
		idVec3 renderOrigin = GetPhysics()->GetOrigin() + modelOffset * renderAxis;
		predictedOrigin = renderOrigin;
		return;
	}

	if ( gameLocal.framenum >= predictedFrame ) {
		idMat3 renderAxis = viewAxis * GetPhysics()->GetAxis();
		idVec3 renderOrigin = GetPhysics()->GetOrigin() + modelOffset * renderAxis;

		if ( gameLocal.framenum == predictedFrame ) {

			predictionOriginError = predictedOrigin - renderOrigin;
			predictionAnglesError = predictedAngles - viewAngles;
			predictionAnglesError.Normalize180();
			predictionErrorTime = gameLocal.time;

			if ( net_showPredictionError.GetInteger() == entityNumber ) {
				renderSystem->DebugGraph( predictionOriginError.Length(), 0.0f, 100.0f, colorGreen );
				renderSystem->DebugGraph( predictionAnglesError.Length(), 0.0f, 180.0f, colorBlue );
			}
		}

		int t = gameLocal.time - predictionErrorTime;
		float f = ( net_predictionErrorDecay.GetFloat() - t ) / net_predictionErrorDecay.GetFloat();
		if ( f > 0.0f && f < 1.0f ) {
			predictedOrigin = renderOrigin + f * predictionOriginError;
			predictedAngles = viewAngles + f * predictionAnglesError;
			predictedAngles.Normalize180();
		} else {
			predictedOrigin = renderOrigin;
			predictedAngles = viewAngles;
		}

		predictedFrame = gameLocal.framenum;

	}

	viewAngles = predictedAngles;
	// adjust them now so they are right for the bound objects ( head and weapon )
	AdjustBodyAngles();

	predictedUpdated = true;
}

/*
===============
idPlayer::WantSmoothing
===============
*/
bool idPlayer::WantSmoothing( void ) const {
	if ( !gameLocal.isClient ) {
		return false;
	}
	if ( net_predictionErrorDecay.GetFloat() <= 0.0f ) {
		return false;
	}
	return true;
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

	if ( vehicleController.IsDriving() ) {
		vehicleController.GetDriverPosition( origin, axis );
		origin.Zero();
		return true;
	}

	PredictionErrorDecay();

	// smoothen the rendered origin and angles of other clients
	if ( gameLocal.framenum >= predictedFrame && WantSmoothing() ) {

		axis = idAngles( 0.0f, predictedAngles.yaw, 0.0f ).ToMat3();
		origin = ( predictedOrigin - GetPhysics()->GetOrigin() ) * axis.Transpose();

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
	msg.WriteByte( inventory.armor );
 	msg.WriteBits( lastDamageDef, gameLocal.entityDefBits );
	msg.WriteDir( lastDamageDir, 9 );
	msg.WriteShort( lastDamageLocation );
	msg.WriteBits( idealWeapon, idMath::BitsForInteger( MAX_WEAPONS ) );
	msg.WriteBits( inventory.weapons, MAX_WEAPONS );
	msg.WriteBits( weaponViewModel.GetSpawnId(), 32 );
	msg.WriteBits( weaponWorldModel.GetSpawnId(), 32 );
//	msg.WriteBits( head.GetSpawnId(), 32 );
	msg.WriteBits( spectator, idMath::BitsForInteger( MAX_CLIENTS ) );
	msg.WriteBits( lastHitToggle, 1 );
	msg.WriteBits( lastArmorHit, 1 );
 	msg.WriteBits( weaponGone, 1 );
 	msg.WriteBits( isLagged, 1 );
 	msg.WriteBits( isChatting, 1 );
	msg.WriteLong( connectTime );
	msg.WriteByte( lastKiller ? lastKiller->entityNumber : 255 );
 	
	// 	vehicleController.WriteToSnapshot( msg );
 	
 	if ( weapon ) {
 		msg.WriteBits( 1, 1 );
 		weapon->WriteToSnapshot( msg );
 	} else {
 		msg.WriteBits( 0, 1 );
 	}
//RITUAL BEGIN
	msg.WriteBits( inBuyZone, 1 );
	msg.WriteLong( (int)buyMenuCash );
//RITUAL END
}

/*
================
idPlayer::ReadFromSnapshot
================
*/
void idPlayer::ReadFromSnapshot( const idBitMsgDelta &msg ) {
 	int		i, oldHealth, newIdealWeapon, weaponSpawnId, weaponWorldSpawnId;
 	bool	newHitToggle, stateHitch, newHitArmor;
	int		lastKillerEntity;
	bool	proto69 = ( gameLocal.GetCurrentDemoProtocol() == 69 );

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
	inventory.armor = msg.ReadByte();
 	lastDamageDef = msg.ReadBits( gameLocal.entityDefBits );
	lastDamageDir = msg.ReadDir( 9 );
	lastDamageLocation = msg.ReadShort();
	newIdealWeapon = msg.ReadBits( idMath::BitsForInteger( MAX_WEAPONS ) );
	inventory.weapons = msg.ReadBits( MAX_WEAPONS );
 	weaponSpawnId = msg.ReadBits( 32 );
 	weaponWorldSpawnId = msg.ReadBits( 32 );
	int latchedSpectator = spectator;
	spectator = msg.ReadBits( idMath::BitsForInteger( MAX_CLIENTS ) );
	if ( spectating && latchedSpectator != spectator && this == gameLocal.GetLocalPlayer() ) {
		// don't do any smoothing with this snapshot
		predictedFrame = gameLocal.framenum;
		// this is where the client updates their spectated player
		if ( gameLocal.gameType == GAME_TOURNEY ) {
			rvTourneyArena& arena = ((rvTourneyGameState*)gameLocal.mpGame.GetGameState())->GetArena( GetArena() );

			if( arena.GetPlayers()[ 0 ] == NULL || arena.GetPlayers()[ 1 ] == NULL || (spectator != arena.GetPlayers()[ 0 ]->entityNumber && spectator != arena.GetPlayers()[ 1 ]->entityNumber) ) {
				gameLocal.mpGame.tourneyGUI.ArenaSelect( GetArena(), TGH_BRACKET );
			} else if( spectator == arena.GetPlayers()[ 0 ]->entityNumber ) {
				gameLocal.mpGame.tourneyGUI.ArenaSelect( GetArena(), TGH_PLAYER_ONE );
			} else if( spectator == arena.GetPlayers()[ 1 ]->entityNumber ) {
				gameLocal.mpGame.tourneyGUI.ArenaSelect( GetArena(), TGH_PLAYER_TWO );
			}

			gameLocal.mpGame.tourneyGUI.UpdateScores();
		}

		if ( gameLocal.entities[ spectator ] ) {
			idPlayer *p = static_cast< idPlayer * >( gameLocal.entities[ spectator ] );
			p->UpdateHudWeapon( p->currentWeapon );
			if ( p->weapon ) {
				p->weapon->SpectatorCycle();
			}
		}
	}
 	newHitToggle = msg.ReadBits( 1 ) != 0;
	newHitArmor = msg.ReadBits( 1 ) != 0;
 	weaponGone = msg.ReadBits( 1 ) != 0;
 	isLagged = msg.ReadBits( 1 ) != 0;
 	isChatting = msg.ReadBits( 1 ) != 0;
	connectTime = msg.ReadLong();
	lastKillerEntity = msg.ReadByte();
	if( lastKillerEntity >= 0 && lastKillerEntity < MAX_CLIENTS)	{
		lastKiller = static_cast<idPlayer *>(gameLocal.entities[ lastKillerEntity ]);
	} else {
		lastKiller = NULL;
	}
	
	if ( idealWeapon != newIdealWeapon ) {
		if ( stateHitch ) {
			weaponCatchup = true;
		}
		idealWeapon = newIdealWeapon;
		StopFiring();
		UpdateHudWeapon();
		usercmd.buttons &= (~BUTTON_ATTACK);		
	}

	// Attach the world and view entities  
	weaponWorldModel.SetSpawnId( weaponWorldSpawnId );
	if ( weaponWorldModel.IsValid() && weaponViewModel.SetSpawnId( weaponSpawnId ) ) {
		currentWeapon = -1;
		SetWeapon( idealWeapon );
	}

	// rjohnson: instance persistance information
	if ( weaponWorldModel.IsValid() ) {
		weaponWorldModel->fl.persistAcrossInstances = true;
		weaponWorldModel->SetInstance( GetInstance() );
	}
	if ( weaponViewModel.IsValid() ) {
		weaponViewModel->fl.persistAcrossInstances = true;
		weaponViewModel->SetInstance( GetInstance() );
	}

	// If we have a weapon then update it from the snapshot, otherwise
	// we just skip whatever it would have read if it were there
	if ( msg.ReadBits( 1 ) ) {
		if ( weapon ) {
			weapon->ReadFromSnapshot( msg );
		} else {
			rvWeapon::SkipFromSnapshot( msg );
		}
	}
	if ( proto69 ) {
		inBuyZone = false;
		buyMenuCash = 0.0f;
	} else {
//RITUAL BEGIN
		inBuyZone = msg.ReadBits( 1 ) != 0;
		int cash = msg.ReadLong();
		if ( cash != (int)buyMenuCash ) {
			buyMenuCash = (float)cash;
			gameLocal.mpGame.RedrawLocalBuyMenu();
		}
//RITUAL END
	}
 	// no msg reading below this
	
	// if not a local client assume the client has all ammo types
	if ( entityNumber != gameLocal.localClientNum ) {
		for( i = 0; i < MAX_AMMO; i++ ) {
			inventory.ammo[ i ] = 999;
		}
	}

	if ( oldHealth > 0 && health <= 0 ) {
 		if ( stateHitch ) {
 			// so we just hide and don't show a death skin
 			UpdateDeathSkin( true );
 		}
		// die
		pfl.dead = true;
		ClearPowerUps();
		SetAnimState( ANIMCHANNEL_LEGS, "Legs_Dead", 4 );
		SetAnimState( ANIMCHANNEL_TORSO, "Torso_Dead", 4 );
		animator.ClearAllJoints();
		StartRagdoll();
		physicsObj.SetMovementType( PM_DEAD );

		if ( !stateHitch ) {
 			StartSound( "snd_death", SND_CHANNEL_VOICE, 0, false, NULL );
 		}
		
		const idDeclEntityDef* def = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex ( DECL_ENTITYDEF, lastDamageDef ));
		if ( def ) {		
			// TODO: get attackers push scale?
			InitDeathPush ( lastDamageDir, lastDamageLocation, &def->dict, 1.0f );
			ClientDamageEffects ( def->dict, lastDamageDir, ( oldHealth - health ) * 4 );
		}

		//gib them here
		if ( health < -20 || ( lastKiller && lastKiller->PowerUpActive( POWERUP_QUADDAMAGE )) )	{	
			ClientGib( lastDamageDir );
		}		

		if ( weapon ) {
			weapon->OwnerDied();

			// Get rid of the weapon now
			delete weapon;
			weapon = NULL;
			currentWeapon = -1;			
		}
	} else if ( oldHealth <= 0 && health > 0 ) {
 		// respawn
		//common->DPrintf( "idPlayer::ReadFromSnapshot() - Player respawn detected for %d '%s' - re-enabling clip\n", entityNumber, GetUserInfo()->GetString( "ui_name" ) );

		// this is the first time we've seen the player since we heard he died - he may have picked up
		// some powerups since he actually spawned in, so restore those
		int latchPowerup = inventory.powerups;
		Init();
		inventory.powerups = latchPowerup;
		StopRagdoll();
		SetPhysics( &physicsObj );
		physicsObj.EnableClip();
		SetCombatContents( true );
	} else if ( oldHealth - health > 2 && health > 0 ) {
 		if ( stateHitch ) {
			lastDmgTime = gameLocal.time;
   		} else {
 			// damage feedback
 			const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>( declManager->DeclByIndex( DECL_ENTITYDEF, lastDamageDef, false ) );
 			if ( def ) {
 				ClientDamageEffects ( def->dict, lastDamageDir, oldHealth - health );
 				pfl.pain = Pain( NULL, NULL, oldHealth - health, lastDamageDir, lastDamageLocation );
 				lastDmgTime = gameLocal.time;
 			} else {
 				common->Warning( "NET: no damage def for damage feedback '%d'\n", lastDamageDef );
 			}
		}
	}

 	if ( lastHitToggle != newHitToggle ) {
		SetLastHitTime( gameLocal.realClientTime, newHitArmor );
	}
	
	if ( msg.HasChanged() ) {
		UpdateVisuals();
	}

	/*if ( (head == NULL || headSpawnId != head.GetSpawnId()) && headSpawnId > 0 ) {
		head.SetSpawnId( headSpawnId );
		SetupHead();
	}*/
}

/*
================
idPlayer::WritePlayerStateToSnapshot
================
*/
void idPlayer::WritePlayerStateToSnapshot( idBitMsgDelta &msg ) const {
	int i;

	msg.WriteDeltaByte( 0, bobCycle );
	msg.WriteDeltaLong( 0, stepUpTime );
	msg.WriteDeltaFloat( 0.0f, stepUpDelta );

	msg.WriteShort( inventory.weapons );
	msg.WriteByte( inventory.armor );
	msg.WriteShort( inventory.powerups );

	for( i = 0; i < MAX_AMMO; i++ ) {
		msg.WriteBits( inventory.ammo[i], ASYNC_PLAYER_INV_AMMO_BITS );
	}

	for( i = 0; i < POWERUP_MAX; i ++ ) {
		msg.WriteLong( inventory.powerupEndTime[ i ] );
	}
}

/*
================
idPlayer::ReadPlayerStateFromSnapshot
================
*/
void idPlayer::ReadPlayerStateFromSnapshot( const idBitMsgDelta &msg ) {
	int i, ammo;

	bobCycle = msg.ReadDeltaByte( 0 );
	stepUpTime = msg.ReadDeltaLong( 0 );
	stepUpDelta = msg.ReadDeltaFloat( 0.0f );

	inventory.weapons = msg.ReadShort();
	inventory.armor = msg.ReadByte();
	inventory.powerups = msg.ReadShort();

	for( i = 0; i < MAX_AMMO; i++ ) {
 		ammo = msg.ReadBits( ASYNC_PLAYER_INV_AMMO_BITS );
 		if ( gameLocal.time >= inventory.ammoPredictTime ) {
 			inventory.ammo[ i ] = ammo;
 		}
	}

	int powerup_max = gameLocal.GetCurrentDemoProtocol() == 69 ? 11 : POWERUP_MAX;
	for ( i = 0; i < powerup_max; i ++ ) {
		inventory.powerupEndTime[ i ] = msg.ReadLong();
	}
	while ( i < POWERUP_MAX ) {
		inventory.powerupEndTime[ i ] = 0;
		i++;
	}

	if ( gameLocal.IsMultiplayer() ) {
		if ( (inventory.weapons&~oldInventoryWeapons)  ) {
			//added a weapon from inventory, bring up bar
			UpdateHudWeapon();
		}
		oldInventoryWeapons = inventory.weapons;
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
			PerformImpulse( msg.ReadBits( IMPULSE_NUMBER_OF_BITS ) );
			return true;
		}
		case EVENT_EMOTE: {
			// forward the emote on to all clients except the one that sent it to us
			ServerSendInstanceEvent( EVENT_EMOTE, &msg, false, entityNumber );

			// Set the emote locally
			SetEmote( (playerEmote_t)msg.ReadByte() );
			
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
 			start = ( msg.ReadBits( 1 ) != 0 );
 			if ( start ) {
				bool team = false;
				if ( gameLocal.GetCurrentDemoProtocol() != 69 ) {
					team = ( msg.ReadBits( 1 ) != 0 );
				}
 				GivePowerUp( powerup, 0, team );
 			} else {
 				ClearPowerup( powerup );
 			}
 
 			return true;
 		}
 		case EVENT_SPECTATE: {
 			bool spectate = ( msg.ReadBits( 1 ) != 0 );
			// force to spectator if we got this event about a client in a different
			// instance
 			Spectate( spectate );

			// spectate might re-link clip for stale players, so re-call ClientStale if we're stale
			if( fl.networkStale ) {
				ClientStale();	
			}
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
		case EVENT_EMOTE: {
			// Set the emote locally
			SetEmote( (playerEmote_t)msg.ReadByte() );

			return true;
		}
 		default: {
			return idActor::ClientReceiveEvent( event, time, msg );
		}
	}
//unreachable
//	return false;
}

/*
================
idPlayer::Hide
================
*/
void idPlayer::Hide( void ) {
	idActor::Hide();
	
	if ( weapon ) {
		weapon->HideWorldModel( );
	}
}

/*
================
idPlayer::Show
================
*/
void idPlayer::Show( void ) {
	idActor::Show();

	if ( weapon ) {
		weapon->ShowWorldModel( );
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
	objectiveSystem->HandleNamedEvent( obj );
	objectiveUp = true;
}


/*
===============
idPlayer::HideObjective
===============
*/
void idPlayer::HideObjective( void ) {
	objectiveSystem->HandleNamedEvent( "closeObjective" );
	objectiveUp = false;
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
================
idPlayer::ToggleFlashlight
================
*/
void idPlayer::ToggleFlashlight ( void ) {
	// Dead people can use flashlights
// RAVEN BEGIN
// mekberg: check to see if the weapon is enabled.
	if ( health <= 0 || !weaponEnabled ) {
		return;
	}
// RAVEN END

	int flashlightWeapon = currentWeapon;
	if ( !spawnArgs.GetBool( va( "weapon%d_flashlight", flashlightWeapon ) ) ) {
		// TODO: find the first flashlight weapon that has ammo starting at the bottom
		for( flashlightWeapon = MAX_WEAPONS - 1; flashlightWeapon >= 0; flashlightWeapon-- ) {
			if ( inventory.weapons & ( 1 << flashlightWeapon ) ) {
				const char *weap = spawnArgs.GetString( va( "def_weapon%d", flashlightWeapon ) );
				int			ammo = inventory.ammo[inventory.AmmoIndexForWeaponClass ( weap ) ];

				if ( !ammo ) {
					continue;
				}

				if ( spawnArgs.GetBool ( va ( "weapon%d_flashlight", flashlightWeapon ) ) ) {
					break;
				}
			}
		}
		
		// Couldnt find flashlight
		if ( flashlightWeapon < 0 ) {
			return;
		}
	}

	// If the current weapon isnt the flashlight then always force the flashlight on
	if ( flashlightWeapon != idealWeapon ) {
		flashlightOn = true;
		idealWeapon = flashlightWeapon;
	// Inform the weapon to toggle the flashlight, this will eventually cause the players
	// Flashlight method to be called 
	} else if ( weapon ) {
		weapon->Flashlight ( );
	}
}

/*
================
idPlayer::Flashlight
================
*/
void idPlayer::Flashlight ( bool on ) {
	flashlightOn = on;	
}

/*
================
idPlayer::DamageFeedback
================
*/
void idPlayer::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	
	//rvTramCars weren't built on the idActor inheritance hierarchy but need to be treated like one when shot.
	//TODO: Maybe add a key to entity flags that will allow them to be shot as actors even if they aren't actors?
	if( !victim || ( !victim->IsType( idActor::GetClassType() ) && !victim->IsType( rvTramCar::GetClassType() ) ) || victim->health <= 0 ) {
		return;
	}

	bool armorHit = false;

	if( gameLocal.isMultiplayer && victim->IsType( idPlayer::GetClassType() ) ) {
		if( this == victim ) {
			// no feedback for self hits
			return;
		}
		if( gameLocal.IsTeamGame() && ((idPlayer*)victim)->team == team ) {
			// no feedback for team hits
			return;
		}
		if( ((idPlayer*)victim)->inventory.armor > 0 ) {
			armorHit = true;
		}
	} 

	SetLastHitTime( gameLocal.time, armorHit );
}

/*
==============
idPlayer::GetWeaponDef
==============
*/
const idDeclEntityDef* idPlayer::GetWeaponDef ( int weaponIndex ) {
	if ( cachedWeaponDefs[weaponIndex] ) {
		return cachedWeaponDefs[weaponIndex];
	}

	idStr weapon;
	weapon = spawnArgs.GetString ( va("def_weapon%d", weaponIndex ) );
	if ( !weapon.Length() ) {
		return NULL;
	}
		
	cachedWeaponDefs[weaponIndex] = gameLocal.FindEntityDef ( weapon, false );
	if ( !cachedWeaponDefs[weaponIndex] ) {
		gameLocal.Error( "Could not find weapon definition '%s'", weapon.c_str() );
	}	
	
	return cachedWeaponDefs[weaponIndex];
}

/*
==============
idPlayer::GetPowerupDef

Returns the powerup dictionary for the given powerup index.  The dictionary is cached to ensure a
speedy retrieval after the first call.
==============
*/
const idDeclEntityDef* idPlayer::GetPowerupDef ( int powerupIndex ) {
	const idDict* types;
	int			  i;
	int			  num;
	
	if ( cachedPowerupDefs[powerupIndex] ) {
		return cachedPowerupDefs[powerupIndex];
	}
	
	types = gameLocal.FindEntityDefDict( "powerup_types", false );
	if ( !types ) {
		gameLocal.Error( "Could not find entity definition for 'powerup_types'" );
	}
	
	num = types->GetNumKeyVals();
	for( i = 0; i < num; i++ ) {
		const idKeyValue* kv;
		kv = types->GetKeyVal( i );
		if ( atoi(kv->GetValue()) == powerupIndex ) {
			cachedPowerupDefs[powerupIndex] = gameLocal.FindEntityDef ( kv->GetKey(), false );
			if ( !cachedPowerupDefs[powerupIndex] ) {
				gameLocal.Error( "Could not find powerup definition '%s'", kv->GetKey().c_str() );
			}
			return cachedPowerupDefs[powerupIndex];
		}
	}	
	
	gameLocal.Error( "Could not find powerup definition '%d'", powerupIndex );
	
	return NULL;
}

/*
==============
idPlayer::discoverSecretArea

Announces a secret area and increases the secret area tally
==============
*/
void idPlayer::DiscoverSecretArea(const char* _description)	{

	//increment the secret area tally
	inventory.secretAreasDiscovered++;
}

/*
==============
idPlayer::StartBossBattle

Starts a boss battle with the given entity.  During a boss battle the health of the boss 
will be displayed on the HUD
==============
*/
void idPlayer::StartBossBattle ( idEntity* enemy ) {
	bossEnemy = enemy;
	idUserInterface *hud_ = GetHud();
	if ( hud_ ) {
		hud_->SetStateInt ( "boss_maxhealth", enemy->health );
		hud_->HandleNamedEvent ( "showBossBar" );
	}
}

/*
=====================
idPlayer::SetInitialHud
=====================
*/
void idPlayer::SetInitialHud ( void ) {
	if ( !mphud || !gameLocal.isMultiplayer || gameLocal.GetLocalPlayer() != this ) {
		return;
	}

	mphud->SetStateInt( "gametype", gameLocal.gameType );

	if( hud ) {
		hud->SetStateInt( "gametype", gameLocal.gameType );
	}

	mphud->HandleNamedEvent( "InitHud" );
	mphud->HandleNamedEvent( "TeamChange" );
	
	if( gameLocal.IsFlagGameType() ) {
		mphud->SetStateFloat( "ap", gameLocal.mpGame.assaultPoints.Num() );

		for( int i = 0; i < TEAM_MAX; i++ ) {
			mphud->SetStateInt( "team", i );
			if( ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( i ) == FS_DROPPED ) {
				mphud->HandleNamedEvent( "flagDrop" );
			} else if( ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( i ) == FS_TAKEN ) {
				mphud->HandleNamedEvent( "flagTaken" );	
			} else if( ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( i ) == FS_TAKEN_MARINE ) {
				mphud->SetStateInt( "team", TEAM_MARINE );
				mphud->HandleNamedEvent( "flagTaken" );	
			} else if( ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( i ) == FS_TAKEN_STROGG ) {
				mphud->SetStateInt( "team", TEAM_STROGG );
				mphud->HandleNamedEvent( "flagTaken" );	
			} else if( ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetFlagState( i ) == FS_AT_BASE ) {
				mphud->SetStateInt( "team", i );
				mphud->HandleNamedEvent( "flagReturn" );
			}
		}

		for( int i = 0; i < gameLocal.mpGame.assaultPoints.Num(); i++ ) {
			mphud->SetStateFloat( "apindex", i );
			//mphud->SetStateInt( "apteam", ((rvCTFGameState*)gameLocal.mpGame.GetGameState())->GetAPOwner( i ) );
			mphud->StateChanged( gameLocal.time );
			mphud->HandleNamedEvent( "APCaptured" );
		}
	}

	mphud->StateChanged ( gameLocal.time );
}

void idPlayer::RemoveClientModel ( const char *entityDefName ) {
	rvClientEntity* cent;
	rvClientEntity*	next;
	
	for( cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( cent->IsType ( rvClientModel::GetClassType() ) ) {
// RAVEN END
			if ( !idStr::Icmp ( ( static_cast<rvClientModel*> ( cent ) )->GetClassname(), entityDefName ) ) {
				cent->Unbind ( );
				delete cent;
			}
		}
	}		
}

rvClientEntityPtr<rvClientModel> idPlayer::AddClientModel ( const char* entityDefName, const char* shaderName ) {
	rvClientEntityPtr<rvClientModel> ptr;
	ptr = NULL;
	
	if ( entityDefName == NULL ) {
		return ptr;
	}
	
	const idDict* entityDef = gameLocal.FindEntityDefDict ( entityDefName, false );
	
	if ( entityDef == NULL ) {
		return ptr;
	}

	rvClientModel *newModel = NULL;

	gameLocal.SpawnClientEntityDef( *entityDef, (rvClientEntity**)(&newModel), false, "rvClientModel" );
	
	if( newModel == NULL ) {
		return ptr;
	}
	idMat3 rotation;
	rotation = entityDef->GetAngles( "angles" ).ToMat3();
	newModel->SetAxis( rotation );

	newModel->SetOrigin( entityDef->GetVector( "origin" ) * rotation );
	
	newModel->Bind ( this, animator.GetJointHandle( entityDef->GetString ( "joint" ) ) );

	newModel->SetCustomShader ( shaderName );
	newModel->GetRenderEntity()->suppressSurfaceInViewID = entityNumber + 1;
	newModel->GetRenderEntity()->noSelfShadow = true;
	newModel->GetRenderEntity()->noShadow = true;
	
	ptr = newModel;

	return ptr;
}

void idPlayer::RemoveClientModels ( void ) {
	rvClientEntity* cent;
	rvClientEntity*	next;
	
	for( cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( cent->IsType ( rvClientModel::GetClassType() ) ) {
// RAVEN END
			cent->Unbind ( );
			delete cent;
		}
	}	
}

/*
=====================
idPlayer::ClientGib()
ddynerman: Spawns client side gibs around this player
=====================
*/
void idPlayer::ClientGib( const idVec3& dir ) {
	
	if( !spawnArgs.GetBool( "gib" )	)	{
		return;
	}

	int i;
	idVec3 entityCenter, velocity;
	idList<rvClientMoveable *> list;

		
	// hide the player
	SetSkin( gibSkin );

	//and the head
	if( gameLocal.isMultiplayer ) {
		if( clientHead ) {
			clientHead->UnlinkCombat();
			delete clientHead;
		}
	} else {
		if ( head.GetEntity() ) {
			head.GetEntity()->Hide();
		}
	}

	// blow out the gibs in the given direction away from the center of the entity
	

	// spawn gib client models
	rvClientMoveable::SpawnClientMoveables( this, "clientgib", &list );

	entityCenter = GetPhysics()->GetAbsBounds().GetCenter();
	for ( i = 0; i < list.Num(); i++ ) {
		list[i]->GetPhysics()->SetContents( CONTENTS_CORPSE );
		// we don't want collision on gibs
		//list[i]->GetPhysics()->SetClipMask( CONTENTS_SOLID );
		velocity = list[i]->GetPhysics()->GetAbsBounds().GetCenter() - entityCenter;
		velocity.NormalizeFast();
		velocity += ( i & 1 ) ? dir : -dir;
		
		list[i]->GetPhysics()->ApplyImpulse( 0, list[i]->GetPhysics()->GetOrigin(), velocity * ( 25000.0f + ( gameLocal.random.RandomFloat() * 50000.0f)) );
//		list[i]->GetPhysics()->SetLinearVelocity( velocity * ( 25.0f + ( gameLocal.random.RandomFloat() * 300.0f)));
		list[i]->GetPhysics()->SetAngularVelocity( velocity * ( -250.0f + ( gameLocal.random.RandomFloat() * 500.0f)));

		list[i]->GetRenderEntity()->noShadow = true;
		list[i]->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;

		list[i]->PostEventMS( &CL_FadeOut, SEC2MS( 4.0f ), SEC2MS( 2.0f ) );
	}
	

	//play gib fx
	gameLocal.PlayEffect( spawnArgs, "fx_gib", GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	// gibs are PVS agnostic.  If we gib a player outside of our PVS, set the oldHealth
	// to below 0 so when this player re-appears in our snap we respawn him
	if( gameLocal.isClient && gameLocal.GetLocalPlayer() && health > 0 ) { 
		health = -100;
	}
}

/*
=====================
idPlayer::CanDamage
=====================
*/
bool idPlayer::CanDamage( const idVec3 &origin, idVec3 &damagePoint, idEntity *ignoreEnt ) {
	if( gameLocal.isMultiplayer && health <= 0 ) {
		return false;
	}

	return idActor::CanDamage( origin, damagePoint, ignoreEnt );
}

/*
=====================
idPlayer::ClientDamageEffects

=====================
*/
void idPlayer::ClientDamageEffects ( const idDict& damageDef, const idVec3& dir, int damage ) {
	idVec3 from;
	idVec3 localDir;
	float  fadeDB;

	// Only necessary on clients
	if ( gameLocal.isMultiplayer && !gameLocal.isClient && !gameLocal.isListenServer ) {
		return;
	}
	
	from = dir;
	from.Normalize();
	viewAxis.ProjectVector( from, localDir );
	
	if ( damage ) {
// RAVEN BEGIN
// jnewquist: Controller rumble
		idPlayer *p = gameLocal.GetLocalPlayer();

		if ( p && ( p == this || ( p->spectating && p->spectator == entityNumber ) ) ) {
			playerView.DamageImpulse( localDir, &damageDef, damage );
		}
// RAVEN END
	}

	// Visual effects
	if ( health > 0 && damage ) {	
		// Let the hud know about the hit
		if ( hud ) {
			hud->SetStateFloat ( "hitdir", localDir.ToAngles()[YAW] + 180.0f );
			hud->HandleNamedEvent ( "playerHit" );
		}
 	}

	// Sound effects	
	if ( damageDef.GetFloat ( "hl_volumeDB", "-40", fadeDB ) ) {
		float fadeTime;
		
		fadeTime = 0.0f;
		if ( !pfl.hearingLoss ) {
			const char* fade;

			fadeTime = damageDef.GetFloat ( "hl_fadeOutTime", ".25" );
			soundSystem->FadeSoundClasses( SOUNDWORLD_GAME, 0, fadeDB, fadeTime );
		
			pfl.hearingLoss = true;

			// sound overlayed?		
			if ( damageDef.GetString ( "snd_hl", "", &fade ) && *fade ) {			
				StartSoundShader ( declManager->FindSound ( fade ), SND_CHANNEL_DEMONIC, 0, false, NULL );
			}
		}
		
		fadeTime += damageDef.GetFloat ( "hl_time", "1" );

		CancelEvents ( &EV_Player_FinishHearingLoss );
		PostEventSec ( &EV_Player_FinishHearingLoss, fadeTime, damageDef.GetFloat ( "hl_fadeInTime", ".25" ) );
	}
}

/*
=====================
idPlayer::GetDebugInfo
=====================
*/
void idPlayer::GetDebugInfo ( debugInfoProc_t proc, void* userData ) {
	idActor::GetDebugInfo ( proc, userData );
	proc ( "idPlayer", "inventory.armor",		va("%d", inventory.armor ), userData );
	proc ( "idPlayer", "inventory.weapons",		va("%d", inventory.weapons ), userData );
	proc ( "idPlayer", "inventory.powerups",	va("%d", inventory.powerups ), userData );
}



// RAVEN END

/*
=====================
idPlayer::ApplyImpulse
=====================
*/
void idPlayer::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse, bool splash ) {
	if( !ent)	{
		gameLocal.Warning( "idPlayer::ApplyImpulse called with null entity as instigator.");
		return;
	}

	lastImpulsePlayer = NULL;
	lastImpulseTime = gameLocal.time + 1000;

	if( ent->IsType( idPlayer::Type ) && ent != this ) {
		lastImpulsePlayer = static_cast<idPlayer*>(ent);
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	} else if( ent->IsType( idProjectile::GetClassType() ) ) {
// RAVEN END
		idEntity* owner = static_cast<idProjectile*>(ent)->GetOwner();
		if( owner && owner->IsType( idPlayer::Type ) && owner != this ) {
			lastImpulsePlayer = static_cast<idPlayer*>(owner);
		}
	}

	idAFEntity_Base::ApplyImpulse( ent, id, point, impulse, splash );
}

/*
=====================
idPlayer::SetupHead
=====================
*/
void idPlayer::SetupHead( const char* headModel, idVec3 headOffset ) {
	if( gameLocal.isMultiplayer ) {
		// player's don't use idActor's real head entities - uses clientEntities instead
		if( clientHead.GetEntity() ) {
			delete clientHead.GetEntity();
			clientHead = NULL;
		}


		if( spectating || (gameLocal.GetLocalPlayer() && instance != gameLocal.GetLocalPlayer()->GetInstance()) ) {
			return;
		}

		const idDict* headDict = gameLocal.FindEntityDefDict( headModel, false );
		if ( !headDict ) {
			return;
		}

		rvClientAFAttachment* headEnt = clientHead.GetEntity();
		gameLocal.SpawnClientEntityDef( *headDict, (rvClientEntity**)&headEnt, false );
		if( headEnt ) {
			idStr jointName = spawnArgs.GetString( "joint_head" );
			jointHandle_t joint = animator.GetJointHandle( jointName );
			if ( joint == INVALID_JOINT ) {
				return;
			}

			headEnt->SetBody ( this, headDict->GetString ( "model" ), joint );

			headEnt->SetOrigin( vec3_origin );		
			headEnt->SetAxis( mat3_identity );		
			headEnt->Bind( this, joint, true );
			headEnt->InitCopyJoints();

			// Spawn might have parsed a skin from the spawnargs, save it for future use here
			headSkin = headEnt->GetRenderEntity()->customSkin;
			clientHead = headEnt;
		}
	} else {
		idActor::SetupHead( headModel, headOffset );

		if ( head ) {
			head->fl.persistAcrossInstances = true;
		}
	}
}

/*
=====================
idPlayer::GUIMainNotice
=====================
*/
void idPlayer::GUIMainNotice( const char* message, bool persist ) {
	if( !gameLocal.isMultiplayer || !mphud ) {
		return;
	}
	
	mphud->SetStateString( "main_notice_text", message );
	mphud->SetStateBool( "main_notice_persist", persist );
	mphud->StateChanged( gameLocal.time );
	mphud->HandleNamedEvent( "main_notice" );
}

/*
=====================
idPlayer::GUIFragNotice
=====================
*/
void idPlayer::GUIFragNotice( const char* message, bool persist ) {
	if( !gameLocal.isMultiplayer || !mphud ) {
		return;
	}

	mphud->SetStateString( "frag_notice_text", message );
	mphud->SetStateBool( "frag_notice_persist", persist );
	mphud->StateChanged( gameLocal.time );
	mphud->HandleNamedEvent( "frag_notice" );
}

/*
=====================
idPlayer::SetHudOverlay
=====================
*/
void idPlayer::SetHudOverlay( idUserInterface* overlay, int duration ) {
	overlayHud = overlay;
	overlayHudTime = gameLocal.time + duration;
}

// RAVEN BEGIN
// mekberg: wrap saveMessages
/*
=====================
idPlayer::SaveMessage
=====================
*/
void idPlayer::SaveMessage( void ) {
#ifndef _XENON
	if ( GetHud( ) ) {
		GetHud()->HandleNamedEvent( "saveMessage" );
	}

	if ( objectiveSystem ) {
		objectiveSystem->HandleNamedEvent( "saveMessage" );
	}
#endif
}

// mekberg: set pm_ cvars
/*
=====================
idPlayer::SetPMCVars
=====================
*/
void idPlayer::SetPMCVars( void ) {
	const idKeyValue	*kv;

 	if ( !gameLocal.isMultiplayer || gameLocal.isServer ) {
 		kv = spawnArgs.MatchPrefix( "pm_", NULL );
 		while( kv ) {
 			cvarSystem->SetCVarString( kv->GetKey(), kv->GetValue() );
 			kv = spawnArgs.MatchPrefix( "pm_", kv );
 		}
	}
}
// RAVEN END

/*
=====================
idPlayer::GetSpawnClassname
=====================
*/
const char* idPlayer::GetSpawnClassname ( void ) {
	idEntity*	world;
	const char*	entityFilter;
	
	// Test player def
	if ( *g_testPlayer.GetString() ) {
		return g_testPlayer.GetString ( );
	}

	// Multiplayer
	if ( gameLocal.isMultiplayer ) {
		return "player_marine_mp";
	}
	
	// See if the world spawn specifies a player
	world = gameLocal.entities[ENTITYNUM_WORLD];
	assert( world );
	
	gameLocal.serverInfo.GetString( "si_entityFilter", "", &entityFilter );
	if ( entityFilter && *entityFilter ) {
		return world->spawnArgs.GetString( va("player_%s", entityFilter ), world->spawnArgs.GetString( "player", "player_marine" ) );
	}
	
	return world->spawnArgs.GetString( "player", "player_marine" );
}

/*
===============
idPlayer::SetInstance
===============
*/
void idPlayer::SetInstance( int newInstance ) {
	common->DPrintf( "idPlayer::SetInstance() - Setting instance for '%s' to %d\n", name.c_str(), newInstance );
	idEntity::SetInstance( newInstance );

	if( head.GetEntity() ) {
		head.GetEntity()->SetInstance( newInstance );
	}

	if( weapon ) {
		if( weapon->GetViewModel() ) {
			weapon->GetViewModel()->SetInstance( newInstance );
		}

		if( weapon->GetWorldModel() ) {
			weapon->GetWorldModel()->SetInstance( newInstance );
		}
	}

	if( weaponWorldModel ) {
		weaponWorldModel->SetInstance( newInstance );
	}
	
	if( weaponViewModel ) {
		weaponViewModel->SetInstance( newInstance );
	}

	// reschedule time announcements if needed
	if( this == gameLocal.GetLocalPlayer() ) {
		gameLocal.mpGame.ScheduleTimeAnnouncements();

		if( gameLocal.isServer ) {
			// remove/add heads on server
			for( int i = 0; i < MAX_CLIENTS; i++ ) {
				idPlayer* player = (idPlayer*)gameLocal.entities[ i ];
				if( player ) {
					if( player->instance != newInstance ) {
						if( player->clientHead.GetEntity() ) {
							delete player->clientHead;
							player->clientHead = NULL;
						}	
					} else {
						player->UpdateModelSetup( true );
					}
				}
			}
		}
	}
}

/*
===============
idPlayer::JoinInstance
===============
*/
void idPlayer::JoinInstance( int newInstance ) {
	assert( gameLocal.isServer );
	if( instance == newInstance ) {
		return;
	}

	if( newInstance < 0 || newInstance >= MAX_ARENAS ) {
		gameLocal.Warning( "idPlayer::JoinInstance() - Invalid instance %d specified\n", newInstance );
	}

	if( gameLocal.GetNumInstances() <= newInstance || gameLocal.GetInstance( newInstance ) == NULL ) {
		// don't populate instance until player gets linked into the right one	
		gameLocal.AddInstance( newInstance );

		if( this == gameLocal.GetLocalPlayer() ) {
			// ensure InstanceLeave() gets called on newly spawned entities before
			// InstanceJoin() gets called.
			gameLocal.mpGame.ServerSetInstance( instance );
		}
	}

	SetArena( newInstance );
	SetInstance( newInstance );

	gameLocal.GetInstance( newInstance )->JoinInstance( this );
}

/*
===============
idPlayer::SetEmote
===============
*/
void idPlayer::SetEmote( playerEmote_t newEmote ) {
	emote = newEmote;

	// if we're the ones generating the emote, pass it along
	if( gameLocal.localClientNum == entityNumber ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_EVENT_PARAM_SIZE];

		assert( entityNumber == gameLocal.localClientNum );
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteByte( emote );
		if( gameLocal.isServer ) {
			ServerSendInstanceEvent( EVENT_EMOTE, &msg, false, -1 );
		} else {
			ClientSendEvent( EVENT_EMOTE, &msg );
		}
	}
}

/*
===============
idPlayer::GetGroundElevator
===============
*/
idEntity* idPlayer::GetGroundElevator( idEntity* testElevator ) const {
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
	//NOTE: for player, don't care if all the way on, or not
	return groundEnt;
}

/*
===================
idPlayer::IsCrouching
===================
*/
bool idPlayer::IsCrouching( void ) const {
	return physicsObj.IsCrouching();
}

/*
===============
idPlayer::SetArena
===============
*/
void idPlayer::SetArena( int newArena ) {
	if( arena == newArena ) {
		return;
	}

	arena = newArena;
	if( gameLocal.GetLocalPlayer() == this && gameLocal.gameType == GAME_TOURNEY ) {
		if( arena >= 0 && arena <= MAX_ARENAS ) {
			if( arena < MAX_ARENAS ) {
// RAVEN BEGIN
// rhummer: localized these strings.
				GUIMainNotice( va( common->GetLocalizedString( "#str_107270" ), newArena + 1 ) );
			} else {
				GUIMainNotice( common->GetLocalizedString( "#str_107271" ) );
			}
// RAVEN END
			
			if( gameLocal.GetLocalPlayer() ) {
				gameLocal.mpGame.tourneyGUI.ArenaSelect( newArena, TGH_BRACKET );
			}
			gameLocal.mpGame.RemoveAnnouncerSoundRange( AS_TOURNEY_JOIN_ARENA_ONE, AS_TOURNEY_JOIN_ARENA_EIGHT );
			gameLocal.mpGame.ScheduleAnnouncerSound( (announcerSound_t)(AS_TOURNEY_JOIN_ARENA_ONE + arena), gameLocal.time );
		} 
	}
}

/*
===============
idPlayer::Event_DamageEffect
===============
*/
void idPlayer::Event_DamageEffect( const char *damageDefName, idEntity* _damageFromEnt )
{ 
	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef( damageDefName, false );
	if ( damageDef )
	{
		idVec3 dir = (_damageFromEnt!=NULL)?(GetEyePosition()-_damageFromEnt->GetEyePosition()):viewAxis[2];
		dir.Normalize();
		int		damage = 1;
		ClientDamageEffects( damageDef->dict, dir, damage );
		if ( !g_testDeath.GetBool() ) {
			lastDmgTime = gameLocal.time;
		}
		lastDamageDir = dir;
  		lastDamageDir.Normalize();
		lastDamageDef = damageDef->Index();
		lastDamageLocation = 0;
	}
}

/*
===============
idPlayer::UpdateDeathShader
===============
*/
void idPlayer::UpdateDeathShader ( bool state_hitch ) {
	if ( !doingDeathSkin && gameLocal.time > deathSkinTime && deathSkinTime ) {
		deathSkinTime = 0;

		deathClearContentsTime = spawnArgs.GetInt( "deathSkinTime" );
		doingDeathSkin = true;
		if ( state_hitch ) {
			renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f - 2.0f;

			if( gameLocal.isMultiplayer ) {
				if( clientHead ) {
					clientHead.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f - 2.0f;
					clientHead.GetEntity()->GetRenderEntity()->noShadow = true;
				}
			} else {
				if( head ) {
					head.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f - 2.0f;
					head.GetEntity()->GetRenderEntity()->noShadow = true;
				}
			}
		} else {
			renderEntity.shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
			if( gameLocal.isMultiplayer ) {
				if( clientHead ) {
					clientHead.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
					clientHead.GetEntity()->GetRenderEntity()->noShadow = true;
				}
			} else {
				if( head ) {
					head.GetEntity()->GetRenderEntity()->shaderParms[ SHADERPARM_TIME_OF_DEATH ] = gameLocal.time * 0.001f;
					head.GetEntity()->GetRenderEntity()->noShadow = true;
				}
			}
		}
		renderEntity.noShadow = true;
		UpdateVisuals();
	}
}

/*
===============
idPlayer::Event_InitWeapon
===============
*/
void idPlayer::InitWeapon( void ) {
	doInitWeapon = false;
	currentWeapon = -1;
	SetWeapon( idealWeapon );
}

/*
===============
idPlayer::GetHitscanTint
===============
*/
const idVec4& idPlayer::GetHitscanTint( void ) {
	if( gameLocal.IsTeamGame() ) {
		if( gameLocal.serverInfo.GetInt( "si_allowHitscanTint" ) >= 2 ) {
			if( team == TEAM_MARINE ) {
				return marineHitscanTint;
			} else if( team == TEAM_STROGG ) {
				return stroggHitscanTint;
			} else {
				gameLocal.Error( "idPlayer::GetHitscanTint() - Unknown team '%d' on player %d '%s'\n", team, entityNumber, GetUserInfo()->GetString( "ui_name" ) );
			}
		} else {
			return defaultHitscanTint;
		}
	}
	
	if( gameLocal.serverInfo.GetInt( "si_allowHitscanTint" ) >= 1 ) {
		return hitscanTint;    
	} 

	return defaultHitscanTint;
}

/*
===============
idPlayer::IsReady
===============
*/
bool idPlayer::IsReady( void ) {
	return !gameLocal.serverInfo.GetBool( "si_useReady" ) || ready || forcedReady;
}

/*
===============
idPlayer::ForceScoreboard
===============
*/
void idPlayer::ForceScoreboard( bool force, int time ) {
	forceScoreBoard = force;
	forceScoreBoardTime = time;
}

/*
===============
idPlayer::GetTextTourneyStatus
===============
*/
const char* idPlayer::GetTextTourneyStatus( void ) {
	if( tourneyStatus == PTS_ADVANCED ) {
		return common->GetLocalizedString( "#str_107740" );
	} else if( tourneyStatus == PTS_ELIMINATED ) {
		return common->GetLocalizedString( "#str_107729" );
	} else if( tourneyStatus == PTS_PLAYING ) {
		return common->GetLocalizedString( "#str_107728" );
	} else if( tourneyStatus == PTS_UNKNOWN ) {
		return common->GetLocalizedString( "#str_107739" );
	}
	return "UNKNOWN TOURNEY STATUS";
}

/*
===============
idPlayer::ClientInstanceJoin
Players know about all other players, even in other instances
We need to hide/show them on the client as we switch to/from instances
===============
*/
void idPlayer::ClientInstanceJoin( void ) {
	assert( gameLocal.isClient );

	common->DPrintf( "idPlayer::ClientInstanceJoin() - client %d ('%s') is being shown\n", entityNumber, GetUserInfo()->GetString( "ui_name" ) );

	// restore client
	Spectate( spectating, true );
}

/*
===============
idPlayer::ClientInstanceLeave
Players know about all other players, even in other instances
We need to hide/show them on the client as we switch to/from instances
===============
*/
void idPlayer::ClientInstanceLeave( void ) {
	assert( gameLocal.isClient );
	common->DPrintf( "idPlayer::ClientInstanceLeave() - client %d ('%s') is being hidden\n", entityNumber, GetUserInfo()->GetString( "ui_name" ) );

	// force client to spectate
	Spectate( spectating, true );
}

/*
===============
idPlayer::ClientStale
===============
*/
bool idPlayer::ClientStale( void ) {
	idEntity::ClientStale();
	
	// remove all powerup effects
	for( int i = 0; i < POWERUP_MAX; i++ ) {
		if( inventory.powerups & ( 1 << i ) ) {
			StopPowerUpEffect( i );
		}
	}


	if( clientHead ) {
		delete clientHead;
		clientHead = NULL;
	}

	// never delete client
	return false;
}

/*
===============
idPlayer::ClientUnstale
===============
*/
void idPlayer::ClientUnstale( void ) {
	idEntity::ClientUnstale();
	
	// force render ent to position
	renderEntity.axis = physicsObj.GetAxis();
	renderEntity.origin = physicsObj.GetOrigin();

	// don't do any smoothing with this snapshot
	predictedFrame = gameLocal.framenum;
	// the powerup effects ( rvClientEntity ) will do some bindings, which in turn will call GetPosition
	// which uses the predictedOrigin .. which won't be updated till we Think() so just don't leave the predictedOrigin to the old position
	predictedOrigin = renderEntity.origin;

	// restart powerup effects on clients that are coming back into our snapshot
	int i;
	for ( i = 0; i < POWERUP_MAX; i++ ) {
		if ( inventory.powerups & (1 << i) ) {
			StartPowerUpEffect( i );
		}
	}

	UpdateModelSetup( true );

	if ( weapon ) {
		weapon->ClientUnstale();
	}
}

/*
===============
idPlayer::AllowedVoiceDest
===============
*/
bool idPlayer::AllowedVoiceDest( int from ) {

	int		i, free;

	free = -1;
	for( i = 0; i < MAX_CONCURRENT_VOICES; i++ ) {

		if( voiceDest[i] == from ) {
			voiceDestTimes[i] = gameLocal.time;
			return true;
		}

		if( voiceDestTimes[i] + 200 < gameLocal.time ) {
			free = i;
		}
	}

	if( free > -1 ) {
		voiceDest[free] = from;
		voiceDestTimes[i] = gameLocal.time;
		return true;
	}

	return false;
}

// RITUAL BEGIN
void idPlayer::ClampCash( float minCash, float maxCash )
{
	if( buyMenuCash < minCash )
		buyMenuCash = minCash;

	if( buyMenuCash > maxCash )
		buyMenuCash = maxCash;
}

void idPlayer::GiveCash( float cashDeltaAmount )
{
	//int minCash = gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerMinCash", 0 );
	//int maxCash = gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerMaxCash", 0 );
	float minCash = (float) gameLocal.serverInfo.GetInt("si_buyModeMinCredits");
	float maxCash = (float) gameLocal.serverInfo.GetInt("si_buyModeMaxCredits");

	float oldCash = buyMenuCash;
	buyMenuCash += cashDeltaAmount;
	ClampCash( minCash, maxCash );

	if( (int)buyMenuCash != (int)oldCash )
	{
		gameLocal.mpGame.RedrawLocalBuyMenu();
	}

	if( (int)buyMenuCash > (int)oldCash )
	{
		// Play the "get cash" sound
//		gameLocal.GetLocalPlayer()->StartSound( "snd_buying_givecash", SND_CHANNEL_ANY, 0, false, NULL );
	}
	else if( (int)buyMenuCash < (int)oldCash )
	{
		// Play the "lose cash" sound
//		gameLocal.GetLocalPlayer()->StartSound( "snd_buying_givecash", SND_CHANNEL_ANY, 0, false, NULL );
	}
}

void idPlayer::SetCash( float newCashAmount )
{
	//int minCash = gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerMinCash", 0 );
	//int maxCash = gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerMaxCash", 0 );
	float minCash = (float) gameLocal.serverInfo.GetInt("si_buyModeMinCredits");
	float maxCash = (float) gameLocal.serverInfo.GetInt("si_buyModeMaxCredits");

	buyMenuCash = newCashAmount;
	ClampCash( minCash, maxCash );
}

void idPlayer::ResetCash()
{
	//int minCash = gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerMinCash", 0 );
	//int maxCash = gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerMaxCash", 0 );
	//buyMenuCash = gameLocal.mpGame.mpBuyingManager.GetIntValueForKey( "playerStartingCash", 0 );

	float minCash = (float) gameLocal.serverInfo.GetInt("si_buyModeMinCredits");
	float maxCash = (float) gameLocal.serverInfo.GetInt("si_buyModeMaxCredits");
	buyMenuCash = (float) gameLocal.serverInfo.GetInt("si_buyModeStartingCredits");
	ClampCash( minCash, maxCash );
}

/**
 * Checks to see if the player can accept this item in their inventory
 *
 * weaponName Name of the weapon.
 */
int idPlayer::CanSelectWeapon(const char* weaponName)
{
	int weaponNum = -1;
	if(weaponName == NULL)
		return weaponNum;

	for( int i = 0; i < MAX_WEAPONS; i++ ) {
		if ( inventory.weapons & ( 1 << i ) ) {
			const char *weap = spawnArgs.GetString( va( "def_weapon%d", i ) );
			if ( !idStr::Cmp( weap, weaponName ) ) {
				weaponNum = i;
				break;
			}
		}
	}

	return weaponNum;
}

// RITUAL END
