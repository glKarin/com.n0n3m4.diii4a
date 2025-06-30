/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWENVIRON.H -- LightWave Environments
 *
 * This header defines the enviroment render handler for backdrops and
 * fog.
 */
#ifndef LWSDK_ENVIRON_H
#define LWSDK_ENVIRON_H

#include <lwrender.h>

#define LWENVIRONMENT_HCLASS    "EnvironmentHandler"
#define LWENVIRONMENT_ICLASS    "EnvironmentInterface"
#define LWENVIRONMENT_VERSION   4


typedef enum en_LWEnvironmentMode {
        EHMODE_PREVIEW,
        EHMODE_REAL
} LWEnvironmentMode;

typedef struct st_LWEnvironmentAccess {
        LWEnvironmentMode mode;
        double            h[2], p[2];
        LWDVector         dir;
        double            colRect[4][3];
        double            color[3];
} LWEnvironmentAccess;

typedef struct st_LWEnvironmentHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        LWRenderFuncs    *rend;
        LWError         (*evaluate) (LWInstance, LWEnvironmentAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWEnvironmentHandler;

#define LWENF_TRANSPARENT       (1<<0)

#endif
