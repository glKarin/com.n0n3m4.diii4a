/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2018-2019 Krzysztof Kondrak
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
 */

#include "header/local.h"

static image_t	*draw_chars;

/*
===============
Draw_InitLocal
===============
*/
void Draw_InitLocal (void)
{
	draw_chars = R_FindPic ("conchars", (findimage_t)Vk_FindImage);

	/* Anachronox */
	if (!draw_chars)
	{
		draw_chars = R_FindPic ("fonts/conchars", (findimage_t)Vk_FindImage);
	}

	/* Daikatana */
	if (!draw_chars)
	{
		draw_chars = R_FindPic ("dkchars", (findimage_t)Vk_FindImage);
	}

	if (!draw_chars)
	{
		Com_Error(ERR_FATAL, "%s: Couldn't load pics/conchars",
			__func__);
	}
}



/*
================
RE_Draw_CharScaled

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void
RE_Draw_CharScaled(int x, int y, int num, float scale)
{
	int	row, col;
	float	frow, fcol, size;

	if (!vk_frameStarted)
		return;

	num &= 255;

	if ((num & 127) == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	float imgTransform[] = { (float)x / vid.width, (float)y / vid.height,
							 8.f * scale / vid.width, 8.f * scale / vid.height,
							 fcol, frow, size, size };
	QVk_DrawTexRect(imgTransform, sizeof(imgTransform), &draw_chars->vk_texture);
}

/*
=============
RE_Draw_FindPic
=============
*/
image_t	*RE_Draw_FindPic(const char *name)
{
	return R_FindPic(name, (findimage_t)Vk_FindImage);
}

/*
=============
RE_Draw_GetPicSize
=============
*/
void
RE_Draw_GetPicSize(int *w, int *h, const char *name)
{
	image_t *image;

	image = R_FindPic(name, (findimage_t)Vk_FindImage);
	if (!image)
	{
		*w = *h = -1;
		return;
	}

	*w = image->width;
	*h = image->height;
}

/*
=============
RE_Draw_StretchPic
=============
*/
void
RE_Draw_StretchPic (int x, int y, int w, int h, const char *name)
{
	image_t *vk;

	if (!vk_frameStarted)
		return;

	vk = R_FindPic(name, (findimage_t)Vk_FindImage);
	if (!vk)
	{
		R_Printf(PRINT_ALL, "%s(): Can't find pic: %s\n", __func__, name);
		return;
	}

	float imgTransform[] = { (float)x / vid.width, (float)y / vid.height,
							 (float)w / vid.width, (float)h / vid.height,
							  0, 0, 1, 1 };
	QVk_DrawTexRect(imgTransform, sizeof(imgTransform), &vk->vk_texture);
}


/*
=============
RE_Draw_PicScaled
=============
*/
void
RE_Draw_PicScaled(int x, int y, const char *name, float scale)
{
	image_t *vk;

	vk = R_FindPic(name, (findimage_t)Vk_FindImage);
	if (!vk)
	{
		R_Printf(PRINT_ALL, "%s(): Can't find pic: %s\n", __func__, name);
		return;
	}

	RE_Draw_StretchPic(x, y, vk->width*scale, vk->height*scale, name);
}

/*
=============
RE_Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void
RE_Draw_TileClear(int x, int y, int w, int h, const char *name)
{
	image_t	*image;

	if (!vk_frameStarted)
		return;

	image = R_FindPic(name, (findimage_t)Vk_FindImage);
	if (!image)
	{
		R_Printf(PRINT_ALL, "%s(): Can't find pic: %s\n", __func__, name);
		return;
	}

	// Change viewport and scissor to draw in the top left corner as the world view.
	VkViewport tileViewport = vk_viewport;
	VkRect2D tileScissor = vk_scissor;

	tileViewport.x = 0.f;
	tileViewport.y = 0.f;
	tileScissor.offset.x = 0;
	tileScissor.offset.y = 0;

	vkCmdSetViewport(vk_activeCmdbuffer, 0u, 1u, &tileViewport);
	vkCmdSetScissor(vk_activeCmdbuffer, 0u, 1u, &tileScissor);

	const float divisor = (vk_pixel_size->value < 1.0f ? 1.0f : vk_pixel_size->value);
	float imgTransform[] = { (float)x / (vid.width * divisor),	(float)y / (vid.height * divisor),
							 (float)w / (vid.width * divisor),	(float)h / (vid.height * divisor),
							 (float)x / (64.0 * divisor),		(float)y / (64.0 * divisor),
							 (float)w / (64.0 * divisor),		(float)h / (64.0 * divisor) };
	QVk_DrawTexRect(imgTransform, sizeof(imgTransform), &image->vk_texture);

	// Restore viewport and scissor.
	vkCmdSetViewport(vk_activeCmdbuffer, 0u, 1u, &vk_viewport);
	vkCmdSetScissor(vk_activeCmdbuffer, 0u, 1u, &vk_scissor);
}


/*
=============
RE_Draw_Fill

Fills a box of pixels with a single color
=============
*/
void RE_Draw_Fill (int x, int y, int w, int h, int c)
{
	union
	{
		unsigned	c;
		byte		v[4];
	} color;

	if (!vk_frameStarted)
		return;

	if ((unsigned)c > 255)
		Com_Error(ERR_FATAL, "%s: bad color", __func__);

	color.c = d_8to24table[c];

	float imgTransform[] = { (float)x / vid.width, (float)y / vid.height,
							 (float)w / vid.width, (float)h / vid.height,
							 color.v[0] / 255.f, color.v[1] / 255.f, color.v[2] / 255.f, 1.f };
	QVk_DrawColorRect(imgTransform, sizeof(imgTransform), RP_UI);
}

