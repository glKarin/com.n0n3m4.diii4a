// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "DemoManager.h"
#include "DemoScript.h"

#include "../Player.h"
#include "../guis/UserInterfaceLocal.h"

//===============================================================
//
//	sdDemoProperties
//
//===============================================================

/*
================
sdDemoProperties::GetProperty
================
*/
sdProperties::sdProperty* sdDemoProperties::GetProperty( const char* name ) {
	return properties.GetProperty( name, sdProperties::PT_INVALID, false );
}

/*
============
sdDemoProperties::GetProperty
============
*/
sdProperties::sdProperty* sdDemoProperties::GetProperty( const char* name, sdProperties::ePropertyType type ) {
	sdProperties::sdProperty* prop = properties.GetProperty( name, sdProperties::PT_INVALID, false );
	if ( prop && prop->GetValueType() != type ) {
		gameLocal.Error( "sdDemoProperties::GetProperty: type mismatch for property '%s'", name );
	}
	return prop;
}

/*
================
sdDemoProperties::Init
================
*/
void sdDemoProperties::Init() {
	properties.RegisterProperty( "state",			state );
	properties.RegisterProperty( "time",			time );
	properties.RegisterProperty( "size",			size );
	properties.RegisterProperty( "position",		position );
	properties.RegisterProperty( "frame",			frame );	
	properties.RegisterProperty( "cutIsSet",		cutIsSet );
	properties.RegisterProperty( "cutStartMarker",	cutStartMarker );
	properties.RegisterProperty( "cutEndMarker",	cutEndMarker );

	properties.RegisterProperty( "demoName",		demoName );
	properties.RegisterProperty( "writingMDF",		writingMDF );
	properties.RegisterProperty( "mdfName",			mdfName );

	properties.RegisterProperty( "viewOrigin",		viewOrigin );
	properties.RegisterProperty( "viewAngles",		viewAngles );
}

/*
================
sdDemoProperties::Shutdown
================
*/
void sdDemoProperties::Shutdown() {
	properties.Clear();
}

/*
================
sdDemoProperties::UpdateProperties
================
*/
void sdDemoProperties::UpdateProperties() {
	state = sdDemoManager::GetInstance().GetState();

	time = MS2SEC( sdDemoManager::GetInstance().GetTime() );
	size = sdDemoManager::GetInstance().GetSize();
	position = sdDemoManager::GetInstance().GetPosition();

	frame = sdDemoManager::GetInstance().GetFrame();

	cutIsSet = sdDemoManager::GetInstance().CutIsSet() ? 1.0f : 0.0f;
	cutStartMarker = sdDemoManager::GetInstance().GetCutStartMarker();
	cutEndMarker = sdDemoManager::GetInstance().GetCutEndMarker();

	demoName = networkSystem->GetDemoName();
	writingMDF = sdDemoManager::GetInstance().WritingMDF() ? 1.0f : 0.0f;
	mdfName = sdDemoManager::GetInstance().GetMDFFileName();

	const renderView_t& view = sdDemoManager::GetInstance().GetRenderedView();
	viewOrigin = view.vieworg;

	idAngles angles = view.viewaxis.ToAngles();
	viewAngles = idVec3( angles.pitch, angles.yaw, angles.roll );
}

//===============================================================
//
//	sdDemoManagerLocal
//
//===============================================================

idCVar sdDemoManagerLocal::g_demoOutputMDF(	"g_demoOutputMDF",	"0",	CVAR_GAME | CVAR_INTEGER,	"output entity keyframe data from demo", 0, 2, idCmdSystem::ArgCompletion_Integer<0,2> );
idCVar sdDemoManagerLocal::g_showDemoHud(	"g_showDemoHud",	"0",	CVAR_GAME | CVAR_BOOL,		"draw the demo hud gui" );
idCVar sdDemoManagerLocal::g_showDemoView(	"g_showDemoView",	"0",	CVAR_GAME | CVAR_BOOL,		"show player's calculated view when paused instead of free-fly cam" );
idCVar sdDemoManagerLocal::g_demoAnalyze(	"g_demoAnalyze",	"0",	CVAR_GAME | CVAR_BOOL,		"analyze demo during playback" );
idCVar sdDemoManagerLocal::demo_noclip(		"demo_noclip",		"0",	CVAR_SYSTEM | CVAR_BOOL,	"noclip through a demo" );

