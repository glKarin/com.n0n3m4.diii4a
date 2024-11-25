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
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "matrix.h"
#include "quat.h"


MyMatrix gIdentity;

static void ComputeEulerMatrix( MyMatrix &matrix,
																float cx, float cy, float cz,
																float sx, float sy, float sz )
{
	// After heavily refactoring the matrix math, this is the
	// final distilled product...

	matrix.mElement[0][0] = cz*cy;
	matrix.mElement[0][1] = sz*cy;
	matrix.mElement[0][2] = -sy;
	matrix.mElement[0][3] = 0.0f;

	matrix.mElement[1][0] = cz*sy*sx - sz*cx;
	matrix.mElement[1][1] = sz*sy*sx + cz*cx;
	matrix.mElement[1][2] = cy*sx;
	matrix.mElement[1][3] = 0.0f;

	matrix.mElement[2][0] = cz*sy*cx + sz*sx;
	matrix.mElement[2][1] = sz*sy*cx - cz*sx;
	matrix.mElement[2][2] = cy*cx;
	matrix.mElement[2][3] = 0.0f;

	matrix.mElement[3][0] = 0.0f;
	matrix.mElement[3][1] = 0.0f;
	matrix.mElement[3][2] = 0.0f;
	matrix.mElement[3][3] = 1.0f;
}


void MyMatrix::Rotate(float x,float y,float z)
{
	//
	ComputeEulerMatrix( *this,
											cosf(x), cosf(y), cosf(z),
											sinf(x), sinf(y), sinf(z) );
}

void MyMatrix::Rotate(float angle,float x,float y,float z)
{
	Quat q;
	Vec3 axis(x,y,z);
	q.AngleAxis(angle,axis);
	q.QuatToMatrix(*this);
}

//***************************************************************************
void MyMatrix::Set(const Vec3 &facing,const Vec3 &N)
{
	//n cross ((h cross n) /|h cross n|)
	Vec3 r0;
	r0.Cross(facing,N);
	r0.Normalize();
	Vec3 v0;
	v0.Cross(N,r0);

	Vec3 v1;

	v1.Cross(N,v0);

	v1.Normalize();

	mElement[0][0] = v0.x;
	mElement[0][1] = v0.y;
	mElement[0][2] = v0.z;
	mElement[0][3] = 0;

	mElement[1][0] = v1.x;
	mElement[1][1] = v1.y;
	mElement[1][2] = v1.z;
	mElement[1][3] = 0;

	mElement[2][0] = N.x;
	mElement[2][1] = N.y;
	mElement[2][2] = N.z;
	mElement[2][3] = 0;

	mElement[3][0] = 0;
	mElement[3][1] = 0;
	mElement[3][2] = 0;
	mElement[3][3] = 1;
}


void MyMatrix::GetPose(const Vec3 &pos,
					   const Quat &rot,
					   Vec3 &tpos,
					   Quat &trot) const
{
	MyMatrix child;
	MyMatrix combine;

	rot.QuatToMatrix(child);
	child.SetTranslation(pos);

	combine.Multiply(child,*this);

	combine.GetTranslation(tpos);
	trot.MatrixToQuat(combine);

}

void MyMatrix::Set(const char *data)
{
	int index = 0;

	Identity();

	while ( *data && *data == 32 ) data++;

	while ( *data )
	{
		float v = (float)atof( data );

		int iy = index / 4;
		int ix = index % 4;

		mElement[ix][iy] = v;

		while ( *data && *data != 32 ) data++;
		while ( *data && *data == 32 ) data++;

		index++;
	}
}

MyMatrix::MyMatrix(const float *quat,const float *pos)
{
	if ( quat )
	{
		Quat q(quat);
		q.QuatToMatrix(*this);
	}
	else
	{
		Identity();
	}
	if ( pos ) SetTranslation(pos);
}

