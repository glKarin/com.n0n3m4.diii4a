// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "Camera.h"
#include "Player.h"

/*
===============================================================================

  idCamera

  Base class for cameras

===============================================================================
*/

ABSTRACT_DECLARATION( idEntity, idCamera )
END_CLASS

/*
=====================
idCamera::Spawn
=====================
*/
void idCamera::Spawn( void ) {
}

/*
=====================
idCamera::Spawn
=====================
*/
void idCamera::InitRenderView( void ) {
	memset( &renderView, 0, sizeof( renderView ) );

	renderView.vieworg = GetPhysics()->GetOrigin();
	renderView.fov_x = 120;
	renderView.fov_y = 120;
	renderView.viewaxis = GetPhysics()->GetAxis();

	// copy global shader parms
	// Gordon: these are never used
	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		renderView.shaderParms[ i ] = gameLocal.globalShaderParms[ i ];
	}

	renderView.globalMaterial = gameLocal.GetGlobalMaterial();

	renderView.time = gameLocal.time;
}

/*
=====================
idCamera::GetRenderView
=====================
*/
renderView_t *idCamera::GetRenderView() {
	InitRenderView();

	GetViewParms( &renderView );
	
	return &renderView;
}

/*
===============================================================================

idCamera_MD5

===============================================================================
*/

/*
================
idCamera_MD5::idCamera_MD5
================
*/
idCamera_MD5::idCamera_MD5( void ) {
	offset.Zero();
	frameRate = 0;
	cycle = 1;
	starttime = 0;
}

/*
================
idCamera_MD5::~idCamera_MD5
================
*/
idCamera_MD5::~idCamera_MD5( void ) {
}

/*
================
idCamera_MD5::LoadAnim
================
*/
bool idCamera_MD5::LoadAnim( const char* filename ) {
	int			version;
	idLexer		parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT );
	idToken		token;
	int			numFrames;
	int			numCuts;
	int			i;
	idStr		md5FileName;

	md5FileName = filename;
	md5FileName.SetFileExtension( MD5_CAMERA_EXT );
	if ( !parser.LoadFile( md5FileName ) ) {
		gameLocal.Warning( "Failed to open '%s'", md5FileName.c_str() );
		return false;
	}

	cameraCuts.Clear();
	cameraCuts.SetGranularity( 1 );
	camera.Clear();
	camera.SetGranularity( 1 );

	parser.ExpectTokenString( MD5_VERSION_STRING );
	version = parser.ParseInt();
	if ( version != MD5_VERSION ) {
		parser.Error( "Invalid version %d.  Should be version %d", version, MD5_VERSION );
		return false;
	}

	// skip the commandline
	parser.ExpectTokenString( "commandline" );
	parser.ReadToken( &token );

	// parse num frames
	parser.ExpectTokenString( "numFrames" );
	numFrames = parser.ParseInt();
	if ( numFrames <= 0 ) {
		parser.Error( "Invalid number of frames: %d", numFrames );
		return false;
	}

	// parse framerate
	parser.ExpectTokenString( "frameRate" );
	frameRate = parser.ParseInt();
	if ( frameRate <= 0 ) {
		parser.Error( "Invalid framerate: %d", frameRate );
		return false;
	}

	// parse num cuts
	parser.ExpectTokenString( "numCuts" );
	numCuts = parser.ParseInt();
	if ( ( numCuts < 0 ) || ( numCuts > numFrames ) ) {
		parser.Error( "Invalid number of camera cuts: %d", numCuts );
		return false;
	}

	// parse the camera cuts
	parser.ExpectTokenString( "cuts" );
	parser.ExpectTokenString( "{" );
	cameraCuts.SetNum( numCuts );
	for( i = 0; i < numCuts; i++ ) {
		cameraCuts[ i ] = parser.ParseInt();
		if ( ( cameraCuts[ i ] < 1 ) || ( cameraCuts[ i ] >= numFrames ) ) {
			parser.Error( "Invalid camera cut" );
			return false;
		}
	}
	parser.ExpectTokenString( "}" );

	// parse the camera frames
	parser.ExpectTokenString( "camera" );
	parser.ExpectTokenString( "{" );
	camera.SetNum( numFrames );
	for( i = 0; i < numFrames; i++ ) {
		parser.Parse1DMatrix( 3, camera[ i ].t.ToFloatPtr() );
		parser.Parse1DMatrix( 3, camera[ i ].q.ToFloatPtr() );
		camera[ i ].fov = parser.ParseFloat();
	}
	parser.ExpectTokenString( "}" );

