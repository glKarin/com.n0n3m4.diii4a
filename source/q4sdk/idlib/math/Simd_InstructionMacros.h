
#ifndef __SIMD_SSE_MACROS_H__
#define __SIMD_SSE_MACROS_H__

//#include <xmmintrin.h>	// MSVC intrinsic SSE is very poor

/*
===============================================================================

	Instruction Macros.

	The first argument of an instruction macro is the destination and
	the second argument is the source operand. For most instructions
	the destination operand can be _xmm0 to _xmm7 only. The source
	operand can be any one of the registers _xmm0 to _xmm7 or _eax,
	_ecx, _edx, _esp, _ebp, _ebx, _esi, or _edi that contains the
	effective address.

	For instance:  haddps    xmm0, xmm1
	becomes:       _haddps( _xmm0, _xmm1 )
	and:           haddps    xmm0, [esi]
	becomes:       _haddps( _xmm0, _esi )

	The ADDRESS_ADD_C macro can be used when the effective source address
	is formed by adding a constant to a general purpose register.
	The constant must be in the range [-128, 127]. Use the ADDRESS_ADD_LC
	macro if the constant is not in this range.

	For instance:  haddps    xmm0, [esi+48]
	becomes:       _haddps( _xmm0, ADDRESS_ADD_C( _esi, 48 ) )

	The ADDRESS_ADD_R macro can be used when the effective source address
	is formed by adding two general purpose registers.

	For instance:  haddps    xmm0, [esi+eax]
	becomes:       _haddps( _xmm0, ADDRESS_ADD_R( _esi, _eax ) )

	The ADDRESS_ADD_RC macro can be used when the effective source address
	is formed by adding two general purpose registers and a constant.
	The constant must be in the range [-128, 127]. Use the ADDRESS_ADD_RLC
	macro if the constant is not in this range.

	For instance:  haddps    xmm0, [esi+eax+48]
	becomes:       _haddps( _xmm0, ADDRESS_ADD_RC( _esi, _eax, 48 ) )

	The ADDRESS_ADD_RS macro can be used when the effective source address is formed
	by adding a scaled general purpose register to another general purpose register.
	The scale must be either 1, 2, 4 or 8.

	For instance:  haddps    xmm0, [esi+eax*4]
	becomes:       _haddps( _xmm0, ADDRESS_ADD_RS( _esi, _eax, 4 ) )

	The ADDRESS_ADD_RSC macro can be used when the effective source address is formed
	by adding a scaled general purpose register to another general purpose register and
	also adding a constant. The scale must be either 1, 2, 4 or 8. The constant must
	be in the range [-128, 127]. Use the ADDRESS_ADD_RSLC macro if the constant is not
	in this range.

	For instance:  haddps    xmm0, [esi+eax*4+64]
	becomes:       _haddps( _xmm0, ADDRESS_ADD_RSC( _esi, _eax, 4, 64 ) )

===============================================================================
*/

/*
===============================================================================

	Register constants.

===============================================================================
*/

#define _eax	0x00
#define _ecx	0x01
#define _edx	0x02
#define _ebx	0x03
#define _esp	0x04
#define _ebp	0x05
#define _esi	0x06
#define _edi	0x07

#define _mm0	0xC0
#define _mm1	0xC1
#define _mm2	0xC2
#define _mm3	0xC3
#define _mm4	0xC4
#define _mm5	0xC5
#define _mm6	0xC6
#define _mm7	0xC7

#define _xmm0	0xC0
#define _xmm1	0xC1
#define _xmm2	0xC2
#define _xmm3	0xC3
#define _xmm4	0xC4
#define _xmm5	0xC5
#define _xmm6	0xC6
#define _xmm7	0xC7

/*
===============================================================================

	Addressing modes.

===============================================================================
*/

#define RSCALE( s )	( ((s)&2)<<5 ) | ( ((s)&4)<<5 ) | ( ((s)&8)<<3 ) | ( ((s)&8)<<4 )

