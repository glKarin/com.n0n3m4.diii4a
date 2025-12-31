/*
** actor.cpp
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

#include "actor.h"
#include "a_inventory.h"
#include "farchive.h"
#include "gamemap.h"
#include "g_mapinfo.h"
#include "id_ca.h"
#include "id_sd.h"
#include "thinker.h"
#include "thingdef/thingdef.h"
#include "thingdef/thingdef_expression.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_loadsave.h"
#include "wl_net.h"
#include "wl_state.h"
#include "id_us.h"
#include "m_random.h"

void T_ExplodeProjectile(AActor *self, AActor *target);
void T_Projectile(AActor *self);

// Old save compatibility
void AActorProxy::Serialize(FArchive &arc)
{
	Super::Serialize(arc);

	bool enabled;
	arc << enabled << actualObject;
}
IMPLEMENT_INTERNAL_CLASS(AActorProxy)

////////////////////////////////////////////////////////////////////////////////

Frame::~Frame()
{
	if(freeActionArgs)
	{
		// We need to delete CallArguments objects. Since we don't default to
		// NULL on these we need to check if a pointer has been set to know if
		// we created an object.
		if(action.pointer)
			delete action.args;
		if(thinker.pointer)
			delete thinker.args;
	}
}

static FRandom pr_statetics("StateTics");
int Frame::GetTics() const
{
	if(randDuration)
		return duration + pr_statetics.GenRand32() % (randDuration + 1);
	return duration;
}

bool Frame::ActionCall::operator() (AActor *self, AActor *stateOwner, const Frame * const caller, ActionResult *result) const
{
	if(pointer)
	{
		args->Evaluate(self);
		return pointer(self, stateOwner, caller, *args, result);
	}
	return false;
}

FArchive &operator<< (FArchive &arc, const Frame *&frame)
{
	if(arc.IsStoring())
	{
		// Find a class which held this state.
		// This should always be able to be found.
		const ClassDef *cls = NULL;
		if(frame)
		{
			ClassDef::ClassIterator iter = ClassDef::GetClassIterator();
			ClassDef::ClassPair *pair;
			while(iter.NextPair(pair))
			{
				cls = pair->Value;
				if(cls->IsStateOwner(frame))
					break;
			}

			arc << cls;
			arc << const_cast<Frame *>(frame)->index;
		}
		else
		{
			arc << cls;
		}
	}
	else
	{
		const ClassDef *cls;
		unsigned int frameIndex;

		arc << cls;
		if(cls)
		{
			arc << frameIndex;

			frame = cls->GetState(frameIndex);
		}
		else
			frame = NULL;
	}

	return arc;
}

////////////////////////////////////////////////////////////////////////////////

EmbeddedList<AActor>::List AActor::actors;
PointerIndexTable<ExpressionNode> AActor::damageExpressions;
PointerIndexTable<AActor::DropList> AActor::dropItems;
IMPLEMENT_POINTY_CLASS(Actor)
	DECLARE_POINTER(inventory)
	DECLARE_POINTER(target)
END_POINTERS

void AActor::AddInventory(AInventory *item)
{
	item->AttachToOwner(this);

	if(inventory == NULL)
		inventory = item;
	else
	{
		AInventory *next = inventory;
		do
		{
			if(next->inventory == NULL)
			{
				next->inventory = item;

				// Prevent the GC from cleaning this item
				GC::WriteBarrier(item);
				break;
			}
		}
		while((next = next->inventory));
	}
}

// This checks if this can see the specified actor. It replaces FL_VISABLE checks.
bool AActor::CheckVisibility(const AActor *check, angle_t fov) const
{
	float angle = (float) atan2 ((float) (check->y - y), (float) (check->x - x));
	if (angle<0)
		angle = (float) (M_PI*2+angle);
	angle_t iangle = 0-(angle_t)(angle*ANGLE_180/M_PI);
	angle_t lowerAngle = MIN(iangle, this->angle);
	angle_t upperAngle = MAX(iangle, this->angle);

	return MIN(upperAngle - lowerAngle, lowerAngle - upperAngle) <= fov && CheckLine(check, this);
}

void AActor::ClearCounters()
{
	if(flags & FL_COUNTITEM)
		--gamestate.treasuretotal;
	if((flags & FL_COUNTKILL) && health > 0)
		--gamestate.killtotal;
	if(flags & FL_COUNTSECRET)
		--gamestate.secrettotal;
	flags &= ~(FL_COUNTITEM|FL_COUNTKILL|FL_COUNTSECRET);
}

void AActor::ClearInventory()
{
	while(inventory)
		RemoveInventory(inventory);
}

void AActor::Destroy()
{
	Super::Destroy();
	RemoveFromWorld();

	// Inventory items don't have a registered thinker so we must free them now
	if(inventory)
	{
		inventory->Destroy();
		inventory = NULL;
	}
}

static FRandom pr_dropitem("DropItem");
void AActor::Die()
{
	if(target && target->player)
		target->player->GivePoints(points);
	else if(points)
	{
		// The targetting system may need some refinement, so if we don't have
		// a usable target to give points to then we should give to player 1
		// and possibly investigate.
		players[0].GivePoints(points);
		NetDPrintf("%s %d points with no target\n", __FUNCTION__, points);
	}

	if(flags & FL_COUNTKILL)
		gamestate.killcount++;
	flags &= ~FL_SHOOTABLE;

	if(flags & FL_MISSILE)
	{
		T_ExplodeProjectile(this, NULL);
		return;
	}

	DropList *dropitems = GetDropList();
	if(dropitems)
	{
		DropList::Iterator item = dropitems->Head();
		DropList::Iterator bestDrop = NULL; // For FL_DROPBASEDONTARGET
		do
		{
			DropList::Iterator drop = item;
			if(pr_dropitem() <= drop->probability)
			{
				const ClassDef *cls = ClassDef::FindClass(drop->className);
				if(cls)
				{
					if(flags & FL_DROPBASEDONTARGET)
					{
						AInventory *inv = target ? target->FindInventory(cls->GetReplacement()) : NULL;
						if(!inv || !bestDrop)
							bestDrop = drop;

						if(item.HasNext())
							continue;
						else
						{
							cls = ClassDef::FindClass(bestDrop->className);
							drop = bestDrop;
						}
					}

					// We can't use tilex/tiley since it's used primiarily by
					// the AI, so it can be off by one.
					static const fixed TILEMASK = ~(TILEGLOBAL-1);

					AActor * const actor = AActor::Spawn(cls, (x&TILEMASK)+TILEGLOBAL/2, (y&TILEMASK)+TILEGLOBAL/2, 0, SPAWN_AllowReplacement);
					actor->angle = angle;
					actor->dir = dir;

					if(cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
					{
						AInventory * const inv = static_cast<AInventory *>(actor);

						// Ammo is multiplied by 0.5
						if(drop->amount > 0)
						{
							// TODO: When a dropammofactor is specified it should
							// apply here too.
							inv->amount = drop->amount;
						}
						else if(cls->IsDescendantOf(NATIVE_CLASS(Ammo)) && inv->amount > 1)
							inv->amount /= 2;

						// TODO: In ZDoom dropped weapons have their ammo multiplied as well
						// but in Wolf3D weapons always have 6 bullets.
					}
				}
			}
		}
		while(item.Next());
	}

	bool isExtremelyDead = health < -GetClass()->Meta.GetMetaInt(AMETA_GibHealth, (GetDefault()->health*gameinfo.GibFactor)>>FRACBITS);
	const Frame *deathstate = NULL;
	if(isExtremelyDead)
		deathstate = FindState(NAME_XDeath);
	if(!deathstate)
		deathstate = FindState(NAME_Death);
	if(deathstate)
		SetState(deathstate);
	else
		Destroy();
}

void AActor::EnterZone(const MapZone *zone)
{
	if(zone)
		soundZone = zone;
}

AInventory *AActor::FindInventory(const ClassDef *cls)
{
	if(inventory == NULL)
		return NULL;

	AInventory *check = inventory;
	do
	{
		if(check->IsA(cls))
			return check;
	}
	while((check = check->inventory));
	return NULL;
}

const Frame *AActor::FindState(const FName &name) const
{
	return GetClass()->FindState(name);
}

int AActor::GetDamage()
{
	int expression = GetClass()->Meta.GetMetaInt(AMETA_Damage, -1);
	if(expression >= 0)
		return static_cast<int>(damageExpressions[expression]->Evaluate(this).GetInt());
	return 0;
}

AActor::DropList *AActor::GetDropList() const
{
	int dropitemsIndex = GetClass()->Meta.GetMetaInt(AMETA_DropItems, -1);
	if(dropitemsIndex == -1)
		return NULL;
	return dropItems[dropitemsIndex];
}

const AActor *AActor::GetDefault() const
{
	return GetClass()->GetDefault();
}

bool AActor::GiveInventory(const ClassDef *cls, int amount, bool allowreplacement)
{
	AInventory *inv = (AInventory *) AActor::Spawn(cls, 0, 0, 0, allowreplacement ? SPAWN_AllowReplacement : 0);

	if(amount)
	{
		if(inv->IsKindOf(NATIVE_CLASS(Health)))
			inv->amount *= amount;
		else
			inv->amount = amount;
	}

	inv->ClearCounters();
	inv->RemoveFromWorld();
	if(!inv->CallTryPickup(this))
	{
		inv->Destroy();
		return false;
	}
	return true;
}

void AActor::Init()
{
	Super::Init();

	ObjectFlags |= OF_JustSpawned;

	distance = 0;
	dir = nodir;
	soundZone = NULL;
	inventory = NULL;

	actors.Push(this);
	if(!loadedgame)
		Activate();

	if(SpawnState)
		SetState(SpawnState, true);
	else
	{
		state = NULL;
		Destroy();
	}
}

// Approximate if a state sequence is running by checking if we are in a
// contiguous sequence.
bool AActor::InStateSequence(const Frame *basestate) const
{
	if(!basestate)
		return false;

	while(state != basestate)
	{
		if(basestate->next != basestate+1)
			return false;
		++basestate;
	}
	return true;
}

bool AActor::IsFast() const
{
	return (flags & FL_ALWAYSFAST) || gamestate.difficulty->FastMonsters;
}

void AActor::PrintInventory()
{
	Printf("%s inventory:\n", GetClass()->GetName().GetChars());
	AInventory *item = inventory;
	while(item)
	{
		Printf("  %s (%d/%d)\n", item->GetClass()->GetName().GetChars(), item->amount, item->maxamount);
		item = item->inventory;
	}
}

void AActor::Serialize(FArchive &arc)
{
	bool hasActorRef = actors.IsLinked(this);

	if(arc.IsStoring())
		arc.WriteSprite(sprite);
	else
		sprite = arc.ReadSprite();

	BYTE dir = this->dir;
	arc << dir;
	this->dir = static_cast<dirtype>(dir);

	arc << flags
		<< distance
		<< x
		<< y;
	if(GameSave::SaveProdVersion >= 0x001003FF && GameSave::SaveVersion >= 1507591295)
		arc << z;
	arc << velx
		<< vely
		<< angle
		<< pitch
		<< health
		<< speed
		<< runspeed
		<< points
		<< radius
		<< ticcount
		<< state
		<< viewx
		<< viewheight
		<< transx
		<< transy;
	if(GameSave::SaveVersion >= 1393719642)
		arc << overheadIcon;
	arc << sighttime
		<< sightrandom
		<< minmissilechance
		<< painchance
		<< missilefrequency
		<< movecount
		<< meleerange
		<< activesound
		<< attacksound
		<< deathsound
		<< seesound
		<< painsound
		<< temp1
		<< hidden
		<< player
		<< inventory
		<< soundZone;
	if(GameSave::SaveProdVersion >= 0x001003FF && GameSave::SaveVersion >= 1459043051)
		arc << target;
	if(arc.IsLoading() && (GameSave::SaveProdVersion < 0x001002FF || GameSave::SaveVersion < 1382102747))
	{
		TObjPtr<AActorProxy> proxy;
		arc << proxy;
	}
	arc << hasActorRef;

	if(GameSave::SaveProdVersion >= 0x001002FF && GameSave::SaveVersion > 1374914454)
		arc << projectilepassheight;

	if(arc.IsLoading() && !hasActorRef)
		actors.Remove(this);

	Super::Serialize(arc);
}

void AActor::SetIdle()
{
	if(const Frame *idle = FindState(NAME_Idle))
		SetState(idle);
	else
		SetState(SpawnState);
}

void AActor::SetState(const Frame *state, bool norun)
{
	if(state == NULL)
	{
		Destroy();
		return;
	}

	this->state = state;
	sprite = state->spriteInf;
	ticcount = state->GetTics();
	if(!norun)
	{
		state->action(this, this, state);

		while(ticcount == 0)
		{
			this->state = this->state->next;
			if(!this->state)
			{
				Destroy();
				break;
			}
			else
			{
				sprite = this->state->spriteInf;
				ticcount = this->state->GetTics();
				this->state->action(this, this, this->state);
			}
		}
	}
}

void AActor::SpawnFog()
{
	if(const ClassDef *cls = ClassDef::FindClass("TeleportFog"))
	{
		AActor *fog = Spawn(cls, x, y, 0, SPAWN_AllowReplacement);
		fog->angle = angle;
		fog->target = this;
	}
}

bool AActor::Teleport(fixed x, fixed y, angle_t angle, bool nofog)
{
	const MapSpot destination = map->GetSpot(x>>FRACBITS, y>>FRACBITS, 0);

	// For non-players, only teleport if spot is clear.
	if(!player)
	{
		if(!TrySpot(this, destination))
			return false;
	}

	if(!nofog)
		SpawnFog(); // Source fog

	this->x = x;
	this->y = y;
	this->angle = angle;

	EnterZone(destination->zone);

	if(!nofog)
		SpawnFog(); // Destination fog
	return true;
}

void AActor::Tick()
{
	// If we just spawned we're not ready to be ticked yet
	// Otherwise we might tick on the same tick we're spawned which would cause
	// an actor with a duration of 1 tic to never display
	if(ObjectFlags & OF_JustSpawned)
	{
		ObjectFlags &= ~OF_JustSpawned;
		return;
	}

	if(state == NULL)
	{
		Destroy();
		return;
	}

	if(ticcount > 0)
		--ticcount;

	if(ticcount == 0)
	{
		SetState(state->next);
		if(ObjectFlags & OF_EuthanizeMe)
			return;
	}

	state->thinker(this, this, state);

	if(flags & FL_MISSILE)
		T_Projectile(this);
}

// Remove an actor from the game world without destroying it.  This will allow
// us to transfer items into inventory for example.
void AActor::RemoveFromWorld()
{
	actors.Remove(this);
	if(IsThinking())
		Deactivate();
}

void AActor::RemoveInventory(AInventory *item)
{
	if(inventory == NULL)
		return;

	AInventory **next = &inventory;
	do
	{
		if(*next == item)
		{
			*next = (*next)->inventory;
			break;
		}
	}
	while(*next && (next = &(*next)->inventory));

	item->DetachFromOwner();
}

/* When we spawn an actor we add them to this list. After the tic has finished
 * processing we process this list to handle any start up actions.
 *
 * This is done so that we don't duplicate tics and actors appear on screen
 * when they should. We can't do this in Spawn() since we want certain
 * properties of the actor (velocity) to be setup before calling actions.
 */
