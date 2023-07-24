// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __RENDERER_ENUMS_H__
#define __RENDERER_ENUMS_H__

// TTimo: enums cannot be forward-declared, so putting them in their own file

// moved from image.h for default parm
typedef enum {
	TR_REPEAT,
	TR_CLAMP,
	TR_CLAMP_TO_BORDER,		// this should replace TR_CLAMP_TO_ZERO and TR_CLAMP_TO_ZERO_ALPHA,
							// but I don't want to risk changing it right now
	TR_CLAMP_TO_ZERO,		// guarantee 0,0,0,255 edge for projected textures,
							// set AFTER image format selection
	TR_CLAMP_TO_ZERO_ALPHA,	// guarantee 0 alpha edge for projected textures,
							// set AFTER image format selection
	TR_CLAMP_X,				// only clamp x direction
	TR_CLAMP_Y,				// only clamp y direction

	TR_MIRROR,
	TR_MIRROR_X,
	TR_MIRROR_Y,
} textureRepeat_t;

typedef enum {
	TF_LINEAR,
	TF_NEAREST,
	TF_LINEARNEAREST,
	TF_DEFAULT				// use the user-specified r_textureFilter
} textureFilter_t;

// increasing numeric values imply more information is stored
enum textureDepth_t {
	TD_SPECULAR,			// may be compressed, and always zeros the alpha channel
	TD_DIFFUSE,				// may be compressed
	TD_DEFAULT,				// will use compressed formats when possible
	TD_BUMP,				// may be compressed with 8 bit lookup
	TD_HIGH_QUALITY			// either 32 bit or a component format, no loss at all
};

enum textureType_t {
	TT_DISABLED,
	TT_2D,
	TT_3D,
	TT_CUBIC,
	TT_RECT
};

enum cubeFiles_t {
	CF_2D,			// not a cube map
	CF_NATIVE,		// _px, _nx, _py, etc, directly sent to GL
	CF_CAMERA,		// _forward, _back, etc, rotated and flipped as needed before sending to GL
	CF_HALFSPHERE	// Half-Spherical projection map resampled to cubemap at load time
};

enum cullType_t {
	CT_FRONT_SIDED,
	CT_BACK_SIDED,
	CT_TWO_SIDED,
	CT_INVALID
};

enum copyBuffer_t {
	CB_COLOR,
	CB_DEPTH
};

#endif
