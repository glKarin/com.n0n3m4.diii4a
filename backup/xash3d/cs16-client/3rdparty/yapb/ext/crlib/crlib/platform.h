//
// crlib, simple class library for private needs.
// Copyright © RWSH Solutions LLC <lab@rwsh.ru>.
//
// SPDX-License-Identifier: MIT
//

#pragma once

CR_NAMESPACE_BEGIN

// detects the build platform
#if defined(__linux__)
#  define CR_LINUX
#elif defined(__APPLE__)
#  define CR_MACOS
#elif defined(_WIN32)
#  define CR_WINDOWS
#endif

#if defined(__EMSCRIPTEN__)
#  define CR_EMSCRIPTEN
#endif

#if defined(__ANDROID__)
#  define CR_ANDROID
#endif

#if defined(__vita__)
#  define CR_PSVITA
#endif

#if !defined(CR_DEBUG) && (defined(DEBUG) || defined(_DEBUG))
#  define CR_DEBUG
#endif

// detects the compiler
#if defined(_MSC_VER)
#  define CR_CXX_MSVC _MSC_VER
#endif

#if defined(__clang__)
#  define CR_CXX_CLANG __clang__
#endif

#if defined(__GNUC__)
#  define CR_CXX_GCC __GNUC__
#endif

// configure macroses
#define CR_C_LINKAGE extern "C"

#if defined(__x86_64) || defined(__x86_64__) || defined(__amd64__) || defined(__amd64) || defined(__aarch64__) || (defined(_MSC_VER) && defined(_M_X64)) || defined(__powerpc64__) || (__riscv_xlen == 64)
#  define CR_ARCH_X64
#elif defined(__i686) || defined(__i686__) || defined(__i386) || defined(__i386__) || defined(i386) || (defined(_MSC_VER) && defined(_M_IX86)) || defined(__powerpc__) || (__riscv_xlen == 32)
#  define CR_ARCH_X32
#endif

#if defined(__arm__)
#  define CR_ARCH_ARM32
#elif defined(__aarch64__)
#  define CR_ARCH_ARM64
#endif
#if defined(CR_ARCH_ARM32) || defined(CR_ARCH_ARM64)
#   define CR_ARCH_ARM
#endif

#if defined(__powerpc64__)
#  define CR_ARCH_PPC64
#elif defined(__powerpc__)
#  define CR_ARCH_PPC32
#endif

#if defined(__riscv)
#  define CR_ARCH_RISCV
#  define CR_NO_GLIBC_VERSIONING
#endif

#if defined(CR_ARCH_PPC32) || defined(CR_ARCH_PPC64)
#   define CR_ARCH_PPC
#endif

#if defined(CR_ARCH_ARM) || defined(CR_ARCH_PPC) || defined(CR_ARCH_RISCV)
#  define CR_ARCH_NON_X86
#endif

#if !defined(CR_DISABLE_SIMD)
#  if !defined(CR_ARCH_NON_X86)
#     define CR_HAS_SIMD_SSE
#  elif defined(__ARM_NEON)
#     define CR_HAS_SIMD_NEON
#  endif
#endif

#if defined(CR_CXX_MSVC)
#  define CR_FORCE_INLINE __forceinline
#else
#  define CR_FORCE_INLINE __attribute__((__always_inline__)) inline
#endif

#if defined(CR_HAS_SIMD_SSE) || defined(CR_HAS_SIMD_NEON)
#  define CR_HAS_SIMD
#endif

#if defined(CR_HAS_SIMD)
#  if defined(CR_CXX_MSVC)
#     define CR_SIMD_ALIGNED __declspec(align(16))
#  else
#     define CR_SIMD_ALIGNED __attribute__((aligned(16)))
#  endif
#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#  define CR_ARCH_CPU_BIG_ENDIAN
#endif

