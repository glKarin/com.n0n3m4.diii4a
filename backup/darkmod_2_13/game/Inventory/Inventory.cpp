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



#include "Inventory.h"
#include "WeaponItem.h"

#include "../Game_local.h"

#include "../Objectives/MissionData.h"
#include "../Shop/Shop.h" // grayman (#2376)

static idStr sLootTypeName[LOOT_COUNT] = 
{
	"loot_none",
	"loot_jewels",
	"loot_gold",
	"loot_goods"
};

CInventory::CInventory() :
	m_HighestCursorId(0),
	m_LootItemCount(0),
	m_Gold(0),
	m_Jewelry(0),
	m_Goods(0)
{
	m_Owner = NULL;

	CreateCategory(TDM_INVENTORY_DEFAULT_GROUP);	// We always have a defaultgroup if nothing else
}

CInventory::~CInventory()
{
	Clear();
}

void CInventory::Clear()
{
	m_Owner = NULL;
	m_Category.ClearFree();
	m_Cursor.ClearFree();
}

int	CInventory::GetNumCategories() const
{
	return m_Category.Num();
}

void CInventory::CopyTo(CInventory& targetInventory)
{
	// Iterate over all categories to copy stuff
	for (int c = 0; c < GetNumCategories(); ++c)
	{
		const CInventoryCategoryPtr& category = GetCategory(c);

		for (int itemIdx = 0; itemIdx < category->GetNumItems(); ++itemIdx)
		{
			const CInventoryItemPtr& item = category->GetItem(itemIdx);

			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Copying item %s to inventory.\r", common->Translate(item->GetName().c_str()));

			// Add this item to the target inventory
			targetInventory.PutItem(item, item->Category()->GetName());
		}
	}
}

void CInventory::CopyPersistentItemsFrom(const CInventory& sourceInventory, idEntity* newOwner)
{
	// Obtain the weapon category for this inventory
	CInventoryCategoryPtr weaponCategory = GetCategory(TDM_PLAYER_WEAPON_CATEGORY);

	// Cycle through all categories to add them
	for ( int c = 0 ; c < sourceInventory.GetNumCategories() ; ++c )
	{
		const CInventoryCategoryPtr& category = sourceInventory.GetCategory(c);

		for ( int itemIdx = 0 ; itemIdx < category->GetNumItems() ; ++itemIdx )
		{
			const CInventoryItemPtr& item = category->GetItem(itemIdx);

			if (item->GetPersistentCount() <= 0)
			{
				DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING(
					"Item %s is not marked as persistent, won't add to player inventory.\r",
					common->Translate(item->GetName().c_str()));

				continue; // not marked as persistent
			}

			// Check if the shop handled that item already
			const idDict* itemDict = item->GetSavedItemEntityDict();

			if (itemDict != NULL)
			{
				CShopItemPtr shopItem = gameLocal.m_Shop->FindShopItemDefByClassName(itemDict->GetString("classname"));

				if ( (shopItem != NULL) && (CShop::GetQuantityForItem(item) > 0) )
				{
					// grayman #3723 - if there's no shop in this mission,
					// then we can't rely on it to process this inventory item.

					if ( gameLocal.m_Shop->ShopExists() )
					{
						DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING(
								"Item %s would be handled by the shop, won't add that to player inventory.\r",
								common->Translate(item->GetName().c_str()));
						continue;
					}
				}
			}

			// Is set to true if we should add this item.
			// For weapon items with ammo this will be set to false to prevent double-additions
			bool addItem = true;

			// Handle weapons separately, otherwise we might end up with duplicate weapon items
			CInventoryWeaponItemPtr weaponItem = std::dynamic_pointer_cast<CInventoryWeaponItem>(item);

			if (weaponItem && weaponCategory)
			{
				// Weapon items need special consideration. For arrow-based weapons try to merge the ammo.
				for ( int w = 0 ; w < weaponCategory->GetNumItems() ; ++w )
				{
					CInventoryWeaponItemPtr thisWeapon = std::dynamic_pointer_cast<CInventoryWeaponItem>(weaponCategory->GetItem(w));

					if (!thisWeapon)
					{
						continue;
					}

					if (thisWeapon->GetWeaponName() == weaponItem->GetWeaponName())
					{
						// Prevent adding this item, we already have one
						addItem = false;

						// Found a matching weapon, does it use ammo?
						if (thisWeapon->NeedsAmmo())
						{
							DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING(
								"Adding persistent ammo %d to player weapon %s.\r",
								weaponItem->GetAmmo(), thisWeapon->GetWeaponName().c_str());

							// Add the persistent ammo count to this item
							thisWeapon->SetAmmo(thisWeapon->GetAmmo() + weaponItem->GetAmmo());
						}
						else 
						{
							// Doesn't need ammo, check enabled state
							if (weaponItem->IsEnabled() && !thisWeapon->IsEnabled())
							{
								DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING(
									"Enabling weapon item %s as the persistent inventory contains an enabled one.\r",
									thisWeapon->GetWeaponName().c_str());

								thisWeapon->SetEnabled(true);
							}
						}

						break;
					}
				}
			}

			if (addItem)
			{
				DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING(
					"Adding persistent item %s to player inventory, quantity: %d.\r",
					common->Translate(item->GetName().c_str()), item->GetPersistentCount());

				item->SetOwner(newOwner);

				// Add this item to our inventory
				PutItem(item, item->Category()->GetName());

				// If we didn't have a weapon category at this point, we should be able to get one now
				if (weaponItem && !weaponCategory)
				{
					weaponCategory = GetCategory(TDM_PLAYER_WEAPON_CATEGORY);
				}
			}
		}
	}
}

