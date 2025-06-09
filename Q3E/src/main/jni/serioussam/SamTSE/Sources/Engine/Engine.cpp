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

#include <Engine/Build.h>
#include <Engine/Base/Profiling.h>
#include <Engine/Base/Input.h>
#include <Engine/Base/Protection.h>
#include <Engine/Base/Console.h>
#include <Engine/Base/Console_internal.h>
#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Base/Shell.h>
#include <Engine/Base/CRC.h>
#include <Engine/Base/CRCTable.h>
#include <Engine/Base/ProgressHook.h>
#include <Engine/Sound/SoundListener.h>
#include <Engine/Sound/SoundLibrary.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/Font.h>
#include <Engine/Network/Network.h>
#include <Engine/Templates/DynamicContainer.cpp>
#include <Engine/Templates/Stock_CAnimData.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CSoundData.h>
#include <Engine/Templates/Stock_CModelData.h>
#include <Engine/Templates/Stock_CEntityClass.h>
#include <Engine/Templates/Stock_CMesh.h>
#include <Engine/Templates/Stock_CSkeleton.h>
#include <Engine/Templates/Stock_CAnimSet.h>
#include <Engine/Templates/Stock_CShader.h>
#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Base/IFeel.h>
#include <Engine/Base/FileSystem.h>

#if PLATFORM_UNIX
#include "SDL.h"
#include <limits.h>
#include <pwd.h>
#include <dirent.h>
#endif

// this version string can be referenced from outside the engine
ENGINE_API CTString _strEngineBuild  = "";
ENGINE_API ULONG _ulEngineBuildMajor = _SE_BUILD_MAJOR;
ENGINE_API ULONG _ulEngineBuildMinor = _SE_BUILD_MINOR;

ENGINE_API BOOL _bDedicatedServer = FALSE;
ENGINE_API BOOL _bWorldEditorApp  = FALSE;
ENGINE_API CTString _strLogFile = "";
ENGINE_API CTFileName _fnmModLibPath;

#ifdef PLATFORM_WIN32
extern HWND _hwndMain = NULL;
extern BOOL _bFullScreen = FALSE;
#else
// global handle for application windows
// !!! FIXME rcg10072001 this needs to be abstracted.
static HWND _hwndMain = NULL;
static BOOL _bFullScreen = FALSE;
#endif

CTCriticalSection zip_csLock; // critical section for access to zlib functions


// to keep system gamma table
#ifdef PLATFORM_WIN32 // DG: other platforms don't (currently?) use this
static UWORD auwSystemGamma[256*3];
#endif


// OS info
static CTString sys_strOS = "";
static INDEX sys_iOSMajor = 0;
static INDEX sys_iOSMinor = 0;
static INDEX sys_iOSBuild = 0;
static CTString sys_strOSMisc = "";

// CPU info
static CTString sys_strCPUVendor = "";
static INDEX sys_iCPUType = 0;
static INDEX sys_iCPUFamily = 0;
static INDEX sys_iCPUModel = 0;
static INDEX sys_iCPUStepping = 0;
static BOOL  sys_bCPUHasMMX = 0;
static BOOL  sys_bCPUHasCMOV = 0;
static INDEX sys_iCPUMHz = 0;
       INDEX sys_iCPUMisc = 0;

// RAM info
static INDEX sys_iRAMPhys = 0;
static INDEX sys_iRAMSwap = 0;

// HDD info
static INDEX sys_iHDDSize = 0;
static INDEX sys_iHDDFree = 0;
static INDEX sys_iHDDMisc = 0;

// MOD info
static CTString sys_strModName = "";
static CTString sys_strModExt  = "";

// Path vars
static INDEX sys_iGameBits = 0;
ENGINE_API INDEX sys_iSysPath = 0;

// Entities Adjesters
ENGINE_API FLOAT _fPlayerFOVAdjuster = 1.0f;
ENGINE_API FLOAT _fWeaponFOVAdjuster = 1.0f;
ENGINE_API FLOAT _fArmorHeightAdjuster = 1.5f;
ENGINE_API FLOAT _fFragScorerHeightAdjuster = 1.5f;

//
char _path[2048];
static int _testfiledone;

// enables paranoia checks for allocation array
__extern BOOL _bAllocationArrayParanoiaCheck = FALSE;

// rcg10072001
#ifdef PLATFORM_WIN32
BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
  switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
      break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    default:
      ASSERT(FALSE);
  }
  return TRUE;
}
#endif

#ifdef PLATFORM_UNIX
static void _list_dir(const char *name, int indent, CTString strGameID)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;

    if ( _testfiledone == 1 )
        return;

    while ((entry = readdir(dir)) != NULL) {
        if ( _testfiledone == 1 )
           break;
        if (entry->d_type == DT_DIR) {
            char path[2048];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            _list_dir(path, indent + 2, strGameID);
        } else {
           if( strGameID == "serioussamse") {
               int _testfile = strncmp((const char *)entry->d_name, (const char *) "SE1_00_Levels.gro", (size_t) 13 ); 
               if( _testfile == 0) {
                  memcpy(_path, name, 2047);
                  CPrintF("Found: %s/%s\n", name, entry->d_name);
                  _testfiledone = 1;
                  break;
               }
           } else {
               int _testfile = strncmp((const char *)entry->d_name, (const char *) "1_00_music.gro", (size_t) 10 ); 
               if( _testfile == 0) {
                  memcpy(_path, name, 2047);
                  CPrintF("Found: %s/%s\n", name, entry->d_name);
                  _testfiledone = 1;
                  break;
               }
           }
        }
    }
    closedir(dir);
}
#endif

