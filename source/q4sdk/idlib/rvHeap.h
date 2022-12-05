//
// rvHeap.h - Heap object (replacing the idHeap class)
// Date: 12/13/04
// Created by: Dwight Luetscher
//

#ifndef __RV_HEAP_H__
#define __RV_HEAP_H__

class rvHeapArena;
struct rvMemoryBlock_s;
struct rvFreeMemoryBlock_s;

//#define _DETECT_BLOCK_MERGE_PROBLEMS

// Define some flags used in describing various properties of allocations 
// coming from this heap (these flags are passed into rvHeap::Init()).
static const uint rvHeapFlagDefault				= 0;
static const uint rvHeapFlagWriteCombine		= 0x0001;	// flag specified if the memory allocated from this heap is intended to have a write-combine cache policy
static const uint rvHeapFlagNoCache				= 0x0002;	// flag specified if the memory allocated from this heap is intended to have a no cache policy

// Define some limits used by each heap 
static const uint NUM_MEMORY_BLOCK_SIZE_BITS	= 27;
static const uint NUM_TAG_BITS					= 5;

static const uint MAX_SINGLE_ALLOCATION_SIZE	= ((1 << (NUM_MEMORY_BLOCK_SIZE_BITS+2)) - 1);	// maximum number of bytes that can be allocated in a single allocation
static const uint NUM_SMALL_BLOCK_TABLE_ENTRIES	= 2048;					// number of entries within each heap's small free memory block table

static const uint SMALL_BLOCK_ALIGN_SIZE_SHIFT	= 2;					// left shift of the size that all allocations are rounded to
static const uint SMALL_BLOCK_ALIGN_SIZE		= 1 << SMALL_BLOCK_ALIGN_SIZE_SHIFT;	// size that all allocations are rounded to
static const uint SMALL_BLOCK_ALIGN_SIZE_MASK	= SMALL_BLOCK_ALIGN_SIZE-1;				// mask of the bits used for the size that all allocations are rounded to
static const uint MAX_SMALL_BLOCK_SIZE			= (NUM_SMALL_BLOCK_TABLE_ENTRIES << SMALL_BLOCK_ALIGN_SIZE_SHIFT) - 1;	// maximum size of an allocation to be considered a "small block" allocation

static const uint PAGE_SIZE_SHIFT				= 16;					// left shift of the page size used by this heap object
static const uint PAGE_SIZE						= 1 << PAGE_SIZE_SHIFT;	// page size used by this heap object
static const uint PAGE_SIZE_MASK				= PAGE_SIZE - 1;

static const uint MAX_SMALL_PAGE_RANGE			= 128;						// maximum number of contiguous pages for a single range to be considered "small"
static const uint MIN_LARGE_PAGE_RANGE			= MAX_SMALL_PAGE_RANGE+1;	// minimum number of contiguous pages for a single range to be considered "large"

// Define the freePageRange_t structure used by this rvHeap used to maintain linked-lists of contiguous page ranges below LARGE_PAGE_RANGE
typedef struct freePageRange_s
{
	freePageRange_s* m_nextFree;						// next range of free pages in a linked-list
}
freePageRange_t;

// Define the freePageRange_t structure used by this rvHeap used to maintain a linked-list of contiguous page ranges above LARGE_PAGE_RANGE
typedef struct freeLargePageRange_s
{
	freeLargePageRange_s* m_nextFree;					// next large range of free pages in a linked-list
	uint m_firstPageOffset;								// offset of the first page in this range
	uint m_numContiguousPages;							// count of the number of contiguous pages in this range
}
freeLargePageRange_t;

class rvHeap 
{
public:
	rvHeap( );											// constructor
	~rvHeap( );											// destructor

	void Init( rvHeapArena &heapArena, uint maxHeapSizeBytes, uint flags = rvHeapFlagDefault );	// initializes this heap for use within the given arena, and with the given size limit that it can grow to 
	void Shutdown( );									// releases this heap from use

	ID_INLINE bool IsInitialized( ) const;				// returns true if this heap is currently initialized, and not shutdown
	
	ID_INLINE void *Allocate( uint sizeBytes, int debugTag = 0 );	// allocate memory
	ID_INLINE void *Allocate16( uint sizeBytes, int debugTag = 0 );	// allocate memory aligned on a 16-byte boundary
	void Free( void *allocation );						// free memory 

	int Msize( void *allocation );						// returns the size, in bytes, of the allocation at the given address (including header, alignment bytes, etc).

	void FreeAll( );									// frees all the allocations from this heap (and decommits all pages)

	ID_INLINE bool DoesAllocBelong( void  *allocBase );	// returns true if the given address was allocated from this heap, false otherwise

	ID_INLINE rvHeapArena *GetArena( );					// returns the arena this heap is associated with

