//
// rvHeap.cpp - Heap object 
// Date: 12/13/04
// Created by: Dwight Luetscher
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#ifdef _RV_MEM_SYS_SUPPORT

//#define DETECT_MEM_OVERWRITES

// Define some structures used by each heap
struct rvMemoryBlock_s 
{
	rvMemoryBlock_s* m_prev;									// previous block in memory (next is implied by address of memory block plus distToNextBlock)
	dword m_dwordDistToNextBlock : NUM_MEMORY_BLOCK_SIZE_BITS;	// distance, in double words (4 bytes), from the first byte of this rvMemoryBlock_s header to the first byte of the next rvMemoryBlock_s header
	dword m_tag : NUM_TAG_BITS;									// bits used to tag the type of allocation and state of memory block (zero for free)

	// GetDistToNextBlock
	//
	// returns: the distance to the next block, in bytes
	ID_INLINE dword GetDistToNextBlock() const
	{
		return m_dwordDistToNextBlock << 2;
	}

	// SetDistToNextBlock
	//
	// sets the distance to the next block, in bytes
	ID_INLINE void SetDistToNextBlock( dword distBytes )
	{
		assert( !(distBytes & 0x03) );
		m_dwordDistToNextBlock = distBytes >> 2;
	}
};

struct rvFreeMemoryBlock_s : rvMemoryBlock_s 
{
	rvFreeMemoryBlock_s* m_prevFree;
	rvFreeMemoryBlock_s* m_nextFree;
};

static const dword rvHeapAlignmentPadding	= 0xFFFFFFFF;	// fixed value used for padding an allocation for alignment

#ifdef DETECT_MEM_OVERWRITES
// a header and a trailer is stored at the front and rear of each allocation for 
// the purposes of detecting if an allocation has been written past its bounds
static const uint OVERWRITE_HEADER_SIZE			= 4;
static const uint OVERWRITE_TRAILER_SIZE		= 4;
static const char rvHeapAllocHeader[OVERWRITE_HEADER_SIZE]		= { '{', 'R', 'a', 'V' };
static const char rvHeapAllocTrailer[OVERWRITE_TRAILER_SIZE]	= { 'e', 'N', '1', '}' };
#else
static const uint OVERWRITE_HEADER_SIZE			= 0;
static const uint OVERWRITE_TRAILER_SIZE		= 0;
#endif

#define SIZE_TO_SMALL_TABLE_OFFSET(sizeBytes)	((((sizeBytes) + SMALL_BLOCK_ALIGN_SIZE_MASK) >> SMALL_BLOCK_ALIGN_SIZE_SHIFT) - 1)
#define SMALL_TABLE_OFFSET_TO_SIZE(tableOffset)	(((tableOffset) + 1) << SMALL_BLOCK_ALIGN_SIZE_SHIFT)

// rvHeap
//
// constructor
rvHeap::rvHeap( ) :
	mDebugID(-1)
{
	// ResetValues();	do this in the Init() call instead (due to the fact that other constructors could call rvHeap::Init() before this constructor is called)
}

// ~rvHeap
//
// destructor
rvHeap::~rvHeap( )
{
	Shutdown();
}

// Init
// 
// initializes this heap for use within the given arena, and with the given size limit that it can grow to 
//
// heapArena			heap arena that this heap is associated with
// maxHeapSizeBytes		maximum number of bytes this heap can grow to
// flags				flags describing the capabilities of this heap
void rvHeap::Init( rvHeapArena &heapArena, uint maxHeapSizeBytes, uint flags )
{
	DWORD protection = PAGE_READWRITE;
	DWORD allocType;
	uint numPageTableBytes, numTablePages;
	uint largeFreeRangeByteSize;
	byte* pageTableData;

	ResetValues();

	// create the critical section used by this heap
	InitializeCriticalSection( &m_criticalSection );

	// determine how many pages the virtual address range will consume
	m_numPages = (maxHeapSizeBytes + PAGE_SIZE - 1) >> PAGE_SIZE_SHIFT;
	m_maxHeapSizeBytes = m_numPages << PAGE_SIZE_SHIFT;
	
	m_heapRangeBytes = m_maxHeapSizeBytes + PAGE_SIZE;	// add a page at the beginning for zero-length allocations (not part of page table and is never committed)

	assert(sizeof(dword) == 4);	// assumed by following code
	m_pageCommitStateArrayByteSize = ((m_numPages + 31) >> 5)*sizeof(dword);

	m_smallFreePageRangeByteSize = m_numPages*sizeof(freePageRange_t);
	m_largeFreeRangeStorageSize = m_numPages/MAX_SMALL_PAGE_RANGE;
	if ( m_largeFreeRangeStorageSize < 1 )
	{
		m_largeFreeRangeStorageSize = 1;
	}
	largeFreeRangeByteSize = m_largeFreeRangeStorageSize*sizeof(freeLargePageRange_t);

	numPageTableBytes = m_pageCommitStateArrayByteSize;	// allocate storage for the m_pageCommitStateTable array 
	numPageTableBytes += m_smallFreePageRangeByteSize;	// allocate storage for the m_smallFreePageRangeTable array
	numPageTableBytes += largeFreeRangeByteSize;		// allocate storage for the m_largeFreeRangeStorage array

	numTablePages = (numPageTableBytes + PAGE_SIZE - 1) >> PAGE_SIZE_SHIFT;
	numPageTableBytes = numTablePages << PAGE_SIZE_SHIFT;

	// reserve the range of virtual memory needed by this heap
	if ( (flags & rvHeapFlagWriteCombine) != 0 )
	{
		protection |= PAGE_WRITECOMBINE;
	}
	if ( (flags & rvHeapFlagNoCache) != 0 )
	{
		protection |= PAGE_NOCACHE;
	}
	allocType = MEM_RESERVE;
#ifdef _XENON
	if ( PAGE_SIZE_SHIFT == 16 )
	{
		allocType |= MEM_LARGE_PAGES;
	}
	allocType |= MEM_NOZERO;
#endif
	m_baseAddress = (byte *) VirtualAlloc( NULL, numPageTableBytes+m_heapRangeBytes, allocType, protection );
	if ( NULL == m_baseAddress )
	{
		common->FatalError( "Unable to reserve desired size for heap.\n" );
		return;
	}
	m_zeroLengthAllocPage = m_baseAddress + numPageTableBytes;
	m_heapStorageStart = m_zeroLengthAllocPage + PAGE_SIZE;

	// commit the initial block of that range for use by the page table
	// Note: We want this to zero the memory for the page table init!
	allocType = MEM_COMMIT;
#ifdef _XENON
	if ( PAGE_SIZE_SHIFT == 16 )
	{
		allocType |= MEM_LARGE_PAGES;
	}
#endif
	pageTableData = (byte *) VirtualAlloc( m_baseAddress, numPageTableBytes, allocType, PAGE_READWRITE );
	if ( NULL == pageTableData )
	{
		common->FatalError( "Unable to commit pages needed for heap's page table.\n" );
		VirtualFree( m_baseAddress, 0, MEM_RELEASE );
		return;
	}
	m_committedSizeBytes = numPageTableBytes;

	m_pageCommitStateTable = (dword*) pageTableData;
	pageTableData += m_pageCommitStateArrayByteSize;

	m_smallFreePageRangeTable = (freePageRange_t *) pageTableData;
	pageTableData += m_smallFreePageRangeByteSize;

	m_largeFreeRangeStorage = (freeLargePageRange_t *) pageTableData;
	BuildLargeFreeRangeReuseList();

	// set up the information members
	m_flags = flags;

	heapArena.InitHeap( *this );
}

