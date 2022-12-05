
#ifndef __STR_H__
#define __STR_H__

/*
===============================================================================

	Character string

===============================================================================
*/

// these library functions should not be used for cross platform compatibility
#define strcmp			idStr::Cmp		// use_idStr_Cmp
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

#ifdef _XENON
class __declspec(align(16)) idVec4;
#else
class idVec4;
#endif

#ifndef FILE_HASH_SIZE
#define FILE_HASH_SIZE		1024
#endif

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
// RAVEN BEGIN
// bdube: added
const int C_COLOR_CONSOLE			= ':';
// RAVEN END

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
// RAVEN BEGIN
// bdube: added
#define S_COLOR_CONSOLE				"^:"
// ddynerman: team colors
#define S_COLOR_MARINE				"^c683"
#define S_COLOR_STROGG				"^c950"
#define S_COLOR_ALERT				"^c920"
// ddynerman: MP icons
#define I_VOICE_ENABLED				"^ivce"
#define I_VOICE_DISABLED			"^ivcd"
#define I_FRIEND_ENABLED			"^ifde"
#define I_FRIEND_DISABLED			"^ifdd"
#define I_FLAG_MARINE				"^iflm"
#define I_FLAG_STROGG				"^ifls"
#define I_READY						"^iyrd"
#define I_NOT_READY					"^inrd"
// shouchard:  server browser stuff
#define I_SERVER_DEDICATED			"^ids0"
#define I_SERVER_DEDICATED_PB		"^idsp"
#define I_SERVER_LOCKED				"^isl0"
#define I_SERVER_FAVORITE			"^isf0"
const int VA_BUF_LEN = 16384;
const int VA_NUM_BUFS = 8;
// RAVEN END

// make idStr a multiple of 16 bytes long
// don't make too large to keep memory requirements to a minimum
const int STR_ALLOC_BASE			= 20;
const int STR_ALLOC_GRAN			= 32;

typedef enum {
	MEASURE_SIZE = 0,
	MEASURE_BANDWIDTH
} Measure_t;

// RAVEN BEGIN
// bdube: string escape codes
#define S_ESCAPE_UNKNOWN			BIT(0)
#define S_ESCAPE_COLOR				BIT(1)
#define S_ESCAPE_COLORINDEX			BIT(2)
#define S_ESCAPE_ICON				BIT(3)
#define S_ESCAPE_COMMAND			BIT(4)
#define	S_ESCAPE_ALL				( S_ESCAPE_COLOR | S_ESCAPE_COLORINDEX | S_ESCAPE_ICON | S_ESCAPE_COMMAND )
// RAVEN END

class idStr {

public:
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
// RAVEN BEGIN
// bdube: changed to escapes
	int					IcmpNoEscape( const char *text ) const;
// RAVEN END

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
	void				Insert( const char a, int index );
	void				Insert( const char *text, int index );
	void				ToLower( void );
	void				ToUpper( void );
	bool				IsNumeric( void ) const;
	bool				HasLower( void ) const;
	bool				HasUpper( void ) const;
// RAVEN BEGIN
// bdube: escapes
	int					IsEscape( int* type = NULL ) const;
	int					LengthWithoutEscapes ( void ) const;	
	idStr &				RemoveEscapes ( int escapes = S_ESCAPE_ALL );
// RAVEN END
	void				CapLength( int );
	void				Fill( const char ch, int newlen );
	int					Find( const char c, int start = 0, int end = -1 ) const;
	int					Find( const char *text, bool casesensitive = true, int start = 0, int end = -1 ) const;
	bool				Filter( const char *filter, bool casesensitive = true ) const;
	int					Last( const char c ) const;						// return the index to the last occurance of 'c', returns -1 if not found
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
	void				StripTrailingWhitespace( void );				// strip trailing white space characters
// RAVEN BEGIN
	void				StripUntil( const char c );						// strip until this character is reached or there is no string left
// RAVEN END
	idStr &				StripQuotes( void );							// strip quotes around string
// RAVEN BEGIN
// scork: added replacement-count return
	int					Replace( const char *old, const char *nw );
// RAVEN END