static void DetectCPU(void)
{
  char strVendor[12+1] = { 0 };
  strVendor[12] = 0;
  ULONG ulTFMS = 0;
  ULONG ulFeatures = 0;

  #if (defined __MSVC_INLINE__) && (defined  PLATFORM_32BIT)
  // test MMX presence and update flag
  __asm {
    xor     eax,eax           ;// request for basic id
    cpuid
    mov     dword ptr [strVendor+0], ebx
    mov     dword ptr [strVendor+4], edx
    mov     dword ptr [strVendor+8], ecx
    mov     eax,1           ;// request for TFMS feature flags
    cpuid
    mov     dword ptr [ulTFMS], eax ;// remember type, family, model and stepping
    mov     dword ptr [ulFeatures], edx
  }

  #elif (defined _MSC_VER) && (defined  PLATFORM_64BIT)
    // test MMX presence and update flag

    // eax, ebx, ecx, edx
    int cpuidData[4];

    // 0: Highest Function Parameter and CPU's manufacturer ID string
    __cpuid(cpuidData, 0);
    // to get string copy 12 bytes in the following order:
    // ebx
    memcpy(&strVendor[0], &cpuidData[1], 4);
    // edx
    memcpy(&strVendor[0], &cpuidData[3], 4);
    // ecx
    memcpy(&strVendor[0], &cpuidData[2], 4);

    // 1: Processor Info and Feature Bits
    __cpuid(cpuidData, 1);
    // eax
    memcpy(&ulTFMS, &cpuidData[0], 4);
    // edx
    memcpy(&ulFeatures, &cpuidData[3], 4);
  
  #elif (defined __GNU_INLINE_X86__)	  
    ULONG eax, ebx, ecx, edx;
    // test MMX presence and update flag
    __asm__ __volatile__ (
    #if (defined __GNU_INLINE_X86_64__)
        "cpuid                    \n\t"
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
    #else
        "movl    %%ebx, %%esi     \n\t"
        "cpuid                    \n\t"
        "xchgl   %%ebx, %%esi     \n\t"
            : "=a" (eax), "=S" (ebx), "=c" (ecx), "=d" (edx)
    #endif
            : "a" (0) // request for basic id
    );
    memcpy(strVendor + 0, &ebx, 4);
    memcpy(strVendor + 4, &edx, 4);
    memcpy(strVendor + 8, &ecx, 4);

    __asm__ __volatile__ (
    #if (defined __GNU_INLINE_X86_64__)
        "cpuid                    \n\t"
            : "=a" (eax), "=b" (ebx), "=c" (ecx), "=d" (edx)
    #else
        "movl    %%ebx, %%esi     \n\t"
        "cpuid                    \n\t"
        "xchgl   %%ebx, %%esi     \n\t"
            : "=a" (eax), "=S" (ebx), "=c" (ecx), "=d" (edx)
    #endif
            : "a" (1) // request for TFMS feature flags
    );
    ulTFMS = eax;
    ulFeatures = edx;

  #endif

  if (ulTFMS == 0) {
    CPrintF(TRANSV("  (No CPU detection in this binary.)\n"));
    return;
  }

  INDEX iType     = (ulTFMS>>12)&0x3;
  INDEX iFamily   = (ulTFMS>> 8)&0xF;
  INDEX iModel    = (ulTFMS>> 4)&0xF;
  INDEX iStepping = (ulTFMS>> 0)&0xF;


  CPrintF(TRANSV("  Vendor: %s\n"), strVendor);
  CPrintF(TRANSV("  Type: %d, Family: %d, Model: %d, Stepping: %d\n"),
    iType, iFamily, iModel, iStepping);

  BOOL bMMX  = ulFeatures & (1<<23);
  BOOL bCMOV = ulFeatures & (1<<15);

  CTString strYes = TRANS("Yes");
  CTString strNo = TRANS("No");

  CPrintF(TRANSV("  MMX : %s\n"), (const char *) (bMMX ?strYes:strNo));
  CPrintF(TRANSV("  CMOV: %s\n"), (const char *) (bCMOV?strYes:strNo));
  CPrintF(TRANSV("  Clock: %.0fMHz\n"), _pTimer->tm_llCPUSpeedHZ/1E6);

  sys_strCPUVendor = strVendor;
  sys_iCPUType = iType;
  sys_iCPUFamily =  iFamily;
  sys_iCPUModel = iModel;
  sys_iCPUStepping = iStepping;
  sys_bCPUHasMMX = bMMX!=0;
  sys_bCPUHasCMOV = bCMOV!=0;
#ifdef PLATFORM_PANDORA
  sys_iCPUMHz = 400;    // conservative, ARM -> x86 cpu translation is not 1 to 1.
#elif defined(PLATFORM_PYRA)
  sys_iCPUMHz = 1000;
#else
  sys_iCPUMHz = INDEX(_pTimer->tm_llCPUSpeedHZ/1E6);
#endif
  if( !bMMX) FatalError( TRANS("MMX support required but not present!"));
}

static void DetectCPUWrapper(void)
{
#ifdef _MSC_VER  // rcg10072001
  __try {
    DetectCPU();
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    CPrintF( TRANS("Cannot detect CPU: exception raised.\n"));
  }
#else
    // We just have to punt and try this. The exception we're catching here
    //  is really a matter of whether the CPUID instruction is missing (on a
    //  pre Pentium system, which can't run this game anyhow) which will raise
    //  SIGILL on Unix platforms, or the CPU doesn't have MMX, in which case
    //  FatalError will end the process. USE_PORTABLE_C users will not have
    //  any exception at all. Have I rationalized this enough, yet?  :) --ryan.
    DetectCPU();
#endif
}

// reverses string
void StrRev( char *str) {
  char ctmp;
  char *pch0 = str;
  char *pch1 = str+strlen(str)-1;
  while( pch1>pch0) {
    ctmp  = *pch0;
    *pch0 = *pch1;
    *pch1 = ctmp;
    pch0++;
    pch1--;
  }
}

static char strExePath[MAX_PATH] = "";
static char strDirPath[MAX_PATH] = "";

static void AnalyzeApplicationPath(void)
{
#ifdef PLATFORM_WIN32	
	strcpy(strDirPath, "D:\\");
	strcpy(strExePath, "D:\\TestExe.xbe");
	char strTmpPath[MAX_PATH] = "";
	// get full path to the exe
	GetModuleFileNameA(NULL, strExePath, sizeof(strExePath) - 1);
	// copy that to the path
	strncpy(strTmpPath, strExePath, sizeof(strTmpPath) - 1);
	strDirPath[sizeof(strTmpPath) - 1] = 0;
	// remove name from application path
	StrRev(strTmpPath);
	// find last backslash
	char *pstr = strchr(strTmpPath, '\\');
	if (pstr == NULL) {
		// not found - path is just "\"
		strcpy(strTmpPath, "\\");
		pstr = strTmpPath;
	}
	// remove 'debug' from app path if needed
	if (strnicmp(pstr, "\\gubed", 6) == 0) pstr += 6;
	if (pstr[0] = '\\') pstr++;
	char *pstrFin = strchr(pstr, '\\');
	if (pstrFin == NULL) {
		strcpy(pstr, "\\");
		pstrFin = pstr;
	}
	// copy that to the path
	StrRev(pstrFin);
	strncpy(strDirPath, pstrFin, sizeof(strDirPath) - 1);
	strDirPath[sizeof(strDirPath) - 1] = 0;
#elif defined(_DIII4A) //karin: using same path
	_pFileSystem->GetExecutablePath(strExePath, sizeof(strExePath) - 1);
	strncpy(strDirPath, strExePath, sizeof(strDirPath));
#else	
	// rcg10072001 rewritten with abstraction layer.
	const char *_dirsep = CFileSystem::GetDirSeparator();
	size_t seplen = strlen(_dirsep);
	char *dirsep = new char[seplen + 1];
	strcpy(dirsep, _dirsep);
	StrRev(dirsep);

	char strTmpPath[MAX_PATH] = "";

	_pFileSystem->GetExecutablePath(strExePath, sizeof(strExePath) - 1);
	strncpy(strTmpPath, strExePath, sizeof(strTmpPath));
	strDirPath[sizeof(strTmpPath) - 1] = 0;
	// remove name from application path
	StrRev(strTmpPath);
	// find last backslash
	char *pstr = strstr(strTmpPath, dirsep);
	if (pstr == NULL) {
		// not found - path is just "\"
		strcpy(strTmpPath, dirsep);
		pstr = strTmpPath;
	}
	// remove 'debug' from app path if needed
	if (strnicmp(pstr, (CTString(dirsep) + "gubed"), 5 + seplen) == 0) pstr += (5 + seplen);
	if (strncmp(pstr, dirsep, seplen) == 0) pstr += seplen;
	char *pstrFin = strstr(pstr, dirsep);
	if (pstrFin == NULL) {
		strcpy(pstr, dirsep);
		pstrFin = pstr;
	}
	// copy that to the path
	StrRev(pstrFin);
	strncpy(strDirPath, pstrFin, sizeof(strDirPath) - 1);
	strDirPath[sizeof(strDirPath) - 1] = 0;
	delete[] dirsep;
#endif 
}

