/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWRENDER.H -- LightWave Rendering State
 *
 * This header contains the basic declarations need to define the
 * simplest LightWave plug-in server.
 */
#ifndef LWSDK_RENDER_H
#define LWSDK_RENDER_H

#include <lwtypes.h>
#include <lwhandler.h>
#include <lwenvel.h>
#include <lwmeshes.h>


typedef void *           LWItemID;
#define LWITEM_NULL      ((LWItemID) 0)

typedef int              LWItemType;
#define LWI_OBJECT       0
#define LWI_LIGHT        1
#define LWI_CAMERA       2
#define LWI_BONE         3

typedef int              LWItemParam;
#define LWIP_POSITION    1
#define LWIP_RIGHT       2
#define LWIP_UP          3
#define LWIP_FORWARD     4
#define LWIP_ROTATION    5
#define LWIP_SCALING     6
#define LWIP_PIVOT       7
#define LWIP_W_POSITION  8
#define LWIP_W_RIGHT     9
#define LWIP_W_UP        10
#define LWIP_W_FORWARD   11
#define LWIP_PIVOT_ROT   12

typedef double           LWRayCastFunc (const LWDVector position,
                                        const LWDVector direction);

typedef double           LWRayTraceFunc (const LWDVector position,
                                         const LWDVector direction,
                                         LWDVector color);

typedef double           LWRayShadeFunc (const LWDVector position,
                                         const LWDVector direction,
                                         struct st_LWShaderAccess *);

typedef int              LWIlluminateFunc (LWItemID light, const LWDVector position,
                                           LWDVector direction, LWDVector color);

#define LWITEM_RADIOSITY        ((LWItemID) 0x21000000)
#define LWITEM_CAUSTICS         ((LWItemID) 0x22000000)


/*
 * Animation item handler extensions.
 */
typedef struct st_LWItemFuncs {
        const LWItemID *        (*useItems) (LWInstance);
        void            (*changeID) (LWInstance, const LWItemID *);
} LWItemFuncs;

typedef struct st_LWItemHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
} LWItemHandler;

#define LWITEM_ALL      ((LWItemID) ~0)


/*
 * Render handler extensions.
 */
typedef struct st_LWRenderFuncs {
        LWError         (*init)    (LWInstance, int);
        void            (*cleanup) (LWInstance);
        LWError         (*newTime) (LWInstance, LWFrame, LWTime);
} LWRenderFuncs;

#define LWINIT_PREVIEW   0
#define LWINIT_RENDER    1

typedef struct st_LWRenderHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        LWRenderFuncs    *rend;
} LWRenderHandler;


/*
 * Globals.
 */
#define LWITEMINFO_GLOBAL       "LW Item Info 3"

typedef struct st_LWItemInfo {
        LWItemID        (*first)        (LWItemType, LWItemID);
        LWItemID        (*next)         (LWItemID);
        LWItemID        (*firstChild)   (LWItemID parent);
        LWItemID        (*nextChild)    (LWItemID parent, LWItemID prevChild);
        LWItemID        (*parent)       (LWItemID);
        LWItemID        (*target)       (LWItemID);
        LWItemID        (*goal)         (LWItemID);
        LWItemType      (*type)         (LWItemID);
        const char *    (*name)         (LWItemID);
        void            (*param)        (LWItemID, LWItemParam, LWTime,
                                         LWDVector);
        unsigned int    (*limits)       (LWItemID, LWItemParam,
                                         LWDVector min, LWDVector max);
        const char *    (*getTag)       (LWItemID, int);
        void            (*setTag)       (LWItemID, int, const char *);
        LWChanGroupID   (*chanGroup)    (LWItemID);
        const char *    (*server)       (LWItemID, const char *, int);
        unsigned int    (*serverFlags)  (LWItemID, const char *, int);
        void            (*controller)   (LWItemID, LWItemParam, int type[3]);
        unsigned int    (*flags)        (LWItemID);
        LWTime          (*lookAhead)    (LWItemID);
        double          (*goalStrength) (LWItemID);
        void            (*stiffness)    (LWItemID, LWItemParam, LWDVector);
} LWItemInfo;

#define LWVECF_0        (1<<0)
#define LWVECF_1        (1<<1)
#define LWVECF_2        (1<<2)

#define LWSRVF_DISABLED (1<<0)
#define LWSRVF_HIDDEN   (1<<1)

