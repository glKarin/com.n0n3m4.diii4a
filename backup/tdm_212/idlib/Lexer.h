/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

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
typedef enum {
	LEXFL_NOERRORS						= BIT(0),	// don't print any errors
	LEXFL_NOWARNINGS					= BIT(1),	// don't print any warnings
	LEXFL_NOFATALERRORS					= BIT(2),	// errors aren't fatal
	LEXFL_NOSTRINGCONCAT				= BIT(3),	// multiple strings seperated by whitespaces are not concatenated
	LEXFL_NOSTRINGESCAPECHARS			= BIT(4),	// no escape characters inside strings
	LEXFL_NODOLLARPRECOMPILE			= BIT(5),	// don't use the $ sign for precompilation
	LEXFL_NOBASEINCLUDES				= BIT(6),	// don't include files embraced with < >
	LEXFL_ALLOWPATHNAMES				= BIT(7),	// allow path seperators in names
	LEXFL_ALLOWNUMBERNAMES				= BIT(8),	// allow names to start with a number
	LEXFL_ALLOWIPADDRESSES				= BIT(9),	// allow ip addresses to be parsed as numbers
	LEXFL_ALLOWFLOATEXCEPTIONS			= BIT(10),	// allow float exceptions like 1.#INF or 1.#IND to be parsed
	LEXFL_ALLOWMULTICHARLITERALS		= BIT(11),	// allow multi character literals
	LEXFL_ALLOWBACKSLASHSTRINGCONCAT	= BIT(12),	// allow multiple strings seperated by '\' to be concatenated
	LEXFL_ONLYSTRINGS					= BIT(13)	// parse as whitespace deliminated strings (quoted strings keep quotes)
} lexerFlags_t;

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

#define P_POINTERREF				23
#define P_CPP1						24
#define P_CPP2						25
#define P_MUL						26
#define P_DIV						27
#define P_MOD						28
#define P_ADD						29
#define P_SUB						30
#define P_ASSIGN					31

#define P_BIN_AND					32
#define P_BIN_OR					33
#define P_BIN_XOR					34
#define P_BIN_NOT					35

#define P_LOGIC_NOT					36
#define P_LOGIC_GREATER				37
#define P_LOGIC_LESS				38

#define P_REF						39
#define P_COMMA						40
#define P_SEMICOLON					41
#define P_COLON						42
#define P_QUESTIONMARK				43

#define P_PARENTHESESOPEN			44
#define P_PARENTHESESCLOSE			45
#define P_BRACEOPEN					46
#define P_BRACECLOSE				47
#define P_SQBRACKETOPEN				48
#define P_SQBRACKETCLOSE			49
#define P_BACKSLASH					50

#define P_PRECOMP					51
#define P_DOLLAR					52

// punctuation
typedef struct punctuation_s
{
	const char *p;						// punctuation character(s)
	int n;							// punctuation id
} punctuation_t;

