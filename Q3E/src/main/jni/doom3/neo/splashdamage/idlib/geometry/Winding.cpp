// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#endif

#if defined( ID_WIN_X86_SSE )

#include "../math/Simd_InstructionMacros.h"

#define VEC5_SIZE	5*4			// sizeof( idVec5 )

ALIGN4_INIT4( int SIMD_PD_signBit, IEEE_SP_ZERO, IEEE_SP_SIGN, IEEE_SP_ZERO, IEEE_SP_SIGN );
ALIGN4_INIT4( int SIMD_PD_firstSignBit, IEEE_SP_ZERO, IEEE_SP_SIGN, IEEE_SP_ZERO, IEEE_SP_ZERO );
ALIGN4_INIT4( int SIMD_PD_lastSignBit, IEEE_SP_ZERO, IEEE_SP_ZERO, IEEE_SP_ZERO, IEEE_SP_SIGN );

#endif


//===============================================================
//
//	idWinding
//
//===============================================================

/*
=============
idWinding::ReAllocate
=============
*/
bool idWinding::ReAllocate( int n, bool keep ) {
	idVec5 *oldP = p;

	n = (n+3) & ~3;	// align up to multiple of four
	p = new idVec5[n];
	if ( oldP ) {
		if ( keep ) {
			memcpy( p, oldP, numPoints * sizeof(p[0]) );
		}
		delete[] oldP;
	}
	allocedSize = n;

	return true;
}

/*
=============
idWinding::BaseForPlane
=============
*/
void idWinding::BaseForPlane( const idVec3 &normal, const float dist, const float radius ) {
	idVec3 org, vright, vup;

	EnsureAlloced( 4 );
	numPoints = 4;

	normal.NormalVectors( vup, vright );
	vup *= radius;
	vright *= radius;
	org = normal * dist;

	p[0].ToVec3() = org - vright + vup;
	p[0].s = p[0].t = 0.0f;
	p[1].ToVec3() = org + vright + vup;
	p[1].s = p[1].t = 0.0f;
	p[2].ToVec3() = org + vright - vup;
	p[2].s = p[2].t = 0.0f;
	p[3].ToVec3() = org - vright - vup;
	p[3].s = p[3].t = 0.0f;
}

/*
=============
TestSides
=============
*/
void TestSides( const idVec5 *points, int numPoints, const idPlane &plane, const float epsilon, double *dists, byte *sides, int counts[3] ) {
/*
	int i, cnts[3];

	cnts[0] = cnts[1] = cnts[2] = 0;
		
	for (i = 0; i < numPoints; i++) {
		double d = plane.Distance( points[i].ToVec3() );
		if ( d != dists[i] ) {
			int bah = 1;
		}
		if ( d > epsilon ) {
			if ( sides[i] != SIDE_FRONT ) {
				int bah = 1;
				}
		} else if ( d < -epsilon ) {
			if ( sides[i] != SIDE_BACK ) {
				int bah = 1;
			}
		} else {
			if ( sides[i] != SIDE_ON ) {
				int bah = 1;
			}
		}
		cnts[sides[i]]++;
	}
	if ( cnts[0] != counts[0] || cnts[1] != counts[1] || cnts[2] != counts[2] ) {
		int bah = 1;
	}
*/
}

/*
=============
CalculateSides

determine sides for each point
=============
*/
void CalculateSides( const idVec5 *points, int numPoints, const idPlane &plane, const float epsilon, double *dists, byte *sides, int counts[3] ) {
#if defined( ID_WIN_X86_SSE2 )

	__asm {
		mov			ecx, counts
		mov			dword ptr [ecx+0], 0
		mov			dword ptr [ecx+4], 0
		mov			dword ptr [ecx+8], 0
		mov			edi, dists
		mov			esi, sides
		mov			eax, numPoints
		add			esi, eax
		lea			edi, [edi+eax*8]
		neg			eax
		jz			done
		mov			edx, plane
		cvtss2sd	xmm0, [edx+ 0]
		cvtss2sd	xmm1, [edx+ 4]
		cvtss2sd	xmm2, [edx+ 8]
		cvtss2sd	xmm3, [edx+12]
		cvtss2sd	xmm7, epsilon
		shufpd		xmm7, xmm7, 0
		xorpd		xmm7, SIMD_PD_firstSignBit
		mov			edx, points
	loop1:
		cvtss2sd	xmm4, [edx+0]
		mulsd		xmm4, xmm0
		cvtss2sd	xmm5, [edx+4]
		mulsd		xmm5, xmm1
		addsd		xmm4, xmm5
		cvtss2sd	xmm5, [edx+8]
		mulsd		xmm5, xmm2
		addsd		xmm4, xmm5
		addsd		xmm4, xmm3
		movsd		[edi+eax*8], xmm4
		pshufd		xmm5, xmm4, R_SHUFFLE_D( 0, 1, 0, 1 )
		cmppd		xmm5, xmm7, 1
		pshufd		xmm4, xmm5, R_SHUFFLE_D( 0, 1, 0, 1 )
		andpd		xmm4, SIMD_PD_lastSignBit
		xorpd		xmm5, xmm4
		movmskpd	ebx, xmm5
		mov			byte ptr [esi+eax], bl
		add			dword ptr [ecx+ebx*4], 1
		add			edx, VEC5_SIZE
		add			eax, 1
		jl			loop1
	done:
		mov			edx, dists
		mov			ecx, sides
		movsd		xmm0, [edx]
		movsd		[edi], xmm0
		mov			al, byte ptr [ecx]
		mov			byte ptr [esi], al
	}

#elif defined( ID_WIN_X86_ASM )

	int i;

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	for ( i = 0; i < numPoints; i++ ) {
		dists[i] = plane.Distance( points[i].ToVec3() );
		float d1 = - epsilon - dists[i];
		float d2 = epsilon - dists[i];
		int s1 = IEEE_FLT_SIGNBITNOTSET( d1 );	// s1 = dists[i] < -epsilon
		int s2 = IEEE_FLT_SIGNBITNOTSET( d2 );	// s2 = dists[i] < epsilon
		sides[i] = s1 | ( ( s1 ^ s2 ) << 1 );
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

#else

	int i;

	counts[SIDE_FRONT] = counts[SIDE_BACK] = counts[SIDE_ON] = 0;

	for ( i = 0; i < numPoints; i++ ) {
		dists[i] = plane.Distance( points[i].ToVec3() );
		if ( dists[i] > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dists[i] < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];
	
#endif
}

/*
=============
idWinding::Split
=============
*/
int idWinding::Split( const idPlane &plane, const float epsilon, idWinding& front, idWinding& back ) const {
	int				i, j;
	double *		dists;
	byte *			sides;
	int				counts[3];
	const idVec5 *	p1, *p2;
	idVec5			mid;
	int				maxpts;

	assert( this != NULL );

	front.numPoints = 0;
	back.numPoints = 0;

	dists = (double *) _alloca( (numPoints+4) * sizeof( dists[0] ) );
	sides = (byte *) _alloca( (numPoints+4) * sizeof( sides[0] ) );

	CalculateSides( p, numPoints, plane, epsilon, dists, sides, counts );

	// if coplanar, put on the front side if the normals match
	if ( !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		idPlane windingPlane;

		GetPlane( windingPlane );
		if ( windingPlane.Normal() * plane.Normal() > 0.0f ) {
			front = *this;
			return SIDE_FRONT;
		} else {
			back = *this;
			return SIDE_BACK;
		}
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		back = *this;
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		front = *this;
		return SIDE_FRONT;
	}

	maxpts = numPoints+4;	// cannot use counts[0]+2 because of fp grouping errors

	front.EnsureAlloced( maxpts );
	back.EnsureAlloced( maxpts );

	for (i = 0; i < numPoints; i++) {
		p1 = &p[i];

		if ( sides[i] == SIDE_ON ) {
			front.p[front.numPoints] = *p1;
			front.numPoints++;
			back.p[back.numPoints] = *p1;
			back.numPoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			front.p[front.numPoints] = *p1;
			front.numPoints++;
		} else if ( sides[i] == SIDE_BACK ) {
			back.p[back.numPoints] = *p1;
			back.numPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		// generate a split point
		p2 = &p[(i+1)%numPoints];

		// always calculate the split going from the same side or minor epsilon issues can happen
		if ( sides[i] == SIDE_FRONT ) {
			double d = dists[i] / ( dists[i] - dists[i+1] );
			for ( j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p1)[j] + d * ( (double)(*p2)[j] - (double)(*p1)[j] );
				}
			}
			mid.s = (double)p1->s + d * ( (double)p2->s - (double)p1->s );
			mid.t = (double)p1->t + d * ( (double)p2->t - (double)p1->t );
		} else {
			double d = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( j = 0; j < 3; j++ ) {	
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p2)[j] + d * ( (double)(*p1)[j] - (double)(*p2)[j] );
				}
			}
			mid.s = (double)p2->s + d * ( (double)p1->s - (double)p2->s );
			mid.t = (double)p2->t + d * ( (double)p1->t - (double)p2->t );
		}

		front.p[front.numPoints] = mid;
		front.numPoints++;
		back.p[back.numPoints] = mid;
		back.numPoints++;
	}

	if ( front.numPoints > maxpts || back.numPoints > maxpts ) {
		idLib::common->FatalError( "idWinding::Split: points exceeded estimate." );
	}

	return SIDE_CROSS;
}

