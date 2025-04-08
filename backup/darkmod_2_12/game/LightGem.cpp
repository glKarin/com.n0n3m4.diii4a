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



#include "LightGem.h"
#include "../renderer/tr_local.h"

//------------------------
// Construction/Destruction
//----------------------------------------------------
LightGem::LightGem()
{
	m_LightgemImgBufferFrontend = (byte*)Mem_Alloc16( DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_BPP * 4 );
	m_LightgemImgBufferBackend = (byte*)Mem_Alloc16( DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_RENDER_WIDTH * DARKMOD_LG_BPP * 4 );
}

LightGem::~LightGem()
{
	Mem_Free16( m_LightgemImgBufferFrontend );
	Mem_Free16( m_LightgemImgBufferBackend );
}


//----------------------------------------------------
// Initialization
//----------------------------------------------------
void LightGem::Clear()
{
	m_LightgemSurface = NULL;
	m_LightgemShotSpot = 0;

	memset(m_LightgemShotValue, 0, sizeof(m_LightgemShotValue));
}

void LightGem::SpawnLightGemEntity( idMapFile *	a_mapFile )
{
	static const char *LightgemName = DARKMOD_LG_ENTITY_NAME;
	idMapEntity *mapEnt = a_mapFile->FindEntity(LightgemName);

	if ( mapEnt == NULL ) {
		mapEnt = new idMapEntity();
		a_mapFile->AddEntity(mapEnt);
		mapEnt->epairs.Set("classname", "func_static");
		mapEnt->epairs.Set("name", LightgemName);
		if ( strlen(cv_lg_model.GetString()) == 0 ) {
			mapEnt->epairs.Set("model", DARKMOD_LG_RENDER_MODEL);
		} else {
			mapEnt->epairs.Set("model", cv_lg_model.GetString());
		}
		mapEnt->epairs.Set("origin", "0 0 0");
		mapEnt->epairs.Set("noclipmodel", "1");
	}
}

void LightGem::InitializeLightGemEntity( void )
{
	m_LightgemSurface = gameLocal.FindEntity(DARKMOD_LG_ENTITY_NAME);
	m_LightgemSurface.GetEntity()->GetRenderEntity()->allowSurfaceInViewID = VID_LIGHTGEM;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->suppressShadowInViewID = 0;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noDynamicInteractions = false;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noShadow = true;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noSelfShadow = true;
	//nbohr1more: #4379 lightgem culling
	m_LightgemSurface.GetEntity()->GetRenderEntity()->isLightgem = true;

	DM_LOG(LC_LIGHT, LT_INFO)LOGSTRING("LightgemSurface: [%08lX]\r", m_LightgemSurface.GetEntity());

	// set lightgem to full dark instead of weird values on game start
	m_LightgemOverrideFrames = DARKMOD_LG_PAUSE;
	m_LightgemOverrideValue = 0.0f;
}

//----------------------------------------------------
// State Persistence 
//----------------------------------------------------

void LightGem::Save( idSaveGame & a_saveGame )
{
	m_LightgemSurface.Save( &a_saveGame );
	a_saveGame.WriteInt(m_LightgemShotSpot);
	for (int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++) {
		a_saveGame.WriteFloat(m_LightgemShotValue[i]);
	}
	// m_LightgemOverrideXXX not saved: reset on load
}

void LightGem::Restore( idRestoreGame & a_savedGame )
{
	m_LightgemSurface.Restore( &a_savedGame );
	a_savedGame.ReadInt(m_LightgemShotSpot);
	for (int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++) {
		a_savedGame.ReadFloat(m_LightgemShotValue[i]);
	}

	// #6088: return old lightgem values read from savefile for the next few frames
	m_LightgemOverrideFrames = DARKMOD_LG_PAUSE;
	m_LightgemOverrideValue = *std::max_element(m_LightgemShotValue, m_LightgemShotValue + DARKMOD_LG_MAX_RENDERPASSES);

	m_LightgemSurface.GetEntity()->GetRenderEntity()->allowSurfaceInViewID = VID_LIGHTGEM;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->suppressShadowInViewID = 0;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noDynamicInteractions = false;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noShadow = true;
	m_LightgemSurface.GetEntity()->GetRenderEntity()->noSelfShadow = true;
	//nbohr1more: #4379 lightgem culling
	m_LightgemSurface.GetEntity()->GetRenderEntity()->isLightgem = true;

	DM_LOG(LC_LIGHT, LT_INFO)LOGSTRING("LightgemSurface: [%08lX]\r", m_LightgemSurface.GetEntity());
}

//----------------------------------------------------
// Calculation
//----------------------------------------------------

