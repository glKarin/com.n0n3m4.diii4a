#include "bc_meta.h"
#include "bc_sabotagelever.h"

//TYPES
// 0 = hyperfuel
// 1 = electricity
// 2 = navcomputer


const int SABOLEVER_THINKINTERVAL = 300;

const idEventDef EV_PostPress("postpress", NULL);

CLASS_DECLARATION( idAnimated, idSabotageLever)
	EVENT(EV_Activate,	idSabotageLever::DoFrob)
	EVENT(EV_PostPress, idSabotageLever::Event_PostPress)
END_CLASS

void idSabotageLever::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( typeIndex ); // int typeIndex
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( nextThinkTime ); // int nextThinkTime
}

void idSabotageLever::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( typeIndex ); // int typeIndex
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( nextThinkTime ); // int nextThinkTime
}

void idSabotageLever::Spawn( void )
{
	//make frobbable, shootable, and no player collision.
	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	this->noGrab = true;
	this->isFrobbable = true;

	renderEntity.shaderParms[ SHADERPARM_RED ] = 0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = 1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;

	nextThinkTime = 0;
	state = IDLE;

	typeIndex = spawnArgs.GetInt("type", "-1");
}

void idSabotageLever::UpdateStates( void )
{
	if (state == PRIMED)
	{
		if (gameLocal.time > nextThinkTime)
		{
			nextThinkTime = gameLocal.time + SABOLEVER_THINKINTERVAL;
			
			//Todo: do a explosion proximity check
			
		}
	}
}

bool idSabotageLever::DoFrob(int index, idEntity * frobber)
{
	if (state == PRIMED)
	{
		//If already pressed, then do nothing.
		return false;
	}

	state = PRIMED;

	StartSound( "snd_press", SND_CHANNEL_BODY, 0, false, NULL );
	
	Event_PlayAnim( spawnArgs.GetString("anim_press", "press"), 1 );

	//Become yellow.
	renderEntity.shaderParms[ SHADERPARM_RED ] = 1;
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = .5f;
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = 0;
	UpdateVisuals();

	isFrobbable = false;

	//Deactivate the pipe type.
	if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->SetPipeStatus(typeIndex, false))
	{
		//The FTL has been totally deactivated.
		StartSound("snd_shutdown_all", SND_CHANNEL_BODY2, 0, false, NULL);

		PostEventMS(&EV_PostPress, 4000); //Metal stress sound.
	}
	else
	{
		ChangeAllLevers(idVec4(1, .8f, 0, 1)); //Make all levers YELLOW.
		StartSound("snd_shutdown_single", SND_CHANNEL_BODY2, 0, false, NULL);
	}

	return true;
}

void idSabotageLever::ChangeAllLevers(idVec4 newcolor)
{
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		//if (gameLocal.entities[i]->entityNumber == this->entityNumber ) //skip self.
		//	continue;

		if (!gameLocal.entities[i]->IsType(idSabotageLever::Type))
			continue;

		if (static_cast<idSabotageLever *>(gameLocal.entities[i])->typeIndex != this->typeIndex)
			continue;

		gameLocal.entities[i]->SetColor(newcolor); //change color.
		gameLocal.entities[i]->isFrobbable = false; //make it not frobbable.
	}
}



void idSabotageLever::Event_PostPress()
{
	//make a ship metal stress sound after the entire FTL drive has been shut down...
	StartSound("snd_shipstress", SND_CHANNEL_BODY3, 0, false, NULL);
}

void idSabotageLever::Think( void )
{
	UpdateStates();
	
	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}


void idSabotageLever::PostFTLReset()
{
	BecomeInactive(TH_THINK);

	if (state == ARMED)
	{
		state = EXHAUSTED;
		isFrobbable = false;
		SetColor(idVec4(1, 0, 0, 1));
	}
	else if (state == PRIMED || state == IDLE)
	{
		//Reset back to idle state.
		state = IDLE;
		isFrobbable = true;
		SetColor(idVec4(0, 1, 0, 1));
	}
}

void idSabotageLever::SetSabotagedArmed()
{
	SetColor(idVec4(1, 0, 0, 1)); //Make it colored RED.
	isFrobbable = false; //Make it unfrobbable.

	if (state == PRIMED) //If this lever has been primed...
	{
		state = ARMED;
		BecomeActive(TH_THINK); //Turn on the tripwire explosive.
	}
}