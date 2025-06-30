/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWGENERIC.H -- LightWave Generic Commands
 */
#ifndef LWSDK_GENERIC_H
#define LWSDK_GENERIC_H

#include <lwtypes.h>
#include <lwdyna.h>

#define LWLAYOUTGENERIC_CLASS   "LayoutGeneric"
#define LWLAYOUTGENERIC_VERSION 2


typedef struct st_LWLayoutGeneric {
        int             (*saveScene) (const char *file);
        int             (*loadScene) (const char *file, const char *name);

        void             *data;
        LWCommandCode   (*lookup)   (void *, const char *cmdName);
        int             (*execute)  (void *, LWCommandCode cmd, int argc,
                                     const DynaValue *argv, DynaValue *result);
        int             (*evaluate) (void *, const char *command);
} LWLayoutGeneric;

#endif

