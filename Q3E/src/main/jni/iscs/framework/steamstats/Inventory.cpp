/*
===========================================================================

Icarus Starship Command Simulator GPL Source Code
Copyright (C) 2017 Steven Eric Boyette.

This file is part of the Icarus Starship Command Simulator GPL Source Code (?Icarus Starship Command Simulator GPL Source Code?).

Icarus Starship Command Simulator GPL Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Icarus Starship Command Simulator GPL Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Icarus Starship Command Simulator GPL Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include <vector>

#ifdef STEAM_BUILD

//#include "stdafx.h"
#include "steam/steam_api.h"
#include "steam/isteamuserstats.h"
#include "steam/isteamremotestorage.h"
#include "steam/isteammatchmaking.h"
#include "steam/steam_gameserver.h"

#include "Inventory.h"
//#include "SpaceWarClient.h"


//-----------------------------------------------------------------------------
// Purpose: singleton instance of CSteamLocalInventory
//-----------------------------------------------------------------------------
CSteamLocalInventory *SteamLocalInventory()
{
	static CSteamLocalInventory inv;
	return &inv;
}

CSteamLocalInventory::CSteamLocalInventory()
:	m_SteamInventoryResult( this, &CSteamLocalInventory::OnSteamInventoryResult ),
	m_SteamInventoryFullUpdate( this, &CSteamLocalInventory::OnSteamInventoryFullUpdate )
{
	// On PC/OSX we always know the user has a SteamID and is logged in already,
	// as Steam enforces this before game launch.  On PS3 however the game must
	// initiate the logon and we need to check the state here and block the user
	// while Steam connects.
	if ( SteamUser()->BLoggedOn() )
	{
		m_SteamIDLocalUser = SteamUser()->GetSteamID();
	}

	m_hPlaytimeRequestResult = k_SteamInventoryResultInvalid;
	m_hPromoRequestResult = k_SteamInventoryResultInvalid;
	m_hLastFullUpdate = k_SteamInventoryResultInvalid;
	m_hExchangeRequestResult = k_SteamInventoryResultInvalid;

	m_LastDropInstanceID = k_SteamItemInstanceIDInvalid;

	// Indicate that this game has a use for item definition properties (we look up "name").
	// If your game hardcodes the complete set of items, then you can probably skip this call.
	SteamInventory()->LoadItemDefinitions();

	// If there are any promotional items which your game offers (or may offer in the future)
	// then this is the call that will grant them. Promotional items are a result of meeting
	// some external criteria like owning another specific game. These criteria are specified
	// in your Steamworks item definitions.
	SteamInventory()->GrantPromoItems( &m_hPromoRequestResult );

#ifdef _DEBUG
	GrantTestItems();
#endif

	// We could pass a variable to receive the result handle, for
	// comparison to the handle in SteamInventoryResultReady_t,
	// but this simple example does not bother to keep track of
	// multiple in-flight API calls.
	SteamInventory()->GetAllItems( NULL ); // this will fire off FullUpdate and then ResultReady
}

//-----------------------------------------------------------------------------
// Purpose: Handles notification that GetAllItems has refreshed the local inventory
//-----------------------------------------------------------------------------
void CSteamLocalInventory::OnSteamInventoryFullUpdate( SteamInventoryFullUpdate_t *callback )
{
	// This callback triggers immediately before the ResultReady callback. We shouldn't
	// free the result handle here, as we wil always free it at the end of ResultReady.

	bool bGotResult = false;
	std::vector<SteamItemDetails_t> vecDetails;
	uint32 count = 0;
	if ( SteamInventory()->GetResultItems( callback->m_handle, NULL, &count ) )
	{
		vecDetails.resize( count );
		bGotResult = SteamInventory()->GetResultItems( callback->m_handle, vecDetails.data(), &count );
	}

	if ( bGotResult )
	{
		// For everything already in the inventory, check for update (exists in result) or removal (does not exist)
		std::list<CSteamItem *>::iterator iter;
		for ( iter = m_listPlayerItems.begin(); iter != m_listPlayerItems.end(); /*incr at end of loop*/ )
		{
			bool bFound = false;
			for ( size_t i = 0; i < vecDetails.size(); i++ )
			{
				if ( (*iter)->GetItemId() == vecDetails[i].m_itemId )
				{
					// Update item with matching item id
					(*iter)->m_Details = vecDetails[i];

					// Remove elements from the result vector as we process updates (fast swap-and-pop removal)
					if ( i < vecDetails.size() - 1 )
						vecDetails[i] = vecDetails.back();
					vecDetails.pop_back();
					
					bFound = true;
					break;
				}
			}

			if ( !bFound )
			{
				// No items in the full update match the existing item. Delete current iterator and advance.
				delete *iter;
				iter = m_listPlayerItems.erase( iter );
			}
			else
			{
				// Increment iterator without deleting.
				++iter;
			}
		}

		// Anything remaining in the result vector is a new item, since we removed all the updates.
		for ( size_t i = 0; i < vecDetails.size(); ++i )
		{
			CSteamItem *item = new CSteamItem();
			item->m_Details = vecDetails[i];
			m_listPlayerItems.push_back( item );
		}
	}

	// Remember that we just processed this full update to avoid doing work in ResultReady
	m_hLastFullUpdate = callback->m_handle;
}