void CInventory::SaveItemEntities(bool persistentOnly)
{
	for (int c = 0; c < GetNumCategories(); ++c)
	{
		const CInventoryCategoryPtr& category = GetCategory(c);

		for (int itemIdx = 0; itemIdx < category->GetNumItems(); ++itemIdx)
		{
			const CInventoryItemPtr& item = category->GetItem(itemIdx);

			if (persistentOnly && item->GetPersistentCount() <= 0)
			{
				continue; // skip non-persistent items
			}

			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Saving item entity of item %s.\r", common->Translate(item->GetName().c_str()));

			item->SaveItemEntityDict();
		}
	}
}

void CInventory::RestoreItemEntities(const idVec3& entPosition)
{
	for (int c = 0; c < GetNumCategories(); ++c)
	{
		const CInventoryCategoryPtr& category = GetCategory(c);

		for (int itemIdx = 0; itemIdx < category->GetNumItems(); ++itemIdx)
		{
			const CInventoryItemPtr& item = category->GetItem(itemIdx);

			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Restoring item entity of item %s, with name '%s'.\r", common->Translate(item->GetName().c_str()),item->GetItemEntity() ? item->GetItemEntity()->GetName():"NULL");

			item->RestoreItemEntityFromDict(entPosition);
		}
	}
}

int CInventory::GetLoot(int& Gold, int& Jewelry, int& Goods)
{
	Gold = 0;
	Jewelry = 0;
	Goods = 0;

	for (int i = 0; i < m_Category.Num(); i++)
	{
		m_Category[i]->GetLoot(Gold, Jewelry, Goods);
	}

	Gold += m_Gold;
	Jewelry += m_Jewelry;
	Goods += m_Goods;

	return Gold + Jewelry + Goods;
}

void CInventory::SetLoot(int Gold, int Jewelry, int Goods)
{
	m_Gold = Gold;
	m_Jewelry = Jewelry;
	m_Goods = Goods;
}

void CInventory::NotifyOwnerAboutPickup(const idStr& pickedUpStr, const CInventoryItemPtr&)
{
	if (!cv_tdm_inv_hud_pickupmessages.GetBool()) return; // setting is turned off

	if (!m_Owner.GetEntity()->IsType(idPlayer::Type)) return; // owner is not a player
	
	idPlayer* player = static_cast<idPlayer*>(m_Owner.GetEntity());

	// Prepend the "acquired" text
	idStr pickedUpMsg = common->Translate( "#str_07215" ) + pickedUpStr;	// Acquired:
	// and replace any newlines from the message with a space so "New\nItem" becomes "New Item"
	pickedUpMsg.Replace("\n"," ");

	// Now actually send the message
	player->SendInventoryPickedUpMessage(pickedUpMsg);
}

