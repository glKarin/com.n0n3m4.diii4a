// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __CLIENT_ENTITY_INLINES_H__
#define __CLIENT_ENTITY_INLINES_H__

/*
TTimo: moved to a seperate header to they can reference gameLocal which has not been defined at time of ClientEntity.h inclusion
 */

template< class type >
ID_INLINE rvClientEntityPtr<type>::rvClientEntityPtr() {
	spawnId = 0;
}

template< class type >
ID_INLINE rvClientEntityPtr<type> &rvClientEntityPtr<type>::operator=( type *cent ) {
	if ( cent == NULL ) {
		spawnId = 0;
	} else {
		spawnId = ( gameLocal.clientSpawnIds[cent->entityNumber] << CENTITYNUM_BITS ) | cent->entityNumber;
	}
	return *this;
}

template< class type >
ID_INLINE void rvClientEntityPtr<type>::SetEntity( const type *cent ) {
	if ( cent == NULL ) {
		spawnId = 0;
	} else {
		spawnId = ( gameLocal.clientSpawnIds[cent->entityNumber] << CENTITYNUM_BITS ) | cent->entityNumber;
	}
}

template< class type >
ID_INLINE bool rvClientEntityPtr<type>::SetSpawnId( int id ) {
	if ( id == spawnId ) {
		return false;
	}
	if ( ( ( unsigned int )id >> CENTITYNUM_BITS ) == gameLocal.clientSpawnIds[ id & ( ( 1 << CENTITYNUM_BITS ) - 1 ) ] ) {
		spawnId = id;
		return true;
	}
	return false;
}

template< class type >
ID_INLINE bool rvClientEntityPtr<type>::IsValid( void ) const {
	return ( gameLocal.clientSpawnIds[ spawnId & ( ( 1 << CENTITYNUM_BITS ) - 1 ) ] == ( ( unsigned int )spawnId >> CENTITYNUM_BITS ) );
}

template< class type >
ID_INLINE type *rvClientEntityPtr<type>::GetEntity( void ) const {
	int entityNum = spawnId & ( ( 1 << CENTITYNUM_BITS ) - 1 );
	if ( ( gameLocal.clientSpawnIds[ entityNum ] == ( ( unsigned int )spawnId >> CENTITYNUM_BITS ) ) ) {
		return static_cast<type *>( gameLocal.clientEntities[ entityNum ] );
	}
	return NULL;
}

template< class type >
ID_INLINE int rvClientEntityPtr<type>::GetEntityNum( void ) const {
	return ( spawnId & ( ( 1 << CENTITYNUM_BITS ) - 1 ) );
}

#endif
