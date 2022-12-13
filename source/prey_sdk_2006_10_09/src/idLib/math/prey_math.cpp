
#include "../precompiled.h"
#pragma hdrstop



const float	hhMath::EXPONENTIAL		= 2.718281828459045f;


/*
===============
hhMath::logBase
===============
*/
float hhMath::logBase(float base, float x) {

	// Compute logarithm of arbitrary base using the rule:
	// Log (x) = Log (x) / Log (b)
	//    b         c         c			for any c

	return log10f(x) / log10f(base);
}

// Decibel conversion functions
// Converts between linear volumes [0..INF) and doom's version of dB (base 6)
float hhMath::dB2Scale( float dB ) {
	if ( dB == 0.0f ) {
		return 1.0f;				// most common
	} else if ( dB <= -60.0f ) {
		return 0.0f;				// infinitly quiet
	}
	return (Pow(2,(dB/6.0f))); 
}

float hhMath::Scale2dB( float scale ) {
	if (scale <= 0.0f) {
		return -60.0f;				// infinitely quiet
	} else if (scale == 1.0f) {
		return 0.0f;				// most common
	}
	return 6.0f * logBase(2.0f, scale);
}

/*
===============
hhMath::Frac
	returns the fractional part of a float
===============
*/
float hhMath::Frac( float a ) {
	return a - ((int)a);
}

/*
===============
hhMath::Pow
===============
*/
float hhMath::Pow( const float num, const float exponent ) {
	return pow( num, exponent );
}

/*
===============
hhMath::MidPointLerp
===============
*/
float hhMath::MidPointLerp( const float startVal, const float midVal, const float endVal, const float alpha ) {
	if( alpha <= 0.0f ) {
		return startVal;
	}

	if( alpha >= 1.0f ) {
		return endVal;
	}

	return ( alpha < 0.5f ) ? Lerp( startVal, midVal, 2.0f * alpha ) : Lerp( midVal, endVal, 2.0f * ( alpha - 0.5f ) );	
}

/*
===============
hhMath::Lerp
===============
*/
float hhMath::Lerp( const float startVal, const float endVal, const float alpha ) {
	if( alpha <= 0.0f ) {
		return startVal;
	}

	if( alpha >= 1.0f ) {
		return endVal;
	}

	return startVal + ( endVal - startVal ) * alpha;
}

/*
===============
hhMath::Lerp
===============
*/
float hhMath::Lerp( const idVec2& valRange, const float alpha ) {
	return Lerp( valRange[0], valRange[1], alpha );
}

//
// GetClosestPtOnBoundary()
//
// JRM - DID NOT FORCE INLINE. Let the compiler decide on this one
//
idVec3	hhMath::GetClosestPtOnBoundary(const idVec3 &pt, const idBounds &bnds )
{
	idVec3 closePt;

	
	idVec3 ul;
	idVec3 lr;
	int i;

	ul = bnds[0];
	lr = bnds[1];	
	

	// We are INSIDE looking for closest boundary
	if(bnds.ContainsPoint(pt))
	{
		closePt = pt;
		int closestSides[3];			// 0==ul 1==lr
		float closestSideDists[3];

		// JRM TODO: Could put this all in one loop....

		// Find closest sides
		for(i=0;i<3;i++)
		{
			float ulDist = pt[i] - ul[i];
			float lrDist = lr[i] - pt[i];
			if(ulDist < lrDist )			
			{
				closestSides[i]		= 0; 
				closestSideDists[i]	= ulDist;
			}
			else
			{
				closestSides[i] = 1; 
				closestSideDists[i]	= lrDist;
			}		
		}

		// Now find closest axis
		int closestAxis = 0;
		for(i=1;i<3;i++)
		{
            if(closestSideDists[i] < closestSideDists[closestAxis])
				closestAxis = i;
		}

		if(closestSides[closestAxis] == 0)
			closePt[closestAxis] = ul[closestAxis];
		else
			closePt[closestAxis] = lr[closestAxis];
        
	}	
	else // OUTSIDE looking for closest boundary - so just clamp
	{
		for(i=0;i<3;i++)
		{			
			if(pt[i] < ul[i])		
				closePt[i] = ul[i];	
			else if(pt[i] > lr[i])		
				closePt[i] = lr[i];	
			else // INSIDE
			{
				closePt[i] = pt[i];
			}
		}
	}

	return closePt;
};

/*
================
hhMath::ProjectPointOntoLine

//HUMANHEAD: aob
================
*/
idVec3 hhMath::ProjectPointOntoLine( const idVec3& point, const idVec3& line, const idVec3& lineStartPoint ) {
	idVec3 lineDir = line;
	lineDir.Normalize();
	float dot = (point - lineStartPoint) * lineDir;

	return (lineDir * dot) + lineStartPoint;
}

/*
================
hhMath::DistFromPointToLine

//HUMANHEAD: aob
================
*/
float hhMath::DistFromPointToLine( const idVec3& point, const idVec3& line, const idVec3& lineStartPoint ) {
	assert( line.Length() );

	return ( (point - lineStartPoint).Cross(line) ).Length() / line.Length();
}

/*
================
hhMath::BuildRotationMatrix

//HUMANHEAD: rww
================
*/
void hhMath::BuildRotationMatrix(float phi, int axis, idMat3 &mat) {
	mat.Identity();

	switch (axis) {
	case 0: //x
		mat[1][0] = 0.0f;
		mat[1][1] = cos(phi);
		mat[1][2] = sin(phi);
		mat[2][0] = 0.0f;
		mat[2][1] = -sin(phi);
		mat[2][2] = cos(phi);
		break;
	case 1: //y
		mat[0][0] = cos(phi);
		mat[0][1] = 0.0f;
		mat[0][2] = sin(phi);
		mat[2][0] = -sin(phi);
		mat[2][1] = 0.0f;
		mat[2][2] = cos(phi);
		break;
	case 2: //z
		mat[0][0] = cos(phi);
		mat[0][1] = sin(phi);
		mat[0][2] = 0.0f;
		mat[1][0] = -sin(phi);
		mat[1][1] = cos(phi);
		mat[1][2] = 0.0f;
		break;
	default:
		break;
	}
}
