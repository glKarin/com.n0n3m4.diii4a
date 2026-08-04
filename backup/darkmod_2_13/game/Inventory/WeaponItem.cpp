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

#include "precompiled.h"
#pragma hdrstop

#pragma warning(disable : 4533 4800)



#include "WeaponItem.h"

#define WEAPON_START_AMMO_PREFIX "start_ammo_"
#define WEAPON_AMMO_REQUIRED "ammo_required"
#define WEAPON_MAX_AMMO "max_ammo"
#define WEAPON_IS_TOGGLEABLE "is_toggleable"

CInventoryWeaponItem::CInventoryWeaponItem() :
	CInventoryItem(NULL),
	m_WeaponDefName(""),
	m_MaxAmmo(0),
	m_Ammo(0),
	m_WeaponIndex(-1),
	m_AllowedEmpty(false),
	m_Enabled(true)
{
	SetType(IT_WEAPON);
}

CInventoryWeaponItem::CInventoryWeaponItem(const idStr& weaponDefName, idEntity* owner) :
	CInventoryItem(owner),
	m_WeaponDefName(weaponDefName),
	m_WeaponIndex(-1),
	m_AllowedEmpty(false),
	m_IsToggleable(false),
	m_Enabled(true)
{
	SetType(IT_WEAPON);

	const idDict* weaponDict = gameLocal.FindEntityDefDict(m_WeaponDefName);
	m_Name = weaponDict->GetString("inv_name", "Unknown weapon");

	m_WeaponName = weaponDict->GetString("inv_weapon_name");
	if (m_WeaponName.IsEmpty())
	{
		gameLocal.Warning("Weapon defined in %s has no 'inv_weapon_name' spawnarg!", m_WeaponDefName.c_str());
	}

	m_MaxAmmo = GetMaxAmmo();
	m_Ammo = GetStartAmmo();

	// Parse the common spawnargs which apply to both this and the base class
	ParseSpawnargs(*weaponDict);
}

void CInventoryWeaponItem::Save( idSaveGame *savefile ) const
{
	// Pass the call to the base class first
	CInventoryItem::Save(savefile);

	savefile->WriteString(m_WeaponDefName);
	savefile->WriteString(m_WeaponName);
	savefile->WriteString(m_ProjectileDefName);
	savefile->WriteInt(m_MaxAmmo);
	savefile->WriteInt(m_Ammo);
	savefile->WriteInt(m_WeaponIndex);
	savefile->WriteBool(m_AllowedEmpty);
	savefile->WriteBool(m_IsToggleable);
	savefile->WriteBool(m_Enabled);
}

void CInventoryWeaponItem::Restore( idRestoreGame *savefile )
{
	// Pass the call to the base class first
	CInventoryItem::Restore(savefile);

	savefile->ReadString(m_WeaponDefName);
	savefile->ReadString(m_WeaponName);
	savefile->ReadString(m_ProjectileDefName);
	savefile->ReadInt(m_MaxAmmo);
	savefile->ReadInt(m_Ammo);
	savefile->ReadInt(m_WeaponIndex);
	savefile->ReadBool(m_AllowedEmpty);
	savefile->ReadBool(m_IsToggleable);
	savefile->ReadBool(m_Enabled);
}

bool CInventoryWeaponItem::IsEnabled() const
{
	return m_Enabled;
}

void CInventoryWeaponItem::SetEnabled(bool enabled)
{
	m_Enabled = enabled;
}

int CInventoryWeaponItem::GetMaxAmmo()
{
	// Get the "max_ammo" spawnarg from the weapon dictionary
	const idDict* weaponDict = gameLocal.FindEntityDefDict(m_WeaponDefName,true); // grayman #3391 - don't create a default 'weaponDict'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if ( weaponDict == NULL )
	{
		return -1;
	}

	return weaponDict->GetInt(WEAPON_MAX_AMMO, "0");
}

bool CInventoryWeaponItem::IsAllowedEmpty() const
{
	return m_AllowedEmpty;
}

bool CInventoryWeaponItem::IsToggleable() const
{
	return m_IsToggleable;
}

int CInventoryWeaponItem::GetStartAmmo()
{
	// Sanity check
	if (m_Owner.GetEntity() == NULL) {
		return -1;
	}

	// Construct the weapon name to retrieve the "start_ammo_mossarrow" string, for instance
	idStr key = WEAPON_START_AMMO_PREFIX + m_WeaponName;
	return m_Owner.GetEntity()->spawnArgs.GetInt(key, "0");
}

int CInventoryWeaponItem::GetAmmo() const
{
	return m_Ammo;
}

void CInventoryWeaponItem::SetAmmo(int newAmount)
{
	if (!NeedsAmmo()) {
		// Don't set ammo of weapons that don't need any
		return;
	}

	m_Ammo = (newAmount > m_MaxAmmo) ? m_MaxAmmo : newAmount;

	if (m_Ammo < 0) {
		m_Ammo = 0;
	}
}

int CInventoryWeaponItem::HasAmmo()
{
	if (!NeedsAmmo()) {
		// Always return 1 for non-ammo weapons
		return 1;
	}
	return m_Ammo;
}

void CInventoryWeaponItem::UseAmmo(int amount)
{
	SetAmmo(m_Ammo - amount);
}

void CInventoryWeaponItem::SetWeaponIndex(int index)
{
	m_WeaponIndex = index;

	// Now that the weapon index is known, cache a few values from the owner spawnargs

	const idDict* weaponDict = gameLocal.FindEntityDefDict(m_WeaponDefName,true); // grayman #3391 - don't create a default 'weaponDict'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if (weaponDict == NULL)
	{
		return;
	}

	m_AllowedEmpty = !weaponDict->GetBool(WEAPON_AMMO_REQUIRED, "1");
	m_IsToggleable = weaponDict->GetBool(WEAPON_IS_TOGGLEABLE, "0");

	// Initialise the projectile def name from the weapon spawnargs
	m_ProjectileDefName = weaponDict->GetString("def_projectile");
}

int CInventoryWeaponItem::GetWeaponIndex() const
{
	return m_WeaponIndex;
}

const idStr& CInventoryWeaponItem::GetWeaponName() const
{
	return m_WeaponName;
}

const idStr& CInventoryWeaponItem::GetProjectileDefName() const
{
	return m_ProjectileDefName;
}

void CInventoryWeaponItem::SetProjectileDefName(const idStr& weaponDefName)
{
	m_ProjectileDefName = weaponDefName;
}

void CInventoryWeaponItem::ResetProjectileDefName()
{
	const idDict* weaponDict = gameLocal.FindEntityDefDict(m_WeaponDefName,true); // grayman #3391 - don't create a default 'projectileDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if (weaponDict == NULL)
	{
		return;
	}

	m_ProjectileDefName = weaponDict->GetString("def_projectile");
}

const idStr& CInventoryWeaponItem::GetWeaponDefName() const
{
	return m_WeaponDefName;
}

void CInventoryWeaponItem::SaveItemEntityDict()
{
	// Don't call the base class, roll our own

	m_ItemDict.reset(new idDict);

	*m_ItemDict = *gameLocal.FindEntityDefDict(m_WeaponDefName);
}

void CInventoryWeaponItem::RestoreItemEntityFromDict(const idVec3& entPosition)
{
	// Don't call the base class, roll our own

	m_ItemDict.reset();
}
