#include "framework/DeclEntityDef.h"

#include "WorldSpawn.h"
#include "Player.h"
#include "Fx.h"
#include "bc_ftl.h"
#include "bc_skullsaver.h"
#include "bc_meta.h"
#include "bc_cat.h"
#include "bc_catcage.h"
#include "bc_lifeboat_catpod.h"

#define DELAY_BEFORE_DEPOSITTAKEOFF 1500
#define CATJUMP_GAPINTERVALTIME 400

#define DELAY_BEFORE_CAREPACKAGESPAWN 400

CLASS_DECLARATION(idLifeboat, idCatpod)
END_CLASS

idCatpod::idCatpod(void)
{
	spacenudgeNode.SetOwner(this);
	spacenudgeNode.AddToEnd(gameLocal.spacenudgeEntities);

	catpodState = CTP_IDLE;
	catpodTimer = 0;

	awaitingScriptCall = false;
	scriptCallTimer = 0;

	doFinalCatSequence = false;
	finalCatSequenceTimer = 0;
	finalCatSequenceActivated = false;
}

idCatpod::~idCatpod(void)
{
	spacenudgeNode.Remove();
}

void idCatpod::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( catpodState ); // int catpodState
	savefile->WriteInt( catpodTimer ); // int catpodTimer

	savefile->WriteInt( guiUpdateTimer ); // int guiUpdateTimer

	savefile->WriteBool( doFinalCatSequence ); // bool doFinalCatSequence
	savefile->WriteInt( finalCatSequenceTimer ); // int finalCatSequenceTimer
	savefile->WriteBool( finalCatSequenceActivated ); // bool finalCatSequenceActivated
	savefile->WriteBool( awaitingScriptCall ); // bool awaitingScriptCall
	savefile->WriteInt( scriptCallTimer ); // int scriptCallTimer
}

void idCatpod::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( catpodState ); // int catpodState
	savefile->ReadInt( catpodTimer ); // int catpodTimer

	savefile->ReadInt( guiUpdateTimer ); // int guiUpdateTimer

	savefile->ReadBool( doFinalCatSequence ); // bool doFinalCatSequence
	savefile->ReadInt( finalCatSequenceTimer ); // int finalCatSequenceTimer
	savefile->ReadBool( finalCatSequenceActivated ); // bool finalCatSequenceActivated
	savefile->ReadBool( awaitingScriptCall ); // bool awaitingScriptCall
	savefile->ReadInt( scriptCallTimer ); // int scriptCallTimer
}

void idCatpod::Spawn(void)
{
	guiUpdateTimer = 0;
	Event_GuiNamedEvent(1, "resetGui");


	int totalCatcages = static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetTotalCatCages();
	Event_SetGuiInt("cats_total", totalCatcages);
}


bool idCatpod::DoFrob(int index, idEntity * frobber)
{
	if (catpodState != CTP_IDLE)
		return true;

	//Only player can frob the pod...
	if (frobber == NULL || frobber != gameLocal.GetLocalPlayer())
		return true;

	idStr scriptCall = gameLocal.world->spawnArgs.GetString("call_catpodfrob");
	if (scriptCall.Length() > 0)
	{
		awaitingScriptCall = true;
		int  catsDeposited  = DepositAvailableCats();		
		scriptCallTimer = gameLocal.time + (catsDeposited * CATJUMP_GAPINTERVALTIME) + 300;
		return true;
	}

	//Deposit the cats.		
	int awaitingDeposit = GetCatsAwaitingDeposit();
	if (awaitingDeposit <= 0)
	{
		if (IsAllCatsRescued() && !static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetPlayerHasEnteredCatpod())
		{
			//This is to handle the situation where the pod intakes all the cats, gets damaged and takes off, and then the player
			//has to call the pod back down to re-enter the pod.
			EnterCatpod();
		}		
		else
		{
			//No cats to deposit.......
			StartSound("snd_cancel", SND_CHANNEL_ANY);
			Event_GuiNamedEvent(1, "noCatError");
		}
	}
	else
	{
		//Deposit the cats.
		int catsDeposited = DepositAvailableCats();
		if (catsDeposited > 0)
		{
			isFrobbable = false;
			
			StartSound("snd_thanks", SND_CHANNEL_VOICE); //cat says "thanks nina"
			StartSound("snd_jingle", SND_CHANNEL_ANY);			

			if (GetCatsAwaitingRescue() <= 0 && catsDeposited > 0)
			{
				//The final cat has been deposited.
				//The player enters the pod.
				doFinalCatSequence = true;
				finalCatSequenceTimer = gameLocal.time + (catsDeposited * CATJUMP_GAPINTERVALTIME) + 500;
			}
			else if (state == LANDED)
			{
				stateTimer = gameLocal.time + DELAY_BEFORE_DEPOSITTAKEOFF; //this is not the final cat. So, just take off at a normal time.
			}			
		}
	}

	return true;
}



