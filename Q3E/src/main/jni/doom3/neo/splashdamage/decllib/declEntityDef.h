// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLENTITYDEF_H__
#define __DECLENTITYDEF_H__

#include "../framework/declManager.h"

/*
===============================================================================

	idDeclEntityDef

===============================================================================
*/

class idDeclEntityDef : public idDecl {
public:
	virtual					~idDeclEntityDef() {}
	idDict					dict;

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void ) const;

	static void				CacheFromDict( const idDict& dict );
};

#endif /* !__DECLENTITYDEF_H__ */