// Shutdown
//
// releases this heap from use
void rvHeap::Shutdown( )
{
	if ( m_arena != NULL ) 
	{
		VirtualFree( m_baseAddress, 0, MEM_RELEASE );
		m_arena->ShutdownHeap( *this );

		DeleteCriticalSection( &m_criticalSection );
	}
	ResetValues();
}

// ResetValues
//
// resets the data members to their pre-initialized state
void rvHeap::ResetValues( )
{
	m_arena = NULL;
	m_next = NULL;
	memset( &m_criticalSection, 0, sizeof(m_criticalSection) );
	m_baseAddress = NULL;
	m_zeroLengthAllocPage = NULL;
	m_heapStorageStart = NULL;
	memset( m_smallFreeBlocks, 0, sizeof(m_smallFreeBlocks) );
	m_largeFreeBlocks = NULL;
	m_pageCommitStateTable = NULL;
	m_smallFreePageRangeTable = NULL;
	memset( m_smallFreePageRangeLists, 0, sizeof(m_smallFreePageRangeLists) );
	m_largeFreePageRangeList = NULL;
	m_largeFreeRangeStorage = NULL;
	m_largeFreeRangeReuseList = NULL;
	m_largeFreeRangeStorageSize = 0;
	m_numPages = 0;
	m_heapRangeBytes = 0;
	m_committedSizeBytes = 0;
	m_maxHeapSizeBytes = 0;
	m_numBytesRequested = 0;
	m_numBytesAllocated = 0;
	memset(m_allocatedBytesByTag, 0, sizeof(m_allocatedBytesByTag));
	memset(m_peekAllocatedBytesByTag, 0, sizeof(m_peekAllocatedBytesByTag));
	memset(m_numAllocationsByTag, 0, sizeof(m_numAllocationsByTag));
	m_numAllocations = 0;
	m_flags = 0;
	m_numZeroLengthAllocations = 0;
	memset(m_name, 0, sizeof(m_name));
}

// BuildLargeFreeRangeReuseList
//
// Builds the m_largeFreeRangeReuseList linked-list from the m_largeFreeRangeStorage[] array.
void rvHeap::BuildLargeFreeRangeReuseList( )
{
	uint nOffset;

	m_largeFreeRangeReuseList = m_largeFreeRangeStorage + 1;
	for ( nOffset = 1; nOffset < m_largeFreeRangeStorageSize-1; nOffset++ )
	{
		m_largeFreeRangeStorage[ nOffset ].m_nextFree = &m_largeFreeRangeStorage[ nOffset + 1 ];
	}
	m_largeFreeRangeStorage[ m_largeFreeRangeStorageSize-1 ].m_nextFree = NULL;

	m_largeFreePageRangeList = m_largeFreeRangeStorage;
	m_largeFreePageRangeList->m_nextFree = NULL;
	m_largeFreePageRangeList->m_firstPageOffset = 0;
	m_largeFreePageRangeList->m_numContiguousPages = m_numPages;
}

// AllocateMemory
//
// allocates memory from this heap.
//
// sizeBytes		Size, in bytes, of desired allocation.
// allocationTag	Tag stored with this allocation (describing type of allocation)
// align16Flag		true if allocation must be aligned on a 16-byte boundary, false otherwise.
//
// returns: a pointer to the allocated memory, NULL if enough memory does not exist for allocation
byte *rvHeap::AllocateMemory( uint sizeBytes, int allocationTag, bool align16Flag )
{
//	align16Flag=true;
	byte *allocation;
	rvMemoryBlock_s *allocatedBlock;
	uint actualSizeBytes, extraBytes, distToNextBlock;

	if ( allocationTag == MA_NONE || allocationTag >= MA_DO_NOT_USE )
	{
		allocationTag = MA_DEFAULT;
	}

	EnterHeapCriticalSection( );

	// check for a zero-length allocation - returns a valid virtual address that will page fault should
	// it be written to, or read from (uncommitted page)
	if ( !sizeBytes )
	{
		// NOTE: The following is an attempt to provide an address within the range of this heap
		//		 that takes up no space (with the exception of a single virtual, uncommitted page).
		//		 The uniqueness of the address returned here is not guaranteed and is not aligned.
		m_numZeroLengthAllocations++;
		m_numAllocations++;
		allocation = (byte *) (m_zeroLengthAllocPage + (m_numZeroLengthAllocations & PAGE_SIZE_MASK));

		ExitHeapCriticalSection( );
		return allocation;
	}

	// determine how big the allocation is really going to need to be
	actualSizeBytes = sizeof(rvMemoryBlock_s) + sizeBytes + OVERWRITE_HEADER_SIZE + OVERWRITE_TRAILER_SIZE;
	extraBytes = OVERWRITE_HEADER_SIZE + OVERWRITE_TRAILER_SIZE;
	if ( align16Flag )
	{
		actualSizeBytes += 3*sizeof(rvHeapAlignmentPadding);	// for alignment
		extraBytes += 3*sizeof(rvHeapAlignmentPadding);
	}

	if ( actualSizeBytes > PAGE_SIZE )
	{
		// if greater than a page, and the tail just barely crosses a page boundary,
		// ask for a size that is a multiple of free memory block structure (so there is room for such a structure at the end)
		if ( (actualSizeBytes & PAGE_SIZE_MASK) < sizeof(rvFreeMemoryBlock_s) )
		{
			actualSizeBytes = (actualSizeBytes + (uint) sizeof(rvFreeMemoryBlock_s) - 1) & ~((uint) sizeof(rvFreeMemoryBlock_s) - 1);
		}
	}
	else if ( actualSizeBytes < sizeof(rvFreeMemoryBlock_s) )
	{
		actualSizeBytes = sizeof(rvFreeMemoryBlock_s);

		// make sure that the size is 4-byte aligned
		actualSizeBytes = ( actualSizeBytes + 3 ) & ~3;
	}

	// make sure that the size is 4-byte aligned
	actualSizeBytes = ( actualSizeBytes + 3 ) & ~3;

	if ( actualSizeBytes > MAX_SINGLE_ALLOCATION_SIZE )
	{
		// beyond what any heap can handle
		ExitHeapCriticalSection( );
		return NULL;
	}

	if ( actualSizeBytes <= MAX_SMALL_BLOCK_SIZE )
	{
		// perform a small block allocation
		allocatedBlock = AllocateMemorySmallBlock( actualSizeBytes );
	}
	else
	{
		// perform a large block allocation
		allocatedBlock = AllocateMemoryLargeBlock( actualSizeBytes );
	}

	if ( NULL == allocatedBlock ) 
	{
		ExitHeapCriticalSection( );
		return NULL;
	}

	distToNextBlock = allocatedBlock->GetDistToNextBlock();

	assert( allocationTag < MA_MAX );
	allocatedBlock->m_tag = (dword) allocationTag;
	m_allocatedBytesByTag[ allocationTag ] += distToNextBlock;
	if ( m_allocatedBytesByTag[ allocationTag ] > m_peekAllocatedBytesByTag[ allocationTag ] )
	{
		m_peekAllocatedBytesByTag[ allocationTag ] = m_allocatedBytesByTag[ allocationTag ];
	}
	m_numAllocationsByTag[ allocationTag ]++;

	m_numBytesAllocated += distToNextBlock;
	m_numBytesRequested += (distToNextBlock - sizeof(rvMemoryBlock_s) - extraBytes);
	m_numAllocations++;

	allocation = (byte *) allocatedBlock + sizeof(rvMemoryBlock_s);

#ifdef DETECT_MEM_OVERWRITES
	memcpy( allocation, rvHeapAllocHeader, OVERWRITE_HEADER_SIZE );
	allocation += OVERWRITE_HEADER_SIZE;
	byte *nextBlockAddress = (byte*) allocatedBlock + distToNextBlock;
	nextBlockAddress -= OVERWRITE_TRAILER_SIZE;
	memcpy( nextBlockAddress, rvHeapAllocTrailer, OVERWRITE_TRAILER_SIZE );
#endif

	if ( align16Flag )
	{
		while ( ((ulong) allocation & 0x0F) != 0 )
		{
			*(dword*)allocation = rvHeapAlignmentPadding;
			allocation += sizeof(rvHeapAlignmentPadding);
		}
	}

	ExitHeapCriticalSection( );
	return allocation;
}

