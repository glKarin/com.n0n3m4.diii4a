/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __QUEUE_H__
#define __QUEUE_H__

/*
===============================================================================

	Queue template

===============================================================================
*/

#define idQueue( type, next )		idQueueTemplate<type, offsetof( type, next )>

template< class type, int nextOffset >
class idQueueTemplate {
public:
							idQueueTemplate( void );

	void					Add( type *element );
	type *				Get( void );
	type *				Peek( void );
	type *				Last( void );

private:
	type *					first;
	type *					last;
};

#define QUEUE_NEXT_PTR( element )		(*((type**)(((byte*)element)+nextOffset)))

template< class type, int nextOffset >
idQueueTemplate<type,nextOffset>::idQueueTemplate( void ) {
	first = last = NULL;
}

template< class type, int nextOffset >
void idQueueTemplate<type,nextOffset>::Add( type *element ) {
	QUEUE_NEXT_PTR(element) = NULL;
	if ( last ) {
		QUEUE_NEXT_PTR(last) = element;
	} else {
		first = element;
	}
	last = element;
}

template< class type, int nextOffset >
type *idQueueTemplate<type,nextOffset>::Get( void ) {
	type *element;

	element = first;
	if ( element ) {
		first = QUEUE_NEXT_PTR(first);
		if ( last == element ) {
			last = NULL;
		}
		QUEUE_NEXT_PTR(element) = NULL;
	}
	return element;
}

template< class type, int nextOffset >
type * idQueueTemplate<type,nextOffset>::Peek( void ) {
	return first;
}

template< class type, int nextOffset >
type * idQueueTemplate<type,nextOffset>::Last( void ) {
	return last;
}

#endif /* !__QUEUE_H__ */