#define LWMOTCTL_KEYFRAMES      0
#define LWMOTCTL_TARGETING      1
#define LWMOTCTL_ALIGN_TO_PATH  2
#define LWMOTCTL_IK             3

#define LWITEMF_ACTIVE          (1<<0)
#define LWITEMF_UNAFFECT_BY_IK  (1<<1)
#define LWITEMF_FULLTIME_IK     (1<<2)
#define LWITEMF_GOAL_ORIENT     (1<<3)
#define LWITEMF_REACH_GOAL      (1<<4)


#define LWOBJECTINFO_GLOBAL     "LW Object Info 4"

typedef struct st_LWObjectInfo {
        const char *    (*filename)    (LWItemID);
        int             (*numPoints)   (LWItemID);
        int             (*numPolygons) (LWItemID);
        unsigned int    (*shadowOpts)  (LWItemID);
        double          (*dissolve)    (LWItemID, LWTime);
        LWMeshInfoID    (*meshInfo)    (LWItemID, int frozen);
        unsigned int    (*flags)       (LWItemID);
        double          (*fog)         (LWItemID, LWTime);
        LWTextureID     (*dispMap)     (LWItemID);
        LWTextureID     (*clipMap)     (LWItemID);
        void            (*patchLevel)  (LWItemID, int *display, int *render);
        void            (*metaballRes) (LWItemID, double *display, double *render);
        LWItemID        (*boneSource)  (LWItemID);
        LWItemID        (*morphTarget) (LWItemID);
        double          (*morphAmount) (LWItemID, LWTime);
        unsigned int    (*edgeOpts)    (LWItemID);
        void            (*edgeColor)   (LWItemID, LWTime, LWDVector color);
        int             (*subdivOrder) (LWItemID);
        double          (*polygonSize) (LWItemID, LWTime);
        int             (*excluded)    (LWItemID object, LWItemID light);
} LWObjectInfo;

#define LWOSHAD_SELF    (1<<0)
#define LWOSHAD_CAST    (1<<1)
#define LWOSHAD_RECEIVE (1<<2)

#define LWOBJF_UNSEEN_BY_CAMERA (1<<0)
#define LWOBJF_UNSEEN_BY_RAYS   (1<<1)
#define LWOBJF_UNAFFECT_BY_FOG  (1<<2)
#define LWOBJF_MORPH_MTSE       (1<<3)
#define LWOBJF_MORPH_SURFACES   (1<<4)

#define LWEDGEF_SILHOUETTE      (1<<0)
#define LWEDGEF_UNSHARED        (1<<1)
#define LWEDGEF_CREASE          (1<<2)
#define LWEDGEF_SURFACE         (1<<3)
#define LWEDGEF_OTHER           (1<<4)
#define LWEDGEF_SHRINK_DIST     (1<<8)


#define LWBONEINFO_GLOBAL       "LW Bone Info 3"

typedef struct st_LWBoneInfo {
        unsigned int    (*flags)      (LWItemID);
        void            (*restParam)  (LWItemID, LWItemParam, LWDVector);
        double          (*restLength) (LWItemID);
        void            (*limits)     (LWItemID, double *inner, double *outer);
        const char *    (*weightMap)  (LWItemID);
        double          (*strength)   (LWItemID);
        int             (*falloff)    (LWItemID);
        void            (*jointComp)  (LWItemID, double *self, double *parent);
        void            (*muscleFlex) (LWItemID, double *self, double *parent);
} LWBoneInfo;

#define LWBONEF_ACTIVE          (1<<0)
#define LWBONEF_LIMITED_RANGE   (1<<1)
#define LWBONEF_SCALE_STRENGTH  (1<<2)
#define LWBONEF_WEIGHT_MAP_ONLY (1<<3)
#define LWBONEF_WEIGHT_NORM     (1<<4)
#define LWBONEF_JOINT_COMP      (1<<5)
#define LWBONEF_JOINT_COMP_PAR  (1<<6)
#define LWBONEF_MUSCLE_FLEX     (1<<7)
#define LWBONEF_MUSCLE_FLEX_PAR (1<<8)


#define LWLIGHTINFO_GLOBAL      "LW Light Info 3"

