/*
** a_playerpawn.h
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

#ifndef __A_PLAYERPAWN_H__
#define __A_PLAYERPAWN_H__

#include "actor.h"

class player_t;
class AWeapon;

enum
{
	APMETA_Start = 0x02000,

	APMETA_Slot0,
	APMETA_Slot1,
	APMETA_Slot2,
	APMETA_Slot3,
	APMETA_Slot4,
	APMETA_Slot5,
	APMETA_Slot6,
	APMETA_Slot7,
	APMETA_Slot8,
	APMETA_Slot9,
	APMETA_StartInventory,
	APMETA_DisplayName,
	APMETA_MoveBob
};

class APlayerPawn : public AActor
{
	DECLARE_NATIVE_CLASS(PlayerPawn, Actor)

	public:
		void		CheckWeaponSwitch(const ClassDef *ammo);
		void		Die();
		DropList	*GetStartInventory();
		void		GiveDeathmatchInventory();
		void		GiveStartingInventory();
		AWeapon		*PickNewWeapon();
		void		RemoveInventory(AInventory *item);
		void		Serialize(FArchive &arc);
		void		SetupWeaponSlots();
		void		Tick();
		void		TickPSprites();

		static PointerIndexTable<DropList> startInventory;

		int32_t		maxhealth;
		int			damagecolor;
		fixed		forwardmove[2];
		fixed		sidemove[2];
		fixed		viewheight;

	protected:
		AWeapon	*BestWeapon(const ClassDef *ammo=NULL);

		void Cmd_Use();
		void DeathTick();
};

#endif
