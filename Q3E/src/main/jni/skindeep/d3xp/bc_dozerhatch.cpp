#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
#include "framework/DeclEntityDef.h"

#include "bc_repairbot.h"
#include "bc_spearbot.h"
#include "bc_dozerhatch.h"

const int DOZERHATCH_SPAWNOFFSET  = 96;
const int DOZERRADIUS = 32;

const int OPENANIM_TIME = 2000;

const int COOLDOWNTIME = 4000; //time between bot spawns.


CLASS_DECLARATION(idAnimated, idDozerhatch)
END_CLASS

idDozerhatch::idDozerhatch(void)
{
	hatchNode.SetOwner(this);
	hatchNode.AddToEnd(gameLocal.hatchEntities);

	
}

idDozerhatch::~idDozerhatch(void)
{
	hatchNode.Remove();
}

void idDozerhatch::Spawn(void)
{
	stateTimer = 0;
	hatchState = HATCH_IDLE;
	team = TEAM_ENEMY;
	BecomeActive(TH_THINK);
}



void idDozerhatch::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( stateTimer ); //  int stateTimer
	savefile->WriteInt( hatchState ); //  int hatchState
}

void idDozerhatch::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( stateTimer ); //  int stateTimer
	savefile->ReadInt( hatchState ); //  int hatchState
}

//botType:  0 = "def_spawndef", 1 = "def_securitybot"
void idDozerhatch::StartSpawnSequence(idEntity * repairableEnt, int botType)
{
	idVec3 up, particlePos, right;
	idAngles particleDir;
	idEntity *bot;

	if (hatchState == HATCH_COOLDOWN)
		return;

	

	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_blink", "skins/objects/dozerhatch/skin_blink"))); //Blinky skin.

	this->GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, &right, &up);
	particleDir = GetPhysics()->GetAxis().ToAngles();
	particleDir.pitch += 180;

	particlePos = GetPhysics()->GetOrigin() + (up * -32) + (right * 48);
	idEntityFx::StartFx("fx/smokepuff02_quick", particlePos, particleDir.ToMat3());

	particlePos = GetPhysics()->GetOrigin() + (up * -32) + (right * -48);
	idEntityFx::StartFx("fx/smokepuff02_quick", particlePos, particleDir.ToMat3());


	Event_PlayAnim("open", 1, false); //Play open animation.
	hatchState = HATCH_OPENING;
	stateTimer = gameLocal.time + OPENANIM_TIME;
	StartSound("snd_open", SND_CHANNEL_BODY3, 0, false, NULL);

	bot = SpawnBot(botType);

	if (bot)
	{
		if (bot->IsType(idAI_Repairbot::Type))
		{
			//static_cast<idAI_Repairbot*>(bot)->SetRepairTask(repairableEnt);
			static_cast<idAI_Repairbot*>(bot)->queuedRepairEnt = repairableEnt;
		}
		else if (bot->IsType(idAI_Spearbot::Type))
		{
			if (team == TEAM_FRIENDLY)
			{
				//if this is a friendly hatch,then spawn a friendly spear.
				bot->DoHack();
			}
		}
	}

	BecomeActive(TH_THINK);
}

