// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 

#include "BotThread.h"
#include "BotThreadData.h"

idBotThread botThreadLocal;
idBotThread* botThread = &botThreadLocal;

/*
=====================
idBotThread::idBotThread
=====================
*/
idBotThread::idBotThread() :
	thread( NULL ),
	lastGameFrameNum( -1 ),
	isWaiting( false ),
	isActive( false ),
	numFrameTimes( 0 ) {
}

/*
=====================
idBotThread::~idBotThread
=====================
*/
idBotThread::~idBotThread() {
}

/*
=====================
idBotThread::StartThread
=====================
*/
void idBotThread::StartThread() {
	if ( thread != NULL ) {
		return;
	}
#ifdef _XENON
	thread = new sdThread( this, THREAD_NORMAL, XENON_STACKSIZE_BOT );
#else
	thread = new sdThread( this, THREAD_LOWEST );
#endif
	thread->SetName( "BotThread" );

#ifdef _XENON
	thread->SetProcessor( XENON_THREADCORE_BOT );
#endif

	if ( !thread->Start() ) {
		common->Error( "idBotThread::StartThread : failed to start thread" );
	}
}

/*
=====================
idBotThread::StopThread
=====================
*/
void idBotThread::StopThread() {
	if ( thread != NULL ) {
		thread->Stop();
		thread->Join();
		thread->Destroy();
		thread = NULL;
	}
}

/*
=====================
idBotThread::Run
=====================
*/
unsigned int idBotThread::Run( void *parm ) {
	int previousTime = 0;

	isActive = true;

	lastGameFrameNum = gameLocal.GetFrameNum();

	gameLocal.clip.AllocThread();

	while ( !Terminating() ) {

		// let the game know another bot AI frame has started
		SignalGameThread();

		// never run more bot think frames than game frames
		while ( lastGameFrameNum >= gameLocal.GetFrameNum() - botThreadData.GetThreadMinFrameDelay() && !Terminating() ) {
			isWaiting = true;
			WaitForGameThread();
			isWaiting = false;
		}
		lastGameFrameNum = gameLocal.GetFrameNum();

		// delete any bots for real
		botThreadData.DeleteBots();

		// let the bots think
		if ( botThreadData.IsThreadingEnabled() ) {
			botThreadData.RunFrame();
		}

		// decrease deleted clip model reference counts
		botThreadData.clip->ThreadDeleteClipModels();

		// update bot think frame times
		int t = sys->Milliseconds();
		int delta = t - previousTime;
		frameTimes[numFrameTimes++ % MAX_FRAME_TIMES] = delta;
		previousTime = t;
	}

	gameLocal.clip.FreeThread();

	isActive = false;

	return 0;
}

/*
=====================
idBotThread::Stop
=====================
*/
void idBotThread::Stop() {
	sdThreadProcess::Stop();
	// need to signal the thread so it wakes up and terminates
	gameSignal.Set();
	botSignal.Set();
}

/*
=====================
idBotThread::GetFrameRate
=====================
*/
int idBotThread::GetFrameRate() const {
	int i, total, fps;

	if ( numFrameTimes < MAX_FRAME_TIMES ) {
		return 0;
	}

	// average multiple frames together to smooth changes out a bit
	total = 0;

	for ( i = 0; i < MAX_FRAME_TIMES; i++ ) {
		total += frameTimes[i];
	}
	if ( total == 0 ) {
		total = 1;
	}
	fps = 2000 * MAX_FRAME_TIMES / total;
	fps = ( fps + 1 ) / 2;

	return fps;
}
