
#include "../idlib/precompiled.h"
#pragma hdrstop

#include "prey_local.h"

#define LETTERBOX_HEIGHT_TOP	50
#define LETTERBOX_HEIGHT_BOTTOM	50

const int IMPULSE_DELAY = 150;

//HUMANHEAD rww - render demo madness
#if _HH_RENDERDEMO_HACKS
	void RENDER_DEMO_VIEWRENDER(const renderView_t *view, const hhPlayerView *pView) {
		if (!view) {
			return;
		}

		const renderView_t *v = view;

		if (pView) {
			static renderView_t hackedView = *view;
			hackedView.viewaxis = hackedView.viewaxis * pView->ShakeAxis();

			v = &hackedView;
		}

		renderSystem->LogViewRender(v);
	}

	void RENDER_DEMO_VIEWRENDER_END(void) {
		renderSystem->LogViewRender(NULL);
	}
#endif
//HUMANHEAD END

hhPlayerView::hhPlayerView() {
	// HUMANHEAD pdm: we don't use the tunnel vision or armor
	bLetterBox = false;
	letterboxMaterial = declManager->FindMaterial( "_black" );
	dirDmgLeftMaterial = declManager->FindMaterial( "textures/interface/directionalDamageLeft" );
	dirDmgFrontMaterial = declManager->FindMaterial( "textures/interface/directionalDamageFront" );
	spiritMaterial = NULL;
	viewOverlayMaterial = NULL;
	viewOverlayColor = colorWhite;
	voTotalTime = -1;
	voRequiresScratchBuffer = false;
	viewOffset.Zero();

	kickSpeed.Zero();
	kickLastTime = 0;
	hurtValue = 100.f;
}

void hhPlayerView::Save(idSaveGame *savefile) const {
	idPlayerView::Save( savefile );

	savefile->WriteBool( bLetterBox );
	savefile->WriteFloat( mbAmplitude );
	savefile->WriteInt( mbFinishTime );
	savefile->WriteInt( mbTotalTime );
	savefile->WriteVec3( mbDirection );
	savefile->WriteVec3( viewOffset );
	savefile->WriteMaterial( viewOverlayMaterial );
	savefile->WriteVec4( viewOverlayColor );
	savefile->WriteInt( voFinishTime );
	savefile->WriteInt( voTotalTime );
	savefile->WriteInt( voRequiresScratchBuffer );
	savefile->WriteMaterial( letterboxMaterial );
	savefile->WriteMaterial( dirDmgLeftMaterial );
	savefile->WriteMaterial( dirDmgFrontMaterial );
	savefile->WriteMaterial( spiritMaterial );
	savefile->WriteVec3( lastDamageLocation );
	savefile->WriteInt( kickLastTime - gameLocal.time );
	savefile->WriteAngles( kickSpeed );
	savefile->WriteFloat( hurtValue );
}

void hhPlayerView::Restore( idRestoreGame *savefile ) {
	idPlayerView::Restore( savefile );

	savefile->ReadBool( bLetterBox );
	savefile->ReadFloat( mbAmplitude );
	savefile->ReadInt( mbFinishTime );
	savefile->ReadInt( mbTotalTime );
	savefile->ReadVec3( mbDirection );
	savefile->ReadVec3( viewOffset );
	savefile->ReadMaterial( viewOverlayMaterial );
	savefile->ReadVec4( viewOverlayColor );
	savefile->ReadInt( voFinishTime );
	savefile->ReadInt( voTotalTime );
	savefile->ReadInt( voRequiresScratchBuffer );
	savefile->ReadMaterial( letterboxMaterial );
	savefile->ReadMaterial( dirDmgLeftMaterial );
	savefile->ReadMaterial( dirDmgFrontMaterial );
	savefile->ReadMaterial( spiritMaterial );
	savefile->ReadVec3( lastDamageLocation );
	savefile->ReadInt( kickLastTime );
	kickLastTime += gameLocal.time;
	savefile->ReadAngles( kickSpeed );
	savefile->ReadFloat( hurtValue );
}

