// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLSURFACETYPEMAP_H__
#define __DECLSURFACETYPEMAP_H__

#include "../framework/DeclManager.h"

class sdDeclSurfaceType;

class sdDeclSurfaceTypeMap : public idDecl {
public:
	struct rect_t {
		const sdDeclSurfaceType *surfaceType;
		idVec3 surfaceColor;
		idList<idVec2> coords;
	};
public:
							sdDeclSurfaceTypeMap( void );
	virtual					~sdDeclSurfaceTypeMap( void ) {}

	// Override from idDecl
	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual size_t			Size( void ) const { return sizeof(sdDeclSurfaceTypeMap); }
	static  void			CacheFromDict( const idDict& dict );
	
	// New for this decl
	int						GetWidth( void ) const { return width; }
	int						GetHeight( void ) const { return height; }
	const idList< rect_t > &GetRects( void ) const { return rects; }

private:
	bool					ParseRect(idParser *src);
	bool					ParseWinding(idParser *src);

private:
	int						width;
	int						height;
	idList< rect_t >		rects;
};

#endif // __DECLSURFACETYPEMAP_H__
