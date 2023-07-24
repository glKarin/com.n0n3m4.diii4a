// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#include "../Game_local.h" 
#include "BotThreadData.h"
#include "BotAI_Main.h"


/*
================
idBotRoutes::idBotRoutes
================
*/
idBotRoutes::idBotRoutes() {
	team = NOTEAM;
	radius = 120.0f;
	groupID = 0;
	isHeadNode = false;
	active = false;
	origin.Zero();
	name = "";
	routeLinks.Clear();
	num = -1;
}