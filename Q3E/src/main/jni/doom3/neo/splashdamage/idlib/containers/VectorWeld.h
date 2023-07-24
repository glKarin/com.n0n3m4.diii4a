// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __VECTORWELD_H__
#define __VECTORWELD_H__

/*
===============================================================================

	Vector Weld

	Welds Vectors

===============================================================================
*/

template< class type, int dimension >
class idVectorWeld {
public:
							idVectorWeld();
							idVectorWeld( const type &mins, const type &maxs, const int boxHashSize, const int initialSize );

	void					Init( const type &mins, const type &maxs, const int boxHashSize, const int initialSize );
	int						HashSizeForBounds( const type &mins, const type &maxs, const float epsilon ) const;
	void					SetGranularity( int newGranularity );
	void					Clear();

	void					AddVector( type *v, const float epsilon );
	int						GetNumUniqueVectors() const;
	void					Weld( const float integralEpsilon );

private:
	struct idWeldVector;

	struct idWeldGroup {
		idWeldVector *		first;		// first vector in weld group
	};
	struct idWeldVector {
		type *				v;			// pointer to vector
		idWeldGroup *		group;		// weld group this vector is in
		idWeldVector *		next;		// next vector in the weld group
	};

	idList<idWeldVector *>	vectors;
	idList<idWeldGroup *>	groups;
	idHashIndex				hash;
	type					mins;
	type					maxs;
	int						boxHashSize;
	float					boxInvSize[dimension];
	float					boxHalfSize[dimension];

	idBlockAlloc<idWeldGroup,1024>	weldGroupAllocator;
	idBlockAlloc<idWeldVector,1024>	weldVectorAllocator;

	void					MergeGroups( idWeldGroup *group1, idWeldGroup *group2 );
};

template< class type, int dimension >
idVectorWeld<type,dimension>::idVectorWeld() {
	hash.Clear( idMath::IPow( boxHashSize, dimension ), 128 );
	boxHashSize = 16;
	memset( boxInvSize, 0, dimension * sizeof( boxInvSize[0] ) );
	memset( boxHalfSize, 0, dimension * sizeof( boxHalfSize[0] ) );
}

template< class type, int dimension >
idVectorWeld<type,dimension>::idVectorWeld( const type &mins, const type &maxs, const int boxHashSize, const int initialSize ) {
	Init( mins, maxs, boxHashSize, initialSize );
}

template< class type, int dimension >
void idVectorWeld<type,dimension>::Init( const type &mins, const type &maxs, const int boxHashSize, const int initialSize ) {
	int i;
	float boxSize;

	vectors.AssureSize( initialSize );
	vectors.SetNum( 0, false );
	vectors.SetGranularity( 1024 );

	groups.AssureSize( initialSize );
	groups.SetNum( 0, false );
	groups.SetGranularity( 1024 );

	hash.Clear( idMath::IPow( boxHashSize, dimension ), initialSize );
	hash.SetGranularity( 1024 );

	this->mins = mins;
	this->maxs = maxs;
	this->boxHashSize = boxHashSize;

	for ( i = 0; i < dimension; i++ ) {
		boxSize = ( maxs[i] - mins[i] ) / (float) boxHashSize;
		boxInvSize[i] = 1.0f / boxSize;
		boxHalfSize[i] = boxSize * 0.5f;
	}
}

template< class type, int dimension >
int idVectorWeld<type,dimension>::HashSizeForBounds( const type &mins, const type &maxs, const float epsilon ) const {
	int i;
	float min = idMath::INFINITY;
	for ( i = 0; i < 3; i++ ) {
		if ( maxs[i] - mins[i] < min ) {
			min = maxs[i] - mins[i];
		}
	}
	min /= epsilon * 4;
	for ( i = 0; i < 7; i++ ) {
		if ( ( 1 << i ) > min ) {
			break;
		}
	}
	return 1 << i;
}

template< class type, int dimension >
void idVectorWeld<type,dimension>::SetGranularity( int newGranularity ) {
	vectors.SetGranularity( newGranularity );
	groups.SetGranularity( newGranularity );
	hash.SetGranularity( newGranularity );
}

