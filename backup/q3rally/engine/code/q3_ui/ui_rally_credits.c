/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2025 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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


typedef struct {
        menuframework_s menu;
} rallycreditsmenu_t;

static rallycreditsmenu_t       s_rally_credits;


/*
=================
UI_CreditMenu_Key
=================
*/
static sfxHandle_t UI_Rally_CreditMenu_Key( int key ) {
        if( key & K_CHAR_FLAG ) {
                return 0;
        }

        trap_Cmd_ExecuteText( EXEC_APPEND, "quit\n" );
        return 0;
}


/*
===============
UI_CreditMenu_Draw
===============
*/
static void UI_Rally_CreditMenu_Draw( void ) {
        int             y;

        y = (380 - (22 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE)) / 2;
        UI_DrawProportionalString( 320, y, "Q3Rally is brought to you by:", UI_CENTER|UI_SMALLFONT, color_white );


        y += 2.0 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Programming until V1.2:", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Steve \"Stone Lance\" Heijster", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Per \"P3rlE\" Thormann", UI_CENTER|UI_SMALLFONT, color_white );

        y += 2.0 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Art and Models:", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Jeff \"Stecki\" Garstecki", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "\"Steel Painter\"", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Per \"P3rlE\" Thormann", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "\"insellium\"", UI_CENTER|UI_SMALLFONT, color_white );
        
        y += 2.0 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Maps:", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Jeff \"Stecki\" Garstecki", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Jim \"gout\" Bahe", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Simon \"System Krash\" Batty", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Jonathan \"Amphetamine\" Garrod", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Michael \"Cyberdemon\" Kaminsky", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Per \"P3rlE\" Thormann", UI_CENTER|UI_SMALLFONT, color_white );

        y += 2.0 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "SDK Document Design and Layout:", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Richard Smith", UI_CENTER|UI_SMALLFONT, color_white );

        y += 2.0 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Special thanks to:", UI_CENTER|UI_SMALLFONT, color_white );
        y += PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawProportionalString( 320, y, "Cyberdemon, Killaz and skw|d", UI_CENTER|UI_SMALLFONT, color_white );

        y += 2.0 * PROP_HEIGHT * PROP_SMALL_SIZE_SCALE;
        UI_DrawString( 320, y, Q3_VERSION " | 2002 - 2025 | www.q3rally.com | It's damn fast baby!", UI_CENTER|UI_SMALLFONT, text_color_normal );
}


/*
===============
UI_Rally_CreditMenu
===============
*/
void UI_Rally_CreditMenu( void ) {

        uis.transitionIn = 0;

        memset( &s_rally_credits, 0 ,sizeof(s_rally_credits) );

        s_rally_credits.menu.draw = UI_Rally_CreditMenu_Draw;
        s_rally_credits.menu.key = UI_Rally_CreditMenu_Key;
        s_rally_credits.menu.fullscreen = qtrue;
        UI_PushMenu ( &s_rally_credits.menu );
}
