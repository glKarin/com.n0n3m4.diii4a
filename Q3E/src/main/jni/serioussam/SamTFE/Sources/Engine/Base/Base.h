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

#ifndef SE_INCL_ENGINE_BASE_BASE_H
#define SE_INCL_ENGINE_BASE_BASE_H
/*
 * rcg10042001 In case these don't get defined in the project file, try to
 *   catch them here...
 */
// be a little more discerning, using these macros will ensure that if someone
// wants to use MINGW then they can
#if (defined _WIN32) || (defined _WIN64) 
  #ifndef PLATFORM_WIN32
    #define PLATFORM_WIN32 1
  #endif

  #ifndef PRAGMA_ONCE
    #define PRAGMA_ONCE
  #endif

  // disable problematic warnings

  #pragma warning(disable: 4251)  // dll interfacing problems
  #pragma warning(disable: 4275)  // dll interfacing problems
  #pragma warning(disable: 4018)  // signed/unsigned mismatch
  #pragma warning(disable: 4244)  // type conversion warnings
  #pragma warning(disable: 4284)  // using -> for UDT
  #pragma warning(disable: 4355)  // 'this' : used in base member initializer list
  #pragma warning(disable: 4660)  // template-class specialization is already instantiated
  #pragma warning(disable: 4723)  // potential divide by 0

  // define engine api exporting declaration specifiers
  #ifdef ENGINE_EXPORTS
    #define ENGINE_API __declspec(dllexport)
  #else
    #define ENGINE_API __declspec(dllimport)

    #ifdef NDEBUG
      #pragma comment(lib, "Engine.lib")
    #else
      #pragma comment(lib, "EngineD.lib")
    #endif
  #endif

#elif (defined __linux__) 
  #if (defined __ANDROID__) || (defined __android__) 
#ifdef _DIII4A //karin: Android support
    #define PLATFORM_LINUX 1
#else
    #error "Android current isn't supported"
#endif
  #else
    #define PLATFORM_LINUX 1
  #endif
#elif (defined __APPLE__)
    #define PLATFORM_MACOSX 1
#elif (defined __FreeBSD__)
    #define PLATFORM_FREEBSD 1
#elif (defined __OpenBSD__) || (defined __NetBSD__)
    #define PLATFORM_OPENBSD 1
    #define PLATFORM_FREEBSD 1
#else
  #warning "UNKNOWN PLATFORM IDENTIFIED!!!!"
  #define PLATFORM_UNKNOWN 1
#endif

#if PLATFORM_LINUX || PLATFORM_MACOSX
  #ifndef PLATFORM_UNIX
    #define PLATFORM_UNIX 1
  #endif
#endif 

#ifdef PLATFORM_UNIX  /* rcg10042001 */
  #define ENGINE_API
#endif

#ifdef PLATFORM_PANDORA
# define INDEX_T unsigned short
# define INDEX_GL GL_UNSIGNED_SHORT
# define FASTMATH __attribute__((pcs("aapcs-vfp")))
#elif defined(__arm__)
# define INDEX_T unsigned short
# define INDEX_GL GL_UNSIGNED_SHORT
# define FASTMATH
#else
# define INDEX_T INDEX
# define INDEX_GL GL_UNSIGNED_INT
# define FASTMATH
#endif

#endif