/*
=============
idWinding::Split
=============
*/
int idWinding::Split( const idPlane &plane, const float epsilon, idWinding **front, idWinding **back ) const {
	int				i, j;
	double *		dists;
	byte *			sides;
	int				counts[3];
	const idVec5 *	p1, *p2;
	idVec5			mid;
	idWinding *		f, *b;
	int				maxpts;

	assert( this != NULL );

	dists = (double *) _alloca( (numPoints+4) * sizeof( dists[0] ) );
	sides = (byte *) _alloca( (numPoints+4) * sizeof( sides[0] ) );

	CalculateSides( p, numPoints, plane, epsilon, dists, sides, counts );

	*front = *back = NULL;

	// if coplanar, put on the front side if the normals match
	if ( !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		idPlane windingPlane;

		GetPlane( windingPlane );
		if ( windingPlane.Normal() * plane.Normal() > 0.0f ) {
			*front = Copy();
			return SIDE_FRONT;
		} else {
			*back = Copy();
			return SIDE_BACK;
		}
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		*back = Copy();
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		*front = Copy();
		return SIDE_FRONT;
	}

	maxpts = numPoints+4;	// cannot use counts[0]+2 because of fp grouping errors

	*front = f = new idWinding(maxpts);
	*back = b = new idWinding(maxpts);
		
	for (i = 0; i < numPoints; i++) {
		p1 = &p[i];
		
		if ( sides[i] == SIDE_ON ) {
			f->p[f->numPoints] = *p1;
			f->numPoints++;
			b->p[b->numPoints] = *p1;
			b->numPoints++;
			continue;
		}
	
		if ( sides[i] == SIDE_FRONT ) {
			f->p[f->numPoints] = *p1;
			f->numPoints++;
		} else if ( sides[i] == SIDE_BACK ) {
			b->p[b->numPoints] = *p1;
			b->numPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}
			
		// generate a split point
		p2 = &p[(i+1)%numPoints];
		
		// always calculate the split going from the same side or minor epsilon issues can happen
		if ( sides[i] == SIDE_FRONT ) {
			double d = dists[i] / ( dists[i] - dists[i+1] );
			for ( j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p1)[j] + d * ( (double)(*p2)[j] - (double)(*p1)[j] );
				}
			}
			mid.s = (double)p1->s + d * ( (double)p2->s - (double)p1->s );
			mid.t = (double)p1->t + d * ( (double)p2->t - (double)p1->t );
		} else {
			double d = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( j = 0; j < 3; j++ ) {	
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p2)[j] + d * ( (double)(*p1)[j] - (double)(*p2)[j] );
				}
			}
			mid.s = (double)p2->s + d * ( (double)p1->s - (double)p2->s );
			mid.t = (double)p2->t + d * ( (double)p1->t - (double)p2->t );
		}

		f->p[f->numPoints] = mid;
		f->numPoints++;
		b->p[b->numPoints] = mid;
		b->numPoints++;
	}
	
	if ( f->numPoints > maxpts || b->numPoints > maxpts ) {
		idLib::common->FatalError( "idWinding::Split: points exceeded estimate." );
	}

	return SIDE_CROSS;
}

/*
=============
idWinding::SplitInPlace
=============
*/
int idWinding::SplitInPlace( const idPlane &plane, const float epsilon, idWinding **back ) {
	int				i, j;
	double *		dists;
	byte *		sides;
	int				counts[3];
	idVec5 *	newPoints;
	int			newNumPoints;
	const idVec5 *	p1, *p2;
	idVec5		mid;
	idWinding *		b;
	int			maxpts;

	assert( this );

	dists = (double *) _alloca( (numPoints+4) * sizeof( dists[0] ) );
	sides = (byte *) _alloca( (numPoints+4) * sizeof( sides[0] ) );

	CalculateSides( p, numPoints, plane, epsilon, dists, sides, counts );

	*back = NULL;

	// if coplanar, put on the front side if the normals match
	if ( !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		idPlane windingPlane;

		GetPlane( windingPlane );
		if ( windingPlane.Normal() * plane.Normal() > 0.0f ) {
			return SIDE_FRONT;
		} else {
			return SIDE_BACK;
		}
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		return SIDE_FRONT;
	}

	maxpts = numPoints+4;	// cannot use counts[0]+2 because of fp grouping errors

	newPoints = (idVec5 *) _alloca16( maxpts * sizeof( newPoints[0] ) );
	newNumPoints = 0;

	*back = b = new idWinding( maxpts );

	for ( i = 0; i < numPoints; i++ ) {
		p1 = &p[i];

		if ( sides[i] == SIDE_ON ) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			b->p[b->numPoints] = *p1;
			b->numPoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		} else if ( sides[i] == SIDE_BACK ) {
			b->p[b->numPoints] = *p1;
			b->numPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}
			
		// generate a split point
		p2 = &p[(i+1)%numPoints];
		
		// always calculate the split going from the same side or minor epsilon issues can happen
		if ( sides[i] == SIDE_FRONT ) {
			double d = dists[i] / ( dists[i] - dists[i+1] );
			for ( j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p1)[j] + d * ( (double)(*p2)[j] - (double)(*p1)[j] );
				}
			}
			mid.s = (double)p1->s + d * ( (double)p2->s - (double)p1->s );
			mid.t = (double)p1->t + d * ( (double)p2->t - (double)p1->t );
		} else {
			double d = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( j = 0; j < 3; j++ ) {	
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
		} else {
					mid[j] = (double)(*p2)[j] + d * ( (double)(*p1)[j] - (double)(*p2)[j] );
				}
			}
			mid.s = (double)p2->s + d * ( (double)p1->s - (double)p2->s );
			mid.t = (double)p2->t + d * ( (double)p1->t - (double)p2->t );
		}

		newPoints[newNumPoints] = mid;
		newNumPoints++;
		b->p[b->numPoints] = mid;
		b->numPoints++;
	}

	if ( !EnsureAlloced( newNumPoints, false ) ) {
		return true;
	}

	numPoints = newNumPoints;
	memcpy( p, newPoints, newNumPoints * sizeof( p[0] ) );

	if ( numPoints > maxpts || b->numPoints > maxpts ) {
		idLib::common->FatalError( "idWinding::Split: points exceeded estimate." );
	}

	return SIDE_CROSS;
}

