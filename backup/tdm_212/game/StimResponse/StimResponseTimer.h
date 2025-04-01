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
#ifndef SR_RESPONSETIMER__H
#define SR_RESPONSETIMER__H

/*
#define GetHours(x)		((x >> 24) & 0xff)
#define GetMinutes(x)	((x >> 16) & 0xff)
#define GetSeconds(x)	((x >> 8) & 0xff)
#define GetMSeconds(x)	(x & 0xff)

#define SetHours(x)		(x << 24)
#define SetMinutes(x)	(x << 16)
#define SetSeconds(x)	(x << 8)
#define SetMSeconds(x)	(x)
*/

#define TIMER_UNDEFINED		-1

typedef struct {
	signed char Flags;
	signed char Hour;
	signed char Minute;
	signed char Second;
	signed short Millisecond;
} TimerValue;

/**
 * CStimResponseTimer handles all timing aspects of stimuli.
 * Each of the values as a copy, which is used to store the actual value at runtime (<X>Val).
 * The original value is used for initialisation but is never changed during runtime
 * as only the Val counterparts will be used to track the current state.
 *
 * m_Timer is the actual timer that determines how long it takes before the stim
 * is getting enabled. After this timer is expired, the stim will be activated. 
 * If a duration timer is also set, it will fire as long as the duration lasts and will
 * be disabled this duration time. If no duration is specified, the timer will fire 
 * exactly once, and then reset itself if it is a restartable timer (SRTT_RELOAD) and 
 * the cycle will begin again. If a reload value is set, it will only be restarted 
 * until the reloadcounter has been depleted (0). If the reloadvalue is -1, it means 
 * that it will reset itself infinitely (or until manually stopped).
 */
class CStimResponseTimer {

	friend class CStim;
	friend class CStimResponseCollection;

public:
	typedef enum {
		SRTT_SINGLESHOT,			// Stimuls can be used exactly once
		SRTT_RELOAD,				// Stimulus can be reused
		SRTT_DEFAULT
	} TimerType;

	typedef enum {
		SRTS_DISABLED,				// Timer is not active
		SRTS_TRIGGERED,				// Stimuls just became active
		SRTS_RUNNING,				// Stimulus is progressing (only relevant for durations)
		SRTS_EXPIRED,				// Stimulus has been expired (can be reloaded if reload is set)
		SRTS_DEFAULT
	} TimerState;

public:
	void Save(idSaveGame *savefile) const;
	void Restore(idRestoreGame *savefile);

	/**
	 * If the stim contains information for a timed event, this function parses the string
	 * and returns a timervalue.
	 *
	 * The timer is initialized by a string on the entity which reads like this:
	 * HH:MM:SS
	 *
	 * HH are the hours in 24 hour format 0-23.
	 * MM are the minutes 0-59.
	 * SS are the seconds 0-59.
	 */
	static TimerValue ParseTimeString(idStr &s);

	/**
	 * Set ticks defines the number of ticks, that this timer should 
	 * use to define a second. This value will differ on different machine
	 * depending on their speed. It would also be possible to use this
	 * value to speed up or delay a timer by adjusting the value accordingly.
	 */
	virtual void SetTicks(double const &TicksPerSecond);

	/**
	 * SetTimer loads the timer with the intended time, that the timer should
	 * run. This time is NOT the actual daytime, it is actually the intervall
	 * that the timer should last.
	 */
	virtual void SetTimer(int Hour, int Minute, int Seconds, int Milisecond);

	/**
	 * Define how often the timer should reload before it expires. After the
	 * last reload is done (reload has reached zero), the timer will stop itself.
	 */
	virtual void SetReload(int Reload);

	/**
	 * Start the timer again, after it has been stopped. If the timer 
	 * has been stopped before, but has not yet expired, it will
	 * just continue where it stopped which is different to Restart().
	 */
	virtual void Start(unsigned int sysTicks);

	/**
	 * Stop will simply stop the timer without any changes
	 */
	virtual void Stop(void);

	/**
	 * Restart will restart the timer with the next cycle. If a reload
	 * is specified it will be decreased, which means that if no more
	 * reloads are possible, restart will have no effect.
	 */
	virtual void Restart(unsigned int sysTicks);
	
	/**
	 * Reset will reset the timer. This means that also the reload
	 * value will be reset as well.
	 */
	virtual void Reset(void);

	void SetState(TimerState State);
	inline TimerState GetState(void) { return m_State; };

	/**
	 * The timer returns FALSE if the timer is inactive or
	 * the time to fire has not yet come.
	 */
	virtual bool Tick(unsigned int sysTicks);

	/**
	 * Returns true if the timer had expired at least once since the 
	 * last tick. It might have been restarted in the meantime, so checking
	 * the current state might not help, but this function will always return
	 * true in such a case.
	 */
	virtual bool WasExpired(void);

	/**
	 * Fills the timevalue calculated from the tick counter.
	 */
	void MakeTime(TimerValue &Time, unsigned int sysTicks);

public:
	CStimResponseTimer();
	virtual ~CStimResponseTimer(void);

protected:
	unsigned int	m_LastTick;
	unsigned int	m_Ticker;
	unsigned int	m_TicksPerMilliSecond;

	bool			m_Fired;

	/**
	* The Timer type specifies if this is a single-use timer or a
	* "reloadable" timer.
	*/
	TimerType		m_Type;
	TimerState		m_State;

	/**
	 * How often can the stimulus be reused. -1 = unlimited
	 */
	int				m_Reload;
	int				m_ReloadVal;

	/**
	 * Timer
	 */
	TimerValue		m_Timer;
	TimerValue		m_TimerVal;
};

#endif /* SR_RESPONSETIMER__H */
