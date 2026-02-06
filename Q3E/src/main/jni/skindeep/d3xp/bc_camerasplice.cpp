#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"
//#include "bc_ftl.h"
#include "idlib/LangDict.h"

#include "bc_meta.h"
#include "bc_notewall.h"
#include "bc_frobcube.h"
#include "SecurityCamera.h"
#include "bc_camerasplice.h"

const int ANIM_OPENTIME = 500; //Delay before camera switches allegience

const float COLOR_UNHACKED_R = 0;
const float COLOR_UNHACKED_G = 1.0f;
const float COLOR_UNHACKED_B = 0;

const float COLOR_HACKED_R = .2f;
const float COLOR_HACKED_G = .2f;
const float COLOR_HACKED_B = .2f;

const float BEAMWIDTH = 0.4f;
const int FANFARETIME = 10000;

const int TRANSCRIPT_LERPTIME = 800;

CLASS_DECLARATION(idAnimatedEntity, idCameraSplice)
END_CLASS

idCameraSplice::idCameraSplice(void)
{
	currentCamIdx = 0;
	transcriptNote = NULL;
	transcriptStartPos = vec3_zero;
	transcriptEndPos = vec3_zero;
	transcriptLerping = false;
	transcriptTimer = 0;

	waitingForTranscriptRead = false;
	waitingForTranscriptReadTimer = 0;

	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	soundParticle = nullptr;
	beamStart = nullptr;
	beamEnd = nullptr;
}

idCameraSplice::~idCameraSplice(void)
{
	StopSound(SND_CHANNEL_BODY3);

	if (state == CS_CLOSED && soundParticle != NULL) {
		soundParticle->PostEventMS(&EV_Remove, 0);
		soundParticle = nullptr;
	}

	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}

void idCameraSplice::Spawn(void)
{
	fl.takedamage = false;

	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);


	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	headlight.shader = declManager->FindMaterial("lights/pulse05", false);
	headlight.pointLight = true;
	headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 24.0f;
	headlight.shaderParms[0] = COLOR_UNHACKED_R;
	headlight.shaderParms[1] = COLOR_UNHACKED_G;
	headlight.shaderParms[2] = COLOR_UNHACKED_B;
	headlight.shaderParms[3] = 1.0f;
	headlight.noShadows = true;
	headlight.isAmbient = false;
	headlight.axis = mat3_identity;
	headlightHandle = gameRenderWorld->AddLightDef(&headlight);
	headlight.origin = GetPhysics()->GetOrigin() + forward * 6;
	gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);


	SetColor(1, 1, 0);
	state = CS_CLOSED;
	Event_PlayAnim("idle", 1);
	BecomeInactive(TH_THINK);

	soundParticle = NULL;

	fanfareState = CSF_NONE;
	fanfareTimer = 0;
	fanfareIcon = NULL;
	beamStart = NULL;
	beamEnd = NULL;

	//button frob cubes.
	idVec3 forwardDir, rightDir, upDir;
	this->GetPhysics()->GetAxis().ToAngles().ToVectors(&forwardDir, &rightDir, &upDir);

	for (int i = 0; i < CAMERASPLICE_BUTTONCOUNT; i++)
	{
		idVec3 buttonPos;
		idStr displayStr;
		if (i == CSB_OVERRIDE)
		{
			//CSB_OVERRIDE
			buttonPos = GetPhysics()->GetOrigin() + (forwardDir * 1) + (rightDir * -5) + (upDir * 1);
			displayStr = "#str_def_camerasplice_override";
		}
		else if (i == CSB_LEFT)
		{
			//CSB_LEFT
			buttonPos = GetPhysics()->GetOrigin() + (forwardDir * 1) + (rightDir * 4) + (upDir * -4);
			displayStr = "#str_def_camerasplice_previous";
		}
		else
		{
			//CSB_RIGHT
			buttonPos = GetPhysics()->GetOrigin() + (forwardDir * 1) + (rightDir * -1) + (upDir * -4);
			displayStr = "#str_def_camerasplice_next";
		}

		idDict args;
		args.Clear();
		args.Set("model", "models/objects/frobcube/cube2x2.ase");
		args.SetVector("cursoroffset", idVec3(1, 0, 0));
		args.SetInt("health", 1);
		args.Set("displayname", displayStr);

		if (i == CSB_OVERRIDE)
		{
			args.SetVector("cursoroffset", idVec3(1, 0, -3.5f));
		}

		buttons[i] = gameLocal.SpawnEntityType(idFrobcube::Type, &args);

		if (buttons[i] != NULL)
		{
			buttons[i]->SetOrigin(buttonPos);
			buttons[i]->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
			buttons[i]->GetPhysics()->GetClipModel()->SetOwner(this);
			static_cast<idFrobcube*>(buttons[i])->SetIndex(i);

			if (i == CSB_LEFT || i == CSB_RIGHT)
			{
				buttons[i]->isFrobbable = false;
			}

			buttons[i]->Bind(this, false);
		}
	}

	this->GetRenderEntity()->gui[0] = uiManager->FindGui(spawnArgs.GetString("gui"), true, true); //Create a UNIQUE gui so that it doesn't auto sync with other guis.


	//3-26-2025: locbox
	#define LOCBOXRADIUS 1
	idDict args;
	args.Clear();
	args.Set("text", common->GetLanguageDict()->GetString("#str_def_camerasplice_name"));
	args.SetVector("origin", GetPhysics()->GetOrigin() + (forward * 1) + (up * -4) + (right  * 1.5f));
	args.SetBool("playerlook_trigger", true);
	args.SetVector("mins", idVec3(-LOCBOXRADIUS, -LOCBOXRADIUS, -LOCBOXRADIUS));
	args.SetVector("maxs", idVec3(LOCBOXRADIUS, LOCBOXRADIUS, LOCBOXRADIUS));
	static_cast<idTrigger_Multi*>(gameLocal.SpawnEntityType(idTrigger_Multi::Type, &args));

	BecomeInactive(TH_THINK);
}

