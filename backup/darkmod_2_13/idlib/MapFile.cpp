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


/*
===============
FloatCRC
===============
*/
ID_INLINE unsigned int FloatCRC( float f ) {
	return *(unsigned int *)&f;
}

/*
===============
StringCRC
===============
*/
ID_INLINE unsigned int StringCRC( const char *str ) {
	unsigned int i, crc;

	crc = 0;
	for ( i = 0; str[i]; i++ ) {
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
static void ComputeAxisBase( const idVec3 &normal, idVec3 &texS, idVec3 &texT ) {
	float RotY, RotZ;
	idVec3 n;

	// do some cleaning
	n[0] = ( idMath::Fabs( normal[0] ) < 1e-6f ) ? 0.0f : normal[0];
	n[1] = ( idMath::Fabs( normal[1] ) < 1e-6f ) ? 0.0f : normal[1];
	n[2] = ( idMath::Fabs( normal[2] ) < 1e-6f ) ? 0.0f : normal[2];

	RotY = -atan2( n[2], idMath::Sqrt( n[1] * n[1] + n[0] * n[0]) );
	RotZ = atan2( n[1], n[0] );
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
void idMapBrushSide::GetTextureVectors( idVec4 v[2] ) const {
	int i;
	idVec3 texX, texY;

	ComputeAxisBase( plane.Normal(), texX, texY );
	for ( i = 0; i < 2; i++ ) {
		v[i][0] = texX[0] * texMat[i][0] + texY[0] * texMat[i][1];
		v[i][1] = texX[1] * texMat[i][0] + texY[1] * texMat[i][1];
		v[i][2] = texX[2] * texMat[i][0] + texY[2] * texMat[i][1];
		v[i][3] = texMat[i][2] + ( origin * v[i].ToVec3() );
	}
}

/*
=================
idMapPatch::Parse
=================
*/
idMapPatch *idMapPatch::Parse( idLexer &src, const idVec3 &origin, bool patchDef3, float version ) {
	float		info[7];
	idDrawVert *vert;
	idToken		token;
	int			i, j;

	if ( !src.ExpectTokenString( "{" ) ) {
		return NULL;
	}

	// read the material (we had an implicit 'textures/' in the old format...)
	if ( !src.ReadToken( &token ) ) {
		src.Error( "idMapPatch::Parse: unexpected EOF" );
		return NULL;
	}

	// Parse it
	if (patchDef3) {
		if ( !src.Parse1DMatrix( 7, info ) ) {
			src.Error( "idMapPatch::Parse: unable to Parse patchDef3 info" );
			return NULL;
		}
	} else {
		if ( !src.Parse1DMatrix( 5, info ) ) {
			src.Error( "idMapPatch::Parse: unable to parse patchDef2 info" );
			return NULL;
		}
	}

	idMapPatch *patch = new idMapPatch( info[0], info[1] );
	patch->SetSize( info[0], info[1] );
	if ( version < 2.0f ) {
		patch->SetMaterial( "textures/" + token );
	} else {
		patch->SetMaterial( token );
	}

	if ( patchDef3 ) {
		patch->SetHorzSubdivisions( info[2] );
		patch->SetVertSubdivisions( info[3] );
		patch->SetExplicitlySubdivided( true );
	}

	if ( patch->GetWidth() < 0 || patch->GetHeight() < 0 ) {
		src.Error( "idMapPatch::Parse: bad size" );
		delete patch;
		return NULL;
	}

	// these were written out in the wrong order, IMHO
	if ( !src.ExpectTokenString( "(" ) ) {
		src.Error( "idMapPatch::Parse: bad patch vertex data" );
		delete patch;
		return NULL;
	}
	for ( j = 0; j < patch->GetWidth(); j++ ) {
		if ( !src.ExpectTokenString( "(" ) ) {
			src.Error( "idMapPatch::Parse: bad vertex row data" );
			delete patch;
			return NULL;
		}
		for ( i = 0; i < patch->GetHeight(); i++ ) {
			float v[5];

			if ( !src.Parse1DMatrix( 5, v ) ) {
				src.Error( "idMapPatch::Parse: bad vertex column data" );
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
		if ( !src.ExpectTokenString( ")" ) ) {
			delete patch;
			src.Error( "idMapPatch::Parse: unable to parse patch control points" );
			return NULL;
		}
	}
	if ( !src.ExpectTokenString( ")" ) ) {
		src.Error( "idMapPatch::Parse: unable to parse patch control points, no closure" );
		delete patch;
		return NULL;
	}

	// read any key/value pairs
	while( src.ReadToken( &token ) ) {
		if ( token == "}" ) {
			src.ExpectTokenString( "}" );
			break;
		}
		if ( token.type == TT_STRING ) {
			idStr key = token;
			src.ExpectTokenType( TT_STRING, 0, &token );
			patch->epairs.Set( key, token );
		}
	}

	return patch;
}

/*
============
idMapPatch::Write
============
*/
bool idMapPatch::Write( idFile *fp, int primitiveNum, const idVec3 &origin ) const {
	int i, j;
	const idDrawVert *v;

	if ( GetExplicitlySubdivided() ) {
		fp->WriteFloatString( "// primitive %d\n{\n patchDef3\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight(), GetHorzSubdivisions(), GetVertSubdivisions());
	} else {
		fp->WriteFloatString( "// primitive %d\n{\n patchDef2\n {\n", primitiveNum );
		fp->WriteFloatString( "  \"%s\"\n  ( %d %d 0 0 0 )\n", GetMaterial(), GetWidth(), GetHeight());
	}

	fp->WriteFloatString( "  (\n" );
	for ( i = 0; i < GetWidth(); i++ ) {
		fp->WriteFloatString( "   ( " );
		for ( j = 0; j < GetHeight(); j++ ) {
			v = &verts[ j * GetWidth() + i ];
			fp->WriteFloatString( " ( %f %f %f %f %f )", v->xyz[0] + origin[0],
								v->xyz[1] + origin[1], v->xyz[2] + origin[2], v->st[0], v->st[1] );
		}
		fp->WriteFloatString( " )\n" );
	}
	fp->WriteFloatString( "  )\n }\n}\n" );

	return true;
}

/*
===============
idMapPatch::GetGeometryCRC
===============
*/
unsigned int idMapPatch::GetGeometryCRC( void ) const {
	int i, j;
	unsigned int crc;

	crc = GetHorzSubdivisions() ^ GetVertSubdivisions();
	for ( i = 0; i < GetWidth(); i++ ) {
		for ( j = 0; j < GetHeight(); j++ ) {
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.x );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.y );
			crc ^= FloatCRC( verts[j * GetWidth() + i].xyz.z );
		}
	}

	crc ^= StringCRC( GetMaterial() );

	return crc;
}

/*
=================
idMapBrush::Parse
=================
*/
idMapBrush *idMapBrush::Parse( idLexer &src, const idVec3 &origin, bool newFormat, float version ) {
	int i;
	idVec3 planepts[3];
	idToken token;
	idList<idMapBrushSide*> sides;
	idMapBrushSide	*side;
	idDict epairs;

	if ( !src.ExpectTokenString( "{" ) ) {
		return NULL;
	}

	do {
		if ( !src.ReadToken( &token ) ) {
			src.Error( "idMapBrush::Parse: unexpected EOF" );
			sides.DeleteContents( true );
			return NULL;
		}
		if ( token == "}" ) {
			break;
		}

		// here we may have to jump over brush epairs ( only used in editor )
		do {
			// if token is a brace
			if ( token == "(" ) {
				break;
			}
			// the token should be a key string for a key/value pair
			if ( token.type != TT_STRING ) {
				src.Error( "idMapBrush::Parse: unexpected %s, expected ( or epair key string", token.c_str() );
				sides.DeleteContents( true );
				return NULL;
			}

			idStr key = token;

			if ( !src.ReadTokenOnLine( &token ) || token.type != TT_STRING ) {
				src.Error( "idMapBrush::Parse: expected epair value string not found" );
				sides.DeleteContents( true );
				return NULL;
			}

			epairs.Set( key, token );

			// try to read the next key
			if ( !src.ReadToken( &token ) ) {
				src.Error( "idMapBrush::Parse: unexpected EOF" );
				sides.DeleteContents( true );
				return NULL;
			}
		} while (1);

		src.UnreadToken( &token );

		side = new idMapBrushSide();
		sides.Append(side);

		if ( newFormat ) {
			if ( !src.Parse1DMatrix( 4, side->plane.ToFloatPtr() ) ) {
				src.Error( "idMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return NULL;
			}
		} else {
			// read the three point plane definition
			if (!src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
				!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) ) {
				src.Error( "idMapBrush::Parse: unable to read brush side plane definition" );
				sides.DeleteContents( true );
				return NULL;
			}

			planepts[0] -= origin;
			planepts[1] -= origin;
			planepts[2] -= origin;

			side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );
		}

		// read the texture matrix
		// this is odd, because the texmat is 2D relative to default planar texture axis
		if ( !src.Parse2DMatrix( 2, 3, side->texMat[0].ToFloatPtr() ) ) {
			src.Error( "idMapBrush::Parse: unable to read brush side texture matrix" );
			sides.DeleteContents( true );
			return NULL;
		}
		side->origin = origin;
		
		// read the material
		if ( !src.ReadTokenOnLine( &token ) ) {
			src.Error( "idMapBrush::Parse: unable to read brush side material" );
			sides.DeleteContents( true );
			return NULL;
		}

		// we had an implicit 'textures/' in the old format...
		if ( version < 2.0f ) {
			side->material = "textures/" + token;
		} else {
			side->material = token;
		}

		// Q2 allowed override of default flags and values, but we don't any more
		if ( src.ReadTokenOnLine( &token ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				if ( src.ReadTokenOnLine( &token ) ) {
				}
			}
		}
	} while( 1 );

	if ( !src.ExpectTokenString( "}" ) ) {
		sides.DeleteContents( true );
		return NULL;
	}

	idMapBrush *brush = new idMapBrush();
	for ( i = 0; i < sides.Num(); i++ ) {
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
=================
idMapBrush::ParseQ3
=================
*/
idMapBrush *idMapBrush::ParseQ3( idLexer &src, const idVec3 &origin ) {
    int i;
	idVec3 planepts[3];
	idToken token;
	idList<idMapBrushSide*> sides;
	idMapBrushSide	*side;
	idDict epairs;

	do {
		if ( src.CheckTokenString( "}" ) ) {
			break;
		}

		side = new idMapBrushSide();
		sides.Append( side );

		// read the three point plane definition
		if (!src.Parse1DMatrix( 3, planepts[0].ToFloatPtr() ) ||
			!src.Parse1DMatrix( 3, planepts[1].ToFloatPtr() ) ||
			!src.Parse1DMatrix( 3, planepts[2].ToFloatPtr() ) ) {
			src.Error( "idMapBrush::ParseQ3: unable to read brush side plane definition" );
			sides.DeleteContents( true );
			return NULL;
		}

		planepts[0] -= origin;
		planepts[1] -= origin;
		planepts[2] -= origin;

		side->plane.FromPoints( planepts[0], planepts[1], planepts[2] );

		// read the material
		if ( !src.ReadTokenOnLine( &token ) ) {
			src.Error( "idMapBrush::ParseQ3: unable to read brush side material" );
			sides.DeleteContents( true );
			return NULL;
		}

		// we have an implicit 'textures/' in the old format
		side->material = "textures/" + token;

        // skip the texture shift, rotate and scale
        src.ParseInt();
        src.ParseInt();
        src.ParseInt();
        src.ParseFloat();
        src.ParseFloat();
		side->texMat[0] = idVec3( 0.03125f, 0.0f, 0.0f );
		side->texMat[1] = idVec3( 0.0f, 0.03125f, 0.0f );
		side->origin = origin;
		
		// Q2 allowed override of default flags and values, but we don't any more
		if ( src.ReadTokenOnLine( &token ) ) {
			if ( src.ReadTokenOnLine( &token ) ) {
				if ( src.ReadTokenOnLine( &token ) ) {
				}
			}
		}
	} while( 1 );

	idMapBrush *brush = new idMapBrush();
	for ( i = 0; i < sides.Num(); i++ ) {
		brush->AddSide( sides[i] );
	}

	brush->epairs = epairs;

	return brush;
}

/*
============
idMapBrush::Write
============
*/
bool idMapBrush::Write( idFile *fp, int primitiveNum, const idVec3 &origin ) const {
	int i;
	idMapBrushSide *side;

	fp->WriteFloatString( "// primitive %d\n{\n brushDef3\n {\n", primitiveNum );

	// write brush epairs
	for ( i = 0; i < epairs.GetNumKeyVals(); i++) {
		fp->WriteFloatString( "  \"%s\" \"%s\"\n", epairs.GetKeyVal(i)->GetKey().c_str(), epairs.GetKeyVal(i)->GetValue().c_str());
	}

	// write brush sides
	for ( i = 0; i < GetNumSides(); i++ ) {
		side = GetSide( i );
		fp->WriteFloatString( "  ( %f %f %f %f ) ", side->plane[0], side->plane[1], side->plane[2], side->plane[3] );
		fp->WriteFloatString( "( ( %f %f %f ) ( %f %f %f ) ) \"%s\" 0 0 0\n",
							side->texMat[0][0], side->texMat[0][1], side->texMat[0][2],
								side->texMat[1][0], side->texMat[1][1], side->texMat[1][2],
									side->material.c_str() );
	}

	fp->WriteFloatString( " }\n}\n" );

	return true;
}

/*
===============
idMapBrush::GetGeometryCRC
===============
*/
unsigned int idMapBrush::GetGeometryCRC( void ) const {
	int i, j;
	idMapBrushSide *mapSide;
	unsigned int crc;

	crc = 0;
	for ( i = 0; i < GetNumSides(); i++ ) {
		mapSide = GetSide(i);
		for ( j = 0; j < 4; j++ ) {
			crc ^= FloatCRC( mapSide->GetPlane()[j] );
		}
		crc ^= StringCRC( mapSide->GetMaterial() );
	}

	return crc;
}

/*
================
idMapEntity::Parse
================
*/
idMapEntity *idMapEntity::Parse( idLexer &src, bool worldSpawn, float version ) {
	idToken	token;
	idMapEntity *mapEnt;
	idMapPatch *mapPatch;
	idMapBrush *mapBrush;
	bool worldent;
	idVec3 origin;
	double v1, v2, v3;

	if ( !src.ReadToken(&token) ) {
		return NULL;
	}

	if ( token != "{" ) {
		src.Error( "idMapEntity::Parse: { not found, found %s", token.c_str() );
		return NULL;
	}

	mapEnt = new idMapEntity();

	if ( worldSpawn ) {
		mapEnt->primitives.Resize( 1024, 256 );
	}

	origin.Zero();
	worldent = false;
	do {
		if ( !src.ReadToken(&token) ) {
			src.Error( "idMapEntity::Parse: EOF without closing brace" );
			return NULL;
		}
		if ( token == "}" ) {
			break;
		}

		if ( token == "{" ) {
			// parse a brush or patch
			if ( !src.ReadToken( &token ) ) {
				src.Error( "idMapEntity::Parse: unexpected EOF" );
				return NULL;
			}

			if ( worldent ) {
				origin.Zero();
			}

			// if is it a brush: brush, brushDef, brushDef2, brushDef3
			if ( token.Icmpn( "brush", 5 ) == 0 ) {
				mapBrush = idMapBrush::Parse( src, origin, ( !token.Icmp( "brushDef2" ) || !token.Icmp( "brushDef3" ) ), version );
				if ( !mapBrush ) {
					return NULL;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
			// if is it a patch: patchDef2, patchDef3
			else if ( token.Icmpn( "patch", 5 ) == 0 ) {
				mapPatch = idMapPatch::Parse( src, origin, !token.Icmp( "patchDef3" ), version );
				if ( !mapPatch ) {
					return NULL;
				}
				mapEnt->AddPrimitive( mapPatch );
			}
			// assume it's a brush in Q3 or older style
			else {
				src.UnreadToken( &token );
				mapBrush = idMapBrush::ParseQ3( src, origin );
				if ( !mapBrush ) {
					return NULL;
				}
				mapEnt->AddPrimitive( mapBrush );
			}
		} else {
			idStr key, value;

			// parse a key / value pair
			key = token;
			src.ReadTokenOnLine( &token );
			value = token;

			// strip trailing spaces that sometimes get accidentally
			// added in the editor
			value.StripTrailingWhitespace();
			key.StripTrailingWhitespace();

			mapEnt->epairs.Set( key, value );

			if ( !idStr::Icmp( key, "origin" ) ) {
				// scanf into doubles, then assign, so it is idVec size independent
				v1 = v2 = v3 = 0;
				sscanf( value, "%lf %lf %lf", &v1, &v2, &v3 );
				origin.x = v1;
				origin.y = v2;
				origin.z = v3;
			}
			else if ( !idStr::Icmp( key, "classname" ) && !idStr::Icmp( value, "worldspawn" ) ) {
				worldent = true;
			}
		}
	} while( 1 );

	return mapEnt;
}

/*
============
idMapEntity::Write
============
*/
bool idMapEntity::Write( idFile *fp, int entityNum ) const {
	int i;
	idMapPrimitive *mapPrim;
	idVec3 origin;

	fp->WriteFloatString( "// entity %d\n{\n", entityNum );

	// write entity epairs
	for ( i = 0; i < epairs.GetNumKeyVals(); i++) {
		fp->WriteFloatString( "\"%s\" \"%s\"\n", epairs.GetKeyVal(i)->GetKey().c_str(), epairs.GetKeyVal(i)->GetValue().c_str());
	}

	epairs.GetVector( "origin", "0 0 0", origin );

	// write pritimives
	for ( i = 0; i < GetNumPrimitives(); i++ ) {
		mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() ) {
			case idMapPrimitive::TYPE_BRUSH:
				static_cast<idMapBrush*>(mapPrim)->Write( fp, i, origin );
				break;
			case idMapPrimitive::TYPE_PATCH:
				static_cast<idMapPatch*>(mapPrim)->Write( fp, i, origin );
				break;
		}
	}

	fp->WriteFloatString( "}\n" );

	return true;
}

/*
===============
idMapEntity::RemovePrimitiveData
===============
*/
void idMapEntity::RemovePrimitiveData() {
	primitives.DeleteContents(true);
}

/*
===============
idMapEntity::GetGeometryCRC
===============
*/
unsigned int idMapEntity::GetGeometryCRC( void ) const {
	int i;
	unsigned int crc;
	idMapPrimitive	*mapPrim;

	crc = 0;
	for ( i = 0; i < GetNumPrimitives(); i++ ) {
		mapPrim = GetPrimitive( i );

		switch( mapPrim->GetType() ) {
			case idMapPrimitive::TYPE_BRUSH:
				crc ^= static_cast<idMapBrush*>(mapPrim)->GetGeometryCRC();
				break;
			case idMapPrimitive::TYPE_PATCH:
				crc ^= static_cast<idMapPatch*>(mapPrim)->GetGeometryCRC();
				break;
		}
	}

	return crc;
}

bool idMapEntity::NeedsReload(const idMapEntity *oldEntity) const {
	const idDict &a = epairs, &b = oldEntity->epairs;
	int m = a.GetNumKeyVals();
	int n = b.GetNumKeyVals();
	if (m != n)
		return true;
	for (int i = 0; i < n; i++) {
		if (*a.GetKeyVal(i) == *b.GetKeyVal(i))
			continue;
		return true;
	}
	if (GetGeometryCRC() != oldEntity->GetGeometryCRC())
		return true;
	return false;
}

idMapFile::idMapFile( void ) {
	version = CURRENT_MAP_VERSION;
	fileTime = 0;
	geometryCRC = 0;
	entities.Resize( 1024, 256 );
	hasPrimitiveData = false;
}

idMapFile::~idMapFile( void ) {
	entities.DeleteContents( true );
}

int idMapReloadInfo::NameAndIdx::Cmp(const NameAndIdx *a, const NameAndIdx *b) {
	return idStr::Cmp(a->name, b->name);
}

idMapReloadInfo idMapFile::TryReload() {
	idStr filename = fileName;
	bool ignoreRegion = true;
	if (filename.CheckExtension(".reg"))
		ignoreRegion = false;
	filename.StripFileExtension();

	idMapReloadInfo res;
	//move data from *this to res.oldMap
	res.oldMap.reset(new idMapFile(*this));
	this->entities.Clear();

	auto RestoreOldMap = [&]() {
		*this = *res.oldMap;
		this->entities = res.oldMap->entities;
		res.oldMap->entities.Clear();
		res.oldMap.reset();
		res = idMapReloadInfo();
	};

	//load new map
	common->SetErrorIndirection(true);
	try {
		Parse(fileName.c_str(), ignoreRegion);
	}
	catch(std::shared_ptr<ErrorReportedException> err) {
		common->Warning("Map not loaded: %s", err->ErrorMessage().c_str());
		RestoreOldMap();
		res.mapInvalid = true;
		return res;
	}
	common->SetErrorIndirection(false);

	typedef idMapReloadInfo::NameAndIdx NameAndIdx;
	//write out entities, sort and check them
	auto PrepareListOfEntityNameAndIds = [](const idList<idMapEntity*> &entities, idList<NameAndIdx> &result) -> bool {
		for (int i = 0; i < entities.Num(); i++) {
			idMapEntity *ent = entities[i];
			if (const idKeyValue *kv = ent->epairs.FindKey("name") )
				result.AddGrow({kv->GetValue().c_str(), i});
			else {
				if (idStr::Cmp(ent->epairs.GetString("classname", ""), "worldspawn") == 0)
					result.AddGrow({"worldspawn", i});
				else {
					common->Warning("idMapFile::Reload: no difference because of unnamed entity %d", i);
					return false;	//unnamed entity: can't detect anything
				}
			}
		}
		result.Sort(NameAndIdx::Cmp);
		for (int i = 1; i < result.Num(); i++)
			if (NameAndIdx::Cmp(&result[i-1], &result[i]) == 0) {
				common->Warning(
					"idMapFile::Reload: no difference because name %s is used twice: %d and %d",
					result[i].name, result[i-1].idx, result[i].idx
				);
				return false;
			}
		return true;
	};
	idList<NameAndIdx> oldEntities, newEntities;
	bool ok = PrepareListOfEntityNameAndIds(this->entities, newEntities);
	ok &= PrepareListOfEntityNameAndIds(res.oldMap->entities, oldEntities);
	if (!ok) {
		RestoreOldMap();
		return res;
	}

	//go over entities (merge-like) and compute difference
	int i = 0, j = 0;
	while (i < newEntities.Num() || j < oldEntities.Num()) {
		int cmp;
		if (i == newEntities.Num())
			cmp = 1;
		else if (j == oldEntities.Num())
			cmp = -1;
		else
			cmp = NameAndIdx::Cmp(&newEntities[i], &oldEntities[j]);

		if (cmp == 0) {	//two entities with equal name
			const idMapEntity *newEnt = this->GetEntity(newEntities[i].idx);
			const idMapEntity *oldEnt = res.oldMap->GetEntity(oldEntities[j].idx);
			if (newEnt->NeedsReload(oldEnt))
				res.modifiedEntities.AddGrow(newEntities[i]);
			i++;  j++;
		}
		else {
			if (cmp < 0)
				res.addedEntities.AddGrow(newEntities[i++]);
			else
				res.removedEntities.AddGrow(oldEntities[j++]);
		}
	}

	//now we are certain we computed diff properly
	res.cannotReload = false;
	return res;
}

idMapReloadInfo idMapFile::ApplyDiff(const char *text) {
	idMapReloadInfo res;

	idList<idMapEntity*> diffAdded, diffRemoved, diffModified;
	idList<bool> diffModRespawn;

	auto ParseDiff = [&]() -> bool {
		idLexer src(LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES | LEXFL_NOFATALERRORS);
		src.LoadMemory(text, strlen(text), "map-diff");
		idToken token;

		idDict seenNames;
		while (src.ReadToken(&token)) {
			int status;
			bool respawn = false;
			if (token == "add")
				status = 1;
			else if (token == "remove")
				status = -1;
			else if (token == "modify")
				status = 0;
			else if (token == "modify_respawn") {
				status = 0;
				respawn = true;
			}
			else {
				src.Error("idMapFile::ApplyDiff: unknown modification type");
				return false;
			}

			src.ReadToken(&token);
			if (token == "entity") {
				idMapEntity *ent = idMapEntity::Parse(src, false);
				if (!ent) {
					src.Error("idMapFile::ApplyDiff: could not parse entity");
					return false;
				}
				const char *name = ent->epairs.GetString("name");
				if (seenNames.FindKey(name)) {
					src.Error("idMapFile::ApplyDiff: entity %s described twice in diff", name);
					return false;
				}
				seenNames.Set(name, "");
				idMapEntity *oldEnt = FindEntity(name);
				if (status <= 0 && !oldEnt) {
					src.Warning("idMapFile::ApplyDiff: changed entity %s does not exist", name);
					return false;
				}
				if (status > 0 && oldEnt) {
					src.Warning("idMapFile::ApplyDiff: added entity %s already exists", name);
					return false;
				}
				if (status < 0)
					diffRemoved.AddGrow(ent);
				else if (status == 0) {
					diffModified.AddGrow(ent);
					diffModRespawn.AddGrow(respawn);
				}
				else if (status > 0)
					diffAdded.AddGrow(ent);
			}
			else {
				src.Error("idMapFile::ApplyDiff: unknown object type");
				return false;
			}
		}

		return true;
	};
	if (!ParseDiff()) {
		diffAdded.DeleteContents();
		diffRemoved.DeleteContents();
		diffModified.DeleteContents();
		res.mapInvalid = true;
		return res;
	}

	//create empty "old" map, to be populated with old versions of entities
	res.oldMap.reset(new idMapFile());
	typedef idMapReloadInfo::NameAndIdx NameAndIdx;

	for (int i = 0; i < diffRemoved.Num(); i++) {
		idMapEntity *diffEnt = diffRemoved[i];
		const char *name = diffEnt->epairs.GetString("name");
		idMapEntity *oldEnt = FindEntity(name);
		entities.Remove(oldEnt);
		int pos = res.oldMap->AddEntity(oldEnt);
		res.removedEntities.AddGrow({oldEnt->epairs.GetString("name"), pos});
	}
	for (int i = 0; i < diffModified.Num(); i++) {
		idMapEntity *diffEnt = diffModified[i];
		const char *name = diffEnt->epairs.GetString("name");
		idMapEntity *oldEnt = FindEntity(name);
		if (!diffModRespawn[i] && !diffEnt->NeedsReload(oldEnt))
			continue;	//no actual change here
		int pos = entities.FindIndex(oldEnt);
		res.oldMap->AddEntity(oldEnt);
		entities[pos] = diffEnt;
		if (diffModRespawn[i])
			res.respawnEntities.AddGrow({name, pos});
		else
			res.modifiedEntities.AddGrow({name, pos});
	}
	for (int i = 0; i < diffAdded.Num(); i++) {
		idMapEntity *diffEnt = diffAdded[i];
		const char *name = diffEnt->epairs.GetString("name");
		int pos = AddEntity(diffEnt);
		res.addedEntities.AddGrow({name, pos});
	}

	res.cannotReload = false;
	return res;
}

/*
===============
idMapFile::Parse
===============
*/
bool idMapFile::Parse( const char *filename, bool ignoreRegion, bool osPath ) {
	TRACE_CPU_SCOPE_TEXT("idMapFile::Parse", filename)

	// no string concatenation for epairs and allow path names for materials
	idLexer src( LEXFL_NOSTRINGCONCAT | LEXFL_NOSTRINGESCAPECHARS | LEXFL_ALLOWPATHNAMES );
	idToken token;
	idStr fullName;
	idMapEntity *mapEnt;
	int i, j, k;

	name = filename;
	name.StripFileExtension();
	fullName = name;
	hasPrimitiveData = false;

	if ( !src.IsLoaded() && !ignoreRegion ) {
		// try loading a .reg file first
		fullName.SetFileExtension( "reg" );
		src.LoadFile( fullName, osPath );
		if (src.IsLoaded())
			fileName = fullName;
	}
	if ( !src.IsLoaded() ) {
		// now try a .map file
		fullName.SetFileExtension( "map" );
		src.LoadFile( fullName, osPath );
		if (src.IsLoaded())
			fileName = fullName;
	}
	if ( !src.IsLoaded() )
		return false;

	version = OLD_MAP_VERSION;
	fileTime = src.GetFileTime();
	entities.DeleteContents( true );

	if ( src.CheckTokenString( "Version" ) ) {
		src.ReadTokenOnLine( &token );
		version = token.GetFloatValue();
	}

	while( 1 ) {
		mapEnt = idMapEntity::Parse( src, ( entities.Num() == 0 ), version );
		if ( !mapEnt ) {
			break;
		}
		entities.Append( mapEnt );
	}

	SetGeometryCRC();

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
		if ( entities[0]->epairs.GetBool( "moveFuncGroups" ) ) {
			for ( i = 1; i < entities.Num(); i++ ) {
				mapEnt = entities[i];
				if ( idStr::Icmp( mapEnt->epairs.GetString( "classname" ), "func_group" ) == 0 ) {
					// SteveL #4300. This was broken. Primitives in entities are written in the map file 
					// relative to the entity origin not the world origin. This needs correcting for as 
					// the entities are copied. 
					// entities[0]->primitives.Append( mapEnt->primitives ); // Old code pre-#4300
					for ( int x = 0; x < mapEnt->primitives.Num(); ++x )
					{
						idMapPrimitive* pr = mapEnt->primitives[x];
						idVec3 groupOrg = mapEnt->epairs.GetVector("origin");
						if ( pr->GetType() == idMapPrimitive::TYPE_BRUSH )
						{
							idMapBrush* br = static_cast<idMapBrush*>(pr);
							for ( int y = 0; y < br->GetNumSides(); ++y )
							{
								idMapBrushSide* bs = br->GetSide(y);
								idPlane p = bs->GetPlane();
								p.SetDist( p.Dist() + groupOrg * p.Normal() );
								bs->SetPlane(p);
							}
						}
						else if ( pr->GetType() == idMapPrimitive::TYPE_PATCH )
						{
							static_cast<idMapPatch*>(pr)->TranslateSelf( groupOrg );
						}
						entities[0]->primitives.Append(pr);
					}
					// End of #4300
					mapEnt->primitives.ClearFree();
				}
			}
		}
	}

	hasPrimitiveData = true;
	return true;
}

/*
============
idMapFile::Write
============
*/
bool idMapFile::Write( const char *fileName, const char *ext, bool fromBasePath ) {
	int i;
	idStr qpath;
	idFile *fp;

	qpath = fileName;
	qpath.SetFileExtension( ext );

	idLib::common->Printf( "writing %s...\n", qpath.c_str() );

	if ( fromBasePath ) {
		fp = idLib::fileSystem->OpenFileWrite( qpath, "fs_devpath", "" );
	}
	else {
		fp = idLib::fileSystem->OpenExplicitFileWrite( qpath );
	}

	if ( !fp ) {
		idLib::common->Warning( "Couldn't open %s", qpath.c_str() );
		return false;
	}

	fp->WriteFloatString( "Version %f\n", (float) CURRENT_MAP_VERSION );

	for ( i = 0; i < entities.Num(); i++ ) {
		entities[i]->Write( fp, i );
	}

	idLib::fileSystem->CloseFile( fp );

	return true;
}

/*
===============
idMapFile::SetGeometryCRC
===============
*/
void idMapFile::SetGeometryCRC( void ) {
	int i;

	geometryCRC = 0;
	for ( i = 0; i < entities.Num(); i++ ) {
		geometryCRC ^= entities[i]->GetGeometryCRC();
	}
}

/*
===============
idMapFile::AddEntity
===============
*/
int idMapFile::AddEntity( idMapEntity *mapEnt ) {
	int ret = entities.Append( mapEnt );
	return ret;
}

/*
===============
idMapFile::FindEntity
===============
*/
idMapEntity *idMapFile::FindEntity( const char *name ) {
	for ( int i = 0; i < entities.Num(); i++ ) {
		idMapEntity *ent = entities[i];
		if ( idStr::Icmp( ent->epairs.GetString( "name" ), name ) == 0 ) {
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
void idMapFile::RemoveEntity( idMapEntity *mapEnt ) {
	entities.Remove( mapEnt );
	delete mapEnt;
}

/*
===============
idMapFile::RemoveEntity
===============
*/
void idMapFile::RemoveEntities( const char *classname ) {
	for ( int i = 0; i < entities.Num(); i++ ) {
		idMapEntity *ent = entities[i];
		if ( idStr::Icmp( ent->epairs.GetString( "classname" ), classname ) == 0 ) {
			delete entities[i];
			entities.RemoveIndex( i );
			i--;
		}
	}
}

/*
===============
idMapFile::RemoveAllEntities
===============
*/
void idMapFile::RemoveAllEntities() {
	entities.DeleteContents( true );
	hasPrimitiveData = false;
}

/*
===============
idMapFile::RemovePrimitiveData
===============
*/
void idMapFile::RemovePrimitiveData() {
	for ( int i = 0; i < entities.Num(); i++ ) {
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
bool idMapFile::NeedsReload() {
	if ( name.Length() ) {
		ID_TIME_T time = (ID_TIME_T)-1;
		if ( idLib::fileSystem->ReadFile( fileName, NULL, &time ) > 0 ) {
			return ( time > fileTime );
		}
	}
	return true;
}


/*
===============
idMapFile::GetTotalPrimitivesNum
===============
*/
int idMapFile::GetTotalPrimitivesNum() const {
	int res = 0;
	for (int i = 0; i < entities.Num(); i++)
		res += entities[i]->GetNumPrimitives();
	return res;
}