/// A lexer created by ID software.
/** This is a lexer for C-like languages. Whitespace and C-style comments are
 *  ignored. The input line is broken up into tokens which consist of string
 *  literals, numeric literals, identifiers and operators.
 *  
 *  String literals are of the form "blah blah blah" or 'c', just like in C.
 *  Double-quoted strings can be of any length, but single-quoted strings are
 *  expected to contain a single character. (escape sequences are considered
 *  to form a single character, so '\n' is a valid string literal) The same
 *  escape characters as C are allowed. Adjacent (ignoring whitespace) string
 *  literals are considered to represent one long string literal who's
 *  contents is the concatenation of the smaller strings. For example,
 *  "foo" "bar" is equivelant to "foobar", and will be returned as a single
 *  token.
 *  Lexer flags relavent to string literals:
 *  LEXFL_NOSTRINGCONCAT: Adjacent string literals aren't considered to form
 *    one long string. Instead, each string is considered its own token.
 *    This will cause "foo" "bar" to be treated as two tokens instead of one.
 *  LEXFL_ALLOWBACKSLASHSTRINGCONCAT: Backslashes may be used to concatenate
 *    string literals. (whitespace is ignored) For example "foo" \ "bar" is
 *    equivelant to "foobar". If LEXFL_NOSTRINGCONCAT is also turned on, then
 *    "foo" "bar" is treated as two tokens, but "foo" \ "bar" is treated as
 *    one.
 *  LEXFL_ALLOWMULTICHARLITERALS: Single-quoted string literals such as 'x'
 *    may to contain multiple characters. So 'blah' is considered a valid
 *    string literal. 'foo' 'bar' is equivelant to 'foobar' unless
 *    LEXFL_NOSTRINGCONCAT is turned on. LEXFL_ALLOWBACKSLASHSTRINGCONCAT
 *    does not have an effect on single-quoted string literals.
 *  LEXFL_NOSTRINGESCAPECHARS: Escape characters aren't allowed in strings.
 *    Instead, backslashes are treated as the contents of the string.
 *    For example, "\\" would be a string of two characters, not one.
 *    Because of this, '\n' would no longer be a valid string literal,
 *    unless LEXFL_ALLOWMULTICHARLITERALS is turned on.
 *  
 *  I haven't learned much about numeric literals. As such, this section is a
 *  stub.
 *  Lexer flags relavent to numeric literals:
 *  LEXFL_ALLOWIPADDRESSES: allow ip addresses to be parsed as numbers
 *  LEXFL_ALLOWFLOATEXCEPTIONS: allow float exceptions like 1.#INF or 1.#IND to be parsed
 *  
 *  Identifiers (names) are the same as in C. They may contain letters,
 *  numbers and underscores, but may not start with numbers.
 *  Lexer flags relavent to identifiers:
 *  LEXFL_ALLOWPATHNAMES: Identifiers are also allowed to have slashes,
 *  backslashes, colons and periods in them.
 *  LEXFL_ALLOWNUMBERNAMES: An identifier may start with a numeric literal.
 *  LEXFL_ONLYSTRINGS: Tokens are only returned as strings and identifiers.
 *  Identifiers may contain dashes in them. I beleive this flag is buggy,
 *  since it looks like +blah is considered a single token, but blah+ is
 *  considered two.
 *  
 *  Generic lexer flags:
 *  LEXFL_NOERRORS: Errors are disabled.
 *  LEXFL_NOWARNINGS: Warnings are disabled.
 *  LEXFL_NOFATALERRORS: Errors are converted to warnings. This flag is
 *    redundant if LEXFL_NOERRORS is turned on. Even if LEXFL_NOWARNINGS is
 *    turned on, warnings that were converted from errors will not be ignored.
 *  
 *  Lexer flags that are used by the parser instead of the lexer:
 *  LEXFL_NODOLLARPRECOMPILE: don't use the $ sign for precompilation
 *  LEXFL_NOBASEINCLUDES: don't include files embraced with < >
 *  
 *  Other notes: UnreadToken() appears to be incompatible with many other
 *    functions, such as the Check or Peek functions. You should Either use
 *    UnreadToken() or the Peek/Check functions but not both.
 *
 */
class idLexer {

	friend class idParser;

public:
					// constructor
					idLexer();
					idLexer( int flags );
					idLexer( const char *filename, int flags = 0, bool OSPath = false );
					idLexer( const char *ptr, int length, const char *name, int flags = 0 );
					// destructor
					~idLexer();
					// load a script from the given file at the given offset with the given length
	int				LoadFile( const char *filename, bool OSPath = false );
					// load a script from the given memory with the given length and a specified line offset,
					// so source strings extracted from a file can still refer to proper line numbers in the file
					// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	int				LoadMemory( const char *ptr, int length, const char *name, int startLine = 1 );
					// stgatilov: if called immediately after LoadMemory, then ownership over "ptr" is passed to this lexer
					// (by default lexer does not delete memory passed to LoadMemory)
	void			OwnLoadedMemory();
					// free the script
	void			FreeSource( void );
					// returns true if a script is loaded
	int				IsLoaded( void ) { return idLexer::loaded; };
					// read a token
	int				ReadToken( idToken *token );
					// expect a certain token, reads the token when available
	int				ExpectTokenString( const char *string );
					// expect a certain token type
	int				ExpectTokenType( int type, int subtype, idToken *token );
					// expect a token
	int				ExpectAnyToken( idToken *token );
					// returns true when the token is available
	int				CheckTokenString( const char *string );
					// returns true an reads the token when a token with the given type is available
	int				CheckTokenType( int type, int subtype, idToken *token );
					// returns true if the next token equals the given string but does not remove the token from the source
	int				PeekTokenString( const char *string );
					// returns true if the next token equals the given type but does not remove the token from the source
	int				PeekTokenType( int type, int subtype, idToken *token );
					// skip tokens until the given token string is read
	int				SkipUntilString( const char *string );
					// skip the rest of the current line
	int				SkipRestOfLine( void );
					// skip the braced section
	int				SkipBracedSection( bool parseFirstBrace = true );
					// unread the given token
	void			UnreadToken( const idToken *token );
					// read a token only if on the same line
	int				ReadTokenOnLine( idToken *token );

					//Returns the rest of the current line
	const char*		ReadRestOfLine(idStr& out);

