#include "bothdefs.h"
#if defined(GLQUAKE) && defined(USE_EGL)
#include "gl_videgl.h"

#include <bcm_host.h>
#include "glquake.h"
#include "shader.h"
qboolean RPI_Init (rendererstate_t *info, unsigned char *palette)
{
	static EGL_DISPMANX_WINDOW_T nativewindow;

	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	int rw, rh;

	if (!EGL_LoadLibrary(info->subrenderer))
	{
		Con_Printf("couldn't load EGL library\n");
		return false;
	}
	bcm_host_init();

	graphics_get_display_size(0 /* LCD */, &rw, &rh);
	Con_Printf("Screen size is actually %i*%i\n", rw, rh);

	if (info->width < 64 || info->height < 64)
	{
		info->width = rw;
		info->height = rh;
	}
	dispman_display = vc_dispmanx_display_open(0 /* LCD */);
	dispman_update = vc_dispmanx_update_start(0);


	dst_rect.x = 0;
	dst_rect.y = 0;
	dst_rect.width = info->width;
	dst_rect.height = info->height;
	  
	src_rect.x = 0;
	src_rect.y = 0;
	src_rect.width = info->width << 16;
	src_rect.height = info->height << 16; 

	vid.pixelwidth = info->width;
	vid.pixelheight = info->height;
	 
	dispman_element = vc_dispmanx_element_add(dispman_update, dispman_display, 0/*layer*/, &dst_rect, 0/*src*/, &src_rect, DISPMANX_PROTECTION_NONE, 0 /*alpha*/, 0/*clamp*/, 0/*transform*/);
  
	nativewindow.element = dispman_element;
	nativewindow.width = info->width;
	nativewindow.height = info->height;
	vc_dispmanx_update_submit_sync(dispman_update);


	if (!EGL_Init(info, palette, &nativewindow, EGL_DEFAULT_DISPLAY))
	{
		Con_Printf("couldn't initialise EGL context\n");
		return false;
	}
	GL_Init(&EGL_Proc);
	return true;
}
void RPI_DeInit(void)
{
	EGL_Shutdown();
}
qboolean RPI_ApplyGammaRamps(unsigned int gammarampsize, unsigned short *ramps)
{
	//not supported
	return false;
}
void RPI_SetCaption(char *text)
{
}

#include "gl_draw.h"
rendererinfo_t rpirendererinfo =
{
    "EGL(VideoCore)",
    {
        "rpi",
		"videocore",
		"rpiegl"
    },
    QR_OPENGL,

    GLDraw_Init,
    GLDraw_DeInit,

	GL_UpdateFiltering,
	GL_LoadTextureMips,
	GL_DestroyTexture,

    GLR_Init,
    GLR_DeInit,
    GLR_RenderView,

    RPI_Init,
    RPI_DeInit,
    RPI_ApplyGammaRamps,
    GLVID_GetRGBInfo,

    RPI_SetCaption,       //setcaption


    GLSCR_UpdateScreen,

    GLBE_SelectMode,
    GLBE_DrawMesh_List,
    GLBE_DrawMesh_Single,
    GLBE_SubmitBatch,
    GLBE_GetTempBatch,
    GLBE_DrawWorld,
    GLBE_Init,
    GLBE_GenBrushModelVBO,
    GLBE_ClearVBO,
    GLBE_UploadAllLightmaps,
    GLBE_SelectEntity,
    GLBE_SelectDLight,
    GLBE_Scissor,
    GLBE_LightCullModel,

    GLBE_VBO_Begin,
    GLBE_VBO_Data,
    GLBE_VBO_Finish,
    GLBE_VBO_Destroy,

    GLBE_RenderToTextureUpdate2d,

    ""
};

#endif

