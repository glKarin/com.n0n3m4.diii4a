////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - VisualLeakDetector Class Definition
//  Copyright (c) 2005-2008 Dan Moulding
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

#include <cstdio>
#include <windows.h>
#include "callstack.h" // Provides a custom class for handling call stacks.
#include "map.h"       // Provides a custom STL-like map template.
#include "ntapi.h"     // Provides access to NT APIs.
#include "set.h"       // Provides a custom STL-like set template.
#include "utility.h"   // Provides miscellaneous utility functions.

#define MAXMODULELISTLENGTH 512     // Maximum module list length, in characters.
#define SELFTESTTEXTA       "Memory Leak Self-Test"
#define SELFTESTTEXTW       L"Memory Leak Self-Test"
#define VLDREGKEYPRODUCT    L"Software\\Visual Leak Detector"
#define VLDVERSION          L"1.9g"

// The Visual Leak Detector APIs.
extern "C" __declspec(dllexport) void VLDDisable ();
extern "C" __declspec(dllexport) void VLDEnable ();

// Data is collected for every block allocated from any heap in the process.
// The data is stored in this structure and these structures are stored in
// a BlockMap which maps each of these structures to its corresponding memory
// block.
typedef struct blockinfo_s {
    CallStack *callstack;
    SIZE_T     serialnumber;
    SIZE_T     size;
} blockinfo_t;

// BlockMaps map memory blocks (via their addresses) to blockinfo_t structures.
typedef Map<LPCVOID, blockinfo_t*> BlockMap;

// Information about each heap in the process is kept in this map. Primarily
// this is used for mapping heaps to all of the blocks allocated from those
// heaps.
typedef struct heapinfo_s {
    BlockMap blockmap;   // Map of all blocks allocated from this heap.
    UINT32   flags;      // Heap status flags:
#define VLD_HEAP_CRT 0x1 //   If set, this heap is a CRT heap (i.e. the CRT uses it for new/malloc).
} heapinfo_t;

// HeapMaps map heaps (via their handles) to BlockMaps.
typedef Map<HANDLE, heapinfo_t*> HeapMap;

