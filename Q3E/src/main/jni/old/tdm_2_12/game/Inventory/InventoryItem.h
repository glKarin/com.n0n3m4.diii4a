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
#ifndef __DARKMOD_INVENTORYITEM_H__
#define __DARKMOD_INVENTORYITEM_H__

#include "LootType.h"

/* FORWARD DECLS */
class CInventoryCategory;

/**
 * InventoryItem is an item that belongs to a group.
 */
class CInventoryItem
{
public:
	enum ItemType
	{
		IT_ITEM,			// Normal item, which is associated to an entity
		IT_LOOT,			// this is a loot item
		IT_LOOT_INFO,		// loot info item
		IT_WEAPON,			// a weapon item
		IT_DUMMY,			// This also doesn't have an entity, but provides a dummy so 
							// we can have an empty space in the inventory.
		IT_COUNT
	};

public:
	CInventoryItem(idEntity *m_Owner);

	// Create an inventoryitem out of the given entity and the given owner
	CInventoryItem(idEntity* itemEntity, idEntity* owner);

	virtual void			Save( idSaveGame *savefile ) const;
	virtual void			Restore( idRestoreGame *savefile );

	CInventoryCategory*		Category() const { return m_Category; }
	void					SetCategory(CInventoryCategory* newCategory) { m_Category = newCategory; };
	
	idEntity*				GetOwner() { return m_Owner.GetEntity(); };
	void					SetOwner(idEntity* newOwner) { m_Owner = newOwner; };

	void					SetItemEntity(idEntity *ent) { m_Item = ent; };
	idEntity*				GetItemEntity() { return m_Item.GetEntity(); }

	// Stores the item entity's spawnargs locally - used before ending a mission to prepare an item/entity transfer
	// Does nothing if the item entity is NULL
	virtual void			SaveItemEntityDict();

	// Restores the item entity from the saved dictionary. Does nothing if the saved dictionary is empty.
	// The position is needed to place the respawned entity somewhere valid
	virtual void			RestoreItemEntityFromDict(const idVec3& entPosition);

	// Get a reference to the saved item entity dictionary. Returns NULL if the spawnargs didn't get saved
	// This usually returns non-NULL for items stored in the game's persistent inventory, for inter-mission item transfer
	const idDict*			GetSavedItemEntityDict() const;

	void					SetType(CInventoryItem::ItemType type) { m_Type = type; };
	ItemType				GetType() { return m_Type; };

	int						GetCount() { return m_Count; };
	void					SetCount(int Amount);

	bool					IsStackable() { return m_Stackable; };
	void					SetStackable(bool);

	void					SetDroppable(bool bDroppable) { m_Droppable = bDroppable; };
	bool					IsDroppable() { return m_Droppable; };

	void					SetLootType(LootType t);
	LootType				GetLootType() { return m_LootType; };

	void					SetValue(int n);
	int						GetValue() { return m_Value; };

	void					SetName(const idStr &n) { m_Name = n; };
	const idStr&			GetName() { return m_Name; };

	void					SetItem(idEntity *item) { m_Item = item; };
	idEntity*				GetItem() { return m_Item.GetEntity(); };

	int						GetOverlay() { return m_Overlay; };
	void					SetOverlay(const idStr &HudName, int Overlay);
	bool					HasHUD() { return m_Hud; };
	void					SetHUD(const idStr &HudName, int layer);
	const idStr&			GetHUD() { return m_HudName; };

	const idStr&			GetIcon() { return m_Icon; };
	void					SetIcon(const idStr& newIcon);

	void					SetItemId(const idStr &id) { m_ItemId = id; };
	const idStr&			GetItemId() { return m_ItemId; };

	static LootType			GetLootTypeFromSpawnargs(const idDict& spawnargs);

	/**
	 * greebo: This returns the number of persistent items contained in this InventoryItem.
	 *         For ordinary persistent items, this is always 1, for non-persistent items this is 0.
	 *         Stackable persistent items will return the current stack count.
	 */
	int						GetPersistentCount();

	bool					IsPersistent() const { return m_Persistent; };
	void					SetPersistent(bool newValue);