/*
================
sdDemoManagerLocal::sdDemoManagerLocal
================
*/
sdDemoManagerLocal::~sdDemoManagerLocal() {
}

/*
================
sdDemoManagerLocal::Init
================
*/
void sdDemoManagerLocal::Init() {
	sdDemoScript::InitClass();

	cameraFactory.RegisterType( sdDemoCamera_Fixed::GetType(), sdCameraFactory::Allocator< sdDemoCamera_Fixed > );
	cameraFactory.RegisterType( sdDemoCamera_Anim::GetType(), sdCameraFactory::Allocator< sdDemoCamera_Anim > );

	state = DS_NONE;
	script = NULL;
	activeCamera = NULL;

	memset( &renderedView, 0, sizeof( renderView_t ) );

	demoGameFrames = 0;
	pausedTime = previousPausedTime = 0;
	melFrames = 0;

	viewAngles.Zero();
	deltaViewAngles.Zero();
	viewOrigin.Zero();
	currentVelocity.Zero();

	melDataFile = NULL;

	localDemoProperties.Init();
}

/*
================
sdDemoManagerLocal::InitGUIs
================
*/
void sdDemoManagerLocal::InitGUIs() {
	hud = gameLocal.LoadUserInterface( "guis/demos/hud", false, true );
}

/*
============
sdDemoManagerLocal::GetPosition
============
*/
float sdDemoManagerLocal::GetPosition() const {
	if( endPosition == startPosition ) {
		return 0;
	}
	return ( position - startPosition ) / static_cast< float >( endPosition - startPosition );
}

/*
============
sdDemoManagerLocal::GetCutStartMarker
============
*/
float sdDemoManagerLocal::GetCutStartMarker() const {
	if( endPosition == startPosition ) {
		return 0;
	}
	return ( cutStartMarker - startPosition ) / static_cast< float >( endPosition - startPosition );
}

/*
============
sdDemoManagerLocal::GetCutEndMarker
============
*/
float sdDemoManagerLocal::GetCutEndMarker() const {
	if( endPosition == startPosition ) {
		return 0;
	}
	return ( cutEndMarker - startPosition ) / static_cast< float >( endPosition - startPosition );
}

/*
================
sdDemoManagerLocal::StartDemo
================
*/
void sdDemoManagerLocal::StartDemo() {
	state = static_cast< demoState_t >( networkSystem->GetDemoState( time, position, length, startPosition, endPosition, cutStartMarker, cutEndMarker ) );

	if ( state != DS_PLAYING ) {
		return;
	}

	viewOrigin.Zero();
	viewAngles.Zero();
	currentVelocity.Zero();
	deltaViewAngles.Zero();
	renderedView.vieworg.Zero();
	renderedView.viewaxis.Identity();

	script = new sdDemoScript;
	if ( !script->Parse( networkSystem->GetDemoName() ) ) {
		delete script;
		script = NULL;
	}
	activeCamera = NULL;

	demoGameFrames = 0;

	if ( g_demoOutputMDF.GetInteger() > 0 ) {
		melDataFileName = "demos/";
		melDataFileName += networkSystem->GetDemoName();

		if ( g_demoOutputMDF.GetInteger() == 2 ) {
			sysTime_t time;
			sys->RealTime( &time );

			melDataFileName += va( "_%d%02d%02d_%02d%02d%02d", 1900 + time.tm_year, 1 + time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec );
		}

		melDataFileName.SetFileExtension( "mdf" );
		melDataFile = fileSystem->OpenFileWrite( melDataFileName, "fs_devpath" );
		melFrames = 0;

		if ( melDataFile ) {
			melDataFile->WriteInt( 0 );	// dummy for numFrames
			melDataFile->WriteInt( ENTITYNUM_MAX_NORMAL );
//			melDataFile->WriteInt( g_demoOutputMDF.GetInteger() == 2 ? 1 : 0 );	// timestamps in demo?
			melDataFile->WriteInt( 1 );	// timestamps in demo?

			melPrimitives.AssureSize( ENTITYNUM_MAX_NORMAL, NULL );
		}
	}

	if ( g_demoAnalyze.GetBool() ) {
		demoAnalyzer.Start();
	}

	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( hud );
	if ( ui && !ui->IsActive() ) {
		ui->Activate();
	}

	localDemoProperties.UpdateProperties();
}

