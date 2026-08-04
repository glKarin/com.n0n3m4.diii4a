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

#ifndef __MODEL_MA_H__
#define __MODEL_MA_H__

/*
===============================================================================

	MA loader. (Maya Ascii Format)

===============================================================================
*/

typedef struct {
	char					name[128];
	char					parent[128];
} maNodeHeader_t;

typedef struct {
	char					name[128];
	int						size;
} maAttribHeader_t;

typedef struct maTransform_s {
	idVec3					translate;
	idVec3					rotate;
	idVec3					scale;
	maTransform_s*			parent;	
} maTransform_t;

typedef struct {
	int						edge[3];
	int						vertexNum[3];
	int						tVertexNum[3];
	int						vertexColors[3];
	idVec3					vertexNormals[3];
} maFace_t;

typedef struct {
	
	//Transform to be applied
	maTransform_t*			transform;

	//Verts
	int						numVertexes;
	idVec3 *				vertexes;
	int						numVertTransforms;
	idVec4 *				vertTransforms;
	int						nextVertTransformIndex;

	//Texture Coordinates
	int						numTVertexes;
	idVec2 *				tvertexes;

	//Edges
	int						numEdges;
	idVec3 *				edges;

	//Colors
	int						numColors;
	byte*					colors;

	//Faces
	int						numFaces;
	maFace_t *				faces;

	//Normals
	int						numNormals;
	idVec3 *				normals;
	bool					normalsParsed;
	int						nextNormal;

} maMesh_t;

typedef struct {
	char					name[128];
	float					uOffset, vOffset;		// max lets you offset by material without changing texCoords
	float					uTiling, vTiling;		// multiply tex coords by this
	float					angle;					// in clockwise radians
} maMaterial_t;

typedef struct {
	char					name[128];
	int						materialRef;
	char					materialName[128];

	maMesh_t				mesh;
} maObject_t;


typedef struct {
	char					name[128];
	char					path[1024];
} maFileNode_t;

typedef struct maMaterialNode_s {
	char					name[128];

	maMaterialNode_s*		child;
	maFileNode_t*				file;

} maMaterialNode_t;

typedef struct maModel_s {
	ID_TIME_T						timeStamp;
	idList<maMaterial_t *>		materials;
	idList<maObject_t *>		objects;
	idHashTable<maTransform_t*> transforms;
	
	//Material Resolution
	idHashTable<maFileNode_t*>		fileNodes;
	idHashTable<maMaterialNode_t*>	materialNodes;

} maModel_t;

maModel_t	*MA_Load( const char *fileName );
void		MA_Free( maModel_t *ma );

#endif /* !__MODEL_MA_H__ */
