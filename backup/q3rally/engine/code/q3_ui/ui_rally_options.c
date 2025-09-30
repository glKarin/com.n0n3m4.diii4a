/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "ui_local.h"

qboolean isRaceObserver( int clientNum )
{
	return qfalse;
}

#define ID_UNITS                10
#define ID_SKID_LENGTH          11
#define ID_CONTROL_MODE         12
#define ID_MANUAL_SHIFT         13
#define ID_REAR_VIEW            14
#define ID_CP_ARROW_MODE        15
#define ID_ATMOSPHERIC_LEVEL	16
#define ID_POSITION_SPRITES     17
#define ID_CAM_TRACKING         18
#define ID_ENGINE_SOUNDS        19
#define ID_DRAW_MINIMAP         20

#define ID_RVRL_PLAYERS         21
#define ID_RVRL_OBJECTS         22
#define ID_RVRL_SMOKE           23
#define ID_RVRL_MARKS           24
#define ID_RVRL_SPARKS          25

#define ID_MVRL_PLAYERS         30
#define ID_MVRL_OBJECTS         31
#define ID_MVRL_SMOKE           32
#define ID_MVRL_MARKS           33
#define ID_MVRL_SPARKS          34

#define ID_BACK                 40

typedef struct {
	menuframework_s	menu;

	menutext_s		banner;

	menulist_s		units;
	menulist_s		arrowMode;
	menulist_s		controlMode;
	menulist_s		atomspheric;

	menuradiobutton_s	manualShift;
	menuradiobutton_s	rearView;
	menuradiobutton_s	positionSprites;
	menuslider_s		skidlength;
	menuslider_s		camtracking;

	menuradiobutton_s	rvrl_players;
	menuradiobutton_s	rvrl_objects;
	menuradiobutton_s	rvrl_smoke;
	menuradiobutton_s	rvrl_marks;
	menuradiobutton_s	rvrl_sparks;

	menuradiobutton_s	mvrl_players;
	menuradiobutton_s	mvrl_objects;
	menuradiobutton_s	mvrl_smoke;
	menuradiobutton_s	mvrl_marks;
	menuradiobutton_s	mvrl_sparks;

	menutext_s		rvrl_heading;
	menutext_s		mvrl_heading;

	menuradiobutton_s	engineSounds;
    menuradiobutton_s   drawMinimap;

	menutext_s		back;
} q3roptionsmenu_t;

static q3roptionsmenu_t	s_q3roptions;

static const char *q3roptions_units[] = {
	"Imperial",
	"Metric",
	0
};

static const char *q3roptions_control_mode[] = {
	"Mouse",
	"Keyboard/Joystick",
	0
};

static const char *q3roptions_cp_arrow_mode[] = {
	"Off",
	"On HUD",
	"Above Car",
	0
};

static const char *q3roptions_atmospheric[] = {
	"None",
	"Low",
	"High",
	0
};


