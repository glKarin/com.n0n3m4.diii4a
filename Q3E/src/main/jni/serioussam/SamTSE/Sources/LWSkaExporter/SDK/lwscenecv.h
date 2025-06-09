/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWSCENECV.H -- LightWave Scene Converters
 */
#ifndef LWSDK_SCENECV_H
#define LWSDK_SCENECV_H

#include <lwhandler.h>

#define LWSCENECONVERTER_CLASS          "SceneConverter"
#define LWSCENECONVERTER_VERSION        1


typedef struct st_LWSceneConverter {
        const char       *filename;
        LWError           readFailure;
        const char       *tmpScene;
        void            (*deleteTmp) (const char *tmpScene);
} LWSceneConverter;


#endif

