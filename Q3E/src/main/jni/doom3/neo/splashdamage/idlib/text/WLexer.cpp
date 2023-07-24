// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop


#define PUNCTABLE

//longer punctuations first
static wpunctuation_t default_punctuations[] = {
	// binary operators
	{L">>=",P_RSHIFT_ASSIGN},
	{L"<<=",P_LSHIFT_ASSIGN},
	//
	{L"...",P_PARMS},
	{L"->*",P_POINTER_TO_MEMBER_POINTER},
	// define merge operator
	{L"##",P_PRECOMPMERGE},				// pre-compiler
	// logic operators
	{L"&&",P_LOGIC_AND},				// pre-compiler
	{L"||",P_LOGIC_OR},					// pre-compiler
	{L">=",P_LOGIC_GEQ},				// pre-compiler
	{L"<=",P_LOGIC_LEQ},				// pre-compiler
	{L"==",P_LOGIC_EQ},					// pre-compiler
	{L"!=",P_LOGIC_UNEQ},				// pre-compiler
	// arithmetic operators
	{L"*=",P_MUL_ASSIGN},
	{L"/=",P_DIV_ASSIGN},
	{L"%=",P_MOD_ASSIGN},
	{L"+=",P_ADD_ASSIGN},
	{L"-=",P_SUB_ASSIGN},
	{L"++",P_INC},
	{L"--",P_DEC},
	// binary operators
	{L"&=",P_BIN_AND_ASSIGN},
	{L"|=",P_BIN_OR_ASSIGN},
	{L"^=",P_BIN_XOR_ASSIGN},
	{L">>",P_RSHIFT},					// pre-compiler
	{L"<<",P_LSHIFT},					// pre-compiler
	// member selection
	{L"->",P_MEMBER_SELECTION_POINTER},
	{L"::",P_SCOPE_RESOLUTION},
	{L".*",P_POINTER_TO_MEMBER_OBJECT},
	// arithmetic operators
	{L"*",P_MUL},						// pre-compiler
	{L"/",P_DIV},						// pre-compiler
	{L"%",P_MOD},						// pre-compiler
	{L"+",P_ADD},						// pre-compiler
	{L"-",P_SUB},						// pre-compiler
	{L"=",P_ASSIGN},
	// binary operators
	{L"&",P_BIN_AND},					// pre-compiler
	{L"|",P_BIN_OR},					// pre-compiler
	{L"^",P_BIN_XOR},					// pre-compiler
	{L"~",P_BIN_NOT},					// pre-compiler
	// logic operators
	{L"!",P_LOGIC_NOT},					// pre-compiler
	{L">",P_LOGIC_GREATER},				// pre-compiler
	{L"<",P_LOGIC_LESS},				// pre-compiler
	// member selection
	{L".",P_MEMBER_SELECTION_OBJECT},
	// separators
	{L",",P_COMMA},						// pre-compiler
	{L";",P_SEMICOLON},
	// label indication
	{L":",P_COLON},						// pre-compiler
	// if statement
	{L"?",P_QUESTIONMARK},				// pre-compiler
	// embracements
	{L"(",P_PARENTHESESOPEN},			// pre-compiler
	{L")",P_PARENTHESESCLOSE},			// pre-compiler
	{L"{",P_BRACEOPEN},					// pre-compiler
	{L"}",P_BRACECLOSE},				// pre-compiler
	{L"[",P_SQBRACKETOPEN},
	{L"]",P_SQBRACKETCLOSE},
	//
	{L"\\",P_BACKSLASH},
	// precompiler operator
	{L"#",P_PRECOMP},					// pre-compiler
	{L"$",P_DOLLAR},
	{NULL, 0}
};

static int default_punctuationtable[ 256 ];
static int default_nextpunctuation[ sizeof( default_punctuations ) / sizeof( wpunctuation_t ) ];
static bool default_setup;