// AllocateMemorySmallBlock
//
// perform a small block allocation - try to use an existing free block pointed to
// by the table.
//
// returns: a pointer to the allocation header in front of the newly allocated block of memory,
//			NULL if enough memory does not exist for allocation
rvMemoryBlock_s *rvHeap::AllocateMemorySmallBlock( uint sizeBytes )
{
	uint smallBlockTableOffset, stopOffset;	//, largerBlockOffset;

	smallBlockTableOffset = SIZE_TO_SMALL_TABLE_OFFSET(sizeBytes);
	assert(smallBlockTableOffset < NUM_SMALL_BLOCK_TABLE_ENTRIES);

#if 1
	stopOffset = smallBlockTableOffset + NUM_SMALL_BLOCK_TABLE_ENTRIES;
#else
	stopOffset = smallBlockTableOffset << 1;
#endif
	if ( stopOffset > m_largestFreeSmallBlockOffset )
	{
		stopOffset = m_largestFreeSmallBlockOffset;
	}

	while ( smallBlockTableOffset < stopOffset )
	{
		// check to see if there is a block that matches exactly
		if ( m_smallFreeBlocks[smallBlockTableOffset] != NULL ) 
		{
			// there is a block that is just the right size, remove from free list and return it.
			return AllocateBlockFromTable( smallBlockTableOffset, sizeBytes );
		}

/*
		NOTE: The following code is experimental and seems to result in greater fragmentation.

		// check to see if there is a block that matches at some multiple of the exact size
		largerBlockOffset = smallBlockTableOffset << 1;
		while ( largerBlockOffset <= m_largestFreeSmallBlockOffset ) 
		{
			if ( m_smallFreeBlocks[largerBlockOffset] != NULL ) 
			{
				// found a larger block to allocate from
				return AllocateBlockFromTable( largerBlockOffset, sizeBytes );
			}
			largerBlockOffset = largerBlockOffset << 1;
		}
*/

		smallBlockTableOffset++;
	}

	// an existing small block could not be found, try a larger block
	return AllocateMemoryLargeBlock( sizeBytes );
}

// AllocateBlockFromTable
//
// Allocates the a small memory block from the free block table
// and partitions it, if necessary, into an allocated chunk
// and a free chunk (which remains in the m_smallFreeBlocks[] table).
//
// returns: a pointer to the allocation header in front of the newly allocated block of memory,
//			NULL if enough memory does not exist for allocation
rvMemoryBlock_s *rvHeap::AllocateBlockFromTable( uint smallBlockTableOffset, uint sizeBytes )
{
	rvFreeMemoryBlock_s* allocatedBlockHeader, *newFreeBlock;
	uint originalBlockSize, newBlockSize, newBlockTableOffset;

	allocatedBlockHeader = m_smallFreeBlocks[smallBlockTableOffset];
	assert( sizeBytes <= allocatedBlockHeader->GetDistToNextBlock() );

	originalBlockSize = allocatedBlockHeader->GetDistToNextBlock();
	assert( originalBlockSize == SMALL_TABLE_OFFSET_TO_SIZE(smallBlockTableOffset) );

	// remove the free block from the table 
	m_smallFreeBlocks[smallBlockTableOffset] = m_smallFreeBlocks[smallBlockTableOffset]->m_nextFree;
	if ( m_smallFreeBlocks[smallBlockTableOffset] != NULL )
	{
		m_smallFreeBlocks[smallBlockTableOffset]->m_prevFree = NULL;
	}

	// see if the requested size essentially covers the entire free block 
	if ( sizeBytes + sizeof(rvFreeMemoryBlock_s) <= allocatedBlockHeader->GetDistToNextBlock() )
	{
		// this block is large enough to be split into two - an allocated chunk and a free chunk.
		// re-add the now smaller free chunk back into smallFreeBlocks[] table
		newBlockSize = allocatedBlockHeader->GetDistToNextBlock() - sizeBytes;

		newFreeBlock = (rvFreeMemoryBlock_s*) ((byte *) allocatedBlockHeader + sizeBytes);

		newFreeBlock->m_prev = allocatedBlockHeader;
		newFreeBlock->SetDistToNextBlock( newBlockSize );
		newFreeBlock->m_tag = MA_NONE;

		newBlockTableOffset = SIZE_TO_SMALL_TABLE_OFFSET(newBlockSize);
		if ( m_smallFreeBlocks[newBlockTableOffset] != NULL )
		{
			m_smallFreeBlocks[newBlockTableOffset]->m_prevFree = newFreeBlock;
		}

		newFreeBlock->m_prevFree = NULL;
		newFreeBlock->m_nextFree = m_smallFreeBlocks[newBlockTableOffset];
		m_smallFreeBlocks[newBlockTableOffset] = newFreeBlock;

		FixUpNextBlocksPrev( (byte *) newFreeBlock, newBlockSize );

		allocatedBlockHeader->SetDistToNextBlock( sizeBytes );
	}

	allocatedBlockHeader->m_tag = MA_DEFAULT;

	// check to see if the largest free small block has moved down
	TestLargestFreeSmallBlockOffset( smallBlockTableOffset );

	return allocatedBlockHeader;
}

// TestLargestFreeSmallBlockOffset
//
// checks to see if the largest free small block has moved down
void rvHeap::TestLargestFreeSmallBlockOffset( uint smallBlockTableOffset )
{
	if ( smallBlockTableOffset >= m_largestFreeSmallBlockOffset ) {

		// find the next largest available small block (if table entry is NULL)
		uint nextAvailableBlock = smallBlockTableOffset;
		while ( nextAvailableBlock > 0 && NULL == m_smallFreeBlocks[nextAvailableBlock] )
		{
			nextAvailableBlock--;
		}
		m_largestFreeSmallBlockOffset = nextAvailableBlock;
	}
}