#define MAX_SCREEN_BLOBS_SYNC			2//MAX_SCREEN_BLOBS

//rwwFIXME it may be possible to remove a lot of this junk by assuring events happen on the client as they do on the server
//concerning damage etc.
void hhPlayerView::WriteToSnapshot( idBitMsgDelta &msg ) const {
	msg.WriteFloat(viewOffset.x);
	msg.WriteFloat(viewOffset.y);
	msg.WriteFloat(viewOffset.z);

	msg.WriteBits(voTotalTime, 32);
	msg.WriteBits(voFinishTime, 32);
	msg.WriteBits(voRequiresScratchBuffer, 1);

	if (viewOverlayMaterial) {
		msg.WriteLong(gameLocal.ServerRemapDecl(-1, DECL_MATERIAL, viewOverlayMaterial->Index()));
	}
	else {
		msg.WriteLong(-1);
	}

	//write screen blobs to snapshot
	for (int i = 0; i < MAX_SCREEN_BLOBS_SYNC; i++) {
		const screenBlob_t	*blob = &screenBlobs[i];

		//i'm disabling this since it might work against delta compression being effective
		/*
		if ( blob->finishTime <= gameLocal.time ) {
			msg.WriteBits(0, 1); //no need to do anymore, continue
			continue;
		}

		//otherwise continue writing
		msg.WriteBits(1, 1);
		*/

		if (blob->material) {
			msg.WriteLong(gameLocal.ServerRemapDecl(-1, DECL_MATERIAL, blob->material->Index()));
		}
		else {
			msg.WriteLong(-1);
		}

		msg.WriteFloat(blob->x);
		msg.WriteFloat(blob->y);
		msg.WriteFloat(blob->w);
		msg.WriteFloat(blob->h);

		msg.WriteFloat(blob->s1);
		msg.WriteFloat(blob->t1);
		msg.WriteFloat(blob->s2);
		msg.WriteFloat(blob->t2);

		msg.WriteBits(blob->finishTime, 32);
		msg.WriteBits(blob->startFadeTime, 32);

		msg.WriteFloat(blob->driftAmount);
	}

	//motion blur
	msg.WriteFloat(mbAmplitude);
	msg.WriteBits(mbFinishTime, 32);
	msg.WriteBits(mbTotalTime, 32);
	msg.WriteFloat(mbDirection.x);
	msg.WriteFloat(mbDirection.y);
	msg.WriteFloat(mbDirection.z);

	msg.WriteFloat(viewOverlayColor.w);
	msg.WriteFloat(viewOverlayColor.x);
	msg.WriteFloat(viewOverlayColor.y);
	msg.WriteFloat(viewOverlayColor.z);
}