float LightGem::Calculate(idPlayer *player)
{
	// analyze rendered shot from previous frame
	AnalyzeRenderImage();
	m_LightgemShotValue[m_LightgemShotSpot] = 0.0f;
	// Check which of the images has the brightest value, and this is what we will use.
	for (int l = 0; l < DARKMOD_LG_MAX_IMAGESPLIT; l++) {
		if (m_fColVal[l] > m_LightgemShotValue[m_LightgemShotSpot]) {
			m_LightgemShotValue[m_LightgemShotSpot] = m_fColVal[l];
		}
	}

	// If player is hidden (i.e the whole player entity is actually hidden)
	if ( player->GetModelDefHandle() == -1 ) {
		return 0.0f;
	}
	
	// Get position for lg
	idEntity* lg = m_LightgemSurface.GetEntity();
	// duzenko #4408 - this happens at map start if no game tics ran in background yet
	if (lg->GetModelDefHandle() == -1) 
		return 0.0f;
	renderEntity_t* lgent = lg->GetRenderEntity();

	const idVec3& Cam = player->GetEyePosition();
	idVec3 LGPos = player->GetPhysics()->GetOrigin();// Set the lightgem position to that of the player

	LGPos.x += (Cam.x - LGPos.x) * 0.3f + cv_lg_oxoffs.GetFloat(); // Move the lightgem out a fraction along the leaning x vector
	LGPos.y += (Cam.y - LGPos.y) * 0.3f + cv_lg_oyoffs.GetFloat(); // Move the lightgem out a fraction along the leaning y vector
	
	// Prevent lightgem from clipping into the floor while crouching
	if ( player->GetPlayerPhysics()->IsCrouching() ) {
		LGPos.z += 50.0f + cv_lg_ozoffs.GetFloat() ;
	} else {
		LGPos.z = Cam.z + cv_lg_ozoffs.GetFloat(); // Set the lightgem's Z-axis position to that of the player's eyes
	}

	m_LightgemShotSpot = (m_LightgemShotSpot + 1) % DARKMOD_LG_MAX_RENDERPASSES;

	// we want to return the highest shot value
	float fRetVal = 0.0f;
	for (int i = 0; i < DARKMOD_LG_MAX_RENDERPASSES; i++) {
		if ( m_LightgemShotValue[i] > fRetVal ) {
			fRetVal = m_LightgemShotValue[i];
		}
	}

	if (m_LightgemOverrideFrames > 0) {
		// #6088: return old value for now
		m_LightgemOverrideFrames--;
		return m_LightgemOverrideValue;
	}

	return fRetVal;
}

void LightGem::AnalyzeRenderImage()
{
	// frontend and backend can run in parallel, which is why we need two buffers and need to swap them
	// on every frame
	std::swap( m_LightgemImgBufferFrontend, m_LightgemImgBufferBackend );

	const byte *buffer = m_LightgemImgBufferFrontend;
	
	// The lightgem will simply blink if the renderbuffer doesn't work.
	if ( buffer == nullptr ) {
		DM_LOG(LC_SYSTEM, LT_ERROR)LOGSTRING("Unable to read image from lightgem render-buffer\r");

		for ( int i = 0; i < DARKMOD_LG_MAX_IMAGESPLIT; i++ ) {
			m_fColVal[i] = (gameLocal.time % 1024 ) > 512;
		}

		return;
	}
	
	/* 	Split up the image into the 4 triangles

		 \11/	0 - east of lightgem render
		3 \/ 0	1 - north of lg
		3 /\ 0	2 - south of lg
		 /22\	3 - west of lg
	
		Note : Serp - This is a simplification of the early version which used two nested loops
	*/

	int in = 0;

	for ( int x = 0; x < DARKMOD_LG_RENDER_WIDTH; x++ ) {
		for ( int y = 0; y < DARKMOD_LG_RENDER_WIDTH; y++, buffer += DARKMOD_LG_BPP ) { // increment the buffer pos
			if ( y <= x && x + y >= (DARKMOD_LG_RENDER_WIDTH -1) ) {
				in = 0;
			} else if ( y < x ) {
				in = 1;
			} else if ( y > (DARKMOD_LG_RENDER_WIDTH -1) - x ) {
				in = 2;
			} else {
				in = 3;
			}

			// The order is RGB. 
			// #4395 Duzenko lightem pixel pack buffer optimization
			m_fColVal[in] += buffer[0] * DARKMOD_LG_RED +
							 buffer[1] * DARKMOD_LG_GREEN +
							 buffer[2] * DARKMOD_LG_BLUE;
		}
	}

	// Calculate the average for each value
	// Could be moved to the return
	// #4395 Duzenko lightem pixel pack buffer optimization
	m_fColVal[0] *= DARKMOD_LG_TRIRATIO * DARKMOD_LG_SCALE;
	m_fColVal[1] *= DARKMOD_LG_TRIRATIO * DARKMOD_LG_SCALE;
	m_fColVal[2] *= DARKMOD_LG_TRIRATIO * DARKMOD_LG_SCALE;
	m_fColVal[3] *= DARKMOD_LG_TRIRATIO * DARKMOD_LG_SCALE;
}
