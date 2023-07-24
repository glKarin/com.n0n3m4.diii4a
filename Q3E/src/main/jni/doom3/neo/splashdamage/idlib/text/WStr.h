// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __WSTR_H__
#define __WSTR_H__

/*
===============================================================================

	Wide Character string

===============================================================================
*/

const int W_COLOR_BITS				= 31;

// color escape character
const int WC_COLOR_ESCAPE			= L'^';
const int WC_COLOR_DEFAULT			= L'0';
const int WC_COLOR_RED				= L'1';
const int WC_COLOR_GREEN			= L'2';
const int WC_COLOR_YELLOW			= L'3';
const int WC_COLOR_BLUE				= L'4';
const int WC_COLOR_CYAN				= L'5';
const int WC_COLOR_MAGENTA			= L'6';
const int WC_COLOR_WHITE			= L'7';
const int WC_COLOR_GRAY				= L'8';
const int WC_COLOR_BLACK			= L'9';
const int WC_COLOR_LTGREY			= L':';
const int WC_COLOR_MDGREEN			= L'<';
const int WC_COLOR_MDYELLOW			= L'=';
const int WC_COLOR_MDBLUE			= L'>';
const int WC_COLOR_MDRED			= L'?';
const int WC_COLOR_LTORANGE			= L'A';
const int WC_COLOR_MDCYAN			= L'B';
const int WC_COLOR_MDPURPLE			= L'C';
const int WC_COLOR_ORANGE			= L'D';

// color escape string
#define WS_COLOR_DEFAULT			L"^0"
#define WS_COLOR_RED				L"^1"
#define WS_COLOR_GREEN				L"^2"
#define WS_COLOR_YELLOW				L"^3"
#define WS_COLOR_BLUE				L"^4"
#define WS_COLOR_CYAN				L"^5"
#define WS_COLOR_MAGENTA			L"^6"
#define WS_COLOR_WHITE				L"^7"
#define WS_COLOR_GRAY				L"^8"
#define WS_COLOR_BLACK				L"^9"
#define WS_COLOR_LTGREY				L"^:"
#define WS_COLOR_MDGREEN			L"^<"
#define WS_COLOR_MDYELLOW			L"^="
#define WS_COLOR_MDBLUE				L"^>"
#define WS_COLOR_MDRED				L"^?"
#define WS_COLOR_LTORANGE			L"^A"
#define WS_COLOR_MDCYAN				L"^B"
#define WS_COLOR_MDPURPLE			L"^C"
#define WS_COLOR_ORANGE				L"^D"

#ifdef ID_THREAD_SAFE_STR
typedef idDynamicBlockAlloc< wchar_t, 1<<18, 128, true >	wideStringDataAllocator_t;
#else
typedef idDynamicBlockAlloc< wchar_t, 1<<18, 128, false >	wideStringDataAllocator_t;
#endif

// the size of a wchar over the network
// stick to _WIN32's wchar size (2 bytes)
const int NET_SIZEOF_WCHAR = 2;

class idWStr {
public:
	struct hmsFormat_t {
		hmsFormat_t() : showZeroMinutes( false ), showZeroHours( false ), showZeroSeconds( true ) {}
		bool showZeroMinutes;
		bool showZeroHours;
		bool showZeroSeconds;
	};	

						idWStr( void );
						idWStr( const idWStr& text );
						idWStr( const idWStr& text, int start, int end );
						idWStr( const wchar_t* text );
						idWStr( const wchar_t* text, int start, int end );
						~idWStr( void );

	size_t				Size( void ) const;
	const wchar_t*		c_str( void ) const;

	wchar_t				operator[]( int index ) const;
	wchar_t &			operator[]( int index );

	void				operator=( const idWStr& text );
	void				operator=( const wchar_t* text );

	friend idWStr		operator+( const idStr &a, const idWStr &b );
	friend idWStr		operator+( const idStr &a, const wchar_t *b );
	friend idWStr		operator+( const wchar_t *a, const idWStr &b );

