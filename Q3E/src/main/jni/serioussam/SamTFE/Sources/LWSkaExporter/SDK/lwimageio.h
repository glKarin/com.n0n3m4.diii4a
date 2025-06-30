/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWIMAGEIO.H -- LightWave Image Input/Ouput
 *
 * This header defines the structures required for basic image I/O.
 * This includes the different image matrix protocols and the local
 * structs for the image loader and image saver.
 */
#ifndef LWSDK_IMAGEIO_H
#define LWSDK_IMAGEIO_H

#include <lwmonitor.h>

#define LWIMAGELOADER_CLASS     "ImageLoader"
#define LWIMAGELOADER_VERSION   2

#define LWIMAGESAVER_CLASS      "ImageSaver"
#define LWIMAGESAVER_VERSION    2


/*
 * Image Pixel Datatypes.
 */
typedef enum en_LWImageType {
        LWIMTYP_RGB24 = 0,
        LWIMTYP_GREY8,
        LWIMTYP_INDEX8,
        LWIMTYP_GREYFP,
        LWIMTYP_RGBFP,
        LWIMTYP_RGBA32,
        LWIMTYP_RGBAFP,
        LWIMTYP_SPECIAL
} LWImageType;

/*
 * Image Pixel Structures.
 */
typedef void *          LWPixelID;

typedef struct st_LWPixelRGB24 {
        unsigned char    r;
        unsigned char    g;
        unsigned char    b;
} LWPixelRGB24;

typedef struct st_LWPixelRGBFP {
        float            r;
        float            g;
        float            b;
} LWPixelRGBFP;

typedef struct st_LWPixelRGBA32 {
        unsigned char    r;
        unsigned char    g;
        unsigned char    b;
        unsigned char    a;
} LWPixelRGBA32;

typedef struct st_LWPixelRGBAFP {
        float            r;
        float            g;
        float            b;
        float            a;
} LWPixelRGBAFP;

/*
 * Image Buffer Protocol with parameter tags.
 */

typedef enum en_LWImageParam {
        LWIMPAR_ASPECT = 1,             /* x / y Pixel Aspect.                  */
        LWIMPAR_NUMCOLS,
        LWIMPAR_PIXELWIDTH,             /* Actual (scanned)Pixel Width in (mm). */
        LWIMPAR_FRAMESPERSECOND,        /* Number Of Frames Per Second.         */
        LWIMPAR_BLACKPOINT,             /* Black Point Of Layer.                */
        LWIMPAR_WHITEPOINT,             /* White Point Of Layer.                */
        LWIMPAR_GAMMA,                  /* Linearity Of RGB Color.              */
} LWImageParam;

typedef struct st_LWImageProtocol {
        int               type;
        void             *priv_data;
        int             (*done)      (void *, int);
        void            (*setSize)   (void *, int w, int h);
        void            (*setParam)  (void *, LWImageParam, int, float);
        int             (*sendLine)  (void *, int, const LWPixelID);
        void            (*setMap)    (void *, int, const unsigned char[3]);
} LWImageProtocol, *LWImageProtocolID;


/*
 * "ImageLoader" local struct.
 */

typedef struct st_LWImageLoaderLocal {
        void               *priv_data;
        int                 result;
        const char         *filename;
        LWMonitor          *monitor;
        LWImageProtocolID (*begin) (void *, LWImageType);
        void              (*done)  (void *, LWImageProtocolID);
} LWImageLoaderLocal;

/*
 * "ImageSaver" local struct.
 */

typedef struct st_LWImageSaverLocal {
        void             *priv_data;
        int               result;
        LWImageType       type;
        const char       *filename;
        LWMonitor        *monitor;
        int             (*sendData) (void *, LWImageProtocolID, int flags);
} LWImageSaverLocal;


/*
        Result Value

        The result value indicates the status of the loader or saver upon 
        completion.  If the load or save was sucessful, the value should be 
        IPSTAT_OK.  If a loader fails to recognize a file as something it can load
        it should set the result to IPSTAT_NOREC.  If the server could not open
        the file it should return IPSTAT_BADFILE.  Any other error is just a
        generic failure of the loader or saver and so should set the result to
        IPSTAT_FAILED.  Other failure modes might be possible if required 
        in the future.
*/

#define IPSTAT_OK        0
#define IPSTAT_NOREC     1
#define IPSTAT_BADFILE   2
#define IPSTAT_ABORT     3
#define IPSTAT_FAILED   99

/* Flags to be passed to 'setSize' and 'sendData' callbacks. */

#define IMGF_REVERSE    (1<<0)

/* There are also some protocol macros defined
   to get the whole calling interface right. */

#define LWIP_SETSIZE(p,w,h)     (*(p)->setSize) ((p)->priv_data,w,h)
#define LWIP_SETPARAM(p,t,i,f)  (*(p)->setParam) ((p)->priv_data,t,i,f)
#define LWIP_ASPECT(p,a)        LWIP_SETPARAM (p, LWIMPAR_ASPECT, 0, a)
#define LWIP_NUMCOLORS(p,n)     LWIP_SETPARAM (p, LWIMPAR_NUMCOLS, n, 0.0)
#define LWIP_SENDLINE(p,ln,d)   (*(p)->sendLine) ((p)->priv_data,ln,d)
#define LWIP_SETMAP(p,i,val)    (*(p)->setMap) ((p)->priv_data,i,val)
#define LWIP_DONE(p,err)        (*(p)->done) ((p)->priv_data,err)

/*
        Compatibility macros.

        These types are obsolete, but are included to make it easier to convert 
        to the new image format.  The IMG_* value have been replaced with the
        LWImageType type codes, although the values of 0, 1 and 2 map to equivalent
        types.  The ImageValue type is gone completely, and a Pixel* structure
        type should be used, or an explicit reference to unsigned char.
 */

#define IMG_RGB24        0
#define IMG_GREY8        1
#define IMG_INDEX8       2
typedef unsigned char   ImageValue;

#endif
