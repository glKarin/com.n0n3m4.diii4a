/*
Copyright (C) 2011 ezQuake team

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
/**
	HUD Editor module

	Initial concept code jogihoogi, rewritten by Cokeman, Feb 2007
*/

#include "ezquakeisms.h"

//#include "qsound.h"
//#include "utils.h"
//#include "menu.h"
//#include "keys.h"
//#include "Ctrl.h"
//#include "ez_controls.h"
//#include "ez_button.h"
//#include "ez_label.h"
//#include "ez_scrollbar.h"
//#include "ez_scrollpane.h"
//#include "ez_slider.h"
//#include "ez_listview.h"
//#include "ez_window.h"
#include "hud.h"
#include "hud_editor.h"

extern unsigned int keydown[];
#define isShiftDown() (keydown[K_LSHIFT]||keydown[K_RSHIFT])
#define isCtrlDown() (keydown[K_LCTRL]||keydown[K_RCTRL])
#define isAltDown() (keydown[K_LALT]||keydown[K_RALT])

#ifdef HAXX
ez_tree_t help_control_tree;
#endif

extern mpic_t		*scr_cursor_icon;	// cl_screen.c
extern cvar_t		*hud_planmode;		// hud_common.c

mpic_t				*hud_editor_move_icon	= NULL;
mpic_t				*hud_editor_resize_icon	= NULL;
mpic_t				*hud_editor_align_icon	= NULL;
mpic_t				*hud_editor_place_icon	= NULL;

cvar_t				*hud_editor_allowresize;//	= {"hud_editor_allowresize", "1"};	// Show resize handles / allow resizing.
cvar_t				*hud_editor_allowmove;//	= {"hud_editor_allowmove", "1"};	// Allow moving...
cvar_t				*hud_editor_allowplace;//	= {"hud_editor_allowplace", "1"};	// Allow placing HUDs.
cvar_t				*hud_editor_allowalign;//	= {"hud_editor_allowalign", "1"};	// Allow aligning HUDs.

extern hud_t		*hud_huds;										// The list of HUDs..
hud_t				*selected_hud			= NULL;					// The currently selected HUD.

qbool				hud_editor				= false;				// If we're in HUD editor mode or not.
hud_editor_mode_t	hud_editor_mode			= hud_editmode_off;		// The current mode the HUD editor is in.
hud_editor_mode_t	hud_editor_prevmode		= hud_editmode_off;		// The previous mode the HUD editor was in.

qbool				hud_editor_showhelp		= false;				// Show the help plaque or not?
qbool				hud_editor_showoutlines	= true;					// Show guidelines around the HUD elements.

float				hud_mouse_x;									// The screen coordinates of the mouse cursor.
float				hud_mouse_y;

// Macros for what mouse buttons are clicked.
#define MOUSEDOWN			( keydown[K_MOUSE1] ||  keydown[K_MOUSE2]|| keydown[K_MOUSE3])
#define MOUSEDOWN_1_2		( keydown[K_MOUSE1] &&  keydown[K_MOUSE2])
#define MOUSEDOWN_1_3		( keydown[K_MOUSE1] &&  keydown[K_MOUSE3])
#define MOUSEDOWN_1_2_3		( keydown[K_MOUSE1] &&  keydown[K_MOUSE2] &&  keydown[K_MOUSE3])
#define MOUSEDOWN_1_ONLY	( keydown[K_MOUSE1] && !keydown[K_MOUSE2] && !keydown[K_MOUSE3])
#define MOUSEDOWN_2_ONLY	(!keydown[K_MOUSE1] &&  keydown[K_MOUSE2] && !keydown[K_MOUSE3])
#define MOUSEDOWN_3_ONLY	(!keydown[K_MOUSE1] && !keydown[K_MOUSE2] &&  keydown[K_MOUSE3])
#define MOUSEDOWN_1_2_ONLY	( keydown[K_MOUSE1] &&  keydown[K_MOUSE2] && !keydown[K_MOUSE3])
#define MOUSEDOWN_1_3_ONLY	( keydown[K_MOUSE1] && !keydown[K_MOUSE2] &&  keydown[K_MOUSE3])
#define MOUSEDOWN_2_3_ONLY	(!keydown[K_MOUSE1] &&  keydown[K_MOUSE2] &&  keydown[K_MOUSE3])
#define MOUSEDOWN_NONE		(!keydown[K_MOUSE1] && !keydown[K_MOUSE2] && !keydown[K_MOUSE3])

#define HUD_CENTER_X(h)		((h)->lx + (h)->lw / 2)	// Gets the coordinates for the center point of the specified hud.
#define HUD_CENTER_Y(h)		((h)->ly + (h)->lh / 2)

//
// HUD align polygons.
//
#define HUD_ALIGN_POLYCOUNT_CORNER	5
#define HUD_ALIGN_POLYCOUNT_EDGE	4
#define HUD_ALIGN_POLYCOUNT_CENTER	8
#define HUD_ALIGN_POLYCOUNT_CONSOLE	4

vec3_t *hud_align_current_poly = NULL;	// The current alignment polygon when in alignment mode.
int hud_align_current_polycount = 0;	// Number of vertices in teh polygon.

vec3_t hud_align_topright_poly[HUD_ALIGN_POLYCOUNT_CORNER];
vec3_t hud_align_top_poly[HUD_ALIGN_POLYCOUNT_EDGE];
vec3_t hud_align_topleft_poly[HUD_ALIGN_POLYCOUNT_CORNER];
vec3_t hud_align_left_poly[HUD_ALIGN_POLYCOUNT_EDGE];
vec3_t hud_align_bottomleft_poly[HUD_ALIGN_POLYCOUNT_CORNER];
vec3_t hud_align_bottom_poly[HUD_ALIGN_POLYCOUNT_EDGE];
vec3_t hud_align_bottomright_poly[HUD_ALIGN_POLYCOUNT_CORNER];
vec3_t hud_align_right_poly[HUD_ALIGN_POLYCOUNT_EDGE];
vec3_t hud_align_center_poly[HUD_ALIGN_POLYCOUNT_CENTER];

vec3_t hud_align_consoleleft_poly[HUD_ALIGN_POLYCOUNT_CONSOLE];
vec3_t hud_align_console_poly[HUD_ALIGN_POLYCOUNT_CONSOLE];
vec3_t hud_align_consoleright_poly[HUD_ALIGN_POLYCOUNT_CONSOLE];

typedef enum hud_alignmode_s
{
	hud_align_center,
	hud_align_top,
	hud_align_topleft,
	hud_align_left,
	hud_align_bottomleft,
	hud_align_bottom,
	hud_align_bottomright,
	hud_align_right,
	hud_align_topright,
	hud_align_consoleleft,
	hud_align_consoleright,
	hud_align_console
	// Not including before/after for now.
} hud_alignmode_t;

hud_alignmode_t	hud_alignmode = hud_align_center;

// Possible positions for a grep handle.
typedef enum hud_greppos_s
{
	pos_visible,	// On screen.
	pos_top,
	pos_left,
	pos_bottom,
	pos_right
} hud_greppos_t;

// A Grep handle for when a HUD goes offscreen.
typedef struct hud_grephandle_s
{
	hud_t					*hud;			// HUD associated with this grep handle.
	int						x;				// The position in screen coordinates for the grephandle.
	int						y;
	int						width;
	int						height;
	qbool					highlighted;	// Should this grephandle be drawn highlighted?
	hud_greppos_t			pos;			// The offscreen position of the HUD associated with this handle.
	struct hud_grephandle_s	*next;
	struct hud_grephandle_s	*previous;
} hud_grephandle_t;

hud_grephandle_t	*hud_greps = NULL;					// The list of "grep handles" that are shown if a HUD element is moved offscreen.

hud_grephandle_t	*hud_hoverlist = NULL;				// The list of HUDs the mouse is hovering.
int					hud_hoverlist_count = 0;			// The number of hovered HUDs...
qbool				hud_hoverlist_pos_is_set = false;	// Has the coordinates been set for the hoverlist yet?
float				hud_hoverlist_x;					// The x position of the hover list.
float				hud_hoverlist_y;					// The y position of the hover list.
hud_grephandle_t	hud_containers[MAX_HUD_ELEMENTS];	// List of HUDs which have been hovered.

//
// Adds a new HUD to the list of HUDs that are being hovered.
//
static void HUD_Editor_AddHoverHud(hud_grephandle_t *hud_container)
{
	// Nothing to add.
	if(!hud_container)
	{
		return;
	}

	hud_container->next = hud_hoverlist;
	if(hud_hoverlist)
	{
		hud_hoverlist->previous = hud_container;
	}

	hud_hoverlist = hud_container;

	hud_hoverlist_count++;
}

//
// Associates ("Creates") a HUD with a HUD container.
//
static hud_grephandle_t *HUD_Editor_CreateHoverHud(hud_t *hud)
{
	static int i = 0;
	int j = 0;

	if(i >= MAX_HUD_ELEMENTS - 1)
	{
		return NULL;
	}

	// Check if the HUD already exists and use that one if that's the case...
	for(j = 0; j < MAX_HUD_ELEMENTS && hud_containers[j].hud; j++)
	{
		if(hud_containers[j].hud == hud)
		{
			hud_containers[j].next = NULL;
			hud_containers[j].previous = NULL;
			return &hud_containers[j];
		}
	}

	// The HUD wasn't found so add it to the end of the list.
	hud_containers[i].hud = hud;
	hud_containers[i].next = NULL;
	hud_containers[i].previous = NULL;
	i++;

	return &hud_containers[i - 1];
}

//
// Find the HUD that is being selected in the hover list.
//
static hud_t *HUD_Editor_FindHoverListSelection(hud_grephandle_t *list)
{
	hud_t *match = NULL;
	hud_grephandle_t *hud_iter = list;

	while(hud_iter)
	{
		if(hud_iter->hud
			&& hud_mouse_x > hud_iter->x
			&& hud_mouse_x < (hud_iter->x + hud_iter->width)
			&& hud_mouse_y > hud_iter->y
			&& hud_mouse_y < (hud_iter->y + hud_iter->height))
		{
			hud_iter->highlighted = true;
			match = hud_iter->hud;
		}
		else
		{
			hud_iter->highlighted = false;
		}

		hud_iter = hud_iter->next;
	}

	return match;
}

//
// Draw the hover list...
//
static qbool HUD_Editor_DrawHoverList(int x, int y, hud_grephandle_t *list)
{
	#define PADDING 5
	#define LINEGAP	1
	int width = 0;
	int height = 0;
	int i = 0;
	clrinfo_t color, highlight;
	hud_grephandle_t *hud_iter = list;

	// No point in showing a list if there's only one hud under the cursor
	// or if the user hasn't right-clicked.
	if (hud_hoverlist_count <= 1 || hud_editor_mode != hud_editmode_hoverlist)
	{
		return false;
	}

	// Get the width of to draw with.
	while (hud_iter)
	{
		width = max(width, (PADDING + 2 + strlen(hud_iter->hud->name)) * 8);
		hud_iter = hud_iter->next;
	}

	// Draw the background fill.
	height = ((hud_hoverlist_count - 1) * (8 + LINEGAP)) + (2 * PADDING);
	Draw_AlphaFillRGB(x, y - height, width, height, 0, 0, 0, 255);

	hud_iter = list;

	// Highlight color (yellow)...
	Vector4Set(highlight.c, 0, 255, 0, 255);

	// Orange.
	Vector4Set(color.c, 255, 150, 0, 255);

	// Draw the list items.
	while(hud_iter)
	{
		// Calculate the size and position of the HUD in the list.
		hud_iter->x = x;
		hud_iter->y = y - height + (i * 8) + LINEGAP;
		hud_iter->width = width;
		hud_iter->height = 8;

		// Draw the z-order + name of the HUD.
		Draw_ColoredString3(hud_iter->x, hud_iter->y,
			va("%2d - %s", hud_iter->hud->order->ival, hud_iter->hud->name),
			(hud_iter->highlighted ? &highlight : &color), 1, 0);

		hud_iter = hud_iter->next;
		i++;
	}

	return true;
}

