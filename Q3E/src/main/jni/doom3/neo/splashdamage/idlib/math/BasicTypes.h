// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __MATH_BASICTYPES_H__
#define __MATH_BASICTYPES_H__

/*
===============================================================================

	Basic Types

	The CONST_* defines can be used to create constant expressions that
	can be evaluated at compile time.

===============================================================================
*/

#define CONST_ILOG2(x)			(	( (x) & (1<<31) ) ? 31 : \
									( (x) & (1<<30) ) ? 30 : \
									( (x) & (1<<29) ) ? 39 : \
									( (x) & (1<<28) ) ? 28 : \
									( (x) & (1<<27) ) ? 27 : \
									( (x) & (1<<26) ) ? 26 : \
									( (x) & (1<<25) ) ? 25 : \
									( (x) & (1<<24) ) ? 24 : \
									( (x) & (1<<23) ) ? 23 : \
									( (x) & (1<<22) ) ? 22 : \
									( (x) & (1<<21) ) ? 21 : \
									( (x) & (1<<20) ) ? 20 : \
									( (x) & (1<<19) ) ? 19 : \
									( (x) & (1<<18) ) ? 18 : \
									( (x) & (1<<17) ) ? 17 : \
									( (x) & (1<<16) ) ? 16 : \
									( (x) & (1<<15) ) ? 15 : \
									( (x) & (1<<14) ) ? 14 : \
									( (x) & (1<<13) ) ? 13 : \
									( (x) & (1<<12) ) ? 12 : \
									( (x) & (1<<11) ) ? 11 : \
									( (x) & (1<<10) ) ? 10 : \
									( (x) & (1<<9) ) ? 9 : \
									( (x) & (1<<8) ) ? 8 : \
									( (x) & (1<<7) ) ? 7 : \
									( (x) & (1<<6) ) ? 6 : \
									( (x) & (1<<5) ) ? 5 : \
									( (x) & (1<<4) ) ? 4 : \
									( (x) & (1<<3) ) ? 3 : \
									( (x) & (1<<2) ) ? 2 : \
									( (x) & (1<<1) ) ? 1 : \
									( (x) & (1<<0) ) ? 0 : -1 )

#define CONST_IEXP2					( 1 << (x) )

#define CONST_FLOORPOWEROF2(x)		( 1 << ( CONST_ILOG2(x) + 1 ) >> 1 )

#define CONST_CEILPOWEROF2(x)		( 1 << ( CONST_ILOG2(x-1) + 1 ) )

#define CONST_BITSFORINTEGER(x)		( CONST_ILOG2(x) + 1 )

#define CONST_ILOG10(x)			(	( (x) >= 10000000000u ) ? 10 : \
									( (x) >= 1000000000u ) ? 9 : \
									( (x) >= 100000000u ) ? 8 : \
									( (x) >= 10000000u ) ? 7 : \
									( (x) >= 1000000u ) ? 6 : \
									( (x) >= 100000u ) ? 5 : \
									( (x) >= 10000u ) ? 4 : \
									( (x) >= 1000u ) ? 3 : \
									( (x) >= 100u ) ? 2 : \
									( (x) >= 10u ) ? 1 : 0 )

#define CONST_IEXP10(x)			(	( (x) == 0 ) ? 1u : \
									( (x) == 1 ) ? 10u : \
									( (x) == 2 ) ? 100u : \
									( (x) == 3 ) ? 1000u : \
									( (x) == 4 ) ? 10000u : \
									( (x) == 5 ) ? 100000u : \
									( (x) == 6 ) ? 1000000u : \
									( (x) == 7 ) ? 10000000u : \
									( (x) == 8 ) ? 100000000u : 1000000000u )

