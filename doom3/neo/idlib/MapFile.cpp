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

#include "precompiled.h"
#pragma hdrstop


/*
===============
FloatCRC
===============
*/
ID_INLINE unsigned int FloatCRC(float f)
{
	return *(unsigned int *)&f;
}

/*
===============
StringCRC
===============
*/
ID_INLINE unsigned int StringCRC(const char *str)
{
	unsigned int i, crc;
	const unsigned char *ptr;

	crc = 0;
	ptr = reinterpret_cast<const unsigned char *>(str);

	for (i = 0; str[i]; i++) {
		crc ^= str[i] << (i & 3);
	}

	return crc;
}

/*
=================
ComputeAxisBase

WARNING : special case behaviour of atan2(y,x) <-> atan(y/x) might not be the same everywhere when x == 0
rotation by (0,RotY,RotZ) assigns X to normal
=================
*/
static void ComputeAxisBase(const idVec3 &normal, idVec3 &texS, idVec3 &texT)
{
	float RotY, RotZ;
	idVec3 n;

	// do some cleaning
	n[0] = (idMath::Fabs(normal[0]) < 1e-6f) ? 0.0f : normal[0];
	n[1] = (idMath::Fabs(normal[1]) < 1e-6f) ? 0.0f : normal[1];
	n[2] = (idMath::Fabs(normal[2]) < 1e-6f) ? 0.0f : normal[2];

	RotY = -atan2(n[2], idMath::Sqrt(n[1] * n[1] + n[0] * n[0]));
	RotZ = atan2(n[1], n[0]);
	// rotate (0,1,0) and (0,0,1) to compute texS and texT
	texS[0] = -sin(RotZ);
	texS[1] = cos(RotZ);
	texS[2] = 0;
	// the texT vector is along -Z ( T texture coorinates axis )
	texT[0] = -sin(RotY) * cos(RotZ);
	texT[1] = -sin(RotY) * sin(RotZ);
	texT[2] = -cos(RotY);
}

/*
=================
idMapBrushSide::GetTextureVectors
=================
*/
void idMapBrushSide::GetTextureVectors(idVec4 v[2]) const
{
	int i;
	idVec3 texX, texY;

	ComputeAxisBase(plane.Normal(), texX, texY);

	for (i = 0; i < 2; i++) {
		v[i][0] = texX[0] * texMat[i][0] + texY[0] * texMat[i][1];
		v[i][1] = texX[1] * texMat[i][0] + texY[1] * texMat[i][1];
		v[i][2] = texX[2] * texMat[i][0] + texY[2] * texMat[i][1];
		v[i][3] = texMat[i][2] + (origin * v[i].ToVec3());
	}
}

/*
=================
idMapPatch::Parse
=================
*/
#ifdef _RAVEN // _QUAKE4
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
idMapPatch *idMapPatch::Parse(Lexer &src, const idVec3 &origin, bool patchDef3, int version)
// RAVEN END
#else
idMapPatch *idMapPatch::Parse(idLexer &src, const idVec3 &origin, bool patchDef3, float version)
#endif
{
	float		info[7];
	idDrawVert *vert;
	idToken		token;
	int			i, j;

	if (!src.ExpectTokenString("{")) {
		return NULL;
	}

	// read the material (we had an implicit 'textures/' in the old format...)
	if (!src.ReadToken(&token)) {
		src.Error("idMapPatch::Parse: unexpected EOF");
		return NULL;
	}

	// Parse it
	if (patchDef3) {
		if (!src.Parse1DMatrix(7, info)) {
			src.Error("idMapPatch::Parse: unable to Parse patchDef3 info");
			return NULL;
		}
	} else {
		if (!src.Parse1DMatrix(5, info)) {
			src.Error("idMapPatch::Parse: unable to parse patchDef2 info");
			return NULL;
		}
	}

	idMapPatch *patch = new idMapPatch(info[0], info[1]);
	patch->SetSize(info[0], info[1]);

#ifdef _RAVEN // _QUAKE4
	if (version < 2)
#else
	if (version < 2.0f)
#endif
	{
		patch->SetMaterial("textures/" + token);
	} else {
		patch->SetMaterial(token);
	}

	if (patchDef3) {
		patch->SetHorzSubdivisions(info[2]);
		patch->SetVertSubdivisions(info[3]);
		patch->SetExplicitlySubdivided(true);
	}

	if (patch->GetWidth() < 0 || patch->GetHeight() < 0) {
		src.Error("idMapPatch::Parse: bad size");
		delete patch;
		return NULL;
	}

	// these were written out in the wrong order, IMHO
	if (!src.ExpectTokenString("(")) {
		src.Error("idMapPatch::Parse: bad patch vertex data");
		delete patch;
		return NULL;
	}

	for (j = 0; j < patch->GetWidth(); j++) {
		if (!src.ExpectTokenString("(")) {
			src.Error("idMapPatch::Parse: bad vertex row data");
			delete patch;
			return NULL;
		}

		for (i = 0; i < patch->GetHeight(); i++) {
			float v[5];

			if (!src.Parse1DMatrix(5, v)) {
				src.Error("idMapPatch::Parse: bad vertex column data");
				delete patch;
				return NULL;
			}

			vert = &((*patch)[i * patch->GetWidth() + j]);
			vert->xyz[0] = v[0] - origin[0];
			vert->xyz[1] = v[1] - origin[1];
			vert->xyz[2] = v[2] - origin[2];
			vert->st[0] = v[3];
			vert->st[1] = v[4];
		}

		if (!src.ExpectTokenString(")")) {
			delete patch;
			src.Error("idMapPatch::Parse: unable to parse patch control points");
			return NULL;
		}
	}

#ifdef _RAVEN // _QUAKE4
	if ( !src.ExpectTokenString( ")" ) ||
			!src.ExpectTokenString( "}" ) ||
				!src.ExpectTokenString( "}" ) )
#else
	if (!src.ExpectTokenString(")"))
#endif
	{
		src.Error("idMapPatch::Parse: unable to parse patch control points, no closure");
		delete patch;
		return NULL;
	}

#if !defined(_RAVEN)
	// read any key/value pairs
	while (src.ReadToken(&token)) {
		if (token == "}") {
			src.ExpectTokenString("}");
			break;
		}

		if (token.type == TT_STRING) {
			idStr key = token;
			src.ExpectTokenType(TT_STRING, 0, &token);
			patch->epairs.Set(key, token);
		}
	}
#endif

	return patch;
}

