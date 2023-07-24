// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLRANK_H__
#define __DECLRANK_H__

class sdDeclLocStr;

class sdDeclRank : public idDecl {
public:
							sdDeclRank( void );
	virtual					~sdDeclRank( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	const sdDeclLocStr*		GetTitle( void ) const { return title; }
	const sdDeclLocStr*		GetShortTitle( void ) const { return shortTitle; }
	const char*				GetMaterial( void ) const { return material; }
	float					GetCost( void ) const { return cost; }
	int						GetLevel( void ) const { return level; }

private:
	const sdDeclLocStr*		title;
	const sdDeclLocStr*		shortTitle;
	idStr					material;
	float					cost;
	int						level;
};

#endif // __DECLRANK_H__