//Find where the cat will spawn from. It's always near the player's head. Find shortest route to the cat cubby hole.
idVec3 idCatpod::FindCatOriginPoint(idVec3 _destination)
{
	//right, up	
	#define HEADDISTANCE 48
	#define ORIGINARRAYSIZE 2
	idVec2 originArray[] =
	{
		idVec2(-HEADDISTANCE,	0),				//left of player head
		idVec2(HEADDISTANCE,	0),				//right of player head
	};

	idVec3 right, up;
	gameLocal.GetLocalPlayer()->viewAngles.ToVectors(NULL, &right, &up);

	float closestDist = 999999999;
	idVec3 closestPos = vec3_zero;
	for (int i = 0; i < ORIGINARRAYSIZE; i++)
	{
		idVec3 originpos = gameLocal.GetLocalPlayer()-> GetPhysics()->GetOrigin() + (right * originArray[i].x) + (up * originArray[i].y);

		float dist = (originpos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin).LengthSqr(); // we only care about relative distance and don't care about the actual distance, so we use lengthsqr
		if (dist < closestDist)
		{
			closestDist = dist;
			closestPos = originpos;
		}
	}

	return closestPos;
}

//Get the catcubby that's closest to the player's eyeball.
idVec3 idCatpod::GetBestCatCubby()
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	//forward, right, up
	#define CUBBYCOUNT 5
	idVec3 cubbyArray[] =
	{
		idVec3(-12.5f,	0,		-26),		//straight down
		idVec3(-12.5f,	24,		-8.5f),		//bottom half 1
		idVec3(-12.5f,	-24,	-8.5f),		//bottom half 2
		idVec3(-12.5f,	15.3f,	21),		//top half 1
		idVec3(-12.5f,	-15.3f,	21)			//top half 2
	};

	float closestDist = 999999999;
	idVec3 closestPos = vec3_zero;
	for (int i = 0; i < CUBBYCOUNT; i++)
	{
		idVec3 cubbypos = GetPhysics()->GetOrigin() + (forward * cubbyArray[i].x) + (right * cubbyArray[i].y) + (up * cubbyArray[i].z);

		float dist = (cubbypos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin).LengthSqr(); // we only care about relative distance and don't care about the actual distance, so we use lengthsqr
		if (dist < closestDist)
		{
			closestDist = dist;
			closestPos = cubbypos;
		}
	}

	return closestPos;
}

