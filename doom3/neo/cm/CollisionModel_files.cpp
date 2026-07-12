/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
===============================================================================

	Trace model vs. polygonal model collision detection.

===============================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "CollisionModel_local.h"

#define CM_FILE_EXT			"cm"
#define CM_FILEID			"CM"
#ifdef _RAVEN //karin: Quake4 cm file version
#define CM_FILEVERSION		"3"
#define CM_DOOM3_FILEVERSION		"1.00"
#define CM_IS_QUAKE4_VERSION() (cmVersion == CM_FILEVERSION)
#define CM_WRITE_IS_QUAKE4_VERSION() (cmVersion == CM_FILEVERSION)
#elif defined(_SPLASHDAMAGE) //karin: ETQW cm file version
#define CM_FILEVERSION		"2.70"
#else
#define CM_FILEVERSION		"1.00"
#endif

/*
===============================================================================

Writing of collision model file

===============================================================================
*/

void CM_GetNodeBounds(idBounds *bounds, cm_node_t *node);
int CM_GetNodeContents(cm_node_t *node);


/*
================
idCollisionModelManagerLocal::WriteNodes
================
*/
void idCollisionModelManagerLocal::WriteNodes(idFile *fp, cm_node_t *node)
{
	fp->WriteFloatString("\t( %d %f )\n", node->planeType, node->planeDist);

	if (node->planeType != -1) {
		WriteNodes(fp, node->children[0]);
		WriteNodes(fp, node->children[1]);
	}
}

/*
================
idCollisionModelManagerLocal::CountPolygonMemory
================
*/
int idCollisionModelManagerLocal::CountPolygonMemory(cm_node_t *node) const
{
	cm_polygonRef_t *pref;
	cm_polygon_t *p;
	int memory;

	memory = 0;

	for (pref = node->polygons; pref; pref = pref->next) {
		p = pref->p;

		if (p->checkcount == checkCount) {
			continue;
		}

		p->checkcount = checkCount;

		memory += sizeof(cm_polygon_t) + (p->numEdges - 1) * sizeof(p->edges[0]);
	}

	if (node->planeType != -1) {
		memory += CountPolygonMemory(node->children[0]);
		memory += CountPolygonMemory(node->children[1]);
	}

	return memory;
}

/*
================
idCollisionModelManagerLocal::WritePolygons
================
*/
void idCollisionModelManagerLocal::WritePolygons(idFile *fp, cm_node_t *node)
{
	cm_polygonRef_t *pref;
	cm_polygon_t *p;
	int i;

	for (pref = node->polygons; pref; pref = pref->next) {
		p = pref->p;

		if (p->checkcount == checkCount) {
			continue;
		}

		p->checkcount = checkCount;
		fp->WriteFloatString("\t%d (", p->numEdges);

		for (i = 0; i < p->numEdges; i++) {
			fp->WriteFloatString(" %d", p->edges[i]);
		}

		fp->WriteFloatString(" ) ( %f %f %f ) %f", p->plane.Normal()[0], p->plane.Normal()[1], p->plane.Normal()[2], p->plane.Dist());
		fp->WriteFloatString(" ( %f %f %f )", p->bounds[0][0], p->bounds[0][1], p->bounds[0][2]);
		fp->WriteFloatString(" ( %f %f %f )", p->bounds[1][0], p->bounds[1][1], p->bounds[1][2]);
#ifdef _RAVEN // Quake4 cm write
        if(CM_WRITE_IS_QUAKE4_VERSION())
        {
        fp->WriteFloatString(" \"%s\"", p->material->GetName());
        fp->WriteFloatString(" ( %f %f )", 0.0f, 0.0f); // TODO: export cm v3 file
        fp->WriteFloatString(" ( %f %f )", 0.0f, 0.0f); // TODO: export cm v3 file
        fp->WriteFloatString(" ( %f %f )", 0.0f, 0.0f); // TODO: export cm v3 file
        fp->WriteFloatString(" %d\n", 0); // TODO: export cm v3 file
        }
        else
		fp->WriteFloatString(" \"%s\"\n", p->material->GetName());
#elif defined(_SPLASHDAMAGE) //karin: ETQW cm file
        fp->WriteFloatString(" \"%s\"", p->material->GetName());
        fp->WriteFloatString(" %d %d", 0, 0);
        fp->WriteFloatString(" %f %f %f %f", 0.0f, 0.0f, 0.0f, 0.0f);
        fp->WriteFloatString(" %d %d\n", 0, 0);
#else
		fp->WriteFloatString(" \"%s\"\n", p->material->GetName());
#endif
	}

	if (node->planeType != -1) {
		WritePolygons(fp, node->children[0]);
		WritePolygons(fp, node->children[1]);
	}
}

