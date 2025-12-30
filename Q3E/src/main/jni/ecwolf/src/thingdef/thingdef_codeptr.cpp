/*
** thingdef_codeptr.cpp
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
** Some action pointer code may closely resemble ZDoom code since they
** should behave the same.
**
*/

#include "actor.h"
#include "id_ca.h"
#include "id_sd.h"
#include "g_mapinfo.h"
#include "g_shared/a_deathcam.h"
#include "g_shared/a_inventory.h"
#include "lnspec.h"
#include "m_random.h"
#include "thingdef/thingdef.h"
#include "wl_act.h"
#include "wl_def.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_play.h"
#include "wl_state.h"

static ActionTable *actionFunctions = NULL;
ActionInfo::ActionInfo(ActionPtr func, const FName &name) : func(func), name(name),
	minArgs(0), maxArgs(0), varArgs(false)
{
	if(actionFunctions == NULL)
		actionFunctions = new ActionTable;
#if 0
	// Debug code - Show registered action functions
	printf("Adding %s @ %d\n", name.GetChars(), actionFunctions->Size());
#endif
	actionFunctions->Push(this);
}

static int FunctionTableComp(const void *f1, const void *f2)
{
	const ActionInfo * const func1 = *((const ActionInfo **)f1);
	const ActionInfo * const func2 = *((const ActionInfo **)f2);
	if(func1->name < func2->name)
		return -1;
	else if(func1->name > func2->name)
		return 1;
	return 0;
}

void InitFunctionTable(ActionTable *table)
{
	if(table == NULL)
		table = actionFunctions;

	qsort(&(*table)[0], table->Size(), sizeof((*table)[0]), FunctionTableComp);
	for(unsigned int i = 1;i < table->Size();++i)
		assert((*table)[i]->name > (*table)[i-1]->name);
}

void ReleaseFunctionTable()
{
	delete actionFunctions;
}

