/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "AnimatedBanner.h"

#define ART_LOGO      "resource/logo.tga"
#define ART_BLIP1     "resource/logo_blip.tga"
#define ART_BLIP2     "resource/logo_blip2.tga"
#define ART_BLUR      "resource/logo_big_blurred_%i"
#define ART_BLUR_BLIP "resource/logo_big_blurred_blip_%i"

bool CMenuAnimatedBanner::TryLoad()
{
	logo.Load( ART_LOGO );
	if( !logo.IsValid( ))
		return false;

	logoBlip[0].Load( ART_BLIP1 );
	logoBlip[1].Load( ART_BLIP2 );
	for( int i = 0; i < ART_BLIP_NUM; i++ )
	{
		if( !logoBlip[i].IsValid( ))
			return false;
	}

	for( int i = 0; i < ART_BLUR_NUM; i++ )
	{
		char name[256];
		snprintf( name, sizeof( name ), ART_BLUR, i );
		logoBlur[i].Load( name );
		if( !logoBlur[i].IsValid( ))
			return false;

		snprintf( name, sizeof( name ), ART_BLUR_BLIP, i );
		logoBlurBlip[i].Load( name );
		if( !logoBlurBlip[i].IsValid( ))
			return false;
	}

	return true;
}

void CMenuAnimatedBanner::VidInit()
{
	// in hardware mode, it's scaled by screen width
	// in software mode, scale must be 1.0f
	scale = ScreenWidth / 1024.0f;

	Size logoSz = EngFuncs::PIC_Size(logo.Handle());

	logoSz = logoSz * scale;
	m_nLogoImageXMin = (ScreenWidth / 2.0f - logoSz.w / 2.0f) - scale * 88.0f;
	m_nLogoImageXMax = (ScreenWidth / 2.0f - logoSz.w / 2.0f) + scale * 32.0f;
	m_nLogoImageXGoal = m_nLogoImageXMax;
	m_nNextLogoBlipType = E_LOGO_BLIP_BOTH;
	m_flTimeLogoNewGoalMin = 0.4f;
	m_flTimeLogoNewGoalMax = 1.95f;
	m_flTimeUntilLogoBlipMin = 0.5f;
	m_flTimeUntilLogoBlipMax = 1.4f;

	m_fLogoImageX = m_nLogoImageXMin + ((m_nLogoImageXMax - m_nLogoImageXMin) / 2.0f);

#if 0 // original GameUI logic
	if( ScreenHeight > 600 )
		m_nLogoImageY = 60 * uiStatic.scaleY;
	else m_nLogoImageY = 20 * uiStatic.scaleY;
#else // similar to logo.avi position
	m_nLogoImageY = ( 70 / 480.0f ) * 768.0f * uiStatic.scaleY;
#endif

	m_nLogoBGOffsetX = scale * -320.0f;
	m_nLogoBGOffsetY = m_nLogoImageY - scale * 54.0f;
	m_fLogoSpeedMin = scale * 13.0f;
	m_fLogoSpeedMax = scale * 65.0f;
	m_flPrevFrameTime = gpGlobals->time;

	RandomizeGoalTime( gpGlobals->time + m_flTimeLogoNewGoalMin );
	RandomizeSpeed( m_fLogoSpeedMin );
	RandomizeBlipTime( gpGlobals->time + m_flTimeUntilLogoBlipMin );
}

void CMenuAnimatedBanner::RandomizeGoalTime( float time )
{
	const float randval = rand() / (float)RAND_MAX;
	const float range = ( m_flTimeLogoNewGoalMax - m_flTimeLogoNewGoalMin );

	m_flTimeLogoNewGoal = time + randval * range;
}

void CMenuAnimatedBanner::RandomizeSpeed( float initial_speed )
{
	const float range = m_fLogoSpeedMax - m_fLogoSpeedMin;
	const float randval = rand() / (float)RAND_MAX;

	m_fLogoSpeed = initial_speed + randval * range;
}

void CMenuAnimatedBanner::RandomizeBlipTime( float time )
{
	const float range = m_flTimeUntilLogoBlipMax - m_flTimeUntilLogoBlipMin;
	const float randval = rand() / (float)RAND_MAX;

	m_flTimeLogoBlip = time + randval * range;
}

void CMenuAnimatedBanner::RandomizeBlip()
{
	// why tho
	const float randval = rand() / (float)RAND_MAX;

	if( randval >= 0.6f )
		m_nLogoBlipType = E_LOGO_BLIP_BOTH;
	else if( randval > 0.4f )
		m_nLogoBlipType = E_LOGO_BLIP_STAGGER;
	else if( randval >= 0.25f )
		m_nLogoBlipType = E_LOGO_BLIP_JUST_LOGO;
	else if( randval > 0.1f )
		m_nLogoBlipType = E_LOGO_BLIP_BOTH_SHOW_BLIP_LOGO_ONLY;
	else
		m_nLogoBlipType = E_LOGO_BLIP_JUST_BG;
}