/*
================
idCollisionModelManagerLocal::CountBrushMemory
================
*/
int idCollisionModelManagerLocal::CountBrushMemory(cm_node_t *node) const
{
	cm_brushRef_t *bref;
	cm_brush_t *b;
	int memory;

	memory = 0;

	for (bref = node->brushes; bref; bref = bref->next) {
		b = bref->b;

		if (b->checkcount == checkCount) {
			continue;
		}

		b->checkcount = checkCount;

		memory += sizeof(cm_brush_t) + (b->numPlanes - 1) * sizeof(b->planes[0]);
	}

	if (node->planeType != -1) {
		memory += CountBrushMemory(node->children[0]);
		memory += CountBrushMemory(node->children[1]);
	}

	return memory;
}

/*
================
idCollisionModelManagerLocal::WriteBrushes
================
*/
void idCollisionModelManagerLocal::WriteBrushes(idFile *fp, cm_node_t *node)
{
	cm_brushRef_t *bref;
	cm_brush_t *b;
	int i;

	for (bref = node->brushes; bref; bref = bref->next) {
		b = bref->b;

		if (b->checkcount == checkCount) {
			continue;
		}

		b->checkcount = checkCount;
		fp->WriteFloatString("\t%d {\n", b->numPlanes);

		for (i = 0; i < b->numPlanes; i++) {
			fp->WriteFloatString("\t\t( %f %f %f ) %f\n", b->planes[i].Normal()[0], b->planes[i].Normal()[1], b->planes[i].Normal()[2], b->planes[i].Dist());
		}

		fp->WriteFloatString("\t} ( %f %f %f )", b->bounds[0][0], b->bounds[0][1], b->bounds[0][2]);
#ifdef _RAVEN // Quake4 cm write
        if(CM_WRITE_IS_QUAKE4_VERSION())
        {
        fp->WriteFloatString(" ( %f %f %f ) \"%s\" %d\n", b->bounds[1][0], b->bounds[1][1], b->bounds[1][2], StringFromContents(b->contents), 0); // TODO: export cm v3 file
        }
        else
		fp->WriteFloatString(" ( %f %f %f ) \"%s\"\n", b->bounds[1][0], b->bounds[1][1], b->bounds[1][2], StringFromContents(b->contents));
#elif defined(_SPLASHDAMAGE) //karin: ETQW cm file
        fp->WriteFloatString(" ( %f %f %f ) \"%s\" %d\n", b->bounds[1][0], b->bounds[1][1], b->bounds[1][2], StringFromContents(b->contents), 0);
#else
		fp->WriteFloatString(" ( %f %f %f ) \"%s\"\n", b->bounds[1][0], b->bounds[1][1], b->bounds[1][2], StringFromContents(b->contents));
#endif
	}

	if (node->planeType != -1) {
		WriteBrushes(fp, node->children[0]);
		WriteBrushes(fp, node->children[1]);
	}
}

/*
================
idCollisionModelManagerLocal::WriteCollisionModel
================
*/
void idCollisionModelManagerLocal::WriteCollisionModel(idFile *fp, cm_model_t *model)
{
	int i, polygonMemory, brushMemory;

#ifdef _RAVEN // Quake4 cm write
    if(CM_WRITE_IS_QUAKE4_VERSION()) //karin: for compat doom3 cm
    fp->WriteFloatString("collisionModel \"%s\" %d {\n", model->name.c_str(), 0); // TODO: export cm v3 file
    else
	fp->WriteFloatString("collisionModel \"%s\" {\n", model->name.c_str());
#elif defined(_SPLASHDAMAGE) //karin: ETQW cm file
    fp->WriteFloatString("collisionModel \"%s\" %d {\n", model->name.c_str(), 0);
#else
	fp->WriteFloatString("collisionModel \"%s\" {\n", model->name.c_str());
#endif
	// vertices
	fp->WriteFloatString("\tvertices { /* numVertices = */ %d\n", model->numVertices);

	for (i = 0; i < model->numVertices; i++) {
		fp->WriteFloatString("\t/* %d */ ( %f %f %f )\n", i, model->vertices[i].p[0], model->vertices[i].p[1], model->vertices[i].p[2]);
	}

	fp->WriteFloatString("\t}\n");
	// edges
	fp->WriteFloatString("\tedges { /* numEdges = */ %d\n", model->numEdges);

	for (i = 0; i < model->numEdges; i++) {
		fp->WriteFloatString("\t/* %d */ ( %d %d ) %d %d\n", i, model->edges[i].vertexNum[0], model->edges[i].vertexNum[1], model->edges[i].internal, model->edges[i].numUsers);
	}

	fp->WriteFloatString("\t}\n");
	// nodes
	fp->WriteFloatString("\tnodes {\n");
	WriteNodes(fp, model->node);
	fp->WriteFloatString("\t}\n");
	// polygons
	checkCount++;
	polygonMemory = CountPolygonMemory(model->node);
#ifdef _RAVEN // Quake4 cm write
    if(CM_WRITE_IS_QUAKE4_VERSION())
    fp->WriteFloatString("\tpolygons /* numPolygons = */ %d /* numPolygonEdges = */ %d {\n", polygonMemory, 0); // TODO: export cm v3 file
    else
	fp->WriteFloatString("\tpolygons /* polygonMemory = */ %d {\n", polygonMemory);
#elif defined(_SPLASHDAMAGE) //karin: ETQW cm file
    fp->WriteFloatString("\tpolygons /* numPolygons = */ %d /* numPolygonEdges = */ %d {\n", polygonMemory, 0);
#else
	fp->WriteFloatString("\tpolygons /* polygonMemory = */ %d {\n", polygonMemory);
#endif
	checkCount++;
	WritePolygons(fp, model->node);
	fp->WriteFloatString("\t}\n");
	// brushes
	checkCount++;
	brushMemory = CountBrushMemory(model->node);
#ifdef _RAVEN // Quake4 cm write
    if(CM_WRITE_IS_QUAKE4_VERSION())
    fp->WriteFloatString("\tbrushes /* numBrushes = */ %d /* numBrushPlanes = */ %d {\n", brushMemory, 0); // TODO: export cm v3 file
    else
	fp->WriteFloatString("\tbrushes /* brushMemory = */ %d {\n", brushMemory);
#elif defined(_SPLASHDAMAGE) //karin: ETQW cm file
    fp->WriteFloatString("\tbrushes /* numBrushes = */ %d /* numBrushPlanes = */ %d {\n", brushMemory, 0);
#else
	fp->WriteFloatString("\tbrushes /* brushMemory = */ %d {\n", brushMemory);
#endif
	checkCount++;
	WriteBrushes(fp, model->node);
	fp->WriteFloatString("\t}\n");
	// closing brace
	fp->WriteFloatString("}\n");
}