CInventoryItemPtr CInventory::ValidateLoot(idEntity *ent, const bool gotFromShop) // grayman (#2376)
{
	CInventoryItemPtr rc;
	int LGroupVal = 0;
	int dummy1(0), dummy2(0), dummy3(0); // for calling GetLoot

	LootType lootType = CInventoryItem::GetLootTypeFromSpawnargs(ent->spawnArgs);
	int value = ent->spawnArgs.GetInt("inv_loot_value", "-1");

	// grayman (#2376) - if anyone ever marks loot "inv_map_start" and it's a valid shop
	// item, the player inventory will already have it at this point.

	if (gotFromShop)
	{
		value = -1;
	}

	if (lootType != LOOT_NONE && value > 0)
	{
		idStr pickedUpMsg = idStr(value);

		// If we have an anonymous loot item, we don't need to 
		// store it in the inventory.
		switch (lootType)
		{
			case LOOT_GOLD:
				m_Gold += value;
				LGroupVal = m_Gold;
				pickedUpMsg += common->Translate("#str_07320");	// " in Gold"
			break;

			case LOOT_GOODS:
				m_Goods += value;
				LGroupVal = m_Goods;
				pickedUpMsg += common->Translate("#str_07321");	// " in Goods"
			break;

			case LOOT_JEWELS:
				m_Jewelry += value;
				LGroupVal = m_Jewelry;
				pickedUpMsg += common->Translate("#str_07322");	// " in Jewels"
			break;
			
			default: break;
		}

		m_LootItemCount++;

		rc = GetItemByType(CInventoryItem::IT_LOOT_INFO);

		assert(rc != NULL); // the loot item must exist

		// greebo: Update the total loot value in the objectives system BEFORE
		// the InventoryCallback. Some comparisons rely on a valid total loot value.
		gameLocal.m_MissionData->ChangeFoundLoot(lootType, value);

		// Objective Callback for loot on a specific entity:
		// Pass the loot type name and the net loot value of that group
		gameLocal.m_MissionData->InventoryCallback( 
			ent, 
			sLootTypeName[lootType], 
			LGroupVal, 
			GetLoot( dummy1, dummy2, dummy3 ), 
			true 
		);

		// Take the loot icon of the picked up item and use it for the loot stats item

		idStr lootIcon = ent->spawnArgs.GetString("inv_icon");
		if (rc != NULL && !lootIcon.IsEmpty())
		{
			rc->SetIcon(lootIcon);
		}
		
		if (!ent->spawnArgs.GetBool("inv_map_start", "0") && !ent->spawnArgs.GetBool("inv_no_pickup_message", "0"))
		{
			NotifyOwnerAboutPickup(pickedUpMsg, rc);
		}
	}
	else
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Item %s doesn't have an inventory name and is not anonymous.\r", ent->name.c_str());
	}

	return rc;
}

void CInventory::SetOwner(idEntity *owner)
{
	m_Owner = owner; 

	for (int i = 0; i < m_Category.Num(); i++)
	{
		m_Category[i]->SetOwner(owner);
	}
}

CInventoryCategoryPtr CInventory::CreateCategory(const idStr& categoryName, int* index)
{
	if (categoryName.IsEmpty()) return CInventoryCategoryPtr(); // empty category name

	// Try to lookup the category, maybe it exists already
	CInventoryCategoryPtr rc = GetCategory(categoryName, index);

	if (rc != NULL) return rc; // Category already exists

	// Try to allocate a new category with a link back to <this> Inventory
	rc = CInventoryCategoryPtr(new CInventoryCategory(this, categoryName));
	
	// Add the new Category to our list
	int i = m_Category.AddUnique(rc);

	// Should we return an index?
	if (index != NULL)
	{
		*index = i;
	}

	return rc;
}

CInventoryCategoryPtr CInventory::GetCategory(const idStr& categoryName, int* index)
{
	// If the groupname is empty we look for the default group
	if (categoryName.IsEmpty())
	{
		return GetCategory(TDM_INVENTORY_DEFAULT_GROUP);
	}

	// Traverse the categories and find the one matching <CategoryName>
	for (int i = 0; i < m_Category.Num(); i++)
	{
		if (m_Category[i]->GetName() == categoryName)
		{
			if (index != NULL)
			{
				*index = i;
			}

			return m_Category[i];
		}
	}

	return CInventoryCategoryPtr(); // not found
}

CInventoryCategoryPtr CInventory::GetCategory(int index) const
{
	if (index >= 0 && index < m_Category.Num()) {
		return m_Category[index];
	}
	// return NULL for invalid indices
	return CInventoryCategoryPtr();
}