/*
=============
idWinding::ClipInPlace
=============
*/
bool idWinding::ClipInPlace( const idPlane &plane, const float epsilon, const bool keepOn ) {
	int			i, j;
	double *	dists;
	byte *		sides;
	int			counts[3];
	idVec5 *	newPoints;
	int			newNumPoints;
	idVec5 *	p1, *p2;
	idVec5		mid;
	int			maxpts;

	assert( this );

	dists = (double *) _alloca( (numPoints+4) * sizeof( dists[0] ) );
	sides = (byte *) _alloca( (numPoints+4) * sizeof( sides[0] ) );

	CalculateSides( p, numPoints, plane, epsilon, dists, sides, counts );

	// if the winding is on the plane and we should keep it
	if ( keepOn && !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		return true;
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		numPoints = 0;
		return false;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		return true;
	}

	maxpts = numPoints + 4;		// cannot use counts[0]+2 because of fp grouping errors

	newPoints = (idVec5 *) _alloca16( maxpts * sizeof( newPoints[0] ) );
	newNumPoints = 0;
		
	for ( i = 0; i < numPoints; i++ ) {
		p1 = &p[i];

		if ( sides[i] == SIDE_ON ) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		// generate a split point
		p2 = &p[(i+1)%numPoints];

		// always calculate the split going from the same side or minor epsilon issues can happen
		if ( sides[i] == SIDE_FRONT ) {
			double d = dists[i] / ( dists[i] - dists[i+1] );
			for ( j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p1)[j] + d * ( (double)(*p2)[j] - (double)(*p1)[j] );
				}
			}
			mid.s = (double)p1->s + d * ( (double)p2->s - (double)p1->s );
			mid.t = (double)p1->t + d * ( (double)p2->t - (double)p1->t );
		} else {
			double d = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p2)[j] + d * ( (double)(*p1)[j] - (double)(*p2)[j] );
				}
			}
			mid.s = (double)p2->s + d * ( (double)p1->s - (double)p2->s );
			mid.t = (double)p2->t + d * ( (double)p1->t - (double)p2->t );
		}

		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}
	
	if ( !EnsureAlloced( newNumPoints, false ) ) {
		return true;
	}

	numPoints = newNumPoints;
	memcpy( p, newPoints, newNumPoints * sizeof( p[0] ) );

	return true;
}

/*
=============
CopyEdgeNums
=============
*/
int *CopyEdgeNums( const int *edgeNums, const int numEdgeNums ) {
	int *newEdgeNums = new int[numEdgeNums];
	for ( int i = 0; i < numEdgeNums; i++ ) {
		newEdgeNums[i] = edgeNums[i];
	}
	return newEdgeNums;
}

/*
=============
idWinding::SplitWithEdgeNums
=============
*/
int idWinding::SplitWithEdgeNums( const int *edgeNums, const idPlane &plane, const int edgeNum, const float epsilon,
								   idWinding **front, idWinding **back, int **frontEdgeNums, int **backEdgeNums ) const {
	int				i, j;
	double *		dists;
	byte *		sides;
	int			counts[3];
	const idVec5 *	p1, *p2;
	idVec5		mid;
	idWinding *		f, *b;
	int *			fe, *be;
	int			maxpts;

	assert( this );

	dists = (double *) _alloca( (numPoints+4) * sizeof( dists[0] ) );
	sides = (byte *) _alloca( (numPoints+4) * sizeof( sides[0] ) );

	CalculateSides( p, numPoints, plane, epsilon, dists, sides, counts );
	
	*front = *back = NULL;
	*frontEdgeNums = *backEdgeNums = NULL;

	// if coplanar, put on the front side if the normals match
	if ( !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		idPlane windingPlane;

		GetPlane( windingPlane );
		if ( windingPlane.Normal() * plane.Normal() > 0.0f ) {
			*frontEdgeNums = CopyEdgeNums( edgeNums, numPoints );
			*front = Copy();
			return SIDE_FRONT;
		} else {
			*backEdgeNums = CopyEdgeNums( edgeNums, numPoints );
			*back = Copy();
			return SIDE_BACK;
		}
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		*backEdgeNums = CopyEdgeNums( edgeNums, numPoints );
		*back = Copy();
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		*frontEdgeNums = CopyEdgeNums( edgeNums, numPoints );
		*front = Copy();
		return SIDE_FRONT;
	}

	maxpts = numPoints+4;	// cannot use counts[0]+2 because of fp grouping errors

	*front = f = new idWinding( maxpts );
	*back = b = new idWinding( maxpts );

	*frontEdgeNums = fe = new int[maxpts];
	*backEdgeNums = be = new int[maxpts];

	for ( i = 0; i < numPoints; i++ ) {
		p1 = &p[i];
		
		if ( sides[i] == SIDE_ON ) {
			if ( sides[i+1] == SIDE_FRONT ) {
				fe[f->numPoints] = edgeNums[i];
				be[b->numPoints] = edgeNum;
			} else if ( sides[i+1] == SIDE_BACK ) {
				fe[f->numPoints] = edgeNum;
				be[b->numPoints] = edgeNums[i];
		} else {
				fe[f->numPoints] = edgeNums[i];
				be[b->numPoints] = edgeNums[i];
		}
			f->p[f->numPoints] = *p1;
			f->numPoints++;
			b->p[b->numPoints] = *p1;
			b->numPoints++;
			continue;
	}

		if ( sides[i] == SIDE_FRONT ) {
			fe[f->numPoints] = edgeNums[i];
			f->p[f->numPoints] = *p1;
			f->numPoints++;
		} else if ( sides[i] == SIDE_BACK ) {
			be[b->numPoints] = edgeNums[i];
			b->p[b->numPoints] = *p1;
			b->numPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}

		// generate a split point
		p2 = &p[(i+1)%numPoints];
		
		// always calculate the split going from the same side or minor epsilon issues can happen
		if ( sides[i] == SIDE_FRONT ) {
			double d = dists[i] / ( dists[i] - dists[i+1] );
			for ( j = 0; j < 3; j++ ) {
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p1)[j] + d * ( (double)(*p2)[j] - (double)(*p1)[j] );
				}
			}
			mid.s = (double)p1->s + d * ( (double)p2->s - (double)p1->s );
			mid.t = (double)p1->t + d * ( (double)p2->t - (double)p1->t );

			fe[f->numPoints] = edgeNum;
			be[b->numPoints] = edgeNums[i];
		} else {
			double d = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( j = 0; j < 3; j++ ) {	
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
		} else {
					mid[j] = (double)(*p2)[j] + d * ( (double)(*p1)[j] - (double)(*p2)[j] );
				}
			}
			mid.s = (double)p2->s + d * ( (double)p1->s - (double)p2->s );
			mid.t = (double)p2->t + d * ( (double)p1->t - (double)p2->t );

			fe[f->numPoints] = edgeNums[i];
			be[b->numPoints] = edgeNum;
		}

		f->p[f->numPoints] = mid;
		f->numPoints++;
		b->p[b->numPoints] = mid;
		b->numPoints++;
	}
	
	if ( f->numPoints > maxpts || b->numPoints > maxpts ) {
		idLib::common->FatalError( "idWinding::Split: points exceeded estimate." );
	}

	return SIDE_CROSS;
}

/*
=============
idWinding::SplitInPlaceWithEdgeNums
=============
*/
int idWinding::SplitInPlaceWithEdgeNums( idList<int> &edgeNums, const idPlane &plane, const float epsilon, idWinding **back, int **backEdgePlanes ) {
	assert( 0 );
	return SIDE_FRONT;
}

