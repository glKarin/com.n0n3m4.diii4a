// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLINVITEM_H__
#define __DECLINVITEM_H__

class sdRequirementContainer;
class sdRequirement;
class idEntity;
class sdDeclInvItemType;
class idRenderModel;
class sdDeclToolTip;

#include "../roles/RoleManager.h"

typedef struct itemClip_s {
	ammoType_t						ammoType;
	int								maxAmmo;
	int								ammoPerShot;
	const idDeclEntityDef*			projectile;
	idStr							clientProjectile;
	int								lowAmmo;
} itemClip_t;

class sdDeclInvItem : public idDecl {
public:
									sdDeclInvItem( void );
	virtual							~sdDeclInvItem( void );

	virtual const char*				DefaultDefinition( void ) const;
	virtual bool					Parse( const char *text, const int textLength );
	virtual void					FreeData( void );

	bool							ParseClip( idParser& src );

	bool							UsesSlot( int index ) const { return slot == index ; }
	bool							operator== ( const sdDeclInvItem& other ) const { return Index() == other.Index(); }
	int								GetSlot() const { return slot; }
	
	const sdRequirementContainer&	GetUsageRequirements( void ) const { return usageRequirements; }
	const idDict&					GetData( void ) const { return data; }

	const sdDeclInvItemType*		GetType( void ) const { return type; }
	idRenderModel*					GetModel( void ) const { return model; }
	const sdDeclLocStr*				GetItemName( void ) const { return name; }
	const char*						GetJoint( void ) const { return joint; }
	const sdDeclToolTip*			GetToolTip( void ) const { return tooltipSelect; }
	int								GetAutoSwitchPriority( void ) const { return autoSwitchPriority; }
	bool							GetAutoSwitchIsExplosive( void ) const { return autoSwitchExplosive; }
	bool							GetWeaponMenuIgnore( void ) const { return weaponMenuIgnore; }
	bool							GetSelectWhenEmpty( void ) const { return selectWhenEmpty; }
	bool							GetWeaponChangeAllowed( void ) const { return weaponChangeAllowed; }
	bool							GetIgnoreViewPitch( void ) const { return ignoreViewPitch; }
	bool							GetAllowProne( void ) const { return allowProne; }

	const idList< itemClip_t >&		GetClips( void ) const { return clips; }

protected:
	const sdDeclLocStr*				name;
	idStr							joint;
	const sdDeclToolTip*			tooltipSelect;

	idRenderModel*					model;
	
	const sdDeclInvItemType*		type;

	idList< itemClip_t >			clips;

	int								slot;
	sdRequirementContainer			usageRequirements;

	int								autoSwitchPriority;
	bool							allowProne;
	bool							autoSwitchExplosive;
	bool							weaponMenuIgnore;
	bool							selectWhenEmpty;

	bool							weaponChangeAllowed;
	bool							ignoreViewPitch;

	idDict							data;
};

#endif // __DECLINVITEM_H__

