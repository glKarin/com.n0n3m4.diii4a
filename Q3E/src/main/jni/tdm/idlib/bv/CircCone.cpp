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
#include "CircCone.h"


static ID_FORCE_INLINE void AngleSum( float cosA, float sinA, float cosB, float sinB, float &cosS, float &sinS ) {
	float newCos = cosA * cosB - sinA * sinB;
	float newSin = sinA * cosB + cosA * sinB;
	cosS = newCos;
	sinS = newSin;
}

void idCircCone::SetAngle( const idVec3 &ax, float angle ) {
	axis = ax;
	if ( axis.Normalize() <= 1e-4f )
		MakeFull();
	else
		idMath::SinCos( angle, sinAngle, cosAngle );
}
void idCircCone::SetBox( const idVec3 &origin, const idBounds &box ) {
	float radius = 0.5f * box.GetSize().Length();
	axis = box.GetCenter() - origin;
	float distance = axis.Normalize();
	if ( radius >= distance ) {
		MakeFull();
		return;
	}
	sinAngle = radius / distance;
	assert( sinAngle >= 0.0f && sinAngle < 1.0f );
	cosAngle = idMath::Sqrt( 1.0f - sinAngle * sinAngle );
}

float idCircCone::GetAngle() const {
	return idMath::ATan( sinAngle, cosAngle );
}

bool idCircCone::ContainsDir( const idVec3 &v ) const {
	float cosInter = axis * v;
	if ( fabsf( cosInter - cosAngle ) > 1e-2f )
		return cosInter >= cosAngle;
	float sinInter = axis.Cross( v ).Length();
	return sinInter * cosAngle <= sinAngle * cosInter;
}
bool idCircCone::ContainsVec( const idVec3 &v ) const {
	idVec3 vnorm = v;
	if ( vnorm.Normalize() <= 1e-10f )
		return !IsEmpty();
	return ContainsDir( vnorm );
}

idCircCone idCircCone::ExpandAngle( float d ) const {
	idCircCone res = *this;
	res.ExpandAngleSelf( d );
	return res;
}
idCircCone &idCircCone::ExpandAngleSelf( float d ) {
	float dcos, dsin;
	idMath::SinCos( d, dsin, dcos );
	ExpandSelf( dcos, dsin );
	return *this;
}
idCircCone &idCircCone::ExpandSelf( float dcos, float dsin ) {
	// delta must be in [0; 180) degrees
	assert( dsin >= 0.0f && dcos > -1.0f );

	if ( axis.x == 0.0f && axis.y == 0.0f && axis.z == 0.0f )
		return *this;	// empty / full

	AngleSum( cosAngle, sinAngle, dcos, dsin, cosAngle, sinAngle );

	if ( sinAngle < 0.0f )
		MakeFull();

	return *this;
}

bool idCircCone::HaveEqual( const idCircCone &c ) const {
	if ( IsEmpty() || c.IsEmpty() )
		return false;
	if ( IsFull() || c.IsFull() )
		return true;

	float cosInter = axis * c.axis;
	float cosSum, sinSum;
	AngleSum( cosAngle, sinAngle, c.cosAngle, c.sinAngle, cosSum, sinSum );

	if ( sinSum < 0.0f )
		return true;	// sum of angles > 180 degrees

	return cosInter >= cosSum;
}
bool idCircCone::HaveCollinear( const idCircCone &c ) const {
	if ( IsEmpty() || c.IsEmpty() )
		return false;
	if ( IsFull() || c.IsFull() )
		return true;

	float cosInter = axis * c.axis;
	float cosSum, sinSum;
	AngleSum( cosAngle, sinAngle, c.cosAngle, c.sinAngle, cosSum, sinSum );

	if ( sinSum < 0.0f )
		return true;	// sum of angles > 180 degrees

	return fabsf( cosInter ) >= cosSum;
}
bool idCircCone::HaveOrthogonal( const idCircCone &c ) const {
	if ( IsEmpty() || c.IsEmpty() )
		return false;
	if ( IsFull() || c.IsFull() )
		return true;

	float cosInter = axis * c.axis;
	float cosSum, sinSum;
	AngleSum( cosAngle, sinAngle, c.cosAngle, c.sinAngle, cosSum, sinSum );

	if ( sinSum < 0.0f || cosSum < 0.0f )
		return true;	// sum of angles > 90 degrees

	return fabsf( cosInter ) <= sinSum;
}
int idCircCone::HaveSameDirection( const idCircCone &c ) const {
	if ( HaveOrthogonal(c) )
		return 0;
	float cosInter = axis * c.axis;
	if ( cosInter > 0.0 )
		return 1;
	if ( cosInter < 0.0 )
		return -1;
	return 0;
}

