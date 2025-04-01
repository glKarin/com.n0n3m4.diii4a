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

#define SFL_SURFACE_HAS_DIFFUSE_TEXTURE					0x01
#define SFL_SURFACE_HAS_SPECULAR_TEXTURE				0x02
#define SFL_SURFACE_HAS_NORMAL_TEXTURE					0x04
#define SFL_SURFACE_HAS_PARALLAX_TEXTURE				0x08

#define SFL_SURFACE_NORMAL_TEXTURE_RGTC					0x10
#define SFL_SURFACE_HAS_TEXTURE_MATRIX					0x20	// in any stage

#define SFL_LIGHT_CUBIC									0x0100
#define SFL_LIGHT_AMBIENT_HAS_DIFFUSE_CUBEMAP			0x0200
#define SFL_LIGHT_AMBIENT_HAS_SPECULAR_CUBEMAP			0x0400
#define SFL_LIGHT_AMBIENT_HAS_SSAO						0x0800

#define SFL_INTERACTION_SHADOWS							0x010000
#define SFL_INTERACTION_BUMPMAP_LIGHT_TOGGLING_FIX		0x020000
#define SFL_INTERACTION_PARALLAX_OFFSET_EXTERNAL_SHADOWS 0x040000
#define SFL_INTERACTION_SHADOW_MAP_CULL_FRONT			0x080000

bool checkFlag(int flags, int mask) {
	return (flags & mask) != 0;
}
