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
// common HUD elements
// like clock etc..
//

#ifndef __HUD_COMMON__H__
#define __HUD_COMMON__H__

extern hud_t *hud_netgraph;

void SCR_HUD_Netgraph(hud_t *hud);
void SCR_HUD_DrawFPS(hud_t *hud);
void SCR_HUD_DrawNetStats(hud_t *hud);

void SCR_HUD_DrawGun2 (hud_t *hud);
void SCR_HUD_DrawGun3 (hud_t *hud);
void SCR_HUD_DrawGun4 (hud_t *hud);
void SCR_HUD_DrawGun5 (hud_t *hud);
void SCR_HUD_DrawGun6 (hud_t *hud);
void SCR_HUD_DrawGun7 (hud_t *hud);
void SCR_HUD_DrawGun8 (hud_t *hud);
void SCR_HUD_DrawGunCurrent (hud_t *hud);

void HUD_NewMap();
void HUD_NewRadarMap();
void SCR_HUD_DrawRadar(hud_t *hud);

void HudCommon_Init(void);
void SCR_HUD_DrawNum(hud_t *hud, int num, qbool low, float scale, int style, int digits, char *s_align);

extern qbool autohud_loaded;
extern cvar_t *mvd_autohud;
void HUD_AutoLoad_MVD(int autoload);

void HUD_BeforeDraw();
void HUD_AfterDraw();
void CommonDraw_Init(void);

#endif // __HUD_COMMON__H__
