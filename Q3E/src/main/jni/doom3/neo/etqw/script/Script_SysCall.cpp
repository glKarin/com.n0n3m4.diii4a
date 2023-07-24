// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Script_SysCall.h"
#include "Script_ScriptObject.h"
#include "Script_Helper.h"

#include "../Entity.h"
#include "../Player.h"
#include "../Camera.h"
#include "../roles/Tasks.h"
#include "../roles/ObjectiveManager.h"
#include "../roles/WayPointManager.h"
#include "../../bse/BSEInterface.h"
#include "../../bse/BSE_Envelope.h"
#include "../../bse/BSE_SpawnDomains.h"
#include "../../bse/BSE_Particle.h"
#include "../../bse/BSE.h"
#include "../client/ClientEntity.h"

#include "../rules/GameRules.h"
#include "../rules/VoteManager.h"

#include "../structures/DeployMask.h"

#include "../guis/UserInterfaceLocal.h"
#include "../guis/UIWindow.h"
#include "../guis/UserInterfaceManager.h"
#include "../../decllib/declTypeHolder.h"
#include "../../decllib/DeclSurfaceType.h"

#include "../proficiency/StatsTracker.h"

trace_t							sdSysCallThread::trace;
float							sdSysCallThread::cachedRoot[ 2 ];
idWStrList						sdSysCallThread::localizationStrings;

extern const idEventDef EV_GetTraceBody;
extern const idEventDef EV_GetTraceFraction;
extern const idEventDef EV_GetTraceEndPos;
extern const idEventDef EV_GetTracePoint;
extern const idEventDef EV_GetTraceNormal;
extern const idEventDef EV_GetTraceEntity;
extern const idEventDef EV_GetTraceSurfaceFlags;
extern const idEventDef EV_GetTraceSurfaceType;
extern const idEventDef EV_GetTraceSurfaceColor;
extern const idEventDef EV_GetTraceJoint;

const idEventDefInternal EV_Thread_Execute( "internal_execute" );
const idEventDef EV_Thread_TerminateThread( "terminate", '\0', DOC_TEXT( "Terminates the thread with the given id." ), 1, NULL, "d", "id", "Id of the thread to terminate." );
const idEventDef EV_Thread_Wait( "wait", '\0', DOC_TEXT( "Pauses the thread for the length of time specified." ), 1, NULL, "f", "time", "Length of time to wait for." );
const idEventDef EV_Thread_WaitFrame( "waitFrame", '\0', DOC_TEXT( "Pauses the thread for a single game frame." ), 0, NULL );
const idEventDef EV_Thread_Print( "print", '\0', DOC_TEXT( "Prints information to the console." ), 1, NULL, "s", "text", "Text to print." );
const idEventDef EV_Thread_PrintLn( "println", '\0', DOC_TEXT( "Prints information to the console, and adds a line break." ), 1, NULL, "s", "text", "Text to print." );
const idEventDef EV_Thread_StackTrace( "stackTrace", '\0', DOC_TEXT( "Prints a stack trace to the console, for debugging purposes." ), 0, NULL );
const idEventDef EV_Thread_Assert( "assert", '\0', DOC_TEXT( "Throws an assert in debug mode, if the parameter is false." ), 1, NULL, "b", "value", "Value to check." );
const idEventDef EV_Thread_Random( "random", 'f', DOC_TEXT( "Returns a random number between 0 and the limit specified." ), 1, NULL, "f", "range", "Maximum number to return." );
const idEventDef EV_Thread_GetTime( "getTime", 'f', DOC_TEXT( "Returns the current game time in seconds." ), 0, "This value has no bearing on how long the current match has been playing for, this is just the time since this map was loaded." );
const idEventDef EV_Thread_ToGuiTime( "toGuiTime", 'f', DOC_TEXT( "Returns the time converted to GUI time." ), 1, NULL, "f", "time", "Time to convert." );
const idEventDef EV_Thread_KillThread( "killThread", '\0', DOC_TEXT( "Terminates all script threads with the specified name." ), 1, "No indication is given as to how many threads, if any, were terminated.", "s", "name", "Name of the thread(s) to terminate." );
const idEventDef EV_Thread_SetThreadName( "threadName", '\0', DOC_TEXT( "Changes the name of the currently active thread." ), 1, NULL, "s", "name", "New name to set." );
const idEventDef EV_Thread_GetEntity( "getEntity", 'e', DOC_TEXT( "Finds an entity with the specified name, if no entity is found, the result will be $null$." ), 1, NULL, "s", "name", "Name of the entity to find." );
const idEventDef EV_Thread_GetEntityByID( "getEntityByID", 'e', DOC_TEXT( "Returns an entity from a key previously given by a call to $event:getSpawnID$." ), 1, "If the entity no longer exists, or the key is not valid, the result will be $null$.", "s", "key", "Key to look up the entity with." );
const idEventDef EV_Thread_Spawn( "spawn", 'e', DOC_TEXT( "Spawns an entity from the specified $decl:entityDef$." ), 1, "An error will be thrown if used during map spawning.", "s", "classname", "Name of the $decl:entityDef$ to use." );
const idEventDef EV_Thread_SpawnClient( "spawnClient", 'o', DOC_TEXT( "Spawns a client entity from the specified $decl:entityDef$" ), 1, "The client entity that is spawned must support scripting, or an error will be thrown.", "s", "classname", "Name of the $decl:entityDef$ to use." );
const idEventDef EV_Thread_SpawnByType( "spawnType", 'e', DOC_TEXT( "Spawns an entity of the specified $class$." ), 1, "Type ids can be looked up using $event:getTypeHandle$.", "d", "type", "Id of the type of entity to spawn." );
const idEventDef EV_Thread_AngToForward( "angToForward", 'v', DOC_TEXT( "Generates a forward looking vector for the given angles." ), 1, "See also $event:angToRight$ and $event:angToUp$.", "v", "angles", "The angles to covert from." );
const idEventDef EV_Thread_AngToRight( "angToRight", 'v', DOC_TEXT( "Generates a right looking vector for the given angles." ), 1, "See also $event:angToForward$ and $event:angToUp$.", "v", "angles", "The angles to covert from." );
const idEventDef EV_Thread_AngToUp( "angToUp", 'v', DOC_TEXT( "Generates an up looking vector for the given angles." ), 1, "See also $event:angToForward$ and $event:angToRight$.", "v", "angles", "The angles to covert from." );
const idEventDef EV_Thread_Sine( "sin", 'f', DOC_TEXT( "Returns the sine of the given value." ), 1, NULL, "f", "value", "Value to take the sine of." );
const idEventDef EV_Thread_Cosine( "cos", 'f', DOC_TEXT( "Returns the cosine of the given value." ), 1, NULL, "f", "value", "Value to take the cosine of." );
const idEventDef EV_Thread_ArcTan( "atan", 'f', DOC_TEXT( "Returns the inverse tangent of the given value." ), 1, "For more accurate results, you may wish to use $event:atan2$.", "f", "value", "Value to take the inverse tangent of." );
const idEventDef EV_Thread_ArcTan2( "atan2", 'f', DOC_TEXT( "Returns the inverse tangent of the given values." ), 2, NULL, "f", "y", "Y Component.", "f", "x", "X Component." );
const idEventDef EV_Thread_ArcCosine( "acos", 'f', DOC_TEXT( "Returns the inverse cosine of the given value." ), 1, NULL, "f", "value", "Value to take the inverse cosine of." );
const idEventDef EV_Thread_ArcSine( "asin", 'f', DOC_TEXT( "Returns the inverse sine of the given value." ), 1, NULL, "f", "value", "Value to take the inverse sine of." );
const idEventDef EV_Thread_GetRoot( "getRoot", 'f', DOC_TEXT( "Returns a root previous calculated using $event:solveRoots$." ), 1, "An index of 0 will return the first root, any other value will return the second root.", "d", "index", "Index of the root to return, either 0 or 1." );
const idEventDef EV_Thread_SolveRoots( "solveRoots", 'd', DOC_TEXT( "Calculates the roots of the quadratic aX^2 + bX + c, and returns the number of roots found. The values of these roots can then be looked up using $event:getRoot$." ), 3, NULL, "f", "a", "The value of a.", "f", "b", "The value of b.", "f", "c", "The value of c." );
const idEventDef EV_Thread_AngleNormalize180( "angleNormalize180", 'f', DOC_TEXT( "Normalizes an angle into the range -180 to 180." ), 1, NULL, "f", "angle", "The angle to normalize." );
const idEventDef EV_Thread_Fabs( "fabs", 'f', DOC_TEXT( "Returns the absolute value of the value given." ), 1, NULL, "f", "value", "Number to take the absolute value of." );
const idEventDef EV_Thread_Floor( "floor", 'f', DOC_TEXT( "Rounds the value given down to the closest integer less than or equal to it." ), 1, NULL, "f", "value", "Number to round." );
const idEventDef EV_Thread_Ceil( "ceil", 'f', DOC_TEXT( "Rounds the value given up to the closest interger greater than or equal to it." ), 1, NULL, "f", "value", "Number to round." );
const idEventDef EV_Thread_Mod( "mod", 'd', DOC_TEXT( "Returns the remainder of the division of the given numbers, A/B." ), 2, "See also $event:fmod$.", "d", "A", "The numerator.", "d", "B", "The denominator." );
const idEventDef EV_Thread_Fmod( "fmod", 'f', DOC_TEXT( "Returns the remainder of the division of the given numbers, A/B." ), 2, "See also $event:mod$.", "f", "A", "The numerator.", "f", "B", "The denominator." );
const idEventDef EV_Thread_SquareRoot( "sqrt", 'f', DOC_TEXT( "Returns the square root of the given number." ), 1, NULL, "f", "value", "Number to take the square root of." );
const idEventDef EV_Thread_Normalize( "vecNormalize", 'v', DOC_TEXT( "Returns a normalized version of the input vector." ), 1, NULL, "v", "value", "The vector to normalize." );
const idEventDef EV_Thread_VecLength( "vecLength", 'f', DOC_TEXT( "Returns the length of the input vector." ), 1, NULL, "v", "value", "The vector to get the length of." );
const idEventDef EV_Thread_VecLengthSquared( "vecLengthSquared", 'f', DOC_TEXT( "Returns the squared length of the input vector." ), 1, NULL, "v", "value", "The vector to get the length of." );
const idEventDef EV_Thread_VecCrossProduct( "crossProduct", 'v', DOC_TEXT( "Returns the cross product of the given vectors." ), 2, NULL, "v", "a", "First vector.", "v", "b", "Second vector." );
const idEventDef EV_Thread_VecToAngles( "vecToAngles", 'v', DOC_TEXT( "Returns a set of angles which represent the given direction." ), 1, NULL, "v", "value", "Direction vector." );
const idEventDef EV_Thread_RotateAngles( "rotateAngles", 'v', DOC_TEXT( "Rotates a set of angles, by another set of angles, and returns the result." ), 2, NULL, "v", "input", "Initial angles.", "v", "rotation", "Angles to rotate by." );
const idEventDef EV_Thread_RotateVecByAngles( "rotateVecByAngles", 'v', DOC_TEXT( "Rotates a vector by a set of angles, and returns the result." ), 2, NULL, "v", "input", "Input vector.", "v", "rotation", "Angles to rotate by." );
const idEventDef EV_Thread_RotateVec( "rotateVec", 'v', DOC_TEXT( "Rotates a vector around another vector by a set angles." ), 3, NULL, "v", "input", "Input vector.", "v", "axis", "Vector to rotate around.", "f", "angle", "Amount to rotate by." );
const idEventDef EV_Thread_ToLocalSpace( "toLocalSpace", 'v', DOC_TEXT( "Converts a vector from world space into the space of the entity's physics." ), 2, "FIXME: This should be handled by the entity, not the thread.", "v", "input", "The vector to translate.", "e", "entity", "The entity to perform the translation." );
const idEventDef EV_Thread_ToWorldSpace( "toWorldSpace", 'v', DOC_TEXT( "Converts a vector from the space of the entity's physics into world space." ), 2, "FIXME: This should be handled by the entity, not the thread.", "v", "input", "The vector to translate.", "e", "entity", "The entity to perform the translation." );
const idEventDef EV_Thread_CheckContents( "checkContents", 'f', DOC_TEXT( "Checks the given bounds for any objects which match the collision mask specified." ), 5, NULL, "v", "start", "Start position for the contents check", "v", "mins", "Mins of the bounds to check with.", "v", "maxs", "Maxs of the bounds to check with.", "d", "mask", "Collision mask to use.", "o", "ignore", "Object to ignore in the check, may be an entity or a client entity." );
const idEventDef EV_Thread_Trace( "trace", 'f', DOC_TEXT( "Traces the given bounds through the world, and returns the fraction of the trace which passed." ), 6, "If both the mins and maxs are empty, an optimized point trace will be used.\nYou can use $event:saveTrace$ to save the results of this trace.", "v", "start", "The start position for the trace.", "v", "end", "The end position for the trace.", "v", "mins", "The mins of the bounds.", "v", "maxs", "The maxs of the bounds", "d", "mask", "Collision mask to use.", "o", "ignore", "Object to ignore in the check, may be an entity or a client entity." );
const idEventDef EV_Thread_TracePoint( "tracePoint", 'f', DOC_TEXT( "Performs a fast point trace through the world, and returns the fraction of the trace which passed." ), 4, "You can use $event:saveTrace$ to save the results of this trace.", "v", "start", "The start position for this trace.", "v", "end", "The end position for this trace.", "d", "mask", "Collision mask to use.", "o", "ignore", "Object to ignore in the check, may be an entity or a client entity." );
const idEventDef EV_Thread_TraceOriented( "traceOriented", 'f', DOC_TEXT( "Traces the given bounds, rotated by the specified angle, through the world, and returns the fraction of the trace which passed." ), 7, "If both the mins and maxs are empty, an optimized point trace will be used.\nYou can use $event:saveTrace$ to save the results of this trace.", "v", "start", "The start position for the trace.", "v", "end", "The end position for the trace.", "v", "mins", "The mins of the bounds.", "v", "maxs", "The maxs of the bounds", "v", "angles", "Angles to rotate the bounds by.", "d", "mask", "Collision mask to use.", "o", "ignore", "Object to ignore in the check, may be an entity or a client entity." );
const idEventDef EV_Thread_SaveTrace( "saveTrace", 'o', DOC_TEXT( "Saves the results of the last thread trace that was performed, and returns an object representing this trace." ), 0, "Only a limited number of saved traces may be saved at any point in time, care should be taken to properly free these traces once you are finished with them. See $event:freeTrace$." );
const idEventDef EV_Thread_FreeTrace( "freeTrace", '\0', DOC_TEXT( "Frees a previously saved trace object." ), 1, "See also $event:saveTrace$.", "o", "trace", "The trace to free." );
const idEventDef EV_GetTraceFraction( "getTraceFraction", 'f', DOC_TEXT( "Returns the fraction of the trace which completed." ), 0, NULL );
const idEventDef EV_GetTraceEndPos( "getTraceEndPos", 'v', DOC_TEXT( "Returns the end position of the object of the trace." ), 0, NULL );
const idEventDef EV_GetTracePoint( "getTracePoint", 'v', DOC_TEXT( "Returns the point at which a collision occured." ), 0, "If the trace fraction is 1, then this result has no meaning." );
const idEventDef EV_GetTraceNormal( "getTraceNormal", 'v', DOC_TEXT( "Returns the normal of the surface where the collision occured." ), 0, "If the trace fraction is 1, then this result has no meaning." );
const idEventDef EV_GetTraceEntity( "getTraceEntity", 'e', DOC_TEXT( "Returns the entity that the trace hit." ), 0, "If the trace fraction is 1, then this result has no meaning." );
const idEventDef EV_GetTraceSurfaceFlags( "getTraceSurfaceFlags", 'd', DOC_TEXT( "Returns the surface flags of the surface where the collision occured." ), 0, "If the trace fraction is 1, then this result has no meaning." );
const idEventDef EV_GetTraceSurfaceType( "getTraceSurfaceType", 's', DOC_TEXT( "Returns the surface type of the surface where the collision occured." ), 0, "If the trace fraction is 1, then this result has no meaning." );
const idEventDef EV_GetTraceSurfaceColor( "getTraceSurfaceColor", 'v', DOC_TEXT( "Returns the color of the surface where the collision occured." ), 0, "If the trace fraction is 1, then this result has no meaning." );
const idEventDef EV_GetTraceJoint( "getTraceJoint", 's', DOC_TEXT( "Returns the name of the joint with most influence on the surface where the collision occured." ), 0, "If the trace fraction is 1, then this result has no meaning.\nIf the model does not support animation, an empty string will be returned." );
const idEventDef EV_GetTraceBody( "getTraceBody", 's', DOC_TEXT( "Returns the name of the body where the collision occured." ), 0, "If the trace fraction is 1, then this result has no meaning.\nIf the entity is not articulated figure based, an empty string will be returned." );
const idEventDef EV_Thread_StartMusic( "music", '\0', DOC_TEXT( "Plays the specified sound on the non localized music channel." ), 1, "This is just a shortcut to using $event:startSoundDirect$.\nPassing a blank string will stop any existing sound on the music channel.", "s", "sound", "Name of the sound shader to play." );
const idEventDef EV_Thread_StartSoundDirect( "startSoundDirect", '\0', DOC_TEXT( "Plays the specified sound on a non localized channel." ), 2, "Passing an empty string will stop any existing sound on the channel.", "s", "sound", "Name of the sound shader to play.", "d", "channel", "Index of the channel to play on." );
const idEventDef EV_Thread_Error( "error", '\0', DOC_TEXT( "Throws an error with the specified text in the error message" ), 1, NULL, "s", "text", "Text to include in the error message." );
const idEventDef EV_Thread_Warning( "warning", '\0', DOC_TEXT( "Prints a warning on the console, and adds it to the warnings log." ), 1, NULL, "s", "text", "Text to include in the warning message." );
const idEventDef EV_Thread_StrLen( "strLength", 'd', DOC_TEXT( "Returns the number of characters in a string." ), 1, NULL, "s", "text", "The string to count." );
const idEventDef EV_Thread_StrLeft( "strLeft", 's', DOC_TEXT( "Returns a string containing the leftmost characters from the input string, up to the limit specified." ), 2, "If there are not enough characters in the string, the whole string will be returned instead.", "s", "text", "The string to work on.", "d", "limit", "The maximum number of characters to return." );
const idEventDef EV_Thread_StrRight( "strRight", 's', DOC_TEXT( "Returns a string containing the rightmost characters from the input string, up to the limit specified." ), 2, "If there are not enough characters in the string, the whole string will be returned instead.", "s", "text", "The string to work on.", "d", "limit", "The maximum number of characters to return." );
const idEventDef EV_Thread_StrSkip( "strSkip", 's', DOC_TEXT( "Strips the leftmost characters from the input string, up to the limit specified, and returns the remaining string." ), 2, "If there are not enough characters in the string, an empty string will be returned.", "s", "text", "The string to work on.", "d", "limit", "The maximum number of characters to strip." );
const idEventDef EV_Thread_StrMid( "strMid", 's', DOC_TEXT( "Returns an arbitrary section from a given string." ), 3, "If start is past the end of the string, an empty string will be returned.", "s", "text", "The string to work on.", "d", "start", "The index of the first character to include.", "d", "count", "The number of characters to include" );
const idEventDef EV_Thread_StrToFloat( "strToFloat", 'f', DOC_TEXT( "Converts the given string into a float." ), 1, "If the text does not represent a valid float, 0 will be returned", "s", "value", "The string to convert." );
const idEventDef EV_Thread_IsClient( "isClient", 'b', DOC_TEXT( "Returns whether this is a network client or not." ), 0, NULL );
const idEventDef EV_Thread_IsServer( "isServer", 'b', DOC_TEXT( "Returns whether this is a network server or not." ), 0, NULL );
const idEventDef EV_Thread_DoClientSideStuff( "doClientSideStuff", 'b', DOC_TEXT( "Returns whether client side only effects should be run or not." ), 0, NULL );
const idEventDef EV_Thread_IsNewFrame( "isNewFrame", 'b', DOC_TEXT( "Returns whether this is a forward prediction frame or not." ), 0, NULL );
const idEventDef EV_Thread_GetFrameTime( "getFrameTime", 'f', DOC_TEXT( "Returns the length of a frame, in seconds." ), 0, NULL );
const idEventDef EV_Thread_GetTicsPerSecond( "getTicsPerSecond", 'f', DOC_TEXT( "Returns the number of game frames run per second." ), 0, "This always returns 30." );
const idEventDef EV_Thread_BroadcastToolTip( "broadcastToolTip", '\0', DOC_TEXT( "Sends a message to a specific client to show the specified tooltip, with optional parameters." ), 6, NULL, "d", "tooltip", "Index of tooltip to be shown.", "e", "client", "Client to send the message to.", "w", "parm1", "Optional text to insert into tooltip #1.", "w", "parm2", "Optional text to insert into tooltip #2.", "w", "parm3", "Optional text to insert into tooltip #3.", "w", "parm4", "Optional text to insert into tooltip #4." );
const idEventDef EV_Thread_DebugLine( "debugLine", '\0', DOC_TEXT( "Adds a debug line between the specified points in the world." ), 4, NULL, "v", "color", "RGB components of the color of the line.", "v", "start", "Start position of the line.", "v", "end", "End position of the line", "f", "time", "Lifetime of the line in seconds." );
const idEventDef EV_Thread_DebugArrow( "debugArrow", '\0', DOC_TEXT( "Adds a debug line, with an arrowhead at the end, between the specified points in the world." ), 5, NULL, "v", "color", "RGB components of the color of the line.", "v", "start", "Start position of the line.", "v", "end", "End position of the line", "d", "size", "Size of the arrowhead.", "f", "time", "Lifetime of the line in seconds." );
const idEventDef EV_Thread_DebugCircle( "debugCircle", '\0', DOC_TEXT( "Adds a debug circle at the specified point in the world." ), 6, NULL, "v", "color", "RGB components of the color of the circle.", "v", "origin", "Origin of the circle.", "v", "normal", "Normal of the plane on which the circle will lie.", "f", "radius", "Radius of the circle.", "d", "segments", "Number of lines to draw the circle using.", "f", "time", "Lifetime of the circle in seconds." );
const idEventDef EV_Thread_DebugBounds( "debugBounds", '\0', DOC_TEXT( "Adds a debug box between the specified positions in the world." ), 4, NULL, "v", "color", "RGB components of the color of the box.", "v", "mins", "Mins of the box.", "v", "maxs", "Maxs of the box.", "f", "time", "Lifetime of the box in seconds." );
const idEventDef EV_Thread_DrawText( "drawText", '\0', DOC_TEXT( "Adds debug text in the world." ), 6, NULL, "s", "text", "Text to display.", "v", "origin", "Origin of the text.", "f", "scale", "Scales the size of the text.", "v", "color", "RGB components of the color of the text.", "d", "align", "Alignment of the text, 0 = left, 1 = center, 2 = right.", "f", "time", "Lifetime of the text in seconds." );
const idEventDef EV_Thread_GetDeclTypeHandle( "getDeclType", 'd' , DOC_TEXT( "Returns the index of the $decl$ type specified." ), 1, "If the type cannot be found, the result will be -1.", "s", "name", "Name of the $decl$ type to look up." );
const idEventDef EV_Thread_GetDeclIndex( "getDeclIndex", 'd', DOC_TEXT( "Returns the index of $decl$ of the specified type and name." ), 2, "If either the type or name are invalid/cannot be found, the result will be -1.", "d", "type", "Index of the $decl$ type.", "s", "name", "Name of the $decl$ to look up." );
const idEventDef EV_Thread_GetDeclName( "getDeclName", 's', DOC_TEXT( "Returns the name of the $decl$ of the specified type and index." ), 2, "If either the type or index are invalid, the result will be an empty string.", "d", "type", "Index of the $decl$ type.", "d", "index", "Index of the $decl$." );
const idEventDef EV_Thread_GetDeclCount( "getDeclCount", 'd', DOC_TEXT( "Returns the $decl$ count for a specified $decl$ type." ), 1, "If the $decl$ type is invalid, the result will be 0.", "d", "type", "Index of the $decl$ type." );
const idEventDef EV_Thread_ApplyRadiusDamage( "applyRadiusDamage", '\0', DOC_TEXT( "Applies radius damage to entities around a given position." ), 8, NULL, "v", "origin", "Origin to check for entities from.", "E", "inflictor", "Entity applying the damage.", "E", "attacker", "Entity responsible for the damage.", "E", "ignore", "Entity to not apply damage to.", "E", "pushIgnore", "Entity to not apply push to.", "d", "damage", "Index of the $decl:damageDef$ to apply.", "f", "damageScale", "Factor to scale applied damage by.", "f", "pushScale", "Factor to scale applied push by." );
const idEventDef EV_Thread_FilterEntity( "filterEntity", 'b', DOC_TEXT( "Returns whether the entity is caught by a $decl:targetInfo$." ), 2, "If the $decl:targetInfo$ is invalid or the entity is $null$, the result will be true.", "d", "index", "Index of the $decl:targetInfo$.", "E", "entity", "The entity to check." );
const idEventDef EV_Thread_GetTableCount( "getTableCount", 'd', DOC_TEXT( "Returns the number of entries in $decl:table$." ), 1, "If the $decl:table$ is invalid, then the result will be 0.", "d", "index", "Index of the $decl:table$." );
const idEventDef EV_Thread_GetTableValue( "getTableValue", 'f', DOC_TEXT( "Returns an entry from a $decl:table$." ), 2, "This is the direct value from the table and does not do a lookup for interpolation, etc. To do that, use $event:getTableValueExact$.\nIf the index is -1, the result will be 0.\nIf the index is otherwise out of range, the game will likely crash or return garbage data.", "d", "index", "Index of the $decl:table$.", "d", "value", "Which value to look up." );
const idEventDef EV_Thread_GetTableValueExact( "getTableValueExact", 'f', DOC_TEXT( "Returns a valur from a $decl:table$, performing any interpolation required for inbetween values." ), 2, "If the index is -1, the result will be 0.\nIf the index is otherwise out of range, the game will likely crash or return garbage data.", "d", "index", "Index of the $decl:table$.", "f", "value", "Position in the table to perform a lookup." );
const idEventDef EV_Thread_GetTypeHandle( "getTypeHandle", 'd', DOC_TEXT( "Returns the index of a $class$." ), 1, "If the $class$ cannot be found, the result will be -1.", "s", "name", "Name of the $class$ to find." );
const idEventDef EV_Thread_Argc( "argc", 'd', DOC_TEXT( "Returns the number of arguments stored from the last action command that was set." ), 0, "You can set an action command from script using $event:setActionCommand$." );
const idEventDef EV_Thread_Argv( "argv", 's', DOC_TEXT( "Returns the string from the action command at the specified index." ), 1, "If the index is out of range, the result will be an empty string.\nYou can find out the current number of strings using $event:argc$.", "d", "index", "Index of the string to look up." );
const idEventDef EV_Thread_FloatArgv( "argvf", 'f', DOC_TEXT( "Returns the string from the action command at the specified index converted to a float." ), 1, "If the index is out of range, or the string cannot be converted to a float, then the result will be 0.\nYou can find out the current number of strings using $event:argc$.", "d", "index", "Index of the string to look up." );
const idEventDef EV_Thread_SetActionCommand( "setActionCommand", '\0', DOC_TEXT( "Stores the given string as a set of command arguments which can be looked up individually, using $event:argc$, $event:argv$ and $event:argvf$." ), 1, NULL, "s", "text", "String to parse into command arguments." );
const idEventDef EV_Thread_GetTeam( "getTeam", 'o', DOC_TEXT( "Returns the script object associated with a given team." ), 1, "If the team cannot be found, the result will be $null$.", "s", "name", "Name of the team to look up." );
const idEventDef EV_Thread_PlayWorldEffect( "playWorldEffect", '\0', DOC_TEXT( "Plays the specified effect at a fixed position in the world." ), 4, NULL, "s", "effect", "Name of the $decl:effect$ to play.", "v", "color", "RGB components of color to apply to the effect.", "v", "origin", "Position in the world to play the effect at.", "v", "forward", "Direction for the effect to point in." );
const idEventDef EV_Thread_PlayWorldEffectRotate( "playWorldEffectRotate", '\0', DOC_TEXT( "Plays the specified effect at a fixed position in the world, with the forward vector rotated by the given angles." ), 5, NULL, "s", "name", "Name of the $decl:effect$ to play.", "v", "color", "RGB components of the color to apply to the effect.", "v", "origin", "Position in the world to play the effect at.", "v", "forward", "Direction for the effect to point in.", "v", "angles", "Angles to rotate the forward vector by." );
const idEventDef EV_Thread_PlayWorldEffectRotateAlign( "playWorldEffectRotateAlign", '\0', DOC_TEXT( "Plays the specified effect at a fixed position in the world, with the forward vector rotated by the given angles, and the other axes aligned towards the specified entity's axes." ), 6, NULL, "s", "name", "Name of the $decl:effect$ to play.", "v", "color", "RGB components of the color to apply to the effect.", "v", "origin", "Position in the world to play the effect at.", "v", "forward", "Direction for the effect to point in.", "v", "angles", "Angles to rotate the forward vector by.", "e", "entity", "Entity to align the other axes with." );
const idEventDef EV_Thread_PlayWorldBeamEffect( "playWorldBeamEffect", '\0', DOC_TEXT( "Creates a beam effect at the specified positions in world space." ), 4, NULL, "s", "effect", "Name of the $decl:effect$ to play.", "v", "color", "RGB components of color to apply to the effect.", "v", "start", "Start position of the beam in world space.", "v", "end", "End position of the beam in world space." );
const idEventDef EV_Thread_GetLocalPlayer( "getLocalPlayer", 'e', DOC_TEXT( "Returns the entity for the local client, if there is one, otherwise returns $null$." ), 0, NULL );
const idEventDef EV_Thread_GetLocalViewPlayer( "getLocalViewPlayer", 'e', DOC_TEXT( "Returns the player entity currently being viewed from by the local client, if appropriate, otherwise returns $null$." ), 0, NULL );

