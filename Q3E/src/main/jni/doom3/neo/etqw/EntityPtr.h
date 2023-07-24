// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __ENTITY_PTR_H__
#define	__ENTITY_PTR_H__

template< class type >
ID_INLINE idEntityPtr<type>::idEntityPtr( void ) {
	spawnId = 0;
}

template< class type >
ID_INLINE idEntityPtr<type>::idEntityPtr( const type* other ) {
	spawnId = 0;
	*this = other;
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( const type *ent ) {
	if ( ent == NULL ) {
		spawnId = 0;
	} else {
		spawnId = ( gameLocal.spawnIds[ ent->entityNumber ] << GENTITYNUM_BITS ) | ent->entityNumber;
	}
	return *this;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::SetSpawnId( int id ) {
	if ( id == spawnId ) {
		return false;
	}
	if( !id ) {
		spawnId = 0;
		return true;
	}
	int entityNum = id & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	if ( ( ( unsigned int )id >> GENTITYNUM_BITS ) == gameLocal.spawnIds[ entityNum ] && ( gameLocal.entities[ entityNum ] && gameLocal.entities[ entityNum ]->IsType( type::Type ) ) ) {
		spawnId = id;
		return true;
	}
	return false;
}

template< class type >
ID_INLINE void idEntityPtr<type>::ForceSpawnId( int id ) {
	spawnId = id;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsValid( void ) const {
	return ( gameLocal.spawnIds[ spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] == ( ( unsigned int )spawnId >> GENTITYNUM_BITS ) );
}

template< class type >
ID_INLINE type *idEntityPtr<type>::GetEntity( void ) const {
	int entityNum = spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	if ( ( gameLocal.spawnIds[ entityNum ] == ( ( unsigned int )spawnId >> GENTITYNUM_BITS ) ) ) {
		return static_cast<type *>( gameLocal.entities[ entityNum ] );
	}
	return NULL;
}

template< class type >
ID_INLINE int idEntityPtr<type>::GetEntityNum( void ) const {
	return ( spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) );
}

template< class type >
ID_INLINE int idEntityPtr<type>::GetId( void ) const {
	return ( unsigned int )spawnId >> GENTITYNUM_BITS;
}

template< class type >
ID_INLINE type * idEntityPtr<type>::operator->( void ) const {
	return GetEntity ( );
}

template< class type >
ID_INLINE idEntityPtr<type>::operator type * ( void ) const { 
	return GetEntity(); 
}

#endif // __ENTITY_PTR_H__
