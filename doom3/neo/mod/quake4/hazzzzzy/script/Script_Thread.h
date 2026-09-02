
#ifndef __SCRIPT_THREAD_H__
#define __SCRIPT_THREAD_H__

extern const idEventDef EV_Thread_Execute;
extern const idEventDef EV_Thread_SetCallback;
extern const idEventDef EV_Thread_TerminateThread;
extern const idEventDef EV_Thread_Pause;
extern const idEventDef EV_Thread_Wait;
extern const idEventDef EV_Thread_WaitFrame;
extern const idEventDef EV_Thread_WaitFor;
extern const idEventDef EV_Thread_WaitForThread;
extern const idEventDef EV_Thread_Print;
extern const idEventDef EV_Thread_PrintLn;
extern const idEventDef EV_Thread_Say;
extern const idEventDef EV_Thread_Assert;
extern const idEventDef EV_Thread_Trigger;
extern const idEventDef EV_Thread_SetCvar;
extern const idEventDef EV_Thread_GetCvar;
extern const idEventDef EV_Thread_Random;
extern const idEventDef EV_Thread_GetTime;
extern const idEventDef EV_Thread_KillThread;
extern const idEventDef EV_Thread_SetThreadName;
extern const idEventDef EV_Thread_GetEntity;
extern const idEventDef EV_Thread_Spawn;
extern const idEventDef EV_Thread_SetSpawnArg;
extern const idEventDef EV_Thread_SpawnString;
extern const idEventDef EV_Thread_SpawnFloat;
extern const idEventDef EV_Thread_SpawnVector;
extern const idEventDef EV_Thread_AngToForward;
extern const idEventDef EV_Thread_AngToRight;
extern const idEventDef EV_Thread_AngToUp;
extern const idEventDef EV_Thread_Sine;
extern const idEventDef EV_Thread_Cosine;
extern const idEventDef EV_Thread_Normalize;
extern const idEventDef EV_Thread_VecLength;
extern const idEventDef EV_Thread_VecDotProduct;
extern const idEventDef EV_Thread_VecCrossProduct;
extern const idEventDef EV_Thread_OnSignal;
extern const idEventDef EV_Thread_ClearSignal;
extern const idEventDef EV_Thread_SetCamera;
extern const idEventDef EV_Thread_FirstPerson;
extern const idEventDef EV_Thread_TraceFraction;
extern const idEventDef EV_Thread_TracePos;
extern const idEventDef EV_Thread_FadeIn;
extern const idEventDef EV_Thread_FadeOut;
extern const idEventDef EV_Thread_FadeTo;
extern const idEventDef EV_Thread_Restart;
extern const idEventDef EV_Thread_SetMatSort;

// RAVEN BEGIN
// rjohnson: new blur special effect
extern const idEventDef EV_Thread_SetSpecialEffect;
extern const idEventDef EV_Thread_SetSpecialEffectParm;
// RAVEN END

class idThread : public idClass {
private:
	static idThread				*currentThread;

	idThread					*waitingForThread;
	int							waitingFor;
	int							waitingUntil;
	idInterpreter				interpreter;

	idDict						spawnArgs;
								
	int 						threadNum;
	idStr 						threadName;

	int							lastExecuteTime;
	int							creationTime;

	bool						manualControl;

	static int					threadIndex;
	static idList<idThread *>	threadList;

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
	void						Event_Print( const char *text );
	void						Event_PrintLn( const char *text );
	void						Event_Say( const char *text );
	void						Event_Assert( float value );
	void						Event_Trigger( idEntity *ent );
	void						Event_SetCvar( const char *name, const char *value ) const;
	void						Event_GetCvar( const char *name ) const;
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
	void						Event_ClearPersistantArgs( void );
	void 						Event_SetPersistantArg( const char *key, const char *value );
	void 						Event_GetPersistantString( const char *key );
	void 						Event_GetPersistantFloat( const char *key );
	void 						Event_GetPersistantVector( const char *key );
	void						Event_AngToForward( idAngles &ang );
	void						Event_AngToRight( idAngles &ang );
	void						Event_AngToUp( idAngles &ang );
	void						Event_GetSine( float angle );
	void						Event_GetCosine( float angle );
	void						Event_GetSquareRoot( float theSquare );
	void						Event_VecNormalize( idVec3 &vec );
	void						Event_VecLength( idVec3 &vec );
	void						Event_VecDotProduct( idVec3 &vec1, idVec3 &vec2 );
	void						Event_VecCrossProduct( idVec3 &vec1, idVec3 &vec2 );
	void						Event_VecToAngles( idVec3 &vec );
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
	void						Event_StrToFloat( const char *string );
	void						Event_RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignore, const char *damageDefName, float dmgPower );
	void						Event_IsClient( void );
	void 						Event_IsMultiplayer( void );
	void 						Event_GetFrameTime( void );
	void 						Event_GetTicsPerSecond( void );
	void						Event_CacheSoundShader( const char *soundName );
	void						Event_DebugLine( const idVec3 &color, const idVec3 &start, const idVec3 &end, const float lifetime );
	void						Event_DebugArrow( const idVec3 &color, const idVec3 &start, const idVec3 &end, const int size, const float lifetime );
	void						Event_DebugCircle( const idVec3 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const float lifetime );
	void						Event_DebugBounds( const idVec3 &color, const idVec3 &mins, const idVec3 &maxs, const float lifetime );
	void						Event_DrawText( const char *text, const idVec3 &origin, float scale, const idVec3 &color, const int align, const float lifetime );
	void						Event_InfluenceActive( void );

