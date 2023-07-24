// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_PRECOMPILED_H__
#define __GAME_PRECOMPILED_H__

#include "../idlib/LibOS.h"

//-----------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <wchar.h>	// wmemset
#include <limits.h>

//-----------------------------------------------------

#include "../framework/BuildDefines.h"

#include "../sys/sys_public.h"
#include "../idlib/Lib.h"
#include "../idlib/LibImpl.h"

#include "../framework/declManager.h"
#include "../framework/CVarSystem.h"
#include "../framework/CmdSystemDeclCompletion.h"
#include "../framework/UsercmdGen.h"
#include "../framework/FileSystem.h"
#include "../framework/async/NetworkSystem.h"
#include "../framework/GraphManager.h"

#include "../cm/CollisionModel.h"

#include "../renderer/RenderSystem.h"
#include "../renderer/RenderWorld.h"
#include "../renderer/Model.h"
#include "../renderer/ModelManager.h"
#include "../renderer/RenderWorld.h"

#include "../decllib/declType.h"

#include "../libs/AASLib/AASFile.h"
#include "../libs/AASLib/AASFileManager.h"

#include "../sound/SoundWorld.h"
#include "../sound/SoundSystem.h"

#include "../libs/filelib/File.h"

#include "../decllib/declTypeHolder.h"

#include "Game.h"

#include "Common.h"

#include "gamesys/Event.h"
#include "gamesys/Class.h"
#include "gamesys/SysCvar.h"
#include "gamesys/SaveGame.h"

#include "Game_local.h"

#endif // __GAME_PRECOMPILED_H__
