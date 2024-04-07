/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__

#ifdef __cplusplus

//-----------------------------------------------------

#if defined(_K_DEV)
#define LOGI(fmt, args...) { common->Printf(fmt, ##args); common->Printf("\n"); }
#define LOGW(fmt, args...) common->Warning(fmt, ##args);
#define LOGE(fmt, args...) common->Error(fmt, ##args);
#endif

#define ID_TIME_T time_t
#ifdef _RAVEN
typedef unsigned char		byte;		// 8 bits

#ifndef BIT
#define BIT( num )				BITT< num >::VALUE
#endif

#ifndef Q4BIT
#define Q4BIT( num )				( 1 << ( num ) )
#endif

template< unsigned int B >
class BITT {
public:
	typedef enum bitValue_e {
		VALUE = 1 << B,
	} bitValue_t;
};
#endif

#ifdef _WIN32

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion

#ifndef GAME_DLL

#ifndef WINVER
#define WINVER				0x501
#endif

#if 0
// Dedicated server hits unresolved when trying to link this way now. Likely because of the 2010/Win7 transition? - TTimo

#ifdef	ID_DEDICATED
// dedicated sets windows version here
#define	_WIN32_WINNT WINVER
#define	WIN32_LEAN_AND_MEAN
#else
// non-dedicated includes MFC and sets windows version here
#include "../tools/comafx/StdAfx.h"			// this will go away when MFC goes away
#endif

#else


#ifdef ID_ALLOW_TOOLS
#include "../tools/comafx/StdAfx.h"
#endif

#endif

#include <winsock2.h>
#include <mmsystem.h>
#include <mmreg.h>

#define DIRECTINPUT_VERSION  0x0800			// was 0x0700 with the old mssdk
#define DIRECTSOUND_VERSION  0x0800

#include <dsound.h>
#include <dinput.h>

#endif /* !GAME_DLL */

#pragma warning(disable : 4100)				// unreferenced formal parameter
#pragma warning(disable : 4244)				// conversion to smaller type, possible loss of data
#pragma warning(disable : 4714)				// function marked as __forceinline not inlined
#pragma warning(disable : 4996)				// unsafe string operations

#include <malloc.h>							// no malloc.h on mac or unix
#include <windows.h>						// for qgl.h
#undef FindText								// stupid namespace poluting Microsoft monkeys

#endif /* _WIN32 */

//-----------------------------------------------------

#if !defined( _DEBUG ) && !defined( NDEBUG )
// don't generate asserts
#define NDEBUG
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <ctype.h>
#include <typeinfo>
#include <errno.h>
#include <math.h>

#include <inttypes.h>

#ifdef _HUMANHEAD
#ifndef TRUE
#define TRUE true
#endif
#ifndef FALSE
#define FALSE false
#endif
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif

typedef intptr_t INT_PTR;
#if !defined(_MSC_VER)
typedef unsigned int DWORD;
typedef bool BOOL;
#endif
#endif

#define round_up(x, y)	(((x) + ((y)-1)) & ~((y)-1))

//-----------------------------------------------------

// non-portable system services
#include "../sys/sys_public.h"

// id lib
#include "../idlib/Lib.h"
#ifdef _RAVEN // raven.h
#include "../raven/idlib/containers/Pair.h"
#include "../raven/idlib/math/Interpolate.h"
#include "../raven/idlib/rvMemSys.h"
// RAVEN BEGIN
// jsinger: add AutoPtr and text-to-binary compiler support
#include "../raven/idlib/AutoPtr.h"
#include "../raven/idlib/LexerFactory.h"
// jsinger: AutoCrit.h contains classes which aid in code synchronization
//          AutoAcquire.h contains a class that aids in thread acquisition of the direct3D device for xenon
//          Both compile out completely if the #define's above are not present
#include "../raven/idlib/threads/AutoCrit.h"
// RAVEN END

class ThreadedAlloc;		// class that is only used to expand the AutoCrit template to tag allocs/frees called from inside the R_AddModelSurfaces call graph
#endif

// framework
#include "../framework/BuildVersion.h"
#include "../framework/BuildDefines.h"
#include "../framework/Licensee.h"
#include "../framework/CmdSystem.h"
#include "../framework/CVarSystem.h"
#include "../framework/Common.h"
#include "../framework/File.h"
#include "../framework/FileSystem.h"
#include "../framework/UsercmdGen.h"

