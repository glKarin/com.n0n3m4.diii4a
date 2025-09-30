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


#define SCROLLSPEED	3.20 // pixels/seconds

#define BACKGROUND_SHADER
// uncomment this for a background shader, otherwise a solid color
// defined in the vec4_t "color_background" is filled to the screen


typedef struct {
	menuframework_s	menu;
} creditsmenu_t;

static creditsmenu_t	s_credits;

int starttime; // game time at which credits are started
float mvolume; // records the original music volume level, as it is
               // modified for the credits

// change this to change the background colour on credits
vec4_t color_background	        = {0.00, 0.00, 0.00, 0.00};
// these are just example colours that are used in credits[] 
vec4_t color_headertext			= {1.00, 1.00, 1.00, 1.00};
vec4_t color_maintext			= {1.00, 1.00, 1.00, 1.00};

qhandle_t	BackgroundShader; // definition of the background shader pointer

/*
Constants to be used for the "style" field of the cr_line credits[] structure...
UI_LEFT - Align to the left of the screen
UI_CENTER - Align to the center
UI_RIGHT - Align to the right of the screen
UI_FORMATMASK - Not sure...
UI_SMALLFONT - Small font
UI_BIGFONT - Big font
UI_GIANTFONT - Giant font
UI_DROPSHADOW - A drop shadow is created behind the text
UI_BLINK - The text blinks
UI_INVERSE - The text is inverted?
UI_PULSE - The text pulses
*/

typedef struct
{
	char *string;
	int style;
	vec4_t *colour;
} cr_line;

cr_line credits[] = { // edit this as necessary for your credits
	
{ "Thank you for playing!", UI_CENTER|UI_BIGFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Q3Rally Team", UI_CENTER|UI_BIGFONT|UI_DROPSHADOW|UI_PULSE, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Programming:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW|UI_PULSE, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Steven 'Stonelance' Heijsters", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Per 'P3rlE' Thormann", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Eddy Valdez aka. 'TheBigBuu'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "ZTurtleMan from TurtleArena", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Eraser from EntityPlus", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Mapping:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW|UI_PULSE, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Jeff 'Stecki' Garstecki", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Jim 'gout' Bahe", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Simon 'System Krash' Batty", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Jonathan 'Amphetamine' Garrod", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Michael 'Cyberdemon' Kaminsky", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Per 'P3rlE' Thormann", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "'MysteriousPoetd' aka 'Poet'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "'OliverV'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Thomas aka. 'To-mos'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Eddy Valdez aka. 'thebigbuu'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "'ailmanki' aka. 'peyote'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Denis 'insellium' Manylov", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Art and Models", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW|UI_PULSE, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Jeff 'Stecki' Garstecki", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Per 'P3rlE' Thormann", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Denis 'insellium' Manylov", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "'Steel Painter'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Thomas aka. 'To-mos'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Sound Design and Music", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW|UI_PULSE, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "P.Andersson", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "F.Segerfalk", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Dale Wilson", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Thomas aka. 'To-mos'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Texture Design", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW|UI_PULSE, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Melanie aka. 'Toxicity'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Eddy Valdez aka. 'thebigbuu'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Denis 'insellium' Manylov", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Document Design and Layout", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW|UI_PULSE, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Richard Smith", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Per 'P3rlE' Thormann", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Mirco 'PaniC' Herrmann", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "BETA Testing", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW|UI_PULSE, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "To-Mos", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Eddy Valdez aka. 'thebigbuu'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Per 'P3rlE' Thormann", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Mirco 'PaniC' Herrmann", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Jeremiah aka. 'BETAMONKEY'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "'Onai'", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Denis 'insellium' Manylov", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "iD Software is:", UI_CENTER|UI_BIGFONT|UI_DROPSHADOW, &color_headertext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Programming:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "John Carmack, John Cash", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Art:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Adrian Carmack, Kevin Cloud,", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "Paul Steed, Kenneth Scott", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Game Designer:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Graeme Devine", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Level Design:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Tim Willits, Christian Antkow", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "Paul Jaquays", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "CEO:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Todd Hollenshead", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Director of Business Development:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Katherine Anna Kang", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },
{ "Biz Assist and id mom:", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_headertext },
{ "Donna Jackson", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_maintext },
{ "", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, &color_blue },

  {NULL}
};

