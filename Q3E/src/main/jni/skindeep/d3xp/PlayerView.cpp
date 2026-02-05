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

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should hav7e received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "renderer/RenderWorld.h"

#include "gamesys/SysCvar.h"
#include "gamesys/SaveGame.h"
#include "GameBase.h"
#include "Player.h"

#include "bc_ftl.h"
#include "bc_meta.h"

#include "PlayerView.h"
#include <bc_ventpeek.h>

// _D3XP : rename all gameLocal.time to gameLocal.slow.time for merge!

#ifdef _D3XP
static int MakePowerOfTwo( int num ) {
	int		pot;
	for (pot = 1 ; pot < num ; pot<<=1) {
	}
	return pot;
}
#endif

const int IMPULSE_DELAY = 150;


const float BLOODOVERLAY_TIME = 5000.0f; //when bloodbag explodes.


const int SIGHTED_FLASHTIME = 500;

const float ISLIGHTED_FADETIME = 200.0f;


#define HIDDEN_FADETIME 500
#define HIDDEN_ALPHA 0.8f	//translucency value of hidden vignette
#define HIDDEN_R	.18f
#define HIDDEN_G	.22f
#define HIDDEN_B	.64f

#define CONFINED_FADETIME 300
#define CONFINED_ALPHA 1.0f





#define CONCUSS_MAXSHIFT	.005f //amount of doublevision.

#define CONCUSS_RAMPMIN		300
#define CONCUSS_RAMPVAR		400

//how long to stay in doublevision mode
#define CONCUSS_DOUBLEVISTIMEMIN	2000
#define CONCUSS_DOUBLEVISTIMEVAR	1000

//how long you have 'normal' vision
#define CONCUSS_IDLEMIN		5000
#define CONCUSS_IDLEVAR		6000

#define CONCUSS_COOLDOWNTIME	3000

#define CONCUSS_MINBLUR .3f

const float ARMORPULSETIME = 300.0f;

const float DURABILITYFLASH_TIME = 600.0f;



/*
==============
idPlayerView::idPlayerView
==============
*/
idPlayerView::idPlayerView()
{
	memset( screenBlobs, 0, sizeof( screenBlobs ) );
	memset( &view, 0, sizeof( view ) );
	player = NULL;
	dvMaterial = declManager->FindMaterial( "_scratch" );
	tunnelMaterial = declManager->FindMaterial( "textures/decals/tunnel" );
	armorMaterial = declManager->FindMaterial( "guis/assets/burst_lines" );
	berserkMaterial = declManager->FindMaterial( "textures/decals/berserk" );
	irGogglesMaterial = declManager->FindMaterial( "textures/decals/irblend" );
	bloodSprayMaterial = declManager->FindMaterial( "textures/decals/bloodspray" );
	bfgMaterial = declManager->FindMaterial( "textures/decals/bfgvision" );
	lagoMaterial = declManager->FindMaterial( LAGO_MATERIAL, false );
	

	//BC
	bloodedgeMaterial = declManager->FindMaterial("textures/fx/bloodedge");
	bokehMaterial = declManager->FindMaterial("textures/fx/bokeh");
	durabilityflashMaterial = declManager->FindMaterial("guis/assets/burst_lines_blur");
	
	bloodbagOverlayActive = false;
	bloodbagState = BAGSTATE_OFF;
	bloodbagTimer = 0;
	sightedTimer = 0;


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
#ifdef _D3XP
	fxManager = NULL;

	if ( !fxManager ) {
		fxManager = new FullscreenFXManager;
		fxManager->Initialize( this );
	}
#endif

	ClearEffects();


	//bc
	hiddenTimer = 0;	
	hiddenStartAlpha = 0;
	hiddenEndAlpha = 0;
	hiddenCurrentAlpha = 0;	
	hiddenActive = false;

	confinedTimer = 0;
	confinedStartAlpha = 0;
	confinedEndAlpha = 0;
	confinedCurrentAlpha = 0;
	confinedActive = false;

	isLightedTimer = 0;
	isLightedActive = false;
	durabilityflashStartTime = 0;
	durabilityflashActive = false;

    bloodrageLerping = false;
    bloodrageStartLerpValue = 0;
    bloodrageEndLerpValue = 0;
    bloodrageLerpTimer = 0;
    bloodrageCurrentValue = 0;
	bloodrageTotalLerpTime = 0;
}

