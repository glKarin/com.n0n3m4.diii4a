/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop


#include "renderer/resources/Image.h"
#include "Game_local.h"

static int MakePowerOfTwo( int num ) {
	int		pot;

	for (pot = 1 ; pot < num ; pot<<=1) {}

	return pot;
}

const int IMPULSE_DELAY = 150;

/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView() :
m_postProcessManager()			// Invoke the postprocess Manager Constructor - J.C.Denton
{
	memset( screenBlobs, 0, sizeof( screenBlobs ) );
	memset( &view, 0, sizeof( view ) );
	player = NULL;
	dvMaterial = declManager->FindMaterial( "textures/postprocess/doublevision" );
	tunnelMaterial = declManager->FindMaterial( "textures/darkmod/decals/tunnel" );	// damage overlay
	bloodSprayMaterial = declManager->FindMaterial( "textures/decals/bloodspray" );
	lagoMaterial = declManager->FindMaterial( LAGO_MATERIAL, false );

	dvFinishTime = 0;
	kickFinishTime = 0;
	kickAngles.Zero();
	lastDamageTime = 0.0f;
	fadeTime = 0;
	fadeRate = 0.0;
	fadeFromColor.Zero();
	fadeToColor.Zero();
	fadeColor.Zero();
	shakeAng.Zero();

	/*
	fxManager = NULL;

	if ( !fxManager ) {
	fxManager = new FullscreenFXManager;
	fxManager->Initialize( this );
	}
	*/

	ClearEffects();
}

/*
==============
idPlayerView::Save
==============
*/
void idPlayerView::Save( idSaveGame *savefile ) const {
	int i;
	const screenBlob_t *blob;

	blob = &screenBlobs[ 0 ];
	for( i = 0; i < MAX_SCREEN_BLOBS; i++, blob++ ) {
		savefile->WriteMaterial( blob->material );
		savefile->WriteFloat( blob->x );
		savefile->WriteFloat( blob->y );
		savefile->WriteFloat( blob->w );
		savefile->WriteFloat( blob->h );
		savefile->WriteFloat( blob->s1 );
		savefile->WriteFloat( blob->t1 );
		savefile->WriteFloat( blob->s2 );
		savefile->WriteFloat( blob->t2 );
		savefile->WriteInt( blob->finishTime );
		savefile->WriteInt( blob->startFadeTime );
		savefile->WriteFloat( blob->driftAmount );
	}

	savefile->WriteInt( dvFinishTime );
	savefile->WriteMaterial( dvMaterial );
	savefile->WriteInt( kickFinishTime );
	savefile->WriteAngles( kickAngles );

	savefile->WriteMaterial( tunnelMaterial );
	savefile->WriteMaterial( bloodSprayMaterial );
	savefile->WriteFloat( lastDamageTime );

	savefile->WriteVec4( fadeColor );
	savefile->WriteVec4( fadeToColor );
	savefile->WriteVec4( fadeFromColor );
	savefile->WriteFloat( fadeRate );
	savefile->WriteInt( fadeTime );

	savefile->WriteAngles( shakeAng );

	savefile->WriteObject( player );
	savefile->WriteRenderView( view );
}

/*
==============
idPlayerView::Restore
==============
*/
void idPlayerView::Restore( idRestoreGame *savefile ) {
	int i;
	screenBlob_t *blob;

	blob = &screenBlobs[ 0 ];
	for( i = 0; i < MAX_SCREEN_BLOBS; i++, blob++ ) {
		savefile->ReadMaterial( blob->material );
		savefile->ReadFloat( blob->x );
		savefile->ReadFloat( blob->y );
		savefile->ReadFloat( blob->w );
		savefile->ReadFloat( blob->h );
		savefile->ReadFloat( blob->s1 );
		savefile->ReadFloat( blob->t1 );
		savefile->ReadFloat( blob->s2 );
		savefile->ReadFloat( blob->t2 );
		savefile->ReadInt( blob->finishTime );
		savefile->ReadInt( blob->startFadeTime );
		savefile->ReadFloat( blob->driftAmount );
	}

	savefile->ReadInt( dvFinishTime );
	savefile->ReadMaterial( dvMaterial );
	savefile->ReadInt( kickFinishTime );
	savefile->ReadAngles( kickAngles );			

	savefile->ReadMaterial( tunnelMaterial );
	savefile->ReadMaterial( bloodSprayMaterial );
	savefile->ReadFloat( lastDamageTime );

	savefile->ReadVec4( fadeColor );
	savefile->ReadVec4( fadeToColor );
	savefile->ReadVec4( fadeFromColor );
	savefile->ReadFloat( fadeRate );
	savefile->ReadInt( fadeTime );

	savefile->ReadAngles( shakeAng );

	savefile->ReadObject( reinterpret_cast<idClass *&>( player ) );
	savefile->ReadRenderView( view );

	// Re-Initialize the PostProcess Manager.	- JC Denton
//	this->m_postProcessManager.Initialize();
}

