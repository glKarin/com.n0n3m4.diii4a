// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __STACK_H__
#define __STACK_H__

/*
===============================================================================

	Stack template

===============================================================================
*/


template< class T >
class sdStack {
public:
	T&				Push();
	T&				Push( const T& element );
	void			Pop();
	T&				Top();
	const T&		Top() const;

	void			Clear();
	void			DeleteContents( bool clear );
	void			SetGranularity( int newGranularity );
	bool			Empty() const;

	int				Num() const;
	void			Swap( sdStack& rhs );

private:
	idList< T > stack;
};

/*
============
sdStack< T >::Push
============
*/
template< class T >
ID_INLINE T& sdStack< T >::Push() {
	return stack.Alloc();
}

/*
============
sdStack< T >::Push
============
*/
template< class T >
ID_INLINE T& sdStack< T >::Push( const T& element ) {
	T& ref = stack.Alloc();
	ref = element;
	return ref;
}

/*
============
sdStack< T >::Pop
============
*/
template< class T >
ID_INLINE void sdStack< T >::Pop() {
	stack.RemoveIndex( Num() - 1 );
}

/*
============
sdStack< T >::Top
============
*/
template< class T >
ID_INLINE T& sdStack< T >::Top() {
	return stack[ Num() - 1 ];
}

/*
============
sdStack< T >::Top
============
*/
template< class T >
ID_INLINE const T& sdStack< T >::Top() const {
	return stack[ Num() - 1 ];
}

/*
============
sdStack< T >::Clear
============
*/
template< class T >
ID_INLINE void sdStack< T >::Clear() {
	stack.Clear();
}

/*
============
sdStack< T >::DeleteContents
============
*/
template< class T >
ID_INLINE void sdStack< T >::DeleteContents( bool clear ) {
	stack.DeleteContents( clear );
}

/*
============
sdStack< T >::SetGranularity
============
*/
template< class T >
ID_INLINE void sdStack< T >::SetGranularity( int newGranularity ) {
	stack.SetGranularity( newGranularity );
}

/*
============
sdStack< T >::Empty
============
*/
template< class T >
ID_INLINE bool sdStack< T >::Empty() const {
	return stack.Num() == 0;
}

/*
============
sdStack< T >::Num
============
*/
template< class T >
ID_INLINE int sdStack< T >::Num() const {
	return stack.Num();
}

/*
============
sdStack< T >::Num
============
*/
template< class T >
ID_INLINE void sdStack< T >::Swap( sdStack& rhs ) {
	stack.Swap( rhs.stack );
}

/* jrad - we need to use a more traditional stack
template< typename type >
class idStackNode {
public:
				idStackNode( void ) { next = NULL; }

	type *		GetNext( void ) const { return next; }
	void		SetNext( type *next ) { this->next = next; }

private:
	type *		next;
};

template< typename type, idStackNode<type> type::*nodePtr >
class idStack {
public:
				idStack( void );

	void		Add( type *element );
	type *		RemoveFirst( void );

	static void	Test( void );

private:
	type *		first;
	type *		last;
};

template< typename type, idStackNode<type> type::*nodePtr >
idStack<type,nodePtr>::idStack( void ) {
	first = last = NULL;
}

template< typename type, idStackNode<type> type::*nodePtr >
void idStack<type,nodePtr>::Add( type *element ) {
	(element->*nodePtr).SetNext( first );
	first = element;
	if ( last == NULL ) {
		last = element;
	}
}

template< typename type, idStackNode<type> type::*nodePtr >
type *idStack<type,nodePtr>::RemoveFirst( void ) {
	type *element;

	element = first;
	if ( element ) {
		first = (first->*nodePtr).GetNext();
		if ( last == element ) {
			last = NULL;
		}
		(element->*nodePtr).SetNext( NULL );
	}
	return element;
}

template< typename type, idStackNode<type> type::*nodePtr >
void idStack<type,nodePtr>::Test( void ) {

	class idMyType {
	public:
		idStackNode<idMyType> stackNode;
	};

	idStack<idMyType,&idMyType::stackNode> myStack;

	idMyType *element = new idMyType;
	myStack.Add( element );
	element = myStack.RemoveFirst();
	delete element;
}
*/

#endif /* !__STACK_H__ */