	idWStr &			operator+=( const idWStr &a );
	idWStr &			operator+=( const wchar_t *a );
	idWStr &			operator+=( const wchar_t a );

						// case sensitive compare
	friend bool			operator==( const idWStr &a, const idWStr &b );
	friend bool			operator==( const idWStr &a, const wchar_t *b );
	friend bool			operator==( const wchar_t *a, const idWStr &b );

						// case sensitive compare
	friend bool			operator!=( const idWStr &a, const idWStr &b );
	friend bool			operator!=( const idWStr &a, const wchar_t *b );
	friend bool			operator!=( const wchar_t *a, const idWStr &b );

						// case sensitive compare
	int					Cmp( const wchar_t *text ) const;
	int					Cmpn( const wchar_t *text, int n ) const;
	int					CmpPrefix( const wchar_t *text ) const;

						// case insensitive compare
	int					Icmp( const wchar_t *text ) const;
	int					Icmpn( const wchar_t *text, int n ) const;
	int					IcmpPrefix( const wchar_t *text ) const;

						// case insensitive compare ignoring color
	int					IcmpNoColor( const wchar_t *text ) const;

	int					Length( void ) const;
	int					Allocated( void ) const;
	void				Empty( void );
	bool				IsEmpty( void ) const;
	void				Clear( void );
	void				Append( const wchar_t a );
	void				Append( const idWStr &text );
	void				Append( const wchar_t *text );
	void				Append( const wchar_t *text, int len );
	void				Append( int count, const wchar_t c );
	void				Insert( const wchar_t a, int index );
	void				Insert( const wchar_t *text, int index );
	bool				IsColor( void ) const;
	int					LengthWithoutColors( void ) const;
	idWStr &			RemoveColors( void );
	void				CapLength( int );
	void				Fill( const wchar_t ch, int newlen );
	void				Swap( idWStr& rhs );
	idWStr&				CollapseColors( void );

	int					Find( const wchar_t c, int start = 0, int end = INVALID_POSITION ) const;
	int					Find( const wchar_t *text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION ) const;
	const wchar_t *		Mid( int start, int len, idWStr &result ) const;	// store 'len' characters starting at 'start' in result
	idWStr				Mid( int start, int len ) const;					// return 'len' characters starting at 'start'
	void				Strip( const char *string );						// strip string from front and end as many times as the string occurs
	void				Replace( const wchar_t *old, const wchar_t *nw );
	void				ReplaceFirst( const wchar_t *old, const wchar_t *nw );
	void				ReplaceChar( wchar_t oldChar, wchar_t newChar );
	void				EraseRange( int start, int len = INVALID_POSITION );
	void				EraseChar( const wchar_t c, int start = 0 );	

	idWStr&				StripFileExtension();

	// char * methods to replace library functions
	static int			Length( const wchar_t* s );
	static bool			IsColor( const wchar_t *s );
	static int			LengthWithoutColors( const wchar_t *s );
	static wchar_t *	RemoveColors( wchar_t *s );
	static wchar_t *	StripFilename( wchar_t *s );
	static int			Cmp( const wchar_t *s1, const wchar_t *s2 );
	static int			Cmpn( const wchar_t *s1, const wchar_t *s2, int n );
	static int			Icmp( const wchar_t *s1, const wchar_t *s2 );
	static int			Icmpn( const wchar_t *s1, const wchar_t *s2, int n );
	static int			IcmpNoColor( const wchar_t *s1, const wchar_t *s2 );
	static void			Append( wchar_t *dest, int size, const wchar_t *src );
	static void			Copynz( wchar_t *dest, const wchar_t *src, int destsize );
	static int			snPrintf( wchar_t *dest, int size, const wchar_t *fmt, ... );
	static int			vsnPrintf( wchar_t *dest, int size, const wchar_t *fmt, va_list argptr );
	static int			FindChar( const wchar_t *str, const wchar_t c, int start = 0, int end = INVALID_POSITION );
	static int			FindText( const wchar_t *str, const wchar_t *text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION );

