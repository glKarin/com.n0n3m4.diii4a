#include "Trigger.h"
#include "framework/DeclEntityDef.h"
#include "Player.h"
#include "idlib/LangDict.h"

#include "bc_meta.h"
#include "bc_trigger_deodorant.h"

//Flammable gas cloud. Emits from deodorant and gas pipe.

CLASS_DECLARATION(idTrigger_Multi, idTrigger_deodorant)
EVENT(EV_Touch, idTrigger_deodorant::Event_Touch)
END_CLASS

idTrigger_deodorant::idTrigger_deodorant()
{
}

idTrigger_deodorant::~idTrigger_deodorant(void)
{
}

void idTrigger_deodorant::Spawn()
{
	displayName = common->GetLanguageDict()->GetString("#str_def_gameplay_flammablegas");

	fl.takedamage = true;
	idTrigger_Multi::Spawn();

	active = true;

	maxlifetime = gameLocal.time + spawnArgs.GetInt("spewLifetime");

	//Particle fx.
	particleEmitter = NULL;
	idDict splashArgs;
	splashArgs.Set("classname", "func_emitter");
	splashArgs.Set("model", spawnArgs.GetString("spewParticle", "deodorantburst01.prt"));
	splashArgs.Set("start_off", "1");
	splashArgs.SetVector("origin", this->GetPhysics()->GetOrigin());
	particleEmitter = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &splashArgs));
	particleEmitter.GetEntity()->SetActive(true);
}

void idTrigger_deodorant::Save(idSaveGame* savefile) const
{
	savefile->WriteBool( active ); // bool active
	particleEmitter.Save( savefile ); // idFuncEmitter * particleEmitter
	savefile->WriteInt( maxlifetime ); // int maxlifetime
}
void idTrigger_deodorant::Restore(idRestoreGame* savefile)
{
	savefile->ReadBool( active ); // bool active
	particleEmitter.Restore( savefile ); // idFuncEmitter * particleEmitter
	savefile->ReadInt( maxlifetime ); // int maxlifetime
}


void idTrigger_deodorant::Event_Touch(idEntity* other, trace_t* trace)
{
	if (other->IsType(idMoveableItem::Type))
	{
		if (static_cast<idMoveableItem *>(other)->IsOnFire())
		{
			//touched by an entity that is on fire, or is sparking.
			BurstIntoFlames(other);
		}
	}
	
	if (other->IsType(idPlayer::Type))
	{
		if (((idPlayer *)other)->GetSmelly())
		{
			((idPlayer *)other)->SetSmelly(0);
		}
	}
}

void idTrigger_deodorant::BurstIntoFlames(idEntity *inflictor)
{
	if (!active)
		return;

	active = false;

	if (inflictor != nullptr)
	{
		idStr inflictorname = (inflictor->displayName.Length() > 0) ? inflictor->displayName : idStr(inflictor->GetName());
		idStr ignitionText = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_cloud_ignited"), inflictorname.c_str());
		gameLocal.AddEventLog(ignitionText.c_str(), GetPhysics()->GetOrigin());

		static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->DoHighlighter(inflictor, this);
	}

	//Make a fireball.
	idEntity *fireballEnt;
	idDict args;
	args.Set("classname", "env_fireball");
	args.SetVector("origin", this->GetPhysics()->GetOrigin());
	args.Set("displayname", "#str_def_gameplay_900059"); //BC 3-20-2025: fixed loc bug (fireball)
	gameLocal.SpawnEntityDef(args, &fireballEnt);

	if (particleEmitter.IsValid())
	{
		particleEmitter.GetEntity()->PostEventMS(&EV_Remove, 500); //let the particle linger a bit so that it disappears during the middle of the fireball.
	}

	//Remove self.
	this->PostEventMS(&EV_Remove, 0);
}

void idTrigger_deodorant::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
{
	const idDeclEntityDef *damageDef = gameLocal.FindEntityDef(damageDefName, false);

	if (!damageDef)
		return;

	if (damageDef->dict.GetBool("isfire"))
	{
		BurstIntoFlames(attacker); //Received damage that has the isfire bool.
	}
}

void idTrigger_deodorant::Think()
{
	if (!active)
		return;

	if (gameLocal.time > maxlifetime && particleEmitter.IsValid())
	{
		particleEmitter.GetEntity()->SetActive(false);
		particleEmitter.GetEntity()->PostEventMS(&EV_Remove, 1000);

		this->PostEventMS(&EV_Remove, 0);
	}
}