	/**
	 * greebo: When this item is active, the lightgem can be modified by this value.
	 *         The integer value should be between 0 and DARKMOD_LG_MAX and is added to the
	 *         the result of the lightgem renderpasses. Corresponds to the spawnarg "inv_lgmodifier"
	 */
	int						GetLightgemModifier() { return m_LightgemModifier; }
	void					SetLightgemModifier(int newValue);

	/**
	 * greebo: When this item is active, the owner can be encumbered (or even accelerated?)
	 * by a certain factor. Each item can have its own movement modifier, which is defined
	 * in the "inv_movement_modifer" spawnarg.
	 *
	 * @modifier value range: a floating point variable in [0, INF]. This value is applied to
	 * the maximum player movement speed by multiplication.
	 */
	float					GetMovementModifier() { return m_MovementModifier; }
	void					SetMovementModifier(float newValue);
	/**
	* Get/set the drop orientation
	**/
	const idMat3&			GetDropOrientation() { return m_DropOrientation; }
	void					SetDropOrientation(const idMat3& newAxis);

	// Returns true when this item should be used by the 'frob' impulse
	bool					UseOnFrob() const { return m_UseOnFrob; }

	// Returns the frob distance cap. If this item doesn't enforce such a value, this returns idMath::INFINITY.
	float					GetFrobDistanceCap()
	{ 
		return (m_FrobDistanceCap == -1) ? idMath::INFINITY : m_FrobDistanceCap;
	}

protected:
	// Reads the values from the given spawnargs into the member variables
	void					ParseSpawnargs(const idDict& spawnArgs);

	// Calls the owner's OnInventoryItemChanged() method
	void					NotifyItemChanged();

protected:
	idEntityPtr<idEntity>	m_Owner;
	idEntityPtr<idEntity>	m_Item;

	// greebo: Optional item dictionary. This is used to transfer inventory items between missions
	// including their item entities. These entities will be saved at mission end and re-spawned at the 
	// start of the next mission.
	std::shared_ptr<idDict> m_ItemDict;

	idEntityPtr<idEntity>	m_BindMaster;
	idStr					m_Name;
	idStr					m_HudName;		// filename for the hud file if it has a custom hud
	idStr					m_ItemId;		// Arbitrary Id, that the mapper can use to idenitfy an item.
											// It is also needed to identify items which are stackable
											// to make them identifiable. This can be used, for example
											// to create a fake health potion which shows up in the inventory
											// as a regular health potion, but actually deals damage. Needs
											// custom scripting to actually do this though.
	CInventoryCategory*		m_Category;
	ItemType				m_Type;
	LootType				m_LootType;
	int						m_Value;
	int						m_Overlay;
	int						m_Count;		// How many of that item are currently represented (i.e. Arrows)
	bool					m_Stackable;	// Counter can be used if true, otherwise it's a unique item
	bool					m_Droppable;		// If the item is not dropable it will be inaccessible after it 
											// is put into the inventory
	bool					m_Hud;
	idStr					m_Icon;			// The inventory icon string
	bool					m_Orientated;	// Taken from the entity

	bool					m_Persistent;	// Can be taken to the next map (default is FALSE)

	int						m_LightgemModifier; // Is added to the lightgem when this item is active

	// A value in [0,1] defining the fraction of the regular movement speed
	// which the owner is allowed to move with when this item is equipped.
	float					m_MovementModifier; 

	/**
	 * greebo: When this inventory item is selected, the frob distance can not be higher
	 * than the value defined here. Is -1 if not set.
	 */
	float					m_FrobDistanceCap;

	bool					m_UseOnFrob;	// Whether this item can be used by the 'frob' button

	/**
	* Which way the object should orient itself when dropping
	* Relative to the player's view axis (x,y,z = forward,left,up)
	* Typically it maintains the same relative orientation to the player
	* that it had when the player picked it up.
	* Except for pitch, which is the same as its original pitch
	**/
	idMat3					m_DropOrientation;
};
typedef std::shared_ptr<CInventoryItem> CInventoryItemPtr;

#endif /* __DARKMOD_INVENTORYITEM_H__ */
