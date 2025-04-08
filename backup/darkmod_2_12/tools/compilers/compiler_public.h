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

#ifndef __COMPILER_PUBLIC_H__
#define __COMPILER_PUBLIC_H__

/*
===============================================================================

	Compilers for map, model, video etc. processing.

===============================================================================
*/

// map processing (also see SuperOptimizeOccluders in tr_local.h)
void Dmap_f( const idCmdArgs &args );

// bump map generation
void RenderBump_f( const idCmdArgs &args );
void RenderBumpFlat_f( const idCmdArgs &args );

// AAS file compiler
void RunAAS_f( const idCmdArgs &args );
void RunAASDir_f( const idCmdArgs &args );
void RunReach_f( const idCmdArgs &args );

// video file encoding
void RoQFileEncode_f( const idCmdArgs &args );

// stgatilov: particle systems precomputation
void RunParticle_f( const idCmdArgs &args );

#endif	/* !__COMPILER_PUBLIC_H__ */
