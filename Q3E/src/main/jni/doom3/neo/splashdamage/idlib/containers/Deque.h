// Copyright (C) 2007 Id Software, Inc.
//



/*
============
sdDeque
============
*/
template< class T >
class sdDeque {
public:
	void	PushBack( const T& element );
	void	PopBack();
	T&		Back();
	
	T&		Front();
	void	PushFront( const T& element );
	void	PopFront();

	void	Clear();
	void	DeleteContents();
	void	SetGranularity( int newGranularity );
	bool	Empty() const;

	int		Num() const;

	void	Swap( sdDeque& rhs );

private:
	idList< T > list;
};

/*
============
sdDeque< T >::PushBack
============
*/
template< class T >
ID_INLINE void sdDeque< T >::PushBack( const T& element ) {
	list.Append( element );
}

/*
============
sdDeque< T >::PopBack
============
*/
template< class T >
ID_INLINE void sdDeque< T >::PopBack() {
	list.RemoveIndex( Num() - 1 );
}

/*
============
sdDeque< T >::Back
============
*/
template< class T >
ID_INLINE T& sdDeque< T >::Back() {
	return list[ Num() - 1 ];
}

/*
============
sdDeque< T >::PushFront
============
*/
template< class T >
ID_INLINE void sdDeque< T >::PushFront( const T& element ) {
	list.Insert( element, 0 );
}

/*
============
sdDeque< T >::PopFront
============
*/
template< class T >
ID_INLINE void sdDeque< T >::PopFront() {
	list.RemoveIndex( 0 );
}

/*
============
sdDeque< T >::Front
============
*/
template< class T >
ID_INLINE T& sdDeque< T >::Front() {
	return list[ 0 ];
}

/*
============
sdDeque< T >::Clear
============
*/
template< class T >
ID_INLINE void sdDeque< T >::Clear() {
	list.Clear();
}

/*
============
sdDeque< T >::DeleteContents
============
*/
template< class T >
ID_INLINE void sdDeque< T >::DeleteContents() {
	list.DeleteContents( true );
}

/*
============
sdDeque< T >::SetGranularity
============
*/
template< class T >
ID_INLINE void sdDeque< T >::SetGranularity( int newGranularity ) {
	list.SetGranularity( newGranularity );
}

/*
============
sdDeque< T >::Empty
============
*/
template< class T >
ID_INLINE bool sdDeque< T >::Empty() const {
	return list.Num() == 0;
}

/*
============
sdDeque< T >::Num
============
*/
template< class T >
ID_INLINE int sdDeque< T >::Num() const {
	return list.Num();
}
/*
============
sdDeque< T >::Swap
============
*/
template< class T >
void sdDeque< T >::Swap( sdDeque& rhs ) {
	list.Swap( rhs.list );
}
