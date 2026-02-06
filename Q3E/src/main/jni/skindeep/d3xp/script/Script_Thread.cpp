/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "gamesys/SysCvar.h"
#include "Player.h"
#include "Camera.h"

#include "framework/FileSystem.h"
#include "framework/DeclEntityDef.h"
#include "Fx.h" //BC

#include "WorldSpawn.h"
#include "script/Script_Thread.h"
#include "renderer/ModelManager.h"

const idEventDef EV_Thread_Execute( "<execute>", NULL );
const idEventDef EV_Thread_SetCallback( "<script_setcallback>", NULL );

// script callable events
const idEventDef EV_Thread_TerminateThread( "terminate", "d" );
const idEventDef EV_Thread_Pause( "pause", NULL );
const idEventDef EV_Thread_Wait( "wait", "f" );
const idEventDef EV_Thread_WaitFrame( "waitFrame" );
const idEventDef EV_Thread_WaitFor( "waitFor", "e" );
const idEventDef EV_Thread_WaitForThread( "waitForThread", "d" );
const idEventDef EV_Thread_Print( "print", "s" );
const idEventDef EV_Thread_PrintLn( "println", "s" );
const idEventDef EV_Thread_Say( "say", "s" );
const idEventDef EV_Thread_Assert( "assert", "f" );
const idEventDef EV_Thread_Trigger( "trigger", "e" );
const idEventDef EV_Thread_SetCvar( "setcvar", "ss" );
const idEventDef EV_Thread_GetCvar( "getcvar", "s", 's' );
const idEventDef EV_Thread_Random( "random", "f", 'f' );
const idEventDef EV_Thread_RandomInt( "randomInt", "d", 'd' );
const idEventDef EV_Thread_GetTime( "getTime", NULL, 'f' );
const idEventDef EV_Thread_KillThread( "killthread", "s" );
const idEventDef EV_Thread_SetThreadName( "threadname", "s" );
const idEventDef EV_Thread_GetEntity( "getEntity", "s", 'e' );
const idEventDef EV_Thread_Spawn( "spawn", "s", 'e' );
const idEventDef EV_Thread_CopySpawnArgs( "copySpawnArgs", "e" );
const idEventDef EV_Thread_SetSpawnArg( "setSpawnArg", "ss" );
const idEventDef EV_Thread_SpawnString( "SpawnString", "ss", 's' );
const idEventDef EV_Thread_SpawnFloat( "SpawnFloat", "sf", 'f' );
const idEventDef EV_Thread_SpawnVector( "SpawnVector", "sv", 'v' );
const idEventDef EV_Thread_ClearPersistantArgs( "clearPersistantArgs" );
const idEventDef EV_Thread_SetPersistantArg( "setPersistantArg", "ss" );
const idEventDef EV_Thread_GetPersistantString( "getPersistantString", "s", 's' );
const idEventDef EV_Thread_GetPersistantFloat( "getPersistantFloat", "s", 'f' );
const idEventDef EV_Thread_GetPersistantVector( "getPersistantVector", "s", 'v' );
const idEventDef EV_Thread_AngToForward( "angToForward", "v", 'v' );
const idEventDef EV_Thread_AngToRight( "angToRight", "v", 'v' );
const idEventDef EV_Thread_AngToUp( "angToUp", "v", 'v' );
const idEventDef EV_Thread_Sine( "sin", "f", 'f' );
const idEventDef EV_Thread_Cosine( "cos", "f", 'f' );
const idEventDef EV_Thread_ArcSine( "asin", "f", 'f' );
const idEventDef EV_Thread_ArcCosine( "acos", "f", 'f' );
const idEventDef EV_Thread_ArcTan( "atan", "f", 'f' );
const idEventDef EV_Thread_SquareRoot( "sqrt", "f", 'f' );
const idEventDef EV_Thread_Normalize( "vecNormalize", "v", 'v' );
const idEventDef EV_Thread_VecLength( "vecLength", "v", 'f' );
const idEventDef EV_Thread_VecDotProduct( "DotProduct", "vv", 'f' );
const idEventDef EV_Thread_VecCrossProduct( "CrossProduct", "vv", 'v' );
const idEventDef EV_Thread_VecToAngles( "VecToAngles", "v", 'v' );
const idEventDef EV_Thread_VecToOrthoBasisAngles( "VecToOrthoBasisAngles", "v", 'v' );
const idEventDef EV_Thread_RotateVector("rotateVector", "vv", 'v');
const idEventDef EV_Thread_OnSignal( "onSignal", "des" );
const idEventDef EV_Thread_ClearSignal( "clearSignalThread", "de" );
const idEventDef EV_Thread_SetCamera( "setCamera", "e" );
const idEventDef EV_Thread_FirstPerson( "firstPerson", NULL );
const idEventDef EV_Thread_Trace( "trace", "vvvvde", 'f' );
const idEventDef EV_Thread_TracePoint( "tracePoint", "vvde", 'f' );
const idEventDef EV_Thread_GetTraceFraction( "getTraceFraction", NULL, 'f' );
const idEventDef EV_Thread_GetTraceEndPos( "getTraceEndPos", NULL, 'v' );
const idEventDef EV_Thread_GetTraceNormal( "getTraceNormal", NULL, 'v' );
const idEventDef EV_Thread_GetTraceEntity( "getTraceEntity", NULL, 'e' );
const idEventDef EV_Thread_GetTraceJoint( "getTraceJoint", NULL, 's' );
const idEventDef EV_Thread_GetTraceBody( "getTraceBody", NULL, 's' );
const idEventDef EV_Thread_FadeIn( "fadeIn", "vf" );
const idEventDef EV_Thread_FadeOut( "fadeOut", "vf" );
const idEventDef EV_Thread_FadeTo( "fadeTo", "vff" );
const idEventDef EV_Thread_StartMusic( "music", "s" );
const idEventDef EV_Thread_Error( "error", "s" );
const idEventDef EV_Thread_Warning( "warning", "s" );
const idEventDef EV_Thread_StrLen( "strLength", "s", 'd' );
const idEventDef EV_Thread_StrLeft( "strLeft", "sd", 's' );
const idEventDef EV_Thread_StrRight( "strRight", "sd", 's' );
const idEventDef EV_Thread_StrSkip( "strSkip", "sd", 's' );
const idEventDef EV_Thread_StrMid( "strMid", "sdd", 's' );
const idEventDef EV_Thread_StrToFloat( "strToFloat", "s", 'f' );
const idEventDef EV_Thread_RadiusDamage( "radiusDamage", "vEEEsf" );
const idEventDef EV_Thread_IsClient( "isClient", NULL, 'f' );
const idEventDef EV_Thread_IsMultiplayer( "isMultiplayer", NULL, 'f' );
const idEventDef EV_Thread_GetFrameTime( "getFrameTime", NULL, 'f' );
const idEventDef EV_Thread_GetTicsPerSecond( "getTicsPerSecond", NULL, 'f' );
const idEventDef EV_Thread_DebugLine( "debugLine", "vvvf" );
const idEventDef EV_Thread_DebugArrow( "debugArrow", "vvvdf" );
const idEventDef EV_Thread_DebugCircle( "debugCircle", "vvvfdf" );
const idEventDef EV_Thread_DebugBounds( "debugBounds", "vvvf" );
const idEventDef EV_Thread_DrawText( "drawText", "svfvdf" );
const idEventDef EV_Thread_InfluenceActive( "influenceActive", NULL, 'd' );
const idEventDef EV_Thread_RayDamage("rayDamage", "vvsd");


//BC
const idEventDef EV_Thread_getClassEntity("getClassEntity", "sd", 'e');
const idEventDef EV_Thread_playFx("playFx", "svv", 'e');
const idEventDef EV_Thread_playParticle("playParticle", "svv");
const idEventDef EV_Thread_makeparticlezoo("makeparticlezoo", NULL);
const idEventDef EV_Thread_makeitemzoo("makeitemzoo", NULL);
const idEventDef EV_Thread_makepropzoo("makepropzoo", NULL);
const idEventDef EV_Thread_floatRound("floatRound", "ff", 's');
const idEventDef EV_Thread_GetTraceSky("getTraceSky", NULL, 'd');
const idEventDef EV_Thread_GetTraceGlass("getTraceGlass", NULL, 'd');
const idEventDef EV_Thread_getClassCount("getClassCount", "s", 'd');
const idEventDef EV_Thread_DebugArrowsimple("debugArrowsimple", "v");
const idEventDef EV_Thread_SpawnInterestpoint("SpawnInterestpoint", "sv");
const idEventDef EV_Thread_parsetimeMS("parsetimeMS", "f", 's');
const idEventDef EV_Thread_parsetimeSec("parsetimeSec", "f", 's');
const idEventDef EV_Thread_loadmap("loadmap", "s");
const idEventDef EV_Thread_setSlowmo("setSlowmo", "d");
const idEventDef EV_Thread_setAmbientLight("setambientlight", "d");
const idEventDef EV_Thread_SetCommand("setcommand", "s");
const idEventDef EV_Thread_CacheModel("cacheModel", "s");
const idEventDef EV_Thread_SetOverrideEfx("setOverrideEfx", "ds");
const idEventDef EV_Thread_getworldspawnint("getworldspawnint", "s", 'd');
const idEventDef EV_Thread_FadeOutMusic("fadeOutMusic", "ff");
const idEventDef EV_Thread_SetMusicMultiplier("setMusicMultiplier", "f");
const idEventDef EV_Thread_GetEntityInBounds("getEntityInBounds", "svv", 'e');
const idEventDef EV_Thread_RandomChar("randomChar", NULL, 's');
const idEventDef EV_Thread_IsAirlessAtPoint("isAirlessAtPoint", "v", 'd');
const idEventDef EV_Thread_RemoveEntitiesWithinBounds("removeEntitiesWithinBounds", "svv");
const idEventDef EV_Thread_RemoveAllDecals("removeAllDecals", NULL);
const idEventDef EV_Thread_ClearInterestPoints("clearInterestPoints", NULL);
const idEventDef EV_RequirementMet("requirementMet", "sd", 'd');
const idEventDef EV_Thread_CacheGui("cacheGui", "s");
const idEventDef EV_Thread_CacheMaterial("cacheMaterial", "s");
const idEventDef EV_Thread_CacheFX("cacheFX", "s");
const idEventDef EV_Thread_CacheSkin("cacheSkin", "s");
const idEventDef EV_Thread_CacheEntityDef("cacheEntityDef", "s");