int CInventory::GetCategoryIndex(const idStr& categoryName)
{
	int i = -1;

	GetCategory(categoryName, &i);

	return i;
}

int CInventory::GetCategoryIndex(const CInventoryCategoryPtr& category)
{
	return m_Category.FindIndex(category);
}

int CInventory::GetCategoryItemIndex(const idStr& itemName, int* itemIndex)
{
	// Set the returned index to -1 if applicable
	if (itemIndex != NULL) *itemIndex = -1;

	if (itemName.IsEmpty()) return -1;

	for (int i = 0; i < m_Category.Num(); i++)
	{
		// Try to find the item within the category
		int n = m_Category[i]->GetItemIndex(itemName);

		if (n != -1)
		{
			// Found, set the item index if desired
			if (itemIndex != NULL) *itemIndex = n;

			// Return the category index
			return i;
		}
	}

	return -1; // not found
}

int CInventory::GetCategoryItemIndex(const CInventoryItemPtr& item, int* itemIndex)
{
	if (itemIndex != NULL) *itemIndex = -1;

	for (int i = 0; i < m_Category.Num(); i++)
	{
		int n = m_Category[i]->GetItemIndex(item);

		if (n != -1)
		{
			// Found, return the index
			if (itemIndex != NULL) *itemIndex = n;

			// Return the category index
			return i;
		}
	}

	return -1; // not found
}

