/*
HUD Editor module

Copyright (C) 2007 Cokeman

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __HUD_EDITOR_H__
#define __HUD_EDITOR_H__

// hud editor drawing function
void HUD_Editor_Draw(void);

// hud editor initialization
void HUD_Editor_Init(void);

// Mouse processing.
qbool HUD_Editor_MouseEvent (float x, float y);

// Key press processing function.
void HUD_Editor_Key(int key, int unichar, qbool down);

//
// Should this HUD element be fully drawn or not when in align mode
// when using the HUD editor.
//
qbool HUD_Editor_ConfirmDraw(hud_t *hud);

typedef enum
{
	hud_editmode_off,
	hud_editmode_align,
	hud_editmode_place,
	hud_editmode_move_resize,
	hud_editmode_resize,
	hud_editmode_move_lockedaxis,
	hud_editmode_hudmenu,
	hud_editmode_menu,
	hud_editmode_hoverlist,
	hud_editmode_normal
} hud_editor_mode_t;

extern hud_editor_mode_t	hud_editor_mode;
extern hud_t				*selected_hud;

#endif // __HUD_EDITOR_H__
