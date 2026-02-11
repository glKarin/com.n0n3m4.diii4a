#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"
//#include "bc_ftl.h"

#include "bc_meta.h"
#include "bc_keypad.h"
#include "bc_maintpanel.h"


const int LIGHTRADIUS_BROKEN = 48;
const int LIGHTRADIUS_OPERATIONAL = 72;

//const int CLOSETIME = 4000; //how long close animation is
const int VO_DELAYTIME = 500; //delay before "all xx unlocked" vo plays.

#define CABLEJOINT "cables2"

CLASS_DECLARATION(idAnimatedEntity, idMaintPanel)
	EVENT(EV_PostSpawn, idMaintPanel::Event_PostSpawn)
	//EVENT(EV_Activate, idMaintPanel::Event_Activate)
END_CLASS

idMaintPanel::idMaintPanel(void)
{
	smokeParticle = NULL;
	soundParticle = NULL;

	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	videoSignage = NULL;
	videoSignageActive = false;
	videoSignageLifetimer = 0;
}

idMaintPanel::~idMaintPanel(void)
{
	StopSound(SND_CHANNEL_ANY, 0);

	if (smokeParticle != NULL) {
		smokeParticle->PostEventMS(&EV_Remove, 0);
		smokeParticle = nullptr;
	}

	if (soundParticle != NULL) {
		soundParticle->PostEventMS(&EV_Remove, 0);
		soundParticle = nullptr;
	}

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}

void idMaintPanel::Spawn(void)
{
	fl.takedamage = false;

	//Remove collision, but still allow frob/damage
	//GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	//GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	GetPhysics()->SetContents(0);

	idVec3 forward, up, right;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);

	idVec3 lightPos = GetPhysics()->GetOrigin() + (forward * 8.0f) + (up * -5.0f) + (right * 3);

	headlight.shader = declManager->FindMaterial("lights/pulse03", false);
	headlight.pointLight = true;
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHTRADIUS_BROKEN;
	headlight.shaderParms[0] = 1;
	headlight.shaderParms[1] = 0;
	headlight.shaderParms[2] = 0;
	headlight.shaderParms[3] = 1.0f;
	headlight.noShadows = true;
	headlight.isAmbient = false;
	headlight.axis = mat3_identity;
	headlightHandle = gameRenderWorld->AddLightDef(&headlight);
	headlight.origin = lightPos;
	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);


	SetColor(1, 0, 0);
	Event_PlayAnim("opened", 1);
	BecomeInactive(TH_THINK);

	//Smoke particle.
	idAngles particleAngle = GetPhysics()->GetAxis().ToAngles();
	particleAngle.pitch += 140;
	particleAngle.yaw += -45;

	idVec3 smokePos = GetPhysics()->GetOrigin() + (forward * 2) + (up * -5.5f); //BC 4-4-2025: fixed where smoke emits from
	idDict args;
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_smoke") );
	args.SetVector("origin", smokePos);
	args.SetMatrix("rotation", particleAngle.ToMat3());	

	//if no keypad, then don't do smoke.
	idStr callArg = spawnArgs.GetString("call");
	if (callArg.Length() > 0)
	{
		args.SetBool("hide", true);
	}

	smokeParticle = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));	

	state = MPL_JAMMED;
	stateTimer = 0;

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true);
	Event_SetGuiParm("gui_parm0", spawnArgs.GetString("gui_parm0"));

	StartSound("snd_static", SND_CHANNEL_AMBIENT);

	PostEventMS(&EV_PostSpawn, 100);


	//sound particle.
	idVec3 jointPos;
	idMat3 jointAxis;
	jointHandle_t jointHandle = GetAnimator()->GetJointHandle(CABLEJOINT);
	if (jointHandle == INVALID_JOINT) { gameLocal.Error("Maintpanel invalid joint '%s'", CABLEJOINT); }
	GetJointWorldTransform(jointHandle, gameLocal.time, jointPos, jointAxis);
	jointPos.z += 2.7f;

	idAngles soundEmitAngle = GetPhysics()->GetAxis().ToAngles();
	soundEmitAngle.pitch += 90;
	soundEmitAngle.yaw += -20;

	args.Clear();
	args.Set("model", spawnArgs.GetString("model_soundbeep"));
	args.SetVector("origin", jointPos);
	args.SetMatrix("rotation", soundEmitAngle.ToMat3());
	args.SetBool("hide", true);
	soundParticle = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	soundParticle->SetActive(false);

	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	systemIndex = spawnArgs.GetInt("systemindex");
}

