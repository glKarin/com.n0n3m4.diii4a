#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

/***********************************************************************

hhItemAutomatic

***********************************************************************/

CLASS_DECLARATION( idEntity, hhItemAutomatic )
END_CLASS


//=============================================================================
//
// hhItemAutomatic::Spawn
//
//=============================================================================
void hhItemAutomatic::Spawn() {
	GetPhysics()->SetContents( CONTENTS_TRIGGER );
	BecomeActive( TH_THINK );
}

//=============================================================================
//
// hhItemAutomatic::Think
//
//=============================================================================

void hhItemAutomatic::Think() {
	if ( thinkFlags & TH_THINK ) {  // Guard against the first level-load think
		// We only think when we are in the PVS.  We think once and remove ourselves.
		hhPlayer *player;
		player = static_cast< hhPlayer *>( gameLocal.GetLocalPlayer() );
		if ( player && player->CheckFOV( this->GetOrigin() ) ) {		
			SpawnItem();
		}
	}
}

//=============================================================================
//
// hhItemAutomatic::SpawnItem
//
//=============================================================================

void hhItemAutomatic::SpawnItem() {
	hhPlayer		*player;
	idDict			Args;
	idStr			itemName = NULL;
	idEntity		*spawned = NULL;

	// Remove this object and spawn in a new item in it's place
	PostEventMS( &EV_Remove, 0 );

	Args.SetBool( "spin", 0 );
	Args.SetFloat( "triggersize", 48.0f );
	Args.SetFloat( "respawn", 0.0f );
	Args.SetBool( "enablePickup", false );
	Args.SetFloat( "wander_radius", 12.0f ); // in case the item spawned is a crawler, don't let it wander very far
	Args.Set( "target", spawnArgs.GetString( "target" ) ); // Pass the target on to the spawned item

	player = static_cast<hhPlayer*>(gameLocal.GetLocalPlayer());

	itemName = GetNewItem();

	if ( itemName.IsEmpty() ) {
		return;
	}

	spawned = gameLocal.SpawnObject( itemName.c_str(), &Args );
	if ( spawned ) {
		spawned->SetOrigin( this->GetOrigin() );
		spawned->SetAxis( this->GetAxis() );
		
		if ( spawned->IsType( hhItem::Type ) ) {
			hhItem *item = static_cast<hhItem *>( spawned );
			item->EnablePickup();
		}
	}
}

//=============================================================================
//
// hhItemAutomatic::GetNewItem
//
//=============================================================================

idStr hhItemAutomatic::GetNewItem() {
	int				i;
	idStr			defaultName;
	float			skipPercent;
	int				numAmmo;
	
	idList<idStr>	weaponNames;
	idList<int>		weaponIndexes;
	idList<bool>	validWeapon;
	idList<float>	ammoPercent;
	idList<idStr>	ammoTypes;
	idList<idStr>	itemNames;
	idList<idStr>	skipPercentString;

	float			total;

	bool			bDontSkip = spawnArgs.GetBool( "bDontSkip", "0" );

	// This is the heart of the DDA system:
	// list all available weapons that can have ammo in the cabinet and how much ammo the player has

	hhPlayer *player = static_cast<hhPlayer *>(gameLocal.GetLocalPlayer());

	// Build lists of all valid weapon names, ammo types and ammo names
	// Note that all these must line up -- so the first weapon corresponds with the first ammo type and the first item name
	hhUtils::SplitString( idStr(spawnArgs.GetString( "weaponNames" )), weaponNames );
	hhUtils::SplitString( idStr(spawnArgs.GetString( "ammoTypes" )), ammoTypes );
	hhUtils::SplitString( idStr(spawnArgs.GetString( "itemNames" )), itemNames );
	hhUtils::SplitString( idStr(spawnArgs.GetString( "skipPercents" )), skipPercentString );

	numAmmo = weaponNames.Num();

	if ( skipPercentString.Num() != numAmmo ) {
		gameLocal.Error( "hhItemAutomatic::GetNewItem:  skipPercent.Num() != weaponNames.Num()\n" );
	}

	weaponIndexes.SetNum( numAmmo );
	validWeapon.SetNum( numAmmo );
	ammoPercent.SetNum( numAmmo );

	for( i = 0; i < weaponNames.Num(); i++ ) {
		weaponIndexes[i] = player->GetWeaponNum( weaponNames[i].c_str() );
	}	

	// compute percentages of each ammo compared to the max allowed
	for( i = 0; i < numAmmo; i++ ) {
		validWeapon[i] = false;
		if ( player->inventory.weapons & (1 << weaponIndexes[i] ) ) {
			int ammoIndex = player->inventory.AmmoIndexForAmmoClass( ammoTypes[i].c_str() );
			ammoPercent[i] = player->inventory.AmmoPercentage( player, ammoIndex );

			if ( !bDontSkip ) { // Facility to guarantee that ammo won't be skipped
				float adjustFactor = FindAmmoNearby( itemNames[i].c_str() ); // If similar ammo is nearby, then reduce the chance of this spawning that ammo
				ammoPercent[i] *= adjustFactor;

				skipPercent = idMath::ClampFloat( 0.0f, 1.0f, (float)atof( skipPercentString[i].c_str() ) );
				skipPercent *= (1.0f - (gameLocal.GetDDAValue() * 0.5f + 0.25f)); // CJR TEST:  Scale based upon DDA

				if ( ammoPercent[i] > skipPercent ) { // Skip this weapon if the player weapon percentage is high enough
					validWeapon[i] = false;
					continue;
				}
			}

			ammoPercent[i] = 1.0f - ammoPercent[i]; // Reverse it to make the math below a bit simpler

			if ( ammoPercent[i] == 0.0f ) {
				ammoPercent[i] = 0.1f; // Give full ammo at least a slight chance
			}

			validWeapon[i] = true;
		}
	}

	// re-compute the total of all percentages
	total = 0;
	for( i = 0; i < numAmmo; i++ ) {
		if ( validWeapon[i] ) {
			total += ammoPercent[i];
		}
	}

	// random number from 0 - total
	float random = gameLocal.random.RandomFloat() * total;

	// calculate which ammo that number is associated with and add that item to the cabinet
	for( i = 0; i < numAmmo; i++ ) {
		if ( validWeapon[i] ) {
			if ( random <= ammoPercent[i] ) { // This is the ammo we want
				return itemNames[i];
			}

			random -= ammoPercent[i]; // Not the item, so remove this percent and check the next value
		}
	}

	return NULL;
}

//=============================================================================
//
// hhItemAutomatic::FindAmmoNearby
//
//=============================================================================

float hhItemAutomatic::FindAmmoNearby( const char *ammoName ) {
	int				i;
	int				e;
	hhItem			*ent;
	idEntity		*entityList[ MAX_GENTITIES ];
	int				numListedEntities;
	idBounds		bounds;
	idVec3			org;
	float			adjustFactor = 1.0f;

	float nearbySize = spawnArgs.GetFloat( "nearbySize", "512" );

	org = GetPhysics()->GetOrigin();
	for ( i = 0 ; i < 3 ; i++ ) {
		bounds[0][i] = org[i] - nearbySize;
		bounds[1][i] = org[i] + nearbySize;
	}

	// Find the closest ammo types that are the same class
	numListedEntities = gameLocal.clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = static_cast< hhItem * >( entityList[e] );

		const char *name = ent->spawnArgs.GetString( "classname" );
		if ( !idStr::Icmp( name, ammoName ) ) {
			adjustFactor += spawnArgs.GetFloat( "nearbyReduction", "0.25" );
		}
	}	

	return adjustFactor;
}