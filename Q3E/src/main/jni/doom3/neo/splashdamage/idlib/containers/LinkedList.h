// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

/*
===============================================================================

	Linked list template

===============================================================================
*/

template< typename type >
class idListNode {
public:
				idListNode( void ) { next = prev = NULL; }

	type *		GetPrev( void ) const { return prev; }
	type *		GetNext( void ) const { return next; }
	void		SetPrev( type *prev ) { this->prev = prev; }
	void		SetNext( type *next ) { this->next = next; }

private:
	type *		next;
	type *		prev;
};

template< typename type, idListNode<type> type::*nodePtr >
class idLinkedList {
public:
				idLinkedList( void );

	type *		GetFirst( void ) { return first; }
	type *		GetLast( void ) { return last; }
	type *		GetNext( type *element ) { return (element->*nodePtr).GetNext(); }
	type *		GetPrev( type *element ) { return (element->*nodePtr).GetPrev(); }

	void		AddToFront( type *element );
	void		AddToEnd( type *element );
	void		InsertBefore( type *element, type *before );
	void		InsertAfter( type *element, type *after );

	void		Remove( type *element );
	type *		RemoveFirst( void );

	static void	Test( void );

private:
	type *		first;
	type *		last;
};

template< typename type, idListNode<type> type::*nodePtr >
idLinkedList<type,nodePtr>::idLinkedList( void ) {
	first = last = NULL;
}

template< typename type, idListNode<type> type::*nodePtr >
void idLinkedList<type,nodePtr>::AddToFront( type *element ) {
	(element->*nodePtr).SetPrev( NULL );
	(element->*nodePtr).SetNext( first );
	if ( first ) {
		(first->*nodePtr).SetPrev( element );
	}
	first = element;
	if ( last == NULL ) {
		last = element;
	}
}

template< typename type, idListNode<type> type::*nodePtr >
void idLinkedList<type,nodePtr>::AddToEnd( type *element ) {
	(element->*nodePtr).SetPrev( last );
	(element->*nodePtr).SetNext( NULL );
	if ( last ) {
	   (last->*nodePtr).SetNext( element );
	}
	last = element;
	if ( first == NULL ) {
		first = element;
	}
}

template< typename type, idListNode<type> type::*nodePtr >
void idLinkedList<type,nodePtr>::InsertBefore( type *element, type *before ) {
	if ( (before->*nodePtr).GetPrev() != NULL ) {
		((before->*nodePtr).GetPrev()->*nodePtr).SetNext( element );
	} else {
		first = element;
	}
	(element->*nodePtr).SetPrev( (before->*nodePtr).GetPrev() );
	(element->*nodePtr).SetNext( before );
	(before->*nodePtr).SetPrev( element );
}

template< typename type, idListNode<type> type::*nodePtr >
void idLinkedList<type,nodePtr>::InsertAfter( type *element, type *after ) {
	if ( (after->*nodePtr).GetNext() != NULL ) {
		((after->*nodePtr).GetNext()->*nodePtr).SetPrev( element );
	} else {
		last = element;
	}
	(element->*nodePtr).SetPrev( after );
	(element->*nodePtr).SetNext( (after->*nodePtr).GetNext() );
	(after->*nodePtr).SetNext( element );
}

template< typename type, idListNode<type> type::*nodePtr >
void idLinkedList<type,nodePtr>::Remove( type *element ) {
	if ( (element->*nodePtr).GetPrev() != NULL ) {
		((element->*nodePtr).GetPrev()->*nodePtr).SetNext( (element->*nodePtr).GetNext() );
	} else {
		first = (element->*nodePtr).GetNext();
	}
	if ( (element->*nodePtr).GetNext() != NULL ) {
		((element->*nodePtr).GetNext()->*nodePtr).SetPrev( (element->*nodePtr).GetPrev() );
	} else {
		last = (element->*nodePtr).GetPrev();
	}
	(element->*nodePtr).SetPrev( NULL );
	(element->*nodePtr).SetNext( NULL );
}

template< typename type, idListNode<type> type::*nodePtr >
type *idLinkedList<type,nodePtr>::RemoveFirst( void ) {
	if ( first == NULL ) {
		return NULL;
	}
	type *element = first;
	first = (element->*nodePtr).GetNext();
	if ( first != NULL ) {
		(first->*nodePtr).SetPrev( NULL );
	} else {
		last = NULL;
	}
	return element;
}

template< typename type, idListNode<type> type::*nodePtr >
void idLinkedList<type,nodePtr>::Test( void ) {

	class idMyType {
	public:
		idListNode<idMyType> listNode;
	};

	idLinkedList<idMyType,&idMyType::listNode> myList;

	idMyType *element = new idMyType;
	myList.AddToFront( element );
	for ( idMyType *e = myList.GetFirst(); e != NULL; e = myList.GetNext( e ) ) {
	}
	myList.Remove( element );
	delete element;
}

#endif /* !__LINKEDLIST_H__ */
