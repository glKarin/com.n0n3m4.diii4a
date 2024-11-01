////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - CallStack Class Implementations
//  Copyright (c) 2005-2006 Dan Moulding
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
//  See COPYING.txt for the full terms of the GNU Lesser General Public License.
//
////////////////////////////////////////////////////////////////////////////////

#include <cassert>
#include <windows.h>
#define __out_xcount(x) // Workaround for the specstrings.h bug in the Platform SDK.
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>    // Provides symbol handling services.
#define VLDBUILD
#include "callstack.h"  // This class' header.
#include "utility.h"    // Provides various utility functions.
#include "vldheap.h"    // Provides internal new and delete operators.
#include "vldint.h"     // Provides access to VLD internals.

#define MAXSYMBOLNAMELENGTH 256

// Imported global variables.
extern HANDLE             currentprocess;
extern HANDLE             currentthread;
extern CRITICAL_SECTION   stackwalklock;
extern CRITICAL_SECTION   symbollock;

// Constructor - Initializes the CallStack with an initial size of zero and one
//   Chunk of capacity.
//
CallStack::CallStack ()
{
    m_capacity   = CALLSTACKCHUNKSIZE;
    m_size       = 0;
    m_status     = 0x0;
    m_store.next = NULL;
    m_topchunk   = &m_store;
    m_topindex   = 0;
}

// Copy Constructor - For efficiency, we want to avoid ever making copies of
//   CallStacks (only pointer passing or reference passing should be performed).
//   The sole purpose of this copy constructor is to ensure that no copying is
//   being done inadvertently.
//
CallStack::CallStack (const CallStack &)
{
    // Don't make copies of CallStacks!
    assert(FALSE);
}

// Destructor - Frees all memory allocated to the CallStack.
//
CallStack::~CallStack ()
{
    CallStack::chunk_t *chunk = m_store.next;
    CallStack::chunk_t *temp;

    while (chunk) {
        temp = chunk;
        chunk = temp->next;
        delete temp;
    }
}

// operator = - Assignment operator. For efficiency, we want to avoid ever
//   making copies of CallStacks (only pointer passing or reference passing
//   should be performed). The sole purpose of this assignment operator is to
//   ensure that no copying is being done inadvertently.
//
CallStack& CallStack::operator = (const CallStack &)
{
    // Don't make copies of CallStacks!
    assert(FALSE);
    return *this;
}

// operator == - Equality operator. Compares the CallStack to another CallStack
//   for equality. Two CallStacks are equal if they are the same size and if
//   every frame in each is identical to the corresponding frame in the other.
//
//  other (IN) - Reference to the CallStack to compare the current CallStack
//    against for equality.
//
//  Return Value:
//
//    Returns true if the two CallStacks are equal. Otherwise returns false.
//
BOOL CallStack::operator == (const CallStack &other) const
{
    const CallStack::chunk_t *chunk = &m_store;
    UINT32                    index;
    const CallStack::chunk_t *otherchunk = &other.m_store;
    const CallStack::chunk_t *prevchunk = NULL;

    if (m_size != other.m_size) {
        // They can't be equal if the sizes are different.
        return FALSE;
    }

    // Walk the chunk list and within each chunk walk the frames array until we
    // either find a mismatch, or until we reach the end of the call stacks.
    while (prevchunk != m_topchunk) {
        for (index = 0; index < ((chunk == m_topchunk) ? m_topindex : CALLSTACKCHUNKSIZE); index++) {
            if (chunk->frames[index] != otherchunk->frames[index]) {
                // Found a mismatch. They are not equal.
                return FALSE;
            }
        }
        prevchunk = chunk;
        chunk = chunk->next;
        otherchunk = otherchunk->next;
    }

    // Reached the end of the call stacks. They are equal.
    return TRUE;
}

// operator [] - Random access operator. Retrieves the frame at the specified
//   index.
//
//   Note: We give up a bit of efficiency here, in favor of efficiency of push
//     operations. This is because walking of a CallStack is done infrequently
//     (only if a leak is found), whereas pushing is done very frequently (for
//     each frame in the program's call stack when the program allocates some
//     memory).
//
//  - index (IN): Specifies the index of the frame to retrieve.
//
//  Return Value:
//
//    Returns the program counter for the frame at the specified index. If the
//    specified index is out of range for the CallStack, the return value is
//    undefined.
//
SIZE_T CallStack::operator [] (UINT32 index) const
{
    UINT32                    count;
    const CallStack::chunk_t *chunk = &m_store;
    UINT32                    chunknumber = index / CALLSTACKCHUNKSIZE;

    for (count = 0; count < chunknumber; count++) {
        chunk = chunk->next;
    }

    return chunk->frames[index % CALLSTACKCHUNKSIZE];
}

// clear - Resets the CallStack, returning it to a state where no frames have
//   been pushed onto it, readying it for reuse.
//
//   Note: Calling this function does not release any memory allocated to the
//     CallStack. We give up a bit of memory-usage efficiency here in favor of
//     performance of push operations.
//
//  Return Value:
//
//    None.
//
VOID CallStack::clear ()
{
    m_size     = 0;
    m_topchunk = &m_store;
    m_topindex = 0;
}

