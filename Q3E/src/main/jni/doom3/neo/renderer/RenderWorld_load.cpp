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

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "tr_local.h"

#ifdef _RAVEN
#include "Model_local.h"
#endif

/*
================
idRenderWorldLocal::FreeWorld
================
*/
void idRenderWorldLocal::FreeWorld()
{
	int i;

	// this will free all the lightDefs and entityDefs
	FreeDefs();

	// free all the portals and check light/model references
	for (i = 0 ; i < numPortalAreas ; i++) {
		portalArea_t	*area;
		portal_t		*portal, *nextPortal;

		area = &portalAreas[i];

		for (portal = area->portals ; portal ; portal = nextPortal) {
			nextPortal = portal->next;
			delete portal->w;
			R_StaticFree(portal);
		}

		// there shouldn't be any remaining lightRefs or entityRefs
		if (area->lightRefs.areaNext != &area->lightRefs) {
			common->Error("FreeWorld: unexpected remaining lightRefs");
		}

		if (area->entityRefs.areaNext != &area->entityRefs) {
			common->Error("FreeWorld: unexpected remaining entityRefs");
		}
	}

	if (portalAreas) {
		R_StaticFree(portalAreas);
		portalAreas = NULL;
		numPortalAreas = 0;
		R_StaticFree(areaScreenRect);
		areaScreenRect = NULL;
	}

	if (doublePortals) {
		R_StaticFree(doublePortals);
		doublePortals = NULL;
		numInterAreaPortals = 0;
	}

	if (areaNodes) {
		R_StaticFree(areaNodes);
		areaNodes = NULL;
	}

	// free all the inline idRenderModels
	for (i = 0 ; i < localModels.Num() ; i++) {
		renderModelManager->RemoveModel(localModels[i]);
		delete localModels[i];
	}

	localModels.Clear();

	areaReferenceAllocator.Shutdown();
	interactionAllocator.Shutdown();
	areaNumRefAllocator.Shutdown();

	mapName = "<FREED>";
}

/*
================
idRenderWorldLocal::TouchWorldModels
================
*/
void idRenderWorldLocal::TouchWorldModels(void)
{
	int i;

	for (i = 0 ; i < localModels.Num() ; i++) {
		renderModelManager->CheckModel(localModels[i]->Name());
	}
}