// decls
#include "../framework/DeclManager.h"
#include "../framework/DeclTable.h"
#include "../framework/DeclSkin.h"
#include "../framework/DeclEntityDef.h"
#include "../framework/DeclFX.h"
#include "../framework/DeclParticle.h"
#include "../framework/DeclAF.h"
#include "../framework/DeclPDA.h"
#ifdef _HUMANHEAD
#include "../humanhead/framework/declPreyBeam.h" // HUMANHEAD CJR
#endif

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

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

// asynchronous networking
#include "../framework/async/NetworkSystem.h"

// user interfaces
#include "../ui/ListGUI.h"
#include "../ui/UserInterface.h"

// collision detection system
#include "../cm/CollisionModel.h"

// AAS files and manager
#include "../tools/compilers/aas/AASFile.h"
#include "../tools/compilers/aas/AASFileManager.h"

#ifdef _RAVEN // raven_engine.h
#include "../raven/idlib/TextCompiler.h"
#include "../raven/idlib/math/Radians.h"
// RAVEN BEGIN
// jscott: Effects system interface
#include "../raven/bse/BSEInterface.h"
// RAVEN END

#include "../raven/framework/DeclPlayerModel.h"
// RAVEN BEGIN
// jscott: new decl types
#include "../raven/framework/declLipSync.h"
#include "../raven/framework/declMatType.h"
#include "../raven/framework/declPlayback.h"
// RAVEN END

// Sanity check for any axis in bounds
const float MAX_BOUND_SIZE = 65536.0f;
#endif

// game
#if defined(_D3XP)

    #if defined(_D3LE)
        #include "../mod/doom3/d3le/Game.h"
    #elif defined(_SABOT)
        #include "../mod/doom3/sabot/Game.h"
    #elif defined(_FRAGGINGFREE)
        #include "../framework/Game.h"
    #else
        #include "../d3xp/Game.h"
    #endif

#elif defined(_RAVEN)

    #include "../quake4/Game.h"

#elif defined(_HUMANHEAD)

    #include "../prey/Game.h"

#else

    #if defined(_CDOOM)
        #include "../mod/doom3/cdoom/Game.h"
    #elif defined(_RIVENSIN)
        #include "../game/Game.h"
    #elif defined(_HARDCORPS)
        #include "../game/Game.h"
    #elif defined(_OVERTHINKED)
        #include "../mod/doom3/overthinked/Game.h"
    #elif defined(_HEXENEOC)
        #include "../framework/Game.h"
    #elif defined(_LIBRECOOP)
        #include "../game/Game.h"
    #else
        #include "../game/Game.h"
    #endif

#endif

//-----------------------------------------------------

#ifdef GAME_DLL

#if defined(_D3XP)

    #if defined(_D3LE)
        #include "../mod/doom3/d3le/Game_local.h"
    #elif defined(_SABOT)
        #include "../mod/doom3/sabot/Game_local.h"
    #elif defined(_FRAGGINGFREE)
        #include "../mod/doom3/fraggingfree/Game_local.h"
    #else
        #include "../d3xp/Game_local.h"
    #endif

#elif defined(_RAVEN)

    #include "../quake4/Game_local.h"

#elif defined(_HUMANHEAD)

    #include "../prey/Game_local.h"

#else

    #if defined(_CDOOM)
        #include "../mod/doom3/cdoom/Game_local.h"
    #elif defined(_RIVENSIN)
        #include "../mod/doom3/rivensin/Game_local.h"
    #elif defined(_HARDCORPS)
        #include "../mod/doom3/hardcorps/Game_local.h"
    #elif defined(_OVERTHINKED)
        #include "../mod/doom3/overthinked/Game_local.h"
    #elif defined(_HEXENEOC)
        #include "../mod/doom3/hexeneoc/Game_local.h"
    #elif defined(_LIBRECOOP)
        #include "../mod/doom3/librecoop/Game_local.h"
    #else
        #include "../game/Game_local.h"
    #endif

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

// The editor entry points are always declared, but may just be
// stubbed out on non-windows platforms.
#include "../tools/edit_public.h"

// Compilers for map, model, video etc. processing.
#include "../tools/compilers/compiler_public.h"

#endif /* !GAME_DLL */

//-----------------------------------------------------

#endif	/* __cplusplus */

#endif /* !__PRECOMPILED_H__ */
