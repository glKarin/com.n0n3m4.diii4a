
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_frobscript( "<frobscript>" );

CLASS_DECLARATION( idAnimated, idLever )
	EVENT( EV_frobscript,		idLever::OnFrobScript)
END_CLASS


void idLever::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool(toggleable);
	savefile->WriteInt(toggleState);
	savefile->WriteInt(state);
	savefile->WriteInt(nextStateTime);
}

void idLever::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool(toggleable);
	savefile->ReadInt(toggleState);
	savefile->ReadInt(state);
	savefile->ReadInt(nextStateTime);
}

void idLever::Spawn( void )
{
	//make frobbable and no collision.
	//this->GetPhysics()->SetContents(CONTENTS_CORPSE);


	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );



	this->isFrobbable = true;
	this->noGrab = true;

	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	nextStateTime = 0;
	toggleable = spawnArgs.GetBool("toggleable", "0");
	toggleState = 0;
	state = IDLE;


	if (spawnArgs.GetBool("start_on", "0"))
	{
		toggleState = !toggleState;
	}


	BecomeActive(TH_THINK);
}

void idLever::Reset( void )
{
	if (spawnArgs.GetBool("start_on", "0"))
	{
		toggleState = 1;
	}
	else
	{
		toggleState = 0;
	}
}

void idLever::UpdateStates( void )
{
	if (state == PRESSED)
	{
		if (gameLocal.time > nextStateTime)
		{
			renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
			renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1;
			renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

			this->isFrobbable = true;

			state = IDLE;
		}
	}
}

void idLever::OnFrobScript( void )
{
	idStr scriptName = spawnArgs.GetString( "call", "" );

	if (scriptName.Length() <= 0)
		return;

	if (toggleable)
	{
		if (toggleState > 0)
		{
			scriptName = scriptName + "_on";
		}
		else
		{
			scriptName = scriptName + "_off";
		}
	}


	const function_t	*scriptFunction;
	scriptFunction = gameLocal.program.FindFunction( scriptName );
	if ( scriptFunction )
	{
		idThread *thread;
		thread = new idThread( scriptFunction );
		thread->DelayedStart( 0 );
	}
	
}

void idLever::OnFrob( idEntity* activator )
{

	float waitTime;

	if (state == PRESSED)
	{
		return;
	}



	state = PRESSED;

	StartSound( "snd_press", SND_CHANNEL_BODY, 0, false, NULL );

	if (!toggleable)
	{
		Event_PlayAnim( spawnArgs.GetString("pushanim", "push"), 4 );
	}
	else
	{
		if (toggleState)
		{
			Event_PlayAnim( spawnArgs.GetString("pullanim", "pull"), 4 );
		}
		else
		{
			Event_PlayAnim( spawnArgs.GetString("pushanim", "push"), 4 );
			
		}

		toggleState = !toggleState;
	}

	renderEntity.shaderParms[ SHADERPARM_RED ] = 1;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = .75f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	waitTime = spawnArgs.GetFloat("animtime", "0.3");

	if (waitTime <= 0)
	{
		waitTime = 0.1f;
	}

	nextStateTime = gameLocal.time + (waitTime * 1000);

	ActivateTargets(this);	

	this->isFrobbable = false;

	float delay = spawnArgs.GetFloat("delay", "0");
	PostEventSec( &EV_frobscript, delay );
}

void idLever::Think( void )
{
	UpdateStates();
	
	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}
