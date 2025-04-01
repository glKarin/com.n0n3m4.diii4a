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

#ifndef __SHOP_H__
#define	__SHOP_H__

#define LIST_SIZE_FOR_SALE 11
#define LIST_SIZE_PURCHASED 9
#define LIST_SIZE_STARTING 7

#include "ShopItem.h"
#include "../DifficultyManager.h"
#include "LootRuleSet.h"

// Represents the Shop
class CShop
{
private:
	ShopItemList	_itemDefs;
	ShopItemList	_itemsForSale;
	ShopItemList	_itemsPurchased;
	ShopItemList	_startingItems;

	int				_gold;
	int				_forSaleTop;
	int				_purchasedTop;
	int				_startingTop;

	// True if the purchase menu should be skipped
	bool			_skipShop;
	
	// grayman (#2376) - Lockpick handling
	bool			_pickSetShop;
	bool			_pickSetStarting;

	// The non-difficulty-specific loot rules, apply if not overridden by diff-specific ones
	LootRuleSet		_generalLootRules;

	// The difficulty-specific loot rules, will override generalLootRules
	LootRuleSet		_diffLootRules[DIFFICULTY_COUNT];

public:
	void Init();

	// Clears out all lists
	void Clear();

	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);

	// handles main menu commands
	void HandleCommands(const char *menuCommand, idUserInterface *gui);

	// how much gold the player has left to spend
	int GetGold();
	void SetGold(int gold);
	void ChangeGold(int change);

	// put item in the For Sale list
	void AddItemForSale(const CShopItemPtr& shopItem);

	// put item in the Starting Items list
	void AddStartingItem(const CShopItemPtr& shopItem);

	// initializes the 'list' based on the map
	int AddItems(const idDict& mapDict, const idStr& itemKey, ShopItemList& list);

	// returns the various lists
	const ShopItemList& GetItemsForSale();
	const ShopItemList& GetStartingItems();
	const ShopItemList& GetPurchasedItems();

	// returns the combination of For Sale and Starting items
	const ShopItemList& GetPlayerStartingEquipment() const;

	// adjust the lists
	void SellItem(int index);
	void BuyItem(int index);
	void DropUndropItem(int index);

	// update the GUI variables to match the shop
	void UpdateGUI(idUserInterface* gui);

	// find items based on the id
	CShopItemPtr FindPurchasedByID(const char *id);
	CShopItemPtr FindForSaleByID(const char *id);
	CShopItemPtr FindStartingItemByID(const char *id);

	CShopItemPtr FindByID(ShopItemList& items, const char *id);

	// Finds a shop item definition for the given classname. If the direct match cannot be found
	// this routine tries another time with "atdm:" prepended
	CShopItemPtr FindShopItemDefByClassName(const idStr& className);

	/**
	 * greebo: Calculates the quantity for the item that can be added to the shop.
	 * Weapon items return their ammonition count for ranged weapons, or 1 for melee weapons.
	 * Ordinary items just return their persistent inventory item count
	 * This routine can be used to judge whether this item would be added to the shop, in combination
	 * with the FindShopItemDefByClassName() routine.
	 */
	static int GetQuantityForItem(const CInventoryItemPtr& item);

	// initialize the shop
	void DisplayShop(idUserInterface *gui);

	// true if there are no items for sale
	bool GetNothingForSale();

	// grayman #3723 - tell us if there's a shop
	bool ShopExists() { return !_skipShop; };

private:
	// read from defs and map to initialze the shop
	void LoadShopItemDefinitions();

	// Add the items from the persistent inventory to the starting equipment
	void AddPersistentStartingEquipment();

	// grayman (#2376) - put inv_map_start entities in the Starting Items list
	void AddMapItems(idMapFile* mapFile);

	// greebo: Tries to merge the named shopitem (with the given quantity) into the existing starting equipment
	// Returns TRUE if the quantity was merged into the list, FALSE if the item doesn't exist yet
	bool MergeIntoStartingEquipment(const idStr& itemName, int quantity, bool isWeapon, bool isMeleeWeapon);

	// grayman (#2376) - check for individual lockpicks
	void CheckPicks(ShopItemList& list);

	// scroll a list to the next "page" of values, returns the new top index
	int ScrollList(int topItem, int maxItems, ShopItemList& list);

	// Load all data from shop entities and worldspawn of the given map
	void LoadFromMap(idMapFile* mapFile);

	// Load all shop and starting items from the given spawnargs
	void LoadFromDict(const idDict& dict);

	// Copies purchasedItems into startingItems
	void CopyPurchasedIntoStartingEquipment();

	// grayman - Provides max_ammo value
	int GetMaxAmmo(const idStr& weaponName);

	// Adds the gold from the previous mission
	void AddGoldFromPreviousMission();

	// Load loot rules from the given map dict
	void LoadLootRules(const idDict& dict);
};

#endif	/* !__SHOP_H__ */