//This is called at game start.
void idCameraSplice::AssignCamera(idEntity *cameraEnt)
{
	assignedCamera = cameraEnt;

	//This is where the camera gets initialized. Piggyback some sound/particle stuff onto here
	StartSound("snd_idle", SND_CHANNEL_BODY3);

	idVec3 forward, right, up;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
	idDict args;
	args.Clear();
	args.Set("model", spawnArgs.GetString("model_soundwave"));
	args.SetVector("origin", GetPhysics()->GetOrigin() + (forward * 2.5f) + (right * -5) + (up * 1));
	soundParticle = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));

	//At game start, make the camera feed appear on the gui.
	SwitchToCamera(cameraEnt);

	if (cameraEnt->IsType(idSecurityCamera::Type))
	{
		currentCamIdx = static_cast<idSecurityCamera *>(cameraEnt)->cameraIndex;
	}


	//Check if transcript note exists.
	if (cameraEnt->targets.Num() > 0)
	{
		if (assignedCamera.GetEntity()->targets[0].GetEntity()->IsType(idNoteWall::Type))
		{
			//Found the note.
			transcriptNote = assignedCamera.GetEntity()->targets[0].GetEntity();

			SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_transcript")));
		}
	}
}

void idCameraSplice::Save(idSaveGame *savefile) const
{
	savefile->WriteObject( assignedCamera ); //  idEntityPtr<idEntity> assignedCamera

	savefile->WriteInt( currentCamIdx ); //  int currentCamIdx

	savefile->WriteInt( state ); //  int state
	savefile->WriteInt( stateTimer ); //  int stateTimer

	savefile->WriteRenderLight( headlight ); //  renderLight_t headlight
	savefile->WriteInt( headlightHandle ); //  int headlightHandle

	savefile->WriteObject( soundParticle ); //  idFuncEmitter			* soundParticle


	savefile->WriteInt( fanfareState ); //  int fanfareState

	savefile->WriteObject( fanfareIcon ); //  idEntity				* fanfareIcon
	savefile->WriteObject( beamStart ); //  idBeam*					 beamStart
	savefile->WriteObject( beamEnd ); //  idBeam*					 beamEnd
	savefile->WriteInt( fanfareTimer ); //  int fanfareTimer

	SaveFileWriteArray(buttons, CAMERASPLICE_BUTTONCOUNT, WriteObject);  // idEntity* buttons[CAMERASPLICE_BUTTONCOUNT];

	savefile->WriteObject( transcriptNote ); //  idEntityPtr<idEntity> transcriptNote
	savefile->WriteVec3( transcriptStartPos ); //  idVec3 transcriptStartPos
	savefile->WriteVec3( transcriptEndPos ); //  idVec3 transcriptEndPos
	savefile->WriteBool( transcriptLerping ); //  bool transcriptLerping
	savefile->WriteInt( transcriptTimer ); //  int transcriptTimer

	savefile->WriteBool( waitingForTranscriptRead ); //  bool waitingForTranscriptRead
	savefile->WriteInt( waitingForTranscriptReadTimer ); //  int waitingForTranscriptReadTimer

}

