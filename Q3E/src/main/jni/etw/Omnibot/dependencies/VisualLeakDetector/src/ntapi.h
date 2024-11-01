////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - NT API Definitions
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

// Return code type used by LdrLoadDll.
typedef ULONG NTSTATUS;
#define   STATUS_SUCCESS 0

// Unicode string structure used by NT APIs.
typedef struct unicodestring_s {
    USHORT length;    // Length of the string.
    USHORT maxlength; // Length of the buffer.
    PWSTR  buffer;    // The buffer containing the string.
} unicodestring_t;

// Function pointer types for explicit dynamic linking with functions that can't
// be load-time linked (no import library is available for these).
typedef NTSTATUS (__stdcall *LdrLoadDll_t) (LPWSTR, PDWORD, unicodestring_t *, PHANDLE);
typedef LPVOID (__stdcall *RtlAllocateHeap_t) (HANDLE, DWORD, SIZE_T);
typedef BOOL (__stdcall *RtlFreeHeap_t) (HANDLE, DWORD, LPVOID);
typedef LPVOID (__stdcall *RtlReAllocateHeap_t) (HANDLE, DWORD, LPVOID, SIZE_T);

// Provide forward declarations for the NT APIs for any source files that
// include this header.
extern LdrLoadDll_t        LdrLoadDll;
extern RtlAllocateHeap_t   RtlAllocateHeap;
extern RtlFreeHeap_t       RtlFreeHeap;
extern RtlReAllocateHeap_t RtlReAllocateHeap;
