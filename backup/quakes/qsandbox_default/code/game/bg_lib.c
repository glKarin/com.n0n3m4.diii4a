#ifdef Q3_VM //karin: build native library
//
//
// bg_lib,c -- standard C library replacement routines used by code
// compiled for the virtual machine

#include "../qcommon/q_shared.h"

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "bg_lib.h"

#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)qsort.c	8.1 (Berkeley) 6/4/93";
#endif
static const char rcsid[] =
#endif /* LIBC_SCCS and not lint */

static char* med3(char *, char *, char *, cmp_t *);
static void	 swapfunc(char *, char *, int, int);

#ifndef min
#define min(a, b)	((a) < (b) ? (a) : (b))
#endif

/*
 * Qsort routine from Bentley & McIlroy's "Engineering a Sort Function".
 */
#define swapcode(TYPE, parmi, parmj, n) { 		\
	long i = (n) / sizeof (TYPE); 			\
	register TYPE *pi = (TYPE *) (parmi); 		\
	register TYPE *pj = (TYPE *) (parmj); 		\
	do { 						\
		register TYPE	t = *pi;		\
		*pi++ = *pj;				\
		*pj++ = t;				\
        } while (--i > 0);				\
}

#define SWAPINIT(a, es) swaptype = ((char *)a - (char *)0) % sizeof(long) || \
	es % sizeof(long) ? 2 : es == sizeof(long)? 0 : 1;

static void
swapfunc(a, b, n, swaptype)
	char *a, *b;
	int n, swaptype;
{
	if(swaptype <= 1)
		swapcode(long, a, b, n)
	else
		swapcode(char, a, b, n)
}

#define swap(a, b)					\
	if (swaptype == 0) {				\
		long t = *(long *)(a);			\
		*(long *)(a) = *(long *)(b);		\
		*(long *)(b) = t;			\
	} else						\
		swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n) 	if ((n) > 0) swapfunc(a, b, n, swaptype)

static char *
med3(a, b, c, cmp)
	char *a, *b, *c;
	cmp_t *cmp;
{
	return cmp(a, b) < 0 ?
	       (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a ))
              :(cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c ));
}

void
qsort(a, n, es, cmp)
	void *a;
	size_t n, es;
	cmp_t *cmp;
{
	char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
	int d, r, swaptype, swap_cnt;

loop:	SWAPINIT(a, es);
	swap_cnt = 0;
	if (n < 7) {
		for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
			for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}
	pm = (char *)a + (n / 2) * es;
	if (n > 7) {
		pl = a;
		pn = (char *)a + (n - 1) * es;
		if (n > 40) {
			d = (n / 8) * es;
			pl = med3(pl, pl + d, pl + 2 * d, cmp);
			pm = med3(pm - d, pm, pm + d, cmp);
			pn = med3(pn - 2 * d, pn - d, pn, cmp);
		}
		pm = med3(pl, pm, pn, cmp);
	}
	swap(a, pm);
	pa = pb = (char *)a + es;

	pc = pd = (char *)a + (n - 1) * es;
	for (;;) {
		while (pb <= pc && (r = cmp(pb, a)) <= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pa, pb);
				pa += es;
			}
			pb += es;
		}
		while (pb <= pc && (r = cmp(pc, a)) >= 0) {
			if (r == 0) {
				swap_cnt = 1;
				swap(pc, pd);
				pd -= es;
			}
			pc -= es;
		}
		if (pb > pc)
			break;
		swap(pb, pc);
		swap_cnt = 1;
		pb += es;
		pc -= es;
	}
	if (swap_cnt == 0) {  /* Switch to insertion sort */
		for (pm = (char *)a + es; pm < (char *)a + n * es; pm += es)
			for (pl = pm; pl > (char *)a && cmp(pl - es, pl) > 0;
			     pl -= es)
				swap(pl, pl - es);
		return;
	}

	pn = (char *)a + n * es;
	r = min(pa - (char *)a, pb - pa);
	vecswap(a, pb - r, r);
	r = min(pd - pc, pn - pd - es);
	vecswap(pb, pn - r, r);
	if ((r = pb - pa) > es)
		qsort(a, r / es, es, cmp);
	if ((r = pd - pc) > es) {
		/* Iterate rather than recurse to save stack space */
		a = pn - r;
		n = r / es;
		goto loop;
	}
/*		qsort(pn - r, r / es, es, cmp);*/
}

size_t strlen( const char *string ) {
	const char	*s;

	s = string;
	while ( *s ) {
		s++;
	}
	return s - string;
}

int strlenru( const char *string ) {
    int result = 0;
    int i = 0;
    
    while (string[i] != '\0') {
        unsigned char c = string[i];
        
        // Если символ ASCII (0-127), то учитываем 1 байт
        if (c <= 127) {
            result += 1;
            i += 1;
        }
        // Если символ в UTF-8 (русский, двухбайтовый)
        else if (c >= 192 && c <= 223) {  // Первый байт в диапазоне 0xC0 - 0xDF
            result += 1;  // Учитываем символ как 1 символ
            i += 2;  // Перемещаемся на 2 байта вперед
        } else {
            // Невозможно обработать этот символ, просто пропускаем его
            i += 1;
        }
    }

    return result;
}

