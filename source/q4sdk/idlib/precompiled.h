
#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__

#ifdef __cplusplus

// RAVEN BEGIN
// nrausch: conditional cvar archive flag so that the pc build will archive certain cvars
#ifdef _XENON

#undef _WINDOWS
#define PC_CVAR_ARCHIVE CVAR_NOCHEAT //so it doesn't clobber
#else
#define PC_CVAR_ARCHIVE CVAR_ARCHIVE
#endif
// RAVEN END
class ThreadedAlloc;		// class that is only used to expand the AutoCrit template to tag allocs/frees called from inside the R_AddModelSurfaces call graph


//-----------------------------------------------------
// RAVEN BEGIN
// jscott: set up conditional compiles
#ifdef _DEBUG_MEMORY
#define ID_REDIRECT_NEWDELETE				// Doesn't work with Radiant
#define ID_DEBUG_MEMORY
#endif

#if defined( _FINAL ) && !defined( _MPBETA )
	#define ID_CONSOLE_LOCK
#endif

#ifdef _WINDOWS

	// _WIN32 always defined
	// _WIN64 also defined for x64 target
	#if !defined( _WIN64 )
		#define ID_WIN_X86_ASM
		#define ID_WIN_X86_MMX
		#define ID_WIN_X86_SSE
		//#define ID_WIN_X86_SSE2
	#endif

	// we should never rely on this define in our code. this is here so dodgy external libraries don't get confused
	#ifndef WIN32
		#define WIN32
	#endif

	#undef _XBOX
	#undef _CONSOLE								// Used to comment out code that can't be used on a console
	#define _OPENGL
	#define _LITTLE_ENDIAN
	#undef _CASE_SENSITIVE_FILESYSTEM
	#define _USE_OPENAL
	#define _USE_VOICECHAT
	#define __WITH_PB__
	//#define _RV_MEM_SYS_SUPPORT
	// when using the PC to make Xenon builds, enable _MD5R_SUPPORT / _MD5R_WRITE_SUPPORT and run with fs_game q4baseXenon
	#ifdef Q4SDK
		// the SDK can't be compiled with _MD5R_SUPPORT, but since the PC version is we need to maintain ABI
		// to make things worse, only the windows version was compiled with _MD5R enabled, the Linux and Mac builds didn't
		#define Q4SDK_MD5R
	#else	// Q4SDK
		#define _MD5R_SUPPORT
		#define _MD5R_WRITE_SUPPORT
	#endif	// !Q4SDK
	#define _GLVAS_SUPPPORT
	//#define RV_BINARYDECLS
	#define RV_SINGLE_DECL_FILE
	// this can't be used with _RV_MEM_SYS_SUPPORT and actually shouldn't be used at all on the Xenon at present
	#if !defined(_RV_MEM_SYS_SUPPORT) && !defined(ID_REDIRECT_NEWDELETE)
		#define RV_UNIFIED_ALLOCATOR
	#endif

	// SMP support for running the backend on a 2nd thread
	#define ENABLE_INTEL_SMP
	// Enables the batching of vertex cache request in SMP mode.
	// Note (TTimo): is tied to ENABLE_INTEL_SMP
	#define ENABLE_INTEL_VERTEXCACHE_OPT
	
	// Empty define for Xbox 360 compatibility
	#define RESTRICT
	#define TIME_THIS_SCOPE(x)

	#define NEWLINE				"\r\n"

	#pragma warning( disable : 4100 )			// unreferenced formal parameter
	#pragma warning( disable : 4127 )			// conditional expression is constant
	#pragma warning( disable : 4201 )			// non standard extension nameless struct or union
	#pragma warning( disable : 4244 )			// conversion to smaller type, possible loss of data
	#pragma warning( disable : 4245 )			// signed/unsigned mismatch
	#pragma warning( disable : 4389 )			// signed/unsigned mismatch
	#pragma warning( disable : 4714 )			// function marked as __forceinline not inlined
	#pragma warning( disable : 4800 )			// forcing value to bool 'true' or 'false' (performance warning)

	class AlignmentChecker
	{
	public:
		static void UpdateCount(void const * const ptr) {}
		static void ClearCount() {}
		static void Print() {}
	};