	ID_INLINE void PushCurrent( );						// makes this heap the new top of stack for its associated arena, thus making it current
	ID_INLINE void PopCurrent( );						// pops this heap, or any other heap, from the top of stack of this heap's associated arena, thus making its predecessor current

	ID_INLINE uint GetSize( ) const;					// returns the current size that this heap has grown to, in bytes
	ID_INLINE uint GetMaxSize( ) const;					// returns the maximum size that this heap can grow to, in bytes

	ID_INLINE uint GetCommittedSize( ) const;			// returns the number of physical bytes this heap is currently using (total bytes of all committed pages)
	ID_INLINE uint GetBytesAllocated( ) const;			// returns the number of bytes currently allocated from this heap (actual memory footprint of all active allocations including internal headers and alignment bytes)
	ID_INLINE uint GetBytesRequested( ) const;
	ID_INLINE int GetNumAllocations( ) const;			// returns the number of allocations made from this heap that are still active (have not been freed)

	ID_INLINE uint GetBytesAllocatedByTag( Mem_Alloc_Types_t allocType ) const;		// returns the number of bytes currently allocated from this heap with the given allocation tag (actual memory footprint of all active allocations including internal headers and alignment bytes)
	ID_INLINE uint GetPeekBytesAllocatedByTag( Mem_Alloc_Types_t allocType ) const;	// returns the peek number of bytes allocated from this heap with the given allocation tag (actual memory footprint of all active allocations including internal headers and alignment bytes)
	ID_INLINE int GetNumAllocationsByTag( Mem_Alloc_Types_t allocType ) const;		// returns the current number of allocation from this heap with the given allocation tag 

	ID_INLINE bool IsWriteCombine( ) const;				// returns true if memory allocated from this heap has a write-combine cache policy, false otherwise
	ID_INLINE bool IsNoCache( ) const;					// returns true if memory allocated from this heap has a no-cache policy, false otherwise

	int SmallBlockCount() const { return NUM_SMALL_BLOCK_TABLE_ENTRIES; }
	int GetSmallBlockFreeCount( int block ) const;
	dword GetSmallBlockFreeSize( int block ) const;
	
	int GetLargeBlockFreeCount() const;
	dword GetLargeBlockFreeSize() const;

	int GetBlockFreeCount( rvFreeMemoryBlock_s* currentBlock ) const;
	dword GetBlockFreeSize( rvFreeMemoryBlock_s* currentBlock ) const;

	void SetName(const char* name);
	const char* GetName(void) const;
	void SetDebugID(byte debugID) {mDebugID=debugID;}
	byte DebugID(void) const {return mDebugID;}

protected:
	rvHeapArena *m_arena;								// arena associated with this heap
	rvHeap *m_next;										// next heap belonging to the same arena
	CRITICAL_SECTION m_criticalSection;					// critical section associated with this heap

	byte *m_baseAddress;								// base address of this heap (virtual)
	byte *m_zeroLengthAllocPage;						// special page for zero-length allocations (remains uncommitted) 
	byte *m_heapStorageStart;							// address of the start of this heap's storage (just past page table)

	rvFreeMemoryBlock_s *m_smallFreeBlocks[NUM_SMALL_BLOCK_TABLE_ENTRIES];	// table used to manage linked-lists of small free memory blocks
	rvFreeMemoryBlock_s *m_largeFreeBlocks;				// linked-list of all of the large free memory blocks

	dword *m_pageCommitStateTable;						// a very long bit mask (1-bit per page) that describes whether the page has been committed or not to physical memory
	freePageRange_t *m_smallFreePageRangeTable;			// array of structures that each correspond to a particular page - the structures representing the pages at the start of a small range are added to lists within the m_smallFreePageRangeLists[] array below

	freePageRange_t *m_smallFreePageRangeLists[MAX_SMALL_PAGE_RANGE];		// array where each entry is a linked-list of structures that individually describe a contiguous range of pages (where the range count matches the index into this array plus 1)
	freeLargePageRange_t *m_largeFreePageRangeList;		// linked-list of structures that individually describe a large contiguous range of pages

	// the following two data members are just used to manage the storage of the freeLargePageRange_t structures
	freeLargePageRange_t *m_largeFreeRangeReuseList;	// linked-list of freeLargePageRange_t that are currently not part of the m_largeFreePageRangeList linked-list and are available for use
	freeLargePageRange_t *m_largeFreeRangeStorage;		// array that acts as a store of the freeLargePageRange_t structures
	uint m_largeFreeRangeStorageSize;					// number of entries in the m_largeFreeRangeStorage[] array
	uint m_pageCommitStateArrayByteSize;				// size of the m_pageCommitStateTable[] array in bytes
	uint m_smallFreePageRangeByteSize;					// size of the m_smallFreePageRangeTable[] array in bytes

