// Copyright (C) 2007 Id Software, Inc.
//


#include "../../decllib/declLocStr.h"

#ifndef __DECLRADIALMENU_H__
#define __DECLRADIALMENU_H__

class sdDeclRadialMenu : public idDecl {
public:
								sdDeclRadialMenu( void );
	virtual						~sdDeclRadialMenu( void );

	virtual const char*			DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );
	virtual void				FreeData( void );

	const idDict&				GetKeys() const { return keys; }
	const sdDeclLocStr*			GetTitle() const { return title; }

	int							GetNumPages() const { return pages.Num(); }
	const sdDeclRadialMenu&		GetPage( int i ) const { return *pages[ i ]; }

	int							GetNumItems() const			{ return items.Num(); }
	const idDict&				GetItemKeys( int i ) const	{ return items[ i ].keys; }
	const sdDeclLocStr*			GetItemTitle( int i ) const { return items[ i ].title; }

private:
	bool						ParseKeys( idParser& src, idDict& dict );
	bool						ParseItem( idParser& src );
	bool						ParsePage( idParser& src );

private:
	struct item_t {
		const sdDeclLocStr* title;
		idDict	keys;
	};

	idDict												keys;
	const sdDeclLocStr*									title;
	idList< item_t >									items;
	idList< const sdDeclRadialMenu* >					pages;
};

#endif // __DECLRADIALMENU_H__