/*
=================
Q3ROptions_MenuEvent
=================
*/
static void Q3ROptions_MenuEvent( void* ptr, int event ) {
	int		value;


	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id )
	{
	case ID_UNITS:
		trap_Cvar_SetValue( "cg_metricUnits", s_q3roptions.units.curvalue );
		break;

	case ID_CP_ARROW_MODE:
		trap_Cvar_SetValue( "cg_checkpointArrowMode", s_q3roptions.arrowMode.curvalue );
		break;

	case ID_CONTROL_MODE:
		trap_Cvar_SetValue( "cg_controlMode", s_q3roptions.controlMode.curvalue );
		if ( s_q3roptions.controlMode.curvalue == 0 )
			trap_Cmd_ExecuteText( EXEC_APPEND, "-strafe" );
		else
			trap_Cmd_ExecuteText( EXEC_APPEND, "+strafe" );

		break;

	case ID_ATMOSPHERIC_LEVEL:
		trap_Cvar_SetValue( "cg_atmosphericLevel", s_q3roptions.atomspheric.curvalue );
		break;



	case ID_MANUAL_SHIFT:
		trap_Cvar_SetValue( "cg_manualShift", s_q3roptions.manualShift.curvalue );
		break;

	case ID_REAR_VIEW:
		trap_Cvar_SetValue( "cg_drawRearView", s_q3roptions.rearView.curvalue );
		break;



	case ID_POSITION_SPRITES:
		trap_Cvar_SetValue( "cg_drawPositionSprites", s_q3roptions.positionSprites.curvalue );
		break;

	case ID_SKID_LENGTH:
		trap_Cvar_SetValue( "cg_minSkidLength", s_q3roptions.skidlength.curvalue );
		break;

	case ID_CAM_TRACKING:
		trap_Cvar_SetValue( "cg_tightCamTracking", s_q3roptions.camtracking.curvalue );
		break;


	case ID_ENGINE_SOUNDS:
		trap_Cvar_SetValue( "cg_engineSounds", s_q3roptions.engineSounds.curvalue );
		break;

    case ID_DRAW_MINIMAP:
        trap_Cvar_SetValue( "cg_drawMMap", s_q3roptions.drawMinimap.curvalue );
        break;

	case ID_RVRL_PLAYERS:
	case ID_RVRL_OBJECTS:
	case ID_RVRL_SMOKE:
	case ID_RVRL_MARKS:
	case ID_RVRL_SPARKS:
		value = 0;
		value |= ( s_q3roptions.rvrl_players.curvalue ) ? RL_PLAYERS : 0;
		value |= ( s_q3roptions.rvrl_objects.curvalue ) ? RL_OBJECTS : 0;
		value |= ( s_q3roptions.rvrl_smoke.curvalue ) ? RL_SMOKE : 0;
		value |= ( s_q3roptions.rvrl_marks.curvalue ) ? RL_MARKS : 0;
		value |= ( s_q3roptions.rvrl_sparks.curvalue ) ? RL_SPARKS : 0;

		trap_Cvar_SetValue( "cg_rearViewRenderLevel", value );
		break;



	case ID_MVRL_PLAYERS:
	case ID_MVRL_OBJECTS:
	case ID_MVRL_SMOKE:
	case ID_MVRL_MARKS:
	case ID_MVRL_SPARKS:
		value = 0;
		value |= ( s_q3roptions.mvrl_players.curvalue ) ? RL_PLAYERS : 0;
		value |= ( s_q3roptions.mvrl_objects.curvalue ) ? RL_OBJECTS : 0;
		value |= ( s_q3roptions.mvrl_smoke.curvalue ) ? RL_SMOKE : 0;
		value |= ( s_q3roptions.mvrl_marks.curvalue ) ? RL_MARKS : 0;
		value |= ( s_q3roptions.mvrl_sparks.curvalue ) ? RL_SPARKS : 0;

		trap_Cvar_SetValue( "cg_mainViewRenderLevel", value );
		break;


	case ID_BACK:
		UI_PopMenu();
		break;
	}
}