const idEventDef EV_Thread_SetGUIFloat( "setGUIFloat", '\0', DOC_TEXT( "Sets a named float property on the specified gui." ), 3, "The constant GUI_GLOBALS_HANDLE may be used to set a property on the globals hierarchy instead.", "d", "handle", "Handle to the gui.", "s", "name", "Name of the property to set.", "f", "value", "The value to set." );
const idEventDef EV_Thread_SetGUIHandle( "setGUIHandle", '\0', DOC_TEXT( "Sets a named handle property on the specified gui." ), 3, "The constant GUI_GLOBALS_HANDLE may be used to set a property on the globals hierarchy instead.", "d", "handle", "Handle to the gui.", "s", "name", "Name of the property to set.", "h", "value", "The value to set." );
const idEventDef EV_Thread_SetGUIVec2( "setGUIVec2", '\0', DOC_TEXT( "Sets a named vec2 property on the specified gui." ), 4, "The constant GUI_GLOBALS_HANDLE may be used to set a property on the globals hierarchy instead.", "d", "handle", "Handle to the gui.", "s", "name", "Name of the property to set.", "f", "x", "X component of the vector.", "f", "y", "Y component of the vector" );
const idEventDef EV_Thread_SetGUIVec3( "setGUIVec3", '\0', DOC_TEXT( "Sets a named vector property on the specified gui." ), 5, "The constant GUI_GLOBALS_HANDLE may be used to set a property on the globals hierarchy instead.", "d", "handle", "Handle to the gui.", "s", "name", "Name of the property to set.", "f", "x", "X component of the vector.", "f", "y", "Y component of the vector", "f", "z", "Z component of the vector" );
const idEventDef EV_Thread_SetGUIVec4( "setGUIVec4", '\0', DOC_TEXT( "Sets a named color property on the specified gui." ), 6, "The constant GUI_GLOBALS_HANDLE may be used to set a property on the globals hierarchy instead.", "d", "handle", "Handle to the gui.", "s", "name", "Name of the property to set.", "f", "r", "Red component of the color.", "f", "g", "Green component of the color.", "f", "b", "Blue component of the color.", "f", "a", "Alpha component of the color." );
const idEventDef EV_Thread_SetGUIString( "setGUIString", '\0', DOC_TEXT( "Sets a named string property on the specified gui." ), 3, "The constant GUI_GLOBALS_HANDLE may be used to set a property on the globals hierarchy instead.", "d", "handle", "Handle to the gui.", "s", "name", "Name of the property to set.", "s", "value", "The value to set." );
const idEventDef EV_Thread_SetGUIWString( "setGUIWString", '\0', DOC_TEXT( "Sets a named wide string property on the specified gui." ), 3, "The constant GUI_GLOBALS_HANDLE may be used to set a property on the globals hierarchy instead.", "d", "handle", "Handle to the gui.", "s", "name", "Name of the property to set.", "w", "value", "The value to set." );
const idEventDef EV_Thread_GUIPostNamedEvent( "guiPostNamedEvent", '\0', DOC_TEXT( "Sends a named event message to a window on a gui." ), 3, NULL, "d", "handle", "Handle to the gui.", "s", "window", "Name of the window to run the event on.", "s", "event", "Name of the event to run." );
const idEventDef EV_Thread_GetGUIFloat( "getGUIFloat", 'f', DOC_TEXT( "Returns the value of a named float property on the specified gui." ), 2, "The constant GUI_GLOBALS_HANDLE may be used to set a property on the globals hierarchy instead.", "d", "handle", "Handle to the gui.", "s", "name", "Name of the property to look up." );
const idEventDef EV_Thread_SetGUITheme( "setGUITheme", '\0', DOC_TEXT( "Applies a theme to the specified gui." ), 2, NULL, "d", "handle", "Handle to the gui.", "s", "name", "Name of the theme to apply." );
const idEventDef EV_Thread_AddNotifyIcon( "addNotifyIcon", 'd', DOC_TEXT( "Add an icon to an iconNotification control, and returns a handle to the icon." ), 3, "The result will be 0 if the action fails.", "d", "handle", "Handle to the gui which contains the control.", "s", "name", "Name of the control.", "s", "material", "Name of the icon material." );
const idEventDef EV_Thread_RemoveNotifyIcon( "removeNotifyIcon", '\0', DOC_TEXT( "Removes an icon from an iconNotification control." ), 3, NULL, "d", "handle", "Handle to the gui which contains the control.", "s", "name", "Name of the control.", "d", "icon", "Handle of the icon to remove." );
const idEventDef EV_Thread_BumpNotifyIcon( "bumpNotifyIcon", '\0', DOC_TEXT( "Makes an icon in an iconNotification control jump, highlighting its presence." ), 3, NULL, "d", "handle", "Handle to the gui which contains the control.", "s", "name", "Name of the control.", "d", "icon", "Handle of the icon to bump." );

const idEventDef EV_Thread_ClearDeployRequest( "clearDeployRequest", '\0', DOC_TEXT( "Cancels a deploy request for a given client index." ), 1, NULL, "d", "index", "Index of the client whose deploy request should be cancelled." );
const idEventDef EV_Thread_GetDeployMask( "getDeployMask", 'd', DOC_TEXT( "Returns a handle to a deploy mask of the given name." ), 1, "This function will always return a valid handle, even if no playzones have a deploy mask with this name.", "s", "name", "Name of the deploy mask to look up." );
const idEventDef EV_Thread_CheckDeployMask( "checkDeployMask", 'f', DOC_TEXT( "Returns whether the mask covering the bounds specified is entirely valid." ), 3, "Deploy mask handles can be looked up using $event:getDeployMask$.\nIf there is no playzone at the midpoint of the bounds which supports deployment the result will be false.\nIf the playzone found does not support the mask asked for, the result will be false.", "v", "mins", "Mins of the bounds to check.", "v", "maxs", "Maxs of the bounds to check.", "d", "handle", "Handle to the mask to check." );

const idEventDef EV_Thread_GetWorldPlayZoneIndex( "getWorldPlayZoneIndex", 'd', DOC_TEXT( "Returns the index of the world playzone which contains the point specified" ), 1, "If no playzone is found, the result will be -1.", "v", "position", "Position to find the playzone at." );

