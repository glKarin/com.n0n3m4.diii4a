
#include "../idlib/precompiled.h"
#pragma hdrstop

// RAVEN BEGIN
// jsinger: attempt to eliminate cross-DLL allocation issues
#ifdef RV_UNIFIED_ALLOCATOR
void *(*Memory::mAllocator)(size_t size);
void (*Memory::mDeallocator)(void *ptr);
size_t (*Memory::mMSize)(void *ptr);
bool Memory::sOK = true;

void Memory::Error(const char *errStr)
{
	// If you land here it's because you allocated dynamic memory in a DLL
	// before the Memory system was initialized and this will lead to instability
	// later.  We can't assert here since that will cause the DLL to fail loading
	// so set a break point on the BreakHere=0xdeadbeef; line in order to debug
	int BreakHere;
	BreakHere=0xdeadbeef;

	if(common != NULL)
	{
		// this can't be an error or the game will fail to load the DLL and crash.
		// While a crash is the desired behavior, a crash without any meaningful
		// message to the user indicating what went wrong is not desired.
		common->Warning(errStr);
	}
	else
	{
		// if common isn't initialized then we set a flag so that we can notify once it
		// gets initialized
		sOK = false;	
	}
}
#endif
// RAVEN END
#ifndef _RV_MEM_SYS_SUPPORT

#ifndef USE_LIBC_MALLOC
	#ifdef _XBOX
		#define USE_LIBC_MALLOC		1
	#else
// RAVEN BEGIN
// scork:  changed this to a 1, since 0 will crash Radiant if you BSP large maps ~3 times because of high watermark-mem that never gets freed.
		#define USE_LIBC_MALLOC		1
// RAVEN END
	#endif
#endif

#ifndef CRASH_ON_STATIC_ALLOCATION
//	#define CRASH_ON_STATIC_ALLOCATION
#endif

// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
inline
void *local_malloc(size_t size)
{
#ifndef _XENON
// RAVEN BEGIN
// jsinger: attempt to eliminate cross-DLL allocation issues
	#ifdef RV_UNIFIED_ALLOCATOR
		return Memory::Allocate(size);
	#else
		void *addr = malloc(size);
		if( !addr && size ) {
			common->FatalError( "Out of memory" );
		}
		return( addr );
	#endif  // RV_UNIFIED_ALLOCATOR
// RAVEN END
#else
#endif
}

inline
void local_free(void *ptr)
{
#ifndef _XENON
// RAVEN BEGIN
// jsinger: attempt to eliminate cross-DLL allocation issues
	#ifdef RV_UNIFIED_ALLOCATOR
		Memory::Free(ptr);
	#else
		return free(ptr);
	#endif  // RV_UNIFIED_ALLOCATOR
// RAVEN END
#else
#endif
}
// RAVEN END

//===============================================================
//
//	idHeap
//
//===============================================================

// RAVEN BEGIN
// amccarthy:  Added space in headers in debug for allocation tag storage
#ifdef _DEBUG
#define SMALL_HEADER_SIZE		( (int) ( sizeof( byte ) + sizeof( byte )+ sizeof( byte ) ) )
#define MEDIUM_HEADER_SIZE		( (int) ( sizeof( mediumHeapEntry_s ) + sizeof( byte )+ sizeof( byte ) ) )
#define LARGE_HEADER_SIZE		( (int) ( sizeof( dword * ) + sizeof( byte )+ sizeof( byte ) ) )

#define MALLOC_HEADER_SIZE		( (int) ( 11*sizeof( byte ) + sizeof( byte ) + sizeof( dword )))
#else
#define SMALL_HEADER_SIZE		( (int) ( sizeof( byte ) + sizeof( byte ) ) )
#define MEDIUM_HEADER_SIZE		( (int) ( sizeof( mediumHeapEntry_s ) + sizeof( byte ) ) )
#define LARGE_HEADER_SIZE		( (int) ( sizeof( dword * ) + sizeof( byte ) ) )
#endif
// RAVEN END

#define ALIGN_SIZE( bytes )		( ( (bytes) + ALIGN - 1 ) & ~(ALIGN - 1) )
#define SMALL_ALIGN( bytes )	( ALIGN_SIZE( (bytes) + SMALL_HEADER_SIZE ) - SMALL_HEADER_SIZE )
#define MEDIUM_SMALLEST_SIZE	( ALIGN_SIZE( 256 ) + ALIGN_SIZE( MEDIUM_HEADER_SIZE ) )

// RAVEN BEGIN
// amccarthy: Allocation tag tracking and reporting
#ifdef _DEBUG
int OutstandingXMallocSize=0;
int OutstandingXMallocTagSize[MA_MAX];
int PeakXMallocTagSize[MA_MAX];
int CurrNumAllocations[MA_MAX];


// Descriptions that go with each tag.  When updating the tag enum in Heap.h please 
// update this list as well.
// (also update the list in rvHeap.cpp)
char *TagNames[] = {
	"none",
	"New operation",
	"default",
	"Lexer",
	"Parser",
	"AAS routing",
	"Class",
	"Script program",
	"Collision Model",
	"CVar",
	"Decl System",
	"File System",
	"Images",
	"Materials",
	"Models",
	"Fonts",
	"Main renderer",
	"Vertex data",
	"Sound",
	"Window",
	"Event loop",
	"Math - Matrices and vectors",
	"Animation",
	"Dynamic Blocks",
	"Strings",
	"GUI",
	"Effects",
	"Entities",
	"Physics",
	"AI",
	"Network",
	"Not Used"
};

// jnewquist: Detect when the tag descriptions are out of sync with the enums.
template<int X>
class TagTableCheck
{
private:
	TagTableCheck();
};

template<>
class TagTableCheck<1>
{
};

// An error here means you need to synchronize TagNames and Mem_Alloc_Types_t 
TagTableCheck<sizeof(TagNames)/sizeof(char*) == MA_MAX> TagTableCheckedHere;
// An error here means there are too many tags.  No more than 32! 
TagTableCheck<MA_DO_NOT_USE<32> TagMaxCheckedHere;

void PrintOutstandingMemAlloc()
{
	
	int i;
	unsigned long totalOutstanding = 0;
	for (i=0;i<MA_MAX;i++)
	{
		if (OutstandingXMallocTagSize[i] || PeakXMallocTagSize[i])
		{
			idLib::common->Printf("%-30s peak %9d curr %9d\n",TagNames[i],PeakXMallocTagSize[i],OutstandingXMallocTagSize[i]);
			totalOutstanding += OutstandingXMallocTagSize[i];
		}
	}
	idLib::common->Printf("Mem_Alloc Outstanding: %d\n",totalOutstanding);
	
}

const char *GetMemAllocStats(int tag, int &num, int &size, int &peak)
{
	num = CurrNumAllocations[tag];
	size = OutstandingXMallocTagSize[tag];
	peak = PeakXMallocTagSize[tag];
	return TagNames[tag];
}
#endif


/*
=================
Mem_ShowMemAlloc_f
=================
*/
// amccarthy: print out outstanding mem_allocs.
void Mem_ShowMemAlloc_f( const idCmdArgs &args ) {

#ifdef _DEBUG
	PrintOutstandingMemAlloc();
#endif
}

// jnewquist: memory tag stack for new/delete
#if defined(_DEBUG) && !defined(ENABLE_INTEL_SMP)
MemScopedTag* MemScopedTag::mTop = NULL;
#endif

//RAVEN END

class idHeap {

public:
					idHeap( void );
					~idHeap( void );				// frees all associated data
	void			Init( void );					// initialize
//RAVEN BEGIN
//amccarthy:  Added allocation tag
	void *			Allocate( const dword bytes, byte tag );	// allocate memory
//RAVEN END
	void			Free( void *p );				// free memory
//RAVEN BEGIN
//amccarthy:  Added allocation tag
	void *			Allocate16( const dword bytes, byte tag );// allocate 16 byte aligned memory
//RAVEN END
	void			Free16( void *p );				// free 16 byte aligned memory
	dword			Msize( void *p );				// return size of data block
	void			Dump( void  );

	void 			AllocDefragBlock( void );		// hack for huge renderbumps

private:

	enum {
		ALIGN = 8									// memory alignment in bytes
	};