/*
=================
Q3ROptions_StatusBar
=================
*/
static void Q3ROptions_StatusBar( void *self )
{
	char	*text;

	text = "Use Arrow Keys or CLICK to change.";

	switch( ((menucommon_s*)self)->id )
	{
	case ID_UNITS:
		text = "Use KPH or MPH on the speedometer.";
		break;

	case ID_CP_ARROW_MODE:
		text = "Display options for the next checkpoint arrow.";
		break;

	case ID_CONTROL_MODE:
		if( s_q3roptions.controlMode.curvalue == 0 )
			text = "Mouse control allows a freelook like normal Q3A and the Car turns towards the direction you are looking.";
		else
			text = "Keyboard/Joystick control maps the axis directly to the wheel angle.";
		break;

	case ID_ATMOSPHERIC_LEVEL:
		text = "Determines the relative number of environment particles to show.";
		break;



	case ID_MANUAL_SHIFT:
		if( s_q3roptions.manualShift.curvalue == 0 )
			text = "The car will automatically shift from forward to reverse when you stop.";
		else
			text = "Use the gear up and gear down keys to switch between forward and reverse manually.";
		break;

	case ID_REAR_VIEW:
		text = "Draw rear view mirror on the HUD.";
		break;



	case ID_POSITION_SPRITES:
		text = "Draw position sprites above cars when their position changes.";
		break;

	case ID_SKID_LENGTH:
		text = "The length of the skid segments.  A lower value will make the skid marks look smoother.";
		break;

	case ID_CAM_TRACKING:
		text = "Tightness of the joystick camera tracking mode.  A higher value makes the camera stay behind the car more.";
		break;

	case ID_ENGINE_SOUNDS:
		text = "Turns on engine sounds in game.";
		break;

    case ID_DRAW_MINIMAP:
        text = "Turns on the Minimap in game.";
        break;

	case ID_RVRL_PLAYERS:
		text = "Show other players in the rear view mirror.";
		break;
	case ID_RVRL_OBJECTS:
		text = "Show map objects in the rear view mirror.";
		break;
	case ID_RVRL_SMOKE:
		text = "Show smoke in the rear view mirror.";
		break;
	case ID_RVRL_MARKS:
		text = "Show skidmarks in the rear view mirror.";
		break;
	case ID_RVRL_SPARKS:
		text = "Show sparks in the rear view mirror.";
		break;



	case ID_MVRL_PLAYERS:
		text = "Show other players in the main view.";
		break;
	case ID_MVRL_OBJECTS:
		text = "Show map objects in the main view.";
		break;
	case ID_MVRL_SMOKE:
		text = "Show smoke in the main view.";
		break;
	case ID_MVRL_MARKS:
		text = "Show marks in the main view.";
		break;
	case ID_MVRL_SPARKS:
		text = "Show sparks in the main view.";
		break;
	}

	UI_DrawString( SCREEN_WIDTH * 0.50f, SCREEN_HEIGHT * 0.85f, text, UI_SMALLFONT|UI_CENTER, colorWhite );
}


