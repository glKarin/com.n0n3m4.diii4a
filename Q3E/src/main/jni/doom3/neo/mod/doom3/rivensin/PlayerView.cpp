/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "renderer/RenderWorld.h"

#include "gamesys/SysCvar.h"
#include "gamesys/SaveGame.h"
#include "GameBase.h"
#include "Player.h"

#include "PlayerView.h"

static int MakePowerOfTwo( int num ) {
	int		pot;
	for (pot = 1 ; pot < num ; pot<<=1) {
	}
	return pot;
}

const int IMPULSE_DELAY = 150;

/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView() :

//HDR start
#ifdef _DENTONMOD
m_postProcessManager()			// Invoke the postprocess Manager Constructor - J.C.Denton
#endif
//HDR end
{
	memset( screenBlobs, 0, sizeof( screenBlobs ) );
	memset( &view, 0, sizeof( view ) );
	player = NULL;
	dvMaterial = declManager->FindMaterial( "_scratch" );
	tunnelMaterial = declManager->FindMaterial( "textures/decals/tunnel" );
	armorMaterial = declManager->FindMaterial( "armorViewEffect" );
	berserkMaterial = declManager->FindMaterial( "textures/decals/berserk" );
	irGogglesMaterial = declManager->FindMaterial( "textures/decals/irblend" );
	bloodSprayMaterial = declManager->FindMaterial( "textures/decals/bloodspray" );
	bfgMaterial = declManager->FindMaterial( "textures/decals/bfgvision" );
	lagoMaterial = declManager->FindMaterial( LAGO_MATERIAL, false );
	bfgVision = false;

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
	savefile->WriteBool( bfgVision );

	savefile->WriteMaterial( tunnelMaterial );
	savefile->WriteMaterial( armorMaterial );
	savefile->WriteMaterial( berserkMaterial );
	savefile->WriteMaterial( irGogglesMaterial );
	savefile->WriteMaterial( bloodSprayMaterial );
	savefile->WriteMaterial( bfgMaterial );
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
	savefile->ReadBool( bfgVision );

	savefile->ReadMaterial( tunnelMaterial );
	savefile->ReadMaterial( armorMaterial );
	savefile->ReadMaterial( berserkMaterial );
	savefile->ReadMaterial( irGogglesMaterial );
	savefile->ReadMaterial( bloodSprayMaterial );
	savefile->ReadMaterial( bfgMaterial );
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
	this->m_postProcessManager.Initialize();
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
		dvFinishTime += g_dvTime.GetFloat() * dvTime;
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
		int	finish = gameLocal.time + g_kickTime.GetFloat() * recoilTime;
		kickFinishTime = finish;
	}

}

