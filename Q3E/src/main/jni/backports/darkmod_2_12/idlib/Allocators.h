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

#ifndef __ALLOCATORS_H__
#define __ALLOCATORS_H__

#include "Heap.h"

template<class T> ID_INLINE T	HMax( T x, T y ) { return ( x > y ) ? x : y; }

/*
===============================================================================

	Block based allocator for fixed size objects.

	All objects of the 'type' are properly constructed.
	However, the constructor is not called for re-used objects.

===============================================================================
*/

template<class type, int blockSize>
class idBlockAlloc {
public:
							idBlockAlloc( void );
							~idBlockAlloc( void );

	void					Shutdown( void );

	type *					Alloc( void );
	void					Free( type *element );

	int						GetTotalCount( void ) const { return total; }
	int						GetAllocCount( void ) const { return active; }
	int						GetFreeCount( void ) const { return total - active; }

private:
	#pragma pack(push, 1)
	typedef struct element_s {
		union {
			struct element_s *	next;
			uint64_t prefix;
		};
		type				t;
	} element_t;
	#pragma pack(pop)
	typedef struct block_s {
		element_t			elements[blockSize];
		struct block_s *	next;
	} block_t;

	block_t *				blocks;
	element_t *				free;
	int						total;
	int						active;
};

template<class type, int blockSize>
idBlockAlloc<type,blockSize>::idBlockAlloc( void ) {
	blocks = NULL;
	free = NULL;
	total = active = 0;
}

template<class type, int blockSize>
idBlockAlloc<type,blockSize>::~idBlockAlloc( void ) {
	Shutdown();
}

template<class type, int blockSize>
type *idBlockAlloc<type,blockSize>::Alloc( void ) {
	if ( !free ) {
		block_t *block = new block_t;
		block->next = blocks;
		blocks = block;
		for ( int i = 0; i < blockSize; i++ ) {
			block->elements[i].next = free;
			free = &block->elements[i];
		}
		total += blockSize;
	}
	active++;
	element_t *element = free;
	free = free->next;
	element->next = NULL;
	return &element->t;
}

template<class type, int blockSize>
void idBlockAlloc<type,blockSize>::Free( type *t ) {
	// old code 
	// element_t *element = (element_t *)( ( (unsigned char *) t ) - ( (int) &((element_t *)0)->t ) );

	// greebo: Use the standard offsetof macro as defined in <cstddef>
	// element_t *element = reinterpret_cast<element_t*>(intptr_t(t) - offsetof(element_t, t));
	// unfortunately, offsetof with non std-layout "type" produces major GCC warning

	// stgatilov: with pragma pack, we are sure that the first member "prefix" occupies exactly 8 bytes
	// however, we should assert that payload has no overaligned types (like __m128 from SSE), otherwise their alignment would be broken
	element_t *element = (element_t *)( uintptr_t(t) - sizeof(element_t::prefix) );
	assert( uintptr_t( &element->t ) == uintptr_t(t) );
	static_assert( alignof(type) <= 8, "overaligned type in idBlockAlloc" );
#ifdef _MSC_VER	// can't easily disable GCC warning about offsetof here
	static_assert( offsetof(element_t, t) == sizeof(element_t::prefix), "idBlockAlloc offset issue" );
#endif

	element->next = free;
	free = element;
	active--;
}

template<class type, int blockSize>
void idBlockAlloc<type,blockSize>::Shutdown( void ) {
	while( blocks ) {
		block_t *block = blocks;
		blocks = blocks->next;
		delete block;
	}
	blocks = NULL;
	free = NULL;
	total = active = 0;
}