/*
================
idCollisionModelManagerLocal::WriteCollisionModelsToFile
================
*/
void idCollisionModelManagerLocal::WriteCollisionModelsToFile(const char *filename, int firstModel, int lastModel, unsigned int mapFileCRC)
{
	int i;
	idFile *fp;
	idStr name;

	name = filename;
	name.SetFileExtension(CM_FILE_EXT);

	common->Printf("writing %s\n", name.c_str());
	// _D3XP was saving to fs_cdpath
	fp = fileSystem->OpenFileWrite(name, "fs_devpath");

	if (!fp) {
		common->Warning("idCollisionModelManagerLocal::WriteCollisionModelsToFile: Error opening file %s\n", name.c_str());
		return;
	}

#ifdef _RAVEN // Quake4: using newer cm version
    cmWriteVersion = CM_FILEVERSION;
#endif
	// write file id and version
	fp->WriteFloatString("%s \"%s\"\n\n", CM_FILEID, CM_FILEVERSION);
	// write the map file crc
	fp->WriteFloatString("%u\n\n", mapFileCRC);

	// write the collision models
	for (i = firstModel; i < lastModel; i++) {
		WriteCollisionModel(fp, models[ i ]);
	}

	fileSystem->CloseFile(fp);
}

/*
================
idCollisionModelManagerLocal::WriteCollisionModelForMapEntity
================
*/
bool idCollisionModelManagerLocal::WriteCollisionModelForMapEntity(const idMapEntity *mapEnt, const char *filename, const bool testTraceModel)
{
	idFile *fp;
	idStr name;
	cm_model_t *model;

	SetupHash();
	model = CollisionModelForMapEntity(mapEnt);
	model->name = filename;

	name = filename;
	name.SetFileExtension(CM_FILE_EXT);

	common->Printf("writing %s\n", name.c_str());
	fp = fileSystem->OpenFileWrite(name, "fs_devpath");

	if (!fp) {
		common->Printf("idCollisionModelManagerLocal::WriteCollisionModelForMapEntity: Error opening file %s\n", name.c_str());
#if defined(_RAVEN) || defined(_SPLASHDAMAGE) //karin: free model memory actually
		FreeModel_memory(model);
#else
		FreeModel(model);
#endif
		return false;
	}

#ifdef _RAVEN // Quake4: using newer cm version
    cmWriteVersion = CM_FILEVERSION;
#endif
	// write file id and version
	fp->WriteFloatString("%s \"%s\"\n\n", CM_FILEID, CM_FILEVERSION);
	// write the map file crc
	fp->WriteFloatString("%u\n\n", 0);

	// write the collision model
	WriteCollisionModel(fp, model);

	fileSystem->CloseFile(fp);

	if (testTraceModel) {
		idTraceModel trm;
		TrmFromModel(model, trm);
	}

#if defined(_RAVEN) || defined(_SPLASHDAMAGE) //karin: free model memory actually
	FreeModel_memory(model);
#else
	FreeModel(model);
#endif

	return true;
}


/*
===============================================================================

Loading of collision model file

===============================================================================
*/

/*
================
idCollisionModelManagerLocal::ParseVertices
================
*/
void idCollisionModelManagerLocal::ParseVertices(idLexer *src, cm_model_t *model)
{
	int i;

	src->ExpectTokenString("{");
	model->numVertices = src->ParseInt();
	model->maxVertices = model->numVertices;
	model->vertices = (cm_vertex_t *) Mem_Alloc(model->maxVertices * sizeof(cm_vertex_t));

	for (i = 0; i < model->numVertices; i++) {
		src->Parse1DMatrix(3, model->vertices[i].p.ToFloatPtr());
		model->vertices[i].side = 0;
		model->vertices[i].sideSet = 0;
		model->vertices[i].checkcount = 0;
	}

	src->ExpectTokenString("}");
}