	// file name methods
	int					FileNameHash( void ) const;						// hash key for the filename (skips extension)
	idStr &				BackSlashesToSlashes( void );					// convert slashes
// RAVEN BEGIN
// jscott:
	idStr &				SlashesToBackSlashes( void );					// convert slashes
// nmckenzie: char swapping routine
	bool				HasChar( const char check );
	bool				HasChars( const char *check );
	idStr &				ReplaceChar( const char from, const char to );
	idStr &				ReplaceChars( const char *from, const char to );
// RAVEN END
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
	void				ExtractFileExtension( idStr &dest ) const;		// copy the file extension to another string
	bool				CheckExtension( const char *ext );
	void				ScrubFileName( void );							// Turns a bad file name into a good one or your money back

// RAVEN BEGIN
// jscott: like the declManager version, but globally accessable
	void				MakeNameCanonical( void );
	void				EnsurePrintable( void );
// RAVEN END

// RAVEN BEGIN
// abahr
	void				Split( idList<idStr>& list, const char delimiter = ',', const char groupDelimiter = '\''  );
// RAVEN END

	// char * methods to replace library functions
	static int			Length( const char *s );
	static char *		ToLower( char *s );
	static char *		ToUpper( char *s );
	static bool			IsNumeric( const char *s );
	static bool			HasLower( const char *s );
	static bool			HasUpper( const char *s );
// RAVEN BEGIN
// bdube: escape codes
	static int			IsEscape( const char *s, int* type = NULL );
	static int			LengthWithoutEscapes( const char *s );	
	static char *		RemoveEscapes( char *s, int escapes = S_ESCAPE_ALL );
// RAVEN END
	static int			Cmp( const char *s1, const char *s2 );
	static int			Cmpn( const char *s1, const char *s2, int n );
	static int			Icmp( const char *s1, const char *s2 );
	static int			Icmpn( const char *s1, const char *s2, int n );
// RAVEN BEGIN
// bdube: escapes
	static int			IcmpNoEscape( const char *s1, const char *s2 );
// RAVEN END
	static int			IcmpPath( const char *s1, const char *s2 );			// compares paths and makes sure folders come first
	static int			IcmpnPath( const char *s1, const char *s2, int n );	// compares paths and makes sure folders come first
	static void			Append( char *dest, int size, const char *src );
	static void			Copynz( char *dest, const char *src, int destsize );
	static int			snPrintf( char *dest, int size, const char *fmt, ... ) id_attribute((format(printf,3,4)));
	static int			vsnPrintf( char *dest, int size, const char *fmt, va_list argptr );
	static int			FindChar( const char *str, const char c, int start = 0, int end = -1 );
	static int			FindText( const char *str, const char *text, bool casesensitive = true, int start = 0, int end = -1 );
	static bool			Filter( const char *filter, const char *name, bool casesensitive );
	static void			StripMediaName( const char *name, idStr &mediaName );
	static bool			CheckExtension( const char *name, const char *ext );
	static const char *	FloatArrayToString( const float *array, const int length, const int precision );

	// hash keys
	static int			Hash( const char *string );
	static int			Hash( const char *string, int length );
	static int			IHash( const char *string );					// case insensitive
	static int			IHash( const char *string, int length );		// case insensitive

	// character methods
	static char			ToLower( byte c );
	static char			ToUpper( byte c );
	static bool			CharIsPrintable( byte c );
	static bool			CharIsLower( byte c );
	static bool			CharIsUpper( byte c );
	static bool			CharIsAlpha( byte c );
	static bool			CharIsNumeric( byte c );
	static bool			CharIsNewLine( byte c );
	static bool			CharIsTab( byte c );
	static int			ColorIndex( int c );
	static idVec4 &		ColorForIndex( int i );

	friend int			sprintf( idStr &dest, const char *fmt, ... );
	friend int			vsprintf( idStr &dest, const char *fmt, va_list ap );

