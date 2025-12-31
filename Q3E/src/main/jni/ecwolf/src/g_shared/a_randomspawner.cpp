/*
** a_randomspawner.cpp
** A thing that randomly spawns one item in a list of many, before disappearing.
** bouncecount is used to keep track of recursions (so as to prevent infinite loops).
** Species is used to store the index of the spawned actor's name.
*/

#include "actor.h"
#include "m_random.h"
#include "thingdef/thingdef.h"
#include "wl_net.h"

static const int MAX_RANDOMSPAWNERS_RECURSION = 32; // Should be largely more than enough, honestly.
static FRandom pr_randomspawn("RandomSpawn");

class ARandomSpawner : public AActor
{
	DECLARE_CLASS (ARandomSpawner, AActor)

	int bouncecount;
	FName Species;
	TObjPtr<AActor> tracer;

	// To handle "RandomSpawning" missiles, the code has to be split in two parts.
	// If the following code is not done in BeginPlay, missiles will use the
	// random spawner's velocity (0...) instead of their own.
	void BeginPlay()
	{
		DropList::Iterator di; // di will be our drop item list iterator
		DropList *drop; // while drop stays as the reference point.
		int n=0;
		bool nomonsters = Net::NoMonsters();

		//Super::BeginPlay();
		drop = GetDropList();
		di = drop->Head();
		if (di)
		{
			do
			{
				if (di->className != NAME_None)
				{
					if (!nomonsters || !(ClassDef::FindClass(di->className)->GetDefault()->flags & FL_ISMONSTER))
					{
						if (di->amount == 0) di->amount = 1; // default value is -1, we need a positive value.
						n += di->amount; // this is how we can weight the list.
					}
					++di;
				}
			}
			while(di);
			if (n == 0)
			{ // Nothing left to spawn. They must have all been monsters, and monsters are disabled.
				Destroy();
				return;
			}
			// Then we reset the iterator to the start position...
			di = drop->Head();
			// Take a random number...
			n = pr_randomspawn(n);
			// And iterate in the array up to the random number chosen.
			while (n > -1 && di)
			{
				if (di->className != NAME_None &&
					(!nomonsters || !(ClassDef::FindClass(di->className)->GetDefault()->flags & FL_ISMONSTER)))
				{
					n -= di->amount;
					if (di.HasNext() && (n > -1))
						++di;
					else
						n = -1;
				}
				else
				{
					++di;
				}
			}
			// So now we can spawn the dropped item.
			if (!di || bouncecount >= MAX_RANDOMSPAWNERS_RECURSION)	// Prevents infinite recursions
			{
				Spawn(ClassDef::FindClass("Unknown"), x, y, 0, false);		// Show that there's a problem.
				Destroy();
				return;
			}
			else if (pr_randomspawn() <= di->probability)	// prob 255 = always spawn, prob 0 = never spawn.
			{
				// Handle replacement here so as to get the proper speed and flags for missiles
				const ClassDef *cls;
				cls = ClassDef::FindClass(di->className);
				if (cls != NULL)
				{
					const ClassDef *rep = cls->GetReplacement();
					if (rep != NULL)
					{
						cls = rep;
					}
				}
				if (cls != NULL)
				{
					Species = cls->GetName();
					const AActor *defmobj = cls->GetDefault();
					this->speed   =  defmobj->speed;
					this->flags  |= (defmobj->flags  & FL_MISSILE);
					/*AActor *defmobj = GetDefaultByType(cls);
					this->Speed   =  defmobj->Speed;
					this->flags  |= (defmobj->flags  & MF_MISSILE);
					this->flags2 |= (defmobj->flags2 & MF2_SEEKERMISSILE);
					this->flags4 |= (defmobj->flags4 & MF4_SPECTRAL);*/
				}
				else
				{
					Species = NAME_None;
				}
			}
		}
	}

