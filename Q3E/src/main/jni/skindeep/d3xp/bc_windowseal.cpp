#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "idlib/LangDict.h"
#include "SmokeParticles.h"
#include "Game_local.h"
#include "Trigger.h"
#include "Misc.h"
#include "BrittleFracture.h"

#include "bc_lever.h"
#include "bc_windowseal.h"

const idEventDef EV_GravityCheck("<gravitycheck>", NULL);
const idEventDef EV_isSealed("isSealed", NULL, 'd');
const idEventDef EV_setSeal("setSeal", NULL, 'd');
const idEventDef EV_isWaitingForSeal("isWaitingForSeal", NULL, 'd');

CLASS_DECLARATION(idStaticEntity, idWindowseal)
	EVENT(EV_Activate,			idWindowseal::Event_Activate)
	EVENT(EV_PostSpawn,			idWindowseal::Event_PostSpawn)
	EVENT(EV_Touch,				idWindowseal::Event_Touch)
	EVENT(EV_GravityCheck,		idWindowseal::DoGravityCheck)
	EVENT(EV_isSealed,			idWindowseal::Event_isSealed)
	EVENT(EV_setSeal,			idWindowseal::Event_setSeal)
	EVENT(EV_isWaitingForSeal,	idWindowseal::Event_isWaitingForSeal)
END_CLASS

#define DISSOLVETIME 800 //how long it takes for windowseal to appear

#define PLAYERLERPTIME 900 //how long the lerp is to lerp player to interior

#define PUSH_FORCE 256
#define PUSH_FREQUENCY 200

#define LEVER_FROBINDEX 7

#define BREACHANNOUNCE_INTERVAL 5000

#define RESEALTIME  60000

idWindowseal::idWindowseal()
{
	breachAnnounceTimer = 0;
	leverEnt = NULL;
	autoResealTimer = 0;

	repairNode.SetOwner(this);
	repairNode.AddToEnd(gameLocal.repairEntities);
}

idWindowseal::~idWindowseal(void)
{
	repairNode.Remove();
}

void idWindowseal::Spawn(void)
{
	this->fl.takedamage = false;

	this->Hide();

	timer = 0;
	state = WINDOWSEAL_NONE;
	needsRepair = false;

	renderEntity.noShadow = true;
	pushTimer = 0;
	pushDir = vec3_zero;

	gameLocal.DoOriginContainmentCheck(this);

	autoReseal = spawnArgs.GetBool("autoseal", "0");

	PostEventMS(&EV_PostSpawn, 0);
}

void idWindowseal::Event_PostSpawn(void)
{
	bool hasVacuum = false;
	bool hasWindow = false;

	if (targets.Num() <= 0)
	{
		gameLocal.Error("window seal '%s' needs a target.", GetName());
		return;
	}
	
	for (int i = 0; i < targets.Num(); i++)
	{
		if (!targets[i].IsValid())
			continue;
		
		if (targets[i].GetEntity()->IsType(idVacuumSeparatorEntity::Type))
		{
			pushDir = targets[i].GetEntity()->GetPhysics()->GetAxis().ToAngles().ToForward();
			hasVacuum = true;
		}

		if (targets[i].GetEntity()->IsType(idBrittleFracture::Type))
		{
			hasWindow = true;
		}

		//Manually-placed windowseal lever.
		if (targets[i].GetEntity()->IsType(idLever::Type))
		{
			//if designer manually places a lever, instead of using the auto lever placement.
			leverEnt = targets[i].GetEntity();
			leverEnt->spawnArgs.SetInt("frobindex", LEVER_FROBINDEX);
			leverEnt->GetPhysics()->GetClipModel()->SetOwner(this);
			leverEnt->GetPhysics()->SetContents(0); // SW 24th April 2025: Don't let this lever be targeted by bash frob trace until it is visible
			leverEnt->Hide();
		}
	}

	if (!hasVacuum )
	{
		gameLocal.Error("window seal '%s' isn't targeting a vacuumSeparator entity.", GetName());
	}

	if ( !hasWindow)
	{
		gameLocal.Error("window seal '%s' isn't targeting a func_fracture entity.", GetName());
	}
}