//
// Sets a new HUD Editor mode (and saves the previous one).
//
static void HUD_Editor_SetMode(hud_editor_mode_t newmode)
{
	hud_editor_prevmode = hud_editor_mode;
	hud_editor_mode = newmode;
}

//
// Draws a tool tip with a background next to the cursor.
//
static void HUD_Editor_DrawTooltip(int x, int y, char *string, float r, float g, float b, float a)
{
	int len = strlen(string) * 8;

	y -= 9;

	// Make sure we're drawing within the screen.
	if (x + len > vid.width)
	{
		x -= len;
	}

	if (y + 9 > vid.height)
	{
		y -= 9;
	}

	Draw_AlphaFillRGB(x, y, len, 8, r, g, b, a);
	Draw_String(x, y, string);
}

//
// Gets an alignment string for a specified alignmode enum.
//
static char *HUD_Editor_GetAlignmentString(hud_alignmode_t align)
{
	switch(hud_alignmode)
	{
		case hud_align_center :		return "center center";
		case hud_align_top :		return "center top";
		case hud_align_topleft :	return "left top";
		case hud_align_left :		return "left center";
		case hud_align_bottomleft :	return "left bottom";
		case hud_align_bottom :		return "center bottom";
		case hud_align_bottomright :return "right bottom";
		case hud_align_right :		return "right center";
		case hud_align_topright :	return "right top";
		case hud_align_consoleleft :return "left console";
		case hud_align_console :	return "center console";
		case hud_align_consoleright:return "right console";
		default :					return "";
	}
}

//
// Returns an alignment enum based on a specified alignment string.
//
static hud_alignmode_t HUD_Editor_GetAlignmentFromString(char *alignstr)
{
	if(!strcmp(alignstr, "center center"))
	{
		return hud_align_center;
	}
	else if(!strcmp(alignstr, "center top"))
	{
		return hud_align_top;
	}
	else if(!strcmp(alignstr, "left top"))
	{
		return hud_align_topleft;
	}
	else if(!strcmp(alignstr, "left center"))
	{
		return hud_align_left;
	}
	else if(!strcmp(alignstr, "left bottom"))
	{
		return hud_align_bottomleft;
	}
	else if(!strcmp(alignstr, "center bottom"))
	{
		return hud_align_bottom;
	}
	else if(!strcmp(alignstr, "right bottom"))
	{
		return hud_align_bottomright;
	}
	else if(!strcmp(alignstr, "right center"))
	{
		return hud_align_right;
	}
	else if(!strcmp(alignstr, "right top"))
	{
		return hud_align_topright;
	}
	else if(!strcmp(alignstr, "left console"))
	{
		return hud_align_consoleleft;
	}
	else if(!strcmp(alignstr, "center console"))
	{
		return hud_align_console;
	}
	else if(!strcmp(alignstr, "right console"))
	{
		return hud_align_consoleright;
	}

	return hud_align_center;
}

qbool IsPointInPolygon(int points, vec3_t planes[], float x, float y)
{
	return false;
}
//
// Returns the alignment we are trying to align the selected HUD to at it's placement
// when in alignment mode.
//
static hud_alignmode_t HUD_Editor_GetAlignment(int x, int y, hud_t *hud_element)
{
	// For less clutter.
	float	mid_x		= 0.0;
	float	mid_y		= 0.0;
	int		offset_x	= 0;
	int		offset_y	= 0;

	if(hud_element)
	{
		// Aligns for HUD elements.
		offset_x = hud_element->lx;
		offset_y = hud_element->ly;
		mid_x = hud_element->lw / 2.0;
		mid_y = hud_element->lh / 2.0;
	}
	else
	{
		// Aligns for Screen.
		offset_x = 0;
		offset_y = (int)(vid.height * 0.05);
		mid_x = vid.width / 2.0;
		mid_y = (vid.height + offset_y) / 2.0;
	}

	if(!hud_element)
	{
		// Console left.
		{
			hud_align_consoleleft_poly[0][0] = 0;
			hud_align_consoleleft_poly[0][1] = 0;
			hud_align_consoleleft_poly[0][2] = 0;

			hud_align_consoleleft_poly[1][0] = 0;
			hud_align_consoleleft_poly[1][1] = offset_y;
			hud_align_consoleleft_poly[1][2] = 0;

			hud_align_consoleleft_poly[2][0] = mid_x / 2.0;
			hud_align_consoleleft_poly[2][1] = offset_y;
			hud_align_consoleleft_poly[2][2] = 0;

			hud_align_consoleleft_poly[3][0] = mid_x / 2.0;
			hud_align_consoleleft_poly[3][1] = 0;
			hud_align_consoleleft_poly[3][2] = 0;

			if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_CONSOLE, hud_align_consoleleft_poly, x, y))
			{
				hud_align_current_poly = hud_align_consoleleft_poly;
				hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CONSOLE;
				return hud_align_consoleleft;
			}
		}

		// Console.
		{
			hud_align_console_poly[0][0] = mid_x / 2.0;
			hud_align_console_poly[0][1] = 0;
			hud_align_console_poly[0][2] = 0;

			hud_align_console_poly[1][0] = mid_x / 2.0;
			hud_align_console_poly[1][1] = offset_y;
			hud_align_console_poly[1][2] = 0;

			hud_align_console_poly[2][0] = mid_x + (mid_x / 2.0);
			hud_align_console_poly[2][1] = offset_y;
			hud_align_console_poly[2][2] = 0;

			hud_align_console_poly[3][0] = mid_x + (mid_x / 2.0);
			hud_align_console_poly[3][1] = 0;
			hud_align_console_poly[3][2] = 0;

			if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_CONSOLE, hud_align_console_poly, x, y))
			{
				hud_align_current_poly = hud_align_console_poly;
				hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CONSOLE;
				return hud_align_console;
			}
		}

		// Console right.
		{
			hud_align_consoleright_poly[0][0] = mid_x + (mid_x / 2.0);
			hud_align_consoleright_poly[0][1] = 0;
			hud_align_consoleright_poly[0][2] = 0;

			hud_align_consoleright_poly[1][0] = mid_x + (mid_x / 2.0);
			hud_align_consoleright_poly[1][1] = offset_y;
			hud_align_consoleright_poly[1][2] = 0;

			hud_align_consoleright_poly[2][0] = 2 * mid_x;
			hud_align_consoleright_poly[2][1] = offset_y;
			hud_align_consoleright_poly[2][2] = 0;

			hud_align_consoleright_poly[3][0] = 2 * mid_x;
			hud_align_consoleright_poly[3][1] = 0;
			hud_align_consoleright_poly[3][2] = 0;

			if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_CONSOLE, hud_align_consoleright_poly, x, y))
			{
				hud_align_current_poly = hud_align_consoleright_poly;
				hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CONSOLE;
				return hud_align_consoleright;
			}
		}
	}

	// Top right.
	{
		hud_align_topright_poly[0][0] = mid_x + (mid_x / 2.0);
		hud_align_topright_poly[0][1] = 0;
		hud_align_topright_poly[0][2] = 0;

		hud_align_topright_poly[1][0] = mid_x + (mid_x / 4.0);
		hud_align_topright_poly[1][1] = mid_y - (mid_y / 2.0);
		hud_align_topright_poly[1][2] = 0;

		hud_align_topright_poly[2][0] = mid_x + (mid_x / 2.0);
		hud_align_topright_poly[2][1] = mid_y - (mid_y / 4.0);
		hud_align_topright_poly[2][2] = 0;

		hud_align_topright_poly[3][0] = 2 * mid_x;
		hud_align_topright_poly[3][1] = mid_y - (mid_y / 2.0);
		hud_align_topright_poly[3][2] = 0;

		hud_align_topright_poly[4][0] = 2 * mid_x;
		hud_align_topright_poly[4][1] = 0;
		hud_align_topright_poly[4][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_CORNER, hud_align_topright_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_topright_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CORNER;
			return hud_align_topright;
		}
	}

	// Top.
	{
		hud_align_top_poly[0][0] = mid_x / 2.0;
		hud_align_top_poly[0][1] = 0;
		hud_align_top_poly[0][2] = 0;

		hud_align_top_poly[1][0] = mid_x - (mid_x / 4.0);
		hud_align_top_poly[1][1] = mid_y - (mid_y / 2.0);
		hud_align_top_poly[1][2] = 0;

		hud_align_top_poly[2][0] = mid_x + (mid_x / 4.0);
		hud_align_top_poly[2][1] = mid_y - (mid_y / 2.0);
		hud_align_top_poly[2][2] = 0;

		hud_align_top_poly[3][0] = mid_x + (mid_x / 2.0);
		hud_align_top_poly[3][1] = 0;
		hud_align_top_poly[3][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_EDGE, hud_align_top_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_top_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_EDGE;
			return hud_align_top;
		}
	}

	// Top Left.
	{
		hud_align_topleft_poly[0][0] = 0;
		hud_align_topleft_poly[0][1] = 0;
		hud_align_topleft_poly[0][2] = 0;

		hud_align_topleft_poly[1][0] = 0;
		hud_align_topleft_poly[1][1] = mid_y - (mid_y / 2.0);
		hud_align_topleft_poly[1][2] = 0;

		hud_align_topleft_poly[2][0] = mid_x - (mid_x / 2.0);
		hud_align_topleft_poly[2][1] = mid_y - (mid_y / 4.0);
		hud_align_topleft_poly[2][2] = 0;

		hud_align_topleft_poly[3][0] = mid_x - (mid_x / 4.0);
		hud_align_topleft_poly[3][1] = mid_y - (mid_y / 2.0);
		hud_align_topleft_poly[3][2] = 0;

		hud_align_topleft_poly[4][0] = mid_x - (mid_x / 2.0);
		hud_align_topleft_poly[4][1] = 0;
		hud_align_topleft_poly[4][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_CORNER, hud_align_topleft_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_topleft_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CORNER;
			return hud_align_topleft;
		}
	}

	// Left.
	{
		hud_align_left_poly[0][0] = 0;
		hud_align_left_poly[0][1] = mid_y / 2.0;
		hud_align_left_poly[0][2] = 0;

		hud_align_left_poly[1][0] = 0;
		hud_align_left_poly[1][1] = mid_y + (mid_y / 2.0);
		hud_align_left_poly[1][2] = 0;

		hud_align_left_poly[2][0] = mid_x / 2.0;
		hud_align_left_poly[2][1] = mid_y + (mid_y / 4.0);
		hud_align_left_poly[2][2] = 0;

		hud_align_left_poly[3][0] = mid_x / 2.0;
		hud_align_left_poly[3][1] = mid_y - (mid_y / 4.0);
		hud_align_left_poly[3][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_EDGE, hud_align_left_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_left_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_EDGE;
			return hud_align_left;
		}
	}

	// Bottom Left.
	{
		hud_align_bottomleft_poly[0][0] = 0;
		hud_align_bottomleft_poly[0][1] = mid_y + (mid_y / 2.0);
		hud_align_bottomleft_poly[0][2] = 0;

		hud_align_bottomleft_poly[1][0] = 0;
		hud_align_bottomleft_poly[1][1] = 2 * mid_y;
		hud_align_bottomleft_poly[1][2] = 0;

		hud_align_bottomleft_poly[2][0] = mid_x / 2.0;
		hud_align_bottomleft_poly[2][1] = 2 * mid_y;
		hud_align_bottomleft_poly[2][2] = 0;

		hud_align_bottomleft_poly[3][0] = mid_x - (mid_x / 4.0);
		hud_align_bottomleft_poly[3][1] = mid_y + (mid_y / 2.0);
		hud_align_bottomleft_poly[3][2] = 0;

		hud_align_bottomleft_poly[4][0] = mid_x / 2.0;
		hud_align_bottomleft_poly[4][1] = mid_y + (mid_y / 4.0);
		hud_align_bottomleft_poly[4][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_CORNER, hud_align_bottomleft_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_bottomleft_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CORNER;
			return hud_align_bottomleft;
		}
	}

	// Bottom.
	{
		hud_align_bottom_poly[0][0] = mid_x - (mid_x / 2.0);
		hud_align_bottom_poly[0][1] = 2 * mid_y;
		hud_align_bottom_poly[0][2] = 0;

		hud_align_bottom_poly[1][0] = mid_x + (mid_x / 2.0);
		hud_align_bottom_poly[1][1] = 2 * mid_y;
		hud_align_bottom_poly[1][2] = 0;

		hud_align_bottom_poly[2][0] = mid_x + (mid_x / 4.0);
		hud_align_bottom_poly[2][1] = mid_y + (mid_y / 2.0);
		hud_align_bottom_poly[2][2] = 0;

		hud_align_bottom_poly[3][0] = mid_x - (mid_x / 4.0);
		hud_align_bottom_poly[3][1] = mid_y + (mid_y / 2.0);
		hud_align_bottom_poly[3][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_EDGE, hud_align_bottom_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_bottom_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_EDGE;
			return hud_align_bottom;
		}
	}

	// Bottom Right.
	{
		hud_align_bottomright_poly[0][0] = mid_x + (mid_x / 2.0);
		hud_align_bottomright_poly[0][1] = 2 * mid_y;
		hud_align_bottomright_poly[0][2] = 0;

		hud_align_bottomright_poly[1][0] = 2 * mid_x;
		hud_align_bottomright_poly[1][1] = 2 * mid_y;
		hud_align_bottomright_poly[1][2] = 0;

		hud_align_bottomright_poly[2][0] = 2 * mid_x;
		hud_align_bottomright_poly[2][1] = mid_y + (mid_y / 2.0);
		hud_align_bottomright_poly[2][2] = 0;

		hud_align_bottomright_poly[3][0] = mid_x + (mid_x / 2.0);
		hud_align_bottomright_poly[3][1] = mid_y + (mid_y / 4.0);
		hud_align_bottomright_poly[3][2] = 0;

		hud_align_bottomright_poly[4][0] = mid_x + (mid_x / 4.0);
		hud_align_bottomright_poly[4][1] = mid_y + (mid_y / 2.0);
		hud_align_bottomright_poly[4][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_CORNER, hud_align_bottomright_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_bottomright_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CORNER;
			return hud_align_bottomright;
		}
	}

	// Right.
	{
		hud_align_right_poly[0][0] = 2 * mid_x;
		hud_align_right_poly[0][1] = mid_y + (mid_y / 2.0);
		hud_align_right_poly[0][2] = 0;

		hud_align_right_poly[1][0] = 2 * mid_x;
		hud_align_right_poly[1][1] = mid_y / 2.0;
		hud_align_right_poly[1][2] = 0;

		hud_align_right_poly[2][0] = mid_x + (mid_x / 2.0);
		hud_align_right_poly[2][1] = mid_y - (mid_y / 4.0);
		hud_align_right_poly[2][2] = 0;

		hud_align_right_poly[3][0] = mid_x + (mid_x / 2.0);
		hud_align_right_poly[3][1] = mid_y + (mid_y / 4.0);
		hud_align_right_poly[3][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_EDGE, hud_align_right_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_right_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_EDGE;
			return hud_align_right;
		}
	}

	// Center.
	{
		hud_align_center_poly[0][0] = mid_x - (mid_x / 4.0);
		hud_align_center_poly[0][1] = mid_y / 2.0;
		hud_align_center_poly[0][2] = 0;

		hud_align_center_poly[1][0] = mid_x / 2.0;
		hud_align_center_poly[1][1] = mid_y - (mid_y / 4.0);
		hud_align_center_poly[1][2] = 0;

		hud_align_center_poly[2][0] = mid_x / 2.0;
		hud_align_center_poly[2][1] = mid_y + (mid_y / 4.0);
		hud_align_center_poly[2][2] = 0;

		hud_align_center_poly[3][0] = mid_x - (mid_x / 4.0);
		hud_align_center_poly[3][1] = mid_y + (mid_y / 2.0);
		hud_align_center_poly[3][2] = 0;

		hud_align_center_poly[4][0] = mid_x + (mid_x / 4.0);
		hud_align_center_poly[4][1] = mid_y + (mid_y / 2.0);
		hud_align_center_poly[4][2] = 0;

		hud_align_center_poly[5][0] = mid_x + (mid_x / 2.0);
		hud_align_center_poly[5][1] = mid_y + (mid_y / 4.0);
		hud_align_center_poly[5][2] = 0;

		hud_align_center_poly[6][0] = mid_x + (mid_x / 2.0);
		hud_align_center_poly[6][1] = mid_y - (mid_y / 4.0);
		hud_align_center_poly[6][2] = 0;

		hud_align_center_poly[7][0] = mid_x + (mid_x / 4.0);
		hud_align_center_poly[7][1] = mid_y / 2.0;
		hud_align_center_poly[7][2] = 0;

		if (IsPointInPolygon(HUD_ALIGN_POLYCOUNT_CENTER, hud_align_center_poly, x - offset_x, y - offset_y))
		{
			hud_align_current_poly = hud_align_center_poly;
			hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CENTER;
			return hud_align_center;
		}
	}

	// Default to center.
	hud_align_current_poly = hud_align_center_poly;
	hud_align_current_polycount = HUD_ALIGN_POLYCOUNT_CENTER;
	return hud_align_center;
}