void idMaintPanel::Event_PostSpawn(void) //We need to do this post-spawn because not all ents exist when Spawn() is called. So, we need to wait until AFTER spawn has happened, and call this post-spawn function.
{
	//Check if anything is hacking me....

	bool foundKeypadLockingMe = false;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idKeypad::Type))
			continue;
		
		//check if this keypad is targeting ME.
		if (ent->targets.Num() <= 0)
			continue;

		if (IsEntTargetingMe(ent))
		{
			foundKeypadLockingMe = true;
		}		
	}

	if (!foundKeypadLockingMe)
	{
		//If I am not being jammed by a keypad, then unlock me at game start.
		Unlock(false);
	}
}

bool idMaintPanel::IsEntTargetingMe(idEntity *ent)
{
	if (ent->targets.Num() <= 0)
		return false;

	for (int i = 0; i < ent->targets.Num(); i++)
	{
		if (!ent->targets[i].IsValid())
			continue;

		if (ent->targets[i].GetEntityNum() == this->entityNumber)
			return true;
	}

	return false;
}

void idMaintPanel::Save(idSaveGame *savefile) const
{
	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle

	savefile->WriteObject( smokeParticle ); // idFuncEmitter * smokeParticle
	savefile->WriteObject( soundParticle ); // idFuncEmitter * soundParticle

	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
	savefile->WriteInt( systemIndex ); // int systemIndex

	savefile->WriteObject( videoSignage ); // idStaticEntity* videoSignage
	savefile->WriteInt( videoSignageLifetimer ); // int videoSignageLifetimer
	savefile->WriteBool( videoSignageActive ); // bool videoSignageActive
}

void idMaintPanel::Restore(idRestoreGame *savefile)
{
	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadObject( CastClassPtrRef(smokeParticle) ); // idFuncEmitter * smokeParticle
	savefile->ReadObject( CastClassPtrRef(soundParticle) ); // idFuncEmitter * soundParticle

	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
	savefile->ReadInt( systemIndex ); // int systemIndex

	savefile->ReadObject( CastClassPtrRef(videoSignage) ); // idStaticEntity* videoSignage
	savefile->ReadInt( videoSignageLifetimer ); // int videoSignageLifetimer
	savefile->ReadBool( videoSignageActive ); // bool videoSignageActive
}

void idMaintPanel::Think(void)
{
	if (state == MPL_CLOSING)
	{
		if (gameLocal.time > stateTimer)
		{
			idStr voCue = "";
			if (systemIndex == SYS_TRASHCHUTES)
			{
				voCue = "snd_unlock_trash";
			}
			else if (systemIndex == SYS_AIRLOCKS)
			{
				voCue = "snd_unlock_airlocks";
			}
			else if (systemIndex == SYS_VENTS)
			{
				voCue = "snd_unlock_vents";
			}
			else if (systemIndex == SYS_WINDOWS)
			{
				voCue = "snd_unlock_windows";
			}

			if (voCue.Length() > 0)
			{
				StartSound(voCue.c_str(), SND_CHANNEL_VOICE);

				idVec3 forward, up;
				GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
				idVec3 particlePos = GetPhysics()->GetOrigin() + (forward * 5.5f);
				gameLocal.DoParticle(spawnArgs.GetString("model_soundwave2"), particlePos);

				//make the voice be a small interestpoint
				gameLocal.SpawnInterestPoint(this, particlePos, spawnArgs.GetString("interest_use"));
			}

			//BecomeInactive(TH_THINK);
			state = MPL_DONE;
		}
	}

	if (videoSignageActive && gameLocal.time > videoSignageLifetimer && videoSignage != nullptr)
	{
		videoSignage->PostEventMS(&EV_Remove, 0);
		videoSignage = nullptr;
		videoSignageActive = false;
	}
	

	idAnimatedEntity::Think();
}

