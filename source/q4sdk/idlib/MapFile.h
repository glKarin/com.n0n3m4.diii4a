
#ifndef __MAPFILE_H__
#define __MAPFILE_H__

/*
===============================================================================

	Reads or writes the contents of .map files into a standard internal
	format, which can then be moved into private formats for collision
	detection, map processing, or editor use.

	No validation (duplicate planes, null area brushes, etc) is performed.
	There are no limits to the number of any of the elements in maps.
	The order of entities, brushes, and sides is maintained.

===============================================================================
*/

const int OLD_MAP_VERSION					= 1;
const int CURRENT_MAP_VERSION				= 3;
const int DEFAULT_CURVE_SUBDIVISION			= 4;
const float DEFAULT_CURVE_MAX_ERROR			= 4.0f;
const float DEFAULT_CURVE_MAX_ERROR_CD		= 24.0f;
const float DEFAULT_CURVE_MAX_LENGTH		= -1.0f;
const float DEFAULT_CURVE_MAX_LENGTH_CD		= -1.0f;


class idMapPrimitive {
public:
	enum { TYPE_INVALID = -1, TYPE_BRUSH, TYPE_PATCH };

	idDict					epairs;

							idMapPrimitive( void ) { type = TYPE_INVALID; }
	virtual					~idMapPrimitive( void ) { }
	int						GetType( void ) const { return type; }

// RAVEN BEGIN
// rjohnson: added resolve for handling func_groups and other aspects.  Before, radiant would do this processing on a map destroying the original data
	virtual void			AdjustOrigin( idVec3 &delta ) { }
// RAVEN END

protected:
	int						type;
};


class idMapBrushSide {
	friend class idMapBrush;

public:
							idMapBrushSide( void );
							~idMapBrushSide( void ) { }
	const char *			GetMaterial( void ) const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	const idPlane &			GetPlane( void ) const { return plane; }
	void					SetPlane( const idPlane &p ) { plane = p; }
	void					SetTextureMatrix( const idVec3 mat[2] ) { texMat[0] = mat[0]; texMat[1] = mat[1]; }
	void					GetTextureMatrix( idVec3 &mat1, idVec3 &mat2 ) { mat1 = texMat[0]; mat2 = texMat[1]; }
	void					GetTextureVectors( idVec4 v[2] ) const;

protected:
	idStr					material;
	idPlane					plane;
	idVec3					texMat[2];
	idVec3					origin;
};

ID_INLINE idMapBrushSide::idMapBrushSide( void ) {
	plane.Zero();
	texMat[0].Zero();
	texMat[1].Zero();
	origin.Zero();
}


class idMapBrush : public idMapPrimitive {
public:
							idMapBrush( void ) { type = TYPE_BRUSH; sides.Resize( 8, 4 ); }
							~idMapBrush( void ) { sides.DeleteContents( true ); }
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
	static idMapBrush *		Parse( Lexer &src, const idVec3 &origin, bool newFormat = true, int version = CURRENT_MAP_VERSION );
	static idMapBrush *		ParseQ3( Lexer &src, const idVec3 &origin );
// RAVEN END
	bool					Write( idFile *fp, int primitiveNum, const idVec3 &origin ) const;
	int						GetNumSides( void ) const { return sides.Num(); }
	int						AddSide( idMapBrushSide *side ) { return sides.Append( side ); }
	idMapBrushSide *		GetSide( int i ) const { return sides[i]; }
	unsigned int			GetGeometryCRC( void ) const;

// RAVEN BEGIN
// rjohnson: added resolve for handling func_groups and other aspects.  Before, radiant would do this processing on a map destroying the original data
	virtual void			AdjustOrigin( idVec3 &delta );
// RAVEN END

protected:
	idList<idMapBrushSide*> sides;
};


class idMapPatch : public idMapPrimitive, public idSurface_Patch {
public:
							idMapPatch( void );
							idMapPatch( int maxPatchWidth, int maxPatchHeight );
							~idMapPatch( void ) { }
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
	static idMapPatch *		Parse( Lexer &src, const idVec3 &origin, bool patchDef3 = true, int version = CURRENT_MAP_VERSION );
// RAVEN END
	bool					Write( idFile *fp, int primitiveNum, const idVec3 &origin ) const;
	const char *			GetMaterial( void ) const { return material; }
	void					SetMaterial( const char *p ) { material = p; }
	int						GetHorzSubdivisions( void ) const { return horzSubdivisions; }
	int						GetVertSubdivisions( void ) const { return vertSubdivisions; }
	bool					GetExplicitlySubdivided( void ) const { return explicitSubdivisions; }
	void					SetHorzSubdivisions( int n ) { horzSubdivisions = n; }
	void					SetVertSubdivisions( int n ) { vertSubdivisions = n; }
	void					SetExplicitlySubdivided( bool b ) { explicitSubdivisions = b; }
	unsigned int			GetGeometryCRC( void ) const;

// RAVEN BEGIN
// rjohnson: added resolve for handling func_groups and other aspects.  Before, radiant would do this processing on a map destroying the original data
	virtual void			AdjustOrigin( idVec3 &delta );
// RAVEN END

protected:
	idStr					material;
	int						horzSubdivisions;
	int						vertSubdivisions;
	bool					explicitSubdivisions;
};

