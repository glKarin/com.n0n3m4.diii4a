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
#include "Model_local.h"

static const char *MD5_SnapshotName = "_MD5_Snapshot_";

/***********************************************************************

	idMD5Mesh

***********************************************************************/

static int c_numVerts = 0;
static int c_numWeights = 0;
static int c_numWeightJoints = 0;

typedef struct vertexWeight_s {
	int							vert;
	int							joint;
	idVec3						offset;
	float						jointWeight;
} vertexWeight_t;

/*
====================
idMD5Mesh::idMD5Mesh
====================
*/
idMD5Mesh::idMD5Mesh()
{
	scaledWeights	= NULL;
	weightIndex		= NULL;
	shader			= NULL;
	numTris			= 0;
	deformInfo		= NULL;
	surfaceNum		= 0;
#ifdef _SPLASHDAMAGE //karin: md5mesh version 11: flags
	flags.vertexColor = false;
	flags.noAnimate = false;
#endif
}

/*
====================
idMD5Mesh::~idMD5Mesh
====================
*/
idMD5Mesh::~idMD5Mesh()
{
	Mem_Free16(scaledWeights);
	Mem_Free16(weightIndex);

	if (deformInfo) {
		R_FreeDeformInfo(deformInfo);
		deformInfo = NULL;
	}
}

/*
====================
idMD5Mesh::ParseMesh
====================
*/
void idMD5Mesh::ParseMesh(idLexer &parser, int numJoints, const idJointMat *joints)
{
	idToken		token;
	idToken		name;
	int			num;
	int			count;
	int			jointnum;
	idStr		shaderName;
	int			i, j;
	idList<int>	tris;
	idList<int>	firstWeightForVertex;
	idList<int>	numWeightsForVertex;
	int			maxweight;
	idList<vertexWeight_t> tempWeights;

	parser.ExpectTokenString("{");

	//
	// parse name
	//
	if (parser.CheckTokenString("name")) {
		parser.ReadToken(&name);
#ifdef _SPLASHDAMAGE //karin: md5mesh version 11 mesh name
		meshName = name.c_str();
		
#endif
	}

	//
	// parse shader
	//
	parser.ExpectTokenString("shader");

	parser.ReadToken(&token);
	shaderName = token;

	shader = declManager->FindMaterial(shaderName);

#ifdef _SPLASHDAMAGE //karin: md5mesh version 11 mesh flags
	parser.ReadToken(&token);
	if(!idStr::Icmp(token, "flags"))
	{
		parser.ExpectTokenString("{");
		while(true) {
			if(!parser.ReadToken(&token))
			{
				break;
			}
			if(!idStr::Cmp(token, "}"))
				break;

			if(!idStr::Icmp(token, "vertexColor")) {
				flags.vertexColor = true;
				continue;
			}

			if(!idStr::Icmp(token, "noAnimate")) {
				flags.noAnimate = true;
				continue;
			}

			common->Warning("Unknown flag '%s' in mesh '%s'", token.c_str(), meshName.c_str());
		}
	}
	else
		parser.UnreadToken(&token);
#endif
	//
	// parse texture coordinates
	//
	parser.ExpectTokenString("numverts");
	count = parser.ParseInt();

	if (count < 0) {
		parser.Error("Invalid size: %s", token.c_str());
	}

	texCoords.SetNum(count);
	firstWeightForVertex.SetNum(count);
	numWeightsForVertex.SetNum(count);
#ifdef _SPLASHDAMAGE //karin: md5mesh version 11 vertex color
	vertColors.SetNum(count);
#endif

	numWeights = 0;
	maxweight = 0;

	for (i = 0; i < texCoords.Num(); i++) {
		parser.ExpectTokenString("vert");
		parser.ParseInt();

		parser.Parse1DMatrix(2, texCoords[ i ].ToFloatPtr());

		firstWeightForVertex[ i ]	= parser.ParseInt();
		numWeightsForVertex[ i ]	= parser.ParseInt();
#ifdef _SPLASHDAMAGE //karin: md5mesh version 11 vertex color
		parser.ReadToken(&token);
		if(!idStr::Cmp(token, "(")) // ( 1 1 1 1 )
		{
			vertColors[i].r = (byte)idMath::FtoiFast(parser.ParseFloat() * 255.0f);
			vertColors[i].g = (byte)idMath::FtoiFast(parser.ParseFloat() * 255.0f);
			vertColors[i].b = (byte)idMath::FtoiFast(parser.ParseFloat() * 255.0f);
			vertColors[i].a = (byte)idMath::FtoiFast(parser.ParseFloat() * 255.0f);
			parser.ExpectTokenString(")");
		}
		else
		{
			parser.UnreadToken(&token);
			vertColors[i].r = 255;
			vertColors[i].g = 255;
			vertColors[i].b = 255;
			vertColors[i].a = 255;
		}
#endif

		if (!numWeightsForVertex[ i ]) {
			parser.Error("Vertex without any joint weights.");
		}

		numWeights += numWeightsForVertex[ i ];

		if (numWeightsForVertex[ i ] + firstWeightForVertex[ i ] > maxweight) {
			maxweight = numWeightsForVertex[ i ] + firstWeightForVertex[ i ];
		}
	}

	//
	// parse tris
	//
	parser.ExpectTokenString("numtris");
	count = parser.ParseInt();

	if (count < 0) {
		parser.Error("Invalid size: %d", count);
	}

	tris.SetNum(count * 3);
	numTris = count;

	for (i = 0; i < count; i++) {
		parser.ExpectTokenString("tri");
		parser.ParseInt();

		tris[ i * 3 + 0 ] = parser.ParseInt();
		tris[ i * 3 + 1 ] = parser.ParseInt();
		tris[ i * 3 + 2 ] = parser.ParseInt();
	}

	//
	// parse weights
	//
	parser.ExpectTokenString("numweights");
	count = parser.ParseInt();

	if (count < 0) {
		parser.Error("Invalid size: %d", count);
	}

	if (maxweight > count) {
		parser.Warning("Vertices reference out of range weights in model (%d of %d weights).", maxweight, count);
	}

	tempWeights.SetNum(count);

	for (i = 0; i < count; i++) {
		parser.ExpectTokenString("weight");
		parser.ParseInt();

		jointnum = parser.ParseInt();

		if ((jointnum < 0) || (jointnum >= numJoints)) {
			parser.Error("Joint Index out of range(%d): %d", numJoints, jointnum);
		}

		tempWeights[ i ].joint			= jointnum;
		tempWeights[ i ].jointWeight	= parser.ParseFloat();

		parser.Parse1DMatrix(3, tempWeights[ i ].offset.ToFloatPtr());
	}

	// create pre-scaled weights and an index for the vertex/joint lookup
	scaledWeights = (idVec4 *) Mem_Alloc16(numWeights * sizeof(scaledWeights[0]));
	weightIndex = (int *) Mem_Alloc16(numWeights * 2 * sizeof(weightIndex[0]));
	memset(weightIndex, 0, numWeights * 2 * sizeof(weightIndex[0]));

	count = 0;

	for (i = 0; i < texCoords.Num(); i++) {
		num = firstWeightForVertex[i];

		for (j = 0; j < numWeightsForVertex[i]; j++, num++, count++) {
			scaledWeights[count].ToVec3() = tempWeights[num].offset * tempWeights[num].jointWeight;
			scaledWeights[count].w = tempWeights[num].jointWeight;
			weightIndex[count * 2 + 0] = tempWeights[num].joint * sizeof(idJointMat);
		}

		weightIndex[count * 2 - 1] = 1;
	}

	tempWeights.Clear();
	numWeightsForVertex.Clear();
	firstWeightForVertex.Clear();

	parser.ExpectTokenString("}");

	// update counters
	c_numVerts += texCoords.Num();
	c_numWeights += numWeights;
	c_numWeightJoints++;

	for (i = 0; i < numWeights; i++) {
		c_numWeightJoints += weightIndex[i*2+1];
	}

	//
	// build the information that will be common to all animations of this mesh:
	// silhouette edge connectivity and normal / tangent generation information
	//

#ifdef _DYNAMIC_ALLOC_STACK_OR_HEAP
	_DROID_ALLOC16_DEF(idDrawVert, verts, (texCoords.Num() * sizeof(idDrawVert)));
#else
	idDrawVert *verts = (idDrawVert *) _alloca16(texCoords.Num() * sizeof(idDrawVert));
#endif

	for (i = 0; i < texCoords.Num(); i++) {
		verts[i].Clear();
		verts[i].st = texCoords[i];
#ifdef _SPLASHDAMAGE //karin: md5mesh version 11 vertex color
		if(flags.vertexColor)
		{
			verts[i].color[0] = vertColors[i].r;
			verts[i].color[1] = vertColors[i].g;
			verts[i].color[2] = vertColors[i].b;
			verts[i].color[3] = vertColors[i].a;
		}
#endif
	}

	TransformVerts(verts, joints);
	deformInfo = R_BuildDeformInfo(texCoords.Num(), verts, tris.Num(), tris.Ptr(), shader->UseUnsmoothedTangents());

#ifdef _DYNAMIC_ALLOC_STACK_OR_HEAP
	_DROID_FREE(verts);
#endif
}