	uint m_numPages;									// number of pages that can span this heap entirely

	uint m_heapRangeBytes;								// the number of bytes in the virtual address space of this heap (including zero-length allocation page)

	uint m_committedSizeBytes;							// the number of physical bytes this heap is currently using (total bytes of all committed pages)
	uint m_maxHeapSizeBytes;							// the maximum size that this heap can grow to, in bytes (allocatable space)

	uint m_numBytesRequested;							// the number of bytes currently requested for allocation from this heap (memory footprint without the overhaed of internal headers and alignment bytes)
	uint m_numBytesAllocated;							// the number of bytes currently allocated from this heap (actual memory footprint of all active allocations including internal headers and alignment bytes)

	uint m_allocatedBytesByTag[MA_MAX];					// the number of bytes allocated from this heap - organized by usage tag
	uint m_peekAllocatedBytesByTag[MA_MAX];				// the peek number of bytes allocated from this heap - organized by usage tag
	int m_numAllocationsByTag[MA_MAX];					// the number of allocations - organized by usage tag

	int m_numAllocations;								// the number of allocations made from this heap that are still active (have not been freed)
	int m_numZeroLengthAllocations;						// the number of zero length allocations made from this heap

	uint m_largestFreeSmallBlockOffset;					// small block table offset of the largest free small block available (block that is part of m_smallFreeBlocks[] table)

	uint m_flags;										// flags that describe various properties of this heap

	byte mDebugID;										// ID that identifies heap. Needed to absolutely identify a heap.
	char m_name[20];									// for debug display

	void ResetValues( );								// resets the data members to their pre-initialized state

	void BuildLargeFreeRangeReuseList( );				// builds the m_largeFreeRangeReuseList linked-list from the m_largeFreeRangeStorage[] array.

	byte *AllocateMemory( uint sizeBytes, int debugTag, bool align16Flag );	// performs memory allocation for the given number of bytes from this heap

	rvMemoryBlock_s *AllocateMemorySmallBlock( uint sizeBytes );	// perform a small block allocation - try to use an existing free block pointed to by the table.
	rvMemoryBlock_s *AllocateBlockFromTable( uint smallBlockTableOffset, uint sizeBytes );	// allocates the a small memory block from the free block table and partitions it, if necessary, into an allocated chunk and a free chunk (which remains in the m_smallFreeBlocks[] table).
	rvMemoryBlock_s *AllocateMemoryLargeBlock( uint sizeBytes );	// perform a large block allocation - try to use an existing free block from within the large free block linked-list.

	void TestLargestFreeSmallBlockOffset( uint smallBlockTableOffset );	// checks to see if the largest free small block has moved down

	void FixUpNextBlocksPrev( byte *newBlock, uint blockSize );	// fixes up the previous pointer of the memory block that lies immediately past the given newBlock, if it remains on the same page.

	void AddFreeBlock( rvFreeMemoryBlock_s *freeBlock );	// adds the given free block to the small allocation table or large allocation list.
	void RemoveFreeBlock( rvFreeMemoryBlock_s *freeBlock );	// removes the given block from the small allocation table or large allocation list.

	void *PageCommit( uint numDesiredPages );				// commits the given number of contiguous pages to physical memory 
	void PageUncommit( byte *pageAddress, uint numPages );	// uncommits the given range of contiguous pages back to physical memory

	void RemoveFromSmallFreeRangeList( uint pageOffset, uint freeBlockPageCount );					// removes the given page range from the m_smallFreePageRangeLists[]
	void RemoveFromLargeFreeRangeList( uint pageOffset, uint &startPageOffset, uint &pageCount );	// removes the given page range from the m_largeFreePageRangeList

	void *CommitPageRange( uint startPageOffset, uint numPages );	// commits the given range of pages and flags them as committed within the m_pageCommitStateTable array
	void UncommitPageRange( uint startPageOffset, uint numPages );	// uncommits the given range of pages and clears their flags within the m_pageCommitStateTable array

	void EnterHeapCriticalSection();					// enters this heap's critical section
	void ExitHeapCriticalSection();						// exits this heap's critical section

#ifdef _DETECT_BLOCK_MERGE_PROBLEMS
	void TestFreeBlockValidity();						// test for free memory block merge problems
#endif

	friend class rvHeapArena;							// give the rvHeapArena class access to the following methods
	// {
	ID_INLINE void SetArena( rvHeapArena *arena );		// sets the arena this heap is associated with
	ID_INLINE void SetNext( rvHeap *next );				// sets the next heap associated with the same arena
	ID_INLINE rvHeap *GetNext( );						// returns the next heap associated with the same arena
	// }
};

// IsInitialized
// 
// returns: true if this heap is currently initialized, and not shutdown
ID_INLINE bool rvHeap::IsInitialized( ) const
{
	return m_arena != NULL;
}