const idEventDef EV_Thread_AllocTargetTimer( "allocTargetTimer", 'd', DOC_TEXT( "Returns the handle to a named float field for all clients which is network synced only for the client currently being watched." ), 1, "The function name refers to these as timers merely because that is their most common use, to keep track of charge bars.\nIf a timer with the specified name does not already exist, a new one will be created.\nThe default value of a newly created timer is -1.", "s", "name", "Name of the timer to look up." );
const idEventDef EV_Thread_GetTargetTimerValue( "getTargetTimerValue", 'f', DOC_TEXT( "Returns the current value of a timer for the specified client." ), 2, "Timers can be allocated using $event:allocTargetTimer$.\nIf the entity passed in is not a player, the result will be -1.\nIf the handle passed in is less than 0, the result will be 0.\nIf the handle passed in is out of range, the game will either crash or return garbage data.", "d", "handle", "Handle of the timer to look up.", "e", "player", "Player to look up the timer on." );
const idEventDef EV_Thread_SetTargetTimerValue( "setTargetTimerValue", '\0', DOC_TEXT( "Sets the new value of a timer for the specified client." ), 3, "Timers can be allocated using $event:allocTargetTimer$.\nIf the entity passed in is not a player, the call will be ignored.\nIf the handle is out of range, an error will be thrown.", "d", "handle", "Handle to the timer to set.", "e", "player", "Player to set the timer on.", "f", "value", "Value to set." );
const idEventDef EV_Thread_AllocCMIcon( "allocCMIcon", 'd', DOC_TEXT( "Allocates a commandmap icon with an initial sort value, which belongs to the specified entity, and returns a handle to it." ), 2, "If a new icon cannot be created, the return value will be -1.", "e", "owner", "Entity which owns the icon.", "d", "sort", "Initial sort value." );
const idEventDef EV_Thread_FreeCMIcon( "freeCMIcon", '\0', DOC_TEXT( "Deletes an existing commandmap icon." ), 2, "The owner passed in must match the original owner, or the icon will not be freed. This is to ensure that uninitialised handles, or old handles don't destroy the icons of other entities.", "e", "owner", "Entity which owns the icon.", "d", "handle", "Handle to the icon to delete." );
const idEventDef EV_Thread_SetCMIconSize( "setCMIconSize", '\0', DOC_TEXT( "Sets the size of the icon when it is 'visible'." ), 2, "See also $event:setCMIconSize2d$.", "d", "handle", "Handle to the icon.", "f", "size", "Size to set." );
const idEventDef EV_Thread_SetCMIconUnknownSize( "setCMIconUnknownSize", '\0', DOC_TEXT( "Sets the size of the icon when it is not 'visible'." ), 2, "See also $event:setCMIconUnknownSize2d$.", "d", "handle", "Handle to the icon.", "f", "size", "Size to set." );
const idEventDef EV_Thread_SetCMIconSize2d( "setCMIconSize2d", '\0', DOC_TEXT( "Sets the size of the icon when it is 'visible'." ), 3, "See also $event:setCMIconSize$.", "d", "handle", "Handle to the icon.", "f", "x", "Width to set.", "f", "y", "Height to set." );
const idEventDef EV_Thread_SetCMIconUnknownSize2d( "setCMIconUnknownSize2d", '\0', DOC_TEXT( "Sets the size of the icon when it is not 'visible'." ), 3, "See also $event:setCMIconUnknownSize$.", "d", "handle", "Handle to the icon.", "f", "x", "Width to set.", "f", "y", "Height to set." );
const idEventDef EV_Thread_SetCMIconSizeMode( "setCMIconSizeMode", '\0', DOC_TEXT( "Sets the scaling mode for the icon, valid values are SM_FIXED, or SM_WORLD." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "mode", "Mode to set." );
const idEventDef EV_Thread_SetCMIconPositionMode( "setCMIconPositionMode", '\0', DOC_TEXT( "Specifies how the icon should be positioned, valid values are PM_ENTITY, or PM_FIXED." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "mode", "Mode to set." );
const idEventDef EV_Thread_SetCMIconOrigin( "setCMIconOrigin", '\0', DOC_TEXT( "Sets the origin of the icon to be used in fixed position mode." ), 2, "See also $event:setCMIconPositionMode$.", "d", "handle", "Handle to the icon.", "v", "origin", "Origin to set." );
const idEventDef EV_Thread_SetCMIconColor( "setCMIconColor", '\0', DOC_TEXT( "Sets the color of the icon to be used in normal color mode." ), 3, "See also $event:setCMIconColorMode$.", "d", "handle", "Handle to the icon.", "v", "color", "RGB components to set.", "f", "alpha", "Alpha to set." );
const idEventDef EV_Thread_SetCMIconColorMode( "setCMIconColorMode", '\0', DOC_TEXT( "Sets the color mode for the icon, valid values are CM_NORMAL, CM_FRIENDLY, or CM_ALLEGIANCE." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "mode", "Mode to set." );
const idEventDef EV_Thread_SetCMIconDrawMode( "setCMIconDrawMode", '\0', DOC_TEXT( "Sets the drawing mode for the icon, valid values are DM_MATERIAL, DM_CIRCLE, DM_ARC, DM_ROTATED_MATERIAL, DM_TEXT or DM_CROSSHAIR." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "mode", "Mode to set." );
const idEventDef EV_Thread_SetCMIconAngle( "setCMIconAngle", '\0', DOC_TEXT( "Sets the angle of the icon to be used in arc, or rotated material mode." ), 2, "See also $event:setCMIconDrawMode$.", "d", "handle", "Handle to the icon.", "f", "angle", "Angle to set." );
const idEventDef EV_Thread_SetCMIconSides( "setCMIconSides", '\0', DOC_TEXT( "Sets the number of sides of the icon to be used in arc, or circle mode." ), 2, "See also $event:setCMIconDrawMode$.", "d", "handle", "Handle to the icon.", "d", "sides", "Number of sides to set." );
const idEventDef EV_Thread_ShowCMIcon( "showCMIcon", '\0', DOC_TEXT( "Makes the specified icon visible." ), 1, NULL, "d", "handle", "Handle to the icon." );
const idEventDef EV_Thread_HideCMIcon( "hideCMIcon", '\0', DOC_TEXT( "Hides the specified icon." ), 1, NULL, "d", "handle", "Handle to the icon." );
const idEventDef EV_Thread_AddCMIconRequirement( "addCMIconRequirement", '\0', DOC_TEXT( "Adds a requirement to an icon. If the viewing player does not pass this requirement, they will not see the icon." ), 2, NULL, "d", "handle", "Handle to the icon.", "s", "requirement", "The requirement to add." );
const idEventDef EV_Thread_SetCMIconMaterial( "setCMIconMaterial", '\0', DOC_TEXT( "Sets the material of the icon to display when it is 'visible'." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "material", "The $decl:material$ to set." );
const idEventDef EV_Thread_SetCMIconUnknownMaterial( "setCMIconUnknownMaterial", '\0', DOC_TEXT( "Sets the material of the icon to display when it is not 'visible'." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "material", "The $decl:material$ to set." );
const idEventDef EV_Thread_SetCMIconFireteamMaterial( "setCMIconFireteamMaterial", '\0', DOC_TEXT( "Sets the material of the icon to display when it is 'visible' and the owner is in the same fireteam as the viewer." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "material", "The $decl:material$ to set." );
const idEventDef EV_Thread_SetCMIconGuiMessage( "setCMIconGuiMessage", '\0', DOC_TEXT( "Sets the message to pass to the owner when the player clicks on the icon." ), 2, NULL, "d", "handle", "Handle to the icon.", "s", "message", "Message to set." );
const idEventDef EV_Thread_SetCMIconFlag( "setCMIconFlag", '\0', DOC_TEXT( "Adds the specified flag(s) to the current set of active flags on the icon." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "flags", "Flags to add." );
const idEventDef EV_Thread_ClearCMIconFlag( "clearCMIconFlag", '\0', DOC_TEXT( "Removes the specified flag(s) from the current set of active flags on the icon." ), 2, NULL, "d", "handle", "Handle to the icon.", "d", "flags", "Flags to remove." );
const idEventDef EV_Thread_SetCMIconArcAngle( "setCMIconArcAngle", '\0', DOC_TEXT( "Sets the size of the arc to draw in arc mode." ), 2, "See also $event:setCMIconDrawMode$.", "d", "handle", "Handle to the icon.", "f", "size", "Size of the arc angle." );
const idEventDef EV_Thread_SetCMIconShaderParm( "setCMIconShaderParm", '\0', DOC_TEXT( "Sets the given shader parm index on the icon to the specified value." ), 3, "Index must be in the range 0-7 otherwise the game will likely crash.\nThese shader parms start 4 offset from the regular parms, i.e. after the colors.", "d", "handle", "Handle to the icon.", "d", "index", "Index of the shader parm to set.", "f", "value", "Value to set the shader parm to." );
const idEventDef EV_Thread_GetCMIconFlags( "getCMIconFlags", 'd', DOC_TEXT( "Returns the current active set of flags for an icon." ), 1, NULL, "d", "handle", "Handle to the icon." );
const idEventDef EV_Thread_FlashCMIcon( "flashCMIcon", '\0', DOC_TEXT( "Flashes the icon for a period of time with a specified $decl:material$, and sets the given flags temporarily." ), 4, "If the $decl:material$ passed is invalid, regular icon $decl:material$ rules will apply.", "d", "handle", "Handle to the icon.", "d", "material", "Index of the $decl:material$ to set.", "f", "time", "Length of the flash in seconds.", "d", "flags", "Temporary flags to set whilst flashing." );

const idEventDef EV_Thread_GetKey( "getEntityDefKey", 's', DOC_TEXT( "Looks up the specified key from an $decl:entityDef$." ), 2, "If the $decl:entityDef$ is invalid, the result will be an empty string.", "d", "index", "Index of the $decl:entityDef$.", "s", "key", "Key to look up." );
const idEventDef EV_Thread_GetIntKey( "getEntityDefIntKey", 'd', DOC_TEXT( "Looks up the specified key from an $decl:entityDef$ and converts it to an integer." ), 2, "If the $decl:entityDef$ is invalid, or the value cannot be converted, the result will be 0.", "d", "index", "Index of the $decl:entityDef$.", "s", "key", "Key to look up." );
const idEventDef EV_Thread_GetFloatKey( "getEntityDefFloatKey", 'f', DOC_TEXT( "Looks up the specified key from an $decl:entityDef$ and converts it to a float." ), 2, "If the $decl:entityDef$ is invalid, or the value cannot be converted, the result will be 0.", "d", "index", "Index of the $decl:entityDef$.", "s", "key", "Key to look up." );
const idEventDef EV_Thread_GetVectorKey( "getEntityDefVectorKey", 'v', DOC_TEXT( "Looks up the specified key from an $decl:entityDef$ and converts it to a vector." ), 2, "If the $decl:entityDef$ is invalid, or the value cannot be converted, the result will be '0 0 0'.", "d", "index", "Index of the $decl:entityDef$.", "s", "key", "Key to look up." );

const idEventDef EV_Thread_AllocDecal( "allocDecal", 'd', DOC_TEXT( "Allocates a decal using the supplied $decl:material$, and returns a handle to it." ), 1, "If a decal could not be allocated, the result will be -1.\nA limited number of decals can be allocated using this, so care should be taken to clean them up properly, using $event:freeDecal$.", "s", "material", "Name of the $decl:material$ to use with this decal." );
const idEventDef EV_Thread_ProjectDecal( "projectDecal", '\0', DOC_TEXT( "Performs a projection and adds the geometry found to this decal." ), 8, "The z component of size is unused.", "d", "handle", "Handle to the decal", "v", "origin", "Start position of the projection.", "v", "direction", "Direction in which to project.", "f", "depth", "Distance to project.", "b", "parallel", "Whether the projection should be parallel or not.", "v", "size", "Size of the projected rectangle.", "f", "angle", "Angle to rotate the rectangle by.", "v", "color", "Color to apply to the vertices." );
const idEventDef EV_Thread_FreeDecal( "freeDecal", '\0', DOC_TEXT( "Frees the decal and any associated geometry." ), 1, NULL, "d", "handle", "Handle to the decal." );
const idEventDef EV_Thread_ResetDecal( "resetDecal", '\0', DOC_TEXT( "Clears any existing geometry created on this decal." ), 1, "This does not free the decal, to do that use $event:freeDecal$.", "d", "handle", "Handle to the decal." );

const idEventDef EV_Thread_AllocHudModule( "allocHudModule", 'd', DOC_TEXT( "Allocates a hud overlay using the specified $decl:gui$." ), 3, "If allowInhibit is enabled, the overlay will not be drawn if hud drawing is disabled.\nThe result may be used in calls to $event:setGUIFloat$, etc.", "s", "gui", "Name of the $decl:gui$ to use.", "d", "sort", "Sort value to use.", "b", "allowInhibit", "Whether this overlay will be disabled along with the rest of the hud." );
const idEventDef EV_Thread_FreeHudModule( "freeHudModule", '\0', DOC_TEXT( "Frees a hud overlay." ), 1, NULL, "d", "handle", "Handle to the overlay." );

const idEventDef EV_Thread_RequestDeployment( "requestDeployment", 'b', DOC_TEXT( "Tries to request deployment of an object at the given location, for a player, and returns whether the request succeeded or not." ), 5, "This does not check the deployment grid for a valid location, to do that, use $event:requestCheckedDeployment$.", "e", "player", "Player the request is for.", "d", "object", "Index of the $decl:deployObject$ to use.", "v", "porition", "Position to request deployment at.", "f", "yaw", "Angle to deploy the object at.", "f", "delay", "Extra delay in seconds to add to the deployment time." );
const idEventDef EV_Thread_RequestCheckedDeployment( "requestCheckedDeployment", 'b', DOC_TEXT( "Tries to request deployment of an object at the location they are currently looking at, and returns whether the request succeeded or not." ), 4, NULL, "e", "player", "Player the request is for.", "d", "object", "Index of the $decl:deployObject$ to use.", "f", "yaw", "Angle to deploy the object at.", "f", "delay", "Extra delay in seconds to add to the deployment time." );

const idEventDef EV_Thread_GetWorldMins( "getWorldMins", 'v', DOC_TEXT( "Returns the mins of the world collision model." ), 0, NULL );
const idEventDef EV_Thread_GetWorldMaxs( "getWorldMaxs", 'v', DOC_TEXT( "Returns the maxs of the world collision model." ), 0, NULL );

const idEventDef EV_Thread_SetDeploymentObject( "setDeploymentObject", '\0', DOC_TEXT( "Sets the object to display in the deployment tool." ), 1, NULL, "d", "object", "Index of the $decl:deployObject$ to use." );
const idEventDef EV_Thread_SetDeploymentState( "setDeploymentState", '\0', DOC_TEXT( "Sets the state to show in the deployment tool. Valid values are DR_CLEAR, DR_WARNING, DR_FAILED, DR_CONDITION_FAILED, or DR_OUT_OF_RANGE." ), 1, NULL, "d", "state", "State to set." );
const idEventDef EV_Thread_SetDeploymentMode( "setDeploymentMode", '\0', DOC_TEXT( "Puts the deployment tool in either position or rotation mode." ), 1, NULL, "b", "state", "Position = false, rotation = true." );
const idEventDef EV_Thread_GetDeploymentMode( "getDeploymentMode", 'b', DOC_TEXT( "Returns whether the deployment tool is in position or rotation mode." ), 0, "Position = false, rotation = true." );
const idEventDef EV_Thread_GetDeploymentRotation( "getDeploymentRotation", 'f', DOC_TEXT( "Returns the current angle of rotation of the deployment tool." ), 0, NULL );
const idEventDef EV_Thread_AllowDeploymentRotation( "allowDeploymentRotation", 'b', DOC_TEXT( "Returns whether the selected object supports rotation in the deployment tool." ), 0, NULL );

const idEventDef EV_Thread_GetDefaultFov( "getDefaultFov", 'f', DOC_TEXT( "Returns the local player's default fov value." ), 0, NULL );

const idEventDef EV_Thread_GetTerritory( "getTerritoryForPoint", 'e', DOC_TEXT( "Returns a territory entity at the given position that matches the specified requirements. If no territories are found, the result is $null$." ), 4, "If team is not a team, it will be treated as if $null$ was passed.", "v", "position", "Position to find the territory at.", "o", "team", "Team which the territory must belong to.", "b", "requireTeam", "Whether the team must match the territory's team or not.", "b", "requireActive", "Whether the territory must be marked active or not." );

const idEventDef EV_Thread_GetMaxClients( "getMaxClients", 'd', DOC_TEXT( "Returns the maximum number of players supported." ), 0, "This will always return 32." );
const idEventDef EV_Thread_GetClient( "getClient", 'e', DOC_TEXT( "Returns the player at the specfied client slot, if there is one, otherwise $null$." ), 1, "If the index is out of the range 0 to ( MAX CLIENTS - 1 ), the result will be $null$.\nMAX CLIENTS can be found using $event:getMaxClients$.", "d", "index", "Index of the client to look up." );

const idEventDef EV_Thread_HandleToString( "handleToString", 's', DOC_TEXT( "Creates a string representation of a handle." ), 1, "See also $event:stringToHandle$.", "h", "handle", "Handle to convert." );
const idEventDef EV_Thread_StringToHandle( "stringToHandle", 'h', DOC_TEXT( "Converts a string back into handle form." ), 1, "See also $event:handleToString$.", "s", "text", "Text to convert." );

const idEventDef EV_Thread_ToWideString( "toWStr", 'w', DOC_TEXT( "Converts the given string into a wide string with the same text." ), 1, NULL, "s", "text", "The string to convert." );

const idEventDef EV_Thread_PushLocalizationString( "pushLocString", '\0', DOC_TEXT( "Pushes a string onto the localization stack." ), 1, "See also $event:localizeStringIndexArgs$ and $event:localizeStringArgs$.", "w", "text", "String to push." );
const idEventDef EV_Thread_PushLocalizationStringIndex( "pushLocStringIndex", '\0', DOC_TEXT( "Pushes a string onto the localization stack." ), 1, "See also $event:localizeStringIndexArgs$ and $event:localizeStringArgs$.", "h", "handle", "Handle to a $decl:locStr$ to push." );
const idEventDef EV_Thread_LocalizeStringIndexArgs( "localizeStringIndexArgs", 'w', DOC_TEXT( "Returns localized text for the given $decl:locStr$ using the values on the localization stack for parameters." ), 1, "Use $event:pushLocString$ and $event:pushLocStringIndex$ for adding values to the localization stack.", "h", "handle", "Handle to a $decl:locStr$ to localize." );
const idEventDef EV_Thread_LocalizeStringArgs( "localizeStringArgs", 'w', DOC_TEXT( "Returns localized text for the given $decl:locStr$ using the values on the localization stack for parameters." ), 1, "Use $event:pushLocString$ and $event:pushLocStringIndex$ for adding values to the localization stack.", "s", "name", "Name of the $decl:locStr$ to localize." );
const idEventDef EV_Thread_LocalizeString( "localizeString", 'h', DOC_TEXT( "Returns a handle to a $decl:locStr$ with the given name." ), 1, NULL, "s", "name", "Name of the $decl:locStr$ to look up." );

const idEventDef EV_Thread_GetMatchTimeRemaining( "getMatchTimeRemaining", 'f', DOC_TEXT( "Returns how many seconds are left in the current part of the game, i.e. warmup/in match." ), 0, NULL );
const idEventDef EV_Thread_GetMatchState( "getMatchState", 'f', DOC_TEXT( "Returns the current game state. Will be one of the following values, GS_INACTIVE, GS_WARMUP, GS_COUNTDOWN, GS_GAMEON, GS_GAMEREVIEW, GS_NEXTGAME, or GS_NEXTMAP." ), 0, NULL );

const idEventDef EV_CreateMaskEditSession( "createMaskEditSession", 'o', DOC_TEXT( "Creates an object which can be used to manipulate masks, in game. The object returns will be of type $class:sdDeployMaskEditSession$." ), 0, NULL );

const idEventDef EV_GetCVar( "getCVar", 'o', DOC_TEXT( "Returns a cvar wrapper object, which can be used to look up and modify that cvar's value. The returned object will be of type $class:sdCVarWrapper$." ), 2, NULL, "s", "name", "Name of the cvar to look up.", "s", "default", "Default value to assign to the cvar if it doesn't already exist." );

const idEventDef EV_GetStat( "getStat", 'h', DOC_TEXT( "Returns a handle to a given named stat." ), 1, "If no stat exists with the given name, no stat will be created, and an invalid handle will be returned.", "s", "name", "Name of the stat to find." );
const idEventDef EV_AllocStatInt( "allocStatInt", 'h', DOC_TEXT( "Allocates an integer based stat of the given name, and returns a handle to it." ), 1, "If the stat already exists, a handle to the existing stat will be returned.", "s", "name", "Name of the stat to allocate." );
const idEventDef EV_AllocStatFloat( "allocStatFloat", 'h', DOC_TEXT( "Allocates an float based stat of the given name, and returns a handle to it." ), 1, "If the stat already exists, a handle to the existing stat will be returned.", "s", "name", "Name of the stat to allocate." );
const idEventDef EV_IncreaseStatInt( "increaseStatInt", '\0', DOC_TEXT( "Increases the value of a stat for a client by a given value." ), 3, "This stat must have been allocated using $event:allocStatInt$.", "h", "handle", "Handle to the stat.", "d", "index", "Index of the client.", "d", "value", "Value to increase the stat by." );
const idEventDef EV_IncreaseStatFloat( "increaseStatFloat", '\0', DOC_TEXT( "Increases the value of a stat for a client by a given value." ), 3, "This stat must have been allocated using $event:allocStatFloat$.", "h", "handle", "Handle to the stat.", "d", "index", "Index of the client.", "f", "value", "Value to increase the stat by." );
const idEventDef EV_GetStatValue( "getStatValue", 'f', DOC_TEXT( "Returns the current value of a stat for a given client." ), 2, NULL, "h", "handle", "Handle to the stat.", "d", "index", "Index of the client." );
const idEventDef EV_GetStatDelta( "getStatDelta", 'f', DOC_TEXT( "Returns the delta between the current value of a stat and the last baseline that was stored for a given client." ), 2, "You can call $event:setStatBaseLine$ to store the baseline for a player.", "h", "handle", "Handle to the stat.", "d", "index", "Index of the client." );
const idEventDef EV_SetStatBaseline( "setStatBaseLine", '\0', DOC_TEXT( "Stores the current values of all stats for a client to be used as a baseline." ), 1, "You can call $event:getStatDelta$ to get the delta between the baseline and the current value for a stat entry.", "d", "index", "Index of the client." );

const idEventDef EV_GetClimateSkin( "getClimateSkin", 's', DOC_TEXT( "Looks up the value of a key in the climate skins table for the current map." ), 1, NULL, "s", "key", "Name of the key to look up." );

const idEventDef EV_SendQuickChat( "sendQuickChat", '\0', DOC_TEXT( "Sends a quick chat message to the clients specified, from the client specified." ), 3, "If the sender is $null$, the message will show as coming from their team's high command.\nIf the recipient is $null$, the message will be sent to everyone.", "E", "sender", "Player sending the message.", "d", "index", "Index of the $decl:quickChat$ to send.", "E", "recipient", "Player to send the message to." );

const idEventDef EV_Thread_GetContextEntity( "getContextEntity", 'e', DOC_TEXT( "Returns the entity that the local player used the context menu on, or $null$ if none." ), 0, NULL );

const idEventDef EV_Thread_SetEndGameStatValue( "setEndGameStatValue", '\0', DOC_TEXT( "Sets the value of an end game stat for a specific client to the specified value." ), 3, NULL, "d", "statIndex", "Index of the stat to set the value of.", "e", "player", "Player to set the value for.", "f", "value", "Value to set." );
const idEventDef EV_Thread_SetEndGameStatWinner( "setEndGameStatWinner", '\0', DOC_TEXT( "Sets this client as the winner of the specified end game stat." ), 2, NULL, "d", "statIndex", "Index of the stat to set the winner of.", "E", "player", "Player which won this award." );
const idEventDef EV_Thread_AllocEndGameStat( "allocEndGameStat", 'd', DOC_TEXT( "Allocates an end game stats entry and return its index." ), 0, NULL );
const idEventDef EV_Thread_SendEndGameStats( "sendEndGameStats", '\0', DOC_TEXT( "Finishes the end game stats list, and sends it to all clients." ), 0, NULL );

const idEventDef EV_Thread_HeightMapTrace( "heightMapTrace", 'f', DOC_TEXT( "Performs a fast heightmap based trace, and returns the fraction which passed." ), 2, "If no appropriate heightmap can be found for the trace, the result will be 1.", "v", "start", "Start point of the trace.", "v", "end", "End point of the trace." );

const idEventDef EV_Thread_EnableBotReachability( "enableBotReachability", "sdb" );
const idEventDef EV_Thread_GetNextBotActionIndex( "getNextBotActionIndex", "dd", 'd' );
const idEventDef EV_Thread_GetBotActionOrigin( "getBotActionOrigin", "d", 'v' );
const idEventDef EV_Thread_GetBotActionDeployableType( "getBotActionDeployableType", "d", 'd' );
const idEventDef EV_Thread_GetBotActionBaseGoalType( "getBotActionBaseGoalType", "do", 'd' );

const idEventDef EV_Thread_EnablePlayerHeadModels( "enablePlayerHeadModels" );
const idEventDef EV_Thread_DisablePlayerHeadModels( "disablePlayerHeadModels" );

ABSTRACT_DECLARATION( sdProgramThread, sdSysCallThread )
	EVENT( EV_Thread_Execute,				sdSysCallThread::Event_Execute )
	EVENT( EV_Thread_TerminateThread,		sdSysCallThread::Event_TerminateThread )
	EVENT( EV_Thread_Wait,					sdSysCallThread::Event_Wait )
	EVENT( EV_Thread_WaitFrame,				sdSysCallThread::Event_WaitFrame )
	EVENT( EV_Thread_Print,					sdSysCallThread::Event_Print )
	EVENT( EV_Thread_PrintLn,				sdSysCallThread::Event_PrintLn )
	EVENT( EV_Thread_StackTrace,			sdSysCallThread::Event_StackTrace )
	EVENT( EV_Thread_Assert,				sdSysCallThread::Event_Assert )
	EVENT( EV_Thread_Random,				sdSysCallThread::Event_Random )
	EVENT( EV_Thread_GetTime,				sdSysCallThread::Event_GetTime )
	EVENT( EV_Thread_ToGuiTime,				sdSysCallThread::Event_ToGuiTime )
	EVENT( EV_Thread_KillThread,			sdSysCallThread::Event_KillThread )
	EVENT( EV_Thread_SetThreadName,			sdSysCallThread::Event_SetThreadName )
	EVENT( EV_Thread_GetEntity,				sdSysCallThread::Event_GetEntity )
	EVENT( EV_Thread_GetEntityByID,			sdSysCallThread::Event_GetEntityByID)
	EVENT( EV_Thread_Spawn,					sdSysCallThread::Event_Spawn )
	EVENT( EV_Thread_SpawnClient,			sdSysCallThread::Event_SpawnClient )
	EVENT( EV_Thread_SpawnByType,			sdSysCallThread::Event_SpawnByType )
	EVENT( EV_Thread_AngToForward,			sdSysCallThread::Event_AngToForward )
	EVENT( EV_Thread_AngToRight,			sdSysCallThread::Event_AngToRight )
	EVENT( EV_Thread_AngToUp,				sdSysCallThread::Event_AngToUp )
	EVENT( EV_Thread_Sine,					sdSysCallThread::Event_GetSine )
	EVENT( EV_Thread_Cosine,				sdSysCallThread::Event_GetCosine )
	EVENT( EV_Thread_GetRoot,				sdSysCallThread::Event_ReturnRoot )
	EVENT( EV_Thread_SolveRoots,			sdSysCallThread::Event_SolveRoots )
	EVENT( EV_Thread_ArcTan,				sdSysCallThread::Event_ArcTan )
	EVENT( EV_Thread_ArcTan2,				sdSysCallThread::Event_ArcTan2 )
	EVENT( EV_Thread_ArcCosine,				sdSysCallThread::Event_GetArcCosine )
	EVENT( EV_Thread_ArcSine,				sdSysCallThread::Event_ArcSine )
	EVENT( EV_Thread_AngleNormalize180,		sdSysCallThread::Event_AngleNormalize180 )
	EVENT( EV_Thread_Fabs,					sdSysCallThread::Event_Fabs )
	EVENT( EV_Thread_Floor,					sdSysCallThread::Event_Floor )
	EVENT( EV_Thread_Ceil,					sdSysCallThread::Event_Ceil )
	EVENT( EV_Thread_Mod,					sdSysCallThread::Event_Mod )
	EVENT( EV_Thread_Fmod,					sdSysCallThread::Event_Fmod )
	EVENT( EV_Thread_SquareRoot,			sdSysCallThread::Event_GetSquareRoot )
	EVENT( EV_Thread_Normalize,				sdSysCallThread::Event_VecNormalize )
	EVENT( EV_Thread_VecLength,				sdSysCallThread::Event_VecLength )
	EVENT( EV_Thread_VecLengthSquared,		sdSysCallThread::Event_VecLengthSquared )
	EVENT( EV_Thread_VecCrossProduct,		sdSysCallThread::Event_VecCrossProduct )
	EVENT( EV_Thread_VecToAngles,			sdSysCallThread::Event_VecToAngles )
	EVENT( EV_Thread_RotateVecByAngles,		sdSysCallThread::Event_RotateVecByAngles )
	EVENT( EV_Thread_RotateAngles,			sdSysCallThread::Event_RotateAngles )
	EVENT( EV_Thread_RotateVec,				sdSysCallThread::Event_RotateVec )
	EVENT( EV_Thread_ToLocalSpace,			sdSysCallThread::Event_ToLocalSpace )
	EVENT( EV_Thread_ToWorldSpace,			sdSysCallThread::Event_ToWorldSpace )
	EVENT( EV_Thread_CheckContents,			sdSysCallThread::Event_CheckContents )
	EVENT( EV_Thread_Trace,					sdSysCallThread::Event_Trace )
	EVENT( EV_Thread_TracePoint,			sdSysCallThread::Event_TracePoint )
	EVENT( EV_Thread_TraceOriented,			sdSysCallThread::Event_TraceOriented )
	EVENT( EV_Thread_SaveTrace,				sdSysCallThread::Event_SaveTrace )
	EVENT( EV_Thread_FreeTrace,				sdSysCallThread::Event_FreeTrace )

	EVENT( EV_GetTraceFraction,				sdSysCallThread::Event_GetTraceFraction )
	EVENT( EV_GetTraceEndPos,				sdSysCallThread::Event_GetTraceEndPos )
	EVENT( EV_GetTracePoint,				sdSysCallThread::Event_GetTracePoint )
	EVENT( EV_GetTraceNormal,				sdSysCallThread::Event_GetTraceNormal )
	EVENT( EV_GetTraceEntity,				sdSysCallThread::Event_GetTraceEntity )
	EVENT( EV_GetTraceSurfaceFlags,			sdSysCallThread::Event_GetTraceSurfaceFlags )
	EVENT( EV_GetTraceSurfaceType,			sdSysCallThread::Event_GetTraceSurfaceType )
	EVENT( EV_GetTraceSurfaceColor,			sdSysCallThread::Event_GetTraceSurfaceColor )
	EVENT( EV_GetTraceJoint,				sdSysCallThread::Event_GetTraceJoint )
	EVENT( EV_GetTraceBody,					sdSysCallThread::Event_GetTraceBody )

	EVENT( EV_SetShaderParm,				sdSysCallThread::Event_SetShaderParm )
	EVENT( EV_Thread_StartMusic,			sdSysCallThread::Event_StartMusic )
	EVENT( EV_Thread_StartSoundDirect,		sdSysCallThread::Event_StartSoundDirect )
	EVENT( EV_Thread_Warning,				sdSysCallThread::Event_Warning )
	EVENT( EV_Thread_Error,					sdSysCallThread::Event_Error )
	EVENT( EV_Thread_StrLen,				sdSysCallThread::Event_StrLen )
	EVENT( EV_Thread_StrLeft,				sdSysCallThread::Event_StrLeft )
	EVENT( EV_Thread_StrRight,				sdSysCallThread::Event_StrRight )
	EVENT( EV_Thread_StrSkip,				sdSysCallThread::Event_StrSkip )
	EVENT( EV_Thread_StrMid,				sdSysCallThread::Event_StrMid )
	EVENT( EV_Thread_StrToFloat,			sdSysCallThread::Event_StrToFloat )
	EVENT( EV_Thread_IsClient,				sdSysCallThread::Event_IsClient )
	EVENT( EV_Thread_IsServer,				sdSysCallThread::Event_IsServer )
	EVENT( EV_Thread_DoClientSideStuff,		sdSysCallThread::Event_DoClientSideStuff )
	EVENT( EV_Thread_IsNewFrame,			sdSysCallThread::Event_IsNewFrame )
	EVENT( EV_Thread_GetFrameTime,			sdSysCallThread::Event_GetFrameTime )
	EVENT( EV_Thread_GetTicsPerSecond,		sdSysCallThread::Event_GetTicsPerSecond )
	EVENT( EV_Thread_BroadcastToolTip,		sdSysCallThread::Event_BroadcastToolTip )

	EVENT( EV_Thread_DebugLine,				sdSysCallThread::Event_DebugLine )
	EVENT( EV_Thread_DebugArrow,			sdSysCallThread::Event_DebugArrow )
	EVENT( EV_Thread_DebugCircle,			sdSysCallThread::Event_DebugCircle )
	EVENT( EV_Thread_DebugBounds,			sdSysCallThread::Event_DebugBounds )
	EVENT( EV_Thread_DrawText,				sdSysCallThread::Event_DrawText )

	EVENT( EV_Thread_GetDeclTypeHandle,				sdSysCallThread::Event_GetDeclTypeHandle )
	EVENT( EV_Thread_GetDeclIndex,					sdSysCallThread::Event_GetDeclIndex )
	EVENT( EV_Thread_GetDeclName,					sdSysCallThread::Event_GetDeclName )
	EVENT( EV_Thread_GetDeclCount,					sdSysCallThread::Event_GetDeclCount )
	EVENT( EV_Thread_ApplyRadiusDamage,				sdSysCallThread::Event_ApplyRadiusDamage )
	EVENT( EV_Thread_FilterEntity,					sdSysCallThread::Event_FilterEntity )
	EVENT( EV_Thread_GetTableCount,					sdSysCallThread::Event_GetTableCount )
	EVENT( EV_Thread_GetTableValue,					sdSysCallThread::Event_GetTableValue )
	EVENT( EV_Thread_GetTableValueExact,			sdSysCallThread::Event_GetTableValueExact )
	EVENT( EV_Thread_GetTypeHandle,					sdSysCallThread::Event_GetTypeHandle )

	EVENT( EV_Thread_Argc,							sdSysCallThread::Event_Argc )
	EVENT( EV_Thread_Argv,							sdSysCallThread::Event_Argv )
	EVENT( EV_Thread_FloatArgv,						sdSysCallThread::Event_FloatArgv )
	EVENT( EV_Thread_SetActionCommand,				sdSysCallThread::Event_SetActionCommand )
	EVENT( EV_Thread_GetTeam,						sdSysCallThread::Event_GetTeam )
	EVENT( EV_Thread_PlayWorldEffect,				sdSysCallThread::Event_PlayWorldEffect )
	EVENT( EV_Thread_PlayWorldEffectRotate,			sdSysCallThread::Event_PlayWorldEffectRotate )
	EVENT( EV_Thread_PlayWorldEffectRotateAlign,	sdSysCallThread::Event_PlayWorldEffectRotateAlign )
	EVENT( EV_Thread_PlayWorldBeamEffect,			sdSysCallThread::Event_PlayWorldBeamEffect )
	EVENT( EV_Thread_GetLocalPlayer,				sdSysCallThread::Event_GetLocalPlayer )
	EVENT( EV_Thread_GetLocalViewPlayer,			sdSysCallThread::Event_GetLocalViewPlayer )
	
	EVENT( EV_Thread_SetGUIFloat,					sdSysCallThread::Event_SetGUIFloat )
	EVENT( EV_Thread_SetGUIHandle,					sdSysCallThread::Event_SetGUIInt )
	EVENT( EV_Thread_SetGUIVec2,					sdSysCallThread::Event_SetGUIVec2 )
	EVENT( EV_Thread_SetGUIVec3,					sdSysCallThread::Event_SetGUIVec3 )
	EVENT( EV_Thread_SetGUIVec4,					sdSysCallThread::Event_SetGUIVec4 )
	EVENT( EV_Thread_SetGUIString,					sdSysCallThread::Event_SetGUIString )
	EVENT( EV_Thread_SetGUIWString,					sdSysCallThread::Event_SetGUIWString )
	EVENT( EV_Thread_GUIPostNamedEvent,				sdSysCallThread::Event_GUIPostNamedEvent )
	EVENT( EV_Thread_GetGUIFloat,					sdSysCallThread::Event_GetGUIFloat )
	EVENT( EV_Thread_SetGUITheme,					sdSysCallThread::Event_SetGUITheme )
	EVENT( EV_Thread_AddNotifyIcon,					sdSysCallThread::Event_AddNotifyIcon )
	EVENT( EV_Thread_RemoveNotifyIcon,				sdSysCallThread::Event_RemoveNotifyIcon )
	EVENT( EV_Thread_BumpNotifyIcon,				sdSysCallThread::Event_BumpNotifyIcon )

	EVENT( EV_Thread_ClearDeployRequest,			sdSysCallThread::Event_ClearDeployRequest )
	EVENT( EV_Thread_GetDeployMask,					sdSysCallThread::Event_GetDeployMask )
	EVENT( EV_Thread_CheckDeployMask,				sdSysCallThread::Event_CheckDeployMask )
	EVENT( EV_Thread_GetWorldPlayZoneIndex,			sdSysCallThread::Event_GetWorldPlayZoneIndex )


	EVENT( EV_Thread_AllocTargetTimer,				sdSysCallThread::Event_AllocTargetTimer )
	EVENT( EV_Thread_GetTargetTimerValue,			sdSysCallThread::Event_GetTargetTimerValue )
	EVENT( EV_Thread_SetTargetTimerValue,			sdSysCallThread::Event_SetTargetTimerValue )

	EVENT( EV_Thread_AllocCMIcon,					sdSysCallThread::Event_AllocCMIcon )
	EVENT( EV_Thread_FreeCMIcon,					sdSysCallThread::Event_FreeCMIcon )
	EVENT( EV_Thread_SetCMIconSize,					sdSysCallThread::Event_SetCMIconSize )
	EVENT( EV_Thread_SetCMIconUnknownSize,			sdSysCallThread::Event_SetCMIconUnknownSize )
	EVENT( EV_Thread_SetCMIconSize2d,				sdSysCallThread::Event_SetCMIconSize2d )
	EVENT( EV_Thread_SetCMIconUnknownSize2d,		sdSysCallThread::Event_SetCMIconUnknownSize2d )

	EVENT( EV_Thread_SetCMIconSizeMode,				sdSysCallThread::Event_SetCMIconSizeMode )
	EVENT( EV_Thread_SetCMIconPositionMode,			sdSysCallThread::Event_SetCMIconPositionMode )
	EVENT( EV_Thread_SetCMIconOrigin,				sdSysCallThread::Event_SetCMIconOrigin )
	EVENT( EV_Thread_SetCMIconColor,				sdSysCallThread::Event_SetCMIconColor )
	EVENT( EV_Thread_SetCMIconColorMode,			sdSysCallThread::Event_SetCMIconColorMode )
	EVENT( EV_Thread_SetCMIconDrawMode,				sdSysCallThread::Event_SetCMIconDrawMode )
	EVENT( EV_Thread_SetCMIconAngle,				sdSysCallThread::Event_SetCMIconAngle )
	EVENT( EV_Thread_SetCMIconSides,				sdSysCallThread::Event_SetCMIconSides )
	EVENT( EV_Thread_HideCMIcon,					sdSysCallThread::Event_HideCMIcon )
	EVENT( EV_Thread_ShowCMIcon,					sdSysCallThread::Event_ShowCMIcon )
	EVENT( EV_Thread_AddCMIconRequirement,			sdSysCallThread::Event_AddCMIconRequirement )
	EVENT( EV_Thread_SetCMIconMaterial,				sdSysCallThread::Event_SetCMIconMaterial )
	EVENT( EV_Thread_SetCMIconUnknownMaterial,		sdSysCallThread::Event_SetCMIconUnknownMaterial )
	EVENT( EV_Thread_SetCMIconFireteamMaterial,		sdSysCallThread::Event_SetCMIconFireteamMaterial )
	EVENT( EV_Thread_SetCMIconGuiMessage,			sdSysCallThread::Event_SetCMIconGuiMessage )
	EVENT( EV_Thread_SetCMIconFlag,					sdSysCallThread::Event_SetCMIconFlag )
	EVENT( EV_Thread_ClearCMIconFlag,				sdSysCallThread::Event_ClearCMIconFlag )
	EVENT( EV_Thread_SetCMIconArcAngle,				sdSysCallThread::Event_SetCMIconArcAngle )
	EVENT( EV_Thread_SetCMIconShaderParm,			sdSysCallThread::Event_SetCMIconShaderParm )
	EVENT( EV_Thread_GetCMIconFlags,				sdSysCallThread::Event_GetCMIconFlags )
	EVENT( EV_Thread_FlashCMIcon,					sdSysCallThread::Event_FlashCMIcon )

	EVENT( EV_Thread_GetKey,						sdSysCallThread::Event_GetEntityDefKey )
	EVENT( EV_Thread_GetIntKey,						sdSysCallThread::Event_GetEntityDefIntKey )
	EVENT( EV_Thread_GetFloatKey,					sdSysCallThread::Event_GetEntityDefFloatKey )
	EVENT( EV_Thread_GetVectorKey,					sdSysCallThread::Event_GetEntityDefVectorKey )

	EVENT( EV_Thread_AllocDecal,					sdSysCallThread::Event_AllocDecal )
	EVENT( EV_Thread_ProjectDecal,					sdSysCallThread::Event_ProjectDecal )
	EVENT( EV_Thread_FreeDecal,						sdSysCallThread::Event_FreeDecal )
	EVENT( EV_Thread_ResetDecal,					sdSysCallThread::Event_ResetDecal )

	EVENT( EV_Thread_AllocHudModule,				sdSysCallThread::Event_AllocHudModule )
	EVENT( EV_Thread_FreeHudModule,					sdSysCallThread::Event_FreeHudModule )

	EVENT( EV_Thread_RequestDeployment,				sdSysCallThread::Event_RequestDeployment )
	EVENT( EV_Thread_RequestCheckedDeployment,		sdSysCallThread::Event_RequestCheckedDeployment )


	EVENT( EV_Thread_GetWorldMins,					sdSysCallThread::Event_GetWorldMins )
	EVENT( EV_Thread_GetWorldMaxs,					sdSysCallThread::Event_GetWorldMaxs )

	EVENT( EV_Thread_SetDeploymentObject,			sdSysCallThread::Event_SetDeploymentObject )
	EVENT( EV_Thread_SetDeploymentState,			sdSysCallThread::Event_SetDeploymentState )

	EVENT( EV_Thread_SetDeploymentMode,				sdSysCallThread::Event_SetDeploymentMode )
	EVENT( EV_Thread_GetDeploymentMode,				sdSysCallThread::Event_GetDeploymentMode )
	EVENT( EV_Thread_GetDeploymentRotation,			sdSysCallThread::Event_GetDeploymentRotation )
	EVENT( EV_Thread_AllowDeploymentRotation,		sdSysCallThread::Event_AllowDeploymentRotation )
	
	EVENT( EV_Thread_GetDefaultFov,					sdSysCallThread::Event_GetDefaultFov )

	EVENT( EV_Thread_GetTerritory,					sdSysCallThread::Event_GetTerritory )
	
	EVENT( EV_Thread_GetMaxClients,					sdSysCallThread::Event_GetMaxClients )
	EVENT( EV_Thread_GetClient,						sdSysCallThread::Event_GetClient )

	EVENT( EV_Thread_HandleToString,				sdSysCallThread::Event_HandleToString )
	EVENT( EV_Thread_StringToHandle,				sdSysCallThread::Event_StringToHandle )

	EVENT( EV_Thread_ToWideString,					sdSysCallThread::Event_ToWideString )

	EVENT( EV_Thread_PushLocalizationString,		sdSysCallThread::Event_PushLocalizationString )
	EVENT( EV_Thread_PushLocalizationStringIndex,	sdSysCallThread::Event_PushLocalizationStringIndex )
	EVENT( EV_Thread_LocalizeStringIndexArgs,		sdSysCallThread::Event_LocalizeStringIndexArgs )
	EVENT( EV_Thread_LocalizeStringArgs,			sdSysCallThread::Event_LocalizeStringArgs )
	EVENT( EV_Thread_LocalizeString,				sdSysCallThread::Event_LocalizeString )

	EVENT( EV_Thread_GetMatchTimeRemaining,			sdSysCallThread::Event_GetMatchTimeRemaining )
	EVENT( EV_Thread_GetMatchState,					sdSysCallThread::Event_GetMatchState )

	EVENT( EV_CreateMaskEditSession,				sdSysCallThread::Event_CreateMaskEditSession )

	EVENT( EV_GetCVar,								sdSysCallThread::Event_GetCVar )

	EVENT( EV_GetStat,								sdSysCallThread::Event_GetStat )
	EVENT( EV_AllocStatInt,							sdSysCallThread::Event_AllocStatInt )
	EVENT( EV_AllocStatFloat,						sdSysCallThread::Event_AllocStatFloat )
	EVENT( EV_IncreaseStatInt,						sdSysCallThread::Event_IncreaseStatInt )
	EVENT( EV_IncreaseStatFloat,					sdSysCallThread::Event_IncreaseStatFloat )
	EVENT( EV_GetStatValue,							sdSysCallThread::Event_GetStatValue )
	EVENT( EV_GetStatDelta,							sdSysCallThread::Event_GetStatDelta )
	EVENT( EV_SetStatBaseline,						sdSysCallThread::Event_SetStatBaseLine )

	EVENT( EV_GetClimateSkin,						sdSysCallThread::Event_GetClimateSkin )

	EVENT( EV_SendQuickChat,						sdSysCallThread::Event_SendQuickChat )

	EVENT( EV_Thread_GetContextEntity,				sdSysCallThread::Event_GetContextEntity )

	EVENT( EV_Thread_SetEndGameStatValue,			sdSysCallThread::Event_SetEndGameStatValue )
	EVENT( EV_Thread_SetEndGameStatWinner,			sdSysCallThread::Event_SetEndGameStatWinner )
	EVENT( EV_Thread_AllocEndGameStat,				sdSysCallThread::Event_AllocEndGameStat )
	EVENT( EV_Thread_SendEndGameStats,				sdSysCallThread::Event_SendEndGameStats )

	EVENT( EV_Thread_HeightMapTrace,				sdSysCallThread::Event_HeightMapTrace )

	EVENT( EV_Thread_EnableBotReachability,			sdSysCallThread::Event_EnableBotReachability )
	
	EVENT( EV_Thread_GetNextBotActionIndex,			sdSysCallThread::Event_GetNextBotActionIndex )
	EVENT( EV_Thread_GetBotActionOrigin,			sdSysCallThread::Event_GetBotActionOrigin )
	EVENT( EV_Thread_GetBotActionDeployableType,	sdSysCallThread::Event_GetBotActionDeployableType )
	EVENT( EV_Thread_GetBotActionBaseGoalType,		sdSysCallThread::Event_GetBotActionBaseGoalType )

	EVENT( EV_Thread_EnablePlayerHeadModels,		sdSysCallThread::Event_EnablePlayerHeadModels )
	EVENT( EV_Thread_DisablePlayerHeadModels,		sdSysCallThread::Event_DisablePlayerHeadModels )
END_CLASS


/*
================
sdSysCallThread::Event_SetThreadName
================
*/
void sdSysCallThread::Event_SetThreadName( const char *name ) {
	SetName( name );
}

/*
================
sdSysCallThread::Event_Execute
================
*/
void sdSysCallThread::Event_Execute( void ) {
	Execute();
}

/*
================
sdSysCallThread::Event_TerminateThread
================
*/
void sdSysCallThread::Event_TerminateThread( int num ) {
	gameLocal.program->KillThread( num );
}

/*
================
sdSysCallThread::Event_Wait
================
*/
void sdSysCallThread::Event_Wait( float time ) {
	Wait( time );
}

/*
================
sdSysCallThread::Event_WaitFrame
================
*/
void sdSysCallThread::Event_WaitFrame( void ) {
	WaitFrame();
}

/*
================
sdSysCallThread::Event_Print
================
*/
void sdSysCallThread::Event_Print( const char *text ) {
	gameLocal.Printf( "%s", text );
}

/*
================
sdSysCallThread::Event_PrintLn
================
*/
void sdSysCallThread::Event_PrintLn( const char *text ) {
	gameLocal.Printf( "%s\n", text );
}

/*
================
sdSysCallThread::Event_StackTrace
================
*/
void sdSysCallThread::Event_StackTrace( void ) {
	sdProgramThread* thread = gameLocal.program->GetCurrentThread();
	if ( thread != NULL ) {
		thread->StackTrace();
	}
}

/*
================
sdSysCallThread::Event_Assert
================
*/
void sdSysCallThread::Event_Assert( bool value ) {
	if ( !value ) {
		Assert();
	}
}

/*
================
sdSysCallThread::Event_GetCVar
================
*/
void sdSysCallThread::Event_GetCVar( const char* name, const char* defaultValue ) const {
	sdCVarWrapper* wrapper = new sdCVarWrapper();
	wrapper->Init( name, defaultValue );
	sdProgram::ReturnObject( wrapper->GetScriptObject() );
}

/*
================
sdSysCallThread::Event_Random
================
*/
void sdSysCallThread::Event_Random( float range ) const {
	float result;

	result = gameLocal.random.RandomFloat();
	sdProgram::ReturnFloat( range * result );
}

/*
================
sdSysCallThread::Event_GetTime
================
*/
void sdSysCallThread::Event_GetTime( void ) {
	sdProgram::ReturnFloat( MS2SEC( gameLocal.time ) );
}

/*
================
sdSysCallThread::Event_GetTime
================
*/
void sdSysCallThread::Event_ToGuiTime( float time ) const {
  	sdProgram::ReturnFloat( MS2SEC( gameLocal.ToGuiTime( SEC2MS( time ) ) ) );
}

/*
================
sdSysCallThread::Event_KillThread
================
*/
void sdSysCallThread::Event_KillThread( const char *name ) {
	gameLocal.program->KillThread( name );
}

/*
================
sdSysCallThread::Event_GetEntity
================
*/
void sdSysCallThread::Event_GetEntity( const char *name ) {
	int			entnum;
	idEntity	*ent;

	assert( name );

	if ( name[ 0 ] == '*' ) {
		entnum = atoi( &name[ 1 ] );
		if ( ( entnum < 0 ) || ( entnum >= MAX_GENTITIES ) ) {
			gameLocal.Error( "Entity number in string out of range." );
		}
		sdProgram::ReturnEntity( gameLocal.entities[ entnum ] );
	} else {
		ent = gameLocal.FindEntity( name );
		sdProgram::ReturnEntity( ent );
	}
}

/*
================
sdSysCallThread::Event_GetEntityByID
================
*/
void sdSysCallThread::Event_GetEntityByID( const char* stringSpawnID ) {
	// need to interpret the string
	int value = sdTypeFromString< int >( stringSpawnID );
	sdProgram::ReturnEntity( gameLocal.EntityForSpawnId( value ) );
}

/*
================
sdSysCallThread::Event_Spawn
================
*/
void sdSysCallThread::Event_Spawn( const char *classname ) {
	if ( gameLocal.insideExecuteMapChange ) {
		gameLocal.Error( "sdSysCallThread::Event_Spawn 'spawn' called during map start" );
	}

	idDict temp;
	temp.Set( "classname", classname );

	idEntity *ent;
	gameLocal.SpawnEntityDef( temp, true, &ent );

	sdProgram::ReturnEntity( ent );
}

/*
================
sdSysCallThread::Event_SpawnClient
================
*/
void sdSysCallThread::Event_SpawnClient( const char *classname ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		sdProgram::ReturnObject( NULL );
		return;
	}

	rvClientEntity *ent;

	idDict temp;
	temp.Set( "classname", classname );
	gameLocal.SpawnClientEntityDef( temp, &ent );
	sdClientScriptEntity *sc = ent->Cast<sdClientScriptEntity>();
	if ( !sc ) {
		gameLocal.Error( "Cannot spawn unscripted client entities from scripts" );
		return;
	}
	sdProgram::ReturnObject( sc->GetScriptObject() );
}

/*
================
sdSysCallThread::EV_Thread_SpawnByType
================
*/
void sdSysCallThread::Event_SpawnByType( int index ) {
	const idDeclEntityDef* def = gameLocal.declEntityDefType[ index ];
	if ( !def ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	idEntity* ent;
	gameLocal.SpawnEntityDef( def->dict, true, &ent );
	sdProgram::ReturnEntity( ent );
}


/*
================
sdSysCallThread::Event_AngToForward
================
*/
void sdSysCallThread::Event_AngToForward( idAngles &ang ) {
	sdProgram::ReturnVector( ang.ToForward() );
}

/*
================
sdSysCallThread::Event_AngToRight
================
*/
void sdSysCallThread::Event_AngToRight( idAngles &ang ) {
	idVec3 vec;

	ang.ToVectors( NULL, &vec );
	sdProgram::ReturnVector( vec );
}

/*
================
sdSysCallThread::Event_AngToUp
================
*/
void sdSysCallThread::Event_AngToUp( idAngles &ang ) {
	idVec3 vec;

	ang.ToVectors( NULL, NULL, &vec );
	sdProgram::ReturnVector( vec );
}

/*
================
sdSysCallThread::Event_GetSine
================
*/
void sdSysCallThread::Event_GetSine( float angle ) {
	sdProgram::ReturnFloat( idMath::Sin( DEG2RAD( angle ) ) );
}

/*
================
sdSysCallThread::Event_GetCosine
================
*/
void sdSysCallThread::Event_GetCosine( float angle ) {
	sdProgram::ReturnFloat( idMath::Cos( DEG2RAD( angle ) ) );
}

/*
================
sdSysCallThread::ReturnRoot
================
*/
void sdSysCallThread::Event_ReturnRoot( int index ) {
	if ( index == 0 ) {
		sdProgram::ReturnFloat( cachedRoot[ 0 ] );
		return;
	}
	sdProgram::ReturnFloat( cachedRoot[ 1 ] );
}

/*
================
sdSysCallThread::SolveRoots
================
*/
void sdSysCallThread::Event_SolveRoots( float a, float b, float c ) {
	float d = Square( b ) - ( 4 * a * c );
	float x;
	if( d < 0 ) {
		sdProgram::ReturnInteger( 0 );
		return;
	}

	if( d == 0 ) {
		cachedRoot[ 0 ] = ( -b / ( 2 * a ) );
		sdProgram::ReturnInteger( 1 );
		return;
	}

	d = sqrt( d );

	x = 1 / ( 2 * a );
	cachedRoot[ 0 ] = ( -b + d ) * x;
	cachedRoot[ 1 ] = ( -b - d ) * x;

	sdProgram::ReturnInteger( 2 );
}

/*
================
sdSysCallThread::Event_ArcTan2
================
*/
void sdSysCallThread::Event_ArcTan2( float a1, float a2 ) {
	sdProgram::ReturnFloat( RAD2DEG( atan2( a1, a2 ) ) );
}

/*
================
sdSysCallThread::Event_ArcTan
================
*/
void sdSysCallThread::Event_ArcTan( float a ) {
	sdProgram::ReturnFloat( RAD2DEG( atan( a ) ) );
}

/*
================
sdSysCallThread::Event_GetArcCosine
================
*/
void sdSysCallThread::Event_GetArcCosine( float dot ) {
	sdProgram::ReturnFloat( RAD2DEG( idMath::ACos( dot ) ) );
}

/*
================
sdSysCallThread::Event_ArcSine
================
*/
void sdSysCallThread::Event_ArcSine( float a ) {
	sdProgram::ReturnFloat( RAD2DEG( idMath::ASin( a ) ) );
}

/*
================
sdSysCallThread::Event_AngleNormalize180
================
*/
void sdSysCallThread::Event_AngleNormalize180( float angle ) {
	sdProgram::ReturnFloat( idMath::AngleNormalize180( angle ) );
}

/*
================
sdSysCallThread::Event_Mod
================
*/
void sdSysCallThread::Event_Mod( int value, int mod ) {
	sdProgram::ReturnInteger( value % mod );
}

/*
================
sdSysCallThread::Event_Fmod
================
*/
void sdSysCallThread::Event_Fmod( float value, float mod ) {
	sdProgram::ReturnFloat( idMath::Fmod( value, mod ) );
}

/*
================
sdSysCallThread::Event_Fabs
================
*/
void sdSysCallThread::Event_Fabs( float value ) {
	sdProgram::ReturnFloat( idMath::Fabs( value ) );
}

/*
================
sdSysCallThread::Event_Floor
================
*/
void sdSysCallThread::Event_Floor( float value ) {
	sdProgram::ReturnFloat( idMath::Floor( value ) );
}

/*
================
sdSysCallThread::Event_Ceil
================
*/
void sdSysCallThread::Event_Ceil( float value ) {
	sdProgram::ReturnFloat( idMath::Ceil( value ) );
}

/*
================
sdSysCallThread::Event_GetSquareRoot
================
*/
void sdSysCallThread::Event_GetSquareRoot( float theSquare ) {
	sdProgram::ReturnFloat( idMath::Sqrt( theSquare ) );
}

/*
================
sdSysCallThread::Event_VecNormalize
================
*/
void sdSysCallThread::Event_VecNormalize( idVec3 &vec ) {
	idVec3 n;

	n = vec;
	n.Normalize();
	sdProgram::ReturnVector( n );
}

/*
================
sdSysCallThread::Event_VecLength
================
*/
void sdSysCallThread::Event_VecLength( const idVec3& vec ) {
	sdProgram::ReturnFloat( vec.Length() );
}

/*
================
sdSysCallThread::Event_VecLengthSquared
================
*/
void sdSysCallThread::Event_VecLengthSquared( const idVec3& vec ) {
	sdProgram::ReturnFloat( vec.LengthSqr() );
}

/*
================
sdSysCallThread::Event_VecCrossProduct
================
*/
void sdSysCallThread::Event_VecCrossProduct( const idVec3 &vec1, const idVec3 &vec2 ) {
	sdProgram::ReturnVector( vec1.Cross( vec2 ) );
}

/*
================
sdSysCallThread::Event_VecToAngles
================
*/
void sdSysCallThread::Event_VecToAngles( const idVec3 &vec ) {
	idAngles ang = vec.ToAngles();
	sdProgram::ReturnVector( idVec3( ang[ 0 ], ang[ 1 ], ang[ 2 ] ) );
}

/*
================
sdSysCallThread::Event_RotateVecByAngles
================
*/
void sdSysCallThread::Event_RotateVecByAngles( const idVec3& vec, const idAngles &angles ) {
	idMat3 matrix = angles.ToMat3();

	sdProgram::ReturnVector( matrix * vec );
}

/*
================
sdSysCallThread::Event_RotateAngles
================
*/
void sdSysCallThread::Event_RotateAngles( const idAngles& angles1, const idAngles& angles2 ) {
	idMat3 matrix1 = angles1.ToMat3();
	idMat3 matrix2 = angles2.ToMat3();

	idMat3 matrix3 = matrix1 * matrix2;

	idAngles resultAngles = matrix3.ToAngles();
	sdProgram::ReturnVector( idVec3( resultAngles.pitch, resultAngles.yaw, resultAngles.roll ) );
}

/*
================
sdSysCallThread::Event_RotateVec
================
*/
void sdSysCallThread::Event_RotateVec( const idVec3& vec, const idVec3& axis, float angle ) {
	idVec3 normAxis = axis;
	normAxis.Normalize();
	idRotation rotation( vec3_origin, normAxis, angle );

	sdProgram::ReturnVector( rotation.ToMat3() * vec );
}

/*
================
sdSysCallThread::Event_ToLocalSpace
================
*/
void sdSysCallThread::Event_ToLocalSpace( const idVec3& vec, idEntity* ent ) {
	idMat3 orientation = ent->GetPhysics()->GetAxis().Transpose();
	const idVec3& origin = ent->GetPhysics()->GetOrigin();

	sdProgram::ReturnVector( ( vec - origin ) * orientation );
}

/*
================
sdSysCallThread::Event_ToWorldSpace
================
*/
void sdSysCallThread::Event_ToWorldSpace( const idVec3& vec, idEntity* ent ) {
	idMat3 orientation = ent->GetPhysics()->GetAxis();
	const idVec3& origin = ent->GetPhysics()->GetOrigin();

	sdProgram::ReturnVector( ( vec * orientation ) + origin );
}

/*
================
sdSysCallThread::Event_CheckContents
================
*/
void sdSysCallThread::Event_CheckContents( const idVec3 &start, const idVec3 &mins, const idVec3 &maxs, int contents_mask, idScriptObject* passObject ) {
	idEntity* ent = NULL;
	rvClientEntity* clientEnt = NULL;
	
	if ( passObject ) {
		ent = passObject->GetClass()->Cast< idEntity >();
		if ( !ent ) {
			rvClientEntity* clientEnt = passObject->GetClass()->Cast< rvClientEntity >();
			if ( clientEnt ) {
				clientEnt->MakeCurrent();
				ent = rvClientEntity::GetClientMaster();
			}
		}
	}

	idBounds bounds( mins, maxs );
	idBounds absBounds = bounds.Translate( start );
	int contents = 0;
	idClipModel* tempClip = NULL;
	if ( mins != vec3_origin || maxs != vec3_origin ) {
		// make a temp clip model
		// FIXME: this needs to change as this creates a new idCollisionModel for every different min/max
		tempClip = new idClipModel( idTraceModel( bounds ), false );
	}

	// check against world
	contents |= gameLocal.clip.ContentsWorld( CLIP_DEBUG_PARMS_SCRIPT start, tempClip, mat3_identity, contents_mask );

	// check against entities
	const idClipModel* clipModels[ MAX_GENTITIES ];
	int numClips = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS absBounds, contents_mask, clipModels, MAX_GENTITIES, ent );
	for ( int i = 0; i < numClips; i++ ) {
		const idClipModel* cm = clipModels[ i ];
		contents |= gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_SCRIPT start, tempClip, mat3_identity, contents_mask, cm, cm->GetOrigin(), cm->GetAxis() );

		// check if the bounds are totally contained by the bounds of another 
		idEntity* other = cm->GetEntity();
		if ( other == NULL || other->GetPhysics() == NULL ) {
			continue;
		}

		// check if within bounds only if explicitly set for ent
		if ( ent != NULL && ent->CheckEntityContentBoundsFilter( other ) ) {
			// use a quite large epsilon to give some margin for error
			if ( other->GetPhysics()->GetAbsBounds().ContainsBounds( absBounds, 2.0f ) ) {
				contents |= cm->GetContents() & contents_mask;
			}
		}
	}

	if ( tempClip != NULL ) {		
		gameLocal.clip.DeleteClipModel( tempClip );
	}

	if ( clientEnt ) {
		clientEnt->ResetCurrent();
	}

	sdProgram::ReturnFloat( contents );
}

/*
================
sdSysCallThread::Event_Trace
================
*/
void sdSysCallThread::Event_Trace( const idVec3 &start, const idVec3 &end, const idVec3 &mins, const idVec3 &maxs, int contents_mask, idScriptObject* passObject ) {
	idEntity* ent = NULL;
	rvClientEntity* clientEnt = NULL;
	
	if ( passObject ) {		
		ent = passObject->GetClass()->Cast< idEntity >();
		if ( !ent ) {
			rvClientEntity* clientEnt = passObject->GetClass()->Cast< rvClientEntity >();
			if ( clientEnt ) {
				clientEnt->MakeCurrent();
				ent = rvClientEntity::GetClientMaster();
			}
		}
	}

	if ( mins == vec3_origin && maxs == vec3_origin ) {
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS_SCRIPT trace, start, end, contents_mask, ent );
	} else {
		gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS_SCRIPT trace, start, end, idBounds( mins, maxs ), mat3_identity, contents_mask, ent );
	}

	if ( clientEnt ) {
		clientEnt->ResetCurrent();
	}

	sdProgram::ReturnFloat( trace.fraction );
}

/*
================
sdSysCallThread::Event_TracePoint
================
*/
void sdSysCallThread::Event_TracePoint( const idVec3 &start, const idVec3 &end, int contents_mask, idScriptObject* passObject ) {
	idEntity* ent = NULL;
	rvClientEntity* clientEnt = NULL;
	
	if ( passObject ) {		
		ent = passObject->GetClass()->Cast< idEntity >();
		if ( !ent ) {
			rvClientEntity* clientEnt = passObject->GetClass()->Cast< rvClientEntity >();
			if ( clientEnt ) {
				clientEnt->MakeCurrent();
				ent = rvClientEntity::GetClientMaster();
			}
		}
	}

	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS_SCRIPT trace, start, end, contents_mask, ent );

	if ( clientEnt ) {
		clientEnt->ResetCurrent();
	}

	sdProgram::ReturnFloat( trace.fraction );
}

/*
================
sdSysCallThread::Event_TraceOriented
================
*/
void sdSysCallThread::Event_TraceOriented( const idVec3 &start, const idVec3 &end, const idVec3 &mins, const idVec3 &maxs, const idVec3& angles, int contents_mask, idScriptObject* passObject ) {
	idEntity* ent = NULL;
	rvClientEntity* clientEnt = NULL;
	
	if ( passObject ) {		
		ent = passObject->GetClass()->Cast< idEntity >();
		if ( !ent ) {
			rvClientEntity* clientEnt = passObject->GetClass()->Cast< rvClientEntity >();
			if ( clientEnt ) {
				clientEnt->MakeCurrent();
				ent = rvClientEntity::GetClientMaster();
			}
		}
	}

	if ( mins == vec3_origin && maxs == vec3_origin ) {
		gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS_SCRIPT trace, start, end, contents_mask, ent );
	} else {
		gameLocal.clip.TraceBounds( CLIP_DEBUG_PARMS_SCRIPT trace, start, end, idBounds( mins, maxs ), idAngles( angles ).ToMat3(), contents_mask, ent );
	}

	if ( clientEnt ) {
		clientEnt->ResetCurrent();
	}

	sdProgram::ReturnFloat( trace.fraction );
}

/*
================
sdSysCallThread::Event_SaveTrace
================
*/
void sdSysCallThread::Event_SaveTrace( void ) {
	sdLoggedTrace* loggedTrace = gameLocal.RegisterLoggedTrace( trace );
	sdProgram::ReturnObject( loggedTrace ? loggedTrace->GetScriptObject() : NULL );
}

/*
================
sdSysCallThread::Event_FreeTrace
================
*/
void sdSysCallThread::Event_FreeTrace( idScriptObject* object ) {
	if ( !object ) {
		gameLocal.Warning( "sdSysCallThread::Event_FreeTrace NULL object" );
		return;
	}
	sdLoggedTrace* trace = object->GetClass()->Cast< sdLoggedTrace >();
	gameLocal.FreeLoggedTrace( trace );
}

/*
================
sdSysCallThread::Event_GetTraceFraction
================
*/
void sdSysCallThread::Event_GetTraceFraction( void ) {
	sdProgram::ReturnFloat( trace.fraction );
}

/*
================
sdSysCallThread::Event_GetTraceEndPos
================
*/
void sdSysCallThread::Event_GetTraceEndPos( void ) {
	sdProgram::ReturnVector( trace.endpos );
}

/*
================
sdSysCallThread::Event_GetTracePoint
================
*/
void sdSysCallThread::Event_GetTracePoint( void ) {
	sdProgram::ReturnVector( trace.c.point );
}

/*
================
sdSysCallThread::Event_GetTraceNormal
================
*/
void sdSysCallThread::Event_GetTraceNormal( void ) {
	if ( trace.fraction < 1.0f ) {
		sdProgram::ReturnVector( trace.c.normal );
	} else {
		sdProgram::ReturnVector( vec3_origin );
	}
}

/*
================
sdSysCallThread::Event_GetTraceEntity
================
*/
void sdSysCallThread::Event_GetTraceEntity( void ) {
	if ( trace.fraction < 1.0f ) {
		sdProgram::ReturnEntity( gameLocal.entities[ trace.c.entityNum ] );
	} else {
		sdProgram::ReturnEntity( NULL );
	}
}

/*
================
sdSysCallThread::Event_GetTraceJoint
================
*/
void sdSysCallThread::Event_GetTraceJoint( void ) {
	if ( trace.fraction < 1.0f && trace.c.id < 0 ) {
		idEntity* ent = gameLocal.entities[ trace.c.entityNum ];
		if ( ent->GetAnimator() != NULL ) {
			sdProgram::ReturnString( ent->GetAnimator()->GetJointName( CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id ) ) );
			return;
		}
	}
	sdProgram::ReturnString( "" );
}

/*
================
sdSysCallThread::Event_GetTraceBody
================
*/
void sdSysCallThread::Event_GetTraceBody( void ) {
	if ( trace.fraction < 1.0f && trace.c.id < 0 ) {
		idAFEntity_Base *af = static_cast<idAFEntity_Base *>( gameLocal.entities[ trace.c.entityNum ] );
		if ( af && af->IsType( idAFEntity_Base::Type ) && af->IsActiveAF() ) {
			int bodyId = af->BodyForClipModelId( trace.c.id );
			idAFBody *body = af->GetAFPhysics()->GetBody( bodyId );
			if ( body ) {
				sdProgram::ReturnString( body->GetName() );
				return;
			}
		}
	}
	sdProgram::ReturnString( "" );
}

/*
================
sdSysCallThread::Event_GetTraceSurfaceFlags
================
*/
void sdSysCallThread::Event_GetTraceSurfaceFlags( void ) {
	if ( trace.fraction < 1.0f && trace.c.material != NULL ) {
		sdProgram::ReturnInteger( trace.c.material->GetSurfaceFlags() );
		return;
	}
	sdProgram::ReturnInteger( 0 );
}

/*
================
sdSysCallThread::Event_GetTraceSurfaceType
================
*/
void sdSysCallThread::Event_GetTraceSurfaceType( void ) {
	if ( trace.fraction < 1.0f && trace.c.surfaceType ) {
		sdProgram::ReturnString( trace.c.surfaceType->GetName() );
	} else {
		sdProgram::ReturnString( "" );
	}
}

/*
================
sdSysCallThread::Event_GetTraceSurfaceColor
================
*/
void sdSysCallThread::Event_GetTraceSurfaceColor( void ) {
	sdProgram::ReturnVector( trace.c.surfaceColor );
}


/*
================
sdSysCallThread::Event_SetShaderParm
================
*/
void sdSysCallThread::Event_SetShaderParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_GLOBAL_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	gameLocal.globalShaderParms[ parmnum ] = value;
}