/*
================
idWLexer::CreatePunctuationTable
================
*/
void idWLexer::CreatePunctuationTable( const wpunctuation_t* punctuations ) {
	int i, n, lastp;
	const wpunctuation_t *p, *newp;

	//get memory for the table
	if ( punctuations == default_punctuations ) {
		punctuationtable = default_punctuationtable;
		nextpunctuation = default_nextpunctuation;
		if ( default_setup ) {
			return;
		}
		default_setup = true;
		i = sizeof( default_punctuations ) / sizeof( punctuation_t );
	}
	else {
		if ( !punctuationtable || punctuationtable == default_punctuationtable ) {
			punctuationtable = (int *) Mem_Alloc( 256 * sizeof( int ) );
		}
		if ( nextpunctuation && nextpunctuation != default_nextpunctuation ) {
			Mem_Free( nextpunctuation );
		}
		for (i = 0; punctuations[i].p; i++) {
		}
		nextpunctuation = (int *) Mem_Alloc(i * sizeof(int));
	}
	memset(punctuationtable, 0xFF, 256 * sizeof(int));
	memset(nextpunctuation, 0xFF, i * sizeof(int));
	//add the punctuations in the list to the punctuation table
	for (i = 0; punctuations[i].p; i++) {
		newp = &punctuations[i];
		lastp = -1;
		//sort the punctuations in this table entry on length (longer punctuations first)
		for (n = punctuationtable[(unsigned int) newp->p[0]]; n >= 0; n = nextpunctuation[n] ) {
			p = &punctuations[n];
			if ( idWStr::Length( p->p ) < idWStr::Length( newp->p ) ) {
				nextpunctuation[i] = n;
				if (lastp >= 0) {
					nextpunctuation[lastp] = i;
				}
				else {
					punctuationtable[(unsigned int) newp->p[0]] = i;
				}
				break;
			}
			lastp = n;
		}
		if (n < 0) {
			nextpunctuation[i] = -1;
			if (lastp >= 0) {
				nextpunctuation[lastp] = i;
			}
			else {
				punctuationtable[(unsigned int) newp->p[0]] = i;
			}
		}
	}
}

/*
================
idWLexer::Error
================
*/
void idWLexer::Error( const char *str, ... ) {
	char text[MAX_STRING_CHARS];
	va_list ap;

	hadError = true;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );

	idLib::common->Warning( "file %s, line %d: %s", filename.c_str(), line, text );
}

/*
================
idWLexer::Warning
================
*/
void idWLexer::Warning( const char *str, ... ) {
	char text[4096];
	va_list ap;

	hadWarning = true;

	va_start( ap, str );
	vsprintf( text, str, ap );
	va_end( ap );

	idLib::common->Warning( "file %s, line %d: %s", filename.c_str(), line, text );
}

/*
================
idWLexer::SetPunctuations
================
*/
void idWLexer::SetPunctuations( const wpunctuation_t* p ) {
#ifdef PUNCTABLE
	if ( p != NULL ) {
		CreatePunctuationTable( p );
	} else {
		CreatePunctuationTable( default_punctuations );
	}
#endif //PUNCTABLE
	if ( p != NULL ) {
		punctuations = p;
	} else {
		punctuations = default_punctuations;
	}
}

/*
================
idWToken::SkipRestOfLine
================
*/
bool idWLexer::SkipRestOfLine( void ) {
	idWToken token;

	while( ReadToken( &token ) ) {
		if ( token.linesCrossed ) {
			script_p = lastScript_p;
			line = lastline;
			return true;
		}
	}
	return false;
}


/*
=================
idWLexer::SkipBracedSection

Skips until a matching close brace is found.
Internal brace depths are properly skipped.
=================
*/
bool idWLexer::SkipBracedSection( bool parseFirstBrace ) {
	idWToken token;
	int depth;

	depth = parseFirstBrace ? 0 : 1;
	do {
		if ( !ReadToken( &token ) ) {
			return false;
		}
		if ( token.type == TT_PUNCTUATION ) {
			if ( token == L"{" ) {
				depth++;
			} else if ( token == L"}" ) {
				depth--;
			}
		}
	} while( depth );
	return true;
}

/*
============
idWLexer::SkipBracedSectionExact
============
*/
bool idWLexer::SkipBracedSectionExact( bool parseFirstBrace ) {
	int		depth;
	if ( parseFirstBrace ) {
		if ( !idWLexer::ExpectTokenString( L"{" ) ) {
			return false;
		}
	}

	depth = 1;	

	while( depth ) {
		if ( !*idWLexer::script_p ) {
			return false;
		}

		char c = *(idWLexer::script_p++);

		switch ( c ) {			
			case L'{':
				depth++;
				break;
			case L'}':
				depth--;
				break;				
		}
	}
	return true;
}