//
// Finds the next child of the specified HUD element.
//
static hud_t *HUD_Editor_FindNextChild(hud_t *hud_element)
{
	static hud_t	*hud_it = NULL;
	static hud_t	*parent = NULL;
	hud_t			*hud_result = NULL;

	// Reset everything if we're given a NULL argument.
	if(!hud_element)
	{
		hud_it = NULL;
		parent = NULL;
		return NULL;
	}

	// There's a new parent so start searching from the beginning.
	if(!parent || strcmp(parent->name, hud_element->name))
	{
		parent = hud_element;
		hud_it = hud_huds;
	}

	while(hud_it)
	{
		// Check if this HUD is placed at the parent, if so we've found a child.
		if(hud_it->place_hud && !strcmp(hud_it->place_hud->name, parent->name))
		{
			hud_result = hud_it;
			hud_it = hud_it->next;
			return hud_result;
		}

		hud_it = hud_it->next;
	}

	parent = NULL;

	// No children found.
	return NULL;
}

//
// Moves a HUD element.
//
static void HUD_Editor_Move(float dx, float dy, hud_t *hud_element)
{
	if(!hud_element)
	{
		return;
	}

	Cvar_SetValue(hud_element->pos_x, hud_element->pos_x->value + dx);
	Cvar_SetValue(hud_element->pos_y, hud_element->pos_y->value + dy);
}

//
// Draw the current alignment.
//
static void HUD_Editor_DrawAlignment(hud_t *hud_parent)
{
	if(hud_parent)
	{
		//Draw_Polygon(hud_parent->lx, hud_parent->ly, hud_align_current_poly, hud_align_current_polycount, true, RGBA_TO_COLOR(255, 255, 0, 50));
		Draw_Polygon(hud_parent->lx, hud_parent->ly, hud_align_current_poly, hud_align_current_polycount, true, 255, 255, 0, 50);
	}
	else
	{
		//Draw_Polygon(0, 0, hud_align_current_poly, hud_align_current_polycount, true, RGBA_TO_COLOR(255, 255, 0, 50));
		Draw_Polygon(0, 0, hud_align_current_poly, hud_align_current_polycount, true, 255, 255, 0, 50);
	}
}

typedef struct hud_resize_handle_s
{
	int x;
	int y;
	int width;
	int height;
} hud_resize_handle_t;

#define HUD_RESIZEHANDLE_THICKNESS		5
#define HUD_RESIZEHANDLE_SIZEFACTOR		0.3
#define HUD_RESIZEHANDLE_COUNT			8
#define HUD_RESIZE_MAGICSCALE			200 // HACK : This seems to work nice though...

#define HUD_RESIZEHANDLE_NONE			-1
#define HUD_RESIZEHANDLE_TOPLEFT		0
#define HUD_RESIZEHANDLE_TOP			1
#define HUD_RESIZEHANDLE_TOPRIGHT		2
#define HUD_RESIZEHANDLE_RIGHT			3
#define HUD_RESIZEHANDLE_BOTTOMRIGHT	4
#define HUD_RESIZEHANDLE_BOTTOM			5
#define HUD_RESIZEHANDLE_BOTTOMLEFT		6
#define HUD_RESIZEHANDLE_LEFT			7

//
// Resizes the height/width of a HUD element by a delta size and alignment.
//
static void HUD_Editor_ResizeDelta(cvar_t *size, float delta_size, hud_alignmode_t alignment)
{
	if(!size)
	{
		return;
	}

	switch(alignment)
	{
		case hud_align_right :
		case hud_align_bottom :
			Cvar_SetValue(size, size->value + delta_size);
			break;
		case hud_align_left :
		case hud_align_top :
			Cvar_SetValue(size, size->value - delta_size);
			break;
		// unhandled
		case hud_align_center:
		case hud_align_topleft:
		case hud_align_bottomleft:
		case hud_align_bottomright:
		case hud_align_topright:
		case hud_align_consoleleft:
		case hud_align_consoleright:
		case hud_align_console:
			break;
	}
}

//
// Scales a HUD element.
//
static void HUD_EditorScaleDelta(cvar_t *scale, float delta_scale, hud_alignmode_t alignment)
{
	if(!scale)
	{
		return;
	}

	switch(alignment)
	{
		case hud_align_topleft :
		case hud_align_bottomleft :
			Cvar_SetValue(scale, scale->value - delta_scale);
			break;
		case hud_align_topright :
		case hud_align_bottomright :
			Cvar_SetValue(scale, scale->value + delta_scale);
			break;
		// unhandled
		case hud_align_center:
		case hud_align_top:
		case hud_align_left:
		case hud_align_bottom:
		case hud_align_right:
		case hud_align_consoleleft:
		case hud_align_consoleright:
		case hud_align_console:
			break;
	}

	if(scale->value < 0)
	{
		Cvar_SetValue(scale, 0);
	}
}

