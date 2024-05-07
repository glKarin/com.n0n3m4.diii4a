#version 320 es
/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

precision mediump float;

// same as TiledCustomMipmapStage::MipmapMode
#pragma tdm_define "MIPMAP_MODE"
// set to 1 if we fill first level from user's source texture
#pragma tdm_define "MIPMAP_FIRST"
// set to number of first lod levels we want to skip
#pragma tdm_define "MIPMAP_SKIP"


// destination mipmap level we need to fill
uniform int u_level;
// for all fetches from source texture, clamp texcoords to this region
// specified as (x1, y1, x2, y2) in pixels (inclusive)
uniform ivec4 u_clampRegion;

ivec2 clampPixelCoords(ivec2 pixelCoords) {
	return clamp(pixelCoords, u_clampRegion.xy, u_clampRegion.zw);
}

// "sample" function returns contents of texture by given pixel coords
#define MODE MIPMAP_MODE //k
#if MIPMAP_FIRST
	#if MODE == 0   // MM_STENCIL_SHADOW
		uniform mediump usampler2D u_sourceTexture;
	#else
		//TODO
		uniform highp sampler2D u_sourceTexture;
	#endif

	vec4 _sample(ivec2 pixelCoords) {
		// fetch from base LOD level
		return vec4(texelFetch(u_sourceTexture, clampPixelCoords(pixelCoords), 0));
	}
#else
	uniform highp sampler2D u_mipmapTexture;

	vec4 _sample(ivec2 pixelCoords) {
		// fetch from previous LOD level
		return texelFetch(u_mipmapTexture, clampPixelCoords(pixelCoords), u_level - 1 - MIPMAP_SKIP);
	}
#endif


// each mode defines behavior via 3 functions:
//   "convertInitial" --- given a raw source texel, computes cooked value for it (the one stored in mipmaps)
//   "combine" --- given cooked values of 4 texels, computes combined cooked value in next LOD level
//   "combineBilinear" --- given simple cooked value sampled from the center of 2x2 texel block, computes combined cooked value in next LOD level
//                         note: define CAN_COMBINE_BILINEAR if this function can be implmented
#if MODE == 0
	float detail_normalizeBoolean(float v) {
		return (v > 0.0 && v < 1.0 ? 0.5 : v);
	}

	vec4 convertInitial(vec4 data) {
		return vec4(data.r <= 128.1);     // 128 or less = lit
	}
	vec4 combine(vec4 a, vec4 b, vec4 c, vec4 d) {
		return vec4(detail_normalizeBoolean((a.r + b.r + c.r + d.r) * 0.25));
	}
#else
	//TODO
#endif


out vec4 FragColor;

void main() {
	ivec2 pixelCoords = ivec2(gl_FragCoord.xy);

	#if MIPMAP_FIRST
		#if MIPMAP_SKIP == 0
			// take raw pixel from source
			FragColor = convertInitial(_sample(pixelCoords));
		#elif MIPMAP_SKIP == 1
			// don't rely on filtering (especially for stencil texture)
			// just fetch 4 raw pixels individually, and combine their values
			FragColor = combine(
				convertInitial(_sample(2 * pixelCoords + ivec2(0, 0))),
				convertInitial(_sample(2 * pixelCoords + ivec2(1, 0))),
				convertInitial(_sample(2 * pixelCoords + ivec2(0, 1))),
				convertInitial(_sample(2 * pixelCoords + ivec2(1, 1)))
			);
		#elif MIPMAP_SKIP == 2
			// don't rely on filtering (especially for stencil texture)
			// just fetch 16 raw pixels individually, and combine their values
			vec4 p0 = combine(
				convertInitial(_sample(4 * pixelCoords + ivec2(0, 0))),
				convertInitial(_sample(4 * pixelCoords + ivec2(1, 0))),
				convertInitial(_sample(4 * pixelCoords + ivec2(0, 1))),
				convertInitial(_sample(4 * pixelCoords + ivec2(1, 1)))
			);
			vec4 p1 = combine(
				convertInitial(_sample(4 * pixelCoords + ivec2(2, 0))),
				convertInitial(_sample(4 * pixelCoords + ivec2(3, 0))),
				convertInitial(_sample(4 * pixelCoords + ivec2(2, 1))),
				convertInitial(_sample(4 * pixelCoords + ivec2(3, 1)))
			);
			vec4 p2 = combine(
				convertInitial(_sample(4 * pixelCoords + ivec2(0, 2))),
				convertInitial(_sample(4 * pixelCoords + ivec2(1, 2))),
				convertInitial(_sample(4 * pixelCoords + ivec2(0, 3))),
				convertInitial(_sample(4 * pixelCoords + ivec2(1, 3)))
			);
			vec4 p3 = combine(
				convertInitial(_sample(4 * pixelCoords + ivec2(2, 2))),
				convertInitial(_sample(4 * pixelCoords + ivec2(3, 2))),
				convertInitial(_sample(4 * pixelCoords + ivec2(2, 3))),
				convertInitial(_sample(4 * pixelCoords + ivec2(3, 3)))
			);
			FragColor = combine(p0, p1, p2, p3);
		#else
			TODO: not implemented yet
		#endif
	#else
		// fetch 4 cooked pixels individually, and combine their values
		FragColor = combine(
			_sample(2 * pixelCoords + ivec2(0, 0)),
			_sample(2 * pixelCoords + ivec2(1, 0)),
			_sample(2 * pixelCoords + ivec2(0, 1)),
			_sample(2 * pixelCoords + ivec2(1, 1))
		);
	#endif
}
