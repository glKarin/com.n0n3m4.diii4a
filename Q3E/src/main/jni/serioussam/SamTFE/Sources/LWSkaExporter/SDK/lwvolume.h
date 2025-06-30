/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWVOLUME.H -- LightWave Volumetric Elements
 *
 * This header defines the volumetric rendering element.
 */
#ifndef LWSDK_VOLUME_H
#define LWSDK_VOLUME_H

#include <lwrender.h>

#define LWVOLUMETRIC_HCLASS     "VolumetricHandler"
#define LWVOLUMETRIC_ICLASS     "VolumetricInterface"
#define LWVOLUMETRIC_VERSION    4


/*
 * A volume sample is a single segment along a ray through a
 * volmetric function that has a uniform color and opacity.  The
 * dist and stride are the position and size of the sample and
 * the opacity and color are given as color vectors.
 */
typedef struct  st_LWVolumeSample {
        double          dist;
        double          stride;
        double          opacity[3];
        double          color[3];
} LWVolumeSample;

/*
 * Volumetric ray access structure is passed to the volume rendering
 * server to add its contribution to a ray passing through space.  The
 * ray is given by a void pointer.
 *
 * flags        evaluation falgs.  Indicates whether color or opacity or
 *              both should be computed.
 *
 * source       origin of ray.  Can be a light, the camera, or an object
 *              (for surface rendering).
 *
 * o,dir        origin and direction of ray.
 *
 * rayColor             color that is viewed from the origin of the ray, before
 *              volumetric effects are applied.
 *
 * far,near     far and near clipping distances.
 *
 * oDist        distance from origin (>0 when raytracing reflections /
 *              refractions).
 *
 * frustum      pixel frustum.
 *
 * addSample    add a new volume sample to the ray.
 *
 * getOpacity   returns opacity (vector and scalar) at specified distance.
 */
typedef struct  st_LWVolumeAccess {
        void             *ray;
        int               flags;
        LWItemID          source;

        double            o[3], dir[3], rayColor[3];
        double            farClip,nearClip;
        double            oDist, frustum;

        void            (*addSample)  (void *ray, LWVolumeSample *smp);
        double          (*getOpacity) (void *ray, double dist, double opa[3]);

        LWIlluminateFunc *illuminate;
        LWRayTraceFunc   *rayTrace;
        LWRayCastFunc    *rayCast;
        LWRayShadeFunc   *rayShade;
} LWVolumeAccess;

#define LWVEF_OPACITY   (1<<0)  // light attenuation is evaluated
#define LWVEF_COLOR             (1<<1)  // light scattering is evaluated
#define LWVEF_RAYTRACE  (1<<2)  // raytracing context

typedef struct st_LWVolumetricHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs              *item;
        LWRenderFuncs    *rend;
        double          (*evaluate) (LWInstance, LWVolumeAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWVolumetricHandler;

#define LWVOLF_SHADOWS          (1<<0)  // element will be evaluated for shadows
#define LWVOLF_REFLECTIONS      (1<<1)  // element will be evaluated for raytraced reflections
#define LWVOLF_REFRACTIONS      (1<<2)  // element will be evaluated for raytraced refractions

#endif