CInventoryItemPtr CInventory::PutItem(idEntity *ent, idEntity *owner)
{
	// Sanity checks
	if (ent == NULL || owner == NULL) return CInventoryItemPtr();

	// grayman (#2376) - If there's a shop with this item in it,
	// and this is an inv_map_start item, we won't put it into the
	// inventory because the player already has it. 

	const ShopItemList& startingItems = gameLocal.m_Shop->GetPlayerStartingEquipment();
	bool gotFromShop = ((startingItems.Num() > 0) && (ent->spawnArgs.GetBool("inv_map_start", "0")));

	// Check for loot items
	CInventoryItemPtr returnValue = ValidateLoot(ent,gotFromShop); // grayman (#2376)

	if (ent->GetAbsenceNoticeability() > 0)
	{
		ent->SpawnAbsenceMarker();
	}

	if (returnValue != NULL)
	{
		// The item is a valid loot item, remove the entity and return
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Added loot item to inventory: %s\r", ent->name.c_str());

		// Remove the entity, it is a loot item (which vanishes when added to the inventory)
		RemoveEntityFromMap(ent, true);

		return returnValue;
	}

	// Let's see if this is an ammunition item
	returnValue = ValidateAmmo(ent,gotFromShop); // grayman (#2376)

	if (returnValue != NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Added ammo item to inventory, removing from map: %s\r", ent->name.c_str());

		// Remove the entity from the game, the ammunition is added
		RemoveEntityFromMap(ent, true);

		return returnValue;
	}

	// Check for a weapon item
	returnValue = ValidateWeapon(ent,gotFromShop); // grayman (#2376)

	if (returnValue != NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Added weapon item to inventory, removing from map: %s\r", ent->name.c_str());

		// Remove the entity from the game, the ammunition is added
		RemoveEntityFromMap(ent, true);

		return returnValue;
	}

	// Not a loot or ammo item, determine name and category to check for existing item of same name/category
	idStr name = ent->spawnArgs.GetString("inv_name", "");
	idStr category = ent->spawnArgs.GetString("inv_category", "");
	// Tels: Replace "\n" with \x0a, otherwise multiline spawnargs set inside DR do not work
	name.Replace( "\\n", "\n" );
	category.Replace( "\\n", "\n" );

	if (name.IsEmpty() || category.IsEmpty())
	{
		// Invalid inv_name or inv_category
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot put %s in inventory: inv_name or inv_category not specified.\r", ent->name.c_str());
		return returnValue;
	}

	// Check for existing items (create the category if necessary (hence the TRUE))
	CInventoryItemPtr existing = GetItem(name, category, true);

	if (existing != NULL)
	{
		// Item must be stackable, if items of the same name/category already exist
		if (!ent->spawnArgs.GetBool("inv_stackable", "0"))
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot put %s in inventory: not stackable.\r", ent->name.c_str());

			// grayman #2467 - Remove the entity from the game if it was already put into the inventory by the shop code.

			if (gotFromShop)
			{
				RemoveEntityFromMap(ent, true);
			}

			return returnValue;
		}

		// Item is stackable, determine how many items should be added to the stack
		int count = ent->spawnArgs.GetInt("inv_count", "1");
		
		// grayman (#2376) - If there's a shop in this mission, all stackable inv_map_start items were shown in
		// the shop's startingItems list, and have already been given to the player. Check if this
		// entity is an inv_map_start entity. If it is, check the size of the startingItems list.
		// If it's 0, then there's no shop and we have to give the player the count from this
		// entity. If > 0, zero the count because the player already has it.

		if (gotFromShop)
		{
			count = 0;	// Item count already given, so clear it.
		}

		// Increase the stack count
		existing->SetCount(existing->GetCount() + count);

		// Persistent flags are latched - once a inv_persistent item is added to a stack, the whole stack
		// snaps into persistent mode.
		if (ent->spawnArgs.GetBool("inv_persistent") && !existing->IsPersistent())
		{
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Marking stackable items as persistent after picking up one persistent item: %s\r", existing->GetName().c_str());
			existing->SetPersistent(true);
		}

		// We added a stackable item that was already in the inventory
		// grayman #3315 - Solution from Zbyl. InventoryCallback() looks at
		// the existing entity to retrieve a bindmaster, and that's already
		// been NULLed. So we need to look at the new entity instead.
		gameLocal.m_MissionData->InventoryCallback(
			ent,
			existing->GetName(), 
			existing->GetCount(), 
			1, 
			true
		);

		// Notify the player, if appropriate
		// grayman #3316 - correct pickup message, courtesy Zbyl
		if ( !ent->spawnArgs.GetBool("inv_map_start", "0") && !ent->spawnArgs.GetBool("inv_no_pickup_message", "0") )
		{
			idStr msg = common->Translate(name);

			if ( count > 1 ) 
			{
				msg += " x" + idStr(count);
			}

			NotifyOwnerAboutPickup(msg, existing);
		}
		
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Added stackable item to inventory: %s\r", ent->name.c_str());
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("New inventory item stack count is: %d\r", existing->GetCount());

		// Remove the entity, it has been stacked
		RemoveEntityFromMap(ent, true);

		// Return the existing value instead of a newly created one
		returnValue = existing;
	}
	else
	{
		// Item doesn't exist, create a new InventoryItem

		// grayman (#2376) - if we got here, the item isn't already in the inventory,
		// so it wasn't given by the shop. No code changes needed.

		CInventoryItemPtr item(new CInventoryItem(ent, owner));

		if (item != NULL)
		{
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Adding new inventory item %s to category %s...\r", common->Translate(name.c_str()), common->Translate(category.c_str()));
			// Put the item into its category
			PutItem(item, category);

			// grayman #3313 - Solution is from Zbyl. InventoryCallback() is called from PutItem(), so it's already been done.

/*			// We added a new inventory item
			gameLocal.m_MissionData->InventoryCallback(
				item->GetItemEntity(), item->GetName(), 
				1, 
				1, 
				true
			);
 */
			// grayman #3316 - correct pickup message, courtesy Zbyl
			if ( !ent->spawnArgs.GetBool("inv_map_start", "0") && !ent->spawnArgs.GetBool("inv_no_pickup_message", "0") )
			{
				idStr msg = common->Translate(name);

				if ( item->GetCount() > 1 ) 
				{
					msg += " x" + idStr(item->GetCount());
				}

				NotifyOwnerAboutPickup(msg, item);
			}

			// Hide the entity from the map (don't delete the entity)
			RemoveEntityFromMap(ent, false);
		}
		else
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Cannot put item into category: %s.\r", ent->name.c_str());
		}

		returnValue = item;
	}
	
	return returnValue;
}

void CInventory::RemoveEntityFromMap(idEntity* ent, bool deleteEntity)
{
	if (ent == NULL) return;

	// greebo: Don't hide inexhaustible items
	if (ent->spawnArgs.GetBool("inv_inexhaustible", "0")) return;

	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Hiding entity from game: %s...\r", ent->name.c_str());

	// Make the item invisible
	ent->Unbind();
	ent->GetPhysics()->PutToRest();
	ent->GetPhysics()->UnlinkClip();
	// Tels: #2826: temp. stop LOD thinking, including all possible things attached to this entity
	LodComponent::DisableLOD( ent, true );
	ent->Hide();

	if (deleteEntity == true)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Deleting entity from game: %s...\r", ent->name.c_str());
		ent->PostEventMS(&EV_Remove, 0);
	}
}