/*
================
sdDemoManagerLocal::EndDemo
================
*/
void sdDemoManagerLocal::EndDemo() {
	state = DS_NONE;

	delete script;
	script = NULL;

	melDataFileName.Clear();
	if ( melDataFile ) {
		melDataFile->Seek( 0, FS_SEEK_SET );
		melDataFile->WriteInt( melFrames );

		fileSystem->CloseFile( melDataFile );
		melDataFile = NULL;

		melPrimitives.DeleteContents( true );
	}

	if ( demoAnalyzer.IsActive() ) {
		demoAnalyzer.Stop();
	}

	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( hud );
	if ( ui && ui->IsActive() ) {
		ui->Deactivate();
	}
}

/*
================
sdDemoManagerLocal::RunDemoFrame
================
*/
void sdDemoManagerLocal::RunDemoFrame( const usercmd_t* demoCmd ) {

	if ( demoCmd ) {
		this->demoCmd = *demoCmd;
	} else {
		memset( &( this->demoCmd ), 0, sizeof( usercmd_t ) );
		if ( gameLocal.localClientNum >= 0 && gameLocal.localClientNum < MAX_CLIENTS ) {
			for( int i = 0; i < 3; i++ ) {
				this->demoCmd.angles[ i ] = gameLocal.usercmds[ gameLocal.localClientNum ].angles[ i ];
			}
		}
	}

	// update demo state
	demoState_t oldState = state;
	previousTime = time;
	state = static_cast< demoState_t >( networkSystem->GetDemoState( time, position, length, startPosition, endPosition, cutStartMarker, cutEndMarker ) );

	switch( state ) {
	case DS_PAUSED:
		if ( oldState != DS_PAUSED ) {
			if ( !demo_noclip.GetBool() ) {
				idPlayer* player = gameLocal.GetLocalViewPlayer();
				if ( player != NULL ) {
					viewOrigin = player->firstPersonViewOrigin;
					viewAngles = player->viewAngles;
				}
				currentVelocity.Zero();
			}
			for( int i = 0; i < 3; i++ ) {
				deltaViewAngles[ i ] = viewAngles[ i ] - SHORT2ANGLE( this->demoCmd.angles[ i ] );
			}
			pausedTime = sys->Milliseconds();
		}
		previousPausedTime = pausedTime;
		pausedTime = sys->Milliseconds();
		break;
	case DS_PLAYING:
		if ( script ) {
			script->RunFrame();
		}
		if ( activeCamera ) {
			activeCamera->RunFrame();
		}
		if ( demoAnalyzer.IsActive() ) {
			demoAnalyzer.RunFrame();
		}
		break;
	}

	localDemoProperties.UpdateProperties();
}

