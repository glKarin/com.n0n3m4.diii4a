/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "Engine/StdH.h"

#ifdef PLATFORM_WIN32
#include <tchar.h>
#endif

#include <Engine/Graphics/MultiMonitor.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/ErrorReporting.h>
#include <Engine/Base/Translation.h>

// Win9x multimonitor enabling/disabling code, 
// idea and original code courtesy of Christian Studer <cstuder@realtimesoft.com>
// added dynamic function loading and exception throwing

extern INDEX gfx_bDisableMultiMonSupport;
extern INDEX gfx_ctMonitors;
extern INDEX gfx_bMultiMonDisabled;

#ifdef PLATFORM_WIN32
#pragma comment(lib, "advapi32.lib")

typedef BOOL EnumDisplayDevices_t(
  PVOID Unused,       // not used; must be NULL
  DWORD iDevNum,      // specifies display device
  PDISPLAY_DEVICE lpDisplayDevice,  // pointer to structure to
                      // receive display device information
  DWORD dwFlags       // flags to condition function behavior
);

typedef LONG ChangeDisplaySettingsEx_t(
  LPCSTR lpszDeviceName,
  LPDEVMODE lpDevMode,  
  HWND     hwnd,
  DWORD dwflags,
  LPVOID lParam
);
 
static HINSTANCE _hUser32Lib = NULL;
static EnumDisplayDevices_t *_pEnumDisplayDevices = NULL;
static ChangeDisplaySettingsEx_t *_pChangeDisplaySettingsEx = NULL;
 
// disables or enables secondary monitors on Win98/Me
void Mon_DisableEnable9x_t(BOOL bDisable)
{
  // load user32
  if (_hUser32Lib==NULL) {
    _hUser32Lib = ::LoadLibraryA( "user32.dll");
    if( _hUser32Lib == NULL) {
      ThrowF_t(TRANS("Cannot load user32.dll."));
    }
  }

  if (_pEnumDisplayDevices==NULL) {
    _pEnumDisplayDevices = (EnumDisplayDevices_t*)GetProcAddress(_hUser32Lib, "EnumDisplayDevicesA");
    if (_pEnumDisplayDevices==NULL) {
      ThrowF_t(TRANS("Cannot find EnumDisplayDevices()."));
    }
  }

  if (_pChangeDisplaySettingsEx==NULL) {
    _pChangeDisplaySettingsEx = (ChangeDisplaySettingsEx_t*)GetProcAddress(_hUser32Lib, "ChangeDisplaySettingsExA");
    if (_pChangeDisplaySettingsEx==NULL) {
      ThrowF_t(TRANS("Cannot find ChangeDisplaySettingsEx()."));
    }
  }

	HKEY key;
	LONG ret = RegOpenKeyEx(HKEY_CURRENT_CONFIG, _T("Display\\Settings"), 0, KEY_ENUMERATE_SUB_KEYS |
		KEY_SET_VALUE, &key);
  if (ret != ERROR_SUCCESS) {
    ThrowF_t(TRANS("Cannot enumerate display settings from registry\n"));
  }

	TCHAR attach[2];
	if (bDisable)
		lstrcpy(attach, _T("0"));
	else
		lstrcpy(attach, _T("1"));

	// enumerate all subkeys. there is at least one subkey for each secondary monitor, maybe more.
	DWORD index = 0;
	while (ret == ERROR_SUCCESS)
	{
		TCHAR keyName[500];
		DWORD keyNameSize = 500;

		ret = RegEnumKeyEx(key, index, keyName, &keyNameSize, 0, 0, 0, 0);
		if (ret == ERROR_SUCCESS)
		{
			// open the subkey and set the AttachToDesktop value (0 or 1)
			HKEY subKey;
			ret = RegOpenKeyEx(key, keyName, 0, KEY_SET_VALUE, &subKey);
			if (ret == ERROR_SUCCESS)
			{
				ret = RegSetValueEx(subKey, _T("AttachToDesktop"), 0, REG_SZ,
					reinterpret_cast<const BYTE*>(attach), sizeof(TCHAR) * 2);
				RegCloseKey(subKey);
				++index;
			}
		}
	}

	RegCloseKey(key);

	bool failed = FALSE;
	DWORD dev = 0;
	DISPLAY_DEVICE dd;
	dd.cb = sizeof(dd);
	while (_pEnumDisplayDevices(0, dev, &dd, 0))
	{
		if (!(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) && !(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE))
		{
			// this is a secondary monitor, change display settings to what's currently stored in the registry
			ret = _pChangeDisplaySettingsEx((const char*)dd.DeviceName, 0, 0, 0, 0);
			// we get DISP_CHANGE_BADPARAM if we try to set settings for a monitor that doesn't exist
			if (ret != DISP_CHANGE_SUCCESSFUL && ret != DISP_CHANGE_BADPARAM)
				failed = TRUE;
		}

		dev++;
	}
  if (failed) {
    ThrowF_t(TRANS("Cannot change settings for at least one secondary monitor\n"));
  }
}
#endif

void MonitorsOff(void)
{
  extern BOOL _bDedicatedServer;
  if (_bDedicatedServer) {
    return;
  }

#ifdef PLATFORM_WIN32
  // check for WinNT or Win2k
  BOOL bNT = FALSE;
  OSVERSIONINFO osv;
  memset(&osv, 0, sizeof(osv));
  osv.dwOSVersionInfoSize = sizeof(osv);
  if (GetVersionEx(&osv) && osv.dwPlatformId==VER_PLATFORM_WIN32_NT) {
    bNT = TRUE;
  }

  // if there is more than one monitor, and OS is not WinNT
  if (gfx_ctMonitors>1 && !bNT) {
    CPrintF(TRANSV("Multimonitor configuration detected...\n"));
    // if multimon is not allowed
    if (gfx_bDisableMultiMonSupport) {
      CPrintF(TRANSV("  Multimonitor support disallowed.\n"));
      CPrintF(TRANSV("  Disabling multimonitor..."));
      // disable all but primary
      try {
        Mon_DisableEnable9x_t(/*bDisable = */ TRUE);
        CPrintF(TRANSV(" disabled\n"));
      } catch (const char *strError) {
        CPrintF(TRANSV(" error: %s\n"), strError);
      }
      gfx_bMultiMonDisabled = TRUE;
    // if multimon is allowed
    } else {
      CPrintF(TRANSV("  Multimonitor support was allowed.\n"));
    }
  }
#else
  CPrintF(TRANSV("Multimonitor is not supported on this platform.\n"));
#endif
}

void MonitorsOn(void)
{
#ifdef PLATFORM_WIN32
  // if multimon was disabled
  if (gfx_bMultiMonDisabled) {
    CPrintF(TRANSV("Multimonitor support was disabled.\n"));
    CPrintF(TRANSV("  re-enabling multimonitor..."));
    // enable all secondary
    try {
      Mon_DisableEnable9x_t(/*bDisable = */ FALSE);
      CPrintF(TRANSV(" enabled\n"));
    } catch (const char *strError) {
      CPrintF(TRANSV(" error: %s\n"), strError);
    }
  }
#else
  CPrintF(TRANSV("Multimonitor is not supported on this platform.\n"));
#endif
}