// AllocateMemoryLargeBlock
//
// perform a large block allocation - try to use an existing free block from within
// the large free block linked-list.
//
// returns: a pointer to the allocation header in front of the newly allocated block of memory,
//			NULL if enough memory does not exist for allocation
rvMemoryBlock_s *rvHeap::AllocateMemoryLargeBlock( uint sizeBytes )
{
	rvFreeMemoryBlock_s **prevNextBlock, *curFreeBlock, *freeBlock;
	rvMemoryBlock_s *block = NULL;
	byte *newPageStart;
	uint numPages, totalAllocated = 0, remainingSize;

	prevNextBlock = &m_largeFreeBlocks;
	curFreeBlock = m_largeFreeBlocks;
	while ( curFreeBlock != NULL ) 
	{
		if ( sizeBytes <= curFreeBlock->GetDistToNextBlock() )
		{
			// we found a block that is big enough, remove it from the large free block list
			*prevNextBlock = curFreeBlock->m_nextFree;
			if ( curFreeBlock->m_nextFree != NULL )
			{
				curFreeBlock->m_nextFree->m_prevFree = curFreeBlock->m_prevFree;
			}
			totalAllocated = curFreeBlock->GetDistToNextBlock();
			block = (rvMemoryBlock_s *) curFreeBlock;
			break;
		}

		prevNextBlock = &curFreeBlock->m_nextFree;
		curFreeBlock = curFreeBlock->m_nextFree;
	}

	if ( NULL == block )
	{
		// an existing block could not be found, commit a new page range for use 
		numPages = (sizeBytes + PAGE_SIZE - 1) >> PAGE_SIZE_SHIFT;
		newPageStart = (byte *) PageCommit( numPages );
		if ( NULL == newPageStart )
		{
			// out of memory error
			return NULL;
		}

		// break the new page up into an allocated chunk (returned) and a free chunk (added to m_smallFreeBlocks[] or m_largeFreeBlocks)
		block = (rvMemoryBlock_s *) newPageStart;
		block->m_prev = NULL;

		totalAllocated = numPages << PAGE_SIZE_SHIFT;
	}

	block->m_tag = MA_DEFAULT;

	remainingSize = totalAllocated - sizeBytes;
	if ( remainingSize < sizeof(rvFreeMemoryBlock_s) )
	{
		// it is not worth creating a free block for what remains of last page
		block->SetDistToNextBlock( totalAllocated );
	}
	else 
	{
		// there is a enough space left at the end of the last page to generate
		// a free block
		block->SetDistToNextBlock( sizeBytes );
		freeBlock = (rvFreeMemoryBlock_s *) ((byte *) block + sizeBytes);
		assert( !((uint) freeBlock & 0x03) );	

		if ( ((uint) block >> PAGE_SIZE_SHIFT) == (((uint) block + sizeBytes) >> PAGE_SIZE_SHIFT) )
		{
			// new free block is on the same page
			freeBlock->m_prev = block;
		}
		else
		{
			// new free block is on a different page
			freeBlock->m_prev = NULL;
		}
		freeBlock->SetDistToNextBlock( remainingSize );
		freeBlock->m_tag = MA_NONE;
		
		FixUpNextBlocksPrev( (byte *) freeBlock, remainingSize );

		AddFreeBlock( freeBlock );
	}
	return block;
}

// FixUpNextBlocksPrev
//
// Fixes up the previous pointer of the memory block that lies immediately
// past the given newBlock, if it remains on the same page.
void rvHeap::FixUpNextBlocksPrev( byte *newBlock, uint blockSize )
{
	rvMemoryBlock_s* nextBlockPastNew;

	if ( ((uint) newBlock >> PAGE_SIZE_SHIFT) == ((uint) (newBlock + blockSize) >> PAGE_SIZE_SHIFT) )
	{
		// the next block remains on the same page, the previous pointer must be fixed up
		nextBlockPastNew = (rvMemoryBlock_s*) (newBlock + blockSize);
		nextBlockPastNew->m_prev = (rvMemoryBlock_s*) newBlock;
	}
}

// AddFreeBlock
// 
// Adds the given free block to the small allocation table or large allocation list.
void rvHeap::AddFreeBlock( rvFreeMemoryBlock_s *freeBlock )
{
	uint smallBlockTableOffset;

	if ( freeBlock->GetDistToNextBlock() <= MAX_SMALL_BLOCK_SIZE )
	{
		// add the block to the small free block table
		smallBlockTableOffset = SIZE_TO_SMALL_TABLE_OFFSET(freeBlock->GetDistToNextBlock());
		assert(smallBlockTableOffset < NUM_SMALL_BLOCK_TABLE_ENTRIES);

		if ( m_smallFreeBlocks[smallBlockTableOffset] != NULL ) 
		{
			m_smallFreeBlocks[smallBlockTableOffset]->m_prevFree = freeBlock;
		}
		else if ( smallBlockTableOffset > m_largestFreeSmallBlockOffset )
		{
			m_largestFreeSmallBlockOffset = smallBlockTableOffset;
		}

		freeBlock->m_prevFree = NULL;
		freeBlock->m_nextFree = m_smallFreeBlocks[smallBlockTableOffset];
		m_smallFreeBlocks[smallBlockTableOffset] = freeBlock;
	}
	else 
	{
		if ( m_largeFreeBlocks != NULL )
		{
			m_largeFreeBlocks->m_prevFree = freeBlock;
		}
		freeBlock->m_prevFree = NULL;
		freeBlock->m_nextFree = m_largeFreeBlocks;
		m_largeFreeBlocks = freeBlock;
	}
}

// RemoveFreeBlock
//
// Removes the given block from the small allocation table or large allocation list.
void rvHeap::RemoveFreeBlock( rvFreeMemoryBlock_s *freeBlock )
{
	uint smallBlockTableOffset;

	if ( freeBlock->GetDistToNextBlock() <= MAX_SMALL_BLOCK_SIZE )
	{
		if ( NULL == freeBlock->m_prevFree )
		{
			smallBlockTableOffset = SIZE_TO_SMALL_TABLE_OFFSET(freeBlock->GetDistToNextBlock());

			assert( smallBlockTableOffset < NUM_SMALL_BLOCK_TABLE_ENTRIES );
			assert( m_smallFreeBlocks[smallBlockTableOffset] == freeBlock );
		
			m_smallFreeBlocks[smallBlockTableOffset] = freeBlock->m_nextFree;

			// check to see if the largest free small block has moved down
			TestLargestFreeSmallBlockOffset( smallBlockTableOffset );
		}
		else
		{
			freeBlock->m_prevFree->m_nextFree = freeBlock->m_nextFree;
		}
	}
	else 
	{
		if ( NULL == freeBlock->m_prevFree )
		{
			assert( m_largeFreeBlocks == freeBlock );
			
			m_largeFreeBlocks = freeBlock->m_nextFree;
		}
		else 
		{
			freeBlock->m_prevFree->m_nextFree = freeBlock->m_nextFree;
		}
	}

	if ( freeBlock->m_nextFree != NULL )
	{
		freeBlock->m_nextFree->m_prevFree = freeBlock->m_prevFree;
	}
}

