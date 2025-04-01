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



#include "Cursor.h"

#include "Inventory.h"

CInventoryCursor::CInventoryCursor(CInventory* inventory, int id) :
	m_Inventory(inventory),
	m_CategoryLock(false),	// Default behaviour ...
	m_WrapAround(true),		// ... is like standard Thief inventory.
	m_CurrentCategory(0),
	m_CurrentItem(0),
	m_CursorId(id)
{}

int	CInventoryCursor::GetId() const
{
	return m_CursorId;
}

void CInventoryCursor::Save(idSaveGame *savefile) const
{
	savefile->WriteBool(m_CategoryLock);
	savefile->WriteBool(m_WrapAround);
	savefile->WriteInt(m_CurrentCategory);
	savefile->WriteInt(m_CurrentItem);
	savefile->WriteInt(m_CursorId);

	savefile->WriteInt(m_CategoryIgnore.Num());
	for (int i = 0; i < m_CategoryIgnore.Num(); i++)
	{
		savefile->WriteInt(m_CategoryIgnore[i]);
	}
}

void CInventoryCursor::Restore(idRestoreGame *savefile)
{
	savefile->ReadBool(m_CategoryLock);
	savefile->ReadBool(m_WrapAround);
	savefile->ReadInt(m_CurrentCategory);
	savefile->ReadInt(m_CurrentItem);
	savefile->ReadInt(m_CursorId);

	int num;
	savefile->ReadInt(num);
	for (int i = 0; i < num; i++)
	{
		int ignoreIndex;
		savefile->ReadInt(ignoreIndex);
		m_CategoryIgnore.AddUnique(ignoreIndex);
	}
}

CInventoryItemPtr CInventoryCursor::GetCurrentItem()
{
	// Return an item if the inventory has items
	CInventoryCategoryPtr currentCategory = m_Inventory->GetCategory(m_CurrentCategory);
	if (currentCategory) {
		return currentCategory->GetItem(m_CurrentItem);
	}
	return CInventoryItemPtr();
}

void CInventoryCursor::ClearItem()
{
	// stifu #2993: Make sure the item index is always valid. Switch to dummy item
	// WARNING: Implicit logic - Assumes TDM_DUMMY_ITEM always exists
	if (!SetCurrentItem(TDM_DUMMY_ITEM))
	{
		m_CurrentItem = 0;
		m_CurrentCategory = 0;
	}	
}

bool CInventoryCursor::SetCurrentItem(CInventoryItemPtr item)
{
	if (item == NULL)
	{
		// NULL item passed, which means "clear cursor": replace the pointer with a pointer to the dummy item
		item = m_Inventory->GetItem(TDM_DUMMY_ITEM);

		if (item == NULL) return false;
	}

	// retrieve the category and item index for the given inventory item
	int itemIdx = -1;
	int category = m_Inventory->GetCategoryItemIndex(item, &itemIdx);

	if (category == -1) return false; // category not found

	// Only change the group and item indices if they are valid.
	m_CurrentCategory = category;
	m_CurrentItem = itemIdx;

	return true;
}

bool CInventoryCursor::SetCurrentItem(const idStr& itemName)
{
	if (itemName.IsEmpty()) return false;

	int itemIdx = -1;
	int category = m_Inventory->GetCategoryItemIndex(itemName, &itemIdx);

	if (category == -1) return false; // category not found

	// Only change the group and item indices if they are valid.
	m_CurrentCategory = category;
	m_CurrentItem = itemIdx;

	return true;
}


void CInventoryCursor::SetCurrentItem(int index)
{
	m_CurrentItem = index;

	Validate();
}


void CInventoryCursor::Validate()
{
	CInventoryCategoryPtr pCurCategory = m_Inventory->GetCategory(m_CurrentCategory);

	if (pCurCategory == nullptr)
	{
		ClearItem();
		return;
	}

	if (pCurCategory->GetNumItems() == 0)
	{
		ClearItem();
		return;
	}

	m_CurrentItem = idMath::ClampInt(0, pCurCategory->GetNumItems() - 1, m_CurrentItem);
}

CInventoryItemPtr CInventoryCursor::GetNextItem()
{
	CInventoryCategoryPtr curCategory = m_Inventory->GetCategory(m_CurrentCategory);

	if (curCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Current Category doesn't exist anymore!\r", m_CurrentCategory);
		return CInventoryItemPtr();
	}

	// Advance our cursor
	m_CurrentItem++;

	// Have we reached the end of the current category?
	if (m_CurrentItem >= curCategory->GetNumItems())
	{
		// Advance to the next allowed category
		curCategory = GetNextCategory();
		if (curCategory->GetNumItems() == 0)
		{
			ClearItem();
			return GetCurrentItem();
		}

		if (m_WrapAround) 
		{
			m_CurrentItem = 0;
		}
		else
		{
			m_CurrentItem = curCategory->GetNumItems() - 1;
		}
	}

	return curCategory->GetItem(m_CurrentItem);
}