	enum {
		INVALID_ALLOC	= 0xdd,
		SMALL_ALLOC		= 0xaa,						// small allocation
		MEDIUM_ALLOC	= 0xbb,						// medium allocaction
		LARGE_ALLOC		= 0xcc						// large allocaction
	};

	struct page_s {									// allocation page
		void *				data;					// data pointer to allocated memory
		dword				dataSize;				// number of bytes of memory 'data' points to
		page_s *			next;					// next free page in same page manager
		page_s *			prev;					// used only when allocated
		dword				largestFree;			// this data used by the medium-size heap manager
		void *				firstFree;				// pointer to first free entry
	};

	struct mediumHeapEntry_s {
		page_s *			page;					// pointer to page
		dword				size;					// size of block
		mediumHeapEntry_s *	prev;					// previous block
		mediumHeapEntry_s *	next;					// next block
		mediumHeapEntry_s *	prevFree;				// previous free block
		mediumHeapEntry_s *	nextFree;				// next free block
		dword				freeBlock;				// non-zero if free block
	};

	// variables
	void *			smallFirstFree[256/ALIGN+1];	// small heap allocator lists (for allocs of 1-255 bytes)
	page_s *		smallCurPage;					// current page for small allocations
	dword			smallCurPageOffset;				// byte offset in current page
	page_s *		smallFirstUsedPage;				// first used page of the small heap manager

	page_s *		mediumFirstFreePage;			// first partially free page
	page_s *		mediumLastFreePage;				// last partially free page
	page_s *		mediumFirstUsedPage;			// completely used page

	page_s *		largeFirstUsedPage;				// first page used by the large heap manager

	page_s *		swapPage;

	dword			pagesAllocated;					// number of pages currently allocated
	dword			pageSize;						// size of one alloc page in bytes

	dword			pageRequests;					// page requests
	dword			OSAllocs;						// number of allocs made to the OS

	int				c_heapAllocRunningCount;

	void			*defragBlock;					// a single huge block that can be allocated
													// at startup, then freed when needed

	// methods
	page_s *		AllocatePage( dword bytes );	// allocate page from the OS
	void			FreePage( idHeap::page_s *p );	// free an OS allocated page

//RAVEN BEGIN
//amccarthy:  Added allocation tags
	void *			SmallAllocate( dword bytes, byte tag );	// allocate memory (1-255 bytes) from small heap manager
	void			SmallFree( void *ptr );			// free memory allocated by small heap manager

	void *			MediumAllocateFromPage( idHeap::page_s *p, dword sizeNeeded, byte tag );
	void *			MediumAllocate( dword bytes, byte tag );	// allocate memory (256-32768 bytes) from medium heap manager
	void			MediumFree( void *ptr );		// free memory allocated by medium heap manager

	void *			LargeAllocate( dword bytes, byte tag );	// allocate large block from OS directly
	void			LargeFree( void *ptr );			// free memory allocated by large heap manager
//RAVEN END
	void			ReleaseSwappedPages( void );
	void			FreePageReal( idHeap::page_s *p );
};


/*
================
idHeap::Init
================
*/
void idHeap::Init () {
	OSAllocs			= 0;
	pageRequests		= 0;
	pageSize			= 65536 - sizeof( idHeap::page_s );
	pagesAllocated		= 0;								// reset page allocation counter

	largeFirstUsedPage	= NULL;								// init large heap manager
	swapPage			= NULL;

	memset( smallFirstFree, 0, sizeof(smallFirstFree) );	// init small heap manager
	smallFirstUsedPage	= NULL;
	smallCurPage		= AllocatePage( pageSize );
	assert( smallCurPage );
	smallCurPageOffset	= SMALL_ALIGN( 0 );

	defragBlock = NULL;

	mediumFirstFreePage	= NULL;								// init medium heap manager
	mediumLastFreePage	= NULL;
	mediumFirstUsedPage	= NULL;

	c_heapAllocRunningCount = 0;

//RAVEN BEGIN
//amccarthy:  initalize allocation tracking
#ifdef _DEBUG
	OutstandingXMallocSize=0;
	int i;
	for (i=0;i<MA_MAX;i++)
	{
		OutstandingXMallocTagSize[i] = 0;
		PeakXMallocTagSize[i] = 0;
		CurrNumAllocations[i] = 0;
	}
#endif
//RAVEN END
}

/*
================
idHeap::idHeap
================
*/
idHeap::idHeap( void ) {
	Init();
}

/*
================
idHeap::~idHeap

  returns all allocated memory back to OS
================
*/
idHeap::~idHeap( void ) {

	idHeap::page_s	*p;

	if ( smallCurPage ) {
		FreePage( smallCurPage );			// free small-heap current allocation page
	}
	p = smallFirstUsedPage;					// free small-heap allocated pages 
	while( p ) {
		idHeap::page_s *next = p->next;
		FreePage( p );
		p= next;
	}

	p = largeFirstUsedPage;					// free large-heap allocated pages
	while( p ) {
		idHeap::page_s *next = p->next;
		FreePage( p );
		p = next;
	}

	p = mediumFirstFreePage;				// free medium-heap allocated pages
	while( p ) {
		idHeap::page_s *next = p->next;
		FreePage( p );
		p = next;
	}

	p = mediumFirstUsedPage;				// free medium-heap allocated completely used pages
	while( p ) {
		idHeap::page_s *next = p->next;
		FreePage( p );
		p = next;
	}

	ReleaseSwappedPages();			

	if ( defragBlock ) {
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
		local_free( defragBlock );
// RAVEN END
	}

	assert( pagesAllocated == 0 );
}

/*
================
idHeap::AllocDefragBlock
================
*/
void idHeap::AllocDefragBlock( void ) {
	int		size = 0x40000000;

	if ( defragBlock ) {
		return;
	}
	while( 1 ) {
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
		defragBlock = local_malloc( size );
// RAVEN END
		if ( defragBlock ) {
			break;
		}
		size >>= 1;
	}
	idLib::common->Printf( "Allocated a %i mb defrag block\n", size / (1024*1024) );
}

/*
================
idHeap::Allocate
================
*/
//RAVEN BEGIN
//amccarthy:  Added allocation tag
void *idHeap::Allocate( const dword bytes, byte tag ) {
//RAVEN END
	if ( !bytes ) {
		return NULL;
	}
	c_heapAllocRunningCount++;

#if USE_LIBC_MALLOC

//RAVEN BEGIN
//amccarthy:  Added allocation tags for malloc
#ifdef _DEBUG
	byte *p = (byte *)local_malloc( bytes + MALLOC_HEADER_SIZE );
	OutstandingXMallocSize += bytes;
	assert( tag > 0 && tag < MA_MAX);
	OutstandingXMallocTagSize[tag] += bytes;
	if (OutstandingXMallocTagSize[tag] > PeakXMallocTagSize[tag])
	{
		PeakXMallocTagSize[tag] = OutstandingXMallocTagSize[tag];
	}
	CurrNumAllocations[tag]++;
	dword *d = (dword *)p;
	d[0] = bytes;
	byte *ret = p+MALLOC_HEADER_SIZE;
	ret[-1] = tag;
	return ret;
#else
	return local_malloc( bytes);
#endif

//RAVEN END

	
#else
//RAVEN BEGIN
//amccarthy:  Added allocation tag
	if ( !(bytes & ~255) ) {
		return SmallAllocate( bytes, tag );
	}
	if ( !(bytes & ~32767) ) {
		return MediumAllocate( bytes, tag );
	}
	return LargeAllocate( bytes, tag );
	
//RAVEN END
#endif
}

/*
================
idHeap::Free
================
*/
void idHeap::Free( void *p ) {
	if ( !p ) {
		return;
	}
	c_heapAllocRunningCount--;

#if USE_LIBC_MALLOC
//RAVEN BEGIN
//amccarthy:  allocation tracking
#ifdef _DEBUG
	byte *ptr = ((byte *)p) - MALLOC_HEADER_SIZE;
	dword size = ((dword*)(ptr))[0];
	byte tag = ((byte *)(p))[-1];
	OutstandingXMallocSize -= size;
	assert( tag > 0 && tag < MA_MAX);
	OutstandingXMallocTagSize[tag] -= size;
	CurrNumAllocations[tag]--;
	local_free( ptr );
#else
	local_free( p );
#endif
//RAVEN END
#else
	switch( ((byte *)(p))[-1] ) {
		case SMALL_ALLOC: {
			SmallFree( p );
			break;
		}
		case MEDIUM_ALLOC: {
			MediumFree( p );
			break;
		}
		case LARGE_ALLOC: {
			LargeFree( p );
			break;
		}
		default: {
			idLib::common->FatalError( "idHeap::Free: invalid memory block (%s)", idLib::sys->GetCallStackCurStr( 4 ) );
			break;
		}
	}
	
#endif
}

