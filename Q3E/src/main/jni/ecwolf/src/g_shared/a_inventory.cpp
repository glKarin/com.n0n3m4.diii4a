/*
** a_inventory.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "a_inventory.h"
#include "g_conversation.h"
#include "id_sd.h"
#include "templates.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "wl_def.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_loadsave.h"

IMPLEMENT_POINTY_CLASS(Inventory)
	DECLARE_POINTER(owner)
END_POINTERS

void AInventory::AttachToOwner(AActor *owner)
{
	this->owner = owner;
}

// This is so we can handle certain things (flags) without requiring all of
// TryPickup to be called.
bool AInventory::CallTryPickup(AActor *toucher)
{
	if(itemFlags & IF_INACTIVE)
		return false;

	bool ret = TryPickup(toucher);

	if(!ret && (itemFlags & IF_ALWAYSPICKUP))
	{
		ret = true;
		GoAwayAndDie();
	}

	return ret;
}

// Either creates a copy if the item or returns itself if it is safe to place
// in the actor's inventory.
AInventory *AInventory::CreateCopy(AActor *holder)
{
	if(!GoAway())
		return this;

	AInventory *copy = reinterpret_cast<AInventory *>(GetClass()->CreateInstance());
	copy->RemoveFromWorld();
	copy->amount = amount;
	copy->maxamount = maxamount;
	return copy;
}

void AInventory::DetachFromOwner()
{
	owner = NULL;
	inventory = NULL;
}

void AInventory::Destroy()
{
	if(owner)
		owner->RemoveInventory(this);

	Super::Destroy();
}

// Used to destroy items which aren't placed into an inventory and don't respawn.
void AInventory::GoAwayAndDie()
{
	if(!GoAway())
	{
		Destroy();
	}
}

// Attempts to hide the actor for respawning. Returns true if hidden, false if
// this actor is safe to be placed in an inventory.
bool AInventory::GoAway()
{
	const Frame *hide = FindState(NAME_Hide);
	if(hide && IsThinking()) // Only hide actors that are thinking
	{
		itemFlags |= IF_INACTIVE;
		SetState(hide);
		return true;
	}
	return false;
}

// Returns true if the pickup was handled by an already existing inventory item.
bool AInventory::HandlePickup(AInventory *item, bool &good)
{
	if(item->IsA(GetClass()))
	{
		if(amount < maxamount)
		{
			amount += item->amount;
			if(amount > maxamount)
				amount = maxamount;
			good = true;
		}
		else
			good = false;
		return true;
	}
	else if(inventory)
		return inventory->HandlePickup(item, good);
	return false;
}

void AInventory::Serialize(FArchive &arc)
{
	arc << itemFlags
		<< owner
		<< pickupsound
		<< amount
		<< maxamount
		<< interhubamount
		<< icon;

	Super::Serialize(arc);
}

void AInventory::Touch(AActor *toucher)
{
	if(!(toucher->flags & FL_PICKUP))
		return;

	if(!CallTryPickup(toucher))
		return;

	if(flags & FL_COUNTITEM)
		++gamestate.treasurecount;
	if(flags & FL_COUNTSECRET)
		++gamestate.secretcount;

	PlaySoundLocActor(pickupsound, toucher);
	if(toucher->player == &players[ConsolePlayer])
		StartBonusFlash();
}

bool AInventory::TryPickup(AActor *toucher)
{
	bool pickupGood = false;
	if(toucher->inventory && toucher->inventory->HandlePickup(this, pickupGood))
	{
		// The actor has this item in their inventory and it has been handled.
		if(!pickupGood)
			return false;
		GoAwayAndDie();
	}
	else if(maxamount == 0)
	{
		// We can add maxamount = 0 items if we can use them right away.
		if(!(itemFlags & IF_AUTOACTIVATE))
			return false;

		toucher->AddInventory(this);
		bool good = Use();
		toucher->RemoveInventory(this);

		if(good)
			GoAwayAndDie();
		else
			return false;
	}
	else
	{
		AInventory *invItem = CreateCopy(toucher);
		if(invItem != this)
			GoAwayAndDie();

		toucher->AddInventory(invItem);
		invItem->RemoveFromWorld();

		if((itemFlags & IF_AUTOACTIVATE) && invItem->Use())
			--invItem->amount;
	}
	return true;
}

bool AInventory::Use()
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(Health)

bool AHealth::TryPickup(AActor *toucher)
{
	if(toucher->health <= 0)
		return false;

	int max = maxamount;

	if(toucher->player)
	{
		if(max == 0)
			max = toucher->player->mo->maxhealth;

		if(toucher->player->health >= max)
			return false;

		toucher->player->health += amount;
		if(toucher->player->health > max)
			toucher->player->health = max;

		const int oldhealth = toucher->health;
		toucher->health = toucher->player->health;
		StatusBar->UpdateFace(oldhealth - toucher->health);
	}
	else
	{
		if(max == 0)
			max = toucher->SpawnHealth();

		if(toucher->health >= max)
			return false;

		toucher->health += amount;
		if(toucher->health > max)
			toucher->health = max;
	}

	Destroy();
	return true;
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(Ammo)

AInventory *AAmmo::CreateCopy(AActor *holder)
{
	const ClassDef *ammoClass = GetAmmoType();

	if(ammoClass == GetClass())
		return Super::CreateCopy(holder);

	GoAwayAndDie();

	AInventory *copy = reinterpret_cast<AInventory *>(ammoClass->CreateInstance());
	copy->RemoveFromWorld();
	copy->amount = amount;
	copy->maxamount = maxamount;
	return copy;
}

const ClassDef *AAmmo::GetAmmoType() const
{
	const ClassDef *cls = GetClass();
	while(cls->GetParent() != NATIVE_CLASS(Ammo))
		cls = cls->GetParent();
	return cls;
}

bool AAmmo::HandlePickup(AInventory *item, bool &good)
{
	if(IsSameKindOf(NATIVE_CLASS(Ammo), item->GetClass()))
	{
		if(amount < maxamount)
		{
			bool regainedAmmo = amount == 0;

			amount += item->amount;
			if(amount > maxamount)
				amount = maxamount;
			good = true;

			if(regainedAmmo && owner && owner->player)
			{
				barrier_cast<APlayerPawn*>(owner)->CheckWeaponSwitch(GetClass());
			}
		}
		else
			good = false;
		return true;
	}
	return Super::HandlePickup(item, good);
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(BackpackItem)

void ABackpackItem::BoostAmmo(AAmmo *ammo)
{
	if(ammo->Backpackboostamount)
	{
		ammo->maxamount += ammo->Backpackboostamount;
		if(ammo->maxamount > ammo->Backpackmaxamount)
			ammo->maxamount = ammo->Backpackmaxamount;
	}
	else
		ammo->maxamount = ammo->Backpackmaxamount;

	ammo->amount += ammo->Backpackamount;
	if(ammo->amount > ammo->maxamount)
		ammo->amount = ammo->maxamount;
}

bool ABackpackItem::HandlePickup(AInventory *item, bool &good)
{
	if(item->IsA(NATIVE_CLASS(BackpackItem)))
	{
		// We seem to have a backpack so just give the ammo
		for(AInventory *item = owner->inventory;item;item = item->inventory)
		{
			if(item->GetClass()->GetParent() == NATIVE_CLASS(Ammo))
			{
				AAmmo *ammo = static_cast<AAmmo*>(item);
				BoostAmmo(ammo);
			}
		}
		good = true;
		return true;
	}
	else if(inventory)
		return inventory->HandlePickup(item, good);
	return false;
}

AInventory *ABackpackItem::CreateCopy(AActor *holder)
{
	// Bump carrying capacity and give ammo
	ClassDef::ClassIterator iter = ClassDef::GetClassIterator();
	ClassDef::ClassPair *pair;
	while(iter.NextPair(pair))
	{
		const ClassDef *cls = pair->Value;
		if(cls->GetParent() == NATIVE_CLASS(Ammo))
		{
			// See if we have this time of ammo
			AAmmo *ammo = static_cast<AAmmo *>(holder->FindInventory(cls));
			if(ammo)
			{
				// Increase amount and give ammo
				BoostAmmo(ammo);
			}
			else
			{
				// Give the ammo type with the proper amounts
				ammo = static_cast<AAmmo *>(AActor::Spawn(cls, 0, 0, 0, 0));
				ammo->amount = 0;
				BoostAmmo(ammo);

				ammo->RemoveFromWorld();
				if(!ammo->CallTryPickup(holder))
					ammo->Destroy();
			}
		}
	}
	return Super::CreateCopy(holder);
}

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_CLASS(CustomInventory)

ACTION_FUNCTION(A_Succeed)
{
	// At the time of writing we don't really have a good action function to
	// call to ensure that a Pickup state succeeds. So we'll make a no op.
	return true;
}

ACTION_FUNCTION(A_WeaponGrin)
{
	StatusBar->WeaponGrin();
	return true;
}

bool ACustomInventory::TryPickup(AActor *toucher)
{
	const Frame *pickup = FindState(NAME_Pickup);
	if(!ExecuteState(toucher, pickup))
		return false;
	return Super::TryPickup(toucher);
}

bool ACustomInventory::ExecuteState(AActor *context, const Frame *frame)
{
	bool success = false;
	ActionResult result;
	memset(&result, 0, sizeof(ActionResult));

	while(frame)
	{
		// Execute both functions since why not.
		success |= frame->action(context, this, frame, &result);
		if(result.JumpFrame)
		{
			frame = result.JumpFrame;
			result.JumpFrame = NULL;
			continue;
		}
		success |= frame->thinker(context, this, frame, &result);
		if(result.JumpFrame)
		{
			frame = result.JumpFrame;
			result.JumpFrame = NULL;
			continue;
		}

		if(frame == frame->next)
		{
			// Fail if we stay on the same state
			success = false;
			break;
		}
		frame = frame->next;
	}
	return success;
}

////////////////////////////////////////////////////////////////////////////////

// Opens a dialog when picked up, but otherwise behaves like a CustomInventory.
class AQuizItem : public ACustomInventory
{
	DECLARE_NATIVE_CLASS(QuizItem, CustomInventory)

public:
	bool TryPickup(AActor *toucher)
	{
		if(Super::TryPickup(toucher))
		{
			Dialog::StartConversation(this);
			return true;
		}
		return false;
	}
};

IMPLEMENT_CLASS(QuizItem)

////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_POINTY_CLASS(Weapon)
	DECLARE_POINTER(ammo[0])
	DECLARE_POINTER(ammo[1])
END_POINTERS

void AWeapon::AttachToOwner(AActor *owner)
{
	Super::AttachToOwner(owner);

	for(unsigned int i = 0;i < 2;++i)
	{
		ammo[i] = static_cast<AAmmo *>(owner->FindInventory(ammotype[i]));
		if(!ammo[i])
		{
			if(ammotype[i])
			{
				ammo[i] = static_cast<AAmmo *>(Spawn(ammotype[i], 0, 0, 0, false));
				ammo[i]->amount = MIN<unsigned int>(ammogive[i], ammo[i]->maxamount);
				owner->AddInventory(ammo[i]);
				ammo[i]->RemoveFromWorld();
			}
		}
		else if(ammo[i]->amount < ammo[i]->maxamount)
		{
			ammo[i]->amount += ammogive[i];
			if(ammo[i]->amount > ammo[i]->maxamount)
				ammo[i]->amount = ammo[i]->maxamount;
		}
	}

	// Autoswitch
	owner->player->PendingWeapon = this;

	// Grin
	if(!(weaponFlags & WF_NOGRIN) && owner->player->mo == players[0].camera)
		StatusBar->WeaponGrin();
}

bool AWeapon::CheckAmmo(AWeapon::FireMode fireMode, bool autoSwitch, bool requireAmmo)
{
	const unsigned int amount1 = ammo[PrimaryFire] != NULL ? ammo[PrimaryFire]->amount : 0;
	const unsigned int amount2 = ammo[AltFire] != NULL ? ammo[AltFire]->amount : 0;

	switch(fireMode)
	{
		case PrimaryFire:
			if(amount1 >= ammouse[PrimaryFire])
				return true;
			break;
		case AltFire:
			if(!FindState(NAME_AltFire))
				return false;
			if(amount2 >= ammouse[AltFire])
				return true;
			break;
		default:
		case EitherFire:
			if(CheckAmmo(PrimaryFire, false) || CheckAmmo(AltFire, false))
				return true;
			break;
	}

	if(autoSwitch)
	{
		static_cast<APlayerPawn *>((AActor*)owner)->PickNewWeapon();
	}

	return false;
}

bool AWeapon::DepleteAmmo()
{
	if(!CheckAmmo(mode, false))
		return false;

	AAmmo * const ammo = this->ammo[mode];
	const unsigned int ammouse = this->ammouse[mode];

	if(ammo == NULL)
		return true;

	if(ammo->amount < ammouse)
		ammo->amount = 0;
	else
		ammo->amount -= ammouse;

	return true;
}

const Frame *AWeapon::GetAtkState(FireMode mode, bool hold) const
{
	const Frame *ret = NULL;
	if(mode == PrimaryFire)
	{
		if(hold)
			ret = FindState(NAME_Hold);
		if(ret == NULL)
			ret = FindState(NAME_Fire);
	}
	else
	{
		if(hold)
			ret = FindState(NAME_AltHold);
		if(ret == NULL)
			ret = FindState(NAME_AltFire);
	}
	return ret;
}

const Frame *AWeapon::GetUpState() const
{
	return FindState(NAME_Select);
}

const Frame *AWeapon::GetDownState() const
{
	return FindState(NAME_Deselect);
}

const Frame *AWeapon::GetReadyState() const
{
	return FindState(NAME_Ready);
}

const Frame *AWeapon::GetReloadState() const
{
	return FindState(NAME_Reload);
}

const Frame *AWeapon::GetZoomState() const
{
	return FindState(NAME_Zoom);
}

bool AWeapon::HandlePickup(AInventory *item, bool &good)
{
	if(item->GetClass() == GetClass())
	{
		good = static_cast<AWeapon *>(item)->UseForAmmo(this);

		// Grin any way
		if(weaponFlags & WF_ALWAYSGRIN)
			StatusBar->WeaponGrin();
		return true;
	}
	else if(inventory)
		return inventory->HandlePickup(item, good);
	return false;
}

void AWeapon::Serialize(FArchive &arc)
{
	BYTE mode = this->mode;
	arc << mode;
	this->mode = static_cast<FireMode>(mode);

	arc << ammotype[0]
		<< ammogive[0]
		<< ammouse[0]
		<< yadjust
		<< ammo[0];

	if(GameSave::SaveProdVersion >= 0x001002FF && GameSave::SaveVersion > 1374729160)
		arc << ammotype[1] << ammogive[1] << ammouse[1] << ammo[1]
			<< fovscale;

	Super::Serialize(arc);
}

bool AWeapon::UseForAmmo(AWeapon *owned)
{
	bool used = false;
	for(unsigned int i = 0;i < 2;++i)
	{
		AAmmo *ammo = owned->ammo[i];
		if(!ammo || ammogive[i] <= 0)
			break;

		if(ammo->amount < ammo->maxamount)
		{
			ammo->amount += ammogive[i];
			if(ammo->amount > ammo->maxamount)
				ammo->amount = ammo->maxamount;
			used = true;
			break;
		}
	}
	return used;
}

ACTION_FUNCTION(A_ReFire)
{
	player_t *player = self->player;
	if(!player)
		return false;

	if(!player->ReadyWeapon->CheckAmmo(player->ReadyWeapon->mode, true))
		return false;

	if(player->PendingWeapon == WP_NOCHANGE || !(player->flags & player_t::PF_REFIRESWITCHOK))
	{
		ACTION_PARAM_STATE(hold, 0, player->ReadyWeapon->GetAtkState(player->ReadyWeapon->mode, true));

		if((player->ReadyWeapon->mode == AWeapon::PrimaryFire && control[player->GetPlayerNum()].buttonstate[bt_attack]) ||
		   (player->ReadyWeapon->mode == AWeapon::AltFire && control[player->GetPlayerNum()].buttonstate[bt_altattack]))
		{
			if(self->MissileState)
				self->SetState(player->mo->MissileState);
			player->SetPSprite(hold, player_t::ps_weapon);
		}
	}
	return true;
}

ACTION_FUNCTION(A_WeaponReady)
{
	enum
	{
		WRF_NOBOB = 1,
		WRF_NOPRIMARY = 2,
		WRF_NOSECONDARY = 4,
		WRF_NOFIRE = WRF_NOPRIMARY|WRF_NOSECONDARY,
		WRF_NOSWITCH = 8,
		WRF_DISABLESWITCH = 0x10,
		WRF_ALLOWRELOAD = 0x20,
		WRF_ALLOWZOOM = 0x40
	};

	ACTION_PARAM_INT(flags, 0);

	if(!(flags & WRF_NOBOB)) self->player->flags |= player_t::PF_WEAPONBOBBING;
	if(!(flags & WRF_NOPRIMARY)) self->player->flags |= player_t::PF_WEAPONREADY;
	if(!(flags & WRF_NOSECONDARY)) self->player->flags |= player_t::PF_WEAPONREADYALT;
	if(!(flags & WRF_NOSWITCH)) self->player->flags |= player_t::PF_WEAPONSWITCHOK|player_t::PF_REFIRESWITCHOK;

	if((flags & WRF_DISABLESWITCH)) { self->player->flags |= player_t::PF_DISABLESWITCH; self->player->flags &= ~player_t::PF_REFIRESWITCHOK; }
	else self->player->flags &= ~player_t::PF_DISABLESWITCH;

	if((flags & WRF_ALLOWRELOAD)) self->player->flags |= player_t::PF_WEAPONRELOADOK;
	if((flags & WRF_ALLOWZOOM)) self->player->flags |= player_t::PF_WEAPONZOOMOK;
	return true;
}


class AWeaponGiver : public AWeapon
{
	DECLARE_NATIVE_CLASS(WeaponGiver, Weapon)

	protected:
		bool TryPickup(AActor *toucher)
		{
			bool pickedup = true;
			AWeapon *switchTo = toucher->player->PendingWeapon;
			DropList *drops = GetDropList();

			// Get the tail since that will be the primary weapon.
			DropList::Iterator item = drops->Tail();

			for(;item;--item)
			{
				// Only the first item in the list should give ammo
				bool noammo = item.HasNext();
				const ClassDef *cls = ClassDef::FindClass(item->className);
				if(!cls || !cls->IsDescendantOf(NATIVE_CLASS(Weapon)))
					continue;

				AWeapon *weap = static_cast<AWeapon *>(AActor::Spawn(cls, 0, 0, 0, 0));
				weap->itemFlags &= ~IF_ALWAYSPICKUP;
				weap->RemoveFromWorld();

				if(noammo)
				{
					weap->ammogive[0] = weap->ammogive[1] = 0;
				}
				else
				{
					if(ammogive[PrimaryFire] >= 0)
						weap->ammogive[PrimaryFire] = ammogive[PrimaryFire];
					if(ammogive[AltFire] >= 0)
						weap->ammogive[AltFire] = ammogive[AltFire];
				}

				if(!weap->CallTryPickup(toucher))
				{
					weap->Destroy();

					// If main weapon isn't picked up, don't continue.
					if(!noammo)
					{
						pickedup = false;
						break;
					}
				}
				else if(!noammo)
				{
					// Main weapon picked up!
					GoAwayAndDie();
					if(toucher->player->PendingWeapon == weap)
						switchTo = weap;
				}
			}

			// Ensure the right weapon is autoswitched.
			toucher->player->PendingWeapon = switchTo;

			return pickedup;
		}
};
IMPLEMENT_CLASS(WeaponGiver)

////////////////////////////////////////////////////////////////////////////////

class AScoreItem : public AInventory
{
	DECLARE_NATIVE_CLASS(ScoreItem, Inventory)

	protected:
		bool TryPickup(AActor *toucher)
		{
			if(toucher->player)
				toucher->player->GivePoints(amount);
			GoAwayAndDie();
			return true;
		}
};
IMPLEMENT_CLASS(ScoreItem)

////////////////////////////////////////////////////////////////////////////////

class AExtraLifeItem : public AInventory
{
	DECLARE_NATIVE_CLASS(ExtraLifeItem, Inventory)

	protected:
		AInventory *CreateCopy(AActor *holder)
		{
			const ClassDef *cls = GetClass();
			while(cls->GetParent() != NATIVE_CLASS(ExtraLifeItem))
				cls = cls->GetParent();

			if(cls == GetClass())
				return Super::CreateCopy(holder);

			GoAwayAndDie();

			AInventory *copy = reinterpret_cast<AInventory *>(cls->CreateInstance());
			copy->RemoveFromWorld();
			copy->amount = amount;
			copy->maxamount = maxamount;
			return copy;
		}

		bool HandlePickup(AInventory *item, bool &good)
		{
			if(IsSameKindOf(NATIVE_CLASS(ExtraLifeItem), item->GetClass()))
			{
				amount += item->amount;
				if(amount >= maxamount)
				{
					if(owner->player)
						owner->player->GiveExtraMan(amount/maxamount);
					amount %= maxamount;
				}
				good = true;
				return true;
			}
			return Super::HandlePickup(item, good);
		}
};
IMPLEMENT_CLASS(ExtraLifeItem)

////////////////////////////////////////////////////////////////////////////////

class AMapRevealer : public AInventory
{
	DECLARE_NATIVE_CLASS(MapRevealer, Inventory)

	protected:
		bool TryPickup(AActor *toucher)
		{
			gamestate.fullmap = true;
			GoAwayAndDie();
			return true;
		}
};
IMPLEMENT_CLASS(MapRevealer)
