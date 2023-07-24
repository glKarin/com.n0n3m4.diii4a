// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Inventory.h"

#include "../decls/DeclInvItem.h"
#include "../Player.h"
#include "../Weapon.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../proficiency/StatsTracker.h"

#include "../botai/BotThreadData.h"

/*
===============================================================================

	sdItemPoolEntry

===============================================================================
*/

/*
==============
sdItemPoolEntry::sdItemPoolEntry
==============
*/
sdItemPoolEntry::sdItemPoolEntry( void ) {
	flags.disabled	= false;
	flags.hidden	= false;
	item			= NULL;
	joint			= INVALID_JOINT;
}

/*
==============
sdItemPoolEntry::SetItem
==============
*/
void sdItemPoolEntry::SetItem( const sdDeclInvItem* _item ) {
	item = _item;

	if ( item ) {
		for ( int j = 0; j < item->GetClips().Num(); j++ ) {
			clips.Alloc() = -1;
		}
	} else {
		clips.SetNum( 0, false );
	}
}

/*
===============================================================================

	sdPlayerClassSetup

===============================================================================
*/

/*
==============
sdPlayerClassSetup::sdPlayerClassSetup
==============
*/
sdPlayerClassSetup::sdPlayerClassSetup( void ) : playerClass( NULL ) {
}

/*
==============
sdPlayerClassSetup::operator=
==============
*/
void sdPlayerClassSetup::operator=( const sdPlayerClassSetup& rhs ) {
	playerClass = rhs.playerClass;
	playerClassOptions.SetNum( rhs.playerClassOptions.Num(), false );
	for ( int i = 0; i < playerClassOptions.Num(); i++ ) {
		playerClassOptions[ i ] = rhs.playerClassOptions[ i ];
	}
}

/*
==============
sdPlayerClassSetup::SetOptions
==============
*/
void sdPlayerClassSetup::SetOptions( const idList< int >& options ) {
	if ( options.Num() != playerClassOptions.Num() ) {
		gameLocal.Warning( "sdPlayerClassSetup::SetOptions Number of options did not match" );
		return;
	}

	playerClassOptions = options;
}

/*
==============
sdPlayerClassSetup::SetClass
==============
*/
bool sdPlayerClassSetup::SetClass( const sdDeclPlayerClass* pc, bool force ) {
	if ( pc == playerClass && !force ) {
		return false;
	}

	playerClass = pc;
	if ( playerClass ) {
		playerClassOptions.Fill( playerClass->GetNumOptions(), 0 );
	} else {
		playerClassOptions.SetNum( 0, false );
	}

	return true;
}

/*
==============
sdPlayerClassSetup::SetOption
==============
*/
bool sdPlayerClassSetup::SetOption( int index, int itemIndex ) {
	if ( index < 0 || index >= playerClassOptions.Num() ) {
		return false;
	}

	if ( playerClassOptions[ index ] == itemIndex ) {
		return false;
	}

	playerClassOptions[ index ] = itemIndex;
	return true;
}


/*
============
sdPlayerClassSetup::GetOption
============
*/
int	sdPlayerClassSetup::GetOption( int index ) const {
	assert( index >= 0 && index < playerClassOptions.Num() );

	return playerClassOptions[ index ];
}

/*
===============================================================================

	sdUpgradeItemPool

===============================================================================
*/

/*
==============
sdUpgradeItemPool::OnHide
==============
*/
void sdUpgradeItemPool::OnHide( void ) {
	for ( int i = 0; i < modelItems.Num(); i++ ) {
		modelItems[ i ]->HideModel();
	}
	modelItems.SetNum( 0, false );
}

/*
==============
sdUpgradeItemPool::OnShow
==============
*/
void sdUpgradeItemPool::OnShow( void ) {
	for ( int i = 0; i < items.Num(); i++ ) {
		if ( !items[ i ].IsVisible() ) {
			continue;
		}
		ShowItem( items[ i ] );
	}
}

/*
==============
sdUpgradeItemPool::ShowItem
==============
*/
bool sdUpgradeItemPool::ShowItem( sdItemPoolEntry& item ) {
	if ( !item.GetModel().hModel ) {
		return false;
	}

	modelItems.AddUnique( &item );
	item.ShowModel();

	return true;
}

/*
==============
sdUpgradeItemPool::HideItem
==============
*/
void sdUpgradeItemPool::HideItem( sdItemPoolEntry& item ) {
	modelItems.RemoveFast( &item );
	item.HideModel();
}

/*
==============
sdUpgradeItemPool::AddItem
==============
*/
int sdUpgradeItemPool::AddItem( const sdDeclInvItem* item, bool enabled ) {
	sdItemPoolEntry& entry = items.Alloc();

	entry.SetItem( item );
	entry.SetDisabled( !enabled );

	idPlayer* player = parent->GetOwner();

	idRenderModel* model = item->GetModel();
	if ( model && enabled ) {
		renderEntity_t& entity = entry.GetModel();

		entity.spawnID						= gameLocal.GetSpawnId( player );//->entityNumber;
		entity.suppressSurfaceInViewID		= player->entityNumber + 1;
		entity.noSelfShadowInViewID			= player->entityNumber + 1;
		entity.axis.Identity();
		entity.origin.Zero();
		entity.hModel						= model;
		entity.bounds						= entity.hModel->Bounds( &entity );
		entity.maxVisDist					= item->GetData().GetInt( "maxVisDist", "2048" );

		SetupModelShadows( entry );
		ShowItem( entry );
	}

	entry.joint = player->GetAnimator()->GetJointHandle( item->GetJoint() );

	return items.Num() - 1;
}

/*
==============
sdUpgradeItemPool::ClearItem
==============
*/
void sdUpgradeItemPool::ClearItem( sdItemPoolEntry& entry ) {
	HideItem( entry );
	entry.SetItem( NULL );
}

/*
==============
sdUpgradeItemPool::Init
==============
*/
void sdUpgradeItemPool::Init( sdInventory* _parent ) {
	parent = _parent;
}

/*
==============
sdUpgradeItemPool::Clear
==============
*/
void sdUpgradeItemPool::Clear( void ) {
	for( int i = 0; i < items.Num(); i++ ) {
		ClearItem( items[ i ] );
	}
	items.Clear();
}

/*
==============
sdUpgradeItemPool::ApplyPlayerState
==============
*/
void sdUpgradeItemPool::ApplyPlayerState( const sdInventoryPlayerStateData& newState ) {
	NET_GET_NEW( sdInventoryPlayerStateData );

	for( int i = 0; i < items.Num(); i++ ) {
		sdItemPoolEntry& item = items[ i ];

		for ( int j = 0; j < item.clips.Num(); j++ ) {
			if ( item.GetItem()->GetClips()[ j ].maxAmmo <= 0 ) {
				continue;
			}

			item.clips[ j ] = newData.itemData[ i ].clips[ j ];
		}
	}
}

/*
==============
sdUpgradeItemPool::ReadPlayerState
==============
*/
void sdUpgradeItemPool::ReadPlayerState( const sdInventoryPlayerStateData& baseState, sdInventoryPlayerStateData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdInventoryPlayerStateData );

	newData.itemData.SetNum( items.Num() );

	for( int i = 0; i < items.Num(); i++ ) {
		const sdItemPoolEntry& item = items[ i ];

		newData.itemData[ i ].clips.SetNum( item.clips.Num() );

		for ( int j = 0; j < item.clips.Num(); j++ ) {
			if ( i < baseData.itemData.Num() && j < baseData.itemData[ i ].clips.Num() ) {
				newData.itemData[ i ].clips[ j ] = msg.ReadDeltaShort( baseData.itemData[ i ].clips[ j ] );
			} else {
				newData.itemData[ i ].clips[ j ] = msg.ReadShort();
			}
		}
	}
}

/*
==============
sdUpgradeItemPool::WritePlayerState
==============
*/
void sdUpgradeItemPool::WritePlayerState( const sdInventoryPlayerStateData& baseState, sdInventoryPlayerStateData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdInventoryPlayerStateData );

	newData.itemData.SetNum( items.Num() );

	for( int i = 0; i < items.Num(); i++ ) {
		const sdItemPoolEntry& item = items[ i ];

		newData.itemData[ i ].clips.SetNum( item.clips.Num() );

		for ( int j = 0; j < item.clips.Num(); j++ ) {
			newData.itemData[ i ].clips[ j ] = item.clips[ j ];
			if ( i < baseData.itemData.Num() && j < baseData.itemData[ i ].clips.Num() ) {
				msg.WriteDeltaShort( baseData.itemData[ i ].clips[ j ], newData.itemData[ i ].clips[ j ] );
			} else {
				msg.WriteShort( newData.itemData[ i ].clips[ j ] );
			}
		}
	}
}