/*
================
idCollisionModelManagerLocal::ParseEdges
================
*/
void idCollisionModelManagerLocal::ParseEdges(idLexer *src, cm_model_t *model)
{
	int i;

	src->ExpectTokenString("{");
	model->numEdges = src->ParseInt();
	model->maxEdges = model->numEdges;
	model->edges = (cm_edge_t *) Mem_Alloc(model->maxEdges * sizeof(cm_edge_t));

	for (i = 0; i < model->numEdges; i++) {
		src->ExpectTokenString("(");
		model->edges[i].vertexNum[0] = src->ParseInt();
		model->edges[i].vertexNum[1] = src->ParseInt();
		src->ExpectTokenString(")");
		model->edges[i].side = 0;
		model->edges[i].sideSet = 0;
		model->edges[i].internal = src->ParseInt();
		model->edges[i].numUsers = src->ParseInt();
		model->edges[i].normal = vec3_origin;
		model->edges[i].checkcount = 0;
		model->numInternalEdges += model->edges[i].internal;
	}

	src->ExpectTokenString("}");
}

/*
================
idCollisionModelManagerLocal::ParseNodes
================
*/
cm_node_t *idCollisionModelManagerLocal::ParseNodes(idLexer *src, cm_model_t *model, cm_node_t *parent)
{
	cm_node_t *node;

	model->numNodes++;
	node = AllocNode(model, model->numNodes < NODE_BLOCK_SIZE_SMALL ? NODE_BLOCK_SIZE_SMALL : NODE_BLOCK_SIZE_LARGE);
	node->brushes = NULL;
	node->polygons = NULL;
	node->parent = parent;
	src->ExpectTokenString("(");
	node->planeType = src->ParseInt();
	node->planeDist = src->ParseFloat();
	src->ExpectTokenString(")");

	if (node->planeType != -1) {
		node->children[0] = ParseNodes(src, model, node);
		node->children[1] = ParseNodes(src, model, node);
	}

	return node;
}

/*
================
idCollisionModelManagerLocal::ParsePolygons
================
*/
void idCollisionModelManagerLocal::ParsePolygons(idLexer *src, cm_model_t *model)
{
	cm_polygon_t *p;
	int i, numEdges;
	idVec3 normal;
	idToken token;

	if (src->CheckTokenType(TT_NUMBER, 0, &token)) {
		model->polygonBlock = (cm_polygonBlock_t *) Mem_Alloc(sizeof(cm_polygonBlock_t) + token.GetIntValue());
		model->polygonBlock->bytesRemaining = token.GetIntValue();
		model->polygonBlock->next = ((byte *) model->polygonBlock) + sizeof(cm_polygonBlock_t);
	}

#ifdef _RAVEN // Quake4 cm file
    // numPolygonEdges
    if(CM_IS_QUAKE4_VERSION())
    if (!src->CheckTokenType(TT_NUMBER, 0, &token)) {
        common->Warning("%s: Expect integer number of numPolygonEdges, but read %s", __FUNCTION__, token.c_str());
    }
#endif
#ifdef _SPLASHDAMAGE //karin: ETQW cm file
	if (!src->CheckTokenType(TT_NUMBER, 0, &token)) {
		common->Warning("%s: Expect integer number of numPolygonEdges, but read %s", __FUNCTION__, token.c_str());
	}
#endif

	src->ExpectTokenString("{");

	while (!src->CheckTokenString("}")) {
		// parse polygon
		numEdges = src->ParseInt();
		p = AllocPolygon(model, numEdges);
		p->numEdges = numEdges;
		src->ExpectTokenString("(");

		for (i = 0; i < p->numEdges; i++) {
			p->edges[i] = src->ParseInt();
		}

		src->ExpectTokenString(")");
		src->Parse1DMatrix(3, normal.ToFloatPtr());
		p->plane.SetNormal(normal);
		p->plane.SetDist(src->ParseFloat());
		src->Parse1DMatrix(3, p->bounds[0].ToFloatPtr());
		src->Parse1DMatrix(3, p->bounds[1].ToFloatPtr());
		src->ExpectTokenType(TT_STRING, 0, &token);
		// get material
		p->material = declManager->FindMaterial(token);
		p->contents = p->material->GetContentFlags();
		p->checkcount = 0;
#ifdef _RAVEN // Quake4 cm file
        if(CM_IS_QUAKE4_VERSION())
        {
        // other unknown (float, float) (float, float) (float, float) integer
        float unknownData[2];
        src->Parse1DMatrix(2, unknownData);
        src->Parse1DMatrix(2, unknownData);
        src->Parse1DMatrix(2, unknownData);
        src->ParseInt();
        }
#endif
#ifdef _SPLASHDAMAGE //karin: ETQW cm file
		// 0 0 0.0000305196 -0 0 -0.0000305157 32768 32768 // int int float float float float int int
		int v43 = src->ParseInt();
		int v11 = src->ParseInt();
		p->contents = ~v11 & (v43 | p->contents);
		//p->contents |= v43;
		//p->contents &= ~v11;
		
		src->ParseFloat();
		src->ParseFloat();
		src->ParseFloat();
		src->ParseFloat();
		src->ParseInt();
		src->ParseInt();
#endif

		// filter polygon into tree
		R_FilterPolygonIntoTree(model, model->node, NULL, p);
	}
}