void idCircCone::AddDir( const idVec3 &v ) {
	if ( ContainsDir( v ) )
		return;

	if ( IsEmpty() ) {
		axis = v;
		cosAngle = 1.0f;
		sinAngle = 0.0f;
		return;
	}

	float cosInter = axis * v;
	float sinInter = axis.Cross( v ).Length();
	float cosSum, sinSum;
	AngleSum( cosInter, sinInter, cosAngle, sinAngle, cosSum, sinSum );
	if ( sinSum <= 1e-4f && ( cosSum < 0.0f || sinSum < 0.0f ) ) {
		// this would be reflex cone, which are rarely more useful than full cones
		MakeFull();
		return;
	}
	float cosDiff, sinDiff;
	AngleSum( cosInter, sinInter, cosAngle, -sinAngle, cosDiff, sinDiff );

	// note: we have to use complicated formulas with cases
	// in order to obtain good precision, i.e. O(eps) instead of O(sqrt(eps))
	float cosHalfSum, sinHalfSum;
	if ( cosSum >= 0.0f ) {
		cosHalfSum = idMath::Sqrt( (1.0f + cosSum) * 0.5f );
		sinHalfSum = sinSum / cosHalfSum * 0.5f;
	}
	else {
		sinHalfSum = idMath::Sqrt( (1.0f - cosSum) * 0.5f );
		cosHalfSum = sinSum / sinHalfSum * 0.5f;
	}
	float sinHalfDiff;
	if ( cosDiff > 0.0f ) {
		float cosHalfDiff = idMath::Sqrt( (1.0f + cosDiff) * 0.5f );
		sinHalfDiff = sinDiff / cosHalfDiff * 0.5f;
	}
	else {
		sinHalfDiff = idMath::Sqrt( (1.0f - cosDiff) * 0.5f );
	}

	if ( sinSum > 0.01f ) {
		float inv = 1.0f / sinInter;
		float coeffAxis = sinHalfSum * inv;
		float coeffDir = sinHalfDiff * inv;
		axis = coeffAxis * axis + coeffDir * v;
		cosAngle = cosHalfSum;
		sinAngle = sinHalfSum;
	}
	else {
		// almost zero angle between axis and dir
		cosAngle = cosInter;
		sinAngle = sinInter;
	}
}
void idCircCone::AddVec( const idVec3 &v ) {
	idVec3 unit = v;
	if ( unit.Normalize() <= 1e-4f )
		MakeFull();
	else
		AddDir( unit );
}
void idCircCone::AddDirSaveAxis( const idVec3 &v ) {
	assert( !IsEmpty() );
	float cosInter = axis * v;
	float sinInter = axis.Cross( v ).Length();
	if ( sinInter * cosAngle > sinAngle * cosInter ) {
		cosAngle = cosInter;
		sinAngle = sinInter;
	}
}
void idCircCone::AddVecSaveAxis( const idVec3 &v ) {
	idVec3 unit = v;
	if ( unit.Normalize() <= 1e-4f )
		MakeFull();
	else
		AddDirSaveAxis( unit );
}
/*void idCircCone::AddCone( const idCircCone &c ) {
	//TODO
}*/
void idCircCone::AddConeSaveAxis( const idCircCone &c ) {
	assert( !IsEmpty() );
	float cosInter = axis * c.axis;
	float sinInter = axis.Cross( c.axis ).Length();
	float cosSum, sinSum;
	AngleSum( cosInter, sinInter, c.cosAngle, c.sinAngle, cosSum, sinSum );
	if ( sinSum <= 1e-4f && ( sinSum < 0.0f || cosInter < 0.0f || c.cosAngle < 0.0f ) )
		MakeFull();
	else if ( sinSum * cosAngle > sinAngle * cosSum ) {
		cosAngle = cosSum;
		sinAngle = sinSum;
	}
}
void idCircCone::FromVectors( const idVec3 *vectors, int num ) {
	Clear();
	for ( int i = 0; i < num; i++ ) {
		AddVec( vectors[i] );
		if ( IsFull() )
			break;
	}
	// avoid denormalization due to loss of precision
	Normalize();
}