/*
==============
sdUpgradeItemPool::CheckPlayerStateChanges
==============
*/
bool sdUpgradeItemPool::CheckPlayerStateChanges( const sdInventoryPlayerStateData& baseState ) const {
	NET_GET_BASE( sdInventoryPlayerStateData );

	if ( baseData.itemData.Num() != items.Num() ) {
		return true;
	}


	for( int i = 0; i < items.Num(); i++ ) {
		const sdItemPoolEntry& item = items[ i ];

		if ( baseData.itemData[ i ].clips.Num() != item.clips.Num() ) {
			return true;
		}

		for ( int j = 0; j < item.clips.Num(); j++ ) {
			if ( baseData.itemData[ i ].clips[ j ] != item.clips[ j ] ) {
				return true;
			}
		}
	}

	return false;
}

/*
==============
sdUpgradeItemPool::UpdateJoints
==============
*/
void sdUpgradeItemPool::UpdateJoints( void ) {
	idPlayer* owner = parent->GetOwner();

	for( int i = 0; i < items.Num(); i++ ) {
		items[ i ].joint = owner->GetAnimator()->GetJointHandle( items[ i ].item->GetJoint() );
	}
}

/*
==============
sdUpgradeItemPool::UpdateModels
==============
*/
void sdUpgradeItemPool::UpdateModels( void ) {
	// no need to do this in reprediction
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	idPlayer* owner = parent->GetOwner();

	for ( int i = 0; i < modelItems.Num(); i++ ) {
		sdItemPoolEntry& item = *modelItems[ i ];

		renderEntity_t& entity = item.GetModel();

		entity.customSkin = owner->GetRenderEntity()->customSkin;

		owner->GetWorldOriginAxisNoUpdate( item.joint, entity.origin, entity.axis );

		item.UpdateModel();
	}
}

/*
==============
sdUpgradeItemPool::~sdUpgradeItemPool
==============
*/
sdUpgradeItemPool::~sdUpgradeItemPool( void ) {
	Clear();
}

/*
==============
sdUpgradeItemPool::SetupModelShadows
==============
*/
void sdUpgradeItemPool::SetupModelShadows( sdItemPoolEntry& entry ) {
	idPlayer* owner = parent->GetOwner();

	renderEntity_t& renderEntity = entry.GetModel();

	viewState_t state = owner->HasShadow();
	switch ( state ) {
		case VS_NONE: {
			renderEntity.suppressShadowInViewID = 0;
			renderEntity.flags.noShadow			= true;
			break;
		}
		case VS_REMOTE: {
			renderEntity.suppressShadowInViewID = owner->entityNumber + 1;
			renderEntity.flags.noShadow			= false;
			break;
		}
		case VS_FULL: {
			renderEntity.suppressShadowInViewID = 0;
			renderEntity.flags.noShadow			= false;
			break;
		}
	}
}

/*
============
sdUpgradeItemPool::UpdateModelShadows
============
*/
void sdUpgradeItemPool::UpdateModelShadows( void ) {
	for ( int i = 0; i < modelItems.Num(); i++ ) {
		SetupModelShadows( *modelItems[ i ] );
	}
}

/*
===============================================================================

	sdInventory

===============================================================================
*/

idList< int > sdInventory::slotForBank;

/*
==============
sdInventoryBroadcastData::MakeDefault
==============
*/
void sdInventoryBroadcastData::MakeDefault( void ) {
	idealWeapon					= -1;
	disabledMask.Shutdown();
	ammoMask.Shutdown();

	proficiencyData.MakeDefault();
}

/*
==============
sdInventoryBroadcastData::Write
==============
*/
void sdInventoryBroadcastData::Write( idFile* file ) const {
	file->WriteInt( idealWeapon );

	file->WriteInt( disabledMask.GetSize() );
	for ( int i = 0; i < disabledMask.GetSize(); i++ ) {
		file->WriteInt( disabledMask.GetDirect( i ) );
	}

	file->WriteInt( ammoMask.GetSize() );
	for ( int i = 0; i < ammoMask.GetSize(); i++ ) {
		file->WriteInt( ammoMask.GetDirect( i ) );
	}

	proficiencyData.Write( file );
}

/*
==============
sdInventoryBroadcastData::Read
==============
*/
void sdInventoryBroadcastData::Read( idFile* file ) {
	file->ReadInt( idealWeapon );

	int sizeDummy;

	file->ReadInt( sizeDummy );
	disabledMask.SetSize( sizeDummy );
	for ( int i = 0; i < disabledMask.GetSize(); i++ ) {
		file->ReadInt( disabledMask.GetDirect( i ) );
	}

	file->ReadInt( sizeDummy );
	ammoMask.SetSize( sizeDummy );
	for ( int i = 0; i < ammoMask.GetSize(); i++ ) {
		file->ReadInt( ammoMask.GetDirect( i ) );
	}

	proficiencyData.Read( file );
}

/*
==============
sdInventoryPlayerStateData::MakeDefault
==============
*/
void sdInventoryPlayerStateData::MakeDefault( void ) {
	ammo.SetNum( gameLocal.declAmmoTypeType.Num() );
	for ( int i = 0; i < ammo.Num(); i++ ) {
		ammo[ i ] = 0;
	}
	itemData.SetNum( 0, false );
}

/*
==============
sdInventoryPlayerStateData::Write
==============
*/
void sdInventoryPlayerStateData::Write( idFile* file ) const {
	file->WriteInt( ammo.Num() );
	for ( int i = 0; i < ammo.Num(); i++ ) {
		file->WriteShort( ammo[ i ] );
	}

	file->WriteInt( itemData.Num() );
	for ( int i = 0; i < itemData.Num(); i++ ) {
		itemData[ i ].Write( file );
	}
}

/*
==============
sdInventoryPlayerStateData::Read
==============
*/
void sdInventoryPlayerStateData::Read( idFile* file ) {
	int count;
	file->ReadInt( count );

	assert( gameLocal.declAmmoTypeType.Num() == count );

	ammo.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		file->ReadShort( ammo[ i ] );
	}

	file->ReadInt( count );

	itemData.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		itemData[ i ].Read( file );
	}
}

/*
==============
sdInventoryItemStateData::MakeDefault
==============
*/
void sdInventoryItemStateData::MakeDefault( void ) {
	clips.SetNum( 0, false );
}

/*
==============
sdInventoryItemStateData::Write
==============
*/
void sdInventoryItemStateData::Write( idFile* file ) const {
	file->WriteInt( clips.Num() );
	for ( int i = 0; i < clips.Num(); i++ ) {
		file->WriteShort( clips[ i ] );
	}
}

/*
==============
sdInventoryItemStateData::Read
==============
*/
void sdInventoryItemStateData::Read( idFile* file ) {
	int count;
	file->ReadInt( count );

	clips.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		file->ReadShort( clips[ i ] );
	}
}

/*
==============
sdInventory::GetCommandMapIcon
==============
*/
const idMaterial* sdInventory::GetCommandMapIcon( const iconType_e iconType ) const {
	const sdDeclPlayerClass* pc = playerClass.GetClass();

	if ( !pc ) {
		return NULL;
	}

	switch( iconType ) {
		case IT_CLASS:
			return pc->GetCommandmapIconClass();
		case IT_ICON:
			return pc->GetCommandmapIcon();
		case IT_UNKNOWN:
			return pc->GetCommandmapIconUnknown();
	}

	return NULL;
}

/*
==============
sdInventory::UpdateItems
==============
*/
void sdInventory::UpdateItems( void ) {
	if ( classThread ) {
		if ( classThread->Execute() ) {
			ShutdownClassThread();
		}
	}
}

/*
==============
sdInventory::Present
==============
*/
void sdInventory::Present( void ) {
	items.UpdateModels();
}


/*
==============
sdInventory::Init
==============
*/
void sdInventory::Init( idDict& dict, bool full, bool setWeapon ) {
	ammo.AssureSize( gameLocal.declAmmoTypeType.Num(), 0 );
	ammoLimits.AssureSize( gameLocal.declAmmoTypeType.Num(), 0 );

	ClearAmmo();
	items.Clear();

	currentWeapon	= -1;
	if ( !gameLocal.isClient ) {
		idealWeapon		= -1;
	}
	switchingWeapon	= -1;

	if ( !gameLocal.isClient ) {
		CheckPlayerClass( setWeapon );
	}
	SetupClassOptions( true, setWeapon );

	SetupModel();

	LogClassTime();
}

/*
==============
sdInventory::WriteDemoBaseData
==============
*/
void sdInventory::WriteDemoBaseData( idFile* file ) const {
	int playerClassIndex = playerClass.GetClass() ? playerClass.GetClass()->Index() : -1;

	file->WriteInt( playerClassIndex );
	if ( playerClassIndex != -1 ) {
		for ( int i = 0; i < playerClass.GetOptions().Num(); i++ ) {
			file->WriteInt( playerClass.GetOptions()[ i ] );
		}
	}

	int cachedPlayerClassIndex = cachedPlayerClass.GetClass() ? cachedPlayerClass.GetClass()->Index() : -1;

	file->WriteInt( cachedPlayerClassIndex );
	if ( cachedPlayerClassIndex != -1 ) {
		for ( int i = 0; i < cachedPlayerClass.GetOptions().Num(); i++ ) {
			file->WriteInt( cachedPlayerClass.GetOptions()[ i ] );
		}
	}
}

