/*
===========================================================================

DarklightNG Source Code
Copyright (C) 2026 - Justin Marshall(aka IceColdDuke).

This file is part of the DarklightNG GPL source code.
This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

DarklightNG is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

DarklightNG is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

===========================================================================
*/

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "MegaTexture.h"
#include "MegaTextureCodec.h"

#include <math.h>

static const byte jpegNaturalOrder[64] = {
	0, 1, 8, 16, 9, 2, 3, 10,
	17, 24, 32, 25, 18, 11, 4, 5,
	12, 19, 26, 33, 40, 48, 41, 34,
	27, 20, 13, 6, 7, 14, 21, 28,
	35, 42, 49, 56, 57, 50, 43, 36,
	29, 22, 15, 23, 30, 37, 44, 51,
	58, 59, 52, 45, 38, 31, 39, 46,
	53, 60, 61, 54, 47, 55, 62, 63
};

static const byte luminanceQuant[64] = {
	16, 11, 10, 16, 24, 40, 51, 61,
	12, 12, 14, 19, 26, 58, 60, 55,
	14, 13, 16, 24, 40, 57, 69, 56,
	14, 17, 22, 29, 51, 87, 80, 62,
	18, 22, 37, 56, 68, 109, 103, 77,
	24, 35, 55, 64, 81, 104, 113, 92,
	49, 64, 78, 87, 103, 121, 120, 101,
	72, 92, 95, 98, 112, 100, 103, 99
};

static const byte chrominanceQuant[64] = {
	17, 18, 24, 47, 99, 99, 99, 99,
	18, 21, 26, 66, 99, 99, 99, 99,
	24, 26, 56, 99, 99, 99, 99, 99,
	47, 66, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99,
	99, 99, 99, 99, 99, 99, 99, 99
};