void idWindowseal::Save(idSaveGame *savefile) const
{
	savefile->WriteString( fxActivate ); // idString fxActivate

	savefile->WriteInt( timer ); // int timer

	savefile->WriteInt( autoResealTimer ); // int autoResealTimer
	savefile->WriteBool( autoReseal ); // bool autoReseal

	savefile->WriteInt( state ); // int state
	savefile->WriteVec3( pushDir ); // idVec3 pushDir
	savefile->WriteInt( pushTimer ); // int pushTimer

	savefile->WriteObject( leverEnt ); // idEntity * leverEnt

	savefile->WriteInt( breachAnnounceTimer ); // int breachAnnounceTimer
}

void idWindowseal::Restore(idRestoreGame *savefile)
{
	savefile->ReadString( fxActivate ); // idString fxActivate

	savefile->ReadInt( timer ); // int timer

	savefile->ReadInt( autoResealTimer ); // int autoResealTimer
	savefile->ReadBool( autoReseal ); // bool autoReseal

	savefile->ReadInt( state ); // int state
	savefile->ReadVec3( pushDir ); // idVec3 pushDir
	savefile->ReadInt( pushTimer ); // int pushTimer

	savefile->ReadObject( leverEnt ); // idEntity * leverEnt

	savefile->ReadInt( breachAnnounceTimer ); // int breachAnnounceTimer
}

//This gets triggered when the window is shattered. Seal starts broadcasting "Hey I need to be repaired"
void idWindowseal::Event_Activate()
{
	//BC Old system that uses the repair bot.
	//renderEntity.shaderParms[7] = 1;
	//UpdateVisuals();	
	//
	//state = WINDOWSEAL_WAITINGFORREPAIR;
	//timer = gameLocal.time;	
	//
	//this->health = 0;
	//needsRepair = true;
	//repairrequestTimestamp = gameLocal.time;

	//renderEntity.shaderParms[7] = 1;
	//UpdateVisuals();

	
	//BC Window suction delay system.
	//state = WINDOWSEAL_SUCTIONDELAY;
	//timer = gameLocal.time + SUCTION_TIME;
	//
	//this->health = 0;
	//BecomeActive(TH_THINK);


	if (state == WINDOWSEAL_LEVERACTIVE)
		return;

	state = WINDOWSEAL_LEVERACTIVE;


	if (leverEnt == NULL)
	{
		//Lever ent doesn't exist. Find where to put it, and spawn it in.
		//Find where to position the lever, and at what angle.
		idAngles leverAng = idAngles(0, 0, 0);
		idBounds sealBounds = this->GetPhysics()->GetAbsBounds();
		idVec3 sealCenter = this->GetPhysics()->GetAbsBounds().GetCenter();
		idVec3 leverPos = vec3_zero;
		int leverSpawnDir = spawnArgs.GetInt("leverangle");
		if (leverSpawnDir == -1)
		{
			//Find position at the TOP.
			leverPos = idVec3(sealCenter.x, sealCenter.y, sealBounds[1].z);

			//make lever orient longwise to the windowseal bounding box.
			if ((sealBounds[1].x - sealBounds[0].x) > (sealBounds[1].y - sealBounds[0].y))
			{
				//rotate 90 degrees.
				leverAng.yaw += 90;
			}
		}
		else if (leverSpawnDir == -2)
		{
			//Find position at the BOTTOM.
			leverPos = idVec3(sealCenter.x, sealCenter.y, sealBounds[0].z);

			leverAng.pitch += 180; //flip it upside down.
			if ((sealBounds[1].x - sealBounds[0].x) > (sealBounds[1].y - sealBounds[1].y))
			{
				//rotate 90 degrees.
				leverAng.yaw += 90;
			}
		}

		//Spawn the lever.
		idDict args;
		args.Set("classname", "env_lever_windowseal");
		args.SetInt("frobindex", LEVER_FROBINDEX);
		args.SetVector("origin", leverPos);
		args.Set("snd_alarm", "vacuum_alarm");
		gameLocal.SpawnEntityDef(args, &leverEnt);
		leverEnt->GetPhysics()->GetClipModel()->SetOwner(this);
		leverEnt->GetPhysics()->SetAxis(leverAng.ToMat3());
		leverEnt->UpdateVisuals();
	}
	else
	{
		//Lever was manually placed by the designer. Unhide it.
		leverEnt->Show();
		leverEnt->GetPhysics()->SetContents(CONTENTS_RENDERMODEL); // SW 24th April 2025
	}

	if (leverEnt != NULL)
	{
		leverEnt->StartSound("snd_alarm", SND_CHANNEL_BODY);
	}

	
	//spawn light.
	idVec3 upDir;
	GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &upDir);
	idDict lightArgs;
	lightArgs.Clear();
	lightArgs.SetVector("origin", leverEnt->GetPhysics()->GetOrigin() + (upDir * -18));
	lightArgs.Set("texture", "lights/defaultlight_blink");
	lightArgs.SetInt("noshadows", 1);
	lightArgs.Set("_color", ".4 0 0 1");
	lightArgs.SetFloat("light", 128);
	idLight *sirenLight = (idLight *)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
	sirenLight->Bind(leverEnt, false);
	
	autoResealTimer = gameLocal.time + RESEALTIME;

	BecomeActive(TH_THINK);
}