//We want the care package to come out of the SECOND CLOSEST cubby to the player. This is because it looks weird if the cubby that the cat enters is where the carepackage comes from.
idVec3 idCatpod::GetBestCarepackageSpot()
{
	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	//forward, right, up
	#define CUBBYCOUNT 5
	idVec3 cubbyArray[] =
	{
		idVec3(0,	0,		-26),		//straight down
		idVec3(0,	24,		-8.5f),		//bottom half 1
		idVec3(0,	-24,	-8.5f),		//bottom half 2
		idVec3(0,	15.3f,	21),		//top half 1
		idVec3(0,	-15.3f,	21)			//top half 2
	};

	idStaticList<vecSpot_t, CUBBYCOUNT> candidates;
	for (int i = 0; i < CUBBYCOUNT; i++)
	{
		idVec3 cubbypos = GetPhysics()->GetOrigin() + (forward * cubbyArray[i].x) + (right * cubbyArray[i].y) + (up * cubbyArray[i].z);

		float dist = (cubbypos - gameLocal.GetLocalPlayer()->firstPersonViewOrigin).LengthSqr(); // we only care about relative distance and don't care about the actual distance, so we use lengthsqr

		vecSpot_t newcandidate;
		newcandidate.position = cubbypos;
		newcandidate.distance = dist;
		candidates.Append(newcandidate);
	}

	qsort((void *)candidates.Ptr(), candidates.Num(), sizeof(spawnSpot_t), (int(*)(const void *, const void *))gameLocal.sortVecPoints_Farthest);
	return candidates[3].position;
}

void idCatpod::SpawnCarepackage()
{
	idVec3 cubbyPos = GetBestCarepackageSpot(); //Get the side of the pod closest to the player.
	idMat3 podForward = GetPhysics()->GetAxis();
	idVec3 podPos = GetPhysics()->GetOrigin();
	
	idEntity *packageEnt;
	idDict args;
	args.Set("classname", "moveable_carepackage");
	args.SetVector("cubbypos", cubbyPos);
	args.SetVector("podpos", podPos);
	args.SetMatrix("podforward", podForward);
	gameLocal.SpawnEntityDef(args, &packageEnt);
}

int idCatpod::DepositAvailableCats()
{
	//Figure out where the cat will spawn from, and where it will jump to.
	idVec3 catcubbyPos = GetBestCatCubby();
	idVec3 catspawnPos = FindCatOriginPoint(catcubbyPos);

	int depositCount = false;
	for (idEntity* catEnt = gameLocal.catfriendsEntities.Next(); catEnt != NULL; catEnt = catEnt->catfriendNode.Next())
	{
		if (!catEnt)
			continue;

		if (!catEnt->IsType(idCat::Type))
			continue;

		if (static_cast<idCat *>(catEnt)->IsAvailable())
		{
			//This is what does the cat animation (cat jump into pod)
			static_cast<idCat *>(catEnt)->PutInPod(catspawnPos, catcubbyPos, depositCount * CATJUMP_GAPINTERVALTIME);
			depositCount++;
		}
	}

	return depositCount;
}

//Called for one frame when the pod lands.
void idCatpod::OnLanded()
{
	if (!gameLocal.world->spawnArgs.GetBool("objectives", "1"))
		return;

	int catsAwaitingDeposit = GetCatsAwaitingDeposit();
	if (catsAwaitingDeposit > 0)
	{
		//There are cats available to place in the pod.
		gameLocal.GetLocalPlayer()->SetObjectiveText("#str_obj_placecatsinpod", false, "icon_obj_catpod"); //"Deposit crew into the Cat Evac Pod"
	}
}

// SW 24th March 2025: restoring this function for the purposes of objective control
// I think ideally I'd like all objective updating to be refactored and routed through one master function that decides what the objective should be
// but we know that's not happening at this stage, don't we?
void idCatpod::OnTakeoff()
{
	if (gameLocal.world->spawnArgs.GetBool("objectives", "1"))
	{
		if (GetCatsAwaitingRescue() > 0)
		{
			// There are still cats left to rescue (this can happen if the player summons the pod prematurely and only deposits some subset of the cats)
			static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->UpdateCagecageObjectiveText();
		}
		else if (GetCatsAwaitingDeposit() > 0)
		{
			// All cats have been rescued from the cages, but not all of them have been deposited in the pod
			gameLocal.GetLocalPlayer()->SetObjectiveText("#str_obj_summonpod", false, "icon_obj_signallamp"); //"Use a Signal Lamp to summon a Cat Evac pod"
		}
		else
		{
			// There are no cats left to rescue or deposit, meaning we are now in pirate reinforcements phase
			gameLocal.GetLocalPlayer()->SetObjectiveText("#str_obj_boardingship", false, "icon_obj_pirates");
		}
	}
}

