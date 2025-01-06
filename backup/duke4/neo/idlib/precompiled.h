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

#ifdef ID_X64
	#undef _USE_32BIT_TIME_T
#endif

//-----------------------------------------------------

#define ID_TIME_T time_t

#ifdef _WIN32

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// prevent auto literal to string conversion

#ifndef _D3SDK
#ifndef GAME_DLL

#define WINVER				0x501

#if _MFC_VER
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

#include "../engine/tools/comafx/StdAfx.h"

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
#endif /* !_D3SDK */

#pragma warning(disable : 4100)				// unreferenced formal parameter
#pragma warning(disable : 4244)				// conversion to smaller type, possible loss of data
#pragma warning(disable : 4714)				// function marked as __forceinline not inlined
#pragma warning(disable : 4996)				// unsafe string operations

#include <malloc.h>							// no malloc.h on mac or unix
#include <windows.h>						// for gl.h
#undef FindText								// stupid namespace poluting Microsoft monkeys

#endif /* _WIN32 */

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
#include "../engine/sys/sys_public.h"
#include "../engine/sys/sys_types.h"
#include "../engine/sys/sys_assert.h"

// id lib
#include "../idlib/Lib.h"

// Threads
#include "../engine/sys/sys_threading.h"
#include "../engine/threads/Thread.h"
#include "../engine/threads/ParallelJobList.h"

// framework
#include "../engine/framework/BuildDefines.h"
#include "../engine/framework/Licensee.h"
#include "../engine/framework/CmdSystem.h"
#include "../engine/framework/CVarSystem.h"
#include "../engine/framework/Common.h"
#include "../engine/framework/File.h"
#include "../engine/framework/FileSystem.h"
#include "../engine/framework/UsercmdGen.h"

// Navigation
#include "../engine/navigation/Nav_public.h"

// decls
#include "../engine/decls/DeclManager.h"
#include "../engine/decls/DeclTable.h"
#include "../engine/decls/DeclSkin.h"
#include "../engine/decls/DeclEntityDef.h"
#include "../engine/decls/DeclFX.h"
#include "../engine/decls/DeclParticle.h"
#include "../engine/decls/DeclAF.h"
#include "../engine/decls/DeclPDA.h"
// jmarshall
#include "../engine/decls/DeclRenderParam.h"
// jmarshall end

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
#include "../engine/renderer/qgl.h"
#include "../engine/renderer/Cinematic.h"
#include "../engine/renderer/Material.h"
#include "../engine/models/Model.h"
#include "../engine/models/ModelManager.h"
#include "../engine/renderer/RenderSystem.h"
#include "../engine/models/RenderWorld.h"
#include "../engine/renderer/Font.h"
#include "../engine/renderer/DeviceContext.h"

// sound engine
#include "../engine/sound/sound.h"

// asynchronous networking
#include "../engine/framework/async/NetworkSystem.h"

// user interfaces
#include "../engine/ui/ListGUI.h"
#include "../engine/ui/UserInterface.h"

// collision detection system
#include "../engine/cm/CollisionModel.h"

// game
#include "../dukegame/Gamelib/Game.h"

// Externals that need to be available across all the engine.
#ifndef _D3SDK
#include "../engine/external/imgui/imgui.h"
#include "../engine/external/imgui/examples/imgui_impl_opengl3.h"
#include "../engine/external/imgui/examples/imgui_impl_win32.h"
#endif

#include "../engine/framework/DemoChecksum.h"

// framework
#include "../engine/framework/Compressor.h"
#include "../engine/framework/EventLoop.h"
#include "../engine/framework/KeyInput.h"
#include "../engine/framework/EditField.h"
#include "../engine/framework/Console.h"
#include "../engine/framework/DemoFile.h"
#include "../engine/framework/Session.h"

//-----------------------------------------------------

#ifndef _D3SDK

#ifdef GAME_DLL

#include "../dukegame/Gamelib/Game_local.h"

#else

// asynchronous networking
#include "../engine/framework/async/AsyncNetwork.h"

// The editor entry points are always declared, but may just be
// stubbed out on non-windows platforms.
#include "../engine/tools/edit_public.h"
// jmarshall
#include "../engine/tools/edit_guis.h"
// jmarshall end

// Compilers for map, model, video etc. processing.
#include "../engine/tools/compilers/compiler_public.h"

#endif /* !GAME_DLL */

#endif /* !_D3SDK */

//-----------------------------------------------------

#endif	/* __cplusplus */

#endif /* !__PRECOMPILED_H__ */
