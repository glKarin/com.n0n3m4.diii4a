/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * Drawing of all images that are not textures
 *
 * =======================================================================
 */

#include "header/local.h"

image_t *draw_chars;

extern qboolean scrap_dirty;
void Scrap_Upload(void);

extern unsigned r_rawpalette[256];

void
Draw_InitLocal(void)
{
	/* load console characters (don't bilerp characters) */
	draw_chars = R_FindImage("pics/conchars.pcx", it_pic);
	R_Bind(draw_chars->texnum);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

/*
 * Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void
Draw_Char(int x, int y, int num)
{
	int row, col;
	float frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
	{
		return; /* space */
	}

	if (y <= -8)
	{
		return; /* totally off screen */
	}

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	R_Bind(draw_chars->texnum);
	
	float texcoords[4][2];
        float verts[4][2];

	verts[0][0] = x;    verts[0][1] = y;
        verts[1][0] = x+8;  verts[1][1] = y;
        verts[2][0] = x+8;  verts[2][1] = y+8;
        verts[3][0] = x;    verts[3][1] = y+8;
	
        texcoords[0][0] = fcol;      texcoords[0][1] = frow;
        texcoords[1][0] = fcol+size;   texcoords[1][1] = frow;
        texcoords[2][0] = fcol+size;   texcoords[2][1] = frow+size;
        texcoords[3][0] = fcol;      texcoords[3][1] = frow+size;
	
        qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
        qglTexCoordPointer( 2, GL_FLOAT, 0, texcoords );
        qglVertexPointer  ( 2, GL_FLOAT, 0, verts );
        qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

image_t *
Draw_FindPic(char *name)
{
	image_t *gl;
	char fullname[MAX_QPATH];

	if ((name[0] != '/') && (name[0] != '\\'))
	{
		Com_sprintf(fullname, sizeof(fullname), "pics/%s.pcx", name);
		gl = R_FindImage(fullname, it_pic);
	}
	else
	{
		gl = R_FindImage(name + 1, it_pic);
	}

	return gl;
}

void
Draw_GetPicSize(int *w, int *h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic(pic);

	if (!gl)
	{
		*w = *h = -1;
		return;
	}

	*w = gl->width;
	*h = gl->height;
}

