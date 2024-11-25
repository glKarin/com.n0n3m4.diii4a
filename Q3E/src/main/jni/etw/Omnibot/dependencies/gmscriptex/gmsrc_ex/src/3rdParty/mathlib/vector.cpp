/*!  
** 
** Copyright (c) 2007 by John W. Ratcliff mailto:jratcliff@infiniplex.net
**
** Portions of this source has been released with the PhysXViewer application, as well as 
** Rocket, CreateDynamics, ODF, and as a number of sample code snippets.
**
** If you find this code useful or you are feeling particularily generous I would
** ask that you please go to http://www.amillionpixels.us and make a donation
** to Troy DeMolay.
**
** DeMolay is a youth group for young men between the ages of 12 and 21.  
** It teaches strong moral principles, as well as leadership skills and 
** public speaking.  The donations page uses the 'pay for pixels' paradigm
** where, in this case, a pixel is only a single penny.  Donations can be
** made for as small as $4 or as high as a $100 block.  Each person who donates
** will get a link to their own site as well as acknowledgement on the
** donations blog located here http://www.amillionpixels.blogspot.com/
**
** If you wish to contact me you can use the following methods:
**
** Skype Phone: 636-486-4040 (let it ring a long time while it goes through switches)
** Skype ID: jratcliff63367
** Yahoo: jratcliff63367
** AOL: jratcliff1961
** email: jratcliff@infiniplex.net
** Personal website: http://jratcliffscarab.blogspot.com
** Coding Website:   http://codesuppository.blogspot.com
** FundRaising Blog: http://amillionpixels.blogspot.com
** Fundraising site: http://www.amillionpixels.us
** New Temple Site:  http://newtemple.blogspot.com
**
**
** The MIT license:
**
** Permission is hereby granted, free of charge, to any person obtaining a copy 
** of this software and associated documentation files (the "Software"), to deal 
** in the Software without restriction, including without limitation the rights 
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
** copies of the Software, and to permit persons to whom the Software is furnished 
** to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in all 
** copies or substantial portions of the Software.

** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
** CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "vector.h"
#include "matrix.h"


// Ripped from Graphics Gems 2
/* lines_intersect:  AUTHOR: Mukesh Prasad
 *
 *   This function computes whether two line segments,
 *   respectively joining the input points (x1,y1) -- (x2,y2)
 *   and the input points (x3,y3) -- (x4,y4) intersect.
 *   If the lines intersect, the output variables x, y are
 *   set to coordinates of the point of intersection.
 *
 *   All values are in integers.  The returned value is rounded
 *   to the nearest integer point.
 *
 *   If non-integral grid points are relevant, the function
 *   can easily be transformed by substituting floating point
 *   calculations instead of integer calculations.
 *
 *   Entry
 *        x1, y1,  x2, y2   Coordinates of endpoints of one segment.
 *        x3, y3,  x4, y4   Coordinates of endpoints of other segment.
 *
 *   Exit
 *        x, y              Coordinates of intersection point.
 *
 *   The value returned by the function is one of:
 *
 *        DONT_INTERSECT    0
 *        DO_INTERSECT      1
 *        COLLINEAR         2
 *
 * Error conditions:
 *
 *     Depending upon the possible ranges, and particularly on 16-bit
 *     computers, care should be taken to protect from overflow.
 *
 *     In the following code, 'long' values have been used for this
 *     purpose, instead of 'int'.
 *
 */

#define	DONT_INTERSECT    0
#define	DO_INTERSECT      1
#define COLLINEAR         2

/**************************************************************
 *                                                            *
 *    NOTE:  The following macro to determine if two numbers  *
 *    have the same sign, is for 2's complement number        *
 *    representation.  It will need to be modified for other  *
 *    number systems.                                         *
 *                                                            *
 **************************************************************/

#define SAME_SIGNS( a, b )	\
		(((int) ((unsigned int) a ^ (unsigned int) b)) >= 0 )


