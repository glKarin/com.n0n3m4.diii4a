////////////////////////////////////////////////////////////////////////////////
//  $Id: dbghelpapi.h,v 1.3 2006/11/12 18:09:19 dmouldin Exp $
//
//  Visual Leak Detector (Version 1.9d) - DbgHelp API Definitions
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
#define __out_xcount(x) // Workaround for the specstrings.h bug in the Platform SDK.
#define DBGHELP_TRANSLATE_TCHAR
#include <dbghelp.h>

// Function pointer types for explicit dynamic linking with functions from the
// Debug Help Library. Though these functions could be load-time linked, we
// do an explicit dynmaic link to ensure that we link with the version of the
// library that was installed by VLD.
typedef BOOL (__stdcall *EnumerateLoadedModulesW64_t) (HANDLE hProcess,
                                                       PENUMLOADED_MODULES_CALLBACKW64 EnumLoadedModulesCallback,
                                                       PVOID UserContext);
typedef PVOID (__stdcall *ImageDirectoryEntryToDataEx_t) (PVOID Base, BOOLEAN MappedAsImage, USHORT DirectoryEntry,
                                                          PULONG Size, PIMAGE_SECTION_HEADER *FoundHeader);
typedef BOOL (__stdcall *StackWalk64_t) (DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame,
                                         PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 *ReadMemoryRoutine,
                                         PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTablAccessRoutine,
                                         PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
                                         PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
typedef BOOL (__stdcall *SymCleanup_t) (HANDLE hProcess);
typedef BOOL (__stdcall *SymFromAddrW_t) (HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement,
                                          PSYMBOL_INFOW Symbol);
typedef PVOID (__stdcall *SymFunctionTableAccess64_t) (HANDLE hProcess, DWORD64 AddrBase);
typedef BOOL (__stdcall *SymGetLineFromAddrW64_t) (HANDLE hProcess, DWORD64 qwAddr, PDWORD pdwDisplacement,
                                                   PIMAGEHLP_LINEW64 Line64);
typedef DWORD64 (__stdcall *SymGetModuleBase64_t) (HANDLE hProcess, DWORD64 qwAddr);
typedef BOOL (__stdcall *SymGetModuleInfoW64_t) (HANDLE hProcess, DWORD64 qwAddr, PIMAGEHLP_MODULEW64 ModuleInfo);
typedef BOOL (__stdcall *SymInitializeW_t) (HANDLE hProcess, PCWSTR UserSearchPath, BOOL fInvadeProcess);
typedef DWORD64 (__stdcall *SymLoadModule64_t) (HANDLE hProcess, HANDLE hFile, PCSTR ImageName, PCSTR ModuleName,
                                                DWORD64 BaseOfDll, DWORD SizeOfDll);
typedef DWORD (__stdcall *SymSetOptions_t) (DWORD SymOptions);
typedef BOOL (__stdcall *SymUnloadModule64_t) (HANDLE hProcess, DWORD64 BaseOfDll);

// Provide forward declarations for the DbgHelp APIs for any source files that
// include this header.
extern EnumerateLoadedModulesW64_t    pEnumerateLoadedModulesW64;
extern ImageDirectoryEntryToDataEx_t  pImageDirectoryEntryToDataEx;
extern StackWalk64_t                  pStackWalk64;
extern SymCleanup_t                   pSymCleanup;
extern SymFromAddrW_t                 pSymFromAddrW;
extern SymFunctionTableAccess64_t     pSymFunctionTableAccess64;
extern SymGetLineFromAddrW64_t        pSymGetLineFromAddrW64;
extern SymGetModuleBase64_t           pSymGetModuleBase64;
extern SymGetModuleInfoW64_t          pSymGetModuleInfoW64;
extern SymInitializeW_t               pSymInitializeW;
extern SymLoadModule64_t              pSymLoadModule64;
extern SymSetOptions_t                pSymSetOptions;
extern SymUnloadModule64_t            pSymUnloadModule64;
