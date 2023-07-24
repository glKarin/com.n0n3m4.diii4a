// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"

/*
================
idBotObstacle::idBotObstacle
================
*/
idBotObstacle::idBotObstacle() {
	num = 0;
	areaNum[0] = 0;
	areaNum[1] = 0;
	bbox.Clear();
}