//
// Checks if the HUD element we're hovering has any dimension variables
// and handles resizing for it if it does by drawing resize handles
// that the user can click.
//
static qbool HUD_Editor_Resizing(hud_t *hud_hover)
{
	extern float mouse_x, mouse_y;		// in_win.c, delta mouse.
	int i					= 0;
	qbool found_handle		= false;	// Did we find any handle under the cursor?
	static cvar_t *width	= NULL;
	static cvar_t *height	= NULL;
	static cvar_t *scale	= NULL;
	hud_resize_handle_t *resize_handles[HUD_RESIZEHANDLE_COUNT];
	static int last_resize_handle = HUD_RESIZEHANDLE_NONE;

	// Is this mode turned on?
	if(!hud_editor_allowresize->value)
	{
		return false;
	}

	// Select a HUD if it hasn't already been done.
	if(hud_hover && hud_editor_mode == hud_editmode_move_resize
		&& !selected_hud && hud_hover)
	{
		selected_hud = hud_hover;
		return true;
	}

	memset(resize_handles, 0, sizeof(resize_handles));

	// Try getting any available dimension variables
	// that are available for this HUD element, these
	// aren't mandatory for all HUD elements, so only
	// some will have them.
	if(hud_hover)
	{
		width	= HUD_FindVar(hud_hover, "width");
		height	= HUD_FindVar(hud_hover, "height");
		scale	= HUD_FindVar(hud_hover, "scale");

		if(width)
		{
			// Right & left.
			static hud_resize_handle_t right;
			static hud_resize_handle_t left;

			right.width		= HUD_RESIZEHANDLE_THICKNESS;
			right.height	= hud_hover->lh * HUD_RESIZEHANDLE_SIZEFACTOR;
			right.x			= hud_hover->lw - right.width;
			right.y			= (hud_hover->lh - right.height) / 2;
			resize_handles[HUD_RESIZEHANDLE_RIGHT] = &right;

			left.width		= HUD_RESIZEHANDLE_THICKNESS;
			left.height		= hud_hover->lh * HUD_RESIZEHANDLE_SIZEFACTOR;
			left.x			= 0;
			left.y			= (hud_hover->lh - left.height) / 2;
			resize_handles[HUD_RESIZEHANDLE_LEFT] = &left;
		}

		if(height)
		{
			// Top & bottom
			static hud_resize_handle_t top;
			static hud_resize_handle_t bottom;

			top.width		= hud_hover->lw * HUD_RESIZEHANDLE_SIZEFACTOR;
			top.height		= HUD_RESIZEHANDLE_THICKNESS;
			top.x			= (hud_hover->lw - top.width) / 2;
			top.y			= 0;
			resize_handles[HUD_RESIZEHANDLE_TOP] = &top;

			bottom.width		= hud_hover->lw * HUD_RESIZEHANDLE_SIZEFACTOR;
			bottom.height		= HUD_RESIZEHANDLE_THICKNESS;
			bottom.x			= (hud_hover->lw - bottom.width) / 2;
			bottom.y			= hud_hover->lh - bottom.height;
			resize_handles[HUD_RESIZEHANDLE_BOTTOM] = &bottom;
		}

		if(scale)
		{
			// Top left, top right, bottom left & bottom right.
			static hud_resize_handle_t topleft;
			static hud_resize_handle_t bottomleft;
			static hud_resize_handle_t topright;
			static hud_resize_handle_t bottomright;

			topleft.width		= HUD_RESIZEHANDLE_THICKNESS;
			topleft.height		= HUD_RESIZEHANDLE_THICKNESS;
			topleft.x			= 0;
			topleft.y			= 0;
			resize_handles[HUD_RESIZEHANDLE_TOPLEFT] = &topleft;

			bottomleft.width	= HUD_RESIZEHANDLE_THICKNESS;
			bottomleft.height	= HUD_RESIZEHANDLE_THICKNESS;
			bottomleft.x		= 0;
			bottomleft.y		= hud_hover->lh - bottomleft.height;
			resize_handles[HUD_RESIZEHANDLE_BOTTOMLEFT] = &bottomleft;

			topright.width		= HUD_RESIZEHANDLE_THICKNESS;
			topright.height		= HUD_RESIZEHANDLE_THICKNESS;
			topright.x			= hud_hover->lw - topright.width;
			topright.y			= 0;
			resize_handles[HUD_RESIZEHANDLE_TOPRIGHT] = &topright;

			bottomright.width	= HUD_RESIZEHANDLE_THICKNESS;
			bottomright.height	= HUD_RESIZEHANDLE_THICKNESS;
			bottomright.x		= hud_hover->lw - bottomright.width;
			bottomright.y		= hud_hover->lh - bottomright.height;
			resize_handles[HUD_RESIZEHANDLE_BOTTOMRIGHT] = &bottomright;
		}
	}

	for(i = 0; i < HUD_RESIZEHANDLE_COUNT; i++)
	{
		// Non existant for this HUD element.
		if(!resize_handles[i])
		{
			continue;
		}

		if(hud_editor_mode == hud_editmode_move_resize)
		{
			// We're in resize mode, so check if we're clicking any of the
			// resize handles.

			if((selected_hud && (last_resize_handle == i)) ||
				  (hud_mouse_x >= (selected_hud->lx + resize_handles[i]->x)
				&& (hud_mouse_x <= (selected_hud->lx + resize_handles[i]->x + resize_handles[i]->width))
				&& (hud_mouse_y >= (selected_hud->ly + resize_handles[i]->y))
				&& (hud_mouse_y <= (selected_hud->ly + resize_handles[i]->y + resize_handles[i]->height))))
			{
				// Keep track of which resize handle we're grabbing
				// so that it doesn't matter if the mouse "slips" outside
				// it's bounding box as long as the mouse is still pressed.
				if(last_resize_handle < 0 || last_resize_handle != i)
				{
					last_resize_handle = i;
				}

				found_handle = true;

				// Draw the resize handle highlighted.
				Draw_AlphaFillRGB(
					selected_hud->lx + resize_handles[last_resize_handle]->x,
					selected_hud->ly + resize_handles[last_resize_handle]->y,
					resize_handles[last_resize_handle]->width,
					resize_handles[last_resize_handle]->height,
					255, 255, 0, 125);

				// Check which resize handle that has been selected
				// and resize the HUD element accordingly.
				switch(i)
				{
					case HUD_RESIZEHANDLE_RIGHT :
						HUD_Editor_ResizeDelta(width, mouse_x, hud_align_right);
						break;
					case HUD_RESIZEHANDLE_LEFT :
						HUD_Editor_ResizeDelta(width, mouse_x, hud_align_left);
						break;
					case HUD_RESIZEHANDLE_TOP :
						HUD_Editor_ResizeDelta(height, mouse_y, hud_align_top);
						break;
					case HUD_RESIZEHANDLE_BOTTOM :
						HUD_Editor_ResizeDelta(height, mouse_y, hud_align_bottom);
						break;
					case HUD_RESIZEHANDLE_TOPRIGHT :
						// HACK : Dividing by 200 seems to work fine, but this could probably be done a lot nicer.
						HUD_EditorScaleDelta(scale, (mouse_x - mouse_y) / HUD_RESIZE_MAGICSCALE, hud_align_topright);
						break;
					case HUD_RESIZEHANDLE_BOTTOMRIGHT :
						HUD_EditorScaleDelta(scale, (mouse_x + mouse_y) / HUD_RESIZE_MAGICSCALE, hud_align_bottomright);
						break;
					case HUD_RESIZEHANDLE_TOPLEFT :
						HUD_EditorScaleDelta(scale, (mouse_x + mouse_y) / HUD_RESIZE_MAGICSCALE, hud_align_topleft);
						break;
					case HUD_RESIZEHANDLE_BOTTOMLEFT :
						HUD_EditorScaleDelta(scale, (mouse_x - mouse_y) / HUD_RESIZE_MAGICSCALE, hud_align_bottomleft);
						break;
				}

				// Recalculate all HUD elements.
				HUD_Recalculate();
				return true;
			}
		}
		else if(hud_editor_prevmode == hud_editmode_move_resize)
		{
			// We left resize mode.
			selected_hud = NULL;
			last_resize_handle = HUD_RESIZEHANDLE_NONE;
		}
		else if(hud_hover)
		{
			// If we're hovering a HUD, always draw all it's resize handles.
			if((hud_mouse_x >= (hud_hover->lx + resize_handles[i]->x)
				&& hud_mouse_x <= (hud_hover->lx + resize_handles[i]->x + resize_handles[i]->width)
				&& hud_mouse_y >= (hud_hover->ly + resize_handles[i]->y)
				&& hud_mouse_y <= (hud_hover->ly + resize_handles[i]->y + resize_handles[i]->height)))
			{
				// Highlight the resize handle under the cursor.
				Draw_AlphaFillRGB(hud_hover->lx + resize_handles[i]->x, hud_hover->ly + resize_handles[i]->y,
					resize_handles[i]->width, resize_handles[i]->height,
					255, 255, 0, 125);

				// Set the resize cursor icon since we're hovering a resize handle.
				scr_cursor_icon = hud_editor_resize_icon;
				found_handle = true;
			}
			else
			{
				// Normal resize handle.
				Draw_AlphaFillRGB(hud_hover->lx + resize_handles[i]->x, hud_hover->ly + resize_handles[i]->y,
				resize_handles[i]->width, resize_handles[i]->height,
				255, 255, 0, 50);
			}
		}
	}

	// We didn't hover any resize handle so show no icon.
	if(!found_handle)
	{
		scr_cursor_icon = NULL;
	}

	// We didn't perform any action, so let others try.
	return false;
}

static qbool hud_editor_locked_axis_is_x = true;	// Are we locking X- or Y-axis movement?
static qbool hud_editor_lockaxis_found= false;		// Have we found what axist to lock on to?
static qbool hud_editor_finding_lockaxis = false;	// Are we in the process of finding the lock axis?

//
// Check if we're supposed to be moving anything.
//
static qbool HUD_Editor_Moving(hud_t *hud_hover)
{
	// Mouse delta (in_win.c)
	extern float mouse_x, mouse_y;

	// Is this mode allowed?
	if(!hud_editor_allowmove->value)
	{
		return false;
	}

	// Set the move cursor icon if we're hovering a HUD element
	// unless it's not already set (resize may have set it if
	// a resize handle is being hovered).
	if(hud_hover && !scr_cursor_icon)
	{
		scr_cursor_icon = hud_editor_move_icon;
	}
	else if(!hud_hover)
	{
		// Not hovering anything so show no cursor.
		scr_cursor_icon = NULL;
	}

	// Left mousebutton down = lets move it !
	if (hud_editor_mode == hud_editmode_move_resize || hud_editor_mode == hud_editmode_move_lockedaxis)
	{
		// If we just entered movement mode, nothing is selected
		// so select the hud we're hovering to start with.
		if(!selected_hud && hud_hover)
		{
			selected_hud = hud_hover;
			return true;
		}

		if(selected_hud)
		{
			// Move using the mouse delta instead of the absolute
			// mouse cursor coordinates on the screen.

			// Lock the movement to the axis that the user starts
			// dragging the HUD element along if we're in locked-axis mode.
			if(hud_editor_mode == hud_editmode_move_lockedaxis)
			{
				// We haven't found the axis.
				if(!hud_editor_lockaxis_found)
				{
					static float last_mouse_x = 0.0;
					static float last_mouse_y = 0.0;

					if(!hud_editor_finding_lockaxis)
					{
						// Start trying to find the lock axis.
						hud_editor_finding_lockaxis = true;
						last_mouse_x = 0;
						last_mouse_y = 0;
					}
					else
					{
						// Get the delta of the mouse movement from when the
						// user clicked the mouse button and started dragging.
						float mouse_delta_x = fabs(last_mouse_x - mouse_x);
						float mouse_delta_y = fabs(last_mouse_y - mouse_y);

						// Increment the mouse movement since last frame.
						last_mouse_x += mouse_x;
						last_mouse_y += mouse_y;

						// If the mouse has moved more than 5 pixels in any
						// direction we decide to lock onto that axis.
						if(mouse_delta_x > 5 || mouse_delta_y > 5)
						{
							hud_editor_locked_axis_is_x = (mouse_delta_x > mouse_delta_y);
							hud_editor_lockaxis_found = true;
						}
					}
				}

				// Move while locked to the axis.
				if(hud_editor_locked_axis_is_x)
				{
					// Move only along X-axis.
					HUD_Editor_Move(mouse_x, 0, hud_hover);
				}
				else
				{
					// Move only along Y-axis.
					HUD_Editor_Move(0, mouse_y, hud_hover);
				}
			}
			else
			{
				// Ordinary move.
				HUD_Editor_Move(mouse_x, mouse_y, hud_hover);
			}

			HUD_Recalculate();
			return true;
		}
	}
	else if((hud_editor_prevmode == hud_editmode_move_resize && hud_editor_mode != hud_editmode_move_lockedaxis)
		 || (hud_editor_prevmode == hud_editmode_move_lockedaxis && hud_editor_mode != hud_editmode_move_resize))
	{
		// Only deselect if we're going from a move mode to a non-move mode (Such as align/place).

		// We've left move mode, so deselect.
		selected_hud = NULL;
		hud_editor_lockaxis_found = false;
		hud_editor_finding_lockaxis = false;
	}

	// We haven't handled the input.
	return false;
}

