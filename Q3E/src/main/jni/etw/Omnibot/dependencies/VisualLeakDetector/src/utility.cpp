////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Various Utility Functions
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
#include <cstdio>
#include <windows.h>
#define __out_xcount(x) // Workaround for the specstrings.h bug in the Platform SDK.
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>    // Provides portable executable (PE) image access functions.
#define VLDBUILD        // Declares that we are building Visual Leak Detector.
#include "utility.h"    // Provides various utility functions and macros.
#include "vldheap.h"    // Provides internal new and delete operators.

// Imported Global Variables
extern CRITICAL_SECTION imagelock;

// Global variables.
static BOOL        reportdelay = FALSE;     // If TRUE, we sleep for a bit after calling OutputDebugString to give the debugger time to catch up.
static FILE       *reportfile = NULL;       // Pointer to the file, if any, to send the memory leak report to.
static BOOL        reporttodebugger = TRUE; // If TRUE, a copy of the memory leak report will be sent to the debugger for display.
static encoding_e  reportencoding = ascii;  // Output encoding of the memory leak report.

// dumpmemorya - Dumps a nicely formatted rendition of a region of memory.
//   Includes both the hex value of each byte and its ASCII equivalent (if
//   printable).
//
//  - address (IN): Pointer to the beginning of the memory region to dump.
//
//  - size (IN): The size, in bytes, of the region to dump.
//
//  Return Value:
//
//    None.
//
VOID dumpmemorya (LPCVOID address, SIZE_T size)
{
    WCHAR  ascdump [18] = {0};
    SIZE_T ascindex;
    BYTE   byte;
    SIZE_T byteindex;
    SIZE_T bytesdone;
    SIZE_T dumplen;
    WCHAR  formatbuf [BYTEFORMATBUFFERLENGTH];
    WCHAR  hexdump [HEXDUMPLINELENGTH] = {0};
    SIZE_T hexindex;

    // Each line of output is 16 bytes.
    if ((size % 16) == 0) {
        // No padding needed.
        dumplen = size;
    }
    else {
        // We'll need to pad the last line out to 16 bytes.
        dumplen = size + (16 - (size % 16));
    }

    // For each byte of data, get both the ASCII equivalent (if it is a
    // printable character) and the hex representation.
    bytesdone = 0;
    for (byteindex = 0; byteindex < dumplen; byteindex++) {
        hexindex = 3 * ((byteindex % 16) + ((byteindex % 16) / 4)); // 3 characters per byte, plus a 3-character space after every 4 bytes.
        ascindex = (byteindex % 16) + (byteindex % 16) / 8;         // 1 character per byte, plus a 1-character space after every 8 bytes.
        if (byteindex < size) {
            byte = ((PBYTE)address)[byteindex];
            _snwprintf_s(formatbuf, BYTEFORMATBUFFERLENGTH, _TRUNCATE, L"%.2X ", byte);
            formatbuf[3] = '\0';
            wcsncpy_s(hexdump + hexindex, HEXDUMPLINELENGTH - hexindex, formatbuf, 4);
            if (isgraph(byte)) {
                ascdump[ascindex] = (WCHAR)byte;
            }
            else {
                ascdump[ascindex] = L'.';
            }
        }
        else {
            // Add padding to fill out the last line to 16 bytes.
            wcsncpy_s(hexdump + hexindex, HEXDUMPLINELENGTH - hexindex, L"   ", 4);
            ascdump[ascindex] = L'.';
        }
        bytesdone++;
        if ((bytesdone % 16) == 0) {
            // Print one line of data for every 16 bytes. Include the
            // ASCII dump and the hex dump side-by-side.
            report(L"    %s    %s\n", hexdump, ascdump);
        }
        else {
            if ((bytesdone % 8) == 0) {
                // Add a spacer in the ASCII dump after every 8 bytes.
                ascdump[ascindex + 1] = L' ';
            }
            if ((bytesdone % 4) == 0) {
                // Add a spacer in the hex dump after every 4 bytes.
                wcsncpy_s(hexdump + hexindex + 3, HEXDUMPLINELENGTH - hexindex - 3, L"   ", 4);
            }
        }
    }
}

