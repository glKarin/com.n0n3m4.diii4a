/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWMESHEDT.H -- LightWave MeshDataEdit Server
 *
 * This header contains the types and declarations for the Modeler
 * MeshDataEdit class.
 */
#ifndef LWSDK_MESHEDT_H
#define LWSDK_MESHEDT_H

#include <lwmodeler.h>
#include <lwmeshes.h>

#define LWMESHEDIT_CLASS        "MeshDataEdit"
#define LWMESHEDIT_VERSION      4


typedef struct st_MeshEditState *       EDStateRef;

typedef struct st_EDPointInfo {
        LWPntID          pnt;
        void            *userData;
        int              layer;
        int              flags;
        double           position[3];
        float           *vmapVec;
} EDPointInfo;

typedef struct st_EDPolygonInfo {
        LWPolID          pol;
        void            *userData;
        int              layer;
        int              flags;
        int              numPnts;
        const LWPntID   *points;
        const char      *surface;
        unsigned long    type;
} EDPolygonInfo;

#define EDDF_SELECT     (1<<0)
#define EDDF_DELETE     (1<<1)
#define EDPF_CCEND      (1<<2)
#define EDPF_CCSTART    (1<<3)

typedef int              EDError;
#define EDERR_NONE       0
#define EDERR_NOMEMORY   1
#define EDERR_BADLAYER   2
#define EDERR_BADSURF    3
#define EDERR_USERABORT  4
#define EDERR_BADARGS    5
#define EDERR_BADVMAP    6

#define EDSELM_CLEARCURRENT     (1<<0)
#define EDSELM_SELECTNEW        (1<<1)
#define EDSELM_FORCEVRTS        (1<<2)
#define EDSELM_FORCEPOLS        (1<<3)

#define OPSEL_MODIFY            (1<<15)

#define EDCOUNT_ALL              0
#define EDCOUNT_SELECT           1
#define EDCOUNT_DELETE           2

typedef EDError          EDPointScanFunc (void *, const EDPointInfo *);
typedef EDError          EDPolyScanFunc (void *, const EDPolygonInfo *);

typedef struct st_EDBoundCv {
        LWPolID          curve;
        int              start, end;
} EDBoundCv;

typedef struct st_MeshEditOp {
        EDStateRef        state;
        int               layerNum;
        void            (*done) (EDStateRef, EDError, int selm);

        int             (*pointCount) (EDStateRef, EltOpLayer, int mode);
        int             (*polyCount)  (EDStateRef, EltOpLayer, int mode);

        EDError         (*pointScan) (EDStateRef, EDPointScanFunc *,
                                      void *, EltOpLayer);
        EDError         (*polyScan)  (EDStateRef, EDPolyScanFunc *,
                                      void *, EltOpLayer);

        EDPointInfo *   (*pointInfo)  (EDStateRef, LWPntID);
        EDPolygonInfo * (*polyInfo)   (EDStateRef, LWPolID);
        int             (*polyNormal) (EDStateRef, LWPolID, double[3]);

        LWPntID         (*addPoint) (EDStateRef, double *xyz);
        LWPolID         (*addFace)  (EDStateRef, const char *surf,
                                     int numPnt, const LWPntID *);
        LWPolID         (*addCurve) (EDStateRef, const char *surf,
                                     int numPnt, const LWPntID *, int flags);
        EDError         (*addQuad)  (EDStateRef, LWPntID, LWPntID,
                                                 LWPntID, LWPntID);
        EDError         (*addTri)   (EDStateRef, LWPntID, LWPntID, LWPntID);
        EDError         (*addPatch) (EDStateRef, int nr, int nc, int lr,
                                     int lc, EDBoundCv *r0, EDBoundCv *r1,
                                     EDBoundCv *c0, EDBoundCv *c1);

        EDError         (*remPoint) (EDStateRef, LWPntID);
        EDError         (*remPoly)  (EDStateRef, LWPolID);

        EDError         (*pntMove) (EDStateRef, LWPntID, const double *);
        EDError         (*polSurf) (EDStateRef, LWPolID, const char *);
        EDError         (*polPnts) (EDStateRef, LWPolID, int, const LWPntID *);
        EDError         (*polFlag) (EDStateRef, LWPolID, int mask, int value);

        EDError         (*polTag)  (EDStateRef, LWPolID, LWID, const char *);
        EDError         (*pntVMap) (EDStateRef, LWPntID, LWID, const char *,
                                    int, float *);

        LWPolID         (*addPoly) (EDStateRef, LWID type, LWPolID, const char *surf,
                                    int numPnt, const LWPntID *);
        LWPntID         (*addIPnt) (EDStateRef, double *xyz, int numPnt,
                                    const LWPntID *, const double *wt);
        EDError         (*initUV)  (EDStateRef, float *uv);

        void *          (*pointVSet) (EDStateRef, void *, LWID, const char *);
        int             (*pointVGet) (EDStateRef, LWPntID, float *);
        const char *    (*polyTag)   (EDStateRef, LWPolID, LWID);

        EDError         (*pntSelect) (EDStateRef, LWPntID, int);
        EDError         (*polSelect) (EDStateRef, LWPolID, int);

        int             (*pointVPGet) (EDStateRef, LWPntID, LWPolID, float *);
        int             (*pointVEval) (EDStateRef, LWPntID, LWPolID, float *);
        EDError         (*pntVPMap)   (EDStateRef, LWPntID, LWPolID,
                                       LWID, const char *, int, float *);
} MeshEditOp;


typedef MeshEditOp *    MeshEditBegin (int pntBuf, int polBuf, EltOpSelect);


#endif

