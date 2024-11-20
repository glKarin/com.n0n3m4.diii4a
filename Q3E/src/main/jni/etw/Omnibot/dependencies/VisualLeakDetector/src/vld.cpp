////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - VisualLeakDetector Class Implementation
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

#pragma comment(lib, "dbghelp.lib")

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <sys/stat.h>
#include <windows.h>
#define __out_xcount(x) // Workaround for the specstrings.h bug in the Platform SDK.
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>    // Provides symbol handling services.
#define VLDBUILD        // Declares that we are building Visual Leak Detector.
#include "callstack.h"  // Provides a class for handling call stacks.
#include "map.h"        // Provides a lightweight STL-like map template.
#include "ntapi.h"      // Provides access to NT APIs.
#include "set.h"        // Provides a lightweight STL-like set template.
#include "utility.h"    // Provides various utility functions.
#include "vldheap.h"    // Provides internal new and delete operators.
#include "vldint.h"     // Provides access to the Visual Leak Detector internals.

#define BLOCKMAPRESERVE     64  // This should strike a balance between memory use and a desire to minimize heap hits.
#define HEAPMAPRESERVE      2   // Usually there won't be more than a few heaps in the process, so this should be small.
#define MAXSYMBOLNAMELENGTH 256 // Maximum symbol name length that we will allow. Longer names will be truncated.
#define MODULESETRESERVE    16  // There are likely to be several modules loaded in the process.

// Imported global variables.
extern vldblockheader_t *vldblocklist;
extern HANDLE            vldheap;
extern CRITICAL_SECTION  vldheaplock;

// Global variables.
HANDLE           currentprocess; // Pseudo-handle for the current process.
HANDLE           currentthread;  // Pseudo-handle for the current thread.
CRITICAL_SECTION imagelock;      // Serializes calls to the Debug Help Library PE image access APIs.
HANDLE           processheap;    // Handle to the process's heap (COM allocations come from here).
CRITICAL_SECTION stackwalklock;  // Serializes calls to StackWalk64 from the Debug Help Library.
CRITICAL_SECTION symbollock;     // Serializes calls to the Debug Help Library symbold handling APIs.

// Function pointer types for explicit dynamic linking with functions listed in
// the import patch table.
typedef void* (__cdecl *_calloc_dbg_t) (size_t, size_t, int, const char*, int);
typedef void* (__cdecl *_malloc_dbg_t) (size_t, int, const char *, int);
typedef void* (__cdecl *_realloc_dbg_t) (void *, size_t, int, const char *, int);
typedef void* (__cdecl *calloc_t) (size_t, size_t);
typedef HRESULT (__stdcall *CoGetMalloc_t) (DWORD, LPMALLOC *);
typedef LPVOID (__stdcall *CoTaskMemAlloc_t) (ULONG);
typedef LPVOID (__stdcall *CoTaskMemRealloc_t) (LPVOID, ULONG);
typedef void* (__cdecl *crt_new_dbg_t) (unsigned int, int, const char *, int);
typedef void* (__cdecl *malloc_t) (size_t);
typedef void* (__cdecl *mfc_new_dbg_t) (unsigned int, const char *, int);
typedef void* (__cdecl *new_t) (unsigned int);
typedef void* (__cdecl *realloc_t) (void *, size_t);

// Global function pointers for explicit dynamic linking with functions listed
// in the import patch table. Using explicit dynamic linking minimizes VLD's
// footprint by loading only modules that are actually used. These pointers will
// be linked to the real functions the first time they are used.
static CoGetMalloc_t      pCoGetMalloc            = NULL;
static CoTaskMemAlloc_t   pCoTaskMemAlloc         = NULL;
static CoTaskMemRealloc_t pCoTaskMemRealloc       = NULL;
static _calloc_dbg_t      pcrt80d__calloc_dbg     = NULL;
static _malloc_dbg_t      pcrt80d__malloc_dbg     = NULL;
static _realloc_dbg_t     pcrt80d__realloc_dbg    = NULL;
static crt_new_dbg_t      pcrt80d__scalar_new_dbg = NULL;
static crt_new_dbg_t      pcrt80d__vector_new_dbg = NULL;
static calloc_t           pcrt80d_calloc          = NULL;
static malloc_t           pcrt80d_malloc          = NULL;
static realloc_t          pcrt80d_realloc         = NULL;
static new_t              pcrt80d_scalar_new      = NULL;
static new_t              pcrt80d_vector_new      = NULL;
static _calloc_dbg_t      pcrtd__calloc_dbg       = NULL;
static _malloc_dbg_t      pcrtd__malloc_dbg       = NULL;
static _realloc_dbg_t     pcrtd__realloc_dbg      = NULL;
static crt_new_dbg_t      pcrtd__scalar_new_dbg   = NULL;
static calloc_t           pcrtd_calloc            = NULL;
static malloc_t           pcrtd_malloc            = NULL;
static realloc_t          pcrtd_realloc           = NULL;
static new_t              pcrtd_scalar_new        = NULL;
static mfc_new_dbg_t      pmfc42d__scalar_new_dbg = NULL;
static new_t              pmfc42d_scalar_new      = NULL;
static mfc_new_dbg_t      pmfc80d__scalar_new_dbg = NULL;
static mfc_new_dbg_t      pmfc80d__vector_new_dbg = NULL;
static new_t              pmfc80d_scalar_new      = NULL;
static new_t              pmfc80d_vector_new      = NULL;

// The one and only VisualLeakDetector object instance.
__declspec(dllexport) VisualLeakDetector vld;

// The import patch table: lists the heap-related API imports that VLD patches
// through to replacement functions provided by VLD. Having this table simply
// makes it more convenient to add additional IAT patches.
patchentry_t VisualLeakDetector::m_patchtable [] = {
    // Win32 heap APIs.
    "kernel32.dll", "GetProcAddress",     0x0, _GetProcAddress, // Not heap related, but can be used to obtain pointers to heap functions.
    "kernel32.dll", "HeapAlloc",          0x0, _RtlAllocateHeap,
    "kernel32.dll", "HeapCreate",         0x0, _HeapCreate,
    "kernel32.dll", "HeapDestroy",        0x0, _HeapDestroy,
    "kernel32.dll", "HeapFree",           0x0, _RtlFreeHeap,
    "kernel32.dll", "HeapReAlloc",        0x0, _RtlReAllocateHeap,

    // MFC new operators (exported by ordinal).
    "mfc42d.dll",   (LPCSTR)714,          0x0, _mfc42d__scalar_new_dbg,
    "mfc42d.dll",   (LPCSTR)711,          0x0, _mfc42d_scalar_new,
    // XXX MFC 7.x DLL new operators still need to be added to this
    //   table, but I don't know their ordinals.
    "mfc80d.dll",   (LPCSTR)895,          0x0, _mfc80d__scalar_new_dbg,
    "mfc80d.dll",   (LPCSTR)269,          0x0, _mfc80d__vector_new_dbg,
    "mfc80d.dll",   (LPCSTR)893,          0x0, _mfc80d_scalar_new,
    "mfc80d.dll",   (LPCSTR)267,          0x0, _mfc80d_vector_new,

    // CRT new operators and heap APIs.
    "msvcr80d.dll", "_calloc_dbg",        0x0, _crt80d__calloc_dbg,
    "msvcr80d.dll", "_malloc_dbg",        0x0, _crt80d__malloc_dbg,
    "msvcr80d.dll", "_realloc_dbg",       0x0, _crt80d__realloc_dbg,
    "msvcr80d.dll", "??2@YAPAXIHPBDH@Z",  0x0, _crt80d__scalar_new_dbg,
    "msvcr80d.dll", "??_U@YAPAXIHPBDH@Z", 0x0, _crt80d__vector_new_dbg,
    "msvcr80d.dll", "calloc",             0x0, _crt80d_calloc,
    "msvcr80d.dll", "malloc",             0x0, _crt80d_malloc,
    "msvcr80d.dll", "realloc",            0x0, _crt80d_realloc,
    "msvcr80d.dll", "??2@YAPAXI@Z",       0x0, _crt80d_scalar_new,
    "msvcr80d.dll", "??_U@YAPAXI@Z",      0x0, _crt80d_vector_new,
    "msvcrtd.dll",  "_calloc_dbg",        0x0, _crtd__calloc_dbg,
    "msvcrtd.dll",  "_malloc_dbg",        0x0, _crtd__malloc_dbg,
    "msvcrtd.dll",  "??2@YAPAXIHPBDH@Z",  0x0, _crtd__scalar_new_dbg,
    "msvcrtd.dll",  "_realloc_dbg",       0x0, _crtd__realloc_dbg,
    "msvcrtd.dll",  "calloc",             0x0, _crtd_calloc,
    "msvcrtd.dll",  "malloc",             0x0, _crtd_malloc,
    "msvcrtd.dll",  "realloc",            0x0, _crtd_realloc,
    "msvcrtd.dll",  "??2@YAPAXI@Z",       0x0, _crtd_scalar_new,

    // NT APIs.
    "ntdll.dll",    "RtlAllocateHeap",    0x0, _RtlAllocateHeap,
    "ntdll.dll",    "RtlFreeHeap",        0x0, _RtlFreeHeap,
    "ntdll.dll",    "RtlReAllocateHeap",  0x0, _RtlReAllocateHeap,

    // COM heap APIs.
    "ole32.dll",    "CoGetMalloc",        0x0, _CoGetMalloc,
    "ole32.dll",    "CoTaskMemAlloc",     0x0, _CoTaskMemAlloc,
    "ole32.dll",    "CoTaskMemRealloc",   0x0, _CoTaskMemRealloc
};