// Free
//
// free memory 
void rvHeap::Free( void *p )
{
	rvMemoryBlock_s *block, *prevBlock, *nextBlock;	
	rvFreeMemoryBlock_s *freeBlock;
	byte *nextBlockAddress, *allocation;
	byte *pageAddress;
	uint numPages, remainingSize, extraBytes, allocationTag, distToNextBlock;

	EnterHeapCriticalSection( );

	assert( m_numAllocations > 0 );
	assert( p != NULL );
	assert( DoesAllocBelong( p ) );

	m_numAllocations--;

	if ( p < m_zeroLengthAllocPage+PAGE_SIZE )
	{
		// this is a zero-length allocation
		assert(m_numZeroLengthAllocations>0);
		m_numZeroLengthAllocations--;
#ifdef _DETECT_BLOCK_MERGE_PROBLEMS
		TestFreeBlockValidity();
#endif
		ExitHeapCriticalSection( );
		return;
	}

	assert( !((uint) p & 0x03) );

	// back up over any heap alignment padding
	allocation = (byte *) p;
	extraBytes = 0;
	if ( *((dword*) allocation - 1) == rvHeapAlignmentPadding )
	{
		allocation -= sizeof(rvHeapAlignmentPadding);
		if ( *((dword*) allocation - 1) == rvHeapAlignmentPadding )
		{
			allocation -= sizeof(rvHeapAlignmentPadding);
			if ( *((dword*) allocation - 1) == rvHeapAlignmentPadding )
			{
				allocation -= sizeof(rvHeapAlignmentPadding);
			}
		}
		extraBytes += 3*sizeof(rvHeapAlignmentPadding);
	}

	// make sure that the overwrite header and trailer are intact
#ifdef DETECT_MEM_OVERWRITES
	allocation -= OVERWRITE_HEADER_SIZE;
	extraBytes += OVERWRITE_HEADER_SIZE + OVERWRITE_TRAILER_SIZE;
	if ( memcmp( allocation, rvHeapAllocHeader, OVERWRITE_HEADER_SIZE ) )
	{
		idLib::common->FatalError( "rvHeap::Free: memory block header overwrite (0x%x)", (dword) allocation );
	}
#endif

	// restore the memory block pointer
	block = (rvMemoryBlock_s *) (allocation - sizeof(rvMemoryBlock_s));

#ifdef DETECT_MEM_OVERWRITES
	nextBlockAddress = (byte*) block + block->GetDistToNextBlock();
	nextBlockAddress -= OVERWRITE_TRAILER_SIZE;
	if ( memcmp( nextBlockAddress, rvHeapAllocTrailer, OVERWRITE_TRAILER_SIZE ) )
	{
		idLib::common->FatalError( "rvHeap::Free: memory block trailer overwrite (0x%x)", (dword) nextBlockAddress );
	}
#endif

	pageAddress = (byte *) (((dword) allocation) & ~PAGE_SIZE_MASK);

    if ( ( block->m_prev != NULL && (byte *) block->m_prev < pageAddress ) ) 
	{
		idLib::common->FatalError( "rvHeap::Free: memory block header corrupted (0x%x)", (dword) allocation );
	}

	if ( block->m_tag == MA_NONE )
	{
		idLib::common->FatalError( "rvHeap::Free: attempt to free allocation that is already freed (0x%x)", (dword) allocation );
	}

	assert( (byte*) block >= m_heapStorageStart && ((byte*) block + block->GetDistToNextBlock()) <= (m_heapStorageStart + m_maxHeapSizeBytes) );

	distToNextBlock = block->GetDistToNextBlock();
	assert( distToNextBlock <= m_numBytesAllocated );
	m_numBytesAllocated -= distToNextBlock;
	m_numBytesRequested -= (distToNextBlock - sizeof(rvMemoryBlock_s) - extraBytes);

	allocationTag = block->m_tag;
	assert( distToNextBlock <= m_allocatedBytesByTag[ allocationTag ] && m_numAllocationsByTag[ allocationTag ] > 0 );
	m_allocatedBytesByTag[ allocationTag ] -= distToNextBlock;
	m_numAllocationsByTag[ allocationTag ]--;

	if ( block->GetDistToNextBlock() > PAGE_SIZE )
	{
		// this allocation that is being freed spans multiple pages - uncommit the ones 
		// before the last one
		numPages = block->GetDistToNextBlock() >> PAGE_SIZE_SHIFT;

		remainingSize = block->GetDistToNextBlock() - (numPages << PAGE_SIZE_SHIFT);

		PageUncommit( pageAddress, numPages );

		if ( !remainingSize )
		{
			// there are no remaining blocks
#ifdef _DETECT_BLOCK_MERGE_PROBLEMS
			TestFreeBlockValidity();
#endif
			ExitHeapCriticalSection( );
			return;
		}

		pageAddress += (numPages << PAGE_SIZE_SHIFT);

		block = (rvMemoryBlock_s *) pageAddress;
		block->m_prev = NULL;
		block->SetDistToNextBlock( remainingSize );
		block->m_tag = MA_NONE;

		FixUpNextBlocksPrev( (byte *) block, remainingSize );
	}

	nextBlockAddress = (byte*) block + block->GetDistToNextBlock();

	// mark the block as free
	block->m_tag = MA_NONE;

	// determine if the block we are freeing should be merged with the previous block
	prevBlock = block->m_prev;
	if ( prevBlock != NULL )
	{
		if ( prevBlock->m_tag == MA_NONE )
		{
			// the previous block is free, merge the block we are freeing to its previous neighbor
			RemoveFreeBlock( (rvFreeMemoryBlock_s *) prevBlock );
			prevBlock->SetDistToNextBlock( prevBlock->GetDistToNextBlock() + block->GetDistToNextBlock() );
			block = prevBlock;

			FixUpNextBlocksPrev( (byte *) prevBlock, prevBlock->GetDistToNextBlock() );
		}
	}

	// determine if the block we are freeing should be merged with the next block
	if ( nextBlockAddress < pageAddress+PAGE_SIZE )
	{
		nextBlock = (rvMemoryBlock_s *) nextBlockAddress;
		if ( nextBlock->m_tag == MA_NONE )
		{
			// the next block is free, merge the block we are freeing to its next neighbor
			RemoveFreeBlock( (rvFreeMemoryBlock_s *) nextBlock );

			block->SetDistToNextBlock( block->GetDistToNextBlock() + nextBlock->GetDistToNextBlock() );

			FixUpNextBlocksPrev( (byte *) block, block->GetDistToNextBlock() );
		}
	}

	// determine if block being freed should be added to small allocation table, large 
	// allocation list, or if the page itself should be uncommitted
	freeBlock = (rvFreeMemoryBlock_s *) block;
	if ( freeBlock->GetDistToNextBlock() < PAGE_SIZE )
	{
		AddFreeBlock( freeBlock );
	}
	else 
	{
		// this free block has reached the size of an entire page, uncommit it
		PageUncommit( pageAddress, 1 );
	}

#ifdef _DETECT_BLOCK_MERGE_PROBLEMS
	TestFreeBlockValidity();
#endif

	ExitHeapCriticalSection( );
}

// Msize
//
// returns: the size, in bytes, of the allocation at the given address (including header, alignment bytes, etc).
int rvHeap::Msize( void *p )
{
	rvMemoryBlock_s *block;	
	byte *allocation;
	dword size;

	EnterHeapCriticalSection( );

	assert( m_numAllocations > 0 );
	assert( p != NULL );
	assert( DoesAllocBelong( p ) );

	if ( p < m_zeroLengthAllocPage+PAGE_SIZE )
	{
		// this is a zero-length allocation
		ExitHeapCriticalSection( );
		return 0;
	}

	assert( !((uint) p & 0x03) );

	// back up over any heap alignment padding
	allocation = (byte *) p;
	if ( *((dword*) allocation - 1) == rvHeapAlignmentPadding )
	{
		allocation -= sizeof(rvHeapAlignmentPadding);
		if ( *((dword*) allocation - 1) == rvHeapAlignmentPadding )
		{
			allocation -= sizeof(rvHeapAlignmentPadding);
			if ( *((dword*) allocation - 1) == rvHeapAlignmentPadding )
			{
				allocation -= sizeof(rvHeapAlignmentPadding);
			}
		}
	}

	// make sure that the overwrite header and trailer are intact
#ifdef DETECT_MEM_OVERWRITES
	allocation -= OVERWRITE_HEADER_SIZE;
	if ( memcmp( allocation, rvHeapAllocHeader, OVERWRITE_HEADER_SIZE ) )
	{
		idLib::common->FatalError( "rvHeap::Free: memory block header overwrite (0x%x)", (dword) allocation );
	}
#endif

	// restore the memory block pointer
	block = (rvMemoryBlock_s *) (allocation - sizeof(rvMemoryBlock_s));

	size = block->GetDistToNextBlock();
	size -= ((dword)p-(dword)block) + OVERWRITE_TRAILER_SIZE;

	ExitHeapCriticalSection( );

	return size;
}