/*
================
idCollisionModelManagerLocal::ParseBrushes
================
*/
void idCollisionModelManagerLocal::ParseBrushes(idLexer *src, cm_model_t *model)
{
	cm_brush_t *b;
	int i, numPlanes;
	idVec3 normal;
	idToken token;

	if (src->CheckTokenType(TT_NUMBER, 0, &token)) {
		model->brushBlock = (cm_brushBlock_t *) Mem_Alloc(sizeof(cm_brushBlock_t) + token.GetIntValue());
		model->brushBlock->bytesRemaining = token.GetIntValue();
		model->brushBlock->next = ((byte *) model->brushBlock) + sizeof(cm_brushBlock_t);
	}

#ifdef _RAVEN // Quake4 cm file
    // numBrushPlanes
    if(CM_IS_QUAKE4_VERSION())
    if (!src->CheckTokenType(TT_NUMBER, 0, &token)) {
        common->Warning("%s: Expect integer number of numBrushPlanes, but read %s", __FUNCTION__, token.c_str());
    }
#endif
#ifdef _SPLASHDAMAGE //karin: ETQW cm file
	if (!src->CheckTokenType(TT_NUMBER, 0, &token)) {
		common->Warning("%s: Expect integer number of numBrushPlanes, but read %s", __FUNCTION__, token.c_str());
	}
#endif

    src->ExpectTokenString("{");

	while (!src->CheckTokenString("}")) {
		// parse brush
		numPlanes = src->ParseInt();
		b = AllocBrush(model, numPlanes);
		b->numPlanes = numPlanes;
		src->ExpectTokenString("{");

		for (i = 0; i < b->numPlanes; i++) {
			src->Parse1DMatrix(3, normal.ToFloatPtr());
			b->planes[i].SetNormal(normal);
			b->planes[i].SetDist(src->ParseFloat());
		}

		src->ExpectTokenString("}");
		src->Parse1DMatrix(3, b->bounds[0].ToFloatPtr());
		src->Parse1DMatrix(3, b->bounds[1].ToFloatPtr());
		src->ReadToken(&token);

		if (token.type == TT_NUMBER) {
			b->contents = token.GetIntValue();		// old .cm files use a single integer
		} else {
			b->contents = ContentsFromString(token);
		}

		b->checkcount = 0;
		b->primitiveNum = 0;
		// filter brush into tree
		R_FilterBrushIntoTree(model, model->node, NULL, b);

#ifdef _RAVEN // Quake4 cm file
        if(CM_IS_QUAKE4_VERSION())
        {
        // other unknown integer
        src->ParseInt();
        }
#endif
#ifdef _SPLASHDAMAGE //karin: ETQW cm file
        src->ParseInt();
#endif
	}
}

/*
================
idCollisionModelManagerLocal::ParseCollisionModel
================
*/
bool idCollisionModelManagerLocal::ParseCollisionModel(idLexer *src)
{
	cm_model_t *model;
	idToken token;

	if (numModels >= MAX_SUBMODELS) {
		common->Error("LoadModel: no free slots");
		return false;
	}

	model = AllocModel();
	models[numModels ] = model;
	numModels++;
	// parse the file
	src->ExpectTokenType(TT_STRING, 0, &token);
	model->name = token;

#ifdef _RAVEN // quake4 cm file
    if(CM_IS_QUAKE4_VERSION()) //karin: for compat doom3 cm
    {
	if (token.Cmpn(PROC_CLIPMODEL_STRING_PRFX, strlen(PROC_CLIPMODEL_STRING_PRFX)) == 0) {
		numInlinedProcClipModels++;
	}

    if (!src->ExpectTokenType(TT_NUMBER, TT_INTEGER, &token))
    {
        common->Warning("%s: Expect integer number, but read %s", __FUNCTION__, token.c_str());
        return false;
    }
    }
#endif
#ifdef _HUMANHEAD
	//HUMANHEAD rww
#if _HH_INLINED_PROC_CLIPMODELS
	if (anyInlinedProcClipMats) {
		if (token.Cmpn(PROC_CLIPMODEL_STRING_PRFX, strlen(PROC_CLIPMODEL_STRING_PRFX)) == 0) {
			numInlinedProcClipModels++;
		}
	}
#endif
	//HUMANHEAD END
#endif
#ifdef _SPLASHDAMAGE //karin: ETQW cm file
	if (!src->ExpectTokenType(TT_NUMBER, TT_INTEGER, &token))
	{
		common->Warning("%s: Expect integer number, but read %s", __FUNCTION__, token.c_str());
		return false;
	}
#endif

	src->ExpectTokenString("{");

	while (!src->CheckTokenString("}")) {

		src->ReadToken(&token);

		if (token == "vertices") {
			ParseVertices(src, model);
			continue;
		}

		if (token == "edges") {
			ParseEdges(src, model);
			continue;
		}

		if (token == "nodes") {
			src->ExpectTokenString("{");
			model->node = ParseNodes(src, model, NULL);
			src->ExpectTokenString("}");
			continue;
		}

		if (token == "polygons") {
			ParsePolygons(src, model);
			continue;
		}

		if (token == "brushes") {
			ParseBrushes(src, model);
			continue;
		}

		src->Error("ParseCollisionModel: bad token \"%s\"", token.c_str());
	}

#ifdef _SPLASHDAMAGE //karin: mark as worldMap model
	model->SetWorld(!idStr::Icmpn(model->name, WORLD_MODEL_NAME, idStr::Length(WORLD_MODEL_NAME)));
#endif
	// calculate edge normals
	checkCount++;
	CalculateEdgeNormals(model, model->node);
	// get model bounds from brush and polygon bounds
	CM_GetNodeBounds(&model->bounds, model->node);
	// get model contents
	model->contents = CM_GetNodeContents(model->node);
	// total memory used by this model
	model->usedMemory = model->numVertices * sizeof(cm_vertex_t) +
	                    model->numEdges * sizeof(cm_edge_t) +
	                    model->polygonMemory +
	                    model->brushMemory +
	                    model->numNodes * sizeof(cm_node_t) +
	                    model->numPolygonRefs * sizeof(cm_polygonRef_t) +
	                    model->numBrushRefs * sizeof(cm_brushRef_t);

	return true;
}

