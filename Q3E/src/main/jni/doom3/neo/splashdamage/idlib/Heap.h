// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __HEAP_H__
#define __HEAP_H__


/*
===============================================================================

	Memory Management

	This is a replacement for the compiler heap code (i.e. "C" malloc() and
	free() calls). On average 2.5-3.0 times faster than MSVC malloc()/free().
	Worst case performance is 1.65 times faster and best case > 70 times.
 
===============================================================================
*/


struct memoryStats_t {
	int		num;
	size_t	minSize;
	size_t	maxSize;
	size_t	totalSize;
};

enum align_t {
	ALIGN_NONE	= 0,
	ALIGN_4		= 4,
	ALIGN_8		= 8,		// memory is always at least aligned on an 8-byte boundary
	ALIGN_16	= 16,
	ALIGN_32	= 32,
	ALIGN_64	= 64,
	ALIGN_128	= 128
};

void		Mem_Init( void );
void		Mem_Shutdown( void );
void		Mem_EnableLeakTest( const char *name );
void		Mem_ClearFrameStats( void );
void		Mem_GetFrameStats( memoryStats_t &allocs, memoryStats_t &frees );
void		Mem_GetStats( memoryStats_t &stats );
void		Mem_AllocDefragBlock( void );

size_t		Mem_Size( void *ptr );

#ifndef ID_DEBUG_MEMORY

void *		Mem_Alloc( const size_t size );
void *		Mem_ClearedAlloc( const size_t size );
void		Mem_Free( void *ptr );
char *		Mem_CopyString( const char *in );
void *		Mem_AllocAligned( const size_t size, const align_t align );
void		Mem_FreeAligned( void *ptr );
void *		Mem_ClearedAllocAligned( const size_t size, const align_t align );

#else /* ID_DEBUG_MEMORY */

enum memoryGroupType_t {
	MEMGROUP_LINE,
	MEMGROUP_FILE,
	MEMGROUP_FOLDER
};

enum memorySortType_t {
	MEMSORT_SIZE,
	MEMSORT_LOCATION,
	MEMSORT_NUMALLOCS,
	MEMSORT_CALLSTACK
};

void		Mem_Dump( const char* fileName );
void		Mem_DumpCompressed( const char *fileName, memoryGroupType_t memGroup, memorySortType_t memSort, int sortCallStack, int numFrames, bool xlFriendly = false );
void		Mem_DumpPerClass( const char *fileName );

void *		Mem_Alloc( const size_t size, const char *fileName, const int lineNumber );
void *		Mem_ClearedAlloc( const size_t size, const char *fileName, const int lineNumber );
void		Mem_Free( void *ptr, const char *fileName, const int lineNumber );
char *		Mem_CopyString( const char *in, const char *fileName, const int lineNumber );
void *		Mem_AllocAligned( const size_t size, const align_t align, const char *fileName, const int lineNumber );
void		Mem_FreeAligned( void *ptr, const char *fileName, const int lineNumber );
void *		Mem_ClearedAllocAligned( const size_t size, const align_t align, const char *fileName, const int lineNumber );

#endif /* ID_DEBUG_MEMORY */


#ifdef ID_REDIRECT_NEWDELETE

#ifndef ID_DEBUG_MEMORY

__inline void *operator new( size_t s ) {
	return Mem_Alloc( s );
}
__inline void operator delete( void *p ) {
	Mem_Free( p );
}
__inline void *operator new[]( size_t s ) {
	return Mem_Alloc( s );
}
__inline void operator delete[]( void *p ) {
	Mem_Free( p );
}

#else /* ID_DEBUG_MEMORY */

