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
#include "PoissonSampling.h"


#include "../tests/testing.h"

TEST_CASE("PoissonSampling") {
	const int RES = 30;
	const int N = RES * RES;
	const int RUNS = 10;

	idRandom rnd;
	auto GenerateRandom = [&]() -> idVec2 {
		idVec2 res;
		res.x = rnd.RandomFloat();
		res.y = rnd.RandomFloat();
		return res;
	};

	idList<idVec2> samples;
	samples.SetNum( N );

	float avgDistance[2] = { 0.0f };

	for ( int t = 0; t < 2; t++ ) {
		for ( int attempt = 0; attempt < RUNS; attempt++ ) {
			CHECK( samples.Num() == N );
			samples.FillZero();

			if ( t == 0 ) {
				for ( idVec2 &p : samples ) {
					p = GenerateRandom();
				}
			} else {
				exponentialSearchParams_t params = { idMath::InvSqrt( N ), 1e-9f, 1.0f, 0.1f, 1e-9f};
				bool ok = GeneratePoissonSamples(
					samples, 10, params, GenerateRandom,
					[]( idVec2 a, idVec2 b, float thresh ) -> bool {
						float distSq = ( a - b ).LengthSqr();
						return distSq <= thresh * thresh;
					}
				);
				CHECK( ok );
			}

			float closest = FLT_MAX;
			for ( int i = 0; i < samples.Num(); i++ )
				for ( int j = 0; j < i; j++ ) {
					float dist = ( samples[i] - samples[j] ).Length();
					closest = idMath::Fmin( closest, dist );
				}
			avgDistance[t] += closest;
		}

		avgDistance[t] /= RUNS;
	}

	// note: these checks are very unlikely to fail...
	float expected = 0.5f * idMath::InvSqrt( N );
	CHECK( avgDistance[1] > 0.5f * expected );
	CHECK( avgDistance[0] < 0.1f * expected );
}
