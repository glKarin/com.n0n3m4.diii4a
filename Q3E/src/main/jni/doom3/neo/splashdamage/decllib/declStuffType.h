// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLSTUFFTYPE_H__
#define __DECLSTUFFTYPE_H__

#include "../framework/declManager.h"

class sdDeclStuffType : public idDecl {
public:
							sdDeclStuffType( void );
	virtual					~sdDeclStuffType( void ) {}

	// Override from idDecl
	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual size_t			Size( void ) const { return sizeof(sdDeclStuffType); }

	// New for this decl
	int						GetNumModels( void ) const { return models.Num(); }
	const char *			GetModelName( int index ) const { return models[index].c_str(); }
	bool					GetRandomizeAngles( void ) const  { return randomizeAngles; } 
	const sdDeclStuffType * GetLodType( void ) const { return lodType; }

private:
	idList<idStr> models;
	bool					RebuildTextSource( void );
	bool randomizeAngles;
	const sdDeclStuffType * lodType;
};

#endif // __DECLSTUFFTYPE_H__