void idCameraSplice::Restore(idRestoreGame *savefile)
{
	savefile->ReadObject( assignedCamera ); //  idEntityPtr<idEntity> assignedCamera

	savefile->ReadInt( currentCamIdx ); //  int currentCamIdx

	savefile->ReadInt( state ); //  int state
	savefile->ReadInt( stateTimer ); //  int stateTimer

	savefile->ReadRenderLight( headlight ); //  renderLight_t headlight
	savefile->ReadInt( headlightHandle ); //  int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}

	savefile->ReadObject( CastClassPtrRef(soundParticle) ); //  idFuncEmitter			* soundParticle


	savefile->ReadInt( fanfareState ); //  int fanfareState

	savefile->ReadObject( fanfareIcon ); //  idEntity				* fanfareIcon
	savefile->ReadObject( CastClassPtrRef(beamStart) ); //  idBeam*					 beamStart
	savefile->ReadObject( CastClassPtrRef(beamEnd) ); //  idBeam*					 beamEnd
	savefile->ReadInt( fanfareTimer ); //  int fanfareTimer

	SaveFileReadArray(buttons, ReadObject);  // idEntity* buttons[CAMERASPLICE_BUTTONCOUNT];

	savefile->ReadObject( transcriptNote ); //  idEntityPtr<idEntity> transcriptNote
	savefile->ReadVec3( transcriptStartPos ); //  idVec3 transcriptStartPos
	savefile->ReadVec3( transcriptEndPos ); //  idVec3 transcriptEndPos
	savefile->ReadBool( transcriptLerping ); //  bool transcriptLerping
	savefile->ReadInt( transcriptTimer ); //  int transcriptTimer

	savefile->ReadBool( waitingForTranscriptRead ); //  bool waitingForTranscriptRead
	savefile->ReadInt( waitingForTranscriptReadTimer ); //  int waitingForTranscriptReadTimer
}

