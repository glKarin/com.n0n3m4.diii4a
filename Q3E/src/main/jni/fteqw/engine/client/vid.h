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
// vid.h -- video driver defs

#define VID_CBITS	6
#define VID_GRADES	(1 << VID_CBITS)

// a pixel can be one, two, or four bytes
typedef qbyte pixel_t;

typedef enum
{
	QR_NONE,		//server-style operation (no rendering).
	QR_HEADLESS,	//no window/rendering at all (system tray only)
	QR_OPENGL,		//gl+gles+etc.
	QR_DIRECT3D9,	//
	QR_DIRECT3D11,	//
	QR_SOFTWARE,	//not worth using
	QR_VULKAN,		//
	QR_DIRECT3D12,	//no implementation
	QR_METAL,		//no implementation
	QR_DIRECT3D8	//liimted. available for win95 where d3d8.1+ does not. also only renderer supported on the original xbox, if that's ever relevant.
} r_qrenderer_t;

typedef struct {
	//you are not allowed to make anything not work if it's not based on these vars...
	int width;
	int height;
	int fullscreen;	//0 = windowed. 1 = fullscreen (mode changes). 2 = borderless+maximized
	qboolean stereo;
	int srgb;	//<0 = gamma-only. 0 = no srgb at all, >0 full srgb, including textures and stuff
	int bpp;	//16, 24(aka 32), 30, and 48 are meaningful
	int depthbits;
	int rate;
	int wait;	//-1 = default, 0 = off, 1 = on, 2 = every other
	int multisample;	//for opengl antialiasing (which requires context stuff)
	int triplebuffer;
	char subrenderer[MAX_QPATH];	//external driver
	char devicename[MAX_QPATH];		//device name (usually monitor)
	struct rendererinfo_s *renderer;
	struct plugvrfuncs_s *vr;		//the vr driver we're trying to use.
} rendererstate_t;
#ifndef SERVERONLY
extern rendererstate_t currentrendererstate;
#endif

typedef struct vrect_s
{
	float				x,y,width,height;
} vrect_t;
typedef struct
{
	int x;
	int y;
	int width;
	int height;
	int maxheight;	//vid.pixelheight or so
} pxrect_t;

//srgb colourspace displays smoother visual gradients, but its more of an illusion than anything else.
//we can be in a few different modes:
//linear - rendering is done using some float framebuffer or so. lighting is more correct. textures are converted from srgb colourspace when reading.
//non-linear - the lame option where lighting is wrong, but used anyway for compat. final colours will be whatever the screen uses (probably srgb, so artwork looks fine)
//srgb - like linear, except our framebuffer does extra transforms to hide that its storage is actually non-linear.
#define VID_SRGBAWARE			(1u<<0)	//we need to convert input srgb values to actual linear values (requires vid_reload to change...)
#define VID_SRGB_FB_LINEAR		(1u<<1) //framebuffer is linear (either the presentation engine is linear, or the blend unit is faking it)
#define VID_SRGB_FB_FAKED		(1u<<2) //renderer is faking it with a linear texture
#define VID_SRGB_CAPABLE		(1u<<3)	//we can toggle VID_SRGB_FB_LINEARISED on or off.
#define VID_FP16				(1u<<4)	//use 16bit currentrender etc to avoid banding
#define VID_SRGB_FB (VID_SRGB_FB_LINEAR|VID_SRGB_FB_FAKED)

typedef struct
{
	qboolean		activeapp;
	qboolean		isminimized;	//can omit rendering as it won't be seen anyway.
	int				fullbright;		// index of first fullbright color
	int				gammarampsize;		//typically 256. but can be up to 1024 (yay 10-bit hardware that's crippled to only actually use 8)

	unsigned		fbvwidth; /*virtual 2d width of the current framebuffer image*/
	unsigned		fbvheight; /*virtual 2d height*/
	unsigned		fbpwidth; /*physical 2d width of the current framebuffer image*/
	unsigned		fbpheight; /*physical 2d height*/
	struct image_s	*framebuffer; /*the framebuffer fbo (set by democapture)*/

	unsigned		width; /*virtual 2d screen width*/
	unsigned		height; /*virtual 2d screen height*/

	int				numpages;
	unsigned int	flags;	//VID_* flags

	unsigned		rotpixelwidth; /*width after rotation in pixels*/
	unsigned		rotpixelheight; /*pixel after rotation in pixels*/
	unsigned		pixelwidth; /*true height in pixels*/
	unsigned		pixelheight; /*true width in pixels*/

	float			dpi_x;
	float			dpi_y;

	conchar_t		*ime_preview;		//pending IME preview text that has been entered but isn't committed yet. set by video code.
	size_t			ime_previewlen;
	size_t			ime_caret;
	float			ime_position[2];	//where to display any ime popups (virtual coords)
	qboolean		ime_allow;			//enable the ime (ie: at the console or messagemode)

	qboolean		forcecursor;
	float			forcecursorpos[2];	//in physical pixels

	struct plugvrfuncs_s *vr;	//how do deal with VR contexts.
} viddef_t;

extern	viddef_t	vid;				// global video state

extern unsigned int	d_8to24rgbtable[256];
extern unsigned int	d_8to24srgbtable[256];
extern unsigned int	d_8to24bgrtable[256];
extern unsigned int	d_quaketo24srgbtable[256];
