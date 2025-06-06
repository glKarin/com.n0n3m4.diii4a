/*
 * LWSDK Header File
 * Copyright 1999, NewTek, Inc.
 *
 * LWTEXTURE.H -- LightWave Procedural Textures
 *
 * This header defines the procedural texture render handler.  These
 * are the building blocks of texture channel layers.
 */
#ifndef LWSDK_TEXTURE_H
#define LWSDK_TEXTURE_H

#include <lwrender.h>

#define LWTEXTURE_HCLASS        "TextureHandler"
#define LWTEXTURE_ICLASS        "TextureInterface"
#define LWTEXTURE_VERSION       5

/*
wPos: coordinates in world space, these are the raw coordinates unaffected by any transformation.

tPos: coordinates in texture space. This is the coordinates that will most likeley be used
by the server, the result from the transformation set by the user (scale, position, etc.)

size: scale of the texture in the 3 directions. This is sometimes usefull for antialiasing.
amp: amplitude of the texture. This information can also be used for antialiasing.
spotSize: for antialiasing.

txGrad: this is a return vector in case the server knows how to compute the gradient itself. This
value will only be used if the server returns the LWTEXF_GRAD flag.

axis: which dimensions are being evaluated. Usually it will be AXIS_X | AXIS_Y | AXIS_Z, though it
might change in some situations if the texture is evaluated for example in a 2D or 1D context.

octaves: this is the number of octaves in the fractal texture required by HV. This is how HV 
maintains a constant level of detail based on the distance from the viewer. This should be discarded
for non fractal textures.

txRGBA: this is the return color and alpha value in case the texture outputs its own color.

Note: when returning a gradient or a color, the texture should also return the texture value at that
point. This value can be thought of as the altitude of the texture at that point.

*/
typedef struct st_LWTextureAccess {
        double          wPos[3],tPos[3],size[3],amp; // input
        double          spotSize; // input
        double          txGrad[3]; // output
        int                     axis,flags; // input
        double          octaves; // input
        double          txRGBA[4]; // output
} LWTextureAccess;

typedef struct st_LWTextureHandler {
        LWInstanceFuncs  *inst;
        LWItemFuncs      *item;
        LWRenderFuncs    *rend;
        double          (*evaluate) (LWInstance, LWTextureAccess *);
        unsigned int    (*flags)    (LWInstance);
} LWTextureHandler;

/*
These are the server flags
*/
#define LWTEXF_GRAD                     (1<<0) /* texture returns its own gradient */
#define LWTEXF_SLOWPREVIEW      (1<<1) /* texture editor preview will be delayed */
#define LWTEXF_AXIS                     (1<<2) /* texture needs an axis control */
#define LWTEXF_AALIAS           (1<<3) /* texture is naturally antialiased */
#define LWTEXF_DISPLACE         (1<<4) /* texture is available for displacement */
#define LWTEXF_HV_SRF           (1<<5) /* texture is available for HV surfaces */
#define LWTEXF_HV_VOL           (1<<6) /* texture is available for HV volumes */
#define LWTEXF_SELF_COLOR       (1<<7) /* texture returns its own color */

/*
These are the evaluation flags passed in the TextureAccess
*/
#define LWTXEF_VECTOR           (1<<0)  /* vector (=bump) evaluation context */
#define LWTXEF_AXISX            (1<<1)
#define LWTXEF_AXISY            (1<<2)
#define LWTXEF_AXISZ            (1<<3)
#define LWTXEF_DISPLACE         (1<<4) /* displacement evaluation context */
#define LWTXEF_COLOR            (1<<5) /* color evaluation context */

#endif
