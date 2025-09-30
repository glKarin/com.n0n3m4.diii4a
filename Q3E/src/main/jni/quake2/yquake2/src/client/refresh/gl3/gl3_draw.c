/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016-2017 Daniel Gibson
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

unsigned d_8to24table[256];

gl3image_t *draw_chars;

static GLuint vbo2D = 0, vao2D = 0, vao2Dcolor = 0; // vao2D is for textured rendering, vao2Dcolor for color-only

void
GL3_Draw_InitLocal(void)
{
	/* load console characters */
	draw_chars = R_FindPic("conchars", (findimage_t)GL3_FindImage);
	if (!draw_chars)
	{
		ri.Sys_Error(ERR_FATAL, "%s: Couldn't load pics/conchars.pcx",
			__func__);
	}

	// set up attribute layout for 2D textured rendering
	glGenVertexArrays(1, &vao2D);
	glBindVertexArray(vao2D);

	glGenBuffers(1, &vbo2D);
	GL3_BindVBO(vbo2D);

	GL3_UseProgram(gl3state.si2D.shaderProgram);

	glEnableVertexAttribArray(GL3_ATTRIB_POSITION);
	// Note: the glVertexAttribPointer() configuration is stored in the VAO, not the shader or sth
	//       (that's why I use one VAO per 2D shader)
	qglVertexAttribPointer(GL3_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);

	glEnableVertexAttribArray(GL3_ATTRIB_TEXCOORD);
	qglVertexAttribPointer(GL3_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 2*sizeof(float));

	// set up attribute layout for 2D flat color rendering

	glGenVertexArrays(1, &vao2Dcolor);
	glBindVertexArray(vao2Dcolor);

	GL3_BindVBO(vbo2D); // yes, both VAOs share the same VBO

	GL3_UseProgram(gl3state.si2Dcolor.shaderProgram);

	glEnableVertexAttribArray(GL3_ATTRIB_POSITION);
	qglVertexAttribPointer(GL3_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);

	GL3_BindVAO(0);
}

void
GL3_Draw_ShutdownLocal(void)
{
	glDeleteBuffers(1, &vbo2D);
	vbo2D = 0;
	glDeleteVertexArrays(1, &vao2D);
	vao2D = 0;
	glDeleteVertexArrays(1, &vao2Dcolor);
	vao2Dcolor = 0;
}

// bind the texture before calling this
static void
drawTexturedRectangle(float x, float y, float w, float h,
                      float sl, float tl, float sh, float th)
{
	/*
	 *  x,y+h      x+w,y+h
	 * sl,th--------sh,th
	 *  |             |
	 *  |             |
	 *  |             |
	 * sl,tl--------sh,tl
	 *  x,y        x+w,y
	 */

	GLfloat vBuf[16] = {
	//  X,   Y,   S,  T
		x,   y+h, sl, th,
		x,   y,   sl, tl,
		x+w, y+h, sh, th,
		x+w, y,   sh, tl
	};

	GL3_BindVAO(vao2D);

	// Note: while vao2D "remembers" its vbo for drawing, binding the vao does *not*
	//       implicitly bind the vbo, so I need to explicitly bind it before glBufferData()
	GL3_BindVBO(vbo2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vBuf), vBuf, GL_STREAM_DRAW);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	//glMultiDrawArrays(mode, first, count, drawcount) ??
}

/*
 * Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void
GL3_Draw_CharScaled(int x, int y, int num, float scale)
{
	int row, col;
	float frow, fcol, size, scaledSize;
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

	scaledSize = 8*scale;

	// TODO: batchen?

	GL3_UseProgram(gl3state.si2D.shaderProgram);
	GL3_Bind(draw_chars->texnum);
	drawTexturedRectangle(x, y, scaledSize, scaledSize, fcol, frow, fcol+size, frow+size);
}

gl3image_t *
GL3_Draw_FindPic(const char *name)
{
	return R_FindPic(name, (findimage_t)GL3_FindImage);
}

void
GL3_Draw_GetPicSize(int *w, int *h, const char *pic)
{
	gl3image_t *gl;

	gl = R_FindPic(pic, (findimage_t)GL3_FindImage);

	if (!gl)
	{
		*w = *h = -1;
		return;
	}

	*w = gl->width;
	*h = gl->height;
}

void
GL3_Draw_StretchPic(int x, int y, int w, int h, const char *pic)
{
	gl3image_t *gl = R_FindPic(pic, (findimage_t)GL3_FindImage);

	if (!gl)
	{
		R_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL3_UseProgram(gl3state.si2D.shaderProgram);
	GL3_Bind(gl->texnum);

	drawTexturedRectangle(x, y, w, h, gl->sl, gl->tl, gl->sh, gl->th);
}

void
GL3_Draw_PicScaled(int x, int y, const char *pic, float factor)
{
	gl3image_t *gl = R_FindPic(pic, (findimage_t)GL3_FindImage);
	if (!gl)
	{
		R_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL3_UseProgram(gl3state.si2D.shaderProgram);
	GL3_Bind(gl->texnum);

	drawTexturedRectangle(x, y, gl->width*factor, gl->height*factor, gl->sl, gl->tl, gl->sh, gl->th);
}

/*
 * This repeats a 64*64 tile graphic to fill
 * the screen around a sized down
 * refresh window.
 */
