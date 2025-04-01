/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#pragma hdrstop



#include "StimResponseTimer.h"

/********************************************************************/
/*                 CStimResponseTimer                               */
/********************************************************************/
CStimResponseTimer::CStimResponseTimer()
{
	m_Type = SRTT_SINGLESHOT;
	m_State = SRTS_DISABLED;
	m_Reload = 0;
	m_ReloadVal = 0;
	m_Timer.Flags = TIMER_UNDEFINED;
	m_TimerVal.Flags = TIMER_UNDEFINED;
	m_LastTick = 0;
	m_Ticker = 0;
	m_TicksPerMilliSecond = 0;
	m_Fired = false;
}

CStimResponseTimer::~CStimResponseTimer(void)
{
}

static void Save(idSaveGame *savefile, const TimerValue &val) {
	savefile->WriteSignedChar(val.Flags);
	savefile->WriteSignedChar(val.Hour);
	savefile->WriteSignedChar(val.Minute);
	savefile->WriteSignedChar(val.Second);
	savefile->WriteInt(val.Millisecond);
}
static void Restore(idRestoreGame *savefile, TimerValue &val) {
	savefile->ReadSignedChar(val.Flags);
	savefile->ReadSignedChar(val.Hour);
	savefile->ReadSignedChar(val.Minute);
	savefile->ReadSignedChar(val.Second);
	int tmp;
	savefile->ReadInt(tmp);
	val.Millisecond = (short)tmp;
}

void CStimResponseTimer::Save(idSaveGame *savefile) const
{
	savefile->WriteFloat(static_cast<float>(m_LastTick));
	savefile->WriteFloat(static_cast<float>(m_Ticker));
	savefile->WriteFloat(static_cast<float>(m_TicksPerMilliSecond));
	savefile->WriteBool(m_Fired);

	savefile->WriteInt(static_cast<int>(m_Type));
	savefile->WriteInt(static_cast<int>(m_State));

	savefile->WriteInt(m_Reload);
	savefile->WriteInt(m_ReloadVal);

	::Save(savefile, m_Timer);
	::Save(savefile, m_TimerVal);
}

void CStimResponseTimer::Restore(idRestoreGame *savefile)
{
	float tempFloat;
	int tempInt;

	savefile->ReadFloat(tempFloat);
	m_LastTick = static_cast<int>(tempFloat);

	savefile->ReadFloat(tempFloat);
	m_Ticker = static_cast<int>(tempFloat);

	savefile->ReadFloat(tempFloat);
	m_TicksPerMilliSecond = static_cast<int>(tempFloat);

	savefile->ReadBool(m_Fired);

	savefile->ReadInt(tempInt);
	m_Type = static_cast<TimerType>(tempInt);

	savefile->ReadInt(tempInt);
	m_State = static_cast<TimerState>(tempInt);

	savefile->ReadInt(m_Reload);
	savefile->ReadInt(m_ReloadVal);

	::Restore(savefile, m_Timer);
	::Restore(savefile, m_TimerVal);
}

void CStimResponseTimer::SetTicks(double const &TicksPerSecond)
{
	m_TicksPerMilliSecond = static_cast<unsigned int>(TicksPerSecond);
}

TimerValue CStimResponseTimer::ParseTimeString(idStr &str)
{
	TimerValue v;
	int h, m, s, ms;
	idStr source = str;

	v.Flags = TIMER_UNDEFINED;

	if(str.Length() == 0)
		goto Quit;

	h = m = s = ms = 0;

	// Get the first few characters that define the hours
	h = atoi( source.Left(source.Find(":")).c_str() );
	
	// Strip the first few numbers plus the colon from the source string
	source = source.Right(source.Length() - source.Find(":") - 1);
	
	// Parse the minutes
	m = atoi( source.Left(source.Find(":")).c_str() );
	if (!(m >= 0 && m <= 59))
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid minute string [%s]\r", str.c_str());
		goto Quit;
	}
	// Strip the first few numbers plus the colon from the source string
	source = source.Right(source.Length() - source.Find(":") - 1);
	
	// Parse the seconds
	s = atoi( source.Left(source.Find(":")).c_str() );
	if (!(s >= 0 && s <= 59))
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid second string [%s]\r", str.c_str());
		goto Quit;
	}

	// Parse the milliseconds, this is the remaining part of the string
	ms = atoi( source.Right(source.Length() - source.Find(":") - 1).c_str() );
	if (!(ms >= 0 && ms <= 999))
	{
		DM_LOG(LC_STIM_RESPONSE, LT_ERROR)LOGSTRING("Invalid millisecond string [%s]\r", str.c_str());
		goto Quit;
	}

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Parsed timer string: [%s] to %d:%d:%d:%d\r", str.c_str(), h, m, s, ms);

	v.Hour = h;
	v.Minute = m;
	v.Second = s;
	v.Millisecond = ms;