//=============================================================================

/*
================
RE_Draw_FadeScreen

================
*/
void RE_Draw_FadeScreen (void)
{
	float imgTransform[] = { 0.f, 0.f, vid.width, vid.height, 0.f, 0.f, 0.f, .8f };

	if (!vk_frameStarted)
		return;

	QVk_DrawColorRect(imgTransform, sizeof(imgTransform), RP_UI);
}


//====================================================================


/*
=============
RE_Draw_StretchRaw
=============
*/
static int vk_rawTexture_height;
static int vk_rawTexture_width;

void RE_Draw_StretchRaw (int x, int y, int w, int h, int cols, int rows, const byte *data, int bits)
{

	int	i, j;
	unsigned *dest;
	byte *source;
	byte *image_scaled = NULL;
	unsigned *raw_image32;

	if (!vk_frameStarted)
		return;

	if (bits == 32)
	{
		raw_image32 = malloc(cols * rows * sizeof(unsigned));
		if (!raw_image32)
		{
			return;
		}

		memcpy(raw_image32, data, cols * rows * sizeof(unsigned));
	}
	else
	{
		if (r_retexturing->value)
		{
			// triple scaling
			if (cols < (vid.width / 3) || rows < (vid.height / 3))
			{
				image_scaled = malloc(cols * rows * 9);

				scale3x(data, image_scaled, cols, rows);

				cols = cols * 3;
				rows = rows * 3;
			}
			else
			// double scaling
			{
				image_scaled = malloc(cols * rows * 4);

				scale2x(data, image_scaled, cols, rows);

				cols = cols * 2;
				rows = rows * 2;
			}
		}
		else
		{
			image_scaled = (byte *)data;
		}

		raw_image32 = malloc(cols * rows * sizeof(unsigned));

		source = image_scaled;
		dest = raw_image32;
		for (i = 0; i < rows; ++i)
		{
			int rowOffset = i * cols;
			for (j = 0; j < cols; ++j)
			{
				byte palIdx = source[rowOffset + j];
				dest[rowOffset + j] = r_rawpalette[palIdx];
			}
		}

		if (r_retexturing->value)
		{
			int scaled_size = cols * rows;

			free(image_scaled);
			SmoothColorImage(raw_image32, scaled_size, (scaled_size) >> 7);
		}
	}

	if (vk_rawTexture.resource.image != VK_NULL_HANDLE &&
	    (vk_rawTexture_width != cols || vk_rawTexture_height != rows))
	{
		QVk_ReleaseTexture(&vk_rawTexture);
		QVVKTEXTURE_CLEAR(vk_rawTexture);
	}

	if (vk_rawTexture.resource.image != VK_NULL_HANDLE)
	{
		QVk_UpdateTextureData(&vk_rawTexture, (unsigned char*)raw_image32, 0, 0, cols, rows);
	}
	else
	{
		vk_rawTexture_width = cols;
		vk_rawTexture_height = rows;

		QVVKTEXTURE_CLEAR(vk_rawTexture);
		QVk_CreateTexture(&vk_rawTexture, (unsigned char*)raw_image32, cols, rows,
			(r_videos_unfiltered->value == 0) ? vk_current_sampler : S_NEAREST,
			false);
		QVk_DebugSetObjectName((uint64_t)vk_rawTexture.resource.image,
			VK_OBJECT_TYPE_IMAGE, "Image: raw texture");
		QVk_DebugSetObjectName((uint64_t)vk_rawTexture.imageView,
			VK_OBJECT_TYPE_IMAGE_VIEW, "Image View: raw texture");
		QVk_DebugSetObjectName((uint64_t)vk_rawTexture.descriptorSet,
			VK_OBJECT_TYPE_DESCRIPTOR_SET, "Descriptor Set: raw texture");
		QVk_DebugSetObjectName((uint64_t)vk_rawTexture.resource.memory,
			VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: raw texture");
	}

	free(raw_image32);

	float imgTransform[] = { (float)x / vid.width, (float)y / vid.height,
							 (float)w / vid.width, (float)h / vid.height,
							 0.f, 0.f, 1.f, 1.0f };
	QVk_DrawTexRect(imgTransform, sizeof(imgTransform), &vk_rawTexture);
}