/*
================
idHeap::Allocate16
================
*/
//RAVEN BEGIN
//amccarthy:  Added allocation tag
void *idHeap::Allocate16( const dword bytes, byte tag ) {
//RAVEN END
	byte *ptr, *alignedPtr;

//RAVEN BEGIN
//amccarthy: Added allocation tag
#ifdef  _DEBUG
	ptr = (byte *) Allocate(bytes+16, tag);
	alignedPtr = (byte *) ( ( (int) ptr ) + 15 & ~15 );
	int padSize = alignedPtr - ptr;
	if ( padSize == 0 ) {
		alignedPtr += 16;
		padSize += 16;
	}
	*((byte *)(alignedPtr - 1)) = (byte) padSize;

	assert( ( unsigned int )alignedPtr < 0xff000000 );

	return (void *) alignedPtr;
#else
//RAVEN END
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
	ptr = (byte *) local_malloc( bytes + 16 + 4 );
// RAVEN END
	if ( !ptr ) {
		if ( defragBlock ) {
			idLib::common->Printf( "Freeing defragBlock on alloc of %i.\n", bytes );
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
			local_free( defragBlock );
// RAVEN END
			defragBlock = NULL;
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
			ptr = (byte *) local_malloc( bytes + 16 + 4 );			
// RAVEN END
			AllocDefragBlock();
		}
		if ( !ptr ) {
			common->FatalError( "malloc failure for %i", bytes );
		}
	}

	alignedPtr = (byte *) ( ( (int) ptr ) + 15 & ~15 );
  	if ( alignedPtr - ptr < 4 ) {
   		alignedPtr += 16;

   	}
  	*((int *)(alignedPtr - 4)) = (int) ptr;
   	assert( ( unsigned int )alignedPtr < 0xff000000 );
   	return (void *) alignedPtr;

//RAVEN BEGIN
//amccarthy: allocation tracking added for debug xbox only.
#endif
//RAVEN END

}

/*
================
idHeap::Free16
================
*/
void idHeap::Free16( void *p ) {
//RAVEN BEGIN
//amccarthy: allocation tag
#ifdef _DEBUG
	byte* ptr = (byte*)p;
	int padSize = *(ptr-1);
	ptr -= padSize;
	Free( ptr );
#else
	local_free( (void *) *((int *) (( (byte *) p ) - 4)) );
#endif
//RAVEN END
}

/*
================
idHeap::Msize

  returns size of allocated memory block
  p	= pointer to memory block
  Notes:	size may not be the same as the size in the original
			allocation request (due to block alignment reasons).
================
*/
dword idHeap::Msize( void *p ) {

	if ( !p ) {
		return 0;
	}

#if USE_LIBC_MALLOC
	#ifdef _WINDOWS
//RAVEN BEGIN
//amccarthy:  allocation tracking
		#ifdef _DEBUG
			byte *ptr = ((byte *)p) - MALLOC_HEADER_SIZE;
// jsinger: attempt to eliminate cross-DLL allocation issues
			#ifdef RV_UNIFIED_ALLOCATOR
						return Memory::MSize(ptr);
			#else
						return _msize(ptr);
			#endif  // RV_UNIFIED_ALLOCATOR
		#else
			#ifdef RV_UNIFIED_ALLOCATOR
						return Memory::MSize(p);
			#else
						return _msize(p);
			#endif  // RV_UNIFIED_ALLOCATOR
		#endif
//RAVEN END
	#else
		return 0;
	#endif
#else
	switch( ((byte *)(p))[-1] ) {
		case SMALL_ALLOC: {
			return SMALL_ALIGN( ((byte *)(p))[-SMALL_HEADER_SIZE] * ALIGN );
		}
		case MEDIUM_ALLOC: {
			return ((mediumHeapEntry_s *)(((byte *)(p)) - ALIGN_SIZE( MEDIUM_HEADER_SIZE )))->size - ALIGN_SIZE( MEDIUM_HEADER_SIZE );
		}
		case LARGE_ALLOC: {
			return ((idHeap::page_s*)(*((dword *)(((byte *)p) - ALIGN_SIZE( LARGE_HEADER_SIZE )))))->dataSize - ALIGN_SIZE( LARGE_HEADER_SIZE );
		}
		default: {
			idLib::common->FatalError( "idHeap::Msize: invalid memory block (%s)", idLib::sys->GetCallStackCurStr( 4 ) );
			return 0;
		}
	}
#endif
}

/*
================
idHeap::Dump

  dump contents of the heap
================
*/
void idHeap::Dump( void ) {
	idHeap::page_s	*pg;

	for ( pg = smallFirstUsedPage; pg; pg = pg->next ) {
		idLib::common->Printf( "%p  bytes %-8d  (in use by small heap)\n", pg->data, pg->dataSize);
	}

	if ( smallCurPage ) {
		pg = smallCurPage;
		idLib::common->Printf( "%p  bytes %-8d  (small heap active page)\n", pg->data, pg->dataSize );
	}

	for ( pg = mediumFirstUsedPage; pg; pg = pg->next ) {
		idLib::common->Printf( "%p  bytes %-8d  (completely used by medium heap)\n", pg->data, pg->dataSize );
	}

	for ( pg = mediumFirstFreePage; pg; pg = pg->next ) {
		idLib::common->Printf( "%p  bytes %-8d  (partially used by medium heap)\n", pg->data, pg->dataSize );
	}
	
	for ( pg = largeFirstUsedPage; pg; pg = pg->next ) {
		idLib::common->Printf( "%p  bytes %-8d  (fully used by large heap)\n", pg->data, pg->dataSize );
	}

	idLib::common->Printf( "pages allocated : %d\n", pagesAllocated );
}

/*
================
idHeap::FreePageReal

  frees page to be used by the OS
  p	= page to free
================
*/
void idHeap::FreePageReal( idHeap::page_s *p ) {
	assert( p );
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
	::local_free( p );
// RAVEN END
}

/*
================
idHeap::ReleaseSwappedPages

  releases the swap page to OS
================
*/
void idHeap::ReleaseSwappedPages () {
	if ( swapPage ) {
		FreePageReal( swapPage );
	}
	swapPage = NULL;
}

/*
================
idHeap::AllocatePage

  allocates memory from the OS
  bytes	= page size in bytes
  returns pointer to page
================
*/
idHeap::page_s* idHeap::AllocatePage( dword bytes ) {
	idHeap::page_s*	p;

	pageRequests++;

	if ( swapPage && swapPage->dataSize == bytes ) {			// if we've got a swap page somewhere
		p			= swapPage;
		swapPage	= NULL;
	}
	else {
		dword size;

		size = bytes + sizeof(idHeap::page_s);

// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
		p = (idHeap::page_s *) ::local_malloc( size + ALIGN - 1 );
// RAVEN END
		if ( !p ) {
			if ( defragBlock ) {
				idLib::common->Printf( "Freeing defragBlock on alloc of %i.\n", size + ALIGN - 1 );
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
				local_free( defragBlock );
// RAVEN END
				defragBlock = NULL;
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
				p = (idHeap::page_s *) ::local_malloc( size + ALIGN - 1 );			
// RAVEN END
				AllocDefragBlock();
			}
			if ( !p ) {
				common->FatalError( "malloc failure for %i", bytes );
			}
		}

		p->data		= (void *) ALIGN_SIZE( (int)((byte *)(p)) + sizeof( idHeap::page_s ) );
		p->dataSize	= size - sizeof(idHeap::page_s);
		p->firstFree = NULL;
		p->largestFree = 0;
		OSAllocs++;
	}

	p->prev = NULL;
	p->next = NULL;

	pagesAllocated++;
	
	return p;
}

