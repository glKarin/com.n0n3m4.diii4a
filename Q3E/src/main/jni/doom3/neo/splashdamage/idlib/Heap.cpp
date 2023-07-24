// Copyright (C) 2007 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#ifndef USE_LIBC_MALLOC
	#define USE_LIBC_MALLOC 1

	#if USE_LIBC_MALLOC
		#if defined( _WIN32 ) && ( _MSC_VER >= 1300 ) && defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
			#include <crtdbg.h>
		#endif
	#endif
#endif

#pragma warning( push )
#pragma warning( disable: 4267 4312 4311 )


#ifndef CRASH_ON_STATIC_ALLOCATION
//	#define CRASH_ON_STATIC_ALLOCATION
#endif

#if USE_LIBC_MALLOC
// uncomment to turn on all CRT heap debugging... SLOW!!
//#define FULL_CRT_DEBUG	1
#endif

//===============================================================
//
//	idHeap
//
//===============================================================

#define SMALL_HEADER_SIZE		( (int) ( sizeof( byte ) + sizeof( byte ) ) )
#define MEDIUM_HEADER_SIZE		( (int) ( sizeof( mediumHeapEntry_s ) + sizeof( byte ) ) )
#define LARGE_HEADER_SIZE		( (int) ( sizeof( dword * ) + sizeof( byte ) ) )

#define ALIGN_SIZE( bytes )		( ( (bytes) + ALIGN - 1 ) & ~(ALIGN - 1) )
#define SMALL_ALIGN( bytes )	( ALIGN_SIZE( (bytes) + SMALL_HEADER_SIZE ) - SMALL_HEADER_SIZE )
#define MEDIUM_SMALLEST_SIZE	( ALIGN_SIZE( 256 ) + ALIGN_SIZE( MEDIUM_HEADER_SIZE ) )

class idHeap {

public:
					idHeap( void );
					~idHeap( void );				// frees all associated data
	void			Init( void );					// initialize
	void *			Allocate( const size_t bytes );	// allocate memory
	void			Free( void *p );				// free memory
	void *			AllocateAligned( const size_t bytes, align_t align );
	void			FreeAligned( void *p );
	size_t			Msize( void *p );				// return size of data block
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

	void *			SmallAllocate( dword bytes );	// allocate memory (1-255 bytes) from small heap manager
	void			SmallFree( void *ptr );			// free memory allocated by small heap manager

	void *			MediumAllocateFromPage( idHeap::page_s *p, dword sizeNeeded );
	void *			MediumAllocate( dword bytes );	// allocate memory (256-32768 bytes) from medium heap manager
	void			MediumFree( void *ptr );		// free memory allocated by medium heap manager

	void *			LargeAllocate( dword bytes );	// allocate large block from OS directly
	void			LargeFree( void *ptr );			// free memory allocated by large heap manager

	void			ReleaseSwappedPages( void );
	void			FreePageReal( idHeap::page_s *p );
};


/*
================
idHeap::Init
================
*/
void idHeap::Init ( void ) {
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

#if !defined( SD_SDK_BUILD )
	#if USE_LIBC_MALLOC && ( _MSC_VER >= 1300 ) && !defined( _XENON ) && !defined( ID_REDIRECT_NEWDELETE )
		#if defined( _DEBUG )
			#if FULL_CRT_DEBUG
				// check on every allocation
				_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF );
			#else
				// always check intermittently in debug builds so these bugs don't go unnoticed!
				//_CrtSetDbgFlag( _CRTDBG_CHECK_EVERY_1024_DF );
				_CrtSetDbgFlag( 0 );
			#endif
		#endif
	#endif