void idCameraSplice::Think(void)
{
	if (state == CS_OPENING)
	{
		if (gameLocal.time >= stateTimer + ANIM_OPENTIME)
		{
			//This is what gets called after a override button, a short delay, and then the camera changes allegience.

			//Open.
			//Event_PlayAnim("idlesway", 4, true);
			state = CS_IDLEOPEN;
			ConnectToCamera();			
			fanfareState = CSF_INITIALIZE;

			//Show hacked fanfare.
			Event_GuiNamedEvent(1, "channelfanfare");
			Event_SetGuiInt("hacked", 1);

			//make buttons frobbable now.
			buttons[CSB_LEFT]->isFrobbable = true;
			buttons[CSB_RIGHT]->isFrobbable = true;
		}
	}

	if (fanfareState == CSF_INITIALIZE)
	{
		if (assignedCamera.IsValid())
		{
			//Spawn the fanfare icons. Draw where the camera is, draw a line connecting to it.
			idDict args;
			args.Set("classname", "func_static");
			args.Set("model", "models/objects/ui_icon/icon.ase");
			args.SetBool("drawglobally", true);
			args.SetVector("origin", assignedCamera.GetEntity()->GetPhysics()->GetOrigin());
			gameLocal.SpawnEntityDef(args, &fanfareIcon);
			
			fanfareIcon->Hide(); //bc disable for now

			//Laser endpoint.
			args.Clear();
			args.SetVector("origin", assignedCamera.GetEntity()->GetPhysics()->GetOrigin());
			//args.Set("width", 8);
			beamEnd = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
			//beamEnd->BecomeActive(TH_PHYSICS);
			//beamEnd->SetOrigin(GetPhysics()->GetOrigin());

			//Laser startpoint.
			args.Clear();
			args.Set("target", beamEnd->name.c_str());
			args.SetVector("origin", GetPhysics()->GetOrigin());
			args.SetBool("start_off", true);
			args.SetFloat("width", BEAMWIDTH);
			args.Set("skin", "skins/beam_camerasplice");
			args.SetBool("drawglobally", true);
			beamStart = (idBeam *)gameLocal.SpawnEntityType(idBeam::Type, &args);
			
			beamStart->Hide(); //bc disable for now...


			fanfareState = CSF_ACTIVE;
			fanfareTimer = gameLocal.time + FANFARETIME;
		}
	}
	else if (fanfareState == CSF_ACTIVE)
	{
		if (gameLocal.time >= fanfareTimer)
		{
			fanfareState = CSF_DONE;
			beamStart->Hide();
			fanfareIcon->Hide();

			//BecomeInactive(TH_THINK);
		}
	}

	if (transcriptLerping)
	{
		float lerp = (gameLocal.time - transcriptTimer) / (float)TRANSCRIPT_LERPTIME;
		lerp = idMath::ClampFloat(0, 1, lerp);

		if (transcriptNote.IsValid())
		{
			idVec3 lerpedPos;
			lerpedPos.Lerp(transcriptStartPos, transcriptEndPos, lerp);
			transcriptNote.GetEntity()->GetPhysics()->SetOrigin(lerpedPos);
			transcriptNote.GetEntity()->UpdateVisuals();
		}

		if (transcriptTimer + TRANSCRIPT_LERPTIME <= gameLocal.time)
		{
			transcriptLerping = false;

			if (transcriptNote.IsValid())
			{
				transcriptNote.GetEntity()->isFrobbable = true;
			}
		}
	}

	if (waitingForTranscriptRead && gameLocal.time > waitingForTranscriptReadTimer && transcriptNote.IsValid())
	{
		waitingForTranscriptReadTimer = gameLocal.time + 300;

		if (transcriptNote.GetEntity()->IsType(idNoteWall::Type))
		{
			if (static_cast<idNoteWall *>(transcriptNote.GetEntity())->GetRead())
			{
				SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_default")));
				waitingForTranscriptRead = false;
			}
		}		
	}

	idAnimatedEntity::Think();
}

bool idCameraSplice::DoFrob(int index, idEntity * frobber)
{
	if (state == CS_CLOSED && index == CSB_OVERRIDE)
	{
		state = CS_OPENING;
		Event_PlayAnim("override", 0);
		stateTimer = gameLocal.time;
		BecomeActive(TH_THINK);

		StopSound(SND_CHANNEL_BODY3);

		soundParticle->PostEventMS(&EV_Remove, 0);
		soundParticle = nullptr;

		SetColor(0, 1, 0);
		//SetSkin(declManager->FindSkin("skins/camerasplice_noblink"));
		if (headlightHandle != -1)
		{
			headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 48.0f;
			headlight.shaderParms[0] = COLOR_HACKED_R;
			headlight.shaderParms[1] = COLOR_HACKED_G;
			headlight.shaderParms[2] = COLOR_HACKED_B;
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
		}

		//Make the transcript appear, if one is assigned.
		if (transcriptNote.IsValid())
		{
			//Found note associated with camerasplice.
			StartSound("snd_print", SND_CHANNEL_ANY);

			idVec3 forward, right, up;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
			idVec3 paperPos = GetPhysics()->GetOrigin() + (right * -1) + (forward  * .5f);
			idEntity *note = transcriptNote.GetEntity();
			note->SetAxis(GetPhysics()->GetAxis());
			note->SetOrigin(paperPos);
			note->Show();
			note->isFrobbable = false;

			transcriptStartPos = paperPos;
			transcriptEndPos = GetPhysics()->GetOrigin() + (right * -13) + (forward  * .5f);
			transcriptLerping = true;
			transcriptTimer = gameLocal.time;

			waitingForTranscriptRead = true;
		}


		buttons[CSB_OVERRIDE]->isFrobbable = false;
	}
	else if (index == CSB_LEFT)
	{
		Event_PlayAnim("buttonleft", 1);
		SwitchToNextCamera(-1);
	}
	else if (index == CSB_RIGHT)
	{
		Event_PlayAnim("buttonright", 1);
		SwitchToNextCamera(1);
	}
	else if (state == CS_IDLEOPEN)
	{
		ConnectToCamera();
	}

	return true;
}

//void idCameraSplice::Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType)
//{
//}