/*
==============
idPlayerView::SetPlayerEntity
==============
*/
void idPlayerView::SetPlayerEntity( idPlayer *playerEnt ) {
	player = playerEnt;
}

/*
==============
idPlayerView::ClearEffects
==============
*/
void idPlayerView::ClearEffects() {
	lastDamageTime = MS2SEC( gameLocal.time - 99999 );

	dvFinishTime = ( gameLocal.time - 99999 );
	kickFinishTime = ( gameLocal.time - 99999 );

	for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ ) {
		screenBlobs[i].finishTime = gameLocal.time;
	}

	fadeTime = 0;
}

/*
==============
idPlayerView::GetScreenBlob
==============
*/
screenBlob_t *idPlayerView::GetScreenBlob() {
	screenBlob_t	*oldest = &screenBlobs[0];

	for ( int i = 1 ; i < MAX_SCREEN_BLOBS ; i++ ) {
		if ( screenBlobs[i].finishTime < oldest->finishTime ) {
			oldest = &screenBlobs[i];
		}
	}
	return oldest;
}

/*
==============
idPlayerView::DamageImpulse

LocalKickDir is the direction of force in the player's coordinate system,
which will determine the head kick direction
==============
*/
void idPlayerView::DamageImpulse( idVec3 localKickDir, const idDict *damageDef ) {
	//
	// double vision effect
	//
	if ( lastDamageTime > 0.0f && SEC2MS( lastDamageTime ) + IMPULSE_DELAY > gameLocal.time ) {
		// keep shotgun from obliterating the view
		return;
	}

	float	dvTime = damageDef->GetFloat( "dv_time" );
	if ( dvTime ) {
		if ( dvFinishTime < gameLocal.time ) {
			dvFinishTime = gameLocal.time;
		}
		dvFinishTime += static_cast<int>(g_dvTime.GetFloat() * dvTime);
		// don't let it add up too much in god mode
		if ( dvFinishTime > gameLocal.time + 5000 ) {
			dvFinishTime = gameLocal.time + 5000;
		}
	}

	//
	// head angle kick
	//
	float	kickTime = damageDef->GetFloat( "kick_time" );
	if ( kickTime ) {
		kickFinishTime = gameLocal.time + static_cast<int>(g_kickTime.GetFloat() * kickTime);

		// forward / back kick will pitch view
		kickAngles[0] = localKickDir[0];

		// side kick will yaw view
		kickAngles[1] = localKickDir[1]*0.5f;

		// up / down kick will pitch view
		kickAngles[0] += localKickDir[2];

		// roll will come from  side
		kickAngles[2] = localKickDir[1];

		float kickAmplitude = damageDef->GetFloat( "kick_amplitude" );
		if ( kickAmplitude ) {
			kickAngles *= kickAmplitude;
		}
	}

	//
	// screen blob
	//
	float	blobTime = damageDef->GetFloat( "blob_time" );
	if ( blobTime ) {
		screenBlob_t	*blob = GetScreenBlob();
		blob->startFadeTime = gameLocal.time;
		blob->finishTime = gameLocal.time + static_cast<int>(blobTime * g_blobTime.GetFloat());

		const char *materialName = damageDef->GetString( "mtr_blob" );
		blob->material = declManager->FindMaterial( materialName );
		blob->x = damageDef->GetFloat( "blob_x" );
		blob->x += ( gameLocal.random.RandomInt()&63 ) - 32;
		blob->y = damageDef->GetFloat( "blob_y" );
		blob->y += ( gameLocal.random.RandomInt()&63 ) - 32;

		float scale = ( 256 + ( ( gameLocal.random.RandomInt()&63 ) - 32 ) ) / 256.0f;
		blob->w = damageDef->GetFloat( "blob_width" ) * g_blobSize.GetFloat() * scale;
		blob->h = damageDef->GetFloat( "blob_height" ) * g_blobSize.GetFloat() * scale;
		blob->s1 = 0;
		blob->t1 = 0;
		blob->s2 = 1;
		blob->t2 = 1;
	}

	//
	// save lastDamageTime for tunnel vision accentuation
	//
	lastDamageTime = MS2SEC( gameLocal.time );

}