static const byte bitsYDC[17] = { 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
static const byte valuesDC[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
static const byte bitsCoCgDC[17] = { 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
static const byte bitsYAC[17] = { 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 125 };
static const byte valuesYAC[162] = {
	0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61,0x07,
	0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,
	0x24,0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,
	0x29,0x2A,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,
	0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,
	0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
	0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,
	0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,
	0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xE1,0xE2,
	0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
	0xF9,0xFA
};
static const byte bitsCoCgAC[17] = { 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 119 };
static const byte valuesCoCgAC[162] = {
	0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71,
	0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,
	0x15,0x62,0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,
	0x27,0x28,0x29,0x2A,0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,
	0x49,0x4A,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,
	0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,
	0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0xA2,0xA3,0xA4,0xA5,
	0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xC2,0xC3,
	0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,
	0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,
	0xF9,0xFA
};

static int ClampByte( int value ) {
	return value < 0 ? 0 : ( value > 255 ? 255 : value );
}

idBareDCTHuffmanTable::idBareDCTHuffmanTable() {
	memset( this, 0, sizeof( *this ) );
}

void idBareDCTHuffmanTable::Init( const byte *bits, const byte *values ) {
	int code = 0;
	int valueIndex = 0;
	memset( minCode, -1, sizeof( minCode ) );
	memset( maxCode, -1, sizeof( maxCode ) );
	memset( valueOffset, 0, sizeof( valueOffset ) );
	memset( symbols, 0, sizeof( symbols ) );
	for ( int length = 1; length <= 16; ++length ) {
		if ( bits[length] ) {
			minCode[length] = code;
			valueOffset[length] = valueIndex - code;
			for ( int i = 0; i < bits[length]; ++i ) {
				symbols[valueIndex] = values[valueIndex];
				++valueIndex;
				++code;
			}
			maxCode[length] = code - 1;
		}
		code <<= 1;
	}
	maxCode[17] = 0x7fffffff;
}

idBareDctBase::idBareDctBase() {
	luminanceQuality = chrominanceQuality = alphaQuality = 75;
	InitHuffmanTable();
	InitQuantTable();
}

idBareDctBase::~idBareDctBase() {
}

int idBareDctBase::QuantizationScaleFromQuality( int quality ) {
	if ( quality <= 0 ) {
		return 5000;
	}
	if ( quality > 100 ) {
		quality = 100;
	}
	return quality < 50 ? 5000 / quality : 2 * ( 100 - quality );
}

void idBareDctBase::ScaleQuantTable( unsigned short *result, const byte *standard, int scale ) {
	for ( int i = 0; i < 64; ++i ) {
		int value = standard[i] * scale / 100;
		result[i] = (unsigned short)idMath::ClampInt( 1, 255, value );
	}
}

void idBareDctBase::InitHuffmanTable() {
	huffTableYDC.Init( bitsYDC, valuesDC );
	huffTableYAC.Init( bitsYAC, valuesYAC );
	huffTableCoCgDC.Init( bitsCoCgDC, valuesDC );
	huffTableCoCgAC.Init( bitsCoCgAC, valuesCoCgAC );
	huffTableADC.Init( bitsYDC, valuesDC );
	huffTableAAC.Init( bitsYAC, valuesYAC );
}

void idBareDctBase::InitQuantTable() {
	SetQuality_Generic( luminanceQuality, chrominanceQuality, alphaQuality );
}

void idBareDctBase::SetQuality( int y, int c, int a ) { SetQuality_Generic( y, c, a ); }
void idBareDctBase::SetQuality_MMX( int y, int c, int a ) { SetQuality_Generic( y, c, a ); }
void idBareDctBase::SetQuality_SSE2( int y, int c, int a ) { SetQuality_Generic( y, c, a ); }
void idBareDctBase::SetQuality_Xenon( int y, int c, int a ) { SetQuality_Generic( y, c, a ); }

void idBareDctBase::SetQuality_Generic( int y, int c, int a ) {
	luminanceQuality = y;
	chrominanceQuality = c;
	alphaQuality = a;
	ScaleQuantTable( quantTableY, luminanceQuant, QuantizationScaleFromQuality( y ) );
	ScaleQuantTable( quantTableCoCg, chrominanceQuant, QuantizationScaleFromQuality( c ) );
	ScaleQuantTable( quantTableA, luminanceQuant, QuantizationScaleFromQuality( a ) );
}

idBareDctDecoder::idBareDctDecoder() {
	data = NULL;
	dataBytes = dataOffset = 0;
	getBuff = 0;
	getBits = 0;
	bitError = false;
	dcY = dcCo = dcCg = dcA = 0;
}

idBareDctDecoder::~idBareDctDecoder() {
}

bool idBareDctDecoder::BeginImage( const byte *inBuf, int width, int height, int inputBytes ) {
	if ( !inBuf || width <= 0 || height <= 0 || inputBytes < 0 ) {
		return false;
	}
	data = inBuf;
	dataBytes = inputBytes;
	dataOffset = 0;
	getBuff = 0;
	getBits = 0;
	bitError = false;
	dcY = dcCo = dcCg = dcA = 0;
	return true;
}

bool idBareDctDecoder::FillBitBuffer( int count ) {
	while ( getBits < count ) {
		if ( dataOffset >= dataBytes ) {
			bitError = true;
			return false;
		}
		getBuff = ( getBuff << 8 ) | data[dataOffset++];
		getBits += 8;
	}
	return true;
}

int idBareDctDecoder::GetBits( int count ) {
	if ( count == 0 ) {
		return 0;
	}
	if ( !FillBitBuffer( count ) ) {
		return 0;
	}
	getBits -= count;
	return ( getBuff >> getBits ) & ( ( 1u << count ) - 1u );
}

int idBareDctDecoder::DecodeSymbol( const idBareDCTHuffmanTable &table ) {
	int code = 0;
	for ( int length = 1; length <= 16; ++length ) {
		code = ( code << 1 ) | GetBits( 1 );
		if ( bitError ) {
			return -1;
		}
		if ( table.maxCode[length] >= 0 && code <= table.maxCode[length] ) {
			const int index = code + table.valueOffset[length];
			return index >= 0 && index < 256 ? table.symbols[index] : -1;
		}
	}
	bitError = true;
	return -1;
}

int idBareDctDecoder::ValueFromCategory( int value, int category ) const {
	if ( category == 0 ) {
		return 0;
	}
	const int half = 1 << ( category - 1 );
	return value < half ? value + 1 - ( 1 << category ) : value;
}

bool idBareDctDecoder::HuffmanDecode( short *coefficients, const idBareDCTHuffmanTable &dcTable,
										const idBareDCTHuffmanTable &acTable, int &lastDC ) {
	memset( coefficients, 0, 64 * sizeof( coefficients[0] ) );
	const int dcCategory = DecodeSymbol( dcTable );
	if ( dcCategory < 0 || dcCategory > 16 ) {
		return false;
	}
	lastDC += ValueFromCategory( GetBits( dcCategory ), dcCategory );
	coefficients[0] = (short)lastDC;

	int coefficient = 1;
	while ( coefficient < 64 && !bitError ) {
		const int symbol = DecodeSymbol( acTable );
		if ( symbol < 0 ) {
			return false;
		}
		const int run = symbol >> 4;
		const int category = symbol & 15;
		if ( category == 0 ) {
			if ( run != 15 ) {
				break;
			}
			coefficient += 16;
			continue;
		}
		coefficient += run;
		if ( coefficient >= 64 ) {
			bitError = true;
			return false;
		}
		const int value = ValueFromCategory( GetBits( category ), category );
		coefficients[jpegNaturalOrder[coefficient]] = (short)value;
		++coefficient;
	}
	return !bitError;
}

void idBareDctDecoder::InverseDCT( const short *coefficients, const unsigned short *quant, short *output ) const {
	static bool initialized = false;
	static double basis[8][8];
	if ( !initialized ) {
		const double pi = 3.14159265358979323846;
		for ( int frequency = 0; frequency < 8; ++frequency ) {
			const double scale = frequency == 0 ? 0.7071067811865475244 : 1.0;
			for ( int position = 0; position < 8; ++position ) {
				basis[frequency][position] = scale * cos( ( ( 2 * position + 1 ) * frequency * pi ) / 16.0 );
			}
		}
		initialized = true;
	}

	for ( int y = 0; y < 8; ++y ) {
		for ( int x = 0; x < 8; ++x ) {
			double sum = 0.0;
			for ( int v = 0; v < 8; ++v ) {
				for ( int u = 0; u < 8; ++u ) {
					sum += basis[u][x] * basis[v][y] * coefficients[v * 8 + u] * quant[v * 8 + u];
				}
			}
			sum *= 0.25;
			output[y * 8 + x] = (short)( sum < 0.0 ? ceil( sum - 0.5 ) : floor( sum + 0.5 ) );
		}
	}
}

bool idBareDctDecoder::DecompressMacroBlock( short blocks[10][64], bool alpha ) {
	for ( int i = 0; i < 4; ++i ) {
		short coefficients[64];
		if ( !HuffmanDecode( coefficients, huffTableYDC, huffTableYAC, dcY ) ) {
			return false;
		}
		InverseDCT( coefficients, quantTableY, blocks[i] );
	}
	for ( int i = 0; i < 2; ++i ) {
		short coefficients[64];
		int &lastDC = i == 0 ? dcCo : dcCg;
		if ( !HuffmanDecode( coefficients, huffTableCoCgDC, huffTableCoCgAC, lastDC ) ) {
			return false;
		}
		InverseDCT( coefficients, quantTableCoCg, blocks[4 + i] );
	}
	if ( alpha ) {
		for ( int i = 0; i < 4; ++i ) {
			short coefficients[64];
			if ( !HuffmanDecode( coefficients, huffTableADC, huffTableAAC, dcA ) ) {
				return false;
			}
			InverseDCT( coefficients, quantTableA, blocks[6 + i] );
		}
	}
	return true;
}

void idBareDctDecoder::StoreRGBMacroBlock( short blocks[10][64], byte *output, int outputWidth,
											int originX, int originY, int width, int height,
											bool alpha, bool storeYCoCg ) {
	for ( int y = 0; y < 16 && originY + y < height; ++y ) {
		for ( int x = 0; x < 16 && originX + x < width; ++x ) {
			const int yBlock = ( y >= 8 ? 2 : 0 ) + ( x >= 8 ? 1 : 0 );
			const int yIndex = ( y & 7 ) * 8 + ( x & 7 );
			const int cIndex = ( y >> 1 ) * 8 + ( x >> 1 );
			const int yy = blocks[yBlock][yIndex] + 128;
			const int co = blocks[4][cIndex];
			const int cg = blocks[5][cIndex];
			byte *pixel = output + 4 * ( ( originY + y ) * outputWidth + originX + x );
			if ( storeYCoCg ) {
				pixel[0] = (byte)ClampByte( yy );
				pixel[1] = (byte)ClampByte( co + 128 );
				pixel[2] = (byte)ClampByte( cg + 128 );
			} else {
				pixel[0] = (byte)ClampByte( yy + co - cg );
				pixel[1] = (byte)ClampByte( yy + cg );
				pixel[2] = (byte)ClampByte( yy - co - cg );
			}
			pixel[3] = alpha ? (byte)ClampByte( blocks[6 + yBlock][yIndex] + 128 ) : 255;
		}
	}
}

bool idBareDctDecoder::DecompressColorImage( const byte *inBuf, byte *outBuf,
										int width, int height, int inputBytes, bool alpha, bool ycocg ) {
	if ( !BeginImage( inBuf, width, height, inputBytes ) ) {
		return false;
	}
	for ( int y = 0; y < height; y += 16 ) {
		for ( int x = 0; x < width; x += 16 ) {
			short blocks[10][64];
			if ( !DecompressMacroBlock( blocks, alpha ) ) {
				return false;
			}
			StoreRGBMacroBlock( blocks, outBuf, width, x, y, width, height, alpha, ycocg );
		}
	}
	return true;
}

bool idBareDctDecoder::DecompressImageRGB_Generic( const byte *in, byte *out, int w, int h, int bytes ) {
	return DecompressColorImage( in, out, w, h, bytes, false, false );
}
bool idBareDctDecoder::DecompressImageRGBA_Generic( const byte *in, byte *out, int w, int h, int bytes ) {
	return DecompressColorImage( in, out, w, h, bytes, true, false );
}
bool idBareDctDecoder::DecompressImageYCoCg_Generic( const byte *in, byte *out, int w, int h, int bytes ) {
	return DecompressColorImage( in, out, w, h, bytes, false, true );
}
bool idBareDctDecoder::DecompressImageRGB( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageRGB_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageRGB_MMX( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageRGB_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageRGB_SSE2( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageRGB_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageRGB_Xenon( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageRGB_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageRGBA( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageRGBA_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageRGBA_MMX( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageRGBA_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageRGBA_SSE2( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageRGBA_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageRGBA_Xenon( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageRGBA_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageYCoCg( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageYCoCg_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageYCoCg_MMX( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageYCoCg_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageYCoCg_SSE2( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageYCoCg_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressImageYCoCg_Xenon( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageYCoCg_Generic( a, b, c, d, e ); }

bool idBareDctDecoder::DecompressImageMono_Generic( const byte *in, byte *out, int width, int height, int bytes ) {
	if ( !BeginImage( in, width, height, bytes ) ) {
		return false;
	}
	for ( int by = 0; by < height; by += 8 ) {
		for ( int bx = 0; bx < width; bx += 8 ) {
			short coefficients[64], block[64];
			if ( !HuffmanDecode( coefficients, huffTableYDC, huffTableYAC, dcY ) ) {
				return false;
			}
			InverseDCT( coefficients, quantTableY, block );
			for ( int y = 0; y < 8 && by + y < height; ++y ) {
				for ( int x = 0; x < 8 && bx + x < width; ++x ) {
					out[( by + y ) * width + bx + x] = (byte)ClampByte( block[y * 8 + x] + 128 );
				}
			}
		}
	}
	return true;
}
bool idBareDctDecoder::DecompressImageMono( const byte *a, byte *b, int c, int d, int e ) { return DecompressImageMono_Generic( a, b, c, d, e ); }

bool idBareDctDecoder::DecompressLuminanceEnhancement_Generic( const byte *in, byte *out, int width, int height, int bytes ) {
	if ( !BeginImage( in, width, height, bytes ) ) {
		return false;
	}
	for ( int by = 0; by < height; by += 8 ) {
		for ( int bx = 0; bx < width; bx += 8 ) {
			short coefficients[64], block[64];
			if ( !HuffmanDecode( coefficients, huffTableYDC, huffTableYAC, dcY ) ) {
				return false;
			}
			InverseDCT( coefficients, quantTableY, block );
			for ( int y = 0; y < 8 && by + y < height; ++y ) {
				for ( int x = 0; x < 8 && bx + x < width; ++x ) {
					byte *pixel = out + 4 * ( ( by + y ) * width + bx + x );
					pixel[0] = (byte)ClampByte( pixel[0] + block[y * 8 + x] );
				}
			}
		}
	}
	return true;
}
bool idBareDctDecoder::DecompressLuminanceEnhancement( const byte *a, byte *b, int c, int d, int e ) { return DecompressLuminanceEnhancement_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressLuminanceEnhancement_MMX( const byte *a, byte *b, int c, int d, int e ) { return DecompressLuminanceEnhancement_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressLuminanceEnhancement_SSE2( const byte *a, byte *b, int c, int d, int e ) { return DecompressLuminanceEnhancement_Generic( a, b, c, d, e ); }
bool idBareDctDecoder::DecompressLuminanceEnhancement_Xenon( const byte *a, byte *b, int c, int d, int e ) { return DecompressLuminanceEnhancement_Generic( a, b, c, d, e ); }

idBareDctEncoder::idBareDctEncoder() :
	output( NULL ), capacity( 0 ), offset( 0 ), pendingByte( 0 ), pendingBits( 0 ), overflow( false ),
	dcY( 0 ), dcCo( 0 ), dcCg( 0 ), dcA( 0 ) {
	BuildCodeTable( codeYDC, bitsYDC, valuesDC );
	BuildCodeTable( codeYAC, bitsYAC, valuesYAC );
	BuildCodeTable( codeCoCgDC, bitsCoCgDC, valuesDC );
	BuildCodeTable( codeCoCgAC, bitsCoCgAC, valuesCoCgAC );
}

void idBareDctEncoder::BuildCodeTable( huffmanCode_t table[256], const byte *bits, const byte *values ) {
	memset( table, 0, sizeof( huffmanCode_t ) * 256 );
	unsigned int code = 0;
	int valueIndex = 0;
	for ( int length = 1; length <= 16; ++length ) {
		for ( int i = 0; i < bits[length]; ++i ) {
			table[values[valueIndex]].code = (unsigned short)code;
			table[values[valueIndex]].bits = (byte)length;
			++valueIndex;
			++code;
		}
		code <<= 1;
	}
}

int idBareDctEncoder::Category( int value ) {
	unsigned int magnitude = value < 0 ? (unsigned int)-value : (unsigned int)value;
	int category = 0;
	while ( magnitude ) {
		++category;
		magnitude >>= 1;
	}
	return category;
}

bool idBareDctEncoder::PutBits( unsigned int value, int count ) {
	for ( int bit = count - 1; bit >= 0; --bit ) {
		pendingByte = ( pendingByte << 1 ) | ( ( value >> bit ) & 1u );
		if ( ++pendingBits == 8 ) {
			if ( offset >= capacity ) {
				overflow = true;
				return false;
			}
			output[offset++] = (byte)pendingByte;
			pendingByte = 0;
			pendingBits = 0;
		}
	}
	return true;
}

bool idBareDctEncoder::FlushBits() {
	if ( pendingBits ) {
		pendingByte <<= 8 - pendingBits;
		if ( offset >= capacity ) {
			overflow = true;
			return false;
		}
		output[offset++] = (byte)pendingByte;
		pendingByte = 0;
		pendingBits = 0;
	}
	return !overflow;
}

void idBareDctEncoder::ForwardDCT( const short *input, const unsigned short *quant, short *coefficients ) const {
	static bool initialized = false;
	static double basis[8][8];
	if ( !initialized ) {
		const double pi = 3.14159265358979323846;
		for ( int frequency = 0; frequency < 8; ++frequency ) {
			const double scale = frequency == 0 ? 0.7071067811865475244 : 1.0;
			for ( int position = 0; position < 8; ++position ) {
				basis[frequency][position] = scale * cos( ( ( 2 * position + 1 ) * frequency * pi ) / 16.0 );
			}
		}
		initialized = true;
	}

	double horizontal[64];
	for ( int y = 0; y < 8; ++y ) {
		for ( int u = 0; u < 8; ++u ) {
			double sum = 0.0;
			for ( int x = 0; x < 8; ++x ) sum += input[y * 8 + x] * basis[u][x];
			horizontal[y * 8 + u] = sum;
		}
	}
	for ( int v = 0; v < 8; ++v ) {
		for ( int u = 0; u < 8; ++u ) {
			double sum = 0.0;
			for ( int y = 0; y < 8; ++y ) sum += horizontal[y * 8 + u] * basis[v][y];
			const double value = 0.25 * sum / quant[v * 8 + u];
			const int rounded = (int)( value < 0.0 ? ceil( value - 0.5 ) : floor( value + 0.5 ) );
			coefficients[v * 8 + u] = (short)idMath::ClampInt( -32767, 32767, rounded );
		}
	}
}

bool idBareDctEncoder::EncodeBlock( const short *input, const unsigned short *quant,
									const huffmanCode_t dcTable[256], const huffmanCode_t acTable[256], int &lastDC ) {
	short coefficients[64];
	ForwardDCT( input, quant, coefficients );
	const int difference = coefficients[0] - lastDC;
	lastDC = coefficients[0];
	const int dcCategory = Category( difference );
	if ( dcCategory > 11 || dcTable[dcCategory].bits == 0 ) return false;
	if ( !PutBits( dcTable[dcCategory].code, dcTable[dcCategory].bits ) ) return false;
	if ( dcCategory ) {
		const unsigned int encoded = difference < 0 ?
			(unsigned int)( difference + ( 1 << dcCategory ) - 1 ) : (unsigned int)difference;
		if ( !PutBits( encoded, dcCategory ) ) return false;
	}

	int zeroRun = 0;
	for ( int index = 1; index < 64; ++index ) {
		const int value = coefficients[jpegNaturalOrder[index]];
		if ( value == 0 ) {
			++zeroRun;
			continue;
		}
		while ( zeroRun >= 16 ) {
			const huffmanCode_t &zrl = acTable[0xF0];
			if ( zrl.bits == 0 || !PutBits( zrl.code, zrl.bits ) ) return false;
			zeroRun -= 16;
		}
		const int category = Category( value );
		if ( category > 15 ) return false;
		const int symbol = ( zeroRun << 4 ) | category;
		const huffmanCode_t &entry = acTable[symbol];
		if ( entry.bits == 0 || !PutBits( entry.code, entry.bits ) ) return false;
		const unsigned int encoded = value < 0 ?
			(unsigned int)( value + ( 1 << category ) - 1 ) : (unsigned int)value;
		if ( !PutBits( encoded, category ) ) return false;
		zeroRun = 0;
	}
	if ( zeroRun ) {
		const huffmanCode_t &eob = acTable[0];
		if ( eob.bits == 0 || !PutBits( eob.code, eob.bits ) ) return false;
	}
	return true;
}

bool idBareDctEncoder::CompressColorImage( const byte *inBuf, byte *outBuf, int width, int height,
									   int outputCapacity, int &outputBytes, bool alpha ) {
	outputBytes = 0;
	if ( !inBuf || !outBuf || width <= 0 || height <= 0 || outputCapacity <= 0 ) return false;
	output = outBuf;
	capacity = outputCapacity;
	offset = pendingBits = 0;
	pendingByte = 0;
	overflow = false;
	dcY = dcCo = dcCg = dcA = 0;

	for ( int originY = 0; originY < height; originY += 16 ) {
		for ( int originX = 0; originX < width; originX += 16 ) {
			short yBlocks[4][64];
			short coBlock[64];
			short cgBlock[64];
			short aBlocks[4][64];
			for ( int y = 0; y < 16; ++y ) {
				for ( int x = 0; x < 16; ++x ) {
					const int sx = idMath::ClampInt( 0, width - 1, originX + x );
					const int sy = idMath::ClampInt( 0, height - 1, originY + y );
					const byte *pixel = inBuf + 4 * ( sy * width + sx );
					const int block = ( y >= 8 ? 2 : 0 ) + ( x >= 8 ? 1 : 0 );
					const int blockIndex = ( y & 7 ) * 8 + ( x & 7 );
					const int yy = ( pixel[0] + 2 * pixel[1] + pixel[2] + 2 ) >> 2;
					yBlocks[block][blockIndex] = (short)( yy - 128 );
					aBlocks[block][blockIndex] = (short)( pixel[3] - 128 );
				}
			}
			for ( int y = 0; y < 8; ++y ) {
				for ( int x = 0; x < 8; ++x ) {
					int co = 0, cg = 0;
					for ( int oy = 0; oy < 2; ++oy ) for ( int ox = 0; ox < 2; ++ox ) {
						const int sx = idMath::ClampInt( 0, width - 1, originX + x * 2 + ox );
						const int sy = idMath::ClampInt( 0, height - 1, originY + y * 2 + oy );
						const byte *pixel = inBuf + 4 * ( sy * width + sx );
						co += ( (int)pixel[0] - pixel[2] ) >> 1;
						cg += ( -(int)pixel[0] + 2 * pixel[1] - pixel[2] + 2 ) >> 2;
					}
					coBlock[y * 8 + x] = (short)( ( co >= 0 ? co + 2 : co - 2 ) / 4 );
					cgBlock[y * 8 + x] = (short)( ( cg >= 0 ? cg + 2 : cg - 2 ) / 4 );
				}
			}
			for ( int block = 0; block < 4; ++block ) {
				if ( !EncodeBlock( yBlocks[block], quantTableY, codeYDC, codeYAC, dcY ) ) return false;
			}
			if ( !EncodeBlock( coBlock, quantTableCoCg, codeCoCgDC, codeCoCgAC, dcCo ) ||
				 !EncodeBlock( cgBlock, quantTableCoCg, codeCoCgDC, codeCoCgAC, dcCg ) ) return false;
			if ( alpha ) for ( int block = 0; block < 4; ++block ) {
				if ( !EncodeBlock( aBlocks[block], quantTableA, codeYDC, codeYAC, dcA ) ) return false;
			}
		}
	}
	if ( !FlushBits() ) return false;
	outputBytes = offset;
	return true;
}

bool idBareDctEncoder::CompressImageRGB( const byte *inBuf, byte *outBuf, int width, int height,
									 int outputCapacity, int &outputBytes ) {
	return CompressColorImage( inBuf, outBuf, width, height, outputCapacity, outputBytes, false );
}

bool idBareDctEncoder::CompressImageRGBA( const byte *inBuf, byte *outBuf, int width, int height,
									  int outputCapacity, int &outputBytes ) {
	return CompressColorImage( inBuf, outBuf, width, height, outputCapacity, outputBytes, true );
}

idDxtEncoder::idDxtEncoder() {
}

static unsigned short Pack565( int r, int g, int b ) {
	return (unsigned short)( ( ( r >> 3 ) << 11 ) | ( ( g >> 2 ) << 5 ) | ( b >> 3 ) );
}

static void Unpack565( unsigned short value, int color[3] ) {
	color[0] = ( ( value >> 11 ) & 31 ) * 255 / 31;
	color[1] = ( ( value >> 5 ) & 63 ) * 255 / 63;
	color[2] = ( value & 31 ) * 255 / 31;
}

void idDxtEncoder::CompressColorBlock( const byte *block, byte *out, bool allowTransparent ) const {
	int minColor[3] = { 255, 255, 255 };
	int maxColor[3] = { 0, 0, 0 };
	for ( int i = 0; i < 16; ++i ) {
		const byte *pixel = block + i * 4;
		for ( int c = 0; c < 3; ++c ) {
			minColor[c] = minColor[c] < (int)pixel[c] ? minColor[c] : (int)pixel[c];
			maxColor[c] = maxColor[c] > (int)pixel[c] ? maxColor[c] : (int)pixel[c];
		}
	}
	unsigned short color0 = Pack565( maxColor[0], maxColor[1], maxColor[2] );
	unsigned short color1 = Pack565( minColor[0], minColor[1], minColor[2] );
	if ( color0 <= color1 ) {
		unsigned short swap = color0;
		color0 = color1;
		color1 = swap;
	}
	out[0] = color0 & 255;
	out[1] = color0 >> 8;
	out[2] = color1 & 255;
	out[3] = color1 >> 8;
	int palette[4][3];
	Unpack565( color0, palette[0] );
	Unpack565( color1, palette[1] );
	for ( int c = 0; c < 3; ++c ) {
		palette[2][c] = ( 2 * palette[0][c] + palette[1][c] ) / 3;
		palette[3][c] = ( palette[0][c] + 2 * palette[1][c] ) / 3;
	}
	unsigned int indices = 0;
	for ( int i = 15; i >= 0; --i ) {
		const byte *pixel = block + i * 4;
		int best = 0;
		int bestError = 0x7fffffff;
		if ( allowTransparent && pixel[3] < 128 ) {
			best = 3;
		} else {
			for ( int p = 0; p < 4; ++p ) {
				const int dr = pixel[0] - palette[p][0];
				const int dg = pixel[1] - palette[p][1];
				const int db = pixel[2] - palette[p][2];
				const int error = dr * dr + dg * dg + db * db;
				if ( error < bestError ) {
					bestError = error;
					best = p;
				}
			}
		}
		indices = ( indices << 2 ) | best;
	}
	for ( int i = 0; i < 4; ++i ) {
		out[4 + i] = (byte)( indices >> ( 8 * i ) );
	}
}

void idDxtEncoder::CompressAlphaBlock( const byte *block, byte *out ) const {
	int minimum = 255;
	int maximum = 0;
	for ( int i = 0; i < 16; ++i ) {
		minimum = minimum < (int)block[i * 4 + 3] ? minimum : (int)block[i * 4 + 3];
		maximum = maximum > (int)block[i * 4 + 3] ? maximum : (int)block[i * 4 + 3];
	}
	out[0] = (byte)maximum;
	out[1] = (byte)minimum;
	int palette[8];
	palette[0] = maximum;
	palette[1] = minimum;
	for ( int i = 1; i <= 6; ++i ) {
		palette[i + 1] = ( ( 7 - i ) * maximum + i * minimum ) / 7;
	}
	unsigned long long bits = 0;
	for ( int i = 15; i >= 0; --i ) {
		int best = 0;
		int bestError = 0x7fffffff;
		for ( int p = 0; p < 8; ++p ) {
			const int error = idMath::Abs( (int)block[i * 4 + 3] - palette[p] );
			if ( error < bestError ) {
				bestError = error;
				best = p;
			}
		}
		bits = ( bits << 3 ) | best;
	}
	for ( int i = 0; i < 6; ++i ) {
		out[2 + i] = (byte)( bits >> ( 8 * i ) );
	}
}

static void GatherBlock( const byte *image, int width, int height, int blockX, int blockY, byte block[64] ) {
	for ( int y = 0; y < 4; ++y ) {
		const int sourceY = blockY * 4 + y < height - 1 ? blockY * 4 + y : height - 1;
		for ( int x = 0; x < 4; ++x ) {
			const int sourceX = blockX * 4 + x < width - 1 ? blockX * 4 + x : width - 1;
			memcpy( block + 4 * ( y * 4 + x ), image + 4 * ( sourceY * width + sourceX ), 4 );
		}
	}
}

bool idDxtEncoder::CompressImageDXT1Fast_Generic( const byte *in, byte *out, int width, int height, int &bytes ) {
	bytes = 0;
	byte block[64];
	for ( int y = 0; y < ( height + 3 ) / 4; ++y ) {
		for ( int x = 0; x < ( width + 3 ) / 4; ++x ) {
			GatherBlock( in, width, height, x, y, block );
			CompressColorBlock( block, out + bytes, false );
			bytes += 8;
		}
	}
	return true;
}

bool idDxtEncoder::CompressImageDXT5Fast_Generic( const byte *in, byte *out, int width, int height, int &bytes ) {
	bytes = 0;
	byte block[64];
	for ( int y = 0; y < ( height + 3 ) / 4; ++y ) {
		for ( int x = 0; x < ( width + 3 ) / 4; ++x ) {
			GatherBlock( in, width, height, x, y, block );
			CompressAlphaBlock( block, out + bytes );
			CompressColorBlock( block, out + bytes + 8, false );
			bytes += 16;
		}
	}
	return true;
}

bool idDxtEncoder::CompressImageDXT1Fast_MMX( const byte *a, byte *b, int c, int d, int &e ) { return CompressImageDXT1Fast_Generic( a, b, c, d, e ); }
bool idDxtEncoder::CompressImageDXT5Fast_MMX( const byte *a, byte *b, int c, int d, int &e ) { return CompressImageDXT5Fast_Generic( a, b, c, d, e ); }
bool idDxtEncoder::CompressImageDXT1Fast_SSE2( const byte *a, byte *b, int c, int d, int &e ) { return CompressImageDXT1Fast_Generic( a, b, c, d, e ); }
bool idDxtEncoder::CompressImageDXT5Fast_SSE2( const byte *a, byte *b, int c, int d, int &e ) { return CompressImageDXT5Fast_Generic( a, b, c, d, e ); }
bool idDxtEncoder::CompressImageDXT1Fast_Xenon( const byte *a, byte *b, int c, int d, int &e ) { return CompressImageDXT1Fast_Generic( a, b, c, d, e ); }
bool idDxtEncoder::CompressImageDXT5Fast_Xenon( const byte *a, byte *b, int c, int d, int &e ) { return CompressImageDXT5Fast_Generic( a, b, c, d, e ); }

void idMipMap::CreateMips( byte *data, int numLevels ) {
	int size = MEGA_TEXTURE_TILE_SIZE;
	byte *source = data;
	byte *destination = data + size * size * 4;
	for ( int level = 1; level < numLevels && size > 1; ++level ) {
		const int nextSize = size >> 1;
		for ( int y = 0; y < nextSize; ++y ) {
			for ( int x = 0; x < nextSize; ++x ) {
				for ( int c = 0; c < 4; ++c ) {
					const int a = source[4 * ( ( y * 2 ) * size + x * 2 ) + c];
					const int b = source[4 * ( ( y * 2 ) * size + x * 2 + 1 ) + c];
					const int d = source[4 * ( ( y * 2 + 1 ) * size + x * 2 ) + c];
					const int e = source[4 * ( ( y * 2 + 1 ) * size + x * 2 + 1 ) + c];
					destination[4 * ( y * nextSize + x ) + c] = (byte)( ( a + b + d + e ) >> 2 );
				}
			}
		}
		source = destination;
		destination += nextSize * nextSize * 4;
		size = nextSize;
	}
}

void idMipMap::CreateMips_MMX( byte *data, int levels ) { CreateMips( data, levels ); }
void idMipMap::CreateMips_SSE2( byte *data, int levels ) { CreateMips( data, levels ); }

int MegaTextureMipChainBytes() {
	int bytes = 0;
	for ( int size = MEGA_TEXTURE_TILE_SIZE; size >= MEGA_TEXTURE_MIN_MIP_SIZE; size >>= 1 ) {
		bytes += size * size * 4;
	}
	return bytes;
}

static float CubicWeight( float value ) {
	value = idMath::Fabs( value );
	if ( value <= 1.0f ) {
		return ( 1.5f * value - 2.5f ) * value * value + 1.0f;
	}
	if ( value < 2.0f ) {
		return ( ( -0.5f * value + 2.5f ) * value - 4.0f ) * value + 2.0f;
	}
	return 0.0f;
}

void MegaTextureUpscale2xBicubic( const byte *source, int width, int height, int stride, byte *destination ) {
	const int outputWidth = width * 2;
	const int outputHeight = height * 2;
	for ( int y = 0; y < outputHeight; ++y ) {
		const float sourceY = ( y + 0.5f ) * 0.5f - 0.5f;
		const int baseY = idMath::FtoiFast( floorf( sourceY ) );
		for ( int x = 0; x < outputWidth; ++x ) {
			const float sourceX = ( x + 0.5f ) * 0.5f - 0.5f;
			const int baseX = idMath::FtoiFast( floorf( sourceX ) );
			for ( int channel = 0; channel < 4; ++channel ) {
				float sum = 0.0f;
				float weightSum = 0.0f;
				for ( int oy = -1; oy <= 2; ++oy ) {
					const int sy = idMath::ClampInt( 0, height - 1, baseY + oy );
					const float wy = CubicWeight( sourceY - ( baseY + oy ) );
					for ( int ox = -1; ox <= 2; ++ox ) {
						const int sx = idMath::ClampInt( 0, width - 1, baseX + ox );
						const float weight = wy * CubicWeight( sourceX - ( baseX + ox ) );
						sum += source[sy * stride + sx * 4 + channel] * weight;
						weightSum += weight;
					}
				}
				destination[4 * ( y * outputWidth + x ) + channel] =
					(byte)ClampByte( idMath::FtoiFast( sum / weightSum + 0.5f ) );
			}
		}
	}
}

void MegaTextureConvertYCoCgToRGB( byte *image, int width, int height ) {
	for ( int i = 0; i < width * height; ++i ) {
		byte *pixel = image + i * 4;
		const int y = pixel[0];
		const int co = pixel[1] - 128;
		const int cg = pixel[2] - 128;
		pixel[0] = (byte)ClampByte( y + co - cg );
		pixel[1] = (byte)ClampByte( y + cg );
		pixel[2] = (byte)ClampByte( y - co - cg );
	}
}
