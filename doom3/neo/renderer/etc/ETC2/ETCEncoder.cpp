/*
===========================================================================

openQ4 ETC2 / EAC encoder.

===========================================================================
*/

#include "../../idlib/precompiled.h"
#include "ETCCodec.h"

/*
================================================================================================

Block layout, for reading the bit twiddling below against the ES 3.0 spec.

An ETC colour block is 64 bits, written most significant byte first. Bit 63 is
the MSB of the first byte out.

	differential ( diff = 1 )         individual ( diff = 0 )
	63..59  R base, 5 bits            63..60  R sub-block 1, 4 bits
	58..56  R delta, 3 bits signed    59..56  R sub-block 2, 4 bits
	55..51  G base                    55..52  G sub-block 1
	50..48  G delta                   51..48  G sub-block 2
	47..43  B base                    47..44  B sub-block 1
	42..40  B delta                   43..40  B sub-block 2
	39..37  table index, sub-block 1
	36..34  table index, sub-block 2
	33      diff
	32      flip
	31..16  pixel index MSB plane
	15..0   pixel index LSB plane

A pixel at column x, row y takes plane bit p = x * 4 + y, so the planes are
column major. Its 2-bit index is ( MSB << 1 ) | LSB, and the decoder adds
etcModifierTable[table][index] to the sub-block's base colour.

flip = 0 splits the block into two 2x4 halves, left ( x < 2 ) and right.
flip = 1 splits it into two 4x2 halves, top ( y < 2 ) and bottom.

An EAC block is also 64 bits:

	63..56  base codeword
	55..52  multiplier
	51..48  table index
	47..0   sixteen 3-bit indices, pixel x,y at shift 45 - 3 * ( x * 4 + y )

decoded = clamp( base + eacModifierTable[table][index] * multiplier )

That is the 8-bit decode, used for the alpha half of ETC2_RGBA8. EAC_RG11 packs
two of the same 64-bit blocks per 4x4 -- R then G -- but decodes them to 11 bits:

	decoded11 = clamp( ( base + eacModifierTable[table][index] * multiplier ) * 8 + 4, 0, 2047 )

which is the 8-bit result rescaled: v/255 versus ( v * 8 + 4 )/2047, equal to
within half a step of 1/2047. So the same integer search in 8-bit space produces
the right bits for both, and EtcCompressEacBlock serves both formats. The only
asymmetry is the +4 rounding bias, which costs a normal component at most
0.002 -- far below the quantisation the block indices impose anyway.

================================================================================================
*/

static const int etcModifierTable[8][4] = {
	{  2,   8,   -2,   -8 },
	{  5,  17,   -5,  -17 },
	{  9,  29,   -9,  -29 },
	{ 13,  42,  -13,  -42 },
	{ 18,  60,  -18,  -60 },
	{ 24,  80,  -24,  -80 },
	{ 33, 106,  -33, -106 },
	{ 47, 183,  -47, -183 }
};

static const int eacModifierTable[16][8] = {
	{ -3, -6,  -9, -15, 2, 5, 8, 14 },
	{ -3, -7, -10, -13, 2, 6, 9, 12 },
	{ -2, -5,  -8, -13, 1, 4, 7, 12 },
	{ -2, -4,  -6, -13, 1, 3, 5, 12 },
	{ -3, -6,  -8, -12, 2, 5, 7, 11 },
	{ -3, -7,  -9, -11, 2, 6, 8, 10 },
	{ -4, -7,  -8, -11, 3, 6, 7, 10 },
	{ -3, -5,  -8, -11, 2, 4, 7, 10 },
	{ -2, -6,  -8, -10, 1, 5, 7,  9 },
	{ -2, -5,  -8, -10, 1, 4, 7,  9 },
	{ -2, -4,  -8, -10, 1, 3, 7,  9 },
	{ -2, -5,  -7, -10, 1, 4, 6,  9 },
	{ -3, -4,  -7, -10, 2, 3, 6,  9 },
	{ -1, -2,  -3, -10, 0, 1, 2,  9 },
	{ -4, -6,  -8,  -9, 3, 5, 7,  8 },
	{ -3, -5,  -7,  -9, 2, 4, 6,  8 }
};