#endif // _WINDOWS

#ifdef __linux__

// for offsetof
#include <stddef.h>
// FLT_MAX and such
#include <limits.h>
#include <float.h>

	#define __WITH_PB__
	#undef WIN32
	#undef _XBOX
	#undef _CONSOLE
	#define _OPENGL
	#define _LITTLE_ENDIAN
	#define _CASE_SENSITIVE_FILESYSTEM

	#define NEWLINE				"\n"

	#define _GLVAS_SUPPPORT

	class AlignmentChecker
	{
	public:
		static void UpdateCount(void const * const ptr) {}
		static void ClearCount() {}
		static void Print() {}
	};

	#define RESTRICT
	#define TIME_THIS_SCOPE(x)

	// we release both a non-SMP and an SMP binary for Linux
	#ifdef ENABLE_INTEL_SMP
	// Enables the batching of vertex cache request in SMP mode.
	// Note (TTimo): is tied to ENABLE_INTEL_SMP
	#define ENABLE_INTEL_VERTEXCACHE_OPT
	#endif

#endif

#ifdef MACOS_X

// for offsetof
#include <stddef.h>

#include <ppc_intrinsics.h>		// for square root estimate instruction
#include <limits.h>
#include <float.h>				// for FLT_MIN

	// SMP support for running the backend on a 2nd thread
#ifndef ENABLE_INTEL_SMP
	#define ENABLE_INTEL_SMP
#endif
	// Enables the batching of vertex cache request in SMP mode.
	// Note (TTimo): is tied to ENABLE_INTEL_SMP
	#define ENABLE_INTEL_VERTEXCACHE_OPT

	#define __WITH_PB__
	#undef WIN32
	#undef _XBOX
	#undef _CONSOLE
	#define _OPENGL
#ifdef __ppc__
	#undef _LITTLE_ENDIAN
#else
	#define _LITTLE_ENDIAN
#endif
	#define _CASE_SENSITIVE_FILESYSTEM
	#define _USE_OPENAL
	#define ID_INLINE inline
	#define NEWLINE				"\n"

	#define _GLVAS_SUPPPORT

	class AlignmentChecker
	{
	public:
		static void UpdateCount(void const * const ptr) {}
		static void ClearCount() {}
		static void Print() {}
	};

	#define RESTRICT
	#define TIME_THIS_SCOPE(x)
#endif

#ifdef _WINDOWS

#ifndef Q4SDK

#if !defined( GAME_DLL ) && !defined( GAME_MONO )

#define _WIN32_WINNT		0x501
#define WINVER				0x501

#ifdef	ID_DEDICATED
// dedicated sets windows version here
#define	WIN32_LEAN_AND_MEAN
#else
#ifdef TOOL_DLL
// non-dedicated includes MFC and sets windows verion here
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion
#include "../tools/comafx/StdAfx.h"
#endif // TOOL_DLL
#endif // ID_DEDICATED

#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#define DIRECTINPUT_VERSION  0x0700
#define DIRECTSOUND_VERSION  0x0800

#include "../mssdk/include/dsound.h"
#include "../mssdk/include/dinput.h"
#include "../mssdk/include/dxerr8.h"

#endif // GAME_DLL
#endif // !Q4SDK

#include <malloc.h>							// no malloc.h on mac or unix
#include <windows.h>						// for qgl.h

// RAVEN BEGIN
// bdube: for dual monitor support in tools
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))
#endif
// RAVEN END

#undef FindText								// stupid namespace poluting Microsoft monkeys

#endif // _WINDOWS

//-----------------------------------------------------