/*
================
sdDemoManagerLocal::EndDemoFrame
================
*/
void sdDemoManagerLocal::EndDemoFrame() {
	if ( state == DS_PLAYING && melDataFile && gameLocal.isNewFrame ) {
		const sdDeclTargetInfo* targetInfoDecl = gameLocal.GetMDFExportTargets();

		idList< idEntity* >	filteredEntities;
		idList< idEntity* > entityList;
		idList< int > entityNumList;

//		if ( g_demoOutputMDF.GetInteger() == 2 ) {
			// write timestamp, assumes demo is played back in normal speed (33 msec frames)
			melDataFile->WriteInt( ( gameLocal.time - gameLocal.startTime ) / USERCMD_MSEC );
//		}

		//
		// write camera data
		//
		idVec3 cameraOrigin;
		idMat3 cameraAxis;
		idAngles cameraAngles;

		cameraOrigin = renderedView.vieworg;

		cameraAxis[ 0 ] = -renderedView.viewaxis[ 1 ];
		cameraAxis[ 1 ] = renderedView.viewaxis[ 2 ];
		cameraAxis[ 2 ] = -renderedView.viewaxis[ 0 ];

		cameraOrigin.ToMayaSelf();
		cameraAxis.ToMayaSelf();
		cameraAngles = cameraAxis.ToAnglesMaya();

		// origin
		melDataFile->WriteDouble( cameraOrigin.x );
		melDataFile->WriteDouble( cameraOrigin.y );
		melDataFile->WriteDouble( cameraOrigin.z );

		// angles
		melDataFile->WriteDouble( cameraAngles.pitch );
		melDataFile->WriteDouble( cameraAngles.yaw );
		melDataFile->WriteDouble( cameraAngles.roll );

		//
		// filter entities and make a list of entities to hide
		//
		for ( int i = 0; i < ENTITYNUM_MAX_NORMAL; i++ ) {
			idEntity* ent = gameLocal.entities[ i ];

			if ( !ent ) {
				if ( melPrimitives[ i ] && melPrimitives[ i ]->visible ) {
					entityNumList.Append( i );
				}
				continue;
			}

			if ( targetInfoDecl ) {
				if ( !targetInfoDecl->FilterEntity( ent ) ) {
					if ( melPrimitives[ ent->entityNumber ] && melPrimitives[ ent->entityNumber ]->visible ) {
						entityNumList.Append( ent->entityNumber );
					}
					continue;
				}
			}

			filteredEntities.Append( ent );
		}
		// write list
		melDataFile->WriteInt( entityNumList.Num() );
		for ( int i = 0; i < entityNumList.Num(); i++ ) {
			melDataFile->WriteInt( entityNumList[ i ] );
		}

		//
		// build list of entities to create
		//
		entityList.Clear();
		for ( int i = 0; i < filteredEntities.Num(); i++ ) {
			idEntity* ent = filteredEntities[ i ];

			if ( !melPrimitives[ ent->entityNumber ] ) {
				melPrimitives[ ent->entityNumber ] = new melPrimitive_t;
				melPrimitives[ ent->entityNumber ]->visible = false;
				melPrimitives[ ent->entityNumber ]->bounds.Zero();

				entityList.Append( ent );
			}
		}
		// write list
		melDataFile->WriteInt( entityList.Num() );
		for ( int i = 0; i < entityList.Num(); i++ ) {
			melDataFile->WriteInt( entityList[ i ]->entityNumber );
		}

		//
		// build list of entities to make visible
		//
		entityList.Clear();
		for ( int i = 0; i < filteredEntities.Num(); i++ ) {
			idEntity* ent = filteredEntities[ i ];

			if ( !melPrimitives[ ent->entityNumber ]->visible ) {
				melPrimitives[ ent->entityNumber ]->visible = true;

				entityList.Append( ent );
			}
		}
		// write list
		melDataFile->WriteInt( entityList.Num() );
		for ( int i = 0; i < entityList.Num(); i++ ) {
			melDataFile->WriteInt( entityList[ i ]->entityNumber );
		}

		//
		// build list of entity bounding box updates
		//
		entityList.Clear();
		for ( int i = 0; i < filteredEntities.Num(); i++ ) {
			idEntity* ent = filteredEntities[ i ];

			if ( !melPrimitives[ ent->entityNumber ]->bounds.Compare( ent->GetPhysics()->GetBounds() ) && !ent->GetPhysics()->GetBounds().IsCleared() ) {
				melPrimitives[ ent->entityNumber ]->bounds = ent->GetPhysics()->GetBounds();
			//if ( !melPrimitives[ ent->entityNumber ]->bounds.Compare( ent->GetRenderEntity()->bounds ) ) {
			//	melPrimitives[ ent->entityNumber ]->bounds = ent->GetRenderEntity()->bounds;

				entityList.Append( ent );
			}
		}
		// write list
		melDataFile->WriteInt( entityList.Num() );
		for ( int i = 0; i < entityList.Num(); i++ ) {
			idEntity* ent = entityList[ i ];

			melDataFile->WriteInt( ent->entityNumber );

			// size information
			melDataFile->WriteDouble( melPrimitives[ ent->entityNumber ]->bounds.GetMaxs().x - melPrimitives[ ent->entityNumber ]->bounds.GetMins().x );
			melDataFile->WriteDouble( melPrimitives[ ent->entityNumber ]->bounds.GetMaxs().y - melPrimitives[ ent->entityNumber ]->bounds.GetMins().y );
			melDataFile->WriteDouble( melPrimitives[ ent->entityNumber ]->bounds.GetMaxs().z - melPrimitives[ ent->entityNumber ]->bounds.GetMins().z );
		}

		//
		// write list of entity origin and angles
		//
		melDataFile->WriteInt( filteredEntities.Num() );
		for ( int i = 0; i < filteredEntities.Num(); i++ ) {
			idEntity* ent = filteredEntities[ i ];

			melDataFile->WriteInt( ent->entityNumber );

			// origin
			idVec3 originMaya = (ent->GetPhysics()->GetOrigin()).ToMaya();
			//idVec3 originMaya = (ent->GetRenderEntity()->origin).ToMaya();
 
			melDataFile->WriteDouble( originMaya.x );
			melDataFile->WriteDouble( originMaya.y );
			melDataFile->WriteDouble( originMaya.z );

			// angles
			if ( ent->IsType( idPlayer::Type ) ) {
				melDataFile->WriteDouble( ent->GetPhysics()->GetAxis().ToAngles().roll );
				melDataFile->WriteDouble( static_cast<const idPlayer *>(ent)->viewAngles.yaw - 90.f );
				melDataFile->WriteDouble( -ent->GetPhysics()->GetAxis().ToAngles().pitch );
			} else {
				idMat3 mayaAxis;
				idAngles anglesMaya;

				mayaAxis[ 0 ] = -ent->GetPhysics()->GetAxis()[ 1 ];
				mayaAxis[ 1 ] = ent->GetPhysics()->GetAxis()[ 2 ];
				mayaAxis[ 2 ] = -ent->GetPhysics()->GetAxis()[ 0 ];

				mayaAxis.ToMayaSelf();
				anglesMaya = mayaAxis.ToAnglesMaya();

				melDataFile->WriteDouble( anglesMaya.pitch );
				melDataFile->WriteDouble( anglesMaya.yaw );
				melDataFile->WriteDouble( anglesMaya.roll );
			}
		}

		melFrames++;
	}

	if ( gameLocal.isNewFrame && state != DS_PAUSED ) {
		demoGameFrames++;
	}
}

 /*
================
sdDemoManagerLocal::SetActiveCamera
================
*/
void sdDemoManagerLocal::SetActiveCamera( sdDemoCamera* camera ) {
	activeCamera = camera;
	activeCamera->Start();

	// jrad - ensure that this is running updates
	sdPostProcess* postProcess = gameLocal.localPlayerProperties.GetPostProcess();
	if( postProcess != NULL && !postProcess->Enabled() ) {
		postProcess->Enable( true );
	}
}