/*
====================
idMD5Mesh::TransformVerts
====================
*/
void idMD5Mesh::TransformVerts(idDrawVert *verts, const idJointMat *entJoints)
{
	SIMDProcessor->TransformVerts(verts, texCoords.Num(), entJoints, scaledWeights, weightIndex, numWeights);
}

/*
====================
idMD5Mesh::TransformScaledVerts

Special transform to make the mesh seem fat or skinny.  May be used for zombie deaths
====================
*/
void idMD5Mesh::TransformScaledVerts(idDrawVert *verts, const idJointMat *entJoints, float scale)
{
	idVec4 *scaledWeights = (idVec4 *) _alloca16(numWeights * sizeof(scaledWeights[0]));
	// Note: scaledWeights name is shadow of idMD5Mesh::scaledWeights, add this->. ETQW is used, but DOOM3/Quake4/Prey are not used
#if 0 // only scale offset
	idVec4 *sw = &this->scaledWeights[0], *tw = &scaledWeights[0];
	for (int i = 0; i < numWeights; i++, sw++, tw++) {
		tw->ToVec3() = sw->ToVec3() * scale;
		tw->ToFloatPtr()[3] = sw->ToFloatPtr()[3];
	}
#else
	SIMDProcessor->Mul(scaledWeights[0].ToFloatPtr(), scale, this->scaledWeights[0].ToFloatPtr(), numWeights * 4);
#endif
	SIMDProcessor->TransformVerts(verts, texCoords.Num(), entJoints, scaledWeights, weightIndex, numWeights);
}

/*
====================
idMD5Mesh::UpdateSurface
====================
*/
void idMD5Mesh::UpdateSurface(const struct renderEntity_s *ent, const idJointMat *entJoints, modelSurface_t *surf)
{
	int i, base;
	srfTriangles_t *tri;

	tr.pc.c_deformedSurfaces++;
	tr.pc.c_deformedVerts += deformInfo->numOutputVerts;
	tr.pc.c_deformedIndexes += deformInfo->numIndexes;

#ifdef _SPLASHDAMAGE
	surf->material = shader;
#else
	surf->shader = shader;
#endif

	if (surf->geometry) {
		// if the number of verts and indexes are the same we can re-use the triangle surface
		// the number of indexes must be the same to assure the correct amount of memory is allocated for the facePlanes
		if (surf->geometry->numVerts == deformInfo->numOutputVerts && surf->geometry->numIndexes == deformInfo->numIndexes) {
			R_FreeStaticTriSurfVertexCaches(surf->geometry);
		} else {
			R_FreeStaticTriSurf(surf->geometry);
			surf->geometry = R_AllocStaticTriSurf();
		}
	} else {
		surf->geometry = R_AllocStaticTriSurf();
	}

	tri = surf->geometry;

	// note that some of the data is references, and should not be freed
	tri->deformedSurface = true;
	tri->tangentsCalculated = false;
	tri->facePlanesCalculated = false;

	tri->numIndexes = deformInfo->numIndexes;
	tri->indexes = deformInfo->indexes;
	tri->silIndexes = deformInfo->silIndexes;
	tri->numMirroredVerts = deformInfo->numMirroredVerts;
	tri->mirroredVerts = deformInfo->mirroredVerts;
	tri->numDupVerts = deformInfo->numDupVerts;
	tri->dupVerts = deformInfo->dupVerts;
	tri->numSilEdges = deformInfo->numSilEdges;
	tri->silEdges = deformInfo->silEdges;
	tri->dominantTris = deformInfo->dominantTris;
	tri->numVerts = deformInfo->numOutputVerts;

	if (tri->verts == NULL) {
		R_AllocStaticTriSurfVerts(tri, tri->numVerts);

		for (i = 0; i < deformInfo->numSourceVerts; i++) {
			tri->verts[i].Clear();
			tri->verts[i].st = texCoords[i];
#ifdef _SPLASHDAMAGE //karin: md5mesh version 11 vertex color
			if(flags.vertexColor)
			{
				tri->verts[i].color[0] = vertColors[i].r;
				tri->verts[i].color[1] = vertColors[i].g;
				tri->verts[i].color[2] = vertColors[i].b;
				tri->verts[i].color[3] = vertColors[i].a;
			}
#endif
		}
	}

	if (ent->shaderParms[ SHADERPARM_MD5_SKINSCALE ] != 0.0f) {
#ifdef _SPLASHDAMAGE //karin: force to 1.0 if parm not 0. when character driving vehicle, the vehicle model scale will be not 0
		TransformScaledVerts(tri->verts, entJoints, 1.0);
#else
		TransformScaledVerts(tri->verts, entJoints, ent->shaderParms[ SHADERPARM_MD5_SKINSCALE ]);
#endif
	} else {
		TransformVerts(tri->verts, entJoints);
	}

	// replicate the mirror seam vertexes
	base = deformInfo->numOutputVerts - deformInfo->numMirroredVerts;

	for (i = 0; i < deformInfo->numMirroredVerts; i++) {
		tri->verts[base + i] = tri->verts[deformInfo->mirroredVerts[i]];
	}

	R_BoundTriSurf(tri);

	// If a surface is going to be have a lighting interaction generated, it will also have to call
	// R_DeriveTangents() to get normals, tangents, and face planes.  If it only
	// needs shadows generated, it will only have to generate face planes.  If it only
	// has ambient drawing, or is culled, no additional work will be necessary
	if (!r_useDeferredTangents.GetBool()) {
		// set face planes, vertex normals, tangents
		R_DeriveTangents(tri);
	}
}

/*
====================
idMD5Mesh::CalcBounds
====================
*/
idBounds idMD5Mesh::CalcBounds(const idJointMat *entJoints)
{
	idBounds	bounds;
#ifdef _DYNAMIC_ALLOC_STACK_OR_HEAP
	_DROID_ALLOC16_DEF(idDrawVert, verts, (texCoords.Num() * sizeof(idDrawVert)));
#else
	idDrawVert *verts = (idDrawVert *) _alloca16(texCoords.Num() * sizeof(idDrawVert));
#endif

	TransformVerts(verts, entJoints);

	SIMDProcessor->MinMax(bounds[0], bounds[1], verts, texCoords.Num());

#ifdef _DYNAMIC_ALLOC_STACK_OR_HEAP
	_DROID_FREE(verts);
#endif

	return bounds;
}

/*
====================
idMD5Mesh::NearestJoint
====================
*/
int idMD5Mesh::NearestJoint(int a, int b, int c) const
{
	int i, bestJoint, vertNum, weightVertNum;
	float bestWeight;

	// duplicated vertices might not have weights
	if (a >= 0 && a < texCoords.Num()) {
		vertNum = a;
	} else if (b >= 0 && b < texCoords.Num()) {
		vertNum = b;
	} else if (c >= 0 && c < texCoords.Num()) {
		vertNum = c;
	} else {
		// all vertices are duplicates which shouldn't happen
		return 0;
	}

	// find the first weight for this vertex
	weightVertNum = 0;

	for (i = 0; weightVertNum < vertNum; i++) {
		weightVertNum += weightIndex[i*2+1];
	}

	// get the joint for the largest weight
	bestWeight = scaledWeights[i].w;
	bestJoint = weightIndex[i*2+0] / sizeof(idJointMat);

	for (; weightIndex[i*2+1] == 0; i++) {
		if (scaledWeights[i].w > bestWeight) {
			bestWeight = scaledWeights[i].w;
			bestJoint = weightIndex[i*2+0] / sizeof(idJointMat);
		}
	}

	return bestJoint;
}