// eacModifierTable[13][4] is the only entry that is exactly zero, which is what
// lets a constant-alpha block reproduce its value with no error at all
static const int EAC_CONSTANT_TABLE = 13;
static const int EAC_CONSTANT_INDEX = 4;

// larger than any squared error this encoder can accumulate: 16 pixels x 3
// channels x 255^2 is about 3.1M, so a 31-bit sentinel can never be beaten
// by a real block
static const int ETC_ERROR_SENTINEL = 0x7FFFFFFF;

static ID_INLINE int EtcClamp255( int v ) {
	return ( v < 0 ) ? 0 : ( ( v > 255 ) ? 255 : v );
}

/*
========================
EtcExtend4To8 / EtcExtend5To8

Bit replication, matching how the decoder expands the stored base colours.
========================
*/
static ID_INLINE int EtcExtend4To8( int v ) {
	return ( v << 4 ) | v;
}

static ID_INLINE int EtcExtend5To8( int v ) {
	return ( v << 3 ) | ( v >> 2 );
}

/*
================================================================================================

	colour blocks

================================================================================================
*/

// one 4x4 block of RGBA, unpacked from the source image
struct etcBlock_t {
	byte	color[16][4];
};

/*
========================
EtcGatherBlock

Reads a 4x4 block out of the source image. The caller guarantees the image is
already padded to a multiple of four, which is what idBinaryImage does before it
calls any block compressor.
========================
*/
static void EtcGatherBlock( const byte *inBuf, int width, int blockX, int blockY, etcBlock_t &block ) {
	for ( int y = 0; y < 4; y++ ) {
		const byte *row = inBuf + ( ( (size_t)blockY + y ) * (size_t)width + blockX ) * 4;
		for ( int x = 0; x < 4; x++ ) {
			// column major, to match the plane bit order
			byte *dst = block.color[ x * 4 + y ];
			dst[0] = row[ x * 4 + 0 ];
			dst[1] = row[ x * 4 + 1 ];
			dst[2] = row[ x * 4 + 2 ];
			dst[3] = row[ x * 4 + 3 ];
		}
	}
}

// the eight plane-bit positions belonging to each half, for both flips
static const int etcSubBlockPixels[2][2][8] = {
	// flip 0: left ( x 0,1 ) then right ( x 2,3 )
	{ { 0, 1, 2, 3, 4, 5, 6, 7 }, { 8, 9, 10, 11, 12, 13, 14, 15 } },
	// flip 1: top ( y 0,1 ) then bottom ( y 2,3 )
	{ { 0, 1, 4, 5, 8, 9, 12, 13 }, { 2, 3, 6, 7, 10, 11, 14, 15 } }
};

/*
========================
EtcSubBlockAverage
========================
*/
static void EtcSubBlockAverage( const etcBlock_t &block, const int *pixels, int average[3] ) {
	int sum[3] = { 0, 0, 0 };
	for ( int i = 0; i < 8; i++ ) {
		const byte *c = block.color[ pixels[i] ];
		sum[0] += c[0];
		sum[1] += c[1];
		sum[2] += c[2];
	}
	average[0] = ( sum[0] + 4 ) >> 3;
	average[1] = ( sum[1] + 4 ) >> 3;
	average[2] = ( sum[2] + 4 ) >> 3;
}

/*
========================
EtcEvaluateSubBlock

Given a decoded base colour, finds the modifier table and the per-pixel indices
that reproduce these eight pixels most closely, and returns the squared error.

The index choice is exhaustive over all four modifiers of the chosen table, and
the table choice is exhaustive over all eight, which is cheap enough at this
size and removes a whole class of "close enough" artefacts.
========================
*/
static int EtcEvaluateSubBlock( const etcBlock_t &block, const int *pixels, const int base[3],
		int &bestTable, int outIndices[8] ) {
	int bestError = ETC_ERROR_SENTINEL;
	bestTable = 0;

	int indices[8];
	for ( int table = 0; table < 8; table++ ) {
		int tableError = 0;
		for ( int i = 0; i < 8; i++ ) {
			const byte *c = block.color[ pixels[i] ];
			int pixelBest = ETC_ERROR_SENTINEL;
			int pixelIndex = 0;
			for ( int index = 0; index < 4; index++ ) {
				const int modifier = etcModifierTable[ table ][ index ];
				const int dr = EtcClamp255( base[0] + modifier ) - c[0];
				const int dg = EtcClamp255( base[1] + modifier ) - c[1];
				const int db = EtcClamp255( base[2] + modifier ) - c[2];
				const int error = dr * dr + dg * dg + db * db;
				if ( error < pixelBest ) {
					pixelBest = error;
					pixelIndex = index;
				}
			}
			indices[i] = pixelIndex;
			tableError += pixelBest;
			if ( tableError >= bestError ) {
				break;
			}
		}
		if ( tableError < bestError ) {
			bestError = tableError;
			bestTable = table;
			memcpy( outIndices, indices, sizeof( indices ) );
		}
	}

	return bestError;
}

