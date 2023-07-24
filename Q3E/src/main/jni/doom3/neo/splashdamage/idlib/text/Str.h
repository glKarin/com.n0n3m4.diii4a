// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __STR_H__
#define __STR_H__

/*
===============================================================================

	Character string

===============================================================================
*/

// these library functions should not be used for cross platform compatibility
#ifndef _WIN32
#define strcmp			idStr::Cmp		// use_idStr_Cmp
#define strlen			idStr::Length	// returns int instead of size_t
#define strncmp			use_idStr_Cmpn
#define StrCmpN			use_idStr_Cmpn
#define strcmpi			use_idStr_Icmp
#define StrCmpI			use_idStr_Icmp
#define stricmp			idStr::Icmp		// use_idStr_Icmp
#define _stricmp		use_idStr_Icmp
#define strcasecmp		use_idStr_Icmp
#define strnicmp		use_idStr_Icmpn
#define _strnicmp		use_idStr_Icmpn
#define _memicmp		use_idStr_Icmpn
#define StrCmpNI		use_idStr_Icmpn
#define snprintf		use_idStr_snPrintf
#define _snprintf		use_idStr_snPrintf
#define vsnprintf		use_idStr_vsnPrintf
#define _vsnprintf		use_idStr_vsnPrintf
#define _vsnwprintf		use_idWStr_vsnPrintf
#define vsnwprintf		use_idWStr_vsnPrintf
#endif

class idVec4;
class idCmdArgs;

#ifndef FILE_HASH_SIZE
#define FILE_HASH_SIZE		1024
#endif

const int COLOR_BITS				= 31;

// color escape character
const int C_COLOR_ESCAPE			= '^';
const int C_COLOR_DEFAULT			= '0';
const int C_COLOR_RED				= '1';
const int C_COLOR_GREEN				= '2';
const int C_COLOR_YELLOW			= '3';
const int C_COLOR_BLUE				= '4';
const int C_COLOR_CYAN				= '5';
const int C_COLOR_MAGENTA			= '6';
const int C_COLOR_WHITE				= '7';
const int C_COLOR_GRAY				= '8';
const int C_COLOR_BLACK				= '9';
const int C_COLOR_LTGREY			= ':';
const int C_COLOR_MDGREEN			= '<';
const int C_COLOR_MDYELLOW			= '=';
const int C_COLOR_MDBLUE			= '>';
const int C_COLOR_MDRED				= '?';
const int C_COLOR_LTORANGE			= 'A';
const int C_COLOR_MDCYAN			= 'B';
const int C_COLOR_MDPURPLE			= 'C';
const int C_COLOR_ORANGE			= 'D';

// color escape string
#define S_COLOR_DEFAULT				"^0"
#define S_COLOR_RED					"^1"
#define S_COLOR_GREEN				"^2"
#define S_COLOR_YELLOW				"^3"
#define S_COLOR_BLUE				"^4"
#define S_COLOR_CYAN				"^5"
#define S_COLOR_MAGENTA				"^6"
#define S_COLOR_WHITE				"^7"
#define S_COLOR_GRAY				"^8"
#define S_COLOR_BLACK				"^9"
#define S_COLOR_LTGREY				"^:"
#define S_COLOR_MDGREEN				"^<"
#define S_COLOR_MDYELLOW			"^="
#define S_COLOR_MDBLUE				"^>"
#define S_COLOR_MDRED				"^?"
#define S_COLOR_LTORANGE			"^A"
#define S_COLOR_MDCYAN				"^B"
#define S_COLOR_MDPURPLE			"^C"
#define S_COLOR_ORANGE				"^D"

// make idStr a multiple of 16 bytes long
// don't make too large to keep memory requirements to a minimum
const int STR_ALLOC_BASE			= 20;
const int STR_ALLOC_GRAN			= 32;

enum measure_t {
	MEASURE_SIZE = 0,
	MEASURE_BANDWIDTH
};

#ifdef ID_THREAD_SAFE_STR
typedef idDynamicBlockAlloc< char, 1<<18, 128, true >	stringDataAllocator_t;
#else
typedef idDynamicBlockAlloc< char, 1<<18, 128, false >	stringDataAllocator_t;
#endif

class idStr {

public:
	struct hmsFormat_t {
		hmsFormat_t() : showZeroMinutes( false ), showZeroHours( false ), showZeroSeconds( true ) {}
		bool showZeroMinutes;
		bool showZeroHours;
		bool showZeroSeconds;
	};