/*
================
sdSysCallThread::Event_StartMusic
================
*/
void sdSysCallThread::Event_StartMusic( const char* shader ) {
	gameSoundWorld->PlayShaderDirectly( gameLocal.declSoundShaderType[ shader ], SND_MUSIC );
}

/*
================
sdSysCallThread::Event_StartSoundDirect
================
*/
void sdSysCallThread::Event_StartSoundDirect( const char* shader, int channel ) {
	gameSoundWorld->PlayShaderDirectly( gameLocal.declSoundShaderType[ shader ], channel );
}

/*
================
sdSysCallThread::Event_Warning
================
*/
void sdSysCallThread::Event_Warning( const char *text ) {
	Warning( "%s", text );
}

/*
================
sdSysCallThread::Event_Error
================
*/
void sdSysCallThread::Event_Error( const char *text ) {
	gameLocal.Error( "%s", text );
}

/*
================
sdSysCallThread::Event_StrLen
================
*/
void sdSysCallThread::Event_StrLen( const char *string ) {
	sdProgram::ReturnInteger( idStr::Length( string ) );
}

/*
================
sdSysCallThread::Event_StrLeft
================
*/
void sdSysCallThread::Event_StrLeft( const char *string, int num ) {
	int len;

	if ( num < 0 ) {
		sdProgram::ReturnString( "" );
		return;
	}

	len = idStr::Length( string );
	if ( len < num ) {
		sdProgram::ReturnString( string );
		return;
	}

	idStr result( string, 0, num );
	sdProgram::ReturnString( result );
}