#include "../tests/testing.h"

TEST_CASE("CircCone:Simple") {
	idCircCone cone;
	cone.SetAngle( idVec3( 1.0f, 2.0f, 3.0f ), idMath::PI / 3.0f );

	CHECK( fabsf( cone.GetAxis().Length() - 1.0f ) <= 1e-6f );
	CHECK( fabsf( cone.GetAngle() - idMath::PI / 3.0f ) <= 1e-6f );
	CHECK( fabsf( cone.GetCos() - 0.5f ) <= 1e-6f );
	CHECK( fabsf( cone.GetSin() - idMath::SQRT_THREE / 2.0f ) <= 1e-6f );
	CHECK( cone.IsEmpty() == false );
	CHECK( cone.IsFull() == false );
	CHECK( cone.IsConvex() == true );
	CHECK( cone.ContainsVec( idVec3( 1.0f, 2.0f, 3.0f ) ) == true );
	CHECK( cone.ContainsDir( idVec3( 0.0f, 0.0f, 1.0f ) ) == true );
	CHECK( cone.ContainsDir( idVec3( 0.0f, 0.0f, -1.0f ) ) == false );
	CHECK( cone.ContainsDir( idVec3( 1.0f, 0.0f, 0.0f ) ) == false );
	CHECK( cone.ContainsVecFast( idVec3( 1.0f, 2.0f, 3.0f ) ) == true );
	CHECK( cone.ContainsDirFast( idVec3( 0.0f, 0.0f, 1.0f ) ) == true );
	CHECK( cone.ContainsDirFast( idVec3( 0.0f, 0.0f, -1.0f ) ) == false );
	CHECK( cone.ContainsDirFast( idVec3( 1.0f, 0.0f, 0.0f ) ) == false );

	cone.ExpandAngleSelf( idMath::PI / 6.0f );
	CHECK( ( cone.GetAxis() - idVec3( 1.0f, 2.0f, 3.0f ).Normalized() ).Length() <= 1e-6f );
	CHECK( fabsf( cone.GetAngle() - idMath::PI / 2.0f ) <= 1e-6f );
	CHECK( fabsf( cone.GetCos() - 0.0f ) <= 1e-6f );
	CHECK( fabsf( cone.GetSin() - 1.0f ) <= 1e-6f );
	CHECK( cone.IsEmpty() == false );
	CHECK( cone.IsFull() == false );
	CHECK( cone.IsConvex() == false );
	CHECK( cone.ContainsVec( idVec3( 1.0f, 2.0f, 3.0f ) ) == true );
	CHECK( cone.ContainsDir( idVec3( 1.0f, 0.0f, 0.0f ) ) == true );

	cone.ExpandAngleSelf( idMath::PI / 6.0f );
	CHECK( fabsf( cone.GetAxis().Cross( idVec3(1.0f, 2.0f, 3.0f) ).Length() ) <= 1e-6f );
	CHECK( fabsf( cone.GetAxis().Length() - 1.0f ) <= 1e-6f );
	CHECK( fabsf( cone.GetAngle() - 2.0f * idMath::PI / 3.0f ) <= 1e-6f );
	CHECK( fabsf( cone.GetCos() + 0.5f ) <= 1e-6f );
	CHECK( fabsf( cone.GetSin() - idMath::SQRT_THREE / 2.0f ) <= 1e-6f );
	CHECK( cone.IsEmpty() == false );
	CHECK( cone.IsFull() == false );
	CHECK( cone.IsConvex() == false );
	CHECK( cone.ContainsVec( idVec3( 1.0f, 2.0f, 3.0f ) ) == true );
	CHECK( cone.ContainsDir( idVec3( 1.0f, 0.0f, 0.0f ) ) == true );
	CHECK( cone.ContainsDir( idVec3( -1.0f, 0.0f, 0.0f ) ) == true );

	idCircCone full1 = cone.ExpandAngle( idMath::PI / 3.0f );
	idCircCone full2 = cone.ExpandAngle( idMath::PI - 0.1f );
	idCircCone full3 = full1.ExpandAngle( 1.0f );
	CHECK( full1.IsFull() == true );
	CHECK( full1.IsEmpty() == false );
	CHECK( full1.IsConvex() == false );
	CHECK( full2.IsFull() == true );
	CHECK( full2.IsEmpty() == false );
	CHECK( full2.IsConvex() == false );
	CHECK( full3.IsFull() == true );
	CHECK( full3.IsEmpty() == false );
	CHECK( full3.IsConvex() == false );
	CHECK( full1.GetAxis().Length() == 0.0f );
	CHECK( full1.GetCos() == -1.0f );
	CHECK( full1.GetSin() == 0.0f );
	CHECK( full2.GetAxis().Length() == 0.0f );
	CHECK( full2.GetCos() == -1.0f );
	CHECK( full2.GetSin() == 0.0f );
	CHECK( full3.GetAxis().Length() == 0.0f );
	CHECK( full3.GetCos() == -1.0f );
	CHECK( full3.GetSin() == 0.0f );

	idCircCone empty;
	empty.Clear();
	CHECK( empty.IsFull() == false );
	CHECK( empty.IsEmpty() == true );
	CHECK( empty.IsConvex() == true );
	CHECK( empty.GetAxis().Length() == 0.0f );
	CHECK( empty.GetCos() == 1.0f );
	CHECK( empty.GetSin() == 0.0f );

	CHECK( cone.ContainsVec( idVec3( 0.0f, 0.0f, 0.0f ) ) == true );
	CHECK( full1.ContainsVec( idVec3( 0.0f, 0.0f, 0.0f ) ) == true );
	CHECK( empty.ContainsVec( idVec3( 0.0f, 0.0f, 0.0f ) ) == false );
	CHECK( cone.ContainsVecFast( idVec3( 0.0f, 0.0f, 0.0f ) ) == true );
	CHECK( full1.ContainsVecFast( idVec3( 0.0f, 0.0f, 0.0f ) ) == true );
	CHECK( empty.ContainsVecFast( idVec3( 0.0f, 0.0f, 0.0f ) ) == false );
	CHECK( full2.ContainsVec( idVec3( -5.0, 3.0, -7.0f ) ) == true );
	CHECK( full3.ContainsVec( idVec3( 555.0, 0.0, 0.0f ) ) == true );
	CHECK( empty.ContainsVec( idVec3( -5.0, 3.0, -7.0f ) ) == false );
	CHECK( empty.ContainsVec( idVec3( 555.0, 0.0, 0.0f ) ) == false );

	cone.MakeFull();
	CHECK( cone.IsFull() == true );
	CHECK( cone.IsEmpty() == false );
	CHECK( cone.IsConvex() == false );
	CHECK( cone.GetAxis().Length() == 0.0f );
	CHECK( cone.GetCos() == -1.0f);
	CHECK( cone.GetSin() == 0.0f);

	cone.Set( idVec3( 0.6f, 0.0f, 0.8f ), 0.8f, 0.6f );
	cone.ContainsVec( idVec3( 0.0f, 0.0f, 1.0f ) );	// exactly on boundary
	idVec3 test1 = idVec3( 1e-6f, 0.0f, 1.0f );
	idVec3 test2 = idVec3( -1e-6f, 0.0f, 1.0f );
	CHECK( cone.ContainsVec( test1 ) == true );
	CHECK( cone.ContainsVec( test2 ) == false );

	for ( int i = 0; i < 300; i++ ) {
		if ( i < 30 ) {
			assert( test1.Length() > 0.0f && test2.Length() > 0.0f );
			CHECK( cone.ContainsVec( test1 ) == true );
			CHECK( cone.ContainsVec( test2 ) == false );
			CHECK( cone.ContainsVecFast( test1 ) == true );
			CHECK( cone.ContainsVecFast( test2 ) == false );
		}
		CHECK( empty.ContainsVec( test1 ) == false );
		CHECK( full1.ContainsVec( test1 ) == true );
		CHECK( empty.ContainsVecFast( test1 ) == false );
		CHECK( full1.ContainsVecFast( test1 ) == true );
		test1 *= 0.5f;
		test2 *= 0.5f;
	}

	CHECK( cone.HaveCollinear( full1 ) == true );
	CHECK( full2.HaveCollinear( full1 ) == true );
	CHECK( cone.HaveCollinear( empty ) == false );
	CHECK( empty.HaveCollinear( empty ) == false );
	CHECK( cone.HaveOrthogonal( full1 ) == true );
	CHECK( full2.HaveOrthogonal( full1 ) == true );
	CHECK( cone.HaveOrthogonal( empty ) == false );
	CHECK( empty.HaveOrthogonal( empty ) == false );

	idCircCone other( idVec3( 0.6f, 0.0f, 0.8f ), 0.8f, 0.6f );
	CHECK( cone.HaveEqual( other ) == true );
	CHECK( cone.HaveEqual( other.Negated() ) == false );
	CHECK( cone.HaveCollinear( other ) == true );
	CHECK( cone.HaveCollinear( other.Negated() ) == true );
	CHECK( cone.HaveOrthogonal( other ) == false );
	CHECK( cone.HaveOrthogonal( other.Negated() ) == false );
	other.Set( idVec3( -0.6f, 0.0f, 0.8f ), 0.8f, 0.6f + 1e-4f );
	other.Normalize();
	CHECK( cone.HaveEqual( other ) == true );
	CHECK( cone.HaveEqual( other.Negated() ) == false );
	CHECK( cone.HaveCollinear( other ) == true );
	CHECK( cone.HaveCollinear( other.Negated() ) == true );
	CHECK( cone.HaveOrthogonal( other ) == true );
	CHECK( cone.HaveOrthogonal( other.Negated() ) == true );
	other.Set( idVec3( -0.6f, 0.0f, 0.8f ), 0.8f, 0.6f - 1e-4f );
	other.Normalize();
	CHECK( cone.HaveEqual( other ) == false );
	CHECK( cone.HaveEqual( other.Negated() ) == false );
	CHECK( cone.HaveCollinear( other ) == false );
	CHECK( cone.HaveCollinear( other.Negated() ) == false );
	CHECK( cone.HaveOrthogonal( other ) == true );
	CHECK( cone.HaveOrthogonal( other.Negated() ) == true );

	other.SetAngle( idVec3( 0.6f, 0.0f, 0.8f ), 0.0f );
	CHECK( other.IsFull() == false );
	CHECK( other.IsEmpty() == false );
	CHECK( other.IsConvex() == true );
	CHECK( other.GetCos() == 1.0f );
	CHECK( other.GetSin() == 0.0f );
	CHECK( other.ContainsDir( idVec3( 0.6f, 0.0f, 0.8f ) ) == true );
	for (int m = 0; m < 8; m++) {
		CHECK( other.ContainsVec( idVec3(
			0.6f + (m & 1 ? -1 : 1) * 1e-2f,
			0.0f + (m & 2 ? -1 : 1) * 1e-2f,
			0.8f + (m & 4 ? -1 : 1) * 1e-2f
		) ) == false );
	}
	CHECK( other.HaveEqual( cone ) == true );
	CHECK( other.HaveEqual( other ) == true );
	CHECK( other.HaveEqual( other.Negated() ) == false );
	CHECK( other.HaveCollinear( other ) == true );
	CHECK( other.HaveCollinear( other.Negated() ) == true );
	CHECK( other.HaveOrthogonal( other ) == false );
	cone.SetAngle( idVec3( 0.0f, 1.0f, 0.0f ), 0.0f );
	CHECK( cone.HaveCollinear( other ) == false );
	CHECK( cone.HaveOrthogonal( other ) == true );

	cone.AddDir( idVec3( 1.0f, 0.0f, 0.0f ) );
	CHECK( fabsf( cone.GetAngle() - idMath::ONEFOURTH_PI ) <= 1e-6f );
	cone.ExpandAngleSelf( 1e-5f );
	CHECK( cone.ContainsDir( idVec3( 1.0f, 0.0f, 0.0f ) ) == true );
	CHECK( cone.ContainsDir( idVec3( 0.0f, 1.0f, 0.0f ) ) == true );
	CHECK( cone.ContainsVec( idVec3( 1.0f, 1.0f, 0.0f ) ) == true );
	CHECK( cone.ContainsVec( idVec3( -1.0f, -1.0f, 0.0f ) ) == false );
	cone.AddVec( idVec3( 0.0f, 0.0f, 0.0f ) );
	CHECK( cone.IsFull() == true );

	cone.SetAngle( idVec3( 1.0f, 2.0f, 3.0f ), 0.3f );
	cone.AddVecSaveAxis( idVec3( 4.0f, 3.0f, 0.0f ) );
	cone.ExpandAngleSelf( 1e-5f );
	CHECK( cone.ContainsVec( idVec3( 1.0f, 2.0f, 3.0f ) ) == true );
	CHECK( cone.ContainsVec( idVec3( 4.0f, 3.0f, 0.0f ) ) == true );
	CHECK( ( cone.GetAxis() - idVec3( 1.0f, 2.0f, 3.0f ).Normalized() ).Length() <= 1e-6f );
	CHECK( cone.GetAngle() < 2.0f );

	cone.SetAngle( idVec3( 1.0f, 0.0f, 0.0f ), idMath::ATan( 0.75f ) + 0.01f );
	other.SetAngle( idVec3( 0.8f, 0.6f, 0.0f ), 0.2f );
	cone.AddConeSaveAxis( other );
	CHECK( ( cone.GetAxis() - idVec3( 1.0f, 0.0f, 0.0f ) ).Length() <= 1e-6f );
	CHECK( fabsf( cone.GetAngle() - ( idMath::ATan( 0.75f ) + 0.2f ) ) <= 1e-6f );
	cone.SetAngle( idVec3( 1.0f, 0.0f, 0.0f ), idMath::ATan( 0.75f ) + 0.3f );
	other.SetAngle( idVec3( 0.8f, 0.6f, 0.0f ), 0.2f );
	cone.AddConeSaveAxis( other );
	CHECK( ( cone.GetAxis() - idVec3( 1.0f, 0.0f, 0.0f ) ).Length() <= 1e-6f );
	CHECK( fabsf( cone.GetAngle() - ( idMath::ATan( 0.75f ) + 0.3f ) ) <= 1e-6f );

	cone.SetBox( idVec3( 1.0f, 1.0f, 1.0f ), idBounds( idVec3( 1.0f, 1.0f, 1.0f ) ) );
	CHECK( cone.IsFull() );
	cone.SetBox( idVec3( 1.0f, 1.0f, 1.0f ), idBounds( idVec3( 2.0f, 1.0f, 1.0f ) ) );
	CHECK( ( cone.GetAxis() - idVec3( 1.0f, 0.0f, 0.0f ) ).Length() <= 1e-6f );
	CHECK( cone.GetAngle() <= 1e-6f );
	idBounds box( idVec3( 4.0f, 10.0f, 10.0f ), idVec3( 6.0f, 14.0f, 16.0f ) );
	cone.SetBox( idVec3( 0.0f, 0.0f, 0.0f ), box );
	CHECK( fabs( cone.GetAngle() - idMath::ASin( idMath::Sqrt( 14.0f / 338.0f ) ) ) <= 1e-6f );
	cone.ExpandAngleSelf( 1e-3f );
	idVec3 pts[8];
	box.ToPoints( pts );
	for ( int v = 0; v < 8; v++ )
		CHECK( cone.ContainsVec(pts[v]) );
}

