// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLSKIN_H__
#define __DECLSKIN_H__

#include "../framework/declManager.h"

/*
===============================================================================

	idDeclSkin

===============================================================================
*/

typedef struct {
	const idMaterial *		from;			// 0 == any unmatched shader
	const idMaterial *		to;
} skinMapping_t;

class idDeclSkin : public idDecl {
public:
	virtual					~idDeclSkin( void ) {}

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	static void				CacheFromDict( const idDict& dict );

	const idMaterial *		RemapShaderBySkin( const idMaterial *shader ) const;

private:
	idList<skinMapping_t>	mappings;
};

#endif /* !__DECLSKIN_H__ */