// Constructor - Initializes private data, loads configuration options, and
//   attaches Visual Leak Detector to all other modules loaded into the current
//   process.
//
VisualLeakDetector::VisualLeakDetector ()
{
    WCHAR      bom = BOM; // Unicode byte-order mark.
    HMODULE    kernel32;
    ModuleSet *newmodules;
    HMODULE    ntdll;
    LPWSTR     symbolpath;

    // Initialize configuration options and related private data.
    _wcsnset_s(m_forcedmodulelist, MAXMODULELISTLENGTH, '\0', _TRUNCATE);
    m_maxdatadump    = 0xffffffff;
    m_maxtraceframes = 0xffffffff;
    m_options        = 0x0;
    m_reportfile     = NULL;
    wcsncpy_s(m_reportfilepath, MAX_PATH, VLD_DEFAULT_REPORT_FILE_NAME, _TRUNCATE);
    m_status         = 0x0;

    // Load configuration options.
    configure();
    if (m_options & VLD_OPT_VLDOFF) {
        report(L"Visual Leak Detector is turned off.\n");
        return;
    }

    kernel32 = GetModuleHandle(L"kernel32.dll");
    ntdll = GetModuleHandle(L"ntdll.dll");

    // Initialize global variables.
    currentprocess    = GetCurrentProcess();
    currentthread     = GetCurrentThread();
    InitializeCriticalSection(&imagelock);
    LdrLoadDll        = (LdrLoadDll_t)GetProcAddress(ntdll, "LdrLoadDll");
    processheap       = GetProcessHeap();
    RtlAllocateHeap   = (RtlAllocateHeap_t)GetProcAddress(ntdll, "RtlAllocateHeap");
    RtlFreeHeap       = (RtlFreeHeap_t)GetProcAddress(ntdll, "RtlFreeHeap");
    RtlReAllocateHeap = (RtlReAllocateHeap_t)GetProcAddress(ntdll, "RtlReAllocateHeap");
    InitializeCriticalSection(&stackwalklock);
    InitializeCriticalSection(&symbollock);
    vldheap           = HeapCreate(0x0, 0, 0);
    InitializeCriticalSection(&vldheaplock);

    // Initialize remaining private data.
    m_heapmap         = new HeapMap;
    m_heapmap->reserve(HEAPMAPRESERVE);
    m_imalloc         = NULL;
    m_leaksfound      = 0;
    m_loadedmodules   = NULL;
    InitializeCriticalSection(&m_loaderlock);
    InitializeCriticalSection(&m_maplock);
    InitializeCriticalSection(&m_moduleslock);
    m_selftestfile    = __FILE__;
    m_selftestline    = 0;
    m_tlsindex        = TlsAlloc();
    InitializeCriticalSection(&m_tlslock);
    m_tlsset          = new TlsSet;

    if (m_options & VLD_OPT_SELF_TEST) {
        // Self-test mode has been enabled. Intentionally leak a small amount of
        // memory so that memory leak self-checking can be verified.
        if (m_options & VLD_OPT_UNICODE_REPORT) {
            wcsncpy_s(new WCHAR [wcslen(SELFTESTTEXTW) + 1], wcslen(SELFTESTTEXTW) + 1, SELFTESTTEXTW, _TRUNCATE);
            m_selftestline = __LINE__ - 1;
        }
        else {
            strncpy_s(new CHAR [strlen(SELFTESTTEXTA) + 1], strlen(SELFTESTTEXTA) + 1, SELFTESTTEXTA, _TRUNCATE);
            m_selftestline = __LINE__ - 1;
        }
    }
    if (m_options & VLD_OPT_START_DISABLED) {
        // Memory leak detection will initially be disabled.
        m_status |= VLD_STATUS_NEVER_ENABLED;
    }
    if (m_options & VLD_OPT_REPORT_TO_FILE) {
        // Reporting to file enabled.
        if (m_options & VLD_OPT_UNICODE_REPORT) {
            // Unicode data encoding has been enabled. Write the byte-order
            // mark before anything else gets written to the file. Open the
            // file for binary writing.
            if (_wfopen_s(&m_reportfile, m_reportfilepath, L"wb") == EINVAL) {
                // Couldn't open the file.
                m_reportfile = NULL;
            }
            else {
                fwrite(&bom, sizeof(WCHAR), 1, m_reportfile);
                setreportencoding(unicode);
            }
        }
        else {
            // Open the file in text mode for ASCII output.
            if (_wfopen_s(&m_reportfile, m_reportfilepath, L"w") == EINVAL) {
                // Couldn't open the file.
                m_reportfile = NULL;
            }
            else {
                setreportencoding(ascii);
            }
        }
        if (m_reportfile == NULL) {
            report(L"WARNING: Visual Leak Detector: Couldn't open report file for writing: %s\n"
                   L"  The report will be sent to the debugger instead.\n", m_reportfilepath);
        }
        else {
            // Set the "report" function to write to the file.
            setreportfile(m_reportfile, m_options & VLD_OPT_REPORT_TO_DEBUGGER);
        }
    }
    if (m_options & VLD_OPT_SLOW_DEBUGGER_DUMP) {
        // Insert a slight delay between messages sent to the debugger for
        // output. (For working around a bug in VC6 where data sent to the
        // debugger gets lost if it's sent too fast).
        insertreportdelay();
    }

    // This is highly unlikely to happen, but just in case, check to be sure
    // we got a valid TLS index.
    if (m_tlsindex == TLS_OUT_OF_INDEXES) {
        report(L"ERROR: Visual Leak Detector could not be installed because thread local"
               L"  storage could not be allocated.");
        return;
    }

    // Initialize the symbol handler. We use it for obtaining source file/line
    // number information and function names for the memory leak report.
    symbolpath = buildsymbolsearchpath();
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (!SymInitializeW(currentprocess, symbolpath, FALSE)) {
        report(L"WARNING: Visual Leak Detector: The symbol handler failed to initialize (error=%lu).\n"
               L"    File and function names will probably not be available in call stacks.\n", GetLastError());
    }
    delete [] symbolpath;

    // Patch into kernel32.dll's calls to LdrLoadDll so that VLD can
    // dynamically attach to new modules loaded during runtime.
    patchimport(kernel32, ntdll, "ntdll.dll", "LdrLoadDll", _LdrLoadDll);

    // Attach Visual Leak Detector to every module loaded in the process.
    newmodules = new ModuleSet;
    newmodules->reserve(MODULESETRESERVE);
    EnumerateLoadedModulesW64(currentprocess, addloadedmodule, newmodules);
    attachtoloadedmodules(newmodules);
    m_loadedmodules = newmodules;
    m_status |= VLD_STATUS_INSTALLED;

    report(L"Visual Leak Detector Version " VLDVERSION L" installed.\n");
    if (m_status & VLD_STATUS_FORCE_REPORT_TO_FILE) {
        // The report is being forced to a file. Let the human know why.
        report(L"NOTE: Visual Leak Detector: Unicode-encoded reporting has been enabled, but the\n"
               L"  debugger is the only selected report destination. The debugger cannot display\n"
               L"  Unicode characters, so the report will also be sent to a file. If no file has\n"
               L"  been specified, the default file name is \"" VLD_DEFAULT_REPORT_FILE_NAME L"\".\n");

    }
    reportconfig();
}

// Destructor - Detaches Visual Leak Detector from all modules loaded in the
//   process, frees internally allocated resources, and generates the memory
//   leak report.
//
VisualLeakDetector::~VisualLeakDetector ()
{
    BlockMap::Iterator   blockit;
    BlockMap            *blockmap;
    size_t               count;
    DWORD                exitcode;
    vldblockheader_t    *header;
    HANDLE               heap;
    HeapMap::Iterator    heapit;
    SIZE_T               internalleaks = 0;
    const char          *leakfile = NULL;
    WCHAR                leakfilew [MAX_PATH];
    int                  leakline = 0;
    ModuleSet::Iterator  moduleit;
    SIZE_T               sleepcount;
    HANDLE               thread;
    BOOL                 threadsactive= FALSE;
    TlsSet::Iterator     tlsit;

    if (m_options & VLD_OPT_VLDOFF) {
        // VLD has been turned off.
        return;
    }

    if (m_status & VLD_STATUS_INSTALLED) {
        // Detach Visual Leak Detector from all previously attached modules.
        EnumerateLoadedModulesW64(currentprocess, detachfrommodule, NULL);

        // See if any threads that have ever entered VLD's code are still active.
        EnterCriticalSection(&m_tlslock);
        for (tlsit = m_tlsset->begin(); tlsit != m_tlsset->end(); ++tlsit) {
            if ((*tlsit)->threadid == GetCurrentThreadId()) {
                // Don't wait for the current thread to exit.
                continue;
            }

            sleepcount = 0;
            thread = OpenThread(THREAD_QUERY_INFORMATION, FALSE, (*tlsit)->threadid);
            if (thread == NULL) {
                // Couldn't query this thread. We'll assume that it exited.
                continue; // XXX should we check GetLastError()?
            }
            while (GetExitCodeThread(thread, &exitcode) == TRUE) {
                if (exitcode != STILL_ACTIVE) {
                    // This thread exited.
                    break;
                }
                else {
                    // There is still at least one other thread running. The CRT
                    // will stomp it dead when it cleans up, which is not a
                    // graceful way for a thread to go down. Warn about this,
                    // and wait until the thread has exited so that we know it
                    // can't still be off running somewhere in VLD's code.
                    threadsactive = TRUE;
                    Sleep(100);
                    sleepcount++;
                    if ((sleepcount % 100) == 0) {
                        // Just in case this takes a long time, let the human
                        // know we are still here and alive.
                        report(L"Visual Leak Detector: Waiting for threads to terminate...\n");
                    }
                }
            }
        }
        LeaveCriticalSection(&m_tlslock);

        if (m_status & VLD_STATUS_NEVER_ENABLED) {
            // Visual Leak Detector started with leak detection disabled and
            // it was never enabled at runtime. A lot of good that does.
            report(L"WARNING: Visual Leak Detector: Memory leak detection was never enabled.\n");
        }
        else {
            // Generate a memory leak report for each heap in the process.
            for (heapit = m_heapmap->begin(); heapit != m_heapmap->end(); ++heapit) {
                heap = (*heapit).first;
                reportleaks(heap);
            }

            // Show a summary.
            if (m_leaksfound == 0) {
                report(L"No memory leaks detected.\n");
            }
            else {
                report(L"Visual Leak Detector detected %lu memory leak", m_leaksfound);
                report((m_leaksfound > 1) ? L"s.\n" : L".\n");
            }
        }

        // Free resources used by the symbol handler.
        if (!SymCleanup(currentprocess)) {
            report(L"WARNING: Visual Leak Detector: The symbol handler failed to deallocate resources (error=%lu).\n",
                   GetLastError());
        }

        // Free internally allocated resources used by the heapmap and blockmap.
        for (heapit = m_heapmap->begin(); heapit != m_heapmap->end(); ++heapit) {
            blockmap = &(*heapit).second->blockmap;
            for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
                delete (*blockit).second->callstack;
                delete (*blockit).second;
            }
            delete blockmap;
        }
        delete m_heapmap;

        // Free internally allocated resources used by the loaded module set.
        for (moduleit = m_loadedmodules->begin(); moduleit != m_loadedmodules->end(); ++moduleit) {
            delete (*moduleit).name;
            delete (*moduleit).path;
        }
        delete m_loadedmodules;

        // Free internally allocated resources used for thread local storage.
        for (tlsit = m_tlsset->begin(); tlsit != m_tlsset->end(); ++tlsit) {
            delete *tlsit;
        }
        delete m_tlsset;

        // Do a memory leak self-check.
        header = vldblocklist;
        while (header) {
            // Doh! VLD still has an internally allocated block!
            // This won't ever actually happen, right guys?... guys?
            internalleaks++;
            leakfile = header->file;
            leakline = header->line;
            mbstowcs_s(&count, leakfilew, MAX_PATH, leakfile, _TRUNCATE);
            report(L"ERROR: Visual Leak Detector: Detected a memory leak internal to Visual Leak Detector!!\n");
            report(L"---------- Block %ld at " ADDRESSFORMAT L": %u bytes ----------\n", header->serialnumber,
                   VLDBLOCKDATA(header), header->size);
            report(L"  Call Stack:\n");
            report(L"    %s (%d): Full call stack not available.\n", leakfilew, leakline);
            if (m_maxdatadump != 0) {
                report(L"  Data:\n");
                if (m_options & VLD_OPT_UNICODE_REPORT) {
                    dumpmemoryw(VLDBLOCKDATA(header), (m_maxdatadump < header->size) ? m_maxdatadump : header->size);
                }
                else {
                    dumpmemorya(VLDBLOCKDATA(header), (m_maxdatadump < header->size) ? m_maxdatadump : header->size);
                }
            }
            report(L"\n");
            header = header->next;
        }
        if (m_options & VLD_OPT_SELF_TEST) {
            if ((internalleaks == 1) && (strcmp(leakfile, m_selftestfile) == 0) && (leakline == m_selftestline)) {
                report(L"Visual Leak Detector passed the memory leak self-test.\n");
            }
            else {
                report(L"ERROR: Visual Leak Detector: Failed the memory leak self-test.\n");
            }
        }

        if (threadsactive == TRUE) {
            report(L"WARNING: Visual Leak Detector: Some threads appear to have not terminated normally.\n"
                L"  This could cause inaccurate leak detection results, including false positives.\n");
        }
        report(L"Visual Leak Detector is now exiting.\n");
    }
    else {
        // VLD failed to load properly.
        delete m_heapmap;
        delete m_tlsset;
    }
    HeapDestroy(vldheap);

    DeleteCriticalSection(&imagelock);
    DeleteCriticalSection(&m_loaderlock);
    DeleteCriticalSection(&m_maplock);
    DeleteCriticalSection(&m_moduleslock);
    DeleteCriticalSection(&stackwalklock);
    DeleteCriticalSection(&symbollock);
    DeleteCriticalSection(&vldheaplock);

    if (m_tlsindex != TLS_OUT_OF_INDEXES) {
        TlsFree(m_tlsindex);
    }

    if (m_reportfile != NULL) {
        fclose(m_reportfile);
    }
}