/*
================
idRenderWorldLocal::ParseModel
================
*/
idRenderModel *idRenderWorldLocal::ParseModel(idLexer *src)
{
	idRenderModel	*model;
	idToken			token;
	int				i, j;
	srfTriangles_t	*tri;
	modelSurface_t	surf;

#ifdef _HUMANHEAD
#if DEATHWALK_AUTOLOAD
	src->ReadToken(&token);
	if(token != "{")
		src->UnreadToken(&token);
#else
	src->ExpectTokenString("{");
#endif
#else
	src->ExpectTokenString("{");
#endif

	// parse the name
	src->ExpectAnyToken(&token);

	model = renderModelManager->AllocModel();
	model->InitEmpty(token);

	int numSurfaces = src->ParseInt();

	if (numSurfaces < 0) {
		src->Error("R_ParseModel: bad numSurfaces");
	}

#ifdef _RAVEN // quake4 proc file
// jmarshall - quake 4 proc format
    if (!src->PeekTokenString("{") && !src->PeekTokenString("}"))
    {
        int sky = src->ParseInt();
		(static_cast<idRenderModelStatic *>(model))->sky = sky ? true : false;
    }
// jmarshall end
#endif

	for (i = 0 ; i < numSurfaces ; i++) {
		src->ExpectTokenString("{");

		src->ExpectAnyToken(&token);

		surf.shader = declManager->FindMaterial(token);

		((idMaterial *)surf.shader)->AddReference();

		tri = R_AllocStaticTriSurf();
		surf.geometry = tri;

		tri->numVerts = src->ParseInt();
		tri->numIndexes = src->ParseInt();

		R_AllocStaticTriSurfVerts(tri, tri->numVerts);

		for (j = 0 ; j < tri->numVerts ; j++) {
#ifdef _RAVEN // quake4 proc file
// jmarshall - quake 4 proc format
            //float	vec[8];
            //src->Parse1DMatrix( 8, vec );

            src->ExpectTokenString("(");

            tri->verts[j].xyz[0] = src->ParseFloat();
            tri->verts[j].xyz[1] = src->ParseFloat();
            tri->verts[j].xyz[2] = src->ParseFloat();
            tri->verts[j].st[0] = src->ParseFloat();
            tri->verts[j].st[1] = src->ParseFloat();
            tri->verts[j].normal[0] = src->ParseFloat();
            tri->verts[j].normal[1] = src->ParseFloat();
            tri->verts[j].normal[2] = src->ParseFloat();

            if (src->PeekTokenString(")"))
            {
                src->ExpectTokenString(")");
            }
            else
            {
                while (!src->PeekTokenString(")"))
                {
                    src->ParseFloat(); // jmarshall: not sure what the extra values here are for.
                }

                src->ExpectTokenString(")");
            }
// jmarshall end
#else
			float	vec[8];

			src->Parse1DMatrix(8, vec);

			tri->verts[j].xyz[0] = vec[0];
			tri->verts[j].xyz[1] = vec[1];
			tri->verts[j].xyz[2] = vec[2];
			tri->verts[j].st[0] = vec[3];
			tri->verts[j].st[1] = vec[4];
			tri->verts[j].normal[0] = vec[5];
			tri->verts[j].normal[1] = vec[6];
			tri->verts[j].normal[2] = vec[7];
#endif
		}

		R_AllocStaticTriSurfIndexes(tri, tri->numIndexes);

		for (j = 0 ; j < tri->numIndexes ; j++) {
			tri->indexes[j] = src->ParseInt();
		}

		src->ExpectTokenString("}");

		// add the completed surface to the model
		model->AddSurface(surf);
	}

	src->ExpectTokenString("}");

	model->FinishSurfaces();

	return model;
}

/*
================
idRenderWorldLocal::ParseShadowModel
================
*/
idRenderModel *idRenderWorldLocal::ParseShadowModel(idLexer *src)
{
	idRenderModel	*model;
	idToken			token;
	int				j;
	srfTriangles_t	*tri;
	modelSurface_t	surf;

	src->ExpectTokenString("{");

	// parse the name
	src->ExpectAnyToken(&token);

	model = renderModelManager->AllocModel();
	model->InitEmpty(token);

	surf.shader = tr.defaultMaterial;

	tri = R_AllocStaticTriSurf();
	surf.geometry = tri;

	tri->numVerts = src->ParseInt();
	tri->numShadowIndexesNoCaps = src->ParseInt();
	tri->numShadowIndexesNoFrontCaps = src->ParseInt();
	tri->numIndexes = src->ParseInt();
	tri->shadowCapPlaneBits = src->ParseInt();

	R_AllocStaticTriSurfShadowVerts(tri, tri->numVerts);
	tri->bounds.Clear();

	for (j = 0 ; j < tri->numVerts ; j++) {
		float	vec[8];

		src->Parse1DMatrix(3, vec);
		tri->shadowVertexes[j].xyz[0] = vec[0];
		tri->shadowVertexes[j].xyz[1] = vec[1];
		tri->shadowVertexes[j].xyz[2] = vec[2];
		tri->shadowVertexes[j].xyz[3] = 1;		// no homogenous value

		tri->bounds.AddPoint(tri->shadowVertexes[j].xyz.ToVec3());
	}

	R_AllocStaticTriSurfIndexes(tri, tri->numIndexes);

	for (j = 0 ; j < tri->numIndexes ; j++) {
		tri->indexes[j] = src->ParseInt();
	}

	// add the completed surface to the model
	model->AddSurface(surf);

	src->ExpectTokenString("}");

	// we do NOT do a model->FinishSurfaceces, because we don't need sil edges, planes, tangents, etc.
//	model->FinishSurfaces();
#ifdef _SHADOW_MAPPING
	tri->shadowIsPrelight = true;
#endif

	return model;
}

