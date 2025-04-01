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



#include "InventoryItem.h"
#include "Inventory.h"
#include <algorithm>

CInventoryItem::CInventoryItem(idEntity *owner)
{
	m_Owner = owner;
	m_Item = NULL;
	m_Category = NULL;
	m_Type = IT_ITEM;
	m_LootType = LOOT_NONE;
	m_Value = 0;
	m_Stackable = false;
	m_Count = 1;
	m_Droppable = false;
	m_Overlay = OVERLAYS_INVALID_HANDLE;
	m_Hud = false;
	m_Orientated = false;
	m_Persistent = false;
	m_LightgemModifier = 0;
	m_MovementModifier = 1.0f;
	m_FrobDistanceCap = -1;
	m_UseOnFrob = false;
	m_DropOrientation = mat3_identity;
}

CInventoryItem::CInventoryItem(idEntity* itemEntity, idEntity* owner) {
	// Don't allow NULL pointers
	assert(owner && itemEntity);

	// Parse a few common spawnargs
	ParseSpawnargs(itemEntity->spawnArgs);

	m_Category = NULL;
	m_Overlay = OVERLAYS_INVALID_HANDLE;
	m_Hud = false;

	m_Owner = owner;
	m_Item = itemEntity;
	
	// Determine and set the loot type
	m_LootType = GetLootTypeFromSpawnargs(itemEntity->spawnArgs);

	// Read the spawnargs into the member variables
	m_Name = itemEntity->spawnArgs.GetString("inv_name", "");
	// Tels: Replace "\n" with \x0a, otherwise multiline names set inside DR do not work
	m_Name.Replace( "\\n", "\n" );
	m_Value	= itemEntity->spawnArgs.GetInt("inv_loot_value", "-1");
	m_Stackable	= itemEntity->spawnArgs.GetBool("inv_stackable", "0");
	m_UseOnFrob = itemEntity->spawnArgs.GetBool("inv_use_on_frob", "0");

	m_Count = (m_Stackable) ? itemEntity->spawnArgs.GetInt("inv_count", "1") : 1;

	m_Droppable = itemEntity->spawnArgs.GetBool("inv_droppable", "0");
	m_ItemId = itemEntity->spawnArgs.GetString("inv_item_id", "");

	if (m_Icon.IsEmpty() && m_LootType == LOOT_NONE)
	{
		DM_LOG(LC_INVENTORY, LT_INFO)LOGSTRING("Information: non-loot item %s has no icon.\r", itemEntity->name.c_str());
	}

	if (m_LootType != LOOT_NONE && m_Value <= 0)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Warning: Value for loot item missing on entity %s\r", itemEntity->name.c_str());
	}

	// Set the item type according to the loot property
	m_Type = (m_LootType != LOOT_NONE) ? IT_LOOT : IT_ITEM;

	m_BindMaster = itemEntity->GetBindMaster();
	m_Orientated = itemEntity->fl.bindOrientated;

	idStr hudName;
	// Item could be added to the inventory, check for custom HUD
	if (itemEntity->spawnArgs.GetString("inv_hud", "", hudName) != false)
	{
		int hudLayer;
		itemEntity->spawnArgs.GetInt("inv_hud_layer", "0", hudLayer);
		SetHUD(hudName, hudLayer);
	}

	// Check for a preferred drop orientation, if not, use current orientation
	if( itemEntity->spawnArgs.FindKey("drop_angles") )
	{
		idAngles DropAngles;
		DropAngles = itemEntity->spawnArgs.GetAngles("drop_angles");
		m_DropOrientation = DropAngles.ToMat3();
	}
	else
	{
		idVec3 dummy;
		idMat3 playerView;
		gameLocal.GetLocalPlayer()->GetViewPos(dummy, playerView);
		// drop orientation is relative to the player view yaw only
		idAngles viewYaw = playerView.ToAngles();
		// ignore pitch and roll
		viewYaw[0] = 0;
		viewYaw[2] = 0;
		idMat3 playerViewYaw = viewYaw.ToMat3();

		m_DropOrientation = itemEntity->GetPhysics()->GetAxis() * playerViewYaw.Transpose();
	}
}

