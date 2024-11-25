////////////////////////////////////////////////////////////////////////////////
//  $Id: dbghelpapi.cpp,v 1.3 2006/11/12 18:09:19 dmouldin Exp $
//
//  Visual Leak Detector (Version 1.9d) - Global DbgHelp API Function Pointers
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

#define VLDBUILD

#include "dbghelpapi.h"

// Global function pointers for explicit dynamic linking with the Debug Help
// Library APIs. Though these functions coule be load-time linked, we do an
// explicit dynmaic link to ensure that we link with the version of the library
// that was installed by VLD.
EnumerateLoadedModulesW64_t    pEnumerateLoadedModulesW64;
ImageDirectoryEntryToDataEx_t  pImageDirectoryEntryToDataEx;
StackWalk64_t                  pStackWalk64;
SymCleanup_t                   pSymCleanup;
SymFromAddrW_t                 pSymFromAddrW;
SymFunctionTableAccess64_t     pSymFunctionTableAccess64;
SymGetLineFromAddrW64_t        pSymGetLineFromAddrW64;
SymGetModuleBase64_t           pSymGetModuleBase64;
SymGetModuleInfoW64_t          pSymGetModuleInfoW64;
SymInitializeW_t               pSymInitializeW;
SymLoadModule64_t              pSymLoadModule64;
SymSetOptions_t                pSymSetOptions;
SymUnloadModule64_t            pSymUnloadModule64;