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
#ifndef __LIGHTGEM_H__
#define __LIGHTGEM_H__

//----------------------------------
// Constants
//----------------------------------
// Number of different passes. Since the lightgem model is pyramid-shaped, we do one pass from above and
// one from below. Each render is then split into 4 parts corresponding to the 4 pyramid sides.
static const int	DARKMOD_LG_MAX_RENDERPASSES = 2;
static const int	DARKMOD_LG_MAX_IMAGESPLIT   = 4;
static const char * DARKMOD_LG_RENDER_MODEL		= "models/darkmod/misc/system/lightgem.lwo";
static const char * DARKMOD_LG_ENTITY_NAME		= "lightgem_surface";

static const int    DARKMOD_LG_RENDER_WIDTH		= 64; // LG render resolution - keep it a power-of-two!
static const float  DARKMOD_LG_RENDER_FOV		= 70.0f;
static const int	DARKMOD_LG_BPP				= 3; // 3 Channels of 8 bits
static const int	DARKMOD_LG_PAUSE			= 6; // #6088: PBO triple-buffering + frontend/backend + passes = 5 frames minimum!

// The colour is converted to a grayscale value which determines the state of the lightgem.
// LightGem = (0.29900*R+0.58700*G+0.11400*B) * 0.0625
static const int	DARKMOD_LG_MIN				= 1;
static const int	DARKMOD_LG_MAX				= 32;
static const float	DARKMOD_LG_FRACTION			= 1.0f / 32.0f;
static const float  DARKMOD_LG_RED				= 0.29900f;
static const float  DARKMOD_LG_GREEN			= 0.58700f;
static const float  DARKMOD_LG_BLUE				= 0.11400f;
static const float  DARKMOD_LG_SCALE			= 1.0f / 255.0f;		// scaling factor for grayscale value
static const float  DARKMOD_LG_TRIRATIO			= 1.0f / (DARKMOD_LG_RENDER_WIDTH*DARKMOD_LG_RENDER_WIDTH / 4.0f);

//----------------------------------
// Class Declarations.
//----------------------------------
struct emptyCommand_t;

class LightGem
{
private:
	int						m_LightgemShotSpot;
	float					m_LightgemShotValue[DARKMOD_LG_MAX_RENDERPASSES];

	float 					m_fColVal[DARKMOD_LG_MAX_IMAGESPLIT];

	// future "Frames" calls will return "Value" instead of actually computed values
	// this is used to avoid wrong lightgem values briefly after quickload (#6088)
	int						m_LightgemOverrideFrames;
	float					m_LightgemOverrideValue;

public:
	unsigned char*			m_LightgemImgBufferFrontend;
	unsigned char*			m_LightgemImgBufferBackend;
	idEntityPtr<idEntity>	m_LightgemSurface;

	//---------------------------------
	// Construction/Destruction
	//---------------------------------
	LightGem	();
	~LightGem	();

	//---------------------------------
	// Initialization
	//---------------------------------
	void Clear			();

	//---------------------------------
	// Persistence
	//---------------------------------
	void Save			( idSaveGame &		a_saveGame );
	void Restore		( idRestoreGame &	a_savedGame );
	//---------------------------------
	
	//---------------------------------
	// SpawnlightgemEntity will create exactly one lightgem entity for the map and ensures
	//  that no multiple copies of it will exist.
	//---------------------------------
	void SpawnLightGemEntity( idMapFile *	a_mapFile );
	void InitializeLightGemEntity();

	//---------------------------------
	// Calculation
	//---------------------------------
	float Calculate		( idPlayer *	a_pPlayer );	

private:
	void AnalyzeRenderImage	( );
};

#endif // __LIGHTGEM_H__