/*
==============
idPlayerView::Save
==============
*/
void idPlayerView::Save( idSaveGame *savefile ) const {
	const screenBlob_t *blob = &screenBlobs[ 0 ];
	savefile->WriteInt( MAX_SCREEN_BLOBS ); // screenBlob_t screenBlobs[MAX_SCREEN_BLOBS]
	for( int i = 0; i < MAX_SCREEN_BLOBS; i++, blob++ ) {
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

	savefile->WriteInt( sightedTimer ); // int sightedTimer

	savefile->WriteInt( hiddenTimer ); // int hiddenTimer
	savefile->WriteBool( hiddenActive ); // bool hiddenActive
	savefile->WriteFloat( hiddenStartAlpha ); // float hiddenStartAlpha
	savefile->WriteFloat( hiddenEndAlpha ); // float hiddenEndAlpha
	savefile->WriteFloat( hiddenCurrentAlpha ); // float hiddenCurrentAlpha

	savefile->WriteInt( confinedTimer ); // int confinedTimer
	savefile->WriteBool( confinedActive ); // bool confinedActive
	savefile->WriteFloat( confinedStartAlpha ); // float confinedStartAlpha
	savefile->WriteFloat( confinedEndAlpha ); // float confinedEndAlpha
	savefile->WriteFloat( confinedCurrentAlpha ); // float confinedCurrentAlpha

	savefile->WriteInt( isLightedTimer ); // int isLightedTimer
	savefile->WriteBool( isLightedActive ); // bool isLightedActive

	savefile->WriteInt( durabilityflashStartTime ); // int durabilityflashStartTime
	savefile->WriteBool( durabilityflashActive ); // bool durabilityflashActive
	savefile->WriteMaterial( durabilityflashMaterial ); // const idMaterial * durabilityflashMaterial

	savefile->WriteBool( bloodrageLerping ); // bool bloodrageLerping
	savefile->WriteFloat( bloodrageStartLerpValue ); // float bloodrageStartLerpValue
	savefile->WriteFloat( bloodrageEndLerpValue ); // float bloodrageEndLerpValue
	savefile->WriteFloat( bloodrageCurrentValue ); // float bloodrageCurrentValue
	savefile->WriteInt( bloodrageLerpTimer ); // int bloodrageLerpTimer
	savefile->WriteInt( bloodrageTotalLerpTime ); // int bloodrageTotalLerpTime

	savefile->WriteInt( dvFinishTime ); // int dvFinishTime
	savefile->WriteMaterial( dvMaterial ); // const idMaterial * dvMaterial

	savefile->WriteInt( kickFinishTime ); // int kickFinishTime
	savefile->WriteAngles( kickAngles ); // idAngles kickAngles

	savefile->WriteBool( bfgVision ); // bool bfgVision

	savefile->WriteMaterial( tunnelMaterial ); // const idMaterial * tunnelMaterial
	savefile->WriteMaterial( armorMaterial ); // const idMaterial * armorMaterial
	savefile->WriteMaterial( berserkMaterial ); // const idMaterial * berserkMaterial
	savefile->WriteMaterial( irGogglesMaterial ); // const idMaterial * irGogglesMaterial
	savefile->WriteMaterial( bloodSprayMaterial ); // const idMaterial * bloodSprayMaterial
	savefile->WriteMaterial( bfgMaterial ); // const idMaterial * bfgMaterial
	savefile->WriteMaterial( lagoMaterial ); // const idMaterial * lagoMaterial
	savefile->WriteFloat( lastDamageTime ); // float lastDamageTime

	savefile->WriteVec4( fadeColor ); // idVec4 fadeColor
	savefile->WriteVec4( fadeToColor ); // idVec4 fadeToColor
	savefile->WriteVec4( fadeFromColor ); // idVec4 fadeFromColor
	savefile->WriteFloat( fadeRate ); // float fadeRate
	savefile->WriteInt( fadeTime ); // int fadeTime

	savefile->WriteAngles( shakeAng ); // idAngles shakeAng

	savefile->WriteObject( player ); // idPlayer * player
	savefile->WriteRenderView( view ); // renderView_t view

	savefile->WriteBool( fxManager != nullptr );  // FullscreenFXManager * fxManager
	if ( fxManager ) {
		fxManager->Save( savefile );
	}

	savefile->WriteMaterial( bloodedgeMaterial ); // const idMaterial * bloodedgeMaterial
	savefile->WriteMaterial( bokehMaterial ); // const idMaterial * bokehMaterial
	savefile->WriteBool( bloodbagOverlayActive ); // bool bloodbagOverlayActive
	savefile->WriteInt( bloodbagState ); // int bloodbagState
	savefile->WriteInt( bloodbagTimer ); // int bloodbagTimer
}

/*
==============
idPlayerView::Restore
==============
*/
void idPlayerView::Restore( idRestoreGame *savefile ) {
	screenBlob_t *blob = &screenBlobs[ 0 ];
	int num;
	savefile->ReadInt( num );  // screenBlob_t screenBlobs[MAX_SCREEN_BLOBS]
	for( int i = 0; i < num; i++, blob++ ) {
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

	savefile->ReadInt( sightedTimer ); // int sightedTimer

	savefile->ReadInt( hiddenTimer ); // int hiddenTimer
	savefile->ReadBool( hiddenActive ); // bool hiddenActive
	savefile->ReadFloat( hiddenStartAlpha ); // float hiddenStartAlpha
	savefile->ReadFloat( hiddenEndAlpha ); // float hiddenEndAlpha
	savefile->ReadFloat( hiddenCurrentAlpha ); // float hiddenCurrentAlpha

	savefile->ReadInt( confinedTimer ); // int confinedTimer
	savefile->ReadBool( confinedActive ); // bool confinedActive
	savefile->ReadFloat( confinedStartAlpha ); // float confinedStartAlpha
	savefile->ReadFloat( confinedEndAlpha ); // float confinedEndAlpha
	savefile->ReadFloat( confinedCurrentAlpha ); // float confinedCurrentAlpha

	savefile->ReadInt( isLightedTimer ); // int isLightedTimer
	savefile->ReadBool( isLightedActive ); // bool isLightedActive

	savefile->ReadInt( durabilityflashStartTime ); // int durabilityflashStartTime
	savefile->ReadBool( durabilityflashActive ); // bool durabilityflashActive
	savefile->ReadMaterial( durabilityflashMaterial ); // const idMaterial * durabilityflashMaterial

	savefile->ReadBool( bloodrageLerping ); // bool bloodrageLerping
	savefile->ReadFloat( bloodrageStartLerpValue ); // float bloodrageStartLerpValue
	savefile->ReadFloat( bloodrageEndLerpValue ); // float bloodrageEndLerpValue
	savefile->ReadFloat( bloodrageCurrentValue ); // float bloodrageCurrentValue
	savefile->ReadInt( bloodrageLerpTimer ); // int bloodrageLerpTimer
	savefile->ReadInt( bloodrageTotalLerpTime ); // int bloodrageTotalLerpTime

	savefile->ReadInt( dvFinishTime ); // int dvFinishTime
	savefile->ReadMaterial( dvMaterial ); // const idMaterial * dvMaterial

	savefile->ReadInt( kickFinishTime ); // int kickFinishTime
	savefile->ReadAngles( kickAngles ); // idAngles kickAngles

	savefile->ReadBool( bfgVision ); // bool bfgVision

	savefile->ReadMaterial( tunnelMaterial ); // const idMaterial * tunnelMaterial
	savefile->ReadMaterial( armorMaterial ); // const idMaterial * armorMaterial
	savefile->ReadMaterial( berserkMaterial ); // const idMaterial * berserkMaterial
	savefile->ReadMaterial( irGogglesMaterial ); // const idMaterial * irGogglesMaterial
	savefile->ReadMaterial( bloodSprayMaterial ); // const idMaterial * bloodSprayMaterial
	savefile->ReadMaterial( bfgMaterial ); // const idMaterial * bfgMaterial
	savefile->ReadMaterial( lagoMaterial ); // const idMaterial * lagoMaterial
	savefile->ReadFloat( lastDamageTime ); // float lastDamageTime

	savefile->ReadVec4( fadeColor ); // idVec4 fadeColor
	savefile->ReadVec4( fadeToColor ); // idVec4 fadeToColor
	savefile->ReadVec4( fadeFromColor ); // idVec4 fadeFromColor
	savefile->ReadFloat( fadeRate ); // float fadeRate
	savefile->ReadInt( fadeTime ); // int fadeTime

	savefile->ReadAngles( shakeAng ); // idAngles shakeAng

	savefile->ReadObject( CastClassPtrRef(player) ); // idPlayer * player
	savefile->ReadRenderView( view ); // renderView_t view

	bool bExists;
	savefile->ReadBool( bExists );  // FullscreenFXManager * fxManager
	if ( bExists ) {
		fxManager->Restore( savefile );
	}

	savefile->ReadMaterial( bloodedgeMaterial ); // const idMaterial * bloodedgeMaterial
	savefile->ReadMaterial( bokehMaterial ); // const idMaterial * bokehMaterial
	savefile->ReadBool( bloodbagOverlayActive ); // bool bloodbagOverlayActive
	savefile->ReadInt( bloodbagState ); // int bloodbagState
	savefile->ReadInt( bloodbagTimer ); // int bloodbagTimer
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
	lastDamageTime = MS2SEC( gameLocal.slow.time - 99999 );

	dvFinishTime = ( gameLocal.fast.time - 99999 );
	kickFinishTime = ( gameLocal.slow.time - 99999 );

	for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ ) {
		screenBlobs[i].finishTime = gameLocal.slow.time;
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


	if (!g_hiteffects.GetBool())
	{
		return;
	}


	if ( lastDamageTime > 0.0f && SEC2MS( lastDamageTime ) + IMPULSE_DELAY > gameLocal.slow.time ) {
		// keep shotgun from obliterating the view
		return;
	}

	float	dvTime = damageDef->GetFloat( "dv_time" );
	if ( dvTime ) {
		if ( dvFinishTime < gameLocal.fast.time ) {
			dvFinishTime = gameLocal.fast.time;
		}
		dvFinishTime += g_dvTime.GetFloat() * dvTime;
		// don't let it add up too much in god mode
		if ( dvFinishTime > gameLocal.fast.time + 5000 ) {
			dvFinishTime = gameLocal.fast.time + 5000;
		}
	}

	//
	// head angle kick
	//
	float	kickTime = damageDef->GetFloat( "kick_time" );
	if ( kickTime ) {
		kickFinishTime = gameLocal.slow.time + g_kickTime.GetFloat() * kickTime;

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
	if ( blobTime )
	{
		int i;
		int blobCount = damageDef->GetInt("blob_count", "1");

		for (i = 0; i < blobCount; i++)
		{
			screenBlob_t	*blob = GetScreenBlob();
			int blob_x_rand;

			blob->startFadeTime = gameLocal.slow.time;
			blob->finishTime = gameLocal.slow.time + blobTime * g_blobTime.GetFloat() * ((float)gameLocal.msec / USERCMD_MSEC);

			const char *materialName = damageDef->GetString("mtr_blob", "genericDamage");
			blob->material = declManager->FindMaterial(materialName);
			blob->x = damageDef->GetFloat("blob_x", "140");
			blob_x_rand = damageDef->GetInt("blob_x_rand", "0");
			if (blob_x_rand != 0)
			{
				blob->x += (gameLocal.random.RandomInt()&blob_x_rand) - (blob_x_rand / 2); //BC custom X random position
			}
			else
			{
				blob->x += (gameLocal.random.RandomInt() & 63) - 32; //default.
			}
			blob->y = damageDef->GetFloat("blob_y", "-100");
			blob->y += (gameLocal.random.RandomInt() & 63) - 32;

			float scale = (256 + ((gameLocal.random.RandomInt() & 63) - 32)) / 256.0f;
			blob->w = damageDef->GetFloat("blob_width", "500") * g_blobSize.GetFloat() * scale;
			blob->h = damageDef->GetFloat("blob_height", "500") * g_blobSize.GetFloat() * scale;
			blob->s1 = 0;
			blob->t1 = 0;
			blob->s2 = 1;
			blob->t2 = 1;
		}
	}

	//
	// save lastDamageTime for tunnel vision accentuation
	//
	lastDamageTime = MS2SEC( gameLocal.slow.time );

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
	blob->startFadeTime = gameLocal.slow.time;
	blob->finishTime = gameLocal.slow.time + ( duration * 1000 );
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
	if ( recoilTime && kickFinishTime < gameLocal.slow.time ) {
		idAngles angles;
		weaponDef->GetAngles( "recoilAngles", "5 0 0", angles );
		kickAngles = angles;
		int	finish = gameLocal.slow.time + g_kickTime.GetFloat() * recoilTime;
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

	float shakeVolume = gameSoundWorld->CurrentShakeAmplitudeForPosition( gameLocal.slow.time, player->firstPersonViewOrigin );
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

	if ( gameLocal.slow.time < kickFinishTime ) {
		float offset = kickFinishTime - gameLocal.slow.time;

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
	//gameSoundWorld->PlaceListener( view->vieworg, view->viewaxis, player->entityNumber + 1, gameLocal.slow.time, hud ? hud->State().GetString( "location" ) : "Undefined" );

	//BC instead of using location string, use the entity name.
	//gameSoundWorld->PlaceListener(view->vieworg, view->viewaxis, player->entityNumber + 1, gameLocal.slow.time, player->reverbLocation);


	
	idVec3 listenPos = view->vieworg;
	if (gameLocal.GetLocalPlayer()->listenmodeActive)
	{
		//BC when leaning through door, make the listener position be on other side of door.
		listenPos = gameLocal.GetLocalPlayer()->listenmodePos;
	}
	else if (gameLocal.GetLocalPlayer()->peekObject.IsValid() && static_cast<idVentpeek *>(gameLocal.GetLocalPlayer()->peekObject.GetEntity())->GetLockListener())
	{
		// SW: ventpeek can optionally force listener to stay at the player's physical position
		// We can't use the eyeball here because the eyeball is somewhere else, 
		// so instead we use some parameters to figure out where the eyeball *would* be
		float eyeheight = gameLocal.GetLocalPlayer()->IsCrouching()
			? pm_crouchviewheight.GetFloat()
			: pm_normalviewheight.GetFloat();

		listenPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + (idVec3::Up() * eyeheight);
	}
	else
	{
		listenPos = view->vieworg;
	}

	gameSoundWorld->PlaceListener(listenPos, view->viewaxis, player->entityNumber + 1, gameLocal.slow.time, player->reverbLocation);


	// if the objective system is up, don't do normal drawing
	if ( player->objectiveSystemOpen )
	{
		player->objectiveSystem->Redraw( gameLocal.fast.time );
		return;
	}

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t	hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

#ifdef _D3XP
	if ( gameLocal.portalSkyEnt.GetEntity() && g_enablePortalSky.GetBool() )
	{
		int vidWidth, vidHeight;
		idVec2 shiftScale;

		renderSystem->GetGLSettings(vidWidth, vidHeight);

		float pot;
		int	 w = vidWidth;
		pot = MakePowerOfTwo(w); shiftScale.x = (float)w / pot;

		int	 h = vidHeight;
		pot = MakePowerOfTwo(h);
		shiftScale.y = (float)h / pot;

		if (gameLocal.IsPortalSkyAcive())
		{
			renderView_t	portalView = hackedView;
			portalView.vieworg = gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin();

			hackedView.shaderParms[4] = shiftScale.x;
			hackedView.shaderParms[5] = shiftScale.y;

			// SM: This is so janky, but need some way to flag this is a portalSky
			// without adding more parameters to renderView_t
			// This fixes SD-90: Extra outlines rendered due to portal sky
			portalView.shaderParms[11] = -42.0f;

			gameRenderWorld->RenderScene(&portalView);
			renderSystem->CaptureRenderToImage("_currentRender");

			hackedView.forceUpdate = true;				// FIX: for smoke particles not drawing when portalSky present
		}
		gameRenderWorld->SetPortalSkyParams(true, gameLocal.portalSkyEnt.GetEntity()->GetPhysics()->GetOrigin(), shiftScale);
	}
	else
	{
		gameRenderWorld->SetPortalSkyParams(false);
	}

	// process the frame
	fxManager->Process( &hackedView );
#endif

	//safescreen.
	if (g_safescreen.GetBool())
	{
		#define	SAFESCREENAMOUNT .9f
		renderSystem->SetColor4(1, 0, 0, .3f);
		int safeWidth = 640 * SAFESCREENAMOUNT;
		int safeHeight = 480 * SAFESCREENAMOUNT;

		int barWidth = (640 - safeWidth) / 2;
		renderSystem->DrawStretchPic(0, 0, barWidth, 480, 0, 0, 1, 1, declManager->FindMaterial("guis/assets/white.tga"));
		renderSystem->DrawStretchPic(640 - barWidth, 0, barWidth, 480, 0, 0, 1, 1, declManager->FindMaterial("guis/assets/white.tga"));

		int barHeight = (480 - safeHeight) / 2;
		renderSystem->DrawStretchPic(0, 0, 640, barHeight, 0, 0, 1, 1, declManager->FindMaterial("guis/assets/white.tga"));
		renderSystem->DrawStretchPic(0, 480 - barHeight, 640, barHeight, 0, 0, 1, 1, declManager->FindMaterial("guis/assets/white.tga"));
	}

	if ( player->spectating )
	{
		

		//Event log.
		player->DrawSpectatorHUD();

		return;
	}

#ifdef _D3XP
	if ( !hud ) {
		return;
	}
#endif

	// draw screen blobs
	if ( !pm_thirdPerson.GetBool() && !g_skipViewEffects.GetBool() )
	{
		for ( int i = 0 ; i < MAX_SCREEN_BLOBS ; i++ )
		{
			screenBlob_t	*blob = &screenBlobs[i];
			if ( blob->finishTime <= gameLocal.slow.time ) {
				continue;
			}

			blob->y += blob->driftAmount;

			float	fade = (float)( blob->finishTime - gameLocal.slow.time ) / ( blob->finishTime - blob->startFadeTime );
			if ( fade > 1.0f ) {
				fade = 1.0f;
			}
			if ( fade ) {
				renderSystem->SetColor4( 1,1,1,fade );
				renderSystem->DrawStretchPic( blob->x, blob->y, blob->w, blob->h,blob->s1, blob->t1, blob->s2, blob->t2, blob->material );
			}
		}

		//bc ventpeek
		if (player->peekObject.IsValid())
		{
			// Draw telescope overlay instead of our regular ventpeek
			// This needs to have a precise aspect ratio no matter what the screen shape is, so it's a bit more work than usual.
			if (static_cast<idVentpeek*>(player->peekObject.GetEntity())->forTelescope)
			{
				float aspectRatio = (float)renderSystem->GetScreenHeight() / (float)renderSystem->GetScreenWidth();
				float overlayMaxWidth = 640 * aspectRatio;
				float sidePadding = (640 - overlayMaxWidth) / 2;
				renderSystem->DrawStretchPic(0, 0, sidePadding, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/black"));
				renderSystem->DrawStretchPic(sidePadding + overlayMaxWidth, 0, sidePadding, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/black"));

				// Effect: draw a biiiig crack in the lens (telescope vignette)
				if (static_cast<idVentpeekTelescope*>(player->peekObject.GetEntity())->IsLensCracked())
				{
					renderSystem->DrawStretchPic(sidePadding, 0, overlayMaxWidth, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/telescope_crack"));
				}

				// We now have a nice square to draw our aperture.

				// First: Lens and base vignette
				renderSystem->DrawStretchPic(sidePadding, 0, overlayMaxWidth, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/telescope"));

				// Next: Glowing reticle overlay
				renderSystem->DrawStretchPic(sidePadding, 0, overlayMaxWidth, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/telescope_reticle"));

				// if the shutter is active, we may also need to draw a shrinking vignette
				float apertureSize = static_cast<idVentpeekTelescope*>(player->peekObject.GetEntity())->GetApertureSize();
				if (apertureSize < 1.0f)
				{
					//camera is currently in the process of taking a picture. Aperture is doing its shutter animation (shrinking then growing).
					float shrunkenWidth = overlayMaxWidth * apertureSize;
					float shrunkenHeight = 480 * apertureSize;
					float sideAperturePadding = (overlayMaxWidth - shrunkenWidth) / 2;
					float topAperturePadding = (480 - shrunkenHeight) / 2;
					// Pad around all four sides
					renderSystem->DrawStretchPic(sidePadding, 0, overlayMaxWidth, topAperturePadding, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/black"));
					renderSystem->DrawStretchPic(sidePadding, 0, sideAperturePadding, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/black"));
					renderSystem->DrawStretchPic(overlayMaxWidth + sidePadding - sideAperturePadding, 0, sideAperturePadding, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/black"));
					renderSystem->DrawStretchPic(sidePadding, 480 - topAperturePadding, overlayMaxWidth, topAperturePadding, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/black"));
					// Draw shrunken aperture
					renderSystem->DrawStretchPic(sidePadding + sideAperturePadding, topAperturePadding, shrunkenWidth, shrunkenHeight, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/telescope_shutter"));
				}
			}
			else
			{
				if (!player->isInLabelInspectMode())
				{

					//tweak it depending on pitch.
					float offsetY = 0;

					#define	OVERLAY_OFFSET 70

					if (player->viewAngles[0] > 0)
					{
						float lerp = min(player->viewAngles[0], 30) / 30.0f;
						offsetY = idMath::Lerp(0, OVERLAY_OFFSET, lerp);
					}
					else
					{
						float lerp = max(player->viewAngles[0], -30) / 30.0f;
						offsetY = idMath::Lerp(0, OVERLAY_OFFSET, lerp);
					}

					//draw ventpeek overlay.
					renderSystem->DrawStretchPic(0, -OVERLAY_OFFSET + offsetY, 640, 480 + (OVERLAY_OFFSET * 2), 0, 0, 1, 1, declManager->FindMaterial("textures/fx/ventpeek"));
				}
			}
		}
		else
		{
			//Zero g fullscreen filter.
			//note: this can interfere with ventpeek and other overlays
			if (player->airless && !player->inDownedState)
			{
				renderSystem->SetColor4(1, 1, 1, 1.0f);
				renderSystem->DrawStretchPic(0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/zerog"));
			}
		}
		

		if (player->GetSmelly())
		{
			renderSystem->SetColor4(.55f, .61f, .08f, 1.0f);
			renderSystem->DrawStretchPic(00, -160, 640, 720, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/smellyvignette"));
		}



		//Fire fx.
		// SW 26th Feb 2025:
		// Don't continue to show this on the screen if the player dies while on fire
		if (player->GetOnFire() && !player->AI_DEAD)
		{
			renderSystem->SetColor4(1, 1, 1, 1.0f);
			renderSystem->DrawStretchPic(-50, 0, 740, 700, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/screenfire"));
		}

        //blood rage. This is the red goo that fills up the player's eyeballs.
        if (bloodrageCurrentValue > 0 || bloodrageLerping)
        {
            if (bloodrageLerping)
            {
                //calculate the bloodrage lerp.
                float bloodragelerp = (gameLocal.time - bloodrageLerpTimer) / (float)bloodrageTotalLerpTime;
                bloodragelerp = idMath::ClampFloat(0, 1, bloodragelerp);

				bloodragelerp = idMath::CubicEaseOut(bloodragelerp);
                bloodrageCurrentValue = idMath::Lerp(bloodrageStartLerpValue, bloodrageEndLerpValue, bloodragelerp);

				if (bloodragelerp >= 1)
				{
					bloodrageLerping = false;
				}
            }
            
			#define	BLOODRAGE_BOTTOMMARGIN 240 //bloodrage only appears a teeny bit at bottom of screen.
			#define BLOODRAGE_TOPMARGIN -260 //bloodrage fills entire screen.
            float bloodrageDrawOffset = idMath::Lerp(BLOODRAGE_BOTTOMMARGIN, BLOODRAGE_TOPMARGIN, bloodrageCurrentValue);
            renderSystem->DrawStretchPic(0, bloodrageDrawOffset, 640, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/bloodrage"));

            renderSystem->DrawStretchPic(0, bloodrageDrawOffset + 480, 640, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/bloodrage_filler"));
        }



		// armor impulse feedback
		float	armorPulse = ( gameLocal.fast.time - player->lastArmorPulse ) / ARMORPULSETIME;

		//BC make a pulse emerge from bottom left
		if ( armorPulse > 0.0f && armorPulse < 1.0f )
		{
			idVec4 pulseVec;
			pulseVec.Lerp(idVec4(20, 410, 30, 40), idVec4(-215, 130, 500, 600), armorPulse);		 //origin point = 35, 430

			float pulseAlpha;
			if (armorPulse < .5f)
				pulseAlpha = 1;
			else
			{
				pulseAlpha = ((gameLocal.fast.time - player->lastArmorPulse) / (ARMORPULSETIME / 2.0f)) - 1.0f;
				pulseAlpha = 1.0f - pulseAlpha;
			}
			
			renderSystem->SetColor4( 1, 1, 1, pulseAlpha);
			renderSystem->DrawStretchPic(pulseVec.x, pulseVec.y, pulseVec.z, pulseVec.w, 0, 0, 1, 1, armorMaterial );
		}


		// tunnel vision
		float	health = 0.0f;
		if ( g_testHealthVision.GetFloat() != 0.0f ) {
			health = g_testHealthVision.GetFloat();
		} else {
			health = player->health;
		}
		float alpha = health / player->maxHealth;
		if ( alpha < 0.0f ) {
			alpha = 0.0f;
		}
		if ( alpha > 1.0f ) {
			alpha = 1.0f;
		}

		

		if ( alpha < 1.0f  && player->health > 0 && g_hiteffects.GetBool())
		{
			renderSystem->SetColor4( ( player->health <= 0.0f ) ? MS2SEC( gameLocal.slow.time ) : lastDamageTime, 1.0f, 1.0f, ( player->health <= 0.0f ) ? 0.0f : alpha );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, tunnelMaterial );
		}

		//BC debug
		if ( bfgVision )
		{
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, bfgMaterial );
		}
		


		
		UpdateConfinedVignette();
		UpdateHiddenVignette();

		UpdateDurabilityFlash();
		



		//The spattered blood screen-edge effect when really low health.

		int SPATTER_UPPERTHRESHOLD = player->maxHealth * .8f;
		int SPATTER_LOWERTHRESHOLD = player->maxHealth * .2f;
		

		if (((player->health < SPATTER_UPPERTHRESHOLD || player->inDownedState) && !gameLocal.inCinematic && player->health > 0 && g_hiteffects.GetBool()) || (player->doForceSpatter && g_hiteffects.GetBool()))
		{
			float bloodColor = 1;
			float bloodwidth = 640;
			float bloodheight = 480;
			renderSystem->SetColor4(0, 0, 0, 1);

			if ((player->health > SPATTER_LOWERTHRESHOLD && !player->inDownedState) || player->doForceSpatter)
			{
				//Transition in.
				float lerp = player->doForceSpatter 
					? 1 - player->forceSpatterAmount // manual control
					: (player->health - SPATTER_LOWERTHRESHOLD) / float(SPATTER_UPPERTHRESHOLD - SPATTER_LOWERTHRESHOLD); // standard gameplay
				
				bloodwidth = idMath::Lerp(640, 1024, lerp);
				bloodheight = idMath::Lerp(480, 960, lerp);
			
				bloodColor = idMath::Lerp(1,0, lerp);
			}

			renderSystem->DrawStretchPic((bloodwidth - 640) / -2, (bloodheight  - 480)  / -2, bloodwidth, bloodheight, 0.0f, 0.0f, 1.0f, 1.0f, bloodedgeMaterial); //this is the black hard-edge weathering pattern

			#define BOKEH_Y_MAXOFFSET -64
			float bokehY = player->viewAngles.pitch / 90.0f;
			bokehY *= BOKEH_Y_MAXOFFSET;

			if (g_bloodEffects.GetBool())
			{
				renderSystem->SetColor4(bloodColor, 0, 0, 1);
				renderSystem->DrawStretchPic(0, bokehY, 640, 480, 0.0f, 0.0f, 1.0f, 1.0f, bokehMaterial); //the fake bokeh bloodsplots
			}
		}

		//if (player->health <= 0)
		//{
		//	//deathcam letterbox
		//	int letterboxheight = 48;
		//	renderSystem->SetColor4(0, 0, 0, 1);
		//	renderSystem->DrawStretchPic(0, 0, 640, letterboxheight, 0, 0, 1, 1, declManager->FindMaterial("_white"));
		//	renderSystem->DrawStretchPic(0, 480 - letterboxheight, 640, letterboxheight, 0, 0, 1, 1, declManager->FindMaterial("_white"));
		//}

		//when blood bag explodes.
		if (bloodbagOverlayActive)
		{
			float lerp;
			float bloodalpha = 1;
			float bloodY;

			lerp = (gameLocal.time - bloodbagTimer) / BLOODOVERLAY_TIME;
			lerp = idMath::ClampFloat(0, 1, lerp);

			if (bloodbagState == BAGSTATE_SLIDING)
			{
				if (gameLocal.time > bloodbagTimer + (BLOODOVERLAY_TIME - 1000))
					bloodbagState = BAGSTATE_FADEOUT;
			}
			else if (bloodbagState == BAGSTATE_FADEOUT)
			{
				float alphaLerp = (gameLocal.time - (bloodbagTimer + (BLOODOVERLAY_TIME - 1000))) / 1000.0f;
				alphaLerp = idMath::ClampFloat(0, 1, alphaLerp);
				bloodalpha = 1.0f - alphaLerp;
			}

			bloodY = idMath::Lerp(.2f, -.5f, lerp);

			renderSystem->SetColor4(0, 0, bloodY, bloodalpha);
			renderSystem->DrawStretchPic(0, 0, 320, 480, 0, 0, 1, 1, declManager->FindMaterial("bloodbag_overlay"));

			if (gameLocal.time >= bloodbagTimer + BLOODOVERLAY_TIME)
				bloodbagOverlayActive = false;
		}

		//The Sighted screen flash.
		if (sightedTimer > gameLocal.time)
		{
			float sightLerp = (sightedTimer - gameLocal.time) / (float)SIGHTED_FLASHTIME;

			renderSystem->SetColor4(sightLerp, sightLerp, sightLerp,1);
			renderSystem->DrawStretchPic(0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/sighted_vignette"));
		}



		


		//highlight system.
		if (gameLocal.menuPause && static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetHightlighterActive())
		{
			static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->DrawHighlighterBars();
		}
		else
		{
			//draw normal hud.
			player->DrawHUD(hud);
		}
	}



	// test a single material drawn over everything
	if ( g_testPostProcess.GetString()[0] )
	{
		const idMaterial *mtr = declManager->FindMaterial( g_testPostProcess.GetString(), false );
		if ( !mtr )
		{
			common->Printf( "Material not found.\n" );
			g_testPostProcess.SetString( "" );
		}
		else
		{
			renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
			renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, mtr );
		}
	}
}

void idPlayerView::DoBloodrageLerp(float newValue, int timeMS)
{
    bloodrageLerping = true;
    bloodrageStartLerpValue = bloodrageCurrentValue;
    bloodrageEndLerpValue = newValue;
    bloodrageLerpTimer = gameLocal.time;
	bloodrageTotalLerpTime = timeMS;
}


void idPlayerView::UpdateHiddenVignette()
{
	//draw dark vignette when in hidden space.
	bool isHidden =  (player->GetHiddenStatus() == LIGHTMETER_UNSEEN);

	if (isHidden != hiddenActive)
	{
		//State change. Has either entered or exited confined state.

		if (isHidden)
		{
			//Entering confined state.
			hiddenActive = true;
			hiddenTimer = gameLocal.time + HIDDEN_FADETIME;
			hiddenStartAlpha = hiddenCurrentAlpha;
			hiddenEndAlpha = HIDDEN_ALPHA;

			player->StartSound("snd_stealthenter", SND_CHANNEL_ANY, 0, false, NULL);
		}
		else
		{
			//Exiting confined state.
			hiddenActive = false;
			hiddenTimer = gameLocal.time + HIDDEN_FADETIME;
			hiddenStartAlpha = hiddenCurrentAlpha;
			hiddenEndAlpha = 0.0f;

			player->StartSound("snd_stealthexit", SND_CHANNEL_ANY, 0, false, NULL);
		}
	}

	if (hiddenTimer > gameLocal.time || isHidden)
	{
		float boundsModifier = 0;

		if (hiddenTimer >= gameLocal.time)
		{
			float lerp = (hiddenTimer - gameLocal.time) / (float)HIDDEN_FADETIME;
			lerp = 1.0f - lerp;
			hiddenCurrentAlpha = idMath::Lerp(hiddenStartAlpha, hiddenEndAlpha, lerp);

			
			
			if (isHidden)
			{
				boundsModifier = idMath::Lerp(30, 0, lerp);
			}
			else
			{
				boundsModifier = idMath::Lerp(0, 30, lerp);
			}
		}
		else if (gameLocal.time >= hiddenTimer && hiddenCurrentAlpha < HIDDEN_ALPHA)
		{
			hiddenCurrentAlpha = HIDDEN_ALPHA;
		}

		

		if (g_showHud.GetBool())
		{
			renderSystem->SetColor4(.38f, .78f, .87f, hiddenCurrentAlpha);
			renderSystem->DrawStretchPic(-boundsModifier, -boundsModifier, 640 + (boundsModifier * 2), 480 + (boundsModifier * 2), 0, 0, 1, 1, declManager->FindMaterial("guis/assets/vignette_unseen"));
		}

		//renderSystem->SetColor4(.5f, 0, 1, confinedHideCurrentAlpha * .8f);
		//renderSystem->DrawStretchPic(0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/vignette_border"));
	}


	//Draw the "you're in the light" border effect.
	//if (player->GetDarknessValue() >= 2 && !isLightedActive)
	//{
	//	isLightedActive = true;
	//	isLightedTimer = gameLocal.time + ISLIGHTED_FADETIME;
	//}
	//else if (player->GetDarknessValue() < 2 && isLightedActive)
	//{
	//	isLightedActive = false;
	//	isLightedTimer = gameLocal.time + ISLIGHTED_FADETIME;
	//}
	//
	//if (isLightedTimer > gameLocal.time || player->GetDarknessValue())
	//{
	//	float lerp = 1.0f - ((isLightedTimer - gameLocal.time) / ISLIGHTED_FADETIME);
	//	lerp = idMath::ClampFloat(0, 1, lerp);
	//	
	//	float alpha;
	//	if (isLightedActive)
	//		alpha = idMath::Lerp(0, 1, lerp);
	//	else
	//		alpha = idMath::Lerp(1, 0, lerp);
	//
	//	renderSystem->SetColor4(1, .9f, .8f, alpha);
	//	renderSystem->DrawStretchPic(0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/vignette_border"));
	//}
	
}

void idPlayerView::UpdateConfinedVignette()
{
	bool isConfined = player->confinedAngleLock;

	if (isConfined != confinedActive)
	{
		//State change. Has either entered or exited confined state.

		if (isConfined)
		{
			//Entering confined state.
			confinedActive = true;
			confinedTimer = gameLocal.time + CONFINED_FADETIME;
			confinedStartAlpha = confinedCurrentAlpha;
			confinedEndAlpha = CONFINED_ALPHA;

			//player->StartSound("snd_stealthenter", SND_CHANNEL_ANY, 0, false, NULL);
		}
		else
		{
			//Exiting confined state.
			confinedActive = false;
			confinedTimer = gameLocal.time + CONFINED_FADETIME;
			confinedStartAlpha = confinedCurrentAlpha;
			confinedEndAlpha = 0.0f;

			//player->StartSound("snd_stealthexit", SND_CHANNEL_ANY, 0, false, NULL);
		}
	}

	if (confinedTimer > gameLocal.time || isConfined)
	{
		if (confinedTimer >= gameLocal.time)
		{
			float lerp = (confinedTimer - gameLocal.time) / (float)CONFINED_FADETIME;
			lerp = 1.0f - lerp;
			confinedCurrentAlpha = idMath::Lerp(confinedStartAlpha, confinedEndAlpha, lerp);
		}
		else if (gameLocal.time >= confinedTimer && confinedCurrentAlpha < CONFINED_ALPHA)
		{
			confinedCurrentAlpha = CONFINED_ALPHA;
		}

		renderSystem->SetColor4(0, 0, 0, confinedCurrentAlpha);
		
		#define CONFINEDVIGNETTE_WIDTH 300

		float leftOffset = idMath::Lerp(-CONFINEDVIGNETTE_WIDTH, 0, confinedCurrentAlpha);
		float rightOffset = idMath::Lerp(CONFINEDVIGNETTE_WIDTH, 0, confinedCurrentAlpha);		

		renderSystem->DrawStretchPic(-10 + leftOffset, 0, CONFINEDVIGNETTE_WIDTH, 480, 0, 0, 1, 1, declManager->FindMaterial("textures/fx/confined_vignette"));
		renderSystem->DrawStretchPic(640 - CONFINEDVIGNETTE_WIDTH + 10 + rightOffset, 0, CONFINEDVIGNETTE_WIDTH, 480, 0, 0, -1, 1, declManager->FindMaterial("textures/fx/confined_vignette"));
	}

}


/*
=================
idPlayerView::Flash

flashes the player view with the given color
=================
*/
void idPlayerView::Flash(idVec4 color, int time ) {
	Fade(idVec4(0, 0, 0, 0), time);
	fadeFromColor = color;
}

/*
=================
idPlayerView::Fade

used for level transition fades
assumes: color.w is 0 or 1
=================
*/
void idPlayerView::Fade( idVec4 color, int time ) {
#ifdef _D3XP
	SetTimeState ts( player->timeGroup );
#endif

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

#ifdef _D3XP
	SetTimeState ts( player->timeGroup );
#endif

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
idPlayerView::RenderPlayerView
===================
*/
void idPlayerView::RenderPlayerView( idUserInterface *hud ) {
	const renderView_t *view = player->GetRenderView();

	SingleView( hud, view );
	ScreenFade();

	//if ( net_clientLagOMeter.GetBool() && lagoMaterial && gameLocal.isClient )
	//{
	//	renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
	//	renderSystem->DrawStretchPic( 10.0f, 380.0f, 64.0f, 64.0f, 0.0f, 0.0f, 1.0f, 1.0f, lagoMaterial );
	//}
}

#ifdef _D3XP
/*
===================
idPlayerView::WarpVision
===================
*/
int idPlayerView::AddWarp( idVec3 worldOrigin, float centerx, float centery, float initialRadius, float durationMsec ) {
	FullscreenFX_Warp *fx = (FullscreenFX_Warp*)( fxManager->FindFX( "warp" ) );

	if ( fx ) {
		fx->EnableGrabber( true );
		return 1;
	}

	return 1;
}

void idPlayerView::FreeWarp( int id ) {
	FullscreenFX_Warp *fx = (FullscreenFX_Warp*)( fxManager->FindFX( "warp" ) );

	if ( fx ) {
		fx->EnableGrabber( false );
		return;
	}
}





/*
==================
FxFader::FxFader
==================
*/
FxFader::FxFader() {
	time = 0;
	state = FX_STATE_OFF;
	alpha = 0;
	msec = 1000;
}

/*
==================
FxFader::SetTriggerState
==================
*/
bool FxFader::SetTriggerState( bool active ) {

	// handle on/off states
	if ( active && state == FX_STATE_OFF ) {
		state = FX_STATE_RAMPUP;
		time = gameLocal.slow.time + msec;
	}
	else if ( !active && state == FX_STATE_ON ) {
		state = FX_STATE_RAMPDOWN;
		time = gameLocal.slow.time + msec;
	}

	// handle rampup/rampdown states
	if ( state == FX_STATE_RAMPUP ) {
		if ( gameLocal.slow.time >= time ) {
			state = FX_STATE_ON;
		}
	}
	else if ( state == FX_STATE_RAMPDOWN ) {
		if ( gameLocal.slow.time >= time ) {
			state = FX_STATE_OFF;
		}
	}

	// compute alpha
	switch ( state ) {
		case FX_STATE_ON:		alpha = 1; break;
		case FX_STATE_OFF:		alpha = 0; break;
		case FX_STATE_RAMPUP:	alpha = 1 - (float)( time - gameLocal.slow.time ) / msec; break;
		case FX_STATE_RAMPDOWN:	alpha = (float)( time - gameLocal.slow.time ) / msec; break;
	}

	if ( alpha > 0 ) {
		return true;
	}
	else {
		return false;
	}
}

/*
==================
FxFader::Save
==================
*/
void FxFader::Save( idSaveGame *savefile ) {
	savefile->WriteInt( time ); // int time
	savefile->WriteInt( state ); // int state
	savefile->WriteFloat( alpha ); // float alpha
	savefile->WriteInt( msec ); // int msec
}

/*
==================
FxFader::Restore
==================
*/
void FxFader::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( time ); // int time
	savefile->ReadInt( state ); // int state
	savefile->ReadFloat( alpha ); // float alpha
	savefile->ReadInt( msec ); // int msec
}





/*
==================
FullscreenFX_Helltime::Save
==================
*/
void FullscreenFX::Save( idSaveGame *savefile ) {
	savefile->WriteString( name ); // idString name
	fader.Save( savefile );
//	FullscreenFXManager *fxman; // FullscreenFXManager * fxman
}

/*
==================
FullscreenFX_Helltime::Restore
==================
*/
void FullscreenFX::Restore( idRestoreGame *savefile ) {
	savefile->ReadString( name ); // idString name
	fader.Restore( savefile );
}


/*
==================
FullscreenFX_Helltime::Initialize
==================
*/
void FullscreenFX_Helltime::Initialize() {
	acInitMaterials[0] = declManager->FindMaterial( "textures/smf/bloodorb1/ac_init" );
	acInitMaterials[1] = declManager->FindMaterial( "textures/smf/bloodorb2/ac_init" );
	acInitMaterials[2] = declManager->FindMaterial( "textures/smf/bloodorb3/ac_init" );

	acCaptureMaterials[0] = declManager->FindMaterial( "textures/smf/bloodorb1/ac_capture" );
	acCaptureMaterials[1] = declManager->FindMaterial( "textures/smf/bloodorb2/ac_capture" );
	acCaptureMaterials[2] = declManager->FindMaterial( "textures/smf/bloodorb3/ac_capture" );

	acDrawMaterials[0] = declManager->FindMaterial( "textures/smf/bloodorb1/ac_draw" );
	acDrawMaterials[1] = declManager->FindMaterial( "textures/smf/bloodorb2/ac_draw" );
	acDrawMaterials[2] = declManager->FindMaterial( "textures/smf/bloodorb3/ac_draw" );

	crCaptureMaterials[0] = declManager->FindMaterial( "textures/smf/bloodorb1/cr_capture" );
	crCaptureMaterials[1] = declManager->FindMaterial( "textures/smf/bloodorb2/cr_capture" );
	crCaptureMaterials[2] = declManager->FindMaterial( "textures/smf/bloodorb3/cr_capture" );

	crDrawMaterials[0] = declManager->FindMaterial( "textures/smf/bloodorb1/cr_draw" );
	crDrawMaterials[1] = declManager->FindMaterial( "textures/smf/bloodorb2/cr_draw" );
	crDrawMaterials[2] = declManager->FindMaterial( "textures/smf/bloodorb3/cr_draw" );

	clearAccumBuffer = true;
}

/*
==================
FullscreenFX_Helltime::DetermineLevel
==================
*/
int FullscreenFX_Helltime::DetermineLevel() {
	idPlayer *player;
	int testfx = g_testHelltimeFX.GetInteger();

	// for testing purposes
	if ( testfx >= 0 && testfx < 3 ) {
		return testfx;
	}

	player = fxman->GetPlayer();

	if ( player->PowerUpActive( INVULNERABILITY ) ) {
		return 2;
	}
	else if ( player->PowerUpActive( BERSERK ) ) {
		return 1;
	}
	else if ( player->PowerUpActive( HELLTIME ) ) {
		return 0;
	}

	return -1;
}

/*
==================
FullscreenFX_Helltime::Active
==================
*/
bool FullscreenFX_Helltime::Active() {

	if ( gameLocal.inCinematic || gameLocal.isMultiplayer ) {
		return false;
	}

	if ( DetermineLevel() >= 0 ) {
		return true;
	}
	else {
		// latch the clear flag
		if ( fader.GetAlpha() == 0 ) {
			clearAccumBuffer = true;
		}
	}

	return false;
}

/*
==================
FullscreenFX_Helltime::AccumPass
==================
*/
void FullscreenFX_Helltime::AccumPass( const renderView_t *view ) {
	idVec2 shiftScale;
	int level = DetermineLevel();

	// for testing
	if ( level < 0 || level > 2 ) {
		level = 0;
	}

	shiftScale = fxman->GetShiftScale();
	renderSystem->SetColor4( 1, 1, 1, 1 );

	// capture pass
	if ( clearAccumBuffer )
	{
		clearAccumBuffer = false;
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, acInitMaterials[level] );
	}
	else {
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, acCaptureMaterials[level] );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, shiftScale.y, shiftScale.x, 0, crCaptureMaterials[level] );
	}

	renderSystem->CaptureRenderToImage( "_accum" );
}

/*
==================
FullscreenFX_Helltime::HighQuality
==================
*/
void FullscreenFX_Helltime::HighQuality() {
	idVec2 shiftScale;
	int level = DetermineLevel();

	// for testing
	if ( level < 0 || level > 2 ) {
		level = 0;
	}

	shiftScale = fxman->GetShiftScale();
	renderSystem->SetColor4( 1, 1, 1, 1 );

	// draw pass
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, acDrawMaterials[level] );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, shiftScale.y, shiftScale.x, 0, crDrawMaterials[level] );
}

/*
==================
FullscreenFX_Helltime::Restore
==================
*/
void FullscreenFX_Helltime::Restore( idRestoreGame *savefile ) {
	FullscreenFX::Restore( savefile );

	// latch the clear flag
	clearAccumBuffer = true;
}





/*
==================
FullscreenFX_Multiplayer::Initialize
==================
*/
void FullscreenFX_Multiplayer::Initialize() {
	acInitMaterials		= declManager->FindMaterial( "textures/smf/multiplayer1/ac_init" );
	acCaptureMaterials	= declManager->FindMaterial( "textures/smf/multiplayer1/ac_capture" );
	acDrawMaterials		= declManager->FindMaterial( "textures/smf/multiplayer1/ac_draw" );
	crCaptureMaterials	= declManager->FindMaterial( "textures/smf/multiplayer1/cr_capture" );
	crDrawMaterials		= declManager->FindMaterial( "textures/smf/multiplayer1/cr_draw" );
	clearAccumBuffer	= true;
}

/*
==================
FullscreenFX_Multiplayer::DetermineLevel
==================
*/
int FullscreenFX_Multiplayer::DetermineLevel() {
	idPlayer *player;
	int testfx = g_testMultiplayerFX.GetInteger();

	// for testing purposes
	if ( testfx >= 0 && testfx < 3 ) {
		return testfx;
	}

	player = fxman->GetPlayer();

	if ( player->PowerUpActive( INVULNERABILITY ) ) {
		return 2;
	}
	//else if ( player->PowerUpActive( HASTE ) ) {
	//	return 1;
	//}
	else if ( player->PowerUpActive( BERSERK ) ) {
		return 0;
	}

	return -1;
}

/*
==================
FullscreenFX_Multiplayer::Active
==================
*/
bool FullscreenFX_Multiplayer::Active() {

	if ( !gameLocal.isMultiplayer && g_testMultiplayerFX.GetInteger() == -1 ) {
		return false;
	}

	if ( DetermineLevel() >= 0 ) {
		return true;
	}
	else {
		// latch the clear flag
		if ( fader.GetAlpha() == 0 ) {
			clearAccumBuffer = true;
		}
	}

	return false;
}

/*
==================
FullscreenFX_Multiplayer::AccumPass
==================
*/
void FullscreenFX_Multiplayer::AccumPass( const renderView_t *view ) {
	idVec2 shiftScale;
	int level = DetermineLevel();

	// for testing
	if ( level < 0 || level > 2 ) {
		level = 0;
	}

	shiftScale = fxman->GetShiftScale();
	renderSystem->SetColor4( 1, 1, 1, 1 );

	// capture pass
	if ( clearAccumBuffer ) {
		clearAccumBuffer = false;
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, acInitMaterials );
	}
	else {
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, acCaptureMaterials );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, shiftScale.y, shiftScale.x, 0, crCaptureMaterials );
	}

	renderSystem->CaptureRenderToImage( "_accum" );
}

/*
==================
FullscreenFX_Multiplayer::HighQuality
==================
*/
void FullscreenFX_Multiplayer::HighQuality() {
	idVec2 shiftScale;
	int level = DetermineLevel();

	// for testing
	if ( level < 0 || level > 2 ) {
		level = 0;
	}

	shiftScale = fxman->GetShiftScale();
	renderSystem->SetColor4( 1, 1, 1, 1 );

	// draw pass
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 1, 1, 0, acDrawMaterials );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, shiftScale.y, shiftScale.x, 0, crDrawMaterials );
}

/*
==================
FullscreenFX_Multiplayer::Restore
==================
*/
void FullscreenFX_Multiplayer::Restore( idRestoreGame *savefile ) {
	FullscreenFX::Restore( savefile );

	// latch the clear flag
	clearAccumBuffer = true;
}





/*
==================
FullscreenFX_Warp::Initialize
==================
*/
void FullscreenFX_Warp::Initialize() {
	material = declManager->FindMaterial( "textures/smf/warp" );
	grabberEnabled = false;
	startWarpTime = 0;
}

/*
==================
FullscreenFX_Warp::Active
==================
*/
bool FullscreenFX_Warp::Active() {
	if ( grabberEnabled )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_Warp::Save
==================
*/
void FullscreenFX_Warp::Save( idSaveGame *savefile ) {
	FullscreenFX::Save( savefile );

	savefile->WriteBool( grabberEnabled );
	savefile->WriteInt( startWarpTime );
}

/*
==================
FullscreenFX_Warp::Restore
==================
*/
void FullscreenFX_Warp::Restore( idRestoreGame *savefile ) {
	FullscreenFX::Restore( savefile );

	savefile->ReadBool( grabberEnabled );
	savefile->ReadInt( startWarpTime );
}

/*
==================
FullscreenFX_Warp::DrawWarp
==================
*/
void FullscreenFX_Warp::DrawWarp( WarpPolygon_t wp, float interp ) {
	idVec4 mid1_uv, mid2_uv;
	idVec4 mid1, mid2;
	idVec2 drawPts[6], shiftScale;
	WarpPolygon_t trans;

	trans = wp;
	shiftScale = fxman->GetShiftScale();

	// compute mid points
	mid1 = trans.outer1 * ( interp ) + trans.center * ( 1 - interp );
	mid2 = trans.outer2 * ( interp ) + trans.center * ( 1 - interp );
	mid1_uv = trans.outer1 * ( 0.5 ) + trans.center * ( 1 - 0.5 );
	mid2_uv = trans.outer2 * ( 0.5 ) + trans.center * ( 1 - 0.5 );

	// draw [outer1, mid2, mid1]
	drawPts[0].Set( trans.outer1.x, trans.outer1.y );
	drawPts[1].Set( mid2.x, mid2.y );
	drawPts[2].Set( mid1.x, mid1.y );
	drawPts[3].Set( trans.outer1.z, trans.outer1.w );
	drawPts[4].Set( mid2_uv.z, mid2_uv.w );
	drawPts[5].Set( mid1_uv.z, mid1_uv.w );
	for ( int j = 0; j < 3; j++ ) {
		drawPts[j+3].x *= shiftScale.x;
		drawPts[j+3].y *= shiftScale.y;
	}
	renderSystem->DrawStretchTri( drawPts[0], drawPts[1], drawPts[2], drawPts[3], drawPts[4], drawPts[5], material );

	// draw [outer1, outer2, mid2]
	drawPts[0].Set( trans.outer1.x, trans.outer1.y );
	drawPts[1].Set( trans.outer2.x, trans.outer2.y );
	drawPts[2].Set( mid2.x, mid2.y );
	drawPts[3].Set( trans.outer1.z, trans.outer1.w );
	drawPts[4].Set( trans.outer2.z, trans.outer2.w );
	drawPts[5].Set( mid2_uv.z, mid2_uv.w );
	for ( int j = 0; j < 3; j++ ) {
		drawPts[j+3].x *= shiftScale.x;
		drawPts[j+3].y *= shiftScale.y;
	}
	renderSystem->DrawStretchTri( drawPts[0], drawPts[1], drawPts[2], drawPts[3], drawPts[4], drawPts[5], material );

	// draw [mid1, mid2, center]
	drawPts[0].Set( mid1.x, mid1.y );
	drawPts[1].Set( mid2.x, mid2.y );
	drawPts[2].Set( trans.center.x, trans.center.y );
	drawPts[3].Set( mid1_uv.z, mid1_uv.w );
	drawPts[4].Set( mid2_uv.z, mid2_uv.w );
	drawPts[5].Set( trans.center.z, trans.center.w );
	for ( int j = 0; j < 3; j++ ) {
		drawPts[j+3].x *= shiftScale.x;
		drawPts[j+3].y *= shiftScale.y;
	}
	renderSystem->DrawStretchTri( drawPts[0], drawPts[1], drawPts[2], drawPts[3], drawPts[4], drawPts[5], material );
}

/*
==================
FullscreenFX_Warp::HighQuality
==================
*/
void FullscreenFX_Warp::HighQuality() {
	float x1, y1, x2, y2, radius, interp;
	idVec2 center;
	int STEP = 9;

	interp = ( idMath::Sin( (float)( gameLocal.slow.time - startWarpTime ) / 1000 ) + 1 ) / 2.f;
	interp = 0.7 * ( 1 - interp ) + 0.3 * ( interp );

	// draw the warps
	center.x = 320;
	center.y = 240;
	radius = 200;

	for ( float i = 0; i < 360; i += STEP ) {
		// compute the values
		x1 = idMath::Sin( DEG2RAD( i ) );
		y1 = idMath::Cos( DEG2RAD( i ) );

		x2 = idMath::Sin( DEG2RAD( i + STEP ) );
		y2 = idMath::Cos( DEG2RAD( i + STEP ) );

		// add warp polygon
		WarpPolygon_t p;

		p.outer1.x = center.x + x1 * radius;
		p.outer1.y = center.y + y1 * radius;
		p.outer1.z = p.outer1.x / 640.f;
		p.outer1.w = 1 - ( p.outer1.y / 480.f );

		p.outer2.x = center.x + x2 * radius;
		p.outer2.y = center.y + y2 * radius;
		p.outer2.z = p.outer2.x / 640.f;
		p.outer2.w = 1 - ( p.outer2.y / 480.f );

		p.center.x = center.x;
		p.center.y = center.y;
		p.center.z = p.center.x / 640.f;
		p.center.w = 1 - ( p.center.y / 480.f );

		// draw it
		DrawWarp( p, interp );
	}
}





/*
==================
FullscreenFX_EnviroSuit::Initialize
==================
*/
void FullscreenFX_EnviroSuit::Initialize() {
	material = declManager->FindMaterial( "textures/smf/enviro_suit" );
}

/*
==================
FullscreenFX_EnviroSuit::Active
==================
*/
bool FullscreenFX_EnviroSuit::Active() {
	idPlayer *player;

	player = fxman->GetPlayer();

	if ( player->PowerUpActive( ENVIROSUIT ) )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_EnviroSuit::HighQuality
==================
*/
void FullscreenFX_EnviroSuit::HighQuality() {
	renderSystem->SetColor4( 1, 1, 1, 1 );
	renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, material );
}





/*
==================
FullscreenFX_DoubleVision::Initialize
==================
*/
void FullscreenFX_DoubleVision::Initialize()
{
	material = declManager->FindMaterial( "textures/smf/doubleVision" );
}

/*
==================
FullscreenFX_DoubleVision::Active
==================
*/
bool FullscreenFX_DoubleVision::Active()
{
	idPlayer *player;
	player = fxman->GetPlayer();
	if (player->spectating)
	{
		return false;
	}

	if ( gameLocal.fast.time < fxman->GetPlayerView()->dvFinishTime )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_DoubleVision::HighQuality
==================
*/
void FullscreenFX_DoubleVision::HighQuality()
{
	int offset = fxman->GetPlayerView()->dvFinishTime - gameLocal.fast.time;
	
	//bc
	//float offset = (idMath::Sin(gameLocal.time * .001f) * 4.0f) + 16.0f;

	float	scale = offset * g_dvAmplitude.GetFloat();
	idPlayer *player;
	idVec2 shiftScale;

	// for testing purposes
	if ( !Active() )
	{
		static int test = 0;
		if ( test > 312 )
		{
			test = 0;
		}

		offset = test++;
		scale = offset * g_dvAmplitude.GetFloat();
	}

	player = fxman->GetPlayer();
	shiftScale = fxman->GetShiftScale();

	offset *= 2;		// crutch up for higher res

	// set the scale and shift
	if ( scale > 0.5f )
	{
		scale = 0.5f;
	}

	float shift = scale * sin( sqrtf( (float)offset ) * g_dvFrequency.GetFloat() );
	shift = fabs( shift );

	// carry red tint if in berserk mode
	idVec4 color(1, 1, 1, 1);
	if ( gameLocal.fast.time < player->inventory.powerupEndTime[ BERSERK ] )
	{
		color.y = 0;
		color.z = 0;
	}

	if ( !gameLocal.isMultiplayer && (gameLocal.fast.time < player->inventory.powerupEndTime[ HELLTIME ] || gameLocal.fast.time < player->inventory.powerupEndTime[ INVULNERABILITY ]))
	{
		color.y = 0;
		color.z = 0;
	}

	renderSystem->SetColor4( color.x, color.y, color.z, 1.0f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, shift, shiftScale.y, shiftScale.x, 0, material );
	renderSystem->SetColor4( color.x, color.y, color.z, 0.5f );
	renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, shiftScale.y, (1-shift) * shiftScale.x, 0, material );
}




/*
==================
FullscreenFX_InfluenceVision::Initialize
==================
*/
void FullscreenFX_InfluenceVision::Initialize() {

}

/*
==================
FullscreenFX_InfluenceVision::Active
==================
*/
bool FullscreenFX_InfluenceVision::Active() {
	idPlayer *player;

	player = fxman->GetPlayer();

	if ( player->GetInfluenceMaterial() || player->GetInfluenceEntity() )
	{
		return true;
	}

	return false;
}

/*
==================
FullscreenFX_InfluenceVision::HighQuality
==================
*/
void FullscreenFX_InfluenceVision::HighQuality() {
	float distance = 0.0f;
	float pct = 1.0f;
	idPlayer *player;
	idVec2 shiftScale;

	shiftScale = fxman->GetShiftScale();
	player = fxman->GetPlayer();

	if ( player->GetInfluenceEntity() ) {
		distance = ( player->GetInfluenceEntity()->GetPhysics()->GetOrigin() - player->GetPhysics()->GetOrigin() ).Length();
		if ( player->GetInfluenceRadius() != 0.0f && distance < player->GetInfluenceRadius() ) {
			pct = distance / player->GetInfluenceRadius();
			pct = 1.0f - idMath::ClampFloat( 0.0f, 1.0f, pct );
		}
	}

	if ( player->GetInfluenceMaterial() ) {
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, pct );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, player->GetInfluenceMaterial() );
	} else if ( player->GetInfluenceEntity() == NULL ) {
		return;
	} else {
//		int offset =  25 + sinf( gameLocal.slow.time );
//		DoubleVision( hud, view, pct * offset );
	}
}




/*
==================
FullscreenFX_Bloom::Initialize
==================
*/
void FullscreenFX_Bloom::Initialize() {
	drawMaterial		= declManager->FindMaterial( "textures/smf/bloom2/draw" );
	initMaterial		= declManager->FindMaterial( "textures/smf/bloom2/init" );
	currentMaterial		= declManager->FindMaterial( "textures/smf/bloom2/currentMaterial" );

	ftlVignetteMaterial = declManager->FindMaterial("textures/fx/ftl_vignette");

	currentIntensity	= 0;
	targetIntensity		= 0;
}

/*
==================
FullscreenFX_Bloom::Active
==================
*/
bool FullscreenFX_Bloom::Active() {
	idPlayer *player;
	player = fxman->GetPlayer();

	if (player->spectating)
	{
		return false;
	}


	if ( player && player->bloomEnabled )
	{
		return true;
	}

	//if (static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetFTLDrive.IsValid())
	//{
	//	idEntity *ftlEnt = static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->GetFTLDrive.GetEntity();
	//	return static_cast<idFTL *>(ftlEnt)->IsJumpActive(false);
	//}

	return false;
}

/*
==================
FullscreenFX_Bloom::HighQuality
==================
*/
void FullscreenFX_Bloom::HighQuality() {
	float shift, delta;
	idVec2 shiftScale;
	idPlayer *player;
	int num;

	shift = 1;
	player = fxman->GetPlayer();
	shiftScale = fxman->GetShiftScale();
	renderSystem->SetColor4( 1, 1, 1, 1 );

	// if intensity value is different, start the blend
	targetIntensity = g_testBloomIntensity.GetFloat();

	if ( player && player->bloomEnabled ) {
		targetIntensity = player->bloomIntensity;
	}

	delta = targetIntensity - currentIntensity;
	float step = 0.001f;

	if ( step < fabs( delta ) ) {
		if ( delta < 0 ) {
			step = -step;
		}

		currentIntensity += step;
	}

	// draw the blends
	num = g_testBloomNumPasses.GetInteger();

	for ( int i = 0; i < num; i++ ) {
		float s1 = 0, t1 = 0, s2 = 1, t2 = 1;
		float alpha;

		// do the center scale
		s1 -= 0.5;
		s1 *= shift;
		s1 += 0.5;
		s1 *= shiftScale.x;

		t1 -= 0.5;
		t1 *= shift;
		t1 += 0.5;
		t1 *= shiftScale.y;

		s2 -= 0.5;
		s2 *= shift;
		s2 += 0.5;
		s2 *= shiftScale.x;

		t2 -= 0.5;
		t2 *= shift;
		t2 += 0.5;
		t2 *= shiftScale.y;

		// draw it
		if ( num == 1 ) {
			alpha = 1;
		}
		else {
			alpha = 1 - (float)i / ( num - 1 );
		}

		renderSystem->SetColor4( alpha, alpha, alpha, 1 );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, s1, t2, s2, t1, drawMaterial );

		shift += currentIntensity;
	}

	renderSystem->SetColor4(1, 1, 1, 1);
	renderSystem->DrawStretchPic(0, 0, 640, 480, 0.0f, 0.0f, 1.0f, 1.0f, ftlVignetteMaterial);
}

/*
==================
FullscreenFX_Bloom::Save
==================
*/
void FullscreenFX_Bloom::Save( idSaveGame *savefile ) {
	FullscreenFX::Save( savefile );
	savefile->WriteFloat( currentIntensity );
	savefile->WriteFloat( targetIntensity );
}

/*
==================
FullscreenFX_Bloom::Restore
==================
*/
void FullscreenFX_Bloom::Restore( idRestoreGame *savefile ) {
	FullscreenFX::Restore( savefile );
	savefile->ReadFloat( currentIntensity );
	savefile->ReadFloat( targetIntensity );
}





//BC
void FullscreenFX_DownedState::Initialize()
{
	material = declManager->FindMaterial("textures/fx/desaturate");
}

bool FullscreenFX_DownedState::Active()
{
	idPlayer *player;
	player = fxman->GetPlayer();

	if (player->spectating)
	{
		return false;
	}


	return (player->inDownedState && player->health > 0); //This determines when the downedstate filter is active.
}

void FullscreenFX_DownedState::HighQuality()
{
	renderSystem->SetColor4(1, 1, 1, 1);
	renderSystem->DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, material);
}







// Roq video playerview.

void FullscreenFX_RoqVideo::Initialize()
{
	roqTimer = 0;
	lastActiveState = false;
	material = declManager->FindMaterial("textures/fx/video_bokeh_a");
}

bool FullscreenFX_RoqVideo::Active()
{
	idPlayer* player;
	player = fxman->GetPlayer();
	return (player->GetROQVideoStateActive() && player->health > 0);
	
}

void FullscreenFX_RoqVideo::HighQuality()
{
	if (!lastActiveState)
	{
		lastActiveState = true;
		roqTimer = gameLocal.time + 1066;
	
		material->ResetCinematicTime(0);
	}
	
	if (gameLocal.time > roqTimer)
	{
		lastActiveState = false;
		gameLocal.GetLocalPlayer()->SetROQVideoState(0);
		return;
	}
	
	renderSystem->SetColor4(1, 1, 1, 1);
	renderSystem->DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, material);
}










void FullscreenFX_BulletwoundVision::Initialize() {
    material = declManager->FindMaterial("textures/fx/blur_bulletwound");
    state = CONC_IDLE;
    stateTimer = 0;
    rampTime = 0;
}

bool FullscreenFX_BulletwoundVision::Active() {

    idPlayer *player;
    player = fxman->GetPlayer();

    if (player->spectating)
    {
        return false;
    }

    //return (player->GetBulletwoundCount() > 0 && player->health > 0);
	return (player->GetShrapnelCount() > 0 && player->health > 0);
}

void FullscreenFX_BulletwoundVision::HighQuality() {
    if (state == CONC_IDLE)
    {
        idPlayer *player;
        player = fxman->GetPlayer();
        if (player->GetPhysics()->GetLinearVelocity().Length() > 0 && gameLocal.time > lastRampdownTime + CONCUSS_COOLDOWNTIME)
        {
            state = CONC_RAMPUP;
            rampTime = CONCUSS_RAMPMIN + gameLocal.random.RandomInt(CONCUSS_RAMPVAR);
            stateTimer = gameLocal.fast.time + rampTime;
        }

		DrawHQ(CONCUSS_MINBLUR);
    }
    else if (state == CONC_RAMPUP)
    {
        float lerp = (stateTimer - gameLocal.fast.time) / (float)rampTime;
        lerp = idMath::ClampFloat(0, 1, 1.0f - lerp);
        lerp = idMath::CubicEaseOut(lerp);

		lerp = idMath::Lerp(CONCUSS_MINBLUR, 1.0f, lerp);
        DrawHQ(lerp);

        if (gameLocal.fast.time > stateTimer)
        {
            state = CONC_DOUBLEVISION;
            stateTimer = gameLocal.fast.time + CONCUSS_DOUBLEVISTIMEMIN + gameLocal.random.RandomInt(CONCUSS_DOUBLEVISTIMEVAR);

            //Do the bulletwound icon flash.
            idPlayer *player;
            player = fxman->GetPlayer();
            if (player)
            {
                //player->hud->HandleNamedEvent("cond_bulletdamage"); //make the bulletwound icon do a flash.
				player->hud->HandleNamedEvent("cond_shrapneldamage"); //make the bulletwound icon do a flash.				
            }
        }
    }
    else if (state == CONC_DOUBLEVISION)
    {
		if (gameLocal.fast.time > stateTimer)
		{
			idPlayer *player;
			player = fxman->GetPlayer();
			if (player->GetPhysics()->GetLinearVelocity().Length() <= 0)
			{
				state = CONC_RAMPDOWN;
				rampTime = CONCUSS_RAMPMIN + gameLocal.random.RandomInt(CONCUSS_RAMPVAR);
				stateTimer = gameLocal.fast.time + rampTime;
			}
		}

        DrawHQ(1.0f);
    }
    else if (state == CONC_RAMPDOWN)
    {
        float lerp = (stateTimer - gameLocal.fast.time) / (float)rampTime;
        lerp = idMath::ClampFloat(0, 1, 1.0f - lerp);
        lerp = idMath::CubicEaseIn(lerp);

		lerp = idMath::Lerp( 1.0f, CONCUSS_MINBLUR,  lerp);
        DrawHQ(lerp);

        if (gameLocal.fast.time > stateTimer)
        {
            state = CONC_IDLE;
            lastRampdownTime = gameLocal.time;
            //stateTimer = gameLocal.fast.time + CONCUSS_IDLEMIN + gameLocal.random.RandomInt(CONCUSS_IDLEVAR);
        }
    }


}

#define BULLETWOUND_MAXBLURPASSES 8
void FullscreenFX_BulletwoundVision::DrawHQ(float lerp)
{

    float startIntensity = idMath::Lerp(0, g_blur_intensity.GetFloat(), lerp);
    
    
    float delta = startIntensity / BULLETWOUND_MAXBLURPASSES;
    idList<idVec2> directions;
    directions.Append(idVec2(startIntensity, 0.0f));
    for (int i = 1; i < BULLETWOUND_MAXBLURPASSES; i++) {
        float amount = startIntensity - delta * i;
        directions.Append(idVec2(i % 2 == 0 ? amount : 0.0f, i % 2 == 1 ? amount : 0.0f));
    }
    
    int startIteration = BULLETWOUND_MAXBLURPASSES - g_blur_numIterations.GetInteger();
    for (int i = startIteration; i < BULLETWOUND_MAXBLURPASSES; i++) {
		if (i != startIteration)
		{
			renderSystem->PrepareForPostProcessPass();
		}
        renderSystem->SetColor4(directions[i].x, directions[i].y, 1, 1);
        renderSystem->DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, material);
    }
}







#define BLUR_LERPTIME 200
// SM
void FullscreenFX_Blur::Initialize() {
	material = declManager->FindMaterial( "textures/fx/blur" );

	blurActive = false;
	blurLastActive = false;
	startTime = 0;
}

bool FullscreenFX_Blur::Active() {

	idPlayer *player;
	player = fxman->GetPlayer();

	if (player->spectating)
	{
		return false;
	}

	if (player->IsBlurActive())
	{
		if (!blurActive)
		{
			//Turn on.
			blurActive = true;
			startTime = gameLocal.hudTime;
		}
	}
	else
	{
		if (blurActive)
		{
			//Turn off.
			blurActive = false;
			startTime = gameLocal.hudTime;
		}
	}

	return (blurActive || (!blurActive && (startTime + BLUR_LERPTIME > gameLocal.hudTime)));
}

void FullscreenFX_Blur::HighQuality() {
	// Calculate directions vector based on parameters

	//float startIntensity = g_blur_intensity.GetFloat();

	float lerp = (gameLocal.hudTime - startTime) / (float)BLUR_LERPTIME;
	lerp = idMath::ClampFloat(0, 1, lerp);
	if (!blurActive)
	{
		lerp = 1.0f - lerp;
	}

	float startIntensity = idMath::Lerp(0, g_blur_intensity.GetFloat(), lerp);


	float delta = startIntensity / MAX_BLUR_PASSES;
	idList<idVec2> directions;
	directions.Append( idVec2( startIntensity, 0.0f ) );
	for ( int i = 1; i < MAX_BLUR_PASSES; i++ ) {
		float amount = startIntensity - delta * i;
		directions.Append( idVec2(i % 2 == 0 ? amount : 0.0f, i % 2 == 1 ? amount : 0.0f) );
	}

	int startIteration = MAX_BLUR_PASSES - g_blur_numIterations.GetInteger();
	for ( int i = startIteration; i < MAX_BLUR_PASSES; i++ ) {
		if (i != startIteration)
		{
			renderSystem->PrepareForPostProcessPass();
		}
		renderSystem->SetColor4( directions[i].x, directions[i].y, 1, 1 );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, material );
	}
}


//========================= BRIGHTNESS ===============================

// SM
void FullscreenFX_Brightness::Initialize()
{
	material = declManager->FindMaterial( "textures/fx/brightness" );
}

bool FullscreenFX_Brightness::Active()
{
	return true;
}

void FullscreenFX_Brightness::HighQuality()
{
	//Illuminate items when inspecting them in dark environments. So that it's possible to read things when nearby lights are dark.

	// First parameter is overall brightness
	// Second parameter is what threshold is considered "low luminance" for inspected items
	// Third parameter is extra multiplier for low luminance inspected items
	renderSystem->SetColor4(
		cvarSystem->GetCVarFloat( "r_brightness" ),	//overall brightness
		0.3f,	//what threshold is considered "low luminance" for inspected items
		1.8f,	//extra multiplier for low luminance inspected items (calculated 1 + pct below low)
		1.0f );	//(this number does nothing)
	renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, material );
}





//============================ FULLSCREEN BLUR FOR TELESCOPE =================================


void FullscreenFX_TelescopeBlur::Initialize() {
	material = declManager->FindMaterial("textures/fx/blur_full");
}

bool FullscreenFX_TelescopeBlur::Active() {

	idPlayer *player;
	player = fxman->GetPlayer();

	if (player->spectating)
	{
		return false;
	}

	if (player->peekObject.IsValid())
	{		
		if (static_cast<idVentpeek*>(player->peekObject.GetEntity())->forTelescope)
		{
			if (player->peekObject.GetEntity()->IsType(idVentpeekTelescope::Type))
			{
				blurLerp = static_cast<idVentpeekTelescope*>(player->peekObject.GetEntity())->GetCurrentBlur();
				
				return (blurLerp > 0);
			}
		}
	}

	return false;
}

void FullscreenFX_TelescopeBlur::HighQuality() {
	// Calculate directions vector based on parameters

	//float startIntensity = g_blur_intensity.GetFloat();

	float lerp = blurLerp;

	float startIntensity = idMath::Lerp(0, g_blur_intensity.GetFloat(), lerp);

	float delta = startIntensity / MAX_TELESCOPEBLUR_PASSES;
	idList<idVec2> directions;
	directions.Append(idVec2(startIntensity, 0.0f));
	for (int i = 1; i < MAX_TELESCOPEBLUR_PASSES; i++) {
		float amount = startIntensity - delta * i;
		directions.Append(idVec2(i % 2 == 0 ? amount : 0.0f, i % 2 == 1 ? amount : 0.0f));
	}

	int startIteration = MAX_TELESCOPEBLUR_PASSES - g_blur_numIterations.GetInteger();
	for (int i = startIteration; i < MAX_TELESCOPEBLUR_PASSES; i++) {
		if (i != startIteration)
		{
			renderSystem->PrepareForPostProcessPass();
		}
		renderSystem->SetColor4(directions[i].x, directions[i].y, 1, 1);
		renderSystem->DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, material);
	}
}