void CInventoryItem::Save( idSaveGame *savefile ) const
{
	m_Owner.Save(savefile);
	m_Item.Save(savefile);

	savefile->WriteBool(m_ItemDict != NULL);

	if (m_ItemDict != NULL)
	{
		savefile->WriteDict(m_ItemDict.get());
	}

	m_BindMaster.Save(savefile);

	savefile->WriteString(m_Name);
	savefile->WriteString(m_HudName);
	savefile->WriteString(m_ItemId);

	savefile->WriteInt(static_cast<int>(m_Type));
	savefile->WriteInt(static_cast<int>(m_LootType));

	savefile->WriteInt(m_Value);
	savefile->WriteInt(m_Overlay);
	savefile->WriteInt(m_Count);

	savefile->WriteBool(m_Stackable);
	savefile->WriteBool(m_Droppable);
	savefile->WriteBool(m_Hud);

	savefile->WriteString(m_Icon);

	savefile->WriteBool(m_Orientated);
	savefile->WriteBool(m_Persistent);
	
	savefile->WriteInt(m_LightgemModifier);
	savefile->WriteFloat(m_MovementModifier);
	savefile->WriteBool(m_UseOnFrob);
	savefile->WriteFloat(m_FrobDistanceCap);
	savefile->WriteMat3(m_DropOrientation);
}

void CInventoryItem::Restore( idRestoreGame *savefile )
{
	m_Owner.Restore(savefile);
	m_Item.Restore(savefile);

	bool hasDict;
	savefile->ReadBool(hasDict);
	
	if (hasDict)
	{
		m_ItemDict.reset(new idDict);
		savefile->ReadDict(m_ItemDict.get());
	}
	else
	{
		m_ItemDict.reset();
	}

	m_BindMaster.Restore(savefile);

	savefile->ReadString(m_Name);
	savefile->ReadString(m_HudName);
	savefile->ReadString(m_ItemId);

	int tempInt;
	savefile->ReadInt(tempInt);
	m_Type = static_cast<ItemType>(tempInt);

	savefile->ReadInt(tempInt);
	m_LootType = static_cast<LootType>(tempInt);

	savefile->ReadInt(m_Value);
	savefile->ReadInt(m_Overlay);
	savefile->ReadInt(m_Count);

	savefile->ReadBool(m_Stackable);
	savefile->ReadBool(m_Droppable);
	savefile->ReadBool(m_Hud);

	savefile->ReadString(m_Icon);

	savefile->ReadBool(m_Orientated);
	savefile->ReadBool(m_Persistent);

	savefile->ReadInt(m_LightgemModifier);
	savefile->ReadFloat(m_MovementModifier);
	savefile->ReadBool(m_UseOnFrob);
	savefile->ReadFloat(m_FrobDistanceCap);
	savefile->ReadMat3(m_DropOrientation);
}

void CInventoryItem::ParseSpawnargs(const idDict& spawnArgs)
{
	m_Persistent = spawnArgs.GetBool("inv_persistent", "0");
	m_LightgemModifier = spawnArgs.GetInt("inv_lgmodifier", "0");
	m_MovementModifier = spawnArgs.GetFloat("inv_movement_modifier", "1");
	m_FrobDistanceCap = spawnArgs.GetFloat("inv_frob_distance_cap", "-1");
	m_Icon = spawnArgs.GetString("inv_icon", "");
}

void CInventoryItem::SetLootType(LootType t)
{
	// Only positive values are allowed
	if (t >= LOOT_NONE && t <= LOOT_COUNT)
	{
		m_LootType = t;
	}
	else
	{
		m_LootType = LOOT_NONE;
	}

	NotifyItemChanged();
}

void CInventoryItem::SetValue(int n)
{
	// Only positive values are allowed
	if (n >= 0)
	{
		m_Value = n;

		NotifyItemChanged();
	}
}

void CInventoryItem::SaveItemEntityDict()
{
	idEntity* ent = GetItemEntity();

	if (ent == NULL)
	{
		return;
	}

	// We have a non-NULL item entity, save its spawnargs
	m_ItemDict.reset(new idDict);

	// Copy spawnargs over
	*m_ItemDict = ent->spawnArgs;
}

