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

#include "../../renderer/RenderSystem_local.h"
#include "../Model_local.h"

static const char* MD5_SnapshotName = "_MD5_Snapshot_";

//#define ID_WIN_X86_SSE2_INTRIN
#ifdef ID_WIN_X86_SSE2_INTRIN
static const __m128 vector_float_posInfinity = { idMath::INFINITY, idMath::INFINITY, idMath::INFINITY, idMath::INFINITY };
static const __m128 vector_float_negInfinity = { -idMath::INFINITY, -idMath::INFINITY, -idMath::INFINITY, -idMath::INFINITY };
#endif



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
idMD5Mesh::idMD5Mesh() {
	scaledWeights = NULL;
	weightIndex = NULL;
	shader = NULL;
	numTris = 0;
	deformInfo = NULL;
	surfaceNum = 0;
	meshJoints = NULL;
	numMeshJoints = 0;
	maxJointVertDist = 0.0f;
}

/*
====================
idMD5Mesh::~idMD5Mesh
====================
*/
idMD5Mesh::~idMD5Mesh() {
	if (meshJoints != NULL) {
		Mem_Free(meshJoints);
		meshJoints = NULL;
	}

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
void idMD5Mesh::ParseMesh(const idStr& fileName, idLexer& parser, int numJoints, const idJointMat* joints) {
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
	idStr path = fileName;

	path.StripFilename();
	parser.ExpectTokenString("{");

	//
	// parse name
	//
	if (parser.CheckTokenString("name")) {
		parser.ReadToken(&name);
	}

	//
	// parse shader
	//
	parser.ExpectTokenString("shader");

	parser.ReadToken(&token);
	shaderName = token;

	shader = declManager->FindMaterial(shaderName, false);
	if (shader == NULL) {
		idStr newShaderPath = va("%s/%s", path.c_str(), shaderName.c_str());
		shader = declManager->FindMaterial(newShaderPath);
	}

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

	numWeights = 0;
	maxweight = 0;
	for (i = 0; i < texCoords.Num(); i++) {
		parser.ExpectTokenString("vert");
		parser.ParseInt();

		parser.Parse1DMatrix(2, texCoords[i].ToFloatPtr());

		firstWeightForVertex[i] = parser.ParseInt();
		numWeightsForVertex[i] = parser.ParseInt();

		if (!numWeightsForVertex[i]) {
			parser.Error("Vertex without any joint weights.");
		}

		numWeights += numWeightsForVertex[i];
		if (numWeightsForVertex[i] + firstWeightForVertex[i] > maxweight) {
			maxweight = numWeightsForVertex[i] + firstWeightForVertex[i];
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

		tris[i * 3 + 0] = parser.ParseInt();
		tris[i * 3 + 1] = parser.ParseInt();
		tris[i * 3 + 2] = parser.ParseInt();
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

		tempWeights[i].joint = jointnum;
		tempWeights[i].jointWeight = parser.ParseFloat();

		parser.Parse1DMatrix(3, tempWeights[i].offset.ToFloatPtr());
	}

	// create pre-scaled weights and an index for the vertex/joint lookup
	scaledWeights = (idVec4*)Mem_Alloc16(numWeights * sizeof(scaledWeights[0]));
	weightIndex = (int*)Mem_Alloc16(numWeights * 2 * sizeof(weightIndex[0]));
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

	parser.ExpectTokenString("}");

	// update counters
	c_numVerts += texCoords.Num();
	c_numWeights += numWeights;
	c_numWeightJoints++;
	for (i = 0; i < numWeights; i++) {
		c_numWeightJoints += weightIndex[i * 2 + 1];
	}


	//
	// build a base pose that can be used for skinning
	//
	idDrawVert* basePose = (idDrawVert*)Mem_ClearedAlloc(texCoords.Num() * sizeof(*basePose));
	for (int j = 0, i = 0; i < texCoords.Num(); i++) {
		idVec3 v = (*(idJointMat*)((byte*)joints + weightIndex[j * 2 + 0])) * scaledWeights[j];
		while (weightIndex[j * 2 + 1] == 0) {
			j++;
			v += (*(idJointMat*)((byte*)joints + weightIndex[j * 2 + 0])) * scaledWeights[j];
		}
		j++;

		basePose[i].Clear();
		basePose[i].xyz = v;
		basePose[i].st = texCoords[i];
		basePose[i].st.y = basePose[i].st.y;
	}

	// build the weights and bone indexes into the verts, so they will be duplicated
	// as necessary at mirror seems

	static int maxWeightsPerVert;
	static float maxResidualWeight;

	const int MAX_VERTEX_WEIGHTS = 4;

	idList< bool > jointIsUsed;
	jointIsUsed.SetNum(numJoints);
	for (int i = 0; i < jointIsUsed.Num(); i++) {
		jointIsUsed[i] = false;
	}

	numMeshJoints = 0;
	maxJointVertDist = 0.0f;

	//-----------------------------------------
	// new-style setup for fixed four weights and normal / tangent deformation
	//
	// Several important models have >25% residual weight in joints after the
	// first four, which is worrisome for using a fixed four joint deformation.
	//-----------------------------------------
	for (int i = 0; i < texCoords.Num(); i++) {
		idDrawVert& dv = basePose[i];

		// some models do have >4 joint weights, so it is necessary to sort and renormalize

		// sort the weights and take the four largest
		int	weights[256];
		const int numWeights = numWeightsForVertex[i];
		for (int j = 0; j < numWeights; j++) {
			weights[j] = firstWeightForVertex[i] + j;
		}
		// bubble sort
		for (int j = 0; j < numWeights; j++) {
			for (int k = 0; k < numWeights - 1 - j; k++) {
				if (tempWeights[weights[k]].jointWeight < tempWeights[weights[k + 1]].jointWeight) {
					SwapValues(weights[k], weights[k + 1]);
				}
			}
		}

		if (numWeights > maxWeightsPerVert) {
			maxWeightsPerVert = numWeights;
		}

		const int usedWeights = Min(MAX_VERTEX_WEIGHTS, numWeights);

		float totalWeight = 0;
		for (int j = 0; j < numWeights; j++) {
			totalWeight += tempWeights[weights[j]].jointWeight;
		}
		assert(totalWeight > 0.999f && totalWeight < 1.001f);

		float usedWeight = 0;
		for (int j = 0; j < usedWeights; j++) {
			usedWeight += tempWeights[weights[j]].jointWeight;
		}

		const float residualWeight = totalWeight - usedWeight;
		if (residualWeight > maxResidualWeight) {
			maxResidualWeight = residualWeight;
		}

		byte finalWeights[MAX_VERTEX_WEIGHTS] = { 0 };
		byte finalJointIndecies[MAX_VERTEX_WEIGHTS] = { 0 };
		for (int j = 0; j < usedWeights; j++) {
			const vertexWeight_t& weight = tempWeights[weights[j]];
			const int jointIndex = weight.joint;
			const float fw = weight.jointWeight;
			assert(fw >= 0.0f && fw <= 1.0f);
			const float normalizedWeight = fw / usedWeight;
			finalWeights[j] = idMath::Ftob(normalizedWeight * 255.0f);
			finalJointIndecies[j] = jointIndex;
		}

		// Sort the weights and indices for hardware skinning
		for (int k = 0; k < 3; ++k) {
			for (int l = k + 1; l < 4; ++l) {
				if (finalWeights[l] > finalWeights[k]) {
					SwapValues(finalWeights[k], finalWeights[l]);
					SwapValues(finalJointIndecies[k], finalJointIndecies[l]);
				}
			}
		}

		// Give any left over to the biggest weight
		finalWeights[0] += Max(255 - finalWeights[0] - finalWeights[1] - finalWeights[2] - finalWeights[3], 0);

		dv.color[0] = finalJointIndecies[0];
		dv.color[1] = finalJointIndecies[1];
		dv.color[2] = finalJointIndecies[2];
		dv.color[3] = finalJointIndecies[3];

		dv.color2[0] = finalWeights[0];
		dv.color2[1] = finalWeights[1];
		dv.color2[2] = finalWeights[2];
		dv.color2[3] = finalWeights[3];

		for (int j = usedWeights; j < 4; j++) {
			assert(dv.color2[j] == 0);
		}

		for (int j = 0; j < usedWeights; j++) {
			if (!jointIsUsed[finalJointIndecies[j]]) {
				jointIsUsed[finalJointIndecies[j]] = true;
				numMeshJoints++;
			}
			const idJointMat& joint = joints[finalJointIndecies[j]];
			float dist = (dv.xyz - joint.GetTranslation()).Length();
			if (dist > maxJointVertDist) {
				maxJointVertDist = dist;
			}
		}
	}

	meshJoints = (byte*)Mem_Alloc(numMeshJoints * sizeof(meshJoints[0]));
	numMeshJoints = 0;
	for (int i = 0; i < numJoints; i++) {
		if (jointIsUsed[i]) {
			meshJoints[numMeshJoints++] = i;
		}
	}

	deformInfo = R_BuildDeformInfo(texCoords.Num(), basePose, tris.Num(), tris.Ptr(), shader->UseUnsmoothedTangents());

	for (int i = 0; i < deformInfo->numOutputVerts; i++) {
		for (int j = 0; j < 4; j++) {
			if (deformInfo->verts[i].color[j] >= numJoints) {
				common->FatalError("Bad joint index");
			}
		}
	}

	tempWeights.Clear();
	numWeightsForVertex.Clear();
	firstWeightForVertex.Clear();

	Mem_Free(basePose);
}

/*
====================
idMD5Mesh::CalcBounds
====================
*/
idBounds idMD5Mesh::CalcBounds(const idJointMat* entJoints) {
#ifdef ID_WIN_X86_SSE2_INTRIN

	__m128 minX = vector_float_posInfinity;
	__m128 minY = vector_float_posInfinity;
	__m128 minZ = vector_float_posInfinity;
	__m128 maxX = vector_float_negInfinity;
	__m128 maxY = vector_float_negInfinity;
	__m128 maxZ = vector_float_negInfinity;
	for (int i = 0; i < numMeshJoints; i++) {
		const idJointMat& joint = entJoints[meshJoints[i]];
		__m128 x = _mm_load_ps(joint.ToFloatPtr() + 0 * 4);
		__m128 y = _mm_load_ps(joint.ToFloatPtr() + 1 * 4);
		__m128 z = _mm_load_ps(joint.ToFloatPtr() + 2 * 4);
		minX = _mm_min_ps(minX, x);
		minY = _mm_min_ps(minY, y);
		minZ = _mm_min_ps(minZ, z);
		maxX = _mm_max_ps(maxX, x);
		maxY = _mm_max_ps(maxY, y);
		maxZ = _mm_max_ps(maxZ, z);
	}
	__m128 expand = _mm_splat_ps(_mm_load_ss(&maxJointVertDist), 0);
	minX = _mm_sub_ps(minX, expand);
	minY = _mm_sub_ps(minY, expand);
	minZ = _mm_sub_ps(minZ, expand);
	maxX = _mm_add_ps(maxX, expand);
	maxY = _mm_add_ps(maxY, expand);
	maxZ = _mm_add_ps(maxZ, expand);
	_mm_store_ss(bounds.ToFloatPtr() + 0, _mm_splat_ps(minX, 3));
	_mm_store_ss(bounds.ToFloatPtr() + 1, _mm_splat_ps(minY, 3));
	_mm_store_ss(bounds.ToFloatPtr() + 2, _mm_splat_ps(minZ, 3));
	_mm_store_ss(bounds.ToFloatPtr() + 3, _mm_splat_ps(maxX, 3));
	_mm_store_ss(bounds.ToFloatPtr() + 4, _mm_splat_ps(maxY, 3));
	_mm_store_ss(bounds.ToFloatPtr() + 5, _mm_splat_ps(maxZ, 3));

#else

	bounds.Clear();
	for (int i = 0; i < numMeshJoints; i++) {
		const idJointMat& joint = entJoints[meshJoints[i]];
		bounds.AddPoint(joint.GetTranslation());
	}
	bounds.ExpandSelf(maxJointVertDist);

#endif
	return bounds;
}

/*
====================
idMD5Mesh::NearestJoint
====================
*/
int idMD5Mesh::NearestJoint(int a, int b, int c) const {
	int i, bestJoint, vertNum, weightVertNum;
	float bestWeight;

	// duplicated vertices might not have weights
	if (a >= 0 && a < texCoords.Num()) {
		vertNum = a;
	}
	else if (b >= 0 && b < texCoords.Num()) {
		vertNum = b;
	}
	else if (c >= 0 && c < texCoords.Num()) {
		vertNum = c;
	}
	else {
		// all vertices are duplicates which shouldn't happen
		return 0;
	}

	// find the first weight for this vertex
	weightVertNum = 0;
	for (i = 0; weightVertNum < vertNum; i++) {
		weightVertNum += weightIndex[i * 2 + 1];
	}

	// get the joint for the largest weight
	bestWeight = scaledWeights[i].w;
	bestJoint = weightIndex[i * 2 + 0] / sizeof(idJointMat);
	for (; weightIndex[i * 2 + 1] == 0; i++) {
		if (scaledWeights[i].w > bestWeight) {
			bestWeight = scaledWeights[i].w;
			bestJoint = weightIndex[i * 2 + 0] / sizeof(idJointMat);
		}
	}
	return bestJoint;
}

/*
====================
idMD5Mesh::NumVerts
====================
*/
int idMD5Mesh::NumVerts(void) const {
	return texCoords.Num();
}

/*
====================
idMD5Mesh::NumTris
====================
*/
int	idMD5Mesh::NumTris(void) const {
	return numTris;
}

/*
====================
idMD5Mesh::NumWeights
====================
*/
int	idMD5Mesh::NumWeights(void) const {
	return numWeights;
}

/***********************************************************************

	idRenderModelMD5

***********************************************************************/

/*
=========================
idRenderModelMD5::idRenderModelMD5
=========================
*/
idRenderModelMD5::idRenderModelMD5() : idRenderModelStatic()
{
	poseMat3 = nullptr;
	jointBuffer = nullptr;
}

/*
=========================
idRenderModelMD5::idRenderModelMD5
=========================
*/
idRenderModelMD5::~idRenderModelMD5()
{
	if (poseMat3 != nullptr)
	{
		Mem_Free16(poseMat3);
		poseMat3 = nullptr;
	}

	if (jointBuffer) {
		delete jointBuffer;
		jointBuffer = nullptr;
	}
}

/*
====================
idRenderModelMD5::ParseJoint
====================
*/
void idRenderModelMD5::ParseJoint(idLexer& parser, idMD5Joint* joint, idJointQuat* defaultPose) {
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
	}
	else {
		if (num >= joints.Num() - 1) {
			parser.Error("Invalid parent for joint '%s'", joint->name.c_str());
		}
		joint->parent = &joints[num];
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
void idRenderModelMD5::InitFromFile(const char* fileName) {
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
void idRenderModelMD5::LoadModel() {
	int			version;
	int			i;
	int			num;
	int			parentNum;
	idToken		token;
	idLexer		parser(LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS);
	idJointQuat* pose;
	idMD5Joint* joint;

	if (!purged) {
		PurgeModel();
	}
	purged = false;

	if (!parser.LoadFile(name)) {
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
	num = parser.ParseInt();
	joints.SetGranularity(1);
	joints.SetNum(num);
	defaultPose.SetGranularity(1);
	defaultPose.SetNum(num);
	// jmarshall
	poseMat3 = (idJointMat*)Mem_Alloc16(num * sizeof(*poseMat3));
	// jmarshall end

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
		poseMat3[i].SetRotation(pose->q.ToMat3());
		poseMat3[i].SetTranslation(pose->t);
		if (joint->parent) {
			parentNum = joint->parent - joints.Ptr();
			pose->q = (poseMat3[i].ToMat3() * poseMat3[parentNum].ToMat3().Transpose()).ToQuat();
			pose->t = (poseMat3[i].ToVec3() - poseMat3[parentNum].ToVec3()) * poseMat3[parentNum].ToMat3().Transpose();
		}
	}
	parser.ExpectTokenString("}");

	//-----------------------------------------
	// create the inverse of the base pose joints to support tech6 style deformation
	// of base pose vertexes, normals, and tangents.
	//
	// vertex * joints * inverseJoints == vertex when joints is the base pose
	// When the joints are in another pose, it gives the animated vertex position
	//-----------------------------------------
	invertedDefaultPose.SetNum(SIMD_ROUND_JOINTS(joints.Num()));
	for (int i = 0; i < joints.Num(); i++) {
		invertedDefaultPose[i] = poseMat3[i];
		invertedDefaultPose[i].Invert();
	}
	SIMD_INIT_LAST_JOINT(invertedDefaultPose.Ptr(), joints.Num());

	for (i = 0; i < meshes.Num(); i++) {
		parser.ExpectTokenString("mesh");
		// jmarshall
				//meshes[ i ].ParseMesh( parser, defaultPose.Num(), poseMat3 );
		meshes[i].ParseMesh(name, parser, defaultPose.Num(), poseMat3);
		// jmarshall end
	}
#ifndef ID_DEDICATED
	{
		// Upload the bind pose to the GPU.
		jointBuffer = new idJointBuffer();
		jointBuffer->AllocBufferObject((float*)invertedDefaultPose.Ptr(), invertedDefaultPose.Num());
	}
#endif

	//
	// calculate the bounds of the model
	//
// jmarshall
	CalculateBounds(poseMat3);
	// jmarshall end

		// set the timestamp for reloadmodels
	fileSystem->ReadFile(name, NULL, &timeStamp);
}

/*
==============
idRenderModelMD5::Print
==============
*/
void idRenderModelMD5::Print() const {
	const idMD5Mesh* mesh;
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
void idRenderModelMD5::List() const {
	int			i;
	const idMD5Mesh* mesh;
	int			totalTris = 0;
	int			totalVerts = 0;

	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		totalTris += mesh->numTris;
		totalVerts += mesh->NumVerts();
	}
	common->Printf(" %4ik %3i %4i %4i %s(MD5)", Memory() / 1024, meshes.Num(), totalVerts, totalTris, Name());

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
void idRenderModelMD5::CalculateBounds(const idJointMat* entJoints) {
	int			i;
	idMD5Mesh* mesh;

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
idBounds idRenderModelMD5::Bounds(const renderEntity_t* ent) const {
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
void idRenderModelMD5::DrawJoints(const renderEntity_t* ent, const struct renderView_t* view) const {
	int					i;
	int					num;
	idVec3				pos;
	const idJointMat* joint;
	const idMD5Joint* md5Joint;
	int					parentNum;

	num = ent->numJoints;
	joint = ent->joints;
	md5Joint = joints.Ptr();
	for (i = 0; i < num; i++, joint++, md5Joint++) {
		pos = ent->origin + joint->ToVec3() * ent->axis;
		if (md5Joint->parent) {
			parentNum = md5Joint->parent - joints.Ptr();
			session->rw->DebugLine(colorWhite, ent->origin + ent->joints[parentNum].ToVec3() * ent->axis, pos);
		}

		session->rw->DebugLine(colorRed, pos, pos + joint->ToMat3()[0] * 2.0f * ent->axis);
		session->rw->DebugLine(colorGreen, pos, pos + joint->ToMat3()[1] * 2.0f * ent->axis);
		session->rw->DebugLine(colorBlue, pos, pos + joint->ToMat3()[2] * 2.0f * ent->axis);
	}

	idBounds bounds;

	bounds.FromTransformedBounds(ent->bounds, vec3_zero, ent->axis);
	session->rw->DebugBounds(colorMagenta, bounds, ent->origin);

	if ((r_jointNameScale.GetFloat() != 0.0f) && (bounds.Expand(128.0f).ContainsPoint(view->vieworg - ent->origin))) {
		idVec3	offset(0, 0, r_jointNameOffset.GetFloat());
		float	scale;

		scale = r_jointNameScale.GetFloat();
		joint = ent->joints;
		num = ent->numJoints;
		for (i = 0; i < num; i++, joint++) {
			pos = ent->origin + joint->ToVec3() * ent->axis;
			session->rw->DrawText(joints[i].name, pos + offset, scale, colorWhite, view->viewaxis, 1);
		}
	}
}

/*
====================
TransformJoints
====================
*/
static void TransformJoints(idJointMat* __restrict outJoints, const int numJoints, const idJointMat* __restrict inJoints1, const idJointMat* __restrict inJoints2) {

	float* outFloats = outJoints->ToFloatPtr();
	const float* inFloats1 = inJoints1->ToFloatPtr();
	const float* inFloats2 = inJoints2->ToFloatPtr();

	//assert_16_byte_aligned(outFloats);
	//assert_16_byte_aligned(inFloats1);
	//assert_16_byte_aligned(inFloats2);

#ifdef ID_WIN_X86_SSE2_INTRIN

	const __m128 mask_keep_last = __m128c(_mm_set_epi32(0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000));

	for (int i = 0; i < numJoints; i += 2, inFloats1 += 2 * 12, inFloats2 += 2 * 12, outFloats += 2 * 12) {
		__m128 m1a0 = _mm_load_ps(inFloats1 + 0 * 12 + 0);
		__m128 m1b0 = _mm_load_ps(inFloats1 + 0 * 12 + 4);
		__m128 m1c0 = _mm_load_ps(inFloats1 + 0 * 12 + 8);
		__m128 m1a1 = _mm_load_ps(inFloats1 + 1 * 12 + 0);
		__m128 m1b1 = _mm_load_ps(inFloats1 + 1 * 12 + 4);
		__m128 m1c1 = _mm_load_ps(inFloats1 + 1 * 12 + 8);

		__m128 m2a0 = _mm_load_ps(inFloats2 + 0 * 12 + 0);
		__m128 m2b0 = _mm_load_ps(inFloats2 + 0 * 12 + 4);
		__m128 m2c0 = _mm_load_ps(inFloats2 + 0 * 12 + 8);
		__m128 m2a1 = _mm_load_ps(inFloats2 + 1 * 12 + 0);
		__m128 m2b1 = _mm_load_ps(inFloats2 + 1 * 12 + 4);
		__m128 m2c1 = _mm_load_ps(inFloats2 + 1 * 12 + 8);

		__m128 tj0 = _mm_and_ps(m1a0, mask_keep_last);
		__m128 tk0 = _mm_and_ps(m1b0, mask_keep_last);
		__m128 tl0 = _mm_and_ps(m1c0, mask_keep_last);
		__m128 tj1 = _mm_and_ps(m1a1, mask_keep_last);
		__m128 tk1 = _mm_and_ps(m1b1, mask_keep_last);
		__m128 tl1 = _mm_and_ps(m1c1, mask_keep_last);

		__m128 ta0 = _mm_splat_ps(m1a0, 0);
		__m128 td0 = _mm_splat_ps(m1b0, 0);
		__m128 tg0 = _mm_splat_ps(m1c0, 0);
		__m128 ta1 = _mm_splat_ps(m1a1, 0);
		__m128 td1 = _mm_splat_ps(m1b1, 0);
		__m128 tg1 = _mm_splat_ps(m1c1, 0);

		__m128 ra0 = _mm_add_ps(tj0, _mm_mul_ps(ta0, m2a0));
		__m128 rd0 = _mm_add_ps(tk0, _mm_mul_ps(td0, m2a0));
		__m128 rg0 = _mm_add_ps(tl0, _mm_mul_ps(tg0, m2a0));
		__m128 ra1 = _mm_add_ps(tj1, _mm_mul_ps(ta1, m2a1));
		__m128 rd1 = _mm_add_ps(tk1, _mm_mul_ps(td1, m2a1));
		__m128 rg1 = _mm_add_ps(tl1, _mm_mul_ps(tg1, m2a1));

		__m128 tb0 = _mm_splat_ps(m1a0, 1);
		__m128 te0 = _mm_splat_ps(m1b0, 1);
		__m128 th0 = _mm_splat_ps(m1c0, 1);
		__m128 tb1 = _mm_splat_ps(m1a1, 1);
		__m128 te1 = _mm_splat_ps(m1b1, 1);
		__m128 th1 = _mm_splat_ps(m1c1, 1);

		__m128 rb0 = _mm_add_ps(ra0, _mm_mul_ps(tb0, m2b0));
		__m128 re0 = _mm_add_ps(rd0, _mm_mul_ps(te0, m2b0));
		__m128 rh0 = _mm_add_ps(rg0, _mm_mul_ps(th0, m2b0));
		__m128 rb1 = _mm_add_ps(ra1, _mm_mul_ps(tb1, m2b1));
		__m128 re1 = _mm_add_ps(rd1, _mm_mul_ps(te1, m2b1));
		__m128 rh1 = _mm_add_ps(rg1, _mm_mul_ps(th1, m2b1));

		__m128 tc0 = _mm_splat_ps(m1a0, 2);
		__m128 tf0 = _mm_splat_ps(m1b0, 2);
		__m128 ti0 = _mm_splat_ps(m1c0, 2);
		__m128 tf1 = _mm_splat_ps(m1b1, 2);
		__m128 ti1 = _mm_splat_ps(m1c1, 2);
		__m128 tc1 = _mm_splat_ps(m1a1, 2);

		__m128 rc0 = _mm_add_ps(rb0, _mm_mul_ps(tc0, m2c0));
		__m128 rf0 = _mm_add_ps(re0, _mm_mul_ps(tf0, m2c0));
		__m128 ri0 = _mm_add_ps(rh0, _mm_mul_ps(ti0, m2c0));
		__m128 rc1 = _mm_add_ps(rb1, _mm_mul_ps(tc1, m2c1));
		__m128 rf1 = _mm_add_ps(re1, _mm_mul_ps(tf1, m2c1));
		__m128 ri1 = _mm_add_ps(rh1, _mm_mul_ps(ti1, m2c1));

		_mm_store_ps(outFloats + 0 * 12 + 0, rc0);
		_mm_store_ps(outFloats + 0 * 12 + 4, rf0);
		_mm_store_ps(outFloats + 0 * 12 + 8, ri0);
		_mm_store_ps(outFloats + 1 * 12 + 0, rc1);
		_mm_store_ps(outFloats + 1 * 12 + 4, rf1);
		_mm_store_ps(outFloats + 1 * 12 + 8, ri1);
	}

#else

	for (int i = 0; i < numJoints; i++) {
		idJointMat::Multiply(outJoints[i], inJoints1[i], inJoints2[i]);
	}

#endif
}

/*
====================
idRenderModelMD5::InstantiateDynamicModel
====================
*/
idRenderModel* idRenderModelMD5::InstantiateDynamicModel(const struct renderEntity_t* ent, const idRenderWorldCommitted* view, idRenderModel* cachedModel) {
	idRenderModelMD5Instance* staticModel;

	if (cachedModel && !r_useCachedDynamicModels.GetBool()) {
		delete cachedModel;
		cachedModel = NULL;
	}

	if (purged) {
		common->DWarning("model %s instantiated while purged", Name());
		LoadModel();
	}

	if (cachedModel) {
		assert(dynamic_cast<idRenderModelMD5Instance*>(cachedModel) != NULL);
		assert(idStr::Icmp(cachedModel->Name(), MD5_SnapshotName) == 0);
		staticModel = static_cast<idRenderModelMD5Instance*>(cachedModel);
	}
	else {
		staticModel = new idRenderModelMD5Instance;
		staticModel->InitEmpty(MD5_SnapshotName);
	}

	// update the GPU joints array
	const int numInvertedJoints = SIMD_ROUND_JOINTS(joints.Num());
	if (staticModel->jointsInverted == NULL) {
		staticModel->numInvertedJoints = numInvertedJoints;
		const int alignment = glConfig.uniformBufferOffsetAlignment;
		staticModel->jointsInverted = (idJointMat*)Mem_ClearedAlloc(ALIGN(numInvertedJoints * sizeof(idJointMat), alignment));
	}
	else {
		assert(staticModel->numInvertedJoints == numInvertedJoints);
	}

	staticModel->jointBuffer = jointBuffer;
	if (ent == nullptr || ent->joints == nullptr)
	{
		TransformJoints(staticModel->jointsInverted, joints.Num(), &poseMat3[0], invertedDefaultPose.Ptr());
	}
	else
	{
		TransformJoints(staticModel->jointsInverted, joints.Num(), ent->joints, invertedDefaultPose.Ptr());
	}

	staticModel->CreateStaticMeshSurfaces(meshes);

	return staticModel;
}

/*
====================
idRenderModelMD5::IsDynamicModel
====================
*/
dynamicModel_t idRenderModelMD5::IsDynamicModel() const {
	return DM_CACHED;
}

/*
====================
idRenderModelMD5::NumJoints
====================
*/
int idRenderModelMD5::NumJoints(void) const {
	return joints.Num();
}

/*
====================
idRenderModelMD5::GetJoints
====================
*/
const idMD5Joint* idRenderModelMD5::GetJoints(void) const {
	return joints.Ptr();
}

/*
====================
idRenderModelMD5::GetDefaultPose
====================
*/
const idJointQuat* idRenderModelMD5::GetDefaultPose(void) const {
	return defaultPose.Ptr();
}

/*
====================
idRenderModelMD5::GetJointHandle
====================
*/
jointHandle_t idRenderModelMD5::GetJointHandle(const char* name) const {
	const idMD5Joint* joint;
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
const char* idRenderModelMD5::GetJointName(jointHandle_t handle) const {
	if ((handle < 0) || (handle >= joints.Num())) {
		return "<invalid joint>";
	}

	return joints[handle].name;
}

/*
====================
idRenderModelMD5::NearestJoint
====================
*/
int idRenderModelMD5::NearestJoint(int surfaceNum, int a, int b, int c) const {
	int i;
	const idMD5Mesh* mesh;

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
void idRenderModelMD5::TouchData() {
	idMD5Mesh* mesh;
	int			i;

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
void idRenderModelMD5::PurgeModel() {
	purged = true;
	joints.Clear();
	defaultPose.Clear();
	meshes.Clear();
}

/*
===================
idRenderModelMD5::Memory
===================
*/
int	idRenderModelMD5::Memory() const {
	int		total, i;

	total = sizeof(*this);
	total += joints.MemoryUsed() + defaultPose.MemoryUsed() + meshes.MemoryUsed();

	// count up strings
	for (i = 0; i < joints.Num(); i++) {
		total += joints[i].name.DynamicMemoryUsed();
	}

	// count up meshes
	for (i = 0; i < meshes.Num(); i++) {
		const idMD5Mesh* mesh = &meshes[i];

		total += mesh->texCoords.MemoryUsed() + mesh->numWeights * (sizeof(mesh->scaledWeights[0]) + sizeof(mesh->weightIndex[0]) * 2);

		// sum up deform info
		total += sizeof(mesh->deformInfo);
		total += R_DeformInfoMemoryUsed(mesh->deformInfo);
	}
	return total;
}


/*
====================
idRenderModelMD5Instance::IsSkeletalMesh
====================
*/
bool idRenderModelMD5Instance::IsSkeletalMesh() const {
	return true;
}

/*
====================
idRenderModelMD5Instance::CreateStaticMeshSurfaces
====================
*/
void idRenderModelMD5Instance::CreateStaticMeshSurfaces(const idList<idMD5Mesh>& meshes) {
	int	i, surfaceNum;
	const idMD5Mesh* mesh;

	// create all the surfaces
	for (mesh = meshes.Ptr(), i = 0; i < meshes.Num(); i++, mesh++) {
		const idMaterial* shader = mesh->GetShader();
		struct deformInfo_s* deformInfo = mesh->deformInfo;
		idModelSurface* surf;

		if (FindSurfaceWithId(i, surfaceNum)) {
			surf = &surfaces[surfaceNum];
		}
		else {
			surf = &surfaces.Alloc();
			surf->geometry = NULL;
			surf->shader = shader;
			surf->id = i;

			UpdateSurfaceGPU(deformInfo, nullptr, surf, shader);

			bounds.AddPoint(surf->geometry->bounds[0]);
			bounds.AddPoint(surf->geometry->bounds[1]);
		}
	}
}


/*
====================
idRenderModelMD5Instance::UpdateSurface
====================
*/
void idRenderModelMD5Instance::UpdateSurfaceGPU(struct deformInfo_s* deformInfo, const renderEntity_t* ent, idModelSurface* surf, const idMaterial* shader) {
	int i, base;
	srfTriangles_t* tri;

	tr.pc.c_deformedSurfaces++;
	tr.pc.c_deformedVerts += deformInfo->numOutputVerts;
	tr.pc.c_deformedIndexes += deformInfo->numIndexes;

	surf->shader = shader;

	if (surf->geometry) {
		// if the number of verts and indexes are the same we can re-use the triangle surface
		// the number of indexes must be the same to assure the correct amount of memory is allocated for the facePlanes
		if (surf->geometry->numVerts == deformInfo->numOutputVerts && surf->geometry->numIndexes == deformInfo->numIndexes) {
			R_FreeStaticTriSurfVertexCaches(surf->geometry);
		}
		else {
			R_FreeStaticTriSurf(surf->geometry);
			surf->geometry = R_AllocStaticTriSurf();
		}
	}
	else {
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

	R_AllocStaticTriSurfVerts(tri, tri->numVerts);
	memcpy(tri->verts, deformInfo->verts, deformInfo->numOutputVerts * sizeof(deformInfo->verts[0]));	// copy over the texture coordinates

	R_DeriveTangents(tri);

	tri->tangentsCalculated = true;
	R_BoundTriSurf(tri);
}