/*
================
idCollisionModelManagerLocal::LoadCollisionModelFile
================
*/
bool idCollisionModelManagerLocal::LoadCollisionModelFile(const char *name, unsigned int mapFileCRC)
{
	idStr fileName;
	idToken token;
	idLexer *src;
	unsigned int crc;

#ifdef _RAVEN
    cmVersion = CM_FILEVERSION;
#endif
	// load it
	fileName = name;
	fileName.SetFileExtension(CM_FILE_EXT);
#ifdef _SPLASHDAMAGE //karin: parse ETQW binary cmb file
	if (LoadCollisionModelFile_Binary(name, mapFileCRC)) {
		return true;
	}
#endif
	src = new idLexer(fileName);
	src->SetFlags(LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE);

	if (!src->IsLoaded()) {
		delete src;
		return false;
	}

	if (!src->ExpectTokenString(CM_FILEID)) {
		common->Warning("%s is not an CM file.", fileName.c_str());
		delete src;
		return false;
	}

#ifdef _RAVEN // quake4 cm file
	if (!src->ReadToken(&token) || (token != CM_FILEVERSION
#if 1
				 && token != CM_DOOM3_FILEVERSION
#endif
				))
#else
	if (!src->ReadToken(&token) || token != CM_FILEVERSION)
#endif
	{
		common->Warning("%s has version %s instead of %s", fileName.c_str(), token.c_str(), CM_FILEVERSION);
		delete src;
		return false;
	}

#ifdef _RAVEN
    cmVersion = token.c_str();
#endif

	if (!src->ExpectTokenType(TT_NUMBER, TT_INTEGER, &token)) {
		common->Warning("%s has no map file CRC", fileName.c_str());
		delete src;
		return false;
	}

	crc = token.GetUnsignedLongValue();

#if !defined(_SPLASHDAMAGE) //karin: cm CRC is 0
	if (mapFileCRC && crc != mapFileCRC) {
		common->Printf("%s is out of date\n", fileName.c_str());
		delete src;
		return false;
	}
#endif

	// parse the file
	while (1) {
		if (!src->ReadToken(&token)) {
			break;
		}

		if (token == "collisionModel") {
			if (!ParseCollisionModel(src)) {
				delete src;
				return false;
			}

			continue;
		}

		src->Error("idCollisionModelManagerLocal::LoadCollisionModelFile: bad token \"%s\"", token.c_str());
	}

	delete src;

	return true;
}

#ifdef _SPLASHDAMAGE //karin: parse ETQW binary cmb file
cm_node_t * idCollisionModelManagerLocal::ParseNodes_Binary(idFile *file, cm_model_t *model, cm_node_t *parent) {
	cm_node_t *node;

	model->numNodes++;
	node = AllocNode(model, model->numNodes < NODE_BLOCK_SIZE_SMALL ? NODE_BLOCK_SIZE_SMALL : NODE_BLOCK_SIZE_LARGE);
	node->brushes = NULL;
	node->polygons = NULL;
	node->parent = parent;
	file->ReadInt(node->planeType);
	file->ReadFloat(node->planeDist);

	if (node->planeType != -1) {
		node->children[0] = ParseNodes_Binary(file, model, node);
		node->children[1] = ParseNodes_Binary(file, model, node);
	}

	return node;
}