void FullscreenFX_MotionBlur::Initialize()
{
	material = declManager->FindMaterial( "textures/fx/motionblur_blendo" );
}

bool FullscreenFX_MotionBlur::Active()
{
	return g_motionBlur_testEnable.GetBool();
}

void FullscreenFX_MotionBlur::HighQuality()
{
	renderSystem->SetColor4( g_motionBlur_intensity.GetFloat(), 1.0f, 1.0f, 1.0f );
	renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, material );
}


/*
==================
FullscreenFXManager::FullscreenFXManager
==================
*/
FullscreenFXManager::FullscreenFXManager() {
	highQualityMode = false;
	playerView = NULL;
	blendBackMaterial = NULL;
	shiftScale.Set( 0, 0 );
}

/*
==================
FullscreenFXManager::~FullscreenFXManager
==================
*/
FullscreenFXManager::~FullscreenFXManager() {

}

/*
==================
FullscreenFXManager::FindFX
==================
*/
FullscreenFX* FullscreenFXManager::FindFX( idStr name ) {
	for ( int i = 0; i < fx.Num(); i++ ) {
		if ( fx[i]->GetName() == name ) {
			return fx[i];
		}
	}

	return NULL;
}

/*
==================
FullscreenFXManager::CreateFX
==================
*/
void FullscreenFXManager::CreateFX( idStr name, idStr fxtype, int fade ) {
	FullscreenFX *pfx = NULL;

	if ( fxtype == "helltime" ) {
		pfx = new FullscreenFX_Helltime;
	}
	else if ( fxtype == "warp" ) {
		pfx = new FullscreenFX_Warp;
	}
	else if ( fxtype == "envirosuit" )
	{
		pfx = new FullscreenFX_EnviroSuit;
	}
	else if ( fxtype == "doublevision" ) {
		pfx = new FullscreenFX_DoubleVision;
	}
	else if ( fxtype == "multiplayer" ) {
		pfx = new FullscreenFX_Multiplayer;
	}
	else if ( fxtype == "influencevision" ) {
		pfx = new FullscreenFX_InfluenceVision;
	}
	else if ( fxtype == "bloom" ) {
		pfx = new FullscreenFX_Bloom;
	}
	else if (fxtype == "downedstate") //BC
	{
		pfx = new FullscreenFX_DownedState;
	}
	else if (fxtype == "concussed") //BC
	{
		pfx = new FullscreenFX_ConcussVision;
	}
    else if (fxtype == "bulletwound") //BC
    {
        pfx = new FullscreenFX_BulletwoundVision;
    }
	else if (fxtype == "blur") // SM
	{
		pfx = new FullscreenFX_Blur;
	}
	else if (fxtype == "gasvision") //BC
	{
		pfx = new FullscreenFX_GasVision;
	}
	else if (fxtype == "brightness") // SM
	{
		pfx = new FullscreenFX_Brightness;
	}
	else if (fxtype == "telescopeblur")
	{
		pfx = new FullscreenFX_TelescopeBlur;
	}
	else if (fxtype == "motionblur")
	{
		pfx = new FullscreenFX_MotionBlur;
	}
	else if (fxtype == "zoommode") //BC
	{
		pfx = new FullscreenFX_ZoomMode;
	}
	else if (fxtype == "roqvideo") //BC
	{
		pfx = new FullscreenFX_RoqVideo;
	}
	else if (fxtype == "outline")
	{
		pfx = new FullscreenFX_Outline;
	}
	else
	{
		assert( 0 );
	}

	if ( pfx ) {
		pfx->Initialize();
		pfx->SetFXManager( this );
		pfx->SetName( name );
		pfx->SetFadeSpeed( fade );
		fx.Append( pfx );
	}
}

