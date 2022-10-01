//
// rvMemSys.h - Memory management system
// Date: 12/17/04
// Created by: Dwight Luetscher
//
// Date: 04/01/05
// Modified by: Marcus Whitlock
// Added permanent heap and new heap push function to push the heap that contains
// a specified piece of memory.
//
// Date: 04/04/05
// Modified by: Marcus Whitlock
// Added rvAutoHeapCtxt class for push and auto-pop of heap context.
//

#ifndef __RV_MEM_SYS_H__
#define __RV_MEM_SYS_H__

typedef enum 
{
	RV_HEAP_ID_DEFAULT,			// heap that exists on application startup
	RV_HEAP_ID_PERMANENT,		// heap for allocations that have permanent (application scope) lifetime
	RV_HEAP_ID_LEVEL,			// heap for allocations that have a level lifetime
	RV_HEAP_ID_MULTIPLE_FRAME,	// heap for run-time allocations that have a lifetime of multiple draw frames 
	RV_HEAP_ID_SINGLE_FRAME,	// heap for run-time allocations that have a lifetime of a single draw frame
	RV_HEAP_ID_TEMPORARY,		// heap for objects that have a short lifetime (temporaries generally used for level loading)
	RV_HEAP_ID_IO_TEMP,			// heap for allocations that are temporaries used in I/O operations like level loading or writing out data
	rv_heap_ID_max_count		// just a count, not a valid type
} 
Rv_Sys_Heap_ID_t;

static const uint MAX_SYSTEM_HEAPS	= (uint) rv_heap_ID_max_count;

#ifdef _RV_MEM_SYS_SUPPORT

//
//	_RV_MEM_SYS_SUPPORT is defined
//

extern rvHeapArena *currentHeapArena;	

// Functions for getting and setting the system heaps.
void	rvSetSysHeap( Rv_Sys_Heap_ID_t sysHeapID, rvHeap *heapPtr );					// associates a heap with the given system heap ID value
rvHeap* rvGetSysHeap( Rv_Sys_Heap_ID_t sysHeapID );										// retrieves the specified system heap
void	rvGetAllSysHeaps( rvHeap *destSystemHeapArray[MAX_SYSTEM_HEAPS] );				// retrieves all the MAX_SYSTEM_HEAPS heap pointers into the given array
void	rvSetAllSysHeaps( rvHeap *srcSystemHeapArray[MAX_SYSTEM_HEAPS] );				// associates all the MAX_SYSTEM_HEAPS heap pointers from the given array with their corresponding id value
bool	rvPushHeapContainingMemory( const void* mem );									// pushes the heap containg the memory specified to the top of the arena stack, making it current - mwhitlock

void rvEnterArenaCriticalSection( );							// enters the heap arena critical section
void rvExitArenaCriticalSection( );								// exits the heap arena critical section
void rvPushSysHeap(Rv_Sys_Heap_ID_t sysHeapID);					// pushes the system heap associated with the given identifier to the top of the arena stack, making it current

// Useful in situations where a heap is pushed, but a return on error or an
// exception could cause the stack to be unwound, bypassing the heap pop
// operation - mwhitlock.
class rvAutoHeapCtxt
{
	bool mPushed;

public:
	rvAutoHeapCtxt(void) :
		mPushed(false)
	{
		// Should never call this.
		assert(0);
	}

	rvAutoHeapCtxt(Rv_Sys_Heap_ID_t sysHeapID) :
		mPushed(false)
	{
		rvPushSysHeap(sysHeapID);
		mPushed = true;
	}

	rvAutoHeapCtxt(const void* mem) :
		mPushed(false)
	{
		mPushed = rvPushHeapContainingMemory( mem );
	}

	~rvAutoHeapCtxt(void)
	{
		if(mPushed)
		{
			currentHeapArena->Pop();
		}
	}
};

//
//	RV_PUSH_SYS_HEAP_ID()
//	Push system heaps by their ID (always available to idLib, Game and executable).
//
#define RV_PUSH_SYS_HEAP_ID(sysHeapID)				rvPushSysHeap(sysHeapID)
#define RV_PUSH_SYS_HEAP_ID_AUTO(varName,sysHeapID)	rvAutoHeapCtxt varName(sysHeapID)

//
//	RV_PUSH_HEAP_MEM()
//	Push the heap containing the piece of memory pointed to. Note that if the
//	piece of memory is not from a heap, no heap will be pushed.
//
#define RV_PUSH_HEAP_MEM(memPtr)					rvPushHeapContainingMemory(memPtr)
#define RV_PUSH_HEAP_MEM_AUTO(varName,memPtr)		rvAutoHeapCtxt varName(memPtr)

//
//	RV_PUSH_HEAP_PTR()
//	Local heaps used mainly by executable (idLib and Game would use these only
//	if heap was passed in)
//
#define RV_PUSH_HEAP_PTR(heapPtr)					( (heapPtr)->PushCurrent() )

//
//	RV_PUSH_SYS_HEAP()
//	Pop top of heap stack, regardless of how it was pushed.
//
#define RV_POP_HEAP()								( currentHeapArena->Pop() )

// The following versions enter/exit the heap arena's critical section so that
// critical section protection remains active between a push/pop pair (NOTE that
// the heap and heap arena are always protected by critical sections within a single method call)
#define RV_PUSH_SYS_HEAP_ENTER_CRIT_SECT(sysHeapID)	{ rvEnterArenaCriticalSection( ); rvPushSysHeap( sysHeapID ); }
#define RV_PUSH_HEAP_ENTER_CRIT_SECT(heapPtr)		{ rvEnterArenaCriticalSection( ); (heapPtr)->PushCurrent( ); }
#define RV_POP_HEAP_EXIT_CRIT_SECT()				{ currentHeapArena->Pop( ); rvExitArenaCriticalSection( ); }

#else	// #ifdef _RV_MEM_SYS_SUPPORT

//
//	_RV_MEM_SYS_SUPPORT is not defined
//

#define RV_PUSH_SYS_HEAP_ID(sysHeapID)
#define RV_PUSH_SYS_HEAP_ID_AUTO(varName,sysHeapID)
#define RV_PUSH_HEAP_MEM(memPtr)
#define RV_PUSH_HEAP_MEM_AUTO(varName,memPtr)
#define RV_PUSH_HEAP_PTR(heapPtr)
#define RV_POP_HEAP()	

#define RV_PUSH_SYS_HEAP_ENTER_CRIT_SECT(sysHeapID)
#define RV_PUSH_HEAP_ENTER_CRIT_SECT(heapPtr)	
#define RV_POP_HEAP_EXIT_CRIT_SECT()			

#endif	// #else not #ifdef _RV_MEM_SYS_SUPPORT

#endif	// #ifndef __RV_MEM_SYS_H__