/*
==================
idPlayerView::AddBloodSpray

If we need a more generic way to add blobs then we can do that
but having it localized here lets the material be pre-looked up etc.
==================
*/
void idPlayerView::AddBloodSpray( float duration ) {
	/*
	if ( duration <= 0 || bloodSprayMaterial == NULL || g_skipViewEffects.GetBool() ) {
	return;
	}
	// visit this for chainsaw
	screenBlob_t *blob = GetScreenBlob();
	blob->startFadeTime = gameLocal.time;
	blob->finishTime = gameLocal.time + ( duration * 1000 );
	blob->material = bloodSprayMaterial;
	blob->x = ( gameLocal.random.RandomInt() & 63 ) - 32;
	blob->y = ( gameLocal.random.RandomInt() & 63 ) - 32;
	blob->driftAmount = 0.5f + gameLocal.random.CRandomFloat() * 0.5;
	float scale = ( 256 + ( ( gameLocal.random.RandomInt()&63 ) - 32 ) ) / 256.0f;
	blob->w = 600 * g_blobSize.GetFloat() * scale;
	blob->h = 480 * g_blobSize.GetFloat() * scale;
	float s1 = 0.0f;
	float t1 = 0.0f;
	float s2 = 1.0f;
	float t2 = 1.0f;
	if ( blob->driftAmount < 0.6 ) {
	s1 = 1.0f;
	s2 = 0.0f;
	} else if ( blob->driftAmount < 0.75 ) {
	t1 = 1.0f;
	t2 = 0.0f;
	} else if ( blob->driftAmount < 0.85 ) {
	s1 = 1.0f;
	s2 = 0.0f;
	t1 = 1.0f;
	t2 = 0.0f;
	}
	blob->s1 = s1;
	blob->t1 = t1;
	blob->s2 = s2;
	blob->t2 = t2;
	*/
}

/*
==================
idPlayerView::WeaponFireFeedback

Called when a weapon fires, generates head twitches, etc
==================
*/
void idPlayerView::WeaponFireFeedback( const idDict *weaponDef ) {
	int		recoilTime;

	recoilTime = weaponDef->GetInt( "recoilTime" );
	// don't shorten a damage kick in progress
	if ( recoilTime && kickFinishTime < gameLocal.time ) {
		idAngles angles;
		weaponDef->GetAngles( "recoilAngles", "5 0 0", angles );
		kickAngles = angles;
		int	finish = gameLocal.time + static_cast<int>(g_kickTime.GetFloat() * recoilTime);
		kickFinishTime = finish;
	}	

}

/*
===================
idPlayerView::CalculateShake
===================
*/
void idPlayerView::CalculateShake() {
	float shakeVolume = gameSoundWorld->CurrentShakeAmplitudeForPosition( gameLocal.time, player->firstPersonViewOrigin );
	//
	// shakeVolume should somehow be molded into an angle here
	// it should be thought of as being in the range 0.0 -> 1.0, although
	// since CurrentShakeAmplitudeForPosition() returns all the shake sounds
	// the player can hear, it can go over 1.0 too.
	//
	shakeAng[0] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAng[1] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAng[2] = gameLocal.random.CRandomFloat() * shakeVolume;
}

/*
===================
idPlayerView::ShakeAxis
===================
*/
idMat3 idPlayerView::ShakeAxis() const {
	return shakeAng.ToMat3();
}