/*
====================
idMD5Mesh::NumVerts
====================
*/
int idMD5Mesh::NumVerts(void) const
{
	return texCoords.Num();
}

/*
====================
idMD5Mesh::NumTris
====================
*/
int	idMD5Mesh::NumTris(void) const
{
	return numTris;
}

/*
====================
idMD5Mesh::NumWeights
====================
*/
int	idMD5Mesh::NumWeights(void) const
{
	return numWeights;
}

/***********************************************************************

	idRenderModelMD5

***********************************************************************/

/*
====================
idRenderModelMD5::ParseJoint
====================
*/
void idRenderModelMD5::ParseJoint(idLexer &parser, idMD5Joint *joint, idJointQuat *defaultPose)
{
	idToken	token;
	int		num;

	//
	// parse name
	//
	parser.ReadToken(&token);
	joint->name = token;

	//
	// parse parent
	//
	num = parser.ParseInt();

	if (num < 0) {
		joint->parent = NULL;
	} else {
		if (num >= joints.Num() - 1) {
			parser.Error("Invalid parent for joint '%s'", joint->name.c_str());
		}

		joint->parent = &joints[ num ];
	}

	//
	// parse default pose
	//
	parser.Parse1DMatrix(3, defaultPose->t.ToFloatPtr());
	parser.Parse1DMatrix(3, defaultPose->q.ToFloatPtr());
	defaultPose->q.w = defaultPose->q.CalcW();
}

/*
====================
idRenderModelMD5::InitFromFile
====================
*/
void idRenderModelMD5::InitFromFile(const char *fileName)
{
	name = fileName;
	LoadModel();
}

/*
====================
idRenderModelMD5::LoadModel

used for initial loads, reloadModel, and reloading the data of purged models
Upon exit, the model will absolutely be valid, but possibly as a default model
====================
*/
void idRenderModelMD5::LoadModel()
{
	int			version;
	int			i;
	int			num;
	int			parentNum;
	idToken		token;
	idLexer		parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS);
	idJointQuat	*pose;
	idMD5Joint	*joint;
	idJointMat *poseMat3;

#if defined(_RAVEN) || defined(_HUMANHEAD) //k: for GUI view of dynamic model in idRenderWorld::GuiTrace
	this->staticModelInstance = NULL;
#endif

	if (!purged) {
		PurgeModel();
	}

	purged = false;

	if (!parser.LoadFile(name)) {
#ifdef _SPLASHDAMAGE //karin: and then try binary md5mesh
		if (!LoadMD5Binary())
#endif
		MakeDefaultModel();
		return;
	}

	parser.ExpectTokenString(MD5_VERSION_STRING);
	version = parser.ParseInt();

	if (version != MD5_VERSION) {
		parser.Error("Invalid version %d.  Should be version %d\n", version, MD5_VERSION);
	}

	//
	// skip commandline
	//
	parser.ExpectTokenString("commandline");
	parser.ReadToken(&token);

	// parse num joints
	parser.ExpectTokenString("numJoints");
	num  = parser.ParseInt();
	joints.SetGranularity(1);
	joints.SetNum(num);
	defaultPose.SetGranularity(1);
	defaultPose.SetNum(num);
	poseMat3 = (idJointMat *)_alloca16(num * sizeof(*poseMat3));

	// parse num meshes
	parser.ExpectTokenString("numMeshes");
	num = parser.ParseInt();

	if (num < 0) {
		parser.Error("Invalid size: %d", num);
	}

	meshes.SetGranularity(1);
	meshes.SetNum(num);

	//
	// parse joints
	//
	parser.ExpectTokenString("joints");
	parser.ExpectTokenString("{");
	pose = defaultPose.Ptr();
	joint = joints.Ptr();

	for (i = 0; i < joints.Num(); i++, joint++, pose++) {
		ParseJoint(parser, joint, pose);
		poseMat3[ i ].SetRotation(pose->q.ToMat3());
		poseMat3[ i ].SetTranslation(pose->t);

		if (joint->parent) {
			parentNum = joint->parent - joints.Ptr();
			pose->q = (poseMat3[ i ].ToMat3() * poseMat3[ parentNum ].ToMat3().Transpose()).ToQuat();
			pose->t = (poseMat3[ i ].ToVec3() - poseMat3[ parentNum ].ToVec3()) * poseMat3[ parentNum ].ToMat3().Transpose();
		}
	}

	parser.ExpectTokenString("}");

	for (i = 0; i < meshes.Num(); i++) {
		parser.ExpectTokenString("mesh");
		meshes[ i ].ParseMesh(parser, defaultPose.Num(), poseMat3);
	}

	//
	// calculate the bounds of the model
	//
	CalculateBounds(poseMat3);

	// set the timestamp for reloadmodels
	fileSystem->ReadFile(name, NULL, &timeStamp);
}

/*
==============
idRenderModelMD5::Print
==============
*/
void idRenderModelMD5::Print() const
{
	const idMD5Mesh	*mesh;
	int			i;

	common->Printf("%s\n", name.c_str());
	common->Printf("Dynamic model.\n");
	common->Printf("Generated smooth normals.\n");
	common->Printf("    verts  tris weights material\n");
	int	totalVerts = 0;
	int	totalTris = 0;
	int	totalWeights = 0;

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		totalVerts += mesh->NumVerts();
		totalTris += mesh->NumTris();
		totalWeights += mesh->NumWeights();
		common->Printf("%2i: %5i %5i %7i %s\n", i, mesh->NumVerts(), mesh->NumTris(), mesh->NumWeights(), mesh->shader->GetName());
	}

	common->Printf("-----\n");
	common->Printf("%4i verts.\n", totalVerts);
	common->Printf("%4i tris.\n", totalTris);
	common->Printf("%4i weights.\n", totalWeights);
	common->Printf("%4i joints.\n", joints.Num());
}

/*
==============
idRenderModelMD5::List
==============
*/
void idRenderModelMD5::List() const
{
	int			i;
	const idMD5Mesh	*mesh;
	int			totalTris = 0;
	int			totalVerts = 0;

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		totalTris += mesh->numTris;
		totalVerts += mesh->NumVerts();
	}

	common->Printf(" %4ik %3i %4i %4i %s(MD5)", Memory()/1024, meshes.Num(), totalVerts, totalTris, Name());

	if (defaulted) {
		common->Printf(" (DEFAULTED)");
	}

	common->Printf("\n");
}

/*
====================
idRenderModelMD5::CalculateBounds
====================
*/
void idRenderModelMD5::CalculateBounds(const idJointMat *entJoints)
{
	int			i;
	idMD5Mesh	*mesh;

	bounds.Clear();

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		bounds.AddBounds(mesh->CalcBounds(entJoints));
	}
}

/*
====================
idRenderModelMD5::Bounds

This calculates a rough bounds by using the joint radii without
transforming all the points
====================
*/
idBounds idRenderModelMD5::Bounds(const renderEntity_t *ent) const
{
#if 0

	// we can't calculate a rational bounds without an entity,
	// because joints could be positioned to deform it into an
	// arbitrarily large shape
	if (!ent) {
		common->Error("idRenderModelMD5::Bounds: called without entity");
	}

#endif

	if (!ent) {
		// this is the bounds for the reference pose
		return bounds;
	}

	return ent->bounds;
}

/*
====================
idRenderModelMD5::DrawJoints
====================
*/
void idRenderModelMD5::DrawJoints(const renderEntity_t *ent, const struct viewDef_s *view) const
{
	int					i;
	int					num;
	idVec3				pos;
	const idJointMat	*joint;
	const idMD5Joint	*md5Joint;
	int					parentNum;

	num = ent->numJoints;
	joint = ent->joints;
	md5Joint = joints.Ptr();

	for (i = 0; i < num; i++, joint++, md5Joint++) {
		pos = ent->origin + joint->ToVec3() * ent->axis;

		if (md5Joint->parent) {
			parentNum = md5Joint->parent - joints.Ptr();
			session->rw->DebugLine(colorWhite, ent->origin + ent->joints[ parentNum ].ToVec3() * ent->axis, pos);
		}

		session->rw->DebugLine(colorRed,	pos, pos + joint->ToMat3()[ 0 ] * 2.0f * ent->axis);
		session->rw->DebugLine(colorGreen,	pos, pos + joint->ToMat3()[ 1 ] * 2.0f * ent->axis);
		session->rw->DebugLine(colorBlue,	pos, pos + joint->ToMat3()[ 2 ] * 2.0f * ent->axis);
	}

	idBounds bounds;

	bounds.FromTransformedBounds(ent->bounds, vec3_zero, ent->axis);
	session->rw->DebugBounds(colorMagenta, bounds, ent->origin);

	if ((r_jointNameScale.GetFloat() != 0.0f) && (bounds.Expand(128.0f).ContainsPoint(view->renderView.vieworg - ent->origin))) {
		idVec3	offset(0, 0, r_jointNameOffset.GetFloat());
		float	scale;

		scale = r_jointNameScale.GetFloat();
		joint = ent->joints;
		num = ent->numJoints;

		for (i = 0; i < num; i++, joint++) {
			pos = ent->origin + joint->ToVec3() * ent->axis;
			session->rw->DrawText(joints[ i ].name, pos + offset, scale, colorWhite, view->renderView.viewaxis, 1);
		}
	}
}

