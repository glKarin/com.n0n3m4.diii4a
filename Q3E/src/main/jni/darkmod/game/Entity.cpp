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



#pragma warning(disable : 4533 4800)

#include "Game_local.h"
#include "DarkModGlobals.h"
#include "declxdata.h"
#include "Objectives/MissionData.h"
#include "Objectives/ObjectiveLocation.h"
#include "Grabber.h"
#include "SndProp.h"
#include "StimResponse/StimResponseCollection.h"
#include "Inventory/Inventory.h"
#include "Inventory/Cursor.h"
#include "AbsenceMarker.h"
#include "Objectives/MissionData.h"
#include <algorithm>
#include "LodComponent.h"
#include "LightEstimateSystem.h"

/*
===============================================================================

	idEntity

===============================================================================
*/

// TDM uses entity shaderparm 11 to control frob highlight state
#define FROB_SHADERPARM 11

#define FIRST_TIME_SOUND_PROP_ALLOWED 2000 // grayman #3768 - no sound propagation before this time

const idStrList areaLockOptions{ "origin", "center" }; // not sure how to make it work with char*[]  

// overridable events
const idEventDef EV_PostSpawn( "<postspawn>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_PostPostSpawn( "<postpostspawn>", EventArgs(), EV_RETURNS_VOID, "internal" ); // grayman #3643
const idEventDef EV_FindTargets( "<findTargets>", EventArgs(), EV_RETURNS_VOID, "internal" );
const idEventDef EV_Touch( "<touch>", EventArgs('e', "", "", 't', "", ""), EV_RETURNS_VOID, "internal" );

const idEventDef EV_GetName("getName", EventArgs(), 's', "Returns the name of this entity.");
const idEventDef EV_SetName( "setName", EventArgs('s', "name", "the new name"), EV_RETURNS_VOID, "Sets the name of this entity.");
const idEventDef EV_IsType ( "isType", EventArgs('s', "spawnclass", "spawn class name"), 'd', "Returns true if this entity is of the given type." );

const idEventDef EV_Activate( "activate", EventArgs('e', "activator", "the entity that caused the action (usually the player)"), 
	EV_RETURNS_VOID, "Activates this entity as if it was activated by a trigger." );

const idEventDef EV_ActivateTargets( "activateTargets", EventArgs('e', "activator", "the entity that caused the action (usually the player)"), 
	EV_RETURNS_VOID, "Causes this entity to activate all it's targets. Similar to how a trigger activates entities." );

const idEventDef EV_AddTarget( "addTarget", EventArgs('e', "target", "the entity to add as target"), EV_RETURNS_VOID, "Add a target to this entity.");
const idEventDef EV_RemoveTarget( "removeTarget", EventArgs('e', "target", "the entity to remove from the targets"), EV_RETURNS_VOID, "Remove a target from this entity." );
const idEventDef EV_NumTargets( "numTargets", EventArgs(), 'f', "Returns the number of entities this entity has targeted." );
const idEventDef EV_GetTarget( "getTarget", EventArgs('f', "num", "The target number. Starts at 0."), 'e', "Returns the requested target entity." );
const idEventDef EV_RandomTarget( "randomTarget", EventArgs('s', "ignoreName", "the name of an entity to ignore"), 'e', 
	"Returns a random targeted entity. Pass in an entity name to skip that entity." );

const idEventDef EV_Bind( "bind", EventArgs('e', "master", "the entity to bind to"), EV_RETURNS_VOID, 
	"Fixes this entity's position and orientation relative to another entity,\nsuch that when the master entity moves, so does this entity." );

const idEventDef EV_BindPosition( "bindPosition", EventArgs('e', "master", "the entity to bind to"), EV_RETURNS_VOID, 
	"Fixes this entity's position (but not orientation) relative to another entity,\nsuch that when the master entity moves, so does this entity.");

const idEventDef EV_BindToJoint( "bindToJoint", 
	EventArgs('e', "master", "the entity to bind to", 
			  's', "boneName", "the bone name", 
			  'f', "rotateWithMaster", "-"), EV_RETURNS_VOID, 
	"Fixes this entity's position and orientation relative to a bone on another entity,\nsuch that when the master's bone moves, so does this entity.");

const idEventDef EV_BindToBody( "bindToBody", 
	EventArgs('e', "master", "entity to bind to", 
			  'd', "bodyID", "AF body ID to bind to", 
			  'd', "orientated", "binds the orientation as well as position, if set to 1"), EV_RETURNS_VOID, "Bind to AF body");

const idEventDef EV_GetBindMaster( "getBindMaster", EventArgs(), 'e', "Returns the entity's bindmaster");
const idEventDef EV_NumBindChildren( "numBindChildren", EventArgs(), 'd', "Returns the number of bound entities lower down in the bind chain than this entity, but be sure to give it the topmost bindmaster" );
const idEventDef EV_GetBindChild( "getBindChild", EventArgs('d', "ind", "child index"), 'e', "Returns the ind_th bind child of this entity or NULL if index is invalid.\nNOTE: indices start at zero" );
const idEventDef EV_Unbind( "unbind", EventArgs(), EV_RETURNS_VOID, "Detaches this entity from its master.");
const idEventDef EV_RemoveBinds( "removeBinds", EventArgs(), EV_RETURNS_VOID, "Removes all attached entities from the game" );

const idEventDef EV_SpawnBind( "<spawnbind>", EventArgs(), EV_RETURNS_VOID, "");

const idEventDef EV_SetOwner( "setOwner", EventArgs('e', "owner", "the entity which will be made owner of this entity"), 
	EV_RETURNS_VOID, "Sets the owner of this entity. Entities will never collide with their owner.");

const idEventDef EV_SetModel( "setModel", EventArgs('s', "modelName", ""), EV_RETURNS_VOID, "Sets the model this entity uses");
const idEventDef EV_SetSkin( "setSkin", EventArgs('s', "skinName", ""), EV_RETURNS_VOID, "Sets the skin this entity uses.  Set to \"\" to turn off the skin.");
const idEventDef EV_ReskinCollisionModel( "reskinCollisionModel", EventArgs(), EV_RETURNS_VOID, 
	"For use after setSkin() on moveables and static models, if the CM needs to be refreshed to update surface "
	"properties after a skin change. CM will be regenerated from the original model file with the new skin.");

const idEventDef EV_GetWorldOrigin( "getWorldOrigin", EventArgs(), 'v', "Returns the current world-space position of this entity (regardless of any bind parent)." );
const idEventDef EV_SetWorldOrigin( "setWorldOrigin", EventArgs('v', "origin", ""), EV_RETURNS_VOID, "Sets the current position of this entity (regardless of any bind parent).");
const idEventDef EV_GetOrigin( "getOrigin", EventArgs(), 'v', "Returns the current position of this entity (relative to bind parent if any)." );
const idEventDef EV_SetOrigin( "setOrigin", EventArgs('v', "origin", "the new origin"), EV_RETURNS_VOID, "Sets the current position of this entity (relative to it's bind parent if any)");
const idEventDef EV_GetAngles( "getAngles", EventArgs(), 'v', "Returns the current orientation of this entity (relative to bind parent if any)." );
const idEventDef EV_SetAngles( "setAngles", EventArgs('v', "angles", "the new orientation"), EV_RETURNS_VOID, "Sets the current orientation of this entity (relative to bind parent if any)");
const idEventDef EV_GetLinearVelocity( "getLinearVelocity", EventArgs(), 'v', 
	"Gets the current linear velocity of this entity. The linear velocity of a physics\nobject is a vector that defines the translation of the center of mass in units per second." );
const idEventDef EV_SetLinearVelocity( "setLinearVelocity", EventArgs('v', "velocity", ""), EV_RETURNS_VOID, 
	"Sets the current linear velocity of this entity in units per second.\nThe linear velocity of a physics object is a vector that defines the translation of the center of mass in units per second.");
const idEventDef EV_GetAngularVelocity( "getAngularVelocity", EventArgs(), 'v', 
	"Gets the current angular velocity of this entity. The angular velocity of\n" \
	"a physics object is a vector that passes through the center of mass. The\n" \
	"direction of this vector defines the axis of rotation and the magnitude\n" \
	"defines the rate of rotation about the axis in radians per second." );
const idEventDef EV_SetAngularVelocity( "setAngularVelocity", EventArgs('v', "velocity", ""), EV_RETURNS_VOID, 
	"Sets the current angular velocity of this entity. The angular velocity of\n" \
	"a physics object is a vector that passes through the center of mass. The\n" \
	"direction of this vector defines the axis of rotation and the magnitude\n" \
	"defines the rate of rotation about the axis in radians per second.");
const idEventDef EV_SetGravity( "setGravity", EventArgs('v', "newGravity", ""), EV_RETURNS_VOID, "Sets a new gravity vector for this entity. Note that non-upright vectors are poorly supported at present.");

// tels #2897
const idEventDef EV_ApplyImpulse( "applyImpulse", EventArgs(
	'e', "source", "Pass $null_entity or the entity that applies the impulse", 
	'd', "bodyid", "For articulated figures, ID of the body, 0 for the first (main) body. Otherwise use 0.", 
	'v',"point", "Point on the body where the impulse is applied to",
	'v',"impulse","Vector of the impulse"
	), EV_RETURNS_VOID, "Applies an impulse to the entity. Example: entity.applyImpulse($player1, 0, entity.getOrigin(), '0 0 2');" ); 

// greebo: Accessor events for the clipmask/contents flags
const idEventDef EV_SetContents( "setContents", EventArgs('f', "contents", ""), EV_RETURNS_VOID, "Sets the contents of the physics object.");
const idEventDef EV_GetContents( "getContents", EventArgs(), 'f', "Returns the contents of the physics object." );
const idEventDef EV_SetClipMask( "setClipMask", EventArgs('d', "clipMask", ""), EV_RETURNS_VOID, "Sets the clipmask of the physics object.");
const idEventDef EV_GetClipMask( "getClipMask", EventArgs(), 'd', "Returns the clipmask of the physics object." );
const idEventDef EV_SetSolid( "setSolid", EventArgs('d', "solidity", ""), EV_RETURNS_VOID, "Set the solidity of the entity. If the entity has never been solid before it will be assigned solid and opaque contents/clip masks." );

const idEventDef EV_GetSize( "getSize", EventArgs(), 'v', "Gets the size of this entity's bounding box." );
const idEventDef EV_SetSize( "setSize", EventArgs('v', "min", "minimum corner coordinates", 'v', "max", "maximum corner coordinates"), EV_RETURNS_VOID, "Sets the size of this entity's bounding box.");
const idEventDef EV_GetMins( "getMins", EventArgs(), 'v', "Gets the minimum corner of this entity's bounding box." );
const idEventDef EV_GetMaxs( "getMaxs", EventArgs(), 'v', "Gets the maximum corner of this entity's bounding box." );
const idEventDef EV_IsHidden( "isHidden", EventArgs(), 'd', "checks if the entity's model is invisible." );
const idEventDef EV_Hide( "hide", EventArgs(), EV_RETURNS_VOID, "Makes this entity invisible.");
const idEventDef EV_Show( "show", EventArgs(), EV_RETURNS_VOID, "Makes this entity visible if it has a model.");
const idEventDef EV_Touches( "touches", EventArgs('E', "other", "the entity to check against"), 'd', "Returns true if this entity touches the other entity." );
const idEventDef EV_ClearSignal( "clearSignal", EventArgs('d', "signalNum", "signal number"), EV_RETURNS_VOID, "Disables the callback function on the specified signal.");
const idEventDef EV_GetShaderParm( "getShaderParm", EventArgs('d', "parm", "shader parm index"), 'f', "Gets the value of the specified shader parm." );
const idEventDef EV_SetShaderParm( "setShaderParm", EventArgs('d', "parm", "shader parm index", 'f', "value", "new value"), EV_RETURNS_VOID, "Sets the value of the specified shader parm.");
const idEventDef EV_SetShaderParms( "setShaderParms", EventArgs('f', "parm0", "red", 'f', "parm1", "green", 'f', "parm2", "blue", 'f', "parm3", "alpha"), EV_RETURNS_VOID, 
	"Sets shader parms Parm0, Parm1, Parm2, and Parm3 (red, green, blue, and alpha respectively)." );
const idEventDef EV_SetColor( "setColor", EventArgs('f', "parm0", "red", 'f', "parm1", "green", 'f', "parm2", "blue"), EV_RETURNS_VOID, 
	"Sets the RGB color of this entity (shader parms Parm0, Parm1, Parm2). See also setColorVec for a variant that accepts a vector instead." );
const idEventDef EV_SetColorVec( "setColorVec", EventArgs('v', "newColor", ""), EV_RETURNS_VOID, "Similar to setColor, but accepts the new RGB color as a vector instead of 3 different floats." );
const idEventDef EV_GetColor( "getColor", EventArgs(), 'v', "Gets the color of this entity (shader parms Parm0, Parm1, Parm2)." );
const idEventDef EV_SetHealth( "setHealth", EventArgs( 'f', "newHealth", "" ), EV_RETURNS_VOID, 
	"Sets the health of this entity to the new value. Setting health to 0 or lower via this method will result in the entity switching to its broken state." );
const idEventDef EV_GetHealth( "getHealth", EventArgs(), 'f', "Gets the health of this entity." );
const idEventDef EV_SetMaxHealth( "setMaxHealth", EventArgs( 'f', "newMaxHealth", "" ), EV_RETURNS_VOID, 
	"Sets the max health of this entity to the new value. If current health is higher than max health, the current health will be lowered to become equal to max health. Setting health to 0 or lower via this method will result in the entity switching to its broken state." );
const idEventDef EV_GetMaxHealth( "getMaxHealth", EventArgs(), 'f', "Gets the max health of this entity." );

const idEventDef EV_SetFrobActionScript( "setFrobActionScript", EventArgs('s', "frobActionScript", "the new script to call when the entity is frobbed"), EV_RETURNS_VOID, 
	"Changes the frob action script of this entity. Also updates the 'frob_action_script' spawnarg.");
const idEventDef EV_SetUsedBy( "setUsedBy", EventArgs('e', "ent", "specify an entity here, like a key", 'd', "b_canUse", "whether the specified entity can use this entity"), EV_RETURNS_VOID,
	"Allows to change which entities can use this entity.");
const idEventDef EV_CacheSoundShader( "cacheSoundShader", EventArgs('s', "shaderName", "the sound shader to cache"), EV_RETURNS_VOID, 
	"Ensure the specified sound shader is loaded by the system.\nPrevents cache misses when playing sound shaders.");
const idEventDef EV_StartSoundShader( "startSoundShader", EventArgs('s', "shaderName", "the sound shader to play", 'd', "channel", "the channel to play the sound on"), 'f', 
	"Plays a specific sound shader on the channel and returns the length of the sound in\n" \
	"seconds. This is not the preferred method of playing a sound since you must ensure\n" \
	"that the sound is loaded." );
const idEventDef EV_StartSound( "startSound", 
	EventArgs('s', "sound", "the spawnarg to reference, e.g. 'snd_move'", 'd', "channel", "the channel to play on", 'd', "netSync", "-"), 'f', 
	"Plays the sound specified by the snd_* key/value pair on the channel and returns\n" \
	"the length of the sound in seconds. This is the preferred method for playing sounds on an\n" \
	"entity since it ensures that the sound is precached." );

const idEventDef EV_StopSound( "stopSound", EventArgs('d', "channel", "the channel to stop playback on", 'd', "netSync", "-"), EV_RETURNS_VOID, "Stops a specific sound shader on the channel.");
const idEventDef EV_FadeSound( "fadeSound", EventArgs('d', "channel", "", 'f', "newLevel", "", 'f', "fadeTime", ""), EV_RETURNS_VOID, 
	"Fades the sound on this entity to a new level over a period of time.  Use SND_CHANNEL_ANY for all currently playing sounds." );

const idEventDef EV_SetSoundVolume( "setSoundVolume", EventArgs('f', "newLevel", ""), EV_RETURNS_VOID, "Set the volume of the sound to play, must be issued before startSoundShader.");
const idEventDef EV_GetSoundVolume( "getSoundVolume", EventArgs('s', "soundName", "the name of the sound"), 'f', "Get the volume of the sound to play."); // grayman #3395

const idEventDef EV_GetNextKey( "getNextKey", EventArgs('s', "prefix", "", 's', "lastMatch", ""), 's', 
	"Searches for the name of a spawn arg that matches the prefix.  For example,\n" \
	"passing in \"attack_target\" matches \"attack_target1\", \"attack_targetx\", \"attack_target_enemy\", \n" \
	"etc. The returned string is the name of the key which can then be passed into\n" \
	"functions like getKey() to lookup the value of that spawn arg.  This\n" \
	"is useful for when you have multiple values to look up, like when you\n" \
	"target multiple objects.  To find the next matching key, pass in the previous\n" \
	"result and the next key returned will be the first one that matches after\n" \
	"the previous result. Pass in \"\" to get the first match. Passing in a\n" \
	"non-existent key is the same as passing in \"\". Returns \"\" when no \n" \
	"more keys match.");

const idEventDef EV_SetKey( "setKey", EventArgs('s', "key", "the spawnarg to set", 's', "value", "the value to store"), EV_RETURNS_VOID, 
	"Sets a key on this entity's spawn args. Note that most spawn args are evaluated when\n" \
	"this entity spawns in, so this will not change the entity's behavior in most cases.\n" \
	"This is chiefly for saving data the script needs in an entity for later retrieval.");

const idEventDef EV_GetKey( "getKey", EventArgs('s', "key", "spawnarg name"), 's', 
	"Retrieves the value of a specific spawn arg, defaulting to ''." );
const idEventDef EV_GetIntKey( "getIntKey", EventArgs('s', "key", "spawnarg name"), 'f', "Retrieves the integer value of a specific spawn arg, defaulting to 0." );
const idEventDef EV_GetBoolKey( "getBoolKey", EventArgs('s', "key", "spawnarg name"), 'f', "Retrieves the boolean value of a specific spawn arg, defaulting to false." );
const idEventDef EV_GetFloatKey( "getFloatKey", EventArgs('s', "key", "spawnarg name"), 'f', "Retrieves the floating point value of a specific spawn arg, defaulting to 0.0f." );
const idEventDef EV_GetVectorKey( "getVectorKey", EventArgs('s', "key", "spawnarg name"), 'v', "Retrieves the vector value of a specific spawn arg, defaulting to '0 0 0'." );
const idEventDef EV_GetEntityKey( "getEntityKey", EventArgs('s', "key", "spawnarg name"), 'e', "Retrieves the entity specified by the spawn arg." );
const idEventDef EV_RemoveKey( "removeKey", EventArgs('s', "key", "the spawnarg to remove"), EV_RETURNS_VOID, "Removes a key from an object's spawnargs, so things like getNextKey() don't retrieve it.");
const idEventDef EV_RestorePosition( "restorePosition", EventArgs(), EV_RETURNS_VOID, 
	"Returns this entity to the position stored in the \"origin\" spawn arg.\n" \
	"This is the position the entity was spawned in unless the \"origin\" key is changed.\n" \
	"Note that there is no guarantee that the entity won't be stuck in another entity\n" \
	"when moved, so care should be taken to make sure that isn't possible." );

const idEventDef EV_UpdateCameraTarget( "<updateCameraTarget>", EventArgs(), EV_RETURNS_VOID, "");

const idEventDef EV_DistanceTo( "distanceTo", EventArgs('E', "other", ""), 'f', "Returns the distance of this entity to another entity." );
const idEventDef EV_DistanceToPoint( "distanceToPoint", EventArgs('v', "point", ""), 'f', "Returns the distance of this entity to a point." );
const idEventDef EV_StartFx( "startFx", EventArgs('s', "fx", ""), EV_RETURNS_VOID, "Starts an FX on this entity.");
const idEventDef EV_HasFunction( "hasFunction", EventArgs('s', "functionName", ""), 'd', "checks if an entity's script object has a specific function" );
const idEventDef EV_CallFunction( "callFunction", EventArgs('s', "functionName", ""), EV_RETURNS_VOID, 
	"Calls a function on an entity's script object. See also callGlobalFunction().");
const idEventDef EV_CallGlobalFunction( "callGlobalFunction", EventArgs('s', "functionName", "", 'E', "other", ""), EV_RETURNS_VOID, 
	"calls a global function and passes the other entity along as the first argument\n" \
	"calls the function in a new thread, so it continues executing in the current\n" \
	"thread right away (unlike entity.callFunction( \"blah\"))");
const idEventDef EV_SetNeverDormant( "setNeverDormant", EventArgs('d', "enable", "1 = enable, 0 = disable"), EV_RETURNS_VOID, "enables or prevents an entity from going dormant");

const idEventDef EV_ExtinguishLights("extinguishLights", EventArgs(), EV_RETURNS_VOID, "Extinguishes all lights (i.e. the <self> entity plus all bound lights)");

const idEventDef EV_InPVS( "inPVS", EventArgs(), 'd', 
	"Returns non-zero if this entity is in PVS.\n" \
	"For lights, it will return true when the light's bounding box is in PVS,\n" \
	"even though the light may not actually be in PVS. (an unmoved shadowcasting\n" \
	"light may not be visible to PVS areas its bounding box intersects with)");

// greebo: Script event definition for dealing damage
const idEventDef EV_Damage("damage", EventArgs(
	'E', "inflictor", "the entity causing the damage (maybe a projectile)", 
	'E', "attacker", "the \"parent\" entity of the inflictor, the one that is responsible for the inflictor (can be the same)", 
	'v', "dir", "the direction the attack is coming from.", 
	's', "damageDefName", "the name of the damage entityDef to know what damage is being dealt to <self> (e.g. \"damage_lava\")", 
	'f', "damageScale", "the scale of the damage (pass 1.0 as default, this should be ok)."), 
	EV_RETURNS_VOID, 
	"Deals damage to this entity (gets translated into the idEntity::Damage() method within the SDK).");

// greebo: Script event definition for healing 
// Takes the name of the healing entity and the healing scale, returns an integer
const idEventDef EV_Heal("heal", 
	EventArgs('s', "healDefName", "the name of the entityDef containing the healing information (e.g. \"heal_potion\")", 
			  'f', "healScale", "the scaling value to be applied to the healAmount found in the healEntityDef"), 
    'd', 
	"Heals the entity this is called on using the specified healing entityDef.\n" \
	"Returns 1 if the entity could be healed, 0 otherwise (if the entity is already at full health, for ex.)");

// tels: Teleport the entity to the position/orientation of the given entity
const idEventDef EV_TeleportTo("teleportTo", EventArgs('e', "other", ""), EV_RETURNS_VOID, 
	"Teleports the entity to the position of the other entity, plus a possible\n" \
	"offset and random offset (defined on the spawnargs of the entity to be teleported)");

// ishtvan: Get/setd droppable on entity and associated inventory item
const idEventDef EV_IsDroppable( "isDroppable", EventArgs(), 'd', "Get whether an item may be dropped from the inventory");
const idEventDef EV_SetDroppable( "setDroppable", 
	EventArgs('d', "droppable", "if non-zero the item becomes droppable, when called with 0 the item becomes non-droppable"), 
	EV_RETURNS_VOID, 
	"Set whether an item may be dropped from the inventory.\n" \
	"");

// tels: set noShadow on this entity to the given argument (true/false)
const idEventDef EV_NoShadows( "noShadows", EventArgs('d', "noShadows", "1 = disable shadows, 0 = enable shadows"), EV_RETURNS_VOID, 
	"Sets the noShadow property on the entity to true/false, turning shadowcasting\n" \
	"on or off for this entity.");

// tels: like EV_noShadows, but does so after a delay of "delay" ms:
const idEventDef EV_NoShadowsDelayed( "noShadowsDelayed", 
	EventArgs('d', "noShadows", "1 = disable shadows, 0 = enable shadows", 'f', "delay", "delay in ms"), EV_RETURNS_VOID, 
	"Sets the noShadow property on the entity to true/false after delay\n" \
	"in ms, turning shadows cast by this entity on or off.");

// tels: Find all lights in the player PVS, then returns their sum.
const idEventDef EV_GetLightInPVS("getLightInPVS", 
	EventArgs('f', "falloff", "0: no falloff with distance\n" \
							" 0.5: sqrt(linear) falloff	(dist 100 => 1/10)\n" \
							" 1: linear falloff			(dist 100 => 1/100)\n" \
							" 2: square falloff			(dist 100 => 1/10000)\n", 
			  'f', "scaling", "factor to scale the distance, can be used to lower/raise distance factor\n" \
					" after the linear or square scaling has been used\n" \
					"good looking values are approx: sqrt(linear): 0.01, linear: 0.1, square 1.0"), 
	'v', 
	"Computes the sum of all light in the PVS of the entity you\n" \
	"call this on, and returns a vector with the sum.");

const idEventDef EV_GetVinePlantLoc("getVinePlantLoc", EventArgs(), 'v', "Event important to the growing of vines from vine arrows");	// grayman #2787
const idEventDef EV_GetVinePlantNormal("getVinePlantNormal", EventArgs(), 'v', "Event important to the growing of vines from vine arrows");	// grayman #2787

const idEventDef EV_IsLight("isLight", EventArgs(), 'd', ""); // grayman #2905
const idEventDef EV_ActivateContacts("activateContacts", EventArgs(), EV_RETURNS_VOID, "Activate objects sitting on this object."); // grayman #3011
const idEventDef EV_GetLocation("getLocation", EventArgs(), 'e', 
	"Returns the idLocation entity corresponding to the entity's current location.\n" \
	"This was player-specific before, but is now available to all entities."); // grayman #3013
const idEventDef EV_GetEntityFlag("getEntityFlag", EventArgs('s', "flagName",
	"Can be one of (case insensitive):\n"
	"\tnotarget: if true never attack or target this entity\n"
	"\tnoknockback: if true no knockback from hits\n"
	"\ttakedamage: if true this entity can be damaged\n"
	"\thidden: if true this entity is not visible\n"
	"\tbindOrientated: if true both the master orientation is used for binding\n"
	"\tsolidForTeam: if true this entity is considered solid when a physics team mate pushes entities\n"
	"\tforcePhysicsUpdate: if true always update from the physics whether the object moved or not\n"
	"\tselected: if true the entity is selected for editing\n"
	"\tneverDormant: if true the entity never goes dormant\n"
	"\tisDormant: if true the entity is dormant\n"
	"\thasAwakened: before a monster has been awakened the first time, use full PVS for dormant instead of area-connected\n"
	"\tinvisible: if true this entity cannot be seen\n"
	"\tinaudible: if true this entity cannot be heard\n"
), 'd', "Returns the value of the specified entity flag.");

//===============================================================
//                   TDM GUI interface
//===============================================================
const idEventDef EV_SetGui( "setGui", EventArgs('d', "handle", "", 's', "guiFile", ""), EV_RETURNS_VOID, "Loads a new file into an existing GUI.");
const idEventDef EV_GetGui( "getGui", EventArgs('d', "handle", ""), 's', "Returns the file currently loaded by a GUI." );
const idEventDef EV_SetGuiString( "setGuiString", 
	EventArgs('d', "handle", "", 's', "key", "", 's', "val", ""), 
	EV_RETURNS_VOID, "Sets a GUI parameter." );
const idEventDef EV_GetGuiString( "getGuiString", EventArgs('d', "handle", "", 's', "key", ""), 's', "Returns a GUI parameter." );
const idEventDef EV_SetGuiFloat( "setGuiFloat", EventArgs('d', "handle", "", 's', "key", "", 'f', "val", ""), 
	EV_RETURNS_VOID, "Sets a GUI parameter." );
const idEventDef EV_GetGuiFloat( "getGuiFloat", EventArgs('d', "handle", "", 's', "key", ""), 'f', "Returns a GUI parameter." );
const idEventDef EV_SetGuiInt( "setGuiInt", EventArgs('d', "handle", "", 's', "key", "", 'd', "val", ""), 
	EV_RETURNS_VOID, "Sets a GUI parameter." );
const idEventDef EV_GetGuiInt( "getGuiInt", EventArgs('d', "handle", "", 's', "key", ""), 'f', "Returns a GUI parameter." );
const idEventDef EV_SetGuiStringFromKey( "setGuiStringFromKey", 
	EventArgs('d', "handle", "", 's', "key", "", 'e', "src", "", 's', "srcKey", ""),
	EV_RETURNS_VOID, 
	"This is a kludge. It is equivelant to: setGuiString( handle, key, src.getKey(srcKey) )\n" \
	"However, it's used to bypass the 127 char size limit on script strings." );
const idEventDef EV_CallGui( "callGui", EventArgs('d', "handle", "", 's', "namedEvent", ""), EV_RETURNS_VOID, "Calls a named event in a GUI.");
const idEventDef EV_CreateOverlay( "createOverlay", EventArgs('s', "guiFile", "", 'd', "layer", ""), 'd', "Creates a GUI overlay. (must be used on the player)" );
const idEventDef EV_DestroyOverlay( "destroyOverlay", EventArgs('d', "handle", ""), EV_RETURNS_VOID, "Destroys a GUI overlay. (must be used on the player)");
const idEventDef EV_LoadExternalData( "loadExternalData", EventArgs('s', "declFile", "", 's', "prefix", ""), 'd', "Load an external xdata declaration." );

// Obsttorte: #5976
const idEventDef EV_addFrobPeer("addFrobPeer", EventArgs('e', "peer", ""), EV_RETURNS_VOID, "Adds the passed entity as a frob peer.");
const idEventDef EV_removeFrobPeer("removeFrobPeer", EventArgs('e', "peer", ""), EV_RETURNS_VOID, "Removes the passed entity as a frob peer.");

const idEventDef EV_setFrobMaster("setFrobMaster", EventArgs('E', "master", ""), EV_RETURNS_VOID, "Sets the passed entity as the frob master. Pass null_entity to remove the current frob master, if any.");

//===============================================================
//                   TDM Inventory
//===============================================================
const idEventDef EV_GetLootAmount("getLootAmount", EventArgs('d', "type", "one of: LOOT_GOLD, LOOT_GOODS, LOOT_JEWELRY, LOOT_TOTAL"), 'd', 
	"Returns the amount of loot for the given type (e.g. LOOT_GOODS). Pass LOOT_TOTAL\n" \
	"to return the sum of all loot types.");
const idEventDef EV_ChangeLootAmount("changeLootAmount", EventArgs('d', "type", "one of: LOOT_GOLD, LOOT_GOODS, LOOT_JEWELRY", 'd', "amount", "can be negative"), 'd', 
	"Changes the loot amount of the given Type (e.g. GOODS) by <amount>. \n" \
	"The mission statisic for loot found gets changed too. \n"
	"The new value of the changed type is returned (e.g. the new GOODS value if this has been changed). \n" \
	"Note: The LOOT_TOTAL type can't be changed and 0 is returned.");

const idEventDef EV_AddInvItem("addInvItem", EventArgs('e', "inv_item", ""), EV_RETURNS_VOID, 
	"Adds the given item to the inventory. Depending on the type\n" \
	"the passed entity will be removed from the game (as for loot items) or hidden.");

const idEventDef EV_AddItemToInv("addItemToInv", EventArgs('e', "target", ""), EV_RETURNS_VOID, 
	"Adds the entity to the given entity's inventory. Depending on the type\n" \
	"the entity will be removed from the game (as for loot items) or hidden.\n" \
	"Example: $book->addItemToInv($player1);");				// Adds this entity to the inventory of the given entity (reversed AddInvItem)

const idEventDef EV_ReplaceInvItem("replaceInvItem", EventArgs('e', "oldItem", "", 'E', "newItem", "can be $null_entity"), 
	'd', 
	"Replaces the entity <oldItem> with <newItem> in the inventory,\n" \
	"while keeping <oldItem>'s inventory position intact.\n" \
	"\n" \
	"Note: The position guarantee only applies if <oldItem> and newItem \n" \
	"share the same category. If the categories are different, the position of <newItem>\n" \
	"is likely to be different than the one of <oldItem>.\n" \
	"\n" \
	"Note that <oldItem> will be removed from the inventory. \n" \
	"If <newItem> is the $null_entity, <oldItem> is just removed and no replacement happens.\n" \
	"\n" \
	"Returns 1 if the operation was successful, 0 otherwise.");	// olditem, newitem -> 1 if succeeded");
const idEventDef EV_HasInvItem("hasInvItem", EventArgs('e', "item", ""), 'd', "Tests whether the player has the specified item entity in his inventory.");
const idEventDef EV_GetNextInvItem("getNextInvItem", EventArgs(), 'e', 
	"Cycles the standard cursor to the next inventory item.\n" \
	"Returns the item entity pointed to after the operation is complete.");		// switches to the next inventory item
const idEventDef EV_GetPrevInvItem("getPrevInvItem", EventArgs(), 'e', 
	"Cycles the standard cursor to the previous inventory item.\n" \
	"Returns the item entity pointed to after the operation is complete.");		// switches to the previous inventory item
const idEventDef EV_ChangeInvItemCount("changeInvItemCount", 
	EventArgs('s', "name", "name of the item", 's', "category", "the item's category", 'd', "amount", ""), 
	EV_RETURNS_VOID, 
	"Decreases the inventory item stack count by amount. The item is addressed\n" \
	"using the name and category of the item. These are usually defined on the inventory\n" \
	"item entity (\"inv_name\", \"inv_category\")\n" \
	"\n" \
	"Amount can be both negative and positive.");		// Changes the stack count (call with "inv_name", "inv_category" and amount)

const idEventDef EV_ChangeInvLightgemModifier("changeInvLightgemModifier", 
	EventArgs('s', "name", "name of the item", 's', "category", "the item's category", 'd', "amount", ""), 
	EV_RETURNS_VOID, 
	"Sets the lightgem modifier value of the given item. Valid arguments are\n" \
	"between 0 and 32 (which is the maximum lightgem value)."); // Changes the lightgem modifier value of the given item.
const idEventDef EV_ChangeInvIcon("changeInvIcon", 
	EventArgs('s', "name", "name of the item", 's', "category", "the item's category", 's', "icon", ""),
	EV_RETURNS_VOID, "Sets the inventory icon of the given item in the given category to <icon>.");

const idEventDef EV_SetCurInvCategory("setCurInvCategory", EventArgs('s', "categoryName", ""), 'd', 
	"Sets the inventory cursor to the first item of the named category.\n" \
	"Returns 1 on success, 0 on failure (e.g. wrong category name)");	// category name -> 1 = success
const idEventDef EV_SetCurInvItem("setCurInvItem", EventArgs('s', "itemName", ""), 'e', 
	"Sets the inventory cursor to the named item.\n" \
	"Returns: the item entity of the newly selected item (can be $null_entity).");				// itemname -> entity
const idEventDef EV_GetCurInvCategory("getCurInvCategory", EventArgs(), 's', "Returns the name of the currently highlighted inventory category.");
const idEventDef EV_GetCurInvItemEntity("getCurInvItemEntity", EventArgs(), 'e', "Returns the currently highlighted inventory item entity.");
const idEventDef EV_GetCurInvItemName("getCurInvItemName", EventArgs(), 's', "Returns the name of the currently highlighted inventory item (the one defined in \"inv_name\").");
const idEventDef EV_GetCurInvItemId("getCurInvItemId", EventArgs(), 's', 
	"Returns the name of the currently highlighted inventory item (the one defined in \"inv_item_id\").\n" \
	"Most items will return an empty string, unless the \"inv_item_id\" is set on purpose.");
const idEventDef EV_GetCurInvIcon("getCurInvIcon", EventArgs(), 's', "Returns the icon of the currently highlighted inventory item.");
const idEventDef EV_GetCurInvItemCount("getCurInvItemCount", EventArgs(), 'd',
	"Returns the item count of the currently highlighted inventory Item, if stackable.Returns - 1 if non - stackable"); // Obsttorte #6096

// greebo: "Private" event which runs right after spawn time to check the inventory-related spawnargs.
const idEventDef EV_InitInventory("_initInventory", EventArgs('d', "", ""), EV_RETURNS_VOID, "Private event which runs right after spawn time to check the inventory-related spawnargs.");

// grayman #2478 - event which runs after spawn time to see if a mine is armed
const idEventDef EV_CheckMine("_checkMine", EventArgs(), EV_RETURNS_VOID, "Private event - runs after spawn time to see if a mine is armed");

// The Dark Mod Stim/Response interface functions for scripting
// Normally I don't like names, which are "the other way around"
// but I think in this case it would be ok, because the interface
// for stims and responses are pretty much the same.
const idEventDef EV_StimAdd( "StimAdd", EventArgs('d', "type", "", 'f', "radius", ""), EV_RETURNS_VOID, "");
const idEventDef EV_StimRemove( "StimRemove", EventArgs('d', "type", ""), EV_RETURNS_VOID, "");
const idEventDef EV_StimEnable( "StimEnable", EventArgs('d', "type", "", 'd', "state", "0 = disabled, 1 = enabled"), EV_RETURNS_VOID, "");
const idEventDef EV_StimClearIgnoreList( "StimClearIgnoreList", EventArgs('d', "type", ""), EV_RETURNS_VOID, 
	"This clears the ignore list for the stim of the given type\n" \
	"It can be used if an entity changes state in some way that it would no longer be ignored");
const idEventDef EV_StimEmit( "StimEmit", 
	EventArgs('d', "type", "Index ID of the stim to emit, i.e. 21 or STIM_TRIGGER for a trigger stim.",
			  'f', "radius", "How far the stim will reach. Pass negative to use the radius settings on the entity.",
			  'v', "stimOrigin", "Emit the stim from here."), EV_RETURNS_VOID, 
	"Emits a stim in a radius around the specified origin. The entity from which this event is called needs to have the corresponding stim setup on itself, it does not need to be active.");
const idEventDef EV_StimSetScriptBased( "StimSetScriptBased", 
	EventArgs('d', "type", "Index ID of the stim to alter, i.e. 21 or STIM_TRIGGER for a trigger stim.",
			  'd', "state", "New state."), EV_RETURNS_VOID, 
	"Converts a stim to being script-based. It will only be emitted when a script calls StimEmit on the entity.");
const idEventDef EV_ResponseEnable( "ResponseEnable", EventArgs('d', "type", "", 'd', "state", "0 = disabled, 1 = enabled"), EV_RETURNS_VOID, "");
const idEventDef EV_ResponseAdd( "ResponseAdd", EventArgs('d', "type", ""), EV_RETURNS_VOID, "");
const idEventDef EV_ResponseRemove( "ResponseRemove", EventArgs('d', "type", ""), EV_RETURNS_VOID, "");
const idEventDef EV_ResponseIgnore( "ResponseIgnore", EventArgs('d', "type", "", 'e', "responder", ""), EV_RETURNS_VOID, 
	"This functions must be called on the stim entity. It will add the response\n" \
	"to the ignore list, so that subsequent stims, should not trigger the stim anymore.");
const idEventDef EV_ResponseAllow( "ResponseAllow", EventArgs('d', "type", "", 'e', "responder", ""), EV_RETURNS_VOID, "");
const idEventDef EV_ResponseSetAction( "ResponseSetAction", EventArgs('d', "type", "", 's', "action", ""), EV_RETURNS_VOID, "");
const idEventDef EV_ResponseTrigger( "ResponseTrigger", EventArgs('e', "source", "", 'd', "stimType", ""), EV_RETURNS_VOID, 
	"Fires a response on this entity, without a stim (a stand-alone response, so to say)");
const idEventDef EV_GetResponseEntity( "GetResponseEntity", EventArgs(), 'e', 
	"Returns the entity which should take the response. Some entities like AI heads are not\n" \
	"responding themselves to stims, but relay it to another entity (i.e. the bodies they're attached to).");

const idEventDef EV_TimerCreate( "CreateTimer", 
	EventArgs('d', "stimId", "", 'd', "hour", "", 'd', "minutes", "", 'd', "seconds", "", 'd', "milliseconds", ""), EV_RETURNS_VOID, "");
const idEventDef EV_TimerStop( "StopTimer", EventArgs('d', "stimId", ""), EV_RETURNS_VOID, "");
const idEventDef EV_TimerStart( "StartTimer", EventArgs('d', "stimId", ""), EV_RETURNS_VOID, "");
const idEventDef EV_TimerRestart( "RestartTimer", EventArgs('d', "stimId", ""), EV_RETURNS_VOID, "");
const idEventDef EV_TimerReset( "ResetTimer", EventArgs('d', "stimId", ""), EV_RETURNS_VOID, "");
const idEventDef EV_TimerSetState( "SetTimerState", EventArgs('d', "stimId", "", 'd', "state", ""), EV_RETURNS_VOID, "");

// soundprop event: Propagate sound directly from scripting
const idEventDef EV_TDM_PropSoundMod( "propSoundMod", EventArgs('s', "name", "", 'f', "volMod", ""), EV_RETURNS_VOID, 
	"propagate a sound directly with a volume modifier");
// I don't think scripting supports optional argument, so I must do this
const idEventDef EV_TDM_PropSound( "propSound", EventArgs('s', "name", ""), EV_RETURNS_VOID, 
	"Sound propagation scriptfunctions on all entities\n" \
	"propagate a sound directly without playing an audible sound");

// For detecting ranged enemies. Returns nonzero if this entity could
// potentially attack the given entity (first parameter) at range.
const idEventDef EV_TDM_RangedThreatTo( "rangedThreatTo", EventArgs('e', "target", ""), 'f', 
	"Could this entity threaten the given (target) entity from a distance?" );

#ifdef MOD_WATERPHYSICS

const idEventDef EV_GetMass( "getMass", EventArgs('d', "body", "") , 'f', "Gets mass of a body for an entity" );
const idEventDef EV_IsInLiquid( "isInLiquid", EventArgs(), 'd', "Returns 1 if the entity is in or touching a liquid." );

#endif      // MOD_WATERPHYSICS

const idEventDef EV_CopyBind( "copyBind", EventArgs('e', "other", ""), EV_RETURNS_VOID, 
	"copy bind information of other to this entity\n" \
	"(i.e., bind this entity to the same entity that other is bound to)");
const idEventDef EV_IsFrobable( "isFrobable", EventArgs(), 'd', "Get whether the entity is frobable" );
const idEventDef EV_SetFrobable( "setFrobable", EventArgs('d', "frobable", ""), EV_RETURNS_VOID, "Set whether the entity is frobable");
const idEventDef EV_IsHilighted( "isHilighted", EventArgs(), 'd', "Returns true if entity is currently frobhilighted." );
const idEventDef EV_Frob("frob", EventArgs(), 'd', 
	"Frobs the entity (i.e. simulates a frob action performed by the player).\n" \
	"Returns TRUE if the entity is frobable, FALSE otherwise.");
const idEventDef EV_FrobHilight("frobHilight", EventArgs('d', "state", ""), EV_RETURNS_VOID, "ishtvan: Tries to make the entity frobhilight or not");

// greebo: Script event to check whether this entity can see a target entity
const idEventDef EV_CanSeeEntity("canSeeEntity", EventArgs('e', "target", "", 'd', "useLighting", ""), 'd', 
	"This is a general version of idAI::canSee, that can be used\n" \
	"by all entities. It doesn't regard FOV,\n" \
	"it just performs a trace to check whether the target is\n" \
	"occluded by world geometry. Is probably useful for stim/response as well\n" \
	"Pass useLighting = true to take the lighting of the target entity\n" \
	"into account. Use \"isEntityHidden\" as a script event with a\n" \
	"threshold.\n" \
	"The constant threshold value for useLighting is defined within the SDK in game/entity.h.");

const idEventDef EV_CanBeUsedBy("canBeUsedBy", EventArgs('e', "ent", ""), 'd', "Returns true if the entity can be used by the argument entity");

const idEventDef EV_CheckAbsence("checkAbsence", EventArgs(), EV_RETURNS_VOID, "description missing");

// greebo: TDM: Team accessor script events
const idEventDef EV_GetTeam("getTeam", EventArgs(), 'd', "Returns the current team number.");
const idEventDef EV_SetTeam("setTeam", EventArgs('d', "newTeam", ""), EV_RETURNS_VOID, "Sets the team number of this entity.");

const idEventDef EV_IsEnemy( "isEnemy", EventArgs('E', "ent", "The entity in question"), 'd', "Returns true if the given entity is an enemy.");
const idEventDef EV_IsFriend( "isFriend", EventArgs('E', "ent", "The entity in question"), 'd', "Returns true if the given entity is a friend.");
const idEventDef EV_IsNeutral( "isNeutral", EventArgs('E', "ent", "The entity in question"), 'd', "Returns true if the given entity is neutral.");

const idEventDef EV_SetEntityRelation( "setEntityRelation", EventArgs('E', "ent", "", 'd', "relation", ""), EV_RETURNS_VOID, 
	"Set a relation to another entity, this can be friendly (>0), neutral(0) or hostile (<0)");
const idEventDef EV_ChangeEntityRelation( "changeEntityRelation", EventArgs('E', "ent", "", 'd', "relationChange", ""), EV_RETURNS_VOID, 
	"This changes the current relation to an entity by adding the new amount.");

const idEventDef EV_PropagateSound( "propagateSound", EventArgs('s', "soundName", "", 'f', "propVolMod", "", 'd', "msgTag", ""), EV_RETURNS_VOID, "Generates a propagated sound" ); // grayman #3355
//const idEventDef EV_PropagateSound( "<propagateSound>", EventArgs('s', "soundName", "", 'f', "propVolMod", "", 'd', "msgTag", ""), EV_RETURNS_VOID, "Generates a propagated sound" ); // grayman #3355


ABSTRACT_DECLARATION( idClass, idEntity )
	EVENT( EV_Thread_SetRenderCallback,	idEntity::Event_WaitForRender )

	EVENT( EV_GetName,				idEntity::Event_GetName )
	EVENT( EV_SetName,				idEntity::Event_SetName )
	EVENT (EV_IsType,				idEntity::Event_IsType )
	EVENT( EV_FindTargets,			idEntity::Event_FindTargets )
	EVENT( EV_ActivateTargets,		idEntity::Event_ActivateTargets )
	EVENT( EV_AddTarget,			idEntity::Event_AddTarget)
	EVENT( EV_RemoveTarget,			idEntity::Event_RemoveTarget)
	EVENT( EV_NumTargets,			idEntity::Event_NumTargets )
	EVENT( EV_GetTarget,			idEntity::Event_GetTarget )
	EVENT( EV_RandomTarget,			idEntity::Event_RandomTarget )
	EVENT( EV_BindToJoint,			idEntity::Event_BindToJoint )
	EVENT( EV_BindToBody,			idEntity::Event_BindToBody )
	EVENT( EV_RemoveBinds,			idEntity::Event_RemoveBinds )
	EVENT( EV_Bind,					idEntity::Event_Bind )
	EVENT( EV_BindPosition,			idEntity::Event_BindPosition )
	EVENT( EV_Unbind,				idEntity::Event_Unbind )
	EVENT( EV_SpawnBind,			idEntity::Event_SpawnBind )
	EVENT( EV_GetBindMaster,		idEntity::Event_GetBindMaster )
	EVENT( EV_NumBindChildren,		idEntity::Event_NumBindChildren )
	EVENT( EV_GetBindChild,			idEntity::Event_GetBindChild )
	EVENT( EV_SetOwner,				idEntity::Event_SetOwner )
	EVENT( EV_SetModel,				idEntity::Event_SetModel )
	EVENT( EV_SetSkin,				idEntity::Event_SetSkin )
	EVENT( EV_ReskinCollisionModel,	idEntity::Event_ReskinCollisionModel ) // SteveL #4232
	EVENT( EV_GetShaderParm,		idEntity::Event_GetShaderParm )
	EVENT( EV_SetShaderParm,		idEntity::Event_SetShaderParm )
	EVENT( EV_SetShaderParms,		idEntity::Event_SetShaderParms )
	EVENT( EV_SetColor,				idEntity::Event_SetColor )
	EVENT( EV_SetColorVec,			idEntity::Event_SetColorVec )
	EVENT( EV_GetColor,				idEntity::Event_GetColor )
	EVENT( EV_SetHealth,			idEntity::Event_SetHealth )
	EVENT( EV_GetHealth,			idEntity::Event_GetHealth )
	EVENT( EV_SetMaxHealth,			idEntity::Event_SetMaxHealth )
	EVENT( EV_GetMaxHealth,			idEntity::Event_GetMaxHealth )
	EVENT( EV_IsHidden,				idEntity::Event_IsHidden )
	EVENT( EV_Hide,					idEntity::Event_Hide )
	EVENT( EV_Show,					idEntity::Event_Show )
	EVENT( EV_SetFrobActionScript,	idEntity::Event_SetFrobActionScript )
	EVENT( EV_SetUsedBy,			idEntity::Event_SetUsedBy )
	EVENT( EV_CacheSoundShader,		idEntity::Event_CacheSoundShader )
	EVENT( EV_StartSoundShader,		idEntity::Event_StartSoundShader )
	EVENT( EV_StartSound,			idEntity::Event_StartSound )
	EVENT( EV_StopSound,			idEntity::Event_StopSound )
	EVENT( EV_FadeSound,			idEntity::Event_FadeSound )
	EVENT( EV_SetSoundVolume,		idEntity::Event_SetSoundVolume )
	EVENT( EV_GetSoundVolume,		idEntity::Event_GetSoundVolume ) // grayman #3395
	EVENT( EV_GetWorldOrigin,		idEntity::Event_GetWorldOrigin )
	EVENT( EV_SetWorldOrigin,		idEntity::Event_SetWorldOrigin )
	EVENT( EV_GetOrigin,			idEntity::Event_GetOrigin )
	EVENT( EV_SetOrigin,			idEntity::Event_SetOrigin )
	EVENT( EV_GetAngles,			idEntity::Event_GetAngles )
	EVENT( EV_SetAngles,			idEntity::Event_SetAngles )
	EVENT( EV_GetLinearVelocity,	idEntity::Event_GetLinearVelocity )
	EVENT( EV_SetLinearVelocity,	idEntity::Event_SetLinearVelocity )
	EVENT( EV_GetAngularVelocity,	idEntity::Event_GetAngularVelocity )
	EVENT( EV_SetAngularVelocity,	idEntity::Event_SetAngularVelocity )
	EVENT( EV_SetGravity,			idEntity::Event_SetGravity )
	EVENT( EV_ApplyImpulse,			idEntity::Event_ApplyImpulse )

	EVENT( EV_SetContents,			idEntity::Event_SetContents )
	EVENT( EV_GetContents,			idEntity::Event_GetContents )
	EVENT( EV_SetClipMask,			idEntity::Event_SetClipMask )
	EVENT( EV_GetClipMask,			idEntity::Event_GetClipMask )
	EVENT( EV_SetSolid,				idEntity::Event_SetSolid )

	EVENT( EV_GetSize,				idEntity::Event_GetSize )
	EVENT( EV_SetSize,				idEntity::Event_SetSize )
	EVENT( EV_GetMins,				idEntity::Event_GetMins)
	EVENT( EV_GetMaxs,				idEntity::Event_GetMaxs )
	EVENT( EV_Touches,				idEntity::Event_Touches )
	EVENT( EV_GetNextKey,			idEntity::Event_GetNextKey )
	EVENT( EV_SetKey,				idEntity::Event_SetKey )
	EVENT( EV_GetKey,				idEntity::Event_GetKey )
	EVENT( EV_GetIntKey,			idEntity::Event_GetIntKey )
	EVENT( EV_GetBoolKey,			idEntity::Event_GetBoolKey )
	EVENT( EV_GetFloatKey,			idEntity::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,			idEntity::Event_GetVectorKey )
	EVENT( EV_GetEntityKey,			idEntity::Event_GetEntityKey )
	EVENT( EV_RemoveKey,			idEntity::Event_RemoveKey )
	EVENT( EV_RestorePosition,		idEntity::Event_RestorePosition )
	EVENT( EV_UpdateCameraTarget,	idEntity::Event_UpdateCameraTarget )
	EVENT( EV_DistanceTo,			idEntity::Event_DistanceTo )
	EVENT( EV_DistanceToPoint,		idEntity::Event_DistanceToPoint )
	EVENT( EV_StartFx,				idEntity::Event_StartFx )
	EVENT( EV_Thread_WaitFrame,		idEntity::Event_WaitFrame )
	EVENT( EV_Thread_Wait,			idEntity::Event_Wait )
	EVENT( EV_HasFunction,			idEntity::Event_HasFunction )
	EVENT( EV_CallFunction,			idEntity::Event_CallFunction )
	EVENT( EV_CallGlobalFunction,	idEntity::Event_CallGlobalFunction )
	EVENT( EV_SetNeverDormant,		idEntity::Event_SetNeverDormant )

	EVENT( EV_ExtinguishLights,		idEntity::Event_ExtinguishLights )

	EVENT( EV_InPVS,				idEntity::Event_InPVS )
	EVENT( EV_Damage,				idEntity::Event_Damage )
	EVENT( EV_Heal,					idEntity::Event_Heal )
	EVENT( EV_TeleportTo,			idEntity::Event_TeleportTo )
	EVENT( EV_IsDroppable,			idEntity::Event_IsDroppable )
	EVENT( EV_SetDroppable,			idEntity::Event_SetDroppable )
	EVENT( EV_GetLightInPVS,		idEntity::Event_GetLightInPVS )

	EVENT( EV_SetGui,				idEntity::Event_SetGui )
	EVENT( EV_GetGui,				idEntity::Event_GetGui )
	EVENT( EV_SetGuiString,			idEntity::Event_SetGuiString )
	EVENT( EV_GetGuiString,			idEntity::Event_GetGuiString )
	EVENT( EV_SetGuiFloat,			idEntity::Event_SetGuiFloat )
	EVENT( EV_GetGuiFloat,			idEntity::Event_GetGuiFloat )
	EVENT( EV_SetGuiInt,			idEntity::Event_SetGuiInt )
	EVENT( EV_GetGuiInt,			idEntity::Event_GetGuiInt )
	EVENT( EV_SetGuiStringFromKey,	idEntity::Event_SetGuiStringFromKey )
	EVENT( EV_CallGui,				idEntity::Event_CallGui )
	EVENT( EV_CreateOverlay,		idEntity::Event_CreateOverlay )
	EVENT( EV_DestroyOverlay,		idEntity::Event_DestroyOverlay )

	EVENT( EV_LoadExternalData,		idEntity::Event_LoadExternalData )

	// Obsttorte #5976 
	EVENT( EV_addFrobPeer,			idEntity::Event_AddFrobPeer ) 
	EVENT( EV_removeFrobPeer,		idEntity::Event_RemoveFrobPeer )
	EVENT( EV_setFrobMaster,		idEntity::Event_SetFrobMaster ) 

	EVENT( EV_GetLootAmount,		idEntity::Event_GetLootAmount )
	EVENT( EV_ChangeLootAmount,		idEntity::Event_ChangeLootAmount )
	EVENT( EV_AddInvItem,			idEntity::Event_AddInvItem )
	EVENT( EV_AddItemToInv,			idEntity::Event_AddItemToInv )
	EVENT( EV_ReplaceInvItem,		idEntity::Event_ReplaceInvItem )
	EVENT( EV_HasInvItem,			idEntity::Event_HasInvItem )
	EVENT( EV_GetNextInvItem,		idEntity::Event_GetNextInvItem )
	EVENT( EV_GetPrevInvItem,		idEntity::Event_GetPrevInvItem )
	EVENT( EV_SetCurInvCategory,	idEntity::Event_SetCurInvCategory )
	EVENT( EV_SetCurInvItem,		idEntity::Event_SetCurInvItem )
	EVENT( EV_GetCurInvCategory,	idEntity::Event_GetCurInvCategory )
	EVENT( EV_GetCurInvItemEntity,	idEntity::Event_GetCurInvItemEntity )
	EVENT( EV_GetCurInvItemName,	idEntity::Event_GetCurInvItemName )
	EVENT( EV_GetCurInvItemId,		idEntity::Event_GetCurInvItemId )
	EVENT( EV_GetCurInvIcon,		idEntity::Event_GetCurInvIcon )
	EVENT( EV_ChangeInvItemCount,	idEntity::ChangeInventoryItemCount )
	EVENT( EV_ChangeInvLightgemModifier, idEntity::ChangeInventoryLightgemModifier )
	EVENT( EV_ChangeInvIcon,		idEntity::ChangeInventoryIcon )
	EVENT( EV_GetCurInvItemCount,	idEntity::Event_GetCurInvItemCount )
	EVENT( EV_InitInventory,		idEntity::Event_InitInventory )

	EVENT( EV_StimAdd,				idEntity::Event_StimAdd)
	EVENT( EV_StimRemove,			idEntity::Event_StimRemove)
	EVENT( EV_StimEnable,			idEntity::Event_StimEnable)
	EVENT( EV_StimClearIgnoreList,	idEntity::Event_StimClearIgnoreList)
	EVENT( EV_StimEmit,				idEntity::Event_StimEmit)
	EVENT( EV_StimSetScriptBased,	idEntity::Event_StimSetScriptBased)
	EVENT( EV_ResponseEnable,		idEntity::Event_ResponseEnable)
	EVENT( EV_ResponseAdd,			idEntity::Event_ResponseAdd)
	EVENT( EV_ResponseRemove,		idEntity::Event_ResponseRemove)
	EVENT( EV_ResponseIgnore,		idEntity::Event_ResponseIgnore)
	EVENT( EV_ResponseAllow,		idEntity::Event_ResponseAllow)
	EVENT( EV_ResponseSetAction,	idEntity::Event_ResponseSetAction)
	EVENT( EV_ResponseTrigger,		idEntity::Event_ResponseTrigger)
	EVENT( EV_GetResponseEntity,	idEntity::Event_GetResponseEntity)

	EVENT( EV_TimerCreate,			idEntity::Event_TimerCreate )
	EVENT( EV_TimerStop,			idEntity::Event_TimerStop )
	EVENT( EV_TimerStart,			idEntity::Event_TimerStart )
	EVENT( EV_TimerRestart,			idEntity::Event_TimerRestart )
	EVENT( EV_TimerReset,			idEntity::Event_TimerReset )
	EVENT( EV_TimerSetState,		idEntity::Event_TimerSetState )

	EVENT( EV_TDM_PropSound,		idEntity::Event_PropSound )
	EVENT( EV_TDM_PropSoundMod,		idEntity::Event_PropSoundMod )

	EVENT( EV_TDM_RangedThreatTo,	idEntity::Event_RangedThreatTo )

#ifdef MOD_WATERPHYSICS

	EVENT( EV_GetMass,              idEntity::Event_GetMass )

	EVENT( EV_IsInLiquid,           idEntity::Event_IsInLiquid )

#endif		// MOD_WATERPHYSICS

	EVENT( EV_CopyBind,				idEntity::Event_CopyBind )
	EVENT( EV_IsFrobable,			idEntity::Event_IsFrobable )
	EVENT( EV_SetFrobable,			idEntity::Event_SetFrobable )
	EVENT( EV_IsHilighted,			idEntity::Event_IsHilighted )
	EVENT( EV_Frob,					idEntity::Event_Frob )
	EVENT( EV_FrobHilight,			idEntity::Event_FrobHilight )
	EVENT( EV_CanSeeEntity,			idEntity::Event_CanSeeEntity )
	EVENT( EV_CanBeUsedBy,			idEntity::Event_CanBeUsedBy )

	EVENT( EV_CheckAbsence,			idEntity::Event_CheckAbsence )
	
	EVENT (EV_GetTeam,				idEntity::Event_GetTeam )
	EVENT (EV_SetTeam,				idEntity::Event_SetTeam )

	EVENT(EV_IsEnemy,				idEntity::Event_IsEnemy )
	EVENT(EV_IsFriend,				idEntity::Event_IsFriend )
	EVENT(EV_IsNeutral,				idEntity::Event_IsNeutral )

	EVENT(EV_SetEntityRelation,		idEntity::Event_SetEntityRelation )
	EVENT(EV_ChangeEntityRelation,	idEntity::Event_ChangeEntityRelation )

	EVENT( EV_NoShadows,			idEntity::Event_noShadows )
	EVENT( EV_NoShadowsDelayed,		idEntity::Event_noShadowsDelayed )

	EVENT( EV_CheckMine,			idEntity::Event_CheckMine )			// grayman #2478
	EVENT( EV_GetVinePlantLoc,		idEntity::Event_GetVinePlantLoc )	// grayman #2478
	EVENT( EV_GetVinePlantNormal,	idEntity::Event_GetVinePlantNormal )// grayman #2478
	EVENT( EV_IsLight,				idEntity::Event_IsLight )			// grayman #2905
	EVENT( EV_ActivateContacts,		idEntity::Event_ActivateContacts )	// grayman #3011
	EVENT( EV_GetLocation,			idEntity::Event_GetLocation )		// grayman #3013
	EVENT( EV_GetEntityFlag,		idEntity::Event_GetEntityFlag)		// dragofer
	EVENT( EV_PropagateSound,		idEntity::Event_PropSoundDirect )	// grayman #3355
	
END_CLASS

/*
================
UpdateGuiParms
================
*/
void UpdateGuiParms( idUserInterface *gui, const idDict *args ) {
	if ( gui == NULL || args == NULL ) {
		return;
	}
	const idKeyValue *kv = args->MatchPrefix( "gui_parm", NULL );
	while( kv ) {
		// tels: Fix #3230
		gui->SetStateString( kv->GetKey(), common->Translate( kv->GetValue() ) );
		kv = args->MatchPrefix( "gui_parm", kv );
	}
	gui->SetStateBool( "noninteractive",  args->GetBool( "gui_noninteractive" ) ) ;
	gui->StateChanged( gameLocal.time );
}

/*
================
AddRenderGui
================
*/
void AddRenderGui( const char *name, idUserInterface **gui, const idDict *args ) {
	const idKeyValue *kv = args->MatchPrefix( "gui_parm", NULL );
	*gui = uiManager->FindGui( name, true, ( kv != NULL ) );
	UpdateGuiParms( *gui, args );
}

/*
================
idGameEdit::ParseSpawnArgsToAxis

stgatilov: common code to extract initial orientation from either "rotation" or "angle".
================
*/
void idGameEdit::ParseSpawnArgsToAxis( const idDict *args, idMat3 &axis ) {
	// get the rotation matrix in either full form, or single angle form
	if ( !args->GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", axis ) ) {
		float angle = args->GetFloat( "angle" );
		if ( angle != 0.0f ) {
			axis = idAngles( 0.0f, angle, 0.0f ).ToMat3();
		} else {
			axis.Identity();
		}
	}
}

/*
================
idGameEdit::ParseSpawnArgsToRenderEntity

parse the static model parameters
this is the canonical renderEntity parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderEntity( const idDict *args, renderEntity_t *renderEntity ) {
	int			i;
	const char	*temp;
	idVec3		color;
	const idDeclModelDef *modelDef;

	memset( renderEntity, 0, sizeof( *renderEntity ) );

	temp = args->GetString( "model" );

	modelDef = NULL;
	if ( temp[0] != '\0' ) {
		if ( !strstr( temp, "." ) ) {

			// FIXME: temp hack to replace obsolete ips particles with prt particles
			if ( !strstr( temp, ".ips" ) ) {
				idStr str = temp;
				str.Replace( ".ips", ".prt" );
				modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, str, false ) );
			} else {
				modelDef = static_cast<const idDeclModelDef *>( declManager->FindType( DECL_MODELDEF, temp, false ) );
			}
			if ( modelDef ) {
				renderEntity->hModel = modelDef->ModelHandle();
			}
		}

		if ( !renderEntity->hModel ) {
			renderEntity->hModel = renderModelManager->FindModel( temp );
		}
	}
	if ( renderEntity->hModel ) {
		renderEntity->bounds = renderEntity->hModel->Bounds( renderEntity );
	} else {
		renderEntity->bounds.Zero();
	}

	idStr rskin = args->GetString( "random_skin", "" ), skin;
	if ( !rskin.IsEmpty() ) {
		// found list, select a random skin
		skin = rskin.RandomPart( gameLocal.random.RandomFloat() ); // 4682 - need this object to live for a while
		temp = skin.c_str();
	} else
		temp = args->GetString( "skin" ); // just use the "skin" spawnarg

	if ( temp[0] != '\0' ) {
		renderEntity->customSkin = declManager->FindSkin( temp );
	} else if ( modelDef ) {
		renderEntity->customSkin = modelDef->GetDefaultSkin();
	}

	temp = args->GetString( "shader" );
	if ( temp[0] != '\0' ) {
		renderEntity->customShader = declManager->FindMaterial( temp );
	}

	args->GetVector( "origin", "0 0 0", renderEntity->origin );
	ParseSpawnArgsToAxis( args, renderEntity->axis );

	renderEntity->referenceSound = NULL;

	// get shader parms
	args->GetVector( "_color", "1 1 1", color );
	renderEntity->shaderParms[ SHADERPARM_RED ]		= color[0];
	renderEntity->shaderParms[ SHADERPARM_GREEN ]	= color[1];
	renderEntity->shaderParms[ SHADERPARM_BLUE ]	= color[2];
	renderEntity->shaderParms[ 3 ]					= args->GetFloat( "shaderParm3", "1" );
	renderEntity->shaderParms[ 4 ]					= args->GetFloat( "shaderParm4", "0" );
	renderEntity->shaderParms[ 5 ]					= args->GetFloat( "shaderParm5", "0" );
	renderEntity->shaderParms[ 6 ]					= args->GetFloat( "shaderParm6", "0" );
	renderEntity->shaderParms[ 7 ]					= args->GetFloat( "shaderParm7", "0" );
	renderEntity->shaderParms[ 8 ]					= args->GetFloat( "shaderParm8", "0" );
	renderEntity->shaderParms[ 9 ]					= args->GetFloat( "shaderParm9", "0" );
	renderEntity->shaderParms[ 10 ]					= args->GetFloat( "shaderParm10", "0" );
	renderEntity->shaderParms[ 11 ]	= args->GetFloat( "shaderParm11", "0" );

	// check noDynamicInteractions flag
	renderEntity->noDynamicInteractions = args->GetBool( "noDynamicInteractions" );
	
	// nbohr1more: #4379 lightgem culling
	renderEntity->isLightgem = args->GetBool( "islightgem" );
	
	//nbohr1more: #3662 noFog entity def
	renderEntity->noFog = args->GetBool( "noFog" );
	
	//nbohr1more: #4956 spectrum entity def
	renderEntity->spectrum = args->GetInt( "spectrum", "0" );
	
	renderEntity->lightspectrum = args->GetInt( "lightspectrum" );
	
	renderEntity->nospectrum = args->GetInt( "nospectrum" );

	// translucent sort order control
	renderEntity->sortOffset = args->GetInt( "drawSortOffset" );

	// check noshadows flag
	renderEntity->noShadow = args->GetBool( "noshadows" );

	// check noselfshadows flag
	renderEntity->noSelfShadow = args->GetBool( "noselfshadows" );
	
	// stgatilov #5172: hacky workaround for shadow-casting entities behind caulk
	renderEntity->forceShadowBehindOpaque = args->GetBool( "forceShadowBehindOpaque" );

	const char* areaLock;
	if ( args->GetString( "areaLock", "", &areaLock ) )
		renderEntity->areaLock = ( renderEntity_s::areaLock_t ) ( areaLockOptions.FindIndex( areaLock ) + 1 );
	
	if ( (args->GetInt( "spectrum" )  < 1 ) && ( args->GetInt( "lightspectrum" ) > 0 ) ) {
		renderEntity->spectrum = renderEntity->lightspectrum;
	}

	// init any guis, including entity-specific states
	for( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		temp = args->GetString( i == 0 ? "gui" : va( "gui%d", i + 1 ) );
		if ( temp[ 0 ] != '\0' ) {
			AddRenderGui( temp, &renderEntity->gui[ i ], args );
		}
	}
}

/*
================
idGameEdit::ParseSpawnArgsToRefSound

parse the sound parameters
this is the canonical refSound parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRefSound( const idDict *args, refSound_t *refSound ) {
	const char	*temp;

	memset( refSound, 0, sizeof( *refSound ) );

	auto ReadFloatParam = [&]( const char *spawnargName, int ssomFlag ) -> float {
		// stgatilov #6346: mark parameter as overriding only when spawnarg is present, nonempty and valid number
		// otherwise, parameter should not override anything (i.e. disabled)
		const char *text;
		float value;
		if ( args->GetString( spawnargName, "", &text ) && sscanf( text, "%f", &value ) == 1 ) {
			refSound->parms.overrideMode |= ssomFlag;
			return value;
		}
		return 0.0f;
	};

	refSound->parms.minDistance = ReadFloatParam( "s_mindistance", SSOM_MIN_DISTANCE_OVERRIDE );
	refSound->parms.maxDistance = ReadFloatParam( "s_maxdistance", SSOM_MAX_DISTANCE_OVERRIDE );
	refSound->parms.volume = ReadFloatParam( "s_volume", SSOM_VOLUME_OVERRIDE );
	refSound->parms.shakes = ReadFloatParam( "s_shakes", SSOM_SHAKES_OVERRIDE );

	args->GetVector( "origin", "0 0 0", refSound->origin );

	refSound->referenceSound  = NULL;

	// if a diversity is not specified, every sound start will make
	// a random one.  Specifying diversity is usefull to make multiple
	// lights all share the same buzz sound offset, for instance.
	refSound->diversity = args->GetFloat( "s_diversity", "-1" );
	refSound->waitfortrigger = args->GetBool( "s_waitfortrigger" );

	if ( args->GetBool( "s_omni" ) ) {
		refSound->parms.soundShaderFlags |= SSF_OMNIDIRECTIONAL;
	}
	if ( args->GetBool( "s_looping" ) ) {
		refSound->parms.soundShaderFlags |= SSF_LOOPING;
	}
	if ( args->GetBool( "s_occlusion" ) ) {
		refSound->parms.soundShaderFlags |= SSF_NO_OCCLUSION;
	}
	if ( args->GetBool( "s_global" ) ) {
		refSound->parms.soundShaderFlags |= SSF_GLOBAL;
	}
	if ( args->GetBool( "s_unclamped" ) ) {
		refSound->parms.soundShaderFlags |= SSF_UNCLAMPED;
	}
	refSound->parms.soundClass = args->GetInt( "s_soundClass" );
	refSound->parms.overrideMode |= SSOM_FLAGS_OR;

	temp = args->GetString( "s_shader" );
	if ( temp[0] != '\0' ) {
		refSound->shader = declManager->FindSound( temp );
	}
}

// ----------------------------------------------------------------

/*
===============
idEntity::UpdateChangeableSpawnArgs

Any key val pair that might change during the course of the game ( via a gui or whatever )
should be initialize here so a gui or other trigger can change something and have it updated
properly. An optional source may be provided if the values reside in an outside dictionary and
first need copied over to spawnArgs
===============
*/
void idEntity::UpdateChangeableSpawnArgs( const idDict *source ) {
	int i;
	const char *target;

	if ( !source ) {
		source = &spawnArgs;
	}
	cameraTarget = NULL;
	target = source->GetString( "cameraTarget" );
	if ( target && target[0] ) {
		// update the camera target
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		UpdateGuiParms( renderEntity.gui[ i ], source );
	}
}

/*
================
idEntity::idEntity
================
*/
idEntity::idEntity()
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX %s\r", this, __FUNCTION__);

	entityNumber	= ENTITYNUM_NONE;
	entityDefNumber = -1;

	spawnNode.SetOwner( this );
	activeIdx = -1;
	lodIdx = -1;

	snapshotNode.SetOwner( this );
	snapshotSequence = -1;
	snapshotBits = 0;

	thinkFlags		= 0;
	dormantStart	= 0;
	cinematic		= false;
	fromMapFile		= false;
	renderView		= NULL;
	cameraTarget	= NULL;
	cameraFovX		= 0;
	cameraFovY		= 0;

	health			= 0;
	maxHealth		= 0;

	m_droppedByAI	= false;	// grayman #1330

	m_isFlinder		= false;	// grayman #4230

	m_preHideContents		= -1; // greebo: initialise this to invalid values
	m_preHideClipMask		= -1;
	m_CustomContents		= -1;

	physics			= NULL;
	bindMaster		= NULL;
	bindJoint		= INVALID_JOINT;
	bindBody		= -1;
	teamMaster		= NULL;
	teamChain		= NULL;
	signals			= NULL;

	memset( PVSAreas, 0, sizeof( PVSAreas ) );
	numPVSAreas		= -1;

	memset( &fl, 0, sizeof( fl ) );
	fl.neverDormant	= true;			// most entities never go dormant

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	modelDefHandle	= -1;
	memset( &refSound, 0, sizeof( refSound ) );

	memset( &m_renderTrigger, 0, sizeof( m_renderTrigger ) );
	m_renderTrigger.axis.Identity();
	m_renderTriggerHandle = -1;
	m_renderWaitingThread = 0;

	mpGUIState = -1;

	m_SetInMotionByActor = NULL;
	m_MovedByActor = NULL;
	m_DroppedInLiquidByActor = NULL; // grayman #3774
	m_bFrobable = false;
	m_bFrobSimple = false;
	m_FrobDistance = 0;
	m_FrobBias = 1.0f;
	m_FrobBox = NULL;
	m_FrobActionScript = "";
	m_bFrobbed = false;
	m_bFrobHighlightState = false;
	m_FrobActionLock = false;
	m_bAttachedAlertControlsSolidity = false;
	m_bIsObjective = false;
	m_bIsClimbableRope = false;
	m_bIsMantleable = false;
	m_bIsBroken = false;
	m_bFlinderize = true;

	// We give all the entities a Stim/Response collection so that we wont have to worry
	// about the pointer being available all the time. The memory footprint of that 
	// object is rather small, so this doesn't really hurt that 
	// much and makes the code easier to control.
	m_StimResponseColl = new CStimResponseCollection;

	// Since we have a function to return inventories/etc, and many entities won't
	// have anything to do with inventories, I figure I'd better wait until
	// absolutely necessary to create these.
	m_Inventory			= CInventoryPtr();
	m_InventoryCursor	= CInventoryCursorPtr();

	m_LightQuotient = 0;
	m_LightQuotientLastEvalTime = -1;

	previousVoiceShader = nullptr;
	previousBodyShader = nullptr;

	// grayman #597 - for hiding arrows when nocked to the bow
	m_HideUntilTime = 0;

	// SteveL #3817. Make decals and overlays persistent.
	needsDecalRestore = false;

	memset( &xrayRenderEnt, 0, sizeof( xrayRenderEnt ) );

	xrayDefHandle = -1;
	xraySkin = NULL;
	xrayModelHandle = nullptr;

	m_VinePlantLoc = vec3_zero;
	m_VinePlantNormal = vec3_zero;
}

/*
================
idEntity::FixupLocalizedStrings
================
*/
void idEntity::FixupLocalizedStrings()
{
	// Tels: Transform here things like "inv_category" "Maps" back to "#str_02390" so that custom
	// entities in FMs with hard-coded english inventory categories or names still work even with
	// the new translation code.

	int c = 2;
	const char* todo[2] = { "inv_category", "inv_name" };

	for (int i = 0; i < c; i++)
	{
	    idStr spName = spawnArgs.GetString( todo[i], "");
		if (!spName.IsEmpty())
		{
			idStr strTemplate = common->GetI18N()->TemplateFromEnglish( spName );
			// "Maps" resulted in "#str_02390"?
			if (spName != strTemplate)
			{
				gameLocal.Printf("%s: Fixing %s from %s to %s.\n", GetName(), todo[i], spName.c_str(), strTemplate.c_str() );
    		    spawnArgs.Set( todo[i], strTemplate );
			}
		}
	}
}

static void ResolveRotationHack(idDict &spawnArgs) {
	const char *rotationStr = spawnArgs.GetString("rotation");
	idDict modelChanges;

	const idKeyValue *kv = spawnArgs.MatchPrefix( "model" );
	while( kv ) {
		const char *argName = kv->GetKey();
		idStr modelName = kv->GetValue();

		if ( !modelName.IsEmpty() ) {
			modelName.ToLower();

			idStr hashedStr = modelName + '#' + rotationStr;
			//quality of hash is rather important, since collision will put wrong model on entity
			uint64 hash = idStr::HashPoly64(hashedStr.c_str());
			char hex[20];
			sprintf(hex, "%016llx", hash);

			idStr baseName, extName;
			modelName.ExtractFileBase(baseName);
			modelName.ExtractFileExtension(extName);
			idStr proxyName = "models/_roth_gen/" + baseName + '_' + hex + ".proxy";

			modelChanges.Set(argName, proxyName);

			idStr intendedContents;
			intendedContents += "model \"" + modelName + "\"\n";
			intendedContents += "rotation \"" + idStr(rotationStr) + "\"\n";

			void *buffer = NULL;
			int len = fileSystem->ReadFile(proxyName, &buffer);
			bool alreadyGood = (len == intendedContents.Length() && memcmp(intendedContents.c_str(), buffer, len) == 0);
			if (len >= 0 && buffer)
				fileSystem->FreeFile(buffer);
			if (!alreadyGood)
				fileSystem->WriteFile(proxyName, intendedContents.c_str(), intendedContents.Length(), "fs_basepath", "");
		}

		kv = spawnArgs.MatchPrefix( "model", kv );
	}

	spawnArgs.Delete("rotation");
	spawnArgs.Copy(modelChanges);
}

/*
================
idEntity::Spawn
================
*/
void idEntity::Spawn( void )
{
	int					i;
	const char			*temp;
	idVec3				origin;
	idMat3				axis;
	const char			*classname;
	const char			*scriptObjectName;

	gameLocal.RegisterEntity( this );

	spawnArgs.GetString( "classname", NULL, &classname );
	const idDeclEntityDef *def = gameLocal.FindEntityDef( classname, false );
	if ( def ) {
		entityDefNumber = def->Index();
	}

	// every object will have a unique name
	temp = spawnArgs.GetString( "name", va( "%s_%s_%d", GetClassname(), spawnArgs.GetString( "classname" ), entityNumber));
	SetName(temp);
	DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("this: %08lX   Name: [%s]\r", this, temp);

	FixupLocalizedStrings();

	idMat3 rotation = spawnArgs.GetMatrix("rotation");
	if (!rotation.IsOrthogonal(1e-3f)) {
		//stgatilov #4970: embed bad rotation into models
		ResolveRotationHack(spawnArgs);
	}

	// parse static models the same way the editor display does
	gameEdit->ParseSpawnArgsToRenderEntity( &spawnArgs, &renderEntity );
	if (gameLocal.world && gameLocal.world->spawnArgs.GetBool("forceAllShadowsBehindOpaque"))
		renderEntity.forceShadowBehindOpaque = true;

	renderEntity.entityNum = entityNumber;
	
	xraySkin = NULL;
	xrayModelHandle = nullptr;
	renderEntity.xrayIndex = 1;

	idStr str;
	if ( spawnArgs.GetString( "skin_xray", "", str ) )
		xraySkin = declManager->FindSkin( str.c_str() );
	if ( spawnArgs.GetString( "model_xray", "", str ) ) {
		if ( str.Find(".") < 0 ) {
			auto modelDef = static_cast<const idDeclModelDef*>( declManager->FindType( DECL_MODELDEF, str.c_str(), false ) );
			if ( modelDef ) {
				xrayModelHandle = modelDef->ModelHandle();
			}
		}
		if ( !xrayModelHandle )
			xrayModelHandle = renderModelManager->FindModel( str );
	}
	if ( spawnArgs.GetString( "xray", "", str ) )
		renderEntity.xrayIndex = 3;

	// go dormant within 5 frames so that when the map starts most monsters are dormant
	dormantStart = gameLocal.time - DELAY_DORMANT_TIME + USERCMD_MSEC * 5;

	origin = renderEntity.origin;
	axis = renderEntity.axis;

	// do the audio parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRefSound( &spawnArgs, &refSound );

	// only play SCHANNEL_PRIVATE when sndworld->PlaceListener() is called with this listenerId
	// don't spatialize sounds from the same entity
	refSound.listenerId = entityNumber + 1;

	cameraTarget = NULL;
	temp = spawnArgs.GetString( "cameraTarget" );
	cameraFovX = spawnArgs.GetInt("cameraFovX", "120");
	cameraFovY = spawnArgs.GetInt("cameraFovY", "120");

	if ( temp && temp[0] ) {
		// update the camera target
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		UpdateGuiParms( renderEntity.gui[ i ], &spawnArgs );

		// Add the (potentially NULL) GUI as an external GUI to our overlaysys,
		// so that the script functions to interact with GUIs can interact with it.
		if ( m_overlays.createOverlay( 0, OVERLAYS_MIN_HANDLE + i ) >= OVERLAYS_MIN_HANDLE )
			m_overlays.setGui( OVERLAYS_MIN_HANDLE + i, renderEntity.gui[ i ] );
		else
			gameLocal.Warning( "Unable to create overlay for renderentity GUI: %d", OVERLAYS_MIN_HANDLE + i );
	}

	fl.solidForTeam = spawnArgs.GetBool( "solidForTeam", "0" );
	fl.neverDormant = spawnArgs.GetBool( "neverDormant", "0" );
	fl.hidden = spawnArgs.GetBool( "hide", "0" );
	if ( fl.hidden ) {
		// make sure we're hidden, since a spawn function might not set it up right
		PostEventMS( &EV_Hide, 0 );
	}
	cinematic = spawnArgs.GetBool( "cinematic", "0" );

	bool haveTargets = false; // grayman #2603
	// if we have targets, wait until all entities are spawned to get them
	if ( spawnArgs.MatchPrefix( "target" ) || spawnArgs.MatchPrefix( "guiTarget" ) )
	{
		haveTargets = true;
		if ( gameLocal.GameState() == GAMESTATE_STARTUP ) {
			PostEventMS( &EV_FindTargets, 0 );
		} else {
			// not during spawn, so it's ok to get the targets
			FindTargets();
		}
	}

	// if we're attaching relight positions, they have to be added as targets,
	// so we have to wait until all entities are spawned to do that

	health = spawnArgs.GetInt( "health" );
	InitDefaultPhysics( origin, axis );

	// TDM: Set custom contents, and store it so it doesn't get overwritten
	m_CustomContents = spawnArgs.GetInt("clipmodel_contents","-1");
	if( m_CustomContents != -1 )
		GetPhysics()->SetContents( m_CustomContents );

	SetOrigin( origin );
	SetAxis( axis );

	// load visual model and broken model 
	LoadModels();

	if ( spawnArgs.GetString( "bind", "", &temp ) ) 
	{
		PostEventMS( &EV_SpawnBind, 0 );
	}

	//TDM: Parse list of pre-defined attachment positions from spawnargs
	ParseAttachPositions();

	//TDM: Parse and spawn and attach any attachments
	ParseAttachments();

	// grayman #2603 - if we spawned relight positions, add them to the target list

	if (spawnArgs.MatchPrefix("relight_position"))
	{
		if (haveTargets)
		{
			if (gameLocal.GameState() != GAMESTATE_STARTUP)
			{
				// not during spawn, so it's ok to add targets from the relight positions now
				FindRelights();
			}
		}
		else
		{
			targets.Clear();
			// it's ok to create targets from the relight positions now
			FindRelights();
		}
	}

	// auto-start a sound on the entity
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_CHANNEL_ANY, 0, false, NULL );
	}

	// setup script object
	if ( ShouldConstructScriptObjectAtSpawn() && spawnArgs.GetString( "scriptobject", NULL, &scriptObjectName ) ) {
		if ( !scriptObject.SetType( scriptObjectName ) ) {
			gameLocal.Error( "Script object '%s' not found on entity '%s'.", scriptObjectName, name.c_str() );
		}

		ConstructScriptObject();
	}

	m_StimResponseColl->InitFromSpawnargs(spawnArgs, this);

	// greebo: Post the inventory check event. If this entity should be added
	// to someone's inventory, the event takes care of that. This must happen
	// after the entity has fully spawned (including subclasses), otherwise
	// the clipmodel and things like that are not initialised (>> crash).

	// grayman #2820 - don't queue EV_InitInventory if it's going to
	// end up doing nothing. Queuing this for every entity causes a ton
	// of event posting during frame 0 that can overrun the event queue limit
	// in large maps w/lots of entities.

	int lootValue = spawnArgs.GetInt( "inv_loot_value", "0" );
	int lootType = spawnArgs.GetInt( "inv_loot_type", "0" );
	bool belongsInInventory = spawnArgs.GetBool( "inv_map_start", "0" );

	// Only post EV_InitInventory if this is loot OR it belongs in someone's inventory.
	// If extra work is added to EV_InitInventory, then that must be accounted for here, to make sure it has
	// a chance of getting done.

	if ( belongsInInventory || ( ( lootType > LOOT_NONE ) && ( lootType < LOOT_COUNT ) && ( lootValue != 0 ) ) )
	{
		PostEventMS( &EV_InitInventory, 0, 0 );
	}

	// grayman #2478 - If this is a mine or flashmine, it can be armed
	// at spawn time. Post an event to replace the mine with its
	// projectile counterpart, which handles arming/disarming. Wait a
	// bit before replacing so the mine has time to settle.

	if ( idStr::Cmp(spawnArgs.GetString("toolclass"), "mine") == 0 )
	{
		PostEventSec(&EV_CheckMine, 1.0f);
	}

	LoadTDMSettings();

	if (m_AbsenceNoticeability > 0)
	{
		PostEventMS(&EV_CheckAbsence, gameLocal.random.RandomFloat() * 5000);
	}
	m_StartBounds = GetPhysics()->GetAbsBounds();
	m_AbsenceStatus = false;

	// parse LOD spawnargs
	LodComponent lodComp;
	if ( lodComp.ParseLODSpawnargs( this, &spawnArgs, gameLocal.random.RandomFloat() ) ) {
		// Have to start thinking if we're distance dependent
		gameLocal.lodSystem.AddToEnd( lodComp );
	}

	// init most recent voice and body sound data for AI - grayman #2341
	previousVoiceShader = NULL;	// shader for the most recent voice request
	previousVoiceIndex = 0;		// index of most recent voice sound requested (1->N, where there are N sounds)
	previousBodyShader = NULL;	// shader for the most recent body sound request
	previousBodyIndex = 0;		// index of most recent body sound requested (1->N, where there are N sounds)

	m_LastRestPos = idVec3(sqrt(idMath::INFINITY)); // grayman #3992

	m_pushedBy = NULL;		// grayman #4603
	m_splashtime = 0;		// grayman #4600
	m_listening = false;	// grayman #4620
}

/*
================
idEntity::LoadModels
================
*/
// tels: this routine loads all the visual and collision models
void idEntity::LoadModels()
{
	idStr model;
	// we only use a brokenModel if we can find one automatically
	bool needBroken = false;

	DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Loading models for entity %d (%s)\r", entityNumber, name.c_str() ); 

	// load the normal visual model
	spawnArgs.GetString( "model", "", model );

	if ( !model.IsEmpty() ) {
		SetModel( model );		
	}

	// was a brokenModel requested?
	spawnArgs.GetString( "broken", "", brokenModel );

	// see if we need to create a broken model name
	if ( !brokenModel.IsEmpty() )
	{
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Need broken model '%s' for entity %d (%s)\r", brokenModel.c_str(), entityNumber, name.c_str() );
		// spawnarg "broken" was set, so we need the broken model
		needBroken = true;
	}
	// only check for an auto-broken model if the entity could be damaged 
	else if ( health || spawnArgs.FindKey( "max_force" ) )
	{
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Looking for broken models for entity %d (%s) (health: %d)\r", entityNumber, name.c_str(), health );

		int pos = model.Find(".");

		if ( pos < 0 )
		{
			pos = model.Length();
		}

		if ( pos > 0 )
		{
			model.Left( pos, brokenModel );
		}

		brokenModel += "_broken";

		if ( pos > 0 )
		{
			brokenModel += &model[ pos ];
		}
	}

	// greebo: Only try to cache the model if it actually exists -> otherwise tons of false warnings get emitted
	bool brokenModelFileExists = brokenModel.Length() > 0 && (fileSystem->FindFile(brokenModel) != FIND_NO);

	if (brokenModelFileExists)
	{
		// check brokenModel to exist, and make sure the brokenModel gets cached
		if ( !renderModelManager->CheckModel( brokenModel ) )
		{
			if ( needBroken ) {
				gameLocal.Error( "Broken model '%s' not loadable for entity %d (%s)", brokenModel.c_str(), entityNumber, name.c_str() );
			} 
			else {
				// couldn't find automatically generated "model_broken.lwo", so don't use brokenModel at all
				brokenModel = "";
			}
		}
	}
	else
	{
		// Broken model file does not exist
		if ( needBroken ) {
			gameLocal.Error( "Broken model '%s' required for entity %d (%s)", brokenModel.c_str(), entityNumber, name.c_str() );
		} 
		// set brokenModel to empty, so we later don't try to use it
		brokenModel = "";
	}

	// can we be damaged?
	if ( health || spawnArgs.FindKey( "max_force" ) || !brokenModel.IsEmpty() )
	{
		fl.takedamage = true;
	}

	if ( brokenModelFileExists && !brokenModel.IsEmpty() )
	{
		if ( model.IsEmpty() )
		{
			gameLocal.Error( "Breakable entity (broken model %s) without a model set on entity #%d (%s)", brokenModel.c_str(), entityNumber, name.c_str() );
		}

		// make sure the collision model for the brokenModel gets cached
        idClipModel::CheckModel( brokenModel );

		// since we can be damaged, setup some physics
/*
* Ishtvan: Commented out, this is handled separately and this call can interfere with other places contents are set
*
		GetPhysics()->SetContents( !spawnArgs.GetBool( "solid" ) ? 0 : CONTENTS_SOLID );
	    // SR CONTENTS_RESONSE FIX
		if( m_StimResponseColl->HasResponse() ) {
			GetPhysics()->SetContents( GetPhysics()->GetContents() | CONTENTS_RESPONSE );
		}
**/

	} // end of loading of broken model(s) and their CM


}

/*
================
idEntity::~idEntity
================
*/
idEntity::~idEntity( void )
{
	DM_LOG(LC_FUNCTION, LT_DEBUG)LOGSTRING("this: %08lX [%s]\r", this, __FUNCTION__);

	// Tels: #2430 - If this entity is shouldered by the player, dequip it forcefully
	//stgatilov: in case of save loading error, grabber may not exist yet
	if (gameLocal.m_Grabber )
	{
		if ( gameLocal.m_Grabber->GetEquipped() == this ) 
		{
			if ( spawnArgs.GetBool("shoulderable") )
			{
				gameLocal.Printf("Grabber: Forcefully unshouldering %s because it will be removed.\n", GetName() );
				gameLocal.m_Grabber->UnShoulderBody(this);
			}
			gameLocal.Printf("Grabber: Forcefully dequipping %s because it will be removed.\n", GetName() );
			gameLocal.m_Grabber->Forget(this);
		}
		else if ( gameLocal.m_Grabber->GetSelected() == this )
		{
			//nbohr1more #1084: ensure grabber forgets held entities on removal
			gameLocal.m_Grabber->Forget(this);
		}
	}

	// Let each objective entity we're currently in know about our destruction
	for (int i = 0; i < m_objLocations.Num(); ++i)
	{
		CObjectiveLocation* locationEnt = m_objLocations[i].GetEntity();

		if (locationEnt == NULL) continue; // probably already deleted

		locationEnt->OnEntityDestroyed(this);
	}

	gameLocal.RemoveResponse(this);
	gameLocal.RemoveStim(this);

	DeconstructScriptObject();
	scriptObject.Free();

	if ( thinkFlags ) {
		BecomeInactive( thinkFlags );
	}
	gameLocal.activeEntities.Remove( this );

	Signal( SIG_REMOVED );

	// we have to set back the default physics object before unbinding because the entity
	// specific physics object might be an entity variable and as such could already be destroyed.
	SetPhysics( NULL );

	// remove any entities that are bound to me
	RemoveBinds( false );

	// unbind from master
	Unbind();

	// sometimes RemoveBinds add this entity back to active list
	// so try to remove it again to avoid dangling pointer
	gameLocal.activeEntities.Remove( this );

	gameLocal.RemoveEntityFromHash( name.c_str(), this );

	delete renderView;
	renderView = NULL;

	delete signals;
	signals = NULL;

	// free optional LOD data
	LodComponent::StopLOD( this, false );

	FreeModelDef();
	FreeSoundEmitter( false );

	if ( xrayDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( xrayDefHandle );
		xrayDefHandle = -1;
	}

	if ( m_renderTriggerHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( m_renderTriggerHandle );
	}

	gameLocal.UnregisterEntity( this );
	gameLocal.RemoveStim(this);
	gameLocal.RemoveResponse(this);
	delete m_StimResponseColl;

	if( m_FrobBox )
		delete m_FrobBox;

	m_FrobPeers.Clear();
}

void idEntity::AddObjectsToSaveGame(idSaveGame* savefile)
{
	// empty default implementation
}

/*
================
idEntity::Save
================
*/
void idEntity::Save( idSaveGame *savefile ) const
{
	int i, j;

	savefile->WriteInt( entityNumber );
	savefile->WriteInt( entityDefNumber );

	// spawnNode and activeNode are restored by gameLocal

	savefile->WriteInt( snapshotSequence );
	savefile->WriteInt( snapshotBits );

	savefile->WriteDict( &spawnArgs );
	savefile->WriteString( name );
	scriptObject.Save( savefile );

	savefile->WriteInt( thinkFlags );
	savefile->WriteInt( dormantStart );
	savefile->WriteBool( cinematic );
	savefile->WriteBool( fromMapFile );

	savefile->WriteObject( cameraTarget );
	savefile->WriteInt( cameraFovX );
	savefile->WriteInt( cameraFovY );
	
	savefile->WriteInt( health );
	savefile->WriteInt( maxHealth );

	savefile->WriteInt( m_preHideContents );
	savefile->WriteInt( m_preHideClipMask );
	savefile->WriteInt( m_CustomContents );

	savefile->WriteInt( targets.Num() );
	for( i = 0; i < targets.Num(); i++ ) {
		targets[ i ].Save( savefile );
	}

	entityFlags_s flags = fl;

	LittleBitField( &flags, sizeof( flags ) );

	savefile->Write( &flags, sizeof( flags ) );

	savefile->WriteInt(m_UsedByName.Num());
	for (i = 0; i < m_UsedByName.Num(); i++)
	{
		savefile->WriteString(m_UsedByName[i].c_str());
	}
	savefile->WriteInt(m_UsedByInvName.Num());
	for (i = 0; i < m_UsedByInvName.Num(); i++)
	{
		savefile->WriteString(m_UsedByInvName[i].c_str());
	}
	savefile->WriteInt(m_UsedByCategory.Num());
	for (i = 0; i < m_UsedByCategory.Num(); i++)
	{
		savefile->WriteString(m_UsedByCategory[i].c_str());
	}
	savefile->WriteInt(m_UsedByClassname.Num());
	for (i = 0; i < m_UsedByClassname.Num(); i++)
	{
		savefile->WriteString(m_UsedByClassname[i].c_str());
	}

	savefile->WriteBool(m_bAttachedAlertControlsSolidity);
	savefile->WriteBool(m_bIsObjective);

	savefile->WriteInt(m_objLocations.Num());
	for (i = 0; i < m_objLocations.Num(); ++i)
	{
		m_objLocations[i].Save(savefile);
	}

	savefile->WriteBool(m_bFrobable);
	savefile->WriteBool(m_bFrobSimple);
	savefile->WriteInt(m_FrobDistance);
	savefile->WriteFloat(m_FrobBias);
	savefile->WriteClipModel(m_FrobBox);

	savefile->WriteBool(m_bIsClimbableRope);

	savefile->WriteInt(m_animRates.Num());
	for (i = 0; i < m_animRates.Num(); i++)
	{
		savefile->WriteFloat(m_animRates[i]);
	}

	m_SetInMotionByActor.Save(savefile);
	m_MovedByActor.Save(savefile);
	m_DroppedInLiquidByActor.Save(savefile); // grayman #3774

	savefile->WriteBool(m_bFrobbed);
	savefile->WriteBool(m_bFrobHighlightState);

	savefile->WriteString(m_FrobActionScript);

	savefile->WriteInt(m_FrobPeers.Num());
	for (i = 0; i < m_FrobPeers.Num(); i++)
	{
		savefile->WriteString(m_FrobPeers[i]);
	}

	savefile->WriteString(m_MasterFrob);
	savefile->WriteBool(m_FrobActionLock);

	savefile->WriteFloat(m_AbsenceNoticeability);
	savefile->WriteBounds(m_StartBounds);
	savefile->WriteBool(m_AbsenceStatus);
	
	m_AbsenceMarker.Save(savefile);

	savefile->WriteInt(team);

    savefile->WriteInt(static_cast<int>(m_EntityRelations.size()));
	for (EntityRelationsMap::const_iterator i = m_EntityRelations.begin(); i != m_EntityRelations.end(); ++i)
	{
		savefile->WriteObject(i->first);
		savefile->WriteInt(i->second);
	}

	savefile->WriteBool( m_bIsMantleable );

	savefile->WriteBool( m_bIsBroken );
	savefile->WriteString( brokenModel );

	m_StimResponseColl->Save(savefile);

	savefile->WriteRenderEntity( xrayRenderEnt );
	savefile->WriteInt( xrayDefHandle );
	savefile->WriteSkin( xraySkin );

	savefile->WriteRenderEntity( renderEntity );
	savefile->WriteInt( modelDefHandle );
	savefile->WriteRefSound( refSound );

	savefile->WriteRenderEntity( m_renderTrigger );
	savefile->WriteInt( m_renderTriggerHandle );
	savefile->WriteInt( m_renderWaitingThread );

	m_overlays.Save( savefile );

	savefile->WriteObject( bindMaster );
	savefile->WriteJoint( bindJoint );
	savefile->WriteInt( bindBody );
	savefile->WriteObject( teamMaster );
	savefile->WriteObject( teamChain );

	savefile->WriteStaticObject( defaultPhysicsObj );

	savefile->WriteInt( numPVSAreas );
	for( i = 0; i < MAX_PVS_AREAS; i++ ) {
		savefile->WriteInt( PVSAreas[ i ] );
	}

	if ( !signals ) {
		savefile->WriteBool( false );
	} else {
		savefile->WriteBool( true );
		for( i = 0; i < NUM_SIGNALS; i++ ) {
			savefile->WriteInt( signals->signal[ i ].Num() );
			for( j = 0; j < signals->signal[ i ].Num(); j++ ) {
				savefile->WriteInt( signals->signal[ i ][ j ].threadnum );
				savefile->WriteString( signals->signal[ i ][ j ].function->Name() );
			}
		}
	}

	savefile->WriteInt( mpGUIState );

	savefile->WriteBool(m_Inventory != NULL);
	if (m_Inventory != NULL) {
		m_Inventory->Save(savefile);
	}

	savefile->WriteBool(m_InventoryCursor != NULL);
	if (m_InventoryCursor != NULL) {
		// Save the ID of the cursor
		savefile->WriteInt(m_InventoryCursor->GetId());
	}

	savefile->WriteInt( m_Attachments.Num() );
	for( int i=0; i < m_Attachments.Num(); i++ )
		m_Attachments[i].Save( savefile );

    savefile->WriteInt(static_cast<int>(m_AttNameMap.size()));
	for ( AttNameMap::const_iterator k = m_AttNameMap.begin();
         k != m_AttNameMap.end(); ++k )
    {
        savefile->WriteString( k->first.c_str() );
        savefile->WriteInt( k->second );
    }

	savefile->WriteInt( m_AttachPositions.Num() );
	for ( i = 0; i < m_AttachPositions.Num(); i++ ) 
	{
		m_AttachPositions[i].Save( savefile );
	}

	m_userManager.Save(savefile);

	savefile->WriteFloat(m_LightQuotient);
	savefile->WriteInt(m_LightQuotientLastEvalTime);

	savefile->WriteBool(m_droppedByAI);		// grayman #1330

	savefile->WriteBool(m_isFlinder);		// grayman #4230

	// grayman #2341 - don't save previous voice and body shaders and indices,
	// since they're irrelevant across saved games

	// removed, since there's no point in saving variables that aren't necessary

	//savefile->WriteSoundShader(NULL);	// previousVoiceShader
	//savefile->WriteInt(0);			// previousVoiceIndex
	//savefile->WriteSoundShader(NULL);	// previousBodyShader
	//savefile->WriteInt(0);			// previousBodyIndex

	// grayman #597

	savefile->WriteInt(m_HideUntilTime); // arrow-hiding timer

	// grayman #2787
	savefile->WriteVec3( m_VinePlantLoc );
	savefile->WriteVec3( m_VinePlantNormal );

	savefile->WriteVec3( m_LastRestPos ); // grayman #3992

	m_pushedBy.Save( savefile ); // grayman #4603

	savefile->WriteInt(m_splashtime); // grayman #4600
	savefile->WriteBool(m_listening); // grayman #4620

	// SteveL #3817: make decals persistent
    savefile->WriteInt(static_cast<int>(decals_list.size()));
	for ( std::list<SDecalInfo>::const_iterator i = decals_list.begin(); i != decals_list.end(); ++i )
	{
		savefile->WriteString( i->decal );
		savefile->WriteVec3( i->origin );
		savefile->WriteVec3( i->dir );
		savefile->WriteFloat( i->size );
		savefile->WriteJoint( i->overlay_joint );
		savefile->WriteInt( i->decal_starttime );
		savefile->WriteFloat( i->decal_depth );
		savefile->WriteBool( i->decal_parallel );
		savefile->WriteFloat( i->decal_angle );
	}
}

/*
================
idEntity::Restore
================
*/
void idEntity::Restore( idRestoreGame *savefile )
{
	int			i, j;
	int			num;
	idStr		funcname;

	savefile->ReadInt( entityNumber );
	savefile->ReadInt( entityDefNumber );

	// spawnNode and activeNode are restored by gameLocal

	savefile->ReadInt( snapshotSequence );
	savefile->ReadInt( snapshotBits );

	savefile->ReadDict( &spawnArgs );
	savefile->ReadString( name );
	SetName( name );

	scriptObject.Restore( savefile );

	savefile->ReadInt( thinkFlags );
	savefile->ReadInt( dormantStart );
	savefile->ReadBool( cinematic );
	savefile->ReadBool( fromMapFile );

	savefile->ReadObject( reinterpret_cast<idClass *&>( cameraTarget ) ); 

	if (cameraTarget)
	{
		// // grayman #4615 - update the camera target (will handle a NULL "cameraTarget")
		PostEventMS( &EV_UpdateCameraTarget, 0 );
	}

	savefile->ReadInt( cameraFovX );
	savefile->ReadInt( cameraFovY );

	savefile->ReadInt( health );
	savefile->ReadInt( maxHealth );

	savefile->ReadInt( m_preHideContents );
	savefile->ReadInt( m_preHideClipMask );
	savefile->ReadInt( m_CustomContents );

	targets.Clear();
	savefile->ReadInt( num );
	targets.SetNum( num );
	for( i = 0; i < num; i++ ) {
		targets[ i ].Restore( savefile );
	}

	savefile->Read( &fl, sizeof( fl ) );
	LittleBitField( &fl, sizeof( fl ) );

	savefile->ReadInt(num);
	m_UsedByName.SetNum(num);
	for (i = 0; i < num; i++)
	{
		savefile->ReadString(m_UsedByName[i]);
	}
	savefile->ReadInt(num);
	m_UsedByInvName.SetNum(num);
	for (i = 0; i < num; i++)
	{
		savefile->ReadString(m_UsedByInvName[i]);
	}
	savefile->ReadInt(num);
	m_UsedByCategory.SetNum(num);
	for (i = 0; i < num; i++)
	{
		savefile->ReadString(m_UsedByCategory[i]);
	}
	savefile->ReadInt(num);
	m_UsedByClassname.SetNum(num);
	for (i = 0; i < num; i++)
	{
		savefile->ReadString(m_UsedByClassname[i]);
	}

	savefile->ReadBool(m_bAttachedAlertControlsSolidity);
	savefile->ReadBool(m_bIsObjective);

	savefile->ReadInt(num);
	m_objLocations.SetNum(num);
	for (i = 0; i < num; ++i)
	{
		m_objLocations[i].Restore(savefile);
	}

	savefile->ReadBool(m_bFrobable);
	savefile->ReadBool(m_bFrobSimple);
	savefile->ReadInt(m_FrobDistance);
	savefile->ReadFloat(m_FrobBias);
	savefile->ReadClipModel(m_FrobBox);

	savefile->ReadBool(m_bIsClimbableRope);

	savefile->ReadInt(num);
	m_animRates.SetNum(num);
	for (i = 0; i < num; i++)
	{
		savefile->ReadFloat(m_animRates[i]);
	}

	m_SetInMotionByActor.Restore(savefile);
	m_MovedByActor.Restore(savefile);
	m_DroppedInLiquidByActor.Restore(savefile); // grayman #3774

	savefile->ReadBool(m_bFrobbed);
	savefile->ReadBool(m_bFrobHighlightState);

	savefile->ReadString(m_FrobActionScript);

	savefile->ReadInt(num);
	m_FrobPeers.SetNum(num);
	for (i = 0; i < num; i++)
	{
		savefile->ReadString(m_FrobPeers[i]);
	}

	savefile->ReadString(m_MasterFrob);
	savefile->ReadBool(m_FrobActionLock);

	savefile->ReadFloat(m_AbsenceNoticeability);
	savefile->ReadBounds(m_StartBounds);
	savefile->ReadBool(m_AbsenceStatus);
	
	m_AbsenceMarker.Restore(savefile);

	savefile->ReadInt(team);

	savefile->ReadInt(num);
	for (int i = 0; i < num; ++i)
	{
		idEntity* entity;
		savefile->ReadObject(reinterpret_cast<idClass*&>(entity));

		int relation;
		savefile->ReadInt(relation);

		m_EntityRelations[entity] = relation;
	}


	savefile->ReadBool(m_bIsMantleable);

	savefile->ReadBool( m_bIsBroken );
	savefile->ReadString( brokenModel );

	m_StimResponseColl->Restore(savefile);

	savefile->ReadRenderEntity( xrayRenderEnt );
	xrayModelHandle = xrayRenderEnt.hModel;
	savefile->ReadInt( xrayDefHandle );
	if ( xrayDefHandle != -1 )
		xrayDefHandle = gameRenderWorld->AddEntityDef( &xrayRenderEnt );
	savefile->ReadSkin( xraySkin );

	savefile->ReadRenderEntity( renderEntity );
	savefile->ReadInt( modelDefHandle );
	savefile->ReadRefSound( refSound );

	savefile->ReadRenderEntity( m_renderTrigger );
	savefile->ReadInt( m_renderTriggerHandle );
	savefile->ReadInt( m_renderWaitingThread );

	m_overlays.Restore( savefile );
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		// Add the (potentially NULL) GUI as an external GUI to our overlaysys,
		// so that the script functions to interact with GUIs can interact with it.
		// If the overlay isn't external, then the entity inheriting us must have
		// messed with the overlays, and doesn't want us to set it. If it IS
		// external, then they'll be resetting it anyways, so no harm done.
		if ( m_overlays.isExternal( OVERLAYS_MIN_HANDLE + i ) )
			m_overlays.setGui( OVERLAYS_MIN_HANDLE + i, renderEntity.gui[ i ] );
	}

	savefile->ReadObject( reinterpret_cast<idClass *&>( bindMaster ) );
	savefile->ReadJoint( bindJoint );
	savefile->ReadInt( bindBody );
	savefile->ReadObject( reinterpret_cast<idClass *&>( teamMaster ) );
	savefile->ReadObject( reinterpret_cast<idClass *&>( teamChain ) );

	savefile->ReadStaticObject( defaultPhysicsObj );
	RestorePhysics( &defaultPhysicsObj );

	savefile->ReadInt( numPVSAreas );
	for( i = 0; i < MAX_PVS_AREAS; i++ ) {
		savefile->ReadInt( PVSAreas[ i ] );
	}

	bool readsignals;
	savefile->ReadBool( readsignals );
	if ( readsignals ) {
		signals = new signalList_t;
		for( i = 0; i < NUM_SIGNALS; i++ ) {
			savefile->ReadInt( num );
			signals->signal[ i ].SetNum( num );
			for( j = 0; j < num; j++ ) {
				savefile->ReadInt( signals->signal[ i ][ j ].threadnum );
				savefile->ReadString( funcname );
				signals->signal[ i ][ j ].function = gameLocal.program.FindFunction( funcname );
				if ( !signals->signal[ i ][ j ].function ) {
					savefile->Error( "Function '%s' not found", funcname.c_str() );
				}
			}
		}
	}

	savefile->ReadInt( mpGUIState );

	bool hasInventory;
	savefile->ReadBool(hasInventory);
	if (hasInventory) {
		Inventory()->Restore(savefile);
	}

	bool hasInventoryCursor;
	savefile->ReadBool(hasInventoryCursor);
	if (hasInventoryCursor) {
		// Get the ID of the cursor
		int cursorId;
		savefile->ReadInt(cursorId);
		m_InventoryCursor = Inventory()->GetCursor(cursorId);
	}

	m_Attachments.Clear();
	savefile->ReadInt( num );
	m_Attachments.SetNum( num );
	for( int i=0; i < num; i++ )
		m_Attachments[i].Restore( savefile );

	m_AttNameMap.clear();
	savefile->ReadInt( num );
    for ( int i = 0; i < num; i++ )
    {
        idStr tempStr;
        savefile->ReadString(tempStr);

        int tempIndex;
        savefile->ReadInt(tempIndex);

        m_AttNameMap.insert(AttNameMap::value_type(tempStr.c_str(), tempIndex));
    }

	m_AttachPositions.Clear();
	savefile->ReadInt( num );
	for ( i = 0; i < num; i++ ) 
	{
		SAttachPosition &attachPos = m_AttachPositions.Alloc();
		attachPos.Restore( savefile );
	}

	// restore must retrieve modelDefHandle from the renderer
	if ( modelDefHandle != -1 ) {
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}

	// restore must retrieve m_renderTriggerHandle from the renderer
	if ( m_renderTriggerHandle != -1 ) {
		m_renderTriggerHandle = gameRenderWorld->AddEntityDef( &m_renderTrigger );
	}

	m_userManager.Restore(savefile);

	savefile->ReadFloat(m_LightQuotient);
	savefile->ReadInt(m_LightQuotientLastEvalTime);

	savefile->ReadBool(m_droppedByAI);	// grayman #1330

	savefile->ReadBool(m_isFlinder);	// grayman #4230

	// grayman #2341 - restore previous voice and body shaders and indices

	// restoring is unnecessary; simply set to NULL and 0
	previousVoiceShader = NULL;
	previousVoiceIndex = 0;
	previousBodyShader = NULL;
	previousBodyIndex = 0;

	// grayman #597

	savefile->ReadInt(m_HideUntilTime); // arrow-hiding timer

	// grayman #2787

	savefile->ReadVec3( m_VinePlantLoc );
	savefile->ReadVec3( m_VinePlantNormal );

	savefile->ReadVec3( m_LastRestPos ); // grayman #3992

	m_pushedBy.Restore( savefile ); // grayman #4603

	savefile->ReadInt(m_splashtime); // grayman #4600
	savefile->ReadBool(m_listening); // grayman #4620

	// SteveL #3817: make decals persistent
	int decalscount;
	savefile->ReadInt( decalscount );
	for ( int i = 0; i < decalscount; ++i )
	{
		SDecalInfo di;
		savefile->ReadString( di.decal );
		savefile->ReadVec3( di.origin );
		savefile->ReadVec3( di.dir );
		savefile->ReadFloat( di.size );
		savefile->ReadJoint( di.overlay_joint );
		savefile->ReadInt( di.decal_starttime );
		savefile->ReadFloat( di.decal_depth );
		savefile->ReadBool( di.decal_parallel );
		savefile->ReadFloat( di.decal_angle );
		decals_list.push_back( di );
	}
	needsDecalRestore = ( decalscount > 0 );

	// Tels #2417: after Restore call RestoreScriptObject() of the scriptObject so the
	// script object can restore f.i. sounds:

	const function_t *func = scriptObject.GetFunction( "RestoreScriptObject" );
	if (func)
	{
		idThread *thread = idThread::CurrentThread();
		if (!thread)
		{
			gameLocal.Printf("No running thread for RestoreScriptObject(), creating new one.\n");
			thread = new idThread();
			thread->SetThreadName( name.c_str() );
		}
		// gameLocal.Warning( "Will call function '%s' in '%s'", "RestoreScriptObject", scriptObject.GetTypeName() );
		if ( func->type->NumParameters() != 1 ) {
			gameLocal.Warning( "Function 'RestoreScriptObject' has the wrong number of parameters");
		}
		thread->CallFunction( this, func, true );
		thread->DelayedStart( 0 );
	}
/*	else
	{
		gameLocal.Warning("No RestoreScriptObject() function in script object for %s", GetName() );
	}
*/
	// done with calling "Restore()" on the script object
}

/*
================
idEntity::GetEntityDefName
================
*/
const char * idEntity::GetEntityDefName( void ) const
{
	if ( entityDefNumber < 0 ) {
		return "*unknown*";
	}
	return declManager->DeclByIndex( DECL_ENTITYDEF, entityDefNumber, false )->GetName();
}

/*
================
idEntity::SetName
================
*/
void idEntity::SetName( const char *newname )
{
	if ( name.Length() ) {
		gameLocal.RemoveEntityFromHash( name.c_str(), this );
		gameLocal.program.SetEntity( name, NULL );
	}

	name = newname;
	if ( name.Length() ) {
		if ( ( name == "NULL" ) || ( name == "null_entity" ) ) {
			gameLocal.Error( "Cannot name entity '%s'.  '%s' is reserved for script.", name.c_str(), name.c_str() );
		}
		gameLocal.AddEntityToHash( name.c_str(), this );
		gameLocal.program.SetEntity( name, this );
	}
}

/*
================
idEntity::GetName
================
*/
const char * idEntity::GetName( void ) const {
	return name.c_str();
}

/*
================
idEntity::SwapLODModel

Virtual function overriden with more complex methods by animated entities.
================
*/
void idEntity::SwapLODModel( const char *modelname )
{
	SetModel( modelname );
	RestoreDecals();	// #3817
}

/*
================
idEntity::Think
================
*/
void idEntity::Think( void )
{
	RunPhysics();
	if ( (thinkFlags & TH_PHYSICS) && m_FrobBox ) 
	{
		// update trigger position
		// TODO: Tels: What about hidden entities, these would use (0,0,0) as origin here?
		m_FrobBox->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	}

	Present();

	if ( needsDecalRestore ) // #3817
	{
		ReapplyDecals();
		needsDecalRestore = false;
	}
}

/*
================
idEntity::DoDormantTests

Monsters and other expensive entities that are completely closed
off from the player can skip all of their work
================
*/
bool idEntity::DoDormantTests( void )
{
	if ( cv_ai_opt_forcedormant.GetInteger() > 0)
		return true; // Everything always dormant!
	if ( cv_ai_opt_forcedormant.GetInteger() < 0)
		return false; // Everything never dormant!

	if ( fl.neverDormant ) {
		return false;
	}

	// if the monster area is not topologically connected to a player
	if ( !gameLocal.InPlayerConnectedArea( this ) ) {
		if ( dormantStart == 0 ) {
			dormantStart = gameLocal.time;
		}
		if ( gameLocal.time - dormantStart < DELAY_DORMANT_TIME ) {
			// just got closed off, don't go dormant yet
			return false;
		}
		return true;
	} else {
		// the monster area is topologically connected to a player, but if
		// the monster hasn't been woken up before, do the more precise PVS check
		// TDM: Never wake up permanently; always do PVS check.
		//      (This may cause bugs. I did this for AI optimisation testing.)
		//	--Crispy
		if ( true || !fl.hasAwakened ) {
			if ( !gameLocal.InPlayerPVS( this ) ) {
				return true;		// stay dormant
			}
		}

		// wake up
		dormantStart = 0;
		fl.hasAwakened = true;		// only go dormant when area closed off now, not just out of PVS
		
		return false;
	}

//	return false;
}

/*
================
idEntity::CheckDormant

Monsters and other expensive entities that are completely closed
off from the player can skip all of their work
================
*/
bool idEntity::CheckDormant( void )
{
	bool dormant;
	
	dormant = DoDormantTests();
	if ( dormant && !fl.isDormant ) {
		fl.isDormant = true;
		DormantBegin();
	} else if ( !dormant && fl.isDormant ) {
		fl.isDormant = false;
		DormantEnd();
	}

	return dormant;
}

/*
================
idEntity::DormantBegin

called when entity becomes dormant
================
*/
void idEntity::DormantBegin( void ) {
}

/*
================
idEntity::DormantEnd

called when entity wakes from being dormant
================
*/
void idEntity::DormantEnd( void ) {
}

/*
================
idEntity::IsBroken
================
*/
bool idEntity::IsBroken( void ) const {
	return m_bIsBroken;
}


/*
================
idEntity::SpawnFlinder spawns zero, one or more flinder objects as
described by one FlinderSpawn struct. The activator is the entity
that caused this entity to break up.

@return: -1 for error, or the number of entities spawned

*/
int idEntity::SpawnFlinder( const FlinderSpawn& fs, idEntity *activator )
{
	int spawned = 0;
	
	for ( int i = 0 ; i < fs.m_Count ; i++ )
	{
		// probability of spawning

		if ( (fs.m_Probability < 1.0) && (gameLocal.random.RandomFloat() > fs.m_Probability) )
		{
			continue;
		}

		const idDict *p_entityDef = gameLocal.FindEntityDefDict( fs.m_Entity.c_str(), false );
	    if ( p_entityDef )
		{
			idEntity *flinder;
			gameLocal.SpawnEntityDef( *p_entityDef, &flinder, false );
			idPhysics *physics;
			idVec3 TumbleVec(vec3_zero);

			if ( !flinder )
			{
				gameLocal.Error( "Failed to spawn flinder entity %s", fs.m_Entity.c_str() );
				return -1;
	        }

			DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING(" Spawned entity %s\r", flinder->GetName());

			flinder->m_isFlinder = true; // grayman #4230 - flinders fly, they don't drop straight to the floor

			// move the entity to the origin (plus offset) and orientation of the original

			physics = flinder->GetPhysics();
			physics->SetOrigin( GetPhysics()->GetOrigin() + fs.m_Offset );
			physics->SetAxis( GetPhysics()->GetAxis() );

			// give the flinders the same speed as the original entity (in case it breaks
			// up while on the move)
			// tels FIXME: We would need to count how many flinders were spawned then distribute
			//			   the impulse from the activator in equal parts to each of them.
			// tels FIXME: Currently these are zero when an arrow hits a bottle.

			// for now add a small random speed so the object moves
			// tels FIXME: Doesn't work. Because the flinder is inside the still solid entity?

			TumbleVec[0] += (gameLocal.random.RandomFloat() * 20) - 10;
			TumbleVec[1] += (gameLocal.random.RandomFloat() * 20) - 10;
			TumbleVec[2] += (gameLocal.random.RandomFloat() * 20) - 10;

			// Dragofer: optional activator
			if( activator != NULL )
			{
				physics->SetLinearVelocity(
					GetPhysics()->GetLinearVelocity() 
					+ activator->GetPhysics()->GetLinearVelocity()
					+ TumbleVec
					);
				physics->SetAngularVelocity(
					GetPhysics()->GetAngularVelocity() 
					+ activator->GetPhysics()->GetAngularVelocity()
					+ TumbleVec
					);
			}

			else
			{
				physics->SetLinearVelocity(
					GetPhysics()->GetLinearVelocity() 
					+ TumbleVec
					);
				physics->SetAngularVelocity(
					GetPhysics()->GetAngularVelocity() 
					+ TumbleVec
					);
			}

			/*
			DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING(" Activator is: %s\r", activator->GetName() );
			DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING(" Set linear velocity to %f %f %f:\r", 
				physics->GetLinearVelocity().x,  
				physics->GetLinearVelocity().y,  
				physics->GetLinearVelocity().z ); 
			DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING(" Set angular velocity to %f %f %f:\r", 
				physics->GetAngularVelocity().x,  
				physics->GetAngularVelocity().y,  
				physics->GetAngularVelocity().z ); 
			*/

			if ( activator != NULL && activator->IsType(idActor::Type) )
			{
				idActor *actor = static_cast<idActor *>(activator);
				flinder->m_SetInMotionByActor = actor;
				flinder->m_MovedByActor = actor;
			}

			// activate the flinder, so it falls realistically down

			flinder->BecomeActive(TH_PHYSICS|TH_THINK);

			// FIXME if this entity has a skin, set the same skin on the flinder
			// FIXME add a small random impulse outwards from entity origin

			spawned++;
		}
	} // end for m_Count

	return spawned;
}

/*
================
idEntity::BecomeBroken
================
*/
void idEntity::BecomeBroken( idEntity *activator )
{
	if ( m_bIsBroken )
	{
		// we are already broken, so do nothing
		return;
	}

	m_bIsBroken = true;

	if ( spawnArgs.GetBool( "hideModelOnBreak" ) )
	{
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Hiding broken entity %s\r", name.c_str() ); 
		SetModel( "" );
		GetPhysics()->SetContents( 0 );
	}

	// switch to the brokenModel if it was defined
	else if ( brokenModel.Length() )
	{
		SetModel( brokenModel );

		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Breaking entity %s (solid: %i)\r", name.c_str(), spawnArgs.GetBool( "solid" ) ); 
		if ( spawnArgs.GetBool( "solid" ) )
		{
			DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Setting new clipmodel '%s'\r)", brokenModel.c_str() ); 

			idClipModel* clipmodel = new idClipModel( brokenModel );

			if (clipmodel && clipmodel->IsTraceModel() && GetPhysics())
			{
				GetPhysics()->SetClipModel( clipmodel, 1.0f );
			}
		}
	} 

	// tels: if a break_up_script is defined, run it:
	idStr str;
	if (this->spawnArgs.GetString("break_up_script", "", str))
	{
		// Call the script
        	idThread* thread = CallScriptFunctionArgs(str.c_str(), true, 0, "e", this);
		if (thread != NULL)
		{
			// Run the thread at once, the script result might be needed below.
			thread->Execute();
		}
	}

	// tels: if we have flinders to spawn on break, do so now
	// Dragofer: make this optional for entities that shouldn't always flinderize when breaking
	if ( m_bFlinderize )
	{
		Flinderize(activator);
	}
}

/*
================
idEntity::BecomeActive
================
*/
void idEntity::BecomeActive( int flags )
{
	if ( ( flags & TH_PHYSICS ) ) {
		// enable the team master if this entity is part of a physics team
		if ( teamMaster && teamMaster != this ) {
			teamMaster->BecomeActive( TH_PHYSICS );
		}
	}

	int oldFlags = thinkFlags;
	thinkFlags |= flags;

	if ( thinkFlags ) {
		if ( !IsActive() ) {
			assert(activeIdx >= -1);
			gameLocal.activeEntities.AddToEnd( this );
		} else if ( !oldFlags ) {
			// we became inactive this frame, so we have to decrease the count of entities to deactivate
			gameLocal.numEntitiesToDeactivate--;
		}
	}
}

/*
================
idEntity::BecomeInactive
================
*/
void idEntity::BecomeInactive( int flags )
{
	if ( ( flags & TH_PHYSICS ) ) {
		// may only disable physics on a team master if no team members are running physics or bound to a joints
		if ( teamMaster == this ) {
			for ( idEntity *ent = teamMaster->teamChain; ent; ent = ent->teamChain ) {
				if ( ( ent->thinkFlags & TH_PHYSICS ) || ( ( ent->bindMaster == this ) && ( ent->bindJoint != INVALID_JOINT ) ) ) {
					flags &= ~TH_PHYSICS;
					break;
				}
			}
		}
	}

	if ( thinkFlags ) {
		thinkFlags &= ~flags;
		if ( !thinkFlags && IsActive() ) {
			gameLocal.numEntitiesToDeactivate++;
		}
	}

	if ( ( flags & TH_PHYSICS ) ) {
		// if this entity has a team master
		if ( teamMaster && teamMaster != this ) {
			// if the team master is at rest
			if ( teamMaster->IsAtRest() ) {
				teamMaster->BecomeInactive( TH_PHYSICS );
			}
		}
	}
}

/***********************************************************************

	Visuals
	
***********************************************************************/

/*
================
idEntity::SetShaderParm
================
*/
void idEntity::SetShaderParm( int parmnum, float value )
{
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Warning( "shader parm index (%d) out of range", parmnum );
		return;
	}

	renderEntity.shaderParms[ parmnum ] = value;
	// tels: TODO: UpdateVisuals does stuff like updating the Model Transform,
	// calling updating the MD5 animation, clear the PVS areas, calling updateSound()
	// etc. All these things might be unnec. for just modifying a shaderParm, see if
	// we can optimize this.
	UpdateVisuals();
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( const idVec3 &color ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= color.x;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= color.y;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= color.z;
	// tels: TODO: See note in idEntity::SetShaderParm about optimizing this.
	UpdateVisuals();
}

/*
================
idEntity::GetColor
================
*/
void idEntity::GetColor( idVec3 &out ) const {
	out[ 0 ] = renderEntity.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( const idVec4 &color ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= color[ 0 ];
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= color[ 1 ];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= color[ 2 ];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= color[ 3 ];
	// tels: TODO: See note in idEntity::SetShaderParm about optimizing this.
	UpdateVisuals();
}

/*
================
idEntity::SetAlpha

Tels: Just set the alpha value. Note: Due to the D3 engine not being
	  able to render faces transparently with correct lighting (you
	  either get 100% opaque with correct light, or 50% transparent
	  with correct light, or X% transparent with incorrect light),
	  this doesn't actually work unless you have a material
	  shader that takes the alpha value into account.
================
*/
void idEntity::SetAlpha( const float alpha ) {
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= alpha;
	// tels: TODO: See note in idEntity::SetShaderParm about optimizing this.
	UpdateVisuals();
}

/*
================
idEntity::SetAlpha

Tels: Set alpha including on our children
================
*/
void idEntity::SetAlpha( const float alpha, const bool bound ) {

	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= alpha;

	if (!bound)
	{
	    // tels: TODO: See note in idEntity::SetShaderParm about optimizing this.
		UpdateVisuals();
		return;
	}

	// show our bind-children
	idEntity *ent;
	idEntity *next;

	for( ent = GetNextTeamEntity(); ent != NULL; ent = next ) 
	{
		next = ent->GetNextTeamEntity();
		if ( ent->GetBindMaster() == this ) 
		{
			ent->SetAlpha( alpha );
		}
	}
	// tels: TODO: See note in idEntity::SetShaderParm about optimizing this.
	UpdateVisuals();
}

/*
================
idEntity::GetColor
================
*/
void idEntity::GetColor( idVec4 &out ) const {
	out[ 0 ] = renderEntity.shaderParms[ SHADERPARM_RED ];
	out[ 1 ] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	out[ 2 ] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	out[ 3 ] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];
}

/*
================
idEntity::UpdateAnimationControllers
================
*/
bool idEntity::UpdateAnimationControllers( void ) {
	// any ragdoll and IK animation controllers should be updated here
	return false;
}

/*
================
idEntity::SetModel
================
*/
void idEntity::SetModel( const char *modelname ) {
	assert( modelname );

	FreeModelDef();

	// FIXME: temp hack to replace obsolete ips particles with prt particles
	if ( strstr( modelname, ".ips" ) != NULL ) {
		idStr str = modelname;
		str.Replace( ".ips", ".prt" );
		renderEntity.hModel = renderModelManager->FindModel( str );
	} else {
		renderEntity.hModel = renderModelManager->FindModel( modelname );
	}

	if ( renderEntity.hModel ) {
		renderEntity.hModel->Reset();
	}

	renderEntity.callback = NULL;

	renderEntity.numJoints = 0;
	renderEntity.joints = NULL;
	if ( renderEntity.hModel ) {
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
	} else {
		renderEntity.bounds.Zero();
	}

	UpdateVisuals();
}

/*
================
idEntity::SetSkin
================
*/
void idEntity::SetSkin( const idDeclSkin *skin ) {
	renderEntity.customSkin = skin;
	UpdateVisuals();
}

/*
================
idEntity::GetSkin
================
*/
const idDeclSkin *idEntity::GetSkin( void ) const {
	return renderEntity.customSkin;
}

/*
================
idEntity::ReskinCollisionModel	// #4232
================
*/
void idEntity::ReskinCollisionModel() 
{
	if ( !physics->GetClipModel() ) 
	{ 
		return; 
	}

	const cmHandle_t skinnedCM = idClipModel::CheckModel( renderEntity.hModel->Name(), renderEntity.customSkin );

	if ( skinnedCM < 0 )
	{
		return;
	}

	idClipModel* cm = new idClipModel( renderEntity.hModel->Name(), renderEntity.customSkin );
	physics->SetClipModel( cm, 1.0f );
}



/*
================
idEntity::FreeModelDef
================
*/
void idEntity::FreeModelDef( void ) {
	if ( modelDefHandle != -1 ) {
		gameRenderWorld->FreeEntityDef( modelDefHandle );
		modelDefHandle = -1;
	}
}

/*
================
idEntity::FreeLightDef
================
*/
void idEntity::FreeLightDef( void ) {
}

/*
================
idEntity::IsHidden
================
*/
bool idEntity::IsHidden( void ) const {
	return fl.hidden;
}

/*
================
idEntity::SetHideUntilTime
================
*/
void idEntity::SetHideUntilTime(const int time) // grayman #597
{
	m_HideUntilTime = time;
}

/*
================
idEntity::GetHideUntilTime
================
*/
int idEntity::GetHideUntilTime() const // grayman #597
{
	return m_HideUntilTime;
}

/*
================
idEntity::Hide
================
*/
void idEntity::Hide( void ) 
{
	if ( !fl.hidden )
	{
		fl.hidden = true;

		idPhysics* p = GetPhysics();

		if (p)
		{
			m_preHideContents = p->GetContents();
			m_preHideClipMask = p->GetClipMask();
		}

		if( m_FrobBox )
				m_FrobBox->SetContents(0);

		FreeModelDef();
		UpdateVisuals();

		// If we are hiding a currently frobbed entity,
		// set the frob pointers to NULL to avoid stale pointers
		idPlayer* player = gameLocal.GetLocalPlayer();
	
		if (player)
		{
			if (player->m_FrobEntity.GetEntity() == this)
			{
				player->m_FrobEntity = NULL;
			}
		}

		// hide our bind-children
		for (idEntity* ent = GetNextTeamEntity(); ent != NULL; ent = ent->GetNextTeamEntity()) 
		{
			if (ent->GetBindMaster() == this) 
			{
				ent->Hide();
			}
		}
	}
}

/*
================
idEntity::Show
================
*/
void idEntity::Show( void ) 
{
	// greebo: If the pre-hide clipmask is still uninitialised on Show(), the entity 
	// has not been hidden before. Set this to something valid (i.e. the current clipmask)
	if ( m_preHideClipMask == -1 )
	{
		m_preHideClipMask = GetPhysics()->GetClipMask();
	}

	if ( m_preHideContents == -1 )
	{
		m_preHideContents = GetPhysics()->GetContents();
	}

	if ( fl.hidden ) 
	{
		fl.hidden = false;
		if ( m_FrobBox && m_bFrobable )
		{
			m_FrobBox->SetContents( CONTENTS_FROBABLE );
		}

		// #6175 make make sure hidden items regain physics on show events
		if ( spawnArgs.GetBool("solid", "1" ) ) {
			SetSolid( true );
		}
		UpdateVisuals();
		RestoreDecals();	// #3817

		// show our bind-children
		idEntity *ent;
		idEntity *next;

		for ( ent = GetNextTeamEntity() ; ent != NULL ; ent = next ) 
		{
			next = ent->GetNextTeamEntity();
			if ( ent->GetBindMaster() == this ) 
			{
				if ( gameLocal.time >= ent->GetHideUntilTime() ) // grayman #597 - one second needs to pass before showing
				{
					ent->Show();
				}
			}
		}
	}

	if ( spawnArgs.GetBool("neverShow","0") ) // grayman #2998
	{
		PostEventMS( &EV_Hide, 0 ); // queue a hide for later
	}
}

/*
================
idEntity::SetSolid
================
*/
void idEntity::SetSolid( bool solidity ) {

	idPhysics* p = GetPhysics();

	// If the contents and clipmask are still uninitialised, the entity has not been hidden
	// or had its solidity altered by this function before.
	// Set this to something valid: the current clipmask and contents
	if ( m_preHideContents == -1 ) {
		m_preHideContents = p->GetContents();
	}
	if ( m_preHideClipMask == -1 ) {
		m_preHideClipMask = p->GetClipMask();
	}

	if( solidity == false )
	{
		p->SetContents( 0 );
		p->SetClipMask( 0 );
	
		// SR CONTENTS_RESPONSE FIX:
		if( m_StimResponseColl->HasResponse() )
			p->SetContents( CONTENTS_RESPONSE );

		// preserve opacity towards AIs
		if( m_preHideContents & CONTENTS_OPAQUE )
		{
			p->SetContents( p->GetContents() | CONTENTS_OPAQUE);
		}
	}

	else if( solidity == true )
	{
		// Set contents. If contents are empty (entity started nonsolid), set some default values.
		p->SetContents( (m_preHideContents) ? m_preHideContents : CONTENTS_SOLID | CONTENTS_OPAQUE );
		p->SetClipMask( (m_preHideClipMask) ? m_preHideClipMask : MASK_SOLID | CONTENTS_OPAQUE );

		if ( m_FrobBox && m_bFrobable )
			m_FrobBox->SetContents( p->GetContents() | CONTENTS_FROBABLE );
	}

}

float idEntity::GetLightQuotient()
{
	if (g_lightQuotientAlgo.GetBool()) {
		float value = gameLocal.m_LightEstimateSystem->GetLightOnEntity(this);
		return value;
	}

	if (m_LightQuotientLastEvalTime < gameLocal.time)
	{
		idPhysics* physics = GetPhysics();

		// Get the bounds and move it upwards a tiny bit
		idBounds bounds = physics->GetAbsBounds() + physics->GetGravityNormal() * 0.1f; // Tweak to stay out of floors
		//gameRenderWorld->DebugBox(colorRed, idBox(bounds), 50000);
		//gameRenderWorld->DebugLine(colorGreen, bounds[0], bounds[1], 50000);

		// A single point doesn't work with ellipse intersection
		bounds.ExpandSelf(0.1f);

		// grayman #3584 - For fallen AI, their bounding box can extend below the floor.
		// If it does, it doesn't represent the body properly. Let's
		// bring it up so it rests on the floor.

		if ( IsType(idAI::Type) && ( ( health <= 0) || static_cast<idAI*>(this)->IsKnockedOut() ) )
		{
			idVec3 startPoint = bounds[0]; // this includes the min z position
			startPoint.z = bounds[1].z; // assume high point is above the floor
			idVec3 endPoint = startPoint;
			endPoint.z -= 100;
	
			// trace down to find the floor

			trace_t result;
			if (gameLocal.clip.TracePoint(result, startPoint, endPoint, MASK_OPAQUE, this))
			{
				// found the floor
		
				if ( result.endpos.z > bounds[0].z )
				{
					bounds[0].z = result.endpos.z + 0.25;// move the target point to just above the floor
				}
			}

			// grayman #3584 - rotate the bounding box to align with the object's orientation

			float ht = bounds[1].z - bounds[0].z;
			idVec3 size = GetPhysics()->GetBounds().GetSize();
			idBox box(vec3_zero, idVec3(ht/2.0f,size.y/2.0f,size.z/2.0f), GetPhysics()->GetAxis().ToAngles().ToMat3());
			box.TranslateSelf(bounds.GetCenter());

			// Update the cache
			m_LightQuotientLastEvalTime = gameLocal.time;

			// grayman #3584 - we want to alter the illumination line per light, based on the
			// line's orientation to the light. Since we don't want to affect other uses of queryLightingAlongLine(),
			// let's create a variation of that routine and use it here.

			m_LightQuotient = LAS.queryLightingAlongBestLine(box, this, true);
		}
		else
		{
			// Update the cache
			m_LightQuotientLastEvalTime = gameLocal.time;
			m_LightQuotient = LAS.queryLightingAlongLine(bounds[0], bounds[1], this, true);
		}
	}

	// Return the cached result
	return m_LightQuotient;
}

bool idEntity::DebugGetLightQuotient(float &result) const {
	if (g_lightQuotientAlgo.GetBool()) {
		return gameLocal.m_LightEstimateSystem->DebugGetLightOnEntity(this, result);
	}

	if (m_LightQuotientLastEvalTime >= gameLocal.time - 300) {
		result = m_LightQuotient;
		return true;
	} else {
		return false;
	}
}

/*
================
idEntity::Event_noShadows

tels: Turn shadows from this entity on or off.
================
*/
void idEntity::Event_noShadows( const bool noShadow ) 
{
	renderEntity.noShadow = ( noShadow ? 1 : 0 );
	UpdateVisuals();
}

/*
================
idEntity::Event_noShadowsDelayed

tels: Turn shadows from this entity on or off, after a delay.
================
*/
void idEntity::Event_noShadowsDelayed( const bool noShadow, const float delay ) 
{
	// post an event to happen in "delay" ms
	PostEventMS( &EV_NoShadows, delay, noShadow );
}

/*
================
idEntity::Event_CheckMine

grayman #2478: Replace an armed mine with its projectile counterpart
================
*/
void idEntity::Event_CheckMine()
{
	if ( !spawnArgs.GetBool( "armed", "0" ) )
	{
		return; // not armed, nothing to do
	}

	const char* replaceWith = spawnArgs.GetString( "def_armed" ); // get replacement entity def

	const idDict *resultDef = gameLocal.FindEntityDefDict( replaceWith, false );
	if ( resultDef )
	{
		idEntity *newMine;
		gameLocal.SpawnEntityDef( *resultDef, &newMine, false );
		idProjectile* projectile = static_cast<idProjectile*>(newMine);
		projectile->Launch( GetPhysics()->GetOrigin(), idVec3( 0,0,1 ), vec3_origin );

		// Undo the launch parameters to keep the mine from flying away.

		idPhysics* projPhysics = projectile->GetPhysics();
		projPhysics->SetOrigin( GetPhysics()->GetOrigin() );
		projPhysics->SetAxis( GetPhysics()->GetAxis() );
		projPhysics->SetLinearVelocity( vec3_origin );
		projPhysics->SetAngularVelocity( vec3_origin );
		projPhysics->PutToRest();
		projectile->UpdateVisuals();
		projectile->SetReplaced(); // grayman #2908 - note that this mine replaced an author-placed armed mine 

		SetFrobable(false);
		PostEventMS( &EV_Remove, 1 ); // Remove the mine, which has been replaced
	}
}

/*
================
idEntity::Event_GetVinePlantLoc

grayman #2787: Return the planting location resulting from a trace
================
*/
void idEntity::Event_GetVinePlantLoc()
{
	idThread::ReturnVector( m_VinePlantLoc );
}

/*
================
idEntity::Event_GetVinePlantNormal

grayman #2787: Return the planting location surface normal resulting from a trace
================
*/
void idEntity::Event_GetVinePlantNormal()
{
	idThread::ReturnVector( m_VinePlantNormal );
}

/*
================
idEntity::UpdateModelTransform
================
*/
void idEntity::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		renderEntity.axis = axis * GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin() + origin * renderEntity.axis;
	} else {
		renderEntity.axis = GetPhysics()->GetAxis();
		renderEntity.origin = GetPhysics()->GetOrigin();
	}
}

/*
================
idEntity::UpdateModel
================
*/
void idEntity::UpdateModel( void ) {
	UpdateModelTransform();

	// check if the entity has an MD5 model
	idAnimator *animator = GetAnimator();
	if(animator && animator->ModelHandle())
	{
		// set the callback to update the joints
		renderEntity.callback = idEntity::ModelCallback;
	}

	// set to invalid number to force an update the next time the PVS areas are retrieved
	ClearPVSAreas();

	// ensure that we call Present this frame
	BecomeActive( TH_UPDATEVISUALS );

	// If the entity has an xray skin, go ahead and add it
	if ( xraySkin != NULL ) {
		renderEntity.xrayIndex = 4;
		xrayRenderEnt = renderEntity;
		xrayRenderEnt.xrayIndex = 2;
		xrayRenderEnt.customSkin = xraySkin;
		if ( xrayDefHandle == -1 )
			xrayDefHandle = gameRenderWorld->AddEntityDef( &xrayRenderEnt );
		else
			gameRenderWorld->UpdateEntityDef( xrayDefHandle, &xrayRenderEnt );
	} 
	if ( xrayModelHandle ) {
		renderEntity.xrayIndex = 4;
		xrayRenderEnt = renderEntity;
		xrayRenderEnt.xrayIndex = 2;
		xrayRenderEnt.hModel = xrayModelHandle;
		xrayRenderEnt.customSkin = xraySkin;

		if ( IsHidden() ) {
			gameRenderWorld->FreeEntityDef( xrayDefHandle );
			xrayDefHandle = -1;
		} else if ( xrayDefHandle == -1 ) {
			xrayDefHandle = gameRenderWorld->AddEntityDef( &xrayRenderEnt );
		} else {
			gameRenderWorld->UpdateEntityDef( xrayDefHandle, &xrayRenderEnt );
		}
	}
}

/*
================
idEntity::UpdateVisuals
================
*/
void idEntity::UpdateVisuals( void ) {
	UpdateModel();
	UpdateSound(); // grayman #4337
}

/*
================
idEntity::UpdatePVSAreas
================
*/
void idEntity::UpdatePVSAreas( void ) {
	int localNumPVSAreas, localPVSAreas[32];
	idBounds modelAbsBounds;
	int i;

	modelAbsBounds.FromTransformedBounds( renderEntity.bounds, renderEntity.origin, renderEntity.axis );
	localNumPVSAreas = gameLocal.pvs.GetPVSAreas( modelAbsBounds, localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );

	// FIXME: some particle systems may have huge bounds and end up in many PVS areas
	// the first MAX_PVS_AREAS may not be visible to a network client and as a result the particle system may not show up when it should
	if ( localNumPVSAreas > MAX_PVS_AREAS ) {
		localNumPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( modelAbsBounds.GetCenter() ).Expand( 64.0f ), localPVSAreas, sizeof( localPVSAreas ) / sizeof( localPVSAreas[0] ) );
	}

	for ( numPVSAreas = 0; numPVSAreas < MAX_PVS_AREAS && numPVSAreas < localNumPVSAreas; numPVSAreas++ ) {
		PVSAreas[numPVSAreas] = localPVSAreas[numPVSAreas];
	}

	for( i = numPVSAreas; i < MAX_PVS_AREAS; i++ ) {
		PVSAreas[ i ] = 0;
	}
}

/*
================
idEntity::UpdatePVSAreas
================
*/
void idEntity::UpdatePVSAreas( const idVec3 &pos ) {
	int i;

	numPVSAreas = gameLocal.pvs.GetPVSAreas( idBounds( pos ), PVSAreas, MAX_PVS_AREAS );
	i = numPVSAreas;
	while ( i < MAX_PVS_AREAS ) {
		PVSAreas[ i++ ] = 0;
	}
}

/*
================
idEntity::GetNumPVSAreas
================
*/
int idEntity::GetNumPVSAreas( void ) {
	if ( numPVSAreas < 0 ) {
		UpdatePVSAreas();
	}
	return numPVSAreas;
}

/*
================
idEntity::GetPVSAreas
================
*/
const int *idEntity::GetPVSAreas( void ) {
	if ( numPVSAreas < 0 ) {
		UpdatePVSAreas();
	}
	return PVSAreas;
}

/*
================
idEntity::ClearPVSAreas
================
*/
void idEntity::ClearPVSAreas( void ) {
	numPVSAreas = -1;
}

/*
================
idEntity::PhysicsTeamInPVS

  FIXME: for networking also return true if any of the entity shadows is in the PVS
================
*/
bool idEntity::PhysicsTeamInPVS( pvsHandle_t pvsHandle ) {
	idEntity *part;

	if ( teamMaster ) {
		for ( part = teamMaster; part; part = part->teamChain ) {
			if ( gameLocal.pvs.InCurrentPVS( pvsHandle, part->GetPVSAreas(), part->GetNumPVSAreas() ) ) {
				return true;
			}
		}
	} else {
		return gameLocal.pvs.InCurrentPVS( pvsHandle, GetPVSAreas(), GetNumPVSAreas() );
	}
	return false;
}

/*
==============
idEntity::ProjectOverlay
==============
*/
void idEntity::ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material, bool save ) {
	float s, c;
	idMat3 axis, axistemp;
	idVec3 localOrigin, localAxis[2];
	idPlane localPlane[2];

	// make sure the entity has a valid model handle
	if ( modelDefHandle < 0 ) {
		return;
	}

	// only do this on dynamic md5 models
	if ( renderEntity.hModel->IsDynamicModel() != DM_CACHED ) {
		return;
	}

	idMath::SinCos16( gameLocal.random.RandomFloat() * idMath::TWO_PI, s, c );

	// Store overlay info for restoration, before 'size' gets modified. -- SteveL #3817
	if (save)
	{
		SaveOverlayInfo( origin, dir, size, material );
	}

	axis[2] = -dir;
	axis[2].NormalVectors( axistemp[0], axistemp[1] );
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	axis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	renderEntity.axis.ProjectVector( origin - renderEntity.origin, localOrigin );
	renderEntity.axis.ProjectVector( axis[0], localAxis[0] );
	renderEntity.axis.ProjectVector( axis[1], localAxis[1] );

	size = 1.0f / size;
	localAxis[0] *= size;
	localAxis[1] *= size;

	localPlane[0] = localAxis[0];
	localPlane[0][3] = -( localOrigin * localAxis[0] ) + 0.5f;

	localPlane[1] = localAxis[1];
	localPlane[1][3] = -( localOrigin * localAxis[1] ) + 0.5f;

	const idMaterial *mtr = declManager->FindMaterial( material );

	// project an overlay onto the model
	gameRenderWorld->ProjectOverlay( modelDefHandle, localPlane, mtr );

	// make sure non-animating models update their overlay
	UpdateVisuals();
}

/*
================
idEntity::RestoreDecals

Reapply decals and overlays after LOD switch / hiding / save game loading.
This is the public method called by external code. It sets a flag which'll be used 
on the next Think(), after the model gets Presented to the renderer. SteveL #3817
================
*/
void idEntity::RestoreDecals()
{
	needsDecalRestore = true;
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idEntity::ReapplyDecals

Reapply decals and overlays after LOD switch / hiding / save game loading.
================
*/
void idEntity::ReapplyDecals()
{
	// #3817: Overridden by idStaticEntity and idAnimatedEntity
}

/*
================
idEntity::SaveOverlayInfo

Store info about an overlay on an animated mesh so that it can be replaced later.
Find the closest joint and store the positioning detail relative to that joint. This tactic 
prevents blood sliding round a moving mesh (esp. head and neck) during LOD transitions. SteveL #3817
================
*/
void idEntity::SaveOverlayInfo( const idVec3& impact_origin, const idVec3& impact_dir, const float size, const char* decal )
{
	if ( !IsType( idAnimatedEntity::Type ) || !GetRenderEntity()->hModel )
	{
		assert( false ); // won't happen in current code, but someone 
		return;			 // might screw up later and we're about to cast
	}

	// Retrieve joint data
	idAnimatedEntity* self = static_cast<idAnimatedEntity*>( this );
	idJointMat* joints;
	int numjoints;
	self->GetAnimator()->GetJoints( &numjoints, &joints );
	const idMD5Joint* bones = self->GetRenderEntity()->hModel->GetJoints();
	
	// Find the nearest joint to the impact
	float near_dist = FLT_MAX;
	const char* near_name = NULL;
	for ( int i = 0; i < numjoints; ++i, ++joints, ++bones )
	{
		const idVec3 joint_world_origin = renderEntity.origin + joints->ToVec3() * renderEntity.axis;
		const float dist = ( impact_origin - joint_world_origin ).Length();
		if ( dist < near_dist )
		{
			near_dist = dist;
			near_name = bones->name.c_str();
		}
	}
	assert( near_dist < FLT_MAX );
	const jointHandle_t near_handle = self->GetAnimator()->GetJointHandle( near_name );

	// Calculate position and impact direction of the overlay relative to chosen joint
	const idJointMat* near_joint = &renderEntity.joints[near_handle];
	const idMat3 jointaxis = near_joint->ToMat3() * renderEntity.axis;
	const idVec3 jointorigin = renderEntity.origin + near_joint->ToVec3() * renderEntity.axis;
	const idVec3 localImpactOrg = ( impact_origin - jointorigin ) * jointaxis.Transpose();
	const idVec3 localImpactDir = impact_dir * jointaxis.Transpose();

	// Store the information
	SDecalInfo di;
	di.overlay_joint = near_handle;
	di.dir = localImpactDir;
	di.origin = localImpactOrg;
	di.size = size;
	di.decal = decal;
	di.decal_depth = 0.0f;		// not used for animated overlays
	di.decal_parallel = false;	// not used for animated overlays
	di.decal_angle = 0.0f;		// not used for animated overlays
	di.decal_starttime = 0;		// not used for animated overlays
	decals_list.push_back( di );
}

/*
================
idEntity::SaveDecalInfo

For restoration after a LOD switch. -- SteveL #3817
================
*/
void idEntity::SaveDecalInfo( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle )
{
	SDecalInfo di;
	di.decal_angle = angle;
	di.decal = material;
	di.decal_depth = depth;
	di.dir = dir;
	di.overlay_joint = INVALID_JOINT; // only needed for animated overlays
	di.origin = origin;
	di.decal_parallel = parallel;
	di.size = size;
	di.decal_starttime = gameLocal.time;
	decals_list.push_back( di );
}

/*
================
idEntity::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void idEntity::Present(void)
{
/*
	if( m_bFrobable )
	{
*/
/*
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("this: %08lX    FrobDistance: %lu\r", this, m_FrobDistance);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("RenderEntity: %08lX\r", renderEntity);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("SurfaceInView: %u\r", renderEntity.allowSurfaceInViewID);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("RenderModel: %08lX\r", renderEntity.hModel);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("CustomShader: %08lX\r", renderEntity.customShader);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("ReferenceShader: %08lX\r", renderEntity.referenceShader);
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("ReferenceShader: %08lX\r", renderEntity.referenceShader);

		for(int i = 0; i < MAX_ENTITY_SHADER_PARMS; i++)
			DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Shaderparam[%u]: %f\r", i, renderEntity.shaderParms[i]);

		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("ForceUpdate: %u\r", renderEntity.forceUpdate);

		renderEntity.customShader = gameLocal.GetGlobalMaterial();
	}
*/

	if ( !gameLocal.isNewFrame )
	{
		return;
	}

	if ( m_bFrobable )
	{
		UpdateFrobState();
		UpdateFrobDisplay();
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !(thinkFlags & TH_UPDATEVISUALS) )
	{
		return;
	}

	if ( !cameraTarget ) // grayman #4615
	{
		BecomeInactive( TH_UPDATEVISUALS );
	}

	// camera target for remote render views
	if ( cameraTarget && gameLocal.InPlayerPVS(this) )
	{
		renderEntity.remoteRenderView = cameraTarget->GetRenderView();

		// grayman #4620 - set (or unset) the entity whose origin will be the origin of the sound the player hears

		idPlayer* player = gameLocal.GetLocalPlayer();
		if ( cameraTarget->m_listening )
		{
			player->m_Listener = static_cast<idListener*>(cameraTarget); // turn on listener
		}
		else
		{
			player->m_Listener = NULL; // turn off listener
		}
	}

	// if set to invisible, skip
	if ( !renderEntity.hModel || IsHidden() )
	{
		return;
	}

	// add to refresh list
	if ( modelDefHandle == -1 )
	{
		modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
	}
	else
	{
		gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
	}

	PresentRenderTrigger();
}

/*
================
idEntity::GetRenderEntity
================
*/
renderEntity_t *idEntity::GetRenderEntity( void ) {
	return &renderEntity;
}

/*
================
idEntity::GetModelDefHandle
================
*/
int idEntity::GetModelDefHandle( void ) {
	return modelDefHandle;
}

/*
================
idEntity::UpdateRenderEntity
================
*/
bool idEntity::UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView )
{
	if ( gameLocal.inCinematic && gameLocal.skipCinematic ) {
		return false;
	}

	idAnimator *animator = GetAnimator();
	if ( animator ) {
		return animator->CreateFrame( gameLocal.time, false );
	}

	return false;
}

/*
================
idEntity::ModelCallback

	NOTE: may not change the game state whatsoever!
================
*/
bool idEntity::ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView )
{
	idEntity *ent;

	ent = gameLocal.entities[ renderEntity->entityNum ];

	if ( !ent ) {
		gameLocal.Error( "idEntity::ModelCallback: callback with NULL game entity" );
	}
	return ent->UpdateRenderEntity( renderEntity, renderView );
}

/*
================
idEntity::GetAnimator

Subclasses will be responsible for allocating animator.
================
*/
idAnimator *idEntity::GetAnimator( void ) {
	return NULL;
}

/*
=============
idEntity::GetRenderView

This is used by remote camera views to look from an entity
=============
*/
renderView_t *idEntity::GetRenderView( void ) {
	if ( !renderView ) {
		renderView = new renderView_t;
	}
	memset( renderView, 0, sizeof( *renderView ) );

	renderView->vieworg = GetPhysics()->GetOrigin();
	renderView->fov_x = cameraFovX;
	renderView->fov_y = cameraFovY;
	renderView->viewaxis = GetPhysics()->GetAxis();

	// copy global shader parms
	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView->shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}

	renderView->globalMaterial = gameLocal.GetGlobalMaterial();

	renderView->time = gameLocal.time;

	return renderView;
}

void idEntity::Activate(idEntity* activator)
{
	// Fire the TRIGGER response
	TriggerResponse(activator, ST_TRIGGER);

	if (RespondsTo(EV_Activate) || HasSignal(SIG_TRIGGER))
	{
		Signal(SIG_TRIGGER);
		ProcessEvent(&EV_Activate, activator);
		TriggerGuis();
	}
}

/***********************************************************************

  Sound
	
***********************************************************************/

/*
================
idEntity::CanPlayChatterSounds

Used for playing chatter sounds on monsters.
================
*/
bool idEntity::CanPlayChatterSounds( void ) const {
	return true;
}

idStr idEntity::GetSoundPropNameForMaterial(const idStr& materialName)
{
	// Object type defaults to "medium" and "hard"
	return idStr("bounce_") + spawnArgs.GetString("spr_object_size", "medium") + 
		"_" + spawnArgs.GetString("spr_object_hardness", "hard") + 
		"_on_" + g_Global.GetSurfaceHardness(materialName);
}

/*
================
idEntity::StartSound
================
*/
bool idEntity::StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length, float propVolMod, int msgTag ) // grayman #3355 
{
	const idSoundShader *shader;
	const char *sound;

	if ( length )
	{
		*length = 0;
	}

	// we should ALWAYS be playing sounds from the def.
	// hardcoded sounds MUST be avoided at all times because they won't get precached.
	assert( idStr::Icmpn( soundName, "snd_", 4 ) == 0 );

	if ( !spawnArgs.GetString( soundName, "", &sound ) )
	{
		return false;
	}

	// ignore empty spawnargs
	if ( sound[0] == '\0' )
	{
		return false;
	}

	if ( !gameLocal.isNewFrame ) 
	{
		// don't play the sound, but don't report an error
		return true;
	}

	// grayman #3355 - run StartSoundShader() first so you can get the
	// length of the sound in ms. Then delay that amount, and 
	// run PropSoundDirect(). This lets associated messages be processed
	// at the end of the sound, rather than the start. More realistic.

	// play the audible sound
	shader = declManager->FindSound( sound );

	bool results = StartSoundShader( shader, channel, soundShaderFlags, broadcast, length );

	int delay = 0;

	if (( msgTag > 0 ) && length )
	{
		delay = *length; // if this sound has a message associated with it, and 'length' is not NULL, delay '*length' ms
	}

	// DarkMod sound propagation:
	PostEventMS(&EV_PropagateSound, delay, soundName, propVolMod, msgTag);

//	PropSoundDirect(soundName, true, false, propVolMod, msgTag); // grayman #3355

	return results;
}

void idEntity::Event_PropSoundDirect( const char *soundName, float propVolMod, int msgTag )
{
	PropSoundDirect(soundName, true, false, propVolMod, msgTag); // grayman #3355
}

/*
================
idEntity::StartSoundShader
================
*/
bool idEntity::StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length) {
	float diversity;
	int len;

	if ( length ) {
		*length = 0;
	}

	if ( !shader ) {
		return false;
	}

	if ( !gameLocal.isNewFrame ) {
		return true;
	}

	// set a random value for diversity unless one was parsed from the entity

	if ( refSound.diversity < 0.0f )
	{
		diversity = gameLocal.random.RandomFloat();

		// grayman #2341 - adjust diversity based on previous sound request,
		//				   to eliminate consecutive sounds. This
		//				   is necessary because sometimes the engine plays the
		//				   wrong sound when the 'no_dup' flag is set.

		int n = shader->GetNumSounds();
		if (n > 1) // duplication is expected when there's only 1 sound choice
		{
			if (channel == SND_CHANNEL_VOICE)
			{
				if ((shader == previousVoiceShader) && (previousVoiceIndex == static_cast<int>(diversity * n) + 1))
				{
					diversity = (static_cast<float> (previousVoiceIndex % n))/(static_cast<float> (n)) + 0.01; // add 0.01 to move off the boundary
				}
			}
			else if (channel == SND_CHANNEL_BODY)
			{
				if ((shader == previousBodyShader) && (previousBodyIndex == static_cast<int>(diversity * n) + 1))
				{
					diversity = (static_cast<float> (previousBodyIndex % n))/(static_cast<float> (n)) + 0.01; // add 0.01 to move off the boundary
				}
			}
		}
	}
	else
	{
		diversity = refSound.diversity;
	}

	// if we don't have a soundEmitter allocated yet, get one now
	if ( !refSound.referenceSound ) {
		refSound.referenceSound = gameSoundWorld->AllocSoundEmitter(GetPhysics()->GetOrigin()); // grayman #4882
	}
			
	UpdateSound(); // grayman #4337

	len = refSound.referenceSound->StartSound( shader, channel, diversity, soundShaderFlags );

	// grayman #2341 - It's rare for the engine to not play the sound, but it can happen. If this happens, don't
	// register this sound as the previous sound played. Stick with the one before that.

	if (len > 0)
	{
		if (channel == SND_CHANNEL_VOICE)
		{
			previousVoiceShader = (idSoundShader*)shader;	// shader for the most recent voice request
			previousVoiceIndex  = static_cast<int>(diversity * shader->GetNumSounds()) + 1;	// index of most recent sound requested (1->N, where there are N sounds)
		}
		else if (channel == SND_CHANNEL_BODY)
		{
			previousBodyShader = (idSoundShader*)shader;	// shader for the most recent body request
			previousBodyIndex  = static_cast<int>(diversity * shader->GetNumSounds()) + 1;	// index of most recent sound requested (1->N, where there are N sounds)
		}
	}

	if ( length ) {
		*length = len;
	}

	// set reference to the sound for shader synced effects
	renderEntity.referenceSound = refSound.referenceSound;

	return true;
}

/*
================
idEntity::StopSound
================
*/
void idEntity::StopSound( const s_channelType channel, bool broadcast ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	if ( refSound.referenceSound ) {
		refSound.referenceSound->StopSound( channel );
	}
}

/*
================
idEntity::SetSoundVolume

  Must be called before starting a new sound.
================
*/
void idEntity::SetSoundVolume( float volume ) {
	refSound.parms.volume = volume;
	refSound.parms.overrideMode |= SSOM_VOLUME_OVERRIDE;
}
void idEntity::SetSoundVolume( void ) {
	refSound.parms.volume = 0.0f;
	refSound.parms.overrideMode &= ~SSOM_VOLUME_OVERRIDE;
}

/*
================
idEntity::UpdateSound
================
*/
void idEntity::UpdateSound( void ) // grayman #4130
{
	if ( refSound.referenceSound )
	{
		idVec3 origin;
		idMat3 axis;

		// grayman #4337 - The fix to #4130 caused sounds on some doors to
		// become faint. Need a better solution.

		if ( GetPhysicsToSoundTransform(origin, axis) )
		{
			refSound.origin = GetPhysics()->GetOrigin() + origin * axis;
		}
		else
		{
			refSound.origin = GetPhysics()->GetOrigin();
		}

		refSound.referenceSound->UpdateEmitter( refSound.origin, refSound.listenerId, &refSound.parms );
	}
}

/*
================
idEntity::GetListenerId
================
*/
int idEntity::GetListenerId( void ) const {
	return refSound.listenerId;
}

/*
================
idEntity::GetSoundEmitter
================
*/
idSoundEmitter *idEntity::GetSoundEmitter( void ) const {
	return refSound.referenceSound;
}

/*
================
idEntity::FreeSoundEmitter
================
*/
void idEntity::FreeSoundEmitter( bool immediate ) {
	if ( refSound.referenceSound ) {
		refSound.referenceSound->Free( immediate );
		refSound.referenceSound = NULL;
	}
}

/*
================
idEntity::PropSoundS
================
*/

// note: the format for kv pair is: <global sound name>:volMod,durMod
// the last two values should be optional, so <global sound name>:volMod,
// and <global sound name>,durMod also work.

void idEntity::PropSoundS( const char *localName, const char *globalName, float VolModIn, int msgTag ) // grayman #3355
{
	if (cv_sndprop_disable.GetBool())
	{
		return;
	}

	// grayman #3393 - don't propagate sounds in early frames,
	// because some AI might not yet be set up to hear them
	if ( gameLocal.time < FIRST_TIME_SOUND_PROP_ALLOWED )
	{
		return;
	}

	int start, end = -1, len;
	bool bHasComma(false), bHasColon(false), bFoundSnd(false);
	float volMod(0.0), durMod(1.0);
	idStr gName(globalName);

	// if there is no local name, skip all the loading of local flags
	// and parms.
	if ( localName == NULL )
	{
		bFoundSnd = gameLocal.m_sndProp->CheckSound( globalName, false );
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("PropSoundS: Propagating global sound %s without checking local sound\r", globalName);
		// note: if we are doing the global only, gName remains set to globalName
		goto Quit;
	}

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("PropSoundS: Found local sound %s, parsing local sound modifiers\r", localName);

	// parse the volMod and durMod if they are tacked on to the globalName
	len = gName.Length();

	// parse volMod, when durMod may or may not be present
	if ( (start = gName.Find(':')) != -1 )
	{
		bHasColon = true;
		start++;
		end = gName.Find(',');

		if( end == -1 )
			end = gName.Length();

		idStr tempstr = gName.Mid(start, (end - start));
		
		if( !tempstr.IsNumeric() || start >= end )
		{
			gameLocal.Warning( "[Soundprop] Bad volume multiplier for sound %s on entity %s.", localName, name.c_str() );
			DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Bad volume multiplier for sound %s on entity %s.\r", localName, name.c_str() );
		}
		else
		{
			volMod = atof( tempstr.c_str() );
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found local volume modifier %f \r", volMod);
		}
	}

	// parse durMod
	if ( (start = gName.Find(',')) != -1 )
	{
		bHasComma = true;
		start++;
		end = gName.Length();

		idStr tempstr = gName.Mid(start, (end - start));
		if ( !tempstr.IsNumeric() || start >= end )
		{
			gameLocal.Warning( "[Soundprop] Bad duration multiplier for sound %s on entity %s.", localName, name.c_str() );
			DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Bad duration multiplier for sound %s on entity %s.\r", localName, name.c_str() );
		}
		else
		{
			durMod = atof( tempstr.c_str() );
			DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found local duration modifier %f \r", durMod);
		}
	}

	// TODO: Get the extra flags, freqmod and bandwidthmod from another key/value pair
	// we will need locName for this.
	
	// strip the durmod and volmod off of the global name
	if ( bHasColon )
	{
		end = gName.Find(':');
	}
	else if ( bHasComma && !bHasColon )
	{
		end = gName.Find(',');
	}
	else if ( !bHasComma && !bHasColon )
	{
		end = gName.Length();
	}

	gName = gName.Mid(0, end);

	bFoundSnd = gameLocal.m_sndProp->CheckSound( gName.c_str(), false );

Quit:
	if ( bFoundSnd )
	{
		// add the input volume modifier
		volMod += VolModIn;

		// grayman #3660 - voices should originate at the head and not at the feet

		idVec3 origin;
		if ( IsType(idAI::Type) &&
			 ((gName == "tell") || (gName == "yell") || (gName == "warn")))
		{
			origin = static_cast<idAI*>(this)->GetEyePosition();
		}
		else
		{
			origin = GetPhysics()->GetOrigin();
		}

		gameLocal.m_sndProp->Propagate( volMod, durMod, gName, 
										origin, 
										//GetPhysics()->GetOrigin(), 
										this, NULL, msgTag ); // grayman #3355
	}

	return;
}

/*
================
idEntity::PropSoundE
================
*/

void idEntity::PropSoundE( const char *localName, const char *globalName, float VolModIn )
{
	bool bFoundSnd(false);
	float volMod(0.0);
	idStr gName(globalName), locName(localName), tempstr;

	if( localName == NULL )
	{
		bFoundSnd = gameLocal.m_sndProp->CheckSound( globalName, true );
		// if we are doing the global only, gName remains set to globalName
		goto Quit;
	}

	// do the parsing of the local vars that modify the Env. sound

	// strip gName of all modifiers at the end

	// add the input volume modifier
	volMod += VolModIn;

Quit:
	
	if( bFoundSnd )
	{
		//TODO : Call soundprop to add env. sound to list
		// have not written this sndprop functionality yet!
	}
	else
	{
		// log error, did not find sound in sound def file
	}
	return;
}

/*
================
idEntity::PropSoundDirect
================
*/

void idEntity::PropSoundDirect( const char *sndName, bool bForceLocal, bool bAssumeEnv, float VolModIn, int msgTag ) // grayman #3355
{
	// grayman #3768 - don't propagate sounds in the early frames
	if (gameLocal.time < FIRST_TIME_SOUND_PROP_ALLOWED)
	{
		return;
	}

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("PropSoundDirect: Attempting to propagate sound \"%s\" Forcelocal = %d\r", sndName, (int) bForceLocal );
	
	// Cut off the "snd_" prefix from the incoming sound name
	idStr sprName = sndName;

	sprName.StripLeading("snd_");

	// Check if we have spr* definitions on the local spawnargs
	idStr sprNameSG;
	idStr sprNameEG;
	bool bIsSusp = spawnArgs.GetString( ("sprS_" + sprName) , "", sprNameSG );
	bool bIsEnv =  spawnArgs.GetString( ("sprE_" + sprName) , "", sprNameEG );

	if (bForceLocal && 
		( !(idStr::Icmpn( sndName, "snd_", 4 ) == 0) || ( !bIsSusp && !bIsEnv ) ) )  
	{
		DM_LOG(LC_SOUND, LT_WARNING)LOGSTRING("Attempted to propagate nonexistant local sound \"%s\" (forceLocal = true)\r", sndName );
		// gameLocal.Warning("[PropSound] Attempted to propagate nonexistant local sound \"%s\" (forceLocal = true)", sndName );
		return;
	}

	if (bIsSusp)
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found local suspicious sound def for %s on entity, attempting to propagate with global sound %s\r", sprName.c_str(), sprNameSG.c_str() );
		PropSoundS( sprName.c_str(), sprNameSG.c_str(), VolModIn, msgTag ); // grayman #3355
		// exit here, because we don't want to allow playing both
		// env. sound AND susp. sound for the same sound and entity
		return;
	}

	if (bIsEnv)
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Found local environmental sound def for %s on entity, attempting to propagating with global sound %s\r", sprName.c_str(), sprNameEG.c_str() );
		PropSoundE( sprName.c_str(), sprNameEG.c_str(), VolModIn );
		return;
	}
	
	// no environmental or suspicious sound 
	// play the unmodified, global sound directly
	sprNameSG = sndName;
	sprNameEG = sndName;
	
	// play the global sound directly.
	if ( bAssumeEnv )
	{
		DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Did not find local def for sound %s, attempting to propagate it as global environmental\r", sprNameEG.c_str() );
		PropSoundE( NULL, sprNameEG.c_str(), VolModIn );
		return;
	}

	DM_LOG(LC_SOUND, LT_DEBUG)LOGSTRING("Did not find local def for sound %s, attempting to propagate it as global suspicious\r", sprNameSG.c_str() );
	PropSoundS( NULL, sprNameSG.c_str(), VolModIn, msgTag ); // grayman #3355
}

/***********************************************************************

  entity binding
	
***********************************************************************/

/*
================
idEntity::PreBind
================
*/
void idEntity::PreBind( void )
{
}

/*
================
idEntity::PostBind
================
*/
void idEntity::PostBind( void )
{
	// #3704: Destroy our frob box if bound to an animated entity.
	if ( bindMaster && bindMaster->IsType(idAnimatedEntity::Type) && m_FrobBox != NULL )
	{
		delete m_FrobBox;
		m_FrobBox = NULL;
	}
}

/*
================
idEntity::PreUnbind
================
*/
void idEntity::PreUnbind( void )
{
}

/*
================
idEntity::PostUnbind
================
*/
void idEntity::PostUnbind( void )
{
}

/*
================
idEntity::InitBind
================
*/
bool idEntity::InitBind( idEntity *master )
{
	if ( master == this ) {
		gameLocal.Error( "Tried to bind '%s' to itself.",GetName());
		return false;
	}

	if ( this == gameLocal.world ) {
		gameLocal.Error( "Tried to bind world to '%s'.",master->GetName());
		return false;
	}

	// unbind myself from my master
	// angua: only do this if the entity is already bound to something
	// and the new master is different from the old one
	if (bindMaster != NULL && master != bindMaster)
	{
		Unbind();
	}

	// add any bind constraints to an articulated figure
	if ( master && IsType( idAFEntity_Base::Type ) ) {
		static_cast<idAFEntity_Base *>(this)->AddBindConstraints();
	}

// TDM: Removed the inability to bind something to the world
// Might have to put it back in if something unforseen happens later because of this
//	if ( !master || master == gameLocal.world ) 
	if ( !master ) 
	{
		// this can happen in scripts, so safely exit out.
		return false;
	}

	return true;
}

// grayman #3156 - make sure everyone on the team has the same cinematic setting

void idEntity::SetCinematicOnTeam(idEntity* ent)
{
	bool cinematicSetting = ent->cinematic;
	idList<idEntity *> children;
	ent->GetTeamChildren(&children);
	for ( int i = 0 ; i < children.Num() ; i++ )
	{
		idEntity *child = children[i];
		if ( child )
		{
			child->cinematic = cinematicSetting;
		}
	}
}

/*
================
idEntity::FinishBind
================
*/
void idEntity::FinishBind( idEntity *newMaster, const char *jointName ) // grayman #3074
{
	// unbind from the previous master (without any pre/post/notify stuff)
	if (bindMaster)
		BreakBindToMaster();

	// bind to the new master (pre/post/notify stuff already done outside)
	EstablishBindToMaster(newMaster);

	// set the master on the physics object
	physics->SetMaster( bindMaster, fl.bindOrientated );

	if (!bindMaster) return; // can be null if self or loop bind

	// if our bindMaster is enabled during a cinematic, we must be, too
	cinematic = bindMaster->cinematic;

	SetCinematicOnTeam(bindMaster); // grayman #3156

	// make sure the team master is active so that physics get run
	teamMaster->BecomeActive( TH_PHYSICS );
	
	// Notify bindmaster of this binding
	bindMaster->BindNotify( this, jointName ); // grayman #3074
}

/*
================
idEntity::Bind

  bind relative to the visual position of the master
================
*/
void idEntity::Bind( idEntity *master, bool orientated )
{
	if ( !InitBind( master ) )
		return;

	// ishtvan: 30/1/2010 : Check for ragdoll adding on idAFAttachment masters when just bind is called
	// (previously only done when bindToJoint or bindToBody was called)
	if	( 
		master->IsType( idAFAttachment::Type )
		&& !IsType( idAnimatedEntity::Type )
		&& ( GetPhysics()->GetClipModel() != NULL )
		&& ( GetPhysics()->GetClipModel()->IsTraceModel() )
		&& ( (GetPhysics()->GetContents() & (CONTENTS_SOLID|CONTENTS_CORPSE)) != 0 ) 
		)
	{
		idAFAttachment *masterAFAtt = static_cast<idAFAttachment *>(master);
		idEntity *masterBody = masterAFAtt->GetBody();
		if( masterBody && masterBody->IsType( idAFEntity_Base::Type ) )
			static_cast<idAFEntity_Base *>(masterBody)->AddEntByJoint( this, masterAFAtt->GetAttachJoint() );
	}


	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	fl.bindOrientated = orientated;

	FinishBind(master, NULL); // grayman #3074

	PostBind( );
}

/*
================
idEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void idEntity::BindToJoint( idEntity *master, const char *jointname, bool orientated ) {
	jointHandle_t	jointnum;
	idAnimator		*masterAnimator;

	masterAnimator = master->GetAnimator();
	if ( !masterAnimator ) 
	{
		gameLocal.Warning( "idEntity::BindToJoint: entity '%s' cannot support skeletal models.", master->GetName() );
		return;
	}

	jointnum = masterAnimator->GetJointHandle( jointname );

	if ( !InitBind( master ) )
		return;

	// Add the ent clipmodel to the AF if appropriate (not done if this ent is an AF)
	if	( 
		(master->IsType( idAFEntity_Base::Type ) || master->IsType( idAFAttachment::Type ))
		&& !IsType( idAnimatedEntity::Type )
		&& ( jointnum != INVALID_JOINT )
		&& ( GetPhysics()->GetClipModel() != NULL )
		&& ( GetPhysics()->GetClipModel()->IsTraceModel() )
		&& ( (GetPhysics()->GetContents() & (CONTENTS_SOLID|CONTENTS_CORPSE)) != 0 ) 
		)
	{
		if( master->IsType( idAFEntity_Base::Type ) )
			static_cast<idAFEntity_Base *>(master)->AddEntByJoint( this, jointnum );
		else if( master->IsType( idAFAttachment::Type ) )
		{
			idEntity *masterBody = static_cast<idAFAttachment *>(master)->GetBody();
			if( masterBody && masterBody->IsType( idAFEntity_Base::Type ) )
				static_cast<idAFEntity_Base *>(masterBody)->AddEntByJoint( this, jointnum );
		}
	}

	if ( jointnum == INVALID_JOINT ) {
		gameLocal.Warning( "idEntity::BindToJoint: joint '%s' not found on entity '%s'.", jointname, master->GetName() );
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	fl.bindOrientated = orientated;

	FinishBind(master, jointname); // grayman #3074

	PostBind();
}

/*
================
idEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void idEntity::BindToJoint( idEntity *master, jointHandle_t jointnum, bool orientated ) 
{
	if ( !InitBind( master ) )
		return;

		// Add the ent clipmodel to the AF if appropriate (not done if this ent is an AF)
	if	( 
		(master->IsType( idAFEntity_Base::Type ) || master->IsType( idAFAttachment::Type ))
		&& !IsType( idAnimatedEntity::Type )
		&& ( jointnum != INVALID_JOINT )
		&& ( GetPhysics()->GetClipModel() != NULL )
		&& ( GetPhysics()->GetClipModel()->IsTraceModel() )
		&& ( (GetPhysics()->GetContents() & (CONTENTS_SOLID|CONTENTS_CORPSE)) != 0 ) 
		)
	{
		if( master->IsType( idAFEntity_Base::Type ) )
			static_cast<idAFEntity_Base *>(master)->AddEntByJoint( this, jointnum );
		else if( master->IsType( idAFAttachment::Type ) )
		{
			idEntity *masterBody = static_cast<idAFAttachment *>(master)->GetBody();
			if( masterBody && masterBody->IsType( idAFEntity_Base::Type ) )
				static_cast<idAFEntity_Base *>(masterBody)->AddEntByJoint( this, jointnum );
		}
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	fl.bindOrientated = orientated;

	idAnimator *masterAnimator = master->GetAnimator();
	const char *jointName = masterAnimator->GetJointName( jointnum );
	FinishBind(master, jointName); // grayman #3074

	PostBind();
}

/*
================
idEntity::BindToBody

  bind relative to a collision model used by the physics of the master
================
*/
void idEntity::BindToBody( idEntity *master, int bodyId, bool orientated ) 
{
	if ( !InitBind( master ) )
		return;

	// Add the ent clipmodel to the AF if appropriate (not done if this ent is an AF)
	if	( 
		(master->IsType( idAFEntity_Base::Type ) || master->IsType( idAFAttachment::Type ))
		&& !IsType( idAnimatedEntity::Type )
		&& ( GetPhysics()->GetClipModel() != NULL )
		&& ( GetPhysics()->GetClipModel()->IsTraceModel() )
		&& ( (GetPhysics()->GetContents() & (CONTENTS_SOLID|CONTENTS_CORPSE)) != 0 ) 
		)
	{
		if( master->IsType( idAFEntity_Base::Type ) )
			static_cast<idAFEntity_Base *>(master)->AddEntByBody( this, bodyId );
		else if( master->IsType( idAFAttachment::Type ) )
		{
			idEntity *masterBody = static_cast<idAFAttachment *>(master)->GetBody();
			if( masterBody && masterBody->IsType( idAFEntity_Base::Type ) )
				static_cast<idAFEntity_Base *>(masterBody)->AddEntByBody( this, bodyId );
		}
	}

	if ( bodyId < 0 )
		gameLocal.Warning( "idEntity::BindToBody: body '%d' not found.", bodyId );

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = bodyId;
	fl.bindOrientated = orientated;

	FinishBind(master, NULL); // grayman #3074

	PostBind();
}


static void ValidateBindTeam_Rec(idEntity* &pos) {
	//first comes the node
	idEntity *ent = pos;
	pos = pos->GetNextTeamEntity();
	//then come the subtrees of its bind sons
	while (pos != NULL && pos->GetBindMaster() == ent)
		ValidateBindTeam_Rec(pos);
}
bool idEntity::ValidateBindTeam( void ) {
	assert(this);
	if (teamMaster == NULL) {
		//no team: isolated entity
		assert(teamChain == NULL);
		assert(bindMaster == NULL);
		return true;
	}
	//check that everyone in team knows their master
	for (idEntity *ent = teamMaster; ent; ent = ent->teamChain) {
		assert(ent->teamMaster == teamMaster);
	}
	//check that team chain is pre-order traversal of the bind tree
	idEntity *pos = teamMaster;
	ValidateBindTeam_Rec(pos);
	assert(pos == NULL);
	return true;
}

void idEntity::EstablishBindToMaster( idEntity *newMaster ) {
	assert(ValidateBindTeam());

	assert(this && !bindMaster && newMaster);
	if (newMaster == this) {
		gameLocal.Warning("Binding entity %s to itself ignored", name.c_str());
		return;
	}
	if (newMaster->IsBoundTo(this)) {
		for (idEntity *ent = newMaster; ent != this; ent = ent->bindMaster)
			gameLocal.Printf("  entity %s\n", ent->name.c_str());
		gameLocal.Warning("Binding entity %s to entity %s ignored to avoid creating a loop", name.c_str(), newMaster->name.c_str());
		return;
	}

	bindMaster = newMaster;

	// check if our new team mate is already on a team
	idEntity *master = newMaster->teamMaster;
	if ( !master ) {
		// he's not on a team, so he's the new teamMaster
		master = newMaster;
		newMaster->teamMaster = newMaster;
		newMaster->teamChain = this;

		// make anyone who's bound to me part of the new team
		for( idEntity *ent = this; ent != NULL; ent = ent->teamChain ) {
			ent->teamMaster = master;
		}
	} else {
		// skip past the chain members bound to the entity we're teaming up with
		idEntity *prev = newMaster;
		idEntity *next = newMaster->teamChain;
		// join after all entities bound to the new master entity
		while( next && next->IsBoundTo( newMaster ) ) {
			prev = next;
			next = next->teamChain;
		}

		// make anyone who's bound to me part of the new team and
		// also find the last member of my team
		idEntity *ent;
		for( ent = this; ent->teamChain != NULL; ent = ent->teamChain ) {
			ent->teamChain->teamMaster = master;
		}

		prev->teamChain = this;
		ent->teamChain = next;
		teamMaster = master;
	}

	assert(ValidateBindTeam());
}

void idEntity::BreakBindToMaster( void ) {
	assert(ValidateBindTeam());
	assert(this && bindMaster);

	idEntity *	prev;
	idEntity *	next;
	idEntity *	last;
	idEntity *	ent;

	// We're still part of a team, so that means I have to extricate myself
	// and any entities that are bound to me from the old team.
	// Find the node previous to me in the team
	prev = teamMaster;
	for( ent = teamMaster->teamChain; ent != NULL && ent != this ; ent = ent->teamChain ) {
		prev = ent;
	}

	assert( ent == this ); // If ent is not pointing to this, then something is very wrong.

	// Find the last node in my team that is bound to me.
	// Also find the first node not bound to me, if one exists.
	last = this;
	for( next = teamChain; next != NULL; next = next->teamChain ) {
		if ( !next->IsBoundTo( this ) ) {
			break;
		}

		// Tell them I'm now the teamMaster
		next->teamMaster = this;
		last = next;
	}

	// disconnect the last member of our team from the old team
	last->teamChain = NULL;

	// connect up the previous member of the old team to the node that
	// follow the last node bound to me (if one exists).
	assert(teamMaster != this);
	prev->teamChain = next;
	if ( !next && ( teamMaster == prev ) ) {
		prev->teamMaster = NULL;
	}

	// If we don't have anyone on our team, then clear the team variables.
	if ( teamChain ) {
		// make myself my own team
		teamMaster = this;
	} else {
		// no longer a team
		teamMaster = NULL;
	}

	idEntity *oldBindMaster = bindMaster;
	//note: bindJoint and bindBody should be saved
	//they are already new if we are rebinding to other entity
	bindMaster = NULL;

	assert(ValidateBindTeam());
	assert(oldBindMaster->ValidateBindTeam());
}

/*
================
idEntity::Unbind
================
*/
void idEntity::Unbind( void ) {
	// remove any bind constraints from an articulated figure
	if ( IsType( idAFEntity_Base::Type ) ) {
		static_cast<idAFEntity_Base *>(this)->RemoveBindConstraints();
	}

	if ( !bindMaster ) {
		return;
	}

	// TDM: Notify bindmaster of unbinding
	bindMaster->UnbindNotify( this );

	if ( !teamMaster ) {
		assert(false);	// must never happen!
		// Teammaster already has been freed
		bindMaster = NULL;
		return;
	}

	PreUnbind();

	if ( physics ) {
		physics->SetMaster( NULL, fl.bindOrientated );
	}

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	BreakBindToMaster();

	PostUnbind();
}

/*
================
idEntity::RemoveBinds
================
*/
void idEntity::RemoveBinds( bool immediately ) {
	//count all entities bound to us
	int k = 0;
	for( idEntity *ent = teamChain; ent != NULL; ent = ent->teamChain )
		if ( ent->bindMaster == this )
			k++;
	if (k == 0)
		return;

	//save all entities bounds to us
	idEntity* *arr = (idEntity**)alloca(k * sizeof(idEntity*));
	k = 0;
	for( idEntity *ent = teamChain; ent != NULL; ent = ent->teamChain )
		if ( ent->bindMaster == this )
			arr[k++] = ent;

	//unbind all saved entities from us
	for (int i = 0; i < k; i++) {
		idEntity *ent = arr[i];
		ent->Unbind();

		if( ent->spawnArgs.GetBool( "removeWithMaster", "1" ) ) {
			//also remove the unbound entity
			if (immediately) {
				//immediately!
				ent->Event_Remove();
			}
			else {
				//on next frame
				ent->PostEventMS( &EV_Remove, 0 );
			}
		}
	}
}

/*
================
idEntity::DetachOnAlert

tels: Remove attached entities when the alert index reaches their "unbindonalertindex".
The attached entities will be removed from the attached list and are also unbound.
================
*/
void idEntity::DetachOnAlert( const int alertIndex )
{
	idEntity *ent = NULL;

	for ( int ind = 0; ind < m_Attachments.Num(); ind ++)
	{
		ent = m_Attachments[ind].ent.GetEntity();
		if( ent && m_Attachments[ind].ent.IsValid() )
		{
			// Crispy: 9999 = "infinity"
			if( alertIndex >= ent->spawnArgs.GetInt( "unbindonalertindex", "9999" ))
			{
				// Tels:
				if ( ent->spawnArgs.GetInt("_spawned_by_anim","0") == 1 )
				{
					// gameLocal.Printf("Removing entity %s spawned by animation\n", ent->GetName() );
					// this entity was spawned automatically by an animation, remove it
					// from the game to prevent left-overs from alerted-during-animation guards
					ent->PostEventMS( &EV_Remove, 0 );
					// and make inactive in the meantime
					ent->BecomeInactive(TH_PHYSICS|TH_THINK);
				}

				DetachInd(ind);	
				CheckAfterDetach(ent); // grayman #2624 - check for frobability and whether to extinguish
				
				// grayman #2624 - account for a falling attachment

				ent->GetPhysics()->Activate();
				ent->m_droppedByAI = true;
				ent->m_SetInMotionByActor = NULL;
				ent->m_MovedByActor = NULL;
			}
		}
	}
}

/*
================
idEntity::IsBound
================
*/
bool idEntity::IsBound( void ) const {
	if ( bindMaster ) {
		return true;
	}
	return false;
}

/*
================
idEntity::IsBoundTo
================
*/
bool idEntity::IsBoundTo( idEntity *master ) const {
	idEntity *ent;

	if ( !bindMaster ) {
		return false;
	}

	for ( ent = bindMaster; ent != NULL; ent = ent->bindMaster ) {
		if ( ent == master ) {
			return true;
		}
	}

	return false;
}

/*
================
idEntity::GetBindMaster
================
*/
idEntity *idEntity::GetBindMaster( void ) const {
	return bindMaster;
}

/*
================
idEntity::GetBindJoint
================
*/
jointHandle_t idEntity::GetBindJoint( void ) const {
	return bindJoint;
}

/*
================
idEntity::GetBindBody
================
*/
int idEntity::GetBindBody( void ) const {
	return bindBody;
}

/*
=====================
idEntity::ConvertLocalToWorldTransform
=====================
*/
void idEntity::ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis ) {
	UpdateModelTransform();

	offset = renderEntity.origin + offset * renderEntity.axis;
	axis *= renderEntity.axis;
}

/*
================
idEntity::GetLocalVector

Takes a vector in worldspace and transforms it into the parent
object's localspace.

Note: Does not take origin into acount.  Use getLocalCoordinate to
convert coordinates.
================
*/
idVec3 idEntity::GetLocalVector( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.ProjectVector( vec, pos );

	return pos;
}

/*
================
idEntity::GetLocalCoordinates

Takes a vector in world coordinates and transforms it into the parent
object's local coordinates.
================
*/
idVec3 idEntity::GetLocalCoordinates( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.ProjectVector( vec - masterOrigin, pos );

	return pos;
}

/*
================
idEntity::GetWorldVector

Takes a vector in the parent object's local coordinates and transforms
it into world coordinates.

Note: Does not take origin into acount.  Use getWorldCoordinate to
convert coordinates.
================
*/
idVec3 idEntity::GetWorldVector( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.UnprojectVector( vec, pos );

	return pos;
}

/*
================
idEntity::GetWorldCoordinates

Takes a vector in the parent object's local coordinates and transforms
it into world coordinates.
================
*/
idVec3 idEntity::GetWorldCoordinates( const idVec3 &vec ) const {
	idVec3	pos;

	if ( !bindMaster ) {
		return vec;
	}

	idVec3	masterOrigin;
	idMat3	masterAxis;

	GetMasterPosition( masterOrigin, masterAxis );
	masterAxis.UnprojectVector( vec, pos );
	pos += masterOrigin;

	return pos;
}

/*
================
idEntity::GetMasterPosition
================
*/
bool idEntity::GetMasterPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const {
	idAnimator	*masterAnimator;

	if ( bindMaster ) 
	{
		// if bound to a joint of an animated model
		if ( bindJoint != INVALID_JOINT ) 
		{
			masterAnimator = bindMaster->GetAnimator();
			if ( !masterAnimator ) 
			{
				masterOrigin = vec3_origin;
				masterAxis = mat3_identity;
				return false;
			} else 
			{
				// Use GetGlobalJointTransform for things bound to weapons
				// This takes into account the view bob, weapon bob and other factors
				if( bindMaster->IsType(idWeapon::Type) )
				{
					static_cast<idWeapon *>(bindMaster)->GetGlobalJointTransform( true, bindJoint, masterOrigin, masterAxis );
				}
				else
				{
					masterAnimator->GetJointTransform( bindJoint, gameLocal.time, masterOrigin, masterAxis );
					masterAxis *= bindMaster->renderEntity.axis;
					masterOrigin = bindMaster->renderEntity.origin + masterOrigin * bindMaster->renderEntity.axis;
				}
			}
		} else if ( bindBody >= 0 && bindMaster->GetPhysics() ) {
			masterOrigin = bindMaster->GetPhysics()->GetOrigin( bindBody );
			masterAxis = bindMaster->GetPhysics()->GetAxis( bindBody );
		} else {
			masterOrigin = bindMaster->renderEntity.origin;
			masterAxis = bindMaster->renderEntity.axis;
		}
		return true;
	} else {
		masterOrigin = vec3_origin;
		masterAxis = mat3_identity;
		return false;
	}
}

/*
================
idEntity::GetWorldVelocities
================
*/
void idEntity::GetWorldVelocities( idVec3 &linearVelocity, idVec3 &angularVelocity ) const {

	linearVelocity = physics->GetLinearVelocity();
	angularVelocity = physics->GetAngularVelocity();

	if ( bindMaster ) {
		idVec3 masterOrigin, masterLinearVelocity, masterAngularVelocity;
		idMat3 masterAxis;

		// get position of master
		GetMasterPosition( masterOrigin, masterAxis );

		// get master velocities
		bindMaster->GetWorldVelocities( masterLinearVelocity, masterAngularVelocity );

		// linear velocity relative to master plus master linear and angular velocity
		linearVelocity = linearVelocity * masterAxis + masterLinearVelocity +
								masterAngularVelocity.Cross( GetPhysics()->GetOrigin() - masterOrigin );
	}
}

idEntity* idEntity::FindMatchingTeamEntity(const idTypeInfo& type, idEntity* lastMatch)
{
	idEntity* part;

	if (lastMatch != NULL)
	{
		for (part = teamChain; part != NULL; part = part->teamChain)
		{
			if (part == lastMatch) 
			{
				// We've found our previous match, increase the pointer and break;
				part = part->teamChain;
				break;
			}
		}
	}
	else
	{
		// No last match specified, set the pointer to the start
		part = teamChain;
	}

	for (/* no initialisation */; part != NULL; part = part->teamChain)
	{
		if (part->IsType(type))
		{
			// Found a suitable one, return it
			return part;
		}
	}

	return NULL;
}

/***********************************************************************

  Physics.
	
***********************************************************************/

/*
================
idEntity::InitDefaultPhysics
================
*/
void idEntity::InitDefaultPhysics( const idVec3 &origin, const idMat3 &axis )
{
	const char *temp;
	idClipModel *clipModel = NULL;

	DM_LOG(LC_ENTITY, LT_DEBUG)LOGSTRING("Entity [%s] test for clipmodel\r", name.c_str());

	// check if a clipmodel key/value pair is set
	if ( spawnArgs.GetString( "clipmodel", "", &temp ) ) {
		if ( idClipModel::CheckModel( temp ) >= 0 ) {
			clipModel = new idClipModel( temp );
		}
	}

	if(!spawnArgs.GetBool( "noclipmodel", "0" ))
	{
		// check if mins/maxs or size key/value pairs are set
		if ( !clipModel )
		{
			idVec3 size;
			idBounds bounds;
			bool setClipModel = false;

			if ( spawnArgs.GetVector( "mins", NULL, bounds[0] ) &&
				spawnArgs.GetVector( "maxs", NULL, bounds[1] ) )
			{
				setClipModel = true;
				if ( bounds[0][0] > bounds[1][0] || bounds[0][1] > bounds[1][1] || bounds[0][2] > bounds[1][2] )
				{
					gameLocal.Error( "Invalid bounds '%s'-'%s' on entity '%s'", bounds[0].ToString(), bounds[1].ToString(), name.c_str() );
				}
			} 
			else
			if ( spawnArgs.GetVector( "size", NULL, size ) )
			{
				if ( ( size.x < 0.0f ) || ( size.y < 0.0f ) || ( size.z < 0.0f ) )
				{
					gameLocal.Error( "Invalid size '%s' on entity '%s'", size.ToString(), name.c_str() );
				}
				bounds[0].Set( size.x * -0.5f, size.y * -0.5f, 0.0f );
				bounds[1].Set( size.x * 0.5f, size.y * 0.5f, size.z );
				setClipModel = true;
			}

			if ( setClipModel ) {
				int numSides;
				idTraceModel trm;

				if ( spawnArgs.GetInt( "cylinder", "0", numSides ) && numSides > 0 ) {
					trm.SetupCylinder( bounds, numSides < 3 ? 3 : numSides );
				} else if ( spawnArgs.GetInt( "cone", "0", numSides ) && numSides > 0 ) {
					trm.SetupCone( bounds, numSides < 3 ? 3 : numSides );
				} else {
					trm.SetupBox( bounds );
				}
				clipModel = new idClipModel( trm );
			}
		}

		// check if the visual model can be used as collision model
		// Updated to take account of skins -- SteveL #4232
		if ( !clipModel ) {
			temp = spawnArgs.GetString( "model" );
			if ( ( temp != NULL ) && ( *temp != 0 ) ) {
				if ( idClipModel::CheckModel( temp, renderEntity.customSkin ) >= 0 ) {
					clipModel = new idClipModel( temp, renderEntity.customSkin );
				}
			}
		}
	}
	else {
		DM_LOG(LC_ENTITY, LT_DEBUG)LOGSTRING("Entity [%s] does not contain a clipmodel\r", name.c_str());
	}

	defaultPhysicsObj.SetSelf( this );
	defaultPhysicsObj.SetClipModel( clipModel, 1.0f );
	defaultPhysicsObj.SetOrigin( origin );
	defaultPhysicsObj.SetAxis( axis );

	physics = &defaultPhysicsObj;

	// create a frob box separate from the collision box for easier frobbing
	bool bUseFrobBox(false);
	idBounds FrobBounds;
	idTraceModel FrobTrm;
	int numSides(0);
	

	// First check if frobbox_mins and frobbox_maxs are set
	if ( spawnArgs.GetVector( "frobbox_mins", NULL, FrobBounds[0] ) &&
				spawnArgs.GetVector( "frobbox_maxs", NULL, FrobBounds[1] ) )
	{
		if ( FrobBounds[0][0] > FrobBounds[1][0] || FrobBounds[0][1] >FrobBounds[1][1] || FrobBounds[0][2] > FrobBounds[1][2] )
		{
			gameLocal.Error( "Invalid frob box bounds '%s'-'%s' on entity '%s'", FrobBounds[0].ToString(), FrobBounds[1].ToString(), name.c_str() );
		}
		bUseFrobBox = true;
	} 
	else 
	{
		float tsize;
		spawnArgs.GetFloat( "frobbox_size", "0.0", tsize );
		if( tsize != 0.0f )
		{
			FrobBounds.Zero();
			FrobBounds.ExpandSelf( tsize );
			bUseFrobBox = true;
		}
	}

	// greebo: Allow the no_frob_box spawnarg to override the settings
	if (spawnArgs.GetBool("no_frob_box", "0"))
	{
		bUseFrobBox = false;
	}

	if( bUseFrobBox )
	{
		if ( spawnArgs.GetInt( "frobbox_cylinder", "0", numSides ) && numSides > 0 ) 
			FrobTrm.SetupCylinder( FrobBounds, numSides < 3 ? 3 : numSides );
		else if ( spawnArgs.GetInt( "frobbox_cone", "0", numSides ) && numSides > 0 )
			FrobTrm.SetupCone( FrobBounds, numSides < 3 ? 3 : numSides );
		else
			FrobTrm.SetupBox( FrobBounds );

		// Initialize frob bounds based on previous spawnarg setup
		m_FrobBox = new idClipModel( FrobTrm );
		m_FrobBox->Link( gameLocal.clip, this, 0, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
		// don't set contents of frob box here, wait for frobbing initialization
	}
	else
		m_FrobBox = NULL;
}

/*
================
idEntity::SetPhysics
================
*/
void idEntity::SetPhysics( idPhysics *phys ) {
	// clear any contacts the current physics object has
	if ( physics ) {
		physics->ClearContacts();
	}
	// set new physics object or set the default physics if NULL
	if ( phys != NULL ) {
		defaultPhysicsObj.SetClipModel( NULL, 1.0f );
		physics = phys;
		physics->Activate();
	} else {
		physics = &defaultPhysicsObj;
	}
	physics->UpdateTime( gameLocal.time );
	physics->SetMaster( bindMaster, fl.bindOrientated );
}

/*
================
idEntity::RestorePhysics
================
*/
void idEntity::RestorePhysics( idPhysics *phys ) {
	assert( phys != NULL );
	// restore physics pointer
	physics = phys;
}

/*
================
idEntity::RunPhysics
================
*/
bool idEntity::RunPhysics( void ) {
	int			i, reachedTime, startTime, endTime;
	idEntity *	part, *blockedPart, *blockingEntity(NULL);
	bool		moved;

	// don't run physics if not enabled
	if ( !( thinkFlags & TH_PHYSICS ) ) {
		// however do update any animation controllers
		if ( UpdateAnimationControllers() ) {
			BecomeActive( TH_ANIMATE );
		}
		return false;
	}

	// if this entity is a team slave don't do anything because the team master will handle everything
	if ( teamMaster && teamMaster != this ) {
		return false;
	}

	// angua: since the AI are not thinking every frame, we need to rescale 
	// their velocities with the corrected time length to prevent them from dying.
	if (IsType(idAI::Type))
	{
		startTime = static_cast<idAI*>(this)->m_lastThinkTime;
	}
	else
	{
		startTime = gameLocal.previousTime;
	}
	endTime = gameLocal.time;

	gameLocal.push.InitSavingPushedEntityPositions();
	blockedPart = NULL;

	// save the physics state of the whole team and disable the team for collision detection
	for ( part = this; part != NULL; part = part->teamChain ) {
		if ( part->physics ) {
			if ( !part->fl.solidForTeam ) {
				part->physics->DisableClip();
			}
			part->physics->SaveState();
		}
	}

	// move the whole team
	for ( part = this; part != NULL; part = part->teamChain ) {

		if ( part->physics )
		{
			// run physics
			moved = part->physics->Evaluate( endTime - startTime, endTime );

			// check if the object is blocked
			blockingEntity = part->physics->GetBlockingEntity();
			if ( blockingEntity ) {
				blockedPart = part;
				break;
			}

			// if moved or forced to update the visual position and orientation from the physics
			if ( moved || part->fl.forcePhysicsUpdate ) {
				part->UpdateFromPhysics( false );
			}

			// update any animation controllers here so an entity bound
			// to a joint of this entity gets the correct position
			if ( part->UpdateAnimationControllers() ) {
				part->BecomeActive( TH_ANIMATE );
			}
		}
	}

	// enable the whole team for collision detection
	for ( part = this; part != NULL; part = part->teamChain ) {
		if ( part->physics ) 
		{
// Ish: I think Id screwed up here, but am not positive
			// if ( !part->fl.solidForTeam ) {
				part->physics->EnableClip();
			// }
		}
	}

	// if one of the team entities is a pusher and blocked
	if ( blockedPart ) {
		// move the parts back to the previous position
		for ( part = this; part != blockedPart; part = part->teamChain ) {

			if ( part->physics ) {
				// restore the physics state
				part->physics->RestoreState();

				// move back the visual position and orientation
				part->UpdateFromPhysics( true );
			}
		}
		for ( part = this; part != NULL; part = part->teamChain ) {
			if ( part->physics ) {
				// update the physics time without moving
				part->physics->UpdateTime( endTime );
			}
		}

		// greebo: Apply the "reaction" to the team master
		if (physics->IsType(idPhysics_RigidBody::Type))
		{
			idPhysics_RigidBody* rigidBodyPhysics = static_cast<idPhysics_RigidBody*>(physics);

			// Calculate the movement (proportional to kinetic energy)
			float movement = rigidBodyPhysics->GetLinearVelocity().LengthSqr() + 
				              rigidBodyPhysics->GetAngularVelocity().LengthSqr();

			//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Movement is %f\r", movement);

			if (movement < 10.0f)
			{
				DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Putting %s to rest, velocity was %f\r", name.c_str(), physics->GetLinearVelocity().LengthFast());
				physics->PutToRest();
			}
			else
			{
				// Create a dummy impulse vector, it gets overwritten by the CollisionImpulse method anyway
				idVec3 impulse(0,0,0);
				rigidBodyPhysics->CollisionImpulse(*blockedPart->GetPhysics()->GetBlockingInfo(), impulse);

				// greebo: Apply some damping due to the collision with a slave
				rigidBodyPhysics->DampenMomentums(0.95f, 0.99f);
			}
		}

		// restore the positions of any pushed entities
		gameLocal.push.RestorePushedEntityPositions();

		// if the master pusher has a "blocked" function, call it
		Signal( SIG_BLOCKED );
		ProcessEvent( &EV_TeamBlocked, blockedPart, blockingEntity );
		// call the blocked function on the blocked part
		blockedPart->ProcessEvent( &EV_PartBlocked, blockingEntity );
		return false;
	}

	// set pushed
	for ( i = 0; i < gameLocal.push.GetNumPushedEntities(); i++ ) {
		idEntity *ent = gameLocal.push.GetPushedEntity( i );
		ent->physics->SetPushed( endTime - startTime );
	}

	// post reached event if the current time is at or past the end point of the motion
	for ( part = this; part != NULL; part = part->teamChain ) {

		if ( part->physics ) {

			reachedTime = part->physics->GetLinearEndTime();
			if ( startTime < reachedTime && endTime >= reachedTime ) {
				part->ProcessEvent( &EV_ReachedPos );
			}
			reachedTime = part->physics->GetAngularEndTime();
			if ( startTime < reachedTime && endTime >= reachedTime ) {
				part->ProcessEvent( &EV_ReachedAng );
			}
		}
	}

	return true;
}

/*
================
idEntity::UpdateFromPhysics
================
*/
void idEntity::UpdateFromPhysics( bool moveBack ) {

	if ( IsType( idActor::Type ) ) {
		idActor *actor = static_cast<idActor *>( this );

		// set master delta angles for actors
		if ( GetBindMaster() ) {
			idAngles delta = actor->GetDeltaViewAngles();
			if ( moveBack ) {
				delta.yaw -= static_cast<idPhysics_Actor *>(physics)->GetMasterDeltaYaw();
			} else {
				delta.yaw += static_cast<idPhysics_Actor *>(physics)->GetMasterDeltaYaw();
			}
			actor->SetDeltaViewAngles( delta );
		}
	}

	UpdateVisuals();
}

/*
================
idEntity::SetOrigin
================
*/
void idEntity::SetOrigin( const idVec3 &org ) {

	GetPhysics()->SetOrigin( org );

	UpdateVisuals();
}

/*
================
idEntity::SetAxis
================
*/
void idEntity::SetAxis( const idMat3 &axis ) {

	if ( GetPhysics()->IsType( idPhysics_Actor::Type ) ) {
		static_cast<idActor *>(this)->viewAxis = axis;
	} else {
		GetPhysics()->SetAxis( axis );
	}

	UpdateVisuals();
}

/*
================
idEntity::SetAngles
================
*/
void idEntity::SetAngles( const idAngles &ang ) {
	SetAxis( ang.ToMat3() );
}

/*
================
idEntity::GetFloorPos
================
*/
bool idEntity::GetFloorPos( float max_dist, idVec3 &floorpos ) const {
	trace_t result;

	if ( !GetPhysics()->HasGroundContacts() ) {
		GetPhysics()->ClipTranslation( result, GetPhysics()->GetGravityNormal() * max_dist, NULL );
		if ( result.fraction < 1.0f ) {
			floorpos = result.endpos;
			return true;
		} else {
			floorpos = GetPhysics()->GetOrigin();
			return false;
		}
	} else {
		floorpos = GetPhysics()->GetOrigin();
		return true;
	}
}

/*
================
idEntity::GetPhysicsToVisualTransform
================
*/
bool idEntity::GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis ) {
	return false;
}

/*
================
idEntity::GetPhysicsToSoundTransform
================
*/
bool idEntity::GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis ) {
	// by default play the sound at the center of the bounding box of the first clip model
	if ( GetPhysics()->GetNumClipModels() > 0 ) {
		origin = GetPhysics()->GetBounds().GetCenter();
		axis.Identity();
		return true;
	}
	return false;
}

/*
================
idEntity::Collide
================
*/

bool idEntity::Collide( const trace_t &collision, const idVec3 &velocity ) {
	return false;
}
/*
================
idEntity::GetImpactInfo
================
*/
void idEntity::GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info ) {
	GetPhysics()->GetImpactInfo( id, point, info );
}

/*
================
idEntity::ApplyImpulse
================
*/
void idEntity::ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse ) {

	// grayman #2603 - disallow impulse on candles and candle holders when the candle is being relit

	bool allowImpulse = true;

	// In case the entity itself is a light (tdm_light_holder light)

	if (IsType(idLight::Type))
	{
		if (static_cast<idLight*>(this)->IsBeingRelit())
		{
			allowImpulse = false; // relighting this light; no impulse
		}
	}
	else if (IsType(idMoveable::Type))
	{
		// Are there any light entities on this team?

		idList<idEntity *> children;
		GetTeamChildren(&children);
		for (int i = 0 ; i < children.Num() ; i++)
		{
			// Based on reading other uses of GetTeamChildren(), it's
			// prudent to check for recursion back to the original entity.

			idEntity* child = children[i];
			if (this == child)
			{
				break;
			}
			if (child->IsType(idLight::Type))
			{
				if (static_cast<idLight*>(child)->IsBeingRelit())
				{
					allowImpulse = false; // relighting this light; no impulse
					break;
				}
			}
		}
	}

	//stgatilov #5599: skip impulses in silent mode of grabber
	if (gameLocal.m_Grabber->GetSelected() == ent && gameLocal.m_Grabber->IsInSilentMode())
		allowImpulse = false;

	if (allowImpulse)
	{
		GetPhysics()->ApplyImpulse( id, point, impulse );
	}
}

/*
================
idEntity::AddForce
================
*/
void idEntity::AddForce( idEntity *ent, int bodyId, const idVec3 &point, const idVec3 &force, const idForceApplicationId &applId ) {
	GetPhysics()->AddForce( bodyId, point, force, applId );
}

/*
================
idEntity::ActivatePhysics
================
*/
void idEntity::ActivatePhysics( idEntity *ent ) {
	GetPhysics()->Activate();
}

/*
================
idEntity::IsAtRest
================
*/
bool idEntity::IsAtRest( void ) const {
	return GetPhysics()->IsAtRest();
}

/*
================
idEntity::GetRestStartTime
================
*/
int idEntity::GetRestStartTime( void ) const {
	return GetPhysics()->GetRestStartTime();
}

/*
================
idEntity::AddContactEntity
================
*/
void idEntity::AddContactEntity( idEntity *ent ) {
	GetPhysics()->AddContactEntity( ent );
}

/*
================
idEntity::RemoveContactEntity
================
*/
void idEntity::RemoveContactEntity( idEntity *ent ) {
	GetPhysics()->RemoveContactEntity( ent );
}

// grayman #3011 - Activate physics thinking for any
// entities sitting on this entity.

// This could be extended in the future to activate all contact entities.

/*
================
idEntity::Event_ActivateContacts
================
*/
void idEntity::Event_ActivateContacts()
{
	ActivateContacts();
}

/*
================
idEntity::ActivateContacts
================
*/
void idEntity::ActivateContacts()
{
	// nbohr1more: #3871 - increase contact limit to 128 watch for future issues with this limit
	// stgatilov: lowered back to 32 --- the same number is now used in physics classes
	contactInfo_t contacts[CONTACTS_MAX_NUMBER];

	idVec6 dir;
	int num;

	dir.SubVec3(0) = -GetPhysics()->GetGravityNormal(); // look up
	dir.SubVec3(1) = vec3_origin; // ignore angular velocity
	idClipModel *clipModel = GetPhysics()->GetClipModel();

	if ( clipModel->IsTraceModel() )
	{
		num = gameLocal.clip.Contacts( contacts, CONTACTS_MAX_NUMBER, GetPhysics()->GetOrigin(),dir, CONTACT_EPSILON, clipModel, mat3_identity, CONTENTS_SOLID, this );
	}
	else
	{
		// this entity has no trace model, so create a new clip model
		// and give it a trace model that can be used for the search
	
		idTraceModel trm(GetPhysics()->GetBounds());
		idClipModel clip(trm);
		num = gameLocal.clip.Contacts( contacts, CONTACTS_MAX_NUMBER, GetPhysics()->GetOrigin(),dir, CONTACT_EPSILON, &clip, mat3_identity, CONTENTS_SOLID, this );
	}

	for ( int i = 0 ; i < num ; i++ )
	{
		idEntity* found = gameLocal.entities[contacts[i].entityNum];
		if ( found != gameLocal.world )
		{
			if ( found && found->IsType(idMoveable::Type) )
			{
				found->ActivatePhysics( this ); // let this object find that it's sitting on air
			}
		}
	}
}

/***********************************************************************

	Damage
	
***********************************************************************/

/*
============
idEntity::CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool idEntity::CanDamage( const idVec3 &origin, idVec3 &damagePoint ) const {
	idVec3 	dest;
	trace_t	tr;
	idVec3 	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin at 0,0,0
	midpoint = ( GetPhysics()->GetAbsBounds()[0] + GetPhysics()->GetAbsBounds()[1] ) * 0.5;

	dest = midpoint;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	// this should probably check in the plane of projection, rather than in world coordinate
	dest = midpoint;
	dest[0] += 15.0;
	dest[1] += 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] += 15.0;
	dest[1] -= 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] += 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] += 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] -= 15.0;
	gameLocal.clip.TracePoint( tr, origin, dest, MASK_SOLID, NULL );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	return false;
}

/*
================
idEntity::DamageFeedback

callback function for when another entity recieved damage from this entity.  damage can be adjusted and returned to the caller.
================
*/
void idEntity::DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage ) {
	// implemented in subclasses
}

/*
============
Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted

inflictor, attacker, dir, and point can be NULL for environmental effects

============
*/
void idEntity::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, 
					  const char *damageDefName, const float damageScale, 
					  const int location, trace_t *tr ) 
{
	if ( !fl.takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}

	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	const idDict *damageDef = gameLocal.FindEntityDefDict( damageDefName, true ); // grayman #3391 - don't create a default 'damageDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if ( !damageDef )
	{
		gameLocal.Error( "Unknown damageDef '%s'\n", damageDefName );
	}

	int	damage = damageDef->GetInt( "damage" ) * damageScale;

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, damage );
	if ( damage )
	{
		// do the damage
		health -= damage;
		if ( health <= 0 )
		{
			if ( health < -999 )
			{
				health = -999;
			}

			Killed( inflictor, attacker, damage, dir, location );
		}
		else
		{
			Pain( inflictor, attacker, damage, dir, location, damageDef );
		}
	}
}

/*
================
idEntity::AddDamageEffect
================
*/
void idEntity::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) {
	const char *decal, *key;
	idStr surfName;

	const idDeclEntityDef *def = gameLocal.FindEntityDef( damageDefName, false );
	if ( def == NULL ) {
		return;
	}

	g_Global.GetSurfName( collision.c.material, surfName );

	// start impact sound based on material type
	key = va( "snd_%s", surfName.c_str() );
	
	// ishtvan: No need to play the sound here anymore, right?
/* 
	sound = spawnArgs.GetString( key );
	if ( *sound == '\0' ) {
		sound = def->dict.GetString( key );
	}
	if ( *sound != '\0' ) {
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}
*/

	if ( g_decals.GetBool() ) {
		// place a wound overlay on the model
		key = va( "mtr_wound_%s", surfName.c_str() );
		decal = spawnArgs.RandomPrefix( key, gameLocal.random );
		if ( *decal == '\0' ) {
			decal = def->dict.RandomPrefix( key, gameLocal.random );
		}
		if ( *decal != '\0' ) {
			idVec3 dir = velocity;
			dir.Normalize();
			ProjectOverlay( collision.c.point, dir, 20.0f, decal );
		}
	}
}

/*
============
idEntity::Pain

Called whenever an entity recieves damage.  Returns whether the entity responds to the pain.
This is a virtual function that subclasses are expected to implement.
============
*/
bool idEntity::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const idDict* damageDef ) {
	return false;
}

/*
============
idEntity::Killed

Called whenever an entity's health is reduced to 0 or less.
This is a virtual function that subclasses are expected to implement.
============
*/
void idEntity::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location ) {
}


/***********************************************************************

  Script functions
	
***********************************************************************/

/*
================
idEntity::ShouldConstructScriptObjectAtSpawn

Called during idEntity::Spawn to see if it should construct the script object or not.
Overridden by subclasses that need to spawn the script object themselves.
================
*/
bool idEntity::ShouldConstructScriptObjectAtSpawn( void ) const {
	return true;
}

/*
================
idEntity::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
idThread *idEntity::ConstructScriptObject( void ) {
	idThread		*thread;
	const function_t *constructor;

	// init the script object's data
	scriptObject.ClearObject();

	// call script object's constructor (usually "init()")
	constructor = scriptObject.GetConstructor();
	if ( constructor ) {
		// start a thread that will initialize after Spawn is done being called
		thread = new idThread();
		thread->SetThreadName( name.c_str() );
		thread->CallFunction( this, constructor, true );
		thread->DelayedStart( 0 );
	} else {
		thread = NULL;
	}

	// clear out the object's memory
	scriptObject.ClearObject();

	return thread;
}

/*
================
idEntity::DeconstructScriptObject

Called during idEntity::~idEntity.  Calls the destructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
Not called during idGameLocal::MapShutdown.
================
*/
void idEntity::DeconstructScriptObject( void ) {
	idThread		*thread;
	const function_t *destructor;

	// don't bother calling the script object's destructor on map shutdown
	if ( gameLocal.GameState() == GAMESTATE_SHUTDOWN ) {
		return;
	}

	// call script object's destructor
	destructor = scriptObject.GetDestructor();
	if ( destructor ) {
		// start a thread that will run immediately and be destroyed
		thread = new idThread();
		thread->SetThreadName( name.c_str() );
		thread->CallFunction( this, destructor, true );
		thread->Execute();
		delete thread;
	}
}

/*
================
idEntity::HasSignal
================
*/
bool idEntity::HasSignal( signalNum_t signalnum ) const {
	if ( !signals ) {
		return false;
	}
	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );
	return ( signals->signal[ signalnum ].Num() > 0 );
}

/*
================
idEntity::SetSignal
================
*/
void idEntity::SetSignal( signalNum_t signalnum, idThread *thread, const function_t *function ) {
	int			i;
	int			num;
	signal_t	sig;
	int			threadnum;

	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );

	if ( !signals ) {
		signals = new signalList_t;
	}

	assert( thread );
	threadnum = thread->GetThreadNum();

	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		if ( signals->signal[ signalnum ][ i ].threadnum == threadnum ) {
			signals->signal[ signalnum ][ i ].function = function;
			return;
		}
	}

	if ( num >= MAX_SIGNAL_THREADS ) {
		thread->Error( "Exceeded maximum number of signals per object" );
	}

	sig.threadnum = threadnum;
	sig.function = function;
	signals->signal[ signalnum ].Append( sig );
}

/*
================
idEntity::ClearSignal
================
*/
void idEntity::ClearSignal( idThread *thread, signalNum_t signalnum ) {
	assert( thread );
	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	signals->signal[ signalnum ].Clear();
}

/*
================
idEntity::ClearSignalThread
================
*/
void idEntity::ClearSignalThread( signalNum_t signalnum, idThread *thread ) {
	int	i;
	int	num;
	int	threadnum;

	assert( thread );

	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	threadnum = thread->GetThreadNum();

	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		if ( signals->signal[ signalnum ][ i ].threadnum == threadnum ) {
			signals->signal[ signalnum ].RemoveIndex( i );
			return;
		}
	}
}

/*
================
idEntity::Signal
================
*/
void idEntity::Signal( signalNum_t signalnum ) {
	int			i;
	int			num;
	signal_t	sigs[ MAX_SIGNAL_THREADS ];
	idThread	*thread;

	assert( ( signalnum >= 0 ) && ( signalnum < NUM_SIGNALS ) );

	if ( !signals ) {
		return;
	}

	// we copy the signal list since each thread has the potential
	// to end any of the threads in the list.  By copying the list
	// we don't have to worry about the list changing as we're
	// processing it.
	num = signals->signal[ signalnum ].Num();
	for( i = 0; i < num; i++ ) {
		sigs[ i ] = signals->signal[ signalnum ][ i ];
	}

	// clear out the signal list so that we don't get into an infinite loop
	// TDM: Removed this, signal should stay after triggering by default
	// signals->signal[ signalnum ].Clear();

	for( i = 0; i < num; i++ ) {
		thread = idThread::GetThread( sigs[ i ].threadnum );
		if ( thread ) 
		{
			thread->CallFunction( this, sigs[ i ].function, true );
			thread->Execute();
		}
		// TDM: Create a new thread if the thread that added the signal is not still around
		else
		{
			thread = new idThread(sigs[i].function);
			thread->CallFunction( this, sigs[ i ].function, true );
			thread->Execute();
		}
	}
}

/*
================
idEntity::SignalEvent
================
*/
void idEntity::SignalEvent( idThread *thread, signalNum_t signalnum ) {
	if ( ( signalnum < 0 ) || ( signalnum >= NUM_SIGNALS ) ) {
		gameLocal.Error( "Signal out of range" );
	}

	if ( !signals ) {
		return;
	}

	Signal( signalnum );
}

/***********************************************************************

  Guis.
	
***********************************************************************/


/*
================
idEntity::TriggerGuis
================
*/
void idEntity::TriggerGuis( void ) {
	int i;
	for ( i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
		if ( renderEntity.gui[ i ] ) {
			renderEntity.gui[ i ]->Trigger( gameLocal.time );
		}
	}
}

/*
================
idEntity::HandleGuiCommands
================
*/
bool idEntity::HandleGuiCommands( idEntity *entityGui, const char *cmds ) {
	idEntity *targetEnt;
	bool ret = false;
	if ( entityGui && cmds && *cmds ) {
		idLexer src;
		idToken token, token2, token3, token4;
        src.LoadMemory(cmds, static_cast<int>(strlen(cmds)), "guiCommands");
		while( 1 ) {

			if ( !src.ReadToken( &token ) ) {
				return ret;
			}

			if ( token == ";" ) {
				continue;
			}

			if ( token.Icmp( "activate" ) == 0 ) {
				bool targets = true;
				if ( src.ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src.UnreadToken( &token2 );
					} else {
						targets = false;
					}
				}

				if ( targets ) {
					entityGui->ActivateTargets( this );
				} else {
					idEntity *ent = gameLocal.FindEntity( token2 );
					if ( ent ) {
						ent->Signal( SIG_TRIGGER );
						ent->PostEventMS( &EV_Activate, 0, this );
					}
				}

				entityGui->renderEntity.shaderParms[ SHADERPARM_MODE ] = 1.0f;
				continue;
			}


			if ( token.Icmp( "runScript" ) == 0 ) {
				if ( src.ReadToken( &token2 ) ) {
					while( src.CheckTokenString( "::" ) ) {
						idToken token3;
						if ( !src.ReadToken( &token3 ) ) {
							gameLocal.Error( "Expecting function name following '::' in gui for entity '%s'", entityGui->name.c_str() );
						}
						token2 += "::" + token3;
					}
					const function_t *func = gameLocal.program.FindFunction( token2 );
					if ( !func ) {
						gameLocal.Warning( "Can't find function '%s' for gui in entity '%s'", token2.c_str(), entityGui->name.c_str() );
					} else {
						idThread *thread = new idThread( func );
						thread->DelayedStart( 0 );
					}
				}
				continue;
			}

			if ( token.Icmp("play") == 0 ) {
				if ( src.ReadToken( &token2 ) ) {
					const idSoundShader *shader = declManager->FindSound(token2);
					entityGui->StartSoundShader( shader, SND_CHANNEL_ANY, 0, false, NULL );
				}
				continue;
			}

			if ( token.Icmp( "setkeyval" ) == 0 ) {
				if ( src.ReadToken( &token2 ) && src.ReadToken(&token3) && src.ReadToken( &token4 ) ) {
					idEntity *ent = gameLocal.FindEntity( token2 );
					if ( ent ) {
						ent->spawnArgs.Set( token3, token4 );
						ent->UpdateChangeableSpawnArgs( NULL );
						ent->UpdateVisuals();
					}
				}
				continue;
			}

			if ( token.Icmp( "setshaderparm" ) == 0 ) {
				if ( src.ReadToken( &token2 ) && src.ReadToken(&token3) ) {
					entityGui->SetShaderParm( atoi( token2 ), atof( token3 ) );
					entityGui->UpdateVisuals();
				}
				continue;
			}

			if ( token.Icmp("close") == 0 ) {
				ret = true;
				continue;
			}

			// handy for debugging GUI stuff
			if ( !token.Icmp( "print" ) ) {
				idStr msg;
				while ( src.ReadToken( &token2 ) ) {
					if ( token2 == ";" ) {
						src.UnreadToken( &token2 );
						break;
					}
					msg += token2.c_str();
				}
				common->Printf( "ent gui 0x%x '%s': %s\n", entityNumber, name.c_str(), msg.c_str() );
				continue;
			}

			// if we get to this point we don't know how to handle it
			src.UnreadToken(&token);
			if ( !HandleSingleGuiCommand( entityGui, &src ) ) {
				// not handled there see if entity or any of its targets can handle it
				// this will only work for one target atm
				if ( entityGui->HandleSingleGuiCommand( entityGui, &src ) ) {
					continue;
				}

				int c = entityGui->targets.Num();
				int i;
				for ( i = 0; i < c; i++) {
					targetEnt = entityGui->targets[ i ].GetEntity();
					if ( targetEnt && targetEnt->HandleSingleGuiCommand( entityGui, &src ) ) {
						break;
					}
				}

				if ( i == c ) {
					// not handled
					common->DPrintf( "idEntity::HandleGuiCommands: '%s' not handled\n", token.c_str() );
					src.ReadToken( &token );
				}
			}

		}
	}
	return ret;
}

/*
================
idEntity::HandleSingleGuiCommand
================
*/
bool idEntity::HandleSingleGuiCommand( idEntity *entityGui, idLexer *src ) {
	return false;
}

/***********************************************************************

  Targets
	
***********************************************************************/

/*
===============
idEntity::FindTargets

We have to wait until all entities are spawned
Used to build lists of targets after the entity is spawned.  Since not all entities
have been spawned when the entity is created at map load time, we have to wait
===============
*/
void idEntity::FindTargets( void ) {
	int i;

	// targets can be a list of multiple names
	gameLocal.GetTargets( spawnArgs, targets, "target" );

	// grayman #2603 - add relight positions as targets
	gameLocal.GetRelights(spawnArgs,targets,"relight_position");

	// ensure that we don't target ourselves since that could cause an infinite loop when activating entities
	for( i = 0; i < targets.Num(); i++ ) {
		if ( targets[ i ].GetEntity() == this ) {
			gameLocal.Error( "Entity '%s' is targeting itself", name.c_str() );
		}
	}
}

// grayman #2603 - convert relight_positions to targets

void idEntity::FindRelights(void)
{
	gameLocal.GetRelights(spawnArgs,targets,"relight_position");
}

/*
================
idEntity::RemoveNullTargets
================
*/
void idEntity::RemoveNullTargets( void ) {
	int i;

	for( i = targets.Num() - 1; i >= 0; i-- ) {
		if ( !targets[ i ].GetEntity() ) {
			targets.RemoveIndex( i );
		}
	}
}

/*
==============================
idEntity::ActivateTargets

"activator" should be set to the entity that initiated the firing.
==============================
*/
void idEntity::ActivateTargets( idEntity *activator ) const
{
	for (int i = 0; i < targets.Num(); i++ )
	{
		idEntity* ent = targets[i].GetEntity();

		if (ent == NULL)
		{
			continue;
		}

		// gameLocal.Printf("Activating entity '%s' (from '%s')\n", ent->GetName(), activator == NULL ? "NULL" : activator->GetName());
		
		// Call the virtual function
		ent->Activate(activator);
		 		
		for (int j = 0; j < MAX_RENDERENTITY_GUI; j++)
		{
			if ( ent->renderEntity.gui[j] ) {
				ent->renderEntity.gui[j]->Trigger(gameLocal.time);
			}
		}
	}
}

void idEntity::RemoveTarget(idEntity* target)
{
	for (int i = 0; i < targets.Num(); i++)
	{
		if (targets[i].GetEntity() == target)
		{
			targets.RemoveIndex(i);
			return;
		}
	}
}

void idEntity::AddTarget(idEntity* target)
{
	targets.AddUnique(target);
}

/***********************************************************************

  Misc.
	
***********************************************************************/

/*
================
idEntity::Teleport

TODO: This does not set the view angles, so if you teleport the player, he will
	  not copy the view from the target, e.g. still look at the same direction.
================
*/
void idEntity::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {

	if (destination == NULL)
	{
		GetPhysics()->SetOrigin( origin );
		GetPhysics()->SetAxis( angles.ToMat3() );
	}
	else
	{
		// tels: copy origin and angles from the destination, but
		//		 use potential "teleport_offset" and "teleport_random_offset" spawnargs
		idVec3 offset = spawnArgs.GetVector( "teleport_offset", "0 0 0" );
		idVec3 rand_offset = spawnArgs.GetVector( "teleport_random_offset", "0 0 0" );

		// replace "3 0 0" with a value of "-1.5 .. 1.5, 0, 0"
		rand_offset.x = gameLocal.random.RandomFloat() * idMath::Fabs(rand_offset.x) - idMath::Fabs(rand_offset.x) / 2;
		rand_offset.y = gameLocal.random.RandomFloat() * idMath::Fabs(rand_offset.y) - idMath::Fabs(rand_offset.y) / 2;
		rand_offset.z = gameLocal.random.RandomFloat() * idMath::Fabs(rand_offset.z) - idMath::Fabs(rand_offset.z) / 2;

		GetPhysics()->SetOrigin( destination->GetPhysics()->GetOrigin() + offset + rand_offset );
		GetPhysics()->SetAxis( destination->GetPhysics()->GetAxis() );
	}

	UpdateVisuals();
}

/*
============
idEntity::TouchTriggers

  Activate all trigger entities touched at the current position.
============
*/
bool idEntity::TouchTriggers( void ) const {
	int				i, numClipModels, numEntities;
	idClipModel *	cm;
	idEntity *		ent;
	trace_t			trace;

	memset( &trace, 0, sizeof( trace ) );
	trace.endpos = GetPhysics()->GetOrigin();
	trace.endAxis = GetPhysics()->GetAxis();

	idClip_ClipModelList clipModels;
	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( GetPhysics()->GetAbsBounds(), CONTENTS_TRIGGER, clipModels );
	numEntities = 0;

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModels[ i ];

		// don't touch it if we're the owner
		if ( cm->GetOwner() == this ) {
			continue;
		}

		ent = cm->GetEntity();

		if ( !ent->RespondsTo( EV_Touch ) && !ent->HasSignal( SIG_TOUCH ) ) {
			continue;
		}

		if ( !GetPhysics()->ClipContents( cm ) ) {
			continue;
		}

		numEntities++;

		trace.c.contents = cm->GetContents();
		trace.c.entityNum = cm->GetEntity()->entityNumber;
		trace.c.id = cm->GetId();

		ent->Signal( SIG_TOUCH );
		ent->ProcessEvent( &EV_Touch, this, &trace );

		if ( !gameLocal.entities[ entityNumber ] ) {
			gameLocal.Printf( "entity was removed while touching triggers\n" );
			return true;
		}
	}

	return ( numEntities != 0 );
}

/*
================
idEntity::GetSpline
================
*/
idCurve_Spline<idVec3> *idEntity::GetSpline( void ) const {
	int i, numPoints, t;
	const idKeyValue *kv;
	idLexer lex;
	idVec3 v;
	idCurve_Spline<idVec3> *spline;
	const char *curveTag = "curve_";

	kv = spawnArgs.MatchPrefix( curveTag );
	if ( !kv ) {
		return NULL;
	}

    idStr str = kv->GetKey().Right(kv->GetKey().Length() - static_cast<int>(strlen(curveTag)));
	if ( str.Icmp( "CatmullRomSpline" ) == 0 ) {
		spline = new idCurve_CatmullRomSpline<idVec3>();
	} else if ( str.Icmp( "nubs" ) == 0 ) {
		spline = new idCurve_NonUniformBSpline<idVec3>();
	} else if ( str.Icmp( "nurbs" ) == 0 ) {
		spline = new idCurve_NURBS<idVec3>();
	} else {
		spline = new idCurve_BSpline<idVec3>();
	}

	spline->SetBoundaryType( CSB_CLAMPED );

	lex.LoadMemory( kv->GetValue(), kv->GetValue().Length(), curveTag );
	numPoints = lex.ParseInt();
	lex.ExpectTokenString( "(" );
	for ( t = i = 0; i < numPoints; i++, t += 100 ) {
		v.x = lex.ParseFloat();
		v.y = lex.ParseFloat();
		v.z = lex.ParseFloat();
		spline->AddValue( t, v );
	}
	lex.ExpectTokenString( ")" );

	return spline;
}

/*
===============
idEntity::ShowEditingDialog
===============
*/
void idEntity::ShowEditingDialog( void ) {
}

/***********************************************************************

   Events
	
***********************************************************************/

/*
================
idEntity::Event_GetName
================
*/
void idEntity::Event_GetName( void ) {
	idThread::ReturnString( name.c_str() );
}

/*
================
idEntity::Event_SetName
================
*/
void idEntity::Event_SetName( const char *newname ) {
	SetName( newname );
}

/*
================
idEntity::Event_IsType
================
*/
void idEntity::Event_IsType( const char *pstr_typeName ) 
{
	idTypeInfo* p_namedType = GetClass (pstr_typeName);
	if (p_namedType == NULL)
	{
		idThread::ReturnInt (0);
	}
	else
	{
		if (IsType (*p_namedType))
		{
			idThread::ReturnInt (1);
		}
		else
		{
			idThread::ReturnInt (0);
		}
	}
}


/*
===============
idEntity::Event_FindTargets
===============
*/
void idEntity::Event_FindTargets( void ) {
	FindTargets();
}

/*
============
idEntity::Event_ActivateTargets

Activates any entities targeted by this entity.  Mainly used as an
event to delay activating targets.
============
*/
void idEntity::Event_ActivateTargets( idEntity *activator ) {
	ActivateTargets( activator );
}


void idEntity::Event_AddTarget(idEntity* target)
{
	AddTarget(target);
}


void idEntity::Event_RemoveTarget(idEntity* target)
{
	RemoveTarget(target);
}


/*
================
idEntity::Event_NumTargets
================
*/
void idEntity::Event_NumTargets( void ) {
	idThread::ReturnFloat( targets.Num() );
}

/*
================
idEntity::Event_GetTarget
================
*/
void idEntity::Event_GetTarget( float index ) {
	int i;

	i = ( int )index;
	if ( ( i < 0 ) || i >= targets.Num() ) {
		idThread::ReturnEntity( NULL );
	} else {
		idThread::ReturnEntity( targets[ i ].GetEntity() );
	}
}

/*
================
idEntity::Event_RandomTarget
================
*/
void idEntity::Event_RandomTarget( const char *ignore ) {
	int			num;
	idEntity	*ent;
	int			i;
	int			ignoreNum;

	RemoveNullTargets();
	if ( !targets.Num() ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	ignoreNum = -1;
	if ( ignore && ( ignore[ 0 ] != 0 ) && ( targets.Num() > 1 ) ) {
		for( i = 0; i < targets.Num(); i++ ) {
			ent = targets[ i ].GetEntity();
			if ( ent && ( ent->name == ignore ) ) {
				ignoreNum = i;
				break;
			}
		}
	}

	if ( ignoreNum >= 0 ) {
		num = gameLocal.random.RandomInt( targets.Num() - 1 );
		if ( num >= ignoreNum ) {
			num++;
		}
	} else {
		num = gameLocal.random.RandomInt( targets.Num() );
	}

	ent = targets[ num ].GetEntity();
	idThread::ReturnEntity( ent );
}

/*
================
idEntity::Event_BindToJoint
================
*/
void idEntity::Event_BindToJoint( idEntity *master, const char *jointname, float orientated ) {
	BindToJoint( master, jointname, ( orientated != 0.0f ) );
}

/*
================
idEntity::Event_BindToBody
================
*/
void idEntity::Event_BindToBody(idEntity *master, int bodyId, bool orientated) {
	BindToBody( master, bodyId, orientated );
}


/*
================
idEntity::Event_RemoveBinds
================
*/
void idEntity::Event_RemoveBinds( void ) {
	RemoveBinds( false );
}

/*
================
idEntity::Event_Bind
================
*/
void idEntity::Event_Bind( idEntity *master ) {
	Bind( master, true );
}

/*
================
idEntity::Event_BindPosition
================
*/
void idEntity::Event_BindPosition( idEntity *master ) {
	Bind( master, false );
}

/*
================
idEntity::Event_Unbind
================
*/
void idEntity::Event_Unbind( void ) {
	Unbind();
}

/*
================
idEntity::Event_SpawnBind
================
*/
void idEntity::Event_SpawnBind( void ) 
{
	idEntity		*parent;
	const char		*bind, *joint, *bindanim;
	jointHandle_t	bindJoint;
	bool			bindOrientated;
	int				id;
	const idAnim	*anim;
	int				animNum;
	idAnimator		*parentAnimator;
	
	if ( spawnArgs.GetString( "bind", "", &bind ) ) 
	{
		parent = gameLocal.FindEntity( bind );

		bindOrientated = spawnArgs.GetBool( "bindOrientated", "1" );
		if ( parent ) 
		{
			if (spawnArgs.GetBool("is_attachment"))
			{
				parent->Attach(this);
			}

			// bind to a joint of the skeletal model of the parent
			if ( spawnArgs.GetString( "bindToJoint", "", &joint ) && *joint ) 
			{
				parentAnimator = parent->GetAnimator();
				if ( !parentAnimator )
					gameLocal.Error( "Cannot bind to joint '%s' on '%s'.  Entity does not support skeletal models.", joint, name.c_str() );

				bindJoint = parentAnimator->GetJointHandle( joint );
				if ( bindJoint == INVALID_JOINT )
					gameLocal.Error( "Joint '%s' not found for bind on '%s'", joint, name.c_str() );

				// bind it relative to a specific anim
				if ( ( parent->spawnArgs.GetString( "bindanim", "", &bindanim ) 
						|| parent->spawnArgs.GetString( "anim", "", &bindanim ) ) && *bindanim ) 
				{
					animNum = parentAnimator->GetAnim( bindanim );
					if ( !animNum ) {
						gameLocal.Error( "Anim '%s' not found for bind on '%s'", bindanim, name.c_str() );
					}
					anim = parentAnimator->GetAnim( animNum );
					if ( !anim ) {
						gameLocal.Error( "Anim '%s' not found for bind on '%s'", bindanim, name.c_str() );
					}

					// make sure parent's render origin has been set
					parent->UpdateModelTransform();

					//FIXME: need a BindToJoint that accepts a joint position
					parentAnimator->CreateFrame( gameLocal.time, true );
					idJointMat *frame = parent->renderEntity.joints;
					gameEdit->ANIM_CreateAnimFrame( parentAnimator->ModelHandle(), anim->MD5Anim( 0 ), parent->renderEntity.numJoints, frame, 0, parentAnimator->ModelDef()->GetVisualOffset(), parentAnimator->RemoveOrigin() );
					BindToJoint( parent, joint, bindOrientated );
					parentAnimator->ForceUpdate();
				} 
				else
					BindToJoint( parent, joint, bindOrientated );
			}
			// bind to a body of the physics object of the parent
			else if ( spawnArgs.GetInt( "bindToBody", "0", id ) )
				BindToBody( parent, id, bindOrientated );
			// no joint specified, bind to the parent
			else
				Bind( parent, bindOrientated );
		}
	}
}

/*
================
idEntity::Event_SetOwner
================
*/
void idEntity::Event_SetOwner( idEntity *owner ) {
	int i;

	for ( i = 0; i < GetPhysics()->GetNumClipModels(); i++ ) {
		GetPhysics()->GetClipModel( i )->SetOwner( owner );
	}
}

/*
================
idEntity::Event_SetModel
================
*/
void idEntity::Event_SetModel( const char *modelname ) {
	SetModel( modelname );
}

/*
================
idEntity::Event_SetSkin
================
*/
void idEntity::Event_SetSkin( const char *skinname ) {
	renderEntity.customSkin = declManager->FindSkin( skinname );
	UpdateVisuals();
}

/*
================
idEntity::Event_ReskinCollisionModel
================
*/
void idEntity::Event_ReskinCollisionModel()
{
	ReskinCollisionModel();
}

/*
================
idEntity::Event_GetShaderParm
================
*/
void idEntity::Event_GetShaderParm( int parmnum ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Error( "shader parm index (%d) out of range", parmnum );
	}

	idThread::ReturnFloat( renderEntity.shaderParms[ parmnum ] );
}

/*
================
idEntity::Event_SetShaderParm
================
*/
void idEntity::Event_SetShaderParm( int parmnum, float value ) {
	SetShaderParm( parmnum, value );
}

/*
================
idEntity::Event_SetShaderParms
================
*/
void idEntity::Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= parm0;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= parm1;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= parm2;
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= parm3;
	UpdateVisuals();
}


/*
================
idEntity::Event_SetColor
================
*/
void idEntity::Event_SetColor( float red, float green, float blue ) {
	SetColor( red, green, blue );
}

/*
================
idEntity::Event_SetColorVec
================
*/
void idEntity::Event_SetColorVec( const idVec3 &newColor ) {
	SetColor( newColor );
}

/*
================
idEntity::Event_GetColor
================
*/
void idEntity::Event_GetColor( void ) {
	idVec3 out;

	GetColor( out );
	idThread::ReturnVector( out );
}

/*
================
idEntity::Event_SetHealth
================
*/
void idEntity::Event_SetHealth( float newHealth ) {
	health = static_cast<int>(newHealth);

	if( health <= 0 && !m_bIsBroken )
	{
		BecomeBroken( NULL );
	}
}

/*
================
idEntity::Event_GetHealth
================
*/
void idEntity::Event_GetHealth( void ) {
	idThread::ReturnInt( health );
}

/*
================
idEntity::Event_SetMaxHealth
================
*/
void idEntity::Event_SetMaxHealth( float newMaxHealth ) {
	maxHealth = static_cast<int>(newMaxHealth);

	if( health > maxHealth )
	{
		health = maxHealth;
	}

	if( health <= 0 && !m_bIsBroken )
	{
		BecomeBroken( NULL );
	}
}

/*
================
idEntity::Event_GetMaxHealth
================
*/
void idEntity::Event_GetMaxHealth( void ) {
	idThread::ReturnInt( maxHealth );
}

/*
================
idEntity::Event_IsHidden
================
*/
void idEntity::Event_IsHidden( void ) {
	idThread::ReturnInt( fl.hidden );
}

/*
================
idEntity::Event_Hide
================
*/
void idEntity::Event_Hide( void ) {
	Hide();
}

/*
================
idEntity::Event_Show
================
*/
void idEntity::Event_Show( void ) {
	Show();
}

/*
================
idEntity::Event_SetFrobActionScript
================
*/
void idEntity::Event_SetFrobActionScript( const char *frobActionScript ) {
	idStr str = frobActionScript;
	m_FrobActionScript = str;
	spawnArgs.Set("frob_action_script", str);
}

/*
================
idEntity::Event_SetUsedBy
================
*/
void idEntity::Event_SetUsedBy( idEntity *useEnt, bool canUse ) {

	if( canUse )
		m_UsedByName.AddUnique( useEnt->name );

	else
		m_UsedByName.Remove( useEnt->name );
}

/*
================
idEntity::Event_CacheSoundShader
================
*/
void idEntity::Event_CacheSoundShader( const char *soundName ) {
	declManager->FindSound( soundName );
}

/*
================
idEntity::Event_StartSoundShader
================
*/
void idEntity::Event_StartSoundShader( const char *soundName, int channel ) {
	int length;

	StartSoundShader( declManager->FindSound( soundName ), (s_channelType)channel, 0, false, &length );
	idThread::ReturnFloat( MS2SEC( length ) );
}

/*
================
idEntity::Event_StopSound
================
*/
void idEntity::Event_StopSound( int channel, int netSync ) {
	StopSound( channel, ( netSync != 0 ) );
}

/*
================
idEntity::Event_StartSound 
================
*/
void idEntity::Event_StartSound( const char *soundName, int channel, int netSync ) {
	int time;
	
	StartSound( soundName, ( s_channelType )channel, 0, ( netSync != 0 ), &time );
	idThread::ReturnFloat( MS2SEC( time ) );
}

/*
================
idEntity::Event_FadeSound
================
*/
void idEntity::Event_FadeSound( int channel, float to, float over ) {
	if ( refSound.referenceSound ) {
		refSound.referenceSound->FadeSound( channel, to, over );
	}
}

/*
================
idEntity::Event_SetSoundVolume

tels:
================
*/
void idEntity::Event_SetSoundVolume( float volume ) {
	SetSoundVolume( volume );
}

/*
================
idEntity::Event_GetSoundVolume

grayman #3395
================
*/
void idEntity::Event_GetSoundVolume( const char* soundName )
{
	float volume;
	const idSoundShader *sndShader = declManager->FindSound(soundName);

	if ( sndShader->GetState() == DS_DEFAULTED )
	{
		gameLocal.Warning( "Sound '%s' not found", soundName );
		volume = 0;
	}
	else
	{
		volume = sndShader->GetParms()->volume;
	}

	idThread::ReturnFloat(volume);
}

/*
================
idEntity::Event_GetWorldOrigin
================
*/
void idEntity::Event_GetWorldOrigin( void ) {
	idThread::ReturnVector( GetPhysics()->GetOrigin() );
}

/*
================
idEntity::Event_SetWorldOrigin
================
*/
void idEntity::Event_SetWorldOrigin( idVec3 const &org ) {
	idVec3 neworg = GetLocalCoordinates( org );
	SetOrigin( neworg );
}

/*
================
idEntity::Event_SetOrigin
================
*/
void idEntity::Event_SetOrigin( idVec3 const &org ) {
	SetOrigin( org );
}

/*
================
idEntity::Event_GetOrigin
================
*/
void idEntity::Event_GetOrigin( void ) {
	idThread::ReturnVector( GetLocalCoordinates( GetPhysics()->GetOrigin() ) );
}

/*
================
idEntity::Event_SetAngles
================
*/
void idEntity::Event_SetAngles( idAngles const &ang ) {
	SetAngles( ang );
}

/*
================
idEntity::Event_GetAngles
================
*/
void idEntity::Event_GetAngles( void ) {
	idAngles ang = GetPhysics()->GetAxis().ToAngles();
	idThread::ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
}

/*
================
idEntity::Event_SetLinearVelocity
================
*/
void idEntity::Event_SetLinearVelocity( const idVec3 &velocity ) {
	GetPhysics()->SetLinearVelocity( velocity );
}

/*
================
idEntity::Event_GetLinearVelocity
================
*/
void idEntity::Event_GetLinearVelocity( void ) {
	idThread::ReturnVector( GetPhysics()->GetLinearVelocity() );
}

/*
================
idEntity::Event_SetAngularVelocity
================
*/
void idEntity::Event_SetAngularVelocity( const idVec3 &velocity ) {
	GetPhysics()->SetAngularVelocity( velocity );
}

/*
================
idEntity::Event_GetAngularVelocity
================
*/
void idEntity::Event_GetAngularVelocity( void ) {
	idThread::ReturnVector( GetPhysics()->GetAngularVelocity() );
}

/*
================
idEntity::Event_SetGravity
================
*/
void idEntity::Event_SetGravity( const idVec3 &newGravity ) {
	GetPhysics()->SetGravity( newGravity );
}

/*
================
idEntity::Event_ApplyImpulse

Tels #2897
================
*/
void idEntity::Event_ApplyImpulse( idEntity *ent, const int id, const idVec3 &point, const idVec3 &impulse ) {
		ApplyImpulse( ent, id, point, impulse );
}

/*
================
idEntity::Event_SetSize
================
*/
void idEntity::Event_SetSize( idVec3 const &mins, idVec3 const &maxs ) {
	GetPhysics()->SetClipBox( idBounds( mins, maxs ), 1.0f );
}

/*
================
idEntity::Event_GetSize
================
*/
void idEntity::Event_GetSize( void ) {
	idBounds bounds;

	bounds = GetPhysics()->GetBounds();
	idThread::ReturnVector( bounds[1] - bounds[0] );
}

/*
================
idEntity::Event_GetMins
================
*/
void idEntity::Event_GetMins( void ) {
	idThread::ReturnVector( GetPhysics()->GetBounds()[0] );
}

/*
================
idEntity::Event_GetMaxs
================
*/
void idEntity::Event_GetMaxs( void ) {
	idThread::ReturnVector( GetPhysics()->GetBounds()[1] );
}

/*
================
idEntity::Event_Touches
================
*/
void idEntity::Event_Touches( idEntity *ent ) {
	if ( !ent ) {
		idThread::ReturnInt( false );
		return;
	}

	const idBounds &myBounds = GetPhysics()->GetAbsBounds();
	const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();

	idThread::ReturnInt( myBounds.IntersectsBounds( entBounds ) );
}

/*
================
idEntity::Event_GetNextKey
================
*/
void idEntity::Event_GetNextKey( const char *prefix, const char *lastMatch ) {
	const idKeyValue *kv;
	const idKeyValue *previous;

	if ( *lastMatch ) {
		previous = spawnArgs.FindKey( lastMatch );
	} else {
		previous = NULL;
	}

	kv = spawnArgs.MatchPrefix( prefix, previous );
	if ( !kv ) {
		idThread::ReturnString( "" );
	} else {
		idThread::ReturnString( kv->GetKey() );
	}
}

/*
================
idEntity::Event_SetKey
================
*/
void idEntity::Event_SetKey( const char *key, const char *value ) {
	spawnArgs.Set( key, value );
}

/*
================
idEntity::Event_GetKey
================
*/
void idEntity::Event_GetKey( const char *key ) const {
	const char *value;

	spawnArgs.GetString( key, "", &value );
	idThread::ReturnString( value );
}

/*
================
idEntity::Event_GetIntKey
================
*/
void idEntity::Event_GetIntKey( const char *key ) const {
	int value;

	spawnArgs.GetInt( key, "0", value );

	// TODO: scripts only support floats
	idThread::ReturnFloat( value );
}

/*
================
idEntity::Event_GetBoolKey
================
*/
void idEntity::Event_GetBoolKey( const char *key ) const {
	bool value;

	spawnArgs.GetBool( key, "0", value );

	// TODO: scripts only support floats
	idThread::ReturnFloat( value );
}

/*
================
idEntity::Event_GetFloatKey
================
*/
void idEntity::Event_GetFloatKey( const char *key ) const {
	float value;

	spawnArgs.GetFloat( key, "0", value );
	idThread::ReturnFloat( value );
}

/*
================
idEntity::Event_GetVectorKey
================
*/
void idEntity::Event_GetVectorKey( const char *key ) const {
	idVec3 value;

	spawnArgs.GetVector( key, "0 0 0", value );
	idThread::ReturnVector( value );
}

/*
================
idEntity::Event_GetEntityKey
================
*/
void idEntity::Event_GetEntityKey( const char *key ) const {
	idEntity *ent;
	const char *entname;

	if ( !spawnArgs.GetString( key, NULL, &entname ) ) {
		idThread::ReturnEntity( NULL );
		return;
	}

	ent = gameLocal.FindEntity( entname );
	if ( !ent ) {
		gameLocal.Warning( "Couldn't find entity '%s' specified in '%s' key in entity '%s'", entname, key, name.c_str() );
	}

	idThread::ReturnEntity( ent );
}

/*
================
idEntity::Event_RemoveKey
================
*/
void idEntity::Event_RemoveKey( const char *key ) {
	spawnArgs.Delete( key );
}

/*
================
idEntity::Event_RestorePosition
================
*/
void idEntity::Event_RestorePosition( void ) {
	idVec3		org;
	idMat3		axis;
	idEntity *	part;

	spawnArgs.GetVector( "origin", "0 0 0", org );
	gameEdit->ParseSpawnArgsToAxis( &spawnArgs, axis );

	GetPhysics()->SetOrigin( org );
	GetPhysics()->SetAxis( axis );

	UpdateVisuals();

	for ( part = teamChain; part != NULL; part = part->teamChain ) {
		if ( part->bindMaster != this ) {
			continue;
		}
		if ( part->GetPhysics()->IsType( idPhysics_Parametric::Type ) ) {
			if ( static_cast<idPhysics_Parametric *>(part->GetPhysics())->IsPusher() ) {
				gameLocal.Warning( "teleported '%s' which has the pushing mover '%s' bound to it", GetName(), part->GetName() );
			}
		} else if ( part->GetPhysics()->IsType( idPhysics_AF::Type ) ) {
			gameLocal.Warning( "teleported '%s' which has the articulated figure '%s' bound to it", GetName(), part->GetName() );
		}
	}
}

/*
================
idEntity::Event_UpdateCameraTarget
================
*/
void idEntity::Event_UpdateCameraTarget( void ) {
	const char *target;
	const idKeyValue *kv;
	idVec3 dir;

	target = spawnArgs.GetString( "cameraTarget" );

	cameraTarget = gameLocal.FindEntity( target );

	if ( cameraTarget ) {
		kv = cameraTarget->spawnArgs.MatchPrefix( "target", NULL );
		while( kv ) {
			idEntity *ent = gameLocal.FindEntity( kv->GetValue() );
			if ( ent && idStr::Icmp( ent->GetEntityDefName(), "target_null" ) == 0) {
				dir = ent->GetPhysics()->GetOrigin() - cameraTarget->GetPhysics()->GetOrigin();
				dir.Normalize();
				cameraTarget->SetAxis( dir.ToMat3() );
				SetAxis(dir.ToMat3());
				break;						
			}
			kv = cameraTarget->spawnArgs.MatchPrefix( "target", kv );
		}
	}
	UpdateVisuals();
}

/*
================
idEntity::Event_DistanceTo
================
*/
void idEntity::Event_DistanceTo( idEntity *ent ) {
	if ( !ent ) {
		// just say it's really far away
		idThread::ReturnFloat( MAX_WORLD_SIZE );
	} else {
		float dist = ( GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin() ).LengthFast();
		idThread::ReturnFloat( dist );
	}
}

/*
================
idEntity::Event_DistanceToPoint
================
*/
void idEntity::Event_DistanceToPoint( const idVec3 &point ) {
	float dist = ( GetPhysics()->GetOrigin() - point ).LengthFast();
	idThread::ReturnFloat( dist );
}

/*
================
idEntity::Event_StartFx
================
*/
void idEntity::Event_StartFx( const char *fx ) {
	idEntityFx::StartFx( fx, NULL, NULL, this, true );
}

/*
================
idEntity::Event_WaitFrame
================
*/
void idEntity::Event_WaitFrame( void ) {
	idThread *thread;
	
	thread = idThread::CurrentThread();
	if ( thread ) {
		thread->WaitFrame();
	}
}

/*
=====================
idEntity::Event_Wait
=====================
*/
void idEntity::Event_Wait( float time ) {
	idThread *thread = idThread::CurrentThread();

	if ( !thread ) {
		gameLocal.Error( "Event 'wait' called from outside thread" );
	}

	thread->WaitSec( time );
}

/*
=====================
idEntity::Event_HasFunction
=====================
*/
void idEntity::Event_HasFunction( const char *name ) {
	const function_t *func;

	func = scriptObject.GetFunction( name );
	if ( func ) {
		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
=====================
idEntity::Event_CallFunction
=====================
*/
void idEntity::Event_CallFunction( const char *funcname ) {
	const function_t *func;
	idThread *thread;

	thread = idThread::CurrentThread();
	if ( !thread ) {
		gameLocal.Error( "Event 'callFunction' called from outside thread" );
	}

	func = scriptObject.GetFunction( funcname );
	if ( !func ) {
		gameLocal.Error( "Unknown function '%s' in '%s'", funcname, scriptObject.GetTypeName() );
	}

	if ( func->type->NumParameters() != 1 ) {
		gameLocal.Error( "Function '%s' has the wrong number of parameters for 'callFunction'", funcname );
	}
	if ( !scriptObject.GetTypeDef()->Inherits( func->type->GetParmType( 0 ) ) ) {
		gameLocal.Error( "Function '%s' is the wrong type for 'callFunction'", funcname );
	}

	// function args will be invalid after this call
	thread->CallFunction( this, func, false );
}

/*
=====================
idEntity::Event_CallGlobalFunction
=====================
*/
void idEntity::Event_CallGlobalFunction( const char *funcname, idEntity *ent ) {
	const function_t *func;
	idThread *thread;

	thread = idThread::CurrentThread();
	if ( !thread ) {
		gameLocal.Error( "Event 'callGlobalFunction' called from outside thread" );
	}

    func = gameLocal.program.FindFunction( funcname );
	if ( !func ) {
		gameLocal.Error( "Unknown global function '%s'", funcname );
	}

	if ( func->type->NumParameters() != 1 ) {
		gameLocal.Error( "Function '%s' has the wrong number of parameters for 'callGlobalFunction'", funcname );
	}
    /*
	if ( !scriptObject.GetTypeDef()->Inherits( func->type->GetParmType( 0 ) ) ) {
		gameLocal.Error( "Function '%s' is the wrong type for 'callGlobalFunction'", funcname );
	} */

	// function args will be invalid after this call
	thread->CallFunction( ent, func, false );
}

/*
================
idEntity::Event_SetNeverDormant
================
*/
void idEntity::Event_SetNeverDormant( int enable ) {
	fl.neverDormant	= ( enable != 0 );
	dormantStart = 0;
}

#ifdef MOD_WATERPHYSICS

/*

================

idEntity::Event_GetMass		MOD_WATERPHYSICS

================

*/

void idEntity::Event_GetMass( int id ) {

	idThread::ReturnFloat(physics->GetMass(id));

}



/*

================

idEntity::Event_IsInLiquid	MOD_WATERPHYSICS

================

*/

void idEntity::Event_IsInLiquid( void ) {

	idThread::ReturnInt(physics->GetWater() != NULL);

}

#endif		// MOD_WATERPHYSICS


/*
================
idEntity::Event_CopyBind
================
*/

void idEntity::Event_CopyBind( idEntity* other ) 
{
	if (other == NULL) return;

	idEntity *master = other->GetBindMaster();

	jointHandle_t joint = other->GetBindJoint();

	int body = other->GetBindBody();
	if( joint != INVALID_JOINT )
	{
		// joint is specified so bind to that joint
		BindToJoint( master, joint, true );
	}
	else if( body >= 0 )
	{ 
		// body is specified so bind to it
		BindToBody( master, body, true );
	}
	else
	{
		// no joint and no body specified, so bind to master
		Bind( master, true );

		// greebo: If the bind master is static, set the solid for team flag
		if (master && master->GetPhysics()->IsType(idPhysics_Static::Type))
		{
			fl.solidForTeam = true;
		}
	}
}

/***********************************************************************

   Network
	
***********************************************************************/

/*
================
idEntity::ClientPredictionThink
================
*/
void idEntity::ClientPredictionThink( void ) {
	RunPhysics();
	Present();
}

/*
================
idEntity::WriteBindToSnapshot
================
*/
void idEntity::WriteBindToSnapshot( idBitMsgDelta &msg ) const {
	int bindInfo;

	if ( bindMaster ) {
		bindInfo = bindMaster->entityNumber;
		bindInfo |= ( fl.bindOrientated & 1 ) << GENTITYNUM_BITS;
		if ( bindJoint != INVALID_JOINT ) {
			bindInfo |= 1 << ( GENTITYNUM_BITS + 1 );
			bindInfo |= bindJoint << ( 3 + GENTITYNUM_BITS );
		} else if ( bindBody != -1 ) {
			bindInfo |= 2 << ( GENTITYNUM_BITS + 1 );
			bindInfo |= bindBody << ( 3 + GENTITYNUM_BITS );
		}
	} else {
		bindInfo = ENTITYNUM_NONE;
	}
	msg.WriteBits( bindInfo, GENTITYNUM_BITS + 3 + 9 );
}

/*
================
idEntity::ReadBindFromSnapshot
================
*/
void idEntity::ReadBindFromSnapshot( const idBitMsgDelta &msg ) {
	int bindInfo, bindEntityNum, bindPos;
	bool bindOrientated;
	idEntity *master;

	bindInfo = msg.ReadBits( GENTITYNUM_BITS + 3 + 9 );
	bindEntityNum = bindInfo & ( ( 1 << GENTITYNUM_BITS ) - 1 );

	if ( bindEntityNum != ENTITYNUM_NONE ) {
		master = gameLocal.entities[ bindEntityNum ];

		bindOrientated = ( bindInfo >> GENTITYNUM_BITS ) & 1;
		bindPos = ( bindInfo >> ( GENTITYNUM_BITS + 3 ) );
		switch( ( bindInfo >> ( GENTITYNUM_BITS + 1 ) ) & 3 ) {
			case 1: {
				BindToJoint( master, (jointHandle_t) bindPos, bindOrientated );
				break;
			}
			case 2: {
				BindToBody( master, bindPos, bindOrientated );
				break;
			}
			default: {
				Bind( master, bindOrientated );
				break;
			}
		}
	} else if ( bindMaster ) {
		Unbind();
	}
}

/*
================
idEntity::WriteColorToSnapshot
================
*/
void idEntity::WriteColorToSnapshot( idBitMsgDelta &msg ) const {
	idVec4 color;

	color[0] = renderEntity.shaderParms[ SHADERPARM_RED ];
	color[1] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	color[2] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	color[3] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];
	msg.WriteLong( PackColor( color ) );
}

/*
================
idEntity::ReadColorFromSnapshot
================
*/
void idEntity::ReadColorFromSnapshot( const idBitMsgDelta &msg ) {
	idVec4 color;

	UnpackColor( msg.ReadLong(), color );
	renderEntity.shaderParms[ SHADERPARM_RED ] = color[0];
	renderEntity.shaderParms[ SHADERPARM_GREEN ] = color[1];
	renderEntity.shaderParms[ SHADERPARM_BLUE ] = color[2];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ] = color[3];
}

/*
================
idEntity::WriteGUIToSnapshot
================
*/
void idEntity::WriteGUIToSnapshot( idBitMsgDelta &msg ) const {
	// no need to loop over MAX_RENDERENTITY_GUI at this time
	if ( renderEntity.gui[ 0 ] ) {
		msg.WriteByte( renderEntity.gui[ 0 ]->State().GetInt( "networkState" ) );
	} else {
		msg.WriteByte( 0 );
	}
}

/*
================
idEntity::ReadGUIFromSnapshot
================
*/
void idEntity::ReadGUIFromSnapshot( const idBitMsgDelta &msg ) {
	int state;
	idUserInterface *gui;
	state = msg.ReadByte( );
	gui = renderEntity.gui[ 0 ];
	if ( gui && state != mpGUIState ) {
		mpGUIState = state;
		gui->SetStateInt( "networkState", state );
		gui->HandleNamedEvent( "networkState" );
	}
}

/*
================
idEntity::WriteToSnapshot
================
*/
void idEntity::WriteToSnapshot( idBitMsgDelta &msg ) const {
}

/*
================
idEntity::ReadFromSnapshot
================
*/
void idEntity::ReadFromSnapshot( const idBitMsgDelta &msg ) {
}

/*
================
idEntity::ServerSendEvent

   Saved events are also sent to any client that connects late so all clients
   always receive the events nomatter what time they join the game.
================
*/
void idEntity::ServerSendEvent( int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient ) const {
}

/*
================
idEntity::ClientSendEvent
================
*/
void idEntity::ClientSendEvent( int eventId, const idBitMsg *msg ) const {
}

/*
================
idEntity::ServerReceiveEvent
================
*/
bool idEntity::ServerReceiveEvent( int event, int time, const idBitMsg &msg ) {
	switch( event ) {
		case 0: {
		}
		default: {
			return false;
		}
	}
}

/*
================
idEntity::ClientReceiveEvent
================
*/
bool idEntity::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	return false;
}

/*
===============================================================================

  idAnimatedEntity
	
===============================================================================
*/

const idEventDef EV_GetJointHandle( "getJointHandle", EventArgs('s', "jointname", ""), 'd', 
	"Looks up the number of the specified joint. Returns INVALID_JOINT if the joint is not found." );
const idEventDef EV_ClearAllJoints( "clearAllJoints", EventArgs(), EV_RETURNS_VOID, 
	"Removes any custom transforms on all joints." );
const idEventDef EV_ClearJoint( "clearJoint", EventArgs('d', "jointnum", ""), EV_RETURNS_VOID, 
	"Removes any custom transforms on the specified joint." );
const idEventDef EV_SetJointPos( "setJointPos", 
	EventArgs('d', "jointnum", "", 
			  'd', "transform_type", "",
			  'v', "pos", ""), 
	EV_RETURNS_VOID,
	"Modifies the position of the joint based on the transform type.");
const idEventDef EV_SetJointAngle( "setJointAngle", 
	EventArgs('d', "jointnum", "", 
			  'd', "transform_type", "",
			  'v', "angles", ""), 
	EV_RETURNS_VOID,
	"Modifies the orientation of the joint based on the transform type.");
const idEventDef EV_GetJointPos( "getJointPos", EventArgs('d', "jointnum", ""), 'v', "Returns the position of the joint in world space." );
const idEventDef EV_GetJointAngle( "getJointAngle", EventArgs('d', "jointnum", ""), 'v', "Returns the angular orientation of the joint in world space." );

CLASS_DECLARATION( idEntity, idAnimatedEntity )
	EVENT( EV_GetJointHandle,		idAnimatedEntity::Event_GetJointHandle )
	EVENT( EV_ClearAllJoints,		idAnimatedEntity::Event_ClearAllJoints )
	EVENT( EV_ClearJoint,			idAnimatedEntity::Event_ClearJoint )
	EVENT( EV_SetJointPos,			idAnimatedEntity::Event_SetJointPos )
	EVENT( EV_SetJointAngle,		idAnimatedEntity::Event_SetJointAngle )
	EVENT( EV_GetJointPos,			idAnimatedEntity::Event_GetJointPos )
	EVENT( EV_GetJointAngle,		idAnimatedEntity::Event_GetJointAngle )
END_CLASS

/*
================
CAttachInfo::Save
================
*/
void CAttachInfo::Save( idSaveGame *savefile ) const
{
	ent.Save( savefile );
	savefile->WriteInt( channel );
	savefile->WriteString( name );
	savefile->WriteInt(savedContents);
	savefile->WriteString(posName); // grayman #2603
}

/*
================
CAttachInfo::Restore
================
*/
void CAttachInfo::Restore( idRestoreGame *savefile )
{
	ent.Restore( savefile );
	savefile->ReadInt( channel );
	savefile->ReadString( name );
	savefile->ReadInt(savedContents);
	savefile->ReadString(posName); // grayman #2603
}

/*
================
idAnimatedEntity::idAnimatedEntity
================
*/
idAnimatedEntity::idAnimatedEntity() :
	lastUpdateTime(-1)
{
	animator.SetEntity( this );
	damageEffects = NULL;
}

/*
================
idAnimatedEntity::~idAnimatedEntity
================
*/
idAnimatedEntity::~idAnimatedEntity() {
	damageEffect_t	*de;

	for ( de = damageEffects; de; de = damageEffects ) {
		damageEffects = de->next;
		delete de;
	}
}

/*
===============
idAnimatedEntity::Spawn
===============
*/
void idAnimatedEntity::Spawn( void )
{
	// Cache animation rates
	CacheAnimRates();
	int anims = animator.NumAnims();
	m_animRates.Clear();
	m_animRates.AssureSize(anims);
	for (int i=0; i<anims; i++) 
	{
		const idAnim *anim = animator.GetAnim(i);
		if (anim != NULL) 
		{
			idStr spawnargname = "anim_rate_";
			spawnargname += anim->Name();
			m_animRates[i] = spawnArgs.GetFloat(spawnargname, "1");
		} else 
		{
			m_animRates[i] = 1.0f;
		}
	}

	lastUpdateTime = 0;
}

/*
================
idAnimatedEntity::Save

archives object for save game file
================
*/
void idAnimatedEntity::Save( idSaveGame *savefile ) const 
{
	animator.Save( savefile );
	savefile->WriteInt(lastUpdateTime);

	// Wounds are very temporary, ignored at this time
	//damageEffect_t			*damageEffects;
}

/*
================
idAnimatedEntity::Restore

unarchives object from save game file
================
*/
void idAnimatedEntity::Restore( idRestoreGame *savefile ) 
{
	animator.Restore( savefile );
	savefile->ReadInt(lastUpdateTime);

	// check if the entity has an MD5 model
	if ( animator.ModelHandle() )
	{
		// set the callback to update the joints
		renderEntity.callback = idEntity::ModelCallback;

		animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
		animator.GetBounds( gameLocal.time, renderEntity.bounds );
		if ( modelDefHandle != -1 ) {
			gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
		}
	}
}

/*
================
idAnimatedEntity::ClientPredictionThink
================
*/
void idAnimatedEntity::ClientPredictionThink( void ) {
	RunPhysics();
	UpdateAnimation();
	Present();
}

/*
================
idAnimatedEntity::Think
================
*/
void idAnimatedEntity::Think( void ) {
	RunPhysics();
	
	UpdateAnimation();

	Present();
	if ( needsDecalRestore ) // #3817
	{
		ReapplyDecals();
		needsDecalRestore = false;
	}
	UpdateDamageEffects();
}

/*
================
idAnimatedEntity::UpdateAnimation
================
*/
void idAnimatedEntity::UpdateAnimation( void ) {
	// don't do animations if they're not enabled
	if ( !( thinkFlags & TH_ANIMATE ) ) {
		return;
	}

	// is the model an MD5?
	if ( !animator.ModelHandle() ) {
		// no, so nothing to do
		return;
	}

	// call any frame commands that have happened since the last update
	if ( !fl.hidden )
	{
		animator.ServiceAnims( lastUpdateTime, gameLocal.time );
		lastUpdateTime = gameLocal.time;
	}

	// if the model is animating then we have to update it
	if ( !animator.FrameHasChanged( gameLocal.time ) ) {
		// still fine the way it was
		return;
	}

	// get the latest frame bounds
	animator.GetBounds( gameLocal.time, renderEntity.bounds );
	if ( renderEntity.bounds.IsCleared() && !fl.hidden ) {
		gameLocal.DPrintf( "%d: inside out bounds\n", gameLocal.time );
	}

	// update the renderEntity
	UpdateVisuals();

	// the animation is updated
	animator.ClearForceUpdate();
}

/*
================
idAnimatedEntity::GetAnimator
================
*/
idAnimator *idAnimatedEntity::GetAnimator( void ) {
	return &animator;
}

/*
================
idAnimatedEntity::SetModel
================
*/
void idAnimatedEntity::SetModel( const char *modelname ) {
	FreeModelDef();

	renderEntity.hModel = animator.SetModel( modelname );
	if ( !renderEntity.hModel ) {
		idEntity::SetModel( modelname );
		return;
	}

	if ( !renderEntity.customSkin ) {
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}

	// set the callback to update the joints
	renderEntity.callback = idEntity::ModelCallback;

	animator.GetJoints( &renderEntity.numJoints, &renderEntity.joints );
	animator.GetBounds( gameLocal.time, renderEntity.bounds );

	UpdateVisuals();
	RestoreDecals(); // #3817
}

/*
================
idAnimatedEntity::SwapLODModel

For LOD. Save current animation state, then swap out the model, maintaining joint data if possible. If 
the new model has animations with the same names as ongoing ones, then start them up at the same-numbered 
frame, but without blending. If the new animation is identical, no blending is needed of course. -- SteveL #3770
================
*/
void idAnimatedEntity::SwapLODModel( const char *modelname )
{
	// Copy anim data from current anim on each channel
	idAnimBlend	oldAnims[ ANIM_NumAnimChannels ]; // auto-initializes its members

	if ( animator.ModelDef() ) // if current model is animated
	{
		for ( int i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; ++i )
		{
			oldAnims[i] = *( animator.CurrentAnim( i ) );
		}
	}

	// Set new model, which will clear all animations
	FreeModelDef();
	needsDecalRestore = true;		// #3817
	renderEntity.hModel = animator.SwapLODModel( modelname );
	if ( !renderEntity.hModel )
	{
		// Can't do a swap that maintains joint data. So fall back to a full SetModel and don't attempt to restore decals.
		idAnimatedEntity::SetModel( modelname );
		needsDecalRestore = false;
		// NB this allows a non-animated replacement model, so check before going on to restore anims
		if ( !animator.ModelDef() )
		{
			return;
		}
	}

	if ( !renderEntity.customSkin ) 
	{
		renderEntity.customSkin = animator.ModelDef()->GetDefaultSkin();
	}

	// Reinstate current anims where we have replacement anims available.
	for ( int i = ANIMCHANNEL_ALL; i < ANIM_NumAnimChannels; ++i )
	{
		const idAnim *oldAnim = oldAnims[i].Anim();
		if ( oldAnim )
		{
			int newAnimNum = animator.ModelDef()->GetAnim( oldAnim->FullName() );
			if ( !newAnimNum )
			{
				newAnimNum = animator.ModelDef()->GetAnim( oldAnim->Name() );
			}
			if ( newAnimNum ) 
			{
				int cycle = oldAnims[i].GetCycleCount();
				int starttime = oldAnims[i].GetStartTime();
				animator.PlayAnim( i, newAnimNum, gameLocal.time, 0 );
				animator.CurrentAnim( i )->SetCycleCount( cycle );
				animator.CurrentAnim( i )->SetStartTime( starttime );
				animator.CurrentAnim( i )->CopyStateData( oldAnims[i] ); // #3800 #3834
				BecomeActive( TH_ANIMATE ); // This gets disabled by the model swap
			}
		}
	}

	UpdateVisuals();
	Present();
	if ( needsDecalRestore )		// #3817
	{
		ReapplyDecals();
		needsDecalRestore = false;
	}
}

/*
=====================
idAnimatedEntity::ReapplyDecals

Reapply overlays after LOD switch, hiding, shouldering, save game loading. #3817
=====================
*/
void idAnimatedEntity::ReapplyDecals()
{
	if ( modelDefHandle == -1 )
	{
		return;	// Can happen if model gets hidden again immediately after being shown. CGrabber::FitsInWorld does this.
	}

	gameRenderWorld->RemoveDecals( modelDefHandle );

	for ( std::list<SDecalInfo>::const_iterator di = decals_list.begin(); di != decals_list.end(); ++di )
	{
		const jointHandle_t jnt = di->overlay_joint;
		if ( jnt != INVALID_JOINT )
		{
			// Calculate world coords for the overlay based on current joint position
			const idVec3 joint_origin = renderEntity.origin + renderEntity.joints[jnt].ToVec3() * renderEntity.axis;
			const idMat3 joint_axis = renderEntity.joints[jnt].ToMat3() * renderEntity.axis;
			const idVec3 splat_origin = joint_origin + di->origin * joint_axis;
			const idVec3 splat_direction = di->dir * joint_axis;
			// Call the idEntity function, bypassing overrides, because the overrides propagate the overlay to bindsiblings
			idEntity::ProjectOverlay( splat_origin, splat_direction, di->size, di->decal, false );
		}
	}
}



/*
=====================
idAnimatedEntity::GetJointWorldTransform
=====================
*/
bool idAnimatedEntity::GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis ) {
	if ( !animator.GetJointTransform( jointHandle, currentTime, offset, axis ) ) {
		return false;
	}

	ConvertLocalToWorldTransform( offset, axis );
	return true;
}

/*
==============
idAnimatedEntity::GetJointTransformForAnim
==============
*/
bool idAnimatedEntity::GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int frameTime, idVec3 &offset, idMat3 &axis ) const {
	const idAnim	*anim;
	int				numJoints;
	idJointMat		*frame;

	anim = animator.GetAnim( animNum );
	if ( !anim ) {
		assert( 0 );
		return false;
	}

	numJoints = animator.NumJoints();
	if ( ( jointHandle < 0 ) || ( jointHandle >= numJoints ) ) {
		assert( 0 );
		return false;
	}

	frame = ( idJointMat * )_alloca16( numJoints * sizeof( idJointMat ) );
	gameEdit->ANIM_CreateAnimFrame( animator.ModelHandle(), anim->MD5Anim( 0 ), renderEntity.numJoints, frame, frameTime, animator.ModelDef()->GetVisualOffset(), animator.RemoveOrigin() );

	offset = frame[ jointHandle ].ToVec3();
	axis = frame[ jointHandle ].ToMat3();
	
	return true;
}

/*
==============
idAnimatedEntity::AddDamageEffect

  Dammage effects track the animating impact position, spitting out particles.
==============
*/
void idAnimatedEntity::AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName ) {
	jointHandle_t jointNum;
	idVec3 origin, dir, localDir, localOrigin, localNormal;
	idMat3 axis;

	if ( !g_bloodEffects.GetBool() || renderEntity.joints == NULL ) {
		return;
	}

	const idDeclEntityDef *def = gameLocal.FindEntityDef( damageDefName, false );
	if ( def == NULL ) {
		return;
	}

	jointNum = CLIPMODEL_ID_TO_JOINT_HANDLE( collision.c.id );
	if ( jointNum == INVALID_JOINT ) {
		return;
	}

	dir = velocity;
	dir.Normalize();

	axis = renderEntity.joints[jointNum].ToMat3() * renderEntity.axis;
	origin = renderEntity.origin + renderEntity.joints[jointNum].ToVec3() * renderEntity.axis;

	localOrigin = ( collision.c.point - origin ) * axis.Transpose();
	localNormal = collision.c.normal * axis.Transpose();
	localDir = dir * axis.Transpose();

	AddLocalDamageEffect( jointNum, localOrigin, localNormal, localDir, def, collision.c.material );
}

/*
==============
idAnimatedEntity::GetDefaultSurfaceType
==============
*/
int	idAnimatedEntity::GetDefaultSurfaceType( void ) const {
	return SURFTYPE_METAL;
}

/*
==============
idAnimatedEntity::AddLocalDamageEffect
==============
*/
void idAnimatedEntity::AddLocalDamageEffect
	( 
		jointHandle_t jointNum, const idVec3 &localOrigin, 
		const idVec3 &localNormal, const idVec3 &localDir, 
		const idDeclEntityDef *def, const idMaterial *collisionMaterial 
	) 
{
	const char *splat, *decal, *bleed, *key;
	damageEffect_t	*de;
	idVec3 origin, dir;
	idMat3 axis;
	idStr surfName;

	axis = renderEntity.joints[jointNum].ToMat3() * renderEntity.axis;
	origin = renderEntity.origin + renderEntity.joints[jointNum].ToVec3() * renderEntity.axis;

	origin = origin + localOrigin * axis;
	dir = localDir * axis;

	if ( !collisionMaterial || collisionMaterial->GetSurfaceType() == SURFTYPE_NONE )
	{
		surfName = gameLocal.surfaceTypeNames[ GetDefaultSurfaceType() ];
	}
	else
	{
		g_Global.GetSurfName( collisionMaterial, surfName );
	}

	// start impact sound based on material type
	key = va( "snd_%s", surfName.c_str() );
	// ishtvan: Shouldn't need to play the sound here anymore, right?
/*
	sound = spawnArgs.GetString( key );
	if ( *sound == '\0' ) {
		sound = def->dict.GetString( key );
	}
	if ( *sound != '\0' ) {
		StartSoundShader( declManager->FindSound( sound ), SND_CHANNEL_BODY, 0, false, NULL );
	}
*/

	// blood splats are thrown onto nearby surfaces
	key = va( "mtr_splat_%s", surfName.c_str() );
	splat = spawnArgs.RandomPrefix( key, gameLocal.random );
	if ( *splat == '\0' ) {
		splat = def->dict.RandomPrefix( key, gameLocal.random );
	}
	if ( *splat != '\0' ) {
		gameLocal.BloodSplat( origin, dir, 64.0f, splat );
	}

	// can't see wounds on the player model in single player mode
	if ( !( IsType( idPlayer::Type ) ) ) {
		// place a wound overlay on the model
		key = va( "mtr_wound_%s", surfName.c_str() );
		decal = spawnArgs.RandomPrefix( key, gameLocal.random );
		if ( *decal == '\0' ) {
			decal = def->dict.RandomPrefix( key, gameLocal.random );
		}
		if ( *decal != '\0' ) {
			ProjectOverlay( origin, dir, 20.0f, decal );
		}
	}

	// a blood spurting wound is added
	key = va( "smoke_wound_%s", surfName.c_str() );
	bleed = spawnArgs.GetString( key );
	if ( *bleed == '\0' ) {
		bleed = def->dict.GetString( key );
	}
	if ( *bleed != '\0' ) {
		de = new damageEffect_t;
		de->next = this->damageEffects;
		this->damageEffects = de;

		de->jointNum = jointNum;
		de->localOrigin = localOrigin;
		de->localNormal = localNormal;
		de->type = static_cast<const idDeclParticle *>( declManager->FindType( DECL_PARTICLE, bleed ) );

		key = va( "smoke_chance_%s", surfName.c_str() );
		float chance;
		chance = def->dict.GetFloat( va("smoke_chance_%s", surfName.c_str()), "1.0" );
		if( gameLocal.random.RandomFloat() > chance )
			de->type = NULL;
		de->time = gameLocal.time;
	}
}

/*
==============
idAnimatedEntity::UpdateDamageEffects
==============
*/
void idAnimatedEntity::UpdateDamageEffects( void ) {
	damageEffect_t	*de, **prev;

	// free any that have timed out
	prev = &this->damageEffects;
	while ( *prev ) {
		de = *prev;
		if ( de->time == 0 ) {	// FIXME:SMOKE
			*prev = de->next;
			delete de;
		} else {
			prev = &de->next;
		}
	}

	if ( !g_bloodEffects.GetBool() ) {
		return;
	}

	// emit a particle for each bleeding wound
	for ( de = this->damageEffects; de; de = de->next ) {
		idVec3 origin, start;
		idMat3 axis;

		animator.GetJointTransform( de->jointNum, gameLocal.time, origin, axis );
		axis *= renderEntity.axis;
		origin = renderEntity.origin + origin * renderEntity.axis;
		start = origin + de->localOrigin * axis;
		if ( !gameLocal.smokeParticles->EmitSmoke( de->type, de->time, gameLocal.random.CRandomFloat(), start, axis ) ) {
			de->time = 0;
		}
	}
}

/*
================
idAnimatedEntity::ClientReceiveEvent
================
*/
bool idAnimatedEntity::ClientReceiveEvent( int event, int time, const idBitMsg &msg ) {
	return false;
}

/*
================
idAnimatedEntity::Attach
================
*/
void idAnimatedEntity::Attach( idEntity *ent, const char *PosName, const char *AttName )
{
	idVec3			origin;
	idMat3			axis;
	jointHandle_t	joint;
	idStr			jointName1,jointName2;
	idAngles		angleOffset, angleSubOffset(0.0f,0.0f,0.0f);
	idVec3			originOffset, originSubOffset(vec3_zero);
	idStr			nm;
	idStr			ClassName;
	SAttachPosition *pos;

// New position system:
	if( PosName && ((pos = GetAttachPosition(PosName)) != NULL) )
	{
		joint = pos->joint;

		originOffset = pos->originOffset;
		angleOffset = pos->angleOffset;

		// entity-specific offsets to a given position
		// entity-specific offsets to a given position
		originSubOffset = ent->spawnArgs.GetVector( va("origin_%s", PosName ) );
		angleSubOffset = ent->spawnArgs.GetAngles( va("angles_%s", PosName ) );
	}
// Old system, will be phased out
	else
	{
		jointName1 = ent->spawnArgs.GetString( "joint" );
		joint = animator.GetJointHandle( jointName1 );
		if ( joint == INVALID_JOINT ) 
		{
			jointName2 = ent->spawnArgs.GetString("bindToJoint");
			joint = animator.GetJointHandle( jointName2 );
			if ( joint == INVALID_JOINT )
			{
				gameLocal.Error( "Neither joint '%s' nor bindToJoint '%s' found for attaching '%s' on '%s'", jointName1.c_str(), jointName2.c_str(),ent->GetClassname(), name.c_str() );
			}
		}

		// Sparhawk's classname-specific offset system
		// Will be phased out in favor of attachment positions
		spawnArgs.GetString("classname", "", ClassName);
		sprintf(nm, "angles_%s", ClassName.c_str());
		if(ent->spawnArgs.GetAngles(nm.c_str(), "0 0 0", angleOffset) == false)
			angleOffset = ent->spawnArgs.GetAngles( "angles" );

		sprintf(nm, "origin_%s", ClassName.c_str());
		if(ent->spawnArgs.GetVector(nm.c_str(), "0 0 0", originOffset) == false)
		{
			originOffset = ent->spawnArgs.GetVector( "origin" );
		}
	}

	GetAttachingTransform( joint, origin, axis );
	idMat3 rotate = angleOffset.ToMat3();
	idMat3 newAxis = rotate * axis;
	
	// Use the local joint axis instead of the overall AI axis
	if (!ent->spawnArgs.GetBool("is_attachment"))
	{
		// angua: don't set origin and axis for attachments added in the map
		// rather than spawned dynamically, this would lead to the entity 
		// floating around through half of the map
		ent->SetOrigin( origin + originOffset * axis + originSubOffset * newAxis );
		ent->SetAxis( angleSubOffset.ToMat3() * newAxis );
	}

	ent->BindToJoint( this, joint, true );
	ent->cinematic = cinematic;

	SetCinematicOnTeam(this); // grayman #3156

	CAttachInfo	&attach = m_Attachments.Alloc();
	attach.channel = animator.GetChannelForJoint( joint );
	attach.ent = ent;
	attach.name = AttName;
	attach.posName = PosName; // grayman #2603

	// Update name->m_Attachment index mapping
	int index = m_Attachments.Num() - 1;
	if( AttName != NULL )
		m_AttNameMap.insert(AttNameMap::value_type(AttName, index));
}

/*
================
idAnimatedEntity::GetAttachingTransform
================
*/
void idAnimatedEntity::GetAttachingTransform( jointHandle_t jointHandle, idVec3 &offset, idMat3 &axis )
{
	GetJointWorldTransform( jointHandle, gameLocal.time, offset, axis );
}

/*
========================
idAnimatedEntity::ReAttachToCoords
========================
*/
void idAnimatedEntity::ReAttachToCoordsOfJoint
	( const char *AttName, idStr jointName, 
		const idVec3 offset, const idAngles angles  ) 
{
	idEntity		*ent( NULL );
	idVec3			origin;
	idMat3			axis, rotate, newAxis;
	jointHandle_t	joint;
	CAttachInfo		*attachment( NULL );

	attachment = GetAttachInfo( AttName );
	if( attachment )
		ent = attachment->ent.GetEntity();

	if( !attachment  || !attachment->ent.IsValid() || !ent )
	{
		// TODO: log bad attachment entity error
		goto Quit;
	}

	joint = animator.GetJointHandle( jointName );
	if ( joint == INVALID_JOINT )
	{
		// TODO: log error
		gameLocal.Warning( "Joint '%s' not found for attaching '%s' on '%s'", jointName.c_str(), ent->GetClassname(), name.c_str() );
		goto Quit;
	}

	attachment->channel = animator.GetChannelForJoint( joint );
	GetJointWorldTransform( joint, gameLocal.time, origin, axis );

	rotate = angles.ToMat3();
	newAxis = rotate * axis;

	ent->Unbind(); 

	// greebo: Note that Unbind() will invalidate the entity pointer in the attachment list
	// Hence, re-assign the attachment entity pointer (the index itself is ok)
	attachment->ent = ent;

	ent->SetAxis( newAxis );
	// Use the local joint axis instead of the overall AI axis
	ent->SetOrigin( origin + offset * axis );

	ent->BindToJoint( this, joint, true );
	ent->cinematic = cinematic;

	SetCinematicOnTeam(this); // grayman #3156

	// set the spawnargs for later retrieval as well
	ent->spawnArgs.Set( "joint", jointName.c_str() );
	ent->spawnArgs.SetVector( "origin", offset );
	ent->spawnArgs.SetAngles( "angles", angles );

	attachment->posName = jointName; // grayman #2603

Quit:
	return;
}

/*
===============
idAnimatedEntity::CacheAnimRates
===============
*/
void idAnimatedEntity::CacheAnimRates( void )
{
	// Cache animation rates
	int anims = animator.NumAnims();
	m_animRates.Clear();
	m_animRates.AssureSize(anims);
	for (int i=0; i<anims; i++) 
	{
		const idAnim *anim = animator.GetAnim(i);
		if (anim != NULL) 
		{
			idStr spawnargname = "anim_rate_";
			spawnargname += anim->Name();
			m_animRates[i] = spawnArgs.GetFloat(spawnargname, "1");
		} else 
		{
			m_animRates[i] = 1.0f;
		}
	}
}

/*
================
idAnimatedEntity::GetJointWorldPos

Returns the position of the joint (by joint name) in worldspace
================
*/
idVec3 idAnimatedEntity::GetJointWorldPos( const char *jointname ) {
	idVec3 offset;
	idMat3 axis;

	jointHandle_t jointnum = animator.GetJointHandle( jointname );
	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	return offset;
}

/*
================
idAnimatedEntity::GetEntityFromClassClosestToJoint

tels: Returns the entity that is nearest to the given joint, selected from all entities
that have a spawnarg "AIUse" matching AIUseType. max_dist_sqr = max_dist * max_dist
================
*/
idEntity* idAnimatedEntity::GetEntityFromClassClosestToJoint( const idVec3 joint_origin, const char* AIUseType, const float max_dist_sqr )
{
	idEntity* closest = NULL;
	float closest_distance = idMath::INFINITY;

	idStr aiuse = AIUseType;
	aiuse = aiuse.Left(6);

	// if the selector is not "AIUSE_...", skip this step
	if (aiuse != "AIUSE_") {
		return NULL;
	}

	// for all entities
    for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
        if ( ent == NULL ) { continue; }

		// does it have the proper AIUse spawnarg?
		if (idStr::Cmp(ent->spawnArgs.GetString("AIUse"), AIUseType) != 0)
		{
			continue;
		}
		
		//gameLocal.Warning ( "  Looking at entity for AIUse spawnarg %s", tempkv->GetValue().c_str() );

		// has the proper AIUse type, so compute distance	
		//gameLocal.Printf("Has %s\n", AIUseType);

		const idVec3& deb = ent->GetPhysics()->GetOrigin();
		idVec3 diff = joint_origin - deb;

		// gameLocal.Printf("Distance: %f %f %f - %f %f %f = %f %f %f\n", joint_origin.x, joint_origin.y, joint_origin.z, deb.x, deb.y, deb.z, diff.x, diff.y, diff.z);

		float distance = diff.LengthSqr();

		//gameLocal.Printf(" Distance %f < closest %f ? < max_dist_sqr %f ?.\n", distance, closest_distance, max_dist_sqr);

		if (distance < closest_distance && distance < max_dist_sqr)
		{
			// use this one
			//gameLocal.Printf(" Distance %f < closest %f and < max_dist_sqr %f, so will use this entity.\n", distance, closest_distance, max_dist_sqr);
			closest_distance = distance;
			closest = ent;
		}

		// try ent
	}

	return closest;
}

/*
================
idAnimatedEntity::GetEntityClosestToJoint

tels: Returns the entity that is nearest to the given joint. The entitySelector is either
an entity name (like "atdm_moveable_food_apple_2" or an entity AIUSE type like "AIUSE_FOOD".

First the code looks at the idAnimatedEntity itself for spawnargs matching the given prefix,
each entity mentioned in this spawnarg is used as the entitySelector first.
If after this round no entity could be found, we fall back to entitySelector and try again.
================
*/
idEntity* idAnimatedEntity::GetEntityClosestToJoint( const char* posName, const char* entitySelector, const char* prefix, const float max_dist ) {
	idEntity* closest = NULL;
	float closest_distance = 1000000.0f;
	idVec3 joint_origin;
	idMat3 joint_axis;
	const float max_dist_sqr = max_dist * max_dist;
	SAttachPosition *pos;

	jointHandle_t jointnum = INVALID_JOINT;
	
// New position system:
	if( (pos = GetAttachPosition(posName)) != NULL) {
		jointnum = pos->joint;
	}
	if (jointnum < 0) {
		gameLocal.Warning( "GetEntityClosestToJoint: Cannot find joint from attach pos %s", posName );
		return NULL;
	}
	if ( !GetJointWorldTransform( jointnum, gameLocal.time, joint_origin, joint_axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	// first see if the entity (the animation is running on) defines an entity
	// via spawnarg: "pickup_" + "name_of_the_animation" (we get this as prefix)

	if (prefix && *prefix != 0x00)
	{
		const idKeyValue *kv = spawnArgs.MatchPrefix( prefix, NULL );
		while( kv )
		{
			idEntity *target = gameLocal.FindEntity( kv->GetValue() );
			if (!target) {
				gameLocal.Warning ( " Can't find entity by name %s, so looking for AIUSE instead.", kv->GetValue().c_str() );
				// haven't found an entity, maybe it is something like "AIUSE_FOOD"?
				target = GetEntityFromClassClosestToJoint( joint_origin, kv->GetValue().c_str(), max_dist_sqr );
			}
			// if we found an entity and it is not ourself:
			if (target != NULL && target != this)
			{
				// check that it is closer than the closest match so far
				idVec3 diff = joint_origin - target->GetPhysics()->GetOrigin();
				float distance = diff.LengthSqr();
				if (distance < closest_distance && distance < max_dist_sqr) {
					// closer and close enough
					// gameLocal.Printf ( " distance %f < closest_distance %f and < max_dist_sqr %f ", distance, closest_distance, max_dist_sqr );
					closest_distance = distance;
					closest = target;
				}
			}
		// next spawnarg
		kv = spawnArgs.MatchPrefix( prefix, kv );
		}
	}

	if (closest == NULL) {
		// couldn't find any usable entity from the the spawnargs
		// so try to use the entity named in the animation
		if (prefix && *prefix != 0x00)
		{
			gameLocal.Warning ( " Didn't find an entity from the %s spawnargs, trying %s directly.", prefix, entitySelector );
		}
		closest = gameLocal.FindEntity( entitySelector );
		if (closest) {
			idVec3 diff = joint_origin - closest->GetPhysics()->GetOrigin();
			float distance = diff.LengthSqr();
			if (distance > max_dist_sqr) {
				// cannot use this one, too far
				gameLocal.Warning ( "   Direct entity %s too far away. (%f > %f).", entitySelector, distance, max_dist_sqr );
				closest = NULL;
			}
		}
		// could not be found either or was too far away?
		if (closest == NULL) {
			// maybe it is a generic AIUSE class, try to find a suitable entity
			gameLocal.Warning ( "   Can't find entity by name %s, so looking for AIUSE class.", entitySelector );
			closest = GetEntityFromClassClosestToJoint( joint_origin, entitySelector, max_dist_sqr );
		}
	}

//	gameLocal.Printf ( "   End of selecting closest entity.");

	// now return the found entity or NULL
	return closest;
}

/*
================
idAnimatedEntity::Event_GetJointHandle

looks up the number of the specified joint.  returns INVALID_JOINT if the joint is not found.
================
*/
void idAnimatedEntity::Event_GetJointHandle( const char *jointname ) {

	jointHandle_t jointnum = animator.GetJointHandle( jointname );
	idThread::ReturnInt( jointnum );
}

/*
================
idAnimatedEntity::Event_ClearAllJoints

removes any custom transforms on all joints
================
*/
void idAnimatedEntity::Event_ClearAllJoints( void ) {
	animator.ClearAllJoints();
}

/*
================
idAnimatedEntity::Event_ClearJoint

removes any custom transforms on the specified joint
================
*/
void idAnimatedEntity::Event_ClearJoint( jointHandle_t jointnum ) {
	animator.ClearJoint( jointnum );
}

/*
================
idAnimatedEntity::Event_SetJointPos

modifies the position of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos ) {
	animator.SetJointPos( jointnum, transform_type, pos );
}

/*
================
idAnimatedEntity::Event_SetJointAngle

modifies the orientation of the joint based on the transform type
================
*/
void idAnimatedEntity::Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles &angles ) {
	idMat3 mat;

	mat = angles.ToMat3();
	animator.SetJointAxis( jointnum, transform_type, mat );
}

/*
================
idAnimatedEntity::Event_GetJointPos

returns the position of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointPos( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	idThread::ReturnVector( offset );
}

/*
================
idAnimatedEntity::Event_GetJointAngle

returns the orientation of the joint in worldspace
================
*/
void idAnimatedEntity::Event_GetJointAngle( jointHandle_t jointnum ) {
	idVec3 offset;
	idMat3 axis;

	if ( !GetJointWorldTransform( jointnum, gameLocal.time, offset, axis ) ) {
		gameLocal.Warning( "Joint # %d out of range on entity '%s'",  jointnum, name.c_str() );
	}

	idAngles ang = axis.ToAngles();
	idVec3 vec( ang[ 0 ], ang[ 1 ], ang[ 2 ] );
	idThread::ReturnVector( vec );
}

/*
===============================================================================

  End of idAnimatedEntity
	
===============================================================================
*/

void idEntity::Flinderize( idEntity *activator )
{
	// Create a new struct
	FlinderSpawn fs;
	// count of entities that were actually spawned
	int spawned = 0;

	// tels: go through all the def_flinder spawnargs and call SpawnFlinder() for each
	const idKeyValue *kv = spawnArgs.MatchPrefix( "def_flinder", NULL );
	while ( kv )
	{
		idStr temp = kv->GetValue();
		DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Loading def_flinder %s:\r", temp.c_str() );
		if ( !temp.IsEmpty() )
		{
			// fill in the name and some defaults
			fs.m_Entity = temp;
			fs.m_Offset.Zero();
			fs.m_Count = 0;
			fs.m_Probability = 1.0;

			// check if we have spawnargs like count, offset or probability:
			idStr index;
			idStr spawnarg = kv->GetKey();
			// strlen("def_flinder") == 11 
			if (spawnarg.Length() > 11)
			{
				index = spawnarg.Right( spawnarg.Length() - 11 );
			}
			// DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING(" Index is %s:\r", index.c_str() );
			spawnArgs.GetVector("flinder_offset"      + index,    "", fs.m_Offset);
			spawnArgs.GetInt   ("flinder_count"       + index,   "1", fs.m_Count);
			spawnArgs.GetFloat ("flinder_probability" + index, "1.0", fs.m_Probability);

			// Dragofer: consider orientation of the flinderizing entity in the flinder offset
			fs.m_Offset *= GetPhysics()->GetAxis();

			//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("  Offset is %f,%f,%f:\r", fs.m_Offset.x, fs.m_Offset.y, fs.m_Offset.z );
			//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("  Count is %i:\r", fs.m_Count );
			//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("  Probability is %f:\r", fs.m_Probability );

			// evaluate what we found and spawn the individual pieces

			spawned += SpawnFlinder( fs, activator );
		}

		kv = spawnArgs.MatchPrefix( "def_flinder", kv );
	} // while MatchPrefix ("def_flinder")

	// if we spawned flinders, but have no broken model, remove this entity
	if ( spawned > 0 && !brokenModel.Length() )
	{
		Hide();
		// remove us in 0.01 seconds
		PostEventMS( &EV_Remove, 10 );
		// and make inactive in the meantime
		BecomeInactive(TH_PHYSICS|TH_THINK);
	}
}

void idEntity::LoadTDMSettings(void)
{
	idStr str;

	// If an item has the frobable flag set to true we will use the 
	// the default value. If the frobdistance is set in the item
	// it will override the defaultsetting. If none of that is set
	// the frobdistance will be set to 0 meaning no frobbing on that item.
	// If the frobsetting is alread initialized we can skip this.
	if(m_FrobDistance == 0)
	{
		spawnArgs.GetBool("frobable", "0", m_bFrobable);
		spawnArgs.GetBool("frob_simple", "0", m_bFrobSimple);
		spawnArgs.GetInt("frob_distance", "0", m_FrobDistance);
		spawnArgs.GetFloat("frob_bias", "1.0", m_FrobBias);

		if( m_FrobDistance <= 0  )
			m_FrobDistance = cv_frob_distance_default.GetInteger();

		if( m_bFrobable && m_FrobBox )
			m_FrobBox->SetContents(CONTENTS_FROBABLE);
	}

	// update the max frobdistance if necessary
	if( m_FrobDistance > g_Global.m_MaxFrobDistance )
		g_Global.m_MaxFrobDistance = m_FrobDistance;

	// Override the frob action script to apply custom events to 
	// specific entities.
	if(spawnArgs.GetString("frob_action_script", "", str))
		m_FrobActionScript = str;

	// Check if this entity is associated to a master frob entity.
	str = spawnArgs.GetString("frob_master");

	// Disallow "self" to be added as same name
	if (!str.IsEmpty() && str != name)
	{
		m_MasterFrob = str;
	}

	const idKeyValue *kv = spawnArgs.MatchPrefix( "frob_peer", NULL );
	// Fill the list of frob peers
	while( kv )
	{
		idStr temp = kv->GetValue();
		if( !temp.IsEmpty() )
			m_FrobPeers.AddUnique(temp);
		kv = spawnArgs.MatchPrefix( "frob_peer", kv );
	}

	// Check if this entity can be used by others.
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("used_by"); kv != NULL; kv = spawnArgs.MatchPrefix("used_by", kv))
	{
		// argh, backwards compatibility, must keep old used_by format for used_by_name
		// explicitly ignore stuff with other prefixes
		idStr kn = kv->GetKey();
		if( !kn.IcmpPrefix("used_by_inv_name") || !kn.IcmpPrefix("used_by_category") || !kn.IcmpPrefix("used_by_classname") )
			continue;

		// Add each entity name to the list
		m_UsedByName.AddUnique(kv->GetValue());
	}
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("used_by_inv_name"); kv != NULL; kv = spawnArgs.MatchPrefix("used_by_inv_name", kv))
	{
		m_UsedByInvName.AddUnique(kv->GetValue());
	}
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("used_by_category"); kv != NULL; kv = spawnArgs.MatchPrefix("used_by_category", kv))
	{
		m_UsedByCategory.AddUnique(kv->GetValue());
	}
	for (const idKeyValue* kv = spawnArgs.MatchPrefix("used_by_classname"); kv != NULL; kv = spawnArgs.MatchPrefix("used_by_classname", kv))
	{
		m_UsedByClassname.AddUnique(kv->GetValue());
	}

	m_AbsenceNoticeability = spawnArgs.GetFloat("absence_noticeability", "0");

	team = spawnArgs.GetInt("team", "-1");

	m_bAttachedAlertControlsSolidity = spawnArgs.GetBool("on_attach_alert_become_solid");

	m_bIsObjective = spawnArgs.GetBool( "objective_ent", "0" );

	m_bIsClimbableRope = spawnArgs.GetBool( "is_climbable_rope", "0" );

	m_bIsMantleable = spawnArgs.GetBool( "is_mantleable", "1" );

	DM_LOG(LC_FROBBING, LT_INFO)LOGSTRING("[%s] this: %08lX FrobDistance: %u\r", name.c_str(), this, m_FrobDistance);
}

void idEntity::UpdateFrobState()
{
	// hidden objects are skipped
	if (IsHidden()) 
	{
		return;
	}

	// greebo: Allow the grabbed entity to stay highlighted
	// grayman #3631 - highlight the head of a grabbed body if it's separate
	if (cv_dragged_item_highlight.GetBool())
	{
		if (gameLocal.m_Grabber->GetSelected() == this)
		{
			SetFrobHighlightState(true);
			return;
		}

		// Any selection of the head is xferred to the body, so for a highlighted
		// body in the grabber, the body is always the entity being grabbed, and
		// the head never is. So if this is a head, and the body is being grabbed,
		// we want to highlight the head here.

		idEntity* bindMaster = GetBindMaster();
		if (bindMaster && bindMaster->IsType(idActor::Type))
		{
			idActor* actor = static_cast<idActor*>(bindMaster);
			if (gameLocal.m_Grabber->GetSelected() == actor)
			{
				idAFAttachment* head = actor->GetHead();
				if (head == this)
				{
					SetFrobHighlightState(true);
					return;
				}
			}
		}
	}

	if ( !m_bFrobbed )	
	{
		// The m_bFrobbed variable is FALSE, hence the player is not looking at this entity
		// or any of its peers. Check if we need to change our state.
		if (m_bFrobHighlightState)
		{
			// stop highlight
			SetFrobHighlightState(false);
		}

		// Done, we're not frobbed, frobhighlight state is cleared
		return;
	}

	// Change the highlight state to TRUE
	SetFrobHighlightState(true);
}

void idEntity::SetFrobHighlightState(const bool newState)
{
	if (m_bFrobHighlightState == newState) return; // Avoid unnecessary work

	// Update the boolean, the UpdateFrobDisplay() routine will pick it up
	m_bFrobHighlightState = newState;

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("Entity [%s]: highlighting is turned %s\r", name.c_str(),newState ? "on" : "off");
}

void idEntity::UpdateFrobDisplay()
{
	// FIX: If we have just been set not frobable, go instantly to un-frobbed hilight state
	if (!m_bFrobable)
	{
		SetShaderParm(FROB_SHADERPARM, 0.0f);
		return;
	}

	float param = renderEntity.shaderParms[ FROB_SHADERPARM ];
	
	if ( (param == 0 && !m_bFrobHighlightState) || (param >= 1.0f && m_bFrobHighlightState) )
	{
		// Nothing to do
		return;
	}

	param = m_bFrobHighlightState;

	SetShaderParm(FROB_SHADERPARM, param);
}

void idEntity::FrobAction(bool frobMaster, bool isFrobPeerAction)
{
	if (m_FrobActionLock) return; // prevent double-entering this function

	if (IsHidden()) return; // don't do frobactions on hidden entities

	m_FrobActionLock = true;

	// greebo: If we have a frob master, just redirect the call and skip everything else
	idEntity* master = GetFrobMaster();

	if (master != NULL)
	{
		master->FrobAction(frobMaster, isFrobPeerAction);

		m_FrobActionLock = false;
		return;
	}

	// Propagate frobactions to all peers to get them triggered as well.
	if (!isFrobPeerAction)
	{
		for (int i = 0; i < m_FrobPeers.Num(); i++)
		{
			idEntity* ent = gameLocal.FindEntity(m_FrobPeers[i]);

			if (ent != NULL && ent != this)
			{
				// Propagate the frob action, but inhibit calls to frob_masters
				ent->FrobAction(false, true);
			}
		}
	}

	DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("This: [%s]   Master: %08lX (%u)\r", name.c_str(), m_MasterFrob.c_str(), frobMaster);

	// Call the frob action script, if available
	if (m_FrobActionScript.Length() > 0)
	{
		idThread* thread = CallScriptFunctionArgs(m_FrobActionScript, true, 0, "e", this);

		if (thread != NULL)
		{
			// greebo: Run the thread at once, the script result might be needed below.
			thread->Execute();
		}
	}
	else
	{
		DM_LOG(LC_FROBBING, LT_DEBUG)LOGSTRING("(%08lX->[%s]) FrobAction has been triggered with empty FrobActionScript!\r", this, name.c_str());
	}

	// Play the (optional) acquire sound (greebo: What's this? Isn't this the wrong place here?)
	StartSound( "snd_acquire", SND_CHANNEL_ANY, 0, false, NULL );

	m_FrobActionLock = false;
}

void idEntity::AttackAction(idPlayer* player)
{
	idEntity* master = GetFrobMaster();

	if (master != NULL) 
	{
		master->AttackAction(player);
	}
}

bool idEntity::CanBeUsedByItem(const CInventoryItemPtr& item, const bool isFrobUse) 
{
	if (item == NULL)
	{
		return false;
	}

	// Redirect the call to the master if we have one
	idEntity* master = GetFrobMaster();
	if ( master != NULL )
	{
		return  master->CanBeUsedByItem(item, isFrobUse);
	}

	// No frob master set
	// Check entity name (if exists), inv_name, inv_category, and classname
	bool bMatchName( false ), bMatchInvName( false );
	bool bMatchCategory( false ), bMatchClassname( false );

	idEntity* ent = item->GetItemEntity();
	if ( ent != NULL )
	{
		bMatchName = ( m_UsedByName.FindIndex(ent->name) != -1 );
		bMatchClassname = ( m_UsedByClassname.FindIndex(ent->GetEntityDefName()) != -1 );
	}

	if ( bMatchName || bMatchClassname )
	{
		return true;
	}

	bMatchInvName = ( m_UsedByInvName.FindIndex(item->GetName()) != -1 );
	bMatchCategory = ( m_UsedByCategory.FindIndex(item->Category()->GetName()) != -1 );
	
	return (bMatchInvName || bMatchCategory);
}

bool idEntity::CanBeUsedByEntity(idEntity* entity, const bool isFrobUse) 
{
	if (entity == NULL) return false;

	// Redirect the call to the master if we have one
	idEntity* master = GetFrobMaster();

	if (master != NULL) 
		return master->CanBeUsedByEntity(entity, isFrobUse);

	// No frob master set
	// Check entity name, inv_name, inv_category, and classname
	bool bMatchName( false ), bMatchInvName( false );
	bool bMatchCategory( false ), bMatchClassname( false );

	bMatchName = ( m_UsedByName.FindIndex(entity->name) != -1 );
	bMatchClassname = ( m_UsedByClassname.FindIndex(entity->GetEntityDefName()) != -1 );
	
	if( bMatchName || bMatchClassname )
		return true;

	// Tels: TODO: Try here also English names instead of "#str_12345"
//	gameLocal.Printf("canUsedBy name %s category %s\n", 
//				entity->spawnArgs.GetString("inv_name"),
//				entity->spawnArgs.GetString("inv_category") );

	// This may be called when the entity is not in the inventory, so have to read from spawnargs
	bMatchInvName = ( m_UsedByInvName.FindIndex(entity->spawnArgs.GetString("inv_name")) != -1 );
	bMatchCategory = ( m_UsedByCategory.FindIndex(entity->spawnArgs.GetString("inv_category")) != -1 );

	return (bMatchInvName || bMatchCategory);
}

bool idEntity::UseByItem(EImpulseState impulseState, const CInventoryItemPtr& item)
{
	// Redirect the call to the master if we have one
	idEntity* master = GetFrobMaster();

	if (master != NULL)
	{
		return master->UseByItem(impulseState, item);
	}

	// no master, continue...
	// Ishtvan: Call used_by script.  First check for scripts with the specific name/inv_name/classname/category/
	// only call the most-specific script
	idStrList suffixes;
	idEntity *ent = item->GetItemEntity();
	if ( ent != NULL )
	{
		suffixes.Append( ent->name );
		suffixes.Append( ent->GetEntityDefName() );
	}
	// inv_name comes before classname
	suffixes.Insert( item->GetName(), 1 );
	suffixes.Append( item->Category()->GetName() );

	idThread* thread = NULL;
	bool bFoundKey = false;
	for ( int i = 0 ; i < suffixes.Num() ; i++ )
	{
		idStr scriptName = "used_action_script_" + suffixes[i];
		const idKeyValue *kv = spawnArgs.FindKey( scriptName.c_str() );
		if ( kv != NULL && kv->GetValue().Length() > 0 )
		{
			thread = CallScriptFunctionArgs(kv->GetValue().c_str(), true, 0, "e", this);
			bFoundKey = true;
			break;
		}
	}
	
	// if we didn't find a specific suffix, use "used_action_script" by itself
	if ( !bFoundKey )
	{
		const idKeyValue *kv = spawnArgs.FindKey( "used_action_script" );
		if ( kv != NULL && kv->GetValue().Length() > 0 )
		{
			thread = CallScriptFunctionArgs(kv->GetValue().c_str(), true, 0, "e", this);
		}
	}

	if (thread != NULL)
	{
		// greebo: Run the thread at once, the script result might be needed below.
		thread->Execute();
		return true;
	}

	return false;
}

void idEntity::SetFrobbed( const bool val )
{
	if (m_bFrobbed == val) return; // avoid loopbacks and unnecessary work

	m_bFrobbed = val;
	//stgatilov: trigger visual update for the entity
	BecomeActive( TH_UPDATEVISUALS );

	// resolve the peer names into entities
	for (int i = 0; i < m_FrobPeers.Num(); i++)
	{
		if (m_FrobPeers[i].Length() == 0) continue;

		idEntity* ent = gameLocal.FindEntity(m_FrobPeers[i]);

		// don't call it on self, would get stuck in a loop
		if (ent != NULL && ent != this)
		{
			ent->SetFrobbed(m_bFrobbed);
		}
	}
}

bool idEntity::IsFrobbed( void ) const
{
	return m_bFrobbed;
}

void idEntity::SetFrobable( const bool bVal )
{
	// greebo: This is to avoid infinite loops
	if (m_bFrobable == bVal) return; 

	m_bFrobable = bVal;

	// If this entity is currently being hilighted, make sure to un-frob it
	if( !bVal )
	{
		SetFrobbed(false);
		SetFrobHighlightState(false);
		if( m_FrobBox )
			m_FrobBox->SetContents(0);
	}
	else
	{
		if( m_FrobBox )
			m_FrobBox->SetContents(CONTENTS_FROBABLE);
	}

	UpdateFrobState();
	UpdateFrobDisplay();

	// Make sure Present gets called when we make something frobable
	BecomeActive( TH_UPDATEVISUALS );

	// greebo: Also update all frob peers
	for (int i = 0; i < m_FrobPeers.Num(); i++)
	{
		idEntity* peer = gameLocal.FindEntity(m_FrobPeers[i]);
		if (peer == NULL) continue;

		peer->SetFrobable(bVal);
	}
}

void idEntity::Event_StimAdd(int stimType, float radius)
{
	AddStim(static_cast<StimType>(stimType), radius);
}

void idEntity::Event_StimRemove(int stimType)
{
	RemoveStim(static_cast<StimType>(stimType));
}

void idEntity::Event_StimEnable(int stimType, int enabled)
{
	SetStimEnabled(static_cast<StimType>(stimType), enabled != 0);
}

void idEntity::Event_StimClearIgnoreList(int stimType)
{
	ClearStimIgnoreList(static_cast<StimType>(stimType));
}

void idEntity::Event_StimEmit(int stimType, float radius, idVec3& stimOrigin)
{
	CStimResponseCollection* srColl = GetStimResponseCollection();
	assert(srColl != NULL);
	const CStimPtr& stim = srColl->GetStimByType( static_cast<StimType>(stimType) );
	if( stim == NULL )
		return;

	if ( stim->m_bScriptBased ) {
		stim->m_bScriptFired = true;
		stim->m_ScriptRadiusOverride = radius;
		stim->m_ScriptPositionOverride = stimOrigin;
	}
}

void idEntity::Event_StimSetScriptBased(int stimType, bool state)
{
	CStimResponseCollection* srColl = GetStimResponseCollection();
	assert(srColl != NULL);
	const CStimPtr& stim = srColl->GetStimByType( static_cast<StimType>(stimType) );
	if( stim == NULL )
		return;

	stim->m_bScriptBased = state;
}

void idEntity::Event_ResponseEnable(int stimType, int enabled)
{
	SetResponseEnabled(static_cast<StimType>(stimType), enabled != 0);
}

void idEntity::Event_ResponseAdd(int stimType)
{
	AddResponse(static_cast<StimType>(stimType));
}

void idEntity::Event_ResponseTrigger(idEntity* source, int stimType)
{
	TriggerResponse(source, static_cast<StimType>(stimType));
}

void idEntity::Event_ResponseRemove(int stimType)
{
	RemoveResponse(static_cast<StimType>(stimType));
}

void idEntity::Event_ResponseIgnore(int stimType, idEntity *e)
{
	IgnoreResponse(static_cast<StimType>(stimType), e);
}

void idEntity::Event_ResponseAllow(int stimType, idEntity *e)
{
	AllowResponse(static_cast<StimType>(stimType), e);
}

void idEntity::Event_ResponseSetAction(int stimType, const char* action)
{
	CResponsePtr resp = m_StimResponseColl->GetResponseByType(static_cast<StimType>(stimType));

	if (resp != NULL)
	{
		resp->SetResponseAction(action);
	}
}

void idEntity::AllowResponse(StimType type, idEntity* fromEntity)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(type);

	if (stim != NULL)
	{
		stim->RemoveResponseIgnore(fromEntity);
	}
}

void idEntity::IgnoreResponse(StimType type, idEntity* fromEntity)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(type);

	if (stim != NULL)
	{
		stim->AddResponseIgnore(fromEntity);
	}
}

// grayman #2872

bool idEntity::CheckResponseIgnore(const StimType type, const idEntity* fromEntity) const
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(type);

	if (stim != NULL)
	{
		return ( stim->CheckResponseIgnore(fromEntity) );
	}
	return true;
}


void idEntity::SetStimEnabled(StimType type, bool enabled)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(type);

	if (stim != NULL)
	{
		stim->SetEnabled(enabled);
	}
}

void idEntity::SetResponseEnabled(StimType type, bool enabled)
{
	CResponsePtr response = m_StimResponseColl->GetResponseByType(type);

	if (response != NULL)
	{
		response->SetEnabled(enabled);
	}
}

void idEntity::ClearStimIgnoreList(StimType type)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(type);

	if (stim != NULL)
	{
		stim->ClearResponseIgnoreList();
	}
}

void idEntity::RemoveStim(StimType type)
{
	if (m_StimResponseColl->RemoveStim(type) == 0)
	{
		gameLocal.RemoveStim(this);
	}
}

void idEntity::RemoveResponse(StimType type)
{
	if (m_StimResponseColl->RemoveResponse(type) == 0)
	{
		gameLocal.RemoveResponse(this);
	}
}

CStimPtr idEntity::AddStim(StimType type, float radius, bool removable, bool isDefault)
{
	CStimPtr stim = m_StimResponseColl->AddStim(this, type, radius, removable, isDefault);

	gameLocal.AddStim(this);

	return stim;
}

CResponsePtr idEntity::AddResponse(StimType type, bool removable, bool isDefault)
{
	CResponsePtr response = m_StimResponseColl->AddResponse(this, type, removable, isDefault);

	gameLocal.AddResponse(this);

	return response;
}

void idEntity::TriggerResponse(idEntity* source, StimType type)
{
	// Try to lookup the response for this item
	CResponsePtr resp = GetStimResponseCollection()->GetResponseByType(type);

	if (resp == NULL || resp->m_State != SS_ENABLED)
	{
		return;
	}

	// There is an enabled response defined
	// Check duration, disable if past duration
	if (resp->m_Duration && (gameLocal.time - resp->m_EnabledTimeStamp) > resp->m_Duration)
	{
		resp->Disable();
	}
	else 
	{
		// Fire the response and pass the originating entity and no stim object
		// no stim means that this is no "real" stim just a temporary or virtual one
		resp->TriggerResponse(source);
	}
}


/**	Called when m_renderTrigger is rendered.
 *	It will resume the m_renderWaitingThread.
 */
bool idEntity::WaitForRenderTriggered( renderEntity_s* renderEntity, const renderView_s* view )
{
	// NOTE: We must avoid changing the game state from within this function.
	// However, we'll add an event to resume the suspended thread.

	idEntity* ent = gameLocal.entities[ renderEntity->entityNum ];
	if ( !ent )
		gameLocal.Error( "idEntity::WaitForRenderTriggered: callback with NULL game entity" );

	renderEntity->callback = NULL;

	if ( ent->m_renderWaitingThread )
	{
		// Fortunately, this doesn't execute the script immediately, so
		// I think it's safe to call from here.
		idThread::ObjectMoveDone( ent->m_renderWaitingThread, ent );
		ent->m_renderWaitingThread = 0;
	}

	return false;
}

/**	Called to update m_renderTrigger after the render entity is modified.
 *	Only updates the render trigger if a thread is waiting for it.
 */
void idEntity::PresentRenderTrigger()
{
	if ( !m_renderWaitingThread )
	{
		goto Quit;
	}

	// Update the renderTrigger to match renderEntity's bounding box.
	// In order to ensure that other code can easily add to the bounding
	// box, we're going to pre-rotate the bounding box, and keep the axis
	// unrotated.
	m_renderTrigger.origin = renderEntity.origin;
	m_renderTrigger.bounds = renderEntity.bounds * renderEntity.axis;
	// I haven't yet figured out where renderEntity.entityNum is set...
	// but it looks like we have to manually set it for m_renderTrigger.
	m_renderTrigger.entityNum = entityNumber;

	// Update the renderTrigger in the render world.
	if ( m_renderTriggerHandle == -1 )
		m_renderTriggerHandle = gameRenderWorld->AddEntityDef( &m_renderTrigger );
	else
		gameRenderWorld->UpdateEntityDef( m_renderTriggerHandle, &m_renderTrigger );

	Quit:
	return;
}

/*
================
idEntity::Inventory

This returns the inventory object of this entity. If this entity doesn't
have one, it creates the inventory.
================
*/
const CInventoryPtr& idEntity::Inventory()
{
	if (m_Inventory == NULL )
	{
		m_Inventory = CInventoryPtr(new CInventory());
		m_Inventory->SetOwner(this);
	}

	return m_Inventory;
}

/*
================
idEntity::InventoryCursor

This returns the inventory cursor object of this entity. If this entity
doesn't have one, it creates the inventory cursor.

The cursor is intended for arbitrary use, and need not point to this
entity's inventory.
================
*/
const CInventoryCursorPtr& idEntity::InventoryCursor()
{
	if (m_InventoryCursor == NULL)
	{
		m_InventoryCursor = Inventory()->CreateCursor();
	}

	return m_InventoryCursor;
}

void idEntity::OnAddToLocationEntity(CObjectiveLocation* locationEnt)
{
	idEntityPtr<CObjectiveLocation> locationEntPtr = locationEnt;

	// Ensure that objective locations don't add themselves twice or more times.

	// Tels: The assert below triggers under Debug build when loading Heart, took
	// it out since it will be ignored under Release builds anyway.

	// grayman: This assert was being triggered by multiple requests that
	// the location entity be registered with the same entity. This is legal (it
	// can happen) when that entity has multiple clip models, since clip model
	// intersection with the location entity was used to build the list of entities
	// inside the location entity. So the assert statement shouldn't be used, and
	// the routine should just handle being fed the same location entity, which it
	// already does.

	//assert(m_objLocations.FindIndex(locationEntPtr) == -1);

	if (m_objLocations.FindIndex(locationEntPtr) != -1)
	{
		return; // this entity already knows it's inside the location entity
	}

	m_objLocations.Alloc() = locationEntPtr;
}

void idEntity::OnRemoveFromLocationEntity(CObjectiveLocation* locationEnt)
{
	idEntityPtr<CObjectiveLocation> locationEntPtr = locationEnt;

	// Ensure that only registered location ents are removed
	assert(m_objLocations.FindIndex(locationEntPtr) != -1 || gameLocal.GameState() == GAMESTATE_SHUTDOWN);

	m_objLocations.Remove(locationEntPtr);
}

void idEntity::Event_PropSound( const char *sndName )
{
	PropSoundDirect( sndName, false, false, 0.0f, 0 ); // grayman #3355
}

void idEntity::Event_PropSoundMod( const char *sndName, float VolModIn )
{
	PropSoundDirect( sndName, false, false, VolModIn, 0 ); // grayman #3355
}

/*
================
idEntity::Event_InPVS

Returns non-zero if this entity is in the player's PVS, zero otherwise.
================
*/
void idEntity::Event_InPVS()
{
	idThread::ReturnFloat( gameLocal.InPlayerPVS( this ) );
}

/*
================
idEntity::Event_WaitForRender

Called by sys.waitForRender(ent) to find out if it can wait for this entity to render.
================
*/
void idEntity::Event_WaitForRender()
{
	if ( !m_renderWaitingThread )
	{
		// Give the renderTrigger an invisible model to prevent a black cube from showing up.
		if ( !m_renderTrigger.hModel )
			m_renderTrigger.hModel = renderModelManager->FindModel( cv_empty_model.GetString() );
		if ( !m_renderTrigger.hModel )
			gameLocal.Error( "Unable to load model: %s\n", cv_empty_model.GetString() );

		m_renderTrigger.callback = idEntity::WaitForRenderTriggered;
		m_renderWaitingThread = idThread::CurrentThreadNum();

		// Make sure Present() gets called.
		BecomeActive( TH_UPDATEVISUALS );

		idThread::ReturnInt( true );
	} else {
		idThread::ReturnInt( false );
	}
}

/*
================
idEntity::Event_SetGui

Changes the GUI file that's loaded by an overlay. Overlays 1, 2 and 3 are
reserved for the entity GUIs, and special code is implemented to change them.
A changed GUI does not see gui_parm spawn args. (for one thing, if there's a
script changing GUIs, then the script could easily send spawnargs to the GUI
if it wanted to.)
================
*/
void idEntity::Event_SetGui( int handle, const char *guiFile )
{
	if ( !uiManager->CheckGui(guiFile) )
	{
		gameLocal.Warning( "Unable to load GUI file: %s", guiFile );
		goto Quit;
	}

	if ( !m_overlays.exists( handle ) )
	{
		gameLocal.Warning( "Non-existant GUI handle: %d", handle );
		goto Quit;
	}

	// Entity GUIs are handled differently from regular ones.
	if ( ( handle >= 1 ) && ( handle <= MAX_RENDERENTITY_GUI ) )
	{
		assert( m_overlays.isExternal( handle ) );

		if ( renderEntity.gui[ handle-1 ] && renderEntity.gui[ handle-1 ]->IsUniqued() )
		{
			// We're dealing with an existing unique GUI.
			// We need to read a new GUI into it.

			// Clear the state.
			const idDict &state = renderEntity.gui[ handle-1 ]->State();
			const idKeyValue *kv;
			while ( ( kv = state.MatchPrefix( "" ) ) != NULL )
			{
				renderEntity.gui[ handle-1 ]->DeleteStateVar( kv->GetKey() );
			}

			renderEntity.gui[ handle-1 ]->InitFromFile( guiFile );
		}
		else
		{
			// We're either dealing with a non-existent GUI, or a non-unique one.
			// It's safe to just set the render entity to point to a new GUI without
			// bothering to deallocate the previous GUI.
			renderEntity.gui[ handle-1 ] = uiManager->FindGui( guiFile, true, true );
			m_overlays.setGui( handle, renderEntity.gui[ handle - 1 ] );
			assert( renderEntity.gui[ handle-1 ] );
		}
		//stgatilov: make sure renderEntity.gui gets into renderer
		BecomeActive(TH_UPDATEVISUALS);
	}
	else if ( !m_overlays.isExternal( handle ) )
	{
		bool result = m_overlays.setGui( handle, guiFile );
		assert( result );
	}
	else
	{
		gameLocal.Warning( "Cannot call setGui() on external handle: %d", handle );
	}

	Quit:
	return;
}

/*
================
idEntity::Event_GetGui

Returns the file loaded by a specific GUI.
================
*/
void idEntity::Event_GetGui( int handle )
{
	idUserInterface *gui = m_overlays.getGui( handle );
	if ( gui )
	{
		idThread::ReturnString( gui->Name() );
	}
	else
	{
		idThread::ReturnString( "" );
	}
}

void idEntity::SetGuiString(int handle, const char *key, const char *val)
{
	if (m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if (gui == NULL)
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
			goto Quit;
		}

		if (!gui->IsUniqued())
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("GUI is not unique. Handle: %d [%s]\r", handle, key);
			goto Quit;
		}

		gui->SetStateString(key, val);
		gui->StateChanged(gameLocal.time);
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Non-existant GUI handle: %d\r", handle);
	}

Quit:
	return;
}

/*
================
idEntity::Event_SetGuiString

Sets a parameter for a GUI. (note that the GUI needs to be unique)
================
*/
void idEntity::Event_SetGuiString(int handle, const char *key, const char *val)
{
	SetGuiString(handle, key, val);
}

const char *idEntity::GetGuiString(int handle, const char *key)
{
	const char *retStr = NULL;

	if (m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if (gui)
		{
			retStr = gui->GetStateString(key);
		}
		else
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
		}
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
	}

	return retStr;
}

void idEntity::Event_GetGuiString(int handle, const char *key)
{
	const char *retStr = GetGuiString(handle, key);

	if (retStr == NULL)
	{
		retStr = "";
	}

	idThread::ReturnString(retStr);
}

void idEntity::SetGuiFloat( int handle, const char *key, float f)
{
	if (m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if (gui == NULL)
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
			goto Quit;
		}

		if (!gui->IsUniqued())
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("GUI is not unique. Handle: %d [%s]\r", handle, key);
			goto Quit;
		}

		gui->SetStateFloat(key, f);
		gui->StateChanged(gameLocal.time);
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d\r", handle);
	}

Quit:
	return;
}

void idEntity::Event_SetGuiFloat(int handle, const char *key, float f)
{
	SetGuiFloat(handle, key, f);
}

float idEntity::GetGuiFloat(int handle, const char *key)
{
	float retVal = 0;
	if (m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui( handle );
		if (gui)
		{
			retVal = gui->GetStateFloat(key);
		}
		else
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
		}
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d\r", handle);
	}

	return retVal;
}

void idEntity::Event_GetGuiFloat(int handle, const char *key)
{
	idThread::ReturnFloat(GetGuiFloat(handle, key));
}

void idEntity::SetGuiInt( int handle, const char *key, int n)
{
	if (m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if (gui == NULL)
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
			goto Quit;
		}

		if (!gui->IsUniqued())
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("GUI is not unique. Handle: %d [%s]\r", handle, key);
			goto Quit;
		}

		gui->SetStateInt(key, n);
		gui->StateChanged(gameLocal.time);
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d\r", handle);
	}

Quit:
	return;
}

void idEntity::Event_SetGuiInt(int handle, const char *key, int n)
{
	SetGuiInt(handle, key, n);
}

int idEntity::GetGuiInt(int handle, const char *key)
{
	int retVal = 0;
	if (m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui(handle);
		if (gui)
		{
			retVal = gui->GetStateInt(key);
		}
		else
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
		}
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d\r", handle);
	}

	return retVal;
}

void idEntity::Event_GetGuiInt(int handle, const char *key)
{
	idThread::ReturnInt(GetGuiInt(handle, key));
}

/*
================
idEntity::Event_SetGuiStringFromKey

This is a kludge. It's hopefully temporary, but probably not. Anyway, it's
used by readables to bypass the 127 char limit on string variables in scripts.
================
*/
void idEntity::Event_SetGuiStringFromKey( int handle, const char *key, idEntity *src, const char *spawnArg )
{
	if(!src)
	{
		gameLocal.Warning( "Unable to get key, since the source entity was NULL." );
		return;
	}

	if(!m_overlays.exists(handle))
	{
		gameLocal.Warning( "Non-existant GUI handle: %d", handle );
		return;
	}

	idUserInterface *gui = m_overlays.getGui( handle );
	if(!gui)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, key);
		return;
	}

	if(!gui->IsUniqued())
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Non-existant GUI handle: %d\r", handle);
		return;
	}

	gui->SetStateString( key, common->Translate( src->spawnArgs.GetString( spawnArg, "" ) ) );
	gui->StateChanged( gameLocal.time );
}

void idEntity::CallGui(int handle, const char *namedEvent)
{
	if(m_overlays.exists(handle))
	{
		idUserInterface *gui = m_overlays.getGui( handle );
		if(gui == NULL)
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Handle points to NULL GUI: %d [%s]\r", handle, namedEvent);
			return;
		}

		if(!gui->IsUniqued())
		{
			DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("GUI is not unique. Handle: %d [%s]\r", handle, namedEvent);
			return;
		}

		gui->HandleNamedEvent( namedEvent );
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("setGui: Non-existant GUI handle: %d [%s]\r", handle, namedEvent);
	}

}

/*
================
idEntity::Event_CallGui

Calls a named event in a GUI. (note that the GUI needs to be unique)
================
*/
void idEntity::Event_CallGui(int handle, const char *namedEvent)
{
	CallGui(handle, namedEvent);
}

/*
================
idEntity::Event_LoadExternalData

Used to load an external xdata declaration into this
object's spawn args. The prefix will be prepended to
the names of all keys in the declaration.
================
*/
void idEntity::Event_LoadExternalData( const char *xdFile, const char* prefix )
{
	const tdmDeclXData *xd = static_cast< const tdmDeclXData* >( declManager->FindType( DECL_XDATA, xdFile, false ) );
	if ( xd != NULL ) {
		const idDict *data = &(xd->m_data);
		const idKeyValue *kv;

		int i = data->GetNumKeyVals();
		while ( i-- ) {
			kv = data->GetKeyVal(i);
			spawnArgs.Set( prefix + kv->GetKey(), kv->GetValue() );
		}

		idThread::ReturnInt( 1 );
	} 
	else
	{
		gameLocal.Warning("Non-existant xdata declaration: %s", xdFile);
		idThread::ReturnInt( 0 );
	}
}

/*
================
idEntity::Event_GetInventory

Returns the entity containing us.
================
*/
void idEntity::Event_GetInventory()
{
/*	CInventoryItem* item = InventoryItem();
	if ( item != NULL ) {
		CInventory* inv = item->Inventory();
		if ( inv != NULL ) {
			idThread::ReturnEntity( inv->m_owner.GetEntity() );
		} else {
			idThread::ReturnEntity( NULL );
		}
	}
*/
}

idThread *idEntity::CallScriptFunctionArgs(const char *fkt, bool ClearStack, int delay, const char *fmt, ...)
{
	idThread *pThread = NULL;
	va_list argptr;

	const function_t *pScriptFkt = scriptObject.GetFunction(fkt);
	if (pScriptFkt == NULL)
	{
		DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Action: %s not found in local space, checking for global namespace.\r", fkt);
		pScriptFkt = gameLocal.program.FindFunction(fkt);
	}

	if (pScriptFkt)
	{
		DM_LOG(LC_MISC, LT_DEBUG)LOGSTRING("Running scriptfunction '%s'\r", fkt);
		pThread = new idThread(pScriptFkt);
		va_start(argptr, fmt);
		pThread->CallFunctionArgsVN(pScriptFkt, ClearStack, fmt, argptr);
		va_end(argptr);
		pThread->DelayedStart(delay);
	}
	else
	{
		DM_LOG(LC_MISC, LT_ERROR)LOGSTRING("Scriptfunction not found! [%s]\r", fkt);
	}

	return pThread;
}

void idEntity::Attach( idEntity *ent, const char *PosName, const char *AttName ) 
{
	idVec3			origin;
	idMat3			axis;
	idAngles		angleOffset, angleSubOffset(0.0f,0.0f,0.0f);
	idVec3			originOffset, originSubOffset(vec3_zero);
	SAttachPosition *pos;

// New position system:
	if( PosName && ((pos = GetAttachPosition(PosName)) != NULL) )
	{
		originOffset = pos->originOffset;
		angleOffset = pos->angleOffset;

		// entity-specific offsets to a given position
		originSubOffset = ent->spawnArgs.GetVector( va("origin_%s", PosName ) );
		angleSubOffset = ent->spawnArgs.GetAngles( va("angles_%s", PosName ) );
	}
// The following is the old system and will be phased out
	else
	{
		//gameLocal.Warning("%s is attaching %s using the deprecated attachment system.", name.c_str(), ent->name.c_str());
		angleOffset = ent->spawnArgs.GetAngles( "angles" );
		originOffset = ent->spawnArgs.GetVector( "origin" );
	}

	origin = GetPhysics()->GetOrigin();
	axis = GetPhysics()->GetAxis();

	idMat3 rotate = angleOffset.ToMat3();
	idMat3 newAxis = rotate * axis;
	ent->SetOrigin( origin + originOffset * axis + originSubOffset * newAxis );
	ent->SetAxis( angleSubOffset.ToMat3() * newAxis );
	ent->Bind( this, true );
	ent->cinematic = cinematic;

	SetCinematicOnTeam(this); // grayman #3156

	CAttachInfo	&attach = m_Attachments.Alloc();
	attach.channel = 0; // overloaded in animated classes
	attach.ent = ent;
	attach.name = AttName;
	idStr positionString = PosName;
	attach.posName = positionString; // grayman #2603

	// Update name->m_Attachment index mapping
	int index = m_Attachments.Num() - 1;
	if( AttName != NULL )
		m_AttNameMap.insert(AttNameMap::value_type(AttName, index));
}

void idEntity::Detach( const char *AttName )
{
	int ind = GetAttachmentIndex( AttName );
	if ( ( ind >= 0 ) && ( ind < m_Attachments.Num() ) )
	{
		DetachInd( ind );
		CheckAfterDetach( m_Attachments[ind].ent.GetEntity() ); // grayman #2624 - check for frobability and whether to extinguish
	}
	else
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("Detach called with invalid attachment name %s\r", AttName);
	}
}

void idEntity::DetachInd( int ind )
{
	idEntity *ent = NULL;

	// DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Detach called for index %d\r", ind);
	if( ind < 0 || ind >= m_Attachments.Num() )
	{
		// TODO: log invalid index error
		goto Quit;
	}

	ent = m_Attachments[ind].ent.GetEntity();

	if( !ent || !m_Attachments[ind].ent.IsValid() )
	{
		// TODO: log bad attachment entity error
		goto Quit;
	}

	// DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("Detach: Ent %s unbound\r", ent->name.c_str());
	ent->Unbind();
	// We don't want to remove it from the list, otherwise mapping of name to index gets screwed up
	m_Attachments[ind].ent = NULL;

	// remove from name->index mapping
	m_AttNameMap.erase( m_Attachments[ind].name.c_str() );

Quit:
	return;
}

// grayman #2624 - check whether dropped attachment should become frobable or should be extinguished
// grayman #3852 - check whether the attachment should be removed from the game

void idEntity::CheckAfterDetach( idEntity *ent )
{
	if ( ent == NULL )
	{
		return;
	}

	// grayman #3852
	bool bDestroy = ent->spawnArgs.GetBool("destroy_on_detach", "0");
	if (bDestroy)
	{
		ent->PostEventMS(&EV_SafeRemove, 0);
		return; // no point in checking the other flags
	}

	bool bSetFrob = ent->spawnArgs.GetBool("drop_set_frobable", "0");
	bool bExtinguish = ent->spawnArgs.GetBool("extinguish_on_drop", "0");

	if (bSetFrob)
	{
		ent->m_bFrobable = true;
	}

	// Check if we should extinguish the attachment, like torches or lanterns
	if ( bExtinguish )
	{
		// Get the delay in milliseconds
		int delay = SEC2MS(ent->spawnArgs.GetInt("extinguish_on_drop_delay", "4"));
		if ( delay < 0 )
		{
			delay = 0;
		}

		// grayman #2624 - add a random amount for variability
		int random = SEC2MS(ent->spawnArgs.GetInt("extinguish_on_drop_delay_random", "0"));
		delay += random * gameLocal.random.RandomFloat();

		// Schedule the extinguish event
		ent->PostEventMS(&EV_ExtinguishLights, delay);
	}
}

void idEntity::ReAttachToCoords
	( const char *AttName, idVec3 offset, idAngles angles  ) 
{
	idEntity		*ent( NULL );
	idVec3			origin;
	idMat3			axis, rotate, newAxis;
	CAttachInfo		*attachment( NULL );

	attachment = GetAttachInfo( AttName );
	if( attachment )
		ent = attachment->ent.GetEntity();

	if( !attachment  || !attachment->ent.IsValid() || !ent )
	{
		// TODO: log bad attachment entity error
		goto Quit;
	}

	axis = GetPhysics()->GetAxis();
	origin = GetPhysics()->GetOrigin();
	rotate = angles.ToMat3();
	newAxis = rotate * axis;

	ent->Unbind(); 

	// greebo: Note that Unbind() will invalidate the entity pointer in the attachment list
	// Hence, re-assign the attachment entity pointer (the index itself is ok)
	attachment->ent = ent;

	ent->SetAxis( newAxis );
	ent->SetOrigin( origin + offset * axis );

	ent->Bind( this, true );
	ent->cinematic = cinematic;

	SetCinematicOnTeam(this); // grayman #3156

	// set the spawnargs for later retrieval as well
	ent->spawnArgs.Set( "joint", "" );
	ent->spawnArgs.SetVector( "origin", offset );
	ent->spawnArgs.SetAngles( "angles", angles );

Quit:
	return;
}

void idEntity::ReAttachToPos ( const char *AttName, const char *PosName  ) 
{
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("idEntity::ReAttachToPos called with attachment name %s, position %s, on entity %s\r", AttName, PosName, name.c_str());

	int ind = GetAttachmentIndex( AttName );
	if (ind == -1 )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("idEntity::ReAttachToPos called with invalid attachment name %s on entity %s\r", AttName, name.c_str());
		return;
	}

	idEntity* ent = GetAttachment( ind );

	if( !ent )
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("idEntity::ReAttachToPos called with invalid attached entity on entity %s\r", AttName, name.c_str());
		return;
	}

	// Hack: Detaching leaves a null entry in the array to preserve indices
	// To leverage existing Attach function, we detach and then re-insert
	// into this place in the array.

	DetachInd( ind );
	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("idEntity::ReAttaching...\r");
	Attach( ent, PosName, AttName );

	int indEnd = m_Attachments.Num();

	// greebo: Decrease the end index by 1 before passing it to operator[]
	indEnd--;

	DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("idEntity::End index is %d\r", indEnd);

	// Copy the attachment info from the end of the list over the previous index
	m_Attachments[ind] = m_Attachments[indEnd];
	
	// remove the newly created end entry (is a duplicate now)
	m_Attachments.RemoveIndex( indEnd );

	// Fix the name mapping to map back to the original index
	m_AttNameMap.erase( AttName );
	m_AttNameMap.insert(AttNameMap::value_type(AttName, ind));
}

void idEntity::ShowAttachment( const char *AttName, const bool bShow )
{
	const int ind = GetAttachmentIndex( AttName );
	if (ind >= 0 )
	{
		ShowAttachmentInd( ind, bShow );
	}
	else
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("ShowAttachment called with invalid attachment name %s on entity %s\r", AttName, name.c_str());
	}
}

void idEntity::ShowAttachmentInd( const int ind, const bool bShow )
{
	if( ind < 0 || ind >= m_Attachments.Num() )
	{
		// TODO: log invalid index error
		return;
	}

	idEntity *ent = m_Attachments[ind].ent.GetEntity();

	if( !ent || !m_Attachments[ind].ent.IsValid() )
	{
		// TODO: log bad attachment entity error
		return;
	}

	if ( bShow )
	{
		ent->Show();
	}
	else
	{
		ent->Hide();
	}

	return;
}

int idEntity::GetAttachmentIndex( const char *AttName )
{
	AttNameMap::iterator k;
	k = m_AttNameMap.find(AttName);
	if (k != m_AttNameMap.end() && k->second < m_Attachments.Num() )
	{
		return k->second;
	}
	else
	{
		DM_LOG(LC_AI,LT_WARNING)LOGSTRING("Attempt to access invalid attachment name %s on entity %s\r", AttName, name.c_str());
		return -1;
	}
}

idEntity *idEntity::GetAttachment( const char *AttName )
{
	/* tels: use GetAttachmentIndex() to bypass GetAttachInfo() */
	const int ind = GetAttachmentIndex(AttName);
	if( ind >= 0 )
		return m_Attachments[ind].ent.GetEntity();
	else
		return NULL;

	/*CAttachInfo *AttInfo = GetAttachInfo( AttName );

	if( AttInfo )
		return AttInfo->ent.GetEntity();
	else
		return NULL;*/
}

idEntity *idEntity::GetAttachment( const int ind )
{
	if( ind < 0 || ind >= m_Attachments.Num() )
	{
		// TODO: log invalid index error
		return NULL;
	}

	idEntity *ent = m_Attachments[ind].ent.GetEntity();

	if( !ent || !m_Attachments[ind].ent.IsValid() )
	{
		// TODO: log bad attachment entity error
		return NULL;
	}

	return ent;
}

CAttachInfo *idEntity::GetAttachInfo( const char *AttName )
{
	const int ind = GetAttachmentIndex(AttName);
	if( ind >= 0 )
		return &m_Attachments[ind];
	else
		return NULL;
}

// Tels: Get the entity attached at the position given by the attachment position
// grayman #2603 - resurrected and spruced up for relight work
idEntity* idEntity::GetAttachmentByPosition( const idStr& PosName )
{
	const int num = m_Attachments.Num();

	for (int i = 0 ; i < num ; i++)
	{
		if ((m_Attachments[i].ent.GetEntity() != NULL) && m_Attachments[i].ent.IsValid())
		{
			if (m_Attachments[i].posName == PosName)
			{
				return m_Attachments[i].ent.GetEntity();
			}
		}
	}

	// not found
	return NULL;
}


idEntity *idEntity::GetAttachmentFromTeam( const char *AttName )
{
	/* tels: Look directly at this entity for the attachment */
	const int ind = GetAttachmentIndex(AttName);
	if( ind >= 0 )
	{
		/* we found it */
		//gameLocal.Printf(" Found entity %s at index %i\n", m_Attachments[ind].ent.GetEntity()->name.c_str(), ind);
		return m_Attachments[ind].ent.GetEntity();
	}

	/* tels: not found, look at entities bound to this entity */
	idEntity* NextEnt = this;

	if ( bindMaster )
		{
		NextEnt = bindMaster;	
		}

	while ( NextEnt != NULL )
	{
		//gameLocal.Printf(" Looking at entity %s\n", NextEnt->name.c_str());
		idEntity* AttEnt = NextEnt->GetAttachment( AttName );
		if (AttEnt)
		{
			/* we found it */
			//gameLocal.Printf(" Found entity %s at entity %s.\n", 
			//	AttEnt->name.c_str(), NextEnt->name.c_str());
			return AttEnt;
		}
	/* not found yet, get next Team member */
	NextEnt = NextEnt->GetNextTeamEntity();
	}

	/* not found at all */
	return NULL;
}

void idEntity::BindNotify( idEntity *ent , const char *jointName) // grayman #3074
{
}

void idEntity::UnbindNotify( idEntity *ent )
{
	// greebo: Activate physics on "Unbind" of any slaves
	physics->Activate();
}

void idEntity::Event_TimerCreate(int stimType, int Hour, int Minute, int Seconds, int Millisecond)
{
	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Create Timer: Stimtype %d, Hour: %d, Minute: %d, Seconds: %d, Milliseconds: %d\r",
		stimType, Hour, Minute, Seconds, Millisecond);

	CStimPtr stim = m_StimResponseColl->GetStimByType(static_cast<StimType>(stimType));

	CStimResponseTimer* timer = stim->AddTimerToGame();
	timer->SetTimer(Hour, Minute, Seconds, Millisecond);
}

void idEntity::Event_TimerStop(int stimType)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(static_cast<StimType>(stimType));
	CStimResponseTimer *timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("StopTimer: Stimtype %d\r", stimType);
	if (timer != NULL)
	{
		timer->Stop();
	}
}

void idEntity::Event_TimerStart(int stimType)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(static_cast<StimType>(stimType));
	CStimResponseTimer* timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("StartTimer: Stimtype %d \r", stimType);

	if (timer != NULL)
	{
		timer->Start(static_cast<unsigned int>(sys->GetClockTicks()));
	}
}

void idEntity::Event_TimerRestart(int stimType)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(static_cast<StimType>(stimType));
	CStimResponseTimer* timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("RestartTimer: Stimtype %d \r", stimType);

	if (timer != NULL)
	{
		timer->Restart(static_cast<unsigned int>(sys->GetClockTicks()));
	}
}

void idEntity::Event_TimerReset(int stimType)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(static_cast<StimType>(stimType));
	CStimResponseTimer* timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("ResetTimer: Stimtype %d \r", stimType);

	if (timer != NULL)
	{
		timer->Reset();
	}
}

void idEntity::Event_TimerSetState(int stimType, int state)
{
	CStimPtr stim = m_StimResponseColl->GetStimByType(static_cast<StimType>(stimType));

	CStimResponseTimer* timer = (stim != NULL) ? stim->GetTimer() : NULL;

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("SetTimerState: Stimtype-%d State: %d\r", stimType, state);

	if (timer != NULL)
	{
		timer->SetState(static_cast<CStimResponseTimer::TimerState>(state));
	}
}

void idEntity::Event_SetFrobable(bool bVal)
{
	SetFrobable( bVal );
}

void idEntity::Event_IsFrobable( void )
{
	idThread::ReturnInt( (int) m_bFrobable );
}

void idEntity::Event_Frob()
{
	idPlayer* player = static_cast<idPlayer*>(gameLocal.entities[gameLocal.localClientNum]);
	if (player != NULL)
	{
		// Let the player frob this entity.
		player->PerformFrob(EPressed, this, false);
	}
}

void idEntity::Event_IsHilighted( void )
{
	int retVal = 0;
	if ( m_bFrobHighlightState )
		retVal++;
	if ( m_bFrobbed )
		retVal++;
	idThread::ReturnInt( retVal );
}


void idEntity::Event_FrobHilight( bool bVal )
{
	SetFrobHighlightState( bVal );
}

void idEntity::Event_CheckAbsence()
{
	if (m_AbsenceNoticeability > 0)
	{
		idBounds currentBounds = GetPhysics()->GetAbsBounds();
		float tolerance = spawnArgs.GetFloat("absence_bounds_tolerance", "0");

		currentBounds.ExpandSelf(tolerance);

		idEntity* location(NULL);
		bool isAbsent(true);
		int locationsCount(0);

		for (const idKeyValue* kv = spawnArgs.MatchPrefix("absence_location"); kv != NULL; kv = spawnArgs.MatchPrefix("absence_location", kv))
		{
			locationsCount++;
			location = gameLocal.FindEntity(kv->GetValue());
			if (location != NULL)
			{
				isAbsent = !currentBounds.IntersectsBounds(location->GetPhysics()->GetAbsBounds());
			}
			if (!isAbsent)
			{
				break;
			}
		}
		
		if (locationsCount == 0)
		{
			isAbsent = !currentBounds.IntersectsBounds(m_StartBounds);
		}

		if (isAbsent && !m_AbsenceStatus)
		{
			// was there before, is absent now
			SpawnAbsenceMarker();
		}
		else if (!isAbsent && m_AbsenceStatus)
		{
			// was absent, is back
			DestroyAbsenceMarker();
		}

		// set absence status
		m_AbsenceStatus = isAbsent;

		PostEventMS(&EV_CheckAbsence, 5000);
	}
}

void idEntity::Event_CanBeUsedBy( idEntity *itemEnt )
{
	idThread::ReturnInt( CanBeUsedByEntity( itemEnt, true ) );
}


bool idEntity::SpawnAbsenceMarker()
{
	idStr absenceMarkerDefName = spawnArgs.GetString("def_absence_marker", "atdm:absence_marker");
	const idDict* markerDef = gameLocal.FindEntityDefDict(absenceMarkerDefName, false);

	if (markerDef == NULL)
	{
		gameLocal.Error( "Failed to find definition of absence marker entity " );
		return false;
	}

	idEntity* ent;
	gameLocal.SpawnEntityDef(*markerDef, &ent, false);

	if (!ent || !ent->IsType(CAbsenceMarker::Type)) 
	{
		gameLocal.Error( "Failed to spawn absence marker entity" );
		return false;
	}

	CAbsenceMarker* absenceMarker = static_cast<CAbsenceMarker*>(ent);
	m_AbsenceMarker = absenceMarker;

	// absenceMarker->Init();
	if (!absenceMarker->initAbsenceReference (this, m_StartBounds))
	{
		gameLocal.Error( "Failed to initialize absence reference in absence marker entity" );
		return false;
	}

	// Success
	return true;
}

bool idEntity::DestroyAbsenceMarker()
{
	idEntity* absenceMarker = m_AbsenceMarker.GetEntity();
	if (absenceMarker != NULL)
	{
		absenceMarker->PostEventMS(&EV_Remove, 0);
		m_AbsenceMarker = NULL;
	}
	return true;
}

float idEntity::GetAbsenceNoticeability() const
{
	return m_AbsenceNoticeability;
}

/*
================
idEntity::Event_RangedThreatTo
================
*/
void idEntity::Event_RangedThreatTo(idEntity* target)
{
	// This needs to be virtual, but I don't think events themselves
	// can be virtual; so this just wraps a virtual method.
	float result = this->RangedThreatTo(target);
	idThread::ReturnFloat(result);
}

/*
================
idEntity::RangedThreatTo

Return nonzero if this entity could potentially
attack the given (target) entity at range, or
entities in general if target is NULL.
For example, for the player this should return 1
if the player has a bow equipped and 0 otherwise.
================
*/
float idEntity::RangedThreatTo(idEntity* target)
{
	// Most entities are not capable of attacking at range
	return 0;
}


const bool idEntity::CanBePickedUp()
{
	const bool bIsLoot =
		CInventoryItem::GetLootTypeFromSpawnargs(spawnArgs) != LOOT_NONE
		&& spawnArgs.GetInt("inv_loot_value", "-1") > 0;
	if (bIsLoot)
		return true;

	const bool bIsGeneralInventoryItem =
		strlen(spawnArgs.GetString("inv_name", "")) > 0;
	if (bIsGeneralInventoryItem)
		return true;

	const bool bIsAmmo = spawnArgs.GetInt("inv_ammo_amount", "0") > 0;
	if (bIsAmmo)
		return true;

	const bool bIsWeapon =
		strlen(spawnArgs.GetString("inv_weapon_name")) > 0;
	return bIsWeapon;
}

void idEntity::GetTeamChildren( idList<idEntity *> *list )
{
	// ishtvan: TODO: I think this is WRONG and can go up and across the team hierarchy when called on bind children
	// It only works as advertised when called on bindmasters.
	// grayman - ABSOLUTELY!!! This routine can wrongly tell you that your brothers and sisters are your children.

	idEntity *NextEnt = NULL;
	
	list->Clear();
	for( NextEnt = GetNextTeamEntity(); NextEnt != NULL; NextEnt = NextEnt->GetNextTeamEntity() )
	{
		if( NextEnt != this )
			list->Append( NextEnt );
	}
}

void idEntity::Event_GetBindMaster( void )
{
	idThread::ReturnEntity( GetBindMaster() );
}

void idEntity::Event_NumBindChildren( void )
{
	idList<idEntity *> ChildList;
	GetTeamChildren( &ChildList );

	idThread::ReturnInt( ChildList.Num() );
}

void idEntity::Event_GetBindChild( int ind )
{
	idEntity *pReturnVal( NULL );

	idList<idEntity *> ChildList;
	GetTeamChildren( &ChildList );

	if( (ChildList.Num() - 1) >= ind )
		pReturnVal = ChildList[ ind ];

	idThread::ReturnEntity( pReturnVal );
}

void idEntity::Event_GetNextInvItem()
{
	NextPrevInventoryItem(1);
	
	CInventoryItemPtr item = InventoryCursor()->GetCurrentItem();

	idThread::ReturnEntity( (item != NULL) ? item->GetItemEntity() : NULL );
}

void idEntity::Event_GetPrevInvItem()
{
	NextPrevInventoryItem(-1);
	
	CInventoryItemPtr item = InventoryCursor()->GetCurrentItem();

	idThread::ReturnEntity( (item != NULL) ? item->GetItemEntity() : NULL );
}

void idEntity::Event_SetCurInvCategory(const char* categoryName)
{
	CInventoryItemPtr prev = InventoryCursor()->GetCurrentItem();

	InventoryCursor()->SetCurrentCategory(categoryName);

	OnInventorySelectionChanged(prev);

	CInventoryCategoryPtr category = InventoryCursor()->GetCurrentCategory();
	idThread::ReturnInt( category && category->GetName() == categoryName );
}

void idEntity::Event_SetCurInvItem(const char* itemName)
{
	CInventoryItemPtr prev = InventoryCursor()->GetCurrentItem();

	InventoryCursor()->SetCurrentItem(itemName);

	OnInventorySelectionChanged(prev);

	CInventoryItemPtr item = InventoryCursor()->GetCurrentItem();
	idThread::ReturnInt( item && item->GetName() == itemName );
}

void idEntity::Event_GetCurInvCategory()
{
	CInventoryCategoryPtr category = InventoryCursor()->GetCurrentCategory();
	idThread::ReturnString( category ? category->GetName() : "" );
}

void idEntity::Event_GetCurInvItemEntity()
{
	CInventoryItemPtr item = InventoryCursor()->GetCurrentItem();
	idThread::ReturnEntity( item ? item->GetItemEntity() : NULL );
}

void idEntity::Event_GetCurInvItemName()
{
	CInventoryItemPtr item = InventoryCursor()->GetCurrentItem();
	idThread::ReturnString( item ? item->GetName() : "" );
}

void idEntity::Event_GetCurInvItemId()
{
	CInventoryItemPtr item = InventoryCursor()->GetCurrentItem();
	idThread::ReturnString( item ? item->GetItemId() : "" );
}

void idEntity::Event_GetCurInvIcon()
{
	CInventoryItemPtr item = InventoryCursor()->GetCurrentItem();
	idThread::ReturnString( item ? item->GetIcon() : "" );
}
// Obsttorte #6096
void idEntity::Event_GetCurInvItemCount()
{
	CInventoryItemPtr item = InventoryCursor()->GetCurrentItem();
	if (!item->IsStackable())
	{
		idThread::ReturnFloat(-1.0);
	}
	idThread::ReturnFloat(item->GetCount());
}
void idEntity::Event_InitInventory(int callCount)
{
	// grayman #2820 - At the time of this writing, this routine checks
	// for loot AND belonging in someone's inventory. If anything else is
	// added here, the pre-check in ai.cpp that wraps around "PostEventMS( &EV_InitInventory, 0, 0 )"
	// must account for that. The pre-check is needed to prevent unnecessary
	// event posting that leads to doing nothing here. (I.e. the entity isn't
	// loot, and it doesn't belong in someone's inventory.)

	// greebo: Check if this entity represents loot - if yes, update the total mission loot count

	// grayman #2820 - It probably never happens, but if loot is placed in the player's
	// inventory at map start, it could get double-counted here if the player hasn't
	// spawned yet and this routine has to call itself below. 'callCount' tracks how many
	// times an attempt has been made, so we'll wrap the loot check in a callCount check
	// to make sure it doesn't get done more than once.

	if ( callCount == 0 )
	{
		int lootValue = spawnArgs.GetInt("inv_loot_value", "0");
		int lootType = spawnArgs.GetInt("inv_loot_type", "0");

		// Check if the loot type is valid
		if (lootType > LOOT_NONE && lootType < LOOT_COUNT && lootValue != 0) 
		{
			gameLocal.m_MissionData->AddMissionLoot(static_cast<LootType>(lootType), lootValue);
		}
	}

	// Check if this object should be put into the inventory of some entity
	// when the object spawns. Default is no.
	if (spawnArgs.GetBool("inv_map_start", "0"))
	{
		// Get the name of the target entity, defaults to PLAYER1
		idStr target = spawnArgs.GetString("inv_target", "player1");

		idEntity* targetEnt = gameLocal.FindEntity(target.c_str());
		if (targetEnt != NULL)
		{
			// Put the item into the target entity's inventory
			targetEnt->Inventory()->PutItem(this, targetEnt);
		}
		else 
		{
			// Target entity not found (not spawned yet?) Postpone this event
			// Check how often we've been called to avoid infinite postponing
			if (callCount < 20)
			{
				// Try again in 250 ms.
				PostEventMS(&EV_InitInventory, 250, callCount+1);
			}
		}
	}
}

void idEntity::Event_GetLootAmount(int lootType)
{
	int gold, jewelry, goods;
	int total = Inventory()->GetLoot(gold, jewelry, goods);

	switch (lootType)
	{
		case LOOT_GOLD:
			idThread::ReturnInt(gold);
			return;

		case LOOT_GOODS:
			idThread::ReturnInt(goods);
			return;

		case LOOT_JEWELS:
			idThread::ReturnInt(jewelry);
			return;
	}

	idThread::ReturnInt(total);
}

void idEntity::Event_ChangeLootAmount(int lootType, int amount)
{
	idThread::ReturnInt( ChangeLootAmount(lootType, amount) );
}

void idEntity::Event_AddInvItem(idEntity* ent)
{
	if (ent == NULL || ent->spawnArgs.FindKey("inv_name") == NULL)
	{
		gameLocal.Warning("Cannot add entity %s without 'inv_name' spawnarg to inventory of %s", ent->name.c_str(), name.c_str());
		return;
	}

	AddToInventory(ent);
}

void idEntity::Event_AddItemToInv(idEntity* ent)
{
	// no target, ignore
	if (ent == NULL)
	{
		gameLocal.Warning("Cannot add entity %s to inventory of NULL target", GetName() );
		return;
	}

	if (spawnArgs.FindKey("inv_name") == NULL)
	{
		gameLocal.Warning("Cannot add entity %s without 'inv_name' spawnarg to inventory of %s", GetName(), ent->name.c_str() );
		return;
	}

	gameLocal.Printf("Adding entity %s to inventory of %s.\n", GetName(), ent->name.c_str() );
	ent->AddToInventory(this);
}

bool idEntity::AddAttachmentsToInventory( idPlayer* player )
{
	// NOTE: m_Attachments might not include a key, purse, or other item attached
	// using "bind" or "bindToJoint", so iterate through children instead.

	if (GetBindMaster() != NULL) {
		// Not a BindMaster, so don't iterate through its children.
		// No items can be added to the player's inventory.
		return false;
	}

	bool didAddItem = false;

	idList<idEntity *> children;
	GetTeamChildren(&children);
	for (int i = 0 ; i < children.Num() ; i++) {
		idEntity* child = children[i];
		if (child && player->AddToInventory(child))
			didAddItem = true;
	}

	return didAddItem;
}

CInventoryItemPtr idEntity::AddToInventory(idEntity *ent)
{
	// Sanity check
	if (ent == NULL) return CInventoryItemPtr();

	// Check if we have an inventory item.
	idStr invName;
	if (!ent->spawnArgs.GetString("inv_name", "", invName))
	{
		return CInventoryItemPtr(); // not an inventory item
	}

	// Get (create) the InventoryCursor of this Entity.
	const CInventoryCursorPtr& crsr = InventoryCursor();

	// Add the new item to the Inventory (with <this> as owner)
	CInventoryItemPtr item = crsr->Inventory()->PutItem(ent, this);
	
	if (item == NULL)
	{
		// not an inventory item
		gameLocal.Warning("Couldn't add entity %s to inventory of %s", ent->name.c_str(), name.c_str());
		return item;
	}

	// Play the (optional) acquire sound
	idStr soundName = ent->spawnArgs.GetString("snd_acquire", "");
	if (! soundName.IsEmpty())
	{
		StartSoundShader(declManager->FindSound(soundName), SCHANNEL_ANY, 0, false, NULL);
	}

	return item;
}

bool idEntity::ReplaceInventoryItem(idEntity* oldItem, idEntity* newItem)
{
	bool result = Inventory()->ReplaceItem(oldItem, newItem);

	// Fire the general "item changed" event
	OnInventoryItemChanged();

	return result;
}

void idEntity::Event_ReplaceInvItem(idEntity* oldItem, idEntity* newItem)
{
	idThread::ReturnInt(ReplaceInventoryItem(oldItem, newItem) ? 1 : 0);
}

void idEntity::Event_HasInvItem(idEntity* item)
{
	idThread::ReturnInt( Inventory()->HasItem(item) ? 1 : 0 );
}

int idEntity::ChangeLootAmount(int lootType, int amount)
{
	int groupTotal = 0;
	int rc = 0;
	idStr groupname;

	int gold, jewelry, goods;
	int total = Inventory()->GetLoot(gold, jewelry, goods);
	bool gained = (amount >= 0);
    bool validLootType = true; // SteveL #3719. gnartsch's patch.

	switch(lootType)
	{
		case LOOT_GOLD:
			gold += amount;
			groupTotal = gold;
			groupname = "loot_gold";
			rc = gold;
		break;

		case LOOT_GOODS:
			goods += amount;
			groupTotal = goods;
			groupname = "loot_goods";
			rc = goods;
		break;

		case LOOT_JEWELS:
			jewelry += amount;
			groupTotal = jewelry;
			groupname = "loot_jewels";
			rc = jewelry;
		break;

		default:
			rc = 0;
            validLootType = false;
		break;
	}

	// Set the new values
	Inventory()->SetLoot(gold, jewelry, goods);

    if (validLootType == true) // #3719
	{
		gameLocal.m_MissionData->InventoryCallback(NULL, groupname, groupTotal, total, gained);  
		gameLocal.m_MissionData->ChangeFoundLoot(static_cast<LootType>(lootType), amount);
	}

	return rc;
}

void idEntity::ChangeInventoryLightgemModifier(const char* invName, const char* invCategory, int value)
{
	CInventoryItemPtr item = Inventory()->GetItem(invName, invCategory);
	if (item != NULL) 
	{
		// Item found, set the value
		item->SetLightgemModifier(value);
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Could not change lightgem modifier, item %s not found\r", invName);
	}
}

void idEntity::ChangeInventoryIcon(const char* invName, const char* invCategory, const char* icon)
{
	CInventoryItemPtr item = Inventory()->GetItem(invName, invCategory); 

	if (item != NULL) 
	{
		// Item found, set the icon
		item->SetIcon(icon);
	}
	else
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Could not change inventory icon, item %s not found\r", invName);
	}
}

void idEntity::NextPrevInventoryItem(const int direction)
{
	const CInventoryCursorPtr& cursor = InventoryCursor();
	assert(cursor != NULL); // all entities have a cursor after calling InventoryCursor()

	CInventoryItemPtr prev = cursor->GetCurrentItem();
	if (prev != NULL && prev->GetName() == TDM_DUMMY_ITEM && IsType(idPlayer::Type))
	{
		// If current item is dummy then try to restore cursor to the last real item
		CInventoryItemPtr lastItemRestored = Inventory()->GetItem(static_cast<idPlayer*>(this)->m_LastItemNameBeforeClear);
		if (lastItemRestored != NULL)
		{
			cursor->SetCurrentItem(lastItemRestored);
		}
		static_cast<idPlayer*>(this)->m_LastItemNameBeforeClear = TDM_DUMMY_ITEM;
	}
	// If the current item has not changed yet, move to the prev/next item
	if (cursor->GetCurrentItem() == prev)
	{
		// direction parameter specifies whether next of previous item is chosen
		if (direction > 0) cursor->GetNextItem();
		if (direction < 0) cursor->GetPrevItem();
	}

	// Call the selection changed event
	OnInventorySelectionChanged(prev);
}

void idEntity::NextPrevInventoryGroup(const int direction)
{
	const CInventoryCursorPtr& cursor = InventoryCursor();
	
	assert(cursor != NULL); // all entities have a cursor after calling InventoryCursor()

	CInventoryItemPtr prev = cursor->GetCurrentItem();

	// direction parameter specifies whether next of previous item is chosen
	if (direction > 0) cursor->GetNextCategory();
	if (direction < 0) cursor->GetPrevCategory();
	
	OnInventorySelectionChanged(prev);
}

void idEntity::CycleInventoryGroup(const idStr& groupName)
{
	const CInventoryCursorPtr& cursor = InventoryCursor();
	
	assert(cursor != NULL); // all entities have a cursor after calling InventoryCursor()

	if ( !cursor->GetCurrentCategory() ) {
		gameLocal.Warning("idEntity::CycleInventoryGroup: no category in current cursor");
		return;
	}

	CInventoryItemPtr prev = cursor->GetCurrentItem();

	if (cursor->GetCurrentCategory()->GetName() == groupName)
	{
		// We are already in the specified group
		CInventoryItemPtr next = cursor->GetNextItem();

		assert(next);
		if (next && next->Category()->GetName() != groupName)
		{
			// We've changed groups, this means that the cursor has wrapAround set to false
			// Set the cursor back to the first item
			cursor->SetCurrentCategory(groupName);
		}
	}
	else
	{
		// Not in the desired group yet, set the cursor to the first item in that group
		cursor->SetCurrentCategory(groupName);

		// If the category was empty, then there is no current item now
		// If we leave it like this, GUI will show the item, which is no longer current
		// Better switch to dummy item (blank)
		if (!cursor->GetCurrentItem()) {
			cursor->SetCurrentItem(TDM_DUMMY_ITEM);
		}
	}

	OnInventorySelectionChanged(prev);
}

void idEntity::OnInventoryItemChanged()
{
	// Nothing here, can be overriden by subclasses
}

void idEntity::OnInventorySelectionChanged(const CInventoryItemPtr& prevItem)
{
	// Nothing here, can be overriden by subclasses
}

void idEntity::ChangeInventoryItemCount(const char* invName, const char* invCategory, int amount) 
{
	const CInventoryPtr& inventory = Inventory();

	CInventoryCategoryPtr category = inventory->GetCategory(invCategory);
	if (category == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Could not change item count, inventory category %s not found\r", invCategory);
		return;
	}

	CInventoryItemPtr item = category->GetItem(invName);
	if (item == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Could not change item count, item name %s not found\r", invName);
		return;
	}

	// Change the counter by amount (explicitly allow negative values)
	item->SetCount(item->GetCount() + amount);

	// We consider items "dropped" if the amount is negative
	bool dropped = (amount < 0);

	// Assume item does not have an overall value since it's not loot
	gameLocal.m_MissionData->InventoryCallback
	( 
		item->GetItemEntity(), 
		item->GetName(), 
		item->GetValue(), 
		1, 
		!dropped 
	);

	if (item->GetCount() <= 0) 
	{
		// Stackable item count reached zero, remove item from category
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Removing empty item from category.\r");
		
		category->RemoveItem(item);

		// Advance the cursor (after removal, otherwise we stick to an invalid id)
		InventoryCursor()->GetNextItem();

		// Call the selection changed event, passing the removed item as previously selected item
		OnInventorySelectionChanged(item);
	}
	
	// Check for empty categories after the item has been removed
	if (category->IsEmpty()) 
	{
		// Remove empty category from inventory
		DM_LOG(LC_INVENTORY, LT_DEBUG)LOGSTRING("Removing empty inventory category.\r");
		
		inventory->RemoveCategory(category);
		
		// Switch the cursor to the next category (after removal)
		InventoryCursor()->GetNextCategory();

		// There shouldn't be a need to call OnInventorySelectionChanged(), as this
		// has already been done by the code above.
	}
}

void idEntity::AddFrobPeer(const idStr& frobPeerName)
{
	m_FrobPeers.AddUnique(frobPeerName);
}

void idEntity::AddFrobPeer(idEntity* peer)
{
	if (peer != NULL)
	{
		AddFrobPeer(peer->name);
	}
}

void idEntity::RemoveFrobPeer(const idStr& frobPeerName)
{
	m_FrobPeers.Remove(frobPeerName);
}

void idEntity::RemoveFrobPeer(idEntity* peer)
{
	if (peer != NULL)
	{
		RemoveFrobPeer(peer->name);
	}
}

idEntity* idEntity::GetFrobMaster()
{
	if (m_MasterFrob.IsEmpty()) return NULL;

	idEntity* master = gameLocal.FindEntity(m_MasterFrob);
	
	if (master == NULL)
	{
		// master doesn't exist anymore
		m_MasterFrob.Clear();
		return NULL;
	}

	return master;
}
// Obsttorte: #5976
void idEntity::Event_AddFrobPeer(idEntity* peer)
{
	AddFrobPeer(peer);
	if (m_bFrobbed) // update the frob state if necessary
	{
		peer->SetFrobbed(true);
	}
}
void idEntity::Event_RemoveFrobPeer(idEntity* peer)
{
	RemoveFrobPeer(peer);
	// don't stay frob-hilighted after peer has been removed
	peer->SetFrobbed(false);
}
void idEntity::Event_SetFrobMaster(idEntity* master)
{
	if( master != NULL )
	{
		m_MasterFrob = master->name;
	}
	else
	{
		m_MasterFrob = "";
	}
}
void idEntity::Event_DestroyOverlay(int handle)
{
	DestroyOverlay(handle);
}

void idEntity::DestroyOverlay(int handle)
{
	if ( handle != OVERLAYS_MIN_HANDLE ) {
		idUserInterface *gui = m_overlays.getGui( handle );
		if ( gui )
			gui->Activate( false, gameLocal.time );
		m_overlays.destroyOverlay( handle );
	} else {
		gameLocal.Warning( "Cannot destroy HUD." );
	}
}

void idEntity::Event_CreateOverlay( const char *guiFile, int layer )
{
	idThread::ReturnInt(CreateOverlay(guiFile, layer));
}

int idEntity::CreateOverlay(const char *guiFile, int layer)
{
	int handle = OVERLAYS_INVALID_HANDLE;

	if(guiFile == NULL || guiFile[0] == 0)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Invalid GUI file name\r");
		return OVERLAYS_INVALID_HANDLE;
	}


	if(!uiManager->CheckGui(guiFile))
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Unable to load GUI file: [%s]\r", guiFile);
		return OVERLAYS_INVALID_HANDLE;
	}
	handle = m_overlays.createOverlay( layer );
	if(handle == OVERLAYS_INVALID_HANDLE)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Unable to create overlay for GUI [%s]\r", guiFile);
		return OVERLAYS_INVALID_HANDLE;
	}

	m_overlays.setGui(handle, guiFile);
	idUserInterface *gui = m_overlays.getGui(handle);
	if(gui == NULL)
	{
		DM_LOG(LC_INVENTORY, LT_ERROR)LOGSTRING("Unable to load GUI [%s] into overlay.\r", guiFile);
		// free handle, or we would leak
		m_overlays.destroyOverlay(handle);
		return OVERLAYS_INVALID_HANDLE;
	}

	gui->SetStateInt("handle", handle);
	gui->Activate(true, gameLocal.time);
	// Let's set a good default value for whether or not the overlay is interactive.
	m_overlays.setInteractive(handle, gui->IsInteractive());

	// scale the GUI according to gui_Width/gui_Height/gui_CenterX/gui_CenterY
	gameLocal.UpdateGUIScaling( gui );

	return handle;
}

idUserInterface* idEntity::GetOverlay(int handle)
{
	return m_overlays.getGui(handle);
}

/**
* If this entity (or any entity that it is attached to) has mantling disabled,
* then this returns false. Otherwise, returns true.
**/
bool idEntity::IsMantleable() const
{
	// the entity itself
	if (!m_bIsMantleable)
	{
		return false;
	}
	// else continue with the bind master (there is only one bind master per team)
	if (bindMaster && (! bindMaster->m_bIsMantleable))
	{
		return false;
	}
	return true;
}

int idEntity::heal(const char* healDefName, float healScale) {
	const idDict* healDef = gameLocal.FindEntityDefDict( healDefName, true ); // grayman #3391 - don't create a default 'healDef'
																				// We want 'false' here, but FindEntityDefDict()
																				// will print its own warning, so let's not
																				// clutter the console with a redundant message
	if ( !healDef )
	{
		gameLocal.Error( "Unknown healDef '%s'\n", healDefName );
	}

	int	healAmount = static_cast<int>(healDef->GetInt( "heal_amount" ) * healScale);
	int healInterval = healDef->GetInt("heal_interval", "0");
	int healStepAmount = healDef->GetInt("heal_step_amount", "5");
	float healIntervalFactor = healDef->GetInt("heal_interval_factor", "1");
	bool isAir = idStr(healDef->GetString("heal_type", "")) == "air";

	// Check if the entity can be healed in the first place
	if ( healAmount == 0 || (!isAir && health >= spawnArgs.GetInt("health")) ) {
		return 0;
	}
	
	// Check for air potions
	if (isAir) {
		if (!IsType(idActor::Type)) { 
			// Don't apply air healing to non-actors
			return 0;
		}
		
		// Try to cast this entity onto idAI or idPlayer
		idAI* ai = dynamic_cast<idAI*>(this);
		idPlayer* player = dynamic_cast<idPlayer*>(this);

		if (ai != NULL) {
			// This entity is an AI
			ai->setAirTicks(ai->getAirTicks() + healAmount);
			return 1;
		}
		else if (player != NULL) {
			// This entity is a player
			player->setAirTicks(player->getAirTicks() + healAmount);
			return 1;
		}

		return 0;
	}
	// Is this "instant" healing?
	else if (healInterval == 0) {
		// Yes, heal the entity
		health += healAmount;

		// It may be possible that negative healAmounts are applied and the entity dies
		if ( health <= 0 ) {
			if ( health < -999 ) {
				health = -999;
			}

			Killed( gameLocal.world, gameLocal.world, abs(healAmount), GetLocalCoordinates(GetPhysics()->GetOrigin()), 0);
		}
		return 1;
	}
	// Is this a gradual healing def? This would only apply to idPlayer
	else if (healInterval > 0 && healStepAmount != 0 && IsType(idPlayer::Type))
	{
		idPlayer* player = dynamic_cast<idPlayer*>(this);
		
		if (player != NULL) {
			player->GiveHealthPool(healAmount);
			// Set the parameters of the health pool
			player->setHealthPoolTimeInterval(healInterval, healIntervalFactor, healStepAmount);
		}
		return 1;
	}
	else {
		// Nothing suitable found in the def arguments, return 0
		return 0;
	}
}

// Deals damage to this entity, documentation: see header
void idEntity::Event_Damage( idEntity *inflictor, idEntity *attacker, 
								 const idVec3 &dir, const char *damageDefName, 
								 const float damageScale )
{
	// Pass the call to the regular member method of this class
	Damage(inflictor, attacker, dir, damageDefName, damageScale, 0, NULL);
}

void idEntity::Event_Heal( const char *healDefName, const float healScale )
{
	// Pass the call to the idEntity::heal method
	idThread::ReturnInt(heal(healDefName, healScale));
}

// tels:
void idEntity::Event_TeleportTo(idEntity* target)
{
	Teleport( vec3_origin, idAngles( 0.0f, 0.0f, 0.0f ), target );
}

void idEntity::Event_IsDroppable( void )
{
	idThread::ReturnInt( spawnArgs.GetBool("inv_droppable") );
}

void idEntity::Event_SetDroppable( bool Droppable )
{
	spawnArgs.SetBool( "inv_droppable", Droppable );
	// update the item in the player's inventory, if there is one
	if( spawnArgs.FindKey("inv_name") != NULL )
	{
		idStr itemName = spawnArgs.GetString("inv_name");
		CInventoryItemPtr item = gameLocal.GetLocalPlayer()->Inventory()->GetItem( itemName );
		if( item != NULL )
			item->SetDroppable( Droppable );
	}
}

// tels:
// lightFalloff == 0   - none
// lightFalloff == 0.5 - sqrt(linear)
// lightFalloff == 1   - liniear with distance (falls off already too fast)
// lightFalloff == 2   - squared with distance
// lightDistScale is a scaling factor to divide the distance. Sensible values are around 1..2
void idEntity::Event_GetLightInPVS( const float lightFalloff, const float lightDistScale )
{
	idVec3 sum(0,0,0);
	idVec3 local_light;
	idVec3 local_light_radius;
	idVec3 origin;
	// divide dist by this factor, so 1 as light in 500 units stays, is half in 1000 etc
	float dist_scaling = 5000 * lightDistScale;

	// If this is a player, use the eye location to match what the script does
	// with $player1.GetLocation()
	const idPlayer* player = gameLocal.GetLocalPlayer();
	
	if (player == this)
	{
		origin = player->GetEyePosition();
	}
	else
	{
		origin = GetPhysics()->GetOrigin();
	}
	int areaNum = gameRenderWorld->GetAreaAtPoint( origin );

	// Find all light entities, then check if they are in the same area as the player:
	for( idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		if ( !ent || !ent->IsType( idLight::Type ) ) {
			continue;
		}

		idLight* light = static_cast<idLight*>( ent );

		// this makes it add all lights in the PVS, which is not good, albeit it makes leaning a bit better
		//if ( gameLocal.InPlayerPVS( light ) ) {

		// light is in the same area?
		idVec3 light_origin = light->GetLightOrigin();
		if ( areaNum == gameRenderWorld->GetAreaAtPoint( light_origin ) ) {
			light->GetColor( local_light );
			// multiply the light color by the radius to get a fake "light energy":
			light->GetRadius( local_light_radius );
			// fireplace: 180+180+120/3 => 160 / 800 => 0.2
			// candle:    130+130+120/3 => 123 / 800 => 0.15
			// firearrow:  10+ 10+ 10/3 =>  10 / 800 => 0.0125
			//gameLocal.Printf (" Adding light %s color %s radius %s\n", light->GetName(), local_light.ToString(),  local_light_radius.ToString() );
			local_light *= (float)
				(local_light_radius.x + 
				 local_light_radius.y + 
				 local_light_radius.z) / 2400;
			// diminish light with distance?
			if (lightFalloff > 0)
			{
				// compute distance
				idVec3 dist3 = light_origin - origin;
				// == 1
				float dist;
				if (lightFalloff < 1)
				{
					dist = idMath::Sqrt( dist3.Length() );	// square root of dist
				}
				else if (lightFalloff > 1)
					{
						// squared
						dist = dist3.LengthSqr();
					}
					else
					{
						// linear
						dist = dist3.Length();
					}
				// divide by dist
				//gameLocal.Warning(" dist %0.2f, falloff %0.0f", dist, lightFalloff );
				dist /= dist_scaling;
				//gameLocal.Warning(" dist is now %0.2f", dist );
				//gameLocal.Warning(" local_light %s", local_light.ToString() );
				local_light /= dist;
				//gameLocal.Warning(" local_light %s", local_light.ToString() );
			}
			sum += local_light;
		}
    }
	idThread::ReturnVector( sum );
}

bool idEntity::canSeeEntity(idEntity* target, const int useLighting)
{
	// The target point is the origin of the other entity.
	idVec3 toPos = target->GetPhysics()->GetOrigin();

	trace_t	tr;
	// Perform a trace from the own origin to the target entity's origin
	gameLocal.clip.TracePoint( tr, GetPhysics()->GetOrigin(), toPos, MASK_OPAQUE, this );

	if ( ( tr.fraction >= 1.0f ) || ( gameLocal.GetTraceEntity(tr) == target ) )
	{
		// Trace test passed, entity is visible

		if (useLighting)
		{
			float lightQuotient = target->GetLightQuotient();

			// Return TRUE if the lighting exceeds the threshold.
			return (lightQuotient >= VISIBILTIY_LIGHTING_THRESHOLD);
		}

		// No lighting to consider, return true
		return true;
	}

	return false;
}

void idEntity::Event_CanSeeEntity(idEntity* target, int useLighting)
{
	idThread::ReturnInt(canSeeEntity(target, useLighting) ? 1 : 0);
}

void idEntity::ProcCollisionStims( idEntity *other, int body )
{
	if (!other)
	{
		return;
	}

	if (other->IsType(idAFEntity_Base::Type) && body >= 0)
	{
		idAFEntity_Base* otherAF = static_cast<idAFEntity_Base*>(other);
		int bodID = otherAF->BodyForClipModelId( body );

		//DM_LOG(LC_AI,LT_DEBUG)LOGSTRING("ProcCollisionStims called GetBody on id %d\r", bodID);

		idAFBody* struckBody = otherAF->GetAFPhysics()->GetBody(bodID);

		idEntity* reroute = struckBody->GetRerouteEnt();

		if (reroute)
		{
			other = reroute;
		}
	}

	assert(other->GetStimResponseCollection() != NULL); // always non-NULL by design

	if (other->GetStimResponseCollection()->HasResponse())
	{
		CStimResponseCollection* thisColl = GetStimResponseCollection();

		// check each stim to see if it's a collision stim
		int numStims = thisColl->GetNumStims();

		for (int i = 0; i < numStims; ++i)
		{
			const CStimPtr& stim = thisColl->GetStim(i);
			
			if (stim->m_bCollisionBased)
			{
				stim->m_bCollisionFired = true;
				stim->m_CollisionEnts.Append(other);
			}
		}
	}
}

/* tels: Parses "def_attach" spawnargs and builds a list of idDicts that
 * contain the spawnargs to spawn the attachments, which is done in
 * ParseAttachments() later. This is a two-phase process to allow other
 * code (e.g. SEED) to just parse the attachments without spawning them.
 */
void idEntity::ParseAttachmentSpawnargs( idList<idDict> *argsList, idDict *from )
{
	const idKeyValue *kv = from->MatchPrefix( "def_attach", NULL );
	while ( kv )
	{
		idDict args;

		// Read the classname of the attachment
		
		// Don't process keyvalues equal to "-" (empty attachment).
		if (kv->GetValue() != "-")
		{
			args.Set( "classname", kv->GetValue().c_str() );

			// make items non-touchable so the player can't take them out of the character's hands
			args.Set( "no_touch", "1" );

			// don't let them drop to the floor
			args.Set( "dropToFloor", "0" );

			// check for attachment position spawnarg
			idStr Suffix = kv->GetKey();
			Suffix.StripLeading( "def_attach" );
			idStr PosKey = "pos_attach" + Suffix;
			// String name of the attachment for later accessing
			idStr AttName = "name_attach" + Suffix;
			idStr AttNameValue = from->GetString(AttName);
			if (! AttNameValue)
			{
				// fall back to the position if no name was defined
				AttNameValue = from->GetString(PosKey);
			}

			// store for later query
			args.Set("_poskey", PosKey);
			args.Set("_attnamevalue", AttNameValue );

			// tels: parse all "set .." spawnargs
			const idKeyValue *kv_set = from->MatchPrefix( "set ", NULL );
			while ( kv_set )
			{
				// "set FOO on BAR" "0.5 0.5 0"
				// means set "_color" "0.5 0.5 0" on the entity attached to the attachment point
				// named "BAR" (defined with "name_attach" "BAR" on the original entity)

				// Get the value
				// example: "0.5 0.5 0"
				idStr SpawnargValue(kv_set->GetValue());

				// "set FOO on BAR"
				idStr SetAttName(kv_set->GetKey());
				// "set FOO on BAR" => "FOO on BAR"
				SetAttName = SetAttName.Right( kv_set->GetKey().Length() - 4 );

				// "FOO on BAR"
				idStr SpawnargName(SetAttName);

				// find position of first ' '	
				int PosSpace = SetAttName.Find( ' ', 0, -1);

				if (PosSpace == -1)
				{
					gameLocal.Warning(
						"%s of class %s: Spawnarg '%s' (value '%s') w/o attachment name. Applying to to all attachments.",
						from->GetString("name", "[unknown]"), from->GetString("classname", "[unknown]"),
						kv_set->GetKey().c_str(), kv_set->GetValue().c_str()
					);
					//kv_set = from->MatchPrefix( "set ", kv_set );
					//continue;		
					// pretend "set _color" "0.1 0.2 0.3" means "set _color on BAR" where BAR is the
					// current attachement. So it applies to all of them.
					// SpawnargName is already right
					SetAttName = AttNameValue;
				}
				else
				{
					// "FOO on BAR" => "FOO"
					SpawnargName = SpawnargName.Left( PosSpace );
					// "FOO on BAR" => "BAR"
					SetAttName = SetAttName.Right( SetAttName.Length() - (PosSpace + 4) );
				}

				// gameLocal.Printf("SetAttName '%s'\n", SetAttName.c_str());
				// gameLocal.Printf("AttNameValue '%s'\n", AttNameValue.c_str());
				// gameLocal.Printf("SpawnargName '%s'\n", SpawnargName.c_str());

				// does this spawnarg apply to the newly spawned entity?
				if (SetAttName == AttNameValue)
				{
					// it matches, so this spawnarg must be applied directly to this entity
					//gameLocal.Printf("Match: Setting '%s' to '%s'\n", SpawnargName.c_str(), SpawnargValue.c_str() );
					args.Set( SpawnargName, SpawnargValue );
				}
				else
				{
					// pass along the original "set ..." spawnarg, it might apply to an
					// def_attached entity of the newly spawned one
					//gameLocal.Printf("No match: Passing along '%s' ('%s')\n", kv_set->GetKey().c_str(), SpawnargValue.c_str() );
					args.Set( kv_set->GetKey(), SpawnargValue );
				}

				kv_set = from->MatchPrefix( "set ", kv_set );
				// end while ( kv_set )
			}

			argsList->Append (args );
		}

		kv = from->MatchPrefix( "def_attach", kv );
	}
}

/* tels: Parses "def_attach" spawnargs and spawns and attaches all entities 
 * that are mentioned there. Before each entity is spawned, spawnargs of the
 * format "set XYZ on ABC" are parsed and either applied to the newly spawned
 * entity, so it can pass them along to its own def_attachments, or converted
 * to a real spawnarg and applied to the entity before spawn. */
void idEntity::ParseAttachments( void )
{
	idEntity *ent			= NULL;			// each spawned entity
	const idKeyValue *kv	= NULL;
	idList<idDict>			args;
   
	ParseAttachmentSpawnargs( &args, &spawnArgs );

	for (int i = 0; i < args.Num(); i++)
	{
		gameLocal.SpawnEntityDef( args[i], &ent );

		if ( ent != NULL)
		{
			if( args[i].FindKey( "_poskey" ))
			{
				Attach( ent, spawnArgs.GetString( args[i].GetString( "_poskey" ) ), args[i].GetString( "_attnamevalue") );
				//gameLocal.Printf(" Attaching '%s' at pos '%s'\n", AttNameValue.c_str(), spawnArgs.GetString( args[i].GetString("poskey") ) );
			}
			else
			{
				Attach( ent, NULL, args[i].GetString( "_attnamevalue") );
				//gameLocal.Printf(" Attaching '%s' at pos 'Unknown'\n", AttNameValue.c_str() );
			}

			// grayman #2603 - generate a "target" spawnarg for later processing
			// when attaching a relight position.

			if (ent->name.Find("relight_position") >= 0)
			{
				idStr name = ent->name;		// start with full name
				int index = name.Last('_');	// find '_nnn' entity number at the end
				idStr suffix = name.Right(name.Length() - index);	// suffix = '_nnn'
				idStr rpString = "relight_position" + suffix;		// add suffix to "relight_position"
				spawnArgs.Set(rpString,name);	// add new spawnarg "relight_position_nnn" "<relight position name>"
			}
		}
		else
		{
			gameLocal.Error( "Couldn't spawn '%s' to attach to entity '%s'", args[i].GetString("name"), name.c_str() );
		}
	}

	// after we have spawned all our attachments, we can parse the "add_link" spawnargs:
	kv = spawnArgs.MatchPrefix( "add_link", NULL );
	while ( kv )
	{
		// get the "source to target" value
		idStr Link = kv->GetValue();

		// "SOURCE to TARGET"
		// find position of first " to "	
		int PosSpace = Link.Find( ' ', 0, -1);
		if (PosSpace == -1)
			{
			gameLocal.Warning( "Invalid spawnarg add_link '%s' on entity '%s'",
					  kv->GetValue().c_str(), name.c_str() );
			kv = spawnArgs.MatchPrefix( "add_link", kv );
			continue;		
			}

		// "SOURCE to TARGET" => "SOURCE"
		idStr Source = Link.Left( PosSpace );
		// "SOURCE to TARGET" => "TARGET"
		idStr Target = Link.Right( Link.Length() - (PosSpace + 4) );

		// gameLocal.Printf("Match: add_link '%s' to '%s'\n", Source.c_str(), Target.c_str() );

		// If the source contains ":attached:name", split it into the base entity
		// and the attachment name:
		idEntity *source_ent = this;
		PosSpace = Source.Find( ":attached:", 0, -1);
		if (PosSpace != -1)
		{
			// "test:attached:flame" => "test"
			// ":attached:flame" => ""
			idStr Source_Entity = Source.Left( PosSpace );
			// "test:attached:flame" => "flame"
			idStr Source_Att = Source.Right( Source.Length() - (PosSpace + 10) );

			if (Source_Entity.Length() > 0)
			{
				// "Name:attached:flame" => entity name, so use "Name"
				// gameLocal.Printf(" Link source: '%s'\n", Source_Entity.c_str() );
				
				Source = Source_Entity;
				// Tels: #3048 - support linking from/to the entity we just created when the name is "self:attached:flame"
				if (Source == "self")
					{
					source_ent = this;
					}
					else
					{
					source_ent = gameLocal.FindEntity( Source );
					}
				if (!source_ent)
					{
					gameLocal.Warning( "Cannot find entity '%s'.", Source.c_str() );
					}
			}
			// else: ":attached:flame" => no entity name, so use ourself

			// now find the attached entity
			source_ent = source_ent->GetAttachmentFromTeam( Source_Att.c_str() );
			if (!source_ent)
			{
				gameLocal.Warning( "Cannot find source attachment '%s' on entity '%s'.",
					Source_Att.c_str(), Source.c_str() );
			}
			else
			{
				//gameLocal.Printf(" Link source entity (GetAttachmentFromTeam): '%s'\n", source_ent->name.c_str() );
			}
		}
		else
		{
			// source is just an entity name
			// Tels: #3048 - support linking from/to the entity we just created when the name is "self:attached:flame"
			if (Source == "self")
				{
				source_ent = this;
				}
				else
				{
				source_ent = gameLocal.FindEntity( Source );
				}
			//if (source_ent)
			//		gameLocal.Printf(" Link source entity (Entity): '%s'\n", source_ent->name.c_str() );
		}

		// If the target contains ":attached:name", split it into the base entity
		// and the attachment name:
		idEntity *target_ent = this;
		PosSpace = Target.Find( ":attached:", 0, -1);
		if (PosSpace != -1)
		{
			// "test:attached:flame" => "test"
			idStr Target_Entity = Target.Left( PosSpace );
			// "test:attached:flame" => "flame"
			idStr Target_Att = Target.Right( Target.Length() - (PosSpace + 10) );

			if (Target_Entity.Length() > 0)
			{
				// "Name:attached:flame" => entity name, so use "Name"
				// gameLocal.Printf(" Link target: '%s'\n", Target_Entity.c_str() );
				
				Target = Target_Entity;
				// Tels: #3048 - support linking from/to the entity we just created when the name is "self:attached:flame"
				if (Target == "self")
					{
					target_ent = this;
					}
					else
					{
					target_ent = gameLocal.FindEntity( Target );
					}
				if (!target_ent)
					{
					gameLocal.Warning( "Cannot find entity '%s'.", Target.c_str() );
					}
			}
			// else: ":attached:flame" => no entity name, so use ourself

			// now find the attached entity by the attachment pos name:
			idEntity *target_ent_att = target_ent->GetAttachmentFromTeam( Target_Att.c_str() );
			if (!target_ent_att)
			{
				gameLocal.Warning( "Cannot find target attachment '%s' on entity '%s'.",
					Target_Att.c_str(), target_ent->name.c_str() );
			}
			else
			{
				//gameLocal.Printf(" Link target entity (GetAttachmentFromTeam): '%s'\n", target_ent->name.c_str() );
			}
			target_ent = target_ent_att;
		}
		else
		{
			// target is just an entity name
			// Tels: #3048 - support linking from/to the entity we just created when the name is "self:attached:flame"
			if (Target == "self")
				{
				target_ent = this;
				}
				else
				{
				target_ent = gameLocal.FindEntity( Target );
				}
			//if (target_ent)
			//	gameLocal.Printf(" Link target entity (Entity): '%s'\n", target_ent->name.c_str() );
		}

		// now that we have entities for source and target, add the link
		if ( source_ent && target_ent )
		{
			// gameLocal.Printf(" add_link from '%s' to '%s'\n", source_ent->name.c_str(), target_ent->name.c_str() );
			source_ent->AddTarget( target_ent );
		}
		else
		{
			// tels: TODO: post an event for later adding
			if ( !target_ent )
				gameLocal.Warning( "Cannot find target entity '%s' for add_link.", Target.c_str() );
			if ( !source_ent )
				gameLocal.Warning( "Cannot find source entity '%s' for add_link.", Source.c_str() );
		}

		// do we have another one?
		kv = spawnArgs.MatchPrefix( "add_link", kv );
	}
}

/*
================
idEntity::ParseAttachPositions
================
*/
void idEntity::ParseAttachPositions( void )
{
	SAttachPosition		pos;
	SAttachPosition		*pPos;
	idStr				prefix, keyname, jointname;
	const idKeyValue	*kv(NULL);

	m_AttachPositions.Clear();

	// Read off basic attachment positions 
	// (intended to be on base class for a skeleton type, e.g., humanoid)
	while( ( kv = spawnArgs.MatchPrefix("attach_pos_name_", kv) ) != NULL )
	{
		keyname = kv->GetKey();
		keyname.StripLeading("attach_pos_name_");

		pos.name = kv->GetValue().c_str();
		
		// If entity has joints, find the joint handle for the joint name
		if( GetAnimator() )
		{
			jointname = spawnArgs.GetString( ("attach_pos_joint_" + keyname).c_str() );
			pos.joint = GetAnimator()->GetJointHandle( jointname );
			if ( pos.joint == INVALID_JOINT ) 
			{
				// TODO: Turn this into a warning and attach relative to entity origin instead?
				gameLocal.Error( "Joint '%s' not found for attachment position '%s' on entity '%s'", jointname.c_str(), pos.name.c_str(), name.c_str() );
			}
		}
		else
			pos.joint = INVALID_JOINT;

		pos.originOffset = spawnArgs.GetVector( ("attach_pos_origin_" + keyname).c_str() );
		pos.angleOffset = spawnArgs.GetAngles( ("attach_pos_angles_" + keyname).c_str() );

		m_AttachPositions.Append( pos );
	}

	kv = NULL;
	// Read off actor-specific offsets to this position and apply them
	while( (kv = spawnArgs.MatchPrefix("attach_posmod_name_", kv)) != NULL )
	{
		keyname = kv->GetKey();
		keyname.StripLeading("attach_posmod_name_");

		// Try to find the position
		pPos = GetAttachPosition( kv->GetValue().c_str() );
		// If we find the position by name, apply the offsets
		if( pPos != NULL )
		{
			idVec3 trans = spawnArgs.GetVector( ("attach_posmod_origin_" + keyname).c_str() );
			idAngles ang = spawnArgs.GetAngles( ("attach_posmod_angles_" + keyname).c_str() );

			pPos->originOffset += trans;
			// TODO: Prove mathematically that adding the angles is the same as converting
			// the angles to rotation matrices and multiplying them??
			// stgatilov: It is wrong. As soon as anyone starts using this spawnarg the proper meaning
			// of this line should be worked out.
			gameLocal.Error( "Using spawn attach_posmod_* with uncertain meaning: report to coders.");
			pPos->angleOffset += ang;
		}
	}
}

/*
================
idEntity::GetAttachPosition
================
*/
SAttachPosition *idEntity::GetAttachPosition( const char *AttachName )
{
	idStr AttName = AttachName;
	SAttachPosition *returnVal = NULL;

// TODO: Probably not worth doing a hash for this short list, linear search OK?
	for( int i=0; i < m_AttachPositions.Num(); i++ )
	{
		if( AttName == m_AttachPositions[i].name )
		{
			returnVal = &m_AttachPositions[i];
			break;
		}
	}

	return returnVal;
}

void idEntity::SaveAttachmentContents()
{
	for (int i = 0; i < m_Attachments.Num(); ++i)
	{
		idEntity* ent = m_Attachments[i].ent.GetEntity();

		if (ent == NULL) continue;

		m_Attachments[i].savedContents = (ent != NULL) ? ent->GetPhysics()->GetContents() : -1;
	}
}

// Sets all attachment contents to the given value
void idEntity::SetAttachmentContents(int newContents)
{
	for (int i = 0; i < m_Attachments.Num(); ++i)
	{
		idEntity* ent = m_Attachments[i].ent.GetEntity();

		if (ent == NULL) continue;

		ent->GetPhysics()->SetContents(newContents);
	}
}

// Restores all CONTENTS values, previously saved with SaveAttachmentContents()
void idEntity::RestoreAttachmentContents()
{
	for (int i = 0; i < m_Attachments.Num(); ++i)
	{
		idEntity* ent = m_Attachments[i].ent.GetEntity();

		if (ent == NULL) continue;

		int savedContents = m_Attachments[i].savedContents;

		if (savedContents != -1)
		{
			ent->GetPhysics()->SetContents(savedContents);
		}
	}
}

/*
=====================
SAttachPosition::Save
=====================
*/
void SAttachPosition::Save( idSaveGame *savefile ) const
{
	savefile->WriteString( name );
	savefile->WriteInt( (int) joint );
	savefile->WriteAngles( angleOffset );
	savefile->WriteVec3( originOffset );
}

/*
=====================
SAttachPosition::Restore
=====================
*/
void SAttachPosition::Restore( idRestoreGame *savefile )
{
	int jointInt;
	savefile->ReadString( name );
	savefile->ReadInt( jointInt );
	joint = (jointHandle_t) jointInt;
	savefile->ReadAngles( angleOffset );
	savefile->ReadVec3( originOffset );
}

void idEntity::Event_SetContents(const int contents)
{
	GetPhysics()->SetContents(contents);
}

void idEntity::Event_GetContents()
{
	idThread::ReturnInt(GetPhysics()->GetContents());
}

void idEntity::Event_SetClipMask(const int clipMask)
{
	GetPhysics()->SetClipMask(clipMask);
}

void idEntity::Event_GetClipMask()
{
	idThread::ReturnInt(GetPhysics()->GetClipMask());
}

void idEntity::Event_SetSolid( bool solidity )
{
	SetSolid( solidity );
}

void idEntity::Event_ExtinguishLights()
{
	// grayman #2624 - if this entity is currently being held by the
	// player, don't extinguish it

	CGrabber *grabber = gameLocal.m_Grabber;
	idEntity *heldEntity = grabber->GetSelected();

	if ( heldEntity == this )
	{
		return;
	}

	// grayman #2624 - special case for lanterns: don't extinguish if vertical

	if ( spawnArgs.GetBool("is_lantern","0") )
	{
		const idVec3& gravityNormal = GetPhysics()->GetGravityNormal();
		idMat3 axis = GetPhysics()->GetAxis();
		idVec3 result(0,0,0);
		axis.ProjectVector(-gravityNormal,result);
		float verticality = result * (-gravityNormal);
		if ( verticality > idMath::Cos(DEG2RAD(45)) )
		{
			return; // lantern is w/in 45 degrees of vertical, so don't extinguish it
		}
	}

	// greebo: check if we ourselves are a light
	if ( IsType(idLight::Type) ) 
	{
		// Call the script function to extinguish this light
		CallScriptFunctionArgs("frob_extinguish", true, 0, "e", this);
	}

	// Now go through the teamChain and check for lights
	for ( idEntity* ent = teamChain ; ent != NULL ; ent = ent->teamChain )
	{
		if ( ent->IsType(idLight::Type) )
		{
			ent->CallScriptFunctionArgs("frob_extinguish", true, 0, "e", ent);
		}
	}
}

void idEntity::Event_GetResponseEntity()
{
	idThread::ReturnEntity(GetResponseEntity());
}

void idEntity::Event_GetTeam()
{
	idThread::ReturnInt(team);
}

void idEntity::Event_SetTeam(int newTeam)
{
	//gameLocal.Printf("%s: Changing to team %i.\n", GetName(), newTeam);
	// greebo: No validity checking so far - todo?
	team = newTeam;
}

void idEntity::SetEntityRelation(idEntity* entity, int relation)
{
	m_EntityRelations[entity] = relation;
}

// angua: this changes the current relation to an actor by adding the new amount
void idEntity::ChangeEntityRelation(idEntity* entity, int relationChange)
{
	assert(entity);
	if (entity == NULL)
	{
		return;
	}

	EntityRelationsMap::iterator found = m_EntityRelations.find(entity);

	if (found == m_EntityRelations.end())
	{
		// Sanity-check, some entities like worldspawn have team == -1
		if (entity->team == -1) return;

		// not yet set, load default from relations manager
		int defaultrel = gameLocal.m_RelationsManager->GetRelNum(team, entity->team);

		std::pair<EntityRelationsMap::iterator, bool> result = m_EntityRelations.insert(
			EntityRelationsMap::value_type(entity, defaultrel)
		);

		// set iterator to newly inserted element
		found = result.first;
	}

	found->second += relationChange;
}

bool idEntity::IsFriend(const idEntity *other) const
{
	if (other == NULL)
	{
		return false;
	}
	else if (other->team == -1)
	{
		// entities with team -1 (not set) are neutral
		return false;
	}
	else
	{
		// angua: look up entity specific relation
		EntityRelationsMap::const_iterator found = m_EntityRelations.find(other);
		if (found != m_EntityRelations.end())
		{
			return (found->second > 0);
		}

		// angua: no specific relation found, fall back to standard team relations
		return gameLocal.m_RelationsManager->IsFriend(team, other->team);
	}
}

bool idEntity::IsNeutral(const idEntity *other) const
{
	if (other == NULL)
	{
		return false;
	}

	if (other->team == -1)
	{
		// entities with team -1 (not set) are neutral
		return true;
	}

	// angua: look up entity specific relation
	EntityRelationsMap::const_iterator found = m_EntityRelations.find(other);
	if (found != m_EntityRelations.end())
	{
		return (found->second == 0);
	}

	// angua: no specific relation found, fall back to standard team relations
	return gameLocal.m_RelationsManager->IsNeutral(team, other->team);
}

bool idEntity::IsEnemy(const idEntity *other ) const
{
	if (other == NULL)
	{
		// The NULL pointer is not your enemy! As long as you remember to check for it to avoid crashes.
		return false;
	}

	if (other->team == -1)
	{
		// entities with team -1 (not set) are neutral
		return false;
	}

	if (other->fl.notarget)
	{
		return false;
	}

	// angua: look up entity specific relation
	EntityRelationsMap::const_iterator found = m_EntityRelations.find(other);
	if (found != m_EntityRelations.end())
	{
		return (found->second < 0);
	}

	// angua: no specific relation found, fall back to standard team relations
	return gameLocal.m_RelationsManager->IsEnemy(team, other->team);
}

void idEntity::Event_IsEnemy( idEntity *ent )
{
	idThread::ReturnInt(static_cast<int>(IsEnemy(ent)));
}

void idEntity::Event_IsFriend( idEntity *ent )
{
	idThread::ReturnInt(IsFriend(ent));
}

void idEntity::Event_IsNeutral( idEntity *ent )
{
	idThread::ReturnInt(IsNeutral(ent));
}


void idEntity::Event_SetEntityRelation(idEntity* entity, int relation)
{
	SetEntityRelation(entity,relation);
}

void idEntity::Event_ChangeEntityRelation(idEntity* entity, int relationChange)
{
	ChangeEntityRelation(entity, relationChange);
}

// grayman #2905

void idEntity::Event_IsLight()
{
	idThread::ReturnInt(static_cast<int>(IsType(idLight::Type)));
}

// grayman #3013

void idEntity::Event_GetLocation()
{
	idThread::ReturnEntity( GetLocation() );
}

idLocationEntity *idEntity::GetLocation( void ) const
{
	return gameLocal.LocationForPoint( GetPhysics()->GetOrigin() );
}

// Dragofer

void idEntity::Event_GetEntityFlag( const char* flagName )
{
	if ( idStr::Icmp("notarget", flagName) == 0 )
	{
		idThread::ReturnInt(fl.notarget);
		return;
	}

	if ( idStr::Icmp("noknockback", flagName) == 0 )
	{
		idThread::ReturnInt(fl.noknockback);
		return;
	}

	if ( idStr::Icmp("takedamage", flagName) == 0 )
	{
		idThread::ReturnInt(fl.takedamage);
		return;
	}

	if ( idStr::Icmp("hidden", flagName) == 0 )
	{
		idThread::ReturnInt(fl.hidden);
		return;
	}

	if ( idStr::Icmp("bindOrientated", flagName) == 0 )
	{
		idThread::ReturnInt(fl.bindOrientated);
		return;
	}

	if ( idStr::Icmp("solidForTeam", flagName) == 0 )
	{
		idThread::ReturnInt(fl.solidForTeam);
		return;
	}

	if ( idStr::Icmp("forcePhysicsUpdate", flagName) == 0 )
	{
		idThread::ReturnInt(fl.forcePhysicsUpdate);
		return;
	}

	if ( idStr::Icmp("selected", flagName) == 0 )
	{
		idThread::ReturnInt(fl.selected);
		return;
	}

	if ( idStr::Icmp("neverDormant", flagName) == 0 )
	{
		idThread::ReturnInt(fl.neverDormant);
		return;
	}

	if ( idStr::Icmp("isDormant", flagName) == 0 )
	{
		idThread::ReturnInt(fl.isDormant);
		return;
	}

	if ( idStr::Icmp("hasAwakened", flagName) == 0 )
	{
		idThread::ReturnInt(fl.hasAwakened);
		return;
	}

	if ( idStr::Icmp("invisible", flagName) == 0 )
	{
		idThread::ReturnInt(fl.invisible);
		return;
	}

	if ( idStr::Icmp("inaudible", flagName) == 0 )
	{
		idThread::ReturnInt(fl.inaudible);
		return;
	}

	gameLocal.Warning("Invalid flag name passed to getEntityFlag(): %s", flagName);
	idThread::ReturnFloat(0.0f);
}

// grayman #3047

bool idEntity::CastsShadows() const
{
	return !renderEntity.noShadow;
}

// grayman #3516 - I collided with something.
// Am I in the grabber, and did I collide with an AI?
// If so, the grabber should drop me.

void idEntity::CheckCollision(idEntity* collidedWith)
{
	if (collidedWith == NULL)
	{
		return; // can't work with a NULL
	}

	if (collidedWith == gameLocal.world)
	{
		return; // hitting the world doesn't cause a drop
	}

	if (this == gameLocal.world)
	{
		return; // world will never be in the grabber
	}

	CGrabber* grabber = gameLocal.m_Grabber;
	idEntity* entHeld = grabber->GetSelected();
	if (entHeld == NULL)
	{
		return; // not holding anything
	}

	bool hitAI = collidedWith->IsType(idAI::Type);

	if (!hitAI)
	{
		// We're interested in the parent of whatever team collidedWith is a part of, if any.

		idEntity *bindMaster = collidedWith->GetBindMaster();
		while ( bindMaster != NULL )
		{
			collidedWith = bindMaster;
			bindMaster = collidedWith->GetBindMaster();
		}
		hitAI = collidedWith->IsType(idAI::Type);
	}

	if (!hitAI)
	{
		return; // only interested if we hit an AI
	}

	idAI *collidedAI = (idAI*)collidedWith;
	if ( collidedAI->AI_DEAD || collidedAI->AI_KNOCKEDOUT )
	{
		return;	// stgatilov: allow to move grabbed object over dead bodies
	}

	// Are we, or someone on our team, being held by the grabber?

	// Simple check first ...
	if (entHeld == this)
	{
		grabber->StopDrag();
		return;
	}

	// Check everyone on my team.

	idEntity *bindMaster = GetBindMaster();
	idEntity *parent = NULL;
	while ( bindMaster != NULL )
	{
		parent = bindMaster;
		bindMaster = parent->GetBindMaster();
	}

	// If we found a parent, is held item on the list of all children?
	if (parent)
	{
		idList<idEntity *> children;
		parent->GetTeamChildren(&children); // gets all children
		for ( int i = 0 ; i < children.Num() ; i++ )
		{
			idEntity *child = children[i];
			if ( child == entHeld )
			{
				grabber->StopDrag();
				return;
			}
		}
	}
}