/*
================
sdSysCallThread::Event_StrRight 
================
*/
void sdSysCallThread::Event_StrRight( const char *string, int num ) {
	int len;

	if ( num < 0 ) {
		sdProgram::ReturnString( "" );
		return;
	}

	len = idStr::Length( string );
	if ( len < num ) {
		sdProgram::ReturnString( string );
		return;
	}

	sdProgram::ReturnString( string + len - num );
}

/*
================
sdSysCallThread::Event_StrSkip
================
*/
void sdSysCallThread::Event_StrSkip( const char *string, int num ) {
	int len;

	if ( num < 0 ) {
		sdProgram::ReturnString( string );
		return;
	}

	len = idStr::Length( string );
	if ( len < num ) {
		sdProgram::ReturnString( "" );
		return;
	}

	sdProgram::ReturnString( string + num );
}

/*
================
sdSysCallThread::Event_StrMid
================
*/
void sdSysCallThread::Event_StrMid( const char *string, int start, int num ) {
	int len;

	if ( num < 0 ) {
		sdProgram::ReturnString( "" );
		return;
	}

	if ( start < 0 ) {
		start = 0;
	}
	len = idStr::Length( string );
	if ( start > len ) {
		start = len;
	}

	if ( start + num > len ) {
		num = len - start;
	}

	idStr result( string, start, start + num );
	sdProgram::ReturnString( result );
}