typedef struct st_LWLightInfo {
        void            (*ambient)      (LWTime, LWDVector color);
        int             (*type)         (LWItemID);
        void            (*color)        (LWItemID, LWTime, LWDVector color);
        int             (*shadowType)   (LWItemID);
        void            (*coneAngles)   (LWItemID, LWTime, double *radius, double *edge);
        unsigned int    (*flags)        (LWItemID);
        double          (*range)        (LWItemID, LWTime);
        int             (*falloff)      (LWItemID);
        LWImageID       (*projImage)    (LWItemID);
        int             (*shadMapSize)  (LWItemID);
        double          (*shadMapAngle) (LWItemID, LWTime);
        double          (*shadMapFuzz)  (LWItemID, LWTime);
        int             (*quality)      (LWItemID);
        void            (*rawColor)     (LWItemID, LWTime, LWDVector color);
        double          (*intensity)    (LWItemID, LWTime);
} LWLightInfo;

#define LWLIGHT_DISTANT  0
#define LWLIGHT_POINT    1
#define LWLIGHT_SPOT     2
#define LWLIGHT_LINEAR   3
#define LWLIGHT_AREA     4

#define LWLSHAD_OFF      0
#define LWLSHAD_RAYTRACE 1
#define LWLSHAD_MAP      2

#define LWLFL_LIMITED_RANGE     (1<<0)
#define LWLFL_NO_DIFFUSE        (1<<1)
#define LWLFL_NO_SPECULAR       (1<<2)
#define LWLFL_NO_CAUSTICS       (1<<3)
#define LWLFL_LENS_FLARE        (1<<4)
#define LWLFL_VOLUMETRIC        (1<<5)
#define LWLFL_NO_OPENGL         (1<<6)
#define LWLFL_FIT_CONE          (1<<7)
#define LWLFL_CACHE_SHAD_MAP    (1<<8)

#define LWLFALL_OFF             0
#define LWLFALL_LINEAR          1
#define LWLFALL_INV_DIST        2
#define LWLFALL_INV_DIST_2      3


#define LWCAMERAINFO_GLOBAL     "LW Camera Info 2"

typedef struct st_LWCameraInfo {
        double          (*zoomFactor)    (LWItemID, LWTime);
        double          (*focalLength)   (LWItemID, LWTime);
        double          (*focalDistance) (LWItemID, LWTime);
        double          (*fStop)         (LWItemID, LWTime);
        double          (*blurLength)    (LWItemID, LWTime);
        void            (*fovAngles)     (LWItemID, LWTime, double *horizontal,
                                                            double *vertical);
        unsigned int    (*flags)         (LWItemID);
        void            (*resolution)    (LWItemID, int *width, int *height);
        double          (*pixelAspect)   (LWItemID, LWTime);
        double          (*separation)    (LWItemID, LWTime);
        void            (*regionLimits)  (LWItemID, int *x0, int *y0, int *x1, int *y1);
        void            (*maskLimits)    (LWItemID, int *x0, int *y0, int *x1, int *y1);
        void            (*maskColor)     (LWItemID, LWDVector color);
} LWCameraInfo;

#define LWCAMF_STEREO           (1<<0)
#define LWCAMF_LIMITED_REGION   (1<<1)
#define LWCAMF_MASK             (1<<2)


#define LWSCENEINFO_GLOBAL      "LW Scene Info 3"

typedef struct st_LWSceneInfo {
        const char       *name;
        const char       *filename;
        int               numPoints;
        int               numPolygons;
        int               renderType;
        int               renderOpts;
        LWFrame           frameStart;
        LWFrame           frameEnd;
        LWFrame           frameStep;
        double            framesPerSecond;
        int               frameWidth;
        int               frameHeight;
        double            pixelAspect;
        int               minSamplesPerPixel;
        int               maxSamplesPerPixel;
        int               limitedRegion[4];     /* x0, y0, x1, y1 */
        int               recursionDepth;
        LWItemID        (*renderCamera) (LWTime);
        int               numThreads;
} LWSceneInfo;

#define LWRTYPE_WIRE             0
#define LWRTYPE_QUICK            1
#define LWRTYPE_REALISTIC        2

#define LWROPT_SHADOWTRACE      (1<<0)
#define LWROPT_REFLECTTRACE     (1<<1)
#define LWROPT_REFRACTTRACE     (1<<2)
#define LWROPT_FIELDS           (1<<3)
#define LWROPT_EVENFIELDS       (1<<4)
#define LWROPT_MOTIONBLUR       (1<<5)
#define LWROPT_DEPTHOFFIELD     (1<<6)
#define LWROPT_LIMITEDREGION    (1<<7)
#define LWROPT_PARTICLEBLUR     (1<<8)


#define LWTIMEINFO_GLOBAL       "LW Time Info"