const idEventDef EV_Thread_steamOpenStoreOverlay("steamOpenStoreOverlay");

CLASS_DECLARATION( idClass, idThread )
	EVENT( EV_Thread_Execute,				idThread::Event_Execute )
	EVENT( EV_Thread_TerminateThread,		idThread::Event_TerminateThread )
	EVENT( EV_Thread_Pause,					idThread::Event_Pause )
	EVENT( EV_Thread_Wait,					idThread::Event_Wait )
	EVENT( EV_Thread_WaitFrame,				idThread::Event_WaitFrame )
	EVENT( EV_Thread_WaitFor,				idThread::Event_WaitFor )
	EVENT( EV_Thread_WaitForThread,			idThread::Event_WaitForThread )
	EVENT( EV_Thread_Print,					idThread::Event_Print )
	EVENT( EV_Thread_PrintLn,				idThread::Event_PrintLn )
	EVENT( EV_Thread_Say,					idThread::Event_Say )
	EVENT( EV_Thread_Assert,				idThread::Event_Assert )
	EVENT( EV_Thread_Trigger,				idThread::Event_Trigger )
	EVENT( EV_Thread_SetCvar,				idThread::Event_SetCvar )
	EVENT( EV_Thread_GetCvar,				idThread::Event_GetCvar )
	EVENT( EV_Thread_Random,				idThread::Event_Random )
#ifdef _D3XP
	EVENT( EV_Thread_RandomInt,				idThread::Event_RandomInt )
#endif
	EVENT( EV_Thread_GetTime,				idThread::Event_GetTime )
	EVENT( EV_Thread_KillThread,			idThread::Event_KillThread )
	EVENT( EV_Thread_SetThreadName,			idThread::Event_SetThreadName )
	EVENT( EV_Thread_GetEntity,				idThread::Event_GetEntity )
	EVENT( EV_Thread_Spawn,					idThread::Event_Spawn )
	EVENT( EV_Thread_CopySpawnArgs,			idThread::Event_CopySpawnArgs )
	EVENT( EV_Thread_SetSpawnArg,			idThread::Event_SetSpawnArg )
	EVENT( EV_Thread_SpawnString,			idThread::Event_SpawnString )
	EVENT( EV_Thread_SpawnFloat,			idThread::Event_SpawnFloat )
	EVENT( EV_Thread_SpawnVector,			idThread::Event_SpawnVector )
	EVENT( EV_Thread_ClearPersistantArgs,	idThread::Event_ClearPersistantArgs )
	EVENT( EV_Thread_SetPersistantArg,		idThread::Event_SetPersistantArg )
	EVENT( EV_Thread_GetPersistantString,	idThread::Event_GetPersistantString )
	EVENT( EV_Thread_GetPersistantFloat,	idThread::Event_GetPersistantFloat )
	EVENT( EV_Thread_GetPersistantVector,	idThread::Event_GetPersistantVector )
	EVENT( EV_Thread_AngToForward,			idThread::Event_AngToForward )
	EVENT( EV_Thread_AngToRight,			idThread::Event_AngToRight )
	EVENT( EV_Thread_AngToUp,				idThread::Event_AngToUp )
	EVENT( EV_Thread_Sine,					idThread::Event_GetSine )
	EVENT( EV_Thread_Cosine,				idThread::Event_GetCosine )
#ifdef _D3XP
	EVENT( EV_Thread_ArcSine,				idThread::Event_GetArcSine )
	EVENT( EV_Thread_ArcCosine,				idThread::Event_GetArcCosine )
	EVENT( EV_Thread_ArcTan,				idThread::Event_GetArcTan )
#endif
	EVENT( EV_Thread_SquareRoot,			idThread::Event_GetSquareRoot )
	EVENT( EV_Thread_Normalize,				idThread::Event_VecNormalize )
	EVENT( EV_Thread_VecLength,				idThread::Event_VecLength )
	EVENT( EV_Thread_VecDotProduct,			idThread::Event_VecDotProduct )
	EVENT( EV_Thread_VecCrossProduct,		idThread::Event_VecCrossProduct )
	EVENT( EV_Thread_VecToAngles,			idThread::Event_VecToAngles )
#ifdef _D3XP
	EVENT( EV_Thread_VecToOrthoBasisAngles, idThread::Event_VecToOrthoBasisAngles )
	EVENT( EV_Thread_RotateVector,			idThread::Event_RotateVector )
#endif
	EVENT( EV_Thread_OnSignal,				idThread::Event_OnSignal )
	EVENT( EV_Thread_ClearSignal,			idThread::Event_ClearSignalThread )
	EVENT( EV_Thread_SetCamera,				idThread::Event_SetCamera )
	EVENT( EV_Thread_FirstPerson,			idThread::Event_FirstPerson )
	EVENT( EV_Thread_Trace,					idThread::Event_Trace )
	EVENT( EV_Thread_TracePoint,			idThread::Event_TracePoint )
	EVENT( EV_Thread_GetTraceFraction,		idThread::Event_GetTraceFraction )
	EVENT( EV_Thread_GetTraceEndPos,		idThread::Event_GetTraceEndPos )
	EVENT( EV_Thread_GetTraceNormal,		idThread::Event_GetTraceNormal )
	EVENT( EV_Thread_GetTraceEntity,		idThread::Event_GetTraceEntity )
	EVENT( EV_Thread_GetTraceJoint,			idThread::Event_GetTraceJoint )
	EVENT( EV_Thread_GetTraceBody,			idThread::Event_GetTraceBody )
	EVENT( EV_Thread_FadeIn,				idThread::Event_FadeIn )
	EVENT( EV_Thread_FadeOut,				idThread::Event_FadeOut )
	EVENT( EV_Thread_FadeTo,				idThread::Event_FadeTo )
	EVENT( EV_SetShaderParm,				idThread::Event_SetShaderParm )
	EVENT( EV_Thread_StartMusic,			idThread::Event_StartMusic )
	EVENT( EV_Thread_Warning,				idThread::Event_Warning )
	EVENT( EV_Thread_Error,					idThread::Event_Error )
	EVENT( EV_Thread_StrLen,				idThread::Event_StrLen )
	EVENT( EV_Thread_StrLeft,				idThread::Event_StrLeft )
	EVENT( EV_Thread_StrRight,				idThread::Event_StrRight )
	EVENT( EV_Thread_StrSkip,				idThread::Event_StrSkip )
	EVENT( EV_Thread_StrMid,				idThread::Event_StrMid )
	EVENT( EV_Thread_StrToFloat,			idThread::Event_StrToFloat )
	EVENT( EV_Thread_RadiusDamage,			idThread::Event_RadiusDamage )
	EVENT( EV_Thread_IsClient,				idThread::Event_IsClient )
	EVENT( EV_Thread_IsMultiplayer,			idThread::Event_IsMultiplayer )
	EVENT( EV_Thread_GetFrameTime,			idThread::Event_GetFrameTime )
	EVENT( EV_Thread_GetTicsPerSecond,		idThread::Event_GetTicsPerSecond )
	EVENT( EV_CacheSoundShader,				idThread::Event_CacheSoundShader )
	EVENT( EV_Thread_DebugLine,				idThread::Event_DebugLine )
	EVENT( EV_Thread_DebugArrow,			idThread::Event_DebugArrow )
	EVENT( EV_Thread_DebugCircle,			idThread::Event_DebugCircle )
	EVENT( EV_Thread_DebugBounds,			idThread::Event_DebugBounds )
	EVENT( EV_Thread_DrawText,				idThread::Event_DrawText )
	EVENT( EV_Thread_InfluenceActive,		idThread::Event_InfluenceActive )
	EVENT(EV_Thread_RayDamage,				idThread::Event_RayDamage)
	

	//BC
	EVENT(EV_Thread_getClassEntity,			idThread::Event_getClassEntity)
	EVENT(EV_Thread_playFx,					idThread::Event_PlayFX)
	EVENT(EV_Thread_playParticle,			idThread::Event_PlayParticle)
	EVENT(EV_Thread_makeparticlezoo,		idThread::Event_MakeParticleZoo)
	EVENT(EV_Thread_makeitemzoo,			idThread::Event_MakeItemZoo)
	EVENT(EV_Thread_makepropzoo,			idThread::Event_MakePropZoo)
	EVENT(EV_Thread_floatRound,				idThread::Event_floatRound)
	EVENT(EV_Thread_GetTraceSky,			idThread::Event_GetTraceSky)
	EVENT(EV_Thread_GetTraceGlass,			idThread::Event_GetTraceGlass)
	EVENT(EV_Thread_getClassCount,			idThread::Event_getClassCount)
	EVENT(EV_Thread_DebugArrowsimple,		idThread::Event_DebugArrowsimple)
	EVENT(EV_Thread_SpawnInterestpoint,		idThread::Event_SpawnInterestpoint)
	EVENT(EV_Thread_parsetimeMS,			idThread::Event_ParseTimeMS)
	EVENT(EV_Thread_parsetimeSec,			idThread::Event_ParseTimeSec)
	EVENT(EV_Thread_loadmap,				idThread::Event_LoadMap)
	EVENT(EV_Thread_setSlowmo,				idThread::Event_SetSlowmo)
	EVENT(EV_Thread_setAmbientLight,		idThread::Event_SetAmbientLight)
	EVENT(EV_Thread_SetCommand,				idThread::Event_SetCommand)
	EVENT(EV_Thread_CacheModel,				idThread::Event_CacheModel)
	EVENT(EV_Thread_SetOverrideEfx,			idThread::Event_SetOverrideEfx)
	EVENT(EV_Thread_getworldspawnint,		idThread::Event_getworldspawnint)
	EVENT(EV_Thread_FadeOutMusic,			idThread::Event_FadeOutMusic)
	EVENT(EV_Thread_SetMusicMultiplier,		idThread::Event_SetMusicMultiplier)
	EVENT(EV_Thread_GetEntityInBounds,		idThread::Event_getEntityInBounds)
	EVENT(EV_Thread_RandomChar,				idThread::Event_RandomChar)
	EVENT(EV_Thread_IsAirlessAtPoint,		idThread::Event_IsAirlessAtPoint)
	EVENT(EV_Thread_RemoveEntitiesWithinBounds, idThread::Event_RemoveEntitiesWithinBounds)
	EVENT(EV_Thread_RemoveAllDecals,		idThread::Event_RemoveAllDecals)
	EVENT(EV_Thread_ClearInterestPoints,	idThread::Event_ClearInterestPoints)
	EVENT(EV_RequirementMet,				idThread::Event_RequirementMet)
	EVENT(EV_Thread_CacheGui,				idThread::Event_CacheGui)
	EVENT(EV_Thread_CacheMaterial,			idThread::Event_CacheMaterial)
	EVENT(EV_Thread_CacheFX,				idThread::Event_CacheFX)
	EVENT(EV_Thread_CacheSkin,				idThread::Event_CacheSkin)
	EVENT(EV_Thread_CacheEntityDef,			idThread::Event_CacheEntityDef)
	

	EVENT(EV_Thread_steamOpenStoreOverlay,	idThread::Event_steamOpenStoreOverlay)		