/*
============
idMapPatch::Write
============
*/
bool idMapPatch::Write(idFile *fp, int primitiveNum, const idVec3 &origin) const
{
	int i, j;
	const idDrawVert *v;

	if (GetExplicitlySubdivided()) {
		fp->WriteFloatString("// primitive %d\n{\n patchDef3\n {\n", primitiveNum);
		fp->WriteFloatString("  \"%s\"\n  ( %d %d %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight(), GetHorzSubdivisions(), GetVertSubdivisions());
	} else {
		fp->WriteFloatString("// primitive %d\n{\n patchDef2\n {\n", primitiveNum);
		fp->WriteFloatString("  \"%s\"\n  ( %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight());
	}

	fp->WriteFloatString("  (\n");

	for (i = 0; i < GetWidth(); i++) {
		fp->WriteFloatString("   ( ");

		for (j = 0; j < GetHeight(); j++) {
			v = &verts[ j * GetWidth() + i ];
			fp->WriteFloatString(" ( %f %f %f %f %f )", v->xyz[0] + origin[0],
			                     v->xyz[1] + origin[1], v->xyz[2] + origin[2], v->st[0], v->st[1]);
		}

		fp->WriteFloatString(" )\n");
	}

	fp->WriteFloatString("  )\n }\n}\n");

	return true;
}

/*
===============
idMapPatch::GetGeometryCRC
===============
*/
unsigned int idMapPatch::GetGeometryCRC(void) const
{
	int i, j;
	unsigned int crc;

	crc = GetHorzSubdivisions() ^ GetVertSubdivisions();

	for (i = 0; i < GetWidth(); i++) {
		for (j = 0; j < GetHeight(); j++) {
			crc ^= FloatCRC(verts[j * GetWidth() + i].xyz.x);
			crc ^= FloatCRC(verts[j * GetWidth() + i].xyz.y);
			crc ^= FloatCRC(verts[j * GetWidth() + i].xyz.z);
		}
	}

	crc ^= StringCRC(GetMaterial());

	return crc;
}

/*
=================
idMapBrush::Parse
=================
*/
#ifdef _RAVEN // _QUAKE4
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
idMapBrush *idMapBrush::Parse( Lexer &src, const idVec3 &origin, bool newFormat, int version )
// RAVEN END
#else
idMapBrush *idMapBrush::Parse(idLexer &src, const idVec3 &origin, bool newFormat, float version)
#endif
{
	int i;
	idVec3 planepts[3];
	idToken token;
	idList<idMapBrushSide *> sides;
	idMapBrushSide	*side;
	idDict epairs;

	if (!src.ExpectTokenString("{")) {
		return NULL;
	}

	do {
		if (!src.ReadToken(&token)) {
			src.Error("idMapBrush::Parse: unexpected EOF");
			sides.DeleteContents(true);
			return NULL;
		}

		if (token == "}") {
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do {
			// if token is a brace
			if (token == "(") {
				break;
			}

			// the token should be a key string for a key/value pair
			if (token.type != TT_STRING) {
				src.Error("idMapBrush::Parse: unexpected %s, expected ( or epair key string", token.c_str());
				sides.DeleteContents(true);
				return NULL;
			}

			idStr key = token;

			if (!src.ReadTokenOnLine(&token) || token.type != TT_STRING) {
				src.Error("idMapBrush::Parse: expected epair value string not found");
				sides.DeleteContents(true);
				return NULL;
			}

			epairs.Set(key, token);

			// try to read the next key
			if (!src.ReadToken(&token)) {
				src.Error("idMapBrush::Parse: unexpected EOF");
				sides.DeleteContents(true);
				return NULL;
			}
		} while (1);

		src.UnreadToken(&token);

		side = new idMapBrushSide();
		sides.Append(side);

		if (newFormat) {
			if (!src.Parse1DMatrix(4, side->plane.ToFloatPtr())) {
				src.Error("idMapBrush::Parse: unable to read brush side plane definition");
				sides.DeleteContents(true);
				return NULL;
			}
		} else {
			// read the three point plane definition
			if (!src.Parse1DMatrix(3, planepts[0].ToFloatPtr()) ||
			    !src.Parse1DMatrix(3, planepts[1].ToFloatPtr()) ||
			    !src.Parse1DMatrix(3, planepts[2].ToFloatPtr())) {
				src.Error("idMapBrush::Parse: unable to read brush side plane definition");
				sides.DeleteContents(true);
				return NULL;
			}

			planepts[0] -= origin;
			planepts[1] -= origin;
			planepts[2] -= origin;

			side->plane.FromPoints(planepts[0], planepts[1], planepts[2]);
		}

		// read the texture matrix
		// this is odd, because the texmat is 2D relative to default planar texture axis
		if (!src.Parse2DMatrix(2, 3, side->texMat[0].ToFloatPtr())) {
			src.Error("idMapBrush::Parse: unable to read brush side texture matrix");
			sides.DeleteContents(true);
			return NULL;
		}

		side->origin = origin;

		// read the material
		if (!src.ReadTokenOnLine(&token)) {
			src.Error("idMapBrush::Parse: unable to read brush side material");
			sides.DeleteContents(true);
			return NULL;
		}

		// we had an implicit 'textures/' in the old format...
#ifdef _RAVEN // _QUAKE4
		if (version < 2)
#else
		if (version < 2.0f)
#endif
		{
			side->material = "textures/" + token;
		} else {
			side->material = token;
		}

#ifdef _RAVEN
// RAVEN BEGIN
// jscott: make sure the material is properly parsed
		declManager->FindMaterial( token );
// RAVEN END
#endif

#ifdef _RAVEN
// RAVEN BEGIN
// jscott: removed these in later versions
		if( version < 3 )
		{
// RAVEN END
#endif
		// Q2 allowed override of default flags and values, but we don't any more
		if (src.ReadTokenOnLine(&token)) {
			if (src.ReadTokenOnLine(&token)) {
				if (src.ReadTokenOnLine(&token)) {
				}
			}
		}
#ifdef _RAVEN
		}
#endif
	} while (1);

	if (!src.ExpectTokenString("}")) {
		sides.DeleteContents(true);
		return NULL;
	}

	idMapBrush *brush = new idMapBrush();

	for (i = 0; i < sides.Num(); i++) {
		brush->AddSide(sides[i]);
	}

	brush->epairs = epairs;

	return brush;
}

/*
=================
idMapBrush::ParseQ3
=================
*/
#ifdef _RAVEN // _QUAKE4
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
idMapBrush *idMapBrush::ParseQ3( Lexer &src, const idVec3 &origin )
// RAVEN END
#else
idMapBrush *idMapBrush::ParseQ3(idLexer &src, const idVec3 &origin)
#endif
{
	int i, shift[2], rotate;
	float scale[2];
	idVec3 planepts[3];
	idToken token;
	idList<idMapBrushSide *> sides;
	idMapBrushSide	*side;
	idDict epairs;

	do {
		if (src.CheckTokenString("}")) {
			break;
		}

		side = new idMapBrushSide();
		sides.Append(side);

		// read the three point plane definition
		if (!src.Parse1DMatrix(3, planepts[0].ToFloatPtr()) ||
		    !src.Parse1DMatrix(3, planepts[1].ToFloatPtr()) ||
		    !src.Parse1DMatrix(3, planepts[2].ToFloatPtr())) {
			src.Error("idMapBrush::ParseQ3: unable to read brush side plane definition");
			sides.DeleteContents(true);
			return NULL;
		}

		planepts[0] -= origin;
		planepts[1] -= origin;
		planepts[2] -= origin;

		side->plane.FromPoints(planepts[0], planepts[1], planepts[2]);

		// read the material
		if (!src.ReadTokenOnLine(&token)) {
			src.Error("idMapBrush::ParseQ3: unable to read brush side material");
			sides.DeleteContents(true);
			return NULL;
		}

		// we have an implicit 'textures/' in the old format
		side->material = "textures/" + token;

		// read the texture shift, rotate and scale
		shift[0] = src.ParseInt();
		shift[1] = src.ParseInt();
		rotate = src.ParseInt();
		scale[0] = src.ParseFloat();
		scale[1] = src.ParseFloat();
		side->texMat[0] = idVec3(0.03125f, 0.0f, 0.0f);
		side->texMat[1] = idVec3(0.0f, 0.03125f, 0.0f);
		side->origin = origin;

		// Q2 allowed override of default flags and values, but we don't any more
		if (src.ReadTokenOnLine(&token)) {
			if (src.ReadTokenOnLine(&token)) {
				if (src.ReadTokenOnLine(&token)) {
				}
			}
		}
	} while (1);

	idMapBrush *brush = new idMapBrush();

	for (i = 0; i < sides.Num(); i++) {
		brush->AddSide(sides[i]);
	}

	brush->epairs = epairs;

	return brush;
}

/*
============
idMapBrush::Write
============
*/
bool idMapBrush::Write(idFile *fp, int primitiveNum, const idVec3 &origin) const
{
	int i;
	idMapBrushSide *side;

	fp->WriteFloatString("// primitive %d\n{\n brushDef3\n {\n", primitiveNum);

	// write brush epairs
	for (i = 0; i < epairs.GetNumKeyVals(); i++) {
		fp->WriteFloatString("  \"%s\" \"%s\"\n", epairs.GetKeyVal(i)->GetKey().c_str(), epairs.GetKeyVal(i)->GetValue().c_str());
	}

	// write brush sides
	for (i = 0; i < GetNumSides(); i++) {
		side = GetSide(i);
		fp->WriteFloatString("  ( %f %f %f %f ) ", side->plane[0], side->plane[1], side->plane[2], side->plane[3]);
#ifdef _RAVEN // _QUAKE4
// RAVEN BEGIN
		fp->WriteFloatString( "( ( %f %f %f ) ( %f %f %f ) ) \"%s\"\n",
// RAVEN END
#else
		fp->WriteFloatString("( ( %f %f %f ) ( %f %f %f ) ) \"%s\" 0 0 0\n",
#endif
		                     side->texMat[0][0], side->texMat[0][1], side->texMat[0][2],
		                     side->texMat[1][0], side->texMat[1][1], side->texMat[1][2],
		                     side->material.c_str());
	}

	fp->WriteFloatString(" }\n}\n");

	return true;
}

/*
===============
idMapBrush::GetGeometryCRC
===============
*/
unsigned int idMapBrush::GetGeometryCRC(void) const
{
	int i, j;
	idMapBrushSide *mapSide;
	unsigned int crc;

	crc = 0;

	for (i = 0; i < GetNumSides(); i++) {
		mapSide = GetSide(i);

		for (j = 0; j < 4; j++) {
			crc ^= FloatCRC(mapSide->GetPlane()[j]);
		}

		crc ^= StringCRC(mapSide->GetMaterial());
	}

	return crc;
}

/*
================
idMapEntity::Parse
================
*/
#ifdef _RAVEN // _QUAKE4
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
idMapEntity *idMapEntity::Parse( Lexer &src, bool worldSpawn, int version )
// RAVEN END
#else
idMapEntity *idMapEntity::Parse(idLexer &src, bool worldSpawn, float version)
#endif
{
	idToken	token;
	idMapEntity *mapEnt;
	idMapPatch *mapPatch;
	idMapBrush *mapBrush;
	bool worldent;
	idVec3 origin;
	double v1, v2, v3;

	if (!src.ReadToken(&token)) {
		return NULL;
	}

	if (token != "{") {
		src.Error("idMapEntity::Parse: { not found, found %s", token.c_str());
		return NULL;
	}

	mapEnt = new idMapEntity();

	if (worldSpawn) {
		mapEnt->primitives.Resize(1024, 256);
	}

	origin.Zero();
	worldent = false;

	do {
		if (!src.ReadToken(&token)) {
			src.Error("idMapEntity::Parse: EOF without closing brace");
			return NULL;
		}

		if (token == "}") {
			break;
		}

		if (token == "{") {
			// parse a brush or patch
			if (!src.ReadToken(&token)) {
				src.Error("idMapEntity::Parse: unexpected EOF");
				return NULL;
			}

			if (worldent) {
				origin.Zero();
			}

			// if is it a brush: brush, brushDef, brushDef2, brushDef3
			if (token.Icmpn("brush", 5) == 0) {
				mapBrush = idMapBrush::Parse(src, origin, (!token.Icmp("brushDef2") || !token.Icmp("brushDef3")), version);

				if (!mapBrush) {
					return NULL;
				}

				mapEnt->AddPrimitive(mapBrush);
			}
			// if is it a patch: patchDef2, patchDef3
			else if (token.Icmpn("patch", 5) == 0) {
				mapPatch = idMapPatch::Parse(src, origin, !token.Icmp("patchDef3"), version);

				if (!mapPatch) {
					return NULL;
				}

				mapEnt->AddPrimitive(mapPatch);
			}
			// assume it's a brush in Q3 or older style
			else {
				src.UnreadToken(&token);
				mapBrush = idMapBrush::ParseQ3(src, origin);

				if (!mapBrush) {
					return NULL;
				}

				mapEnt->AddPrimitive(mapBrush);
			}
		} else {
			idStr key, value;

			// parse a key / value pair
			key = token;
			src.ReadTokenOnLine(&token);
			value = token;

			// strip trailing spaces that sometimes get accidentally
			// added in the editor
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();

			mapEnt->epairs.Set(key, value);

			if (!idStr::Icmp(key, "origin")) {
				// scanf into doubles, then assign, so it is idVec size independent
				v1 = v2 = v3 = 0;
				sscanf(value, "%lf %lf %lf", &v1, &v2, &v3);
				origin.x = v1;
				origin.y = v2;
				origin.z = v3;
			} else if (!idStr::Icmp(key, "classname") && !idStr::Icmp(value, "worldspawn")) {
				worldent = true;
			}
		}
	} while (1);

	return mapEnt;
}

/*
============
idMapEntity::Write
============
*/
bool idMapEntity::Write(idFile *fp, int entityNum) const
{
	int i;
	idMapPrimitive *mapPrim;
	idVec3 origin;

	fp->WriteFloatString("// entity %d\n{\n", entityNum);

	// write entity epairs
	for (i = 0; i < epairs.GetNumKeyVals(); i++) {
		fp->WriteFloatString("\"%s\" \"%s\"\n", epairs.GetKeyVal(i)->GetKey().c_str(), epairs.GetKeyVal(i)->GetValue().c_str());
	}

	epairs.GetVector("origin", "0 0 0", origin);

	// write pritimives
	for (i = 0; i < GetNumPrimitives(); i++) {
		mapPrim = GetPrimitive(i);

		switch (mapPrim->GetType()) {
			case idMapPrimitive::TYPE_BRUSH:
				static_cast<idMapBrush *>(mapPrim)->Write(fp, i, origin);
				break;
			case idMapPrimitive::TYPE_PATCH:
				static_cast<idMapPatch *>(mapPrim)->Write(fp, i, origin);
				break;
		}
	}

	fp->WriteFloatString("}\n");

	return true;
}

/*
===============
idMapEntity::RemovePrimitiveData
===============
*/
void idMapEntity::RemovePrimitiveData()
{
	primitives.DeleteContents(true);
}

/*
===============
idMapEntity::GetGeometryCRC
===============
*/
unsigned int idMapEntity::GetGeometryCRC(void) const
{
	int i;
	unsigned int crc;
	idMapPrimitive	*mapPrim;

	crc = 0;

	for (i = 0; i < GetNumPrimitives(); i++) {
		mapPrim = GetPrimitive(i);

		switch (mapPrim->GetType()) {
			case idMapPrimitive::TYPE_BRUSH:
				crc ^= static_cast<idMapBrush *>(mapPrim)->GetGeometryCRC();
#ifdef _RAVEN
				if ( epairs.GetString( "model" ) ) {
					crc ^= StringCRC( epairs.GetString( "model" ) );
				}
#endif
				break;
			case idMapPrimitive::TYPE_PATCH:
				crc ^= static_cast<idMapPatch *>(mapPrim)->GetGeometryCRC();
#ifdef _RAVEN
				if ( epairs.GetString( "model" ) ) {
					crc ^= StringCRC( epairs.GetString( "model" ) );
				}
#endif
				break;
		}
	}

	return crc;
}

/*
===============
idMapFile::Parse
===============
*/
bool idMapFile::Parse(const char *filename, bool ignoreRegion, bool osPath)
{
	// no string concatenation for epairs and allow path names for materials
#ifdef _RAVEN // _QUAKE4
// RAVEN BEGIN
// jsinger: Done this way to reduce the amount of code change.  The auto pointer will
// delete the lexer when this method exits
	idAutoPtr<Lexer> lexer(LexerFactory::MakeLexer( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES ));
	Lexer &src(*lexer);
// RAVEN END
#else
	idLexer src(LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES);
#endif
	idToken token;
	idStr fullName;
	idMapEntity *mapEnt;
	int i, j, k;

	name = filename;
	name.StripFileExtension();
	fullName = name;
	hasPrimitiveData = false;

	if (!ignoreRegion) {
		// try loading a .reg file first
		fullName.SetFileExtension("reg");
		src.LoadFile(fullName, osPath);
	}

	if (!src.IsLoaded()) {
		// now try a .map file
		fullName.SetFileExtension("map");
		src.LoadFile(fullName, osPath);

		if (!src.IsLoaded()) {
			// didn't get anything at all
			return false;
		}
	}

	version = OLD_MAP_VERSION;
	fileTime = src.GetFileTime();
	entities.DeleteContents(true);

	if (src.CheckTokenString("Version")) {
		src.ReadTokenOnLine(&token);
#ifdef _RAVEN // _QUAKE4
		version = token.GetIntValue();
#else
		version = token.GetFloatValue();
#endif
	}

	while (1) {
		mapEnt = idMapEntity::Parse(src, (entities.Num() == 0), version);

		if (!mapEnt) {
			break;
		}

#ifdef _RAVEN
// RAVEN BEGIN
// rhummer: Check to see if there are func_groups in the map file.
		if ( !mHasFuncGroups && !idStr::Icmp( mapEnt->epairs.GetString( "classname" ), "func_group" ) ) {
			mHasFuncGroups = true;
		}
// RAVEN END
#endif

		entities.Append(mapEnt);
	}

#ifdef _RAVEN
    ParseExport(filename, osPath);
#endif

	SetGeometryCRC();

#ifdef _RAVEN // _QUAKE4
// RAVEN BEGIN
    mHasBeenResolved = false;
// RAVEN END
#else
	// if the map has a worldspawn
	if (entities.Num()) {

		// "removeEntities" "classname" can be set in the worldspawn to remove all entities with the given classname
		const idKeyValue *removeEntities = entities[0]->epairs.MatchPrefix("removeEntities", NULL);

		while (removeEntities) {
			RemoveEntities(removeEntities->GetValue());
			removeEntities = entities[0]->epairs.MatchPrefix("removeEntities", removeEntities);
		}

		// "overrideMaterial" "material" can be set in the worldspawn to reset all materials
		idStr material;

		if (entities[0]->epairs.GetString("overrideMaterial", "", material)) {
			for (i = 0; i < entities.Num(); i++) {
				mapEnt = entities[i];

				for (j = 0; j < mapEnt->GetNumPrimitives(); j++) {
					idMapPrimitive *mapPrimitive = mapEnt->GetPrimitive(j);

					switch (mapPrimitive->GetType()) {
						case idMapPrimitive::TYPE_BRUSH: {
							idMapBrush *mapBrush = static_cast<idMapBrush *>(mapPrimitive);

							for (k = 0; k < mapBrush->GetNumSides(); k++) {
								mapBrush->GetSide(k)->SetMaterial(material);
							}

							break;
						}
						case idMapPrimitive::TYPE_PATCH: {
							static_cast<idMapPatch *>(mapPrimitive)->SetMaterial(material);
							break;
						}
					}
				}
			}
		}

		// force all entities to have a name key/value pair
		if (entities[0]->epairs.GetBool("forceEntityNames")) {
			for (i = 1; i < entities.Num(); i++) {
				mapEnt = entities[i];

				if (!mapEnt->epairs.FindKey("name")) {
					mapEnt->epairs.Set("name", va("%s%d", mapEnt->epairs.GetString("classname", "forcedName"), i));
				}
			}
		}

		// move the primitives of any func_group entities to the worldspawn
		if (entities[0]->epairs.GetBool("moveFuncGroups")) {
			for (i = 1; i < entities.Num(); i++) {
				mapEnt = entities[i];

				if (idStr::Icmp(mapEnt->epairs.GetString("classname"), "func_group") == 0) {
					entities[0]->primitives.Append(mapEnt->primitives);
					mapEnt->primitives.Clear();
				}
			}
		}
	}
#endif

	hasPrimitiveData = true;
	return true;
}

/*
============
idMapFile::Write
============
*/
bool idMapFile::Write(const char *fileName, const char *ext, bool fromBasePath)
{
	int i;
	idStr qpath;
	idFile *fp;

	qpath = fileName;
	qpath.SetFileExtension(ext);

	idLib::common->Printf("writing %s...\n", qpath.c_str());

	if (fromBasePath) {
		fp = idLib::fileSystem->OpenFileWrite(qpath, "fs_devpath");
	} else {
		fp = idLib::fileSystem->OpenExplicitFileWrite(qpath);
	}

	if (!fp) {
		idLib::common->Warning("Couldn't open %s\n", qpath.c_str());
		return false;
	}

	fp->WriteFloatString("Version %f\n", (float) CURRENT_MAP_VERSION);

	for (i = 0; i < entities.Num(); i++) {
		entities[i]->Write(fp, i);
	}

	idLib::fileSystem->CloseFile(fp);

	return true;
}

/*
===============
idMapFile::SetGeometryCRC
===============
*/
void idMapFile::SetGeometryCRC(void)
{
	int i;

	geometryCRC = 0;

	for (i = 0; i < entities.Num(); i++) {
		geometryCRC ^= entities[i]->GetGeometryCRC();
	}
}

/*
===============
idMapFile::AddEntity
===============
*/
int idMapFile::AddEntity(idMapEntity *mapEnt)
{
	int ret = entities.Append(mapEnt);
	return ret;
}

/*
===============
idMapFile::FindEntity
===============
*/
idMapEntity *idMapFile::FindEntity(const char *name)
{
	for (int i = 0; i < entities.Num(); i++) {
		idMapEntity *ent = entities[i];

		if (idStr::Icmp(ent->epairs.GetString("name"), name) == 0) {
			return ent;
		}
	}

	return NULL;
}

/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntity(idMapEntity *mapEnt)
{
	entities.Remove(mapEnt);
	delete mapEnt;
}

/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntities(const char *classname)
{
	for (int i = 0; i < entities.Num(); i++) {
		idMapEntity *ent = entities[i];

		if (idStr::Icmp(ent->epairs.GetString("classname"), classname) == 0) {
			delete entities[i];
			entities.RemoveIndex(i);
			i--;
		}
	}
}

/*
===============
idMapFile::RemoveAllEntities
===============
*/
void idMapFile::RemoveAllEntities()
{
	entities.DeleteContents(true);
	hasPrimitiveData = false;
}

/*
===============
idMapFile::RemovePrimitiveData
===============
*/
void idMapFile::RemovePrimitiveData()
{
	for (int i = 0; i < entities.Num(); i++) {
		idMapEntity *ent = entities[i];
		ent->RemovePrimitiveData();
	}

	hasPrimitiveData = false;
}

/*
===============
idMapFile::NeedsReload
===============
*/
bool idMapFile::NeedsReload()
{
	if (name.Length()) {
		ID_TIME_T time = (ID_TIME_T)-1;

		if (idLib::fileSystem->ReadFile(name, NULL, &time) > 0) {
			return (time > fileTime);
		}
	}

	return true;
}

#ifdef _RAVEN

// RAVEN BEGIN
// rjohnson: added resolve for handling func_groups and other aspects.  Before, radiant would do this processing on a map destroying the original data
void idMapFile::Resolve( void )
{
	int i, j, k;
	idMapEntity *mapEnt;

	if ( !hasPrimitiveData ) {
		return;
	}

	// if the map has a worldspawn
	if ( entities.Num() ) {

		// "removeEntities" "classname" can be set in the worldspawn to remove all entities with the given classname
		const idKeyValue *removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", NULL );
		while ( removeEntities ) {
			RemoveEntities( removeEntities->GetValue() );
			removeEntities = entities[0]->epairs.MatchPrefix( "removeEntities", removeEntities );
		}

		// "overrideMaterial" "material" can be set in the worldspawn to reset all materials
		idStr material;
		if ( entities[0]->epairs.GetString( "overrideMaterial", "", material ) ) {
			for ( i = 0; i < entities.Num(); i++ ) {
				mapEnt = entities[i];
				for ( j = 0; j < mapEnt->GetNumPrimitives(); j++ ) {
					idMapPrimitive *mapPrimitive = mapEnt->GetPrimitive( j );
					switch( mapPrimitive->GetType() ) {
						case idMapPrimitive::TYPE_BRUSH: {
							idMapBrush *mapBrush = static_cast<idMapBrush *>(mapPrimitive);
							for ( k = 0; k < mapBrush->GetNumSides(); k++ ) {
								mapBrush->GetSide( k )->SetMaterial( material );
							}
							break;
						}
						case idMapPrimitive::TYPE_PATCH: {
							static_cast<idMapPatch *>(mapPrimitive)->SetMaterial( material );
							break;
						}
					}
				}
			}
		}

		// force all entities to have a name key/value pair
		if ( entities[0]->epairs.GetBool( "forceEntityNames" ) ) {
			for ( i = 1; i < entities.Num(); i++ ) {
				mapEnt = entities[i];
				if ( !mapEnt->epairs.FindKey( "name" ) ) {
					mapEnt->epairs.Set( "name", va( "%s%d", mapEnt->epairs.GetString( "classname", "forcedName" ), i ) );
				}
			}
		}

		// move the primitives of any func_group entities to the worldspawn
		for ( i = 1; i < entities.Num(); i++ ) {
			mapEnt = entities[i];
			if ( idStr::Icmp( mapEnt->epairs.GetString( "classname" ), "func_group" ) == 0 ) {
				idVec3	delta;
				
				mapEnt->epairs.GetVector( "origin", "0 0 0", delta );

				for( j = 0; j < mapEnt->primitives.Num(); j++ ) {
					idMapPrimitive	*mapPrim = mapEnt->primitives[j];

					mapPrim->AdjustOrigin( delta );
				}
				entities[0]->primitives.Append( mapEnt->primitives );
				mapEnt->primitives.Clear();
			}
		}
		RemoveEntities( "func_group" );
	}
// rhummer:
	mHasBeenResolved = true;
	if ( cvarSystem->GetCVarBool( "developer" ) ) {
		common->Printf( "^2idMapFile::Resolve has been run on ^0%s^2 so no func_groups exist.\n", name.c_str() );
	}
}

// rjohnson: added export
bool idMapFile::WriteExport( const char *fileName, bool fromBasePath )
{
	int			i, j;
	idDict		newPairs;

	for ( i = 0; i < entities.Num(); i++ ) 
	{
		idMapEntity *ent = entities[i];

		if ( ent->epairs.GetInt("export") == 1 )
		{
/*			for ( j = 0; j < ent->epairs.GetNumKeyVals(); j++) 
			{
				const idKeyValue	*kv = ent->epairs.GetKeyVal(j);

				if (kv->key.CmpPrefix("export_") == 0)
				{
					ent->epairs.Delete(kv->key.c_str());
					j--;
					continue;
				}
			}
*/
			newPairs.Clear();

			for ( j = 0; j < ent->epairs.GetNumKeyVals(); j++) 
			{
				const idKeyValue	*kv = ent->epairs.GetKeyVal(j);

				if (kv->GetKey().CmpPrefix("export_") != 0)
				{
					char				newName[1024];

					sprintf(newName, "export_%s", kv->GetKey().c_str());

					if (!ent->epairs.GetString(newName, 0))
					{	// don't overwrite an existing export
						newPairs.Set (newName, kv->GetValue().c_str());
					}
				}
			}

			ent->epairs.Copy(newPairs);
		}
	}

	return Write(fileName, "export", fromBasePath, true);
}

// rjohnson: added export
/*
============
idMapFile::Write
============
*/
bool idMapFile::Write( const char *fileName, const char *ext, bool fromBasePath, bool exportOnly ) {
	int i;
	idStr qpath;
	idFile *fp;
// RAVEN BEGIN
// rhummer: Used to verify func_groups didn't disappear somehow.
	bool funcGroupFound = false;
// RAVEN END

	qpath = fileName;
	qpath.SetFileExtension( ext );

	idLib::common->Printf( "writing %s...\n", qpath.c_str() );

	if ( fromBasePath ) {
		fp = idLib::fileSystem->OpenFileWrite( qpath, "fs_devpath" );
	}
	else {
		fp = idLib::fileSystem->OpenExplicitFileWrite( qpath );
	}

	if ( !fp ) {
		idLib::common->Warning( "Couldn't open %s\n", qpath.c_str() );
		return false;
	}

	fp->WriteFloatString( "Version %d\n", CURRENT_MAP_VERSION );

	if (exportOnly)
	{
		for ( i = 0; i < entities.Num(); i++ )
		{
			if (entities[i]->epairs.GetInt("export"))
			{
				entities[i]->Write( fp, i );
			}
		}
	}
	else
	{
		for ( i = 0; i < entities.Num(); i++ ) {
			entities[i]->Write( fp, i );
// RAVEN BEGIN
// rhummer: See if there are func_groups, there should be if there was during loading..
			if ( !idStr::Icmp( entities[i]->epairs.GetString( "classname" ), "func_group" ) ) {
				funcGroupFound = true;
			}
// RAVEN END
		}
	}

// RAVEN BEGIN
// rhummer: If this is true..something bad happened to cause all func_groups to go away between loading and saving.
	if ( mHasFuncGroups && !funcGroupFound && cvarSystem->GetCVarBool( "developer" ) ) {
		common->Warning( "^5Did not find any ^2func_groups^0 during save, but there were ^2func_groups^0 found during loading.  Was this intended?\n");
	}
// RAVEN END

	idLib::fileSystem->CloseFile( fp );

	return true;
}
// RAVEN END


bool idMapFile::ParseExport( const char *filename, bool osPath )
{
    // no string concatenation for epairs and allow path names for materials
// RAVEN BEGIN
// jsinger: Done this way to reduce the amount of code to change.
//          The auto pointer will delete the lexer when this method exits.
    idAutoPtr<Lexer>		lexer(LexerFactory::MakeLexer( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES ) );
    Lexer &src(*lexer);
// RAVEN END
    idToken		token;
    idStr		fullName;
    idMapEntity *mapEnt;
    int			i, j;
    int			numMerged, numNotMerged, numAdded, numRemoved;

    numMerged = numNotMerged = numAdded = numRemoved = 0;
    mHasExportEntities = false;

    fullName = filename;
    fullName.StripFileExtension();
    fullName.SetFileExtension( "export" );
    src.LoadFile( fullName, osPath );
    if ( !src.IsLoaded() )
    {
        // didn't get anything at all
        return false;
    }

    version = OLD_MAP_VERSION;
    fileTime = src.GetFileTime();
    mExportEntities.DeleteContents( true );

    if ( src.CheckTokenString( "Version" ) )
    {
        src.ReadTokenOnLine( &token );
        version = token.GetIntValue();
    }

    while( 1 )
    {
        mapEnt = idMapEntity::Parse( src, false, version );
        if ( !mapEnt )
        {
            break;
        }
        mExportEntities.Append( mapEnt );
    }

    if (mExportEntities.Num())
    {
        mHasExportEntities = true;
    }

    for(i=0; i<mExportEntities.Num(); i++)
    {
        idMapEntity *ent = mExportEntities[i];
        idMapEntity *mapEnt;
        const char	*name;

        name = ent->epairs.GetString("name");
        mapEnt = FindEntity(name);

        if (!mapEnt)
        {
            // check to see if we have an export_name
            name = ent->epairs.GetString("export_name", 0);
            if (!name || ent->epairs.GetInt("original") )
            {
                // this is a new entity
                ent->epairs.SetInt("export", 2);			// 2 = when re-exporting, don't duplicated the export_ fields
                ent->epairs.SetInt("merged", 1);
                ent->epairs.SetInt("original", 0);			// 0 = now the entity in map is always treated as master
                entities.Append(ent);
                mExportEntities.RemoveIndex(i);
                i--;
                numAdded++;
                continue;
            }
            else
            {
                mapEnt = FindEntity(name);
            }
        }

        if (!mapEnt)
        {
            // this export entity has been removed from the original map
            numNotMerged++;
        }
        else
        {
            // exists in both, do a merge!
            mapEnt->epairs.SetInt("export", 2);			// 2 = when re-exporting, don't duplicated the export_ fields
            mapEnt->epairs.SetInt("merged", 1);
            for ( j = 0; j < ent->epairs.GetNumKeyVals(); j++)
            {
                char				origName[1024];
                const idKeyValue	*exportKV = ent->epairs.GetKeyVal(j);
                const idKeyValue	*origKV, *mapKV;

                if (exportKV->GetKey().CmpPrefix("export_") != 0)
                {
                    sprintf(origName, "export_%s", exportKV->GetKey().c_str());
                    origKV = ent->epairs.FindKey(origName);
                    mapKV = mapEnt->epairs.FindKey(exportKV->GetKey().c_str());

                    if (origKV && mapKV)
                    {
                        // keys exist in all spots, compare
                        if (origKV->GetValue() == exportKV->GetValue())
                        {
                            // export hasn't change
                        }
                        else if (mapKV->GetValue() == origKV->GetValue())
                        {
                            // map is same as the orig, yet export has changed, so bring it over
                            mapEnt->epairs.Set(exportKV->GetKey().c_str(), exportKV->GetValue().c_str());
                        }
                    }
                    else if (origKV)
                    {
                        // map has removed this key, so we shouldn't merge it
                    }
                    else if (mapKV)
                    {
                        // map has added this key, so we'll ignore the exported value, which has also been added
                    }
                    else
                    {
                        // this was added to the export, so merge it in
                        mapEnt->epairs.Set(exportKV->GetKey().c_str(), exportKV->GetValue().c_str());
                    }
                }
                else
                {
                    mapEnt->epairs.Set(exportKV->GetKey().c_str(), exportKV->GetValue().c_str());
                }
            }
            numMerged++;
        }
    }

    // check for any entities that have been marked as exported in the original map but have been removed from the export file
    for(i=0; i<entities.Num(); i++)
    {
        idMapEntity *ent = entities[i];

        if (ent->epairs.GetInt("export"))
        {
            if (!ent->epairs.GetInt("merged"))
            {
                entities.RemoveIndex(i);
                numRemoved++;
                i--;
            }
            else
            {
                ent->epairs.Delete("merged");
            }
        }
    }

    common->Printf("Export Merge: %d added, %d merged, %d not merged, %d removed\n", numAdded, numMerged, numNotMerged, numRemoved);

    mExportEntities.DeleteContents( true );

    return true;
}

// RAVEN BEGIN
// rjohnson: added resolve for handling func_groups and other aspects.  Before, radiant would do this processing on a map destroying the original data
/*
===============
idMapPatch::AdjustOrigin
===============
*/
void idMapPatch::AdjustOrigin( idVec3 &delta ) {
	TranslateSelf( delta );
}
// RAVEN END

// RAVEN BEGIN
// rjohnson: added resolve for handling func_groups and other aspects.  Before, radiant would do this processing on a map destroying the original data

// This is taken from tools/radiant/EditorBrushPrimit.cpp
float SarrusDet(idVec3 a, idVec3 b, idVec3 c) {
	return (float)a[0] * (float)b[1] * (float)c[2] + (float)b[0] * (float)c[1] * (float)a[2] + (float)c[0] * (float)a[1] * (float)b[2] - (float)c[0] * (float)b[1] * (float)a[2] - (float)a[1] * (float)b[0] * (float)c[2] -	(float)a[0] * (float)b[2] * (float)c[1];
}

/*
===============
idMapBrush::AdjustOrigin
===============
*/
void idMapBrush::AdjustOrigin( idVec3 &delta ) {
	int				i;
	idMapBrushSide	*mapSide;

	for ( i = 0; i < GetNumSides(); i++ ) {
		mapSide = GetSide(i);

		mapSide->SetPlane( mapSide->GetPlane().Translate( delta ) );

		// This is taken from Face_MoveTexture_BrushPrimit() in tools/radiant/EditorBrushPrimit.cpp
		idVec3	texS, texT;
		float	tx, ty;
		idVec3	M[3];	// columns of the matrix .. easier that way
		float	det;
		idVec3	D[2];

		// compute plane axis base ( doesn't change with translation )
		ComputeAxisBase( mapSide->GetPlane().Normal(), texS, texT);

		// compute translation vector in plane axis base
		tx = DotProduct(delta, texS);
		ty = DotProduct(delta, texT);

		// fill the data vectors
		M[0][0] = tx;
		M[0][1] = 1.0f + tx;
		M[0][2] = tx;
		M[1][0] = ty;
		M[1][1] = ty;
		M[1][2] = 1.0f + ty;
		M[2][0] = 1.0f;
		M[2][1] = 1.0f;
		M[2][2] = 1.0f;

		idVec3	tm[2];
		mapSide->GetTextureMatrix( tm[0], tm[1] );

		D[0][0] = tm[0][2];
		D[0][1] = tm[0][0] + tm[0][2];
		D[0][2] = tm[0][1] + tm[0][2];
		D[1][0] = tm[1][2];
		D[1][1] = tm[1][0] + tm[1][2];
		D[1][2] = tm[1][1] + tm[1][2];

		// solve
		det = SarrusDet(M[0], M[1], M[2]);
		if ( det != 0. ) {
			tm[0][0] = SarrusDet(D[0], M[1], M[2]) / det;
			tm[0][1] = SarrusDet(M[0], D[0], M[2]) / det;
			tm[0][2] = SarrusDet(M[0], M[1], D[0]) / det;
			tm[1][0] = SarrusDet(D[1], M[1], M[2]) / det;
			tm[1][1] = SarrusDet(M[0], D[1], M[2]) / det;
			tm[1][2] = SarrusDet(M[0], M[1], D[1]) / det;
			mapSide->SetTextureMatrix(tm);
		}
	}
}
// RAVEN END

#endif

