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

#ifndef __MODEL_OBJ_H__
#define __MODEL_OBJ_H__

#include <vector>
#include "idlib/Str.h"

class idMaterial;

/*
===============================================================================

	OBJ loader

===============================================================================
*/

typedef struct {
	int vertex;
	int normal;
	int texcoord;
} obj_index_t;

typedef struct {
	idStr name;
	const idMaterial *material;
} obj_material_t;

typedef struct {
	// vertex attributes
	idList<idVec3> vertices;   // 'v'
	idList<idVec3> normals;    // 'vn'
	idList<idVec2> texcoords;  // 'vt'
	idList<idVec3> colors;     // 'v' extension

	// triangle indices and material refs
	idList<obj_index_t> indices;
	idList<int> materialIds;
	bool usesUnknownMaterial;

	// materials
	idList<obj_material_t> materials;
	ID_TIME_T timestamp;
} obj_file_t;

obj_file_t *OBJ_Load( const char *fileName );
void		OBJ_Free( obj_file_t *obj );

#endif /* !__MODEL_ASE_H__ */