END_CLASS

idThread			*idThread::currentThread = NULL;
int					idThread::threadIndex = 0;
idList<idThread *>	idThread::threadList;
trace_t				idThread::trace;

/*
================
idThread::CurrentThread
================
*/
idThread *idThread::CurrentThread( void ) {
	return currentThread;
}

/*
================
idThread::CurrentThreadNum
================
*/
int idThread::CurrentThreadNum( void ) {
	if ( currentThread ) {
		return currentThread->GetThreadNum();
	} else {
		return 0;
	}
}

/*
================
idThread::BeginMultiFrameEvent
================
*/
bool idThread::BeginMultiFrameEvent( idEntity *ent, const idEventDef *event ) {
	if ( !currentThread ) {
		gameLocal.Error( "idThread::BeginMultiFrameEvent called without a current thread" );
	}
	return currentThread->interpreter.BeginMultiFrameEvent( ent, event );
}

/*
================
idThread::EndMultiFrameEvent
================
*/
void idThread::EndMultiFrameEvent( idEntity *ent, const idEventDef *event ) {
	if ( !currentThread ) {
		gameLocal.Error( "idThread::EndMultiFrameEvent called without a current thread" );
	}
	currentThread->interpreter.EndMultiFrameEvent( ent, event );
}

/*
================
idThread::idThread
================
*/
idThread::idThread() {
	Init();
	SetThreadName( va( "thread_%d", threadIndex ) );
	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread( idEntity *self, const function_t *func ) {
	assert( self );

	Init();
	SetThreadName( self->name );
	interpreter.EnterObjectFunction( self, func, false );
	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread( const function_t *func ) {
	assert( func );

	Init();
	SetThreadName( func->Name() );
	interpreter.EnterFunction( func, false );
	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread( idInterpreter *source, const function_t *func, int args ) {
	Init();
	interpreter.ThreadCall( source, func, args );
	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::idThread
================
*/
idThread::idThread( idInterpreter *source, idEntity *self, const function_t *func, int args ) {
	assert( self );

	Init();
	SetThreadName( self->name );
	interpreter.ThreadCall( source, func, args );
	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: create thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
}

/*
================
idThread::~idThread
================
*/
idThread::~idThread() {
	idThread	*thread;
	int			i;
	int			n;

	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: end thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}
	threadList.Remove( this );
	n = threadList.Num();
	for( i = 0; i < n; i++ ) {
		thread = threadList[ i ];
		if ( thread->WaitingOnThread() == this ) {
			thread->ThreadCallback( this );
		}
	}

	if ( currentThread == this ) {
		currentThread = NULL;
	}
}

/*
================
idThread::ManualDelete
================
*/
void idThread::ManualDelete( void ) {
	interpreter.terminateOnExit = false;
}

/*
================
idThread::Save
================
*/
void idThread::Save( idSaveGame *savefile ) const {

	// We will check on restore that threadNum is still the same,
	//  threads should have been restored in the same order.
	savefile->WriteInt( threadNum );

	savefile->WriteObject( waitingForThread );
	savefile->WriteInt( waitingFor );
	savefile->WriteInt( waitingUntil );

	interpreter.Save( savefile );

	savefile->WriteDict( &spawnArgs );
	savefile->WriteString( threadName );

	savefile->WriteInt( lastExecuteTime );
	savefile->WriteInt( creationTime );

	savefile->WriteBool( manualControl );
}

/*
================
idThread::Restore
================
*/
void idThread::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( threadNum );

	savefile->ReadObject( reinterpret_cast<idClass *&>( waitingForThread ) );
	savefile->ReadInt( waitingFor );
	savefile->ReadInt( waitingUntil );

	interpreter.Restore( savefile );

	savefile->ReadDict( &spawnArgs );
	savefile->ReadString( threadName );

	savefile->ReadInt( lastExecuteTime );
	savefile->ReadInt( creationTime );

	savefile->ReadBool( manualControl );
}

/*
================
idThread::Init
================
*/
void idThread::Init( void ) {
	// create a unique threadNum
	do {
		threadIndex++;
		if ( threadIndex == 0 ) {
			threadIndex = 1;
		}
	} while( GetThread( threadIndex ) );

	threadNum = threadIndex;
	threadList.Append( this );

	creationTime = gameLocal.time;
	lastExecuteTime = 0;
	manualControl = false;

	ClearWaitFor();

	interpreter.SetThread( this );
}

/*
================
idThread::GetThread
================
*/
idThread *idThread::GetThread( int num ) {
	int			i;
	int			n;
	idThread	*thread;

	n = threadList.Num();
	for( i = 0; i < n; i++ ) {
		thread = threadList[ i ];
		if ( thread->GetThreadNum() == num ) {
			return thread;
		}
	}

	return NULL;
}

/*
================
idThread::DisplayInfo
================
*/
void idThread::DisplayInfo( void ) {
	gameLocal.Printf(
		"%12i: '%s'\n"
		"        File: %s(%d)\n"
		"     Created: %d (%d ms ago)\n"
		"      Status: ",
		threadNum, threadName.c_str(),
		interpreter.CurrentFile(), interpreter.CurrentLine(),
		creationTime, gameLocal.time - creationTime );

	if ( interpreter.threadDying ) {
		gameLocal.Printf( "Dying\n" );
	} else if ( interpreter.doneProcessing ) {
		gameLocal.Printf(
			"Paused since %d (%d ms)\n"
			"      Reason: ",  lastExecuteTime, gameLocal.time - lastExecuteTime );
		if ( waitingForThread ) {
			gameLocal.Printf( "Waiting for thread #%3i '%s'\n", waitingForThread->GetThreadNum(), waitingForThread->GetThreadName() );
		} else if ( ( waitingFor != ENTITYNUM_NONE ) && ( gameLocal.entities[ waitingFor ] ) ) {
			gameLocal.Printf( "Waiting for entity #%3i '%s'\n", waitingFor, gameLocal.entities[ waitingFor ]->name.c_str() );
		} else if ( waitingUntil ) {
			gameLocal.Printf( "Waiting until %d (%d ms total wait time)\n", waitingUntil, waitingUntil - lastExecuteTime );
		} else {
			gameLocal.Printf( "None\n" );
		}
	} else {
		gameLocal.Printf( "Processing\n" );
	}

	interpreter.DisplayInfo();

	gameLocal.Printf( "\n" );
}

/*
================
idThread::ListThreads_f
================
*/
void idThread::ListThreads_f( const idCmdArgs &args ) {
	int	i;
	int	n;

	n = threadList.Num();
	for( i = 0; i < n; i++ ) {
		//threadList[ i ]->DisplayInfo();
		gameLocal.Printf( "%3i: %-20s : %s(%d)\n", threadList[ i ]->threadNum, threadList[ i ]->threadName.c_str(), threadList[ i ]->interpreter.CurrentFile(), threadList[ i ]->interpreter.CurrentLine() );
	}
	gameLocal.Printf( "%d active threads\n\n", n );
}

/*
================
idThread::Restart
================
*/
void idThread::Restart( void ) {
	int	i;
	int	n;

	// reset the threadIndex
	threadIndex = 0;

	currentThread = NULL;
	n = threadList.Num();
	for( i = n - 1; i >= 0; i-- ) {
		delete threadList[ i ];
	}
	threadList.Clear();

	memset( &trace, 0, sizeof( trace ) );
	trace.c.entityNum = ENTITYNUM_NONE;
}

/*
================
idThread::DelayedStart
================
*/
void idThread::DelayedStart( int delay ) {
	CancelEvents( &EV_Thread_Execute );
	if ( gameLocal.time <= 0 ) {
		delay++;
	}
	PostEventMS( &EV_Thread_Execute, delay );
}

/*
================
idThread::Start
================
*/
bool idThread::Start( void ) {
	bool result;

	CancelEvents( &EV_Thread_Execute );
	result = Execute();

	return result;
}

/*
================
idThread::SetThreadName
================
*/
void idThread::SetThreadName( const char *name ) {
	threadName = name;
}

/*
================
idThread::ObjectMoveDone
================
*/
void idThread::ObjectMoveDone( int threadnum, idEntity *obj ) {
	idThread *thread;

	if ( !threadnum ) {
		return;
	}

	thread = GetThread( threadnum );
	if ( thread ) {
		thread->ObjectMoveDone( obj );
	}
}

/*
================
idThread::End
================
*/
void idThread::End( void ) {
	// Tell thread to die.  It will exit on its own.
	Pause();
	interpreter.threadDying	= true;
}

/*
================
idThread::KillThread
================
*/
void idThread::KillThread( const char *name ) {
	int			i;
	int			num;
	int			len;
	const char	*ptr;
	idThread	*thread;

	// see if the name uses a wild card
	ptr = strchr( name, '*' );
	if ( ptr ) {
		len = ptr - name;
	} else {
		len = strlen( name );
	}

	// kill only those threads whose name matches name
	num = threadList.Num();
	for( i = 0; i < num; i++ ) {
		thread = threadList[ i ];
		if ( !idStr::Cmpn( thread->GetThreadName(), name, len ) ) {
			thread->End();
		}
	}
}

/*
================
idThread::KillThread
================
*/
void idThread::KillThread( int num ) {
	idThread *thread;

	thread = GetThread( num );
	if ( thread ) {
		// Tell thread to die.  It will delete itself on it's own.
		thread->End();
	}
}

/*
================
idThread::Execute
================
*/
bool idThread::Execute( void ) {
	idThread	*oldThread;
	bool		done;

	if ( manualControl && ( waitingUntil > gameLocal.time ) ) {
		return false;
	}

	oldThread = currentThread;
	currentThread = this;

	lastExecuteTime = gameLocal.time;
	ClearWaitFor();
	done = interpreter.Execute();
	if ( done ) {
		End();
		if ( interpreter.terminateOnExit ) {
			PostEventMS( &EV_Remove, 0 );
		}
	} else if ( !manualControl ) {
		if ( waitingUntil > lastExecuteTime ) {
			PostEventMS( &EV_Thread_Execute, waitingUntil - lastExecuteTime );
		} else if ( interpreter.MultiFrameEventInProgress() ) {
			PostEventMS( &EV_Thread_Execute, gameLocal.msec );
		}
	}

	currentThread = oldThread;

	return done;
}

/*
================
idThread::IsWaiting

Checks if thread is still waiting for some event to occur.
================
*/
bool idThread::IsWaiting( void ) {
	if ( waitingForThread || ( waitingFor != ENTITYNUM_NONE ) ) {
		return true;
	}

	if ( waitingUntil && ( waitingUntil > gameLocal.time ) ) {
		return true;
	}

	return false;
}

/*
================
idThread::CallFunction

NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
================
*/
void idThread::CallFunction( const function_t *func, bool clearStack ) {
	ClearWaitFor();
	interpreter.EnterFunction( func, clearStack );
}

/*
================
idThread::CallFunction

NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
================
*/
void idThread::CallFunction( idEntity *self, const function_t *func, bool clearStack ) {
	assert( self );
	ClearWaitFor();
	interpreter.EnterObjectFunction( self, func, clearStack );
}


void idThread::PushArg( idEntity* ent )
{
	interpreter.Push( ent->entityNumber + 1 );
}

/*
================
idThread::ClearWaitFor
================
*/
void idThread::ClearWaitFor( void ) {
	waitingFor			= ENTITYNUM_NONE;
	waitingForThread	= NULL;
	waitingUntil		= 0;
}

/*
================
idThread::IsWaitingFor
================
*/
bool idThread::IsWaitingFor( idEntity *obj ) {
	assert( obj );
	return waitingFor == obj->entityNumber;
}

/*
================
idThread::ObjectMoveDone
================
*/
void idThread::ObjectMoveDone( idEntity *obj ) {
	assert( obj );

	if ( IsWaitingFor( obj ) ) {
		ClearWaitFor();
		DelayedStart( 0 );
	}
}

/*
================
idThread::ThreadCallback
================
*/
void idThread::ThreadCallback( idThread *thread ) {
	if ( interpreter.threadDying ) {
		return;
	}

	if ( thread == waitingForThread ) {
		ClearWaitFor();
		DelayedStart( 0 );
	}
}

/*
================
idThread::Event_SetThreadName
================
*/
void idThread::Event_SetThreadName( const char *name ) {
	SetThreadName( name );
}

/*
================
idThread::Error
================
*/
void idThread::Error( const char *fmt, ... ) const {
	va_list	argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	interpreter.Error( text );
}

/*
================
idThread::Warning
================
*/
void idThread::Warning( const char *fmt, ... ) const {
	va_list	argptr;
	char	text[ 1024 ];

	va_start( argptr, fmt );
	vsprintf( text, fmt, argptr );
	va_end( argptr );

	interpreter.Warning( text );
}

/*
================
idThread::ReturnString
================
*/
void idThread::ReturnString( const char *text ) {
	gameLocal.program.ReturnString( text );
}

/*
================
idThread::ReturnFloat
================
*/
void idThread::ReturnFloat( float value ) {
	gameLocal.program.ReturnFloat( value );
}

/*
================
idThread::ReturnInt
================
*/
void idThread::ReturnInt( int value ) {
	// true integers aren't supported in the compiler,
	// so int values are stored as floats
	gameLocal.program.ReturnFloat( value );
}

/*
================
idThread::ReturnVector
================
*/
void idThread::ReturnVector( idVec3 const &vec ) {
	gameLocal.program.ReturnVector( vec );
}

/*
================
idThread::ReturnEntity
================
*/
void idThread::ReturnEntity( idEntity *ent ) {
	gameLocal.program.ReturnEntity( ent );
}

/*
================
idThread::Event_Execute
================
*/
void idThread::Event_Execute( void ) {
	Execute();
}

/*
================
idThread::Pause
================
*/
void idThread::Pause( void ) {
	ClearWaitFor();
	interpreter.doneProcessing = true;
}

/*
================
idThread::WaitMS
================
*/
void idThread::WaitMS( int time ) {
	if (time > 0)
	{
		Pause();
		waitingUntil = gameLocal.time + time;
	}
}

/*
================
idThread::WaitSec
================
*/
void idThread::WaitSec( float time ) {
	WaitMS( SEC2MS( time ) );
}

/*
================
idThread::WaitFrame
================
*/
void idThread::WaitFrame( void ) {
	Pause();

	// manual control threads don't set waitingUntil so that they can be run again
	// that frame if necessary.
	if ( !manualControl ) {
		waitingUntil = gameLocal.time + gameLocal.msec;
	}
}

/***********************************************************************

  Script callable events

***********************************************************************/

/*
================
idThread::Event_TerminateThread
================
*/
void idThread::Event_TerminateThread( int num ) {
	KillThread( num );
}

/*
================
idThread::Event_Pause
================
*/
void idThread::Event_Pause( void ) {
	Pause();
}

/*
================
idThread::Event_Wait
================
*/
void idThread::Event_Wait( float time ) {
	WaitSec( time );
}

/*
================
idThread::Event_WaitFrame
================
*/
void idThread::Event_WaitFrame( void ) {
	WaitFrame();
}

/*
================
idThread::Event_WaitFor
================
*/
void idThread::Event_WaitFor( idEntity *ent ) {
	if ( ent && ent->RespondsTo( EV_Thread_SetCallback ) ) {
		ent->ProcessEvent( &EV_Thread_SetCallback );
		if ( gameLocal.program.GetReturnedInteger() ) {
			Pause();
			waitingFor = ent->entityNumber;
		}
	}
}

/*
================
idThread::Event_WaitForThread
================
*/
void idThread::Event_WaitForThread( int num ) {
	idThread *thread;

	thread = GetThread( num );
	if ( !thread ) {
		if ( g_debugScript.GetBool() ) {
			// just print a warning and continue executing
			Warning( "Thread %d not running", num );
		}
	} else {
		Pause();
		waitingForThread = thread;
	}
}

/*
================
idThread::Event_Print
================
*/
void idThread::Event_Print( const char *text ) {
	gameLocal.Printf( "%s", text );
}

/*
================
idThread::Event_PrintLn
================
*/
void idThread::Event_PrintLn( const char *text ) {
	gameLocal.Printf( "%s\n", text );
}

/*
================
idThread::Event_Say
================
*/
void idThread::Event_Say( const char *text ) {
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "say \"%s\"", text ) );
}