void
GL3_Draw_TileClear(int x, int y, int w, int h, const char *pic)
{
	gl3image_t *image = R_FindPic(pic, (findimage_t)GL3_FindImage);
	if (!image)
	{
		R_Printf(PRINT_ALL, "Can't find pic: %s\n", pic);
		return;
	}

	GL3_UseProgram(gl3state.si2D.shaderProgram);
	GL3_Bind(image->texnum);

	drawTexturedRectangle(x, y, w, h, x/64.0f, y/64.0f, (x+w)/64.0f, (y+h)/64.0f);
}

void
GL3_DrawFrameBufferObject(int x, int y, int w, int h, GLuint fboTexture, const float v_blend[4])
{
	qboolean underwater = (gl3_newrefdef.rdflags & RDF_UNDERWATER) != 0;
	gl3ShaderInfo_t* shader = underwater ? &gl3state.si2DpostProcessWater
	                                     : &gl3state.si2DpostProcess;
	GL3_UseProgram(shader->shaderProgram);
	GL3_Bind(fboTexture);

	if(underwater && shader->uniLmScalesOrTime != -1)
	{
		glUniform1f(shader->uniLmScalesOrTime, gl3_newrefdef.time);
	}
	if(shader->uniVblend != -1)
	{
		glUniform4fv(shader->uniVblend, 1, v_blend);
	}

	drawTexturedRectangle(x, y, w, h, 0, 1, 1, 0);
}

/*
 * Fills a box of pixels with a single color
 */
void
GL3_Draw_Fill(int x, int y, int w, int h, int c)
{
	union
	{
		unsigned c;
		byte v[4];
	} color;
	int i;

	if ((unsigned)c > 255)
	{
		ri.Sys_Error(ERR_FATAL, "Draw_Fill: bad color");
	}

	color.c = d_8to24table[c];

	GLfloat vBuf[8] = {
	//  X,   Y
		x,   y+h,
		x,   y,
		x+w, y+h,
		x+w, y
	};

	for(i=0; i<3; ++i)
	{
		gl3state.uniCommonData.color.Elements[i] = color.v[i] * (1.0f/255.0f);
	}
	gl3state.uniCommonData.color.A = 1.0f;

	GL3_UpdateUBOCommon();

	GL3_UseProgram(gl3state.si2Dcolor.shaderProgram);
	GL3_BindVAO(vao2Dcolor);

	GL3_BindVBO(vbo2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vBuf), vBuf, GL_STREAM_DRAW);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// in GL1 this is called R_Flash() (which just calls R_PolyBlend())
// now implemented in 2D mode and called after SetGL2D() because
// it's pretty similar to GL3_Draw_FadeScreen()
void
GL3_Draw_Flash(const float color[4], float x, float y, float w, float h)
{
	if (gl_polyblend->value == 0)
	{
		return;
	}

	int i=0;

	GLfloat vBuf[8] = {
	//  X,   Y
		x,   y+h,
		x,   y,
		x+w, y+h,
		x+w, y
	};

	glEnable(GL_BLEND);

	for(i=0; i<4; ++i)  gl3state.uniCommonData.color.Elements[i] = color[i];

	GL3_UpdateUBOCommon();

	GL3_UseProgram(gl3state.si2Dcolor.shaderProgram);

	GL3_BindVAO(vao2Dcolor);

	GL3_BindVBO(vbo2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vBuf), vBuf, GL_STREAM_DRAW);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_BLEND);
}

void
GL3_Draw_FadeScreen(void)
{
	float color[4] = {0, 0, 0, 0.6f};
	GL3_Draw_Flash(color, 0, 0, vid.width, vid.height);
}

void
GL3_Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int bits)
{
	int i, j;

	GL3_Bind(0);

	unsigned image32[320*240]; /* was 256 * 256, but we want a bit more space */

	unsigned* img = image32;

	if (bits == 32)
	{
		img = (unsigned *)data;
	}
	else
	{
		if(cols*rows > 320*240)
		{
			/* in case there is a bigger video after all,
			 * malloc enough space to hold the frame */
			img = (unsigned*)malloc(cols*rows*4);
		}

		for(i=0; i<rows; ++i)
		{
			int rowOffset = i*cols;
			for(j=0; j<cols; ++j)
			{
				byte palIdx = data[rowOffset+j];
				img[rowOffset+j] = gl3_rawpalette[palIdx];
			}
		}
	}

	GL3_UseProgram(gl3state.si2D.shaderProgram);

	GLuint glTex;
	glGenTextures(1, &glTex);
	GL3_SelectTMU(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glTex);

	glTexImage2D(GL_TEXTURE_2D, 0, gl3_tex_solid_format,
	             cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);

	if(img != image32 && img != (unsigned *)data)
	{
		free(img);
	}

	// Note: gl_filter_min could be GL_*_MIPMAP_* so we can't use it for min filter here (=> no mipmaps)
	//       but gl_filter_max (either GL_LINEAR or GL_NEAREST) should do the trick.
	GLint filter = (r_videos_unfiltered->value == 0) ? gl_filter_max : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	drawTexturedRectangle(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);

	glDeleteTextures(1, &glTex);

	GL3_Bind(0);
}