#define ADDRESS_ADD_C( reg0, constant )					0x40 | ( reg0 & 7 )		\
	_asm _emit constant

#define ADDRESS_ADD_LC( reg0, constant )				0x80 | ( reg0 & 7 )		\
	_asm _emit ( ( (constant) >>  0 ) & 255 )									\
	_asm _emit ( ( (constant) >>  8 ) & 255 )									\
	_asm _emit ( ( (constant) >> 16 ) & 255 )									\
	_asm _emit ( ( (constant) >> 24 ) & 255 )

#define ADDRESS_ADD_R( reg0, reg1 )						0x04					\
	_asm _emit ( ( reg1 & 7 ) << 3 ) | ( reg0 & 7 )

#define ADDRESS_ADD_RC( reg0, reg1, constant )			0x44	 				\
	_asm _emit ( ( reg1 & 7 ) << 3 ) | ( reg0 & 7 )								\
	_asm _emit constant

#define ADDRESS_ADD_RLC( reg0, reg1, constant )			0x84	 				\
	_asm _emit ( ( reg1 & 7 ) << 3 ) | ( reg0 & 7 )								\
	_asm _emit ( ( (constant) >>  0 ) & 255 )									\
	_asm _emit ( ( (constant) >>  8 ) & 255 )									\
	_asm _emit ( ( (constant) >> 16 ) & 255 )									\
	_asm _emit ( ( (constant) >> 24 ) & 255 )

#define ADDRESS_ADD_RS( reg0, reg1, scale )				0x04					\
	_asm _emit ( ( reg1 & 7 ) << 3 ) | ( reg0 & 7 ) | RSCALE( scale )

#define ADDRESS_ADD_RSC( reg0, reg1, scale, constant )	0x44					\
	_asm _emit ( ( reg1 & 7 ) << 3 ) | ( reg0 & 7 ) | RSCALE( scale )			\
	_asm _emit constant

#define ADDRESS_ADD_RSLC( reg0, reg1, scale, constant )	0x84					\
	_asm _emit ( ( reg1 & 7 ) << 3 ) | ( reg0 & 7 ) | RSCALE( scale )			\
	_asm _emit ( ( (constant) >>  0 ) & 255 )									\
	_asm _emit ( ( (constant) >>  8 ) & 255 )									\
	_asm _emit ( ( (constant) >> 16 ) & 255 )									\
	_asm _emit ( ( (constant) >> 24 ) & 255 )

/*
===============================================================================

	Macros for third operand of shuffle/swizzle instructions.

===============================================================================
*/

#define SHUFFLE_PS( x, y, z, w )	(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define R_SHUFFLE_PS( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

#define SHUFFLE_PD( x, y )			(( (x) & 1 ) << 1 | ( (y) & 1 ))
#define R_SHUFFLE_PD( x, y )		(( (y) & 1 ) << 1 | ( (x) & 1 ))

#define SHUFFLE_D( x, y, z, w )		(( (x) & 3 ) << 6 | ( (y) & 3 ) << 4 | ( (z) & 3 ) << 2 | ( (w) & 3 ))
#define R_SHUFFLE_D( x, y, z, w )	(( (w) & 3 ) << 6 | ( (z) & 3 ) << 4 | ( (y) & 3 ) << 2 | ( (x) & 3 ))

/*
===============================================================================

	SSE compare instructions.

===============================================================================
*/

#define _EQ			0
#define _LT			1
#define _LE			2
#define _UNORDERED	3
#define _NEQ		4
#define _NLT		5
#define _NLE		6
#define _ORDERED	7

#define _cmpps( dst, src, cond )				\
	_asm _emit 0x0F								\
	_asm _emit 0xC2								\
	_asm _emit ( ( dst & 7 ) << 3 ) | src		\
	_asm _emit cond

