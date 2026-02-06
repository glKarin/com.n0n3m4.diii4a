#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Entity.h"
//#include "Light.h"
#include "Player.h"
//#include "Fx.h"
//#include "framework/DeclEntityDef.h"
//#include "bc_ftl.h"


#include "bc_frobcube.h"
#include "bc_teletext.h"

//const int TRANSCRIPT_LERPTIME = 800;
const int ROTATESPEED = -15;

const idEventDef EV_teletext_setActive("TeletextSetActive", "d");
const idEventDef EV_teletext_setMaterial("TeletextSetMaterial", "s");


CLASS_DECLARATION(idAnimated, idTeletext)
	EVENT(EV_teletext_setActive, idTeletext::SetTeletextActive)
	EVENT(EV_teletext_setMaterial, idTeletext::SetTeletextMaterial)
END_CLASS

idTeletext::idTeletext(void)
{
	state = TT_IDLE;
	mover = NULL;
}

idTeletext::~idTeletext(void)
{
}

void idTeletext::Spawn(void)
{
	fl.takedamage = false;

	//Remove collision, but still allow frob/damage
	GetPhysics()->SetContents(CONTENTS_RENDERMODEL);
	GetPhysics()->SetClipMask(MASK_SOLID | CONTENTS_MOVEABLECLIP);

	Event_PlayAnim("idle", 1);
	BecomeActive(TH_THINK);

	//Set up the 2 spot lights.
	for (int i = 0; i < 2; i++)
	{
		idDict lightArgs;
		lightArgs.Clear();
		lightArgs.SetVector("origin", GetPhysics()->GetOrigin());
		lightArgs.Set("texture", spawnArgs.GetString("mtr_slide"));
		lightArgs.SetInt("noshadows", 0);
		lightArgs.SetInt("start_off", 1);

		if (i == 0)
		{
			lightArgs.Set("light_target", "384 0 0");
			lightArgs.Set("light_right", "0 0 256");
			lightArgs.Set("light_up", "0 256 0");
		}
		else
		{
			lightArgs.Set("light_target", "-384 0 0");
			lightArgs.Set("light_right", "0 0 256");
			lightArgs.Set("light_up", "0 -256 0");
		}

		
		lightArgs.SetBool("noshadows", true);
		spotlights[i] = (idLight*)gameLocal.SpawnEntityType(idLight::Type, &lightArgs);
		spotlights[i]->Bind(this, true);
	}

	//Spawn the yellow arrow.
	idDict args;
	args.Set("classname", "func_static");
	args.Set("model", spawnArgs.GetString("model_arrow"));
	args.SetBool("spin", true);
	args.Set("bind", GetName());
	args.SetVector("origin", GetPhysics()->GetOrigin() + idVec3(0, 0, 21));
	args.SetBool("solid", false);
	gameLocal.SpawnEntityDef(args, &arrowProp);
}


void idTeletext::Save(idSaveGame *savefile) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer

	SaveFileWriteArray( spotlights, 2, WriteObject ); // idLight* spotlights[2]
	savefile->WriteObject( mover ); // idMover* mover

	savefile->WriteObject( arrowProp ); // idEntity* arrowProp
}

void idTeletext::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer

	SaveFileReadArrayCast( spotlights, ReadObject, idClass*& ); // idLight* spotlights[2]
	savefile->ReadObject( CastClassPtrRef(mover) ); // idMover* mover

	savefile->ReadObject( arrowProp ); // idEntity* arrowProp
}

void idTeletext::Think(void)
{
	if (state == TT_MOVINGTOPOSITION)
	{
		if (gameLocal.time > stateTimer)
		{
			//Start the projector show.

			StartSlideShow();
		}
	}

	idAnimated::Think();
}

void idTeletext::StartSlideShow()
{
	if (state == TT_PROJECTING)
		return;

	state = TT_PROJECTING;
	SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_on")));

	for (int i = 0; i < 2; i++)
	{
		spotlights[i]->FadeIn(2);
	}

	mover->Event_SetAccellerationTime(0);
	mover->Event_SetDecelerationTime(0);
	idAngles rotateSpeed = idAngles(0, ROTATESPEED, 0);
	mover->Event_Rotate(rotateSpeed);

	idStr showstartScript = spawnArgs.GetString("call_showstart");
	if (showstartScript.Length() > 0)
	{
		gameLocal.RunMapScriptArgs(showstartScript.c_str(), this, this);
	}

	StartSound("snd_start", SND_CHANNEL_ANY);
}


void idTeletext::SetTeletextMaterial(const char* materialName)
{
	for (int i = 0; i < 2; i++)
	{
		spotlights[i]->SetShader(materialName);
	}
}

void idTeletext::SetTeletextActive(int value)
{
	//make it stop the show.

	if (value <= 0)
	{
		mover->Event_StopRotating();

		for (int i = 0; i < 2; i++)
		{
			spotlights[i]->FadeOut(.5f);
		}

		SetSkin(declManager->FindSkin(spawnArgs.GetString("skin_off")));
		state = TT_DORMANT;
	}
}

bool idTeletext::DoFrob(int index, idEntity * frobber)
{
	if (state == TT_IDLE)
	{
		//move to position.
		state = TT_MOVINGTOPOSITION;

		arrowProp->Hide();

		if (targets.Num() > 0)
		{			
			idDict args;
			args.Clear();
			args.SetVector("origin", this->GetPhysics()->GetOrigin()); //start sunken underground.
			args.SetMatrix("rotation", GetPhysics()->GetAxis());
			args.SetBool("solid", false);
			mover = (idMover*)gameLocal.SpawnEntityType(idMover::Type, &args);

			if (mover)
			{
				Bind(mover, true);
				mover->GetPhysics()->GetClipModel()->SetOwner(this);
				
				float moveTime = spawnArgs.GetFloat("movetime");
				mover->Event_SetMoveTime(moveTime);
				mover->Event_SetAccellerationTime(moveTime * .4f);
				mover->Event_SetDecelerationTime(moveTime * .4f);				

				idVec3 targetPos = targets[0].GetEntity()->GetPhysics()->GetOrigin();
				mover->Event_MoveToPos(targetPos);

				stateTimer = gameLocal.time + ((moveTime + .5f) * 1000);
				isFrobbable = false;

				StartSound("snd_move", SND_CHANNEL_ANY);
			}
			else
			{
				gameLocal.Error("teletext '%s' failed to spawn movers.", GetName());
			}			
		}
		else
		{
			gameLocal.Error("teletext '%s' has no targets.", GetName());
		}
	}
	else if (state == TT_DORMANT)
	{
		isFrobbable = false;
		StartSlideShow();
	}



	return true;
}