// FreeAll
//
// frees all the allocations from this heap (and decommits all pages)
void rvHeap::FreeAll( )
{
	if ( NULL == m_baseAddress )
	{
		return;
	}

	EnterHeapCriticalSection( );

	VirtualFree( m_heapStorageStart, m_maxHeapSizeBytes, MEM_DECOMMIT );	// this decommits all of the physical pages (frees physical memory), but leaves the virtual address range reserved to this heap

	memset( m_smallFreeBlocks, 0, sizeof(rvFreeMemoryBlock_s*)*NUM_SMALL_BLOCK_TABLE_ENTRIES );
	m_largeFreeBlocks = NULL;

	m_committedSizeBytes = 0;
	m_numBytesAllocated = 0;
	m_numBytesRequested = 0;

	m_numAllocations = 0;
	m_numZeroLengthAllocations = 0;

	m_largestFreeSmallBlockOffset = 0;

	memset( m_pageCommitStateTable, 0, m_pageCommitStateArrayByteSize );
	memset( m_smallFreePageRangeTable, 0, m_smallFreePageRangeByteSize );
    BuildLargeFreeRangeReuseList( );

	ExitHeapCriticalSection( );
}

// PageCommit
//
// commits the given number of contiguous pages to physical memory 
// (actually allocates the physical pages).
//
// returns: pointer to the first virtual address of the first committed page, 
//			NULL if commit failed.
void *rvHeap::PageCommit( uint numDesiredPages )
{
	freePageRange_t *freePageRange;
	freeLargePageRange_t *freeLargePageRange, **prevNextLargePageRange;
	uint listOffset, tableOffset, numRemainingPages, remainingPagesOffset, remainingListOffset;

	if ( numDesiredPages <= MAX_SMALL_PAGE_RANGE )
	{
		// the desired range of contiguous pages is considered "small" and the retrieval
		// of such a range is accelerated by the m_smallFreePageRangeLists[] table
		listOffset = numDesiredPages - 1;
		do
		{
			if ( m_smallFreePageRangeLists[listOffset] != NULL )
			{
				// we have found a contiguous range of pages that will hold the desired amount
				freePageRange = m_smallFreePageRangeLists[listOffset];	
				m_smallFreePageRangeLists[listOffset] =  m_smallFreePageRangeLists[listOffset]->m_nextFree;

				// determine the starting table offset of the page range (starting page number)
				tableOffset = freePageRange - m_smallFreePageRangeTable;

				// re-add any remainder back into the m_smallFreePageRangeTable[]
				numRemainingPages = listOffset + 1 - numDesiredPages;
				if ( numRemainingPages > 0 )
				{
					remainingListOffset = numRemainingPages - 1;
					remainingPagesOffset = tableOffset + numDesiredPages;
					m_smallFreePageRangeTable[ remainingPagesOffset ].m_nextFree = m_smallFreePageRangeLists[ remainingListOffset  ];
					m_smallFreePageRangeLists[ remainingListOffset  ] = &m_smallFreePageRangeTable[ remainingPagesOffset ];
				}

				// actually commit the pages to physical memory and mark the pages as committed within the m_pageCommitStateTable[]
				return CommitPageRange( tableOffset, numDesiredPages );
			}

			listOffset++;
		}
		while ( listOffset < MAX_SMALL_PAGE_RANGE );
	}

	// try find a suitable contiguous range of pages within the m_largeFreePageRangeList linked list
	freeLargePageRange = m_largeFreePageRangeList;
	prevNextLargePageRange = &m_largeFreePageRangeList;
	while ( freeLargePageRange != NULL )
	{
		if ( freeLargePageRange->m_numContiguousPages >= numDesiredPages )
		{
			// found a block that is big enough - split it if necessary
			tableOffset = freeLargePageRange->m_firstPageOffset;
			numRemainingPages = freeLargePageRange->m_numContiguousPages - numDesiredPages;
			remainingPagesOffset = tableOffset + numDesiredPages;

			if ( numRemainingPages <= MAX_SMALL_PAGE_RANGE )
			{
				// the freeLargePageRange_t structure is no longer needed in the m_largeFreePageRangeList list, remove it
				*prevNextLargePageRange = freeLargePageRange->m_nextFree;
				freeLargePageRange->m_nextFree = m_largeFreeRangeReuseList;
				m_largeFreeRangeReuseList = freeLargePageRange;

				if ( numRemainingPages > 0 )
				{
					// add the remainder into the m_smallFreePageRangeTable[]
					remainingListOffset = numRemainingPages - 1;
					m_smallFreePageRangeTable[ remainingPagesOffset ].m_nextFree = m_smallFreePageRangeLists[ remainingListOffset  ];
					m_smallFreePageRangeLists[ remainingListOffset  ] = &m_smallFreePageRangeTable[ remainingPagesOffset ];
				}
			}
			else 
			{
				// add the remainder back into the m_largeFreePageRangeList linked-list (NOTE: structure is still part of list)
				freeLargePageRange->m_firstPageOffset = remainingPagesOffset;
				freeLargePageRange->m_numContiguousPages = numRemainingPages;
			}

			return CommitPageRange( tableOffset, numDesiredPages );
		}

		prevNextLargePageRange = &freeLargePageRange->m_nextFree;
		freeLargePageRange = freeLargePageRange->m_nextFree;
	}

	return NULL;
}

// PageUncommit
//
// uncommits the given range of contiguous pages back to physical memory 
// (actually frees the physical pages).
void rvHeap::PageUncommit( byte *pageAddress, uint numPages )
{
	freeLargePageRange_t *freeLargePageRange = NULL;
	uint startPageOffset, prevPageOffset, nextPageOffset, listOffset;
	uint curDWordOffset, freeBlockPageCount, largeRangeCount, orgStartPageOffset, orgNumPages;

	pageAddress = (byte *) (((dword) pageAddress) & ~PAGE_SIZE_MASK);

	startPageOffset = (uint) (pageAddress - m_heapStorageStart) >> PAGE_SIZE_SHIFT;

	orgStartPageOffset = startPageOffset;
	orgNumPages = numPages;

	// determine if there is a free block of pages immediately previous and/or immediately following
	// this newly freed block of pages - merge into one big block if we can.
	if ( startPageOffset > 0 )
	{
		prevPageOffset = startPageOffset - 1;
		curDWordOffset = prevPageOffset >> 5;
		if ( !(m_pageCommitStateTable[ curDWordOffset ] & (1 << (prevPageOffset & 0x1F))) )
		{
			// the previous block is free, so now we need to determine what linked-list it
			// is on (how big is the continuous block of pages)
			freeBlockPageCount = 1;
			while ( prevPageOffset > 0 && freeBlockPageCount <= MAX_SMALL_PAGE_RANGE )
			{
				curDWordOffset = (prevPageOffset - 1) >> 5;
				if ( (m_pageCommitStateTable[ curDWordOffset ] & (1 << ((prevPageOffset - 1) & 0x1F))) != 0 )
				{
					break;
				}
				prevPageOffset--;
				freeBlockPageCount++;
			}

			if ( freeBlockPageCount <= MAX_SMALL_PAGE_RANGE )
			{
				RemoveFromSmallFreeRangeList( prevPageOffset, freeBlockPageCount );
				startPageOffset = prevPageOffset;
				numPages += freeBlockPageCount;
			}
			else
			{
				RemoveFromLargeFreeRangeList( prevPageOffset, startPageOffset, largeRangeCount );
				numPages += largeRangeCount;
			}
		}
	}

	if ( startPageOffset + numPages < m_numPages )
	{
		nextPageOffset = startPageOffset + numPages;
		curDWordOffset = nextPageOffset >> 5;
		if ( !(m_pageCommitStateTable[ curDWordOffset ] & (1 << (nextPageOffset & 0x1F))) )
		{
			// the next block is free, so now we need to determine what linked-list it
			// is on (how big is the continuous block of pages)
			freeBlockPageCount = 1;
			while ( nextPageOffset < m_numPages && freeBlockPageCount <= MAX_SMALL_PAGE_RANGE )
			{
				nextPageOffset++;
				curDWordOffset = nextPageOffset >> 5;
				if ( (m_pageCommitStateTable[ curDWordOffset ] & (1 << (nextPageOffset & 0x1F))) != 0 )
				{
					break;
				}
				freeBlockPageCount++;
			}

			if ( freeBlockPageCount <= MAX_SMALL_PAGE_RANGE )
			{
				RemoveFromSmallFreeRangeList( startPageOffset + numPages, freeBlockPageCount );
				numPages += freeBlockPageCount;
			}
			else
			{
				RemoveFromLargeFreeRangeList( startPageOffset + numPages, nextPageOffset, largeRangeCount );
				numPages += largeRangeCount;
			}
		}
	}

	if ( numPages <= MAX_SMALL_PAGE_RANGE ) 
	{
		// add the uncommitted page block into the m_smallFreePageRangeTable[]
		listOffset = numPages - 1;
		m_smallFreePageRangeTable[ startPageOffset ].m_nextFree = m_smallFreePageRangeLists[ listOffset  ];
		m_smallFreePageRangeLists[ listOffset ] = &m_smallFreePageRangeTable[ startPageOffset ];
	}
	else
	{
		// add the uncommitted page block onto the m_largeFreePageRangeList linked-list
		assert( m_largeFreeRangeReuseList != NULL );
		freeLargePageRange = m_largeFreeRangeReuseList;
		m_largeFreeRangeReuseList = m_largeFreeRangeReuseList->m_nextFree;

		freeLargePageRange->m_nextFree = m_largeFreePageRangeList;
		m_largeFreePageRangeList = freeLargePageRange;

		freeLargePageRange->m_firstPageOffset = startPageOffset;
		freeLargePageRange->m_numContiguousPages = numPages;
	}

	// actually perform the decommit of the pages and clear the bits in the page table
	UncommitPageRange( orgStartPageOffset, orgNumPages );
}

