#ifndef _GLES_ETC_H
#define _GLES_ETC_H

//#define USE_RG_ETC1
#ifdef USE_RG_ETC1
#include "ETC1/etc_rg_etc1.h"
#else

#include "ETC1/etc1_android.h"

#endif

#ifdef _OPENGLES3
//#define USE_RG_ETC2
#ifdef USE_RG_ETC2
unsigned char * etc2_encode_image_rgba(const unsigned char* image, int width, int height, int *size);
unsigned char * etc2_encode_image_rgb(const unsigned char* image, int width, int height, int *size);
unsigned int etc2_data_size_rgba(int width, int height);
unsigned int etc2_data_size_rgb(int width, int height);
#else
// #include "EtcLib/Etc/Etc.h"

extern void EncodeC(const unsigned char *a_pafSourceRGBA,
             unsigned int a_uiSourceWidth,
             unsigned int a_uiSourceHeight,
             int a_format,
             int a_eErrMetric,
             float a_fEffort,
             unsigned int a_uiJobs,
             unsigned int a_uimaxJobs,
             unsigned char **a_ppaucEncodingBits,
             unsigned int *a_puiEncodingBitsBytes,
             unsigned int *a_puiExtendedWidth,
             unsigned int *a_puiExtendedHeight);
#endif
#endif

#endif