/*
================
idRenderWorldLocal::SetupAreaRefs
================
*/
void idRenderWorldLocal::SetupAreaRefs()
{
	int		i;

	connectedAreaNum = 0;

	for (i = 0 ; i < numPortalAreas ; i++) {
		portalAreas[i].areaNum = i;
		portalAreas[i].lightRefs.areaNext =
		        portalAreas[i].lightRefs.areaPrev =
		                &portalAreas[i].lightRefs;
		portalAreas[i].entityRefs.areaNext =
		        portalAreas[i].entityRefs.areaPrev =
		                &portalAreas[i].entityRefs;
	}
}

/*
================
idRenderWorldLocal::ParseInterAreaPortals
================
*/
void idRenderWorldLocal::ParseInterAreaPortals(idLexer *src)
{
	int i, j;

	src->ExpectTokenString("{");

	numPortalAreas = src->ParseInt();

	if (numPortalAreas < 0) {
		src->Error("R_ParseInterAreaPortals: bad numPortalAreas");
		return;
	}

#ifdef _HUMANHEAD
#if DEATHWALK_AUTOLOAD
	numPortalAreas += numAppendPortalAreas;
#endif
#endif
	portalAreas = (portalArea_t *)R_ClearedStaticAlloc(numPortalAreas * sizeof(portalAreas[0]));
	areaScreenRect = (idScreenRect *) R_ClearedStaticAlloc(numPortalAreas * sizeof(idScreenRect));

	// set the doubly linked lists
	SetupAreaRefs();

	numInterAreaPortals = src->ParseInt();

	if (numInterAreaPortals < 0) {
		src->Error("R_ParseInterAreaPortals: bad numInterAreaPortals");
		return;
	}

	doublePortals = (doublePortal_t *)R_ClearedStaticAlloc(numInterAreaPortals *
	                sizeof(doublePortals [0]));

	for (i = 0 ; i < numInterAreaPortals ; i++) {
		int		numPoints, a1, a2;
		idWinding	*w;
		portal_t	*p;

		numPoints = src->ParseInt();
		a1 = src->ParseInt();
		a2 = src->ParseInt();

		w = new idWinding(numPoints);
		w->SetNumPoints(numPoints);

		for (j = 0 ; j < numPoints ; j++) {
			src->Parse1DMatrix(3, (*w)[j].ToFloatPtr());
			// no texture coordinates
			(*w)[j][3] = 0;
			(*w)[j][4] = 0;
		}

#ifdef _RAVEN //k: quake4 extras
		idToken nextToken;
		if(src->ReadTokenOnLine(&nextToken))
		{
			if(nextToken == "(")
			{
				//k: ("_black" 123.00 456.00)
#if 0
				src->ReadToken(&nextToken); // fadeImage
				src->ParseFloat(); // distanceNear
				src->ParseFloat(); // distanceFar
				src->ExpectTokenString(")");
#else
				src->SkipUntilString(")");
#endif
			}
			else
				src->UnreadToken(&nextToken);
		}
#endif

		// add the portal to a1
		p = (portal_t *)R_ClearedStaticAlloc(sizeof(*p));
		p->intoArea = a2;
		p->doublePortal = &doublePortals[i];
		p->w = w;
		p->w->GetPlane(p->plane);

		p->next = portalAreas[a1].portals;
		portalAreas[a1].portals = p;

		doublePortals[i].portals[0] = p;

		// reverse it for a2
		p = (portal_t *)R_ClearedStaticAlloc(sizeof(*p));
		p->intoArea = a1;
		p->doublePortal = &doublePortals[i];
		p->w = w->Reverse();
		p->w->GetPlane(p->plane);

		p->next = portalAreas[a2].portals;
		portalAreas[a2].portals = p;

		doublePortals[i].portals[1] = p;
	}

	src->ExpectTokenString("}");
}