	// The second half of random spawning. Now that the spawner is initialized, the
	// real actor can be created. If the following code were in BeginPlay instead,
	// missiles would not have yet obtained certain information that is absolutely
	// necessary to them -- such as their source and destination.
	void PostBeginPlay()
	{
		AActor * newmobj = NULL;
		bool boss = false;
		//Super::PostBeginPlay();
		if (Species == NAME_None) { Destroy(); return; }
		const ClassDef * cls = ClassDef::FindClass(Species);
		// Flags passed to Spawn(). Currently only SPAWN_Patrol is relevant, since
		// class replacement is already handled in BeginPlay().
		int spawnflags = 0;
		if (flags & FL_PATHING) spawnflags |= SPAWN_Patrol;
		/*if (this->flags & MF_MISSILE && target && target->target) // Attempting to spawn a missile.
		{
			if ((tracer == NULL) && (flags2 & MF2_SEEKERMISSILE)) tracer = target->target;
			newmobj = P_SpawnMissileXYZ(x, y, z, target, target->target, cls, false);
		}
		else*/ newmobj = Spawn(cls, x, y, 0, spawnflags);
		if (newmobj != NULL)
		{
			// copy everything relevant
			newmobj->angle = angle;
			newmobj->dir = dir;
			newmobj->flags |= (flags & FL_AMBUSH);
			newmobj->target = target;
			/*newmobj->SpawnAngle = newmobj->angle = angle;
			newmobj->SpawnPoint[2] = SpawnPoint[2];
			newmobj->special    = special;
			newmobj->args[0]    = args[0];
			newmobj->args[1]    = args[1];
			newmobj->args[2]    = args[2];
			newmobj->args[3]    = args[3];
			newmobj->args[4]    = args[4];
			newmobj->special1   = special1;
			newmobj->special2   = special2;
			newmobj->SpawnFlags = SpawnFlags;
			newmobj->HandleSpawnFlags();
			newmobj->tid        = tid;
			newmobj->AddToHash();
			newmobj->velx = velx;
			newmobj->vely = vely;
			newmobj->velz = velz;
			newmobj->master = master;	// For things such as DamageMaster/DamageChildren, transfer mastery.
			newmobj->target = target;
			newmobj->tracer = tracer;
			newmobj->CopyFriendliness(this, false);*/
			// This handles things such as projectiles with the MF4_SPECTRAL flag that have
			// a health set to -2 after spawning, for internal reasons.
			//if (health != SpawnHealth()) newmobj->health = health;
			//if (!(flags & MF_DROPPED)) newmobj->flags &= ~MF_DROPPED;
			// Handle special altitude flags
			/*if (newmobj->flags & MF_SPAWNCEILING)
			{
				newmobj->z = newmobj->ceilingz - newmobj->height - SpawnPoint[2];
			}
			else if (newmobj->flags2 & MF2_SPAWNFLOAT) 
			{
				fixed_t space = newmobj->ceilingz - newmobj->height - newmobj->floorz;
				if (space > 48*FRACUNIT)
				{
					space -= 40*FRACUNIT;
					newmobj->z = MulScale8 (space, pr_randomspawn()) + newmobj->floorz + 40*FRACUNIT;
				}
				newmobj->z += SpawnPoint[2];
			}
			if (newmobj->flags & MF_MISSILE)
				P_CheckMissileSpawn(newmobj);*/
			// Bouncecount is used to count how many recursions we're in.
			if (newmobj->IsKindOf(ClassDef::FindClass("RandomSpawner")))
				((ARandomSpawner*)newmobj)->bouncecount = ++bouncecount;
			// If the spawned actor has either of those flags, it's a boss.
			//if ((newmobj->flags4 & MF4_BOSSDEATH) || (newmobj->flags2 & MF2_BOSS))
			//	boss = true;
			// If a replaced actor has either of those same flags, it's also a boss.
			//AActor * rep = GetDefaultByType(GetClass()->ActorInfo->GetReplacee()->Class);
			//if (rep && ((rep->flags4 & MF4_BOSSDEATH) || (rep->flags2 & MF2_BOSS)))
			//	boss = true;
		}
		if (boss)
			this->tracer = newmobj;
		else	// "else" because a boss-replacing spawner must wait until it can call A_BossDeath.
			Destroy();
	}

	void Tick()	// This function is needed for handling boss replacers
	{
		Super::Tick();
		if (tracer == NULL || tracer->health <= 0)
		{
			CALL_ACTION(A_BossDeath, this);
			Destroy();
		}
	}

};

IMPLEMENT_POINTY_CLASS (RandomSpawner)
	DECLARE_POINTER(tracer)
END_POINTERS
