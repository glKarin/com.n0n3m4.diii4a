// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/

template< typename type >
class idQueueNode {
public:
				idQueueNode( void ) { next = NULL; }

	type *		GetNext( void ) const { return next; }
	void		SetNext( type *next ) { this->next = next; }

private:
	type *		next;
};

template< typename type, idQueueNode<type> type::*nodePtr >
class idQueue {
public:
				idQueue( void );

	void		Add( type *element );
	type *		RemoveFirst( void );

	static void	Test( void );

private:
	type *		first;
	type *		last;
};

template< typename type, idQueueNode<type> type::*nodePtr >
idQueue<type,nodePtr>::idQueue( void ) {
	first = last = NULL;
}

template< typename type, idQueueNode<type> type::*nodePtr >
void idQueue<type,nodePtr>::Add( type *element ) {
	(element->*nodePtr).SetNext( NULL );
	if ( last ) {
		(last->*nodePtr).SetNext( element );
	} else {
		first = element;
	}
	last = element;
}

template< typename type, idQueueNode<type> type::*nodePtr >
type *idQueue<type,nodePtr>::RemoveFirst( void ) {
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

template< typename type, idQueueNode<type> type::*nodePtr >
void idQueue<type,nodePtr>::Test( void ) {

	class idMyType {
	public:
		idQueueNode<idMyType> queueNode;
	};

	idQueue<idMyType,&idMyType::queueNode> myQueue;

	idMyType *element = new idMyType;
	myQueue.Add( element );
	element = myQueue.RemoveFirst();
	delete element;
}

#endif /* !__QUEUE_H__ */