void idCollisionModelManagerLocal::ParseVertices_Binary(idFile *file, cm_model_t *model) {
	int i;

	file->ReadInt(model->numVertices);
	model->maxVertices = model->numVertices;
	model->vertices = (cm_vertex_t *) Mem_Alloc(model->maxVertices * sizeof(cm_vertex_t));

	for (i = 0; i < model->numVertices; i++) {
		file->ReadFloat(model->vertices[i].p[0]);
		file->ReadFloat(model->vertices[i].p[1]);
		file->ReadFloat(model->vertices[i].p[2]);
		model->vertices[i].side = 0;
		model->vertices[i].sideSet = 0;
		model->vertices[i].checkcount = 0;
	}

}

void idCollisionModelManagerLocal::ParseEdges_Binary(idFile *file, cm_model_t *model) {
	int i;
	unsigned short sh[4];

	file->ReadInt(model->numEdges);
	model->maxEdges = model->numEdges;
	model->edges = (cm_edge_t *) Mem_Alloc(model->maxEdges * sizeof(cm_edge_t));

	for (i = 0; i < model->numEdges; i++) {
		file->ReadUnsignedShort(sh[0]);
		file->ReadUnsignedShort(sh[1]);
		file->ReadUnsignedShort(sh[2]);
		file->ReadUnsignedShort(sh[3]);

		model->edges[i].vertexNum[0] = sh[0];
		model->edges[i].vertexNum[1] = sh[1];
		model->edges[i].side = 0;
		model->edges[i].sideSet = 0;
		model->edges[i].internal = sh[2];
		model->edges[i].numUsers = sh[3];
		model->edges[i].normal = vec3_origin;
		model->edges[i].checkcount = 0;
		model->numInternalEdges += model->edges[i].internal;
	}
}

void idCollisionModelManagerLocal::ParsePolygons_Binary(idFile *file, cm_model_t *model, const idStrList &materials) {
	cm_polygon_t *p;
	int i, j;
	unsigned short numEdges;
	int numPolygons, numPolygonEdges;
	short sh;
	float f;
	short bv[6];
	int index;
	unsigned short uh;

	file->ReadInt(numPolygons);
	model->polygonBlock = (cm_polygonBlock_t *) Mem_Alloc(sizeof(cm_polygonBlock_t) + numPolygons);
	model->polygonBlock->bytesRemaining = numPolygons;
	model->polygonBlock->next = ((byte *) model->polygonBlock) + sizeof(cm_polygonBlock_t);

	file->ReadInt(numPolygonEdges);

	for (i = 0; i < numPolygons; i++) {
		// parse polygon
		file->ReadUnsignedShort(numEdges);
		p = AllocPolygon(model, numEdges);
		p->numEdges = numEdges;

		for (j = 0; j < p->numEdges; j++) {
			file->ReadShort(sh);
			p->edges[j] = sh;
		}

		file->ReadFloat(p->plane[0]);
		file->ReadFloat(p->plane[1]);
		file->ReadFloat(p->plane[2]);
		file->ReadFloat(p->plane[3]); // distance has reversed, don't call idPlane::SetDist
		file->ReadShort(bv[0]);
		file->ReadShort(bv[1]);
		file->ReadShort(bv[2]);
		file->ReadShort(bv[3]);
		file->ReadShort(bv[4]);
		file->ReadShort(bv[5]);
		p->bounds[0][0] = bv[0];
		p->bounds[0][1] = bv[1];
		p->bounds[0][2] = bv[2];
		p->bounds[1][0] = bv[3];
		p->bounds[1][1] = bv[4];
		p->bounds[1][2] = bv[5];
		file->ReadInt(index);
		// get material
		if (index >= 0 && index < materials.Num())
			p->material = declManager->FindMaterial(materials[index]);
		else
			p->material = NULL;
		p->contents = p->material ? p->material->GetContentFlags() : 0;
		p->checkcount = 0;
		// 0 0 0.0000305196 -0 0 -0.0000305157 32768 32768 // int int float float float float ushort ushort
		int v18, v19;
		file->ReadInt(v18);
		file->ReadInt(v19);
		p->contents |= v18;
		p->contents &= ~v19;

		file->ReadFloat(f);
		file->ReadFloat(f);
		file->ReadFloat(f);
		file->ReadFloat(f);
		file->ReadUnsignedShort(uh);
		file->ReadUnsignedShort(uh);
		
		// filter polygon into tree
		R_FilterPolygonIntoTree(model, model->node, NULL, p);

	}

	// end of polygons
	file->ReadUnsignedShort(uh); // -1/65535
	assert(i == -1)
}

