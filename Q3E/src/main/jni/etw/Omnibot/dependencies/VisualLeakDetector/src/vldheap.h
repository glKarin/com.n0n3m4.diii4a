////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Internal C++ Heap Management Definitions
//  Copyright (c) 2006 Dan Moulding
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

#define GAPSIZE 4

// Memory block header structure used internally by the debug CRT. All blocks
// allocated by the CRT are allocated from the CRT heap and, in debug mode, they
// have this header prepended to them (there's also a trailer appended at the
// end, but we're not interested in that).
typedef struct crtdbgblockheader_s
{
    struct crtblockheader_s *next;          // Pointer to the next block in the list of blocks allocated from the CRT heap.
    struct crtblockheader_s *prev;          // Pointer to the previous block in the list of blocks allocated from the CRT heap.
    char                    *file;          // Source file where this block was allocated.
    int                      line;          // Line of code, within the above file, where this block was allocated.
#ifdef _WIN64
    int                      use;           // This block's "use type": see below.
    size_t                   size;          // Size of the data portion of the block.
#else
    size_t                   size;          // Size of the data portion of the block.
    int                      use;           // This block's "use type":
#endif // _WIN64
#define CRT_USE_FREE     0                  //   This block has been freed.
#define CRT_USE_NORMAL   1                  //   This is a normal (user) block.
#define CRT_USE_INTERNAL 2                  //   This block is used internally by the CRT.
#define CRT_USE_IGNORE   3                  //   This block is a specially tagged block that is ignored during some debug error checking.
#define CRT_USE_CLIENT   4                  //   This block is a specially tagged block with special use defined by the user application.
    long                     request;       // This block's "request" number. Basically a serial number.
    unsigned char            gap [GAPSIZE]; // No-man's land buffer zone, for buffer overrun/underrun checking.
} crtdbgblockheader_t;

// Macro to strip off any sub-type information stored in a block's "use type".
#define CRT_USE_TYPE(use) (use & 0xFFFF)

// Memory block header structure used internally by VLD. All internally
// allocated blocks are allocated from VLD's private heap and have this header
// prepended to them.
typedef struct vldblockheader_s
{
    struct vldblockheader_s *next;          // Pointer to the next block in the list of internally allocated blocks.
    struct vldblockheader_s *prev;          // Pointer to the preceding block in the list of internally allocated blocks.
    const char              *file;          // Name of the file where this block was allocated.
    int                      line;          // Line number within the above file where this block was allocated.
    unsigned int             size;          // The size of this memory block, not including this header.
    SIZE_T                   serialnumber;  // Each block is assigned a unique serial number, starting from zero.
} vldblockheader_t;

// Data-to-Header and Header-to-Data conversion
#define CRTDBGBLOCKHEADER(d) (crtdbgblockheader_t*)(((PBYTE)d) - sizeof(crtdbgblockheader_t))
#define CRTDBGBLOCKDATA(h) (LPVOID)(((PBYTE)h) + sizeof(crtdbgblockheader_t))
#define VLDBLOCKHEADER(d) (vldblockheader_t*)(((PBYTE)d) - sizeof(vldblockheader_t))
#define VLDBLOCKDATA(h) (LPVOID)(((PBYTE)h) + sizeof(vldblockheader_t))

// new and delete operators for allocating from VLD's private heap.
void operator delete (void *block);
void operator delete [] (void *block);
void operator delete (void *block, const char *file, int line);
void operator delete [] (void *block, const char *file, int line);
void* operator new (unsigned int size, const char *file, int line);
void* operator new [] (unsigned int size, const char *file, int line);

// All calls to the new operator from within VLD are mapped to the version of
// new that allocates from VLD's private heap.
#define new new(__FILE__, __LINE__)