//
// Handles input from mouse if in alignment mode.
//
static qbool HUD_Editor_Aligning(hud_t *hud_hover)
{
	// Is this mode allowed?
	if(!hud_editor_allowalign->value)
	{
		return false;
	}

	if(hud_editor_mode == hud_editmode_align)
	{
		// If we just entered alignment mode, nothing is selected
		// so select the hud we're hovering to start with.
		if(!selected_hud && hud_hover)
		{
			selected_hud = hud_hover;
			return true;
		}

		// Set the align icon for the cursor.
		scr_cursor_icon = hud_editor_align_icon;

		// We have something selected so show some visual
		// feedback for when aligning to the user.
		if(selected_hud)
		{
			if(selected_hud->place_hud)
			{
				// Placed at another HUD, so align onto that.
				hud_alignmode = HUD_Editor_GetAlignment(hud_mouse_x, hud_mouse_y, selected_hud->place_hud);
				HUD_Editor_DrawAlignment(selected_hud->place_hud);
			}
			else
			{
				// Placed on the screen.
				hud_alignmode = HUD_Editor_GetAlignment(hud_mouse_x, hud_mouse_y, NULL);
				HUD_Editor_DrawAlignment(NULL);
			}
		}
	}
	else if(hud_editor_prevmode == hud_editmode_align && isAltDown())
	{
		// The user just released the mousebutton but is still holding
		// down ALT. If the user releases ALT before the mouse button
		// the operation will be cancelled. So commit the users actions.

		// We must have something to align.
		if(selected_hud)
		{
			// Reset position.
			Cvar_Set(selected_hud->pos_x, "0");
			Cvar_Set(selected_hud->pos_y, "0");

			// Align to the area the mouse is placed over (previously set in hud_alignmode).
			Cbuf_AddText(va("align %s %s\n", selected_hud->name, HUD_Editor_GetAlignmentString(hud_alignmode)));
			HUD_Recalculate();

			// Free selection.
			selected_hud = NULL;
			return true;
		}
	}

	return false;
}

//
// Handles feedback/commiting of actions when in placement mode.
//
static qbool HUD_Editor_Placing(hud_t *hud_hover)
{
	// Is this mode allowed?
	if(!hud_editor_allowplace->value)
	{
		return false;
	}

	// Show the place icon at the cursor if ctrl is pressed
	// while hovering a HUD element.
	if(hud_hover && isCtrlDown())
	{
		// Set the place cursor icon.
		scr_cursor_icon = hud_editor_place_icon;
	}
	else if(!hud_hover)
	{
		scr_cursor_icon = NULL;
	}

	if(hud_editor_mode == hud_editmode_place)
	{
		// If we just entered placement mode, nothing is selected
		// so select the hud we're hovering to start with.
		if(!selected_hud && hud_hover)
		{
			selected_hud = hud_hover;
			return true;
		}

		// Set the place cursor icon.
		scr_cursor_icon = hud_editor_place_icon;

		if(selected_hud)
		{
			// Find the center of the selected HUD so we know where
			// to draw the line from.

			if(hud_hover)
			{
				// We're trying to place the HUD on itself or on the HUD it's already placed at.
				if(hud_hover == selected_hud || (selected_hud->place_hud && selected_hud->place_hud == hud_hover))
				{
					// Red "not allowed".
					Draw_AlphaRectangleRGB(hud_hover->lx, hud_hover->ly, hud_hover->lw, hud_hover->lh, 1, true, 255, 0, 0, 25);
					Draw_AlphaRectangleRGB(hud_hover->lx, hud_hover->ly, hud_hover->lw, hud_hover->lh, 1, false, 255, 0, 0, 255);
				}
				else
				{
					// Green "allowed" placement.
					Draw_AlphaRectangleRGB(hud_hover->lx, hud_hover->ly, hud_hover->lw, hud_hover->lh, 1, true, 0, 255, 0, 25);
				}

				return true;
			}

			// Placement at the screen (We don't care about stuff like IFREE, HBAR, SBAR and so on).
			if(!strcmp(selected_hud->place->string, "screen"))
			{
				// Not allowed, already placed there.
				Draw_AlphaRectangleRGB(0, 0, vid.width, vid.height, 1, true, 255, 0, 0, 25);
			}
			else
			{
				// Allowed.
				Draw_AlphaRectangleRGB(0, 0, vid.width, vid.height, 1, true, 0, 255, 0, 25);
			}
		}
	}
	else if(hud_editor_prevmode == hud_editmode_place && isCtrlDown())
	{
		// We've just exited placement mode, but control is still pressed,
		// that means we should place the selected_hud.
		// (If you release ctrl before you release Mouse 1, you cancel the place operation).

		if(selected_hud)
		{
			// If we're hovering a HUD place it there.
			if(hud_hover)
			{
				// We're trying to place the HUD on itself or on the HUD it's already placed at so do nothing.
				if(hud_hover == selected_hud || (selected_hud->place_hud && selected_hud->place_hud == hud_hover))
				{
					return true;
				}

				// Place at other HUD.
				Cvar_Set(selected_hud->align_x, "center");
				Cvar_Set(selected_hud->align_y, "center");
				Cvar_Set(selected_hud->pos_x, "0");
				Cvar_Set(selected_hud->pos_y, "0");
				Cvar_Set(selected_hud->place, hud_hover->name);

				// Make sure the child has a higher order.
				if((int)selected_hud->order->value <= (int)hud_hover->order->value)
				{
					Cvar_SetValue(selected_hud->order, hud_hover->order->value + 1);
				}

				HUD_Recalculate();

				// Free selection.
				selected_hud = NULL;
				return true;
			}
			else
			{
				// Mouse button was released on on the "screen".

				if(!strcmp(selected_hud->place->string, "screen"))
				{
					return false;
				}

				// Place at other HUD.
				Cvar_Set(selected_hud->align_x, "center");
				Cvar_Set(selected_hud->align_y, "center");
				Cvar_Set(selected_hud->pos_x, "0");
				Cvar_Set(selected_hud->pos_y, "0");
				Cvar_Set(selected_hud->place, "screen");
				HUD_Recalculate();

				// Free selection.
				selected_hud = NULL;
				return true;
			}
		}
	}

	// We weren't placing something so check other states also.
	return false;
}

// =============================================================================
//							HUD Editor Grep Handles.
// =============================================================================
// These show up when a HUD element is moved completly offscreen (by accident
// most likely), so that the user can still grab it and move it back onto the
// screen again.
// =============================================================================

//
// Finds a HUD grephandle (if the hud element has gone offscreen).
//
static hud_grephandle_t *HUD_Editor_FindGrep(hud_t *hud_element)
{
	hud_grephandle_t *greps_it = NULL;
	greps_it = hud_greps;

	while(greps_it)
	{
		if(!strcmp(greps_it->hud->name, hud_element->name))
		{
			return greps_it;
		}

		greps_it = greps_it->next;
	}

	return NULL;
}

//
// Returns an "offscreen" arrow in the correct direction based on where
// offscreen the HUD element is located.
//
static char *HUD_Editor_GetGrepArrow(hud_grephandle_t *grep)
{
	switch(grep->pos)
	{
		case pos_top :		return "/\\";
		case pos_bottom :	return "\\/";
		case pos_left :		return "<<<";
		case pos_right :	return ">>>";
		default :			return "";
	}
}

//
// Draws the grephandles.
//
static void HUD_Editor_DrawGreps()
{
	clrinfo_t color, highlight;
	hud_grephandle_t *greps_it = NULL;
	greps_it = hud_greps;

	// Highlight color (yellow).
	Vector4Set(highlight.c, 0, 255, 0, 255);

	// Orange.
	Vector4Set(color.c, 255, 150, 0, 255);

	while(greps_it)
	{
		Draw_ColoredString3(greps_it->x, greps_it->y,
			va("%s %s", HUD_Editor_GetGrepArrow(greps_it), greps_it->hud->name),
			(greps_it->highlighted ? &highlight : &color), 1, 0);

		greps_it = greps_it->next;
	}
}

//
// Get's the position offscreen for a HUD element
// left/right/top/bottom or visible if it's not offscreen.
//
static hud_greppos_t HUD_Editor_GetHudGrepPosition(hud_t *hud)
{
	if(hud->lx + hud->lw <= 0)
	{
		return pos_left;
	}
	else if(hud->lx >= (signed)vid.width)
	{
		return pos_right;
	}
	else if(hud->ly + hud->lh <= 0)
	{
		return pos_top;
	}
	else if(hud->ly >= (signed)vid.height)
	{
		return pos_bottom;
	}

	return pos_visible;
}

//
// Positions a grephandle based on it's position and where the HUD element
// it's associated with is located.
//
static void HUD_Editor_PositionGrep(hud_t *hud_element, hud_grephandle_t *grep)
{
	// Get the position of the grephandle.
	grep->pos = HUD_Editor_GetHudGrepPosition(hud_element);

	// Position the grephandle on screen.
	switch(grep->pos)
	{
		case pos_top :
			grep->x	= hud_element->lx;
			grep->y	= 5;
			break;
		case pos_bottom :
			grep->x	= hud_element->lx;
			grep->y	= vid.height - grep->height - 5;
			break;
		case pos_left :
			grep->x = 5;
			grep->y = hud_element->ly;
			break;
		case pos_right :
			grep->x = vid.width - grep->width - 5;
			grep->y = hud_element->ly;
			break;
		default :
			grep->x = hud_element->lx;
			grep->y = hud_element->ly;
			break;
	}
}

//
// Creates a new grephandle and associates it with a HUD element that is offscreen.
//
static hud_grephandle_t *HUD_Editor_CreateGrep(hud_t *hud_element)
{
	hud_grephandle_t *grep = NULL;

	grep			= Q_malloc(sizeof(hud_grephandle_t));
	memset(grep, 0, sizeof(*grep));

	grep->width		= 8 * (4 + strlen(hud_element->name));
	grep->height	= 8;
	grep->hud		= hud_element;
	grep->previous	= NULL;
	grep->next		= hud_greps;
	if(hud_greps)
	{
		hud_greps->previous = grep;
	}

	hud_greps = grep;

	HUD_Editor_PositionGrep(hud_element, grep);

	return grep;
}

//
// Destroys a grephandle (called if it's no longer offscreen).
//
static void HUD_Editor_DestroyGrep(hud_grephandle_t *grep)
{
	// Already destroyed.
	if(!grep)
	{
		return;
	}

	// Relink any neighbours in the list.
	if(grep->next && grep->previous)
	{
		grep->previous->next = grep->next;
		grep->next->previous = grep->previous;
	}
	else if(grep->next)
	{
		grep->next->previous = NULL;
		hud_greps = grep->next;
	}
	else if(grep->previous)
	{
		grep->previous->next = NULL;
	}
	else
	{
		hud_greps = NULL;
	}

	memset(grep, 0, sizeof(*grep));

	Q_free(grep);
}

//
// Finds a HUD element associated with the grephandle under the mouse cursor.
//
static hud_t *HUD_Editor_FindHudByGrep()
{
	hud_grephandle_t *greps_it = NULL;
	greps_it = hud_greps;

	while(greps_it)
	{
		if (greps_it->hud
			&& hud_mouse_x >= greps_it->x
			&& hud_mouse_x <= (greps_it->x + greps_it->width)
			&& hud_mouse_y >= greps_it->y
			&& hud_mouse_y <= (greps_it->y + greps_it->height))
		{
			greps_it->highlighted = true;
			return greps_it->hud;
		}

		greps_it->highlighted = false;

		greps_it = greps_it->next;
	}

	return NULL;
}

