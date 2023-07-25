
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

/*

  game_endlevel.cpp

  This entity is targeted to complete a level, and it also handles
  running the stats and moving the camera.

*/


CLASS_DECLARATION( idEntity, idTarget_EndLevel )
	EVENT( EV_Activate,		idTarget_EndLevel::Event_Trigger )
END_CLASS

/*
================
idTarget_EndLevel::Spawn
================
*/
void idTarget_EndLevel::Spawn( void ) {
	idStr		guiName;

	gui = NULL;
	noGui = spawnArgs.GetBool("noGui");
	if (!noGui) {
		spawnArgs.GetString( "guiName", "guis/EndLevel.gui", guiName );

		if (guiName.Length()) {
			gui = idUserInterface::FindGui( guiName, true, false, true );
		}
	}

	buttonsReleased = false;
	readyToExit = false;

	exitCommand = "";
}

/*
================
idTarget_EndLevel::~idTarget_EndLevel()
================
*/
idTarget_EndLevel::~idTarget_EndLevel() {
	//FIXME: need to go to smart ptrs for gui allocs or the unique method 
	//delete gui;
}

/*
================
idTarget_EndLevel::Event_Trigger
================
*/
void idTarget_EndLevel::Event_Trigger( idEntity *activator ) {
	if ( gameLocal.endLevel ) {
		return;
	}
	
	// mark the endLevel, which will modify some game actions
	// and pass control to us for drawing the stats and camera position
	gameLocal.endLevel = this;

	// grab the activating player view position
	idPlayer *player = (idPlayer *)(activator);

	initialViewOrg = player->GetEyePosition();
	initialViewAngles = idVec3( player->viewAngles[0], player->viewAngles[1], player->viewAngles[2] );

	// kill all the sounds
	gameSoundWorld->StopAllSounds();

	if ( noGui ) {
		readyToExit = true;
	}
}

/*
================
idTarget_EndLevel::Draw
================
*/
void idTarget_EndLevel::Draw() {

	if (noGui) {
		return;
	}

	renderView_t			renderView;

	memset( &renderView, 0, sizeof( renderView ) );

	renderView.width = SCREEN_WIDTH;
	renderView.height = SCREEN_HEIGHT;
	renderView.x = 0;
	renderView.y = 0;

	renderView.fov_x = 90;
	renderView.fov_y = gameLocal.CalcFovY( renderView.fov_x );
	renderView.time = gameLocal.time;

#if 0
	renderView.vieworg = initialViewOrg;
	renderView.viewaxis = idAngles(initialViewAngles).toMat3();
#else
	renderView.vieworg = renderEntity.origin;
	renderView.viewaxis = renderEntity.axis;
#endif

	gameRenderWorld->RenderScene( &renderView );

	// draw the gui on top of the 3D view
	gui->Redraw(gameLocal.time);
}

/*
================
idTarget_EndLevel::PlayerCommand
================
*/
void idTarget_EndLevel::PlayerCommand( int buttons ) {
	if ( !( buttons & BUTTON_ATTACK ) ) {
		buttonsReleased = true;
		return;
	}
	if ( !buttonsReleased ) {
		return;
	}

	// we will exit at the end of the next game frame
	readyToExit = true;
}

/*
================
idTarget_EndLevel::ExitCommand
================
*/
const char *idTarget_EndLevel::ExitCommand() {
	if ( !readyToExit ) {
		return NULL;
	}

	idStr nextMap;

	if (spawnArgs.GetString( "nextMap", "", nextMap )) {
		sprintf( exitCommand, "map %s", nextMap.c_str() );
	} else {
		exitCommand = "";
	}

	return exitCommand;
}