/*
==============================================================================

	Dynamic allocator, simple wrapper for normal allocations which can
	be interchanged with idDynamicBlockAlloc.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

template<class type, int baseBlockSize, int minBlockSize>
class idDynamicAlloc {
public:
									idDynamicAlloc( void );
									~idDynamicAlloc( void );

	void							Init( void );
	void							Shutdown( void );
	void							SetFixedBlocks( int numBlocks ) {}
	void							SetLockMemory( bool lock ) {}
	void							FreeEmptyBaseBlocks( void ) {}

	type *							Alloc( const int num );
	type *							Resize( type *ptr, const int num );
	void							Free( type *ptr );
	const char *					CheckMemory( const type *ptr ) const;

	int								GetNumBaseBlocks( void ) const { return 0; }
	int								GetBaseBlockMemory( void ) const { return 0; }
	int								GetNumUsedBlocks( void ) const { return numUsedBlocks; }
	int								GetUsedBlockMemory( void ) const { return usedBlockMemory; }
	int								GetNumFreeBlocks( void ) const { return 0; }
	int								GetFreeBlockMemory( void ) const { return 0; }
	int								GetNumEmptyBaseBlocks( void ) const { return 0; }

private:
	int								numUsedBlocks;			// number of used blocks
	int								usedBlockMemory;		// total memory in used blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear( void );
};

template<class type, int baseBlockSize, int minBlockSize>
idDynamicAlloc<type, baseBlockSize, minBlockSize>::idDynamicAlloc( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicAlloc<type, baseBlockSize, minBlockSize>::~idDynamicAlloc( void ) {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Init( void ) {
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Shutdown( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
type *idDynamicAlloc<type, baseBlockSize, minBlockSize>::Alloc( const int num ) {
	numAllocs++;
	if ( num <= 0 ) {
		return NULL;
	}
	numUsedBlocks++;
	usedBlockMemory += num * sizeof( type );
	return (type*)Mem_Alloc16( num * sizeof( type ) );
}

template<class type, int baseBlockSize, int minBlockSize>
type *idDynamicAlloc<type, baseBlockSize, minBlockSize>::Resize( type *ptr, const int num ) {

	numResizes++;

	if ( ptr == NULL ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( ptr );
		return NULL;
	}

	assert( 0 );
	return ptr;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Free( type *ptr ) {
	numFrees++;
	if ( ptr == NULL ) {
		return;
	}
	Mem_Free16( ptr );
}

template<class type, int baseBlockSize, int minBlockSize>
const char *idDynamicAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( const type *ptr ) const {
	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicAlloc<type, baseBlockSize, minBlockSize>::Clear( void ) {
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;
}


/*
==============================================================================

	Fast dynamic block allocator.

	No constructor is called for the 'type'.
	Allocated blocks are always 16 byte aligned.

==============================================================================
*/

#include "containers/BTree.h"

//#define DYNAMIC_BLOCK_ALLOC_CHECK

template<class type>
class idDynamicBlock {
public:
	type *							GetMemory( void ) const { return (type *)( ( (byte *) this ) + sizeof( idDynamicBlock<type> ) ); }
	int								GetSize( void ) const { return abs( size ); }
	void							SetSize( int s, bool isBaseBlock ) { size = isBaseBlock ? -s : s; }
	bool							IsBaseBlock( void ) const { return ( size < 0 ); }

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								id[3];
	void *							allocator;
#endif

	int								size;					// size in bytes of the block
	idDynamicBlock<type> *			prev;					// previous memory block
	idDynamicBlock<type> *			next;					// next memory block
	idBTreeNode<idDynamicBlock<type>,int> *node;			// node in the B-Tree with free blocks
};

template<class type, int baseBlockSize, int minBlockSize>
class idDynamicBlockAlloc {
public:
									idDynamicBlockAlloc( void );
									~idDynamicBlockAlloc( void );

	void							Init( void );
	void							Shutdown( void );
	void							SetFixedBlocks( int numBlocks );
	void							SetLockMemory( bool lock );
	void							FreeEmptyBaseBlocks( void );

	type *							Alloc( const int num );
	type *							Resize( type *ptr, const int num );
	void							Free( type *ptr );
	const char *					CheckMemory( const type *ptr ) const;

	int								GetNumBaseBlocks( void ) const { return numBaseBlocks; }
	int								GetBaseBlockMemory( void ) const { return baseBlockMemory; }
	int								GetNumUsedBlocks( void ) const { return numUsedBlocks; }
	int								GetUsedBlockMemory( void ) const { return usedBlockMemory; }
	int								GetNumFreeBlocks( void ) const { return numFreeBlocks; }
	int								GetFreeBlockMemory( void ) const { return freeBlockMemory; }
	int								GetNumEmptyBaseBlocks( void ) const;

private:
	idDynamicBlock<type> *			firstBlock;				// first block in list in order of increasing address
	idDynamicBlock<type> *			lastBlock;				// last block in list in order of increasing address
	idBTree<idDynamicBlock<type>,int,4>freeTree;			// B-Tree with free memory blocks
	bool							allowAllocs;			// allow base block allocations
	bool							lockMemory;				// lock memory so it cannot get swapped out

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								blockId[3];
#endif