int ifstrlenru( const char *string ) {
	int		result;
	int		n;
	int 	i;
	int		rucount;
	const char	*s;

	s = string;
	result = strlen(s);
	
	n = strlen(s);
	rucount = 0;
	
	for(i=0; i<n; i++){
	if(s[i] >= -64 && s[i] <= -1){	//ебаный unicode
	rucount += 1;
	}
	}
	return rucount;
}


char *strcat( char *strDestination, const char *strSource ) {
	char	*s;

	s = strDestination;
	while ( *s ) {
		s++;
	}
	while ( *strSource ) {
		*s++ = *strSource++;
	}
	*s = 0;
	return strDestination;
}

char *strcpy( char *strDestination, const char *strSource ) {
	char *s;

	s = strDestination;
	while ( *strSource ) {
		*s++ = *strSource++;
	}
	*s = 0;
	return strDestination;
}


int strcmp( const char *string1, const char *string2 ) {
	while ( *string1 == *string2 && *string1 && *string2 ) {
		string1++;
		string2++;
	}
	return *string1 - *string2;
}

int strcasecmp(const char *string1, const char *string2) {
    while (tolower((unsigned char)*string1) == tolower((unsigned char)*string2) && *string1 && *string2) {
        string1++;
        string2++;
    }
    return tolower((unsigned char)*string1) - tolower((unsigned char)*string2);
}

char *strchr( const char *string, int c ) {
	while ( *string ) {
		if ( *string == c ) {
			return ( char * )string;
		}
		string++;
	}
	if(c)
                return NULL;
        else
                return (char*) string;
}

char *strrchr(const char *string, int c)
{
	const char *found = NULL;
	
	while(*string)
	{
		if(*string == c)
			found = string;

		string++;
	}
	
	if(c)
		return (char *) found;
	else
		return (char *) string;
}

char *strstr( const char *string, const char *strCharSet ) {
	while ( *string ) {
		int		i;

		for ( i = 0 ; strCharSet[i] ; i++ ) {
			if ( string[i] != strCharSet[i] ) {
				break;
			}
		}
		if ( !strCharSet[i] ) {
			return (char *)string;
		}
		string++;
	}
	return (char *)0;
}

int tolower( int c ) {
	if ( c >= 'A' && c <= 'Z' ) {
		c += 'a' - 'A';
	}
	return c;
}


int toupper( int c ) {
	if ( c >= 'a' && c <= 'z' ) {
		c += 'A' - 'a';
	}
	return c;
}

void *memmove( void *dest, const void *src, size_t count ) {
	int		i;

	if ( dest > src ) {
		for ( i = count-1 ; i >= 0 ; i-- ) {
			((char *)dest)[i] = ((char *)src)[i];
		}
	} else {
		for ( i = 0 ; i < count ; i++ ) {
			((char *)dest)[i] = ((char *)src)[i];
		}
	}
	return dest;
}

double tan( double x ) {
	return sin(x) / cos(x);
}

static int randSeed = 0;

void	srand( unsigned seed ) {
	randSeed = seed;
}

int		rand( void ) {
	randSeed = (69069 * randSeed + 1);
	return randSeed & 0x7fff;
}

double atof( const char *string ) {
	float sign;
	float value;
	int		c;


	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	c = string[0];
	if ( c != '.' ) {
		do {
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value = value * 10 + c;
		} while ( 1 );
	} else {
		string++;
	}

	// check for decimal point
	if ( c == '.' ) {
		double fraction;

		fraction = 0.1;
		do {
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value += c * fraction;
			fraction *= 0.1;
		} while ( 1 );

	}

	// not handling 10e10 notation...

	return value * sign;
}

double _atof( const char **stringPtr ) {
	const char	*string;
	float sign;
	float value;
	int		c = '0';

	string = *stringPtr;

	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			*stringPtr = string;
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	if ( string[0] != '.' ) {
		do {
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value = value * 10 + c;
		} while ( 1 );
	}

	// check for decimal point
	if ( c == '.' ) {
		double fraction;

		fraction = 0.1;
		do {
			c = *string++;
			if ( c < '0' || c > '9' ) {
				break;
			}
			c -= '0';
			value += c * fraction;
			fraction *= 0.1;
		} while ( 1 );

	}

	// not handling 10e10 notation...
	*stringPtr = string;

	return value * sign;
}

