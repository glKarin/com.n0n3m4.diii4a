
#ifndef __MATH_SIMD_MMX_H__
#define __MATH_SIMD_MMX_H__

/*
===============================================================================

	MMX implementation of idSIMDProcessor

===============================================================================
*/

class idSIMD_MMX : public idSIMD_Generic {
#if defined( _WIN32 ) || defined( __linux__ )
public:
	virtual const char * VPCALL GetName( void ) const;

	virtual void VPCALL Memcpy( void *dst,			const void *src,		const int count );
	virtual void VPCALL Memset( void *dst,			const int val,			const int count );

#endif
};

#endif /* !__MATH_SIMD_MMX_H__ */