/*
===================
idPlayerView::AngleOffset

kickVector, a world space direction that the attack should 
===================
*/
idAngles idPlayerView::AngleOffset() const {
	idAngles	ang;

	ang.Zero();

	if ( gameLocal.time < kickFinishTime ) {
		float offset = kickFinishTime - gameLocal.time;

		ang = kickAngles * offset * offset * g_kickAmplitude.GetFloat();

		for ( int i = 0 ; i < 3 ; i++ ) {
			if ( ang[i] > 70.0f ) {
				ang[i] = 70.0f;
			} else if ( ang[i] < -70.0f ) {
				ang[i] = -70.0f;
			}
		}
	}
	return ang;
}

/*
==================
idPlayerView::SingleView
==================
*/
void idPlayerView::SingleView( idUserInterface *hud, const renderView_t *view, bool drawHUD ) {

	// normal rendering
	if ( !view ) {
		return;
	}

	// place the sound origin for the player
	// TODO: Support overriding the location area so that reverb settings can be applied for listening thru doors?
	idVec3 p = player->GetPrimaryListenerLoc(); // grayman #4882

	const idLocationEntity* currentLocation = player->GetLocation();
	idStr efxPreset;

	// #6273 The map might not be using the location system
	if (currentLocation != NULL) {
		efxPreset = currentLocation->spawnArgs.GetString("efx_preset");
	}

	gameSoundWorld->PlaceListener( p, view->viewaxis, player->entityNumber + 1, gameLocal.time, hud ? hud->State().GetString( "location" ) : "Undefined", efxPreset ); // grayman #4882
//	gameSoundWorld->PlaceListener(player->GetListenerLoc(), view->viewaxis, player->entityNumber + 1, gameLocal.time, hud ? hud->State().GetString("location") : "Undefined");

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t	hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

	//gameRenderWorld->RenderScene( &hackedView );

	// grayman #3108 - contributed by neuro & 7318
	idVec3 diff, currentEyePos, PSOrigin, Zero;
	
	Zero.Zero();
		
	if ( ( gameLocal.CheckGlobalPortalSky() ) || ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_LOCAL ) ) {		
		// in a case of a moving portalSky
		
		currentEyePos = hackedView.vieworg;
		
		if ( gameLocal.playerOldEyePos == Zero ) {
			// Initialize playerOldEyePos. This will only happen in one tick.
			gameLocal.playerOldEyePos = currentEyePos;
		}

		diff = ( currentEyePos - gameLocal.playerOldEyePos) / gameLocal.portalSkyScale;
		gameLocal.portalSkyGlobalOrigin += diff; // This is for the global portalSky.
												 // It should keep going even when not active.
	}

	if ( gameLocal.portalSkyEnt.GetEntity() && g_enablePortalSky.GetInteger() && ( gameLocal.IsPortalSkyActive() || g_stopTime.GetBool() ) ) {
		
		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_STANDARD ) {
			PSOrigin = gameLocal.portalSkyOrigin;
		}
		
		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_GLOBAL ) {
			PSOrigin = gameLocal.portalSkyGlobalOrigin;
		}
		
		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_LOCAL ) {
			gameLocal.portalSkyOrigin += diff;
			PSOrigin = gameLocal.portalSkyOrigin;
		}
	
		gameLocal.playerOldEyePos = currentEyePos;
		// end neuro & 7318

		renderView_t portalView = hackedView;

		portalView.vieworg = PSOrigin;	// grayman #3108 - contributed by neuro & 7318