static idVec3 GenDir( idRandom &rnd ) {
	idVec3 res;
	float sqrLen;
	do {
		res.x = rnd.CRandomFloat();
		res.y = rnd.CRandomFloat();
		res.z = rnd.CRandomFloat();
		sqrLen = res.LengthSqr();
	} while ( sqrLen < 0.1f || sqrLen > 1.0f );
	res *= idMath::InvSqrt( sqrLen );
	return res;
}

TEST_CASE("CircCone:StressCheck") {
	idRandom rnd;

	static const int RUNS = 100000;

	// Contains
	for ( int r = 0; r < RUNS; r++ ) {
		idVec3 axis = GenDir( rnd );
		float angle = rnd.RandomFloat() * idMath::PI;
		idVec3 dir = GenDir( rnd );

		idCircCone cone;
		cone.SetAngle( axis, angle );
		bool contains = cone.ContainsDir( dir );
		bool contains2 = cone.ContainsDirFast( dir );

		float angBetween = idMath::ATan( axis.Cross( dir ).Length(), axis * dir );
		if ( fabsf( angBetween - angle ) <= 1e-5f )
			continue;
		if ( contains != (angBetween <= angle) )
			CHECK( false );
		if ( contains != contains2 )
			CHECK( false );
	}

	// HaveXXX
	for ( int r = 0; r < RUNS; r++ ) {
		idVec3 axisA = GenDir( rnd );
		idVec3 axisB = GenDir( rnd );
		float angleA = rnd.RandomFloat() * idMath::PI;
		float angleB = rnd.RandomFloat() * idMath::PI;

		idCircCone coneA, coneB;
		coneA.SetAngle( axisA, angleA );
		coneB.SetAngle( axisB, angleB );

		float angBetween = idMath::ATan( axisA.Cross( axisB ).Length(), axisA * axisB );
		float angColl = idMath::Fmin( angBetween, idMath::PI - angBetween );
		float angOrtho = idMath::HALF_PI - angColl;
		float angSum = angleA + angleB;

		if ( fabsf( angBetween - angSum ) > 1e-5f ) {
			bool res = coneA.HaveEqual( coneB );
			if ( res != (angBetween <= angSum) )
				CHECK( false );
		}
		if ( fabsf( angColl - angSum ) > 1e-5f ) {
			bool res = coneA.HaveCollinear( coneB );
			if ( res != (angColl <= angSum) )
				CHECK( false );
		}
		if ( fabsf( angOrtho - angSum ) > 1e-5f ) {
			bool res = coneA.HaveOrthogonal( coneB );
			int sign = coneA.HaveSameDirection( coneB );
			if ( res != (angOrtho <= angSum) )
				CHECK( false );
			if ( (sign == 0) != res )
				CHECK( false );
			if ( sign && sign < 0 != angBetween > idMath::HALF_PI )
				CHECK( false );
		}
	}
}

TEST_CASE("CircCone:StressCheck") {
	idRandom rnd;

	static const int RUNS = 100000;

	for ( int r = 0; r < RUNS; r++ ) {
		idVec3 arr[8];
		int k = 1 + rnd.RandomInt( 5 );
		for ( int i = 0; i < k; i++ )
			arr[i] = GenDir( rnd );

		idCircCone cone;
		cone.FromVectors( arr, k );
		cone.ExpandAngleSelf( 1e-4f );
		for ( int i = 0; i < k; i++ )
			if ( !cone.ContainsDir( arr[i] ) )
				CHECK( false );

		float tightAng = 0.0f;
		for ( int i = 0; i < k; i++ ) {
			float angBetween = idMath::ATan( arr[0].Cross( arr[i] ).Length(), arr[0] * arr[i] );
			tightAng = idMath::Fmax( tightAng, angBetween );
		}

		if ( tightAng < 0.5f ) {
			// note: computed bounds are usually not tight
			// I don't know how much larger they can be
			// so we use a rather big tolerance here...
			float maxAllowed = tightAng * 1.5f + 0.1f;
			if ( cone.IsFull() || cone.GetAngle() > maxAllowed )
				CHECK( false );
		}
	}
}