// RAVEN BEGIN
// kfuller: added
	void						Event_SetSpawnVector( const char *key, idVec3 &vec );
	void						Event_GetArcSine( float sinValue );
	void						Event_GetArcCosine( float cosValue );
	void						Event_ClearSignalAllThreads( int signal, idEntity *ent );
	void						Event_VecRotate(idVec3 &vecToBeRotated, idVec3 &rotateHowMuch);
	void						Event_IsStringEmpty( const char *checkString );
	void						Event_AnnounceToAI( const char *announcement);
	void						Event_ChangeCrossings(const char *originalType, const char *newType);

// abahr: so we can fake having pure script objects
	void						Event_ReferenceScriptObjectProxy( const char* scriptObjectName );
	void						Event_ReleaseScriptObjectProxy( const char* proxyName );
	void						Event_ClampFloat( float min, float max, float val );
	void						Event_MinFloat( float val1, float val2 );
	void						Event_MaxFloat( float val1, float val2 );
	void						Event_StrFind( idStr& sourceStr, idStr& subStr );
	void						Event_RandomInt( float range ) const;

// rjohnson: new blur special effect
	void						Event_SetSpecialEffect( int Effect, int Enabled );
	void						Event_SetSpecialEffectParm( int Effect, int Parm, float Value );

// nmckenzie: string signaling
	void						Event_PlayWorldEffect( const char *effectName, idVec3 &org, idVec3 &angle );
// asalmon: achievements for Xenon
	void						Event_AwardAchievement( const char *name);
// twhitaker: ceil, floor and intVal
	void						Event_GetCeil( float val );
	void						Event_GetFloor( float val );
	void						Event_ToInt( float val );
// jdischler: send named event string to specified gui
	void						Event_SendNamedEvent( int guiEnum, const char *namedEvent );
	void						Event_BeginManualStreaming( void );
	void						Event_EndManualStreaming( void );
	void						Event_SetMatSort( const char *name, const char *val ) const;
// RAVEN END

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
	void						CallFunction( const function_t	*func, bool clearStack );

								// NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
	void						CallFunction( idEntity *obj, const function_t *func, bool clearStack );

// RAVEN BEGIN
// bgeisler: added way to list functions
	void						ListStates( void );

// abahr: added helper functions for pushing parms onto stack
	void						ClearStack( void );
	void						PushInt( int value );
	void						PushFloat( float value );
	void						PushVec3( const idVec3& value );
	void						PushEntity( const idEntity* ent );
	void						PushString( const char* str );
	void						PushBool( bool value );
// RAVEN END

	void						DisplayInfo( void );
	static idThread				*GetThread( int num );
	static void					ListThreads_f( const idCmdArgs &args );
	static void					Restart( void );
	static void					ObjectMoveDone( int threadnum, idEntity *obj );
									
	static idList<idThread*>&	GetThreads( void );
	
	bool						IsDoneProcessing( void );
	bool						IsDying ( void );	
								
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

	void						Error( const char *fmt, ... ) const;
	void						Warning( const char *fmt, ... ) const;
								
	static idThread				*CurrentThread( void );
	static int					CurrentThreadNum( void );
	static bool					BeginMultiFrameEvent( idEntity *ent, const idEventDef *event );
	static void					EndMultiFrameEvent( idEntity *ent, const idEventDef *event );

	static void					ReturnString( const char *text );
	static void					ReturnFloat( float value );
	static void					ReturnInt( int value );
	static void					ReturnVector( idVec3 const &vec );
// RAVEN BEGIN
// abahr: added const
	static void					ReturnEntity( const idEntity *ent );
// RAVEN END
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