void hhPlayerView::ReadFromSnapshot( const idBitMsgDelta &msg ) {
	viewOffset.x = msg.ReadFloat();
	viewOffset.y = msg.ReadFloat();
	viewOffset.z = msg.ReadFloat();

	voTotalTime = msg.ReadBits(32);
	voFinishTime = msg.ReadBits(32);
	voRequiresScratchBuffer = !!msg.ReadBits(1);

	int matIndex = msg.ReadLong();
	if (matIndex == -1) {
		viewOverlayMaterial = NULL;
	}
	else {
		int mappedIndex;

		if (voTotalTime != -1 && gameLocal.time >= voFinishTime) { //it's timed out so force it off
			voTotalTime = -1;
			voRequiresScratchBuffer = false;
			viewOverlayMaterial = NULL;
			mappedIndex = -1;
		}
		else {
			mappedIndex = gameLocal.ClientRemapDecl(DECL_MATERIAL, matIndex);
		}

		if (mappedIndex != -1) {
			viewOverlayMaterial = static_cast<const idMaterial *>(declManager->DeclByIndex(DECL_MATERIAL, mappedIndex));
		}
		else {
			viewOverlayMaterial = NULL;
		}
	}

	//read screen blobs from snapshot
	for (int i = 0; i < MAX_SCREEN_BLOBS_SYNC; i++) {
		screenBlob_t	*blob = &screenBlobs[i];

		//i'm disabling this since it might work against delta compression being effective
		/*
		bool valid = !!msg.ReadBits(1);
		if (!valid) {
			continue;
		}
		*/

		int blobMat = msg.ReadLong();
		if (blobMat == -1) {
			blob->material = NULL;
		}
		else {
			int mappedIndex = gameLocal.ClientRemapDecl(DECL_MATERIAL, blobMat);
			if (mappedIndex != -1) {
				blob->material = static_cast<const idMaterial *>(declManager->DeclByIndex(DECL_MATERIAL, mappedIndex));
			}
			else {
				blob->material = NULL;
			}
		}

		blob->x = msg.ReadFloat();
		blob->y = msg.ReadFloat();
		blob->w = msg.ReadFloat();
		blob->h = msg.ReadFloat();

		blob->s1 = msg.ReadFloat();
		blob->t1 = msg.ReadFloat();
		blob->s2 = msg.ReadFloat();
		blob->t2 = msg.ReadFloat();

		blob->finishTime = msg.ReadBits(32);
		blob->startFadeTime = msg.ReadBits(32);

		blob->driftAmount = msg.ReadFloat();
	}

	//motion blur
	mbAmplitude = msg.ReadFloat();
	mbFinishTime = msg.ReadBits(32);
	mbTotalTime = msg.ReadBits(32);
	mbDirection.x = msg.ReadFloat();
	mbDirection.y = msg.ReadFloat();
	mbDirection.z = msg.ReadFloat();

	viewOverlayColor.w = msg.ReadFloat();
	viewOverlayColor.x = msg.ReadFloat();
	viewOverlayColor.y = msg.ReadFloat();
	viewOverlayColor.z = msg.ReadFloat();
}

void hhPlayerView::ClearEffects() {
	idPlayerView::ClearEffects();

	// HUMANHEAD pdm
	mbFinishTime = gameLocal.time;
}


void hhPlayerView::SetDamageLoc(const idVec3 &damageLoc) {
	// This called before damageImpulse() so store the location
	lastDamageLocation = damageLoc;
}


