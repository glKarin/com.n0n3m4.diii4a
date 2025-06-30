/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWCUSTOBJ.H -- LightWave Custom Objects
 */
#ifndef LWSDK_CUSTOBJ_H
#define LWSDK_CUSTOBJ_H

#include <lwrender.h>

#define LWCUSTOMOBJ_HCLASS      "CustomObjHandler"
#define LWCUSTOMOBJ_ICLASS      "CustomObjInterface"
#define LWCUSTOMOBJ_VERSION     5


typedef struct st_LWCustomObjAccess {
        int               view;
        int               flags;
        void             *dispData;
        void            (*setColor)   (void *, float rgba[4]);
        void            (*setPattern) (void *, int lpat);
        void            (*setTexture) (void *, int, unsigned char *);
        void            (*setUVs)     (void *, double[2], double[2], double[2], double[2]);
        void            (*point)      (void *, double[3], int csys);
        void            (*line)       (void *, double[3], double[3], int csys);
        void            (*triangle)   (void *, double[3], double[3], double[3], int csys);
        void            (*quad)       (void *, double[3], double[3], double[3], double[3], int csys);
        void            (*circle)     (void *, double[3], double, int csys);
        void            (*text)       (void *, double[3], const char *, int just, int csys);
        LWDVector         viewPos, viewDir;
} LWCustomObjAccess;

#define LWVIEW_ZY        0
#define LWVIEW_XZ        1
#define LWVIEW_XY        2
#define LWVIEW_PERSP     3
#define LWVIEW_LIGHT     4
#define LWVIEW_CAMERA    5
#define LWVIEW_SCHEMA    6

#define LWCOFL_SELECTED (1<<0)

#define LWLPAT_SOLID     0
#define LWLPAT_DOT       1
#define LWLPAT_DASH      2
#define LWLPAT_LONGDOT   3

#define LWCSYS_WORLD     0
#define LWCSYS_OBJECT    1
#define LWCSYS_ICON      2

typedef struct st_LWCustomObjHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        LWRenderFuncs    *rend;
        void            (*evaluate) (LWInstance, const LWCustomObjAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWCustomObjHandler;

#define LWCOF_SCHEMA_OK  1

#endif

