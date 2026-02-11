#include "weaponslots.h"
#include "thingdef/thingdef.h"
#include "a_inventory.h"
#include "wl_agent.h"

#include <climits>

/* Weapon slots ***********************************************************/

//===========================================================================
//
// FWeaponSlot :: AddWeapon
//
// Adds a weapon to the end of the slot if it isn't already in it.
//
//===========================================================================

bool FWeaponSlot::AddWeapon(const char *type)
{
	return AddWeapon (ClassDef::FindClass (type));
}

bool FWeaponSlot::AddWeapon(const ClassDef *type)
{
	unsigned int i;
	
	if (type == NULL)
	{
		return false;
	}
	
	if (!type->IsDescendantOf(NATIVE_CLASS(Weapon)))
	{
		Printf("Can't add non-weapon %s to weapon slots\n", type->GetName().GetChars());
		return false;
	}

	for (i = 0; i < Weapons.Size(); i++)
	{
		if (Weapons[i].Type == type)
			return true;	// Already present
	}
	WeaponInfo info = { type, -1 };
	Weapons.Push(info);
	return true;
}

//===========================================================================
//
// FWeaponSlot :: AddWeaponList
//
// Appends all the weapons from the space-delimited list to this slot.
// Set clear to true to remove any weapons already in this slot first.
//
//===========================================================================

void FWeaponSlot :: AddWeaponList(const char *list, bool clear)
{
	FString copy(list);
	char *buff = copy.LockBuffer();
	char *tok;

	if (clear)
	{
		Clear();
	}
	tok = strtok(buff, " ");
	while (tok != NULL)
	{
		AddWeapon(tok);
		tok = strtok(NULL, " ");
	}
}

//===========================================================================
//
// FWeaponSlot :: LocateWeapon
//
// Returns the index for the specified weapon in this slot, or -1 if it isn't
// in this slot.
//
//===========================================================================

int FWeaponSlot::LocateWeapon(const ClassDef *type)
{
	unsigned int i;

	for (i = 0; i < Weapons.Size(); ++i)
	{
		if (Weapons[i].Type == type)
		{
			return (int)i;
		}
	}
	return -1;
}

//===========================================================================
//
// FWeaponSlot :: PickWeapon
//
// Picks a weapon from this slot. If no weapon is selected in this slot,
// or the first weapon in this slot is selected, returns the last weapon.
// Otherwise, returns the previous weapon in this slot. This means
// precedence is given to the last weapon in the slot, which by convention
// is probably the strongest. Does not return weapons you have no ammo
// for or which you do not possess.
//
//===========================================================================

AWeapon *FWeaponSlot::PickWeapon(player_t *player, bool checkammo)
{
	int i, j;

	if (player->mo == NULL)
	{
		return NULL;
	}
	// Does this slot even have any weapons?
	if (Weapons.Size() == 0)
	{
		return player->ReadyWeapon;
	}
	if (player->ReadyWeapon != NULL)
	{
		for (i = 0; (unsigned)i < Weapons.Size(); i++)
		{
			if (Weapons[i].Type == player->ReadyWeapon->GetClass())// ||
//				(player->ReadyWeapon->WeaponFlags & WIF_POWERED_UP &&
//				 player->ReadyWeapon->SisterWeapon != NULL &&
//				 player->ReadyWeapon->SisterWeapon->GetClass() == Weapons[i].Type))
			{
				for (j = (i == 0 ? Weapons.Size() - 1 : i - 1);
					j != i;
					j = (j == 0 ? Weapons.Size() - 1 : j - 1))
				{
					AWeapon *weap = static_cast<AWeapon *> (player->mo->FindInventory(Weapons[j].Type));

					if (weap != NULL && weap->IsKindOf(NATIVE_CLASS(Weapon)))
					{
						if (!checkammo || weap->CheckAmmo(AWeapon::EitherFire, false))
						{
							return weap;
						}
					}
				}
			}
		}
	}
	for (i = Weapons.Size() - 1; i >= 0; i--)
	{
		AWeapon *weap = static_cast<AWeapon *> (player->mo->FindInventory(Weapons[i].Type));

		if (weap != NULL && weap->IsKindOf(NATIVE_CLASS(Weapon)))
		{
			if (!checkammo || weap->CheckAmmo(AWeapon::EitherFire, false))
			{
				return weap;
			}
		}
	}
	return player->ReadyWeapon;
}