//		portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();
		portalView.viewaxis = portalView.viewaxis * gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetAxis();

		// setup global fixup projection vars
		if ( 1 ) {
			int vidWidth, vidHeight;
			idVec2 shiftScale;

			renderSystem->GetGLSettings( vidWidth, vidHeight );

			float pot;
			int	 w = vidWidth;
			pot = w;// MakePowerOfTwo( w );
			shiftScale.x = (float)w / pot;

			int	 h = vidHeight;
			pot = h;// MakePowerOfTwo( h );
			shiftScale.y = (float)h / pot;

			hackedView.shaderParms[6] = shiftScale.x; // grayman #3108 - neuro used [4], we use [6]
			hackedView.shaderParms[7] = shiftScale.y; // grayman #3108 - neuro used [5], we use [7]
		}

		/*gameRenderWorld->RenderScene( &portalView );
		if (g_enablePortalSky.GetInteger() == 1) // duzenko #4414 - the new method will use the left-over pixels in framebuffer
			renderSystem->CaptureRenderToImage( "_currentRender" );*/

		hackedView.forceUpdate = true;				// FIX: for smoke particles not drawing when portalSky present
	}
	else // grayman #3108 - contributed by 7318 
	{
		// So if g_enablePortalSky is disabled, GlobalPortalSkies doesn't break.
		// When g_enablePortalSky gets re-enabled, GlobalPortalSkies keeps working. 
		gameLocal.playerOldEyePos = currentEyePos;
	}

	hackedView.forceUpdate = true; // Fix for lightgem problems? -Gildoran
	gameRenderWorld->RenderScene( hackedView );
	// process the frame

	//	fxManager->Process( &hackedView );

	if ( player->spectating ) {
		return;
	}

	// draw screen blobs
	if ( !pm_thirdPerson.GetBool() && !g_skipViewEffects.GetBool() ) {
		for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ ) {
			screenBlob_t	*blob = &screenBlobs[i];
			if ( blob->finishTime <= gameLocal.time ) {
				continue;
			}

			blob->y += blob->driftAmount;

			float	fade = (float)( blob->finishTime - gameLocal.time ) / ( blob->finishTime - blob->startFadeTime );
			if ( fade > 1.0f ) {
				fade = 1.0f;
			}
			if ( fade ) {
				renderSystem->SetColor4( 1,1,1,fade );
				renderSystem->DrawStretchPic( blob->x, blob->y, blob->w, blob->h,blob->s1, blob->t1, blob->s2, blob->t2, blob->material );
			}
		}
		/*if (drawHUD)
		{
			player->DrawHUD( hud );
		}*/

		// tunnel vision

		// grayman - This is where the red screen overlay is
		// applied for player damage. The red overlay's alpha is
		// the player's current health divided by his max health.
		// Less health when damaged means more overlay is visible.
		// The amount of alpha is also used to determine how long
		// the overlay is visible, its alpha climbing to 1.0
		// over the duration of the effect. Less health means a longer
		// duration. Some key words for search purposes:
		// damage dealt, damage hud, hurt hud, blood overlay

		float health = 0.0f;
		if ( g_testHealthVision.GetFloat() != 0.0f )
		{
			health = g_testHealthVision.GetFloat();
		}
		else
		{
			health = player->health;
		}

		float alpha = health / 100.0f;
		if ( alpha < 0.0f )
		{
			alpha = 0.0f;
		}
		else if ( alpha > 1.0f )
		{
			alpha = 1.0f;
		}

		if ( alpha < 1.0f  )
		{
			// Tels: parm0: when the last damage occured
			// Tels: parm1: TODO: set here f.i. to color the material different when in gas cloud
			// Tels: parm2: TODO: set here f.i. to color the material different when poisoned
			// Tels: parm3: alpha value, depending on health
			renderSystem->SetColor4( ( player->health <= 0.0f ) ? MS2SEC( gameLocal.time ) : lastDamageTime, 1.0f, 1.0f, ( player->health <= 0.0f ) ? 0.0f : alpha );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, tunnelMaterial );
		}
	}

	// Rotoscope (Cartoon-like) rendering - (Rotoscope Shader v1.0 by Hellborg) - added by Dram
	if ( g_rotoscope.GetBool() ) {
		const idMaterial *mtr = declManager->FindMaterial( "textures/postprocess/rotoedge", false );
		if ( !mtr ) {
			common->Printf( "Rotoscope material not found.\n" );
		} else {
			renderSystem->CaptureRenderToImage( *globalImages->currentRenderImage );
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}

	// test a single material drawn over everything
	if ( g_testPostProcess.GetString()[0] ) {
		const idMaterial *mtr = declManager->FindMaterial( g_testPostProcess.GetString(), false );
		if ( !mtr ) {
			common->Printf( "Material not found.\n" );
			g_testPostProcess.SetString( "" );
		} else {
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}
}

// grayman #4882 - PeekView temporarily sets the player view on the far side of an opening

/*
==================
idPlayerView::PeekView
==================
*/
void idPlayerView::PeekView(const renderView_t *view)
{
	// normal rendering
	if ( !view ) {
		return;
	}

	renderView_t hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

	// grayman #3108 - contributed by neuro & 7318
	idVec3 diff, currentEyePos, PSOrigin, Zero;

	Zero.Zero();

	if ( (gameLocal.CheckGlobalPortalSky()) || (gameLocal.GetCurrentPortalSkyType() == PORTALSKY_LOCAL) ) {
		// in a case of a moving portalSky

		currentEyePos = hackedView.vieworg;

		if ( gameLocal.playerOldEyePos == Zero ) {
			// Initialize playerOldEyePos. This will only happen in one tick.
			gameLocal.playerOldEyePos = currentEyePos;
		}

		diff = (currentEyePos - gameLocal.playerOldEyePos) / gameLocal.portalSkyScale;
		gameLocal.portalSkyGlobalOrigin += diff; // This is for the global portalSky.
												 // It should keep going even when not active.
	}

	if ( gameLocal.portalSkyEnt.GetEntity() && gameLocal.IsPortalSkyActive() && g_enablePortalSky.GetInteger() ) {

		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_STANDARD ) {
			PSOrigin = gameLocal.portalSkyOrigin;
		}

		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_GLOBAL ) {
			PSOrigin = gameLocal.portalSkyGlobalOrigin;
		}

		if ( gameLocal.GetCurrentPortalSkyType() == PORTALSKY_LOCAL ) {
			gameLocal.portalSkyOrigin += diff;
			PSOrigin = gameLocal.portalSkyOrigin;
		}

		gameLocal.playerOldEyePos = currentEyePos;
		// end neuro & 7318

		renderView_t portalView = hackedView;

		portalView.vieworg = PSOrigin;	// grayman #3108 - contributed by neuro & 7318
		// portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();
		portalView.viewaxis = portalView.viewaxis * gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetAxis();

		// setup global fixup projection vars
		if ( 1 ) {
			int vidWidth, vidHeight;
			idVec2 shiftScale;

			renderSystem->GetGLSettings(vidWidth, vidHeight);

			float pot;
			int	 w = vidWidth;
			pot = w;// MakePowerOfTwo( w );
			shiftScale.x = (float)w / pot;

			int	 h = vidHeight;
			pot = h;// MakePowerOfTwo( h );
			shiftScale.y = (float)h / pot;

			hackedView.shaderParms[6] = shiftScale.x; // grayman #3108 - neuro used [4], we use [6]
			hackedView.shaderParms[7] = shiftScale.y; // grayman #3108 - neuro used [5], we use [7]
		}

		hackedView.forceUpdate = true;				// FIX: for smoke particles not drawing when portalSky present
	}
	else // grayman #3108 - contributed by 7318 
	{
		// So if g_enablePortalSky is disabled, GlobalPortalSkies doesn't break.
		// When g_enablePortalSky gets re-enabled, GlobalPortalSkies keeps working. 
		gameLocal.playerOldEyePos = currentEyePos;
	}

	hackedView.forceUpdate = true; // Fix for lightgem problems? -Gildoran
	if ( g_enablePortalSky.GetInteger() != -1 ) // duzenko #4414: debug tool to present the skybox stage result
		gameRenderWorld->RenderScene(hackedView);
	// process the frame

	if ( player->spectating ) {
		return;
	}

	// draw screen blobs
	if ( !pm_thirdPerson.GetBool() && !g_skipViewEffects.GetBool() ) {
		for ( int i = 0; i < MAX_SCREEN_BLOBS; i++ ) {
			screenBlob_t	*blob = &screenBlobs[i];
			if ( blob->finishTime <= gameLocal.time ) {
				continue;
			}

			blob->y += blob->driftAmount;

			float	fade = (float)(blob->finishTime - gameLocal.time) / (blob->finishTime - blob->startFadeTime);
			if ( fade > 1.0f ) {
				fade = 1.0f;
			}
			if ( fade ) {
				renderSystem->SetColor4(1, 1, 1, fade);
				renderSystem->DrawStretchPic(blob->x, blob->y, blob->w, blob->h, blob->s1, blob->t1, blob->s2, blob->t2, blob->material);
			}
		}
	}

	// Rotoscope (Cartoon-like) rendering - (Rotoscope Shader v1.0 by Hellborg) - added by Dram
	if ( g_rotoscope.GetBool() ) {
		const idMaterial *mtr = declManager->FindMaterial("textures/postprocess/rotoedge", false);
		if ( !mtr ) {
			common->Printf("Rotoscope material not found.\n");
		}
		else {
			renderSystem->CaptureRenderToImage(*globalImages->currentRenderImage);
			renderSystem->SetColor4(1.0f, 1.0f, 1.0f, 1.0f);
			renderSystem->DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, mtr);
		}
	}

	// test a single material drawn over everything
	if ( g_testPostProcess.GetString()[0] ) {
		const idMaterial *mtr = declManager->FindMaterial(g_testPostProcess.GetString(), false);
		if ( !mtr ) {
			common->Printf("Material not found.\n");
			g_testPostProcess.SetString("");
		}
		else {
			renderSystem->SetColor4(1.0f, 1.0f, 1.0f, 1.0f);
			renderSystem->DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, mtr);
		}
	}
}