/*
==============
strtod

Without an errno variable, this is a fair bit less useful than it is in libc
but it's still a fair bit more capable than atof or _atof
Handles inf[inity], nan (ignoring case), hexadecimals, and decimals
Handles decimal exponents like 10e10 and hex exponents like 0x7f8p20
10e10 == 10000000000 (power of ten)
0x7f8p20 == 0x7f800000 (decimal power of two)
The variable pointed to by endptr will hold the location of the first character
in the nptr string that was not used in the conversion
==============
*/
double strtod( const char *nptr, const char **endptr )
{
	double res;
	qboolean neg = qfalse;

	// skip whitespace
	while( isspace( *nptr ) )
		nptr++;

	// special string parsing
	if( Q_stricmpn( nptr, "nan", 3 ) == 0 )
	{
		floatint_t nan;
		if( endptr == NULL )
		{
			nan.ui = 0x7fffffff;
			return nan.f;
		}
		*endptr = &nptr[3];
		// nan can be followed by a bracketed number (in hex, octal,
		// or decimal) which is then put in the mantissa
		// this can be used to generate signalling or quiet NaNs, for
		// example (though I doubt it'll ever be used)
		// note that nan(0) is infinity!
		if( nptr[3] == '(' )
		{
			const char *end;
			int mantissa = strtol( &nptr[4], &end, 0 );
			if( *end == ')' )
			{
				nan.ui = 0x7f800000 | ( mantissa & 0x7fffff );
				if( endptr )
					*endptr = &end[1];
				return nan.f;
			}
		}
		nan.ui = 0x7fffffff;
		return nan.f;
	}
	if( Q_stricmpn( nptr, "inf", 3 ) == 0 )
	{
		floatint_t inf;
		inf.ui = 0x7f800000;
		if( endptr == NULL )
			return inf.f;
		if( Q_stricmpn( &nptr[3], "inity", 5 ) == 0 )
			*endptr = &nptr[8];
		else
			*endptr = &nptr[3];
		return inf.f;
	}

	// normal numeric parsing
	// sign
	if( *nptr == '-' )
	{
		nptr++;
		neg = qtrue;
	}
	else if( *nptr == '+' )
		nptr++;
	// hex
	if( Q_stricmpn( nptr, "0x", 2 ) == 0 )
	{
		// track if we use any digits
		const char *s = &nptr[1], *end = s;
		nptr += 2;
		res = 0;
		while( qtrue )
		{
			if( isdigit( *nptr ) )
				res = 16 * res + ( *nptr++ - '0' );
			else if( *nptr >= 'A' && *nptr <= 'F' )
				res = 16 * res + 10 + *nptr++ - 'A';
			else if( *nptr >= 'a' && *nptr <= 'f' )
				res = 16 * res + 10 + *nptr++ - 'a';
			else
				break;
		}
		// if nptr moved, save it
		if( end + 1 < nptr )
			end = nptr;
		if( *nptr == '.' )
		{
			float place;
			nptr++;
			// 1.0 / 16.0 == 0.0625
			// I don't expect the float accuracy to hold out for
			// very long but since we need to know the length of
			// the string anyway we keep on going regardless
			for( place = 0.0625;; place /= 16.0 )
			{
				if( isdigit( *nptr ) )
					res += place * ( *nptr++ - '0' );
				else if( *nptr >= 'A' && *nptr <= 'F' )
					res += place * ( 10 + *nptr++ - 'A' );
				else if( *nptr >= 'a' && *nptr <= 'f' )
					res += place * ( 10 + *nptr++ - 'a' );
				else
					break;
			}
			if( end < nptr )
				end = nptr;
		}
		// parse an optional exponent, representing multiplication
		// by a power of two
		// exponents are only valid if we encountered at least one
		// digit already (and have therefore set end to something)
		if( end != s && tolower( *nptr ) == 'p' )
		{
			int exp;
			float res2;
			// apparently (confusingly) the exponent should be
			// decimal
			exp = strtol( &nptr[1], &end, 10 );
			if( &nptr[1] == end )
			{
				// no exponent
				if( endptr )
					*endptr = nptr;
				return res;
			}
			if( exp > 0 )
			{
				while( exp-- > 0 )
				{
					res2 = res * 2;
					// check for infinity
					if( res2 <= res )
						break;
					res = res2;
				}
			}
			else
			{
				while( exp++ < 0 )
				{
					res2 = res / 2;
					// check for underflow
					if( res2 >= res )
						break;
					res = res2;
				}
			}
		}
		if( endptr )
			*endptr = end;
		return res;
	}
	// decimal
	else
	{
		// track if we find any digits
		const char *end = nptr, *p = nptr;
		// this is most of the work
		for( res = 0; isdigit( *nptr );
			res = 10 * res + *nptr++ - '0' );
		// if nptr moved, we read something
		if( end < nptr )
			end = nptr;
		if( *nptr == '.' )
		{
			// fractional part
			float place;
			nptr++;
			for( place = 0.1; isdigit( *nptr ); place /= 10.0 )
				res += ( *nptr++ - '0' ) * place;
			// if nptr moved, we read something
			if( end + 1 < nptr )
				end = nptr;
		}
		// exponent
		// meaningless without having already read digits, so check
		// we've set end to something
		if( p != end && tolower( *nptr ) == 'e' )
		{
			int exp;
			float res10;
			exp = strtol( &nptr[1], &end, 10 );
			if( &nptr[1] == end )
			{
				// no exponent
				if( endptr )
					*endptr = nptr;
				return res;
			}
			if( exp > 0 )
			{
				while( exp-- > 0 )
				{
					res10 = res * 10;
					// check for infinity to save us time
					if( res10 <= res )
						break;
					res = res10;
				}
			}
			else if( exp < 0 )
			{
				while( exp++ < 0 )
				{
					res10 = res / 10;
					// check for underflow
					// (test for 0 would probably be just
					// as good)
					if( res10 >= res )
						break;
					res = res10;
				}
			}
		}
		if( endptr )
			*endptr = end;
		return res;
	}
}