ActionInfo *LookupFunction(const FName &func, const ActionTable *table)
{
	if(table == NULL)
		table = actionFunctions;

	ActionInfo *inf;
	unsigned int max = table->Size()-1;
	unsigned int min = 0;
	unsigned int mid = max/2;
	do
	{
		inf = (*table)[mid];
		if(inf->name == func)
			return inf;

		if(inf->name > func)
			max = mid-1;
		else if(inf->name < func)
			min = mid+1;
		mid = (max+min)/2;
	}
	while(max >= min && max < table->Size());
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

ACTION_FUNCTION(A_CallSpecial)
{
	ACTION_PARAM_INT(special, 0);
	ACTION_PARAM_INT(arg1, 1);
	ACTION_PARAM_INT(arg2, 2);
	ACTION_PARAM_INT(arg3, 3);
	ACTION_PARAM_INT(arg4, 4);
	ACTION_PARAM_INT(arg5, 5);

	int specialArgs[5] = {arg1, arg2, arg3, arg4, arg5};

	Specials::LineSpecialFunction function = Specials::LookupFunction(static_cast<Specials::LineSpecials>(special));
	return function(map->GetSpot(self->tilex, self->tiley, 0), specialArgs, MapTrigger::East, self) != 0;
}

////////////////////////////////////////////////////////////////////////////////

extern FRandom pr_chase;
ACTION_FUNCTION(A_ActiveSound)
{
	ACTION_PARAM_INT(chance, 0);

	// If chance == 3 this has the same chance as A_Chase. Useful for giving
	// wolfenstein style monsters activesounds without making it 8x as likely
	if(chance >= 256 || pr_chase() < chance)
		PlaySoundLocActor(self->activesound, self);
	return true;
}

ACTION_FUNCTION(A_AlertMonsters)
{
	madenoise = true;
	return true;
}

ACTION_FUNCTION(A_BossDeath)
{
	// Deathcam involves a little bit of spaghetti code. To sum it up:
	// 1.) This function creates the death cam and lets it's spawn state run.
	//     Victory flag is set by the death cam.
	// 2.) After the actor's death state runs again this function should be
	//     called. It will restart the spawn state for timing.
	// 3.) The deathcam signals this function for the third time with the
	//     victory flag removed. We'll call the action specials and then if
	//     we're still in the game, return control to the player.
	ADeathCam *deathcam = NULL;

	bool alldead = true;
	AActor::Iterator iter = AActor::GetIterator();
	while(iter.Next())
	{
		AActor * const other = iter;

		if(other != self)
		{
			if(other->GetClass() == NATIVE_CLASS(DeathCam))
				deathcam = static_cast<ADeathCam *> (other);
			else if(other->GetClass() == self->GetClass())
			{
				if(other->health > 0)
				{
					alldead = false;
					break;
				}
			}
		}
	}

	if(!alldead)
		return false;

	if(levelInfo->DeathCam && (!deathcam || deathcam->camState != ADeathCam::CAM_FINISHED))
	{
		if(!deathcam)
		{
			ADeathCam *dc = (ADeathCam*)AActor::Spawn(NATIVE_CLASS(DeathCam), 0, 0, 0, SPAWN_AllowReplacement);
			dc->SetupDeathCam(self, players[0].mo);
		}
		else
		{
			// Let the deathcam reanimate
			deathcam->SetState(deathcam->SpawnState);
		}
	}
	else
	{
		for(unsigned int i = 0;i < levelInfo->SpecialActions.Size();++i)
		{
			const LevelInfo::SpecialAction &action = levelInfo->SpecialActions[i];
			if(action.Class == self->GetClass())
			{
				const Specials::LineSpecials special = static_cast<Specials::LineSpecials>(action.Special);
				Specials::LineSpecialFunction function = Specials::LookupFunction(special);
				function(map->GetSpot(self->tilex, self->tiley, 0), action.Args, MapTrigger::East, self);
			}
		}

		if(deathcam && playstate == ex_stillplaying)
		{
			// Return the camera to the player if we're still going
			players[0].camera = players[0].mo;
			players[0].BringUpWeapon();
			gamestate.victoryflag = false;
		}
	}
	return true;
}

// Sets or unsets a flag on an actor.
ACTION_FUNCTION(A_ChangeFlag)
{
	ACTION_PARAM_STRING(flag, 0);
	ACTION_PARAM_BOOL(value, 1);

	// We'll also want to keep the counts consistant
	const bool countedKill = !!(self->flags & FL_COUNTKILL);
	const bool countedSecret = !!(self->flags & FL_COUNTSECRET);
	const bool countedItem = !!(self->flags & FL_COUNTITEM);

	FString prefix;
	if(flag.IndexOf(".") != -1)
	{
		prefix = flag.Left(flag.IndexOf("."));
		flag = flag.Mid(flag.IndexOf(".")+1);
	}
	if(!ClassDef::SetFlag(self->GetClass(), self, prefix, flag, value))
	{
		Printf("A_ChangeFlag: Attempt to change unknown flag '%s'.\n", (prefix.IsEmpty() ? flag.GetChars() : (prefix + "." + flag).GetChars()));
		return false;
	}

	const bool countsKill = !!(self->flags & FL_COUNTKILL);
	const bool countsSecret = !!(self->flags & FL_COUNTSECRET);
	const bool countsItem = !!(self->flags & FL_COUNTITEM);
	if(countedKill != countsKill)
	{
		if(countsKill) ++gamestate.killtotal;
		else --gamestate.killtotal;
	}
	if(countedItem != countsItem)
	{
		if(countsItem) ++gamestate.treasuretotal;
		else --gamestate.treasuretotal;
	}
	if(countedSecret != countsSecret)
	{
		if(countsSecret) ++gamestate.secrettotal;
		else --gamestate.secrettotal;
	}
	return true;
}

ACTION_FUNCTION(A_ChangeVelocity)
{
	enum
	{
		CVF_RELATIVE = 1,
		CVF_REPLACE = 2
	};

	ACTION_PARAM_DOUBLE(x, 0);
	ACTION_PARAM_DOUBLE(y, 1);
	ACTION_PARAM_DOUBLE(z, 2);
	ACTION_PARAM_INT(flags, 3);

	fixed fx, fy;
	if(flags & CVF_RELATIVE)
	{
		fx = static_cast<fixed>(((x * finecosine[self->angle>>ANGLETOFINESHIFT]) + (y * finesine[self->angle>>ANGLETOFINESHIFT]))/64);
		fy = static_cast<fixed>(((y * finecosine[self->angle>>ANGLETOFINESHIFT]) - (x * finesine[self->angle>>ANGLETOFINESHIFT]))/64);
	}
	else
	{
		fx = static_cast<fixed>(x*(FRACUNIT/64));
		fy = static_cast<fixed>(y*(FRACUNIT/64));
	}

	if(flags & CVF_REPLACE)
	{
		self->velx = fx;
		self->vely = fy;
	}
	else
	{
		self->velx += fx;
		self->vely += fy;
	}
	return true;
}

ACTION_FUNCTION(A_Explode)
{
	enum
	{
		XF_HURTSOURCE = 1
	};

	ACTION_PARAM_INT(damage, 0);
	ACTION_PARAM_INT(radius, 1);
	ACTION_PARAM_INT(flags, 2);
	ACTION_PARAM_BOOL(alert, 3);
	ACTION_PARAM_INT(fulldamageradius, 4);

	if(alert)
		madenoise = true;

	const double rolloff = 1.0/static_cast<double>(radius - fulldamageradius);
	for(AActor::Iterator iter = AActor::GetIterator();iter.Next();)
	{
		AActor * const target = iter;

		// Calculate distance from origin to outer bound of target actor
		const fixed dist = MAX<fixed>(0, MAX(abs(target->x - self->x), abs(target->y - self->y)) - target->radius) >> (FRACBITS - 6);

		// First check if the target is in range (also don't mess with ourself)
		if(dist >= radius || target == self || !(target->flags & FL_SHOOTABLE))
			continue;
		// Next see if we should damage the target
		if(!(flags&XF_HURTSOURCE) &&
			!((self->target && self->target->player) ^ (!!target->player)))
			continue;

		double output = damage;
		if(dist > fulldamageradius)
			output *= 1.0 - static_cast<double>(dist - fulldamageradius)*rolloff;
		if(output <= 0.0)
			continue;

		DamageActor(target, self->target, static_cast<unsigned int>(output));
	}
	return true;
}

ACTION_FUNCTION(A_FaceTarget)
{
	ACTION_PARAM_DOUBLE(max_turn, 0);
	ACTION_PARAM_DOUBLE(max_pitch, 1);

	A_Face(self, self->target, angle_t(max_turn*ANGLE_45/45));
	return true;
}

ACTION_FUNCTION(A_Fall)
{
	self->flags &= ~FL_SOLID;
	return true;
}

ACTION_FUNCTION(A_GiveExtraMan)
{
	ACTION_PARAM_INT(amount, 0);

	if(self->player)
		self->player->GiveExtraMan(amount);
	return true;
}

ACTION_FUNCTION(A_GiveInventory)
{
	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_INT(amount, 1);

	const ClassDef *cls = ClassDef::FindClass(className);

	if(amount == 0)
		amount = 1;

	if(cls && cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
	{
		return self->GiveInventory(cls, amount);
	}
	return true;
}

ACTION_FUNCTION(A_GunFlash)
{
	if(!self->player)
		return false;

	ACTION_PARAM_STATE(flash, 0, self->player->ReadyWeapon->FindState(self->player->ReadyWeapon->mode != AWeapon::AltFire ? NAME_Flash : NAME_AltFlash));

	if(self->MeleeState)
		self->SetState(self->MeleeState);
	self->player->SetPSprite(flash, player_t::ps_flash);
	return true;
}

#define STATE_JUMP(frame) DoStateJump(frame, self, caller, args, result)
static void DoStateJump(const Frame *frame, AActor *self, const Frame * const caller, const CallArguments &args, ActionResult *result)
{
	if(!frame)
		return;

	if(result)
	{
		result->JumpFrame = frame;
		return;
	}

	if(self->player)
	{
		if(self->player->psprite[player_t::ps_weapon].frame == caller)
		{
			self->player->SetPSprite(frame, player_t::ps_weapon);
			return;
		}
		else if(self->player->psprite[player_t::ps_flash].frame == caller)
		{
			self->player->SetPSprite(frame, player_t::ps_flash);
			return;
		}
	}

	self->SetState(frame);
}

static FRandom pr_cajump("CustomJump");
ACTION_FUNCTION(A_Jump)
{
	ACTION_PARAM_INT(chance, 0);

	if(chance >= 256 || pr_cajump() < chance)
	{
		ACTION_PARAM_STATE(frame, (ACTION_PARAM_COUNT == 2 ? 1 : (1 + pr_cajump() % (ACTION_PARAM_COUNT - 1))), NULL);

		STATE_JUMP(frame);
	}

	// Jumps will always return false so that they don't trigger success as a whole
	return false;
}

ACTION_FUNCTION(A_JumpIf)
{
	ACTION_PARAM_BOOL(expr, 0);
	ACTION_PARAM_STATE(frame, 1, NULL);

	if(expr)
		STATE_JUMP(frame);

	// Jumps will always return false so that they don't trigger success as a whole
	return false;
}

ACTION_FUNCTION(A_JumpIfCloser)
{
	ACTION_PARAM_DOUBLE(distance, 0);
	ACTION_PARAM_STATE(frame, 1, NULL);

	AActor *check;
	if(self->player)
		check = self->player->FindTarget();
	else
		check = self->target;

	// << 6 - Adjusts to Doom scale
	if(check && P_AproxDistance((self->x-check->x)<<6, (self->y-check->y)<<6) < (fixed)(distance*FRACUNIT))
	{
		STATE_JUMP(frame);
	}

	// Jumps will always return false so that they don't trigger success as a whole
	return false;
}

ACTION_FUNCTION(A_JumpIfInventory)
{
	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_INT(amount, 1);
	ACTION_PARAM_STATE(frame, 2, NULL);

	const ClassDef *cls = ClassDef::FindClass(className);
	AInventory *inv = self->FindInventory(cls);

	if(!inv)
		return false;

	// Amount of 0 means check if the amount is the maxamount.
	// Otherwise check if we have at least that amount.
	if((amount == 0 && inv->amount == inv->maxamount) ||
		(amount > 0 && inv->amount >= static_cast<unsigned int>(amount)))
	{
		STATE_JUMP(frame);
	}

	// Jumps will always return false so that they don't trigger success as a whole
	return false;
}

ACTION_FUNCTION(A_Light)
{
	ACTION_PARAM_INT(level, 0);

	self->player->extralight = clamp(level, -20, 20);
	return true;
}
// Might as well support these as well since they're far more popular than the
// generic version and don't require much code.
ACTION_FUNCTION(A_Light0) { self->player->extralight = 0; return true; }
ACTION_FUNCTION(A_Light1) { self->player->extralight = 1; return true; }
ACTION_FUNCTION(A_Light2) { self->player->extralight = 2; return true; }

static FRandom pr_meleeattack("MeleeAccuracy");
ACTION_FUNCTION(A_MeleeAttack)
{
	ACTION_PARAM_INT(damage, 0);
	ACTION_PARAM_DOUBLE(accuracy, 1);
	ACTION_PARAM_STRING(hitsound, 2);
	ACTION_PARAM_STRING(misssound, 3);

	if(misssound.Compare("*") == 0)
		misssound = hitsound;

	A_Face(self, self->target);
	if(CheckMeleeRange(self, self->target, self->speed))
	{
		if(pr_meleeattack() < static_cast<int>(accuracy*256))
		{
			DamageActor(self->target, self, damage);
			if(!hitsound.IsEmpty())
				PlaySoundLocActor(hitsound, self);
			return true;
		}
	}
	if(!misssound.IsEmpty())
		PlaySoundLocActor(misssound, self);
	return false;
}

static FRandom pr_monsterrefire("MonsterRefire");
ACTION_FUNCTION(A_MonsterRefire)
{
	ACTION_PARAM_INT(probability, 0);
	ACTION_PARAM_STATE(jump, 1, NULL);

	AActor *target = self->target;
	A_Face(self, target);

	if(pr_monsterrefire() < probability)
		return false;

	if(jump && (
		!(self->flags & FL_ATTACKMODE) ||
		!target ||
		target->health <= 0 ||
		!CheckLine(self, target)
	))
	{
		STATE_JUMP(jump);
	}
	return true;
}

ACTION_FUNCTION(A_Pain)
{
	PlaySoundLocActor(self->painsound, self);
	return true;
}

ACTION_FUNCTION(A_PlaySound)
{
	ACTION_PARAM_STRING(sound, 0);

	PlaySoundLocActor(sound, self);
	return true;
}

ACTION_FUNCTION(A_ScaleVelocity)
{
	ACTION_PARAM_DOUBLE(scale, 0);

	self->velx = FLOAT2FIXED(self->velx*scale);
	self->vely = FLOAT2FIXED(self->vely*scale);
	return true;
}

ACTION_FUNCTION(A_SetTics)
{
	ACTION_PARAM_DOUBLE(duration, 0);

	if(self->player)
	{
		if(self->player->psprite[player_t::ps_weapon].frame == caller)
		{
			self->player->psprite[player_t::ps_weapon].ticcount = static_cast<int> (duration*2);
			return true;
		}
		else if(self->player->psprite[player_t::ps_flash].frame == caller)
		{
			self->player->psprite[player_t::ps_flash].ticcount = static_cast<int> (duration*2);
			return true;
		}
	}

	self->ticcount = static_cast<int> (duration*2);
	return true;
}

ACTION_FUNCTION(A_SpawnItem)
{
	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_DOUBLE(distance, 1);
	ACTION_PARAM_DOUBLE(zheight, 2);

	const ClassDef *cls = ClassDef::FindClass(className);
	if(cls == NULL)
		return false;

	AActor *newobj = AActor::Spawn(cls,
		self->x + fixed(distance*finecosine[self->angle>>ANGLETOFINESHIFT])/64,
		self->y - fixed(distance*finesine[self->angle>>ANGLETOFINESHIFT])/64,
		0, SPAWN_AllowReplacement);
	return true;
}

static FRandom pr_spawnitemex("SpawnItemEx");
ACTION_FUNCTION(A_SpawnItemEx)
{
	enum
	{
		SXF_TRANSFERPOINTERS = 0x1
	};

	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_DOUBLE(xoffset, 1);
	ACTION_PARAM_DOUBLE(yoffset, 2);
	ACTION_PARAM_DOUBLE(zoffset, 3);
	ACTION_PARAM_DOUBLE(xvel, 4);
	ACTION_PARAM_DOUBLE(yvel, 5);
	ACTION_PARAM_DOUBLE(zvel, 6);
	ACTION_PARAM_DOUBLE(angle, 7);
	ACTION_PARAM_INT(flags, 8);
	ACTION_PARAM_INT(chance, 9);

	if(chance > 0 && pr_spawnitemex() < chance)
		return false;

	const ClassDef *cls = ClassDef::FindClass(className);
	if(cls == NULL)
		return false;

	angle_t ang = self->angle>>ANGLETOFINESHIFT;

	fixed x = self->x + fixed(xoffset*finecosine[ang])/64 + fixed(yoffset*finesine[ang])/64;
	fixed y = self->y - fixed(xoffset*finesine[ang])/64 + fixed(yoffset*finecosine[ang])/64;
	angle = angle_t((angle*ANGLE_45)/45) + self->angle;

	AActor *newobj = AActor::Spawn(cls, x, y, 0, SPAWN_AllowReplacement);

	if(flags & SXF_TRANSFERPOINTERS)
	{
		newobj->flags |= self->flags&(FL_ATTACKMODE|FL_FIRSTATTACK);
		newobj->flags &= ~(~self->flags&(FL_PATHING));
		if(newobj->flags & FL_ATTACKMODE)
			newobj->speed = newobj->runspeed;
	}

	newobj->angle = static_cast<angle_t>(angle);

	//We divide by 128 here since Wolf is 70hz instead of 35.
	newobj->velx = (fixed(xvel*finecosine[ang]) + fixed(yvel*finesine[ang]))/128;
	newobj->vely = (-fixed(xvel*finesine[ang]) + fixed(yvel*finecosine[ang]))/128;
	return true;
}

ACTION_FUNCTION(A_Stop)
{
	self->velx = 0;
	self->vely = 0;
	self->dir = nodir;
	return true;
}

ACTION_FUNCTION(A_TakeInventory)
{
	ACTION_PARAM_STRING(className, 0);
	ACTION_PARAM_INT(amount, 1);

	const ClassDef *cls = ClassDef::FindClass(className);
	AInventory *inv = self->FindInventory(cls);
	if(inv)
	{
		// Taking an amount of 0 mean take all
		if(amount == 0 || amount >= static_cast<int>(inv->amount))
			inv->Destroy();
		else
			inv->amount -= amount;
		return true;
	}
	return false;
}

#include "wl_main.h"
ACTION_FUNCTION(A_ZoomFactor)
{
	enum
	{
		ZOOM_INSTANT = 1,
		ZOOM_NOSCALETURNING = 2
	};

	ACTION_PARAM_DOUBLE(zoom, 0);
	ACTION_PARAM_INT(flags, 1);

	if(!self->player || !self->player->ReadyWeapon)
		return false;

	self->player->ReadyWeapon->fovscale = 1.0f / clamp<float>((float)zoom, 0.1f, 50.0f);
	if(flags & ZOOM_INSTANT)
		self->player->FOV = -self->player->DesiredFOV*self->player->ReadyWeapon->fovscale;
	if(flags & ZOOM_NOSCALETURNING)
		self->player->ReadyWeapon->fovscale *= -1;
	return true;
}