/*
================
sdDemoManagerLocal::CalculateRenderView
================
*/
bool sdDemoManagerLocal::CalculateRenderView( renderView_t* renderView ) {
	if ( sdDemoManager::GetInstance().NoClip() ) {

		// jrad - ensure that this is running updates
		sdPostProcess* postProcess = gameLocal.localPlayerProperties.GetPostProcess();
		if( postProcess != NULL && !postProcess->Enabled() ) {
			postProcess->Enable( true );
		}

		// update camera origin
		for( int i = 0; i < 3; i++ ) {
			viewAngles[ i ] = idMath::AngleNormalize180( SHORT2ANGLE( demoCmd.angles[ i ] ) + deltaViewAngles[ i ] );
		}

		viewAngles.pitch = idMath::ClampFloat( -89.f, 89.f, viewAngles.pitch );

		for( int i = 0; i < 3; i++ ) {
			deltaViewAngles[ i ] = viewAngles[ i ] - SHORT2ANGLE( demoCmd.angles[ i ] );
		}

		// calculate vectors
		idVec3 gravityNormal = gameLocal.GetGravity();
		idVec3 viewForward, viewRight;

		gravityNormal.Normalize();

		viewForward = viewAngles.ToForward();
		viewForward.Normalize();
		viewRight = gravityNormal.Cross( viewForward );
		viewRight.Normalize();

		// scale user command
		int		max;
		float	scale;
		int		forwardmove;
		int		rightmove;
		int		upmove;

		forwardmove = demoCmd.forwardmove;
		rightmove = demoCmd.rightmove;
		upmove = demoCmd.upmove;

		max = abs( forwardmove );
		if ( abs( rightmove ) > max ) {
			max = abs( rightmove );
		}
		if ( abs( upmove ) > max ) {
			max = abs( upmove );
		}

		if ( !max ) {
			scale = 0.0f;
		} else {
			float total = idMath::Sqrt( (float) forwardmove * forwardmove + rightmove * rightmove + upmove * upmove );
			scale = pm_democamspeed.GetFloat() * max / ( 127.0f * total );
		}

		// move
		float frametime;

		if ( state == DS_PAUSED ) {
			frametime = MS2SEC( pausedTime - previousPausedTime );
		} else {
			frametime = MS2SEC( gameLocal.msec );
		}

		float	speed, drop, friction, newspeed, stopspeed;
		float	wishspeed;
		idVec3	wishdir;

		// friction
		speed = currentVelocity.Length();
		if ( speed < 20.f ) {
			currentVelocity.Zero();
		}
		else {
			stopspeed = pm_democamspeed.GetFloat() * 0.3f;
			if ( speed < stopspeed ) {
				speed = stopspeed;
			}
			friction = 12.0f /*PM_NOCLIPFRICTION*/;
			drop = speed * friction * frametime;

			// scale the velocity
			newspeed = speed - drop;
			if (newspeed < 0) {
				newspeed = 0;
			}

			currentVelocity *= newspeed / speed;
		}

		// accelerate
		wishdir = scale * (viewForward * demoCmd.forwardmove + viewRight * demoCmd.rightmove);
		wishdir -= scale * gravityNormal * demoCmd.upmove;
		wishspeed = wishdir.Normalize();
		wishspeed *= scale;

		// q2 style accelerate
		float addspeed, accelspeed, currentspeed;

		currentspeed = currentVelocity * wishdir;
		addspeed = wishspeed - currentspeed;
		if ( addspeed > 0 ) {
			accelspeed = 10.0f /*PM_ACCELERATE*/ * frametime * wishspeed;
			if ( accelspeed > addspeed ) {
				accelspeed = addspeed;
			}
			
			currentVelocity += accelspeed * wishdir;
		}

		// move
		viewOrigin += frametime * currentVelocity;

		renderView->viewID = 0;
		renderView->vieworg = viewOrigin;
		renderView->viewaxis = viewAngles.ToMat3();
		renderView->fov_x = g_fov.GetFloat();

		renderView->time = gameLocal.time;

		renderView->x = 0;
		renderView->y = 0;
		renderView->width = SCREEN_WIDTH;
		renderView->height = SCREEN_HEIGHT;

		return true;
	} else {
		currentVelocity.Zero();
	}

	if ( state == DS_PLAYING ) {
		if ( activeCamera ) {
			renderView->viewID = 0;
			renderView->vieworg = activeCamera->GetOrigin();
			renderView->viewaxis = activeCamera->GetAxis();
			renderView->fov_x = activeCamera->GetFov();

			renderView->time = gameLocal.time;

			renderView->x = 0;
			renderView->y = 0;
			renderView->width = SCREEN_WIDTH;
			renderView->height = SCREEN_HEIGHT;

			return true;
		}
	}

	return false;
}

/*
================
sdDemoManagerLocal::Shutdown
================
*/
void sdDemoManagerLocal::Shutdown() {
	localDemoProperties.Shutdown();
	cameraFactory.Shutdown();

	delete script;

	sdDemoScript::Shutdown();
}