/*
================
idThread::Event_Assert
================
*/
void idThread::Event_Assert( float value ) {
	assert( value );
}

/*
================
idThread::Event_Trigger
================
*/
void idThread::Event_Trigger( idEntity *ent ) {
	if ( ent ) {
		ent->Signal( SIG_TRIGGER );
		ent->ProcessEvent( &EV_Activate, gameLocal.GetLocalPlayer() );
		ent->TriggerGuis();
	}
}

/*
================
idThread::Event_SetCvar
================
*/
void idThread::Event_SetCvar( const char *name, const char *value ) const {
	cvarSystem->SetCVarString( name, value, CVAR_SCRIPTMODIFIED );
}

void idThread::Event_SetCommand(const char *name) const {

#if _DEBUG
	//we only allow this in debug build because it is possible (??????) there are some console commands that may be destructive (??????)
	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, name);
#endif
}


void idThread::Event_CacheModel( const char *modelName )
{
	if (renderModelManager->FindModel(modelName) == nullptr)
	{
		common->Warning("sys.cacheModel failed to cache '%s'\n", modelName);
	}
}

/*
================
idThread::Event_GetCvar
================
*/
void idThread::Event_GetCvar( const char *name ) const {
	ReturnString( cvarSystem->GetCVarString( name ) );
}

/*
================
idThread::Event_Random
================
*/
void idThread::Event_Random( float range ) const {
	float result;

	result = gameLocal.random.RandomFloat();
	ReturnFloat( range * result );
}

#ifdef _D3XP

void idThread::Event_RandomInt( int range ) const {
	int result;
	result = gameLocal.random.RandomInt(range);
	ReturnFloat(result);
}

#endif

/*
================
idThread::Event_GetTime
================
*/
void idThread::Event_GetTime( void ) {
	ReturnFloat( MS2SEC( gameLocal.realClientTime ) );
}

/*
================
idThread::Event_KillThread
================
*/
void idThread::Event_KillThread( const char *name ) {
	KillThread( name );
}

/*
================
idThread::Event_GetEntity
================
*/
void idThread::Event_GetEntity( const char *name ) {
	int			entnum;
	idEntity	*ent;

	assert( name );

	if ( name[ 0 ] == '*' ) {
		entnum = atoi( &name[ 1 ] );
		if ( ( entnum < 0 ) || ( entnum >= MAX_GENTITIES ) ) {
			Error( "Entity number in string out of range." );
		}
		ReturnEntity( gameLocal.entities[ entnum ] );
	} else {
		ent = gameLocal.FindEntity( name );
		ReturnEntity( ent );
	}
}

/*
================
idThread::Event_Spawn
================
*/
void idThread::Event_Spawn( const char *classname ) {
	idEntity *ent;

	spawnArgs.Set( "classname", classname );
	gameLocal.SpawnEntityDef( spawnArgs, &ent );
	ReturnEntity( ent );
	spawnArgs.Clear();
}

/*
================
idThread::Event_CopySpawnArgs
================
*/
void idThread::Event_CopySpawnArgs( idEntity *ent ) {
	spawnArgs.Copy( ent->spawnArgs );
}

/*
================
idThread::Event_SetSpawnArg
================
*/
void idThread::Event_SetSpawnArg( const char *key, const char *value ) {
	spawnArgs.Set( key, value );
}

/*
================
idThread::Event_SpawnString
================
*/
void idThread::Event_SpawnString( const char *key, const char *defaultvalue ) {
	const char *result;

	spawnArgs.GetString( key, defaultvalue, &result );
	ReturnString( result );
}

