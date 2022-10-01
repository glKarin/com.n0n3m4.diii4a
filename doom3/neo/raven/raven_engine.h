#ifndef _KARIN_RAVEN_ENGINE_H
#define _KARIN_RAVEN_ENGINE_H

#include "idlib/TextCompiler.h"
#include "idlib/math/Radians.h"
#include "bse/BSEInterface.h"

// RAVEN BEGIN
// AReis: Render flags.
enum
{
	RF_NORMAL					= 0,
	// Let the renderer know its in a editor of sorts.
	RF_IS_EDITOR				= BIT( 0 ),
	// Viewdef is fullscreen and is 2d (mostly for main menu)
	RF_IS_FULLSCREEN_2D			= BIT( 1 ),
	// Don't draw the GUI, just the world.
	RF_NO_GUI					= BIT( 2 ),
	// Only draw the GUI, not the world.
	RF_GUI_ONLY					= BIT( 3 ),
// RAVEN BEGIN
// dluetscher: added render flag to denote that penumbra map rendering is desired 
	RF_PENUMBRA_MAP				= BIT( 4 ),
// dluetscher: added render flag that defers the command buffer submission of render 
//			   commands until the first non-deferred render command (with the exception of 
//			   certain render commands like RC_DRAW_PENUMBRA_MAPS - which ignores deferred
//			   render commands and lets them get submitted past)
	RF_DEFER_COMMAND_SUBMIT		= BIT( 5 ),
	// this is a portal sky view
	RF_PORTAL_SKY				= BIT( 6 ),
	// this the primary view - when this is rendered, we then know we can capture the screen buffer
	RF_PRIMARY_VIEW				= BIT( 7 ),
// RAVEN END
};

// RAVEN END

#include "framework/declLipSync.h"
#include "framework/declMatType.h"
#include "framework/declPlayback.h"
#include "framework/DeclPlayerModel.h"

// RAVEN BEGIN
// dluetscher: added a default value for light detail levels
#define DEFAULT_LIGHT_DETAIL_LEVEL	10.f
// RAVEN END 

enum {
// RAVEN BEGIN
// bdube: new clip that blocks monster visibility
	CONTENTS_SIGHTCLIP			= BIT(16),	// used for blocking sight for actors and cameras
	CONTENTS_LARGESHOTCLIP		= BIT(17),	// used to block large shots (fence that allows bullets through but not rockets for example)
// cdr: AASTactical
	CONTENTS_NOTACTICALFEATURES	= BIT(18),	// don't place tactical features here
	CONTENTS_VEHICLECLIP		= BIT(19),	// solid to vehicles

	CONTENTS_FLYCLIP			= BIT(22),	// solid to vehicles

// mekberg: added
	CONTENTS_ITEMCLIP			= BIT(23),	// so items can collide
	CONTENTS_PROJECTILECLIP		= BIT(24),  // unlike contents_projectile, projectiles only NOT hitscans
// RAVEN END
};

// RAVEN BEGIN
static const int        SSF_USEDOPPLER = BIT(10);       // allow doppler pitch shifting effects
static const int        SSF_NO_RANDOMSTART = BIT(11);   // don't offset the start position for looping sounds
static const int        SSF_VO_FOR_PLAYER = BIT(12);    // Notifies a funcRadioChatter that this shader is directed at the player
static const int        SSF_IS_VO = BIT(13);    // this sound is VO
static const int        SSF_CAUSE_RUMBLE = BIT(14);     // causes joystick rumble
static const int        SSF_CENTER = BIT(15);   // sound through center channel only
static const int        SSF_HILITE = BIT(16);   // display debug info for this emitter
// RAVEN END

#endif

