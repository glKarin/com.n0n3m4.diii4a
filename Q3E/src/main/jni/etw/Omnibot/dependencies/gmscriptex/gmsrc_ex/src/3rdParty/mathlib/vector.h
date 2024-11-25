#ifndef VECTOR_H

#define VECTOR_H

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



#ifdef _WIN32
#pragma warning(disable:4786)
#endif

#include <math.h>
#include <float.h>
#include <vector>



const float DEG_TO_RAD = ((2.0f * 3.14152654f) / 360.0f);
const float RAD_TO_DEG = (360.0f / (2.0f * 3.141592654f));

class Vec3
{
public:
	Vec3() { };

	Vec3(float a,float b,float c)
	{
		x = a;
		y = b;
		z = c;
	}
	Vec3(const Vec3 &a)
	{
		x = a.x;
		y = a.y;
		z = a.z;
	}
	Vec3(const float *t)
	{
		x = t[0];
		y = t[1];
		z = t[2];
	}
	Vec3(const double *t)
	{
		x = (float)t[0];
		y = (float)t[1];
		z = (float)t[2];
	}
	Vec3(const int *t)
	{
		x = (float)t[0];
		y = (float)t[1];
		z = (float)t[2];
	}
	operator const float *() const { return (const float*)this;  }
	operator float *() { return (float*)this;  }