/*
================
idHeap::FreePage

  frees a page back to the operating system
  p	= pointer to page
================
*/
void idHeap::FreePage( idHeap::page_s *p ) {
	assert( p );

	if ( p->dataSize == pageSize && !swapPage ) {			// add to swap list?
		swapPage = p;
	}
	else {
		FreePageReal( p );
	}

	pagesAllocated--;
}

//===============================================================
//
//	small heap code
//
//===============================================================

/*
================
idHeap::SmallAllocate

  allocate memory (1-255 bytes) from the small heap manager
  bytes = number of bytes to allocate
  returns pointer to allocated memory
================
*/
//RAVEN BEGIN
//amccarthy:  Added allocation tag
void *idHeap::SmallAllocate( dword bytes, byte tag ) {
//RAVEN END
	// we need the at least sizeof( dword ) bytes for the free list
	if ( bytes < sizeof( dword ) ) {
		bytes = sizeof( dword );
	}

	// increase the number of bytes if necessary to make sure the next small allocation is aligned
	bytes = SMALL_ALIGN( bytes );

	byte *smallBlock = (byte *)(smallFirstFree[bytes / ALIGN]);
	if ( smallBlock ) {
		dword *link = (dword *)(smallBlock + SMALL_HEADER_SIZE);
//RAVEN BEGIN
//amccarthy:  Added allocation tag
#ifdef _DEBUG
		OutstandingXMallocSize += smallBlock[0];
		assert( tag > 0 && tag < MA_MAX);
		OutstandingXMallocTagSize[tag] += smallBlock[0];
		if (OutstandingXMallocTagSize[tag] > PeakXMallocTagSize[tag])
		{
			PeakXMallocTagSize[tag] = OutstandingXMallocTagSize[tag];
		}
		CurrNumAllocations[tag]++;
		smallBlock[1] = tag;
		smallBlock[2] = SMALL_ALLOC;		// allocation identifier

#else
		
		smallBlock[1] = SMALL_ALLOC;		// allocation identifier
#endif
		
//RAVEN END
		smallFirstFree[bytes / ALIGN] = (void *)(*link);
		return (void *)(link);
	}

	dword bytesLeft = (long)(pageSize) - smallCurPageOffset;
	// if we need to allocate a new page
	if ( bytes >= bytesLeft ) {

		smallCurPage->next	= smallFirstUsedPage;
		smallFirstUsedPage	= smallCurPage;
		smallCurPage		= AllocatePage( pageSize );
		if ( !smallCurPage ) {
			return NULL;
		}
		// make sure the first allocation is aligned
		smallCurPageOffset	= SMALL_ALIGN( 0 );
	}

	smallBlock			= ((byte *)smallCurPage->data) + smallCurPageOffset;
	smallBlock[0]		= (byte)(bytes / ALIGN);		// write # of bytes/ALIGN
//RAVEN BEGIN
//amccarthy:  Added allocation tag
#ifdef _DEBUG
		OutstandingXMallocSize += smallBlock[0];
		assert( tag > 0 && tag < MA_MAX);
		OutstandingXMallocTagSize[tag] += smallBlock[0];
		if (OutstandingXMallocTagSize[tag] > PeakXMallocTagSize[tag])
		{
			PeakXMallocTagSize[tag] = OutstandingXMallocTagSize[tag];
		}
		CurrNumAllocations[tag]++;
		smallBlock[1] = tag;
		smallBlock[2] = SMALL_ALLOC;		// allocation identifier

#else
		
		smallBlock[1] = SMALL_ALLOC;		// allocation identifier
#endif
//RAVEN END
	smallCurPageOffset  += bytes + SMALL_HEADER_SIZE;	// increase the offset on the current page
	return ( smallBlock + SMALL_HEADER_SIZE );			// skip the first two bytes
}

/*
================
idHeap::SmallFree

  frees a block of memory allocated by SmallAllocate() call
  data = pointer to block of memory
================
*/
void idHeap::SmallFree( void *ptr ) {
	((byte *)(ptr))[-1] = INVALID_ALLOC;

//RAVEN BEGIN
//amccarthy:  allocation tracking
#ifdef _DEBUG
	byte tag = ((byte *)(ptr))[-2];
	byte size = ((byte *)(ptr))[-3];
	OutstandingXMallocSize -= size;
	assert( tag > 0 && tag < MA_MAX);
	OutstandingXMallocTagSize[tag] -= size;
	CurrNumAllocations[tag]--;
#endif
//RAVEN END

	byte *d = ( (byte *)ptr ) - SMALL_HEADER_SIZE;
	dword *dt = (dword *)ptr;
	// index into the table with free small memory blocks
	dword ix = *d;

	// check if the index is correct
	if ( ix > (256 / ALIGN) ) {
		idLib::common->FatalError( "SmallFree: invalid memory block" );
	}

	*dt = (dword)smallFirstFree[ix];	// write next index
	smallFirstFree[ix] = (void *)d;		// link
}

//===============================================================
//
//	medium heap code
//
//	Medium-heap allocated pages not returned to OS until heap destructor
//	called (re-used instead on subsequent medium-size malloc requests).
//
//===============================================================

/*
================
idHeap::MediumAllocateFromPage

  performs allocation using the medium heap manager from a given page
  p				= page
  sizeNeeded	= # of bytes needed
  returns pointer to allocated memory
================
*/
//RAVEN BEGIN
//amccarthy:  Added allocation tag
void *idHeap::MediumAllocateFromPage( idHeap::page_s *p, dword sizeNeeded, byte tag ) {
//RAVEN END

	mediumHeapEntry_s	*best,*nw = NULL;
	byte				*ret;

	best = (mediumHeapEntry_s *)(p->firstFree);			// first block is largest

	assert( best );
	assert( best->size == p->largestFree );
	assert( best->size >= sizeNeeded );

	// if we can allocate another block from this page after allocating sizeNeeded bytes
	if ( best->size >= (dword)( sizeNeeded + MEDIUM_SMALLEST_SIZE ) ) {
		nw = (mediumHeapEntry_s *)((byte *)best + best->size - sizeNeeded);
		nw->page		= p;
		nw->prev		= best;
		nw->next		= best->next;
		nw->prevFree	= NULL;
		nw->nextFree	= NULL;
		nw->size		= sizeNeeded;
		nw->freeBlock	= 0;			// used block
		if ( best->next ) {
			best->next->prev = nw;
		}
		best->next	= nw;
		best->size	-= sizeNeeded;
		
		p->largestFree = best->size;
	}
	else {
		if ( best->prevFree ) {
			best->prevFree->nextFree = best->nextFree;
		}
		else {
			p->firstFree = (void *)best->nextFree;
		}
		if ( best->nextFree ) {
			best->nextFree->prevFree = best->prevFree;
		}

		best->prevFree  = NULL;
		best->nextFree  = NULL;
		best->freeBlock = 0;			// used block
		nw = best;

		p->largestFree = 0;
	}

	ret		= (byte *)(nw) + ALIGN_SIZE( MEDIUM_HEADER_SIZE );
//RAVEN BEGIN
//amccarthy:  Added allocation tag
#ifdef _DEBUG
	OutstandingXMallocSize += nw->size;
	assert( tag > 0 && tag < MA_MAX);
	OutstandingXMallocTagSize[tag] += nw->size;
	if (OutstandingXMallocTagSize[tag] > PeakXMallocTagSize[tag])
	{
		PeakXMallocTagSize[tag] = OutstandingXMallocTagSize[tag];
	}
	CurrNumAllocations[tag]++;
	ret[-2] = tag;
#endif
//RAVEN END
	ret[-1] = MEDIUM_ALLOC;		// allocation identifier

	return (void *)(ret);
}