/*
===================
idPlayerView::DoubleVision
===================
*/
void idPlayerView::DoubleVision( idUserInterface *hud, const renderView_t *view, int offset ) {

	if ( !g_doubleVision.GetBool() ) {
		SingleView( hud, view );
		return;
	}

	float	scale = offset * g_dvAmplitude.GetFloat();
	if ( scale > 0.5f ) {
		scale = 0.5f;
	}
	float shift = scale * sin( sqrt( (float)offset ) * g_dvFrequency.GetFloat() );
	shift = fabs( shift );

	// if double vision, render to a texture
	renderSystem->CropRenderSize( SCREEN_WIDTH, SCREEN_HEIGHT, true );

	// greebo: Draw the single view, but skip the HUD, this is done later
	SingleView( hud, view, false ); 

	renderSystem->CaptureRenderToImage( *globalImages->scratchImage );
	renderSystem->UnCrop();

#if 0
	// take dvMaterial = _scratch image and render it twice with small shift
	// blend to renders in 50 : 50 proportion
	idVec3 color(1, 1, 1);
	renderSystem->SetColor4( color.x, color.y, color.z, 1.0f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, shift, 1-shift, 1, 0, dvMaterial );
	renderSystem->SetColor4( color.x, color.y, color.z, 0.5f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1-shift, shift, dvMaterial );
#else
	// stgatilov: render whole-screen quad with custom post-processing shader
	// shift is passed in parm0
	renderSystem->SetColor4( shift, 0.0f, 0.0f, 0.0f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, dvMaterial );
#endif

	// Do not post-process the HUD - JC Denton
	// Bloom related - added by Dram
	// 	if ( r_bloom_hud.GetBool() || !r_bloom.GetBool() ) // If HUD blooming is enabled or bloom is disabled
	// 	{
	// 		player->DrawHUD(hud);
	// 	}
}