//===========================================================================
//
// FWeaponSlot :: SetInitialPositions
//
// Fills in the position field for every weapon currently in the slot based
// on its position in the slot. These are not scaled to [0,1] so that extra
// weapons can use those values to go to the start or end of the slot.
//
//===========================================================================

void FWeaponSlot::SetInitialPositions()
{
	unsigned int size = Weapons.Size(), i;

	if (size == 1)
	{
		Weapons[0].Position = 0x8000;
	}
	else
	{
		for (i = 0; i < size; ++i)
		{
			Weapons[i].Position = i * 0xFF00 / (size - 1) + 0x80;
		}
	}
}

//===========================================================================
//
// FWeaponSlot :: Sort
//
// Rearranges the weapons by their position field.
//
//===========================================================================

void FWeaponSlot::Sort()
{
	// This does not use qsort(), because the sort should be stable, and
	// there is no guarantee that qsort() is stable. This insertion sort
	// should be fine.
	int i, j;

	for (i = 1; i < (int)Weapons.Size(); ++i)
	{
		fixed_t pos = Weapons[i].Position;
		const ClassDef *type = Weapons[i].Type;
		for (j = i - 1; j >= 0 && Weapons[j].Position > pos; --j)
		{
			Weapons[j + 1] = Weapons[j];
		}
		Weapons[j + 1].Type = type;
		Weapons[j + 1].Position = pos;
	}
}

//===========================================================================
//
// FWeaponSlots - Copy Constructor
//
//===========================================================================

FWeaponSlots::FWeaponSlots(const FWeaponSlots &other)
{
	for (int i = 0; i < NUM_WEAPON_SLOTS; ++i)
	{
		Slots[i] = other.Slots[i];
	}
}

//===========================================================================
//
// FWeaponSlots :: Clear
//
// Removes all weapons from every slot.
//
//===========================================================================

void FWeaponSlots::Clear()
{
	for (int i = 0; i < NUM_WEAPON_SLOTS; ++i)
	{
		Slots[i].Clear();
	}
}

//===========================================================================
//
// FWeaponSlots :: AddDefaultWeapon
//
// If the weapon already exists in a slot, don't add it. If it doesn't,
// then add it to the specified slot.
//
//===========================================================================

ESlotDef FWeaponSlots::AddDefaultWeapon (int slot, const ClassDef *type)
{
	int currSlot, index;

	if (!LocateWeapon (type, &currSlot, &index))
	{
		if (slot >= 0 && slot < NUM_WEAPON_SLOTS)
		{
			bool added = Slots[slot].AddWeapon (type);
			return added ? SLOTDEF_Added : SLOTDEF_Full;
		}
		return SLOTDEF_Full;
	}
	return SLOTDEF_Exists;
}

//===========================================================================
//
// FWeaponSlots :: LocateWeapon
//
// Returns true if the weapon is in a slot, false otherwise. If the weapon
// is found, it can also optionally return the slot and index for it.
//
//===========================================================================

bool FWeaponSlots::LocateWeapon (const ClassDef *type, int *const slot, int *const index)
{
	int i, j;

	for (i = 0; i < NUM_WEAPON_SLOTS; i++)
	{
		j = Slots[i].LocateWeapon(type);
		if (j >= 0)
		{
			if (slot != NULL) *slot = i;
			if (index != NULL) *index = j;
			return true;
		}
	}
	return false;
}

//===========================================================================
//
// FindMostRecentWeapon
//
// Locates the slot and index for the most recently selected weapon. If the
// player is in the process of switching to a new weapon, that is the most
// recently selected weapon. Otherwise, the current weapon is the most recent
// weapon.
//
//===========================================================================