	int								numBaseBlocks;			// number of base blocks
	int								baseBlockMemory;		// total memory in base blocks
	int								numUsedBlocks;			// number of used blocks
	int								usedBlockMemory;		// total memory in used blocks
	int								numFreeBlocks;			// number of free blocks
	int								freeBlockMemory;		// total memory in free blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear( void );
	idDynamicBlock<type> *			AllocInternal( const int num );
	idDynamicBlock<type> *			ResizeInternal( idDynamicBlock<type> *block, const int num );
	void							FreeInternal( idDynamicBlock<type> *block );
	void							LinkFreeInternal( idDynamicBlock<type> *block );
	void							UnlinkFreeInternal( idDynamicBlock<type> *block );
	void							CheckMemory( void ) const;
};

template<class type, int baseBlockSize, int minBlockSize>
idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::idDynamicBlockAlloc( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::~idDynamicBlockAlloc( void ) {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Init( void ) {
	freeTree.Init();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Shutdown( void ) {
	idDynamicBlock<type> *block;

	for ( block = firstBlock; block != NULL; block = block->next ) {
		if ( block->node == NULL ) {
			FreeInternal( block );
		}
	}

	for ( block = firstBlock; block != NULL; block = firstBlock ) {
		firstBlock = block->next;
		assert( block->IsBaseBlock() );
		if ( lockMemory ) {
			idLib::sys->UnlockMemory( block, block->GetSize() + (int)sizeof( idDynamicBlock<type> ) );
		}
		Mem_Free16( block );
	}

	freeTree.Shutdown();

	Clear();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::SetFixedBlocks( int numBlocks ) {
	idDynamicBlock<type> *block;

	for ( int i = numBaseBlocks; i < numBlocks; i++ ) {
		block = ( idDynamicBlock<type> * ) Mem_Alloc16( baseBlockSize );
		if ( lockMemory ) {
			idLib::sys->LockMemory( block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = (void*)this;
#endif
		block->SetSize( baseBlockSize - (int)sizeof( idDynamicBlock<type> ), true );
		block->next = NULL;
		block->prev = lastBlock;
		if ( lastBlock ) {
			lastBlock->next = block;
		} else {
			firstBlock = block;
		}
		lastBlock = block;
		block->node = NULL;

		FreeInternal( block );

		numBaseBlocks++;
		baseBlockMemory += baseBlockSize;
	}

	allowAllocs = false;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::SetLockMemory( bool lock ) {
	lockMemory = lock;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::FreeEmptyBaseBlocks( void ) {
	idDynamicBlock<type> *block, *next;

	for ( block = firstBlock; block != NULL; block = next ) {
		next = block->next;

		if ( block->IsBaseBlock() && block->node != NULL && ( next == NULL || next->IsBaseBlock() ) ) {
			UnlinkFreeInternal( block );
			if ( block->prev ) {
				block->prev->next = block->next;
			} else {
				firstBlock = block->next;
			}
			if ( block->next ) {
				block->next->prev = block->prev;
			} else {
				lastBlock = block->prev;
			}
			if ( lockMemory ) {
				idLib::sys->UnlockMemory( block, block->GetSize() + (int)sizeof( idDynamicBlock<type> ) );
			}
			numBaseBlocks--;
			baseBlockMemory -= block->GetSize() + (int)sizeof( idDynamicBlock<type> );
			Mem_Free16( block );
		}
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
int idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::GetNumEmptyBaseBlocks( void ) const {
	int numEmptyBaseBlocks;
	idDynamicBlock<type> *block;

	numEmptyBaseBlocks = 0;
	for ( block = firstBlock; block != NULL; block = block->next ) {
		if ( block->IsBaseBlock() && block->node != NULL && ( block->next == NULL || block->next->IsBaseBlock() ) ) {
			numEmptyBaseBlocks++;
		}
	}
	return numEmptyBaseBlocks;
}

template<class type, int baseBlockSize, int minBlockSize>
type *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Alloc( const int num ) {
	idDynamicBlock<type> *block;

	numAllocs++;

	if ( num <= 0 ) {
		return NULL;
	}

	block = AllocInternal( num );
	if ( block == NULL ) {
		return NULL;
	}
	block = ResizeInternal( block, num );
	if ( block == NULL ) {
		return NULL;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	numUsedBlocks++;
	usedBlockMemory += block->GetSize();

	return block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize>
type *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Resize( type *ptr, const int num ) {

	numResizes++;

	if ( ptr == NULL ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( ptr );
		return NULL;
	}

	idDynamicBlock<type> *block = ( idDynamicBlock<type> * ) ( ( (byte *) ptr ) - (int)sizeof( idDynamicBlock<type> ) );

	usedBlockMemory -= block->GetSize();

	block = ResizeInternal( block, num );
	if ( block == NULL ) {
		return NULL;
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

	usedBlockMemory += block->GetSize();

	return block->GetMemory();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Free( type *ptr ) {

	numFrees++;

    // temp fix for linux segfault on exit - underlying cause still undetermined
    if ( ptr == NULL || numUsedBlocks == 0 ) {
		return;
	}

	idDynamicBlock<type> *block = ( idDynamicBlock<type> * ) ( ( (byte *) ptr ) - (int)sizeof( idDynamicBlock<type> ) );

	numUsedBlocks--;
	usedBlockMemory -= block->GetSize();

	FreeInternal( block );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
const char *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( const type *ptr ) const {
	idDynamicBlock<type> *block;

	if ( ptr == NULL ) {
		return NULL;
	}

	block = ( idDynamicBlock<type> * ) ( ( (byte *) ptr ) - (int)sizeof( idDynamicBlock<type> ) );

	if ( block->node != NULL ) {
		return "memory has been freed";
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	if ( block->id[0] != 0x11111111 || block->id[1] != 0x22222222 || block->id[2] != 0x33333333 ) {
		return "memory has invalid id";
	}
	if ( block->allocator != (void*)this ) {
		return "memory was allocated with different allocator";
	}
#endif

	/* base blocks can be larger than baseBlockSize which can cause this code to fail
	idDynamicBlock<type> *base;
	for ( base = firstBlock; base != NULL; base = base->next ) {
		if ( base->IsBaseBlock() ) {
			if ( ((int)block) >= ((int)base) && ((int)block) < ((int)base) + baseBlockSize ) {
				break;
			}
		}
	}
	if ( base == NULL ) {
		return "no base block found for memory";
	}
	*/

	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::Clear( void ) {
	firstBlock = lastBlock = NULL;
	allowAllocs = true;
	lockMemory = false;
	numBaseBlocks = 0;
	baseBlockMemory = 0;
	numUsedBlocks = 0;
	usedBlockMemory = 0;
	numFreeBlocks = 0;
	freeBlockMemory = 0;
	numAllocs = 0;
	numResizes = 0;
	numFrees = 0;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	blockId[0] = 0x11111111;
	blockId[1] = 0x22222222;
	blockId[2] = 0x33333333;
#endif
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicBlock<type> *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::AllocInternal( const int num ) {
	idDynamicBlock<type> *block;
	int alignedBytes = ( num * sizeof( type ) + 15 ) & ~15;

	block = freeTree.FindSmallestLargerEqual( alignedBytes );
	if ( block != NULL ) {
		UnlinkFreeInternal( block );
	} else if ( allowAllocs ) {
		int allocSize = HMax( baseBlockSize, alignedBytes + (int)sizeof( idDynamicBlock<type> ) );
		block = ( idDynamicBlock<type> * ) Mem_Alloc16( allocSize );
		if ( lockMemory ) {
			idLib::sys->LockMemory( block, baseBlockSize );
		}
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = (void*)this;
#endif
		block->SetSize( allocSize - (int)sizeof( idDynamicBlock<type> ), true );
		block->next = NULL;
		block->prev = lastBlock;
		if ( lastBlock ) {
			lastBlock->next = block;
		} else {
			firstBlock = block;
		}
		lastBlock = block;
		block->node = NULL;

		numBaseBlocks++;
		baseBlockMemory += allocSize;
	}

	return block;
}

template<class type, int baseBlockSize, int minBlockSize>
idDynamicBlock<type> *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::ResizeInternal( idDynamicBlock<type> *block, const int num ) {
	int alignedBytes = ( num * sizeof( type ) + 15 ) & ~15;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == (void*)this );
#endif

	// if the new size is larger
	if ( alignedBytes > block->GetSize() ) {

		idDynamicBlock<type> *nextBlock = block->next;

		// try to annexate the next block if it's free
		if ( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != NULL &&
				block->GetSize() + (int)sizeof( idDynamicBlock<type> ) + nextBlock->GetSize() >= alignedBytes ) {

			UnlinkFreeInternal( nextBlock );
			block->SetSize( block->GetSize() + (int)sizeof( idDynamicBlock<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
			block->next = nextBlock->next;
			if ( nextBlock->next ) {
				nextBlock->next->prev = block;
			} else {
				lastBlock = block;
			}
		} else {
			// allocate a new block and copy
			idDynamicBlock<type> *oldBlock = block;
			block = AllocInternal( num );
			if ( block == NULL ) {
				return NULL;
			}
			memcpy( block->GetMemory(), oldBlock->GetMemory(), oldBlock->GetSize() );
			FreeInternal( oldBlock );
		}
	}

	// if the unused space at the end of this block is large enough to hold a block with at least one element
	if ( block->GetSize() - alignedBytes - (int)sizeof( idDynamicBlock<type> ) < HMax( minBlockSize, (int)sizeof( type ) ) ) {
		return block;
	}

	idDynamicBlock<type> *newBlock;

	newBlock = ( idDynamicBlock<type> * ) ( ( (byte *) block ) + (int)sizeof( idDynamicBlock<type> ) + alignedBytes );
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	memcpy( newBlock->id, blockId, sizeof( newBlock->id ) );
	newBlock->allocator = (void*)this;
#endif
	newBlock->SetSize( block->GetSize() - alignedBytes - (int)sizeof( idDynamicBlock<type> ), false );
	newBlock->next = block->next;
	newBlock->prev = block;
	if ( newBlock->next ) {
		newBlock->next->prev = newBlock;
	} else {
		lastBlock = newBlock;
	}
	newBlock->node = NULL;
	block->next = newBlock;
	block->SetSize( alignedBytes, block->IsBaseBlock() );

	FreeInternal( newBlock );

	return block;
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::FreeInternal( idDynamicBlock<type> *block ) {

	assert( block->node == NULL );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == (void*)this );
#endif

	// try to merge with a next free block
	idDynamicBlock<type> *nextBlock = block->next;
	if ( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != NULL ) {
		UnlinkFreeInternal( nextBlock );
		block->SetSize( block->GetSize() + (int)sizeof( idDynamicBlock<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
		block->next = nextBlock->next;
		if ( nextBlock->next ) {
			nextBlock->next->prev = block;
		} else {
			lastBlock = block;
		}
	}

	// try to merge with a previous free block
	idDynamicBlock<type> *prevBlock = block->prev;
	if ( prevBlock && !block->IsBaseBlock() && prevBlock->node != NULL ) {
		UnlinkFreeInternal( prevBlock );
		prevBlock->SetSize( prevBlock->GetSize() + (int)sizeof( idDynamicBlock<type> ) + block->GetSize(), prevBlock->IsBaseBlock() );
		prevBlock->next = block->next;
		if ( block->next ) {
			block->next->prev = prevBlock;
		} else {
			lastBlock = prevBlock;
		}
		LinkFreeInternal( prevBlock );
	} else {
		LinkFreeInternal( block );
	}
}

template<class type, int baseBlockSize, int minBlockSize>
ID_INLINE void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::LinkFreeInternal( idDynamicBlock<type> *block ) {
	block->node = freeTree.Add( block, block->GetSize() );
	numFreeBlocks++;
	freeBlockMemory += block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize>
ID_INLINE void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::UnlinkFreeInternal( idDynamicBlock<type> *block ) {
	freeTree.Remove( block->node );
	block->node = NULL;
	numFreeBlocks--;
	freeBlockMemory -= block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize>::CheckMemory( void ) const {
	idDynamicBlock<type> *block;

	for ( block = firstBlock; block != NULL; block = block->next ) {
		// make sure the block is properly linked
		if ( block->prev == NULL ) {
			assert( firstBlock == block );
		} else {
			assert( block->prev->next == block );
		}
		if ( block->next == NULL ) {
			assert( lastBlock == block );
		} else {
			assert( block->next->prev == block );
		}
	}
}


#endif
