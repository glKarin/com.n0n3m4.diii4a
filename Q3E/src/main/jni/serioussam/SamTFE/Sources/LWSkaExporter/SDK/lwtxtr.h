/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWTXTR.H -- LightWave Textures
 */
 
#ifndef LWSDK_TXTR_H
#define LWSDK_TXTR_H

typedef struct st_TxtrLayer                     *LWTLayerID;
typedef struct st_LWMicropol            *LWMicropolID;
typedef struct st_TxtrContext           *LWTxtrContextID;

#include        <lwrender.h>
#include        <lwenvel.h>
#include        <lwio.h>

/*
Texture coordinate context: 
0 = any: all layers are evaluated. 
1 = object: only object coordinate layers will be evaluated
2 = world: only world coordinate layers will be evaluated.
The conetxt should usually be set to 0, unless the texture needs 
to be evaluated in 2 steps, which is unusual.
*/
#define TCC_ANY         0
#define TCC_OBJECT      1
#define TCC_WORLD       2

typedef struct st_LWMicropol {
        double            oPos[3],wPos[3],oScl[3];
        double            gNorm[3],wNorm[3],ray[3];
        double            bumpHeight,txVal;                     
        double            spotSize;
        double            raySource[3];                 
        double            rayLength;                    
        double            cosine;                               
        double            oXfrm[9],wXfrm[9];    // transformation matrix
        LWItemID          objID,srfID;                  // object ID, surface ID
        LWPntID           verts[4];                             // surrounding vertex IDs
        float             weights[4];                   // vertex weigths
        float         vertsWPos[4][3];          // vertex world positions
        int               polNum,oAxis,wAxis,context;   // polygon number, dominant axis in object and world coordinates, coordinates context

        LWIlluminateFunc *illuminate;
        LWRayTraceFunc   *rayTrace;
        LWRayCastFunc    *rayCast;
        LWRayShadeFunc   *rayShade;
        void                     *userData;
        LWPolID                  polygon;
} LWMicropol;

typedef struct  st_LWTxtrParamDesc{
        char                    *name;          // parameter name for gradient input POPUP
        double                  start,end;      // start and end values for this parameter
        int                             type,flags,itemType; // return type (unit), gradient flags
                                                                                 // type of item used by gradient
        LWItemID                itemID;         // item used if any     
        char                    *itemName;      // item name if any
} LWTxtrParamDesc;

/*input parameter Types */
#define         LWIPT_FLOAT                             2
#define         LWIPT_DISTANCE                  3
#define         LWIPT_PERCENT                   4
#define         LWIPT_ANGLE                             5
/* gParamDesc flags */
#define         LWGF_FIXED_MIN                  (1<<0)  // min value fixed
#define         LWGF_FIXED_MAX                  (1<<1)  // max value fixed
#define         LWGF_FIXED_START                (1<<2)  // start param value fixed
#define         LWGF_FIXED_END                  (1<<3)  // end param value fixed
#define         LWGF_AUTOSIZE                   (1<<4)  // autosize enabled
/* item type*/
#define         LWGI_NONE                               -1
#define         LWGI_OBJECT                             0
#define         LWGI_LIGHT                              1
#define         LWGI_CAMERA                             2
#define         LWGI_BONE                               3
#define         LWGI_VMAP                               4

typedef void    *gParamData;
typedef struct st_LWTxtrParamFuncs {
        double                  (*paramEvaluate)(LWTxtrParamDesc        *param,int      paramNb,LWMicropol      *mp,gParamData  data);
        gParamData              (*paramTime)(void       *userData,LWTxtrParamDesc       *param,int      paramNb,LWTime  t,LWFrame       f);
        void                    (*paramCleanup)(LWTxtrParamDesc *param,int      paramNb,gParamData      data);
} LWTxtrParamFuncs;


#define LWTEXTUREFUNCS_GLOBAL   "Texture Functions 2"

