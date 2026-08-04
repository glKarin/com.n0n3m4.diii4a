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



#include "../Game_local.h"
#include "../Relations.h"
#include "../SndProp.h"
#include "../Objectives/MissionData.h"
#include "../Missions/MissionManager.h"

class CRelations;
class CsndProp;

const idEventDef EV_Thread_Execute( "<execute>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Thread_SetCallback( "<script_setcallback>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Thread_SetRenderCallback( "<script_setrendercallback>", EventArgs(), EV_RETURNS_VOID, "internal" );
																	
// script callable events
const idEventDef EV_Thread_TerminateThread( "terminate", EventArgs('d', "threadNumber", ""), EV_RETURNS_VOID, "Terminates a thread.");
const idEventDef EV_Thread_Pause( "pause", EventArgs(), EV_RETURNS_VOID, "Pauses the current thread." );
const idEventDef EV_Thread_Wait( "wait", EventArgs('f', "time", ""), EV_RETURNS_VOID, "Suspends execution of the current thread for the given number of seconds.");
const idEventDef EV_Thread_WaitFrame( "waitFrame", EventArgs(), EV_RETURNS_VOID, "Suspends execution of current thread for one game frame." );
const idEventDef EV_Thread_WaitFor( "waitFor", EventArgs('e', "mover", ""), EV_RETURNS_VOID, "Waits for the given entity to complete its move.");
const idEventDef EV_Thread_WaitForThread( "waitForThread", EventArgs('d', "threadNumber", ""), EV_RETURNS_VOID, "Waits for the given thread to terminate.");
const idEventDef EV_Thread_WaitForRender( "waitForRender", EventArgs('e', "e", ""), EV_RETURNS_VOID, 
	"Suspends the current thread until 'e' might have been rendered.\n" \
	"It's event based, so it doesn't waste CPU repeatedly checking inPVS().\n" \
	"e.inPVS() will very likely be true when the thread resumes.\n" \
	"If e.inPVS() is true, calling waitForRender() will probably just wait\n" \
	"a frame, unless D3 can figure out that the entity doesn't need to be rendered.\n" \
	"Optimizations regarding shadowcasting lights may not apply to this function -\n" \
	"it is based purely off whether or not the entity's bounding box is visible.");
const idEventDef EV_Thread_Print( "print", EventArgs('s', "text", ""), EV_RETURNS_VOID, "Prints the given string to the console.");
const idEventDef EV_Thread_PrintLn( "println", EventArgs('s', "text", ""), EV_RETURNS_VOID, "Prints the given line to the console.");
const idEventDef EV_Thread_Say( "say", EventArgs('s', "text", ""), EV_RETURNS_VOID, "Multiplayer - Print this line on the network");
const idEventDef EV_Thread_Assert( "assert", EventArgs('f', "condition", ""), EV_RETURNS_VOID, "Breaks if the condition is zero. (Only works in debug builds.)");
const idEventDef EV_Thread_Trigger( "trigger", EventArgs('e', "entityToTrigger", ""), EV_RETURNS_VOID, "Triggers the given entity.");
const idEventDef EV_Thread_SetCvar( "setcvar", EventArgs('s', "name", "", 's', "value", ""), EV_RETURNS_VOID, "Sets a cvar.");
const idEventDef EV_Thread_GetCvar( "getcvar", EventArgs('s', "name", ""), 's', "Returns the string for a cvar.");
const idEventDef EV_Thread_GetCvarF( "getcvarf", EventArgs('s', "name", ""), 'f', "Returns float value for a cvar.");
const idEventDef EV_Thread_Random( "random", EventArgs('f', "range", ""), 'f', "Returns a random value X where 0 <= X < range.");
const idEventDef EV_Thread_GetTime( "getTime", EventArgs(), 'f', "Returns the current game time in seconds." );
const idEventDef EV_Thread_KillThread( "killthread", EventArgs('s', "threadName", ""), EV_RETURNS_VOID, "Kills all threads with the specified name");
const idEventDef EV_Thread_SetThreadName( "threadname", EventArgs('s', "name", ""), EV_RETURNS_VOID, "Sets the name of the current thread.");
const idEventDef EV_Thread_GetEntity( "getEntity", EventArgs('s', "name", ""), 'e', "Returns a reference to the entity with the specified name.");
const idEventDef EV_Thread_Spawn( "spawn", EventArgs('s', "classname", ""), 'e', "Creates an entity of the specified classname and returns a reference to the entity.");
const idEventDef EV_Thread_CopySpawnArgs( "copySpawnArgs", EventArgs('e', "ent", ""), EV_RETURNS_VOID, "copies the spawn args from an entity");
const idEventDef EV_Thread_SetSpawnArg( "setSpawnArg", EventArgs('s', "key", "", 's', "value", ""), EV_RETURNS_VOID, "Sets a key/value pair to be used when a new entity is spawned.");
const idEventDef EV_Thread_SpawnString( "SpawnString", EventArgs('s', "key", "", 's', "default", ""), 's', "Returns the string for the given spawn argument." );
const idEventDef EV_Thread_SpawnFloat( "SpawnFloat", EventArgs('s', "key", "", 'f', "default", ""), 'f', "Returns the floating point value for the given spawn argument.");
const idEventDef EV_Thread_SpawnVector( "SpawnVector", EventArgs('s', "key", "", 'v', "default", ""), 'v', "Returns the vector for the given spawn argument.");
const idEventDef EV_Thread_ClearPersistentArgs( "clearPersistentArgs", EventArgs(), EV_RETURNS_VOID, "Clears data that persists between maps." );
const idEventDef EV_Thread_SetPersistentArg( "setPersistentArg", EventArgs('s', "key", "", 's', "value", ""), EV_RETURNS_VOID, "Sets a key/value pair that persists between maps");
const idEventDef EV_Thread_GetPersistentString( "getPersistentString", EventArgs('s', "key", ""), 's', "Returns the string for the given persistent arg");
const idEventDef EV_Thread_GetPersistentFloat( "getPersistentFloat", EventArgs('s', "key", ""), 'f', "Returns the floating point value for the given persistent arg");
const idEventDef EV_Thread_GetPersistentVector( "getPersistentVector", EventArgs('s', "key", ""), 'v', "Returns the vector for the given persistent arg");

// Returns the number of the current mission (0-based)
const idEventDef EV_Thread_GetCurrentMissionNum( "getCurrentMissionNum", EventArgs(), 'f', "Returns the number of the current mission (0-based, the first mission has number 0).");

// Returns the version of TDM as floats, 1.08 for v1.08 etc.
const idEventDef EV_Thread_GetTDMVersion( "getTDMVersion", EventArgs(), 'd', 
	"Get the current TDM version as integer. The value will be 108\n" \
	"for v1.08, 109 for v1.09 and 200 for v2.00 etc." );

const idEventDef EV_Thread_AngRotate( "angRotate", EventArgs('v', "angles1", "", 'v', "angles2", ""), 'v', "Rotates the given Euler angles by each other.");
const idEventDef EV_Thread_AngToForward( "angToForward", EventArgs('v', "angles", ""), 'v', "Returns a forward vector for the given Euler angles.");
const idEventDef EV_Thread_AngToRight( "angToRight", EventArgs('v', "angles", ""), 'v', "Returns a right vector for the given Euler angles.");
const idEventDef EV_Thread_AngToUp( "angToUp", EventArgs('v', "angles", ""), 'v', "Returns an up vector for the given Euler angles.");
const idEventDef EV_Thread_Sine( "sin", EventArgs('f', "degrees", ""), 'f', "Returns the sine of the given angle in degrees.");
const idEventDef EV_Thread_ASine("asin", EventArgs('f', "sine", ""), 'f', "Returns the angle in degrees with the given sine."); // grayman #4882
const idEventDef EV_Thread_Cosine( "cos", EventArgs('f', "degrees", ""), 'f', "Returns the cosine of the given angle in degrees.");
const idEventDef EV_Thread_ACosine("acos", EventArgs('f', "cosine", ""), 'f', "Returns the angle in degrees with the given cosine."); // grayman #4882
const idEventDef EV_Thread_Log( "log", EventArgs('f', "x", ""), 'f', "Returns the log of the given argument.");
const idEventDef EV_Thread_Pow( "pow", EventArgs('f', "x", "", 'f', "y", ""), 'f', "Returns the power of x to y.");
const idEventDef EV_Thread_Ceil( "ceil", EventArgs('f', "x", ""), 'f', "Returns the smallest integer that is greater than or equal to the given value.");
const idEventDef EV_Thread_Floor( "floor", EventArgs('f', "x", ""), 'f', "Returns the largest integer that is less than or equal to the given value.");
const idEventDef EV_Thread_Min( "min", EventArgs('f', "x", "", 'f', "y", ""), 'f', "returns the smaller of two provided float values.");
const idEventDef EV_Thread_Max( "max", EventArgs('f', "x", "", 'f', "y", ""), 'f', "returns the larger of two provided float values.");
const idEventDef EV_Thread_SquareRoot( "sqrt", EventArgs('f', "square", ""), 'f', "Returns the square root of the given number.");
const idEventDef EV_Thread_Normalize( "vecNormalize", EventArgs('v', "vec", ""), 'v', "Returns the normalized version of the given vector.");
const idEventDef EV_Thread_VecLength( "vecLength", EventArgs('v', "vec", ""), 'f', "Returns the length of the given vector.");
const idEventDef EV_Thread_VecDotProduct( "DotProduct", EventArgs('v', "vec1", "", 'v', "vec2", ""), 'f', "Returns the dot product of the two vectors.");
const idEventDef EV_Thread_VecCrossProduct( "CrossProduct", EventArgs('v', "vec1", "", 'v', "vec2", ""), 'v', "Returns the cross product of the two vectors.");
const idEventDef EV_Thread_VecToAngles( "VecToAngles", EventArgs('v', "vec", ""), 'v', "Returns Euler angles for the given direction.");
const idEventDef EV_Thread_VecRotate( "VecRotate", EventArgs('v', "vector", "", 'v', "angles", ""), 'v', "Rotates a vector by the specified angles.");
const idEventDef EV_Thread_GetInterceptTime( "getInterceptTime", 
	EventArgs('v', "velocityTarget", "current velocity of target",
			  'f', "speedInterceptor", "speed of interceptor",
			  'v', "positionTarget", "current position of target",
			  'v', "positionInterceptor", "starting position of interceptor"),
	'f', 
	"Returns how much time it will take for an interceptor like a projectile to intercept a moving target at the earliest possible opportunity. Returns 0 if an intercept is not possible or the speed of the target and interceptor are too similar.");

const idEventDef EV_Thread_OnSignal( "onSignal", EventArgs('d', "signalNum", "", 'e', "ent", "", 's', "functionName", ""), EV_RETURNS_VOID, "Sets a script callback function for when the given signal is raised on the given entity.");
const idEventDef EV_Thread_ClearSignal( "clearSignalThread", EventArgs('d', "signalNum", "", 'e', "ent", ""), EV_RETURNS_VOID, "Clears the script callback function set for when the given signal is raised on the given entity.");
const idEventDef EV_Thread_SetCamera( "setCamera", EventArgs('e', "cameraEnt", ""), EV_RETURNS_VOID, "Turns over view control to the given camera entity.");
const idEventDef EV_Thread_FirstPerson( "firstPerson", EventArgs(), EV_RETURNS_VOID, "Returns view control to the player entity." );
const idEventDef EV_Thread_Trace( "trace", 
	EventArgs('v', "start", "", 
			  'v', "end", "",
			  'v', "mins", "",
			  'v', "maxs", "",
			  'd', "contents_mask", "",
			  'e', "passEntity", ""), 
	'f', 
	"Returns the fraction of movement completed before the box from 'mins' to 'maxs' hits solid geometry\n" \
	"when moving from 'start' to 'end'. The 'passEntity' is considered non-solid during the move." );

const idEventDef EV_Thread_TracePoint( "tracePoint", 
	EventArgs('v', "start", "", 
			  'v', "end", "",
			  'd', "contents_mask", "",
			  'e', "passEntity", ""), 
	'f', 
	"Returns the fraction of movement completed before the trace hits solid geometry\n" \
	"when moving from 'start' to 'end'. The 'passEntity' is considered non-solid during the move." );

const idEventDef EV_Thread_GetTraceFraction( "getTraceFraction", EventArgs(), 'f', "Returns the fraction of movement completed during the last call to trace or tracePoint." );
const idEventDef EV_Thread_GetTraceEndPos( "getTraceEndPos", EventArgs(), 'v', "Returns the position the trace stopped due to a collision with solid geometry during the last call to trace or tracePoint");
const idEventDef EV_Thread_GetTraceNormal( "getTraceNormal", EventArgs(), 'v', "Returns the normal of the hit plane during the last call to trace or tracePoint");
const idEventDef EV_Thread_GetTraceEntity( "getTraceEntity", EventArgs(), 'e', "Returns a reference to the entity which was hit during the last call to trace or tracePoint");
const idEventDef EV_Thread_GetTraceJoint( "getTraceJoint", EventArgs(), 's', 
	"Returns the number of the skeletal joint closest to the location on the entity which was hit\n" \
	"during the last call to trace or tracePoint" );
const idEventDef EV_Thread_GetTraceBody( "getTraceBody", EventArgs(), 's', "Returns the number of the body part of the entity which was hit during the last call to trace or tracePoint" );
const idEventDef EV_Thread_GetTraceSurfType( "getTraceSurfType", EventArgs(), 's', "Returns the type of the surface (i.e. metal, snow) which was hit during the last call to trace or tracePoint" );
const idEventDef EV_Thread_FadeIn( "fadeIn", EventArgs('v', "color", "", 'f', "time", "in seconds"), EV_RETURNS_VOID, "Fades towards the given color over the given time in seconds.");
const idEventDef EV_Thread_FadeOut( "fadeOut", EventArgs('v', "color", "", 'f', "time", "in seconds"), EV_RETURNS_VOID, "Fades from the given color over the given time in seconds.");
const idEventDef EV_Thread_FadeTo( "fadeTo", EventArgs('v', "color", "", 'f', "alpha", "", 'f', "time", "in seconds"), EV_RETURNS_VOID, "Fades to the given color up to the given alpha over the given time in seconds.");
const idEventDef EV_Thread_StartMusic( "music", EventArgs('s', "shaderName", ""), EV_RETURNS_VOID, "Starts playing background music.");
const idEventDef EV_Thread_Error( "error", EventArgs('s', "text", ""), EV_RETURNS_VOID, "Issues an error.");
const idEventDef EV_Thread_Warning( "warning", EventArgs('s', "text", ""), EV_RETURNS_VOID, "Issues a warning.");
const idEventDef EV_Thread_StrLen( "strLength", EventArgs('s', "text", ""), 'd', "Returns the number of characters in the string" );
const idEventDef EV_Thread_StrLeft( "strLeft", EventArgs('s', "text", "", 'd', "num", ""), 's', "Returns a string composed of the first num characters" );
const idEventDef EV_Thread_StrRight( "strRight", EventArgs('s', "text", "", 'd', "num", ""), 's', "Returns a string composed of the last num characters" );
const idEventDef EV_Thread_StrSkip( "strSkip", EventArgs('s', "text", "", 'd', "num", ""), 's', "Returns the string following the first num characters" );
const idEventDef EV_Thread_StrMid( "strMid", EventArgs('s', "text", "", 'd', "start", "", 'd', "num", ""), 's', "Returns a string composed of the characters from start to start + num" );
const idEventDef EV_Thread_StrRemove( "strRemove", EventArgs('s', "text", "", 's', "remove", ""), 's', "Replace all occurances of the given substring with \"\". Example: StrRemove(\"abba\",\"bb\") results in \"aa\"."); // Tels #3854
const idEventDef EV_Thread_StrReplace( "strReplace", EventArgs('s', "text", "", 's', "remove", "", 's', "replace",""), 's', "Replace all occurances of the given string with the replacement string. Example: StrRemove(\"abba\",\"bb\",\"ccc\") results in \"accca\"."); // Tels #3854
const idEventDef EV_Thread_StrFind( "strFind", EventArgs('s', "text", "", 's', "find", "", 'd', "casesensitive","0", 'd',"start","0", 'd',"end","-1" ), 'd', "Return the position of the given substring, counting from 0, or -1 if not found."); // Tels #3854
const idEventDef EV_Thread_StrToFloat( "strToFloat", EventArgs('s', "text", ""), 'f', "Returns the numeric value of the given string." );
const idEventDef EV_Thread_StrToInt( "strToInt", EventArgs('s', "text", ""), 'd', "Returns the integer value of the given string." );
const idEventDef EV_Thread_RadiusDamage( "radiusDamage", 
	EventArgs('v', "origin", "",
			  'E', "inflictor", "the entity causing the damage",
			  'E', "attacker", "",
			  'E', "ignore", "an entity to not cause damage to",
			  's', "damageDefName", "",
			  'f', "dmgPower", "scales the damage (for cases where damage is dependent on time)"), 
	EV_RETURNS_VOID,
	"damages entities within a radius defined by the damageDef.  inflictor is the entity \n" \
	"causing the damage and can be the same as the attacker (in the case \n " \
	"of projectiles, the projectile is the inflictor, while the attacker is the character \n" \
	"that fired the projectile).");
const idEventDef EV_Thread_GetFrameTime( "getFrameTime", EventArgs(), 'f', "returns the length of time between game frames.  this is not related to renderer frame rate." );
const idEventDef EV_Thread_GetTicsPerSecond( "getTicsPerSecond", EventArgs(), 'f', "returns the number of game frames per second.  this is not related to renderer frame rate." );

const idEventDef EV_Thread_DebugLine( "debugLine", 
	EventArgs('v', "color", "", 'v', "start", "", 'v', "end", "", 'f', "lifetime", ""), EV_RETURNS_VOID, 
	"line drawing for debug visualization.  lifetime of 0 == 1 frame." );
const idEventDef EV_Thread_DebugArrow( "debugArrow", 
	EventArgs('v', "color", "", 'v', "start", "", 'v', "end", "", 'd', "size", "", 'f', "lifetime", ""), EV_RETURNS_VOID, 
	"line drawing for debug visualization.  lifetime of 0 == 1 frame." );
const idEventDef EV_Thread_DebugCircle( "debugCircle", 
	EventArgs('v', "color", "", 'v', "origin", "", 'v', "dir", "", 'f', "radius", "", 'd', "numSteps", "", 'f', "lifetime", ""), EV_RETURNS_VOID, 
	"line drawing for debug visualization.  lifetime of 0 == 1 frame." );
const idEventDef EV_Thread_DebugBounds( "debugBounds", 
	EventArgs('v', "color", "", 'v', "mins", "", 'v', "maxs", "", 'f', "lifetime", ""),
	EV_RETURNS_VOID, "line drawing for debug visualization.  lifetime of 0 == 1 frame." );
const idEventDef EV_Thread_DrawText( "drawText", 
	EventArgs('s', "text", "",
			  'v', "origin", "",
			  'f', "scale", "",
			  'v', "color", "",
			  'd', "align", "0 = left, 1 = center, 2 = right",
			  'f', "lifetime", ""),
	EV_RETURNS_VOID, 
	"text drawing for debugging. lifetime of 0 == 1 frame." );

const idEventDef EV_Thread_InfluenceActive( "influenceActive", EventArgs(), 'd', "Checks if an influence is active" );

//AI relationship manager events
const idEventDef EV_AI_GetRelationSys( "getRelation", EventArgs('d', "team1", "", 'd', "team2", ""), 'd', "");
const idEventDef EV_AI_SetRelation( "setRelation", EventArgs('d', "team1", "", 'd', "team2", "", 'd', "val", ""), EV_RETURNS_VOID, "");
const idEventDef EV_AI_OffsetRelation( "offsetRelation", EventArgs('d', "team1", "", 'd', "team2", "", 'd', "val", ""), EV_RETURNS_VOID, "");

// Dark Mod soundprop events
const idEventDef EV_TDM_SetPortSoundLoss( "setPortSoundLoss", EventArgs('d', "handle", "", 'f', "value", ""), EV_RETURNS_VOID, "Sound propagation scriptfunction on the sys object");
const idEventDef EV_TDM_SetPortAISoundLoss( "setPortAISoundLoss", EventArgs('d', "handle", "", 'f', "value", ""), EV_RETURNS_VOID, "AI sound propagation scriptfunction on the sys object");
const idEventDef EV_TDM_SetPortPlayerSoundLoss( "setPortPlayerSoundLoss", EventArgs('d', "handle", "", 'f', "value", ""), EV_RETURNS_VOID, "Player sound loss scriptfunction on the sys object");
const idEventDef EV_TDM_GetPortSoundLoss( "getPortSoundLoss", EventArgs('d', "handle", ""), 'f', "Sound propagation scriptfunction on the sys object");
const idEventDef EV_TDM_GetPortAISoundLoss( "getPortAISoundLoss", EventArgs('d', "handle", ""), 'f', "AI sound propagation scriptfunction on the sys object");
const idEventDef EV_TDM_GetPortPlayerSoundLoss( "getPortPlayerSoundLoss", EventArgs('d', "handle", ""), 'f', "Player sound loss  scriptfunction on the sys object");

// greebo: General water test function, tests if a point is in a liquid
const idEventDef EV_PointInLiquid( "pointInLiquid", EventArgs('v', "point", "", 'e', "ignoreEntity", ""), 'f', 
	"Checks if a point is in a liquid, returns 1 if this is the case.");

// tels: Translate a string into the current language
const idEventDef EV_Translate( "translate", EventArgs('s', "input", ""), 's', "Translates a string (like #str_12345) into the current language");

// greebo: Writes the string to the Darkmod.log file using DM_LOG
const idEventDef EV_LogString("logString", EventArgs('d', "logClass", "", 'd', "logType", "", 's', "output", ""), EV_RETURNS_VOID, "This is the script counterpart to DM_LOG");

// Propagates the string to the sessioncommand variable in gameLocal
const idEventDef EV_SessionCommand("sessionCommand", EventArgs('s', "cmd", ""), EV_RETURNS_VOID, "Sends the sessioncommand to the game");

const idEventDef EV_SaveConDump("saveConDump", EventArgs('s', "cmd", "", 's', "cmd", ""), EV_RETURNS_VOID, "Saves condump into FM directory; first argument is appended to dump filename, everything before last occurence of second argument is removed");

const idEventDef EV_HandleMissionEvent("handleMissionEvent", 
	EventArgs('e', "objEnt", "the entity that triggered this event (e.g. a readable)", 
			  'd', "eventType", "a numeric identifier (enumerated both in MissionData.h and tdm_defs.script) specifying the type of event", 
			  's', "argument", "an optional string parameter, eventtype-specific."), 
			  EV_RETURNS_VOID, 
			  "Generic interface for passing on mission events from scripts to the SDK. Available since TDM 1.02");

const idEventDef EV_Thread_CanPlant( "canPlant", 
	EventArgs('v', "traceStart", "", 'v', "traceEnd", "", 'E', "ignore", "", 'e', "vine", ""), 'f', "" );  // grayman #2787

// grayman #3132 - get main ambient light
const idEventDef EV_GetMainAmbientLight("getMainAmbientLight", EventArgs(), 'e', "Returns the entity of the main ambient light.");

// tels #3271 - get the difficulty level during this mission
const idEventDef EV_GetDifficultyLevel("getDifficultyLevel", EventArgs(), 'd', "Returns 0 (Easy), 1 (Medium) or 2 (Hard), depending on the difficulty level of the current mission.");
// SteveL #3304 - 2 scripting events to get mission statistics by Zbyl
const idEventDef EV_GetDifficultyName("getDifficultyName", EventArgs('d', "difficultyLevel", "0 (Easy), 1 (Medium), 2 (Hard)"), 's', "Returns the (translated) name of the difficulty level passed as the argument.");
const idEventDef EV_GetMissionStatistic("getMissionStatistic", EventArgs('s', "statisticName",
	"Can be one of (case insensitive):\n"
	"\tgamePlayTime: gameplay time in seconds\n"
	"\tdamageDealt: damage dealt to enemies\n"
	"\tdamageReceived: damage received by player\n"
	"\thealthReceived: health received by player\n"
	"\tpocketsPicked: pockets picked by player\n"
	"\tfoundLoot: loot found by player\n"
	"\tmissionLoot: total loot available in mission\n"
	"\ttotalTimePlayerSeen: total time the player was seen by enemies in seconds. Updates only when AI lose sight of player\n"
	"\tnumberTimesPlayerSeen: number of times player was seen by enemies\n"
	"\tnumberTimesAISuspicious: number of times AI was 'observant' or 'suspicious'. A single AI passing through both alert levels will add 2 to the score.\n"
	"\tnumberTimesAISearched: number of times AI was 'investigating' or 'searching'. A single AI passing through both alert levels will add 2 to the score.\n"
	"\tsightingScore: sighting score (number of times player was seen * weight)\n"
	"\tstealthScore: stealth score (sighting score + alerts * weights)\n"
	"\tkilledByPlayer: number of enemies killed by player\n"
	"\tknockedOutByPlayer: number of enemies knocked out by player\n"
	"\tbodiesFound: number of times enemies have spotted a body\n"
	"\tsecretsFound: number of secrets found by the player\n"
	"\tsecretsTotal: total number of secrets in the mission\n"
), 'f', "Returns current mission statistic.");

// SteveL #3802 -- Allow scripts to discover entties in the map
const idEventDef EV_GetNextEntity( "getNextEntity",
	EventArgs(
	's', "key", "Optional string: prefix for spawnarg key match. E.g. \"target\" will match \"target\", \"target1\" etc.",
	's', "value", "Optional string: spawnarg value to match. Can be used independently of ''key''. If ''key'' is not set, all spawnargs will be checked for the value.",
	'E', "lastMatch", "Last match: search will start after this entity. Use $null_entity or pass an uninitialized entity variable to start a new search." ),
	'e',
	"Discover all entities in the map. Returns $null_entity when no more found.");

// SteveL #3962 -- Allow scripts to use the World particle system
const idEventDef EV_EmitParticle( "emitParticle",
	EventArgs(  's', "particle",  "String: name of particle effect.",
				'f', "startTime", "Game seconds since map start: use sys.getTime() for the first call unless you want to "
								  "back-date the particle so that it starts part way through its cycle.",
				'f', "diversity", "Randomizer value between 0 and 1. All particles with the same diversity will have the same "
								  "path and rotation. Use sys.random(1) for a random path.",
				'v', "origin",	  "Origin of the particle effect.",
				'v', "angle",	  "Axis for the particle effect. Use $<entityname>.getAngles() to align the particle to an "
								  "entity. use '0 0 0' for an upright (world-aligned) particle effect."),
	'f',
	"Start a particle effect in the world without using an entity emitter. Will emit one quad per particle stage when first called with "
	"sys.getTime() as the start time. Designed to be called once per frame with the same startTime each call to achieve a normal particle "
	"effect, or on demand with sys.getTime() as the startTime for finer grained control, 1 quad at a time. Returns True (1) if there are "
	"more particles to be emitted from the stage, False (0) if the stage has released all its quads.");
const idEventDef EV_ProjectDecal( "projectDecal", EventArgs(
				'v', "traceOrigin", "Start of the trace.",
				'v', "traceEnd", "End of the trace.",
				'e', "passEntity", "This entity will be considered non-solid by the trace.",
				's', "decal", "Decal to be projected.",
				'f', "decalSize", "Size of the decal quad.",
				'f', "angle", "Angle of the decal quad."), 
	EV_RETURNS_VOID,	
	"Performs a trace from the specified origin and end positions, then projects a decal in that direction.");

const idEventDef EV_SetSecretsFound("setSecretsFound", EventArgs('f', "secrets", ""), EV_RETURNS_VOID, "Set how many secrets the player has found. Use getMissionStatistic() for getting the current value.");
const idEventDef EV_SetSecretsTotal("setSecretsTotal", EventArgs('f', "secrets", ""), EV_RETURNS_VOID, "Set how many secrets exist in the map in total. Use getMissionStatistic() for getting the current value.");

const idEventDef EV_PointIsInBounds( "pointIsInBounds", EventArgs(
														'v', "point", "test whether this point is in the bounds",
														'v', "mins", "minimal corner of the bounds", 
														'v', "maxs", "maximal corner of the bounds"),
	'd', "Returns true if the point is within the bounds specified.");
const idEventDef EV_GetLocationPoint("getLocationPoint", EventArgs('v', "point", "point whose location to check"), 'e',
	"Returns the idLocation entity corresponding to the specified point's location.");

const idEventDef EV_Thread_CallFunctionsByWildcard(
	"callFunctionsByWildcard", EventArgs('s', "functionName", ""), EV_RETURNS_VOID,
	"Calls global functions with names matching the specified wildcard in separate threads (in lexicographical order). "
	"INTERNAL: don't use in mission scripting!"
);

CLASS_DECLARATION( idClass, idThread )
	EVENT( EV_Thread_Execute,				idThread::Event_Execute )
	EVENT( EV_Thread_TerminateThread,		idThread::Event_TerminateThread )
	EVENT( EV_Thread_Pause,					idThread::Event_Pause )
	EVENT( EV_Thread_Wait,					idThread::Event_Wait )
	EVENT( EV_Thread_WaitFrame,				idThread::Event_WaitFrame )
	EVENT( EV_Thread_WaitFor,				idThread::Event_WaitFor )
	EVENT( EV_Thread_WaitForThread,			idThread::Event_WaitForThread )
	EVENT( EV_Thread_WaitForRender,			idThread::Event_WaitForRender )
	EVENT( EV_Thread_Print,					idThread::Event_Print )
	EVENT( EV_Thread_PrintLn,				idThread::Event_PrintLn )
	EVENT( EV_Thread_Say,					idThread::Event_Say )
	EVENT( EV_Thread_Assert,				idThread::Event_Assert )
	EVENT( EV_Thread_Trigger,				idThread::Event_Trigger )
	EVENT( EV_Thread_SetCvar,				idThread::Event_SetCvar )
	EVENT( EV_Thread_GetCvar,				idThread::Event_GetCvar )
	EVENT( EV_Thread_GetCvarF,				idThread::Event_GetCvarF )
	EVENT( EV_Thread_Random,				idThread::Event_Random )
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
	EVENT( EV_Thread_ClearPersistentArgs,	idThread::Event_ClearPersistentArgs )
	EVENT( EV_Thread_SetPersistentArg,		idThread::Event_SetPersistentArg )
	EVENT( EV_Thread_GetPersistentString,	idThread::Event_GetPersistentString )
	EVENT( EV_Thread_GetPersistentFloat,	idThread::Event_GetPersistentFloat )
	EVENT( EV_Thread_GetPersistentVector,	idThread::Event_GetPersistentVector )

	EVENT( EV_Thread_GetCurrentMissionNum,	idThread::Event_GetCurrentMissionNum )
	EVENT( EV_Thread_GetTDMVersion,			idThread::Event_GetTDMVersion )

	EVENT( EV_Thread_AngRotate,				idThread::Event_AngRotate )
	EVENT( EV_Thread_AngToForward,			idThread::Event_AngToForward )
	EVENT( EV_Thread_AngToRight,			idThread::Event_AngToRight )
	EVENT( EV_Thread_AngToUp,				idThread::Event_AngToUp )
	EVENT( EV_Thread_Sine,					idThread::Event_GetSine )
	EVENT( EV_Thread_ASine,					idThread::Event_GetASine ) // grayman #4882
	EVENT( EV_Thread_Cosine,				idThread::Event_GetCosine )
	EVENT( EV_Thread_ACosine,				idThread::Event_GetACosine ) // grayman #4882
	EVENT( EV_Thread_Log,					idThread::Event_GetLog )
	EVENT( EV_Thread_Pow,					idThread::Event_GetPow )
	EVENT( EV_Thread_Floor,					idThread::Event_GetFloor )
	EVENT( EV_Thread_Ceil,					idThread::Event_GetCeil )
	EVENT( EV_Thread_Min,					idThread::Event_GetMin )
	EVENT( EV_Thread_Max,					idThread::Event_GetMax )
	EVENT( EV_Thread_SquareRoot,			idThread::Event_GetSquareRoot )
	EVENT( EV_Thread_Normalize,				idThread::Event_VecNormalize )
	EVENT( EV_Thread_VecLength,				idThread::Event_VecLength )
	EVENT( EV_Thread_VecDotProduct,			idThread::Event_VecDotProduct )
	EVENT( EV_Thread_VecCrossProduct,		idThread::Event_VecCrossProduct )
	EVENT( EV_Thread_VecToAngles,			idThread::Event_VecToAngles )
	EVENT( EV_Thread_VecRotate,				idThread::Event_VecRotate )
	EVENT( EV_Thread_GetInterceptTime,		idThread::Event_GetInterceptTime )	
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
	EVENT( EV_Thread_GetTraceSurfType,		idThread::Event_GetTraceSurfType )
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
	EVENT( EV_Thread_StrRemove,				idThread::Event_StrRemove )
	EVENT( EV_Thread_StrReplace,			idThread::Event_StrReplace )
	EVENT( EV_Thread_StrFind,				idThread::Event_StrFind )
	EVENT( EV_Thread_StrToFloat,			idThread::Event_StrToFloat )
	EVENT( EV_Thread_StrToInt,				idThread::Event_StrToInt )
	EVENT( EV_Thread_RadiusDamage,			idThread::Event_RadiusDamage )
	EVENT( EV_Thread_GetFrameTime,			idThread::Event_GetFrameTime )
	EVENT( EV_Thread_GetTicsPerSecond,		idThread::Event_GetTicsPerSecond )
	EVENT( EV_CacheSoundShader,				idThread::Event_CacheSoundShader )
	EVENT( EV_PointIsInBounds,				idThread::Event_PointIsInBounds )
	EVENT( EV_GetLocationPoint,				idThread::Event_GetLocationPoint )
	EVENT( EV_Thread_DebugLine,				idThread::Event_DebugLine )
	EVENT( EV_Thread_DebugArrow,			idThread::Event_DebugArrow )
	EVENT( EV_Thread_DebugCircle,			idThread::Event_DebugCircle )
	EVENT( EV_Thread_DebugBounds,			idThread::Event_DebugBounds )
	EVENT( EV_Thread_DrawText,				idThread::Event_DrawText )
	EVENT( EV_Thread_InfluenceActive,		idThread::Event_InfluenceActive )

	EVENT( EV_AI_GetRelationSys,			idThread::Event_GetRelation )
	EVENT( EV_AI_SetRelation,				idThread::Event_SetRelation )
	EVENT( EV_AI_OffsetRelation,			idThread::Event_OffsetRelation )

	// grayman #3042 - allow AI- and Player-specific sound loss script access
	EVENT( EV_TDM_SetPortSoundLoss,			idThread::Event_SetPortAISoundLoss ) // legacy sets AI loss, as it always did
	EVENT( EV_TDM_SetPortAISoundLoss,		idThread::Event_SetPortAISoundLoss )
	EVENT( EV_TDM_SetPortPlayerSoundLoss,	idThread::Event_SetPortPlayerSoundLoss )
	EVENT( EV_TDM_GetPortSoundLoss,			idThread::Event_GetPortAISoundLoss ) // legacy gets AI loss, as it always did
	EVENT( EV_TDM_GetPortAISoundLoss,		idThread::Event_GetPortAISoundLoss )
	EVENT( EV_TDM_GetPortPlayerSoundLoss,	idThread::Event_GetPortPlayerSoundLoss )
	
	EVENT( EV_PointInLiquid,				idThread::Event_PointInLiquid )
	EVENT( EV_Translate,					idThread::Event_Translate )

	EVENT( EV_LogString,					idThread::Event_LogString )
	EVENT( EV_SessionCommand,				idThread::Event_SessionCommand )
	EVENT( EV_SaveConDump,					idThread::Event_SaveConDump )

	EVENT( EV_HandleMissionEvent,			idThread::Event_HandleMissionEvent )

	EVENT( EV_Thread_CanPlant,	 			idThread::Event_CanPlant )	// grayman #2787

	EVENT( EV_GetMainAmbientLight,			idThread::Event_GetMainAmbientLight )	// grayman #3132

	EVENT( EV_GetDifficultyLevel,			idThread::Event_GetDifficultyLevel )	// tels	   #3271
	EVENT( EV_GetDifficultyName,			idThread::Event_GetDifficultyName )		// SteveL #3304: 2 scriptevents
	EVENT( EV_GetMissionStatistic,			idThread::Event_GetMissionStatistic )	//               from Zbyl

	EVENT( EV_GetNextEntity,				idThread::Event_GetNextEntity )	// SteveL #3802
	EVENT( EV_EmitParticle,  				idThread::Event_EmitParticle )  // SteveL #3962
	EVENT( EV_ProjectDecal,  				idThread::Event_ProjectDecal )

	EVENT( EV_SetSecretsFound,				idThread::Event_SetSecretsFound )
	EVENT( EV_SetSecretsTotal,				idThread::Event_SetSecretsTotal )

	EVENT( EV_Thread_CallFunctionsByWildcard, idThread::Event_CallFunctionsByWildcard )
	END_CLASS

idThread			*idThread::currentThread = NULL;
int					idThread::threadIndex = 0;
idList<idThread *>	idThread::threadList;
idList<int>			idThread::posFreeList;
idHashIndex			idThread::threadNumsHash;
trace_t				idThread::trace;

#define VINE_TRACE_CONTENTS 1281 // grayman #2787 - CONTENTS_CORPSE|CONTENTS_BODY|CONTENTS_SOLID

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
	if ( g_debugScript.GetBool() ) {
		gameLocal.Printf( "%d: end thread (%d) '%s'\n", gameLocal.time, threadNum, threadName.c_str() );
	}

	assert(threadList[threadPos] == this);
	threadList[threadPos] = NULL;
	posFreeList.Append(threadPos);
	assert(threadNumsHash.First(threadNum) >= 0);
	threadNumsHash.Remove(threadNum, threadPos);

	// note that ThreadCallback usually removes itself from the array
	// that's why here we copy the list to make iteration over it safe
	idList<idThread*> waitersCopy = threadsWaitingForThis;
	for ( int i = 0; i < waitersCopy.Num(); i++ ) {
		idThread *thread = waitersCopy[i];
		assert( thread->WaitingOnThread() == this );
		thread->ThreadCallback( this );
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

	savefile->WriteInt( threadsWaitingForThis.Num() );
	for ( int i = 0; i < threadsWaitingForThis.Num(); i++ )
		savefile->WriteObject( threadsWaitingForThis[i] );

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
	// stgatilov: we change threadNum here, so we need to update already filled hash table
	threadNumsHash.Remove( threadNum, threadPos );
	savefile->ReadInt( threadNum );
	threadNumsHash.Add( threadNum, threadPos );

	savefile->ReadObject( reinterpret_cast<idClass *&>( waitingForThread ) );
	savefile->ReadInt( waitingFor );
	savefile->ReadInt( waitingUntil );

	int n;
	savefile->ReadInt( n );
	threadsWaitingForThis.SetNum( n );
	for ( int i = 0; i < threadsWaitingForThis.Num(); i++ )
		savefile->ReadObject( reinterpret_cast<idClass *&>( threadsWaitingForThis[i] ) );

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
	if (posFreeList.Num() == 0)
		posFreeList.Append(threadList.Append(NULL));
	threadPos = posFreeList.Pop();
	threadList[threadPos] = this;
	threadNumsHash.Add(threadNum, threadPos);

	creationTime = gameLocal.time;
	lastExecuteTime = 0;
	manualControl = false;

	waitingForThread = NULL;
	ClearWaitFor();

	interpreter.SetThread( this );
}

/*
================
idThread::GetThread
================
*/
idThread *idThread::GetThread( int num ) {
	for ( int i = threadNumsHash.First(num); i != -1; i = threadNumsHash.Next(i) ) {
		idThread *thread = threadList[i];
		assert( thread );
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
		if ( !threadList[i] )
			continue;
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
	posFreeList.Clear();
	threadNumsHash.Clear();

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
        len = static_cast<int>(strlen(name));
	}

	// kill only those threads whose name matches name
	num = threadList.Num();
	for( i = 0; i < num; i++ ) {
		thread = threadList[ i ];
		if ( !thread )
			continue;
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
			PostEventMS( &EV_Thread_Execute, USERCMD_MSEC );
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

/*
================
idThread::ClearWaitFor
================
*/
void idThread::ClearWaitFor( void ) {
	if ( waitingForThread ) {
		bool had = waitingForThread->threadsWaitingForThis.Remove( this );
		assert( had );
	}
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

	interpreter.Error( "%s", text );
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

	interpreter.Warning( "%s", text );
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
void idThread::ReturnFloat( const float value ) {
	gameLocal.program.ReturnFloat( value );
}

/*
================
idThread::ReturnInt
================
*/
void idThread::ReturnInt( const int value ) {
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
	Pause();
	waitingUntil = gameLocal.time + time;
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
		waitingUntil = gameLocal.time + USERCMD_MSEC;
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
		thread->threadsWaitingForThis.Append( this );
	}
}

/*
================
idThread::Event_WaitForRender

Waits for an entity to render before resuming.
================
*/
void idThread::Event_WaitForRender( idEntity* ent )
{
	if ( ent && ent->RespondsTo( EV_Thread_SetRenderCallback ) ) {
		ent->ProcessEvent( &EV_Thread_SetRenderCallback );
		if ( gameLocal.program.GetReturnedInteger() ) {
			Pause();
			waitingFor = ent->entityNumber;
		}
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
	//DM_LOG(LC_AAS, LT_DEBUG)LOGSTRING( "%s\r", text ); // grayman - uncomment when debugging scripts
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
		ent->Activate(NULL);
	}
}

/*
================
idThread::Event_SetCvar
================
*/
void idThread::Event_SetCvar( const char *name, const char *value ) const {
	cvarSystem->SetCVarMissionString( name, value );
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
idThread::Event_GetCvarFloat
================
*/
void idThread::Event_GetCvarF( const char *name ) const {
	ReturnFloat( cvarSystem->GetCVarFloat( name ) );
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
	if ( strstr( value, "${" ) ) { // 5236 workaround for strings > MAX_STRING_LEN
		idStr text( value );
		for ( int index = 0; index < spawnArgs.GetNumKeyVals(); index++ ) {
			auto keyVal = spawnArgs.GetKeyVal( index );
			idStr old = idStr::Fmt( "${%s}", keyVal->GetKey().c_str() );
			text.Replace( old.c_str(), keyVal->GetValue().c_str() );
		}
		spawnArgs.Set( key, text.c_str() );
		return;
	}
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
void idThread::Event_SpawnVector( const char *key, idVec3 &defaultvalue ) {
	idVec3 result;

	spawnArgs.GetVector( key, va( "%f %f %f", defaultvalue.x, defaultvalue.y, defaultvalue.z ), result );
	ReturnVector( result );
}

/*
================
idThread::Event_ClearPersistentArgs
================
*/
void idThread::Event_ClearPersistentArgs( void ) {
	gameLocal.persistentLevelInfo.Clear();
}


/*
================
idThread::Event_SetPersistentArg
================
*/
void idThread::Event_SetPersistentArg( const char *key, const char *value ) {
	gameLocal.persistentLevelInfo.Set( key, value );
}

/*
================
idThread::Event_GetPersistentString
================
*/
void idThread::Event_GetPersistentString( const char *key ) {
	const char *result;

	gameLocal.persistentLevelInfo.GetString( key, "", &result );
	ReturnString( result );
}

/*
================
idThread::Event_GetPersistentFloat
================
*/
void idThread::Event_GetPersistentFloat( const char *key ) {
	float result;

	gameLocal.persistentLevelInfo.GetFloat( key, "0", result );
	ReturnFloat( result );
}

/*
================
idThread::Event_GetPersistentVector
================
*/
void idThread::Event_GetPersistentVector( const char *key ) {
	idVec3 result;

	gameLocal.persistentLevelInfo.GetVector( key, "0 0 0", result );
	ReturnVector( result );
}

void idThread::Event_GetCurrentMissionNum()
{
	ReturnFloat(gameLocal.m_MissionManager->GetCurrentMissionIndex());
}

void idThread::Event_GetTDMVersion() const
{
	// Tels: #3232 Return version as 108, 109 etc.
	ReturnInt( GAME_API_VERSION );
}

/*
================
idThread::Event_AngRotate
================
*/
void idThread::Event_AngRotate( idAngles &ang1, idAngles& ang2 ) {
	idAngles ang3 = (ang1.ToMat3() * ang2.ToMat3()).ToAngles();
	ReturnVector(idVec3(ang3[0], ang3[1], ang3[2]));
}

/*
================
idThread::Event_AngToForward
================
*/
void idThread::Event_AngToForward( idAngles &ang ) {
	ReturnVector( ang.ToForward() );
}

/*
================
idThread::Event_AngToRight
================
*/
void idThread::Event_AngToRight( idAngles &ang ) {
	idVec3 vec;

	ang.ToVectors( NULL, &vec );
	ReturnVector( vec );
}

/*
================
idThread::Event_AngToUp
================
*/
void idThread::Event_AngToUp( idAngles &ang ) {
	idVec3 vec;

	ang.ToVectors( NULL, NULL, &vec );
	ReturnVector( vec );
}

/*
================
idThread::Event_GetSine
================
*/
void idThread::Event_GetSine( const float angle ) {
	ReturnFloat( idMath::Sin( DEG2RAD( angle ) ) );
}

/*
================
idThread::Event_GetASine
================
*/
void idThread::Event_GetASine(const float s) // grayman #4882
{
	float angle = idMath::ASin(s);
	ReturnFloat(RAD2DEG(angle));
}

/*
================
idThread::Event_GetCosine
================
*/
void idThread::Event_GetCosine( const float angle ) {
	ReturnFloat( idMath::Cos( DEG2RAD( angle ) ) );
}

/*
================
idThread::Event_GetACosine
================
*/
void idThread::Event_GetACosine(const float s) // grayman #4882
{
	float angle = idMath::ACos(s);
	ReturnFloat(RAD2DEG(angle));
}

/*
================
Tels: idThread::Event_GetLog
================
*/
void idThread::Event_GetLog( const float x ) {
	ReturnFloat( idMath::Log( x ) );
}

/*
================
Tels: idThread::Event_GetPow
================
*/
void idThread::Event_GetPow( const float x, const float y ) {
	ReturnFloat( idMath::Pow( x, y ) );
}

/*
Tels: idThread::Event_GetCeil - returns the smallest integer that is greater than or equal to the given value
================
*/
void idThread::Event_GetCeil( const float x ) {
	ReturnInt( idMath::Ceil( x ) );
}

/*
Tels: idThread::Event_GetFloor - returns the largest integer that is less than or equal to the given value
================
*/
void idThread::Event_GetFloor( const float x ) {
	ReturnInt( idMath::Floor( x ) );
}

/*
Dragofer: idThread::Event_Min - returns the smaller of two provided float values
================
*/
void idThread::Event_GetMin( const float x, const float y ) {
	ReturnFloat( idMath::Fmin( x, y ) );
}

/*
Dragofer: idThread::Event_Max - returns the larger of two provided float values
================
*/
void idThread::Event_GetMax( const float x, const float y ) {
	ReturnFloat( idMath::Fmax( x, y ) );
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
void idThread::Event_VecNormalize( idVec3 &vec ) {
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
void idThread::Event_VecLength( idVec3 &vec ) {
	ReturnFloat( vec.Length() );
}

/*
================
idThread::Event_VecDotProduct
================
*/
void idThread::Event_VecDotProduct( idVec3 &vec1, idVec3 &vec2 ) {
	ReturnFloat( vec1 * vec2 );
}

/*
================
idThread::Event_VecCrossProduct
================
*/
void idThread::Event_VecCrossProduct( idVec3 &vec1, idVec3 &vec2 ) {
	ReturnVector( vec1.Cross( vec2 ) );
}

/*
================
idThread::Event_VecToAngles
================
*/
void idThread::Event_VecToAngles( idVec3 &vec ) {
	idAngles ang = vec.ToAngles();
	ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

/*
================
idThread::Event_VecRotate
================
*/
void idThread::Event_VecRotate( idVec3 &vector, idAngles &angles ) {

	idMat3 axis		= angles.ToMat3();
	idVec3 new_vec	= vector * axis;

	ReturnVector( new_vec );
}

/*
================
idThread::Event_GetInterceptTime
================
*/
void idThread::Event_GetInterceptTime(idVec3 &velTarget, float speedInterceptor, idVec3 &posTarget, idVec3 &posInterceptor ) {

	ReturnFloat( idPolynomial::GetInterceptTime( velTarget, speedInterceptor, posTarget, posInterceptor) );
}

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
		Error( "Entity not found" );
		return;
	}

	if ( !ent->IsType( idCamera::Type ) ) {
		Error( "Entity is not a camera" );
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
idThread::Event_GetTraceSurfType
================
*/
void idThread::Event_GetTraceSurfType( void ) {
	if ( trace.fraction < 1.0f ) {
		idStr typeName = g_Global.GetSurfName( trace.c.material );
		ReturnString( typeName );
		return;
	}
	ReturnString( "" );
}

/*
================
idThread::Event_FadeIn
================
*/
void idThread::Event_FadeIn( idVec3 &color, float time ) {
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
void idThread::Event_FadeOut( idVec3 &color, float time ) {
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
void idThread::Event_FadeTo( idVec3 &color, float alpha, float time ) {
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
void idThread::Event_StartMusic( const char *text ) {
	gameSoundWorld->PlayShaderDirectly( text );
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
	
    int len = static_cast<int>(strlen(string));
	idThread::ReturnInt( len );
}

/*
================
idThread::Event_StrLeft
================
*/
void idThread::Event_StrLeft( const char *string, int num ) {

	if ( num < 0 ) {
		idThread::ReturnString( "" );
		return;
	}

    int len = static_cast<int>(strlen(string));
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

	if ( num < 0 ) {
		idThread::ReturnString( "" );
		return;
	}

    int len = static_cast<int>(strlen(string));
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

	if ( num < 0 ) {
		idThread::ReturnString( string );
		return;
	}

    int len = static_cast<int>(strlen(string));
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

	if ( num < 0 ) {
		idThread::ReturnString( "" );
		return;
	}

	if ( start < 0 ) {
		start = 0;
	}
    int len = static_cast<int>(strlen(string));
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
idThread::Event_StrRemove
================
*/
void idThread::Event_StrRemove( const char *string, const char *remove ) {
	
    int slen = static_cast<int>(strlen(string));
    int rlen = static_cast<int>(strlen(remove));
	// nothing to replace (slen==0), remove longer than input string, or remove empty?
	if ( slen < rlen || rlen == 0 ) {
		ReturnString( string );
		return;
	}

	idStr result( string );
	result.Remove(remove);

	ReturnString( result );
}

/*
================
idThread::Event_StrReplace
================
*/
void idThread::Event_StrReplace( const char *string, const char *remove, const char *replace ) {
	
	int slen = static_cast<int>(strlen( string ));
	int rlen = static_cast<int>(strlen( remove ));
	// nothing to replace (slen==0), remove longer than input string, or remove empty?
	if ( slen < rlen || rlen == 0 ) {
		ReturnString( string );
		return;
	}

	idStr result( string );
	result.Replace(remove, replace);

	ReturnString( result );
}

/*
================
idThread::Event_StrFind
================
*/
void idThread::Event_StrFind( const char *string, const char *find, const int mcasesensitive, const int mstart, const int mend ) {
	const int slen = static_cast<int>(strlen( string ));
	const int flen = static_cast<int>(strlen( find ));

	// avoid FindText() calling strlen again
	// mend == -1 => go to the end
	// mend >= string length => go to the end
	// mend  < string length => go to mend
	const int end = (mend < 0 || mend > slen) ? slen : mend;

	// nothing to find, or find > input?
	if ( slen < flen || flen == 0 ) {
		ReturnInt( -1 );
		return;
	}

	int rc = idStr::FindText( string, find, (mcasesensitive != 0) ? true : false, (mstart > 0) ? mstart : 0, end );

	ReturnInt( rc );
}

/*
================
idThread::Event_StrToFloat( const char *string )
================
*/
void idThread::Event_StrToFloat( const char *string ) {
	float result;

	result = atof( string );
	ReturnFloat( result );
}

/*
================
tels: idThread::Event_StrToInt( const char *string )
================
*/
void idThread::Event_StrToInt( const char *string ) {
	int result;

	result = atoi( string );
	ReturnInt( result );
}

/*
================
idThread::Event_RadiusDamage
================
*/
void idThread::Event_RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignore, const char *damageDefName, float dmgPower ) {
	gameLocal.RadiusDamage( origin, inflictor, attacker, ignore, ignore, damageDefName, dmgPower );
}

/*
================
idThread::Event_GetFrameTime
================
*/
void idThread::Event_GetFrameTime( void ) { 
	idThread::ReturnFloat( MS2SEC( USERCMD_MSEC ) );
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
idThread::Event_PointIsInBounds
================
*/
void idThread::Event_PointIsInBounds(const idVec3 &point, const idVec3 &mins, const idVec3 &maxs) {

	idVec3 corner1 = mins;
	idVec3 corner2 = maxs;

	//make sure corner1 is the minimum and corner2 is the maximum
	if (corner1[0] > corner2[0])
	{
		corner1[0] = maxs[0];
		corner2[0] = mins[0];
	}

	if (corner1[1] > corner2[1])
	{
		corner1[1] = maxs[1];
		corner2[1] = mins[1];
	}

	if (corner1[2] > corner2[2])
	{
		corner1[2] = maxs[2];
		corner2[2] = mins[2];
	}

	idBounds bounds( corner1, corner2 );
	bool retInt = bounds.ContainsPoint( point );

	idThread::ReturnInt( retInt );
}

/*
================
idThread::Event_GetLocationPoint
================
*/
void idThread::Event_GetLocationPoint( const idVec3 &point )
{
	idThread::ReturnEntity( gameLocal.LocationForPoint( point ) );
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
	gameRenderWorld->DebugText( text, origin, scale, idVec4( color.x, color.y, color.z, 0.0f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), align, SEC2MS( lifetime ) );
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

/**
* DarkMod: The following script events are a frontend for
* the global AI relationship manager (stored in game_local)
**/
void	idThread::Event_GetRelation( int team1, int team2 )
{
	idThread::ReturnInt( gameLocal.m_RelationsManager->GetRelNum( team1, team2 ) );
}

void	idThread::Event_SetRelation( int team1, int team2, int val )
{
	gameLocal.m_RelationsManager->SetRel( team1, team2, val );
}

void	idThread::Event_OffsetRelation( int team1, int team2, int offset )
{
	gameLocal.m_RelationsManager->ChangeRel( team1, team2, offset );
}

void idThread::Event_SetPortAISoundLoss( int handle, float value )
{
	gameLocal.m_sndProp->SetPortalAILoss( handle, value );
}

void idThread::Event_SetPortPlayerSoundLoss( int handle, float value )
{
	gameLocal.m_sndProp->SetPortalPlayerLoss( handle, value );
}

void idThread::Event_GetPortAISoundLoss( int handle )
{
	idThread::ReturnFloat( gameLocal.m_sndProp->GetPortalAILoss( handle ) );
}

void idThread::Event_GetPortPlayerSoundLoss( int handle )
{
	idThread::ReturnFloat( gameLocal.m_sndProp->GetPortalPlayerLoss( handle ) );
}

void idThread::Event_LogString(int logClass, int logType, const char* output) 
{
	DM_LOG(static_cast<LC_LogClass>(logClass), static_cast<LT_LogType>(logType))LOGSTRING(const_cast<char*>(output));
}

/*
================
idThread::CallFunctionArgs

NOTE: If this is called from within a event called by this thread, the function arguments will be invalid after calling this function.
================
*/
bool idThread::CallFunctionArgs(const function_t *func, bool clearStack, const char *fmt, ...)
{
	bool rc = false;
	va_list argptr;

	ClearWaitFor();

	va_start(argptr, fmt);
	rc = interpreter.EnterFunctionVarArgVN(func, clearStack, fmt, argptr);
	va_end(argptr);

	return rc;
}

bool idThread::CallFunctionArgsVN(const function_t *func, bool clearStack, const char *fmt, va_list args)
{
	bool rc = false;

	ClearWaitFor();

	rc = interpreter.EnterFunctionVarArgVN(func, clearStack, fmt, args);

	return rc;
}

void idThread::Event_PointInLiquid( const idVec3 &point, idEntity* ignoreEntity ) {
	// Check if the point is in water
	int contents = gameLocal.clip.Contents( point, NULL, mat3_identity, -1, ignoreEntity );
	ReturnFloat( (contents & MASK_WATER) ? 1 : 0);
}

void idThread::Event_Translate( const char* input ) {
	// Tels: #3193 - Translate a string template into the current language
	ReturnString( common->Translate(input) );
}

void idThread::Event_SessionCommand(const char* cmd)
{
	gameLocal.sessionCommand = cmd;
}

void idThread::Event_SaveConDump(const char *filename, const char *startline)
{
	static int numberOfTimesSaved = 0;
	if (++numberOfTimesSaved > 100) {
		gameLocal.Warning("Event_SaveConDump called %d time: suppressed", numberOfTimesSaved);
		return;
	}

	idStr fn = filename;
	//avoid too lengthy filenames
	fn.CapLength(20);
	//replace all chars except for letters, digits, and underscores
	for (int i = 0; i < fn.Length(); i++) {
		char ch = fn[i];
		if (ch >= 0 && (isalnum(ch) || ch == '_'))
			continue;
		if (ch == '.') {
			//if extension is set, drop it
			fn.CapLength(i);
			break;
		}
		fn[i] = '_';
	}
	//avoid empty name
	if (fn.Length() == 0)
		fn = "default";
	fn = "condump_" + fn + ".txt";

	//execute console command (right now)
	idStr command = idStr::Fmt("condump %s unwrap modsavepath\n", fn.c_str());
	cmdSystem->BufferCommandText(CMD_EXEC_NOW, command.c_str());

	if (strlen(startline) > 0) {
		//search for special "start line" and remove everything before it
		//this allows mapper to print exact text into the file
		idFile *f = fileSystem->OpenFileRead(fn);
		if (f) {
			//prepare string to search for
			idStr needle = startline;
			#ifdef _WIN32
			//let's hope EOL style is detected correctly on out platforms...
			needle += '\r';
			#endif
			needle += '\n';
			//read file
			idList<char> text;
			text.SetNum(f->Length() + 1);
			f->Read(text.Ptr(), f->Length());
			text[f->Length()] = 0;
			fileSystem->CloseFile(f);
			//search for last occurence of string
			const char *last = nullptr;
			int pos = 0;
			while (1) {
				int newpos = idStr::FindText(text.Ptr(), needle, true, pos);
				if (newpos < 0)
					break;
				pos = newpos + needle.Length();
				last = text.Ptr() + pos;
			}
			if (last) {
				//resave without starting text
				idFile *f = fileSystem->OpenFileWrite(fn);
				f->Write(last, strlen(last));
				fileSystem->CloseFile(f);
			}
		}
	}
}

void idThread::Event_HandleMissionEvent(idEntity* entity, int eventType, const char* argument)
{
	// Safety check the enum
	if (eventType < EVENT_NOTHING || eventType >= EVENT_INVALID)
	{
		gameLocal.Warning("Invalid mission event type passed by %s to handleMissionEvent(): %d", entity->name.c_str(), eventType);
		return;
	}

	// Pass on the call
	gameLocal.m_MissionData->HandleMissionEvent(entity, static_cast<EMissionEventType>(eventType), argument);
}

// grayman #2787

void idThread::Event_CanPlant( const idVec3 &traceStart, const idVec3 &traceEnd, idEntity *ignore, idEntity *vine )
{
	float vineFriendly = 0; // assume the vine can't grow
	trace_t result;
	if ( gameLocal.clip.TracePoint( result, traceStart, traceEnd, VINE_TRACE_CONTENTS, ignore ) )
	{
		int contents = gameLocal.clip.Contents( result.endpos, NULL, mat3_identity, -1, ignore );
		if ( !( contents & MASK_WATER ) ) // grow if not in water
		{
			if ( abs( result.c.normal.z ) < 0.866 ) // grow if not on a flat surface
			{
				idEntity* struckEnt = gameLocal.entities[result.c.entityNum];
				if ( struckEnt )
				{
					if ( ( struckEnt == gameLocal.world ) || struckEnt->IsType( idStaticEntity::Type ) )
					{
						// Either we hit a world brush, or a func_static. We can grow on both types.

						const idMaterial* material = result.c.material;
						if ( material )
						{
							idStr description = material->GetDescription();
							if ( idStr::FindText(description,"vine_friendly") >= 0 )
							{
								// We can plant here, so save planting data on the vine entity

								vine->m_VinePlantLoc = result.endpos;
								vine->m_VinePlantNormal = result.c.normal;
								vineFriendly = 1;
							}
						}
					}
				}
			}
		}
	}

	idThread::ReturnFloat( vineFriendly );
}

// grayman #3132
void idThread::Event_GetMainAmbientLight()
{
	idLight* light = gameLocal.FindMainAmbientLight(false);
	idThread::ReturnEntity(light);
}

// tels #3271
void idThread::Event_GetDifficultyLevel()
{
	int level = gameLocal.m_DifficultyManager.GetDifficultyLevel();
	idThread::ReturnInt(level);
}

// SteveL #3304: 2 new scriptevents from Zbyl
void idThread::Event_GetDifficultyName( int level )
{
	if ( level < 0 || level >= DIFFICULTY_COUNT )
	{
		gameLocal.Warning("Invalid difficulty level passed to getDifficultyName(): %d", level);
		idThread::ReturnString("");
		return;
	}

	idStr difficultyName = gameLocal.m_DifficultyManager.GetDifficultyName(level);
	idThread::ReturnString(difficultyName);
}

void idThread::Event_GetMissionStatistic( const char* statisticName )
{
	if (idStr::Icmp("gamePlayTime", statisticName) == 0)
	{
		unsigned int time = gameLocal.m_GamePlayTimer.GetTimeInSeconds();
		idThread::ReturnFloat(time);
		return;
	}

	if (idStr::Icmp("damageDealt", statisticName) == 0)
	{
		int damageDealt = gameLocal.m_MissionData->GetDamageDealt();
		idThread::ReturnFloat(damageDealt);
		return;
	}

	if (idStr::Icmp("damageReceived", statisticName) == 0)
	{
		int damageReceived = gameLocal.m_MissionData->GetDamageReceived();
		idThread::ReturnFloat(damageReceived);
		return;
	}

	if (idStr::Icmp("healthReceived", statisticName) == 0)
	{
		int healthReceived = gameLocal.m_MissionData->GetHealthReceived();
		idThread::ReturnFloat(healthReceived);
		return;
	}

	if (idStr::Icmp("pocketsPicked", statisticName) == 0)
	{
		int pocketsPicked = gameLocal.m_MissionData->GetPocketsPicked();
		idThread::ReturnFloat(pocketsPicked);
		return;
	}

	if (idStr::Icmp("foundLoot", statisticName) == 0)
	{
		int foundLoot = gameLocal.m_MissionData->GetFoundLoot();
		idThread::ReturnFloat(foundLoot);
		return;
	}

	if (idStr::Icmp("missionLoot", statisticName) == 0)
	{
		int missionLoot = gameLocal.m_MissionData->GetMissionLoot();
		idThread::ReturnFloat(missionLoot);
		return;
	}

	if (idStr::Icmp("totalTimePlayerSeen", statisticName) == 0)
	{
		int timeSeen = gameLocal.m_MissionData->GetTotalTimePlayerSeen();
		idThread::ReturnFloat(timeSeen / 1000.0); // convert to seconds
		return;
	}

	if (idStr::Icmp("numberTimesPlayerSeen", statisticName) == 0)
	{
		int timesSeen = gameLocal.m_MissionData->GetNumberTimesPlayerSeen();
		idThread::ReturnFloat(timesSeen);
		return;
	}

	if (idStr::Icmp("numberTimesAISuspicious", statisticName) == 0)
	{
		int timesSuspicious = gameLocal.m_MissionData->GetNumberTimesAISuspicious();
		idThread::ReturnFloat(timesSuspicious);
		return;
	}

	if (idStr::Icmp("numberTimesAISearched", statisticName) == 0)
	{
		int timesSearched = gameLocal.m_MissionData->GetNumberTimesAISearched();
		idThread::ReturnFloat(timesSearched);
		return;
	}

	if (idStr::Icmp("sightingScore", statisticName) == 0)
	{
		float sightingScore = gameLocal.m_MissionData->GetSightingScore();
		idThread::ReturnFloat(sightingScore);
		return;
	}

	if (idStr::Icmp("stealthScore", statisticName) == 0)
	{
		float stealthScore = gameLocal.m_MissionData->GetStealthScore();
		idThread::ReturnFloat(stealthScore);
		return;
	}

	if (idStr::Icmp("killedByPlayer", statisticName) == 0)
	{
		int killed = gameLocal.m_MissionData->GetStatOverall(COMP_KILL);
		idThread::ReturnFloat(killed);
		return;
	}

	if (idStr::Icmp("knockedOutByPlayer", statisticName) == 0)
	{
		int knockedOut = gameLocal.m_MissionData->GetStatOverall(COMP_KO);
		idThread::ReturnFloat(knockedOut);
		return;
	}

	if (idStr::Icmp("bodiesFound", statisticName) == 0)
	{
		int bodiesFound = gameLocal.m_MissionData->GetStatOverall(COMP_AI_FIND_BODY);
		idThread::ReturnFloat(bodiesFound);
		return;
	}
	if (idStr::Icmp("totalSaves", statisticName) == 0)
	{
		int totalSaves = gameLocal.m_MissionData->getTotalSaves();
		idThread::ReturnFloat(totalSaves);
		return;
	}
	if (idStr::Icmp("secretsFound", statisticName) == 0)
	{
		int foundSecrets = gameLocal.m_MissionData->GetSecretsFound();
		idThread::ReturnFloat(foundSecrets);
		return;
	}
	if (idStr::Icmp("secretsTotal", statisticName) == 0)
	{
		int totalSecrets = gameLocal.m_MissionData->GetSecretsTotal();
		idThread::ReturnFloat(totalSecrets);
		return;
	}
	gameLocal.Warning("Invalid statistic name passed to getMissionStatistic(): %s", statisticName);
	idThread::ReturnFloat(0.0f);
}

// SteveL #3802
void idThread::Event_GetNextEntity( const char* key, const char* value, const idEntity* lastMatch )
{
	bool noKeyFilter	=	( *key   == '\0' );
	bool noValueFilter	=	( *value == '\0' );
	
	// Step 1: Work out where to start in the gameLocal.entities index
	int i = lastMatch ? lastMatch->entityNumber + 1 : 0;

	// Step 2: Advance i to the next matching entity
	for ( ; i < MAX_GENTITIES ; ++i )
	{
		idEntity* ent = gameLocal.entities[i];
		
		if ( !ent )
		{
			continue;	// skip past nulls in the index
		}
		else if ( noKeyFilter && noValueFilter )
		{
			break;		// any entity will do
		}
		
		// Search spawnargs for a matching value. If key is empty, all keys will be tested.
		bool foundMatch = false;
		const idKeyValue* kv = NULL;
		while ( (kv = ent->spawnArgs.MatchPrefix(key, kv)) != NULL )
		{
			if ( noValueFilter || kv->GetValue() == value )
			{
				foundMatch = true;
				break;
			}
		}

		if ( foundMatch )
		{
			break;
		}
	}

	// Step 3: Return a value
	idThread::ReturnEntity( i < MAX_GENTITIES ? gameLocal.entities[i] : NULL );
}

// Allow scripts to use the World particle system -- SteveL #3962
void idThread::Event_EmitParticle( const char* particle, float startTime, float diversity, const idVec3& origin, const idVec3& angle )
{
	const idMat3 axis = idAngles(angle).ToMat3();
	const idDeclParticle* ptcl = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, particle ) );
	const bool emitted = gameLocal.smokeParticles->EmitSmoke( ptcl, startTime*1000, diversity, origin, axis );
	idThread::ReturnFloat( emitted? 1.0f : 0.0f );
}

void idThread::Event_ProjectDecal( const idVec3& traceOrigin, const idVec3& traceEnd, idEntity* passEntity, const char* decal, float decalSize, float angle )
{
	trace_t tr;

	gameLocal.clip.TracePoint( tr, traceOrigin, traceEnd, MASK_OPAQUE, passEntity );

	// if trace made contact with a surface...
	if ( trace.fraction < 1.0f )
	{
		gameLocal.ProjectDecal( tr.c.point, -tr.c.normal, 8.0f, true, decalSize, decal,
								DEG2RAD(angle), gameLocal.entities[tr.c.entityNum], true, -1, false );
	}
}

//Script events for the secrets system
void idThread::Event_SetSecretsFound( float secrets )
{
	gameLocal.m_MissionData->SetSecretsFound( secrets );
}

void idThread::Event_SetSecretsTotal( float secrets )
{
	gameLocal.m_MissionData->SetSecretsTotal( secrets );
}

// stgatilov #6336: initializing several independent user addons
void idThread::Event_CallFunctionsByWildcard( const char* functionNameWildcard )
{
	idList<function_t*> functions = gameLocal.program.FindFunctions( functionNameWildcard );

	for ( int i = 0; i < functions.Num(); i++ ) {
		function_t *func = functions[i];
		// only functions with no parameters and no return can be called
		if ( func->type->NumParameters() != 0 || func->type->ReturnType() != &type_void )
			continue;

		// call in separate thread
		// note: in order to call within same thread, we have to insert some thunk code =(
		idThread *newThread = new idThread( func );
		newThread->CallFunction( func, true );
		newThread->DelayedStart( 0 );
	}
}
