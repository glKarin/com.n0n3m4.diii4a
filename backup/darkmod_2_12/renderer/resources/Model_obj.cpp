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

#include "precompiled.h"
#pragma hdrstop

#include "renderer/resources/Model_obj.h"


obj_file_t *OBJ_Load(const char *fileName) {
	idLexer lexer(fileName, LEXFL_NOFATALERRORS);
	if (!lexer.IsLoaded())
		return nullptr;

	std::unique_ptr<obj_file_t> obj(new obj_file_t);
	obj->timestamp = lexer.GetFileTime();
	obj->usesUnknownMaterial = false;

	idToken token;
	int materialIdx = -1;	// -1 = unknown -> default
	int numFacesExcessive = 0;

	while (lexer.ReadToken(&token)) {

		if (token.Cmp("v") == 0) {
			idVec3 pos;
			pos.x = lexer.ParseFloat();
			pos.y = lexer.ParseFloat();
			pos.z = lexer.ParseFloat();

			idVec3 color(1.0f);
			if (lexer.ReadTokenOnLine(&token) && token.type == TT_NUMBER) {
				float red = token.GetFloatValue();
				if (lexer.ReadTokenOnLine(&token) && token.type == TT_NUMBER) {
					float green = token.GetFloatValue();
					if (lexer.ReadTokenOnLine(&token) && token.type == TT_NUMBER) {
						float blue = token.GetFloatValue();
						// this is RGB color (common extension)
						color.Set(red, green, blue);
					}
				}
			}

			obj->vertices.AddGrow(pos);
			obj->colors.AddGrow(color);
		}
		else if (token.Cmp("vt") == 0) {
			idVec2 tc;
			tc.x = lexer.ParseFloat();
			tc.y = lexer.ParseFloat();
			// note: we don't read third texcoord (if available)
			// since calling code does not use it anyway

			// note: we need to invert V coord to match texture
			// we do the same for LWO loading, fhDOOM does the same for OBJ
			tc.y = 1.0f - tc.y;

			obj->texcoords.AddGrow(tc);
		}
		else if (token.Cmp("vn") == 0) {
			idVec3 normal;
			normal.x = lexer.ParseFloat();
			normal.y = lexer.ParseFloat();
			normal.z = lexer.ParseFloat();

			obj->normals.AddGrow(normal);
		}
		else if (token.Cmp("f") == 0) {
			// note: explicit texcoords and normals are mandatory for us!
			// so we don't support formats like:  145  14/17  45//57
			auto ReadIdx = [&lexer, &obj]() -> obj_index_t {
				obj_index_t res;
				res.vertex = lexer.ParseInt();
				lexer.ExpectTokenString("/");
				res.texcoord = lexer.ParseInt();
				lexer.ExpectTokenString("/");
				res.normal = lexer.ParseInt();

				if (res.vertex < 0)
					res.vertex += obj->vertices.Num();
				else
					res.vertex--;

				if (res.texcoord < 0)
					res.texcoord += obj->texcoords.Num();
				else
					res.texcoord--;

				if (res.normal < 0)
					res.normal += obj->normals.Num();
				else
					res.normal--;

				return res;
			};

			obj_index_t idx0 = ReadIdx();
			obj_index_t idx1 = ReadIdx();
			obj_index_t idx2 = ReadIdx();
			if (lexer.ReadTokenOnLine(&token)) {
				numFacesExcessive++;
			}

			// note: we reverse vertex order, since D3 convention is "CCW normal looks inwards"
			// we do the same for LWO loading, fhDOOM does the same for OBJ
			obj->indices.AddGrow(idx0);
			obj->indices.AddGrow(idx2);
			obj->indices.AddGrow(idx1);
			obj->materialIds.AddGrow(materialIdx);

			if (materialIdx < 0) {
				// signal to caller: create one more surface for default material
				obj->usesUnknownMaterial = true;
			}
		}
		else if (token.Cmp("usemtl") == 0) {
			idStr matname;
			lexer.ReadRestOfLine(matname);
			matname.StripWhitespace();

			materialIdx = -1;
			for (int i = 0; i < obj->materials.Num(); i++)
				if (matname.Icmp(obj->materials[i].name) == 0) {
					materialIdx = i;
					break;
				}

			if (materialIdx < 0) {
				obj_material_t mat;
				mat.name = matname;
				mat.material = declManager->FindMaterial(matname);
				materialIdx = obj->materials.AddGrow(mat);
			}
		}
		else if (token[0] == '#') {
			// comment: skip
		}
		else {
			// unknown: skip
		}

		if (lexer.HadError())
			return nullptr;
		lexer.SkipRestOfLine();
	}

	if (numFacesExcessive) {
		common->Warning("Model %s: excessive info for %d faces (only triangles are supported)", fileName, numFacesExcessive);
	}

	// unfortunately, I can't find whether faces must reference predefined data or not
	// so in order to validate vertex indices, we need second pass
	int numVertexErrors = 0;
	int numTexcoordErrors = 0;
	int numNormalErrors = 0;
	int newIdsNum = 0;
	for (int i = 0; i < obj->indices.Num(); i += 3) {
		bool ok = true;
		auto CheckIdx = [&](const obj_index_t &idx) -> void {
			if ( unsigned(idx.vertex) >= unsigned(obj->vertices.Num()) ) {
				numVertexErrors++;
				ok = false;
			}
			if ( unsigned(idx.texcoord) >= unsigned(obj->texcoords.Num()) ) {
				numTexcoordErrors++;
				ok = false;
			}
			if ( unsigned(idx.normal) >= unsigned(obj->normals.Num()) ) {
				numNormalErrors++;
				ok = false;
			}
		};

		CheckIdx(obj->indices[i + 0]);
		CheckIdx(obj->indices[i + 1]);
		CheckIdx(obj->indices[i + 2]);

		if (ok) {
			obj->indices[newIdsNum + 0] = obj->indices[i + 0];
			obj->indices[newIdsNum + 1] = obj->indices[i + 1];
			obj->indices[newIdsNum + 2] = obj->indices[i + 2];
			newIdsNum += 3;
		}
	}

	if (numVertexErrors || numTexcoordErrors || numNormalErrors) {
		common->Warning("Model %s: errors in indices (%d/%d/%d)", fileName, numVertexErrors, numTexcoordErrors, numNormalErrors);
	}
	obj->indices.SetNum(newIdsNum);

	return obj.release();
}
