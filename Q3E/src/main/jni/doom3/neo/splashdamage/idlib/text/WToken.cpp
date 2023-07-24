// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

/*
================
idWToken::NumberValue
================
*/
void idWToken::NumberValue( void ) {
	int i, pow, div, c;
	const wchar_t *p;
	double m;

	assert( type == TT_NUMBER );
	p = c_str();
	floatvalue = 0;
	intvalue = 0;
	// floating point number
	if ( subtype & TT_FLOAT ) {
		if ( subtype & ( TT_INFINITE | TT_INDEFINITE | TT_NAN ) ) {
			if ( subtype & TT_INFINITE ) {			// 1.#INF
				unsigned int inf = 0x7f800000;
				floatvalue = (double) *(float*)&inf;
			}
			else if ( subtype & TT_INDEFINITE ) {	// 1.#IND
				unsigned int ind = 0xffc00000;
				floatvalue = (double) *(float*)&ind;
			}
			else if ( subtype & TT_NAN ) {			// 1.#QNAN
				unsigned int nan = 0x7fc00000;
				floatvalue = (double) *(float*)&nan;
			}
		}
		else {
			while( *p && *p != L'.' && *p != L'e' ) {
				floatvalue = floatvalue * 10.0 + (double) (*p - L'0');
				p++;
			}
			if ( *p == L'.' ) {
				p++;
				for( m = 0.1f; *p && *p != L'e'; p++ ) {
					floatvalue = floatvalue + (double) (*p - L'0') * m;
					m *= 0.1;
				}
			}
			if ( *p == L'e' ) {
				p++;
				if ( *p == L'-' ) {
					div = true;
					p++;
				}
				else if ( *p == L'+' ) {
					div = false;
					p++;
				}
				else {
					div = false;
				}
				pow = 0;
				for ( pow = 0; *p; p++ ) {
					pow = pow * 10 + (int) (*p - L'0');
				}
				for ( m = 1.0, i = 0; i < pow; i++ ) {
					m *= 10.0;
				}
				if ( div ) {
					floatvalue /= m;
				}
				else {
					floatvalue *= m;
				}
			}
		}
		intvalue = idMath::Ftol( floatvalue );
	}
	else if ( subtype & TT_DECIMAL ) {
		while( *p ) {
			intvalue = intvalue * 10 + (*p - L'0');
			floatvalue = floatvalue * 10.0 + (double) (*p - L'0');
			p++;
		}
	}
	else if ( subtype & TT_IPADDRESS ) {
		c = 0;
		while( *p && *p != L':' ) {
			if ( *p == L'.' ) {
				while( c != 3 ) {
					intvalue = intvalue * 10;
					c++;
				}
				c = 0;
			}
			else {
				intvalue = intvalue * 10 + (*p - L'0');
				c++;
			}
			p++;
		}
		while( c != 3 ) {
			intvalue = intvalue * 10;
			c++;
		}
		floatvalue = intvalue;
	}
	else if ( subtype & TT_OCTAL ) {
		// step over the first zero
		p += 1;
		while( *p ) {
			intvalue = (intvalue << 3) + (*p - L'0');
			p++;
		}
		floatvalue = intvalue;
	}
	else if ( subtype & TT_HEX ) {
		// step over the leading 0x or 0X
		p += 2;
		while( *p ) {
			intvalue <<= 4;
			if (*p >= L'a' && *p <= L'f')
				intvalue += *p - L'a' + 10;
			else if (*p >= L'A' && *p <= L'F')
				intvalue += *p - L'A' + 10;
			else
				intvalue += *p - L'0';
			p++;
		}
		floatvalue = intvalue;
	}
	else if ( subtype & TT_BINARY ) {
		// step over the leading 0b or 0B
		p += 2;
		while( *p ) {
			intvalue = (intvalue << 1) + (*p - L'0');
			p++;
		}
		floatvalue = intvalue;
	}
	subtype |= TT_VALUESVALID;
}

/*
================
idWToken::ClearTokenWhiteSpace
================
*/
void idWToken::ClearTokenWhiteSpace( void ) {
	whiteSpaceStart_p = NULL;
	whiteSpaceEnd_p = NULL;
	linesCrossed = 0;
}
