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

// note: must enable GL_ARB_texture_query_lod to include this

// wrapper around function "textureQueryLod", which is not provided in GL3
// note: returns unclamped value; it can be safely passed to textureLod though
float queryTextureLod(sampler2D texture, vec2 coord) {
#ifdef GL_ARB_texture_query_lod
	// this is the name from extension
	// GL4 version has different name =(
	return textureQueryLOD(texture, coord).y;
#else
	// compute derivatives: both texcoords and screencoords in pixels
	vec2 tcdx = dFdx(coord * textureSize(texture, 0));
	vec2 tcdy = dFdy(coord * textureSize(texture, 0));
	// see 8.14 Texture Minification in specs
	// NOTE: anisotropic filtering is considered to be disabled!
	// with anisotropic filtering correct LOD level might be smaller...
	float maxDerMagnSqr = max(dot(tcdx, tcdx), dot(tcdy, tcdy));
	return log2(maxDerMagnSqr) * 0.5;
#endif
}
