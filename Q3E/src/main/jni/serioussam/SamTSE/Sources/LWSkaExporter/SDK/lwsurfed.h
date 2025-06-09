/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWSURFED.H -- LightWave Surface Editor
 */
 
#ifndef LWSDK_SURFED_H
#define LWSDK_SURFED_H

#include <lwsurf.h>

#define LWSURFEDFUNCS_GLOBAL    "SurfaceEditor Functions"

typedef struct st_LWSurfEdFuncs {
        void                    (*open)(int);
        void                    (*close)(void);
        int                             (*isOpen)(void);
        void                    (*setSurface)(LWSurfaceID);
        void                    (*setPosition)(int      x, int  y);
        void                    (*getPosition)(int      *x, int *y,int *w,int *h);
} LWSurfEdFuncs;

#endif