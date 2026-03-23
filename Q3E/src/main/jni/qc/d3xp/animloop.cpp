
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_loopsetactive("loopsetactive", "d");
const idEventDef EV_loopdone( "<loopdone>" );

CLASS_DECLARATION( idAnimated, idAnimloop )
	EVENT( EV_loopdone,				idAnimloop::Loopdone )
	EVENT( EV_loopsetactive,		idAnimloop::loopsetactive )
END_CLASS


void idAnimloop::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(blendFrames);
	savefile->WriteFloat(randomtime);
	savefile->WriteFloat(randomstart);
	savefile->WriteString(animName.c_str());
	savefile->WriteBool(active);
}

void idAnimloop::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(blendFrames);
	savefile->ReadFloat(randomtime);
	savefile->ReadFloat(randomstart);
	savefile->ReadString(animName);
	savefile->ReadBool(active);
}

void idAnimloop::loopsetactive(int value)
{
	active = value;
}

void idAnimloop::Loopdone( void )
{
	if (!active)
		return;

	idAnimatedEntity::Event_PlayAnim(animName, blendFrames);

	int len = animator.CurrentAnim( ANIMCHANNEL_ALL )->PlayLength();
	if (randomtime > 0)
	{
		len -= gameLocal.random.RandomFloat() * randomtime;
	}

	PostEventMS( &EV_loopdone, len );
}

void idAnimloop::Spawn( void )
{
	active = true;

	blendFrames = spawnArgs.GetInt("blendframes", "24");
	randomtime = spawnArgs.GetFloat("randomtime", "0");
	randomstart = spawnArgs.GetFloat("randomstart", "0");
	animName = spawnArgs.GetString("anim");
	
	if (randomstart > 0)
	{
		idAnimatedEntity::Event_PlayAnim(animName, blendFrames);
		PostEventSec( &EV_loopdone, randomstart );
	}
	else if (randomstart < 0)
	{
		//choose a random value.
		idAnimatedEntity::Event_PlayAnim(animName, blendFrames);

		float randValue = gameLocal.random.RandomFloat() * 3.1f;
		PostEventSec( &EV_loopdone, randValue );
	}
	else
	{
		Loopdone();
	}	
}