void CInventory::PutItem(const CInventoryItemPtr& item, const idStr& categoryName)
{
	if (item == NULL) return;
	
	CInventoryCategoryPtr category;

	// Check if it is the default group or not.
	if (categoryName.IsEmpty())
	{
		// category is empty, assign the item to the default group
		category = m_Category[0];
	}
	else
	{
		// Try to find the category with the given name
		category = GetCategory(categoryName);

		// If not found, create it
		if (category == NULL)
		{
			category = CreateCategory(categoryName);
		}
	}

	if (!category) {
		gameLocal.Warning("CInventory::PutItem failed: called with empty name but no categories exist");
		return;
	}

	// Pack the item into the category
	category->PutItemFront(item);

	// Objective callback for non-loot items:
	// non-loot item passes in inv_name and individual item count, SuperGroupVal of 1
	gameLocal.m_MissionData->InventoryCallback( 
		item->GetItemEntity(), 
		item->GetName(), 
		item->GetCount(), 
		1, 
		true
	);
}

bool CInventory::ReplaceItem(idEntity* oldItemEnt, idEntity* newItemEnt)
{
	if (oldItemEnt == NULL) return false;

	idStr oldInvName = oldItemEnt->spawnArgs.GetString("inv_name");

	CInventoryItemPtr oldItem = GetItem(oldInvName);

	if (oldItem == NULL)
	{
		gameLocal.Warning("Could not find old inventory item for %s", oldItemEnt->name.c_str());
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Could not find old inventory item for %s\n", oldItemEnt->name.c_str());
		return false;
	}

	// greebo: Let's call PutItem on the new entity first to see what kind of item this is
	// PutItem will also take care of the mission data callbacks for the objectives
	CInventoryItemPtr newItem = PutItem(newItemEnt, m_Owner.GetEntity());

	if (newItem != NULL && newItem->Category() == oldItem->Category())
	{
		// New item has been added, swap the old and the new one to fulfil the inventory position guarantee
		oldItem->Category()->SwapItemPosition(oldItem, newItem);
	}
	
	// If SwapItemPosition has been called, newItem now takes the place of oldItem before the operation.
	// Remove the old item in any case, but only if the items are actually different.
	// In case anybody wonder, newItem might be the same as oldItem in the case of stackable items or loot.
	if (oldItem != newItem)
	{
		RemoveItem(oldItem);
	}

	return true;
}

void CInventory::RemoveItem(const CInventoryItemPtr& item)
{
	if (item == nullptr || item->Category() == nullptr) return;

	idStr sCategoryName = item->Category()->GetName();

	item->Category()->RemoveItem(item);

	// Validate all cursors of the same category
	int iCursorIdx = -1;
	for (int i = 0; i < m_Cursor.Num(); i++)
	{
		CInventoryCursorPtr& pCursor(m_Cursor[i]);
		if (pCursor == nullptr)
			continue;

		CInventoryCategoryPtr pCategory(pCursor->GetCurrentCategory());
		if (pCategory == nullptr)
			continue;

		if (pCategory->GetName() == sCategoryName)
			pCursor->Validate();
	}
}

bool CInventory::HasItem(idEntity* itemEnt)
{
	if (itemEnt == NULL)
		return false;

	idStr invName = itemEnt->spawnArgs.GetString("inv_name");
	CInventoryItemPtr item = GetItem(invName);

	return item != NULL;
}

CInventoryItemPtr CInventory::GetItem(const idStr& name, const idStr& categoryName, bool createCategory)
{
	// Do we have a specific category to search in?
	if (!categoryName.IsEmpty())
	{
		// We have a category name, look it up
		CInventoryCategoryPtr category = GetCategory(categoryName);

		if (category == NULL && createCategory)
		{
			// Special case, the caller requested to create this category if not found
			category = CreateCategory(categoryName);
		}

		// Let the category search for the item, may return NULL
		return (category != NULL) ? category->GetItem(name) : CInventoryItemPtr();
	}

	// No specific category specified, look in all categories
	for (int i = 0; i < m_Category.Num(); i++)
	{
		CInventoryItemPtr foundItem = m_Category[i]->GetItem(name);

		if (foundItem != NULL)
		{
			// Found the item
			return foundItem;
		}
	}

	return CInventoryItemPtr(); // nothing found
}