/*
==============
sdInventory::ReadDemoBaseData
==============
*/
void sdInventory::ReadDemoBaseData( idFile* file ) {
	int playerClassIndex;

	file->ReadInt( playerClassIndex );
	if ( playerClassIndex != -1 ) {
		const sdDeclPlayerClass* cls = gameLocal.declPlayerClassType[ playerClassIndex ];

		SetPlayerClass( cls );

		for ( int i = 0; i < playerClass.GetOptions().Num(); i++ ) {
			int itemIndex;

			file->ReadInt( itemIndex );
			playerClass.SetOption( i, itemIndex );
		}
	}

	int cachedPlayerClassIndex;

	file->ReadInt( cachedPlayerClassIndex );
	if ( cachedPlayerClassIndex != -1 ) {
		const sdDeclPlayerClass* cls = gameLocal.declPlayerClassType[ cachedPlayerClassIndex ];
		cachedPlayerClass.SetClass( cls, true );

		for ( int i = 0; i < cachedPlayerClass.GetOptions().Num(); i++ ) {
			int itemIndex;

			file->ReadInt( itemIndex );
			cachedPlayerClass.SetOption( i, itemIndex );
		}
	}

	SetupClassOptions( true, false );
}

/*
==============
sdInventory::sdInventory
==============
*/
sdInventory::sdInventory( void ) {
	weaponChanged		= false;
	classThread			= NULL;
	timeClassChanged	= 0;
	currentWeaponIndex	= -1;
	idealWeapon			= -1;


	SetOwner( NULL );

	items.Init( this );
}

/*
==============
sdInventory::SetOwner
==============
*/
void sdInventory::SetOwner( idPlayer* _owner ) {
	owner = _owner;
}

/*
==============
sdInventory::GetOwner
==============
*/
idPlayer* sdInventory::GetOwner( void ) {
	return owner;
}

/*
==============
sdInventory::~sdInventory
==============
*/
sdInventory::~sdInventory( void ) {
}

/*
==============
sdInventory::GetAmmoFraction
==============
*/
float sdInventory::GetAmmoFraction( void ) {
	if ( !playerClass.GetClass() ) {
		return 0.f;
	}

	int max = 0;
	int ammoCount = 0;
	for ( int i = 0; i < ammo.Num(); i++ ) {
		ammoCount += ammo[ i ];
		max += ammoLimits[ i ];
	}

	return ammoCount / ( float )( max );
}

/*
==============
sdInventory::BuildSlotBankLookup
==============
*/
void sdInventory::BuildSlotBankLookup( void ) {
	slotForBank.Clear();

	const sdDeclStringMap* stringMap = gameLocal.declStringMapType.LocalFind( "inventorySlots" );
	const idDict& dict = stringMap->GetDict();

	int i = 0;
	while( true ) {
		//const sdDeclInvSlot* slot = gameLocal.declInvSlotType.LocalFindByIndex( i, true );
		const char* value = dict.GetString( va( "slot%i", i ) );
		i++;
		if( value[ 0 ] == '\0' ) {
			break;
		}
		const sdDeclInvSlot* slot = gameLocal.declInvSlotType.LocalFind( value, true );

		int slotBank = slot->GetBank();
		if( slotBank == -1 ) {
			continue;
		}

		slotForBank.AssureSize( slotBank + 1, -1 );

		if( slotForBank[ slotBank ] != -1 ) {
			gameLocal.Error( "sdInventory::BuildSlotBankLookup Multiple Slots Using Weapon Bank %i", slotBank );
		}

		slotForBank[ slotBank ] = slot->Index();		
	}
}

/*
==============
sdInventory::GetSlotForWeapon
==============
*/
int sdInventory::GetSlotForWeapon( int weapon ) const {
	if ( weapon >= 0 && weapon < items.Num() ) {
		if ( CanEquip( weapon, false ) ) {

			int count = slotForBank.Num();
			for ( int i = count - 1; i >= 0; i-- ) {
				int slot = slotForBank[ i ];
				if ( slot == -1 ) {
					continue;
				}

				if ( items[ weapon ].item->UsesSlot( slot ) ) {
					return i;
				}
			}
		}
	}
	return -1;
}

/*
==============
sdInventory::GetCurrentSlot
==============
*/
int sdInventory::GetCurrentSlot( void ) const {
	return GetSlotForWeapon( idealWeapon );
}

/*
==============
sdInventory::GetSwitchingSlot
==============
*/
int sdInventory::GetSwitchingSlot( void ) const {
	return GetSlotForWeapon( switchingWeapon );
}


/*
============
sdInventory::FindBestWeapon
============
*/
int sdInventory::FindBestWeapon( bool allowCurrent ) {
	int bestIndex = -1;
	int bestRank = 9999;

	for ( int i = 0; i < items.Num(); i++ ) {
		if ( !allowCurrent ) {
			if( ( i == switchingWeapon || i == currentWeapon ) ) {
				continue;
			}
		}

		if ( !CanAutoEquip( i, true ) ) {
			continue;
		}

		const sdDeclInvItem* item = items[ i ].item;

		int rank = item->GetAutoSwitchPriority();
		if ( rank < bestRank && rank > 0 && CheckWeaponHasAmmo( item ) ) {
			bestIndex = i;
			bestRank = rank;
		}
	}

	return bestIndex;
}

/*
============
sdInventory::CycleNextSafeWeapon
============
*/
void sdInventory::SelectBestWeapon( bool allowCurrent ) {	
	int bestIndex = FindBestWeapon( allowCurrent );
	if ( bestIndex != -1 ) {
		SetIdealWeapon( bestIndex );
	}
}

/*
============
sdInventory::CycleNextSafeWeapon
============
*/
void sdInventory::CycleNextSafeWeapon( void ) {	
	int bestIndex = FindBestWeapon( false );
	if ( bestIndex != -1 ) {
		SetSwitchingWeapon( bestIndex );
	}
}

/*
==============
sdInventory::CycleWeaponsNext
==============
*/
void sdInventory::CycleWeaponsNext( int currentSlot ) {
	bool force = true;
	bool looped;
	int weapon;

	if( currentSlot == -999 ) {
		force = false;
		currentSlot = ChooseCurrentSlot();
	}
	
	weapon = CycleWeaponByPosition( currentSlot, true, looped, false, false );

	if( !looped && weapon != -1 ) {
		SetSwitchingWeapon( weapon );
		return;
	}

	int cnt = 0;
	int max = slotForBank.Num();
	for( int i = currentSlot + 1; cnt < max; cnt++ ) {
		int pos = ( i + cnt ) % max;

		weapon = CycleWeaponByPosition( pos, true, looped, false, false );
		if( weapon != -1 ) {
			SetSwitchingWeapon( weapon );
			return;
		}
	}
}

/*
==============
sdInventory::CycleWeaponsPrev
==============
*/
void sdInventory::CycleWeaponsPrev( int currentSlot ) {
	bool force = true;
	bool looped;
	int weapon;

	if( currentSlot == -999 ) {
		force = false;
		currentSlot = ChooseCurrentSlot();
	}

	weapon = CycleWeaponByPosition( currentSlot, false, looped, false, false );
	if( !looped && weapon != -1 ) {
		SetSwitchingWeapon( weapon );
		return;
	}

	int cnt = 0;
	int max = slotForBank.Num();
	for( int i = currentSlot - 1; cnt < max; cnt++ ) {
		int pos = ( ( i - cnt ) + max ) % max;

		weapon = CycleWeaponByPosition( pos, false, looped, false, false );
		if( weapon != -1 ) {
			SetSwitchingWeapon( weapon );
			return;
		}
	}
}

/*
==============
sdInventory::CycleWeaponByPosition

if "primaryOnly" == true, it will only select the first weapon in that slot, never looping to any other weapon that
may share that slot.
==============
*/
int sdInventory::CycleWeaponByPosition( int pos, bool forward, bool& looped, bool force, bool primaryOnly ) {
	if( pos < 0 || pos >= slotForBank.Num() ) {
		return false;
	}

	int slot = slotForBank[ pos ];
	if( slot == -1 ) {
		return false;
	}

	int startpos = 0;

	if ( !primaryOnly ) {
        if( ( ChooseCurrentSlot() == pos ) ) {
			if( switchingWeapon >= 0 ) {
				startpos = switchingWeapon;
			} else if( idealWeapon >= 0 ) {
				startpos = idealWeapon;
			} else {
				startpos = currentWeapon;
			}
		}
	}

	return CycleWeaponBySlot( slot, forward, looped, force, startpos );
}

/*
==============
sdInventory::UpdatePrimaryWeapon
==============
*/
void sdInventory::UpdatePrimaryWeapon( void ) {
    for ( int i = 0; i < items.Num(); i++ ) {
		
		const sdDeclInvItem* item = items[ i ].GetItem();
	
		if ( item == NULL || !item->UsesSlot( slotForBank[ GUN_SLOT ] ) ) {
			continue;
		}

		if ( !CanEquip( i, true ) ) {
			continue;
		}

		int weaponNum = item->GetData().GetInt( "player_weapon_num", "-1" );

		if ( weaponNum != 14 && weaponNum != -1 ) { //mal_FIXME: 14 = hack for the AR/Grenade Launcher combo. Need to fix this!
			botThreadData.GetGameWorldState()->clientInfo[ owner->entityNumber ].weapInfo.primaryWeapon = ( playerWeaponTypes_t ) weaponNum;
			break;
		}
	}
}