static bool FindMostRecentWeapon(player_t *player, int *slot, int *index)
{
	if (player->PendingWeapon != WP_NOCHANGE)
	{
		return player->weapons.LocateWeapon(player->PendingWeapon->GetClass(), slot, index);
	}
	else if (player->ReadyWeapon != NULL)
	{
		AWeapon *weap = player->ReadyWeapon;
		if (!player->weapons.LocateWeapon(weap->GetClass(), slot, index))
		{
			// If the current weapon wasn't found and is powered up,
			// look for its non-powered up version.
			//if (weap->WeaponFlags & WIF_POWERED_UP && weap->SisterWeaponType != NULL)
			//{
			//	return player->weapons.LocateWeapon(weap->SisterWeaponType, slot, index);
			//}
			return false;
		}
		return true;
	}
	else
	{
		return false;
	}
}

//===========================================================================
//
// FWeaponSlots :: PickNextWeapon
//
// Returns the "next" weapon for this player. If the current weapon is not
// in a slot, then it just returns that weapon, since there's nothing to
// consider it relative to.
//
//===========================================================================

AWeapon *FWeaponSlots::PickNextWeapon(player_t *player)
{
	int startslot, startindex;
	int slotschecked = 0;

	if (player->mo == NULL)
	{
		return NULL;
	}
	if (player->ReadyWeapon == NULL || FindMostRecentWeapon(player, &startslot, &startindex))
	{
		int slot;
		int index;

		if (player->ReadyWeapon == NULL)
		{
			startslot = NUM_WEAPON_SLOTS - 1;
			startindex = Slots[startslot].Size() - 1;
		}

		slot = startslot;
		index = startindex;
		do
		{
			if (++index >= Slots[slot].Size())
			{
				index = 0;
				slotschecked++;
				if (++slot >= NUM_WEAPON_SLOTS)
				{
					slot = 0;
				}
			}
			const ClassDef *type = Slots[slot].GetWeapon(index);
			AWeapon *weap = static_cast<AWeapon *>(player->mo->FindInventory(type));
			if (weap != NULL && weap->CheckAmmo(AWeapon::EitherFire, false))
			{
				return weap;
			}
		}
		while ((slot != startslot || index != startindex) && slotschecked < NUM_WEAPON_SLOTS);
	}
	return player->ReadyWeapon;
}

//===========================================================================
//
// FWeaponSlots :: PickPrevWeapon
//
// Returns the "previous" weapon for this player. If the current weapon is
// not in a slot, then it just returns that weapon, since there's nothing to
// consider it relative to.
//
//===========================================================================

AWeapon *FWeaponSlots::PickPrevWeapon (player_t *player)
{
	int startslot, startindex;
	int slotschecked = 0;

	if (player->mo == NULL)
	{
		return NULL;
	}
	if (player->ReadyWeapon == NULL || FindMostRecentWeapon (player, &startslot, &startindex))
	{
		int slot;
		int index;

		if (player->ReadyWeapon == NULL)
		{
			startslot = 0;
			startindex = 0;
		}

		slot = startslot;
		index = startindex;
		do
		{
			if (--index < 0)
			{
				slotschecked++;
				if (--slot < 0)
				{
					slot = NUM_WEAPON_SLOTS - 1;
				}
				index = Slots[slot].Size() - 1;
			}
			const ClassDef *type = Slots[slot].GetWeapon(index);
			AWeapon *weap = static_cast<AWeapon *>(player->mo->FindInventory(type));
			if (weap != NULL && weap->CheckAmmo(AWeapon::EitherFire, false))
			{
				return weap;
			}
		}
		while ((slot != startslot || index != startindex) && slotschecked < NUM_WEAPON_SLOTS);
	}
	return player->ReadyWeapon;
}

//===========================================================================
//
// FWeaponSlots :: AddExtraWeapons
//
// For every weapon class for the current game, add it to its desired slot
// and position within the slot. Does not first clear the slots.
//
//===========================================================================

