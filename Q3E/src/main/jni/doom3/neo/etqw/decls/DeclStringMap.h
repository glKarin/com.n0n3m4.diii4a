// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLSTRINGMAP_H__
#define __DECLSTRINGMAP_H__

class sdDeclStringMap : public idDecl {
public:
							sdDeclStringMap( void );
	virtual					~sdDeclStringMap( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
			void			RebuildSourceText();
			void			Save();

	const char*				GetValue( const char* key ) const;
	const idDict&			GetDict( void ) const { return dict; }
	idDict&					GetDict( void ) { return dict; }

	static void				CacheFromDict( const idDict& dict );

protected:
	idDict					dict;
};

#endif // __DECLSTRINGMAP_H__

