/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWDISPLCE.H -- LightWave Vertex Displacements
 */
#ifndef LWSDK_DISPLCE_H
#define LWSDK_DISPLCE_H

#include <lwrender.h>
#include <lwmeshes.h>

#define LWDISPLACEMENT_HCLASS   "DisplacementHandler"
#define LWDISPLACEMENT_ICLASS   "DisplacementInterface"
#define LWDISPLACEMENT_VERSION  5


typedef struct st_LWDisplacementAccess {
        LWDVector        oPos;
        LWDVector        source;
        LWPntID          point;
        LWMeshInfo      *info;
} LWDisplacementAccess;

typedef struct st_LWDisplacementHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        LWRenderFuncs    *rend;
        void            (*evaluate) (LWInstance, LWDisplacementAccess *);
        unsigned int    (*flags) (LWInstance);
} LWDisplacementHandler;

#define LWDMF_WORLD             (1<<0)
#define LWDMF_BEFOREBONES       (1<<1)


#endif

