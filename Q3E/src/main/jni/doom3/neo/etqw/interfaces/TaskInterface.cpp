// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "TaskInterface.h"
#include "../Player.h"
#include "../roles/Tasks.h"

/*
================
sdTaskInterface::sdTaskInterface
================
*/
sdTaskInterface::sdTaskInterface( idEntity* entity ) {
	taskNode.SetOwner( entity ); 
	sdTaskManager::GetInstance().AddTaskEntity( taskNode ); 
}