/*
=============
idWinding::ClipInPlaceWithEdgeNums
=============
*/
int idWinding::ClipInPlaceWithEdgeNums( idList<int> &edgeNums, const idPlane &plane, const int edgeNum, const float epsilon, const bool keepOn ) {
	int			i, j;
	double *	dists;
	byte *		sides;
	int			counts[3];
	idVec5 *	newPoints;
	int *		newEdgeNums;
	int			newNumPoints;
	idVec5 *	p1, *p2;
	idVec5		mid;
	int			maxpts;

	assert( this );

	dists = (double *) _alloca( (numPoints+4) * sizeof( dists[0] ) );
	sides = (byte *) _alloca( (numPoints+4) * sizeof( sides[0] ) );

	CalculateSides( p, numPoints, plane, epsilon, dists, sides, counts );
	
	// if the winding is on the plane and we should keep it
	if ( keepOn && !counts[SIDE_FRONT] && !counts[SIDE_BACK] ) {
		return SIDE_ON;
	}
	// if nothing at the front of the clipping plane
	if ( !counts[SIDE_FRONT] ) {
		numPoints = 0;
		return SIDE_BACK;
	}
	// if nothing at the back of the clipping plane
	if ( !counts[SIDE_BACK] ) {
		return SIDE_FRONT;
	}

	maxpts = numPoints + 4;		// cannot use counts[0]+2 because of fp grouping errors

	newPoints = (idVec5 *) _alloca16( maxpts * sizeof( newPoints[0] ) );
	newEdgeNums = (int *) _alloca16( maxpts * sizeof( newEdgeNums[0] ) );
	newNumPoints = 0;

	for ( i = 0; i < numPoints; i++ ) {
		p1 = &p[i];

		if ( sides[i] == SIDE_ON ) {
			if ( sides[i+1] == SIDE_FRONT ) {
				newEdgeNums[newNumPoints] = edgeNums[i];
			} else if ( sides[i+1] == SIDE_BACK ) {
				newEdgeNums[newNumPoints] = edgeNum;
			} else {
				newEdgeNums[newNumPoints] = edgeNums[i];
		}
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
			continue;
		}
	
		if ( sides[i] == SIDE_FRONT ) {
			newEdgeNums[newNumPoints] = edgeNums[i];
			newPoints[newNumPoints] = *p1;
			newNumPoints++;
		}

		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}
			
		// generate a split point
		p2 = &p[(i+1)%numPoints];
		
		// always calculate the split going from the same side or minor epsilon issues can happen
		if ( sides[i] == SIDE_FRONT ) {
			double d = dists[i] / ( dists[i] - dists[i+1] );
		for ( j = 0; j < 3; j++ ) {
			// avoid round off error when possible
			if ( plane.Normal()[j] == 1.0f ) {
				mid[j] = plane.Dist();
			} else if ( plane.Normal()[j] == -1.0f ) {
				mid[j] = -plane.Dist();
			} else {
				mid[j] = (double)(*p1)[j] + d * ( (double)(*p2)[j] - (double)(*p1)[j] );
			}
		}
			mid.s = (double)p1->s + d * ( (double)p2->s - (double)p1->s );
			mid.t = (double)p1->t + d * ( (double)p2->t - (double)p1->t );

			newEdgeNums[newNumPoints] = edgeNum;
		} else {
			double d = dists[i+1] / ( dists[i+1] - dists[i] );
			for ( j = 0; j < 3; j++ ) {	
				// avoid round off error when possible
				if ( plane.Normal()[j] == 1.0f ) {
					mid[j] = plane.Dist();
				} else if ( plane.Normal()[j] == -1.0f ) {
					mid[j] = -plane.Dist();
				} else {
					mid[j] = (double)(*p2)[j] + d * ( (double)(*p1)[j] - (double)(*p2)[j] );
				}
			}
			mid.s = (double)p2->s + d * ( (double)p1->s - (double)p2->s );
			mid.t = (double)p2->t + d * ( (double)p1->t - (double)p2->t );

			newEdgeNums[newNumPoints] = edgeNums[i];
		}
			
		newPoints[newNumPoints] = mid;
		newNumPoints++;
	}

	if ( !EnsureAlloced( newNumPoints, false ) ) {
		return true;
	}

	numPoints = newNumPoints;
	memcpy( p, newPoints, newNumPoints * sizeof( p[0] ) );

	edgeNums.SetNum( newNumPoints, false );
	memcpy( edgeNums.Begin(), newEdgeNums, newNumPoints * sizeof( edgeNums[0] ) );

	return SIDE_CROSS;
}

/*
=============
idWinding::Copy
=============
*/
idWinding *idWinding::Copy( void ) const {
	idWinding *w;

	w = new idWinding( numPoints );
	w->numPoints = numPoints;
	memcpy( w->p, p, numPoints * sizeof(p[0]) );
	return w;
}

/*
=============
idWinding::Reverse
=============
*/
idWinding *idWinding::Reverse( void ) const {
	idWinding *w;
	int i;

	w = new idWinding( numPoints );
	w->numPoints = numPoints;
	for ( i = 0; i < numPoints; i++ ) {
		w->p[ numPoints - i - 1 ] = p[i];
	}
	return w;
}

/*
=============
idWinding::ReverseSelf
=============
*/
void idWinding::ReverseSelf( void ) {
	idVec5 v;
	int i;

	for ( i = 0; i < (numPoints>>1); i++ ) {
		v = p[i];
		p[i] = p[numPoints - i - 1];
		p[numPoints - i - 1] = v;
	}
}

/*
=============
idWinding::Check
=============
*/
bool idWinding::Check( bool print ) const {
	int				i, j;
	float			d, edgedist;
	idVec3			dir, edgenormal;
	float			area;
	idPlane			plane;

	if ( numPoints < 3 ) {
		if ( print ) {
			idLib::common->Printf( "idWinding::Check: only %i points.", numPoints );
		}
		return false;
	}
	
	area = GetArea();
	if ( area < 1.0f ) {
		if ( print ) {
			idLib::common->Printf( "idWinding::Check: tiny area: %f", area );
		}
		return false;
	}

	GetPlane( plane );
	
	for ( i = 0; i < numPoints; i++ ) {
		const idVec3 &p1 = p[i].ToVec3();

		// check if the winding is huge
		for ( j = 0; j < 3; j++ ) {
			if ( p1[j] >= MAX_WORLD_COORD || p1[j] <= MIN_WORLD_COORD ) {
				if ( print ) {
					idLib::common->Printf( "idWinding::Check: point %d outside world %c-axis: %f", i, 'X'+j, p1[j] );
				}
				return false;
			}
		}

		j = i + 1 == numPoints ? 0 : i + 1;
		
		// check if the point is on the face plane
		d = p1 * plane.Normal() + plane[3];
		if ( d < -ON_EPSILON || d > ON_EPSILON ) {
			if ( print ) {
				idLib::common->Printf( "idWinding::Check: point %d off plane.", i );
			}
			return false;
		}
	
		// check if the edge isn't degenerate
		const idVec3 &p2 = p[j].ToVec3();
		dir = p2 - p1;
		
		if ( dir.Length() < ON_EPSILON) {
			if ( print ) {
				idLib::common->Printf( "idWinding::Check: edge %d is degenerate.", i );
			}
			return false;
		}

		// check if the winding is convex
		edgenormal = plane.Normal().Cross( dir );
		edgenormal.Normalize();
		edgedist = p1 * edgenormal;
		edgedist += ON_EPSILON;
		
		// all other points must be on front side
		for ( j = 0; j < numPoints; j++ ) {
			if ( j == i ) {
				continue;
			}
			d = p[j].ToVec3() * edgenormal;
			if ( d > edgedist ) {
				if ( print ) {
					idLib::common->Printf( "idWinding::Check: non-convex." );
				}
				return false;
			}
		}
	}
	return true;
}

