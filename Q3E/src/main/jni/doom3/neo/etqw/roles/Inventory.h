// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ROLES_INVENTORY_H__
#define __GAME_ROLES_INVENTORY_H__

#include "../misc/RenderEntityBundle.h"

class idEntity;
class sdRequirement;
class idPlayer;
class sdInventory;
class sdInventoryManager;
class sdDeclItemPackage;
class sdDeclInvItem;

class sdItemPoolEntry {
public:
										sdItemPoolEntry( void );

	void								SetItem( const sdDeclInvItem* );
	const sdDeclInvItem*				GetItem( void ) const { return item; }

	void								SetVisible( bool visible ) { flags.hidden = !visible; }
	bool								IsVisible( void ) const { return !flags.hidden; }

	void								SetDisabled( bool disabled ) { flags.disabled = disabled; }
	bool								IsDisabled( void ) const { return flags.disabled; }

	void								ShowModel( void ) { renderEntity.Show(); }
	void								HideModel( void ) { renderEntity.Hide(); }
	void								UpdateModel( void ) { renderEntity.Update(); }

	renderEntity_t&						GetModel( void ) { return renderEntity.GetEntity(); }

public:
	idList< int >						clips;
	const sdDeclInvItem*				item;
	jointHandle_t						joint;

private:
	typedef struct flags_s {
		bool							disabled	: 1;
		bool							hidden		: 1;
	} flags_t;
	
	sdRenderEntityBundle				renderEntity;

	flags_t								flags;
};

class sdInventoryPlayerStateData;

class sdPlayerClassSetup {
public:
								sdPlayerClassSetup( void );

	void						operator=( const sdPlayerClassSetup& rhs );

	const sdDeclPlayerClass*	GetClass( void ) const { return playerClass; }
	const idList< int >&		GetOptions( void ) const { return playerClassOptions; }

	void						SetOptions( const idList< int >& options );
	bool						SetClass( const sdDeclPlayerClass* pc, bool force = false );
	bool						SetOption( int index, int itemIndex );
	int							GetOption( int index ) const;

private:
	const sdDeclPlayerClass*	playerClass;
	idList< int >				playerClassOptions;
};

class sdUpgradeItemPool {
public:
								~sdUpgradeItemPool( void );

	void						Clear( void );
	void						Init( sdInventory* _parent );
	int							AddItem( const sdDeclInvItem* item, bool enabled );

	const sdItemPoolEntry&		operator[] ( int index ) const { return items[ index ]; }
	sdItemPoolEntry&			operator[] ( int index ) { return items[ index ]; }

	void						UpdateModels( void );
	void						UpdateModelShadows( void );
	void						SetupModelShadows( sdItemPoolEntry& entry );

	void						ApplyPlayerState( const sdInventoryPlayerStateData& newState );
	void						ReadPlayerState( const sdInventoryPlayerStateData& baseState, sdInventoryPlayerStateData& newState, const idBitMsg& msg ) const;
	void						WritePlayerState( const sdInventoryPlayerStateData& baseState, sdInventoryPlayerStateData& newState, idBitMsg& msg ) const;
	bool						CheckPlayerStateChanges( const sdInventoryPlayerStateData& baseState ) const;

	int							Num( void ) const { return items.Num(); }
	
	void						UpdateJoints( void );

	void						OnHide( void );
	void						OnShow( void );

	bool						ShowItem( sdItemPoolEntry& item );
	void						HideItem( sdItemPoolEntry& item );

private:
	void						ClearItem( sdItemPoolEntry& entry );

	idList< sdItemPoolEntry >	items;
	idList< sdItemPoolEntry* >	modelItems;
	sdInventory*				parent;
};

class sdInventoryBroadcastData : public sdEntityStateNetworkData {
public:
								sdInventoryBroadcastData( void ) { ; }

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	int							idealWeapon;
	sdBitField_Dynamic			disabledMask;
	sdBitField_Dynamic			ammoMask; // Gordon: says whether the client has *any* ammo in this slot, will be sent for players you are not viewing, as you don't have full ammo information for them
	sdProficiencyTable::sdNetworkData proficiencyData;
};

class sdInventoryItemStateData {
public:
								sdInventoryItemStateData( void ) { ; }

	void						MakeDefault( void );

	void						Write( idFile* file ) const;
	void						Read( idFile* file );

	idList< short >				clips;
};

class sdInventoryPlayerStateData : public sdEntityStateNetworkData {
public:
								sdInventoryPlayerStateData( void ) { ; }

	virtual void				MakeDefault( void );

	virtual void				Write( idFile* file ) const;
	virtual void				Read( idFile* file );

