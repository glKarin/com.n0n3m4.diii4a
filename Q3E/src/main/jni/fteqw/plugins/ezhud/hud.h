/*
Copyright (C) 2011 azazello and ezQuake team

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
//
// HUD commands
//

#ifndef __hud_h__
#define __hud_h__

// flags
#define HUD_NO_DRAW            (1 <<  0)  // don't draw this automatically
#define HUD_NO_SHOW            (1 <<  1)  // doesn't support show/hide
#define HUD_NO_POS_X           (1 <<  2)  // doesn't support x positioning
#define HUD_NO_POS_Y           (1 <<  3)  // doesn't support y positioning
#define HUD_NO_FRAME           (1 <<  4)  // don't add frame
#define HUD_ON_DIALOG          (1 <<  5)  // draw on dialog too
#define HUD_ON_INTERMISSION    (1 <<  6)  // draw on intermission too
#define HUD_ON_FINALE          (1 <<  7)  // draw on finale too
#define HUD_ON_SCORES          (1 <<  8)  // draw on +showscores too
#define HUD_NO_GROW            (1 <<  9)  // no frame grow
#define HUD_PLUSMINUS          (1 << 10)  // auto add +/- commands
#define HUD_OPACITY				(1 << 11)

#define HUD_INVENTORY          (HUD_NO_GROW)   // aply for sbar elements

#define HUD_MAX_PARAMS  24

#define	HUD_REGEXP_OFFSET_COUNT	20

// Placement
#define HUD_PLACE_SCREEN		1
#define HUD_PLACE_TOP			2
#define HUD_PLACE_VIEW			3
#define HUD_PLACE_SBAR			4
#define HUD_PLACE_IBAR			5
#define HUD_PLACE_HBAR			6
#define HUD_PLACE_SFREE			7
#define HUD_PLACE_IFREE			8
#define HUD_PLACE_HFREE			9

// Alignment
#define HUD_ALIGN_LEFT			1
#define HUD_ALIGN_TOP			1
#define HUD_ALIGN_CENTER		2
#define HUD_ALIGN_RIGHT			3
#define HUD_ALIGN_BOTTOM		3
#define HUD_ALIGN_BEFORE		4
#define HUD_ALIGN_AFTER			5
#define HUD_ALIGN_CONSOLE		6

typedef struct hud_s
{
    char *name;							// Element name.
    char *description;					// Little help.

    void (*draw_func) (struct hud_s *);	// Drawing function.

	cvar_t *order;						// Higher it is, later this element will be drawn
										// and more probable that will be on top.

    cvar_t *show;						// Show cvar.
    cvar_t *frame;						// Frame cvar.
	cvar_t *frame_color;				// Frame color cvar.
	byte frame_color_cache[4];			// Cache for parsed frame color.

	cvar_t *opacity;					// The overall opacity of the entire HUD element.

    // placement
    cvar_t *place;						// Place string, parsed to:
    struct hud_s *place_hud;			// if snapped to hud element
    qbool place_outside;				// if hud: inside ot outside
    int place_num;						// place number (our or parent if hud)
										// note: item is placed at another HUD element
										// if place_hud != NULL

    cvar_t *align_x;					// Alignment cvars (left, right, ...)
    cvar_t *align_y;
    int    align_x_num;					// Parsed alignment.
    int    align_y_num;

    cvar_t *pos_x;						// Position cvars.
    cvar_t *pos_y;

	cvar_t **params;					// Registered parameters for the HUD element.
    int num_params;

    cactive_t  min_state;				// At least this state is required
										// to draw this element.

    unsigned   flags;

    // Last draw parameters (mostly used by children)
    int lx, ly, lw, lh;					// Last position.
    int al, ar, at, ab;					// Last frame params.

    int last_try_sequence;				// Sequence, at which object tried to draw itself.
    int last_draw_sequence;				// Sequence, at which it was last drawn successfully.

    struct hud_s *next;					// Next HUD in the list.
} hud_t;

typedef  void (*hud_func_type) (struct hud_s *);

#define MAX_HUD_ELEMENTS 256

//
// Initialize
// 
void HUD_Init(void);

//
// Add element to list
// parameter format: "name1", "default1", ..., "nameX", "defaultX", NULL
//
hud_t * HUD_Register(char *name, char *var_alias, char *description,
                     int flags, cactive_t min_state, int draw_order,
                     hud_func_type draw_func,
                     char *show, char *place, char *align_x, char *align_y,
                     char *pos_x, char *pos_y, char *frame, char *frame_color,
                     char *item_opacity, char *params, ...);


//
// Draw all active elements.
//
void HUD_Draw(void);

//
// Retrieve hud cvar.
//
cvar_t *HUD_FindVar(hud_t *hud, char *subvar);

//
// Find element in list.
//
hud_t * HUD_Find(char *name);

//
// Calculate screen position of element.
// return value:
//    true  - draw it
//    false - don't draw, it is off screen (mayby partially)
//
qbool HUD_PrepareDraw(
			hud_t *hud, int width, int height,		// In.
			int *ret_x, int *ret_y);				// Out.

qbool HUD_PrepareDrawByName(
			char *element, int width, int height,	// In.
			int *ret_x, int *ret_y);				// Out.


// Sort all HUD Elements.
void HUD_Sort(void);

// Recalculate the position of all hud elements.
void HUD_Recalculate(void);

// when show pre-selected weapon/ammo? 1) player uses this system 2) not dead 3) when playing
#define ShowPreselectedWeap()  (cl_weaponpreselect->value && cl.stats[STAT_HEALTH] > 0 && !cls.demoplayback && !cl.spectator)

#endif // __hud_h__