void idCatpod::Think_Landed()
{
	if (!tractorbeam->IsHidden())
		tractorbeam->Hide(); //for some reason this needs to be called........

	if (gameLocal.time > guiUpdateTimer)
	{
		guiUpdateTimer = gameLocal.time + 1000;
		Event_SetGuiInt("cats_rescued", GetCatsRescued());
	}

	if (catpodState == CTP_PRECAREPACKAGE)
	{
		if (gameLocal.time > catpodTimer)
		{
			gameLocal.GetLocalPlayer()->SetImpactSlowmo(true);
			SpawnCarepackage();
			catpodState = CTP_CAREPACKAGESPAWNED;
			catpodTimer = gameLocal.time + 1000;
		}
	}
	else if (catpodState == CTP_CAREPACKAGESPAWNED)
	{
		if (gameLocal.time > catpodTimer)
		{
			catpodState = CTP_READYFORTAKEOFF;
			stateTimer = 0; //take off.
		}
	}
	
	if (awaitingScriptCall && gameLocal.time > scriptCallTimer)
	{
		awaitingScriptCall = false;
		idStr scriptCall = gameLocal.world->spawnArgs.GetString("call_catpodfrob");
		if (scriptCall.Length() > 0)
		{
			gameLocal.RunMapScript(scriptCall.c_str());
		}
	}

	if (doFinalCatSequence && gameLocal.time > finalCatSequenceTimer && !finalCatSequenceActivated)
	{
		EnterCatpod();
	}

	return;
}

void idCatpod::EnterCatpod()
{
	gameLocal.GetLocalPlayer()->SetObjectiveText(""); //clear out the objective text.

	finalCatSequenceActivated = true;
	catpodState = CTP_PLAYERINSIDE;
	state = CATPOD_EQUIPPING; //SO It doesn't take off.
	static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->SetPlayerEnterCatpod();
}

int idCatpod::GetCatsAwaitingDeposit()
{
	int amount = 0;
	for (idEntity* catEnt = gameLocal.catfriendsEntities.Next(); catEnt != NULL; catEnt = catEnt->catfriendNode.Next())
	{
		if (!catEnt)
			continue;

		if (!catEnt->IsType(idCat::Type))
			continue;

		if (static_cast<idCat *>(catEnt)->IsAvailable())
			amount++;
	}
	return amount;
}

int idCatpod::GetCatsRescued()
{
	int amount = 0;
	for (idEntity* catEnt = gameLocal.catfriendsEntities.Next(); catEnt != NULL; catEnt = catEnt->catfriendNode.Next())
	{
		if (!catEnt)
			continue;

		if (!catEnt->IsType(idCat::Type))
			continue;

		if (static_cast<idCat *>(catEnt)->IsInPod())
			amount++;
	}
	return amount;
}

bool idCatpod::IsAllCatsRescued()
{
	int amountRescued = 0;
	int totalAmount = 0;

	for (idEntity* catEnt = gameLocal.catfriendsEntities.Next(); catEnt != NULL; catEnt = catEnt->catfriendNode.Next())
	{
		if (!catEnt)
			continue;

		if (!catEnt->IsType(idCat::Type))
			continue;

		if (static_cast<idCat*>(catEnt)->IsInPod())
			amountRescued++;

		totalAmount++;
	}
	
	return (amountRescued >= totalAmount);
}

//
int idCatpod::GetCatsAwaitingRescue()
{
	int amount = 0;
	for (idEntity* catEnt = gameLocal.catfriendsEntities.Next(); catEnt != NULL; catEnt = catEnt->catfriendNode.Next())
	{
		if (!catEnt)
			continue;

		if (!catEnt->IsType(idCat::Type))
			continue;

		if (static_cast<idCat *>(catEnt)->IsCaged())
			amount++;
	}
	return amount;
}

