/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

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
#ifdef _RAVEN
// RAVEN BEGIN
// jsinger: added to support binary writing from the lexer in either byte swapped or non-byte swapped formats
	,
	LEXFL_READBINARY					= BIT(29),	// read a byte code compiled version of the file
	LEXFL_BYTESWAP						= BIT(30),	// when writing the byte code compiled file, byte swap numbers
	LEXFL_WRITEBINARY					= BIT(31)	// write out a byte code compiled version of the file
// RAVEN END
#endif
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
#ifdef _RAVEN
// RAVEN BEGIN
#define P_INVERTED_PLING			53
#define P_INVERTED_QUERY			54
 // RAVEN END
#endif


// punctuation
typedef struct punctuation_s {
	const char *p;						// punctuation character(s)
	int n;							// punctuation id
} punctuation_t;


class idLexer
{

		friend class idParser;

	public:
		// constructor
		idLexer();
		idLexer(int flags);
		idLexer(const char *filename, int flags = 0, bool OSPath = false);
		idLexer(const char *ptr, int length, const char *name, int flags = 0);
		// destructor
		~idLexer();
		// load a script from the given file at the given offset with the given length
		int				LoadFile(const char *filename, bool OSPath = false);
		// load a script from the given memory with the given length and a specified line offset,
		// so source strings extracted from a file can still refer to proper line numbers in the file
		// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
		int				LoadMemory(const char *ptr, int length, const char *name, int startLine = 1);
		// free the script
		void			FreeSource(void);
		// returns true if a script is loaded
		int				IsLoaded(void) {
			return idLexer::loaded;
		};
		// read a token
		int				ReadToken(idToken *token);
		// expect a certain token, reads the token when available
		int				ExpectTokenString(const char *string);
		// expect a certain token type
		int				ExpectTokenType(int type, int subtype, idToken *token);
		// expect a token
		int				ExpectAnyToken(idToken *token);
		// returns true when the token is available
		int				CheckTokenString(const char *string);
		// returns true an reads the token when a token with the given type is available
		int				CheckTokenType(int type, int subtype, idToken *token);
		// returns true if the next token equals the given string but does not remove the token from the source
		int				PeekTokenString(const char *string);
		// returns true if the next token equals the given type but does not remove the token from the source
		int				PeekTokenType(int type, int subtype, idToken *token);
		// skip tokens until the given token string is read
		int				SkipUntilString(const char *string);
		// skip the rest of the current line
		int				SkipRestOfLine(void);
		// skip the braced section
		int				SkipBracedSection(bool parseFirstBrace = true);
		// unread the given token
		void			UnreadToken(const idToken *token);
		// read a token only if on the same line
		int				ReadTokenOnLine(idToken *token);

		//Returns the rest of the current line
		const char		*ReadRestOfLine(idStr &out);

		// read a signed integer
		int				ParseInt(void);
		// read a boolean
		bool			ParseBool(void);
		// read a floating point number.  If errorFlag is NULL, a non-numeric token will
		// issue an Error().  If it isn't NULL, it will issue a Warning() and set *errorFlag = true
		float			ParseFloat(bool *errorFlag = NULL);
		// parse matrices with floats
		int				Parse1DMatrix(int x, float *m);
		int				Parse2DMatrix(int y, int x, float *m);
		int				Parse3DMatrix(int z, int y, int x, float *m);
		// parse a braced section into a string
		const char 	*ParseBracedSection(idStr &out);
		// parse a braced section into a string, maintaining indents and newlines
		const char 	*ParseBracedSectionExact(idStr &out, int tabs = -1);
		// parse the rest of the line
		const char 	*ParseRestOfLine(idStr &out);
		// retrieves the white space characters before the last read token
		int				GetLastWhiteSpace(idStr &whiteSpace) const;
		// returns start index into text buffer of last white space
		int				GetLastWhiteSpaceStart(void) const;
		// returns end index into text buffer of last white space
		int				GetLastWhiteSpaceEnd(void) const;
		// set an array with punctuations, NULL restores default C/C++ set, see default_punctuations for an example
		void			SetPunctuations(const punctuation_t *p);
		// returns a pointer to the punctuation with the given id
		const char 	*GetPunctuationFromId(int id);
		// get the id for the given punctuation
		int				GetPunctuationId(const char *p);
		// set lexer flags
		void			SetFlags(int flags);
		// get lexer flags
		int				GetFlags(void);
		// reset the lexer
		void			Reset(void);
		// returns true if at the end of the file
		int				EndOfFile(void);
		// returns the current filename
		const char 	*GetFileName(void);
		// get offset in script
		const int		GetFileOffset(void);
		// get file time
		const ID_TIME_T	GetFileTime(void);
		// returns the current line number
		const int		GetLineNum(void);
		// print an error message
		void			Error(const char *str, ...) id_attribute((format(printf,2,3)));
		// print a warning message
		void			Warning(const char *str, ...) id_attribute((format(printf,2,3)));
		// returns true if Error() was called with LEXFL_NOFATALERRORS or LEXFL_NOERRORS set
		bool			HadError(void) const;

