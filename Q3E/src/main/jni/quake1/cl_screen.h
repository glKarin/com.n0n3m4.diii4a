
#ifndef CL_SCREEN_H
#define CL_SCREEN_H

#include "qtypes.h"

void SHOWLMP_decodehide(void);
void SHOWLMP_decodeshow(void);
void SHOWLMP_drawall(void);

extern struct cvar_s vid_conwidth;
extern struct cvar_s vid_conheight;
extern struct cvar_s vid_pixelheight;
extern struct cvar_s scr_screenshot_jpeg;
extern struct cvar_s scr_screenshot_jpeg_quality;
extern struct cvar_s scr_screenshot_png;
extern struct cvar_s scr_screenshot_gammaboost;
extern struct cvar_s scr_screenshot_name;

extern char cl_connect_status[MAX_QPATH];

void CL_Screen_NewMap(void);
void CL_Screen_Init(void);
void CL_Screen_Shutdown(void);
void CL_UpdateScreen(void);

qbool R_Stereo_Active(void);
qbool R_Stereo_ColorMasking(void);

#endif

