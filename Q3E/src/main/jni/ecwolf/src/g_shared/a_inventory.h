/*
** a_inventory.h
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

#ifndef __A_INVENTORY_H__
#define __A_INVENTORY_H__

#include "actor.h"
#include "textures/textures.h"

typedef TFlags<ItemFlag> ItemFlags;
DEFINE_TFLAGS_OPERATORS (ItemFlags)

class AInventory : public AActor
{
	DECLARE_NATIVE_CLASS(Inventory, Actor)
	HAS_OBJECT_POINTERS

	public:
		virtual void	AttachToOwner(AActor *owner);
		void			BeginPlay();
		bool			CallTryPickup(AActor *toucher);
		virtual void	DetachFromOwner();
		virtual void	Destroy();
		virtual bool	HandlePickup(AInventory *item, bool &good);
		void			ItemFog();
		void			LevelSpawned();
		void			Serialize(FArchive &arc);
		virtual bool	ShouldRespawn();
		void			Tick();
		void			Touch(AActor *toucher);
		virtual bool	Use();

		ItemFlags		itemFlags;
		TObjPtr<AActor>	owner;

		FNameNoInit		pickupsound;
		unsigned int	amount;
		unsigned int	maxamount;
		unsigned int	interhubamount;
		FTextureID		icon;

		unsigned int	respawnTimer;
	protected:
		virtual AInventory	*CreateCopy(AActor *holder);
		void				GoAwayAndDie();
		bool				GoAway();
		virtual bool		ShouldStay();
		virtual bool		TryPickup(AActor *toucher);
};

class AAmmo : public AInventory
{
	DECLARE_NATIVE_CLASS(Ammo, Inventory)

	public:
		const ClassDef	*GetAmmoType() const;
		bool			HandlePickup(AInventory *item, bool &good);

		unsigned int	Backpackamount;
		unsigned int	Backpackmaxamount;
		unsigned int	Backpackboostamount;
	protected:
		AInventory		*CreateCopy(AActor *holder);
};

class ABackpackItem : public AInventory
{
	DECLARE_NATIVE_CLASS(BackpackItem, Inventory)

	public:
		bool			HandlePickup(AInventory *item, bool &good);

	protected:
		void			BoostAmmo(AAmmo *ammo);
		AInventory		*CreateCopy(AActor *holder);
};

class ACustomInventory : public AInventory
{
	DECLARE_NATIVE_CLASS(CustomInventory, Inventory)

	public:
		bool	TryPickup(AActor *toucher);

	protected:
		bool	ExecuteState(AActor *context, const Frame *frame);
};

class AHealth : public AInventory
{
	DECLARE_NATIVE_CLASS(Health, Inventory)

	protected:
		bool	TryPickup(AActor *toucher);
};

enum
{
	AWMETA_Start = 0x01000,

	AWMETA_SelectionOrder,
	AWMETA_SlotNumber,
	AWMETA_SlotPriority
};

typedef TFlags<WeaponFlag> WeaponFlags;
DEFINE_TFLAGS_OPERATORS (WeaponFlags)

class AWeapon : public AInventory
{
	DECLARE_NATIVE_CLASS(Weapon, Inventory)
	HAS_OBJECT_POINTERS

	public:
		enum FireMode
		{
			PrimaryFire,
			AltFire,
			EitherFire
		};

		enum EBobStyle
		{
			BobNormal,
			BobInverse,
			BobAlpha,
			BobInverseAlpha,
			BobSmooth,
			BobInverseSmooth,
			BobThrust
		};

		void		AttachToOwner(AActor *owner);
		bool		CheckAmmo(FireMode fireMode, bool autoSwitch, bool requireAmmo=false);
		bool		DepleteAmmo();

		const Frame	*GetAtkState(FireMode mode, bool hold) const;
		const Frame	*GetDownState() const;
		const Frame	*GetReadyState() const;
		const Frame *GetReloadState() const;
		const Frame	*GetUpState() const;
		const Frame *GetZoomState() const;

		bool		HandlePickup(AInventory *item, bool &good);
		void		Serialize(FArchive &arc);

		WeaponFlags		weaponFlags;
		const ClassDef	*ammotype[2];
		int				ammogive[2];
		unsigned int	ammouse[2];
		fixed			yadjust;
		float			fovscale;

		// Inventory instance variables
		FireMode		mode;
		TObjPtr<AAmmo>	ammo[2];

		// Bob
		EBobStyle	BobStyle;
		fixed		BobRangeX, BobRangeY;
		fixed		BobSpeed;

	protected:
		bool	UseForAmmo(AWeapon *owned);
		bool	ShouldStay();
};

#endif