typedef struct st_LWTextureFuncs {
        LWTxtrContextID         (*contextCreate)(LWTxtrParamFuncs       funcs);
        void                    (*contextDestroy)(LWTxtrContextID       gc);
        void                    (*contextAddParam)(LWTxtrContextID      gc,LWTxtrParamDesc      pd);

        LWTextureID             (*create)(int   returnType,const char   *name,LWTxtrContextID   gc,void         *userData);
        void                    (*destroy)(LWTextureID txtr);
        void                    (*copy)(LWTextureID     to,LWTextureID  from);
        void                    (*newtime)(LWTextureID  txtr,LWTime     t,LWFrame       f);
        void                    (*cleanup)(LWTextureID  txtr);
        void                    (*load)(LWTextureID     txtr,const LWLoadState  *loadState);
        void                    (*save)(LWTextureID     txtr,const LWSaveState  *saveState);
        double                  (*evaluate)(LWTextureID txtr,LWMicropolID       mp,double       *rv);
        void                    (*setEnvGroup) (LWTextureID txtr,LWChanGroupID  grp);

        LWTLayerID              (*firstLayer)(LWTextureID txtr);
        LWTLayerID              (*lastLayer)(LWTextureID txtr);
        LWTLayerID              (*nextLayer)(LWTextureID txtr,LWTLayerID layer);
        LWTLayerID              (*layerAdd)(LWTextureID txtr,int layerType);
        void                    (*layerSetType)(LWTLayerID              layer,int layerType);
        int                             (*layerType)(LWTLayerID         layer);
        double                  (*layerEvaluate)(LWTLayerID     layer,LWMicropolID      mp,double       *rv);
        LWChanGroupID   (*layerEnvGroup)(LWTLayerID     layer);
        int                             (*setParam)(LWTLayerID          layer,int       tag,void        *data);
        int                             (*getParam)(LWTLayerID          layer,int       tag,void        *data);
        void                    (*evaluateUV)(LWTLayerID        layer,int       wAxis,int       oAxis,double    oPos[3],double  wPos[3],double  uv[2]);

        double                  (*noise)(double         p[3]);  // returns Perlin's noise values in [0,1]
        void                    *(*userData)(LWTextureID txtr);
        LWChanGroupID   (*envGroup)(LWTextureID txtr);

        LWTextureID             (*texture)(LWTLayerID   layer);
        const char              *(*name)(LWTextureID txtr);
        int                             (*type)(LWTextureID txtr);
        LWTxtrContextID (*context)(LWTextureID txtr);
} LWTextureFuncs;

/*layer types*/
#define TLT_IMAGE               0
#define TLT_PROC                1
#define TLT_GRAD                2

/*return types*/
#define TRT_VECTOR                      0
#define TRT_COLOR                       1
#define TRT_PERCENT                     2
#define TRT_SCALAR                      3
#define TRT_DISPLACEMENT        4

/* parameter tags */
/*      Tag                             data type*/
typedef enum LWTextureTag 
{
        TXTAG_POSI,     //      double [3]
        TXTAG_ROTA,     //      double [3]      
        TXTAG_SIZE,     //      double [3]
        TXTAG_FALL,     //      double [3]
        TXTAG_PROJ,     //      int *
        TXTAG_AXIS,     //      int *
        TXTAG_WWRP, //  double *
        TXTAG_HWRP,     //      double *
        TXTAG_COORD,//  int *   
        TXTAG_IMAGE,//  ImageID *
        TXTAG_VMAP,     //      VMapID *
        TXTAG_ROBJ,     //      LWItemID *
        TXTAG_OPAC,     //      double *
        TXTAG_AA,       //      int     *
        TXTAG_AAVAL,    //      double  *
        TXTAG_PIXBLEND, //      int     *
        TXTAG_WREPEAT,  //      int     *
        TXTAG_HREPEAT   //      int     *
};

/* projection modes */
typedef enum LWTextureProjection {
        TXPRJ_PLANAR,
        TXPRJ_CYLINDRICAL,
        TXPRJ_SPHERICAL,
        TXPRJ_CUBIC,
        TXPRJ_FRONT,
        TXPRJ_UVMAP
};

/* repeat modes */
typedef enum LWTextureRepeatMode {
        TXRPT_RESET,
        TXRPT_REPEAT,
        TXRPT_MIRROR,
        TXRPT_EDGE
};

#endif
