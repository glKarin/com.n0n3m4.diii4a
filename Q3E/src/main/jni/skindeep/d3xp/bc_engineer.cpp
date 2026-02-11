#include "Fx.h"
#include "WorldSpawn.h"
#include "gamesys/SysCvar.h"
#include "Player.h"
#include "framework/DeclEntityDef.h"

#include "bc_vomanager.h"
#include "bc_airlock.h"
#include "bc_searchnode.h"
#include "bc_ventdoor.h"
#include "bc_interestpoint.h"
#include "bc_idletask.h"
#include "bc_meta.h"
#include "bc_engineer.h"

#define MAX_MINES 3

#define CHECKTIME_MIN 5000
#define CHECKTIME_MAX 15000

#define PLACEMENT_PROXIMITY_XY_MIN  384
#define PLACEMENT_PROXIMITY_Z_MIN  64

//#define RADARDISH_PING_INTERVALTIME 5000

CLASS_DECLARATION(idGunnerMonster, idEngineerMonster)
END_CLASS

void idEngineerMonster::Spawn(void)
{
	placedMines = 0;
	minecheckTimer = gameLocal.time + gameLocal.random.RandomInt(CHECKTIME_MIN, CHECKTIME_MAX);
	mineplaceStatus = MINEPLACE_COOLDOWN;
}

void idEngineerMonster::Save(idSaveGame* savefile) const
{
	savefile->WriteInt( mineplaceStatus ); //  int mineplaceStatus
	savefile->WriteInt( placedMines ); //  int placedMines
	savefile->WriteInt( minecheckTimer ); //  int minecheckTimer
}
void idEngineerMonster::Restore(idRestoreGame* savefile)
{
	savefile->ReadInt( mineplaceStatus ); //  int mineplaceStatus
	savefile->ReadInt( placedMines ); //  int placedMines
	savefile->ReadInt( minecheckTimer ); //  int minecheckTimer
}

void idEngineerMonster::State_Idle()
{
	idGunnerMonster::State_Idle();

    //nextRadardishPingTime = gameLocal.time + RADARDISH_PING_INTERVALTIME;
}

void idEngineerMonster::State_Combat()
{
    idGunnerMonster::State_Combat();
}

void idEngineerMonster::Think()
{
    idGunnerMonster::Think();

	//Update landmine placement logic.
	if ((static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_COMBAT
		|| static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_SEARCH)
		&& CanAcceptStimulus() )
	{
		UpdateLandmineCheck();
	}


    //if (gameLocal.time > nextRadardishPingTime && static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->combatMetastate == COMBATSTATE_COMBAT)
    //{
    //    nextRadardishPingTime = gameLocal.time + RADARDISH_PING_INTERVALTIME;
	//
    //    //ping the player's location.
    //    bool isInSameRoom = false;
    //    idLocationEntity *entLocation = NULL;
    //    entLocation = gameLocal.LocationForEntity(this);
	//
    //    idLocationEntity *playerLocation = NULL;
    //    playerLocation = gameLocal.LocationForEntity(gameLocal.GetLocalPlayer());
	//
    //    if (entLocation != NULL && playerLocation != NULL)
    //    {
    //        if (entLocation->entityNumber == playerLocation->entityNumber)
    //        {
    //            isInSameRoom = true;
    //        }
    //    }
	//
    //    if (isInSameRoom)
    //    {
    //        gameLocal.GetLocalPlayer()->SetCenterMessage("your position was revealed by radar enemy");
    //        StartSound("snd_radar", SND_CHANNEL_ANY);
	//
    //        static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetLKPPositionByEntity(gameLocal.GetLocalPlayer());
    //        static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->UpdateMetaLKP(false);
    //        static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->AlertAIFriends(NULL); //Summon baddies. Go to combat state.
    //    }
    //}
}