// Equal ( dst[0-3] = ( dst[0-3] == src[0-3] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpeps( dst, src )		_cmpps( dst, src, _EQ )
#define _cmpeqps( dst, src )	_cmpps( dst, src, _EQ )

// Not Equal ( dst[0-3] = ( dst[0-3] != src[0-3] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpneps( dst, src )	_cmpps( dst, src, _NEQ )
#define _cmpneqps( dst, src )	_cmpps( dst, src, _NEQ )

// Less Than ( dst[0-3] = ( dst[0-3] < src[0-3] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpltps( dst, src )	_cmpps( dst, src, _LT )

// Less Equal ( dst[0-3] = ( dst[0-3] <= src[0-3] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpleps( dst, src )	_cmpps( dst, src, _LE )

// Greater Equal ( dst[0-3] = ( dst[0-3] >= src[0-3] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpgeps( dst, src )	_cmpps( dst, src, _NLT )

// Greater Than ( dst[0-3] = ( dst[0-3] > src[0-3] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpgtps( dst, src )	_cmpps( dst, src, _NLE )

// Not Less Than ( dst[0-3] = ( dst[0-3] >= src[0-3] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpnltps( dst, src )	_cmpps( dst, src, _NLT )

// Not Less Equal ( dst[0-3] = ( dst[0-3] > src[0-3] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpnleps( dst, src )	_cmpps( dst, src, _NLE )

// Ordered ( dst[0-3] = ( dst[0-3] != NaN && src[0-3] != NaN ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpordps( dst, src )	_cmpps( dst, src, _ORDERED )

// Unordered ( dst[0-3] = ( dst[0-3] == NaN || src[0-3] == NaN ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpunordps( dst, src )	_cmpps( dst, src, _UNORDERED )

#define _cmpss( dst, src, cond )				\
	_asm _eint 0xF3								\
	_asm _emit 0x0F								\
	_asm _emit 0xC2								\
	_asm _emit ( ( dst & 7 ) << 3 ) | src		\
	_asm _emit cond

// Equal ( dst[0] = ( dst[0] == src[0] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpess( dst, src )		_cmpss( dst, src, _EQ )
#define _cmpeqss( dst, src )	_cmpss( dst, src, _EQ )

// Not Equal ( dst[0] = ( dst[0] != src[0] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpness( dst, src )	_cmpss( dst, src, _NEQ )
#define _cmpneqss( dst, src )	_cmpss( dst, src, _NEQ )

// Less Than ( dst[0] = ( dst[0] < src[0] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpltss( dst, src )	_cmpss( dst, src, _LT )

// Less Equal ( dst[0] = ( dst[0] <= src[0] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpless( dst, src )	_cmpss( dst, src, _LE )

// Greater Equal ( dst[0] = ( dst[0] >= src[0] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpgess( dst, src )	_cmpss( dst, src, _NLT )

// Greater Than ( dst[0] = ( dst[0] > src[0] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpgtss( dst, src )	_cmpss( dst, src, _NLE )

// Not Less Than ( dst[0] = ( dst[0] >= src[0] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpnltss( dst, src )	_cmpss( dst, src, _NLT )

// Not Less Equal ( dst[0] = ( dst[0] > src[0] ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpnless( dst, src )	_cmpss( dst, src, _NLE )

// Ordered ( dst[0] = ( dst[0] != NaN && src[0] != NaN ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpordss( dst, src )	_cmpss( dst, src, _ORDERED )

// Unordered ( dst[0] = ( dst[0] == NaN || src[0] == NaN ) ? 0xFFFFFFFF : 0x00000000 )
#define _cmpunordss( dst, src )	_cmpss( dst, src, _UNORDERED )

/*
===============================================================================

	SSE3 instructions.

===============================================================================
*/

// Packed Single-FP Add/Subtract ( dst[0]=dst[0]+src[0], dst[1]=dst[1]-src[1], dst[2]=dst[2]+src[2], dst[3]=dst[3]-src[3] )
#define _addsubps( dst, src )						\
	_asm _emit 0xF2									\
	_asm _emit 0x0F									\
	_asm _emit 0xD0									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Packed Double-FP Add/Subtract ( dst[0]=dst[0]+src[0], dst[1]=dst[1]-src[1] )
