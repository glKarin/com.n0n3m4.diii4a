#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "bc_aloe.h"


//Almighty aloe, healer of burn wounds, cute plant


CLASS_DECLARATION(idMoveableItem, idAloe)
//EVENT(EV_Touch, idAloe::Event_Touch)
END_CLASS

void idAloe::Save(idSaveGame *savefile) const
{
	// blendo eric: no saveload data
}

void idAloe::Restore(idRestoreGame *savefile)
{
	// blendo eric: no saveload data
}

void idAloe::Spawn(void)
{
	//Allow player to clip through it.
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL | CONTENTS_TRIGGER);
	physicsObj.SetClipMask(MASK_SOLID | CONTENTS_RENDERMODEL);

	showItemLine = false;
}


void idAloe::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idMoveableItem::Killed(inflictor, attacker, damage, dir, location);

	//Spawn the leaves.
    DropLeaves();    
}


bool idAloe::DoFrob(int index, idEntity * frobber)
{
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer())
		return false;

	if (index == CARRYFROB_INDEX)
	{
		//special frob when carrying the item.

		bool isSmelly = gameLocal.GetLocalPlayer()->GetSmelly();
		if (isSmelly)
		{
			gameLocal.GetLocalPlayer()->SetSmelly(false, false);
		}
		

		int burnwoundCount = gameLocal.GetLocalPlayer()->GetWoundcount_Burn();
		if (burnwoundCount > 0)
		{
			//Use the aloe to heal burn wound.
            gameLocal.GetLocalPlayer()->HealWound_Burn();
            
			idEntityFx::StartFx("fx/aloe_use", gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), mat3_identity, NULL);
			gameLocal.GetLocalPlayer()->DropCarryable(this);
			Killed(NULL, NULL, 0, vec3_zero, 0);
		}
        
		if (!isSmelly && burnwoundCount <= 0)
		{
			// SW 17th Feb 2025: fixed missing underscore in loc string
			gameLocal.GetLocalPlayer()->SetCenterMessage("#str_def_gameplay_aloe_error");
		}
		else if (isSmelly && burnwoundCount > 0)
		{
			gameLocal.GetLocalPlayer()->SetFanfareMessage("#str_def_gameplay_aloe_healall");
		}
		else if (isSmelly)
		{
			gameLocal.GetLocalPlayer()->SetFanfareMessage("#str_def_gameplay_aloe_healsmell");
		}
		else if (burnwoundCount > 0)
		{
			gameLocal.GetLocalPlayer()->SetFanfareMessage("#str_def_gameplay_aloe_healburn");
		}
	}
    else
    {
        idMoveableItem::DoFrob(index, frobber);
    }

	return true;
}

void idAloe::DropLeaves()
{
    int leafCount = spawnArgs.GetInt("leaves", "0");
    if (leafCount <= 0)
        return;

    for (int i = 0; i < leafCount; i++)
    {
        idDict args;
        args.Clear();
        args.SetVector("origin", GetPhysics()->GetOrigin());
        args.Set("classname", "moveable_aloeleaf");
        
        idEntity *leafEnt;
        gameLocal.SpawnEntityDef(args, &leafEnt);

        if (leafEnt)
        {
            idAngles randAngle = idAngles(gameLocal.random.RandomInt(-60, 60), gameLocal.random.RandomInt(0, 359), 0);
            float speed = gameLocal.random.RandomInt(32, 128);

            leafEnt->GetPhysics()->SetLinearVelocity(randAngle.ToForward() * speed);
            leafEnt->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.RandomInt(-30, 30), gameLocal.random.RandomInt(-30, 30), 0));

			//dont make drop sound.
			if (leafEnt->IsType(idItem::Type))
			{
				static_cast<idItem *>(leafEnt)->SetJustDropped(true);
			}
        }
    }	
}