	void				ReAllocate( int amount, bool keepold );				// reallocate string data buffer
	void				FreeData( void );									// free allocated string memory

						// format value in the given measurement with the best unit, returns the best unit
	int					BestUnit( const char *format, float value, Measure_t measure );
						// format value in the requested unit and measurement
	void				SetUnit( const char *format, float value, int unit, Measure_t measure );

	static void			InitMemory( void );
	static void			ShutdownMemory( void );
	static void			PurgeMemory( void );
	static void			ShowMemoryUsage_f( const idCmdArgs &args );

	int					DynamicMemoryUsed() const;
	static idStr		FormatNumber( int number );

	static void			Split( const char* source, idList<idStr>& list, const char delimiter = ',', const char groupDelimiter = '\''  );

	idStr				GetLastColorCode( void ) const;

protected:
	int					len;
	char *				data;
	int					alloced;
	char				baseBuffer[ STR_ALLOC_BASE ];

	void				Init( void );										// initialize string using base buffer
	void				EnsureAlloced( int amount, bool keepold = true );	// ensure string data buffer is large anough

// RAVEN BEGIN
public:
	static const bool	printableCharacter[256];
	static const char	upperCaseCharacter[256];
	static const char	lowerCaseCharacter[256];
// RAVEN END
};

char *					va( const char *fmt, ... ) id_attribute((format(printf,1,2)));

// RAVEN BEGIN
// abahr
char*					fe( int errorId );
// RAVEN END

ID_INLINE void idStr::EnsureAlloced( int amount, bool keepold ) {
	if ( amount > alloced ) {
		ReAllocate( amount, keepold );
	}
}

ID_INLINE void idStr::Init( void ) {
	len = 0;
	alloced = STR_ALLOC_BASE;
	data = baseBuffer;
	data[ 0 ] = '\0';
#ifdef ID_DEBUG_MEMORY
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
		l = strlen( text );
		EnsureAlloced( l + 1 );
		strcpy( data, text );
		len = l;
	}
}

ID_INLINE idStr::idStr( const char *text, int start, int end ) {
	int i;
	int l = strlen( text );

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
	FreeData();
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
	return idStr::Cmpn( data, text, strlen( text ) );
}

ID_INLINE int idStr::Icmp( const char *text ) const {
	assert( text );
	return idStr::Icmp( data, text );
}

ID_INLINE int idStr::Icmpn( const char *text, int n ) const {
	assert( text );
	return idStr::Icmpn( data, text, n );
}

ID_INLINE int idStr::IcmpPrefix( const char *text ) const {
	assert( text );
	return idStr::Icmpn( data, text, strlen( text ) );
}

// RAVEN BEGIN
// bdube: escapes
ID_INLINE int idStr::IcmpNoEscape( const char *text ) const {
	assert( text );
	return idStr::IcmpNoEscape( data, text );
}
// RAVEN END

ID_INLINE int idStr::IcmpPath( const char *text ) const {
	assert( text );
	return idStr::IcmpPath( data, text );
}

ID_INLINE int idStr::IcmpnPath( const char *text, int n ) const {
	assert( text );
	return idStr::IcmpnPath( data, text, n );
}

ID_INLINE int idStr::IcmpPrefixPath( const char *text ) const {
	assert( text );
	return idStr::IcmpnPath( data, text, strlen( text ) );
}

ID_INLINE int idStr::Length( void ) const {
	return len;
}

ID_INLINE int idStr::Allocated( void ) const {
	if ( data != baseBuffer ) {
		return alloced;
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
	return ( idStr::Cmp( data, "" ) == 0 );
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
		newLen = len + strlen( text );
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

	l = strlen( text );
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
// RAVEN BEGIN
		data[i] = ToLower( data[i] );
// RAVEN END
	}
}

ID_INLINE void idStr::ToUpper( void ) {
	for (int i = 0; data[i]; i++ ) {
// RAVEN BEGIN
		data[i] = ToUpper( data[i] );
// RAVEN END
	}
}