/*
=============
idWinding::GetArea
=============
*/
float idWinding::GetArea( void ) const {
	int i;
	idVec3 d1, d2, cross;
	float total;

	total = 0.0f;
	for ( i = 2; i < numPoints; i++ ) {
		d1 = p[i-1].ToVec3() - p[0].ToVec3();
		d2 = p[i].ToVec3() - p[0].ToVec3();
		cross = d1.Cross( d2 );
		total += cross.Length();
	}
	return total * 0.5f;
}

/*
=============
idWinding::GetRadius
=============
*/
float idWinding::GetRadius( const idVec3 &center ) const {
	int i;
	float radius, r;
	idVec3 dir;

	radius = 0.0f;
	for ( i = 0; i < numPoints; i++ ) {
		dir = p[i].ToVec3() - center;
		r = dir * dir;
		if ( r > radius ) {
			radius = r;
		}
	}
	return idMath::Sqrt( radius );
}

/*
=============
idWinding::GetCenter
=============
*/
idVec3 idWinding::GetCenter( void ) const {
	int i;
	idVec3 center;

	center.Zero();
	for ( i = 0; i < numPoints; i++ ) {
		center += p[i].ToVec3();
	}
	center *= ( 1.0f / numPoints );
	return center;
}

/*
=============
idWinding::GetNormal
=============
*/
idVec3 idWinding::GetNormal( void ) const {
	idVec3 v1, v2, center, normal;

	if ( numPoints < 3 ) {
		normal.Zero();
		return normal;
	}

	center = GetCenter();
	v1 = p[0].ToVec3() - center;
	v2 = p[1].ToVec3() - center;
	normal = v2.Cross( v1 );
	normal.Normalize();
	return normal;
}

/*
=============
idWinding::GetPlane
=============
*/
void idWinding::GetPlane( idVec3 &normal, float &dist ) const {
	idVec3 v1, v2, center;

	if ( numPoints < 3 ) {
		normal.Zero();
		dist = 0.0f;
		return;
	}

	center = GetCenter();
	v1 = p[0].ToVec3() - center;
	v2 = p[1].ToVec3() - center;
	normal = v2.Cross( v1 );
	normal.Normalize();
	dist = p[0].ToVec3() * normal;
}

/*
=============
idWinding::GetPlane
=============
*/
void idWinding::GetPlane( idPlane &plane ) const {
	idVec3 v1, v2;
	idVec3 center;

	if ( numPoints < 3 ) {
		plane.Zero();
		return;
	}

	center = GetCenter();
	v1 = p[0].ToVec3() - center;
	v2 = p[1].ToVec3() - center;
	plane.SetNormal( v2.Cross( v1 ) );
	plane.Normalize();
	plane.FitThroughPoint( p[0].ToVec3() );
}

/*
=============
idWinding::GetPlane
=============
*/
idPlane idWinding::GetPlane() const {
	idPlane toReturn;
	GetPlane( toReturn );
	return toReturn;
}

/*
=============
idWinding::GetBounds
=============
*/
void idWinding::GetBounds( idBounds &bounds ) const {
	int i;

	if ( !numPoints ) {
		bounds.Clear();
		return;
	}

	bounds[0] = bounds[1] = p[0].ToVec3();
	for ( i = 1; i < numPoints; i++ ) {
		if ( p[i].x < bounds[0].x ) {
			bounds[0].x = p[i].x;
		} else if ( p[i].x > bounds[1].x ) {
			bounds[1].x = p[i].x;
		}
		if ( p[i].y < bounds[0].y ) {
			bounds[0].y = p[i].y;
		} else if ( p[i].y > bounds[1].y ) {
			bounds[1].y = p[i].y;
		}
		if ( p[i].z < bounds[0].z ) {
			bounds[0].z = p[i].z;
		} else if ( p[i].z > bounds[1].z ) {
			bounds[1].z = p[i].z;
		}
	}
}

/*
=============
idWinding::GetPlane
=============
*/
idBounds idWinding::GetBounds() const {
	idBounds toReturn;
	GetBounds( toReturn );
	return toReturn;
}

/*
=============
idWinding::RemoveEqualPoints
=============
*/
void idWinding::RemoveEqualPoints( const float epsilon ) {
	int i, j;

	for ( i = 0; i < numPoints; i++ ) {
		if ( (p[i].ToVec3() - p[(i+numPoints-1)%numPoints].ToVec3()).LengthSqr() >= Square( epsilon ) ) {
			continue;
		}
		numPoints--;
		for ( j = i; j < numPoints; j++ ) {
			p[j] = p[j+1];
		}
		i--;
	}
}

/*
=============
idWinding::RemoveColinearPoints
=============
*/
void idWinding::RemoveColinearPoints( const idVec3 &normal, const float epsilon ) {
	int i, j;
	idVec3 edgeNormal;
	float dist;

	if ( numPoints <= 3 ) {
		return;
	}

	for ( i = 0; i < numPoints; i++ ) {

		// create plane through edge orthogonal to winding plane
		edgeNormal = (p[i].ToVec3() - p[(i+numPoints-1)%numPoints].ToVec3()).Cross( normal );
		edgeNormal.Normalize();
		dist = edgeNormal * p[i].ToVec3();

		if ( idMath::Fabs( edgeNormal * p[(i+1)%numPoints].ToVec3() - dist ) > epsilon ) {
			continue;
		}

		numPoints--;
		for ( j = i; j < numPoints; j++ ) {
			p[j] = p[j+1];
		}
		i--;
	}
}

/*
=============
idWinding::AddToConvexHull

  Adds the given winding to the convex hull.
  Assumes the current winding already is a convex hull with three or more points.
=============
*/
void idWinding::AddToConvexHull( const idWinding *winding, const idVec3 &normal, const float epsilon ) {
	int				i, j, k;
	idVec3			dir;
	float			d;
	int				maxPts;
	idVec3 *		hullDirs;
	bool *			hullSide;
	bool			outside;
	int				numNewHullPoints;
	idVec5 *		newHullPoints;

	if ( !winding ) {
		return;
	}

	maxPts = this->numPoints + winding->numPoints;

	if ( !this->EnsureAlloced( maxPts, true ) ) {
		return;
	}

	newHullPoints = (idVec5 *) _alloca( maxPts * sizeof( idVec5 ) );
	hullDirs = (idVec3 *) _alloca( maxPts * sizeof( idVec3 ) );
	hullSide = (bool *) _alloca( maxPts * sizeof( bool ) );

	for ( i = 0; i < winding->numPoints; i++ ) {
		const idVec5 &p1 = winding->p[i];

		// calculate hull edge vectors
		for ( j = 0; j < this->numPoints; j++ ) {
			dir = this->p[ (j + 1) % this->numPoints ].ToVec3() - this->p[ j ].ToVec3();
			dir.Normalize();
			hullDirs[j] = normal.Cross( dir );
		}

		// calculate side for each hull edge
		outside = false;
		for ( j = 0; j < this->numPoints; j++ ) {
			dir = p1.ToVec3() - this->p[j].ToVec3();
			d = dir * hullDirs[j];
			if ( d >= epsilon ) {
				outside = true;
			}
			if ( d >= -epsilon ) {
				hullSide[j] = true;
			} else {
				hullSide[j] = false;
			}
		}

		// if the point is effectively inside, do nothing
		if ( !outside ) {
			continue;
		}

		// find the back side to front side transition
		for ( j = 0; j < this->numPoints; j++ ) {
			if ( !hullSide[ j ] && hullSide[ (j + 1) % this->numPoints ] ) {
				break;
			}
		}
		if ( j >= this->numPoints ) {
			continue;
		}

		// insert the point here
		newHullPoints[0] = p1;
		numNewHullPoints = 1;

		// copy over all points that aren't double fronts
		j = (j+1) % this->numPoints;
		for ( k = 0; k < this->numPoints; k++ ) {
			if ( hullSide[ (j+k) % this->numPoints ] && hullSide[ (j+k+1) % this->numPoints ] ) {
				continue;
			}
			newHullPoints[numNewHullPoints] = this->p[ (j+k+1) % this->numPoints ];
			numNewHullPoints++;
		}

		this->numPoints = numNewHullPoints;
		memcpy( this->p, newHullPoints, numNewHullPoints * sizeof(idVec5) );
	}
}