// define export macros
#if !defined(__GNUC__) || defined(CR_WINDOWS)
#  define CR_FORCE_STACK_ALIGN
#  define CR_EXPORT CR_C_LINKAGE __declspec(dllexport)
#  define CR_STDCALL __stdcall
#else
#  if defined(__i386__)
#     define CR_FORCE_STACK_ALIGN __attribute__((force_align_arg_pointer,noinline))
#  else
#     define CR_FORCE_STACK_ALIGN
#endif
#  define CR_EXPORT CR_C_LINKAGE CR_FORCE_STACK_ALIGN __attribute__((visibility("default"),used))
#  define CR_STDCALL
#endif

// msvc provides us placement new by default
#if defined(CR_CXX_MSVC)
#  define __PLACEMENT_NEW_INLINE
#endif

#if (defined(CR_CXX_MSVC) && !defined(CR_CXX_CLANG)) || defined(CR_ARCH_NON_X86)
#  define CR_SIMD_TARGET(dest) CR_FORCE_INLINE
#  define CR_SIMD_TARGET_AIL(dest) CR_FORCE_INLINE
#  define CR_SIMD_TARGET_TIL(dest) inline
#else
#  define CR_SIMD_TARGET(dest) __attribute__((target(dest)))
#  define CR_SIMD_TARGET_AIL(dest) __attribute__((__always_inline__,target(dest))) inline
#  define CR_SIMD_TARGET_TIL(dest) __attribute__((target(dest))) inline
#endif

// no symbol versioning in native builds
#if !defined(CR_NATIVE_BUILD) && !defined(CR_NO_GLIBC_VERSIONING)

// set the minimal glibc as we can
#if defined(CR_ARCH_ARM64) || defined(CR_ARCH_PPC64)
#  define GLIBC_VERSION_MIN "2.17"
#elif defined(CR_ARCH_ARM32)
#  define GLIBC_VERSION_MIN "2.4"
#elif defined(CR_ARCH_X64) && !defined(CR_ARCH_ARM)
#  define GLIBC_VERSION_MIN "2.2.5"
#else
#  define GLIBC_VERSION_MIN "2.0"
#endif

// avoid linking to high GLIBC versions
#if defined(CR_LINUX) && !defined(CR_ANDROID)
#  if defined(__GLIBC__)
   __asm__ (".symver powf, powf@GLIBC_" GLIBC_VERSION_MIN);
#     if __GLIBC__ >= 2 && __GLIBC_MINOR__ < 34
         __asm__ (".symver dlsym, dlsym@GLIBC_" GLIBC_VERSION_MIN);
         __asm__ (".symver dladdr, dladdr@GLIBC_" GLIBC_VERSION_MIN);
         __asm__ (".symver dlclose, dlclose@GLIBC_" GLIBC_VERSION_MIN);
         __asm__ (".symver dlopen, dlopen@GLIBC_" GLIBC_VERSION_MIN);
#     endif
#  endif
#endif
#endif

CR_NAMESPACE_END

#if defined(CR_WINDOWS)
constexpr auto kPathSeparator = "\\";
constexpr auto kLibrarySuffix = ".dll";

// raise windows api version if doesn't build for xp
#if !defined(CR_HAS_WINXP_SUPPORT) && !defined(CR_CXX_MSVC)
#  undef _WIN32_WINNT
#  define _WIN32_WINNT 0x0600
#  define WINVER 0x0600
#endif

#if !defined(NOMINMAX)
#  define NOMINMAX
#endif