						idStr( void );
						idStr( const idStr &text );
						idStr( const idStr &text, int start, int end );
						idStr( const char *text );
						idStr( const char *text, int start, int end );
						explicit idStr( const bool b );
						explicit idStr( const char c );
						explicit idStr( const int i );
						explicit idStr( const unsigned u );
						explicit idStr( const float f );
						~idStr( void );

	size_t				Size( void ) const;
	const char *		c_str( void ) const;
	operator			const char *( void ) const;
	operator			const char *( void );

	char				operator[]( int index ) const;
	char &				operator[]( int index );

	void				operator=( const idStr &text );
	void				operator=( const char *text );

	friend idStr		operator+( const idStr &a, const idStr &b );
	friend idStr		operator+( const idStr &a, const char *b );
	friend idStr		operator+( const char *a, const idStr &b );

	friend idStr		operator+( const idStr &a, const float b );
	friend idStr		operator+( const idStr &a, const int b );
	friend idStr		operator+( const idStr &a, const unsigned b );
	friend idStr		operator+( const idStr &a, const bool b );
	friend idStr		operator+( const idStr &a, const char b );

	idStr &				operator+=( const idStr &a );
	idStr &				operator+=( const char *a );
	idStr &				operator+=( const float a );
	idStr &				operator+=( const char a );
	idStr &				operator+=( const int a );
	idStr &				operator+=( const unsigned a );
	idStr &				operator+=( const bool a );

						// case sensitive compare
	friend bool			operator==( const idStr &a, const idStr &b );
	friend bool			operator==( const idStr &a, const char *b );
	friend bool			operator==( const char *a, const idStr &b );

						// case sensitive compare
	friend bool			operator!=( const idStr &a, const idStr &b );
	friend bool			operator!=( const idStr &a, const char *b );
	friend bool			operator!=( const char *a, const idStr &b );

						// case sensitive compare
	int					Cmp( const char *text ) const;
	int					Cmpn( const char *text, int n ) const;
	int					CmpPrefix( const char *text ) const;

						// case insensitive compare
	int					Icmp( const char *text ) const;
	int					Icmpn( const char *text, int n ) const;
	int					IcmpPrefix( const char *text ) const;

						// case insensitive compare ignoring color
	int					IcmpNoColor( const char *text ) const;

						// compares paths and makes sure folders come first
	int					IcmpPath( const char *text ) const;
	int					IcmpnPath( const char *text, int n ) const;
	int					IcmpPrefixPath( const char *text ) const;

	int					Length( void ) const;
	int					Allocated( void ) const;
	void				Empty( void );
	bool				IsEmpty( void ) const;
	void				Clear( void );
	void				Append( const char a );
	void				Append( const idStr &text );
	void				Append( const char *text );
	void				Append( const char *text, int len );
	void				Append( int count, const char c );
	void				Insert( const char a, int index );
	void				Insert( const char *text, int index );
	void				ToLower( void );
	void				ToUpper( void );
	bool				IsNumeric( void ) const;
	bool				IsColor( void ) const;
	bool				IsHexColor( void ) const;
	bool				HasHexColorAlpha( void ) const;
	bool				HasLower( void ) const;
	bool				HasUpper( void ) const;
	int					LengthWithoutColors( void ) const;
	idStr &				RemoveColors( void );
	void				CapLength( int );
	void				Fill( const char ch, int newlen );
	void				Swap( idStr& rhs );