int atoi( const char *string ) {
	int		sign;
	int		value;
	int		c;


	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	do {
		c = *string++;
		if ( c < '0' || c > '9' ) {
			break;
		}
		c -= '0';
		value = value * 10 + c;
	} while ( 1 );

	// not handling 10e10 notation...

	return value * sign;
}


int _atoi( const char **stringPtr ) {
	int		sign;
	int		value;
	int		c;
	const char	*string;

	string = *stringPtr;

	// skip whitespace
	while ( *string <= ' ' ) {
		if ( !*string ) {
			return 0;
		}
		string++;
	}

	// check sign
	switch ( *string ) {
	case '+':
		string++;
		sign = 1;
		break;
	case '-':
		string++;
		sign = -1;
		break;
	default:
		sign = 1;
		break;
	}

	// read digits
	value = 0;
	do {
		c = *string++;
		if ( c < '0' || c > '9' ) {
			break;
		}
		c -= '0';
		value = value * 10 + c;
	} while ( 1 );

	// not handling 10e10 notation...

	*stringPtr = string;

	return value * sign;
}

/*
==============
strtol

Handles any base from 2 to 36. If base is 0 then it guesses
decimal, hex, or octal based on the format of the number (leading 0 or 0x)
Will not overflow - returns LONG_MIN or LONG_MAX as appropriate
*endptr is set to the location of the first character not used
==============
*/
long strtol( const char *nptr, const char **endptr, int base )
{
	long res;
	qboolean pos = qtrue;

	if( endptr )
		*endptr = nptr;
	// bases other than 0, 2, 8, 16 are very rarely used, but they're
	// not much extra effort to support
	if( base < 0 || base == 1 || base > 36 )
		return 0;
	// skip leading whitespace
	while( isspace( *nptr ) )
		nptr++;
	// sign
	if( *nptr == '-' )
	{
		nptr++;
		pos = qfalse;
	}
	else if( *nptr == '+' )
		nptr++;
	// look for base-identifying sequences e.g. 0x for hex, 0 for octal
	if( nptr[0] == '0' )
	{
		nptr++;
		// 0 is always a valid digit
		if( endptr )
			*endptr = nptr;
		if( *nptr == 'x' || *nptr == 'X' )
		{
			if( base != 0 && base != 16 )
			{
				// can't be hex, reject x (accept 0)
				if( endptr )
					*endptr = nptr;
				return 0;
			}
			nptr++;
			base = 16;
		}
		else if( base == 0 )
			base = 8;
	}
	else if( base == 0 )
		base = 10;
	res = 0;
	while( qtrue )
	{
		int val;
		if( isdigit( *nptr ) )
			val = *nptr - '0';
		else if( islower( *nptr ) )
			val = 10 + *nptr - 'a';
		else if( isupper( *nptr ) )
			val = 10 + *nptr - 'A';
		else
			break;
		if( val >= base )
			break;
		// we go negative because LONG_MIN is further from 0 than
		// LONG_MAX
		if( res < ( LONG_MIN + val ) / base )
			res = LONG_MIN; // overflow
		else
			res = res * base - val;
		nptr++;
		if( endptr )
			*endptr = nptr;
	}
	if( pos )
	{
		// can't represent LONG_MIN positive
		if( res == LONG_MIN )
			res = LONG_MAX;
		else
			res = -res;
	}
	return res;
}

int abs( int n ) {
	return n < 0 ? -n : n;
}

double fabs( double x ) {
	return x < 0 ? -x : x;
}

/* 
 * New implementation by Patrick Powell and others for vsnprintf.
 * Supports length checking in strings.
*/

/*
 * Copyright Patrick Powell 1995
 * This code is based on code written by Patrick Powell (papowell@astart.com)
 * It may be used for any purpose as long as this notice remains intact
 * on all source code distributions
 */

