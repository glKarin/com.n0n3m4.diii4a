
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"


#define OFFSET_MULTIPLIER 60000

const idEventDef EV_clockreset(		"clockreset" );
const idEventDef EV_clocksettime(	"clocksettime", "d" );

CLASS_DECLARATION( idAnimatedEntity, idClock )
	EVENT( EV_clockreset,		idClock::Event_reset)
	EVENT( EV_clocksettime,		idClock::Event_setTime)
	
END_CLASS

void idClock::Save( idSaveGame *savefile ) const
{
	savefile->WriteBool(digital);
	savefile->WriteInt(startTime);
	savefile->WriteInt(offsetTime);
	savefile->WriteInt(lastMinuteCount);
	savefile->WriteFloat(startAngle);
	savefile->WriteFloat(endAngle);
	savefile->WriteInt(minuteStartTime);
	savefile->WriteInt(minuteEndTime);

	savefile->WriteInt(nextSecondsInterval);
	savefile->WriteInt(secondsLerpStart);
	savefile->WriteInt(secondsLerpEnd);
	savefile->WriteInt(secondsState);
	savefile->WriteInt(secondsAngleStart);
	savefile->WriteInt(secondsAngleEnd);
}

void idClock::Restore( idRestoreGame *savefile )
{
	savefile->ReadBool(digital);
	savefile->ReadInt(startTime);
	savefile->ReadInt(offsetTime);
	savefile->ReadInt(lastMinuteCount);
	savefile->ReadFloat(startAngle);
	savefile->ReadFloat(endAngle);
	savefile->ReadInt(minuteStartTime);
	savefile->ReadInt(minuteEndTime);

	savefile->ReadInt(nextSecondsInterval);
	savefile->ReadInt(secondsLerpStart);
	savefile->ReadInt(secondsLerpEnd);
	savefile->ReadInt(secondsState);
	savefile->ReadInt(secondsAngleStart);
	savefile->ReadInt(secondsAngleEnd);
}

void idClock::Spawn( void )
{
	digital = spawnArgs.GetBool("digital", "0");

	this->GetPhysics()->SetContents(0);
	
	this->startTime = 0;


	lastMinuteCount = 0;
	startAngle = 0;
	endAngle = 0;
	minuteStartTime = 0;
	minuteEndTime = 0;

	nextSecondsInterval = 0;
	secondsLerpStart = 0;
	secondsLerpEnd = 0;
	secondsState = 0;
	secondsAngleStart = 0;
	secondsAngleEnd = 0;

	offsetTime = spawnArgs.GetInt("offset", "0") * OFFSET_MULTIPLIER;

	BecomeActive( TH_THINK );
}

void idClock::Event_setTime( int time )
{
	//this->startTime = gameLocal.time - (time * 1000);

	offsetTime = time * 1000;
}

void idClock::Event_reset( void )
{
	lastMinuteCount = 0;
	startAngle = 0;
	endAngle = 0;
	minuteStartTime = 0;
	minuteEndTime = 0;

	nextSecondsInterval = 0;
	secondsLerpStart = 0;
	secondsLerpEnd = 0;
	secondsState = 0;
	secondsAngleStart = 0;
	secondsAngleEnd = 0;

	offsetTime = spawnArgs.GetInt("offset", "0") * OFFSET_MULTIPLIER;

	this->startTime = gameLocal.time;


	SetHandAngle("minute", 0);
	SetHandAngle("hour", 0);
	SetHandAngle("second", 0);
}

void idClock::SetHandAngle( const char *jointName, float angle )
{
	idMat3 bodyAxis;
	idVec3 offset;
	jointHandle_t originJoint = animator.GetJointHandle( "origin" );
	animator.GetJointTransform( originJoint, gameLocal.time, offset, bodyAxis );

	jointHandle_t	secondJoint;
	idRotation		secondRotation;
	secondJoint = animator.GetJointHandle( jointName );
	secondRotation.SetVec( bodyAxis[0] );
	secondRotation.SetAngle( angle );
	animator.SetJointAxis(secondJoint, JOINTMOD_WORLD, secondRotation.ToMat3());

}