#define _addsubpd( dst, src )						\
	_asm _emit 0x66									\
	_asm _emit 0x0F									\
	_asm _emit 0xD0									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Packed Single-FP Horizontal Add ( dst[0]=dst[0]+dst[1], dst[1]=dst[2]+dst[3], dst[2]=src[0]+src[1], dst[3]=src[2]+src[3] )
#define _haddps( dst, src )							\
	_asm _emit 0xF2									\
	_asm _emit 0x0F									\
	_asm _emit 0x7C									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Packed Double-FP Horizontal Add ( dst[0]=dst[0]+dst[1], dst[1]=src[0]+src[1] )
#define _haddpd( dst, src )							\
	_asm _emit 0x66									\
	_asm _emit 0x0F									\
	_asm _emit 0x7C									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Packed Single-FP Horizontal Subtract ( dst[0]=dst[0]-dst[1], dst[1]=dst[2]-dst[3], dst[2]=src[0]-src[1], dst[3]=src[2]-src[3] )
#define _hsubps( dst, src )							\
	_asm _emit 0xF2									\
	_asm _emit 0x0F									\
	_asm _emit 0x7D									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Packed Double-FP Horizontal Subtract ( dst[0]=dst[0]-dst[1], dst[1]=src[0]-src[1] )
#define _hsubpd( dst, src )							\
	_asm _emit 0x66									\
	_asm _emit 0x0F									\
	_asm _emit 0x7D									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Move Packed Single-FP Low and Duplicate ( dst[0]=src[0], dst[1]=src[0], dst[2]=src[2], dst[3]=src[2] )
#define _movsldup( dst, src )						\
	_asm _emit 0xF3									\
	_asm _emit 0x0F									\
	_asm _emit 0x12									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Move One Double-FP Low and Duplicate ( dst[0]=src[0], dst[1]=src[0] )
#define _movdldup( dst, src )						\
	_asm _emit 0xF2									\
	_asm _emit 0x0F									\
	_asm _emit 0x12									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Move Packed Single-FP High and Duplicate ( dst[0]=src[1], dst[1]=src[1], dst[2]=src[3], dst[3]=src[3] )
#define _movshdup( dst, src )						\
	_asm _emit 0xF3									\
	_asm _emit 0x0F									\
	_asm _emit 0x16									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Move One Double-FP High and Duplicate ( dst[0]=src[1], dst[1]=src[1] )
#define _movdhdup( dst, src )						\
	_asm _emit 0xF2									\
	_asm _emit 0x0F									\
	_asm _emit 0x16									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src

// Load Unaligned Integer 128 bits
#define _lddqu( dst, src )							\
	_asm _emit 0xF2									\
	_asm _emit 0x0F									\
	_asm _emit 0xF0									\
	_asm _emit ( ( dst & 7 ) << 3 ) | src


/*
===============================================================================

	SSE transpose macros

===============================================================================
*/

