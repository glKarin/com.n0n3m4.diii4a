/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWOBJREP.H -- LightWave Object Replacement
 */
#ifndef LWSDK_OBJREP_H
#define LWSDK_OBJREP_H

#include <lwrender.h>

#define LWOBJREPLACEMENT_HCLASS         "ObjReplacementHandler"
#define LWOBJREPLACEMENT_ICLASS         "ObjReplacementInterface"
#define LWOBJREPLACEMENT_VERSION        4


typedef struct st_LWObjReplacementAccess {
        LWItemID         objectID;
        LWFrame          curFrame, newFrame;
        LWTime           curTime,  newTime;
        int              curType,  newType;
        const char      *curFilename;
        const char      *newFilename;
} LWObjReplacementAccess;

#define LWOBJREP_NONE    0
#define LWOBJREP_PREVIEW 1
#define LWOBJREP_RENDER  2


typedef struct st_LWObjReplacementHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        void            (*evaluate) (LWInstance, LWObjReplacementAccess *);
} LWObjReplacementHandler;


#endif

