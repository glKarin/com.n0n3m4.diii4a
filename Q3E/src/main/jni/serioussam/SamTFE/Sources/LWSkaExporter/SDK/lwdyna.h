/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWDYNA.H -- LightWave DynaTypes
 *
 * This header defines the types and macros for simple DynaTypes.
 */
#ifndef LWSDK_DYNA_H
#define LWSDK_DYNA_H

#include <lwmonitor.h>
#include <lwxpanel.h>


/*
 * DynaType codes.
 */
typedef int             DynaType;

#define DY_NULL         0
#define DY_STRING       1
#define DY_INTEGER      2
#define DY_FLOAT        3
#define DY_DISTANCE     4
#define DY_VINT         5
#define DY_VFLOAT       6
#define DY_VDIST        7
#define DY_BOOLEAN      8
#define DY_CHOICE       9
#define DY_SURFACE      10
#define DY_FONT         11
#define DY_TEXT         12
#define DY_LAYERS       13
#define DY_CUSTOM       14
#define DY__LAST        DY_CUSTOM


/*
 * DynaValue union.
 */
typedef struct st_DyValString {
        DynaType         type;
        char            *buf;
        int              bufLen;
} DyValString;

typedef struct st_DyValInt {
        DynaType         type;
        int              value;
        int              defVal;
} DyValInt;

typedef struct st_DyValFloat {
        DynaType         type;
        double           value;
        double           defVal;
} DyValFloat;

typedef struct st_DyValIVector {
        DynaType         type;
        int              val[3];
        int              defVal;
} DyValIVector;

typedef struct st_DyValFVector {
        DynaType         type;
        double           val[3];
        double           defVal;
} DyValFVector;

typedef struct st_DyValCustom {
        DynaType         type;
        int              val[4];
} DyValCustom;

typedef union un_DynaValue {
        DynaType         type;
        DyValString      str;
        DyValInt         intv;
        DyValFloat       flt;
        DyValIVector     ivec;
        DyValFVector     fvec;
        DyValCustom      cust;
} DynaValue;


/*
 * Conversion hints.
 */
typedef struct st_DyChoiceHint {
        const char      *item;
        int              value;
} DyChoiceHint;

typedef struct st_DyBitfieldHint {
        char             code;
        int              bitval;
} DyBitfieldHint;

typedef struct st_DynaStringHint {
        DyChoiceHint    *chc;
        DyBitfieldHint  *bits;
} DynaStringHint;


/*
 * Dynamic Requester types.
 */
typedef struct st_DynaRequest   *DynaRequestID;

typedef struct st_DyReqStringDesc {
        DynaType         type;
        int              width;
} DyReqStringDesc;

typedef struct st_DyReqChoiceDesc {
        DynaType         type;
        const char     **items;
        int              vertical;
} DyReqChoiceDesc;

typedef struct st_DyReqTextDesc {
        DynaType         type;
        const char     **text;
} DyReqTextDesc;

typedef union un_DyReqControlDesc {
        DynaType         type;
        DyReqStringDesc  string;
        DyReqChoiceDesc  choice;
        DyReqTextDesc    text;
} DyReqControlDesc;



/*
 * DynaType and DynaValue error codes.
 */
#define DYERR_NONE                0
#define DYERR_MEMORY            (-1)
#define DYERR_BADTYPE           (-2)
#define DYERR_BADSEQ            (-3)
#define DYERR_BADCTRLID         (-4)
#define DYERR_TOOMANYCTRL       (-5)
#define DYERR_INTERNAL          (-6)


/*
 * DynaValue conversion global service.
 */
#define LWDYNACONVERTFUNC_GLOBAL        "LWM: Dynamic Conversion"

typedef int     DynaConvertFunc (const DynaValue *, DynaValue *,
                                 const DynaStringHint *);


/*
 * Dynamic requester service.
 */
#define LWDYNAREQFUNCS_GLOBAL           "LWM: Dynamic Request 2"

typedef struct st_DynaReqFuncs {
        DynaRequestID   (*create)   (const char *);
        int             (*addCtrl)  (DynaRequestID, const char *,
                                     DyReqControlDesc *);
        DynaType        (*ctrlType) (DynaRequestID, int);
        int             (*valueSet) (DynaRequestID, int, DynaValue *);
        int             (*valueGet) (DynaRequestID, int, DynaValue *);
        int             (*post)     (DynaRequestID);
        void            (*destroy)  (DynaRequestID);
        LWXPanelID      (*xpanel)   (DynaRequestID);
} DynaReqFuncs;


/*
 * Global monitor.
 */
#define LWDYNAMONITORFUNCS_GLOBAL       "LWM: Dynamic Monitor"

typedef struct st_DynaMonitorFuncs {
        LWMonitor *     (*create)  (const char *, const char *);
        void            (*destroy) (LWMonitor *);
} DynaMonitorFuncs;


#endif

