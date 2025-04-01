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

#ifndef __STACK_H__
#define __STACK_H__

/*
===============================================================================

	Stack template

===============================================================================
*/

#define idStack( type, next )		idStackTemplate<type, offsetof( type, next )>

template< class type, int nextOffset >
class idStackTemplate {
public:
							idStackTemplate( void );

	void					Add( type *element );
	type *				Get( void );
	type *				Peek( void );

private:
	type *					top;
	type *					bottom;
};

#define STACK_NEXT_PTR( element )		(*(type**)(((byte*)element)+nextOffset))

template< class type, int nextOffset >
idStackTemplate<type,nextOffset>::idStackTemplate( void ) {
	top = bottom = NULL;
}

template< class type, int nextOffset >
void idStackTemplate<type,nextOffset>::Add( type *element ) {
	STACK_NEXT_PTR(element) = top;
	top = element;
	if ( !bottom ) {
		bottom = element;
	}
}

template< class type, int nextOffset >
type *idStackTemplate<type,nextOffset>::Get( void ) {
	type *element;

	element = top;
	if ( element ) {
		top = STACK_NEXT_PTR(top);
		if ( bottom == element ) {
			bottom = NULL;
		}
		STACK_NEXT_PTR(element) = NULL;
	}
	return element;
}

template< class type, int nextOffset >
type * idStackTemplate<type,nextOffset>::Peek( void ) {
	return top;
}


#endif /* !__STACK_H__ */
