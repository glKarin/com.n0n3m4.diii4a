// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __IDLIB_HANDLES_H__
#define __IDLIB_HANDLES_H__

#include "../Handle.h"

/*
============
sdHandles
class to manage a set of handles to items
============
*/
template< class T >
class sdHandles {
public:
	typedef sdUtility::sdHandle< int, -1 > handle_t;

	handle_t			Acquire();
	void				Release( handle_t& handle );
	T&					operator[]( const handle_t& handle );
	const T&			operator[]( const handle_t& handle ) const;

	handle_t			GetFirst() const;
	handle_t			GetNext( const handle_t& handle ) const; 

	void				SetGranularity( int newGranularity );

						// returns the total number of slots available, use IsValid to make sure an element is actually usable
	int					Num() const;
	bool				IsValid( const handle_t& handle ) const;
					
						// all handles will be invalidated
	void				DeleteContents();

	void				Swap( sdHandles& rhs );

private:
	idList< T >			items;
};

/*
============
sdHandles< T >::Acquire
============
*/
template< class T >
typename sdHandles< T >::handle_t sdHandles< T >::Acquire() {
	for( int i = 0; i < items.Num(); i++ ) {
		if( items[ i ] == NULL ) {
			return handle_t( i );
		}
	}	
	return handle_t( items.Append( T() )); 
}

/*
============
sdHandles< T >::Release
============
*/
template< class T >
void sdHandles< T >::Release( handle_t& handle ) {
	assert( handle.IsValid() );
	items[ handle ] = NULL;
	handle.Release();
}

/*
============
sdHandles< T >::operator[]
============
*/
template< class T >
T& sdHandles< T >::operator[]( const handle_t& handle ) {
	assert( handle.IsValid() );
	return items[ handle ];
}

/*
============
sdHandles< T >::operator[]
============
*/
template< class T >
const T& sdHandles< T >::operator[]( const handle_t& handle ) const {
	assert( handle.IsValid() );
	return items[ handle ];	
}

/*
============
sdHandles< T >::SetGranularity
============
*/
template< class T >
void sdHandles< T >::SetGranularity( int newGranularity ) {
	items.SetGranularity( newGranularity );
}

/*
============
sdHandles< T >::Num
============
*/
template< class T >
int sdHandles< T >::Num() const {
	return items.Num();
}

/*
============
sdHandles< T >::IsValid
============
*/
template< class T >
bool sdHandles< T >::IsValid( const handle_t& handle ) const {
	if( !handle.IsValid() ) {
		return false;
	}
	return items[ handle ] != NULL;
}

/*
============
sdHandles< T >::DeleteContents
============
*/
template< class T >
void sdHandles< T >::DeleteContents() {
	items.DeleteContents( true );
}

/*
============
sdHandles< T >::GetFirst
============
*/
template< class T >
typename sdHandles< T >::handle_t sdHandles< T >::GetFirst() const {
	for( int i = 0; i < items.Num(); i++ ) {
		if( IsValid( i ) ) {
			return handle_t( i );
		}
	}
	return handle_t();
}

/*
============
sdHandles< T >::GetNext
============
*/
template< class T >
typename sdHandles< T >::handle_t sdHandles< T >::GetNext( const handle_t& handle ) const {
	if( !handle.IsValid() || handle >= items.Num() ) {
		return handle_t();
	}

	for( int i = handle + 1; i < items.Num(); i++ ) {
		if( IsValid( i ) ) {
			return handle_t( i );
		}
	}
	return handle_t();
}

/*
============
sdHandles< T >::Swap
============
*/
template< class T >
void sdHandles< T >::Swap( sdHandles& rhs ) {
	items.Swap( rhs.items );
}


#endif // ! __IDLIB_HANDLES_H__