/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_CreditMenu_Key( int key ) {
	if( key & K_CHAR_FLAG ) {
		return 0;
	}

	// pressing the escape key or clicking the mouse will exit
	// we also reset the music volume to the user's original
	// choice here,  by setting s_musicvolume to the stored var
	trap_Cmd_ExecuteText( EXEC_APPEND, 
                         va("s_musicvolume %f; quit\n", mvolume));
	return 0;
}

/*
=================
ScrollingCredits_Draw
This is the main drawing function for the credits.
Most of the code is self-explanatory.
=================
*/
static void ScrollingCredits_Draw(void)
{
  int x = 320, y, n, ysize = 0;

  // ysize is used to determine the entire length 
  // of the credits in pixels. 
  // We can then use this in further calculations
  if(!ysize) // ysize not calculated, so calculate it dammit!
  {
    // loop through entire credits array
    for(n = 0; n < ARRAY_LEN(credits); n++) 
    {
      // it is a small character
      if(credits[n].style & UI_SMALLFONT) 
      {
        // add small character height
        ysize += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        
      // it is a big character
      }else if(credits[n].style & UI_BIGFONT) 
      {
        // add big character size
        ysize += PROP_HEIGHT;
        
      // it is a huge character
      }else if(credits[n].style & UI_GIANTFONT) 
      {
        // add giant character size.
        ysize += PROP_HEIGHT * (1 / PROP_SMALL_SIZE_SCALE); 
      }
    }
  }

  // first, fill the background with the specified colour/shader
  // we are drawing a shader
#ifdef BACKGROUND_SHADER
  UI_DrawHandlePic(-uis.bias, 0, SCREEN_WIDTH+uis.bias*2, SCREEN_HEIGHT, BackgroundShader);

#else
  // we are just filling a color
  UI_FillRect(-uis.bias, 0, SCREEN_WIDTH+uis.bias*2, SCREEN_HEIGHT, color_background);
#endif


  // let's draw the stuff
  // set initial y location
  y = 480 - SCROLLSPEED * (float)(uis.realtime - starttime) / 100;
  
  // loop through the entire credits sequence
  for(n = 0; n < ARRAY_LEN(credits); n++)
  {
    // this NULL string marks the end of the credits struct
    if(credits[n].string == NULL) 
    {
      if(y < -16) // credits sequence is completely off screen
      {
        trap_Cmd_ExecuteText( EXEC_APPEND, 
                         va("s_musicvolume %f; quit\n", mvolume));
        break; // end of credits
      }
      break;
    }
		
    if( strlen(credits[n].string) == 1) // spacer string, no need to draw
      continue;

    if( y > -(PROP_HEIGHT * (1 / PROP_SMALL_SIZE_SCALE))) 
      // the line is within the visible range of the screen
      UI_DrawProportionalString(x, y, credits[n].string, 
                                credits[n].style, *credits[n].colour );
		
    // re-adjust y for next line
    if(credits[n].style & UI_SMALLFONT)
    {
      y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
    }else if(credits[n].style & UI_BIGFONT)
    {
      y += PROP_HEIGHT;
    }else if(credits[n].style & UI_GIANTFONT)
    {
      y += PROP_HEIGHT * (1 / PROP_SMALL_SIZE_SCALE);
    }

	// if y is off the screen, break out of loop
    if (y > 480)
    break;
  }
}

/*
===============
UI_CreditMenu
===============
*/
void UI_CreditMenu( void ) {
	memset( &s_credits, 0 ,sizeof(s_credits) );

	s_credits.menu.draw = ScrollingCredits_Draw;
	s_credits.menu.key = UI_CreditMenu_Key;
	s_credits.menu.fullscreen = qtrue;
	UI_PushMenu ( &s_credits.menu );

	starttime = uis.realtime; // record start time for credits to scroll properly
	mvolume = trap_Cvar_VariableValue( "s_musicvolume" );
	if(mvolume < 0.5)
		trap_Cmd_ExecuteText( EXEC_APPEND, "s_musicvolume 0.5\n" );
	trap_Cmd_ExecuteText( EXEC_APPEND, "music music/credits\n" );

	// load the background shader
#ifdef BACKGROUND_SHADER
	BackgroundShader =
		trap_R_RegisterShaderNoMip("menu/art/menu_back");
#endif
}