static TArray<AActor *> SpawnedActors;
void AActor::FinishSpawningActors()
{
	unsigned int i = SpawnedActors.Size();
	while(i-- > 0)
	{
		AActor * const actor = SpawnedActors[i];

		// Run the first action pointer and all zero tic states!
		actor->SetState(actor->state);
		actor->ObjectFlags &= ~OF_JustSpawned;
	}
	SpawnedActors.Clear();
}

FRandom pr_spawnmobj("SpawnActor");
AActor *AActor::Spawn(const ClassDef *type, fixed x, fixed y, fixed z, int flags)
{
	if(type == NULL)
	{
		printf("Tried to spawn classless actor.\n");
		return NULL;
	}

	if(flags & SPAWN_AllowReplacement)
		type = type->GetReplacement();

	AActor *actor = type->CreateInstance();
	actor->x = x;
	actor->y = y;
	actor->z = z;
	actor->velx = 0;
	actor->vely = 0;
	actor->health = actor->SpawnHealth();

	MapSpot spot = map->GetSpot(actor->tilex, actor->tiley, 0);
	actor->EnterZone(spot->zone);

	// Execute begin play hook and then check if the actor is still alive.
	actor->BeginPlay();
	if(actor->ObjectFlags & OF_EuthanizeMe)
		return NULL;

	if(actor->flags & FL_COUNTKILL)
		++gamestate.killtotal;
	if(actor->flags & FL_COUNTITEM)
		++gamestate.treasuretotal;
	if(actor->flags & FL_COUNTSECRET)
		++gamestate.secrettotal;

	if(levelInfo && levelInfo->SecretDeathSounds)
	{
		const char* snd = type->Meta.GetMetaString(AMETA_SecretDeathSound);
		if(snd)
			actor->deathsound = snd;
	}

	if(actor->flags & FL_MISSILE)
	{
		PlaySoundLocActor(actor->seesound, actor);
		if((actor->flags & FL_RANDOMIZE) && actor->ticcount > 0)
		{
			actor->ticcount -= pr_spawnmobj() & 7;
			if(actor->ticcount < 1)
				actor->ticcount = 1;
		}
	}
	else
	{
		if((actor->flags & FL_RANDOMIZE) && actor->ticcount > 0)
			actor->ticcount = pr_spawnmobj() % actor->ticcount;
	}

	// Change between patrolling and normal spawn and also execute any zero
	// tic functions.
	if(flags & SPAWN_Patrol)
	{
		actor->flags |= FL_PATHING;

		// Pathing monsters should take at least a one tile step.
		// Otherwise the paths will break early.
		actor->distance = TILEGLOBAL;
		if(actor->PathState)
		{
			actor->SetState(actor->PathState, true);
			if(actor->flags & FL_RANDOMIZE)
				actor->ticcount = pr_spawnmobj() % actor->ticcount;
		}
	}

	SpawnedActors.Push(actor);

	return actor;
}