ID_INLINE idMapPatch::idMapPatch( void ) {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxHeight = 0;
	expanded = false;
}

ID_INLINE idMapPatch::idMapPatch( int maxPatchWidth, int maxPatchHeight ) {
	type = TYPE_PATCH;
	horzSubdivisions = vertSubdivisions = 0;
	explicitSubdivisions = false;
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}


class idMapEntity {
	friend class			idMapFile;

public:
	idDict					epairs;

public:
							idMapEntity( void ) { epairs.SetHashSize( 64 ); }
							~idMapEntity( void ) { primitives.DeleteContents( true ); }
// RAVEN BEGIN
// jsinger: changed to be Lexer instead of idLexer so that we have the ability to read binary files
	static idMapEntity *	Parse( Lexer &src, bool worldSpawn = false, int version = CURRENT_MAP_VERSION );
// RAVEN END
	bool					Write( idFile *fp, int entityNum ) const;
	int						GetNumPrimitives( void ) const { return primitives.Num(); }
	idMapPrimitive *		GetPrimitive( int i ) const { return primitives[i]; }
	void					AddPrimitive( idMapPrimitive *p ) { primitives.Append( p ); }
	unsigned int			GetGeometryCRC( void ) const;
	void					RemovePrimitiveData();

protected:
	idList<idMapPrimitive*>	primitives;
};


class idMapFile {
public:
							idMapFile( void );
// RAVEN BEGIN
// rhummer: moved body to mapfile.cpp to help debug some odd issues with func_groups
							~idMapFile( void ); /*{ entities.DeleteContents( true ); }*/
// RAVEN END

							// filename does not require an extension
							// normally this will use a .reg file instead of a .map file if it exists,
							// which is what the game and dmap want, but the editor will want to always
							// load a .map file
	bool					Parse( const char *filename, bool ignoreRegion = false, bool osPath = false );

// RAVEN BEGIN
// rjohnson: added resolve
	void					Resolve( void );
// rhummer: added boolean to dictate if the Resolve function has been run on this map.
	bool					HasBeenResloved() { return mHasBeenResolved; }
// rjohnson: added export
	bool					Write( const char *fileName, const char *ext, bool fromBasePath = true, bool exportOnly = false );
// RAVEN END

							// get the number of entities in the map
	int						GetNumEntities( void ) const { return entities.Num(); }
							// get the specified entity
	idMapEntity *			GetEntity( int i ) const { return entities[i]; }
							// get the name without file extension
	const char *			GetName( void ) const { return name; }
							// get the file time
	unsigned int			GetFileTime( void ) const { return fileTime; }
							// get CRC for the map geometry
							// texture coordinates and entity key/value pairs are not taken into account
	unsigned int			GetGeometryCRC( void ) const { return geometryCRC; }
							// returns true if the file on disk changed
	bool					NeedsReload();

	int						AddEntity( idMapEntity *mapentity );
	idMapEntity *			FindEntity( const char *name );
	void					RemoveEntity( idMapEntity *mapEnt );
	void					RemoveEntities( const char *classname );
	void					RemoveAllEntities();
	void					RemovePrimitiveData();
	bool					HasPrimitiveData() { return hasPrimitiveData; }

// RAVEN BEGIN
// rjohnson: added export
	bool					WriteExport( const char *fileName, bool fromBasePath = true );
	bool					ParseExport( const char *filename, bool osPath = false );

	bool					HasExportEntities(void) { return mHasExportEntities; }
// RAVEN END

protected:
	int						version;
	unsigned int			fileTime;
	unsigned int			geometryCRC;
	idList<idMapEntity *>	entities;
	idStr					name;
	bool					hasPrimitiveData;

// RAVEN BEGIN
// rjohnson: added export
	idList<idMapEntity *>	mExportEntities;
	bool					mHasExportEntities;
// rhummer: Added to inform if func_groups some how disappeared between the loading and saving of the map file.
	bool					mHasFuncGroups;
// rhummer: Added to notify that this map has been resloved, so func_groups have been removed.
	bool					mHasBeenResolved;
// RAVEN END

private:
	void					SetGeometryCRC( void );
};

ID_INLINE idMapFile::idMapFile( void ) {
	version = CURRENT_MAP_VERSION;
	fileTime = 0;
	geometryCRC = 0;
	entities.Resize( 1024, 256 );
	hasPrimitiveData = false;
// RAVEN BEGIN
// rhummer: Used to make sure func_groups don't disappear.
	mHasFuncGroups = false;
// RAVEN END
}

#endif /* !__MAPFILE_H__ */
