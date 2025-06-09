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

#ifndef SE_INCL_SERIOUSSAM_STDH_H
#define SE_INCL_SERIOUSSAM_STDH_H

#ifdef PRAGMA_ONCE
#pragma once
#endif

#include <Engine/Engine.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CModelData.h>
#include <GameMP/Game.h>

/* rcg10042001 protect against Visual C-isms. */
#ifdef _MSC_VER
#define DECL_DLL _declspec(dllimport)
#define _offsetof offsetof
#endif

#ifdef _MSC_VER
#define __extern extern
#else
#define __extern
#endif

#ifdef PLATFORM_UNIX
#define DECL_DLL 
#endif

#ifdef FIRST_ENCOUNTER
#include <Entities/Global.h>
#include <Entities/Common/Common.h>
#include <Entities/Common/GameInterface.h>
#include <Entities/WorldLink.h>
#include <Entities/Player.h>
#else
#include <EntitiesMP/Global.h>
#include <EntitiesMP/Common/Common.h>
#include <EntitiesMP/Common/GameInterface.h>
#include <EntitiesMP/WorldLink.h>
#include <EntitiesMP/Player.h>
#endif
#undef DECL_DLL

#include "SeriousSam.h"
#include "Menu.h"
#include "MenuGadgets.h"

#endif