//------------------------------------------------------
//
// DamageImpulse
//
// LocalKickDir is the direction of force in the player's coordinate system,
// which will determine the head kick direction
//------------------------------------------------------
void hhPlayerView::DamageImpulse( idVec3 localKickDir, const idDict *damageDef ) {

	// No screen damage effects when in third person
	if (pm_thirdPerson.GetBool()) {
		return;
	}

	if ( lastDamageTime > 0.0f && SEC2MS( lastDamageTime ) + IMPULSE_DELAY > gameLocal.time ) {
		// keep shotgun from obliterating the view
		return;
	}

	if (!player->InVehicle()) {
		//
		// Motion Blur effect
		//	HUMANHEAD pdm
		float mbTime = damageDef->GetFloat( "mb_time" );
		float severity = damageDef->GetFloat( "mb_amplitude" );
		if ( mbTime ) {
			idVec3 blurDirection;
			blurDirection.y = localKickDir[0];	// forward/back kick will blur vertically
			blurDirection.x = localKickDir[1];	// side kick will blur horizontally
			blurDirection.y += localKickDir[2];	// up/down kick will add to vertical
			MotionBlur(mbTime, severity, blurDirection);
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
		// HUMANHEAD: Screen blobs, changed functionality
		//
		float	blobTime = damageDef->GetFloat( "blob_time" );
		if ( blobTime ) {
			screenBlob_t	*blob = GetScreenBlob();
			blob->startFadeTime = gameLocal.time;
			blob->finishTime = gameLocal.time + blobTime * g_blobTime.GetFloat();

			const char *materialName = damageDef->GetString( "mtr_blob" );
			blob->material = declManager->FindMaterial( materialName );

			// Scale blob by 100% +/- blob_devscale%
			float scale = ( 256.0f + ( 256.0f*damageDef->GetFloat("blob_devscale")*gameLocal.random.CRandomFloat() ) ) / 256.0f;
			blob->w = damageDef->GetFloat( "blob_width" ) * g_blobSize.GetFloat() * scale;
			blob->h = damageDef->GetFloat( "blob_height" ) * g_blobSize.GetFloat() * scale;
			blob->s1 = 0;
			blob->t1 = 0;
			blob->s2 = 1;
			blob->t2 = 1;

			if (damageDef->GetBool("blob_projected") && player->renderView) {
				//rww - renderView null check added. I am not clear on when this may be null, but it apparently was. however, if a player were
				//to take damage before thinking, it seems very possible this would occur. the renderView will not be initialized until after
				//the first think.
				// Project hit location onto screen
				idVec3 localDamageLocation = hhUtils::ProjectOntoScreen(lastDamageLocation, *player->renderView);
				blob->x = localDamageLocation.x - blob->w * 0.5f;
				blob->y = localDamageLocation.y - blob->h * 0.5f;
			}
			else if (damageDef->GetBool("blob_directional")) {
				// Directional blobs to show where damage is coming from (360 degree)
				float dirDist = damageDef->GetFloat("blob_dirdistance");
				blob->x = 320.0f + (dirDist * 320.0f * localKickDir.y) - (blob->w * 0.5f);
				blob->y = 240.0f + (dirDist * 240.0f * localKickDir.x) - (blob->h * 0.5f);
			}
			else {
				// Place blob centered at (blob_x,blob_y)
				blob->x =  damageDef->GetFloat( "blob_x" ) - blob->w * 0.5f;
				blob->y =  damageDef->GetFloat( "blob_y" ) - blob->h * 0.5f;
			}

			// Deviate blob +/- (blob_devx,blob_devy)
			blob->x += damageDef->GetFloat( "blob_devx" ) * gameLocal.random.CRandomFloat();
			blob->y += damageDef->GetFloat( "blob_devy" ) * gameLocal.random.CRandomFloat();
		}
	}


	//
	// Global directional damage system
	//
/*
	const int directionDamageTime = 1000;
	screenBlob_t	*blob = GetScreenBlob();
	blob->startFadeTime = gameLocal.time;
	blob->finishTime = gameLocal.time + directionDamageTime;

	blob->s1 = 0;
	blob->t1 = 0;
	blob->s2 = 1;
	blob->t2 = 1;
	if (idMath::Fabs(localKickDir[0]) >= idMath::Fabs(localKickDir[1])) {
		// More in X direction
		blob->material = dirDmgFrontMaterial;
		blob->w = 500.0f;
		blob->h = 80.0f;
		blob->x = 320.0f - blob->w * 0.5f;
		if (localKickDir[0] >= 0.0f) {
			// From Rear
			blob->y = 480.0f - blob->h;
			idSwap(blob->t1, blob->t2);
		}
		else {
			// From Front
			blob->y = 0.0f;
		}
	}
	else {
		// More in Y direction
		blob->material = dirDmgLeftMaterial;
		blob->w = 80.0f;
		blob->h = 400.0f;
		blob->y = 240.0f - blob->h * 0.5f;
		if (localKickDir[1] >= 0.0f) {
			// From Right
			blob->x = 640.0f - blob->w;
			idSwap(blob->s1, blob->s2);
		}
		else {
			// From Left
			blob->x = 0.0f;
		}
	}
*/

	//
	// save lastDamageTime for tunnel vision accentuation
	//
	lastDamageTime = MS2SEC( gameLocal.time );

}

//------------------------------------------------------
// WeaponFireFeedback
// HUMANHEAD bjk
//------------------------------------------------------
void hhPlayerView::WeaponFireFeedback( const idDict *weaponDef ) {
	idAngles angles;
	idVec2 pitch, yaw, viewSpring;
	weaponDef->GetVec2( "recoilPitch", "0 0", pitch );
	weaponDef->GetVec2( "recoilYaw", "0 0", yaw );
	weaponDef->GetVec2( "viewSpring", "150 11", viewSpring );

	player->kickSpring = viewSpring.x;
	player->kickDamping = viewSpring.y;

	pitch.x = pitch.x + gameLocal.random.RandomFloat()*(pitch.y - pitch.x);
	yaw.x = yaw.x + gameLocal.random.RandomFloat()*(yaw.y - yaw.x);
	angles=idAngles(-pitch.x,-yaw.x,0);
	kickSpeed+=angles;
	assert(!FLOAT_IS_NAN(kickSpeed.pitch) && !FLOAT_IS_NAN(kickSpeed.yaw) && !FLOAT_IS_NAN(kickSpeed.roll));
}

//------------------------------------------------------
// AngleOffset
// HUMANHEAD bjk
//------------------------------------------------------
idAngles hhPlayerView::AngleOffset(float kickSpring, float kickDamping) {
	//HUMANHEAD rww - for other clients, do not add angle offset, it isn't predicted well
	if (gameLocal.isMultiplayer && !gameLocal.isServer && gameLocal.localClientNum != -1 && player && gameLocal.localClientNum != player->entityNumber) {
		kickSpeed.Zero();
		kickAngles.Zero();
		return kickAngles;
	}
	//HUMANHEAD END

	float frametime = (gameLocal.time - kickLastTime);
	kickLastTime = gameLocal.time;
	//kickAngles = kickAngles*(1-offset*kickSpeed)+kickBlendTo*offset*kickSpeed;
	//kickBlendTo = kickBlendTo*(1-offset*kickReturnSpeed)+ang*offset*kickReturnSpeed;

	//HUMANHEAD PCF rww 04/27/06 - these values can get unreasonable at times
#if 0
	for (int i = 0; i < 3; i++) {
		if (fabsf(kickSpeed[i]) > 9999.0f) {
			kickSpeed[i] = 0.0f;
		}
		if (fabsf(kickAngles[i]) > 9999.0f) {
			kickAngles[i] = 0.0f;
		}
	}
#endif
	if (frametime > 48.0f) {
		frametime = 48.0f;
	}
	//HUMANHEAD END

	assert(!FLOAT_IS_NAN(kickSpeed.pitch) && !FLOAT_IS_NAN(kickSpeed.yaw) && !FLOAT_IS_NAN(kickSpeed.roll));
	assert(!FLOAT_IS_NAN(kickAngles.pitch) && !FLOAT_IS_NAN(kickAngles.yaw) && !FLOAT_IS_NAN(kickAngles.roll));

	kickSpeed-=kickDamping*kickSpeed*frametime/1000;
	kickSpeed-=kickSpring*kickAngles*frametime/1000;

	if (gameLocal.isNewFrame) { //HUMANHEAD rww
		kickAngles+=kickSpeed*frametime/1000;
	}
	
	return kickAngles;
}

//------------------------------------------------------
// SingleView
//------------------------------------------------------
void hhPlayerView::SingleView( idUserInterface *hud, const renderView_t *view ) {
	// normal rendering

	if ( !view ) {
		return;
	}

	// place the sound origin for the player
	gameSoundWorld->PlaceListener( view->vieworg, view->viewaxis, player->entityNumber + 1, gameLocal.time, hud ? hud->State().GetString( "location" ) : "Undefined" );

	// hack the shake in at the very last moment, so it can't cause any consistency problems
	renderView_t	hackedView = *view;
	hackedView.viewaxis = hackedView.viewaxis * ShakeAxis();

	gameRenderWorld->RenderScene( &hackedView );

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
				renderSystem->DrawStretchPic( blob->x, blob->y, blob->w, blob->h, blob->s1, blob->t1, blob->s2, blob->t2, blob->material );
			}
		}

		// HUMANHEAD: CJR
		if ( voTotalTime != -1 ) { // Check if the viewOverlay should time out
			if ( gameLocal.time >= voFinishTime ) {
				voTotalTime = -1;
				voRequiresScratchBuffer = false;
				viewOverlayMaterial = NULL; // Remove the overlay
			}
		}

		//HUMANHEAD: aob
		if( viewOverlayMaterial ) {		
			renderSystem->SetColor4( viewOverlayColor[0], viewOverlayColor[1], viewOverlayColor[2], viewOverlayColor[3] );
			renderSystem->DrawStretchPic(0, 0, 640, 480, 0, 1, 1, 0, viewOverlayMaterial);
		}
		//HUMANHEAD END

		if ( player->health > 0 && player->health < 25 && static_cast<hhPlayer*>(player)->IsSpiritOrDeathwalking()==false ) {
			hurtValue += 0.05f*(player->health - hurtValue);
			if( player->health > 30 )
				hurtValue = player->health;

			renderSystem->SetColor4( hurtValue/25.0f, 1, 1, 1 );
			renderSystem->DrawStretchPic(0, 0, 640, 480, 0, 1, 1, 0, hurtMaterial);
		}
	}

	// If this level has a sun corona, then attempt to draw it - CJR
	if ( !g_skipViewEffects.GetBool() && gameLocal.GetSunCorona() ) {
		gameLocal.GetSunCorona()->Draw( (hhPlayer *)player );
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

// Hermite()
// Hermite Interpolator
// Returns an alpha value [0..1] based on Hermite Parameters N1, N2, S1, S2 and an input alpha 't'
float Hermite(float t, float N1, float N2, float S1, float S2) {
	float tSquared = t*t;
	float tCubed = tSquared*t;
	return	(2*tCubed - 3*tSquared + 1)*N1 +
			(-2*tCubed + 3*tSquared)*N2 +
			(tCubed - 2*tSquared + t)*S1 +
			(tCubed - tSquared)*S2;
}


//------------------------------------------------------
// MotionBlurVision
//	HUMANHEAD pdm
//------------------------------------------------------

void hhPlayerView::MotionBlurVision(idUserInterface *hud, const renderView_t *view) {
	if ( !g_doubleVision.GetBool() ) {
		SingleView( hud, view );
		return;
	}

	float alpha;
	const float N1 = 0.2f;
	const float N2 = 0.0f;
	const float S1 = 1.0f;
	const float S2 = 1.0f;

	float remainingTime = mbFinishTime - gameLocal.time;
	float elapsedTime = mbTotalTime - remainingTime;
	float scale = remainingTime / mbTotalTime;

	// Render to a texture
	RENDER_DEMO_VIEWRENDER(view, this); //HUMANHEAD rww
	renderSystem->CropRenderSize( 512, 256, true );
	SingleView( hud, view );
	renderSystem->CaptureRenderToImage( "_scratch" );
	renderSystem->UnCrop();
	RENDER_DEMO_VIEWRENDER_END(); //HUMANHEAD rww

	// Motion blur
	float xshift, yshift;
	for (int index=0; index<g_mbNumBlurs.GetInteger(); index++) {

		float curtime = elapsedTime - g_mbFrameSpan.GetFloat()*index*scale;
		float blurfactor = (index+1)/g_mbNumBlurs.GetFloat();

		// Hermite blend here (N1 is severity (0 - 0.5) )
		alpha = curtime/mbTotalTime;
		alpha = Hermite(alpha, N1, N2, S1, S2);	// Hermite parms: N1, N2, S1, S2
		
		xshift = blurfactor * scale * mbDirection.x * alpha * mbAmplitude;
		yshift = blurfactor * scale * mbDirection.y * alpha * mbAmplitude;

		#define CLIP(a) ((a)<0?0:(a)>1?1:(a))
		renderSystem->SetColor4( 1,1,1, index==0 ? 1.0f : 0.2f );
		renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
			CLIP(xshift), CLIP(1+yshift), CLIP(1+xshift), CLIP(yshift), scratchMaterial ); // clipped
	}
}

