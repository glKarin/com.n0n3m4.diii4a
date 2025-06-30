/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWSHADER.H -- LightWave Surface Shaders
 */
#ifndef LWSDK_SHADER_H
#define LWSDK_SHADER_H

#include <lwrender.h>

#define LWSHADER_HCLASS         "ShaderHandler"
#define LWSHADER_ICLASS         "ShaderInterface"
#define LWSHADER_VERSION        4


typedef struct st_LWShaderAccess {
        int               sx, sy;
        double            oPos[3], wPos[3];
        double            gNorm[3];
        double            spotSize;
        double            raySource[3];
        double            rayLength;
        double            cosine;
        double            oXfrm[9],  wXfrm[9];
        LWItemID          objID;
        int               polNum;

        double            wNorm[3];
        double            color[3];
        double            luminous;
        double            diffuse;
        double            specular;
        double            mirror;
        double            transparency;
        double            eta;
        double            roughness;

        LWIlluminateFunc *illuminate;
        LWRayTraceFunc   *rayTrace;
        LWRayCastFunc    *rayCast;
        LWRayShadeFunc   *rayShade;

        int               flags;
        int               bounces;
        LWItemID          sourceID;
        double            wNorm0[3];
        double            bumpHeight;
        double            translucency;
        double            colorHL;
        double            colorFL;
        double            addTransparency;
        double            difSharpness;

        LWPntID           verts[4];                             // surrounding vertex IDs
        float             weights[4];                   // vertex weigths
        float         vertsWPos[4][3];          // vertex world positions
        LWPolID           polygon;                              // polygon ID

        double            replacement_percentage;
        double            replacement_color[3]; 
        double            reflectionBlur;
        double            refractionBlur;
} LWShaderAccess;

#define LWSAF_SHADOW     (1<<0)


typedef struct st_LWShaderHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        LWRenderFuncs    *rend;
        void            (*evaluate) (LWInstance, LWShaderAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWShaderHandler;

#define LWSHF_NORMAL            (1<<0)
#define LWSHF_COLOR                     (1<<1)
#define LWSHF_LUMINOUS          (1<<2)
#define LWSHF_DIFFUSE           (1<<3)
#define LWSHF_SPECULAR          (1<<4)
#define LWSHF_MIRROR            (1<<5)
#define LWSHF_TRANSP            (1<<6)
#define LWSHF_ETA                       (1<<7)
#define LWSHF_ROUGH                     (1<<8)
#define LWSHF_TRANSLUCENT       (1<<9)
#define LWSHF_RAYTRACE          (1<<10)


#endif

