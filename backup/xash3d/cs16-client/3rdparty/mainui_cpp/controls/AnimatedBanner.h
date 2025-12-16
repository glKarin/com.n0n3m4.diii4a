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

#ifndef ANIMATED_TITLE_H
#define ANIMATED_TITLE_H

#include "BaseItem.h"
#include "Image.h"

#define ART_BLIP_NUM  2
#define ART_BLUR_NUM  4

class CMenuAnimatedBanner : public CMenuBaseItem
{
public:
	virtual void VidInit() override;
	virtual void Draw() override;
	virtual void Think() override;

	virtual bool TryLoad();
private:
	void RandomizeGoalTime( float time );
	void RandomizeSpeed( float initial_speed );
	void RandomizeBlipTime( float time );
	void RandomizeBlip( void );

	float scale;

	CImage logo;
	CImage logoBlip[ART_BLIP_NUM];
	CImage logoBlur[ART_BLUR_NUM];
	CImage logoBlurBlip[ART_BLUR_NUM];

	int   m_nLogoImageXMin;
	int   m_nLogoImageXMax;
	int   m_nLogoImageXGoal;
	float m_flPrevFrameTime;
	float m_flTimeLogoNewGoal;
	float m_flTimeLogoNewGoalMin;
	float m_flTimeLogoNewGoalMax;
	float m_flTimeUntilLogoBlipMin;
	float m_flTimeUntilLogoBlipMax;
	float m_flTimeLogoBlip;
	int   m_nLogoBlipType;
	int   m_nLogoImageY;
	float m_fLogoImageX;
	float m_fLogoSpeedMin;
	float m_fLogoSpeedMax;
	float m_fLogoSpeed;
	int   m_nLogoBGOffsetX;
	int   m_nLogoBGOffsetY;

	enum LogoBlip_t
	{
		E_LOGO_BLIP_BOTH,
		E_LOGO_BLIP_JUST_LOGO,
		E_LOGO_BLIP_JUST_BG,
		E_LOGO_BLIP_STAGGER,
		E_LOGO_BLIP_BOTH_SHOW_BLIP_LOGO_ONLY
	} m_nNextLogoBlipType;

	// not referenced
	bool drawBlip[ART_BLIP_NUM];
	bool drawBgBlip;

	Size trueLogoSz;
	Size trueLogoBlurSz[ART_BLUR_NUM];
};

#endif // ANIMATED_TITLE_H
