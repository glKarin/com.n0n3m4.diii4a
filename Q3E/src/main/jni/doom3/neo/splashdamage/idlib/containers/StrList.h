// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __STRLIST_H__
#define __STRLIST_H__

/*
===============================================================================

	idStrList

===============================================================================
*/

class idStrList : public idListGranularityOne<idStr> {
public:
 idStrList( int gran = 1 ) : idListGranularityOne<idStr>::idListGranularityOne( gran ) { }
	void Sort( cmp_t *compare = idListSortCompare<idStr> );
	size_t Size( void ) const;
};

typedef idList<idStr*> idStrPtrList;
typedef idStr *idStrPtr;



/*
============
idSplitStringIntoList
============
*/
ID_INLINE void idSplitStringIntoList( idStrList& list, const char* string, const char* separator = "|" ) {
	int separatorLength = idStr::Length( separator );

	assert( separatorLength > 0 );

	idStr str( string );

	// append a terminator there's no terminating one
	if ( idStr::Icmp( str.Mid( str.Length() - separatorLength, separatorLength ), separator ) != 0 ) {
		str += separator;
	}

	int startIndex = 0;
	int endIndex = str.Find( separator );

	while ( endIndex != -1 && endIndex < str.Length() ) {
		list.Append( str.Mid( startIndex, endIndex - startIndex ));
		startIndex = endIndex + separatorLength;
		endIndex = str.Find( separator, false, startIndex );
	}
}

/*
================
idListSortCompare<idStrPtr>

Compares two pointers to strings. Used to sort a list of string pointers alphabetically in idList<idStr>::Sort.
================
*/
template<>
ID_INLINE int idListSortCompare<idStrPtr>( const idStrPtr *a, const idStrPtr *b ) {
	return ( *a )->Icmp( **b );
}

/*
================
idStrList::Sort

Sorts the list of strings alphabetically. Creates a list of pointers to the actual strings and sorts the
pointer list. Then copies the strings into another list using the ordered list of pointers.
================
*/
ID_INLINE void idStrList::Sort( cmp_t *compare ) {
	int i;

	if ( !num ) {
		return;
	}

	idList<idStr>		other;
	idList<idStrPtr>	pointerList;

	pointerList.SetNum( num );
	for( i = 0; i < num; i++ ) {
		pointerList[ i ] = &( *this )[ i ];
	}

	pointerList.Sort();

	other.SetNum( num );
	other.SetGranularity( granularity );
	for( i = 0; i < other.Num(); i++ ) {
		other[ i ] = *pointerList[ i ];
	}

	this->Swap( other );
}

/*
================
idStrList::Size
================
*/
ID_INLINE size_t idStrList::Size( void ) const {
	size_t s;
	int i;

	s = sizeof( *this );
	for( i = 0; i < Num(); i++ ) {
		s += ( *this )[ i ].Size();
	}

	return s;
}

/*
===============================================================================

	idStrList path sorting

===============================================================================
*/

/*
================
idListSortComparePaths

Compares two pointers to strings. Used to sort a list of string pointers alphabetically in idList<idStr>::Sort.
================
*/
template<class idStrPtr>
ID_INLINE int idListSortComparePaths( const idStrPtr *a, const idStrPtr *b ) {
	return ( *a )->IcmpPath( **b );
}

/*
================
idStrListSortPaths

Sorts the list of path strings alphabetically and makes sure folders come first.
================
*/
ID_INLINE void idStrListSortPaths( idStrList &list ) {
	int i;

	if ( !list.Num() ) {
		return;
	}

	idList<idStr>		other;
	idList<idStrPtr>	pointerList;

	pointerList.SetNum( list.Num() );
	for( i = 0; i < list.Num(); i++ ) {
		pointerList[ i ] = &list[ i ];
	}

	pointerList.Sort( idListSortComparePaths<idStrPtr> );

	other.SetNum( list.Num() );
	other.SetGranularity( list.GetGranularity() );
	for( i = 0; i < other.Num(); i++ ) {
		other[ i ] = *pointerList[ i ];
	}

	list.Swap( other );
}
/*
===============================================================================

	idSimpleStrList

===============================================================================
*/

typedef idList<idSimpleStr> idSimpleStrList;
typedef idList<idSimpleStr*> idSimpleStrPtrList;
typedef idSimpleStr *idSimpleStrPtr;

/*
===============================================================================

	idWStrList

===============================================================================
*/

typedef idList<idWStr> idWStrList;
typedef idList<idWStr*> idWStrPtrList;
typedef idWStr *idWStrPtr;


/*
============
idSplitStringIntoList
============
*/
ID_INLINE void idSplitStringIntoList( idWStrList& list, const wchar_t* string, const wchar_t* separator = L"|" ) {
	int separatorLength = idWStr::Length( separator );

	assert( separatorLength > 0 );

	idWStr str( string );

	// append a terminator there's no terminating one
	if( idWStr::Icmp( str.Mid( str.Length() - separatorLength, separatorLength ).c_str(), separator ) != 0 ) {
		str += separator;
	}

	int startIndex = 0;
	int endIndex = str.Find( separator );

	while( endIndex != -1 && endIndex < str.Length() ) {
		list.Append( str.Mid( startIndex, endIndex - startIndex ));
		startIndex = endIndex + separatorLength;
		endIndex = str.Find( separator, false, startIndex );
	}
}

/*
================
idWStrList::Size
================
*/
template<>
ID_INLINE size_t idWStrList::Size( void ) const {
	size_t s;
	int i;

	s = sizeof( *this );
	for( i = 0; i < Num(); i++ ) {
		s += ( *this )[ i ].Size();
	}

	return s;
}

#endif /* !__STRLIST_H__ */