void FWeaponSlots::AddExtraWeapons()
{
	unsigned int i;

	// Set fractional positions for current weapons.
	for (i = 0; i < NUM_WEAPON_SLOTS; ++i)
	{
		Slots[i].SetInitialPositions();
	}

	// Append extra weapons to the slots.
	ClassDef::ClassIterator iter = ClassDef::GetClassIterator();
	ClassDef::ClassPair *pair;
	while (iter.NextPair(pair))
	{
		ClassDef *cls = pair->Value;

		if (//cls->ActorInfo != NULL &&
			//(cls->ActorInfo->GameFilter == GAME_Any || (cls->ActorInfo->GameFilter & gameinfo.gametype)) &&
			//cls->ActorInfo->Replacement == NULL &&	// Replaced weapons don't get slotted.
			cls->IsDescendantOf(NATIVE_CLASS(Weapon)) &&
			!LocateWeapon(cls, NULL, NULL)			// Don't duplicate it if it's already present.
			)
		{
			int slot = cls->Meta.GetMetaInt(AWMETA_SlotNumber, -1);
			if ((unsigned)slot < NUM_WEAPON_SLOTS)
			{
				fixed_t position = cls->Meta.GetMetaFixed(AWMETA_SlotPriority, INT_MAX);
				FWeaponSlot::WeaponInfo info = { cls, position };
				Slots[slot].Weapons.Push(info);
			}
		}
	}

	// Now resort every slot to put the new weapons in their proper places.
	for (i = 0; i < NUM_WEAPON_SLOTS; ++i)
	{
		Slots[i].Sort();
	}
}

//===========================================================================
//
// FWeaponSlots :: SetFromGameInfo
//
// If neither the player class nor any defined weapon contain a
// slot assignment, use the game's defaults
//
//===========================================================================

void FWeaponSlots::SetFromGameInfo()
{
	unsigned int i;

	// Only if all slots are empty
	for (i = 0; i < NUM_WEAPON_SLOTS; ++i)
	{
		if (Slots[i].Size() > 0) return;
	}

	// Append extra weapons to the slots.
	/*for (i = 0; i < NUM_WEAPON_SLOTS; ++i)
	{
		for (unsigned j = 0; j < gameinfo.DefaultWeaponSlots[i].Size(); i++)
		{
			const ClassDef *cls = ClassDef::FindClass(gameinfo.DefaultWeaponSlots[i][j]);
			if (cls == NULL)
			{
				Printf("Unknown weapon class '%s' found in default weapon slot assignments\n",
					gameinfo.DefaultWeaponSlots[i][j].GetChars());
			}
			else
			{
				Slots[i].AddWeapon(cls);
			}
		}
	}*/
}

//===========================================================================
//
// FWeaponSlots :: StandardSetup
//
// Setup weapons in this order:
// 1. Use slots from player class.
// 2. Add extra weapons that specify their own slots.
// 3. If all slots are empty, use the settings from the gameinfo (compatibility fallback)
//
//===========================================================================

void FWeaponSlots::StandardSetup(const ClassDef *type)
{
	SetFromPlayer(type);
	AddExtraWeapons();
	SetFromGameInfo();
}

//===========================================================================
//
// FWeaponSlots :: LocalSetup
//
// Setup weapons in this order:
// 1. Run KEYCONF weapon commands, affecting slots accordingly.
// 2. Read config slots, overriding current slots. If WeaponSection is set,
//    then [<WeaponSection>.<PlayerClass>.Weapons] is tried, followed by
//    [<WeaponSection>.Weapons] if that did not exist. If WeaponSection is
//    empty, then the slots are read from [<PlayerClass>.Weapons].
//
//===========================================================================

void FWeaponSlots::LocalSetup(const ClassDef *type)
{
//	P_PlaybackKeyConfWeapons(this);
/*	if (WeaponSection.IsNotEmpty())
	{
		FString sectionclass(WeaponSection);
		sectionclass << '.' << type->GetName().GetChars();
		if (RestoreSlots(GameConfig, sectionclass) == 0)
		{
			RestoreSlots(GameConfig, WeaponSection);
		}
	}
	else
	{
		RestoreSlots(GameConfig, type->GetName().GetChars());
	}*/
}