// dump - Dumps a nicely formatted rendition of the CallStack, including
//   symbolic information (function names and line numbers) if available.
//
//   Note: The symbol handler must be initialized prior to calling this
//     function.
//
//  - showinternalframes (IN): If true, then all frames in the CallStack will be
//      dumped. Otherwise, frames internal to the heap will not be dumped.
//
//  Return Value:
//
//    None.
//
VOID CallStack::dump (BOOL showinternalframes) const
{
    DWORD            displacement;
    DWORD64          displacement64;
    BOOL             foundline;
    UINT32           frame;
    SYMBOL_INFO     *functioninfo;
    LPWSTR           functionname;
    SIZE_T           programcounter;
    IMAGEHLP_LINE64  sourceinfo = { 0 };
    BYTE             symbolbuffer [sizeof(SYMBOL_INFO) + (MAXSYMBOLNAMELENGTH * sizeof(WCHAR)) - 1] = { 0 };

    if (m_status & CALLSTACK_STATUS_INCOMPLETE) {
        // This call stack appears to be incomplete. Using StackWalk64 may be
        // more reliable.
        report(L"    HINT: The following call stack may be incomplete. Setting \"StackWalkMethod\"\n"
               L"      in the vld.ini file to \"safe\" instead of \"fast\" may result in a more\n"
               L"      complete stack trace.\n");
    }

    // Initialize structures passed to the symbol handler.
    functioninfo = (SYMBOL_INFO*)&symbolbuffer;
    functioninfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    functioninfo->MaxNameLen = MAXSYMBOLNAMELENGTH;
    sourceinfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    // Iterate through each frame in the call stack.
    for (frame = 0; frame < m_size; frame++) {
        // Try to get the source file and line number associated with
        // this program counter address.
        programcounter = (*this)[frame];
        EnterCriticalSection(&symbollock);
        if ((foundline = SymGetLineFromAddrW64(currentprocess, programcounter, &displacement, &sourceinfo)) == TRUE) {
            if (!showinternalframes) {
                _wcslwr_s(sourceinfo.FileName, wcslen(sourceinfo.FileName) + 1);
                if (wcsstr(sourceinfo.FileName, L"afxmem.cpp") ||
                    wcsstr(sourceinfo.FileName, L"dbgheap.c") ||
                    wcsstr(sourceinfo.FileName, L"malloc.c") ||
                    wcsstr(sourceinfo.FileName, L"new.cpp") ||
                    wcsstr(sourceinfo.FileName, L"newaop.cpp")) {
                    // Don't show frames in files internal to the heap.
                    continue;
                }
            }
        }

        // Try to get the name of the function containing this program
        // counter address.
        if (SymFromAddrW(currentprocess, (*this)[frame], &displacement64, functioninfo)) {
            functionname = functioninfo->Name;
        }
        else {
            functionname = L"(Function name unavailable)";
        }
        LeaveCriticalSection(&symbollock);

        // Display the current stack frame's information.
        if (foundline) {
            report(L"    %s (%d): %s\n", sourceinfo.FileName, sourceinfo.LineNumber, functionname);
        }
        else {
            report(L"    " ADDRESSFORMAT L" (File and line number not available): ", (*this)[frame]);
            report(L"%s\n", functionname);
        }
    }
}

// push_back - Pushes a frame's program counter onto the CallStack. Pushes are
//   always appended to the back of the chunk list (aka the "top" chunk).
//
//   Note: This function will allocate additional memory as necessary to make
//     room for new program counter addresses.
//
//  - programcounter (IN): The program counter address of the frame to be pushed
//      onto the CallStack.
//
//  Return Value:
//
//    None.
//
VOID CallStack::push_back (const SIZE_T programcounter)
{
    CallStack::chunk_t *chunk;

    if (m_size == m_capacity) {
        // At current capacity. Allocate additional storage.
        chunk = new CallStack::chunk_t;
        chunk->next = NULL;
        m_topchunk->next = chunk;
        m_topchunk = chunk;
        m_topindex = 0;
        m_capacity += CALLSTACKCHUNKSIZE;
    }
    else if (m_topindex == CALLSTACKCHUNKSIZE) {
        // There is more capacity, but not in this chunk. Go to the next chunk.
        // Note that this only happens if this CallStack has previously been
        // cleared (clearing resets the data, but doesn't give up any allocated
        // space).
        m_topchunk = m_topchunk->next;
        m_topindex = 0;
    }

    m_topchunk->frames[m_topindex++] = programcounter;
    m_size++;
}