template< class type, int dimension >
void idVectorWeld<type,dimension>::Clear() {
	vectors.Clear();
	groups.Clear();
	hash.Clear();
}

template< class type, int dimension >
void idVectorWeld<type,dimension>::AddVector( type *v, const float epsilon ) {
	int i, j, k, hashKey, partialHashKey[dimension];

	idWeldVector *vector = weldVectorAllocator.Alloc();
	vector->group = weldGroupAllocator.Alloc();
	vector->group->first = vector;
	vector->v = v;
	vector->next = NULL;

	vectors.Alloc() = vector;
	groups.Alloc() = vector->group;

	// create partial hash key for this vector
	for ( i = 0; i < dimension; i++ ) {
		assert( epsilon <= boxHalfSize[i] );
		partialHashKey[i] = idMath::Ftoi( ( (*vector->v)[i] - mins[i] - boxHalfSize[i] ) * boxInvSize[i] );
	}

	// find nearby vectors and merge their groups into the group of this vector
	for ( i = 0; i < ( 1 << dimension ); i++ ) {

		hashKey = 0;
		for ( j = 0; j < dimension; j++ ) {
			hashKey *= boxHashSize;
			hashKey += partialHashKey[j] + ( ( i >> j ) & 1 );
		}

		for ( j = hash.GetFirst( hashKey ); j != idHashIndex::NULL_INDEX; j = hash.GetNext( j ) ) {
			// if already in the same group
			if ( vectors[j]->group == vector->group ) {
				continue;
			}

			// merge groups if the vertices are close enough to each other
			const type &v1 = *vector->v;
			const type &v2 = *vectors[j]->v;
			for ( k = 0; k < dimension; k++ ) {
				if ( idMath::Fabs( v1[k] - v2[k] ) > epsilon ) {
					break;
				}
			}

			if ( k >= dimension ) {
				MergeGroups( vector->group, vectors[j]->group );
			}
		}
	}

	// add the vector to the hash
	hashKey = 0;
	for ( i = 0; i < dimension; i++ ) {
		hashKey *= boxHashSize;
		hashKey += idMath::Ftoi( ( (*vector->v)[i] - mins[i] ) * boxInvSize[i] );
	}

	hash.Add( hashKey, vectors.Num() - 1 );
}

template< class type, int dimension >
int idVectorWeld<type,dimension>::GetNumUniqueVectors() const {
	return groups.Num();
}

template< class type, int dimension >
void idVectorWeld<type,dimension>::Weld( const float integralEpsilon ) {
	int i, j;
	idWeldVector *vector;
	type mins, maxs, center;

	for ( i = 0; i < groups.Num(); i++ ) {
		if ( groups[i]->first == NULL ) {
			continue;
		}
		for ( j = 0; j < dimension; j++ ) {
			mins[j] = idMath::INFINITY;
			maxs[j] = -idMath::INFINITY;
		}
		for ( vector = groups[i]->first; vector != NULL; vector = vector->next ) {
			const type &v = *vector->v;
			for ( j = 0; j < dimension; j++ ) {
				if ( v[j] < mins[j] ) {
					mins[j] = v[j];
				}
				if ( v[j] > maxs[j] ) {
					maxs[j] = v[j];
				}
			}
		}
		center = ( mins + maxs ) * 0.5f;
		for ( j = 0; j < dimension; j++ ) {
			if ( idMath::Fabs( center[j] - idMath::Rint( center[j] ) ) < integralEpsilon ) {
				center[j] = idMath::Rint( center[j] );
			}
		}
		for ( vector = groups[i]->first; vector != NULL; vector = vector->next ) {
			*vector->v = center;
		}
	}
}

template< class type, int dimension >
void idVectorWeld<type,dimension>::MergeGroups( idWeldGroup *group1, idWeldGroup *group2 ) {
	idWeldVector *v, *next;

	// move all vertices from group2 to group1
	for ( v = group2->first; v != NULL; v = next ) {
		next = v->next;
		v->group = group1;
		v->next = group1->first;
		group1->first = v;
	}

	// remove group2
	group2->first = NULL;
	groups.RemoveFast( group2 );
	weldGroupAllocator.Free( group2 );
}

#endif /* !__VECTORWELD_H__ */