	static const wchar_t*	MS2HMS( double ms, const hmsFormat_t& formatSpec = defaultHMSFormat );

	// hash keys
/*	static int			Hash( const char *string );
	static int			Hash( const char *string, int length );
	static int			IHash( const char *string );					// case insensitive
	static int			IHash( const char *string, int length );		// case insensitive
*/

	// character methods
	static int			ColorIndex( wchar_t c );
	static const idVec4&ColorForIndex( int i );
	static const idVec4&ColorForChar( wchar_t c );
	static const char*	StrForColorIndex( int i );
	static dword&		DColorForIndex( int i );
	static dword&		DColorForChar( wchar_t c );

	friend int			swprintf( idWStr &dest, const wchar_t *fmt, ... );
	friend int			vswprintf( idWStr &dest, const wchar_t *fmt, va_list ap );

	void				ReAllocate( int amount, bool keepold );				// reallocate string data buffer
	void				FreeData( void );									// free allocated string memory

	static void			InitMemory( void );
	static void			ShutdownMemory( void );
	static void			PurgeMemory( void );
	static void			ShowMemoryUsage_f( const idCmdArgs &args );

	static void			SetStringAllocator( wideStringDataAllocator_t* allocator );
	static wideStringDataAllocator_t* GetStringAllocator( void );

	static void			Test( void );

protected:
	int					len;
	wchar_t *			data;
	int					alloced;
	wchar_t				baseBuffer[ STR_ALLOC_BASE ];

	void				Init( void );										// initialize string using base buffer
	void				EnsureAlloced( int amount, bool keepold = true );	// ensure string data buffer is large enough

	static				wideStringDataAllocator_t*	stringDataAllocator;
	static bool			stringAllocatorIsShared;	
	static hmsFormat_t	defaultHMSFormat;

public:
	static const int INVALID_POSITION = -1;
};

/*
============
va utility funcs
============
*/

wchar_t* va( const wchar_t *fmt, ... );


ID_INLINE void idWStr::EnsureAlloced( int amount, bool keepold ) {
	if ( amount > alloced ) {
		ReAllocate( amount, keepold );
	}
}

ID_INLINE void idWStr::Init( void ) {
	len = 0;
	alloced = STR_ALLOC_BASE;
	data = baseBuffer;
	data[ 0 ] = L'\0';
#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
	::wmemset( baseBuffer, 0, sizeof( baseBuffer ) );
#endif
}

ID_INLINE idWStr::idWStr( void ) {
	Init();
}

ID_INLINE idWStr::idWStr( const idWStr &text ) {
	int l;

	Init();
	l = text.Length();
	EnsureAlloced( l + 1 );
	wcscpy( data, text.data );
	len = l;
}

ID_INLINE idWStr::idWStr( const idWStr &text, int start, int end ) {
	int i;
	int l;

	Init();
	if ( end > text.Length() ) {
		end = text.Length();
	}
	if ( start > text.Length() ) {
		start = text.Length();
	} else if ( start < 0 ) {
		start = 0;
	}

	l = end - start;
	if ( l < 0 ) {
		l = 0;
	}

	EnsureAlloced( l + 1 );

	for ( i = 0; i < l; i++ ) {
		data[ i ] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

ID_INLINE idWStr::idWStr( const wchar_t *text ) {
	int l;

	Init();
	if ( text ) {
		l = Length( text );
		EnsureAlloced( l + 1 );
		wcscpy( data, text );
		len = l;
	}
}

ID_INLINE idWStr::idWStr( const wchar_t *text, int start, int end ) {
	int i;
	int l = Length( text );

	Init();
	if ( end > l ) {
		end = l;
	}
	if ( start > l ) {
		start = l;
	} else if ( start < 0 ) {
		start = 0;
	}

	l = end - start;
	if ( l < 0 ) {
		l = 0;
	}

	EnsureAlloced( l + 1 );

	for ( i = 0; i < l; i++ ) {
		data[ i ] = text[ start + i ];
	}

	data[ l ] = '\0';
	len = l;
}

ID_INLINE idWStr::~idWStr( void ) {
	Clear();
}

ID_INLINE size_t idWStr::Size( void ) const {
	return sizeof( *this ) + Allocated() * sizeof( wchar_t );
}

ID_INLINE const wchar_t *idWStr::c_str( void ) const {
	return data;
}

ID_INLINE wchar_t idWStr::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[ index ];
}

ID_INLINE wchar_t &idWStr::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[ index ];
}