/*
=============
idWinding::AddToConvexHull

  Add a point to the convex hull.
  The current winding must be convex but may be degenerate and can have less than three points.
=============
*/
void idWinding::AddToConvexHull( const idVec3 &point, const idVec3 &normal, const float epsilon ) {
	int				j, k, numHullPoints;
	idVec3			dir;
	float			d;
	idVec3 *		hullDirs;
	bool *			hullSide;
	idVec5 *		hullPoints;
	bool			outside;

	switch( numPoints ) {
		case 0: {
			p[0] = point;
			numPoints++;
			return;
		}
		case 1: {
			// don't add the same point second
			if ( p[0].ToVec3().Compare( point, epsilon ) ) {
				return;
			}
			p[1].ToVec3() = point;
			numPoints++;
			return;
		}
		case 2: {
			// don't add a point if it already exists
			if ( p[0].ToVec3().Compare( point, epsilon ) || p[1].ToVec3().Compare( point, epsilon ) ) {
				return;
			}
			// if only two points make sure we have the right ordering according to the normal
			dir = point - p[0].ToVec3();
			dir = dir.Cross( p[1].ToVec3() - p[0].ToVec3() );
			if ( dir[0] == 0.0f && dir[1] == 0.0f && dir[2] == 0.0f ) {
				// points don't make a plane
				return;
			}
			if ( dir * normal > 0.0f ) {
				p[2].ToVec3() = point;
			}
			else {
				p[2] = p[1];
				p[1].ToVec3() = point;
			}
			numPoints++;
			return;
		}
	}

	hullDirs = (idVec3 *) _alloca( numPoints * sizeof( idVec3 ) );
	hullSide = (bool *) _alloca( numPoints * sizeof( bool ) );

	// calculate hull edge vectors
	for ( j = 0; j < numPoints; j++ ) {
		dir = p[(j + 1) % numPoints].ToVec3() - p[j].ToVec3();
		hullDirs[j] = normal.Cross( dir );
	}

	// calculate side for each hull edge
	outside = false;
	for ( j = 0; j < numPoints; j++ ) {
		dir = point - p[j].ToVec3();
		d = dir * hullDirs[j];
		if ( d >= epsilon ) {
			outside = true;
		}
		if ( d >= -epsilon ) {
			hullSide[j] = true;
		} else {
			hullSide[j] = false;
		}
	}

	// if the point is effectively inside, do nothing
	if ( !outside ) {
		return;
	}

	// find the back side to front side transition
	for ( j = 0; j < numPoints; j++ ) {
		if ( !hullSide[ j ] && hullSide[ (j + 1) % numPoints ] ) {
			break;
		}
	}
	if ( j >= numPoints ) {
		return;
	}

	hullPoints = (idVec5 *) _alloca( (numPoints+1) * sizeof( idVec5 ) );

	// insert the point here
	hullPoints[0] = point;
	numHullPoints = 1;

	// copy over all points that aren't double fronts
	j = (j+1) % numPoints;
	for ( k = 0; k < numPoints; k++ ) {
		if ( hullSide[ (j+k) % numPoints ] && hullSide[ (j+k+1) % numPoints ] ) {
			continue;
		}
		hullPoints[numHullPoints] = p[ (j+k+1) % numPoints ];
		numHullPoints++;
	}

	if ( !EnsureAlloced( numHullPoints, false ) ) {
		return;
	}
	numPoints = numHullPoints;
	memcpy( p, hullPoints, numHullPoints * sizeof(idVec5) );
}

/*
=============
idWinding::TryMerge
=============
*/
#define	CONTINUOUS_EPSILON	0.005f

idWinding *idWinding::TryMerge( const idWinding &w, const idVec3 &planenormal, int keep ) const {
	idVec3			*p1, *p2, *p3, *p4, *back;
	idWinding		*newf;
	const idWinding	*f1, *f2;
	int				i, j, k, l;
	idVec3			normal, delta;
	float			dot;
	bool			keep1, keep2;

	f1 = this;
	f2 = &w;
	//
	// find a idLib::common edge
	//	
	p1 = p2 = NULL;	// stop compiler warning
	j = 0;
	
	for ( i = 0; i < f1->numPoints; i++ ) {
		p1 = &f1->p[i].ToVec3();
		p2 = &f1->p[(i+1) % f1->numPoints].ToVec3();
		for ( j = 0; j < f2->numPoints; j++ ) {
			p3 = &f2->p[j].ToVec3();
			p4 = &f2->p[(j+1) % f2->numPoints].ToVec3();
			for (k = 0; k < 3; k++ ) {
				if ( idMath::Fabs((*p1)[k] - (*p4)[k]) > 0.1f ) {
					break;
				}
				if ( idMath::Fabs((*p2)[k] - (*p3)[k]) > 0.1f ) {
					break;
				}
			}
			if ( k == 3 ) {
				break;
			}
		}
		if ( j < f2->numPoints ) {
			break;
		}
	}
	
	if ( i == f1->numPoints ) {
		return NULL;			// no matching edges
	}

	//
	// check slope of connected lines
	// if the slopes are colinear, the point can be removed
	//
	back = &f1->p[(i+f1->numPoints-1)%f1->numPoints].ToVec3();
	delta = (*p1) - (*back);
	normal = planenormal.Cross(delta);
	normal.Normalize();
	
	back = &f2->p[(j+2)%f2->numPoints].ToVec3();
	delta = (*back) - (*p1);
	dot = delta * normal;
	if ( dot > CONTINUOUS_EPSILON ) {
		return NULL;			// not a convex polygon
	}

	keep1 = (bool)(dot < -CONTINUOUS_EPSILON);
	
	back = &f1->p[(i+2)%f1->numPoints].ToVec3();
	delta = (*back) - (*p2);
	normal = planenormal.Cross( delta );
	normal.Normalize();

	back = &f2->p[(j+f2->numPoints-1)%f2->numPoints].ToVec3();
	delta = (*back) - (*p2);
	dot = delta * normal;
	if ( dot > CONTINUOUS_EPSILON ) {
		return NULL;			// not a convex polygon
	}

	keep2 = (bool)(dot < -CONTINUOUS_EPSILON);

	//
	// build the new polygon
	//
	newf = new idWinding( f1->numPoints + f2->numPoints );
	
	// copy first polygon
	for ( k = (i+1) % f1->numPoints; k != i; k = (k+1) % f1->numPoints ) {
		if ( !keep && k == (i+1) % f1->numPoints && !keep2 ) {
			continue;
		}
		
		newf->p[newf->numPoints] = f1->p[k];
		newf->numPoints++;
	}
	
	// copy second polygon
	for ( l = (j+1) % f2->numPoints; l != j; l = (l+1) % f2->numPoints ) {
		if ( !keep && l == (j+1) % f2->numPoints && !keep1 ) {
			continue;
		}
		newf->p[newf->numPoints] = f2->p[l];
		newf->numPoints++;
	}

	return newf;
}

/*
=============
idWinding::RemovePoint
=============
*/
void idWinding::RemovePoint( int point ) {
	if ( point < 0 || point >= numPoints ) {
		idLib::common->FatalError( "idWinding::removePoint: point out of range" );
	}
	if ( point < numPoints - 1) {
		memmove(&p[point], &p[point+1], (numPoints - point - 1) * sizeof(p[0]) );
	}
	numPoints--;
}