		// set the base folder to load files from
		static void		SetBaseFolder(const char *path);
#ifdef _RAVEN
// RAVEN BEGIN
// rjohnson: added vertex color support to proc files.  assume a default RGBA of 0x000000ff
	int				Parse1DMatrixOpenEnded( int MaxCount, float *m );
// RAVEN END

// RAVEN BEGIN
					// write a binary representation of a token
	void			WriteBinaryToken(idToken *tok);
// RAVEN END

// RAVEN BEGIN
// dluetscher: added method to parse a structure array that is made up of numerics (floats, ints), and stores them in the given storage
	void			ParseNumericStructArray( int numStructElements, int tokenSubTypeStructElements[], int arrayCount, byte *arrayStorage );
// RAVEN END
#endif

	private:
		int				loaded;					// set when a script file is loaded from file or memory
		idStr			filename;				// file name of the script
		int				allocated;				// true if buffer memory was allocated
		const char 	*buffer;					// buffer containing the script
		const char 	*script_p;				// current pointer in the script
		const char 	*end_p;					// pointer to the end of the script
		const char 	*lastScript_p;			// script pointer before reading token
		const char 	*whiteSpaceStart_p;		// start of last white space
		const char 	*whiteSpaceEnd_p;		// end of last white space
		ID_TIME_T			fileTime;				// file time
		int				length;					// length of the script in bytes
		int				line;					// current line in script
		int				lastline;				// line before reading token
		int				tokenavailable;			// set by unreadToken
		int				flags;					// several script flags
		const punctuation_t *punctuations;		// the punctuations used in the script
		int 			*punctuationtable;		// ASCII table with punctuations
		int 			*nextpunctuation;		// next punctuation in chain
		idToken			token;					// available token
		idLexer 		*next;					// next script in a chain
		bool			hadError;				// set by idLexer::Error, even if the error is supressed

		static char		baseFolder[ 256 ];		// base folder to load files from

	private:
		void			CreatePunctuationTable(const punctuation_t *punctuations);
		int				ReadWhiteSpace(void);
		int				ReadEscapeCharacter(char *ch);
		int				ReadString(idToken *token, int quote);
		int				ReadName(idToken *token);
		int				ReadNumber(idToken *token);
		int				ReadPunctuation(idToken *token);
		int				ReadPrimitive(idToken *token);
		int				CheckString(const char *str) const;
		int				NumLinesCrossed(void);

#ifdef _RAVEN
// RAVEN BEGIN
// jsinger: This is the file that WriteBinaryToken will write to when the proper flags are set
	idFile			*mBinaryFile;
// RAVEN END
#endif
};

ID_INLINE const char *idLexer::GetFileName(void)
{
	return idLexer::filename;
}

ID_INLINE const int idLexer::GetFileOffset(void)
{
	return idLexer::script_p - idLexer::buffer;
}

ID_INLINE const ID_TIME_T idLexer::GetFileTime(void)
{
	return idLexer::fileTime;
}

ID_INLINE const int idLexer::GetLineNum(void)
{
	return idLexer::line;
}

ID_INLINE void idLexer::SetFlags(int flags)
{
	idLexer::flags = flags;
}

ID_INLINE int idLexer::GetFlags(void)
{
	return idLexer::flags;
}

#ifdef _RAVEN
// RAVEN BEGIN
// jsinger: defines used by the binary lexer and WriteBinaryToken method of idLexer

// binary token types
// low order 3 bits form the type
#define BTT_STRING					0
#define BTT_LITERAL					1
#define BTT_NUMBER					2
#define BTT_NAME					3
#define BTT_PUNCTUATION				4
#define BTT_PUNCTUATION2			5		// when punctuation, least significant bit is used as part of the punctuation type
// 3 bits
#define BTT_SUBTYPE_INT				0
#define BTT_SUBTYPE_UNSIGNEDINT		1
#define BTT_SUBTYPE_LONG			2
#define BTT_SUBTYPE_UNSIGNEDLONG	3
#define BTT_SUBTYPE_FLOAT			4
#define BTT_SUBTYPE_DOUBLE			5
#define BTT_SUBTYPE_IPADDRESS		6
#define BTT_SUBTYPE_IPPORT			7

