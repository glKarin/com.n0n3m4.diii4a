
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "Game_Log.h"

/*
===============================================================================

	rvGameLog

===============================================================================
*/

rvGameLogLocal	gameLogLocal;
rvGameLog*		gameLog = &gameLogLocal;

/*
================
rvGameLogLocal::rvGameLogLocal
================
*/
rvGameLogLocal::rvGameLogLocal ( ) {
	initialized	= false;
}

/*
================
rvGameLogLocal::Init
================
*/
void rvGameLogLocal::Init ( void ) {
	file        = NULL;
	indexCount  = 0;
	initialized	= true;
	
	index.Clear ( );
	frame.Clear ( );
	oldframe.Clear ( );
}

/*
================
rvGameLogLocal::Shutdown
================
*/
void rvGameLogLocal::Shutdown	( void ) {
	index.Clear ( );
	frame.Clear ( );
	oldframe.Clear ( );

	if ( initialized && file ) {
		const char* out;
		out = va(":%d %d", gameLocal.time, gameLocal.framenum );
		file->Write ( out, strlen ( out ) );
		file->Flush ( );
		fileSystem->CloseFile ( file );
		file = NULL;
	}
	initialized	= false;
}

/*
================
rvGameLogLocal::BeginFrame
================
*/
void rvGameLogLocal::BeginFrame	( int time ) {
	// See if logging has been turned on or not
	if ( g_gamelog.GetBool ( ) != initialized ) {
		if ( initialized ) {
			Shutdown ( );
			return;
		} else { 
			Init ( );
		}
	} else if ( !g_gamelog.GetBool ( ) ) {
		return;
	}		
}

/*
================
rvGameLogLocal::EndFrame
================
*/
void rvGameLogLocal::EndFrame ( void ) {
	int			i;
	const char* out;
	bool		wroteTime;

	// Dont do anything if not logging
	if ( !g_gamelog.GetBool ( ) ) {
		return;
	}

	// When not in multiplayer, log the players approx origin and viewangles
	if ( !gameLocal.isMultiplayer ) {
		idPlayer* player;
		player = gameLocal.GetLocalPlayer ( );
		if ( player ) {
			Set ( "player0_origin", va("%d %d %d", (int)player->GetPhysics()->GetOrigin()[0], (int)player->GetPhysics()->GetOrigin()[1], (int)player->GetPhysics()->GetOrigin()[2] ) );
			Set ( "player0_angles_yaw", va("%g", (float)player->viewAngles[YAW] ) );
			Set ( "player0_angles_pitch", va("%g", (float)player->viewAngles[PITCH] ) );
			Set ( "player0_buttons", player->usercmd.buttons );
			Set ( "player0_health", player->health );
			Set ( "player0_armor", player->inventory.armor );
		}
	}	

	if ( !file ) {
		idStr mapName;
		idStr filename;
		mapName = gameLocal.serverInfo.GetString( "si_map" );
		mapName.StripFileExtension ( );
		filename = "logs/" + mapName + "/" + cvarSystem->GetCVarString("win_username") + "_";

		// Find a unique filename
		for ( i = 0; fileSystem->ReadFile( filename + va("%06d.log", i ), NULL, NULL ) > 0; i ++ );		

		// Actually open the file now 
		file = fileSystem->OpenFileWrite ( filename + va("%06d.log", i ), "fs_cdpath" );				
		if ( !file ) {
			return;
		}
		
		timer_fps.Stop( );
		timer_fps.Clear ( );
		timer_fps.Start ( );
	} else {
		static int		fpsIndex;
		static float	fpsValue[4];
				
		timer_fps.Stop ( );	
		fpsValue[(fpsIndex++)%4] = 1000.0f / (timer_fps.Milliseconds ( ) + 1);
		if ( fpsIndex >= 4 ) {
			GAMELOG_SET ( "fps", Min(60,(int)((int)(fpsValue[0] + fpsValue[1] + fpsValue[2] + fpsValue[3]) / 40.0f) * 10) );
		}
		timer_fps.Clear ( );
		timer_fps.Start ( );
	}
	
	// Write out any new indexes that were added this frame
	for ( ; indexCount < index.Num(); indexCount ++ ) {
		const char* out;
		out = va("#%d ", indexCount );
		file->Write ( out, strlen ( out ) );
		file->Write ( index[indexCount].c_str(), index[indexCount].Length() );	
		file->Write ( "\r\n", 2 );
	}
		
	// Write out any data that was added this frame	
	wroteTime = false;
	for ( i = frame.Num() - 1; i >= 0; i -- ) {
		// TODO: filter
		if ( oldframe[i] != frame[i] ) {
			if ( !wroteTime ) {
				out = va(":%d %d", gameLocal.time, gameLocal.framenum );
				file->Write ( out, strlen ( out ) );
				wroteTime = true;
			}		
			out = va(" %d \"", i );
			file->Write ( out, strlen(out) );
			file->Write ( frame[i].c_str(), frame[i].Length ( ) );
			file->Write ( "\"", 1 );
			oldframe[i] = frame[i];
		}		
	}		

	if ( wroteTime ) {
		file->Write ( "\r\n", 2 );	
		file->Flush ( );
	}

	// Clear the frame for next time
	for ( i = index.Num() - 1; i >= 0; i -- ) {
		frame[i] = "";
	}
}

/*
================
rvGameLogLocal::Set
================
*/
void rvGameLogLocal::Set ( const char* keyword, const char* value ) {	
	int i;
	i = index.AddUnique ( keyword );
	frame.SetNum ( index.Num(), true );
	oldframe.SetNum ( index.Num(), true );
	frame[i] = value;
}

void rvGameLogLocal::Set ( const char* keyword, int value ) {
	Set ( keyword, va("%d", value ) );	
}

void rvGameLogLocal::Set ( const char* keyword, float value ) {
	Set ( keyword, va("%g", value ) );
}

void rvGameLogLocal::Set ( const char* keyword, bool value ) {
	Set ( keyword, va("%d", (int)value ) );
}
	
/*
================
rvGameLogLocal::Add
================
*/
void rvGameLogLocal::Add ( const char* keyword, int value ) {
	int i;
	i = index.AddUnique ( keyword );
	frame.SetNum ( index.Num(), true );
	oldframe.SetNum ( index.Num(), true );
	frame[i] = va("%d",atoi(frame[i].c_str()) + value );
}

void rvGameLogLocal::Add ( const char* keyword, float value ) {
	int i;
	i = index.AddUnique ( keyword );
	frame.SetNum ( index.Num(), true );
	oldframe.SetNum ( index.Num(), true );
	frame[i] = va("%g",atof(frame[i].c_str()) + value );
}


