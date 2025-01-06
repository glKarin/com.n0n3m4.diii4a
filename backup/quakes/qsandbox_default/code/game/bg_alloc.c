//KK-OAX
/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

This file is part of the Open Arena source code.
Copied from Tremulous under GPL version 2 including any later version.

Open Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Open Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Open Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_local.h"

#define  POOLSIZE ( 1024 * 1024 ) * 256       //QVM_MEMORY note: use 16 for 32bit

#define  FREEMEMCOOKIE  ((int)0xDEADBE3F)  // Any unlikely to be used value
#define  ROUNDBITS    (unsigned int)31          // Round to 32 bytes

typedef struct freeMemNode_s
{
  // Size of ROUNDBITS
  int cookie, size;        // Size includes node (obviously)
  struct freeMemNode_s *prev, *next;
} freeMemNode_t;

static char           memoryPool[POOLSIZE];
static freeMemNode_t  *freeHead;
static int            freeMem;

/*
 * Returns qtrue if BG_Alloc will succeed, qfalse otherwise
 */
qboolean BG_CanAlloc( unsigned int size) {
    freeMemNode_t *fmn;
    int allocsize = ( size + sizeof(int) + ROUNDBITS ) & ~ROUNDBITS;    // Round to 32-byte boundary
    for( fmn = freeHead; fmn; fmn = fmn->next ) {
        if( fmn->cookie != FREEMEMCOOKIE ) {
            //Memory curroption
            return qfalse;
        }
        if( fmn->size >= allocsize ) {
            //At least one useable block
            return qtrue;
        }
    }
    return qfalse;
}

void *BG_Alloc( unsigned int size ) {
  // Find a free block and allocate.
  // Does two passes, attempts to fill same-sized free slot first.

  freeMemNode_t *fmn, *prev, *next, *smallest;
  int allocsize, smallestsize;
  char *endptr;
  int *ptr;
  int usedMem;
  usedMem = POOLSIZE - freeMem;

  allocsize = ( size + sizeof(int) + ROUNDBITS ) & ~ROUNDBITS;    // Round to 32-byte boundary
  ptr = NULL;

  smallest = NULL;
  smallestsize = POOLSIZE + 1;    // Guaranteed not to miss any slots :)
  for( fmn = freeHead; fmn; fmn = fmn->next ) {
    if( fmn->cookie != FREEMEMCOOKIE )
      Com_Error( ERR_DROP, "BG_Alloc: Memory corruption detected!\n" );

    if( fmn->size >= allocsize ) {
      // We've got a block
      if( fmn->size == allocsize ) {
        // Same size, just remove

        prev = fmn->prev;
        next = fmn->next;
        if( prev )
          prev->next = next;      // Point previous node to next
        if( next )
          next->prev = prev;      // Point next node to previous
        if( fmn == freeHead )
          freeHead = next;      // Set head pointer to next
        ptr = (int *) fmn;
        break;              // Stop the loop, this is fine
      } else {
        // Keep track of the smallest free slot
        if( fmn->size < smallestsize ) {
          smallest = fmn;
          smallestsize = fmn->size;
        }
      }
    }
  }

  if( !ptr && smallest ) {
    // We found a slot big enough
    smallest->size -= allocsize;
    endptr = (char *) smallest + smallest->size;
    ptr = (int *) endptr;
  }

  if( ptr ) {
    freeMem -= allocsize;
    memset( ptr, 0, allocsize );
    *ptr++ = allocsize;        // Store a copy of size for deallocation
    return( (void *) ptr );
  }

	G_Printf( "BG_Alloc: failed on allocation of %i bytes\n", usedMem, POOLSIZE );
  return( NULL );
}

void BG_Free( void *ptr ) {
  // Release allocated memory, add it to the free list.

  freeMemNode_t *fmn;
  char *freeend;
  int *freeptr;

  freeptr = ptr;
  freeptr--;

  freeMem += *freeptr;

  for( fmn = freeHead; fmn; fmn = fmn->next ) {
    freeend = ((char *) fmn) + fmn->size;
    if( freeend == (char *) freeptr ) {
      // Released block can be merged to an existing node

      fmn->size += *freeptr;    // Add size of node.
      return;
    }
  }
  // No merging, add to head of list

  fmn = (freeMemNode_t *) freeptr;
  fmn->size = *freeptr;        // Set this first to avoid corrupting *freeptr
  fmn->cookie = FREEMEMCOOKIE;
  fmn->prev = NULL;
  fmn->next = freeHead;
  freeHead->prev = fmn;
  freeHead = fmn;
}

void BG_InitMemory( void ) {
  // Set up the initial node

  freeHead = (freeMemNode_t *)memoryPool;
  freeHead->cookie = FREEMEMCOOKIE;
  freeHead->size = POOLSIZE;
  freeHead->next = NULL;
  freeHead->prev = NULL;
  freeMem = sizeof( memoryPool );
}

void BG_DefragmentMemory( void ) {
  // If there's a frenzy of deallocation and we want to
  // allocate something big, this is useful. Otherwise...
  // not much use.

  freeMemNode_t *startfmn, *endfmn, *fmn;

  for( startfmn = freeHead; startfmn; ) {
    endfmn = (freeMemNode_t *)(((char *) startfmn) + startfmn->size);
    for( fmn = freeHead; fmn; ) {
      if( fmn->cookie != FREEMEMCOOKIE )
        Com_Error( ERR_DROP, "BG_DefragmentMemory: Memory corruption detected!\n" );

      if( fmn == endfmn ) {
        // We can add fmn onto startfmn.

        if( fmn->prev )
          fmn->prev->next = fmn->next;
        if( fmn->next ) {
          if( !(fmn->next->prev = fmn->prev) )
            freeHead = fmn->next;  // We're removing the head node
        }
        startfmn->size += fmn->size;
        memset( fmn, 0, sizeof(freeMemNode_t) );  // A redundant call, really.

        startfmn = freeHead;
        endfmn = fmn = NULL;        // Break out of current loop
      }
      else
        fmn = fmn->next;
    }

    if( endfmn )
      startfmn = startfmn->next;    // endfmn acts as a 'restart' flag here
  }
}

//KK-OAX This was moved from g_mem.c to keep functionality from being broken. 
void Svcmd_GameMem_f( void ) {

    int usedMem;
    usedMem = POOLSIZE - freeMem;
	G_Printf( "Game memory status: %i out of %i bytes allocated\n", usedMem, POOLSIZE );
}