	bool operator==(const Vec3 &a) const
	{
		return( a.x == x && a.y == y && a.z == z );
	}
	bool operator!=(const Vec3 &a) const
	{
		return( a.x != x || a.y != y || a.z != z );
	}
	// Operators
	Vec3& operator = (const Vec3& A) 
	{
		x=A.x; y=A.y; z=A.z;
		return(*this);  
	}
	Vec3 operator + (const Vec3& A) const
	{
		Vec3 Sum(x+A.x, y+A.y, z+A.z);
		return(Sum); 
	}
	Vec3 operator - (const Vec3& A) const
	{
		Vec3 Diff(x-A.x, y-A.y, z-A.z);
		return(Diff); 
	}
	Vec3 operator * (const float s) const
	{
		Vec3 Scaled(x*s, y*s, z*s);
		return(Scaled); 
	}
	Vec3 operator + (const float s) const
	{
		Vec3 Scaled(x+s, y+s, z+s);
		return(Scaled); 
	}
	Vec3 operator / (const float s) const
	{
		float r = 1.0f / s;
		Vec3 Scaled(x*r, y*r, z*r);
		return(Scaled);
	}
	void operator /= (float A)
	{
		x/=A; y/=A; z/=A; 
	}
	void operator += (const Vec3 A)
	{
		x+=A.x; y+=A.y; z+=A.z; 
	}
	void operator -= (const Vec3 A)
	{
		x-=A.x; y-=A.y; z-=A.z; 
	}
	void operator *= (const float s)
	{
		x*=s; y*=s; z*=s;
	}
	void operator += (const float A)
	{
		x+=A; y+=A; z+=A; 
	}
	Vec3 operator - () const
	{
		return Vec3(-x, -y, -z); 
	}
	float operator [] (const int i) const
	{
		return( (i==0)?x:((i==1)?y:z) ); 
	}
	float & operator [] (const int i)
	{
		return( (i==0)?x:((i==1)?y:z) ); 
	}
	//
	bool IsSame(const Vec3 &v,float epsilon) const
	{
		float dx = fabsf( x - v.x );
		if ( dx > epsilon ) return false;
		float dy = fabsf( y - v.y );
		if ( dy > epsilon ) return false;
		float dz = fabsf( z - v.z );
		if ( dz > epsilon ) return false;
		return true;
	}
	float ComputeNormal(const Vec3 &A,const Vec3 &B,const Vec3 &C)
	{
		float vx,vy,vz,wx,wy,wz,vw_x,vw_y,vw_z,mag;

		vx = (B.x - C.x);
		vy = (B.y - C.y);
		vz = (B.z - C.z);

		wx = (A.x - B.x);
		wy = (A.y - B.y);
		wz = (A.z - B.z);

		vw_x = vy * wz - vz * wy;
		vw_y = vz * wx - vx * wz;
		vw_z = vx * wy - vy * wx;

		mag = sqrtf((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

		if ( mag < 0.000001f )
		{
			mag = 0;
		}
		else
		{
			mag = 1.0f/mag;
		}

		x = vw_x * mag;
		y = vw_y * mag;
		z = vw_z * mag;

		return mag;
	}
	void ScaleSumScale(float c0,float c1,const Vec3 &pos)
	{
		x = (x*c0) + (pos.x*c1);
		y = (y*c0) + (pos.y*c1);
		z = (z*c0) + (pos.z*c1);
	}
	void SwapYZ()
	{
		float t = y;
		y = z;
		z = t;
	}
	void Get(float *v) const
	{
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}
	void Set(const int *p)
	{
		x = (float) p[0];
		y = (float) p[1];
		z = (float) p[2];
	}
	void Set(const float *p)
	{
		x = p[0];
		y = p[1];
		z = p[2];
	}
	void Set(float a,float b,float c)
	{
		x = a;
		y = b;
		z = c;
	}
	void Zero()
	{
		x = y = z = 0;
	};

	const float* Ptr() const { return &x; }
	float* Ptr() { return &x; }

// return -(*this).
	Vec3 negative() const
	{
		return Vec3(-x,-y,-z);
	}
	float Magnitude() const
	{
		return sqrt(x * x + y * y + z * z);
	}
	void Lerp(const Vec3& from,const Vec3& to,float slerp)
	{
		x = ((to.x - from.x) * slerp) + from.x;
		y = ((to.y - from.y) * slerp) + from.y;
		z = ((to.z - from.z) * slerp) + from.z;
	}
	// Highly specialized interpolate routine.  Will compute the interpolated position
	// shifted forward or backwards along the ray defined between (from) and (to).
	// Reason for existance is so that when a bullet collides with a wall, for
	// example, you can generate a graphic effect slightly *before* it hit the
	// wall so that the effect doesn't sort into the wall itself.
	void Interpolate(const Vec3 &from,const Vec3 &to,float offset)
	{
		x = to.x-from.x;
		y = to.y-from.y;
		z = to.z-from.z;
		float d = sqrtf( x*x + y*y + z*z );
		float recip = 1.0f / d;
		x*=recip;
		y*=recip;
		z*=recip; // normalize vector
		d+=offset; // shift along ray
		x = x*d + from.x;
		y = y*d + from.y;
		z = z*d + from.z;
	}
/** Computes the reflection vector between two vectors.*/
	void Reflection(const Vec3 &a,const Vec3 &b)
	{
		Vec3 c;
		Vec3 d;

		float dot = a.Dot(b) * 2.0f;

		c = b * dot;

		d = c - a;

		x = -d.x;
		y = -d.y;
		z = -d.z;
	}
	void AngleAxis(float angle,const Vec3& axis)
	{
		x = axis.x*angle;
		y = axis.y*angle;
		z = axis.z*angle;
	}
	float Length() const
	{
		return float(sqrt( x*x + y*y + z*z ));
	}
	float ComputePlane(const Vec3 &A,const Vec3 &B,const Vec3 &C)
	{
		float vx,vy,vz,wx,wy,wz,vw_x,vw_y,vw_z,mag;

		vx = (B.x - C.x);
		vy = (B.y - C.y);
		vz = (B.z - C.z);

		wx = (A.x - B.x);
		wy = (A.y - B.y);
		wz = (A.z - B.z);

		vw_x = vy * wz - vz * wy;
		vw_y = vz * wx - vx * wz;
		vw_z = vx * wy - vy * wx;

		mag = sqrtf((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

		if ( mag < 0.000001f )
		{
			mag = 0;
		}
		else
		{
			mag = 1.0f/mag;
		}

		x = vw_x * mag;
		y = vw_y * mag;
		z = vw_z * mag;


		float D = 0.0f - ((x*A.x)+(y*A.y)+(z*A.z));

		return D;
	}	
	float Length2() const         // squared distance, prior to square root.
	{
		float l2 = x*x+y*y+z*z;
		return l2;
	}
	float Distance(const Vec3 &a) const   // distance between two points.
	{
		Vec3 d(a.x-x,a.y-y,a.z-z);
		return d.Length();
	}	
	float DistanceXY(const Vec3 &a) const
	{
		float dx = a.x - x;
		float dy = a.y - y;
		float dist = dx*dx + dy*dy;
		return dist;
	}
	float Distance2(const Vec3 &a) const  // squared distance.
	{
		float dx = a.x - x;
		float dy = a.y - y;
		float dz = a.z - z;
		return dx*dx + dy*dy + dz*dz;
	}
	float Partial(const Vec3 &p) const
	{
		return (x*p.y) - (p.x*y);
	}
	float Area(const Vec3 &p1,const Vec3 &p2) const
	{
		float A = Partial(p1);
		A+= p1.Partial(p2);
		A+= p2.Partial(*this);
		return A*0.5f;
	}
	inline float Normalize()       // normalize to a unit vector, returns distance.
	{
		float d = sqrtf( static_cast< float >( x*x + y*y + z*z ) );
		if ( d > 0 )
		{
			float r = 1.0f / d;
			x *= r;
			y *= r;
			z *= r;
		}
		else
		{
			x = y = z = 1;
		}
		return d;
	}	
	float Dot(const Vec3 &a) const
	{
		return (x * a.x + y * a.y + z * a.z );
	}
	Vec3 Cross(const Vec3& other) const
	{
 		return Vec3( y*other.z - z*other.y,  z*other.x - x*other.z,  x*other.y - y*other.x );
	}
	void Cross(const Vec3 &a,const Vec3 &b)  // cross two vectors result in this one.
	{
		x = a.y*b.z - a.z*b.y;
		y = a.z*b.x - a.x*b.z;
		z = a.x*b.y - a.y*b.x;
	}
	/******************************************/
	// Check if next edge (b to c) turns inward
	//
	//    Edge from a to b is already in face
	//    Edge from b to c is being considered for addition to face
	/******************************************/
	bool Concave(const Vec3& a,const Vec3& b)
	{
		float vx,vy,vz,wx,wy,wz,vw_x,vw_y,vw_z,mag,nx,ny,nz,mag_a,mag_b;

		wx = b.x - a.x;
		wy = b.y - a.y;
		wz = b.z - a.z;

		mag_a = (float) sqrtf((wx * wx) + (wy * wy) + (wz * wz));

		vx = x - b.x;
		vy = y - b.y;
		vz = z - b.z;

		mag_b = (float) sqrtf((vx * vx) + (vy * vy) + (vz * vz));

		vw_x = (vy * wz) - (vz * wy);
		vw_y = (vz * wx) - (vx * wz);
		vw_z = (vx * wy) - (vy * wx);

		mag = (float) sqrtf((vw_x * vw_x) + (vw_y * vw_y) + (vw_z * vw_z));

		// Check magnitude of cross product, which is a sine function
		// i.e., mag (a x b) = mag (a) * mag (b) * sin (theta);
		// If sin (theta) small, then angle between edges is very close to
		// 180, which we may want to call a concavity.	Setting the
		// CONCAVITY_TOLERANCE value greater than about 0.01 MAY cause
		// face consolidation to get stuck on particular face.	Most meshes
		// convert properly with a value of 0.0

		if (mag/(mag_a*mag_b) <= 0.0f )	return true;

		mag = 1.0f / mag;

		nx = vw_x * mag;
		ny = vw_y * mag;
		nz = vw_z * mag;

		// Dot product of tri normal with cross product result will
		// yield positive number if edges are convex (+1.0 if two tris
		// are coplanar), negative number if edges are concave (-1.0 if
		// two tris are coplanar.)

		mag = ( x * nx) + ( y * ny) + ( z * nz);

		if (mag > 0.0f ) return false;

		return(true);
	}
	bool PointTestXY(const Vec3 &i,const Vec3 &j) const
	{
		if (((( i.y <= y ) && ( y  < j.y )) ||
				 (( j.y <= y ) && ( y  < i.y ))) &&
					( x < (j.x - i.x) * (y - i.y) / (j.y - i.y) + i.x)) return true;
		return false;
	}
	// test to see if this point is inside the triangle specified by
	// these three points on the X/Y plane.
	bool PointInTriXY(const Vec3 &p1,const Vec3 &p2,const Vec3 &p3) const
	{
		float ax  = p3.x - p2.x;
		float ay  = p3.y - p2.y;
		float bx  = p1.x - p3.x;
		float by  = p1.y - p3.y;
		float cx  = p2.x - p1.x;
		float cy  = p2.y - p1.y;
		float apx = x - p1.x;
		float apy = y - p1.y;
		float bpx = x - p2.x;
		float bpy = y - p2.y;
		float cpx = x - p3.x;
		float cpy = y - p3.y;

		float aCROSSbp = ax*bpy - ay*bpx;
		float cCROSSap = cx*apy - cy*apx;
		float bCROSScp = bx*cpy - by*cpx;

		return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
	}
	// test to see if this point is inside the triangle specified by
	// these three points on the X/Y plane.
	bool PointInTriYZ(const Vec3 &p1,const Vec3 &p2,const Vec3 &p3) const
	{
		float ay  = p3.y - p2.y;
		float az  = p3.z - p2.z;
		float by  = p1.y - p3.y;
		float bz  = p1.z - p3.z;
		float cy  = p2.y - p1.y;
		float cz  = p2.z - p1.z;
		float apy = y - p1.y;
		float apz = z - p1.z;
		float bpy = y - p2.y;
		float bpz = z - p2.z;
		float cpy = y - p3.y;
		float cpz = z - p3.z;

		float aCROSSbp = ay*bpz - az*bpy;
		float cCROSSap = cy*apz - cz*apy;
		float bCROSScp = by*cpz - bz*cpy;

		return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
	}
	// test to see if this point is inside the triangle specified by
	// these three points on the X/Y plane.
	bool PointInTriXZ(const Vec3 &p1,const Vec3 &p2,const Vec3 &p3) const
	{
		float az  = p3.z - p2.z;
		float ax  = p3.x - p2.x;
		float bz  = p1.z - p3.z;
		float bx  = p1.x - p3.x;
		float cz  = p2.z - p1.z;
		float cx  = p2.x - p1.x;
		float apz = z - p1.z;
		float apx = x - p1.x;
		float bpz = z - p2.z;
		float bpx = x - p2.x;
		float cpz = z - p3.z;
		float cpx = x - p3.x;

		float aCROSSbp = az*bpx - ax*bpz;
		float cCROSSap = cz*apx - cx*apz;
		float bCROSScp = bz*cpx - bx*cpz;

		return ((aCROSSbp >= 0.0f) && (bCROSScp >= 0.0f) && (cCROSSap >= 0.0f));
	}
	// Given a point and a line (defined by two points), compute the closest point
	// in the line.  (The line is treated as infinitely long.)
	void NearestPointInLine(const Vec3 &point,const Vec3 &line0,const Vec3 &line1)
	{
		Vec3 &nearestPoint = *this;
		Vec3 lineDelta     = line1 - line0;

		// Handle degenerate lines
		if ( lineDelta == Vec3(0, 0, 0) )
		{
			nearestPoint = line0;
		}
		else
		{
			float delta = (point-line0).Dot(lineDelta) / (lineDelta).Dot(lineDelta);
			nearestPoint = line0 + lineDelta*delta;
		}
	}
	// Given a point and a line segment (defined by two points), compute the closest point
	// in the line.  Cap the point at the endpoints of the line segment.
	void NearestPointInLineSegment(const Vec3 &point,const Vec3 &line0,const Vec3 &line1)
	{
		Vec3 &nearestPoint = *this;
		Vec3 lineDelta     = line1 - line0;

		// Handle degenerate lines
		if ( lineDelta == Vec3(0, 0, 0) )
		{
			nearestPoint = line0;
		}
		else
		{
			float delta = (point-line0).Dot(lineDelta) / (lineDelta).Dot(lineDelta);

			// Clamp the point to conform to the segment's endpoints
			if ( delta < 0 )
				delta = 0;
			else if ( delta > 1 )
				delta = 1;

			nearestPoint = line0 + lineDelta*delta;
		}
	}
	// Given a point and a plane (defined by three points), compute the closest point
	// in the plane.  (The plane is unbounded.)
	void NearestPointInPlane(
		const Vec3 &point,
		const Vec3 &triangle0,
		const Vec3 &triangle1,
		const Vec3 &triangle2)
	{
		Vec3 &nearestPoint = *this;
		Vec3 lineDelta0    = triangle1 - triangle0;
		Vec3 lineDelta1    = triangle2 - triangle0;
		Vec3 pointDelta    = point - triangle0;
		Vec3 normal;

		// Get the normal of the polygon (doesn't have to be a unit vector)
		normal.Cross(lineDelta0, lineDelta1);

		float delta = normal.Dot(pointDelta) / normal.Dot(normal);
		nearestPoint = point - normal*delta;
	}

	// Given a point and a plane (defined by a coplanar point and a normal), compute the closest point
	// in the plane.  (The plane is unbounded.)
	void NearestPointInPlane(const Vec3 &point,const Vec3 &planePoint,const Vec3 &planeNormal)
	{
		Vec3 &nearestPoint = *this;
		Vec3 pointDelta    = point - planePoint;

		float delta = planeNormal.Dot(pointDelta) / planeNormal.Dot(planeNormal);
		nearestPoint = point - planeNormal*delta;
	}

	// Given a point and a triangle (defined by three points), compute the closest point
	// in the triangle.  Clamp the point so it's confined to the area of the triangle.
	void NearestPointInTriangle(
		const Vec3 &point,
		const Vec3 &triangle0,
		const Vec3 &triangle1,
		const Vec3 &triangle2)
	{
		Vec3 &nearestPoint = *this;
		Vec3 lineDelta0 = triangle1 - triangle0;
		Vec3 lineDelta1 = triangle2 - triangle0;

		// Handle degenerate triangles
		if ( (lineDelta0 == Vec3(0, 0, 0)) || (lineDelta1 == Vec3(0, 0, 0)) )
		{
			nearestPoint.NearestPointInLineSegment(point, triangle1, triangle2);
		}
		else if ( lineDelta0 == lineDelta1 )
		{
			nearestPoint.NearestPointInLineSegment(point, triangle0, triangle1);
		}
		else
		{
			static Vec3 axis[3];
			axis[0].NearestPointInLine(triangle0, triangle1, triangle2);
			axis[1].NearestPointInLine(triangle1, triangle0, triangle2);
			axis[2].NearestPointInLine(triangle2, triangle0, triangle1);

			float axisDot[3];
			axisDot[0] = (triangle0-axis[0]).Dot(point-axis[0]);
			axisDot[1] = (triangle1-axis[1]).Dot(point-axis[1]);
			axisDot[2] = (triangle2-axis[2]).Dot(point-axis[2]);

			bool            bForce         = true;
			float            bestMagnitude2 = 0;
			float            closeMagnitude2;
			Vec3		closePoint;

			if ( axisDot[0] < 0 )
			{
				closePoint.NearestPointInLineSegment(point, triangle1, triangle2);
				closeMagnitude2 = point.Distance2(closePoint);
				if ( bForce || (bestMagnitude2 > closeMagnitude2) )
				{
					bForce         = false;
					bestMagnitude2 = closeMagnitude2;
					nearestPoint   = closePoint;
				}
			}
			if ( axisDot[1] < 0 )
			{
				closePoint.NearestPointInLineSegment(point, triangle0, triangle2);
				closeMagnitude2 = point.Distance2(closePoint);
				if ( bForce || (bestMagnitude2 > closeMagnitude2) )
				{
					bForce         = false;
					bestMagnitude2 = closeMagnitude2;
					nearestPoint   = closePoint;
				}
			}
			if ( axisDot[2] < 0 )
			{
				closePoint.NearestPointInLineSegment(point, triangle0, triangle1);
				closeMagnitude2 = point.Distance2(closePoint);
				if ( bForce || (bestMagnitude2 > closeMagnitude2) )
				{
					bForce         = false;
					bestMagnitude2 = closeMagnitude2;
					nearestPoint   = closePoint;
				}
			}

			// If bForce is true at this point, it means the nearest point lies
			// inside the triangle; use the nearest-point-on-a-plane equation
			if ( bForce )
			{
				Vec3 normal;

				// Get the normal of the polygon (doesn't have to be a unit vector)
				normal.Cross(lineDelta0, lineDelta1);

				Vec3 pointDelta = point - triangle0;
				float delta = normal.Dot(pointDelta) / normal.Dot(normal);

				nearestPoint = point - normal*delta;
			}
		}
	}

	float GetYaw() const
	{
		return -atan2f(-x, y);
	}

	float GetPitch() const
	{
		return asinf(z);
	}

	void FromSpherical( float heading, float pitch, float radius ) 
	{ 
		float 
			fST = sinf(heading), fCT = cosf(heading), 
			fSP = sinf(pitch), fCP = cosf(pitch);
		*this = Vec3(fCP*fST, fCP*fCT, fSP) * radius;
	}

	void ToSpherical( float &heading, float &pitch, float &radius )
	{
		// reference vector is 0,0,1
		radius = Length();
		pitch = radius > 0.0f ? asinf(z/radius) : 0.0f;
		heading = atan2f(x, y);
	}

//private:

	float x;
	float y;
	float z;
};


class Vec2
{
public:
	Vec2() { };
	Vec2(const Vec2 &a)
	{
		x = a.x;
		y = a.y;
	}
	Vec2(const float *t)
	{
		x = t[0];
		y = t[1];
	}
	Vec2(float a,float b)
	{
		x = a;
		y = b;
	}
	const float* Ptr() const { return &x; }
	float* Ptr() { return &x; }
	Vec2 & operator+=(const Vec2 &a)
	{
		x+=a.x;
		y+=a.y;
		return *this;
	}
	Vec2 & operator-=(const Vec2 &a)
	{
		x-=a.x;
		y-=a.y;
		return *this;
	}
	Vec2 & operator*=(const Vec2 &a)
	{
		x*=a.x;
		y*=a.y;
		return *this;
	}
	Vec2 & operator/=(const Vec2 &a)
	{
		x/=a.x;
		y/=a.y;
		return *this;
	}
	bool operator==(const Vec2 &a) const
	{
		if ( a.x == x && a.y == y ) return true;
		return false;
	}
	bool operator!=(const Vec2 &a) const
	{
		if ( a.x != x || a.y != y ) return true;
		return false;
	}
	Vec2 operator+(Vec2 a) const
	{
		a.x+=x;
		a.y+=y;
		return a;
	}
	Vec2 operator-(Vec2 a) const
	{
		a.x = x-a.x;
		a.y = y-a.y;
		return a;
	}
	Vec2 operator - () const
	{
		return negative();
	}
	Vec2 operator*(Vec2 a) const
	{
		a.x*=x;
		a.y*=y;
		return a;
	}
	Vec2 operator*(float c) const
	{
		return Vec2(x * c,y * c);
	}
	Vec2 operator/(const Vec2 &a)
	{
		return Vec2(x/a.x,y/a.y);
	}
	float Dot(const Vec2 &a) const
	{
		return (x * a.x + y * a.y );
	}	
	void Set(const int *p)
	{
		x = (float) p[0];
		y = (float) p[1];
	}
	void Set(const float *p)
	{
		x = p[0];
		y = p[1];
	}
	void Set(float a,float b)
	{
		x = a;
		y = b;
	}
	void Zero()
	{
		x = y = 0;
	}
	Vec2 negative() const
	{
		return Vec2(-x,-y);
	}
	float magnitude() const
	{
		return (float)sqrtf(x * x + y * y);
	}	
	void Reflection(const Vec2 &a,const Vec2 &b);
	float Length() const
	{
		return float(sqrtf( x*x + y*y ));
	}	
	float Length2()
	{
		return x*x+y*y;
	}
	float Distance(const Vec2 &a) const
	{
		float dx = a.x - x;
		float dy = a.y - y;
		float d  = dx*dx+dy*dy;
		return sqrtf(d);
	}	
	float Distance2(const Vec2 &a)
	{
		float dx = a.x - x;
		float dy = a.y - y;
		return dx*dx + dy *dy;
	}
	void Lerp(const Vec2& from,const Vec2& to,float slerp)
	{
		x = ((to.x - from.x)*slerp) + from.x;
		y = ((to.y - from.y)*slerp) + from.y;
	}
	void Cross(const Vec2 &a,const Vec2 &b) 
	{
		x = a.y*b.x - a.x*b.y;
		y = a.x*b.x - a.x*b.x;
	}
	float Normalize()
	{
		float l = Length();
		if ( l != 0 )
		{
			l = float( 1 ) / l;
			x*=l;
			y*=l;
		}
		else
		{
			x = y = 0;
		}
		return l;
	}
	
	float x;
	float y;
};

class Line
{
public:
	Line(const Vec3 &from,const Vec3 &to)
	{
		mP1 = from;
		mP2 = to;
	}
	// JWR  Test for the intersection of two lines.
	bool Intersect(const Line& src,Vec3 &sect);
private:
	Vec3 mP1;
	Vec3 mP2;
};

inline Vec3 operator*(float s, const Vec3 &v)
{
	return Vec3(v.x*s, v.y*s, v.z*s);
}
inline Vec2 operator*(float s, const Vec2 &v)
{
	return Vec2(v.x*s, v.y*s);
}

class MyMatrix;

//class BoundingBox
//{
//public:
//	void InitMinMax()
//	{
//		bmin.Set(FLT_MAX,FLT_MAX,FLT_MAX);
//		bmax.Set(-FLT_MAX,-FLT_MAX,-FLT_MAX);
//	}
//	void MinMax(const Vec3 &p)
//	{
//		if ( p.x < bmin.x ) bmin.x = p.x;
//		if ( p.y < bmin.y ) bmin.y = p.y;
//		if ( p.z < bmin.z ) bmin.z = p.z;
//
//		if ( p.x > bmax.x ) bmax.x = p.x;
//		if ( p.y > bmax.y ) bmax.y = p.y;
//		if ( p.z > bmax.z ) bmax.z = p.z;
//	}
//	void MinMax(const BoundingBox &b)
//	{
//		if ( b.bmin.x < bmin.x ) bmin.x = b.bmin.x;
//		if ( b.bmin.y < bmin.y ) bmin.y = b.bmin.y;
//		if ( b.bmin.z < bmin.z ) bmin.z = b.bmin.z;
//
//		if ( b.bmax.x > bmax.x ) bmax.x = b.bmax.x;
//		if ( b.bmax.y > bmax.y ) bmax.y = b.bmax.y;
//		if ( b.bmax.z > bmax.z ) bmax.z = b.bmax.z;
//	}
//	void GetCenter(Vec3 &center) const
//	{
//		center.x = (bmax.x - bmin.x)*0.5f + bmin.x;
//		center.y = (bmax.y - bmin.y)*0.5f + bmin.y;
//		center.z = (bmax.z - bmin.z)*0.5f + bmin.z;
//	}
//	void GetSides(Vec3 &sides) const
//	{
//		sides.x = bmax.x - bmin.x;
//		sides.y = bmax.y - bmin.y;
//		sides.z = bmax.z - bmin.z;
//	}
//	float GetLength() const
//	{
//		return bmin.Distance(bmax);
//	}
//	void TransformBoundAABB(const MyMatrix &t,const BoundingBox &b);
//
//	void BoundTest(const MyMatrix &transform,float x,float y,float z);
//
//	Vec3 bmin;
//	Vec3 bmax;
//};

class BoundingBox
{
public:
	bool IsZero() const
	{
		for(int i = 0; i < 3; ++i)
		{
			if(mMins[i] != 0.f || 
				mMaxs[i] != 0.f)
				return false;
		}
		return true;
	}

	void Set(const Vec3 &_pt)
	{
		for(int i = 0; i < 3; ++i)
		{
			mMins[i] = _pt[i];
			mMaxs[i] = _pt[i];
		}
	}
	void SetMinMax(const Vec3 &_min, const Vec3 &_max)
	{
		mMins.x = _min.x < _max.x ? _min.x : _max.x;
		mMaxs.x = _min.x > _max.x ? _min.x : _max.x;

		mMins.y = _min.y < _max.y ? _min.y : _max.y;
		mMaxs.y = _min.y > _max.y ? _min.y : _max.y;

		mMins.z = _min.z < _max.z ? _min.z : _max.z;
		mMaxs.z = _min.z > _max.z ? _min.z : _max.z;
	}
	void CenterPoint(Vec3 &_out) const
	{
		_out.x = (mMins.x + mMaxs.x) * 0.5f;
		_out.y = (mMins.y + mMaxs.y) * 0.5f;
		_out.z = (mMins.z + mMaxs.z) * 0.5f;
	}
	void CenterTop(Vec3 &_out) const
	{
		_out.x = (mMins.x + mMaxs.x) * 0.5f;
		_out.y = (mMins.y + mMaxs.y) * 0.5f;
		_out.z = mMaxs.z;
	}
	void CenterBottom(Vec3 &_out) const
	{
		_out.x = (mMins.x + mMaxs.x) * 0.5f;
		_out.y = (mMins.y + mMaxs.y) * 0.5f;
		_out.z = mMins.z;
	}
	void MoveCenter(const Vec3 &_v)
	{
		Vec3 center;
		CenterPoint(center);
		mMins -= center;
		mMaxs -= center;

		mMins += _v;
		mMaxs += _v;
	}
	void ExpandWithPoint(const Vec3 &_pt)
	{
		if(_pt.x < mMins.x) mMins.x = _pt.x;
		if(_pt.x > mMaxs.x) mMaxs.x = _pt.x;

		if(_pt.y < mMins.y) mMins.y = _pt.y;
		if(_pt.y > mMaxs.y) mMaxs.y = _pt.y;

		if(_pt.z < mMins.z) mMins.z = _pt.z;
		if(_pt.z > mMaxs.z) mMaxs.z = _pt.z;
	}
	void ExpandWithBounds(const BoundingBox &_bbox)
	{
		ExpandWithPoint(_bbox.mMins);
		ExpandWithPoint(_bbox.mMaxs);
	}
	void Expand(float _expand)
	{
		for(int i = 0; i < 3; ++i)
		{
			mMins[i] -= _expand;
			mMaxs[i] += _expand;
		}
	}
	void ExpandX(float _expand) { mMins.x -= _expand; mMaxs.x += _expand; }
	void ExpandY(float _expand) { mMins.y -= _expand; mMaxs.y += _expand; }
	void ExpandZ(float _expand) { mMins.z -= _expand; mMaxs.z += _expand; }
	bool Intersects(const BoundingBox &_bbox) const
	{
		for (int i = 0; i < 3; i++)
		{
			if (mMaxs[i] < _bbox.mMins[i] || mMins[i] > _bbox.mMaxs[i])
				return false;
		}
		return true;
	}
	bool Contains(const Vec3 &_pt) const
	{
		if(mMaxs.x < _pt.x || mMins.x > _pt.x)
			return false;
		if(mMaxs.y < _pt.y || mMins.y > _pt.y)
			return false;
		if(mMaxs.z < _pt.z || mMins.z > _pt.z)
			return false;
		return true;
	}
	bool FindIntersection(const BoundingBox &_bbox, BoundingBox& _overlap) const
	{
		if(Intersects(_bbox))
		{
			if(mMaxs.x <= _bbox.mMaxs.x)
				_overlap.mMaxs.x = mMaxs.x;
			else
				_overlap.mMaxs.x = _bbox.mMaxs.x;

			if(mMins.x <= _bbox.mMins.x)
				_overlap.mMins.x = _bbox.mMins.x;
			else
				_overlap.mMins.x = mMins.x;

			if(mMaxs.y <= _bbox.mMaxs.y)
				_overlap.mMaxs.y = mMaxs.y;
			else
				_overlap.mMaxs.y = _bbox.mMaxs.y;

			if(mMins.y <= _bbox.mMins.y)
				_overlap.mMins.y = _bbox.mMins.y;
			else
				_overlap.mMins.y = mMins.y;

			if(mMaxs.z <= _bbox.mMaxs.z)
				_overlap.mMaxs.z = mMaxs.z;
			else
				_overlap.mMaxs.z = _bbox.mMaxs.z;

			if(mMins.z <= _bbox.mMins.z)
				_overlap.mMins.z = _bbox.mMins.z;
			else
				_overlap.mMins.z = mMins.z;
			return true;
		}
		return false;
	}
	float GetLengthX() const { return mMaxs.x - mMins.x; }
	float GetLengthY() const { return mMaxs.y - mMins.y; }
	float GetLengthZ() const { return mMaxs.z - mMins.z; }

	float GetArea() const
	{
		return GetLengthX() * GetLengthY() * GetLengthZ();
	}
	float DistanceFromBottom(const Vec3 &_pt) const
	{
		return -(mMins.z - _pt.z);
	}
	float DistanceFromTop(const Vec3 &_pt) const
	{
		return (mMaxs.z - _pt.z);
	}
	void Scale(float _scale)
	{
		mMins.x *= _scale;
		mMins.y *= _scale;
		mMins.z *= _scale;
		mMaxs.x *= _scale;
		mMaxs.y *= _scale;
		mMaxs.z *= _scale;
	}
	BoundingBox ScaleCopy(float _scale)
	{
		BoundingBox out = *this; // cs: was AABB, but gcc said NO
		out.Scale(_scale);
		return out;
	}

	void GetBottomCorners(Vec3 &_bl, Vec3 &_tl, Vec3 &_tr, Vec3 &_br)
	{
		_bl.x = mMins.x;
		_bl.y = mMins.y;
		_bl.z = mMins.x;

		_tl.x = mMins.x;
		_tl.y = mMaxs.y;
		_tl.z = mMins.x;

		_tr.x = mMaxs.x;
		_tr.y = mMaxs.y;
		_tr.z = mMins.x;

		_br.x = mMaxs.x;
		_br.y = mMins.y;
		_br.z = mMins.x;
	}
	void GetTopCorners(Vec3 &_bl, Vec3 &_tl, Vec3 &_tr, Vec3 &_br)
	{
		GetBottomCorners(_bl, _tl, _tr, _br);
		_bl.z = mMaxs.z;
		_tl.z = mMaxs.z;
		_tr.z = mMaxs.z;
		_br.z = mMaxs.z;
	}
	void Translate(const Vec3 &_pos)
	{
		mMins.x += _pos.x;
		mMaxs.x += _pos.x;

		mMins.y += _pos.y;
		mMaxs.y += _pos.y;

		mMins.z += _pos.z;
		mMaxs.z += _pos.z;
	}
	void UnTranslate(const Vec3 &_pos)
	{
		mMins.x -= _pos.x;
		mMaxs.x -= _pos.x;

		mMins.y -= _pos.y;
		mMaxs.y -= _pos.y;

		mMins.z -= _pos.z;
		mMaxs.z -= _pos.z;
	}
	BoundingBox TranslateCopy(const Vec3 &_pos) const
	{
		BoundingBox aabb = *this;
		aabb.Translate(_pos);
		return aabb;
	}
	BoundingBox(const Vec3 &_mins, const Vec3 &_maxs)
	{
		SetMinMax(_mins, _maxs);
	}
	BoundingBox(const Vec3 &_center)
	{
		Set(_center);
	}
	BoundingBox()
	{
		Set(Vec3(0,0,0));
	}

	Vec3	mMins;
	Vec3	mMaxs;
};

#endif