/*
==================
FullscreenFXManager::Initialize
==================
*/
void FullscreenFXManager::Initialize( idPlayerView *pv ) {
	// set the playerview
	playerView = pv;
	blendBackMaterial = declManager->FindMaterial( "textures/smf/blendBack" );

	// allocate the fx
	CreateFX( "helltime", "helltime", 1000 );
	CreateFX( "warp", "warp", 0 );
	CreateFX( "envirosuit", "envirosuit", 500 );
	//CreateFX( "doublevision", "doublevision", 0 ); // SM: Disable doublevision because it looks bad
	CreateFX( "multiplayer", "multiplayer", 1000 );
	CreateFX( "influencevision", "influencevision", 1000 );
	CreateFX( "bloom", "bloom", 0 );
	CreateFX( "downedstate", "downedstate", 0);
	CreateFX("concussed", "concussed", 0);
	CreateFX( "blur", "blur", 0 );
	CreateFX("gasvision", "gasvision", 0);
    CreateFX("bulletwound", "bulletwound", 0);
	CreateFX("telescopeblur", "telescopeblur", 0);
	CreateFX( "motionblur", "motionblur", 0 );
	CreateFX("zoommode", "zoommode", 0);
	CreateFX("roqvideo", "roqvideo", 0);
	CreateFX("outline", "outline", 0);

	// SM -- this NEEDS to be the last post effect
	CreateFX("brightness", "brightness", 0);

	// pre-cache the texture grab so we dont hitch
	renderSystem->CropRenderSize( 512, 512, true );
	renderSystem->CaptureRenderToImage( "_accum" );
	renderSystem->UnCrop();

	renderSystem->CropRenderSize( 512, 256, true );
	renderSystem->CaptureRenderToImage( "_scratch" );
	renderSystem->UnCrop();

	renderSystem->CaptureRenderToImage( "_currentRender" );
}