/*
=============
idWinding::InsertPoint
=============
*/
void idWinding::InsertPoint( const idVec3 &point, int spot ) {
	int i;

	if ( spot > numPoints ) {
		idLib::common->FatalError( "idWinding::insertPoint: spot > numPoints" );
	}

	if ( spot < 0 ) {
		idLib::common->FatalError( "idWinding::insertPoint: spot < 0" );
	}

	EnsureAlloced( numPoints+1, true );
	for ( i = numPoints; i > spot; i-- ) {
		p[i] = p[i-1];
	}
	p[spot] = point;
	numPoints++;
}

/*
=============
idWinding::InsertPointIfOnEdge
=============
*/
bool idWinding::InsertPointIfOnEdge( const idVec3 &point, const idPlane &plane, const float epsilon, int *index ) {
	int i;
	float d, len;
	idVec3 dir, projected;

	// point may not be too far from the winding plane
	if ( idMath::Fabs( plane.Distance( point ) ) > epsilon ) {
		return false;
	}

	for ( i = 0; i < numPoints; i++ ) {

		const idVec3 &p0 = p[i].ToVec3();
		const idVec3 &p1 = p[(i+1)%numPoints].ToVec3();

		dir = p1 - p0;
		len = dir.Normalize();

		// skip if the point is close to one of the edge points
		d = ( ( point - p0 ) * dir );
		if ( d <= epsilon || d >= len - epsilon ) {
			continue;
		}

		// make sure the point is on the edge
		projected = p0 + d * dir;
		d = ( projected - point ).LengthSqr();
		if ( d >= epsilon * epsilon ) {
			continue;
		}

		InsertPoint( projected, i+1 );

		if ( index != NULL ) {
			*index = i+1;
		}
		return true;
	}
	return false;
}

/*
============
idWinding::CreateTriangles
============
*/
int idWinding::CreateTriangles( int *indices, const float epsilon ) const {
	int i, a, b, c, bestCorner;
	float lengthSqr, dist1Sqr, dist2Sqr, bestDistSqr;
	idVec3 v1, v2;

	assert( numPoints >= 3 );

	if ( numPoints == 3 ) {
	for ( i = 0; i < numPoints; i++ ) {
			indices[i] = i;
			}
		return numPoints;
		}

	bestDistSqr = 0.0f;
	bestCorner = -1;

	for ( i = 0; i < numPoints; i++ ) {
		a = i;
		b = ( i + 1 ) % numPoints;
		c = ( i + 2 ) % numPoints;
		v1 = p[c].ToVec3() - p[a].ToVec3();
		v2 = p[b].ToVec3() - p[a].ToVec3();
		lengthSqr = v1.LengthSqr();
		if ( lengthSqr < epsilon * epsilon ) {
			continue;
			}
		v1 = v2 - ( v1 * v2 / lengthSqr ) * v1;
		dist1Sqr = v1.LengthSqr();
		if ( dist1Sqr < epsilon * epsilon ) {
			continue;
		}

		a = i;
		b = ( i - 2 + numPoints ) % numPoints;
		c = ( i - 1 + numPoints ) % numPoints;
		v1 = p[c].ToVec3() - p[a].ToVec3();
		v2 = p[b].ToVec3() - p[a].ToVec3();
		lengthSqr = v1.LengthSqr();
		if ( lengthSqr < epsilon * epsilon ) {
			continue;
		}
		v1 = v2 - ( v1 * v2 / lengthSqr )  * v1;
		dist2Sqr = v1.LengthSqr();
		if ( dist2Sqr < epsilon * epsilon ) {
			continue;
		}

		if ( dist1Sqr + dist2Sqr > bestDistSqr ) {
			bestDistSqr = dist1Sqr + dist2Sqr;
			bestCorner = i;
			}
		}

	if ( bestCorner != -1 ) {
		// fan out from corner
		for ( i = 0; i < numPoints - 2; i++ ) {
			indices[i*3+0] = bestCorner;
			indices[i*3+1] = ( bestCorner + i + 1 ) % numPoints;
			indices[i*3+2] = ( bestCorner + i + 2 ) % numPoints;
		}
		return ( numPoints - 2 ) * 3;
	} else {
		// fan out from center
		// FIXME: could also try ear clipping
		for ( i = 0; i < numPoints; i++ ) {
			indices[i*3+0] = numPoints;					// index of center = numPoints
			indices[i*3+1] = i;
			indices[i*3+2] = ( i + 1 ) % numPoints;
		}
		return numPoints * 3;
	}
}

/*
=============
idWinding::IsTiny
=============
*/
#define	EDGE_LENGTH		0.2f

bool idWinding::IsTiny( void ) const {
	int		i;
	float	len;
	idVec3	delta;
	int		edges;

	edges = 0;
	for ( i = 0; i < numPoints; i++ ) {
		delta = p[(i+1)%numPoints].ToVec3() - p[i].ToVec3();
		len = delta.Length();
		if ( len > EDGE_LENGTH ) {
			if ( ++edges == 3 ) {
				return false;
			}
		}
	}
	return true;
}

/*
=============
idWinding::IsTiny
=============
*/
bool idWinding::IsTiny( float epsilon ) const {
	int		i;
	float	len;
	idVec3	delta;
	int		edges;

	edges = 0;
	for ( i = 0; i < numPoints; i++ ) {
		delta = p[(i+1)%numPoints].ToVec3() - p[i].ToVec3();
		len = delta.Length();
		if ( len > epsilon ) {
			if ( ++edges == 3 ) {
				return false;
			}
		}
	}
	return true;
}

/*
=============
idWinding::IsHuge
=============
*/
bool idWinding::IsHuge( float radius ) const {
	int i, j;

	for ( i = 0; i < numPoints; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			if ( p[i][j] <= -radius || p[i][j] >= radius ) {
				return true;
			}
		}
	}
	return false;
}


/*
=============
idWinding::IsHuge
=============
*/
bool idWinding::IsHuge( void ) const {
	int i, j;

	for ( i = 0; i < numPoints; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			if ( p[i][j] <= MIN_WORLD_COORD || p[i][j] >= MAX_WORLD_COORD ) {
				return true;
			}
		}
	}
	return false;
}

/*
=============
idWinding::Print
=============
*/
void idWinding::Print( void ) const {
	int i;

	for ( i = 0; i < numPoints; i++ ) {
		idLib::common->Printf( "(%5.1f, %5.1f, %5.1f)\n", p[i][0], p[i][1], p[i][2] );
	}
}

/*
=============
idWinding::PlaneDistance
=============
*/
float idWinding::PlaneDistance( const idPlane &plane ) const {
	int		i;
	float	d, min, max;

	min = idMath::INFINITY;
	max = -idMath::INFINITY;
	for ( i = 0; i < numPoints; i++ ) {
		d = plane.Distance( p[i].ToVec3() );
		if ( d < min ) {
			min = d;
			if ( IEEE_FLT_SIGNBITSET( min ) & IEEE_FLT_SIGNBITNOTSET( max ) ) {
				return 0.0f;
			}
		}
		if ( d > max ) {
			max = d;
			if ( IEEE_FLT_SIGNBITSET( min ) & IEEE_FLT_SIGNBITNOTSET( max ) ) {
				return 0.0f;
			}
		}
	}
	if ( IEEE_FLT_SIGNBITNOTSET( min ) ) {
		return min;
	}
	if ( IEEE_FLT_SIGNBITSET( max ) ) {
		return max;
	}
	return 0.0f;
}

/*
=============
idWinding::PlaneSide
=============
*/
int idWinding::PlaneSide( const idPlane &plane, const float epsilon ) const {
	bool	front, back;
	int		i;
	float	d;

	front = false;
	back = false;
	for ( i = 0; i < numPoints; i++ ) {
		d = plane.Distance( p[i].ToVec3() );
		if ( d < -epsilon ) {
			if ( front ) {
				return SIDE_CROSS;
			}
			back = true;
			continue;
		} else if ( d > epsilon ) {
			if ( back ) {
				return SIDE_CROSS;
			}
			front = true;
			continue;
		}
	}

	if ( back ) {
		return SIDE_BACK;
	}
	if ( front ) {
		return SIDE_FRONT;
	}
	return SIDE_ON;
}