/*
====================
idRenderModelMD5::InstantiateDynamicModel
====================
*/
#ifdef _RAVEN
idRenderModel *idRenderModelMD5::InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel, dword surfMask)
#else
idRenderModel *idRenderModelMD5::InstantiateDynamicModel(const struct renderEntity_s *ent, const struct viewDef_s *view, idRenderModel *cachedModel)
#endif
{
#ifdef _RAVEN
    (void)surfMask;
#endif
	int					i, surfaceNum;
	idMD5Mesh			*mesh;
	idRenderModelStatic	*staticModel;

#if defined(_RAVEN) || defined(_HUMANHEAD) //k: for GUI view of dynamic model in idRenderWorld::GuiTrace
	this->staticModelInstance = NULL;
#endif

	if (cachedModel && !r_useCachedDynamicModels.GetBool()) {
		delete cachedModel;
		cachedModel = NULL;
	}

	if (purged) {
		common->DWarning("model %s instantiated while purged", Name());
		LoadModel();
	}

	if (!ent->joints) {
		common->Printf("idRenderModelMD5::InstantiateDynamicModel: NULL joints on renderEntity for '%s'\n", Name());
		delete cachedModel;
		return NULL;
	} else if (ent->numJoints != joints.Num()) {
		common->Printf("idRenderModelMD5::InstantiateDynamicModel: renderEntity has different number of joints than model for '%s'\n", Name());
		delete cachedModel;
		return NULL;
	}

	tr.pc.c_generateMd5++;

	if (cachedModel) {
		assert(dynamic_cast<idRenderModelStatic *>(cachedModel) != NULL);
		assert(idStr::Icmp(cachedModel->Name(), MD5_SnapshotName) == 0);
		staticModel = static_cast<idRenderModelStatic *>(cachedModel);
	} else {
		staticModel = new idRenderModelStatic;
		staticModel->InitEmpty(MD5_SnapshotName);
	}

	staticModel->bounds.Clear();

	if (r_showSkel.GetInteger()) {
		if ((view != NULL) && (!r_skipSuppress.GetBool() || !ent->suppressSurfaceInViewID || (ent->suppressSurfaceInViewID != view->renderView.viewID))) {
			// only draw the skeleton
			DrawJoints(ent, view);
		}

		if (r_showSkel.GetInteger() > 1) {
			// turn off the model when showing the skeleton
			staticModel->InitEmpty(MD5_SnapshotName);
			return staticModel;
		}
	}

	// create all the surfaces
	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		// avoid deforming the surface if it will be a nodraw due to a skin remapping
		// FIXME: may have to still deform clipping hulls
		const idMaterial *shader = mesh->shader;

		shader = R_RemapShaderBySkin(shader, ent->customSkin, ent->customShader);

		if (!shader || (!shader->IsDrawn() && !shader->SurfaceCastsShadow())) {
			staticModel->DeleteSurfaceWithId(i);
			mesh->surfaceNum = -1;
			continue;
		}

		modelSurface_t *surf;

		if (staticModel->FindSurfaceWithId(i, surfaceNum)) {
			mesh->surfaceNum = surfaceNum;
			surf = &staticModel->surfaces[surfaceNum];
		} else {

			// Remove Overlays before adding new surfaces
			idRenderModelOverlay::RemoveOverlaySurfacesFromModel(staticModel);

			mesh->surfaceNum = staticModel->NumSurfaces();
			surf = &staticModel->surfaces.Alloc();
			surf->geometry = NULL;
#ifdef _SPLASHDAMAGE
			surf->material = NULL;
#else
			surf->shader = NULL;
#endif
			surf->id = i;
		}

		mesh->UpdateSurface(ent, ent->joints, surf);

		staticModel->bounds.AddPoint(surf->geometry->bounds[0]);
		staticModel->bounds.AddPoint(surf->geometry->bounds[1]);
	}

#if defined(_RAVEN) || defined(_HUMANHEAD) //k: for GUI view of dynamic model in idRenderWorld::GuiTrace
	this->staticModelInstance = staticModel;
#endif

#ifdef _RAVEN //k: show/hide surface
	if(surfaceShaderList.Num() != staticModel->surfaces.Num())
		surfaceShaderList.SetNum(staticModel->surfaces.Num());
	for(i = 0; i < staticModel->surfaces.Num(); i++)
	{
		const modelSurface_t *surf = &staticModel->surfaces[i];
		surfaceShaderList[i] = surf && surf->shader ? surf->shader->GetName() : "";
	}
#endif

	return staticModel;
}

/*
====================
idRenderModelMD5::IsDynamicModel
====================
*/
dynamicModel_t idRenderModelMD5::IsDynamicModel() const
{
	return DM_CACHED;
}

/*
====================
idRenderModelMD5::NumJoints
====================
*/
int idRenderModelMD5::NumJoints(void) const
{
	return joints.Num();
}

/*
====================
idRenderModelMD5::GetJoints
====================
*/
const idMD5Joint *idRenderModelMD5::GetJoints(void) const
{
	return joints.Ptr();
}

/*
====================
idRenderModelMD5::GetDefaultPose
====================
*/
const idJointQuat *idRenderModelMD5::GetDefaultPose(void) const
{
	return defaultPose.Ptr();
}

/*
====================
idRenderModelMD5::GetJointHandle
====================
*/
jointHandle_t idRenderModelMD5::GetJointHandle(const char *name) const
{
	const idMD5Joint *joint;
	int	i;

	joint = joints.Ptr();

	for (i = 0; i < joints.Num(); i++, joint++) {
		if (idStr::Icmp(joint->name.c_str(), name) == 0) {
			return (jointHandle_t)i;
		}
	}

	return INVALID_JOINT;
}

/*
=====================
idRenderModelMD5::GetJointName
=====================
*/
const char *idRenderModelMD5::GetJointName(jointHandle_t handle) const
{
	if ((handle < 0) || (handle >= joints.Num())) {
		return "<invalid joint>";
	}

	return joints[ handle ].name;
}

/*
====================
idRenderModelMD5::NearestJoint
====================
*/
int idRenderModelMD5::NearestJoint(int surfaceNum, int a, int b, int c) const
{
	int i;
	const idMD5Mesh *mesh;

	if (surfaceNum > meshes.Num()) {
		common->Error("idRenderModelMD5::NearestJoint: surfaceNum > meshes.Num()");
	}

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		if (mesh->surfaceNum == surfaceNum) {
			return mesh->NearestJoint(a, b, c);
		}
	}

	return 0;
}

/*
====================
idRenderModelMD5::TouchData

models that are already loaded at level start time
will still touch their materials to make sure they
are kept loaded
====================
*/
void idRenderModelMD5::TouchData()
{
	idMD5Mesh	*mesh;
	int			i;

#ifdef _SPLASHDAMAGE //karin: game not call idRenderModelManager::FindModel and only call TouchData if md5 model has loaded, although md5 model is purged, so reload md5 model here. see idGameEdit::ParseSpawnArgsToRenderEntity in Entity.cpp and idGameEdit::ANIM_CreateAnimFrame in anim/Anim_Blend.cpp, it caused joints num is different error.
	if (IsLevelLoadReferenced() && !IsLoaded() && IsReloadable()) {
		LoadModel();
	}
#endif

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		declManager->FindMaterial(mesh->shader->GetName());
	}
}

