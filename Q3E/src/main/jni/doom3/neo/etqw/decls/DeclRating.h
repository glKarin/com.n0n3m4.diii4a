// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLRATING_H__
#define __DECLRATING_H__

class sdDeclRating : public idDecl {
public:
							sdDeclRating( void );
	virtual					~sdDeclRating( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	const sdDeclLocStr*		GetTitle( void ) const { return title; }
	const sdDeclLocStr*		GetShortTitle( void ) const { return shortTitle; }


private:
	const sdDeclLocStr*		title;
	const sdDeclLocStr*		shortTitle;
};

#endif // __DECLRATING_H__