//
// Draws the outline of the visible HUD elements.
//
static void HUD_Editor_DrawOutlines(void)
{
	hud_t *temp_hud	= hud_huds;

	if(!temp_hud || !hud_editor_showoutlines)
	{
		return;
	}

	while(temp_hud->next)
	{
		// Check if the item is visible.
		if (!temp_hud->show->value || (temp_hud->place_hud && !temp_hud->place_hud->show->value))
		{
			temp_hud = temp_hud->next;
			continue;
		}

		// Draw an outline for all hud elements (faint).
		Draw_AlphaRectangleRGB(temp_hud->lx, temp_hud->ly, temp_hud->lw, temp_hud->lh, 1, false, 0, 255, 0, 25);

		temp_hud = temp_hud->next;
	}
}

//
// Finds if there's any HUD under the cursor and draws outlines for all HUD elements.
//
static qbool HUD_Editor_FindHudUnderCursor(hud_t **hud)
{
	hud_grephandle_t	*grep		= NULL;
	hud_greppos_t		pos			= pos_visible;
	hud_t				*temp_hud	= hud_huds;
	qbool				found		= false;

	if(!temp_hud)
	{
		return false;
	}

	// Check if we already had something selected since last time and was moving.
	if(selected_hud && (hud_editor_mode == hud_editmode_move_resize || hud_editor_mode == hud_editmode_move_lockedaxis))
	{
		found = true;
		(*hud) = selected_hud;
	}

	// If the hover list is being showed, only look for HUDs in that
	// not the HUD's that are below the cursor.
	if(hud_editor_mode == hud_editmode_hoverlist && hud_hoverlist_count > 0)
	{
		(*hud) = HUD_Editor_FindHoverListSelection(hud_hoverlist);
		return (hud != NULL);
	}

	// Reset the hoverlist since we might be hovering something new.
	hud_hoverlist_count = 0;
	hud_hoverlist = NULL;

	while(temp_hud)
	{
		// Not visible.
		if (!temp_hud->show->value || (temp_hud->place_hud && !temp_hud->place_hud->show->value))
		{
			temp_hud = temp_hud->next;
			continue;
		}

		// We found one.
		if (hud_mouse_x >= temp_hud->lx
			&& hud_mouse_x <= (temp_hud->lx + temp_hud->lw)
			&& hud_mouse_y >= temp_hud->ly
			&& hud_mouse_y <= (temp_hud->ly + temp_hud->lh))
		{
			// If we're moving/resizing something only continue checking for
			// more HUD elements we have the mouse over if we haven't already
			// found one. If we don't do this when you drag a HUD element
			// over another HUD element that has a greater Z-order the selection
			// will jump to that HUD element, and you'll start moving that instead.
			//
			// Vice versa if we would skip any HUD item after we've found one
			// when not already moving an item, it would mean that we could only
			// select HUD elements that are topmost in the Z-order, so an item
			// placed within another item would not be selectable.
			if(((hud_editor_mode == hud_editmode_move_resize || hud_editor_mode == hud_editmode_move_lockedaxis) && !found)
				|| (hud_editor_mode != hud_editmode_move_resize && hud_editor_mode != hud_editmode_move_lockedaxis))
			{
				found = true;
				(*hud) = temp_hud;

				// Add this HUD to the list of HUDs under the cursor.
				HUD_Editor_AddHoverHud(HUD_Editor_CreateHoverHud(temp_hud));
			}
		}

		// Check if the hud element is offscreen and
		// if there's any grep handle for this hud element
		{
			pos = HUD_Editor_GetHudGrepPosition(temp_hud);
			grep = HUD_Editor_FindGrep(temp_hud);

			if(pos != pos_visible)
			{
				// We didn't find any grep handle so create one.
				if(!grep)
				{
					grep = HUD_Editor_CreateGrep(temp_hud);
				}

				// Position the grep if we got one.
				if(grep)
				{
					HUD_Editor_PositionGrep(temp_hud, grep);
				}
			}
			else
			{
				// The HUD element is visbile, so no need for a grep handle for it.
				HUD_Editor_DestroyGrep(grep);
			}
		}

		temp_hud = temp_hud->next;
	}

	// We didn't find any HUD's under the cursor, but
	// what about "grep handles" (for offscreen HUDs).
	if(!found)
	{
		temp_hud = HUD_Editor_FindHudByGrep();

		if(temp_hud)
		{
			found = true;
			(*hud) = temp_hud;
		}
	}

	// Nothing found, make sure result is NULL.
	if (!found)
	{
		(*hud) = NULL;
	}

	return found;
}

//
// Returns the point a HUD is aligned to on it's parent in screen coordinates.
//
static void HUD_Editor_GetAlignmentPoint(hud_t *hud, int *x, int *y)
{
	extern float scr_con_current; // Console height. console.c
	int parent_x = 0;
	int parent_y = 0;
	int parent_w = 0;
	int parent_h = 0;
	hud_alignmode_t alignmode = HUD_Editor_GetAlignmentFromString(va("%s %s", hud->align_x->string, hud->align_y->string));

	if(hud->place_hud)
	{
		// Placed at another HUD.
		parent_x = hud->place_hud->lx;
		parent_y = hud->place_hud->ly;
		parent_w = hud->place_hud->lw;
		parent_h = hud->place_hud->lh;

		(*x) = HUD_CENTER_X(hud->place_hud);
		(*y) = HUD_CENTER_Y(hud->place_hud);
	}
	else
	{
		// Placed at "screen".
		parent_x = 0;
		parent_y = 0;
		parent_w = vid.width;
		parent_h = vid.height;

		(*x) = vid.width / 2;
		(*y) = vid.height / 2;
	}

	switch(alignmode)
	{
		default:
		case hud_align_center:
			// Already set.
			break;
		case hud_align_right:
			(*x) = parent_x + parent_w;
			(*y) = parent_y + (parent_h / 2);
			break;
		case hud_align_topright:
			(*x) = parent_x + parent_w;
			(*y) = parent_y;
			break;
		case hud_align_top:
			(*x) = parent_x + (parent_w / 2);
			(*y) = parent_y;
			break;
		case hud_align_topleft:
			(*x) = parent_x;
			(*y) = parent_y;
			break;
		case hud_align_left:
			(*x) = parent_x;
			(*y) = parent_y + (parent_h / 2);
			break;
		case hud_align_bottomleft:
			(*x) = parent_x;
			(*y) = parent_y + parent_h;
			break;
		case hud_align_bottom:
			(*x) = parent_x + (parent_w / 2);
			(*y) = parent_y + parent_h;
			break;
		case hud_align_bottomright:
			(*x) = parent_x + parent_w;
			(*y) = parent_y + parent_h;
			break;
		case hud_align_consoleleft:
			(*x) = parent_x;
			(*y) = scr_con_current;
			break;
		case hud_align_console:
			(*x) = parent_x + (parent_w / 2);
			(*y) = scr_con_current;
			break;
		case hud_align_consoleright:
			(*x) = parent_x + parent_w;
			(*y) = scr_con_current;
			break;
	}
}

//
// Draws a green line to each corner of a HUD element from a specified point.
//
/*
static void HUD_Editor_DrawLinesToEachCorner(hud_t *hud, int x, int y)
{
	Draw_AlphaLineRGB(hud->lx, hud->ly,						x, y, 1, RGBA_TO_COLOR(0, 255, 0, 25));
	Draw_AlphaLineRGB(hud->lx, hud->ly + hud->lh,			x, y, 1, RGBA_TO_COLOR(0, 255, 0, 25));
	Draw_AlphaLineRGB(hud->lx + hud->lw, hud->ly,			x, y, 1, RGBA_TO_COLOR(0, 255, 0, 25));
	Draw_AlphaLineRGB(hud->lx + hud->lw, hud->ly + hud->lh, x, y, 1, RGBA_TO_COLOR(0, 255, 0, 25));
}
*/

//
// Draws connections to/from a HUD element.
//
static void HUD_Editor_DrawConnections(hud_t *hud_hover)
{
	hud_t *child = NULL;
	int align_x = 0.0;
	int align_y = 0.0;

	if (!hud_hover || !hud_editor_showoutlines)
	{
		return;
	}

	// Get the alignment point at the parent in screen coordinates.
	HUD_Editor_GetAlignmentPoint(hud_hover, &align_x, &align_y);

	// Draw a line to the parent of the HUD we're hovering.
	Draw_AlphaLineRGB(HUD_CENTER_X(hud_hover), HUD_CENTER_Y(hud_hover), align_x, align_y, 1, 0, 255, 0, 25);

	// Draw a line to all children of the HUD we're hovering.
	while((child = HUD_Editor_FindNextChild(hud_hover)))
	{
		// Don't bother with hidden children.
		if(!child->show->value)
		{
			continue;
		}

		// Draw a line from the alignment point on the parent to the child.
		HUD_Editor_GetAlignmentPoint(child, &align_x, &align_y);
		Draw_AlphaLineRGB(align_x, align_y, HUD_CENTER_X(child), HUD_CENTER_Y(child), 1, 0, 255, 0, 25);
	}
}

//
// Evaluates the current mouse/keyboard state and sets the appropriate mode.
//
static void HUD_Editor_EvaluateState(hud_t *hud_hover)
{
	// Mouse 1			= Move + Resize
	// Mouse 2			= Toggle menu
	// Ctrl  + Mouse 1	= Place
	// Alt	 + Mouse 1	= Align
	// Shift + Mouse 1	= Lock moving to one axis (If you start dragging along x-axis, it will stick to that)

	if (MOUSEDOWN)
	{
		// Turn of help on mouse click.
		hud_editor_showhelp = false;
	}

	if (hud_editor_mode == hud_editmode_hoverlist)
	{
		if (!MOUSEDOWN)
		{
			// Stay in hoverlist mode until the user clicks something.
			return;
		}
	}

	if (hud_hover && MOUSEDOWN_1_ONLY && isShiftDown())
	{
		// Move (Locked to an axis).
		HUD_Editor_SetMode(hud_editmode_move_lockedaxis);
	}
	else if ((hud_hover || hud_editor_prevmode == hud_editmode_place) && MOUSEDOWN_1_ONLY && isCtrlDown())
	{
		// Place.
		HUD_Editor_SetMode(hud_editmode_place);
	}
	else if ((hud_hover || hud_editor_prevmode == hud_editmode_align) && MOUSEDOWN_1_ONLY && isAltDown())
	{
		// Align.
		HUD_Editor_SetMode(hud_editmode_align);
	}
	else if ((hud_hover || selected_hud) && MOUSEDOWN_1_ONLY)
	{
		// Move + resize.
		HUD_Editor_SetMode(hud_editmode_move_resize);
	}
	else if (hud_hoverlist_count > 1 && MOUSEDOWN_2_ONLY)
	{
		// Hover list for when hovering more than one HUD.
		HUD_Editor_SetMode(hud_editmode_hoverlist);
	}
	else if (hud_hover && MOUSEDOWN_2_ONLY)
	{
		// HUD element menu for the HUD element we have the mouse over.
		HUD_Editor_SetMode(hud_editmode_hudmenu);
	}
	else if (MOUSEDOWN_2_ONLY)
	{
		// Main menu for adding HUDs if we right click non-occupied space.
		HUD_Editor_SetMode(hud_editmode_menu);
	}
	else
	{
		// Nothing special happening.
		HUD_Editor_SetMode(hud_editmode_normal);
	}
}