/**************************************************************
 * Original:
 * Patrick Powell Tue Apr 11 09:48:21 PDT 1995
 * A bombproof version of doprnt (dopr) included.
 * Sigh.  This sort of thing is always nasty do deal with.  Note that
 * the version here does not include floating point...
 *
 * snprintf() is used instead of sprintf() as it does limit checks
 * for string length.  This covers a nasty loophole.
 *
 * The other functions are there to prevent NULL pointers from
 * causing nast effects.
 *
 * More Recently:
 *  Brandon Long <blong@fiction.net> 9/15/96 for mutt 0.43
 *  This was ugly.  It is still ugly.  I opted out of floating point
 *  numbers, but the formatter understands just about everything
 *  from the normal C string format, at least as far as I can tell from
 *  the Solaris 2.5 printf(3S) man page.
 *
 *  Brandon Long <blong@fiction.net> 10/22/97 for mutt 0.87.1
 *    Ok, added some minimal floating point support, which means this
 *    probably requires libm on most operating systems.  Don't yet
 *    support the exponent (e,E) and sigfig (g,G).  Also, fmtint()
 *    was pretty badly broken, it just wasn't being exercised in ways
 *    which showed it, so that's been fixed.  Also, formated the code
 *    to mutt conventions, and removed dead code left over from the
 *    original.  Also, there is now a builtin-test, just compile with:
 *           gcc -DTEST_SNPRINTF -o snprintf snprintf.c -lm
 *    and run snprintf for results.
 * 
 *  Thomas Roessler <roessler@guug.de> 01/27/98 for mutt 0.89i
 *    The PGP code was using unsigned hexadecimal formats. 
 *    Unfortunately, unsigned formats simply didn't work.
 *
 *  Michael Elkins <me@cs.hmc.edu> 03/05/98 for mutt 0.90.8
 *    The original code assumed that both snprintf() and vsnprintf() were
 *    missing.  Some systems only have snprintf() but not vsnprintf(), so
 *    the code is now broken down under HAVE_SNPRINTF and HAVE_VSNPRINTF.
 *
 *  Andrew Tridgell (tridge@samba.org) Oct 1998
 *    fixed handling of %.0f
 *    added test for HAVE_LONG_DOUBLE
 *
 *  Russ Allbery <rra@stanford.edu> 2000-08-26
 *    fixed return value to comply with C99
 *    fixed handling of snprintf(NULL, ...)
 *
 *  Hrvoje Niksic <hniksic@arsdigita.com> 2000-11-04
 *    include <config.h> instead of "config.h".
 *    moved TEST_SNPRINTF stuff out of HAVE_SNPRINTF ifdef.
 *    include <stdio.h> for NULL.
 *    added support and test cases for long long.
 *    don't declare argument types to (v)snprintf if stdarg is not used.
 *    use int instead of short int as 2nd arg to va_arg.
 *
 **************************************************************/

/* BDR 2002-01-13  %e and %g were being ignored.  Now do something,
   if not necessarily correctly */

#if (SIZEOF_LONG_DOUBLE > 0)
/* #ifdef HAVE_LONG_DOUBLE */
#define LDOUBLE long double
#else
#define LDOUBLE double
#endif

#if (SIZEOF_LONG_LONG > 0)
/* #ifdef HAVE_LONG_LONG */
# define LLONG long long
#else
# define LLONG long
#endif

static int dopr (char *buffer, size_t maxlen, const char *format, 
                 va_list args);
static int fmtstr (char *buffer, size_t *currlen, size_t maxlen,
		   char *value, int flags, int min, int max);
static int fmtint (char *buffer, size_t *currlen, size_t maxlen,
		   LLONG value, int base, int min, int max, int flags);
static int fmtfp (char *buffer, size_t *currlen, size_t maxlen,
		  LDOUBLE fvalue, int min, int max, int flags);
static int dopr_outch (char *buffer, size_t *currlen, size_t maxlen, char c );

/*
 * dopr(): poor man's version of doprintf
 */

/* format read states */
#define DP_S_DEFAULT 0
#define DP_S_FLAGS   1
#define DP_S_MIN     2
#define DP_S_DOT     3
#define DP_S_MAX     4
#define DP_S_MOD     5
#define DP_S_MOD_L   6
#define DP_S_CONV    7
#define DP_S_DONE    8

/* format flags - Bits */
#define DP_F_MINUS 	(1 << 0)
#define DP_F_PLUS  	(1 << 1)
#define DP_F_SPACE 	(1 << 2)
#define DP_F_NUM   	(1 << 3)
#define DP_F_ZERO  	(1 << 4)
#define DP_F_UP    	(1 << 5)
#define DP_F_UNSIGNED 	(1 << 6)

/* Conversion Flags */
#define DP_C_SHORT   1
#define DP_C_LONG    2
#define DP_C_LLONG   3
#define DP_C_LDOUBLE 4

#define char_to_int(p) (p - '0')
#define MAX(p,q) ((p >= q) ? p : q)
#define MIN(p,q) ((p <= q) ? p : q)