void idCollisionModelManagerLocal::ParseBrushes_Binary(idFile *file, cm_model_t *model) {
	cm_brush_t *b;
	int i, numPlanes, j;
	int numBrushes, numBrushComponents;
	short bv[6];
	idStr token;

	file->ReadInt(numBrushes);
	model->brushBlock = (cm_brushBlock_t *) Mem_Alloc(sizeof(cm_brushBlock_t) + numBrushes);
	model->brushBlock->bytesRemaining = numBrushes;
	model->brushBlock->next = ((byte *) model->brushBlock) + sizeof(cm_brushBlock_t);

	file->ReadInt(numBrushComponents);

	for (i = 0; i < numBrushes; i++) {
		// parse brush
		file->ReadInt(numPlanes);
		b = AllocBrush(model, numPlanes);
		b->numPlanes = numPlanes;

		for (j = 0; j < b->numPlanes; j++) {
			file->ReadFloat(b->planes[j][0]);
			file->ReadFloat(b->planes[j][1]);
			file->ReadFloat(b->planes[j][2]);
			file->ReadFloat(b->planes[j][3]); // distance has reversed, don't call idPlane::SetDist
		}

		file->ReadShort(bv[0]);
		file->ReadShort(bv[1]);
		file->ReadShort(bv[2]);
		file->ReadShort(bv[3]);
		file->ReadShort(bv[4]);
		file->ReadShort(bv[5]);
		b->bounds[0][0] = bv[0];
		b->bounds[0][1] = bv[1];
		b->bounds[0][2] = bv[2];
		b->bounds[1][0] = bv[3];
		b->bounds[1][1] = bv[4];
		b->bounds[1][2] = bv[5];
		file->ReadString(token);

		b->contents = ContentsFromString(token);

		b->checkcount = 0;
		b->primitiveNum = 0;
		// filter brush into tree
		R_FilterBrushIntoTree(model, model->node, NULL, b);
	}

	// end of brushes
	file->ReadInt(i); // -1/0xFFFFFFFF
	assert(i == -1)
}

bool idCollisionModelManagerLocal::ParseCollisionModel_Binary(idFile *file) {
	cm_model_t *model;
	idStr token;

	if (numModels >= MAX_SUBMODELS) {
		common->Error("ParseCollisionModel_Binary:LoadModel: no free slots");
		return false;
	}

	model = AllocModel();
	models[numModels ] = model;
	numModels++;
	// parse the file
	file->ReadString(token);
	model->name = token;
	int unknown;
	file->ReadInt(unknown);
	idStrList materials;

	ParseVertices_Binary(file, model);

	ParseEdges_Binary(file, model);

	model->node = ParseNodes_Binary(file, model, NULL);

	ParseMaterials_Binary(file, materials);

	ParsePolygons_Binary(file, model, materials);

	ParseBrushes_Binary(file, model);

	model->SetWorld(!idStr::Icmpn(model->name, WORLD_MODEL_NAME, idStr::Length(WORLD_MODEL_NAME)));

	// calculate edge normals
	checkCount++;
	CalculateEdgeNormals(model, model->node);
	// get model bounds from brush and polygon bounds
	CM_GetNodeBounds(&model->bounds, model->node);
	// get model contents
	model->contents = CM_GetNodeContents(model->node);
	// total memory used by this model
	model->usedMemory = model->numVertices * sizeof(cm_vertex_t) +
	                    model->numEdges * sizeof(cm_edge_t) +
	                    model->polygonMemory +
	                    model->brushMemory +
	                    model->numNodes * sizeof(cm_node_t) +
	                    model->numPolygonRefs * sizeof(cm_polygonRef_t) +
	                    model->numBrushRefs * sizeof(cm_brushRef_t);

	return true;
}

bool idCollisionModelManagerLocal::LoadCollisionModelFile_Binary(const char *name, unsigned int mapFileCRC) {
	(void)mapFileCRC;
	idStr fileName(name);
	fileName.SetFileExtension(".cmb");

	idFile *file = fileSystem->OpenFileRead(fileName.c_str());
	if (!file) {
		return false;
	}

	//karin: 1. read fileID
	idStr token;
	file->ReadString(token);
	if (idStr::Icmp(token, CM_FILEID)) {
		common->Warning("LoadCollisionModelFile_Binary: %s is not an CM file.", fileName.c_str());
		fileSystem->CloseFile(file);
		return false;
	}

	//karin: 2. read version
	idStr version;
	file->ReadString(version);
	if (version != CM_FILEVERSION)
	{
		common->Warning("LoadCollisionModelFile_Binary: %s has version %s instead of %s", fileName.c_str(), version.c_str(), CM_FILEVERSION);
		fileSystem->CloseFile(file);
		return false;
	}

	unsigned int crc = 0;
	file->ReadUnsignedInt(crc);

	//karin: 3. parse data chunk
	while (file->Tell() < file->Length()) {
		if (!ParseCollisionModel_Binary(file)) {
			fileSystem->CloseFile(file);
			return false;
		}
	}

	common->Printf("LoadCollisionModelFile_Binary: binary cm file '%s' loaded\n", fileName.c_str());

#if 0 //karin: output ascii cm file
	fileName.SetFileExtension(".cm_ascii");
	WriteCollisionModelsToFile(fileName, 0, numModels, mapFileCRC);
#endif
	fileSystem->CloseFile(file);
	return true;
}

void idCollisionModelManagerLocal::ParseMaterials_Binary(idFile *file, idStrList &materials) {
	int num = 0;
	file->ReadInt(num);
	materials.SetNum(num);
	for (int i = 0; i < num; i++) {
		file->ReadString(materials[i]);
	}
}

#endif
