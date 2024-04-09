/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#pragma hdrstop

#include "renderer/tr_local.h"

/*
================
R_ResampleTexture

Used to resample images in a more general than quartering fashion.

This will only have filter coverage if the resampled size
is greater than half the original size.

If a larger shrinking is needed, use the mipmap function 
after resampling to the next lower power of two.
================
*/
#define	MAX_DIMENSION	4096
byte *R_ResampleTexture( const byte *in, int inwidth, int inheight,  
							int outwidth, int outheight ) {
	int		i, j;
	const byte	*inrow, *inrow2;
	unsigned int	frac, fracstep;
	unsigned int	p1[MAX_DIMENSION], p2[MAX_DIMENSION];
	const byte		*pix1, *pix2, *pix3, *pix4;
	byte		*out, *out_p;

	if ( outwidth > MAX_DIMENSION ) {
		outwidth = MAX_DIMENSION;
	}

	if ( outheight > MAX_DIMENSION ) {
		outheight = MAX_DIMENSION;
	}
	out = (byte *)R_StaticAlloc( outwidth * outheight * 4 );
	out_p = out;

	fracstep = inwidth*0x10000/outwidth;
	frac = fracstep>>2;

	for ( i=0 ; i<outwidth ; i++ ) {
		p1[i] = 4*(frac>>16);
		frac += fracstep;
	}
	frac = 3*(fracstep>>2);

	for ( i=0 ; i<outwidth ; i++ ) {
		p2[i] = 4*(frac>>16);
		frac += fracstep;
	}

	for (i=0 ; i<outheight ; i++, out_p += outwidth*4 ) {
		inrow = in + 4 * inwidth * (int)( ( i + 0.25f ) * inheight / outheight );
		inrow2 = in + 4 * inwidth * (int)( ( i + 0.75f ) * inheight / outheight );
		frac = fracstep >> 1;

		for (j=0 ; j<outwidth ; j++) {
			pix1 = inrow + p1[j];
			pix2 = inrow + p2[j];
			pix3 = inrow2 + p1[j];
			pix4 = inrow2 + p2[j];
			out_p[j*4+0] = (pix1[0] + pix2[0] + pix3[0] + pix4[0])>>2;
			out_p[j*4+1] = (pix1[1] + pix2[1] + pix3[1] + pix4[1])>>2;
			out_p[j*4+2] = (pix1[2] + pix2[2] + pix3[2] + pix4[2])>>2;
			out_p[j*4+3] = (pix1[3] + pix2[3] + pix3[3] + pix4[3])>>2;
		}
	}
	return out;
}

/*
================
R_Dropsample

Used to resample images in a more general than quartering fashion.
Normal maps and such should not be bilerped.
================
*/
byte *R_Dropsample( const byte *in, int inwidth, int inheight,  
							int outwidth, int outheight ) {
	int			i, j, k;
	const byte	*inrow;
	const byte	*pix1;
	byte		*out, *out_p;

	out = (byte *)R_StaticAlloc( outwidth * outheight * 4 );
	out_p = out;

	for (i=0 ; i<outheight ; i++, out_p += outwidth*4 ) {
		inrow = in + 4*inwidth*(int)((i+0.25)*inheight/outheight);
		for (j=0 ; j<outwidth ; j++) {
			k = j * inwidth / outwidth;
			pix1 = inrow + k * 4;
			out_p[j*4+0] = pix1[0];
			out_p[j*4+1] = pix1[1];
			out_p[j*4+2] = pix1[2];
			out_p[j*4+3] = pix1[3];
		}
	}
	return out;
}


/*
===============
R_SetBorderTexels

===============
*/
void R_SetBorderTexels( byte *inBase, int width, int height, const byte border[4] ) {
	int		i;
	byte	*out;

	out = inBase;

	for (i=0 ; i<height ; i++, out+=width*4) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
	out = inBase+(width-1)*4;

	for (i=0 ; i<height ; i++, out+=width*4) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
	out = inBase;

	for (i=0 ; i<width ; i++, out+=4) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
	out = inBase+width*4*(height-1);

	for (i=0 ; i<width ; i++, out+=4) {
		out[0] = border[0];
		out[1] = border[1];
		out[2] = border[2];
		out[3] = border[3];
	}
}

/*
================
R_MipMap

Returns a new copy of the texture, quartered in size and filtered.
================
*/
byte *R_MipMap( const byte *in, int width, int height ) {

	if ( width < 1 || height < 1 || ( width + height == 2 ) ) {
		common->FatalError( "R_MipMap called with size %i,%i", width, height );
	}

	int newWidth = idMath::Imax(width >> 1, 1);
	int newHeight = idMath::Imax(height >> 1, 1);
	byte *out = (byte *)R_StaticAlloc( newWidth * newHeight * 4 );

	if (width == 1 || height == 1) {
		int n = idMath::Imax(width, height);
		int bn = n >> 1;
		for (int i = 0; i < bn; i++) {
			for (int c = 0; c < 4; c++)
				out[4*i+c] = ((unsigned)in[8*i+c] + in[8*i+4+c] + 1) >> 1;
		}
	}
	else {
		SIMDProcessor->GenerateMipMap2x2(in, width * 4, newWidth, newHeight, out, newWidth * 4);
	}

	return out;
}

/*
==================
R_BlendOverTexture

Apply a color blend over a set of pixels
==================
*/
void R_BlendOverTexture( byte *data, int pixelCount, const byte blend[4] ) {
	int		i;
	int		inverseAlpha;
	int		premult[3];

	inverseAlpha = 255 - blend[3];
	premult[0] = blend[0] * blend[3];
	premult[1] = blend[1] * blend[3];
	premult[2] = blend[2] * blend[3];

	for ( i = 0 ; i < pixelCount ; i++, data+=4 ) {
		data[0] = ( data[0] * inverseAlpha + premult[0] ) >> 9;
		data[1] = ( data[1] * inverseAlpha + premult[1] ) >> 9;
		data[2] = ( data[2] * inverseAlpha + premult[2] ) >> 9;
	}
}


/*
==================
R_HorizontalFlip

Flip the image in place
==================
*/
void R_HorizontalFlip( byte *data, int width, int height ) {
	int		i, j;
	int		temp;

	for ( i = 0 ; i < height ; i++ ) {
		for ( j = 0 ; j < width / 2 ; j++ ) {
			temp = *( (int *)data + i * width + j );
			*( (int *)data + i * width + j ) = *( (int *)data + i * width + width - 1 - j );
			*( (int *)data + i * width + width - 1 - j ) = temp;
		}
	}
}

void R_VerticalFlip( byte *data, int width, int height ) {
	int		i, j;
	int		temp;

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < height / 2 ; j++ ) {
			temp = *( (int *)data + j * width + i );
			*( (int *)data + j * width + i ) = *( (int *)data + ( height - 1 - j ) * width + i );
			*( (int *)data + ( height - 1 - j ) * width + i ) = temp;
		}
	}
}

void R_RotatePic( byte *data, int width ) {
	int		i, j;
	int		*temp;

	temp = (int *)R_StaticAlloc( width * width * 4 );

	for ( i = 0 ; i < width ; i++ ) {
		for ( j = 0 ; j < width ; j++ ) {
			*( temp + i * width + j ) = *( (int *)data + j * width + i );
		}
	}
	memcpy( data, temp, width * width * 4 );

	R_StaticFree( temp );
}