// _CoGetMalloc - Calls to CoGetMalloc are patched through to this function.
//   This function returns a pointer to Visual Leak Detector's implementation
//   of the IMalloc interface, instead of returning a pointer to the system
//   implementation. This allows VLD's implementation of the IMalloc interface
//   (which is basically a thin wrapper around the system implementation) to be
//   invoked in place of the system implementation.
//
//  - context (IN): Reserved; value must be 1.
//
//  - imalloc (IN): Address of a pointer to receive the address of VLD's
//      implementation of the IMalloc interface.
//
//  Return Value:
//
//    Always returns S_OK.
//
HRESULT VisualLeakDetector::_CoGetMalloc (DWORD context, LPMALLOC *imalloc)
{
    HMODULE ole32;

    *imalloc = (LPMALLOC)&vld;

    if (pCoGetMalloc == NULL) {
        // This is the first call to this function. Link to the real
        // CoGetMalloc and get a pointer to the system implementation of the
        // IMalloc interface.
        ole32 = GetModuleHandle(L"ole32.dll");
        pCoGetMalloc = (CoGetMalloc_t)GetProcAddress(ole32, "CoGetMalloc");
        pCoGetMalloc(context, &vld.m_imalloc);
    }

    return S_OK;
}

// _CoTaskMemAlloc - Calls to CoTaskMemAlloc are patched through to this
//   function. This function is just a wrapper around the real CoTaskMemAlloc
//   that sets appropriate flags to be consulted when the memory is actually
//   allocated by RtlAllocateHeap.
//
//  - size (IN): Size of the memory block to allocate.
//
//  Return Value:
//
//    Returns the value returned from CoTaskMemAlloc.
//
LPVOID VisualLeakDetector::_CoTaskMemAlloc (ULONG size)
{
    LPVOID   block;
    SIZE_T   fp;
    HMODULE  ole32;
    tls_t   *tls = vld.gettls();

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pCoTaskMemAlloc == NULL) {
        // This is the first call to this function. Link to the real
        // CoTaskMemAlloc.
        ole32 = GetModuleHandle(L"ole32.dll");
        pCoTaskMemAlloc = (CoTaskMemAlloc_t)GetProcAddress(ole32, "CoTaskMemAlloc");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pCoTaskMemAlloc(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;
    
    return block;
}

// _CoTaskMemRealloc - Calls to CoTaskMemRealloc are patched through to this
//   function. This function is just a wrapper around the real CoTaskMemRealloc
//   that sets appropriate flags to be consulted when the memory is actually
//   allocated by RtlAllocateHeap.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size, in bytes, of the block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from CoTaskMemRealloc.
//
LPVOID VisualLeakDetector::_CoTaskMemRealloc (LPVOID mem, ULONG size)
{
    LPVOID   block;
    SIZE_T   fp;
    HMODULE  ole32;
    tls_t   *tls = vld.gettls();

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pCoTaskMemRealloc == NULL) {
        // This is the first call to this function. Link to the real
        // CoTaskMemRealloc.
        ole32 = GetModuleHandle(L"ole32.dll");
        pCoTaskMemRealloc = (CoTaskMemRealloc_t)GetProcAddress(ole32, "CoTaskMemRealloc");
    }

    // Do the allocation. The block will be mapped by _RtlReAllocateHeap.
    block = pCoTaskMemRealloc(mem, size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d__calloc_dbg - Calls to _calloc_dbg from msvcr80d.dll are patched
//   through to this function. This function is just a wrapper around the real
//   _calloc_dbg that sets appropriate flags to be consulted when the memory is
//   actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _calloc_dbg.
//
void* VisualLeakDetector::_crt80d__calloc_dbg (size_t num, size_t size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // _malloc_dbg is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d__calloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__calloc_dbg = (_calloc_dbg_t)GetProcAddress(msvcr80d, "_calloc_dbg");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrt80d__calloc_dbg(num, size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d__malloc_dbg - Calls to _malloc_dbg from msvcr80d.dll are patched
//   through to this function. This function is just a wrapper around the real
//   _malloc_dbg that sets appropriate flags to be consulted when the memory is
//   actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _malloc_dbg.
//
void* VisualLeakDetector::_crt80d__malloc_dbg (size_t size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // _malloc_dbg is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d__malloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__malloc_dbg = (_malloc_dbg_t)GetProcAddress(msvcr80d, "_malloc_dbg");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrt80d__malloc_dbg(size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d__realloc_dbg - Calls to _realloc_dbg from msvcr80d.dll are patched
//   through to this function. This function is just a wrapper around the real
//   _realloc_dbg that sets appropriate flags to be consulted when the memory is
//   actually allocated by RtlAllocateHeap.
//
//  - mem (IN): Pointer to the memory block to be reallocated.
//
//  - size (IN): The size of the memory block to reallocate.
//
//  - type (IN): The CRT "use type" of the block to be reallocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above filel, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _realloc_dbg.
//
void* VisualLeakDetector::_crt80d__realloc_dbg (void *mem, size_t size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // _realloc_dbg is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d__realloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _realloc_dbg.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__realloc_dbg = (_realloc_dbg_t)GetProcAddress(msvcr80d, "_realloc_dbg");
    }

    // Do the allocation. The block will be mapped by _RtlReAllocateHeap.
    block = pcrt80d__realloc_dbg(mem, size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d__scalar_new_dbg - Calls to the CRT's debug scalar new operator from
//   msvcr80d.dll are patched through to this function. This function is just a
//   wrapper around the real CRT debug scalar new operator that sets appropriate
//   flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug scalar new operator.
//
void* VisualLeakDetector::_crt80d__scalar_new_dbg (unsigned int size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // The debug new operator is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // new operator.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__scalar_new_dbg = (crt_new_dbg_t)GetProcAddress(msvcr80d, "??2@YAPAXIHPBDH@Z");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrt80d__scalar_new_dbg(size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d__vector_new_dbg - Calls to the CRT's debug vector new operator from
//   msvcr80d.dll are patched through to this function. This function is just a
//   wrapper around the real CRT debug vector new operator that sets appropriate
//   flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug vector new operator.
//
void* VisualLeakDetector::_crt80d__vector_new_dbg (unsigned int size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // The debug new operator is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d__vector_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // new operator.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d__vector_new_dbg = (crt_new_dbg_t)GetProcAddress(msvcr80d, "??_U@YAPAXIHPBDH@Z");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrt80d__vector_new_dbg(size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d_calloc - Calls to calloc from msvcr80d.dll are patched through to
//   this function. This function is just a wrapper around the real calloc that
//   sets appropriate flags to be consulted when the memory is actually
//   allocated by RtlAllocateHeap.
//
//  - num (IN): The number of blocks, of size 'size', to be allocated.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from calloc.
//
void* VisualLeakDetector::_crt80d_calloc (size_t num, size_t size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // malloc is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d_calloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_calloc = (calloc_t)GetProcAddress(msvcr80d, "calloc");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrt80d_calloc(num, size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d_malloc - Calls to malloc from msvcr80d.dll are patched through to
//   this function. This function is just a wrapper around the real malloc that
//   sets appropriate flags to be consulted when the memory is actually
//   allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from malloc.
//
void* VisualLeakDetector::_crt80d_malloc (size_t size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // malloc is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d_malloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_malloc = (malloc_t)GetProcAddress(msvcr80d, "malloc");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrt80d_malloc(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d_realloc - Calls to realloc from msvcr80d.dll are patched through to
//   this function. This function is just a wrapper around the real realloc that
//   sets appropriate flags to be consulted when the memory is actually
//   allocated by RtlAllocateHeap.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from realloc.
//
void* VisualLeakDetector::_crt80d_realloc (void *mem, size_t size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // realloc is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d_realloc == NULL) {
        // This is the first call to this function. Link to the real realloc.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_realloc = (realloc_t)GetProcAddress(msvcr80d, "realloc");
    }

    // Do the allocation. The block will be mapped by _RtlReAllocateHeap.
    block = pcrt80d_realloc(mem, size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d_scalar_new - Calls to the CRT's scalar new operator from msvcr80d.dll
//   are patched through to this function. This function is just a wrapper
//   around the real CRT scalar new operator that sets appropriate flags to be
//   consulted when the memory is actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT scalar new operator.
//
void* VisualLeakDetector::_crt80d_scalar_new (unsigned int size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // The new operator is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d_scalar_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_scalar_new = (new_t)GetProcAddress(msvcr80d, "??2@YAPAXI@Z");
    }

    // Do tha allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrt80d_scalar_new(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crt80d_vector_new - Calls to the CRT's vector new operator from msvcr80d.dll
//   are patched through to this function. This function is just a wrapper
//   around the real CRT vector new operator that sets appropriate flags to be
//   consulted when the memory is actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT vector new operator.
//
void* VisualLeakDetector::_crt80d_vector_new (unsigned int size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcr80d;
    tls_t   *tls = vld.gettls();

    // The new operator is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrt80d_vector_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        msvcr80d = GetModuleHandle(L"msvcr80d.dll");
        pcrt80d_vector_new = (new_t)GetProcAddress(msvcr80d, "??_U@YAPAXI@Z");
    }

    // Do tha allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrt80d_vector_new(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crtd__calloc_dbg - Calls to _calloc_dbg from msvcrtd.dll are patched through
//   to this function. This function is just a wrapper around the real
//   _calloc_dbg that sets appropriate flags to be consulted when the memory is
//   actually allocated by RtlAllocateHeap.
//
//  - num (IN): The number of blocks, of size 'size', to be allocated.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _calloc_dbg.
//
void* VisualLeakDetector::_crtd__calloc_dbg (size_t num, size_t size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcrtd;
    tls_t   *tls = vld.gettls();

    // _malloc_dbg is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrtd__calloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__calloc_dbg = (_calloc_dbg_t)GetProcAddress(msvcrtd, "_calloc_dbg");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrtd__calloc_dbg(num, size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crtd__malloc_dbg - Calls to _malloc_dbg from msvcrtd.dll are patched through
//   to this function. This function is just a wrapper around the real
//   _malloc_dbg that sets appropriate flags to be consulted when the memory is
//   actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _malloc_dbg.
//
void* VisualLeakDetector::_crtd__malloc_dbg (size_t size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcrtd;
    tls_t   *tls = vld.gettls();

    // _malloc_dbg is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrtd__malloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _malloc_dbg.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__malloc_dbg = (_malloc_dbg_t)GetProcAddress(msvcrtd, "_malloc_dbg");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrtd__malloc_dbg(size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crtd__realloc_dbg - Calls to _realloc_dbg from msvcrtd.dll are patched
//   through to this function. This function is just a wrapper around the real
//   _realloc_dbg that sets appropriate flags to be consulted when the memory is
//   actually allocated by RtlAllocateHeap.
//
//  - mem (IN): Pointer to the memory block to be reallocated.
//
//  - size (IN): The size of the memory block to reallocate.
//
//  - type (IN): The CRT "use type" of the block to be reallocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above filel, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by _realloc_dbg.
//
void* VisualLeakDetector::_crtd__realloc_dbg (void *mem, size_t size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcrtd;
    tls_t   *tls = vld.gettls();

    // _realloc_dbg is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrtd__realloc_dbg == NULL) {
        // This is the first call to this function. Link to the real
        // _realloc_dbg.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__realloc_dbg = (_realloc_dbg_t)GetProcAddress(msvcrtd, "_realloc_dbg");
    }

    // Do the allocation. The block will be mapped by _RtlReAllocateHeap.
    block = pcrtd__realloc_dbg(mem, size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crtd__scalar_new_dbg - Calls to the CRT's debug scalar new operator from
//   msvcrtd.dll are patched through to this function. This function is just a
//   wrapper around the real CRT debug scalar new operator that sets appropriate
//   flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - type (IN): The CRT "use type" of the block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the CRT debug scalar new operator.
//
void* VisualLeakDetector::_crtd__scalar_new_dbg (unsigned int size, int type, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcrtd;
    tls_t   *tls = vld.gettls();

    // The debug new operator is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrtd__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real CRT debug
        // new operator.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd__scalar_new_dbg = (crt_new_dbg_t)GetProcAddress(msvcrtd, "??2@YAPAXIHPBDH@Z");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrtd__scalar_new_dbg(size, type, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crtd_calloc - Calls to calloc from msvcrtd.dll are patched through to this
//   function. This function is just a wrapper around the real calloc that sets
//   appropriate flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - num (IN): The number of blocks, of size 'size', to be allocated.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from calloc.
//
void* VisualLeakDetector::_crtd_calloc (size_t num, size_t size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcrtd;
    tls_t   *tls = vld.gettls();

    // malloc is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrtd_calloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_calloc = (calloc_t)GetProcAddress(msvcrtd, "calloc");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrtd_calloc(num, size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crtd_malloc - Calls to malloc from msvcrtd.dll are patched through to this
//   function. This function is just a wrapper around the real malloc that sets
//   appropriate flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the valued returned from malloc.
//
void* VisualLeakDetector::_crtd_malloc (size_t size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcrtd;
    tls_t   *tls = vld.gettls();

    // malloc is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrtd_malloc == NULL) {
        // This is the first call to this function. Link to the real malloc.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_malloc = (malloc_t)GetProcAddress(msvcrtd, "malloc");
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrtd_malloc(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crtd_realloc - Calls to realloc from msvcrtd.dll are patched through to this
//   function. This function is just a wrapper around the real realloc that sets
//   appropriate flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned from realloc.
//
void* VisualLeakDetector::_crtd_realloc (void *mem, size_t size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcrtd;
    tls_t   *tls = vld.gettls();

    // realloc is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrtd_realloc == NULL) {
        // This is the first call to this function. Link to the real realloc.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_realloc = (realloc_t)GetProcAddress(msvcrtd, "realloc");
    }

    // Do the allocation. The block will be mapped by _RtlReAllocateHeap.
    block = pcrtd_realloc(mem, size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _crtd_scalar_new - Calls to the CRT's scalar new operator from msvcrtd.dll
//   are patched through to this function. This function is just a wrapper
//   around the real CRT scalar new operator that sets appropriate flags to be
//   consulted when the memory is actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the CRT scalar new operator.
//
void* VisualLeakDetector::_crtd_scalar_new (unsigned int size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  msvcrtd;
    tls_t   *tls = vld.gettls();

    // The new operator is a CRT function and allocates from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pcrtd_scalar_new == NULL) {
        // This is the first call to this function. Link to the real CRT new
        // operator.
        msvcrtd = GetModuleHandle(L"msvcrtd.dll");
        pcrtd_scalar_new = (new_t)GetProcAddress(msvcrtd, "??2@YAPAXI@Z");
    }

    // Do tha allocation. The block will be mapped by _RtlAllocateHeap.
    block = pcrtd_scalar_new(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _GetProcAddress - Calls to GetProcAddress are patched through to this
//   function. If the requested function is a function that has been patched
//   through to one of VLD's handlers, then the address of VLD's handler
//   function is returned instead of the real address. Otherwise, this 
//   function is just a wrapper around the real GetProcAddress.
//
//  - module (IN): Handle (base address) of the module from which to retrieve
//      the address of an exported function.
//
//  - procname (IN): ANSI string containing the name of the exported function
//      whose address is to be retrieved.
//
//  Return Value:
//
//    Returns a pointer to the requested function, or VLD's replacement for
//    the function, if there is a replacement function.
//
FARPROC VisualLeakDetector::_GetProcAddress (HMODULE module, LPCSTR procname)
{
    patchentry_t *entry;
    UINT          index;
    UINT          tablesize = sizeof(vld.m_patchtable) / sizeof(patchentry_t);

    // See if there is an entry in the patch table that matches the requested
    // function.
    for (index = 0; index < tablesize; index++) {
        entry = &vld.m_patchtable[index];
        if ((entry->modulebase == 0x0) || ((HMODULE)entry->modulebase != module)) {
            // This patch table entry is for a different module.
            continue;
        }

        // This patch table entry is for the specified module. If the requested
        // import's name matches the entry's import name (or ordinal), then
        // return the address of the replacement instead of the address of the
        // actual import.
        if ((SIZE_T)entry->importname < (SIZE_T)vld.m_vldbase) {
            // This entry's import name is not a valid pointer to data in
            // vld.dll. It must be an ordinal value.
            if ((UINT)entry->importname == (UINT)procname) {
                return (FARPROC)entry->replacement;
            }
        }
        else {
            if (strcmp(entry->importname, procname) == 0) {
                return (FARPROC)entry->replacement;
            }
        }
    }

    // The requested function is not a patched function. Just return the real
    // address of the requested function.
    return GetProcAddress(module, procname);
}

// _HeapCreate - Calls to HeapCreate are patched through to this function. This
//   function is just a wrapper around the real HeapCreate that calls VLD's heap
//   creation tracking function after the heap has been created.
//
//  - options (IN): Heap options.
//
//  - initsize (IN): Initial size of the heap.
//
//  - maxsize (IN): Maximum size of the heap.
//
//  Return Value:
//
//    Returns the value returned by HeapCreate.
//
HANDLE VisualLeakDetector::_HeapCreate (DWORD options, SIZE_T initsize, SIZE_T maxsize)
{
    DWORD64            displacement;
    SIZE_T             fp;
    SYMBOL_INFO       *functioninfo;
    HANDLE             heap;
    HeapMap::Iterator  heapit;
    SIZE_T             ra;
    BYTE               symbolbuffer [sizeof(SYMBOL_INFO) + (MAXSYMBOLNAMELENGTH * sizeof(WCHAR)) - 1] = { 0 };
    BOOL               symfound;

    // Get the return address within the calling function.
    FRAMEPOINTER(fp);
    ra = *((SIZE_T*)fp + 1);

    // Create the heap.
    heap = HeapCreate(options, initsize, maxsize);

    // Map the created heap handle to a new block map.
    vld.mapheap(heap);

    // Try to get the name of the function containing the return address.
    functioninfo = (SYMBOL_INFO*)&symbolbuffer;
    functioninfo->SizeOfStruct = sizeof(SYMBOL_INFO);
    functioninfo->MaxNameLen = MAXSYMBOLNAMELENGTH;
    EnterCriticalSection(&symbollock);
    symfound = SymFromAddrW(currentprocess, ra, &displacement, functioninfo);
    LeaveCriticalSection(&symbollock);
    if (symfound == TRUE) {
        if (wcscmp(L"_heap_init", functioninfo->Name) == 0) {
            // HeapCreate was called by _heap_init. This is a static CRT heap.
            EnterCriticalSection(&vld.m_maplock);
            heapit = vld.m_heapmap->find(heap);
            assert(heapit != vld.m_heapmap->end());
            (*heapit).second->flags |= VLD_HEAP_CRT;
            LeaveCriticalSection(&vld.m_maplock);
        }
    }

    return heap;
}

// _HeapDestroy - Calls to HeapDestroy are patched through to this function.
//   This function is just a wrapper around the real HeapDestroy that calls
//   VLD's heap destruction tracking function after the heap has been destroyed.
//
//  - heap (IN): Handle to the heap to be destroyed.
//
//  Return Value:
//
//    Returns the valued returned by HeapDestroy.
//
BOOL VisualLeakDetector::_HeapDestroy (HANDLE heap)
{
    // After this heap is destroyed, the heap's address space will be unmapped
    // from the process's address space. So, we'd better generate a leak report
    // for this heap now, while we can still read from the memory blocks
    // allocated to it.
    vld.reportleaks(heap);

    vld.unmapheap(heap);

    return HeapDestroy(heap);
}

// _LdrLoadDll - Calls to LdrLoadDll are patched through to this function. This
//   function invokes the real LdrLoadDll and then re-attaches VLD to all
//   modules loaded in the process after loading of the new DLL is complete.
//   All modules must be re-enumerated because the explicit load of the
//   specified module may result in the implicit load of one or more additional
//   modules which are dependencies of the specified module.
//
//  - searchpath (IN): The path to use for searching for the specified module to
//      be loaded.
//
//  - flags (IN): Pointer to action flags.
//
//  - modulename (IN): Pointer to a unicodestring_t structure specifying the
//      name of the module to be loaded.
//
//  - modulehandle (OUT): Address of a HANDLE to receive the newly loaded
//      module's handle.
//
//  Return Value:
//
//    Returns the value returned by LdrLoadDll.
//
NTSTATUS VisualLeakDetector::_LdrLoadDll (LPWSTR searchpath, PDWORD flags, unicodestring_t *modulename,
                                          PHANDLE modulehandle)
{
    ModuleSet::Iterator  moduleit;
    ModuleSet           *newmodules;
    ModuleSet           *oldmodules;
    NTSTATUS             status;

    EnterCriticalSection(&vld.m_loaderlock);

    // Load the DLL.
    status = LdrLoadDll(searchpath, flags, modulename, modulehandle);
    
    if (STATUS_SUCCESS == status) {
        // Create a new set of all loaded modules, including any newly loaded
        // modules.
        newmodules = new ModuleSet;
        newmodules->reserve(MODULESETRESERVE);
        EnumerateLoadedModulesW64(currentprocess, addloadedmodule, newmodules);

        // Attach to all modules included in the set.
        vld.attachtoloadedmodules(newmodules);

        // Start using the new set of loaded modules.
        EnterCriticalSection(&vld.m_moduleslock);
        oldmodules = vld.m_loadedmodules;
        vld.m_loadedmodules = newmodules;
        LeaveCriticalSection(&vld.m_moduleslock);

        // Free resources used by the old module list.
        for (moduleit = oldmodules->begin(); moduleit != oldmodules->end(); ++moduleit) {
            delete (*moduleit).name;
            delete (*moduleit).path;
        }
        delete oldmodules;
    }

    LeaveCriticalSection(&vld.m_loaderlock);

    return status;
}

// _mfc42d__scalar_new_dbg - Calls to the MFC debug scalar new operator from
//   mfc42d.dll are patched through to this function. This function is just a
//   wrapper around the real MFC debug scalar new operator that sets appropriate
//   flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC debug scalar new operator.
//
void* VisualLeakDetector::_mfc42d__scalar_new_dbg (unsigned int size, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  mfc42d;
    tls_t   *tls = vld.gettls();

    // The MFC new operators are CRT-based and allocate from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pmfc42d__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfc42d = GetModuleHandle(L"mfc42d.dll");
        pmfc42d__scalar_new_dbg = (mfc_new_dbg_t)GetProcAddress(mfc42d, (LPCSTR)714);
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pmfc42d__scalar_new_dbg(size, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _mfc42d_scalar_new - Calls to the MFC scalar new operator from mfc42d.dll are
//   patched through to this function. This function is just a wrapper around
//   the real MFC scalar new operator that sets appropriate flags to be
//   consulted when the memory is actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC scalar new operator.
//
void* VisualLeakDetector::_mfc42d_scalar_new (unsigned int size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  mfc42d;
    tls_t   *tls = vld.gettls();

    // The MFC new operators are CRT-based and allocate from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pmfc42d_scalar_new == NULL) {
        // This is the first call to this function. Link to the real MFC new
        // operator.
        mfc42d = GetModuleHandle(L"mfc42d.dll");
        pmfc42d_scalar_new = (new_t)GetProcAddress(mfc42d, (LPCSTR)711);
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pmfc42d_scalar_new(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _mfc80d__scalar_new_dbg - Calls to the MFC debug scalar new operator from
//   mfc80d.dll are patched through to this function. This function is just a
//   wrapper around the real MFC debug scalar new operator that sets appropriate
//   flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC debug scalar new operator.
//
void* VisualLeakDetector::_mfc80d__scalar_new_dbg (unsigned int size, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  mfc80d;
    tls_t   *tls = vld.gettls();

    // The MFC new operators are CRT-based and allocate from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pmfc80d__scalar_new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfc80d = GetModuleHandle(L"mfc80d.dll");
        pmfc80d__scalar_new_dbg = (mfc_new_dbg_t)GetProcAddress(mfc80d, (LPCSTR)895);
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pmfc80d__scalar_new_dbg(size, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _mfc80d__vector_new_dbg - Calls to the MFC debug vector new operator from
//   mfc80d.dll are patched through to this function. This function is just a
//   wrapper around the real MFC debug vector new operator that sets appropriate
//   flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  - file (IN): The name of the file from which this function is being called.
//
//  - line (IN): The line number, in the above file, at which this function is
//      being called.
//
//  Return Value:
//
//    Returns the value returned by the MFC debug vector new operator.
//
void* VisualLeakDetector::_mfc80d__vector_new_dbg (unsigned int size, const char *file, int line)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  mfc80d;
    tls_t   *tls = vld.gettls();

    // The MFC new operators are CRT-based and allocate from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pmfc80d__vector_new_dbg == NULL) {
        // This is the first call to this function. Link to the real MFC debug
        // new operator.
        mfc80d = GetModuleHandle(L"mfc80d.dll");
        pmfc80d__vector_new_dbg = (mfc_new_dbg_t)GetProcAddress(mfc80d, (LPCSTR)269);
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pmfc80d__vector_new_dbg(size, file, line);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _mfc80d_scalar_new - Calls to the MFC scalar new operator from mfc80d.dll are
//   patched through to this function. This function is just a wrapper around
//   the real MFC scalar new operator that sets appropriate flags to be
//   consulted when the memory is actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC scalar new operator.
//
void* VisualLeakDetector::_mfc80d_scalar_new (unsigned int size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  mfc80d;
    tls_t   *tls = vld.gettls();

    // The MFC new operators are CRT-based and allocate from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pmfc80d_scalar_new == NULL) {
        // This is the first call to this function. Link to the real MFC 8.0 new
        // operator.
        mfc80d = GetModuleHandle(L"mfc80d.dll");
        pmfc80d_scalar_new = (new_t)GetProcAddress(mfc80d, (LPCSTR)893);
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pmfc80d_scalar_new(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _mfc80d_vector_new - Calls to the MFC vector new operator from mfc80d.dll are
//   patched through to this function. This function is just a wrapper around
//   the real MFC vector new operator that sets appropriate flags to be
//   consulted when the memory is actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size, in bytes, of the memory block to be allocated.
//
//  Return Value:
//
//    Returns the value returned by the MFC vector new operator.
//
void* VisualLeakDetector::_mfc80d_vector_new (unsigned int size)
{
    void    *block;
    SIZE_T   fp;
    HMODULE  mfc80d;
    tls_t   *tls = vld.gettls();

    // The MFC new operators are CRT-based and allocate from the CRT heap.
    tls->flags |= VLD_TLS_CRTALLOC;

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    if (pmfc80d_vector_new == NULL) {
        // This is the first call to this function. Link to the real MFC 8.0 new
        // operator.
        mfc80d = GetModuleHandle(L"mfc80d.dll");
        pmfc80d_vector_new = (new_t)GetProcAddress(mfc80d, (LPCSTR)267);
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    block = pmfc80d_vector_new(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _RtlAllocateHeap - Calls to RtlAllocateHeap are patched through to this
//   function. This function invokes the real RtlAllocateHeap and then calls
//   VLD's allocation tracking function. Pretty much all memory allocations
//   will eventually result in a call to RtlAllocateHeap, so this is where we
//   finally map the allocated block.
//
//  - heap (IN): Handle to the heap from which to allocate memory.
//
//  - flags (IN): Heap allocation control flags.
//
//  - size (IN): Size, in bytes, of the block to allocate.
//
//  Return Value:
//
//    Returns the return value from RtlAllocateHeap.
//
LPVOID VisualLeakDetector::_RtlAllocateHeap (HANDLE heap, DWORD flags, SIZE_T size)
{
    BOOL                 crtalloc;
    BOOL                 excluded = FALSE;
    SIZE_T               fp;
    LPVOID               block;
    moduleinfo_t         moduleinfo;
    ModuleSet::Iterator  moduleit;
    SIZE_T               returnaddress;
    tls_t               *tls = vld.gettls();

    // Allocate the block.
    block = RtlAllocateHeap(heap, flags, size);
    if ((block != NULL) && vld.enabled()) {
        if (tls->addrfp == 0x0) {
            // This is the first call to enter VLD for the current allocation.
            // Record the current frame pointer.
            FRAMEPOINTER(fp);
        }
        else {
            fp = tls->addrfp;
        }
        crtalloc = (tls->flags & VLD_TLS_CRTALLOC) ? TRUE : FALSE;

        // Reset thread local flags and variables, in case any libraries called
        // into while mapping the block allocate some memory.
        tls->addrfp = 0x0;
        tls->flags &=~VLD_TLS_CRTALLOC;

        // Find the information for the module that initiated this allocation.
        returnaddress = *((SIZE_T*)fp + 1);
        moduleinfo.addrhigh = returnaddress;
        moduleinfo.addrlow  = returnaddress;
        EnterCriticalSection(&vld.m_moduleslock);
        moduleit = vld.m_loadedmodules->find(moduleinfo);
        if (moduleit != vld.m_loadedmodules->end()) {
            excluded = (*moduleit).flags & VLD_MODULE_EXCLUDED ? TRUE : FALSE;
        }
        LeaveCriticalSection(&vld.m_moduleslock);
        if (!excluded) {
            // The module that initiated this allocation is included in leak
            // detection. Map this block to the specified heap.
            vld.mapblock(heap, block, size, fp, crtalloc);
        }
    }

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// _RtlFreeHeap - Calls to RtlFreeHeap are patched through to this function.
//   This function calls VLD's free tracking function and then invokes the real
//   RtlFreeHeap. Pretty much all memory frees will eventually result in a call
//   to RtlFreeHeap, so this is where we finally unmap the freed block.
//
//  - heap (IN): Handle to the heap to which the block being freed belongs.
//
//  - flags (IN): Heap control flags.
//
//  - mem (IN): Pointer to the memory block being freed.
//
//  Return Value:
//
//    Returns the value returned by RtlFreeHeap.
//
BOOL VisualLeakDetector::_RtlFreeHeap (HANDLE heap, DWORD flags, LPVOID mem)
{
    BOOL status;

    // Unmap the block from the specified heap.
    vld.unmapblock(heap, mem);

    status = RtlFreeHeap(heap, flags, mem);

    return status;
}

// _RtlReAllocateHeap - Calls to RtlReAllocateHeap are patched through to this
//   function. This function invokes the real RtlReAllocateHeap and then calls
//   VLD's reallocation tracking function. All arguments passed to this function
//   are passed on to the real RtlReAllocateHeap without modification. Pretty
//   much all memory re-allocations will eventually result in a call to
//   RtlReAllocateHeap, so this is where we finally remap the reallocated block.
//
//  - heap (IN): Handle to the heap to reallocate memory from.
//
//  - flags (IN): Heap control flags.
//
//  - mem (IN): Pointer to the currently allocated block which is to be
//      reallocated.
//
//  - size (IN): Size, in bytes, of the block to reallocate.
//
//  Return Value:
//
//    Returns the value returned by RtlReAllocateHeap.
//
LPVOID VisualLeakDetector::_RtlReAllocateHeap (HANDLE heap, DWORD flags, LPVOID mem, SIZE_T size)
{
    BOOL                 crtalloc;
    BOOL                 excluded = FALSE;
    SIZE_T               fp;
    moduleinfo_t         moduleinfo;
    ModuleSet::Iterator  moduleit;
    LPVOID               newmem;
    SIZE_T               returnaddress;
    tls_t               *tls = vld.gettls();

    // Reallocate the block.
    newmem = RtlReAllocateHeap(heap, flags, mem, size);

    if (newmem != NULL) {
        if (tls->addrfp == 0x0) {
            // This is the first call to enter VLD for the current allocation.
            // Record the current frame pointer.
            FRAMEPOINTER(fp);
        }
        else {
            fp = tls->addrfp;
        }
        crtalloc = (tls->flags & VLD_TLS_CRTALLOC) ? TRUE : FALSE;

        // Reset thread local flags and variables, in case any libraries called
        // into while remapping the block allocate some memory.
        tls->addrfp = 0x0;
        tls->flags &= ~VLD_TLS_CRTALLOC;

        // Find the information for the module that initiated this reallocation.
        returnaddress = *((SIZE_T*)fp + 1);
        moduleinfo.addrhigh = returnaddress;
        moduleinfo.addrlow  = returnaddress;
        EnterCriticalSection(&vld.m_moduleslock);
        moduleit = vld.m_loadedmodules->find(moduleinfo);
        if (moduleit != vld.m_loadedmodules->end()) {
            excluded = (*moduleit).flags & VLD_MODULE_EXCLUDED ? TRUE : FALSE;
        }
        LeaveCriticalSection(&vld.m_moduleslock);
        if (!excluded) {
            // The module that initiated this allocation is included in leak
            // detection. Remap the block.
            vld.remapblock(heap, mem, newmem, size, fp, crtalloc);
        }
    }

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return newmem;
}

// AddRef - Calls to IMalloc::AddRef end up here. This function is just a
//   wrapper around the real IMalloc::AddRef implementation.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::AddRef.
//
ULONG VisualLeakDetector::AddRef ()
{
    assert(m_imalloc != NULL);
    return m_imalloc->AddRef();
}

// Alloc - Calls to IMalloc::Alloc end up here. This function is just a wrapper
//   around the real IMalloc::Alloc implementation that sets appropriate flags
//   to be consulted when the memory is actually allocated by RtlAllocateHeap.
//
//  - size (IN): The size of the memory block to allocate.
//
//  Return Value:
//
//    Returns the value returned by the system's IMalloc::Alloc implementation.
//
LPVOID VisualLeakDetector::Alloc (ULONG size)
{
    LPVOID  block;
    SIZE_T  fp;
    tls_t  *tls = vld.gettls();

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    // Do the allocation. The block will be mapped by _RtlAllocateHeap.
    assert(m_imalloc != NULL);
    block = m_imalloc->Alloc(size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;

    return block;
}

// addloadedmodule - Callback function for EnumerateLoadedModules64. This
//   function records information about every module loaded in the process,
//   each time adding the module's information to the provided ModuleSet (the
//   "context" parameter).
//
//   When EnumerateLoadedModules64 has finished calling this function for each
//   loaded module, then the resulting ModuleSet can be used at any time to get
//   information about any modules loaded into the process.
//
//   - modulepath (IN): The fully qualified path from where the module was
//       loaded.
//
//   - modulebase (IN): The base address at which the module has been loaded.
//
//   - modulesize (IN): The size, in bytes, of the loaded module.
//
//   - context (IN): Pointer to the ModuleSet to which information about each
//       module is to be added.
//
//  Return Value:
//
//    Always returns TRUE, which tells EnumerateLoadedModules64 to continue
//    enumerating.
//
BOOL VisualLeakDetector::addloadedmodule (PCWSTR modulepath, DWORD64 modulebase, ULONG modulesize, PVOID context)
{
    size_t        count;
    patchentry_t *entry;
    CHAR          extension [_MAX_EXT];
    CHAR          filename [_MAX_FNAME];
    UINT          index;
    moduleinfo_t  moduleinfo;
    LPSTR         modulenamea;
    LPSTR         modulepatha;
    ModuleSet*    newmodules = (ModuleSet*)context;
    SIZE_T        size;
    UINT          tablesize = sizeof(m_patchtable) / sizeof(patchentry_t);

    // Convert the module path to ASCII.
    size = wcslen(modulepath) + 1;
    modulepatha = new CHAR [size];
    wcstombs_s(&count, modulepatha, size, modulepath, _TRUNCATE);

    // Extract just the filename and extension from the module path.
    _splitpath_s(modulepatha, NULL, 0, NULL, 0, filename, _MAX_FNAME, extension, _MAX_EXT);
    size = strlen(filename) + strlen(extension) + 1;
    modulenamea = new CHAR [size];
    strncpy_s(modulenamea, size, filename, _TRUNCATE);
    strncat_s(modulenamea, size, extension, _TRUNCATE);
    _strlwr_s(modulenamea, size);

    if (_stricmp(modulenamea, "vld.dll") == 0) {
        // Record Visual Leak Detector's own base address.
        vld.m_vldbase = (HMODULE)modulebase;
    }
    else {
        // See if this is a module listed in the patch table. If it is, update
        // the corresponding patch table entries' module base address.
        for (index = 0; index < tablesize; index++) {
            entry = &m_patchtable[index];
            if (_stricmp(entry->exportmodulename, modulenamea) == 0) {
                entry->modulebase = (SIZE_T)modulebase;
            }
        }
    }

    // Record the module's information and store it in the set.
    moduleinfo.addrlow  = (SIZE_T)modulebase;
    moduleinfo.addrhigh = (SIZE_T)(modulebase + modulesize) - 1;
    moduleinfo.flags    = 0x0;
    moduleinfo.name     = modulenamea;
    moduleinfo.path     = modulepatha;
    newmodules->insert(moduleinfo);

    return TRUE;
}

// attachtoloadedmodules - Attaches VLD to all modules contained in the provided
//   ModuleSet. Not all modules are in the ModuleSet will actually be included
//   in leak detection. Only modules that import the global VisualLeakDetector
//   class object, or those that are otherwise explicitly included in leak
//   detection, will be checked for memory leaks.
//
//   When VLD attaches to a module, it means that any of the imports listed in
//   the import patch table which are imported by the module, will be redirected
//   to VLD's designated replacements.
//
//  - newmodules (IN): Pointer to a ModuleSet containing information about any
//      loaded modules that need to be attached.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::attachtoloadedmodules (ModuleSet *newmodules)
{
    size_t                count;
    DWORD64               modulebase;
    UINT32                moduleflags;
    IMAGEHLP_MODULE64     moduleimageinfo;
    LPCSTR                modulename;
#define MAXMODULENAME (_MAX_FNAME + _MAX_EXT)
    WCHAR                 modulenamew [MAXMODULENAME];
    LPCSTR                modulepath;
    DWORD                 modulesize;
    ModuleSet::Iterator   newit;
    ModuleSet::Iterator   oldit;
    ModuleSet            *oldmodules;
    BOOL                  refresh;
    UINT                  tablesize = sizeof(m_patchtable) / sizeof(patchentry_t);
    ModuleSet::Muterator  updateit;

    // Iterate through the supplied set, until all modules have been attached.
    for (newit = newmodules->begin(); newit != newmodules->end(); ++newit) {
        modulebase  = (DWORD64)(*newit).addrlow;
        moduleflags = 0x0;
        modulename  = (*newit).name;
        modulepath  = (*newit).path;
        modulesize  = (DWORD)((*newit).addrhigh - (*newit).addrlow) + 1;

        refresh = FALSE;
        EnterCriticalSection(&m_moduleslock);
        oldmodules = m_loadedmodules;
        if (oldmodules != NULL) {
            // This is not the first time we have been called to attach to the
            // currently loaded modules.
            oldit = oldmodules->find(*newit);
            if (oldit != oldmodules->end()) {
                // We've seen this "new" module loaded in the process before.
                moduleflags = (*oldit).flags;
                LeaveCriticalSection(&m_moduleslock);
                if (moduleispatched((HMODULE)modulebase, m_patchtable, tablesize)) {
                    // This module is already attached. Just update the module's
                    // flags, nothing more.
                    updateit = newit;
                    (*updateit).flags = moduleflags;
                    continue;
                }
                else {
                    // This module may have been attached before and has been
                    // detached. We'll need to try reattaching to it in case it
                    // was unloaded and then subsequently reloaded.
                    refresh = TRUE;
                }
            }
            else {
                LeaveCriticalSection(&m_moduleslock);
            }
        }
        else {
            LeaveCriticalSection(&m_moduleslock);
        }

        EnterCriticalSection(&symbollock);
        if ((refresh == TRUE) && (moduleflags & VLD_MODULE_SYMBOLSLOADED)) {
            // Discard the previously loaded symbols, so we can refresh them.
            if (SymUnloadModule64(currentprocess, modulebase) == FALSE) {
                report(L"WARNING: Visual Leak Detector: Failed to unload the symbols for %s. Function names and line"
                       L" numbers shown in the memory leak report for %s may be inaccurate.", modulename, modulename);
            }
        }

        // Try to load the module's symbols. This ensures that we have loaded
        // the symbols for every module that has ever been loaded into the
        // process, guaranteeing the symbols' availability when generating the
        // leak report.
        moduleimageinfo.SizeOfStruct = sizeof(IMAGEHLP_MODULE64);
        if ((SymGetModuleInfoW64(currentprocess, (DWORD64)modulebase, &moduleimageinfo) == TRUE) ||
            ((SymLoadModule64(currentprocess, NULL, modulepath, NULL, modulebase, modulesize) == modulebase) &&
            (SymGetModuleInfoW64(currentprocess, modulebase, &moduleimageinfo) == TRUE))) {
            moduleflags |= VLD_MODULE_SYMBOLSLOADED;
        }
        LeaveCriticalSection(&symbollock);

        if (_stricmp("vld.dll", modulename) == 0) {
            // What happens when a module goes through it's own portal? Bad things.
            // Like infinite recursion. And ugly bald men wearing dresses. VLD
            // should not, therefore, attach to itself.
            continue;
        }

        mbstowcs_s(&count, modulenamew, MAXMODULENAME, modulename, _TRUNCATE);
        if ((findimport((HMODULE)modulebase, m_vldbase, "vld.dll", "?vld@@3VVisualLeakDetector@@A") == FALSE) &&
            (wcsstr(vld.m_forcedmodulelist, modulenamew) == NULL)) {
            // This module does not import VLD. This means that none of the module's
            // sources #included vld.h. Exclude this module from leak detection.
            moduleflags |= VLD_MODULE_EXCLUDED;
        }
        else if (!(moduleflags & VLD_MODULE_SYMBOLSLOADED) || (moduleimageinfo.SymType == SymExport)) {
            // This module is going to be included in leak detection, but complete
            // symbols for this module couldn't be loaded. This means that any stack
            // traces through this module may lack information, like line numbers
            // and function names.
            report(L"WARNING: Visual Leak Detector: A module, %s, included in memory leak detection\n"
                   L"  does not have any debugging symbols available, or they could not be located.\n"
                   L"  Function names and/or line numbers for this module may not be available.\n", modulename);
        }

        // Update the module's flags in the "new modules" set.
        updateit = newit;
        (*updateit).flags = moduleflags;

        // Attach to the module.
        patchmodule((HMODULE)modulebase, m_patchtable, tablesize);
    }
}

// buildsymbolsearchpath - Builds the symbol search path for the symbol handler.
//   This helps the symbol handler find the symbols for the application being
//   debugged.
//
//  Return Value:
//
//    Returns a string containing the search path. The caller is responsible for
//    freeing the string.
//
LPWSTR VisualLeakDetector::buildsymbolsearchpath ()
{
    WCHAR   directory [_MAX_DIR];
    WCHAR   drive [_MAX_DRIVE];
    LPWSTR  env;
    DWORD   envlen;
    SIZE_T  index;
    SIZE_T  length;
    HMODULE module;
    LPWSTR  path = new WCHAR [MAX_PATH];
    SIZE_T  pos = 0;
    WCHAR   system [MAX_PATH];
    WCHAR   windows [MAX_PATH];

    // Oddly, the symbol handler ignores the link to the PDB embedded in the
    // executable image. So, we'll manually add the location of the executable
    // to the search path since that is often where the PDB will be located.
    path[0] = L'\0';
    module = GetModuleHandle(NULL);
    GetModuleFileName(module, path, MAX_PATH);
    _wsplitpath_s(path, drive, _MAX_DRIVE, directory, _MAX_DIR, NULL, 0, NULL, 0);
    wcsncpy_s(path, MAX_PATH, drive, _TRUNCATE);
    strapp(&path, directory);

    // When the symbol handler is given a custom symbol search path, it will no
    // longer search the default directories (working directory, system root,
    // etc). But we'd like it to still search those directories, so we'll add
    // them to our custom search path.
    //
    // Append the working directory.
    strapp(&path, L";.\\");

    // Append the Windows directory.
    if (GetWindowsDirectory(windows, MAX_PATH) != 0) {
        strapp(&path, L";");
        strapp(&path, windows);
    }

    // Append the system directory.
    if (GetSystemDirectory(system, MAX_PATH) != 0) {
        strapp(&path, L";");
        strapp(&path, system);
    }

    // Append %_NT_SYMBOL_PATH%.
    envlen = GetEnvironmentVariable(L"_NT_SYMBOL_PATH", NULL, 0);
    if (envlen != 0) {
        env = new WCHAR [envlen];
        if (GetEnvironmentVariable(L"_NT_SYMBOL_PATH", env, envlen) != 0) {
            strapp(&path, L";");
            strapp(&path, env);
        }
        delete [] env;
    }

    //  Append %_NT_ALT_SYMBOL_PATH%.
    envlen = GetEnvironmentVariable(L"_NT_ALT_SYMBOL_PATH", NULL, 0);
    if (envlen != 0) {
        env = new WCHAR [envlen];
        if (GetEnvironmentVariable(L"_NT_ALT_SYMBOL_PATH", env, envlen) != 0) {
            strapp(&path, L";");
            strapp(&path, env);
        }
        delete [] env;
    }

    // Remove any quotes from the path. The symbol handler doesn't like them.
    pos = 0;
    length = wcslen(path);
    while (pos < length) {
        if (path[pos] == L'\"') {
            for (index = pos; index < length; index++) {
                path[index] = path[index + 1];
            }
        }
        pos++;
    }

    return path;
}

// configure - Configures VLD using values read from the vld.ini file.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::configure ()
{
#define BSIZE 64
    WCHAR        buffer [BSIZE];
    WCHAR        filename [MAX_PATH];
    WCHAR        inipath [MAX_PATH];
    BOOL         keyopen = FALSE;
    DWORD        length;
    HKEY         productkey;
    LONG         regstatus;
    struct _stat s;
    DWORD        valuetype;

    if (_wstat(L".\\vld.ini", &s) == 0) {
        // Found a copy of vld.ini in the working directory. Use it.
        wcsncpy_s(inipath, MAX_PATH, L".\\vld.ini", _TRUNCATE);
    }
    else {
        // Get the location of the vld.ini file from the registry.
        regstatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, VLDREGKEYPRODUCT, 0, KEY_QUERY_VALUE, &productkey);
        if (regstatus == ERROR_SUCCESS) {
            keyopen = TRUE;
            regstatus = RegQueryValueEx(productkey, L"IniFile", NULL, &valuetype, (LPBYTE)&inipath, &length);
        }
        if (keyopen) {
            RegCloseKey(productkey);
        }
        if ((regstatus != ERROR_SUCCESS) || (_wstat(inipath, &s) != 0)) {
            // The location of vld.ini could not be read from the registry. As a
            // last resort, look in the Windows directory.
            wcsncpy_s(inipath, MAX_PATH, L"vld.ini", _TRUNCATE);
        }
    }

    // Read the boolean options.
    GetPrivateProfileString(L"Options", L"VLD", L"on", buffer, BSIZE, inipath);
    if (strtobool(buffer) == FALSE) {
        m_options |= VLD_OPT_VLDOFF;
        return;
    }

    GetPrivateProfileString(L"Options", L"AggregateDuplicates", L"", buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_options |= VLD_OPT_AGGREGATE_DUPLICATES;
    }

    GetPrivateProfileString(L"Options", L"SelfTest", L"", buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_options |= VLD_OPT_SELF_TEST;
    }

    GetPrivateProfileString(L"Options", L"SlowDebuggerDump", L"", buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_options |= VLD_OPT_SLOW_DEBUGGER_DUMP;
    }

    GetPrivateProfileString(L"Options", L"StartDisabled", L"", buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_options |= VLD_OPT_START_DISABLED;
    }

    GetPrivateProfileString(L"Options", L"TraceInternalFrames", L"", buffer, BSIZE, inipath);
    if (strtobool(buffer) == TRUE) {
        m_options |= VLD_OPT_TRACE_INTERNAL_FRAMES;
    }

    // Read the integer configuration options.
    m_maxdatadump = GetPrivateProfileInt(L"Options", L"MaxDataDump", VLD_DEFAULT_MAX_DATA_DUMP, inipath);
    m_maxtraceframes = GetPrivateProfileInt(L"Options", L"MaxTraceFrames", VLD_DEFAULT_MAX_TRACE_FRAMES, inipath);
    if (m_maxtraceframes < 1) {
        m_maxtraceframes = VLD_DEFAULT_MAX_TRACE_FRAMES;
    }

    // Read the force-include module list.
    GetPrivateProfileString(L"Options", L"ForceIncludeModules", L"", m_forcedmodulelist, MAXMODULELISTLENGTH, inipath);
    _wcslwr_s(m_forcedmodulelist, MAXMODULELISTLENGTH);

    // Read the report destination (debugger, file, or both).
    GetPrivateProfileString(L"Options", L"ReportFile", L"", filename, MAX_PATH, inipath);
    if (wcslen(filename) == 0) {
        wcsncpy_s(filename, MAX_PATH, VLD_DEFAULT_REPORT_FILE_NAME, _TRUNCATE);
    }
    _wfullpath(m_reportfilepath, filename, MAX_PATH);
    GetPrivateProfileString(L"Options", L"ReportTo", L"", buffer, BSIZE, inipath);
    if (_wcsicmp(buffer, L"both") == 0) {
        m_options |= (VLD_OPT_REPORT_TO_DEBUGGER | VLD_OPT_REPORT_TO_FILE);
    }
    else if (_wcsicmp(buffer, L"file") == 0) {
        m_options |= VLD_OPT_REPORT_TO_FILE;
    }
    else {
        m_options |= VLD_OPT_REPORT_TO_DEBUGGER;
    }

    // Read the report file encoding (ascii or unicode).
    GetPrivateProfileString(L"Options", L"ReportEncoding", L"", buffer, BSIZE, inipath);
    if (_wcsicmp(buffer, L"unicode") == 0) {
        m_options |= VLD_OPT_UNICODE_REPORT;
    }
    if ((m_options & VLD_OPT_UNICODE_REPORT) && !(m_options & VLD_OPT_REPORT_TO_FILE)) {
        // If Unicode report encoding is enabled, then the report needs to be
        // sent to a file because the debugger will not display Unicode
        // characters, it will display question marks in their place instead.
        m_options |= VLD_OPT_REPORT_TO_FILE;
        m_status |= VLD_STATUS_FORCE_REPORT_TO_FILE;
    }

    // Read the stack walking method.
    GetPrivateProfileString(L"Options", L"StackWalkMethod", L"", buffer, BSIZE, inipath);
    if (_wcsicmp(buffer, L"safe") == 0) {
        m_options |= VLD_OPT_SAFE_STACK_WALK;
    }
}

// detachfrommodule - Callback function for EnumerateLoadedModules64 that
//   detaches Visual Leak Detector from the specified module. If the specified
//   module has not previously been attached to, then calling this function will
//   not actually result in any changes.
//
//  - modulepath (IN): String containing the name, which may inlcude a path, of
//      the module to detach from (ignored).
//
//  - modulebase (IN): Base address of the module.
//
//  - modulesize (IN): Total size of the module (ignored).
//
//  - context (IN): User-supplied context (ignored).
//
//  Return Value:
//
//    Always returns TRUE.
//
BOOL VisualLeakDetector::detachfrommodule (PCWSTR /*modulepath*/, DWORD64 modulebase, ULONG /*modulesize*/,
                                           PVOID /*context*/)
{
    UINT tablesize = sizeof(m_patchtable) / sizeof(patchentry_t);

    restoremodule((HMODULE)modulebase, m_patchtable, tablesize);

    return TRUE;
}

// DidAlloc - Calls to IMalloc::DidAlloc will end up here. This function is just
//   a wrapper around the system implementation of IMalloc::DidAlloc.
//
//  - mem (IN): Pointer to a memory block to inquire about.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::DidAlloc.
//
INT VisualLeakDetector::DidAlloc (LPVOID mem)
{
    assert(m_imalloc != NULL);
    return m_imalloc->DidAlloc(mem);
}

// enabled - Determines if memory leak detection is enabled for the current
//   thread.
//
//  Return Value:
//
//    Returns true if Visual Leak Detector is enabled for the current thread.
//    Otherwise, returns false.
//
BOOL VisualLeakDetector::enabled ()
{
    tls_t *tls = vld.gettls();

    if (!(m_status & VLD_STATUS_INSTALLED)) {
        // Memory leak detection is not yet enabled because VLD is still
        // initializing.
        return FALSE;
    }

    if (!(tls->flags & VLD_TLS_DISABLED) && !(tls->flags & VLD_TLS_ENABLED)) {
        // The enabled/disabled state for the current thread has not been 
        // initialized yet. Use the default state.
        if (m_options & VLD_OPT_START_DISABLED) {
            tls->flags |= VLD_TLS_DISABLED;
        }
        else {
            tls->flags |= VLD_TLS_ENABLED;
        }
    }

    return ((tls->flags & VLD_TLS_ENABLED) != 0);
}

// eraseduplicates - Erases, from the block maps, blocks that appear to be
//   duplicate leaks of an already identified leak.
//
//  - element (IN): BlockMap Iterator referencing the block of which to search
//      for duplicates.
//
//  Return Value:
//
//    Returns the number of duplicate blocks erased from the block map.
//
SIZE_T VisualLeakDetector::eraseduplicates (const BlockMap::Iterator &element)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    blockinfo_t        *elementinfo;
    SIZE_T              erased = 0;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;
    BlockMap::Iterator  previt;

    elementinfo = (*element).second;

    // Iteratate through all block maps, looking for blocks with the same size
    // and callstack as the specified element.
    for (heapit = m_heapmap->begin(); heapit != m_heapmap->end(); ++heapit) {
        blockmap = &(*heapit).second->blockmap;
        for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
            if (blockit == element) {
                // Don't delete the element of which we are searching for
                // duplicates.
                continue;
            }
            info = (*blockit).second;
            if ((info->size == elementinfo->size) && (*(info->callstack) == *(elementinfo->callstack))) {
                // Found a duplicate. Erase it.
                delete info->callstack;
                delete info;
                previt = blockit - 1;
                blockmap->erase(blockit);
                blockit = previt;
                erased++;
            }
        }
    }

    return erased;
}

// Free - Calls to IMalloc::Free will end up here. This function is just a
//   wrapper around the real IMalloc::Free implementation.
//
//  - mem (IN): Pointer to the memory block to be freed.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::Free (LPVOID mem)
{
    assert(m_imalloc != NULL);
    m_imalloc->Free(mem);
}

// GetSize - Calls to IMalloc::GetSize will end up here. This function is just a
//   wrapper around the real IMalloc::GetSize implementation.
//
//  - mem (IN): Pointer to the memory block to inquire about.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::GetSize.
//
ULONG VisualLeakDetector::GetSize (LPVOID mem)
{
    assert(m_imalloc != NULL);
    return m_imalloc->GetSize(mem);
}

// gettls - Obtains the thread local strorage structure for the calling thread.
//
//  Return Value:
//
//    Returns a pointer to the thread local storage structure. (This function
//    always succeeds).
//
tls_t* VisualLeakDetector::gettls ()
{
    tls_t *tls;
    
    // Get the pointer to this thread's thread local storage structure.
    tls = (tls_t*)TlsGetValue(m_tlsindex);
    assert(GetLastError() == ERROR_SUCCESS);

    if (tls == NULL) {
        // This thread's thread local storage structure has not been allocated.
        tls = new tls_t;
        TlsSetValue(m_tlsindex, tls);
        tls->addrfp = 0x0;
        tls->flags = 0x0;
        tls->threadid = GetCurrentThreadId();

        // Add this thread's TLS to the TlsSet.
        EnterCriticalSection(&m_tlslock);
        m_tlsset->insert(tls);
        LeaveCriticalSection(&m_tlslock);
    }

    return tls;
}

// HeapMinimize - Calls to IMalloc::HeapMinimize will end up here. This function
//   is just a wrapper around the real IMalloc::HeapMinimize implementation.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::HeapMinimize ()
{
    assert(m_imalloc != NULL);
    m_imalloc->HeapMinimize();
}

// mapblock - Tracks memory allocations. Information about allocated blocks is
//   collected and then the block is mapped to this information.
//
//  - heap (IN): Handle to the heap from which the block has been allocated.
//
//  - mem (IN): Pointer to the memory block being allocated.
//
//  - size (IN): Size, in bytes, of the memory block being allocated.
//
//  - framepointer (IN): Framepointer at the time this allocation first entered
//      VLD's code. This is used from determining the starting point for the
//      stack trace.
//
//  - crtalloc (IN): Should be set to TRUE if this allocation is a CRT memory
//      block. Otherwise should be FALSE.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::mapblock (HANDLE heap, LPCVOID mem, SIZE_T size, SIZE_T framepointer, BOOL crtalloc)
{
    blockinfo_t        *blockinfo;
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    HeapMap::Iterator   heapit;
    static SIZE_T       serialnumber = 0;

    // Record the block's information.
    blockinfo = new blockinfo_t;
    if (m_options & VLD_OPT_SAFE_STACK_WALK) {
        blockinfo->callstack = new SafeCallStack;
    }
    else {
        blockinfo->callstack = new FastCallStack;
    }
    if (m_options & VLD_OPT_TRACE_INTERNAL_FRAMES) {
        // Passing NULL for the frame pointer argument will force the stack
        // trace to begin at the current frame.
        blockinfo->callstack->getstacktrace(m_maxtraceframes, NULL);
    }
    else {
        // Start the stack trace at the call that first entered VLD's code.
        blockinfo->callstack->getstacktrace(m_maxtraceframes, (SIZE_T*)framepointer);
    }
    blockinfo->serialnumber = serialnumber++;
    blockinfo->size = size;

    // Insert the block's information into the block map.
    EnterCriticalSection(&m_maplock);
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // We haven't mapped this heap to a block map yet. Do it now.
        mapheap(heap);
        heapit = m_heapmap->find(heap);
        assert(heapit != m_heapmap->end());
    }
    if (crtalloc == TRUE) {
        // The heap that this block was allocated from is a CRT heap.
        (*heapit).second->flags |= VLD_HEAP_CRT;
    }
    blockmap = &(*heapit).second->blockmap;
    blockit = blockmap->insert(mem, blockinfo);
    if (blockit == blockmap->end()) {
        // A block with this address has already been allocated. The
        // previously allocated block must have been freed (probably by some
        // mechanism unknown to VLD), or the heap wouldn't have allocated it
        // again. Replace the previously allocated info with the new info.
        blockit = blockmap->find(mem);
        delete (*blockit).second->callstack;
        delete (*blockit).second;
        blockmap->erase(blockit);
        blockmap->insert(mem, blockinfo);
    }
    LeaveCriticalSection(&m_maplock);
}

// mapheap - Tracks heap creation. Creates a block map for tracking individual
//   allocations from the newly created heap and then maps the heap to this
//   block map.
//
//  - heap (IN): Handle to the newly created heap.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::mapheap (HANDLE heap)
{
    heapinfo_t        *heapinfo;
    HeapMap::Iterator  heapit;

    // Create a new block map for this heap and insert it into the heap map.
    heapinfo = new heapinfo_t;
    heapinfo->blockmap.reserve(BLOCKMAPRESERVE);
    heapinfo->flags = 0x0;
    EnterCriticalSection(&m_maplock);
    heapit = m_heapmap->insert(heap, heapinfo);
    if (heapit == m_heapmap->end()) {
        // Somehow this heap has been created twice without being destroyed,
        // or at least it was destroyed without VLD's knowledge. Unmap the heap
        // from the existing heapinfo, and remap it to the new one.
        report(L"WARNING: Visual Leak Detector detected a duplicate heap (" ADDRESSFORMAT L").\n", heap);
        heapit = m_heapmap->find(heap);
        unmapheap((*heapit).first);
        m_heapmap->insert(heap, heapinfo);
    }
    LeaveCriticalSection(&m_maplock);
}

// QueryInterface - Calls to IMalloc::QueryInterface will end up here. This
//   function is just a wrapper around the real IMalloc::QueryInterface
//   implementation.
//
//  - iid (IN): COM interface ID to query about.
//
//  - object (IN): Address of a pointer to receive the requested interface
//      pointer.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::QueryInterface.
//
HRESULT VisualLeakDetector::QueryInterface (REFIID iid, LPVOID *object)
{
    assert(m_imalloc != NULL);
    return m_imalloc->QueryInterface(iid, object);
}

// Realloc - Calls to IMalloc::Realloc will end up here. This function is just a
//   wrapper around the real IMalloc::Realloc implementation that sets
//   appropriate flags to be consulted when the memory is actually allocated by
//   RtlAllocateHeap.
//
//  - mem (IN): Pointer to the memory block to reallocate.
//
//  - size (IN): Size, in bytes, of the memory block to reallocate.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::Realloc.
//
LPVOID VisualLeakDetector::Realloc (LPVOID mem, ULONG size)
{
    LPVOID  block;
    SIZE_T  fp;
    tls_t  *tls = vld.gettls();

    if (tls->addrfp == 0x0) {
        // This is the first call to enter VLD for the current allocation.
        // Record the current frame pointer.
        FRAMEPOINTER(fp);
        tls->addrfp = fp;
    }

    // Do the allocation. The block will be mapped by _RtlReAllocateHeap.
    assert(m_imalloc != NULL);
    block = m_imalloc->Realloc(mem, size);

    // Reset thread local flags and variables for the next allocation.
    tls->addrfp = 0x0;
    tls->flags &= ~VLD_TLS_CRTALLOC;
    
    return block;
}

// Release - Calls to IMalloc::Release will end up here. This function is just
//   a wrapper around the real IMalloc::Release implementation.
//
//  Return Value:
//
//    Returns the value returned by the system implementation of
//    IMalloc::Release.
//
ULONG VisualLeakDetector::Release ()
{
    assert(m_imalloc != NULL);
    return m_imalloc->Release();
}

// remapblock - Tracks reallocations. Unmaps a block from its previously
//   collected information and remaps it to updated information.
//
//  Note: If the block itself remains at the same address, then the block's
//   information can simply be updated rather than having to actually erase and
//   reinsert the block.
//
//  - heap (IN): Handle to the heap from which the memory is being reallocated.
//
//  - mem (IN): Pointer to the memory block being reallocated.
//
//  - newmem (IN): Pointer to the memory block being returned to the caller
//      that requested the reallocation. This pointer may or may not be the same
//      as the original memory block (as pointed to by "mem").
//
//  - size (IN): Size, in bytes, of the new memory block.
//
//  - framepointer (IN): The frame pointer at which this reallocation entered
//      VLD's code. Used for determining the starting point of the stack trace.
//
//  - crtalloc (IN): Should be set to TRUE if this reallocation is for a CRT
//      memory block. Otherwise should be set to FALSE.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::remapblock (HANDLE heap, LPCVOID mem, LPCVOID newmem, SIZE_T size, SIZE_T framepointer,
                                     BOOL crtalloc)
{
    BlockMap::Iterator   blockit;
    BlockMap            *blockmap;
    HeapMap::Iterator    heapit;
    blockinfo_t         *info;

    if (newmem != mem) {
        // The block was not reallocated in-place. Instead the old block was
        // freed and a new block allocated to satisfy the new size.
        unmapblock(heap, mem);
        mapblock(heap, newmem, size, framepointer, crtalloc);
        return;
    }

    // The block was reallocated in-place. Find the existing blockinfo_t
    // entry in the block map and update it with the new callstack and size.
    EnterCriticalSection(&m_maplock);
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // We haven't mapped this heap to a block map yet. Obviously the
        // block has also not been mapped to a blockinfo_t entry yet either,
        // so treat this reallocation as a brand-new allocation (this will
        // also map the heap to a new block map).
        mapblock(heap, newmem, size, framepointer, crtalloc);
        LeaveCriticalSection(&m_maplock);
        return;
    }

    // Find the block's blockinfo_t structure so that we can update it.
    blockmap = &(*heapit).second->blockmap;
    blockit = blockmap->find(mem);
    if (blockit == blockmap->end()) {
        // The block hasn't been mapped to a blockinfo_t entry yet.
        // Treat this reallocation as a new allocation.
        mapblock(heap, newmem, size, framepointer, crtalloc);
        LeaveCriticalSection(&m_maplock);
        return;
    }

    // Found the blockinfo_t entry for this block. Update it with
    // a new callstack and new size.
    info = (*blockit).second;
    info->callstack->clear();
    if (crtalloc) {
        // The heap that this block was allocated from is a CRT heap.
        (*heapit).second->flags |= VLD_HEAP_CRT;
    }
    LeaveCriticalSection(&m_maplock);

    // Update the block's callstack and size.
    if (m_options & VLD_OPT_TRACE_INTERNAL_FRAMES) {
        // Passing NULL for the frame pointer argument will force
        // the stack trace to begin at the current frame.
        info->callstack->getstacktrace(m_maxtraceframes, NULL);
    }
    else {
        // Start the stack trace at the call that first entered
        // VLD's code.
        info->callstack->getstacktrace(m_maxtraceframes, (SIZE_T*)framepointer);
    }
    info->size = size;
}

// reportconfig - Generates a brief report summarizing Visual Leak Detector's
//   configuration, as loaded from the vld.ini file.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::reportconfig ()
{
    if (m_options & VLD_OPT_AGGREGATE_DUPLICATES) {
        report(L"    Aggregating duplicate leaks.\n");
    }
    if (wcslen(m_forcedmodulelist) != 0) {
        report(L"    Forcing inclusion of these modules in leak detection: %s\n", m_forcedmodulelist);
    }
    if (m_maxdatadump != VLD_DEFAULT_MAX_DATA_DUMP) {
        if (m_maxdatadump == 0) {
            report(L"    Suppressing data dumps.\n");
        }
        else {
            report(L"    Limiting data dumps to %lu bytes.\n", m_maxdatadump);
        }
    }
    if (m_maxtraceframes != VLD_DEFAULT_MAX_TRACE_FRAMES) {
        report(L"    Limiting stack traces to %u frames.\n", m_maxtraceframes);
    }
    if (m_options & VLD_OPT_UNICODE_REPORT) {
        report(L"    Generating a Unicode (UTF-16) encoded report.\n");
    }
    if (m_options & VLD_OPT_REPORT_TO_FILE) {
        if (m_options & VLD_OPT_REPORT_TO_DEBUGGER) {
            report(L"    Outputting the report to the debugger and to %s\n", m_reportfilepath);
        }
        else {
            report(L"    Outputting the report to %s\n", m_reportfilepath);
        }
    }
    if (m_options & VLD_OPT_SLOW_DEBUGGER_DUMP) {
        report(L"    Outputting the report to the debugger at a slower rate.\n");
    }
    if (m_options & VLD_OPT_SAFE_STACK_WALK) {
        report(L"    Using the \"safe\" (but slow) stack walking method.\n");
    }
    if (m_options & VLD_OPT_SELF_TEST) {
        report(L"    Perfoming a memory leak self-test.\n");
    }
    if (m_options & VLD_OPT_START_DISABLED) {
        report(L"    Starting with memory leak detection disabled.\n");
    }
    if (m_options & VLD_OPT_TRACE_INTERNAL_FRAMES) {
        report(L"    Including heap and VLD internal frames in stack traces.\n");
    }
}

// reportleaks - Generates a memory leak report for the specified heap.
//
//  - heap (IN): Handle to the heap for which to generate a memory leak
//      report.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::reportleaks (HANDLE heap)
{
    LPCVOID              address;
    LPCVOID              block;
    BlockMap::Iterator   blockit;
    BlockMap            *blockmap;
    crtdbgblockheader_t *crtheader;
    SIZE_T               duplicates;
    heapinfo_t          *heapinfo;
    HeapMap::Iterator    heapit;
    blockinfo_t         *info;
    SIZE_T               size;

    // Find the heap's information (blockmap, etc).
    EnterCriticalSection(&m_maplock);
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // Nothing is allocated from this heap. No leaks.
        LeaveCriticalSection(&m_maplock);
        return;
    }

    heapinfo = (*heapit).second;
    blockmap = &heapinfo->blockmap;
    for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
        // Found a block which is still in the BlockMap. We've identified a
        // potential memory leak.
        block = (*blockit).first;
        info = (*blockit).second;
        address = block;
        size = info->size;
        if (heapinfo->flags & VLD_HEAP_CRT) {
            // This block is allocated to a CRT heap, so the block has a CRT
            // memory block header prepended to it.
            crtheader = (crtdbgblockheader_t*)block;
            if (CRT_USE_TYPE(crtheader->use) == CRT_USE_INTERNAL) {
                // This block is marked as being used internally by the CRT.
                // The CRT will free the block after VLD is destroyed.
                continue;
            }
            // The CRT header is more or less transparent to the user, so
            // the information about the contained block will probably be
            // more useful to the user. Accordingly, that's the information
            // we'll include in the report.
            address = CRTDBGBLOCKDATA(block);
            size = crtheader->size;
        }
        // It looks like a real memory leak.
        if (m_leaksfound == 0) {
            report(L"WARNING: Visual Leak Detector detected memory leaks!\n");
        }
        m_leaksfound++;
        report(L"---------- Block %ld at " ADDRESSFORMAT L": %u bytes ----------\n", info->serialnumber, address, size);
        if (m_options & VLD_OPT_AGGREGATE_DUPLICATES) {
            // Aggregate all other leaks which are duplicates of this one
            // under this same heading, to cut down on clutter.
            duplicates = eraseduplicates(blockit);
            if (duplicates) {
                report(L"A total of %lu leaks match this size and call stack. Showing only the first one.\n",
                       duplicates + 1);
                m_leaksfound += duplicates;
            }
        }
        // Dump the call stack.
        report(L"  Call Stack:\n");
        info->callstack->dump(m_options & VLD_OPT_TRACE_INTERNAL_FRAMES);
        // Dump the data in the user data section of the memory block.
        if (m_maxdatadump != 0) {
            report(L"  Data:\n");
            if (m_options & VLD_OPT_UNICODE_REPORT) {
                dumpmemoryw(address, (m_maxdatadump < size) ? m_maxdatadump : size);
            }
            else {
                dumpmemorya(address, (m_maxdatadump < size) ? m_maxdatadump : size);
            }
        }
        report(L"\n");
    }

    LeaveCriticalSection(&m_maplock);
}

// unmapblock - Tracks memory blocks that are freed. Unmaps the specified block
//   from the block's information, relinquishing internally allocated resources.
//
//  - heap (IN): Handle to the heap to which this block is being freed.
//
//  - mem (IN): Pointer to the memory block being freed.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::unmapblock (HANDLE heap, LPCVOID mem)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    HeapMap::Iterator   heapit;
    blockinfo_t        *info;

    // Find this heap's block map.
    EnterCriticalSection(&m_maplock);
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // We don't have a block map for this heap. We must not have monitored
        // this allocation (probably happened before VLD was initialized).
        LeaveCriticalSection(&m_maplock);
        return;
    }

    // Find this block in the block map.
    blockmap = &(*heapit).second->blockmap;
    blockit = blockmap->find(mem);
    if (blockit == blockmap->end()) {
        // This block is not in the block map. We must not have monitored this
        // allocation (probably happened before VLD was initialized).
        LeaveCriticalSection(&m_maplock);
        return;
    }

    // Free the blockinfo_t structure and erase it from the block map.
    info = (*blockit).second;
    delete info->callstack;
    delete info;
    blockmap->erase(blockit);
    LeaveCriticalSection(&m_maplock);
}

// unmapheap - Tracks heap destruction. Unmaps the specified heap from its block
//   map. The block map is cleared and deleted, relinquishing internally
//   allocated resources.
//
//  - heap (IN): Handle to the heap which is being destroyed.
//
//  Return Value:
//
//    None.
//
VOID VisualLeakDetector::unmapheap (HANDLE heap)
{
    BlockMap::Iterator  blockit;
    BlockMap           *blockmap;
    heapinfo_t         *heapinfo;
    HeapMap::Iterator   heapit;

    // Find this heap's block map.
    EnterCriticalSection(&m_maplock);
    heapit = m_heapmap->find(heap);
    if (heapit == m_heapmap->end()) {
        // This heap hasn't been mapped. We must not have monitored this heap's
        // creation (probably happened before VLD was initialized).
        LeaveCriticalSection(&m_maplock);
        return;
    }

    // Free all of the blockinfo_t structures stored in the block map.
    heapinfo = (*heapit).second;
    blockmap = &heapinfo->blockmap;
    for (blockit = blockmap->begin(); blockit != blockmap->end(); ++blockit) {
        delete (*blockit).second->callstack;
        delete (*blockit).second;
    }
    delete heapinfo;

    // Remove this heap's block map from the heap map.
    m_heapmap->erase(heapit);
    LeaveCriticalSection(&m_maplock);
}