/*
===================
idPlayerView::CalculateShake
===================
*/
void idPlayerView::CalculateShake() {
	idVec3	origin, matrix;

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
void idPlayerView::SingleView( idUserInterface *hud, const renderView_t *view ) {

	// normal rendering
	if ( !view ) {
		return;
	}

	// place the sound origin for the player
	gameSoundWorld->PlaceListener( view->vieworg, view->viewaxis, player->entityNumber + 1, gameLocal.time, hud ? hud->State().GetString( "location" ) : "Undefined" );

	// if the objective system is up, don't do normal drawing
	if ( player->objectiveSystemOpen ) {
		player->objectiveSystem->Redraw( gameLocal.time );
		return;
	}

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t	hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

#ifdef _PORTALSKY
	if ( gameLocal.portalSkyEnt.GetEntity() && gameLocal.IsPortalSkyAcive() && g_enablePortalSky.GetBool() ) {

		renderView_t	portalView = hackedView;
		portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();

		// setup global fixup projection vars
		if ( 1 ) {
			int vidWidth, vidHeight;
			idVec2 shiftScale;

			renderSystem->GetGLSettings( vidWidth, vidHeight );

			float pot;
			int	 w = vidWidth;
			pot = MakePowerOfTwo( w );
			shiftScale.x = (float)w / pot;

			int	 h = vidHeight;
			pot = MakePowerOfTwo( h );
			shiftScale.y = (float)h / pot;

			hackedView.shaderParms[4] = shiftScale.x;
			hackedView.shaderParms[5] = shiftScale.y;
		}

		gameRenderWorld->RenderScene( &portalView );
		renderSystem->CaptureRenderToImage( "_currentRender" );

		hackedView.forceUpdate = true;				// FIX: for smoke particles not drawing when portalSky present
	}
#endif // _PORTALSKY

	gameRenderWorld->RenderScene( &hackedView );

	if ( player->spectating ) {
		return;
	}

////Enable thirdperson hud code start REVILITY	
		player->DrawHUD( hud );
////Enable thirdperson hud code END REVILITY
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

		/*		player->DrawHUD( hud );*/

		// armor impulse feedback
		float	armorPulse = ( gameLocal.time - player->lastArmorPulse ) / 250.0f;

		if ( armorPulse > 0.0f && armorPulse < 1.0f ) {
			renderSystem->SetColor4( 1, 1, 1, 1.0 - armorPulse );
			renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, armorMaterial );
		}


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

		if ( player->PowerUpActive(BERSERK) ) {
			int berserkTime = player->inventory.powerupEndTime[ BERSERK ] - gameLocal.time;
			if ( berserkTime > 0 ) {
				// start fading if within 10 seconds of going away
				alpha = (berserkTime < 10000) ? (float)berserkTime / 10000 : 1.0f;
				renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, alpha );
				renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, berserkMaterial );
			}
		}

		if ( bfgVision ) {
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, bfgMaterial );
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
	float shift = scale * sin( sqrtf( offset ) * g_dvFrequency.GetFloat() );
	shift = fabs( shift );

	// if double vision, render to a texture
	renderSystem->CropRenderSize( 512, 256, true );
	SingleView( hud, view );
	renderSystem->CaptureRenderToImage( "_scratch" );
	renderSystem->UnCrop();

	// carry red tint if in berserk mode
	idVec4 color(1, 1, 1, 1);
	if ( gameLocal.time < player->inventory.powerupEndTime[ BERSERK ] ) {
		color.y = 0;
		color.z = 0;
	}

	renderSystem->SetColor4( color.x, color.y, color.z, 1.0f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, shift, 1, 1, 0, dvMaterial );
	renderSystem->SetColor4( color.x, color.y, color.z, 0.5f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1-shift, 0, dvMaterial );
}

/*
===================
idPlayerView::BerserkVision
===================
*/
void idPlayerView::BerserkVision( idUserInterface *hud, const renderView_t *view ) {
	renderSystem->CropRenderSize( 512, 256, true );
	SingleView( hud, view );
	renderSystem->CaptureRenderToImage( "_scratch" );
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
		renderSystem->CaptureRenderToImage( "_currentRender" );
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, pct );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, player->GetInfluenceMaterial() );
	} else if ( player->GetInfluenceEntity() == NULL ) {
		SingleView( hud, view );
		return;
	} else {
		int offset =  25 + sinf( gameLocal.time );
		DoubleVision( hud, view, pct * offset );
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

	if ( g_skipViewEffects.GetBool() ) {
		SingleView( hud, view );
	} else {


		/* Render the standard view */
		if ( player->GetInfluenceMaterial() || player->GetInfluenceEntity() ) {
			InfluenceVision( hud, view );
		} else if ( gameLocal.time < dvFinishTime ) {
			DoubleVision( hud, view, dvFinishTime - gameLocal.time );
		} else if ( player->PowerUpActive( BERSERK ) ) {
			BerserkVision( hud, view );
		} else {
			SingleView( hud, view );
		}

#ifdef _DENTONMOD
		// HDR related - J.C.Denton
		/* Update HDR post-process */
		this->m_postProcessManager.Update();
#endif
			ScreenFade();
	}

	/* Render the hud on top of everything */
	if ( !pm_thirdPerson.GetBool() && !g_skipViewEffects.GetBool() && !player->objectiveSystemOpen && !player->spectating ) {
		player->DrawHUD( hud );
	}

	if ( net_clientLagOMeter.GetBool() && lagoMaterial && gameLocal.isClient ) {
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
		renderSystem->DrawStretchPic( 10.0f, 380.0f, 64.0f, 64.0f, 0.0f, 0.0f, 1.0f, 1.0f, lagoMaterial );
	}
}

