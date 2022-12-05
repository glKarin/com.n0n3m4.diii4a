
#ifndef __TOKEN_H__
#define __TOKEN_H__

/*
===============================================================================

	idToken is a token read from a file or memory with idLexer or idParser

===============================================================================
*/
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

// token types
#define TT_STRING					1		// string
#define TT_LITERAL					2		// literal
#define TT_NUMBER					3		// number
#define TT_NAME						4		// name
#define TT_PUNCTUATION				5		// punctuation

// number sub types
#define TT_INTEGER					0x00001		// integer
#define TT_DECIMAL					0x00002		// decimal number
#define TT_HEX						0x00004		// hexadecimal number
#define TT_OCTAL					0x00008		// octal number
#define TT_BINARY					0x00010		// binary number
#define TT_LONG						0x00020		// long int
#define TT_UNSIGNED					0x00040		// unsigned int
#define TT_FLOAT					0x00080		// floating point number
#define TT_SINGLE_PRECISION			0x00100		// float
#define TT_DOUBLE_PRECISION			0x00200		// double
#define TT_EXTENDED_PRECISION		0x00400		// long double
#define TT_INFINITE					0x00800		// infinite 1.#INF
#define TT_INDEFINITE				0x01000		// indefinite 1.#IND
#define TT_NAN						0x02000		// NaN
#define TT_IPADDRESS				0x04000		// ip address
#define TT_IPPORT					0x08000		// ip port
#define TT_VALUESVALID				0x10000		// set if intvalue and floatvalue are valid

// string sub type is the length of the string
// literal sub type is the ASCII code
// punctuation sub type is the punctuation id
// name sub type is the length of the name

class idToken : public idStr {

	friend class idParser;
	friend class idLexer;
// RAVEN BEGIN
// jsinger: added to allow Lexer direct access to token internals as well
	friend class Lexer;
// RAVEN END

public:
	int				type;								// token type
	int				subtype;							// token sub type
	int				line;								// line in script the token was on
	int				linesCrossed;						// number of lines crossed in white space before token
	int				flags;								// token flags, used for recursive defines

public:
					idToken( void );
					idToken( const idToken *token );
					~idToken( void );

	void			operator=( const idStr& text );
	void			operator=( const char *text );

	double			GetDoubleValue( void );				// double value of TT_NUMBER
	float			GetFloatValue( void );				// float value of TT_NUMBER
	unsigned long	GetUnsignedLongValue( void );		// unsigned long value of TT_NUMBER
	int				GetIntValue( void );				// int value of TT_NUMBER
	int				WhiteSpaceBeforeToken( void ) const;// returns length of whitespace before token
	void			ClearTokenWhiteSpace( void );		// forget whitespace before token

	void			NumberValue( void );				// calculate values for a TT_NUMBER

private:
	unsigned long	intvalue;							// integer value
	double			floatvalue;							// floating point value
	const char *	whiteSpaceStart_p;					// start of white space before token, only used by idLexer
	const char *	whiteSpaceEnd_p;					// end of white space before token, only used by idLexer
	idToken *		next;								// next token in chain, only used by idParser

	void			AppendDirty( const char a );		// append character without adding trailing zero
};

// RAVEN BEGIN
// rjohnson: initialized floatvalue to prevent fpu exceptin

ID_INLINE idToken::idToken( void ) :
	floatvalue(0.0)
{
}

// RAVEN END

ID_INLINE idToken::idToken( const idToken *token ) {
	*this = *token;
}

ID_INLINE idToken::~idToken( void ) {
}

ID_INLINE void idToken::operator=( const char *text) {
	*static_cast<idStr *>(this) = text;
}

ID_INLINE void idToken::operator=( const idStr& text ) {
	*static_cast<idStr *>(this) = text;
}

ID_INLINE double idToken::GetDoubleValue( void ) {
	if ( type != TT_NUMBER ) {
		return 0.0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return floatvalue;
}

ID_INLINE float idToken::GetFloatValue( void ) {
	return (float) GetDoubleValue();
}

ID_INLINE unsigned long	idToken::GetUnsignedLongValue( void ) {
	if ( type != TT_NUMBER ) {
		return 0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return intvalue;
}

ID_INLINE int idToken::GetIntValue( void ) {
	return (int) GetUnsignedLongValue();
}

ID_INLINE int idToken::WhiteSpaceBeforeToken( void ) const {
	return ( whiteSpaceEnd_p > whiteSpaceStart_p );
}

ID_INLINE void idToken::AppendDirty( const char a ) {
	EnsureAlloced( len + 2, true );
// RAVEN BEGIN
// jscott: I hate slashes nearly as much as KRABS
	if( a == '\\' ) {
		data[len++] = '/';
	} else {
		data[len++] = a;
	}
// RAVEN END
}

#endif /* !__TOKEN_H__ */