	idList< short >						ammo;
	idList< sdInventoryItemStateData >	itemData;
};

class sdInventory {
public:
								sdInventory( void );
								~sdInventory( void );
	typedef enum iconType_e {
		IT_ICON, IT_CLASS, IT_UNKNOWN
	};

	void						Init( idDict& dict, bool full, bool setWeapon );

	void						WriteDemoBaseData( idFile* file ) const;
	void						ReadDemoBaseData( idFile* file );

	float						GetAmmoFraction( void );

	void						ShutdownThreads( void ) { ShutdownClassThread(); }

	int							GetMaxAmmo( int index ) const { return ammoLimits[ index ]; }
	int							GetAmmo( int index ) const;
	int							GetClip( int index, int modIndex ) const;
	int							GetClipSize( int index, int modIndex ) const;
	bool						GetWeaponChanged( void ) const { return weaponChanged; }
	int							GetCurrentWeapon( void ) const { return currentWeapon; }
	const sdDeclInvItem*		GetCurrentItem( void ) const;
	int							GetCurrentSlot( void ) const;
	int							GetSwitchingSlot( void ) const;
	int							GetIdealWeapon( void ) const { return idealWeapon; }
	int							GetSwitchingWeapon( void ) const { return switchingWeapon; }
	const char*					GetCurrentWeaponName( void );
	const char*					GetCurrentWeaponClass( void );
	const idMaterial*			GetCommandMapIcon( const iconType_e iconType ) const;
	const sdDeclLocStr*			GetWeaponTitle( void ) const;
	const char*					GetWeaponName( void ) const;
	sdUpgradeItemPool&			GetItemPool( void ) { return items; }
	const sdUpgradeItemPool&	GetItemPool( void ) const { return items; }
	idPlayer*					GetOwner( void );


	void						UpdatePlayerClassInfo( const sdDeclPlayerClass* pc ); //mal: update the players class/team info - called on every class change: sdInventory::SetPlayerClass

	const sdDeclPlayerClass*	GetClass( void ) const { return playerClass.GetClass(); }
	int							GetClassOption( int option ) const { return playerClass.GetOption( option ); }
	const idList< int >&		GetClassOptions( void ) const { return playerClass.GetOptions(); }
	const sdPlayerClassSetup*	GetClassSetup( void ) const { return &playerClass; }

	int							GetCachedClassOption( int option ) const { return cachedPlayerClass.GetOption( option ); }
	const idList< int >&		GetCachedClassOptions( void ) const { return cachedPlayerClass.GetOptions(); }
	const sdDeclPlayerClass*	GetCachedClass( void ) const { return cachedPlayerClass.GetClass(); }

	bool						CanEquip( int index, bool checkRequirements ) const;
	bool						CanAutoEquip( int index, bool checkExplosive ) const;

	void						SetOwner( idPlayer* _owner );
	void						SetMaxAmmo( int index, int count ) { ammoLimits[ index ] = count; }
	void						SetAmmo( int index, int count ) { ammo[ index ] = count; }
	void						SetClip( int index, int modIndex, int count );
	void						SelectWeaponByName( const char* weaponName, bool ignoreInhibit );
	void						SelectWeaponByNumber( const playerWeaponTypes_t weaponNum );
	int							FindWeaponNum( const playerWeaponTypes_t weaponNum );
	int							FindWeapon( const char* weaponName );
	void						SetIdealWeapon( int pos, bool force = false );
	void						SetSwitchingWeapon( int pos ) { switchingWeapon = pos; }
	void						SetWeaponChanged( void ) { weaponChanged = true; }
	bool						CheckWeaponHasAmmo( const sdDeclInvItem* item ) const;

	bool						CheckWeaponSlotHasAmmo( int slot );
	void						UpdatePrimaryWeapon();

	int							GetNumItems() const { return items.Num(); }
	const sdDeclInvItem*		GetItem( int index ) const { return index < 0 ? NULL : items[ index ].GetItem(); }
	void						HideCurrentItem( bool hide );
	bool						SetClass( const idBitMsg& msg, bool cached );

	void						CycleWeaponsNext( int currentSlot = -999 );
	void						CycleWeaponsPrev( int currentSlot = -999 );
	int							CycleWeaponByPosition( int pos, bool forward, bool& looped, bool force, bool primaryOnly );
	int							CycleWeaponBySlot( int slot, bool forward, bool& looped, bool force, int startingpos );
	void						CycleNextSafeWeapon( void );
	void						SelectBestWeapon( bool allowCurrent );
	int							FindBestWeapon( bool allowCurrent );