//-----------------------------------------------------------------------------
// Purpose: Handles notification that the inventory is updated
//-----------------------------------------------------------------------------
void CSteamLocalInventory::OnSteamInventoryResult( SteamInventoryResultReady_t *callback )
{
	// Ignore results that belong to some other SteamID - this normally won't happen, unless you start
	// calling SerializeResult/DeserializeResult, but it is better to be safe. Also ignore anything that
	// we just processed in OnSteamInventoryFullUpdate to avoid duplicate work.
	if ( callback->m_result == k_EResultOK && m_hLastFullUpdate != callback->m_handle &&
		 SteamInventory()->CheckResultSteamID( callback->m_handle, GetLocalSteamID() ) )
	{
		bool bGotResult = false;
		std::vector<SteamItemDetails_t> vecDetails;
		uint32 count = 0;
		if ( SteamInventory()->GetResultItems( callback->m_handle, NULL, &count ) )
		{
			vecDetails.resize( count );
			bGotResult = SteamInventory()->GetResultItems( callback->m_handle, vecDetails.data(), &count );
		}

		if ( bGotResult )
		{
			// For everything already in the inventory, check for update or removal
			std::list<CSteamItem *>::iterator iter;
			for ( iter = m_listPlayerItems.begin(); iter != m_listPlayerItems.end(); /*incr at end of loop*/ )
			{
				bool bDestroy = false;
				for ( size_t i = 0; i < vecDetails.size(); i++ )
				{
					if ( (*iter)->GetItemId() == vecDetails[i].m_itemId )
					{
						// If flagged for removal by a partial update, remove it
						if ( vecDetails[i].m_unFlags & k_ESteamItemRemoved )
						{
							bDestroy = true;
						}
						else
						{
							(*iter)->m_Details = vecDetails[i];
						}

						// Remove elements from the result vector as we process updates (fast swap-and-pop removal)
						if ( i < vecDetails.size() - 1 )
							vecDetails[i] = vecDetails.back();
						vecDetails.pop_back();

						break;
					}
				}

				if ( bDestroy )
				{
					// Delete list element at current iterator and advance.
					delete *iter;
					iter = m_listPlayerItems.erase( iter );
				}
				else
				{
					// Increment iterator without deleting.
					++iter;
				}
			}

			// Anything remaining in the result vector is a new item, unless flagged for removal by an operation result.
			for ( size_t i = 0; i < vecDetails.size(); ++i )
			{
				if ( !( vecDetails[i].m_unFlags & k_ESteamItemRemoved ) )
				{
					CSteamItem *item = new CSteamItem();
					item->m_Details = vecDetails[i];
					m_listPlayerItems.push_back( item );
				}
			}
		}
	}

	// Clear out any pending handles.
	if ( callback->m_handle == m_hPlaytimeRequestResult )
		m_hPlaytimeRequestResult = -1;
	if ( callback->m_handle == m_hExchangeRequestResult )
		m_hExchangeRequestResult = -1;
	if ( callback->m_handle == m_hPromoRequestResult )
		m_hPromoRequestResult = -1;
	if ( callback->m_handle == m_hLastFullUpdate )
		m_hLastFullUpdate = -1;

	// We're not hanging on the the result after processing it.
	SteamInventory()->DestroyResult( callback->m_handle );
}

void CSteamLocalInventory::CheckForItemDrops()
{
	SteamInventory()->TriggerItemDrop( &m_hPlaytimeRequestResult, k_SpaceWarItem_TimedDropList );
}

