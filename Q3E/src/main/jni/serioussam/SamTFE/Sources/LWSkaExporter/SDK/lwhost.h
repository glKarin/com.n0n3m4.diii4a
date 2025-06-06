/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWHOST.H -- LightWave Host Services
 *
 * This header contains the declarations for globals provided by all
 * LightWave host applications.
 */
#ifndef LWSDK_HOST_H
#define LWSDK_HOST_H

#include <lwdialog.h>


/*
 * File Request Function.  This function is returned as the "File Request"
 * global service.  It gets a simple filename from the user.
 */
#define LWFILEREQFUNC_GLOBAL            "File Request"

typedef int     LWFileReqFunc (const char *hail, char *name,
                               char *path, char *fullName, int buflen);


/*
 * File Request Activate Function.  This function is returned as the
 * "File Request 2" global service.  It takes a file request local struct
 * and version number and returns the activation code directly.
 */
#define LWFILEACTIVATEFUNC_GLOBAL       "File Request 2"

typedef int     LWFileActivateFunc (int version, LWFileReqLocal *);


/*
 * Color Picker Activate Function.  This function is returned as the
 * "Color Picker" global service.  It takes a color picker local struct
 * and version number and returns the activation code directly.
 */
#define LWCOLORACTIVATEFUNC_GLOBAL      "Color Picker"

typedef int     LWColorActivateFunc (int version, LWColorPickLocal *);


/*
 * File Type Function.  This function is returned as the "File Type Pattern"
 * global service.  It returns filename pattern strings given file type
 * code strings.
 */
#define LWFILETYPEFUNC_GLOBAL           "File Type Pattern"

typedef const char *
                LWFileTypeFunc (const char *);


/*
 * Directory Function.  This function is returned as the "Directory Info"
 * global service.  It returns filename pattern strings given file type
 * code strings.
 */
#define LWDIRINFOFUNC_GLOBAL            "Directory Info"

typedef const char *
                LWDirInfoFunc (const char *);

/*
 * These file type code strings are recognized by the File Type and Directory Info
 * Functions. They correspond with the current configuration for the system.
 */
#define LWFTYPE_ANIMATION       "Animations"
#define LWFTYPE_IMAGE           "Images"
#define LWFTYPE_ENVELOPE        "Envelopes"
#define LWFTYPE_MOTION          "Motions"
#define LWFTYPE_OBJECT          "Objects"
#define LWFTYPE_PLUGIN          "Plug-ins"
#define LWFTYPE_PREVIEW         "Previews"
#define LWFTYPE_PSFONT          "PSFonts"
#define LWFTYPE_SCENE           "Scenes"
#define LWFTYPE_SETTING         "Settings"
#define LWFTYPE_SURFACE         "Surfaces"

/****
 * Message Functions.  This block of functions is returned as the
 * "Info Messages 2" global service.  They display various info
 * and other confirmation dialogs to the user. 
 * The return codes are as follows:
 *  OKCancel                     ok(1) cancel(0)
 *  YesNo                       yes(1) no(0)
 *  YesNoCancel           yes(2) no(1) cancel(0)
 *  YesNoAll    yesAll(3) yes(2) no(1) cancel(0)
 ****
 */
#define LWMESSAGEFUNCS_GLOBAL           "Info Messages 2"

typedef struct st_LWMessageFuncs {
        void    (*info)     (const char *, const char *);
        void    (*error)    (const char *, const char *);
        void    (*warning)  (const char *, const char *);
        int     (*okCancel) (const char *ttl, const char *, const char *);
        int     (*yesNo)    (const char *ttl, const char *, const char *);
        int     (*yesNoCan) (const char *ttl, const char *, const char *);
        int     (*yesNoAll) (const char *ttl, const char *, const char *);
} LWMessageFuncs;


/*
 * System Information.  The value returned as the "System ID" global is
 * a value that should be parsed into bits.  The low bits are the dongle
 * serial number and the high bits are the application code.
 */
#define LWSYSTEMID_GLOBAL       "System ID"

#define LWSYS_TYPEBITS          0xF0000000
#define LWSYS_SERIALBITS        0x0FFFFFFF

#define LWSYS_LAYOUT            0x00000000
#define LWSYS_MODELER           0x10000000
#define LWSYS_SCREAMERNET       0x20000000


/*
 * Product Information.  The value returned as the "Product Info" global
 * is a value that should be parsed into bits.  The various groups of
 * bits contain codes for the specific product and revision.
 */
#define LWPRODUCTINFO_GLOBAL    "Product Info"

#define LWINF_PRODUCT           0x0000000F
#define LWINF_BUILD             0x0000FFF0
#define LWINF_MINORREV          0x000F0000
#define LWINF_MAJORREV          0x00F00000
#define LWINF_RESERVED          0xFF000000

#define LWINF_PRODLWAV          0x00000001
#define LWINF_PRODINSP3D        0x00000002
#define LWINF_PRODOTHER         0x00000004

#define LWINF_GETMAJOR(x)       (((x) & LWINF_MAJORREV) >> 20)
#define LWINF_GETMINOR(x)       (((x) & LWINF_MINORREV) >> 16)
#define LWINF_GETBUILD(x)       (((x) & LWINF_BUILD)    >>  4)


/*
 * Locale Information.  The value returned as the "Locale Info" global
 * is a value whose low bits contain the locale ID for the host
 * application.  The possible values are the LANGID codes defined in
 * lwserver.
 */
#define LWLOCALEINFO_GLOBAL     "Locale Info"

#define LWLOC_LANGID            0x0000FFFF
#define LWLOC_RESERVED          0xFFFF0000

#endif

