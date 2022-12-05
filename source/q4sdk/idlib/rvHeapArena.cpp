//
// rvHeapArena.cpp - Heap arena object that manages a set of heaps
// Date: 12/13/04
// Created by: Dwight Luetscher
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#ifdef _RV_MEM_SYS_SUPPORT

// rvHeapArena
//
// constructor
rvHeapArena::rvHeapArena()
{
	// ResetValues();	do this in the Init() call instead (due to the fact that other constructors could call rvHeapArena::Init() before this constructor is called)
}

// ~rvHeapArena
//
// destructor
rvHeapArena::~rvHeapArena()
{
	Shutdown();
}

// Init
//
// initializes this heap arena for use
void rvHeapArena::Init( )
{
	if ( m_isInitialized )
	{
		return;
	}

	ResetValues();
	m_isInitialized = true;

	// create the critical section used by this heap arena
	InitializeCriticalSection( &m_criticalSection );
}

// Shutdown
//
// releases this heap arena from use (shutting down all associated heaps)
void rvHeapArena::Shutdown( )
{
	// shutdown each heap from this arena's list
	rvHeap *curHeap = m_heapList, *nextHeap;
	while ( curHeap != NULL )
	{
		nextHeap = curHeap->GetNext();

		curHeap->Shutdown();

		curHeap = nextHeap;
	}
	DeleteCriticalSection( &m_criticalSection );
	ResetValues();
}

// ResetValues
// 
// resets the data members to their pre-initialized state
void rvHeapArena::ResetValues( )
{
	memset( m_heapStack, 0, sizeof(m_heapStack) );
	memset( &m_criticalSection, 0, sizeof(m_criticalSection) );
	m_tos = -1;
	m_heapList = NULL;
	m_isInitialized = false;
}

// Push
//
// pushes the given heap onto the top of the stack making it the active one for this arena
void rvHeapArena::Push( rvHeap &newActiveHeap )
{
	EnterArenaCriticalSection();
	assert( newActiveHeap.GetArena() == this );
	assert(m_tos+1 < maxHeapStackDepth);	// stack overflow?
	if (m_tos+1 < maxHeapStackDepth) 
	{
		m_heapStack[++m_tos] = &newActiveHeap;
	}
	ExitArenaCriticalSection();
}

// Pop
//
// pops the top of the stack, restoring the previous heap as the active heap for this arena
void rvHeapArena::Pop( )
{
	EnterArenaCriticalSection();
	assert(m_tos > -1);						// stack underflow?
	if (m_tos > -1) 
	{
		m_tos--;
	}
	ExitArenaCriticalSection();
}

// GetHeap
//
// returns: the heap that the given allocation was made from, NULL for none
rvHeap *rvHeapArena::GetHeap( void *p )
{
	EnterArenaCriticalSection();
	if ( !m_isInitialized )
	{
		ExitArenaCriticalSection();
		return NULL;
	}

	rvHeap *curHeap = m_heapList;
	while ( curHeap != NULL && !curHeap->DoesAllocBelong(p) )
	{
		curHeap = curHeap->GetNext();
	}
	ExitArenaCriticalSection();

	return curHeap;
}


// Allocate
// 
// allocates the given amount of memory from this arena.
void *rvHeapArena::Allocate( unsigned int sizeBytes, int debugTag )
{
	rvHeap *curHeap;

	EnterArenaCriticalSection();
	assert( m_tos >= 0 && m_tos < maxHeapStackDepth );
	if ( m_tos < 0 )
	{
		ExitArenaCriticalSection();
		return NULL;
	}
	curHeap = m_heapStack[ m_tos ];
	ExitArenaCriticalSection();

	return curHeap->Allocate( sizeBytes, debugTag );
}

