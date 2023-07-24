// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __UTF8_H__
#define __UTF8_H__

class sdUTF8 {
public:
			sdUTF8( idFile* file );	
			sdUTF8( const byte* data, const int size );	
			~sdUTF8( void );

	int		DecodeLength( void ) const;
	int		Decode( wchar_t* to ) const;

	static void	Encode( idFile* file, const wchar_t* data, int len );

private:
	void	Init( void );
	void	Release( void );
	void	EnsureAlloced( int size );

	int		UTF8toUCS2( const byte* data, const int len, wchar_t* ucs2 ) const;

private:
	byte*	data;
	int		len;
	int		alloced;
};

ID_INLINE sdUTF8::~sdUTF8( void ) {
	Release();
}

ID_INLINE void sdUTF8::Init( void ) {
	len = 0;
	alloced = 0;
	data = NULL;
}

ID_INLINE void sdUTF8::Release( void ) {
	delete [] data;
	Init();
}

ID_INLINE void sdUTF8::EnsureAlloced( int size ) {
	if ( size > alloced ) {
		Release();
	}
	data = new byte[size];
	alloced = size;
}

#endif /* !__UTF8_H__ */