#ifdef _DENTONMOD
/*
===================
idPlayerView::dnPostProcessManager Class Definitions - JC Denton
===================
*/

idPlayerView::dnPostProcessManager::dnPostProcessManager():
	m_imageCurrentRender				( "_currentRender"			),
	m_imageCurrentRender8x8DownScaled	( "_RTtoTextureScaled64x"	),
	m_imageLuminance64x64				( "_luminanceTexture64x64"	),
	m_imageluminance4x4					( "_luminanceTexture4x4"	),
	m_imageAdaptedLuminance1x1			( "_adaptedLuminance"		),
	m_imageBloom						( "_bloomImage"				),
	m_imageHalo							( "_haloImage"				),
	m_imageCookedMath					( "_cookedMath"				),
	m_matCookMath_pass1		( declManager->FindMaterial( "postprocess/cookMath_pass1" )		),
	m_matCookMath_pass2		( declManager->FindMaterial( "postprocess/cookMath_pass2" )		),
	m_matCookMath_pass3		( declManager->FindMaterial( "postprocess/cookMath_pass3" )		),

	m_matAvgLuminance64x	( declManager->FindMaterial( "postprocess/averageLum64" )	),
	m_matAvgLumSample4x4	( declManager->FindMaterial( "postprocess/averageLum4" )	),
	m_matAdaptLuminance		( declManager->FindMaterial( "postprocess/adaptLum" )		),
	m_matBrightPass			( declManager->FindMaterial( "postprocess/brightPassOptimized" )		),
	m_matGaussBlurX			( declManager->FindMaterial( "postprocess/blurx" )			),
	m_matGaussBlurY			( declManager->FindMaterial( "postprocess/blury" )			),
	m_matHalo				( declManager->FindMaterial(  "postprocess/halo" )			),
	m_matGaussBlurXHalo		( declManager->FindMaterial( "postprocess/blurx_halo" )		),
	m_matGaussBlurYHalo		( declManager->FindMaterial( "postprocess/blury_halo" )		),
	//m_matFinalScenePass		( declManager->FindMaterial( "postprocess/finalScenePass" )	),
	m_matFinalScenePass		( declManager->FindMaterial( "postprocess/finalScenePassOptimized" )	),

	m_matCookVignette		( declManager->FindMaterial( "postprocess/cookVignette" )		),

	// Materials for debugging intermediate textures
	m_matDecodedLumTexture64x64	( declManager->FindMaterial( "postprocess/decode_luminanceTexture64x64" )	), 
	m_matDecodedLumTexture4x4	( declManager->FindMaterial( "postprocess/decode_luminanceTexture4x4" )		),
	m_matDecodedAdaptLuminance	( declManager->FindMaterial( "postprocess/decode_adaptedLuminance" )		)
{

	m_iScreenHeight = m_iScreenWidth = 0;
	m_iScreenHeightPow2 = m_iScreenWidthPow2 = 0;
	m_fShiftScale_x = m_fShiftScale_y = 0;
	m_bForceUpdateOnCookedData = false;
	m_nFramesSinceLumUpdate = 0;
	m_nFramesToUpdateCookedData = 0;

	// Initialize once this object is created.	
	this->Initialize();

#if !defined(_RIVENSIN)
	if(!common->SetCallback(idCommon::CB_ReloadImages, (idCommon::FunctionPointer)ReloadImagesCallback, this))
	{
		gameLocal.Warning("Couldn't set ReloadImages Callback from Ruiner game DLL! This could lead to errors on vid_restart and similar!\n");
	}
#endif
}

