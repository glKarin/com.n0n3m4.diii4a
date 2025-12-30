// From ZDoom

#ifndef __WEAPONSLOTS_H__
#define __WEAPONSLOTS_H__

#include "wl_def.h"
#include "tarray.h"

#define NUM_WEAPON_SLOTS		10

class player_t;
class FConfigFile;
class AWeapon;
class ClassDef;

class FWeaponSlot
{
public:
	FWeaponSlot() { Clear(); }
	FWeaponSlot(const FWeaponSlot &other) { Weapons = other.Weapons; }
	FWeaponSlot &operator= (const FWeaponSlot &other) { Weapons = other.Weapons; return *this; }
	void Clear() { Weapons.Clear(); }
	bool AddWeapon (const char *type);
	bool AddWeapon (const ClassDef *type);
	void AddWeaponList (const char *list, bool clear);
	AWeapon *PickWeapon (player_t *player, bool checkammo = false);
	int Size () const { return (int)Weapons.Size(); }
	int LocateWeapon (const ClassDef *type);

	inline const ClassDef *GetWeapon (int index) const
	{
		if ((unsigned)index < Weapons.Size())
		{
			return Weapons[index].Type;
		}
		else
		{
			return NULL;
		}
	}

	friend struct FWeaponSlots;

private:
	struct WeaponInfo
	{
		const ClassDef *Type;
		fixed_t Position;
	};
	void SetInitialPositions();
	void Sort();
	TArray<WeaponInfo> Weapons;
};
// FWeaponSlots::AddDefaultWeapon return codes
enum ESlotDef
{
	SLOTDEF_Exists,		// Weapon was already assigned a slot
	SLOTDEF_Added,		// Weapon was successfully added
	SLOTDEF_Full		// The specifed slot was full
};

struct FWeaponSlots
{
	FWeaponSlots() { Clear(); }
	FWeaponSlots(const FWeaponSlots &other);

	FWeaponSlot Slots[NUM_WEAPON_SLOTS];

	AWeapon *PickNextWeapon (player_t *player);
	AWeapon *PickPrevWeapon (player_t *player);

	void Clear ();
	bool LocateWeapon (const ClassDef *type, int *const slot, int *const index);
	ESlotDef AddDefaultWeapon (int slot, const ClassDef *type);
	void AddExtraWeapons();
	void SetFromGameInfo();
	void SetFromPlayer(const ClassDef *type);
	void StandardSetup(const ClassDef *type);
	void LocalSetup(const ClassDef *type);
	void SendDifferences(const FWeaponSlots &other);
	int RestoreSlots (FConfigFile *config, const char *section);
	void PrintSettings();

	void AddSlot(int slot, const ClassDef *type, bool feedback);
	void AddSlotDefault(int slot, const ClassDef *type, bool feedback);

};

#endif
