#include "quakedef.h"
#include "cl_video.h"

void libavw_close(void *stream);
void *LibAvW_OpenVideo(clvideo_t *video, char *filename, const char **errorstring);
qbool LibAvW_OpenLibrary(void);
void LibAvW_CloseLibrary(void);