idPlayerView::dnPostProcessManager::~dnPostProcessManager()
{
#if !defined(_RIVENSIN)
	// remove callback because this object is destroyed (and this was passed as userArg)
	common->SetCallback(idCommon::CB_ReloadImages, NULL, NULL);
#endif
}

// this is called by the engine whenever vid_restart or reloadImages is executed
// (replaces old SourceHook hack)
/* static */ void idPlayerView::dnPostProcessManager::ReloadImagesCallback(void* arg, const idCmdArgs& cmdArgs)
{
	idPlayerView::dnPostProcessManager* self = (idPlayerView::dnPostProcessManager*)arg;
	self->m_nFramesToUpdateCookedData = 1;
	if( r_HDR_postProcess.GetBool() )
		gameLocal.Printf("Cooked Data will be updated after %d frames...\n", self->m_nFramesToUpdateCookedData  );
	else
		gameLocal.Printf("Cooked Data will be updated after %d frames immediately after r_HDR_postProcess is enabled.\n", self->m_nFramesToUpdateCookedData  );
}

void idPlayerView::dnPostProcessManager::Initialize()
{
	m_bForceUpdateOnCookedData = true;

	// Make sure that we always measure luminance at first frame. 
	m_nFramesSinceLumUpdate	= r_HDR_lumUpdateRate.GetInteger();

	r_HDR_enable.SetModified();
}

void idPlayerView::dnPostProcessManager::UpdateCookedData( void )
{

	if( m_nFramesToUpdateCookedData > 0 )
	{
		m_nFramesToUpdateCookedData --;
		m_bForceUpdateOnCookedData = true;
		return;
	}

	if (	m_bForceUpdateOnCookedData || 
			r_HDR_middleGray.IsModified() || r_HDR_maxColorIntensity.IsModified()		|| 
			r_HDR_colorCurveBias.IsModified() || r_HDR_brightPassOffset.IsModified()	|| 
			r_HDR_brightPassThreshold.IsModified() || r_HDR_sceneExposure.IsModified()	||
			r_HDR_gammaCorrection.IsModified()	|| r_HDR_vignetteBias.IsModified()		||
			r_HDR_eyeAdjustmentBias.IsModified() || r_HDR_eyeAdjustmentBloomBias.IsModified()
		)
	{

		if( m_bForceUpdateOnCookedData )
			gameLocal.Printf( "Forcing an update on cooked math data.\n" );

		gameLocal.Printf( "Cooking math data please wait...\n" );

		//------------------------------------------------------------------------
		// Crop backbuffer image to the size of our cooked math image
		//------------------------------------------------------------------------
		renderSystem->CropRenderSize(1024, 256, true);
		//------------------------------------------------------------------------

		// Changed from max to Max for cross platform compiler compatibility.
		const float fMaxColorIntensity = Max( r_HDR_maxColorIntensity.GetFloat(), 0.00001f );

		//------------------------------------------------------------------------
		// Cook math Pass 1 
		//------------------------------------------------------------------------
		renderSystem->SetColor4( r_HDR_middleGray.GetFloat(), 1.0f/fMaxColorIntensity, r_HDR_sceneExposure.GetFloat(), r_HDR_eyeAdjustmentBias.GetFloat() );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matCookMath_pass1 );
		renderSystem->CaptureRenderToImage( m_imageCookedMath );

		//------------------------------------------------------------------------
		// Cook math Pass 2 
		//------------------------------------------------------------------------
 		renderSystem->SetColor4( r_HDR_middleGray.GetFloat(), r_HDR_brightPassThreshold.GetFloat(), r_HDR_brightPassOffset.GetFloat(), r_HDR_eyeAdjustmentBloomBias.GetFloat() );
 		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matCookMath_pass2 );
 		renderSystem->CaptureRenderToImage( m_imageCookedMath );

		//------------------------------------------------------------------------
		// Cook math Pass 3 
		//------------------------------------------------------------------------
		renderSystem->SetColor4( r_HDR_colorCurveBias.GetFloat(), r_HDR_gammaCorrection.GetFloat(), 0.0f, 0.0f  );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matCookMath_pass3 );
		renderSystem->CaptureRenderToImage( m_imageCookedMath );

		//------------------------------------------------------------------------
		// Cooke Vignette image 
		//------------------------------------------------------------------------
		renderSystem->SetColor4( r_HDR_vignetteBias.GetFloat(), 1.0f/m_fShiftScale_x, 1.0f/m_fShiftScale_y, 0.0f  );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matCookVignette );
		renderSystem->CaptureRenderToImage( m_imageCookedMath );
		//------------------------------------------------------------------------
		renderSystem->UnCrop();
		//------------------------------------------------------------------------
		r_HDR_middleGray.ClearModified();
		r_HDR_maxColorIntensity.ClearModified();
		r_HDR_colorCurveBias.ClearModified(); 
		r_HDR_brightPassOffset.ClearModified();
		r_HDR_brightPassThreshold.ClearModified();
		r_HDR_sceneExposure.ClearModified();
		r_HDR_gammaCorrection.ClearModified();
		r_HDR_vignetteBias.ClearModified();
		r_HDR_eyeAdjustmentBias.ClearModified();
		r_HDR_eyeAdjustmentBloomBias.ClearModified();

		m_bForceUpdateOnCookedData = false;

		gameLocal.Printf( "Cooking complete.\n" );
	}
}


