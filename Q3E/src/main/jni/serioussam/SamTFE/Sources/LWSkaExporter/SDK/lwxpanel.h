/****
 * lwxpanel.h
 ****
 * COPYRIGHT (C) 1999 NewTek, Inc.
 * Description:
 *  LWXPanel Services
 ****
 */

#ifndef LWSDK_LWXPANEL_H
#define LWSDK_LWXPANEL_H

#include <lwtypes.h>

#define LWXPANELFUNCS_GLOBAL         "LWXPanel"

/* Hint Tags */

typedef void *                  LWXPanelHint;

#define XpH(x)                  ((void *) (x))
#define XPTAG_END               XpH(0)
#define XPTAG_NULL              XpH(0)
#define XPTAG_CALL              XpH(0xDD002)
#define XPTAG_NEST              XpH(0x101)

#define XPTAG_LABEL             XpH(0x3D000B03)
#define XPTAG_CLASS             XpH(0x3D000C02)
#define XPTAG_ADD               XpH(0x3D000D03)
#define XPTAG_DELETE            XpH(0x3D000E01)
#define XPTAG_CHGNOTIFY         XpH(0x3D000F01)
#define XPTAG_FOCUS             XpH(0x3D001201)
#define XPTAG_AUTORESTORE       XpH(0x3D001481)
#define XPTAG_DESTROYNOTIFY     XpH(0x3D001302)

#define XPTAG_VALUE             XpH(0x3D006502)
#define XPTAG_LINK              XpH(0x3D006681)
#define XPTAG_IMMUPD            XpH(0x3D006781)
#define XPTAG_TRACK             XpH(0x3D006802)
#define XPTAG_ORIENT            XpH(0x3D00CB02)

#define XPTAG_MIN               XpH(0x3D014002)
#define XPTAG_MAX               XpH(0x3D014102)
#define XPTAG_STEP              XpH(0x3D014202)
#define XPTAG_RANGE             XpH(0x3D014304)
#define XPTAG_HARDMINMAX        XpH(0x3D014703)
#define XPTAG_CLRMINMAX         XpH(0x3D014803)
#define XPTAG_SENS              XpH(0x3D014402)

#define XPTAG_BUTNOTIFY         XpH(0x3D015F02)
#define XPTAG_POPCMDFUNC        XpH(0x3D016002)
#define XPTAG_POPFUNCS          XpH(0x3D016203)
#define XPTAG_INTXFORM          XpH(0x3D016303)
#define XPTAG_DRAWCBFUNC        XpH(0x3D016502)
#define XPTAG_ZOOMCBFUNC        XpH(0x3D016603)

#define XPTAG_STRLIST           XpH(0x3D014503)
#define XPTAG_VECLABEL          XpH(0x3D017202)
#define XPTAG_CTRLFRONT         XpH(0x3D017301)
#define XPTAG_CTRLCFG           XpH(0x3D017403)
#define XPTAG_XREQCFG           XpH(0x3D019105)

#define XPTAG_ALIAS             XpH(0x3D01F481)
#define XPTAG_UNALIAS           XpH(0x3D01F581)
#define XPTAG_ENABLE            XpH(0x3D01F984)

#define XPTAG_GROUP             XpH(0x3D01FE81)
#define XPTAG_ORDER             XpH(0x3D01FF81)
#define XPTAG_STACK             XpH(0x3D020983)
#define XPTAG_TABS              XpH(0x3D020A81)

#define XPTAG_LEFT              XpH(0x3D021280)
#define XPTAG_NARROW            XpH(0x3D021380)
#define XPTAG_DIVADD            XpH(0x3D021481)
#define XPTAG_DIVREM            XpH(0x3D021581)
#define XPTAG_BORDER            XpH(0x3D021501)

#define XPTAG_DLOGTYPE          XpH(0x3D02BD01)

/* Hint Macros */

/****
 * Variable name guide
 * IDs are unsigned long > 0x8000
 *      cid  = ControlID
 *      gid  = GroupID
 *      vid  = ValueID
 *      cgid = Either a ControlID or GroupID acceptable
 *      id   = General ID variable; use depends on context
 ****
 */

#define XpEND                           XPTAG_END
#define XpCALL(ref)                     XPTAG_CALL, XPTAG_NULL, XpH(ref)
#define XpSUBCALL(id,ref)               XPTAG_CALL, XpH(id), XpH(ref)
#define XpNEST(id)                      XPTAG_NEST, XpH(id)