void idWindowseal::Think()
{
	if (state == WINDOWSEAL_LEVERACTIVE)
	{
		if (leverEnt != NULL)
		{
			if (!leverEnt->IsHidden() && gameLocal.time > breachAnnounceTimer)
			{
				breachAnnounceTimer = gameLocal.time + BREACHANNOUNCE_INTERVAL;
				leverEnt->StartSound("snd_vacuumbreach", SND_CHANNEL_VOICE);
			}

			if (gameLocal.time >= autoResealTimer && autoReseal)
			{
				DoFrob(LEVER_FROBINDEX, nullptr);
			}
		}
	}
	else if (state == WINDOWSEAL_MATERIALIZING)
	{
		float lerp = (gameLocal.time - timer) / (float)DISSOLVETIME;
		lerp = 1.0f - lerp;

		if (lerp > 1.0f)
			lerp = 1.0f;
		if (lerp < 0)
			lerp = 0;

		renderEntity.shaderParms[7] = lerp;
		UpdateVisuals();

		if (lerp <= 0)
		{
			//Turn solid.
			GetPhysics()->SetContents(CONTENTS_SOLID);
			GetPhysics()->SetClipMask(MASK_SOLID);
			
			BecomeInactive(TH_THINK);
			state = WINDOWSEAL_DONE;
			PostEventMS(&EV_GravityCheck, 100); //Wait a short bit and then update the world gravity.

			renderEntity.noShadow = false;
			renderEntity.shaderParms[7] = 0; //disable dissolve effect.
			UpdateVisuals();

			//Turn ON the vacuumseparator.
			for (int i = 0; i < targets.Num(); i++)
			{
				if (!targets[i].IsValid())
					continue;

				if (targets[i].GetEntity()->IsType(idVacuumSeparatorEntity::Type))
				{
					static_cast<idVacuumSeparatorEntity *>(targets[i].GetEntity())->SetBlocked(true, true);
				}

				if (targets[i].GetEntity()->IsType(idBrittleFracture::Type))
				{
					targets[i].GetEntity()->PostEventMS(&EV_Remove, 0);	//Remove the window entity.				
				}
			}


			//remove the lever, if it exists.
			if (leverEnt != NULL)
			{
				leverEnt->PostEventMS(&EV_Remove, 0);
				leverEnt = nullptr;
			}


			BecomeInactive(TH_THINK);

			//TODO: one final check to see if anything is trapped inside the brush.
		}
	}
	//else if (state == WINDOWSEAL_SUCTIONDELAY)
	//{
	//	//New system that bypasses the repair bot.
	//	if (gameLocal.time > timer)
	//	{
	//		timer = gameLocal.time;
	//		renderEntity.shaderParms[7] = 0;
	//		UpdateVisuals();
	//		state = WINDOWSEAL_MATERIALIZING;
	//		this->Show();
	//
	//		GetPhysics()->SetContents(CONTENTS_TRIGGER);
	//
	//		//Repair is done!
	//		health = maxHealth;			
	//	}
	//}

	idStaticEntity::Think();
}