// dumpmemoryw - Dumps a nicely formatted rendition of a region of memory.
//   Includes both the hex value of each byte and its Unicode equivalent.
//
//  - address (IN): Pointer to the beginning of the memory region to dump.
//
//  - size (IN): The size, in bytes, of the region to dump.
//
//  Return Value:
//
//    None.
//
VOID dumpmemoryw (LPCVOID address, SIZE_T size)
{
    BYTE   byte;
    SIZE_T byteindex;
    SIZE_T bytesdone;
    SIZE_T dumplen;
    WCHAR  formatbuf [BYTEFORMATBUFFERLENGTH];
    WCHAR  hexdump [HEXDUMPLINELENGTH] = {0};
    SIZE_T hexindex;
    WORD   word;
    WCHAR  unidump [18] = {0};
    SIZE_T uniindex;

    // Each line of output is 16 bytes.
    if ((size % 16) == 0) {
        // No padding needed.
        dumplen = size;
    }
    else {
        // We'll need to pad the last line out to 16 bytes.
        dumplen = size + (16 - (size % 16));
    }

    // For each word of data, get both the Unicode equivalent and the hex
    // representation.
    bytesdone = 0;
    for (byteindex = 0; byteindex < dumplen; byteindex++) {
        hexindex = 3 * ((byteindex % 16) + ((byteindex % 16) / 4));   // 3 characters per byte, plus a 3-character space after every 4 bytes.
        uniindex = ((byteindex / 2) % 8) + ((byteindex / 2) % 8) / 8; // 1 character every other byte, plus a 1-character space after every 8 bytes.
        if (byteindex < size) {
            byte = ((PBYTE)address)[byteindex];
            _snwprintf_s(formatbuf, BYTEFORMATBUFFERLENGTH, _TRUNCATE, L"%.2X ", byte);
            formatbuf[BYTEFORMATBUFFERLENGTH - 1] = '\0';
            wcsncpy_s(hexdump + hexindex, HEXDUMPLINELENGTH - hexindex, formatbuf, 4);
            if (((byteindex % 2) == 0) && ((byteindex + 1) < dumplen)) {
                // On every even byte, print one character.
                word = ((PWORD)address)[byteindex / 2];
                if ((word == 0x0000) || (word == 0x0020)) {
                    unidump[uniindex] = L'.';
                }
                else {
                    unidump[uniindex] = word;
                }
            }
        }
        else {
            // Add padding to fill out the last line to 16 bytes.
            wcsncpy_s(hexdump + hexindex, HEXDUMPLINELENGTH - hexindex, L"   ", 4);
            unidump[uniindex] = L'.';
        }
        bytesdone++;
        if ((bytesdone % 16) == 0) {
            // Print one line of data for every 16 bytes. Include the
            // ASCII dump and the hex dump side-by-side.
            report(L"    %s    %s\n", hexdump, unidump);
        }
        else {
            if ((bytesdone % 8) == 0) {
                // Add a spacer in the ASCII dump after every 8 bytes.
                unidump[uniindex + 1] = L' ';
            }
            if ((bytesdone % 4) == 0) {
                // Add a spacer in the hex dump after every 4 bytes.
                wcsncpy_s(hexdump + hexindex + 3, HEXDUMPLINELENGTH - hexindex - 3, L"   ", 4);
            }
        }
    }
}

// findimport - Determines if the specified module imports the named import
//   from the named exporting module.
//
//  - importmodule (IN): Handle (base address) of the module to be searched to
//      see if it imports the specified import.
//
//  - exportmodule (IN): Handle (base address) of the module that exports the
//      import to be searched for.
//
//  - exportmodulename (IN): ANSI string containing the name of the module that
//      exports the import to be searched for.
//
//  - importname (IN): ANSI string containing the name of the import to search
//      for. May be an integer cast to a string if the import is exported by
//      ordinal.
//
//  Return Value:
//
//    Returns TRUE if the module imports to the specified import. Otherwise
//    returns FALSE.
//
BOOL findimport (HMODULE importmodule, HMODULE exportmodule, LPCSTR exportmodulename, LPCSTR importname)
{
    IMAGE_THUNK_DATA        *iate;
    IMAGE_IMPORT_DESCRIPTOR *idte;
    FARPROC                  import;
    IMAGE_SECTION_HEADER    *section;
    ULONG                    size;
            
    // Locate the importing module's Import Directory Table (IDT) entry for the
    // exporting module. The importing module actually can have several IATs --
    // one for each export module that it imports something from. The IDT entry
    // gives us the offset of the IAT for the module we are interested in.
    EnterCriticalSection(&imagelock);
    idte = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx((PVOID)importmodule, TRUE,
                                                                  IMAGE_DIRECTORY_ENTRY_IMPORT, &size, &section);
    LeaveCriticalSection(&imagelock);
    if (idte == NULL) {
        // This module has no IDT (i.e. it imports nothing).
        return FALSE;
    }
    while (idte->OriginalFirstThunk != 0x0) {
        if (_stricmp((PCHAR)R2VA(importmodule, idte->Name), exportmodulename) == 0) {
            // Found the IDT entry for the exporting module.
            break;
        }
        idte++;
    }
    if (idte->OriginalFirstThunk == 0x0) {
        // The importing module does not import anything from the exporting
        // module.
        return FALSE;
    }
    
    // Get the *real* address of the import. If we find this address in the IAT,
    // then we've found that the module does import the named import.
    import = GetProcAddress(exportmodule, importname);
    assert(import != NULL); // Perhaps the named export module does not actually export the named import?

    // Locate the import's IAT entry.
    iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
    while (iate->u1.Function != 0x0) {
        if (iate->u1.Function == (DWORD_PTR)import) {
            // Found the IAT entry. The module imports the named import.
            return TRUE;
        }
        iate++;
    }

    // The module does not import the named import.
    return FALSE;
}

