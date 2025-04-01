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
#include "PlaneSet.h"


idCVar dmap_planeHashing(
	"dmap_planeHashing", "1", CVAR_BOOL | CVAR_SYSTEM,
	"Use faster algorithm for hashing planes in idPlaneSet.\n"
	"May affect dmap behavior if very close planes are present.\n"
	"(small performance improvement in TDM 2.10 and later)"
);


int idPlaneSet::FindPlane( const idPlane &plane, const float normalEps, const float distEps )
{
	if (dmap_planeHashing.GetBool()) {

		//stgatilov: use full-fledged cellular hash by all 4 coordinates
		auto HashOfCell = [](int a, int b, int c, int d) -> int {
			int cellHash = (
				unsigned(a) * 3084996963ULL +
				unsigned(b) * 1292913987ULL +
				unsigned(c) * 1367130551ULL +
				unsigned(d) * 2654435769ULL
			) % 1000000007;	//note: non-power-of-two modulo is important here!
			return cellHash;
		};

		//cells of size 1/4 by distance to origin
		//      of size 1/32 by normal components
		static const double invCellSize[4] = {32.0, 32.0, 32.0, 4.0};
		const double eps[4] = {normalEps, normalEps, normalEps, distEps};

		int bnds[2][4];	//inclusive bounds of cells to search in
		for (int i = 0; i < 4; i++) {
			bnds[0][i] = (int)floor((plane[i] - eps[i]) * invCellSize[i] + 0.5);
			bnds[1][i] = (int)floor((plane[i] + eps[i]) * invCellSize[i] + 0.5);
		}

		//while we can search up to 16 cells here, in most cases we consider only one
		for (int a = bnds[0][0]; a <= bnds[1][0]; a++)
			for (int b = bnds[0][1]; b <= bnds[1][1]; b++)
				for (int c = bnds[0][2]; c <= bnds[1][2]; c++)
					for (int d = bnds[0][3]; d <= bnds[1][3]; d++) {
						int cellHash = HashOfCell(a, b, c, d);
						for (int i = hash.First(cellHash); i >= 0; i = hash.Next(i)) {
							if ((*this)[i].Compare(plane, normalEps, distEps)) {
								return i;
							}
						}
					}

		//for some reason, it is important which side is added first
		//(learned the hard way on NHAT3:politics)
		int negativeFirst = false;
		if (plane.Type() >= PLANETYPE_NEGX && plane.Type() < PLANETYPE_TRUEAXIAL)
			negativeFirst = true;

		for (int neg = 0; neg < 2; neg++) {
			idPlane added = (neg ^ negativeFirst ? -plane : plane);
			int ctr[4];		//the main cell to add new plane to
			for (int i = 0; i < 4; i++)
				ctr[i] = (int)floor(added[i] * invCellSize[i] + 0.5);
			int cellHash = HashOfCell(ctr[0], ctr[1], ctr[2], ctr[3]);
			AddGrow(added);
			hash.Add(cellHash, Num() - 1);
		}
		return Num() - 2 + negativeFirst;

	}
	else {

		//stgatilov: in the old code, hash cells have size = 8.0 by plan distance only
		//mappers often create many func_static-s with brushes and patches
		//they have low coordinates, and mostly get into same hash cell
		//(see e.g. NHAT3:politics with its stalagmytes)
		int i, border, hashKey;

		assert( distEps <= 0.125f );

		hashKey = (int)( idMath::Fabs( plane.Dist() ) * 0.125f );
		for ( border = -1; border <= 1; border++ ) {
			for ( i = hash.First( hashKey + border ); i >= 0; i = hash.Next( i ) ) {
				if ( (*this)[i].Compare( plane, normalEps, distEps ) ) {
					return i;
				}
			}
		}

		if ( plane.Type() >= PLANETYPE_NEGX && plane.Type() < PLANETYPE_TRUEAXIAL ) {
			Append( -plane );
			hash.Add( hashKey, Num()-1 );
			Append( plane );
			hash.Add( hashKey, Num()-1 );
			return ( Num() - 1 );
		}
		else {
			Append( plane );
			hash.Add( hashKey, Num()-1 );
			Append( -plane );
			hash.Add( hashKey, Num()-1 );
			return ( Num() - 2 );
		}

	}
}

void idPlaneSet::ClearFree( void ) {
	idList<idPlane>::ClearFree();
	hash.ClearFree();
}

void idPlaneSet::Init( int newHashSize, int newIndexSize ) {
	idList<idPlane>::AssureSize( newIndexSize );
	idList<idPlane>::SetNum( 0, false );
	hash.ClearFree(newHashSize, newIndexSize);
}