//------------------------------------------------------
// SpiritVision
//	HUMANHEAD cjr
//------------------------------------------------------

void hhPlayerView::SpiritVision( idUserInterface *hud, const renderView_t *view ) {
	int oldTime = voTotalTime;
	const idMaterial *oldMaterial = viewOverlayMaterial;

	voTotalTime = -1;
	viewOverlayMaterial = NULL;

	SingleView( hud, view );

	// Draw the spiritwalk image over the top
	if ( player && !spiritMaterial ) {
		spiritMaterial = declManager->FindMaterial( player->spawnArgs.GetString( "mtr_spiritwalk" ) );
	}

	renderSystem->SetColor4( 1, 1, 1, 1 );
	renderSystem->DrawStretchPic(0, 0, 640, 480, 0, 1, 1, 0, spiritMaterial );

	voTotalTime = oldTime;
	viewOverlayMaterial = oldMaterial;
}

//------------------------------------------------------
// ApplyLetterbox
//	HUMANHEAD pdm
//------------------------------------------------------
void hhPlayerView::ApplyLetterBox(const renderView_t *view) {
	if (bLetterBox) {
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
		renderSystem->DrawStretchPic(0, 0, 640, LETTERBOX_HEIGHT_TOP, 0, 0, 1, 1, letterboxMaterial);
		renderSystem->DrawStretchPic(0, 480-LETTERBOX_HEIGHT_BOTTOM, 640, LETTERBOX_HEIGHT_BOTTOM, 0, 0, 1, 1, letterboxMaterial);
	}
}