	bool						IsCurrentWeaponValid( void ) const;
	bool						IsIdealWeaponValid( void ) const;
	int							NumValidWeaponBanks() const;
	bool						IsWeaponBankValid( int bank ) const;	// 0-based weapon bank (0 is fists, 1 is pistol, etc)
	bool						IsWeaponValid( int weapon ) const;
	bool						IsSwitchActive() const { return switchingWeapon != -1; }

	idPlayer*					GetActivePlayer( void ) const;
	const sdDeclPlayerClass*	GetActivePlayerClass( void ) const;
	void						SetupModel( void );
	void						SetupModel( const sdDeclPlayerClass* cls );
	jointHandle_t				GetPlayerJoint( const idDict& dict, const char* name );
	void						CheckPlayerClass( bool setWeapon );
	bool						SetClassOption( int optionIndex, int itemIndex, bool sendInfo = true );

	static const idDeclSkin*	SkinForClass( const sdDeclPlayerClass* cls );

	void						ClearAmmo( void );

	void						UpdateCurrentWeapon( void );
	void						AcceptWeaponSwitch( void );
	void						CancelWeaponSwitch( void );
	void						UpdateItems( void );
	void						Present( void );


	bool						GiveClass( const sdDeclPlayerClass* cls, bool sendInfo );
	bool						GiveConsumables( const sdDeclItemPackage* item );
	void						SortClips( void );

	bool						HasAbility( qhandle_t handle ) const;

	void						ClearClass( void );

	bool						SetCachedClass( const sdDeclPlayerClass* pc, bool sendInfo = true );
	bool						SetCachedClassOption( int optionIndex, int itemIndex, bool sendInfo = true );
	void						SendClassInfo( bool cached );

	static void					BuildSlotBankLookup( void );

	void						ApplyNetworkState( const sdInventoryBroadcastData& newState );
	void						ReadNetworkState( const sdInventoryBroadcastData& baseState, sdInventoryBroadcastData& newState, const idBitMsg& msg ) const;
	void						WriteNetworkState( const sdInventoryBroadcastData& baseState, sdInventoryBroadcastData& newState, idBitMsg& msg ) const;
	bool						CheckNetworkStateChanges( const sdInventoryBroadcastData& baseState ) const;

	void						ApplyPlayerState( const sdInventoryPlayerStateData& newState );
	void						ReadPlayerState( const sdInventoryPlayerStateData& baseState, sdInventoryPlayerStateData& newState, const idBitMsg& msg ) const;
	void						WritePlayerState( const sdInventoryPlayerStateData& baseState, sdInventoryPlayerStateData& newState, idBitMsg& msg ) const;
	bool						CheckPlayerStateChanges( const sdInventoryPlayerStateData& baseState ) const;

	int							GetMaxBanks() const { return slotForBank.Num(); }
	int							GetSlotForWeapon( int weapon ) const;
	int							GetSlotForBank( int bank ) const { return slotForBank[ bank ]; }

	void						UpdateForDisguise( void );

	void						OnHide( void );
	void						OnShow( void );

	void						UpdateModelShadows( void );
	void						SetupModelShadows( sdItemPoolEntry& entry );

	bool						CheckItems( const sdDeclItemPackage* package );

	// Stats
	void						LogSuicide( void );
	void						LogDeath( void );
	void						LogRevive( void );
	void						LogTapOut( void );
	void						LogRespawn( void );
	void						LogClassTime( void );

private:
	int							ChooseCurrentSlot() const { return IsSwitchActive() ? GetSwitchingSlot() : GetCurrentSlot(); }	
	void						SetupClassOptions( bool clearAmmo, bool setWeapon, bool allowCurrentWeapon = true );
	void						SetPlayerClass( const sdDeclPlayerClass* cls );
	bool						AddItems( const sdDeclItemPackage* package, bool enabled = true );
	bool						AddItemNode( const sdDeclItemPackageNode& node, bool enabled, bool testOnly );
	bool						GiveConsumablesNode( const sdDeclItemPackageNode& node );
	void						ShutdownClassThread( void );
	void						SetupClassThread( void );

	void						SetupLocationalDamage( const idDict& dict );

	sdUpgradeItemPool			items;

	idPlayer*					owner;

	idList< int >				ammo;
	idList< int >				ammoLimits;

	int							currentWeapon;
	int							idealWeapon;
	int							switchingWeapon;
	int							currentWeaponIndex;

	int							timeClassChanged;

	bool						weaponChanged;

	sdProgramThread*			classThread;

	static idList< int >		slotForBank;

	sdPlayerClassSetup			playerClass;
	sdPlayerClassSetup			cachedPlayerClass;
};

#endif // __GAME_ROLES_INVENTORY_H__