// getstacktrace - Traces the stack as far back as possible, or until 'maxdepth'
//   frames have been traced. Populates the CallStack with one entry for each
//   stack frame traced.
//
//   Note: This function uses a very efficient method to walk the stack from
//     frame to frame, so it is quite fast. However, unconventional stack frames
//     (such as those created when frame pointer omission optimization is used)
//     will not be successfully walked by this function and will cause the
//     stack trace to terminate prematurely.
//
//  - maxdepth (IN): Maximum number of frames to trace back.
//
//  - framepointer (IN): Frame (base) pointer at which to begin the stack trace.
//      If NULL, then the stack trace will begin at this function.
//
//  Return Value:
//
//    None.
//
VOID FastCallStack::getstacktrace (UINT32 maxdepth, SIZE_T *framepointer)
{
    UINT32  count = 0;

    if (framepointer == NULL) {
        // Begin the stack trace with the current frame. Obtain the current
        // frame pointer.
        FRAMEPOINTER(framepointer);
    }

    while (count < maxdepth) {
        if ((SIZE_T*)*framepointer < framepointer) {
            if ((SIZE_T*)*framepointer == NULL) {
                // Looks like we reached the end of the stack.
                break;
            }
            else {
                // Invalid frame pointer. Frame pointer addresses should always
                // increase as we move up the stack.
                m_status |= CALLSTACK_STATUS_INCOMPLETE;
                break;
            }
        }
        if ((SIZE_T)*framepointer & (sizeof(SIZE_T*) - 1)) {
            // Invalid frame pointer. Frame pointer addresses should always
            // be aligned to the size of a pointer. This probably means that
            // we've encountered a frame that was created by a module built with
            // frame pointer omission (FPO) optimization turned on.
            m_status |= CALLSTACK_STATUS_INCOMPLETE;
            break;
        }
        if (IsBadReadPtr((SIZE_T*)*framepointer, sizeof(SIZE_T*))) {
            // Bogus frame pointer. Again, this probably means that we've
            // encountered a frame built with FPO optimization.
            m_status |= CALLSTACK_STATUS_INCOMPLETE;
            break;
        }
        count++;
        push_back(*(framepointer + 1));
        framepointer = (SIZE_T*)*framepointer;
    }
}

// getstacktrace - Traces the stack as far back as possible, or until 'maxdepth'
//   frames have been traced. Populates the CallStack with one entry for each
//   stack frame traced.
//
//   Note: This function uses a documented Windows API to walk the stack. This
//     API is supposed to be the most reliable way to walk the stack. It claims
//     to be able to walk stack frames that do not follow the conventional stack
//     frame layout. However, this robustness comes at a cost: it is *extremely*
//     slow compared to walking frames by following frame (base) pointers.
//
//  - maxdepth (IN): Maximum number of frames to trace back.
//
//  - framepointer (IN): Frame (base) pointer at which to begin the stack trace.
//      If NULL, then the stack trace will begin at this function.
//
//  Return Value:
//
//    None.
//
VOID SafeCallStack::getstacktrace (UINT32 maxdepth, SIZE_T *framepointer)
{
    DWORD        architecture;
    CONTEXT      context;
    UINT32       count = 0;
    STACKFRAME64 frame;
    SIZE_T       programcounter;
    SIZE_T       stackpointer;

    if (framepointer == NULL) {
        // Begin the stack trace with the current frame. Obtain the current
        // frame pointer.
        FRAMEPOINTER(framepointer);
    }

    // Get the required values for initialization of the STACKFRAME64 structure
    // to be passed to StackWalk64(). Required fields are AddrPC and AddrFrame.
#if defined(_M_IX86) || defined(_M_X64)
    architecture   = X86X64ARCHITECTURE;
    programcounter = *(framepointer + 1);
    stackpointer   = *framepointer;  // An approximation.
    context.BPREG  = *framepointer;
    context.IPREG  = programcounter;
    context.SPREG  = stackpointer;
#else
// If you want to retarget Visual Leak Detector to another processor
// architecture then you'll need to provide architecture-specific code to
// obtain the program counter and stack pointer from the given frame pointer.
#error "Visual Leak Detector is not supported on this architecture."
#endif // _M_IX86 || _M_X64

    // Initialize the STACKFRAME64 structure.
    memset(&frame, 0x0, sizeof(frame));
    frame.AddrFrame.Offset = *framepointer;
    frame.AddrFrame.Mode   = AddrModeFlat;
    frame.AddrPC.Offset    = programcounter;
    frame.AddrPC.Mode      = AddrModeFlat;
    frame.AddrStack.Offset = stackpointer;
    frame.AddrStack.Mode   = AddrModeFlat;

    // Walk the stack.
    EnterCriticalSection(&stackwalklock);
    while (count < maxdepth) {
        count++;
        if (!StackWalk64(architecture, currentprocess, currentthread, &frame, &context, NULL,
                         SymFunctionTableAccess64, SymGetModuleBase64, NULL)) {
            // Couldn't trace back through any more frames.
            break;
        }
        if (frame.AddrFrame.Offset == 0) {
            // End of stack.
            break;
        }

        // Push this frame's program counter onto the CallStack.
        push_back((SIZE_T)frame.AddrPC.Offset);
    }
    LeaveCriticalSection(&stackwalklock);
}