typedef struct st_LWTimeInfo {
        LWTime           time;
        LWFrame          frame;
} LWTimeInfo;


#define LWCOMPINFO_GLOBAL       "LW Compositing Info"

typedef struct st_LWCompInfo {
        LWImageID        bg;
        LWImageID        fg;
        LWImageID        fgAlpha;
} LWCompInfo;


#define LWBACKDROPINFO_GLOBAL   "LW Backdrop Info 2"

typedef struct st_LWBackdropInfo {
        void            (*backdrop) (LWTime, const double ray[3], double color[3]);
        int               type;
        void            (*color)    (LWTime, double zenith[3], double sky[3], double ground[3], double nadir[3]);
        void            (*squeeze)  (LWTime, double *sky, double *ground);
} LWBackdropInfo;

#define LWBACK_SOLID            0
#define LWBACK_GRADIENT         1


#define LWFOGINFO_GLOBAL        "LW Fog Info"

typedef struct st_LWFogInfo {
        int               type;
        unsigned int      flags;
        double          (*minDist) (LWTime);
        double          (*maxDist) (LWTime);
        double          (*minAmt)  (LWTime);
        double          (*maxAmt)  (LWTime);
        void            (*color)   (LWTime, double col[3]);
} LWFogInfo;

#define LWFOG_NONE              0
#define LWFOG_LINEAR            1
#define LWFOG_NONLINEAR1        2
#define LWFOG_NONLINEAR2        3

#define LWFOGF_BACKGROUND       (1<<0)


#define LWINTERFACEINFO_GLOBAL  "LW Interface Info 3"

typedef struct st_LWInterfaceInfo {
        LWTime            curTime;
        const LWItemID   *selItems;
        unsigned int    (*itemFlags) (LWItemID);
        LWFrame           previewStart;
        LWFrame           previewEnd;
        LWFrame           previewStep;
        int               dynaUpdate;
        void            (*schemaPos) (LWItemID, double *x, double *y);
        int             (*itemVis) (LWItemID);
        unsigned int      displayFlags;
        unsigned int      generalFlags;
        int               boxThreshold;
        int             (*itemColor) (LWItemID);
        int               alertLevel;
} LWInterfaceInfo;

#define LWITEMF_SELECTED        (1<<0)
#define LWITEMF_SHOWCHILDREN    (1<<1)
#define LWITEMF_SHOWCHANNELS    (1<<2)
#define LWITEMF_LOCKED          (1<<3)

#define LWDYNUP_OFF             0
#define LWDYNUP_DELAYED         1
#define LWDYNUP_INTERACTIVE     2

#define LWIVIS_HIDDEN           0
#define LWIVIS_VISIBLE          1

#define LWOVIS_HIDDEN           0
#define LWOVIS_BOUNDINGBOX      1
#define LWOVIS_VERTICES         2
#define LWOVIS_WIREFRAME        3
#define LWOVIS_FFWIREFRAME      4
#define LWOVIS_SHADED           5
#define LWOVIS_TEXTURED         6

#define LWDISPF_MOTIONPATHS     (1<<0)
#define LWDISPF_HANDLES         (1<<1)
#define LWDISPF_IKCHAINS        (1<<2)
#define LWDISPF_CAGES           (1<<3)
#define LWDISPF_SAFEAREAS       (1<<4)
#define LWDISPF_FIELDCHART      (1<<5)

#define LWGENF_HIDETOOLBAR      (1<<0)
#define LWGENF_RIGHTTOOLBAR     (1<<1)
#define LWGENF_PARENTINPLACE    (1<<2)
#define LWGENF_FRACTIONALFRAME  (1<<3)
#define LWGENF_KEYSINSLIDER     (1<<4)
#define LWGENF_PLAYEXACTRATE    (1<<5)

#define LWALERT_BEGINNER        0
#define LWALERT_INTERMEDIATE    1
#define LWALERT_EXPERT          2


#define LWGLOBALPOOL_RENDER_GLOBAL      "Global Render Memory"
#define LWGLOBALPOOL_GLOBAL             "Global Memory"

typedef void *          LWMemChunk;

typedef struct st_LWGlobalPool {
        LWMemChunk      (*first) (void);
        LWMemChunk      (*next)  (LWMemChunk);
        const char *    (*ID)    (LWMemChunk);
        int             (*size)  (LWMemChunk);

        LWMemChunk      (*find)   (const char *ID);
        LWMemChunk      (*create) (const char *ID, int size);
} LWGlobalPool;

#endif
