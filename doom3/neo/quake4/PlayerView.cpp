// RAVEN BEGIN
// bdube: note that this file is no longer merged with Doom3 updates
//
// MERGE_DATE 07/07/2004

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#if defined(_XENON) && (defined(_PROFILE) || defined(_DEBUG))
#include "../sys/xenon/ProfilingSupport.h"
#endif

const int IMPULSE_DELAY = 150;
/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView() {
	memset( screenBlobs, 0, sizeof( screenBlobs ) );
	memset( &view, 0, sizeof( view ) );
	player = NULL;
	dvMaterial = NULL;
	dvMaterialBlend = NULL;
	tunnelMaterial = NULL;
	armorMaterial = NULL;
	bloodSprayMaterial = NULL;
	bfgVision = false;
	dvFinishTime = 0;
	tvFinishTime = 0;
	tvStartTime = 0;
	kickFinishTime = 0;
	kickAngles.Zero();
	lastDamageTime = 0.0f;
	fadeTime = 0;
	fadeRate = 0.0;
	fadeFromColor.Zero();
	fadeToColor.Zero();
	fadeColor.Zero();

	ClearEffects();

	dvScale = 1.0f;
	shakeScale = 1.0f;
	tvScale = 1.0f;
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

	savefile->WriteMaterial( dvMaterialBlend );	// cnicholson: Added unsaved var

	savefile->WriteFloat ( dvScale );

	savefile->WriteInt( kickFinishTime );
	savefile->WriteAngles( kickAngles );

	savefile->WriteBool( bfgVision );

	savefile->WriteMaterial( tunnelMaterial );
	savefile->WriteMaterial( armorMaterial );

	savefile->WriteMaterial( bloodSprayMaterial );

	savefile->WriteFloat( lastDamageTime );

	savefile->WriteFloat( shakeFinishTime );
	savefile->WriteFloat( shakeScale );
	savefile->WriteFloat( tvScale );
	savefile->WriteInt( tvFinishTime );
	savefile->WriteInt( tvStartTime );

	savefile->WriteVec4( fadeColor );
	savefile->WriteVec4( fadeToColor );
	savefile->WriteVec4( fadeFromColor );
	savefile->WriteFloat( fadeRate );
	savefile->WriteInt( fadeTime );

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

	savefile->ReadMaterial( dvMaterialBlend );	// cnicholson: Added unrestored var

	savefile->ReadFloat( dvScale );

	savefile->ReadInt( kickFinishTime );
	savefile->ReadAngles( kickAngles );			

	savefile->ReadBool( bfgVision );

	savefile->ReadMaterial( tunnelMaterial );
	savefile->ReadMaterial( armorMaterial );

	savefile->ReadMaterial( bloodSprayMaterial );

	savefile->ReadFloat( lastDamageTime );

	savefile->ReadFloat( shakeFinishTime );
	savefile->ReadFloat( shakeScale );
	savefile->ReadFloat( tvScale );
	savefile->ReadInt( tvFinishTime );
	savefile->ReadInt ( tvStartTime );

	savefile->ReadVec4( fadeColor );
	savefile->ReadVec4( fadeToColor );
	savefile->ReadVec4( fadeFromColor );
	savefile->ReadFloat( fadeRate );
	savefile->ReadInt( fadeTime );

	savefile->ReadObject( reinterpret_cast<idClass *&>( player ) );
	savefile->ReadRenderView( view );
}

/*
==============
idPlayerView::SetPlayerEntity
==============
*/
void idPlayerView::SetPlayerEntity( idPlayer *playerEnt ) {
	player = playerEnt;

	const idDict* dict = NULL;
	if( !playerEnt ) {
		return;
	}
		
	dict = gameLocal.FindEntityDefDict( playerEnt->spawnArgs.GetString("def_playerView"), false );

	if( dict ) {
		dvMaterial = declManager->FindMaterial( dict->GetString("mtr_doubleVision") );
		dvMaterialBlend = declManager->FindMaterial( dict->GetString("mtr_doubleVisionBlend") );
		tunnelMaterial = declManager->FindMaterial( dict->GetString("mtr_tunnel") );
		armorMaterial = declManager->FindMaterial( dict->GetString("mtr_armourEffect") );
		bloodSprayMaterial = declManager->FindMaterial( dict->GetString("mtr_bloodspray") );
		lagoMaterial = declManager->FindMaterial( LAGO_MATERIAL, false );
	}
}

