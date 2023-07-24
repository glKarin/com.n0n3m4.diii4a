// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "script/Script_Helper.h"
#include "script/Script_ScriptObject.h"

#include "physics/Physics_Parametric.h"
#include "physics/Physics_Actor.h"
#include "physics/Physics_AF.h"
#include "client/ClientEffect.h"
#include "../bse/BSEInterface.h"
#include "../bse/BSE_Envelope.h"
#include "../bse/BSE_SpawnDomains.h"
#include "../bse/BSE_Particle.h"
#include "../bse/BSE.h"
#include "WorldSpawn.h"
#include "Mover.h"
#include "ContentMask.h"
#include "Projectile.h"
#include "structures/TeamManager.h"
#include "guis/UserInterfaceManager.h"
#include "Player.h"
#include "guis/UserInterfaceLocal.h"
#include "vehicles/Transport.h"
#include "../decllib/declTypeHolder.h"
#include "rules/GameRules.h"
#include "../decllib/DeclSurfaceType.h"
#include "guis/GuiSurface.h"
#include "effects/Flares.h"
#include "effects/FootPrints.h"
#include "InputMode.h"
#include "proficiency/StatsTracker.h"
#include "AntiLag.h"

#include "botai/BotThreadData.h"

/*
===============================================================================

	idEntity

===============================================================================
*/

extern const idEventDef EV_GetNumPositions;
extern const idEventDef EV_SetSkin;
extern const idEventDef EV_SyncScriptFieldBroadcast;
extern const idEventDef EV_SyncScriptFieldCallback;
extern const idEventDef EV_GetName;
extern const idEventDef EV_SetColor;
extern const idEventDef EV_GetColor;
extern const idEventDef EV_GetShaderParm;
extern const idEventDef EV_SetShaderParm;
extern const idEventDef EV_SetShaderParms;
extern const idEventDef EV_IsHidden;
extern const idEventDef EV_GetJointHandle;
extern const idEventDef EV_SendNetworkEvent;
extern const idEventDef EV_GetAngles;
extern const idEventDef EV_GetHealth;
extern const idEventDef EV_GetOrigin;
extern const idEventDef EV_DisableKnockback;
extern const idEventDef EV_EnableKnockback;

