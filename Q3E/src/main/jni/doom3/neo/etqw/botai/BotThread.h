// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOTTHREAD_H__
#define __BOTTHREAD_H__

/*
===============================================================================

   idBotThread

   The bot thread is setup such that the bot AI can never think for more
   frames than there are game frames. Furthermore the bot AI cannot
   think for less frames than one fourth the number of game frames.
   In other words the AI runs asynchronously but it can run at most
   at 30 Hz (USERCMD_HZ) and if not enough CPU is available it can
   seamlessly scale down to 6 Hz. If the bot AI runs fast the bot
   thread will wait for the game thread. If the bot AI runs really
   slow such that it cannot maintain 8 Hz the game code will wait
   for the bot thread to catch up.

===============================================================================
*/

#define MAX_FRAME_TIMES		16

class idBotAI;

class idBotThread : public sdThreadProcess {
public:
							idBotThread();
							~idBotThread();

	void					StartThread();
	void					StopThread();

	virtual unsigned int	Run( void* parm );
	virtual void			Stop();

	void					WaitForGameThread() { gameSignal.SignalAndWait( botSignal ); }
	void					SignalGameThread() { botSignal.Set(); }

	void					WaitForBotThread() { botSignal.Wait(); }
	void					SignalBotThread() { gameSignal.Set(); }

	void					Lock() { lock.Acquire(); }
	void					UnLock() { lock.Release(); }

	int						GetLastGameFrameNum() const { return lastGameFrameNum; }
	int						GetFrameRate() const;
	bool					IsActive() const { return isActive; }
	bool					IsWaiting() const { return isWaiting; }

private:
	sdThread *				thread;
	sdSignal				gameSignal;
	sdSignal				botSignal;
	sdLock					lock;

	int						lastGameFrameNum;
	bool					isWaiting;
	bool					isActive;

	int						frameTimes[MAX_FRAME_TIMES];
	int						numFrameTimes;
};

extern idBotThread *		botThread;

#endif /* !__BOTTHREAD_H__ */
