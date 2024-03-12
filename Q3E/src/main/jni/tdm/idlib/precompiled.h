/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __PRECOMPILED_H__
#define __PRECOMPILED_H__

#include "sys/sys_defines.h"
#include "sys/sys_includes.h"
#include "sys/sys_assert.h"
#include "sys/sys_types.h"
#include "sys/sys_threading.h"

//-----------------------------------------------------

#undef min
#undef max
#if !defined(_NO_TRACY) // __ANDROID__
#include <Tracy.hpp>
#endif

#define ID_TIME_T time_t

//-----------------------------------------------------

// non-portable system services
#include "../sys/sys_public.h"

//stgatilov: make sure build defines take effect on idlib headers included from Lib.h
#include "../framework/BuildDefines.h"

// id lib
#include "../idlib/Lib.h"

// framework
#include "../framework/Licensee.h"
#include "../framework/CmdSystem.h"
#include "../framework/CVarSystem.h"
#include "../framework/Common.h"
#include "../framework/I18N.h"
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
#ifndef ID_TYPEINFO
#include "../framework/Tracing.h"
#endif

// We have expression parsing and evaluation code in multiple places:
// materials, sound shaders, and guis. We should unify them.
const int MAX_EXPRESSION_OPS = 4096;
const int MAX_EXPRESSION_REGISTERS = 4096;

// renderer
#include "renderer/backend/qgl/qgl.h"
#include "renderer/resources/Cinematic.h"
#include "renderer/resources/Material.h"
#include "renderer/resources/Model.h"
#include "renderer/resources/ModelManager.h"
#include "renderer/RenderSystem.h"
#include "renderer/frontend/RenderWorld.h"

// sound engine
#include "../sound/sound.h"

// user interfaces
#include "../ui/ListGUI.h"
#include "../ui/UserInterface.h"

// collision detection system
#include "../cm/CollisionModel.h"

// AAS files and manager
#include "../tools/compilers/aas/AASFile.h"
#include "../tools/compilers/aas/AASFileManager.h"

// game
#include "../game/Game.h"

// stgatilov: automation (for tests)
#include "../game/gamesys/Automation.h"

//-----------------------------------------------------

#ifndef _D3SDK

#include "../game/Game_local.h"
#ifdef GAME_DLL

#if defined(_D3XP)
#include "../d3xp/Game_local.h"
#else
#include "../game/Game_local.h"
#endif

#else


// framework
#include "../framework/Compressor.h"
#include "../framework/EventLoop.h"
#include "../framework/KeyInput.h"
#include "../framework/EditField.h"
#include "../framework/Console.h"
#include "../framework/DemoFile.h"
#include "../framework/Session.h"

// The editor entry points are always declared, but may just be
// stubbed out on non-windows platforms.
#include "../tools/edit_public.h"

// Compilers for map, model, video etc. processing.
#include "../tools/compilers/compiler_public.h"

#endif /* !GAME_DLL */

#endif /* !_D3SDK */

//-----------------------------------------------------

#undef min
#undef max
#include <algorithm>	// for min / max / swap

#endif /* !__PRECOMPILED_H__ */
