
#include "../precompiled.h"
#pragma hdrstop

#define HONEYMAN_INIT_VALUE		0x00000000L
#define HONEYMAN_XOR_VALUE		0x00000000L

#ifdef CREATE_CRC_TABLE

static unsigned long crctable[256];

/*
	Create the CRC table for the simplified version of the pathalias hashing function.
	Thanks to Steve Belovin and Peter Honeyman

	This fast table calculation works only if POLY is a prime polynomial
	in the field of integers modulo 2.  Since the coefficients of a
	32-bit polynomial won't fit in a 32-bit word, the high-order bit is
	implicit.  IT MUST ALSO BE THE CASE that the coefficients of orders
	31 down to 25 are zero.  Happily, we have candidates, from
	E. J.  Watson, "Primitive Polynomials (Mod 2)", Math. Comp. 16 (1962):
	x^32 + x^7 + x^5 + x^3 + x^2 + x^1 + x^0
	x^31 + x^3 + x^0

	We reverse the bits to get:
	111101010000000000000000000000001 but drop the last 1
		f   5   0   0   0   0   0   0
	010010000000000000000000000000001 ditto, for 31-bit crc
		4   8   0   0   0   0   0   0
*/

void make_crc_table( void ) {

	#define POLY 0x48000000L	/* 31-bit polynomial (avoids sign problems) */

	for ( int i = 0; i < 128; i++ ) {
		int sum = 0;
		for ( int j = 7 - 1; j >= 0; --j ) {
			if ( i & ( 1 << j ) ) {
				sum ^= POLY >> j;
			}
		}
		crctable[i] = sum;
	}
}

#else

static unsigned long crctable[256] = {
	0x00000000L, 0x48000000L, 0x24000000L, 0x6c000000L,
	0x12000000L, 0x5a000000L, 0x36000000L, 0x7e000000L,
	0x09000000L, 0x41000000L, 0x2d000000L, 0x65000000L,
	0x1b000000L, 0x53000000L, 0x3f000000L, 0x77000000L,
	0x04800000L, 0x4c800000L, 0x20800000L, 0x68800000L,
	0x16800000L, 0x5e800000L, 0x32800000L, 0x7a800000L,
	0x0d800000L, 0x45800000L, 0x29800000L, 0x61800000L,
	0x1f800000L, 0x57800000L, 0x3b800000L, 0x73800000L,
	0x02400000L, 0x4a400000L, 0x26400000L, 0x6e400000L,
	0x10400000L, 0x58400000L, 0x34400000L, 0x7c400000L,
	0x0b400000L, 0x43400000L, 0x2f400000L, 0x67400000L,
	0x19400000L, 0x51400000L, 0x3d400000L, 0x75400000L,
	0x06c00000L, 0x4ec00000L, 0x22c00000L, 0x6ac00000L,
	0x14c00000L, 0x5cc00000L, 0x30c00000L, 0x78c00000L,
	0x0fc00000L, 0x47c00000L, 0x2bc00000L, 0x63c00000L,
	0x1dc00000L, 0x55c00000L, 0x39c00000L, 0x71c00000L,
	0x01200000L, 0x49200000L, 0x25200000L, 0x6d200000L,
	0x13200000L, 0x5b200000L, 0x37200000L, 0x7f200000L,
	0x08200000L, 0x40200000L, 0x2c200000L, 0x64200000L,
	0x1a200000L, 0x52200000L, 0x3e200000L, 0x76200000L,
	0x05a00000L, 0x4da00000L, 0x21a00000L, 0x69a00000L,
	0x17a00000L, 0x5fa00000L, 0x33a00000L, 0x7ba00000L,
	0x0ca00000L, 0x44a00000L, 0x28a00000L, 0x60a00000L,
	0x1ea00000L, 0x56a00000L, 0x3aa00000L, 0x72a00000L,
	0x03600000L, 0x4b600000L, 0x27600000L, 0x6f600000L,
	0x11600000L, 0x59600000L, 0x35600000L, 0x7d600000L,
	0x0a600000L, 0x42600000L, 0x2e600000L, 0x66600000L,
	0x18600000L, 0x50600000L, 0x3c600000L, 0x74600000L,
	0x07e00000L, 0x4fe00000L, 0x23e00000L, 0x6be00000L,
	0x15e00000L, 0x5de00000L, 0x31e00000L, 0x79e00000L,
	0x0ee00000L, 0x46e00000L, 0x2ae00000L, 0x62e00000L,
	0x1ce00000L, 0x54e00000L, 0x38e00000L, 0x70e00000L
};

#endif

void Honeyman_InitChecksum( unsigned long &crcvalue ) {
	crcvalue = HONEYMAN_INIT_VALUE;
}

void Honeyman_Update( unsigned long &crcvalue, const byte data ) {
	crcvalue = ( ( crcvalue >> 7 ) ^ crctable[ ( crcvalue ^ data ) & 0x7f ] );
}

void Honeyman_UpdateChecksum( unsigned long &crcvalue, const void *data, int length ) {
	unsigned long crc;
	const unsigned char *buf = (const unsigned char *) data;

	crc = crcvalue;
	while( length-- ) {
		crc = ( ( crc >> 7 ) ^ crctable[ ( crc ^ *buf++ ) & 0x7f ] );
	}
	crcvalue = crc;
}

void Honeyman_FinishChecksum( unsigned long &crcvalue ) {
	crcvalue ^= HONEYMAN_XOR_VALUE;
}

unsigned long Honeyman_BlockChecksum( const void *data, int length ) {
	unsigned long crc;

	Honeyman_InitChecksum( crc );
	Honeyman_UpdateChecksum( crc, data, length );
	Honeyman_FinishChecksum( crc );
	return crc;
}