#endif // SD_SDK_BUILD
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
		free( defragBlock );
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
		defragBlock = malloc( size );
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
void *idHeap::Allocate( const size_t bytes ) {
	if ( !bytes ) {
		return NULL;
	}
	c_heapAllocRunningCount++;

#if USE_LIBC_MALLOC
	return malloc( bytes );
#else
	if ( !(bytes & ~255) ) {
		return SmallAllocate( bytes );
	}
	if ( !(bytes & ~32767) ) {
		return MediumAllocate( bytes );
	}
	return LargeAllocate( bytes );
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
	free( p );
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
idHeap::AllocateAligned
================
*/
void *idHeap::AllocateAligned( const size_t bytes, const align_t align ) {
	byte *ptr, *alignedPtr;

	if ( align != ALIGN_NONE && align != ALIGN_4 && align != ALIGN_8 && align != ALIGN_16 && align != ALIGN_32 && align != ALIGN_64 && align != ALIGN_128 ) {
		idLib::common->FatalError( "idHeap::AllocateAligned: bad alignment %d", align );
	}

	ptr = (byte *) malloc( bytes + align + sizeof( UINT_PTR ) );
	if ( !ptr ) {
		if ( defragBlock ) {
			idLib::common->Printf( "Freeing defragBlock on alloc of %li.\n", bytes );
			free( defragBlock );
			defragBlock = NULL;
			ptr = (byte *) malloc( bytes + align + sizeof( UINT_PTR ) );
			AllocDefragBlock();
		}
		if ( !ptr ) {
#if defined( _WIN32 ) && !defined( _XENON )
			MEMORYSTATUSEX statex;
			statex.dwLength = sizeof( statex );
			GlobalMemoryStatusEx( &statex );
			common->Printf( "\nTotal Physical Memory: %I64d bytes\nAvailable Physical Memory: %I64d bytes\nMemory Utilization: %d %%\n\n", 
				statex.ullTotalPhys, statex.ullAvailPhys, (int)statex.dwMemoryLoad );
#endif
#if defined( SD_PUBLIC_BUILD )
			*( int* )( 0x00000000 ) = 7;
#endif // SD_PUBLIC_BUILD
			common->FatalError( "idHeap::AllocateAligned request for %li bytes aligned at %i failed", bytes, align );
		}
	}
	if ( align == ALIGN_NONE ) {
		alignedPtr = ptr + sizeof( UINT_PTR );
	} else {
		alignedPtr = (byte *) ( ( (UINT_PTR)ptr + ( align - 1 ) ) & ~( align - 1 ) );
		if ( alignedPtr - ptr < sizeof( UINT_PTR ) ) {
			alignedPtr += align;
		}
	}
	*((UINT_PTR *)( (UINT_PTR)alignedPtr - sizeof( UINT_PTR ) )) = (UINT_PTR) ptr;
	return (void *) alignedPtr;
}

/*
================
idHeap::Free
================
*/
void idHeap::FreeAligned( void *p ) {
	free( (void *) *((int *) (( (byte *) p ) - sizeof( uintptr_t ))) );
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
size_t idHeap::Msize( void *p ) {

	if ( !p ) {
		return 0;
	}

#if USE_LIBC_MALLOC
	#ifdef _WIN32
		return _msize( p );
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
	::free( p );
}

/*
================
idHeap::ReleaseSwappedPages

  releases the swap page to OS
================
*/
void idHeap::ReleaseSwappedPages ( void ) {
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

		p = (idHeap::page_s *) ::malloc( size + ALIGN - 1 );
		if ( !p ) {
			if ( defragBlock ) {
				idLib::common->Printf( "Freeing defragBlock on alloc of %i.\n", size + ALIGN - 1 );
				free( defragBlock );
				defragBlock = NULL;
				p = (idHeap::page_s *) ::malloc( size + ALIGN - 1 );			
				AllocDefragBlock();
			}
			if ( !p ) {
#if defined( _WIN32 ) && !defined( _XENON )
				MEMORYSTATUSEX statex;
				statex.dwLength = sizeof( statex );
				GlobalMemoryStatusEx( &statex );
				common->Printf( "\nTotal Physical Memory: %I64d bytes\nAvailable Physical Memory: %I64d bytes\nMemory Utilization: %i %%\n\n", 
					statex.ullTotalPhys, statex.ullAvailPhys, (int)statex.dwMemoryLoad );
#endif
				common->FatalError( "idHeap::AllocatePage request for %i bytes failed", bytes );
			}
		}

		p->data		= (void *) ALIGN_SIZE( (UINT_PTR)((byte *)(p)) + sizeof( idHeap::page_s ) );
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
void *idHeap::SmallAllocate( dword bytes ) {
	// we need the at least sizeof( dword ) bytes for the free list
	if ( bytes < sizeof( dword ) ) {
		bytes = sizeof( dword );
	}

	// increase the number of bytes if necessary to make sure the next small allocation is aligned
	bytes = SMALL_ALIGN( bytes );

	byte *smallBlock = (byte *)(smallFirstFree[bytes / ALIGN]);
	if ( smallBlock ) {
		dword *link = (dword *)(smallBlock + SMALL_HEADER_SIZE);
		smallBlock[1] = SMALL_ALLOC;					// allocation identifier
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
	smallBlock[1]		= SMALL_ALLOC;					// allocation identifier
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
void *idHeap::MediumAllocateFromPage( idHeap::page_s *p, dword sizeNeeded ) {

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
void *idHeap::MediumAllocate( dword bytes ) {
	idHeap::page_s		*p;
	void				*data;

	dword sizeNeeded = ALIGN_SIZE( bytes ) + ALIGN_SIZE( MEDIUM_HEADER_SIZE );

	// find first page with enough space
	for ( p = mediumFirstFreePage; p; p = p->next ) {
		if ( p->largestFree >= sizeNeeded ) {
			break;
		}

		assert( p->next != mediumFirstFreePage );	// this should never happen
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

	data = MediumAllocateFromPage( p, sizeNeeded );		// allocate data from page

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
		assert( p->prev );

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
void *idHeap::LargeAllocate( dword bytes ) {
	idHeap::page_s *p = AllocatePage( bytes + ALIGN_SIZE( LARGE_HEADER_SIZE ) );

	assert( p );

	if ( !p ) {
		return NULL;
	}

	byte *	d	= (byte*)(p->data) + ALIGN_SIZE( LARGE_HEADER_SIZE );
	dword *	dw	= (dword*)(d - ALIGN_SIZE( LARGE_HEADER_SIZE ));
	dw[0]		= (dword)p;				// write pointer back to page table
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
static memoryStats_t	mem_total_allocs = { 0, 0x0fffffff, 0, 0 };
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
	mem_frame_allocs.maxSize = mem_frame_frees.maxSize = 0;
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
void Mem_UpdateStats( memoryStats_t &stats, size_t size ) {
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
void Mem_UpdateAllocStats( size_t size ) {
	Mem_UpdateStats( mem_frame_allocs, size );
	Mem_UpdateStats( mem_total_allocs, size );
}

/*
==================
Mem_UpdateFreeStats
==================
*/
void Mem_UpdateFreeStats( size_t size ) {
	Mem_UpdateStats( mem_frame_frees, size );
	mem_total_allocs.num--;
	mem_total_allocs.totalSize -= size;
}

/*
==================
Mem_Size
==================
*/
size_t Mem_Size( void *ptr ) {
	return mem_heap->Msize(ptr);
}

#ifndef ID_DEBUG_MEMORY

/*
==================
Mem_Alloc
==================
*/
void *Mem_Alloc( const size_t size ) {
	if ( !size ) {
		return NULL;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		return malloc( size );
	}
	void *mem = mem_heap->Allocate( size );
	Mem_UpdateAllocStats( mem_heap->Msize( mem ) );
	if ( !mem ) {
#if defined( _WIN32 ) && !defined( _XENON )
		MEMORYSTATUSEX statex;
		statex.dwLength = sizeof( statex );
		GlobalMemoryStatusEx( &statex );
		common->Printf( "\nTotal Physical Memory: %I64d bytes\nAvailable Physical Memory: %I64d bytes\nMemory Utilization: %i %%\n\n", 
			statex.ullTotalPhys, statex.ullAvailPhys, (int)statex.dwMemoryLoad );
#endif
		common->FatalError( "Mem_Alloc request for %li bytes failed", size );
	}
	return mem;
}

/*
==================
Mem_Free
==================
*/
void Mem_Free( void *ptr ) {
	if ( ptr == NULL ) {
		return;
	}
#if 0 // used to catch memory allocated with Mem_AllocAligned
	UINT_PTR checkPtr = *((UINT_PTR *)( (UINT_PTR)ptr - sizeof( UINT_PTR ) )); 
	if ( checkPtr > (UINT_PTR)ptr - 132 && checkPtr < (UINT_PTR)ptr ) {
		assert( !"possible aligned memory deallocation" );
	}
#endif
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		free( ptr );
		return;
	}
	Mem_UpdateFreeStats( mem_heap->Msize( ptr ) );
 	mem_heap->Free( ptr );
}

/*
==================
Mem_AllocAligned
==================
*/
void *Mem_AllocAligned( const size_t size, const align_t align ) {
#if !defined(_XENON)
	// DirectX relies on being able to allocate 0 bytes and get back a legit pointer
	if ( !size ) {
		return NULL;
	}
#endif
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		return malloc( size );
	}
	void *mem = mem_heap->AllocateAligned( size, align );
	// make sure the memory is aligned
	assert( align == ALIGN_NONE || ( ((int)mem) & (align-1)) == 0 );
	return mem;
}

/*
==================
Mem_FreeAligned
==================
*/
void Mem_FreeAligned( void *ptr ) {
	if ( !ptr ) {
		return;
	}
	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		free( ptr );
		return;
	}
 	mem_heap->FreeAligned( ptr );
}

/*
==================
Mem_ClearedAlloc
==================
*/
void *Mem_ClearedAlloc( const size_t size ) {
	void *mem = Mem_Alloc( size );
	SIMDProcessor->Memset( mem, 0, size );
	return mem;
}

/*
===============
Mem_ClearedAllocAligned
===============
*/
void *Mem_ClearedAllocAligned( const size_t size, const align_t align ) {
	void *mem = Mem_AllocAligned( size, align );
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
	
	out = (char *)Mem_Alloc( idStr::Length( in ) + 1 );
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
#undef		Mem_Free
#undef		Mem_CopyString
#undef		Mem_AllocAligned
#undef		Mem_FreeAligned
#undef		Mem_ClearedAllocAligned

#define MAX_CALLSTACK_DEPTH				16
#define DEBUG_MEMORY_INFO_HASH_SIZE		1024

struct debugMemoryInfo_t {
	const char *			fileName;
	int						lineNumber;
	int						size;
	address_t				callStack[MAX_CALLSTACK_DEPTH];
	debugMemoryInfo_t *		next;
};

// size of this struct must be a multiple of 8 bytes
struct debugMemory_t {
	debugMemoryInfo_t *		info;
	short					frameNumber;
	short					offset;
	debugMemory_t *			prev;
	debugMemory_t *			next;
};

assert_sizeof_8_byte_multiple( debugMemory_t );

static debugMemory_t *		mem_debugMemory = NULL;
static debugMemoryInfo_t *	mem_debugMemoryInfo[DEBUG_MEMORY_INFO_HASH_SIZE];
static char					mem_leakName[256] = "";

/*
==================
Mem_CleanupFileName
==================
*/
debugMemoryInfo_t *Mem_FindDebugMemoryInfo( const debugMemoryInfo_t &info ) {
	int i, j, hash;
	debugMemoryInfo_t *ip;

	hash = idStr::Hash( info.fileName );
	hash ^= info.lineNumber ^ info.size;
	for ( i = 0; i < MAX_CALLSTACK_DEPTH; i++ ) {
		hash ^= info.callStack[i] << i;
	}
	hash &= ( DEBUG_MEMORY_INFO_HASH_SIZE - 1 );

	for ( ip = mem_debugMemoryInfo[hash]; ip != NULL; ip = ip->next ) {
		if ( ip->lineNumber != info.lineNumber ) {
			continue;
		}
		if ( ip->size != info.size ) {
			continue;
		}
		for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
			if ( ip->callStack[j] != info.callStack[j] ) {
				break;
			}
		}
		if ( j < MAX_CALLSTACK_DEPTH ) {
			continue;
		}
		if ( idStr::Cmp( ip->fileName, info.fileName ) != 0 ) {
			continue;
		}
		return ip;
	}

	ip = (debugMemoryInfo_t *) malloc( sizeof( debugMemoryInfo_t ) );
	ip->fileName = info.fileName;
	ip->lineNumber = info.lineNumber;
	ip->size = info.size;
	memcpy( ip->callStack, info.callStack, sizeof( ip->callStack ) );
	ip->next = mem_debugMemoryInfo[hash];
	mem_debugMemoryInfo[hash] = ip;

	return ip;
}

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
	i1 = newFileName.Find( "quack", false );
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
	newFileName.StripLeading( "./" );
	index = ( index + 1 ) & 3;
	strncpy( newFileNames[index], newFileName.c_str(), sizeof( newFileNames[index] ) );
	return newFileNames[index];
}

/*
==================
Mem_GetNonIdLibSourceFile
==================
*/
const char *Mem_GetNonIdLibSourceFile( address_t callStack[MAX_CALLSTACK_DEPTH] ) {
	int i;
	idStr sourceFile;
	static char staticSourceFile[MAX_STRING_CHARS];

	for ( i = 0; i < MAX_CALLSTACK_DEPTH && callStack[i] != 0; i++ ) {
		sourceFile = idLib::sys->GetFunctionSourceFile( callStack[i] );
		if ( sourceFile.Length() == 0 ) {
			continue;
		}
		// skip source files from idlib/
		if ( sourceFile.Find( "idlib", false ) != idStr::INVALID_POSITION ) {
			continue;
		}
		// skip .obj files that may show up for functions without a line number
		if ( sourceFile.Find( ".obj", false ) != idStr::INVALID_POSITION ) {
			continue;
		}
		int index = sourceFile.Find( "quack", false );
		if ( index != idStr::INVALID_POSITION ) {
			idStr::Copynz( staticSourceFile, sourceFile.c_str() + index + 6, sizeof( staticSourceFile ) );
		} else {
			idStr::Copynz( staticSourceFile, sourceFile.c_str(), sizeof( staticSourceFile ) );
		}
		return staticSourceFile;
	}
	return "";
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
		totalSize += b->info->size;
		for ( i = 0; i < (sizeof(dump)-1) && i < b->info->size; i++) {
			if ( ptr[i] >= 32 && ptr[i] < 127 ) {
				dump[i] = ptr[i];
			} else {
				dump[i] = '_';
			}
		}
		dump[i] = '\0';
		if ( ( b->info->size >> 10 ) != 0 ) {
			fprintf( f, "size: %6d kB: %s, line: %d [%s], call stack: %s\r\n", ( b->info->size >> 10 ), Mem_CleanupFileName( b->info->fileName ), b->info->lineNumber, dump, idLib::sys->GetCallStackStr( b->info->callStack, MAX_CALLSTACK_DEPTH ) );
		}
		else {
			fprintf( f, "size: %7d B: %s, line: %d [%s], call stack: %s\r\n", b->info->size, Mem_CleanupFileName( b->info->fileName ), b->info->lineNumber, dump, idLib::sys->GetCallStackStr( b->info->callStack, MAX_CALLSTACK_DEPTH ) );
		}
	}

	idLib::sys->ShutdownSymbols();

	fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
	fprintf( f, "%8d kB memory allocated\r\n", ( totalSize >> 10 ) );

	fclose( f );
}

/*
==================
Mem_DumpCompressed
==================
*/
struct allocInfo_t {
	char *					fileName;
	int						lineNumber;
	int						size;
	int						numAllocs;
	address_t				callStack[MAX_CALLSTACK_DEPTH];
	allocInfo_t *			next;
};

void Mem_DumpCompressed( const char *fileName, memoryGroupType_t memGroup, memorySortType_t memSort, int sortCallStack, int numFrames, bool xlFriendly ) {
	int numBlocks, totalSize, r, j, lineNumber;
	debugMemory_t *b;
	allocInfo_t *a, *nexta, *allocInfo = NULL, *sortedAllocInfo = NULL, *prevSorted, *nextSorted;
	idStr module, funcName, path;
	FILE *f;

	// build list with memory allocations
	totalSize = 0;
	numBlocks = 0;
	for ( b = mem_debugMemory; b; b = b->next ) {

		if ( numFrames && b->frameNumber < idLib::frameNumber - numFrames ) {
			continue;
		}

		numBlocks++;
		totalSize += b->info->size;

		path = Mem_GetNonIdLibSourceFile( b->info->callStack );
		lineNumber = b->info->lineNumber;

		if ( memGroup == MEMGROUP_FOLDER ) {
			path.StripFilename();
			path += "\\";
			lineNumber = idStr::Hash( path );
		} else if ( memGroup == MEMGROUP_FILE ) {
			lineNumber = idStr::Hash( path );
		}

		// search for an allocation from the same source location
		for ( a = allocInfo; a; a = a->next ) {
			if ( a->lineNumber != lineNumber ) {
				continue;
			}
			if ( memGroup == MEMGROUP_LINE ) {
				for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
					if ( a->callStack[j] != b->info->callStack[j] ) {
						break;
					}
				}
				if ( j < MAX_CALLSTACK_DEPTH ) {
					continue;
				}
			}
			if ( path.Cmp( a->fileName ) != 0 ) {
				continue;
			}
			a->numAllocs++;
			a->size += b->info->size;
			break;
		}

		// if this is an allocation from a new source location
		if ( !a ) {
			int len = path.Length() + 1;
			a = (allocInfo_t *) ::malloc( sizeof( allocInfo_t ) + len );
			a->fileName = ((char *)a) + sizeof( allocInfo_t );
			idStr::Copynz( a->fileName, path, len );
			a->lineNumber = lineNumber;
			a->size = b->info->size;
			a->numAllocs = 1;
			for ( j = 0; j < MAX_CALLSTACK_DEPTH; j++ ) {
				a->callStack[j] = b->info->callStack[j];
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
					r = idStr::IcmpPath( a->fileName, nextSorted->fileName );
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
		} else {
			prevSorted->next = a;
			a->next = nextSorted;
		}
	}

	f = fopen( fileName, "wb" );
	if ( !f ) {
		idLib::common->Warning( "Could not open '%s' for writing", fileName );
		return;
	}

	if ( xlFriendly ) {
		if ( memGroup == MEMGROUP_FILE || memGroup == MEMGROUP_FOLDER ) {
			fprintf( f, "  Size Allocs File\r\n" );
		} else {
			fprintf( f, "  Size Allocs  Line File Call Stack\r\n" );
		}
	}
	// write list to file
	if ( xlFriendly ) {
		if ( memGroup == MEMGROUP_FILE || memGroup == MEMGROUP_FOLDER ) {
			for ( a = sortedAllocInfo; a; a = nexta ) {
				nexta = a->next;
				fprintf( f, "%6d %6d %s\r\n", ( a->size >> 10 ), a->numAllocs, a->fileName );
				::free( a );
			}
		} else {
			for ( a = sortedAllocInfo; a; a = nexta ) {
				nexta = a->next;
				fprintf( f, "%6d %6d %5d %s %s\r\n", ( a->size >> 10 ), a->numAllocs, a->lineNumber, a->fileName, idLib::sys->GetCallStackStr( a->callStack, MAX_CALLSTACK_DEPTH ) );
				::free( a );
			}
		}
	} else {
		if ( memGroup == MEMGROUP_FILE || memGroup == MEMGROUP_FOLDER ) {
			for ( a = sortedAllocInfo; a; a = nexta ) {
				nexta = a->next;
				fprintf( f, "size: %6d kB, allocs: %5d: %s\r\n", ( a->size >> 10 ), a->numAllocs, a->fileName );
				::free( a );
			}
		} else {
			for ( a = sortedAllocInfo; a; a = nexta ) {
				nexta = a->next;
				fprintf( f, "size: %6d kB, allocs: %5d: %s, line: %d, call stack: %s\r\n", ( a->size >> 10 ), a->numAllocs, a->fileName, a->lineNumber, idLib::sys->GetCallStackStr( a->callStack, MAX_CALLSTACK_DEPTH ) );
				::free( a );
			}
		}
	}

	idLib::sys->ShutdownSymbols();

	if ( !xlFriendly ) {
		fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
		fprintf( f, "%8d KB memory allocated\r\n", ( totalSize >> 10 ) );
	}

	fclose( f );
}

/*
==================
Mem_DumpPerClass
==================
*/
struct allocClass_t {
	char				className[MAX_STRING_CHARS];
	int					totalSize;
	int					numAllocs;
	allocClass_t *		next;
};

#define ALLOC_CLASS_HASH_SIZE		4096

void Mem_DumpPerClass( const char *fileName ) {
	int i, j, numBlocks, totalBlocks, totalSize;
	const char *funcName, *colon;
	char className[MAX_CALLSTACK_DEPTH][MAX_STRING_CHARS];
	debugMemory_t *b;
	FILE *f;
	allocClass_t * allocClass[ALLOC_CLASS_HASH_SIZE];
	allocClass_t * a, *nexta;

	f = fopen( fileName, "wb" );
	if ( !f ) {
		return;
	}

	idLib::common->SetRefreshOnPrint( true );

	memset( allocClass, 0, sizeof( allocClass ) );

	for ( totalBlocks = 0, b = mem_debugMemory; b != NULL; b = b->next, totalBlocks++ ) {
	}

	int lastPercentage = 0;

	totalSize = 0;
	for ( numBlocks = 0, b = mem_debugMemory; b != NULL; b = b->next, numBlocks++ ) {
		totalSize += b->info->size;

		for ( i = 0; i < MAX_CALLSTACK_DEPTH; i++ ) {
			funcName = idLib::sys->GetFunctionName( b->info->callStack[i] );
			colon = strstr( funcName, "::" );
			if ( colon == NULL ) {
				continue;
			}
			idStr::Copynz( className[i], funcName, colon - funcName + 1 );

			for ( j = 0; j < i; j++ ) {
				if ( idStr::Cmp( className[j], className[i] ) == 0 ) {
					break;
				}
			}
			if ( j < i ) {
				continue;
			}

			int hash = idStr::Hash( className[i] ) & ( ALLOC_CLASS_HASH_SIZE - 1 );

			for ( a = allocClass[hash]; a; a = a->next ) {
				if ( idStr::Cmp( a->className, className[i] ) == 0 ) {
					a->totalSize += b->info->size;
					a->numAllocs++;
					break;
				}
			}
			if ( a == NULL ) {
				a = (allocClass_t *) ::malloc( sizeof( allocClass_t ) );
				idStr::Copynz( a->className, className[i], sizeof( a->className ) );
				a->totalSize = b->info->size;
				a->numAllocs = 1;
				a->next = allocClass[hash];
				allocClass[hash] = a;
			}
		}

		int percentage = numBlocks * 100 / totalBlocks;
		if ( percentage != lastPercentage ) {
			idLib::common->Printf( "\r%3d%%", percentage );
			lastPercentage = percentage;
		}
	}

	for ( i = 0; i < ALLOC_CLASS_HASH_SIZE; i++ ) {
		for ( a = allocClass[i]; a; a = nexta ) {
			nexta = a->next;
			fprintf( f, "size: %6d kB, allocs: %5d: %s\r\n", ( a->totalSize >> 10 ), a->numAllocs, a->className );
			::free( a );
		}
	}

	idLib::common->Printf( "\r100%%" );
	idLib::common->SetRefreshOnPrint( false );

	idLib::sys->ShutdownSymbols();

	fprintf( f, "%8d total memory blocks allocated\r\n", numBlocks );
	fprintf( f, "%8d kB memory allocated\r\n", ( totalSize >> 10 ) );

	fclose( f );
}

/*
==================
Mem_AllocDebugMemory
==================
*/
void *Mem_AllocDebugMemory( const size_t size, const char *fileName, const int lineNumber, const align_t align ) {
	void *p;
	debugMemory_t *m;
	debugMemoryInfo_t info;
	short offset = 0;

	if ( !size ) {
		return NULL;
	}

	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		// NOTE: set a breakpoint here to find memory allocations before mem_heap is initialized
		return malloc( size );
	}

	if ( align > ALIGN_8 ) {
		offset = ( ( sizeof( debugMemory_t ) + align - 1 ) / align ) * align - sizeof( debugMemory_t ) ;
		p = mem_heap->AllocateAligned( size + sizeof( debugMemory_t ) + offset, align );
	} else {
		p = mem_heap->Allocate( size + sizeof( debugMemory_t ) );
	}

	Mem_UpdateAllocStats( size );

	info.fileName = fileName;
	info.lineNumber = lineNumber;
	info.size = size;
	idLib::sys->GetCurCallStack( info.callStack, MAX_CALLSTACK_DEPTH );
	info.next = NULL;

	m = (debugMemory_t *) ( ( (byte *) p ) + offset );
	m->info = Mem_FindDebugMemoryInfo( info );
	m->frameNumber = idLib::frameNumber;
	m->offset = offset;
	m->next = mem_debugMemory;
	m->prev = NULL;
	if ( mem_debugMemory ) {
		mem_debugMemory->prev = m;
	}
	mem_debugMemory = m;

	return ( ( (byte *) p ) + sizeof( debugMemory_t ) + offset );
}

/*
==================
Mem_FreeDebugMemory
==================
*/
void Mem_FreeDebugMemory( void *p, const char *fileName, const int lineNumber, const bool aligned ) {
	debugMemory_t *m;
	short offset;

	if ( !p ) {
		return;
	}

	if ( !mem_heap ) {
#ifdef CRASH_ON_STATIC_ALLOCATION
		*((int*)0x0) = 1;
#endif
		// NOTE: set a breakpoint here to find memory being freed before mem_heap is initialized
		free( p );
		return;
	}

	m = (debugMemory_t *) ( ( (byte *) p ) - sizeof( debugMemory_t ) );

	if ( m->offset < 0 ) {
		idLib::common->FatalError( "memory freed twice, first from %s, now from %s", idLib::sys->GetCallStackStr( m->info->callStack, MAX_CALLSTACK_DEPTH ), idLib::sys->GetCurCallStackStr( MAX_CALLSTACK_DEPTH ) );
	}

	Mem_UpdateFreeStats( m->info->size );

	if ( m->next ) {
		m->next->prev = m->prev;
	}
	if ( m->prev ) {
		m->prev->next = m->next;
	} else {
		mem_debugMemory = m->next;
	}

	offset = m->offset;
	m->offset = -1;

	if ( aligned ) {
 		mem_heap->FreeAligned( ( ( byte * )m ) - offset );
	} else {
 		mem_heap->Free( m );
	}
}

/*
==================
Mem_Alloc
==================
*/
void *Mem_Alloc( const size_t size, const char *fileName, const int lineNumber ) {
	if ( !size ) {
		return NULL;
	}
	return Mem_AllocDebugMemory( size, fileName, lineNumber, ALIGN_8 );
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
Mem_AllocAligned
==================
*/
void *Mem_AllocAligned( const size_t size, const align_t align, const char *fileName, const int lineNumber ) {
	if ( !size ) {
		return NULL;
	}
	void *mem = Mem_AllocDebugMemory( size, fileName, lineNumber, align );
	// make sure the memory is aligned
	assert( align == ALIGN_NONE || ( ((int)mem) & (align-1)) == 0 );
	return mem;
}

/*
==================
Mem_FreeAligned
==================
*/
void Mem_FreeAligned( void *ptr, const char *fileName, const int lineNumber ) {
	if ( !ptr ) {
		return;
	}
	Mem_FreeDebugMemory( ptr, fileName, lineNumber, true );
}

/*
==================
Mem_ClearedAlloc
==================
*/
void *Mem_ClearedAlloc( const size_t size, const char *fileName, const int lineNumber ) {
	void *mem = Mem_Alloc( size, fileName, lineNumber );
	SIMDProcessor->Memset( mem, 0, size );
	return mem;
}

/*
===============
Mem_ClearedAllocAligned
===============
*/
void *Mem_ClearedAllocAligned( const size_t size, const align_t align, const char *fileName, const int lineNumber ) {
	void *mem = Mem_AllocAligned( size, align, fileName, lineNumber );
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
	
	out = (char *)Mem_Alloc( idStr::Length( in ) + 1, fileName, lineNumber );
	idStr::Copynz( out, in, idStr::Length( in ) + 1 );
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
		Mem_DumpCompressed( va( "%s_leak_size.txt", mem_leakName ), MEMGROUP_LINE, MEMSORT_SIZE, 0, 0, false );
		Mem_DumpCompressed( va( "%s_leak_location.txt", mem_leakName ), MEMGROUP_LINE, MEMSORT_LOCATION, 0, 0, false );
		Mem_DumpCompressed( va( "%s_leak_cs1.txt", mem_leakName ), MEMGROUP_LINE, MEMSORT_CALLSTACK, 2, 0, false );
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

#endif /* !ID_DEBUG_MEMORY */

#pragma warning( pop )
