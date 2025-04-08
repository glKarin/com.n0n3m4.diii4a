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
#ifndef __gdefs_h__
#define __gdefs_h__

/*==================*
 * TYPE DEFINITIONS *
 *==================*/

typedef unsigned char byte;
typedef unsigned short word;
#pragma once

#define	dabs(a) (((a)<0) ? -(a) : (a))
#define CLAMP(v,l,h) ((v)<(l) ? (l) : (v)>(h) ? (h) : v)
#define	xswap(a,b) { a^=b; b^=a; a^=b; }
#define lum(a) ( 0.2990*(a>>16) + 0.5870*((a>>8)&0xff) + 0.1140*(a&0xff) )
#define gsign(a)  	((a) < 0 ? -1 : 1)
#define mnint(a)	((a) < 0 ? (int)(a - 0.5) : (int)(a + 0.5))
#define mmax(a, b)  	((a) > (b) ? (a) : (b))
#define mmin(a, b)  	((a) < (b) ? (a) : (b))
#define RGBDIST( src0, src1 ) ( ((src0[0]-src1[0])*(src0[0]-src1[0])) + \
								((src0[1]-src1[1])*(src0[1]-src1[1])) + \
								((src0[2]-src1[2])*(src0[2]-src1[2])) )

#define RGBADIST( src0, src1 ) ( ((src0[0]-src1[0])*(src0[0]-src1[0])) + \
								 ((src0[1]-src1[1])*(src0[1]-src1[1])) + \
								 ((src0[2]-src1[2])*(src0[2]-src1[2])) + \
								 ((src0[3]-src1[3])*(src0[3]-src1[3])) )


#define RMULT 0.2990f				// use these for televisions
#define GMULT 0.5870f				
#define BMULT 0.1140f

#define RIEMULT -0.16874f
#define RQEMULT  0.50000f
#define GIEMULT -0.33126f
#define GQEMULT -0.41869f
#define BIEMULT  0.50000f
#define BQEMULT -0.08131f

#endif // gdefs
