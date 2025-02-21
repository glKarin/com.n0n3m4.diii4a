/*
Copyright (C) 2006-2007 Mark Olsen

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

#include <exec/exec.h>
#define SYSTEM_PRIVATE
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/extensions.h>
#include <cybergraphx/cybergraphics.h>

#include <tgl/gl.h>
#include <tgl/gla.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/cybergraphics.h>
#include <proto/tinygl.h>

#include "quakedef.h"
#include "glquake.h"

#include "gl_vidtinyglstubs.c"

#ifndef SA_GammaControl
#define SA_GammaControl (SA_Dummy + 123)
#endif

#ifndef SA_3DSupport
#define SA_3DSupport (SA_Dummy + 127)
#endif

#ifndef SA_GammaRed
#define SA_GammaRed (SA_Dummy + 124)
#endif

#ifndef SA_GammaBlue
#define SA_GammaBlue (SA_Dummy + 125)
#endif

#ifndef SA_GammaGreen
#define SA_GammaGreen (SA_Dummy + 126)
#endif

#define WARP_WIDTH              320
#define WARP_HEIGHT             200

extern qboolean gammaworks;
extern unsigned short ramps[3][256];

struct Library *TinyGLBase = 0;
GLContext *__tglContext;
static int glctx = 0;

unsigned char *mosgammatable;

struct Window *window;
struct Screen *screen;

static void *pointermem;

static void *TinyGL_GetSymbol(char *name)
{
	void *ret = 0;

	if (strcmp(name, "glAlphaFunc") == 0)
		ret = stub_glAlphaFunc;

	else if (strcmp(name, "glBegin") == 0)
		ret = stub_glBegin;

	else if (strcmp(name, "glBlendFunc") == 0)
		ret = stub_glBlendFunc;

	else if (strcmp(name, "glBindTexture") == 0)
		ret = stub_glBindTexture;

	else if (strcmp(name, "glClear") == 0)
		ret = stub_glClear;

	else if (strcmp(name, "glClearColor") == 0)
		ret = stub_glClearColor;

	else if (strcmp(name, "glClearDepth") == 0)
		ret = stub_glClearDepth;

	else if (strcmp(name, "glClearStencil") == 0)
		ret = stub_glClearStencil;

	else if (strcmp(name, "glColor3f") == 0)
		ret = stub_glColor3f;

	else if (strcmp(name, "glColor3ub") == 0)
		ret = stub_glColor3ub;

	else if (strcmp(name, "glColor4f") == 0)
		ret = stub_glColor4f;

	else if (strcmp(name, "glColor4fv") == 0)
		ret = stub_glColor4fv;

	else if (strcmp(name, "glColor4ub") == 0)
		ret = stub_glColor4ub;

	else if (strcmp(name, "glColor4ubv") == 0)
		ret = stub_glColor4ubv;

	else if (strcmp(name, "glColorMask") == 0)
		ret = stub_glColorMask;

	else if (strcmp(name, "glCopyTexImage2D") == 0)
		ret = stub_glCopyTexImage2D;

	else if (strcmp(name, "glCopyTexSubImage2D") == 0)
		ret = stub_glCopyTexSubImage2D;

	else if (strcmp(name, "glCullFace") == 0)
		ret = stub_glCullFace;

	else if (strcmp(name, "glDepthFunc") == 0)
		ret = stub_glDepthFunc;

	else if (strcmp(name, "glDepthMask") == 0)
		ret = stub_glDepthMask;

	else if (strcmp(name, "glDepthRange") == 0)
		ret = stub_glDepthRange;

	else if (strcmp(name, "glDisable") == 0)
		ret = stub_glDisable;

	else if (strcmp(name, "glDrawBuffer") == 0)
		ret = stub_glDrawBuffer;

	else if (strcmp(name, "glDrawPixels") == 0)
		ret = stub_glDrawPixels;

	else if (strcmp(name, "glEnable") == 0)
		ret = stub_glEnable;

	else if (strcmp(name, "glEnd") == 0)
		ret = stub_glEnd;

	else if (strcmp(name, "glFinish") == 0)
		ret = stub_glFinish;

	else if (strcmp(name, "glFlush") == 0)
		ret = stub_glFlush;

	else if (strcmp(name, "glFrustum") == 0)
		ret = stub_glFrustum;

	else if (strcmp(name, "glGetFloatv") == 0)
		ret = stub_glGetFloatv;

	else if (strcmp(name, "glGetIntegerv") == 0)
		ret = stub_glGetIntegerv;

	else if (strcmp(name, "glGetString") == 0)
		ret = stub_glGetString;

	else if (strcmp(name, "glGetTexLevelParameteriv") == 0)
		ret = stub_glGetTexLevelParameteriv;

	else if (strcmp(name, "glHint") == 0)
		ret = stub_glHint;

	else if (strcmp(name, "glLoadIdentity") == 0)
		ret = stub_glLoadIdentity;

	else if (strcmp(name, "glLoadMatrixf") == 0)
		ret = stub_glLoadMatrixf;

	else if (strcmp(name, "glNormal3f") == 0)
		ret = stub_glNormal3f;

	else if (strcmp(name, "glNormal3fv") == 0)
		ret = stub_glNormal3fv;

	else if (strcmp(name, "glMatrixMode") == 0)
		ret = stub_glMatrixMode;

	else if (strcmp(name, "glMultMatrixf") == 0)
		ret = stub_glMultMatrixf;

	else if (strcmp(name, "glOrtho") == 0)
		ret = stub_glOrtho;

	else if (strcmp(name, "glPolygonMode") == 0)
		ret = stub_glPolygonMode;

	else if (strcmp(name, "glPopMatrix") == 0)
		ret = stub_glPopMatrix;

	else if (strcmp(name, "glPushMatrix") == 0)
		ret = stub_glPushMatrix;

	else if (strcmp(name, "glReadBuffer") == 0)
		ret = stub_glReadBuffer;

	else if (strcmp(name, "glReadPixels") == 0)
		ret = stub_glReadPixels;

	else if (strcmp(name, "glRotatef") == 0)
		ret = stub_glRotatef;

	else if (strcmp(name, "glScalef") == 0)
		ret = stub_glScalef;

	else if (strcmp(name, "glShadeModel") == 0)
		ret = stub_glShadeModel;

	else if (strcmp(name, "glTexCoord1f") == 0)
		ret = stub_glTexCoord1f;

	else if (strcmp(name, "glTexCoord2f") == 0)
		ret = stub_glTexCoord2f;

	else if (strcmp(name, "glTexCoord2fv") == 0)
		ret = stub_glTexCoord2fv;

	else if (strcmp(name, "glTexEnvf") == 0)
		ret = stub_glTexEnvf;

	else if (strcmp(name, "glTexEnvfv") == 0)
		ret = stub_glTexEnvfv;

	else if (strcmp(name, "glTexEnvi") == 0)
		ret = stub_glTexEnvi;

	else if (strcmp(name, "glTexGeni") == 0)
		ret = stub_glTexGeni;

	else if (strcmp(name, "glTexImage2D") == 0)
		ret = stub_glTexImage2D;

	else if (strcmp(name, "glTexParameteri") == 0)
		ret = stub_glTexParameteri;

	else if (strcmp(name, "glTexParameterf") == 0)
		ret = stub_glTexParameterf;

	else if (strcmp(name, "glTexSubImage2D") == 0)
		ret = stub_glTexSubImage2D;

	else if (strcmp(name, "glTranslatef") == 0)
		ret = stub_glTranslatef;

	else if (strcmp(name, "glVertex2f") == 0)
		ret = stub_glVertex2f;

	else if (strcmp(name, "glVertex3f") == 0)
		ret = stub_glVertex3f;

	else if (strcmp(name, "glVertex3fv") == 0)
		ret = stub_glVertex3fv;

	else if (strcmp(name, "glViewport") == 0)
		ret = stub_glViewport;

	else if (strcmp(name, "glGetError") == 0)
		ret = stub_glGetError;

	else if (strcmp(name, "glDrawElements") == 0)
		ret = stub_glDrawElements;

	else if (strcmp(name, "glArrayElement") == 0)
		ret = stub_glArrayElement;

	else if (strcmp(name, "glVertexPointer") == 0)
		ret = stub_glVertexPointer;

	else if (strcmp(name, "glNormalPointer") == 0)
		ret = stub_glNormalPointer;

	else if (strcmp(name, "glTexCoordPointer") == 0)
		ret = stub_glTexCoordPointer;

	else if (strcmp(name, "glColorPointer") == 0)
		ret = stub_glColorPointer;

	else if (strcmp(name, "glDrawArrays") == 0)
		ret = stub_glDrawArrays;

	else if (strcmp(name, "glEnableClientState") == 0)
		ret = stub_glEnableClientState;

	else if (strcmp(name, "glDisableClientState") == 0)
		ret = stub_glDisableClientState;

	else if (strcmp(name, "glStencilOp") == 0)
		ret = stub_glStencilOp;

	else if (strcmp(name, "glStencilFunc") == 0)
		ret = stub_glStencilFunc;

	else if (strcmp(name, "glPushAttrib") == 0)
		ret = stub_glPushAttrib;

	else if (strcmp(name, "glPopAttrib") == 0)
		ret = stub_glPopAttrib;

	else if (strcmp(name, "glScissor") == 0)
		ret = stub_glScissor;

	else if (strcmp(name, "glMultiTexCoord2fARB") == 0)
		ret = stub_glMultiTexCoord2fARB;

	else if (strcmp(name, "glMultiTexCoord3fARB") == 0)
		ret = stub_glMultiTexCoord3fARB;

	else if (strcmp(name, "glActiveTextureARB") == 0)
		ret = stub_glActiveTextureARB;

	else if (strcmp(name, "glClientActiveTextureARB") == 0)
		ret = stub_glClientActiveTextureARB;

	else
		printf("Function \"%s\" not found\n", name);

	return ret;
}

qboolean GLVID_Init (rendererstate_t *info, unsigned char *palette)
{
	int argnum;

	int r;

	int i;

	int depth = 24;

	struct TagItem tgltags[] =
	{
		{ 0, 0 },
		{ TGL_CONTEXT_STENCIL, TRUE },
		{ TAG_DONE }
	};

	if (IntuitionBase->LibNode.lib_Version > 50 || (IntuitionBase->LibNode.lib_Version == 50 && IntuitionBase->LibNode.lib_Revision >= 74))
	{
		mosgammatable = AllocVec(256*3, MEMF_ANY);
		if (mosgammatable)
			gammaworks = 1;
	}

	vid.pixelwidth = info->width;
	vid.pixelheight = info->height;
	vid.numpages = 3;

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

	TinyGLBase = OpenLibrary("tinygl.library", 0);
	if (TinyGLBase)
	{
		if (TinyGLBase->lib_Version > 50 || (TinyGLBase->lib_Version == 50 && TinyGLBase->lib_Revision >= 9))
		{
			if (info->fullscreen)
			{
				screen = OpenScreenTags(0,
					SA_Width, vid.width,
					SA_Height, vid.height,
					SA_Depth, depth,
					SA_Quiet, TRUE,
					SA_GammaControl, TRUE,
					SA_3DSupport, TRUE,
					TAG_DONE);
			}

			window = OpenWindowTags(0,
				WA_InnerWidth, vid.width,
				WA_InnerHeight, vid.height,
				WA_Title, "FTEQuake",
				WA_DragBar, screen?FALSE:TRUE,
				WA_DepthGadget, screen?FALSE:TRUE,
				WA_Borderless, screen?TRUE:FALSE,
				WA_RMBTrap, TRUE,
				screen?WA_PubScreen:TAG_IGNORE, (ULONG)screen,
				WA_Activate, TRUE,
				TAG_DONE);

			if (window)
			{
				if (screen == 0)
					gammaworks = 0;

				__tglContext = GLInit();
				if (__tglContext)
				{
					if (screen)
					{
						tgltags[0].ti_Tag = TGL_CONTEXT_SCREEN;
						tgltags[0].ti_Data = screen;
					}
					else
					{
						tgltags[0].ti_Tag = TGL_CONTEXT_WINDOW;
						tgltags[0].ti_Data = window;
					}

					r = GLAInitializeContext(__tglContext, tgltags);

					if (r)
					{
						glctx = 1;

						gl_stencilbits = 8;

						pointermem = AllocVec(256, MEMF_ANY|MEMF_CLEAR);
						if (pointermem)
						{
							SetPointer(window, pointermem, 16, 16, 0, 0);

#if 0
							lastwindowedmouse = 1;
#endif

							if (vid.height > vid.pixelheight)
								vid.height = vid.pixelheight;
							if (vid.width > vid.pixelwidth)
								vid.width = vid.pixelwidth;

							GL_Init(&TinyGL_GetSymbol);

							VID_SetPalette(palette);

							vid.recalc_refdef = 1;

							return true;
						}
						else
						{
							Con_Printf("Unable to allocate memory for mouse pointer");
						}

						if (screen && !(TinyGLBase->lib_Version == 0 && TinyGLBase->lib_Revision < 4))
						{
							glADestroyContextScreen();
						}
						else
						{
							glADestroyContextWindowed();
						}

						glctx = 0;
					}
					else
					{
						Con_Printf("Unable to initialize GL context");
					}

					GLClose(__tglContext);
					__tglContext = 0;
				}
				else
				{
					Con_Printf("Unable to create GL context");
				}

				if (screen)
				{
					CloseScreen(screen);
					screen = 0;
				}

				CloseWindow(window);
				window = 0;
			}
			else
			{
				Con_Printf("Unable to open window");
			}

		}

		CloseLibrary(TinyGLBase);
		TinyGLBase = 0;
	}
	else
	{
		Con_Printf("Couldn't open tinygl.library");
	}

	return false;
}

void GLVID_DeInit(void)
{
	if (glctx)
	{
		if (screen && !(TinyGLBase->lib_Version == 0 && TinyGLBase->lib_Revision < 4))
		{
			glADestroyContextScreen();
		}
		else
		{
			glADestroyContextWindowed();
		}

		glctx = 0;
	}

	if (__tglContext)
	{
		GLClose(__tglContext);
		__tglContext = 0;
	}

	if (window)
	{
		CloseWindow(window);
		window = 0;
	}

	if (pointermem)
	{
		FreeVec(pointermem);
		pointermem = 0;
	}

	if (screen)
	{
		CloseScreen(screen);
		screen = 0;
	}

	if (TinyGLBase)
	{
		CloseLibrary(TinyGLBase);
		TinyGLBase = 0;
	}

	if (mosgammatable)
	{
		FreeVec(mosgammatable);
		mosgammatable = 0;
	}
}

void GLVID_SwapBuffers (void)
{
	glASwapBuffers();
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
	for (i=0 ; i<256 ; i++)
	{
		r = gammatable[pal[0]];
		g = gammatable[pal[1]];
		b = gammatable[pal[2]];
		pal += 3;
		
		*table1++ = LittleLong((255<<24) + (r<<0) + (g<<8) + (b<<16));
	}
	d_8to24rgbtable[255] &= LittleLong(0xffffff);	// 255 is transparent
}

void GLVID_ShiftPalette (unsigned char *palette)
{
	int i;

	if (gammaworks)
	{
		for(i=0;i<768;i++)
		{
			mosgammatable[i] = ramps[0][i]>>8;
		}

		SetAttrs(screen,
			SA_GammaRed, mosgammatable,
			SA_GammaGreen, mosgammatable+256,
			SA_GammaBlue, mosgammatable+512,
			TAG_DONE);
	}
}

void Sys_SendKeyEvents(void)
{
}

void GLVID_SetCaption(char *caption)
{
	SetWindowTitles(window, caption, (void *)-1);
}

