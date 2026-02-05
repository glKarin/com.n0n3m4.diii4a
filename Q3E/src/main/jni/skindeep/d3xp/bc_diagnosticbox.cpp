#include "sys/platform.h"
#include "gamesys/SysCvar.h"
//#include "physics/Physics_RigidBody.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"
//#include "Moveable.h"
//#include "trigger.h"
\
#include "ai/AI.h"
#include "WorldSpawn.h"
#include "framework/DeclEntityDef.h"

#include "SecurityCamera.h"
#include "bc_sabotagepoint.h"
#include "bc_meta.h"
#include "bc_diagnosticbox.h"


const int UPDATETIME = 300;


const int CLOSE_MINDISTANCE = 384; //close the panel if player gets xx distance away

const int STAYOPENTIME = 4000;

const float PILLAR_MOVETIME = .9f; //in seconds.


#define FROBINDEX_DRAWINGRESS_ON 40
#define FROBINDEX_DRAWINGRESS_OFF 41

#define FROBINDEX_PREVCAM 50
#define FROBINDEX_NEXTCAM 51



#define INGTYPE_TRASH		1
#define INGTYPE_AIRLOCK		2
#define INGTYPE_WINDOW		3


CLASS_DECLARATION(idAnimated, idDiagnosticBox)
	//EVENT(EV_PostSpawn, idDiagnosticBox::Event_PostSpawn)
END_CLASS


idDiagnosticBox::idDiagnosticBox(void)
{
	spacenudgeNode.SetOwner(this);
	spacenudgeNode.AddToEnd(gameLocal.spacenudgeEntities);
}

idDiagnosticBox::~idDiagnosticBox(void)
{
	spacenudgeNode.Remove();
}

void idDiagnosticBox::Spawn(void)
{
	state = DB_CLOSED;
	fl.takedamage = false;
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	isFrobbable = true;
    updateTimer = 0;


	//Determine whether to use the flipped upside-down pillar or not. We determine this if roll = close-ish to 180 degrees.
	idAngles myAngle = GetPhysics()->GetAxis().ToAngles();
	isUpsideDown = (180 - idMath::Fabs(myAngle.roll) < 5);


	//Create the pillar mover.
	idVec3 up, forward;
	GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, &up);
	idDict args;
	args.Clear();
	args.SetVector("origin", this->GetPhysics()->GetOrigin() + (up * -73)); //start sunken underground.
	args.SetMatrix("rotation", GetPhysics()->GetAxis());
	args.Set("model", isUpsideDown ? "models/objects/diagnosticbox/diagnosticbox_pillar_flip.ase" : "models/objects/diagnosticbox/diagnosticbox_pillar.ase");
	args.Set("gui", "guis/game/diagnosticbox.gui");
	args.SetBool("send_owner_impulse", true);
	pillar = (idMover *)gameLocal.SpawnEntityType(idMover::Type, &args);
	pillar->SetAngles(this->GetPhysics()->GetAxis().ToAngles());
	pillar->Event_SetMoveTime(PILLAR_MOVETIME);
	pillar->GetPhysics()->GetClipModel()->SetOwner(this);



	//Create the radar tower.
	//args.Clear();
	//args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * -16) + (up * -24));
	//args.SetMatrix("rotation", GetPhysics()->GetAxis());
	//args.Set("model", "models/objects/diagnosticbox/radar/tower.ase");
	//args.SetInt("solid", 1);
	//idEntity *tower = gameLocal.SpawnEntityType(idStaticEntity::Type, &args);
	//tower->Bind(pillar, true);
	//
	////Create the rotating radar dish.
	//#define DISH_ROTATE_SPEED 24
	//args.Clear();
	//args.SetVector("origin", this->GetPhysics()->GetOrigin() + (forward * -16) + (up * 168));
	//args.SetMatrix("rotation", GetPhysics()->GetAxis());
	//args.Set("model", "models/objects/diagnosticbox/radar/dish.ase");
	//args.SetInt("solid", 1);
	//idEntity *dish = (idMover *)gameLocal.SpawnEntityType(idMover::Type, &args);
	
	//idAngles angles(0, DISH_ROTATE_SPEED, 0);
	//static_cast<idMover *>(dish)->Event_Rotate(angles);
	//
	//dish->Bind(tower, true);

	currentCamIdx = 0;


    BecomeActive(TH_THINK);	

	PostEventMS(&EV_PostSpawn, 0);
}