idEntity * idDozerhatch::SpawnBot(int botType)
{
	idVec3 up, forward, right;
	trace_t mechTr;
	idBounds botBounds;
	idVec3 spawnPos;
	bool foundSpawnPos = false;

	StartSound("snd_spawn", SND_CHANNEL_BODY, 0, false, NULL);

	//Just check right in front of the hatch doors.
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	spawnPos = GetPhysics()->GetOrigin() + up * -DOZERHATCH_SPAWNOFFSET;
	botBounds = idBounds(idVec3(-DOZERRADIUS, -DOZERRADIUS, -DOZERRADIUS), idVec3(DOZERRADIUS, DOZERRADIUS, DOZERRADIUS));

	//do a check and see if we have space available at the spawn location.
	gameLocal.clip.TraceBounds(mechTr, spawnPos, spawnPos, botBounds, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE), this);	

	if (mechTr.fraction >= 1)
	{
		//gameRenderWorld->DebugBounds(colorCyan, botBounds, spawnPos, 10000);
		foundSpawnPos = true;
	}
	else
	{
		int i;

		#define	SPAWNOFFSET 64		

		//Ok, failed to find a clear spot in front of the door. Create an array of several nearby spots and see if they're clear.
		idVec3 candidateSpots[] =
		{
			spawnPos + (forward * -SPAWNOFFSET),
			spawnPos + (forward * -SPAWNOFFSET) + (right * SPAWNOFFSET),
			spawnPos + (forward * -SPAWNOFFSET) + (right * -SPAWNOFFSET),

			spawnPos + (forward * SPAWNOFFSET),
			spawnPos + (forward * SPAWNOFFSET) + (right * SPAWNOFFSET),
			spawnPos + (forward * SPAWNOFFSET) + (right * -SPAWNOFFSET),

			spawnPos + (right * SPAWNOFFSET),
			spawnPos + (right * -SPAWNOFFSET)
		};

		for (i = 0; i < 8; i++)
		{
			gameLocal.clip.TraceBounds(mechTr, candidateSpots[i], candidateSpots[i], botBounds, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE), this);

			if (mechTr.fraction >= 1)
			{
				//gameRenderWorld->DebugBounds(colorYellow, botBounds, candidateSpots[i], 10000);
				spawnPos = candidateSpots[i];
				foundSpawnPos = true;
				break;
			}
		}
	}

	if (foundSpawnPos)
	{
		const idDeclEntityDef *botDef;
		idEntity *botEnt;

		//Attempt to spawn bot.		
		idStr botclassname = GetBotDef(botType);
		botDef = gameLocal.FindEntityDef(botclassname.c_str(), false);
		if (!botDef)
		{
			gameLocal.Error("idDozerhatch %s failed to spawn '%s'\n", GetName(), botclassname.c_str());
		}
		gameLocal.SpawnEntityDef(botDef->dict, &botEnt, false);
		if (botEnt)
		{
			//Spawn the bot.
			botEnt->GetPhysics()->SetOrigin(spawnPos); //Where it appears.
			botEnt->UpdateVisuals();

			idEntityFx::StartFx("fx/smoke_ring09", &spawnPos, &mat3_identity, NULL, false); //Smoke particle.
			return botEnt;
		}
	}

	return NULL;
}

idStr idDozerhatch::GetBotDef(int botType)
{
	if (botType == BOT_REPAIRBOT)
		return spawnArgs.GetString("def_spawndef");
	if (botType == BOT_SECURITYBOT)
		return spawnArgs.GetString("def_securitybot");

	return "";
}


void idDozerhatch::Think(void)
{
	idAnimated::Think();

	if (hatchState == HATCH_OPENING)
	{
		//Wait for opening animation to finish.

		if (gameLocal.time > stateTimer)
		{			
			hatchState = HATCH_COOLDOWN;
			stateTimer = gameLocal.time + COOLDOWNTIME;

			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin", "skins/objects/dozerhatch/default")));
			Event_PlayAnim("close", 1, false);
			StartSound("snd_open", SND_CHANNEL_BODY, 0, false, NULL);
		}
	}
	else if (hatchState == HATCH_COOLDOWN)
	{
		if (gameLocal.time > stateTimer)
		{
			hatchState = HATCH_IDLE;
			BecomeInactive(TH_THINK);
		}
	}



}


bool idDozerhatch::IsCurrentlySpawning()
{
	if (hatchState == HATCH_OPENING || hatchState == HATCH_COOLDOWN)
		return true;

	return false;
}

void idDozerhatch::SetTeam(int value)
{
	team = value;

	//TODO : make the hatch visually change its appearance somehow
}