// transpose a 4x4 matrix loaded into 4 xmm registers (reg4 is temporary)
#define TRANSPOSE_4x4( reg0, reg1, reg2, reg3, reg4 )											\
	__asm	movaps		reg4, reg2								/* reg4 =  8,  9, 10, 11 */		\
	__asm	unpcklps	reg2, reg3								/* reg2 =  8, 12,  9, 13 */		\
	__asm	unpckhps	reg4, reg3								/* reg4 = 10, 14, 11, 15 */		\
	__asm	movaps		reg3, reg0								/* reg3 =  0,  1,  2,  3 */		\
	__asm	unpcklps	reg0, reg1								/* reg0 =  0,  4,  1,  5 */		\
	__asm	unpckhps	reg3, reg1								/* reg3 =  2,  6,  3,  7 */		\
	__asm	movaps		reg1, reg0								/* reg1 =  0,  4,  1,  5 */		\
	__asm	shufps		reg0, reg2, R_SHUFFLE_PS( 0, 1, 0, 1 )	/* reg0 =  0,  4,  8, 12 */		\
	__asm	shufps		reg1, reg2, R_SHUFFLE_PS( 2, 3, 2, 3 )	/* reg1 =  1,  5,  9, 13 */		\
	__asm	movaps		reg2, reg3								/* reg2 =  2,  6,  3,  7 */		\
	__asm	shufps		reg2, reg4, R_SHUFFLE_PS( 0, 1, 0, 1 )	/* reg2 =  2,  6, 10, 14 */		\
	__asm	shufps		reg3, reg4, R_SHUFFLE_PS( 2, 3, 2, 3 )	/* reg3 =  3,  7, 11, 15 */

// transpose a 4x4 matrix from memory into 4 xmm registers (reg4 is temporary)
#define TRANPOSE_4x4_FROM_MEMORY( address, reg0, reg1, reg2, reg3, reg4 )						\
	__asm	movlps		reg1, [address+ 0]						/* reg1 =  0,  1,  X,  X */		\
	__asm	movlps		reg3, [address+ 8]						/* reg3 =  2,  3,  X,  X */		\
	__asm	movhps		reg1, [address+16]						/* reg1 =  0,  1,  4,  5 */		\
	__asm	movhps		reg3, [address+24]						/* reg3 =  2,  3,  6,  7 */		\
	__asm	movlps		reg2, [address+32]						/* reg2 =  8,  9,  X,  X */		\
	__asm	movlps		reg4, [address+40]						/* reg4 = 10, 11,  X,  X */		\
	__asm	movhps		reg2, [address+48]						/* reg2 =  8,  9, 12, 13 */		\
	__asm	movhps		reg4, [address+56]						/* reg4 = 10, 11, 14, 15 */		\
	__asm	movaps		reg0, reg1								/* reg0 =  0,  1,  4,  5 */		\
	__asm	shufps		reg0, reg2, R_SHUFFLE_PS( 0, 2, 0, 2 )	/* reg0 =  0,  4,  8, 12 */		\
	__asm	shufps		reg1, reg2, R_SHUFFLE_PS( 1, 3, 1, 3 )	/* reg1 =  1,  5,  9, 13 */		\
	__asm	movaps		reg2, reg3								/* reg2 =  2,  3,  6,  7 */		\
	__asm	shufps		reg2, reg4, R_SHUFFLE_PS( 0, 2, 0, 2 )	/* reg2 =  2,  6, 10, 14 */		\
	__asm	shufps		reg3, reg4, R_SHUFFLE_PS( 1, 3, 1, 3 )	/* reg3 =  3,  7, 11, 15 */

// transpose a 4x4 matrix to memory from 4 xmm registers (reg4 is temporary)
#define TRANPOSE_4x4_TO_MEMORY( address, reg0, reg1, reg2, reg3, reg4 )							\
	__asm	movaps		reg4, reg0								/* reg4 =  0,  4,  8, 12 */		\
	__asm	unpcklps	reg0, reg1								/* reg0 =  0,  1,  4,  5 */		\
	__asm	unpckhps	reg4, reg1								/* reg4 =  8,  9, 12, 13 */		\
	__asm	movaps		reg1, reg2								/* reg1 =  2,  6, 10, 14 */		\
	__asm	unpcklps	reg2, reg3								/* reg2 =  2,  3,  6,  7 */		\
	__asm	unpckhps	reg1, reg3								/* reg1 = 10, 11, 14, 15 */		\
	__asm	movlps		[address+ 0], reg0						/* mem0 =  0,  1,  X,  X */		\
	__asm	movlps		[address+ 8], reg2						/* mem0 =  0,  1,  2,  3 */		\
	__asm	movhps		[address+16], reg0						/* mem1 =  4,  5,  X,  X */		\
	__asm	movhps		[address+24], reg2						/* mem1 =  4,  5,  6,  7 */		\
	__asm	movlps		[address+32], reg4						/* mem2 =  8,  9,  X,  X */		\
	__asm	movlps		[address+40], reg1						/* mem2 =  8,  9, 10, 11 */		\
	__asm	movhps		[address+48], reg4						/* mem3 = 12, 13,  X,  X */		\
	__asm	movhps		[address+56], reg1						/* mem3 = 12, 13, 14, 15 */