//===========================================================================
//
// FWeaponSlots :: SendDifferences
//
// Sends the weapon slots from this instance that differ from other's.
//
//===========================================================================

void FWeaponSlots::SendDifferences(const FWeaponSlots &other)
{
	int i, j;

	for (i = 0; i < NUM_WEAPON_SLOTS; ++i)
	{
		if (other.Slots[i].Size() == Slots[i].Size())
		{
			for (j = (int)Slots[i].Size(); j-- > 0; )
			{
				if (other.Slots[i].GetWeapon(j) != Slots[i].GetWeapon(j))
				{
					break;
				}
			}
			if (j < 0)
			{ // The two slots are the same.
				continue;
			}
		}
		// The slots differ. Send mine.
		/*Net_WriteByte(DEM_SETSLOT);
		Net_WriteByte(i);
		Net_WriteByte(Slots[i].Size());
		for (j = 0; j < Slots[i].Size(); ++j)
		{
			Net_WriteWeapon(Slots[i].GetWeapon(j));
		}*/
	}
}

//===========================================================================
//
// FWeaponSlots :: SetFromPlayer
//
// Sets all weapon slots according to the player class.
//
//===========================================================================

void FWeaponSlots::SetFromPlayer(const ClassDef *type)
{
	Clear();
	for (int i = 0; i < NUM_WEAPON_SLOTS; ++i)
	{
		const char *str = type->Meta.GetMetaString(APMETA_Slot0 + i);
		if (str != NULL)
		{
			Slots[i].AddWeaponList(str, false);
		}
	}
}

//===========================================================================
//
// FWeaponSlots :: RestoreSlots
//
// Reads slots from a config section. Any slots in the section override
// existing slot settings, while slots not present in the config are
// unaffected. Returns the number of slots read.
//
//===========================================================================

int FWeaponSlots::RestoreSlots(FConfigFile *config, const char *section)
{
/*	FString section_name(section);
	const char *key, *value;
	int slotsread = 0;

	section_name += ".Weapons";
	if (!config->SetSection(section_name))
	{
		return 0;
	}
	while (config->NextInSection (key, value))
	{
		if (strnicmp (key, "Slot[", 5) != 0 ||
			key[5] < '0' ||
			key[5] > '0'+NUM_WEAPON_SLOTS ||
			key[6] != ']' ||
			key[7] != 0)
		{
			continue;
		}
		Slots[key[5] - '0'].AddWeaponList(value, true);
		slotsread++;
	}
	return slotsread;*/
	return 0;
}

//===========================================================================
//
// CCMD setslot
//
//===========================================================================

void FWeaponSlots::PrintSettings()
{
	for (int i = 1; i <= NUM_WEAPON_SLOTS; ++i)
	{
		int slot = i % NUM_WEAPON_SLOTS;
		if (Slots[slot].Size() > 0)
		{
			Printf("Slot[%d]=", slot);
			for (int j = 0; j < Slots[slot].Size(); ++j)
			{
				Printf("%s ", Slots[slot].GetWeapon(j)->GetName().GetChars());
			}
			Printf("\n");
		}
	}
}

