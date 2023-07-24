// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __FLOATERRORS_H__
#define __FLOATERRORS_H__

/*
===============================================================================

	Float to int conversions are expensive and should all be made explicit
	in the code using idMath::Ftoi(). Double literals should also never be
	used throughout the engine because automatic promotion of expressions
	to double may come at a performance penalty.

	The define FORCE_FLOAT_ERRORS can be enabled to generate compile errors
	for all C/C++ float to int conversions, both implicit and explicit.
	With this define enabled the compiler also generates errors for all
	double literals.

===============================================================================
*/

#ifndef __TYPE_INFO_GEN__
//#define FORCE_FLOAT_ERRORS
#endif

#ifdef FORCE_FLOAT_ERRORS

struct idTestFloat;

struct idUnionFloat {
	operator			float & ( void ) { return value; }
	operator			float ( void ) const { return value; }
	float				operator=( const float f ) { value = f; return f; }
	float				value;
};

struct idTestFloat {
						idTestFloat( void ) {}
						idTestFloat( float f ) { value = f; }
						idTestFloat( int i ) { value = i; }
						idTestFloat( long i ) { value = i; }
						idTestFloat( unsigned int i ) { value = i; }
						idTestFloat( unsigned long i ) { value = i; }
						idTestFloat( idUnionFloat f ) { value = f.value; }

	operator			float & ( void ) { return value; }
	operator			float ( void ) const { return value; }
	operator			bool ( void ) { return ( value != 0.0f ); }

	idTestFloat			operator-() const { return idTestFloat( -value ); }
	idTestFloat			operator+() const { return *this; }
	bool				operator!() const { return ( value == 0.0f ); }

	bool				operator&&( const idTestFloat f ) const { return ( value != 0.0f && f.value != 0.0f ); }
	bool				operator||( const idTestFloat f ) const { return ( value != 0.0f || f.value != 0.0f ); }
	bool				operator&&( const bool b ) const { return ( value != 0.0f && b ); }
	bool				operator||( const bool b ) const { return ( value != 0.0f || b ); }

	idTestFloat			operator*( const idTestFloat f ) const { return idTestFloat( value * f.value ); }
	idTestFloat			operator/( const idTestFloat f ) const { return idTestFloat( value / f.value ); }
	idTestFloat			operator+( const idTestFloat f ) const { return idTestFloat( value + f.value ); }
	idTestFloat			operator-( const idTestFloat f ) const { return idTestFloat( value - f.value ); }
	bool				operator<( const idTestFloat f ) const { return ( value < f.value ); }
	bool				operator<=( const idTestFloat f ) const { return ( value <= f.value ); }
	bool				operator>( const idTestFloat f ) const { return ( value > f.value ); }
	bool				operator>=( const idTestFloat f ) const { return ( value >= f.value ); }
	bool				operator==( const idTestFloat f ) const { return ( value == f.value ); }
	bool				operator!=( const idTestFloat f ) const { return ( value != f.value ); }
	idTestFloat			operator*=( const idTestFloat f ) { value *= f.value; return *this; }
	idTestFloat			operator/=( const idTestFloat f ) { value /= f.value; return *this; }
	idTestFloat			operator+=( const idTestFloat f ) { value += f.value; return *this; }
	idTestFloat			operator-=( const idTestFloat f ) { value -= f.value; return *this; }

	idTestFloat			operator*( const float f ) const { return idTestFloat( value * f ); }
	idTestFloat			operator/( const float f ) const { return idTestFloat( value / f ); }
	idTestFloat			operator+( const float f ) const { return idTestFloat( value + f ); }
	idTestFloat			operator-( const float f ) const { return idTestFloat( value - f ); }
	bool				operator<( const float f ) const { return ( value < f ); }
	bool				operator<=( const float f ) const { return ( value <= f ); }
	bool				operator>( const float f ) const { return ( value > f ); }
	bool				operator>=( const float f ) const { return ( value >= f ); }
	bool				operator==( const float f ) const { return ( value == f ); }
	bool				operator!=( const float f ) const { return ( value != f ); }
	idTestFloat			operator*=( const float f ) { value *= f; return *this; }
	idTestFloat			operator/=( const float f ) { value /= f; return *this; }
	idTestFloat			operator+=( const float f ) { value += f; return *this; }
	idTestFloat			operator-=( const float f ) { value -= f; return *this; }
	friend idTestFloat	operator*( const float i, const idTestFloat f ) { return idTestFloat( i * f.value ); }
	friend idTestFloat	operator/( const float i, const idTestFloat f ) { return idTestFloat( i / f.value ); }
	friend idTestFloat	operator+( const float i, const idTestFloat f ) { return idTestFloat( i + f.value ); }
	friend idTestFloat	operator-( const float i, const idTestFloat f ) { return idTestFloat( i - f.value ); }
	friend bool			operator<( const float i, const idTestFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const float i, const idTestFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const float i, const idTestFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const float i, const idTestFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const float i, const idTestFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const float i, const idTestFloat f ) { return ( i != f.value ); }

