
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

void hhInventory::Clear() {
	memset( &requirements, 0, sizeof( requirements ) );
	maxSpirit = 0;
	bHasDeathwalked = false;
	storedHealth = 0;
	energyType = "energy_plasma";
	memset( altMode, 0, sizeof( altMode ) );
	memset( weaponRaised, 0, sizeof( weaponRaised ) );
	memset( lastShot, 0, sizeof( lastShot ) );		//HUMANHEAD bjk PATCH 7-27-06
	zoomFov = 0;

	idInventory::Clear();
}

void hhInventory::Save(idSaveGame *savefile) const {
	idInventory::Save( savefile );

	savefile->WriteBool(bHasDeathwalked);
	savefile->Write(&maxSpirit, sizeof(maxSpirit));
	savefile->Write(&requirements, sizeof(requirements));
	savefile->WriteInt( storedHealth );
	savefile->WriteString( energyType.c_str() );
	savefile->Write(&altMode, sizeof(altMode));
	savefile->Write(&weaponRaised, sizeof(weaponRaised));
	savefile->WriteInt( zoomFov );
}

void hhInventory::Restore( idRestoreGame *savefile ) {
	idInventory::Restore( savefile );

	savefile->ReadBool(bHasDeathwalked);
	savefile->Read(&maxSpirit, sizeof(maxSpirit));
	savefile->Read(&requirements, sizeof(requirements));
	savefile->ReadInt( storedHealth );
	savefile->ReadString( energyType );
	savefile->Read(&altMode, sizeof(altMode));
	savefile->Read(&weaponRaised, sizeof(weaponRaised));
	memset( lastShot, 0, sizeof( lastShot ) );		//HUMANHEAD bjk PATCH 7-27-06
	savefile->ReadInt( zoomFov );
}

/*
==============
hhInventory::GetPersistantData
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
==============
*/
void hhInventory::GetPersistantData( idDict &dict ) {
	int		i;
	int		num;
	idDict	*item;
	idStr	key;
	const idKeyValue *kv;
	const char *name;

    // don't bother with powerups or the clip

	// maxhealth, maxspirit
	dict.SetInt( "maxhealth", maxHealth);
	dict.SetInt( "max_ammo_spiritpower", maxSpirit);
	dict.SetBool( "bHasDeathwalked", bHasDeathwalked );
	dict.SetInt( "storedHealth", storedHealth );
	dict.Set( "energyType", energyType.c_str() );	//HUMANHEAD bjk
	dict.SetInt( "zoomFov", zoomFov );	//HUMANHEAD bjk

	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if ( name ) {
			dict.SetInt( name, ammo[ i ] );
		}
	}

	//HUMANHEAD bjk: weapons
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		sprintf( key, "altMode_%i", i );
		dict.SetBool( key, altMode[i] );
		sprintf( key, "weaponRaised_%i", i );
		dict.SetBool( key, weaponRaised[i] );
	}
	//HUMANHEAD END

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

			// HUMANHEAD CJR: copy all keys with "def_"
			// Needed for Hunter Hand GUI
			kv = item->MatchPrefix( "def_" );
			if ( kv ) {
				while( kv ) {
					sprintf( key, "item_%i %s", num, kv->GetKey().c_str() );
					dict.Set( key, kv->GetValue() );
					kv = item->MatchPrefix( "def_", kv );
				}
			} // HUMANHEAD END	

			// HUMANHEAD CJR: copy all keys with "passtogui_"
			// Needed for Hunter Hand GUI
			kv = item->MatchPrefix( "passtogui_" );
			if ( kv ) {
				while( kv ) {
					sprintf( key, "item_%i %s", num, kv->GetKey().c_str() );
					dict.Set( key, kv->GetValue() );
					kv = item->MatchPrefix( "passtogui_", kv );
				}
			} // HUMANHEAD END	

			num++;
		}
	}
	dict.SetInt( "items", num );

	// weapons
	dict.SetInt( "weapon_bits", weapons );
	
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
hhInventory::RestoreInventory
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
==============
*/
void hhInventory::RestoreInventory( idPlayer *owner, const idDict &dict ) {
	int			i;
	int			num;
	idDict		*item;
	idStr		key;
	idStr		itemname;
	const idKeyValue *kv;
	const char	*name;

	Clear();

	// health
	maxHealth		= dict.GetInt( "maxhealth", "100" );
	bHasDeathwalked = dict.GetBool( "bHasDeathwalked" );
	storedHealth	= dict.GetInt( "storedHealth", "0" );
	energyType		= dict.GetString( "energyType", "energy_plasma" );
	zoomFov			= dict.GetInt( "zoomFov" );

	// the clip and powerups aren't restored

	// max spirit
	maxSpirit		= dict.GetInt( "max_ammo_spiritpower" );

	// ammo
	for( i = 0; i < AMMO_NUMTYPES; i++ ) {
		name = idWeapon::GetAmmoNameForNum( ( ammo_t )i );
		if ( name ) {
			ammo[ i ] = dict.GetInt( name );
		}
	}

	//HUMANHEAD bjk: weapons
	for( i = 0; i < MAX_WEAPONS; i++ ) {
		sprintf( key, "altMode_%i", i );
		altMode[i] = dict.GetBool( key );
		sprintf( key, "weaponRaised_%i", i );
		weaponRaised[i] = dict.GetBool( key );
	}
	//HUMANHEAD END

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

	//HUMANHEAD aob: in addition to the persistent items, give hardcoded items from players.def
	Give( owner, dict, "item", dict.GetString( "item" ), NULL, true );
	//HUMANHEAD END

	// weapons are stored as a number for persistant data, but as strings in the entityDef
	weapons	= dict.GetInt( "weapon_bits", "0" );
	Give( owner, dict, "weapon", dict.GetString( "weapon" ), NULL, false );

	num = dict.GetInt( "levelTriggers" );
	for ( i = 0; i < num; i++ ) {
		sprintf( itemname, "levelTrigger_Level_%i", i );
		idLevelTriggerInfo lti;
		lti.levelName = dict.GetString( itemname );
		sprintf( itemname, "levelTrigger_Trigger_%i", i );
		lti.triggerName = dict.GetString( itemname );
		levelTriggers.Append( lti );
	}

	// Keep weapon switch HUD element from showing up at level load
	weaponPulse = false;
}

