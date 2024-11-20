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

#include "quat.h"

#include <math.h>


#define DELTA 1e-6     // error tolerance


static float ranf(void)
{
	float v = (float) rand() *(1.0f/32767.0f);
	return v;
}


/*SDOC***********************************************************************

	Name:		gluQuatSlerp_EXT

	Action:	Smoothly (spherically, shortest path on a quaternion sphere)
			interpolates between two UNIT quaternion positions

	Params:   GLQUAT (first and second quaternion), float (interpolation
			parameter [0..1]), GL_QUAT (resulting quaternion; inbetween)

	Returns:  nothing

	Comments: Most of this code is optimized for speed and not for readability

			As t goes from 0 to 1, qt goes from p to q.
		slerp(p,q,t) = (p*sin((1-t)*omega) + q*sin(t*omega)) / sin(omega)

***********************************************************************EDOC*/
void  Quat::Slerp(const Quat &from,const Quat &to,float t)  // smooth interpolation.
{
	float           to1[4];
	float           omega, cosom, sinom;
	float           scale0, scale1;

	// calc cosine
	cosom = from.q.x * to.q.x + from.q.y * to.q.y + from.q.z * to.q.z + from.w * to.w;

	// adjust signs (if necessary)
	if ( cosom < 0.0f )
	{
		cosom  = -cosom;
		to1[0] = -to.q.x;
		to1[1] = -to.q.y;
		to1[2] = -to.q.z;
		to1[3] = -to.w;
	}
	else
	{
		to1[0] = to.q.x;
		to1[1] = to.q.y;
		to1[2] = to.q.z;
		to1[3] = to.w;
	}

	// calculate coefficients
	if ( (1.0f - cosom) > DELTA )
	{
		// standard case (slerp)
		omega = (float)acos(cosom);
		sinom = (float)sin(omega);
		scale0 = (float)sin((1.0f - t) * omega) / sinom;
		scale1 = (float)sin(t * omega) / sinom;
	}
	else
	{
		// "from" and "to" quaternions are very close
		//  ... so we can do a linear interpolation
		scale0 = 1.0f - t;
		scale1 = t;
	}

	// calculate final values
	q.x = scale0 * from.q.x + scale1 * to1[0];
	q.y = scale0 * from.q.y + scale1 * to1[1];
	q.z = scale0 * from.q.z + scale1 * to1[2];
	w   = scale0 * from.w + scale1 * to1[3];

}



/*SDOC***********************************************************************

	Name:		gluQuatLerp_EXT

	Action:   Linearly interpolates between two quaternion positions

	Params:   GLQUAT (first and second quaternion), float (interpolation
			parameter [0..1]), GL_QUAT (resulting quaternion; inbetween)

	Returns:  nothing

	Comments: fast but not as nearly as smooth as Slerp

***********************************************************************EDOC*/
void  Quat::Lerp(const Quat &from,const Quat &to,float t)   // fast interpolation, not as smooth.
{
	float           to1[4];
	float           cosom;
	float           scale0, scale1;

	// calc cosine
	cosom = from.q.x * to.q.x + from.q.y * to.q.y + from.q.z * to.q.z + from.w * to.w;

	// adjust signs (if necessary)
	if ( cosom < 0.0f )
	{
		to1[0] = -to.q.x;
		to1[1] = -to.q.y;
		to1[2] = -to.q.z;
		to1[3] = -to.w;
	}
	else
	{
		to1[0] = to.q.x;
		to1[1] = to.q.y;
		to1[2] = to.q.z;
		to1[3] = to.w;
	}
	// interpolate linearly
	scale0 = 1.0f - t;
	scale1 = t;

	// calculate final values
	q.x = scale0 * from.q.x + scale1 * to1[0];
	q.y = scale0 * from.q.y + scale1 * to1[1];
	q.z = scale0 * from.q.z + scale1 * to1[2];
	w   = scale0 * from.w +   scale1 * to1[3];

}


void Quat::RandomRotation(bool x,bool y,bool z)
{
	float ex = 0;
	float ey = 0;
	float ez = 0;

	if ( x )
	{
		ex = ranf()*PI*2;
	}
	if ( y )
	{
		ey = ranf()*PI*2;
	}
	if ( z )
	{
		ez = ranf()*PI*2;
	}

	EulerToQuat(ex,ey,ez);

}