/*
==============
sdInventory::CheckWeaponSlotHasAmmo
==============
*/
bool sdInventory::CheckWeaponSlotHasAmmo( int slot ) {
	bool hasAmmo = false;
	int i, ammoInClip, clipSize;

	clientInfo_t &client = botThreadData.GetGameWorldState()->clientInfo[ owner->entityNumber ];

    for ( i = 0; i < items.Num(); i++ ) {
		
		const sdDeclInvItem* item = items[ i ].GetItem();
	
		if ( item == NULL || !item->UsesSlot( slotForBank[ slot ] ) ) {
			continue;
		}

		if ( !CanEquip( i, true ) ) {
			continue;
		}

		if ( CheckWeaponHasAmmo( item ) ) {
			hasAmmo = true;

			if ( slot == GUN_SLOT ) { //mal: if this is our primary gun we're checking, see if it needs ammo.
				const idList< itemClip_t >& itemClips = item->GetClips();
				int ammoFraction;
				int maxAmmo, curAmmo;

				if ( !itemClips.Num() ) {
					break;
				}

				if ( client.team == GDF ) {
					if ( client.classType == SOLDIER && client.weapInfo.primaryWeapon == SMG ) {
                        ammoFraction = 4; //mal: GDF soldiers with assault rifle get as much ammo as strogg!
					} else {
						ammoFraction = 2;
					}
				} else {
					if ( client.weapInfo.primaryWeapon == ROCKET ) {
						ammoFraction = 2;
					} else {
						ammoFraction = 4; //mal: non-rocket launcher strogg have SO much more ammo then GDF.
					}
				}

				maxAmmo = GetMaxAmmo( itemClips[ MAIN_GUN ].ammoType );
				curAmmo = GetAmmo( itemClips[ MAIN_GUN ].ammoType );

//mal: while we're here, lets check if our primary weapon needs a reload, so we can use all of the stuff we've already calc'd
				if ( client.team == STROGG && client.weapInfo.primaryWeapon != ROCKET ) {
					client.weapInfo.primaryWeapNeedsReload = false;
					client.weapInfo.primaryWeapClipEmpty = false;
				} else {
					ammoInClip = GetClip( i, MAIN_GUN );
					clipSize = GetClipSize( i, MAIN_GUN );

					if ( ammoInClip == 0 || ammoInClip < clipSize ) {
						client.weapInfo.primaryWeapNeedsReload = true;
					} else {
						client.weapInfo.primaryWeapNeedsReload = false;
					}

					if ( ammoInClip == 0 ) {
						client.weapInfo.primaryWeapClipEmpty = true;
					} else {
						client.weapInfo.primaryWeapClipEmpty = false;
					}

					if ( curAmmo > clipSize ) {
						client.weapInfo.hasAmmoForReload = true;
					} else {
						client.weapInfo.hasAmmoForReload = false;
					}
				}

				if ( ( maxAmmo / ammoFraction ) > curAmmo ) {
					botThreadData.GetGameWorldState()->clientInfo[ owner->entityNumber ].weapInfo.primaryWeapNeedsAmmo = true;
				} else {
					botThreadData.GetGameWorldState()->clientInfo[ owner->entityNumber ].weapInfo.primaryWeapNeedsAmmo = false;
				}
			}
			break;
		}
	}

	return hasAmmo;
}

/*
==============
sdInventory::CyleWeaponBySlot
==============
*/
int sdInventory::CycleWeaponBySlot( int slot, bool forward, bool& looped, bool force, int startingpos ) {
	looped = false;

	int i = startingpos;// + ( forward ? 1 : -1 );
	int j;
	int max = items.Num();

	for( j = 0; j < max; j++ ) {
		if( forward ) {
			i++;
			if( i >= max ) { looped = true;	i -= max;}
		} else {
			i--;
			if( i < 0 ) { looped = true; i += max; }
		}

//		if ( !force ) {
//
//			if( /*!looped && */( i == switchingWeapon || i == currentWeapon )) {
//				continue;
//			}
//		}

		if ( !CanAutoEquip( i, false ) ) {
			continue;
		}

		const sdDeclInvItem* item = items[ i ].item;
		if ( !item->UsesSlot( slot ) ) {
			continue;
		}

		if ( !item->GetSelectWhenEmpty() && !CheckWeaponHasAmmo( item )) {
			continue;
		}

		return i;
	}

	return -1;
}

/*
==============
sdInventory::SelectWeaponByName
==============
*/
void sdInventory::SelectWeaponByName( const char* weaponName, bool ignoreInhibit ) {
	if ( !ignoreInhibit  ) {
		if ( owner->InhibitWeaponSwitch() ) {
			return;
		}
	}

	int weaponIndex = FindWeapon( weaponName );
	if ( weaponIndex == -1 ) {
		return;
	}

	if ( !CanEquip( weaponIndex, true ) ) {
		return;
	}

	SetIdealWeapon( weaponIndex );
}

/*
==============
sdInventory::SelectWeaponByNumber
==============
*/
void sdInventory::SelectWeaponByNumber( const playerWeaponTypes_t weaponNum ) {
	if ( owner->InhibitWeaponSwitch() ) {
		return;
	}

	int weaponIndex = FindWeaponNum( weaponNum );
	if ( weaponIndex == -1 ) {
		return;
	}

	if ( !CanEquip( weaponIndex, true ) ) {
		return;
	}

	SetIdealWeapon( weaponIndex );
}

/*
==============
sdInventory::CanEquip
==============
*/
bool sdInventory::CanEquip( int index, bool checkRequirements ) const {
	if ( index < 0 || index >= items.Num() ) {
		return false;
	}

	const sdDeclInvItem* item = items[ index ].item;
	if ( item == NULL ) {
		assert( false );
		return false;
	}

	if ( items[ index ].IsDisabled() ) {
		return false;
	}

	if ( owner->GetProxyEntity() != NULL ) {
		if( !item->GetType()->IsVehicleEquipable() ) {
			return false;
		}
	} else {
		if( !item->GetType()->IsEquipable() ) {
			return false;
		}
	}

	if ( checkRequirements ) {
		if ( !item->GetUsageRequirements().Check( owner ) ) {
			return false;
		}
	}

	return true;
}

/*
==============
sdInventory::CanAutoEquip
==============
*/
bool sdInventory::CanAutoEquip( int index, bool checkExplosive ) const {
	if ( !CanEquip( index, true ) ) {
		return false;
	}

	const sdDeclInvItem* item = items[ index ].item;
	if ( item->GetWeaponMenuIgnore() ) {
		return false;
	}

	if ( checkExplosive ) {
		if ( item->GetAutoSwitchIsExplosive() && owner->userInfo.ignoreExplosiveWeapons ) {
			return false;
		}
	}

	return true;
}

/*
==============
sdInventory::FindWeapon
==============
*/
int sdInventory::FindWeapon( const char* weaponName ) {
	for( int i = 0; i < items.Num(); i++ ) {
		if ( !CanEquip( i, true ) ) {
			continue;
		}

		const sdDeclInvItem* item = items[ i ].item;
		if ( !idStr::Icmp( item->GetName(), weaponName ) ) {
			return i;
		}
	}
	return -1;
}

/*
==============
sdInventory::FindWeaponNum

A handy function that lets the player pick a specific weapon, without cycling thru the slots.
==============
*/
int sdInventory::FindWeaponNum( const playerWeaponTypes_t weaponNum ) {
	for( int i = 0; i < items.Num(); i++ ) {
		if ( !CanEquip( i, true ) ) {
			continue;
		}

		const sdDeclInvItem* weapItem = items[ i ].item;

		if ( weapItem->GetData().GetInt( "player_weapon_num", "-1" ) == weaponNum ) {
			return i;
		}
	}
	return -1;
}

/*
==============
sdInventory::SetIdealWeapon
==============
*/
void sdInventory::SetIdealWeapon( int pos, bool force ) {
	if ( pos < -1 || pos >= items.Num() ) {
		return;
	}

	bool set = force || pos == -1;
	if ( !set ) {
		if ( pos == currentWeapon ) {
			set = items[ pos ].item->Index() != currentWeaponIndex;
		} else if ( pos != idealWeapon ) {
			set = true;
		}
	}

	if ( set ) {
		idealWeapon = pos;
		weaponChanged = true;

		if ( pos != -1 ) {		
			currentWeaponIndex = items[ pos ].item->Index();
			owner->SpawnToolTip( items[ pos ].item->GetToolTip() );
		} else {
			currentWeaponIndex = -1;
		}
	}

	CancelWeaponSwitch();
}

/*
==============
sdInventory::GetClip
==============
*/
int	sdInventory::GetClip( int index, int modIndex ) const {
	if ( index >= items.Num() || modIndex >= items[ index ].clips.Num() ) {
		return 0;
	}
	return items[ index ].clips[ modIndex ];
}


