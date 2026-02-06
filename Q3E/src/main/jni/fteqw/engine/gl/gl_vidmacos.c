/*
 
 Copyright (C) 2001-2002       A Nourai
 Copyright (C) 2006            Jacek Piszczek (Mac OSX port)
 
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
 
 See the included (GNU.txt) GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "quakedef.h"
#include "glquake.h"

#include <dlfcn.h>

//#include "vid_macos.h"

// note: cocoa code is separated in vid_cocoa.m because of compilation pbs

extern cvar_t vid_hardwaregamma;

static void *agllibrary;
static void *opengllibrary;
void *AGL_GetProcAddress(char *functionname)
{
	void *func;
	if (agllibrary)
	{
		func = dlsym(agllibrary, functionname);
		if (func)
			return func;
	}
	if (opengllibrary)
	{
		func = dlsym(opengllibrary, functionname);
		if (func)
			return func;
	}
	return NULL;
}

qboolean GLVID_Init(rendererstate_t *info, unsigned char *palette)
{
	int argnum;
	int i;

	agllibrary = dlopen("/System/Library/Frameworks/AGL.framework/AGL", RTLD_LAZY);
	if (!agllibrary)
	{
		Con_Printf("Couldn't load AGL framework\n");
		return false;
	}
	opengllibrary = dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL", RTLD_LAZY);
	//don't care if opengl failed.

	vid.numpages = 2;

	// initialise the NSApplication and the screen
	initCocoa(info);


	// calculate the conwidth AFTER the screen has been opened
	if (vid.pixelwidth <= 640)
	{
		vid.width = vid.pixelwidth;
		vid.height = vid.pixelheight;
	}
	else
	{
		vid.width = vid.pixelwidth/2;
		vid.height = vid.pixelheight/2;
	}

	if ((i = COM_CheckParm("-conwidth")) && i + 1 < com_argc)
	{
		vid.width = Q_atoi(com_argv[i + 1]);

		// pick a conheight that matches with correct aspect
		vid.height = vid.width * 3 / 4;
	}

	vid.width &= 0xfff8; // make it a multiple of eight

	if ((i = COM_CheckParm("-conheight")) && i + 1 < com_argc)
		vid.height = Q_atoi(com_argv[i + 1]);

	if (vid.width < 320)
		vid.width = 320;

	if (vid.height < 200)
		vid.height = 200;

	GL_Init(info, AGL_GetProcAddress);

	GLVID_SetPalette(palette);

	return true;
}

void GLVID_DeInit(void)
{
	killCocoa();
	GL_ForgetPointers();
}

void GLVID_SetPalette (unsigned char *palette)
{
	qbyte *pal;
	unsigned int r,g,b;
	int i;
	unsigned *table1;
	extern qbyte gammatable[256];

	Con_Printf("Converting 8to24\n");

	pal = palette;
	table1 = d_8to24rgbtable;

	if (vid_hardwaregamma.value)
	{
		for (i=0 ; i<256 ; i++)
		{
			r = pal[0];
			g = pal[1];
			b = pal[2];
			pal += 3;

			*table1++ = LittleLong((255<<24) + (r<<0) + (g<<8) + (b<<16));
		}
	}
	else
	{
		for (i=0 ; i<256 ; i++)
		{
			r = gammatable[pal[0]];
			g = gammatable[pal[1]];
			b = gammatable[pal[2]];
			pal += 3;

			*table1++ = LittleLong((255<<24) + (r<<0) + (g<<8) + (b<<16));
		}
	}
	d_8to24rgbtable[255] &= LittleLong(0xffffff);	// 255 is transparent
	Con_Printf("Converted\n");
}

void Sys_SendKeyEvents(void)
{
}

void GLVID_LockBuffer(void)
{
}

void GLVID_UnlockBuffer(void)
{
}

qboolean GLVID_IsLocked(void)
{
	return 0;
}

void GLVID_SetCaption(const char *text)
{
}

void GLVID_SwapBuffers(void)
{
	flushCocoa();
}

void GLVID_SetDeviceGammaRamp(unsigned short *ramps)
{
	cocoaGamma(ramps,ramps+256,ramps+512);
}

void GLVID_ShiftPalette(unsigned char *p)
{
	extern	unsigned short ramps[3][256];
	if (vid_hardwaregamma.value)
		GLVID_SetDeviceGammaRamp(ramps);
}

//I'm too lazy to put these stubs elsewhere.
void INS_Init (void)
{
}
void INS_ReInit(void)
{
}
void INS_Shutdown (void)
{
}
void INS_Commands (void)
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
}
void INS_Move (void)
{
}

#define SYS_CLIPBOARD_SIZE  256
static char clipboard_buffer[SYS_CLIPBOARD_SIZE] = {0};
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, char *utf8), void *ctx)
{
	callback(ctx, clipboard_buffer);
}
void Sys_SaveClipboard(clipboardtype_t cbt, char *text)
{
 	Q_strncpyz(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
}

