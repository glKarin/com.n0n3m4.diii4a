// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __LEXER_H__
#define __LEXER_H__

/*
===============================================================================

	Lexicographical parser

	Does not use memory allocation during parsing. The lexer uses no
	memory allocation if a source is loaded with LoadMemory().
	However, idToken may still allocate memory for large strings.
	
	A number directly following the escape character '\' in a string is
	assumed to be in decimal format instead of octal. Binary numbers of
	the form 0b.. or 0B.. can also be used.

===============================================================================
*/

// lexer flags
enum lexerFlags_t {
	LEXFL_NOERRORS						= BIT(0),	// don't print any errors
	LEXFL_NOWARNINGS					= BIT(1),	// don't print any warnings
	LEXFL_NOFATALERRORS					= BIT(2),	// errors aren't fatal
	LEXFL_VCSTYLEREPORTS				= BIT(3),	// warnings and errors are reported in M$ VC style
	LEXFL_NOSTRINGCONCAT				= BIT(4),	// multiple strings separated by whitespaces are not concatenated
	LEXFL_NOSTRINGESCAPECHARS			= BIT(5),	// no escape characters inside strings
	LEXFL_NODOLLARPRECOMPILE			= BIT(6),	// don't use the $ sign for precompilation
	LEXFL_NOBASEINCLUDES				= BIT(7),	// don't include files embraced with < >
	LEXFL_ALLOWPATHNAMES				= BIT(8),	// allow path separators in names
	LEXFL_ALLOWNUMBERNAMES				= BIT(9),	// allow names to start with a number
	LEXFL_ALLOWIPADDRESSES				= BIT(10),	// allow ip addresses to be parsed as numbers
	LEXFL_ALLOWFLOATEXCEPTIONS			= BIT(11),	// allow float exceptions like 1.#INF or 1.#IND to be parsed
	LEXFL_ALLOWMULTICHARLITERALS		= BIT(12),	// allow multi character literals
	LEXFL_ALLOWBACKSLASHSTRINGCONCAT	= BIT(13),	// allow multiple strings seperated by '\' to be concatenated
	LEXFL_ONLYSTRINGS					= BIT(14),	// parse as whitespace deliminated strings (quoted strings keep quotes)
	LEXFL_NOEMITSTRINGESCAPECHARS		= BIT(15),	// no escape characters inside strings
	LEXFL_ALLOWRAWSTRINGBLOCKS			= BIT(16),	// allow raw text blocks embraced with <% %>	
};

// punctuation ids
#define P_RSHIFT_ASSIGN				1
#define P_LSHIFT_ASSIGN				2
#define P_PARMS						3
#define P_PRECOMPMERGE				4

#define P_LOGIC_AND					5
#define P_LOGIC_OR					6
#define P_LOGIC_GEQ					7
#define P_LOGIC_LEQ					8
#define P_LOGIC_EQ					9
#define P_LOGIC_UNEQ				10

#define P_MUL_ASSIGN				11
#define P_DIV_ASSIGN				12
#define P_MOD_ASSIGN				13
#define P_ADD_ASSIGN				14
#define P_SUB_ASSIGN				15
#define P_INC						16
#define P_DEC						17

#define P_BIN_AND_ASSIGN			18
#define P_BIN_OR_ASSIGN				19
#define P_BIN_XOR_ASSIGN			20
#define P_RSHIFT					21
#define P_LSHIFT					22

#define P_SCOPE_RESOLUTION			23
#define P_MEMBER_SELECTION_OBJECT	24
#define P_MEMBER_SELECTION_POINTER	25
#define P_POINTER_TO_MEMBER_OBJECT	26
#define P_POINTER_TO_MEMBER_POINTER	27

#define P_MUL						28
#define P_DIV						29
#define P_MOD						30
#define P_ADD						31
#define P_SUB						32
#define P_ASSIGN					33

#define P_BIN_AND					34
#define P_BIN_OR					35
#define P_BIN_XOR					36
#define P_BIN_NOT					37

#define P_LOGIC_NOT					38
#define P_LOGIC_GREATER				39
#define P_LOGIC_LESS				40

#define P_COMMA						41
#define P_SEMICOLON					42
#define P_COLON						43
#define P_QUESTIONMARK				44