/*
===================
idPlayerView::BerserkVision
===================
*/
void idPlayerView::BerserkVision( idUserInterface *hud, const renderView_t *view ) {
	renderSystem->CropRenderSize( 512, 256, true );
	SingleView( hud, view );
	renderSystem->CaptureRenderToImage( *globalImages->scratchImage );
	renderSystem->UnCrop();
	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, dvMaterial );
}


/*
=================
idPlayerView::Flash

flashes the player view with the given color
=================
*/
void idPlayerView::Flash(idVec4 color, int time ) {
	Fade(idVec4(0, 0, 0, 0), time);
	fadeFromColor = colorWhite;
}

/*
=================
idPlayerView::Fade

used for level transition fades
assumes: color.w is 0 or 1
=================
*/
void idPlayerView::Fade( idVec4 color, int time ) {

	if ( !fadeTime ) {
		fadeFromColor.Set( 0.0f, 0.0f, 0.0f, 1.0f - color[ 3 ] );
	} else {
		fadeFromColor = fadeColor;
	}
	fadeToColor = color;

	if ( time <= 0 ) {
		fadeRate = 0;
		time = 0;
		fadeColor = fadeToColor;
	} else {
		fadeRate = 1.0f / ( float )time;
	}

	if ( gameLocal.realClientTime == 0 && time == 0 ) {
		fadeTime = 1;
	} else {
		fadeTime = gameLocal.realClientTime + time;
	}
}