void idDiagnosticBox::Event_PostSpawn(void)
{
	//Gather up info on how many entities we have.
	int trashCount = 0;
	int airlockCount = 0;	
	int windowCount = 0;

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->spawnArgs.GetBool("ingress_ent") || ent->IsHidden())
			continue;

		int ingressType = ent->spawnArgs.GetInt("ingress_type");

		if (ingressType == INGTYPE_TRASH)
			trashCount++;
		else if (ingressType == INGTYPE_AIRLOCK)
			airlockCount++;
		else if (ingressType == INGTYPE_WINDOW)
			windowCount++;
		else
			gameLocal.Warning("Entity '%s' has unknown ingress_type value.", ent->GetName());
	}

	pillar->Event_SetGuiInt("ingress_total", trashCount + airlockCount + windowCount);
	pillar->Event_SetGuiInt("ingress_airlock",  airlockCount );
	pillar->Event_SetGuiInt("ingress_trash", trashCount );
	pillar->Event_SetGuiInt("ingress_window", windowCount);


	pillar->Event_SetGuiParm("shipname", gameLocal.GetShipName().c_str());
	pillar->Event_SetGuiParm("shipdesc", gameLocal.GetShipDesc().c_str());

}

void idDiagnosticBox::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( currentCamIdx ); //  int currentCamIdx

	savefile->WriteInt( stateTimer ); //  int stateTimer
	savefile->WriteInt( state ); //  int state

	savefile->WriteInt( updateTimer ); //  int updateTimer

	savefile->WriteObject( pillar ); //  idMover * pillar

	savefile->WriteBool( isUpsideDown ); //  bool isUpsideDown
}

void idDiagnosticBox::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( currentCamIdx ); //  int currentCamIdx

	savefile->ReadInt( stateTimer ); //  int stateTimer
	savefile->ReadInt( state ); //  int state

	savefile->ReadInt( updateTimer ); //  int updateTimer

	savefile->ReadObject( CastClassPtrRef(pillar) ); //  idMover * pillar

	savefile->ReadBool( isUpsideDown ); //  bool isUpsideDown
}

void idDiagnosticBox::Think(void)
{
    if (state == DB_OPEN)
    {
        if (gameLocal.time >= updateTimer)
        {
            updateTimer = gameLocal.time + UPDATETIME;

            int itemsMarked = 0;

            //Iterate over all the bad guys.
            for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
            {
                if (!entity->IsActive() || entity->IsHidden() || entity->team != TEAM_ENEMY || entity->health <= 0)
                    continue;

                if (!entity->IsType(idAI::Type))
                    continue;            

				itemsMarked++;
            }            

			pillar->Event_SetGuiInt("soulscount", itemsMarked);

			int worldState = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetCombatState();
			if (worldState == COMBATSTATE_SEARCH)
			{
				pillar->Event_SetGuiParm("status", "#str_gui_hud_100107"); /*searching*/
			}
			else if (worldState == COMBATSTATE_COMBAT)
			{
				pillar->Event_SetGuiParm("status", "#str_gui_hud_100106"); /*combat*/
			}
			else
			{
				pillar->Event_SetGuiParm("status", "#str_gui_hud_100154"); /*clear*/
			}
        }

        if (gameLocal.time > stateTimer)
        {
            //has stayed open for the minimum time. Now check for player's distance
            float distanceToPlayer = (GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();

            if (distanceToPlayer > CLOSE_MINDISTANCE)
            {
                state = DB_CLOSED;
                Event_PlayAnim("close", 1);
                isFrobbable = true;
                BecomeInactive(TH_THINK);

				idVec3 up;
				GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);
				idVec3 pillarUndeployedPosition = GetPhysics()->GetOrigin() + (up * -73);
				pillar->Event_MoveToPos(pillarUndeployedPosition);
            }
        }
    }

	idAnimated::Think();
}