ID_INLINE void idWStr::operator=( const idWStr &text ) {
	int l;

	l = text.Length();
	EnsureAlloced( l + 1, false );
	::wmemcpy( data, text.data, l );
	data[l] = L'\0';
	len = l;
}

ID_INLINE idWStr operator+( const idWStr &a, const idWStr &b ) {
	idWStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idWStr operator+( const idWStr &a, const wchar_t *b ) {
	idWStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idWStr operator+( const wchar_t *a, const idWStr &b ) {
	idWStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idWStr operator+( const idWStr &a, const wchar_t b ) {
	idWStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idWStr &idWStr::operator+=( const idWStr &a ) {
	Append( a );
	return *this;
}

ID_INLINE idWStr &idWStr::operator+=( const wchar_t *a ) {
	Append( a );
	return *this;
}

ID_INLINE idWStr &idWStr::operator+=( const wchar_t a ) {
	Append( a );
	return *this;
}

ID_INLINE bool operator==( const idWStr &a, const idWStr &b ) {
	return ( !idWStr::Cmp( a.data, b.data ) );
}

ID_INLINE bool operator==( const idWStr &a, const wchar_t *b ) {
	assert( b );
	return ( !idWStr::Cmp( a.data, b ) );
}

ID_INLINE bool operator==( const wchar_t *a, const idWStr &b ) {
	assert( a );
	return ( !idWStr::Cmp( a, b.data ) );
}

ID_INLINE bool operator!=( const idWStr &a, const idWStr &b ) {
	return !( a == b );
}

ID_INLINE bool operator!=( const idWStr &a, const wchar_t *b ) {
	return !( a == b );
}

ID_INLINE bool operator!=( const wchar_t *a, const idWStr &b ) {
	return !( a == b );
}

ID_INLINE int idWStr::Cmp( const wchar_t *text ) const {
	assert( text );
	return idWStr::Cmp( data, text );
}

ID_INLINE int idWStr::Cmpn( const wchar_t *text, int n ) const {
	assert( text );
	return idWStr::Cmpn( data, text, n );
}

ID_INLINE int idWStr::CmpPrefix( const wchar_t *text ) const {
	assert( text );
	return Cmpn( data, text, Length( text ) );
}

ID_INLINE int idWStr::Icmp( const wchar_t *text ) const {
	assert( text );
	return Icmp( data, text );
}

ID_INLINE int idWStr::Icmpn( const wchar_t *text, int n ) const {
	assert( text );
	return Icmpn( data, text, n );
}

ID_INLINE int idWStr::IcmpPrefix( const wchar_t *text ) const {
	assert( text );
	return Icmpn( data, text, Length( text ) );
}

ID_INLINE int idWStr::IcmpNoColor( const wchar_t *text ) const {
	assert( text );
	return IcmpNoColor( data, text );
}

ID_INLINE int idWStr::Length( void ) const {
	return len;
}

ID_INLINE int idWStr::Allocated( void ) const {
	if ( data != baseBuffer ) {
		return alloced;
	} else {
		return 0;
	}
}

ID_INLINE void idWStr::Empty( void ) {
	EnsureAlloced( 1 );
	data[ 0 ] = L'\0';
	len = 0;
}

ID_INLINE bool idWStr::IsEmpty( void ) const {
	return ( Cmp( data, L"" ) == 0 );
}

ID_INLINE void idWStr::Clear( void ) {
	FreeData();
	Init();
}

ID_INLINE void idWStr::Append( const wchar_t a ) {
	EnsureAlloced( len + 2 );
	data[ len ] = a;
	len++;
	data[ len ] = L'\0';
}

ID_INLINE void idWStr::Append( const idWStr &text ) {
	int newLen;
	int i;

	newLen = len + text.Length();
	EnsureAlloced( newLen + 1 );
	for ( i = 0; i < text.len; i++ ) {
		data[ len + i ] = text[ i ];
	}
	len = newLen;
	data[ len ] = L'\0';
}

ID_INLINE void idWStr::Append( const wchar_t *text ) {
	int newLen;
	int i;

	if ( text ) {
		newLen = len + Length( text );
		EnsureAlloced( newLen + 1 );
		for ( i = 0; text[ i ]; i++ ) {
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = L'\0';
	}
}

ID_INLINE void idWStr::Append( const wchar_t *text, int l ) {
	int newLen;
	int i;

	if ( text && l ) {
		newLen = len + l;
		EnsureAlloced( newLen + 1 );
		for ( i = 0; text[ i ] && i < l; i++ ) {
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = L'\0';
	}
}

ID_INLINE void idWStr::Insert( const wchar_t a, int index ) {
	int i, l;

	if ( index < 0 ) {
		index = 0;
	} else if ( index > len ) {
		index = len;
	}

	l = 1;
	EnsureAlloced( len + l + 1 );
	for ( i = len; i >= index; i-- ) {
		data[i+l] = data[i];
	}
	data[index] = a;
	len++;
}

ID_INLINE void idWStr::Insert( const wchar_t *text, int index ) {
	int i, l;

	if ( index < 0 ) {
		index = 0;
	} else if ( index > len ) {
		index = len;
	}

	l = Length( text );
	EnsureAlloced( len + l + 1 );
	for ( i = len; i >= index; i-- ) {
		data[i+l] = data[i];
	}
	for ( i = 0; i < l; i++ ) {
		data[index+i] = text[i];
	}
	len += l;
}

ID_INLINE bool idWStr::IsColor( void ) const {
	return IsColor( data );
}

ID_INLINE idWStr &idWStr::RemoveColors( void ) {
	RemoveColors( data );
	len = Length( data );
	return *this;
}

ID_INLINE int idWStr::LengthWithoutColors( void ) const {
	return LengthWithoutColors( data );
}

ID_INLINE void idWStr::CapLength( int newlen ) {
	if ( len <= newlen ) {
		return;
	}
	data[ newlen ] = L'\0';
	len = newlen;
}

ID_INLINE void idWStr::Fill( const wchar_t ch, int newlen ) {
	EnsureAlloced( newlen + 1 );
	len = newlen;
	::wmemset( data, ch, len );
	data[ len ] = L'\0';
}

ID_INLINE int idWStr::Find( const wchar_t c, int start, int end ) const {
	if ( end == INVALID_POSITION ) {
		end = len;
	}
	return FindChar( data, c, start, end );
}

ID_INLINE int idWStr::Find( const wchar_t *text, bool casesensitive, int start, int end ) const {
	if ( end == INVALID_POSITION ) {
		end = len;
	}
	return FindText( data, text, casesensitive, start, end );
}

ID_INLINE int idWStr::Length( const wchar_t *s ) {
	int i;
	for ( i = 0; s[i] != L'\0'; i++ ) {}
	return i;
}

ID_INLINE bool idWStr::IsColor( const wchar_t *s ) {
	return ( s && s[0] == WC_COLOR_ESCAPE && s[1] != L'\0' && s[1] != L' ' );
}

ID_INLINE int idWStr::ColorIndex( wchar_t c ) {
	return ( ( c - L'0' ) & W_COLOR_BITS );
}

ID_INLINE void idWStr::Swap( idWStr& rhs ) {
	if( rhs.data != rhs.baseBuffer && data != baseBuffer ) {
		idSwap( data, rhs.data );
		idSwap( len, rhs.len);
		idSwap( alloced, rhs.alloced );
	} else {
		idSwap( *this, rhs );
	}
}

#endif /* !__WSTR_H__ */

