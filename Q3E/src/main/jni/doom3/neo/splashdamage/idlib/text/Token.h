// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __TOKEN_H__
#define __TOKEN_H__

/*
===============================================================================

	idToken is a token read from a file or memory with idLexer or idParser

===============================================================================
*/

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

extern const char sdPoolAllocator_idToken[];
class idToken : 
	public idStr,
	public sdPoolAllocator< idToken, sdPoolAllocator_idToken, 128 > {

	friend class idParser;
	friend class idLexer;
	friend class idLexerBinary;
	friend class idTokenCache;

public:
	int				type;								// token type
	int				subtype;							// token sub type
	int				line;								// line in script the token was on
	int				linesCrossed;						// number of lines crossed in white space before token
	int				flags;								// token flags, used for recursive defines

public:
					idToken( void );
					idToken( const idToken &token );
					~idToken( void );

	void			operator=( const idStr &text );
	void			operator=( const char *text );
	//idToken&		operator=( const idToken& rhs );

	double			GetDoubleValue( void );				// double value of TT_NUMBER
	float			GetFloatValue( void );				// float value of TT_NUMBER
	unsigned long	GetUnsignedLongValue( void );		// unsigned long value of TT_NUMBER
	int				GetIntValue( void );				// int value of TT_NUMBER
	int				WhiteSpaceBeforeToken( void ) const;// returns length of whitespace before token
	void			ClearTokenWhiteSpace( void );		// forget whitespace before token
	unsigned short	GetBinaryIndex( void ) const;			// token index in a binary stream

	void			NumberValue( void );				// calculate values for a TT_NUMBER

	// only to be used by parsers
	void			AppendDirty( const char a );		// append character without adding trailing zero

	void			SetIntValue( unsigned long intvalue );
	void			SetFloatValue( double floatvalue );

private:
	unsigned long	intvalue;							// integer value
	double			floatvalue;							// floating point value
	const char *	whiteSpaceStart_p;					// start of white space before token, only used by idLexer
	const char *	whiteSpaceEnd_p;					// end of white space before token, only used by idLexer
	idToken *		next;								// next token in chain, only used by idParser
	unsigned short	binaryIndex;						// token index in a binary stream
};

ID_INLINE idToken::idToken( void ) :
	intvalue( 0 ),
	floatvalue( 0.0 ),
	whiteSpaceStart_p( NULL ),
	whiteSpaceEnd_p( NULL ),
	next( NULL ),
	type( 0 ),
	subtype( 0 ),
	line( 0 ),
	linesCrossed( 0 ),
	flags( 0 ),
	binaryIndex( 0 ) {
}

ID_INLINE idToken::idToken( const idToken &token ) {
	*this = token;
}

ID_INLINE idToken::~idToken( void ) {
}
/*
ID_INLINE idToken& idToken::operator=( const idToken& rhs ) {
	if( this != &rhs ) {
		intvalue = rhs.intvalue;
		floatvalue = rhs.floatvalue;
		whiteSpaceEnd_p = rhs.whiteSpaceEnd_p;
		whiteSpaceStart_p = rhs.whiteSpaceStart_p;
		next = rhs.next;

		type  = rhs.type;
		subtype = rhs.subtype;
		line = rhs.line;
		linesCrossed = rhs.linesCrossed;
		flags = rhs.flags;
	}
	return *this;
}
*/

ID_INLINE void idToken::operator=( const char *text) {
	idStr::operator=( text );
}

ID_INLINE void idToken::operator=( const idStr& text ) {
	idStr::operator=( text );
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
	data[len++] = a;
}

ID_INLINE void idToken::SetIntValue( unsigned long intvalue ) {
	this->intvalue = intvalue;
}

ID_INLINE void idToken::SetFloatValue( double floatvalue ) {
	this->floatvalue = floatvalue;
}

ID_INLINE unsigned short idToken::GetBinaryIndex( void ) const {
	return binaryIndex;
}

#endif /* !__TOKEN_H__ */