	int					Find( const char c, int start = 0, int end = INVALID_POSITION ) const;
	int					Find( const char *text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION ) const;
	const char*			FindString( const char* text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION ) const;
	int					CountChar( const char c );
	bool				Filter( const char *filter, bool casesensitive ) const;
	int					Last( const char c, int index = INVALID_POSITION ) const;						// return the index to the last occurance of 'c', returns INVALID_POSITION if not found
	int					Last( const char* str, bool casesensitive = true, int index = INVALID_POSITION ) const;						// return the index to the last occurance of 'c', returns INVALID_POSITION if not found
	const char *		Left( int len, idStr &result ) const;			// store the leftmost 'len' characters in the result
	const char *		Right( int len, idStr &result ) const;			// store the rightmost 'len' characters in the result
	const char *		Mid( int start, int len, idStr &result ) const;	// store 'len' characters starting at 'start' in result
	idStr				Left( int len ) const;							// return the leftmost 'len' characters
	idStr				Right( int len ) const;							// return the rightmost 'len' characters
	idStr				Mid( int start, int len ) const;				// return 'len' characters starting at 'start'
	void				StripLeading( const char c );					// strip char from front as many times as the char occurs
	void				StripLeading( const char *string );				// strip string from front as many times as the string occurs
	bool				StripLeadingOnce( const char *string );			// strip string from front just once if it occurs
	void				StripTrailing( const char c );					// strip char from end as many times as the char occurs
	void				StripTrailing( const char *string );			// strip string from end as many times as the string occurs
	bool				StripTrailingOnce( const char *string );		// strip string from end just once if it occurs
	void				Strip( const char c );							// strip char from front and end as many times as the char occurs
	void				Strip( const char *string );					// strip string from front and end as many times as the string occurs
	void				StripLeadingWhiteSpace( void );					// strip leading white space characters
	void				StripTrailingWhiteSpace( void );				// strip trailing white space characters
	idStr &				StripQuotes( void );							// strip quotes around string
	void				Replace( const char *old, const char *nw );
	void				ReplaceFirst( const char *old, const char *nw );
	void				ReplaceChar( char oldChar, char newChar );
	void				EraseRange( int start, int len = INVALID_POSITION );
	void				EraseChar( const char c, int start = 0 );	

	// file name methods
	int					FileNameHash( const int hashSize ) const;						// hash key for the filename (skips extension)
	idStr &				CollapsePath( void );							// where possible removes /../ and /./ from path
	idStr &				BackSlashesToSlashes( void );					// convert slashes
	idStr &				SlashesToBackSlashes( void );					// convert slashes
	idStr &				SetFileExtension( const char *extension );		// set the given file extension
	idStr &				StripFileExtension( void );						// remove any file extension
	idStr &				StripAbsoluteFileExtension( void );				// remove any file extension looking from front (useful if there are multiple .'s)
	idStr &				DefaultFileExtension( const char *extension );	// if there's no file extension use the default
	idStr &				DefaultPath( const char *basepath );			// if there's no path use the default
	void				AppendPath( const char *text );					// append a partial path
	idStr &				StripFilename( void );							// remove the filename from a path
	idStr &				StripPath( void );								// remove the path from the filename
	void				ExtractFilePath( idStr &dest ) const;			// copy the file path to another string
	void				ExtractFileName( idStr &dest ) const;			// copy the filename to another string
	void				ExtractFileBase( idStr &dest ) const;			// copy the filename minus the extension to another string
	void				ExtractFileExtension( idStr &dest ) const;		// copy the file extension to another string ( the period will not be included in the dest )
	bool				CheckExtension( const char *ext );
	idStr&				StripComments();								// remove C++ and C style comments
	idStr& 				Indent();										// indents brace-delimited text, preserving tabs in the middle of lines
	idStr& 				Unindent();										// unindents brace-delimited text, preserving tabs in the middle of lines
	idStr&				CleanFilename( void );							// strips bad characters
	bool				IsValidEmailAddress( void );
	idStr&				CollapseColors( void );							// removes redundant color codes

// jscott: like the declManager version, but globally accessible
	void				MakeNameCanonical( void );