#if !defined( _DEBUG ) && !defined( NDEBUG )
	// don't generate asserts
	#define NDEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>

//-----------------------------------------------------

// non-portable system services
#include "../sys/sys_public.h"

// id lib
#include "../idlib/Lib.h"

#if !defined( Q4SDK ) && defined( __WITH_PB__ )
	#include "../punkbuster/pbcommon.h"
#endif

// RAVEN BEGIN
// jsinger: added to allow support for serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
// jsinger: Serializable class support
#include "../serialization/Serializable.h"
#endif
// RAVEN END

// framework
#include "../framework/BuildVersion.h"
#include "../framework/BuildDefines.h"
#include "../framework/licensee.h"
#include "../framework/CmdSystem.h"
#include "../framework/CVarSystem.h"
#include "../framework/Common.h"
#include "../framework/File.h"
#include "../framework/FileSystem.h"
#include "../framework/UsercmdGen.h"

// decls
#include "../framework/declManager.h"
#include "../framework/declTable.h"
#include "../framework/declSkin.h"
#include "../framework/declEntityDef.h"
// RAVEN BEGIN
// jscott: not using
//#include "../framework/DeclFX.h"
//#include "../framework/DeclParticle.h"
// RAVEN END
#include "../framework/declAF.h"
#include "../framework/DeclPDA.h"
#include "../framework/DeclPlayerModel.h"
// RAVEN BEGIN
// jscott: new decl types
#include "../framework/declMatType.h"
#include "../framework/declLipSync.h"
#include "../framework/declPlayback.h"
// RAVEN END

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// Sanity check for any axis in bounds
const float MAX_BOUND_SIZE = 65536.0f;

// renderer
#include "../renderer/qgl.h"
#include "../renderer/Cinematic.h"
#include "../renderer/Material.h"
#include "../renderer/Model.h"
#include "../renderer/ModelManager.h"
#include "../renderer/RenderSystem.h"
#include "../renderer/RenderWorld.h"

// sound engine
#include "../sound/sound.h"

// RAVEN BEGIN
// jscott: Effects system interface
#include "../bse/BSEInterface.h"
// RAVEN END

// asynchronous networking
#include "../framework/async/NetworkSystem.h"

// user interfaces
#include "../ui/ListGUI.h"
#include "../ui/UserInterface.h"

// collision detection system
#include "../cm/CollisionModel.h"

// AAS files and manager
#include "../aas/AASFile.h"
#include "../aas/AASFileManager.h"

// game
#include "../game/Game.h"

//-----------------------------------------------------

#if defined( Q4SDK ) || defined( GAME_DLL ) || defined( GAME_MONO )

#ifdef GAME_MPAPI
#include "../mpgame/Game_local.h"
#else
#include "../game/Game_local.h"
#endif

#else

#include "../framework/DemoChecksum.h"

// framework
#include "../framework/Compressor.h"
#include "../framework/EventLoop.h"
#include "../framework/KeyInput.h"
#include "../framework/EditField.h"
#include "../framework/Console.h"
#include "../framework/DemoFile.h"
#include "../framework/Session.h"

// asynchronous networking
#include "../framework/async/AsyncNetwork.h"

// RAVEN BEGIN
#include "../tools/Tools.h"
// RAVEN END

#endif /* !GAME_DLL */

// RAVEN BEGIN
// jsinger: add AutoPtr and text-to-binary compiler support
#include "AutoPtr.h"
#include "LexerFactory.h"
#include "TextCompiler.h"
// jsinger: AutoCrit.h contains classes which aid in code synchronization
//          AutoAcquire.h contains a class that aids in thread acquisition of the direct3D device for xenon
//          Both compile out completely if the #define's above are not present
#include "threads/AutoCrit.h"
// RAVEN END

//-----------------------------------------------------

#endif	/* __cplusplus */

#endif /* !__PRECOMPILED_H__ */
