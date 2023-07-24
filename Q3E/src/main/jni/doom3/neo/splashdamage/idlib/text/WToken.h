// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __WTOKEN_H__
#define __WTOKEN_H__

/*
===============================================================================

	idWToken is a wide string version of idToken

===============================================================================
*/

class idWToken : 
	public idWStr,
	public sdPoolAllocator< idWToken, sdPoolAllocator_DefaultIdentifier, 128 > {

	friend class idWLexer;

public:
	int				type;								// token type
	int				subtype;							// token sub type
	int				line;								// line in script the token was on
	int				linesCrossed;						// number of lines crossed in white space before token
	int				flags;								// token flags, used for recursive defines

public:
					idWToken( void );
					idWToken( const idWToken &token );
					~idWToken( void );

	void			operator=( const idWStr &text );
	void			operator=( const wchar_t *text );
	//idWToken&		operator=( const idWToken& rhs );

	double			GetDoubleValue( void );				// double value of TT_NUMBER
	float			GetFloatValue( void );				// float value of TT_NUMBER
	unsigned long	GetUnsignedLongValue( void );		// unsigned long value of TT_NUMBER
	int				GetIntValue( void );				// int value of TT_NUMBER
	int				WhiteSpaceBeforeToken( void ) const;// returns length of whitespace before token
	void			ClearTokenWhiteSpace( void );		// forget whitespace before token

	void			NumberValue( void );				// calculate values for a TT_NUMBER

	// only to be used by parsers
	void			AppendDirty( const wchar_t a );		// append character without adding trailing zero

	void			SetIntValue( unsigned long intvalue );
	void			SetFloatValue( double floatvalue );

private:
	unsigned long	intvalue;							// integer value
	double			floatvalue;							// floating point value
	const wchar_t *	whiteSpaceStart_p;					// start of white space before token, only used by idLexer
	const wchar_t *	whiteSpaceEnd_p;					// end of white space before token, only used by idLexer
	idWToken *		next;								// next token in chain, only used by idParser
};

ID_INLINE idWToken::idWToken( void ) :
	intvalue( 0 ),
	floatvalue( 0.0 ),
	whiteSpaceStart_p( NULL ),
	whiteSpaceEnd_p( NULL ),
	next( NULL ),
	type( 0 ),
	subtype( 0 ),
	line( 0 ),
	linesCrossed( 0 ),
	flags( 0 ) {
}

ID_INLINE idWToken::idWToken( const idWToken &token ) {
	*this = token;
}

ID_INLINE idWToken::~idWToken( void ) {
}
/*
ID_INLINE idWToken& idWToken::operator=( const idWToken& rhs ) {
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

ID_INLINE void idWToken::operator=( const wchar_t *text) {
	idWStr::operator=( text );
}

ID_INLINE void idWToken::operator=( const idWStr& text ) {
	idWStr::operator=( text );
}

ID_INLINE double idWToken::GetDoubleValue( void ) {
	if ( type != TT_NUMBER ) {
		return 0.0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return floatvalue;
}

ID_INLINE float idWToken::GetFloatValue( void ) {
	return (float) GetDoubleValue();
}

ID_INLINE unsigned long	idWToken::GetUnsignedLongValue( void ) {
	if ( type != TT_NUMBER ) {
		return 0;
	}
	if ( !(subtype & TT_VALUESVALID) ) {
		NumberValue();
	}
	return intvalue;
}

ID_INLINE int idWToken::GetIntValue( void ) {
	return (int) GetUnsignedLongValue();
}

ID_INLINE int idWToken::WhiteSpaceBeforeToken( void ) const {
	return ( whiteSpaceEnd_p > whiteSpaceStart_p );
}

ID_INLINE void idWToken::AppendDirty( const wchar_t a ) {
	EnsureAlloced( len + 2, true );
	data[len++] = a;
}

ID_INLINE void idWToken::SetIntValue( unsigned long intvalue ) {
	this->intvalue = intvalue;
}

ID_INLINE void idWToken::SetFloatValue( double floatvalue ) {
	this->floatvalue = floatvalue;
}

#endif /* !__TOKEN_H__ */