/*
==============
idPlayerView::ClearEffects
==============
*/
void idPlayerView::ClearEffects() {
	lastDamageTime = MS2SEC( gameLocal.time - 99999 );

	dvFinishTime = ( gameLocal.time - 99999 );
	tvFinishTime = ( gameLocal.time - 99999 );
	tvStartTime = ( gameLocal.time - 99999 );
	kickFinishTime = ( gameLocal.time - 99999 );
	shakeFinishTime = gameLocal.time; 

	for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ ) {
		screenBlobs[i].finishTime = gameLocal.time;
	}

	fadeTime = 0;
	bfgVision = false;
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
// RAVEN BEGIN
// jnewquist: Controller rumble
void idPlayerView::DamageImpulse( idVec3 localKickDir, const idDict *damageDef, int damage ) {
	//
	// double vision effect
	//
	float tvTime = damageDef->GetFloat( "tv_time" );
	if ( tvTime ) {
		tvStartTime = gameLocal.time;
		tvFinishTime = gameLocal.time + tvTime;		
		damageDef->GetFloat ( "tv_scale", "1", tvScale );
	}

	if ( lastDamageTime > 0.0f && SEC2MS( lastDamageTime ) + IMPULSE_DELAY > gameLocal.time ) {
		// keep shotgun from obliterating the view
		return;
	}

	//
	// double vision effect
	//
	float dvTime = damageDef->GetFloat( "dv_time" );
	if ( dvTime ) {
		dvFinishTime = gameLocal.time + (g_dvTime.GetFloat() * dvTime);
		damageDef->GetFloat ( "dv_scale", "1", dvScale );
	}

	//
	// head angle kick
	//
	const float	modifierScale = 0.25f;
	const float inverseModifier = ( 1.0f - modifierScale );

	float		modifier = idMath::ClampFloat( 0.0f, inverseModifier, damage / 100.0f * inverseModifier ) + modifierScale;
	float	kickTime = damageDef->GetFloat( "kick_time" );

	if ( kickTime ) {
		kickFinishTime = gameLocal.time + g_kickTime.GetFloat() * kickTime;

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

		if ( modifier < kickAmplitude ) {
			modifier = kickAmplitude;
		}
	}
	else {
		kickTime = 500;
	}

	//
	// screen blob
	//
	float	blobTime = damageDef->GetFloat( "blob_time" );
	if ( blobTime ) {
		screenBlob_t	*blob = GetScreenBlob();
		blob->startFadeTime = gameLocal.time;
		blob->finishTime = gameLocal.time + blobTime * g_blobTime.GetFloat();

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
// RAVEN END

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
	if ( recoilTime && recoilTime > (kickFinishTime - gameLocal.time) ) {
		idAngles angles;
		weaponDef->GetAngles( "recoilAngles", "5 0 0", angles );
		kickAngles = angles;
		int	finish = gameLocal.time + g_kickTime.GetFloat() * recoilTime;
		kickFinishTime = finish;
	}	

}

/*
===================
idPlayerView::CalculateShake
===================
*/
// RAVEN BEGIN
// jnewquist: Controller rumble
float idPlayerView::CalculateShake( idAngles &shakeAngleOffset ) const {
	idVec3	origin, matrix;

	float shakeVolume = soundSystem->CurrentShakeAmplitudeForPosition( SOUNDWORLD_GAME, gameLocal.time, player->firstPersonViewOrigin );
	//
	// shakeVolume should somehow be molded into an angle here
	// it should be thought of as being in the range 0.0 -> 1.0, although
	// since CurrentShakeAmplitudeForPosition() returns all the shake sounds
	// the player can hear, it can go over 1.0 too.
	//
	shakeAngleOffset[0] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAngleOffset[1] = gameLocal.random.CRandomFloat() * shakeVolume;
	shakeAngleOffset[2] = gameLocal.random.CRandomFloat() * shakeVolume;

	return shakeVolume;
}
// RAVEN END

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
===================
idPlayerView::ShakeOffsets
===================
*/
// RAVEN BEGIN
// jnewquist: Controller rumble
void idPlayerView::ShakeOffsets( idVec3 &shakeOffset, idAngles &shakeAngleOffset, const idBounds bounds ) const {
	float shakeVolume = 0.0f;
	shakeOffset.Zero();
	shakeAngleOffset.Zero();

	if( gameLocal.isMultiplayer ) {
		return;
	}

	shakeVolume = CalculateShake( shakeAngleOffset );

	if( gameLocal.time < shakeFinishTime ) {
		float offset = ( shakeFinishTime - gameLocal.time ) * shakeScale * 0.001f;

		shakeOffset[0] = idMath::ClampFloat( bounds[0][0] - 1.0f, bounds[1][0] + 1.0f, rvRandom::flrand( -offset, offset ) );
		shakeOffset[1] = idMath::ClampFloat( bounds[0][1] - 1.0f, bounds[1][1] + 1.0f, rvRandom::flrand( -offset, offset ) );
		shakeOffset[2] = idMath::ClampFloat( bounds[0][2] - 1.0f, bounds[1][2] + 1.0f, rvRandom::flrand( -offset, offset ) );

		shakeAngleOffset[0] = idMath::ClampFloat( -70.0f, 70.0f, rvRandom::flrand( -offset, offset ) );
		shakeAngleOffset[1] = idMath::ClampFloat( -70.0f, 70.0f, rvRandom::flrand( -offset, offset ) );
		shakeAngleOffset[2] = idMath::ClampFloat( -70.0f, 70.0f, rvRandom::flrand( -offset, offset ) );
	}
}
// RAVEN END

/*
==================
idPlayerView::SingleView
==================
*/
void idPlayerView::SingleView( idUserInterface *hud, const renderView_t *view, int renderFlags ) {
	// normal rendering
	if ( !view ) {
		return;
	}

	if ( !( RF_GUI_ONLY & renderFlags ) ) {
		// jscott: portal sky rendering with KRABS
		idCamera *portalSky = gameLocal.GetPortalSky();
		if( portalSky ) {
			renderView_t portalSkyView = *view;
			portalSky->GetViewParms( &portalSkyView );
			gameRenderWorld->RenderScene( &portalSkyView, ( renderFlags & ( ~RF_PRIMARY_VIEW ) ) | RF_DEFER_COMMAND_SUBMIT | RF_PORTAL_SKY );
		}
		gameRenderWorld->RenderScene( view, renderFlags | RF_PENUMBRA_MAP );
	}

	if ( RF_NO_GUI & renderFlags ) {
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

		// Render tunnel vision
		if ( gameLocal.time < tvFinishTime ) {
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, tvScale * ((float)(tvFinishTime - gameLocal.time) / (float)(tvFinishTime - tvStartTime)) );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, tunnelMaterial );
		}

		player->DrawHUD( hud );

			
/*
		// tunnel vision
		float	health = 0.0f;
		if ( g_testHealthVision.GetFloat() != 0.0f ) {
			health = g_testHealthVision.GetFloat();
		} else {
			health = player->health;
		}
		float alpha = health / 100.0f;
		if ( alpha < 0.0f ) {
			alpha = 0.0f;
		}
		if ( alpha > 1.0f ) {
			alpha = 1.0f;
		}

		if ( alpha < 1.0f  ) {
			renderSystem->SetColor4( ( player->health <= 0.0f ) ? MS2SEC( gameLocal.time ) : lastDamageTime, 1.0f, 1.0f, ( player->health <= 0.0f ) ? 0.0f : alpha );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, tunnelMaterial );
		}
*/		

		

		// Render the object system
		// RAVEN BEGIN
		// twhitaker: always draw objective system
		if ( player->objectiveSystem ) {
			player->objectiveSystem->Redraw( gameLocal.time );
		}		
		// RAVEN END
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

/*
===================
idPlayerView::DoubleVision
===================
*/
void idPlayerView::DoubleVision( idUserInterface *hud, const renderView_t *view, int offset ) {

	if ( !g_doubleVision.GetBool() ) {
		SingleView( hud, view, RF_NO_GUI );
		return;
	}

	float	scale = offset * g_dvAmplitude.GetFloat() * dvScale;
	if( scale < 0.0f ) {
		return;
	}

	if ( scale > 0.5f ) {
		scale = 0.5f;
	}
	float shift = scale * idMath::Sin( idMath::Sqrt ( offset ) * g_dvFrequency.GetFloat() );
	shift = fabs( shift );

	// if double vision, render to a texture
	renderSystem->CropRenderSize( 512, 256, true );
	SingleView( hud, view, RF_NO_GUI );
	renderSystem->CaptureRenderToImage( "_scratch" );
	renderSystem->UnCrop();

	// carry red tint if in berserk mode
	idVec4 color(1, 1, 1, 1);

	renderSystem->SetColor4( color.x, color.y, color.z, 1.0f );
// RAVEN BEGIN
// jnewquist: Call DrawStretchCopy, which will flip the texcoords for D3D
	renderSystem->DrawStretchCopy( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, shift, 1, 1, 0, dvMaterial );
	renderSystem->DrawStretchCopy( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1-shift, 0, dvMaterialBlend );
// RAVEN END
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
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, pct );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, player->GetInfluenceMaterial() );
	} else if ( player->GetInfluenceEntity() == NULL ) {
		SingleView( hud, view, RF_NO_GUI );
		return;
	} else {
		int offset =  25 + idMath::Sin ( gameLocal.time );
		DoubleVision( hud, view, pct * offset );
	}
}