void idPlayerView::dnPostProcessManager::Update( void )
{
	static const float fBloomImageDownScale = 4.0f;
	static const float fHaloImageDownScale = 8.0f;
	static const float fBackbufferLumDownScale = 8.0f;


	// Check the interaction.vfp settings
 	if( r_HDR_enable.IsModified() )
 	{
 		this->UpdateInteractionShader();
 		r_HDR_enable.ClearModified();
 	}

	if ( r_HDR_postProcess.GetBool() ) 
	{
		this->UpdateBackBufferParameters();

		renderSystem->CaptureRenderToImage( m_imageCurrentRender );
		this->UpdateCookedData();

		// Delayed luminance measurement and adaptation for performance improvement.
		if( r_HDR_eyeAdjustmentBias.GetFloat() > 0.0f )
		{
			if( m_nFramesSinceLumUpdate >= r_HDR_lumUpdateRate.GetInteger() )
			{
				//-------------------------------------------------
				// Downscale 
				//-------------------------------------------------
				renderSystem->CropRenderSize(m_iScreenWidthPow2/fBackbufferLumDownScale, m_iScreenHeightPow2/fBackbufferLumDownScale, true);
				renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
				renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, m_fShiftScale_y, m_fShiftScale_x, 0, m_imageCurrentRender );
				renderSystem->CaptureRenderToImage( m_imageCurrentRender8x8DownScaled );
				renderSystem->UnCrop();
				//-------------------------------------------------
				// Measure Luminance from Downscaled Image
				//-------------------------------------------------
				renderSystem->CropRenderSize(64, 64, true);
				renderSystem->SetColor4( 1.0f/Min( 192.0f, m_iScreenWidthPow2/fBackbufferLumDownScale), 1.0f, 1.0f, 1.0f );			 
				renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matAvgLuminance64x );
				renderSystem->CaptureRenderToImage( m_imageLuminance64x64 );
				renderSystem->UnCrop();
				//-------------------------------------------------
				// Average out the luminance of the scene to a 4x4 Texture
				//-------------------------------------------------
				renderSystem->CropRenderSize(4, 4, true);
				renderSystem->SetColor4( 1.0f/16.0f, 1.0f, 1.0f, 1.0f );			 
				renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matAvgLumSample4x4 );
				renderSystem->CaptureRenderToImage( m_imageluminance4x4 );
				renderSystem->UnCrop();


				// Reset vars
				m_nFramesSinceLumUpdate	= 1;
			}
			else
			{
				m_nFramesSinceLumUpdate ++;
			}

			//-------------------------------------------------
			// Adapt to the newly calculated Luminance from previous Luminance.
			//-------------------------------------------------
			renderSystem->CropRenderSize(1, 1, true);
			renderSystem->SetColor4( (gameLocal.time - gameLocal.previousTime)/(1000.0f * r_HDR_eyeAdjustmentDelay.GetFloat() ), r_HDR_max_luminance.GetFloat(), r_HDR_min_luminance.GetFloat(), 1.0f );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matAdaptLuminance );
			renderSystem->CaptureRenderToImage( m_imageAdaptedLuminance1x1 );
			renderSystem->UnCrop();
			//---------------------
		}	// End of : r_HDR_eyeAdjustmentBias.GetFloat() > 0.0f 

		const float fHDRBloomIntensity = r_HDR_bloomIntensity.GetFloat();
		const float fHDRHaloIntensity = fHDRBloomIntensity > 0.0f ? r_HDR_haloIntensity.GetFloat() : 0.0f;

		if( fHDRBloomIntensity > 0.0f )
		{
			//-------------------------------------------------
			// Apply the bright-pass filter to acquire bloom image
			//-------------------------------------------------
			renderSystem->CropRenderSize(m_iScreenWidthPow2/fBloomImageDownScale, m_iScreenHeightPow2/fBloomImageDownScale, true);

			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matBrightPass );
			renderSystem->CaptureRenderToImage( m_imageBloom );

			//-------------------------------------------------
			// Apply Gaussian Smoothing to create bloom
			//-------------------------------------------------

			renderSystem->SetColor4( fBloomImageDownScale/m_iScreenWidthPow2, 1.0f, 1.0f, 1.0f );			 
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matGaussBlurX );
			renderSystem->CaptureRenderToImage( m_imageBloom );
			renderSystem->SetColor4( fBloomImageDownScale/m_iScreenHeightPow2, 1.0f, 1.0f, 1.0f );		 
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matGaussBlurY );

			renderSystem->CaptureRenderToImage( m_imageBloom );
			renderSystem->UnCrop();
			//---------------------

			if(  fHDRHaloIntensity > 0.0f )
			{
				//-------------------------------------------------
				// Downscale bloom image and blur again to obtain Halo Image
				//-------------------------------------------------
				renderSystem->CropRenderSize(m_iScreenWidthPow2/fHaloImageDownScale, m_iScreenHeightPow2/fHaloImageDownScale, true);

				renderSystem->SetColor4( fHaloImageDownScale/m_iScreenWidthPow2, 1.0f, 1.0f, 1.0f );			 
				renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matGaussBlurXHalo );
				renderSystem->CaptureRenderToImage( m_imageHalo );
				renderSystem->SetColor4( fHaloImageDownScale/m_iScreenHeightPow2, 1.0f, 1.0f, 1.0f );		 
				renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, m_matGaussBlurYHalo );
				renderSystem->CaptureRenderToImage( m_imageHalo );
				renderSystem->UnCrop();
			}
		}

		//-------------------------------------------------
		// Calculate and Render Final Image
		//-------------------------------------------------
		renderSystem->SetColor4( fHDRBloomIntensity, fHDRHaloIntensity, .5f, 1.0f );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, m_fShiftScale_y, m_fShiftScale_x, 0, m_matFinalScenePass );
		//-------------------------------------------------

		this->RenderDebugTextures();
	}
}

