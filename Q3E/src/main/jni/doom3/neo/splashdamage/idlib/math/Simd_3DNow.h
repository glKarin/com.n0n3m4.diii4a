// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __MATH_SIMD_3DNOW_H__
#define __MATH_SIMD_3DNOW_H__

/*
===============================================================================

	3DNow! implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_3DNow : public idSIMD_MMX {
#ifdef ID_WIN_X86_ASM
public:
	virtual const char * VPCALL GetName( void ) const;

	virtual void VPCALL Memcpy( void *dst,			const void *src,		const int count );

#endif
};

#endif /* !__MATH_SIMD_3DNOW_H__ */
