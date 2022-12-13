#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

const idEventDef EV_GiveItems("<giveItems>", "e");

/***********************************************************************

hhItemCabinet

***********************************************************************/

const idEventDef EV_EnableItemClip( "<enableItemClip>" );

CLASS_DECLARATION( hhAnimatedEntity, hhItemCabinet )
	EVENT( EV_Activate,							hhItemCabinet::Event_Activate )
	EVENT( EV_EnableItemClip,					hhItemCabinet::Event_EnableItemClip )
	EVENT( EV_Broadcast_AppendFxToList,			hhItemCabinet::Event_AppendFxToIdleList )
	EVENT( EV_PostSpawn,						hhItemCabinet::Event_PostSpawn ) //rww
END_CLASS

/*
================
hhItemCabinet::hhItemCabinet
================
*/
hhItemCabinet::hhItemCabinet() {
}

/*
================
hhItemCabinet::Spawn
================
*/
void hhItemCabinet::Spawn() {
	animDoneTime = 0;

	GetPhysics()->SetContents( CONTENTS_BODY );

	ResetItemList();

	InitBoneInfo();

	if (!gameLocal.isClient) {
		PostEventMS(&EV_PostSpawn, 0);
	}

	StartSound( "snd_idle", SND_CHANNEL_IDLE );
}

/*
================
hhItemCabinet::Event_PostSpawn
================
*/
void hhItemCabinet::Event_PostSpawn(void) {
	SpawnIdleFX();
}

/*
================
hhItemCabinet::~hhItemCabinet
================
*/
hhItemCabinet::~hhItemCabinet() {

	for( int i = 0; i < CABINET_MAX_ITEMS; i++) {
		SAFE_REMOVE( itemList[i] );
	}

	hhUtils::RemoveContents< idEntityPtr<idEntityFx> >( idleFxList, true );

	StopSound( SND_CHANNEL_IDLE );
}

/*
================
hhItemCabinet::InitBoneInfo
================
*/
void hhItemCabinet::InitBoneInfo() {
	boneList[0] = idStr( spawnArgs.GetString("bone_shelfTop") );
	boneList[1] = idStr(spawnArgs.GetString("bone_shelfMiddle") );
	boneList[2] = idStr(spawnArgs.GetString("bone_shelfBottom") );
}

void hhItemCabinet::Save(idSaveGame *savefile) const {
	savefile->WriteInt( animDoneTime );

	int num = idleFxList.Num();
	savefile->WriteInt( num );
	for( int i = 0; i < num; i++ ) {
		idleFxList[i].Save( savefile );
	}

	for( int i = 0; i < CABINET_MAX_ITEMS; i++ ) {
		itemList[i].Save( savefile );
	}

	for( int i = 0; i < CABINET_MAX_ITEMS; i++ ) {
		savefile->WriteString( boneList[i] );
	}
}

void hhItemCabinet::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( animDoneTime );

	int num;
	savefile->ReadInt( num );
	idleFxList.Clear();
	idleFxList.SetNum( num );
	for( int i = 0; i < num; i++ ) {
		idleFxList[i].Restore( savefile );
	}

	for( int i = 0; i < CABINET_MAX_ITEMS; i++ ) {
		itemList[i].Restore( savefile );
	}

	for( int i = 0; i < CABINET_MAX_ITEMS; i++ ) {
		savefile->ReadString( boneList[i] );
	}
}

/*
================
hhItemCabinet::PlayAnim
================
*/
void hhItemCabinet::PlayAnim( const char* pAnimName, int iBlendTime ) {
	PlayAnim( GetAnimator()->GetAnim(pAnimName), iBlendTime );
}

/*
================
hhItemCabinet::PlayAnim
================
*/
void hhItemCabinet::PlayAnim( int pAnim, int iBlendTime ) {
	int iAnimLength = 0;

	if( pAnim ) {
		animator.ClearAllAnims( gameLocal.GetTime(), 0 );
		animator.PlayAnim( ANIMCHANNEL_ALL, pAnim, gameLocal.GetTime(), iBlendTime );
		iAnimLength = GetAnimator()->GetAnim( pAnim )->Length();
	}

	animDoneTime = gameLocal.GetTime() + iAnimLength;
}

/*
================
hhItemCabinet::SpawnIdleFX
================
*/
void hhItemCabinet::SpawnIdleFX() {
	hhUtils::RemoveContents< idEntityPtr<idEntityFx> >( idleFxList, true );

	BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_idle"), spawnArgs.GetString("bone_idleRight"), NULL, &EV_Broadcast_AppendFxToList );
	BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_idle"), spawnArgs.GetString("bone_idleLeft"), NULL, &EV_Broadcast_AppendFxToList );
}

//=============================================================================
//
// hhItemCabinet::SpawnDefaultItems
//
//=============================================================================

bool hhItemCabinet::SpawnDefaultItems() {
	idList<idStr> items;

	if ( spawnArgs.GetString( "items", NULL ) ) {
		hhUtils::SplitString( idStr(spawnArgs.GetString( "items" )), items );

		int slot = 0;
		for( int i = 0; i < items.Num(); i++ ) {
			AddItem( items[i], slot );
			if ( ++slot >= CABINET_MAX_ITEMS ) {
				return true;
			}
		}

		return true;
	}

	return false; // No default items
}

/*
================
hhItemCabinet::SpawnItems
================
*/