#define P_PARENTHESESOPEN			45
#define P_PARENTHESESCLOSE			46
#define P_BRACEOPEN					47
#define P_BRACECLOSE				48
#define P_SQBRACKETOPEN				49
#define P_SQBRACKETCLOSE			50
#define P_BACKSLASH					51

#define P_PRECOMP					52
#define P_DOLLAR					53

// punctuation
struct punctuation_t {
	char *p;						// punctuation character(s)
	int n;							// punctuation id
};


class idLexer {

	friend class idParser;

public:
							// constructor
							idLexer();
							idLexer( int flags );
							idLexer( const char *filename, int flags = 0, bool OSPath = false, int startLine = 1 );
							idLexer( const char *ptr, int length, const char *name, int flags = 0, int startLine = 1 );
							// destructor
							~idLexer();
							// load a script from the given file at the given offset with the given length
	bool					LoadFile( const char *filename, bool OSPath = false, int startLine = 1 );
							// load a script from the given memory with the given length and a specified line offset,
							// so source strings extracted from a file can still refer to proper line numbers in the file
							// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	bool					LoadMemory( const char *ptr, int length, const char *name, int startLine = 1 );
							// Load a binary token table and indices as a token stream
	bool					LoadMemoryBinary( const byte* ptr, int length, const char *name, idTokenCache* globals = NULL );
	bool					LoadTokenStream( const idList<unsigned short>& indices, const idTokenCache& tokens, const char* name );
							

	idLexerBinary&			GetBinary() ;
	const idLexerBinary&	GetBinary() const;

							// free the script
	void					FreeSource( void );
							// returns true if a script is loaded
	bool					IsLoaded( void ) const { return loaded; }
							// read a token
	int						ReadToken( idToken *token );
							// expect a certain token, reads the token when available
	bool					ExpectTokenString( const char *string );
							// expect a certain token type
	bool					ExpectTokenType( int type, int subtype, idToken *token );
							// expect a token
	bool					ExpectAnyToken( idToken *token );
							// returns true when the token is available
	bool					CheckTokenString( const char *string );
							// returns true an reads the token when a token with the given type is available
	bool					CheckTokenType( int type, int subtype, idToken *token );
							// returns true if the next token equals the given string but does not remove the token from the source
	int						PeekTokenString( const char *string );
							// returns true if the next token equals the given type but does not remove the token from the source
	int						PeekTokenType( int type, int subtype, idToken *token );
							// skip tokens until the given token string is read
	bool					SkipUntilString( const char *string, idToken* token = NULL );
							// skip the rest of the current line
	int						SkipRestOfLine( void );
							// skip the braced section
	int						SkipBracedSection( bool parseFirstBrace = true );
							// skip the braced section, maintaining indents and newlines
	bool					SkipBracedSectionExact( int tabs = -1, bool parseFirstBrace = true );
							// skips spaces, tabs, C-like comments etc.
	int						SkipWhiteSpace( bool currentLine );
							// unread the given token
	void					UnreadToken( const idToken *token );
							// read a token only if on the same line
	int						ReadTokenOnLine( idToken *token );
							// read a signed integer
	int						ParseInt( void );
							// read a boolean
	bool					ParseBool( void );
							// read a floating point number.  If errorFlag is NULL, a non-numeric token will
							// issue an Error().  If it isn't NULL, it will issue a Warning() and set *errorFlag = true
	float					ParseFloat( bool *errorFlag = NULL );
							// parse matrices with floats
	int						Parse1DMatrix( int x, float *m, bool expectCommas = false );
	int						Parse2DMatrix( int y, int x, float *m );
	int						Parse3DMatrix( int z, int y, int x, float *m );
							// parse a braced section into a string
	const char *			ParseBracedSection( idStr &out, int tabs = -1, bool parseFirstBrace = true, char intro = '{', char outro = '}' );
							// parse a braced section into a string, maintaining indents and newlines
	bool					ParseBracedSectionExact( idStr &out, int tabs = -1, bool parseFirstBrace = true );
							// parse the rest of the line
	const char *			ParseRestOfLine( idStr &out );
							// pulls the entire line, including the \n at the end
	const char *			ParseCompleteLine( idStr &out );
							// retrieves the white space after the last read token
	int						GetNextWhiteSpace( idStr &whiteSpace, bool currentLine );
							// retrieves the white space characters before the last read token
	int						GetLastWhiteSpace( idStr &whiteSpace ) const;
							// returns start index into text buffer of last white space
	int						GetLastWhiteSpaceStart( void ) const;
							// returns end index into text buffer of last white space
	int						GetLastWhiteSpaceEnd( void ) const;
							// set an array with punctuations, NULL restores default C/C++ set, see default_punctuations for an example
	void					SetPunctuations( const punctuation_t *p );
							// returns a pointer to the punctuation with the given id
	const char *			GetPunctuationFromId( int id ) const;
							// get the id for the given punctuation
	int						GetPunctuationId( const char *p ) const;
							// set lexer flags
	void					SetFlags( int flags );
							// get lexer flags
	int						GetFlags( void ) const;
							// reset the lexer
	void					Reset( void );
							// returns true if at the end of the file
	int						EndOfFile( void ) const;
							// returns the current filename
	const char *			GetFileName( void ) const;
							// get offset in script
	int						GetFileOffset( void ) const;
							// get offset in script
	int						GetLastFileOffset( void ) const;
							// get total size of script
	int						GetFileSize( void ) const;
							// get file time
	unsigned int			GetFileTime( void ) const;
							// returns the current line number
	int						GetLineNum( void ) const;
							// print an error message
	void					Error( const char *str, ... );
							// print a warning message
	void					Warning( const char *str, ... );
							// returns true if Error() was called with LEXFL_NOFATALERRORS or LEXFL_NOERRORS set
	bool					HadError( void ) const;
							// returns true if any warnings were printed
	bool					HadWarning( void ) const;