// rcg03242003
static void SanityCheckTypes(void)
{
    ASSERT(sizeof (SBYTE) == 1);
    ASSERT(sizeof (UBYTE) == 1);
    ASSERT(sizeof (UWORD) == 2);
    ASSERT(sizeof (SWORD) == 2);
    ASSERT(sizeof (ULONG) == 4);
    ASSERT(sizeof (SLONG) == 4);
    ASSERT(sizeof (INDEX) == 4);
    ASSERT(sizeof (BOOL) == 4);

    ULONG val = 0x02000001;
    UBYTE num = *((UBYTE *) &val);
    #if PLATFORM_BIGENDIAN
        #if PLATFORM_LITTLEENDIAN
            #error uh...what?
        #endif
        ASSERT(num == 0x02);
    #endif
    #if PLATFORM_LITTLEENDIAN
        #if PLATFORM_BIGENDIAN
            #error uh...what?
        #endif
        ASSERT(num == 0x01);
    #endif
}
// don't want to export this function
static void PlatformIdentification(void) 
{
// !!! FIXME: Abstract this somehow.
#if (defined PLATFORM_WIN32)
  OSVERSIONINFOA osv;
  memset(&osv, 0, sizeof(osv));
  osv.dwOSVersionInfoSize = sizeof(osv);
  if (GetVersionExA(&osv)) {
    switch (osv.dwPlatformId) {
    case VER_PLATFORM_WIN32s:         sys_strOS = "Win32s";  break;
    case VER_PLATFORM_WIN32_WINDOWS:  sys_strOS = "Win9x"; break;
    case VER_PLATFORM_WIN32_NT:       sys_strOS = "WinNT"; break;
    default: sys_strOS = "Unknown\n"; break;
    }

    sys_iOSMajor = osv.dwMajorVersion;
    sys_iOSMinor = osv.dwMinorVersion;
    sys_iOSBuild = osv.dwBuildNumber & 0xFFFF;
    sys_strOSMisc = osv.szCSDVersion;

    CPrintF(TRANSV("  Type: %s\n"), (const char*)sys_strOS);
    CPrintF(TRANSV("  Version: %d.%d, build %d\n"), 
      osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber & 0xFFFF);
    CPrintF(TRANSV("  Misc: %s\n"), osv.szCSDVersion);
  } else {
    CPrintF(TRANSV("Error getting OS info: %s\n"), GetWindowsError(GetLastError()) );
  }

#elif (defined PLATFORM_MACOSX)
    STUBBED("Use some Gestalt replacement, or whatever");
    #if 0
    long osver = 0x0000;
    OSErr err = Gestalt(gestaltSystemVersion, &osver);
    if (err != noErr)
        osver = 0x0000;

    sys_iOSMajor = ((osver & 0x0F00) >> 8) + (((osver & 0xF000) >> 12) * 10);
    sys_iOSMinor = ((osver & 0x00F0) >> 4);
    sys_iOSBuild = ((osver & 0x000F) >> 0);
    #else
    sys_iOSMajor = 10;  // !!! FIXME: just flatly false.
    sys_iOSMinor = 6;
    sys_iOSBuild = 0;
    #endif

    sys_strOS = "Mac OS X";
    sys_strOSMisc = "Mac OS";
    CPrintF(TRANSV("  Type: %s\n"), (const char*)sys_strOS);
    CPrintF(TRANSV("  Version: %d.%d.%d\n"),
                 (int)sys_iOSMajor, (int)sys_iOSMinor, (int)sys_iOSBuild);

#elif (defined PLATFORM_UNIX)  // !!! FIXME: rcg10082001 what to do with this?
	// FIXME: probably want to use uname function on Linux but it isn't totally applicable...hmm...
    sys_iOSMajor = 1;
    sys_iOSMinor = 0;
    sys_iOSBuild = 0;
    sys_strOS = "Unix";
    sys_strOSMisc = "Unix";
    CPrintF(TRANSV("  Type: %s\n"), (const char*)sys_strOS);

#else
   #error Do something with this for your platform.
#endif
}

static void SetupMemoryManager(void) 
{

#if (defined PLATFORM_WIN32)  // !!! FIXME: Abstract this somehow.
  MEMORYSTATUS ms;
  GlobalMemoryStatus(&ms);

  #define MB (1024*1024)
  sys_iRAMPhys = ms.dwTotalPhys    /MB;
  sys_iRAMSwap = ms.dwTotalPageFile/MB;

#elif (defined PLATFORM_UNIX)
  #ifdef PLATFORM_PANDORA
  sys_iRAMPhys = 256; // conservative here, there is 256MB models and 512MB...
  FILE* esrev = fopen("/etc/powervr-esrev", "r");
  if (esrev) {
    int rev = 0;
    fscanf(esrev,"%d", &rev);
    if (rev==3 || rev==5)
      sys_iRAMPhys = 512;
    fclose(esrev);
  };
  #else
  sys_iRAMPhys = 1;  // !!! FIXME: This is bad. Bad. BAD.
  #endif
  sys_iRAMSwap = 1;

#else
   #error Do something with this for your platform.
#endif
}

static void SetupSecondaryStorage(void) 
{
#if (defined PLATFORM_WIN32)  // !!! FIXME: Abstract this somehow.
  // get info on the first disk in system
  DWORD dwSerial;
  DWORD dwFreeClusters;
  DWORD dwClusters;
  DWORD dwSectors;
  DWORD dwBytes;

  char strDrive[] = "C:\\";
  strDrive[0] = strExePath[0];

  GetVolumeInformationA(strDrive, NULL, 0, &dwSerial, NULL, NULL, NULL, 0);
  GetDiskFreeSpaceA(strDrive, &dwSectors, &dwBytes, &dwFreeClusters, &dwClusters);
  sys_iHDDSize = ((__int64)dwSectors)*dwBytes*dwClusters/MB;
  sys_iHDDFree = ((__int64)dwSectors)*dwBytes*dwFreeClusters/MB;
  sys_iHDDMisc = dwSerial;

#elif (defined PLATFORM_UNIX)  // !!! FIXME: Uhh...?
  sys_iHDDSize = 1;
  sys_iHDDFree = 1;
  sys_iHDDMisc = 0xDEADBEEF;

#else
   #error Do something with this for your platform.
#endif
}

static void InitIFeel(void) 
{
// !!! FIXME : rcg12072001 Move this somewhere else.
#ifdef PLATFORM_WIN32
  // init IFeel
  HWND hwnd = NULL;//GetDesktopWindow();
  HINSTANCE hInstance = GetModuleHandle(NULL);
  if(IFeel_InitDevice(hInstance,hwnd))
  {
    CTString strDefaultProject = "Data\\Default.ifr";
    // get project file name for this device
    CTString strIFeel = IFeel_GetProjectFileName();
    // if no file name is returned use default file
    if(strIFeel.Length()==0) strIFeel = strDefaultProject;
    if(!IFeel_LoadFile(strIFeel))
    {
      if(strIFeel!=strDefaultProject)
      {
        IFeel_LoadFile(strDefaultProject);
      }
    }
    CPrintF("\n");
  }
#endif

}