/*
================
idWLexer::SkipWhiteSpace

Reads spaces, tabs, C-like comments etc.
When a newline character is found the scripts line counter is increased.
Returns 0 if there is no token left to be read.
================
*/
bool idWLexer::SkipWhiteSpace( bool currentLine ) {
	while ( true ) {
		// skip white space
		while ( *script_p <= L' ' ) {
			if ( !*script_p ) {
				return 0;
			}
			if ( *script_p == L'\n' ) {
				line++;
				if ( currentLine ) {
					script_p++;
					return 1;
				}
			}
			script_p++;
		}
		// skip comments
		if ( *script_p == L'/' ) {
			// comments //
			if ( *( script_p + 1 ) == L'/' ) {
				script_p++;
				do {
					script_p++;
					if ( *script_p == L'\0' ) {
						return false;
					}
				}
				while ( *script_p != L'\n' );
				line++;
				script_p++;
				if ( currentLine ) {
					return true;
				}
				if ( *script_p == L'\0' ) {
					return false;
				}
				continue;
			}
			// comments /* */
			else if ( *( script_p + 1 ) == L'*' ) {
				script_p++;
				while ( true ) {
					script_p++;
					if ( *script_p == L'\0' ) {
						return 0;
					}
					if ( *script_p == L'\n' ) {
						line++;
					}
					else if ( *script_p == L'/' ) {
						if ( *( script_p - 1 ) == L'*' ) {
							break;
						}
						if ( *( script_p + 1 ) == L'*' ) {
							Warning( "nested comment" );
						}
					}
				}
				script_p++;
				if ( *script_p == L'\0' ) {
					return false;
				}
				continue;
			}
		}
		break;
	}
	return true;
}

/*
================
idWLexer::ReadEscapeCharacter
================
*/
bool idWLexer::ReadEscapeCharacter( wchar_t* ch ) {
	wchar_t c;
	//int val, i;

	// step over the leading '\\'
	script_p++;
	// determine the escape character
	switch( *script_p ) {
		case L'\\': c = L'\\'; break;
		case L'n': c = L'\n'; break;
		case L'r': c = L'\r'; break;
		case L't': c = L'\t'; break;
		case L'v': c = L'\v'; break;
		case L'b': c = L'\b'; break;
		case L'f': c = L'\f'; break;
		case L'a': c = L'\a'; break;
		case L'\'': c = L'\''; break;
		case L'\"': c = L'\"'; break;
		case L'\?': c = L'\?'; break;
		default:
			Error( "unsupported escape character" );
			return false;
			break;
#if 0
		case L'x':
			{
				script_p++;
				for ( i = 0, val = 0; ; i++, script_p++ ) {
					c = *script_p;
					if ( c >= L'0' && c <= L'9' )
						c = c - L'0';
					else if (c >= L'A' && c <= L'Z')
						c = c - L'A' + 10;
					else if (c >= L'a' && c <= L'z')
						c = c - L'a' + 10;
					else
						break;
					val = ( val << 4 ) + c;
				}
				script_p--;
				if ( val > 0xFF ) {
					Warning( "too large value in escape character" );
					val = 0xFF;
				}
				c = val;
				break;
			}
		default: //NOTE: decimal ASCII code, NOT octal
			{
				if ( *script_p < L'0' || *script_p > L'9' ) {
					idLexer::Error("unknown escape char");
				}
				for ( i = 0, val = 0; ; i++, script_p++ ) {
					c = *script_p;
					if ( c >= L'0' && c <= L'9' )
						c = c - L'0';
					else
						break;
					val = val * 10 + c;
				}
				script_p--;
				if ( val > 0xFF ) {
					Warning( "too large value in escape character" );
					val = 0xFF;
				}
				c = val;
				break;
			}
		}
#endif
	}
	
	// step over the escape character or the last digit of the number
	script_p++;
	// store the escape character
	*ch = c;
	// succesfully read escape character
	return true;
}