/*
================
idThread::Event_SpawnFloat
================
*/
void idThread::Event_SpawnFloat( const char *key, float defaultvalue ) {
	float result;

	spawnArgs.GetFloat( key, va( "%f", defaultvalue ), result );
	ReturnFloat( result );
}

/*
================
idThread::Event_SpawnVector
================
*/
void idThread::Event_SpawnVector( const char *key, const idVec3 &defaultvalue ) {
	idVec3 result;

	spawnArgs.GetVector( key, va( "%f %f %f", defaultvalue.x, defaultvalue.y, defaultvalue.z ), result );
	ReturnVector( result );
}

/*
================
idThread::Event_ClearPersistantArgs
================
*/
void idThread::Event_ClearPersistantArgs( void ) {
	gameLocal.persistentLevelInfo.Clear();
}


/*
================
idThread::Event_SetPersistantArg
================
*/
void idThread::Event_SetPersistantArg( const char *key, const char *value ) {
	gameLocal.persistentLevelInfo.Set( key, value );
}

/*
================
idThread::Event_GetPersistantString
================
*/
void idThread::Event_GetPersistantString( const char *key ) {
	const char *result;

	gameLocal.persistentLevelInfo.GetString( key, "", &result );
	ReturnString( result );
}

/*
================
idThread::Event_GetPersistantFloat
================
*/
void idThread::Event_GetPersistantFloat( const char *key ) {
	float result;

	gameLocal.persistentLevelInfo.GetFloat( key, "0", result );
	ReturnFloat( result );
}

/*
================
idThread::Event_GetPersistantVector
================
*/
void idThread::Event_GetPersistantVector( const char *key ) {
	idVec3 result;

	gameLocal.persistentLevelInfo.GetVector( key, "0 0 0", result );
	ReturnVector( result );
}

/*
================
idThread::Event_AngToForward
================
*/
void idThread::Event_AngToForward( const idAngles &ang ) {
	ReturnVector( ang.ToForward() );
}

/*
================
idThread::Event_AngToRight
================
*/
void idThread::Event_AngToRight( const idAngles &ang ) {
	idVec3 vec;

	ang.ToVectors( NULL, &vec );
	ReturnVector( vec );
}

/*
================
idThread::Event_AngToUp
================
*/
void idThread::Event_AngToUp( const idAngles &ang ) {
	idVec3 vec;

	ang.ToVectors( NULL, NULL, &vec );
	ReturnVector( vec );
}

/*
================
idThread::Event_GetSine
================
*/
void idThread::Event_GetSine( float angle ) {
	ReturnFloat( idMath::Sin( DEG2RAD( angle ) ) );
}

/*
================
idThread::Event_GetCosine
================
*/
void idThread::Event_GetCosine( float angle ) {
	ReturnFloat( idMath::Cos( DEG2RAD( angle ) ) );
}

#ifdef _D3XP
/*
================
idThread::Event_GetArcSine
================
*/
void idThread::Event_GetArcSine( float a ) {
	ReturnFloat(RAD2DEG(idMath::ASin(a)));
}

/*
================
idThread::Event_GetArcCosine
================
*/
void idThread::Event_GetArcCosine( float a ) {
	ReturnFloat(RAD2DEG(idMath::ACos(a)));
}
#endif

void idThread::Event_GetArcTan(float a) {
	ReturnFloat(RAD2DEG(idMath::ATan(a)));
}

/*
================
idThread::Event_GetSquareRoot
================
*/
void idThread::Event_GetSquareRoot( float theSquare ) {
	ReturnFloat( idMath::Sqrt( theSquare ) );
}

/*
================
idThread::Event_VecNormalize
================
*/
void idThread::Event_VecNormalize( const idVec3 &vec ) {
	idVec3 n;

	n = vec;
	n.Normalize();
	ReturnVector( n );
}

/*
================
idThread::Event_VecLength
================
*/
void idThread::Event_VecLength( const idVec3 &vec ) {
	ReturnFloat( vec.Length() );
}

/*
================
idThread::Event_VecDotProduct
================
*/
void idThread::Event_VecDotProduct( const idVec3 &vec1, const idVec3 &vec2 ) {
	ReturnFloat( vec1 * vec2 );
}

/*
================
idThread::Event_VecCrossProduct
================
*/
void idThread::Event_VecCrossProduct( const idVec3 &vec1, const idVec3 &vec2 ) {
	ReturnVector( vec1.Cross( vec2 ) );
}

/*
================
idThread::Event_VecToAngles
================
*/
void idThread::Event_VecToAngles( const idVec3 &vec ) {
	idAngles ang = vec.ToAngles();
	ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

#ifdef _D3XP
/*
================
idThread::Event_VecToOrthoBasisAngles
================
*/
void idThread::Event_VecToOrthoBasisAngles( idVec3 &vec ) {
	idVec3 left, up;
	idAngles ang;

	vec.OrthogonalBasis( left, up );
	idMat3 axis( left, up, vec );

	ang = axis.ToAngles();

	ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

void idThread::Event_RotateVector( idVec3 &vec, idVec3 &ang ) {

	idAngles tempAng(ang);
	idMat3 axis = tempAng.ToMat3();
	idVec3 ret = vec * axis;
	ReturnVector(ret);

}
#endif

/*
================
idThread::Event_OnSignal
================
*/
void idThread::Event_OnSignal( int signal, idEntity *ent, const char *func ) {
	const function_t *function;

	assert( func );

	if ( !ent ) {
		Error( "Entity not found" );
	}

	if ( ( signal < 0 ) || ( signal >= NUM_SIGNALS ) ) {
		Error( "Signal out of range" );
	}

	function = gameLocal.program.FindFunction( func );
	if ( !function ) {
		Error( "Function '%s' not found", func );
	}

	ent->SetSignal( ( signalNum_t )signal, this, function );
}

/*
================
idThread::Event_ClearSignalThread
================
*/
void idThread::Event_ClearSignalThread( int signal, idEntity *ent ) {
	if ( !ent ) {
		Error( "Entity not found" );
	}

	if ( ( signal < 0 ) || ( signal >= NUM_SIGNALS ) ) {
		Error( "Signal out of range" );
	}

	ent->ClearSignalThread( ( signalNum_t )signal, this );
}

/*
================
idThread::Event_SetCamera
================
*/
void idThread::Event_SetCamera( idEntity *ent ) {
	if ( !ent ) {
		Error( "Camera entity not found." );
		return;
	}

	if ( !ent->IsType( idCamera::Type ) ) {
		Error( "Entity is not a camera." );
		return;
	}

	gameLocal.SetCamera( ( idCamera * )ent );
}

/*
================
idThread::Event_FirstPerson
================
*/
void idThread::Event_FirstPerson( void ) {
	gameLocal.SetCamera( NULL );
}

/*
================
idThread::Event_Trace
================
*/
void idThread::Event_Trace( const idVec3 &start, const idVec3 &end, const idVec3 &mins, const idVec3 &maxs, int contents_mask, idEntity *passEntity ) {
	if ( mins == vec3_origin && maxs == vec3_origin ) {
		gameLocal.clip.TracePoint( trace, start, end, contents_mask, passEntity );
	} else {
		gameLocal.clip.TraceBounds( trace, start, end, idBounds( mins, maxs ), contents_mask, passEntity );
	}
	ReturnFloat( trace.fraction );
}

/*
================
idThread::Event_TracePoint
================
*/
void idThread::Event_TracePoint( const idVec3 &start, const idVec3 &end, int contents_mask, idEntity *passEntity ) {
	gameLocal.clip.TracePoint( trace, start, end, contents_mask, passEntity );
	ReturnFloat( trace.fraction );
}

/*
================
idThread::Event_GetTraceFraction
================
*/
void idThread::Event_GetTraceFraction( void ) {
	ReturnFloat( trace.fraction );
}

/*
================
idThread::Event_GetTraceEndPos
================
*/
void idThread::Event_GetTraceEndPos( void ) {
	ReturnVector( trace.endpos );
}

/*
================
idThread::Event_GetTraceNormal
================
*/
void idThread::Event_GetTraceNormal( void ) {
	if ( trace.fraction < 1.0f ) {
		ReturnVector( trace.c.normal );
	} else {
		ReturnVector( vec3_origin );
	}
}

/*
================
idThread::Event_GetTraceEntity
================
*/
void idThread::Event_GetTraceEntity( void ) {
	if ( trace.fraction < 1.0f ) {
		ReturnEntity( gameLocal.entities[ trace.c.entityNum ] );
	} else {
		ReturnEntity( ( idEntity * )NULL );
	}
}

/*
================
idThread::Event_GetTraceJoint
================
*/
void idThread::Event_GetTraceJoint( void ) {
	if ( trace.fraction < 1.0f && trace.c.id < 0 ) {
		idAFEntity_Base *af = static_cast<idAFEntity_Base *>( gameLocal.entities[ trace.c.entityNum ] );
		if ( af && af->IsType( idAFEntity_Base::Type ) && af->IsActiveAF() ) {
			ReturnString( af->GetAnimator()->GetJointName( CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id ) ) );
			return;
		}
	}
	ReturnString( "" );
}

/*
================
idThread::Event_GetTraceBody
================
*/
void idThread::Event_GetTraceBody( void ) {
	if ( trace.fraction < 1.0f && trace.c.id < 0 ) {
		idAFEntity_Base *af = static_cast<idAFEntity_Base *>( gameLocal.entities[ trace.c.entityNum ] );
		if ( af && af->IsType( idAFEntity_Base::Type ) && af->IsActiveAF() ) {
			int bodyId = af->BodyForClipModelId( trace.c.id );
			idAFBody *body = af->GetAFPhysics()->GetBody( bodyId );
			if ( body ) {
				ReturnString( body->GetName() );
				return;
			}
		}
	}
	ReturnString( "" );
}

/*
================
idThread::Event_FadeIn
================
*/
void idThread::Event_FadeIn( const idVec3 &color, float time ) {
	idVec4		fadeColor;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( player ) {
		fadeColor.Set( color[ 0 ], color[ 1 ], color[ 2 ], 0.0f );
		player->playerView.Fade(fadeColor, SEC2MS( time ) );
	}
}

/*
================
idThread::Event_FadeOut
================
*/
void idThread::Event_FadeOut( const idVec3 &color, float time ) {
	idVec4		fadeColor;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( player ) {
		fadeColor.Set( color[ 0 ], color[ 1 ], color[ 2 ], 1.0f );
		player->playerView.Fade(fadeColor, SEC2MS( time ) );
	}
}

/*
================
idThread::Event_FadeTo
================
*/
void idThread::Event_FadeTo( const idVec3 &color, float alpha, float time ) {
	idVec4		fadeColor;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( player ) {
		fadeColor.Set( color[ 0 ], color[ 1 ], color[ 2 ], alpha );
		player->playerView.Fade(fadeColor, SEC2MS( time ) );
	}
}

/*
================
idThread::Event_SetShaderParm
================
*/
void idThread::Event_SetShaderParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_GLOBAL_SHADER_PARMS ) ) {
		Error( "shader parm index (%d) out of range", parmnum );
	}

	gameLocal.globalShaderParms[ parmnum ] = value;
}

