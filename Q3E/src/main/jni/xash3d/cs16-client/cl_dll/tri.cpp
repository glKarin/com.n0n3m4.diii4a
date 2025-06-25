//========= Copyright ? 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any
#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "rain.h"

extern int g_iWaterLevel;

FogParameters g_FogParameters;

void RenderFog()
{
	FogParameters fog;

	fog = g_FogParameters;

	if( cl_fog_density )
		fog.density = cl_fog_density->value;

	if( cl_fog_r )
		fog.color[0] = cl_fog_r->value;

	if( cl_fog_g )
		fog.color[1] = cl_fog_g->value;

	if( cl_fog_b )
		fog.color[2] = cl_fog_b->value;
	
	gEngfuncs.pTriAPI->FogParams( fog.density, fog.affectsSkyBox );
	gEngfuncs.pTriAPI->Fog( fog.color, 100.0f, 2000.0f, g_iWaterLevel <= 1 ? fog.density > 0.0f : 0 );
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{
	gHUD.m_Spectator.DrawOverview();
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
extern bool Rain_Initialized;
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	RenderFog();

	if( Rain_Initialized )
	{
		ProcessFXObjects();
		ProcessRain();
		DrawRain();
		DrawFXObjects();
	}
}