/*
============
sdInventory::GetClipSize
============
*/
int	sdInventory::GetClipSize( int index, int modIndex ) const {
	if ( index >= items.Num() || modIndex >= items[ index ].clips.Num() ) {
		return 0;
	}
	return items[ index ].GetItem()->GetClips()[ modIndex ].maxAmmo;
}

/*
==============
sdInventory::SetClip
==============
*/
void sdInventory::SetClip( int index, int modIndex, int count ) {
	if ( index >= items.Num() || modIndex >= items[ index ].clips.Num() ) {
		return;
	}
	items[ index ].clips[ modIndex ] = count;
}

/*
==============
sdInventory::IsWeaponValid
==============
*/
bool sdInventory::IsWeaponValid( int weapon ) const {
	if ( weapon < 0 || weapon >= items.Num() ) {
		return false;
	}

	return CanEquip( weapon, false );
}

/*
==============
sdInventory::IsWeaponBankValid
0-based weapon bank (0 is fists, 1 is pistol, etc)
==============
*/
bool sdInventory::IsWeaponBankValid( int slot ) const {
	if( slot < 0 || slot >= slotForBank.Num() ) {
		return false;
	}
	
	bool allValid = false;
	for ( int i = 0; i < items.Num(); i++ ) {
		if ( !CanEquip( i, true ) ) {
			continue;
		}

		const sdDeclInvItem* item = items[ i ].GetItem();
		if ( !item->UsesSlot( slotForBank[ slot ] )) {
			continue;
		}

		if ( CheckWeaponHasAmmo( item ) || item->GetSelectWhenEmpty() ) {
			allValid = true;
			break;
		}
	}

	return allValid;
}


/*
============
sdInventory::NumValidWeaponBanks
============
*/
int sdInventory::NumValidWeaponBanks() const {
	int numValid = 0;
	int max = slotForBank.Num();
	for( int i = 0; i < max; i++ ) {
		if( IsWeaponBankValid( i )) {
			numValid++;
		}
	}

	return numValid;
}

/*
==============
sdInventory::GetActivePlayer
==============
*/
idPlayer* sdInventory::GetActivePlayer( void ) const {
	idPlayer* modelPlayer = NULL;

	int disguiseClient = owner->GetDisguiseClient();
	if ( disguiseClient != -1 ) {
		modelPlayer = gameLocal.EntityForSpawnId( disguiseClient )->Cast< idPlayer >();
	}
	if ( modelPlayer == NULL ) {
		modelPlayer = owner;
	}
	return modelPlayer;
}

/*
==============
sdInventory::GetActivePlayerClass
==============
*/
const sdDeclPlayerClass* sdInventory::GetActivePlayerClass( void ) const {
	if ( owner->IsDisguised() ) {
		return owner->GetDisguiseClass();
	}
	return playerClass.GetClass();
}

/*
==============
sdInventory::SetupModel
==============
*/
void sdInventory::SetupModel( void ) {
	SetupModel( GetActivePlayerClass() );
}

/*
==============
sdInventory::GetPlayerJoint
==============
*/
jointHandle_t sdInventory::GetPlayerJoint( const idDict& dict, const char* name ) {
	const char* value = dict.GetString( name );
	jointHandle_t handle = owner->GetAnimator()->GetJointHandle( value );
	if ( handle == INVALID_JOINT ) {
		gameLocal.Error( "sdInventory::GetPlayerJoint '%s' not found for '%s' on '%s'", value, name, owner->name.c_str() );
	}
	return handle;
}

/*
==============
sdInventory::SkinForClass
==============
*/
const idDeclSkin* sdInventory::SkinForClass( const sdDeclPlayerClass* cls ) {
	if ( gameLocal.mapSkinPool != NULL ) {
		const char* skinKey = cls->GetClimateSkinKey();
		if ( *skinKey != '\0' ) {
			const char* skinName = gameLocal.mapSkinPool->GetDict().GetString( va( "skin_%s", skinKey ) );
			if ( *skinName == '\0' ) {
				gameLocal.Warning( "sdInventory::SetupModel No Skin Set For '%s'", skinKey );
			} else {
				const idDeclSkin* skin = gameLocal.declSkinType[ skinName ];
				if ( skin == NULL ) {
					gameLocal.Warning( "sdScriptEntity::Spawn Skin '%s' Not Found", skinName );
				} else {
					return skin;
				}
			}
		}
	}

	return NULL;
}

/*
==============
sdInventory::SetupModel
==============
*/
void sdInventory::SetupModel( const sdDeclPlayerClass* cls ) {
	owner->SetSkin( NULL );

	if ( cls == NULL ) {
		owner->SetModel( "" );
		owner->UpdateShadows();
		owner->SetHipJoint( INVALID_JOINT );
		owner->SetTorsoJoint( INVALID_JOINT );
		owner->SetHeadJoint( INVALID_JOINT );

		items.UpdateJoints();

		return;
	}

	if( !cls->GetModel() ) {
		gameLocal.Error( "sdInventory::SetupModel NULL model for class '%s'", cls->GetName() );
	}

	const idDict& dict = cls->GetModelData();
	
	owner->SetModel( cls->GetModel()->GetName() );
	owner->UpdateShadows();

	const idDeclSkin* skin = SkinForClass( cls );
	if ( skin != NULL ) {
		owner->SetSkin( skin );
	}

	owner->SetHipJoint( GetPlayerJoint( dict, "bone_hips" ) );
	owner->SetHeadJoint( GetPlayerJoint( dict, "bone_head" ) );
	owner->SetChestJoint( GetPlayerJoint( dict, "bone_chest" ) );
	owner->SetTorsoJoint( GetPlayerJoint( dict, "bone_torso" ) );
	owner->SetShoulderJoint( 0, GetPlayerJoint( dict, "bone_left_shoulder" ) );
	owner->SetElbowJoint( 0, GetPlayerJoint( dict, "bone_left_elbow" ) );
	owner->SetHandJoint( 0, GetPlayerJoint( dict, "bone_left_hand" ) );
	owner->SetFootJoint( 0, GetPlayerJoint( dict, "bone_left_foot" ) );
	owner->SetShoulderJoint( 1, GetPlayerJoint( dict, "bone_right_shoulder" ) );
	owner->SetElbowJoint( 1, GetPlayerJoint( dict, "bone_right_elbow" ) );
	owner->SetHandJoint( 1, GetPlayerJoint( dict, "bone_right_hand" ) );
	owner->SetFootJoint( 1, GetPlayerJoint( dict, "bone_right_foot" ) );

	owner->SetHeadModelJoint( GetPlayerJoint( dict, "bone_head_model" ) );
	owner->SetHeadModelOffset( dict.GetVector( "head_offset" ) );

	SetupLocationalDamage( dict );

	sdScriptHelper h;
	owner->CallNonBlockingScriptEvent( owner->GetScriptFunction( "OnNewModel" ), h );

	items.UpdateJoints();
}

/*
============
sdInventory::SetupLocationalDamage
============
*/
void sdInventory::SetupLocationalDamage( const idDict& dict ) {
	owner->RemoveLocationalDamageInfo();

	locationalDamageInfo_t info;
	int	numLocationDamageJoints = dict.GetInt( "loc_damage_joint_num", "6" );
	for( int i = 0; i < numLocationDamageJoints; i++ ) {
		const char* jointName = dict.GetString( va( "loc_damage_joint_%d", i ), "" );
		info.joint = owner->GetAnimator()->GetJointHandle( jointName );
		if( info.joint == INVALID_JOINT ) {
			gameLocal.Warning( "Invalid locational damage joint %d", i );
			continue;
		}
		
		if( !owner->GetAnimator()->GetJointTransform( info.joint, gameLocal.time, info.pos ) ) {
			gameLocal.Warning( "Invalid local transform for locational damage joint %d", i );
			continue;
		}

		info.area = owner->LocationalDamageAreaForString( dict.GetString( va( "loc_damage_area_%d", i ), "" ) );
		if( info.area == LDA_INVALID ) {
			gameLocal.Warning( "Invalid locational damage area for joint %d", i );
			continue;
		}

//		gameLocal.Printf( "Adding jointPos: ( %.0f %.0f %.0f )\n", info.pos.x, info.pos.y, info.pos.z );
		owner->AddLocationalDamageInfo( info );
	}
}

/*
==============
sdInventory::CheckPlayerClass
==============
*/
void sdInventory::CheckPlayerClass( bool setWeapon ) {
	sdTeamInfo* team = owner->GetTeam();
	if ( !team ) {
		return;
	}

	const sdDeclPlayerClass* newClass = playerClass.GetClass();

	sdPlayerClassSetup oldCachedClassSetup = cachedPlayerClass;

	if ( cachedPlayerClass.GetClass() ) {
		if ( cachedPlayerClass.GetClass()->GetTeam() == team ) {
			newClass = cachedPlayerClass.GetClass();
		}

		SetCachedClass( NULL );
	}

	if ( newClass ) {
		if ( newClass->GetTeam() != team ) {
			newClass = NULL;
		}
	}

	if ( !newClass ) {
		newClass = team->GetDefaultClass();
	}

	bool sendInfo = false;

	if ( newClass != playerClass.GetClass() ) {
		sendInfo |= GiveClass( newClass, false );
	}

	if ( oldCachedClassSetup.GetClass() == playerClass.GetClass() ) {
		playerClass.SetOptions( oldCachedClassSetup.GetOptions() );
	
		SetupClassOptions( true, setWeapon );

		sendInfo = true;
	}

	if ( sendInfo ) {
		SendClassInfo( false );
	}
}