/*
=================
idPlayerView::ScreenFade
=================
*/
void idPlayerView::ScreenFade() {
	int		msec;
	float	t;

	if ( !fadeTime ) {
		return;
	}

	msec = fadeTime - gameLocal.realClientTime;

	if ( msec <= 0 ) {
		fadeColor = fadeToColor;
		if ( fadeColor[ 3 ] == 0.0f ) {
			fadeTime = 0;
		}
	} else {
		t = ( float )msec * fadeRate;
		fadeColor = fadeFromColor * t + fadeToColor * ( 1.0f - t );
	}

	if ( fadeColor[ 3 ] != 0.0f ) {
		renderSystem->SetColor4( fadeColor[ 0 ], fadeColor[ 1 ], fadeColor[ 2 ], fadeColor[ 3 ] );
		renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
	}
}

/*
===================
idPlayerView::InfluenceVision
===================
*/
void idPlayerView::InfluenceVision( idUserInterface *hud, const renderView_t *view ) {

	float distance = 0.0f;
	float pct = 1.0f;
	if ( player->GetInfluenceEntity() ) {
		distance = ( player->GetInfluenceEntity()->GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin() ).Length();
		if ( player->GetInfluenceRadius() != 0.0f && distance < player->GetInfluenceRadius() ) {
			pct = distance / player->GetInfluenceRadius();
			pct = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, pct );
		}
	}
	if ( player->GetInfluenceMaterial() ) {
		SingleView( hud, view );
		renderSystem->CaptureRenderToImage( *globalImages->currentRenderImage );

		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, pct );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, player->GetInfluenceMaterial() );
	} else if ( player->GetInfluenceEntity() == NULL ) {
		SingleView( hud, view );
		return;
	} else {
		int offset =  static_cast<int>(25 + sin(static_cast<float>(gameLocal.time)));
		DoubleVision( hud, view, static_cast<int>(pct * offset) );
	}
}

/*
===================
idPlayerView::RenderPlayerView
===================
*/
void idPlayerView::RenderPlayerView( idUserInterface *hud )
{
	const renderView_t *view = player->GetRenderView();

	if(g_skipViewEffects.GetBool())
	{
		if ( player->usePeekView )
		{
			PeekView(view);
		}
		else
		{
			SingleView(hud, view);
		}
	} else {

		/*if ( player->GetInfluenceMaterial() || player->GetInfluenceEntity() ) {
		InfluenceVision( hud, view );
		} else if ( gameLocal.time < dvFinishTime ) {
		DoubleVision( hud, view, dvFinishTime - gameLocal.time );
		} else {*/

		// greebo: For underwater effects, use the Doom3 Doubleview
		if (player->GetPlayerPhysics()->GetWaterLevel() >= WATERLEVEL_HEAD)
		{
			DoubleVision(hud, view, cv_tdm_underwater_blur.GetInteger());
		}
		else
		{
			// Do not postprocess the HUD
			// 			if ( r_bloom_hud.GetBool() || !r_bloom.GetBool() ) // If HUD blooming is enabled or bloom is disabled
			// 			{
			// 				SingleView( hud, view );
			// 			}
			// 			else
			{
				if ( player->usePeekView )
				{
					PeekView(view);
				}
				else
				{
					SingleView(hud, view);
				}
			}
		}
		//}

		// Bloom related - J.C.Denton
		/* Update  post-process */
		this->m_postProcessManager.Update();

		ScreenFade();
	}

	player->DrawHUD(hud);
}

void idPlayerView::OnReloadImages()
{
//	m_postProcessManager.ScheduleCookedDataUpdate();
}

void idPlayerView::OnVidRestart()
{
//	m_postProcessManager.ScheduleCookedDataUpdate();
}

/*
===================
idPlayerView::dnPostProcessManager Class Definitions - JC Denton
===================
*/

idPlayerView::dnPostProcessManager::dnPostProcessManager()
	: m_ImageAnisotropyHandle (-1)
{
	/*m_iScreenHeight = m_iScreenWidth = 0;
	m_iScreenHeightPowOf2 = m_iScreenWidthPowOf2 = 0;
	m_nFramesToUpdateCookedData = 0;8*/
}

idPlayerView::dnPostProcessManager::~dnPostProcessManager()
{
}

void idPlayerView::dnPostProcessManager::Update( void )
{
	// duzenko: do bloom in the back renderer
	renderSystem->PostProcess(); 
}