void CInventoryItem::RestoreItemEntityFromDict(const idVec3& entPosition)
{
	if (!m_ItemDict)
	{
		return; // no saved spawnargs, do nothing
	}

	// We have an item dictionary, let's respawn our entity
	idEntity* ent;
	
	// grayman #3723 - When restoring items this way, it's possible
	// that the name of the object we're about to restore is the same
	// as an object in the map. If so, change the name of the restored object.

	idStr		error;
	const char  *name;

	if ( (*m_ItemDict).GetString( "name", "", &name ) )
	{
		sprintf( error, " on '%s'", name);
	}

	// check if this name is already in use

	if (gameLocal.FindEntity(name))
	{
		gameLocal.Warning("Multiple entities named '%s'", name);
		DM_LOG(LC_INIT, LT_INIT)LOGSTRING("WARNING - Multiple entities named '%s'\r", name);

		// Rename with a unique name.

		(*m_ItemDict).Set("name",va("%s_%d",name,gameLocal.random.RandomInt(1000)));
	}

	if (!gameLocal.SpawnEntityDef(*m_ItemDict, &ent))
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Can't respawn inventory item entity '%s'!\r", m_ItemDict->GetString("name"));
		gameLocal.Error("Can't respawn inventory item entity '%s'!", m_ItemDict->GetString("name"));
	}

	// Place the entity at the given position
	ent->SetOrigin(entPosition);

	// Hide the entity (don't delete it)
	CInventory::RemoveEntityFromMap(ent, false);

	// Set this as new item entity
	SetItemEntity(ent);

	// Finally, remove our saved spawnargs
	m_ItemDict.reset();
}

const idDict* CInventoryItem::GetSavedItemEntityDict() const
{
	return m_ItemDict ? m_ItemDict.get() : NULL;
}

void CInventoryItem::SetCount(int n)
{
	// Only positive values are allowed
	m_Count = (n >= 0) ? n : 0;

	NotifyItemChanged();
}

void CInventoryItem::SetStackable(bool stack)
{
	m_Stackable = stack;
}

void CInventoryItem::SetHUD(const idStr &HudName, int layer)
{
	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Setting hud %s on layer %d\r", HudName.c_str(), layer); 
	if (m_Overlay == OVERLAYS_INVALID_HANDLE || m_HudName != HudName)
	{
		idEntity *owner = GetOwner();

		m_Hud = true;
		m_HudName = HudName;
		m_Overlay = owner->CreateOverlay(HudName, layer);
		idEntity* it = m_Item.GetEntity();
		if (it != NULL)
		{
			it->CallScriptFunctionArgs("inventory_item_init", true, 0, "eefs", it, owner, (float)m_Overlay, HudName.c_str());
		}
	}

	NotifyItemChanged();
}

void CInventoryItem::SetOverlay(const idStr &HudName, int overlay)
{
	if (overlay != OVERLAYS_INVALID_HANDLE)
	{
		idEntity *owner = GetOwner();

		m_Hud = true;
		m_HudName = HudName;
		m_Overlay = overlay;
		idEntity* it = m_Item.GetEntity();
		if (it != NULL)
		{
			it->CallScriptFunctionArgs("inventory_item_init", true, 0, "eefs", it, owner, (float)m_Overlay, HudName.c_str());
		}
	}
	else
	{
		m_Hud = false;
	}
}

LootType CInventoryItem::GetLootTypeFromSpawnargs(const idDict& spawnargs)
{
	// Determine the loot type
	int lootTypeInt;
	LootType returnValue = LOOT_NONE;

	if (spawnargs.GetInt("inv_loot_type", "", lootTypeInt) != false)
	{
		if (lootTypeInt >= LOOT_NONE && lootTypeInt < LOOT_COUNT)
		{
			returnValue = static_cast<LootType>(lootTypeInt);
		}
		else
		{
			DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid loot type: %d\r", lootTypeInt);
		}
	}

	return returnValue;
}

int CInventoryItem::GetPersistentCount()
{
	if (m_Persistent)
	{
		return m_Count;
	}
	else 
	{
		return 0;
	}
}

void CInventoryItem::SetPersistent(bool newValue)
{
	m_Persistent = newValue;
}

void CInventoryItem::SetLightgemModifier(int newValue)
{
	// greebo: Clamp the value to [0..1]
	m_LightgemModifier = idMath::ClampInt(0, DARKMOD_LG_MAX, newValue);
}

void CInventoryItem::SetMovementModifier(float newValue)
{
	if (newValue > 0)
	{
		m_MovementModifier = newValue;
	}
}

void CInventoryItem::SetDropOrientation(const idMat3& newAxis)
{
	m_DropOrientation = newAxis;
}

void CInventoryItem::SetIcon(const idStr& newIcon)
{
	m_Icon = newIcon;

	NotifyItemChanged();
}

void CInventoryItem::NotifyItemChanged()
{
	if (m_Owner.GetEntity() == NULL) return;

	m_Owner.GetEntity()->OnInventoryItemChanged();
}
