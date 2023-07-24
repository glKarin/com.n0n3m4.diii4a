// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLMAPINFO_H__
#define __DECLMAPINFO_H__

#include "../Common.h"

class sdDeclMapInfo : public idDecl {
public:
								sdDeclMapInfo( void );
	virtual						~sdDeclMapInfo( void );

	virtual const char*			DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );
	virtual void				FreeData( void );

	const char*					GetHeightmapFile( void ) const { return heightMap.c_str(); }
	const idDict&				GetData( void ) const { return data; }
	const idStrList&			GetMegatextureMaterials( void ) const { return megatextureMaterials; }
	const idVec2&				GetLocation( void ) const { return location; }
	const idMaterial*			GetServerShot( void ) const;

protected:
	idStrList					megatextureMaterials;
	idStr						heightMap;
	idDict						data;
	idVec2						location;
	idStr						serverShot;
};

#endif // __DECLMAPINFO_H__