/*
================
idRenderWorldLocal::ParseNodes
================
*/
void idRenderWorldLocal::ParseNodes(idLexer *src)
{
	int			i;

	src->ExpectTokenString("{");

	numAreaNodes = src->ParseInt();

	if (numAreaNodes < 0) {
		src->Error("R_ParseNodes: bad numAreaNodes");
	}

	areaNodes = (areaNode_t *)R_ClearedStaticAlloc(numAreaNodes * sizeof(areaNodes[0]));

	for (i = 0 ; i < numAreaNodes ; i++) {
		areaNode_t	*node;

		node = &areaNodes[i];

		src->Parse1DMatrix(4, node->plane.ToFloatPtr());
		node->children[0] = src->ParseInt();
		node->children[1] = src->ParseInt();
	}

	src->ExpectTokenString("}");
}

/*
================
idRenderWorldLocal::CommonChildrenArea_r
================
*/
int idRenderWorldLocal::CommonChildrenArea_r(areaNode_t *node)
{
	int	nums[2];

	for (int i = 0 ; i < 2 ; i++) {
		if (node->children[i] <= 0) {
			nums[i] = -1 - node->children[i];
		} else {
			nums[i] = CommonChildrenArea_r(&areaNodes[ node->children[i] ]);
		}
	}

	// solid nodes will match any area
	if (nums[0] == AREANUM_SOLID) {
		nums[0] = nums[1];
	}

	if (nums[1] == AREANUM_SOLID) {
		nums[1] = nums[0];
	}

	int	common;

	if (nums[0] == nums[1]) {
		common = nums[0];
	} else {
		common = CHILDREN_HAVE_MULTIPLE_AREAS;
	}

	node->commonChildrenArea = common;

	return common;
}

/*
=================
idRenderWorldLocal::ClearWorld

Sets up for a single area world
=================
*/
void idRenderWorldLocal::ClearWorld()
{
	numPortalAreas = 1;
	portalAreas = (portalArea_t *)R_ClearedStaticAlloc(sizeof(portalAreas[0]));
	areaScreenRect = (idScreenRect *) R_ClearedStaticAlloc(sizeof(idScreenRect));

	SetupAreaRefs();

	// even though we only have a single area, create a node
	// that has both children pointing at it so we don't need to
	//
	areaNodes = (areaNode_t *)R_ClearedStaticAlloc(sizeof(areaNodes[0]));
	areaNodes[0].plane[3] = 1;
	areaNodes[0].children[0] = -1;
	areaNodes[0].children[1] = -1;
}

/*
=================
idRenderWorldLocal::FreeDefs

dump all the interactions
=================
*/
void idRenderWorldLocal::FreeDefs()
{
	int		i;

	generateAllInteractionsCalled = false;

	if (interactionTable) {
		R_StaticFree(interactionTable);
		interactionTable = NULL;
	}

	// free all lightDefs
	for (i = 0 ; i < lightDefs.Num() ; i++) {
		idRenderLightLocal	*light;

		light = lightDefs[i];

		if (light && light->world == this) {
			FreeLightDef(i);
			lightDefs[i] = NULL;
		}
	}

	// free all entityDefs
	for (i = 0 ; i < entityDefs.Num() ; i++) {
		idRenderEntityLocal	*mod;

		mod = entityDefs[i];

		if (mod && mod->world == this) {
			FreeEntityDef(i);
			entityDefs[i] = NULL;
		}
	}
}