CInventoryItemPtr CInventory::GetItemById(const idStr& id, const idStr& categoryName, bool createCategory)
{
	// Do we have a specific category to search in?
	if (!categoryName.IsEmpty())
	{
		// We have a category name, look it up
		CInventoryCategoryPtr category = GetCategory(categoryName);

		if (category == NULL && createCategory)
		{
			// Special case, the caller requested to create this category if not found
			category = CreateCategory(categoryName);
		}

		// Let the category search for the item, may return NULL
		return (category != NULL) ? category->GetItemById(id) : CInventoryItemPtr();
	}

	// No specific category specified, look in all categories
	for (int i = 0; i < m_Category.Num(); i++)
	{
		CInventoryItemPtr foundItem = m_Category[i]->GetItemById(id);

		if (foundItem != NULL)
		{
			// Found the item
			return foundItem;
		}
	}

	return CInventoryItemPtr(); // nothing found
}

CInventoryItemPtr CInventory::GetItemByType(CInventoryItem::ItemType type)
{
	for (int i = 0; i < m_Category.Num(); i++)
	{
		CInventoryItemPtr foundItem = m_Category[i]->GetItemByType(type);

		if (foundItem != NULL)
		{
			// Found the item
			return foundItem;
		}
	}

	return CInventoryItemPtr(); // nothing found
}

CInventoryCursorPtr CInventory::CreateCursor()
{
	// Get a new ID for this cursor
	int id = GetNewCursorId();

	CInventoryCursorPtr cursor(new CInventoryCursor(this, id));

	if (cursor != NULL)
	{
		m_Cursor.AddUnique(cursor);
	}

	return cursor;
}

CInventoryCursorPtr CInventory::GetCursor(int id)
{
	for (int i = 0; i < m_Cursor.Num(); i++)
	{
		if (m_Cursor[i]->GetId() == id)
		{
			return m_Cursor[i];
		}
	}

	DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Requested Cursor Id %d not found!\r", id);
	return CInventoryCursorPtr();
}

int CInventory::GetHighestCursorId()
{
	return m_HighestCursorId;
}

int CInventory::GetNewCursorId()
{
	return ++m_HighestCursorId;
}

void CInventory::Save(idSaveGame *savefile) const
{
	m_Owner.Save(savefile);
	
	savefile->WriteInt(m_Category.Num());
	for (int i = 0; i < m_Category.Num(); i++) {
		m_Category[i]->Save(savefile);
	}

	savefile->WriteInt(m_LootItemCount);
	savefile->WriteInt(m_Gold);
	savefile->WriteInt(m_Jewelry);
	savefile->WriteInt(m_Goods);
	savefile->WriteInt(m_HighestCursorId);

	savefile->WriteInt(m_Cursor.Num());
	for (int i = 0; i < m_Cursor.Num(); i++) {
		m_Cursor[i]->Save(savefile);
	}
}

void CInventory::Restore(idRestoreGame *savefile)
{
	// Clear all member variables beforehand 
	Clear();

	m_Owner.Restore(savefile);

	int num;
	savefile->ReadInt(num);
	for(int i = 0; i < num; i++) {
		CInventoryCategoryPtr category(new CInventoryCategory(this, ""));

		category->Restore(savefile);
		m_Category.Append(category);
	}

	savefile->ReadInt(m_LootItemCount);
	savefile->ReadInt(m_Gold);
	savefile->ReadInt(m_Jewelry);
	savefile->ReadInt(m_Goods);
	savefile->ReadInt(m_HighestCursorId);

	savefile->ReadInt(num);
	for(int i = 0; i < num; i++) {
		CInventoryCursorPtr cursor(new CInventoryCursor(this, 0));

		cursor->Restore(savefile);
		m_Cursor.Append(cursor);
	}
}

void CInventory::RemoveCategory(const CInventoryCategoryPtr& category)
{
	m_Category.Remove(category);
}