	idTestFloat			operator*( const int i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator/( const int i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator+( const int i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator-( const int i ) const { return idTestFloat( value * i ); }
	bool				operator<( const int i ) const { return ( value < i ); }
	bool				operator<=( const int i ) const { return ( value <= i ); }
	bool				operator>( const int i ) const { return ( value > i ); }
	bool				operator>=( const int i ) const { return ( value >= i ); }
	bool				operator==( const int i ) const { return ( value == i ); }
	bool				operator!=( const int i ) const { return ( value != i ); }
	idTestFloat			operator*=( const int i ) { value *= i; return *this; }
	idTestFloat			operator/=( const int i ) { value /= i; return *this; }
	idTestFloat			operator+=( const int i ) { value += i; return *this; }
	idTestFloat			operator-=( const int i ) { value -= i; return *this; }
	friend idTestFloat	operator*( const int i, const idTestFloat f ) { return idTestFloat( i * f.value ); }
	friend idTestFloat	operator/( const int i, const idTestFloat f ) { return idTestFloat( i / f.value ); }
	friend idTestFloat	operator+( const int i, const idTestFloat f ) { return idTestFloat( i + f.value ); }
	friend idTestFloat	operator-( const int i, const idTestFloat f ) { return idTestFloat( i - f.value ); }
	friend bool			operator<( const int i, const idTestFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const int i, const idTestFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const int i, const idTestFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const int i, const idTestFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const int i, const idTestFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const int i, const idTestFloat f ) { return ( i != f.value ); }

	idTestFloat			operator*( const short i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator/( const short i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator+( const short i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator-( const short i ) const { return idTestFloat( value * i ); }
	bool				operator<( const short i ) const { return ( value < i ); }
	bool				operator<=( const short i ) const { return ( value <= i ); }
	bool				operator>( const short i ) const { return ( value > i ); }
	bool				operator>=( const short i ) const { return ( value >= i ); }
	bool				operator==( const short i ) const { return ( value == i ); }
	bool				operator!=( const short i ) const { return ( value != i ); }
	idTestFloat			operator*=( const short i ) { value *= i; return *this; }
	idTestFloat			operator/=( const short i ) { value /= i; return *this; }
	idTestFloat			operator+=( const short i ) { value += i; return *this; }
	idTestFloat			operator-=( const short i ) { value -= i; return *this; }
	friend idTestFloat	operator*( const short i, const idTestFloat f ) { return idTestFloat( i * f.value ); }
	friend idTestFloat	operator/( const short i, const idTestFloat f ) { return idTestFloat( i / f.value ); }
	friend idTestFloat	operator+( const short i, const idTestFloat f ) { return idTestFloat( i + f.value ); }
	friend idTestFloat	operator-( const short i, const idTestFloat f ) { return idTestFloat( i - f.value ); }
	friend bool			operator<( const short i, const idTestFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const short i, const idTestFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const short i, const idTestFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const short i, const idTestFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const short i, const idTestFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const short i, const idTestFloat f ) { return ( i != f.value ); }

	idTestFloat			operator*( const char i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator/( const char i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator+( const char i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator-( const char i ) const { return idTestFloat( value * i ); }
	bool				operator<( const char i ) const { return ( value < i ); }
	bool				operator<=( const char i ) const { return ( value <= i ); }
	bool				operator>( const char i ) const { return ( value > i ); }
	bool				operator>=( const char i ) const { return ( value >= i ); }
	bool				operator==( const char i ) const { return ( value == i ); }
	bool				operator!=( const char i ) const { return ( value != i ); }
	idTestFloat			operator*=( const char i ) { value *= i; return *this; }
	idTestFloat			operator/=( const char i ) { value /= i; return *this; }
	idTestFloat			operator+=( const char i ) { value += i; return *this; }
	idTestFloat			operator-=( const char i ) { value -= i; return *this; }
	friend idTestFloat	operator*( const char i, const idTestFloat f ) { return idTestFloat( i * f.value ); }
	friend idTestFloat	operator/( const char i, const idTestFloat f ) { return idTestFloat( i / f.value ); }
	friend idTestFloat	operator+( const char i, const idTestFloat f ) { return idTestFloat( i + f.value ); }
	friend idTestFloat	operator-( const char i, const idTestFloat f ) { return idTestFloat( i - f.value ); }
	friend bool			operator<( const char i, const idTestFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const char i, const idTestFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const char i, const idTestFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const char i, const idTestFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const char i, const idTestFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const char i, const idTestFloat f ) { return ( i != f.value ); }

	idTestFloat			operator*( const byte i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator/( const byte i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator+( const byte i ) const { return idTestFloat( value * i ); }
	idTestFloat			operator-( const byte i ) const { return idTestFloat( value * i ); }
	bool				operator<( const byte i ) const { return ( value < i ); }
	bool				operator<=( const byte i ) const { return ( value <= i ); }
	bool				operator>( const byte i ) const { return ( value > i ); }
	bool				operator>=( const byte i ) const { return ( value >= i ); }
	bool				operator==( const byte i ) const { return ( value == i ); }
	bool				operator!=( const byte i ) const { return ( value != i ); }
	idTestFloat			operator*=( const byte i ) { value *= i; return *this; }
	idTestFloat			operator/=( const byte i ) { value /= i; return *this; }
	idTestFloat			operator+=( const byte i ) { value += i; return *this; }
	idTestFloat			operator-=( const byte i ) { value -= i; return *this; }
	friend idTestFloat	operator*( const byte i, const idTestFloat f ) { return idTestFloat( i * f.value ); }
	friend idTestFloat	operator/( const byte i, const idTestFloat f ) { return idTestFloat( i / f.value ); }
	friend idTestFloat	operator+( const byte i, const idTestFloat f ) { return idTestFloat( i + f.value ); }
	friend idTestFloat	operator-( const byte i, const idTestFloat f ) { return idTestFloat( i - f.value ); }
	friend bool			operator<( const byte i, const idTestFloat f ) { return ( i < f.value ); }
	friend bool			operator<=( const byte i, const idTestFloat f ) { return ( i <= f.value ); }
	friend bool			operator>( const byte i, const idTestFloat f ) { return ( i > f.value ); }
	friend bool			operator>=( const byte i, const idTestFloat f ) { return ( i >= f.value ); }
	friend bool			operator==( const byte i, const idTestFloat f ) { return ( i == f.value ); }
	friend bool			operator!=( const byte i, const idTestFloat f ) { return ( i != f.value ); }

	idTestFloat			operator*( const double d ) const { return idTestFloat( value * d ); }
	idTestFloat			operator/( const double d ) const { return idTestFloat( value * d ); }
	idTestFloat			operator+( const double d ) const { return idTestFloat( value * d ); }
	idTestFloat			operator-( const double d ) const { return idTestFloat( value * d ); }
	bool				operator<( const double d ) const { return ( value < d ); }
	bool				operator<=( const double d ) const { return ( value <= d ); }
	bool				operator>( const double d ) const { return ( value > d ); }
	bool				operator>=( const double d ) const { return ( value >= d ); }
	bool				operator==( const double d ) const { return ( value == d ); }
	bool				operator!=( const double d ) const { return ( value != d ); }
	idTestFloat			operator*=( const double d ) { value *= d; return *this; }
	idTestFloat			operator/=( const double d ) { value /= d; return *this; }
	idTestFloat			operator+=( const double d ) { value += d; return *this; }
	idTestFloat			operator-=( const double d ) { value -= d; return *this; }
	friend idTestFloat	operator*( const double d, const idTestFloat f ) { return idTestFloat( d * f.value ); }
	friend idTestFloat	operator/( const double d, const idTestFloat f ) { return idTestFloat( d / f.value ); }
	friend idTestFloat	operator+( const double d, const idTestFloat f ) { return idTestFloat( d + f.value ); }
	friend idTestFloat	operator-( const double d, const idTestFloat f ) { return idTestFloat( d - f.value ); }
	friend bool			operator<( const double d, const idTestFloat f ) { return ( d < f.value ); }
	friend bool			operator<=( const double d, const idTestFloat f ) { return ( d <= f.value ); }
	friend bool			operator>( const double d, const idTestFloat f ) { return ( d > f.value ); }
	friend bool			operator>=( const double d, const idTestFloat f ) { return ( d >= f.value ); }
	friend bool			operator==( const double d, const idTestFloat f ) { return ( d == f.value ); }
	friend bool			operator!=( const double d, const idTestFloat f ) { return ( d != f.value ); }

private:
						idTestFloat( double f ) { value = f; }
	operator			int ( void ) { }
	float				value;
};

inline float strtof( const char * str ) { return (float)atof( str ); }
#define atof strtof

#define union_float idUnionFloat
#define union_double idUnionFloat
#define float idTestFloat
#define double idTestFloat
#define C_FLOAT_TO_INT( x )		0.0f
#define C_FLOAT_TO_LONG( x )	0.0f

#else

#define union_float float
#define union_double double
#define C_FLOAT_TO_INT( x )		(int)(x)
#define C_FLOAT_TO_LONG( x )	(long)(x)

#endif

#endif	/* !__FLOATERRORS_H__ */