ID_INLINE bool idStr::IsNumeric( void ) const {
	return idStr::IsNumeric( data );
}

ID_INLINE bool idStr::HasLower( void ) const {
	return idStr::HasLower( data );
}

ID_INLINE bool idStr::HasUpper( void ) const {
	return idStr::HasUpper( data );
}

ID_INLINE int idStr::IsEscape( int* type ) const {
	return idStr::IsEscape( data, type );
}
ID_INLINE idStr &idStr::RemoveEscapes ( int escapes ) {
	idStr::RemoveEscapes( data, escapes );
	len = Length( c_str() );
	return *this;
}
ID_INLINE int idStr::LengthWithoutEscapes( void ) const {
	return idStr::LengthWithoutEscapes( data );
}
// RAVEN END

ID_INLINE void idStr::CapLength( int newlen ) {
	if ( len <= newlen ) {
		return;
	}
	data[ newlen ] = 0;
	len = newlen;
}

ID_INLINE void idStr::Fill( const char ch, int newlen ) {
	EnsureAlloced( newlen + 1 );
	len = newlen;
	memset( data, ch, len );
	data[ len ] = 0;
}

ID_INLINE int idStr::Find( const char c, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return idStr::FindChar( data, c, start, end );
}

ID_INLINE int idStr::Find( const char *text, bool casesensitive, int start, int end ) const {
	if ( end == -1 ) {
		end = len;
	}
	return idStr::FindText( data, text, casesensitive, start, end );
}

ID_INLINE bool idStr::Filter( const char *filter, bool casesensitive ) const {
	return idStr::Filter( filter, data, casesensitive );
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
	return idStr::CheckExtension( data, ext );
}

ID_INLINE int idStr::Length( const char *s ) {
	int i;
	for ( i = 0; s[i]; i++ ) {}
	return i;
}

ID_INLINE char *idStr::ToLower( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
// RAVEN BEGIN
		s[i] = ToLower( s[i] );
// RAVEN END
	}
	return s;
}

ID_INLINE char *idStr::ToUpper( char *s ) {
	for ( int i = 0; s[i]; i++ ) {
// RAVEN BEGIN
		s[i] = ToUpper( s[i] );
// RAVEN END
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

ID_INLINE char idStr::ToLower( byte c ) {
// RAVEN BEGIN
	if( lowerCaseCharacter[c] ) {
		return lowerCaseCharacter[c];
	}
// RAVEN END
	return c;
}

ID_INLINE char idStr::ToUpper( byte c ) {
// RAVEN BEGIN
	if( upperCaseCharacter[c] ) {
		return upperCaseCharacter[c];
	}
// RAVEN END
	return c;
}

ID_INLINE bool idStr::CharIsPrintable( byte c ) {
// RAVEN BEGIN
	return printableCharacter[c];
// RAVEN END
}

ID_INLINE bool idStr::CharIsLower( byte c ) {
// RAVEN BEGIN
	return !!lowerCaseCharacter[c] && ( lowerCaseCharacter[c] == c );
// RAVEN END
}

ID_INLINE bool idStr::CharIsUpper( byte c ) {
// RAVEN BEGIN
	return !!upperCaseCharacter[c] && ( upperCaseCharacter[c] == c );
// RAVEN END
}

ID_INLINE bool idStr::CharIsAlpha( byte c ) {
	// test for regular ascii and western European high-ascii chars
	return !!upperCaseCharacter[c];
}

ID_INLINE bool idStr::CharIsNumeric( byte c ) {
	return ( c <= '9' && c >= '0' );
}

ID_INLINE bool idStr::CharIsNewLine( byte c ) {
	return ( c == '\n' || c == '\r' || c == '\v' );
}

ID_INLINE bool idStr::CharIsTab( byte c ) {
	return ( c == '\t' );
}

ID_INLINE int idStr::ColorIndex( int c ) {
	return ( c & 15 );
}

ID_INLINE int idStr::DynamicMemoryUsed() const {
	return ( data == baseBuffer ) ? 0 : alloced;
}

#endif /* !__STR_H__ */
