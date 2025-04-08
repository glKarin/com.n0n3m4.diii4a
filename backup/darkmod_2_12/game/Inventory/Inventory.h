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
#ifndef __DARKMOD_INVENTORY_H__
#define __DARKMOD_INVENTORY_H__

#include "Cursor.h"

#define TDM_INVENTORY_DEFAULT_GROUP		"DEFAULT"
#define TDM_DUMMY_ITEM					"dummy"
#define TDM_INVENTORY_DROPSCRIPT		"inventoryDrop"

/* DESCRIPTION: This file contains the inventory handling for TDM. The inventory 
 * has nothing in common with the original idInventory and is totally independent
 * from it. It contains the inventory itself, the groups, items and cursors for
 * interaction with the inventory.
 *
 * Each entity has exactly one inventory. An inventory is created when the entity's
 * inventory is accessed for th first time and also one default group is added 
 * named "DEFAULT".
 *
 * Each item belongs to a group (category). If no group is specified, then it will be 
 * put in the default group. Each item also knows its owning entity and the
 * entity it references. When an entity is destroyed, it will also destroy
 * its item. Therefore you should never keep a pointer of an item in memory
 * and always fetch it from the inventory when you need it, as you can never 
 * know for sure, that the respective entity hasn't been destroyed yet (or
 * the item itself).
 *
 * Categories and inventories are removed when they are empty. This happens
 * in ChangeInventoryItemCount() in the idPlayer code.
 *
 * Inventories are accessed via cursors. This way you can have multiple
 * cursors pointing to the same inventory with different tasks. You can
 * also add a ignore filter, so that a given cursor can only cycle through
 * a subset of the existing categories in a given inventory.
 *
 * At the time of writing, there are two implementations of CInventoryItem:
 * The default implementation for loot and ordinary inventory items and
 * items representing weapons (CInventoryWeaponItem). The WeaponItem extend
 * the interface and member variables with weapon-related stuff like ammo-handling.
 * For instance: The idWeapon class requests the active WeaponItem 
 * from the idPlayer class to check whether the active weapon is ready to fire.
 */

/**
 * An inventory is a container for groups of items. An inventory has one default group
 * which is always created. All other groups are up to the mapper to decide.
 *
 * If an item is put into an inventory, without specifying the group, it will be put
 * in the default group which is always at index 0 and has the name DEFAULT.
 */
class CInventory
{
public:
	CInventory();
	~CInventory();

	void					Clear();

	CInventoryCursorPtr		CreateCursor();

	/**
	 * Retrieves the cursor with the given Id or NULL if the Id doesn't exist
	 */
	CInventoryCursorPtr		GetCursor(int id);

	int						GetLoot(int& gold, int& jewelry, int& goods);
	void					SetLoot(int gold, int jewelry, int goods);

	idEntity*				GetOwner() { return m_Owner.GetEntity(); };
	void					SetOwner(idEntity* owner);

	/**
	 * CreateCategory creates the named group if it doesn't already exist.
	 */
	CInventoryCategoryPtr	CreateCategory(const idStr& categoryName, int* index = NULL);

	/**
	 * greebo: Removes the category from this inventory. Will NOT check whether the
	 *         the category is empty or not.
	 */
	void					RemoveCategory(const CInventoryCategoryPtr& category);

	/**
	 * GetCategory returns the pointer to the given group and its index, 
	 * if the pointer is not NULL.
	 */
	CInventoryCategoryPtr	GetCategory(const idStr& categoryName, int* index = NULL);

	/**
	 * Retrieves the category with the given index. Useful for scriptevents to 
	 * reference a certain inventory category.
	 *
	 * @returns: NULL, if the category with the given index was not found.
	 */
	CInventoryCategoryPtr	GetCategory(int index) const;

	/**
	 * GetCategoryIndex returns the index to the given group or -1 if not found.
	 */
	int						GetCategoryIndex(const idStr& categoryName);
	int						GetCategoryIndex(const CInventoryCategoryPtr& category);

	/**
	 * Return the groupindex of the item or -1 if it doesn't exist. Optionally
	 * the itemindex within that group can also be obtained. Both are set to -1 
	 * if the item can not be found. The itemIndex pointer only when it is not NULL of course.
	 */
	int						GetCategoryItemIndex(const CInventoryItemPtr& item, int* itemIndex = NULL);
	int						GetCategoryItemIndex(const idStr& itemName, int* itemIndex = NULL);

	/**
	 * Remove entity from map will remove the entity from the map. If 
	 * bDelete is set, the entity will be deleted as well. This can only
	 * be done if droppable is set to 0 as well. Otherwise it may be
	 * possible for the player to drop the item, in which case the entity 
	 * must stay around.
	 */
	static void				RemoveEntityFromMap(idEntity *ent, bool bDelete = false);

	/**
	 * Put an item in the inventory. Use the default group if none is specified.
	 * The name, that is to be displayed on a GUI, must be set on the respective
	 * entity. Non-existent categories will be created, provided the specified name
	 * is not empty.
	 *
	 * greebo: This routine basically checks all the spawnargs, determines the 
	 * inventory category, the properties like "droppable" and such.
	 * If the according spawnarg is set, the entity is removed from the map.
	 * This can either mean "hide" or "delete", depending on the stackable property.
	 */
	CInventoryItemPtr		PutItem(idEntity *Item, idEntity *Owner);
	void					PutItem(const CInventoryItemPtr& item, const idStr& category);