__inline void *operator new( size_t s, size_t t1, int t2, char *fileName, int lineNumber ) {
	return Mem_Alloc( s, fileName, lineNumber );
}
__inline void operator delete( void *p, size_t size, int t2, char *fileName, int lineNumber ) {
	Mem_Free( p, fileName, lineNumber );
}
__inline void *operator new[]( size_t s, size_t t1, int t2, char *fileName, int lineNumber ) {
	return Mem_Alloc( s, fileName, lineNumber );
}
__inline void operator delete[]( void *p, size_t size, int t2, char *fileName, int lineNumber ) {
	Mem_Free( p, fileName, lineNumber );
}
__inline void *operator new( size_t s ) {
	return Mem_Alloc( s, "__global", 0 );
}
__inline void operator delete( void *p ) {
	Mem_Free( p, "__global", 0 );
}
__inline void *operator new[]( size_t s ) {
	return Mem_Alloc( s, "__global", 0 );
}
__inline void operator delete[]( void *p ) {
	Mem_Free( p, "__global", 0 );
}

#endif /* ID_DEBUG_MEMORY */

#define ID_DEBUG_NEW						new( 0, 0, __FILE__, __LINE__ )
#define ID_DEBUG_DELETE						delete( 0, 0, __FILE__, __LINE__ )
#undef new
#define new									ID_DEBUG_NEW

#endif /* ID_REDIRECT_NEWDELETE */


#ifdef ID_DEBUG_MEMORY

#define		Mem_Alloc( size )				Mem_Alloc( size, __FILE__, __LINE__ )
#define		Mem_ClearedAlloc( size )		Mem_ClearedAlloc( size, __FILE__, __LINE__ )
#define		Mem_Free( ptr )					Mem_Free( ptr, __FILE__, __LINE__ )
#define		Mem_CopyString( s )				Mem_CopyString( s, __FILE__, __LINE__ )
#define		Mem_AllocAligned( size, align )	Mem_AllocAligned( size, align, __FILE__, __LINE__ )
#define		Mem_FreeAligned( ptr )			Mem_FreeAligned( ptr, __FILE__, __LINE__ )
#define		Mem_ClearedAllocAligned( size, align )	Mem_ClearedAllocAligned( size, align, __FILE__, __LINE__ )

#endif /* ID_DEBUG_MEMORY */

/*
===============================================================================

	Block based allocator for fixed size objects.

	All objects of the 'type' are properly constructed.
	However, the constructor is not called for re-used objects.

===============================================================================
*/

template<class type, int blockSize, bool threadSafe = false >
class idBlockAlloc {
public:
							idBlockAlloc( void );
							~idBlockAlloc( void );

	void					Shutdown( void );

	type *					Alloc( void );
	void					Free( type *element );

	size_t					GetTotalCount( void ) const { return total; }
	int						GetAllocCount( void ) const { return active; }
	int						GetFreeCount( void ) const { return total - active; }

private:
	struct element_t {
		type				t;
		element_t *			next;
	};
	struct block_t {
		element_t			elements[blockSize];
		block_t *			next;
	};

	block_t *				blocks;
	element_t *				free;
	size_t					total;
	int						active;
	sdLock					lock;					// lock for thread safe memory allocation
};

template<class type, int blockSize, bool threadSafe>
idBlockAlloc<type,blockSize,threadSafe>::idBlockAlloc( void ) {
	blocks = NULL;
	free = NULL;
	total = active = 0;
}

template<class type, int blockSize, bool threadSafe>
idBlockAlloc<type,blockSize,threadSafe>::~idBlockAlloc( void ) {
	Shutdown();
}

