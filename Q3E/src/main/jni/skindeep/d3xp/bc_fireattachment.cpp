#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "ai/AI.h"
#include "bc_fireattachment.h"


#define DEFAULT_LIFETIME "2.5" //how long it lasts.
#define DEFAULT_DAMAGEINTERLVAL ".3" //delay between damage inflictions

CLASS_DECLARATION(idEntity, idFireAttachment)
END_CLASS

void idFireAttachment::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( lifetimer ); //  int lifetimer
	savefile->WriteInt( lifetimeMax ); //  int lifetimeMax
	savefile->WriteInt( damageTimerMax ); //  int damageTimerMax

	savefile->WriteString( damageDefname ); // idStr damageDefname

	savefile->WriteObject( attachOwner ); //  idEntityPtr<idEntity> attachOwner
	savefile->WriteObject( particleEmitter ); //  idFuncEmitter			* particleEmitter

	savefile->WriteBounds( damageBounds ); //  idBounds damageBounds
}

void idFireAttachment::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( lifetimer ); //  int lifetimer
	savefile->ReadInt( lifetimeMax ); //  int lifetimeMax
	savefile->ReadInt( damageTimerMax ); //  int damageTimerMax

	savefile->ReadString( damageDefname ); // idStr damageDefname

	savefile->ReadObject( attachOwner ); //  idEntityPtr<idEntity> attachOwner
	savefile->ReadObject( CastClassPtrRef(particleEmitter) ); //  idFuncEmitter			* particleEmitter

	savefile->ReadBounds( damageBounds ); //  idBounds damageBounds
}

idFireAttachment::idFireAttachment(void)
{
	particleEmitter = NULL;
}

idFireAttachment::~idFireAttachment(void)
{
	if (particleEmitter != NULL)
	{
		delete particleEmitter;
	}

}


void idFireAttachment::Spawn(void)
{
	fl.takedamage = false;

    //intervals between damage bursts.
    damageTimerMax = (int)(spawnArgs.GetFloat("delay", DEFAULT_DAMAGEINTERLVAL) * 1000.0f);

	//how long does it last.
    lifetimeMax = (int)(spawnArgs.GetFloat("lifetime", DEFAULT_LIFETIME) * 1000.0f);
	lifetimer = gameLocal.time + lifetimeMax;

	//damage def.
    damageDefname = spawnArgs.GetString("def_damage", "damage_generic");      

	//damage bounding box
	damageBounds = idBounds(spawnArgs.GetVector("mins", "-8 -8 0"), spawnArgs.GetVector("maxs", "8 8 60"));

	//Spawn particle emitter.
	idDict splashArgs;
	splashArgs.Set("model", spawnArgs.GetString("spewParticle", "fire_actor.prt"));
	splashArgs.Set("start_off", "1");
	splashArgs.SetVector("origin", this->GetPhysics()->GetOrigin());
	particleEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &splashArgs));
	particleEmitter->Bind(this, false);
	particleEmitter->SetActive(true);

	GetPhysics()->SetContents(0);
	GetPhysics()->SetClipMask(0);

    BecomeActive(TH_THINK);
}

void idFireAttachment::Think(void)
{
	if (IsHidden())
		return;

	//Inflict damage every XX milliseconds.
	if (attachOwner.IsValid())
	{
		if (gameLocal.time > attachOwner.GetEntity()->lastFireattachmentDamagetime)
		{
			//Inflict damage.
			attachOwner.GetEntity()->lastFireattachmentDamagetime = gameLocal.time + damageTimerMax;

			int entityCount;
			idEntity *entityList[MAX_GENTITIES];

			//attachOwner.GetEntity()->Damage(this, this, vec3_zero, damageDefname, 1.0f, 0, 0);

			//Area damage.
			entityCount = gameLocal.EntitiesWithinBoundingbox(damageBounds, GetPhysics()->GetOrigin(), entityList, MAX_GENTITIES);
			for (int i = 0; i < entityCount; i++)
			{
				idEntity *ent = entityList[i];

				if (!ent)
					continue;

				if (ent->IsHidden() || !ent->fl.takedamage || ent == this)
					continue;

				ent->Damage(this, this, vec3_zero, damageDefname, 1.0f, 0, 0);
			}
		}
	}

    if (gameLocal.time >= lifetimer)
    {
		Extinguish(); //Lifetime expiration
    }

	if (attachOwner.IsValid())
	{
		if (attachOwner.GetEntity()->health <= 0 || attachOwner.GetEntity()->IsHidden())
		{
			Extinguish();
		}
	}
}

void idFireAttachment::Extinguish()
{
	if (IsHidden())
		return;

	BecomeInactive(TH_THINK);
	this->Hide();
	PostEventMS(&EV_Remove, 0);
}

//Attach fire to something.
void idFireAttachment::AttachTo(idEntity *ent)
{
	if (ent == NULL)
		return;

	attachOwner = ent;
	SetOrigin(ent->GetPhysics()->GetOrigin());
	Bind(ent, false);
}