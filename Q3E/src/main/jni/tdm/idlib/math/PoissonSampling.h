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
#pragma once

#include "containers/BinSearch.h"

template<class Sample, class GenSample, class AreClose>
bool GeneratePoissonSamples(
	idList<Sample> &samples,
	int retries, const exponentialSearchParams_t &searchParams,
	const GenSample &genRandomSample, const AreClose &areSamplesClose
) {
	int count = samples.Num();

	float bestThresh = -FLT_MAX;
	idList<Sample> work;
	work.SetNum( count );

	auto CanGenerateWithThreshold = [&]( float thresh ) -> bool {
		for ( int i = 0; i < count; i++ ) {

			// generate sample several times, trying to respect proximity threshold
			bool found = false;
			for ( int att = 0; att < retries; att++ ) {
				work[i] = genRandomSample();

				bool good = true;
				for ( int j = 0; j < i; j++ ) {
					if ( areSamplesClose( work[i], work[j], thresh ) ) {
						good = false;
						break;
					}
				}

				if ( good ) {
					found = true;
					break;
				}
			}

			if ( !found )
				return false;
		}

		// save best successful samples into output
		if ( bestThresh < thresh ) {
			bestThresh = thresh;
			samples = work;
		}
		return true;
	};

	if ( count > 1 ) {
		idExponentialSearch_MaxTrue( LambdaToFuncPtr( CanGenerateWithThreshold ), &CanGenerateWithThreshold, searchParams );
		return ( bestThresh > -FLT_MAX );
	} else {
		// no need to retry if there is 0 or 1 samples (under threshold automatically)
		return CanGenerateWithThreshold( searchParams.initValue );
	}
}