template<class type, int blockSize, bool threadSafe>
type *idBlockAlloc<type,blockSize,threadSafe>::Alloc( void ) {

	sdScopedLock< threadSafe > scopedLock( lock );

	if ( free == NULL ) {
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

template<class type, int blockSize, bool threadSafe>
void idBlockAlloc<type,blockSize,threadSafe>::Free( type *t ) {
	if( t == NULL ) {
		return;
	}

	sdScopedLock< threadSafe > scopedLock( lock );

	element_t *element = (element_t *)( ( (unsigned char *) t ) - ( (UINT_PTR) &((element_t *)0)->t ) );
	//element_t *element;
	//element = ( element_t * )( ( (unsigned char *)t ) - offsetof( element_t, t ) );
	element->next = free;
	free = element;
	active--;
}

template<class type, int blockSize, bool threadSafe>
void idBlockAlloc<type,blockSize,threadSafe>::Shutdown( void ) {

	sdScopedLock< threadSafe > scopedLock( lock );

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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
class idDynamicAlloc {
public:
									idDynamicAlloc( void );
									~idDynamicAlloc( void );

	void							Init( void );
	void							Shutdown( void );
	void							SetFixedBlocks( int numBlocks ) {}
	void							FreeEmptyBaseBlocks( void ) {}

	type *							Alloc( const size_t num );
	type *							Resize( type *ptr, const size_t num );
	void							Free( type *ptr );
	const char *					CheckMemory( const type *ptr ) const;

	int								GetNumBaseBlocks( void ) const { return 0; }
	size_t							GetBaseBlockMemory( void ) const { return 0; }
	int								GetNumUsedBlocks( void ) const { return numUsedBlocks; }
	size_t							GetUsedBlockMemory( void ) const { return usedBlockMemory; }
	int								GetNumFreeBlocks( void ) const { return 0; }
	int								GetFreeBlockMemory( void ) const { return 0; }
	int								GetNumEmptyBaseBlocks( void ) const { return 0; }

private:
	int								numUsedBlocks;			// number of used blocks
	size_t							usedBlockMemory;		// total memory in used blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear( void );
};

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
idDynamicAlloc<type, baseBlockSize, minBlockSize, threadSafe>::idDynamicAlloc( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
idDynamicAlloc<type, baseBlockSize, minBlockSize, threadSafe>::~idDynamicAlloc( void ) {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Shutdown( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
type *idDynamicAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Alloc( const size_t num ) {
	numAllocs++;
	if ( num <= 0 ) {
		return NULL;
	}
	numUsedBlocks++;
	usedBlockMemory += num * sizeof( type );
	return (type *)Mem_AllocAligned( num * sizeof( type ), ALIGN_16 );
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
type *idDynamicAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Resize( type *ptr, const size_t num ) {

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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Free( type *ptr ) {
	numFrees++;
	if ( ptr == NULL ) {
		return;
	}
	Mem_FreeAligned( ptr );
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
const char *idDynamicAlloc<type, baseBlockSize, minBlockSize, threadSafe>::CheckMemory( const type *ptr ) const {
	return NULL;
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Clear( void ) {
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

const int MAX_DYNAMICBLOCK_SIZE = static_cast< unsigned int >( 1 << 31 ) - 1;

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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
class idDynamicBlockAlloc {
public:
									idDynamicBlockAlloc( void );
									~idDynamicBlockAlloc( void );

	void							Init( void );
	void							Shutdown( void );
	void							SetFixedBlocks( int numBlocks );
	void							FreeEmptyBaseBlocks( void );

	type *							Alloc( const size_t num );
	type *							Resize( type *ptr, const size_t num );
	void							Free( type *ptr );
	const char *					CheckMemory( const type *ptr ) const;

	int								GetNumBaseBlocks( void ) const { return numBaseBlocks; }
	size_t							GetBaseBlockMemory( void ) const { return baseBlockMemory; }
	int								GetNumUsedBlocks( void ) const { return numUsedBlocks; }
	size_t							GetUsedBlockMemory( void ) const { return usedBlockMemory; }
	int								GetNumFreeBlocks( void ) const { return numFreeBlocks; }
	int								GetFreeBlockMemory( void ) const { return freeBlockMemory; }
	int								GetNumEmptyBaseBlocks( void ) const;

private:
	idDynamicBlock<type> *			firstBlock;				// first block in list in order of increasing address
	idDynamicBlock<type> *			lastBlock;				// last block in list in order of increasing address
	idBTree<idDynamicBlock<type>,int,4>freeTree;			// B-Tree with free memory blocks
	bool							allowAllocs;			// allow base block allocations
	sdLock							lock;					// lock for thread safe memory allocation

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	int								blockId[3];
#endif

	int								numBaseBlocks;			// number of base blocks
	size_t							baseBlockMemory;		// total memory in base blocks
	int								numUsedBlocks;			// number of used blocks
	size_t							usedBlockMemory;		// total memory in used blocks
	int								numFreeBlocks;			// number of free blocks
	int								freeBlockMemory;		// total memory in free blocks

	int								numAllocs;
	int								numResizes;
	int								numFrees;

	void							Clear( void );
	idDynamicBlock<type> *			AllocInternal( const size_t num );
	idDynamicBlock<type> *			ResizeInternal( idDynamicBlock<type> *block, const size_t num );
	void							FreeInternal( idDynamicBlock<type> *block );
	void							LinkFreeInternal( idDynamicBlock<type> *block );
	void							UnlinkFreeInternal( idDynamicBlock<type> *block );
	void							CheckMemory( void ) const;
};

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::idDynamicBlockAlloc( void ) {
	Clear();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::~idDynamicBlockAlloc( void ) {
	Shutdown();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Init( void ) {
	freeTree.Init();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Shutdown( void ) {
	idDynamicBlock<type> *block;

	for ( block = firstBlock; block != NULL; block = block->next ) {
		if ( block->node == NULL ) {
			FreeInternal( block );
		}
	}

	for ( block = firstBlock; block != NULL; block = firstBlock ) {
		firstBlock = block->next;
		assert( block->IsBaseBlock() );
		Mem_FreeAligned( block );
	}

	freeTree.Shutdown();

	Clear();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::SetFixedBlocks( int numBlocks ) {
	idDynamicBlock<type> *block;

	for ( int i = numBaseBlocks; i < numBlocks; i++ ) {
		block = ( idDynamicBlock<type> * ) Mem_AllocAligned( baseBlockSize, ALIGN_16 );
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = (void*)this;
#endif
		block->SetSize( baseBlockSize - sizeof( idDynamicBlock<type> ), true );
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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::FreeEmptyBaseBlocks( void ) {
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
			numBaseBlocks--;
			baseBlockMemory -= block->GetSize() + sizeof( idDynamicBlock<type> );
			Mem_FreeAligned( block );
		}
	}

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
int idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::GetNumEmptyBaseBlocks( void ) const {
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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
type *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Alloc( const size_t num ) {
	idDynamicBlock<type> *block;

	sdScopedLock< threadSafe > scopedLock( lock );

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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
type *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Resize( type *ptr, const size_t num ) {

	sdScopedLock< threadSafe > scopedLock( lock );

	numResizes++;

	if ( ptr == NULL ) {
		return Alloc( num );
	}

	if ( num <= 0 ) {
		Free( ptr );
		return NULL;
	}

	idDynamicBlock<type> *block = ( idDynamicBlock<type> * ) ( ( (byte *) ptr ) - sizeof( idDynamicBlock<type> ) );

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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Free( type *ptr ) {

	sdScopedLock< threadSafe > scopedLock( lock );

	numFrees++;

	if ( ptr == NULL ) {
		return;
	}

	idDynamicBlock<type> *block = ( idDynamicBlock<type> * ) ( ( (byte *) ptr ) - sizeof( idDynamicBlock<type> ) );

	numUsedBlocks--;
	usedBlockMemory -= block->GetSize();

	FreeInternal( block );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	CheckMemory();
#endif

}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
const char *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::CheckMemory( const type *ptr ) const {
	idDynamicBlock<type> *block;

	if ( ptr == NULL ) {
		return NULL;
	}

	block = ( idDynamicBlock<type> * ) ( ( (byte *) ptr ) - sizeof( idDynamicBlock<type> ) );

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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::Clear( void ) {
	firstBlock = lastBlock = NULL;
	allowAllocs = true;
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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
idDynamicBlock<type> *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::AllocInternal( const size_t num ) {
	idDynamicBlock<type> *block;
	size_t alignedNum = ( num * sizeof( type ) + 15 ) & ~15;

	if ( alignedNum > MAX_DYNAMICBLOCK_SIZE ) {
		return NULL;
	}

	int alignedBytes = static_cast< int >( alignedNum );

	block = freeTree.FindSmallestLargerEqual( alignedBytes );
	if ( block != NULL ) {
		UnlinkFreeInternal( block );
	} else if ( allowAllocs ) {
		int allocSize = Max( baseBlockSize, alignedBytes + static_cast< int >( sizeof( idDynamicBlock<type> ) ) );
		block = ( idDynamicBlock<type> * ) Mem_AllocAligned( allocSize, ALIGN_16 );
#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
		memcpy( block->id, blockId, sizeof( block->id ) );
		block->allocator = (void*)this;
#endif
		block->SetSize( allocSize - sizeof( idDynamicBlock<type> ), true );
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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
idDynamicBlock<type> *idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::ResizeInternal( idDynamicBlock<type> *block, const size_t num ) {
	size_t alignedNum = ( num * sizeof( type ) + 15 ) & ~15;

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == (void*)this );
#endif

	if ( alignedNum > MAX_DYNAMICBLOCK_SIZE ) {
		return NULL;
	}

	int alignedBytes = static_cast< int >( alignedNum );

	// if the new size is larger
	if ( alignedBytes > block->GetSize() ) {

		idDynamicBlock<type> *nextBlock = block->next;

		// try to annexate the next block if it's free
		if ( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != NULL &&
				block->GetSize() + static_cast< int >( sizeof( idDynamicBlock<type> ) ) + nextBlock->GetSize() >= alignedBytes ) {

			UnlinkFreeInternal( nextBlock );
			block->SetSize( block->GetSize() + sizeof( idDynamicBlock<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
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
	if ( block->GetSize() - alignedBytes - static_cast< int >( sizeof( idDynamicBlock<type> ) ) < Max( minBlockSize, static_cast< int >( sizeof( type ) ) ) ) {
		return block;
	}

	idDynamicBlock<type> *newBlock;

	newBlock = ( idDynamicBlock<type> * ) ( ( (byte *) block ) + sizeof( idDynamicBlock<type> ) + alignedBytes );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	memcpy( newBlock->id, blockId, sizeof( newBlock->id ) );
	newBlock->allocator = (void*)this;
#endif
	newBlock->SetSize( block->GetSize() - alignedBytes - sizeof( idDynamicBlock<type> ), false );
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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::FreeInternal( idDynamicBlock<type> *block ) {

	assert( block->node == NULL );

#ifdef DYNAMIC_BLOCK_ALLOC_CHECK
	assert( block->id[0] == 0x11111111 && block->id[1] == 0x22222222 && block->id[2] == 0x33333333 && block->allocator == (void*)this );
#endif

	// try to merge with a next free block
	idDynamicBlock<type> *nextBlock = block->next;
	if ( nextBlock && !nextBlock->IsBaseBlock() && nextBlock->node != NULL ) {
		UnlinkFreeInternal( nextBlock );
		block->SetSize( block->GetSize() + sizeof( idDynamicBlock<type> ) + nextBlock->GetSize(), block->IsBaseBlock() );
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
		prevBlock->SetSize( prevBlock->GetSize() + sizeof( idDynamicBlock<type> ) + block->GetSize(), prevBlock->IsBaseBlock() );
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

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
ID_INLINE void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::LinkFreeInternal( idDynamicBlock<type> *block ) {
	block->node = freeTree.Add( block, block->GetSize() );
	numFreeBlocks++;
	freeBlockMemory += block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
ID_INLINE void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::UnlinkFreeInternal( idDynamicBlock<type> *block ) {
	freeTree.Remove( block->node );
	block->node = NULL;
	numFreeBlocks--;
	freeBlockMemory -= block->GetSize();
}

template<class type, int baseBlockSize, int minBlockSize, bool threadSafe>
void idDynamicBlockAlloc<type, baseBlockSize, minBlockSize, threadSafe>::CheckMemory( void ) const {
	idDynamicBlock<type> *block;

	sdScopedLock< threadSafe > scopedLock( lock );

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

#endif /* !__HEAP_H__ */