/*
===================
idRenderModelMD5::PurgeModel

frees all the data, but leaves the class around for dangling references,
which can regenerate the data with LoadModel()
===================
*/
void idRenderModelMD5::PurgeModel()
{
	purged = true;
	joints.Clear();
	defaultPose.Clear();
	meshes.Clear();
#if defined(_RAVEN) || defined(_HUMANHEAD) //k: md5 model ref def->dynamicModel, set to 0
	staticModelInstance = NULL;
#endif
#ifdef _SPLASHDAMAGE
	guiSurfaces.Clear();
#endif
}

/*
===================
idRenderModelMD5::Memory
===================
*/
int	idRenderModelMD5::Memory() const
{
	int		total, i;

	total = sizeof(*this);
	total += joints.MemoryUsed() + defaultPose.MemoryUsed() + meshes.MemoryUsed();

	// count up strings
	for (i = 0; i < joints.Num(); i++) {
		total += joints[i].name.DynamicMemoryUsed();
	}

	// count up meshes
	for (i = 0 ; i < meshes.Num() ; i++) {
		const idMD5Mesh *mesh = &meshes[i];

		total += mesh->texCoords.MemoryUsed() + mesh->numWeights * (sizeof(mesh->scaledWeights[0]) + sizeof(mesh->weightIndex[0]) * 2);

		// sum up deform info
		total += sizeof(mesh->deformInfo);
		total += R_DeformInfoMemoryUsed(mesh->deformInfo);
	}

	return total;
}

#ifdef _RAVEN //k: for ShowSurface/HideSurface, md5 model using mesh index as mask: 1 << index, name is shader material name
int idRenderModelMD5::GetSurfaceMask(const char *name) const
{
	int i;
	const idMD5Mesh			*mesh;

	if(!name || !name[0] || meshes.Num() == 0)
		return 0;

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++)
	{
		if(i > 31) //k: greate than int bits, it should be happened, but if happen, return 0 for do nothing
			break;
		const idMaterial *shader = mesh->shader;
		if(!shader)
			continue;
		if(!idStr::Icmp(name, shader->GetName()))
			return SUPPRESS_SURFACE_MASK(i);
	}
	return  0;
}
#endif

#ifdef _SPLASHDAMAGE //karin: md5b parsing
int idRenderModelMD5::FindSurfaceId( const char *surfaceName ) {
	int i;
	const idMD5Mesh			*mesh;

	if(!surfaceName || !surfaceName[0] || meshes.Num() == 0)
		return -1;

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++)
	{
#if 1
		if(!idStr::Icmp(surfaceName, mesh->meshName))
			return i;
#else
		if(!idStr::Icmp(surfaceName, mesh->meshName) && i < surfaces.Num())
			return surfaces[i].id;
#endif
	}
	return -1;
}

idBounds idRenderModelMD5::CalcMeshBounds( int meshIndex, const idJointMat *joints, const idVec3 &offset, const idMat3 &axis, bool useDefaultAnim ) {
	idBounds bounds = meshes[meshIndex].CalcBounds(joints);
	bounds.RotateSelf(axis);
	bounds.TranslateSelf(offset);
	return bounds;
}

extern void R_AllocMirroredVerts(deformInfo_t *deformInfo);
extern void R_AllocSilIndexes(deformInfo_t *deformInfo, int num);
extern void R_AllocSilEdges(deformInfo_t *deformInfo);
extern void R_AllocIndexes(deformInfo_t *deformInfo);

typedef struct md5meshBinaryJoint_s
{
    idStr boneName; // The name of this bone.
    int parentIndex; // The index of this bone’s parent.
    idVec3 pos; // The X/Y/Z component of this bone’s XYZ position. world coordinate
    idQuat orient; // The X/Y/Z component of this bone’s XYZ orentation quaternion. world coordinate
    idVec3 bindpos; // world
    idMat3 bindmat; // world
} md5meshBinaryJoint_t;

static void R_ConvertJointTransforms(idList<md5meshBinaryJoint_t> &joints)
{
    int i;
    md5meshBinaryJoint_t *md5Bone;

    for (i = 0, md5Bone = &joints[0]; i < joints.Num(); i++, md5Bone++)
    {
        idVec3 jointpos = md5Bone->pos;
        idMat3 jointaxis = md5Bone->orient.ToMat3();
        if(md5Bone->parentIndex >= 0)
        {
            md5meshBinaryJoint_t *parent = &joints[md5Bone->parentIndex];

            // convert to local coordinates
            idMat3 inv = parent->bindmat.Transpose();
            jointpos = (jointpos - parent->bindpos) * inv;
            jointaxis = jointaxis * inv;
        }

        if(md5Bone->parentIndex >= 0)
        {
            md5meshBinaryJoint_t *parent = &joints[md5Bone->parentIndex];

            md5Bone->bindmat = jointaxis * parent->bindmat;
            md5Bone->bindpos = parent->bindpos + jointpos * parent->bindmat;
        }
        else
        {
            md5Bone->bindmat = jointaxis;
            md5Bone->bindpos = jointpos;
        }
    }
}

/*
shader_v48 = a3->shader_v48;
			if ( (shader_v48->materialFlags & 0x10000) == 0 // BIT(16) # MF_NOSURFACEMERGE
			  && !shader_v48->entityGui
			  && shader_v48->deform == DFRM_NONE
			  && -3.0 != shader_v48->sort
			  && (shader_v48->surfaceFlags & 0x40) == 0 // BIT(6) # SURF_DISCRETE
			  && shader_v48->deform == DFRM_NONE
			  && v10->shader_v48 == shader_v48
			  && v10->noAnimate_bool_v196 == a3->noAnimate_bool_v196 )
			{
			  break;
			}
 */
int idMD5Mesh::FindBinaryVert(const idList<binaryVertGroup_t> &list) const
{
	const binaryVertGroup_t *group = list.Ptr();
	for(int i = 0; i < list.Num(); i++, group++)
	{
		if(group->shader != shader)
			continue;
		if(group->noAnimate != flags.noAnimate)
                continue;
            if (group->shader->GetEntityGui())
                continue;
/*            if (group->shader->Deform() != DFRM_NONE)
                continue;
            if (group->shader->GetSort() == SS_SUBVIEW) // - 3.0
                continue;
            if ((group->shader->GetSurfaceFlags() & SURF_DISCRETE) != 0)
                continue;
            if (group->shader->TestMaterialFlag(MF_NOSURFACEMERGE))
                continue;*/
		return i;
	}

	return -1;
}

