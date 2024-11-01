#include "PrecompCommon.h"
#include "Timer.h"

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOWINRES
#define NOWINRES
#endif
#ifndef NOSERVICE
#define NOSERVICE
#endif
#ifndef NOMCX
#define NOMCX
#endif
#ifndef NOIME
#define NOIME
#endif
#include <windows.h>
#else
#include <time.h>
#endif

bool initialized = false; // a bit lame but whatever

#ifdef WIN32
	LARGE_INTEGER iCounterFrequency;
#endif

void Init()
{
#ifdef WIN32
	QueryPerformanceFrequency(&iCounterFrequency);
#endif
	initialized = true;
}

Timer::Timer()
{
	if(!initialized)
		Init();

#ifdef WIN32
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	mLastTimer = li.QuadPart;
#else
	mLastTimer = clock();
#endif
}

void Timer::Reset()
{
#ifdef WIN32
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	mLastTimer = li.QuadPart;
#else
	mLastTimer = clock();
#endif
}

/*
Function: GetElapsedSeconds
Description:		
Returns: 
Returns the elapsed seconds since the timer was last reset.
*/
double Timer::GetElapsedSeconds()
{
#ifdef WIN32
	// Get the current count
	LARGE_INTEGER iCurrent, iLastCount;
	iLastCount.QuadPart = mLastTimer;
	QueryPerformanceCounter(&iCurrent);

	return double((iCurrent.QuadPart - iLastCount.QuadPart) /
		double(iCounterFrequency.QuadPart));
#else
	return static_cast<double>(clock() - mLastTimer) / CLOCKS_PER_SEC;
#endif
}

//////////////////////////////////////////////////////////////////////////

GameTimer::GameTimer() 
	: m_TriggerTime(0)
{
}

void GameTimer::Delay(float _seconds)
{
	m_TriggerTime = IGame::GetTime() + Utils::SecondsToMilliseconds(_seconds);
}

void GameTimer::DelayRandom(float _min, float _max)
{
	m_TriggerTime = IGame::GetTime() + Utils::SecondsToMilliseconds(Mathf::IntervalRandom(_min,_max));
}

void GameTimer::Delay(int _ms)
{
	m_TriggerTime = IGame::GetTime() + _ms;
}

void GameTimer::DelayRandom(int _min, int _max)
{
	m_TriggerTime = IGame::GetTime() + Mathf::IntervalRandomInt(_min,_max);
}

bool GameTimer::IsExpired() const
{
	return IGame::GetTime() >= m_TriggerTime;
}