// findpatch - Determines if the specified module has been patched to use the
//   specified replacement.
//
//  - importmodule (IN): Handle (base address) of the module to be searched to
//      see if it imports the specified replacement export.
//
//  - exportmodulename (IN): ANSI string containing the name of the module that
//      normally exports that import that would have been patched to use the
//      replacement export.
//
//  - replacement (IN): Address of the replacement, or destination, function or
//      variable to search for.
//
//  Return Value:
//
//    Returns TRUE if the module has been patched to use the specified
//    replacement export.
//
BOOL findpatch (HMODULE importmodule, LPCSTR exportmodulename, LPCVOID replacement)
{
    IMAGE_THUNK_DATA        *iate;
    IMAGE_IMPORT_DESCRIPTOR *idte;
    IMAGE_SECTION_HEADER    *section;
    ULONG                    size;
            
    // Locate the importing module's Import Directory Table (IDT) entry for the
    // exporting module. The importing module actually can have several IATs --
    // one for each export module that it imports something from. The IDT entry
    // gives us the offset of the IAT for the module we are interested in.
    EnterCriticalSection(&imagelock);
    idte = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx((PVOID)importmodule, TRUE,
                                                                  IMAGE_DIRECTORY_ENTRY_IMPORT, &size, &section);
    LeaveCriticalSection(&imagelock);
    if (idte == NULL) {
        // This module has no IDT (i.e. it imports nothing).
        return FALSE;
    }
    while (idte->OriginalFirstThunk != 0x0) {
        if (_stricmp((PCHAR)R2VA(importmodule, idte->Name), exportmodulename) == 0) {
            // Found the IDT entry for the exporting module.
            break;
        }
        idte++;
    }
    if (idte->OriginalFirstThunk == 0x0) {
        // The importing module does not import anything from the exporting
        // module.
        return FALSE;
    }
    
    // Locate the replacement's IAT entry.
    iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
    while (iate->u1.Function != 0x0) {
        if (iate->u1.Function == (DWORD_PTR)replacement) {
            // Found the IAT entry for the replacement. This patch has been
            // installed.
            return TRUE;
        }
        iate++;
    }

    // The module does not import the replacement. The patch has not been
    // installed.
    return FALSE;
}

// insertreportdelay - Sets the report function to sleep for a bit after each
//   call to OutputDebugString, in order to allow the debugger to catch up.
//
//  Return Value:
//
//    None.
//
VOID insertreportdelay ()
{
    reportdelay = TRUE;
}

// moduleispatched - Checks to see if any of the imports listed in the specified
//   patch table have been patched into the specified importmodule.
//
//  - importmodule (IN): Handle (base address) of the module to be queried to
//      determine if it has been patched.
//
//  - patchtable (IN): An array of patchentry_t structures specifying all of the
//      import patches to search for.
//
//  - tablesize (IN): Size, in entries, of the specified patch table.
//
//  Return Value:
//
//    Returns TRUE if at least one of the patches listed in the patch table is
//    installed in the importmodule. Otherwise returns FALSE.
//
BOOL moduleispatched (HMODULE importmodule, patchentry_t patchtable [], UINT tablesize)
{
    patchentry_t *entry;
    BOOL          found = FALSE;
    UINT          index;

    // Loop through the import patch table, individually checking each patch
    // entry to see if it is installed in the import module. If any patch entry
    // is installed in the import module, then the module has been patched.
    for (index = 0; index < tablesize; index++) {
        entry = &patchtable[index];
        found = findpatch(importmodule, entry->exportmodulename, entry->replacement);
        if (found == TRUE) {
            // Found one of the listed patches installed in the import module.
            return TRUE;
        }
    }

    // No patches listed in the patch table were found in the import module.
    return FALSE;
}

