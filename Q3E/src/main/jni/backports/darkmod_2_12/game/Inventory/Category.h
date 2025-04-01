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
#ifndef __DARKMOD_INVENTORYCATEGORY_H__
#define __DARKMOD_INVENTORYCATEGORY_H__

#include "InventoryItem.h"

/**
 * InventoryCategory is just a container for items that are currently held by an entity.
 * We put the constructor and PutItem into the protected scope, so we can ensure
 * that groups are only created by using the inventory class.
 */
class CInventoryCategory
{
public:
	CInventoryCategory(CInventory* inventory, const idStr& name = "");
	~CInventoryCategory();

	const idStr&			GetName() { return m_Name; }

	void					SetInventory(CInventory *Inventory) { m_Inventory = Inventory; };

	idEntity*				GetOwner() { return m_Owner.GetEntity(); };
	void					SetOwner(idEntity *Owner);

	// Look up an InventoryItem by its ItemId (NOT equivalent to GetItem(const idStr& Name) btw).
	CInventoryItemPtr		GetItemById(const idStr& id);

	CInventoryItemPtr		GetItemByType(CInventoryItem::ItemType type);

	// Look up an InventoryItem by <name> or <index>
	CInventoryItemPtr		GetItem(const idStr& itemName);
	CInventoryItemPtr		GetItem(int index);

	// Returns the index of the given item or the item with the given name. Returns -1 if not found.
	int						GetItemIndex(const idStr& itemName);
	int						GetItemIndex(const CInventoryItemPtr& item);

	// Returns the sum of all gold, loot and jewelry of this category plus the gold, loot, jewelry sums on their own.
	int						GetLoot(int& gold, int& jewelry, int& goods);

	/**
	 * greebo: Adds the given item to this category, behaves like std::list::push_front()
	 */
	void					PutItemFront(const CInventoryItemPtr& item)
	{
		PutItem(item, true);
	}

	/**
	 * greebo: Adds the given item to this category, behaves like std::list::push_back()
	 */
	void					PutItemBack(const CInventoryItemPtr& item)
	{
		PutItem(item, false);
	}

	/**
	 * greebo: Removes the specified <item> from this category.
	 */
	void					RemoveItem(const CInventoryItemPtr& item);

	/**
	 * greebo: Swaps the position of the given two inventory items. Returns TRUE on success.
	 */
	bool					SwapItemPosition(const CInventoryItemPtr& item1, const CInventoryItemPtr& item2);

	/** greebo: Returns true if the category contains no items.
	 */
	bool					IsEmpty() const;

	/**
	 * Returns the number of items in this category.
	 */
	int						GetNumItems() const;

	void					Save(idSaveGame *savefile) const;
	void					Restore(idRestoreGame *savefile);

private:
	void					PutItem(const CInventoryItemPtr& item, bool insertAtFront);

private:
	CInventory*				m_Inventory;			// The inventory this group belongs to.
	idEntityPtr<idEntity>	m_Owner;

	// The name of this group.
	idStr					m_Name;

	// A list of contained items (are deleted on destruction of this object).
	idList<CInventoryItemPtr>	m_Item;
};
typedef std::shared_ptr<CInventoryCategory> CInventoryCategoryPtr;

#endif /* __DARKMOD_INVENTORYCATEGORY_H__ */
