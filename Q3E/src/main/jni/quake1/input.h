/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
/// \file input.h -- external (non-keyboard) input devices

#ifndef INPUT_H
#define INPUT_H

#include "qtypes.h"
#include "cvar.h"

extern cvar_t in_pitch_min;
extern cvar_t in_pitch_max;

extern qbool in_client_mouse;
extern float in_windowmouse_x, in_windowmouse_y;
extern float in_mouse_x, in_mouse_y;

//enum input_dest_e {input_game,input_message,input_menu} input_dest;

void IN_Move (void);
// add additional movement on top of the keyboard move cmd

#define IN_BESTWEAPON_MAX 32
typedef struct
{
	char name[32];
	int impulse;
	int activeweaponcode;
	int weaponbit;
	int ammostat;
	int ammomin;
	/// \TODO add a parameter for the picture to be used by the sbar, and use it there
}
in_bestweapon_info_t;
extern in_bestweapon_info_t in_bestweapon_info[IN_BESTWEAPON_MAX];
void IN_BestWeapon_ResetData(void); ///< call before each map so QC can start from a clean state

#endif