/*
================
sdSysCallThread::Event_StrToFloat( const char *string )
================
*/
void sdSysCallThread::Event_StrToFloat( const char *string ) {
	sdProgram::ReturnFloat( static_cast< float >( atof( string ) ) );
}

/*
================
sdSysCallThread::Event_IsClient
================
*/
void sdSysCallThread::Event_IsClient( void ) { 
	sdProgram::ReturnBoolean( gameLocal.isClient );
}

/*
================
sdSysCallThread::Event_IsServer
================
*/
void sdSysCallThread::Event_IsServer( void ) { 
	sdProgram::ReturnBoolean( gameLocal.isServer );
}

/*
================
sdSysCallThread::Event_DoClientSideStuff
================
*/
void sdSysCallThread::Event_DoClientSideStuff( void ) { 
	sdProgram::ReturnBoolean( gameLocal.DoClientSideStuff() );
}

/*
================
sdSysCallThread::Event_IsNewFrame
================
*/
void sdSysCallThread::Event_IsNewFrame( void ) {
	sdProgram::ReturnBoolean( gameLocal.isNewFrame );
}

/*
================
sdSysCallThread::Event_GetFrameTime
================
*/
void sdSysCallThread::Event_GetFrameTime( void ) { 
	sdProgram::ReturnFloat( MS2SEC( gameLocal.msec ) );
}

/*
================
sdSysCallThread::Event_GetTicsPerSecond
================
*/
void sdSysCallThread::Event_GetTicsPerSecond( void ) { 
	sdProgram::ReturnFloat( static_cast< float >( USERCMD_HZ ) );
}

/*
================
sdSysCallThread::Event_BroadcastToolTip
================
*/
void sdSysCallThread::Event_BroadcastToolTip( int index, idEntity* other, const wchar_t* string1, const wchar_t* string2, const wchar_t* string3, const wchar_t* string4 ) {
	if ( gameLocal.isClient ) {
		return;
	}

	idPlayer* otherPlayer = other->Cast< idPlayer >();
	if ( otherPlayer == NULL ) {
		gameLocal.Error( "sdSysCallThread::Event_BroadcastToolTip Supplied Entity is not a Player" );
	}

	const wchar_t* strings[ 4 ];

	strings[ 0 ] = string1;
	strings[ 1 ] = string2;
	strings[ 2 ] = string3;
	strings[ 3 ] = string4;

	int count = 0;
	for ( count = 0; count < 4; count++ ) {
		if ( !*strings[ count ] ) {
			break;
		}
	}

	const sdDeclToolTip* toolTip = gameLocal.declToolTipType.SafeIndex( index );
	if ( !toolTip ) {
		gameLocal.Warning( "sdSysCallThread::Event_BroadcastToolTip Invalid Tooltip" );
		return;
	}


	sdToolTipParms* parms = NULL;
	if ( count > 0 ) {
		parms = new sdToolTipParms();
		for ( int i = 0; i < count; i++ ) {
			parms->Add( strings[ i ] );
		}
	}

	otherPlayer->SendToolTip( toolTip, parms );
}

/*
================
sdSysCallThread::Event_DebugLine
================
*/
void sdSysCallThread::Event_DebugLine( const idVec3 &color, const idVec3 &start, const idVec3 &end, const float lifetime ) {
	gameRenderWorld->DebugLine( idVec4( color.x, color.y, color.z, 0.0f ), start, end, SEC2MS( lifetime ) );
}

/*
================
sdSysCallThread::Event_DebugArrow
================
*/
void sdSysCallThread::Event_DebugArrow( const idVec3 &color, const idVec3 &start, const idVec3 &end, const int size, const float lifetime ) {
	gameRenderWorld->DebugArrow( idVec4( color.x, color.y, color.z, 0.0f ), start, end, size, SEC2MS( lifetime ) );
}

/*
================
sdSysCallThread::Event_DebugCircle
================
*/
void sdSysCallThread::Event_DebugCircle( const idVec3 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const float lifetime ) {
	gameRenderWorld->DebugCircle( idVec4( color.x, color.y, color.z, 0.0f ), origin, dir, radius, numSteps, SEC2MS( lifetime ) );
}

/*
================
sdSysCallThread::Event_DebugBounds
================
*/
void sdSysCallThread::Event_DebugBounds( const idVec3 &color, const idVec3 &mins, const idVec3 &maxs, const float lifetime ) {
	gameRenderWorld->DebugBounds( idVec4( color.x, color.y, color.z, 0.0f ), idBounds( mins, maxs ), vec3_origin, mat3_identity, SEC2MS( lifetime ) );
}

/*
================
sdSysCallThread::Event_DrawText
================
*/
void sdSysCallThread::Event_DrawText( const char *text, const idVec3 &origin, float scale, const idVec3 &color, const int align, const float lifetime ) {
	gameRenderWorld->DrawText( text, origin, scale, idVec4( color.x, color.y, color.z, 0.0f ), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), align, SEC2MS( lifetime ) );
}

/*
================
sdSysCallThread::Event_GetDeclTypeHandle
================
*/
void sdSysCallThread::Event_GetDeclTypeHandle( const char* declTypeName ) {
	sdProgram::ReturnInteger( declManager->GetDeclTypeHandle( declTypeName ) );
}

/*
================
sdSysCallThread::Event_GetDeclCount
================
*/
void sdSysCallThread::Event_GetDeclCount( int declTypeHandle ) {
	if ( declTypeHandle == -1 ) {
		sdProgram::ReturnInteger( 0 );
		return;
	}
	
	sdProgram::ReturnInteger( declManager->GetNumDecls( declTypeHandle ) );
}

/*
================
sdSysCallThread::Event_GetDeclIndex
================
*/
void sdSysCallThread::Event_GetDeclIndex( int declTypeHandle, const char* declName ) {
	if ( declTypeHandle == -1 ) {
		sdProgram::ReturnInteger( -1 );
		return;
	}

	const idDecl* decl = declManager->FindType( declTypeHandle, declName, false );
	sdProgram::ReturnInteger( decl ? decl->Index() : -1 );
}

/*
================
sdSysCallThread::Event_GetDeclName
================
*/
void sdSysCallThread::Event_GetDeclName( int declTypeHandle, int index ) {
	if ( declTypeHandle == -1 ) {
		sdProgram::ReturnString( "" );
		return;
	}

	const idDecl* decl = declManager->DeclByIndex( declTypeHandle, index, false );
	sdProgram::ReturnString( decl ? decl->GetName() : "" );
}

/*
================
sdSysCallThread::Event_ApplyRadiusDamage
================
*/
void sdSysCallThread::Event_ApplyRadiusDamage( const idVec3& origin, idEntity *inflictor, idEntity *attacker, idEntity *ignore, idEntity *ignorePush, int damageIndex, float damagePower, float radiusScale ) {
	const sdDeclDamage *damageDecl = gameLocal.declDamageType[ damageIndex ];
	if ( damageDecl == NULL ) {
		gameLocal.Warning( "Event_ApplyRadiusDamage: damageDecl is NULL" );
		return;
	}
	gameLocal.RadiusDamage( origin, inflictor, attacker, ignore, ignorePush, damageDecl, damagePower, radiusScale );
}

/*
================
sdSysCallThread::Event_FilterEntity
================
*/
void sdSysCallThread::Event_FilterEntity( int filterIndex, idEntity* entity ) {
	if ( filterIndex == -1 || !entity ) {
		sdProgram::ReturnBoolean( true );
		return;
	}

	const sdDeclTargetInfo* filter = gameLocal.declTargetInfoType[ filterIndex ];
	if ( !filter ) {
		sdProgram::ReturnBoolean( true );
		return;
	}

	sdProgram::ReturnBoolean( filter->FilterEntity( entity ) );
}

/*
================
sdSysCallThread::Event_GetTableCount
================
*/
void sdSysCallThread::Event_GetTableCount( int tableIndex ) {
	if ( tableIndex == -1 ) {
		sdProgram::ReturnInteger( 0 );
		return;
	}

	const idDeclTable* table = gameLocal.declTableType[ tableIndex ];
	sdProgram::ReturnInteger( table->NumValues() );
}

/*
================
sdSysCallThread::Event_GetTableValue
================
*/
void sdSysCallThread::Event_GetTableValue( int tableIndex, int valueIndex ) {
	if ( tableIndex == -1 ) {
		sdProgram::ReturnFloat( 0 );
		return;
	}

	const idDeclTable* table = gameLocal.declTableType[ tableIndex ];
	sdProgram::ReturnFloat( table->GetValue( valueIndex ) );
}

/*
================
sdSysCallThread::Event_GetTableValueExact
================
*/
void sdSysCallThread::Event_GetTableValueExact( int tableIndex, float valueIndex ) {
	if ( tableIndex == -1 ) {
		sdProgram::ReturnFloat( 0 );
		return;
	}

	const idDeclTable* table = gameLocal.declTableType[ tableIndex ];
	sdProgram::ReturnFloat( table->TableLookup( valueIndex ) );
}

/*
================
sdSysCallThread::Event_GetTypeHandle
================
*/
void sdSysCallThread::Event_GetTypeHandle( const char* typeName ) {
	idTypeInfo* type = idClass::GetClass( typeName );
	if ( !type ) {
		sdProgram::ReturnInteger( -1 );
		return;
	}
	sdProgram::ReturnInteger( type->typeNum );
}

/*
================
sdSysCallThread::Event_Argc
================
*/
void sdSysCallThread::Event_Argc( void ) {
	sdProgram::ReturnInteger( gameLocal.GetActionArgs().Argc() );
}

/*
================
sdSysCallThread::Event_Argv
================
*/
void sdSysCallThread::Event_Argv( int index ) {
	sdProgram::ReturnString( gameLocal.GetActionArgs().Argv( index ) );
}

/*
================
sdSysCallThread::Event_FloatArgv
================
*/
void sdSysCallThread::Event_FloatArgv( int index ) {
	sdProgram::ReturnFloat( static_cast< float >( atof( gameLocal.GetActionArgs().Argv( index ) ) ) );
}

/*
================
sdSysCallThread::Event_SetActionCommand
================
*/
void sdSysCallThread::Event_SetActionCommand( const char* message ) {
	gameLocal.SetActionCommand( message );
}

/*
================
sdSysCallThread::Event_GetTeam
================
*/
void sdSysCallThread::Event_GetTeam( const char* name ) {
	sdTeamInfo* team = sdTeamManager::GetInstance().GetTeamSafe( name );
	sdProgram::ReturnObject( team ? team->GetScriptObject() : NULL );
}

/*
================
sdSysCallThread::Event_PlayWorldEffect
================
*/
void sdSysCallThread::Event_PlayWorldEffect( const char *effectName, const idVec3& color, const idVec3 &org, const idVec3 &forward ) {
	gameLocal.PlayEffect( gameLocal.FindEffect( effectName )->Index(), color, org, forward.ToMat3() ); 
}

/*
================
sdSysCallThread::Event_PlayWorldEffectRotateAlign
================
*/
void sdSysCallThread::Event_PlayWorldEffectRotateAlign( const char *effectName, const idVec3& color, const idVec3 &org, const idVec3 &forward, const idAngles &rotate, idEntity *ent ) {
	idMat3 rot = rotate.ToMat3();
	idMat3 axis;

	const idMat3& inaxis = ent->GetAxis();
	axis[ 0 ] = forward;
	axis[ 1 ] = inaxis[ 1 ];
	axis[ 2 ] = inaxis[ 2 ];
	axis.OrthoNormalizeSelf();
	
	gameLocal.PlayEffect( gameLocal.FindEffect( effectName )->Index(), color, org, rot * axis );
}

/*
================
sdSysCallThread::Event_PlayWorldEffectRotate
================
*/
void sdSysCallThread::Event_PlayWorldEffectRotate( const char *effectName, const idVec3& color, const idVec3 &org, const idVec3 &forward, const idAngles &rotate ) {
	idMat3 rot = rotate.ToMat3();
	gameLocal.PlayEffect( gameLocal.FindEffect( effectName )->Index(), color, org, rot * forward.ToMat3() ); 
}