// overridable events
const idEventDefInternal EV_PostSpawn( "internal_postspawn" );
const idEventDefInternal EV_FindTargets( "internal_findTargets" );
const idEventDef EV_GetName( "getName", 's', DOC_TEXT( "Returns the internal name of the object." ), 0, NULL );
const idEventDef EV_Activate( "activate", '\0', DOC_TEXT( "Toggles the state of the object" ), 1, NULL, "e", DOC_TEXT( "activator" ), DOC_TEXT( "The entity which caused the activation." ) );
const idEventDef EV_Bind( "bind", '\0', DOC_TEXT( "Binds the physics for this entity to the entity specified. They will be bound in their current relative position and orientation." ), 1, NULL, "e", DOC_TEXT( "master" ), DOC_TEXT( "The entity to bind to." ) );
const idEventDef EV_BindPosition( "bindPosition", '\0', DOC_TEXT( "Binds the physics for this entity to the entity specified. They will be bound in their current relative position, but the entity will keep its own independant orientation." ), 1, NULL, "e", DOC_TEXT( "master" ), DOC_TEXT( "The entity to bind to." ) );
const idEventDef EV_BindToJoint( "bindToJoint", '\0', DOC_TEXT( "Binds the physics for this entity to the joint on the entity specified. They will be bound in their current relative position, but the entity will keep its own independant orientation unless the oriented flag is set." ), 3, NULL, "e", DOC_TEXT( "master" ), DOC_TEXT( "The entity to bind to." ), "s", DOC_TEXT( "joint" ), DOC_TEXT( "The name of the joint to bind to." ), "b", DOC_TEXT( "oriented" ), DOC_TEXT( "Whether the bind is position only, or also oriented." ) );
const idEventDef EV_Unbind( "unbind", '\0', DOC_TEXT( "Removes any active bind on the entity." ), 0, NULL );
const idEventDef EV_RemoveBinds( "removeBinds", '\0', DOC_TEXT( "Unbinds all entities bound to this entity." ), 0, NULL );
const idEventDef EV_SetModel( "setModel", '\0', DOC_TEXT( "Changes the render model for this entity." ), 1, NULL, "s", DOC_TEXT( "name" ), DOC_TEXT( "Name of the model, or $decl:modelDef$ for animated models, to set." ) );
const idEventDef EV_SetSkin( "setSkin", '\0', DOC_TEXT( "Changes the $decl:skin$ applied to the render model for this entity." ), 1, DOC_TEXT( "If an empty string is passed in, the skin will be cleared." ), "s", DOC_TEXT( "name" ), DOC_TEXT( "Name of the $decl:skin$ to apply." ) );
const idEventDef EV_SetCoverage( "setCoverage", '\0', DOC_TEXT( "This sets the coverage level on the render model of the entity. This can used to fade an entity out." ), 1, NULL, "f", DOC_TEXT( "value" ), DOC_TEXT( "The level of coverage applied, in the range 0 -> 1." ) );
const idEventDef EV_GetWorldAxis( "getWorldAxis", 'v', DOC_TEXT( "Returns a vector representing the specified physics axis of the object in world space." ), 1, DOC_TEXT( "Values outside of the range 0 -> 2 are likely to return garbage data, or crash." ), "d", DOC_TEXT( "index" ), DOC_TEXT( "Which axis to return, 0 = Forward, 1 = Right, 2 = Up." ) );
const idEventDef EV_SetWorldAxis( "setWorldAxis", '\0', DOC_TEXT( "Sets the physics axes of the entity in world space." ), 3, DOC_TEXT( "The input values are not guarded against non-orthogonal axes, nor them being flipped." ), "v", DOC_TEXT( "forward" ), DOC_TEXT( "The forward component." ), "v", DOC_TEXT( "right" ), DOC_TEXT( "The right component." ), "v", DOC_TEXT( "up" ), DOC_TEXT( "The up component." ) );
const idEventDef EV_GetWorldOrigin( "getWorldOrigin", 'v', DOC_TEXT( "Returns the origin of the entity in world space." ), 0, NULL );
const idEventDef EV_GetGravityNormal( "getGravityNormal", 'v', DOC_TEXT( "Returns the axis in which gravity acts for this entity." ), 0, NULL );
const idEventDef EV_SetWorldOrigin( "setWorldOrigin", '\0', DOC_TEXT( "Sets the physics origin for the entity in world space. This will update the relative offset to the bind master, if present." ), 1, NULL, "v", DOC_TEXT( "origin" ), DOC_TEXT( "The new origin to set." ) );
const idEventDef EV_GetOrigin( "getOrigin", 'v', DOC_TEXT( "This will return the origin of the physics of the object, relative to its bind master, if it has one, otherwise in world space." ), 0, NULL );
const idEventDef EV_SetOrigin( "setOrigin", '\0', DOC_TEXT( "This will set the origin of the entity relative to its bind master, if it has one, otherwise in world space." ), 1, NULL, "v", DOC_TEXT( "origin" ), DOC_TEXT( "New origin to set." ) );
const idEventDef EV_GetAngles( "getAngles", 'v', DOC_TEXT( "Returns the orientation of the object in angles form. This value will be relative to a bind master, if present and bound in orientation locked mode, otherwise in world space." ), 0, NULL );
const idEventDef EV_SetAngles( "setAngles", '\0', DOC_TEXT( "Sets the axes of the physics object from the given angles relative to the bind master if it has one, and is in locked orientation mode, otherwise in world space." ), 1, NULL, "v", DOC_TEXT( "angles" ), DOC_TEXT( "Angles to apply." ) );
const idEventDef EV_SetGravity( "setGravity", '\0', DOC_TEXT( "Sets the magnitude and direction of gravity for this entity." ), 1, DOC_TEXT( "This is not supported for all physics types." ), "v", DOC_TEXT( "gravity" ), DOC_TEXT( "New gravity value to set." ) );
const idEventDef EV_AlignToAxis( "alignToAxis", '\0', DOC_TEXT( "Aligns the specified axis of the physics along the supplied vector." ), 2, DOC_TEXT( "The vector passed in should be normalized.\nNothing will happen if index is out of range." ), "v", DOC_TEXT( "axis" ), DOC_TEXT( "Vector to align the axis to." ), "d", DOC_TEXT( "index" ), DOC_TEXT( "Index of the axis to align, 0 = Forward, 1 = Right, 2 = Up." ) );
const idEventDef EV_GetLinearVelocity( "getLinearVelocity", 'v', DOC_TEXT( "Returns the linear velocity of the entity in world space." ), 0, DOC_TEXT( "This is not supported for all physics types." ) );
const idEventDef EV_SetLinearVelocity( "setLinearVelocity", '\0', DOC_TEXT( "Sets the linear velocity of the entity in world space." ), 1, DOC_TEXT( "This is not supported for all physics types." ), "v", DOC_TEXT( "velocity" ), DOC_TEXT( "Velocity to set." ) );
const idEventDef EV_GetAngularVelocity( "getAngularVelocity", 'v', DOC_TEXT( "Returns the angular velocity of the entity in world space." ), 0, DOC_TEXT( "This is not supported for all physics types." ) );
const idEventDef EV_SetAngularVelocity( "setAngularVelocity", '\0', DOC_TEXT( "Sets the angular velocity of the entity in world space." ), 1, DOC_TEXT( "This is not supported for all physics types." ), "v", DOC_TEXT( "velocity" ), DOC_TEXT( "Velocity to set." ) );
const idEventDef EV_GetMass( "getMass", 'f', DOC_TEXT( "Returns the mass of the entity." ), 0, DOC_TEXT( "This is not supported for all physics types." ) );
const idEventDef EV_GetCenterOfMass( "getCenterOfMass", 'v', DOC_TEXT( "Returns the center of mass of the entity." ), 0, DOC_TEXT( "This is not supported for all physics types." ) );
const idEventDef EV_SetFriction( "setFriction", '\0', DOC_TEXT( "Sets the linear, angular, and contact friction for the entity." ), 3, DOC_TEXT( "This is not supported for all physics types." ), "f", DOC_TEXT( "linear" ), DOC_TEXT( "Linear 'air' friction to apply." ), "f", DOC_TEXT( "angular" ), DOC_TEXT( "Angular 'air' friction to apply." ), "f", DOC_TEXT( "contact" ), DOC_TEXT( "Contact friction to apply." ) );
const idEventDef EV_GetSize( "getSize", 'v', DOC_TEXT( "Returns the size of the physics bounds in entity space." ), 0, NULL );
const idEventDef EV_SetSize( "setSize", '\0', DOC_TEXT( "Sets the bounds of the clip model of the entity with density 1." ), 2, NULL, "v", DOC_TEXT( "mins" ), DOC_TEXT( "Mins of the bounds." ), "v", DOC_TEXT( "maxs" ), DOC_TEXT( "Maxs of the bounds" ) );
const idEventDef EV_GetMins( "getMins", 'v', DOC_TEXT( "Returns the mins of the physics bounds in entity space." ), 0, NULL );
const idEventDef EV_GetMaxs( "getMaxs", 'v', DOC_TEXT( "Returns the maxs of the physics bounds in entity space." ), 0, NULL );
const idEventDef EV_GetAbsMins( "getAbsMins", 'v', DOC_TEXT( "Returns the mins of the physics bounds in world space." ), 0, NULL );
const idEventDef EV_GetAbsMaxs( "getAbsMaxs", 'v', DOC_TEXT( "Returns the maxs of the physics bounds in world space." ), 0, NULL );
const idEventDef EV_GetRenderMins( "getRenderMins", 'v', DOC_TEXT( "Returns the mins of the render model bounds in entity space." ), 0, NULL );
const idEventDef EV_GetRenderMaxs( "getRenderMaxs", 'v', DOC_TEXT( "Returns the maxs of the render model bounds in entity space." ), 0, NULL );
const idEventDef EV_SetRenderBounds( "setRenderBounds", '\0', DOC_TEXT( "Overrides the calculated render model bounds." ), 2, NULL, "v", DOC_TEXT( "mins" ), DOC_TEXT( "Mins of the bounds in entity space." ), "v", DOC_TEXT( "maxs" ), DOC_TEXT( "Maxs of the bounds in entity space." ) );
const idEventDef EV_IsHidden( "isHidden", 'b', DOC_TEXT( "Returns whether the render model is hidden." ), 0, NULL );
const idEventDef EV_Hide( "hide", '\0', DOC_TEXT( "Hides the render model." ), 0, DOC_TEXT( "See also $event:show$." ) );
const idEventDef EV_Show( "show", '\0', DOC_TEXT( "Shows the render model." ), 0, DOC_TEXT( "See also $event:hide$." ) );
const idEventDef EV_Touches( "touches", 'b', DOC_TEXT( "Returns whether any of the trace models from this entity intersect with the clip models of the specified entity." ), 2, DOC_TEXT( "If entity is $null$, the result will be false." ), "E", DOC_TEXT( "entity" ), DOC_TEXT( "Entity to check against." ), "b", DOC_TEXT( "ignoreNonTrace" ), DOC_TEXT( "If not set, an error will be thrown if this entity has no trace models." ) );
const idEventDef EV_TouchesBounds( "touchesBounds", 'b', DOC_TEXT( "Returns whether the bounds of this entity interests the bounds of the specified entity." ), 1, DOC_TEXT( "If entity is $null$, the result will be false." ), "E", DOC_TEXT( "entity" ), DOC_TEXT( "Entity to check against." ) );
const idEventDef EV_GetShaderParm( "getShaderParm", 'f', DOC_TEXT( "Gets the shader parm at the specified index." ), 1, "Index must be in the range 0 -> 11.", "d", DOC_TEXT( "index" ), DOC_TEXT( "Index of the shader parm to return." ) );
const idEventDef EV_SetShaderParm( "setShaderParm", '\0', DOC_TEXT( "Sets the shader parm at the specified index to the value given." ), 2, DOC_TEXT( "Index must be in the range 0 -> 11 for entities, and 0 - 3 for sys calls." ), "d", DOC_TEXT( "index" ), DOC_TEXT( "Index of the shader parm to set." ), "f", DOC_TEXT( "value" ), DOC_TEXT( "Value to set the shader parm to." ) );
const idEventDef EV_SetShaderParms( "setShaderParms", '\0', DOC_TEXT( "Sets the RGBA shader parms to the given values." ), 4, NULL, "f", "red", "Value to set the red shader parm to.", "f", "green", "Value to set the green shader parm to.", "f", "blue", "Value to set the blue shader parm to.", "f", "alpha", "Value to set the alpha shader parm to." );
const idEventDef EV_SetColor( "setColor", '\0', DOC_TEXT( "Sets the RGB shader parms to the give values." ), 3, NULL, "f", "red", "Value to set the red shader parm to.", "f", "green", "Value to set the green shader parm to.", "f", "blue", "Value to set the blue shader parm to." );
const idEventDef EV_GetColor( "getColor", 'v', DOC_TEXT( "Returns the RGB shader parms." ), 0, NULL );
const idEventDef EV_StartSound( "startSound", 'f', DOC_TEXT( "Starts a sound shader on the specified channel, and returns the length of the sound played." ), 2, "The key must begin with 'snd_'.", "s", "key", "Key to look up the $decl:sound$.", "d", DOC_TEXT( "channel" ), DOC_TEXT( "Channel to start the $decl:sound$ on." ) );
const idEventDef EV_StopSound( "stopSound", '\0', DOC_TEXT( "Stops any sounds playing on the specified channel." ), 1, NULL, "d", "channel", "Channel to stop playing sounds on." );
const idEventDef EV_FadeSound( "fadeSound", '\0', DOC_TEXT( "Linearly interpolates the volume on the channel to the new volume over the length of time specified." ), 3, NULL, "d", "channel", "The channel to fade.", "f", "to", "The new volume to fade to.", "f", "over", "Time to fade over in seconds." );
const idEventDef EV_SetChannelPitchShift( "setChannelPitchShift", '\0', DOC_TEXT( "Applies the new pitch shift to any sound playing on the specified channel." ), 2, NULL, "d", "channel", "Channel to set the pitch shift on.", "f", "shift", "Pitch shift to set, in the range 0.5 -> 8." );
const idEventDef EV_SetChannelFlags( "setChannelFlags", '\0', DOC_TEXT( "Sets the given flags on any sound playing on the specified channel." ), 2, NULL, "d", "channel", "Channel to which the flags are to be applied.", "d", "flags", "Flags to apply to the channel." );
const idEventDef EV_ClearChannelFlags( "clearChannelFlags", '\0', DOC_TEXT( "Clears the given flags on any sound playing on the specified channel." ), 2, NULL, "d", "channel", "Channel on which the flags are to be removed.", "d", "flags", "Flags to remove from the channel." );
const idEventDef EV_SetNumCrosshairLines( "chSetNumLines", '\0', DOC_TEXT( "Sets the number of active crosshair info lines." ), 1, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.", "d", "count", "The new line count." );
const idEventDef EV_AddCrosshairLine( "chAddLine", 'd', DOC_TEXT( "Adds a new crosshair info line, and returns the index of the added line." ), 0, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash." );
const idEventDef EV_GetCrosshairDistance( "chGetDistance", 'd', DOC_TEXT( "Returns the distance to the object under the crosshair." ), 0, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.\nIf there is no crosshair object the entire trace length will be returned." );
const idEventDef EV_SetCrosshairLineText( "chSetLineText", '\0', DOC_TEXT( "Sets the text for a specified crosshair info line." ), 2, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.\nThis line should be set to text mode using $event:chSetLineType$.", "d", "index", "Index of the line to set text on.", "w", "text", "Text to set on the line." );
const idEventDef EV_SetCrosshairLineTextIndex( "chSetLineTextIndex", '\0', DOC_TEXT( "Sets the text for a specified crosshair info line." ), 2, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.\nThis line should be set to text mode using $event:chSetLineType$.", "d", "index", "Index of the line to set text on.", "h", "handle", "Handle of $decl:locStr$ to set on this line." );
const idEventDef EV_SetCrosshairLineMaterial( "chSetLineMaterial", '\0', DOC_TEXT( "Sets the material for a specified crosshair info line." ), 2, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.\nThis line should be set to image mode using $event:chSetLineType$.", "d", "index", "Index of the line to set the $decl:material$ on.", "s", "material", "Name of the $decl:material$ to set." );
const idEventDef EV_SetCrosshairLineColor( "chSetLineColor", '\0', DOC_TEXT( "Sets the foreground color for the specified crosshair info line." ), 3, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.\nThis is supported by all drawing modes.", "d", "index", "Index of the line to set the color on.", "v", "color", "RGB components of the color to set.", "f", "alpha", "Alpha component of the color to set." );
const idEventDef EV_SetCrosshairLineSize( "chSetLineSize", '\0', DOC_TEXT( "Sets the width and height of the specified crosshair info line." ), 3, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.\nThis applies to both bar and image mode, but not text mode. Set the line mode using $event:chSetLineType$.", "d", "index", "Index of the line to set the size of.", "f", "width", "New width of the line.", "f", "height", "New height of the line." );
const idEventDef EV_SetCrosshairLineFraction( "chSetLineFraction", '\0', DOC_TEXT( "Sets the current progress fraction of the specified crosshair info line." ), 2, "This is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.\nThis line should be set to bar mode using $event:chSetLineType$.", "d", "index", "Index of the line to set progress on.", "f", "fraction ", "Progress to set, in the range 0 -> 1." );
const idEventDef EV_SetCrosshairLineType( "chSetLineType", '\0', DOC_TEXT( "Sets the type of the specified crosshair info line." ), 2, "Setting an unknown type will cause the line to not be visible.\nThis is only valid within the OnUpdateCrosshairInfo function, if called outside of this, the game will crash.", "d", "index", "Index of the line to set the type of.", "d", "type", "Type to set. Accepted values are CI_BAR, CI_TEXT or CI_IMAGE." );

const idEventDef EV_SendNetworkCommand( "sendNetworkCommand", '\0', DOC_TEXT( "Sends a message from a network client to this entity." ), 1, NULL, "s", "message", "Message to send." );
const idEventDef EV_SendNetworkEvent( "sendNetworkEvent", '\0', DOC_TEXT( "Sends a message from the server to this object on the specified client." ), 3, NULL, "d", "clientIndex", "Index of the client to send the message to, or -1 for all clients.", "b", "isRepeater", "Whether this is the index of a repeater client, or a regular client.", "s", "message", "Message to send." );

const idEventDef EV_GetNextKey( "getNextKey", 's', DOC_TEXT( "Finds the next key with the given prefix on the entity's metadata." ), 2, "To find the first key that matches, pass an empty string as the previous match.", "s", "prefix", "Prefix of the keys to match.", "s", "lastMatch", "Last key that was found." );
const idEventDef EV_SetKey( "setKey", '\0', DOC_TEXT( "Sets the given key on the entity's metadata to the specified value." ), 2, NULL, "s", "key", "Key to set.", "s", "value", "Value to set the key to." );

const idEventDef EV_GetKey( "getKey", 's', DOC_TEXT( "Looks up meta data from the object." ), 1, "If the key is not found, an empty string will be returned.", "s", "key", "The name of the key to look up." );
const idEventDef EV_GetIntKey( "getIntKey", 'd', DOC_TEXT( "Looks up meta data from the object and converts it to an integer." ), 1, "If the key is not found, or it cannot be converted, 0 will be returned.", "s", "key", "The name of the key to look up." );
const idEventDef EV_GetFloatKey( "getFloatKey", 'f', DOC_TEXT( "Looks up meta data from the object and converts it to a float." ), 1, "If the key is not found, or it cannot be converted, 0 will be returned.", "s", "key", "The name of the key to look up." );
const idEventDef EV_GetVectorKey( "getVectorKey", 'v', DOC_TEXT( "Looks up meta data from the object and converts it to a vector." ), 1, "If the key is not found, or it cannot be converted, '0 0 0' will be returned.", "s", "key", "The name of the key to look up." );
const idEventDef EV_GetEntityKey( "getEntityKey", 'e', DOC_TEXT( "Looks up meta data from the object and finds an entity with the given name." ), 1, "If the key is not found, or there is no entity with the given name, $null$ will be returned.", "s", "key", "The name of the key to look up." );

const idEventDef EV_GetKeyWithDefault( "getKeyWithDefault", 's', DOC_TEXT( "Looks up meta data from the object." ), 2, "If the key is not found, the default value will be returned.", "s", "key", "The name of the key to look up.", "s", "default", "The default value to return." );
const idEventDef EV_GetIntKeyWithDefault( "getIntKeyWithDefault", 'd', DOC_TEXT( "Looks up meta data from the object and converts it to an integer." ), 2, "If the key is not found, the default value will be returned.\nIf the data cannot be converted, 0 will be returned.", "s", "key", "The name of the key to look up.", "d", "default", "The default value to return." );
const idEventDef EV_GetFloatKeyWithDefault( "getFloatKeyWithDefault", 'f', DOC_TEXT( "Looks up meta data from the object and converts it to a float." ), 2, "If the key is not found, the default value will be returned.\nIf the data cannot be converted, 0 will be returned.", "s", "key", "The name of the key to look up.", "f", "default", "The default value to return." );
const idEventDef EV_GetVectorKeyWithDefault( "getVectorKeyWithDefault", 'v', DOC_TEXT( "Looks up meta data from the object and converts it to a vector." ), 2, "If the key is not found, the default value will be returned.\nIf the data cannot be converted, '0 0 0' will be returned.", "s", "key", "The name of the key to look up.", "v", "default", "The default value to return." );

const idEventDef EV_DistanceTo( "distanceTo", 'f', DOC_TEXT( "Returns the distance from this entity to the given entity." ), 1, "If the entity passed in is $null$, a very large distance will be returned.", "E", "entity", "Entity to check the distance to." );
const idEventDef EV_DistanceToPoint( "distanceToPoint", 'f', DOC_TEXT( "Returns the distance from this entity to the given point." ), 1, NULL, "v", "position", "Position to check the distance to in world space." );
const idEventDef EV_GetMaster( "getMaster", 'e', DOC_TEXT( "Returns the master of the chain of bound entities to which this entity belongs, or $null$ is this entity is not in a chain." ), 0, NULL );

const idEventDef EV_PlayMaterialEffect( "playMaterialEffect", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified joint, and returns a handle to the effect. If a special version matching that of the $decl:surfaceType$ passed is found, that will be played, otherwise the exact key name will be used." ), 5, DOC_TEXT( "If no joint is specified, the joint cannot be found, or the entity does not support animated models, the effect will be played at the entity's origin.\nThe key must start with 'fx_'." ), "s", DOC_TEXT( "key" ), DOC_TEXT( "Key to look up the $decl:effect$ from." ), "v", DOC_TEXT( "color" ), DOC_TEXT( "RGB color components." ), "s", DOC_TEXT( "joint" ), DOC_TEXT( "Name of the joint to play the effect on." ), "s", DOC_TEXT( "surfacetype" ), DOC_TEXT( "$decl:surfaceType$ to look up any special effect from." ), "b", DOC_TEXT( "loop" ), DOC_TEXT( "Whether the effect should be once only, or loop." ) );
const idEventDef EV_PlayMaterialEffectMaxVisDist( "playMaterialEffectMaxVisDist", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified joint, and returns a handle to the effect. If a special version matching that of the $decl:surfaceType$ passed is found, that will be played, otherwise the exact key name will be used." ), 7, DOC_TEXT( "If no joint is specified, the joint cannot be found, or the entity does not support animated models, the effect will be played at the entity's origin.\nThe key must start with 'fx_'." ), "s", DOC_TEXT( "key" ), DOC_TEXT( "Key to look up the $decl:effect$ from." ), "v", DOC_TEXT( "color" ), DOC_TEXT( "RGB color components." ), "s", DOC_TEXT( "joint" ), DOC_TEXT( "Name of the joint to play the effect on." ), "s", DOC_TEXT( "surfacetype" ), DOC_TEXT( "$decl:surfaceType$ to look up any special effect from." ), "b", "loop", DOC_TEXT( "Whether the effect should be once only, or loop." ), "f", DOC_TEXT( "maxVisDist" ), DOC_TEXT( "Maximum distance the effect will be visible from." ), "b", DOC_TEXT( "isStatic" ), DOC_TEXT( "Whether the particle effect is static or not" ) );
const idEventDef EV_PlayEffect( "playEffect", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified joint, and returns a handle to the effect." ), 3, "If no joint is specified, the joint cannot be found, or the entity does not support animated models, the effect will be played at the entity's origin.\nThe key must start with 'fx_'.", "s", "key", "Key to look up the $decl:effect$ from.", "s", "joint", "Name of the joint to play the effect on.", "b", "loop", "Whether the effect should be once only, or loop." );
const idEventDef EV_PlayEffectMaxVisDist( "playEffectMaxVisDist", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified joint, and returns a handle to the effect." ), 5, "If no joint is specified, the joint cannot be found, or the entity does not support animated models, the effect will be played at the entity's origin.\nThe key must start with 'fx_'.", "s", "key", "Key to look up the $decl:effect$ from.", "s", "joint", "Name of the joint to play the effect on.", "b", "loop", "Whether the effect should be once only, or loop.", "f", "maxVisDist", "Maximum distance the effect will be visible from.", "b", "isStatic", "Whether the particle effect is static or not" );
const idEventDef EV_PlayJointEffect( "playJointEffect", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified joint, and returns a handle to the effect." ), 3, "If the joint is invalid, the effect will be played at the entity's origin.\nThe key must start with 'fx_'.", "s", "key", "Key to look up the $decl:effect$ from.", "d", "joint", "Index of the joint to play the effect on.", "b", "loop", "Whether the effect should be once only, or loop." );
const idEventDef EV_PlayJointEffectViewSuppress( "playJointEffectViewSuppress", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified joint, and returns a handle to the effect." ), 4, "If the joint is invalid, the effect will be played at the entity's origin.\nThe key must start with 'fx_'.", "s", "key", "Key to look up the $decl:effect$ from.", "d", "joint", "Index of the joint to play the effect on.", "b", "loop", "Whether the effect should be once only, or loop.", "b", "viewSuppress", "If set, the effect will inherit the view suppress setting from this entity, otherwise it will have no view supress settings." );
const idEventDef EV_PlayOriginEffect( "playOriginEffect", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified position in the world, and returns a handle to the effect. If a special version matching that of the $decl:surfaceType$ passed is found, that will be played, otherwise the exact key name will be used." ), 5, "The key must start with 'fx_'.", "s", "key", "Key to look up the $decl:effect$ from.","s", "surfacetype", "$decl:surfaceType$ to look up any special effect from.", "v", "origin", "Origin to play the effect at in world space.", "v", "forward", "Forward vector to orientate the effect with in world space.", "b", "loop", "Whether the effect should be once only, or loop." );
const idEventDef EV_PlayOriginEffectMaxVisDist( "playOriginEffectMaxVisDist", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified position in the world, and returns a handle to the effect. If a special version matching that of the $decl:surfaceType$ passed is found, that will be played, otherwise the exact key name will be used." ), 7, DOC_TEXT( "The key must start with 'fx_'." ), "s", DOC_TEXT( "key" ), DOC_TEXT( "Key to look up the $decl:effect$ from." ),"s", DOC_TEXT( "surfacetype" ), DOC_TEXT( "$decl:surfaceType$ to look up any special effect from." ), "v", DOC_TEXT( "origin" ), DOC_TEXT( "Origin to play the effect at in world space." ), "v", DOC_TEXT( "forward" ), DOC_TEXT( "Forward vector to orientate the effect with in world space." ), "b", DOC_TEXT( "loop" ), DOC_TEXT( "Whether the effect should be once only, or loop." ), "f", DOC_TEXT( "maxVisDist" ), DOC_TEXT( "Maximum distance the effect will be visible from." ), "b", DOC_TEXT( "isStatic" ), DOC_TEXT( "Whether the particle effect is static or not" ) );
const idEventDef EV_PlayBeamEffect( "playBeamEffect", 'h', DOC_TEXT( "Looks up an effect on the entity and plays it at the specified position in the world, and returns a handle to the effect. If a special version matching that of the $decl:surfaceType$ passed is found, that will be played, otherwise the exact key name will be used." ), 5, DOC_TEXT( "The key must start with 'fx_'." ), "s", DOC_TEXT( "key" ), DOC_TEXT( "Key to look up the $decl:effect$ from." ),"s", DOC_TEXT( "surfacetype" ), DOC_TEXT( "$decl:surfaceType$ to look up any special effect from." ), "v", DOC_TEXT( "origin" ), DOC_TEXT( "Origin to play the effect at in world space." ), "v", DOC_TEXT( "endOrigin" ), DOC_TEXT( "End position for the beam." ), "b", DOC_TEXT( "loop" ), DOC_TEXT( "Whether the effect should be once only, or loop." ) );
const idEventDef EV_LookupEffect( "lookupEffect", 's', DOC_TEXT( "Performs an effect lookup on the entity, using the key and $decl:surfaceType$ provided." ), 2, "The key must start with 'fx_'.", "s", "key", "Key to look up the $decl:effect$ from.", "s", DOC_TEXT( "surfacetype" ), DOC_TEXT( "$decl:surfaceType$ to look up any special effect from." ) );
const idEventDef EV_StopEffect( "stopEffect", '\0', DOC_TEXT( "Stops all effects spawned using the specified key on this entity, and any existing particles will decay over time." ), 1, NULL, "s", "key", "Key to stop effects with." );
const idEventDef EV_KillEffect( "killEffect", '\0', DOC_TEXT( "Stops all effects spawned using the specified key on this entity, and clears out any particles they had already created." ), 1, NULL, "s", "key", "Key to stop effects with." );
const idEventDef EV_StopEffectHandle( "stopEffectHandle", '\0', DOC_TEXT( "Stops a specific effect created on this entity, and any existing particles will decay over time.." ), 1, NULL, "h", "handle", "Handle to the effect to stop." );
const idEventDef EV_UnBindEffectHandle( "unBindEffectHandle", '\0', DOC_TEXT( "Unbinds an effect that was created on this entity from this entity." ), 1, NULL, "h", "handle", "Handle to the effect to unbind." );
const idEventDef EV_SetEffectRenderBounds( "setEffectRenderBounds", '\0', DOC_TEXT( "Sets or clears the useRenderBounds flag on the specified effect." ), 2, NULL, "h", "handle", "Handle to the effect to set the flag on.", "b", "state", "Whether to enable or disable the flag." );
const idEventDef EV_SetEffectAttenuation( "setEffectAttenuation", '\0', DOC_TEXT( "Sets the attenuation factor on the specified effect." ), 2, NULL, "h", "handle", "Handle to the effect to set the attenuation of.", "f", "attenuation", "Attenuation factor to set." );
const idEventDef EV_SetEffectColor( "setEffectColor", '\0', DOC_TEXT( "Sets the color on the specified effect." ), 3, NULL, "h", "handle", "Handle of the effect to set the color on.", "v", "color", "RGB components of the color to set.", "f", "alpha", "Alpha component of the color to set." );
const idEventDef EV_SetEffectOrigin( "setEffectOrigin", '\0', DOC_TEXT( "Sets the position of the specified effect, relative to the bind master if it has one, otherwise in world space." ), 2, NULL, "h", "handle", "Handle to the effect to set the origin of.", "v", "origin", "New origin to set." );
const idEventDef EV_SetEffectAngles( "setEffectAngles", '\0', DOC_TEXT( "Sets the orientation of the specified effect, relative to the bind master if it has one, otherwise in world space." ), 2, NULL, "h", "handle", "Handle to the effect to set the orientation of.", "v", "angles", "Angles to set." );
const idEventDef EV_GetEffectOrigin( "getEffectOrigin", "h", 'v' );
const idEventDef EV_GetEffectEndOrigin( "getEffectEndOrigin", "h", 'v' );
const idEventDef EV_StopAllEffects ( "stopAllEffects", '\0', DOC_TEXT( "Stops all effects playing on this entity, and any existing particles will decay over time." ), 0, NULL );
const idEventDef EV_DetachRotationBind( "detachRotationBind", '\0', DOC_TEXT( "Disables orientation locked mode binding for the specified effect." ), 1, NULL, "h", "handle", "Handle to the effect to disable this on." );

const idEventDef EV_DisablePhysics( "disablePhysics", '\0', DOC_TEXT( "Turns off this entity's clip models temporarily." ), 0, "See also $event:enablePhysics$." );
const idEventDef EV_EnablePhysics( "enablePhysics", '\0', DOC_TEXT( "Turns on this entity's clip models after a previous call to $event:disablePhysics$." ), 0, "This call will not do anything if the clip models have been disabled with $event:forceDisableClip$." );

const idEventDef EV_EntitiesInBounds( "entitiesInBounds", 'd', DOC_TEXT( "Finds all entities within the given bounds, adds them to the entity cache, and returns the number found." ), 4, "This function clears the entity cache first.", "v", "mins", "Mins of the bounds to check.", "v", "maxs", "Maxs of the bounds to check.", "d", "mask", "Mask of contents of entities to find.", "b", "absolute", "Whether the bounds are in world space, or in entity space." );
const idEventDef EV_EntitiesInLocalBounds( "entitiesInLocalBounds", 'd', DOC_TEXT( "Finds all entities with the given bounds, in entity space, adds them to the entity cache, and returns the number found." ), 3, "This function clears the entity cache first.", "v", "mins", "Mins of the bounds to check.", "v", "maxs", "Maxs of the bounds to check.", "d", "mask", "Mask of the contents of entities to find." );
const idEventDef EV_EntitiesInTranslation( "entitiesInTranslation", 'd', DOC_TEXT( "Finds all entities along the given translation, adds them to the entity cache, and returns the number found." ), 4, "This function clears the entity cache first.", "v", "start", "Start point of the trace in world space.", "v", "end", "End point of the trace in world space.", "d", "mask", "Mask of the contents of entities to find.", "E", "ignore", "Entity to ignore in the trace." );
const idEventDef EV_EntitiesInRadius( "entitiesInRadius", 'd', DOC_TEXT( "Finds all entities in the given sphere, adds them to the entity cache, and returns the number found." ), 4, "This function clears the entity cache first.", "v", "center", "Center of the sphere.", "f", "radius", "Radius of the sphere.", "d", "mask", "Mask of the contents of entities to find.", "b", "absolute", "Whether the sphere is in world space, or in entity space." );
const idEventDef EV_EntitiesOfClass( "entitiesOfClass", 'd', DOC_TEXT( "Finds all entities of the given $class$, adds them to the entity cache, and returns the size of the entity cache." ), 2, NULL, "d", "index", "Index of the $class$ to find instances of.", "b", "additive", "Whether the entities should be added to the current cache, or the cache should be cleared first." );
const idEventDef EV_EntitiesOfType( "entitiesOfType", 'd', DOC_TEXT( "Finds all entities which were spawned using the given $decl:entityDef$, adds them to the entity cache, and returns the number found." ), 1, "This function clears the entity cache first.", "d", "index", "Index of the $decl:entityDef$ to search with." );
const idEventDef EV_EntitiesOfCollection( "entitiesOfCollection", 'd', DOC_TEXT( "Find all entities in the given collection, adds them to the entity cache, and returns the number found." ), 1, "This function clears the entity cache first.", "s", "name", "Name of the collection to look up." );
const idEventDef EV_FilterEntitiesByRadius( "filterEntitiesByRadius", 'd', DOC_TEXT( "Removes entities from the entity cache based on whether they are inside or outside of the specified sphere, and returns the number that are left." ), 3, NULL, "v", "center", "Center of the sphere.", "f", "radius", "Radius of the sphere.", "b", "inclusive", "Whether to include or exlude the entities withing the sphere." );
const idEventDef EV_FilterEntitiesByClass( "filterEntitiesByClass", 'd', DOC_TEXT( "Removes entities from the entity cache based on whether they are of the given class or not, and returns the number that are left." ), 2, NULL, "s", "name", "Name of the class type to check for.", "b", "inclusive", "Whether to include or exclude the entities of the specified type." );
const idEventDef EV_FilterEntitiesByAllegiance( "filterEntitiesByAllegiance", 'd', DOC_TEXT( "Removes entities from the entity cache based on whether they are of the given allegiance relative to this entity, and returns the number that are left." ), 2, "This does not use disguise information, for that use $event:filterEntitiesByDisguiseAllegiance$.", "d", "mask", "Mask of which allegiances to match. This may include TA_FLAG_FRIEND, TA_FLAG_NEUTRAL, and TA_FLAG_ENEMY.", "b", "inclusive", "Whether to include or exclude matched entities." );
const idEventDef EV_FilterEntitiesByDisguiseAllegiance( "filterEntitiesByDisguiseAllegiance", 'd', DOC_TEXT( "Removes entities from the entity cache based on whether they are of the given allegiance relative to this entity, and returns the number that are left." ), 2, "This includes disguise information, to not do that use $event:filterEntitiesByAllegiance$.", "d", "mask", "Mask of which allegiances to match. This may include TA_FLAG_FRIEND, TA_FLAG_NEUTRAL, and TA_FLAG_ENEMY.", "b", "inclusive", "Whether to include or exclude matched entities." );
const idEventDef EV_FilterEntitiesByFilter( "filterEntitiesByFilter", 'd', DOC_TEXT( "Removes entities from the entity cache based on whether they match the specified $decl:targetInfo$ filter, and returns the number that are left." ), 2, "If the index is out of range, nothing will be removed.", "d", "index", "Index of the $decl:targetInfo$ filter to use.", "b", "inclusive", "Whether to include or exclude entities which match the given filter." );
const idEventDef EV_FilterEntitiesByTouching( "filterEntitiesByTouching", 'd', DOC_TEXT( "Removes entities from the entity cache based on whether their bounds overlap the bounds of this entity." ), 1, NULL, "b", "inclusive", "Whether to include or exclude entities which match." );

const idEventDef EV_GetBoundsCacheCount( "getBoundsCacheCount", 'd', DOC_TEXT( "Returns the number of entities in the entity cache." ), 0, NULL );
const idEventDef EV_GetBoundsCacheEntity( "getBoundsCacheEntity", 'e', DOC_TEXT( "Returns the entity at the given index in the entity cache." ), 1, "This may return $null$ if the entity has been removed since being added to the cache, or the index is out of range.", "d", "index", "Index of the entity to look up." );

const idEventDef EV_GetSavedCacheCount( "getSavedCacheCount", 'd', DOC_TEXT( "Returns the number of entities in the specified saved entity cache." ), 1, "The result will be 0 if the handle is invalid.", "d", "handle", "Handle to the saved entity cache." );
const idEventDef EV_GetSavedCacheEntity( "getSavedCacheEntity", 'e', DOC_TEXT( "Returns the entity at the given index in the specified saved entity cache." ), 2, "This may return $null$ if the entity has been removed since being added to the cache, or the index is out of range.", "d", "index", "Index of the entity to look up.", "d", "handle", "Handle to the saved entity cache." );
const idEventDef EV_SaveCachedEntities( "saveCachedEntities", 'd', DOC_TEXT( "Saves the state of the entity cache, and returns a handle to the saved state." ), 0, "There are a limited number of saved states which can exist at once, so care should be taken to call $event:freeSavedCache$ when you no longer need it.\nThe result will be -1 if the cache could not be saved." );
const idEventDef EV_FreeSavedCache( "freeSavedCache", '\0', DOC_TEXT( "Frees the given saved entity cache." ), 1, NULL, "d", "handle", "Handle to the saved entity cache." );

const idEventDef EV_GetEntityAllegiance( "getEntityAllegiance", 'd', DOC_TEXT( "Returns a value indicating the relationship between the team of this entity and the team of the specified entity." ), 1, "The result will be TA_FRIEND, TA_NEUTRAL, or TA_ENEMY", "E", "entity", "Entity to compare with." );
const idEventDef EV_TakesDamage( "takesDamage", 'b', DOC_TEXT( "Returns the state of the takeDamage flag on the entity." ), 0, NULL );
const idEventDef EV_SetTakesDamage( "setTakesDamage", '\0', DOC_TEXT( "Sets whether or not this entity should take damage." ), 1, NULL, "b", "state", "Whether the entity should take damage or not." );
const idEventDef EV_SetNetworkSynced( "setNetworkSynced", '\0', DOC_TEXT( "Enables/disables network syncing for this entity." ), 1, NULL, "b", "state", "Whether to enable or disable syncing." );
const idEventDef EV_ApplyDamage( "applyDamage", '\0', DOC_TEXT( "Applies damage from the specified $decl:damageDecl$ to this entity." ), 6, NULL, "E", "inflictor", "Entity which is applying the damage.", "E", "attacker", "Entity responsible for the damage.", "v", "direction", "Direction the damage is applied in.", "d", "damage", "Index of the $decl:damageDef$ to apply.", "f", "scale", "Scale factor to apply to the damage.", "o", "trace", "Trace object that represents the collision, if any." );
const idEventDef EV_HasAbility( "hasAbility", 'd', DOC_TEXT( "Returns whether the entity has the given named ability property." ), 1, NULL, "s", "name", "Name of the ability to check for." );
const idEventDef EV_SyncScriptField( "sync", '\0', DOC_TEXT( "Sets up the supplied variable to be network syncronized in the 'visible' stream." ), 1, "The 'visible' stream does not always update straight away.\nSee also $event:syncBroadcast$.\nOnly object, vector, boolean and float types can be synced.", "s", "name", "Name of the variable to sync." );
const idEventDef EV_SyncScriptFieldBroadcast( "syncBroadcast", '\0', DOC_TEXT( "Sets up the supplied variable to be network syncronized in the 'broadcast' stream." ), 1, "The 'broadcast' stream will update straight away whenever there is any change.\nSee also $event:sync$.\nOnly object, vector, boolean and float types can be synced.", "s", "name", "Name of the variable to sync." );
const idEventDef EV_SyncScriptFieldCallback( "syncCallback", '\0', DOC_TEXT( "Sets up a callback to be called whenever the given variable is updated from the server." ), 2, "The variable must have already been synced using either $event:sync$ or $event:syncBroadcast$.\nThe callback must be a function on the same script object.", "s", "name", "Name of the synced variable.", "s", "callback", "Name of the function to call when the value changes." );
const idEventDef EV_Physics_ClearContacts( "clearContacts", '\0', DOC_TEXT( "Removes all contact points stored on the physics object." ), 0, NULL );
const idEventDef EV_Physics_SetContents( "setContents", '\0', DOC_TEXT( "Sets the contents for all the clipmodels belonging to the entity." ), 1, NULL, "d", "contents", "Contents to apply." );
const idEventDef EV_Physics_SetClipmask( "setClipmask", '\0', DOC_TEXT( "Sets the clipmask for all the clipmodels belonging to the entity." ), 1, NULL, "d", "clipmask", "Clipmask to apply." );
const idEventDef EV_Physics_PutToRest( "putToRest", '\0', DOC_TEXT( "Forcefully puts the object to rest." ), 0, "This will potentially in a position where the object would not normally come to rest, in this case, the object will likely begin moving again if it is shot/pushed unless the physics are disabled as well.", 0, NULL );
const idEventDef EV_Physics_HasGroundContacts( "hasGroundContacts", 'b', DOC_TEXT( "Returns whether this object has any contacts opposing gravity." ), 0, "This is not supported by all physics types, if not supported, the result will be false." );
const idEventDef EV_Physics_DisableGravity( "disableGravity", '\0', DOC_TEXT( "Disables/enables gravity on an object. If only you could do this in real life, eh?" ), 1, "This is only supported on physics of type $class:sdPhysics_RigidBodyMultiple$.", "b", "state", "Whether to disable or enable." );
const idEventDef EV_Physics_IsAtRest( "isAtRest", 'b', DOC_TEXT( "Returns whether the physics object is rested or not." ), 0, NULL );
const idEventDef EV_Physics_IsBound( "isBound", 'b', DOC_TEXT( "Returns whether this entity is bound to anything." ), 0, NULL );
const idEventDef EV_Physics_DisableImpact( "disableImpact", '\0', DOC_TEXT( "Makes the object unpushable, and will ignore any force/impulse application." ), 0, "See also $event:enableImpact$." );
const idEventDef EV_Physics_EnableImpact( "enableImpact", '\0', DOC_TEXT( "Makes the object pushable again, and will accept force/impulse application." ), 0, "See also $event:disableImpact$." );
const idEventDef EV_EnableKnockback( "enableKnockback", '\0', DOC_TEXT( "Allows the entity to be pushed by explosive damage." ), 0, "See also $event:disableKnockback$." );
const idEventDef EV_DisableKnockback( "disableKnockback", '\0', DOC_TEXT( "Stops the entity being pushed by explosive damage." ), 0, "See also $event:enableKnockback$." );
const idEventDef EV_HasForceDisableClip( "hasForceDisableClip", 'b', DOC_TEXT( "Returns whether the entity's clip models has been forcefully disabled by a call to $event:forceDisableClip$." ), 0, NULL );
const idEventDef EV_ForceDisableClip( "forceDisableClip", '\0', DOC_TEXT( "Forcefully disables the clip models on this entity, they will only be re-enabled by a call to $event:forceEnableClip$." ), 0, NULL );
const idEventDef EV_ForceEnableClip( "forceEnableClip", '\0', DOC_TEXT( "Re-enables the clip models on this entity after they have been disabled by a call to $event:forceDisableClip$" ), 0, NULL );
const idEventDef EV_TurnTowards( "turnTowards", '\0', DOC_TEXT( "Rotates the physics axis towards the specified forward vector." ), 2, NULL, "v", "forward", "New forward vector to turn towards.", "f", "rate", "Maximum angular velocity to rotate with." );
const idEventDef EV_GetTeam( "getGameTeam", 'o', DOC_TEXT( "Returns the entity's team, or $null$ if none." ), 0, "Teams are of type $class:sdTeamInfo$.\nSee also $event:setGameTeam$." );
const idEventDef EV_SetTeam( "setGameTeam", '\0', DOC_TEXT( "Sets the entity's team." ), 1, "See also $event:getGameTeam$.", "o", "team", "New team to set, or $null$ for none." );
const idEventDef EV_LaunchMissile( "launchMissileSimple", 'e', DOC_TEXT( "Launches a projectile and returns the entity." ), 8, "The result will be $null$ if the projectileIndex is out of range, the projectile is hitscan, or this is a client.", "E", "owner", "Primary owner of the projectile.", "E", "owner2", "Secondary owner of the projectile.", "E", "enemy", "Target of the projectile, for tracking projectiles.", "d", "projectileDef", "Index of the $decl:entityDef$ to use to spawn the entity on the server.", "d", "clientProjectileDef", "Index of the $decl:entityDef$ to use to spawn the entity on the client.", "f", "spread", "Spread scale factor.", "v", "origin", "Position in world space to launch the projectile from.", "v", "velocity", "Initial velocity of the projectile in world space." );
const idEventDef EV_LaunchBullet( "launchBullet", '\0', DOC_TEXT( "Launches a hitscan projectile." ), 8, NULL, "E", "owner", "Primary owner of the projectile.", "E", "igonre", "Secondary entity for the projectile trace to ignore.", "d", "projectileDef", "Index of the $decl:entityDef$ to spawn the projectile with.", "f", "spread", "Spread scale factor.", "v", "origin", "Starting position of the bullet in world space.", "v", "direction", "Direction to fire the bullet in.", "d", "forceTracer", "Whether to play a tracer or not, TRACER_CHANCE for random, TRACER_FORCE for always, or TRACER_OFF for never", "b", "antilag", "Whether ot not to use antilag." );
const idEventDef EV_GetBulletTracerStart( "getBulletTracerStart", 'v', DOC_TEXT( "Returns the stored position of the start of the last bullet tracer." ), 0, NULL );
const idEventDef EV_GetBulletTracerEnd( "getBulletTracerEnd", 'v', DOC_TEXT( "Returns the stored position of the end of the last bullet tracer." ), 0, NULL );
const idEventDef EV_Physics_SetComeToRest( "setComeToRest", '\0', DOC_TEXT( "Sets whether the physics for this entity may come to rest or not." ), 1, NULL, "b", "state", "Whether to enable or disable coming to rest." );
const idEventDef EV_Physics_ApplyImpulse( "applyImpulse", '\0', DOC_TEXT( "Applies an impules to the entity." ), 2, NULL, "v", "impulse", "Impulse to apply.", "v", "position", "Position in world space to apply the force." );
const idEventDef EV_Physics_AddForce( "addForce", '\0', DOC_TEXT( "Applies a non-localized force to the entity." ), 1, NULL, "v", "force", "Force to apply." );
const idEventDef EV_Physics_AddForceAt( "addForceAt", '\0', DOC_TEXT( "Applies a localized force to the entity at the specified position in world space." ), 2, NULL, "v", "force", "Force to apply.", "v", "position", "Position in world space to apply the force." );
const idEventDef EV_Physics_AddTorque( "addTorque", '\0', DOC_TEXT( "Applies torque directly to the entity." ), 1, NULL, "v", "torque", "Torque to apply." );
const idEventDef EV_Physics_Activate( "activatePhysics", '\0', DOC_TEXT( "Makes the physics 'wake up' if it has gone to rest." ), 0, NULL );
const idEventDef EV_GetGUI( "getGUI", 'd', DOC_TEXT( "Returns a handle to the specified GUI on the entity, or a global one." ), 1, "Key may be one of the following:\n'0': Returns the first entity GUI.\n'1': Returns the secpnd entity GUI.\n'2': Returns the third entity GUI.\n'scoreboard': Returns the global scoreboard GUI.\n'hud': Returns the global hud GUI.", "s", "name", "key of the GUI to look up." );
const idEventDef EV_PointInRadar( "pointInRadar", 'b', DOC_TEXT( "Returns whether the point specified is covered by radar for the team this entity is on/" ), 3, "If the entity is not on a team, the result will be false.", "v", "position", "Position to check in world space.", "d", "mask", "Mask of which radar layers to include.", "b", "ignoreJammers", "If set, jammers will not be checked." );
const idEventDef EV_PreventDeployment( "preventDeployment", '\0', DOC_TEXT( "Sets whether this entity should prevent deployment of objects on top it or not." ), 1, NULL, "b", "state", "Whether to prevent deployment or not." );
const idEventDef EV_AllocBeam( "allocBeam", 'd', DOC_TEXT( "Allocates a beam model using the specified $decl:material$ and returns a handle to it." ), 1, "Beam handles are only valid for the entity they were created using.\nUse $event:updateBeam$ to position the beam, and $event:freeBeam$ to remove it.", "s", "material", "Name of the $decl:material$ to use." );
const idEventDef EV_UpdateBeam( "updateBeam", '\0', DOC_TEXT( "Updates the specified beam model." ), 6, "You can allocate beams using $event:allocBeam$.", "d", "handle", "Handle to the beam.", "v", "start", "Start position of the beam in world space.", "v", "end", "End position of the beam in world space.", "v", "color", "RGB components of the color of the beam.", "f", "alpha", "Alpha component of the color of the beam.", "f", "width", "Width of the beam." );
const idEventDef EV_FreeBeam( "freeBeam", '\0', DOC_TEXT( "Frees the specified beam model." ), 1, "You can allocate beams using $event:allocBeam$.", "d", "handle", "Handle to the beam to free." );
const idEventDef EV_FreeAllBeams( "freeAllBeams", '\0', DOC_TEXT( "Frees all beams created on this entity." ), 0, "You can allocate beams using $event:allocBeam$." );
const idEventDef EV_GetNextTeamEntity( "getNextTeamEntity", 'e', DOC_TEXT( "Returns the next entity in a team of bound entities." ), 0, "This will return $null$ if the entity is not bound, or is the last entity in the chain." );
const idEventDef EV_GetHealth( "getHealth", 'd', DOC_TEXT( "Returns the current health of the object." ), 0, NULL );
const idEventDef EV_SetHealth( "setHealth", '\0', DOC_TEXT( "Sets the current health of the object." ), 1, NULL, "d", "health", "New health value to set." );
const idEventDef EV_GetMaxHealth( "getMaxHealth", 'd', DOC_TEXT( "Returns the maximum health of the object." ), 0, NULL );
const idEventDef EV_SetMaxHealth( "setMaxHealth", '\0', DOC_TEXT( "Sets the maximum health of the object." ), 1, NULL, "d", "max", "Maximum health value to set." );
const idEventDef EV_SetCanCollideWithTeam( "setCanCollideWithTeam", '\0', DOC_TEXT( "Controls whether this entity can collide with entities on the same team as it." ), 1, NULL, "b", "state", "Whether to allow collsions or not." );
const idEventDef EV_IsInWater( "isInWater", 'f', DOC_TEXT( "Returns the fraction of the entity which is under water." ), 0, "Not all physics types support this, if they do not, they will always return 0." ); 
const idEventDef EV_SpawnClientEffect( "spawnClientEffect", 'e', DOC_TEXT( "Spawns a client entity of type $class:rvClientEffect$, and returns it." ), 1, "The key must begin with 'fx_'.\nThe result will be $null$ if spawning the effect fails.", "s", "key", "Key to look up the $decl:effect$ from." );
const idEventDef EV_SpawnClientCrawlEffect( "spawnClientCrawlEffect", 'e', DOC_TEXT( "Spawns a client crawl effect of type $class:rvClientCrawlEffect$ on the specified entity, and returns it." ), 3, "The key must begin with 'fx_'.\nThe result will be $null$ if spawning the effect fails.", "s", "key", "Key to look up the $decl:effect$ from.", "e", "entity", "Entity the effect will crawl over.", "f", "time", "How long the effect will last, in seconds." );

const idEventDef EV_GetSpawnID( "getSpawnID", 's', DOC_TEXT( "Returns the entity's spawnID in string form, for network transmission." ), 0, NULL );

const idEventDef EV_SetSpotted( "setSpotted", '\0', DOC_TEXT( "Marks the entity as having been spotted. If the entity has never been marked before, it will call the callback OnSpotted, with the supplied entity as the parameter." ), 1, NULL, "e", "spotter", "Entity responsible for having spotted this entity." );
const idEventDef EV_IsSpotted( "isSpotted", 'f', DOC_TEXT( "Returns whether this entity has ever been marked as spotted by a call to $event:setSpotted$." ), 0, NULL );
const idEventDef EV_ForceNetworkUpdate( "forceNetworkUpdate", '\0', DOC_TEXT( "Forces any current changes to the entity's 'visible' network stream to not be delayed by AoR." ), 0, NULL );

const idEventDef EV_AddCheapDecal( "addCheapDecal", '\0', DOC_TEXT( "Adds a cheap decal to the specified entity." ), 5, NULL, "E", "entity", "Entity to attach the decal to, or $null$ for the world.", "v", "origin", "Origin to add the decal at.", "v", "normal", "Normal of the plane the decal is on.", "s", "decal", "Name of the decal entry to use.", "s", "surfaceType", "Name of the $decl:surfaceType$ to look up any special decal for." );

const idEventDef EV_GetEntityNumber( "getEntityNumber", 'd', DOC_TEXT( "Returns the index of the entity." ), 0, NULL );

const idEventDef EV_DisableCrosshairTrace( "disableCrosshairTrace", '\0', DOC_TEXT( "Disables/enables this entity for use during player crosshair traces." ), 1, NULL, "b", "state", "Whether to disable or enabled traces on this entity." );
const idEventDef EV_InCollection( "inCollection", 'b', DOC_TEXT( "Returns whether the entity is a member of the given named collection." ), 1, "If the named collection does not exist, the result will be false.", "s", "name", "Name of the collection to check." );

const idEventDef EV_GetEntityContents( "getEntityContents", 'd', DOC_TEXT( "Returns the combined contents of anything within the clipmodels of this entity." ), 0, NULL );
const idEventDef EV_GetMaskedEntityContents( "getMaskedEntityContents", 'd', DOC_TEXT( "Returned the combined contents of anything within the clipmodels of this entity, masked off with the specified mask." ), 1, NULL, "d", "mask", "Mask of contents to include." );

const idEventDef EV_GetDriver( "getDriver", 'e', DOC_TEXT( "Returns the person controlling this entity as a proxy." ), 0, "This is just shorthand for calling $event:getPositionPlayer$ with 0 as a parameter." );
const idEventDef EV_GetPositionPlayer( "getPositionPlayer", 'e', DOC_TEXT( "Returns the player in the proxy at the specified proxy index." ), 1, "If the entity does not support being a proxy, or the index is out of range, the result will be $null$.", "d", "index", "Index of the position to look up." );

const idEventDef EV_GetDamageScale( "getDamageScale", 'f', DOC_TEXT( "Returns the damage XP multiplier for the entity." ), 0, NULL );
const idEventDef EV_GetJointHandle( "getJointHandle", 'd', DOC_TEXT( "Returns a handle to the joint specified." ), 1, "If the entity does not support animation, or the joint cannot be found, the result will be -1.", "s", "name", "Name of the joint to look up." );
const idEventDef EV_GetDefaultSurfaceType( "getDefaultSurfaceType", 's', DOC_TEXT( "Returns the name of the $decl:surfaceType$ to use for parts of the entity with no $decl:surfaceType$." ), 0, NULL );
const idEventDef EV_ForceRunPhysics( "forceRunPhysics", '\0', DOC_TEXT( "Runs a physics tic for the entity." ), 0, NULL );

idEntity::entityCache_t					idEntity::scriptEntityCache;
sdPair< bool, idEntity::entityCache_t >	idEntity::savedEntityCache[ NUM_ENTITY_CAHCES ];
sdCrosshairInfo*						idEntity::crosshairInfo = NULL;

ABSTRACT_DECLARATION( idClass, idEntity )
	EVENT( EV_Remove,						idEntity::Event_Remove )
	EVENT( EV_GetName,						idEntity::Event_GetName )
	EVENT( EV_FindTargets,					idEntity::Event_FindTargets )
	EVENT( EV_BindToJoint,					idEntity::Event_BindToJoint )
	EVENT( EV_RemoveBinds,					idEntity::Event_RemoveBinds )
	EVENT( EV_Bind,							idEntity::Event_Bind )
	EVENT( EV_BindPosition,					idEntity::Event_BindPosition )
	EVENT( EV_Unbind,						idEntity::Event_Unbind )
	EVENT( EV_SetModel,						idEntity::Event_SetModel )
	EVENT( EV_SetSkin,						idEntity::Event_SetSkin )
	EVENT( EV_SetCoverage,					idEntity::Event_SetCoverage )
	EVENT( EV_GetShaderParm,				idEntity::Event_GetShaderParm )
	EVENT( EV_SetShaderParm,				idEntity::Event_SetShaderParm )
	EVENT( EV_SetShaderParms,				idEntity::Event_SetShaderParms )
	EVENT( EV_SetColor,						idEntity::Event_SetColor )
	EVENT( EV_GetColor,						idEntity::Event_GetColor )
	EVENT( EV_IsHidden,						idEntity::Event_IsHidden )
	EVENT( EV_Hide,							idEntity::Event_Hide )
	EVENT( EV_Show,							idEntity::Event_Show )
	EVENT( EV_StopSound,					idEntity::Event_StopSound )
	EVENT( EV_StartSound,					idEntity::Event_StartSound )
	EVENT( EV_FadeSound,					idEntity::Event_FadeSound )
	EVENT( EV_SetChannelPitchShift,			idEntity::Event_SetChannelPitchShift )
	EVENT( EV_SetChannelFlags,				idEntity::Event_SetChannelFlags )
	EVENT( EV_ClearChannelFlags,			idEntity::Event_ClearChannelFlags )
	EVENT( EV_GetWorldAxis,					idEntity::Event_GetWorldAxis )
	EVENT( EV_SetWorldAxis,					idEntity::Event_SetWorldAxis )
	EVENT( EV_GetWorldOrigin,				idEntity::Event_GetWorldOrigin )
	EVENT( EV_GetGravityNormal,				idEntity::Event_GetGravityNormal )
	EVENT( EV_SetWorldOrigin,				idEntity::Event_SetWorldOrigin )
	EVENT( EV_GetOrigin,					idEntity::Event_GetOrigin )
	EVENT( EV_SetOrigin,					idEntity::Event_SetOrigin )
	EVENT( EV_GetAngles,					idEntity::Event_GetAngles )
	EVENT( EV_SetAngles,					idEntity::Event_SetAngles )
	EVENT( EV_SetGravity,					idEntity::Event_SetGravity )
	EVENT( EV_AlignToAxis,					idEntity::Event_AlignToAxis )
	EVENT( EV_GetLinearVelocity,			idEntity::Event_GetLinearVelocity )
	EVENT( EV_SetLinearVelocity,			idEntity::Event_SetLinearVelocity )
	EVENT( EV_GetAngularVelocity,			idEntity::Event_GetAngularVelocity )
	EVENT( EV_SetAngularVelocity,			idEntity::Event_SetAngularVelocity )
	EVENT( EV_GetMass,						idEntity::Event_GetMass )
	EVENT( EV_GetCenterOfMass,				idEntity::Event_GetCenterOfMass )
	EVENT( EV_SetFriction,					idEntity::Event_SetFriction )
	EVENT( EV_GetSize,						idEntity::Event_GetSize )
	EVENT( EV_SetSize,						idEntity::Event_SetSize )
	EVENT( EV_GetMins,						idEntity::Event_GetMins )
	EVENT( EV_GetMaxs,						idEntity::Event_GetMaxs )
	EVENT( EV_GetAbsMins,					idEntity::Event_GetAbsMins )
	EVENT( EV_GetAbsMaxs,					idEntity::Event_GetAbsMaxs )
	EVENT( EV_GetRenderMins,				idEntity::Event_GetRenderMins )
	EVENT( EV_GetRenderMaxs,				idEntity::Event_GetRenderMaxs )
	EVENT( EV_SetRenderBounds,				idEntity::Event_SetRenderBounds )
	EVENT( EV_Touches,						idEntity::Event_Touches )
	EVENT( EV_TouchesBounds,				idEntity::Event_TouchesBounds )
	EVENT( EV_GetNextKey,					idEntity::Event_GetNextKey )
	EVENT( EV_SetKey,						idEntity::Event_SetKey )

	EVENT( EV_GetKey,						idEntity::Event_GetKey )
	EVENT( EV_GetIntKey,					idEntity::Event_GetIntKey )
	EVENT( EV_GetFloatKey,					idEntity::Event_GetFloatKey )
	EVENT( EV_GetVectorKey,					idEntity::Event_GetVectorKey )
	EVENT( EV_GetEntityKey,					idEntity::Event_GetEntityKey )

	EVENT( EV_GetKeyWithDefault,			idEntity::Event_GetKeyWithDefault )
	EVENT( EV_GetIntKeyWithDefault,			idEntity::Event_GetIntKeyWithDefault )
	EVENT( EV_GetFloatKeyWithDefault,		idEntity::Event_GetFloatKeyWithDefault )
	EVENT( EV_GetVectorKeyWithDefault,		idEntity::Event_GetVectorKeyWithDefault )

	EVENT( EV_DistanceTo,					idEntity::Event_DistanceTo )
	EVENT( EV_DistanceToPoint,				idEntity::Event_DistanceToPoint )
	EVENT( EV_DisablePhysics,				idEntity::Event_DisablePhysics )
	EVENT( EV_EnablePhysics,				idEntity::Event_EnablePhysics )
	EVENT( EV_GetMaster,					idEntity::Event_GetMaster )

	EVENT( EV_PlayEffect,					idEntity::Event_PlayEffect )
	EVENT( EV_PlayEffectMaxVisDist,			idEntity::Event_PlayEffectMaxVisDist )
	EVENT( EV_PlayJointEffect,				idEntity::Event_PlayJointEffect )
	EVENT( EV_PlayJointEffectViewSuppress,	idEntity::Event_PlayJointEffectViewSuppress )
	EVENT( EV_PlayOriginEffect,				idEntity::Event_PlayOriginEffect )
	EVENT( EV_PlayOriginEffectMaxVisDist,	idEntity::Event_PlayOriginEffectMaxVisDist )
	EVENT( EV_PlayBeamEffect,				idEntity::Event_PlayBeamEffect )
	EVENT( EV_PlayMaterialEffect,			idEntity::Event_PlayMaterialEffect )
	EVENT( EV_PlayMaterialEffectMaxVisDist,	idEntity::Event_PlayMaterialEffectMaxVisDist )
	EVENT( EV_LookupEffect,					idEntity::Event_LookupEffect )
	EVENT( EV_StopEffect,					idEntity::Event_StopEffect )
	EVENT( EV_KillEffect,					idEntity::Event_KillEffect )
	EVENT( EV_StopEffectHandle,				idEntity::Event_StopEffectHandle )
	EVENT( EV_UnBindEffectHandle,			idEntity::Event_UnBindEffectHandle )
	EVENT( EV_SetEffectRenderBounds,		idEntity::Event_SetEffectRenderBounds )
	EVENT( EV_SetEffectAttenuation,			idEntity::Event_SetEffectAttenuation )
	EVENT( EV_SetEffectColor,				idEntity::Event_SetEffectColor )
	EVENT( EV_SetEffectOrigin,				idEntity::Event_SetEffectOrigin )
	EVENT( EV_SetEffectAngles,				idEntity::Event_SetEffectAngles )
	EVENT( EV_GetEffectOrigin,				idEntity::Event_GetEffectOrigin )
	EVENT( EV_GetEffectEndOrigin,			idEntity::Event_GetEffectEndOrigin )
	EVENT( EV_StopAllEffects,				idEntity::Event_StopAllEffects )
	EVENT( EV_DetachRotationBind,			idEntity::Event_DetachRotationBind )

	EVENT( EV_EntitiesInBounds,				idEntity::Event_EntitiesInBounds )
	EVENT( EV_EntitiesInLocalBounds,		idEntity::Event_EntitiesInLocalBounds )
	EVENT( EV_EntitiesInTranslation,		idEntity::Event_EntitiesInTranslation )
	EVENT( EV_EntitiesInRadius,				idEntity::Event_EntitiesInRadius )
	EVENT( EV_EntitiesOfType,				idEntity::Event_EntitiesOfType )
	EVENT( EV_EntitiesOfCollection,			idEntity::Event_EntitiesOfCollection )
	EVENT( EV_EntitiesOfClass,				idEntity::Event_EntitiesOfClass )
	EVENT( EV_FilterEntitiesByRadius,		idEntity::Event_FilterEntitiesByRadius )
	EVENT( EV_FilterEntitiesByClass,		idEntity::Event_FilterEntitiesByClass )
	EVENT( EV_FilterEntitiesByAllegiance,	idEntity::Event_FilterEntitiesByAllegiance )
	EVENT( EV_FilterEntitiesByDisguiseAllegiance,	idEntity::Event_FilterEntitiesByDisguiseAllegiance )
	EVENT( EV_FilterEntitiesByFilter,		idEntity::Event_FilterEntitiesByFilter )
	EVENT( EV_FilterEntitiesByTouching,		idEntity::Event_FilterEntitiesByTouching )

	EVENT( EV_GetBoundsCacheCount,			idEntity::Event_GetBoundsCacheCount )
	EVENT( EV_GetBoundsCacheEntity,			idEntity::Event_GetBoundsCacheEntity )

	EVENT( EV_GetSavedCacheCount,			idEntity::Event_GetSavedCacheCount )
	EVENT( EV_GetSavedCacheEntity,			idEntity::Event_GetSavedCacheEntity )
	EVENT( EV_SaveCachedEntities,			idEntity::Event_SaveCachedEntities )
	EVENT( EV_FreeSavedCache,				idEntity::Event_FreeSavedCache )

	EVENT( EV_GetEntityAllegiance,			idEntity::Event_GetEntityAllegiance )
	EVENT( EV_HasAbility,					idEntity::Event_HasAbility )
	EVENT( EV_SyncScriptField,				idEntity::Event_SyncScriptField )
	EVENT( EV_SyncScriptFieldBroadcast,		idEntity::Event_SyncScriptFieldBroadcast )
	EVENT( EV_SyncScriptFieldCallback,		idEntity::Event_SyncScriptFieldCallback )
	EVENT( EV_TakesDamage,					idEntity::Event_TakesDamage )
	EVENT( EV_SetTakesDamage,				idEntity::Event_SetTakesDamage )
	EVENT( EV_SetNetworkSynced,				idEntity::Event_SetNetworkSynced )
	EVENT( EV_ApplyDamage,					idEntity::Event_ApplyDamage )

	EVENT( EV_Physics_ClearContacts,		idEntity::Event_Physics_ClearContacts )
	EVENT( EV_Physics_SetContents,			idEntity::Event_Physics_SetContents )
	EVENT( EV_Physics_SetClipmask,			idEntity::Event_Physics_SetClipmask )
	EVENT( EV_Physics_PutToRest,			idEntity::Event_Physics_PutToRest )
	EVENT( EV_Physics_HasGroundContacts,	idEntity::Event_Physics_HasGroundContacts )
	EVENT( EV_Physics_DisableGravity,		idEntity::Event_DisableGravity )
	EVENT( EV_Physics_SetComeToRest,		idEntity::Event_Physics_SetComeToRest )
	EVENT( EV_Physics_ApplyImpulse,			idEntity::Event_Physics_ApplyImpulse )
	EVENT( EV_Physics_AddForce,				idEntity::Event_Physics_AddForce )
	EVENT( EV_Physics_AddForceAt,			idEntity::Event_Physics_AddForceAt )
	EVENT( EV_Physics_AddTorque,			idEntity::Event_Physics_AddTorque )
	EVENT( EV_Physics_IsAtRest,				idEntity::Event_IsAtRest )
	EVENT( EV_Physics_IsBound,				idEntity::Event_IsBound )
	EVENT( EV_Physics_DisableImpact,		idEntity::Event_DisableImpact )
	EVENT( EV_Physics_EnableImpact,			idEntity::Event_EnableImpact )
	EVENT( EV_EnableKnockback,				idEntity::Event_EnableKnockback )
	EVENT( EV_DisableKnockback,				idEntity::Event_DisableKnockback )
	EVENT( EV_Physics_Activate,				idEntity::Event_Physics_Activate )
	EVENT( EV_HasForceDisableClip,			idEntity::Event_HasForceDisableClip )
	EVENT( EV_ForceDisableClip,				idEntity::Event_ForceDisableClip )
	EVENT( EV_ForceEnableClip,				idEntity::Event_ForceEnableClip )
	EVENT( EV_TurnTowards,					idEntity::Event_TurnTowards )

	EVENT( EV_GetTeam,						idEntity::Event_GetTeam )
	EVENT( EV_SetTeam,						idEntity::Event_SetTeam )

	EVENT( EV_LaunchMissile,				idEntity::Event_LaunchMissile )
	EVENT( EV_LaunchBullet,					idEntity::Event_LaunchBullet )
	EVENT( EV_GetBulletTracerStart,			idEntity::Event_GetBulletTracerStart )
	EVENT( EV_GetBulletTracerEnd,			idEntity::Event_GetBulletTracerEnd )

	EVENT( EV_GetHealth,					idEntity::Event_GetHealth )
	EVENT( EV_SetHealth,					idEntity::Event_SetHealth )
	EVENT( EV_GetMaxHealth,					idEntity::Event_GetMaxHealth )
	EVENT( EV_SetMaxHealth,					idEntity::Event_SetMaxHealth )

	EVENT( EV_GetGUI,						idEntity::Event_GetGUI )

	EVENT( EV_SetNumCrosshairLines,			idEntity::Event_SetNumCrosshairLines )
	EVENT( EV_AddCrosshairLine,				idEntity::Event_AddCrosshairLine )
	EVENT( EV_GetCrosshairDistance,			idEntity::Event_GetCrosshairDistance )
	EVENT( EV_SetCrosshairLineText,			idEntity::Event_SetCrosshairLineText )
	EVENT( EV_SetCrosshairLineTextIndex,	idEntity::Event_SetCrosshairLineTextIndex )
	EVENT( EV_SetCrosshairLineMaterial,		idEntity::Event_SetCrosshairLineMaterial )
	EVENT( EV_SetCrosshairLineColor,		idEntity::Event_SetCrosshairLineColor )
	EVENT( EV_SetCrosshairLineSize,			idEntity::Event_SetCrosshairLineSize )
	EVENT( EV_SetCrosshairLineFraction,		idEntity::Event_SetCrosshairLineFraction )
	EVENT( EV_SetCrosshairLineType,			idEntity::Event_SetCrosshairLineType )

	EVENT( EV_SendNetworkCommand,			idEntity::Event_SendNetworkCommand )
	EVENT( EV_SendNetworkEvent,				idEntity::Event_SendNetworkEvent )
	EVENT( EV_PointInRadar,					idEntity::Event_PointInRadar )

	EVENT( EV_PreventDeployment,			idEntity::Event_PreventDeployment )

	EVENT( EV_AllocBeam,					idEntity::Event_AllocBeam )
	EVENT( EV_UpdateBeam,					idEntity::Event_UpdateBeam )
	EVENT( EV_FreeBeam,						idEntity::Event_FreeBeam )
	EVENT( EV_FreeAllBeams,					idEntity::Event_FreeAllBeams )

	EVENT( EV_GetNextTeamEntity,			idEntity::Event_GetNextTeamEntity )

	EVENT( EV_SetCanCollideWithTeam,		idEntity::Event_SetCanCollideWithTeam )
	EVENT( EV_SpawnClientEffect,			idEntity::Event_SpawnClientEffect )
	EVENT( EV_SpawnClientCrawlEffect,		idEntity::Event_SpawnClientCrawlEffect )

	EVENT( EV_IsInWater,					idEntity::Event_IsInWater )
	EVENT( EV_GetSpawnID,					idEntity::Event_GetSpawnID )

	EVENT( EV_SetSpotted,					idEntity::Event_SetSpotted )
	EVENT( EV_IsSpotted,					idEntity::Event_IsSpotted )
	EVENT( EV_ForceNetworkUpdate,			idEntity::Event_ForceNetworkUpdate )

	EVENT( EV_AddCheapDecal,				idEntity::Event_AddCheapDecal )	

	EVENT( EV_GetEntityNumber,				idEntity::Event_GetEntityNumber )

	EVENT( EV_DisableCrosshairTrace,		idEntity::Event_DisableCrosshairTrace )
	EVENT( EV_InCollection,					idEntity::Event_InCollection )

	EVENT( EV_GetEntityContents,			idEntity::Event_GetEntityContents )
	EVENT( EV_GetMaskedEntityContents,		idEntity::Event_GetMaskedEntityContents )

	EVENT( EV_GetDriver,					idEntity::Event_GetDriver )
	EVENT( EV_GetNumPositions,				idEntity::Event_GetNumPositions )
	EVENT( EV_GetPositionPlayer,			idEntity::Event_GetPositionPlayer )

	EVENT( EV_GetDamageScale,				idEntity::Event_GetDamageScale )
	EVENT( EV_GetJointHandle,				idEntity::Event_GetJointHandle )

	EVENT( EV_GetDefaultSurfaceType,		idEntity::Event_GetDefaultSurfaceType )

	EVENT( EV_ForceRunPhysics,				idEntity::Event_ForceRunPhysics )
END_CLASS

/*
================
idGameEdit::ParseSpawnArgsToRenderEntity

parse the static model parameters
this is the canonical renderEntity parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRenderEntity( const idDict& args, renderEntity_t& renderEntity ) {
	memset( &renderEntity, 0, sizeof( renderEntity ) );

	const char* temp;

	temp = args.GetString( "model" );
	const idDeclModelDef* modelDef = NULL;
	if ( *temp ) {
		if( !strstr( temp, "." ) ) {
			modelDef = gameLocal.declModelDefType.LocalFind( temp, false );
			if ( modelDef ) {
				renderEntity.hModel = modelDef->ModelHandle();
			}
		}

		if ( !renderEntity.hModel ) {
			renderEntity.hModel = renderModelManager->FindModel( temp );
		}
	}

	if ( renderEntity.hModel ) {
		renderEntity.hModel->SetLevelLoadReferenced( true );
		renderEntity.hModel->TouchData();
	}

	temp = args.GetString( "skin" );
	if ( *temp ) {
		renderEntity.customSkin = declHolder.declSkinType.LocalFind( temp );
	} else if ( modelDef ) {
		renderEntity.customSkin = modelDef->GetDefaultSkin();
	}

	RefreshRenderEntity( args, renderEntity );
}

/*
================
idGameEdit::RefreshRenderEntity
================
*/
void idGameEdit::RefreshRenderEntity( const idDict& args, renderEntity_t& renderEntity ) {
	if ( renderEntity.hModel ) {
		renderEntity.bounds = renderEntity.hModel->Bounds( &renderEntity );
	} else {
		renderEntity.bounds.Zero();
	}

	renderEntity.mapId = args.GetInt( "spawn_mapSpawnId" );

	const char* temp;
	temp = args.GetString( "shader" );
	if ( *temp != '\0' ) {
		renderEntity.customShader = declHolder.declMaterialType.LocalFind( temp );
	}

	temp = args.GetString( "imposter" );
	if ( *temp != '\0' ) {
		renderEntity.imposter = declHolder.FindImposter( temp );
	}

	temp = args.GetString( "ambientCubeMap" );
	if ( *temp != '\0' ) {	
		renderEntity.ambientCubeMap = declHolder.FindAmbientCubeMap( temp );
	}

	renderEntity.flags.forceImposter = args.GetBool( "forceImposter", "0" );

	args.GetVector( "origin", "0 0 0", renderEntity.origin );

	// get the rotation matrix in either full form, or single angle form
	if ( !args.GetMatrix( "rotation", "1 0 0 0 1 0 0 0 1", renderEntity.axis ) ) {
		float angle = args.GetFloat( "angle" );
		if ( angle != 0.0f ) {
			idAngles::YawToMat3( angle, renderEntity.axis );
		} else {
			renderEntity.axis.Identity();
		}
	}

	renderEntity.referenceSound = NULL;

	// get shader parms
	idVec3		color;
	args.GetVector( "_color", "1 1 1", color );
	float overBright = args.GetFloat( "_overbright", "1.0f" );

	renderEntity.shaderParms[ SHADERPARM_RED ]		= color[0] * overBright;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= color[1] * overBright;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= color[2] * overBright;
	renderEntity.shaderParms[ 3 ]					= args.GetFloat( "shaderParm3", "1" );
	renderEntity.shaderParms[ 4 ]					= args.GetFloat( "shaderParm4", "0" );
	renderEntity.shaderParms[ 5 ]					= args.GetFloat( "shaderParm5", "0" );
	renderEntity.shaderParms[ 6 ]					= args.GetFloat( "shaderParm6", "0" );
	renderEntity.shaderParms[ 7 ]					= args.GetFloat( "shaderParm7", "0" );
	renderEntity.shaderParms[ 8 ]					= args.GetFloat( "shaderParm8", "0" );
	renderEntity.shaderParms[ 9 ]					= args.GetFloat( "shaderParm9", "0" );
	renderEntity.shaderParms[ 10 ]					= args.GetFloat( "shaderParm10", "0" );
	renderEntity.shaderParms[ 11 ]					= args.GetFloat( "shaderParm11", "0" );

	idVec3 fadeOrigin;
	renderEntity.flags.useFadeOrigin = args.GetVector( "fadeOrigin", "0 0 0", fadeOrigin );
	if ( renderEntity.flags.useFadeOrigin ) {
		renderEntity.shaderParms[ SHADERPARM_FADE_ORIGIN_X ] = fadeOrigin.x;
		renderEntity.shaderParms[ SHADERPARM_FADE_ORIGIN_Y ] = fadeOrigin.y;
		renderEntity.shaderParms[ SHADERPARM_FADE_ORIGIN_Z ] = fadeOrigin.z;
	}

	// check noDynamicInteractions flag
	renderEntity.flags.noDynamicInteractions = args.GetBool( "noDynamicInteractions" );

	// check noshadows flag
	renderEntity.flags.noShadow = args.GetBool( "noshadows" );

	// check noselfshadows flag
	renderEntity.flags.noSelfShadow = args.GetBool( "noselfshadows" );

	renderEntity.flags.usePointTestForAmbientCubeMaps = args.GetBool( "usePointTestForAmbientCubeMaps" );

	// check pushIntoConnectedOutsideAreas flag
	renderEntity.flags.pushIntoOutsideAreas = args.GetBool( "pushIntoOutsideAreas" );
	renderEntity.flags.pushIntoConnectedOutsideAreas = args.GetBool( "pushIntoConnectedOutsideAreas" );
	renderEntity.flags.overridenCoverage = args.GetBool( "overridenCoverage" );
	renderEntity.coverage = 1.f;

	renderEntity.minGpuSpec = args.GetInt( "minGpuSpec", "0" );
	renderEntity.shadowVisDistMult = args.GetFloat( "shadowVisDistMult", "0" );
	renderEntity.maxVisDist = args.GetInt( "maxVisDist", "0" );
	renderEntity.minVisDist = args.GetInt( "minVisDist", "0" );
	renderEntity.visDistFalloff = args.GetFloat( "visDistFalloff", "0.25" );

	idStr gpuSpecParam = args.GetString( "drawSpec", "low" );
	if ( gpuSpecParam.Icmp( "high" ) == 0 ) {
		renderEntity.drawSpec = 2;
	} else if ( gpuSpecParam.Icmp( "med" ) == 0 || gpuSpecParam.Icmp( "medium" ) == 0 ) {
		renderEntity.drawSpec = 1;
	} else if ( gpuSpecParam.Icmp( "low" ) == 0 ) {
		renderEntity.drawSpec = 0;
	} else {
		renderEntity.drawSpec = 0;
	}

	idStr shadowSpec = args.GetString( "shadowSpec", "low" );
	if ( shadowSpec.Icmp( "high" ) == 0 ) {
		renderEntity.shadowSpec = 2;
	} else if ( shadowSpec.Icmp( "med" ) == 0 || shadowSpec.Icmp( "medium" ) == 0) {
		renderEntity.shadowSpec = 1;
	} else if ( shadowSpec.Icmp( "low" ) == 0 ) {
		renderEntity.shadowSpec = 0;
	} else {
		renderEntity.shadowSpec = 0;
	}

	renderEntity.flags.occlusionTest = args.GetBool( "occlusionTest" );

	idStr mirrorParam = args.GetString( "mirror", "always" );
	if ( mirrorParam.Icmp( "mirroronly" ) == 0 ) {
		renderEntity.allowSurfaceInViewID = MIRROR_VIEW_ID;
	} else if ( mirrorParam.Icmp( "viewonly" ) == 0 ) {
		renderEntity.suppressSurfaceInViewID = MIRROR_VIEW_ID;
	}

	renderEntity.flags.weaponDepthHack = args.GetBool( "weaponDepthHack" );

	// init any guis, including entity-specific states
	//if ( !networkSystem->IsDedicated() ) {
	//	idStr theme;
	//	for ( int i = 0; i < MAX_RENDERENTITY_GUI; i++ ) {
	//		temp = args.GetString( i == 0 ? "gui" : va( "gui%d", i + 1 ) );
	//		if ( *temp != '\0' ) {
	//			theme = args.GetString( i == 0 ? "gui_theme" : va( "gui%d_theme", i + 1 ), "default" );
	//			renderEntity.gui[ i ] = gameLocal.LoadUserInterface( temp, true, false, theme );
	//			sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( renderEntity.gui[ i ] );
	//			if ( ui != NULL ) {
	//				ui->Activate();
	//			}
	//		}
	//	}
	//}
}

/*
================
idGameEdit::ParseSpawnArgsToRefSound

parse the sound parameters
this is the canonical refSound parm parsing,
which should be used by dmap and the editor
================
*/
void idGameEdit::ParseSpawnArgsToRefSound( const idDict& args, refSound_t& refSound ) {
	const char	*temp;

	memset( &refSound, 0, sizeof( refSound ) );

	refSound.parms.minDistance = args.GetFloat( "s_mindistance" );
	refSound.parms.maxDistance = args.GetFloat( "s_maxdistance" );
	refSound.parms.volume = args.GetFloat( "s_volume" );
	refSound.parms.shakes = args.GetFloat( "s_shakes" );

	args.GetVector( "origin", "0 0 0", refSound.origin );

	refSound.referenceSound  = NULL;

	// if a diversity is not specified, every sound start will make
	// a random one.  Specifying diversity is useful to make multiple
	// lights all share the same buzz sound offset, for instance.
	refSound.diversity = args.GetFloat( "s_diversity", "-1" );
	refSound.waitfortrigger = args.GetBool( "s_waitfortrigger" );

	if ( args.GetBool( "s_omni" ) ) {
		refSound.parms.soundShaderFlags |= SSF_OMNIDIRECTIONAL;
	}
	if ( args.GetBool( "s_looping" ) ) {
		refSound.parms.soundShaderFlags |= SSF_LOOPING;
	}
	if ( !args.GetBool( "s_occlusion", "1" ) ) {
		refSound.parms.soundShaderFlags |= SSF_NO_OCCLUSION;
	}
	if ( args.GetBool( "s_global" ) ) {
		refSound.parms.soundShaderFlags |= SSF_GLOBAL;
	}
	if ( args.GetBool( "s_unclamped" ) ) {
		refSound.parms.soundShaderFlags |= SSF_UNCLAMPED;
	}
	if ( args.GetBool( "s_singleArea" ) ) {
		if ( gameRenderWorld != NULL ) {
			refSound.parms.soundArea = gameRenderWorld->PointInArea( refSound.origin ) + 1;
		}
	}
	refSound.parms.soundClass = args.GetInt( "s_soundClass" );

	temp = args.GetString( "s_shader" );
	if ( *temp ) {
		refSound.shader = declHolder.declSoundShaderType.LocalFind( temp );
	}
}

/*
================
idGameEdit::DestroyRenderEntity
================
*/
void idGameEdit::DestroyRenderEntity( renderEntity_t& renderEntity ) {
}

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
}

/*
================
idEntity::idEntity
================
*/
idEntity::idEntity( void ) {

	entityNumber	= ENTITYNUM_NONE;
	entityDefNumber = -1;
	mapSpawnId		= -1;

	spawnNode.SetOwner( this );
	activeNode.SetOwner( this );
	activeNetworkNode.SetOwner( this );
	networkNode.SetOwner( this );
	instanceNode.SetOwner( this );
	interpolateNode.SetOwner( this );

	snapshotPVSFlags	= 0;
	aorFlags			= 0;
	aorPacketSpread		= 0;
	aorDistanceSqr		= 0.0f;

	for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
		freeStates[ i ]	= NULL;
	}

	thinkFlags		= 0;

	physics			= NULL;
	bindMaster		= NULL;
	bindJoint		= INVALID_JOINT;
	bindBody		= -1;
	teamMaster		= NULL;
	teamChain		= NULL;

	scriptObject	= NULL;
	taskName		= NULL;

	fl.notarget				= false;
	fl.noknockback			= false;
	fl.takedamage			= false;
	fl.bindOrientated		= false;
	fl.forceDisableClip		= false;
	fl.preventDeployment	= false;
	fl.selected				= false;
	fl.spotted				= false;
	fl.canCollideWithTeam	= true;
	fl.hidden				= true;
	fl.noCrosshairTrace		= false;
	fl.unlockInterpolate	= false;
	fl.noGuiInteraction		= false;
	fl.dontLink				= false;
	fl.noDamageFeedback		= false;
	fl.allowPredictionErrorDecay = false;
	fl.forceDoorCollision	= false;
	fl.forceDecalUsageLocal  = false;

	occlusionQueryHandle	= -1;

	contentBoundsFilter		= NULL;

	memset( &renderEntity, 0, sizeof( renderEntity ) );
	modelDefHandle	= -1;
	memset( &refSound, 0, sizeof( refSound ) );

	networkNode.AddToEnd( gameLocal.nonNetworkedEntities );

	lastPushedOrigin.Zero();
	lastPushedAxis.Identity();

	bulletTracerStart.Zero();
	bulletTracerEnd.Zero();
}

/*
================
idEntity::Spawn
================
*/
void idEntity::Spawn( void ) {
	gameLocal.RegisterEntity( this );

	const char* classname = spawnArgs.GetString( "classname" );
	if ( *classname ) {
		const idDeclEntityDef *def = declHolder.declEntityDefType.LocalFind( classname, false );
		if ( def ) {
			entityDefNumber = def->Index();
		}
	}

	// parse static models the same way the editor display does
	gameEdit->ParseSpawnArgsToRenderEntity( spawnArgs, renderEntity );

	renderEntity.spawnID = gameLocal.GetSpawnId( this );//.entityNum = entityNumber;
	
	idVec3 origin	= renderEntity.origin;
	idMat3 axis		= renderEntity.axis;
	lastPushedOrigin = origin;
	lastPushedAxis = axis;

	// do the audio parsing the same way dmap and the editor do
	gameEdit->ParseSpawnArgsToRefSound( spawnArgs, refSound );

	// only play SCHANNEL_PRIVATE when sndworld->PlaceListener() is called with this listenerId
	// don't spatialize sounds from the same entity
	refSound.listenerId = entityNumber + 1;

	// every object will have a unique name
	const char* temp = spawnArgs.GetString( "name", va( "%s_%s_%d", GetClassname(), spawnArgs.GetString( "classname" ), entityNumber ) );
	SetName( temp );

	aorLayout = gameLocal.declAORType[ spawnArgs.GetString( "aor_layout", "default" ) ];

	fl.dontLink = spawnArgs.GetBool( "dont_link_clip", "0" );
	InitDefaultPhysics( origin, axis );

	SetPosition( origin, axis );

	temp = spawnArgs.GetString( "model" );
	if ( *temp ) {
		SetModel( temp );
	}

	// auto-start a sound on the entity
	if ( refSound.shader && !refSound.waitfortrigger ) {
		StartSoundShader( refSound.shader, SND_ANY, 0, false, NULL );
	}

	// setup script object
	if ( ShouldConstructScriptObjectAtSpawn() ) {
		scriptObject = gameLocal.program->AllocScriptObject( this, spawnArgs.GetString( "scriptobject", "default" ) );
		ConstructScriptObject();
	}

	fl.noGuiInteraction		= spawnArgs.GetBool( "gui_noninteractive", "0" );
	fl.forceDoorCollision	= spawnArgs.GetBool( "force_door_collision", "0" );
	fl.forceDecalUsageLocal	= spawnArgs.GetBool( "force_decalusagelocal", "0" );

	temp = spawnArgs.GetString( "task_name" );
	taskName = declHolder.declLocStrType.LocalFind( temp, false ); 

	if ( spawnArgs.GetBool( "no_damage_feedback" ) ) {
		fl.noDamageFeedback = true;
	}

	// spawn gui entities
	if ( !networkSystem->IsDedicated() && renderEntity.hModel != NULL ) {
		for ( int i = 0; i < renderEntity.hModel->NumGUISurfaces(); i++ ) {
			const guiSurface_t* guiSurface = renderEntity.hModel->GetGUISurface( i );

			const char* guiName = spawnArgs.GetString( guiSurface->guiNum == 0 ? "gui" : va( "gui%d", guiSurface->guiNum + 1 ) );
			if ( *guiName == '\0' ) {
				continue;
			}
			
			idStr theme = spawnArgs.GetString( guiSurface->guiNum == 0 ? "gui_theme" : va( "gui%d_theme", guiSurface->guiNum + 1 ), "default" );

			guiHandle_t handle = gameLocal.LoadUserInterface( guiName, true, false, theme );

			if ( handle.IsValid() ) {
				sdGuiSurface* cent = reinterpret_cast< sdGuiSurface* >( sdGuiSurface::Type.CreateInstance() );

				cent->Init( this, *guiSurface, handle );

				sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
				if ( ui != NULL ) {
					ui->SetEntity( this );
					ui->Activate();
				}
			}
		}
	}

	// add to occlusion query list
	occlusionQueryHandle = -1;
	if ( spawnArgs.GetBool( "occlusion_query" ) ) {
		occlusionQueryBBScale				= spawnArgs.GetFloat( "occlusion_query_bb_scale", "-0.125" );

		occlusionQueryInfo.axis				= mat3_identity;
		occlusionQueryInfo.bb.Zero();
		occlusionQueryInfo.origin.Zero();
		occlusionQueryInfo.view				= 0;

		occlusionQueryHandle = gameLocal.AddEntityOcclusionQuery( this );
	}

	contentBoundsFilter = gameLocal.declTargetInfoType.LocalFind( spawnArgs.GetString( "ti_content_bounds_filter" ), false );

	memset( interpolateHistory, 0, sizeof(interpolateHistory) );
	interpolateLastFramenum = 0;

	missileRandom.SetSeed( gameLocal.time );

	Show();
	Present();
}

/*
================
idEntity::FixupRemoteCamera
================
*/
void idEntity::FixupRemoteCamera( void ) {
	for ( int i = 0; i < gameLocal.numClients; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( player->GetRemoteCamera() == this ) {
			player->SetRemoteCamera( NULL, true );
		}
	}
}

/*
================
idEntity::~idEntity
================
*/
idEntity::~idEntity( void ) {
	// destroy renderEntity
	gameEdit->DestroyRenderEntity( renderEntity );

	if ( !gameLocal.isClient && IsNetSynced() ) {
		WriteDestroyEvent();
	}

	DeconstructScriptObject();

	if ( thinkFlags ) {
		BecomeInactive( thinkFlags );
	}
	activeNode.Remove();
	activeNetworkNode.Remove();
	interpolateNode.Remove();

	// we have to set back the default physics object before unbinding because the entity
	// specific physics object might be an entity variable and as such could already be destroyed.
	SetPhysics( NULL );

	// unbind from master
	Unbind();
	QuitTeam();

	gameLocal.RemoveEntityFromHash( name.c_str(), this );

	StopSound( SCHANNEL_ANY );
	
	RemoveClientEntities();

	FreeModelDef();

	FreeSoundEmitter( false );

	FreeAllBeams();

	gameLocal.FreeNetworkState( entityNumber );
	for ( int i = 0; i < NSM_NUM_MODES; i++ ) {
		for ( sdEntityState* next = freeStates[ i ]; next != NULL; ) {
			sdEntityState* current = next;
			next = current->next;
			gameLocal.FreeNetworkState( current );
		}
		freeStates[ i ] = NULL;
	}

	if ( occlusionQueryHandle != -1 ) {
		gameLocal.FreeEntityOcclusionQuery( this );
	}

	gameLocal.UnregisterEntity( this );
}

/*
================
idEntity::WriteCreateEvent
================
*/
void idEntity::WriteCreateEvent( idBitMsg* msg, const sdReliableMessageClientInfoBase& target ) {
	if ( gameLocal.isServer || msg != NULL ) {
		sdReliableServerMessage _msg( GAME_RELIABLE_SMESSAGE_CREATE_ENT );
		if ( msg == NULL ) {
			msg = &_msg;
		}

		msg->WriteLong( gameLocal.GetSpawnId( this ) );
		if ( mapSpawnId != -1 ) {
			msg->WriteBits( 0, 2 );
			msg->WriteBits( mapSpawnId, idMath::BitsForInteger( gameLocal.GetNumMapEntities() ) );
		} else if ( entityDefNumber != -1 ) {
			msg->WriteBits( 1, 2 );
			msg->WriteBits( entityDefNumber + 1, gameLocal.GetNumEntityDefBits() );
		} else {
			msg->WriteBits( 2, 2 );
			msg->WriteBits( GetType()->typeNum, idClass::GetTypeNumBits() );
		}

		if ( msg == &_msg ) {
			_msg.Send( target );
		}
	}

	if ( gameLocal.isServer ) {
		gameLocal.CreateNetworkState( entityNumber );
	}
}

/*
================
idEntity::WriteCreationData
================
*/
void idEntity::WriteCreationData( idFile* file ) const {
	file->WriteInt( gameLocal.GetSpawnId( this ) );
	if ( mapSpawnId != -1 ) {
		file->WriteChar( 0 );
		file->WriteInt( mapSpawnId );
	} else if ( entityDefNumber != -1 ) {
		file->WriteChar( 1 );
		file->WriteInt( entityDefNumber );
	} else {
		file->WriteChar( 2 );
		file->WriteInt( GetType()->typeNum );
	}
}

/*
================
idEntity::FromCreationData
================
*/
idEntity* idEntity::FromCreationData( idFile* file ) {
	int temp;
	file->ReadInt( temp );

	int entityNum		= gameLocal.EntityNumForSpawnId( temp );
	int spawnId			= gameLocal.SpawnNumForSpawnId( temp );

	char createType;
	file->ReadChar( createType );

	int index;
	file->ReadInt( index );

	switch ( createType ) {
		case 0: {
			gameLocal.ClientSpawn( entityNum, spawnId, -1, -1, index );
			break;
		}
		case 1: {
			gameLocal.ClientSpawn( entityNum, spawnId, -1, index, -1 );
			break;
		}
		default:
		case 2: {
			gameLocal.ClientSpawn( entityNum, spawnId, index, -1, -1 );
			break;
		}
	}

	return gameLocal.entities[ entityNum ];
}

/*
================
idEntity::LoadDemoBaseData
================
*/
void idEntity::LoadDemoBaseData( idFile* file ) {
	ReadDemoBaseData( file );

	gameLocal.CreateNetworkState( entityNumber );

	networkNode.AddToEnd( gameLocal.networkedEntities );
}


/*
================
idEntity::WriteDestroyEvent
================
*/
void idEntity::WriteDestroyEvent( void ) {
	if ( gameLocal.isClient ) {
		return;
	}

	sdReliableServerMessage outMsg( GAME_RELIABLE_SMESSAGE_DELETE_ENT );
	outMsg.WriteBits( gameLocal.GetSpawnId( this ), 32 );
	outMsg.Send( sdReliableMessageClientInfoAll() );

	gameLocal.FreeNetworkState( entityNumber );
}

/*
================
idEntity::CallNonBlockingScriptEvent
================
*/
void idEntity::CallNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper ) const {
	if ( !function ) {
		return;
	}

	if ( helper.Init( GetScriptObject(), function ) ) {
		helper.Run();
		if ( !helper.Done() ) {
			gameLocal.Error( "idEntity::CallNonBlockingScriptEvent '%s' Cannot be Blocking", function->GetName() );
		}
	}
}

/*
================
idEntity::CallBooleanNonBlockingScriptEvent
================
*/
bool idEntity::CallBooleanNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper ) const {
	if ( !function ) {
		return false;
	}

	if ( helper.Init( GetScriptObject(), function ) ) {
		helper.Run();
		if ( !helper.Done() ) {
			gameLocal.Error( "idEntity::CallBooleanNonBlockingScriptEvent '%s' Cannot be Blocking", function->GetName() );
		}
	} else {
		return false;
	}
	return helper.GetReturnedInteger() != 0;
}

/*
================
idEntity::CallFloatNonBlockingScriptEvent
================ 
*/
float idEntity::CallFloatNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper ) const {
	if ( !function ) {
		return 0.0f;
	}

	if ( helper.Init( GetScriptObject(), function ) ) {
		helper.Run();
		if ( !helper.Done() ) {
			gameLocal.Error( "idEntity::CallFloatNonBlockingScriptEvent '%s' Cannot be Blocking", function->GetName() );
		}
	} else {
		return 0.0f;
	}
	return helper.GetReturnedFloat();
}

/*
================
idEntity::GetEntityDefName
================
*/
const char* idEntity::GetEntityDefName( void ) const {
	if ( entityDefNumber < 0 ) {
		return "*unknown*";
	}
	return declHolder.declEntityDefType.LocalFindByIndex( entityDefNumber, false )->GetName();
}

/*
================
idEntity::SetName
================
*/
void idEntity::SetName( const char *newname ) {
	if ( name.Length() ) {
		gameLocal.RemoveEntityFromHash( name.c_str(), this );
	}

	name = newname;
	if ( name.Length() ) {
		if ( ( name == "NULL" ) || ( name == "null_entity" ) ) {
			gameLocal.Error( "Cannot name entity '%s'.  '%s' is reserved for script.", name.c_str(), name.c_str() );
		}
		gameLocal.AddEntityToHash( name.c_str(), this );
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


/***********************************************************************

	Thinking
	
***********************************************************************/

/*
================
idEntity::Think
================
*/
void idEntity::Think( void ) {
	RunPhysics();
	if ( gameLocal.isNewFrame ) {
		Present();
	}
}

/*
================
idEntity::IsActive
================
*/
bool idEntity::IsActive( void ) const {
	return activeNode.InList();
}

/*
================
idEntity::BecomeActive
================
*/
void idEntity::BecomeActive( int flags, bool force ) {
	flags &= ~thinkFlags;
	if ( !flags ) {
		return;
	}

	if ( flags & TH_PHYSICS ) {
		if ( teamMaster != NULL ) {
			if ( teamMaster != this ) {
				if ( !force ) {
					flags &= ~TH_PHYSICS;
				}
			} else {
				idEntity* next = NULL;
				for ( idEntity* other = teamChain; other; other = next ) {
					next = other->teamChain;

					if ( other == this ) {
						continue;
					}

					other->BecomeActive( TH_PHYSICS, true );
				}
			}
		}

		if ( flags & TH_PHYSICS ) {
			// if this is a pusher
			if ( physics->IsType( idPhysics_Actor::Type ) ) {
				gameLocal.sortPushers = true;
			}
		}
	}

	thinkFlags |= flags;

	if ( !IsActive() ) {
		gameLocal.PushChangedEntity( this );
	}
}

/*
================
idEntity::BecomeInactive
================
*/
void idEntity::BecomeInactive( int flags, bool force ) {
	flags &= thinkFlags;
	if ( !flags ) {
		return;
	}

	if ( flags & TH_PHYSICS ) {
		if ( teamMaster != NULL ) {
			if ( teamMaster != this ) {
				if ( !force ) {
					flags &= ~TH_PHYSICS;
				}
			} else {
				idEntity* next = NULL;
				for ( idEntity* other = teamChain; other; other = next ) {
					next = other->teamChain;

					if ( other == this ) {
						continue;
					}

					other->BecomeInactive( TH_PHYSICS, true );
				}
			}
		}
	}

	thinkFlags &= ~flags;

	if ( !thinkFlags ) {
		if ( IsActive() ) {
			gameLocal.PushChangedEntity( this );
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
void idEntity::SetShaderParm( int parmnum, float value ) {
	if ( ( parmnum < 0 ) || ( parmnum >= MAX_ENTITY_SHADER_PARMS ) ) {
		gameLocal.Warning( "shader parm index (%d) out of range", parmnum );
		return;
	}

	if ( renderEntity.shaderParms[ parmnum ] != value ) {
		renderEntity.shaderParms[ parmnum ] = value;
		BecomeActive( TH_UPDATEVISUALS );

	}
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( float red, float green, float blue ) {
	renderEntity.shaderParms[ SHADERPARM_RED ]		= red;
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= green;
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= blue;
	UpdateVisuals();
}

/*
================
idEntity::SetColor
================
*/
void idEntity::SetColor( const idVec3 &color ) {
	SetColor( color[ 0 ], color[ 1 ], color[ 2 ] );
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

	renderEntity.hModel = renderModelManager->FindModel( modelname );

	if ( renderEntity.hModel != NULL ) {
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
void idEntity::SetSkin( const idDeclSkin* skin ) {
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
idEntity::FreeModelDef
================
*/
void idEntity::FreeModelDef( void ) {
	if ( modelDefHandle != -1 ) {
		gameRenderWorld->RemoveDecals( modelDefHandle );
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
idEntity::Hide
================
*/
void idEntity::Hide( void ) {
	if ( !IsHidden() ) {
		fl.hidden = true;
		FreeModelDef();
	}
}

/*
================
idEntity::Show
================
*/
void idEntity::Show( void ) {
	if ( IsHidden() ) {
		fl.hidden = false;
		UpdateVisuals();
	}
}

/*
================
idEntity::UpdateModelTransform
================
*/
void idEntity::UpdateModelTransform( void ) {
	idVec3 origin;
	idMat3 axis;
	idPhysics* _physics = GetPhysics();

	if ( GetPhysicsToVisualTransform( origin, axis ) ) {
		renderEntity.axis = axis * _physics->GetAxis();
		renderEntity.origin = _physics->GetOrigin() + origin * renderEntity.axis;
	} else {
		renderEntity.axis = _physics->GetAxis();
		renderEntity.origin = _physics->GetOrigin();
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
	if ( animator && animator->ModelHandle() ) {
		// set the callback to update the joints
		renderEntity.callback = idEntity::ModelCallback;
	}

	// ensure that we call Present this frame
	BecomeActive( TH_UPDATEVISUALS );
}

/*
================
idEntity::UpdateVisuals
================
*/
void idEntity::UpdateVisuals( void ) {
	UpdateModel();
	UpdateSound();
	OnUpdateVisuals();
}

/*
================
idEntity::OnBindMasterVisChanged
================
*/
void idEntity::OnBindMasterVisChanged() {
	if ( bindMaster != NULL ) {
		const renderEntity_t* masterRenderEnt = bindMaster->GetRenderEntity();
		assert( masterRenderEnt );
		// copy the master's suppression flags
		renderEntity.suppressShadowInViewID = masterRenderEnt->suppressShadowInViewID;
		renderEntity.suppressSurfaceInViewID = masterRenderEnt->suppressSurfaceInViewID;
	} else {
		// assume that it should be visible - this should be correct in 99.99999999999% of all cases!
		renderEntity.suppressShadowInViewID = 0;
		renderEntity.suppressSurfaceInViewID = 0;
	}

	UpdateVisuals();
}

/*
================
idEntity::NotifyVisChanged
================
*/
void idEntity::NotifyVisChanged() {
	// notify everyone in the team that the visibility of the owner has changed
	idEntity* next;
	for( idEntity* ent = teamChain; ent != NULL; ent = next ) {
		next = ent->teamChain;
		
		ent->OnBindMasterVisChanged();
	} 
}

/*
================
idEntity::OnUpdateVisuals
================
*/
void idEntity::OnUpdateVisuals( void ) {
}

/*
================
idEntity::SetNetworkSynced
================
*/
void idEntity::SetNetworkSynced( bool value ) {
	if ( value == IsNetSynced() ) {
		return;
	}

	if ( IsSpawning() ) {
		gameLocal.Warning( "idEntity::SetNetworkSynced Cannot Set Sync State During Spawning" );
		return;
	}

	if ( gameLocal.isClient ) {
		gameLocal.PushChangedEntity( this );
	}

	if ( value ) {
		networkNode.AddToEnd( gameLocal.networkedEntities );
		WriteCreateEvent( NULL, sdReliableMessageClientInfoAll() );
	} else {
		networkNode.AddToEnd( gameLocal.nonNetworkedEntities );
		WriteDestroyEvent();
	}
}

/*
================
idEntity::PostMapSpawn
================
*/
void idEntity::PostMapSpawn() {
	FindTargets();
}

/*
================
idEntity::Present

Present is called to allow entities to generate refEntities, lights, etc for the renderer.
================
*/
void idEntity::Present( void ) {
	if ( !gameLocal.isNewFrame ) {
		return;
	}

	// don't present to the renderer if the entity hasn't changed
	if ( !( thinkFlags & TH_UPDATEVISUALS ) ) {
		return;
	}
	BecomeInactive( TH_UPDATEVISUALS );

	if ( gameLocal.com_unlockFPS->GetBool() && g_unlock_interpolateMoving.GetBool() ) {
		// it should be possible to keep entities back one frame and record their positions while we can't unlock frames
		// have to watch out not to do that to an entity we're bound to however (like riding the icarus)
		// without constantly keeping origins one frame behind, some might jump a bit when we go in and out of unlock mode
		if ( !gameLocal.unlock.unlockedDraw && gameLocal.unlock.canUnlockFrames ) {
			// piggyback the AOR INHIBIT_IK setting to trigger the interpolation
			if ( fl.unlockInterpolate && ( aorFlags & AOR_INHIBIT_IK ) == 0 ) {
				if ( !interpolateNode.InList() ) {
					interpolateNode.AddToFront( gameLocal.interpolateEntities );
				}
				if ( gameLocal.framenum - interpolateLastFramenum != 1 ) {
					// that entity is starting interpolation, stick to the current position this frame
					interpolateHistory[0] = interpolateHistory[1] = renderEntity.origin;
				} else {
					// note: this works if there are multiple calls, it keeps storing latest origin and setting previous one
					interpolateHistory[ gameLocal.framenum & 1 ] = renderEntity.origin;
					renderEntity.origin = interpolateHistory[ ( gameLocal.framenum + 1 ) & 1 ];
				}
				interpolateLastFramenum = gameLocal.framenum;
			}
		}
	}

	// if set to invisible, skip
	if ( renderEntity.hModel && !IsHidden() ) {

		if ( renderSystem->IsSMPEnabled() ) {
			idAnimator *animator = GetAnimator();
			if ( animator ) {		
				if ( animator->CreateFrame( gameLocal.time, false ) ) {
				}
			}
		}

		// add to refresh list
		if ( modelDefHandle == -1 ) {
			modelDefHandle = gameRenderWorld->AddEntityDef( &renderEntity );
			OnModelDefCreated();
		} else {
			gameRenderWorld->UpdateEntityDef( modelDefHandle, &renderEntity );
		}
	}

	lastPushedOrigin = renderEntity.origin;
	lastPushedAxis = renderEntity.axis;

	if ( gameLocal.unlock.unlockedDraw ) {
		// manually update all client entities bound to us during unlock
		for ( rvClientEntity* cent = clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {
			cent->ClientUpdateView();
		}
	}

	OnUpdateVisuals();
}

/*
================
idEntity::GetModelDefHandle
================
*/
int idEntity::GetModelDefHandle( int id ) {
	return modelDefHandle;
}

/*
================
idEntity::UpdateRenderEntity
================
*/
bool idEntity::UpdateRenderEntity( renderEntity_t *renderEntity, const renderView_t *renderView, int& lastGameModifiedTime ) {
	idAnimator *animator = GetAnimator();
	if ( animator ) {		
		if ( animator->CreateFrame( gameLocal.time, false ) || lastGameModifiedTime != animator->GetTransformCount() ) {
			lastGameModifiedTime = animator->GetTransformCount();
			return true;
		}
	}

	return false;
}

/*
================
idEntity::ModelCallback

	NOTE: may not change the game state whatsoever!
================
*/
bool idEntity::ModelCallback( renderEntity_t *renderEntity, const renderView_t *renderView, int& lastGameModifiedTime ) {
	idEntity* ent = gameLocal.EntityForSpawnId( renderEntity->spawnID );//gameLocal.entities[ renderEntity->entityNum ];
	if ( !ent ) {
		//gameLocal.Warning( "idEntity::ModelCallback: callback with NULL game entity" );
		return false;
	}

	if ( !renderSystem->IsSMPEnabled() ) {
		if ( ent->UpdateRenderEntity( renderEntity, renderView, lastGameModifiedTime ) ) {
			return true;
		}
	}

	return false;
}

/*
=============
idEntity::GetRenderView

This is used by remote camera views to look from an entity
=============
*/
renderView_t *idEntity::GetRenderView( void ) {
	return NULL;
}

/***********************************************************************

  effects
	
***********************************************************************/

/*
================
idEntity::PlayEffect
================
*/
rvClientEffect* idEntity::PlayEffect( const int effectHandle, const idVec3& color, jointHandle_t joint, bool loop, const idVec3& endOrigin ) {
	if ( !gameLocal.DoClientSideStuff() ) { 
		return NULL;
	}

	if ( joint == INVALID_JOINT ) {
		gameLocal.Warning( "idEntity::PlayEffect Invalid Joint" );
		return NULL;
	}
//	assert ( joint != INVALID_JOINT );	

	if ( effectHandle < 0 ) {
		return NULL;
	}

	rvClientEffect* effect = new rvClientEffect ( effectHandle );
	effect->SetOrigin( vec3_origin );
	effect->SetAxis( mat3_identity );
	effect->Bind( this, joint );
	effect->SetGravity( gameLocal.GetGravity() );
	effect->SetMaterialColor( color );
	
	if ( !effect->Play ( gameLocal.time, loop, endOrigin ) ) {
		delete effect;
		return NULL;
	}
	
	return effect;
}

rvClientEffect* idEntity::PlayEffectMaxVisDist( const int effectHandle, const idVec3& color, jointHandle_t joint, bool loop, bool isStatic, float maxVisDist, const idVec3& endOrigin ) {
	if ( !gameLocal.DoClientSideStuff() ) { 
		return NULL;
	}

	if ( joint == INVALID_JOINT ) {
		gameLocal.Warning( "idEntity::PlayEffect Invalid Joint" );
		return NULL;
	}
//	assert ( joint != INVALID_JOINT );	

	if ( effectHandle < 0 ) {
		return NULL;
	}

	rvClientEffect* effect = new rvClientEffect ( effectHandle );
	effect->SetOrigin( vec3_origin );
	effect->SetAxis( mat3_identity );
	effect->Bind( this, joint );
	effect->SetGravity( gameLocal.GetGravity() );
	effect->SetMaterialColor( color );
	effect->SetMaxVisDist( maxVisDist );
	effect->GetRenderEffect()->isStatic = isStatic;
	
	if ( !effect->Play ( gameLocal.time, loop, endOrigin ) ) {
		delete effect;
		return NULL;
	}
	
	return effect;
}

/*
============
idEntity::PlayEffect
============
*/
rvClientEffect* idEntity::PlayEffect ( const int effectHandle, const idVec3& color, const idVec3& origin, const idMat3& axis, bool loop, const idVec3& endOrigin, bool viewsuppress ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		return NULL;
	}

	idVec3 localOrigin;
	idMat3 localAxis;
	
	// jrad - the isNewFrame check causes issues in network games for effects and sounds played via scripts
	// since this is primarily how we play effects and sounds now, this check is removed
	if ( effectHandle < 0 /*|| !gameLocal.isNewFrame */ ) {
		return NULL;
	}

	if ( entityNumber == ENTITYNUM_WORLD ) {
		return gameLocal.PlayEffect( effectHandle, color, origin, axis, loop, endOrigin );
	}

	// Calculate the local origin and axis from the given globals
	localOrigin = ( origin - renderEntity.origin ) * renderEntity.axis.Transpose();
	localAxis   = axis * renderEntity.axis.Transpose();

	rvClientEffect* effect = new rvClientEffect ( effectHandle );
	effect->SetOrigin ( localOrigin );
	effect->SetAxis ( localAxis );
	effect->SetMaterialColor( color );
	effect->Bind ( this );
	effect->SetViewSuppress( viewsuppress );
	effect->SetGravity( gameLocal.GetGravity() );
	if ( !effect->Play ( gameLocal.time, loop, endOrigin ) ) {
		delete effect;
		return NULL;
	}
	
	return effect;
}

rvClientEffect* idEntity::PlayEffectMaxVisDist ( const int effectHandle, const idVec3& color, const idVec3& origin, const idMat3& axis, bool loop, bool isStatic, float maxVisDist, const idVec3& endOrigin ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		return NULL;
	}

	idVec3 localOrigin;
	idMat3 localAxis;
	
	// jrad - the isNewFrame check causes issues in network games for effects and sounds played via scripts
	// since this is primarily how we play effects and sounds now, this check is removed
	if ( effectHandle < 0 /*|| !gameLocal.isNewFrame */ ) {
		return NULL;
	}

	if ( entityNumber == ENTITYNUM_WORLD ) {
		return gameLocal.PlayEffect( effectHandle, color, origin, axis, loop, endOrigin );
	}

	// Calculate the local origin and axis from the given globals
	localOrigin = ( origin - renderEntity.origin ) * renderEntity.axis.Transpose();
	localAxis   = axis * renderEntity.axis.Transpose();

	rvClientEffect* effect = new rvClientEffect ( effectHandle );
	effect->SetOrigin ( localOrigin );
	effect->SetAxis ( localAxis );
	effect->SetMaterialColor( color );
	effect->Bind ( this );
	effect->SetGravity( gameLocal.GetGravity() );
	effect->SetMaxVisDist( maxVisDist );
	effect->GetRenderEffect()->isStatic = isStatic;

	if ( !effect->Play ( gameLocal.time, loop, endOrigin ) ) {
		delete effect;
		return NULL;
	}
	
	return effect;
}

/*
================
idEntity::StopAllEffects
================
*/
void idEntity::StopAllEffects ( bool destroyParticles ) {
	rvClientEntity* next;

	for( rvClientEntity* cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();

		rvClientEffect* effect = cent->Cast< rvClientEffect >();
		if ( effect ) {
			effect->Stop( destroyParticles );
		}
	}		
}

/*
================
idEntity::StopEffect
================
*/
void idEntity::StopEffect( int effectIndex, bool destroyParticles ) {	
	rvClientEntity* next;
	for ( rvClientEntity* cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();

		rvClientEffect* effect = cent->Cast< rvClientEffect >();
		if ( effect == NULL ) {
			continue;
		}
		
		if ( effect->GetEffectIndex() == effectIndex ) {
			effect->Stop( destroyParticles );
		}
	}
}

void idEntity::StopEffect( const char* effectName, bool destroyParticles ) {
	StopEffect( gameLocal.GetEffectHandle ( spawnArgs, effectName, NULL ), destroyParticles );
}

/*
================
idEntity::StopEffectHandle
================
*/
void idEntity::StopEffectHandle( unsigned int handle, bool destroyParticles ) {	
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		effect.GetEntity()->Stop( destroyParticles );
	}
}

/*
================
idEntity::UnBindEffectHandle
================
*/
void idEntity::UnBindEffectHandle( unsigned int handle ) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );

	rvClientEffect* effectEnt = effect;
	if ( effectEnt != NULL ) {
		effectEnt->Unbind();
	}
}

/*
================
idEntity::Event_SetEffectRenderBounds
================
*/
void idEntity::Event_SetEffectRenderBounds( int handle, bool renderBounds ) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		effect.GetEntity()->SetRenderBounds( renderBounds );
	}
}

/*
================
idEntity::Event_GetEffectOrigin
================
*/
void idEntity::Event_GetEffectOrigin( int handle ) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		sdProgram::ReturnVector( effect.GetEntity()->GetOrigin() );
	}
}

/*
================
idEntity::Event_GetEffectEndOrigin
================
*/
void idEntity::Event_GetEffectEndOrigin( int handle ) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		sdProgram::ReturnVector( effect.GetEntity()->GetEndOrigin() );
	}
}

/*
================
idEntity::SetEffectAttenuation
================
*/
void idEntity::SetEffectAttenuation( unsigned int handle, float attenuation ) {	
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		effect.GetEntity()->Attenuate( attenuation );
	}
}

/*
================
idEntity::SetEffectColor
================
*/
void idEntity::SetEffectColor( unsigned int handle, const idVec4& color ) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		effect->SetColor( color );
	}
}

/*
================
idEntity::SetEffectOrigin
================
*/
void idEntity::SetEffectOrigin( unsigned int handle, const idVec3& origin ) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		effect->SetOrigin( origin );
	}
}

/*
================
idEntity::SetEffectAxis
================
*/
void idEntity::SetEffectAxis( unsigned int handle, const idMat3& axis) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		effect->SetAxis( axis );
	}
}

/*
================
idEntity::Event_AddCheapDecal
================
*/
void idEntity::Event_AddCheapDecal( idEntity *attachTo, idVec3 &origin, idVec3 &normal, const char* decalName, const char* materialName ) {
	gameLocal.AddCheapDecal( spawnArgs, attachTo, origin, normal, INVALID_JOINT, 0, decalName, materialName );
}

/*
================
idEntity::Event_GetEntityNumber
================
*/
void idEntity::Event_GetEntityNumber( void ) {
	sdProgram::ReturnInteger( entityNumber );
}

/*
================
idEntity::Event_DisableCrosshairTrace
================
*/
void idEntity::Event_DisableCrosshairTrace( bool value ) {
	fl.noCrosshairTrace	= value;
}

/***********************************************************************

  Sound
	
***********************************************************************/

/*
================
idEntity::StartSound
================
*/
bool idEntity::StartSound( const char *soundName, const soundChannel_t channel, int soundShaderFlags, int *length ) {
	return StartSound( soundName, channel, channel, soundShaderFlags, length );
}

/*
================
idEntity::StartSound
================
*/
bool idEntity::StartSound( const char *soundName, const soundChannel_t channelStart, const soundChannel_t channelEnd, int soundShaderFlags, int *length ) {
	const idSoundShader *shader;
	const char *sound;

	if ( length ) {
		*length = 0;
	}

	// we should ALWAYS be playing sounds from the def.
	// hardcoded sounds MUST be avoided at all times because they won't get precached.
	assert( idStr::Icmpn( soundName, "snd_", 4 ) == 0 );

	if ( !spawnArgs.GetString( soundName, "", &sound ) ) {
		return false;
	}

	if ( !*sound ) {
		return false;
	}

	shader = declHolder.declSoundShaderType.LocalFind( sound );
	return StartSoundShader( shader, channelStart, channelEnd, soundShaderFlags, length );
}

/*
================
idEntity::StartSoundShader
================
*/
bool idEntity::StartSoundShader( const idSoundShader *shader, const soundChannel_t channel, int soundShaderFlags, int *length ) {
	return StartSoundShader( shader, channel, channel, soundShaderFlags, length );
}

/*
================
idEntity::StartSoundShader
================
*/
bool idEntity::StartSoundShader( const idSoundShader *shader, const soundChannel_t channelStart, const soundChannel_t channelEnd, int soundShaderFlags, int *length ) {

	if ( length ) {
		*length = 0;
	}

	if ( !shader ) {
		return false;
	}

	// set a random value for diversity unless one was parsed from the entity
	float diversity;
	if ( refSound.diversity < 0.0f ) {
		diversity = gameLocal.random.RandomFloat();
	} else {
		diversity = refSound.diversity;
	}

	// if we don't have a soundEmitter allocated yet, get one now
	if ( !refSound.referenceSound ) {
		refSound.referenceSound = gameSoundWorld->AllocSoundEmitter();
#if 0
		for ( int i = 0; i < gameLocal.numEntities; i++ ) {
			if ( i == entityNumber ) {
				continue;
			}
			if ( gameLocal.entities[ i ] != NULL ) {
				assert( gameLocal.entities[ i ]->refSound.referenceSound != refSound.referenceSound );
			}
		}
#endif // 0
	}

	UpdateSound();

	int len = refSound.referenceSound->StartSound( shader, channelStart, channelEnd, diversity, soundShaderFlags );
	if ( length ) {
		*length = len;
	}

	// set reference to the sound for shader synced effects
	renderEntity.referenceSound = refSound.referenceSound;

	return true;
}

/*
================
idEntity::SetChannelOffset
================
*/
void idEntity::SetChannelOffset( const soundChannel_t channel, int ms ) {
	assert( refSound.referenceSound );

	refSound.referenceSound->SetChannelOffset( channel, ms );
}

/*
================
idEntity::StopSound
================
*/
void idEntity::StopSound( const soundChannel_t channel ) {
	if ( refSound.referenceSound ) {
		refSound.referenceSound->StopSound( channel );
	}
}

/*
================
idEntity::SetChannelPitchShift
================
*/
void idEntity::SetChannelPitchShift( const soundChannel_t channel, float pitchshift ) {
	if ( !refSound.referenceSound ) {
		return;
	}

	soundShaderParms_t parms = refSound.referenceSound->GetChannelParms( channel );

	parms.pitchShift = pitchshift;

	refSound.referenceSound->ModifySound( channel, parms );
}

/*
================
idEntity::SetChannelFlags
================
*/
void idEntity::SetChannelFlags( const soundChannel_t channel, int flags ) {
	if ( !refSound.referenceSound ) {
		return;
	}

	soundShaderParms_t parms = refSound.referenceSound->GetChannelParms( channel );

	parms.soundShaderFlags |= flags;

	refSound.referenceSound->ModifySound( channel, parms );
}

/*
================
idEntity::ClearChannelFlags
================
*/
void idEntity::ClearChannelFlags( const soundChannel_t channel, int flags ) {
	if ( !refSound.referenceSound ) {
		return;
	}

	soundShaderParms_t parms = refSound.referenceSound->GetChannelParms( channel );

	parms.soundShaderFlags &= ~flags;

	refSound.referenceSound->ModifySound( channel, parms );
}

/*
================
idEntity::SetChannelVolume
================
*/
void idEntity::SetChannelVolume( const soundChannel_t channel, float volume ) {
	if( !refSound.referenceSound ) {
		return;
	}

	soundShaderParms_t parms = refSound.referenceSound->GetChannelParms( channel );

	parms.volume = volume;
	refSound.referenceSound->ModifySound( channel, parms );
}

/*
================
idEntity::SetSoundVolume

  Must be called before starting a new sound.
================
*/
void idEntity::SetSoundVolume( float volume ) {
	refSound.parms.volume = volume;
}

/*
================
idEntity::UpdateSound
================
*/
void idEntity::UpdateSound( void ) {
	if ( !gameLocal.isNewFrame ) {
		// don't bother updating the emitter during reprediction
		return;
	}

	if ( refSound.referenceSound ) {
		idVec3 origin;
		idMat3 axis;

		if ( GetPhysicsToSoundTransform( origin, axis ) ) {
			refSound.origin = GetPhysics()->GetOrigin() + origin * axis;
		} else {
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
	if ( refSound.referenceSound != NULL ) {
		refSound.referenceSound->Free( immediate );
		refSound.referenceSound = NULL;
	}
}

/*
================
idEntity::FadeSound
================
*/
void idEntity::FadeSound( const soundChannel_t channel, float to, float over ) {
	if ( refSound.referenceSound ) {
		refSound.referenceSound->FadeSound( channel, to, over );
	}
}


/***********************************************************************

  client entities
	
***********************************************************************/

/*
================
idEntity::RemoveClientEntities
================
*/
void idEntity::RemoveClientEntities ( void ) {
	rvClientEntity* cent;
	rvClientEntity* next;

	// jrad - keep client effect entities around so they can finish thinking/evaluating
	for( cent = clientEntities.Next(); cent != NULL; cent = next ) {
		next = cent->bindNode.Next();

		rvClientEffect* effect = cent->Cast< rvClientEffect >();
		if ( effect ) {			
			effect->Stop();
			continue;
		}

		cent->Dispose();
	}
	clientEntities.Clear();
}

/***********************************************************************

  entity binding
	
***********************************************************************/

/*
================
idEntity::PreBind
================
*/
void idEntity::PreBind( void ) {
}

/*
================
idEntity::PostBind
================
*/
void idEntity::PostBind( void ) {
}

/*
================
idEntity::PreUnbind
================
*/
void idEntity::PreUnbind( void ) {
}

/*
================
idEntity::PostUnbind
================
*/
void idEntity::PostUnbind( void ) {
}

/*
================
idEntity::IsNetSynced
================
*/
bool idEntity::IsNetSynced( void ) const {
	return networkNode.ListHead() == &gameLocal.networkedEntities;
}

/*
================
idEntity::IsInterpolated
================
*/
bool idEntity::IsInterpolated( void ) const {
	return interpolateNode.ListHead() == &gameLocal.interpolateEntities;
}

/*
================
idEntity::InitBind
================
*/
bool idEntity::InitBind( idEntity *master ) {

	if ( master == this ) {
		gameLocal.Error( "Tried to bind an object to itself." );
		return false;
	}

	if ( this == gameLocal.world ) {
		gameLocal.Error( "Tried to bind world to another entity" );
		return false;
	}

	if ( master != NULL ) {
		if ( master->GetBindMaster() == this ) {
			gameLocal.Warning( "Tried to bind entity to an entity which is bound to it" );
			return false;
		}
	} else {
		gameLocal.Warning( "idEntity::InitBind Tried to Bind to NULL" );
	}

	// unbind myself from my master
	Unbind();

	if ( !master || master == gameLocal.world ) {
		// this can happen in scripts, so safely exit out.
		return false;
	}

	return true;
}

/*
================
idEntity::FinishBind
================
*/
void idEntity::FinishBind( void ) {

	// set the master on the physics object
	physics->SetMaster( bindMaster, fl.bindOrientated );

	// We are now separated from our previous team and are either
	// an individual, or have a team of our own.  Now we can join
	// the new bindMaster's team.  Bindmaster must be set before
	// joining the team, or we will be placed in the wrong position
	// on the team.
	JoinTeam( bindMaster );

	// make sure the team master is active so that physics get run
	teamMaster->BecomeActive( TH_PHYSICS );
	BecomeActive( TH_PHYSICS, true );
}

/*
================
idEntity::Bind

  bind relative to the visual position of the master
================
*/
void idEntity::Bind( idEntity *master, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();

	OnBindMasterVisChanged();
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

	if ( !InitBind( master ) ) {
		return;
	}

	masterAnimator = master->GetAnimator();
	if ( !masterAnimator ) {
		gameLocal.Warning( "idEntity::BindToJoint: entity '%s' cannot support skeletal models.", master->GetName() );
		return;
	}

	jointnum = masterAnimator->GetJointHandle( jointname );
	if ( jointnum == INVALID_JOINT ) {
		gameLocal.Warning( "idEntity::BindToJoint: joint '%s' not found on entity '%s'.", jointname, master->GetName() );
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

//	master->GetWorldOriginAxis( bindJoint, bindOrg, bindAxis );

	FinishBind();

	PostBind();
}

/*
================
idEntity::BindToJoint

  bind relative to a joint of the md5 model used by the master
================
*/
void idEntity::BindToJoint( idEntity *master, jointHandle_t jointnum, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	PreBind();

	bindJoint = jointnum;
	bindBody = -1;
	bindMaster = master;
	fl.bindOrientated = orientated;

//	master->GetWorldOriginAxis( bindJoint, bindOrg, bindAxis );

	FinishBind();

	PostBind();
}

/*
================
idEntity::BindToBody

  bind relative to a collision model used by the physics of the master
================
*/
void idEntity::BindToBody( idEntity *master, int bodyId, bool orientated ) {

	if ( !InitBind( master ) ) {
		return;
	}

	if ( bodyId < 0 ) {
		gameLocal.Warning( "idEntity::BindToBody: body '%d' not found.", bodyId );
	}

	PreBind();

	bindJoint = INVALID_JOINT;
	bindBody = bodyId;
	bindMaster = master;
	fl.bindOrientated = orientated;

	FinishBind();

	PostBind();
}

/*
================
idEntity::Unbind
================
*/
void idEntity::Unbind( void ) {
	idEntity *	prev;
	idEntity *	next;
	idEntity *	last;
	idEntity *	ent;

	if ( !bindMaster ) {
		return;
	}

	if ( !teamMaster ) {
		// Teammaster already has been freed
		bindMaster = NULL;
		return;
	}

	PreUnbind();

	if ( physics ) {
		physics->SetMaster( NULL, fl.bindOrientated );
	}

	// We're still part of a team, so that means I have to extricate myself
	// and any entities that are bound to me from the old team.
	// Find the node previous to me in the team
	prev = teamMaster;
	for( ent = teamMaster->teamChain; ent && ( ent != this ); ent = ent->teamChain ) {
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
	if ( teamMaster != this ) {
		prev->teamChain = next;
		if ( !next && ( teamMaster == prev ) ) {
			prev->teamMaster = NULL;
		}
	} else if ( next ) {
		// If we were the teamMaster, then the nodes that were not bound to me are now
		// a disconnected chain.  Make them into their own team.
		for( ent = next; ent->teamChain != NULL; ent = ent->teamChain ) {
			ent->teamMaster = next;
		}
		next->teamMaster = next;
	}

	// If we don't have anyone on our team, then clear the team variables.
	if ( teamChain ) {
		// make myself my own team
		teamMaster = this;
	} else {
		// no longer a team
		teamMaster = NULL;
	}

	bindJoint = INVALID_JOINT;
	bindBody = -1;
	bindMaster = NULL;

	PostUnbind();

	OnBindMasterVisChanged();
}

/*
================
idEntity::RemoveBinds
================
*/
void idEntity::RemoveBinds( idBounds* bounds, bool onlyUnbindScripted ) {
	idEntity* next;
	for( idEntity* ent = teamChain; ent != NULL; ent = next ) {
		next = ent->teamChain;
		if ( ent->bindMaster == this ) {
			if ( bounds != NULL && !bounds->IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) ) {
				continue;
			}

			idScriptObject* scriptObject = ent->GetScriptObject();
			bool doUnbind = !onlyUnbindScripted;
			if ( scriptObject ) {
				const sdProgram::sdFunction* function = scriptObject->GetFunction( "OnUnbind" );
				if ( function ) {
					sdScriptHelper h1;
					scriptObject->CallNonBlockingScriptEvent( function, h1 );
					doUnbind = true;
				}
			}
			if ( doUnbind ) {
				ent->Unbind();
				next = teamChain;
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
=====================
idEntity::ConvertLocalToWorldTransform
=====================
*/
void idEntity::ConvertLocalToWorldTransform( idVec3& offset ) {
	UpdateModelTransform();

	offset = renderEntity.origin + offset * renderEntity.axis;
}

/*
=====================
idEntity::ConvertLocalToWorldTransform
=====================
*/
void idEntity::ConvertLocalToWorldTransform( idMat3& axis ) {
	UpdateModelTransform();

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
idEntity::DistanceTo2d
================
*/
float idEntity::DistanceTo2d ( const idVec3& pos ) {
	idVec3 pos1;
	idVec3 pos2;
	pos1 = pos - (pos * GetPhysics()->GetGravityNormal ( )) * GetPhysics()->GetGravityNormal ( );
	pos2 = GetPhysics()->GetOrigin ( );
	pos2 = pos2 - (pos2 * GetPhysics()->GetGravityNormal ( )) * GetPhysics()->GetGravityNormal ( );
	return (pos2 - pos1).LengthFast ( );
}

/*
================
idEntity::GetLocalAngles
================
*/
void idEntity::GetLocalAngles(idAngles &localAng) 
{
	idVec3 localVec = GetPhysics()->GetAxis()[0];

	GetLocalVector(localVec);
	localAng = localVec.ToAngles();
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
	idVec3		localOrigin;
	idMat3		localAxis;

	if ( bindMaster ) {
		// if bound to a joint of an animated model
		if ( bindJoint != INVALID_JOINT ) {
			bindMaster->GetWorldOriginAxis( bindJoint, masterOrigin, masterAxis );
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

/*
================
idEntity::JoinTeam
================
*/
void idEntity::JoinTeam( idEntity *teammember ) {
	idEntity *ent;
	idEntity *master;
	idEntity *prev;
	idEntity *next;

	// if we're already on a team, quit it so we can join this one
	if ( teamMaster && ( teamMaster != this ) ) {
		QuitTeam();
	}

	assert( teammember );

	if ( teammember == this ) {
		teamMaster = this;
		return;
	}

	// check if our new team mate is already on a team
	master = teammember->teamMaster;
	if ( !master ) {
		// he's not on a team, so he's the new teamMaster
		master = teammember;
		teammember->teamMaster = teammember;
		teammember->teamChain = this;

		// make anyone who's bound to me part of the new team
		for( ent = teamChain; ent != NULL; ent = ent->teamChain ) {
			ent->teamMaster = master;
		}
	} else {
		// skip past the chain members bound to the entity we're teaming up with
		prev = teammember;
		next = teammember->teamChain;
		if ( bindMaster ) {
			// if we have a bindMaster, join after any entities bound to the entity
			// we're joining
			while( next && next->IsBoundTo( teammember ) ) {
				prev = next;
				next = next->teamChain;
			}
		} else {
			// if we're not bound to someone, then put us at the end of the team
			while( next ) {
				prev = next;
				next = next->teamChain;
			}
		}

		// make anyone who's bound to me part of the new team and
		// also find the last member of my team
		for( ent = this; ent->teamChain != NULL; ent = ent->teamChain ) {
			ent->teamChain->teamMaster = master;
		}

    	prev->teamChain = this;
		ent->teamChain = next;
	}

	teamMaster = master;

	// reorder the active entity list 
	gameLocal.sortTeamMasters = true;
}

/*
================
idEntity::IsInRadar
================
*/
bool idEntity::IsInRadar( idPlayer* player ) {
	sdTeamInfo* team = player->GetGameTeam();
	if ( team ) {
		sdTeamInfo* localTeam = GetGameTeam();
		if ( localTeam ) {
			if ( localTeam->PointInJammer( GetPhysics()->GetOrigin(), RM_RADAR ) ) {
				return false;
			}
		}

		if ( team->PointInRadar( GetPhysics()->GetOrigin(), RM_RADAR ) ) {
			return true;
		}
	}

	return false;
}

/*
================
idEntity::SetSpotted
================
*/
void idEntity::SetSpotted( idEntity* spotter ) {
	if ( fl.spotted ) {
		return;
	}

	fl.spotted = true;

	idScriptObject* scriptObject = GetScriptObject();
	if ( scriptObject ) {
		sdScriptHelper h;
		h.Push( spotter->GetScriptObject() );
		scriptObject->CallNonBlockingScriptEvent( scriptObject->GetFunction( "OnSpotted" ), h );
	}
}

/*
================
idEntity::SendCommandMapInfo
================
*/
bool idEntity::SendCommandMapInfo( idPlayer* player ) {
	if ( player == NULL ) {
		return false;
	}

	if ( fl.spotted ) {
		return true;
	}

	teamAllegiance_t allegiance = GetEntityAllegiance( player );
	if ( allegiance == TA_FRIEND ) {
		return true;
	}

	if ( IsInRadar( player ) ) {
		return true;
	}

	return false;
}

/*
================
idEntity::QuitTeam
================
*/
void idEntity::QuitTeam( void ) {
	idEntity *ent;

	if ( !teamMaster ) {
		return;
	}

	// check if I'm the teamMaster
	if ( teamMaster == this ) {
		// do we have more than one teammate?
		if ( !teamChain->teamChain ) {
			// no, break up the team
			teamChain->teamMaster = NULL;
		} else {
			// yes, so make the first teammate the teamMaster
			for( ent = teamChain; ent; ent = ent->teamChain ) {
				ent->teamMaster = teamChain;
			}
		}
	} else {
		assert( teamMaster );
		assert( teamMaster->teamChain );

		// find the previous member of the teamChain
		ent = teamMaster;
		while( ent->teamChain != this ) {
			assert( ent->teamChain ); // this should never happen
			ent = ent->teamChain;
		}

		// remove this from the teamChain
		ent->teamChain = teamChain;

		// if no one is left on the team, break it up
		if ( !teamMaster->teamChain ) {
			teamMaster->teamMaster = NULL;
		}
	}

	teamMaster = NULL;
	teamChain = NULL;
}

/***********************************************************************

  Physics.
	
***********************************************************************/


/*
============
idEntity::InitDefaultClipModel
============
*/
idClipModel* idEntity::InitDefaultClipModel() {
	if ( spawnArgs.GetBool( "noClipModel" ) ) {
		return NULL;
	}

	// check if mins/maxs or size key/value pairs are set
	idVec3 size;
	idBounds bounds;
	bool setClipModel = false;

	if ( spawnArgs.GetVector( "mins", NULL, bounds[ 0 ] ) && spawnArgs.GetVector( "maxs", NULL, bounds[ 1 ] ) ) {
		setClipModel = true;

		if ( bounds[0][0] > bounds[1][0] || bounds[0][1] > bounds[1][1] || bounds[0][2] > bounds[1][2] ) {
			gameLocal.Error( "Invalid bounds '%s'-'%s' on entity '%s'", bounds[0].ToString(), bounds[1].ToString(), name.c_str() );
		}

	} else if ( spawnArgs.GetVector( "size", NULL, size ) ) {
		if ( ( size.x < 0.0f ) || ( size.y < 0.0f ) || ( size.z < 0.0f ) ) {
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

			idAngles coneAngles = spawnArgs.GetAngles( "cone_angles" );
			trm.Rotate( coneAngles.ToMat3() );
		} else {
			trm.SetupBox( bounds );
		}
		return new idClipModel( trm, false );
	}

	const char* clipModelName = GetClipModelName();
	if ( *clipModelName ) {
		return new idClipModel( clipModelName );
	}

	return NULL;
}

/*
================
idEntity::InitDefaultPhysics
================
*/
void idEntity::InitDefaultPhysics( const idVec3 &origin, const idMat3 &axis ) {
	
	idClipModel* clipModel = InitDefaultClipModel();

	defaultPhysicsObj.SetSelf( this );
	defaultPhysicsObj.SetClipModel( clipModel, 1.0f );
	defaultPhysicsObj.SetOrigin( origin );
	defaultPhysicsObj.SetAxis( axis );

	physics = &defaultPhysicsObj;
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
idEntity::DisableClip
================
*/
void idEntity::DisableClip( bool activateContacting ) {
	physics->DisableClip( activateContacting );
}

/*
================
idEntity::EnableClip
================
*/
void idEntity::EnableClip( void ) {
	if ( fl.forceDisableClip ) {
		return;
	}
	physics->EnableClip();
}

/*
================
idEntity::RunPhysics
================
*/
bool idEntity::RunPhysics( void ) {
	int			i, reachedTime, startTime, endTime;
	idEntity *	part, *blockedPart, *blockingEntity;
	trace_t		results;
	bool		moved;

	// if this entity is a team slave don't do anything because the team master will handle everything
	if ( teamMaster && teamMaster != this ) {
		return false;
	}

	if ( gameLocal.isClient && IsPhysicsInhibited() ) {		
		return false;
	}

	startTime = gameLocal.previousTime;
	endTime = gameLocal.time;
	if ( gameLocal.IsPaused() ) {
		if ( RunPausedPhysics() ) { // Gordon: Hack to let spectators move in pause
			startTime = endTime - USERCMD_MSEC;
		}
	}

	gameLocal.push.InitSavingPushedEntityPositions();
	blockedPart = NULL;

	// save the physics state of the whole team and disable the team for collision detection
	for ( part = this; part != NULL; part = part->teamChain ) {
		part->physics->SaveState();
	}

	// move the whole team
	for ( part = this; part != NULL; part = part->teamChain ) {
		// run physics
		moved = part->physics->Evaluate( endTime - startTime, endTime );
		part->fl.allowPredictionErrorDecay = true;

		// check if the object is blocked
		blockingEntity = part->physics->GetBlockingEntity();
		if ( blockingEntity ) {
			blockedPart = part;
			break;
		}

		// if moved or forced to update the visual position and orientation from the physics
		if ( moved ) {
			part->UpdateVisuals();
		}

		// update any animation controllers here so an entity bound
		// to a joint of this entity gets the correct position
		if ( part->UpdateAnimationControllers() ) {
			part->BecomeActive( TH_ANIMATE );
		}
	}

	// if one of the team entities is a pusher and blocked
	if ( blockedPart ) {
		// move the parts back to the previous position
		for ( part = this; part != blockedPart; part = part->teamChain ) {
			// restore the physics state
			part->physics->RestoreState();

			// move back the visual position and orientation
			part->UpdateVisuals();
		}
		for ( part = this; part != NULL; part = part->teamChain ) {
			// update the physics time without moving
			part->physics->UpdateTime( endTime );
		}

		// restore the positions of any pushed entities
		gameLocal.push.RestorePushedEntityPositions();

		if ( !gameLocal.isClient ) {
			OnTeamBlocked( blockedPart, blockingEntity );
			blockedPart->OnPartBlocked( blockingEntity );
		}

		return false;
	}

	// set pushed
	for ( i = 0; i < gameLocal.push.GetNumPushedEntities(); i++ ) {
		idEntity* ent = gameLocal.push.GetPushedEntity( i );
		ent->physics->SetPushed( endTime - startTime );
	}

	// post reached event if the current time is at or past the end point of the motion
	for ( part = this; part != NULL; part = part->teamChain ) {
		reachedTime = part->physics->GetLinearEndTime();
		if ( startTime < reachedTime && endTime >= reachedTime ) {
			part->ReachedPosition();
		}
	}

	return true;
}

/*
================
idEntity::SetOrigin
================
*/
void idEntity::SetOrigin( const idVec3 &org ) {
	GetPhysics()->SetOrigin( org );
	ResetPredictionErrorDecay();
	UpdateVisuals();
	Present();
}

/*
================
idEntity::SetAxis
================
*/
void idEntity::SetAxis( const idMat3 &axis ) {
	GetPhysics()->SetAxis( axis );
	ResetPredictionErrorDecay();
	UpdateVisuals();
	Present();
}

/*
================
idEntity::GetAxis
================
*/
const idMat3 &idEntity::GetAxis( void ) {
	return GetPhysics()->GetAxis();
}

/*
================
idEntity::SetPosition
================
*/
void idEntity::SetPosition( const idVec3 &org, const idMat3 &axis ) {
	GetPhysics()->SetOrigin( org );
	GetPhysics()->SetAxis( axis );
	ResetPredictionErrorDecay();
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
idEntity::GetWorldOrigin
================
*/
bool idEntity::GetWorldOrigin( jointHandle_t joint, idVec3& org ) {
	UpdateModelTransform();

	if ( !GetAnimator()->GetJointTransform( joint, gameLocal.time, org ) ) {
		return false;
	}
	org = renderEntity.origin + ( org * renderEntity.axis );
	return true;
}

/*
================
idEntity::GetWorldAxis
================
*/
bool idEntity::GetWorldAxis( jointHandle_t joint, idMat3& axis ) {
	UpdateModelTransform();

	if ( !GetAnimator()->GetJointTransform( joint, gameLocal.time, axis ) ) {
		return false;
	}
	axis *= renderEntity.axis;
	return true;
}

/*
================
idEntity::GetWorldOriginAxis
================
*/
bool idEntity::GetWorldOriginAxis( jointHandle_t joint, idVec3& org, idMat3& axis ) {
	UpdateModelTransform();

	if ( !GetAnimator()->GetJointTransform( joint, gameLocal.time, org, axis ) ) {
		return false;
	}
	org = renderEntity.origin + ( org * renderEntity.axis );
	axis *= renderEntity.axis;
	return true;
}

/*
================
idEntity::GetWorldOriginAxisNoUpdate
================
*/
bool idEntity::GetWorldOriginAxisNoUpdate( jointHandle_t joint, idVec3& org, idMat3& axis ) {
	if ( !GetAnimator()->GetJointTransform( joint, gameLocal.time, org, axis ) ) {
		return false;
	}
	org = renderEntity.origin + ( org * renderEntity.axis );
	axis *= renderEntity.axis;
	return true;
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
bool idEntity::Collide( const trace_t &collision, const idVec3 &velocity, int bodyId ) {
	// this entity collides with collision.c.entityNum
	return false;
}

/*
================
idEntity::CollideFatal
================
*/
void idEntity::CollideFatal( idEntity* other ) {
}

/*
================
idEntity::Hit

	This is the symmetrical part of the "Collide" member when Collide gets called on the hitter
	Hit gets called on the hitee
================
*/
void idEntity::Hit( const trace_t &collision, const idVec3 &velocity, idEntity *other ) {
	// this entity was hit by other

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
	GetPhysics()->ApplyImpulse( id, point, impulse );
}

/*
================
idEntity::AddForce
================
*/
void idEntity::AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force ) {
	GetPhysics()->AddForce( id, point, force );
}

/*
================
idEntity::ActivatePhysics
================
*/
void idEntity::ActivatePhysics( void ) {
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



/***********************************************************************

	Damage
	
***********************************************************************/

/*
============
idEntity::ApplyRadiusPush
============
*/
void idEntity::ApplyRadiusPush( const idVec3& pushOrigin, const idVec3& entityOrigin, const sdDeclDamage* damageDecl, float pushScale, float radius ) {
	// scale the push for the inflictor
	// scale by the push scale of the entity
	idVec3 impulse = entityOrigin - pushOrigin;
	float dist = impulse.Normalize();
	if ( dist > radius ) {
		return;
	}

	float distScale = Square( 1.0f - dist / radius );
	impulse.z += 1.0f;
	float scale = distScale * pushScale * GetRadiusPushScale();
	if ( damageDecl != NULL ) {
		scale *= damageDecl->GetPush();
	}
	if ( idMath::Fabs( scale ) < idMath::FLT_EPSILON ) {
		return;
	}

	// push the force application point closer to the entity so it causes less rotation
	GetPhysics()->AddForce( 0, ( pushOrigin + entityOrigin ) * 0.5f, impulse * scale );
}

/*
============
idEntity::CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
bool idEntity::CanDamage( const idVec3 &origin, idVec3 &damagePoint, int mask, idEntity* passEntity, trace_t* _tr ) const {
	idVec3 	dest;	
	idVec3 	midpoint;

	trace_t		__tr;
	trace_t&	tr = _tr ? *_tr : __tr;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin at 0,0,0
	midpoint = ( GetPhysics()->GetAbsBounds()[0] + GetPhysics()->GetAbsBounds()[1] ) * 0.5;

	dest = midpoint;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, origin, dest, mask, passEntity );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	// this should probably check in the plane of projection, rather than in world coordinate
	dest = midpoint;
	dest[0] += 15.0;
	dest[1] += 15.0;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, origin, dest, mask, passEntity );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] += 15.0;
	dest[1] -= 15.0;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, origin, dest, mask, passEntity );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] += 15.0;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, origin, dest, mask, passEntity );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, origin, dest, mask, passEntity );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] += 15.0;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, origin, dest, mask, passEntity );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	dest = midpoint;
	dest[2] -= 15.0;
	gameLocal.clip.TracePoint( CLIP_DEBUG_PARMS tr, origin, dest, mask, passEntity );
	if ( tr.fraction == 1.0 || ( gameLocal.GetTraceEntity( tr ) == this ) ) {
		damagePoint = tr.endpos;
		return true;
	}

	return false;
}

/*
============
idEntity::CheckTeamDamage
============
*/
bool idEntity::CheckTeamDamage( idEntity *inflictor, const sdDeclDamage* damageDecl ) {
	if ( inflictor == this ) {
		return true;
	}

	if ( !si_teamDamage.GetBool() && !damageDecl->GetNoTeam() && !gameLocal.rules->IsWarmup() ) {
		if ( GetEntityAllegiance( inflictor ) == TA_FRIEND ) {
			return false;
		}
	}

	return true;
}

/*
============
idEntity::Damage

this		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ

	example: this=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback in global space
collision	point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted

inflictor, attacker, dir, and collision can be NULL for environmental effects

============
*/
void idEntity::Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const sdDeclDamage* damageDecl, const float damageScale, const trace_t* collision, bool forceKill ) {
	if ( !fl.takedamage ) {
		return;
	}

	if ( attacker != this ) {
		if ( !CheckTeamDamage( inflictor, damageDecl ) && !forceKill ) {
			return;
		}
	}

	if ( !inflictor ) {
		inflictor = gameLocal.world;
	}

	if ( !attacker ) {
		attacker = gameLocal.world;
	}

	bool noScale;
	float damage = damageDecl->GetDamage( this, noScale );
	if ( !noScale ) {
		damage *= damageScale;
	}

	if ( forceKill ) {
		damage = GetHealth();
	}

	if ( damage < idMath::FLT_EPSILON ) {
		return;
	}

	// do the damage

	if ( gameLocal.isClient ) {
		Pain( inflictor, attacker, damage, dir, JOINTHANDLE_FOR_TRACE( collision ), damageDecl );
		return;
	}

	int oldHealth = GetHealth();

	int newHealth = oldHealth - damage;

	SetHealthDamaged( newHealth, damageDecl->IsTeamDamage() ? TA_FRIEND : inflictor->GetEntityAllegiance( this ) );

	SetLastAttacker( attacker, damageDecl );

	// inform the attacker that they hit someone
	attacker->DamageFeedback( this, inflictor, oldHealth, newHealth, damageDecl, false );

	if ( attacker != gameLocal.world && attacker->entityNumber < MAX_CLIENTS && attacker->entityNumber > -1 ) {
		if ( this->Cast< idPlayer >() == NULL && this->Cast< sdTransport >() == NULL ) {
			botThreadData.GetGameWorldState()->clientInfo[ attacker->entityNumber ].lastAttackedEntity = entityNumber;
			botThreadData.GetGameWorldState()->clientInfo[ attacker->entityNumber ].lastAttackedEntityTime = gameLocal.time;
		}
	}

	if ( newHealth <= 0 ) {
		if ( newHealth < -999 ) {
			SetHealth( -999 );
		}
		if ( oldHealth > 0 ) {
			Killed( inflictor, attacker, damage, dir, JOINTHANDLE_FOR_TRACE( collision ), damageDecl );
		}

		return;
	}

	Pain( inflictor, attacker, damage, dir, JOINTHANDLE_FOR_TRACE( collision ), damageDecl );
}


/*
============
idEntity::Pain

Called whenever an entity receives damage.  Returns whether the entity responds to the pain.
This is a virtual function that subclasses are expected to implement.
============
*/
bool idEntity::Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
	return false;
}

/*
============
idEntity::Killed

Called whenever an entity's health is reduced to 0 or less.
This is a virtual function that subclasses are expected to implement.
============
*/
void idEntity::Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl ) {
}

/*
============
idEntity::CanCollide
============
*/
bool idEntity::CanCollide( const idEntity* other, int traceId ) const {
	if ( fl.noCrosshairTrace && traceId == TM_CROSSHAIR_INFO ) {
		return false;
	}

	if ( !fl.canCollideWithTeam ) {
		if ( GetEntityAllegiance( other ) == TA_FRIEND ) {
			return false;
		}
	} 
	
	return other != this;
}

/*
============
idEntity::ShouldProxyCollide
============
*/
bool idEntity::ShouldProxyCollide( const idEntity* other ) const {
	return false;
}

/*
============
idEntity::CheckForDuplicateRenderEntityHandles
============
*/
void idEntity::CheckForDuplicateRenderEntityHandles( void ) {
	idList< renderEntity_t* > checkList;

	for ( int i = 0; i < MAX_GENTITIES; i++ ) {
		idEntity* entity = gameLocal.entities[ i ];
		if ( entity == NULL ) {
			continue;
		}

		int handle = entity->modelDefHandle;
		if ( handle == -1 ) {
			continue;
		}

		checkList.AssureSize( handle + 1, NULL );
		if ( checkList[ handle ] != NULL ) {
			gameLocal.Warning( "Duplicate Render Entity Handle" );
		}
		
		checkList[ handle ] = &entity->renderEntity;
	}

	int handle;

	handle = footPrintManager->GetModelHandle();
	if ( handle != -1 ) {
		checkList.AssureSize( handle + 1, NULL );
		if ( checkList[ handle ] != NULL ) {
			gameLocal.Warning( "Duplicate Render Entity Handle" );
		}
		checkList[ handle ] = footPrintManager->GetRenderEntity();
	}

	sdInstanceCollector< sdMiscFlare > flares( true );
	for ( int i = 0; i < flares.Num(); i++ ) {
		sdHardcodedParticleFlare& system = flares[ i ]->GetFlareSystem();

		handle = system.GetModelHandle();
		if ( handle != -1 ) {
			checkList.AssureSize( handle + 1, NULL );
			if ( checkList[ handle ] != NULL ) {
				gameLocal.Warning( "Duplicate Render Entity Handle" );
			}
			checkList[ handle ] = &system.GetRenderEntity();
		}
	}
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
	return !spawnArgs.GetBool( "noscriptobject", "0" );
}

/*
================
idEntity::ConstructScriptObject

Called during idEntity::Spawn.  Calls the constructor on the script object.
Can be overridden by subclasses when a thread doesn't need to be allocated.
================
*/
sdProgramThread* idEntity::ConstructScriptObject( void ) {
	if ( !scriptObject ) {
		return NULL;
	}

	// init the script object's data
	scriptObject->ClearObject();

	sdScriptHelper h1, h2;
	CallNonBlockingScriptEvent( scriptObject->GetSyncFunc(), h1 );
	CallNonBlockingScriptEvent( scriptObject->GetPreConstructor(), h2 );

	sdProgramThread* thread = NULL;

	if ( idStr::Cmp( scriptObject->GetTypeName(), "default" ) != 0 ) { // Gordon: assume the default type doesn't have a constructor
		// call script object's constructor
		const sdProgram::sdFunction* constructor = scriptObject->GetConstructor();
		if ( constructor ) {
			// start a thread that will initialize after Spawn is done being called
			thread = gameLocal.program->CreateThread();
			thread->SetName( name.c_str() );
			thread->CallFunction( scriptObject, constructor );
			thread->DelayedStart( 0 );
		}
	}

	return thread;
}

/*
================
idEntity::DeconstructScriptObject
================
*/
void idEntity::DeconstructScriptObject( void ) {
	RemoveBinds();
	FixupRemoteCamera();

	if ( !scriptObject ) {
		return;
	}

	// call script object's destructor
	sdScriptHelper h;
	CallNonBlockingScriptEvent( scriptObject->GetDestructor(), h );

	gameLocal.program->FreeScriptObject( scriptObject );
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
	int			i;

	// targets can be a list of multiple names
	gameLocal.GetTargets( spawnArgs, targets, "target" );

	// ensure that we don't target ourselves since that could cause an infinite loop when activating entities
	for( i = 0; i < targets.Num(); i++ ) {
		if ( targets[ i ].GetEntity() == this ) {
			gameLocal.Error( "Entity '%s' is targeting itself", name.c_str() );
		}
	}
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

/***********************************************************************

  Misc.
	
***********************************************************************/

/*
================
idEntity::Event_StopAllEffects
================
*/
void idEntity::Event_StopAllEffects ( void ) {
	StopAllEffects ( );
}

/*
================
idEntity::Event_DetachRotationBind
================
*/
void idEntity::Event_DetachRotationBind( int handle ) {
	rvClientEntityPtr< rvClientEffect > effect;
	effect.ForceSpawnId( handle );
	if ( effect.GetEntity() ) {
		effect.GetEntity()->EnableAxisBind( false );
	}
}

/*
================
idEntity::Event_PlayMaterialEffect
================
*/
void idEntity::Event_PlayMaterialEffect( const char *effectName, const idVec3& color, const char* jointName, const char* materialType, bool loop ) {
	jointHandle_t joint;
	rvClientEntityPtr< rvClientEffect > eff;

	joint = GetAnimator() ? GetAnimator()->GetJointHandle( jointName ) : INVALID_JOINT;
	if ( joint != INVALID_JOINT ) {
		eff = PlayEffect( effectName, color, materialType, joint, loop );
	} else {				  
		eff = PlayEffect( effectName, color, materialType, renderEntity.origin, renderEntity.axis, loop );
	}	

	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

void idEntity::Event_PlayMaterialEffectMaxVisDist( const char *effectName, const idVec3& color, const char* jointName, const char* materialType, bool loop, float maxVisDist, bool isStatic ) {
	jointHandle_t joint;
	rvClientEntityPtr< rvClientEffect > eff;

	joint = GetAnimator() ? GetAnimator()->GetJointHandle( jointName ) : INVALID_JOINT;
	if ( joint != INVALID_JOINT ) {
		eff = PlayEffectMaxVisDist( effectName, color, materialType, joint, loop, isStatic, maxVisDist );
	} else {				  
		eff = PlayEffectMaxVisDist( effectName, color, materialType, renderEntity.origin, renderEntity.axis, loop, isStatic, maxVisDist );
	}	

	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

/*
================
idEntity::Event_PlayEffect
================
*/
void idEntity::Event_PlayEffect( const char *effectName, const char* jointName, bool loop ) {
	Event_PlayMaterialEffect( effectName, colorWhite.ToVec3(), jointName, NULL, loop );
}

/*
============
idEntity::Event_PlayEffectMaxVisDist
============
*/
void idEntity::Event_PlayEffectMaxVisDist( const char *effectName, const char* jointName, bool loop, float maxVisDist, bool isStatic ) {
	Event_PlayMaterialEffectMaxVisDist( effectName, colorWhite.ToVec3(), jointName, NULL, loop, maxVisDist, isStatic );
}

/*
================
idEntity::Event_PlayJointEffect

TODO: Play on origin of an invalid handle is passed in like the normal play effect
================
*/
void idEntity::Event_PlayJointEffect( const char *effectName, jointHandle_t joint, bool loop ) {
	rvClientEntityPtr< rvClientEffect > eff;
	eff = PlayEffect( effectName, colorWhite.ToVec3(), NULL, joint, loop );
	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

/*
================
idEntity::Event_PlayJointEffectViewSuppress

TODO: Play on origin of an invalid handle is passed in like the normal play effect
================
*/
void idEntity::Event_PlayJointEffectViewSuppress( const char* effectName, jointHandle_t joint, bool loop, bool suppress ) {
	rvClientEntityPtr< rvClientEffect > eff;
	eff = PlayEffect( effectName, colorWhite.ToVec3(), NULL, joint, loop );

	if ( eff.IsValid() ) {
		eff->SetViewSuppress( suppress );
	}

	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

/*
============
idEntity::Event_PlayOriginEffect
============
*/
void idEntity::Event_PlayOriginEffect( const char* effectName, const char* materialType, const idVec3& origin, const idVec3& forward, bool loop ) {
	rvClientEntityPtr< rvClientEffect > eff;
	eff = PlayEffect( effectName, colorWhite.ToVec3(), materialType, origin, forward.ToMat3(), loop );
	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

void idEntity::Event_PlayOriginEffectMaxVisDist( const char* effectName, const char* materialType, const idVec3& origin, const idVec3& forward, bool loop, float maxVisDist, bool isStatic ) {
	rvClientEntityPtr< rvClientEffect > eff;
	eff = PlayEffectMaxVisDist( effectName, colorWhite.ToVec3(), materialType, origin, forward.ToMat3(), loop, isStatic, maxVisDist );
	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

/*
============
idEntity::Event_PlayBeamEffect
============
*/
void idEntity::Event_PlayBeamEffect( const char* effectName, const char* materialType, const idVec3& origin, const idVec3& endOrigin, bool loop ) {
	rvClientEntityPtr< rvClientEffect > eff;	
	eff = PlayEffect( effectName, colorWhite.ToVec3(), materialType, origin, mat3_identity, loop, endOrigin );	// FIXME: mat3_identity to dir.ToMat3() ?
	sdProgram::ReturnHandle( eff.GetSpawnId() );
}

/*
================
idEntity::Event_StopEffect
================
*/
void idEntity::Event_StopEffect( const char *effectName ) {
	StopEffect( effectName );
}

/*
================
idEntity::Event_KillEffect
================
*/
void idEntity::Event_KillEffect( const char *effectName ) {
	StopEffect( effectName, true );
}

/*
================
idEntity::Event_StopEffectHandle
================
*/
void idEntity::Event_StopEffectHandle( int handle ) {
	StopEffectHandle( handle );
}

/*
================
idEntity::Event_SetEffectOrigin
================
*/
void idEntity::Event_SetEffectOrigin( int handle, const idVec3& origin ) {
	SetEffectOrigin( handle, origin );
}

/*
================
idEntity::Event_SetEffectAngles
================
*/
void idEntity::Event_SetEffectAngles( int handle, const idAngles& angles ) {
	SetEffectAxis( handle, angles.ToMat3() );
}

/*
================
idEntity::Event_UnBindEffectHandle
================
*/
void idEntity::Event_UnBindEffectHandle( int handle ) {
	UnBindEffectHandle( handle );
}

/*
================
idEntity::Event_SetEffectAttenuation
================
*/
void idEntity::Event_SetEffectAttenuation( int handle, float attenuation ) {
	SetEffectAttenuation( handle, attenuation );
}

/*
================
idEntity::Event_SetEffectColor
================
*/
void idEntity::Event_SetEffectColor( int handle, const idVec3& color, float alpha ) {
	SetEffectColor( handle, idVec4( color.x, color.y, color.z, alpha ) );
}

/*
============
idEntity::Event_SpawnClientEffect
============
*/
void idEntity::Event_SpawnClientEffect( const char* effectName ) {
	int effectHandle = gameLocal.GetEffectHandle( spawnArgs, effectName, NULL );	

	if( effectHandle != -1 ) {
		rvClientEffect* effect = new rvClientEffect( effectHandle );
		effect->Create( NULL, gameLocal.program->GetDefaultType() );
		effect->SetOrigin( vec3_zero );
		effect->SetAxis( mat3_identity );
		effect->SetGravity( gameLocal.GetGravity() );

		if( effect->Play( gameLocal.time ) ) {
			sdProgram::ReturnObject( effect->GetScriptObject() );
			return;
		}

		delete effect;
	}

	sdProgram::ReturnObject( NULL );
	return;
}

/*
============
idEntity::Event_SpawnClientCrawlEffect
============
*/
void idEntity::Event_SpawnClientCrawlEffect( const char* effectName, idEntity* ent, float crawlTime ) {
	int effectHandle = gameLocal.GetEffectHandle( spawnArgs, effectName, NULL );	

	if( effectHandle != -1 ) {
		rvClientCrawlEffect* effect = new rvClientCrawlEffect( effectHandle, ent, SEC2MS( crawlTime ) );
		if( effect->Play( gameLocal.time ) ) {
			sdProgram::ReturnObject( effect->GetScriptObject() );
			return;
		}

		delete effect;
	}

	sdProgram::ReturnObject( NULL );
	return;
}

/*
================
idEntity::UpdateCrosshairInfo
================
*/
bool idEntity::UpdateCrosshairInfo( idPlayer* player, sdCrosshairInfo& info ) {
	return false;
}

/*
================
idEntity::Teleport
================
*/
void idEntity::Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination ) {
	GetPhysics()->SetOrigin( origin );
	GetPhysics()->SetAxis( angles.ToMat3() );

	ResetPredictionErrorDecay();
	UpdateVisuals();
}

/*
============
idEntity::TouchTriggers

  Activate all trigger entities touched at the current position.
============
*/
bool idEntity::TouchTriggers( void ) {
	int				i, numClipModels, numEntities;
	const idClipModel*	cm;
	const idClipModel*	clipModels[ MAX_GENTITIES ];
	idEntity*		ent;
	trace_t			trace;

	numClipModels = gameLocal.clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS GetPhysics()->GetAbsBounds(), CONTENTS_TRIGGER, clipModels, MAX_GENTITIES, this );
	numEntities = 0;
	
	if ( !numClipModels ) {
		return false;
	}

	memset( &trace, 0, sizeof( trace ) );
	trace.endpos = GetPhysics()->GetOrigin();
	trace.endAxis = GetPhysics()->GetAxis();

	for ( i = 0; i < numClipModels; i++ ) {
		cm = clipModels[ i ];
		ent = cm->GetEntity();

		if ( !ent->WantsTouch() ) {
			continue;
		}

		if ( !GetPhysics()->ClipContents( cm ) ) {
			continue;
		}

		numEntities++;

		trace.c.contents = cm->GetContents();
		trace.c.entityNum = cm->GetEntity()->entityNumber;
		trace.c.id = cm->GetId();

		ent->OnTouch( this, trace );

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

	idStr str = kv->GetKey().Right( kv->GetKey().Length() - idStr::Length( curveTag ) );
	if ( str.Icmp( "CatmullRomSpline" ) == 0 ) {
		spline = new idCurve_CatmullRomSpline<idVec3>();
	} else if ( str.Icmp( "nubs" ) == 0 ) {
		spline = new idCurve_NonUniformBSpline<idVec3>();
	} else if ( str.Icmp( "nurbs" ) == 0 ) {
		spline = new idCurve_NURBS<idVec3>();
	} else {
		spline = new idCurve_BSpline<idVec3>();
	}

	spline->SetBoundaryType( idCurve_Spline<idVec3>::BT_CLAMPED );

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
	sdProgram::ReturnString( name.c_str() );
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
================
idEntity::Event_BindToJoint
================
*/
void idEntity::Event_BindToJoint( idEntity *master, const char *jointname, bool orientated ) {
	BindToJoint( master, jointname, ( orientated != 0.0f ) );
}

/*
================
idEntity::Event_RemoveBinds
================
*/
void idEntity::Event_RemoveBinds( void ) {
	RemoveBinds();
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
	if( !*skinname ) {
		SetSkin( NULL );
	} else {
		SetSkin( gameLocal.declSkinType[ skinname ] );
	}
}

/*
================
idEntity::Event_SetCoverage
================
*/
void idEntity::Event_SetCoverage( float v ) {
	renderEntity.coverage = idMath::ClampFloat( 0.f, 1.f, v );
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

	sdProgram::ReturnFloat( renderEntity.shaderParms[ parmnum ] );
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
idEntity::Event_GetColor
================
*/
void idEntity::Event_GetColor( void ) {
	idVec3 out;

	GetColor( out );
	sdProgram::ReturnVector( out );
}

/*
================
idEntity::Event_IsHidden
================
*/
void idEntity::Event_IsHidden( void ) {
	sdProgram::ReturnBoolean( fl.hidden );
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
idEntity::Event_StartSound
================
*/
void idEntity::Event_StartSound( const char *soundName, int channel ) {
	int time;
	StartSound( soundName, channel, channel, 0, &time );
	sdProgram::ReturnFloat( MS2SEC( time ) );
}

/*
================
idEntity::Event_StopSound
================
*/
void idEntity::Event_StopSound( int channel ) {
	StopSound( channel );
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
idEntity::Event_SetChannelPitchShift
================
*/
void idEntity::Event_SetChannelPitchShift( const soundChannel_t channel, float shift ) {
	SetChannelPitchShift( channel, shift );
}

/*
================
idEntity::Event_SetChannelFlags
================
*/
void idEntity::Event_SetChannelFlags( const soundChannel_t channel, int flags ) {
	SetChannelFlags( channel, flags );
}

/*
================
idEntity::Event_ClearChannelFlags
================
*/
void idEntity::Event_ClearChannelFlags( const soundChannel_t channel, int flags ) {
	ClearChannelFlags( channel, flags );
}

/*
================
idEntity::Event_GetWorldAxis
================
*/
void idEntity::Event_GetWorldAxis( int index ) {
	sdProgram::ReturnVector( GetPhysics()->GetAxis()[ index ] );
}

/*
================
idEntity::Event_SetWorldAxis
================
*/
void idEntity::Event_SetWorldAxis( const idVec3& fwd, const idVec3& right, const idVec3& up ) {
	idMat3 axis( fwd, right, up );
	GetPhysics()->SetAxis( axis );
}

/*
================
idEntity::Event_GetWorldOrigin
================
*/
void idEntity::Event_GetWorldOrigin( void ) {
	sdProgram::ReturnVector( GetPhysics()->GetOrigin() );
}

/*
================
idEntity::Event_GetGravityNormal
================
*/
void idEntity::Event_GetGravityNormal( void ) {
	sdProgram::ReturnVector( GetPhysics()->GetGravityNormal() );
}

/*
================
idEntity::Event_SetWorldOrigin
================
*/
void idEntity::Event_SetWorldOrigin( const idVec3 &org ) {
	idVec3 neworg = GetLocalCoordinates( org );
	SetOrigin( neworg );
}

/*
================
idEntity::Event_SetOrigin
================
*/
void idEntity::Event_SetOrigin( const idVec3 &org ) {
	SetOrigin( org );
}

/*
================
idEntity::Event_GetOrigin
================
*/
void idEntity::Event_GetOrigin( void ) {
	sdProgram::ReturnVector( GetLocalCoordinates( GetPhysics()->GetOrigin() ) );
}

/*
================
idEntity::Event_SetAngles
================
*/
void idEntity::Event_SetAngles( const idAngles& ang ) {
	SetAngles( ang );
}

/*
================
idEntity::Event_SetGravity
================
*/
void idEntity::Event_SetGravity( const idVec3& gravity ) {
	GetPhysics()->SetGravity( gravity );
}

/*
================
idEntity::Event_AlignToAxis
================
*/
void idEntity::Event_AlignToAxis( const idVec3& vec, int axis ) {
	idMat3 out;
	switch ( axis ) {
	default:
		return;
	case 2:
		vec.ToMat3( out );
		Swap( out[ 0 ], out[ 2 ] );
		Swap( out[ 0 ], out[ 1 ] );
		break;
	case 1:
		vec.ToMat3( out );
		Swap( out[ 0 ], out[ 1 ] );
		Swap( out[ 0 ], out[ 2 ] );
		break;
	case 0:
		vec.ToMat3( out );
		break;
	}
	SetAxis( out );
}

/*
================
idEntity::Event_GetAngles
================
*/
void idEntity::Event_GetAngles( void ) {
	idAngles ang = GetPhysics()->GetAxis().ToAngles();
	sdProgram::ReturnVector( idVec3( ang[0], ang[1], ang[2] ) );
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
	sdProgram::ReturnVector( GetPhysics()->GetLinearVelocity() );
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
	sdProgram::ReturnVector( GetPhysics()->GetAngularVelocity() );
}

/*
================
idEntity::Event_GetMass
================
*/
void idEntity::Event_GetMass( void ) {
	sdProgram::ReturnFloat( GetPhysics()->GetMass() );
}

/*
================
idEntity::Event_GetCenterOfMass
================
*/
void idEntity::Event_GetCenterOfMass( void ) {
	sdProgram::ReturnVector( GetPhysics()->GetCenterOfMass() );
}

/*
================
idEntity::Event_SetFriction
================
*/
void idEntity::Event_SetFriction( float linear, float angular, float contact ) {
	// Gordon: FIXME: This is awful, just make this virtual
	idPhysics* physics = GetPhysics();
	idPhysics_RigidBody* rigidBody = physics->Cast< idPhysics_RigidBody >();
	if ( rigidBody != NULL ) {
		rigidBody->SetFriction( linear, angular, contact );
		return;
	}

	sdPhysics_RigidBodyMultiple* rigidBodyMult = physics->Cast< sdPhysics_RigidBodyMultiple >();
	if ( rigidBodyMult != NULL ) {
		rigidBodyMult->SetFriction( linear, angular );
	}

	sdPhysics_SimpleRigidBody* simpleRigidBody = physics->Cast< sdPhysics_SimpleRigidBody >();
	if ( simpleRigidBody != NULL ) {
		simpleRigidBody->SetFriction( linear, angular, contact );
		return;
	}
}

/*
================
idEntity::Event_SetSize
================
*/
void idEntity::Event_SetSize( const idVec3 &mins, const idVec3 &maxs ) {
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
	sdProgram::ReturnVector( bounds[1] - bounds[0] );
}

/*
================
idEntity::Event_GetMins
================
*/
void idEntity::Event_GetMins( void ) {
	sdProgram::ReturnVector( GetPhysics()->GetBounds()[0] );
}

/*
================
idEntity::Event_GetMaxs
================
*/
void idEntity::Event_GetMaxs( void ) {
	sdProgram::ReturnVector( GetPhysics()->GetBounds()[1] );
}

/*
================
idEntity::Event_GetAbsMins
================
*/
void idEntity::Event_GetAbsMins( void ) {
	sdProgram::ReturnVector( GetPhysics()->GetAbsBounds()[0] );
}

/*
================
idEntity::Event_GetAbsMaxs
================
*/
void idEntity::Event_GetAbsMaxs( void ) {
	sdProgram::ReturnVector( GetPhysics()->GetAbsBounds()[1] );
}

/*
================
idEntity::Event_GetRenderMins
================
*/
void idEntity::Event_GetRenderMins( void ) {
	sdProgram::ReturnVector( renderEntity.bounds[ 0 ] );
}

/*
================
idEntity::Event_GetRenderMaxs
================
*/
void idEntity::Event_GetRenderMaxs( void ) {
	sdProgram::ReturnVector( renderEntity.bounds[ 1 ] );
}

/*
================
idEntity::Event_SetRenderBounds
================
*/
void idEntity::Event_SetRenderBounds( const idVec3& mins, const idVec3& maxs ) {
	renderEntity.bounds[ 0 ] = mins;
	renderEntity.bounds[ 1 ] = maxs;
}

/*
================
idEntity::Event_Touches
================
*/
void idEntity::Event_Touches( idEntity *ent, bool ignoreNonTrace ) {
	if ( !ent ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	// check if these touch
	idPhysics* myPhysics = GetPhysics();
	idPhysics* entPhysics = ent->GetPhysics();

	// loop through all the clip models and test them against each other
	int numTraceModels = 0;
	for ( int myClipNum = 0; myClipNum < myPhysics->GetNumClipModels(); myClipNum++ ) {
		idClipModel* myClip = myPhysics->GetClipModel( myClipNum );
		if ( myClip->IsTraceModel() ) {
			numTraceModels++;
		} else {
			continue;
		}

		const idVec3& myOrigin = myClip->GetOrigin();
		const idMat3& myAxis = myClip->GetAxis();

		for ( int otherClipNum = 0; otherClipNum < entPhysics->GetNumClipModels(); otherClipNum++ ) {
			idClipModel* otherClip = entPhysics->GetClipModel( otherClipNum );

			int touches = gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS_SCRIPT myOrigin, myClip, myAxis, -1, 
														otherClip, otherClip->GetOrigin(), otherClip->GetAxis() );
			if ( touches != 0 ) {
				sdProgram::ReturnBoolean( true );
				return;
			}
		}
	}

	if ( numTraceModels == 0 ) {
		sdProgram::ReturnBoolean( false );
		if ( !ignoreNonTrace ) {
			gameLocal.Error( "touches() called on event with no trace model: %s", GetName() );
		}
		return;
	}

	sdProgram::ReturnBoolean( false );
}

/*
================
idEntity::Event_TouchesBounds
================
*/
void idEntity::Event_TouchesBounds( idEntity *ent ) {
	if ( !ent ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	idBox a = idBox( GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );
	idBox b = idBox( ent->GetPhysics()->GetBounds(), ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetAxis() );
	bool touches = a.IntersectsBox( b );
	
	sdProgram::ReturnBoolean( touches );
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
		sdProgram::ReturnString( "" );
	} else {
		sdProgram::ReturnString( kv->GetKey() );
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
void idEntity::Event_GetKey( const char *key ) {
	const char *value;
	spawnArgs.GetString( key, "", &value );
	sdProgram::ReturnString( value );
}

/*
================
idEntity::Event_GetIntKey
================
*/
void idEntity::Event_GetIntKey( const char *key ) {
	int value;
	spawnArgs.GetInt( key, "0", value );
	sdProgram::ReturnInteger( value );
}

/*
================
idEntity::Event_GetFloatKey
================
*/
void idEntity::Event_GetFloatKey( const char *key ) {
	float value;
	spawnArgs.GetFloat( key, "0", value );
	sdProgram::ReturnFloat( value );
}

/*
================
idEntity::Event_GetVectorKey
================
*/
void idEntity::Event_GetVectorKey( const char *key ) {
	idVec3 value;
	spawnArgs.GetVector( key, "0 0 0", value );
	sdProgram::ReturnVector( value );
}

/*
================
idEntity::Event_GetEntityKey
================
*/
void idEntity::Event_GetEntityKey( const char *key ) {
	idEntity *ent;
	const char *entname;

	if ( !spawnArgs.GetString( key, NULL, &entname ) ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	ent = gameLocal.FindEntity( entname );
	if ( !ent ) {
		gameLocal.Warning( "Couldn't find entity '%s' specified in '%s' key in entity '%s'", entname, key, name.c_str() );
	}

	sdProgram::ReturnEntity( ent );
}

/*
================
idEntity::Event_DistanceTo
================
*/
void idEntity::Event_DistanceTo( idEntity *ent ) {
	if ( !ent ) {
		// just say it's really far away
		sdProgram::ReturnFloat( MAX_WORLD_SIZE );
	} else {
		float dist = ( GetPhysics()->GetOrigin() - ent->GetPhysics()->GetOrigin() ).LengthFast();
		sdProgram::ReturnFloat( dist );
	}
}

/*
================
idEntity::Event_DistanceToPoint
================
*/
void idEntity::Event_DistanceToPoint( const idVec3 &point ) {
	float dist = ( GetPhysics()->GetOrigin() - point ).LengthFast();
	sdProgram::ReturnFloat( dist );
}

/***********************************************************************

   Network
	
***********************************************************************/

/*
================
sdColorNetworkData::sdColorNetworkData
================
*/
sdColorNetworkData::sdColorNetworkData( void ) {
}

/*
================
sdColorNetworkData::MakeDefault
================
*/
void sdColorNetworkData::MakeDefault( void ) {
	color = 0;
}

/*
================
sdColorNetworkData::Write
================
*/
void sdColorNetworkData::Write( const idEntity* ent, const sdColorNetworkData& base, idBitMsg& msg ) {
	color = GetColor( ent );

	msg.WriteDeltaLong( base.color, color );
}

/*
================
sdColorNetworkData::Read
================
*/
void sdColorNetworkData::Read( const idEntity* ent, const sdColorNetworkData& base, const idBitMsg& msg ) {
	color = msg.ReadDeltaLong( base.color );
}

/*
================
sdColorNetworkData::Apply
================
*/
void sdColorNetworkData::Apply( idEntity* ent ) const {
	idVec4 temp;
	sdColor4::UnpackColor( color, temp );

	renderEntity_t& renderEntity = *ent->GetRenderEntity();

	renderEntity.shaderParms[ SHADERPARM_RED ]		= temp[ 0 ];
	renderEntity.shaderParms[ SHADERPARM_GREEN ]	= temp[ 1 ];
	renderEntity.shaderParms[ SHADERPARM_BLUE ]		= temp[ 2 ];
	renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= temp[ 3 ];
}

/*
================
sdColorNetworkData::Check
================
*/
bool sdColorNetworkData::Check( const idEntity* ent ) const {
	return GetColor( ent ) != color;
}

/*
================
sdColorNetworkData::GetColor
================
*/
int sdColorNetworkData::GetColor( const idEntity* ent ) {
	const renderEntity_t& renderEntity = *( ent->GetRenderEntity() );

	idVec4 temp;
	temp[ 0 ] = renderEntity.shaderParms[ SHADERPARM_RED ];
	temp[ 1 ] = renderEntity.shaderParms[ SHADERPARM_GREEN ];
	temp[ 2 ] = renderEntity.shaderParms[ SHADERPARM_BLUE ];
	temp[ 3 ] = renderEntity.shaderParms[ SHADERPARM_ALPHA ];

	return sdColor4::PackColor( temp );
}

/*
================
sdColorNetworkData::Write
================
*/
void sdColorNetworkData::Write( idFile* file ) const {
	file->WriteInt( color );
}

/*
================
sdColorNetworkData::Read
================
*/
void sdColorNetworkData::Read( idFile* file ) {
	file->ReadInt( color );
}

const int sdBindNetworkData::bindPosBits = idMath::BitsForInteger( 64 );

/*
================
sdBindNetworkData::sdBindNetworkData
================
*/
sdBindNetworkData::sdBindNetworkData( void ) {
}

/*
================
sdBindNetworkData::MakeDefault
================
*/
void sdBindNetworkData::MakeDefault( void ) {
	joint			= INVALID_JOINT;
	bodyId			= INVALID_BODY;
	bindMasterId	= 0;
	oriented		= false;
}

/*
================
sdBindNetworkData::Write
================
*/
void sdBindNetworkData::Write( const idEntity* ent, const sdBindNetworkData& base, idBitMsg& msg ) {
	if ( !base.Check( ent ) ) {
		joint			= base.joint;
		bodyId			= base.bodyId;
		bindMasterId	= base.bindMasterId;
		oriented		= base.oriented;

		msg.WriteBits( 0, 1 );
		return;
	}

	bodyId			= ent->GetBindBody();
	bindMasterId	= gameLocal.GetSpawnId( ent->GetBindMaster() );
	joint			= ent->GetBindJoint();
	oriented		= ent->fl.bindOrientated;

	msg.WriteBits( 1, 1 );

	msg.WriteDelta( base.joint + 1, joint + 1, idMath::BitsForInteger( 64 ) );
	msg.WriteDelta( base.bodyId + 1, bodyId + 1, idMath::BitsForInteger( 64 ) );
	msg.WriteDeltaLong( base.bindMasterId, bindMasterId );
	msg.WriteBool( oriented );
}

/*
================
sdBindNetworkData::Read
================
*/
void sdBindNetworkData::Read( const idEntity* ent, const sdBindNetworkData& base, const idBitMsg& msg ) {
	if ( msg.ReadBits( 1 ) == 0 ) {
		joint			= base.joint;
		bodyId			= base.bodyId;
		bindMasterId	= base.bindMasterId;
		oriented		= base.oriented;
	} else {
		joint			= ( jointHandle_t )( msg.ReadDelta( base.joint + 1, idMath::BitsForInteger( 64 ) ) - 1 );
		bodyId			= msg.ReadDelta( base.bodyId + 1, idMath::BitsForInteger( 64 ) ) - 1;
		bindMasterId	= msg.ReadDeltaLong( base.bindMasterId );
		oriented		= msg.ReadBool();
	}
}

/*
================
sdBindNetworkData::Apply
================
*/
void sdBindNetworkData::Apply( idEntity* ent ) const {
	idEntity* master	= ent->GetBindMaster();

	idEntity* other		= gameLocal.EntityForSpawnId( bindMasterId );
	if ( !other ) {
		if ( master ) {
			ent->Unbind();
		}
	} else {
		if ( other != master ) {
			if ( master ) {
				ent->Unbind();
			}
			if ( joint != INVALID_JOINT ) {
				ent->BindToJoint( other, joint, oriented );
			} else if ( bodyId != INVALID_BODY ) {
				ent->BindToBody( other, bodyId, oriented );
			} else {
				ent->Bind( other, oriented );
			}
		}
	}
}

/*
================
sdBindNetworkData::Check
================
*/
bool sdBindNetworkData::Check( const idEntity* ent ) const {
	if ( ent->GetBindJoint() != joint ) {
		return true;
	}

	if ( ent->GetBindBody() != bodyId ) {
		return true;
	}

	if ( gameLocal.GetSpawnId( ent->GetBindMaster() ) != bindMasterId ) {
		return true;
	}

	if ( ent->fl.bindOrientated != oriented ) {
		return true;
	}

	return false;
}

/*
================
sdBindNetworkData::Write
================
*/
void sdBindNetworkData::Write( idFile* file ) const {
	file->WriteInt( joint );
	file->WriteInt( bodyId );
	file->WriteInt( bindMasterId );
	file->WriteBool( oriented );
}

/*
================
sdBindNetworkData::Read
================
*/
void sdBindNetworkData::Read( idFile* file ) {
	int temp;
	file->ReadInt( temp );
	joint = static_cast< jointHandle_t >( temp );

	file->ReadInt( bodyId );
	file->ReadInt( bindMasterId );
	file->ReadBool( oriented );
}

/*
================
idEntity::ForceNetworkUpdate
================
*/
void idEntity::ForceNetworkUpdate( void ) {
	gameLocal.ForceNetworkUpdate( this );
}

/*
================
idEntity::GetEntityAllegiance
================
*/
teamAllegiance_t idEntity::GetEntityAllegiance( const idEntity* other ) const {
	sdTeamInfo* otherTeam = other ? other->GetGameTeam() : NULL;
	sdTeamInfo* thisTeam =  this ? this->GetGameTeam() : NULL;

	if ( !thisTeam || !otherTeam ) {
		return TA_NEUTRAL;
	}

	if ( ( *otherTeam != *thisTeam ) ) {
		return TA_ENEMY;
	}

	return TA_FRIEND;
}

/*
================
idEntity::BroadcastEvent

   Saved events are also sent to any client that connects late so all clients
   always receive the events nomatter what time they join the game.
================
*/
void idEntity::BroadcastEvent( int eventId, const idBitMsg *msg, bool saveEvent, const sdReliableMessageClientInfoBase& target ) const {
	if ( !gameLocal.isServer ) {
		return;
	}

	sdReliableServerMessage outMsg( GAME_RELIABLE_SMESSAGE_ENTITY_EVENT );
	sdEntityNetEvent event;
	event.Create( this, eventId, saveEvent, msg );
	event.Write( outMsg );
	outMsg.Send( target );

	if ( target.SendToClients() && target.SendToAll() && g_debugNetworkWrite.GetBool() ) {
		gameLocal.LogNetwork( va( "Entity Broadcast: %s %d Size: %d bits\n", GetType()->classname, eventId, outMsg.GetNumBitsWritten() ) );
	}

	gameLocal.FreeEntityNetworkEvents( this, eventId );

	if ( saveEvent ) {
		if ( !target.SendToAll() || !target.SendToClients() || !target.SendToRepeaterClients() ) {
			gameLocal.Error( "idEntity::BroadcastEvent() - can't save an event going to a specific client!" );
		}

		gameLocal.SaveEntityNetworkEvent( event );
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
================
idEntity::BroadcastUnreliableEvent
================
*/
void idEntity::BroadcastUnreliableEvent( int eventId, const idBitMsg *msg ) const {
	if ( !gameLocal.isServer ) {
		return;
	}

	gameLocal.SendUnreliableEntityNetworkEvent( this, eventId, msg );
}

/*
================
idEntity::ClientReceiveUnreliableEvent
================
*/
bool idEntity::ClientReceiveUnreliableEvent( int event, int time, const idBitMsg &msg ) {
	return false;
}

/*
================
idEntity::ClearEntityCaches
================
*/
void idEntity::ClearEntityCaches( void ) {
	for ( int i = 0; i < NUM_ENTITY_CAHCES; i++ ) {
		savedEntityCache[ i ].first = false;
	}
}

/*
================
idEntity::Event_DisablePhysics
================
*/
void idEntity::Event_DisablePhysics( void ) {
	DisableClip();
}

/*
================
idEntity::Event_EnablePhysics
================
*/
void idEntity::Event_GetMaster( void ) {
	sdProgram::ReturnEntity( teamMaster == this ? NULL : teamMaster );
}

/*
================
idEntity::Event_EnablePhysics
================
*/
void idEntity::Event_EnablePhysics( void ) {
	EnableClip();
}

/*
================
idEntity::Event_EntitiesInTranslation
================
*/
void idEntity::Event_EntitiesInTranslation( const idVec3& start, const idVec3& end, int contentMask, idEntity* passEntity ) {
	idEntity* entityList[ MAX_PROXIMITY_ENTITIES ];
	int count = gameLocal.clip.EntitiesForTranslation( CLIP_DEBUG_PARMS_SCRIPT start, end, contentMask, passEntity, entityList, MAX_PROXIMITY_ENTITIES );

	scriptEntityCache.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		scriptEntityCache[ i ] = entityList[ i ];
	}
	sdProgram::ReturnInteger( count );	
}

/*
================
idEntity::Event_EntitiesInRadius
================
*/
void idEntity::Event_EntitiesInRadius( const idVec3& centre, float radius, int contentMask, bool absoluteCoords ) {
	idVec3 org = centre;
	if ( !absoluteCoords ) {
		org += GetPhysics()->GetOrigin();
	}

	idSphere sphere( org, radius );

	idEntity* entityList[ MAX_PROXIMITY_ENTITIES ];
	int count = gameLocal.clip.EntitiesTouchingRadius( sphere, contentMask, entityList, MAX_PROXIMITY_ENTITIES, true );

	scriptEntityCache.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		scriptEntityCache[ i ] = entityList[ i ];
	}
	sdProgram::ReturnInteger( count );
}

/*
================
idEntity::Event_EntitiesOfType
================
*/
void idEntity::Event_EntitiesOfType( int typeHandle ) {
	scriptEntityCache.SetNum( 0 );

	const idDeclEntityDef* def = gameLocal.declEntityDefType.SafeIndex( typeHandle );
	if ( def ) {
		idEntity* find = NULL;
		while ( find = gameLocal.FindEntityByType( find, def ) ) {
			idEntityPtr< idEntity >* ent = scriptEntityCache.Alloc();
			if ( !ent ) {
				break;
			}
			*ent = find;
		};
	}
	
	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::Event_EntitiesOfCollection
================
*/
void idEntity::Event_EntitiesOfCollection( const char* name ) {
	scriptEntityCache.SetNum( 0 );

	const sdEntityCollection* collection = gameLocal.GetEntityCollection( name );

	if ( collection ) {
		int i;
		for( i = 0; i < collection->Num(); i++ ) {
			idEntityPtr< idEntity >* ent = scriptEntityCache.Alloc();
			if ( !ent ) {
				break;
			}
			*ent = ( *collection )[ i ];			
		}
	}

	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::AddClassToBoundsCache
================
*/
void idEntity::AddClassToBoundsCache( idClass* cls ) {
	idEntityPtr< idEntity >* ent = scriptEntityCache.Alloc();
	if ( !ent ) {
		return;
	}
	*ent = static_cast< idEntity* >( cls );
}

/*
================
idEntity::Event_EntitiesOfClass
================
*/

typedef void (*InstancesFunction_t)(idClass *);

void idEntity::Event_EntitiesOfClass( int typeHandle, bool additive ) {
	if ( !additive ) {
		scriptEntityCache.SetNum( 0 );
	}

	idTypeInfo* type = idClass::GetType( typeHandle );
	if ( type ) {
		// not a const ref, the previous 'direct passing' syntax was wrong
		// gcc 4.2 picks it up as an error (previous versions didn't)
		InstancesFunction_t f = AddClassToBoundsCache;
		type->GetInstances( f, true );
	}

	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}


/*
================
idEntity::Event_EntitiesInBounds
================
*/
void idEntity::Event_EntitiesInBounds( const idVec3& mins, const idVec3& maxs, int contentMask, bool absoluteCoords ) {
	idBounds bounds;
	bounds.Clear();
	bounds.AddPoint( mins );
	bounds.AddPoint( maxs );
	if ( !absoluteCoords ) {
		bounds.TranslateSelf( GetPhysics()->GetOrigin() );
	}
	
	idEntity* entityList[ MAX_PROXIMITY_ENTITIES ];

	int count = gameLocal.clip.EntitiesTouchingBounds( bounds, contentMask, entityList, MAX_PROXIMITY_ENTITIES, true );

	scriptEntityCache.SetNum( count );
	for ( int i = 0; i < count; i++ ) {
		scriptEntityCache[ i ] = entityList[ i ];
	}

	sdProgram::ReturnInteger( count );
}

/*
================
idEntity::Event_EntitiesInLocalBounds
================
*/
void idEntity::Event_EntitiesInLocalBounds( const idVec3& mins, const idVec3& maxs, int contentMask ) {
	idBounds bounds;
	bounds.Clear();
	bounds.AddPoint( mins );
	bounds.AddPoint( maxs );

	idBounds worldBounds = bounds;
	worldBounds.RotateSelf( GetPhysics()->GetAxis() );
	worldBounds.TranslateSelf( GetPhysics()->GetOrigin() );
	
	idEntity* entityList[ MAX_PROXIMITY_ENTITIES ];
	int count = gameLocal.clip.EntitiesTouchingBounds( worldBounds, contentMask, entityList, MAX_PROXIMITY_ENTITIES, true );

	// filter out the entities that don't touch the local bounds
	idBox a = idBox( bounds, GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	scriptEntityCache.Clear();
	for ( int i = 0; i < count; i++ ) {
		idEntity* other = entityList[ i ];
		// not totally 100% accurate, but pretty good
		// ideally would check against all clip models, but meh
		idBox b = idBox( other->GetPhysics()->GetBounds(), other->GetPhysics()->GetOrigin(), other->GetPhysics()->GetAxis() );
		if ( a.IntersectsBox( b ) ) {
			idEntityPtr< idEntity >* ptr = scriptEntityCache.Alloc();
			if ( ptr == NULL ) {
				break;
			}

			*ptr = other;
		}
	}

	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::Event_FilterEntitiesByRadius
================
*/
void idEntity::Event_FilterEntitiesByRadius( const idVec3& origin, float radius, bool inclusive ) {	
	radius *= radius;

	int i;
	for ( i = 0; i < scriptEntityCache.Num(); ) {
		idEntity* ent = scriptEntityCache[ i ];
		float len = ( ent->GetPhysics()->GetOrigin() - origin ).LengthSqr();
		if ( ( len <= radius ) == ( inclusive == 0 ) ) {
			scriptEntityCache.RemoveIndex( i );
		} else {
			i++;
		}
	}
	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::Event_FilterEntitiesByClass
================
*/
void idEntity::Event_FilterEntitiesByClass( const char* typeName, bool inclusive ) {
	idTypeInfo* type = idClass::GetClass( typeName );
	if ( !type ) {
		gameLocal.Error( "idEntity::Event_FilterEntitiesByClass Invalid Type '%s'", typeName );
	}

	int i;
	for ( i = 0; i < scriptEntityCache.Num(); ) {
		if ( scriptEntityCache[ i ]->IsType( *type ) == ( inclusive == 0 ) ) {
			scriptEntityCache.RemoveIndex( i );
		} else {
			i++;
		}
	}
	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::Event_FilterEntitiesByAllegiance
================
*/
void idEntity::Event_FilterEntitiesByAllegiance( int mask, bool inclusive ) {
	FilterEntitiesByAllegiance( mask, inclusive != 0, true );
	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::Event_FilterEntitiesByDisguiseAllegiance
================
*/
void idEntity::Event_FilterEntitiesByDisguiseAllegiance( int mask, bool inclusive ) {
	FilterEntitiesByAllegiance( mask, inclusive != 0, false );
	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::FilterEntitiesByAllegiance
================
*/
void idEntity::FilterEntitiesByAllegiance( int mask, bool inclusive, bool ignoreDisguise ) {
	bool match;

	int i;
	for ( i = 0; i < scriptEntityCache.Num(); ) {
		idEntity* ent =  scriptEntityCache[ i ];
		idEntity* disguisedEnt = ent;
		if ( !ignoreDisguise ) {
			ent = ent->GetDisguiseEntity();
		}

		teamAllegiance_t entAlly = GetEntityAllegiance( ent );
		teamAllegiance_t disguisedAlly = GetEntityAllegiance( disguisedEnt );

		// friendlies know that the disguised guy is actually a friend
		if ( disguisedAlly == TA_FRIEND ) {
			entAlly = TA_FRIEND;
		}

		switch( entAlly ) {
			case TA_ENEMY:
				match = mask & TA_FLAG_ENEMY ? true : false;
				break;
			case TA_FRIEND:
				match = mask & TA_FLAG_FRIEND ? true : false;
				break;
			case TA_NEUTRAL:
				match = mask & TA_FLAG_NEUTRAL ? true : false;
				break;
		}

		if ( match != inclusive ) {
			scriptEntityCache.RemoveIndex( i );
		} else {
			i++;			
		}
	}
}

/*
================
idEntity::Event_FilterEntitiesByTouching
================
*/
void idEntity::Event_FilterEntitiesByTouching( bool inclusive ) {
	idBounds bounds;
	bounds.FromTransformedBounds( GetPhysics()->GetBounds(), GetPhysics()->GetOrigin(), GetPhysics()->GetAxis() );

	int i;
	for ( i = 0; i < scriptEntityCache.Num(); ) {
		idEntity* other = scriptEntityCache[ i ];

		if ( other->GetPhysics()->GetAbsBounds().IntersectsBounds( bounds ) == ( inclusive ? false : true ) ) {
			scriptEntityCache.RemoveIndex( i );
		} else {
			i++;			
		}
	}
	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::Event_FilterEntitiesByFilter
================
*/
void idEntity::Event_FilterEntitiesByFilter( int filterIndex, bool inclusive ) {
	if ( filterIndex < 0 || filterIndex > gameLocal.declTargetInfoType.Num() ) {
		assert( false );
		sdProgram::ReturnInteger( scriptEntityCache.Num() );
		return;
	}

	const sdDeclTargetInfo* filter = gameLocal.declTargetInfoType[ filterIndex ];

	bool match;

	int i;
	for ( i = 0; i < scriptEntityCache.Num(); ) {
		match = filter->FilterEntity( scriptEntityCache[ i ] );

		if ( match == ( inclusive ? false : true ) ) {
			scriptEntityCache.RemoveIndex( i );
		} else {
			i++;			
		}
	}
	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::Event_GetBoundsCacheCount
================
*/
void idEntity::Event_GetBoundsCacheCount( void ) {
	sdProgram::ReturnInteger( scriptEntityCache.Num() );
}

/*
================
idEntity::Event_GetBoundsCacheEntity
================
*/
void idEntity::Event_GetBoundsCacheEntity( int index ) {
	if ( index < 0 || index >= scriptEntityCache.Num() ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	sdProgram::ReturnEntity( scriptEntityCache[ index ] );
}

/*
================
idEntity::Event_GetEntityAllegiance
================
*/
void idEntity::Event_GetEntityAllegiance( idEntity* entity ) {
	sdProgram::ReturnInteger( GetEntityAllegiance( entity ) );
}

/*
================
idEntity::Event_HasAbility
================
*/
void idEntity::Event_HasAbility( const char* abilityName ) {
	qhandle_t abilityHandle = sdRequirementManager::GetInstance().RegisterAbility( abilityName );
	sdProgram::ReturnFloat( HasAbility( abilityHandle ) ? 1.f : 0.f );
}

/*
================
idEntity::Event_SyncScriptField
================
*/
void idEntity::Event_SyncScriptField( const char* fieldName ) {
	scriptObject->SetSynced( fieldName, false );
}

/*
================
idEntity::Event_SyncScriptFieldBroadcast
================
*/
void idEntity::Event_SyncScriptFieldBroadcast( const char* fieldName ) {
	scriptObject->SetSynced( fieldName, true );
}

/*
================
idEntity::Event_SyncScriptFieldCallback
================
*/
void idEntity::Event_SyncScriptFieldCallback( const char* fieldName, const char* functionName ) {
	scriptObject->SetSyncCallback( fieldName, functionName );
}

/*
================
idEntity::Event_TakesDamage
================
*/
void idEntity::Event_TakesDamage( void ) {
	sdProgram::ReturnBoolean( fl.takedamage );
}

/*
================
idEntity::Event_SetTakesDamage
================
*/
void idEntity::Event_SetTakesDamage( bool value ) {
	fl.takedamage = value != 0;
}

/*
================
idEntity::Event_SetNetworkSynced
================
*/
void idEntity::Event_SetNetworkSynced( bool value ) {
	if ( gameLocal.isClient ) {
		return;
	}

	SetNetworkSynced( value != 0 );
}

/*
================
idEntity::Event_ApplyDamage
================
*/
void idEntity::Event_ApplyDamage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, int damageIndex, float damageScale, idScriptObject* collisionHandle ) {
	const trace_t* trace = NULL;
	if ( collisionHandle ) {
		sdLoggedTrace* loggedTrace = collisionHandle->GetClass()->Cast< sdLoggedTrace >();
		if ( loggedTrace ) {
			trace = loggedTrace->GetTrace();
		}
	}

	const sdDeclDamage* damage = gameLocal.declDamageType[ damageIndex ];
	if ( !damage ) {
		return;
	}

	Damage( inflictor, attacker, dir, damage, damageScale, trace );
}

/*
================
idEntity::Event_Physics_ClearContacts
================
*/
void idEntity::Event_Physics_ClearContacts( void ) {
	GetPhysics()->ClearContacts();
}

/*
================
idEntity::Event_Physics_ClearContacts
================
*/
void idEntity::Event_Physics_SetContents( int contents ) {
	GetPhysics()->SetContents( contents );
}

/*
================
idEntity::Event_Physics_SetClipmask
================
*/
void idEntity::Event_Physics_SetClipmask( int clipmask ) {
	GetPhysics()->SetClipMask( clipmask );
}

/*
================
idEntity::Event_Physics_PutToRest
================
*/
void idEntity::Event_Physics_PutToRest( void ) {
	GetPhysics()->PutToRest();
}

/*
================
idEntity::Event_Physics_HasGroundContacts
================
*/
void idEntity::Event_Physics_HasGroundContacts( void ) {
	sdProgram::ReturnBoolean( GetPhysics()->HasGroundContacts() );
}

/*
================
idEntity::Event_DisableGravity
================
*/
void idEntity::Event_DisableGravity( bool disable ) {
	GetPhysics()->DisableGravity( disable );
}

/*
================
idEntity::Event_Physics_SetComeToRest
================
*/
void idEntity::Event_Physics_SetComeToRest( bool value ) {
	GetPhysics()->SetComeToRest( value );
}

/*
================
idEntity::Event_Physics_ApplyImpulse
================
*/
void idEntity::Event_Physics_ApplyImpulse( const idVec3& origin, const idVec3& impulse ) {
	GetPhysics()->ApplyImpulse( 0, origin, impulse );
}

/*
================
idEntity::Event_Physics_AddForce
================
*/
void idEntity::Event_Physics_AddForce( const idVec3& force ) {
	GetPhysics()->AddForce( force );
}

/*
================
idEntity::Event_Physics_AddForceAt
================
*/
void idEntity::Event_Physics_AddForceAt( const idVec3& force, const idVec3& position ) {
	GetPhysics()->AddForce( 0, position, force );
}

/*
================
idEntity::Event_Physics_AddTorque
================
*/
void idEntity::Event_Physics_AddTorque( const idVec3& torque ) {
	GetPhysics()->AddTorque( torque );
}

/*
================
idEntity::Event_HasForceDisableClip
================
*/
void idEntity::Event_HasForceDisableClip( void ) {
	sdProgram::ReturnBoolean( fl.forceDisableClip );
}

/*
================
idEntity::Event_PreventDeployment
================
*/
void idEntity::Event_PreventDeployment( bool prevent ) {
	fl.preventDeployment = prevent;
}

/*
================
idEntity::ForceDisableClip
================
*/
void idEntity::ForceDisableClip( void ) {
	fl.forceDisableClip = true;
	DisableClip();
}

/*
================
idEntity::ForceEnableClip
================
*/
void idEntity::ForceEnableClip( void ) {
	fl.forceDisableClip = false;
	EnableClip();
}

/*
================
idEntity::Event_ForceDisableClip
================
*/
void idEntity::Event_ForceDisableClip( void ) {
	ForceDisableClip();
}

/*
================
idEntity::Event_ForceEnableClip
================
*/
void idEntity::Event_ForceEnableClip( void ) {
	ForceEnableClip();
}

/*
================
idEntity::Event_TurnTowards
================
*/
void idEntity::Event_TurnTowards( const idVec3& dir, float maxAngularSpeed ) {
	idMat3 newAxes = dir.ToMat3();

	idMat3 change;
	TransposeMultiply( GetPhysics()->GetAxis(), newAxes, change );
	
	idRotation rotation = change.ToRotation();
	float rate = maxAngularSpeed * MS2SEC( gameLocal.msec );		
	rotation.SetAngle( idMath::ClampFloat( -rate, rate, rotation.GetAngle() ) );

	GetPhysics()->SetAxis( GetPhysics()->GetAxis() * rotation.ToMat3() );
}

/*
================
idEntity::Event_GetTeam
================
*/
void idEntity::Event_GetTeam( void ) {
	sdTeamInfo* _team = GetGameTeam();
	sdProgram::ReturnObject( _team ? _team->GetScriptObject() : NULL );
}

/*
================
idEntity::Event_SetTeam
================
*/
void idEntity::Event_SetTeam( idScriptObject* object ) {
	if ( object == NULL ) {
		SetGameTeam( NULL );
		return;
	}
	sdTeamInfo* team = object->GetClass()->Cast< sdTeamInfo >();
	if ( team == NULL ) {
		gameLocal.Warning( "idEntity::Event_SetTeam Invalid Team" );
		return;
	}
	SetGameTeam( team );
}

/*
================
idEntity::Event_LaunchMissile
================
*/
void idEntity::Event_LaunchMissile( idEntity* owner, idEntity* owner2, idEntity* enemy, int projectileDefIndex, int clientProjectileDefIndex, float spread, const idVec3& origin, const idVec3& velocity ) {

	idVec3 muzzleDir;
	muzzleDir = velocity;
	float push = muzzleDir.Normalize();

	const idDeclEntityDef* entityDef = gameLocal.declEntityDefType[ projectileDefIndex ];
	const idDeclEntityDef* clientEntityDef = gameLocal.declEntityDefType[ clientProjectileDefIndex ];

	if ( projectileDefIndex < 0 || projectileDefIndex > gameLocal.declEntityDefType.Num() ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	// calculate the direction to fire in
	float spreadRad = DEG2RAD( spread );

	missileRandom.SetSeed( gameLocal.time );
	float ang = idMath::Sin( spreadRad * missileRandom.RandomFloat() );
	float spin = DEG2RAD( 360.0f ) * missileRandom.RandomFloat();

	idMat3 muzzleAxis = muzzleDir.ToMat3();

	idVec3 dir = muzzleAxis[ 0 ] + muzzleAxis[ 2 ] * ( ang * idMath::Sin( spin ) ) - muzzleAxis[ 1 ] * ( ang * idMath::Cos( spin ) );
	dir.Normalize();

	// find out if its a bullet
	const idDict& entityDict = entityDef->dict;

	bool isBullet = entityDict.GetBool( "is_bullet" );
	bool clientIsBullet = isBullet;
	if ( clientEntityDef ) {
		clientIsBullet = clientEntityDef->dict.GetBool( "is_bullet" );
	}

	if ( isBullet != clientIsBullet ) {
		gameLocal.Warning( "idEntity::Event_LaunchMissile - server dict and client dict disagree about is_bullet!" );
		isBullet = false;
	}

	if ( isBullet ) {
		idMat3 axis = dir.ToMat3();
		LaunchBullet( owner, owner, entityDict, origin, axis, origin, axis, 1.0f, false );
		sdProgram::ReturnEntity( NULL );
		return;
	} else {
		if ( gameLocal.isClient ) {		
			if ( clientEntityDef ) {
				sdClientProjectile* clientEntity = new sdClientProjectile();
				clientEntity->CreateByName( &clientEntityDef->dict, clientEntityDef->dict.GetString( "scriptobject" )) ;
				clientEntity->SetOrigin( origin );
				clientEntity->SetAxis( muzzleDir.ToMat3() );

				if ( owner2 != NULL && owner2 != owner ) {
					clientEntity->AddOwner( owner2 );
				}
				clientEntity->Launch( owner, origin, muzzleDir.ToMat3() );
			}
			sdProgram::ReturnEntity( NULL );
			return;
		}

		idEntity* ent;
		gameLocal.SpawnEntityDef( entityDict, true, &ent );

		if ( !ent || ( !ent->IsType( idProjectile::Type ) ) ) {
			gameLocal.Error( "'%s' is not an idProjectile", entityDef->GetName() );
		}

		// Gordon: FIXME: This would be better handled in script
		bool isStroyBomb = entityDict.GetBool( "is_stroybomb" );

		if ( isStroyBomb ) {
			idPlayer* bombOwner = owner->Cast< idPlayer >();
			if ( bombOwner != NULL ) {
				bombOwner->SetStroyBombState( ent );
			}
		}

		idProjectile* proj = ent->Cast< idProjectile >();
		if ( owner2 != NULL && owner2 != owner ) {
			proj->AddOwner( owner2 );
		}
		proj->Create( owner, origin, muzzleDir );
		proj->SetEnemy( enemy );
		proj->Launch( origin, dir, vec3_zero, 0, push, 1.f );

		sdProgram::ReturnEntity( proj );
	}
}


/*
================
idEntity::Event_GetBulletTracerStart
================
*/
void idEntity::Event_GetBulletTracerStart( void ) {
	sdProgram::ReturnVector( bulletTracerStart );
}

/*
================
idEntity::Event_GetBulletTracerEnd
================
*/
void idEntity::Event_GetBulletTracerEnd( void ) {
	sdProgram::ReturnVector( bulletTracerEnd );
}

/*
================
idEntity::Event_LaunchBullet
	Hitscan, no cliententities and all that get spawned, eva, mate!
	the velocity parameters is a direction... it is hitscan...
================
*/
void idEntity::Event_LaunchBullet( idEntity* owner, idEntity* ignoreEntity, int projectileDefIndex, float spread, const idVec3& origin, const idVec3& direction, int forceTracer, bool useAntiLag ) {

	if ( projectileDefIndex < 0 || projectileDefIndex > gameLocal.declEntityDefType.Num() ) {
		return;
	}

	idVec3 muzzleDir = direction;
	muzzleDir.Normalize();

	const idDeclEntityDef* entityDef = gameLocal.declEntityDefType[ projectileDefIndex ];
	const idDict& entityDict = entityDef->dict;

	// calculate the direction to fire in
	float spreadRad = DEG2RAD( spread );

	missileRandom.SetSeed( gameLocal.time );
	float ang = idMath::Sin( spreadRad * missileRandom.RandomFloat() );
	float spin = 2 * idMath::PI * missileRandom.RandomFloat();

	idMat3 muzzleAxis = muzzleDir.ToMat3();

	float s, c;
	idMath::SinCos( spin, s, c );

	idVec3 dir = muzzleAxis[ 0 ] + muzzleAxis[ 2 ] * ( ang * s ) - muzzleAxis[ 1 ] * ( ang * c );
	dir.Normalize();

	idMat3 axis = dir.ToMat3();

	LaunchBullet( owner, ignoreEntity, entityDict, origin, axis, origin, axis, 1.0f, forceTracer, NULL, useAntiLag );
}

/*
=====================
idEntity::PlayEffect 
=====================
*/
rvClientEffect* idEntity::PlayEffect( const char* effectName, const idVec3& color, const char* materialType, const idVec3& origin, const idMat3& axis, bool loop, const idVec3& endOrigin, bool viewSuppress ) {
	return PlayEffect( gameLocal.GetEffectHandle( spawnArgs, effectName, materialType ), color, origin, axis, loop, endOrigin, viewSuppress );
}

rvClientEffect* idEntity::PlayEffectMaxVisDist( const char* effectName, const idVec3& color, const char* materialType, const idVec3& origin, const idMat3& axis, bool loop, bool isStatic, float maxVisDist, const idVec3& endOrigin ) {
	return PlayEffectMaxVisDist( gameLocal.GetEffectHandle( spawnArgs, effectName, materialType ), color, origin, axis, loop, isStatic, maxVisDist, endOrigin );
}

/*
=====================
idEntity::Event_LookupEffect
=====================
*/
void idEntity::Event_LookupEffect( const char* effectName, const char* materialType ) {
	int index = gameLocal.GetEffectHandle( spawnArgs, effectName, materialType );
	if ( index == -1 ) {
		sdProgram::ReturnString( "" );
		return;
	}

	sdProgram::ReturnString( declHolder.declEffectsType[ index ]->GetName() );
}

/*
=====================
idEntity::PlayEffect 
=====================
*/
rvClientEffect* idEntity::PlayEffect( const char* effectName, const idVec3& color, const char* materialType, jointHandle_t jointHandle, bool loop, const idVec3& endOrigin ) {
	return PlayEffect( gameLocal.GetEffectHandle( spawnArgs, effectName, materialType ), color, jointHandle, loop, endOrigin );
}

rvClientEffect* idEntity::PlayEffectMaxVisDist( const char* effectName, const idVec3& color, const char* materialType, jointHandle_t jointHandle, bool loop, bool isStatic, float maxVisDist, const idVec3& endOrigin ) {
	return PlayEffectMaxVisDist( gameLocal.GetEffectHandle( spawnArgs, effectName, materialType ), color, jointHandle, loop, isStatic, maxVisDist, endOrigin );
}


/*
====================
idEntity::OnEventRemove
===================
*/
void idEntity::OnEventRemove( void ) {
	idClass::OnEventRemove();
	gameLocal.localPlayerProperties.OnEntityDestroyed( this );

	if ( DisableClipOnRemove() && !gameLocal.IsDoingMapRestart() && gameLocal.GameState() != GAMESTATE_SHUTDOWN ) {
		// make sure all the contacting entities are woken up
		GetPhysics()->DisableClip();
	}
}

/*
====================
idEntity::FindSurfaceId
===================
*/
int idEntity::FindSurfaceId( const char* surfaceName ) {
	if ( renderEntity.hModel ) {
		return renderEntity.hModel->FindSurfaceId( surfaceName );
	}

	return -1;
}

/*
============
idEntity::GetEntityGuiHandle
============
*/
guiHandle_t idEntity::GetEntityGuiHandle( const int guiNum ) {
	for ( rvClientEntity* cent = clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {
		sdGuiSurface* guiSurface = cent->Cast< sdGuiSurface >();
		if ( guiSurface != NULL ) {
			if ( guiSurface->GetRenderable().GetGuiNum() == guiNum ) {
				return guiSurface->GetRenderable().GetGuiHandle();
			}
		}
	}

	return 0;
}

/*
============
idEntity::Event_GetGUI
============
*/
void idEntity::Event_GetGUI( const char* name ) {
	if( networkSystem->IsDedicated() ) {
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	guiHandle_t retVal;

	if ( networkSystem->IsDedicated() ) {
		sdProgram::ReturnInteger( retVal );
		return;
	}

	if( !idStr::Cmp( name, "0" ) ) {
		retVal = GetEntityGuiHandle( 0 );
	} else if( !idStr::Cmp( name, "1" ) ) {
		retVal = GetEntityGuiHandle( 1 );
	} else if( !idStr::Cmp( name, "2" ) ) {
		retVal = GetEntityGuiHandle( 2 );
	} else if( !idStr::Cmp( name, "scoreboard" ) ) {
		retVal = gameLocal.localPlayerProperties.GetScoreBoard();
	} else if( !idStr::Cmp( name, "hud" ) ) {
		sdPlayerHud* hud = gameLocal.localPlayerProperties.GetPlayerHud();
		if ( hud ) {
			retVal = hud->GetGuiHandle();
		}
	}

	sdProgram::ReturnInteger( retVal );
}

/*
================
idEntity::Event_IsAtRest
================
*/
void idEntity::Event_IsAtRest( void ) {
	sdProgram::ReturnBoolean( IsAtRest() );
}

/*
================
idEntity::Event_IsAtRest
================
*/
void idEntity::Event_IsBound( void ) {
	sdProgram::ReturnBoolean( IsBound() );
}

/*
================
idEntity::Event_DisableImpact
================
*/
void idEntity::Event_DisableImpact( void ) {
	GetPhysics()->DisableImpact();
}

/*
================
idEntity::Event_EnableImpact
================
*/
void idEntity::Event_EnableImpact( void ) {
	GetPhysics()->EnableImpact();
}

/*
================
idEntity::Event_EnableKnockback
================
*/
void idEntity::Event_EnableKnockback( void ) {
	fl.noknockback = false;
}

/*
================
idEntity::Event_DisableKnockback
================
*/
void idEntity::Event_DisableKnockback( void ) {
	fl.noknockback = true;
}

/*
================
idEntity::Event_Physics_Activate
================
*/
void idEntity::Event_Physics_Activate( void ) {
	ActivatePhysics();
}


/*
================
idEntity::Event_SetNumCrosshairLines
================
*/
void idEntity::Event_SetNumCrosshairLines( int count ) {
	crosshairInfo->SetNumLines( count );
}

/*
================
idEntity::Event_AddCrosshairLine
================
*/
void idEntity::Event_AddCrosshairLine() {
	int numLines = crosshairInfo->GetNumLines();
	crosshairInfo->SetNumLines( numLines + 1 );
	sdProgram::ReturnInteger( numLines );
}

/*
================
idEntity::Event_GetCrosshairDistance
================
*/
void idEntity::Event_GetCrosshairDistance() {
	sdProgram::ReturnFloat( crosshairInfo->GetDistance() );
}

/*
================
idEntity::Event_SetCrosshairLineText
================
*/
void idEntity::Event_SetCrosshairLineText( int index, const wchar_t* text ) {
	crosshairInfo->GetLine( index ).text = text;
}

/*
================
idEntity::Event_SetCrosshairLineTextIndex
================
*/
void idEntity::Event_SetCrosshairLineTextIndex( int index, const int textIndex ) {
	crosshairInfo->GetLine( index ).text = declHolder.FindLocStrByIndex( textIndex )->GetText();
}

/*
================
idEntity::Event_SetCrosshairLineMaterial
================
*/
void idEntity::Event_SetCrosshairLineMaterial( int index, const char* material ) {
	crosshairInfo->GetLine( index ).material = declHolder.FindMaterial( material );
}

/*
================
idEntity::Event_SetCrosshairLineColor
================
*/
void idEntity::Event_SetCrosshairLineColor( int index, const idVec3& color, float alpha ) {
	crosshairInfo->GetLine( index ).foreColor.ToVec3() = color;
	crosshairInfo->GetLine( index ).foreColor.w = alpha;
}

/*
================
idEntity::Event_SetCrosshairLineSize
================
*/
void idEntity::Event_SetCrosshairLineSize( int index, float x, float y ) {
	crosshairInfo->GetLine( index ).xy.x = x;
	crosshairInfo->GetLine( index ).xy.y = y;
}

/*
================
idEntity::Event_SetCrosshairLineFraction
================
*/
void idEntity::Event_SetCrosshairLineFraction( int index, float frac ) {
	crosshairInfo->GetLine( index ).frac = frac;
}

/*
================
idEntity::Event_SetCrosshairLineType
================
*/
void idEntity::Event_SetCrosshairLineType( int index, int type ) {
	crosshairInfo->GetLine( index ).type = static_cast< chInfoLineType_t >( type );
}

/*
================
idEntity::Event_SendNetworkEvent
================
*/
void idEntity::Event_SendNetworkEvent( int clientIndex, bool isRepeaterClient, const char* message ) {
	if ( !isRepeaterClient ) {
		if ( clientIndex == -1 ) {
			if ( gameLocal.GetLocalPlayer() != NULL ) {
				gameLocal.HandleNetworkEvent( this, message );
			}
		} else {
			idPlayer* player = gameLocal.GetClient( clientIndex );
			if ( player != NULL && gameLocal.IsLocalPlayer( player ) ) {
				gameLocal.HandleNetworkEvent( this, message );
			}
		}
	}

	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_NETWORKEVENT );
	msg.WriteLong( gameLocal.GetSpawnId( this ) );
	msg.WriteString( message );
	if ( isRepeaterClient ) {
		msg.Send( sdReliableMessageClientInfoRepeater( clientIndex ) );
	} else {
		msg.Send( sdReliableMessageClientInfo( clientIndex ) );
	}
}

/*
================
idEntity::Event_SendNetworkCommand
================
*/
void idEntity::Event_SendNetworkCommand( const char* message ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_NETWORKCOMMAND );
		msg.WriteLong( gameLocal.GetSpawnId( this ) );
		msg.WriteString( message );
		msg.Send();
	} else {
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer ) {
			gameLocal.HandleNetworkMessage( localPlayer, this, message );
		} else {
			gameLocal.Warning( "idEntity::Event_SendNetworkCommand Called with no local client" );
		}
	}
}

/*
================
idEntity::Event_PointInRadar
================
*/
void idEntity::Event_PointInRadar( const idVec3& point, radarMasks_t type, bool ignoreJammers ) {
	sdTeamInfo* team = GetGameTeam();
	if ( !team ) {
		sdProgram::ReturnBoolean( false );
		return;
	}

	// first check if the point is jammed
	if ( !ignoreJammers ) {
		sdTeamManagerLocal& manager = sdTeamManager::GetInstance();
		int numTeams = manager.GetNumTeams();
		for ( int i = 0; i < numTeams; i++ ) {
			sdTeamInfo& otherTeam = manager.GetTeamByIndex( i );
			if ( &otherTeam == team ) {
				continue;
			}

			if ( otherTeam.PointInJammer( point, type ) ) {
				sdProgram::ReturnBoolean( false );
				return;
			}
		}
	}

	// point wasn't jammed
	sdProgram::ReturnBoolean( team->PointInRadar( point, type ) != NULL );
}

/*
=====================
idEntity::Event_GetHealth
=====================
*/
void idEntity::Event_GetHealth( void ) {
	sdProgram::ReturnInteger( GetHealth() );
}

/*
================
idEntity::Event_SetHealth
================
*/
void idEntity::Event_SetHealth( int value ) {
	SetHealth( value );
}

/*
=====================
idEntity::Event_GetMaxHealth
=====================
*/
void idEntity::Event_GetMaxHealth( void ) {
	sdProgram::ReturnInteger( GetMaxHealth() );
}

/*
================
idEntity::Event_SetMaxHealth
================
*/
void idEntity::Event_SetMaxHealth( int value ) {
	SetMaxHealth( value );
}

/*
============
idEntity::GetColorForAllegiance
============
*/
const idVec4& idEntity::GetColorForAllegiance( idEntity* other ) const {
	return GetColorForAllegiance( GetEntityAllegiance( other ) );
}

idCVar g_friendlyColor( "g_friendlyColor", ".5 .83 0 1", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "color of friendly units" );
idCVar g_neutralColor( "g_neutralColor", "0.75 0.75 0.75", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "color of neutral units" );
idCVar g_enemyColor( "g_enemyColor", "0.9 0.1 0.1 1", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "color of enemy units" );
idCVar g_fireteamColor( "g_fireteamColor", "1 1 1 1", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "color of fireteam units" );
idCVar g_fireteamLeaderColor( "g_fireteamLeaderColor", "1 1 0 1", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "color of fireteam leader" );
idCVar g_buddyColor( "g_buddyColor", "0 1 1 1", CVAR_GAME | CVAR_ARCHIVE | CVAR_PROFILE, "color of buddies" );
idCVar g_cheapDecalsMaxDistance( "g_cheapDecalsMaxDistance", "16384", CVAR_GAME | CVAR_ARCHIVE, "max distance decals are created" );

/*
============
idEntity::GetColorForAllegiance
============
*/
const idVec4& idEntity::GetColorForAllegiance( teamAllegiance_t allegiance ) {
	static idVec4 color;
	color.w = 1.0f;
	switch( allegiance ) {
		default:
		case TA_NEUTRAL:
			sdProperties::sdFromString( color.ToVec3(), g_neutralColor.GetString() );
			break;
		case TA_FRIEND:
			sdProperties::sdFromString( color.ToVec3(), g_friendlyColor.GetString() );
			break;
		case TA_ENEMY:
			sdProperties::sdFromString( color.ToVec3(), g_enemyColor.GetString() );
			break;
	}
	return color;
}

/*
============
idEntity::GetFireteamColor
============
*/
const idVec4& idEntity::GetFireteamColor( void ) {
	static idVec4 color;
	color.w = 1.0f;
	sdProperties::sdFromString( color.ToVec3(), g_fireteamColor.GetString() );
	return color;
}

/*
============
idEntity::GetFireteamLeaderColor
============
*/
const idVec4& idEntity::GetFireteamLeaderColor( void ) {
	static idVec4 color;
	color.w = 1.0f;
	sdProperties::sdFromString( color.ToVec3(), g_fireteamLeaderColor.GetString() );
	return color;
}

/*
============
idEntity::GetBuddyColor
============
*/
const idVec4& idEntity::GetBuddyColor( void ) {
	static idVec4 color;
	color.w = 1.0f;
	sdProperties::sdFromString( color.ToVec3(), g_buddyColor.GetString() );
	return color;
}

/*
================
idEntity::Event_AllocBeam
================
*/
void idEntity::Event_AllocBeam( const char* shader ) {
	int i;
	beamInfo_t* info = NULL;
	for( i = 0; i < beams.Num(); i++ ) {
		if( beams[ i ] == NULL ) {
			break;
		}
	}

	if( i == beams.Num() ) {
		info = beams.Alloc();
		i = beams.Num() - 1;
	}

	info = new beamInfo_t;
	beams[ i ] = info;

	info->beamHandle = -1;
	memset( &info->beamEnt, 0, sizeof( info->beamEnt ) );

	info->beamEnt.axis.Identity();	
	info->beamEnt.hModel = renderModelManager->FindModel( "_beam" );
	info->beamEnt.customShader = declHolder.declMaterialType.LocalFind( shader );
	info->beamEnt.bounds.Clear();
	info->beamEnt.bounds.AddPoint( idVec3( MIN_WORLD_COORD, MIN_WORLD_COORD, MIN_WORLD_COORD) );
	info->beamEnt.bounds.AddPoint( idVec3( MAX_WORLD_COORD, MAX_WORLD_COORD, MAX_WORLD_COORD) );

	sdProgram::ReturnInteger( i );
}

/*
================
idEntity::Event_UpdateBeam
================
*/
void idEntity::Event_UpdateBeam( int handle, const idVec3& start, const idVec3& end, const idVec3& color, float alpha, float width ) {
	if( handle < 0 || handle >= beams.Num() ) {
		gameLocal.Error( "ClearBeam: index '%d' out of bounds", handle );
	}

	beamInfo_t* info = beams[ handle ];
	if( !info ) {
		gameLocal.Error( "ClearBeam: handle points to an invalid beam" );
	}

	info->beamEnt.origin = start;
	info->beamEnt.shaderParms[ SHADERPARM_RED ]			= color[ 0 ];
	info->beamEnt.shaderParms[ SHADERPARM_GREEN ]		= color[ 1 ];
	info->beamEnt.shaderParms[ SHADERPARM_BLUE ]		= color[ 2 ];
	info->beamEnt.shaderParms[ SHADERPARM_ALPHA ]		= alpha;
	info->beamEnt.shaderParms[ SHADERPARM_BEAM_END_X ]	= end[ 0 ];
	info->beamEnt.shaderParms[ SHADERPARM_BEAM_END_Y ]	= end[ 1 ];
	info->beamEnt.shaderParms[ SHADERPARM_BEAM_END_Z ]	= end[ 2 ];
	info->beamEnt.shaderParms[ SHADERPARM_BEAM_WIDTH ]	= width;

	if ( info->beamHandle >= 0 ) {
		gameRenderWorld->UpdateEntityDef( info->beamHandle, &info->beamEnt );
	} else {
		info->beamHandle = gameRenderWorld->AddEntityDef( &info->beamEnt );
	}
}

/*
================
idEntity::Event_FreeBeam
================
*/
void idEntity::Event_FreeBeam( int index ) {
	FreeBeam( index );
}

/*
================
idEntity::Event_FreeAllBeams
================
*/
void idEntity::Event_FreeAllBeams() {
	FreeAllBeams();
}

/*
================
idEntity::Event_GetNextTeamEntity
================
*/
void idEntity::Event_GetNextTeamEntity( void ) {
	sdProgram::ReturnEntity( GetNextTeamEntity() );
}

/*
================
idEntity::FreeBeam
================
*/
void idEntity::FreeBeam( int index ) {
	if( index < 0 || index >= beams.Num() ) {
		gameLocal.Error( "FreeBeam: handle '%i' is invalid", index );
	}
	beamInfo_t* info = beams[ index ];
	if( !info ) {
		gameLocal.Error( "FreeBeam: handle '%i' points to an invalid beam", index );
	}

	if( info->beamHandle >= 0 ) {
		gameRenderWorld->FreeEntityDef( info->beamHandle );
	}
	delete info;
	beams[ index ] = NULL;
}

/*
================
idEntity::FreeAllBeams
================
*/
void idEntity::FreeAllBeams( void ) {
	int i;
	for( i = 0; i < beams.Num(); i++ ) {
		if ( beams[ i ] ) {
			FreeBeam( i );
		}
	}
	beams.Clear();
}

/*
============
idEntity::Event_SetCanCollideWithTeam
============
*/
void idEntity::Event_SetCanCollideWithTeam( bool canCollide ) {
	fl.canCollideWithTeam = canCollide;
}

/*
================
idEntity::Event_GetKeyWithDefault
================
*/
void idEntity::Event_GetKeyWithDefault( const char *key, const char *defaultvalue ) {
	const char *result;

	spawnArgs.GetString( key, defaultvalue, &result );
	sdProgram::ReturnString( result );
}

/*
================
idEntity::Event_GetIntKeyWithDefault
================
*/
void idEntity::Event_GetIntKeyWithDefault( const char *key, int defaultvalue ) {
	int result;
	spawnArgs.GetInt( key, va( "%i", defaultvalue ), result );
	sdProgram::ReturnInteger( result );
}

/*
================
idEntity::Event_GetFloatKeyWithDefault
================
*/
void idEntity::Event_GetFloatKeyWithDefault( const char *key, float defaultvalue ) {
	float result;

	spawnArgs.GetFloat( key, va( "%f", defaultvalue ), result );
	sdProgram::ReturnFloat( result );
}

/*
================
idEntity::Event_GetVectorKeyWithDefault
================
*/
void idEntity::Event_GetVectorKeyWithDefault( const char *key, idVec3 &defaultvalue ) {
	idVec3 result;

	spawnArgs.GetVector( key, defaultvalue.ToString(), result );
	sdProgram::ReturnVector( result );
}

/*
===============
idEntity::Event_IsInWater
===============
*/
void idEntity::Event_IsInWater( void ) {
	sdProgram::ReturnFloat( physics->InWater() );
}

/*
===============
idEntity::Event_GetSpawnID
===============
*/
void idEntity::Event_GetSpawnID( void ) {
	int spawnID = gameLocal.GetSpawnId( this );
	char buffer[64];
	sprintf( buffer, "%i", spawnID );
	sdProgram::ReturnString( buffer );
}

/*
===============
idEntity::Event_SetSpotted
===============
*/
void idEntity::Event_SetSpotted( idEntity* spotter ) {
	SetSpotted( spotter );
}

/*
===============
idEntity::Event_IsSpotted
===============
*/
void idEntity::Event_IsSpotted( void ) {
	sdProgram::ReturnFloat( fl.spotted ? 1.f : 0.f );
}

/*
===============
idEntity::Event_ForceNetworkUpdate
===============
*/
void idEntity::Event_ForceNetworkUpdate( void ) {
	ForceNetworkUpdate();
}

/*
============
idEntity::LaunchBullet
============
*/
idCVar g_debugBulletFiring( "g_debugBulletFiring", "0", CVAR_GAME | CVAR_BOOL, "spam info about bullet traces" );

bool idEntity::LaunchBullet( idEntity* owner, idEntity* ignoreEntity, const idDict& projectileDict, const idVec3& origin, const idMat3& axis, const idVec3& tracerMuzzleOrigin, const idMat3& tracerMuzzleAxis, float damagePower, int forceTracer, rvClientEffect** tracerOut, bool useAntiLag ) {
// TODO: This entire function could be cleaned up by getting a lot of the information out of the dict much earlier & storing it

	// find out the range of the weapon
	float bulletRange = projectileDict.GetFloat( "range" );
	if ( bulletRange <= 0.0f ) {
		bulletRange = 8192.f;
	}

	// find out the damage def of the bullet
	const sdDeclDamage* bulletDamage = NULL;

	const char* damageDefName = projectileDict.GetString( "dmg_damage" );
	if ( *damageDefName ) {
		bulletDamage = gameLocal.declDamageType[ damageDefName ];
	}

	if ( bulletDamage == NULL ) {
		return false;
	}

	idPlayer* playerOwner = owner->Cast< idPlayer >();
	
	if ( bulletDamage->GetRecordHitStats() ) {
		if ( playerOwner != NULL ) {
			if ( gameLocal.totalShotsFiredStat != NULL ) {
				gameLocal.totalShotsFiredStat->IncreaseValue( playerOwner->entityNumber, 1 );
			}
		}
	}

	// set up the trace
	idVec3 startPos = origin;
	idVec3 endPos = startPos + ( axis[ 0 ] * bulletRange );

	if ( g_debugBulletFiring.GetBool() ) {
		idVec4 colors[] = { 
			idVec4( 1.0f, 0.0f, 0.0f, 1.0f ),
			idVec4( 0.0f, 1.0f, 0.0f, 1.0f ),
			idVec4( 0.0f, 0.0f, 1.0f, 1.0f ),
			idVec4( 1.0f, 0.0f, 1.0f, 1.0f ),
			idVec4( 0.0f, 1.0f, 1.0f, 1.0f ),
			idVec4( 1.0f, 1.0f, 1.0f, 1.0f ),
			idVec4( 0.0f, 0.0f, 0.0f, 1.0f ),
		};

		int numColors = sizeof( colors ) / sizeof( idVec4 );
		static int colorNumUpto = 0;
		idVec4 colorToDraw = colors[ ( colorNumUpto++ )%numColors ];

		// HACK - change the clipmodel drawing color
		idCVar* colorCvar = cvarSystem->Find( "cm_drawColor" );
		idVec4 oldCvarColor;
		if ( colorCvar != NULL ) {
			sscanf( colorCvar->GetString(), "%f %f %f %f", &oldCvarColor.x, &oldCvarColor.y, &oldCvarColor.z, &oldCvarColor.w );
			colorCvar->SetString( va( "%f %f %f %f", colorToDraw.x, colorToDraw.y, colorToDraw.z, colorToDraw.w ) );
			colorCvar->SetModified();
		}

		// draw all the clients
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL || player == playerOwner ) {
				continue;
			}
	

			player->GetPhysics()->GetClipModel( 1 )->Draw( player->GetPhysics()->GetOrigin(), player->GetPhysics()->GetAxis(), 0.0, 20000 );
		}

		// HACK - change the clipmodel drawing color back
		if ( colorCvar != NULL ) {
			colorCvar->SetString( va( "%f %f %f %f", oldCvarColor.x, oldCvarColor.y, oldCvarColor.z, oldCvarColor.w ) );
			colorCvar->SetModified();
		}

		gameRenderWorld->DebugArrow( colorToDraw, startPos, endPos, 8, 20000 );

		// draw the ideal forwards
		if ( playerOwner != NULL ) {
			idMat3 axis = playerOwner->firstPersonViewAxis;
			idVec3 origin = playerOwner->firstPersonViewOrigin;
			gameRenderWorld->DebugArrow( colorYellow, origin, origin + axis[ 0 ] * 2048.0f, 8, 20000 );
			
			gameLocal.Printf( "%i %.4f %.4f %.4f %.6f %.6f %.6f ", gameLocal.time, origin.x, origin.y, origin.z, axis[ 0 ].x, axis[ 0 ].y, axis[ 0 ].z );
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				idPlayer* player = gameLocal.GetClient( i );
				if ( player == NULL || player == playerOwner ) {
					continue;
				}
				idVec3 otherOrigin = player->GetPhysics()->GetOrigin();
				gameLocal.Printf( " %.4f %.4f %.4f ", otherOrigin.x, otherOrigin.y, otherOrigin.z );
			}

			gameLocal.Printf( "\n" );
		}
	}

	bool doImpact = !gameLocal.isClient;
	bool doEffects = gameLocal.DoClientSideStuff();

	const int serverMask = MASK_SHOT_RENDERMODEL | MASK_SHOT_BOUNDINGBOX;
	const int clientMask = serverMask | CONTENTS_WATER;

	gameLocal.EnablePlayerHeadModels();

	bool hit = false;
	if ( doImpact ) {
		if ( doEffects ) {
			DoLaunchBullet( owner, ignoreEntity, projectileDict, startPos, endPos, tracerMuzzleOrigin, tracerMuzzleAxis, damagePower, forceTracer, tracerOut, clientMask, NULL, true, false, useAntiLag );
		}
		hit = DoLaunchBullet( owner, ignoreEntity, projectileDict, startPos, endPos, tracerMuzzleOrigin, tracerMuzzleAxis, damagePower, false, NULL, serverMask, bulletDamage, false, true, useAntiLag );
		if ( hit ) {
			if ( bulletDamage->GetRecordHitStats() ) {
				if ( playerOwner != NULL ) {
					if ( gameLocal.totalShotsHitStat != NULL ) {
						gameLocal.totalShotsHitStat->IncreaseValue( playerOwner->entityNumber, 1 );
					}
				}
			}
		}
	} else {
		DoLaunchBullet( owner, ignoreEntity, projectileDict, startPos, endPos, tracerMuzzleOrigin, tracerMuzzleAxis, damagePower, forceTracer, tracerOut, clientMask, bulletDamage, true, true, useAntiLag );
	}

	gameLocal.DisablePlayerHeadModels();

	return hit;
}

static const float COS_75 = 0.258819f;
bool idEntity::DoLaunchBullet( idEntity* owner, idEntity* ignoreEntity, const idDict& projectileDict, const idVec3& startPos, const idVec3& endPos, const idVec3& tracerMuzzleOrigin, const idMat3& tracerMuzzleAxis, float damagePower, int forceTracer, rvClientEffect** tracerOut, int mask, const sdDeclDamage* bulletDamage, bool doEffects, bool doImpact, bool useAntiLag ) {
	idPlayer* playerOwner = owner->Cast< idPlayer >();
	// trace
	trace_t trace;
	if ( useAntiLag ) {
		sdAntiLagManager::GetInstance().Trace( trace, startPos, endPos, mask, playerOwner, ignoreEntity );
	} else {
		gameLocal.clip.TracePoint( trace, startPos, endPos, mask, ignoreEntity );
	}

	bool damageDone = false;

	if ( trace.fraction != 1.f ) {
		idEntity* collisionEnt = gameLocal.GetTraceEntity( trace );

		// Play effects
		if ( doEffects ) {
			idVec3 normalisedVelocity = ( endPos - startPos ); // direction of traveling object
			normalisedVelocity.Normalize();

			idVec3 traceNormal = -trace.c.normal;
			float dot = normalisedVelocity * traceNormal;
			idVec3 materialColor = trace.c.surfaceColor;

			idVec3 reflection = normalisedVelocity + ((dot * 2.f) * trace.c.normal);
			idMat3 axis = trace.c.normal.ToMat3();

			idStr collisionSurface;
			if ( trace.c.surfaceType != NULL ) {
				collisionSurface = trace.c.surfaceType->GetName();	
			} else if ( collisionEnt != NULL ) {
				collisionSurface = collisionEnt->GetDefaultSurfaceType();
			}

			if ( dot < COS_75 ) {
				int effectHandle = gameLocal.GetEffectHandle( projectileDict, "fx_ricochet", collisionSurface.c_str() );
				gameLocal.PlayEffect( effectHandle, materialColor, trace.endpos, axis );
			}
			int effectHandle = gameLocal.GetEffectHandle( projectileDict, "fx_explode", collisionSurface.c_str() );
			rvClientEffect *eff = gameLocal.PlayEffect( effectHandle, materialColor, trace.endpos, axis );
			if ( eff != NULL && trace.c.entityNum != ENTITYNUM_WORLD && trace.c.entityNum >= 0 && trace.c.entityNum <= sizeof( gameLocal.entities ) / sizeof( gameLocal.entities[0] ) ) {
				idEntity* ent = gameLocal.entities[ trace.c.entityNum ];
				eff->Monitor( ent );
			}

			// draw the hit decal
			if ( (trace.c.material==NULL || trace.c.material->AllowOverlays()) ) {
				bool createDecal = true;

				idPlayer *lp = gameLocal.GetLocalPlayer();
				if ( lp ) {
					float dist = (lp->GetViewPos() - trace.endpos).LengthSqr();
					float md2 = g_cheapDecalsMaxDistance.GetFloat() * g_cheapDecalsMaxDistance.GetFloat();
					createDecal = dist < md2;
				}
				if ( createDecal ) {
					idVec3 negTrace = -traceNormal;
					gameLocal.AddCheapDecal( projectileDict, collisionEnt, trace.endpos, negTrace, CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id ), trace.c.id, "dec_impact", collisionSurface.c_str() );
				}
			}
		}

		if ( doImpact ) {
			if ( collisionEnt != NULL ) {
				if ( !gameLocal.isClient && collisionEnt->fl.takedamage ) {
					idVec3 dir = endPos - startPos;
					dir.Normalize();

					float power = damagePower;

					float cutoff = projectileDict.GetFloat( "min_damage_percent", "100" ) * 0.01;
					if ( cutoff < 1.f && cutoff > 0.f ) {
						if ( trace.fraction > cutoff ) {
							float range = 1.f - cutoff;
							power *= cos( ( idMath::PI * ( trace.fraction - cutoff ) ) / ( 2 * range ) );
						}
					}

					int oldHealth = collisionEnt->GetHealth();
					collisionEnt->Damage( this, owner, dir, bulletDamage, power, &trace );

					if ( collisionEnt->GetEntityAllegiance( owner ) != TA_FRIEND ) {
						damageDone = collisionEnt->GetHealth() < oldHealth;
					}
				}

				collisionEnt->OnBulletImpact( owner, trace );
			}
		}
	}

	bulletTracerStart = tracerMuzzleOrigin;
	bulletTracerEnd = trace.endpos;

	if ( doEffects ) {
		if ( forceTracer != 2 && ( forceTracer == 1 || missileRandom.RandomFloat() < projectileDict.GetFloat( "tracer_chance" ) ) ) {
			if ( !gameLocal.isClient || !projectileDict.GetBool( "tracer_server_only" ) || owner == gameLocal.GetLocalPlayer() ) {
				// draw a tracer
				int effectHandle = gameLocal.GetEffectHandle( projectileDict, "fx_tracer", "" );
				idVec3 dir = trace.endpos - tracerMuzzleOrigin;
				dir.Normalize();

				rvClientEffect* tracerEffect = gameLocal.PlayEffect( effectHandle, idVec3( 1.0f, 1.0f, 1.0f ), tracerMuzzleOrigin, dir.ToMat3(), false, trace.endpos ); 
				if ( tracerOut != NULL ) {
					*tracerOut = tracerEffect;
				}
			}
		}
	}

	return damageDone;
}

/*
============
idEntity::GetDecalUsage
============
*/
cheapDecalUsage_t idEntity::GetDecalUsage( void ) {
	if ( GetModelDefHandle() < 0 || (GetPhysics()->GetType()->IsType( idPhysics_Static::Type ) && !fl.forceDecalUsageLocal)) {
		return CDU_WORLD;
	} else {
		return CDU_LOCAL;
	}
}

/*
============
idEntity::GetClipModelName
============
*/
const char* idEntity::GetClipModelName( void ) const {
	return spawnArgs.GetString( "cm_model" );
}

/*
============
idEntity::CheckEntityContentBoundsFilter
============
*/
bool idEntity::CheckEntityContentBoundsFilter( idEntity* other ) const {
	if ( contentBoundsFilter != NULL && other != NULL ) {
		return contentBoundsFilter->FilterEntity( other );
	}
	return false;
}

/*
============
idEntity::PlayHitBeep
============
*/
int idEntity::PlayHitBeep( idPlayer* player, bool headshot ) const {
	int duration = 0;
	if( GetEntityAllegiance( player ) == TA_ENEMY ) {
		player->StartSound( headshot ? "snd_hit_feedback_headshot" : "snd_hit_feedback", SND_ANY, SSF_PRIVATE_SOUND, &duration );
	} else {
		player->StartSound( "snd_hit_friendly_feedback", SND_ANY, SSF_PRIVATE_SOUND, &duration );
	}
	return duration;
}

/*
============
idEntity::Event_InCollection
============
*/
void idEntity::Event_InCollection( const char* name ) {
	const sdEntityCollection* collection = gameLocal.GetEntityCollection( name );
	if ( collection != NULL ) {
		if ( collection->Contains( this ) ) {
			sdProgram::ReturnBoolean( true );
			return;
		}
	}

	sdProgram::ReturnBoolean( false );
}

/*
============
idEntity::Event_GetEntityContents
============
*/
void idEntity::Event_GetEntityContents() {
	int contents;
	idClipModel *clipModel;

	contents = 0;
	for ( int i = 0; i < GetPhysics()->GetNumClipModels(); i++ ) {
		clipModel = GetPhysics()->GetClipModel( i );
		contents |= gameLocal.clip.Contents( CLIP_DEBUG_PARMS_SCRIPT GetPhysics()->GetOrigin( i ), clipModel, GetPhysics()->GetAxis( i ), GetPhysics()->GetClipMask( i ), this );
	}

	sdProgram::ReturnInteger( contents );
}

/*
============
idEntity::Event_GetMaskedEntityContents
============
*/
void idEntity::Event_GetMaskedEntityContents( int mask ) {
	int contents = 0;
	for ( int i = 0; i < GetPhysics()->GetNumClipModels(); i++ ) {
		idClipModel* clipModel = GetPhysics()->GetClipModel( i );
		contents |= gameLocal.clip.Contents( CLIP_DEBUG_PARMS_SCRIPT GetPhysics()->GetOrigin( i ), clipModel, GetPhysics()->GetAxis( i ), mask, this );
	}

	sdProgram::ReturnInteger( contents );
}

/*
============
idEntity::Event_SaveCachedEntities
============
*/
void idEntity::Event_SaveCachedEntities( void ) {
	for ( int i = 0; i < NUM_ENTITY_CAHCES; i++ ) {
		if ( savedEntityCache[ i ].first ) {
			continue;
		}

		savedEntityCache[ i ].first = true;
		savedEntityCache[ i ].second = scriptEntityCache;

		sdProgram::ReturnInteger( i );
		return;
	}
	sdProgram::ReturnInteger( -1 );
}

/*
============
idEntity::Event_FreeSavedCache
============
*/
void idEntity::Event_FreeSavedCache( int index ) {
	if ( index < 0 || index >= NUM_ENTITY_CAHCES ) {
		gameLocal.Warning( "idEntity::Event_FreeSavedCache Index out of Range '%d'", index );
		return;
	}

	if ( !savedEntityCache[ index ].first ) {
		gameLocal.Warning( "idEntity::Event_FreeSavedCache Index '%d' Already Freed", index );
		return;
	}
	savedEntityCache[ index ].first = false;
}

/*
============
idEntity::Event_GetSavedCacheCount
============
*/
void idEntity::Event_GetSavedCacheCount( int index ) {
	if ( index < 0 || index >= NUM_ENTITY_CAHCES ) {
		gameLocal.Warning( "idEntity::Event_GetSavedCacheCount Index out of Range '%d'", index );
		return;
	}

	assert( savedEntityCache[ index ].first );
	sdProgram::ReturnInteger( savedEntityCache[ index ].second.Num() );
}

/*
============
idEntity::Event_GetSavedCacheEntity
============
*/
void idEntity::Event_GetSavedCacheEntity( int index, int entityIndex ) {
	if ( index < 0 || index >= NUM_ENTITY_CAHCES ) {
		gameLocal.Warning( "idEntity::Event_GetSavedCacheEntity Index out of Range '%d'", index );
		return;
	}

	if ( entityIndex < 0 || entityIndex >= savedEntityCache[ index ].second.Num() ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	sdProgram::ReturnEntity( savedEntityCache[ index ].second[ entityIndex ] );
}

/*
===============
idEntity::Event_GetPositionPlayer
===============
*/
void idEntity::Event_GetPositionPlayer( int index ) {
	sdUsableInterface* iface = GetUsableInterface();
	if ( iface == NULL ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}

	if ( index < 0 || index >= iface->GetNumPositions() ) {
		sdProgram::ReturnEntity( NULL );
		return;
	}	
	sdProgram::ReturnEntity( iface->GetPlayerAtPosition( index ) );
}

/*
===============
idEntity::Event_GetDriver
===============
*/
void idEntity::Event_GetDriver( void ) {
	Event_GetPositionPlayer( 0 );
}

/*
===============
idEntity::Event_GetNumPositions
===============
*/
void idEntity::Event_GetNumPositions( void ) {
	sdUsableInterface* iface = GetUsableInterface();
	if ( iface == NULL ) {
		sdProgram::ReturnInteger( 0 );
		return;
	}

	sdProgram::ReturnInteger( iface->GetNumPositions() );
}

/*
===============
idEntity::Event_GetDamageScale
===============
*/
void idEntity::Event_GetDamageScale( void ) {
	sdProgram::ReturnFloat( GetDamageXPScale() );
}

/*
===============
idEntity::Event_GetJointHandle
===============
*/
void idEntity::Event_GetJointHandle( const char *jointname ) {
	if ( GetAnimator() != NULL ) {
		jointHandle_t joint;
		joint = GetAnimator()->GetJointHandle( jointname );
		sdProgram::ReturnInteger( joint );
	} else {
		sdProgram::ReturnInteger( INVALID_JOINT );
	}
}

/*
===============
idEntity::Event_GetDefaultSurfaceType
===============
*/
void idEntity::Event_GetDefaultSurfaceType( void ) {
	sdProgram::ReturnString( GetDefaultSurfaceType() );
}

/*
================
idEntity::Event_ForceRunPhysics
================
*/
void idEntity::Event_ForceRunPhysics( void ) {
	// run physics
	int startTime = gameLocal.previousTime;
	int endTime = gameLocal.time;
	for ( idEntity* ent = this; ent != NULL; ent = ent->GetNextTeamEntity() ) {
		if ( ent->GetPhysics()->Evaluate( endTime - startTime, endTime ) ) {
			ent->fl.allowPredictionErrorDecay = true;
			ent->UpdateVisuals();
			if ( ent->IsType( sdScriptEntity::Type ) ) {
				static_cast< sdScriptEntity* >( ent )->UpdateRadar();
			}
			ent->Present();
		}
	}
}

/*
===============
idEntity::UpdateOcclusionInfo
===============
*/
const occlusionTest_t& idEntity::UpdateOcclusionInfo( int viewId ) {
	assert( GetRenderEntity() );
	assert( GetPhysics() );

	occlusionQueryInfo.origin	= renderEntity.origin;
	occlusionQueryInfo.axis		= renderEntity.axis;
	occlusionQueryInfo.bb		= GetPhysics()->GetBounds();
	occlusionQueryInfo.bb.ExpandSelf( occlusionQueryInfo.bb.Size() * occlusionQueryBBScale );
	occlusionQueryInfo.view		= viewId;

	return occlusionQueryInfo;
}

/*
==============
idEntity::IsVisibleOcclusionTest
==============
*/
bool idEntity::IsVisibleOcclusionTest() {
	if ( occlusionQueryHandle != -1 ) {
		return gameRenderWorld->IsVisibleOcclusionTestDef( occlusionQueryHandle );
	}
	return true;
}

/*
==============
idEntity::OnControllerMove
==============
*/
void idEntity::OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
									const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {

	// default behaviour is just to handle it as a player would
	for ( int i = 0; i < numControllers; i++ ) {
		int num = controllerNumbers[ i ];
		sdInputModePlayer::ControllerMove( doGameCallback, num, controllerAxis[ i ], viewAngles, cmd );
	}
}

/*
============
idEntity::GetTaskName
============
*/
void idEntity::GetTaskName( idWStr& out ) {
	out = taskName == NULL ? L"" : taskName->GetText();
}

/*
============
idEntity::GetAORPhysicsLOD
============
*/
int idEntity::GetAORPhysicsLOD( void ) const {
	if ( aorFlags & AOR_PHYSICS_LOD_1 ) {
		return 1;
	} else if ( aorFlags & AOR_PHYSICS_LOD_2 ) {
		return 2;
	} else if ( aorFlags & AOR_PHYSICS_LOD_3 ) {
		return 3;
	}

	return 0;
}