// This structure stores information, primarily the virtual address range, about
// a given module and can be used with the Set template because it supports the
// '<' operator (sorts by virtual address range).
typedef struct moduleinfo_s {
    BOOL operator < (const struct moduleinfo_s &other) const
    {
        if (addrhigh < other.addrlow) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    SIZE_T addrhigh;                 // Highest address within the module's virtual address space (i.e. base + size).
    SIZE_T addrlow;                  // Lowest address within the module's virtual address space (i.e. base address).
    UINT32 flags;                    // Module flags:
#define VLD_MODULE_EXCLUDED      0x1 //   If set, this module is excluded from leak detection.
#define VLD_MODULE_SYMBOLSLOADED 0x2 //   If set, this module's debug symbols have been loaded.
    LPCSTR name;                     // The module's name (e.g. "kernel32.dll").
    LPCSTR path;                     // The fully qualified path from where the module was loaded.
} moduleinfo_t;

// ModuleSets store information about modules loaded in the process.
typedef Set<moduleinfo_t> ModuleSet;

// Thread local storage structure. Every thread in the process gets its own copy
// of this structure. Thread specific information, such as the currentl leak
// detection status (enabled or disabled) and the address that initiated the
// current allocation is stored here.
typedef struct tls_s {
    SIZE_T addrfp;           // Frame pointer at the first call that entered VLD's code for the current allocation.
    UINT32 flags;            // Thread-local status flags:
#define VLD_TLS_CRTALLOC 0x1 //   If set, the current allocation is a CRT allocation.
#define VLD_TLS_DISABLED 0x2 //   If set, memory leak detection is disabled for the current thread.
#define VLD_TLS_ENABLED  0x4 //   If set, memory leak detection is enabled for the current thread.
    DWORD  threadid;         // Thread ID of the thread that owns this TLS structure.
} tls_t;

// The TlsSet allows VLD to keep track of all thread local storage structures
// allocated in the process.
typedef Set<tls_t*> TlsSet;

////////////////////////////////////////////////////////////////////////////////
//
// The VisualLeakDetector Class
//
//   One global instance of this class is instantiated. Upon construction it
//   patches the import address table (IAT) of every other module loaded in the
//   process (see the "patchimport" utility function) to allow key Windows heap
//   APIs to be patched through to, or redirected to, functions provided by VLD.
//   Patching the IATs in this manner allows VLD to be made aware of all
//   relevant heap activity, making it possible for VLD to detect and trace
//   memory leaks.
//
//   The one global instance of this class is constructed within the context of
//   the process' main thread during process initialization and is destroyed in
//   the same context during process termination.
//
//   When the VisualLeakDetector object is destroyed, it consults its internal
//   datastructures, looking for any memory that has not been freed. A memory
//   leak report is then generated, indicating any memory leaks that may have
//   been identified.
//
//   This class is derived from IMalloc so that it can provide an implementation
//   of the IMalloc COM interface in order to support detection of COM-based
//   memory leaks. However, this implementation of IMalloc is actually just a
//   thin wrapper around the system's implementation of IMalloc.
//
class VisualLeakDetector : public IMalloc
{
public:
    VisualLeakDetector();
    ~VisualLeakDetector();

    // Public IMalloc methods - for support of COM-based memory leak detection.
    ULONG __stdcall AddRef ();
    LPVOID __stdcall Alloc (ULONG size);
    INT __stdcall DidAlloc (LPVOID mem);
    VOID __stdcall Free (LPVOID mem);
    ULONG __stdcall GetSize (LPVOID mem);
    VOID __stdcall HeapMinimize ();
    HRESULT __stdcall QueryInterface (REFIID iid, LPVOID *object);
    LPVOID __stdcall Realloc (LPVOID mem, ULONG size);
    ULONG __stdcall Release ();

private:
    // Import patching replacement functions - see each function definition for details.
    static HRESULT __stdcall _CoGetMalloc (DWORD context, LPMALLOC *imalloc);
    static LPVOID __stdcall _CoTaskMemAlloc (ULONG size);
    static LPVOID __stdcall _CoTaskMemRealloc (LPVOID mem, ULONG size);
    static void* __cdecl _crt80d__calloc_dbg (size_t num, size_t size, int type, const char *file, int line);
    static void* __cdecl _crt80d__malloc_dbg (size_t size, int type, const char *file, int line);
    static void* __cdecl _crt80d__realloc_dbg (void *mem, size_t size, int type, const char *file, int line);
    static void* __cdecl _crt80d__scalar_new_dbg (unsigned int size, int type, const char *file, int line);
    static void* __cdecl _crt80d__vector_new_dbg (unsigned int size, int type, const char *file, int line);
    static void* __cdecl _crt80d_calloc (size_t num, size_t size);
    static void* __cdecl _crt80d_malloc (size_t size);
    static void* __cdecl _crt80d_realloc (void *mem, size_t size);
    static void* __cdecl _crt80d_scalar_new (unsigned int size);
    static void* __cdecl _crt80d_vector_new (unsigned int size);
    static void* __cdecl _crtd__calloc_dbg (size_t num, size_t size, int type, const char *file, int line);
    static void* __cdecl _crtd__malloc_dbg (size_t size, int type, const char *file, int line);
    static void* __cdecl _crtd__realloc_dbg (void *mem, size_t size, int type, const char *file, int line);
    static void* __cdecl _crtd__scalar_new_dbg (unsigned int size, int type, const char *file, int line);
    static void* __cdecl _crtd_calloc (size_t num, size_t size);
    static void* __cdecl _crtd_malloc (size_t size);
    static void* __cdecl _crtd_realloc (void *mem, size_t size);
    static void* __cdecl _crtd_scalar_new (unsigned int size);
    static FARPROC __stdcall _GetProcAddress(HMODULE module, LPCSTR procname);
    static HANDLE __stdcall _HeapCreate (DWORD options, SIZE_T initsize, SIZE_T maxsize);
    static BOOL __stdcall _HeapDestroy (HANDLE heap);
    static NTSTATUS __stdcall _LdrLoadDll (LPWSTR searchpath, PDWORD flags, unicodestring_t *modulename,
                                           PHANDLE modulehandle);
    static void* __cdecl _mfc42d__scalar_new_dbg (unsigned int size, const char *file, int line);
    static void* __cdecl _mfc42d_scalar_new (unsigned int size);
    static void* __cdecl _mfc80d__scalar_new_dbg (unsigned int size, const char *file, int line);
    static void* __cdecl _mfc80d__vector_new_dbg (unsigned int size, const char *file, int line);
    static void* __cdecl _mfc80d_scalar_new (unsigned int size);
    static void* __cdecl _mfc80d_vector_new (unsigned int size);
    static LPVOID __stdcall _RtlAllocateHeap (HANDLE heap, DWORD flags, SIZE_T size);
    static BOOL __stdcall _RtlFreeHeap (HANDLE heap, DWORD flags, LPVOID mem);
    static LPVOID __stdcall _RtlReAllocateHeap (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T size);

    // Private functions - see each function definition for details.
    static BOOL __stdcall addloadedmodule (PCWSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context);
    VOID attachtoloadedmodules (ModuleSet *newmodules);
    LPWSTR buildsymbolsearchpath ();
    VOID configure ();
    static BOOL __stdcall detachfrommodule (PCWSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context);
    BOOL enabled ();
    SIZE_T eraseduplicates (const BlockMap::Iterator &element);
    tls_t* gettls ();
    VOID mapblock (HANDLE heap, LPCVOID mem, SIZE_T size, SIZE_T framepointer, BOOL crtalloc);
    VOID mapheap (HANDLE heap);
    VOID remapblock (HANDLE heap, LPCVOID mem, LPCVOID newmem, SIZE_T size, SIZE_T framepointer, BOOL crtalloc);
    VOID reportconfig ();
    VOID reportleaks (HANDLE heap);
    VOID unmapblock (HANDLE heap, LPCVOID mem);
    VOID unmapheap (HANDLE heap);

    // Private data.
    WCHAR                m_forcedmodulelist [MAXMODULELISTLENGTH]; // List of modules to be forcefully included in leak detection.
    HeapMap             *m_heapmap;           // Map of all active heaps in the process.
    IMalloc             *m_imalloc;           // Pointer to the system implementation of IMalloc.
    SIZE_T               m_leaksfound;        // Total number of leaks found.
    ModuleSet           *m_loadedmodules;     // Contains information about all modules loaded in the process.
    CRITICAL_SECTION     m_loaderlock;        // Serializes the attachment of newly loaded modules.
    CRITICAL_SECTION     m_maplock;           // Serializes access to the heap and block maps.
    SIZE_T               m_maxdatadump;       // Maximum number of user-data bytes to dump for each leaked block.
    UINT32               m_maxtraceframes;    // Maximum number of frames per stack trace for each leaked block.
    CRITICAL_SECTION     m_moduleslock;       // Protects accesses to the "loaded modules" ModuleSet.
    UINT32               m_options;           // Configuration options:
#define VLD_OPT_AGGREGATE_DUPLICATES    0x1   //   If set, aggregate duplicate leaks in the leak report.
#define VLD_OPT_MODULE_LIST_INCLUDE     0x2   //   If set, modules in the module list are included, all others are excluded.
#define VLD_OPT_REPORT_TO_DEBUGGER      0x4   //   If set, the memory leak report is sent to the debugger.
#define VLD_OPT_REPORT_TO_FILE          0x8   //   If set, the memory leak report is sent to a file.
#define VLD_OPT_SAFE_STACK_WALK         0x10  //   If set, the stack is walked using the "safe" method (StackWalk64).
#define VLD_OPT_SELF_TEST               0x20  //   If set, peform a self-test to verify memory leak self-checking.
#define VLD_OPT_SLOW_DEBUGGER_DUMP      0x40  //   If set, inserts a slight delay between sending output to the debugger.
#define VLD_OPT_START_DISABLED          0x80  //   If set, memory leak detection will initially disabled.
#define VLD_OPT_TRACE_INTERNAL_FRAMES   0x100 //   If set, include useless frames (e.g. internal to VLD) in call stacks.
#define VLD_OPT_UNICODE_REPORT          0x200 //   If set, the leak report will be encoded UTF-16 instead of ASCII.
#define VLD_OPT_VLDOFF                  0x400 //   If set, VLD will be completely deactivated. It will not attach to any modules.
    static patchentry_t  m_patchtable [];     // Table of imports patched for attaching VLD to other modules.
    FILE                *m_reportfile;        // File where the memory leak report may be sent to.
    WCHAR                m_reportfilepath [MAX_PATH]; // Full path and name of file to send memory leak report to.
    const char          *m_selftestfile;      // Filename where the memory leak self-test block is leaked.
    int                  m_selftestline;      // Line number where the memory leak self-test block is leaked.
    UINT32               m_status;            // Status flags:
#define VLD_STATUS_DBGHELPLINKED        0x1   //   If set, the explicit dynamic link to the Debug Help Library succeeded.
#define VLD_STATUS_INSTALLED            0x2   //   If set, VLD was successfully installed.
#define VLD_STATUS_NEVER_ENABLED        0x4   //   If set, VLD started disabled, and has not yet been manually enabled.
#define VLD_STATUS_FORCE_REPORT_TO_FILE 0x8   //   If set, the leak report is being forced to a file.
    DWORD                m_tlsindex;          // Thread-local storage index.
    CRITICAL_SECTION     m_tlslock;           // Protects accesses to the Set of TLS structures.
    TlsSet              *m_tlsset;            // Set of all all thread-local storage structres for the process.
    HMODULE              m_vldbase;           // Visual Leak Detector's own module handle (base address).

    // The Visual Leak Detector APIs are our friends.
    friend __declspec(dllexport) void VLDDisable ();
    friend __declspec(dllexport) void VLDEnable ();
};

// Configuration option default values
#define VLD_DEFAULT_MAX_DATA_DUMP    0xffffffff
#define VLD_DEFAULT_MAX_TRACE_FRAMES 0xffffffff
#define VLD_DEFAULT_REPORT_FILE_NAME L".\\memory_leak_report.txt"
