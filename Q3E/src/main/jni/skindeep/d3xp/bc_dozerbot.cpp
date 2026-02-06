//Custom code for the dozerbot.

#include "sys/platform.h"
#include "script/Script_Thread.h"
#include "gamesys/SysCvar.h"
#include "Mover.h"
#include "Fx.h"

#include "bc_trashairlock.h"
#include "ai/AI.h"

class idAI_Dozerbot : public idAI {
public:
	CLASS_PROTOTYPE(idAI_Dozerbot);

	virtual void			Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location);

private:	
	void	Event_ChooseCubeToPush( const idVec3 &mins, const idVec3 &maxs );
};

const idEventDef AI_Dozerbot_ChooseCubeToPush( "Dozerbot_ChooseCubeToPush", "vv", 'e' );

CLASS_DECLARATION( idAI, idAI_Dozerbot)
	EVENT(AI_Dozerbot_ChooseCubeToPush, idAI_Dozerbot::Event_ChooseCubeToPush)
END_CLASS


void idAI_Dozerbot::Event_ChooseCubeToPush( const idVec3 &mins, const idVec3 &maxs)
{
	idEntity	*entityList[MAX_GENTITIES];
	int			listedEntities, i;

	//detect if there's stuff waiting to be trashed...
	listedEntities = gameLocal.EntitiesWithinAbsBoundingbox(idBounds(mins, maxs), entityList, MAX_GENTITIES);

	int closestIndex = -1;
	int closestDistance = 4096;

	if (listedEntities > 0)
	{
		for (i = 0; i < listedEntities; i++)
		{
			idEntity *ent = entityList[i];

			if (!ent)
				continue;

			if (ent->IsType(idMover::Type) && ent->spawnArgs.GetInt("trashcube") > 0)
			{
				
				//Check if the cube's assigned airlock is ready to be used.
				if (ent->targets.Num() <= 0)
				{
					//trashcube has no airlock. Error out...
					gameLocal.Error("Event_ChooseCubeToPush: trash cube %s has no airlock assigned to it.", ent->GetName());
					return;
				}
				else
				{
					if (!ent->targets[0].GetEntity())
					{
						gameLocal.Error("Event_ChooseCubeToPush: failed to access target[0] of trashcube %s", ent->GetName());
						return;
					}
					
					if (!ent->targets[0].GetEntity()->IsType(idTrashAirlock::Type))
					{
						gameLocal.Error("Event_ChooseCubeToPush: trashcube %s target[0] is not an airlock.", ent->GetName());
						return;
					}

					//OK. We now have an airlock to examine.
					if (!static_cast<idTrashAirlock*>(ent->targets[0].GetEntity())->GetReadyStatus())
					{
						//airlock is not ready. don't consider this cube.
						continue;
					}
				}
				
				//Found candidate.
				float curDist = (GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin()).Length();

				if (curDist < closestDistance)
				{
					closestDistance = curDist;
					closestIndex = i;
				}
			}
		}

		if (closestIndex >= 0)
		{
			idThread::ReturnEntity(entityList[closestIndex]);
			return;
		}
	}

	idThread::ReturnEntity( NULL );
}

void idAI_Dozerbot::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
{
	idAI::Killed(inflictor, attacker, damage, dir, location);

	//If we're receiving crushing damage, then just make me immediately explode.
	if (inflictor->spawnArgs.GetBool("crusher", "0"))
	{
	
		idEntityFx::StartFx("fx/explosion_gascylinder", &GetPhysics()->GetOrigin(), &mat3_identity, NULL, false);

		//Blow up
		Hide();
		GetPhysics()->SetContents(0);
		PostEventMS(&EV_Remove, 100);
		gameLocal.RadiusDamage(GetPhysics()->GetOrigin(), this, this, this, this, "damage_explosion");
	}
}