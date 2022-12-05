
#include "../precompiled.h"
#pragma hdrstop

typedef struct {
	double re;
	double im;
} cpxDouble_t;

/*
===================

  "A New Principle for Fast Fourier Transformation", Charles M. Rader and N.M. Brenner, I.E.E.E. Transactions on Acoustics
  "Programs for Digital Signal Processing", published by I.E.E.E. Press, chapter 1, section 1.1, 1979.

===================
*/
// RAVEN BEGIN
// jscott: added stride
void idFFT::FFT1D( cpxFloat_t *data, int N, int ISI, int stride ) 
{
// RAVEN END
	int i, j, m, mmax, istep;
	cpxFloat_t cfTemp;
	cpxDouble_t cdTemp1, cdTemp2;
	double theta, dTemp;
	const double pi = idMath::PI;

	// first operation puts data in bit-reversed order
	j = 0;
	for(i = 0; i < N; i++) {
		if(i < j) {
// RAVEN BEGIN
			cfTemp.re = data[j * stride].re;
			cfTemp.im = data[j * stride].im;

			data[j * stride].re = data[i * stride].re;
			data[j * stride].im = data[i * stride].im;

			data[i * stride].re = cfTemp.re;
			data[i * stride].im = cfTemp.im;
// RAVEN END
		}
		m = N / 2;
		while(j >= m) {
			j = j - m;
			m = m / 2;
			if(m == 0){
				break;
			}
		}
		j = j + m;
	}

	// second operation computes the butterflies
	mmax = 1;
	while (mmax < N) {
		istep = 2 * mmax;
		theta = pi * ISI / mmax;
		dTemp = idMath::Sin(theta / 2.0);
		cdTemp2.re = -2.0 * dTemp * dTemp;
		cdTemp2.im = idMath::Sin(theta);
		cdTemp1.re = 1.0;
		cdTemp1.im = 0.0;
		for(m = 0; m < mmax; m++) {
			for(i = m; i < N; i += istep) {
				j = i + mmax;
// RAVEN BEGIN
				cfTemp.re = (float)(cdTemp1.re * data[j * stride].re - cdTemp1.im * data[j * stride].im);
				cfTemp.im = (float)(cdTemp1.re * data[j * stride].im + cdTemp1.im * data[j * stride].re);
				data[j * stride].re = data[i * stride].re - cfTemp.re;
				data[j * stride].im = data[i * stride].im - cfTemp.im;
				data[i * stride].re += cfTemp.re;
				data[i * stride].im += cfTemp.im;
// RAVEN END
			}
			dTemp = cdTemp1.re;
			cdTemp1.re = cdTemp1.re * cdTemp2.re - cdTemp1.im * cdTemp2.im + cdTemp1.re;
			cdTemp1.im = cdTemp1.im * cdTemp2.re + dTemp * cdTemp2.im + cdTemp1.im;
		}
		mmax = istep;
	}
}

// RAVEN BEGIN
// jscott: added 2d Fourier transform - cost 2N
void idFFT::FFT2D( cpxFloat_t *data, int N, int ISI ) 
{
	cpxFloat_t	*orig;
	int			i;

	orig = data;

	// 1D horizontal transform
	for( i = 0; i < N; i++ )
	{
		FFT1D( data, N, ISI, 1 );
		data += N;
	}

	// 1D vertical transform
	data = orig;
	for( i = 0; i < N; i++ )
	{
		FFT1D( data, N, ISI, N );
		data++;
	}
}

// jscott: added 3d Fourier transform - cost 3N^2
void idFFT::FFT3D( cpxFloat_t *data, int N, int ISI ) 
{
	cpxFloat_t	*orig;
	int			i, j;

	orig = data;

	// Transform each slice
	for( j = 0; j < N; j++ )
	{
		// 1D transform
		data = orig + j * N * N;
		for( i = 0; i < N; i++ )
		{
			FFT1D( data, N, ISI, 1 );
			data += N;
		}

		// 1D transform
		data = orig + j * N * N;
		for( i = 0; i < N; i++ )
		{
			FFT1D( data, N, ISI, N );
			data++;
		}
	}

	// Transform the volume
	data = orig;
	for( j = 0; j < N * N; j++ )
	{
		FFT1D( data, N, ISI, N * N );
		data++;
	}
}
// RAVEN END
