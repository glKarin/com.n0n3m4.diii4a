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
// view.h

extern	cvar_t		v_gamma;
extern	cvar_t		v_gammainverted;
extern	cvar_t		v_contrast;
extern	cvar_t		v_contrastboost;
extern	cvar_t		v_brightness;
extern float sw_blend[4];
extern float hw_blend[4];

extern qboolean r_secondaryview;

void V_Init (void);
void V_RenderView (qboolean no2d);
float V_CalcRoll (vec3_t angles, vec3_t velocity);
void V_UpdatePalette (qboolean force);
void V_ClearCShifts (void);
void V_ClearEntity(entity_t *e);
entity_t *V_AddEntity(entity_t *in);
entity_t *V_AddNewEntity(void);
void VQ2_AddLerpEntity(entity_t *in);
void V_AddAxisEntity(entity_t *in);
void CLQ1_AddShadow(entity_t *ent);
int V_AddLight (int entsource, vec3_t org, float quant, float r, float g, float b);

extern qbyte gammatable[256];	//for texture gamma.
extern qboolean gammaworks;

extern cvar_t r_projection, ffov;
extern cvar_t crosshair, crosshairalpha, crosshairsize, crosshaircolor, crosshairimage, cl_crossx, cl_crossy, crosshaircorrect;
extern cvar_t v_viewheight;

extern cvar_t v_gunkick_q2, gl_cshiftenabled, gl_cshiftborder;	//q2 logic needs some of these cvars.

extern cvar_t v_contentblend; //for menus
extern cvar_t chase_active, chase_back, chase_up;	//I fucking hate this cvar. die die die.