Quit:
	return v;
}

void CStimResponseTimer::SetReload(int Reload)
{
	m_Reload = Reload;
	m_ReloadVal = Reload;
}

void CStimResponseTimer::SetTimer(int Hour, int Minute, int Second, int Millisecond)
{
//	m_Timer = SetHours(Hour) |  SetMinutes(Minute) | SetSeconds(Seconds) | SetMSeconds(Milisecond);
	m_TimerVal.Hour = Hour;
	m_TimerVal.Minute = Minute;
	m_TimerVal.Second = Second;
	m_TimerVal.Millisecond = Millisecond;
	memset(&m_Timer, 0, sizeof(TimerValue));
}

void CStimResponseTimer::Stop(void)
{
	SetState(SRTS_DISABLED);
}

void CStimResponseTimer::Start(unsigned int sysTicks)
{
	m_LastTick = sysTicks;
	SetState(SRTS_RUNNING);
}

void CStimResponseTimer::Restart(unsigned int sysTicks)
{
	// Switch to the next timer cycle if reloading is still possible or 
	// reloading is ignored.
	m_Ticker = sysTicks;

	if(m_Reload > 0 || m_Reload == -1)
	{
		memset(&m_Timer, 0, sizeof(TimerValue));
		if(m_Reload != -1)
			m_Reload--;
		Start(sysTicks);
	}
	else
		Stop();
}

void CStimResponseTimer::Reset(void)
{
	memset(&m_Timer, 0, sizeof(TimerValue));
	m_Reload = m_ReloadVal;
}

void CStimResponseTimer::SetState(TimerState State)
{
	m_State = State;
}

bool CStimResponseTimer::Tick(unsigned int sysTicks)
{
	bool returnValue = false;
	unsigned int ticksPassed;
	double msPassed;
	
	if(m_State != SRTS_RUNNING)
		goto Quit;

	// We don't really care for an overrun of the ticckcounter. If 
	// it really happens, the worst thing would be that a particular
	// timer object would take longer to complete, because for this
	// one cycle, the tick would become negative and thus would subtract
	// the value instead of adding it. In the next cylce, everything 
	// should work again though, since we always store the current
	// value to remember it for the next cycle.
	/* unsigned int */ ticksPassed = sysTicks - m_LastTick;
	
	// If the overrun happened, we just ignore this tick. It's the easiest
	// thing to do and the safest.
	if (ticksPassed < 0.0)
		goto Quit;

	m_Ticker += ticksPassed;

	// Calculate the number of milliseconds that have passed since the last visit
	/* double */ msPassed = floor(static_cast<double>(m_Ticker) / m_TicksPerMilliSecond);
	
	// The remaining ticks are what's left after the division (modulo)
	m_Ticker %= m_TicksPerMilliSecond;

	// Increase the hours/minutes/seconds/milliseconds
	m_Timer.Millisecond += static_cast<short int>(msPassed);

	if (m_Timer.Millisecond > 999)
	{
		// Increase the seconds
		m_Timer.Second += static_cast<signed char>(floor(m_Timer.Millisecond / 1000.0));
		m_Timer.Millisecond %= 1000;

		m_Timer.Minute += static_cast<signed char>(floor(m_Timer.Second / 60.0));
		m_Timer.Second %= 60;

		m_Timer.Hour += static_cast<signed char>(floor(m_Timer.Minute / 60.0));
		m_Timer.Minute %= 60;
	}

	// Now check if the timer already expired.
	if (m_Timer.Hour >= m_TimerVal.Hour && m_Timer.Minute >= m_TimerVal.Minute && 
		m_Timer.Second >= m_TimerVal.Second && m_Timer.Millisecond >= m_TimerVal.Millisecond) 
	{
		m_Fired = true;
		returnValue = true;
		if(m_Type == SRTT_SINGLESHOT)
			Stop();
		else
			Restart(sysTicks);
	}

Quit:
	m_LastTick = sysTicks;

	return returnValue;
}

void CStimResponseTimer::MakeTime(TimerValue &t, unsigned int Ticks)
{
	double msPassed = floor(static_cast<double>(Ticks) / m_TicksPerMilliSecond);

	memset(&t, 0, sizeof(TimerValue));
	t.Millisecond = static_cast<short int>(msPassed);

	t.Second += static_cast<signed char>(floor(t.Millisecond / 1000.0));
	t.Millisecond %= 1000;

	t.Minute += static_cast<signed char>(floor(t.Second / 60.0));
	t.Second %= 60;

	t.Hour += static_cast<signed char>(floor(t.Minute / 60.0));
	t.Minute %= 60;
}

bool CStimResponseTimer::WasExpired(void)
{
	bool rc = m_Fired;
	m_Fired = false;
	return rc;
}