/*
===================
idPlayerView::RenderPlayerView
===================
*/
void idPlayerView::RenderPlayerView( idUserInterface *hud ) {
	if ( !player ) {
		return;
	}

	const renderView_t *view = player->GetRenderView();
	if ( !view ) {
		return;
	}
	
	bool guiRendered = false;

	// place the sound origin for the player
	soundSystem->PlaceListener( view->vieworg, view->viewaxis, player->entityNumber + 1, gameLocal.time, "Undefined" );

	if ( g_skipViewEffects.GetBool() ) {
		SingleView( hud, view );
	} else {
		if ( player->GetInfluenceMaterial() || player->GetInfluenceEntity() ) {
			InfluenceVision( hud, view );
			guiRendered = true;
		} else if ( g_doubleVision.GetBool() && gameLocal.time < dvFinishTime ) {
			DoubleVision( hud, view, dvFinishTime - gameLocal.time );
			guiRendered = false;
		} else {
			SingleView( hud, view, RF_NO_GUI | RF_PRIMARY_VIEW );
		}

		// Now draw GUI's.
		if ( !guiRendered ) {
			SingleView( hud, view, RF_GUI_ONLY );
		}

		ScreenFade();
	}

	if ( net_clientLagOMeter.GetBool() && lagoMaterial && gameLocal.isClient && !( gameLocal.GetDemoState() == DEMO_PLAYING && ( gameLocal.IsServerDemo() || gameLocal.IsTimeDemo() ) ) ) {
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
		renderSystem->DrawStretchPic( 10.0f, 380.0f, 64.0f, 64.0f, 0.0f, 0.0f, 1.0f, 1.0f, lagoMaterial );
	}

}

// RAVEN END
