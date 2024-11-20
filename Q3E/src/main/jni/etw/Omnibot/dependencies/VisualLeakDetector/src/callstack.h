////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - CallStack Class Definitions
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

#pragma once

#ifndef VLDBUILD
#error \
"This header should only be included by Visual Leak Detector when building it from source. \
Applications should never include this header."
#endif

#include <windows.h>

#define CALLSTACKCHUNKSIZE 32 // Number of frame slots in each CallStack chunk.

////////////////////////////////////////////////////////////////////////////////
//
//  The CallStack Class
//
//    CallStack objects can be used for obtaining, storing, and displaying the
//    call stack at a given point during program execution.
//
//    The primary data structure used by the CallStack is similar in concept to
//    a STL vector, but is specifically tailored for use by VLD, making it more
//    efficient than a standard STL vector.
//
//    Inside the CallStack are a number of "chunks" which are arranged in a
//    linked list. Each chunk contains an array of frames (each frame is
//    represented by a program counter address). If we run out of space when
//    pushing new frames onto an existing chunk in the CallStack chunk list,
//    then a new chunk is allocated and appended to the end of the list. In this
//    way, the CallStack can grow dynamically as needed. New frames are always
//    pushed onto the chunk at the end of the list known as the "top" chunk.
//
class CallStack
{
public:
    CallStack ();
    CallStack (const CallStack &other);
    ~CallStack ();

    // Public APIs - see each function definition for details.
    VOID clear ();
    VOID dump (BOOL showinternalframes) const;
    virtual VOID getstacktrace (UINT32 maxdepth, SIZE_T *framepointer) = 0;
    CallStack& operator = (const CallStack &other);
    BOOL operator == (const CallStack &other) const;
    SIZE_T operator [] (UINT32 index) const;
    VOID push_back (const SIZE_T programcounter);

protected:
    // Protected data.
    UINT32 m_status;                    // Status flags:
#define CALLSTACK_STATUS_INCOMPLETE 0x1 //   If set, the stack trace stored in this CallStack appears to be incomplete.

private:
    // The chunk list is made of a linked list of Chunks.
    typedef struct chunk_s {
        struct chunk_s *next;                        // Pointer to the next chunk in the chunk list.
        SIZE_T          frames [CALLSTACKCHUNKSIZE]; // Pushed frames (program counter addresses) are stored in this array.
    } chunk_t;

    // Private data.
    UINT32              m_capacity; // Current capacity limit (in frames)
    UINT32              m_size;     // Current size (in frames)
    CallStack::chunk_t  m_store;    // Pointer to the underlying data store (i.e. head of the chunk list)
    CallStack::chunk_t *m_topchunk; // Pointer to the chunk at the top of the stack
    UINT32              m_topindex; // Index, within the top chunk, of the top of the stack
};


////////////////////////////////////////////////////////////////////////////////
//
//  The FastCallStack Class
//
//    This class is a specialization of the CallStack class which provides a
//    very fast stack tracing function.
//
class FastCallStack : public CallStack
{
public:
    VOID getstacktrace (UINT32 maxdepth, SIZE_T *framepointer);
};

////////////////////////////////////////////////////////////////////////////////
//
//  The SafeCallStack Class
//
//    This class is a specialization of the CallStack class which provides a
//    more robust, but quite slow, stack tracing function.
//
class SafeCallStack : public CallStack
{
public:
    VOID getstacktrace (UINT32 maxdepth, SIZE_T *framepointer);
};