/*
================
idHeap::MediumAllocate

  allocate memory (256-32768 bytes) from medium heap manager
  bytes	= number of bytes to allocate
  returns pointer to allocated memory
================
*/
//RAVEN BEGIN
//amccarthy:  Added allocation tag
void *idHeap::MediumAllocate( dword bytes, byte tag ) {
//RAVEN END
	idHeap::page_s		*p;
	void				*data;

	dword sizeNeeded = ALIGN_SIZE( bytes ) + ALIGN_SIZE( MEDIUM_HEADER_SIZE );

	// find first page with enough space
	for ( p = mediumFirstFreePage; p; p = p->next ) {
		if ( p->largestFree >= sizeNeeded ) {
			break;
		}
	}

	if ( !p ) {								// need to allocate new page?
		p = AllocatePage( pageSize );
		if ( !p ) {
			return NULL;					// malloc failure!
		}
		p->prev		= NULL;
		p->next		= mediumFirstFreePage;
		if (p->next) {
			p->next->prev = p;
		}
		else {
			mediumLastFreePage	= p;
		}

		mediumFirstFreePage		= p;
		
		p->largestFree	= pageSize;
		p->firstFree	= (void *)p->data;

		mediumHeapEntry_s *e;
		e				= (mediumHeapEntry_s *)(p->firstFree);
		e->page			= p;
		// make sure ((byte *)e + e->size) is aligned
		e->size			= pageSize & ~(ALIGN - 1);
		e->prev			= NULL;
		e->next			= NULL;
		e->prevFree		= NULL;
		e->nextFree		= NULL;
		e->freeBlock	= 1;
	}

//RAVEN BEGIN
//amccarthy:  Added allocation tag
	data = MediumAllocateFromPage( p, sizeNeeded, tag );		// allocate data from page
//RAVEN END

    // if the page can no longer serve memory, move it away from free list
	// (so that it won't slow down the later alloc queries)
	// this modification speeds up the pageWalk from O(N) to O(sqrt(N))
	// a call to free may swap this page back to the free list

	if ( p->largestFree < MEDIUM_SMALLEST_SIZE ) {
		if ( p == mediumLastFreePage ) {
			mediumLastFreePage = p->prev;
		}

		if ( p == mediumFirstFreePage ) {
			mediumFirstFreePage = p->next;
		}

		if ( p->prev ) {
			p->prev->next = p->next;
		}
		if ( p->next ) {
			p->next->prev = p->prev;
		}

		// link to "completely used" list
		p->prev = NULL;
		p->next = mediumFirstUsedPage;
		if ( p->next ) {
			p->next->prev = p;
		}
		mediumFirstUsedPage = p;
		return data;
	} 

	// re-order linked list (so that next malloc query starts from current
	// matching block) -- this speeds up both the page walks and block walks

	if ( p != mediumFirstFreePage ) {
		assert( mediumLastFreePage );
		assert( mediumFirstFreePage );
		assert( p->prev);

		mediumLastFreePage->next	= mediumFirstFreePage;
		mediumFirstFreePage->prev	= mediumLastFreePage;
		mediumLastFreePage			= p->prev;
		p->prev->next				= NULL;
		p->prev						= NULL;
		mediumFirstFreePage			= p;
	}

	return data;
}

/*
================
idHeap::MediumFree

  frees a block allocated by the medium heap manager
  ptr	= pointer to data block
================
*/
void idHeap::MediumFree( void *ptr ) {
	((byte *)(ptr))[-1] = INVALID_ALLOC;

	mediumHeapEntry_s	*e = (mediumHeapEntry_s *)((byte *)ptr - ALIGN_SIZE( MEDIUM_HEADER_SIZE ));
	idHeap::page_s		*p = e->page;
	bool				isInFreeList;

	isInFreeList = p->largestFree >= MEDIUM_SMALLEST_SIZE;

	assert( e->size );
	assert( e->freeBlock == 0 );

//RAVEN BEGIN
//amccarthy:  allocation tracking
#ifdef _DEBUG
	byte tag = ((byte *)(ptr))[-2];
	dword size = e->size;
	OutstandingXMallocSize -= size;
	assert( tag > 0 && tag < MA_MAX);
	OutstandingXMallocTagSize[tag] -= size;
	CurrNumAllocations[tag]--;
#endif
//RAVEN END

	mediumHeapEntry_s *prev = e->prev;

	// if the previous block is free we can merge
	if ( prev && prev->freeBlock ) {
		prev->size += e->size;
		prev->next = e->next;
		if ( e->next ) {
			e->next->prev = prev;
		}
		e = prev;
	}
	else {
		e->prevFree		= NULL;				// link to beginning of free list
		e->nextFree		= (mediumHeapEntry_s *)p->firstFree;
		if ( e->nextFree ) {
			assert( !(e->nextFree->prevFree) );
			e->nextFree->prevFree = e;
		}

		p->firstFree	= e;
		p->largestFree	= e->size;
		e->freeBlock	= 1;				// mark block as free
	}
			
	mediumHeapEntry_s *next = e->next;

	// if the next block is free we can merge
	if ( next && next->freeBlock ) {
		e->size += next->size;
		e->next = next->next;
		
		if ( next->next ) {
			next->next->prev = e;
		}
		
		if ( next->prevFree ) {
			next->prevFree->nextFree = next->nextFree;
		}
		else {
			assert( next == p->firstFree );
			p->firstFree = next->nextFree;
		}

		if ( next->nextFree ) {
			next->nextFree->prevFree = next->prevFree;
		}
	}

	if ( p->firstFree ) {
		p->largestFree = ((mediumHeapEntry_s *)(p->firstFree))->size;
	}
	else {
		p->largestFree = 0;
	}

	// did e become the largest block of the page ?

	if ( e->size > p->largestFree ) {
		assert( e != p->firstFree );
		p->largestFree = e->size;

		if ( e->prevFree ) {
			e->prevFree->nextFree = e->nextFree;
		}
		if ( e->nextFree ) {
			e->nextFree->prevFree = e->prevFree;
		}
		
		e->nextFree = (mediumHeapEntry_s *)p->firstFree;
		e->prevFree = NULL;
		if ( e->nextFree ) {
			e->nextFree->prevFree = e;
		}
		p->firstFree = e;
	}

	// if page wasn't in free list (because it was near-full), move it back there
	if ( !isInFreeList ) {

		// remove from "completely used" list
		if ( p->prev ) {
			p->prev->next = p->next;
		}
		if ( p->next ) {
			p->next->prev = p->prev;
		}
		if ( p == mediumFirstUsedPage ) {
			mediumFirstUsedPage = p->next;
		}

		p->next = NULL;
		p->prev = mediumLastFreePage;

		if ( mediumLastFreePage ) {
			mediumLastFreePage->next = p;
		}
		mediumLastFreePage = p;
		if ( !mediumFirstFreePage ) {
			mediumFirstFreePage = p;
		}
	} 
}

//===============================================================
//
//	large heap code
//
//===============================================================

/*
================
idHeap::LargeAllocate

  allocates a block of memory from the operating system
  bytes	= number of bytes to allocate
  returns pointer to allocated memory
================
*/
//RAVEN BEGIN
//amccarthy:  Added allocation tag
void *idHeap::LargeAllocate( dword bytes, byte tag ) {
//RAVEN END
	idHeap::page_s *p = AllocatePage( bytes + ALIGN_SIZE( LARGE_HEADER_SIZE ) );

	assert( p );

	if ( !p ) {
		return NULL;
	}

	byte *	d	= (byte*)(p->data) + ALIGN_SIZE( LARGE_HEADER_SIZE );
	dword *	dw	= (dword*)(d - ALIGN_SIZE( LARGE_HEADER_SIZE ));
	dw[0]		= (dword)p;				// write pointer back to page table
//RAVEN BEGIN
//amccarthy:  Added allocation tag
#ifdef _DEBUG
	OutstandingXMallocSize += p->dataSize;
	assert( tag > 0 && tag < MA_MAX);
	OutstandingXMallocTagSize[tag] += p->dataSize;
	if (OutstandingXMallocTagSize[tag] > PeakXMallocTagSize[tag])
	{
		PeakXMallocTagSize[tag] = OutstandingXMallocTagSize[tag];
	}
	CurrNumAllocations[tag]++;
	d[-2]		= tag;
#endif
//RAVEN END
	d[-1]		= LARGE_ALLOC;			// allocation identifier

	// link to 'large used page list'
	p->prev = NULL;
	p->next = largeFirstUsedPage;
	if ( p->next ) {
		p->next->prev = p;
	}
	largeFirstUsedPage = p;

	return (void *)(d);
}

