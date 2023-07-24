// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLKEYBINDING_H__
#define __DECLKEYBINDING_H__

class sdDeclKeyBinding : public idDecl {
public:
								sdDeclKeyBinding( void );
	virtual						~sdDeclKeyBinding( void );

	virtual const char*			DefaultDefinition( void ) const;
	virtual bool				Parse( const char *text, const int textLength );
	virtual void				FreeData( void );

	const idDict&				GetKeys() const { return keys; }
	const sdDeclLocStr*			GetTitle() const { return title; }

private:
	bool						ParseKeys( idParser& src );

private:
	idDict						keys;
	const sdDeclLocStr*			title;
};

#endif // __DECLKEYBINDING_H__