#if 0
	if ( !gameLocal.GetLocalPlayer() ) {
		return true;
	}

	idDebugGraph gGraph;
	idDebugGraph tGraph;
	idDebugGraph qGraph;
	idDebugGraph dtGraph;
	idDebugGraph dqGraph;
	gGraph.SetNumSamples( numFrames );
	tGraph.SetNumSamples( numFrames );
	qGraph.SetNumSamples( numFrames );
	dtGraph.SetNumSamples( numFrames );
	dqGraph.SetNumSamples( numFrames );

	gameLocal.Printf( "\n\ndelta vec:\n" );
	float diff_t, last_t, t;
	float diff_q, last_q, q;
	diff_t = last_t = 0.0f;
	diff_q = last_q = 0.0f;
	for( i = 1; i < numFrames; i++ ) {
		t = ( camera[ i ].t - camera[ i - 1 ].t ).Length();
		q = ( camera[ i ].q.ToQuat() - camera[ i - 1 ].q.ToQuat() ).Length();
		diff_t = t - last_t;
		diff_q = q - last_q;
		gGraph.AddValue( ( i % 10 ) == 0 );
		tGraph.AddValue( t );
		qGraph.AddValue( q );
		dtGraph.AddValue( diff_t );
		dqGraph.AddValue( diff_q );

		gameLocal.Printf( "%d: %.8f  :  %.8f,     %.8f  :  %.8f\n", i, t, diff_t, q, diff_q  );
		last_t = t;
		last_q = q;
	}

	gGraph.Draw( colorBlue, 300.0f );
	tGraph.Draw( colorOrange, 60.0f );
	dtGraph.Draw( colorYellow, 6000.0f );
	qGraph.Draw( colorGreen, 60.0f );
	dqGraph.Draw( colorCyan, 6000.0f );
#endif

	return true;
}

/*
================
idCamera_MD5::SkipToEnd
================
*/
bool idCamera_MD5::SkipToEnd() {
	if ( camera.Num() < 2 ) {
		// 1 frame anims never end
		return false;
	}

	int frame;
	int frameTime;

	if ( frameRate == USERCMD_HZ ) {
		frameTime	= gameLocal.time - starttime;
		frame		= frameTime / gameLocal.msec;
	} else {
		frameTime	= ( gameLocal.time - starttime ) * frameRate;
		frame		= frameTime / 1000;
	}
	
	if ( frame > camera.Num() + cameraCuts.Num() - 2 ) {
		if ( cycle > 0 ) {
			cycle--;
		}

		if ( cycle != 0 ) {
			// advance start time so that we loop
			starttime += ( ( camera.Num() - cameraCuts.Num() ) * 1000 ) / frameRate;
		} else {
			return true;
		}
	}
	return false;
}