/*
================
idWLexer::ReadString

Escape characters are interpretted.
Reads two strings with only a white space between them as one string.
================
*/
bool idWLexer::ReadString( idWToken *token, wchar_t quote ) {
	wchar_t ch;

	if ( quote == L'\"' ) {
		token->type = TT_STRING;
	} else {
		token->type = TT_LITERAL;
	}

	// leading quote
	script_p++;

	while ( true ) {
		// if there is an escape character and escape characters are allowed
		if ( *script_p == L'\\' ) {
			if ( !ReadEscapeCharacter( &ch ) ) {
				return false;
			}
			token->AppendDirty( ch );
		}
		// if a trailing quote
		else if ( *script_p == quote ) {
			// step over the quote
			script_p++;
			// consecutive strings should not be concatenated
			break;
		}
		else {
			if ( *script_p == L'\0' ) {
				Error( "missing trailing quote" );
				return false;
			}
			if ( *script_p == L'\n' ) {
				Error( "newline inside string" );
				return false;
			}
			token->AppendDirty( *script_p++ );
		}
	}
	token->data[token->len] = L'\0';

	if ( token->type == TT_LITERAL ) {
		token->subtype = (*token)[0];
	} else {
		// the sub type is the length of the string
		token->subtype = token->Length();
	}
	return true;
}


/*
================
idWLexer::ReadName
================
*/
bool idWLexer::ReadName( idWToken *token ) {
	wchar_t c;

	token->type = TT_NAME;
	do {
		token->AppendDirty( *script_p++ );
		c = *script_p;
	} while ((c >= L'a' && c <= L'z') ||
				(c >= L'A' && c <= L'Z') ||
				(c >= L'0' && c <= L'9') ||
				c == L'_' );
	token->data[token->len] = L'\0';
	//the sub type is the length of the name
	token->subtype = token->Length();
	return true;
}

/*
================
idWLexer::CheckString
================
*/
ID_INLINE bool idWLexer::CheckString( const wchar_t *str ) const {
	int i;

	for ( i = 0; str[i]; i++ ) {
		if ( script_p[i] != str[i] ) {
			return false;
		}
	}
	return true;
}

