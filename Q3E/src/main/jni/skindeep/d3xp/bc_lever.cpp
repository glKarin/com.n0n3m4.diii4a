#include "script/Script_Thread.h"

#include "Player.h"
#include "ai/AI.h"
#include "bc_lever.h"

const idEventDef EV_Button_Enable("enableButton", "d");

CLASS_DECLARATION(idAnimated, idLever)
	EVENT(EV_Activate,		idLever::Event_Activate)
	EVENT(EV_Button_Enable, idLever::SetActive)
END_CLASS

idLever::idLever()
{
	soundParticle = NULL;
}

idLever::~idLever(void)
{
	if (soundParticle)
	{
		delete soundParticle;
	}
}

void idLever::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( nextStateTime ); // int nextStateTime
	savefile->WriteBool( isActive ); // bool isActive
	savefile->WriteObject( soundParticle ); // idFuncEmitter * soundParticle
}

void idLever::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( nextStateTime ); // int nextStateTime
	savefile->ReadBool( isActive ); // bool isActive
	savefile->ReadObject( CastClassPtrRef(soundParticle) ); // idFuncEmitter * soundParticle
}

void idLever::Spawn( void )
{
	//make frobbable, shootable, and no player collision.
	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	this->noGrab = true;
	this->isFrobbable = spawnArgs.GetBool("frobbable", "1");
	isActive = true;

	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	nextStateTime = 0;
	state = IDLE;


	int soundloopParticle = spawnArgs.GetInt("soundparticle", "0");
	if (soundloopParticle > 0)
	{
		//Sound particle. This is for the windowseal lever.
		idVec3 jointPos;
		idMat3 jointAxis;
		jointHandle_t leverJoint = animator.GetJointHandle("lever");
		this->GetJointWorldTransform(leverJoint, gameLocal.time, jointPos, jointAxis);
		idDict args;
		args.Clear();
		args.Set("model", (soundloopParticle <= 1) ? "sound_loop.prt" : "sound_loop_big.prt");
		args.SetVector("origin", jointPos);
		soundParticle = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	}
	else
	{
		soundParticle = NULL;
	}


	BecomeActive(TH_THINK);
	isActive = spawnArgs.GetBool("buttonactive", "1");
	if (!isActive)
	{
		SetActive(false);
	}
}

void idLever::UpdateStates( void )
{
	if (state == PRESSED)
	{
		if (gameLocal.time > nextStateTime)
		{
			//Return to green.
			if (isActive)
			{
				renderEntity.shaderParms[SHADERPARM_RED] = 0;
				renderEntity.shaderParms[SHADERPARM_GREEN] = 1;
				renderEntity.shaderParms[SHADERPARM_BLUE] = 0;
			}

			state = IDLE;
			isFrobbable = true;

			UpdateVisuals();
		}
	}
}

bool idLever::DoFrob(int index, idEntity * frobber)
{
	idEntity::DoFrob(index, frobber);

	if (state == IDLE)
	{
		//does button require a key or something
		idStr requirement = spawnArgs.GetString("requireClass");
		if (requirement.Length() > 0)
		{
			//TODO: allow throwing the object directly at the thing.
			if (gameLocal.GetLocalPlayer()->HasItemViaClassname(requirement))
			{
				//Success.
				
				//Remove object from inventory.
				gameLocal.GetLocalPlayer()->RemoveItemViaClassname(requirement);
			}
			else
			{
				//Fail.				
				StartSound("snd_cancel", SND_CHANNEL_ANY);
				return true;
			}
		}
	}

	return Event_Activate( frobber );
}