// 2 bits
#define BTT_STORED_1BYTE			0
#define BTT_STORED_2BYTE			1
#define BTT_STORED_4BYTE			2
#define BTT_STORED_8BYTE			3

// retrieval macros
#define BTT_GET_TYPE(x)				((x) & 0x07)
#define BTT_GET_SUBTYPE(x)			(((x) & 0x38)>>3)
#define BTT_GET_STORED_SIZE(x)		(((x) & 0xC0)>>6)
#define BTT_GET_STRING_LENGTH(x)	(((x) & 0xFC)>>3)
#define BTT_GET_PUNCTUATION(x)		(((x)&1)|(((x)>>2)&~1))

// punctuation types
#define BTT_PUNC_ASNULLTERMINATED	0
#define BTT_PUNC_RIGHTPAREN			1
#define BTT_PUNC_LEFTBRACE			2
#define BTT_PUNC_RIGHTBRACE			3
#define BTT_PUNC_MINUS				4
#define BTT_PUNC_PLUS				5
#define BTT_PUNC_COMMA				6
#define BTT_PUNC_PLUSPLUS			7
#define BTT_PUNC_LEFTBRACKET		8
#define BTT_PUNC_RIGHTBRACKET		9
#define BTT_PUNC_EQUAL				10
#define BTT_PUNC_EQUALEQUAL			11
#define BTT_PUNC_NOTEQUAL			12
#define BTT_PUNC_PERCENT			13
#define BTT_PUNC_LESSTHAN			14
#define BTT_PUNC_GREATERTHAN		15
#define BTT_PUNC_LOGICALAND			16
#define BTT_PUNC_AMPERSAND			17
#define BTT_PUNC_MINUSMINUS			18
#define BTT_PUNC_HASH				19
#define BTT_PUNC_LESSOREQUAL		20
#define BTT_PUNC_GREATEROREQUAL		21
#define BTT_PUNC_FORWARDSLASH		22
#define BTT_PUNC_SHIFTLEFT			23
#define BTT_PUNC_SHIFTRIGHT			24
#define BTT_PUNC_LEFTPAREN			25
#define BTT_PUNC_SEMICOLON			26
#define BTT_PUNC_ASTERISK			27
#define BTT_PUNC_PERIOD				28
#define BTT_PUNC_DOLLARSIGN			29
#define BTT_PUNC_PLUSEQUAL			30
#define BTT_PUNC_MINUSEQUAL			31
#define BTT_PUNC_TILDE				32
#define BTT_PUNC_EXCLAMATION		33
#define BTT_PUNC_PIPE				34
#define BTT_PUNC_BACKSLASH			35
#define BTT_PUNC_DOUBLEHASH			36
#define BTT_PUNC_DOUBLECOLON		37
#define BTT_PUNC_TIMESEQUAL			38
#define BTT_PUNC_DOUBLEPIPE			39
#define BTT_PUNC_INVERTEDPLING		40
#define BTT_PUNC_INVERTEDQUERY		41

// set macro
#define BTT_MAKENUMBER_PREFIX(type, subtype, storedsize) ((unsigned char)((type)|(subtype<<3)|(storedsize<<6)))
#define BTT_MAKESTRING_PREFIX(type, length) ((unsigned char)((type)|((((length)<32)?(length):0)<<3)))
#define BTT_MAKEPUNCTUATION_PREFIX(punc) (((punc<<2)&~7)|(punc&1))|BTT_PUNCTUATION
// RAVEN END

// this class is only necessary to parse binary files
class Lexer {
	friend class idParser;

public:

#ifdef LEXER_READ_AHEAD	
	static void				BeginLevelLoad();
	static void				EndLevelLoad();
#endif

							// constructors
							Lexer( const char *filename, int flags = 0, bool OSPath = false );
							Lexer(int flags=0);
							Lexer( char const * const ptr, int length, char const * const name, int flags = 0);

