/*
hud_overlays.cpp - HUD Overlays
Copyright (C) 2015-2016 a1batross

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.

*/

#include "hud.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "cl_util.h"

#include "draw_util.h"

int CHudSniperScope::Init()
{
	if( g_iXash )
		gHUD.AddHudElem(this);

	m_iFlags = HUD_DRAW;
	m_iScopeArc[0] = m_iScopeArc[1] =m_iScopeArc[2] = m_iScopeArc[3]  = 0;
	return 1;
}

int CHudSniperScope::VidInit()
{
	if( g_iXash == 0 )
	{
		ConsolePrint("^3No Xash Found Warning^7: CHudSniperScope is disabled!\n");
		m_iFlags = 0;
		return 0;
	}

	m_iScopeArc[0] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_nw.tga", NULL, 0, TF_NEAREST|TF_NOMIPMAP|TF_CLAMP);
	m_iScopeArc[1] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_ne.tga", NULL, 0, TF_NEAREST|TF_NOMIPMAP|TF_CLAMP);
	m_iScopeArc[2] = gRenderAPI.GL_LoadTexture("sprites/scope_arc.tga",    NULL, 0, TF_NEAREST|TF_NOMIPMAP|TF_CLAMP);
	m_iScopeArc[3] = gRenderAPI.GL_LoadTexture("sprites/scope_arc_sw.tga", NULL, 0, TF_NEAREST|TF_NOMIPMAP|TF_CLAMP);

	if( !m_iScopeArc[0] || !m_iScopeArc[1] || !m_iScopeArc[2] || !m_iScopeArc[3] )
	{
		gRenderAPI.Host_Error( "^3Error^7: Cannot load Sniper Scope arcs. Check sprites/scope_arc*.tga files\n" );
	}
	
	left = (TrueWidth - TrueHeight)/2.0;
	right = left + TrueHeight;
	centerx = TrueWidth/2.0;
	centery = TrueHeight/2.0;
	return 1;
}

inline void DrawTexture( int tex, float x1, float y1, float x2, float y2 )
{
	gRenderAPI.GL_Bind( 0, tex );
	//gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	DrawUtils::Draw2DQuad( x1, y1, x2, y2 );
	//gEngfuncs.pTriAPI->End();
}

int CHudSniperScope::Draw(float flTime)
{
	if(gHUD.m_iFOV > 40)
		return 1;

	gEngfuncs.pTriAPI->RenderMode(kRenderTransColor);
	gEngfuncs.pTriAPI->Brightness(1.0);
	gEngfuncs.pTriAPI->Color4ub(0, 0, 0, 255);
	gEngfuncs.pTriAPI->CullFace(TRI_NONE);

	gRenderAPI.GL_SelectTexture(0);

	DrawTexture( m_iScopeArc[0], left, 0, centerx, centery );
	DrawTexture( m_iScopeArc[1], centerx, 0, right, centery );
	DrawTexture( m_iScopeArc[2], centerx, centery, right, TrueHeight );
	DrawTexture( m_iScopeArc[3], left, centery, centerx, TrueHeight );

	gRenderAPI.GL_Bind( 0, gHUD.m_WhiteTex );
	// gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		DrawUtils::Draw2DQuad( 0, 0, left + 2, TrueHeight );
		DrawUtils::Draw2DQuad( right, 0, right + ( TrueWidth - right ), TrueHeight );
	
	// default crosshair pixel perfect lines
		DrawUtils::Draw2DQuad( left, centery + 1, right, centery + 2 );
		DrawUtils::Draw2DQuad( centerx - 1, 0, centerx, TrueHeight );
	// gEngfuncs.pTriAPI->End();

	return 0;
}

void CHudSniperScope::Shutdown()
{
	for( int i = 0; i < 4; i++ )
		gRenderAPI.GL_FreeTexture( m_iScopeArc[i] );
}