void idWindowseal::DoGravityCheck()
{
	gameLocal.DoGravityCheck();
}

void idWindowseal::DoRepairTick(int amount)
{
	timer = gameLocal.time;
	renderEntity.shaderParms[7] = 0;
	UpdateVisuals();
	state = WINDOWSEAL_MATERIALIZING;
	this->Show();

	GetPhysics()->SetContents(CONTENTS_TRIGGER);

	//Repair is done!
	health = maxHealth;
	needsRepair = false;
	BecomeActive(TH_THINK);

	StartSound("snd_seal", SND_CHANNEL_BODY);
}

void idWindowseal::Event_Touch(idEntity* other, trace_t* trace)
{
	//Push entities out of the way when windowseal is materializing.
	if (gameLocal.time > pushTimer && state != WINDOWSEAL_DONE)
	{
		idVec3 dir = pushDir * -PUSH_FORCE;
		other->GetPhysics()->SetLinearVelocity(other->GetPhysics()->GetLinearVelocity() + dir);

		pushTimer = gameLocal.time + PUSH_FREQUENCY;
	}
}

bool idWindowseal::DoFrob(int index, idEntity * frobber)
{
	if (index == LEVER_FROBINDEX && state == WINDOWSEAL_LEVERACTIVE)
	{
		//emergency lever has been pulled. Repair the window.
		DoRepairTick(0);

		if (leverEnt != NULL)
		{
			leverEnt->StopSound(SND_CHANNEL_ANY);
		}

		// SW: Added support for window seal to fire off scripts for tutorialisation purposes
		idStr scriptName = spawnArgs.GetString("call", "");
		if (scriptName.Length() > 0)
		{
			gameLocal.RunMapScript(scriptName.c_str());

			//const function_t* scriptFunction;
			//scriptFunction = gameLocal.program.FindFunction(scriptName);
			//if (scriptFunction)
			//{
			//	idThread* thread;
			//	thread = new idThread(scriptFunction);
			//	thread->DelayedStart(0);
			//}
		}


		if (frobber != nullptr)
		{
			if (frobber == gameLocal.GetLocalPlayer())
			{
				//Check if player is in outer space.
				if (gameLocal.GetLocalPlayer()->isInOuterSpace())
				{
					//Player is in outer space. Lerp player inside.

					idVec3 interiorLerpPosition = FindInteriorLerpPosition();
					if (interiorLerpPosition != vec3_zero)
					{
						gameLocal.GetLocalPlayer()->StartMovelerp(interiorLerpPosition, PLAYERLERPTIME);
					}
				}
			}
		}



		return true;
	}

	return false;
}