// Allocate16
//
// allocates the given amount of memory from this arena, 
// aligned on a 16-byte boundary.
void *rvHeapArena::Allocate16( unsigned int sizeBytes, int debugTag )
{
	rvHeap *curHeap;

	EnterArenaCriticalSection();
	assert( m_tos >= 0 && m_tos < maxHeapStackDepth );
	if ( m_tos < 0 )
	{
		ExitArenaCriticalSection();
		return NULL;
	}
	curHeap = m_heapStack[ m_tos ];
	ExitArenaCriticalSection();

	return curHeap->Allocate16( sizeBytes, debugTag );
}

// Free
// 
// free memory back to this arena
void rvHeapArena::Free( void *p )
{
	rvHeap *heap = GetHeap( p );	// arena critical section protection is in GetHeap()
	if (heap != NULL)
	{
		heap->Free( p );
	}
}
 
// Msize
//
// returns: the size, in bytes, of the allocation at the given address (including header, alignment bytes, etc).
int rvHeapArena::Msize( void *p )
{
	rvHeap *heap = GetHeap( p );	// arena critical section protection is in GetHeap()
	if (heap != NULL)
	{
		return heap->Msize( p );
	}
	return 0;
}

// InitHeap
//
// initializes the given heap to be under the care of this arena
void rvHeapArena::InitHeap( rvHeap &newActiveHeap )
{
	assert( newActiveHeap.GetArena() == NULL );

	newActiveHeap.SetArena( this );
	newActiveHeap.SetNext( m_heapList );
	m_heapList = &newActiveHeap;
}

// ShutdownHeap
//
// releases the given heap from the care of this arena
void rvHeapArena::ShutdownHeap( rvHeap &activeHeap )
{
	int stackPos, copyPos;

	assert( activeHeap.GetArena() == this );

	activeHeap.SetArena( NULL );

	// make sure that the heap is removed from the stack
	for ( stackPos = 0; stackPos <= m_tos; stackPos++ )
	{
		if ( m_heapStack[stackPos] == &activeHeap )
		{
			for ( copyPos = stackPos; copyPos < m_tos; copyPos++ )
			{
				m_heapStack[copyPos] = m_heapStack[copyPos+1];
			}
			m_tos--;
		}
	}

	// remove the heap from this arena's list
	rvHeap *curHeap = m_heapList, * prevHeap = NULL;
	while ( curHeap != NULL )
	{
		if ( curHeap == &activeHeap )
		{
			if ( NULL == prevHeap )
			{
				m_heapList = m_heapList->GetNext();
			}
			else 
			{
				prevHeap->SetNext( curHeap->GetNext() );
			}
			break;
		}
		prevHeap = curHeap;
		curHeap = curHeap->GetNext();
	}
}

// GetNextHeap
//
// returns: that follows the given one (associated with this arena), NULL for none
rvHeap *rvHeapArena::GetNextHeap( rvHeap &rfPrevHeap )
{
	rvHeap *nextHeap;

	EnterArenaCriticalSection();
	if ( rfPrevHeap.GetArena() != this )
	{
		nextHeap = NULL;
	}
	else
	{
		nextHeap = rfPrevHeap.GetNext();
	}
	ExitArenaCriticalSection();

	return nextHeap;
}

// GetTagStats
//
// returns: the total stats for a particular tag type (across all heaps managed by this arena)
void rvHeapArena::GetTagStats(int tag, int &num, int &size, int &peak)
{
	int curPeak;

	assert( tag < MA_MAX );

	EnterArenaCriticalSection();

	num = size = peak = 0;

	rvHeap *curHeap = m_heapList;
	while ( curHeap != NULL )
	{
		num += curHeap->GetNumAllocationsByTag( (Mem_Alloc_Types_t) tag );
		size += curHeap->GetBytesAllocatedByTag( (Mem_Alloc_Types_t) tag );

		curPeak = curHeap->GetPeekBytesAllocatedByTag( (Mem_Alloc_Types_t) tag );
		if ( curPeak > peak )
		{
			peak = curPeak;
		}

		curHeap = curHeap->GetNext();
	}

	ExitArenaCriticalSection();
}

#endif	// #ifdef _RV_MEM_SYS_SUPPORT