void idPlayerView::dnPostProcessManager::UpdateBackBufferParameters()
{
	// This condition makes sure that, the 2 loops inside run once only when resolution changes or map starts.
	if( m_iScreenHeight != renderSystem->GetScreenHeight() || m_iScreenWidth !=renderSystem->GetScreenWidth() )
	{
		m_iScreenHeightPow2 = 256, m_iScreenWidthPow2 = 256;

		// This should probably fix the ATI issue...
		renderSystem->GetGLSettings( m_iScreenWidth, m_iScreenHeight );

		//assert( iScreenWidth != 0 && iScreenHeight != 0 );

		while( m_iScreenWidthPow2 < m_iScreenWidth ) {
			m_iScreenWidthPow2 <<= 1;
		}
		while( m_iScreenHeightPow2 < m_iScreenHeight ) {
			m_iScreenHeightPow2 <<= 1;
		}
		m_fShiftScale_x = m_iScreenWidth  / (float)m_iScreenWidthPow2;
		m_fShiftScale_y = m_iScreenHeight / (float)m_iScreenHeightPow2;
	}
}

void idPlayerView::dnPostProcessManager::RenderDebugTextures()
{
	const int iDebugMode = r_HDR_enableDebugMode.GetInteger();

	if( 1 == iDebugMode )
	{
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
		renderSystem->DrawStretchPic( 0,				0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, 1, 1, 0, m_imageCurrentRender8x8DownScaled );
		renderSystem->DrawStretchPic( SCREEN_WIDTH/5,	0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, 1, 1, 0, m_imageLuminance64x64 );
		renderSystem->DrawStretchPic( SCREEN_WIDTH*2/5, 0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, 1, 1, 0, m_imageluminance4x4 );
		renderSystem->DrawStretchPic( SCREEN_WIDTH*3/5, 0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, 1, 1, 0, m_imageAdaptedLuminance1x1 );
		renderSystem->DrawStretchPic( SCREEN_WIDTH*4/5, 0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, m_fShiftScale_y, m_fShiftScale_x, 0, m_imageBloom );

	}
	else if ( 2 == iDebugMode )
	{
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
		renderSystem->DrawStretchPic( 0,				0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, 1, 1, 0, m_imageCurrentRender8x8DownScaled );
		renderSystem->DrawStretchPic( SCREEN_WIDTH/5,	0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, 1, 1, 0, m_matDecodedLumTexture64x64 );
		renderSystem->DrawStretchPic( SCREEN_WIDTH*2/5, 0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, 1, 1, 0, m_matDecodedLumTexture4x4 );
		renderSystem->DrawStretchPic( SCREEN_WIDTH*3/5, 0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, 1, 1, 0, m_matDecodedAdaptLuminance );
		renderSystem->DrawStretchPic( SCREEN_WIDTH*4/5, 0, SCREEN_WIDTH/6,	SCREEN_HEIGHT/6, 0, m_fShiftScale_y, m_fShiftScale_x, 0, m_imageBloom );
	}

	const int iDebugTexture = r_HDR_debugTextureIndex.GetInteger();
	if( 0!= iDebugMode && 0 < iDebugTexture && 5 > iDebugTexture ) 
	{
		struct {
			dnImageWrapper *m_pImage;
			float m_fShiftScaleX, m_fShiftScaleY;
		} 
		const arrStretchedImages[4] = { 
				{&m_imageCurrentRender8x8DownScaled, 1, 1},
				{&m_imageBloom,			m_fShiftScale_x, m_fShiftScale_y },
				{&m_imageHalo,			m_fShiftScale_x, m_fShiftScale_y },
				{&m_imageCookedMath,	1, 1},
		};
		
		int i = iDebugTexture - 1;

		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
 			renderSystem->DrawStretchPic( 0, SCREEN_HEIGHT * .2f, SCREEN_WIDTH * 0.6f, SCREEN_HEIGHT * 0.6f, 0, 
				arrStretchedImages[i].m_fShiftScaleY, arrStretchedImages[i].m_fShiftScaleX, 0, 
				*arrStretchedImages[i].m_pImage );
	}
}

void idPlayerView::dnPostProcessManager::UpdateInteractionShader()
{
	// Check the CVARs.
	if (r_HDR_enable.GetBool())
	{
		cvarSystem->SetCVarInteger("r_testARBProgram", 1);
		r_HDR_postProcess.SetBool(true);
		gameLocal.Printf("HDR enabled.\n");
	}
	else
	{
		cvarSystem->SetCVarInteger("r_testARBProgram", 0);
		r_HDR_postProcess.SetBool(false);
		gameLocal.Printf("HDR disabled.\n");
	}
}
#endif