/*
================
idWLexer::ReadNumber
================
*/
bool idWLexer::ReadNumber( idWToken *token ) {
	int i;
	int dot;
	wchar_t c, c2;

	token->type = TT_NUMBER;
	token->subtype = 0;
	token->intvalue = 0;
	token->floatvalue = 0;

	c = *script_p;
	c2 = *(script_p + 1);

	if ( c == L'0' && c2 != L'.' ) {
		// check for a hexadecimal number
		if ( c2 == L'x' || c2 == L'X' ) {
			token->AppendDirty( *script_p++ );
			token->AppendDirty( *script_p++ );
			c = *script_p;
			while((c >= L'0' && c <= L'9') ||
						(c >= L'a' && c <= L'f') ||
						(c >= L'A' && c <= L'F')) {
				token->AppendDirty( c );
				c = *(++script_p);
			}
			token->subtype = TT_HEX | TT_INTEGER;
		}
		// check for a binary number
		else if ( c2 == L'b' || c2 == L'B' ) {
			token->AppendDirty( *script_p++ );
			token->AppendDirty( *script_p++ );
			c = *script_p;
			while( c == L'0' || c == L'1' ) {
				token->AppendDirty( c );
				c = *(++script_p);
			}
			token->subtype = TT_BINARY | TT_INTEGER;
		}
		// its an octal number
		else {
			token->AppendDirty( *script_p++ );
			c = *script_p;
			while( c >= L'0' && c <= L'7' ) {
				token->AppendDirty( c );
				c = *(++script_p);
			}
			token->subtype = TT_OCTAL | TT_INTEGER;
		}
	}
	else {
		// decimal integer or floating point number or ip address
		dot = 0;
		while( 1 ) {
			if ( c >= L'0' && c <= L'9' ) {
			} else if ( c == L'.' ) {
				dot++;
			} else {
				break;
			}
			token->AppendDirty( c );
			c = *(++script_p);
		}
		// if a floating point number
		if ( dot == 1 || c == L'e' ) {
			token->subtype = TT_DECIMAL | TT_FLOAT;
			// check for floating point exponent
			if ( c == L'e' ) {
				c = *(++script_p);
				if ( c == L'-' ) {
					token->AppendDirty( c );
					c = *(++script_p);
				} else if ( c == L'+' ) {
					token->AppendDirty( c );
					c = *(++script_p);
				}
				while( c >= L'0' && c <= L'9' ) {
					token->AppendDirty( c );
					c = *(++script_p);
				}
			}
			// check for floating point exception infinite 1.#INF or indefinite 1.#IND or NaN
			else if ( c == L'#' ) {
				token->AppendDirty( c );
				c = *(++script_p);
				if ( CheckString( L"INF" ) ) {
					token->subtype |= TT_INFINITE;
					c2 = 3;
				} else if ( CheckString( L"IND" ) ) {
					token->subtype |= TT_INDEFINITE;
					c2 = 3;
				} else if ( CheckString( L"NAN" ) ) {
					token->subtype |= TT_NAN;
					c2 = 3;
				} else if ( CheckString( L"QNAN" ) ) {
					token->subtype |= TT_NAN;
					c2 = 4;
				} else if ( CheckString( L"SNAN" ) ) {
					token->subtype |= TT_NAN;
					c2 = 4;
				}
				for ( i = 0; i < c2; i++ ) {
					token->AppendDirty( c );
					c = *(++script_p);
				}
				while( c >= L'0' && c <= L'9' ) {
					token->AppendDirty( c );
					c = *(++script_p);
				}
				token->AppendDirty( 0 );	// zero terminate for c_str
				Error( "parsed %s", token->c_str() );
			}
		}
		else if ( dot > 1 ) {
			Error( "more than one dot in number" );
		}
		else {
			token->subtype = TT_DECIMAL | TT_INTEGER;
		}
	}

	if ( token->subtype & TT_FLOAT ) {
		if ( c > L' ' ) {
			// single-precision: float
			if ( c == L'f' || c == L'F' ) {
				token->subtype |= TT_SINGLE_PRECISION;
				script_p++;
			}
			// extended-precision: long double
			else if ( c == L'l' || c == L'L' ) {
				token->subtype |= TT_EXTENDED_PRECISION;
				script_p++;
			}
			// default is double-precision: double
			else {
				token->subtype |= TT_DOUBLE_PRECISION;
			}
		}
		else {
			token->subtype |= TT_DOUBLE_PRECISION;
		}
	}
	else if ( token->subtype & TT_INTEGER ) {
		if ( c > L' ' ) {
			// default: signed long
			for ( i = 0; i < 2; i++ ) {
				// long integer
				if ( c == L'l' || c == L'L' ) {
					token->subtype |= TT_LONG;
				}
				// unsigned integer
				else if ( c == L'u' || c == L'U' ) {
					token->subtype |= TT_UNSIGNED;
				}
				else {
					break;
				}
				c = *(++script_p);
			}
		}
	}
	token->data[token->len] = L'\0';
	return true;
}