bool idLever::Event_Activate( idEntity *activator )
{
	float waitTime;
	if ( state == PRESSED )
	{
		//If already pressed, then do nothing.
		return false;
	}

	if ( jockeyFrobDebounceMax > 0 && activator != NULL )
	{
		if ( activator->IsType( idAI::Type ) )
		{
			if ( static_cast< idAI * >( activator )->aiState == AISTATE_JOCKEYED )
			{
				if ( gameLocal.time < jockeyFrobDebounceTimer )
				{
					//debounce time hasn't expired yet. Exit here.
					return false;
				}

				jockeyFrobDebounceTimer = gameLocal.time + jockeyFrobDebounceMax;
			}
		}
	}

	state = PRESSED;
	isFrobbable = false;

	

	if (isActive)
	{
		//is Unlocked.
		StartSound("snd_press", SND_CHANNEL_BODY, 0, false, NULL);
		Event_PlayAnim(spawnArgs.GetString("anim_press", "press"), 1);		
	}
	else if ( !isActive )
	{
		//is Locked.
		StartSound("snd_cancel", SND_CHANNEL_BODY, 0, false, NULL);
		Event_PlayAnim(spawnArgs.GetString("anim_locked", "press"), 1);
		nextStateTime = gameLocal.time + 500;

		//BC call script call when frobbed while locked.
		idStr scriptName = spawnArgs.GetString("call_locked", "");
		if (scriptName.Length() > 0)
		{
			gameLocal.RunMapScriptArgs(scriptName.c_str(), activator, this);
		}

		return false;
	}

	waitTime = spawnArgs.GetFloat("animtime", "0.5");
	if (waitTime <= 0)
	{
		waitTime = 0.1f;
	}
	nextStateTime = gameLocal.time + (waitTime * 1000);

	//Become yellow.
	renderEntity.shaderParms[SHADERPARM_RED] = 1;
	renderEntity.shaderParms[SHADERPARM_GREEN] = .7f;
	renderEntity.shaderParms[SHADERPARM_BLUE] = 0;

	

	if ( GetPhysics()->GetClipModel()->GetOwner() != NULL )
	{
		GetPhysics()->GetClipModel()->GetOwner()->DoFrob( spawnArgs.GetInt( "frobindex", "0" ), activator );
	}
	else
	{
		idStr frobTargetname = spawnArgs.GetString("frobtarget");
		if (frobTargetname.Length() > 0)
		{
			idEntity *frobtargetEnt = gameLocal.FindEntity(frobTargetname);
			if (frobtargetEnt)
			{
				frobtargetEnt->DoFrob(0, activator);
			}
		}

		ActivateTargets( this );

		//BC call script.
		idStr scriptName = spawnArgs.GetString("call", "");
		if ( scriptName.Length() > 0 )
		{
			gameLocal.RunMapScriptArgs(scriptName.c_str(), activator, this);
			//const function_t	*scriptFunction;
			//scriptFunction = gameLocal.program.FindFunction( scriptName );
			//if ( scriptFunction )
			//{
			//	assert( scriptFunction->parmSize.Num() <= 2 );
			//	idThread *thread = new idThread();
			//
			//	// 1 or 0 args always pushes activator (this preserves the old behavior)
			//	thread->PushArg( activator );
			//	// 2 args pushes self as well
			//	if ( scriptFunction->parmSize.Num() == 2 )
			//	{
			//		thread->PushArg( this );
			//	}
			//
			//	thread->CallFunction( scriptFunction, false );
			//	thread->DelayedStart( 0 );
			//}
		}
	}

	

	return true;
}

void idLever::Think( void )
{
	UpdateStates();
	
	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}

//Whether the button is locked or unlocked
void idLever::SetActive(bool value)
{
	isActive = value;

	if (!value)
	{
		//make red.
		renderEntity.shaderParms[SHADERPARM_RED] = 1;
		renderEntity.shaderParms[SHADERPARM_GREEN] = 0;
		renderEntity.shaderParms[SHADERPARM_BLUE] = 0;
	}
	else
	{
		//make green.
		renderEntity.shaderParms[SHADERPARM_RED] = 0;
		renderEntity.shaderParms[SHADERPARM_GREEN] = 1;
		renderEntity.shaderParms[SHADERPARM_BLUE] = 0;
	}

	UpdateVisuals();
}



void idLever::Hide(void)
{
	idEntity::Hide();
	
	if (soundParticle != NULL)
	{
		soundParticle->Hide();
	}
}

void idLever::Show(void)
{
	idEntity::Show();

	if (soundParticle != NULL)
	{
		soundParticle->Show();
	}
}