/*
====================
idMD5Mesh::ReadBinary
====================
*/
bool idMD5Mesh::ReadBinary(idFile *file, int numJoints, const void *transforms, const idJointMat *joints, idList<binaryVertGroup_t> &retVerts)
{
	int			num;
	int			count;
	int			jointnum;
	idStr		shaderName;
	int			i, j;
	idList<glIndex_t>	tris;
	idList<int>	firstWeightForVertex;
	idList<int>	numWeightsForVertex;
	int			maxweight;
	idList<vertexWeight_t> tempWeights;
	unsigned short ush;
	int numRawVertex;
	unsigned char uch;
	unsigned char weight;
	bool b;
	bool vertexRigidFlag;
	int numSilEdges;
	bool bool_v196;
	bool bool_v198;
	int numSilIndex; // v72
	int numSourceVerts; // v120
	int numOutputVerts; // v124; // numVertex
	int numMirroredVerts; // v128
	float biTangentSign;
	const md5meshBinaryJoint_t *trans = (const md5meshBinaryJoint_t *)transforms;
	binaryVertGroup_t *group = NULL;

	//
	// parse name
	//
	file->ReadString(meshName);

	file->ReadInt(i); // index0_v36
	//if(i) flags |= MD5MF_VERTEX_COLOR;

	file->ReadInt(i); // index1_v40

	file->ReadInt(i); // v44
	file->ReadInt(numRawVertex); // v52
	file->ReadBool(bool_v196); // v196
	if(bool_v196) flags.noAnimate = true;
	file->ReadBool(bool_v198);
	file->ReadBool(vertexRigidFlag);

	//
	// parse shader
	//
	file->ReadString(shaderName);
	shader = declManager->FindMaterial(shaderName);

	file->ReadInt(numSilIndex);
	file->ReadInt(i); // v76

	//
	// parse tris
	//
	file->ReadInt(count); // numTris_v80
	if (count < 0) {
		common->Warning("Invalid size: %d", count);
		return false;
	}

	tris.SetNum(count); // is num of indices
	numTris = count / 3; // is num of triangles

	for (i = 0; i < tris.Num(); i++) {
		file->ReadUnsignedShort(ush);
		tris[ i ] = ush;
	}

	file->ReadInt(numSilEdges); // numSilEdges_v92
	idList<silEdge_t> silEdges;
	silEdges.SetNum(numSilEdges);
	for (i = 0; i < numSilEdges; i++) {
		silEdge_t &edge = silEdges[ i ];
		file->ReadUnsignedShort(ush);
		edge.p1 = ush;
		file->ReadUnsignedShort(ush);
		edge.p2 = ush;
		file->ReadUnsignedShort(ush);
		edge.v1 = ush;
		file->ReadUnsignedShort(ush);
		edge.v2 = ush;
	}
	idList<glIndex_t> silIndexes;
	if (!bool_v198) {
		silIndexes.SetNum(numSilIndex);
		for (i = 0; i < numSilIndex; i++) {
			file->ReadUnsignedShort(ush);
			silIndexes[i] = ush;
		}
	}

	//karin: numOutputVerts = numSourceVerts + numMirroredVerts
	file->ReadInt(numSourceVerts);
	file->ReadInt(numOutputVerts);
	file->ReadInt(numMirroredVerts);

	idList<int> mirroredVerts;
	mirroredVerts.SetNum(numMirroredVerts);
	for (i = 0; i < numMirroredVerts; i++)
		file->ReadInt(mirroredVerts[i]);

	file->ReadInt(i); // v136
	file->ReadInt(i); // v148

	file->ReadInt(count);
	idList<byte> jointTables;
	jointTables.SetNum(count);
	for (i = 0; i < jointTables.Num(); i++)
		file->ReadUnsignedChar(jointTables[i]);

	//
	// parse texture coordinates
	//
	count = numOutputVerts;

	if (count < 0) {
		common->Warning("Invalid size: %d", count);
		return false;
	}
	else if(count > 0)
	{
		group = &retVerts.Alloc();
		group->shader = shader;
		group->noAnimate = bool_v196;
		group->verts.SetNum(count);
	}

	texCoords.SetNum(count);
	firstWeightForVertex.SetNum(count);
	numWeightsForVertex.SetNum(count);
	vertColors.SetNum(count);

	numWeights = 0;
	maxweight = 0;

	idList<idDrawVert> verts;
	verts.SetNum(texCoords.Num());
	for (i = 0; i < texCoords.Num(); i++) {
		idDrawVert &vert = verts[ i ];
		vert.Clear();
		file->ReadVec3(vert.xyz);

		file->ReadVec2(texCoords[ i ]);
		vert.st = texCoords[i];

		file->ReadVec3(vert.normal);
		file->ReadVec3(vert.tangents[0]);
		file->ReadFloat(biTangentSign);
		vert.SetBiTangent(biTangentSign);

		file->ReadUnsignedChar(vertColors[i].r);
		file->ReadUnsignedChar(vertColors[i].g);
		file->ReadUnsignedChar(vertColors[i].b);
		file->ReadUnsignedChar(vertColors[i].a);

		vert.color[0] = vertColors[i].r;
		vert.color[1] = vertColors[i].g;
		vert.color[2] = vertColors[i].b;
		vert.color[3] = vertColors[i].a;

		binaryVert_t &bin = group->verts[i];
		bin.st = texCoords[i];
		bin.color[0] = vertColors[i].r;
		bin.color[1] = vertColors[i].g;
		bin.color[2] = vertColors[i].b;
		bin.color[3] = vertColors[i].a;
		bin.num = 0;
	}

	//
	// parse weights
	//

	if (vertexRigidFlag)
	{
		count = numOutputVerts;
		tempWeights.SetNum(numOutputVerts);
		for (i = 0; i < numOutputVerts; i++)
		{
			file->ReadUnsignedChar(uch);
			jointnum = jointTables[uch / 3];
			if ((jointnum < 0) || (jointnum >= numJoints)) {
				common->Warning("Joint Index out of range(%d): %d", numJoints, jointnum);
				return false;
			}

			tempWeights[ i ].joint			= jointnum;
			tempWeights[ i ].jointWeight	= 1.0f;

			//karin: calc joint relative offset by global position
			trans[jointnum].bindmat.ProjectVector(verts[i].xyz - trans[jointnum].bindpos, tempWeights[ i ].offset);

			firstWeightForVertex[ i ]	= i;
			numWeightsForVertex[ i ]	= 1;

			numWeights += numWeightsForVertex[ i ];

			if (numWeightsForVertex[ i ] + firstWeightForVertex[ i ] > maxweight) {
				maxweight = numWeightsForVertex[ i ] + firstWeightForVertex[ i ];
			}
			
			binaryVert_t &bin = group->verts[i];
			bin.joints[0] = jointnum;
			bin.weights[0] = 1.0f;
			bin.offsets[0] = tempWeights[i].offset;
			bin.num = 1;
		}
	}
	else
	{
		count = 0;
		for (i = 0; i < numOutputVerts; i++)
		{
			firstWeightForVertex[ i ]	= tempWeights.Num();
			numWeightsForVertex[ i ]	= 0;
			
			binaryVert_t &bin = group->verts[i];
			bin.num = 0;

			for (j = 0; j < 4; j++)
			{
				file->ReadUnsignedChar(uch);
				file->ReadUnsignedChar(weight);
				if (weight > 0)
				{
					jointnum = jointTables[uch / 3];
					if ((jointnum < 0) || (jointnum >= numJoints)) {
						common->Warning("Joint Index out of range(%d): %d", numJoints, jointnum);
						return false;
					}

					vertexWeight_t w;
					w.joint			= jointnum;
					w.jointWeight	= (float)weight / 255.0f;

					//karin: calc joint relative offset by global position
					trans[jointnum].bindmat.ProjectVector(verts[i].xyz - trans[jointnum].bindpos, w.offset);
					tempWeights.Append(w);
					numWeightsForVertex[ i ]++;
					count++;

					bin.joints[bin.num] = jointnum;
					bin.weights[bin.num] = w.jointWeight;
					bin.offsets[bin.num] = w.offset;
					bin.num++;
				}
			}

			if (!numWeightsForVertex[ i ]) {
				common->Warning("Vertex without any joint weights.");
				return false;
			}

			numWeights += numWeightsForVertex[ i ];

			if (numWeightsForVertex[ i ] + firstWeightForVertex[ i ] > maxweight) {
				maxweight = numWeightsForVertex[ i ] + firstWeightForVertex[ i ];
			}
		}
	}

	file->ReadBool(b); // bool_v216
	file->ReadInt(i);

	//Sys_Printf("xxx %s|%s: v=%d t=%d | %d a=%d | %d %d %d | %d %d %d | %d\n", meshName.c_str(),shaderName.c_str(), numSilIndex, numTris, bool_v198, flags & MD5MF_NO_ANIMATE, numSourceVerts, numOutputVerts, numMirroredVerts, maxweight, numWeights, count, numSilEdges);
	
	//karin: find other mesh verts if no verts
	if(numOutputVerts == 0)
	{
		int groupIndex = FindBinaryVert(retVerts);
		if(groupIndex < 0)
		{
			common->Warning("Couldn't find mesh group: %s", meshName.c_str());
			return false;
		}
		const idList<binaryVert_t> &groupVerts = retVerts[groupIndex].verts;
		/*
		printf("fff %d\n", index);
		for (i = 0; i < tris.Num(); i++) {
			if(tris[i] >= groupVerts.Num())
			printf("kkk %d ", tris[i]);
		}
		printf("\n");
		*/

		//karin: filter verts of used only, but not need to do it in ETQW original, because it using GPU skinning
		idList<const binaryVert_t *> used;
		idList<int> mapBefore;
		idList<int> mapAfter;
		for(int j = 0; j < tris.Num(); j++)
		{
			int &idx = tris[j];
			int oldIndex = mapBefore.FindIndex(idx);
			if(oldIndex == -1)
			{
				mapBefore.Append(idx);
				int index = used.Append(&groupVerts[idx]);
				idx = mapAfter.Append(index);
			}
			else
			{
				idx = mapAfter[oldIndex];
			}
		}

		texCoords.SetNum(used.Num());
		vertColors.SetNum(texCoords.Num());
		firstWeightForVertex.SetNum(texCoords.Num());
		numWeightsForVertex.SetNum(texCoords.Num());
		verts.SetNum(texCoords.Num());

		numWeights = 0;
		idDrawVert *dv = verts.Ptr();
		for(int j = 0; j < used.Num(); j++, dv++)
		{
			const binaryVert_t *bin = used[j];
			texCoords[j] = bin->st;
			vertColors[j].r = bin->color[0];
			vertColors[j].g = bin->color[1];
			vertColors[j].b = bin->color[2];
			vertColors[j].a = bin->color[3];
			numWeights += bin->num;
			firstWeightForVertex[ j ]	= tempWeights.Num();
			numWeightsForVertex[ j ]	= bin->num;
			for(int k = 0; k < bin->num; k++)
			{
				vertexWeight_t w;
				w.joint			= bin->joints[k];
				w.jointWeight	= bin->weights[k];
				w.offset		= bin->offsets[k];
				tempWeights.Append(w);
			}

			dv->Clear();
			dv->st = texCoords[j];
			dv->color[0] = vertColors[j].r;
			dv->color[1] = vertColors[j].g;
			dv->color[2] = vertColors[j].b;
			dv->color[3] = vertColors[j].a;
		}
		count = numWeights;
	}

	if (maxweight > count) {
		common->Warning("Vertices reference out of range weights in model (%d of %d weights).", maxweight, count);
	}
	/*
	for (i = 0; i < numSilIndex; i++) {
			printf(" %d", silIndexes[i]);
	}
	printf("\naaa\n");
	for (i = 0; i < numMirroredVerts; i++) {
			printf(" %d", mirroredVerts[i]);
	}
	printf("\n");
	for (i = 0; i < tempWeights.Num(); i++) {
			printf("%5d:  %d %f | %f %f %f\n", i,tempWeights[i].joint, tempWeights[i].jointWeight, tempWeights[i].offset.x, tempWeights[i].offset.y, tempWeights[i].offset.z);
	}
	*/


	// create pre-scaled weights and an index for the vertex/joint lookup
	scaledWeights = (idVec4 *) Mem_Alloc16(numWeights * sizeof(scaledWeights[0]));
	weightIndex = (int *) Mem_Alloc16(numWeights * 2 * sizeof(weightIndex[0]));
	memset(weightIndex, 0, numWeights * 2 * sizeof(weightIndex[0]));

	count = 0;

	for (i = 0; i < texCoords.Num(); i++) {
		num = firstWeightForVertex[i];

		for (j = 0; j < numWeightsForVertex[i]; j++, num++, count++) {
			scaledWeights[count].ToVec3() = tempWeights[num].offset * tempWeights[num].jointWeight;
			scaledWeights[count].w = tempWeights[num].jointWeight;
			weightIndex[count * 2 + 0] = tempWeights[num].joint * sizeof(idJointMat);
		}

		weightIndex[count * 2 - 1] = 1;
	}

	tempWeights.Clear();
	numWeightsForVertex.Clear();
	firstWeightForVertex.Clear();

	// update counters
	c_numVerts += texCoords.Num();
	c_numWeights += numWeights;
	c_numWeightJoints++;

	for (i = 0; i < numWeights; i++) {
		c_numWeightJoints += weightIndex[i*2+1];
	}

	//
	// build the information that will be common to all animations of this mesh:
	// silhouette edge connectivity and normal / tangent generation information
	//

#if 0 // TODO: using binary instead programming calc
	deformInfo = (deformInfo_t *)R_ClearedStaticAlloc(sizeof(*deformInfo));

	deformInfo->numSourceVerts = numSourceVerts;
	deformInfo->numOutputVerts = numOutputVerts;

	deformInfo->numIndexes = numTris;
	R_AllocIndexes(deformInfo);
	memcpy(deformInfo->indexes, tris.Ptr(), sizeof(*deformInfo->indexes) * deformInfo->numIndexes);

	if (silIndexes.Num() > 0) {
		R_AllocSilIndexes(deformInfo, silIndexes.Num());
		memcpy(deformInfo->silIndexes, silIndexes.Ptr(), sizeof(*deformInfo->silIndexes) * silIndexes.Num());
	}
	else
		deformInfo->silIndexes = NULL;

	deformInfo->numSilEdges = numSilEdges;
	R_AllocSilEdges(deformInfo);
	memcpy(deformInfo->silEdges, silEdges.Ptr(), sizeof(*deformInfo->silEdges) * deformInfo->numSilEdges);

	deformInfo->dominantTris = NULL;

	deformInfo->numMirroredVerts = numMirroredVerts;
	R_AllocMirroredVerts(deformInfo);
	memcpy(deformInfo->mirroredVerts, mirroredVerts.Ptr(), sizeof(*deformInfo->mirroredVerts) * deformInfo->numMirroredVerts);

	deformInfo->numDupVerts = 0;
	deformInfo->dupVerts = NULL;
#else
	TransformVerts(verts.Ptr(), joints);
	deformInfo = R_BuildDeformInfo(texCoords.Num(), verts.Ptr(), tris.Num(), tris.Ptr(), shader->UseUnsmoothedTangents());
#endif

	return true;
}

