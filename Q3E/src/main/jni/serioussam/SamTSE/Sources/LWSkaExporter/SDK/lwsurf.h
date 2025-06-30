/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWSURF.H -- LightWave Surfaces
 */
 
#ifndef LWSDK_SURF_H
#define LWSDK_SURF_H

typedef struct st_GCoreSurface          *LWSurfaceID;

#include        <lwrender.h>
#include        <lwtxtr.h>
#include        <lwenvel.h>
#include        <lwimage.h>

#define LWSURFACEFUNCS_GLOBAL   "Surface Functions 2"

#define SURF_COLR        "BaseColor"
#define SURF_LUMI        "Luminosity"
#define SURF_DIFF        "Diffuse"
#define SURF_SPEC        "Specularity"
#define SURF_REFL        "Reflectivity"
#define SURF_TRAN        "Transparency"
#define SURF_TRNL        "Translucency"
#define SURF_RIND        "IOR"
#define SURF_BUMP        "Bump"
#define SURF_GLOS        "Glossiness"
#define SURF_BUF1        "SpecialBuffer1"
#define SURF_BUF2        "SpecialBuffer2"
#define SURF_BUF3        "SpecialBuffer3"
#define SURF_BUF4    "SpecialBuffer4"
#define SURF_SHRP        "DiffuseSharpness"
#define SURF_SMAN        "SmoothingAngle"
#define SURF_RSAN        "ReflectionSeamAngle"
#define SURF_TSAN        "RefractionSeamAngle"
#define SURF_RBLR        "ReflectionBlurring"
#define SURF_TBLR        "RefractionBlurring"
#define SURF_CLRF        "ColorFilter"
#define SURF_CLRH        "ColorHighlights"
#define SURF_ADTR        "AdditiveTransparency"
#define SURF_AVAL        "AlphaValue"
#define SURF_GVAL        "GlowValue"
#define SURF_LCOL        "LineColor"
#define SURF_LSIZ        "LineSize"
#define SURF_ALPH        "AlphaOptions"
#define SURF_RFOP        "ReflectionOptions"
#define SURF_TROP        "RefractionOptions"
#define SURF_SIDE        "Sidedness"
#define SURF_GLOW        "Glow"
#define SURF_LINE        "RenderOutlines"
#define SURF_RIMG        "ReflectionImage"
#define SURF_TIMG        "RefractionImage"
#define SURF_VCOL        "VertexColoring"


typedef struct st_LWSurfaceFuncs {
        LWSurfaceID             (*create)(const char    *objName,const char     *surfName);
        LWSurfaceID             (*first)(void);
        LWSurfaceID             (*next)(LWSurfaceID     surf);
        LWSurfaceID             *(*byName)(const char   *name,const char        *objName);
        LWSurfaceID             *(*byObject)(const char *name);
        const char              *(*name)(LWSurfaceID    surf);
        const char              *(*sceneObject)(LWSurfaceID     surf);

        int                             (*getInt)(LWSurfaceID   surf,const char *channel);
        double                  *(*getFlt)(LWSurfaceID  surf,const char *channel);
        LWEnvelopeID    (*getEnv)(LWSurfaceID   surf,const char *channel);
        LWTextureID             (*getTex)(LWSurfaceID   surf,const char *channel);
        LWImageID               (*getImg)(LWSurfaceID   surf,const char *channel);

        LWChanGroupID   (*chanGrp)(LWSurfaceID  surf);
        const char              *(*getColorVMap)(LWSurfaceID    surf);
        void                    (*setColorVMap)(LWSurfaceID     surf,const char *vmapName,int   type);
} LWSurfaceFuncs;

#endif