// Allocate
// 
// allocate memory
void *rvHeap::Allocate( uint sizeBytes, int debugTag )
{
	return AllocateMemory( sizeBytes, debugTag, false );
}

// Allocate16
//
// allocate memory aligned on a 16-byte boundary
void *rvHeap::Allocate16( uint sizeBytes, int debugTag )
{
	return AllocateMemory( sizeBytes, debugTag, true );
}

// DoesAllocBelong
//
// returns: true if the given address was allocated from this heap, false otherwise
ID_INLINE bool rvHeap::DoesAllocBelong( void  *allocBase )
{
	return allocBase >= m_zeroLengthAllocPage && allocBase < m_heapStorageStart + m_heapRangeBytes;
}

// GetArena
//
// returns: the arena this heap is associated with
ID_INLINE rvHeapArena *rvHeap::GetArena( )
{
	return m_arena;
}

// GetNext
//
// returns: the next heap belonging to the same arena
ID_INLINE rvHeap *rvHeap::GetNext( )
{
	return m_next;
}

// PushCurrent
//  
// makes this heap the new top of stack for its associated arena, thus making it current.
ID_INLINE void rvHeap::PushCurrent( )
{
	m_arena->Push( *this );
}

// PopCurrent
//
// pops this heap, or any other heap, from the top of stack of this heap's associated arena, 
// thus making its predecessor current
ID_INLINE void rvHeap::PopCurrent( )
{
	m_arena->Pop( );
}

// GetCommittedSize
//
// returns: the number of physical bytes this heap is currently using (total bytes of all committed pages)
ID_INLINE uint rvHeap::GetCommittedSize( ) const
{
	return m_committedSizeBytes;
}

// GetMaxSize
// 
// returns the maximum size that this heap can grow to, in bytes
ID_INLINE uint rvHeap::GetMaxSize( ) const
{
	return m_maxHeapSizeBytes;
}

// GetBytesAllocated
//
// returns: the number of bytes currently allocated from this heap 
//			(actual memory footprint of all active allocations 
//			including internal headers and alignment bytes)
//
ID_INLINE uint rvHeap::GetBytesAllocated( ) const
{
	return m_numBytesAllocated;
}

// GetBytesRequested
//
// returns: the number of bytes currently requested from this heap
//			(i.e. without overhead)
//
ID_INLINE uint rvHeap::GetBytesRequested( ) const
{
	return m_numBytesRequested;
}

// GetNumAllocations
//
// returns: the number of allocations made from this heap that are still active (have not been freed)
ID_INLINE int rvHeap::GetNumAllocations( ) const
{
	return m_numAllocations;
}

// GetBytesAllocatedByTag
//
// returns: the number of bytes currently allocated from this heap with the given allocation tag 
//			(actual memory footprint of all active allocations including internal headers and 
//			alignment bytes).
ID_INLINE uint rvHeap::GetBytesAllocatedByTag( Mem_Alloc_Types_t allocType ) const
{
	assert( allocType < MA_MAX );
	return m_allocatedBytesByTag[(uint) allocType];
}

// GetPeekBytesAllocatedByTag
//
// returns: the peek number of bytes allocated from this heap with the given allocation tag (actual 
//			memory footprint of all active allocations including internal headers and alignment bytes)
ID_INLINE uint rvHeap::GetPeekBytesAllocatedByTag( Mem_Alloc_Types_t allocType ) const
{
	assert( allocType < MA_MAX );
	return m_peekAllocatedBytesByTag[(uint) allocType];
}

// GetNumAllocationsByTag
// 
// returns: the current number of allocation from this heap with the given allocation tag 
ID_INLINE int rvHeap::GetNumAllocationsByTag( Mem_Alloc_Types_t allocType ) const
{
	assert( allocType < MA_MAX );
	return m_numAllocationsByTag[(uint) allocType];
}

// IsWriteCombine
//
// returns: true if memory allocated from this heap has a write-combine cache policy, false otherwise
ID_INLINE bool rvHeap::IsWriteCombine( ) const
{
	return (m_flags & rvHeapFlagWriteCombine) != 0;
}

// IsNoCache
//
// returns: true if memory allocated from this heap has a no-cache policy, false otherwise
ID_INLINE bool rvHeap::IsNoCache( ) const
{
	return (m_flags & rvHeapFlagNoCache) != 0;
}

// SetArena
//
// sets the arena this heap is associated with
ID_INLINE void rvHeap::SetArena( rvHeapArena *arena )
{
	m_arena = arena;
}

// SetNext
// 
// sets the next heap associated with the same arena
ID_INLINE void rvHeap::SetNext( rvHeap *next )
{
	m_next = next;
}

extern void Mem_FragmentationStats_f( const class idCmdArgs &args );

#endif	// #ifndef __RV_HEAP_H__