/*
================
idHeap::LargeFree

  frees a block of memory allocated by the 'large memory allocator'
  p	= pointer to allocated memory
================
*/
void idHeap::LargeFree( void *ptr) {
	idHeap::page_s*	pg;

	((byte *)(ptr))[-1] = INVALID_ALLOC;

	// get page pointer
	pg = (idHeap::page_s *)(*((dword *)(((byte *)ptr) - ALIGN_SIZE( LARGE_HEADER_SIZE ))));

	//RAVEN BEGIN
//amccarthy:  allocation tracking
#ifdef _DEBUG
	byte tag = ((byte *)(ptr))[-2];
	dword size = pg->dataSize;
	OutstandingXMallocSize -= size;
	assert( tag > 0 && tag < MA_MAX);
	OutstandingXMallocTagSize[tag] -= size;
	CurrNumAllocations[tag]--;
#endif
//RAVEN END

	// unlink from doubly linked list
	if ( pg->prev ) {
		pg->prev->next = pg->next;
	}
	if ( pg->next ) {
		pg->next->prev = pg->prev;
	}
	if ( pg == largeFirstUsedPage ) {
		largeFirstUsedPage = pg->next;
	}
	pg->next = pg->prev = NULL;

	FreePage(pg);
}

//===============================================================
//
//	memory allocation all in one place
//
//===============================================================

#undef new

static idHeap *			mem_heap = NULL;
static memoryStats_t	mem_total_allocs = { 0, 0x0fffffff, -1, 0 };
static memoryStats_t	mem_frame_allocs;
static memoryStats_t	mem_frame_frees;

/*
==================
Mem_ClearFrameStats
==================
*/
void Mem_ClearFrameStats( void ) {
	mem_frame_allocs.num = mem_frame_frees.num = 0;
	mem_frame_allocs.minSize = mem_frame_frees.minSize = 0x0fffffff;
	mem_frame_allocs.maxSize = mem_frame_frees.maxSize = -1;
	mem_frame_allocs.totalSize = mem_frame_frees.totalSize = 0;
}

/*
==================
Mem_GetFrameStats
==================
*/
void Mem_GetFrameStats( memoryStats_t &allocs, memoryStats_t &frees ) {
	allocs = mem_frame_allocs;
	frees = mem_frame_frees;
}

/*
==================
Mem_GetStats
==================
*/
void Mem_GetStats( memoryStats_t &stats ) {
	stats = mem_total_allocs;
}

/*
==================
Mem_UpdateStats
==================
*/
void Mem_UpdateStats( memoryStats_t &stats, int size ) {
	stats.num++;
	if ( size < stats.minSize ) {
		stats.minSize = size;
	}
	if ( size > stats.maxSize ) {
		stats.maxSize = size;
	}
	stats.totalSize += size;
}

/*
==================
Mem_UpdateAllocStats
==================
*/
void Mem_UpdateAllocStats( int size ) {
	Mem_UpdateStats( mem_frame_allocs, size );
	Mem_UpdateStats( mem_total_allocs, size );
}

/*
==================
Mem_UpdateFreeStats
==================
*/
void Mem_UpdateFreeStats( int size ) {
	Mem_UpdateStats( mem_frame_frees, size );
	mem_total_allocs.num--;
	mem_total_allocs.totalSize -= size;
}


#ifndef ID_DEBUG_MEMORY

/*
==================
Mem_Alloc
==================
*/
// RAVEN BEGIN
// amccarthy: Added allocation tag
void *Mem_Alloc( const int size, byte tag ) {
	if ( !size ) {
		return NULL;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
// jnewquist: send all allocations through one place on the Xenon
		return local_malloc( size );
	}
// amccarthy: Added allocation tag
	void *mem = mem_heap->Allocate( size, tag );
	Mem_UpdateAllocStats( mem_heap->Msize( mem ) );
	return mem;
}
// RAVEN END

/*
==================
Mem_Free
==================
*/
void Mem_Free( void *ptr ) {
	if ( !ptr ) {
		return;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
		local_free( ptr );
// RAVEN END
		return;
	}
	Mem_UpdateFreeStats( mem_heap->Msize( ptr ) );
 	mem_heap->Free( ptr );
}

/*
==================
Mem_Alloc16
==================
*/
// RAVEN BEGIN
// amccarthy: Added allocation tag
void *Mem_Alloc16( const int size, byte tag ) {
	if ( !size ) {
		return NULL;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
// jnewquist: send all allocations through one place on the Xenon
		return local_malloc( size );
	}

// amccarthy: Added allocation tag
	void *mem = mem_heap->Allocate16( size, tag );
	// make sure the memory is 16 byte aligned
	assert( ( ((int)mem) & 15) == 0 );
	return mem;
}
// RAVEN END

/*
==================
Mem_Free16
==================
*/
void Mem_Free16( void *ptr ) {
	if ( !ptr ) {
		return;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
		local_free( ptr );
// RAVEN END
		return;
	}
	// make sure the memory is 16 byte aligned
	assert( ( ((int)ptr) & 15) == 0 );
 	mem_heap->Free16( ptr );
}

/*
==================
Mem_ClearedAlloc
==================
*/
// RAVEN BEGIN
// amccarthy: Added allocation tag
void *Mem_ClearedAlloc( const int size, byte tag ) {
	void *mem = Mem_Alloc( size, tag );
// RAVEN END
	SIMDProcessor->Memset( mem, 0, size );
	return mem;
}

/*
==================
Mem_ClearedAlloc
==================
*/
void Mem_AllocDefragBlock( void ) {
	mem_heap->AllocDefragBlock();
}

/*
==================
Mem_CopyString
==================
*/
char *Mem_CopyString( const char *in ) {
	char	*out;

// RAVEN BEGIN
// amccarthy: Added allocation tag
	out = (char *)Mem_Alloc( strlen(in) + 1, MA_STRING );
// RAVEN END
	strcpy( out, in );
	return out;
}

/*
==================
Mem_Dump_f
==================
*/
void Mem_Dump_f( const idCmdArgs &args ) {
}

/*
==================
Mem_DumpCompressed_f
==================
*/
void Mem_DumpCompressed_f( const idCmdArgs &args ) {
}

/*
==================
Mem_Init
==================
*/
void Mem_Init( void ) {
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_DEFAULT);
// RAVEN END
	mem_heap = new idHeap;
	Mem_ClearFrameStats();
}

/*
==================
Mem_Shutdown
==================
*/
void Mem_Shutdown( void ) {
	idHeap *m = mem_heap;
	mem_heap = NULL;
	delete m;
}

/*
==================
Mem_EnableLeakTest
==================
*/
void Mem_EnableLeakTest( const char *name ) {
}


#else /* !ID_DEBUG_MEMORY */

#undef		Mem_Alloc
#undef		Mem_ClearedAlloc
#undef		Com_ClearedReAlloc
#undef		Mem_Free
#undef		Mem_CopyString
#undef		Mem_Alloc16
#undef		Mem_Free16

#define MAX_CALLSTACK_DEPTH		6

// RAVEN BEGIN
// jnewquist: Add a signature to avoid misinterpreting an early allocation
// size of this struct must be a multiple of 16 bytes
typedef struct debugMemory_s {
	unsigned short			signature;
	unsigned short			lineNumber;
	const char *			fileName;
	int						frameNumber;
	int						size;
	address_t				callStack[MAX_CALLSTACK_DEPTH];
	struct debugMemory_s *	prev;
	struct debugMemory_s *	next;
} debugMemory_t;
// RAVEN END

static debugMemory_t *	mem_debugMemory = NULL;
static char				mem_leakName[256] = "";

/*
==================
Mem_CleanupFileName
==================
*/
const char *Mem_CleanupFileName( const char *fileName ) {
	int i1, i2;
	idStr newFileName;
	static char newFileNames[4][MAX_STRING_CHARS];
	static int index;

	newFileName = fileName;
	newFileName.BackSlashesToSlashes();
	i1 = newFileName.Find( "neo", false );
	if ( i1 >= 0 ) {
		i1 = newFileName.Find( "/", false, i1 );
		newFileName = newFileName.Right( newFileName.Length() - ( i1 + 1 ) );
	}
	while( 1 ) {
		i1 = newFileName.Find( "/../" );
		if ( i1 <= 0 ) {
			break;
		}
		i2 = i1 - 1;
		while( i2 > 1 && newFileName[i2-1] != '/' ) {
			i2--;
		}
		newFileName = newFileName.Left( i2 - 1 ) + newFileName.Right( newFileName.Length() - ( i1 + 4 ) );
	}
	index = ( index + 1 ) & 3;
	strncpy( newFileNames[index], newFileName.c_str(), sizeof( newFileNames[index] ) );
	return newFileNames[index];
}