//
// Draws the tooltips for a HUD element based on the state we're in.
//
static void HUD_Editor_DrawTooltips(hud_t *hud_hover)
{
	char *message = NULL;
	byte color[4] = {0, 0, 0, 0};

	if(!hud_hover)
	{
		return;
	}

	if (selected_hud)
	{
		switch(hud_editor_mode)
		{
			case hud_editmode_move_lockedaxis :
			case hud_editmode_move_resize :
			{
				message = va("(%d, %d) moving %s", (int)selected_hud->pos_x->value, (int)selected_hud->pos_y->value, selected_hud->name);
				color[0] = 255;
				color[3] = 125;
				break;
			}
			case hud_editmode_align :
			{
				char *align = NULL;

				align = HUD_Editor_GetAlignmentString(hud_alignmode);

				message = va("align %s to %s", selected_hud->name, align);
				color[1] = 255;
				color[2] = 255;
				color[3] = 125;

				break;
			}
			case hud_editmode_place :
			{
				message = va("placing %s", selected_hud->name);
				color[0] = 255;
				color[3] = 125;
				break;
			}
			case hud_editmode_normal :
			{
				message = hud_hover->name;
				color[2] = 255;
				color[3] = 125;
				break;
			}
			// unhandled
			case hud_editmode_off:
			case hud_editmode_resize:
			case hud_editmode_hudmenu:
			case hud_editmode_menu:
			case hud_editmode_hoverlist:
				break;
		}
	}

	if(!message)
	{
		message = hud_hover->name;
		color[2] = 255;
		color[3] = 125;
	}

	HUD_Editor_DrawTooltip(hud_mouse_x, hud_mouse_y, message, color[0], color[1], color[2], color[3]);
}

//
// Draws a help window.
//
static void HUD_Editor_DrawHelp()
{
	#define HUD_EDITOR_HELP_BORDER	32
	#define HUD_EDITOR_HELP_WIDTH	min(vid.width - (2 * HUD_EDITOR_HELP_BORDER), 500)
	#define HUD_EDITOR_HELP_HEIGHT	(vid.height - (2 * HUD_EDITOR_HELP_BORDER))
	#define HUD_EDITOR_HELP_X		((vid.width - HUD_EDITOR_HELP_WIDTH) / 2)
	#define HUD_EDITOR_HELP_Y		HUD_EDITOR_HELP_BORDER
	#define HUD_EDITOR_HELP_TITLE	"&cfd0HUD EDITOR HELP"

	Draw_TextBox(HUD_EDITOR_HELP_X, HUD_EDITOR_HELP_Y, HUD_EDITOR_HELP_WIDTH / 8, HUD_EDITOR_HELP_HEIGHT / 8);

	Draw_ColoredString(
		HUD_EDITOR_HELP_X + ((HUD_EDITOR_HELP_WIDTH - strlen(HUD_EDITOR_HELP_TITLE) * 8) / 2),
		HUD_EDITOR_HELP_Y + 10,
		HUD_EDITOR_HELP_TITLE, 1);

	UI_PrintTextBlock(
		HUD_EDITOR_HELP_X + 10,
		HUD_EDITOR_HELP_Y + 30,
		HUD_EDITOR_HELP_WIDTH,
		HUD_EDITOR_HELP_HEIGHT - 30,
		"The HUD Editor helps you to customize your Heads Up Display. "
		"When you move the cursor over a HUD element it will be "
		"highlighted and it's name will be shown. When hovering a HUD "
		"you can perform the following actions:\n"
		"\n"
		"&cfd0MOVE&r relative to the HUD elements parent/alignment by "
		"holding down &c0dfMOUSE 1&r and &c0dfdragging&r.\n"
		"(Lock movement to a specific axis by holding down &c0dfSHIFT&r).\n"
		"\n"
		"&cfd0RESIZE&r the HUD element by clicking on one of the "
		"&c0dfresize handles&r that appears when hovering and item and "
		"&c0dfdragging&r. (Not all HUD elements are resizeable/scaleable).\n"
		"\n"
		"&cfd0PLACE&r the HUD element at another HUD element or a location "
		"such as the screen/console by holding down &c0dfCTRL&r when "
		"dragging. The target that the element will be placed in "
		"will turn green if you can place it there, red otherwise.\n"
		"\n"
		"&cfd0ALIGN&r the HUD element in different ways to it's parent by "
		"holding down &c0dfALT&r when dragging. Doing this will show "
		"yellow highlights at the position you're about to align to.\n"
		"\n"
		"  &cfd0Keyboard shortcuts:\n"
		"  &c0dfP&r      Toggle HUD planmode on/off (default on).\n"
		"  &c0dfH&r      Toggle this help.\n"
		"  &c0dfF1&r     Toggle if moving should be allowed.\n"
		"  &c0dfF2&r     Toggle resizing.\n"
		"  &c0dfF3&r     Toggle aligning.\n"
		"  &c0dfF4&r     Toggle placing.\n"
		"  &c0dfSPACE&r  Toggle outlines/guidelines.\n",
		0);
}

#ifdef HAXX
int Test_OnGotFocus(ez_control_t *self, void *payload, void *ext_event_info)
{
	EZ_control_SetBackgroundColor(self, self->background_color[0], self->background_color[1], self->background_color[2], 200);
	return 0;
}

int Test_OnLostFocus(ez_control_t *self, void *payload, void *ext_event_info)
{
	EZ_control_SetBackgroundColor(self, self->background_color[0], self->background_color[1], self->background_color[2], 100);
	return 0;
}

ez_control_t *root = NULL;
ez_control_t *child1 = NULL;
ez_control_t *child2 = NULL;
ez_button_t *button = NULL;
ez_label_t *label = NULL;
ez_label_t *label2 = NULL;
ez_slider_t *slider = NULL;
ez_scrollbar_t *scrollbar = NULL;
ez_scrollpane_t *scrollpane = NULL;
ez_listview_t *listview = NULL;
ez_window_t *window = NULL;

int Test_OnButtonDraw(ez_control_t *self, void *payload, void *ext_event_info)
{
	int x, y;
	EZ_control_GetDrawingPosition(self, &x, &y);

	return 0;
}

int Test_OnSliderPositionChanged(ez_control_t *self, void *payload, void *ext_event_info)
{
	ez_slider_t *slider = (ez_slider_t *)self;

	int slider_pos = EZ_slider_GetPosition(slider);

	EZ_label_SetText(label2, va("%i", slider_pos));

	return 0;
}

int Test_OnControlDraw(ez_control_t *self, void *payload, void *ext_event_info)
{
	int x, y; //, i;
	EZ_control_GetDrawingPosition(self, &x, &y);

	/*for (i = 0; i < 30; i++)
	{
		Draw_String(x, y + i*8, va("%d%d%d%d", i, i, i, i));
	}*/

	/*
	{
		char str[] = "Hello this is a sentence that's supposed to fit within a box of stuff, I hope this works...";
		char line[1024];
		int last_index = 0;
		int i = 0;

		while (Util_GetNextWordwrapString(str, line, last_index, &last_index, sizeof(str) / sizeof(char), self->width, 8))
		{
			Draw_String(x, y + i*8, line);
			i++;
		}
	}
	*/

	/*{
		clrinfo_t color;
		color.i = 0;
		color.c = RGBA_TO_COLOR(0, 255, 0, 255);
		Draw_BigString(x, y, "Hej", &color, 1, 1, 1, 0);
	}*/

	return 0;
}
#endif

static hud_t *hud_hover = NULL;

//
// Main HUD Editor function.
//
static void HUD_Editor(void)
{
	qbool found = false;

	// If we just entered hoverlist mode we want to keep the mouse coordinates
	// so we know where to draw the list until the user has picked a HUD.
	if (!hud_hoverlist_pos_is_set
		&& hud_editor_mode == hud_editmode_hoverlist
		&& hud_editor_prevmode != hud_editmode_hoverlist)
	{
		hud_hoverlist_x = cursor_x;
		hud_hoverlist_y = cursor_y;
		hud_hoverlist_pos_is_set = true;
	}
	else if (hud_editor_mode != hud_editmode_hoverlist)
	{
		hud_hoverlist_pos_is_set = false;
	}

	// Find the HUD we're moving or have the cursor over.
	found = HUD_Editor_FindHudUnderCursor(&hud_hover);

	// Draw faint outlines for all visible hud elements.
	HUD_Editor_DrawOutlines();

	// Draw the "grep handles" for offscreen HUDs.
	HUD_Editor_DrawGreps();

	// Draw a rectangle around the currently active HUD element.
	if(found && hud_hover)
	{
		Draw_AlphaRectangleRGB(hud_hover->lx, hud_hover->ly, hud_hover->lw, hud_hover->lh, 1, false, 0, 255, 0, 255);
	}

	// If we are realigning draw a green outline for the selected hud element.
	if (selected_hud)
	{
		Draw_AlphaRectangleRGB(selected_hud->lx, selected_hud->ly, selected_hud->lw, selected_hud->lh, 2, false, 0, 255, 0, 255);
	}

	// Check the mouse/keyboard states and if we're hovering above a hud or not.
	HUD_Editor_EvaluateState(hud_hover);

	// Draw the child/parent connections the hud we're hovering has.
	HUD_Editor_DrawConnections(hud_hover);

	// Draw a red line from selected hud to cursor.
	if (selected_hud)
	{
		Draw_AlphaLineRGB(hud_mouse_x, hud_mouse_y, HUD_CENTER_X(selected_hud), HUD_CENTER_Y(selected_hud), 1, 255, 0, 0, 255);
	}

	// Check if we're performing any action.
	// (Only perform one at any given time).
	(void)
	(HUD_Editor_DrawHoverList(hud_hoverlist_x, hud_hoverlist_y, hud_hoverlist)
		 || HUD_Editor_Resizing(hud_hover)
		 || HUD_Editor_Moving(hud_hover)
		 || HUD_Editor_Placing(hud_hover)
		 || HUD_Editor_Aligning(hud_hover));

	// Draw tooltips for the HUD.
	HUD_Editor_DrawTooltips(hud_hover);

	// Show the help window?
	if(hud_editor_showhelp)
	{
		HUD_Editor_DrawHelp();
	}

#ifdef HAXX
	EZ_tree_EventLoop(&help_control_tree);
#endif
}

//
// Toggles the HUD Editor on or off.
//
void HUD_Editor_Toggle_f(void)
{
	// static keydest_t key_dest_prev = key_game;
	static int old_hud_planmode = 0;

	if (cls.state != ca_active)
	{
		// We can't turn on the hud editor when disconnected.
		if(!hud_editor)
		{
			Com_Printf("You need to be in game to use the HUD editor.\n");
		}

		// If the hud editor managed to still be on while disconnected.
		hud_editor = false;
	}
#ifdef HAXX
	else if (!scr_newHud->value)
	{
		Com_Printf("You have to have scr_newHud turned on to use the HUD editor.\n");
		hud_editor = false;
	}
#endif
	else
	{
		// Toggle.
		hud_editor = !hud_editor;
//		S_LocalSound("misc/basekey.wav");
	}

	if (hud_editor)
	{
		// Start HUD Editor.

		inputfuncs->SetMenuFocus(true, "", 0, 0, 0);
		HUD_Editor_SetMode(hud_editmode_normal);

		// Set planmode by default.
		old_hud_planmode = hud_planmode->value;
		Cvar_SetValue(hud_planmode, 1.0);

		// Start showing the help plaque so the user learns the controls.
		hud_editor_showhelp = true;
	}
	else
	{
		// Exit the HUD Editor.

		inputfuncs->SetMenuFocus(false, "", 0, 0, 0);
		HUD_Editor_SetMode(hud_editmode_off);
		scr_cursor_icon = NULL;

		// Reset to the old value for HUD planmode.
		Cvar_SetValue(hud_planmode, old_hud_planmode);
	}
}

//
// Handles mouse events sent to the HUD editor.
//
qbool HUD_Editor_MouseEvent(float x, float y)
{
	// Updating cursor location.
	if(hud_editor_mode == hud_editmode_move_lockedaxis)
	{
		// Don't update the HUD Editor cursor if the axis is locked
		// so that we avoid explicit checks for that.
		// The cursor will still move around the screen properly.
		if(hud_editor_locked_axis_is_x)
		{
			hud_mouse_x = x;

			// Draw a line that indicates that the movement is locked to the X-axis.
			if (selected_hud)
			{
				Draw_AlphaLineRGB(0, HUD_CENTER_Y(selected_hud), vid.width, HUD_CENTER_Y(selected_hud), 1, 255, 0, 0, 75);
			}
		}
		else
		{
			hud_mouse_y = y;

			if (selected_hud)
			{
				Draw_AlphaLineRGB(HUD_CENTER_X(selected_hud), 0, HUD_CENTER_X(selected_hud), vid.height, 1, 255, 0, 0, 75);
			}
		}
	}
	else
	{
		// Normal operation, always update cursor.
		hud_mouse_x = x;
		hud_mouse_y = y;
	}

#ifdef HAXX
	return EZ_tree_MouseEvent(&help_control_tree, ms);
#else
	return true;
#endif
}