							// set the base folder to load files from
	static void				SetBaseFolder( const char *path );

private:
							idLexer( const idLexer& rhs );
private:	

	bool					loaded;					// set when a script file is loaded from file or memory
	idStr					filename;				// file name of the script
	int						allocated;				// true if buffer memory was allocated
	const char *			buffer;					// buffer containing the script
	const char *			script_p;				// current pointer in the script
	const char *			end_p;					// pointer to the end of the script
	const char *			lastScript_p;			// script pointer before reading token
	const char *			whiteSpaceStart_p;		// start of last white space
	const char *			whiteSpaceEnd_p;		// end of last white space
	unsigned int			fileTime;				// file time
	int						length;					// length of the script in bytes
	int						line;					// current line in script
	int						lastline;				// line before reading token
	int						flags;					// several script flags
	const punctuation_t		*punctuations;		// the punctuations used in the script
	int *					punctuationtable;		// ASCII table with punctuations
	int *					nextpunctuation;		// next punctuation in chain
	idList< idToken >		tokens;				// available token
	idLexer *				next;					// next script in a chain
	bool					hadError;				// set by idLexer::Error, even if the error is suppressed
	bool					hadWarning;				// set by idLexer::Warning, even if the warning is suppressed

	idLexerBinary			binary;

	static char				baseFolder[ 256 ];		// base folder to load files from

private:
	void					CreatePunctuationTable( const punctuation_t *punctuations );
	int						ReadWhiteSpace( void );
	int						ReadEscapeCharacter( char *ch );
	int						ReadString( idToken *token, int quote );
	int						ReadName( idToken *token );
	int						ReadNumber( idToken *token );
	int						ReadRawStringBlock( idToken *token );
	int						ReadPunctuation( idToken *token );
	int						CheckString( const char *str ) const;
	int						NumLinesCrossed( void ) const;
};

ID_INLINE const char *idLexer::GetFileName( void ) const {
	return this->filename;
}

ID_INLINE int idLexer::GetFileOffset( void ) const {
	return this->script_p - this->buffer;
}

ID_INLINE int idLexer::GetLastFileOffset( void ) const {
	return this->lastScript_p - this->buffer;
}

ID_INLINE int idLexer::GetFileSize( void ) const {
	return this->end_p - this->buffer;
}

ID_INLINE unsigned int idLexer::GetFileTime( void ) const {
	return this->fileTime;
}

ID_INLINE int idLexer::GetLineNum( void ) const {
	return this->line;
}

ID_INLINE void idLexer::SetFlags( int flags ) {
	assert( !loaded );		// all flags must be set before loading the file
	this->flags = flags;
}

ID_INLINE int idLexer::GetFlags( void ) const {
	return this->flags;
}

ID_INLINE idLexerBinary& idLexer::GetBinary() {
	return binary;
}

ID_INLINE const idLexerBinary& idLexer::GetBinary() const {
	return binary;
}

#endif /* !__LEXER_H__ */