int hhInventory::MaxAmmoForAmmoClass( idPlayer *owner, const char *ammo_classname ) const {
	int max = 0;
	if (ammo_classname != NULL) {
		if (!idStr::Icmp(ammo_classname, "ammo_spiritpower")) {
			max = maxSpirit;
		}
		else {
			max = idInventory::MaxAmmoForAmmoClass(owner, ammo_classname);
		}
	}
	return max;
}

//	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
void hhInventory::AddPickupName( const char *name, const char *icon, bool bIsWeapon) {
	if ( idStr::Length(icon) > 0 ) {
		idItemInfo &info = pickupItemNames.Alloc();

		if ( !idStr::Icmpn( name, STRTABLE_ID, strlen( STRTABLE_ID ) ) ) {
			info.name = common->GetLanguageDict()->GetString( name );
		} else {
			info.name = name;
		}
		info.icon = icon;
		info.time = 0;
		info.slotZeroTime = 0;
		info.matcolorAlpha = 0.0f;
		info.bDoubleWide = bIsWeapon;
	}
}

/*
==============
hhInventory::Give
	PDMMERGE PERSISTENTMERGE: Overridden, Done for 6-03-05 merge
==============
*/
bool hhInventory::Give( idPlayer *owner, const idDict &spawnArgs, const char *statname, const char *value, int *idealWeapon, bool updateHud ) {
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
	hhPlayer*				playerOwner;


	if( owner && owner->IsType( hhPlayer::Type ) ) {
		playerOwner = static_cast<hhPlayer*>(owner);
	}

	if ( !idStr::Icmp( statname, "health" ) || !idStr::Icmp( statname, "healthspecial" ) ) { //healthspecial is for mp and indicates that this item can push a player's health up to the "real" maxhealth
		int localMaxHealth = maxHealth;
		if (gameLocal.isMultiplayer) { //rww - only pipes can put us above 100 in mp
			if (idStr::Icmp( statname, "healthspecial" )) {
				localMaxHealth = MAX_HEALTH_NORMAL_MP;
			}
		}
		if ( playerOwner->health >= localMaxHealth ) {
			return false;
		}
		int oldHealth = playerOwner->health;
		playerOwner->health += atoi( value );
		if ( playerOwner->health > localMaxHealth ) {
			playerOwner->health = localMaxHealth;
		}
		if (playerOwner) {
			playerOwner->healthPulse = true;
		}
	} else if ( !idStr::Icmpn( statname, "ammo_", 5 ) ) {
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
		}
		if (playerOwner && !idStr::Icmp(statname, "ammo_spiritpower")) {
			playerOwner->spiritPulse = true;
		}
	} else if ( !idStr::Icmp( statname, "item" ) ) {
		pos = value;
		while( pos != NULL ) {
			end = strchr( pos, ',' );
			if ( end ) {
				len = end - pos;
				end++;
			} else {
				len = strlen( pos );
			}
		
			idStr itemName( pos, 0, len );
			
			GiveItem( spawnArgs, gameLocal.FindEntityDefDict(itemName.c_str(), false) );
		
			pos = end;
		}
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
			for( i = 1; i < MAX_WEAPONS; i++ ) {
				if ( weaponName == playerOwner->GetWeaponName(i) ) {
					break;
				}
			}

			if ( i >= MAX_WEAPONS ) {
				gameLocal.Error( "Unknown weapon '%s'", weaponName.c_str() );
			}

			// cache the media for this weapon
			weaponDecl = gameLocal.FindEntityDef( weaponName, false );

			// don't pickup "no ammo" weapon types twice
			// not for D3 SP .. there is only one case in the game where you can get a no ammo
			// weapon when you might already have it, in that case it is more conistent to pick it up
			if ( gameLocal.isMultiplayer && weaponDecl && ( weapons & ( 1 << i ) ) && !weaponDecl->dict.GetInt( "ammoRequired" ) ) {
				continue;
			}

			if ( !gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) || ( weaponName == "weaponobj_fists" ) ) {
				playerOwner->UnlockWeapon( i ); //TODO add key for disabling this
				if ( ( weapons & ( 1 << i ) ) == 0 || gameLocal.isMultiplayer ) {
					if ( (owner->GetUserInfo()->GetBool( "ui_autoSwitch" ) || !gameLocal.isMultiplayer) && idealWeapon ) {
						// HUMANHEAD pdm: added spirit check, so we don't autoswitch to weapons when picking them up in spriitwalk
						// HUMANHEAD pdm: also added check for spiritweapon, don't autoswitch to it.
						if (!static_cast<hhPlayer*>(owner)->IsSpiritOrDeathwalking() && weaponName.Icmp("weaponobj_bow")) {
							assert( !gameLocal.isClient );
							*idealWeapon = i;
						}
					}

					// Pulse if not picking up the spirit bow
					if (weaponName.Icmp("weaponobj_bow") != 0) {
						weaponPulse = true;
					}
					weapons |= ( 1 << i );
					tookWeapon = true;
				}
			}
		}
		return tookWeapon;
	}
	else if ( !idStr::Icmp( statname, "maxhealth" ) ) {
		if (owner) {
			if (gameLocal.GetLocalPlayer() == owner) { //sp, listen server
				if (owner->hud) {
					owner->hud->HandleNamedEvent( "maxHealthPulse" );
				}
			}
			else if (gameLocal.isMultiplayer && !gameLocal.isClient) { //otherwise, broadcast event to owner
				idBitMsg	msg;
				byte		msgBuf[MAX_EVENT_PARAM_SIZE];

				msg.Init(msgBuf, sizeof(msgBuf));
				msg.WriteBits((1<<idPlayer::MENU_NET_EVENT_MAXHEALTHPULSE), idPlayer::MENU_NET_EVENT_NUM);
				owner->ServerSendEvent(idPlayer::EVENT_MENUEVENT, &msg, false, -1, owner->entityNumber);
			}
		}
		if (gameLocal.isMultiplayer) { //rww - different behaviour for mp
			maxHealth = atoi(value);
		}
		else {
			maxHealth += atoi(value);
		}
	}
	else if ( !idStr::Icmp( statname, "maxspirit" ) ) {
		owner->hud->HandleNamedEvent( "maxSpiritPulse" );
		maxSpirit += atoi(value);
	}
	else {
		// unknown item
		return false;
	}

	return true;
}

