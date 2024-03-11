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

#include "precompiled.h"
#pragma hdrstop


void LinkedListBubbleSortRaw(void **ppFirst, ptrdiff_t nextOffset, const void *context, bool (*isLess)(const void *, const void *, const void *)) {
	//given a node, returns a pointer to the next node (as lvalue)
	#define NEXT(ptr) ( *(void**) ( ((char*)(ptr)) + nextOffset) )

	//algorithm taken from https://stackoverflow.com/a/19524059/556899

	// p always points to the head of the list
	void *p = *ppFirst;
	*ppFirst = nullptr;

	while ( p ) {
		void **lhs = &p;
		void **rhs = &NEXT(p);
		bool swapped = false;

		// keep going until qq holds the address of a null pointer
		while ( *rhs ) {
			bool needsSwap = isLess(context, *rhs, *lhs);
			// if the left side is greater than the right side
			if ( needsSwap ) {
				// swap linked node ptrs, then swap *back* their next ptrs
				idSwap( *lhs, *rhs );
				idSwap( NEXT(*lhs), NEXT(*rhs) );
				lhs = &NEXT(*lhs);
				swapped = true;
			} else {   // no swap. advance both pointer-pointers
				lhs = rhs;
				rhs = &NEXT(*rhs);
			}
		}

		// link last node to the sorted segment
		*rhs = *ppFirst;

		// if we swapped, detach the final node, terminate the list, and continue.
		if ( swapped ) {
			// take the last node off the list and push it into the result.
			*ppFirst = *lhs;
			*lhs = nullptr;
		}

		// otherwise we're done. since no swaps happened the list is sorted.
		// set the output parameter and terminate the loop.
		else {
			*ppFirst = p;
			break;
		}
	}

	#undef NEXT
}