// only skip
bool idRenderModelMD5::SkipLOD(idFile *file) const
{
	int			num;
	int			count;
	idStr		shaderName;
	int			i;
	bool vertexRigidFlag;
	int numSilEdges;
	bool bool_v198;
	int numSilIndex;
	int numSourceVerts;
	int numOutputVerts;
	int numMirroredVerts;
	idStr meshName;

	file->ReadInt(num);
	for(i = 0; i < num; i++)
	{
		//
		// parse name
		//
		file->ReadString(meshName);

		file->Seek(4 + 4 + 4 + 4 + 1, FS_SEEK_CUR);
		file->ReadBool(bool_v198);
		file->ReadBool(vertexRigidFlag);

		//
		// parse shader
		//
		file->ReadString(shaderName);

		file->ReadInt(numSilIndex);
		file->Seek(4, FS_SEEK_CUR);

		//
		// parse tris
		//
		file->ReadInt(count); // numTris_v80
		if (count < 0) {
			common->Warning("Invalid size: %d", count);
			return false;
		}

		file->Seek(count * 2, FS_SEEK_CUR);

		file->ReadInt(numSilEdges); // numSilEdges_v92
		file->Seek(numSilEdges * 4 * 2, FS_SEEK_CUR);
		if (!bool_v198) {
			file->Seek(numSilIndex * 2, FS_SEEK_CUR);
		}

		//karin: numOutputVerts = numSourceVerts + numMirroredVerts
		file->ReadInt(numSourceVerts);
		file->ReadInt(numOutputVerts);
		file->ReadInt(numMirroredVerts);

		file->Seek(numMirroredVerts * 4, FS_SEEK_CUR);

		file->Seek(4 + 4, FS_SEEK_CUR);

		file->ReadInt(count);
		file->Seek(count, FS_SEEK_CUR);

		//
		// parse texture coordinates
		//
		if (count < 0) {
			common->Warning("Invalid size: %d", count);
			return false;
		}

		file->Seek(numOutputVerts * ((3 + 2 + 3 + 3 + 1) * 4 + 4), FS_SEEK_CUR);

		//
		// parse weights
		//
		if (vertexRigidFlag)
		{
			file->Seek(numOutputVerts, FS_SEEK_CUR);
		}
		else
		{
			file->Seek(numOutputVerts * 4 * 2, FS_SEEK_CUR);
		}

		file->Seek(1 + 4, FS_SEEK_CUR);
	}
	return true;
}

/*
====================
idRenderModelMD5::ParseJoint_Binary
====================
*/
bool idRenderModelMD5::ParseJoint_Binary(idFile *file, idMD5Joint *joint, idJointQuat *defaultPose, idJointMat *poseMat3)
{
	idStr	token;
	int		num;

	//
	// parse name
	//
	file->ReadString(joint->name);

	//
	// parse parent
	//
	file->ReadInt(num);

	if (num < 0) {
		joint->parent = NULL;
	} else {
		if (num >= joints.Num() - 1) {
			common->Warning("Invalid parent for joint '%s'", joint->name.c_str());
			return false;
		}

		joint->parent = &joints[ num ];
	}

	//
	// parse default pose
	//
	file->ReadFloat(defaultPose->q[0]);
	file->ReadFloat(defaultPose->q[1]);
	file->ReadFloat(defaultPose->q[2]);
	file->ReadFloat(defaultPose->q[3]);
	//defaultPose->q.w = defaultPose->q.CalcW();
	file->ReadFloat(defaultPose->t[0]);
	file->ReadFloat(defaultPose->t[1]);
	file->ReadFloat(defaultPose->t[2]);
	file->ReadFloat(defaultPose->w);
	file->ReadFloatArray(poseMat3->ToFloatPtr(), 12);

	return true;
}