static int dopr (char *buffer, size_t maxlen, const char *format, va_list args)
{
  char ch;
  LLONG value;
  LDOUBLE fvalue;
  char *strvalue;
  int min;
  int max;
  int state;
  int flags;
  int cflags;
  int total;
  size_t currlen;
  
  state = DP_S_DEFAULT;
  currlen = flags = cflags = min = 0;
  max = -1;
  ch = *format++;
  total = 0;

  while (state != DP_S_DONE)
  {
    if (ch == '\0')
      state = DP_S_DONE;

    switch(state) 
    {
    case DP_S_DEFAULT:
      if (ch == '%') 
	state = DP_S_FLAGS;
      else 
	total += dopr_outch (buffer, &currlen, maxlen, ch);
      ch = *format++;
      break;
    case DP_S_FLAGS:
      switch (ch) 
      {
      case '-':
	flags |= DP_F_MINUS;
        ch = *format++;
	break;
      case '+':
	flags |= DP_F_PLUS;
        ch = *format++;
	break;
      case ' ':
	flags |= DP_F_SPACE;
        ch = *format++;
	break;
      case '#':
	flags |= DP_F_NUM;
        ch = *format++;
	break;
      case '0':
	flags |= DP_F_ZERO;
        ch = *format++;
	break;
      default:
	state = DP_S_MIN;
	break;
      }
      break;
    case DP_S_MIN:
      if ('0' <= ch && ch <= '9')
      {
	min = 10*min + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	min = va_arg (args, int);
	ch = *format++;
	state = DP_S_DOT;
      } 
      else 
	state = DP_S_DOT;
      break;
    case DP_S_DOT:
      if (ch == '.') 
      {
	state = DP_S_MAX;
	ch = *format++;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MAX:
      if ('0' <= ch && ch <= '9')
      {
	if (max < 0)
	  max = 0;
	max = 10*max + char_to_int (ch);
	ch = *format++;
      } 
      else if (ch == '*') 
      {
	max = va_arg (args, int);
	ch = *format++;
	state = DP_S_MOD;
      } 
      else 
	state = DP_S_MOD;
      break;
    case DP_S_MOD:
      switch (ch) 
      {
      case 'h':
	cflags = DP_C_SHORT;
	ch = *format++;
	break;
      case 'l':
	cflags = DP_C_LONG;
	ch = *format++;
	break;
      case 'L':
	cflags = DP_C_LDOUBLE;
	ch = *format++;
	break;
      default:
	break;
      }
      if (cflags != DP_C_LONG)
	state = DP_S_CONV;
      else
	state = DP_S_MOD_L;
      break;
    case DP_S_MOD_L:
      switch (ch)
	{
	case 'l':
	  cflags = DP_C_LLONG;
	  ch = *format++;
	  break;
	default:
	  break;
	}
      state = DP_S_CONV;
      break;
    case DP_S_CONV:
      switch (ch) 
      {
      case 'd':
      case 'i':
	if (cflags == DP_C_SHORT) 
	  value = (short int)va_arg (args, int);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, long int);
	else if (cflags == DP_C_LLONG)
	  value = va_arg (args, LLONG);
	else
	  value = va_arg (args, int);
	total += fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'o':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
//	  value = (unsigned short int) va_arg (args, unsigned short int); // Thilo: This does not work because the rcc compiler cannot do that cast correctly.
	  value = va_arg (args, unsigned int) & ( (1 << sizeof(unsigned short int) * 8) - 1); // Using this workaround instead.
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else if (cflags == DP_C_LLONG)
	  value = va_arg (args, unsigned LLONG);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 8, min, max, flags);
	break;
      case 'u':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
	  value = va_arg (args, unsigned int) & ( (1 << sizeof(unsigned short int) * 8) - 1);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else if (cflags == DP_C_LLONG)
	  value = va_arg (args, unsigned LLONG);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 10, min, max, flags);
	break;
      case 'X':
	flags |= DP_F_UP;
      case 'x':
	flags |= DP_F_UNSIGNED;
	if (cflags == DP_C_SHORT)
	  value = va_arg (args, unsigned int) & ( (1 << sizeof(unsigned short int) * 8) - 1);
	else if (cflags == DP_C_LONG)
	  value = va_arg (args, unsigned long int);
	else if (cflags == DP_C_LLONG)
	  value = va_arg (args, unsigned LLONG);
	else
	  value = va_arg (args, unsigned int);
	total += fmtint (buffer, &currlen, maxlen, value, 16, min, max, flags);
	break;
      case 'f':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	/* um, floating point? */
	total += fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'E':
	flags |= DP_F_UP;
      case 'e':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	/* um, floating point? */
	total += fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'G':
	flags |= DP_F_UP;
      case 'g':
	if (cflags == DP_C_LDOUBLE)
	  fvalue = va_arg (args, LDOUBLE);
	else
	  fvalue = va_arg (args, double);
	/* um, floating point? */
	total += fmtfp (buffer, &currlen, maxlen, fvalue, min, max, flags);
	break;
      case 'c':
	total += dopr_outch (buffer, &currlen, maxlen, va_arg (args, int));
	break;
      case 's':
	strvalue = va_arg (args, char *);
	total += fmtstr (buffer, &currlen, maxlen, strvalue, flags, min, max);
	break;
      case 'p':
	strvalue = va_arg (args, void *);
	total += fmtint (buffer, &currlen, maxlen, (long) strvalue, 16, min,
                         max, flags);
	break;
      case 'n':
	if (cflags == DP_C_SHORT) 
	{
	  short int *num;
	  num = va_arg (args, short int *);
	  *num = currlen;
        }
	else if (cflags == DP_C_LONG) 
	{
	  long int *num;
	  num = va_arg (args, long int *);
	  *num = currlen;
        } 
	else if (cflags == DP_C_LLONG) 
	{
	  LLONG *num;
	  num = va_arg (args, LLONG *);
	  *num = currlen;
        } 
	else 
	{
	  int *num;
	  num = va_arg (args, int *);
	  *num = currlen;
        }
	break;
      case '%':
	total += dopr_outch (buffer, &currlen, maxlen, ch);
	break;
      case 'w':
	/* not supported yet, treat as next char */
	ch = *format++;
	break;
      default:
	/* Unknown, skip */
	break;
      }
      ch = *format++;
      state = DP_S_DEFAULT;
      flags = cflags = min = 0;
      max = -1;
      break;
    case DP_S_DONE:
      break;
    default:
      /* hmm? */
      break; /* some picky compilers need this */
    }
  }
  if (buffer != NULL)
  {
    if (currlen < maxlen - 1) 
      buffer[currlen] = '\0';
    else 
      buffer[maxlen - 1] = '\0';
  }
  return total;
}

