// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLAMMOTYPE_H__
#define __DECLAMMOTYPE_H__

#include "../Common.h"

class sdDeclAmmoType : public idDecl {
public:
							sdDeclAmmoType( void );
	virtual					~sdDeclAmmoType( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	ammoType_t				GetAmmoType( void ) const;
protected:
};

#endif // __DECLAMMOTYPE_H__
