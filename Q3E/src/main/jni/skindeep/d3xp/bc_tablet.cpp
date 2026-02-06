#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
#include "Light.h"
#include "Player.h"
#include "Fx.h"

#include "bc_notewall.h"
#include "bc_tablet.h"


CLASS_DECLARATION(idMoveableItem, idTablet)
END_CLASS

idTablet::idTablet(void)
{
	memset(&headlight, 0, sizeof(headlight));
	headlightHandle = -1;

	state = TBLT_LOCKED;
	thinkTimer = 0;
}

idTablet::~idTablet(void)
{
	if (headlightHandle != -1)
		gameRenderWorld->FreeLightDef(headlightHandle);
}

void idTablet::Spawn(void)
{
	isFrobbable = true;
	fl.takedamage = true;	

	SetColor(0, 1, 0); //green.
	//BecomeActive(TH_THINK);
}

void idTablet::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( thinkTimer ); // int thinkTimer

	savefile->WriteRenderLight( headlight ); // renderLight_t headlight
	savefile->WriteInt( headlightHandle ); // int headlightHandle
}

void idTablet::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( thinkTimer ); // int thinkTimer

	savefile->ReadRenderLight( headlight ); // renderLight_t headlight
	savefile->ReadInt( headlightHandle ); // int headlightHandle
	if ( headlightHandle != - 1 ) {
		gameRenderWorld->UpdateLightDef( headlightHandle, &headlight );
	}
}

void idTablet::Think(void)
{
	if (!fl.hidden)
	{		
		if (headlightHandle != -1)
		{
			//Update light position.
			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			headlight.origin = GetPhysics()->GetOrigin() + (forward * 4);
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
		}
		else
		{
			idVec3 forward;
			GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
			idVec3 lightPos = GetPhysics()->GetOrigin() + forward * 8;
			headlight.shader = declManager->FindMaterial("lights/defaultPointLight", false);
			headlight.pointLight = true;
			headlight.lightRadius[0] = headlight.lightRadius[1] = headlight.lightRadius[2] = 16;
			headlight.shaderParms[0] = .2f;
			headlight.shaderParms[1] = .4f;
			headlight.shaderParms[2] = .2f;
			headlight.shaderParms[3] = 1.0f;
			headlight.noShadows = true;
			headlight.isAmbient = false;
			headlight.axis = mat3_identity;
			headlightHandle = gameRenderWorld->AddLightDef(&headlight);
			headlight.origin = lightPos;
			gameRenderWorld->UpdateLightDef(headlightHandle, &headlight);
		}
	}

	//if (state == TBLT_LOCKED)
	//{
	//	if (gameLocal.time > thinkTimer)
	//	{
	//		thinkTimer = gameLocal.time + 200;
	//
	//		if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
	//		{
	//			if (gameLocal.GetLocalPlayer()->GetCarryable() == this)
	//			{
	//				
	//			}
	//		}
	//	}
	//}

	idMoveableItem::Think();
}

//void idTablet::Killed(idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location)
//{
//	if (!fl.takedamage)
//		return;
//
//	fl.takedamage = false;	
//}

bool idTablet::DoFrob(int index, idEntity * frobber)
{
	if (index == CARRYFROB_INDEX && frobber == gameLocal.GetLocalPlayer())
	{
		if (state == TBLT_LOCKED)
		{
			SetRead();
			//BecomeInactive(TH_THINK);
		}
		else if (state == TBLT_UNLOCKED)
		{
			//special frob when holding the tablet.
			gameLocal.GetLocalPlayer()->DoInspectCurrentItem();
		}

		return true;
	}

	return idMoveableItem::DoFrob(index, frobber);
}

void idTablet::SetRead()
{
	//unlock it.
	state = TBLT_UNLOCKED;

	StartSound("snd_unlock", SND_CHANNEL_ANY);
	Event_GuiNamedEvent(1, "unlock");
	SetColor(0, 0, 0); //no light.	
	CreateMemorypalaceClone();
}

bool idTablet::GetRead()
{
	return (state == TBLT_UNLOCKED);
}

void idTablet::CreateMemorypalaceClone()
{
	//Create the memory palace note clone.
	idEntity *noteClone = NULL;
	idDict args;
	args.Clear();
	args.Set("classname", spawnArgs.GetString("def_memoryent"));
	args.Set("gui_parm0", spawnArgs.GetString("gui_parm0"));
	args.SetFloat("gui_parm1", spawnArgs.GetFloat("gui_parm1"));
	args.Set("gui", spawnArgs.GetString("gui_memory"));

	// SW 3rd April 2025: To avoid the camera clipping into the wall, 
	// zoom-inspecting memory palace notes is done exclusively via FOV reduction.
	// Please ensure that the below `zoominspect_campos` offset is equivalent to 
	// the FORWARD_DISTANCE defined in idPlayer::DoMemoryPalace().
	// The goal is to prevent the camera from physically moving in space.
	args.SetVector("zoominspect_campos", idVec3(30, 0, 0));
	args.SetFloat("zoominspect_fov", spawnArgs.GetFloat("zoominspect_memory_fov", "70"));

	gameLocal.SpawnEntityDef(args, &noteClone);
	if (noteClone)
	{
		if (noteClone->IsType(idNoteWall::Type))
		{
			static_cast<idNoteWall *>(noteClone)->SetMemorypalaceClone();
		}
	}
}

void idTablet::Hide(void)
{
	idMoveableItem::Hide();

	if (headlightHandle != -1)
	{
		gameRenderWorld->FreeLightDef(headlightHandle);
		headlightHandle = -1;
	}
}