// everything needed to write one candidate encoding of a block
struct etcCandidate_t {
	int		error;
	int		flip;
	int		diff;
	int		base[2][3];		// per sub-block, in stored ( 4 or 5 bit ) space
	int		table[2];
	int		indices[2][8];
};

/*
========================
EtcTryFlip

Evaluates one flip, in both differential and individual mode, and keeps the
better of the two in the candidate.

Differential mode stores sub-block 1 at 5 bits and sub-block 2 as a 3-bit signed
delta from it, so the two halves are not independent: the delta only reaches
-4..3. When the halves are further apart than that the mode simply cannot be
used, and individual mode's 4-bit-per-channel bases are the fallback. That is
the one real quality compromise in an ETC1-subset encoder, and it is why the
error of both modes is measured rather than assumed.
========================
*/
static void EtcTryFlip( const etcBlock_t &block, int flip, etcCandidate_t &best ) {
	int average[2][3];
	EtcSubBlockAverage( block, etcSubBlockPixels[ flip ][ 0 ], average[0] );
	EtcSubBlockAverage( block, etcSubBlockPixels[ flip ][ 1 ], average[1] );

	// --- differential ---
	int base5[2][3];
	bool deltaFits = true;
	for ( int half = 0; half < 2; half++ ) {
		for ( int c = 0; c < 3; c++ ) {
			base5[ half ][ c ] = ( average[ half ][ c ] * 31 + 127 ) / 255;
		}
	}
	for ( int c = 0; c < 3; c++ ) {
		const int delta = base5[1][c] - base5[0][c];
		if ( delta < -4 || delta > 3 ) {
			deltaFits = false;
			break;
		}
	}

	if ( deltaFits ) {
		etcCandidate_t candidate;
		candidate.error = 0;
		candidate.flip = flip;
		candidate.diff = 1;
		for ( int half = 0; half < 2; half++ ) {
			int decoded[3];
			for ( int c = 0; c < 3; c++ ) {
				candidate.base[ half ][ c ] = base5[ half ][ c ];
				decoded[ c ] = EtcExtend5To8( base5[ half ][ c ] );
			}
			candidate.error += EtcEvaluateSubBlock( block, etcSubBlockPixels[ flip ][ half ],
				decoded, candidate.table[ half ], candidate.indices[ half ] );
		}
		if ( candidate.error < best.error ) {
			best = candidate;
		}
	}

	// --- individual ---
	{
		etcCandidate_t candidate;
		candidate.error = 0;
		candidate.flip = flip;
		candidate.diff = 0;
		for ( int half = 0; half < 2; half++ ) {
			int decoded[3];
			for ( int c = 0; c < 3; c++ ) {
				const int quantized = ( average[ half ][ c ] * 15 + 127 ) / 255;
				candidate.base[ half ][ c ] = quantized;
				decoded[ c ] = EtcExtend4To8( quantized );
			}
			candidate.error += EtcEvaluateSubBlock( block, etcSubBlockPixels[ flip ][ half ],
				decoded, candidate.table[ half ], candidate.indices[ half ] );
		}
		if ( candidate.error < best.error ) {
			best = candidate;
		}
	}
}

