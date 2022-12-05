
#ifndef __MATH_FFT_H__
#define __MATH_FFT_H__

/*
===============================================================================

  Fast Fourier Transform

===============================================================================
*/

// complex number
typedef struct {
	float re;
	float im;
} cpxFloat_t;

class idFFT {
public:
// RAVEN BEGIN
// jscott: added stride to 1D, created 2D
	static void		FFT1D( cpxFloat_t *data, int N, int ISI, int stride = 1 );
	static void		FFT2D( cpxFloat_t *data, int N, int ISI );
	static void		FFT3D( cpxFloat_t *data, int N, int ISI );
// RAVEN END
};

#endif  /* !__MATH_FFT_H__ */