/* Configuration Hints */
#define XpLABEL(id,str)                 XPTAG_LABEL, XpH(id), XpH(str), XPTAG_NULL
#define XpCLASS(id,cl)                  XPTAG_CLASS, XpH(id), XpH(cl)

#define XpADD(id,cl,vid)                XPTAG_ADD, XpH(id), XpH(cl), XpH(vid)
#define XpDELETE(id)                    XPTAG_DELETE, XpH(id)

#define XpCHGNOTIFY(func)               XPTAG_CHGNOTIFY, XpH(func)
#define XpFOCUS(cid)                    XPTAG_FOCUS, XpH(cid)
#define XpRESTORE(vid)                  XPTAG_AUTORESTORE, XpH(1), XpH(vid), XPTAG_NULL
#define XpRESTORE_()                    XPTAG_AUTORESTORE, XpH(1)
#define XpRESTORE_ON                    XPTAG_AUTORESTORE, XpH(1), XPTAG_NULL
#define XpRESTORE_OFF                   XPTAG_AUTORESTORE, XpH(0), XPTAG_NULL
#define XpDESTROYNOTIFY(func)           XPTAG_DESTROYNOTIFY, XpH(func), XPTAG_NULL

#define XpVALUE(dep,vid)                XPTAG_VALUE, XpH(dep), XpH(vid)
#define XpLINK_(vid)                    XPTAG_LINK, XpH(vid)
#define XpLINK(vid,link)                XPTAG_LINK, XpH(vid), XpH(link), XPTAG_NULL
#define XpIMMUPD(cid,did)               XPTAG_IMMUPD, XpH(cid), XpH(did), XPTAG_NULL
#define XpIMMUPD_(cid)                  XPTAG_IMMUPD, XpH(cid)
#define XpTRACK(cid,track)              XPTAG_TRACK, XpH(cid), XpH(track)
#define XpORIENT(cid,orient)            XPTAG_ORIENT, XpH(cid), XpH(orient)

#define XpMIN(cid,min)                  XPTAG_MIN, XpH(cid), XpH(min)
#define XpMAX(cid,max)                  XPTAG_MAX, XpH(cid), XpH(max)
#define XpSTEP(cid,step)                XPTAG_STEP, XpH(cid), XpH(step)
#define XpRANGE(cid,mn,mx,st)           XPTAG_RANGE, XpH(cid), XpH(mn), XpH(mx), XpH(st)
#define XpHARDMINMAX(cid,mn,mx)         XPTAG_HARDMINMAX, XpH(cid), XpH(mn), XpH(mx)
#define XpCLRMINMAX(cid,mn,mx)          XPTAG_CLRMINMAX, XpH(cid), XpH(mn), XpH(mx)
#define XpSENS(cid,sens)                XPTAG_SENS, XpH(cid), XpH(sens)

#define XpSTRLIST(cid,slist)            XPTAG_STRLIST, XpH(cid), XpH(slist), XPTAG_NULL
#define XpVECLABEL(cid,slist)           XPTAG_VECLABEL, XpH(cid), XpH(slist)
#define XpCTRLFRONT(cid)                XPTAG_CTRLFRONT, XpH(cid)
#define XpCTRLCFG(cid,opt)              XPTAG_CTRLCFG, XpH(cid), XPTAG_NULL, XpH(opt)
#define XpXREQCFG(cid,mode,ttl,ftyp)    XPTAG_XREQCFG, XpH(cid), XpH(mode), XpH(ttl), XpH(ftyp), XPTAG_NULL

/* Callbacks */
#define XpBUTNOTIFY(cid,func)           XPTAG_BUTNOTIFY, XpH(cid), XpH(func)
#define XpPOPCMDFUNC(cid,func)          XPTAG_POPCMDFUNC, XpH(cid), XpH(func)
#define XpPOPFUNCS(cid,cnt,nam)         XPTAG_POPFUNCS, XpH(cid), XpH(cnt), XpH(nam)
#define XpINTXFORM(cid,cnt,ilist)       XPTAG_INTXFORM, XpH(cid), XpH(cnt), XpH(ilist)
#define XpPOPXFORM(cid,cnt,ilist)       XPTAG_INTXFORM(cid,cnt,ilist)
#define XpCHOXFORM(cid,cnt,ilist)       XPTAG_INTXFORM(cid,cnt,ilist)
#define XpDRAWCBFUNC(cid,func)          XPTAG_DRAWCBFUNC, XpH(cid), XpH(func)
#define XpZOOMCBFUNC(cid,func,reg)      XPTAG_ZOOMCBFUNC, XpH(cid), XpH(func), XpH(reg)