/*
==================
FullscreenFXManager::Blendback
==================
*/
void FullscreenFXManager::Blendback( float alpha ) {
	// alpha fade
	if ( alpha < 1.f ) {
		renderSystem->SetColor4( 1, 1, 1, 1 - alpha );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, shiftScale.y, shiftScale.x, 0.f, blendBackMaterial );
	}
}

/*
==================
FullscreenFXManager::Save
==================
*/
void FullscreenFXManager::Save( idSaveGame *savefile ) {

	for ( int i = 0; i < fx.Num(); i++ ) { 	//idList<FullscreenFX*>	fx;
		FullscreenFX *pfx = fx[i];
		pfx->Save( savefile );
	}


	savefile->WriteBool( highQualityMode ); // bool highQualityMode
	savefile->WriteVec2( shiftScale ); // idVec2 shiftScale

	//idPlayerView *playerView  // owned by playerview

	savefile->WriteMaterial( blendBackMaterial ); //const idMaterial* blendBackMaterial;
}

/*
==================
FullscreenFXManager::Restore
==================
*/
void FullscreenFXManager::Restore( idRestoreGame *savefile ) {

	for ( int i = 0; i < fx.Num(); i++ ) {  	//idList<FullscreenFX*>	fx
		FullscreenFX *pfx = fx[i];
		pfx->Restore( savefile );
	}

	savefile->ReadBool( highQualityMode ); // bool highQualityMode
	savefile->ReadVec2( shiftScale ); // idVec2 shiftScale

	//idPlayerView *playerView // owned by playerview


	savefile->ReadMaterial( blendBackMaterial ); //const idMaterial* blendBackMaterial;
}