/*
===============
Q3ROptions_MenuInit
===============
*/
void Q3ROptions_MenuInit( void ) {
//	int				y;

	memset( &s_q3roptions, 0, sizeof(q3roptionsmenu_t) );
	s_q3roptions.menu.wrapAround = qtrue;
	s_q3roptions.menu.fullscreen = qtrue;


	// load current values
	s_q3roptions.units.curvalue = ui_metricUnits.integer;
	s_q3roptions.arrowMode.curvalue = ui_checkpointArrowMode.integer;
	s_q3roptions.controlMode.curvalue = ui_controlMode.integer;
	s_q3roptions.atomspheric.curvalue = ui_atmosphericLevel.integer;

	s_q3roptions.manualShift.curvalue = ui_manualShift.integer;
	s_q3roptions.rearView.curvalue = ui_drawRearView.integer;
	s_q3roptions.positionSprites.curvalue = ui_drawPositionSprites.integer;

	s_q3roptions.skidlength.curvalue = ui_minSkidLength.integer;
	s_q3roptions.camtracking.curvalue = ui_tightCamTracking.integer;

	s_q3roptions.engineSounds.curvalue = ui_engineSounds.integer;
    
    s_q3roptions.drawMinimap.curvalue = ui_drawMinimap.integer;

	s_q3roptions.rvrl_players.curvalue = ( ui_rearViewRenderLevel.integer & RL_PLAYERS ) ? 1 : 0;
	s_q3roptions.rvrl_objects.curvalue = ( ui_rearViewRenderLevel.integer & RL_OBJECTS ) ? 1 : 0;
	s_q3roptions.rvrl_smoke.curvalue = ( ui_rearViewRenderLevel.integer & RL_SMOKE ) ? 1 : 0;
	s_q3roptions.rvrl_marks.curvalue = ( ui_rearViewRenderLevel.integer & RL_MARKS ) ? 1 : 0;
	s_q3roptions.rvrl_sparks.curvalue = ( ui_rearViewRenderLevel.integer & RL_SPARKS ) ? 1 : 0;

	s_q3roptions.mvrl_players.curvalue = ( ui_mainViewRenderLevel.integer & RL_PLAYERS ) ? 1 : 0;
	s_q3roptions.mvrl_objects.curvalue = ( ui_mainViewRenderLevel.integer & RL_OBJECTS ) ? 1 : 0;
	s_q3roptions.mvrl_smoke.curvalue = ( ui_mainViewRenderLevel.integer & RL_SMOKE ) ? 1 : 0;
	s_q3roptions.mvrl_marks.curvalue = ( ui_mainViewRenderLevel.integer & RL_MARKS ) ? 1 : 0;
	s_q3roptions.mvrl_sparks.curvalue = ( ui_mainViewRenderLevel.integer & RL_SPARKS ) ? 1 : 0;






	s_q3roptions.banner.generic.type	= MTYPE_BTEXT;
	s_q3roptions.banner.generic.flags	= QMF_CENTER_JUSTIFY;
	s_q3roptions.banner.generic.x		= 320;
	s_q3roptions.banner.generic.y		= 16;
	s_q3roptions.banner.string		    = "Q3R OPTIONS ";
	s_q3roptions.banner.color			= color_white;
	s_q3roptions.banner.style			= UI_CENTER;



	// lists
	s_q3roptions.units.generic.type			= MTYPE_SPINCONTROL;
	s_q3roptions.units.generic.name			= "Unit Type:";
	s_q3roptions.units.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_q3roptions.units.generic.callback		= Q3ROptions_MenuEvent;
	s_q3roptions.units.generic.statusbar	= Q3ROptions_StatusBar;
	s_q3roptions.units.generic.id			= ID_UNITS;
	s_q3roptions.units.generic.x			= 200;
	s_q3roptions.units.generic.y			= 90;
	s_q3roptions.units.itemnames			= q3roptions_units;

	s_q3roptions.arrowMode.generic.type			= MTYPE_SPINCONTROL;
	s_q3roptions.arrowMode.generic.name			= "Checkpoint Arrow Mode:";
	s_q3roptions.arrowMode.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_q3roptions.arrowMode.generic.callback		= Q3ROptions_MenuEvent;
	s_q3roptions.arrowMode.generic.statusbar	= Q3ROptions_StatusBar;
	s_q3roptions.arrowMode.generic.id			= ID_CP_ARROW_MODE;
	s_q3roptions.arrowMode.generic.x			= 200;
	s_q3roptions.arrowMode.generic.y			= 90 + 20;
	s_q3roptions.arrowMode.itemnames			= q3roptions_cp_arrow_mode;

	s_q3roptions.controlMode.generic.type		= MTYPE_SPINCONTROL;
	s_q3roptions.controlMode.generic.name		= "Control Mode:";
	s_q3roptions.controlMode.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_q3roptions.controlMode.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.controlMode.generic.statusbar	= Q3ROptions_StatusBar;
	s_q3roptions.controlMode.generic.id			= ID_CONTROL_MODE;
	s_q3roptions.controlMode.generic.x			= 200;
	s_q3roptions.controlMode.generic.y			= 90 + 40;
	s_q3roptions.controlMode.itemnames			= q3roptions_control_mode;

	s_q3roptions.atomspheric.generic.type		= MTYPE_SPINCONTROL;
	s_q3roptions.atomspheric.generic.name		= "Atmospheric Level:";
	s_q3roptions.atomspheric.generic.flags		= QMF_PULSEIFFOCUS|QMF_SMALLFONT;
	s_q3roptions.atomspheric.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.atomspheric.generic.statusbar	= Q3ROptions_StatusBar;
	s_q3roptions.atomspheric.generic.id			= ID_ATMOSPHERIC_LEVEL;
	s_q3roptions.atomspheric.generic.x			= 200;
	s_q3roptions.atomspheric.generic.y			= 90 + 60;
	s_q3roptions.atomspheric.itemnames			= q3roptions_atmospheric;



	// radio buttons
	s_q3roptions.manualShift.generic.type			= MTYPE_RADIOBUTTON;
	s_q3roptions.manualShift.generic.flags			= QMF_SMALLFONT;
	s_q3roptions.manualShift.generic.x				= 500;
	s_q3roptions.manualShift.generic.y				= 90;
	s_q3roptions.manualShift.generic.name			= "Manual Forward/Reverse:";
	s_q3roptions.manualShift.generic.id				= ID_MANUAL_SHIFT;
	s_q3roptions.manualShift.generic.callback		= Q3ROptions_MenuEvent;
	s_q3roptions.manualShift.generic.statusbar		= Q3ROptions_StatusBar;

	s_q3roptions.rearView.generic.type				= MTYPE_RADIOBUTTON;
	s_q3roptions.rearView.generic.flags				= QMF_SMALLFONT;
	s_q3roptions.rearView.generic.x					= 500;
	s_q3roptions.rearView.generic.y					= 90 + 20;
	s_q3roptions.rearView.generic.name				= "Rear View Mirror:";
	s_q3roptions.rearView.generic.id				= ID_REAR_VIEW;
	s_q3roptions.rearView.generic.callback			= Q3ROptions_MenuEvent;
	s_q3roptions.rearView.generic.statusbar			= Q3ROptions_StatusBar;

	s_q3roptions.positionSprites.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.positionSprites.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.positionSprites.generic.x			= 500;
	s_q3roptions.positionSprites.generic.y			= 90 + 40;
	s_q3roptions.positionSprites.generic.name		= "Race Position Sprites:";
	s_q3roptions.positionSprites.generic.id			= ID_POSITION_SPRITES;
	s_q3roptions.positionSprites.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.positionSprites.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.engineSounds.generic.type			= MTYPE_RADIOBUTTON;
	s_q3roptions.engineSounds.generic.flags			= QMF_SMALLFONT;
	s_q3roptions.engineSounds.generic.x				= 500;
	s_q3roptions.engineSounds.generic.y				= 90 + 60;
	s_q3roptions.engineSounds.generic.name			= "Engine Sounds:";
	s_q3roptions.engineSounds.generic.id			= ID_ENGINE_SOUNDS;
	s_q3roptions.engineSounds.generic.callback		= Q3ROptions_MenuEvent;
	s_q3roptions.engineSounds.generic.statusbar		= Q3ROptions_StatusBar;

    s_q3roptions.drawMinimap.generic.type           = MTYPE_RADIOBUTTON;
    s_q3roptions.drawMinimap.generic.flags          = QMF_SMALLFONT;
    s_q3roptions.drawMinimap.generic.x              = 500;
    s_q3roptions.drawMinimap.generic.y              = 90 + 80;
    s_q3roptions.drawMinimap.generic.name           = "Minimap:";
    s_q3roptions.drawMinimap.generic.id             = ID_DRAW_MINIMAP;
    s_q3roptions.drawMinimap.generic.callback       = Q3ROptions_MenuEvent;
    s_q3roptions.drawMinimap.generic.statusbar      = Q3ROptions_StatusBar;
    
	// sliders
	s_q3roptions.skidlength.generic.type		= MTYPE_SLIDER;
	s_q3roptions.skidlength.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.skidlength.generic.x			= 200;
	s_q3roptions.skidlength.generic.y			= 200;
	s_q3roptions.skidlength.generic.name		= "Skid Segment Length:";
	s_q3roptions.skidlength.generic.id			= ID_SKID_LENGTH;
	s_q3roptions.skidlength.minvalue			= 4;
	s_q3roptions.skidlength.maxvalue			= 24;
	s_q3roptions.skidlength.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.skidlength.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.camtracking.generic.type		= MTYPE_SLIDER;
	s_q3roptions.camtracking.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.camtracking.generic.x			= 500;
	s_q3roptions.camtracking.generic.y			= 200;
	s_q3roptions.camtracking.generic.name		= "Camera Tracking Scale:";
	s_q3roptions.camtracking.generic.id			= ID_CAM_TRACKING;
	s_q3roptions.camtracking.minvalue			= 0;
	s_q3roptions.camtracking.maxvalue			= 5;
	s_q3roptions.camtracking.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.camtracking.generic.statusbar	= Q3ROptions_StatusBar;



	// render levels
	s_q3roptions.rvrl_heading.generic.type		= MTYPE_PTEXT;
	s_q3roptions.rvrl_heading.generic.flags		= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_q3roptions.rvrl_heading.generic.x			= 200-20;
	s_q3roptions.rvrl_heading.generic.y			= 240;
	s_q3roptions.rvrl_heading.generic.id		= 0;
	s_q3roptions.rvrl_heading.generic.callback	= Q3ROptions_MenuEvent; 
	s_q3roptions.rvrl_heading.string			= "Rear View Render Level:";
	s_q3roptions.rvrl_heading.color				= text_color_normal;
	s_q3roptions.rvrl_heading.style				= UI_CENTER | UI_SMALLFONT;

	s_q3roptions.rvrl_players.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.rvrl_players.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.rvrl_players.generic.x			= 200;
	s_q3roptions.rvrl_players.generic.y			= 260;
	s_q3roptions.rvrl_players.generic.name		= "Players:";
	s_q3roptions.rvrl_players.generic.id		= ID_RVRL_PLAYERS;
	s_q3roptions.rvrl_players.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.rvrl_players.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.rvrl_objects.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.rvrl_objects.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.rvrl_objects.generic.x			= 200;
	s_q3roptions.rvrl_objects.generic.y			= 260 + 20;
	s_q3roptions.rvrl_objects.generic.name		= "Objects:";
	s_q3roptions.rvrl_objects.generic.id		= ID_RVRL_OBJECTS;
	s_q3roptions.rvrl_objects.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.rvrl_objects.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.rvrl_smoke.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.rvrl_smoke.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.rvrl_smoke.generic.x			= 200;
	s_q3roptions.rvrl_smoke.generic.y			= 260 + 40;
	s_q3roptions.rvrl_smoke.generic.name		= "Smoke:";
	s_q3roptions.rvrl_smoke.generic.id			= ID_RVRL_SMOKE;
	s_q3roptions.rvrl_smoke.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.rvrl_smoke.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.rvrl_marks.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.rvrl_marks.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.rvrl_marks.generic.x			= 200;
	s_q3roptions.rvrl_marks.generic.y			= 260 + 60;
	s_q3roptions.rvrl_marks.generic.name		= "Marks:";
	s_q3roptions.rvrl_marks.generic.id			= ID_RVRL_MARKS;
	s_q3roptions.rvrl_marks.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.rvrl_marks.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.rvrl_sparks.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.rvrl_sparks.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.rvrl_sparks.generic.x			= 200;
	s_q3roptions.rvrl_sparks.generic.y			= 260 + 80;
	s_q3roptions.rvrl_sparks.generic.name		= "Sparks:";
	s_q3roptions.rvrl_sparks.generic.id			= ID_RVRL_SPARKS;
	s_q3roptions.rvrl_sparks.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.rvrl_sparks.generic.statusbar	= Q3ROptions_StatusBar;



	s_q3roptions.mvrl_heading.generic.type		= MTYPE_PTEXT;
	s_q3roptions.mvrl_heading.generic.flags		= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_q3roptions.mvrl_heading.generic.x			= 500-20;
	s_q3roptions.mvrl_heading.generic.y			= 240;
	s_q3roptions.mvrl_heading.generic.id		= 0;
	s_q3roptions.mvrl_heading.generic.callback	= Q3ROptions_MenuEvent; 
	s_q3roptions.mvrl_heading.string			= "Main View Render Level:";
	s_q3roptions.mvrl_heading.color				= text_color_normal;
	s_q3roptions.mvrl_heading.style				= UI_CENTER | UI_SMALLFONT;

	s_q3roptions.mvrl_players.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.mvrl_players.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.mvrl_players.generic.x			= 500;
	s_q3roptions.mvrl_players.generic.y			= 260;
	s_q3roptions.mvrl_players.generic.name		= "Players:";
	s_q3roptions.mvrl_players.generic.id		= ID_MVRL_PLAYERS;
	s_q3roptions.mvrl_players.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.mvrl_players.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.mvrl_objects.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.mvrl_objects.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.mvrl_objects.generic.x			= 500;
	s_q3roptions.mvrl_objects.generic.y			= 260 + 20;
	s_q3roptions.mvrl_objects.generic.name		= "Objects:";
	s_q3roptions.mvrl_objects.generic.id		= ID_MVRL_OBJECTS;
	s_q3roptions.mvrl_objects.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.mvrl_objects.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.mvrl_smoke.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.mvrl_smoke.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.mvrl_smoke.generic.x			= 500;
	s_q3roptions.mvrl_smoke.generic.y			= 260 + 40;
	s_q3roptions.mvrl_smoke.generic.name		= "Smoke:";
	s_q3roptions.mvrl_smoke.generic.id			= ID_MVRL_SMOKE;
	s_q3roptions.mvrl_smoke.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.mvrl_smoke.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.mvrl_marks.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.mvrl_marks.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.mvrl_marks.generic.x			= 500;
	s_q3roptions.mvrl_marks.generic.y			= 260 + 60;
	s_q3roptions.mvrl_marks.generic.name		= "Marks:";
	s_q3roptions.mvrl_marks.generic.id			= ID_MVRL_MARKS;
	s_q3roptions.mvrl_marks.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.mvrl_marks.generic.statusbar	= Q3ROptions_StatusBar;

	s_q3roptions.mvrl_sparks.generic.type		= MTYPE_RADIOBUTTON;
	s_q3roptions.mvrl_sparks.generic.flags		= QMF_SMALLFONT;
	s_q3roptions.mvrl_sparks.generic.x			= 500;
	s_q3roptions.mvrl_sparks.generic.y			= 260 + 80;
	s_q3roptions.mvrl_sparks.generic.name		= "Sparks:";
	s_q3roptions.mvrl_sparks.generic.id			= ID_MVRL_SPARKS;
	s_q3roptions.mvrl_sparks.generic.callback	= Q3ROptions_MenuEvent;
	s_q3roptions.mvrl_sparks.generic.statusbar	= Q3ROptions_StatusBar;



	s_q3roptions.back.generic.type			= MTYPE_PTEXT;
	s_q3roptions.back.generic.flags			= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_q3roptions.back.generic.x				= 20;
	s_q3roptions.back.generic.y				= 480 - 50;
	s_q3roptions.back.generic.id			= ID_BACK;
	s_q3roptions.back.generic.callback		= Q3ROptions_MenuEvent; 
	s_q3roptions.back.string				= "< BACK";
	s_q3roptions.back.color					= text_color_normal;
	s_q3roptions.back.style					= UI_LEFT | UI_SMALLFONT;



	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.banner );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.back );


	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.units );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.arrowMode );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.controlMode );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.atomspheric );

	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.manualShift );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.rearView );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.positionSprites );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.engineSounds );
    Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.drawMinimap );

	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.skidlength );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.camtracking );

	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.rvrl_heading );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.rvrl_players );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.rvrl_objects );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.rvrl_smoke );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.rvrl_marks );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.rvrl_sparks );

	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.mvrl_heading );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.mvrl_players );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.mvrl_objects );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.mvrl_smoke );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.mvrl_marks );
	Menu_AddItem( &s_q3roptions.menu, ( void * ) &s_q3roptions.mvrl_sparks );
}


/*
===============
UI_SystemConfigMenu
===============
*/
void UI_Q3ROptionsMenu( void )
{
// STONELANCE FIXME: get rid of this after proper tansitions are added
	uis.transitionIn = 0;
// END

	Q3ROptions_MenuInit();
	UI_PushMenu ( &s_q3roptions.menu );
}