/*
================
idWLexer::ReadPunctuation
================
*/
bool idWLexer::ReadPunctuation( idWToken *token ) {
	int l, n, i;
	wchar_t* p;
	const wpunctuation_t *punc;

	if ( (unsigned int)*(script_p) >= 0xFF ) {
		return false;
	}

#ifdef PUNCTABLE
	for (n = punctuationtable[(unsigned int)*(script_p)]; n >= 0; n = nextpunctuation[n])
	{
		punc = &(punctuations[n]);
#else
	for (n = 0; punctuations[n].p; n++) {
		punc = &punctuations[n];
#endif
		p = punc->p;
		// check for this punctuation in the script
		for ( l = 0; p[l] && script_p[l]; l++ ) {
			if ( script_p[l] != p[l] ) {
				break;
			}
		}
		if ( !p[l] ) {
			//
			token->EnsureAlloced( l+1, false );
			for ( i = 0; i <= l; i++ ) {
				token->data[i] = p[i];
			}
			token->len = l;
			//
			script_p += l;
			token->type = TT_PUNCTUATION;
			// sub type is the punctuation id
			token->subtype = punc->n;
			return true;
		}
	}
	return false;
}

/*
================
idWLexer::ReadToken
================
*/
bool idWLexer::ReadToken( idWToken* token ) {
	wchar_t c;

	if ( !loaded ) {
		idLib::common->Error( "idWLexer::ReadToken: no file loaded" );
		return false;
	}

	// save script pointer
	lastScript_p = script_p;
	// save line counter
	lastline = line;
	// clear the token stuff
	token->data[0] = L'\0';
	token->len = 0;
	// start of the white space
	whiteSpaceStart_p = script_p;
	token->whiteSpaceStart_p = script_p;
	// read white space before token
	if ( !SkipWhiteSpace( false ) ) {
		return false;
	}
	// end of the white space
	whiteSpaceEnd_p = script_p;
	token->whiteSpaceEnd_p = script_p;
	// line the token is on
	token->line = line;
	// number of lines crossed before token
	token->linesCrossed = line - lastline;
	// clear token flags
	token->flags = 0;

	c = *script_p;

	// if we're keeping everything as whitespace delimited strings
	/*if ( flags & LEXFL_ONLYSTRINGS ) {
		// if there is a leading quote
		if ( c == '\"' || c == '\'' ) {
			if (!idLexer::ReadString( token, c )) {
				return 0;
			}
		} else if ( !idLexer::ReadName( token ) ) {
			return 0;
		}
	}*/
	// if there is a number
	if ( ( c >= L'0' && c <= L'9' ) ||
			( c == L'.' && ( *(script_p + 1) >= L'0' && *(script_p + 1) <= L'9' ) ) ) {
		if ( !ReadNumber( token ) ) {
			return false;
		}
	}
	// if there is a leading quote
	else if ( c == L'\"' || c == L'\'' ) {
		if ( !ReadString( token, c ) ) {
			return false;
		}
	}
	// if there is a name
	else if ( (c >= 'a' && c <= 'z') ||	(c >= 'A' && c <= 'Z') || c == '_' ) {
		if ( !ReadName( token ) ) {
			return 0;
		}
	}
	// check for punctuations
	else if ( !ReadPunctuation( token ) ) {
		Error( "unknown punctuation %lc", c );
		return 0;
	}
	// successfully read a token
	return true;
}

/*
================
idWLexer::ExpectTokenString
================
*/
bool idWLexer::ExpectTokenString( const wchar_t* string ) {
	idWToken token;

	if ( !ReadToken( &token ) ) {
		Error( "couldn't find expected '%ls'", string );
		return false;
	}
	if ( token != string ) {
		Error( "expected '%ls' but found '%ls'", string, token.c_str() );
		return false;
	}
	return true;
}

/*
================
idWLexer::ExpectAnyToken
================
*/
bool idWLexer::ExpectAnyToken( idWToken* token ) {
	if ( !ReadToken( token ) ) {
		Error( "couldn't read expected token" );
		return false;
	} else {
		return true;
	}
}

/*
================
idWLexer::LoadMemory
================
*/
bool idWLexer::LoadMemory( const wchar_t* ptr, int length, const char* name, int startLine ) {
	if ( loaded ) {
		idLib::common->Error( "idWLexer::LoadMemory: another script already loaded" );
		return false;
	}
	filename = name;
	filename.CollapsePath();
	buffer = ptr;
	length = length;
	// pointer in script buffer
	script_p = buffer;
	// pointer in script buffer before reading token
	lastScript_p = buffer;
	// pointer to end of script buffer
	end_p = &(buffer[length]);

	line = startLine;
	lastline = startLine;
	allocated = false;
	loaded = true;

	return true;
}

/*
================
idWLexer::FreeSource
================
*/
void idWLexer::FreeSource( void ) {
	if ( allocated ) {
		Mem_Free( (void*)buffer );
		buffer = NULL;
		allocated = false;
	}
	loaded = false;
}

/*
================
idWLexer::idWLexer
================
*/
idWLexer::idWLexer( void ) {
	loaded = false;
	filename = "";
	SetPunctuations( NULL );
	allocated = false;
	line = 0;
	lastline = 0;
	hadError = false;
	hadWarning = false;
}

/*
================
idWLexer::idWLexer
================
*/
idWLexer::idWLexer( const wchar_t* ptr, int length, const char* name, int startLine ) {
	loaded = false;
	SetPunctuations( NULL );
	allocated = false;
	hadError = false;
	hadWarning = false;
	LoadMemory( ptr, length, name, startLine );
}

/*
================
idWLexer::~idWLexer
================
*/
idWLexer::~idWLexer( void ) {
	FreeSource();
}