//Calling eventactivate will UNLOCK a jammed maintpanel.
void idMaintPanel::Unlock(bool unlockAll)
{
	if (state != MPL_JAMMED)
		return;

	//keypad has been hacked.
	state = MPL_WAITINGFORPRESS;

	//light turns blue.
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = LIGHTRADIUS_OPERATIONAL;
	headlight.shaderParms[0] = 0;
	headlight.shaderParms[1] = .5f;
	headlight.shaderParms[2] = 0;
	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);

	SetColor(0, .6f, 1); //Make the light turn blue.

	if (smokeParticle->GetParticleActive())
	{
		smokeParticle->SetActive(false); //Turn off smoke emitters.
	}

	isFrobbable = true;

	Event_GuiNamedEvent(1, "onEnabled");

	StopSound(SND_CHANNEL_AMBIENT);
	StartSound("snd_beep", SND_CHANNEL_AMBIENT);
	soundParticle->Show();
	soundParticle->SetActive(true);
	
	

	//if (unlockAll)
	//{
	//	//keypads can be linked together. All keypads with same link value will deactivate.
	//	int myLinkNumber = spawnArgs.GetInt("link");
	//	if (myLinkNumber > 0)
	//	{
	//		idEntity *ent = NULL;
	//		for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	//		{
	//			if (!ent)
	//				continue;
	//
	//			if (ent->IsType(idMaintPanel::Type) && ent->entityNumber != this->entityNumber && ent->spawnArgs.GetInt("link") == myLinkNumber)
	//			{
	//				static_cast<idMaintPanel *>(ent)->Unlock(false); //activate the maintpanel.
	//			}
	//
	//			//Detach any linked keypads from the wall.
	//			if (ent->IsType(idKeypad::Type) && ent->entityNumber != this->entityNumber && ent->spawnArgs.GetInt("link") == myLinkNumber)
	//			{
	//				if (ent->targets.Num() > 0)
	//				{
	//					if (ent->targets[0].GetEntityNum() == this->entityNumber)
	//					{
	//						continue;
	//					}
	//				}
	//
	//				static_cast<idKeypad *>(ent)->DetachFromWall();
	//			}
	//		}
	//	}
	//}
}

bool idMaintPanel::DoFrob(int index, idEntity * frobber)
{
	if (state == MPL_WAITINGFORPRESS)
	{
		int myLinkNumber = spawnArgs.GetInt("link");
		if (myLinkNumber > 0)
		{
			idEntity *ent = NULL;
			for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
			{
				if (!ent)
					continue;

				if (ent->IsType(idMaintPanel::Type) && ent->spawnArgs.GetInt("link") == myLinkNumber)
				{
					static_cast<idMaintPanel *>(ent)->Close();
				}

				//Detach any linked keypads from the wall.
				if (ent->IsType(idKeypad::Type) && ent->spawnArgs.GetInt("link") == myLinkNumber)
				{
					//if (ent->targets.Num() > 0)
					//{
					//	if (ent->targets[0].GetEntityNum() == this->entityNumber)
					//	{
					//		continue;
					//	}
					//}

					static_cast<idKeypad *>(ent)->DetachFromWall();
				}
			}
		}
		else
		{
			Close();
		}
		
		//Call script.
		idStr callArg = spawnArgs.GetString("call");
		if (callArg.Length() > 0)
		{
			gameLocal.RunMapScriptArgs(callArg, gameLocal.GetLocalPlayer(), this);
		}

		//particle effect.
		idVec3 forward, up;
		GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
		idVec3 particlePos = GetPhysics()->GetOrigin() + (forward * 2.5f) + (up * -2);
		gameLocal.DoParticle(spawnArgs.GetString("model_leversmoke"), particlePos);

		//Do the system index.
		UnlockSystemIndex();
		//Event_GuiNamedEvent(1, "onLever");
		

		idStr videoModel = spawnArgs.GetString("model_videosignage");
		if (videoModel.Length() > 0)
		{
			//Create the video signage.
			idVec3 signagePos = GetPhysics()->GetOrigin() + (forward * 1) + (up * 11);

			idDict args;
			args.Clear();
			args.SetVector("origin", signagePos);
			args.SetMatrix("rotation", GetPhysics()->GetAxis());
			args.Set("model", videoModel.c_str());
			args.SetBool("solid", false);
			args.SetBool("noclipmodel", true);
			videoSignage = (idStaticEntity*)gameLocal.SpawnEntityType(idStaticEntity::Type, &args);

			if (videoSignage)
			{
				//This is a hack
				//We need to reset the video to start at the beginning but I can't quite figure out how to get
				//the material of the model.
				//So just manually grab the video material and reset its timer.

				const idMaterial* material;
				material = declManager->FindMaterial(spawnArgs.GetString("mtr_videosignage"));
				if (material)
				{
					material->ResetCinematicTime(0);
				}

				int videoDuration = spawnArgs.GetInt("videosignage_duration");
				if (videoDuration <= 0)
				{
					common->Error("maintpanel '%s' is missing 'videosignage_duration' spawnarg.", GetName());
				}

				videoSignageActive = true;
				videoSignageLifetimer = gameLocal.time + videoDuration;
			}
		}

		

		return true;
	}	
	

	//Still locked/jammed.
	Event_GuiNamedEvent(1, "onStuck");
	Event_PlayAnim("stuck", 0);
	return true;
}