#  define WIN32_LEAN_AND_MEAN
#  define NOGDICAPMASKS
#  define NOVIRTUALKEYCODES
#  define NOWINMESSAGES
#  define NOWINSTYLES
#  define NOSYSMETRICS
#  define NOMENUS
#  define NOICONS
#  define NOKEYSTATES
#  define NOSYSCOMMANDS
#  define NORASTEROPS
#  define NOSHOWWINDOW
#  define OEMRESOURCE
#  define NOATOM
#  define NOCLIPBOARD
#  define NOCOLOR
#  define NOCTLMGR
#  define NODRAWTEXT
#  define NOGDI
#  define NOKERNEL
#  define NONLS
#  define NOMEMMGR
#  define NOMETAFILE
#  define NOMSG
#  define NOOPENFILE
#  define NOSCROLL
#  define NOSERVICE
#  define NOSOUND
#  define NOTEXTMETRIC
#  define NOWH
#  define NOWINOFFSETS
#  define NOCOMM
#  define NOKANJI
#  define NOHELP
#  define NOPROFILER
#  define NODEFERWINDOWPOS
#  define NOMCX
#  define NOWINRES
#  define NOIME

#  include <windows.h>
#  include <direct.h>
#  include <io.h>
#else
constexpr auto kPathSeparator = "/";

#  if defined(CR_MACOS)
      constexpr auto kLibrarySuffix = ".dylib";
#  else
      constexpr auto kLibrarySuffix = ".so";
#endif

#  if defined(CR_PSVITA)
#     define FNM_CASEFOLD 0x10
#     include <sys/syslimits.h>
#  endif

#  include <unistd.h>
#  include <dirent.h>
#  include <fnmatch.h>
#  include <strings.h>
#  include <sys/time.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <locale.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>

#if defined(CR_ANDROID)
#  include <android/log.h>
#endif

#include <time.h>

CR_NAMESPACE_BEGIN

// undef bzero under android
#if defined(CR_ANDROID)
#  undef bzero
#endif

// helper struct for platform detection
struct Platform : public Singleton <Platform> {
   bool win = false;
   bool nix = false;
   bool macos = false;
   bool android = false;
   bool x64 = false;
   bool arm = false;
   bool ppc = false;
   bool simd = false;
   bool psvita = false;
   bool emscripten = false;
   bool riscv = false;

   char appName[64] = {};

   Platform () {
#if defined(CR_WINDOWS)
      win = true;
#endif

#if defined(CR_ANDROID)
      android = true;
#endif

#if defined(CR_LINUX)
      nix = true;
#endif

#if defined(CR_MACOS)
      macos = true;
#endif

#if defined(CR_ARCH_X64) || defined(CR_ARCH_ARM64)
      x64 = true;
#endif

#if defined(CR_ARCH_ARM)
      arm = true;
#endif

#if defined(CR_ARCH_RISCV)
      riscv = true;
#endif

#if defined(CR_PSVITA)
      psvita = true;
#endif

#if defined(CR_ARCH_PPC)
      ppc = true;
#endif

#if defined(CR_EMSCRIPTEN)
   	  emscripten = true;
#endif

#if !defined(CR_DISABLE_SIMD)
      simd = true;
#endif
   }

   // set the app name
   void setAppName (const char *name) {
      snprintf (appName, cr::bufsize (appName), "%s", name);
   }

   // running on no-x86 platform ?
   bool isNonX86 () const {
      return arm || ppc || riscv;
   }

   // helper platform-dependant functions
   template <typename U> bool isValidPtr (U *ptr) {
#if defined(CR_WINDOWS)
#if defined(CR_HAS_WINXP_SUPPORT)
      if (IsBadCodePtr (reinterpret_cast <FARPROC> (ptr))) {
         return false;
      }
#else
      MEMORY_BASIC_INFORMATION mbi {};

      if (VirtualQuery (reinterpret_cast <LPVOID> (ptr), &mbi, sizeof (mbi))) {
         auto result = !!(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY));

         if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) {
            result = false;
         }
         return result;
      }
#endif
#else
      (void) (ptr);
