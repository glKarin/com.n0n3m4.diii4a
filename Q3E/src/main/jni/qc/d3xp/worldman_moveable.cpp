
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_wmm_startplay( "worldmoveable_play" );
const idEventDef EV_wmm_startrecord( "worldmoveable_record" );
const idEventDef EV_wmm_stop( "worldmoveable_stop" );


CLASS_DECLARATION( idEntity, idWorldManager_Moveable )
	EVENT( EV_wmm_startplay,			idWorldManager_Moveable::Event_startplay)
	EVENT( EV_wmm_startrecord,			idWorldManager_Moveable::Event_startrecord)
	EVENT( EV_wmm_stop,					idWorldManager_Moveable::Event_stop)

END_CLASS


void idWorldManager_Moveable::Spawn( void )
{
	if (spawnArgs.GetString("target0"))
	{
		targetEnt = gameLocal.FindEntity(spawnArgs.GetString("target0",""));
	}
	else if (spawnArgs.GetString("target"))
	{
		targetEnt = gameLocal.FindEntity(spawnArgs.GetString("target",""));
	}

	if (!targetEnt.IsValid())
	{
		common->Error("unable to find target ent for worldmanager_moveable: %s", this->GetName());
		return;
	}

	events.Clear();
	lastPosition.Zero();
	state = OFF;
	lastRecordTime = 0;
	startTime = 0;
	index = 0;
	lastTimestamp = 0;

	BecomeActive( TH_THINK );
}

void idWorldManager_Moveable::Event_startplay( void )
{
	index = 0;
	state = PLAYING;
	startTime = gameLocal.time;
}

void idWorldManager_Moveable::Event_startrecord( bool continuation )
{
	index = 0;
	state = RECORDING;
	startTime = gameLocal.time;

	if (!continuation)
	{
		events.Clear();
	}

	if (events.Num() > 0)
	{
		lastTimestamp = events[events.Num() - 1].timestamp + gameLocal.time;
	}
}


void idWorldManager_Moveable::Event_stop( void )
{
	index = 0;
	state = OFF;
}

void idWorldManager_Moveable::UpdateRecord( void )
{
	if (gameLocal.time < lastRecordTime)
	{
		return;
	}

	lastRecordTime = gameLocal.time + 200;

	idVec3 currentPosition = targetEnt.GetEntity()->GetPhysics()->GetOrigin();

	if ((int)lastPosition.x == (int)currentPosition.x && (int)lastPosition.y == (int)currentPosition.y && (int)lastPosition.z == (int)currentPosition.z)
	{
		return;
	}

	lastPosition = currentPosition;

	record_moveable_t newEvent;
	newEvent.timestamp = gameLocal.time - startTime;
	newEvent.position = currentPosition;
	newEvent.angle = targetEnt.GetEntity()->GetPhysics()->GetAxis();	
	
	events.Append( newEvent );
}

void idWorldManager_Moveable::UpdatePlay( void )
{
	if (index > events.Num() - 1)
	{
		// No more events! Get outta here.
		return;
	}

	if (events[index].timestamp > gameLocal.time - startTime)
	{
		return;
	}

	// Valid event. Execute it.
	targetEnt.GetEntity()->SetOrigin(events[index].position);
	targetEnt.GetEntity()->SetAngles(events[index].angle.ToAngles());

	index++;
}

void idWorldManager_Moveable::Think( void )
{
	if (state == OFF)
	{
		return;
	}

	if (state == RECORDING)
	{
		if (gameLocal.time < lastTimestamp)
		{
			UpdatePlay();
		}
		else
		{
			UpdateRecord();
		}
	}
	else if (state == PLAYING)
	{
		UpdatePlay();
	}
}