static void InitSystemGammaSettings(void) 
{
// !!! FIXME: Move this into GfxLibrary...
#ifdef PLATFORM_WIN32
  // readout system gamma table
  HDC  hdc = GetDC(NULL);
  BOOL bOK = GetDeviceGammaRamp( hdc, &auwSystemGamma[0]);
  _pGfx->gl_ulFlags |= GLF_ADJUSTABLEGAMMA;
  if( !bOK) {
    _pGfx->gl_ulFlags &= ~GLF_ADJUSTABLEGAMMA;
    CPrintF( TRANS("\nWARNING: Gamma, brightness and contrast are not adjustable!\n\n"));
  } // done
  ReleaseDC( NULL, hdc);
#else
  // !!! FIXME : rcg01072002 This CAN be done with SDL, actually. Move this somewhere.
  #ifdef PLATFORM_PANDORA
  // hacked gamma support
  _pGfx->gl_ulFlags |= GLF_ADJUSTABLEGAMMA;
  #else
  CPrintF( TRANS("\nWARNING: Gamma, brightness and contrast are not adjustable!\n\n"));
  #endif
#endif

}

// System specific platform init functions
static void PlatformSpecificInit(void) 
{
  #if PLATFORM_UNIX
  extern SDL_EventType WM_SYSKEYDOWN;
  extern SDL_EventType WM_LBUTTONDOWN;
  extern SDL_EventType WM_LBUTTONUP;
  extern SDL_EventType WM_RBUTTONDOWN;
  extern SDL_EventType WM_RBUTTONUP;
  extern SDL_EventType WM_PAINT;
#ifdef _DIII4A // must setup although without SDL
  int uet = SDL_USEREVENT + 1;
#define SDL_RegisterEvents(x) (uet++)
#endif
  WM_SYSKEYDOWN = (SDL_EventType) SDL_RegisterEvents(1);
  WM_LBUTTONDOWN = (SDL_EventType) SDL_RegisterEvents(1);
  WM_LBUTTONUP = (SDL_EventType) SDL_RegisterEvents(1);
  WM_RBUTTONDOWN = (SDL_EventType) SDL_RegisterEvents(1);
  WM_RBUTTONUP = (SDL_EventType) SDL_RegisterEvents(1);
  WM_PAINT = (SDL_EventType) SDL_RegisterEvents(1);
#ifdef _DIII4A // must setup although without SDL
#undef SDL_RegisterEvents
#endif
  #endif
}