					// read a signed integer
	int				ParseInt( void );
					// read a boolean
	bool			ParseBool( void );
					// read a floating point number.  If errorFlag is NULL, a non-numeric token will
					// issue an Error().  If it isn't NULL, it will issue a Warning() and set *errorFlag = true
	float			ParseFloat( bool *errorFlag = NULL );
					/**
					* Parse a 1d float matrix of length x and store it in m.  
					* If bIntsOnly is TRUE, a non-integer token will issue an Error().
					**/
	int				Parse1DMatrix( int x, float *m, bool bIntsOnly = false );
					/**
					* Parse 1d integer matrix by overloading parse1DMatrix
					**/
	int				Parse1DMatrix( int x, int *m );
	int				Parse2DMatrix( int y, int x, float *m );
	int				Parse3DMatrix( int z, int y, int x, float *m );
					// parse a braced section into a string
	const char *	ParseBracedSection( idStr &out );
					// parse a braced section into a string, maintaining indents and newlines
	const char *	ParseBracedSectionExact ( idStr &out, int tabs = -1 );
					// parse the rest of the line
	const char *	ParseRestOfLine( idStr &out );
					// retrieves the white space characters before the last read token
	int				GetLastWhiteSpace( idStr &whiteSpace ) const;
					// returns start index into text buffer of last white space
	int				GetLastWhiteSpaceStart( void ) const;
					// returns end index into text buffer of last white space
	int				GetLastWhiteSpaceEnd( void ) const;
					// set an array with punctuations, NULL restores default C/C++ set, see default_punctuations for an example
	void			SetPunctuations( const punctuation_t *p );
					// returns a pointer to the punctuation with the given id
	const char *	GetPunctuationFromId( int id );
					// get the id for the given punctuation
	int				GetPunctuationId( const char *p );
					// set lexer flags
	void			SetFlags( int flags );
					// get lexer flags
	int				GetFlags( void );
					// reset the lexer
	void			Reset( void );
					// returns true if at the end of the file
	int				EndOfFile( void );
					// returns the current filename
	const char *	GetFileName( void );
	const char *	GetDisplayFileName( void );
					// get offset in script
	const int		GetFileOffset( void );
					// get file time
	const ID_TIME_T GetFileTime( void );
					// returns the current line number
	const int		GetLineNum( void );
					// print an error message
	void			Error( const char *str, ... ) id_attribute((format(printf,2,3)));

					// print a warning message

	void			Warning( const char *str, ... ) id_attribute((format(printf,2,3)));

					// returns true if Error() was called with LEXFL_NOFATALERRORS or LEXFL_NOERRORS set
	bool			HadError( void ) const;

					// set the base folder to load files from
	static void		SetBaseFolder( const char *path );

private:
	int				loaded;					// set when a script file is loaded from file or memory
	idStr			displayFilename;		// shortened file path for printing warnings
	idStr			filename;				// file path of the script (absolute)
	int				allocated;				// true if buffer memory was allocated
	const char *	buffer;					// buffer containing the script
	const char *	script_p;				// current pointer in the script
	const char *	end_p;					// pointer to the end of the script
	const char *	lastScript_p;			// script pointer before reading token
	const char *	whiteSpaceStart_p;		// start of last white space
	const char *	whiteSpaceEnd_p;		// end of last white space
	unsigned int	fileTime;				// file time
	int				length;					// length of the script in bytes
	int				line;					// current line in script
	int				lastline;				// line before reading token
	int				tokenavailable;			// set by unreadToken
	int				flags;					// several script flags
	const punctuation_t *punctuations;		// the punctuations used in the script
	int *			punctuationtable;		// ASCII table with punctuations
	int *			nextpunctuation;		// next punctuation in chain
	idToken			token;					// available token
	idLexer *		next;					// next script in a chain
	bool			hadError;				// set by idLexer::Error, even if the error is supressed

	static char		baseFolder[ 256 ];		// base folder to load files from

private:
	void			CreatePunctuationTable( const punctuation_t *punctuations );
	int				ReadWhiteSpace( void );
	int				ReadEscapeCharacter( char *ch );
	int				ReadString( idToken *token, int quote );
	int				ReadName( idToken *token );
	int				ReadNumber( idToken *token );
	int				ReadPunctuation( idToken *token );
	int				ReadPrimitive( idToken *token );
	int				CheckString( const char *str ) const;
	int				NumLinesCrossed( void );
};

ID_INLINE const char *idLexer::GetFileName( void ) {
	return idLexer::filename;
}

ID_INLINE const char *idLexer::GetDisplayFileName( void ) {
	return idLexer::displayFilename;
}

ID_INLINE const int idLexer::GetFileOffset( void ) {
	return idLexer::script_p - idLexer::buffer;
}

ID_INLINE const ID_TIME_T idLexer::GetFileTime( void ) {
	return idLexer::fileTime;
}

ID_INLINE const int idLexer::GetLineNum( void ) {
	return idLexer::line;
}

ID_INLINE void idLexer::SetFlags( int flags ) {
	idLexer::flags = flags;
}

ID_INLINE int idLexer::GetFlags( void ) {
	return idLexer::flags;
}

#endif /* !__LEXER_H__ */