	// char * methods to replace library functions
	static int			Length( const char *s );
	static char *		ToLower( char *s );
	static char *		ToUpper( char *s );
	static bool			IsNumeric( const char *s );
	static bool			IsColor( const char *s );
	static bool			IsHexColor( const char *s );
	static bool			HasHexColorAlpha( const char *s );
	static bool			HasLower( const char *s );
	static bool			HasUpper( const char *s );
	static int			LengthWithoutColors( const char *s );
	static char *		RemoveColors( char *s );
	static bool			IsBadFilenameChar( char c );
	static char *		CleanFilename( char *s );
	static char *		StripFilename( char *s );
	static char *		StripPath( char *s );
	static int			Cmp( const char *s1, const char *s2 );
	static int			Cmpn( const char *s1, const char *s2, int n );
	static int			Icmp( const char *s1, const char *s2 );
	static int			Icmpn( const char *s1, const char *s2, int n );
	static int			IcmpNoColor( const char *s1, const char *s2 );
	static int			IcmpPath( const char *s1, const char *s2 );			// compares paths and makes sure folders come first
	static int			IcmpnPath( const char *s1, const char *s2, int n );	// compares paths and makes sure folders come first
	static void			Append( char *dest, int size, const char *src );
	static void			Copynz( char *dest, const char *src, int destsize );
	static int			snPrintf( char *dest, int size, const char *fmt, ... );
	static int			vsnPrintf( char *dest, int size, const char *fmt, va_list argptr );
	static int			FindChar( const char *str, const char c, int start = 0, int end = INVALID_POSITION );
	static int			FindText( const char *str, const char *text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION );
	static const char*	FindString( const char *str, const char *text, bool casesensitive = true, int start = 0, int end = INVALID_POSITION );
	static int			CountChar( const char *str, const char c );
	static bool			Filter( const char *filter, const char *name, bool casesensitive );
	static void			StripMediaName( const char *name, idStr &mediaName );
	static bool			CheckExtension( const char *name, const char *ext );
	static const char *	FloatArrayToString( const float *array, const int length, const int precision );
	static int			NumLonelyLF( const char *src );	// return the number of line feeds not paired with a carriage return... usefull for getting a correct destination buffer size for ToCRLF
	static bool			ToCRLF( const char *src, char *dest, int maxLength );
	static const char *	CStyleQuote( const char *str );
	static const char *	CStyleUnQuote( const char *str );
	static void			IndentAndPad( int indent, int pad, idStr &str, const char *fmt, ... );  // indent and pad out formatted text	

	static void			StringToBinaryString( idStr& out, void *pv, int size);
	static bool			BinaryStringToString( const char* str,  void *pv, int size );

    static bool			IsValidEmailAddress( const char* address );

	static const char*	MS2HMS( double ms, const hmsFormat_t& formatSpec = defaultHMSFormat );
	static const char * FormatInt( const int num );	 // formats an integer as a value with commas

	// hash keys
	static int			Hash( const char *string );
	static int			Hash( const char *string, int length );
	static int			IHash( const char *string );					// case insensitive
	static int			IHash( const char *string, int length );		// case insensitive
	static int			FileNameHash( const char *string, const int hashSize );	// hash key for the filename (skips extension)

	// character methods
	static char			ToLower( char c );
	static char			ToUpper( char c );
	static bool			CharIsPrintable( int c );
	static bool			CharIsLower( int c );
	static bool			CharIsUpper( int c );
	static bool			CharIsAlpha( int c );
	static bool			CharIsNumeric( int c );
	static bool			CharIsNewLine( char c );
	static bool			CharIsTab( char c );
	static bool			CharIsHex( int c );
	static int			ColorIndex( int c );
	static const idVec4&ColorForIndex( int i );
	static const idVec4&ColorForChar( int c );
	static const char*	StrForColorIndex( int i );
	static int			HexForChar( int c );

	friend int			sprintf( idStr &dest, const char *fmt, ... );
	friend int			vsprintf( idStr &dest, const char *fmt, va_list ap );

	void				ReAllocate( int amount, bool keepold );				// reallocate string data buffer
	void				FreeData( void );									// free allocated string memory
	void				SetStaticBuffer( char *buffer, int l );

						// format value in the given measurement with the best unit, returns the best unit
	int					BestUnit( const char *format, float value, measure_t measure );
						// format value in the requested unit and measurement
	void				SetUnit( const char *format, float value, int unit, measure_t measure );


	static void			InitMemory( void );
	static void			ShutdownMemory( void );
	static void			PurgeMemory( void );
	static void			ShowMemoryUsage_f( const idCmdArgs &args );

	static void			SetStringAllocator( stringDataAllocator_t* allocator );
	static stringDataAllocator_t* GetStringAllocator( void );

	static void			Test( void );

protected:
	int					len;
	char *				data;
	int					alloced;
	char				baseBuffer[ STR_ALLOC_BASE ];

	void				Init( void );										// initialize string using base buffer
	void				EnsureAlloced( int amount, bool keepold = true );	// ensure string data buffer is large enough

	static				stringDataAllocator_t*	stringDataAllocator;
	static				bool					stringAllocatorIsShared;	
	static hmsFormat_t	defaultHMSFormat;

public:
	static const int	INVALID_POSITION = -1;
};


/*
============
va utility funcs
============
*/

char *					va( const char *fmt, ... );
char *					vva( char *buf, const char *fmt, ... );
char *					va_floatstring( const char *fmt, ... );


ID_INLINE void idStr::EnsureAlloced( int amount, bool keepold ) {
	if ( amount > abs(alloced) ) {
		ReAllocate( amount, keepold );
	}
}

