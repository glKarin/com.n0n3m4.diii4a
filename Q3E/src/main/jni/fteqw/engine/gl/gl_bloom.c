/*
Copyright (C) 1997-2001 Id Software, Inc.

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
// gl_bloom.c: 2D lighting post process effect


/*
info about bloom algo:
bloom is basically smudging.
screen is nearest-downsampled to some usable scale and filtered to remove low-value light (this is what stops non-bright stuff from blooming)
this filtered image is then downsized multiple times
the downsized image is then blured 
the downsized images are then blured horizontally, and then vertically.
final pass simply adds each blured level to the original image.
all samples are then added together for final rendering (with some kind of tone mapping if you want proper hdr).

note: the horizontal/vertical bluring is a guassian filter
note: bloom comes from the fact that the most downsampled image doesn't have too many pixels. the pixels that it does have are spread over a large area.

http://prideout.net/archive/bloom/ contains some sample code
*/


//http://www.quakesrc.org/forums/viewtopic.php?t=4340&start=0

#include "quakedef.h"

#if defined(GLQUAKE) || defined(VKQUAKE)
#include "shader.h"
#include "glquake.h"
#include "gl_draw.h"

cvar_t		r_bloom = CVARAFD("r_bloom", "0", "gl_bloom", CVAR_ARCHIVE, "Enables bloom (light bleeding from bright objects). Fractional values reduce the amount shown.");
cvar_t		r_bloom_retain = CVARD("r_bloom_retain", "1", "How much of the regular scene to retain when bloom is active.");
cvar_t		r_bloom_filter = CVARD("r_bloom_filter", "0.7 0.7 0.7", "Controls how bright the image must get before it will bloom (3 separate values, in RGB order).");
cvar_t		r_bloom_size = CVARD("r_bloom_size", "4", "Target bloom kernel size (assuming a video width of 320).");
cvar_t		r_bloom_downsize = CVARD("r_bloom_downsize", "0", "Technically more correct with a value of 1, but you probably won't notice.");
cvar_t		r_bloom_initialscale = CVARD("r_bloom_initialscale", "1", "Initial scaling factor for bloom. should be either 1 or 0.5");

static shader_t *bloomfilter;
static shader_t *bloomrescale;
static shader_t *bloomblur;
static shader_t *bloomfinal;

#define MAXLEVELS 16
texid_t pingtex[2][MAXLEVELS];
fbostate_t fbo_bloom;
static int scrwidth, scrheight;
static int texwidth[MAXLEVELS], texheight[MAXLEVELS];