	/**
	 * greebo: This replaces the inventory item (represented by oldItem) with the
	 * given <newItem> entity. <newItem> needs to be a valid inventory item entity or NULL.
	 *
	 * <oldItem> is removed from the inventory and <newItem> will take its position, provided
	 * both items share the same category name. If the categories are different, the position
	 * can not be guaranteed to be the same after the operation.
	 *
	 * If <newItem> is NULL, <oldItem> will just be removed and no replacement takes place.
	 *
	 * @returns: TRUE on success, FALSE otherwise.
	 */
	bool					ReplaceItem(idEntity* oldItemEnt, idEntity* newItemEnt);

	/**
	 * Dragofer: tests whether the player has an item entity in his inventory
	 */
	bool					HasItem(idEntity* itemEnt);

	/**
	 * greebo: Removes the given inventory item and updates all cursors pointing to it.
	 */
	void					RemoveItem(const CInventoryItemPtr& item);

	/**
	 * Retrieve an item from an inventory. If no group is specified, all of 
	 * them are searched, otherwise only the given group.
	 */
	CInventoryItemPtr		GetItem(const idStr& Name, const idStr& Category = "", bool bCreateCategory = false);

	CInventoryItemPtr		GetItemById(const idStr& Name, const idStr& Category = "", bool bCreateCategory = false);

	// Get the first item in the inventory, by type
	CInventoryItemPtr		GetItemByType(CInventoryItem::ItemType type);

	void					Save(idSaveGame *savefile) const;
	void					Restore(idRestoreGame *savefile);

	/**
	 * Returns the highest active cursor index associated to this inventory
	 */
	int						GetHighestCursorId();

	/**
	 * greebo: Returns a new unique cursor ID.
	 */
	int						GetNewCursorId();

	/**
	 * greebo: Returns the number of categories in this inventory.
	 */
	int						GetNumCategories() const;

	/**
	 * greebo: Copies all inventory items from this inventory to the given targetInventory.
	 * Items are copied by reference (they are handled via smart pointers), so no actual
	 * item instances need to be copy-constructed. The categories in the target inventory
	 * will be created on-demand during copying.
	 */
	void					CopyTo(CInventory& targetInventory);

	/**
	 * greebo: Copies all inventory items from the given sourceInventory that are marked
	 * as persistent (i.e. have GetPersistentCount() > 0). No items are deleted from the source.
	 * The given newOwner entity is set to the new owner for all the copied items.
	 */
	void					CopyPersistentItemsFrom(const CInventory& sourceInventory, idEntity* newOwner);

	// Save the spawnargs of persistent items. This is needed for respawning them in the next mission
	void					SaveItemEntities(bool persistentOnly = true);

	// Restore the item entities at the given position in the map
	void					RestoreItemEntities(const idVec3& entPosition);

private:

	/**
	 * greebo: If this inventory is owned by a player, a HUD message is sent back to the player.
	 */
	void					NotifyOwnerAboutPickup(const idStr& message, const CInventoryItemPtr& pickedUpItem);

	/**
	 * greebo: This checks the given entity for loot items and adds the loot value 
	 *         to the loot sum.
	 * 
	 * @returns: The standard loot info InventoryItem or NULL if the item is not a valid loot item.
	 */
	CInventoryItemPtr		ValidateLoot(idEntity *ent, const bool gotFromShop); // grayman (#2376)

	/**
	 * greebo: Checks the given entity for ammo definitions. Does not remove the entity.
	 *
	 * @returns: The weaponItem the ammo has been added to or NULL, if <ent> isn't a valid ammo item.
	 */
	CInventoryItemPtr		ValidateAmmo(idEntity* ent, const bool gotFromShop); // grayman (#2376)

	/**
	 * greebo: Checks the given entity for weapon definitions. Does not remove the entity.
	 *
	 * @returns: The weaponItem which has been added or enabled in the inventory or NULL, 
	 * if <ent> isn't a valid ammo item.
	 */
	CInventoryItemPtr		ValidateWeapon(idEntity* ent, const bool gotFromShop); // grayman (#2376)

private:
	idEntityPtr<idEntity>				m_Owner;

	idList<CInventoryCursorPtr>			m_Cursor;
	int									m_HighestCursorId;

	/**
	 * List of groups in that inventory
	 */
	idList<CInventoryCategoryPtr>		m_Category;

	/**
	 * Here we keep the lootcount for the items, that don't need to actually 
	 * be stored in the inventory, because they can't get displayed anyway.
	 * LootItemCount will only count the items, that are stored here. Items
	 * that are visible, will not be counted here. This is only for stats.
	 */
	int		m_LootItemCount;
	int		m_Gold;
	int		m_Jewelry;
	int		m_Goods;
};
typedef std::shared_ptr<CInventory> CInventoryPtr;

#endif /* __DARKMOD_INVENTORY_H__ */