/*
================
sdSysCallThread::Event_PlayWorldBeamEffect
================
*/
void sdSysCallThread::Event_PlayWorldBeamEffect( const char *effectName, const idVec3& color, const idVec3 &org, const idVec3 &endOrigin ) {
	idVec3 dir = endOrigin - org;
	dir.NormalizeFast();
	gameLocal.PlayEffect( gameLocal.FindEffect( effectName )->Index(), color, org, dir.ToMat3(), false, endOrigin ); 
}

/*
================
sdSysCallThread::Event_GetLocalPlayer
================
*/
void sdSysCallThread::Event_GetLocalPlayer( void ) {
	sdProgram::ReturnEntity( gameLocal.GetLocalPlayer() );
}

/*
================
sdSysCallThread::Event_GetLocalViewPlayer
================
*/
void sdSysCallThread::Event_GetLocalViewPlayer( void ) {
	sdProgram::ReturnEntity( gameLocal.GetLocalViewPlayer() );
}

/*
============
sdSysCallThread::Event_SetGUIFloat
============
*/
void sdSysCallThread::Event_SetGUIFloat( int handle, const char* name, float value ) {
	gameLocal.SetGUIFloat( handle, name, value );
}

/*
============
sdSysCallThread::Event_SetGUIInt
============
*/
void sdSysCallThread::Event_SetGUIInt( int handle, const char* name, int value ) {
	gameLocal.SetGUIInt( handle, name, value );
}

/*
============
sdSysCallThread::Event_GetGUIFloat
============
*/
void sdSysCallThread::Event_GetGUIFloat( int handle, const char* name ) {
	sdProgram::ReturnFloat( gameLocal.GetGUIFloat( handle, name ) );
}

/*
============
sdSysCallThread::Event_SetGUIVec2
============
*/
void sdSysCallThread::Event_SetGUIVec2( int handle, const char* name, float x, float y ) {
	gameLocal.SetGUIVec2( handle, name, idVec2( x, y ) );
}

/*
============
sdSysCallThread::Event_SetGUIVec3
============
*/
void sdSysCallThread::Event_SetGUIVec3( int handle, const char* name, float x, float y, float z ) {
	gameLocal.SetGUIVec3( handle, name, idVec3( x, y, z ) );
}

/*
============
sdSysCallThread::Event_SetGUIVec4
============
*/
void sdSysCallThread::Event_SetGUIVec4( int handle, const char* name, float x, float y, float z, float w ) {
	gameLocal.SetGUIVec4( handle, name, idVec4( x, y, z, w ) );
}

/*
============
sdSysCallThread::Event_SetGUIString
============
*/
void sdSysCallThread::Event_SetGUIString( int handle, const char* name, const char* value ) {
	gameLocal.SetGUIString( handle, name, value );
}

/*
============
sdSysCallThread::Event_SetGUIWString
============
*/
void sdSysCallThread::Event_SetGUIWString( int handle, const char* name, const wchar_t* value ) {
	gameLocal.SetGUIWString( handle, name, value );
}

/*
============
sdSysCallThread::Event_GUIPostNamedEvent
============
*/
void sdSysCallThread::Event_GUIPostNamedEvent( int handle, const char* window, const char* name ) {
	gameLocal.GUIPostNamedEvent( handle, window, name );
}

/*
============
sdSysCallThread::Event_ClearDeployRequest
============
*/
void sdSysCallThread::Event_ClearDeployRequest( int deployIndex ) {
	gameLocal.ClearDeployRequest( deployIndex );
}

/*
============
sdSysCallThread::Event_GetDeployMask
============
*/
void sdSysCallThread::Event_GetDeployMask( const char* maskname ) {
	sdProgram::ReturnInteger( gameLocal.GetDeploymentMask( maskname ) );	
}

/*
============
sdSysCallThread::Event_CheckDeployMask
============
*/
void sdSysCallThread::Event_CheckDeployMask( const idVec3& mins, const idVec3& maxs, qhandle_t handle ) {
	idVec3 mid = ( mins + maxs ) * 0.5f;

	const sdPlayZone* playZone = gameLocal.GetPlayZone( mid, sdPlayZone::PZF_DEPLOYMENT );
	if ( !playZone ) {
		sdProgram::ReturnFloat( 0.f );
		return;
	}

	const sdDeployMaskInstance* deployMask = playZone->GetMask( handle );
	if ( deployMask == NULL ) {
		sdProgram::ReturnFloat( 0.f );
		return;
	}

	sdProgram::ReturnFloat( deployMask->IsValid( idBounds( mins, maxs ) ) == DR_CLEAR ? 1.f : 0.f );
}

/*
============
sdSysCallThread::Event_GetWorldPlayZoneIndex
============
*/
void sdSysCallThread::Event_GetWorldPlayZoneIndex( const idVec3& point ) {
	sdProgram::ReturnInteger( gameLocal.GetWorldPlayZoneIndex( point ) );
}

/*
============
sdSysCallThread::Event_AllocTargetTimer
============
*/
void sdSysCallThread::Event_AllocTargetTimer( const char* targetName ) {
	sdProgram::ReturnInteger( gameLocal.AllocTargetTimer( targetName ) );
}

/*
============
sdSysCallThread::Event_GetTargetTimerValue
============
*/
void sdSysCallThread::Event_GetTargetTimerValue( qhandle_t handle, idEntity* entity ) {
	idPlayer* player = entity->Cast< idPlayer >();
	if ( !player ) {
		sdProgram::ReturnFloat( -1.f );
		return;
	}

	sdProgram::ReturnFloat( MS2SEC( gameLocal.GetTargetTimerValue( handle, player ) ) );
}

/*
============
sdSysCallThread::Event_SetTargetTimerValue
============
*/
void sdSysCallThread::Event_SetTargetTimerValue( qhandle_t handle, idEntity* entity, float t ) {
	idPlayer* player = entity->Cast< idPlayer >();
	if ( !player ) {
		return;
	}

	gameLocal.SetTargetTimer( handle, player, SEC2MS( t ) );
}

/*
============
sdSysCallThread::Event_SetGUITheme
============
*/
void sdSysCallThread::Event_SetGUITheme( guiHandle_t handle, const char* theme ) {
	gameLocal.SetGUITheme( handle, theme );
}

/*
============
sdSysCallThread::Event_AllocCMIcon
============
*/
void sdSysCallThread::Event_AllocCMIcon( idEntity* owner, int sort ) {
	qhandle_t handle = sdCommandMapInfoManager::GetInstance().Alloc( owner, sort );
/*
	NOTE: This code comes in very handy when trying to track down cases where the player's command map icon
		  mysteriously disappears :)

	if ( (int) handle == 0 ) {
		gameLocal.Warning( "+++++++++ Alloced at %i", gameLocal.time );
	}
*/


	sdProgram::ReturnInteger( handle );
}

/*
============
sdSysCallThread::Event_FreeCMIcon
============
*/
void sdSysCallThread::Event_FreeCMIcon( idEntity* owner, qhandle_t handle ) {

/*	NOTE: This code comes in very handy when trying to track down cases where the player's command map icon
		  mysteriously disappears :)

	if ( (int) handle == 0 ) {
		gameLocal.Warning( "--------- Freed at %i", gameLocal.time );
	}
*/

	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( info == NULL ) {
		gameLocal.Warning( "sdSysCallThread::Event_FreeCMIcon Tried to Free Unknown Icon" );
		return;
	}

	if ( info->GetOwner() != owner ) {
		gameLocal.Warning( "sdSysCallThread::Event_FreeCMIcon Owner Did Not Match" );
	}

	sdCommandMapInfoManager::GetInstance().Free( handle );
}


/*
============
sdSysCallThread::Event_SetCMIconSize
============
*/
void sdSysCallThread::Event_SetCMIconSize( qhandle_t handle, float size ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetSize( size );
}

/*
============
sdSysCallThread::Event_SetCMIconShaderParm
============
*/
void sdSysCallThread::Event_SetCMIconShaderParm( qhandle_t handle, int index, float value ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetShaderParm( index, value );
}

/*
============
sdSysCallThread::Event_SetCMIconSize2d
============
*/
void sdSysCallThread::Event_SetCMIconSize2d( qhandle_t handle, float sizeX, float sizeY ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetSize( idVec2( sizeX, sizeY ) );
}

/*
============
sdSysCallThread::Event_SetCMIconUnknownSize
============
*/
void sdSysCallThread::Event_SetCMIconUnknownSize( qhandle_t handle, float size ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetUnknownSize( size );
}

/*
============
sdSysCallThread::Event_SetCMIconUnknownSize2d
============
*/
void sdSysCallThread::Event_SetCMIconUnknownSize2d( qhandle_t handle, float sizeX, float sizeY ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetUnknownSize( idVec2( sizeX, sizeY ) );
}

/*
============
sdSysCallThread::Event_SetCMIconSizeMode
============
*/
void sdSysCallThread::Event_SetCMIconSizeMode( qhandle_t handle, sdCommandMapInfo::scaleMode_t mode ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetScaleMode( mode );
}

/*
============
sdSysCallThread::Event_SetCMIconPositionMode
============
*/
void sdSysCallThread::Event_SetCMIconPositionMode( qhandle_t handle, sdCommandMapInfo::positionMode_t mode ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetPositionMode( mode );
}

/*
============
sdSysCallThread::Event_SetCMIconOrigin
============
*/
void sdSysCallThread::Event_SetCMIconOrigin( qhandle_t handle, const idVec3& origin ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetOrigin( origin.ToVec2() );
}

/*
============
sdSysCallThread::Event_GetCMIconFlags
============
*/
void sdSysCallThread::Event_GetCMIconFlags( qhandle_t handle ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		gameLocal.Warning( "sdSysCallThread::Event_GetCMIconFlags: Unknown command map icon" );
		sdProgram::ReturnInteger( -1 );
		return;
	}

	sdProgram::ReturnInteger( info->GetFlags() );
}

/*
============
sdSysCallThread::Event_FlashCMIcon
============
*/
void sdSysCallThread::Event_FlashCMIcon( qhandle_t handle, int materialIndex, float seconds, int setFlags ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	const idMaterial* material = gameLocal.declMaterialType[ materialIndex ];
	info->Flash( material, SEC2MS( seconds ), setFlags );
}

/*
============
sdSysCallThread::Event_SetCMIconColor
============
*/
void sdSysCallThread::Event_SetCMIconColor( qhandle_t handle, const idVec3& color, float alpha ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetColor( color );
	info->SetAlpha( alpha );
}

/*
============
sdSysCallThread::Event_SetCMIconColorMode
============
*/
void sdSysCallThread::Event_SetCMIconColorMode( qhandle_t handle, sdCommandMapInfo::colorMode_t mode ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetColorMode( mode );
}

/*
============
sdSysCallThread::Event_SetCMIconDrawMode
============
*/
void sdSysCallThread::Event_SetCMIconDrawMode( qhandle_t handle, sdCommandMapInfo::drawMode_t mode ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetDrawMode( mode );
}

/*
============
sdSysCallThread::Event_SetCMIconAngle
============
*/
void sdSysCallThread::Event_SetCMIconAngle( qhandle_t handle, float angle ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetAngle( angle );
}

/*
============
sdSysCallThread::Event_SetCMIconSides
============
*/
void sdSysCallThread::Event_SetCMIconSides( qhandle_t handle, int sides ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetSides( sides );
}

/*
============
sdSysCallThread::Event_HideCMIcon
============
*/
void sdSysCallThread::Event_HideCMIcon( qhandle_t handle ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->Hide();
}

/*
============
sdSysCallThread::Event_ShowCMIcon
============
*/
void sdSysCallThread::Event_ShowCMIcon( qhandle_t handle ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->Show();
}

/*
============
sdSysCallThread::Event_AddCMIconRequirement
============
*/
void sdSysCallThread::Event_AddCMIconRequirement( qhandle_t handle, const char* requirement ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info || idStr::Length( requirement ) == 0 ) {
		return;
	}

	info->GetRequirements().Load( requirement );
}

/*
============
sdSysCallThread::Event_SetCMIconMaterial
============
*/
void sdSysCallThread::Event_SetCMIconMaterial( qhandle_t handle, int materialIndex ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( info == NULL ) {
		return;
	}

	info->SetMaterial( declHolder.FindMaterialByIndex( materialIndex ) );
}

/*
============
sdSysCallThread::Event_SetCMIconUnknownMaterial
============
*/
void sdSysCallThread::Event_SetCMIconUnknownMaterial( qhandle_t handle, int materialIndex ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetUnknownMaterial( declHolder.FindMaterialByIndex( materialIndex ) );
}

/*
============
sdSysCallThread::Event_SetCMIconFiretaemMaterial
============
*/
void sdSysCallThread::Event_SetCMIconFireteamMaterial( qhandle_t handle, int materialIndex ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	assert( info->GetOwner()->Cast<idPlayer>() != NULL );
	info->SetFireteamMaterial( declHolder.FindMaterialByIndex( materialIndex ) );
}

/*
============
sdSysCallThread::Event_SetCMIconGuiMessage
============
*/
void sdSysCallThread::Event_SetCMIconGuiMessage( qhandle_t handle, const char* message ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetGuiMessage( message );
}

/*
============
sdSysCallThread::Event_SetCMIconFlag
============
*/
void sdSysCallThread::Event_SetCMIconFlag( qhandle_t handle, int flag ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetFlag( flag );
}

/*
============
sdSysCallThread::Event_ClearCMIconFlag
============
*/
void sdSysCallThread::Event_ClearCMIconFlag( qhandle_t handle, int flag ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->ClearFlag( flag );
}

/*
============
sdSysCallThread::Event_SetCMIconArcAngle
============
*/
void sdSysCallThread::Event_SetCMIconArcAngle( qhandle_t handle, float arcAngle ) {
	sdCommandMapInfo* info = sdCommandMapInfoManager::GetInstance().GetInfo( handle );
	if ( !info ) {
		return;
	}

	info->SetArcAngle( arcAngle );
}

/*
============
sdSysCallThread::Event_GetEntityDefKey
============
*/
void sdSysCallThread::Event_GetEntityDefKey( int index, const char* key ) {
	const idDeclEntityDef* entityDef = gameLocal.declEntityDefType[ index ];
	if ( !entityDef ) {
		sdProgram::ReturnString( "" );
		return;
	}

	sdProgram::ReturnString( entityDef->dict.GetString( key ) );
}

/*
============
sdSysCallThread::Event_GetEntityDefIntKey
============
*/
void sdSysCallThread::Event_GetEntityDefIntKey( int index, const char* key ) {
	const idDeclEntityDef* entityDef = gameLocal.declEntityDefType[ index ];
	if ( !entityDef ) {
		sdProgram::ReturnInteger( 0 );
		return;
	}

	sdProgram::ReturnInteger( entityDef->dict.GetInt( key ) );
}

/*
============
sdSysCallThread::Event_GetEntityDefFloatKey
============
*/
void sdSysCallThread::Event_GetEntityDefFloatKey( int index, const char* key ) {
	const idDeclEntityDef* entityDef = gameLocal.declEntityDefType[ index ];
	if ( !entityDef ) {
		sdProgram::ReturnFloat( 0.f );
		return;
	}

	sdProgram::ReturnFloat( entityDef->dict.GetFloat( key ) );
}

/*
============
sdSysCallThread::Event_GetEntityDefVectorKey
============
*/
void sdSysCallThread::Event_GetEntityDefVectorKey( int index, const char* key ) {
	const idDeclEntityDef* entityDef = gameLocal.declEntityDefType[ index ];
	if ( !entityDef ) {
		sdProgram::ReturnVector( vec3_zero );
		return;
	}

	sdProgram::ReturnVector( entityDef->dict.GetVector( key ) );
}

/*
============
sdSysCallThread::Event_AllocDecal
============
*/
void sdSysCallThread::Event_AllocDecal( const char* material ) {
	sdProgram::ReturnInteger( gameLocal.RegisterLoggedDecal( gameLocal.declMaterialType[ material ] ) );
}

/*
============
sdSysCallThread::Event_ProjectDecal
============
*/
void sdSysCallThread::Event_ProjectDecal( qhandle_t handle, const idVec3& origin, const idVec3& direction, float depth, bool parallel, const idVec3& size, float angle, const idVec3& color ) {
	gameDecalInfo_t* info = gameLocal.GetLoggedDecal( handle );
	if( !info || !info->renderEntity.hModel ) {
		Warning( "ProjectDecal: NULL decal info" );
		return;
	}

	gameLocal.CreateProjectedDecal( origin, direction, depth, parallel, size.x, size.y, angle, idVec4( color.x, color.y, color.z, 1.0f ), info->renderEntity.hModel );
}

/*
============
sdSysCallThread::Event_FreeDecal
============
*/
void sdSysCallThread::Event_FreeDecal( qhandle_t handle ) {
	gameLocal.FreeLoggedDecal( handle );
}

/*
============
sdSysCallThread::Event_ResetDecal
============
*/
void sdSysCallThread::Event_ResetDecal( qhandle_t handle ) {
	gameLocal.ResetLoggedDecal( handle );
}

/*
===============
sdSysCallThread::Event_AllocHudModule
===============
*/
void sdSysCallThread::Event_AllocHudModule( const char* name, int sort, bool allowInhibit ) {
	sdProgram::ReturnInteger( gameLocal.localPlayerProperties.AllocHudModule( name, sort, allowInhibit ) );
}

/*
===============
sdSysCallThread::Event_FreeHudModule
===============
*/
void sdSysCallThread::Event_FreeHudModule( int handle ) {
	gameLocal.localPlayerProperties.FreeHudModule( guiHandle_t( handle ) );
}