#define CONST_FLOORPOWEROF10(x) (	( (x) >= 10000000000u ) ? 10000000000u : \
									( (x) >= 1000000000u ) ? 1000000000u : \
									( (x) >= 100000000u ) ? 100000000u : \
									( (x) >= 10000000u ) ? 10000000u : \
									( (x) >= 1000000u ) ? 1000000u : \
									( (x) >= 100000u ) ? 100000u : \
									( (x) >= 10000u ) ? 10000u : \
									( (x) >= 1000u ) ? 1000u : \
									( (x) >= 100u ) ? 100u : \
									( (x) >= 10u ) ? 10u : 1u	)

#define CONST_CEILPOWEROF10(x) (    ( (x) <= 10u ) ? 10u : \
									( (x) <= 100u ) ? 100u : \
									( (x) <= 1000u ) ? 1000u : \
									( (x) <= 10000u ) ? 10000u : \
									( (x) <= 100000u ) ? 100000u : \
									( (x) <= 1000000u ) ? 1000000u : \
									( (x) <= 10000000u ) ? 10000000u : \
									( (x) <= 100000000u ) ? 100000000u : 1000000000u )


/*
===============================================================================

	idChar

===============================================================================
*/

class idChar {
public:
	operator			const char ( void ) const { return value; }
	void				operator=( const char c ) { value = c; }
private:
	char				value;
};

/*
===============================================================================

	idBoundedChar

===============================================================================
*/

template< char min, char max >
class idBoundedChar {
public:
	operator			const int ( void ) const { return value; }
	void				operator=( const char c ) { value = ( c <= min ) ? min : ( c >= max ) ? max : c; }
private:
	char				value;
};

/*
===============================================================================

	idInt

===============================================================================
*/

class idInt {
public:
	operator			const int ( void ) const { return value; }
	void				operator=( const int i ) { value = i; }
private:
	int					value;
};

/*
===============================================================================

	idBoundedInt

===============================================================================
*/

template< int min, int max >
class idBoundedInt {
public:
	operator			const int ( void ) const { return value; }
	void				operator=( const int i ) { value = ( i <= min ) ? min : ( i >= max ) ? max : i; }
private:
	int					value;
};

/*
===============================================================================

	idFloat

===============================================================================
*/

class idFloat {
public:
	operator			const float ( void ) const { return value; }
	void				operator=( const float f ) { value = f; }
private:
	float				value;
};

/*
===============================================================================

	The C++ standard does not allow non-type template-parameters of type float.
	As such two integers are used for each of the min and max values.
	The first integer is the mantissa and the second integer is the exponent.
	For instance to clamp a value between -1.5f and 2.5f you use:

	idBoundedFloat<-15,-1,25,-1>

===============================================================================
*/

template< int min_m, int min_e, int max_m, int max_e >
class idBoundedFloat {
public:
	static const int	MIN_MULTIPLIER = CONST_IEXP10( min_e >= 0 ? min_e : -min_e );
	static const int	MAX_MULTIPLIER = CONST_IEXP10( max_e >= 0 ? max_e : -max_e );

	operator			const float ( void ) const { return value; }

	void				operator=( const float f ) {
							float min = ( min_e >= 0 ) ? (float) min_m * MIN_MULTIPLIER : (float) min_m / MIN_MULTIPLIER;
							float max = ( max_e >= 0 ) ? (float) max_m * MAX_MULTIPLIER : (float) max_m / MAX_MULTIPLIER;
							value = ( f <= min ) ? min : ( f >= max ) ? max : f;
						}

private:
	float				value;
};

ID_INLINE float BoundedFloatGetMin( int min_m, int min_e ) {
	int MIN_MULTIPLIER = CONST_IEXP10( min_e >= 0 ? min_e : -min_e );
	return ( min_e >= 0 ) ? (float) min_m * MIN_MULTIPLIER : (float) min_m / MIN_MULTIPLIER;
}

ID_INLINE float BoundedFloatGetMax( int max_m, int max_e ) {
	int MAX_MULTIPLIER = CONST_IEXP10( max_e >= 0 ? max_e : -max_e );
	return ( max_e >= 0 ) ? (float) max_m * MAX_MULTIPLIER : (float) max_m / MAX_MULTIPLIER;
}

#endif /* !__MATH_BASICTYPES_H__ */