// patchimport - Patches all future calls to an imported function, or references
//   to an imported variable, through to a replacement function or variable.
//   Patching is done by replacing the import's address in the specified target
//   module's Import Address Table (IAT) with the address of the replacement
//   function or variable.
//
//  - importmodule (IN): Handle (base address) of the target module for which
//      calls or references to the import should be patched.
//
//  - exportmodule (IN): Handle (base address) of the module that exports the
//      the function or variable to be patched.
//
//  - exportmodulename (IN): ANSI string containing the name of the module that
//      exports the function or variable to be patched.
//
//  - importname (IN): ANSI string containing the name of the imported function
//      or variable to be patched. May be an integer cast to a string if the
//      import is exported by ordinal.
//
//  - replacement (IN): Address of the function or variable to which future
//      calls or references should be patched through to. This function or
//      variable can be thought of as effectively replacing the original import
//      from the point of view of the module specified by "importmodule".
//
//  Return Value:
//
//    Returns TRUE if the patch was installed into the import module. If the
//    import module does not import the specified export, so nothing changed,
//    then FALSE will be returned.
//   
BOOL patchimport (HMODULE importmodule, HMODULE exportmodule, LPCSTR exportmodulename, LPCSTR importname,
                  LPCVOID replacement)
{
    IMAGE_THUNK_DATA        *iate;
    IMAGE_IMPORT_DESCRIPTOR *idte;
    FARPROC                  import;
    DWORD                    protect;
    IMAGE_SECTION_HEADER    *section;
    ULONG                    size;

    // Locate the importing module's Import Directory Table (IDT) entry for the
    // exporting module. The importing module actually can have several IATs --
    // one for each export module that it imports something from. The IDT entry
    // gives us the offset of the IAT for the module we are interested in.
    EnterCriticalSection(&imagelock);
    idte = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx((PVOID)importmodule, TRUE,
                                                                  IMAGE_DIRECTORY_ENTRY_IMPORT, &size, &section);
    LeaveCriticalSection(&imagelock);
    if (idte == NULL) {
        // This module has no IDT (i.e. it imports nothing).
        return FALSE;
    }
    while (idte->OriginalFirstThunk != 0x0) {
        if (_stricmp((PCHAR)R2VA(importmodule, idte->Name), exportmodulename) == 0) {
            // Found the IDT entry for the exporting module.
            break;
        }
        idte++;
    }
    if (idte->OriginalFirstThunk == 0x0) {
        // The importing module does not import anything from the exporting
        // module.
        return FALSE;
    }
    
    // Get the *real* address of the import. If we find this address in the IAT,
    // then we've found the entry that needs to be patched.
    import = GetProcAddress(exportmodule, importname);
    assert(import != NULL); // Perhaps the named export module does not actually export the named import?

    // Locate the import's IAT entry.
    iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
    while (iate->u1.Function != 0x0) {
        if (iate->u1.Function == (DWORD_PTR)import) {
            // Found the IAT entry. Overwrite the address stored in the IAT
            // entry with the address of the replacement. Note that the IAT
            // entry may be write-protected, so we must first ensure that it is
            // writable.
            VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function), PAGE_READWRITE, &protect);
            iate->u1.Function = (DWORD_PTR)replacement;
            VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function), protect, &protect);

            // The patch has been installed in the import module.
            return TRUE;
        }
        iate++;
    }

    // The import's IAT entry was not found. The importing module does not
    // import the specified import.
    return FALSE;
}

// patchmodule - Patches all imports listed in the supplied patch table, and
//   which are imported by the specified module, through to their respective
//   replacement functions.
//
//   Note: If the specified module does not import any of the functions listed
//     in the patch table, then nothing is changed for the specified module.
//
//  - importmodule (IN): Handle (base address) of the target module which is to
//      have its imports patched.
//
//  - patchtable (IN): An array of patchentry_t structures specifying all of the
//      imports to patch for the specified module.
//
//  - tablesize (IN): Size, in entries, of the specified patch table.
//
//  Return Value:
//
//    Returns TRUE if at least one of the patches listed in the patch table was
//    installed in the importmodule. Otherwise returns FALSE.
//
BOOL patchmodule (HMODULE importmodule, patchentry_t patchtable [], UINT tablesize)
{
    patchentry_t *entry;
    UINT          index;
    BOOL          patched = FALSE;

    // Loop through the import patch table, individually patching each import
    // listed in the table.
    for (index = 0; index < tablesize; index++) {
        entry = &patchtable[index];
        if (patchimport(importmodule, (HMODULE)entry->modulebase, entry->exportmodulename, entry->importname,
                        entry->replacement) == TRUE) {
            patched = TRUE;
        }
    }

    return patched;
}

