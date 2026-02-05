#include "Trigger.h"
#include "Player.h"

#include "bc_meta.h"
#include "bc_trigger_confinedarea.h"

CLASS_DECLARATION(idTrigger_Multi, idTrigger_confinedarea)
EVENT(EV_Touch, idTrigger_confinedarea::Event_Touch)
END_CLASS

idTrigger_confinedarea::idTrigger_confinedarea()
{
	confinedNode.SetOwner(this);
	confinedNode.AddToEnd(gameLocal.confinedEntities);
}

idTrigger_confinedarea::~idTrigger_confinedarea(void)
{
	confinedNode.Remove();
}

void idTrigger_confinedarea::Spawn()
{
	idTrigger_Multi::Spawn();
	lastUpdatetime = 0;
	baseAngle = spawnArgs.GetFloat("baseangle");

	if (baseAngle == 0)
		baseAngle = 1;

	playerEnterAngle = 0;
	adjustedBaseAngle = 0;

	gameLocal.DoOriginContainmentCheck(this);
}

void idTrigger_confinedarea::Save(idSaveGame* savefile) const
{
	savefile->WriteFloat( adjustedBaseAngle ); // float adjustedBaseAngle
	savefile->WriteFloat( baseAngle ); // float baseAngle
	savefile->WriteFloat( playerEnterAngle ); // float playerEnterAngle

	savefile->WriteInt( lastUpdatetime ); // int lastUpdatetime
}
void idTrigger_confinedarea::Restore(idRestoreGame* savefile)
{
	savefile->ReadFloat( adjustedBaseAngle ); // float adjustedBaseAngle
	savefile->ReadFloat( baseAngle ); // float baseAngle
	savefile->ReadFloat( playerEnterAngle ); // float playerEnterAngle

	savefile->ReadInt( lastUpdatetime ); // int lastUpdatetime
}

void idTrigger_confinedarea::Event_Touch(idEntity* other, trace_t* trace)
{
	float vdot = 0.0f;
	idAngles playerAng;
	idAngles triggerAng;


	//if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->IsPurging() && spawnArgs.GetBool("purge"))
	//{
	//	common->Printf("othername %s\n", other->GetName());
	//
	//	if (other->fl.takedamage)
	//	{
	//		other->Damage(this, this, vec3_zero, "damage_ventpurge", 1.0f, 0);
	//
	//		//special case for player. if player takes damage, make a particle effect appear beneath them.
	//		if (other->IsType(idPlayer::Type))
	//		{
	//			gameLocal.DoParticle("explosion_ventpurge.prt", gameLocal.GetLocalPlayer()->firstPersonViewOrigin + gameLocal.GetLocalPlayer()->viewAngles.ToForward() * 16);
	//		}
	//	}
	//}

	if (!other->IsType(idPlayer::Type) || lastUpdatetime > gameLocal.time || !this->GetPhysics()->GetAbsBounds().ContainsPoint(gameLocal.GetLocalPlayer()->GetEyePosition()))
		return;

	//Only need this stuff for confined tunnels. For the yellow tarp areas, skip this.
	if (spawnArgs.GetBool("confined", "1"))
	{
		//if (gameLocal.time - lastUpdatetime > SNEEZETRIGGER_UPDATERATE + 100)
		{
			playerEnterAngle = ((idPlayer *)other)->viewAngles.yaw;
		}

		lastUpdatetime = gameLocal.time + SNEEZETRIGGER_UPDATERATE;

		triggerAng = idAngles(0, baseAngle, 0);
		playerAng = idAngles(0, playerEnterAngle, 0);

		vdot = DotProduct(playerAng.ToForward(), triggerAng.ToForward());

		adjustedBaseAngle = vdot < 0 ? baseAngle + 180 : baseAngle;
	}

	((idPlayer *)other)->SetConfinedState(vdot < 0 ? baseAngle + 180 : baseAngle, spawnArgs.GetFloat("sneezemultiplier", "1"), spawnArgs.GetBool("confined", "1"));

}