static void R_InitBloomTextures(void)
{
	int i;

	bloomfilter = NULL;
	bloomblur = NULL;
	bloomfinal = NULL;
	scrwidth = 0, scrheight = 0;

	for (i = 0; i < MAXLEVELS; i++)
	{
		pingtex[0][i] = r_nulltex;
		pingtex[1][i] = r_nulltex;
	}
}
void R_BloomRegister(void)
{
	Cvar_Register (&r_bloom, "bloom");
	Cvar_Register (&r_bloom_retain, "bloom");
	Cvar_Register (&r_bloom_filter, "bloom");
	Cvar_Register (&r_bloom_size, "bloom");
	Cvar_Register (&r_bloom_downsize, "bloom");
	Cvar_Register (&r_bloom_initialscale, "bloom");
}
static void R_SetupBloomTextures(int w, int h)
{
	int i, j;
	if (w == scrwidth && h == scrheight && !r_bloom_initialscale.modified)
		return;
	r_bloom_initialscale.modified = false;
	scrwidth = w;
	scrheight = h;
	//I'm depending on npot here
	w *= r_bloom_initialscale.value;
	h *= r_bloom_initialscale.value;
	for (i = 0; i < MAXLEVELS; i++)
	{
		/*I'm paranoid*/
		if (w < 4)
			w = 4;
		if (h < 4)
			h = 4;

		texwidth[i] = w;
		texheight[i] = h;

		w /= 2;
		h /= 2;
	}

	/*destroy textures for each level, to ensure they're created fresh as needed*/
	for (j = 0; j < MAXLEVELS; j++)
	{
		for (i = 0; i < 2; i++)
		{
			if (TEXVALID(pingtex[i][j]))
				Image_UnloadTexture(pingtex[i][j]);
			pingtex[i][j] = r_nulltex;
		}
	}


	bloomfilter = R_RegisterShader("bloom_filter", SUF_NONE,
		"{\n"
			"cull none\n"
			"program bloom_filter\n"
			"{\n"
				"map $sourcecolour\n"
			"}\n"
		"}\n");
	bloomrescale = R_RegisterShader("bloom_rescale", SUF_NONE,
		"{\n"
			"cull none\n"
			"program default2d\n"
			"{\n"
				"map $sourcecolour\n"
			"}\n"
		"}\n");
	bloomblur = R_RegisterShader("bloom_blur", SUF_NONE,
		"{\n"
			"cull none\n"
			"program bloom_blur\n"
			"{\n"
				"map $sourcecolour\n"
			"}\n"
		"}\n");
	bloomfinal = R_RegisterShader("bloom_final", SUF_NONE,
		"{\n"
			"cull none\n"
			"program bloom_final\n"
			"{\n"
				"map $sourcecolour\n"
			"}\n"
			"{\n"
				"map $diffuse\n"
			"}\n"
			"{\n"
				"map $loweroverlay\n"
			"}\n"
			"{\n"
				"map $upperoverlay\n"
			"}\n"
		"}\n");
}
qboolean R_CanBloom(void)
{
	if (!r_bloom.value)
		return false;
	switch(qrenderer)
	{
#ifdef GLQUAKE
	case QR_OPENGL:
		if (!gl_config.ext_framebuffer_objects)
			return false;
		if (!gl_config.arb_shader_objects)
			return false;
		if (!sh_config.texture_non_power_of_two_pic)
			return false;
		break;
#endif
#ifdef VKQUAKE
	case QR_VULKAN:
		break;
#endif
	default:
		return false;
	}

	return true;
}