// report - Sends a printf-style formatted message to the debugger for display
//   and/or to a file.
//
//   Note: A message longer than MAXREPORTLENGTH characters will be truncated
//     to MAXREPORTLENGTH.
//
//  - format (IN): Specifies a printf-compliant format string containing the
//      message to be sent to the debugger.
//
//  - ... (IN): Arguments to be formatted using the specified format string.
//
//  Return Value:
//
//    None.
//
VOID report (LPCWSTR format, ...)
{
    va_list args;
    size_t  count;
    CHAR    messagea [MAXREPORTLENGTH + 1];
    WCHAR   messagew [MAXREPORTLENGTH + 1];

    va_start(args, format);
    _vsnwprintf_s(messagew, MAXREPORTLENGTH + 1, _TRUNCATE, format, args);
    va_end(args);
    messagew[MAXREPORTLENGTH] = L'\0';

    if (reportencoding == unicode) {
        if (reportfile != NULL) {
            // Send the report to the previously specified file.
            fwrite(messagew, sizeof(WCHAR), wcslen(messagew), reportfile);
        }
        if (reporttodebugger) {
            OutputDebugStringW(messagew);
        }
    }
    else {
        if (wcstombs_s(&count, messagea, MAXREPORTLENGTH + 1, messagew, _TRUNCATE) == -1) {
            // Failed to convert the Unicode message to ASCII.
            assert(FALSE);
            return;
        }
        messagea[MAXREPORTLENGTH] = '\0';
        if (reportfile != NULL) {
            // Send the report to the previously specified file.
            fwrite(messagea, sizeof(CHAR), strlen(messagea), reportfile);
        }
        if (reporttodebugger) {
            OutputDebugStringA(messagea);
        }
    }

    if (reporttodebugger && (reportdelay == TRUE)) {
        Sleep(10); // Workaround the Visual Studio 6 bug where debug strings are sometimes lost if they're sent too fast.
    }
}

// restoreimport - Restores the IAT entry for an import previously patched via
//   a call to "patchimport" to the original address of the import.
//
//  - importmodule (IN): Handle (base address) of the target module for which
//      calls or references to the import should be restored.
//
//  - exportmodule (IN): Handle (base address) of the module that exports the
//      function or variable to be patched.
//
//  - exportmodulename (IN): ANSI string containing the name of the module that
//      exports the function or variable to be patched.
//
//  - importname (IN): ANSI string containing the name of the imported function
//      or variable to be restored. May be an integer cast to a string if the
//      import is exported by ordinal.
//
//  - replacement (IN): Address of the function or variable which the import was
//      previously patched through to via a call to "patchimport".
//
//  Return Value:
//
//    None.
//   
VOID restoreimport (HMODULE importmodule, HMODULE exportmodule, LPCSTR exportmodulename, LPCSTR importname,
                    LPCVOID replacement)
{
    IMAGE_THUNK_DATA        *iate;
    IMAGE_IMPORT_DESCRIPTOR *idte;
    FARPROC                  import;
    DWORD                    protect;
    IMAGE_SECTION_HEADER    *section;
    ULONG                    size;

    // Locate the importing module's Import Directory Table (IDT) entry for the
    // exporting module. The importing module actually can have several IATs --
    // one for each export module that it imports something from. The IDT entry
    // gives us the offset of the IAT for the module we are interested in.
    EnterCriticalSection(&imagelock);
    idte = (IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToDataEx((PVOID)importmodule, TRUE,
                                                                  IMAGE_DIRECTORY_ENTRY_IMPORT, &size, &section);
    LeaveCriticalSection(&imagelock);
    if (idte == NULL) {
        // This module has no IDT (i.e. it imports nothing).
        return;
    }
    while (idte->OriginalFirstThunk != 0x0) {
        if (_stricmp((PCHAR)R2VA(importmodule, idte->Name), exportmodulename) == 0) {
            // Found the IDT entry for the exporting module.
            break;
        }
        idte++;
    }
    if (idte->OriginalFirstThunk == 0x0) {
        // The importing module does not import anything from the exporting
        // module.
        return;
    }
    
    // Get the *real* address of the import.
    import = GetProcAddress(exportmodule, importname);
    assert(import != NULL); // Perhaps the named export module does not actually export the named import?

    // Locate the import's original IAT entry (it currently has the replacement
    // address in it).
    iate = (IMAGE_THUNK_DATA*)R2VA(importmodule, idte->FirstThunk);
    while (iate->u1.Function != 0x0) {
        if (iate->u1.Function == (DWORD_PTR)replacement) {
            // Found the IAT entry. Overwrite the address stored in the IAT
            // entry with the import's real address. Note that the IAT entry may
            // be write-protected, so we must first ensure that it is writable.
            VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function), PAGE_READWRITE, &protect);
            iate->u1.Function = (DWORD_PTR)import;
            VirtualProtect(&iate->u1.Function, sizeof(iate->u1.Function), protect, &protect);
            break;
        }
        iate++;
    }
}