int32_t AActor::SpawnHealth() const
{
	return GetClass()->Meta.GetMetaInt(AMETA_DefaultHealth1 + gamestate.difficulty->SpawnFilter, health);
}

DEFINE_SYMBOL(Actor, angle)
DEFINE_SYMBOL(Actor, health)

//==============================================================================

/*
===================
=
= Player travel functions
=
===================
*/

void StartTravel ()
{
	// Set thinker priorities to TRAVEL so that they don't get wiped on level
	// load.  We'll transfer them to a new actor.

	for(unsigned int i = 0;i < Net::InitVars.numPlayers;++i)
	{
		AActor *player = players[i].mo;

		player->SetPriority(ThinkerList::TRAVEL);
	}
}

void FinishTravel ()
{
	gamestate.victoryflag = false;

	ThinkerList::Iterator node = thinkerList.GetHead(ThinkerList::TRAVEL);
	while(node)
	{
		AActor *actor = static_cast<AActor *>((Thinker*)node);
		node.Next();

		if(actor->IsKindOf(NATIVE_CLASS(PlayerPawn)))
		{
			APlayerPawn *pawn = static_cast<APlayerPawn *>(actor);
			player_t *player = pawn->player;

			// The player_t::mo has been replaced with a newly spawned player
			// we want to transfer properties from the new player object onto
			// the old one and then put the old one in place of the new one.
			APlayerPawn *tmppawn = player->mo;
			pawn->x = tmppawn->x;
			pawn->y = tmppawn->y;
			pawn->angle = tmppawn->angle;
			pawn->EnterZone(tmppawn->GetZone());

			player->mo = pawn;
			player->camera = pawn;
			tmppawn->Destroy();

			// We must move the linked list iterator here since we'll
			// transfer to the new linked list at the SetPriority call
			pawn->SetPriority(ThinkerList::PLAYER);
			continue;
		}
	}
}
