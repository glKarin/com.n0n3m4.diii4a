////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector - Exported APIs
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

#define VLDBUILD     // Declares that we are building Visual Leak Detector.
#include "vldint.h"  // Provides access to the Visual Leak Detector internals.
#include "vldheap.h" // Provides internal new and delete operators.

// Imported global variables.
extern VisualLeakDetector vld;

////////////////////////////////////////////////////////////////////////////////
//
//  Visual Leak Detector APIs - see vldapi.h for each function's details.
//

extern "C" __declspec(dllexport) void VLDDisable ()
{
    tls_t *tls;

    if (vld.m_options & VLD_OPT_VLDOFF) {
        // VLD has been turned off.
        return;
    }

    // Disable memory leak detection for the current thread. There are two flags
    // because if neither flag is set, it means that we are in the default or
    // "starting" state, which could be either enabled or disabled depending on
    // the configuration.
    tls = vld.gettls();
    tls->flags &= ~VLD_TLS_ENABLED;
    tls->flags |= VLD_TLS_DISABLED;
}

extern "C" __declspec(dllexport) void VLDEnable ()
{
    tls_t *tls;

    if (vld.m_options & VLD_OPT_VLDOFF) {
        // VLD has been turned off.
        return;
    }

    // Enable memory leak detection for the current thread.
    tls = vld.gettls();
    tls->flags &= ~VLD_TLS_DISABLED;
    tls->flags |= VLD_TLS_ENABLED;
    vld.m_status &= ~VLD_STATUS_NEVER_ENABLED;
}