void
Draw_StretchPic(int x, int y, int w, int h, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic(pic);

	if (!gl)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	if (scrap_dirty)
	{
		Scrap_Upload();
	}

	R_Bind(gl->texnum);

	float texcoords[4][2];
        float verts[4][2];

	verts[0][0] = x;    verts[0][1] = y;
        verts[1][0] = x+w;  verts[1][1] = y;
        verts[2][0] = x+w;  verts[2][1] = y+h;
        verts[3][0] = x;    verts[3][1] = y+h;
	
        texcoords[0][0] = gl->sl;      texcoords[0][1] = gl->tl;
        texcoords[1][0] = gl->sh;   texcoords[1][1] = gl->tl;
        texcoords[2][0] = gl->sh;   texcoords[2][1] = gl->th;
        texcoords[3][0] = gl->sl;      texcoords[3][1] = gl->th;
	
        qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
        qglTexCoordPointer( 2, GL_FLOAT, 0, texcoords );
        qglVertexPointer  ( 2, GL_FLOAT, 0, verts );
        qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

void
Draw_Pic(int x, int y, char *pic)
{
	image_t *gl;

	gl = Draw_FindPic(pic);

	if (!gl)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	if (scrap_dirty)
	{
		Scrap_Upload();
	}

	R_Bind(gl->texnum);

	float texcoords[4][2];
        float verts[4][2];

	verts[0][0] = x;    verts[0][1] = y;
        verts[1][0] = x+gl->width;  verts[1][1] = y;
        verts[2][0] = x+gl->width;  verts[2][1] = y+gl->height;
        verts[3][0] = x;    verts[3][1] = y+gl->height;
	
        texcoords[0][0] = gl->sl;      texcoords[0][1] = gl->tl;
        texcoords[1][0] = gl->sh;   texcoords[1][1] = gl->tl;
        texcoords[2][0] = gl->sh;   texcoords[2][1] = gl->th;
        texcoords[3][0] = gl->sl;      texcoords[3][1] = gl->th;
	
        qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
        qglTexCoordPointer( 2, GL_FLOAT, 0, texcoords );
        qglVertexPointer  ( 2, GL_FLOAT, 0, verts );
        qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

/*
 * This repeats a 64*64 tile graphic to fill
 * the screen around a sized down
 * refresh window.
 */
void
Draw_TileClear(int x, int y, int w, int h, char *pic)
{
	image_t *image;

	image = Draw_FindPic(pic);

	if (!image)
	{
		ri.Con_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	R_Bind(image->texnum);

	float texcoords[4][2];
        float verts[4][2];

	verts[0][0] = x;    verts[0][1] = y;
        verts[1][0] = (x+w);  verts[1][1] = y;
        verts[2][0] = (x+w);  verts[2][1] = (y+h);
        verts[3][0] = x;    verts[3][1] = (y+h);
	
        texcoords[0][0] = x/64.0;      texcoords[0][1] = y/64.0;
        texcoords[1][0] = (x+w)/64.0;   texcoords[1][1] = y/64.0;
        texcoords[2][0] = (x+w)/64.0;   texcoords[2][1] = (y+h)/64.0;
        texcoords[3][0] = x/64.0;      texcoords[3][1] = (y+h)/64.0;
	
        qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
        qglTexCoordPointer( 2, GL_FLOAT, 0, texcoords );
        qglVertexPointer  ( 2, GL_FLOAT, 0, verts );
        qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

/*
 * Fills a box of pixels with a single color
 */
void
Draw_Fill(int x, int y, int w, int h, int c)
{
	union
	{
		unsigned c;
		byte v[4];
	} color;

	if ((unsigned)c > 255)
	{
		ri.Sys_Error(ERR_FATAL, "Draw_Fill: bad color");
	}

	qglDisable(GL_TEXTURE_2D);

	color.c = d_8to24table[c];
	qglColor3f(color.v[0] / 255.0, color.v[1] / 255.0,
			color.v[2] / 255.0);

	float verts[4][2];

	verts[0][0] = x;    verts[0][1] = y;
        verts[1][0] = (x+w);  verts[1][1] = y;
        verts[2][0] = (x+w);  verts[2][1] = (y+h);
        verts[3][0] = x;    verts[3][1] = (y+h);
	
        qglVertexPointer  ( 2, GL_FLOAT, 0, verts );
        qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        
	qglColor3f(1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
}

void
Draw_FadeScreen(void)
{
	qglEnable(GL_BLEND);
	qglDisable(GL_TEXTURE_2D);
	qglColor4f(0, 0, 0, 0.8);
	float verts[4][2];

	verts[0][0] = 0;    verts[0][1] = 0;
        verts[1][0] = vid.width;  verts[1][1] = 0;
        verts[2][0] = vid.width;  verts[2][1] = vid.height;
        verts[3][0] = 0;    verts[3][1] = vid.height;
	
        qglVertexPointer  ( 2, GL_FLOAT, 0, verts );
        qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

	qglColor4f(1, 1, 1, 1);
	qglEnable(GL_TEXTURE_2D);
	qglDisable(GL_BLEND);
}

void
Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, byte *data)
{
	unsigned image32[256 * 256];
	unsigned char image8[256 * 256];
	int i, j, trows;
	byte *source;
	int frac, fracstep;
	float hscale;
	int row;
	float t;

	R_Bind(0);

	if (rows <= 256)
	{
		hscale = 1;
		trows = rows;
	}
	else
	{
		hscale = rows / 256.0;
		trows = 256;
	}

	t = rows * hscale / 256 - 1.0 / 512.0;
	#if 0
	if (!qglColorTableEXT)
	{
	#endif
		unsigned *dest;

		for (i = 0; i < trows; i++)
		{
			row = (int)(i * hscale);

			if (row > rows)
			{
				break;
			}

			source = data + cols * row;
			dest = &image32[i * 256];
			fracstep = cols * 0x10000 / 256;
			frac = fracstep >> 1;

			for (j = 0; j < 256; j++)
			{
				dest[j] = r_rawpalette[source[frac >> 16]];
				frac += fracstep;
			}
		}

		qglTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format,
				256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE,
				image32);
	#if 0
	}
	else
	{
		unsigned char *dest;

		for (i = 0; i < trows; i++)
		{
			row = (int)(i * hscale);

			if (row > rows)
			{
				break;
			}

			source = data + cols * row;
			dest = &image8[i * 256];
			fracstep = cols * 0x10000 / 256;
			frac = fracstep >> 1;

			for (j = 0; j < 256; j++)
			{
				dest[j] = source[frac >> 16];
				frac += fracstep;
			}
		}

		qglTexImage2D(GL_TEXTURE_2D,
				0,
				GL_COLOR_INDEX8_EXT,
				256, 256,
				0,
				GL_COLOR_INDEX,
				GL_UNSIGNED_BYTE,
				image8);
	}
	#endif
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	float texcoords[4][2];
        float verts[4][2];

	verts[0][0] = x;    verts[0][1] = y;
        verts[1][0] = (x+w);  verts[1][1] = y;
        verts[2][0] = (x+w);  verts[2][1] = (y+h);
        verts[3][0] = x;    verts[3][1] = (y+h);
	
        texcoords[0][0] = 1.0 / 512.0;      texcoords[0][1] = 1.0 / 512.0;
        texcoords[1][0] = 511.0 / 512.0;   texcoords[1][1] = 1.0 / 512.0;
        texcoords[2][0] = 511.0 / 512.0;   texcoords[2][1] = t;
        texcoords[3][0] = 1.0 / 512.0;      texcoords[3][1] = t;
	
        qglEnableClientState( GL_TEXTURE_COORD_ARRAY );
        qglTexCoordPointer( 2, GL_FLOAT, 0, texcoords );
        qglVertexPointer  ( 2, GL_FLOAT, 0, verts );
        qglDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
        qglDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

int
Draw_GetPalette(void)
{
	int i;
	int r, g, b;
	unsigned v;
	byte *pic, *pal;
	int width, height;

	/* get the palette */
	LoadPCX("pics/colormap.pcx", &pic, &pal, &width, &height);

	if (!pal)
	{
		ri.Sys_Error(ERR_FATAL, "Couldn't load pics/colormap.pcx");
	}

	for (i = 0; i < 256; i++)
	{
		r = pal[i * 3 + 0];
		g = pal[i * 3 + 1];
		b = pal[i * 3 + 2];

		v = (255 << 24) + (r << 0) + (g << 8) + (b << 16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff); /* 255 is transparent */

	free(pic);
	free(pal);

	return 0;
}