void hhItemCabinet::SpawnItems() {
	int				i;
	idStr			defaultName;

	int				numAmmo;
	
	idList<idStr>	weaponNames;
	idList<int>		weaponIndexes;
	idList<bool>	validWeapon;
	idList<float>	ammoPercent;
	idList<idStr>	ammoTypes;
	idList<idStr>	itemNames;

	float			total;

	bool			bAutomatic = true;

	if ( !bAutomatic ) { // Items were placed by a designer, so don't automatically add any items
		return;
	}		

	// This is the heart of the DDA system:
	// list all available weapons that can have ammo in the cabinet and how much ammo the player has

	hhPlayer *player = static_cast<hhPlayer *>(gameLocal.GetLocalPlayer());

	// Build lists of all valid weapon names, ammo types and ammo names
	// Note that all these must line up -- so the first weapon corresponds with the first ammo type and the first item name
	hhUtils::SplitString( idStr(spawnArgs.GetString( "weaponNames" )), weaponNames );
	hhUtils::SplitString( idStr(spawnArgs.GetString( "ammoTypes" )), ammoTypes );
	hhUtils::SplitString( idStr(spawnArgs.GetString( "itemNames" )), itemNames );

	numAmmo = weaponNames.Num();

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
			ammoPercent[i] = 1.0f - player->inventory.AmmoPercentage( player, ammoIndex );
			if ( ammoPercent[i] == 0.0f ) { // Give full ammo weapons a slight chance
				ammoPercent[i] = 0.01f;
			}
			validWeapon[i] = true;
		}
	}	

	// for each slot in the cabinet:
	for( int slot = 0; slot < numAmmo; slot++ ) {
		if ( gameLocal.random.RandomFloat() < spawnArgs.GetFloat( "emptyChance", "0.15" ) ) { // Chance the slot is empty
			continue;
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
					AddItem( itemNames[i], slot ); // Add the item
					ammoPercent[i] *= spawnArgs.GetFloat( "repeatReduce", "0.5" ); // reduce this item's chance of being chosen for the next slot
					break; // No need to check further, go to the next slot
				}

				random -= ammoPercent[i]; // Not the item, so remove this percent and check the next value
			}
		}
	}		
}

/*
================
hhItemCabinet::ResetItemList
================
*/
void hhItemCabinet::ResetItemList() {
	for( int i = 0; i < CABINET_MAX_ITEMS; i++ ) {
		SAFE_REMOVE( itemList[i] );
	}
}

/*
================
hhItemCabinet::AddItem
================
*/
void hhItemCabinet::AddItem( idStr itemName, int slot ) {
	idDict			Args;
	hhItem*			item = NULL;

	if ( slot < 0 || slot >= CABINET_MAX_ITEMS ) {
		return;
	}

	Args.SetBool( "spin", 0 );
	Args.SetFloat( "triggersize", 48.0f );
	Args.SetFloat( "respawn", 0.0f );
	Args.SetBool( "enablePickup", false );

	item = static_cast<hhItem *>( gameLocal.SpawnObject( itemName.c_str(), &Args ) );

	BroadcastFxInfoAlongBone( spawnArgs.GetString("fx_shelf"), boneList[slot] );

	// Check if the item should be rotated
	bool bRotated = false;
	for ( int i = 0; i < spawnArgs.GetInt( "numRotated" ); i++ ) {
		if ( !idStr::Icmp( spawnArgs.GetString( va("rotate%d", i) ), itemName.c_str() ) ) {
			bRotated = true;
		}
	}

	if( item ) {	
		const char *boneName = boneList[ slot ];

		idVec3 boneOffset;
		idMat3 boneAxis;
		this->GetJointWorldTransform( boneName, boneOffset, boneAxis );

		if ( bRotated ) {
			item->SetOrigin( boneOffset + GetAxis()[1] * -14 + GetAxis()[2] * 8 );
			item->SetAxis( idMat3( idVec3( 1, 0, 0 ), idVec3( 0, 0, -1), idVec3( 0, 1, 0 ) ) * GetAxis() );
		} else {
			item->MoveToJoint( this, boneName );
			item->SetAxis( GetAxis() );
		}
		item->BindToJoint( this, boneName, false );

		itemList[ slot ] = item;
		return;
	}
}

/*
===============
hhItemCabinet::HandleSingleGuiCommand
===============
*/
bool hhItemCabinet::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {

	idToken token;

	if( !src->ReadToken(&token) ) {
		return false;
	}

	if( token == ";" ) {
		return false;
	}

	if( token.Icmp("openCabinet") == 0 ) {
		Event_Activate( NULL );
		return true;
	}

	src->UnreadToken( &token );
	return false;
}

/*
================
hhItemCabinet::Event_AppendFxToIdleList
================
*/
void hhItemCabinet::Event_AppendFxToIdleList( hhEntityFx* fx ) {
	idleFxList.Append( fx );
}

/*
================
hhItemCabinet::Event_Activate
================
*/
void hhItemCabinet::Event_Activate( idEntity* pActivator ) {
	if( gameLocal.GetTime() < animDoneTime ) {
		return;
	}

	if ( !SpawnDefaultItems() ) {
		SpawnItems();
	}

	PlayAnim( "open", 0 );
	StartSound( "snd_open", SND_CHANNEL_ANY );
	PostEventMS( &EV_EnableItemClip, animDoneTime - gameLocal.GetTime() );

	ActivateTargets(pActivator);

	StartSound( "snd_idle_open", SND_CHANNEL_IDLE );
}

/*
================
hhItemCabinet::Event_EnableItemClip
================
*/
void hhItemCabinet::Event_EnableItemClip() {
	for( int i = 0; i < CABINET_MAX_ITEMS; i++ ) {
		if( !itemList[i].IsValid() ) {
			continue;
		}

		itemList[i]->EnablePickup();
		itemList[i] = NULL;
	}
}