bool idRenderModelMD5::LoadMD5Binary(void) {
	int			version;
	int			numLod;
	int			i;
	int			num;
	int			parentNum;
	idToken		token;
	idFile		*file;
	idJointQuat	*pose;
	idMD5Joint	*joint;
	idJointMat *poseMat3;

	idStr binPath = "generated/md5binary";
	binPath.AppendPath(name);
	binPath.SetFileExtension(".md5b");

	file = fileSystem->OpenFileRead(binPath);
	if (!file)
		return false;

	file->ReadInt(version);

	if (version != MD5B_VERSION) {
		fileSystem->CloseFile(file);
		common->Warning("Wrong version loading MD5Binary: %s (%i, expected %i)", Name(), version, MD5B_VERSION);
		return false;
	}

	if (!purged) {
		PurgeModel();
	}

	purged = false;

	file->ReadInt(numLod);
	if (numLod <= 0)
	{
		fileSystem->CloseFile(file);
		common->Error("Invalid LOD size: %d", numLod);
		return false;
	}

	// parse num joints
	file->ReadInt(num);
	joints.SetGranularity(1);
	joints.SetNum(num);
	defaultPose.SetGranularity(1);
	defaultPose.SetNum(num);
	poseMat3 = (idJointMat *)_alloca16(num * sizeof(*poseMat3));

	//
	// parse joints
	//
	pose = defaultPose.Ptr();
	joint = joints.Ptr();
	/**
	 * //karin: NOTE
	 * ASCII: joints transform are global/absolute in md5mesh
	 * binary: joints transform are local/relative in md5mesh
	 * binary joint format is Qx Qy Qz Qw Tx Ty Tz w global_mat3x4
	 * so we don't need calc extras like parsing ASCII joints
	 */
	idList<md5meshBinaryJoint_t> md5Bones;
	md5Bones.SetNum(joints.Num());
    md5meshBinaryJoint_t *md5Bone;
	md5Bone = &md5Bones[0];

	for (i = 0; i < joints.Num(); i++, joint++, pose++, md5Bone++) {
		if(!ParseJoint_Binary(file, joint, pose, &poseMat3[i]))
		{
			fileSystem->CloseFile(file);
			return false;
		}

		md5Bone->boneName = joint->name;
		md5Bone->parentIndex = joint->parent ? joint->parent - joints.Ptr() : -1;

		if (joint->parent) {
			parentNum = joint->parent - joints.Ptr();

            idVec3 rotated;
            idQuat quat;

			md5Bone->pos = pose->t;
			md5Bone->orient = pose->q;
            md5meshBinaryJoint_t *parent;

            parent = &md5Bones[md5Bone->parentIndex];

            idMat3 m = parent->orient.ToMat3();
            rotated = m * md5Bone->pos;

            quat = md5Bone->orient * parent->orient;
            md5Bone->orient = quat.Normalize();
            md5Bone->pos = parent->pos + rotated;
		}
		else
		{
			md5Bone->pos = pose->t;
			md5Bone->orient = pose->q;
		}
		//printf("%d %s | %f %f %f | %f %f %f\n", md5Bones[i].parentIndex, joint->name.c_str(), md5Bone->pos[0], md5Bone->pos[1], md5Bone->pos[2], md5Bone->orient[0], md5Bone->orient[1], md5Bone->orient[2]);
		//for(int k=0;k<12;k++) printf(" %f", poseMat3[i].ToFloatPtr()[k]); printf("\n");
	}

	R_ConvertJointTransforms(md5Bones);

	// parse num meshes
	file->ReadInt(num);

	if (num < 0) {
		fileSystem->CloseFile(file);
		common->Error("Invalid size: %d", num);
		return false;
	}

	meshes.SetGranularity(1);
	meshes.SetNum(num);
	idList<idMD5Mesh::binaryVertGroup_t> verts;

	for (i = 0; i < num; i++) {
		if(! meshes[ i ].ReadBinary(file, joints.Num(), md5Bones.Ptr(), poseMat3, verts))
		{
			fileSystem->CloseFile(file);
			return false;
		}
	}

	//Sys_Printf("zzz %s %d\n", Name(), numLod);
	//karin: only read LOD 0, skip others
	bool res = true;
	for (int lod = 1; lod < numLod; lod++) {
		// parse num LOD meshes
#if 1 // skip LODs
		if (!SkipLOD(file)) {
			res = false; // but md5mesh has loaded although fail here
			break;
		}
#else
		file->ReadInt(num);

		if (num < 0) {
			res = false; // but md5mesh has loaded although fail here
		}

		if (res) {
			idList<idMD5Mesh> lodMeshes;
			lodMeshes.SetGranularity(1);
			lodMeshes.SetNum(num);
			verts.Clear();
			for (i = 0; i < num; i++) {
				if(! lodMeshes[ i ].ReadBinary(file, joints.Num(), md5Bones.Ptr(), poseMat3, verts))
				{
					res = false; // but md5mesh has loaded although fail here
					break;
				}
			}
		}
#endif
	}

	//
	// calculate the bounds of the model
	//
	// if want to use bounds in file, or re-calc
	if(res)
	{
		file->ReadVec3(bounds[0]);
		file->ReadVec3(bounds[1]);
	}
	CalculateBounds(poseMat3);

	if(res)
	{
		//
		// parse gui surfaces of the model
		//
		if (ParseGUISurfaces(file) < 0)
		{
			//karin: only clear parsed gui surfaces, but md5mesh is load finished although parse gui surfaces fail
			guiSurfaces.Clear();
		}
	}
	//Sys_Printf("qqq %s %d\n", Name(), guiSurfaces.Num());

	// set the timestamp for reloadmodels
	timeStamp = file->Timestamp();

	fileSystem->CloseFile(file);

	return true;
}

int idRenderModelMD5::ParseGUISurfaces(idFile *file)
{
	int num = 0;
	guiSurface_t *guiSurf;
	int jointIndex;
	int numPlanes;

	file->ReadInt(num);
	if (num <= 0)
		return num;

	guiSurfaces.SetNum(num);
	guiSurf = guiSurfaces.Ptr();
	for (int i = 0; i < num; i++, guiSurf++)
	{
		file->ReadInt(guiSurf->guiNum);
		file->ReadVec3(guiSurf->bounds[0]);
		file->ReadVec3(guiSurf->bounds[1]);
		file->ReadVec3(guiSurf->origin);
		file->ReadMat3(guiSurf->axis);
		file->ReadInt(jointIndex);
		guiSurf->joint = *(jointHandle_t *)&jointIndex;
		file->ReadFloatArray(guiSurf->plane.ToFloatPtr(), 4);
		file->ReadInt(numPlanes);
		if (numPlanes > MAX_GUISURFACE_TRIANGLES)
		{
			common->Warning("Number of planes is over: %d > %d", numPlanes, MAX_GUISURFACE_TRIANGLES);
			return false;
		}
		for (int k = 0; k < numPlanes; k++)
		{
			file->ReadFloatArray(guiSurf->edgePlanes[k][0].ToFloatPtr(), 4);
			file->ReadFloatArray(guiSurf->edgePlanes[k][1].ToFloatPtr(), 4);
		}
		guiSurf->numTris = numPlanes;
	}

	return num;
}

#endif

#ifdef _MODEL_MD5_EXT
#define MD5_APPEND_COMMENT 1
#endif

#ifdef MD5_STATIC_MESH_EXT
#include "model/Model_md5mesh.cpp"
#endif

#ifdef _MODEL_MD5_EXT
#include "model/Model_md5anim.cpp"

#include "model/Model_md5convert.h"
#endif

#ifdef _MODEL_MD5V6
#include "model/Model_md5mesh_v6.cpp"
#include "model/Model_md5anim_v6.cpp"
#endif

#ifdef _MODEL_PSK
#include "model/Model_psk.cpp"
#include "model/Model_psa.cpp"
#endif

#ifdef _MODEL_IQM
#include "model/Model_iqm.cpp"
#endif

#ifdef _MODEL_SMD
#include "model/Model_smd.cpp"
#endif

#ifdef _MODEL_GLTF
#include "model/Model_gltf.cpp"
#endif

#ifdef _MODEL_FBX
#include "model/Model_fbx.cpp"
#endif

#ifdef _MODEL_MD5_EXT
#include "model/Model_md5convert.cpp"
#endif
