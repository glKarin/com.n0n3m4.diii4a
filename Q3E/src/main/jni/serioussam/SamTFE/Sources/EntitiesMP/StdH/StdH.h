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

#include <Engine/Engine.h>
#include <GameMP/SessionProperties.h>
#include <GameMP/PlayerSettings.h>

/* rcg10042001 protect against Visual C-isms. */
#ifdef _MSC_VER
#define DECL_DLL _declspec(dllexport)
#ifndef _offsetof
#define _offsetof offsetof
#endif
#endif

#ifdef PLATFORM_UNIX
#define DECL_DLL 
#endif

#include "../Global.h"
#include "../Common/Flags.h"
#include "../Common/Common.h"
#include "../Common/Particles.h"
#include "../Common/EmanatingParticles.h"
#include "../Common/GameInterface.h"

