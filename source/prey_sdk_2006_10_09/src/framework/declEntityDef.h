// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __DECLENTITYDEF_H__
#define __DECLENTITYDEF_H__

/*
===============================================================================

	idDeclEntityDef

===============================================================================
*/

class idDeclEntityDef : public idDecl {
public:
	idDict					dict;

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void );
};

#endif /* !__DECLENTITYDEF_H__ */
