// Model_obj.cpp
//


#include "../../renderer/RenderSystem_local.h"
#include "../Model_local.h"

/*
========================
idRenderModelStatic::ParseOBJ
========================
*/
void idRenderModelStatic::ParseOBJ(rvmListSTL<idDrawVert> &drawVerts, const char *fileName, const char *objFileBuffer, int length) {
	idLexer src;
	idToken	token, token2;
	rvmListSTL<idVec3> vertexes;
	rvmListSTL<idVec2> texCoords;
	rvmListSTL<idVec3> normals;

	src.LoadMemory(objFileBuffer, length, fileName, 0);

	while (1) {
		if (!src.ReadToken(&token)) {
			break;
		}

		if (token == "v") {
			idVec3 vertex;
			vertex.x = src.ParseFloat();
			vertex.z = src.ParseFloat();
			vertex.y = -src.ParseFloat();
			vertexes.Append(vertex);
		}
		else if (token == "vt") {
			idVec2 st;
			st.x = src.ParseFloat();
			st.y = 1.0 - src.ParseFloat();
			texCoords.Append(st);
		}
		else if (token == "#") {
			idStr line;

			// Skip comments
			src.ReadRestOfLine(line);
		}
		else if (token == "mtllib") {
			idStr line;

			// We don't use obj materials.
			src.ReadRestOfLine(line);
		}
		else if (token == "s") {
			idStr line;
			src.ReadRestOfLine(line);
		}
		else if (token == "g") {
			idStr line;
			src.ReadRestOfLine(line);
		}
		else if (token == "usemtl") {
			idStr line;
			src.ReadRestOfLine(line);
		}
		else if (token == "vn") {
			idVec3 normal;
			normal.x = src.ParseFloat();
			normal.y = src.ParseFloat();
			normal.z = src.ParseFloat();
			normals.Append(normal);
		}
		else if (token == "f") {
			idStr line;
			int vertexIndex[3];
			int uvIndex[3];
			int normalIndex[3];

			src.ReadRestOfLine(line);

			if (normals.Num() > 0)
			{
				sscanf(line.c_str(), "%d/%d/%d %d/%d/%d %d/%d/%d", &vertexIndex[0], &uvIndex[0], &normalIndex[0], & vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			}
			else
			{
				sscanf(line.c_str(), "%d/%d %d/%d %d/%d", &vertexIndex[0], &uvIndex[0], &vertexIndex[1], &uvIndex[1], &vertexIndex[2], &uvIndex[2]);
			}
			


			for (int i = 0; i < 3; i++)
			{
				idDrawVert drawVert;

				int vertidx = vertexIndex[i] - 1;
				int texidx = uvIndex[i] - 1;
				//int normalidx = normalIndex[i] - 1;

				drawVert.xyz = vertexes[vertidx];
				drawVert.st = texCoords[texidx];
				//drawVert.normal = normals[normalidx];

				drawVerts.Append(drawVert);
			}
		}
		else {
			common->FatalError("idRenderModelStatic::ParseOBJ: Unknown or unexpected token %s", token.c_str());
		}
	}
}

/*
========================
idRenderModelStatic::LoadOBJ
========================
*/
bool idRenderModelStatic::LoadOBJ(const char *fileName) {
	const char *objBuffer;
	rvmListSTL<idDrawVert> drawVerts;

	// Try to read in the entire obj file.
	int objBufferLen = fileSystem->ReadFile(fileName, (void **)&objBuffer);
	if (objBufferLen <= 0 || objBuffer == nullptr) {
		common->Warning("idRenderModelStatic::LoadOBJ: Failed to readfile %s\n", fileName);
		return false;
	}

	// Parse the OBJ.
	ParseOBJ(drawVerts, fileName, objBuffer, objBufferLen);

	// Load the material for our obj mesh.
	idStr materialName = fileName;
	materialName.StripFileExtension();
	const idMaterial *material = declManager->FindMaterial("_default");

	// Build the surfaces for these draw vertexes.
	bounds.Clear();

	srfTriangles_t *tri = R_AllocStaticTriSurf();

	// Due to the stupid format of OBJ's, our indexes are simply 1-N were N is vertex count.
	R_AllocStaticTriSurfVerts(tri, drawVerts.Num());
	R_AllocStaticTriSurfIndexes(tri, drawVerts.Num());

	tri->numVerts = drawVerts.Num();
	tri->numIndexes = drawVerts.Num();

	tri->generateNormals = true;

	for (int i = 0; i < drawVerts.Num(); i++) {
		tri->verts[i] = drawVerts[i];		
	}

	for (int i = 0; i < drawVerts.Num(); i+=3)
	{
		tri->indexes[i + 2] = i;
		tri->indexes[i + 1] = i + 1;
		tri->indexes[i + 0] = i + 2;
	}

	R_BoundTriSurf(tri);

	R_CreateVertexNormals(tri);

	{
		idModelSurface	surf;

		surf.id = 0;
		surf.geometry = tri;
		surf.shader = material;

		AddSurface(surf);
		bounds.AddPoint(surf.geometry->bounds[0]);
		bounds.AddPoint(surf.geometry->bounds[1]);
	}

	// Free the objBuffer.
	fileSystem->FreeFile((void *)objBuffer);

	return true;
}