int lines_intersect(int x1,int y1,   /* First line segment */
				 int x2,int y2,

				 int x3,int y3,   /* Second line segment */
				 int x4,int y4,

				 int *x,
				 int *y         /* Output value:
										* point of intersection */
							 )
{
		int a1, a2, b1, b2, c1, c2; /* Coefficients of line eqns. */
		int r1, r2, r3, r4;         /* 'Sign' values */
		int denom, offset, num;     /* Intermediate values */

		/* Compute a1, b1, c1, where line joining points 1 and 2
		 * is "a1 x  +  b1 y  +  c1  =  0".
		 */

		a1 = y2 - y1;
		b1 = x1 - x2;
		c1 = x2 * y1 - x1 * y2;

		/* Compute r3 and r4.
		 */


		r3 = a1 * x3 + b1 * y3 + c1;
		r4 = a1 * x4 + b1 * y4 + c1;

		/* Check signs of r3 and r4.  If both point 3 and point 4 lie on
		 * same side of line 1, the line segments do not intersect.
		 */

		if ( r3 != 0 &&
				 r4 != 0 &&
				 SAME_SIGNS( r3, r4 ))
				return ( DONT_INTERSECT );

		/* Compute a2, b2, c2 */

		a2 = y4 - y3;
		b2 = x3 - x4;
		c2 = x4 * y3 - x3 * y4;

		/* Compute r1 and r2 */

		r1 = a2 * x1 + b2 * y1 + c2;
		r2 = a2 * x2 + b2 * y2 + c2;

		/* Check signs of r1 and r2.  If both point 1 and point 2 lie
		 * on same side of second line segment, the line segments do
		 * not intersect.
		 */

		if ( r1 != 0 &&
				 r2 != 0 &&
				 SAME_SIGNS( r1, r2 ))
				return ( DONT_INTERSECT );

		/* Line segments intersect: compute intersection point. 
		 */

		denom = a1 * b2 - a2 * b1;
		if ( denom == 0 )
				return ( COLLINEAR );
		offset = denom < 0 ? - denom / 2 : denom / 2;

		/* The denom/2 is to get rounding instead of truncating.  It
		 * is added or subtracted to the numerator, depending upon the
		 * sign of the numerator.
		 */

		num = b1 * c2 - b2 * c1;
		*x = ( num < 0 ? num - offset : num + offset ) / denom;

		num = a2 * c1 - a1 * c2;
		*y = ( num < 0 ? num - offset : num + offset ) / denom;

		return ( DO_INTERSECT );
		} /* lines_intersect */

/* A main program to test the function.
 */

bool Line::Intersect(const Line& src,Vec3 &sect)
{
	int x,y;

	int ret = lines_intersect( int(mP1.x), int(mP1.y),
														 int(mP2.x), int(mP2.y),
														 int(src.mP1.x), int(src.mP1.y),
														 int(src.mP2.x), int(src.mP2.y),&x,&y );

	if ( ret == DO_INTERSECT )
	{
		sect.x = float(x);
		sect.y = float(y);
		sect.z = 0;
		return true;
	}
	return false;
}



//void BoundingBox::TransformBoundAABB(const MyMatrix &transform,const BoundingBox &b)
//{
//	InitMinMax();
//	BoundTest(transform,b.bmin.x,b.bmin.y,b.bmin.z);
//	BoundTest(transform,b.bmax.x,b.bmin.y,b.bmin.z);
//	BoundTest(transform,b.bmax.x,b.bmax.y,b.bmin.z);
//	BoundTest(transform,b.bmin.x,b.bmax.y,b.bmin.z);
//	BoundTest(transform,b.bmin.x,b.bmin.y,b.bmax.z);
//	BoundTest(transform,b.bmax.x,b.bmin.y,b.bmax.z);
//	BoundTest(transform,b.bmax.x,b.bmax.y,b.bmax.z);
//	BoundTest(transform,b.bmin.x,b.bmax.y,b.bmax.z);
//}
//
//
//void BoundingBox::BoundTest(const MyMatrix &transform,float x,float y,float z)
//{
//	Vec3 pos(x,y,z);
//	Vec3 t;
//	transform.Transform(pos,t);
//	MinMax(t);
//};