/*
=================
idRenderWorldLocal::InitFromMap

A NULL or empty name will make a world without a map model, which
is still useful for displaying a bare model
=================
*/
bool idRenderWorldLocal::InitFromMap(const char *name)
{
	idLexer 		*src;
	idToken			token;
	idStr			filename;
	idRenderModel 	*lastModel;

	// if this is an empty world, initialize manually
	if (!name || !name[0]) {
		FreeWorld();
		mapName.Clear();
		ClearWorld();
		return true;
	}


	// load it
	filename = name;
	filename.SetFileExtension(PROC_FILE_EXT);

	// if we are reloading the same map, check the timestamp
	// and try to skip all the work
	ID_TIME_T currentTimeStamp;
	fileSystem->ReadFile(filename, NULL, &currentTimeStamp);

	if (name == mapName) {
		if (currentTimeStamp != FILE_NOT_FOUND_TIMESTAMP && currentTimeStamp == mapTimeStamp) {
			common->Printf("idRenderWorldLocal::InitFromMap: retaining existing map\n");
			FreeDefs();
			TouchWorldModels();
			AddWorldModelEntities();
			ClearPortalStates();
			return true;
		}

		common->Printf("idRenderWorldLocal::InitFromMap: timestamp has changed, reloading.\n");
	}

	FreeWorld();

	src = new idLexer(filename, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE);

	if (!src->IsLoaded()) {
		common->Printf("idRenderWorldLocal::InitFromMap: %s not found\n", filename.c_str());
		ClearWorld();
		return false;
	}


	mapName = name;
	mapTimeStamp = currentTimeStamp;

	// if we are writing a demo, archive the load command
	if (session->writeDemo) {
		WriteLoadMap();
	}

	if (!src->ReadToken(&token) || token.Icmp(PROC_FILE_ID)) {
		common->Printf("idRenderWorldLocal::InitFromMap: bad id '%s' instead of '%s'\n", token.c_str(), PROC_FILE_ID);
		delete src;
		return false;
	}

#ifdef _RAVEN // quake4 proc file
// jmarshall: quake 4 proc format
	if (!src->ReadToken(&token) || token.Icmp(PROC_FILEVERSION)) {
		common->Printf("idRenderWorldLocal::InitFromMap: bad version '%s' instead of '%s'\n", token.c_str(), PROC_FILEVERSION);
		delete src;
		return false;
	}

	// Map CRC, we aren't going to use it.
	src->ReadToken(&token);
// jmarshall end
#endif

#ifdef _HUMANHEAD
// HUMANHEAD pdm: Support for level appending
#if DEATHWALK_AUTOLOAD
	const char *dwMapName = session->GetDeathwalkMapName();
	bool bAppending = dwMapName && dwMapName[0] && session->ShouldAppendLevel() && idStr::Icmp(name, dwMapName);
	numAppendPortalAreas = bAppending ? 1 : 0;
	if(bAppending)
		common->Printf("[Harmattan]: Appending deathwalk proc: %s of %s\n", dwMapName, name);
	else
		common->Printf("[Harmattan]: no deathwalk map in %s\n", name);
#endif
#endif

	// parse the file
	while (1) {
		if (!src->ReadToken(&token)) {
			break;
		}

		if (token == "model") {
			lastModel = ParseModel(src);

			// add it to the model manager list
			renderModelManager->AddModel(lastModel);

			// save it in the list to free when clearing this map
			localModels.Append(lastModel);
			continue;
		}

		if (token == "shadowModel") {
			lastModel = ParseShadowModel(src);

			// add it to the model manager list
			renderModelManager->AddModel(lastModel);

			// save it in the list to free when clearing this map
			localModels.Append(lastModel);
			continue;
		}

		if (token == "interAreaPortals") {
			ParseInterAreaPortals(src);
			continue;
		}

		if (token == "nodes") {
			ParseNodes(src);
			continue;
		}

		src->Error("idRenderWorldLocal::InitFromMap: bad token \"%s\"", token.c_str());
	}

	delete src;

	// if it was a trivial map without any areas, create a single area
	if (!numPortalAreas) {
		ClearWorld();
	}

#ifdef _HUMANHEAD
// HUMANHEAD pdm: Support for level appending
#if DEATHWALK_AUTOLOAD
	src = 0;
	while (bAppending)
	{
		filename = dwMapName;
		filename.SetFileExtension(PROC_FILE_EXT);
		common->Printf("[Harmattan]: Appending deathwalk proc: %s\n", filename.c_str());
		src = new idLexer(filename, LEXFL_NOSTRINGCONCAT | LEXFL_NODOLLARPRECOMPILE);

		if (!src->IsLoaded()) {
			numAppendPortalAreas = 0;
			common->Warning("[Harmattan]: Appending deathwalk proc -> idRenderWorldLocal::InitFromMap: %s not found\n", filename.c_str());
			break;
		}

#if 0
		// if we are writing a demo, archive the load command
		if (session->writeDemo) {
			WriteLoadMap();
		}
#endif

		if (!src->ReadToken(&token) || token.Icmp(PROC_FILE_ID)) {
			numAppendPortalAreas = 0;
			common->Warning("[Harmattan]: Appending deathwalk proc -> idRenderWorldLocal::InitFromMap: bad id '%s' instead of '%s'\n", token.c_str(), PROC_FILE_ID);
			break;
		}

		// parse the file
		while (1) {
			if (!src->ReadToken(&token)) {
				break;
			}

			if (token == "model") {
				//k: replace `_areaXXX` name
				src->ExpectTokenString("{");
				idToken tmp;
				src->ExpectAnyToken(&tmp);
				if(!idStr::Icmpn(tmp, "_area", 5))
				{
					common->Printf("[Harmattan]: Appending deathwalk proc: parse area(%s) to portal area(%d)\n", tmp.c_str(), numPortalAreas - 1);
					int areaNo = atoi(tmp.c_str() + 5);
					tmp = va("_area%d", numPortalAreas - 1);
				}
				src->UnreadToken(&tmp);

				lastModel = ParseModel(src);

				// add it to the model manager list
				renderModelManager->AddModel(lastModel);

				// save it in the list to free when clearing this map
				localModels.Append(lastModel);
				continue;
			}

			if (token == "shadowModel") {
				lastModel = ParseShadowModel(src);

				// add it to the model manager list
				renderModelManager->AddModel(lastModel);

				// save it in the list to free when clearing this map
				localModels.Append(lastModel);
				continue;
			}

			if (token == "interAreaPortals") {
				src->ExpectTokenString("{");
				src->SkipBracedSection(false);
				continue;
			}

			if (token == "nodes") {
				src->ExpectTokenString("{");
				src->SkipBracedSection(false);
				continue;
			}

			src->Error("[Harmattan]: Appending deathwalk proc -> idRenderWorldLocal::InitFromMap: bad token \"%s\"", token.c_str());
		}

		//k: realloc nodes, add 6 nodes of deathwalk map to front, 6 nodes using deathwalk map bounds min/max points
		idRenderModel *model = renderModelManager->CheckModel(va("_area%i", numPortalAreas - 1));
		common->Printf("[Harmattan]: Appending deathwalk proc: find area model(%d %s)\n", numPortalAreas - 1, model ? model->Name() : "null");
		if(model)
		{
			int numAreaNodes_orig = numAreaNodes;
			numAreaNodes += 6;
			common->Printf("[Harmattan]: Appending deathwalk proc: push 6 nodes to list front(%d -> %d)\n", numAreaNodes_orig, numAreaNodes);

			areaNode_t *areaNodes_orig = areaNodes;
			areaNodes = (areaNode_t *)R_ClearedStaticAlloc(numAreaNodes * sizeof(areaNodes[0]));

			common->Printf("[Harmattan]: Appending deathwalk proc: copying area nodes(%d)\n", numAreaNodes_orig);
			for (int i = 0 ; i < numAreaNodes_orig ; i++) {
				areaNode_t	*node, *node_orig;

				//k: make positive child index + 6
				node = &areaNodes[i + 6];
				node_orig = &areaNodes_orig[i];

				node->plane = node_orig->plane;
				node->children[0] = node_orig->children[0];
				node->children[1] = node_orig->children[1];
				if(node->children[0] > 0)
					node->children[0] += 6;
				if(node->children[1] > 0)
					node->children[1] += 6;
			}

			R_StaticFree(areaNodes_orig); //k: free origin nodes

			//k: add deathwalk map 6 nodes to front, last area is deathwalk map
			idBounds bounds = model->Bounds();
			areaNode_t	*node;
			idVec3 dir;

			common->Printf("[Harmattan]: Appending deathwalk proc: area bounds(%s, %s)\n", bounds[0].ToString(), bounds[1].ToString());

			// 0: positive X direction, bounds min-X = next node(1), origin first node(now index is 6)
			node = &areaNodes[0];
			dir = idVec3(1, 0, 0);
			node->plane = idPlane(dir, bounds[0][0]);
			node->children[0] = 1;
			node->children[1] = 6;
			common->Printf("[Harmattan]: Appending deathwalk proc: \"/* node %d */ ( %s ) %d %d\"\n", 0, node->plane.ToString(), node->children[0], node->children[1]);

			// 1: positive Y direction, bounds min-Y = next node(2), origin first node(now index is 6)
			node = &areaNodes[1];
			dir = idVec3(0, 1, 0);
			node->plane = idPlane(dir, bounds[0][1]);
			node->children[0] = 2;
			node->children[1] = 6;
			common->Printf("[Harmattan]: Appending deathwalk proc: \"/* node %d */ ( %s ) %d %d\"\n", 1, node->plane.ToString(), node->children[0], node->children[1]);

			// 2: positive Z direction, bounds min-Z = next node(3), origin first node(now index is 6)
			node = &areaNodes[2];
			dir = idVec3(0, 0, 1);
			node->plane = idPlane(dir, bounds[0][2]);
			node->children[0] = 3;
			node->children[1] = 6;
			common->Printf("[Harmattan]: Appending deathwalk proc: \"/* node %d */ ( %s ) %d %d\"\n", 2, node->plane.ToString(), node->children[0], node->children[1]);

			// 3: negative X direction, bounds max-X = next node(4), origin first node(now index is 6)
			node = &areaNodes[3];
			dir = idVec3(-1, 0, 0);
			node->plane = idPlane(dir, -bounds[1][0]);
			node->children[0] = 4;
			node->children[1] = 6;
			common->Printf("[Harmattan]: Appending deathwalk proc: \"/* node %d */ ( %s ) %d %d\"\n", 3, node->plane.ToString(), node->children[0], node->children[1]);

			// 4: negative Y direction, bounds max-Y = next node(5), origin first node(now index is 6)
			node = &areaNodes[4];
			dir = idVec3(0, -1, 0);
			node->plane = idPlane(dir, -bounds[1][1]);
			node->children[0] = 5;
			node->children[1] = 6;
			common->Printf("[Harmattan]: Appending deathwalk proc: \"/* node %d */ ( %s ) %d %d\"\n", 4, node->plane.ToString(), node->children[0], node->children[1]);

			// 5: negative Z direction, bounds max-Z = deathwalk portal area index, origin first node(now index is 6)
			node = &areaNodes[5];
			dir = idVec3(0, 0, -1);
			node->plane = idPlane(dir, -bounds[1][2]);
			node->children[0] = -1 - (numPortalAreas - 1);
			node->children[1] = 6;
			common->Printf("[Harmattan]: Appending deathwalk proc: \"/* node %d */ ( %s ) %d %d\"\n", 5, node->plane.ToString(), node->children[0], node->children[1]);
		}

		common->Printf("[Harmattan]: Appending deathwalk proc finish\n");
		break;
	}
	delete src;
#endif
#endif

	// find the points where we can early-our of reference pushing into the BSP tree
	CommonChildrenArea_r(&areaNodes[0]);

	AddWorldModelEntities();
	ClearPortalStates();

	// done!
	return true;
}

