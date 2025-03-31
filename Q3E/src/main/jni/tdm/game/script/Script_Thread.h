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

#ifndef __SCRIPT_THREAD_H__
#define __SCRIPT_THREAD_H__

extern const idEventDef EV_Thread_Execute;
extern const idEventDef EV_Thread_SetCallback;
extern const idEventDef EV_Thread_SetRenderCallback;
extern const idEventDef EV_Thread_Wait;
extern const idEventDef EV_Thread_WaitFrame;

class idThread : public idClass {
private:
	static idThread				*currentThread;

	idThread					*waitingForThread;
	int							waitingFor;
	int							waitingUntil;
	idList<idThread*>			threadsWaitingForThis;		//stgatilov: reverse to waitingForThread, empty in most cases

	idInterpreter				interpreter;

	idDict						spawnArgs;
								
	int 						threadNum;		//stgatilov: assigned sequentally (similar to idEntity::spawnIdx)
	int							threadPos;		//stgatilov: index in threadList (similar to idEntity::entityNum)
	idStr 						threadName;

	int							lastExecuteTime;
	int							creationTime;

	bool						manualControl;

	static int					threadIndex;
	static idList<idThread *>	threadList;
	static idList<int>			posFreeList;	//stgatilov: indices of NULLs in threadList
	static idHashIndex			threadNumsHash;

	static trace_t				trace;

	void						Init( void );
	void						Pause( void );

	void						Event_Execute( void );
	void						Event_SetThreadName( const char *name );

	//
	// script callable Events
	//
	void						Event_TerminateThread( int num );
	void						Event_Pause( void );
	void						Event_Wait( float time );
	void						Event_WaitFrame( void );
	void						Event_WaitFor( idEntity *ent );
	void						Event_WaitForThread( int num );
	void						Event_WaitForRender( idEntity *ent );
	void						Event_Print( const char *text );
	void						Event_PrintLn( const char *text );
	void						Event_Say( const char *text );
	void						Event_Assert( float value );
	void						Event_Trigger( idEntity *ent );
	void						Event_SetCvar( const char *name, const char *value ) const;
	void						Event_GetCvar( const char *name ) const;
	void						Event_GetCvarF( const char *name ) const;
	void						Event_Random( float range ) const;
	void						Event_GetTime( void );
	void						Event_KillThread( const char *name );
	void						Event_GetEntity( const char *name );
	void						Event_Spawn( const char *classname );
	void						Event_CopySpawnArgs( idEntity *ent );
	void						Event_SetSpawnArg( const char *key, const char *value );
	void						Event_SpawnString( const char *key, const char *defaultvalue );
	void						Event_SpawnFloat( const char *key, float defaultvalue );
	void						Event_SpawnVector( const char *key, idVec3 &defaultvalue );
	void						Event_ClearPersistentArgs( void );
	void 						Event_SetPersistentArg( const char *key, const char *value );
	void 						Event_GetPersistentString( const char *key );
	void 						Event_GetPersistentFloat( const char *key );
	void 						Event_GetPersistentVector( const char *key );

	void						Event_GetCurrentMissionNum();
	void						Event_GetTDMVersion() const;