//
// Handles key events sent to the HUD editor.
//
void HUD_Editor_Key(int key, int unichar, qbool down)
{
	static int planmode = 1;
#ifdef HAXX
	int togglekeys[2];

	EZ_tree_KeyEvent(&help_control_tree, key, unichar, down);

	M_FindKeysForCommand("toggleconsole", togglekeys);
	if ((key == togglekeys[0]) || (key == togglekeys[1]))
	{
		Con_ToggleConsole_f();
		return;
	}
#endif

	if (down)
	{
		switch (key)
		{
			case K_ESCAPE:
			case K_GP_BACK:
				HUD_Editor_Toggle_f();
				break;
			case 'p' :
				// Toggle HUD plan mode.
				planmode = !planmode;
				Cvar_SetValue(hud_planmode, planmode);
				break;
			case 'h' :
				// Toggle the help window.
				hud_editor_showhelp = !hud_editor_showhelp;
				break;
			case K_SPACE :
				// Toggle hud element outlines.
				hud_editor_showoutlines = !hud_editor_showoutlines;
				break;
			case K_F1 :
				// Toggle moving.
				Cvar_SetValue(hud_editor_allowmove, !hud_editor_allowmove->value);
				break;
			case K_F2 :
				// Toggle resizing.
				Cvar_SetValue(hud_editor_allowresize, !hud_editor_allowresize->value);
				break;
			case K_F3 :
				// Toggle aligning.
				Cvar_SetValue(hud_editor_allowalign, !hud_editor_allowalign->value);
				break;
			case K_F4 :
				// Toggle placing.
				Cvar_SetValue(hud_editor_allowplace, !hud_editor_allowplace->value);
				break;
			case K_UPARROW :
			case K_KP_UPARROW:
			case K_GP_DPAD_UP:
				// TODO : Add "nudging" in hud editor.
				break;
			case K_DOWNARROW :
			case K_KP_DOWNARROW:
			case K_GP_DPAD_DOWN:
				// TODO : Add "nudging" in hud editor.
				break;
			case K_LEFTARROW :
			case K_KP_LEFTARROW:
			case K_GP_DPAD_LEFT:
				// TODO : Add "nudging" in hud editor.
				break;
			case K_RIGHTARROW :
			case K_KP_RIGHTARROW:
			case K_GP_DPAD_RIGHT:
				// TODO : Add "nudging" in hud editor.
				break;
		}
	}
}

//
// Inits HUD Editor.
//
void HUD_Editor_Init(void)
{
	extern mpic_t *SCR_LoadCursorImage(char *cursorimage);

#if 0
	clrinfo_t color;

	color.c = RGBA_TO_COLOR(255, 255, 255, 255);
	color.i = 0;

	// Root
	{
		root = EZ_control_Create(&help_control_tree, NULL, "Test window", "Test", 50, 50, 400, 400, control_focusable | control_movable | control_resize_h | control_resize_v);
		EZ_control_SetBackgroundColor(root, 0, 0, 0, 100);
		EZ_control_SetSize(root, 400, 400);
	}

	// Child 1
	{
		child1 = EZ_control_Create(&help_control_tree, root, "Child 1", "Test", 10, 10, 50, 50, control_focusable | control_movable | control_contained | control_scrollable | control_resizeable);

		EZ_control_AddOnGotFocus(child1, Test_OnGotFocus, NULL);
		EZ_control_AddOnLostFocus(child1, Test_OnLostFocus, NULL);
		EZ_control_AddOnDraw(child1, Test_OnControlDraw, NULL);

		EZ_control_SetMinVirtualSize(child1, child1->width * 3, child1->height * 3);
		//EZ_control_SetVirtualSize(child1, child1->width * 4, child1->height * 2);

		EZ_control_SetBackgroundColor(child1, 150, 150, 0, 100);
	}

	// Child 2
	{
		child2 = EZ_control_Create(&help_control_tree, root, "Child 2", "Test", 30, 50, 50, 20, control_focusable | control_contained);

		EZ_control_AddOnGotFocus(child2, Test_OnGotFocus, NULL);
		EZ_control_AddOnLostFocus(child2, Test_OnLostFocus, NULL);

		EZ_control_SetBackgroundColor(child2, 150, 150, 200, 100);
	}

	// Button.
	{
		button = EZ_button_Create(&help_control_tree, child1, "button", "A crazy button!", 15, -15, 80, 60, control_contained | control_resizeable);
		EZ_control_AddOnDraw((ez_control_t *)button, Test_OnButtonDraw, NULL);

		EZ_button_SetFocusedColor(button, 255, 0, 0, 255);
		EZ_button_SetNormalColor(button, 255, 255, 0, 100);
		EZ_button_SetPressedColor(button, 255, 255, 0, 255);
		EZ_button_SetHoverColor(button, 255, 0, 0, 150);

		EZ_button_SetToggleable(button, true);

		EZ_button_SetText(button, "Button");
		EZ_button_SetTextAlignment(button, middle_center);

		EZ_control_SetAnchor((ez_control_t *)button, (anchor_left | anchor_right | anchor_bottom));
	}

	// Label.
	{
		label = EZ_label_Create(&help_control_tree, root,
			"label", "A crazy label!", 200, 200, 250, 80,
			control_focusable | control_contained | control_resizeable | control_scrollable /*| control_movable */ | control_resize_h | control_resize_v,
			label_wraptext | label_autosize,
			"Hello\nthis is a test are you fine because I am bla bla bla this is a very long string and it's plenty of fun haha!");

		EZ_label_SetTextScale(label, 2.0);
		//EZ_label_SetTextFlags(label, LABEL_READONLY);

		EZ_control_SetBackgroundColor((ez_control_t *)label, 150, 150, 0, 50);
		//EZ_control_SetAnchor((ez_control_t *)label, anchor_top | anchor_right | anchor_bottom);
	}

	// Label 2.
	{
		label2 = EZ_label_Create(&help_control_tree, root,
			"label2", "A crazy label!", 100, 50, 32, 16,
			control_focusable | control_contained | control_resizeable,
			0, "");
	}

	// Slider.
	{
		slider = EZ_slider_Create(&help_control_tree, root,
			"slider", "Slider omg", 50, 100, 150, 8, control_focusable | control_contained | control_resizeable);

		EZ_control_SetAnchor((ez_control_t *)slider, anchor_left | anchor_right);

		EZ_slider_SetMax(slider, 100);
		EZ_slider_SetMin(slider, 50);
		EZ_slider_SetPosition(slider, 5);
		EZ_slider_SetScale(slider, 1.0);

		EZ_slider_AddOnSliderPositionChanged(slider, Test_OnSliderPositionChanged, NULL);
	}

	/*
	// Scrollbar.
	{
		ez_control_t *label_ctrl = (ez_control_t *)label;

		scrollbar = EZ_scrollbar_Create(&help_control_tree, root, "Scrollbar", "",
			30, 150, 10, 150, control_anchor_viewport);

		EZ_scrollbar_SetTargetParent(scrollbar, false);
		//EZ_control_SetContained((ez_control_t *)scrollbar, false);
		EZ_control_SetAnchor((ez_control_t *)scrollbar, anchor_right | anchor_top | anchor_bottom);
		EZ_control_SetMovable((ez_control_t *)scrollbar, false);
	}
	*/

	// Listview
	{
		listview = EZ_listview_Create(&help_control_tree, root, "Listview", "", 50, 50, 200, 200,
			control_resize_h | control_resize_v | control_resizeable);

		EZ_listview_SetHeaderText(listview, 0, "Hej");
		EZ_listview_SetHeaderText(listview, 1, "Hej 2");

		EZ_listview_SetColumnWidth(listview, 0, 80);
		EZ_listview_SetColumnWidth(listview, 1, 50);
	}

	// Scrollpane
	{
		scrollpane = EZ_scrollpane_Create(&help_control_tree, root, "Scrollpane", "", -10, -20, 150, 150,
			control_resize_h | control_resize_v | control_resizeable);

		EZ_control_SetBackgroundColor((ez_control_t *)scrollpane, 255, 0, 0, 100);

		//EZ_scrollpane_SetTarget(scrollpane, child1);
		EZ_scrollpane_SetTarget(scrollpane, (ez_control_t *)listview);
	}

	// Window.
	{
		window = EZ_window_Create(&help_control_tree, root, "Window", NULL, 20, 20, 150, 150,
			control_movable | control_focusable | control_resize_h | control_resize_v | control_contained);

		EZ_control_SetBackgroundColor((ez_control_t *)window, 0, 100, 0, 100);

		EZ_window_SetWindowAreaMinVirtualSize(window, 200, 200);

		//EZ_window_AddChild(window, (ez_control_t *)scrollpane);
	}

	/*
	// Test.
	{
		ez_control_t *c = EZ_control_Create(&help_control_tree, root, "C test 1", "Test", 10, 10, 150, 150,
			control_resize_h | control_focusable | control_movable | control_contained | control_scrollable | control_resizeable);

		ez_control_t *c2 = EZ_control_Create(&help_control_tree, c, "C test 1", "Test", 0, 10, 30, 10,
			control_focusable | control_movable | control_contained | control_scrollable | control_resizeable);

		EZ_control_SetAnchor(c2, anchor_top | anchor_right);
		EZ_control_SetBackgroundColor(c2, 150, 0, 20, 100);

		EZ_control_SetBackgroundColor(c, 50, 40, 50, 100);
	}
	*/

	EZ_tree_Refresh(&help_control_tree);

#endif

	// Register commands.
	Cmd_AddCommand("hud_editor", HUD_Editor_Toggle_f);

	// Register variables.
	hud_editor_allowresize	= cvarfuncs->GetNVFDG("hud_editor_allowresize",	"1", 0, NULL, "hud");
	hud_editor_allowmove	= cvarfuncs->GetNVFDG("hud_editor_allowmove",	"1", 0, NULL, "hud");
	hud_editor_allowplace	= cvarfuncs->GetNVFDG("hud_editor_allowplace",	"1", 0, NULL, "hud");
	hud_editor_allowalign	= cvarfuncs->GetNVFDG("hud_editor_allowalign",	"1", 0, NULL, "hud");

	// Load HUD editor cursor icons.
	hud_editor_move_icon = SCR_LoadCursorImage("gfx/hud_move_icon");
	hud_editor_resize_icon = SCR_LoadCursorImage("gfx/hud_resize_icon");
	hud_editor_align_icon = SCR_LoadCursorImage("gfx/hud_align_icon");
	hud_editor_place_icon = SCR_LoadCursorImage("gfx/hud_place_icon");

	hud_editor = false;
	HUD_Editor_SetMode(hud_editmode_off);
}

//
// Draws the HUD Editor if it's on.
//
void HUD_Editor_Draw(void)
{
	if (!hud_editor)
		return;

	HUD_Editor();
}

//
// Should this HUD element be fully drawn or not when in align mode
// when using the HUD editor?
//
qbool HUD_Editor_ConfirmDraw(hud_t *hud)
{
	if(hud_editor_mode == hud_editmode_align || hud_editor_mode == hud_editmode_place)
	{
		// If this is the selected hud, or the parent of the selected hud then draw it.
		if((selected_hud && !strcmp(selected_hud->name, hud->name))
			|| (selected_hud && hud->place_hud && !strcmp(selected_hud->name, hud->place_hud->name)))
		{
			return true;
		}

		return false;
	}

	return true;
}