// RemoveFromSmallFreeRangeList
//
// Removes the given page range from the m_smallFreePageRangeLists[]
void rvHeap::RemoveFromSmallFreeRangeList( uint pageOffset, uint freeBlockPageCount )
{
	freePageRange_t *freePageRange, **prevNextFreePageRange;
	uint curOffset;

	assert( freeBlockPageCount <= MAX_SMALL_PAGE_RANGE && freeBlockPageCount > 0 );

	prevNextFreePageRange = &m_smallFreePageRangeLists[ freeBlockPageCount - 1 ];
	freePageRange = m_smallFreePageRangeLists[ freeBlockPageCount - 1 ];
	while ( freePageRange != NULL )
	{
		curOffset = (uint) (freePageRange - m_smallFreePageRangeTable);
		if ( curOffset == pageOffset )
		{
			*prevNextFreePageRange = freePageRange->m_nextFree;
			return;
		}
		prevNextFreePageRange = &freePageRange->m_nextFree;
		freePageRange = freePageRange->m_nextFree;
	}
	assert(0);	// we should not reach this
}

// RemoveFromLargeFreeRangeList
//
// Removes the given page range from the m_largeFreePageRangeList
void rvHeap::RemoveFromLargeFreeRangeList( uint pageOffset, uint &startPageOffset, uint &pageCount )
{
	freeLargePageRange_t *freeLargePageRange, ** prevNextFreeLargePageRange;

	prevNextFreeLargePageRange = &m_largeFreePageRangeList;
	freeLargePageRange = m_largeFreePageRangeList;
	while ( freeLargePageRange != NULL )
	{
		if ( pageOffset >= freeLargePageRange->m_firstPageOffset &&
			 pageOffset < freeLargePageRange->m_firstPageOffset + freeLargePageRange->m_numContiguousPages )
		{
			// found it!
			*prevNextFreeLargePageRange = freeLargePageRange->m_nextFree;
			startPageOffset = freeLargePageRange->m_firstPageOffset;
			pageCount = freeLargePageRange->m_numContiguousPages;

			freeLargePageRange->m_nextFree = m_largeFreeRangeReuseList;
			m_largeFreeRangeReuseList = freeLargePageRange;
			return;
		}

		prevNextFreeLargePageRange = &freeLargePageRange->m_nextFree;
		freeLargePageRange = freeLargePageRange->m_nextFree;
	}
	assert(0);	// we should not reach this
}

// CommitPageRange
//
// Commits the given range of pages and flags them as committed within the m_pageCommitStateTable array
void *rvHeap::CommitPageRange( uint startPageOffset, uint numPages )
{
	void* committedBaseAddress;
	uint curDWordOffset, endDWordOffset, commitSize, tailShift;
	dword mask;
	DWORD allocType;

	assert( startPageOffset + numPages <= m_numPages );

	// implement the commit
	commitSize = numPages << PAGE_SIZE_SHIFT;
	m_committedSizeBytes += commitSize;

	allocType = MEM_COMMIT;
#ifdef _XENON
	if ( PAGE_SIZE_SHIFT == 16 )
	{
		allocType |= MEM_LARGE_PAGES;
	}
	allocType |= MEM_NOZERO;
#endif
	committedBaseAddress = VirtualAlloc( m_heapStorageStart + (startPageOffset << PAGE_SIZE_SHIFT), commitSize, allocType, PAGE_READWRITE );
	if ( NULL == committedBaseAddress)
	{
		common->Warning( "Out of physical memory - unable to commit requested page range.\n" );
		return NULL;
	}

	assert(sizeof(dword) == 4);	// the following code assumes that a dword is 4 bytes
	
	// enable the bits that correspond to the pages being committed within the m_pageCommitStateTable array
	curDWordOffset = startPageOffset >> 5;
	endDWordOffset = (startPageOffset + numPages - 1) >> 5;

	if ( curDWordOffset == endDWordOffset )
	{
		mask = 0xFFFFFFFF << (startPageOffset & 0x1F);
		tailShift = (startPageOffset + numPages) & 0x1F;
		if ( tailShift != 0 )
		{
			mask ^= (0xFFFFFFFF << tailShift);
		}
		m_pageCommitStateTable[ curDWordOffset ] |= mask;
	}
	else 
	{
		mask = 0xFFFFFFFF << (startPageOffset & 0x1F);
		m_pageCommitStateTable[ curDWordOffset++ ] |= mask;

		while ( curDWordOffset < endDWordOffset )
		{
			m_pageCommitStateTable[ curDWordOffset ] |= 0xFFFFFFFF;
			curDWordOffset++;
		}

		mask = 0xFFFFFFFF >> (32 - (startPageOffset + numPages - (endDWordOffset << 5)));
		m_pageCommitStateTable[ curDWordOffset ] |= mask;
	}
	return committedBaseAddress;
}