CInventoryItemPtr CInventory::ValidateAmmo(idEntity* ent, const bool gotFromShop) // grayman (#2376)
{
	// Sanity check
	if (ent == NULL) return CInventoryItemPtr();
	
	// Check for ammonition
	int amount = ent->spawnArgs.GetInt("inv_ammo_amount", "0");

	if (amount <= 0) 
	{
		return CInventoryItemPtr(); // not ammo
	}

	CInventoryItemPtr returnValue;

	idStr weaponName = ent->spawnArgs.GetString("inv_weapon_name", "");

	if (weaponName.IsEmpty())
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Could not find 'inv_weapon_name' on item %s.\r", ent->name.c_str());
		gameLocal.Warning("Could not find 'inv_weapon_name' on item %s.", ent->name.c_str());
		return returnValue;
	}

	// grayman (#2376)
	// If we already got this from the shop, zero the ammo count. 

	if (gotFromShop)
	{
		amount = 0; // Ammo already given, so clear it. We could leave at
					// this point, but the calling method expects a CInventoryItemPtr,
					// so we have to execute the following code to obtain it.
	}

	// Find the weapon category
	CInventoryCategoryPtr weaponCategory = GetCategory(TDM_PLAYER_WEAPON_CATEGORY);

	if (weaponCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Could not find weapon category in inventory.\r");
		return returnValue;
	}
	
	// Look for the weapon with the given name
	for (int i = 0; i < weaponCategory->GetNumItems(); i++)
	{
		CInventoryWeaponItemPtr weaponItem = 
			std::dynamic_pointer_cast<CInventoryWeaponItem>(weaponCategory->GetItem(i));

		// Is this the right weapon?
		if (weaponItem != NULL && weaponItem->GetWeaponName() == weaponName)
		{
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Adding %d ammo to weapon %s.\r", amount, weaponName.c_str());

			// Add the ammo to this weapon
			weaponItem->SetAmmo(weaponItem->GetAmmo() + amount);

			if (!ent->spawnArgs.GetBool("inv_map_start", "0") && !ent->spawnArgs.GetBool("inv_no_pickup_message", "0"))
			{
				// Tels: For some reason "inv_name" here is "Fire Arrow", even tho this string never appears anywhere
				// 	 when running f.i. in "German". So use weaponItem->GetName(), which is correctly "#str_02435":
				idStr msg = common->Translate( weaponItem->GetName() ); 

				if (amount > 1)
				{
					msg += " x" + idStr(amount);
				}
				NotifyOwnerAboutPickup(msg, weaponItem);
			}
			
			// We're done
			return weaponItem;
		}
	}

	return returnValue;
}

CInventoryItemPtr CInventory::ValidateWeapon(idEntity* ent, const bool gotFromShop) // grayman (#2376)
{
	// Sanity check
	if (ent == NULL) return CInventoryItemPtr();

	idStr weaponName = ent->spawnArgs.GetString("inv_weapon_name");

	if (weaponName.IsEmpty())
	{
		// Not a weapon item
		return CInventoryItemPtr();
	}

	// Entity has a weapon name set, check for match in our inventory

	// Find the weapon category
	CInventoryCategoryPtr weaponCategory = GetCategory(TDM_PLAYER_WEAPON_CATEGORY);

	if (weaponCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Could not find weapon category in inventory.\r");
		return CInventoryItemPtr();
	}
	
	// Look for the weapon with the given name
	for (int i = 0; i < weaponCategory->GetNumItems(); i++)
	{
		CInventoryWeaponItemPtr weaponItem = 
			std::dynamic_pointer_cast<CInventoryWeaponItem>(weaponCategory->GetItem(i));

		// Is this the right weapon? (must be a melee weapon, allowed empty)
		if (weaponItem != NULL && weaponItem->IsAllowedEmpty() && weaponItem->GetWeaponName() == weaponName)
		{
			DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Entity %s is matching the melee weapon %s.\r", ent->name.c_str(), weaponName.c_str());

			// Enable this weapon

			if (!gotFromShop) // grayman (#2376)
			{
				weaponItem->SetEnabled(true);
			}

			if (!ent->spawnArgs.GetBool("inv_map_start", "0") && !ent->spawnArgs.GetBool("inv_no_pickup_message", "0"))
			{
				NotifyOwnerAboutPickup( common->Translate( ent->spawnArgs.GetString("inv_name") ), weaponItem);
			}
			
			// We're done
			return weaponItem;
		}
	}

	DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Couldn't find a match for weapon name: %s.\r", weaponName.c_str());

	return CInventoryItemPtr();
}