	void						Event_AngRotate( idAngles &ang1, idAngles& ang2 );
	void						Event_AngToForward( idAngles &ang );
	void						Event_AngToRight( idAngles &ang );
	void						Event_AngToUp( idAngles &ang );
	void						Event_GetSine( const float angle );
	void						Event_GetASine(const float sin); // grayman #4882
	void						Event_GetCosine( const float angle );
	void						Event_GetACosine(const float cos); // grayman #4882
	void						Event_GetLog( const float x );
	void						Event_GetPow( const float x, const float y );
	// largest integer (-1.5 => -1, 1.5 => 2, 1.3 => 2)
	void						Event_GetCeil( const float x );
	// smallest integer (-1.5 => -2, 1.5 => 1, 1.3 => 1)
	void						Event_GetFloor( const float x );
	void						Event_GetMin( const float x, const float y );
	void						Event_GetMax( const float x, const float y );
	void						Event_GetSquareRoot( float theSquare );
	void						Event_VecNormalize( idVec3 &vec );
	void						Event_VecLength( idVec3 &vec );
	void						Event_VecDotProduct( idVec3 &vec1, idVec3 &vec2 );
	void						Event_VecCrossProduct( idVec3 &vec1, idVec3 &vec2 );
	void						Event_VecToAngles( idVec3 &vec );
	void						Event_VecRotate( idVec3 &vector, idAngles &angles );
	void						Event_GetInterceptTime( idVec3 &velTarget, float speedInterceptor, idVec3 &posTarget, idVec3 &posInterceptor );
	void						Event_OnSignal( int signal, idEntity *ent, const char *func );
	void						Event_ClearSignalThread( int signal, idEntity *ent );
	void						Event_SetCamera( idEntity *ent );
	void						Event_FirstPerson( void );
	void						Event_Trace( const idVec3 &start, const idVec3 &end, const idVec3 &mins, const idVec3 &maxs, int contents_mask, idEntity *passEntity );
	void						Event_TracePoint( const idVec3 &start, const idVec3 &end, int contents_mask, idEntity *passEntity );
	void						Event_GetTraceFraction( void );
	void						Event_GetTraceEndPos( void );
	void						Event_GetTraceNormal( void );
	void						Event_GetTraceEntity( void );
	void						Event_GetTraceJoint( void );
	void						Event_GetTraceBody( void );
	void						Event_GetTraceSurfType( void );
	void						Event_FadeIn( idVec3 &color, float time );
	void						Event_FadeOut( idVec3 &color, float time );
	void						Event_FadeTo( idVec3 &color, float alpha, float time );
	void						Event_SetShaderParm( int parmnum, float value );
	void						Event_StartMusic( const char *name );
	void						Event_Warning( const char *text );
	void						Event_Error( const char *text );
	void 						Event_StrLen( const char *string );
	void 						Event_StrLeft( const char *string, int num );
	void 						Event_StrRight( const char *string, int num );
	void 						Event_StrSkip( const char *string, int num );
	void 						Event_StrMid( const char *string, int start, int num );
	// Tels #3854: Replace the given string with "". Example: StrRemove("abba","bb") results in "aa".
	void 						Event_StrRemove( const char *string, const char *remove );	
	// Tels #3854: Replace the given string with X. Example: StrReplace("abba","bb","cc") results in "acca"
	void 						Event_StrReplace( const char *string, const char *remove, const char *replace );
	// Tels #3854: Return the position of the substring, or -1 if not found
	void 						Event_StrFind( const char *string, const char *find, const int mcasesensitive, const int mstart, const int mend );
	void						Event_StrToFloat( const char *string );
	void						Event_StrToInt( const char *string );
	void						Event_RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignore, const char *damageDefName, float dmgPower );
	void						Event_IsClient( void );
	void 						Event_IsMultiplayer( void );
	void 						Event_GetFrameTime( void );
	void 						Event_GetTicsPerSecond( void );
	void						Event_CacheSoundShader( const char *soundName );
	void						Event_PointIsInBounds( const idVec3 &point, const idVec3 &mins, const idVec3 &maxs );
	void						Event_GetLocationPoint( const idVec3 &point );	// get location of a point rather than an entity
	void						Event_DebugLine( const idVec3 &color, const idVec3 &start, const idVec3 &end, const float lifetime );
	void						Event_DebugArrow( const idVec3 &color, const idVec3 &start, const idVec3 &end, const int size, const float lifetime );
	void						Event_DebugCircle( const idVec3 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const float lifetime );
	void						Event_DebugBounds( const idVec3 &color, const idVec3 &mins, const idVec3 &maxs, const float lifetime );
	void						Event_DrawText( const char *text, const idVec3 &origin, float scale, const idVec3 &color, const int align, const float lifetime );
	void						Event_InfluenceActive( void );

	/**
	* greebo: Tests if a point is in a liquid.
	*
	* @ignoreEntity: This excludes an entity from the clip test.
	* @returns: returns TRUE to the calling thread if the point is in a liquid
	*/
	void						Event_PointInLiquid( const idVec3 &point, idEntity* ignoreEntity );

	// Emits the string to the session command variable in gameLocal.
	void						Event_SessionCommand(const char* cmd);

	// stgatilov #5369: Save condump to FM-local file (triggered by script).
	// if "startline" is not empty, and it is present in console output, then everything before the last its occurence is removed from the file
	void						Event_SaveConDump(const char *filename, const char *startline);

	/**
	* Tels: #3193 - translate a string template into the current language.
	*/
	void						Event_Translate(const char* input);

	/**
	* The following events are a frontend for the AI relationship
	* manager (stored in game_local).
	* See CRelations definition for descriptions of functions called
	**/
	void						Event_GetRelation( int team1, int team2 );
	void						Event_SetRelation( int team1, int team2, int val );
	void						Event_OffsetRelation( int team1, int team2, int offset );

	/**
	* TDM Soundprop Events:
	* Set or get the acoustical loss for a portal with a given handle.
	* Handle must be greater than zero and less than the number of portals in the map.
	* grayman #3042 - allow access to AI- and Player-specific loss
	**/
	void						Event_SetPortAISoundLoss( int handle, float value );
	void						Event_SetPortPlayerSoundLoss( int handle, float value );
	void						Event_GetPortAISoundLoss( int handle );
	void						Event_GetPortPlayerSoundLoss( int handle );
	
	// The scriptevent counterpart of DM_LOG
	void						Event_LogString(int logClass, int logType, const char* output);

	// The script interface for raising mission events, like readable callbacks
	void						Event_HandleMissionEvent(idEntity* entity, int eventType, const char* argument);
	
	void						Event_CanPlant( const idVec3 &traceStart, const idVec3 &traceEnd, idEntity *ignore, idEntity *vine ); // grayman #2787
	void						Event_GetMainAmbientLight();	// grayman #3132

	void						Event_GetDifficultyLevel();	// tels    #3271
	void						Event_GetDifficultyName( int level );                   // SteveL #3304: Add 2 scriptevents
	void						Event_GetMissionStatistic( const char* statisticName ); //               from Zbyl

	void						Event_GetNextEntity( const char* key, const char* value, const idEntity* lastMatch );	// SteveL #3802
	void						Event_EmitParticle( const char* particle, float startTime, float diversity, const idVec3& origin, const idVec3& angle ); // SteveL #3962
	void						Event_ProjectDecal( const idVec3& traceOrigin, const idVec3& traceEnd, idEntity* passEntity, const char* decal, float decalSize, float angle );

	void						Event_SetSecretsFound( float secrets);
	void						Event_SetSecretsTotal( float secrets);

	void						Event_CallFunctionsByWildcard( const char* functionNameWildcard );