/*
==============
sdInventory::IsIdealWeaponValid
==============
*/
bool sdInventory::IsIdealWeaponValid( void ) const {
	return IsWeaponValid( idealWeapon );
}

/*
==============
sdInventory::IsCurrentWeaponValid
==============
*/
bool sdInventory::IsCurrentWeaponValid( void ) const {
	return IsWeaponValid( currentWeapon );
}

/*
==============
sdInventory::GetCurrentItem
==============
*/
const sdDeclInvItem* sdInventory::GetCurrentItem( void ) const {
	if ( currentWeapon < 0 || currentWeapon >= items.Num() ) {
		return NULL;
	}

	if ( !CanEquip( currentWeapon, false ) ) {
		return NULL;
	}

	return items[ currentWeapon ].GetItem();
}

/*
==============
sdInventory::GetCurrentWeaponName
==============
*/
const char* sdInventory::GetCurrentWeaponName( void ) {
	const sdDeclInvItem* item = GetCurrentItem();

	if ( !item ) {
		return NULL;
	}

	return item->GetData().GetString( "anim_prefix" );
}

/*
==============
sdInventory::GetCurrentWeaponClass
==============
*/
const char* sdInventory::GetCurrentWeaponClass( void ) {
	const sdDeclInvItem* item = GetCurrentItem();

	if ( !item ) {
		return NULL;
	}

	return item->GetData().GetString( "anim_prefix_class" );
}

/*
==============
sdInventory::ClearAmmo
==============
*/
void sdInventory::ClearAmmo( void ) {
	for ( int i = 0; i < ammo.Num(); i++ ) {
		ammo[ i ]		= 0;
		ammoLimits[ i ] = 0;
	}
}

/*
==============
sdInventory::GiveClass
==============
*/
bool sdInventory::GiveClass( const sdDeclPlayerClass* cls, bool sendInfo ) {
	if ( gameLocal.isClient ) {
		assert( false );
		return false;
	}

 	if ( cls == NULL ) {
		gameLocal.Error( "sdInventory::GiveClass: NULL player class" );
		return false;
	}

	if ( cls->GetPackage() == NULL ) {
		gameLocal.Error( "sdInventory::GiveClass: NULL package on player class '%s'", cls->GetName() );
		return false;
	}

	const sdTeamInfo* otherTeam = cls->GetTeam();
	if ( otherTeam != NULL && otherTeam != owner->GetGameTeam() ) {
		return false;
	}

	if ( playerClass.GetClass() == cls ) {
		return false;
	}

	idEntity* proxy = owner->GetProxyEntity();
	if ( proxy != NULL ) {
		proxy->GetUsableInterface()->OnExit( owner, true );
	}

	SetPlayerClass( cls );
	SelectBestWeapon( true );

	if ( sendInfo ) {
		SendClassInfo( false );
	}

	return true;
}

/*
==============
sdInventory::SendClassInfo
==============
*/
void sdInventory::SendClassInfo( bool cached ) {
	if ( !gameLocal.isServer ) {
		return;
	}

	sdPlayerClassSetup& setup = cached ? cachedPlayerClass : playerClass;

	const sdDeclPlayerClass* cls = setup.GetClass();

	sdEntityBroadcastEvent msg( owner, cached ? idPlayer::EVENT_SETCACHEDCLASS : idPlayer::EVENT_SETCLASS );
	msg.WriteBits( cls ? cls->Index() + 1 : 0, gameLocal.GetNumPlayerClassBits() );
	if ( cls ) {
		for ( int i = 0; i < cls->GetNumOptions(); i++ ) {
			const sdDeclPlayerClass::optionList_t& option = cls->GetOption( i );
			msg.WriteBits( setup.GetOptions()[ i ], idMath::BitsForInteger( option.Num() ) );
		}
	}
	msg.Send( true, sdReliableMessageClientInfoAll() );
}

/*
==============
sdInventory::SetupClassThread
==============
*/
void sdInventory::SetupClassThread( void ) {
	ShutdownClassThread();
	if ( playerClass.GetClass() ) {
		const char* classThreadName = playerClass.GetClass()->GetClassThreadName();
		if ( *classThreadName ) {
			classThread = gameLocal.program->CreateThread();
			classThread->SetName( idStr( owner->GetName() ) + "_classThread" );
			classThread->CallFunction( owner->scriptObject, owner->scriptObject->GetFunction( classThreadName ) );
			classThread->ManualDelete();
			classThread->ManualControl();
			classThread->DelayedStart( 0 );
		}
	}
}

/*
==============
sdInventory::ClearClass
==============
*/
void sdInventory::ClearClass( void ) {
	SetPlayerClass( NULL );
	SendClassInfo( false );
}

/*
==============
sdInventory::LogClassTime
==============
*/
void sdInventory::LogClassTime( void ) {
	const sdDeclPlayerClass* pc = playerClass.GetClass();
	if ( pc != NULL ) {
		const sdDeclPlayerClass::stats_t& stats = pc->GetStats();
		if ( stats.timePlayed ) {
			int t = MS2SEC( gameLocal.time - timeClassChanged );
			stats.timePlayed->IncreaseValue( owner->entityNumber, t );
		}
	}
	timeClassChanged = gameLocal.time;
}

/*
==============
sdInventory::SetPlayerClass
==============
*/
void sdInventory::SetPlayerClass( const sdDeclPlayerClass* cls ) {
	LogClassTime();

	if ( gameLocal.isServer ) {
		owner->DropDisguise();
	}

	SetCachedClass( NULL );

	playerClass.SetClass( cls, true );

	SetupClassOptions( true, false );
	SetupClassThread();


	sdScriptHelper h1;
	owner->CallFloatNonBlockingScriptEvent( owner->GetScriptObject()->GetFunction( "OnClassChanged" ), h1 );

	SetupModel();

	owner->UpdateRating();

	UpdatePlayerClassInfo( cls ); //mal: update this clients class info!
}

/*
==============
sdInventory::SetClassOption
==============
*/
bool sdInventory::SetClassOption( int optionIndex, int itemIndex, bool sendInfo ) {
	if ( !playerClass.SetOption( optionIndex, itemIndex ) ) {
		return false;
	}

	idEntity* proxy = owner->GetProxyEntity();
	if ( proxy != NULL ) {
		proxy->GetUsableInterface()->OnExit( owner, true );
	}

	SetupClassOptions( true, true );

	if ( sendInfo ) {
		SendClassInfo( false );
	}

	return true;
}

/*
============
sdInventory::SetCachedClass
============
*/
bool sdInventory::SetCachedClass( const sdDeclPlayerClass* pc, bool sendInfo ) {
	bool changed = cachedPlayerClass.SetClass( pc );

	if ( changed && sendInfo ) {
		SendClassInfo( true );
	}

	return changed;
}

/*
============
sdInventory::SetCachedClassOption
============
*/
bool sdInventory::SetCachedClassOption( int optionIndex, int itemIndex, bool sendInfo ) {
	if ( optionIndex < 0 || optionIndex >= cachedPlayerClass.GetOptions().Num() ) {
		return false;
	}

	bool changed = cachedPlayerClass.SetOption( optionIndex, itemIndex );

	if ( changed && sendInfo ) {
		SendClassInfo( true );
	}

	return changed;
}

/*
==============
sdInventory::AddItems
==============
*/
bool sdInventory::AddItems( const sdDeclItemPackage* package, bool enabled ) {
	const sdDeclItemPackageNode& node = package->GetItemRoot();
	return AddItemNode( node, enabled, false );
}

/*
==============
sdInventory::CheckItems
==============
*/
bool sdInventory::CheckItems( const sdDeclItemPackage* package ) {
	const sdDeclItemPackageNode& node = package->GetItemRoot();
	return AddItemNode( node, true, true );
}

/*
==============
sdInventory::AddItemNode
==============
*/
bool sdInventory::AddItemNode( const sdDeclItemPackageNode& node, bool enabled, bool testOnly ) {
	bool added = false;

	if ( !node.GetRequirements().Check( owner ) ) {
		enabled = false;
	}

	for ( int i = 0; i < node.GetItems().Num(); i++ ) {
		const sdDeclInvItem* item = node.GetItems()[ i ];
		if ( !testOnly ) {
			items.AddItem( item, enabled );
		}
		added |= enabled;
	}

	for ( int i = 0; i < node.GetNodes().Num(); i++ ) {
		added |= AddItemNode( *node.GetNodes()[ i ], enabled, testOnly );
	}

	return added;
}