ID_INLINE void idStr::Init( void ) {
	len = 0;
	alloced = -STR_ALLOC_BASE;
	data = baseBuffer;
	data[ 0 ] = '\0';
#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
	memset( baseBuffer, 0, sizeof( baseBuffer ) );
#endif
}

ID_INLINE idStr::idStr( void ) {
	Init();
}

ID_INLINE idStr::idStr( const idStr &text ) {
	int l;

	Init();
	l = text.Length();
	EnsureAlloced( l + 1 );
	strcpy( data, text.data );
	len = l;
}

ID_INLINE idStr::idStr( const idStr &text, int start, int end ) {
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

ID_INLINE idStr::idStr( const char *text ) {
	int l;

	Init();
	if ( text ) {
		l = (unsigned)strlen( text );
		EnsureAlloced( l + 1 );
		strcpy( data, text );
		len = l;
	}
}

ID_INLINE idStr::idStr( const char *text, int start, int end ) {
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

ID_INLINE idStr::idStr( const bool b ) {
	Init();
	EnsureAlloced( 2 );
	data[ 0 ] = b ? '1' : '0';
	data[ 1 ] = '\0';
	len = 1;
}

ID_INLINE idStr::idStr( const char c ) {
	Init();
	EnsureAlloced( 2 );
	data[ 0 ] = c;
	data[ 1 ] = '\0';
	len = 1;
}

ID_INLINE idStr::idStr( const int i ) {
	char text[ 64 ];
	int l;

	Init();
	l = sprintf( text, "%d", i );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ID_INLINE idStr::idStr( const unsigned u ) {
	char text[ 64 ];
	int l;

	Init();
	l = sprintf( text, "%u", u );
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ID_INLINE idStr::idStr( const float f ) {
	char text[ 64 ];
	int l;

	Init();
	l = idStr::snPrintf( text, sizeof( text ), "%f", f );
	while( l > 0 && text[l-1] == '0' ) text[--l] = '\0';
	while( l > 0 && text[l-1] == '.' ) text[--l] = '\0';
	EnsureAlloced( l + 1 );
	strcpy( data, text );
	len = l;
}

ID_INLINE idStr::~idStr( void ) {
	Clear();
}

ID_INLINE size_t idStr::Size( void ) const {
	return sizeof( *this ) + Allocated();
}

ID_INLINE const char *idStr::c_str( void ) const {
	return data;
}

ID_INLINE idStr::operator const char *( void ) {
	return c_str();
}

ID_INLINE idStr::operator const char *( void ) const {
	return c_str();
}

ID_INLINE char idStr::operator[]( int index ) const {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[ index ];
}

ID_INLINE char &idStr::operator[]( int index ) {
	assert( ( index >= 0 ) && ( index <= len ) );
	return data[ index ];
}

ID_INLINE void idStr::operator=( const idStr &text ) {
	int l;

	l = text.Length();
	EnsureAlloced( l + 1, false );
	memcpy( data, text.data, l );
	data[l] = '\0';
	len = l;
}

ID_INLINE idStr operator+( const idStr &a, const idStr &b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr operator+( const idStr &a, const char *b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr operator+( const char *a, const idStr &b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr operator+( const idStr &a, const bool b ) {
	idStr result( a );
	result.Append( b ? "true" : "false" );
	return result;
}

ID_INLINE idStr operator+( const idStr &a, const char b ) {
	idStr result( a );
	result.Append( b );
	return result;
}

ID_INLINE idStr operator+( const idStr &a, const float b ) {
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%f", b );
	result.Append( text );

	return result;
}

ID_INLINE idStr operator+( const idStr &a, const int b ) {
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%d", b );
	result.Append( text );

	return result;
}

ID_INLINE idStr operator+( const idStr &a, const unsigned b ) {
	char	text[ 64 ];
	idStr	result( a );

	sprintf( text, "%u", b );
	result.Append( text );

	return result;
}

ID_INLINE idStr &idStr::operator+=( const float a ) {
	char text[ 64 ];

	sprintf( text, "%f", a );
	Append( text );

	return *this;
}

ID_INLINE idStr &idStr::operator+=( const int a ) {
	char text[ 64 ];

	sprintf( text, "%d", a );
	Append( text );

	return *this;
}

ID_INLINE idStr &idStr::operator+=( const unsigned a ) {
	char text[ 64 ];

	sprintf( text, "%u", a );
	Append( text );

	return *this;
}

ID_INLINE idStr &idStr::operator+=( const idStr &a ) {
	Append( a );
	return *this;
}

ID_INLINE idStr &idStr::operator+=( const char *a ) {
	Append( a );
	return *this;
}

ID_INLINE idStr &idStr::operator+=( const char a ) {
	Append( a );
	return *this;
}

ID_INLINE idStr &idStr::operator+=( const bool a ) {
	Append( a ? "true" : "false" );
	return *this;
}

ID_INLINE bool operator==( const idStr &a, const idStr &b ) {
	return ( !idStr::Cmp( a.data, b.data ) );
}

ID_INLINE bool operator==( const idStr &a, const char *b ) {
	assert( b );
	return ( !idStr::Cmp( a.data, b ) );
}

ID_INLINE bool operator==( const char *a, const idStr &b ) {
	assert( a );
	return ( !idStr::Cmp( a, b.data ) );
}

ID_INLINE bool operator!=( const idStr &a, const idStr &b ) {
	return !( a == b );
}

ID_INLINE bool operator!=( const idStr &a, const char *b ) {
	return !( a == b );
}

ID_INLINE bool operator!=( const char *a, const idStr &b ) {
	return !( a == b );
}

ID_INLINE int idStr::Cmp( const char *text ) const {
	assert( text );
	return idStr::Cmp( data, text );
}

ID_INLINE int idStr::Cmpn( const char *text, int n ) const {
	assert( text );
	return idStr::Cmpn( data, text, n );
}

ID_INLINE int idStr::CmpPrefix( const char *text ) const {
	assert( text );
	return Cmpn( data, text, Length( text ) );
}

ID_INLINE int idStr::Icmp( const char *text ) const {
	assert( text );
	return Icmp( data, text );
}

ID_INLINE int idStr::Icmpn( const char *text, int n ) const {
	assert( text );
	return Icmpn( data, text, n );
}

ID_INLINE int idStr::IcmpPrefix( const char *text ) const {
	assert( text );
	return Icmpn( data, text, Length( text ) );
}

ID_INLINE int idStr::IcmpNoColor( const char *text ) const {
	assert( text );
	return IcmpNoColor( data, text );
}

ID_INLINE int idStr::IcmpPath( const char *text ) const {
	assert( text );
	return IcmpPath( data, text );
}

ID_INLINE int idStr::IcmpnPath( const char *text, int n ) const {
	assert( text );
	return IcmpnPath( data, text, n );
}

ID_INLINE int idStr::IcmpPrefixPath( const char *text ) const {
	assert( text );
	return IcmpnPath( data, text, Length( text ) );
}

ID_INLINE int idStr::Length( void ) const {
	return len;
}

ID_INLINE int idStr::Allocated( void ) const {
	if ( data != baseBuffer ) {
		return abs( alloced );
	} else {
		return 0;
	}
}

ID_INLINE void idStr::Empty( void ) {
	EnsureAlloced( 1 );
	data[ 0 ] = '\0';
	len = 0;
}

ID_INLINE bool idStr::IsEmpty( void ) const {
	return ( Cmp( data, "" ) == 0 );
}

ID_INLINE void idStr::Clear( void ) {
	FreeData();
	Init();
}

ID_INLINE void idStr::Append( const char a ) {
	EnsureAlloced( len + 2 );
	data[ len ] = a;
	len++;
	data[ len ] = '\0';
}

ID_INLINE void idStr::Append( const idStr &text ) {
	int newLen;
	int i;

	newLen = len + text.Length();
	EnsureAlloced( newLen + 1 );
	for ( i = 0; i < text.len; i++ ) {
		data[ len + i ] = text[ i ];
	}
	len = newLen;
	data[ len ] = '\0';
}

ID_INLINE void idStr::Append( const char *text ) {
	int newLen;
	int i;

	if ( text ) {
		newLen = len + Length( text );
		EnsureAlloced( newLen + 1 );
		for ( i = 0; text[ i ]; i++ ) {
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

ID_INLINE void idStr::Append( const char *text, int l ) {
	int newLen;
	int i;

	if ( text && l ) {
		newLen = len + l;
		EnsureAlloced( newLen + 1 );
		for ( i = 0; text[ i ] && i < l; i++ ) {
			data[ len + i ] = text[ i ];
		}
		len = newLen;
		data[ len ] = '\0';
	}
}

ID_INLINE void idStr::Insert( const char a, int index ) {
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

ID_INLINE void idStr::Insert( const char *text, int index ) {
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

ID_INLINE void idStr::ToLower( void ) {
	for (int i = 0; data[i]; i++ ) {
		if ( CharIsUpper( data[i] ) ) {
			data[i] += ( 'a' - 'A' );
		}
	}
}

ID_INLINE void idStr::ToUpper( void ) {
	for (int i = 0; data[i]; i++ ) {
		if ( CharIsLower( data[i] ) ) {
			data[i] -= ( 'a' - 'A' );
		}
	}
}

ID_INLINE bool idStr::IsNumeric( void ) const {
	return IsNumeric( data );
}

ID_INLINE bool idStr::IsColor( void ) const {
	return IsColor( data );
}

ID_INLINE bool idStr::IsHexColor( void ) const {
	return IsHexColor( data );
}

ID_INLINE bool idStr::HasHexColorAlpha( void ) const {
	return HasHexColorAlpha( data );
}

ID_INLINE bool idStr::HasLower( void ) const {
	return HasLower( data );
}

ID_INLINE bool idStr::HasUpper( void ) const {
	return HasUpper( data );
}

ID_INLINE idStr &idStr::RemoveColors( void ) {
	RemoveColors( data );
	len = Length( data );
	return *this;
}

ID_INLINE int idStr::LengthWithoutColors( void ) const {
	return LengthWithoutColors( data );
}

ID_INLINE void idStr::CapLength( int newlen ) {
	if ( len <= newlen ) {
		return;
	}
	data[ newlen ] = '\0';
	len = newlen;
}

ID_INLINE void idStr::Fill( const char ch, int newlen ) {
	EnsureAlloced( newlen + 1 );
	len = newlen;
	memset( data, ch, len );
	data[ len ] = '\0';
}

ID_INLINE int idStr::Find( const char c, int start, int end ) const {
	if ( end == INVALID_POSITION ) {
		end = len;
	}
	return FindChar( data, c, start, end );
}

ID_INLINE int idStr::Find( const char *text, bool casesensitive, int start, int end ) const {
	if ( end == INVALID_POSITION ) {
		end = len;
	}
	return FindText( data, text, casesensitive, start, end );
}

ID_INLINE const char* idStr::FindString( const char* text, bool casesensitive, int start, int end ) const {
	int i;

	if ( end == INVALID_POSITION ) {
		end = len;
	}

	i = FindText( data, text, casesensitive, start, end );
	if ( i == INVALID_POSITION ) {
		return NULL;
	} else {
		return &data[ i ];
	}
}

ID_INLINE const char* idStr::FindString( const char *str, const char *text, bool casesensitive, int start, int end ) {
	int i;

	if ( end == INVALID_POSITION ) {
		end = idStr::Length( str );
	}

	i = FindText( str, text, casesensitive, start, end );
	if ( i == INVALID_POSITION ) {
		return NULL;
	} else {
		return &str[ i ];
	}
}

ID_INLINE bool idStr::Filter( const char *filter, bool casesensitive ) const {
	return Filter( filter, data, casesensitive );
}

ID_INLINE const char *idStr::Left( int len, idStr &result ) const {
	return Mid( 0, len, result );
}

ID_INLINE const char *idStr::Right( int len, idStr &result ) const {
	if ( len >= Length() ) {
		result = *this;
		return result;
	}
	return Mid( Length() - len, len, result );
}

ID_INLINE idStr idStr::Left( int len ) const {
	return Mid( 0, len );
}

ID_INLINE idStr idStr::Right( int len ) const {
	if ( len >= Length() ) {
		return *this;
	}
	return Mid( Length() - len, len );
}

ID_INLINE void idStr::Strip( const char c ) {
	StripLeading( c );
	StripTrailing( c );
}

ID_INLINE void idStr::Strip( const char *string ) {
	StripLeading( string );
	StripTrailing( string );
}

ID_INLINE bool idStr::CheckExtension( const char *ext ) {
	return CheckExtension( data, ext );
}

ID_INLINE idStr& idStr::CleanFilename( void ) {
	CleanFilename( data );
	len = Length( data );
	return *this;
}

ID_INLINE int idStr::Length( const char *s ) {
	int i;
	for ( i = 0; s[i] != '\0'; i++ ) {}
	return i;
}

ID_INLINE char *idStr::ToLower( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsUpper( s[i] ) ) {
			s[i] += ( 'a' - 'A' );
		}
	}
	return s;
}

ID_INLINE char *idStr::ToUpper( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
		if ( CharIsLower( s[i] ) ) {
			s[i] -= ( 'a' - 'A' );
		}
	}
	return s;
}

ID_INLINE int idStr::Hash( const char *string ) {
	int i, hash = 0;
	for ( i = 0; *string != '\0'; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

ID_INLINE int idStr::Hash( const char *string, int length ) {
	int i, hash = 0;
	for ( i = 0; i < length; i++ ) {
		hash += ( *string++ ) * ( i + 119 );
	}
	return hash;
}

ID_INLINE int idStr::IHash( const char *string ) {
	int i, hash = 0;
	for( i = 0; *string != '\0'; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

ID_INLINE int idStr::IHash( const char *string, int length ) {
	int i, hash = 0;
	for ( i = 0; i < length; i++ ) {
		hash += ToLower( *string++ ) * ( i + 119 );
	}
	return hash;
}

ID_INLINE int idStr::FileNameHash( const int hashSize ) const {
	return FileNameHash( data, hashSize );
}

ID_INLINE bool idStr::IsColor( const char *s ) {
	return ( s && s[0] == C_COLOR_ESCAPE && s[1] != '\0' && s[1] != ' ' );
}

ID_INLINE bool idStr::IsHexColor( const char *s ) {
	int i;
	for ( i = 0; s[i] && i < 6; i++ ) {
		if( !CharIsHex( s[i] ) ) {
			return false;
		}
	}
	return ( i == 6 );
}

ID_INLINE bool idStr::HasHexColorAlpha( const char *s ) {
	int i;
	for ( i = 6; s[i] && i < 8; i++ ) {
		if( !CharIsHex( s[i] ) ) {
			return false;
		}
	}
	return ( i == 8 );
}

ID_INLINE char idStr::ToLower( char c ) {
	if ( c <= 'Z' && c >= 'A' ) {
		return ( c + ( 'a' - 'A' ) );
	}
	return c;
}

ID_INLINE char idStr::ToUpper( char c ) {
	if ( c >= 'a' && c <= 'z' ) {
		return ( c - ( 'a' - 'A' ) );
	}
	return c;
}

ID_INLINE bool idStr::CharIsPrintable( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 0x20 && c <= 0x7E ) || ( c >= 0xA1 && c <= 0xFF );
}

ID_INLINE bool idStr::CharIsLower( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c >= 'a' && c <= 'z' ) || ( c >= 0xE0 && c <= 0xFF );
}

ID_INLINE bool idStr::CharIsUpper( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( c <= 'Z' && c >= 'A' ) || ( c >= 0xC0 && c <= 0xDF );
}

ID_INLINE bool idStr::CharIsAlpha( int c ) {
	// test for regular ascii and western European high-ascii chars
	return ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
			 ( c >= 0xC0 && c <= 0xFF ) );
}

ID_INLINE bool idStr::CharIsNumeric( int c ) {
	return ( c <= '9' && c >= '0' );
}

ID_INLINE bool idStr::CharIsNewLine( char c ) {
	return ( c == '\n' || c == '\r' || c == '\v' );
}

ID_INLINE bool idStr::CharIsTab( char c ) {
	return ( c == '\t' );
}

ID_INLINE bool idStr::CharIsHex( int c ) {
	return ( ( c >= '0' && c <= '9' ) || ( c >= 'A' && c <= 'F' ) || ( c >= 'a' && c <= 'f' ) );
}

ID_INLINE int idStr::ColorIndex( int c ) {
	return ( ( c - '0' ) & COLOR_BITS );
}

ID_INLINE int idStr::HexForChar( int c ) {
	return ( c > '9' ? ( c >= 'a' ? ( c - 'a' + 10 ) : ( c - '7' ) ) : ( c - '0' ) );
}

ID_INLINE int idStr::CountChar( const char c ) {
	return CountChar( data, c );
}

ID_INLINE void idStr::Swap( idStr& rhs ) {
	if( rhs.data != rhs.baseBuffer && data != baseBuffer ) {
		idSwap( data, rhs.data );
		idSwap( len, rhs.len);
		idSwap( alloced, rhs.alloced );
	} else {
		idSwap( *this, rhs );
	}
}

ID_INLINE bool idStr::IsValidEmailAddress( void ) {
	return IsValidEmailAddress( data );
}

#endif /* !__STR_H__ */