/*
==============
hhInventory::GiveItem
==============
*/
bool hhInventory::GiveItem( const idDict& spawnArgs, const idDict* item ) {
	idStr	itemName;

	if( !item ) {
		return false;
	}

	idDict* dict = new idDict( *item );

	if( !FindItem(item) ) {
		items.Append( dict );
		return true;
	}

	SAFE_DELETE_PTR( dict );
	return false;
}

/*
===============
hhInventory::HasAmmo
===============
*/
int hhInventory::HasAmmo( ammo_t type, int amount ) {
	assert(type >= 0 && type < AMMO_NUMTYPES);

	if ( ( type == idWeapon::GetAmmoNumForName("ammo_none") ) || !amount ) {
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
hhInventory::HasAmmo
===============
*/
int hhInventory::HasAmmo( const char *weapon_classname ) {
	int ammoRequired;
	ammo_t ammo_i = AmmoIndexForWeaponClass( weapon_classname, &ammoRequired );
	return HasAmmo( ammo_i, ammoRequired );
}

/*
===============
hhInventory::HasAltAmmo
HUMANHEAD bjk
===============
*/
int hhInventory::HasAltAmmo( const char *weapon_classname ) {
	int ammoRequired;
	ammo_t ammo_i = AltAmmoIndexForWeaponClass( weapon_classname, &ammoRequired );
	return HasAmmo( ammo_i, ammoRequired );
}

/*
===============
hhInventory::UseAmmo
===============
*/
bool hhInventory::UseAmmo( ammo_t type, int amount ) {
	if ( !HasAmmo( type, amount ) ) {
		return false;
	}

	// take an ammo away if not infinite
	if ( ammo[ type ] >= 0 ) {
		ammo[ type ] -= amount;
		//rww - don't forget this, it's important!
		ammoPredictTime = gameLocal.time; // mp client: we predict this. mark time so we're not confused by snapshots
	}

	return true;
}

float hhInventory::AmmoPercentage(idPlayer *player, ammo_t type) {
	float amount = ammo[type];
	const char *ammoName = idWeapon::GetAmmoNameForNum( type );
	float max = MaxAmmoForAmmoClass( player, ammoName );
	max = max(1, max);
	return amount / max;
}

/*
===============
hhInventory::FindInventoryItem
===============
*/
idDict* hhInventory::FindItem( const idDict* dict ) {
	if( !dict ) {
		return NULL;
	}

	return FindItem( dict->GetString("inv_name") );
}

/*
===============
hhInventory::FindInventoryItem
===============
*/
idDict* hhInventory::FindItem( const char *name ) {
	const char* lname = NULL;

	for( int ix = 0; ix < items.Num(); ++ix ) {
		if( !items[ix] ) {
			continue;
		}
	
		lname = items[ix]->GetString( "inv_name" );
		if ( lname && *lname ) {
			if ( idStr::Icmp( name, lname ) == 0 ) {
				return items[ix];
			}
		}
	}
	return NULL;
}

/*
===============
hhInventory::EvaluateRequirements
===============
*/
void hhInventory::EvaluateRequirements(idPlayer *p) {
	if (p) {
		requirements.bCanDeathWalk =	gameLocal.RequirementMet(p, p->spawnArgs.GetString("requirement_deathwalk"), 0);
		requirements.bCanSpiritWalk =	gameLocal.RequirementMet(p, p->spawnArgs.GetString("requirement_spiritwalk"), 0);
		requirements.bCanSummonTalon =	gameLocal.RequirementMet(p, p->spawnArgs.GetString("requirement_talon"), 0);
		requirements.bCanUseBowVision =	gameLocal.RequirementMet(p, p->spawnArgs.GetString("requirement_bowvision"), 0);
		requirements.bCanUseLighter =	gameLocal.RequirementMet(p, p->spawnArgs.GetString("requirement_lighter"), 0);
		requirements.bCanWallwalk =		gameLocal.RequirementMet(p, p->spawnArgs.GetString("requirement_wallwalk"), 0);
		requirements.bHunterHand =		gameLocal.RequirementMet(p, p->spawnArgs.GetString("requirement_hunterhand"), 0);

		// See if talon should be spawned
		if (p->IsType(hhPlayer::Type)) {
			static_cast<hhPlayer*>(p)->TrySpawnTalon();
		}
	}
}

