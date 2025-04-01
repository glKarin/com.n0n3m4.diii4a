/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#ifndef __DARKMOD_INVENTORYWEAPONITEM_H__
#define __DARKMOD_INVENTORYWEAPONITEM_H__

#include "InventoryItem.h"

/**
 * WeaponInventoryItem is an item that belongs to a group. This item represents
 * a weapon entityDef and provides some methods to manage the weapon's ammo.
 */
class CInventoryWeaponItem :
	public CInventoryItem
{
protected:
	// The name of the weapon entityDef
	idStr	m_WeaponDefName;

	// The unique name for this weapon, e.g. "broadhead" or "blackjack"
	idStr	m_WeaponName;

	// The name of the projectile def name for this weapon (can change during runtime). 
	// Can be empty for melee weapons.
	idStr	m_ProjectileDefName;

	// The maximum amount of ammo for this weapon
	int		m_MaxAmmo;

	// The current amount of ammonition (set to getStartAmmo() in constructor)
	int		m_Ammo;

	// The index of this weapon [0..INF)
	int		m_WeaponIndex;

	// TRUE, if this weapon doesn't need ammo (like shortsword, blackjack)
	bool	m_AllowedEmpty;

	// TRUE if toggling this weapon is allowed (i.e. selecting it when it is already selected)
	bool	m_IsToggleable;

	// TRUE if this weapon can be selected.
	bool	m_Enabled;

public:
	// Default constructor, should only be used during restoring from savegames
	CInventoryWeaponItem();

	CInventoryWeaponItem(const idStr& weaponDefName, idEntity* owner);

	virtual void	Save( idSaveGame *savefile ) const override;
	virtual void	Restore(idRestoreGame *savefile) override;

	// TRUE if this weapon is enabled
	bool IsEnabled() const;
	void SetEnabled(bool enabled);

	// Retrieves the maximum amount of ammo this weapon can hold
	int GetMaxAmmo();
	// Retrives the amount of ammo at player spawn time
	int GetStartAmmo();

	// Returns TRUE if this weapon doesn't need ammo and therefore can be selected 
	bool IsAllowedEmpty() const;

	// Convenience method
	bool NeedsAmmo() const { return !IsAllowedEmpty(); };

	// Returns TRUE if this weapon is toggleable
	bool IsToggleable() const;

	// Returns the currently available ammonition
	int GetAmmo() const;
	// Sets the new ammonition value (is automatically clamped to [0,maxAmmo])
	void SetAmmo(int newAmount);

	/**
 	 * This is used to check whether a weapon can "fire". This is always "1" for 
	 * weapons without ammo (sword, blackjack). For all other weapons, the ammo amount
	 * is returned.
	 */
	int HasAmmo();

	/**
	 * "Uses" a certain <amount> of ammo. This decreases the current ammo counter
	 * by the given value. Only affects the ammo count of weapons that actually need ammo.
	 */
	void UseAmmo(int amount);

	// Sets/Returns the weapon index (corresponds to the keyboard number keys used to access the weapons)
	void SetWeaponIndex(int index);
	int  GetWeaponIndex() const;

	/**
	 * greebo: Returns the name of the weapon, as derived from the weaponDef name.
	 *         entityDef "weapon_broadhead" => weapon name: "broadhead"
	 */
	const idStr& GetWeaponName() const;

	/**
	 * greebo: Returns the name of the projectile entityDef, if this weapon fires projectiles in the first place.
	 * Melee weapons will return an empty string.
	 */
	const idStr& GetProjectileDefName() const;
	void SetProjectileDefName(const idStr& weaponDefName);

	// Restores the projectile def name as originally defined in the weapon def
	void ResetProjectileDefName();

	// Returns the name of the weapon entityDef
	const idStr& GetWeaponDefName() const;

	// Override CInventoryItem::SaveItemEntityDict(), as weapons don't have entities behind them but still need to have a dict
	virtual void SaveItemEntityDict() override;

	// Override CInventoryItem::RestoreItemEntityFromDict, don't do anything but clear the saved dict
	virtual void RestoreItemEntityFromDict(const idVec3& entPosition) override;
};
typedef std::shared_ptr<CInventoryWeaponItem> CInventoryWeaponItemPtr;

#endif /* __DARKMOD_INVENTORYWEAPONITEM_H__ */