// UncommitPageRange
//
// Uncommits the given range of pages and clears their flags within the m_pageCommitStateTable array
void rvHeap::UncommitPageRange( uint startPageOffset, uint numPages )
{
	BOOL rtnValue;
	uint curDWordOffset, endDWordOffset, decommitSize, tailShift;
	dword mask;

	assert( startPageOffset + numPages <= m_numPages );

	// implement the free
	decommitSize = numPages << PAGE_SIZE_SHIFT;
	assert( decommitSize <= m_committedSizeBytes );
	m_committedSizeBytes -= decommitSize;

	rtnValue = VirtualFree( m_heapStorageStart + (startPageOffset << PAGE_SIZE_SHIFT), 
							decommitSize, 
							MEM_DECOMMIT );
	assert( rtnValue );
	assert(sizeof(dword) == 4);	// the following code assumes that a dword is 4 bytes
	
	// disable the bits that correspond to the pages being decommitted within the m_pageCommitStateTable array
	curDWordOffset = startPageOffset >> 5;
	endDWordOffset = (startPageOffset + numPages - 1) >> 5;

	if ( curDWordOffset == endDWordOffset )
	{
		mask = 0xFFFFFFFF << (startPageOffset & 0x1F);
		tailShift = (startPageOffset + numPages) & 0x1F;
		if ( tailShift != 0 )
		{
			mask ^= (0xFFFFFFFF << tailShift);
		}
		m_pageCommitStateTable[ curDWordOffset ] &= ~mask;
	}
	else 
	{
		mask = 0xFFFFFFFF << (startPageOffset & 0x1F);
		m_pageCommitStateTable[ curDWordOffset++ ] &= ~mask;

		while ( curDWordOffset < endDWordOffset )
		{
			m_pageCommitStateTable[ curDWordOffset++ ] = 0;
		}

		mask = 0xFFFFFFFF >> (32 - (startPageOffset + numPages - (endDWordOffset << 5)));
		m_pageCommitStateTable[ curDWordOffset ] &= ~mask;
	}
}

// EnterHeapCriticalSection
//
// enters this heap's critical section
void rvHeap::EnterHeapCriticalSection()
{
	::EnterCriticalSection( &m_criticalSection );
}

// ExitHeapCriticalSection
//
// exits this heap's critical section
void rvHeap::ExitHeapCriticalSection()
{
	::LeaveCriticalSection( &m_criticalSection );
}

// GetSmallBlockFreeCount
//
// Get number of free blocks for this block type
// If there's a lot of free blocks, then it may be bad.
int rvHeap::GetSmallBlockFreeCount( int block ) const
{
	return GetBlockFreeCount( m_smallFreeBlocks[block] );
}


// GetSmallBlockFreeSize
//
// Get the actual physical storage committed for empty space in this block
dword rvHeap::GetSmallBlockFreeSize( int block ) const
{
	return GetBlockFreeSize( m_smallFreeBlocks[block] );
}

void rvHeap::SetName(const char* name)
{
	if(name)
	{
		idStr::Copynz(m_name, name, sizeof(m_name));
	}
}

const char* rvHeap::GetName(void) const
{
	if(!m_name[0])
	{
		return "UN-NAMED";
	}
	return m_name;
}

// GetLargeBlockFreeCount
//
int rvHeap::GetLargeBlockFreeCount() const 
{
	return GetBlockFreeCount( m_largeFreeBlocks );
}

// GetLargeBlockFreeSize
//
dword rvHeap::GetLargeBlockFreeSize() const 
{
	return GetBlockFreeSize( m_largeFreeBlocks );
}

// GetBlockFreeCount
//
int rvHeap::GetBlockFreeCount( rvFreeMemoryBlock_s* currentBlock ) const 
{
	dword freeCount = 0;
	while ( currentBlock ) 
	{
		currentBlock = currentBlock->m_nextFree;
		++freeCount;
	}
	return freeCount;
}

// GetBlockFreeSize
//
dword rvHeap::GetBlockFreeSize( rvFreeMemoryBlock_s* currentBlock ) const 
{
	dword freeSize = 0;
	while (currentBlock) 
	{
		dword blockSize = currentBlock->m_dwordDistToNextBlock << 2;
		
		if ( blockSize > PAGE_SIZE )
		{
#ifdef _DEBUG
   			dword initialBlockSize = blockSize;
#endif			
			// Round up to the next block boundary
			dword rem = (dword)currentBlock & ~PAGE_SIZE_MASK;
			blockSize-=rem;
			
			// Subtract off the pages that should be decomitted
			while ( blockSize > PAGE_SIZE ) 
			{
				blockSize-=PAGE_SIZE;
			}
			
			// Make sure we didn't wrap
#ifdef _DEBUG
   			assert( initialBlockSize > blockSize );
#endif			
			freeSize+=blockSize;
		} 
		else
		{
			// Does not span blocks, so just add
			freeSize+=blockSize;
		}
		
		currentBlock = currentBlock->m_nextFree;
	}
	
	return freeSize;
}

// GetSmallBlockFreeSize
//
void Mem_FragmentationStats_f( const idCmdArgs &args ) 
{
	rvHeap *heapArray[MAX_SYSTEM_HEAPS];
	rvGetAllSysHeaps( heapArray );
	
	dword unusedMem = 0;
	dword totalFragments = 0;
	
	for ( int i = 0; i < MAX_SYSTEM_HEAPS; ++i ) 
	{
		rvHeap *curr = heapArray[i];
		if ( curr ) 
		{
			for ( int j = 0; j < curr->SmallBlockCount(); ++j ) 
			{
				int freeCount = curr->GetSmallBlockFreeCount( j );
				if ( freeCount ) 
				{
					dword unused = curr->GetSmallBlockFreeSize(j);
					idLib::common->Printf( "i:%d c:%d t:%d - ", j, freeCount, unused );
					unusedMem+=unused;
					totalFragments+=freeCount;
				}
			}
			
			dword unused = curr->GetLargeBlockFreeSize();
			dword freeCount = curr->GetLargeBlockFreeCount();
			unusedMem+=unused;
			totalFragments+=freeCount;
			idLib::common->Printf( "i:large c:%d t:%d\n", freeCount, unused );
			
//			dword comSize = curr->GetCommittedSize();
			dword bytesAl = curr->GetBytesAllocated();
			
			idLib::common->Printf( "Total fragments: %d Fragment memory: %d\n", totalFragments, unusedMem );
			idLib::common->Printf( "Fragmentation: %f\n", float(unusedMem) / float(bytesAl) );
			
			// We only want the first one, since they all appear to be the same heap right now.
			break;
		}
	}
}

#ifdef _DETECT_BLOCK_MERGE_PROBLEMS
// TestFreeBlockValidity
//
// Tests for free memory block merge problems
void rvHeap::TestFreeBlockValidity()
{
	byte *pageAddress;
	rvMemoryBlock_s *nextBlock;
	rvFreeMemoryBlock_s *freeBlock;
	int smallBlockTableOffset;

	for ( smallBlockTableOffset = 0; 
		  smallBlockTableOffset < NUM_SMALL_BLOCK_TABLE_ENTRIES; 
		  smallBlockTableOffset++ )
	{
		// check to see if there is a block that matches exactly
		freeBlock = m_smallFreeBlocks[smallBlockTableOffset];
		while ( freeBlock != NULL ) 
		{
			// check the blocks around this block
			if ( freeBlock->m_prev != NULL && freeBlock->m_prev->m_tag == MA_NONE )
			{
				common->Warning( "Previous block failed to merge" );
			}

			nextBlock = (rvMemoryBlock_s *) ((byte*) freeBlock + freeBlock->GetDistToNextBlock());
			pageAddress = (byte *) (((dword) freeBlock) & ~PAGE_SIZE_MASK);
			
			if ( (byte*) nextBlock < pageAddress+PAGE_SIZE && nextBlock->m_tag == MA_NONE )
			{
				common->Warning( "Next block failed to merge" );
			}
						
			freeBlock = freeBlock->m_nextFree;
		}
	}
}
#endif

#endif	// #ifdef _RV_MEM_SYS_SUPPORT