/*
==================
Mem_Dump
==================
*/
void Mem_Dump( const char *fileName ) {
	int i, numBlocks, totalSize;
	char dump[32], *ptr;
	debugMemory_t *b;
	idStr module, funcName;
	FILE *f;

	f = fopen( fileName, "wb" );
	if ( !f ) {
		return;
	}

	totalSize = 0;
	for ( numBlocks = 0, b = mem_debugMemory; b; b = b->next, numBlocks++ ) {
		ptr = ((char *) b) + sizeof(debugMemory_t);
		totalSize += b->size;
		for ( i = 0; i < (sizeof(dump)-1) && i < b->size; i++) {
			if ( ptr[i] >= 32 && ptr[i] < 127 ) {
				dump[i] = ptr[i];
			} else {
				dump[i] = '_';
			}
		}
		dump[i] = '\0';
		if ( ( b->size >> 10 ) != 0 ) {
			fprintf( f, "size: %6d KB: %s, line: %d [%s], call stack: %s\r\n", ( b->size >> 10 ), Mem_CleanupFileName(b->fileName), b->lineNumber, dump, idLib::sys->GetCallStackStr( b->callStack, MAX_CALLSTACK_DEPTH ) );
		}
		else {
			fprintf( f, "size: %7d B: %s, line: %d [%s], call stack: %s\r\n", b->size, Mem_CleanupFileName(b->fileName), b->lineNumber, dump, idLib::sys->GetCallStackStr( b->callStack, MAX_CALLSTACK_DEPTH ) );
		}
	}

	idLib::sys->ShutdownSymbols();

	fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
	fprintf( f, "%8d KB memory allocated\r\n", ( totalSize >> 10 ) );

	fclose( f );
}



/*
==================
Mem_Dump_f
==================
*/
void Mem_Dump_f( const idCmdArgs &args ) {
	const char *fileName;

	if ( args.Argc() >= 2 ) {
		fileName = args.Argv( 1 );
	}
	else {
		fileName = "memorydump.txt";
	}
	Mem_Dump( fileName );
}

/*
==================
Mem_DumpCompressed
==================
*/

typedef struct allocInfo_s {
	const char *			fileName;
	int						lineNumber;
	int						size;
	int						numAllocs;
	address_t				callStack[MAX_CALLSTACK_DEPTH];
	struct allocInfo_s *	next;
} allocInfo_t;

typedef enum {
	MEMSORT_SIZE,
	MEMSORT_LOCATION,
	MEMSORT_NUMALLOCS,
	MEMSORT_CALLSTACK
} memorySortType_t;

void Mem_DumpCompressed( const char *fileName, memorySortType_t memSort, int sortCallStack, int numFrames ) {
	int numBlocks, totalSize, r, j;
	debugMemory_t *b;
	allocInfo_t *a, *nexta, *allocInfo = NULL, *sortedAllocInfo = NULL, *prevSorted, *nextSorted;
	idStr module, funcName;

	// build list with memory allocations
	totalSize = 0;
	numBlocks = 0;
// RAVEN BEGIN
	nextSorted = NULL;
// RAVEN END

	for ( b = mem_debugMemory; b; b = b->next ) {

		if ( numFrames && b->frameNumber < idLib::frameNumber - numFrames ) {
			continue;
		}

		numBlocks++;
		totalSize += b->size;

		// search for an allocation from the same source location
		for ( a = allocInfo; a; a = a->next ) {
			if ( a->lineNumber != b->lineNumber ) {
				continue;
			}
// RAVEN BEGIN
// dluetscher: removed the call stack info for better consolidation of info and speed of dump
#ifndef _XENON
			for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
				if ( a->callStack[j] != b->callStack[j] ) {
					break;
				}
			}
			if ( j < MAX_CALLSTACK_DEPTH ) {
				continue;
			}
#endif
// RAVEN END
			if ( idStr::Cmp( a->fileName, b->fileName ) != 0 ) {
				continue;
			}
			a->numAllocs++;
			a->size += b->size;
			break;
		}

		// if this is an allocation from a new source location
		if ( !a ) {
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
			a = (allocInfo_t *) ::local_malloc( sizeof( allocInfo_t ) );
// RAVEN END
			a->fileName = b->fileName;
			a->lineNumber = b->lineNumber;
			a->size = b->size;
			a->numAllocs = 1;
			for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
				a->callStack[j] = b->callStack[j];
			}
			a->next = allocInfo;
			allocInfo = a;
		}
	}

	// sort list
	for ( a = allocInfo; a; a = nexta ) {
		nexta = a->next;

		prevSorted = NULL;
		switch( memSort ) {
			// sort on size
			case MEMSORT_SIZE: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->size > nextSorted->size ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on file name and line number
			case MEMSORT_LOCATION: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					r = idStr::Cmp( Mem_CleanupFileName( a->fileName ), Mem_CleanupFileName( nextSorted->fileName ) );
					if ( r < 0 || ( r == 0 && a->lineNumber < nextSorted->lineNumber ) ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on the number of allocations
			case MEMSORT_NUMALLOCS: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->numAllocs > nextSorted->numAllocs ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
			// sort on call stack
			case MEMSORT_CALLSTACK: {
				for ( nextSorted = sortedAllocInfo; nextSorted; nextSorted = nextSorted->next ) {
					if ( a->callStack[sortCallStack] < nextSorted->callStack[sortCallStack] ) {
						break;
					}
					prevSorted = nextSorted;
				}
				break;
			}
		}
		if ( !prevSorted ) {
			a->next = sortedAllocInfo;
			sortedAllocInfo = a;
		}
		else {
			prevSorted->next = a;
			a->next = nextSorted;
		}
	}

// RAVEN BEGIN
// dluetscher: changed xenon version to output anything above 1K to the console 
#ifdef _XENON
	// write list to debug output and console
	for ( a = sortedAllocInfo; a; a = nexta ) {
		nexta = a->next;
		if ( (a->size >> 10) > 0 ) {

			idLib::common->Printf("size: %6d KB, allocs: %5d: %s, line: %d, call stack: %s\r\n",
						(a->size >> 10), a->numAllocs, Mem_CleanupFileName(a->fileName),
								a->lineNumber, idLib::sys->GetCallStackStr( a->callStack, MAX_CALLSTACK_DEPTH ) );

		}
		::local_free( a );
	}

	idLib::sys->ShutdownSymbols();

	idLib::common->Printf("%8d total memory blocks allocated\r\n", numBlocks );
	idLib::common->Printf("%8d KB memory allocated\r\n", ( totalSize >> 10 ) );
#else
// RAVEN END
	FILE *f;

	f = fopen( fileName, "wb" );
	if ( !f ) {
		return;
	}

	// write list to file
	for ( a = sortedAllocInfo; a; a = nexta ) {
		nexta = a->next;
		fprintf( f, "size: %6d KB, allocs: %5d: %s, line: %d, call stack: %s\r\n",
					(a->size >> 10), a->numAllocs, Mem_CleanupFileName(a->fileName),
							a->lineNumber, idLib::sys->GetCallStackStr( a->callStack, MAX_CALLSTACK_DEPTH ) );
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
		::local_free( a );
// RAVEN END
	}

	idLib::sys->ShutdownSymbols();

	fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
	fprintf( f, "%8d KB memory allocated\r\n", ( totalSize >> 10 ) );

	fclose( f );
#endif
}