void idClock::Think( void )
{
	int curTime  = (gameLocal.time - startTime) + offsetTime;

	//common->Printf("curtime %d\n", curTime);

	if (!digital)
	{
		//SET SECONDS.
		int totalMinutes = (int)(curTime / 1000.0f);
		int secondsFraction = curTime - totalMinutes;
		float secondsPercentage = secondsFraction / 1000.0f;
		float secondsAngle = secondsPercentage * 6.0f;


		//gameLocal.GetLocalPlayer()->DebugMessage(va("%d", secondsFraction));

		//secondsState 0 = idle. 1 = lerping.

		if (gameLocal.time > nextSecondsInterval)
		{
			secondsState = 1;
			nextSecondsInterval = gameLocal.time + 1000;
			//SetHandAngle("second", secondsAngle);

			secondsAngleStart = secondsAngleEnd;
			secondsAngleEnd = secondsAngle;
			
			secondsLerpStart = gameLocal.time;
			secondsLerpEnd = gameLocal.time + 100;
	
			StartSound( "snd_tick", SND_CHANNEL_ANY, 0, false, NULL );
		}

		if (secondsState == 1)
		{
			float lerp;
			float currentTime = gameLocal.time;
			float lookTimeMax = this->secondsLerpEnd - this->secondsLerpStart;	
			currentTime -= this->secondsLerpStart;
			lerp = currentTime / lookTimeMax;

			if (lerp >= 1)
			{
				lerp = 1;
				secondsState = 0;
			}

			SetHandAngle("second", idMath::Lerp(secondsAngleStart, secondsAngleEnd, lerp));
		}
		
		
		


		


		//SET MINUTES. (discrete movement)
		int minuteCount = totalMinutes / 60;
		//gameLocal.GetLocalPlayer()->DebugMessage(va("time %d", minuteCount));
		if (minuteCount > 60)
		{
			int i;
			for (i = 0; i < 999; i++)
			{
				if (minuteCount > 60)
					minuteCount -= 60;
				else
					break;
			}
		}

		if (minuteCount != lastMinuteCount)
		{
			minuteStartTime = gameLocal.time;
			minuteEndTime = gameLocal.time + 300;

			startAngle = endAngle;
			endAngle = 360.0f * (minuteCount / 60.0f);
			lastMinuteCount = minuteCount;

			StartSound( "snd_minute", SND_CHANNEL_ANY, 0, false, NULL );



			//hack. force the second hand to reset.

			secondsAngleStart = secondsAngle;
			secondsAngleEnd = secondsAngle;
			
			secondsLerpStart = gameLocal.time;
			secondsLerpEnd = gameLocal.time + 50;
			secondsState = 1;

			nextSecondsInterval = gameLocal.time + 1000;

		}


		if (minuteEndTime >= gameLocal.time)
		{
			float lerp;
			float currentTime = gameLocal.time;
			float lookTimeMax = this->minuteEndTime - this->minuteStartTime;	
			currentTime -= this->minuteStartTime;
			lerp = currentTime / lookTimeMax;

			//gameLocal.Printf(va("time %f", lerp));

			if (lerp > 1)
				lerp = 1;

			SetHandAngle("minute", idMath::Lerp(startAngle, endAngle, lerp));
		}


		//SET HOURS.
		float hourAngle = secondsPercentage * 0.001666666666666f;
		SetHandAngle("hour", hourAngle);
	}
	else
	{
		//digital.


		int fullHours = curTime / 3600000;

		int fullMinutes = curTime / 60000;
		if (fullHours > 0)
		{
			fullMinutes -= fullHours * 60;
		}		

		int seconds = curTime / 1000;
		if (fullMinutes > 0)
		{
			seconds -= fullMinutes * 60;
		}

		if (fullHours > 0)
		{
			seconds -= (fullHours * 60) * 60;
		}

		this->renderEntity.gui[0]->SetStateString("gui_parm0", va("%02d", fullHours));
		this->renderEntity.gui[0]->SetStateString("gui_parm1", va("%02d", fullMinutes));
		this->renderEntity.gui[0]->SetStateString("gui_parm2", va("%02d", seconds));
	}
	
	//idAnimatedEntity::Think();
	idAnimatedEntity::Present();
}