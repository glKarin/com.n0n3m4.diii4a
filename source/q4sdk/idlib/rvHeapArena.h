//
// rvHeapArena.h - Heap arena object that manages a set of heaps
// Date: 12/13/04
// Created by: Dwight Luetscher
//

#ifndef __RV_HEAP_ARENA_H__
#define __RV_HEAP_ARENA_H__

class rvHeap;

// Define some limits used by a heap arena
static const int maxNumHeapsPerArena			= 16;	// maximum number of heaps that can be simultaneously initialized with the same arena
static const int maxHeapStackDepth				= maxNumHeapsPerArena*2;	// maximum depth of each arena's heap stack


class rvHeapArena {
public:
	rvHeapArena();								// constructor
	~rvHeapArena();								// destructor

	void Init( );								// initializes this heap arena for use
	void Shutdown( );							// releases this heap arena from use (shutting down all associated heaps)

	ID_INLINE bool IsInitialized( ) const;		// returns true if this rvHeapArena object is currently initialized and not released, false otherwise

	// Push the current heap through the heap object itself (you can Pop() there as well)
	void Pop( );								// pops the top of the stack, restoring the previous heap as the active heap for this arena
	ID_INLINE bool IsStackFull( );				// returns true if the heap arena stack is full, false otherwise

	ID_INLINE rvHeap *GetActiveHeap( );			// returns the active heap for this arena (from the top of the stack, NULL if stack is empty)

	ID_INLINE void EnterArenaCriticalSection();			// enters this heap arena's critical section
	ID_INLINE void ExitArenaCriticalSection();			// exits this heap arena's critical section
	
	void *Allocate( unsigned int sizeBytes, int debugTag = 0);		// allocates the given amount of memory from this arena
	void *Allocate16( unsigned int sizeBytes, int debugTag = 0);	// allocates the given amount of memory from this arena, aligned on a 16-byte boundary
	void Free( void *p );											// free memory back to this arena

	int Msize( void *p );						// returns the size, in bytes, of the allocation at the given address (including header, alignment bytes, etc).

	rvHeap *GetHeap( void *p );					// returns the heap that the given allocation was made from, NULL for none

	ID_INLINE rvHeap *GetFirstHeap( );			// returns the first heap associated with this arena, NULL for none
	rvHeap *GetNextHeap( rvHeap &rfPrevHeap );	// returns that follows the given one (associated with this arena), NULL for none

	void GetTagStats(int tag, int &num, int &size, int &peak);	// returns the total stats for a particular tag type (across all heaps managed by this arena)

protected:
	// NOTE: we cannot use idList here - dynamic memory is not yet available.
	rvHeap *m_heapStack[maxHeapStackDepth];		// stack of heap object pointers
	CRITICAL_SECTION m_criticalSection;			// critical section associated with this heap
	int m_tos;									// top of stack
	rvHeap *m_heapList;							// linked-list of all the heaps that are actively associated with this arena 
	bool m_isInitialized;						// set to true if this rvHeapArena object is currently initialized and not released

	void ResetValues( );						// resets the data members to their pre-initialized state

	friend class rvHeap;						// give the rvHeap class access to the following methods
	// {
	void Push( rvHeap &newActiveHeap );			// pushes the given heap onto the top of the stack making it the active one for this arena
	void InitHeap( rvHeap &newActiveHeap );		// initializes the given heap to be under the care of this arena
	void ShutdownHeap( rvHeap &newActiveHeap );	// releases the given heap from the care of this arena
	// }
};

// IsInitialized
//
// returns: true if this rvHeapArena object is currently initialized and not released, false otherwise
ID_INLINE bool rvHeapArena::IsInitialized( ) const
{
	return m_isInitialized;
}

// EnterArenaCriticalSection
//
// enters this heap arena's critical section
ID_INLINE void rvHeapArena::EnterArenaCriticalSection() 
{
	::EnterCriticalSection( &m_criticalSection );
}

// ExitArenaCriticalSection
//
// exits this heap arena's critical section
ID_INLINE void rvHeapArena::ExitArenaCriticalSection() 
{
	::LeaveCriticalSection( &m_criticalSection );
}	

// IsStackFull
//
// returns: true if the heap arena stack is full, false otherwise
ID_INLINE bool rvHeapArena::IsStackFull( )
{
	bool full;

	EnterArenaCriticalSection();
	full = m_tos >= (maxHeapStackDepth - 1);
	ExitArenaCriticalSection();

	return full;
}

// GetActiveHeap
//
// returns: the active heap for this arean (from the top of the stack, NULL if stack is empty)
ID_INLINE rvHeap *rvHeapArena::GetActiveHeap( )
{
	rvHeap *tos;
	EnterArenaCriticalSection();
	if ( m_tos < 0 )
	{
		ExitArenaCriticalSection();
		return NULL;
	}
	tos = m_heapStack[ m_tos ];
	ExitArenaCriticalSection();
	return tos;
}

// GetFirstHeap
//
// returns: the first heap associated with this arena, NULL for none
ID_INLINE rvHeap *rvHeapArena::GetFirstHeap( )
{
	rvHeap *firstHeap;

	EnterArenaCriticalSection();
	firstHeap = m_heapList;
	ExitArenaCriticalSection();

	return firstHeap;
}

#endif	// #ifndef __RV_HEAP_MANAGER_H__