#endif
      return true;
   }

   bool createDirectory (const char *dir, int mode = -1) {
      int result = 1;
#if defined(CR_WINDOWS)
      result = _mkdir (dir);
      (void) mode;
#else
      result = mkdir (dir, mode == -1 ? (S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) : mode);
#endif
      return result == 0;
   }

   bool removeFile (const char *dir) {
#if defined(CR_WINDOWS)
      _unlink (dir);
#else
      unlink (dir);
#endif
      return true;
   }

   bool hasModule (const char *mod) {
#if defined(CR_WINDOWS)
      return GetModuleHandleA (mod) != nullptr;
#else
      (void) (mod);
      return true;
#endif
   }

   float seconds () {
#if defined(CR_WINDOWS)
      LARGE_INTEGER count {}, freq {};

      count.QuadPart = 0;
      freq.QuadPart = 0;

      QueryPerformanceFrequency (&freq);
      QueryPerformanceCounter (&count);

      return static_cast <float> (count.QuadPart / freq.QuadPart);
#else
      timeval tv;
      gettimeofday (&tv, nullptr);

      static auto startTime = tv.tv_sec;

      return static_cast <float> (tv.tv_sec - startTime);
#endif
   }

   void abort (const char *msg = "OUT OF MEMORY!") noexcept {
      fprintf (stderr, "%s\n", msg);

#if defined(CR_ANDROID)
      __android_log_write (ANDROID_LOG_ERROR, appName, msg);
#endif

#if defined(CR_WINDOWS)
      DestroyWindow (GetForegroundWindow ());
      MessageBoxA (GetActiveWindow (), msg, appName, MB_ICONSTOP);
#endif

#if defined(CR_DEBUG) && defined(CR_CXX_MSVC)
      DebugBreak ();
#else
      ::abort ();
#endif
   }

   // analogue of memset
   template <typename U> void bzero (U *ptr, size_t len) noexcept {
      memset (reinterpret_cast <void *> (ptr), 0, len);
   }

   void loctime (tm *_tm, const time_t *_time) {
#if defined(CR_WINDOWS)
      localtime_s (_tm, _time);
#else
      localtime_r (_time, _tm);
#endif
   }

   const char *env (const char *var) {
      static char result[384];
      bzero (result, cr::bufsize (result));

#if defined(CR_CXX_MSVC)
      char *buffer = nullptr;
      size_t size = 0;

      if (_dupenv_s (&buffer, &size, var) == 0 && buffer != nullptr) {
         strncpy_s (result, buffer, sizeof (result));
         free (buffer);
      }
#else
      auto data = getenv (var);

      if (data) {
         strncpy (result, data, cr::bufsize (result));
      }
#endif
      return result;
   }

   [[nodiscard]] const char *tmpfname () noexcept {
#if defined(CR_CXX_MSVC)
      static char name[L_tmpnam_s];
      tmpnam_s (name);

      if (name[0] == '\\') {
         for (auto i = 0; name[i] != '\0'; i++) {
            name[i] = name[i + 1];
         }
      }
#else
      char templ[PATH_MAX] { "/tmp/crtmp-XXXXXX" };
      static char name[PATH_MAX] {};

      strncpy (name, templ, cr::bufsize (name));
      mkstemp (name);
#endif
      return name;
   }

   int32_t hardwareConcurrency () {
#if defined(CR_WINDOWS)
      SYSTEM_INFO sysinfo;
      GetSystemInfo (&sysinfo);

      return sysinfo.dwNumberOfProcessors;
#else
      return sysconf (_SC_NPROCESSORS_ONLN);
#endif
   }

   bool fileExists (const char *path) {
#if defined(CR_WINDOWS)
      return _access (path, 0) == 0;
#else
      return access (path, F_OK) == 0;
#endif
   }

   FILE *openStdioFile (const char *path, const char *mode) {
      FILE *handle = nullptr;

#if defined(CR_CXX_MSVC)
      fopen_s (&handle, path, mode);
#else
      handle = fopen (path, mode);
#endif
      return handle;
   }
};

// expose platform singleton
CR_EXPOSE_GLOBAL_SINGLETON (Platform, plat);

CR_NAMESPACE_END
