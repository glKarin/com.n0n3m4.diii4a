// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PARSER_H__
#define __PARSER_H__

/*
===============================================================================

	C/C++ compatible pre-compiler

===============================================================================
*/

#define DEFINE_FIXED			0x0001

#define BUILTIN_LINE			1
#define BUILTIN_FILE			2
#define BUILTIN_DATE			3
#define BUILTIN_TIME			4
#define BUILTIN_STDC			5

#define INDENT_IF				0x0001
#define INDENT_ELSE				0x0002
#define INDENT_ELIF				0x0004
#define INDENT_IFDEF			0x0008
#define INDENT_IFNDEF			0x0010

// macro definitions
struct define_t {
	char *			name;						// define name
	int				flags;						// define flags
	int				builtin;					// > 0 if builtin define
	int				numparms;					// number of define parameters
	idToken *		parms;						// define parameters
	idToken *		tokens;						// macro tokens (possibly containing parm tokens)
	define_t *		next;						// next defined macro in a list
	define_t *		hashnext;					// next define in the hash chain
};

// indents used for conditional compilation directives:
// #if, #else, #elif, #ifdef, #ifndef
struct indent_t {
	int				type;						// indent type
	int				skip;						// true if skipping current indent
	int				skipElse;					// true if any following else sections should be skipped
	idLexer *		script;						// script the indent was in
	indent_t *		next;						// next indent on the indent stack
};

typedef void (*pragmaFunc_t)( void *data, const char *pragma );

class idParser {

public:
						// constructor
						idParser();
						idParser( int flags );
						idParser( const char *filename, int flags = 0, bool OSPath = false );
						idParser( const char *ptr, int length, const char *name, int flags = 0 );
						// destructor
						~idParser();
						// load a source file
	int					LoadFile( const char *filename, bool OSPath = false, int startLine = 1 );
						// load a source from the given memory with the given length
						// NOTE: the ptr is expected to point at a valid C string: ptr[length] == '\0'
	int					LoadMemory( const char *ptr, int length, const char *name, int startLine = 1 );
	int					LoadMemoryBinary( const byte *ptr, int length, const char *name, idTokenCache* globals );

	void				WriteBinary( idFile* f, idTokenCache* tokenCache = NULL );

	void				ResetBinaryParsing();

						// free the current source
	void				FreeSource( bool keepDefines = false );
						// returns true if a source is loaded
	int					IsLoaded( void ) const { return idParser::loaded; }
						// read a token from the source
	int					ReadToken( idToken *token );
						// expect a certain token, reads the token when available
	bool				ExpectTokenString( const char *string, idToken* other = NULL );
						// expect a certain token type
	int					ExpectTokenType( int type, int subtype, idToken *token );
						// expect a token
	int					ExpectAnyToken( idToken *token );
						// returns true if the next token equals the given string and removes the token from the source
	int					CheckTokenString( const char *string );
						// returns true if the next token equals the given type and removes the token from the source
	int					CheckTokenType( int type, int subtype, idToken *token );
						// returns true if the next token equals the given string but does not remove the token from the source
	int					PeekTokenString( const char *string );
						// returns true if the next token equals the given type but does not remove the token from the source
	int					PeekTokenType( int type, int subtype, idToken *token );
						// skip tokens until the given token string is read
	int					SkipUntilString( const char *string, idToken *token );
						// skip the rest of the current line
	int					SkipRestOfLine( void );
						// skip the braced section
	int					SkipBracedSection( bool parseFirstBrace = true );
						// parse a braced section into a string
	const char *		ParseBracedSection( idStr &out, int tabs = -1, bool parseFirstBrace = true, char intro = '{', char outro = '}' );
						// parse the rest of the line
	const char *		ParseRestOfLine( idStr &out );
						// unread the given token
	void				UnreadToken( const idToken& token );
						// read a token only if on the current line
	int					ReadTokenOnLine( idToken *token );
						// read a signed integer
	int					ParseInt( void );
						// read a boolean
	bool				ParseBool( void );
						// read a floating point number
	float				ParseFloat( bool* hadError = NULL );
						// parse matrices with floats
	int					Parse1DMatrix( int x, float *m );
	int					Parse2DMatrix( int y, int x, float *m );
	int					Parse3DMatrix( int z, int y, int x, float *m );
						// retrieves the white space after the last read token
	int					GetNextWhiteSpace( idStr &whiteSpace, bool currentLine );
						// retrieves the white space before the last read token
	int					GetLastWhiteSpace( idStr &whiteSpace ) const;
						// add a define to the source
	int					AddDefine( const char *string );
						// add includes to the source
	int					AddIncludes( const idStrList& includes );
						// add an include to the source
	int					AddInclude( const char *string );
						// add builtin defines
	void				AddBuiltinDefines( void );
						// set the source include path
	void				SetIncludePath( const char *path );
						// set pragma callback
	void				SetPragmaCallback( void *data, pragmaFunc_t func );
						// set the punctuation set
	void				SetPunctuations( const punctuation_t *p );
						// returns a pointer to the punctuation with the given id
	const char *		GetPunctuationFromId( int id );
						// get the id for the given punctuation
	int					GetPunctuationId( const char *p );
						// set lexer flags
	void				SetFlags( int flags );
						// get lexer flags
	int					GetFlags( void ) const;
						// returns the current filename
	const char *		GetFileName( void ) const;
						// get current offset in current script
	const int			GetFileOffset( void ) const;
						// get file time for current script
	const unsigned int	GetFileTime( void ) const;
						// returns the current line number
	const int			GetLineNum( void ) const;
						// print an error message
	void				Error( const char *str, ... ) const;
						// print a warning message
	void				Warning( const char *str, ... ) const;
						// for enumerating only the current level
	int					GetCurrentDependency( void ) const;
						// walk all included files, start from 0 to enumerate all
	const char*			GetNextDependency( int &index ) const;
						// add a global define that will be added to all opened sources
	static int			AddGlobalDefine( const char *string );
						// remove the given global define
	static int			RemoveGlobalDefine( const char *name );
						// remove all global defines
	static void			RemoveAllGlobalDefines( void );
						// set the base folder to load files from
	static void			SetBaseFolder( const char *path );