/*
=============
idWinding::PlanesConcave
=============
*/
#define WCONVEX_EPSILON		0.2f

bool idWinding::PlanesConcave( const idWinding &w2, const idVec3 &normal1, const idVec3 &normal2, float dist1, float dist2 ) const {
	int i;

	// check if one of the points of winding 1 is at the back of the plane of winding 2
	for ( i = 0; i < numPoints; i++ ) {
		if ( normal2 * p[i].ToVec3() - dist2 > WCONVEX_EPSILON ) {
			return true;
		}
	}
	// check if one of the points of winding 2 is at the back of the plane of winding 1
	for ( i = 0; i < w2.numPoints; i++ ) {
		if ( normal1 * w2.p[i].ToVec3() - dist1 > WCONVEX_EPSILON ) {
			return true;
		}
	}

	return false;
}

/*
=============
idWinding::PointInside
=============
*/
bool idWinding::PointInside( const idVec3 &normal, const idVec3 &point, const float epsilon ) const {
	int i;
	idVec3 dir, n, pointvec;

	for ( i = 0; i < numPoints; i++ ) {
		dir = p[(i+1) % numPoints].ToVec3() - p[i].ToVec3();
		pointvec = point - p[i].ToVec3();

		n = dir.Cross( normal );
		n.Normalize();

		if ( pointvec * n < -epsilon ) {
			return false;
		}
	}
	return true;
}

/*
=============
idWinding::LineIntersection
=============
*/
bool idWinding::LineIntersection( const idPlane &windingPlane, const idVec3 &start, const idVec3 &end, bool backFaceCull ) const {
	float front, back, frac;
	idVec3 mid;

	front = windingPlane.Distance( start );
	back = windingPlane.Distance( end );

	// if both points at the same side of the plane
	if ( front < 0.0f && back < 0.0f ) {
		return false;
	}

	if ( front > 0.0f && back > 0.0f ) {
		return false;
	}

	// if back face culled
	if ( backFaceCull && front < 0.0f ) {
		return false;
	}

	// get point of intersection with winding plane
	if ( idMath::Fabs(front - back) < 0.0001f ) {
		mid = end;
	}
	else {
		frac = front / (front - back);
		mid[0] = start[0] + (end[0] - start[0]) * frac;
		mid[1] = start[1] + (end[1] - start[1]) * frac;
		mid[2] = start[2] + (end[2] - start[2]) * frac;
	}

	return PointInside( windingPlane.Normal(), mid, 0.0f );
}

/*
=============
idWinding::RayIntersection
=============
*/
bool idWinding::RayIntersection( const idPlane &windingPlane, const idVec3 &start, const idVec3 &dir, float &scale, bool backFaceCull ) const {
	int i;
	bool side, lastside = false;
	idPluecker pl1, pl2;

	scale = 0.0f;
	pl1.FromRay( start, dir );
	for ( i = 0; i < numPoints; i++ ) {
		pl2.FromLine( p[i].ToVec3(), p[(i+1)%numPoints].ToVec3() );
		side = pl1.PermutedInnerProduct( pl2 ) > 0.0f;
		if ( i && side != lastside ) {
			return false;
		}
		lastside = side;
	}
	if ( !backFaceCull || lastside ) {
		windingPlane.RayIntersection( start, dir, scale );
		return true;
	}
	return false;
}

//===============================================================
//
//	idGrowingWinding
//
//===============================================================


bool	idGrowingWinding::ReAllocate( int n, bool keep ) {
	idVec5 *oldP = p;

	n = (n+3) & ~3;	// align up to multiple of four
	p = new idVec5[n];
	if ( oldP ) {
		if ( keep ) {
			memcpy( p, oldP, numPoints * sizeof(p[0]) );
		}
		if ( oldP != data ) {
			delete[] oldP;
		}
	}
	allocedSize = n;

	return true;
}


//===============================================================
//
//	idFixedWinding
//
//===============================================================

/*
=============
idFixedWinding::ReAllocate
=============
*/
bool idFixedWinding::ReAllocate( int n, bool keep ) {

	assert( n <= MAX_POINTS_ON_WINDING );

	if ( n > MAX_POINTS_ON_WINDING ) {
		idLib::common->Printf("WARNING: idFixedWinding -> MAX_POINTS_ON_WINDING overflowed\n");
		return false;
	}
	return true;
}

/*
=============
idFixedWinding::SplitInPlace
=============
*/
int idFixedWinding::SplitInPlace( const idPlane &plane, const float epsilon, idFixedWinding *back ) {
	int		counts[3];
	double	dists[MAX_POINTS_ON_WINDING+4];
	byte	sides[MAX_POINTS_ON_WINDING+4];
	int		i, j;
	idVec5 *p1, *p2;
	idVec5	mid;
	idFixedWinding out;

	CalculateSides( p, numPoints, plane, epsilon, dists, sides, counts );

	if ( !counts[SIDE_BACK] ) {
		if ( !counts[SIDE_FRONT] ) {
			return SIDE_ON;
		} else {
			return SIDE_FRONT;
		}
	}
	
	if ( !counts[SIDE_FRONT] ) {
		return SIDE_BACK;
	}

	out.numPoints = 0;
	back->numPoints = 0;

	for ( i = 0; i < numPoints; i++ ) {
		p1 = &p[i];

		if ( !out.EnsureAlloced( out.numPoints+1, true ) ) {
			return SIDE_FRONT;		// can't split -- fall back to original
		}
		if ( !back->EnsureAlloced( back->numPoints+1, true ) ) {
			return SIDE_FRONT;		// can't split -- fall back to original
		}

		if ( sides[i] == SIDE_ON ) {
			out.p[out.numPoints] = *p1;
			out.numPoints++;
			back->p[back->numPoints] = *p1;
			back->numPoints++;
			continue;
		}
	
		if ( sides[i] == SIDE_FRONT ) {
			out.p[out.numPoints] = *p1;
			out.numPoints++;
		}
		if ( sides[i] == SIDE_BACK ) {
			back->p[back->numPoints] = *p1;
			back->numPoints++;
		}
		
		if ( sides[i+1] == SIDE_ON || sides[i+1] == sides[i] ) {
			continue;
		}
			
		if ( !out.EnsureAlloced( out.numPoints+1, true ) ) {
			return SIDE_FRONT;		// can't split -- fall back to original
		}

		if ( !back->EnsureAlloced( back->numPoints+1, true ) ) {
			return SIDE_FRONT;		// can't split -- fall back to original
		}

		// generate a split point
		j = i + 1;
		if ( j >= numPoints ) {
			p2 = &p[0];
		} else {
			p2 = &p[j];
		}

		double d = dists[i] / ( dists[i] - dists[i+1] );
		for ( j = 0; j < 3; j++ ) {
			// avoid round off error when possible
			if ( plane.Normal()[j] == 1.0f ) {
				mid[j] = plane.Dist();
			} else if ( plane.Normal()[j] == -1.0f ) {
				mid[j] = -plane.Dist();
			} else {
				mid[j] = (double)(*p1)[j] + d * ( (double)(*p2)[j] - (double)(*p1)[j] );
			}
		}
		mid.s = (double)p1->s + d * ( (double)p2->s - (double)p1->s );
		mid.t = (double)p1->t + d * ( (double)p2->t - (double)p1->t );
			
		out.p[out.numPoints] = mid;
		out.numPoints++;
		back->p[back->numPoints] = mid;
		back->numPoints++;
	}
	for ( i = 0; i < out.numPoints; i++ ) {
		p[i] = out.p[i];
	}
	numPoints = out.numPoints;

	return SIDE_CROSS;
}