CInventoryItemPtr CInventoryCursor::GetPrevItem()
{
	CInventoryCategoryPtr curCategory = m_Inventory->GetCategory(m_CurrentCategory);

	if (curCategory == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Current Category doesn't exist anymore!\r", m_CurrentCategory);
		return CInventoryItemPtr();
	}

	// Move our cursor backwards
	m_CurrentItem--;

	if (m_CurrentItem < 0)
	{
		curCategory = GetPrevCategory();
		if (curCategory->GetNumItems() == 0)
		{
			ClearItem();
			return GetCurrentItem();
		}

		if (m_WrapAround)
		{
			m_CurrentItem = curCategory->GetNumItems() - 1;
		}
		else 
		{
			// Not allowed to wrap around.
			m_CurrentItem = 0;
			return CInventoryItemPtr();
		}
	}

	return curCategory->GetItem(m_CurrentItem);
}

CInventoryCategoryPtr CInventoryCursor::GetNextCategory()
{
	if (m_CategoryLock) 
	{
		// Category lock is switched on, we don't allow to switch to another category.
		return m_Inventory->GetCategory(m_CurrentCategory);
	}

	int cnt = 0;

	CInventoryCategoryPtr rc;

	int n = m_Inventory->GetNumCategories() - 1;

	if (n < 0) n = 0;

	while (true)
	{
		m_CurrentCategory++;

		// Check if we already passed through all the available categories.
		// This means that either the inventory is quite empty, or there are
		// no categories that are allowed for this cursor.
		cnt++;
		if(cnt > n)
		{
			rc = CInventoryCategoryPtr();
			m_CurrentCategory = 0;
			break;
		}

		if(m_CurrentCategory > n)
		{
			if(m_WrapAround == true)
				m_CurrentCategory = 0;
			else
				m_CurrentCategory = n;
		}

		rc = m_Inventory->GetCategory(m_CurrentCategory);

		if (rc != NULL && !IsCategoryIgnored(rc) && !rc->IsEmpty())
		{
			break; // We found a suitable category (not ignored and not empty)
		}
	}

	// Finally, ensure that the item index is reset to 0
	m_CurrentItem = 0;

	return rc;
}

CInventoryCategoryPtr CInventoryCursor::GetPrevCategory()
{
	CInventoryCategoryPtr rc;

	// If category lock is switched on, we don't allow to switch 
	// to another category.
	if (m_CategoryLock == true)
		return CInventoryCategoryPtr();

	int n = m_Inventory->GetNumCategories();
	int cnt = 0;

	n--;
	if(n < 0)
		n = 0;

	while(1)
	{
		m_CurrentCategory--;

		// Check if we already passed through all the available categories.
		// This means that either the inventory is quite empty, or there are
		// no categories that are allowed for this cursor.
		cnt++;
		if(cnt > n)
		{
			rc = CInventoryCategoryPtr();
			m_CurrentCategory = 0;
			break;
		}

		if(m_CurrentCategory < 0)
		{
			if(m_WrapAround == true)
				m_CurrentCategory = n;
			else
				m_CurrentCategory = 0;
		}

		rc = m_Inventory->GetCategory(m_CurrentCategory);

		if (rc != NULL && !IsCategoryIgnored(rc) && !rc->IsEmpty())
		{
			break; // We found a suitable category (not ignored and not empty)
		}
	}

	// Finally, ensure that the item index is reset to 0
	m_CurrentItem = 0;

	return rc;
}

void CInventoryCursor::SetCurrentCategory(int index)
{
	// Ensure the index is within bounds
	index = idMath::ClampInt(0, m_Inventory->GetNumCategories() - 1, index);

	m_CurrentCategory = index;
	m_CurrentItem = 0;
}

void CInventoryCursor::SetCurrentCategory(const idStr& categoryName)
{
	int index = m_Inventory->GetCategoryIndex(categoryName);

	if (index != -1)
	{
		m_CurrentCategory = index;
		m_CurrentItem = 0;
	}
}

void CInventoryCursor::AddCategoryIgnored(const CInventoryCategoryPtr& category)
{
	if (category != NULL)
	{
		m_CategoryIgnore.AddUnique( m_Inventory->GetCategoryIndex(category) );
	}
}

void CInventoryCursor::AddCategoryIgnored(const idStr& categoryName)
{
	if (categoryName.IsEmpty()) return;

	// Resolve the name and pass the call to the overload
	AddCategoryIgnored( m_Inventory->GetCategory(categoryName) );
}

void CInventoryCursor::RemoveCategoryIgnored(const CInventoryCategoryPtr& category)
{
	int categoryIndex = m_Inventory->GetCategoryIndex(category);
	m_CategoryIgnore.Remove(categoryIndex);
}

void CInventoryCursor::RemoveCategoryIgnored(const idStr& categoryName)
{
	if (categoryName.IsEmpty()) return;

	// Resolve the name and pass the call to the overload
	RemoveCategoryIgnored( m_Inventory->GetCategory(categoryName) );
}

bool CInventoryCursor::IsCategoryIgnored(const CInventoryCategoryPtr& category) const
{
	int categoryIndex = m_Inventory->GetCategoryIndex(category);

	return (m_CategoryIgnore.FindIndex(categoryIndex) != -1);
}

CInventoryCategoryPtr CInventoryCursor::GetCurrentCategory() const
{
	return (m_Inventory != NULL) ? m_Inventory->GetCategory(m_CurrentCategory) : CInventoryCategoryPtr();
}

bool CInventoryCursor::IsLastItemInCategory() const
{
	if (m_Inventory == NULL) return true; // no inventory => last item

	CInventoryCategoryPtr curCat = m_Inventory->GetCategory(m_CurrentCategory);

	return (curCat != NULL) ? m_CurrentItem + 1 >= curCat->GetNumItems() : true;
}