/* Relationships */
#define XpALIAS_(id)                    XPTAG_ALIAS, XpH(id)
#define XpUNALIAS_(id)                  XPTAG_UNALIAS, XpH(id)

#define XpENABLE(vid,sub)               XPTAG_ENABLE, XpH(vid), XPTAG_NULL, XPTAG_NULL, XPTAG_NULL, XpH(sub), XPTAG_NULL
#define XpENABLE_(vid)                  XPTAG_ENABLE, XpH(vid), XPTAG_NULL, XPTAG_NULL, XPTAG_NULL
#define XpENABLE_MAP_(vid,map)          XPTAG_ENABLE, XpH(vid), XPTAG_NULL, XPTAG_NULL, XpH(map)
#define XpENABLEMSG_(vid,msg)           XPTAG_ENABLE, XpH(vid), XpH(msg), XPTAG_NULL, XPTAG_NULL
#define XpENABLEMSG_MAP_(vid,map,msg)   XPTAG_ENABLE, XpH(vid), XpH(msg), XPTAG_NULL, XpH(map)

/* Layout Hints */
#define XpGROUP_(gid)                   XPTAG_GROUP, XpH(gid)
#define XpORDER_(gid)                   XPTAG_ORDER, XpH(gid)
#define XpSTACK_(gid,vid)               XPTAG_STACK, XpH(gid), XpH(vid), XPTAG_NULL
#define XpSTACK_MAP_(gid,vid,map)       XPTAG_STACK, XpH(gid), XpH(vid), XpH(map)
#define XpTABS_(gid)                    XPTAG_TABS, XpH(gid)

#define XpLEFT(gid)                     XPTAG_LEFT, XpH(gid), XPTAG_NULL
#define XpLEFT_()                       XPTAG_LEFT
#define XpNARROW(gid)                   XPTAG_NARROW, XpH(gid), XPTAG_NULL
#define XpNARROW_()                     XPTAG_NARROW

#define XpDIVADD(id)                    XPTAG_DIVADD, XpH(id), XPTAG_NULL
#define XpDIVADD_(id)                   XPTAG_DIVADD, XpH(id)
#define XpDIVREM(id)                    XPTAG_DIVREM, XpH(id), XPTAG_NULL
#define XpDIVREM_(id)                   XPTAG_DIVREM, XpH(id)

#define XpBORDER(typ)                   XPTAG_BORDER, XpH(typ)
#define XpDLOGTYPE(typ)                 XPTAG_DLOGTYPE, XpH(typ)

/* TypeDefs */

typedef void *LWXPanelID;

/* Controls */
typedef struct st_LWXPanelControl {
  unsigned long   cid;
  const char      *label;
  const char      *ctrlclass;
} LWXPanelControl;

/* Data Description */
typedef struct st_LWXPanelDataDesc {
  unsigned long   vid;
  const char      *name;
  const char      *datatype;
} LWXPanelDataDesc;

/* Draw Functions */
typedef void *LWXPDrAreaID;
typedef struct st_LWXPDrawFuncs {
        void   (*drawPixel)    ( LWXPDrAreaID p, int c, int x, int y );
        void   (*drawRGBPixel) ( LWXPDrAreaID p,
                                 int r, int g, int b, int x, int y );
        void   (*drawLine)     ( LWXPDrAreaID p,
                                 int c, int x, int y, int x2, int y2 );
        void   (*drawBox)      ( LWXPDrAreaID p,
                                 int c, int x, int y, int w, int h );
        void   (*drawRGBBox)   ( LWXPDrAreaID p,
                                 int r, int g, int b,
                                 int x, int y, int w, int h );
        void   (*drawBorder)   ( LWXPDrAreaID p,
                                 int indent, int x, int y, int w, int h );
        int    (*textWidth)    ( LWXPDrAreaID p, char *s );
        int    (*textHeight)   ( LWXPDrAreaID p, char *s );
        void   (*drawText)     ( LWXPDrAreaID p, char *s, int c, int x, int y );
} LWXPDrawFuncs;

/* Panel Types */
#define LWXP_VIEW       1
#define LWXP_FORM       2

/****
 * Client Notification Event Types
 *
 * TRACK     - Value changes are in progress; e.g., mouse drag of a mini-slider
 * VALUE     - Value has changed. Change could result for a
 *             mouse-up event, user hitting tab or enter, etc.
 *             These events are usually generated by controls
 *             which have an underlying value (e.g., "string")
 * HIT       - Similar to value events but usually associated
 *             with NULL-based control types (vButton, etc.)
 * FOCUS     - LWXPanel has gained user's attention
 * LOSEFOCUS - LWXPanel lost user's attention
 */
