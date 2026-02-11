/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../../idlib/precompiled.h"
#include "../posix/posix_public.h"
#include "local.h"

#include <pthread.h>

// toggled by grab calls - decides if we ignore MotionNotify events
static bool mouse_active = false;

idCVar in_mouse("in_mouse", "1", CVAR_SYSTEM | CVAR_ARCHIVE, "");
idCVar in_dgamouse("in_dgamouse", "1", CVAR_SYSTEM | CVAR_ARCHIVE, "");
idCVar in_nograb("in_nograb", "0", CVAR_SYSTEM | CVAR_NOCHEAT, "");

void IN_Clear_f(const idCmdArgs &args)
{
	idKeyInput::ClearStates();
}

void Sys_InitInput(void)
{
}

void Sys_GrabMouseCursor(bool grabIt)
{
	if ( grabIt && !mouse_active ) {
		Android_GrabMouseCursor(true);
		mouse_active = true;
	} else if ( !grabIt && mouse_active ) {
		Android_GrabMouseCursor(false);
		mouse_active = false;
	}
}

void Posix_PollInput()
{
	Android_PollInput();
}

void Sys_ShutdownInput(void)
{
}

unsigned char Sys_MapCharForKey(int _key)
{
	return (unsigned char)_key;
}

void Sys_ForceResolution(void)
{
	cvarSystem->SetCVarBool("r_fullscreen",  true);
	cvarSystem->SetCVarInteger("r_mode", -1);

	cvarSystem->SetCVarInteger("r_customWidth", screen_width);
	cvarSystem->SetCVarInteger("r_customHeight", screen_height);

	int autoAspectRatio = cvarSystem->GetCVarInteger("harm_r_autoAspectRatio");
	if(autoAspectRatio > 0)
	{
		float r = (float) screen_width / (float) screen_height;

		if(autoAspectRatio == 2)
		{
			idStr aspectRatio;
			if (r > 1.7f) {
				cvarSystem->SetCVarInteger("r_aspectRatio", 1);    // 16:9
				aspectRatio = "16:9";
			} else if (r > 1.55f) {
				cvarSystem->SetCVarInteger("r_aspectRatio", 2);    // 16:10
				aspectRatio = "16:10";
			} else {
				cvarSystem->SetCVarInteger("r_aspectRatio", 0);    // 4:3
				aspectRatio = "4:3";
			}

			Sys_Printf("r_mode(%i), r_customWidth(%i), r_customHeight(%i), r_aspectRatio(%i) = %s\n",
					   -1, screen_width, screen_height, cvarSystem->GetCVarInteger("r_aspectRatio"), aspectRatio.c_str());
		}
		else
		{
			cvarSystem->SetCVarInteger("r_aspectRatio", -1);
			Sys_Printf("r_mode(%i), r_customWidth(%i), r_customHeight(%i), r_aspectRatio(%i) = %f\n",
					   -1, screen_width, screen_height, cvarSystem->GetCVarInteger("r_aspectRatio"), r);
		}
	}
}

void Sys_Analog(int &side, int &forward, const int &KEY_MOVESPEED)
{
	if (analogenabled)
	{
		side = (int)(KEY_MOVESPEED * analogx);
		forward = (int)(KEY_MOVESPEED * analogy);
	}
}