// transpose a 4x3 matrix loaded into 3 xmm registers (reg3 is temporary)
// NOTE: the middle two colums are interchanged which is much faster and fine for most calculations
#define TRANSPOSE_4x3( reg0, reg1, reg2, reg3 )													\
	__asm	movaps		reg3, reg2								/* reg3 =  8,  9, 10, 11 */		\
	__asm	shufps		reg3, reg1, R_SHUFFLE_PS( 2, 3, 0, 1 )	/* reg3 = 10, 11,  4,  5 */		\
	__asm	shufps		reg2, reg0, R_SHUFFLE_PS( 0, 1, 2, 3 )	/* reg2 =  8,  9,  2,  3 */		\
	__asm	shufps		reg1, reg0, R_SHUFFLE_PS( 2, 3, 0, 1 )	/* reg1 =  6,  7,  0,  1 */		\
	__asm	movaps		reg0, reg1								/* reg0 =  6,  7,  0,  1 */		\
	__asm	shufps		reg0, reg2, R_SHUFFLE_PS( 2, 0, 3, 1 )	/* reg0 =  0,  6,  3,  9 */		\
	__asm	shufps		reg1, reg3, R_SHUFFLE_PS( 3, 1, 2, 0 )	/* reg1 =  1,  7,  4, 10 */		\
	__asm	shufps		reg2, reg3, R_SHUFFLE_PS( 2, 0, 3, 1 )	/* reg2 =  2,  8,  5, 11 */

// transpose a 4x3 matrix from memory into 3 xmm registers (reg3 is temporary)
// NOTE: the middle two colums are interchanged which is much faster and fine for most calculations
#define TRANSPOSE_4x3_FROM_MEMORY( address, reg0, reg1, reg2, reg3 )							\
	__asm	movlps		reg1, [address+ 0]						/* reg1 =  0,  1,  X,  X */		\
	__asm	movlps		reg2, [address+ 8]						/* reg2 =  2,  3,  X,  X */		\
	__asm	movlps		reg3, [address+16]						/* reg3 =  4,  5,  X,  X */		\
	__asm	movhps		reg1, [address+24]						/* reg1 =  0,  1,  6,  7 */		\
	__asm	movhps		reg2, [address+32]						/* reg2 =  2,  3,  8,  9 */		\
	__asm	movhps		reg3, [address+40]						/* reg3 =  4,  5, 10, 11 */		\
	__asm	movaps		reg0, reg1								/* reg0 =  0,  1,  6,  7 */		\
	__asm	shufps		reg0, reg2, R_SHUFFLE_PS( 0, 2, 1, 3 )	/* reg0 =  0,  6,  3,  9 */		\
	__asm	shufps		reg1, reg3, R_SHUFFLE_PS( 1, 3, 0, 2 )	/* reg1 =  1,  7,  4, 10 */		\
	__asm	shufps		reg2, reg3, R_SHUFFLE_PS( 0, 2, 1, 3 )	/* reg2 =  2,  8,  5, 11 */

