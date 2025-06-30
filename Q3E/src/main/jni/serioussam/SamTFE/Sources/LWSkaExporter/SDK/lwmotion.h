/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWMOTION.H -- LightWave Item Motions
 */
#ifndef LWSDK_MOTION_H
#define LWSDK_MOTION_H

#include <lwrender.h>

#define LWITEMMOTION_HCLASS     "ItemMotionHandler"
#define LWITEMMOTION_ICLASS     "ItemMotionInterface"
#define LWITEMMOTION_VERSION    4


typedef struct st_LWItemMotionAccess {
        LWItemID          item;
        LWFrame           frame;
        LWTime            time;
        void            (*getParam) (LWItemParam, LWTime, LWDVector);
        void            (*setParam) (LWItemParam, const LWDVector);
} LWItemMotionAccess;

typedef struct st_LWItemMotionHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        void            (*evaluate) (LWInstance, const LWItemMotionAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWItemMotionHandler;

#define LWIMF_AFTERIK   (1<<0)


#endif