/*
=====================
idRenderWorldLocal::ClearPortalStates
=====================
*/
void idRenderWorldLocal::ClearPortalStates()
{
	int		i, j;

	// all portals start off open
	for (i = 0 ; i < numInterAreaPortals ; i++) {
		doublePortals[i].blockingBits = PS_BLOCK_NONE;
	}

	// flood fill all area connections
	for (i = 0 ; i < numPortalAreas ; i++) {
		for (j = 0 ; j < NUM_PORTAL_ATTRIBUTES ; j++) {
			connectedAreaNum++;
			FloodConnectedAreas(&portalAreas[i], j);
		}
	}
}

/*
=====================
idRenderWorldLocal::AddWorldModelEntities
=====================
*/
void idRenderWorldLocal::AddWorldModelEntities()
{
	int		i;

	// add the world model for each portal area
	// we can't just call AddEntityDef, because that would place the references
	// based on the bounding box, rather than explicitly into the correct area
	for (i = 0 ; i < numPortalAreas ; i++) {
		idRenderEntityLocal	*def;
		int			index;

		def = new idRenderEntityLocal;

		// try and reuse a free spot
		index = entityDefs.FindNull();

		if (index == -1) {
			index = entityDefs.Append(def);
		} else {
			entityDefs[index] = def;
		}

		def->index = index;
		def->world = this;

		def->parms.hModel = renderModelManager->FindModel(va("_area%i", i));

		if (def->parms.hModel->IsDefaultModel() || !def->parms.hModel->IsStaticWorldModel()) {
			common->Error("idRenderWorldLocal::InitFromMap: bad area model lookup");
		}

		idRenderModel *hModel = def->parms.hModel;

		for (int j = 0; j < hModel->NumSurfaces(); j++) {
			const modelSurface_t *surf = hModel->Surface(j);

			if (surf->shader->GetName() == idStr("textures/smf/portal_sky")) {
				def->needsPortalSky = true;
			}
		}

		def->referenceBounds = def->parms.hModel->Bounds();

		def->parms.axis[0][0] = 1;
		def->parms.axis[1][1] = 1;
		def->parms.axis[2][2] = 1;

		R_AxisToModelMatrix(def->parms.axis, def->parms.origin, def->modelMatrix);

		// in case an explicit shader is used on the world, we don't
		// want it to have a 0 alpha or color
		def->parms.shaderParms[0] =
		        def->parms.shaderParms[1] =
		                def->parms.shaderParms[2] =
		                        def->parms.shaderParms[3] = 1;

		AddEntityRefToArea(def, &portalAreas[i]);
	}
}

/*
=====================
CheckAreaForPortalSky
=====================
*/
bool idRenderWorldLocal::CheckAreaForPortalSky(int areaNum)
{
	areaReference_t	*ref;

	assert(areaNum >= 0 && areaNum < numPortalAreas);

	for (ref = portalAreas[areaNum].entityRefs.areaNext; ref->entity; ref = ref->areaNext) {
		assert(ref->area == &portalAreas[areaNum]);

		if (ref->entity && ref->entity->needsPortalSky) {
			return true;
		}
	}

	return false;
}

#ifdef _RAVEN
bool idRenderWorldLocal::HasSkybox(int areaNum)
{
	assert(areaNum >= 0 && areaNum < numPortalAreas);
	if(r_skipSky.GetBool())
		return false;

	const idRenderModel *model = renderModelManager->CheckModel(va("_area%i", areaNum));
	return model ? static_cast<const idRenderModelStatic *>(model)->sky : false;
}
#endif