enum en_LWXPEventTypes {
        LWXPEVENT_TRACK = 1,
        LWXPEVENT_VALUE,
        LWXPEVENT_HIT,
        LWXPEVENT_FOCUS,
        LWXPEVENT_LOSEFOCUS
};

typedef enum en_LWXPRefreshCodes {
        LWXPRC_NONE = 0,
        LWXPRC_DFLT, 
        LWXPRC_DRAW, 
        LWXPRC_FULL
} LWXPRefreshCode;

enum LWXPDialogTypes {
        LWXPDLG_OKCANCEL = 2,  // ok(1)/cancel(0) (default panel type);
        LWXPDLG_DONE,          // done(1);
        LWXPDLG_OKONLY,        // ok(1);
        LWXPDLG_YESNO,         // yes(1)/no(0);
        LWXPDLG_YESNOALL,      // yes(1)/no(0)/yes-to-all(2)/cancel(3).
        LWXPDLG_YESNOCAN       // yes(1)/no(0)/cancel(3);
};

/* sFileName modes */
#define LWXPREQ_LOAD   0
#define LWXPREQ_SAVE   1
#define LWXPREQ_DIR    2

/* Border types */
#define LWXPBDR_NONE   0
#define LWXPBDR_UP     1
#define LWXPBDR_DOWN   2

/* Control configuration */

/* Thumbnail Control */
#define THUM_SML  (1<<0)
#define THUM_MED  (1<<1)
#define THUM_LRG  (1<<2)
#define THUM_XLG  (1<<3)

#define THUM_SQ   (1<<16)
#define THUM_NTSC (1<<17)
#define THUM_35MM (1<<18)
#define THUM_PORT (1<<19)
#define THUM_WIDE (1<<20)
#define THUM_ANAW (1<<21)
#define THUM_EURO (1<<22)

#define THUM_FULL (1<<31)
#define THUM_LAND THUM_NTSC


/****
 * Callback Prototypes
 * The "Get" returns a pointer to the value for ValueID 'vid'.
 * The pointer is then cast and dereferenced as needed.
 * The "Set" is given a pointer to the value for the ValueID, 'vid'.
 * The "Set" returns one of the above refresh flags where 'NONE'
 * indicates 'vid' was unrecognized.
 ****
 */
typedef void *LWXPanelGetFunc ( void            *inst,
                                unsigned long   vid );

typedef LWXPRefreshCode LWXPanelSetFunc ( void            *inst,
                                          unsigned long   vid,
                                          void            *value );


/****
 * Destroy notification
 ****
 * This is called _after_ destruction of the panel instance and is
 * intended for cleanup of any panel specific allocations, etc.
 * This is necessary when the panel was returned to the application
 * by setting the LWInterface "panel" to a plugin created LWXPanelID.
 * In such cases, the application may destroy the panel automatically.
 * The panData argument is whatever was set using "setData (pan,0,panData)"
 * to set the panel's userdata.
 *
 * If the callback is called, the client should not destroy the panel.
 * If it is not called, the client should destroy its interface with
 * the client is destroyed.
 ****
 */
typedef void  LWXPanelDestroyNotifyFunc ( void *panData );

/* Event notification */
typedef LWXPRefreshCode  LWXPanelChangeNotifyFunc  ( LWXPanelID    pan,
                                                     unsigned long cid,
                                                     unsigned long vid,
                                                     int event_type );

/* Button click event notification */
typedef void  LWXPanelBtnClickFunc      ( LWXPanelID    pan,
                                          int           cid );

/* Popup command */
typedef void  LWXPanelPopCmdFunc        ( LWXPanelID    pan,
                                          int           cid,
                                          int           cmdid );

/* Popup choice */
typedef int         LWXPanelPopCntFunc  ( void *userdata );
typedef const char *LWXPanelPopNameFunc ( void *userdata, int idx );

/* Draw Function */
typedef void LWXPanelControlDrawFunc    ( LWXPanelID pan,
                                          unsigned long cid,
                                          LWXPDrAreaID reg,
                                          int w, int h );
typedef void LWXPanelControlZoomFunc    ( LWXPanelID pan,
                                          unsigned long cid,
                                          int x, int y,
                                          int *region,
                                          int clickcount );

