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

// draw.h -- these are the only functions outside the refresh allowed
// to touch the vid buffer

void GLDraw_Init (void);
void GLDraw_DeInit (void);
void Surf_DeInit (void);

void R2D_Init(void);
mpic_t	*R2D_SafeCachePic (const char *path);
mpic_t *R2D_SafePicFromWad (const char *name);
void R2D_ImageColours(float r, float g, float b, float a);
void R2D_Image(float x, float y, float w, float h, float s1, float t1, float s2, float t2, mpic_t *pic);
void R2D_Line(float x1, float y1, float x2, float y2, mpic_t *pic);
void R2D_ScalePic (float x, float y, float width, float height, mpic_t *pic);
void R2D_SubPic(float x, float y, float width, float height, mpic_t *pic, float srcx, float srcy, float srcwidth, float srcheight);
void R2D_Letterbox(float sx, float sy, float sw, float sh, mpic_t *pic, float pw, float ph);
void R2D_ConsoleBackground (int firstline, int lastline, qboolean forceopaque);
void R2D_EditorBackground (void);
void R2D_TileClear (float x, float y, float w, float h);
void R2D_FadeScreen (void);
void R2D_Init(void);
void R2D_Shutdown(void);

extern void (*R2D_Flush)(void);	//if set, something is queued and must be flushed at some point. if its set to what you will set it to, then you can build onto your batch yourself.

void R2D_PolyBlend (void);
void R2D_BrightenScreen (void);

void QDECL R2D_Conback_Callback(struct cvar_s *var, char *oldvalue);