void idCameraSplice::ConnectToCamera()
{
	if (assignedCamera.IsValid())
	{
		bool newUnlock = false;
		if (assignedCamera.GetEntity()->IsType(idSecurityCamera::Type))
		{			
			if (assignedCamera.GetEntity()->team != TEAM_FRIENDLY)
			{
				//Swap camera to friendly team.
				newUnlock = true;
				static_cast<idSecurityCamera *>(assignedCamera.GetEntity())->SetTeam(TEAM_FRIENDLY);
			}
		}

		//gameLocal.GetLocalPlayer()->StartCameraSplice(assignedCamera.GetEntity(), newUnlock);
		return;
	}	

	//Assigned camera is down, deleted, killed, etc. Return null, we just search for and pick anything available.
	//gameLocal.GetLocalPlayer()->StartCameraSplice(NULL);
}





void idCameraSplice::SwitchToCamera(idEntity *cameraEnt)
{
	spawnArgs.Set("cameratarget", cameraEnt->GetName());
	Event_UpdateCameraTarget();
	
	idLocationEntity *locationEnt = gameLocal.LocationForEntity(cameraEnt);

	//if (locationEnt) gameRenderWorld->DebugTextSimple(locationEnt->GetLocation(), cameraEnt->GetPhysics()->GetOrigin());

	Event_SetGuiParm("roomname", (locationEnt == NULL) ? "#str_loc_unknown_00104" : locationEnt->GetLocation());

	int totalCameras = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->totalSecuritycamerasAtGameStart;	
	int displayIdx = 0;
	if (cameraEnt->IsType(idSecurityCamera::Type))
	{
		displayIdx = static_cast<idSecurityCamera *>(cameraEnt)->cameraIndex + 1;
	}
	Event_SetGuiParm("channelstring", va("%d/%d", displayIdx, totalCameras));

	bool isHacked = cameraEnt->team == TEAM_FRIENDLY;
	Event_SetGuiInt("hacked", isHacked);
}

void idCameraSplice::SwitchToNextCamera(int direction)
{
	int newIdx = currentCamIdx + direction;

	int totalCameras = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->totalSecuritycamerasAtGameStart;
	if (newIdx >= totalCameras)
	{
		newIdx = 0; //go back to first camera.
	}
	else if (newIdx < 0)
	{
		newIdx = totalCameras - 1; //go to last camera.
	}

	currentCamIdx = newIdx;

	idEntity * newActiveCamera = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetSecurityCameraViaIndex(currentCamIdx);
	if (newActiveCamera == NULL)
	{
		//color bars.
		Event_SetGuiInt("colorbars", 1);
		Event_SetGuiInt("hacked", 0);
	}
	else
	{
		Event_SetGuiInt("colorbars", 0);
		SwitchToCamera(newActiveCamera);
	}

	Event_GuiNamedEvent(1, "channelSwitch");
	StartSound("snd_channelswitch", SND_CHANNEL_ANY);
}



void idCameraSplice::ListByClassNameDebugDraw()
{
	if (assignedCamera.IsValid())
	{
		idVec3 origin = GetPhysics()->GetOrigin();	
		
		// Find the closest camera
		float closestDist = idMath::INFINITY;
		idVec3 closestPoint(vec3_zero);
		idEntity *closestEnt = NULL;
		idEntity *ent = NULL;
		for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
		{
			if (ent->IsType(idSecurityCamera::Type) && ent->GetPhysics())
			{
				float newDist = (ent->GetPhysics()->GetOrigin() - origin).Length();
				if (newDist < closestDist)
				{
					closestPoint = ent->GetPhysics()->GetOrigin();
					closestDist = newDist;
					closestEnt = ent;
				}
			}
		}

		// If the assigned camera is the closest camera, draw a single green arrow
		// Otherwise draw a red arrow to camera and blue arrow to closest
		if (closestEnt == assignedCamera.GetEntity())
		{
			gameRenderWorld->DebugArrow(colorGreen, origin, assignedCamera.GetEntity()->GetPhysics()->GetOrigin(), 8, 900000);
		}
		else
		{
			gameRenderWorld->DebugArrow(colorRed, origin, assignedCamera.GetEntity()->GetPhysics()->GetOrigin(), 8, 900000);
			gameRenderWorld->DebugArrow(colorBlue, origin, closestPoint, 8, 900000);
		}
	}
}

