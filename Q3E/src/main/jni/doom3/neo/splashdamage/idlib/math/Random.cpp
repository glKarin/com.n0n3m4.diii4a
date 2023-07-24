// Copyright (C) 2005 Splash Damage Ltd.
//

#include "../precompiled.h"
#pragma hdrstop

idRandom idRandom::staticRandom;

idVec3 idRandom::RandomVectorInCone( float halfOpeningAngle ) {
	// This should give uniform distribution: http://www.flipcode.org/cgi-bin/fcmsg.cgi?thread_show=7028

	// Random height between cos(hO), 1
	float c = idMath::Cos( DEG2RAD( halfOpeningAngle ) );
	float z = c + ( 1.0f - c ) * RandomFloat();

	// Random between 0, 2 PI
	float a = RandomFloat() * ( 2.0f * idMath::PI );

	// Make sure vector is normalized & apply azimuth
	float r = idMath::Sqrt( 1.0f - z*z );
	float sin, cos; idMath::SinCos( a, sin, cos );
	return idVec3( r * cos, r * sin, z );
}

idVec3 idRandom::RandomVectorInCone( idVec3 dir, float halfOpeningAngle ) {

	// Random height between cos(hO), 1
	float c = idMath::Cos( DEG2RAD( halfOpeningAngle ) );
	float x = c + ( 1.0f - c ) * RandomFloat();

	// Random between 0, 2 PI
	float a = RandomFloat() * ( 2.0f * idMath::PI );

	// Make sure vector is normalized & apply azimuth
	float r = idMath::Sqrt( 1.0f - x*x );
	float sin, cos; idMath::SinCos( a, sin, cos );
	idVec3 result( x, r * cos, r * sin );

	return result * dir.ToMat3();
}