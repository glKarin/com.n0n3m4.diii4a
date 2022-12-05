#ifndef __BLOCKPOOL_H__
#define __BLOCKPOOL_H__

#ifdef _RV_MEM_SYS_SUPPORT

/*
===============================================================================

	Block based allocator for fixed size objects.

	All objects of the 'type' are properly constructed.
	However, the constructor is not called for re-used objects.

	This is essentially the same as idBlockAlloc found in heap.h. The difference
	is that rvBlockPool is aware of which system heap it should allocate into.

===============================================================================
*/

template<class type, int blockSize, byte memoryTag, Rv_Sys_Heap_ID_t heapID>
class rvBlockPool {
public:
							rvBlockPool( void );
							~rvBlockPool( void );

	void					Shutdown( void );

	type *					Alloc( void );
	void					Free( type *element );

	int						GetTotalCount( void ) const { return total; }
	int						GetAllocCount( void ) const { return active; }
	int						GetFreeCount( void ) const { return total - active; }

	size_t					Allocated( void ) const { return( total * sizeof( type ) ); }

private:
	typedef struct element_s {
		struct element_s *	next;
		type				t;
	} element_t;
	typedef struct block_s {
		element_t			elements[blockSize];
		struct block_s *	next;
	} block_t;

	block_t *				blocks;
	element_t *				free;
	int						total;
	int						active;
};

template<class type, int blockSize, byte memoryTag, Rv_Sys_Heap_ID_t heapID>
rvBlockPool<type,blockSize,memoryTag,heapID>::rvBlockPool( void ) {
	blocks = NULL;
	free = NULL;
	total = active = 0;
}

template<class type, int blockSize, byte memoryTag, Rv_Sys_Heap_ID_t heapID>
rvBlockPool<type,blockSize,memoryTag,heapID>::~rvBlockPool( void ) {
	Shutdown();
}

template<class type, int blockSize, byte memoryTag, Rv_Sys_Heap_ID_t heapID>
type *rvBlockPool<type,blockSize,memoryTag,heapID>::Alloc( void ) {
	if ( !free ) {
		RV_PUSH_SYS_HEAP_ID(heapID);
		block_t *block = new block_t;
		RV_POP_HEAP();
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

template<class type, int blockSize, byte memoryTag, Rv_Sys_Heap_ID_t heapID>
void rvBlockPool<type,blockSize,memoryTag,heapID>::Free( type *t ) {
	element_t *element = (element_t *)( ( (unsigned char *) t ) - ( (int) &((element_t *)0)->t ) );
	element->next = free;
	free = element;
	active--;
}

template<class type, int blockSize, byte memoryTag, Rv_Sys_Heap_ID_t heapID>
void rvBlockPool<type,blockSize,memoryTag,heapID>::Shutdown( void ) {
	while( blocks ) {
		block_t *block = blocks;
		blocks = blocks->next;
		delete block;
	}
	blocks = NULL;
	free = NULL;
	total = active = 0;
}

#endif // _RV_MEM_SYS_SUPPORT
#endif // __BLOCKPOOL_H__
