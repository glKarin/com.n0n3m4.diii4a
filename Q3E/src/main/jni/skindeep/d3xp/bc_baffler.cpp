#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "Moveable.h"
#include "WorldSpawn.h"
#include "SmokeParticles.h"
#include "framework/DeclEntityDef.h"
#include "Trigger.h"

#include "bc_baffler.h"

//TODO: add noise when bubble appears

const int ACTIVATIONTIME = 500;
const int GROWTIME = 500;

CLASS_DECLARATION(idMoveableItem, idBaffler)
END_CLASS

//TODO: figure out way to make this both placeable and throwable
idBaffler::idBaffler(void)
{
	bafflerNode.SetOwner(this);
	bafflerNode.AddToEnd(gameLocal.bafflerEntities);
}

idBaffler::~idBaffler(void)
{
	bafflerNode.Remove();
}

void idBaffler::Spawn(void)
{
	idDict args;

	state = BAFFLER_OFF;
	timer = gameLocal.time;
	

	args.Clear();
	args.SetVector("origin", GetPhysics()->GetOrigin());
	args.Set("model", "worldmodel_bafflerbubble");
	bubble = gameLocal.SpawnEntityType(idAnimated::Type, &args);
	bubble->Bind(this, false);
	bubble->Hide();

	healthState = BAFFLERHEALTH_PRISTINE;

	maxHealth = spawnArgs.GetInt("health");

	damageSmoke = NULL;
	damageSmokeFlyTime = 0;
}


void idBaffler::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); //  int state
	savefile->WriteInt( timer ); //  int timer

	savefile->WriteObject( bubble ); //  idEntity* bubble

	savefile->WriteParticle( soundParticles ); // const  idDeclParticle * soundParticles
	savefile->WriteInt( soundParticlesFlyTime ); //  int soundParticlesFlyTime

	savefile->WriteInt( healthState ); //  int healthState
	savefile->WriteInt( maxHealth ); //  int maxHealth

	savefile->WriteParticle( damageSmoke ); // const  idDeclParticle * damageSmoke
	savefile->WriteInt( damageSmokeFlyTime ); //  int damageSmokeFlyTime
}

void idBaffler::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); //  int state
	savefile->ReadInt( timer ); //  int timer

	savefile->ReadObject( bubble ); //  idEntity* bubble

	savefile->ReadParticle( soundParticles ); // const  idDeclParticle * soundParticles
	savefile->ReadInt( soundParticlesFlyTime ); //  int soundParticlesFlyTime

	savefile->ReadInt( healthState ); //  int healthState
	savefile->ReadInt( maxHealth ); //  int maxHealth

	savefile->ReadParticle( damageSmoke ); // const  idDeclParticle * damageSmoke
	savefile->ReadInt( damageSmokeFlyTime ); //  int damageSmokeFlyTime
}



void idBaffler::Think(void)
{
	idMoveableItem::Think();

	if (this->IsHidden())
		return;

	if (state == BAFFLER_OFF)
	{
		if (gameLocal.time > timer + ACTIVATIONTIME)
		{
			const char *smokeName = spawnArgs.GetString("smoke_fly");

			state = BAFFLER_ACTIVATING;
			((idAnimated *)bubble)->Event_PlayAnim("grow", 4, false);
			bubble->Show();
			timer = gameLocal.time + GROWTIME;

			isFrobbable = true;
			GetPhysics()->GetClipModel()->SetOwner(NULL); //So player can shoot it.

			soundParticlesFlyTime = 0;
			if (*smokeName != '\0')
			{
				soundParticles = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, smokeName));
				soundParticlesFlyTime = gameLocal.time;
			}

			StartSound("snd_spawn", SND_CHANNEL_HEART, 0, false, NULL);
		}
	}
	else if (state == BAFFLER_ACTIVATING)
	{
		if (gameLocal.time > timer)
		{
			((idAnimated *)bubble)->Event_PlayAnim("idle", 4, true);
			state = BAFFLER_ON;
			
			this->spawnArgs.SetInt("baffleactive", BAFFLE_MUTE);

						
		}
	}
	else if (state == BAFFLER_ON)
	{
		if (soundParticles != NULL && soundParticlesFlyTime && !IsHidden())
		{
			idVec3 dir = idVec3(0, 1, 0);

			if (!gameLocal.smokeParticles->EmitSmoke(soundParticles, soundParticlesFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup ))
			{
				soundParticlesFlyTime = gameLocal.time;
			}
		}

		if (damageSmoke != NULL && damageSmokeFlyTime && !IsHidden())
		{
			idVec3 dir = idVec3(0, 1, 0);

			if (!gameLocal.smokeParticles->EmitSmoke(damageSmoke, damageSmokeFlyTime, gameLocal.random.RandomFloat(), GetPhysics()->GetOrigin(), dir.ToMat3(), timeGroup))
			{
				damageSmokeFlyTime = gameLocal.time;
			}
		}
	}

}

void idBaffler::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType )
{
	idMoveableItem::Damage(inflictor, attacker, dir, damageDefName, damageScale, location);

	if (healthState == BAFFLERHEALTH_DEAD)
		return;

	if (health <= 0 && healthState != BAFFLERHEALTH_DEAD)
	{
		healthState = BAFFLERHEALTH_DEAD;

		idMoveableItem::DropItemsBurst(this, "gib", idVec3(8, 0, 0));

		//explosion fx.
		idEntityFx::StartFx("fx/explosion", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);

		//Decal.
		gameLocal.ProjectDecal(GetPhysics()->GetOrigin(), idVec3(0, 0, -1), 8.0f, true, 40, "textures/decals/scorch");
	}
	else if (health <= maxHealth * .5f)
	{
		if (healthState != BAFFLERHEALTH_HEAVYDAMAGE)
		{
			healthState = BAFFLERHEALTH_HEAVYDAMAGE;
			
			damageSmoke = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, "machine_damaged_smokeheavy.prt"));
			damageSmokeFlyTime = gameLocal.time;			
		}
	}
	else if (health <= maxHealth)
	{
		if (healthState != BAFFLERHEALTH_LIGHTDAMAGE)
		{
			healthState = BAFFLERHEALTH_LIGHTDAMAGE;

			damageSmoke = static_cast<const idDeclParticle *>(declManager->FindType(DECL_PARTICLE, "machine_damaged_smokelight.prt"));
			damageSmokeFlyTime = gameLocal.time;
		}
	}
	else if (health >= maxHealth)
	{
		if (healthState != BAFFLERHEALTH_PRISTINE)
		{
			healthState = BAFFLERHEALTH_PRISTINE;
			damageSmoke = NULL;
		}
	}

	
}

bool idBaffler::DoFrob(int index, idEntity * frobber)
{
	idVec3 fxPos = GetPhysics()->GetOrigin();
	idMat3 fxMat = mat3_identity;

	if (state == BAFFLER_DELETING)
		return false;

	state = BAFFLER_DELETING;

	fl.takedamage = false;
	PostEventMS(&EV_Remove, 100);

	//gameLocal.GetLocalPlayer()->SetAmmoDelta("ammo_baffler", 1);
	gameLocal.GetLocalPlayer()->GiveItem("weapon_sonar");

	//Particle fx for picking up item.
	idEntityFx::StartFx("fx/pickupitem", &fxPos, &fxMat, NULL, false);

	gameLocal.GetLocalPlayer()->StartSound("snd_grab", SND_CHANNEL_ANY, 0, false, NULL);

	return true;
}

