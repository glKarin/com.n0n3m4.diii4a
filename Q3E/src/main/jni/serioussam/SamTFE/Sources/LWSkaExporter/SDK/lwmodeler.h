/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWMODELER.H -- LightWave Modeler Global State
 *
 * This header contains declarations for the global services and
 * internal states of the Modeler host application.
 */
#ifndef LWSDK_MODELER_H
#define LWSDK_MODELER_H

#include <lwtypes.h>


typedef int              EltOpLayer;
#define OPLYR_PRIMARY    0
#define OPLYR_FG         1
#define OPLYR_BG         2
#define OPLYR_SELECT     3
#define OPLYR_ALL        4
#define OPLYR_EMPTY      5
#define OPLYR_NONEMPTY   6

typedef int              EltOpSelect;
#define OPSEL_GLOBAL     0
#define OPSEL_USER       1
#define OPSEL_DIRECT     2


#define LWSTATEQUERYFUNCS_GLOBAL        "LWM: State Query 3"

typedef struct st_LWStateQueryFuncs {
        int             (*numLayers) (void);
        unsigned int    (*layerMask) (EltOpLayer);
        const char *    (*surface)   (void);
        unsigned int    (*bbox)      (EltOpLayer, double *minmax);
        const char *    (*layerList) (EltOpLayer, const char *);
        const char *    (*object)    (void);
        int             (*mode)      (int);
        const char *    (*vmap)      (int, LWID *);
} LWStateQueryFuncs;

#define LWM_MODE_SELECTION       0
#define LWM_MODE_SYMMETRY        1
#define LWM_VMAP_WEIGHT          0
#define LWM_VMAP_TEXTURE         1
#define LWM_VMAP_MORPH           2


#define LWFONTLISTFUNCS_GLOBAL          "LWM: Font List"

typedef struct st_LWFontListFuncs {
        int             (*count) (void);
        int             (*index) (const char *name);
        const char *    (*name)  (int index);
        int             (*load)  (const char *filename);
        void            (*clear) (int index);
} LWFontListFuncs;


#endif