/*
==============
sdInventory::SetupClassOptions
==============
*/
void sdInventory::SetupClassOptions( bool clearAmmo, bool setWeapon, bool allowCurrentWeapon ) {
	bool disguise = owner->IsDisguised();
	if ( disguise ) {
		items.Clear();

		const sdDeclPlayerClass* cls = owner->GetDisguiseClass();
		if ( cls == NULL ) {
			return;
		}

		AddItems( cls->GetDisguisePackage() );
	} else { 

		if ( clearAmmo ) {
			ClearAmmo();
		}

		items.Clear();

		const sdDeclPlayerClass* cls = playerClass.GetClass();
		if ( cls == NULL ) {
			return;
		}

		if ( clearAmmo ) {
			for ( int i = 0; i < ammoLimits.Num(); i++ ) {
				ammoLimits[ i ] = cls->GetAmmoLimit( i );
			}
		}

		const sdDeclItemPackage* package = cls->GetPackage();
		AddItems( package );
		if ( clearAmmo && !disguise ) {
			GiveConsumables( package );
		}

		for ( int i = 0; i < playerClass.GetOptions().Num(); i++ ) {
			const sdDeclPlayerClass::optionList_t& list = cls->GetOption( i );

			int index = playerClass.GetOptions()[ i ];
			if ( index < 0 || index >= list.Num() ) {
				index = 0;
			}

			if ( !CheckItems( list[ index ] ) ) {
				index = 0;
			}

			for ( int j = 0; j < list.Num(); j++ ) {
				AddItems( list[ j ], j == index );
			}

			if ( clearAmmo ) {
				GiveConsumables( list[ index ] );
			}
		}
	}

	SortClips();
	if ( !gameLocal.isClient && setWeapon ) {
		SelectBestWeapon( allowCurrentWeapon );
	}
}

/*
==============
sdInventory::HasAbility
==============
*/
bool sdInventory::HasAbility( qhandle_t handle ) const {
	const sdDeclPlayerClass* cls = playerClass.GetClass();
	return cls && cls->HasAbility( handle );
}

/*
==============
sdInventory::SortClips
==============
*/
void sdInventory::SortClips( void ) {
	for ( int i = 0; i < items.Num(); i++ ) {
		const sdDeclInvItem* item = GetItem( i );
		for( int clip = 0; clip < item->GetClips().Num(); clip++ ) {
			const itemClip_t& clipInfo = item->GetClips()[ clip ];
			
			if ( clipInfo.maxAmmo > 0 && clipInfo.ammoPerShot > 0 ) {
				int max = Min( clipInfo.maxAmmo, GetAmmo( clipInfo.ammoType ) );
				SetClip( i, clip, max );
			}
		}
	}
}

/*
==============
sdInventory::GiveConsumables
==============
*/
bool sdInventory::GiveConsumablesNode( const sdDeclItemPackageNode& node ) {
	if ( !node.GetRequirements().Check( owner ) ) {
		return false;
	}

	bool given = false;

	idList< const sdConsumable* > consumables = node.GetConsumables();
	for ( int i = 0; i < consumables.Num(); i++ ) {
		if ( !consumables[ i ]->Give( owner ) ) {
			continue;
		}
		given = true;
	}

	for ( int i = 0; i < node.GetNodes().Num(); i++ ) {
		given |= GiveConsumablesNode( *node.GetNodes()[ i ] );
	}

	return given;
}

/*
==============
sdInventory::GiveConsumables
==============
*/
bool sdInventory::GiveConsumables( const sdDeclItemPackage* package ) {
	return GiveConsumablesNode( package->GetItemRoot() );
}

/*
==============
sdInventory::GetWeaponTitle
==============
*/
const sdDeclLocStr* sdInventory::GetWeaponTitle( void ) const {
	if( IsSwitchActive() ) {
		return items[ switchingWeapon ].item->GetItemName();
	}

	if ( !IsCurrentWeaponValid() ) {
		return NULL;
	}

	return items[ currentWeapon ].item->GetItemName();
}

/*
==============
sdInventory::GetWeaponName
==============
*/
const char* sdInventory::GetWeaponName( void ) const {
	if( IsSwitchActive() ) {
		return items[ switchingWeapon ].item->GetName();
	}

	if ( !IsCurrentWeaponValid() ) {
		return "";
	}

	return items[ currentWeapon ].item->GetName();
}

/*
============
sdInventory::CheckWeaponHasAmmo
============
*/
bool sdInventory::CheckWeaponHasAmmo( const sdDeclInvItem* item ) const {
	const idList< itemClip_t >& itemClips = item->GetClips();

	if ( !itemClips.Num() ) {
		return true;
	}

	for ( int i = 0; i < itemClips.Num(); i++ ) {
		if ( GetAmmo( itemClips[ i ].ammoType ) >= itemClips[ i ].ammoPerShot ) {
			return true;
		}
	}
	return false;
}

/*
============
sdInventory::GetAmmo
============
*/
int sdInventory::GetAmmo( int index ) const {
	return ammo[ index ];
}

/*
==============
sdInventory::ShutdownClassThread
==============
*/
void sdInventory::ShutdownClassThread( void ) {
	if ( classThread != NULL ) {
		gameLocal.program->FreeThread( classThread );
		classThread = NULL;
	}
}

/*
============
sdInventory::CancelWeaponSwitch
============
*/
void sdInventory::CancelWeaponSwitch( void ) {
	switchingWeapon = -1;
}


/*
============
sdInventory::AcceptWeaponSwitch
============
*/
void sdInventory::AcceptWeaponSwitch( void ) {
	if ( CanEquip( switchingWeapon, true ) ) {
		SetIdealWeapon( switchingWeapon );
		switchingWeapon = -1;
	}
}

/*
============
sdInventory::UpdateCurrentWeapon
============
*/
void sdInventory::UpdateCurrentWeapon( void ) {
	if ( currentWeapon != idealWeapon ) {
		if ( currentWeapon >= 0 && currentWeapon < items.Num() ) {
			if ( items.ShowItem( items[ currentWeapon ] ) ) {
				items[ currentWeapon ].SetVisible( true );
			}
		}

		currentWeapon = idealWeapon;

		if ( currentWeapon >= 0 && currentWeapon < items.Num() ) {
			items[ currentWeapon ].SetVisible( false );
			items.HideItem( items[ currentWeapon ] );
		}
	}

	weaponChanged = false;
}

/*
============
sdInventory::ApplyNetworkState
============
*/
void sdInventory::ApplyNetworkState( const sdInventoryBroadcastData& newData ) {
	if ( newData.idealWeapon >= -1 && newData.idealWeapon < items.Num() ) {
		if ( newData.idealWeapon == -1 || newData.idealWeapon != idealWeapon || ( currentWeaponIndex != items[ newData.idealWeapon ].item->Index() ) ) {
			SetIdealWeapon( newData.idealWeapon, true );
		}
	}

	assert( sdBitField_Dynamic::SizeForBits( items.Num() ) == newData.disabledMask.GetSize() );
	for ( int i = 0; i < items.Num(); i++ ) {
		items[ i ].SetDisabled( newData.disabledMask.Get( i ) != 0 );
	}

	if ( !owner->ShouldReadPlayerState() ) {
		assert( sdBitField_Dynamic::SizeForBits( gameLocal.declAmmoTypeType.Num() ) == newData.ammoMask.GetSize() );
		for ( int i = 0; i < gameLocal.declAmmoTypeType.Num(); i++ ) {
			ammo[ i ] = ( newData.ammoMask[ i ] != 0 ) ? 999 : 0;
		}
	}

	owner->GetProficiencyTable().ApplyNetworkState( newData.proficiencyData );
}

/*
============
sdInventory::ReadNetworkState
============
*/
void sdInventory::ReadNetworkState( const sdInventoryBroadcastData& baseData, sdInventoryBroadcastData& newData, const idBitMsg& msg ) const {
	newData.idealWeapon				= msg.ReadDeltaLong( baseData.idealWeapon );

	newData.disabledMask.Init( items.Num() );
	for ( int i = 0; i < newData.disabledMask.GetSize(); i++ ) {
		if ( i < baseData.disabledMask.GetSize() ) {
			newData.disabledMask.GetDirect( i ) = msg.ReadDeltaLong( baseData.disabledMask.GetDirect( i ) );
		} else {
			newData.disabledMask.GetDirect( i ) = msg.ReadLong();
		}
	}

	if ( !owner->ShouldReadPlayerState() ) {
		newData.ammoMask.Init( gameLocal.declAmmoTypeType.Num() );
		for ( int i = 0; i < newData.ammoMask.GetSize(); i++ ) {
			if ( i < baseData.ammoMask.GetSize() ) {
				newData.ammoMask.GetDirect( i ) = msg.ReadDeltaLong( baseData.ammoMask.GetDirect( i ) );
			} else {
				newData.ammoMask.GetDirect( i ) = msg.ReadLong();
			}
		}
	} else {
		newData.ammoMask.SetSize( baseData.ammoMask.GetSize() );
		for ( int i = 0; i < newData.ammoMask.GetSize(); i++ ) {
			newData.ammoMask.GetDirect( i ) = baseData.ammoMask.GetDirect( i );
		}
	}

	owner->GetProficiencyTable().ReadNetworkState( baseData.proficiencyData, newData.proficiencyData, msg );
}