/****
 *      GLOBAL SERVICE INTERFACE.
 *              These are the functions required to create
 *              and manipulate an LWXPanel.
 *
 * version:     the version number of the interface.  Certain control types
 *              or hints may not be available in older interfaces.
 *
 *      SETUP FUNCTIONS
 *
 * create:      construct a new panel of the given type from the control
 *              description.  The type can be LWXP_VIEW or LWXP_FORM.
 *
 * destroy:     free a panel.
 *
 * describe:    define the data values for this panel.  The data description
 *              contains the list of value IDs and their types.  If this is
 *              a VIEW panel, then the get and set functions are required to
 *              read and write the values from the opaque client instance.
 *
 * hint:        apply a hint array to the panel.  This may be called multiple
 *              times, but only while the panel is closed.
 *
 *      USER DATA FUNCTIONS
 * 
 * setData:     set userdata for a control which will be passed to the
 *              callbacks for that control.  A control ID of zero sets
 *              the data for the whole panel. (NOTE: userdata for ID
 *              zero is returned in the deestroy notification callback)
 *
 * getData:     get the userdata for a control or the whole panel.
 *
 *      PANEL VALUE FUNCTIONS
 *              (a brief description)
 *              The panel can present data as a VIEW or a FORM.
 *              A VIEW panel is a "view" of a client owned data instance.
 *              Therefore, the client must provide get and set methods to
 *              allow the panel to directly update the data instance set
 *              with viewInst.
 *              A FORM panel, on the other hand, presents a collection of
 *              data parameters where the data instance itself is owned by
 *              the panel. The parameter values are accessed by the client
 *              using the formGet and formSet methods.
 *
 * formSet:     write a data value to a slot in a FORM panel.  The value
 *              pointer should point to the new value for the entry, or
 *              null for none.
 *
 * formGet:     read the value from a slot in a FORM panel.  Null is returned
 *              for slots that are unset.
 *
 * viewInst:    set the instance pointer for a VIEW panel.  The values on the
 *              panel will be read from the instance using the client's get
 *              and set methods.
 *
 * viewRefresh: "Refreshes" a VIEW panel by reloading control values from the
 *              client data instance. Generally, this is reserved for use when
 *              the client data instance is modified from an external source
 *              other than the panel interface or through other operations
 *              internal to the client.
 *
 *      INTERACTION FUNCTIONS
 *
 * post:        display the panel for modal interaction.  The panel will open
 *              as a modal dialog and will not return control to the caller
 *              until the user selects 'OK' or 'Cancel'.  The function returns
 *              true or false, respectively for those cases.
 *
 * open:        display the panel for non-modal interaction.  This function
 *              returns true immediately and the panel stays open until the
 *              user closes it.
 *
 ****
 */

/* Global Services */
typedef struct st_LWXPanelFuncs {
        int             version;
        LWXPDrawFuncs   *drawf;
        LWXPanelID      (*create)       ( int type, LWXPanelControl *ctrlDesc );
        void            (*destroy)      ( LWXPanelID panel );
        void            (*describe)     ( LWXPanelID panel,
                                          LWXPanelDataDesc *dataDesc,
                                          LWXPanelGetFunc *get,
                                          LWXPanelSetFunc *set );
        void            (*hint)         ( LWXPanelID panel, unsigned long id,
                                          LWXPanelHint *hints );
        void            *(*getData)     ( LWXPanelID panel,
                                          unsigned long id );
        void            (*setData)      ( LWXPanelID panel,
                                          unsigned long id,
                                          void *data );
        void            *(*formGet)     ( LWXPanelID panel,
                                          unsigned long vid );
        void            (*formSet)      ( LWXPanelID panel,
                                          unsigned long vid,
                                          void *value );
        void            (*viewInst)     ( LWXPanelID panel, void *inst );
        void            (*viewRefresh)  ( LWXPanelID panel );
        int             (*post)         ( LWXPanelID panel );
        int             (*open)         ( LWXPanelID panel );
} LWXPanelFuncs;

/****
 * Helper Macros
 * These simplify use of the LWXPanel formGet and formSet functions
 ****
 */
#define FSETINT(fun,pan,cid,i)  {long l=(long)i;(*fun->formSet)(pan,cid,&l);}
#define FGETINT(fun,pan,cid)    (*((int*)(*fun->formGet)(pan,cid)))
#define FSETFLT(fun,pan,cid,f)  {double d=(double)f;(*fun->formSet)(pan,cid,&d);}
#define FGETFLT(fun,pan,cid)    (*((double*)(*fun->formGet)(pan,cid)))

/* close LWSDK_LWXPANEL_H */
#endif