void idEngineerMonster::UpdateLandmineCheck()
{
	if (placedMines >= MAX_MINES)
		return;

	if (mineplaceStatus == MINEPLACE_COOLDOWN)
	{
		if (minecheckTimer > gameLocal.time)
			return;

		mineplaceStatus = MINEPLACE_QUERYING;
	}
	else if (mineplaceStatus == MINEPLACE_QUERYING)
	{
		if (minecheckTimer > gameLocal.time)
			return;

		minecheckTimer = gameLocal.time + 500;

		//Attempt to place a mine.

		//See if there's any nearby mines.
		if (!DoMineClearanceCheck(GetPhysics()->GetOrigin()))
			return; //Found a nearby mine. Exit.

		//Find place to plant mine.
		//Start with forward trace.
		idVec3 forwardDir = viewAxis.ToAngles().ToForward();
		trace_t forwardTr;
		idVec3 traceStartPos = GetPhysics()->GetOrigin() + idVec3(0, 0, 8);
		gameLocal.clip.TracePoint(forwardTr, traceStartPos, traceStartPos + forwardDir * 32, MASK_SOLID, this);
		if (forwardTr.fraction < 1)
			return; //No clearance in front of me. Exit here.

		trace_t downTr;
		gameLocal.clip.TracePoint(downTr, forwardTr.endpos, forwardTr.endpos + idVec3(0, 0, -9), MASK_SOLID, NULL);
		if (downTr.fraction >= 1 || downTr.fraction < .5f)
			return; //No floor found. Exit here.

		if (downTr.c.normal != idVec3(0, 0, 1))
			return; //Not a flat floor. Ignore it.

		if (!gameLocal.entities[downTr.c.entityNum]->IsType(idWorldspawn::Type))
			return; //Not worldspawn. Exit.


		trace_t boundTr;
		gameLocal.clip.TraceBounds(boundTr, downTr.endpos + idVec3(0, 0, 1), downTr.endpos + idVec3(0, 0, 1), idBounds(idVec3(-9, -9, 0), idVec3(9, 9, 4)), MASK_SHOT_RENDERMODEL, NULL);
		if (boundTr.fraction < 1)
			return; //Something is blocking the bounding box. Exit here.


		//Successfully place a mine.
		placedMines++;		

		idEntity *newEnt;
		idDict args;
		args.Set("classname", spawnArgs.GetString("def_landmine"));
		args.SetVector("origin", downTr.endpos);
		gameLocal.SpawnEntityDef(args, &newEnt);

		if (!newEnt)
		{
			gameLocal.Warning("'%s' failed to spawn landmine.\n", GetName());
		}

		minecheckTimer = gameLocal.time + gameLocal.random.RandomInt(CHECKTIME_MIN, CHECKTIME_MAX); //how often to do mine placement check.
		mineplaceStatus = MINEPLACE_COOLDOWN;
	}
}

//return TRUE if place is CLEAR AND GOOD. Return FALSE if place is no good.
bool idEngineerMonster::DoMineClearanceCheck(idVec3 _position)
{
	for (idEntity* mineEnt = gameLocal.landmineEntities.Next(); mineEnt != NULL; mineEnt = mineEnt->landmineNode.Next())
	{
		if (!mineEnt)
			continue;

		//We want a fairly sizeable XY distance between mines, but we don't care so much about the Z distance. We do this to allow the AI to better handle rooms/maps with multiple floor heights in them.

		float xy_distance = (mineEnt->GetPhysics()->GetOrigin() - idVec3(_position.x, _position.y, mineEnt->GetPhysics()->GetOrigin().z)).LengthFast();
		if (xy_distance > PLACEMENT_PROXIMITY_XY_MIN)
			continue;

		float z_distance = fabs(mineEnt->GetPhysics()->GetOrigin().z - _position.z);
		if (z_distance > PLACEMENT_PROXIMITY_Z_MIN)
			continue;

		return false;
	}

	return true;
}

void idEngineerMonster::Resurrect()
{
	idGunnerMonster::Resurrect();

	if (placedMines >= MAX_MINES)
		placedMines = 0;
}