//------------------------------------------------------
// MotionBlur
//	HUMANHEAD pdm
//------------------------------------------------------
void hhPlayerView::MotionBlur(int mbTime, float severity, idVec3 &direction) {
	mbTotalTime = mbTime;
	mbFinishTime = gameLocal.time + mbTotalTime;
	mbAmplitude = severity;
	mbDirection = direction;
}

//------------------------------------------------------
// SetLetterBox
//	HUMANHEAD pdm
//------------------------------------------------------
void hhPlayerView::SetLetterBox(bool on) {
	bLetterBox = on;
}

//------------------------------------------------------
// RenderPlayerView
//------------------------------------------------------
void hhPlayerView::RenderPlayerView( idUserInterface *hud ) {
	const renderView_t *view = player->GetRenderView();

	if ( g_skipViewEffects.GetBool() ) {
		SingleView( hud, view );
	} else {
		if (gameLocal.time < mbFinishTime) {
			MotionBlurVision(hud, view);
		} else if ( ((hhPlayer *)player)->IsSpiritWalking() ) {
			SpiritVision( hud, view );
		} else {
			SingleView(hud, view);
		}
		ScreenFade();

		// HUMANHEAD pdm: letterbox
		ApplyLetterBox(view);
	}

	// HUMANHEAD: Draw the HUD after all over overlay effects.
	if (hud) {
		hud->SetStateBool("letterbox", bLetterBox);
		player->DrawHUD( hud );
	}

	if ( net_clientLagOMeter.GetBool() && lagoMaterial && gameLocal.isClient ) {
		renderSystem->SetColor4( 1.0f, 1.0f, 1.0f, 1.0f );
		renderSystem->DrawStretchPic( 10.0f, 380.0f, 64.0f, 64.0f, 0.0f, 0.0f, 1.0f, 1.0f, lagoMaterial );
	}	
}