// restoremodule - Restores all imports listed in the supplied patch table, and
//   which are imported by the specified module, to their original functions.
//
//   Note: If the specified module does not import any of the functions listed
//     in the patch table, then nothing is changed for the specified module.
//
//  - importmodule (IN): Handle (base address) of the target module which is to
//      have its imports restored.
//
//  - patchtable (IN): Array of patchentry_t structures specifying all of the
//      imports to restore for the specified module.
//
//  - tablesize (IN): Size, in entries, of the specified patch table.
//
//  Return Value:
//
//    None.
//
VOID restoremodule (HMODULE importmodule, patchentry_t patchtable [], UINT tablesize)
{
    patchentry_t *entry;
    UINT          index;

    // Loop through the import patch table, individually restoring each import
    // listed in the table.
    for (index = 0; index < tablesize; index++) {
        entry = &patchtable[index];
        restoreimport(importmodule, (HMODULE)entry->modulebase, entry->exportmodulename, entry->importname,
                      entry->replacement);
    }
}

// setreportencoding - Sets the output encoding of report messages to either
//   ASCII (the default) or Unicode.
//
//  - encoding (IN): Specifies either "ascii" or "unicode".
//
//  Return Value:
//
//    None.
//
VOID setreportencoding (encoding_e encoding)
{
    switch (encoding) {
    case ascii:
    case unicode:
        reportencoding = encoding;
        break;

    default:
        assert(FALSE);
    }
}

// setreportfile - Sets a destination file to which all report messages should
//   be sent. If this function is not called to set a destination file, then
//   report messages will be sent to the debugger instead of to a file.
//
//  - file (IN): Pointer to an open file, to which future report messages should
//      be sent.
//
//  - copydebugger (IN): If true, in addition to sending report messages to
//      the specified file, a copy of each message will also be sent to the
//      debugger.
//
//  Return Value:
//
//    None.
//
VOID setreportfile (FILE *file, BOOL copydebugger)
{
    reportfile = file;
    reporttodebugger = copydebugger;
}

// strapp - Appends the specified source string to the specified destination
//   string. Allocates additional space so that the destination string "grows"
//   as new strings are appended to it. This function is fairly infrequently
//   used so efficiency is not a major concern.
//
//  - dest (IN/OUT): Address of the destination string. Receives the resulting
//      combined string after the append operation.
//
//  - source (IN): Source string to be appended to the destination string.
//
//  Return Value:
//
//    None.
//
VOID strapp (LPWSTR *dest, LPCWSTR source)
{
    SIZE_T length;
    LPWSTR temp;

    temp = *dest;
    length = wcslen(*dest) + wcslen(source);
    *dest = new WCHAR [length + 1];
    wcsncpy_s(*dest, length + 1, temp, _TRUNCATE);
    wcsncat_s(*dest, length + 1, source, _TRUNCATE);
    delete [] temp;
}

// strtobool - Converts string values (e.g. "yes", "no", "on", "off") to boolean
//   values.
//
//  - s (IN): String value to convert.
//
//  Return Value:
//
//    Returns TRUE if the string is recognized as a "true" string. Otherwise
//    returns FALSE.
//
BOOL strtobool (LPCWSTR s) {
    WCHAR *end;

    if ((_wcsicmp(s, L"true") == 0) ||
        (_wcsicmp(s, L"yes") == 0) ||
        (_wcsicmp(s, L"on") == 0) ||
        (wcstol(s, &end, 10) == 1)) {
        return TRUE;
    }
    else {
        return FALSE;
    }
}
