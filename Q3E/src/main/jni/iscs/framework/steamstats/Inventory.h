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

#ifndef INVENTORY_H
#define INVENTORY_H

//#include "SpaceWar.h"
//#include "GameEngine.h"
#include <list>

class CSteamItem;

// These are hardcoded in the game and match the item definition IDs which were uploaded to Steam.
enum ESteamItemDefIDs
{
	k_SpaceWarItem_TimedDropList = 10,
	k_SpaceWarItem_ShipDecoration1 = 100,
	k_SpaceWarItem_ShipDecoration2 = 101,
	k_SpaceWarItem_ShipDecoration3 = 102,
	k_SpaceWarItem_ShipDecoration4 = 103,
	k_SpaceWarItem_ShipWeapon1 = 110,
	k_SpaceWarItem_ShipWeapon2 = 111,
	k_SpaceWarItem_ShipSpecial1 = 120,
	k_SpaceWarItem_ShipSpecial2 = 121
};


class CSteamLocalInventory
{
public:
	void RefreshFromServer();

	void GrantTestItems();
	void CheckForItemDrops();
	void DoExchange();
	// Get the steam id for the local user at this client
	CSteamID GetLocalSteamID() { return m_SteamIDLocalUser; }
	// Get the local players name
	const char* GetLocalPlayerName() 
	{ 
		return SteamFriends()->GetFriendPersonaName( m_SteamIDLocalUser ); 
	}


	const std::list<CSteamItem *>& GetItemList() const { return m_listPlayerItems; }
	const CSteamItem * GetItem( SteamItemInstanceID_t nItemId ) const;
	const CSteamItem *  GetInstanceOf( SteamItemDef_t nDefinition ) const;
	bool HasInstanceOf( SteamItemDef_t nItemId ) const;

	bool IsWaitingForDropResults() const { return m_hPlaytimeRequestResult != k_SteamInventoryResultInvalid; }
	const CSteamItem * GetLastDroppedItem() const { return GetItem( m_LastDropInstanceID ); }

private:
	friend CSteamLocalInventory *SteamLocalInventory();
	CSteamLocalInventory();
	STEAM_CALLBACK( CSteamLocalInventory, OnSteamInventoryResult, SteamInventoryResultReady_t, m_SteamInventoryResult );
	STEAM_CALLBACK( CSteamLocalInventory, OnSteamInventoryFullUpdate, SteamInventoryFullUpdate_t, m_SteamInventoryFullUpdate );

private:
	SteamInventoryResult_t m_hPlaytimeRequestResult;
	SteamInventoryResult_t m_hPromoRequestResult;
	SteamInventoryResult_t m_hLastFullUpdate;
	SteamInventoryResult_t m_hExchangeRequestResult;

	std::list<CSteamItem *> m_listPlayerItems;
	SteamItemInstanceID_t m_LastDropInstanceID;

	// SteamID for the local user on this client
	CSteamID m_SteamIDLocalUser;
};

//CSteamLocalInventory *SpaceWarLocalInventory();


class CSteamItem
{
public:
	SteamItemInstanceID_t GetItemId() const { return m_Details.m_itemId; }
	SteamItemDef_t GetDefinition() const { return m_Details.m_iDefinition; }
	uint16 GetQuantity() const { return m_Details.m_unQuantity; }
	std::string GetLocalizedName() const;
	std::string GetLocalizedDescription() const;
	std::string GetIconURL() const;
private:
	friend class CSteamLocalInventory;
	SteamItemDetails_t m_Details;
};



#endif // INVENTORY_H