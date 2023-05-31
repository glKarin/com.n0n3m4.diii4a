
#ifndef __LIST_GAME_H__
#define __LIST_GAME_H__

// GCC 4 compiles templates when they are encountered in a source file,
// not when they are used. Therefore all references, must be resolved.
// RemoveContents() from idLib\List.h references global variables only
// available in the GAME DLL.

/*
================
idList<type>::RemoveContents
================
*/
template< class type >
ID_INLINE void idList<type>::RemoveContents( bool clear ) {
	RemoveNull();

	for( int ix = Num() - 1; ix >= 0; --ix ) {
		list[ ix ]->PostEventMS( &EV_Remove, 0 );
		list[ ix ] = NULL;
	}

	if ( clear ) {
		Clear();
	} else {
		memset( list, 0, Allocated() );
	}
}

/*
================
idList<type>::RemoveNull
================
*/
template< class type >
ID_INLINE void idList<type>::RemoveNull() {
        for( int ix = Num() - 1; ix >= 0; --ix ) {
                if( !list[ix] ) {
                        RemoveIndex( ix );
                }
        }
}

#endif // __LIST_GAME_H__