void CMenuAnimatedBanner::Draw()
{
	if( EngFuncs::ClientInGame() && EngFuncs::GetCvarFloat( "ui_renderworld" ) != 0.0f )
		return;

	Point logoPt( m_fLogoImageX, m_nLogoImageY );
	Size logoSz = EngFuncs::PIC_Size( logo.Handle( )) * scale;

	EngFuncs::PIC_Set( logo.Handle(), 255, 255, 255 );
	EngFuncs::PIC_DrawTrans( logoPt, logoSz );

	for( int i = 0; i < ART_BLIP_NUM; i++ )
	{
		if( drawBlip[i] )
		{
			EngFuncs::PIC_Set( logoBlip[i], 255, 255, 255 );
			EngFuncs::PIC_DrawTrans( logoPt, logoSz );
		}
	}

#if 0
	// makes big logo centered but in original it's a bit offset
	logoPt.x = m_fLogoImageX + m_nLogoBGOffsetX;
#else
	logoPt.x = 0;
#endif

	float t1 = m_nLogoImageXMax - m_nLogoImageXMin;
	float t2 = m_nLogoBGOffsetX - t1 * 1.85f;
	float t3 = t2 + ( t1 * 3.7f ) * (( m_fLogoImageX - m_nLogoImageXMin ) / t1 );

	logoPt.x += t3;
	logoPt.y = m_nLogoBGOffsetY;

	const CImage *images = drawBgBlip ? logoBlurBlip : logoBlur;
	for( int i = 0; i < ART_BLUR_NUM; i++ )
	{
		logoSz = EngFuncs::PIC_Size( logoBlur[i].Handle( )) * scale;

		EngFuncs::PIC_Set( images[i].Handle( ), 255, 255, 255 );
		EngFuncs::PIC_DrawTrans( logoPt, logoSz );

		logoPt.x += logoSz.w;
	}
}

void CMenuAnimatedBanner::Think()
{
	// m_fLogoImageX = uiStatic.cursorX;
	// return;

	float deltatime = gpGlobals->time - m_flPrevFrameTime;
	deltatime = bound( 0.0001, deltatime, 0.3 );

	m_flPrevFrameTime = gpGlobals->time;

	float deltaX = deltatime * m_fLogoSpeed;

	if( m_fLogoImageX >= m_nLogoImageXGoal )
	{
		m_fLogoImageX -= deltaX;

		if( m_fLogoImageX <= m_nLogoImageXGoal || gpGlobals->time >= m_flTimeLogoNewGoal )
		{
			RandomizeGoalTime( gpGlobals->time + m_flTimeLogoNewGoalMin );
			RandomizeSpeed( m_fLogoSpeedMin );

			m_nLogoImageXGoal = m_nLogoImageXMax;
		}
	}
	else
	{
		m_fLogoImageX += deltaX;

		if( m_fLogoImageX >= m_nLogoImageXGoal || gpGlobals->time >= m_flTimeLogoNewGoal )
		{
			RandomizeGoalTime( gpGlobals->time + m_flTimeLogoNewGoalMin );
			RandomizeSpeed( m_fLogoSpeedMin );

			m_nLogoImageXGoal = m_nLogoImageXMin;
		}
	}

	drawBlip[0] = drawBlip[1] = false;
	drawBgBlip = false;

	if( gpGlobals->time > m_flTimeLogoBlip )
	{
		RandomizeBlipTime( gpGlobals->time + m_flTimeUntilLogoBlipMin );
		RandomizeBlip();
	}
	else
	{
		float timeToBlip;

		if( m_nLogoBlipType == E_LOGO_BLIP_STAGGER )
			timeToBlip = 0.09f; // TODO
		else if( m_nLogoBlipType == E_LOGO_BLIP_BOTH_SHOW_BLIP_LOGO_ONLY )
			timeToBlip = 0.07f;
		else
			timeToBlip = 0.06f;

		if( gpGlobals->time + timeToBlip > m_flTimeLogoBlip )
		{
			switch( m_nLogoBlipType )
			{
			case E_LOGO_BLIP_BOTH:
				drawBgBlip  = true;
				drawBlip[0] = true;
				break;
			case E_LOGO_BLIP_JUST_LOGO:
				drawBlip[0] = true;
				break;
			case E_LOGO_BLIP_JUST_BG:
				drawBgBlip  = true;
				break;
			case E_LOGO_BLIP_STAGGER:
				drawBgBlip  = true;
				drawBlip[0] = gpGlobals->time + ( timeToBlip / 2.0f ) <= m_flTimeLogoBlip;
				break;
			case E_LOGO_BLIP_BOTH_SHOW_BLIP_LOGO_ONLY:
				drawBgBlip  = true;
				drawBlip[0] = true;
				drawBlip[1] = true;
				break;
			}
		}
	}
}