/*
==================
FullscreenFXManager::CaptureCurrentRender
==================
*/
void FullscreenFXManager::CaptureCurrentRender() {
	renderSystem->PrepareForPostProcessPass();
}

/*
==================
FullscreenFXManager::Process
==================
*/
void FullscreenFXManager::Process( const renderView_t *view ) {
	bool allpass = false;

	if ( g_testFullscreenFX.GetInteger() == -2 ) {
		allpass = true;
	}

	if ( g_lowresFullscreenFX.GetBool() ) {
		highQualityMode = false;
	}
	else {
		highQualityMode = true;
	}

	// compute the shift scale
	if ( highQualityMode ) {
		int vidWidth, vidHeight;
		renderSystem->GetGLSettings( vidWidth, vidHeight );

		float pot;
		int	 w = vidWidth;
		pot = MakePowerOfTwo( w );
		shiftScale.x = (float)w / pot;

		int	 h = vidHeight;
		pot = MakePowerOfTwo( h );
		shiftScale.y = (float)h / pot;
	}
	else {
		// if we're in low-res mode, shrink view down
		shiftScale.x = 1;
		shiftScale.y = 1;
		renderSystem->CropRenderSize( 512, 512, true );
	}

	// do the first render
	gameRenderWorld->RenderScene( view );

	// do the process
	for ( int i = 0; i < fx.Num(); i++ ) {
		FullscreenFX *pfx = fx[i];
		bool drawIt = false;

		// determine if we need to draw
		if ( pfx->Active() || g_testFullscreenFX.GetInteger() == i || allpass ) {
			drawIt = pfx->SetTriggerState( true );
		}
		else {
			drawIt = pfx->SetTriggerState( false );
		}

		// do the actual drawing
		if ( drawIt ) {
			// we need to dump to _currentRender
			CaptureCurrentRender();

			// handle the accum pass if we have one
			if ( pfx->HasAccum() ) {

				// if we're in high quality mode, we need to crop the accum pass
				if ( highQualityMode ) {
					renderSystem->CropRenderSize( 512, 512, true );
					pfx->AccumPass( view );
					renderSystem->UnCrop();
				}
				else {
					pfx->AccumPass( view );
				}
			}

			// do the high quality pass
			pfx->HighQuality();

			// do the blendback
			Blendback( pfx->GetFadeAlpha() );
		}
	}

	if ( !highQualityMode ) {
		// we need to dump to _currentRender
		CaptureCurrentRender();

		// uncrop view
		renderSystem->UnCrop();

		// draw the final full-screen image
		renderSystem->SetColor4( 1, 1, 1, 1 );
		renderSystem->DrawStretchPic( 0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 1, 1, 0.f, blendBackMaterial );
	}
}

