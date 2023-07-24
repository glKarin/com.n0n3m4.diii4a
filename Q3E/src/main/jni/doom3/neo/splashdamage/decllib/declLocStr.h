// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECL_LOCSTR_H__
#define __DECL_LOCSTR_H__

#include "../framework/declManager.h"

/*
===============================================================================

	sdDeclLocStr

===============================================================================
*/

class sdDeclLocStr : public idDecl {
public:
	virtual					~sdDeclLocStr() {}

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void ) const;

	const wchar_t*			GetText( void ) const { return locText.c_str(); }
	virtual bool			Format( idWStr& result, const idWStrList& inputs ) const;

private:
	idWStr					locText;
	int						numArgs;
};

#endif /* !__DECL_LOCSTR_H__ */