						// save off the state of the dependencies list
	void				PushDependencies();
	void				PopDependencies();

private:
	int					loaded;						// set when a source file is loaded from file or memory
	idStr				filename;					// file name of the script
	idStr				includepath;				// path to include files
	bool				OSPath;						// true if the file was loaded from an OS path
	const punctuation_t *punctuations;				// punctuations to use
	int					flags;						// flags used for script parsing
	idLexer *			scriptstack;				// stack with scripts of the source
	idToken *			tokens;						// tokens to read first
	define_t *			defines;					// list with macro definitions
	define_t **			definehash;					// hash chain with defines
	indent_t *			indentstack;				// stack with indents
	int					skip;						// > 0 if skipping conditional code
	pragmaFunc_t		pragmaCallback;				// called when a #pragma is parsed
	void *				pragmaData;
	int					startLine;					// line offset

	static define_t *	globaldefines;				// list with global defines added to every source loaded

	idStrList			dependencies;				// list of filenames that have been included
	sdStack< int >		dependencyStateStack;		// stack of the number of dependencies


private:
	void				PushIndent( int type, int skip, int skipElse );
	void				PopIndent( int &type, int &skip, int &skipElse );
	bool				PushScript( idLexer *script );
	int					ReadSourceToken( idToken *token );
	int					ReadLine( idToken *token, bool multiline );
	bool				UnreadSourceToken( const idToken& token );
	int					ReadDefineParms( define_t *define, idToken **parms, int maxparms );
	int					StringizeTokens( idToken *tokens, idToken *token );
	int					MergeTokens( idToken *t1, idToken *t2 );
	int					ExpandBuiltinDefine( idToken *deftoken, define_t *define, idToken **firsttoken, idToken **lasttoken );
	int					ExpandDefine( idToken *deftoken, define_t *define, idToken **firsttoken, idToken **lasttoken );
	int					ExpandDefineIntoSource( idToken *deftoken, define_t *define );
	void				AddGlobalDefinesToSource( void );
	define_t *			CopyDefine( define_t *define );
	define_t *			FindHashedDefine(define_t **definehash, const char *name);
	int					FindDefineParm( define_t *define, const char *name );
	void				AddDefineToHash(define_t *define, define_t **definehash);
	static void			PrintDefine( define_t *define );
	static void			FreeDefine( define_t *define );
	static define_t *	FindDefine( define_t *defines, const char *name );
	static define_t *	DefineFromString( const char *string );
	define_t *			CopyFirstDefine( void );
	void				UnreadSignToken( void );

	int					EvaluateTokens( idToken *tokens, signed long int *intvalue, double *floatvalue, int integer );
	int					Evaluate( signed long int *intvalue, double *floatvalue, int integer );
	int					DollarEvaluate( signed long int *intvalue, double *floatvalue, int integer);

	int					Directive_include( void );
	int					Directive_define( bool isTemplate );
	int					Directive_undef( void );
	int					Directive_if_def( int type );
	int					Directive_ifdef( void );
	int					Directive_ifndef( void );
	int					Directive_else( void );
	int					Directive_endif( void );
	int					Directive_elif( void );
	int					Directive_if( void );
	int					Directive_line( void );
	int					Directive_error( void );
	int					Directive_warning( void );
	int					Directive_pragma( void );
	int					ReadDirective( void );

	int					DollarDirective_if_def( int type );
	int					DollarDirective_ifdef( void );
	int					DollarDirective_ifndef( void );
	int					DollarDirective_else( void );
	int					DollarDirective_endif( void );
	int					DollarDirective_elif( void );
	int					DollarDirective_if( void );
	int					DollarDirective_evalint( void );
	int					DollarDirective_evalfloat( void );
	int					ReadDollarDirective( void );
};

ID_INLINE const char *idParser::GetFileName( void ) const {
	if ( scriptstack ) {
		return scriptstack->GetFileName();
	} else {
		return "";
	}
}

ID_INLINE const int idParser::GetFileOffset( void ) const {
	if ( scriptstack ) {
		return scriptstack->GetFileOffset();
	} else {
		return 0;
	}
}

ID_INLINE const unsigned int idParser::GetFileTime( void ) const {
	if ( scriptstack ) {
		return scriptstack->GetFileTime();
	} else {
		return 0;
	}
}

ID_INLINE const int idParser::GetLineNum( void ) const {
	if ( scriptstack ) {
		return scriptstack->GetLineNum();
	} else {
		return 0;
	}
}

#endif /* !__PARSER_H__ */