/*
================
idThread::Event_StartMusic
================
*/
void idThread::Event_StartMusic( const char *text )
{	
	//gameSoundWorld->FadeOutMusic(0, 0.1f); //BC hack - when playing music, force music volume to default to zero

	gameSoundWorld->PlayShaderDirectly( text, SND_CHANNEL_MUSIC );
}

//SM
void idThread::Event_FadeOutMusic(float fadeToDb, float seconds)
{
	gameSoundWorld->FadeOutMusic(fadeToDb, seconds);
}

//blendo eric
void idThread::Event_SetMusicMultiplier(float multVar)
{
	gameSoundWorld->SetMusicMultiplier(multVar);
}


/*
================
idThread::Event_Warning
================
*/
void idThread::Event_Warning( const char *text ) {
	Warning( "%s", text );
}

/*
================
idThread::Event_Error
================
*/
void idThread::Event_Error( const char *text ) {
	Error( "%s", text );
}

/*
================
idThread::Event_StrLen
================
*/
void idThread::Event_StrLen( const char *string ) {
	int len;

	len = strlen( string );
	idThread::ReturnInt( len );
}

/*
================
idThread::Event_StrLeft
================
*/
void idThread::Event_StrLeft( const char *string, int num ) {
	int len;

	if ( num < 0 ) {
		idThread::ReturnString( "" );
		return;
	}

	len = strlen( string );
	if ( len < num ) {
		idThread::ReturnString( string );
		return;
	}

	idStr result( string, 0, num );
	idThread::ReturnString( result );
}

/*
================
idThread::Event_StrRight
================
*/
void idThread::Event_StrRight( const char *string, int num ) {
	int len;

	if ( num < 0 ) {
		idThread::ReturnString( "" );
		return;
	}

	len = strlen( string );
	if ( len < num ) {
		idThread::ReturnString( string );
		return;
	}

	idThread::ReturnString( string + len - num );
}

/*
================
idThread::Event_StrSkip
================
*/
void idThread::Event_StrSkip( const char *string, int num ) {
	int len;

	if ( num < 0 ) {
		idThread::ReturnString( string );
		return;
	}

	len = strlen( string );
	if ( len < num ) {
		idThread::ReturnString( "" );
		return;
	}

	idThread::ReturnString( string + num );
}

/*
================
idThread::Event_StrMid
================
*/
void idThread::Event_StrMid( const char *string, int start, int num ) {
	int len;

	if ( num < 0 ) {
		idThread::ReturnString( "" );
		return;
	}

	if ( start < 0 ) {
		start = 0;
	}
	len = strlen( string );
	if ( start > len ) {
		start = len;
	}

	if ( start + num > len ) {
		num = len - start;
	}

	idStr result( string, start, start + num );
	idThread::ReturnString( result );
}

/*
================
idThread::Event_StrToFloat( const char *string )
================
*/
void idThread::Event_StrToFloat( const char *string ) {
	float result;

	result = atof( string );
	idThread::ReturnFloat( result );
}

/*
================
idThread::Event_RadiusDamage
================
*/
void idThread::Event_RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignore, const char *damageDefName, float dmgPower ) {
	gameLocal.RadiusDamage( origin, inflictor, attacker, ignore, ignore, damageDefName, dmgPower );
}

void idThread::Event_RayDamage(const idVec3 &originPos, const idVec3 &targetPos, const char *damageDefName, int forever)
{
	idVec3 rayDir = targetPos - originPos;
	rayDir.Normalize();

	trace_t tr;
	idVec3 finalTargetPosition;
	if (forever <= 0)
	{
		//just go between the 2 points.
		finalTargetPosition = targetPos;
	}
	else
	{
		finalTargetPosition = originPos + rayDir * 2048;
	}

	gameLocal.clip.TracePoint(tr, originPos, finalTargetPosition, MASK_SOLID, NULL);

	if (tr.c.entityNum > 0 && tr.c.entityNum < MAX_GENTITIES - 2 && tr.fraction < 1)
	{
		if (gameLocal.entities[tr.c.entityNum])
		{
			if (gameLocal.entities[tr.c.entityNum]->fl.takedamage)
			{				
				gameLocal.entities[tr.c.entityNum]->AddDamageEffect(tr, rayDir * 128, damageDefName);
				gameLocal.entities[tr.c.entityNum]->Damage(NULL, NULL, rayDir, damageDefName, 1.0f, 0);
				gameLocal.entities[tr.c.entityNum]->AddForce(NULL, 0, tr.endpos, rayDir * 16384.0f);
			}
		}
	}

	
}

/*
================
idThread::Event_IsClient
================
*/
void idThread::Event_IsClient( void ) {
	idThread::ReturnFloat( gameLocal.isClient );
}

/*
================
idThread::Event_IsMultiplayer
================
*/
void idThread::Event_IsMultiplayer( void ) {
	idThread::ReturnFloat( gameLocal.isMultiplayer );
}

/*
================
idThread::Event_GetFrameTime
================
*/
void idThread::Event_GetFrameTime( void ) {
	idThread::ReturnFloat( MS2SEC( gameLocal.msec ) );
}

/*
================
idThread::Event_GetTicsPerSecond
================
*/
void idThread::Event_GetTicsPerSecond( void ) {
	idThread::ReturnFloat( USERCMD_HZ );
}

/*
================
idThread::Event_CacheSoundShader
================
*/
void idThread::Event_CacheSoundShader( const char *soundName ) {
	declManager->FindSound( soundName );
}