							//destructor
							~Lexer();
							// load a script from the given file at the given offset with the given length
	int						LoadFile( const char *filename, bool OSPath = false );
							// load a script from the given memory with the given length and a specified line offset,
							// so source strings extracted from a file can still refer to proper line numbers in the file
							// NOTE: the buffer is expect to be a valid C string: ptr[length] == '\0'
	int						LoadMemory( const char *ptr, int length, const char *name, int startLine = 1 );
							// free the script
	void					FreeSource( void );
							// returns true if a script is loaded
	int						IsLoaded( void );
							// read a token
	int						ReadToken( idToken *token );
							// expect a certain token, reads the token when available
	int						ExpectTokenString( const char *string );
							// expect a certain token type
	int						ExpectTokenType( int type, int subtype, idToken *token );
							// expect a token
	int						ExpectAnyToken( idToken *token );
							// returns true when the token is available
	int						CheckTokenString( const char *string );
							// returns true an reads the token when a token with the given type is available
	int						CheckTokenType( int type, int subtype, idToken *token );
							// skip tokens until the given token string is read
	int						SkipUntilString( const char *string );
							// skip the rest of the current line
	int						SkipRestOfLine( void );
							// skip the braced section
	int						SkipBracedSection( bool parseFirstBrace = true );
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
	int						Parse1DMatrix( int x, float *m );
	int						Parse1DMatrixOpenEnded( int MaxCount, float *m );
	int						Parse2DMatrix( int y, int x, float *m );
	int						Parse3DMatrix( int z, int y, int x, float *m );
							// parse a braced section into a string
	const char *			ParseBracedSection( idStr &out );
							// parse a braced section into a string, maintaining indents and newlines
	const char *			ParseBracedSectionExact ( idStr &out, int tabs = -1 );
							// parse the rest of the line
	const char *			ParseRestOfLine( idStr &out );
							// retrieves the white space characters before the last read token
	int						GetLastWhiteSpace( idStr &whiteSpace ) const;
							// returns start index into text buffer of last white space
	int						GetLastWhiteSpaceStart( void ) const;
							// returns end index into text buffer of last white space
	int						GetLastWhiteSpaceEnd( void ) const;
							// set an array with punctuations, NULL restores default C/C++ set, see default_punctuations for an example
	void					SetPunctuations( const punctuation_t *p );
							// returns a pointer to the punctuation with the given id
	const char *			GetPunctuationFromId( int id );
							// get the id for the given punctuation
	int						GetPunctuationId( const char *p );
							// set lexer flags
	void					SetFlags( int flags );
							// get lexer flags
	int						GetFlags( void );
							// reset the lexer
	void					Reset( void );
							// returns true if at the end of the file
	int						EndOfFile( void );
							// returns the current filename
	const char *			GetFileName( void );
							// get offset in script
	const int				GetFileOffset( void );
							// get file time
	const unsigned int		GetFileTime( void );
							// returns the current line number
	const int				GetLineNum( void );
							// print an error message
	void					Error( const char *str, ... );
							// print a warning message
	void					Warning( const char *str, ... );
							// returns true if Error() was called with LEXFL_NOFATALERRORS or LEXFL_NOERRORS set
	bool					HadError( void ) const;

							// write a binary representation of a token
	void					WriteBinaryToken(idToken *tok);

	static void				SetBaseFolder(char const * const);

// RAVEN BEGIN
// dluetscher: added method to parse a structure array that is made up of numerics (floats, ints), and stores them in the given storage
	void					ParseNumericStructArray( int numStructElements, int tokenSubTypeStructElements[], int arrayCount, byte *arrayStorage );
// RAVEN END

	// suffix that should be appended to the normal file extension to signify that the file is binary
	static idStr const		sCompiledFileSuffix;

protected:
	static char				baseFolder[256];

private:
	bool					OpenFile(char const *filename, bool OSPath);
	
#ifdef LEXER_READ_AHEAD
	LexerIOWrapper			mLexerIOWrapper;
#else
	idFile					*mFile;
#endif
	
	unsigned int			offset;
	idToken					unreadToken;
	unsigned int			unreadSize;
	bool					tokenAvailable;
	idLexer					*mDelegate;
	int						mFlags;
};

#endif

#ifdef _RAVEN // _QUAKE4
// jmarshall
class rvmScopedLexerBaseFolder
{
public:
	rvmScopedLexerBaseFolder(const char* baseFolder)
	{
		idLexer::SetBaseFolder(baseFolder);
	}

	~rvmScopedLexerBaseFolder()
	{
		idLexer::SetBaseFolder("");
	}
};
// jmarshall end
#endif

#endif /* !__LEXER_H__ */

