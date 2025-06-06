/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWMASTER.H -- LightWave Master Handlers
 *
 * This header defines the master handler.  This gets notified of changes
 * in the scene and can respond by issuing commands.
 */
#ifndef LWSDK_MASTER_H
#define LWSDK_MASTER_H

#include <lwrender.h>
#include <lwdyna.h>

#define LWMASTER_HCLASS         "MasterHandler"
#define LWMASTER_ICLASS         "MasterInterface"
#define LWMASTER_VERSION        4


typedef struct st_LWMasterAccess {
        int               eventCode;
        void             *eventData;

        void             *data;
        LWCommandCode   (*lookup)   (void *, const char *cmdName);
        int             (*execute)  (void *, LWCommandCode cmd, int argc,
                                     const DynaValue *argv, DynaValue *result);
        int             (*evaluate) (void *, const char *command);
} LWMasterAccess;

#define LWEVNT_NOTHING           0
#define LWEVNT_COMMAND           1
#define LWEVNT_TIME              2
#define LWEVNT_SELECT            3

typedef struct st_LWMasterHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        int               type;
        double          (*event) (LWInstance, const LWMasterAccess *);
        unsigned int    (*flags) (LWInstance);
} LWMasterHandler;

#define LWMAST_SCENE             0
#define LWMAST_LAYOUT            1

#endif