// startup engine 
#ifdef PLATFORM_UNIX
ENGINE_API void SE_InitEngine(const char *argv0, CTString strGameID)
#else
ENGINE_API void SE_InitEngine(CTString strGameID)
#endif
{
  SanityCheckTypes();

  PlatformSpecificInit();

  const char *gamename = "UnknownGame";
  if (strGameID != "")
    gamename = (const char *) strGameID;

#ifdef PLATFORM_UNIX
  _pFileSystem = CFileSystem::GetInstance(argv0, gamename);  // rcg10082001
#else
  _pFileSystem = CFileSystem::GetInstance(" ", gamename);
#endif

  //#pragma message(">> Remove this from SE_InitEngine : _bWorldEditorApp")
  if(strGameID=="SeriousEditor") {
    _bWorldEditorApp = TRUE;
  }

  AnalyzeApplicationPath();
  _fnmApplicationPath = CTString(strDirPath);
  _fnmApplicationExe = CTString(strExePath);
#ifdef PLATFORM_UNIX
    // rcg01012002 calculate user dir.
  char buf[MAX_PATH];
  _pFileSystem->GetUserDirectory(buf, sizeof (buf));
  _fnmUserDir = CTString(buf);
#endif
  try {
    _fnmApplicationExe.RemoveApplicationPath_t();
  } catch (const char *strError) {
    (void) strError;
    ASSERT(FALSE);
  }

  _pConsole = new CConsole;
  if (_strLogFile=="") {/*
    _strLogFile = CTFileName(CTString(strExePath)).FileName();
    // chop off end of Unix executable filename... --ryan.
    _strLogFile.ReplaceSubstr(CTString("-bin"), CTString(""));*/
#ifdef PLATFORM_UNIX
    _strLogFile = "SeriousSam";
#else
    _strLogFile = CTFileName(CTString(strExePath)).FileName();
#endif
  }
#ifdef PLATFORM_UNIX
  if (_bPortableVersion == FALSE) {
    _pConsole->Initialize(_fnmUserDir+_strLogFile+".log", 90, 512);
  } else {
    _pConsole->Initialize(_fnmApplicationPath + _strLogFile + ".log", 90, 512);
  }
#else
  _pConsole->Initialize(_fnmApplicationPath + _strLogFile + ".log", 90, 512);
#endif
  _pAnimStock        = new CStock_CAnimData;
  _pTextureStock     = new CStock_CTextureData;
  _pSoundStock       = new CStock_CSoundData;
  _pModelStock       = new CStock_CModelData;
  _pEntityClassStock = new CStock_CEntityClass;
  _pMeshStock        = new CStock_CMesh;
  _pSkeletonStock    = new CStock_CSkeleton;
  _pAnimSetStock     = new CStock_CAnimSet;
  _pShaderStock      = new CStock_CShader;

  // rcg11232001 I moved this here so I can register platform-specific cvars.
  // init main shell
  _pShell = new CShell;
  _pShell->Initialize();

  _pTimer = new CTimer;
  _pGfx   = new CGfxLibrary;
  _pSound = new CSoundLibrary;
  _pInput = new CInput;
  _pNetwork = new CNetworkLibrary;

  CRCT_Init();

  _strEngineBuild.PrintF( TRANS("SeriousEngine Build: %d.%d"), _SE_BUILD_MAJOR, _SE_BUILD_MINOR);

  // print basic engine info
  CPrintF(TRANSV("--- Serious Engine Startup ---\n"));
  CPrintF("  %s\n\n", (const char *) _strEngineBuild);

  // Path vars
  sys_iGameBits  = (int)(CHAR_BIT * sizeof(void *));
  CPrintF(TRANSV("Running %d-bit version\n"), sys_iGameBits);

#ifdef PLATFORM_UNIX
#if defined(__OpenBSD__) || defined(__FreeBSD__)
  int _isystempath = strncmp((const char *)strExePath, (const char *) "/usr/local/bin/", (size_t) 15 );
#elif defined(__NetBSD__)
  int _isystempath = strncmp((const char *)strExePath, (const char *) "/usr/pkg/bin/", (size_t) 13 );
#else
  int _isystempath = strncmp((const char *)strExePath, (const char *) "/usr/bin/", (size_t) 9 );
#endif
  if( _isystempath == 0 ) {
       sys_iSysPath = 1; // using system path
  } else {
       sys_iSysPath = 0; // using standarted path
  }

  // get library path for mods
  _fnmModLibPath = "";
#if defined(__OpenBSD__) || defined(__FreeBSD__)
  if( sys_iSysPath == 1 ) {
    _fnmModLibPath = "/usr/local/lib/" + strGameID + "/";
#elif defined(__NetBSD__)
  if( sys_iSysPath == 1 ) {
    _fnmModLibPath = "/usr/pkg/lib/" + strGameID + "/";
#else
  if( sys_iSysPath == 1 && sys_iGameBits == 64 && _pFileSystem->IsDirectory((const char *) "/usr/lib/aarch64-linux-gnu/" + strGameID)) {
    _fnmModLibPath = "/usr/lib/aarch64-linux-gnu/" + strGameID + "/"; 
  } else if( sys_iSysPath == 1 && sys_iGameBits == 32 && _pFileSystem->IsDirectory((const char *) "/usr/lib/arm-linux-gnueabihf/" + strGameID)) {
    _fnmModLibPath = "/usr/lib/arm-linux-gnueabihf/" + strGameID + "/";
  } else if( sys_iSysPath == 1 && sys_iGameBits == 64 && _pFileSystem->IsDirectory((const char *) "/usr/lib/riscv64-linux-gnu/" + strGameID)) {
    _fnmModLibPath = "/usr/lib/riscv64-linux-gnu/" + strGameID + "/";
  } else if( sys_iSysPath == 1 && sys_iGameBits == 64 && _pFileSystem->IsDirectory((const char *) "/usr/lib/s390x-linux-gnu/" + strGameID)) {
    _fnmModLibPath = "/usr/lib/s390x-linux-gnu/" + strGameID + "/"; 
  } else if( sys_iSysPath == 1 && sys_iGameBits == 64 && _pFileSystem->IsDirectory((const char *) "/usr/lib/powerpc64-linux-gnu/" + strGameID)) {
    _fnmModLibPath = "/usr/lib/powerpc64-linux-gnu/" + strGameID + "/"; 
  } else if( sys_iSysPath == 1 && sys_iGameBits == 64 && _pFileSystem->IsDirectory((const char *) "/usr/lib/x86_64-linux-gnu/" + strGameID)) {
    _fnmModLibPath = "/usr/lib/x86_64-linux-gnu/" + strGameID + "/";
  } else if( sys_iSysPath == 1 && sys_iGameBits == 32 && _pFileSystem->IsDirectory((const char *) "/usr/lib/i386-linux-gnu/" + strGameID)) {
    _fnmModLibPath = "/usr/lib/i386-linux-gnu/" + strGameID + "/";
  } else if( sys_iSysPath == 1 && sys_iGameBits == 64 && _pFileSystem->IsDirectory((const char *) "/usr/lib64/" + strGameID)) {
    _fnmModLibPath = "/usr/lib64/" + strGameID + "/";
  } else if( sys_iSysPath == 1 && sys_iGameBits == 32 && _pFileSystem->IsDirectory((const char *) "/usr/lib//" + strGameID)) {
    _fnmModLibPath = "/usr/lib/" + strGameID + "/";
#endif
  } else if( sys_iSysPath == 0 ) {
    _fnmModLibPath = _fnmApplicationPath;
  } else {
    CPrintF(TRANSV("ERROR: Game libraries not ound!\n"));
    FatalError(TRANSV("Failed to search game libraries installed!\nPlease reinstall the game.\nSee the log for more details.\nGame log is here: ~/.local/share/Serious-Engine/%s/SeriousSam.log"),(const char *) strGameID);
  }

  if( sys_iSysPath == 1 ) { // search game data
    CTFileName _fnm_usr_TestFile, _fnm_local_TestFile, _fnm_home_TestFile;
    CTString _fnmUserDataPath, _fnm_TestFile;
    int _fd;

    if( access((const char *) _fnmUserDir+_strLogFile+".cfg", F_OK) == 0 ) {
      _fd = open((const char *) _fnmUserDir+_strLogFile+".cfg", O_RDONLY,S_IRUSR);
      size_t x = read(_fd, _path, 2048);
      _fnmUserDataPath = (CTString)_path + "/";
      close(_fd);
      CPrintF(TRANSV("Testing home path: %s\n"), (const char *) _fnmUserDataPath);
    } else { 
      _fnmUserDataPath = "";
    }

    // set testing files
    if( strGameID == "serioussamse") {
      _fnm_TestFile = "SE1_00_Levels.gro";
    } else {
      _fnm_TestFile = "1_00_music.gro";
    }
    CPrintF(TRANSV("Testing file: %s\n"), (const char *) _fnm_TestFile);
#if defined(__OpenBSD__) || defined(__FreeBSD__)
    _fnm_usr_TestFile = "/usr/local/share/" + strGameID + "/" + _fnm_TestFile; //  data in usr
#elif defined(__NetBSD__)
    _fnm_usr_TestFile = "/usr/pkg/share/" + strGameID + "/" + _fnm_TestFile; //  data in usr
#else
    _fnm_usr_TestFile = "/usr/share/" + strGameID + "/" + _fnm_TestFile; //  data in usr
#endif
    _fnm_local_TestFile = _fnmUserDir + _fnm_TestFile;                   //  data in home .local
    _fnm_home_TestFile = (const char *)_fnmUserDataPath  + _fnm_TestFile;
    // test
    if( access((const char *) _fnm_home_TestFile, F_OK) != 0 ) {
      CPrintF(TRANSV("Testing system path: %s\n"), (const char *) _fnm_usr_TestFile);
      CPrintF(TRANSV("Testing local  path: %s\n"), (const char *) _fnm_local_TestFile);
      if( access((const char *) _fnm_usr_TestFile, F_OK) == 0 ) {
#if defined(__OpenBSD__) || defined(__FreeBSD__)
        _fnmApplicationPath = "/usr/local/share/" + strGameID + "/";                       // all game data
#elif defined(__NetBSD__)
        _fnmApplicationPath = "/usr/pkg/share/" + strGameID + "/";                       // all game data
#else
        _fnmApplicationPath = "/usr/share/" + strGameID + "/";                       // all game data
#endif
        CPrintF(TRANSV("Found usr path: %s\n"), (const char *) _fnmUserDataPath);
      } else if( access((const char *) _fnm_local_TestFile, F_OK) == 0 ) {
        _fnmApplicationPath = _fnmUserDir;                                           // all game data
        CPrintF(TRANSV("Found local path: %s\n"), (const char *) _fnmUserDataPath);
      } else {
        // search in home dir 
        // BOOL YesNoMessage(const char *strFormat, ...)
#if defined(__OpenBSD__) || defined(__FreeBSD__)
        InfoMessage(TRANS("No game files were found in /usr/local/share/%s/\n or %s\nThe home directory will be searched."),(const char *) strGameID,(const char *) _fnmUserDir);
#elif defined(__NetBSD__)
        InfoMessage(TRANS("No game files were found in /usr/pkg/share/%s/\n or %s\nThe home directory will be searched."),(const char *) strGameID,(const char *) _fnmUserDir);
#else
        InfoMessage(TRANS("No game files were found in /usr/share/%s/\n or %s\nThe home directory will be searched."),(const char *) strGameID,(const char *) _fnmUserDir);
#endif
        struct passwd *pw = getpwuid(getuid());
        const char *_homedir = pw->pw_dir;
        _testfiledone = 0;
        _list_dir(_homedir, 0, strGameID);
        CTString _PATH;
        _PATH = (CTString)_path;
        _fnmApplicationPath = (CTFileName) _PATH + "/";
        _fnm_home_TestFile  = (CTFileName) _PATH + "/" + _fnm_TestFile; 
        if( access((const char *) _fnm_home_TestFile, F_OK) == 0 ) {
          //_fnmApplicationPath = (CTFileName) _PATH + "/";
          CPrintF(TRANSV("Found home path: %s\n"), (const char *) _fnm_home_TestFile);
          _fd = open((const char *) _fnmUserDir+_strLogFile+".cfg", O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
          size_t x = write(_fd, _path, 2048);
          close(_fd);
        } else {
          CPrintF(TRANSV("ERROR: Game data not ound!\n"));
          _fnmUserDataPath = "";
#if defined(__OpenBSD__) || defined(__FreeBSD__)
          FatalError(TRANSV("Failed to search game data!\nPlease put the game data in the paths:\n/usr/local/share/%s/\n or %s\n or somewhere in your home directory\nSee the log for more details.\nGame log is here: ~/.local/share/Serious-Engine/%s/SeriousSam.log"),(const char *) strGameID,(const char *) _fnmUserDir,(const char *) strGameID);
#elif defined(__NetBSD__)
          FatalError(TRANSV("Failed to search game data!\nPlease put the game data in the paths:\n/usr/pkg/share/%s/\n or %s\n or somewhere in your home directory\nSee the log for more details.\nGame log is here: ~/.local/share/Serious-Engine/%s/SeriousSam.log"),(const char *) strGameID,(const char *) _fnmUserDir,(const char *) strGameID);
#else
          FatalError(TRANSV("Failed to search game data!\nPlease put the game data in the paths:\n/usr/share/%s/\n or %s\n or somewhere in your home directory\nSee the log for more details.\nGame log is here: ~/.local/share/Serious-Engine/%s/SeriousSam.log"),(const char *) strGameID,(const char *) _fnmUserDir,(const char *) strGameID);
#endif
        }
      } 
    } else {
      CPrintF(TRANSV("Found path: %s\n"), (const char *) _fnmUserDataPath);
      _fnmApplicationPath = _fnmUserDataPath;                                         // all game data
    }
  }

  // print info on the started application
  CPrintF(TRANSV("Executable: %s\n"), (const char *) strExePath);
  CPrintF(TRANSV("Assumed engine data directory: %s\n"), (const char *) _fnmApplicationPath);
  CPrintF(TRANSV("Assumed mods library directory: %s\n"), (const char *) _fnmModLibPath);
#endif

  CPrintF("\n");

  // report os info
  CPrintF(TRANSV("Examining underlying OS...\n"));
  
  PlatformIdentification();

  CPrintF("\n");

  // (rcg11232001 this is where _pShell was originally created.)

  // report CPU
  CPrintF(TRANSV("Detecting CPU...\n"));
  DetectCPUWrapper();
  CPrintF("\n");

  // report memory info
  extern void ReportGlobalMemoryStatus(void);
  ReportGlobalMemoryStatus();

  SetupMemoryManager();
  // initialize zip semaphore
  zip_csLock.cs_iIndex = -1;  // not checked for locking order


// rcg10082001 Honestly, all of this is meaningless in a multitasking OS.
//  That includes Windows, too.
  SetupSecondaryStorage(); /// FIXME: does that name make sense
 
  // add console variables
  extern INDEX con_bNoWarnings;
  extern INDEX wld_bFastObjectOptimization;
  extern INDEX fil_bPreferZips;
  extern FLOAT mth_fCSGEpsilon;
  _pShell->DeclareSymbol("user INDEX con_bNoWarnings;", (void *) &con_bNoWarnings);
  _pShell->DeclareSymbol("user INDEX wld_bFastObjectOptimization;", (void *) &wld_bFastObjectOptimization);
  _pShell->DeclareSymbol("user FLOAT mth_fCSGEpsilon;", (void *) &mth_fCSGEpsilon);
  _pShell->DeclareSymbol("persistent user INDEX fil_bPreferZips;", (void *) &fil_bPreferZips);
  // OS info
  _pShell->DeclareSymbol("user const CTString sys_strOS    ;", (void *) &sys_strOS);
  _pShell->DeclareSymbol("user const INDEX sys_iOSMajor    ;", (void *) &sys_iOSMajor);
  _pShell->DeclareSymbol("user const INDEX sys_iOSMinor    ;", (void *) &sys_iOSMinor);
  _pShell->DeclareSymbol("user const INDEX sys_iOSBuild    ;", (void *) &sys_iOSBuild);
  _pShell->DeclareSymbol("user const CTString sys_strOSMisc;", (void *) &sys_strOSMisc);
  // CPU info
  _pShell->DeclareSymbol("user const CTString sys_strCPUVendor;", (void *) &sys_strCPUVendor);
  _pShell->DeclareSymbol("user const INDEX sys_iCPUType       ;", (void *) &sys_iCPUType    );
  _pShell->DeclareSymbol("user const INDEX sys_iCPUFamily     ;", (void *) &sys_iCPUFamily  );
  _pShell->DeclareSymbol("user const INDEX sys_iCPUModel      ;", (void *) &sys_iCPUModel   );
  _pShell->DeclareSymbol("user const INDEX sys_iCPUStepping   ;", (void *) &sys_iCPUStepping);
  _pShell->DeclareSymbol("user const INDEX sys_bCPUHasMMX     ;", (void *) &sys_bCPUHasMMX  );
  _pShell->DeclareSymbol("user const INDEX sys_bCPUHasCMOV    ;", (void *) &sys_bCPUHasCMOV );
  _pShell->DeclareSymbol("user const INDEX sys_iCPUMHz        ;", (void *) &sys_iCPUMHz     );
  _pShell->DeclareSymbol("     const INDEX sys_iCPUMisc       ;", (void *) &sys_iCPUMisc    );
  // RAM info
  _pShell->DeclareSymbol("user const INDEX sys_iRAMPhys;", (void *) &sys_iRAMPhys);
  _pShell->DeclareSymbol("user const INDEX sys_iRAMSwap;", (void *) &sys_iRAMSwap);
  _pShell->DeclareSymbol("user const INDEX sys_iHDDSize;", (void *) &sys_iHDDSize);
  _pShell->DeclareSymbol("user const INDEX sys_iHDDFree;", (void *) &sys_iHDDFree);
  _pShell->DeclareSymbol("     const INDEX sys_iHDDMisc;", (void *) &sys_iHDDMisc);
  // MOD info
  _pShell->DeclareSymbol("user const CTString sys_strModName;", (void *) &sys_strModName);
  _pShell->DeclareSymbol("user const CTString sys_strModExt;",  (void *) &sys_strModExt);
  // Path vars
  _pShell->DeclareSymbol("user const INDEX sys_iGameBits    ;", (void *) &sys_iGameBits);
  _pShell->DeclareSymbol("user const INDEX sys_iSysPath    ;", (void *) &sys_iSysPath);
#ifdef PLATFORM_WIN32
  _pShell->DeclareSymbol("user const FLOAT _fPlayerFOVAdjuster    ;", (void *) &_fPlayerFOVAdjuster);
  _pShell->DeclareSymbol("user const FLOAT _fWeaponFOVAdjuster    ;", (void *) &_fWeaponFOVAdjuster);
  _pShell->DeclareSymbol("user const FLOAT _fArmorHeightAdjuster    ;", (void *) &_fArmorHeightAdjuster);
  _pShell->DeclareSymbol("user const FLOAT _fFragScorerHeightAdjuster    ;", (void *) &_fFragScorerHeightAdjuster);
#endif
  // Stock clearing
  extern void FreeUnusedStock(void);
  _pShell->DeclareSymbol("user void FreeUnusedStock(void);", (void *) &FreeUnusedStock);
  
  // Timer tick quantum
  _pShell->DeclareSymbol("user const FLOAT fTickQuantum;", (FLOAT*)&_pTimer->TickQuantum);

  // init MODs and stuff ...
  extern void InitStreams(void);
  InitStreams();
  // keep mod name in sys cvar
  sys_strModName = _strModName;
  sys_strModExt  = _strModExt;

// checking of crc
#if 0
  ULONG ulCRCActual = -2;
  SLONG ulCRCExpected = -1;
  try {
    // get the checksum of engine
    #ifndef NDEBUG
      #define SELFFILE "Bin\\Debug\\EngineD.dll"
      #define SELFCRCFILE "Bin\\Debug\\EngineD.crc"
    #else
      #define SELFFILE "Bin\\Engine.dll"
      #define SELFCRCFILE "Bin\\Engine.crc"
    #endif
    ulCRCActual = GetFileCRC32_t(CTString(SELFFILE));
    // load expected checksum from the file on disk
    ulCRCExpected = 0;
    LoadIntVar(CTString(SELFCRCFILE), ulCRCExpected);
  } catch (const char *strError) {
    CPrintF("%s\n", strError);
  }
  // if not same
  if (ulCRCActual!=ulCRCExpected) {
    // don't run
    //FatalError(TRANS("Engine CRC is invalid.\nExpected %08x, but found %08x.\n"), ulCRCExpected, ulCRCActual);
  }
#endif

  _pInput->Initialize();

  _pGfx->Init();
  _pSound->Init();

  if (strGameID!="") {
    _pNetwork->Init(strGameID);
    // just make classes declare their shell variables
    try {
      CEntityClass* pec = _pEntityClassStock->Obtain_t(CTString("Classes\\Player.ecl"));
      ASSERT(pec != NULL);
      _pEntityClassStock->Release(pec);  // this must not be a dependency!
    // if couldn't find player class
    } catch (const char *strError) {
      FatalError(TRANS("Cannot initialize classes:\n%s"), strError);
    }
  } else {
    _pNetwork = NULL;
  }

  // mark that default fonts aren't loaded (yet)
  _pfdDisplayFont = NULL;
  _pfdConsoleFont = NULL;

  InitSystemGammaSettings();
  InitIFeel(); // on non win32 platforms this will be optimized out if we play our cards right
}



static void PlatformSpecificDeinit(void)
{
// !!! FIXME: Move this into GfxLibrary...
#ifdef PLATFORM_WIN32
  // restore system gamma table (if needed)
  if( _pGfx->gl_ulFlags&GLF_ADJUSTABLEGAMMA) {
    HDC  hdc = GetDC(NULL);
    BOOL bOK = SetDeviceGammaRamp( hdc, &auwSystemGamma[0]);
    //ASSERT(bOK);
    ReleaseDC( NULL, hdc);
  }
#elif defined(PLATFORM_PANDORA)
  // restore default gamma
  system("sudo /usr/pandora/scripts/op_gamma.sh 0");
#endif
}

// shutdown entire engine
ENGINE_API void SE_EndEngine(void)
{
  PlatformSpecificDeinit();

  // free stocks
  delete _pEntityClassStock;  _pEntityClassStock = NULL;
  delete _pModelStock;        _pModelStock       = NULL; 
  delete _pSoundStock;        _pSoundStock       = NULL; 
  delete _pTextureStock;      _pTextureStock     = NULL; 
  delete _pAnimStock;         _pAnimStock        = NULL; 
  delete _pMeshStock;         _pMeshStock        = NULL; 
  delete _pSkeletonStock;     _pSkeletonStock    = NULL; 
  delete _pAnimSetStock;      _pAnimSetStock     = NULL; 
  delete _pShaderStock;       _pShaderStock      = NULL; 

  // free all memory used by the crc cache
  CRCT_Clear();

  // shutdown
  if( _pNetwork != NULL) { delete _pNetwork;  _pNetwork=NULL; }
  delete _pInput;    _pInput   = NULL;  
  delete _pSound;    _pSound   = NULL;  
  delete _pGfx;      _pGfx     = NULL;    
  delete _pTimer;    _pTimer   = NULL;  
  delete _pShell;    _pShell   = NULL;  
  delete _pConsole;  _pConsole = NULL;
#ifdef PLATFORM_UNIX
  delete _pFileSystem;  _pFileSystem = NULL;
#endif
  extern void EndStreams(void);
  EndStreams();

  // shutdown profilers
#ifdef PLATFORM_WIN32
  _sfStats.Clear();
  _pfGfxProfile.pf_apcCounters.Clear();
  _pfGfxProfile.pf_aptTimers.Clear();
  _pfModelProfile.pf_apcCounters.Clear();
  _pfModelProfile.pf_aptTimers.Clear();
  _pfSoundProfile.pf_apcCounters.Clear();
  _pfSoundProfile.pf_aptTimers.Clear();
  _pfNetworkProfile.pf_apcCounters.Clear();
  _pfNetworkProfile.pf_aptTimers.Clear();
  _pfRenderProfile.pf_apcCounters.Clear();
  _pfRenderProfile.pf_aptTimers.Clear();
  _pfWorldEditingProfile.pf_apcCounters.Clear();
  _pfWorldEditingProfile.pf_aptTimers.Clear();
  _pfPhysicsProfile.pf_apcCounters.Clear();
  _pfPhysicsProfile.pf_aptTimers.Clear();
#else
  _sfStats.Clear();
  _pfGfxProfile           .CountersClear();
  _pfGfxProfile           .TimersClear();
  _pfModelProfile         .CountersClear();
  _pfModelProfile         .TimersClear();
  _pfSoundProfile         .CountersClear();
  _pfSoundProfile         .TimersClear();
  _pfNetworkProfile       .CountersClear();
  _pfNetworkProfile       .TimersClear();
  _pfRenderProfile        .CountersClear();
  _pfRenderProfile        .TimersClear();
  _pfWorldEditingProfile  .CountersClear();
  _pfWorldEditingProfile  .TimersClear();
  _pfPhysicsProfile       .CountersClear();
  _pfPhysicsProfile       .TimersClear();
#endif

  // remove default fonts if needed
  if( _pfdDisplayFont != NULL) { delete _pfdDisplayFont;  _pfdDisplayFont=NULL; }
  if( _pfdConsoleFont != NULL) { delete _pfdConsoleFont;  _pfdConsoleFont=NULL; } 

  // deinit IFeel
  IFeel_DeleteDevice();
}


// prepare and load some default fonts
ENGINE_API void SE_LoadDefaultFonts(void)
{
  _pfdDisplayFont = new CFontData;
  _pfdConsoleFont = new CFontData;

  // try to load default fonts
  try {
    _pfdDisplayFont->Load_t( CTFILENAME( "Fonts\\Display3-narrow.fnt"));
    _pfdConsoleFont->Load_t( CTFILENAME( "Fonts\\Console1.fnt"));
  }
  catch (const char *strError) {
    FatalError( TRANS("Error loading font: %s."), strError);
  }
  // change fonts' default spacing a bit
  _pfdDisplayFont->SetCharSpacing( 0);
  _pfdDisplayFont->SetLineSpacing(+1);
  _pfdConsoleFont->SetCharSpacing(-1);
  _pfdConsoleFont->SetLineSpacing(+1);
  _pfdConsoleFont->SetFixedWidth();
}


// updates main windows' handles for windowed mode and fullscreen
ENGINE_API void SE_UpdateWindowHandle( HWND hwndMain)
{
  ASSERT( hwndMain!=NULL);
  _hwndMain = hwndMain;
#ifdef _DIII4A //karin: always fullscreen
  _bFullScreen = TRUE;
#else
  _bFullScreen = _pGfx!=NULL && (_pGfx->gl_ulFlags&GLF_FULLSCREEN);
#endif
}

#ifdef PLATFORM_WIN32
static BOOL TouchBlock(UBYTE *pubMemoryBlock, INDEX ctBlockSize)
{
#if (defined __MSC_VER)
  // cannot pretouch block that are smaller than 64KB :(
  ctBlockSize -= 16*0x1000;
  if( ctBlockSize<4) return FALSE; 

  __try {
    // 4 times should be just enough
    for( INDEX i=0; i<4; i++) {
      // must do it in asm - don't know what VC will try to optimize
      __asm {
        // The 16-page skip is to keep Win 95 from thinking we're trying to page ourselves in
        // (we are doing that, of course, but there's no reason we shouldn't) - THANX JOHN! :)
        mov   esi,dword ptr [pubMemoryBlock]
        mov   ecx,dword ptr [ctBlockSize]
        shr   ecx,2
touchLoop:
        mov   eax,dword ptr [esi]
        mov   ebx,dword ptr [esi+16*0x1000]
        add   eax,ebx     // BLA, BLA, TROOCH, TRUCH
        add   esi,4
        dec   ecx
        jnz   touchLoop
      }
    }
  }
  __except(EXCEPTION_EXECUTE_HANDLER) { 
    return FALSE;
  }

#else

  // !!! FIXME: How necessary is this on a system with a good memory manager?
  // !!! More importantly, will this help if the system is paging to disk
  // !!! like mad anyhow? Leaving this as a no-op for most systems seems safe
  // !!! to me.  --ryan.
  // DG: put this into #ifdef PLATFORM_WIN32 because otherwise the function is not called anyway

#endif

  return TRUE;
}
#endif // PLATFORM_WIN32

// pretouch all memory commited by process
__extern BOOL _bNeedPretouch = FALSE;
ENGINE_API extern void SE_PretouchIfNeeded(void)
{
#if (defined PLATFORM_WIN32)
  // only if pretouching is needed?
  extern INDEX gam_bPretouch;
  if( !_bNeedPretouch || !gam_bPretouch) return;
  _bNeedPretouch = FALSE;

  // set progress bar
  SetProgressDescription( TRANS("pretouching"));
  CallProgressHook_t(0.0f);

  // need to do this two times - 1st for numerations, and 2nd for real (progress bar and that shit)
  BOOL bPretouched = TRUE;
  INDEX ctFails, ctBytes, ctBlocks;
  INDEX ctPassBytes, ctTotalBlocks;
  for( INDEX iPass=1; iPass<=2; iPass++)
  { 
    // flush variables
    ctFails=0; ctBytes=0; ctBlocks=0; ctTotalBlocks=0;
    void *pvNextBlock = NULL;
    MEMORY_BASIC_INFORMATION mbi;
    // lets walk thru memory blocks
    while( VirtualQuery( pvNextBlock, &mbi, sizeof(mbi)))
    { 
      // don't mess with kernel's memory and zero-sized blocks    
      if( ((ULONG)pvNextBlock)>0x7FFF0000UL || mbi.RegionSize<1) break;

      // if this region of memory belongs to our process
      BOOL bCanAccess = (mbi.Protect==PAGE_READWRITE); // || (mbi.Protect==PAGE_EXECUTE_READWRITE);
      if( mbi.State==MEM_COMMIT && bCanAccess && mbi.Type==MEM_PRIVATE) // && !IsBadReadPtr( mbi.BaseAddress, 1)
      { 
        // increase counters
        ctBlocks++;
        ctBytes += mbi.RegionSize;
        // in first pass we only count
        if( iPass==1) goto nextRegion;
        // update progress bar
        CallProgressHook_t( (FLOAT)ctBytes/ctPassBytes);
        // pretouch
        ASSERT( mbi.RegionSize>0);
        BOOL bOK = TouchBlock((UBYTE *)mbi.BaseAddress, mbi.RegionSize);
        if( !bOK) { 
          // whoops!
          ctFails++;
        }
        // for easier debugging (didn't help much, though)
        //Sleep(5);  
      }
nextRegion:
      // advance to next region
      pvNextBlock = ((UBYTE*)mbi.BaseAddress) + mbi.RegionSize;
      ctTotalBlocks++;
    }
    // done with one pass
    ctPassBytes = ctBytes;
    if( (ctPassBytes/1024/1024)>sys_iRAMPhys) {
      // not enough RAM, sorry :(
      bPretouched = FALSE;
      break;
    }
  }

  // report
  if( bPretouched) {
    // success
    CPrintF( TRANS("Pretouched %d KB of memory in %d blocks.\n"), ctBytes/1024, ctBlocks); //, ctTotalBlocks);
  } else {
    // fail
    CPrintF( TRANS("Cannot pretouch due to lack of physical memory (%d KB of overflow).\n"), ctPassBytes/1024-sys_iRAMPhys*1024);
  }
  // some blocks failed?
  if( ctFails>1) CPrintF( TRANS("(%d blocks were skipped)\n"), ctFails);
  //_pShell->Execute("StockDump();");

#else

  // See dissertation in TouchBlock().  --ryan.

  _bNeedPretouch = FALSE;

#endif
}




#if 0

      // printout block info
      CPrintF("--------\n");
      CTString strTmp1, strTmp2;
      CPrintF("Base/Alloc Address: 0x%8X / 0x%8X - Size: %d KB\n", mbi.BaseAddress, mbi.AllocationBase, mbi.RegionSize/1024);
      switch( mbi.Protect) {
      case PAGE_READONLY:          strTmp1 = "PAGE_READONLY";          break;
      case PAGE_READWRITE:         strTmp1 = "PAGE_READWRITE";         break;
      case PAGE_WRITECOPY:         strTmp1 = "PAGE_WRITECOPY";         break;
      case PAGE_EXECUTE:           strTmp1 = "PAGE_EXECUTE";           break;
      case PAGE_EXECUTE_READ:      strTmp1 = "PAGE_EXECUTE_READ";      break;
      case PAGE_EXECUTE_READWRITE: strTmp1 = "PAGE_EXECUTE_READWRITE"; break;
      case PAGE_GUARD:             strTmp1 = "PAGE_GUARD";             break;
      case PAGE_NOACCESS:          strTmp1 = "PAGE_NOACCESS";          break;
      case PAGE_NOCACHE:           strTmp1 = "PAGE_NOCACHE";           break;
      }
      switch( mbi.AllocationProtect) {
      case PAGE_READONLY:          strTmp2 = "PAGE_READONLY";          break;
      case PAGE_READWRITE:         strTmp2 = "PAGE_READWRITE";         break;
      case PAGE_WRITECOPY:         strTmp2 = "PAGE_WRITECOPY";         break;
      case PAGE_EXECUTE:           strTmp2 = "PAGE_EXECUTE";           break;
      case PAGE_EXECUTE_READ:      strTmp2 = "PAGE_EXECUTE_READ";      break;
      case PAGE_EXECUTE_READWRITE: strTmp2 = "PAGE_EXECUTE_READWRITE"; break;
      case PAGE_GUARD:             strTmp2 = "PAGE_GUARD";             break;
      case PAGE_NOACCESS:          strTmp2 = "PAGE_NOACCESS";          break;
      case PAGE_NOCACHE:           strTmp2 = "PAGE_NOCACHE";           break;
      }
      CPrintF("Current/Alloc protect: %s / %s\n", strTmp1, strTmp2);
      switch( mbi.State) {
      case MEM_COMMIT:  strTmp1 = "MEM_COMMIT";  break;
      case MEM_FREE:    strTmp1 = "MEM_FREE";    break;
      case MEM_RESERVE: strTmp1 = "MEM_RESERVE"; break;
      }
      switch( mbi.Type) {
      case MEM_IMAGE:   strTmp2 = "MEM_IMAGE";   break;
      case MEM_MAPPED:  strTmp2 = "MEM_MAPPED";  break;
      case MEM_PRIVATE: strTmp2 = "MEM_PRIVATE"; break;
      }
      CPrintF("State/Type: %s / %s\n", strTmp1, strTmp2);

#endif