//Find a clear space for the player to lerp to when pulling the windowlever.
idVec3 idWindowseal::FindInteriorLerpPosition()
{
	if (leverEnt == nullptr)
	{
		common->Warning("Windowseal '%s' unable to find lever entity.\n", GetName());
		return vec3_zero;
	}

	idVec3 forward, right, up;
	leverEnt->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	#define INTARRAYSIZE 12
	idVec3 randomArray[] =
	{
		idVec3(-32,0,-32),
		idVec3(-32,32,-32),
		idVec3(-32,-32,-32),

		idVec3(-32,0,-64),
		idVec3(-32,32,-64),
		idVec3(-32,-32,-64),

		idVec3(-48,0,-32),
		idVec3(-48,32,-32),
		idVec3(-48,-32,-32),

		idVec3(-48,0,-64),
		idVec3(-48,32,-64),
		idVec3(-48,-32,-64),
	};

	//Determine whether forward = interior, or if forward = exterior.
	float forwardMultiplier = 1.0f;
	idVec3 forwardSpot = leverEnt->GetPhysics()->GetOrigin() + (forward * 32) + (up * -24);
	idVec3 reverseSpot = leverEnt->GetPhysics()->GetOrigin() + (forward * -32) + (up * -24);
	idLocationEntity* forwardLoc = gameLocal.LocationForPoint(forwardSpot);
	idLocationEntity* reverseLoc = gameLocal.LocationForPoint(reverseSpot);
	if (forwardLoc != nullptr && reverseLoc != nullptr)
	{
		if (idStr::Cmp(forwardLoc->GetLocation(), common->GetLanguageDict()->GetString("#str_00000"))
			&&
			!idStr::Cmp(reverseLoc->GetLocation(), common->GetLanguageDict()->GetString("#str_00000"))
			)
		{
			//forward is space. THerefore, reverse the forward check direction.
			forwardMultiplier = -1.0f;
		}
		else if (!idStr::Cmp(forwardLoc->GetLocation(), common->GetLanguageDict()->GetString("#str_00000"))
			&&
			idStr::Cmp(reverseLoc->GetLocation(), common->GetLanguageDict()->GetString("#str_00000"))
			)
		{
			//forward is interior. Therefore, keep the forward check value.
			forwardMultiplier = 1.0f;
		}
		else
		{
			common->Warning("Windowseal '%s': location check cannot find outer space.\n", GetName());
			return vec3_zero;
		}
	}
	else
	{
		common->Warning("Windowseal '%s': failed location check for forwardMultiplier.\n", GetName());
		return vec3_zero;
	}



	idBounds playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
	playerbounds.ExpandSelf(1); //expand a little for safety sake.

	for (int i = 0; i < INTARRAYSIZE; i++)
	{
		idVec3 candidatePos = leverEnt->GetPhysics()->GetOrigin() + (forward * (randomArray[i].x * forwardMultiplier)) + (right * randomArray[i].y) + (up * randomArray[i].z);

		int penetrationContents = gameLocal.clip.Contents(candidatePos, NULL, mat3_identity, CONTENTS_SOLID, NULL);
		if (penetrationContents & MASK_SOLID)
		{
			continue; //If it starts in solid, then exit.
		}

		//gameRenderWorld->DebugBounds(colorGreen, playerbounds, candidatePos, 90000);

		trace_t boundTr;
		gameLocal.clip.TraceBounds(boundTr, candidatePos, candidatePos, playerbounds, MASK_SOLID, NULL);
		if (boundTr.fraction < 1)
			continue;

		//We now have a reasonable position to land.
		//gameRenderWorld->DebugArrowSimple(candidatePos);
		return candidatePos;
	}	

	common->Warning("Windowseal '%s': couldn't find valid lerp position.\n", GetName());
	return vec3_zero;
}


//Post FTL, do the window repair.
void idWindowseal::PostFTLReset()
{
	if (state != WINDOWSEAL_LEVERACTIVE)
		return;
	
	DoFrob(LEVER_FROBINDEX, NULL);
}

void idWindowseal::Event_isSealed()
{
	if (state == WINDOWSEAL_MATERIALIZING || state == WINDOWSEAL_DONE)
	{
		idThread::ReturnInt(1);
		return;
	}
	
	idThread::ReturnInt(0);
}

void idWindowseal::Event_setSeal()
{
	//Sela up the window.

	if (state == WINDOWSEAL_MATERIALIZING || state == WINDOWSEAL_DONE)
	{
		idThread::ReturnInt(0);
		return;
	}

	if (leverEnt != NULL)
	{
		leverEnt->StopSound(SND_CHANNEL_ANY);
	}

	DoRepairTick(1);

	idThread::ReturnInt(1);
}

void idWindowseal::Event_isWaitingForSeal()
{
	if (state == WINDOWSEAL_LEVERACTIVE)
	{
		idThread::ReturnInt(1);
		return;
	}

	idThread::ReturnInt(0);
}
