// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DECLSURFACETYPE_H__
#define __DECLSURFACETYPE_H__

#ifdef _SPLASHDAMAGE
#include "../framework/DeclManager.h"
#else
#include "../framework/declManager.h"
#endif

/*
===============================================================================

sdDeclSurfaceType

===============================================================================
*/

class sdDeclSurfaceType : public idDecl {
public:
	virtual					~sdDeclSurfaceType( void ) {}

	// Override from idDecl
	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual size_t			Size( void ) const { return sizeof( sdDeclSurfaceType ); }
	virtual void			FreeData();

	const char*				GetType( void ) const { return type; }
	const idDict&			GetProperties( void ) const { return properties; }

private:
	idStr					type;
	idDict					properties;
};

#endif /* !__DECLSURFACETYPE_H__ */