/*
===============
sdSysCallThread::Event_RequestDeployment
===============
*/
void sdSysCallThread::Event_RequestDeployment( idEntity* other, int deploymentObjectIndex, const idVec3& position, float yaw, float extraDelay ) {
	const sdDeclDeployableObject* object = gameLocal.declDeployableObjectType.SafeIndex( deploymentObjectIndex );
	if ( !object ) {
		gameLocal.Warning( "sdSysCallThread::Event_RequestDeployment Deployment Object Index OOB" );
		sdProgram::ReturnBoolean( false );
		return;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( !player ) {
		gameLocal.Warning( "sdSysCallThread::Event_RequestDeployment Entity Is Not a Player" );
		sdProgram::ReturnBoolean( true );
		return;
	}

	sdProgram::ReturnBoolean( gameLocal.RequestDeployment( player, object, position, yaw, SEC2MS( extraDelay ) ) );
}

/*
===============
sdSysCallThread::Event_RequestCheckedDeployment
===============
*/
void sdSysCallThread::Event_RequestCheckedDeployment( idEntity* other, int deploymentObjectIndex, float yaw, float extraDelay ) {
	const sdDeclDeployableObject* object = gameLocal.declDeployableObjectType.SafeIndex( deploymentObjectIndex );

	if ( !object ) {
		gameLocal.Warning( "sdSysCallThread::Event_CheckDeployment Deployment Object Index OOB" );
		sdProgram::ReturnBoolean( false );
		return;
	}

	idPlayer* player = other->Cast< idPlayer >();
	if ( !player ) {
		gameLocal.Warning( "sdSysCallThread::Event_CheckDeployment Entity Is Not a Player" );
		sdProgram::ReturnBoolean( false );
		return;
	}

	sdProgram::ReturnBoolean( player->ServerDeploy( object, yaw, SEC2MS( extraDelay ) ) );
}


/*
===============
sdSysCallThread::Event_GetWorldMins
===============
*/
void sdSysCallThread::Event_GetWorldMins( void ) {
	sdProgram::ReturnVector( gameLocal.clip.GetWorldBounds().GetMins() );
}

/*
===============
sdSysCallThread::Event_GetWorldMaxs
===============
*/
void sdSysCallThread::Event_GetWorldMaxs( void ) {
	sdProgram::ReturnVector( gameLocal.clip.GetWorldBounds().GetMaxs() );
}

/*
===============
sdSysCallThread::Event_SetDeploymentObject
===============
*/
void sdSysCallThread::Event_SetDeploymentObject( int index ) {
	sdDeployMenu* menu = gameLocal.localPlayerProperties.GetDeployMenu();
	if ( !menu ) {
		return;
	}

	const sdDeclDeployableObject* object = gameLocal.declDeployableObjectType.SafeIndex( index );
	if ( !object ) {
		return;
	}

	menu->SetObject( object );
}

/*
===============
sdSysCallThread::Event_SetDeploymentState
===============
*/
void sdSysCallThread::Event_SetDeploymentState( deployResult_t state ) {
	sdDeployMenu* menu = gameLocal.localPlayerProperties.GetDeployMenu();
	if ( !menu ) {
		return;
	}

	menu->SetState( state );
}

/*
===============
sdSysCallThread::Event_SetDeploymentMode
===============
*/
void sdSysCallThread::Event_SetDeploymentMode( bool mode ) {
	sdDeployMenu* menu = gameLocal.localPlayerProperties.GetDeployMenu();
	if ( !menu ) {
		return;
	}

	menu->SetDeployMode( mode );
}

/*
===============
sdSysCallThread::Event_GetDeploymentMode
===============
*/
void sdSysCallThread::Event_GetDeploymentMode( void ) {
	sdDeployMenu* menu = gameLocal.localPlayerProperties.GetDeployMenu();
	if ( !menu ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	sdProgram::ReturnBoolean( menu->GetDeployMode() );
}

/*
===============
sdSysCallThread::Event_GetDeploymentRotation
===============
*/
void sdSysCallThread::Event_GetDeploymentRotation( void ) {
	sdDeployMenu* menu = gameLocal.localPlayerProperties.GetDeployMenu();
	if ( !menu ) {
		sdProgram::ReturnFloat( 0.f );
		return;
	}

	sdProgram::ReturnFloat( menu->GetRotation() );
}

/*
===============
sdSysCallThread::Event_AllowDeploymentRotation
===============
*/
void sdSysCallThread::Event_AllowDeploymentRotation( void ) {
	sdDeployMenu* menu = gameLocal.localPlayerProperties.GetDeployMenu();
	if ( !menu ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	sdProgram::ReturnBoolean( menu->AllowRotation() );
}

/*
===============
sdSysCallThread::Event_GetDefaultFov
===============
*/
void sdSysCallThread::Event_GetDefaultFov( void ) {
	sdProgram::ReturnFloat( idPlayer::DefaultFov() );
}

/*
===============
sdSysCallThread::Event_GetTerritory
===============
*/
void sdSysCallThread::Event_GetTerritory( const idVec3& point, idScriptObject* object, bool requireTeam, bool requireActive ) {
	sdTeamInfo* team = object ? object->GetClass()->Cast< sdTeamInfo >() : NULL;
	sdProgram::ReturnEntity( gameLocal.TerritoryForPoint( point, team, requireTeam, requireActive ) );
}

/*
===============
sdSysCallThread::Event_GetMaxClients
===============
*/
void sdSysCallThread::Event_GetMaxClients( void ) {
	sdProgram::ReturnInteger( MAX_CLIENTS );
}

/*
===============
sdSysCallThread::Event_GetClient
===============
*/
void sdSysCallThread::Event_GetClient( int index ) {
	if ( index < 0 || index > MAX_CLIENTS ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}
	sdProgram::ReturnEntity( gameLocal.GetClient( index ) );
}

/*
============
sdSysCallThread::Event_HandleToString
============
*/
void sdSysCallThread::Event_HandleToString( const int handle ) {
	sdProgram::ReturnString( va( "%x", handle ) );
}

/*
============
sdSysCallThread::Event_StringToHandle
============
*/
void sdSysCallThread::Event_StringToHandle( const char* str ) {
	sdProgram::ReturnHandle( ::strtoul( str, NULL, 16 ) );
}

/*
============
sdSysCallThread::Event_ToWideString
============
*/
void sdSysCallThread::Event_ToWideString( const char* str ) {
	sdProgram::ReturnWString( va( L"%hs", str ) );
}

/*
============
sdSysCallThread::Event_PushLocalizationString
============
*/
void sdSysCallThread::Event_PushLocalizationString( const wchar_t* str ) {
	localizationStrings.Append( str );
}

/*
============
sdSysCallThread::Event_PushLocalizationStringIndex
============
*/
void sdSysCallThread::Event_PushLocalizationStringIndex( const int index ) {
	localizationStrings.Append( common->LocalizeText( declHolder.FindLocStrByIndex( index ) ) );
}

/*
============
sdSysCallThread::Event_LocalizeStringIndexArgs
============
*/
void sdSysCallThread::Event_LocalizeStringIndexArgs( const int index ) {
	const sdDeclLocStr* locStr = declHolder.declLocStrType.SafeIndex( index );

	idWStr formatted = common->LocalizeText( locStr, localizationStrings );
	
	localizationStrings.Clear();

	sdProgram::ReturnWString( formatted.c_str() );
}

/*
============
sdSysCallThread::Event_LocalizeStringArgs
============
*/
void sdSysCallThread::Event_LocalizeStringArgs( const char* str ) {	
	idWStr formatted = common->LocalizeText( str, localizationStrings );
	
	localizationStrings.Clear();

	sdProgram::ReturnWString( formatted.c_str() );
}

/*
============
sdSysCallThread::Event_LocalizeString
============
*/
void sdSysCallThread::Event_LocalizeString( const char* str ) {	
	if( str[ 0 ] == '\0' ) {
		Warning( "Tried to localize an empty string" );
		sdProgram::ReturnHandle( declHolder.FindLocStr( "_default" )->Index() );
		return;
	}
	sdProgram::ReturnHandle( declHolder.FindLocStr( str )->Index() );
}

/*
================
sdSysCallThread::Event_GetMatchTimeRemaining
================
*/
void sdSysCallThread::Event_GetMatchTimeRemaining( void ) { 
	sdProgram::ReturnFloat( MS2SEC( gameLocal.rules->GetGameTime() ) );
}

/*
================
sdSysCallThread::Event_GetMatchState
================
*/
void sdSysCallThread::Event_GetMatchState( void ) {
	sdProgram::ReturnInteger( gameLocal.rules->GetState() );
}

/*
================
sdSysCallThread::Event_CreateMaskEditSession
================
*/
void sdSysCallThread::Event_CreateMaskEditSession( void ) {
	sdDeployMaskEditSession* edit = new sdDeployMaskEditSession();
	sdProgram::ReturnObject( edit->GetScriptObject() );
}

/*
================
sdSysCallThread::Event_AllocStatInt
================
*/
void sdSysCallThread::Event_AllocStatInt( const char* name ) {
	sdProgram::ReturnHandle( sdGlobalStatsTracker::GetInstance().AllocStat( name, sdNetStatKeyValue::SVT_INT ) );
}

/*
================
sdSysCallThread::Event_GetStat
================
*/
void sdSysCallThread::Event_GetStat( const char* name ) {
	sdProgram::ReturnHandle( sdGlobalStatsTracker::GetInstance().GetStat( name ) );
}

/*
================
sdSysCallThread::Event_AllocStatFloat
================
*/
void sdSysCallThread::Event_AllocStatFloat( const char* name ) {
	sdProgram::ReturnHandle( sdGlobalStatsTracker::GetInstance().AllocStat( name, sdNetStatKeyValue::SVT_FLOAT ) );
}

/*
================
sdSysCallThread::Event_IncreaseStatInt
================
*/
void sdSysCallThread::Event_IncreaseStatInt( int handle, int playerIndex, int count ) {
	sdPlayerStatEntry* entry = sdGlobalStatsTracker::GetInstance().GetStat( handle );
	if ( !entry ) {
		gameLocal.Warning( "sdSysCallThread::Event_IncreaseStatInt Invalid Handle '%i'", handle );
		return;
	}
	if ( playerIndex < 0 || playerIndex >= MAX_CLIENTS ) {
		gameLocal.Warning( "sdSysCallThread::Event_IncreaseStatInt Invalid Player Index '%i'", playerIndex );
		return;
	}
	entry->IncreaseValue( playerIndex, count );
}

/*
================
sdSysCallThread::Event_IncreaseStatFloat
================
*/
void sdSysCallThread::Event_IncreaseStatFloat( int handle, int playerIndex, float count ) {
	sdPlayerStatEntry* entry = sdGlobalStatsTracker::GetInstance().GetStat( handle );
	if ( !entry ) {
		gameLocal.Warning( "sdSysCallThread::Event_IncreaseStatInt Invalid Handle '%i'", handle );
		return;
	}
	if ( playerIndex < 0 || playerIndex >= MAX_CLIENTS ) {
		gameLocal.Warning( "sdSysCallThread::Event_IncreaseStatInt Invalid Player Index '%i'", playerIndex );
		return;
	}
	entry->IncreaseValue( playerIndex, count );
}

/*
================
sdSysCallThread::Event_GetStatValue
================
*/
void sdSysCallThread::Event_GetStatValue( int handle, int playerIndex ) {
	sdPlayerStatEntry* entry = sdGlobalStatsTracker::GetInstance().GetStat( handle );
	if ( entry == NULL ) {
		gameLocal.Warning( "sdSysCallThread::Event_GetStatValue Invalid Handle '%i'", handle );
		return;
	}
	if ( playerIndex < 0 || playerIndex >= MAX_CLIENTS ) {
		gameLocal.Warning( "sdSysCallThread::Event_GetStatValue Invalid Player Index '%i'", playerIndex );
		return;
	}
	sdPlayerStatEntry::statValue_t value = entry->GetValue( playerIndex );
	switch ( entry->GetType() ) {
		case sdNetStatKeyValue::SVT_INT:
		case sdNetStatKeyValue::SVT_INT_MAX:
			sdProgram::ReturnFloat( value.GetInt() );
			break;
		case sdNetStatKeyValue::SVT_FLOAT:
		case sdNetStatKeyValue::SVT_FLOAT_MAX:
			sdProgram::ReturnFloat( value.GetFloat() );
			break;
	}
}

/*
================
sdSysCallThread::Event_GetStatDelta
================
*/
void sdSysCallThread::Event_GetStatDelta( int handle, int playerIndex ) {
	sdPlayerStatEntry* entry = sdGlobalStatsTracker::GetInstance().GetStat( handle );
	if ( entry == NULL ) {
		gameLocal.Warning( "sdSysCallThread::Event_GetStatDelta Invalid Handle '%i'", handle );
		return;
	}
	if ( playerIndex < 0 || playerIndex >= MAX_CLIENTS ) {
		gameLocal.Warning( "sdSysCallThread::Event_GetStatDelta Invalid Player Index '%i'", playerIndex );
		return;
	}

	sdPlayerStatEntry::statValue_t value = entry->GetDeltaValue( playerIndex );
	switch ( entry->GetType() ) {
		case sdNetStatKeyValue::SVT_INT:
		case sdNetStatKeyValue::SVT_INT_MAX:
			sdProgram::ReturnFloat( value.GetInt() );
			break;
		case sdNetStatKeyValue::SVT_FLOAT:
		case sdNetStatKeyValue::SVT_FLOAT_MAX:
			sdProgram::ReturnFloat( value.GetFloat() );
			break;
	}
}

/*
================
sdSysCallThread::Event_SetStatBaseLine
================
*/
void sdSysCallThread::Event_SetStatBaseLine( int playerIndex ) {
	sdGlobalStatsTracker::GetInstance().SetStatBaseLine( playerIndex );
}

/*
================
sdSysCallThread::Event_GetClimateSkin
================
*/
void sdSysCallThread::Event_GetClimateSkin( const char* key ) {
	if ( gameLocal.mapSkinPool == NULL ) {
		sdProgram::ReturnString( "" );
		return;
	}
	sdProgram::ReturnString( gameLocal.mapSkinPool->GetDict().GetString( key ) );
}

/*
===============
sdSysCallThread::Event_SendQuickChat
===============
*/
void sdSysCallThread::Event_SendQuickChat( idEntity* sender, int quickChatIndex, idEntity* recipient ) {
	if ( gameLocal.isClient ) {
		return;
	}

	idPlayer* sendPlayer = NULL;
	if ( sender != NULL ) {
		sendPlayer = sender->Cast< idPlayer >();
		if ( sendPlayer == NULL ) {
			gameLocal.Warning( "idPlayer::Event_SendQuickChat sender is not a Player" );
			return;
		}
	}

	idPlayer* recipientPlayer = NULL;
	if ( recipient != NULL ) {
		recipientPlayer = recipient->Cast< idPlayer >();
		if ( recipientPlayer == NULL ) {
			gameLocal.Warning( "idPlayer::Event_SendQuickChat recipient is not a Player" );
			return;
		}
	}

	gameLocal.ServerSendQuickChatMessage( sendPlayer, gameLocal.declQuickChatType.SafeIndex( quickChatIndex ), recipientPlayer );
}

/*
============
sdSysCallThread::Event_AddNotifyIcon
============
*/
void sdSysCallThread::Event_AddNotifyIcon( guiHandle_t handle, const char* window, const char* material ) {
	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
	if( !ui ) {
		Warning( "Event_AddNotifyIcon: could not find GUI" );
		return;
	}
	
	sdUserInterfaceScope* scope = NULL;
	
	sdUIObject* object = ui->GetWindow( window );
	if( object != NULL ) {
		scope = &object->GetScope();
	} else {
		Warning( "Event_AddNotifyIcon: could not find window '%s'", window );
		return;
	}
	assert( scope != NULL );
	sdUIFunctionInstance* instance = scope->GetFunction( "addIcon" );
	if( instance == NULL ) {
		Warning( "Event_AddNotifyIcon: could not find 'addIcon' event on '%s'", window );
		return;
	}

	sdUIFunctionStack stack;
	stack.Push( material );
	instance->Run( stack );

	int newHandle;
	stack.Pop( newHandle );
	sdProgram::ReturnInteger( newHandle );	
}


/*
============
sdSysCallThread::Event_RemoveNotifyIcon
============
*/
void sdSysCallThread::Event_RemoveNotifyIcon( guiHandle_t handle, const char* window, int iconHandle ) {
	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
	if( !ui ) {
		Warning( "Event_RemoveNotifyIcon: could not find GUI" );
		return;
	}

	sdUserInterfaceScope* scope = NULL;

	sdUIObject* object = ui->GetWindow( window );
	if( object != NULL ) {
		scope = &object->GetScope();
	} else {
		Warning( "Event_RemoveNotifyIcon: could not find window '%s'", window );
		return;
	}
	assert( scope != NULL );
	sdUIFunctionInstance* instance = scope->GetFunction( "removeIcon" );
	if( instance == NULL ) {
		Warning( "Event_RemoveNotifyIcon: could not find 'removeIcon' event on '%s'", window );
		return;
	}

	sdUIFunctionStack stack;
	stack.Push( iconHandle );
	instance->Run( stack );
}


/*
============
sdSysCallThread::Event_BumpNotifyIcon
============
*/
void sdSysCallThread::Event_BumpNotifyIcon(guiHandle_t handle, const char* window, int iconHandle ) {
	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
	if( !ui ) {
		Warning( "Event_BumpNotifyIcon: could not find GUI" );
		return;
	}

	sdUserInterfaceScope* scope = NULL;

	sdUIObject* object = ui->GetWindow( window );
	if( object != NULL ) {
		scope = &object->GetScope();
	} else {
		Warning( "Event_BumpNotifyIcon: could not find window '%s'", window );
		return;
	}
	assert( scope != NULL );
	sdUIFunctionInstance* instance = scope->GetFunction( "bumpIcon" );
	if( instance == NULL ) {
		Warning( "Event_BumpNotifyIcon: could not find 'removeIcon' event on '%s'", window );
		return;
	}

	sdUIFunctionStack stack;
	stack.Push( iconHandle );
	instance->Run( stack );
}

/*
============
sdSysCallThread::Event_GetContextEntity
============
*/
void sdSysCallThread::Event_GetContextEntity() {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}
	sdProgram::ReturnEntity( gameLocal.localPlayerProperties.GetContextEntity() );
}


/*
============
sdSysCallThread::Event_SetEndGameStatValue
============
*/
void sdSysCallThread::Event_SetEndGameStatValue( int statIndex, idEntity* ent, float value ) {
	idPlayer* player = ent->Cast< idPlayer >();
	if ( player == NULL ) {
		gameLocal.Warning( "sdSysCallThread::Event_SetEndGameStatValue Invalid Player" );
		return;
	}

	gameLocal.SetEndGameStatValue( statIndex, player, value );
}

/*
============
sdSysCallThread::Event_SetEndGameStatWinner
============
*/
void sdSysCallThread::Event_SetEndGameStatWinner( int statIndex, idEntity* ent ) {
	if ( ent == NULL ) {
		return;
	}
	idPlayer* player = ent->Cast< idPlayer >();
	if ( player == NULL ) {
		gameLocal.Warning( "sdSysCallThread::Event_SetEndGameStatWinner Invalid Player" );
		return;
	}

	gameLocal.SetEndGameStatWinner( statIndex, player );
}

/*
============
sdSysCallThread::Event_AllocEndGameStat
============
*/
void sdSysCallThread::Event_AllocEndGameStat( void ) {
	sdProgram::ReturnInteger( gameLocal.AllocEndGameStat() );
}

/*
============
sdSysCallThread::Event_SendEndGameStats
============
*/
void sdSysCallThread::Event_SendEndGameStats( void ) {
	gameLocal.SendEndGameStats( sdReliableMessageClientInfoAll() );
}

/*
============
sdSysCallThread::Event_HeightMapTrace
============
*/
void sdSysCallThread::Event_HeightMapTrace( const idVec3& start, const idVec3& end ) {
	sdProgram::ReturnFloat( 1.0f );

	const sdPlayZone* playZoneHeight = gameLocal.GetPlayZone( start, sdPlayZone::PZF_HEIGHTMAP );
	if ( playZoneHeight == NULL ) {
		return;
	}
	const sdHeightMapInstance& heightMap = playZoneHeight->GetHeightMap();
	if ( !heightMap.IsValid() ) {
		return;
	}

	idVec3 tempVec;
	sdProgram::ReturnFloat( heightMap.TracePoint( start, end, tempVec ) );
}

#include "../botai/BotThreadData.h"

/*
============
sdSysCallThread::Event_EnableBotReachability
============
*/
void sdSysCallThread::Event_EnableBotReachability( const char *name, int team, bool enable ) {
	botThreadData.EnableReachability( name, team, enable );
}


/*
============
sdSysCallThread::Event_GetNextBotActionIndex
============
*/
void sdSysCallThread::Event_GetNextBotActionIndex( int startIndex, int type ) {
	for ( int i = startIndex; i < botThreadData.botActions.Num(); i++ ) {
		if ( botThreadData.botActions[ i ]->GetBaseActionType() != type ) {
			continue;
		}

		sdProgram::ReturnInteger( i );
		return;
	}

	sdProgram::ReturnInteger( -1 );
}

/*
============
sdSysCallThread::Event_GetBotActionOrigin
============
*/
void sdSysCallThread::Event_GetBotActionOrigin( int index ) {
	if ( index < 0 || index >= botThreadData.botActions.Num() ) {
		gameLocal.Error( "sdSysCallThread::Event_GetBotActionOrigin Index out of Range" );
	}

	sdProgram::ReturnVector( botThreadData.botActions[ index ]->GetActionOrigin() );
}

/*
============
sdSysCallThread::Event_GetBotActionDeployableType
============
*/
void sdSysCallThread::Event_GetBotActionDeployableType( int index ) {
	if ( index < 0 || index >= botThreadData.botActions.Num() ) {
		gameLocal.Error( "sdSysCallThread::Event_GetBotActionDeployableType Index out of Range" );
	}

	sdProgram::ReturnInteger( botThreadData.botActions[ index ]->GetDeployableType() );
}

/*
============
sdSysCallThread::Event_GetBotActionBaseGoalType
============
*/
void sdSysCallThread::Event_GetBotActionBaseGoalType( int index, idScriptObject* obj ) {
	if ( obj == NULL ) {
		gameLocal.Error( "sdSysCallThread::Event_GetBotActionBaseGoalType No Team Specified" );
	}

	sdTeamInfo* team = obj->GetClass()->Cast< sdTeamInfo >();
	if ( team == NULL ) {
		gameLocal.Error( "sdSysCallThread::Event_GetBotActionBaseGoalType Invalid Team" );
	}

	if ( index < 0 || index >= botThreadData.botActions.Num() ) {
		gameLocal.Error( "sdSysCallThread::Event_GetBotActionBaseGoalType Index out of Range" );
	}

	sdProgram::ReturnInteger( botThreadData.botActions[ index ]->GetBaseObjForTeam( team->GetBotTeam() ) );
}

/*
============
sdSysCallThread::Event_EnablePlayerHeadModels
============
*/
void sdSysCallThread::Event_EnablePlayerHeadModels( void ) {
	gameLocal.EnablePlayerHeadModels();
}

/*
============
sdSysCallThread::Event_DisablePlayerHeadModels
============
*/
void sdSysCallThread::Event_DisablePlayerHeadModels( void ) {
	gameLocal.DisablePlayerHeadModels();
}