static int fmtstr (char *buffer, size_t *currlen, size_t maxlen,
                   char *value, int flags, int min, int max)
{
  int padlen, strln;     /* amount to pad */
  int cnt = 0;
  int total = 0;
  
  if (value == 0)
  {
    value = "<NULL>";
  }

  for (strln = 0; value[strln]; ++strln); /* strlen */
  if (max >= 0 && max < strln)
    strln = max;
  padlen = min - strln;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justify */

  while (padlen > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
  }
  while (*value && ((max < 0) || (cnt < max)))
  {
    total += dopr_outch (buffer, currlen, maxlen, *value++);
    ++cnt;
  }
  while (padlen < 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
  }
  return total;
}

/* Have to handle DP_F_NUM (ie 0x and 0 alternates) */

static int fmtint (char *buffer, size_t *currlen, size_t maxlen,
		   LLONG value, int base, int min, int max, int flags)
{
  int signvalue = 0;
  unsigned LLONG uvalue;
  char convert[24];
  int place = 0;
  int spadlen = 0; /* amount to space pad */
  int zpadlen = 0; /* amount to zero pad */
  const char *digits;
  int total = 0;
  
  if (max < 0)
    max = 0;

  uvalue = value;

  if(!(flags & DP_F_UNSIGNED))
  {
    if( value < 0 ) {
      signvalue = '-';
      uvalue = -value;
    }
    else
      if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
	signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';
  }
  
  if (flags & DP_F_UP)
    /* Should characters be upper case? */
    digits = "0123456789ABCDEF";
  else
    digits = "0123456789abcdef";

  do {
    convert[place++] = digits[uvalue % (unsigned)base];
    uvalue = (uvalue / (unsigned)base );
  } while(uvalue && (place < sizeof (convert)));
  if (place == sizeof (convert)) place--;
  convert[place] = 0;

  zpadlen = max - place;
  spadlen = min - MAX (max, place) - (signvalue ? 1 : 0);
  if (zpadlen < 0) zpadlen = 0;
  if (spadlen < 0) spadlen = 0;
  if (flags & DP_F_ZERO)
  {
    zpadlen = MAX(zpadlen, spadlen);
    spadlen = 0;
  }
  if (flags & DP_F_MINUS) 
    spadlen = -spadlen; /* Left Justifty */

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "zpad: %d, spad: %d, min: %d, max: %d, place: %d\n",
      zpadlen, spadlen, min, max, place));
#endif

  /* Spaces */
  while (spadlen > 0) 
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --spadlen;
  }

  /* Sign */
  if (signvalue) 
    total += dopr_outch (buffer, currlen, maxlen, signvalue);

  /* Zeros */
  if (zpadlen > 0) 
  {
    while (zpadlen > 0)
    {
      total += dopr_outch (buffer, currlen, maxlen, '0');
      --zpadlen;
    }
  }

  /* Digits */
  while (place > 0) 
    total += dopr_outch (buffer, currlen, maxlen, convert[--place]);
  
  /* Left Justified spaces */
  while (spadlen < 0) {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++spadlen;
  }

  return total;
}

static LDOUBLE abs_val (LDOUBLE value)
{
  LDOUBLE result = value;

  if (value < 0)
    result = -value;

  return result;
}

static LDOUBLE pow10 (int exp)
{
  LDOUBLE result = 1;

  while (exp)
  {
    result *= 10;
    exp--;
  }
  
  return result;
}

static long round (LDOUBLE value)
{
  long intpart;

  intpart = value;
  value = value - intpart;
  if (value >= 0.5)
    intpart++;

  return intpart;
}

