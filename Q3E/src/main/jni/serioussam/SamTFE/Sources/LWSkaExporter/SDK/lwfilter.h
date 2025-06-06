/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWFILTER.H -- LightWave Image and Pixel Filters
 *
 * This header contains the basic declarations need to define the
 * simplest LightWave plug-in server.
 */
#ifndef LWSDK_FILTER_H
#define LWSDK_FILTER_H

#include <lwmonitor.h>
#include <lwrender.h>

#define LWIMAGEFILTER_HCLASS    "ImageFilterHandler"
#define LWIMAGEFILTER_ICLASS    "ImageFilterInterface"
#define LWIMAGEFILTER_VERSION   4

#define LWPIXELFILTER_HCLASS    "PixelFilterHandler"
#define LWPIXELFILTER_ICLASS    "PixelFilterInterface"
#define LWPIXELFILTER_VERSION   4


#define LWBUF_SPECIAL    0
#define LWBUF_LUMINOUS   1
#define LWBUF_DIFFUSE    2
#define LWBUF_SPECULAR   3
#define LWBUF_MIRROR     4
#define LWBUF_TRANS      5
#define LWBUF_RAW_RED    6
#define LWBUF_RAW_GREEN  7
#define LWBUF_RAW_BLUE   8
#define LWBUF_SHADING    9
#define LWBUF_SHADOW     10
#define LWBUF_GEOMETRY   11
#define LWBUF_DEPTH      12
#define LWBUF_DIFFSHADE  13
#define LWBUF_SPECSHADE  14
#define LWBUF_MOTION_X   15
#define LWBUF_MOTION_Y   16
#define LWBUF_REFL_RED   17
#define LWBUF_REFL_GREEN 18
#define LWBUF_REFL_BLUE  19
#define LWBUF_RED        32
#define LWBUF_GREEN      33
#define LWBUF_BLUE       34
#define LWBUF_ALPHA      35

typedef struct st_LWFilterAccess {
        int               width, height;
        LWFrame           frame;
        LWTime            start, end;
        float *         (*getLine)  (int type, int y);
        void            (*setRGB)   (int x, int y, const LWFVector rgb);
        void            (*setAlpha) (int x, int y, float alpha);
        LWMonitor        *monitor;
} LWFilterAccess;

typedef struct st_LWImageFilterHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        void            (*process) (LWInstance, const LWFilterAccess *);
        unsigned int    (*flags) (LWInstance);
} LWImageFilterHandler;


typedef struct st_LWPixelAccess {
        double            sx, sy;
        void            (*getVal)  (int type, int num, float *);
        void            (*setRGBA) (const float[4]);
        void            (*setVal)  (int type, int num, float *);
        LWIlluminateFunc *illuminate;
        LWRayTraceFunc   *rayTrace;
        LWRayCastFunc    *rayCast;
        LWRayShadeFunc   *rayShade;
} LWPixelAccess;

typedef struct st_LWPixelFilterHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        LWRenderFuncs    *rend;
        void            (*evaluate) (LWInstance, const LWPixelAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWPixelFilterHandler;

#define LWPFF_BEFOREVOLUME      (1<<30)
#define LWPFF_RAYTRACE  (1<<31)

typedef unsigned int LWFilterContext;

#define LWFCF_PREPROCESS  (1<<0) /* Filter applied in image editor or as pre process */

#endif