/*
================
idCamera_MD5::Evaluate
================
*/
bool idCamera_MD5::Evaluate( idVec3& origin, idMat3& axis, float& fov, int time ) {
	int				realFrame;
	int				frame;
	int				frameTime;
	float			lerp;
	float			invlerp;
	cameraFrame_t	*camFrame;
	int				i;
	int				cut;
	idQuat			q1, q2, q3;
	bool			stopped = false;

	if ( camera.Num() == 0 ) {
		// we most likely are in the middle of a restore
		// FIXME: it would be better to fix it so this doesn't get called during a restore
		origin = vec3_origin;
		axis = mat3_identity;
		fov = 90.f;
		return stopped;
	}

	if ( frameRate == USERCMD_HZ ) {
		frameTime	= time - starttime;
		frame		= frameTime / gameLocal.msec;
		lerp		= 0.0f;
	} else {
		frameTime	= ( time - starttime ) * frameRate;
		frame		= frameTime / 1000;
		lerp		= ( frameTime % 1000 ) * 0.001f;
	}

	// skip any frames where camera cuts occur
	realFrame = frame;
	cut = 0;
	for( i = 0; i < cameraCuts.Num(); i++ ) {
		if ( frame < cameraCuts[ i ] ) {
			break;
		}
		frame++;
		cut++;
	}

	if ( g_debugCinematic.GetBool() ) {
		int prevFrameTime	= ( time - starttime - gameLocal.msec ) * frameRate;
		int prevFrame		= prevFrameTime / 1000;
		int prevCut;

		prevCut = 0;
		for( i = 0; i < cameraCuts.Num(); i++ ) {
			if ( prevFrame < cameraCuts[ i ] ) {
				break;
			}
			prevFrame++;
			prevCut++;
		}

		if ( prevCut != cut ) {
			gameLocal.Printf( "%d: cut %d\n", gameLocal.framenum, cut );
		}
	}

	// clamp to the first frame.  also check if this is a one frame anim.  one frame anims would end immediately,
	// but since they're mainly used for static cams anyway, just stay on it infinitely.
	if ( ( frame < 0 ) || ( camera.Num() < 2 ) ) {
		axis = camera[ 0 ].q.ToQuat().ToMat3();
		origin = camera[ 0 ].t + offset;
		fov = camera[ 0 ].fov;
	} else if ( frame > camera.Num() - 2 ) {
		if ( cycle > 0 ) {
			cycle--;
		}

		if ( cycle != 0 ) {
			// advance start time so that we loop
			starttime += ( ( camera.Num() - cameraCuts.Num() ) * 1000 ) / frameRate;
			Evaluate( origin, axis, fov, time );
			return stopped;
		}

		stopped = true;

		// use our last frame
		camFrame = &camera[ camera.Num() - 1 ];
		axis = camFrame->q.ToQuat().ToMat3();
		origin = camFrame->t + offset;
		fov = camFrame->fov;
	} else if ( lerp == 0.0f ) {
		camFrame = &camera[ frame ];
		axis = camFrame[ 0 ].q.ToMat3();
		origin = camFrame[ 0 ].t + offset;
		fov = camFrame[ 0 ].fov;
	} else {
		camFrame = &camera[ frame ];
		invlerp = 1.0f - lerp;
		q1 = camFrame[ 0 ].q.ToQuat();
		q2 = camFrame[ 1 ].q.ToQuat();
		q3.Slerp( q1, q2, lerp );
		axis = q3.ToMat3();
		origin = camFrame[ 0 ].t * invlerp + camFrame[ 1 ].t * lerp + offset;
		fov = camFrame[ 0 ].fov * invlerp + camFrame[ 1 ].fov * lerp;
	}

#if 0
	static int lastFrame = 0;
	static idVec3 lastFrameVec( 0.0f, 0.0f, 0.0f );
	if ( time != lastFrame ) {
		gameRenderWorld->DebugBounds( colorCyan, idBounds( view->vieworg ).Expand( 16.0f ), vec3_origin, gameLocal.msec );
		gameRenderWorld->DebugLine( colorRed, view->vieworg, view->vieworg + idVec3( 0.0f, 0.0f, 2.0f ), 10000, false );
		gameRenderWorld->DebugLine( colorCyan, lastFrameVec, view->vieworg, 10000, false );
		gameRenderWorld->DebugLine( colorYellow, view->vieworg + view->viewaxis[ 0 ] * 64.0f, view->vieworg + view->viewaxis[ 0 ] * 66.0f, 10000, false );
		gameRenderWorld->DebugLine( colorOrange, view->vieworg + view->viewaxis[ 0 ] * 64.0f, view->vieworg + view->viewaxis[ 0 ] * 64.0f + idVec3( 0.0f, 0.0f, 2.0f ), 10000, false );
		lastFrameVec = view->vieworg;
		lastFrame = time;
	}
#endif

	if ( g_showcamerainfo.GetBool() ) {
		gameLocal.Printf( "^5Frame: ^7%d/%d\n\n\n", realFrame + 1, camera.Num() - cameraCuts.Num() );
	}

	return stopped;
}


/*
===============================================================================

sdCamera_Placement

===============================================================================
*/


CLASS_DECLARATION( idCamera, sdCamera_Placement )
END_CLASS



/*
============
sdCamera_Placement::Spawn
============
*/
void sdCamera_Placement::Spawn() {
	fov = spawnArgs.GetFloat( "fov", "90" );
}

/*
============
sdCamera_Placement::GetViewParms
============
*/
void sdCamera_Placement::GetViewParms( renderView_t* view ) {
	assert( view );
	view->vieworg = GetPhysics()->GetOrigin();
	view->viewaxis = GetPhysics()->GetAxis();
	gameLocal.CalcFov( fov, view->fov_x, view->fov_y );
}
