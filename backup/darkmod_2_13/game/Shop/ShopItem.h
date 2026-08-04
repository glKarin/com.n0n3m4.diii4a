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

#ifndef __SHOPITEM_H__
#define	__SHOPITEM_H__


// Represents an item for sale
class CShopItem
{
private:
	idStr		id;
	idStr		name;
	idStr		description;
	int			cost;
	idStr		image;
	int			count;
	bool		persistent;
	bool		canDrop;
	int			dropped;	// tels if dropped, store how many we had so player can undo drop

	// The list of entityDef names to add to the player's inventory 
	// when this shop item is purchased
	idStringList classNames;
	
	bool		stackable; // grayman (#2376)

public:
	CShopItem();

	CShopItem(const idStr& id, 
			  const idStr& name, 
			  const idStr& description, 
			  int cost,
			  const idStr& image, 
			  int count, 
			  bool persistent = false, 
			  bool canDrop = true,
			  bool stackable = false); // grayman (#2376)

	CShopItem(const CShopItem& item, 
			  int count, 
			  int cost = 0, 
			  bool persistent = false);

	// unique identifier for this item
	const idStr& GetID() const;

	// name of the item (for display), possible translated
	const idStr GetName() const;

	// description of the item (for display), possible translated
	const idStr GetDescription() const;

	// Get the list of classnames of entities to spawn for this shop item
	const idStringList& GetClassnames() const;

	// Adds a new classname for this shop item to be added to the player's inventory
	void AddClassname(const idStr& className);

	// cost of the item
	int GetCost();	

	// modal name (for displaying)
	const idStr& GetImage() const;

	// number of the items in this collection (number for sale,
	// or number user has bought, or number user started with)
	int GetCount();				

	// if starting item and it was dropped, the count before the drop (so we can undrop it)
	int GetDroppedCount();				

	// whether the item can be carried to the next mission
	bool GetPersistent();

	// whether the item can dropped by the player from the starting items list
	bool GetCanDrop();
	void SetCanDrop(bool canDrop);

	// grayman (#2376) - whether the item can be stacked
	bool GetStackable();
	void SetStackable(bool stackable);

	// modifies number of items
	void ChangeCount( int amount );

	// sets dropped => count and count => 0
	void Drop( void );

	// sets count => dropped and dropped => 0
	void Undrop( void );

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);
};
typedef std::shared_ptr<CShopItem> CShopItemPtr;

// A list of shop items
typedef idList<CShopItemPtr> ShopItemList;

#endif
