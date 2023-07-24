// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceLocal.h"
#include "UserInterfaceExpressions.h"
#include "UserInterfaceManagerLocal.h"
#include "UIWindow.h"

#include "../../framework/KeyInput.h"

#include "../script/Script_Helper.h"

#include "../gamesys/SysCmds.h"

#include "../Player.h"

using namespace sdProperties;

#define INIT_SCRIPT_FUNCTION( SCRIPTNAME, RETURN, PARMS, FUNCTION ) uiFunctions.Set( SCRIPTNAME, new uiFunction_t( RETURN, PARMS, &sdUserInterfaceLocal::FUNCTION ) );

/*
============
sdUserInterfaceLocal::InitFunctions
============
*/
#pragma inline_depth( 0 )
#pragma optimize( "", off )
SD_UI_PUSH_CLASS_TAG( sdUserInterfaceLocal )
void sdUserInterfaceLocal::InitFunctions( void ) {

	// Utility script functions
	SD_UI_PUSH_GROUP_TAG( "Utility Functions" )

	SD_UI_FUNC_TAG( consoleCommand, "Execute a console command, append to the command buffer." )
		SD_UI_FUNC_PARM( string, "cmd", "Console command." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "consoleCommand", 'v', "s", Script_ConsoleCommand );

	SD_UI_FUNC_TAG( consoleCommandImmediate, "Execute a console command, immediate execution of the command." )
		SD_UI_FUNC_PARM( string, "cmd", "Console command." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "consoleCommandImmediate", 'v', "s", Script_ConsoleCommandImmediate );

	SD_UI_FUNC_TAG( openURL, "Opens a URL in a browser." )
		SD_UI_FUNC_PARM( string, "url", "URL." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "openURL", 'v', "s", Script_OpenURL );

	SD_UI_FUNC_TAG( playSound, "Play a sound in the current sound world." )
		SD_UI_FUNC_PARM( string, "name", "Sound name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "playSound", 'v', "s", Script_PlaySound );

	SD_UI_FUNC_TAG( playGameSound, "Play a sound in the game sound world." )
		SD_UI_FUNC_PARM( string, "name", "Sound name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "playGameSound", 'v', "s", Script_PlayGameSound );

	SD_UI_FUNC_TAG( playGameSoundDirectly, "Play a sound in the game sound world without looking up in the GUI sound dictionary." )
		SD_UI_FUNC_PARM( string, "name", "Sound name." )
		SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "playGameSoundDirectly", 'v', "s", Script_PlayGameSoundDirectly );

	SD_UI_FUNC_TAG( playMusic, "Play a sound on the music channel." )
		SD_UI_FUNC_PARM( string, "name", "Sound name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "playMusic", 'v', "s", Script_PlayMusic );

	SD_UI_FUNC_TAG( stopMusic, "Stop any sound on the music channel." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "stopMusic", 'v', "", Script_StopMusic );

	SD_UI_FUNC_TAG( playVoice, "Play a sound on the voice channel." )
		SD_UI_FUNC_PARM( string, "name", "Sound name." )
		SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "playVoice", 'v', "s", Script_PlayVoice );

	SD_UI_FUNC_TAG( stopVoice, "Stop any sound on the voice channel." )
		SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "stopVoice", 'v', "", Script_StopVoice );

	SD_UI_FUNC_TAG( querySpeakers, "Desired number of speakers." )
		SD_UI_FUNC_PARM( float, "num", "Number of speakers." )
		SD_UI_FUNC_RETURN_PARM( float, "True if the desired number of speakers is allowed." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "querySpeakers", 'f', "f", Script_QuerySpeakers );

	SD_UI_FUNC_TAG( fadeSoundClass, "Fade a sound class to a volume in X milliseconds." )
		SD_UI_FUNC_PARM( float, "soundClass", "The sound class." )
		SD_UI_FUNC_PARM( float, "to", "Volume." )
		SD_UI_FUNC_PARM( float, "over", "Time in milliseconds." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "fadeSoundClass", 'v', "fff", Script_FadeSoundClass );

	SD_UI_FUNC_TAG( soundTest, "Manage VOIP sound tests." )
		SD_UI_FUNC_PARM( float, "command", "Command to execute" )
		SD_UI_FUNC_PARM( float, "data", "Duration of the sound test" )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "soundTest", 'f', "ff", Script_SoundTest );

	SD_UI_FUNC_TAG( cacheMaterial, "Store a material to the material cache." )
		SD_UI_FUNC_PARM( string, "alias", "Alias must be a unique name within the GUI." )
		SD_UI_FUNC_PARM( string, "material", "Material name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "cacheMaterial", 'i', "ss", Script_CacheMaterial );

	SD_UI_FUNC_TAG( activate, "Activate the GUI if it is not active. Calls onActivate events for windows." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "activate", 'v', "", Script_Activate );

	SD_UI_FUNC_TAG( deactivate, "Deactivate the GUI. Calls onDeactivate events for windows." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "deactivate", 'v', "", Script_Deactivate );

	SD_UI_FUNC_TAG( deleteFile, "Permanently deletes a file from the filesystem. Only works for base-relative paths." )
		SD_UI_FUNC_PARM( string, "filename", "Relative file path." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "deleteFile", 'v', "s", Script_DeleteFile );

	SD_UI_FUNC_TAG( voiceChat, "Enable a VOIP mode. Only works when manual VOIP control is enabled." )
		SD_UI_FUNC_PARM( float, "mode", "One of the VOIPC_ constants." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "voiceChat", 'v', "f", Script_VoiceChat );

	SD_UI_FUNC_TAG( refreshSoundDevices, "Redetect available sound devices" )
		SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "refreshSoundDevices", 'v', "", Script_RefreshSoundDevices );

	SD_UI_FUNC_TAG( sendCommand, "Send a script command, The GUI must be attached to an entity. Script event OnNetworkMessage " \
		"for the player will be called with the command data." )
		SD_UI_FUNC_PARM( string, "command", "Command data to send." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "sendCommand", 'v', "s", Script_SendCommand );

	SD_UI_FUNC_TAG( sendNetworkCommand, "Send a script command. Script event OnNetworkMessage for the player will be called with the command data." )
		SD_UI_FUNC_PARM( string, "command", "Command data to send." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "sendNetworkCommand", 'v', "s", Script_SendNetworkCommand );

	SD_UI_FUNC_TAG( postNamedEvent, "Call a named event." )
		SD_UI_FUNC_PARM( string, "name", "Event name to call." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "postNamedEvent", 'v', "s", Script_PostNamedEvent );

	SD_UI_FUNC_TAG( postNamedEventOn, "Call a named event on a window." )
		SD_UI_FUNC_PARM( string, "name", "Event name to call." )
		SD_UI_FUNC_PARM( string, "window", "Window to call event on." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "postNamedEventOn", 'v', "ss", Script_PostNamedEventOn );

	SD_UI_FUNC_TAG( scriptCall, "Call a script event on the entity this GUI is bound to. The script parameters should be pushed." \
		"on the stack using scriptPush<type>() script calls." )
		SD_UI_FUNC_PARM( string, "funcName", "Function name." )
		SD_UI_FUNC_PARM( string, "parms", "Script parameter list. Valid values are \"e\" - entity, \"s\" - string, \"f\" - float, \"p\" - player." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptCall", 'v', "ss", Script_ScriptCall );

	SD_UI_FUNC_TAG( setCookieString, "Set a key/val which will be saved in the user profile." )
		SD_UI_FUNC_PARM( string, "key", "Key name." )
		SD_UI_FUNC_PARM( string, "val", "Value name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setCookieString", 'v', "ss", Script_SetCookieString );

	SD_UI_FUNC_TAG( getCookieString, "Return value for the key." )
		SD_UI_FUNC_PARM( string, "key", "Key name." )
		SD_UI_FUNC_RETURN_PARM( string, "Value for the key." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCookieString", 's', "s", Script_GetCookieString );

	SD_UI_FUNC_TAG( setCookieInt, "Set an integer key/val which will be saved in the user profile." )
		SD_UI_FUNC_PARM( string, "key", "Key name." )
		SD_UI_FUNC_PARM( float, "value", "Float value." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setCookieInt", 'v', "sf", Script_SetCookieInt );

	SD_UI_FUNC_TAG( getCookieInt, "Return integer value for the key." )
		SD_UI_FUNC_PARM( string, "key", "Key name." )
		SD_UI_FUNC_RETURN_PARM( float, "Float value for the key." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCookieInt", 'f', "s", Script_GetCookieInt );

	SD_UI_FUNC_TAG( setShaderParm, "Sets a shader parameter value. This is for example used while drawing the command map so " \
		"different material stages can be used depending on if the command map is drawn in a circle or a rectangle" )
		SD_UI_FUNC_PARM( float, "parm", "Shader parameter." )
		SD_UI_FUNC_PARM( float, "value", "Value." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setShaderParm", 'v', "ff", Script_SetShaderParm );

	SD_UI_FUNC_TAG( getKeyBind, "Get the key for a binding." )
		SD_UI_FUNC_PARM( string, "key", "Key bind." )
		SD_UI_FUNC_PARM( string, "context", "Bind context, may be an empty string in which case the default context will be used." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Wide string key name for bind." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getKeyBind", 'w', "ss", Script_GetKeyBind );

	SD_UI_FUNC_TAG( print, "Print a string to the console. _newline or _quote should be used instead of the escape characters \\n and \\\" respectively." )
		SD_UI_FUNC_PARM( string, "str", "String to print." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "print", 'v', "s", Script_Print );

	SD_UI_FUNC_TAG( getStringForProperty, "Get string for a window property." )
		SD_UI_FUNC_PARM( string, "window", "Window name." )
		SD_UI_FUNC_PARM( string, "default", "Default value." )
		SD_UI_FUNC_RETURN_PARM( string, "String value for the property." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getStringForProperty", 's', "ss", Script_GetStringForProperty );

	SD_UI_FUNC_TAG( setPropertyFromString, "Set a property from a string." )
		SD_UI_FUNC_PARM( string, "property", "Property name." )
		SD_UI_FUNC_PARM( string, "value", "Property value." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setPropertyFromString", 'v', "ss", Script_SetPropertyFromString );

	SD_UI_FUNC_TAG( broadcastEvent, "Broadcast a named event to a window. If no window is specified post the event on the GUI." )
		SD_UI_FUNC_PARM( string, "window", "Window to broadcast event to." )
		SD_UI_FUNC_PARM( string, "event", "Event name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "broadcastEvent", 'v', "ss", Script_BroadcastEvent );

	SD_UI_FUNC_TAG( broadcastEventToChildren, "Broadcast an event only to immediate children." )
		SD_UI_FUNC_PARM( string, "parent", "Parent window name." )
		SD_UI_FUNC_PARM( string, "event", "Event name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "broadcastEventToChildren", 'v', "ss", Script_BroadcastEventToChildren );

	SD_UI_FUNC_TAG( broadcastEventToDescendants, "Broadcast an event recursively to all children." )
		SD_UI_FUNC_PARM( string, "parent", "Parent window name." )
		SD_UI_FUNC_PARM( string, "event", "Event name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "broadcastEventToDescendants", 'v', "ss", Script_BroadcastEventToDescendants );

	SD_UI_FUNC_TAG( getParentName, "Get parent name." )
		SD_UI_FUNC_PARM( string, "parent", "Window name." )
		SD_UI_FUNC_RETURN_PARM( string, "Parent name or an empty string if there is no parent." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getParentName", 's', "s", Script_GetParentName );

	// inventory/weapon info
//	INIT_SCRIPT_FUNCTION( "resupplyPlayer", 'v', "ff", Script_Resupply );

	SD_UI_FUNC_TAG( copyHandle, "Copy a handle. Should be used instead of immediate for integers." )
		SD_UI_FUNC_PARM( string, "handle", "Handle to copy." )
		SD_UI_FUNC_RETURN_PARM( handle, "Copy of handle." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "copyHandle", 'i', "i", Script_CopyHandle );

	SD_UI_FUNC_TAG( activateMenuSoundWorld, "Activates the menu sound world and pauses the game sound world or deactivates the " \
		"menu sound world and unpauses the game sound world." )
		SD_UI_FUNC_PARM( float, "enable", "Enable the menu sound world." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "activateMenuSoundWorld", 'v', "f", Script_ActivateMenuSoundWorld );

	SD_UI_FUNC_TAG( getStringMapValue, "Get value for a key for the give string map def. Used to cache crosshair material used by the player." )
		SD_UI_FUNC_PARM( string, "def", "Name of stringMap def." )
		SD_UI_FUNC_PARM( string, "key", "Name of key." )
		SD_UI_FUNC_PARM( string, "default", "Default value." )
		SD_UI_FUNC_RETURN_PARM( string, "Value of key, or \"\" if def or key isn't found." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getStringMapValue", 's', "sss", Script_GetStringMapValue );

	SD_UI_FUNC_TAG( isBackgroundLoadComplete, "Check if loading of menu models (earth/stars/strogg fleet etc.) has been loaded." )
		SD_UI_FUNC_RETURN_PARM( float, "True if all partial load media has completed its background loading." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "isBackgroundLoadComplete", 'f', "", Script_IsBackgroundLoadComplete );

	SD_UI_FUNC_TAG( copyText, "Copy the text to the clipboard." )
		SD_UI_FUNC_PARM( wstring, "text", "Copy the wide text to the clipboard." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "copyText", 'v', "w", Script_CopyText );

	SD_UI_FUNC_TAG( pasteText, "Copy the clipboard data." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Copy the clipboard data to the wide string." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "pasteText", 'w', "", Script_PasteText );

	SD_UI_FUNC_TAG( uploadLevelShot, "Upload an image to a target image. Used for map screenshots." )
		SD_UI_FUNC_PARM( string, "imageName", "Image to load." )
		SD_UI_FUNC_PARM( string, "targetImage", "Image to replace." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "uploadLevelShot", 'v', "ss", Script_UploadLevelShot );

	SD_UI_FUNC_TAG( getGameTag, "Game build name." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Game identifier." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getGameTag", 'w', "", Script_GetGameTag );

	SD_UI_FUNC_TAG( getLoadTip, "Get a load tooltip intended for the map loading screen." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Localized tooltip." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getLoadTip", 'w', "", Script_GetLoadTip );

	SD_UI_FUNC_TAG( cancelToolTip, "Set float property active to false for the toolTip window." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "cancelToolTip", 'v', "", Script_CancelToolTip );

	SD_UI_FUNC_TAG( checkCVarsAgainstCFG, "Used in game options to check the machine spec the user has chosen." )
		SD_UI_FUNC_PARM( string, "path", "Relative path." )
		SD_UI_FUNC_PARM( string, "ext", "Config extension (with .)." )
		SD_UI_FUNC_PARM( string, "base", "Base config name." )
		SD_UI_FUNC_RETURN_PARM( wstring, "True if the current cvars match the spec." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "checkCVarsAgainstCFG", 'f', "sss", Script_CheckCVarsAgainstCFG );

	SD_UI_FUNC_TAG( collapseColors, "Collapse all color codes." )
		SD_UI_FUNC_PARM( string, "strColor", "Color input." )
		SD_UI_FUNC_RETURN_PARM( string, "String with all unneccessary color codes stripped". )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "collapseColors", 's', "s", Script_CollapseColors );

	SD_UI_POP_GROUP_TAG

	// Game script functions
	SD_UI_PUSH_GROUP_TAG( "Game Functions" )

	SD_UI_FUNC_TAG( getSpectatorList, "List of spectators separated with commas. Used with the marquee window type for scrolling." )
		SD_UI_FUNC_RETURN_PARM( string, "List of spectators." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getSpectatorList", 's', "", Script_GetSpecatorList );

	SD_UI_FUNC_TAG( getTeamPlayerCount, "Get the number of players on a team." )
		SD_UI_FUNC_PARM( string, "teamName", "Team name." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of players on the team or 0 if invalid team." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getTeamPlayerCount", 'f', "s", Script_GetTeamPlayerCount );

	SD_UI_FUNC_TAG( getWeaponData, "Get weapon value given a key. Mainly used for getting model and model/joint names for player in the limbo menu." )
		SD_UI_FUNC_PARM( string, "class", "Player class." )
		SD_UI_FUNC_PARM( float, "slot", "Weapon slot." )
		SD_UI_FUNC_PARM( float, "item", "Item number." )
		SD_UI_FUNC_PARM( float, "package", "Weapon package name." )
		SD_UI_FUNC_PARM( string, "key", "Key to get value for." )
		SD_UI_FUNC_RETURN_PARM( string, "Value for the given key or an empty string if key is not found in the entity def." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getWeaponData", 's', "sfffs", Script_GetWeaponData );

	SD_UI_FUNC_TAG( getNumWeaponPackages, "Get number of weapon packages for a class." )
		SD_UI_FUNC_PARM( string, "className", "Player class." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of weapon packages." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getNumWeaponPackages", 'f', "s", Script_GetNumWeaponPackages );

	SD_UI_FUNC_TAG( getWeaponBankForName, "Get bank number for slot name." )
		SD_UI_FUNC_PARM( string, "slotName", "Slot name." )
		SD_UI_FUNC_RETURN_PARM( float, "Bank number." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getWeaponBankForName", 'f', "s", Script_GetWeaponBankForName );

	SD_UI_FUNC_TAG( updateLimboProficiency, "Set new player class and update the proficiency related limbo properties. " \
		"Used for updating limbo properties when selecting a class." )
		SD_UI_FUNC_PARM( string, "className", "Class name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "updateLimboProficiency", 'v', "s", Script_UpdateLimboProficiency );

	SD_UI_FUNC_TAG( getRoleCountForTeam, "Number of players with a specific role on a team." )
		SD_UI_FUNC_PARM( string, "team", "Team name." )
		SD_UI_FUNC_PARM( string, "role", "Role name." )
		SD_UI_FUNC_RETURN_PARM( float, "Number of players." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getRoleCountForTeam", 'f', "ss", Script_GetRoleCountForTeam );

	SD_UI_FUNC_TAG( getEquivalentClass, "Get the equivalent class on the target team to the class on the current team." )
		SD_UI_FUNC_PARM( string, "currentTeam", "Current team." )
		SD_UI_FUNC_PARM( string, "targetTeam", "Target team." )
		SD_UI_FUNC_PARM( string, "currentClass", "Current class." )
		SD_UI_FUNC_RETURN_PARM( string, "The equivalent class on the target team. Returns an empty string if not valid. " \
		"Returns the default class on the target team if current team is spectators." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getEquivalentClass", 's', "sss", Script_GetEquivalentClass );

	SD_UI_FUNC_TAG( getClassSkin, "Get skin name for class." )
		SD_UI_FUNC_PARM( string, "className", "Class name." )
		SD_UI_FUNC_RETURN_PARM( string, "Skin name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getClassSkin", 's', "s", Script_GetClassSkin );

	SD_UI_FUNC_TAG( toggleReady, "Toggle the local player to be ready during warmup." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "toggleReady", 'v', "", Script_ToggleReady );

	SD_UI_FUNC_TAG( execVote, "Execute a vote on client or listen server. The vote index then the vote key should have been " \
		"pushed on the GUI stack before calling this function." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "execVote", 'v', "", Script_ExecVote );

	SD_UI_FUNC_TAG( getCommandMapTitle, "Get the command map title for a playzone on the current map." )
		SD_UI_FUNC_PARM( float, "playzone", "Playzone index." )
		SD_UI_FUNC_RETURN_PARM( handle, "Localized text handle." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCommandMapTitle", 'i', "f", Script_GetCommandMapTitle );

	SD_UI_FUNC_TAG( getPersistentRankInfo, "Get the current rank or rank material name for the local player." )
		SD_UI_FUNC_PARM( string, "type", "Where the type is \"title\" or \"material\"." )
		SD_UI_FUNC_RETURN_PARM( string, "Returns the rank title or material name for the players current rank." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getPersistentRankInfo", 's', "s", Script_GetPersistentRankInfo );

	SD_UI_FUNC_TAG( setSpawnPoint, "Set the current spawn point for the local player. Send the name to the server if a client." )
		SD_UI_FUNC_PARM( string, "name", "Spawn point name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setSpawnPoint", 'v', "s", Script_SetSpawnPoint );

	SD_UI_FUNC_TAG( highlightSpawnPoint, "Call script event OnHighlight on the spawnpoint for the team." )
		SD_UI_FUNC_PARM( string, "name", "Spawnpoint name." )
		SD_UI_FUNC_PARM( string, "team", "Team name." )
		SD_UI_FUNC_PARM( float, "highlight", "True or false to be passed on as a parameter to OnHighlight." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "highlightSpawnPoint", 'v', "ssf", Script_HighlightSpawnPoint );

	SD_UI_FUNC_TAG( mutePlayer, "Toggle mute for a player." )
		SD_UI_FUNC_PARM( string, "playerName", "Player to mute." )
		SD_UI_FUNC_PARM( float, "mute", "Either true or false." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "mutePlayer", 'v', "sf", Script_MutePlayer );

	SD_UI_FUNC_TAG( mutePlayerQuickChat, "Toggle quickchat mute for a player." )
		SD_UI_FUNC_PARM( string, "playerName", "Player to mute." )
		SD_UI_FUNC_PARM( float, "mute", "Either true or false." )
		SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "mutePlayerQuickChat", 'v', "sf", Script_MutePlayerQuickChat );

	SD_UI_FUNC_TAG( spectateClient, "Spectate a specific client, works on client and listen server." )
		SD_UI_FUNC_PARM( float, "spectatee", "Spectatee client number." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "spectateClient", 'v', "f", Script_SpectateClient );

	SD_UI_FUNC_TAG( chatCommand, "Send a chat command." )
		SD_UI_FUNC_PARM( string, "chatCommand", "Chat command is either \"say\", \"sayteam\" or \"sayfireteam\"." )
		SD_UI_FUNC_PARM( wstring, "text", "Chat string." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "chatCommand", 'v', "sw", Script_ChatCommand );

	SD_UI_POP_GROUP_TAG

	// CVar functions
	SD_UI_PUSH_GROUP_TAG( "CVar Functions" )

	SD_UI_FUNC_TAG( setCVar, "Set a cvar." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_PARM( string, "str", "String to set." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setCVar", 'v', "ss", Script_SetCVar );

	SD_UI_FUNC_TAG( getCVar, "Get a cvar value." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_RETURN_PARM( string, "String value of the CVar." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCVar", 's', "s", Script_GetCVar );

	SD_UI_FUNC_TAG( resetCVar, "Reset a CVar, gives it the default value." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "resetCVar", 'v', "s", Script_ResetCVar );

	SD_UI_FUNC_TAG( setCVarFloat, "Set a cvar." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_PARM( float, "value", "Float value to set." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setCVarFloat", 'v', "sf", Script_SetCVarFloat );

	SD_UI_FUNC_TAG( getCVarFloat, "Get a cvar value." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_RETURN_PARM( float, "Float value of the CVar." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCVarFloat", 'f', "s", Script_GetCVarFloat );

	SD_UI_FUNC_TAG( setCVarInt, "Set a cvar." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_PARM( float, "value", "Float value will be converted to an integer." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setCVarInt", 'v', "sf", Script_SetCVarInt );

	SD_UI_FUNC_TAG( getCVarInt, "Get a cvar value." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_RETURN_PARM( float, "Integer value of the CVar." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCVarInt", 'f', "s", Script_GetCVarInt );

	SD_UI_FUNC_TAG( setCVarBool, "Set a cvar." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_PARM( float, "value", "Float value will be converted to a boolean." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "setCVarBool", 'v', "sf", Script_SetCVarBool );

	SD_UI_FUNC_TAG( getCVarBool, "Get a cvar value." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_RETURN_PARM( float, "True or false value of the CVar." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCVarBool", 'f', "s", Script_GetCVarBool );

	SD_UI_FUNC_TAG( getCVarColor, "Get a cvar value." )
		SD_UI_FUNC_PARM( string, "name", "CVar name." )
		SD_UI_FUNC_RETURN_PARM( vec4, "Vec4 value of the CVar." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getCVarColor", '4', "s", Script_GetCVarColor );

	SD_UI_FUNC_TAG( isCVarLocked, "Check if a CVar has the CVAR_GUILOCKED flag set." )
		SD_UI_FUNC_RETURN_PARM( float, "True if CVar is locked." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "isCVarLocked", 'f', "s", Script_IsCVarLocked );

	SD_UI_POP_GROUP_TAG

	// Stack functions
	SD_UI_PUSH_GROUP_TAG( "Stack Functions" )

	SD_UI_FUNC_TAG( scriptPushString, "Push a string on the stack." )
		SD_UI_FUNC_PARM( string, "str", "String to push." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptPushString", 'v', "s", Script_PushString );

	SD_UI_FUNC_TAG( scriptPushFloat, "Push a float on the stack." )
		SD_UI_FUNC_PARM( float, "value", "Float to push on the stack." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptPushFloat", 'v', "f", Script_PushFloat );

	SD_UI_FUNC_TAG( scriptPushVec2, "Push a vec2 on the stack." )
		SD_UI_FUNC_PARM( vec2, "vec", "Vec2 to push on the stack." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptPushVec2", 'v', "2", Script_PushVec2 );

	SD_UI_FUNC_TAG( scriptPushVec3, "Push a vec3 on the stack." )
		SD_UI_FUNC_PARM( vec3, "vec", "Vec3 to push on the stack." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptPushVec3", 'v', "3", Script_PushVec3 );

	SD_UI_FUNC_TAG( scriptPushVec4, "Push a vec4 on the stack." )
		SD_UI_FUNC_PARM( vec4, "vec", "Vec4 to push on the stack." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptPushVec4", 'v', "4", Script_PushVec4 );

	SD_UI_FUNC_TAG( scriptGetWStringResult, "Pop a wide string from the stack." )
		SD_UI_FUNC_RETURN_PARM( wstring, "Wide string." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptGetWStringResult", 'w', "", Script_GetWStringResult );

	SD_UI_FUNC_TAG( scriptGetStringResult, "Pop a string from the stack." )
		SD_UI_FUNC_RETURN_PARM( string, "String." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptGetStringResult", 's', "", Script_GetStringResult );

	SD_UI_FUNC_TAG( scriptGetFloatResult, "Pop a float from the stack." )
		SD_UI_FUNC_RETURN_PARM( float, "Float value." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptGetFloatResult", 'f', "", Script_GetFloatResult );

	SD_UI_FUNC_TAG( scriptGetVec2Result, "Pop a vec2 from the stack." )
		SD_UI_FUNC_RETURN_PARM( vec2, "Vec2 value." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptGetVec2Result", '2', "", Script_GetVec2Result );

	SD_UI_FUNC_TAG( scriptGetVec4Result, "Pop a vec4 from the stack." )
		SD_UI_FUNC_RETURN_PARM( vec4, "Vec4 value." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "scriptGetVec4Result", '4', "", Script_GetVec4Result );

	SD_UI_FUNC_TAG( pushGeneralString, "Push a string on the stack with the given name. Any literal stack name is valid." )
		SD_UI_FUNC_PARM( string, "stackName", "Name of the stack to use." )
		SD_UI_FUNC_PARM( string, "str", "String to push." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "pushGeneralString", 'v', "ss", Script_PushGeneralString );

	SD_UI_FUNC_TAG( popGeneralString, "Pop a string on the stack with the given name. Any literal stack name is valid." )
		SD_UI_FUNC_PARM( string, "stackName", "Name of the stack to use." )
		SD_UI_FUNC_RETURN_PARM( string, "String on the stack." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "popGeneralString", 's', "s", Script_PopGeneralString );

	SD_UI_FUNC_TAG( getGeneralString, "Get the string on the top of the stack with the given name, does not pop the stack. " \
		"Any literal stack name is valid." )
		SD_UI_FUNC_PARM( string, "stackName", "Name of the stack to use." )
		SD_UI_FUNC_RETURN_PARM( string, "String on the stack." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "getGeneralString", 's', "s", Script_GetGeneralString );

	SD_UI_FUNC_TAG( generalStringAvailable, "Check if a stack with the given name is available." )
		SD_UI_FUNC_PARM( string, "stackName", "Name of the stack to check." )
		SD_UI_FUNC_RETURN_PARM( string, "Returns true if the stack is available." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "generalStringAvailable", 'f', "s", Script_GeneralStringAvailable );

	SD_UI_FUNC_TAG( clearGeneralStrings, "Clear the stack with the given name." )
		SD_UI_FUNC_PARM( string, "stackName", "Name of the stack to clear." )
	SD_UI_END_FUNC_TAG
	INIT_SCRIPT_FUNCTION( "clearGeneralStrings", 'v', "s", Script_ClearGeneralStrings );

	SD_UI_POP_GROUP_TAG


	SD_UI_PUSH_GROUP_TAG( "GUI Flags" )

	SD_UI_ENUM_TAG( GUI_FRONTEND, "Receive time updates from the system, rather than the game." )
	sdDeclGUI::AddDefine( va( "GUI_FRONTEND %i", GUI_FRONTEND ) );

	SD_UI_ENUM_TAG( GUI_FULLSCREEN, "Allow for aspect ratio corrections. Updates SCREEN_WIDTH." )
	sdDeclGUI::AddDefine( va( "GUI_FULLSCREEN %i", GUI_FULLSCREEN ) );

	SD_UI_ENUM_TAG( GUI_SHOWCURSOR, "Draw cursor. GUI does not accept Mouse events unless this flag is set." )
	sdDeclGUI::AddDefine( va( "GUI_SHOWCURSOR %i", GUI_SHOWCURSOR ) );

	SD_UI_ENUM_TAG( GUI_TOOLTIPS, "Allow a tooltip pop-up window. Post onQueryTooltip events. For a tooltip " \
		"to pop up for a window under the cursor the window should have a wide string " \
		"text named \"toolTipText\". The GUI should have a window named toolTip. The tooltip window should have a float property called " \
		"\"active\" and a wide string property called \"tipText\" which will be set to the toolTipText text when active is true." )
	sdDeclGUI::AddDefine( va( "GUI_TOOLTIPS %i", GUI_TOOLTIPS ) );

	SD_UI_ENUM_TAG( GUI_INTERACTIVE, "For GUIs in the world. Replaces normal GUI drawing with a simple image when the player is far away." )
	sdDeclGUI::AddDefine( va( "GUI_INTERACTIVE %i", GUI_INTERACTIVE ) );

	SD_UI_ENUM_TAG( GUI_SCREENSAVER, "Draw a screen saver material instead of the cursor if the player is not interacting with the GUI." )
	sdDeclGUI::AddDefine( va( "GUI_SCREENSAVER %i", GUI_SCREENSAVER ) );

	SD_UI_ENUM_TAG( GUI_CATCH_ALL_EVENTS, "Capture all events, used to prevent keys from making it back to the game." )
	sdDeclGUI::AddDefine( va( "GUI_CATCH_ALL_EVENTS %i", GUI_CATCH_ALL_EVENTS ) );

	SD_UI_ENUM_TAG( GUI_NON_FOCUSED_MOUSE_EVENTS, "Allow windows to catch mouse events even if not focused. Used by the quick chat and context menus." )
	sdDeclGUI::AddDefine( va( "GUI_NON_FOCUSED_MOUSE_EVENTS %i", GUI_NON_FOCUSED_MOUSE_EVENTS ) );

	SD_UI_ENUM_TAG( GUI_USE_MOUSE_PITCH, "Negative m_pitch will cause mouse movement to invert as well. Used by the quick chat and context menus." )
	sdDeclGUI::AddDefine( va( "GUI_USE_MOUSE_PITCH %i", GUI_USE_MOUSE_PITCH ) );

	SD_UI_ENUM_TAG( GUI_INHIBIT_GAME_WORLD, "The game view should not be drawn if this is set." )
	sdDeclGUI::AddDefine( va( "GUI_INHIBIT_GAME_WORLD %i", GUI_INHIBIT_GAME_WORLD ) );

	SD_UI_POP_GROUP_TAG

	SD_UI_ENUM_TAG( SCREEN_WIDTH, "GUI screen width (alias for gui.screenDimensions.x)." )
	sdDeclGUI::AddDefine( "SCREEN_WIDTH ( gui.screenDimensions.x )" );

	SD_UI_ENUM_TAG( SCREEN_HEIGHT, "GUI screen width (alias for gui.screenDimensions.y)." )
	sdDeclGUI::AddDefine( "SCREEN_HEIGHT ( gui.screenDimensions.y )" );

	SD_UI_ENUM_TAG( VIRTUAL_WIDTH, "Uncorrected GUI screen width (640)." )
	sdDeclGUI::AddDefine( va( "VIRTUAL_WIDTH %i", SCREEN_WIDTH ) );

	SD_UI_ENUM_TAG( VIRTUAL_HEIGHT, "Uncorrected GUI screen height (480)." )
	sdDeclGUI::AddDefine( va( "VIRTUAL_HEIGHT %i", SCREEN_HEIGHT ) );

	SD_UI_ENUM_TAG( CENTER_X, "GUI screen center (alias for gui.screenCenter.x)." )
	sdDeclGUI::AddDefine( "CENTER_X ( gui.screenCenter.x )" );

	SD_UI_ENUM_TAG( CENTER_Y, "GUI screen center (alias for gui.screenCenter.y)." )
	sdDeclGUI::AddDefine( "CENTER_Y ( gui.screenCenter.y )" );

	SD_UI_ENUM_TAG( STC_START, "Start a VOIP sound test." )
	sdDeclGUI::AddDefine( va( "STC_START %i", STC_START ) );

	SD_UI_ENUM_TAG( STC_STATUS_RECORDING, "Query status of VOIP recording" )
	sdDeclGUI::AddDefine( va( "STC_STATUS_RECORDING %i", STC_STATUS_RECORDING ) );

	SD_UI_ENUM_TAG( STC_STATUS_PLAYBACK, "Query the status of VOIP playback." )
	sdDeclGUI::AddDefine( va( "STC_STATUS_PLAYBACK %i", STC_STATUS_PLAYBACK ) );

	SD_UI_ENUM_TAG( STC_STATUS_PERCENT, "Query the percent of the VOIP test completed." )
	sdDeclGUI::AddDefine( va( "STC_STATUS_PERCENT %i", STC_STATUS_PERCENT ) );

	SD_UI_ENUM_TAG( VOIPC_GLOBAL, "Enable global VOIP input." )
	sdDeclGUI::AddDefine( va( "VOIPC_GLOBAL %i", VOIPC_GLOBAL ) );

	SD_UI_ENUM_TAG( VOIPC_FIRETEAM, "Enable fireteam VOIP input." )
	sdDeclGUI::AddDefine( va( "VOIPC_FIRETEAM %i", VOIPC_FIRETEAM ) );

	SD_UI_ENUM_TAG( VOIPC_TEAM, "Enable team VOIP input." )
	sdDeclGUI::AddDefine( va( "VOIPC_TEAM %i", VOIPC_TEAM ) );

	SD_UI_ENUM_TAG( VOIPC_DISABLE, "Disable all VOIP input." )
	sdDeclGUI::AddDefine( va( "VOIPC_DISABLE %i", VOIPC_DISABLE ) );

	// precaching
	gameLocal.declStringMapType[ "classToProficiency" ];
}
SD_UI_POP_CLASS_TAG
#pragma optimize( "", on )
#pragma inline_depth()

/*
============
sdUserInterfaceLocal::Script_PushString
============
*/
void sdUserInterfaceLocal::Script_PushString( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );
	scriptStack.Push( str );
}

/*
============
sdUserInterfaceLocal::Script_GetKeyBind
============
*/
void sdUserInterfaceLocal::Script_GetKeyBind( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );

	idStr contextStr;
	stack.Pop( contextStr );

	sdBindContext* context = gameLocal.GetDefaultBindContext();
	if( !contextStr.IsEmpty() ) {
		context = keyInputManager->AllocBindContext( contextStr );
	}

	idWStr keyName;
	keyInputManager->KeysFromBinding( context, str, true, keyName );

	stack.Push( keyName.c_str() );
}


/*
============
sdUserInterfaceLocal::Script_Print
============
*/
void sdUserInterfaceLocal::Script_Print( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );

	gameLocal.Printf( "%s", str.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_GetCVar
============
*/
void sdUserInterfaceLocal::Script_GetCVar( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );
	stack.Push( cvarSystem->GetCVarString( str ));
}

/*
============
sdUserInterfaceLocal::Script_ResetCVar
============
*/
void sdUserInterfaceLocal::Script_ResetCVar( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "reset %s", str.c_str() ) );
}

/*
============
sdUserInterfaceLocal::Script_SetCVar
============
*/
void sdUserInterfaceLocal::Script_SetCVar( sdUIFunctionStack& stack ) {
	idStr cvar;
	idStr value;
	stack.Pop( cvar );
	stack.Pop( value );
	cvarSystem->SetCVarString( cvar, value );

	if( g_debugGUIEvents.GetInteger() > 1 ) {
		gameLocal.Printf( "Set string '%s' to '%s'\n", cvar.c_str(), value.c_str() );
	}
}

/*
============
sdUserInterfaceLocal::Script_GetCVarFloat
============
*/
void sdUserInterfaceLocal::Script_GetCVarFloat( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );
	stack.Push( cvarSystem->GetCVarFloat( str ));
}

/*
============
sdUserInterfaceLocal::Script_SetCVarFloat
============
*/
void sdUserInterfaceLocal::Script_SetCVarFloat( sdUIFunctionStack& stack ) {
	idStr cvar;
	float value;
	stack.Pop( cvar );
	stack.Pop( value );
	cvarSystem->SetCVarFloat( cvar, value );

	if( g_debugGUIEvents.GetInteger() > 1 ) {
		gameLocal.Printf( "Set float '%s' to '%f'\n", cvar.c_str(), value );
	}
}

/*
============
sdUserInterfaceLocal::Script_GetCVarInt
============
*/
void sdUserInterfaceLocal::Script_GetCVarInt( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );
	stack.Push( cvarSystem->GetCVarInteger( str ));
}

/*
============
sdUserInterfaceLocal::Script_SetCVarInt
============
*/
void sdUserInterfaceLocal::Script_SetCVarInt( sdUIFunctionStack& stack ) {
	idStr cvar;
	float value;
	stack.Pop( cvar );
	stack.Pop( value );
	cvarSystem->SetCVarInteger( cvar, idMath::Ftoi( value ) );
	if( g_debugGUIEvents.GetInteger() > 1 ) {
		gameLocal.Printf( "Set int '%s' to '%i'\n", cvar.c_str(), idMath::Ftoi( value ) );
	}
}

/*
============
sdUserInterfaceLocal::Script_GetCVarBool
============
*/
void sdUserInterfaceLocal::Script_GetCVarBool( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );
	stack.Push( cvarSystem->GetCVarBool( str ));
}

/*
============
sdUserInterfaceLocal::Script_SetCVarBool
============
*/
void sdUserInterfaceLocal::Script_SetCVarBool( sdUIFunctionStack& stack ) {
	idStr cvar;
	float value;
	stack.Pop( cvar );
	stack.Pop( value );
	cvarSystem->SetCVarBool( cvar, value != 0.0f );
	if( g_debugGUIEvents.GetInteger() > 1 ) {
		gameLocal.Printf( "Set bool '%s' to '%s'\n", cvar.c_str(), value != 0.0f ? "true" : "false" );
	}
}

/*
============
sdUserInterfaceLocal::Script_GetCVarColor
============
*/
void sdUserInterfaceLocal::Script_GetCVarColor( sdUIFunctionStack& stack ) {
	idStr str;
	stack.Pop( str );

	sdColor4 color;
	sdProperties::sdFromString( color, cvarSystem->GetCVarString( str ));
	stack.Push( color.ToVec4() );
}


/*
============
sdUserInterfaceLocal::Script_IsCVarLocked
============
*/
void sdUserInterfaceLocal::Script_IsCVarLocked( sdUIFunctionStack& stack ) {
	idStr cvarName;
	stack.Pop( cvarName );
	const idCVar* cvar = cvarSystem->Find( cvarName.c_str() );
	if( cvar == NULL ) {
		gameLocal.Error( "Script_IsCVarLocked: Could not find '%s'", cvarName.c_str() );
	}
	stack.Push( cvar->IsGuiLocked() );
}


/*
============
sdUserInterfaceLocal::Script_PushFloat
============
*/
void sdUserInterfaceLocal::Script_PushFloat( sdUIFunctionStack& stack ) {
	float f;
	stack.Pop( f );
	scriptStack.Push( f );
}
/*
============
sdUserInterfaceLocal::Script_PushVec2
============
*/
void sdUserInterfaceLocal::Script_PushVec2( sdUIFunctionStack& stack ) {
	idVec2 v;
	stack.Pop( v );
	scriptStack.Push( v );
}
/*
============
sdUserInterfaceLocal::Script_PushVec3
============
*/
void sdUserInterfaceLocal::Script_PushVec3( sdUIFunctionStack& stack ) {
	idVec3 v;
	stack.Pop( v );
	scriptStack.Push( v );
}
/*
============
sdUserInterfaceLocal::Script_PushVec4
============
*/
void sdUserInterfaceLocal::Script_PushVec4( sdUIFunctionStack& stack ) {
	idVec4 v;
	stack.Pop( v );
	scriptStack.Push( v );

}

/*
============
sdUserInterfaceLocal::Script_GetFloatResult
============
*/
void sdUserInterfaceLocal::Script_GetFloatResult( sdUIFunctionStack& stack ) {
	float f;
	scriptStack.Pop( f );
	stack.Push( f );
}

/*
============
sdUserInterfaceLocal::Script_GetVec2Result
============
*/
void sdUserInterfaceLocal::Script_GetVec2Result( sdUIFunctionStack& stack ) {
	idVec2 v;
	scriptStack.Pop( v );
	stack.Push( v );
}

/*
============
sdUserInterfaceLocal::Script_GetVec4Result
============
*/
void sdUserInterfaceLocal::Script_GetVec4Result( sdUIFunctionStack& stack ) {
	idVec4 v;
	scriptStack.Pop( v );
	stack.Push( v );
}

/*
============
sdUserInterfaceLocal::Script_GetStringResult
============
*/
void sdUserInterfaceLocal::Script_GetStringResult( sdUIFunctionStack& stack ) {
	idStr str;
	scriptStack.Pop( str );
	stack.Push( str );
}

/*
============
sdUserInterfaceLocal::Script_GetWStringResult
============
*/
void sdUserInterfaceLocal::Script_GetWStringResult( sdUIFunctionStack& stack ) {
	idWStr str;
	scriptStack.Pop( str );
	stack.Push( str.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_PostNamedEvent
============
*/
void sdUserInterfaceLocal::Script_PostNamedEvent( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );
	PostNamedEvent( name );
}

/*
============
sdUserInterfaceLocal::Script_PostNamedEventOn
============
*/
void sdUserInterfaceLocal::Script_PostNamedEventOn( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );

	idStr windowName;
	stack.Pop( windowName );

	sdUIObject* window = GetWindow( windowName.c_str() );

	if ( window != NULL ) {
		window->PostNamedEvent( name, true );
	}
}

/*
============
sdUserInterfaceLocal::Script_ScriptCall
============
*/
void sdUserInterfaceLocal::Script_ScriptCall( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );

	idStr parms;
	stack.Pop( parms );

	if( !entity ) {
		gameLocal.Warning( "ScriptCall: '%s' tried to run '%s' on a NULL entity", GetName(), name.c_str() );
		return;
	}


	sdScriptHelper helper;
	if ( helper.Init( entity->GetScriptObject(), name ) ) {
		idStr strParm;
		float floatParm;

		for( int i = 0; i < parms.Length(); i++ ) {
			switch( parms[ i ] ) {
				case 'e':
					helper.Push( entity->GetScriptObject() );
					break;
				case 's':
					scriptStack.Pop( strParm );
					helper.Push( strParm );
					break;
				case 'f':
					scriptStack.Pop( floatParm );
					helper.Push( floatParm );
					break;
				case 'p':
					helper.Push( gameLocal.GetLocalPlayer()->GetScriptObject() );
					break;
				default:
					gameLocal.Error( "ScriptCall: Unknown format option '%c'", parms[ i ] );
					break;
			}
		}
		helper.Run();
		return;
	}
}


/*
================
sdUserInterfaceLocal::Script_ConsoleCommand
================
*/
void sdUserInterfaceLocal::Script_ConsoleCommand( sdUIFunctionStack& stack ) {
	idStr command;
	stack.Pop( command );
	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, command.c_str() );
}

/*
================
sdUserInterfaceLocal::Script_ConsoleCommandImmediate
================
*/
void sdUserInterfaceLocal::Script_ConsoleCommandImmediate( sdUIFunctionStack& stack ) {
	idStr command;
	stack.Pop( command );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, command.c_str() );
}

/*
================
sdUserInterfaceLocal::Script_PlaySound
================
*/
void sdUserInterfaceLocal::Script_PlaySound( sdUIFunctionStack& stack ) {
	idStr filename;
	stack.Pop( filename );

	idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		sw->PlayShaderDirectly( declHolder.declSoundShaderType.LocalFind( GetSound( filename ) ) );
	}
}

/*
================
sdUserInterfaceLocal::Script_PlayGameSound
================
*/
void sdUserInterfaceLocal::Script_PlayGameSound( sdUIFunctionStack& stack ) {
	idStr filename;
	stack.Pop( filename );

	if ( gameSoundWorld ) {
		gameSoundWorld->PlayShaderDirectly( declHolder.declSoundShaderType.LocalFind( GetSound( filename ) ) );
	}
}

/*
================
sdUserInterfaceLocal::Script_PlayGameSoundDirectly
================
*/
void sdUserInterfaceLocal::Script_PlayGameSoundDirectly( sdUIFunctionStack& stack ) {
	idStr filename;
	stack.Pop( filename );

	if ( gameSoundWorld ) {
		gameSoundWorld->PlayShaderDirectly( declHolder.declSoundShaderType.LocalFind( filename ) );
	}
}

/*
============
sdUserInterfaceLocal::Script_QuerySpeakers
============
*/
void sdUserInterfaceLocal::Script_QuerySpeakers( sdUIFunctionStack& stack ) {
	int numSpeakers;
	stack.Pop( numSpeakers );

	bool result = soundSystem->QuerySpeakers( numSpeakers );
	stack.Push( result );
}

/*
================
sdUserInterfaceLocal::Script_PlayMusic
================
*/
void sdUserInterfaceLocal::Script_PlayMusic( sdUIFunctionStack& stack ) {
	idStr filename;
	stack.Pop( filename );

	idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		sw->PlayShaderDirectly( declHolder.declSoundShaderType.LocalFind( GetSound( filename ) ), SND_MUSIC );
	}
}

/*
============
sdUserInterfaceLocal::Script_StopMusic
============
*/
void sdUserInterfaceLocal::Script_StopMusic( sdUIFunctionStack& stack ) {
	idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		sw->PlayShaderDirectly( declHolder.declSoundShaderType.LocalFind( "silence" ), SND_MUSIC );
	}
}

/*
================
sdUserInterfaceLocal::Script_PlayVoice
================
*/
void sdUserInterfaceLocal::Script_PlayVoice( sdUIFunctionStack& stack ) {
	idStr filename;
	stack.Pop( filename );

	idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		sw->PlayShaderDirectly( declHolder.declSoundShaderType.LocalFind( GetSound( filename ) ), SND_PLAYER_VO );
	}
}

/*
============
sdUserInterfaceLocal::Script_StopVoice
============
*/
void sdUserInterfaceLocal::Script_StopVoice( sdUIFunctionStack& stack ) {
	idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		sw->PlayShaderDirectly( declHolder.declSoundShaderType.LocalFind( "silence" ), SND_PLAYER_VO );
	}
}

/*
============
sdUserInterfaceLocal::Script_FadeSoundClass
============
*/
void sdUserInterfaceLocal::Script_FadeSoundClass( sdUIFunctionStack& stack ) {
	int soundClass;
	float to;
	float over;
	stack.Pop( soundClass );
	stack.Pop( to );
	stack.Pop( over );

	idSoundWorld* sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		sw->FadeSoundClasses( soundClass, to, MS2SEC( over ) );
	}
}

/*
================
sdUserInterfaceLocal::Script_Activate
================
*/
void sdUserInterfaceLocal::Script_Activate( sdUIFunctionStack& stack ) {
	Activate();
}

/*
================
sdUserInterfaceLocal::Script_Deactivate
================
*/
void sdUserInterfaceLocal::Script_Deactivate( sdUIFunctionStack& stack ) {
	Deactivate( true );
}

/*
============
sdUserInterfaceLocal::Script_SendNetworkCommand
============
*/
void sdUserInterfaceLocal::Script_SendNetworkCommand( sdUIFunctionStack& stack ) {
	idStr value;
	stack.Pop( value );

	assert( entity );

	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_NETWORKCOMMAND );
		msg.WriteBits( gameLocal.GetSpawnId( entity ), 32 );
		msg.WriteString( value );
		msg.Send();
	} else {
		gameLocal.HandleNetworkMessage( gameLocal.GetLocalPlayer(), entity, value );
	}
}

/*
============
sdUserInterfaceLocal::Script_SendCommand
============
*/
void sdUserInterfaceLocal::Script_SendCommand( sdUIFunctionStack& stack ) {
	idStr value;
	stack.Pop( value );

	assert( entity );

	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_GUISCRIPT );
		msg.WriteBits( gameLocal.GetSpawnId( entity ), 32 );
		msg.WriteString( value );
		msg.Send();
	} else {
		gameLocal.HandleGuiScriptMessage( gameLocal.GetLocalPlayer(), entity, value );
	}
}

/*
============
sdUserInterfaceLocal::Script_SetShaderParm
============
*/
void sdUserInterfaceLocal::Script_SetShaderParm( sdUIFunctionStack& stack ) {
	int parm;
	float value;

	stack.Pop( parm );
	stack.Pop( value );

	if( parm < 4 || parm > MAX_ENTITY_SHADER_PARMS ) {
		gameLocal.Warning( "SetShaderParm: '%s' parameter '%i' out of range ( 4 - %i allowed )", GetName(), parm, MAX_ENTITY_SHADER_PARMS - 1 );
		return;
	}

	if( entity.IsValid() ) {
		entity->SetShaderParm( parm, value );
	}
	shaderParms[ parm - 4 ] = value;
}

/*
============
sdUserInterfaceLocal::Script_GetStringForProperty
============
*/
void sdUserInterfaceLocal::Script_GetStringForProperty( sdUIFunctionStack& stack ) {
	idStr propertyName;
	idStr value;

	stack.Pop( propertyName );
	stack.Pop( value );

	idLexer src( propertyName.c_str(), propertyName.Length(), "Script_GetStringForProperty" );

	bool success = false;
	sdUserInterfaceScope* scope = gameLocal.GetUserInterfaceScope( GetState(), &src );
	if( scope != NULL ) {
		idToken token;
		src.ReadToken( &token );

		sdProperties::sdProperty* prop = scope->GetProperty( token );
		if( prop != NULL ) {
			value = sdToString( *prop );
			success = true;
		}
	}
	stack.Push( value );
	if( !success ) {
		gameLocal.Warning( "GetStringForProperty: could  not find '%s'", propertyName.c_str() );
	}
}

/*
============
sdUserInterfaceLocal::Script_SetPropertyFromString
============
*/
void sdUserInterfaceLocal::Script_SetPropertyFromString( sdUIFunctionStack& stack ) {
	idStr propertyName;
	idStr value;

	stack.Pop( propertyName);
	stack.Pop( value );

	idLexer src( propertyName.c_str(), propertyName.Length(), "Script_SetPropertyFromString" );

	sdUserInterfaceScope* scope = gameLocal.GetUserInterfaceScope( GetState(), &src );
	if( scope != NULL ) {
		idToken token;
		src.ReadToken( &token );

		sdProperties::sdProperty* prop = scope->GetProperty( token );
		if( prop != NULL ) {
			sdFromString( *prop, value );
			return;
		}
	}
	gameLocal.Warning( "SetPropertyFromString: could  not set '%s' to '%s'", propertyName.c_str(), value.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_BroadcastEvent
============
*/
void sdUserInterfaceLocal::Script_BroadcastEvent( sdUIFunctionStack& stack ) {
	idStr parent;
	stack.Pop( parent );

	idStr eventName;
	stack.Pop( eventName );

	if( eventName.IsEmpty() ) {
		return;
	}

	// post to the UI if no parent is specified
	if( parent.IsEmpty() ) {
		PostNamedEvent( eventName, false );
		return;
	}

	sdUIObject* object = GetWindow( parent );
	if( object == NULL ) {
		gameLocal.Warning( "Script_BroadcastEvent: '%s' could not find parent window '%s'", guiDecl->GetName(), parent.c_str() );
		return;
	}
	object->PostNamedEvent( eventName, true );
	if( g_debugGUIEvents.GetInteger() ) {
		gameLocal.Printf( "Ran '%s' on '%s'\n", parent.c_str(), eventName.c_str() );
	}
}

/*
============
sdUserInterfaceLocal::Script_BroadcastEventToChildren
============
*/
void sdUserInterfaceLocal::Script_BroadcastEventToChildren( sdUIFunctionStack& stack ) {
	idStr parent;
	stack.Pop( parent );

	idStr eventName;
	stack.Pop( eventName );

	sdUIObject* object = GetWindow( parent );
	if( object == NULL ) {
		gameLocal.Warning( "Script_BroadcastEventToChildren: '%s' could not find parent window '%s'", guiDecl->GetName(), parent.c_str() );
		return;
	}

	sdUIObject* child = object->GetNode().GetChild();
	while( child != NULL ) {
		child->PostNamedEvent( eventName, true );
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUserInterfaceLocal::Script_BroadcastEventToDescendants
============
*/
void sdUserInterfaceLocal::Script_BroadcastEventToDescendants( sdUIFunctionStack& stack ) {
	idStr parent;
	stack.Pop( parent );

	idStr eventName;
	stack.Pop( eventName );

	sdUIObject* object = GetWindow( parent );
	if( object == NULL ) {
		gameLocal.Warning( "Script_BroadcastEventToDescendants: '%s' could not find parent window '%s'", guiDecl->GetName(), parent.c_str() );
		return;
	}

	sdUIObject* child = object->GetNode().GetChild();
	while( child != NULL ) {
		BroadcastEventToDescendants_r( child, eventName.c_str() );
		child = child->GetNode().GetSibling();
	}
}

/*
============
sdUserInterfaceLocal::BroadcastEventToDescendants_r
============
*/
void sdUserInterfaceLocal::BroadcastEventToDescendants_r( sdUIObject* parent, const char* eventName ) {
	if( parent == NULL ) {
		return;
	}

	bool result = parent->PostNamedEvent( eventName, true );
	if( result && g_debugGUIEvents.GetInteger() ) {
		gameLocal.Printf( "Script_BroadcastEventToDescendants: '%s' ran '%s' on '%s'\n", GetName(), eventName, parent->name.GetValue().c_str() );
	}

	sdUIObject* child = parent->GetNode().GetChild();
	while( child != NULL ) {
		BroadcastEventToDescendants_r( child, eventName );
		child = child->GetNode().GetSibling();
	}
}


/*
============
sdUserInterfaceLocal::Script_PushGeneralString
============
*/
void sdUserInterfaceLocal::Script_PushGeneralString( sdUIFunctionStack& stack ) {
	idStr stackName;
	idStr value;
	stack.Pop( stackName );
	stack.Pop( value );

	PushGeneralScriptVar( stackName.c_str(), value.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_PopGeneralString
============
*/
void sdUserInterfaceLocal::Script_PopGeneralString( sdUIFunctionStack& stack ) {
	idStr stackName;
	idStr value;
	stack.Pop( stackName );

	PopGeneralScriptVar( stackName.c_str(), value );

	stack.Push( value );
}

/*
============
sdUserInterfaceLocal::Script_GetGeneralString
============
*/
void sdUserInterfaceLocal::Script_GetGeneralString( sdUIFunctionStack& stack ) {
	idStr stackName;
	idStr value;
	stack.Pop( stackName );

	GetGeneralScriptVar( stackName.c_str(), value );

	stack.Push( value );
}

/*
============
sdUserInterfaceLocal::Script_GeneralStringAvailable
============
*/
void sdUserInterfaceLocal::Script_GeneralStringAvailable( sdUIFunctionStack& stack ) {
	idStr stackName;
	stack.Pop( stackName );
	stackHash_t::Iterator iter = generalStacks.Find( stackName.c_str() );
	bool available = !( iter == generalStacks.End() || iter->second.Empty() );
	stack.Push( available ? 1.0f : 0.0f );
}

/*
============
sdUserInterfaceLocal::Script_ClearGeneralStrings
============
*/
void sdUserInterfaceLocal::Script_ClearGeneralStrings( sdUIFunctionStack& stack ) {
	idStr stackName;
	stack.Pop( stackName );
	ClearGeneralStrings( stackName.c_str() );
}


/*
============
sdUserInterfaceLocal::Script_GetParentName
============
*/
void sdUserInterfaceLocal::Script_GetParentName( sdUIFunctionStack& stack ) {
	idStr windowName;
	stack.Pop( windowName );

	sdUIObject* obj = GetWindow( windowName.c_str() );
	if( obj == NULL ) {
		gameLocal.Warning( "Script_GetParentName: '%s' could not find window '%s'", guiDecl->GetName(), windowName.c_str() );
		stack.Push( "" );
		return;
	}

	sdUIObject* parent = obj->GetNode().GetParent();
	if( parent == NULL ) {
		stack.Push( "" );
	} else {
		stack.Push( parent->name.GetValue() );
	}
}


/*
============
sdUserInterfaceLocal::Script_CopyHandle
============
*/
void sdUserInterfaceLocal::Script_CopyHandle( sdUIFunctionStack& stack ) {
	// noop - this is a bit of a hack around the fact that int types aren't given an immediate() call
}

/*
============
sdUserInterfaceLocal::Script_ActivateMenuSoundWorld
============
*/
void sdUserInterfaceLocal::Script_ActivateMenuSoundWorld( sdUIFunctionStack& stack ) {
	float enable;
	stack.Pop( enable );

	if ( enable ) {
		idSoundWorld* gameSoundWorld = common->GetGameSoundWorld();
		idSoundWorld* menuSoundWorld = common->GetMenuSoundWorld();

		// pause the game sound world
		if ( gameSoundWorld != NULL && !gameSoundWorld->IsPaused() ) {
			gameSoundWorld->Pause();
		}

		// start playing the menu sounds
		soundSystem->SetPlayingSoundWorld( menuSoundWorld );
	} else {
		idSoundWorld* gameSoundWorld = common->GetGameSoundWorld();
		idSoundWorld* menuSoundWorld = common->GetMenuSoundWorld();

		// pause the game sound world
		if ( menuSoundWorld != NULL && !menuSoundWorld->IsPaused() ) {
			menuSoundWorld->Pause();
		}

		// go back to the game sounds
		soundSystem->SetPlayingSoundWorld( gameSoundWorld );

		// unpause the game sound world
		if ( gameSoundWorld != NULL && gameSoundWorld->IsPaused() ) {
			gameSoundWorld->UnPause();
		}
	}
}

/*
============
sdUserInterfaceLocal::Script_GetStringMapValue
============
*/
void sdUserInterfaceLocal::Script_GetStringMapValue( sdUIFunctionStack& stack ) {
	idStr defName;
	idStr key;
	idStr defaultValue;

	stack.Pop( defName );
	stack.Pop( key );
	stack.Pop( defaultValue );

	const sdDeclStringMap* decl = gameLocal.declStringMapType[ defName ];
	if( decl == NULL ) {
		gameLocal.Warning( "%s: GetStringMapValue: couldn't find stringMap '%s'", GetName(), defName.c_str() );
		stack.Push( "" );
		return;
	}
	stack.Push( decl->GetDict().GetString( key.c_str(), defaultValue.c_str() ) );
}


/*
============
sdUserInterfaceLocal::Script_IsBackgroundLoadComplete
============
*/
void sdUserInterfaceLocal::Script_IsBackgroundLoadComplete( sdUIFunctionStack& stack ) {
	bool complete = true;

	const idStrList& models = GetDecl()->GetPartialLoadModels();
	for( int i = 0; i < models.Num(); i++ ) {
		idRenderModel* renderModel = NULL;

		const idDeclModelDef* modelDef = gameLocal.declModelDefType.LocalFind( models[ i ].c_str(), false );
		if ( modelDef != NULL ) {
			renderModel = modelDef->ModelHandle();
		} else {
			renderModel = renderModelManager->GetModel( models[ i ].c_str() );
		}

		if ( renderModel != NULL ) {
			if( !renderModel->IsFinishedPartialLoading() ) {
				complete = false;
				break;
			}
		}
	}

	stack.Push( complete ? 1.0f : 0.0f );
}


/*
============
sdUserInterfaceLocal::Script_SetCookieString
============
*/
void sdUserInterfaceLocal::Script_SetCookieString( sdUIFunctionStack& stack ) {
	idStr key;
	idStr value;
	stack.Pop( key );
	stack.Pop( value );
	gameLocal.SetCookieString( key.c_str(), value.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_GetCookieString
============
*/
void sdUserInterfaceLocal::Script_GetCookieString( sdUIFunctionStack& stack ) {
	idStr key;
	stack.Pop( key );

	stack.Push( gameLocal.GetCookieString( key.c_str() ) );
}

/*
============
sdUserInterfaceLocal::Script_SetCookieInt
============
*/
void sdUserInterfaceLocal::Script_SetCookieInt( sdUIFunctionStack& stack ) {
	idStr key;
	int value;
	stack.Pop( key );
	stack.Pop( value );
	gameLocal.SetCookieInt( key.c_str(), value );
}

/*
============
sdUserInterfaceLocal::Script_GetCookieInt
============
*/
void sdUserInterfaceLocal::Script_GetCookieInt( sdUIFunctionStack& stack ) {
	idStr key;
	stack.Pop( key );

	stack.Push( gameLocal.GetCookieInt( key.c_str() ) );
}

/*
============
sdUserInterfaceLocal::Script_CacheMaterial
============
*/
void sdUserInterfaceLocal::Script_CacheMaterial( sdUIFunctionStack& stack ) {
	idStr alias;
	stack.Pop( alias );

	idStr material;
	stack.Pop( material );

	int handle;
	SetCachedMaterial( alias, material, handle );
	stack.Push( handle );
}

/*
============
sdUserInterfaceLocal::Script_CopyText
============
*/
void sdUserInterfaceLocal::Script_CopyText( sdUIFunctionStack& stack ) {
	idWStr str;
	stack.Pop( str );
	sys->SetClipboardData( str.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_PasteText
============
*/
void sdUserInterfaceLocal::Script_PasteText( sdUIFunctionStack& stack ) {
	stack.Push( sys->GetClipboardData().c_str() );
}

/*
============
sdUserInterfaceLocal::Script_UploadLevelShot
============
*/
void sdUserInterfaceLocal::Script_UploadLevelShot( sdUIFunctionStack& stack ) {
	idStr image;
	stack.Pop( image );

	idStr targetName;
	stack.Pop( targetName );

	if( image.IsEmpty() ) {
		gameLocal.Warning( "UploadLevelShot: blank image name" );
		return;
	}

	if( targetName.IsEmpty() ) {
		gameLocal.Warning( "UploadLevelShot: blank target name" );
		return;
	}

	byte* pic;
	int width;
	int height;

	fileSystem->ReadTGA( image, &pic, &width, &height, NULL, false );	// ensure that paks aren't marked as referenced
	if ( pic != NULL ) {
		if( renderSystem != NULL ) {
			renderSystem->UploadImage( targetName, (byte *)pic, width, height, true );
		}
	}
	fileSystem->FreeTGA( pic );
}

/*
============
sdUserInterfaceLocal::Script_GetLoadTip
============
*/
void sdUserInterfaceLocal::Script_GetLoadTip( sdUIFunctionStack& stack ) {
	int max = 0;
	while( true ) {
		if( declHolder.declLocStrType.LocalFind( va( "loadtips/%i", max ), false ) == NULL ) {
			break;
		}
		max++;
	}
	if( max == 0 ) {
		stack.Push( L"" );
		return;
	}

	int num = gameLocal.random.RandomInt( max );
	idWStr text = declHolder.declLocStrType.LocalFind( va( "loadtips/%i", num ) )->GetText();

	// from tooltip parsing
	for ( int index = 0; index < text.Length(); index++ ) {
		wchar_t c = text[ index ];
		if ( c != L'%' ) {
			continue;
		}

		index++;
		if ( index > text.Length() ) {
			break;
		}

		c = text[ index ];

		if ( c == L'k' ) {
			int keyStart = index - 1;	// grab the %

			index++;
			if ( index >= text.Length() || text[ index ] != L'(' ) {
				stack.Push( L"" );
				return;
			}

			idWStr keyBuffer;
			while ( true ) {
				index++;

				if( index >= text.Length() ) {
					stack.Push( L"" );
					return;
				}
				if ( text[ index ] == L')' ) {
					break;
				}
				keyBuffer += text[ index ];
			}

			idWStr key;
			keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), va( "%ls", keyBuffer.c_str() ), true, key );

			const int keyEnd = index;
			index = keyStart;
			text.EraseRange( keyStart, keyEnd - keyStart + 1 );
			text.Insert( key.c_str(), keyStart );
		}
	}

	stack.Push( text.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_GetGameTag
============
*/
void sdUserInterfaceLocal::Script_GetGameTag( sdUIFunctionStack& stack ) {
#define SD_BUILD_TAG L"Public Beta"
#if defined( SD_BUILD_TAG )
	stack.Push( SD_BUILD_TAG );
#else
	stack.Push( L"" );
#endif
}

/*
============
sdUserInterfaceLocal::Script_CollapseColors
============
*/
void sdUserInterfaceLocal::Script_CollapseColors( sdUIFunctionStack& stack ) {
	idStr text;
	stack.Pop( text );

	text.CollapseColors();
	stack.Push( text );
}


/*
============
sdUserInterfaceLocal::Script_CancelToolTip
============
*/
void sdUserInterfaceLocal::Script_CancelToolTip( sdUIFunctionStack& stack ) {
	CancelToolTip();
}


/*
============
sdUserInterfaceLocal::Script_CheckCVarsAgainstCFG
quick and dirty config check
============
*/
void sdUserInterfaceLocal::Script_CheckCVarsAgainstCFG( sdUIFunctionStack& stack ) {
	idStr folder;
	stack.Pop( folder );

	idStr extension;
	stack.Pop( extension );

	idStr fileBase;
	stack.Pop( fileBase );

	idFileList* files = fileSystem->ListFiles( folder.c_str(), extension.c_str() );
	sdStringBuilder_Heap builder;

	bool match = true;
	idToken token;
	for( int i = 0; i < files->GetNumFiles(); i++ ) {
		const char* file = files->GetFile( i );
		if( fileBase.Icmpn( file, fileBase.Length() ) != 0 ) {
			continue;
		}
		builder = files->GetBasePath();
		builder += "/";
		builder += file;
		idLexer src( builder.c_str(), LEXFL_ALLOWPATHNAMES );
		if( src.IsLoaded() ) {
			while( src.ReadToken( &token ) && match ) {
				if( token.Icmpn( "set", 3 ) == 0 ) {
					idToken cvar;
					idToken value;
					src.ReadToken( &cvar );
					src.ReadToken( &value );

					// negative number
					if( value.Cmp( "-" ) == 0 ) {
						idStr temp = value;
						src.ReadToken( &value );
						temp += value;
						value = temp;
					}

					idCVar* cvarObj = cvarSystem->Find( cvar.c_str() );
					if( cvarObj != NULL ) {
						if( value.Icmp( cvarObj->GetString() ) != 0 ) {
							match = false;
						}
					}
				} else if( token.Icmp( "reset" ) == 0 ) {
					idToken cvar;
					src.ReadToken( &cvar );

					idCVar* cvarObj = cvarSystem->Find( cvar.c_str() );
					if( cvarObj != NULL ) {
						if( !cvarObj->IsDefaultValue() ) {
							match = false;
						}
					}
				} else {
					while( src.ReadTokenOnLine( &token ) ) { ; }
				}
			}
		}
	}
	fileSystem->FreeFileList( files );

	stack.Push( match );

}

/*
============
sdUserInterfaceLocal::Script_OpenURL
============
*/
void sdUserInterfaceLocal::Script_OpenURL( sdUIFunctionStack& stack ) {
	idStr url;
	stack.Pop( url );
	url.StripLeadingWhiteSpace();
	url.StripTrailingWhiteSpace();

	sys->OpenURL( url, true );
}


/*
============
sdUserInterfaceLocal::Script_SoundTest
============
*/
void sdUserInterfaceLocal::Script_SoundTest( sdUIFunctionStack& stack ) {
	int iCommand;
	stack.Pop( iCommand );

	int duration;
	stack.Pop( duration );

	float returnValue = 0.0f;
	soundTestCommand_e command;
	if( sdIntToContinuousEnum<soundTestCommand_e>( iCommand, STC_MIN, STC_MAX, command ) ) {
		switch( command ) {
			case STC_START:
				networkSystem->StartSoundTest( duration );
				break;
			case STC_STATUS_PERCENT:
				returnValue = networkSystem->GetSoundTestProgress();
				break;
			case STC_STATUS_PLAYBACK:
				returnValue = networkSystem->IsSoundTestPlaybackActive() ? 1.0f : 0.0f;
				break;
			case STC_STATUS_RECORDING:
				returnValue = networkSystem->IsSoundTestActive() ? 1.0f : 0.0f;
				break;
		}
	} else {
		gameLocal.Warning( "Script_SoundTest: Invalid enum '%i'", iCommand );
	}
	
	stack.Push( returnValue );
}

/*
============
sdUserInterfaceLocal::Script_DeleteFile
============
*/
void sdUserInterfaceLocal::Script_DeleteFile( sdUIFunctionStack& stack ) {
	idStr file;
	stack.Pop( file );
	fileSystem->RemoveFile( file.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_VoiceChat
============
*/
void sdUserInterfaceLocal::Script_VoiceChat( sdUIFunctionStack& stack ) {
	int iCommand;
	stack.Pop( iCommand );

	float returnValue = 0.0f;
	voipCommand_e command;
	if( sdIntToContinuousEnum<voipCommand_e>( iCommand, VOIPC_MIN, VOIPC_MAX, command ) ) {
		switch( command ) {
			case VOIPC_DISABLE:
				networkSystem->DisableVoip();
				break;
			case VOIPC_FIRETEAM:
				networkSystem->EnableVoip( VO_FIRETEAM );
				break;
			case VOIPC_GLOBAL:
				networkSystem->EnableVoip( VO_GLOBAL );
				break;
			case VOIPC_TEAM:
				networkSystem->EnableVoip( VO_TEAM );
				break;
		}
	} else {
		gameLocal.Warning( "Script_VoiceChat: Invalid enum '%i'", iCommand );
	}
}

/*
============
sdUserInterfaceLocal::Script_RefreshSoundDevices
============
*/
void sdUserInterfaceLocal::Script_RefreshSoundDevices( sdUIFunctionStack& stack ) {
	soundSystem->RefreshSoundDevices();
}