#ifdef VKQUAKE
#include "../vk/vkrenderer.h"
struct vk_rendertarg vk_rt_bloom[2][MAXLEVELS], vk_rt_filter;
void VK_R_BloomBlend (texid_t source, int x, int y, int w, int h)
{
	int i;
//	struct vk_rendertarg *oldfbo = vk.rendertarg;
	texid_t intex;
	int pixels = 1;
	int targetpixels = r_bloom_size.value * vid.pixelwidth / 320;

	targetpixels *= r_bloom_initialscale.value;

	/*whu?*/
	if (!w || !h)
		return;

	/*update textures if we need to resize them*/
	R_SetupBloomTextures(w, h);

	if (R2D_Flush)
		R2D_Flush();
#if 1
	/*filter the screen into a downscaled image*/
	VKBE_RT_Gen(&vk_rt_filter, NULL, texwidth[0], texheight[0], false, RT_IMAGEFLAGS);
	VKBE_RT_Begin(&vk_rt_filter);
	vk.sourcecolour = source;
	R2D_ScalePic(0, 0, vid.width, vid.height, bloomfilter);
	VKBE_RT_End(&vk_rt_filter);
	intex = &vk_rt_filter.q_colour;
#else
	intex = source;
#endif

	for (pixels = 1, i = 0; pixels < targetpixels && i < MAXLEVELS; i++, pixels <<= 1)
	{
		//downsize the blur, for added accuracy
		/*if (i > 0 && r_bloom_downsize.ival)
		{
			//simple downscale that multiple times
			VKBE_RT_Gen(&vk_rt_bloom[0][i], texwidth[i], texheight[i], false);
			VKBE_RT_Begin(&vk_rt_bloom[0][i]);
			vk.sourcecolour = source;

			R2D_ScalePic(0, vid.height, vid.width, -(int)vid.height, bloomrescale);
			if (R2D_Flush)
				R2D_Flush();
			intex = &vk_rt_bloom[0][i];
			r_worldentity.glowmod[0] = 1.0 / intex->width;
		}
		else*/
			r_worldentity.glowmod[0] = 2.0 / intex->width;

		r_worldentity.glowmod[1] = 0;

		VKBE_RT_Gen(&vk_rt_bloom[1][i], NULL, texwidth[i], texheight[i], false, RT_IMAGEFLAGS);
		VKBE_RT_Begin(&vk_rt_bloom[1][i]);
		vk.sourcecolour = intex;
		BE_SelectEntity(&r_worldentity);
		R2D_ScalePic(0, 0, vid.width, vid.height, bloomblur);
		VKBE_RT_End(&vk_rt_bloom[1][i]);

		r_worldentity.glowmod[0] = 0;
		r_worldentity.glowmod[1] = 1.0 / texheight[i];

		VKBE_RT_Gen(&vk_rt_bloom[0][i], NULL, texwidth[i], texheight[i], false, RT_IMAGEFLAGS);
		VKBE_RT_Begin(&vk_rt_bloom[0][i]);
		vk.sourcecolour = &vk_rt_bloom[1][i].q_colour;
		BE_SelectEntity(&r_worldentity);
		R2D_ScalePic(0, 0, vid.width, vid.height, bloomblur);
		VKBE_RT_End(&vk_rt_bloom[0][i]);

		intex = &vk_rt_bloom[0][i].q_colour;
	}
	r_worldentity.glowmod[0] = 0;
	r_worldentity.glowmod[1] = 0;

	/*combine them onto the screen*/
	bloomfinal->defaulttextures->base			= intex;
	bloomfinal->defaulttextures->loweroverlay	= (i >= 2)?&vk_rt_bloom[0][i-2].q_colour:0;
	bloomfinal->defaulttextures->upperoverlay	= (i >= 3)?&vk_rt_bloom[0][i-3].q_colour:0;
	vk.sourcecolour = source;
	R2D_ScalePic(x, y, w, h, bloomfinal);
	R2D_Flush();
}
void VK_R_BloomShutdown(void)
{
	int i;
	for (i = 0; i < MAXLEVELS; i++)
	{
		VKBE_RT_Gen(&vk_rt_bloom[0][i], NULL, 0, 0, false, RT_IMAGEFLAGS);
		VKBE_RT_Gen(&vk_rt_bloom[1][i], NULL, 0, 0, false, RT_IMAGEFLAGS);
	}
	VKBE_RT_Gen(&vk_rt_filter, NULL, 0, 0, false, RT_IMAGEFLAGS);

	R_InitBloomTextures();
}
#endif
#ifdef GLQUAKE
void R_BloomBlend (texid_t source, int x, int y, int w, int h)
{
	int i;
	int oldfbo = 0;
	texid_t intex;
	int pixels = 1;
	int targetpixels = r_bloom_size.value * vid.pixelwidth / 320;
	char name[64];

	targetpixels *= r_bloom_initialscale.value;

	/*whu?*/
	if (!w || !h)
		return;

	/*update textures if we need to resize them*/
	R_SetupBloomTextures(r_refdef.pxrect.width, r_refdef.pxrect.height);

	/*filter the screen into a downscaled image*/
	if (!TEXVALID(pingtex[0][0]))
	{
		sprintf(name, "***bloom*%c*%i***", 'a'+0, 0);
		TEXASSIGN(pingtex[0][0], Image_CreateTexture(name, NULL, IF_CLAMP|IF_NOMIPMAP|IF_NOPICMIP|IF_LINEAR|IF_NOPURGE));
		Image_Upload(pingtex[0][0], PTI_RGBA8, NULL, NULL, texwidth[0], texheight[0], 1, IF_CLAMP|IF_NOMIPMAP|IF_NOPICMIP|IF_LINEAR|IF_NOSRGB);
	}

	if (R2D_Flush)
		R2D_Flush();

	oldfbo = GLBE_FBO_Update(&fbo_bloom, 0, &pingtex[0][0], 1, r_nulltex, 0, 0, 0);
	GLBE_FBO_Sources(source, r_nulltex);
	qglViewport (0, 0, texwidth[0], texheight[0]);
	R2D_ScalePic(0, vid.height, vid.width, -(int)vid.height, bloomfilter);

	if (R2D_Flush)
		R2D_Flush();

	intex = pingtex[0][0];

	for (pixels = 1, i = 0; pixels < targetpixels && i < MAXLEVELS; i++, pixels <<= 1)
	{
		/*create any textures if they're not valid yet*/
		if (!TEXVALID(pingtex[0][i]))
		{
			sprintf(name, "***bloom*%c*%i***", 'a'+0, i);
			TEXASSIGN(pingtex[0][i], Image_CreateTexture(name, NULL, IF_CLAMP|IF_NOMIPMAP|IF_NOPICMIP|IF_LINEAR|IF_NOPURGE));
			Image_Upload(pingtex[0][i], PTI_RGBA8, NULL, NULL, texwidth[i], texheight[i], 1, IF_CLAMP|IF_NOMIPMAP|IF_NOPICMIP|IF_LINEAR|IF_NOSRGB);
		}
		if (!TEXVALID(pingtex[1][i]))
		{
			sprintf(name, "***bloom*%c*%i***", 'a'+1, i);
			TEXASSIGN(pingtex[1][i], Image_CreateTexture(name, NULL, IF_CLAMP|IF_NOMIPMAP|IF_NOPICMIP|IF_LINEAR|IF_NOPURGE));
			Image_Upload(pingtex[1][i], PTI_RGBA8, NULL, NULL, texwidth[i], texheight[i], 1, IF_CLAMP|IF_NOMIPMAP|IF_NOPICMIP|IF_LINEAR|IF_NOSRGB);
		}

		if (R2D_Flush)
			R2D_Flush();
		//downsize the blur, for added accuracy
		if (i > 0 && r_bloom_downsize.ival)
		{
			/*simple downscale that multiple times*/
			GLBE_FBO_Update(&fbo_bloom, 0, &pingtex[0][i], 1, r_nulltex, 0, 0, 0);
			GLBE_FBO_Sources(pingtex[0][i-1], r_nulltex);
			qglViewport (0, 0, texwidth[i], texheight[i]);
			R2D_ScalePic(0, vid.height, vid.width, -(int)vid.height, bloomrescale);
			if (R2D_Flush)
				R2D_Flush();
			intex = pingtex[0][i];
			r_worldentity.glowmod[0] = 1.0 / intex->width;
		}
		else
			r_worldentity.glowmod[0] = 2.0 / intex->width;

		r_worldentity.glowmod[1] = 0;
		GLBE_FBO_Update(&fbo_bloom, 0, &pingtex[1][i], 1, r_nulltex, 0, 0, 0);
		GLBE_FBO_Sources(intex, r_nulltex);
		qglViewport (0, 0, pingtex[1][i]->width, pingtex[1][i]->height);
		BE_SelectEntity(&r_worldentity);
		R2D_ScalePic(0, vid.height, vid.width, -(int)vid.height, bloomblur);
		if (R2D_Flush)
			R2D_Flush();

		r_worldentity.glowmod[0] = 0;
		r_worldentity.glowmod[1] = 1.0 / pingtex[1][i]->height;
		GLBE_FBO_Update(&fbo_bloom, 0, &pingtex[0][i], 1, r_nulltex, 0, 0, 0);
		GLBE_FBO_Sources(pingtex[1][i], r_nulltex);
		qglViewport (0, 0, pingtex[0][i]->width, pingtex[0][i]->height);
		BE_SelectEntity(&r_worldentity);
		R2D_ScalePic(0, vid.height, vid.width, -(int)vid.height, bloomblur);

		intex = pingtex[0][i];
	}

	if (R2D_Flush)
		R2D_Flush();

	r_worldentity.glowmod[0] = 0;
	r_worldentity.glowmod[1] = 0;

	bloomfinal->defaulttextures->base			= intex;
	bloomfinal->defaulttextures->loweroverlay	= (i >= 2)?pingtex[0][i-2]:0;
	bloomfinal->defaulttextures->upperoverlay	= (i >= 3)?pingtex[0][i-3]:0;

	/*combine them onto the screen*/
	GLBE_FBO_Pop(oldfbo);
	GLBE_FBO_Sources(source, r_nulltex);
	GL_Set2D(false);
	R2D_ScalePic(x, y + h, w, -h, bloomfinal);
}
void R_BloomShutdown(void)
{
	GLBE_FBO_Destroy(&fbo_bloom);

	R_InitBloomTextures();
}
#endif

#endif
