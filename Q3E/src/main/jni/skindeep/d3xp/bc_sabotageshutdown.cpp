#include "bc_meta.h"
#include "bc_sabotageshutdown.h"

//This is for the primary sabotage entity.
//This shuts down an FTL jump.

const int PRESS_DELAYTIME = 500;

CLASS_DECLARATION(idAnimatedEntity, idSabotageShutdown)
	EVENT(EV_Activate,	idSabotageShutdown::DoFrob)
END_CLASS

void idSabotageShutdown::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt( state ); // int state
	savefile->WriteInt( stateTimer ); // int stateTimer
}

void idSabotageShutdown::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt( state ); // int state
	savefile->ReadInt( stateTimer ); // int stateTimer
}

void idSabotageShutdown::Spawn( void )
{
	//make frobbable, shootable, and no player collision.
	GetPhysics()->SetContents( CONTENTS_RENDERMODEL );
	GetPhysics()->SetClipMask( MASK_SOLID | CONTENTS_MOVEABLECLIP );

	this->noGrab = true;
	this->isFrobbable = true;

	state = IDLE;
	SetColor(idVec4(0, 1, 0, 1));
	stateTimer = 0;
}


bool idSabotageShutdown::DoFrob(int index, idEntity * frobber)
{
	if (state != IDLE)
	{
		//If already pressed, then do nothing.
		return false;
	}

	state = PRESS_DELAY;
	stateTimer = gameLocal.time + PRESS_DELAYTIME;

	Event_PlayAnim(spawnArgs.GetString("anim_press", "press"), 1);
	StartSound( "snd_press", SND_CHANNEL_BODY, 0, false, NULL );
	SetColor(idVec4(1, 1, 0, 1));
	
	return true;
}

void idSabotageShutdown::Think( void )
{
	if (state == PRESS_DELAY)
	{
		if (gameLocal.time > stateTimer)
		{
			SetEnabled(false);

			//Shut down the FTL.
			static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->FTL_DoShutdown();
		}
	}

	idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}



void idSabotageShutdown::SetEnabled(bool value)
{
    if (value)
    {
        state = IDLE;
        SetColor(colorGreen);
    }
    else
    {
        state = COOLDOWN_WAIT;
        SetColor(colorRed);
    }
}