public:							
								CLASS_PROTOTYPE( idThread );
								
								idThread();
								idThread( idEntity *self, const function_t *func );
								idThread( const function_t *func );
								idThread( idInterpreter *source, const function_t *func, int args );
								idThread( idInterpreter *source, idEntity *self, const function_t *func, int args );

	virtual						~idThread();

								// tells the thread manager not to delete this thread when it ends
	void						ManualDelete( void );

	// save games
	void						Save( idSaveGame *savefile ) const;				// archives object for save game file
	void						Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void						EnableDebugInfo( void ) { interpreter.debug = true; };
	void						DisableDebugInfo( void ) { interpreter.debug = false; };

	void						WaitMS( int time );
	void						WaitSec( float time );
	void						WaitFrame( void );
								
								// NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
	void						CallFunction(const function_t	*func, bool clearStack );

	bool						CallFunctionArgs(const function_t *func, bool clearStack, const char *fmt, ...);
	bool						CallFunctionArgsVN(const function_t *func, bool clearStack, const char *fmt, va_list args);

								// NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
	void						CallFunction( idEntity *obj, const function_t *func, bool clearStack );

	void						DisplayInfo();
	static idThread				*GetThread( int num );
	static void					ListThreads_f( const idCmdArgs &args );
	static void					Restart( void );
	static void					ObjectMoveDone( int threadnum, idEntity *obj );
								
	static idList<idThread*>&	GetThreads ( void );
	
	bool						IsDoneProcessing ( void );
	bool						IsDying			 ( void );	
								
	void						End( void );
	static void					KillThread( const char *name );
	static void					KillThread( int num );
	bool						Execute( void );
	void						ManualControl( void ) { manualControl = true; CancelEvents( &EV_Thread_Execute ); };
	void						DoneProcessing( void ) { interpreter.doneProcessing = true; };
	void						ContinueProcessing( void ) { interpreter.doneProcessing = false; };
	bool						ThreadDying( void ) { return interpreter.threadDying; };
	void						EndThread( void ) { interpreter.threadDying = true; };
	bool						IsWaiting( void );
	void						ClearWaitFor( void );
	bool						IsWaitingFor( idEntity *obj );
	void						ObjectMoveDone( idEntity *obj );
	void						ThreadCallback( idThread *thread );
	void						DelayedStart( int delay );
	bool						Start( void );
	idThread					*WaitingOnThread( void );
	void						SetThreadNum( int num );
	int 						GetThreadNum( void );
	void						SetThreadName( const char *name );
	const char					*GetThreadName( void );

	void						Error( const char *fmt, ... ) const id_attribute((format(printf,2,3)));

	void						Warning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));

								
	static idThread				*CurrentThread( void );
	static int					CurrentThreadNum( void );
	static bool					BeginMultiFrameEvent( idEntity *ent, const idEventDef *event );
	static void					EndMultiFrameEvent( idEntity *ent, const idEventDef *event );

	static void					ReturnString( const char *text );
	static void					ReturnFloat( const float value );
	static void					ReturnInt( const int value );
	static void					ReturnVector( idVec3 const &vec );
	static void					ReturnEntity( idEntity *ent );
};

/*
================
idThread::WaitingOnThread
================
*/
ID_INLINE idThread *idThread::WaitingOnThread( void ) {
	return waitingForThread;
}

/*
================
idThread::SetThreadNum
================
*/
ID_INLINE void idThread::SetThreadNum( int num ) {
	threadNum = num;
}

/*
================
idThread::GetThreadNum
================
*/
ID_INLINE int idThread::GetThreadNum( void ) {
	return threadNum;
}

/*
================
idThread::GetThreadName
================
*/
ID_INLINE const char *idThread::GetThreadName( void ) {
	return threadName.c_str();
}

/*
================
idThread::GetThreads
================
*/
ID_INLINE idList<idThread*>& idThread::GetThreads ( void ) {
	return threadList;
}	

/*
================
idThread::IsDoneProcessing
================
*/
ID_INLINE bool idThread::IsDoneProcessing ( void ) {
	return interpreter.doneProcessing;
}

/*
================
idThread::IsDying
================
*/
ID_INLINE bool idThread::IsDying ( void ) {
	return interpreter.threadDying;
}

#endif /* !__SCRIPT_THREAD_H__ */