void CSteamLocalInventory::DoExchange()
{
	const CSteamItem *item100 = GetInstanceOf( k_SpaceWarItem_ShipDecoration1 );
	const CSteamItem *item101 = GetInstanceOf( k_SpaceWarItem_ShipDecoration2 );
	const CSteamItem *item102 = GetInstanceOf( k_SpaceWarItem_ShipDecoration3 );
	const CSteamItem *item103 = GetInstanceOf( k_SpaceWarItem_ShipDecoration4 );
	if ( item100 && item101 && item102 && item103 )
	{
		SteamItemInstanceID_t inputItems[4];
		uint32 inputQuantities[4];
		inputItems[0] = item100->GetItemId();
		inputQuantities[0] = 1;
		inputItems[1] = item101->GetItemId();
		inputQuantities[1] = 1;
		inputItems[2] = item102->GetItemId();
		inputQuantities[2] = 1;
		inputItems[3] = item103->GetItemId();
		inputQuantities[3] = 1;
		SteamItemDef_t outputItems[1];
		outputItems[0] = 110;
		uint32 outputQuantity[1];
		outputQuantity[0] = 1;
		SteamInventory()->ExchangeItems( &m_hExchangeRequestResult, outputItems, outputQuantity, 1, inputItems, inputQuantities, 4 );
	}
}

void CSteamLocalInventory::GrantTestItems()
{
	std::vector<SteamItemDef_t> newItems;
	newItems.push_back( k_SpaceWarItem_ShipDecoration1 );
	newItems.push_back( k_SpaceWarItem_ShipDecoration2 );
	SteamInventory()->GenerateItems( NULL, newItems.data(), NULL, (uint32) newItems.size() );
}

const CSteamItem * CSteamLocalInventory::GetItem( SteamItemInstanceID_t nItemId ) const
{
	std::list<CSteamItem *>::const_iterator iter;
	for ( iter = m_listPlayerItems.begin(); iter != m_listPlayerItems.end(); ++iter )
	{
		if ( (*iter)->GetItemId() == nItemId )
			return (*iter);
	}
	return NULL;
}

bool CSteamLocalInventory::HasInstanceOf( SteamItemDef_t nDefinition ) const
{
	std::list<CSteamItem *>::const_iterator iter;
	for ( iter = m_listPlayerItems.begin(); iter != m_listPlayerItems.end(); ++iter )
	{
		if ( ( *iter )->GetDefinition() == nDefinition )
			return true;
	}
	return false;
}

const CSteamItem *  CSteamLocalInventory::GetInstanceOf( SteamItemDef_t nDefinition ) const
{
	std::list<CSteamItem *>::const_iterator iter;
	for ( iter = m_listPlayerItems.begin(); iter != m_listPlayerItems.end(); ++iter )
	{
		if ( ( *iter )->GetDefinition() == nDefinition )
			return (*iter);
	}
	return NULL;
}

void CSteamLocalInventory::RefreshFromServer()
{
	// This will trigger the SteamInventoryResultReady_t callback,
	// and possibly the SteamInventoryFullUpdate_t callback first.
	
	// We could pass a variable to receive the result handle, for
	// comparison to the handle in SteamInventoryResultReady_t,
	// but this simple example does not bother to keep track of
	// multiple in-flight API calls.
	SteamInventory()->GetAllItems( NULL );
}


std::string CSteamItem::GetLocalizedName() const
{
	std::string ret;
	char buf[512];
	uint32 bufSize = sizeof(buf);
	if ( SteamInventory()->GetItemDefinitionProperty( GetDefinition(), "name", buf, &bufSize ) && bufSize <= sizeof(buf) )
	{
		ret = buf;
	}
	else
	{
		ret = "(unknown)";
	}
	return ret;
}

std::string CSteamItem::GetLocalizedDescription() const
{
	std::string ret;
	char buf[2048];
	uint32 bufSize = sizeof(buf);
	if ( SteamInventory()->GetItemDefinitionProperty( GetDefinition(), "description", buf, &bufSize ) && bufSize <= sizeof(buf) )
	{
		ret = buf;
	}
	return ret;
}

std::string CSteamItem::GetIconURL() const
{
	std::string ret;
	char buf[512];
	uint32 bufSize = sizeof(buf);
	if ( SteamInventory()->GetItemDefinitionProperty( GetDefinition(), "icon_url", buf, &bufSize ) && bufSize <= sizeof(buf) )
	{
		ret = buf;
	}
	return ret;
}

#endif