void idPlayerView::SetBloodbagOverlay()
{
	bloodbagOverlayActive = true;
	bloodbagState = BAGSTATE_SLIDING;
	bloodbagTimer = gameLocal.time;
}

void idPlayerView::DoSightedFlash()
{
	sightedTimer = gameLocal.time + SIGHTED_FLASHTIME;
	Flash(idVec4(1, 1, 1, .2f), SIGHTED_FLASHTIME);
}






void FullscreenFX_ConcussVision::Initialize()
{
	material = declManager->FindMaterial("textures/smf/doubleVision");
	state = CONC_IDLE;
	stateTimer = 0;
	rampTime = 0;
}


bool FullscreenFX_ConcussVision::Active()
{
	//idPlayer *player;
	//player = fxman->GetPlayer();
    //
	//if (player->spectating)
	//{
	//	return false;
	//}
    //
	//return (player->GetBulletwoundCount() > 0 && player->health > 0);

    return false;
}

void FullscreenFX_ConcussVision::HighQuality()
{
	idVec2 shiftScale = fxman->GetShiftScale();	

	float shift = 0;

	if (state == CONC_IDLE)
	{
		idPlayer *player;
		player = fxman->GetPlayer();
		if (player->GetPhysics()->GetLinearVelocity().Length() > 0 && gameLocal.time > lastRampdownTime + CONCUSS_COOLDOWNTIME)
		{
			state = CONC_RAMPUP;
			rampTime = CONCUSS_RAMPMIN + gameLocal.random.RandomInt(CONCUSS_RAMPVAR);
			stateTimer = gameLocal.fast.time + rampTime;
		}
	}
	else if (state == CONC_RAMPUP)
	{
		float lerp = (stateTimer  - gameLocal.fast.time) / (float)rampTime;
		lerp = idMath::ClampFloat(0, 1, 1.0f - lerp);
		lerp = idMath::CubicEaseOut(lerp);
		shift = idMath::Lerp(0, CONCUSS_MAXSHIFT, lerp);

		if (gameLocal.fast.time > stateTimer)
		{
			state = CONC_DOUBLEVISION;
			//stateTimer = gameLocal.fast.time + CONCUSS_DOUBLEVISTIMEMIN + gameLocal.random.RandomInt(CONCUSS_DOUBLEVISTIMEVAR);

			//Do the bulletwound icon flash.
			idPlayer *player;
			player = fxman->GetPlayer();
			if (player)
			{
				player->hud->HandleNamedEvent("cond_bulletdamage"); //make the bulletwound icon do a flash.
			}
		}
	}
	else if (state == CONC_DOUBLEVISION)
	{
		shift = CONCUSS_MAXSHIFT;

		idPlayer *player;
		player = fxman->GetPlayer();
		if (player->GetPhysics()->GetLinearVelocity().Length() <= 0)
		{
			state = CONC_RAMPDOWN;
			rampTime = CONCUSS_RAMPMIN + gameLocal.random.RandomInt(CONCUSS_RAMPVAR);
			stateTimer = gameLocal.fast.time + rampTime;
		}
	}
	else if (state == CONC_RAMPDOWN)
	{
		float lerp = (stateTimer - gameLocal.fast.time) / (float)rampTime;
		lerp = idMath::ClampFloat(0, 1, 1.0f - lerp);
		lerp = idMath::CubicEaseIn(lerp);
		shift = idMath::Lerp(CONCUSS_MAXSHIFT, 0, lerp);
		if (gameLocal.fast.time > stateTimer)
		{
			state = CONC_IDLE;
			lastRampdownTime = gameLocal.time;
			//stateTimer = gameLocal.fast.time + CONCUSS_IDLEMIN + gameLocal.random.RandomInt(CONCUSS_IDLEVAR);
		}
	}

	if (shift <= 0)
		return;

	idVec4 color(1, 1, 1, 1);
	renderSystem->SetColor4(color.x, color.y, color.z, 1.0f);
	renderSystem->DrawStretchPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, shift, shiftScale.y, shiftScale.x, 0, material);
	renderSystem->SetColor4(color.x, color.y, color.z, 0.5f);
	renderSystem->DrawStretchPic(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, shiftScale.y, (1 - shift) * shiftScale.x, 0, material);
}