bool idDiagnosticBox::DoFrob(int index, idEntity * frobber)
{
	idEntity::DoFrob(index, frobber);

    if (state == DB_CLOSED)
    {
		if (spawnArgs.GetBool("activateTargetsOnOpen"))
			ActivateTargets(gameLocal.GetLocalPlayer());

        state = DB_OPEN;
        Event_PlayAnim("open", 1);
        isFrobbable = false;
        stateTimer = gameLocal.time + STAYOPENTIME;

		idVec3 up;
		GetPhysics()->GetAxis().ToAngles().ToVectors(NULL, NULL, &up);
		idAngles particleAngle = up.ToAngles();
		particleAngle.pitch += 90;
		gameLocal.DoParticle("smoke_ring18.prt", GetPhysics()->GetOrigin() + up * 8, particleAngle.ToForward());

		idVec3 pillarDeployedPosition;

		if (isUpsideDown)
		{
			//when upside down, the pillar extrudes out less distance.
			pillarDeployedPosition = GetPhysics()->GetOrigin() + (up * -32);
		}
		else
		{
			//right-side up.
			pillarDeployedPosition = GetPhysics()->GetOrigin() + (up * -32);
		}

		pillar->Event_MoveToPos(pillarDeployedPosition);

		InitializeDiagnosticCamera();

        BecomeActive(TH_THINK);
    }

	return true;
}

void idDiagnosticBox::DoGenericImpulse(int index)
{
	if (index == FROBINDEX_PREVCAM)
	{
		SwitchToNextCamera(-1);
	}
	else if (index == FROBINDEX_NEXTCAM)
	{
		SwitchToNextCamera(1);
	}
	else if (index == FROBINDEX_DRAWINGRESS_ON)
	{
		gameLocal.GetLocalPlayer()->SetDrawIngressPoints(true);
	}
	else if (index == FROBINDEX_DRAWINGRESS_OFF)
	{
		gameLocal.GetLocalPlayer()->SetDrawIngressPoints(false);
	}

}

void idDiagnosticBox::InitializeDiagnosticCamera()
{
	idEntity * activeCamera = GetAnyCamera();

	if (activeCamera == NULL)
		return;

	idLocationEntity *locationEnt = gameLocal.LocationForEntity(activeCamera);
	pillar->Event_SetGuiParm("roomname", locationEnt == NULL ? "UNKNOWN" : locationEnt->GetLocation());
	//pillar->Event_SetGuiInt("colorbars", 0);
	pillar->spawnArgs.Set("cameratarget", activeCamera->GetName());	
	pillar->Event_UpdateCameraTarget();

	currentCamIdx = static_cast<idSecurityCamera *>(activeCamera)->cameraIndex;
}

idEntity *idDiagnosticBox::GetAnyCamera()
{	
	for (idEntity* secCamEnt = gameLocal.securitycameraEntities.Next(); secCamEnt != NULL; secCamEnt = secCamEnt->securitycameraNode.Next())
	{
		if (secCamEnt == NULL)
			continue;

		if (!secCamEnt->IsType(idSecurityCamera::Type))
			continue;
		
		return secCamEnt;
	}

	return NULL;	
}

//bc: This code is unfortunately copy pasted from bc_camerasplice
void idDiagnosticBox::SwitchToNextCamera(int direction)
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
		pillar->Event_SetGuiInt("colorbars", 1);
		pillar->Event_SetGuiInt("hacked", 0);
	}
	else
	{
		pillar->Event_SetGuiInt("colorbars", 0);
		SwitchToCamera(newActiveCamera);
	}

	pillar->Event_GuiNamedEvent(1, "channelSwitch");
	pillar->StartSound("snd_channelswitch", SND_CHANNEL_ANY);
}

void idDiagnosticBox::SwitchToCamera(idEntity *cameraEnt)
{
	pillar->spawnArgs.Set("cameratarget", cameraEnt->GetName());
	pillar->Event_UpdateCameraTarget();

	idLocationEntity *locationEnt = gameLocal.LocationForEntity(cameraEnt);
	pillar->Event_SetGuiParm("roomname", (locationEnt == NULL) ? "UNKNOWN..." : locationEnt->GetLocation());

	int totalCameras = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->totalSecuritycamerasAtGameStart;
	int displayIdx = 0;
	if (cameraEnt->IsType(idSecurityCamera::Type))
	{
		displayIdx = static_cast<idSecurityCamera *>(cameraEnt)->cameraIndex + 1;
	}
	pillar->Event_SetGuiParm("channelstring", va("%d/%d", displayIdx, totalCameras));

	bool isHacked = cameraEnt->team == TEAM_FRIENDLY;
	pillar->Event_SetGuiInt("hacked", isHacked);
}