/*
========================
EtcWriteColorBlock
========================
*/
static void EtcWriteColorBlock( const etcCandidate_t &candidate, byte *outBuf ) {
	uint64_t bits = 0;

	if ( candidate.diff ) {
		for ( int c = 0; c < 3; c++ ) {
			const int base = candidate.base[0][c];
			const int delta = candidate.base[1][c] - base;
			const int shift = 59 - c * 8;
			bits |= ( uint64_t )( base & 0x1F ) << shift;
			bits |= ( uint64_t )( delta & 0x07 ) << ( shift - 3 );
		}
	} else {
		for ( int c = 0; c < 3; c++ ) {
			const int shift = 60 - c * 8;
			bits |= ( uint64_t )( candidate.base[0][c] & 0x0F ) << shift;
			bits |= ( uint64_t )( candidate.base[1][c] & 0x0F ) << ( shift - 4 );
		}
	}

	bits |= ( uint64_t )( candidate.table[0] & 0x07 ) << 37;
	bits |= ( uint64_t )( candidate.table[1] & 0x07 ) << 34;
	bits |= ( uint64_t )( candidate.diff & 1 ) << 33;
	bits |= ( uint64_t )( candidate.flip & 1 ) << 32;

	for ( int half = 0; half < 2; half++ ) {
		const int *pixels = etcSubBlockPixels[ candidate.flip ][ half ];
		for ( int i = 0; i < 8; i++ ) {
			const int plane = pixels[i];
			const int index = candidate.indices[ half ][ i ];
			bits |= ( uint64_t )( index & 1 ) << plane;
			bits |= ( uint64_t )( ( index >> 1 ) & 1 ) << ( 16 + plane );
		}
	}

	for ( int i = 0; i < 8; i++ ) {
		outBuf[i] = ( byte )( ( bits >> ( 56 - i * 8 ) ) & 0xFF );
	}
}

/*
========================
EtcCompressColorBlock
========================
*/
static void EtcCompressColorBlock( const etcBlock_t &block, byte *outBuf ) {
	etcCandidate_t best;
	best.error = ETC_ERROR_SENTINEL;
	best.flip = 0;
	best.diff = 0;
	memset( best.base, 0, sizeof( best.base ) );
	memset( best.table, 0, sizeof( best.table ) );
	memset( best.indices, 0, sizeof( best.indices ) );

	EtcTryFlip( block, 0, best );
	EtcTryFlip( block, 1, best );

	EtcWriteColorBlock( best, outBuf );
}

/*
================================================================================================

	alpha blocks

================================================================================================
*/

/*
========================
EtcCompressEacBlock

EAC, one channel of the block into one 64-bit unit. Searches every table, and
for each one only the multipliers that could plausibly span the block's range,
which keeps this far cheaper than the full 16 x 15 x 256 space without
measurably costing quality on the mostly flat alpha Quake 4's art carries.

channel selects the source: 3 for the alpha half of ETC2_RGBA8, 0 and 1 for the
X and Y halves of EAC_RG11. See the format note at the top for why one 8-bit
search covers the 11-bit decode as well.
========================
*/
static void EtcCompressEacBlock( const etcBlock_t &block, int channel, byte *outBuf ) {
	int alphaMin = 255;
	int alphaMax = 0;
	for ( int i = 0; i < 16; i++ ) {
		const int a = block.color[i][ channel ];
		alphaMin = Min( alphaMin, a );
		alphaMax = Max( alphaMax, a );
	}

	int bestBase = alphaMin;
	int bestMultiplier = 1;
	int bestTable = EAC_CONSTANT_TABLE;
	int bestIndices[16];
	int bestError = ETC_ERROR_SENTINEL;

	if ( alphaMin == alphaMax ) {
		// exact, via the one modifier in the tables that is zero
		for ( int i = 0; i < 16; i++ ) {
			bestIndices[i] = EAC_CONSTANT_INDEX;
		}
		bestError = 0;
	} else {
		const int range = alphaMax - alphaMin;
		for ( int table = 0; table < 16; table++ ) {
			int tableMin = eacModifierTable[ table ][0];
			int tableMax = eacModifierTable[ table ][0];
			for ( int i = 1; i < 8; i++ ) {
				tableMin = Min( tableMin, eacModifierTable[ table ][i] );
				tableMax = Max( tableMax, eacModifierTable[ table ][i] );
			}
			const int tableRange = tableMax - tableMin;
			if ( tableRange <= 0 ) {
				continue;
			}

			const int centerMultiplier = Max( 1, ( range + tableRange - 1 ) / tableRange );
			for ( int m = Max( 1, centerMultiplier - 1 ); m <= Min( 15, centerMultiplier + 1 ); m++ ) {
				// place the base so the table's reach straddles the block's range
				int base = alphaMin - tableMin * m;
				base = Max( 0, Min( 255, base ) );

				int indices[16];
				int error = 0;
				for ( int i = 0; i < 16; i++ ) {
					const int a = block.color[i][ channel ];
					int pixelBest = ETC_ERROR_SENTINEL;
					int pixelIndex = 0;
					for ( int index = 0; index < 8; index++ ) {
						const int decoded = EtcClamp255( base + eacModifierTable[ table ][ index ] * m );
						const int d = decoded - a;
						const int e = d * d;
						if ( e < pixelBest ) {
							pixelBest = e;
							pixelIndex = index;
						}
					}
					indices[i] = pixelIndex;
					error += pixelBest;
					if ( error >= bestError ) {
						break;
					}
				}

				if ( error < bestError ) {
					bestError = error;
					bestBase = base;
					bestMultiplier = m;
					bestTable = table;
					memcpy( bestIndices, indices, sizeof( indices ) );
				}
			}
		}
	}

	uint64_t bits = 0;
	bits |= ( uint64_t )( bestBase & 0xFF ) << 56;
	bits |= ( uint64_t )( bestMultiplier & 0x0F ) << 52;
	bits |= ( uint64_t )( bestTable & 0x0F ) << 48;
	for ( int i = 0; i < 16; i++ ) {
		bits |= ( uint64_t )( bestIndices[i] & 0x07 ) << ( 45 - i * 3 );
	}

	for ( int i = 0; i < 8; i++ ) {
		outBuf[i] = ( byte )( ( bits >> ( 56 - i * 8 ) ) & 0xFF );
	}
}

