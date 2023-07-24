// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __PLANESET_H__
#define __PLANESET_H__

/*
===============================================================================

	Plane Set

===============================================================================
*/

class idPlaneSet : public idList<idPlane> {
public:

	void					Clear( void ) { idList<idPlane>::Clear(); hash.Free(); }

	int						FindPlane( const idPlane &plane, const float normalEps, const float distEps );

private:
	idHashIndex				hash;
};

ID_INLINE int idPlaneSet::FindPlane( const idPlane &plane, const float normalEps, const float distEps ) {
	int i, border, hashKey;

	assert( distEps <= 0.125f );

	hashKey = idMath::Ftoi( idMath::Fabs( plane.Dist() ) * 0.125f );
	for ( border = -1; border <= 1; border++ ) {
		for ( i = hash.GetFirst( hashKey + border ); i != idHashIndex::NULL_INDEX; i = hash.GetNext( i ) ) {
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

#endif /* !__PLANESET_H__ */
