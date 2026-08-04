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
#include "BinSearch.h"


float idExponentialSearch_MaxTrue( bool (*condition)(void*, float), void *context, const exponentialSearchParams_t &params ) {
	assert( params.minValue > 0.0f );
	assert( params.maxValue > params.minValue );
	float value = idMath::ClampFloat( params.minValue, params.maxValue, params.initValue );

	float minBound, maxBound;
	if ( condition( context, value ) ) {
		while ( 1 ) {
			float nextValue = idMath::Fmin( value * 2.0f, params.maxValue );
			if ( value == nextValue )
				return value;	// true even at maxValue
			if ( !condition( context, nextValue ) ) {
				minBound = value;
				maxBound = nextValue;
				break;
			}
			value = nextValue;
		}
	} else {
		while ( 1 ) {
			float prevValue = idMath::Fmax( value * 0.5f, params.minValue );
			if ( value == prevValue )
				return value * ( 1.0f - 5.0f * idMath::FLT_EPS );	// false even at minValue
			if ( condition( context, prevValue ) ) {
				minBound = prevValue;
				maxBound = value;
				break;
			}
			value = prevValue;
		}
	}

	while ( maxBound - minBound > idMath::Fmax( params.absolutePrecision, minBound * params.relativePrecision ) ) {
		float middle = ( minBound + maxBound ) * 0.5f;
		if ( condition( context, middle ) ) {
			minBound = middle;
		} else {
			maxBound = middle;
		}
	}

	return minBound;
}

#include "../tests/testing.h"

TEST_CASE("ExponentialSearch") {
	static const float minBound = 1e-5f;
	static const float maxBound = 1e+5f;
	static const float relativePrecision = 1e-5f;
	static const float absolutePrecision = 1e-5f;

	idRandom rnd;
	for ( int attempt = 0; attempt < 10000; attempt++ ) {

		float exactAnswer = minBound / 10.0f * idMath::Pow( 100.0f * maxBound / minBound, rnd.RandomFloat() );
		float initValue = minBound / 10.0f * idMath::Pow( 100.0f * maxBound / minBound, rnd.RandomFloat() );
		float areaBound = idMath::PI * exactAnswer * exactAnswer;

		auto CircleAreaBounded = [&] ( float radius ) -> bool {
			float area = idMath::PI * radius * radius;
			return area <= areaBound;
		};

		exponentialSearchParams_t params = { initValue, minBound, maxBound, relativePrecision, absolutePrecision };
		float answer = idExponentialSearch_MaxTrue( LambdaToFuncPtr( CircleAreaBounded ), &CircleAreaBounded, params );

		bool good;
		if ( exactAnswer < minBound * 0.999f ) {
			good = ( answer < minBound );
		} else if ( exactAnswer > maxBound * 1.001f ) {
			good = ( answer == maxBound );
		} else {
			float diff = idMath::Fabs( answer - idMath::ClampFloat( minBound, maxBound, exactAnswer ) );
			float threshold = idMath::Fmax( absolutePrecision, relativePrecision * exactAnswer );
			good = ( diff <= threshold * 1.002f );
		}
		CHECK( good );

		if ( answer >= minBound && answer <= maxBound )
			CHECK( CircleAreaBounded( answer ) );
	}
}