// transpose a 4x3 matrix to memory from 3 xmm registers (reg3 is temporary)
// NOTE: the middle two colums are interchanged which is much faster and fine for most calculations
#define TRANSPOSE_4x3_TO_MEMORY( address, reg0, reg1, reg2, reg3 )								\
	__asm	movhlps		reg3, reg0 								/* reg3 =  3,  9,  X,  X */		\
	__asm	unpcklps	reg0, reg1								/* reg0 =  0,  1,  6,  7 */		\
	__asm	unpckhps	reg1, reg2								/* reg1 =  4,  5, 10, 11 */		\
	__asm	unpcklps	reg2, reg3								/* reg2 =  2,  3,  8,  9 */		\
	__asm	movlps		[address+ 0], reg0						/* mem0 =  0,  1,  X,  X */		\
	__asm	movlps		[address+ 8], reg2						/* mem0 =  0,  1,  2,  3 */		\
	__asm	movlps		[address+16], reg1						/* mem1 =  4,  5,  X,  X */		\
	__asm	movhps		[address+24], reg0						/* mem1 =  4,  5,  6,  7 */		\
	__asm	movhps		[address+32], reg2						/* mem2 =  8,  9,  X,  X */		\
	__asm	movhps		[address+40], reg1						/* mem2 =  8,  9, 10, 11 */

/*
===============================================================================

	SSE2 transpose macros

===============================================================================
*/

// transpose a 4x4 integer matrix loaded into 4 xmm registers (reg4 is temporary)
// this is faster than the SSE float version because punpcklqdq has a lower latency than shufps
#define TRANSPOSE_4x4_INT( reg0, reg1, reg2, reg3, reg4 )										\
	__asm	movdqa		reg4, reg2								/* reg4 =  8,  9, 10, 11 */		\
	__asm	punpckldq	reg2, reg3								/* reg2 =  8, 12,  9, 13 */		\
	__asm	punpckhdq	reg4, reg3								/* reg4 = 10, 14, 11, 15 */		\
	__asm	movdqa		reg3, reg0								/* reg3 =  0,  1,  2,  3 */		\
	__asm	punpckldq	reg0, reg1								/* reg0 =  0,  4,  1,  5 */		\
	__asm	punpckhdq	reg3, reg1								/* reg3 =  2,  6,  3,  7 */		\
	__asm	movdqa		reg1, reg0								/* reg1 =  0,  4,  1,  5 */		\
	__asm	punpcklqdq	reg0, reg2								/* reg0 =  0,  4,  8, 12 */		\
	__asm	punpckhqdq	reg1, reg2								/* reg1 =  1,  5,  9, 13 */		\
	__asm	movdqa		reg2, reg3								/* reg2 =  2,  6,  3,  7 */		\
	__asm	punpcklqdq	reg2, reg4								/* reg2 =  2,  6, 10, 14 */		\
	__asm	punpckhqdq	reg3, reg4								/* reg3 =  3,  7, 11, 15 */


/*
===============================================================================

	Macros to create SSE constants.

===============================================================================
*/

#define ALIGN4_INIT1( X, I0 )				ALIGN16( static X[4] ) = { I0, I0, I0, I0 }
#define ALIGN4_INIT4( X, I0, I1, I2, I3 )	ALIGN16( static X[4] ) = { I0, I1, I2, I3 }
#define ALIGN8_INIT1( X, I0 )				ALIGN16( static X[8] ) = { I0, I0, I0, I0, I0, I0, I0, I0 }
#define ALIGN16_INIT1( X, I0 )				ALIGN16( static X[16] ) = { I0, I0, I0, I0, I0, I0, I0, I0, I0, I0, I0, I0, I0, I0, I0, I0 }

#define IEEE_SP_ZERO						0
#define IEEE_SP_ONE							0x3f800000
#define IEEE_SP_SIGN						((unsigned long) ( 1 << 31 ))
#define IEEE_SP_INF							((unsigned long) ( 1 << 23 ))

/*
===============================================================================

	Macro to remove boundary check from switch statements.

===============================================================================
*/

#ifdef _DEBUG
#define NODEFAULT	default: assert( 0 )
#elif _WIN32
#define NODEFAULT	default: __assume( 0 )
#else
#define NODEFAULT
#endif

#endif /* !__SIMD_SSE_MACROS_H__ */