/*CCMD (setslot)
{
	int slot;

	if (argv.argc() < 2 || (slot = atoi (argv[1])) >= NUM_WEAPON_SLOTS)
	{
		Printf("Usage: setslot [slot] [weapons]\nCurrent slot assignments:\n");
		if (players[consoleplayer].mo != NULL)
		{
			FString config(GameConfig->GetConfigPath(false));
			Printf(TEXTCOLOR_BLUE "Add the following to " TEXTCOLOR_ORANGE "%s" TEXTCOLOR_BLUE
				" to retain these bindings:\n" TEXTCOLOR_NORMAL "[", config.GetChars());
			if (WeaponSection.IsNotEmpty())
			{
				Printf("%s.", WeaponSection.GetChars());
			}
			Printf("%s.Weapons]\n", players[consoleplayer].mo->GetClass()->GetName().GetChars());
		}
		players[consoleplayer].weapons.PrintSettings();
		return;
	}

	if (ParsingKeyConf)
	{
		KeyConfWeapons.Push(argv.args());
	}
	else if (PlayingKeyConf != NULL)
	{
		PlayingKeyConf->Slots[slot].Clear();
		for (int i = 2; i < argv.argc(); ++i)
		{
			PlayingKeyConf->Slots[slot].AddWeapon(argv[i]);
		}
	}
	else
	{
		if (argv.argc() == 2)
		{
			Printf ("Slot %d cleared\n", slot);
		}

		Net_WriteByte(DEM_SETSLOT);
		Net_WriteByte(slot);
		Net_WriteByte(argv.argc()-2);
		for (int i = 2; i < argv.argc(); i++)
		{
			Net_WriteWeapon(ClassDef::FindClass(argv[i]));
		}
	}
}*/

//===========================================================================
//
// CCMD addslot
//
//===========================================================================

void FWeaponSlots::AddSlot(int slot, const ClassDef *type, bool feedback)
{
	if (type != NULL && !Slots[slot].AddWeapon(type) && feedback)
	{
		Printf ("Could not add %s to slot %d\n", type->GetName().GetChars(), slot);
	}
}

/*CCMD (addslot)
{
	unsigned int slot;

	if (argv.argc() != 3 || (slot = atoi (argv[1])) >= NUM_WEAPON_SLOTS)
	{
		Printf ("Usage: addslot <slot> <weapon>\n");
		return;
	}

	if (ParsingKeyConf)
	{
		KeyConfWeapons.Push(argv.args());
	}
	else if (PlayingKeyConf != NULL)
	{
		PlayingKeyConf->AddSlot(int(slot), ClassDef::FindClass(argv[2]), false);
	}
	else
	{
		Net_WriteByte(DEM_ADDSLOT);
		Net_WriteByte(slot);
		Net_WriteWeapon(ClassDef::FindClass(argv[2]));
	}
}*/

//===========================================================================
//
// CCMD weaponsection
//
//===========================================================================

/*CCMD (weaponsection)
{
	if (argv.argc() > 1)
	{
		WeaponSection = argv[1];
	}
}*/

//===========================================================================
//
// CCMD addslotdefault
//
//===========================================================================
void FWeaponSlots::AddSlotDefault(int slot, const ClassDef *type, bool feedback)
{
	if (type != NULL && type->IsDescendantOf(NATIVE_CLASS(Weapon)))
	{
		switch (AddDefaultWeapon(slot, type))
		{
		case SLOTDEF_Full:
			if (feedback)
			{
				Printf ("Could not add %s to slot %d\n", type->GetName().GetChars(), slot);
			}
			break;

		default:
		case SLOTDEF_Added:
			break;

		case SLOTDEF_Exists:
			break;
		}
	}
}

/*CCMD (addslotdefault)
{
	const ClassDef *type;
	unsigned int slot;

	if (argv.argc() != 3 || (slot = atoi (argv[1])) >= NUM_WEAPON_SLOTS)
	{
		Printf ("Usage: addslotdefault <slot> <weapon>\n");
		return;
	}

	type = ClassDef::FindClass (argv[2]);
	if (type == NULL || !type->IsDescendantOf (NATIVE_CLASS(Weapon)))
	{
		Printf ("%s is not a weapon\n", argv[2]);
		return;
	}

	if (ParsingKeyConf)
	{
		KeyConfWeapons.Push(argv.args());
	}
	else if (PlayingKeyConf != NULL)
	{
		PlayingKeyConf->AddSlotDefault(int(slot), ClassDef::FindClass(argv[2]), false);
	}
	else
	{
		Net_WriteByte(DEM_ADDSLOTDEFAULT);
		Net_WriteByte(slot);
		Net_WriteWeapon(ClassDef::FindClass(argv[2]));
	}
}*/
