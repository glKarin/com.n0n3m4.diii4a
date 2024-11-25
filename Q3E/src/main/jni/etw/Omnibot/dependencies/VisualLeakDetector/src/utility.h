////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Various Utility Definitions
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

#include <cstdio>
#include <windows.h>

#ifdef _WIN64
#define ADDRESSFORMAT   L"0x%.16X" // Format string for 64-bit addresses
#else
#define ADDRESSFORMAT   L"0x%.8X"  // Format string for 32-bit addresses
#endif // _WIN64
#define BOM             0xFEFF     // Unicode byte-order mark.
#define MAXREPORTLENGTH 511        // Maximum length, in characters, of "report" messages.

// Architecture-specific definitions for x86 and x64
#if defined(_M_IX86)
#define SIZEOFPTR 4
#define X86X64ARCHITECTURE IMAGE_FILE_MACHINE_I386
#define AXREG Eax
#define BPREG Ebp
#define IPREG Eip
#define SPREG Esp
#elif defined(_M_X64)
#define SIZEOFPTR 8
#define X86X64ARCHITECTURE IMAGE_FILE_MACHINE_AMD64
#define AXREG Rax
#define BPREG Rbp
#define IPREG Rip
#define SPREG Rsp
#endif // _M_IX86

#if defined(_M_IX86) || defined (_M_X64)
#define FRAMEPOINTER(fp) __asm mov fp, BPREG // Copies the current frame pointer to the supplied variable.
#else
// If you want to retarget Visual Leak Detector to another processor
// architecture then you'll need to provide an architecture-specific macro to
// obtain the frame pointer (or other address) which can be used to obtain the
// return address and stack pointer of the calling frame.
#error "Visual Leak Detector is not supported on this architecture."
#endif // _M_IX86 || _M_X64

// Miscellaneous definitions
#define R2VA(modulebase, rva)  (((PBYTE)modulebase) + rva) // Relative Virtual Address to Virtual Address conversion.
#define BYTEFORMATBUFFERLENGTH 4
#define HEXDUMPLINELENGTH      58

// Reports can be encoded as either ASCII or Unicode (UTF-16).
enum encoding_e {
    ascii,
    unicode
};

// This structure allows us to build a table of APIs which should be patched
// through to replacement functions provided by VLD.
typedef struct patchentry_s
{
    LPCSTR  exportmodulename; // The name of the module exporting the patched API.
    LPCSTR  importname;       // The name (or ordinal) of the imported API being patched.
    SIZE_T  modulebase;       // The base address of the exporting module (filled in at runtime when the modules are loaded).
    LPCVOID replacement;      // Pointer to the function to which the imported API should be patched through to.
} patchentry_t;

// Utility functions. See function definitions for details.
VOID dumpmemorya (LPCVOID address, SIZE_T length);
VOID dumpmemoryw (LPCVOID address, SIZE_T length);
BOOL findimport (HMODULE importmodule, HMODULE exportmodule, LPCSTR exportmodulename, LPCSTR importname);
BOOL findpatch (HMODULE importmodule, LPCSTR exportmodulename, LPCVOID replacement);
VOID insertreportdelay ();
BOOL moduleispatched (HMODULE importmodule, patchentry_t patchtable [], UINT tablesize);
BOOL patchimport (HMODULE importmodule, HMODULE exportmodule, LPCSTR exportmodulename, LPCSTR importname,
                  LPCVOID replacement);
BOOL patchmodule (HMODULE importmodule, patchentry_t patchtable [], UINT tablesize);
VOID report (LPCWSTR format, ...);
VOID restoreimport (HMODULE importmodule, HMODULE exportmodule, LPCSTR exportmodulename, LPCSTR importname,
                    LPCVOID replacement);
VOID restoremodule (HMODULE importmodule, patchentry_t patchtable [], UINT tablesize);
VOID setreportencoding (encoding_e encoding);
VOID setreportfile (FILE *file, BOOL copydebugger);
VOID strapp (LPWSTR *dest, LPCWSTR source);
BOOL strtobool (LPCWSTR s);