/*
================
idThread::Event_DebugLine
================
*/
void idThread::Event_DebugLine( const idVec3 &color, const idVec3 &start, const idVec3 &end, const float lifetime ) {
	gameRenderWorld->DebugLine( idVec4( color.x, color.y, color.z, 0.0f ), start, end, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_DebugArrow
================
*/
void idThread::Event_DebugArrow( const idVec3 &color, const idVec3 &start, const idVec3 &end, const int size, const float lifetime ) {
	gameRenderWorld->DebugArrow( idVec4( color.x, color.y, color.z, 0.0f ), start, end, size, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_DebugCircle
================
*/
void idThread::Event_DebugCircle( const idVec3 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const float lifetime ) {
	gameRenderWorld->DebugCircle( idVec4( color.x, color.y, color.z, 0.0f ), origin, dir, radius, numSteps, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_DebugBounds
================
*/
void idThread::Event_DebugBounds( const idVec3 &color, const idVec3 &mins, const idVec3 &maxs, const float lifetime ) {
	gameRenderWorld->DebugBounds( idVec4( color.x, color.y, color.z, 0.0f ), idBounds( mins, maxs ), vec3_origin, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_DrawText
================
*/
void idThread::Event_DrawText( const char *text, const idVec3 &origin, float scale, const idVec3 &color, const int align, const float lifetime ) {
	gameRenderWorld->DrawText( text, origin, scale, idVec4( color.x, color.y, color.z, 0.0f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), align, SEC2MS( lifetime ) );
}

/*
================
idThread::Event_InfluenceActive
================
*/
void idThread::Event_InfluenceActive( void ) {
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( player && player->GetInfluenceLevel() ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}




//BC ====================== CUSTOM SCRIPT CALLS ======================

void idThread::Event_MakePropZoo()
{
	int xPosition, yPosition;
	idFileList*	aseFiles;
	idStrList aseList;

	aseFiles = fileSystem->ListFilesTree("models/objects", ".ase", true);
	aseList = aseFiles->GetList();
	xPosition = 0; //Column. Goes horizontally.
	yPosition = 0; //Row. Each row is a prop with a new starting letter, A,B,C,D,etc

	#define ROW_GAPSIZE 256
	#define COLUMN_MIN_GAPSIZE 96

	idStr	lastPropName = "aaaaa";
	int		longest_Y_length = 0;

	for (int i = 0; i < aseList.Num(); i++)	
	{
		const idDeclEntityDef *textDef;
		const idDeclEntityDef *emitterDef;

		idDict args;
		idEntity* ent;
		idEntity* textEnt;
		idStr sanitizedName;

		sanitizedName = aseList[i].c_str();
		
		if (sanitizedName.Find("_cm.ase") > 0) //skip collision meshes.
			continue;


		//See if we need to start a new ROW. We do this by comparing the first letter of the prop name.
		idStr t1 = sanitizedName.Right(sanitizedName.Length() - 15);
		t1 = t1.Left(1);
		idStr t2 = lastPropName.Left(1);
		if (idStr::Icmp(t1, t2))
		{
			xPosition = 0;
			yPosition += max(ROW_GAPSIZE, longest_Y_length + 128);
			longest_Y_length = 0;
			lastPropName = t1;

			idEntity* textEnt2;
			idAngles textAngle2 = idAngles(0, 270, 89);
			args.Clear();
			textDef = gameLocal.FindEntityDef("info_text", false);
			args = textDef->dict;
			idStr displayLetter = lastPropName;
			displayLetter.ToUpper();
			args.Set("text", displayLetter.c_str());
			args.SetBool("playerOriented", false);
			args.SetFloat("textsize", 7);
			args.SetMatrix("rotation", textAngle2.ToMat3());
			args.SetBool("depthtest", false);			
			args.SetVector("origin", idVec3(-112, yPosition - 64, 0));
			gameLocal.SpawnEntityDef(args, &textEnt2, false);
		}


		//Spawn the prop.
		args.Clear();
		emitterDef = gameLocal.FindEntityDef("func_static", false);
		args = emitterDef->dict;
		args.Set("model",  aseList[i].c_str());
		args.SetFloat("angle", 270);
		gameLocal.SpawnEntityDef(args, &ent, false);

		//idBounds bb = ent->GetPhysics()->GetBounds();
		//common->Printf("", bb);
		int totalLength = idMath::Abs(ent->GetPhysics()->GetBounds()[0][0]) + idMath::Abs(ent->GetPhysics()->GetBounds()[1][0]);
		if (totalLength > longest_Y_length)
			longest_Y_length = totalLength;

		int currentRadius = idMath::Abs(ent->GetPhysics()->GetBounds()[1][1]); //right bound of model.
		int bottomEdgePosition = ent->GetPhysics()->GetBounds()[0][2]; //bottom bound of model.
		idVec3 propPosition = idVec3(xPosition + currentRadius, yPosition + ent->GetPhysics()->GetBounds()[1][0], 0);
		ent->SetOrigin(propPosition + idVec3(0, 0, .5f +abs(bottomEdgePosition))); //bump it upward a bit so it doesnt dig into the ground.

		//Spawn the label.
		idAngles textAngle = idAngles(0, 270, 89);
		sanitizedName.StripLeading("models/objects/");
		sanitizedName.StripFileExtension();
		args.Clear();
		textDef = gameLocal.FindEntityDef("info_text", false);
		args = textDef->dict;
		args.Set("text", sanitizedName);
		args.SetBool("playerOriented", false);
		args.SetFloat("textsize", .17f);
		//args.SetFloat("angle", 270);
		args.SetMatrix("rotation", textAngle.ToMat3());
		args.SetBool("depthtest", false);
		int lowerBound = idMath::Abs(ent->GetPhysics()->GetBounds()[1][0]);
		args.SetVector("origin", propPosition + idVec3(0, -lowerBound - 4,0));
		gameLocal.SpawnEntityDef(args, &textEnt, false);


		xPosition = propPosition.x + Max(COLUMN_MIN_GAPSIZE, currentRadius);
		
	}
	
	common->Printf("\n\n --- MakePropZoo done. Found %d props. ---\n\n", aseList.Num());

	fileSystem->FreeFileList(aseFiles);

	//gameLocal.GetLocalPlayer()->noclip = true; //turn on noclip.
}

void idThread::Event_MakeParticleZoo()
{
	//Spawn ALL the particle effects into the map. So that we can browse effects and see how they look in-game. This will spawn all the fx in a long row.
	int count, i;
	idStrList particleList;
	//bool secondRowActivated = false;	
	int xPosition = 0;
	int yPosition = 0;

	#define Z_POS 64

	#define PARTICLE_GAP 192
	#define ROW_GAP 256

	count = declManager->GetNumDecls(DECL_PARTICLE);

	for (i = 0; i < count; i++)
	{
		const idDecl *decl = declManager->DeclByIndex(DECL_PARTICLE, i, false);

		if (decl)
		{
			particleList.AddUnique( decl->GetName());
		}
	}

	particleList.Sort(); //alphabetize.

	idStr lastParticleName = "a";

	for (i = 0; i < particleList.Num(); i++)
	//for (i = 0; i < 64; i++)
	{
		idDict args;

		//Detect if we need to start a new row.
		idStr t1 = lastParticleName.Left(1);
		idStr t2 = particleList[i].Left(1);
		if (idStr::Icmp(t1, t2))
		{
			xPosition = 0;
			yPosition += ROW_GAP;
			lastParticleName = t2;

			const idDeclEntityDef *textDef;
			idEntity* textEnt2;
			idAngles textAngle2 = idAngles(0, 270, 89);
			args.Clear();
			textDef = gameLocal.FindEntityDef("info_text", false);
			args = textDef->dict;
			idStr displayLetter = lastParticleName;
			displayLetter.ToUpper();
			args.Set("text", displayLetter.c_str());
			args.SetBool("playerOriented", false);
			args.SetFloat("textsize", 7);
			args.SetMatrix("rotation", textAngle2.ToMat3());
			args.SetBool("depthtest", false);
			args.SetVector("origin", idVec3(-128, yPosition - 64, 0));
			gameLocal.SpawnEntityDef(args, &textEnt2, false);
		}


		idEntity* ent;
		idEntity* textEnt;		
		const idDeclEntityDef *emitterDef;
		const idDeclEntityDef *textDef;
		idVec3 particlePosition = idVec3(xPosition, yPosition, Z_POS);

		//Spawn the particle.
		args.Clear();
		emitterDef = gameLocal.FindEntityDef("func_emitter", false);
		args = emitterDef->dict;
		args.Set("model", va("%s.prt", particleList[i].c_str() ));
		args.SetBool("start_off", false);
		gameLocal.SpawnEntityDef(args, &ent, false);
		ent->SetOrigin(particlePosition);

		//Spawn the label.
		//idAngles textAngle = idAngles(0, 270, 89);
		args.Clear();
		textDef = gameLocal.FindEntityDef("info_text", false);
		args = textDef->dict;
		args.Set("text", particleList[i].c_str());
		args.SetBool("playerOriented", false);
		args.SetFloat("textsize", .2f);
		args.SetFloat("angle", 270);
		//args.SetMatrix("rotation", textAngle.ToMat3());
		gameLocal.SpawnEntityDef(args, &textEnt, false);
		textEnt->SetOrigin(idVec3(particlePosition.x, particlePosition.y, 4));

		xPosition += PARTICLE_GAP;

		//if ((i >= particleList.Num() / 2) && !secondRowActivated)
		//{
		//	secondRowActivated = true;
		//	zPosition = 0;
		//	xPosition = 0;
		//}
	}
}

void idThread::Event_MakeItemZoo()
{
	

	//Spawn all items into the map. So that we can browse effects and see how they look in-game. This will spawn all the fx in a long row.
	int count, i;
	idStrList itementityList;

	int xPosition = 0;
	int yPosition = 0;

	

	count = declManager->GetNumDecls(DECL_ENTITYDEF);

	for (i = 0; i < count; i++)
	{
		const idDecl* decl = declManager->DeclByIndex(DECL_ENTITYDEF, i, false);

		if (!decl)
			continue;

		//Filter only moveables and items.
		idStr defName = decl->GetName();

		if (defName.Find("moveable_generic", false) == 0 
			|| defName.Find("moveable_carepackage", false) == 0
			|| defName.Find("ammo_types", false) == 0) //blacklist
			continue;

		if (defName.Find("moveable_", false) == 0 || defName.Find("ammo_", false) == 0)
		{
			itementityList.AddUnique(decl->GetName());
		}
	}

	itementityList.Sort(); //alphabetize.	
	

	for (i = 0; i < itementityList.Num(); i++)
	{
		idDict args;
	
		//Detect if we need to start a new row.
		#define ROW_MAXLENGTH 1900
		if (xPosition > ROW_MAXLENGTH)
		{
			xPosition = 0;
			#define ROW_GAP 256
			yPosition += ROW_GAP;	
		}
		
		
		idEntity* ent;
		idEntity* textEnt;
		const idDeclEntityDef* entityDef;
		const idDeclEntityDef* textDef;
		idVec3 particlePosition = idVec3(xPosition, yPosition, 0);
	
		//Spawn the entity.
		args.Clear();
		entityDef = gameLocal.FindEntityDef(itementityList[i].c_str(), false);
		common->Printf("\nITEMZOO SPAWN: '%s'\n", itementityList[i].c_str());
		if (entityDef)
		{
			args = entityDef->dict;
			//args.Set("model", va("%s.prt", particleList[i].c_str()));
			//args.SetBool("start_off", false);
			args.SetVector("origin", particlePosition);
			gameLocal.SpawnEntityDef(args, &ent, false);
			//ent->SetOrigin(particlePosition);

			if (ent)
			{
				idVec3 inspectValue = ent->spawnArgs.GetVector("inspect_offset");
				if (inspectValue != vec3_zero)
				{
					//can be inspected. Draw green arrow.
					gameRenderWorld->DebugArrow(colorGreen, particlePosition + idVec3(0, 0, 128), particlePosition + idVec3(0, 0, 4), 4, 9000000);
				}
				
			}
		}
	
		//Spawn the label.
		//idAngles textAngle = idAngles(0, 270, 89);
		args.Clear();
		textDef = gameLocal.FindEntityDef("info_text", false);
		args = textDef->dict;
		args.Set("text", itementityList[i].c_str());
		args.SetBool("playerOriented", false);
		args.SetFloat("textsize", .1f);
		args.SetFloat("angle", 270);
		args.SetVector("origin", idVec3(particlePosition.x, particlePosition.y, 1));
		//args.SetMatrix("rotation", textAngle.ToMat3());
		gameLocal.SpawnEntityDef(args, &textEnt, false);



		//#define PARTICLE_GAP 48
		xPosition += 48;
	
		//if ((i >= particleList.Num() / 2) && !secondRowActivated)
		//{
		//	secondRowActivated = true;
		//	zPosition = 0;
		//	xPosition = 0;
		//}
	}
}

void idThread::Event_PlayFX(const char *fxName, const idVec3 &position, const idVec3 &angle)
{
	idMat3 dir;
	idAngles ang;
	idEntity *fx;

	ang = angle.ToAngles();
	ang.pitch += 90;
	dir = ang.ToMat3();

	//gameRenderWorld->DebugArrow(colorGreen, position, position + (ang.ToForward() * 128), 4, 5000);
	fx = idEntityFx::StartFx(fxName, &position, &dir, NULL, false);

	idThread::ReturnEntity(fx);
}

void idThread::Event_PlayParticle(const char *particleName, const idVec3 &position, const idVec3 &angle)
{
	idMat3 dir;
	idAngles ang;

	ang = angle.ToAngles();
	ang.pitch += 90;
	dir = ang.ToMat3();

	gameLocal.DoParticle(particleName, position, angle);
}

void idThread::Event_getClassEntity(const char *classname, int lastFound)
{
	int i;
	lastFound++;

	if (lastFound >= gameLocal.num_entities || lastFound < 0)
	{
		idThread::ReturnEntity(NULL);
		return;
	}

	for (i = lastFound; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		//check if the classname matches.

		idStr strClassname = classname;
		int subIndex = strClassname.Find("*", false, 0, -1);

		if (subIndex >= 0)
		{
			//Has a wildcard.
			idStr classnameChunk = strClassname.Mid(0, subIndex);
			idStr entityClassname = gameLocal.entities[i]->spawnArgs.GetString("classname");

			if (idStr::Icmpn(classnameChunk, entityClassname, subIndex) != 0)
			{
				continue;
			}
		}
		else if (idStr::Icmp(classname, gameLocal.entities[i]->spawnArgs.GetString("classname")) != 0) //No wildcard. Do normal check.
		{
			continue;
		}

		idThread::ReturnEntity(gameLocal.entities[i]);
		return;
	}

	idThread::ReturnEntity(NULL);
}

void idThread::Event_getClassCount(const char *classname)
{
	int i;
	int count = 0;

	for (i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		idStr strClassname = classname;
		int subIndex = strClassname.Find("*", false, 0, -1);

		if (subIndex >= 0)
		{
			//Has a wildcard.
			idStr classnameChunk = strClassname.Mid(0, subIndex);
			idStr entityClassname = gameLocal.entities[i]->spawnArgs.GetString("classname");

			if (idStr::Icmpn(classnameChunk, entityClassname, subIndex) != 0)
			{
				continue;
			}
		}
		else if (idStr::Icmp(classname, gameLocal.entities[i]->spawnArgs.GetString("classname")) != 0) //No wildcard. Do normal check.
		{
			continue;
		}

		if (gameLocal.entities[i]->health <= 0) //skip dead entities.
			continue;

		//Found match.
		count++;
	}

	idThread::ReturnInt(count);
}

void idThread::Event_floatRound(float value, float decimals)
{
	if (decimals <= 0)
		ReturnString(va("%.0f", value));
	else if (decimals == 1)
		ReturnString(va("%.1f", value));
	else if (decimals == 2)
		ReturnString(va("%.2f", value));
	else if (decimals == 3)
		ReturnString(va("%.3f", value));
	else if (decimals == 4)
		ReturnString(va("%.4f", value));
	else if (decimals == 5)
		ReturnString(va("%.5f", value));
	else
		ReturnString(va("%.6f", value));
}

//Return 1 if hit sky. Return 0 if NOT hit sky.
void idThread::Event_GetTraceSky(void)
{
	if (trace.c.material == NULL)
	{
		ReturnInt(0);
		return;
	}

	if (trace.c.material->GetSurfaceFlags() >= 256)
	{
		ReturnInt(1);
		return;
	}

	ReturnInt(0);
}

void idThread::Event_GetTraceGlass(void)
{
	if (trace.c.material == NULL)
	{
		ReturnInt(0);
		return;
	}

	if (trace.c.material->GetSurfaceFlags() == 64) //This... is probably not the right way of doing this
	{
		ReturnInt(1);
		return;
	}

	ReturnInt(0);
}

void idThread::Event_DebugArrowsimple(const idVec3 &end)
{
	gameRenderWorld->DebugArrow(idVec4(0,1,0, 0.0f),end  + idVec3(0,0,128), end, 4, 10000);
}

void idThread::Event_SpawnInterestpoint(const char *interestDefname, const idVec3 &position)
{
	gameLocal.SpawnInterestPoint(NULL, position, interestDefname);
}

void idThread::Event_ParseTimeMS(float value)
{
	ReturnString( gameLocal.ParseTimeMS(value));
}

void idThread::Event_ParseTimeSec(float value)
{
	ReturnString(gameLocal.ParseTime(value));
}

void idThread::Event_LoadMap(const char *mapName)
{
	if (gameLocal.GetLocalPlayer())
	{
		//Since we're leaving this map, update the level progression index.
		gameLocal.GetLocalPlayer()->UpdateLevelProgressionIndex();
	}

	if (mapName == NULL || mapName[0] == '\0')
	{
		gameLocal.Warning("Loadmap: empty map name.\n");
		return;
	}

	idStr command = idStr::Format("map %s", mapName);
	gameLocal.sessionCommand = command.c_str();
}

void idThread::Event_SetSlowmo(bool doSlowmo)
{
	gameLocal.SetSlowmo(doSlowmo);
}

void idThread::Event_SetAmbientLight(bool active)
{
	renderSystem->SetUseBlendoAmbience( active );
}

void idThread::Event_SetOverrideEfx(bool active, const char* name)
{
	gameSoundWorld->SetOverrideEfx(active, name);
}

void idThread::Event_getworldspawnint(const char *keyname)
{
	if (!gameLocal.world)
	{
		idThread::ReturnInt(0);
		return;
	}

	idThread::ReturnInt(gameLocal.world->spawnArgs.GetInt(keyname, "0"));
	return;
}

//Return entity if it's within a specified bounds value
void idThread::Event_getEntityInBounds(const char *name, const idVec3 &mins, const idVec3 &maxs)
{
	if (mins == vec3_zero && maxs == vec3_zero)
	{
		idThread::ReturnEntity(NULL);
		common->Warning("getEntityInBounds script: invalid bounds (0,0,0  0,0,0)\n");
		return;
	}

	for (idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsHidden())
			continue;

		idBounds boundTrigger = idBounds(mins, maxs);
		if (boundTrigger.ContainsPoint(ent->GetPhysics()->GetOrigin()))
		{
			idStr strClassname = name;
			int subIndex = strClassname.Find("*", false, 0, -1);
			if (subIndex >= 0)
			{
				//Has a wildcard.
				idStr classnameChunk = strClassname.Mid(0, subIndex);
				idStr entityClassname = ent->spawnArgs.GetString("classname");

				if (idStr::Icmpn(classnameChunk, entityClassname, subIndex) != 0)
				{
					continue;
				}
			}
			else if (idStr::Icmp(name, ent->spawnArgs.GetString("classname")) != 0) //No wildcard. Do normal check.
			{
				continue;
			}

			//Ignore items held by player.
			if (gameLocal.GetLocalPlayer()->GetCarryable() != NULL)
			{
				if (gameLocal.GetLocalPlayer()->GetCarryable() == ent)
				{
					continue;
				}
			}

			idThread::ReturnEntity(ent);
			return;
		}
	}
	
	idThread::ReturnEntity(NULL);
}

// Returns a random alphanumeric character (A-Z, a-z, 0-9)
void idThread::Event_RandomChar(void)
{
	// Casting from int to char creates a char with the ASCII code number of the integer.
	// 0 to 9 is 48 to 57
	// A to Z is 65 to 90
	// a to z is 97 to 122
	// In total, we have 62 characters (26 + 26 + 10)
	// Assume we zero-index this
	int offset = gameLocal.random.RandomInt(62);
	int index;

	if (offset <= 9)
	{
		index = 48 + offset;
	}
	else
	{
		offset -= 10;
		if (offset <= 25)
		{
			index = 65 + offset;
		}
		else
		{
			offset -= 26;
			index = 97 + offset;
		}
	}

	char randomChar = static_cast<char>(index);
	idThread::ReturnString(va("%c", randomChar));
}

void idThread::Event_IsAirlessAtPoint(const idVec3 &point)
{
	idThread::ReturnFloat((float)gameLocal.GetAirlessAtPoint(point));
}

// SW: Removes all entities matching the match string (wildcard supported) within the provided bounds.
// Intended for room cleanup/reset in tutorial
void idThread::Event_RemoveEntitiesWithinBounds(const char* matchString, const idVec3 &mins, const idVec3 &maxs)
{
	idEntity* entityList[MAX_GENTITIES];
	idEntity* ent;
	idBounds bounds;
	int numEntities;

	if (mins == vec3_zero && maxs == vec3_zero)
	{
		common->Warning("removeEntitiesWithinBounds: invalid bounds (0,0,0  0,0,0)\n");
		return;
	}

	bounds = idBounds(mins, maxs);

	numEntities = gameLocal.EntitiesWithinAbsBoundingbox(bounds, entityList, MAX_GENTITIES);

	for (int i = 0; i < numEntities; i++)
	{
		ent = entityList[i];

		if (!ent)
			continue;

		if (ent->IsHidden())
			continue;

		idStr strClassname = matchString;
		int subIndex = strClassname.Find("*", false, 0, -1);
		if (subIndex >= 0)
		{
			//Has a wildcard.
			idStr classnameChunk = strClassname.Mid(0, subIndex);
			idStr entityClassname = ent->spawnArgs.GetString("classname");

			if (idStr::Icmpn(classnameChunk, entityClassname, subIndex) != 0)
			{
				continue;
			}
		}
		else if (idStr::Icmp(matchString, ent->spawnArgs.GetString("classname")) != 0) //No wildcard. Do normal check.
		{
			continue;
		}

		// Ent matches the bounds and the match string. Remove it.
		ent->PostEventMS(&EV_Remove, 0);
	}
}

// SW: Clears all model decals
void idThread::Event_RemoveAllDecals(void)
{
	gameRenderWorld->RemoveAllDecals();
}

// SW: Removes all interestpoints from the world
void idThread::Event_ClearInterestPoints(void)
{
	gameLocal.ClearInterestPoints();
}

void idThread::Event_RequirementMet(const char* entityName, int removeItem)
{
	if (gameLocal.RequirementMet_Inventory(gameLocal.GetLocalPlayer(), entityName, removeItem))
	{
		idThread::ReturnInt(1);
		return;
	}
	
	idThread::ReturnInt(0);	
}

void idThread::Event_CacheGui(const char* guiName)
{
	bool success = false;
	idUserInterface* gui = uiManager->Alloc();
	if (gui) {
		success = gui->InitFromFile(guiName);
		uiManager->DeAlloc(gui);
	}
	if (!success)
	{
		common->Warning("sys.cacheGui failed to cache '%s'\n", guiName);
	}
}

void idThread::Event_CacheMaterial(const char* mtrName)
{
	if (!declManager->FindType(DECL_MATERIAL, mtrName))
	{
		common->Warning("sys.cacheMaterial failed to cache '%s'\n", mtrName);
	}
}

void idThread::Event_CacheFX(const char* fxName)
{
	if (!declManager->FindType(DECL_FX, fxName))
	{
		common->Warning("sys.cacheFX failed to cache '%s'\n", fxName);
	}
}

void idThread::Event_CacheSkin(const char* skinName)
{
	if (!declManager->FindType(DECL_SKIN, skinName))
	{
		common->Warning("sys.cacheSkin failed to cache '%s'\n", skinName);
	}
}

void idThread::Event_CacheEntityDef(const char* defName)
{
	if (gameLocal.FindEntityDef(defName, false) == NULL)
	{
		common->Warning("sys.cacheEntityDef failed to cache '%s'\n", defName);
	}
}

void idThread::Event_steamOpenStoreOverlay()
{
	common->g_SteamUtilities->OpenSteamOverlaypageStore();
}