//Gas vision.

#define EYELID_CLOSETIME 140
#define EYELID_OPENTIME 70
#define EYELID_BLINK_RANDTIME_MIN 200
#define EYELID_BLINK_RANDTIME_MAX 600

void FullscreenFX_GasVision::Initialize()
{
	material = declManager->FindMaterial("textures/fx/gasvision_eyelids"); //static overlay.
	lidMaterial = declManager->FindMaterial("textures/fx/gasvision_lid"); //the part that animates.
	blurMaterial = declManager->FindMaterial("textures/fx/blur_gas");
	stateTimer = 0;
	state = GV_IDLE;
}


bool FullscreenFX_GasVision::Active()
{
	idPlayer *player;
	player = fxman->GetPlayer();

	if (player->spectating)
	{
		return false;
	}

	return player->cond_gascloud;
}

void FullscreenFX_GasVision::HighQuality()
{
	//renderSystem->SetColor4(0, 0, 0, 0.9f);
	#define GASBLUR_PASSES 8
	#define GASBLUR_AMOUNT 7
	idList<idVec2> directions;
	directions.Append(idVec2(1.0f, 0.0f));
	for (int i = 1; i < GASBLUR_PASSES; i++) {
		float amount = GASBLUR_AMOUNT;
		directions.Append(idVec2(i % 2 == 0 ? amount : 0.0f, i % 2 == 1 ? amount : 0.0f));
	}
	for (int i = 0; i < GASBLUR_PASSES; i++) {
		if (i != 0)
		{
			renderSystem->PrepareForPostProcessPass();
		}
		renderSystem->SetColor4(directions[i].x, directions[i].y, 1, 1);
		renderSystem->DrawStretchPic(0.0f, 0.0f, 640.0f, 480.0f, 0.0f, 0.0f, 1.0f, 1.0f, blurMaterial);
	}


	//overlay.
	renderSystem->SetColor4(0,0,0,0.9f);
	renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, material); //static overlay.

	if (state == GV_IDLE)
	{
		if (gameLocal.time > stateTimer)
		{
			stateTimer = gameLocal.time + EYELID_CLOSETIME;
			state = GV_CLOSING;
		}
	}
	else if (state == GV_CLOSING)
	{
		float lerp = (stateTimer - gameLocal.time) / (float)EYELID_CLOSETIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = 1 - lerp;

		renderSystem->SetColor4(0, 0, 0, idMath::Lerp(0, 1, lerp));
		renderSystem->DrawStretchPic( 0.0f, 0.0f, SCREEN_WIDTH, idMath::Lerp(SCREEN_HEIGHT/3, SCREEN_HEIGHT * 1.8f, lerp), 0.0f, 0.0f, 1.0f, 1.0f, lidMaterial);

		if (gameLocal.time > stateTimer)
		{
			stateTimer = gameLocal.time + EYELID_OPENTIME;
			state = GV_OPENING;
		}
	}
	else if (state == GV_OPENING)
	{
		float lerp = (stateTimer - gameLocal.time) / (float)EYELID_OPENTIME;
		lerp = idMath::ClampFloat(0, 1, lerp);
		lerp = 1 - lerp;

		renderSystem->SetColor4(0, 0, 0, idMath::Lerp(1, 0, lerp));
		renderSystem->DrawStretchPic( 0.0f, 0.0f, SCREEN_WIDTH, idMath::Lerp(SCREEN_HEIGHT * 1.8f, SCREEN_HEIGHT / 3, lerp), 0.0f, 0.0f, 1.0f, 1.0f, lidMaterial);

		if (gameLocal.time > stateTimer)
		{
			stateTimer = gameLocal.time + gameLocal.random.RandomInt(EYELID_BLINK_RANDTIME_MIN, EYELID_BLINK_RANDTIME_MAX);
			state = GV_IDLE;
		}
	}
	

	
}








void FullscreenFX_ZoomMode::Initialize()
{
	material = declManager->FindMaterial("textures/fx/zoominspect"); //static overlay.
}


bool FullscreenFX_ZoomMode::Active()
{
	idPlayer *player = fxman->GetPlayer();

	return (player->isInZoomMode());
}

void FullscreenFX_ZoomMode::HighQuality()
{
	//overlay.
	renderSystem->SetColor4(1.0f, 0.8f, 0.0f, 2.0f); //xyz = color, w = line thickness
	renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, material);
}

void FullscreenFX_Outline::Initialize()
{
	material = declManager->FindMaterial("textures/fx/outline");
}

bool FullscreenFX_Outline::Active()
{
	if (!g_froboutline.GetBool())
		return false;
	
	return true;
}

void FullscreenFX_Outline::HighQuality()
{
	// The first 3 components are the color of the outline
	// The last component is the thickness
	renderSystem->SetColor4(.37f, 0.75f, 1.0f, 3.0f); //xyz = color, w = line thickness
	renderSystem->DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, material);
}

void idPlayerView::DoDurabilityFlash()
{
	durabilityflashActive = true;
	durabilityflashStartTime = gameLocal.fast.time;
}

void idPlayerView::UpdateDurabilityFlash()
{
	if (!durabilityflashActive)
		return;

	float flashLerp = (gameLocal.fast.time - durabilityflashStartTime) / (float)DURABILITYFLASH_TIME;
	flashLerp = idMath::ClampFloat(0, 1, flashLerp);

	idVec4 flashRect;
	flashRect.Lerp(idVec4(280, 190, 80, 100), idVec4(-580, -760, 1800, 2000), flashLerp); //lerp the size of the rect
	
	float flashAlpha = idMath::Lerp(.3f, 0, flashLerp);
	renderSystem->SetColor4(1, 1, 1, flashAlpha);
	renderSystem->DrawStretchPic(flashRect.x, flashRect.y, flashRect.z, flashRect.w, 0, 0, 1, 1, durabilityflashMaterial); //draw it.

	if (gameLocal.fast.time >= durabilityflashStartTime + DURABILITYFLASH_TIME)
	{
		durabilityflashActive = false; //timer expired.
	}
}

#endif