/*
================================================================================================

	entry points

================================================================================================
*/

/*
========================
idEtcEncoder::CompressImageETC2_RGB8
========================
*/
void idEtcEncoder::CompressImageETC2_RGB8( const byte *inBuf, byte *outBuf, int width, int height ) const {
	assert( ( width & 3 ) == 0 && ( height & 3 ) == 0 );

	for ( int y = 0; y < height; y += 4 ) {
		for ( int x = 0; x < width; x += 4 ) {
			etcBlock_t block;
			EtcGatherBlock( inBuf, width, x, y, block );
			EtcCompressColorBlock( block, outBuf );
			outBuf += 8;
		}
	}
}

/*
========================
idEtcEncoder::CompressImageETC2_RGBA8
========================
*/
void idEtcEncoder::CompressImageETC2_RGBA8( const byte *inBuf, byte *outBuf, int width, int height ) const {
	assert( ( width & 3 ) == 0 && ( height & 3 ) == 0 );

	for ( int y = 0; y < height; y += 4 ) {
		for ( int x = 0; x < width; x += 4 ) {
			etcBlock_t block;
			EtcGatherBlock( inBuf, width, x, y, block );
			// alpha first, then colour: the order the format defines
			EtcCompressEacBlock( block, 3, outBuf );
			EtcCompressColorBlock( block, outBuf + 8 );
			outBuf += 16;
		}
	}
}

/*
========================
idEtcEncoder::CompressImageEAC_RG11

Two-channel normal maps. The caller has already put the normal's X in red and
Y in green -- both the DXT5/RXGB decoder and the heightmap image programs emit
that layout -- and the interaction shaders rebuild Z as sqrt( 1 - x^2 - y^2 ),
so the third component is never stored.

Giving X and Y a private channel each is the whole point of the format: ETC2's
RGB modes fit one colour line through all three channels at once, which is a
good model for a photograph and a bad one for a normal, where X and Y are
independent by construction.
========================
*/
void idEtcEncoder::CompressImageEAC_RG11( const byte *inBuf, byte *outBuf, int width, int height ) const {
	assert( ( width & 3 ) == 0 && ( height & 3 ) == 0 );

	for ( int y = 0; y < height; y += 4 ) {
		for ( int x = 0; x < width; x += 4 ) {
			etcBlock_t block;
			EtcGatherBlock( inBuf, width, x, y, block );
			// R then G, the order GL_COMPRESSED_RG11_EAC defines
			EtcCompressEacBlock( block, 0, outBuf );
			EtcCompressEacBlock( block, 1, outBuf + 8 );
			outBuf += 16;
		}
	}
}