static int fmtfp (char *buffer, size_t *currlen, size_t maxlen,
		  LDOUBLE fvalue, int min, int max, int flags)
{
  int signvalue = 0;
  LDOUBLE ufvalue;
  char iconvert[20];
  char fconvert[20];
  int iplace = 0;
  int fplace = 0;
  int padlen = 0; /* amount to pad */
  int zpadlen = 0; 
  int caps = 0;
  int total = 0;
  long intpart;
  long fracpart;
  
  /* 
   * AIX manpage says the default is 0, but Solaris says the default
   * is 6, and sprintf on AIX defaults to 6
   */
  if (max < 0)
    max = 6;

  ufvalue = abs_val (fvalue);

  if (fvalue < 0)
    signvalue = '-';
  else
    if (flags & DP_F_PLUS)  /* Do a sign (+/i) */
      signvalue = '+';
    else
      if (flags & DP_F_SPACE)
	signvalue = ' ';

#if 0
  if (flags & DP_F_UP) caps = 1; /* Should characters be upper case? */
#endif

  intpart = ufvalue;

  /* 
   * Sorry, we only support 9 digits past the decimal because of our 
   * conversion method
   */
  if (max > 9)
    max = 9;

  /* We "cheat" by converting the fractional part to integer by
   * multiplying by a factor of 10
   */
  fracpart = round ((pow10 (max)) * (ufvalue - intpart));

  if (fracpart >= pow10 (max))
  {
    intpart++;
    fracpart -= pow10 (max);
  }

#ifdef DEBUG_SNPRINTF
  dprint (1, (debugfile, "fmtfp: %f =? %d.%d\n", fvalue, intpart, fracpart));
#endif

  /* Convert integer part */
  do {
    iconvert[iplace++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")[intpart % 10];
    intpart = (intpart / 10);
  } while(intpart && (iplace < 20));
  if (iplace == 20) iplace--;
  iconvert[iplace] = 0;

  /* Convert fractional part */
  do {
    fconvert[fplace++] =
      (caps? "0123456789ABCDEF":"0123456789abcdef")[fracpart % 10];
    fracpart = (fracpart / 10);
  } while(fracpart && (fplace < 20));
  if (fplace == 20) fplace--;
  fconvert[fplace] = 0;

  /* -1 for decimal point, another -1 if we are printing a sign */
  padlen = min - iplace - max - 1 - ((signvalue) ? 1 : 0); 
  zpadlen = max - fplace;
  if (zpadlen < 0)
    zpadlen = 0;
  if (padlen < 0) 
    padlen = 0;
  if (flags & DP_F_MINUS) 
    padlen = -padlen; /* Left Justifty */

  if ((flags & DP_F_ZERO) && (padlen > 0)) 
  {
    if (signvalue) 
    {
      total += dopr_outch (buffer, currlen, maxlen, signvalue);
      --padlen;
      signvalue = 0;
    }
    while (padlen > 0)
    {
      total += dopr_outch (buffer, currlen, maxlen, '0');
      --padlen;
    }
  }
  while (padlen > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    --padlen;
  }
  if (signvalue) 
    total += dopr_outch (buffer, currlen, maxlen, signvalue);

  while (iplace > 0) 
    total += dopr_outch (buffer, currlen, maxlen, iconvert[--iplace]);

  /*
   * Decimal point.  This should probably use locale to find the correct
   * char to print out.
   */
  if (max > 0)
  {
    total += dopr_outch (buffer, currlen, maxlen, '.');

    while (zpadlen-- > 0)
      total += dopr_outch (buffer, currlen, maxlen, '0');

    while (fplace > 0) 
      total += dopr_outch (buffer, currlen, maxlen, fconvert[--fplace]);
  }

  while (padlen < 0) 
  {
    total += dopr_outch (buffer, currlen, maxlen, ' ');
    ++padlen;
  }

  return total;
}

static int dopr_outch (char *buffer, size_t *currlen, size_t maxlen, char c)
{
  if (*currlen + 1 < maxlen)
    buffer[(*currlen)++] = c;
  return 1;
}

int Q_vsnprintf(char *str, size_t length, const char *fmt, va_list args)
{
	if (str != NULL)
		str[0] = 0;
	return dopr(str, length, fmt, args);
}

int Q_snprintf(char *str, size_t length, const char *fmt, ...)
{
	va_list ap;
	int retval;

	va_start(ap, fmt);
	retval = Q_vsnprintf(str, length, fmt, ap);
	va_end(ap);
	
	return retval;
}

/* this is really crappy */
int sscanf( const char *buffer, const char *fmt, ... ) {
	int		cmd;
	va_list		ap;
	int		count;
	size_t		len;

	va_start (ap, fmt);
	count = 0;

	while ( *fmt ) {
		if ( fmt[0] != '%' ) {
			fmt++;
			continue;
		}

		fmt++;
		cmd = *fmt;

		if (isdigit (cmd)) {
			len = (size_t)_atoi (&fmt);
			cmd = *(fmt - 1);
		} else {
			len = MAX_STRING_CHARS - 1;
			fmt++;
		}

		switch ( cmd ) {
		case 'i':
		case 'd':
		case 'u':
			*(va_arg (ap, int *)) = _atoi( &buffer );
			break;
		case 'f':
			*(va_arg (ap, float *)) = _atof( &buffer );
			break;
		case 's':
			{
			char *s = va_arg (ap, char *);
			while (isspace (*buffer))
				buffer++;
			while (*buffer && !isspace (*buffer) && len-- > 0 )
				*s++ = *buffer++;
			*s++ = '\0';
			break;
			}
		}
	}

	va_end (ap);
	return count;
}
#else //karin: build native library
#include <string.h>

int strlenru( const char *string ) {
    int result = 0;
    int i = 0;
    
    while (string[i] != '\0') {
        unsigned char c = string[i];
        
        // Если символ ASCII (0-127), то учитываем 1 байт
        if (c <= 127) {
            result += 1;
            i += 1;
        }
        // Если символ в UTF-8 (русский, двухбайтовый)
        else if (c >= 192 && c <= 223) {  // Первый байт в диапазоне 0xC0 - 0xDF
            result += 1;  // Учитываем символ как 1 символ
            i += 2;  // Перемещаемся на 2 байта вперед
        } else {
            // Невозможно обработать этот символ, просто пропускаем его
            i += 1;
        }
    }

    return result;
}

int ifstrlenru( const char *string ) {
	int		result;
	int		n;
	int 	i;
	int		rucount;
	const char	*s;

	s = string;
	result = strlen(s);
	
	n = strlen(s);
	rucount = 0;
	
	for(i=0; i<n; i++){
	if(s[i] >= -64 && s[i] <= -1){	//ебаный unicode
	rucount += 1;
	}
	}
	return rucount;
}


#endif

