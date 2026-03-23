
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


const idEventDef EV_countdownreset(		"countdownreset"  );
const idEventDef EV_countdownstart(		"countdownstart"  );
const idEventDef EV_countdownhasTripped("countdownhasTripped", NULL, 'd'  );
                                         
CLASS_DECLARATION( idEntity, idCountdowntimer )
	EVENT( EV_countdownreset,		idCountdowntimer::Event_reset)
	EVENT( EV_countdownstart,		idCountdowntimer::Event_start)

	EVENT( EV_countdownhasTripped,	idCountdowntimer::Event_hasTripped)

	EVENT( EV_Activate,				idCountdowntimer::Event_Activate )
END_CLASS


void idCountdowntimer::Save( idSaveGame *savefile ) const
{
	savefile->WriteInt(time);
	savefile->WriteInt(maxTime);
	savefile->WriteBool(active);
	savefile->WriteBool(hasTripped);

	for (int i = 0; i < VO_BOOLSIZE; i++)
	{
		savefile->WriteBool(voBool[i]);
	}
}

void idCountdowntimer::Restore( idRestoreGame *savefile )
{
	savefile->ReadInt(time);
	savefile->ReadInt(maxTime);
	savefile->ReadBool(active);
	savefile->ReadBool(hasTripped);

	for (int i = 0; i < VO_BOOLSIZE; i++)
	{
		savefile->ReadBool(voBool[i]);
	}
}

void idCountdowntimer::Spawn( void )
{
	for (int i = 0; i < VO_BOOLSIZE; i++)
	{
		voBool[i] = false;
	}

	int totaltime = spawnArgs.GetInt("time", "60");
	maxTime = totaltime;

	active = false;
	hasTripped = false;

	BecomeActive( TH_THINK );
}


void idCountdowntimer::Think( void )
{
	if (!active)
		return;

	int timeLeft = time - gameLocal.time;

	//common->Printf("%d time remaining\n", timeLeft);

	if (timeLeft <= 0)
	{
		//we have run out of time.
		ActivateTargets(this);

		idStr scriptName = spawnArgs.GetString( "call" );

		if (scriptName.Length() > 0)
		{
			const function_t	*scriptFunction;
			scriptFunction = gameLocal.program.FindFunction( scriptName );
			if ( scriptFunction )
			{
				idThread *thread;
				thread = new idThread( scriptFunction );
				thread->DelayedStart( 0 );
			}
		}

		StartSound(  "snd_timeexpired" , SND_CHANNEL_ANY, 0, false, NULL );		

		active = false;
		return;
	}

	DoVO(timeLeft, 1, VO_1SEC, "time_01");		
	DoVO(timeLeft, 2, VO_2SEC, "time_02");
	DoVO(timeLeft, 3, VO_3SEC, "time_03");
	DoVO(timeLeft, 4, VO_4SEC, "time_04");
	DoVO(timeLeft, 5, VO_5SEC, "time_05");
	DoVO(timeLeft, 6, VO_6SEC, "time_06");
	DoVO(timeLeft, 7, VO_7SEC, "time_07");
	DoVO(timeLeft, 8, VO_8SEC, "time_08");
	DoVO(timeLeft, 9, VO_9SEC, "time_09");
	DoVO(timeLeft, 10, VO_10SEC, "time_10");
	DoVO(timeLeft, 30, VO_30SEC, "time_30");
	DoVO(timeLeft, 45, VO_45SEC, "time_45");
	DoVO(timeLeft, 60, VO_1MIN, "time_60");
	DoVO(timeLeft, 120, VO_2MIN, "time_02min");
	DoVO(timeLeft, 180, VO_3MIN, "time_03min");
	DoVO(timeLeft, 240, VO_4MIN, "time_04min");
	DoVO(timeLeft, 300, VO_5MIN, "time_05min");
		
}

bool idCountdowntimer::DoVO(int timeLeft, int count, int voEnum, const char *voName)
{
	if (maxTime >= count && timeLeft <= (count * 1000) && !voBool[voEnum])
	{
		StartSound( va("snd_%s", voName), SND_CHANNEL_ANY, 0, false, NULL );
		voBool[voEnum] = true;
		return true;
	}

	return false;
}

void idCountdowntimer::Event_hasTripped( void )
{
	idThread::ReturnInt( this->hasTripped );
}

void idCountdowntimer::Event_reset( void )
{
	for (int i = 0; i < VO_BOOLSIZE; i++)
	{
		voBool[i] = false;
	}

	active = false;
	hasTripped = false;
}

void idCountdowntimer::Event_start( void )
{
	if (active)
		return;

	active = true; //make this thing active.

	time = gameLocal.time + (maxTime * 1000);
	gameLocal.GetLocalPlayer()->Event_hudMessage( va( common->GetLanguageDict()->GetString( "#str_02120" ), maxTime ) );
	hasTripped = true;
}

void idCountdowntimer::Event_Activate( idEntity *activator )
{
	Event_start();
}