/*
==================
Mem_DumpCompressed_f
==================
*/
void Mem_DumpCompressed_f( const idCmdArgs &args ) {
	int argNum;
	const char *arg, *fileName;
	memorySortType_t memSort = MEMSORT_LOCATION;
	int sortCallStack = 0, numFrames = 0;

	// get cmd-line options
	argNum = 1;
	arg = args.Argv( argNum );
	while( arg[0] == '-' ) {
		arg = args.Argv( ++argNum );
		if ( idStr::Icmp( arg, "s" ) == 0 ) {
			memSort = MEMSORT_SIZE;
		} else if ( idStr::Icmp( arg, "l" ) == 0 ) {
			memSort = MEMSORT_LOCATION;
		} else if ( idStr::Icmp( arg, "a" ) == 0 ) {
			memSort = MEMSORT_NUMALLOCS;
		} else if ( idStr::Icmp( arg, "cs1" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 2;
		} else if ( idStr::Icmp( arg, "cs2" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 1;
		} else if ( idStr::Icmp( arg, "cs3" ) == 0 ) {
			memSort = MEMSORT_CALLSTACK;
			sortCallStack = 0;
		} else if ( arg[0] == 'f' ) {
			numFrames = atoi( arg + 1 );
		} else {
			idLib::common->Printf( "memoryDumpCompressed [options] [filename]\n"
						"options:\n"
						"  -s     sort on size\n"
						"  -l     sort on location\n"
						"  -a     sort on the number of allocations\n"
						"  -cs1   sort on first function on call stack\n"
						"  -cs2   sort on second function on call stack\n"
						"  -cs3   sort on third function on call stack\n"
						"  -f<X>  only report allocations the last X frames\n"
						"By default the memory allocations are sorted on location.\n"
						"By default a 'memorydump.txt' is written if no file name is specified.\n" );
			return;
		}
		arg = args.Argv( ++argNum );
	}
	if ( argNum >= args.Argc() ) {
		fileName = "memorydump.txt";
	} else {
		fileName = arg;
	}
	Mem_DumpCompressed( fileName, memSort, sortCallStack, numFrames );
}

/*
==================
Mem_AllocDebugMemory
==================
*/
// RAVEN BEGIN
void *Mem_AllocDebugMemory( const int size, const char *fileName, const int lineNumber, const bool align16, byte tag ) {
// RAVEN END
	void *p;
	debugMemory_t *m;

	if ( !size ) {
		return NULL;
	}

	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		// NOTE: set a breakpoint here to find memory allocations before mem_heap is initialized
// RAVEN BEGIN
// jnewquist: send all allocations through one place on the Xenon
		return local_malloc( size );
// RAVEN END
	}

	if ( align16 ) {
// RAVEN BEGIN
		p = mem_heap->Allocate16( size + sizeof( debugMemory_t ), tag );
	}
	else {
		p = mem_heap->Allocate( size + sizeof( debugMemory_t ), tag );
// RAVEN END
	}

	Mem_UpdateAllocStats( size );

	m = (debugMemory_t *) p;
// RAVEN BEGIN
// jnewquist: Add a signature to avoid misinterpreting an early allocation
	m->signature = 0xf00d;
// RAVEN END
	m->fileName = fileName;
	m->lineNumber = lineNumber;
	m->frameNumber = idLib::frameNumber;
	m->size = size;
	m->next = mem_debugMemory;
	m->prev = NULL;
	if ( mem_debugMemory ) {
		mem_debugMemory->prev = m;
	}
	mem_debugMemory = m;
	idLib::sys->GetCallStack( m->callStack, MAX_CALLSTACK_DEPTH );

	return ( ( (byte *) p ) + sizeof( debugMemory_t ) );
}

/*
==================
Mem_FreeDebugMemory
==================
*/
void Mem_FreeDebugMemory( void *p, const char *fileName, const int lineNumber, const bool align16 ) {
	debugMemory_t *m;

	if ( !p ) {
		return;
	}

// RAVEN BEGIN
// jnewquist: Add a signature to avoid misinterpreting an early allocation
	m = (debugMemory_t *) ( ( (byte *) p ) - sizeof( debugMemory_t ) );

	if ( !mem_heap || m->signature != 0xf00d ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		// NOTE: set a breakpoint here to find memory being freed before mem_heap is initialized
// jnewquist: send all allocations through one place on the Xenon
		local_free( p );
		return;
	}

// RAVEN END
	if ( m->size < 0 ) {
		idLib::common->FatalError( "memory freed twice, first from %s, now from %s", idLib::sys->GetCallStackStr( m->callStack, MAX_CALLSTACK_DEPTH ), idLib::sys->GetCallStackCurStr( MAX_CALLSTACK_DEPTH ) );
	}

	Mem_UpdateFreeStats( m->size );

	if ( m->next ) {
		m->next->prev = m->prev;
	}
	if ( m->prev ) {
		m->prev->next = m->next;
	}
	else {
		mem_debugMemory = m->next;
	}

	m->fileName = fileName;
	m->lineNumber = lineNumber;
	m->frameNumber = idLib::frameNumber;
	m->size = -m->size;
	idLib::sys->GetCallStack( m->callStack, MAX_CALLSTACK_DEPTH );

	if ( align16 ) {
 		mem_heap->Free16( m );
	}
	else {
 		mem_heap->Free( m );
	}
}

/*
==================
Mem_Alloc
==================
*/
void *Mem_Alloc( const int size, const char *fileName, const int lineNumber, byte tag ) {
	if ( !size ) {
		return NULL;
	}
	return Mem_AllocDebugMemory( size, fileName, lineNumber, false, tag );
}

/*
==================
Mem_Free
==================
*/
void Mem_Free( void *ptr, const char *fileName, const int lineNumber ) {
	if ( !ptr ) {
		return;
	}
	Mem_FreeDebugMemory( ptr, fileName, lineNumber, false );
}

/*
==================
Mem_Alloc16
==================
*/
void *Mem_Alloc16( const int size, const char *fileName, const int lineNumber, byte tag ) {
	if ( !size ) {
		return NULL;
	}
	void *mem = Mem_AllocDebugMemory( size, fileName, lineNumber, true, tag );
	// make sure the memory is 16 byte aligned
	assert( ( ((int)mem) & 15) == 0 );
	return mem;
}

/*
==================
Mem_Free16
==================
*/
void Mem_Free16( void *ptr, const char *fileName, const int lineNumber ) {
	if ( !ptr ) {
		return;
	}
	// make sure the memory is 16 byte aligned
	assert( ( ((int)ptr) & 15) == 0 );
	Mem_FreeDebugMemory( ptr, fileName, lineNumber, true );
}

/*
==================
Mem_ClearedAlloc
==================
*/
void *Mem_ClearedAlloc( const int size, const char *fileName, const int lineNumber, byte tag ) {
	void *mem = Mem_Alloc( size, fileName, lineNumber, tag );
	SIMDProcessor->Memset( mem, 0, size );
	return mem;
}

/*
==================
Mem_CopyString
==================
*/
char *Mem_CopyString( const char *in, const char *fileName, const int lineNumber ) {
	char	*out;
	
	out = (char *)Mem_Alloc( strlen(in) + 1, fileName, lineNumber );
	strcpy( out, in );
	return out;
}

/*
==================
Mem_Init
==================
*/
void Mem_Init( void ) {
	mem_heap = new idHeap;
}

/*
==================
Mem_Shutdown
==================
*/
void Mem_Shutdown( void ) {

	if ( mem_leakName[0] != '\0' ) {
		Mem_DumpCompressed( va( "%s_leak_size.txt", mem_leakName ), MEMSORT_SIZE, 0, 0 );
		Mem_DumpCompressed( va( "%s_leak_location.txt", mem_leakName ), MEMSORT_LOCATION, 0, 0 );
		Mem_DumpCompressed( va( "%s_leak_cs1.txt", mem_leakName ), MEMSORT_CALLSTACK, 2, 0 );
	}

	idHeap *m = mem_heap;
	mem_heap = NULL;
	delete m;
}

/*
==================
Mem_EnableLeakTest
==================
*/
void Mem_EnableLeakTest( const char *name ) {
	idStr::Copynz( mem_leakName, name, sizeof( mem_leakName ) );
}

// RAVEN BEGIN
// jnewquist: Add Mem_Size to query memory allocation size
/*
==================
Mem_Size
==================
*/
int	Mem_Size( void *ptr ) {
	return mem_heap->Msize(ptr);
}
// RAVEN END

#endif /* !ID_DEBUG_MEMORY */

#endif	// #ifndef _RV_MEM_SYS_SUPPORT