/*
============
sdInventory::WriteNetworkState
============
*/
void sdInventory::WriteNetworkState( const sdInventoryBroadcastData& baseData, sdInventoryBroadcastData& newData, idBitMsg& msg ) const {
	newData.idealWeapon				= idealWeapon;

	msg.WriteDeltaLong( baseData.idealWeapon, newData.idealWeapon );

	newData.disabledMask.Init( items.Num() );
	for ( int i = 0; i < items.Num(); i++ ) {
		if ( items[ i ].IsDisabled() ) {
			newData.disabledMask.Set( i );
		} else {
			newData.disabledMask.Clear( i );
		}
	}

	for ( int i = 0; i < newData.disabledMask.GetSize(); i++ ) {
		if ( i < baseData.disabledMask.GetSize() ) {
			msg.WriteDeltaLong( baseData.disabledMask.GetDirect( i ), newData.disabledMask.GetDirect( i ) );
		} else {
			msg.WriteLong( newData.disabledMask.GetDirect( i ) );
		}
	}

	if ( !owner->ShouldWritePlayerState() ) {
		int ammoCount = gameLocal.declAmmoTypeType.Num();
		newData.ammoMask.Init( ammoCount );
		for ( int i = 0; i < ammoCount; i++ ) {
			if ( ammo[ i ] != 0 ) {
				newData.ammoMask.Set( i );
			} else {
				newData.ammoMask.Clear( i );
			}
		}

		for ( int i = 0; i < newData.ammoMask.GetSize(); i++ ) {
			if ( i < baseData.ammoMask.GetSize() ) {
				msg.WriteDeltaLong( baseData.ammoMask.GetDirect( i ), newData.ammoMask.GetDirect( i ) );
			} else {
				msg.WriteLong( newData.ammoMask.GetDirect( i ) );
			}
		}
	} else {
		newData.ammoMask.SetSize( baseData.ammoMask.GetSize() );
		for ( int i = 0; i < newData.ammoMask.GetSize(); i++ ) {
			newData.ammoMask.GetDirect( i ) = baseData.ammoMask.GetDirect( i );
		}
	}

	owner->GetProficiencyTable().WriteNetworkState( baseData.proficiencyData, newData.proficiencyData, msg );
}

/*
============
sdInventory::CheckNetworkStateChanges
============
*/
bool sdInventory::CheckNetworkStateChanges( const sdInventoryBroadcastData& baseData ) const {
	NET_CHECK_FIELD( idealWeapon, idealWeapon );

	if ( sdBitField_Dynamic::SizeForBits( items.Num() ) != baseData.disabledMask.GetSize() ) {
		return true;
	}

	for ( int i = 0; i < items.Num(); i++ ) {
		if ( ( baseData.disabledMask.Get( i ) != 0 ) != items[ i ].IsDisabled() ) {
			return true;
		}
	}

	if ( !owner->ShouldWritePlayerState() ) {
		if ( baseData.ammoMask.GetSize() != sdBitField_Dynamic::SizeForBits( gameLocal.declAmmoTypeType.Num() ) ) {
			return true;
		}

		for ( int i = 0; i < gameLocal.declAmmoTypeType.Num(); i++ ) {
			if ( ( baseData.ammoMask.Get( i ) != 0 ) != ( ammo[ i ] != 0 ) ) {
				return true;
			}
		}
	}

	return owner->GetProficiencyTable().CheckNetworkStateChanges( baseData.proficiencyData );
}

/*
============
sdInventory::ApplyPlayerState
============
*/
void sdInventory::ApplyPlayerState( const sdInventoryPlayerStateData& newState ) {
	NET_GET_NEW( sdInventoryPlayerStateData );

	for ( int i = 0; i < newData.ammo.Num(); i++ ) {
		SetAmmo( i, newData.ammo[ i ] );
	}

	items.ApplyPlayerState( newData );
}

/*
============
sdInventory::ReadPlayerState
============
*/
void sdInventory::ReadPlayerState( const sdInventoryPlayerStateData& baseState, sdInventoryPlayerStateData& newState, const idBitMsg& msg ) const {
	NET_GET_STATES( sdInventoryPlayerStateData );

	newData.ammo.SetNum( gameLocal.declAmmoTypeType.Num() );
	for ( int i = 0; i < newData.ammo.Num(); i++ ) {
		newData.ammo[ i ] = msg.ReadDeltaShort( baseData.ammo[ i ] );
	}

	items.ReadPlayerState( baseState, newData, msg );
}

/*
============
sdInventory::WritePlayerState
============
*/
void sdInventory::WritePlayerState( const sdInventoryPlayerStateData& baseState, sdInventoryPlayerStateData& newState, idBitMsg& msg ) const {
	NET_GET_STATES( sdInventoryPlayerStateData );

	newData.ammo.SetNum( gameLocal.declAmmoTypeType.Num() );
	for ( int i = 0; i < newData.ammo.Num(); i++ ) {
		newData.ammo[ i ] = GetAmmo( i );
		msg.WriteDeltaShort( baseData.ammo[ i ], newData.ammo[ i ] );
	}

	items.WritePlayerState( baseState, newState, msg );
}

/*
============
sdInventory::CheckPlayerStateChanges
============
*/
bool sdInventory::CheckPlayerStateChanges( const sdInventoryPlayerStateData& baseState ) const {
	NET_GET_BASE( sdInventoryPlayerStateData );

	for ( int i = 0; i < baseData.ammo.Num(); i++ ) {
		if ( baseData.ammo[ i ] != GetAmmo( i ) ) {
			return true;
		}
	}

	return items.CheckPlayerStateChanges( baseState );
}


/*
============
sdInventory::HideCurrentItem
============
*/
void sdInventory::HideCurrentItem( bool hide ) {
	if( currentWeapon != -1 ) {
		if ( hide ) {
			items[ currentWeapon ].SetVisible( false );
			items.HideItem( items[ currentWeapon ] );
		} else {
			if ( items.ShowItem( items[ currentWeapon ] ) ) {
				items[ currentWeapon ].SetVisible( true );
			}
		}
	}
}

/*
============
sdInventory::SetClass
============
*/
bool sdInventory::SetClass( const idBitMsg& msg, bool cached ) {
	sdPlayerClassSetup& setup = cached ? cachedPlayerClass : playerClass;

	int playerClassIndex = msg.ReadBits( gameLocal.GetNumPlayerClassBits() ) - 1;
	const sdDeclPlayerClass* pc = NULL;
	if ( playerClassIndex != -1 ) {
		pc = gameLocal.declPlayerClassType[ playerClassIndex ];
	}

	if ( setup.GetClass() != pc ) {
		if ( cached ) {
			SetCachedClass( pc );
		} else {
			SetPlayerClass( pc );
		}
	}

	if ( pc ) {
		bool changed = false;
		for ( int i = 0; i < pc->GetNumOptions(); i++ ) {
			int itemIndex = msg.ReadBits( idMath::BitsForInteger( pc->GetOption( i ).Num() ) );
			if ( setup.GetOptions()[ i ] != itemIndex ) {
				setup.SetOption( i, itemIndex );
				changed = true;
			}
		}
		if ( changed && !cached ) {
			SetupClassOptions( true, false );
		}
	}

	return true;
}

/*
============
sdInventory::UpdateForDisguise
============
*/
void sdInventory::UpdateForDisguise( void ) {
	SetupClassOptions( false, true, false );
	SetupModel();
}

/*
============
sdInventory::OnHide
============
*/
void sdInventory::OnHide( void ) {
	items.OnHide();
}

/*
============
sdInventory::OnShow
============
*/
void sdInventory::OnShow( void ) {
	items.OnShow();
}

/*
============
sdInventory::LogSuicide
============
*/
void sdInventory::LogSuicide( void ) {
	const sdDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const sdDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.suicides ) {
		stats.suicides->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
sdInventory::LogDeath
============
*/
void sdInventory::LogDeath( void ) {
	const sdDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const sdDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.deaths != NULL ) {
		stats.deaths->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
sdInventory::LogRevive
============
*/
void sdInventory::LogRevive( void ) {
	const sdDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const sdDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.revived ) {
		stats.revived->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
sdInventory::LogTapOut
============
*/
void sdInventory::LogTapOut( void ) {
	const sdDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const sdDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.tapouts ) {
		stats.tapouts->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
sdInventory::LogRespawn
============
*/
void sdInventory::LogRespawn( void ) {
	const sdDeclPlayerClass* pc = playerClass.GetClass();
	if ( !pc ) {
		return;
	}

	const sdDeclPlayerClass::stats_t& stats = pc->GetStats();
	if ( stats.respawns != NULL ) {
		stats.respawns->IncreaseValue( owner->entityNumber, 1 );
	}
}

/*
============
sdInventory::UpdatePlayerClassInfo

update the players class info game side for the bots.
============
*/
void sdInventory::UpdatePlayerClassInfo( const sdDeclPlayerClass* pc ) {

 	if ( !pc ) {
		return;
	}
	
	botThreadData.GetGameWorldState()->clientInfo[ owner->entityNumber ].classType = pc->GetPlayerClassNum();
}