void idMaintPanel::UnlockSystemIndex()
{
	idStr systemname = "";

	if (systemIndex <= SYS_NONE)
	{
		return;
	}
	else if (systemIndex == SYS_TRASHCHUTES)
	{
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetEnableTrashchutes(true);
		
		Event_GuiNamedEvent(1, "onLever");
		Event_SetGuiParm("unlockText", "#str_def_gameplay_trash_unlocked");
		gameLocal.AddEventLog("#str_def_gameplay_trash_unlocked", GetPhysics()->GetOrigin());
		systemname = "#str_def_gameplay_trash_name";
	}
	else if (systemIndex == SYS_AIRLOCKS)
	{
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetEnableAirlocks(true);

		Event_GuiNamedEvent(1, "onLever");
		Event_SetGuiParm("unlockText", "#str_def_gameplay_airlock_unlocked");
		gameLocal.AddEventLog("#str_def_gameplay_airlock_unlocked", GetPhysics()->GetOrigin());
		systemname = "#str_def_gameplay_airlock_name";
	}
	else if (systemIndex == SYS_VENTS)
	{
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetEnableVents(true);

		Event_GuiNamedEvent(1, "onLever");
		Event_SetGuiParm("unlockText", "#str_def_gameplay_vent_unlocked");
		gameLocal.AddEventLog("#str_def_gameplay_vent_unlocked", GetPhysics()->GetOrigin());
		systemname = "#str_def_gameplay_vent_name";
	}
	else if (systemIndex == SYS_WINDOWS)
	{
		static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetEnableWindows(true);

		Event_GuiNamedEvent(1, "onLever");
		Event_SetGuiParm("unlockText", "#str_def_gameplay_window_unlocked");
		gameLocal.AddEventLog("#str_def_gameplay_window_unlocked", GetPhysics()->GetOrigin());
		systemname = "#str_def_gameplay_window_name";
	}
	else
	{
		gameLocal.Error("Maintpanel '%s' has invalid systemindex value.", GetName());
	}

	gameLocal.GetLocalPlayer()->SetArmstatsFuseboxNeedUpdate();
}

void idMaintPanel::Close()
{
	BecomeActive(TH_THINK);
	state = MPL_CLOSING;
	Event_PlayAnim("close", 0);
	isFrobbable = false;
	StopSound(SND_CHANNEL_AMBIENT);
	StartSound("snd_lever", SND_CHANNEL_ANY);
	//stateTimer = gameLocal.time + CLOSETIME;
	stateTimer = gameLocal.time + VO_DELAYTIME;

	if (soundParticle->GetParticleActive())
	{
		soundParticle->SetActive(false);
	}

	if (smokeParticle->GetParticleActive())
	{
		smokeParticle->SetActive(false); //Turn off smoke emitters.
	}

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);

	gameLocal.GetLocalPlayer()->SetArmstatsFuseboxNeedUpdate();

	// SW 18th Feb 2025
	// We overwrite the zoom inspect offset here because the maintpanel has closed, 
	// and thus, we no longer want to be inspecting the inside of the door.
	spawnArgs.SetVector("zoominspect_campos", spawnArgs.GetVector("zoominspect_campos_afterclosing"));
}

bool idMaintPanel::IsDone()
{
	return (state == MPL_CLOSING || state == MPL_DONE);
}

int idMaintPanel::GetSystemIndex()
{
	return systemIndex;
}