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
#include "FrobHelper.h"
#include "Game_local.h"
#include "FrobLock.h"
#include "FrobLockHandle.h"
#include "FrobDoorHandle.h"
#include <limits>

#pragma hdrstop

extern idGameLocal			gameLocal;

CFrobHelper::CFrobHelper()
: m_bShouldBeDisplayed(false)
, m_fCurrentAlpha(0.0f)
, m_fLastStateChangeAlpha(0.0f)
, m_iLastStateChangeTime(0)
, m_bReachedTargetAlpha(true)
{
}

CFrobHelper::~CFrobHelper()
{
}


void CFrobHelper::HideInstantly()
{
	if (!IsActive())
		return;

	if (m_fCurrentAlpha > 0.0f)
	{
		m_fCurrentAlpha = 0.0f;
		m_fLastStateChangeAlpha = 0.0f;
		m_bShouldBeDisplayed = false;
		m_bReachedTargetAlpha = true;
	}	
}


void CFrobHelper::Hide()
{
	if (!IsActive())
		return;

	if (m_bShouldBeDisplayed)
	{
		m_bShouldBeDisplayed	= false;
		m_iLastStateChangeTime	= gameLocal.time;
		m_fLastStateChangeAlpha = m_fCurrentAlpha;
		m_bReachedTargetAlpha	= false;
	}
}


void CFrobHelper::Show()
{
	if (!IsActive())
		return;

	if (!m_bShouldBeDisplayed)
	{
		m_bShouldBeDisplayed	= true;
		m_iLastStateChangeTime	= gameLocal.time;
		m_fLastStateChangeAlpha = m_fCurrentAlpha;
		m_bReachedTargetAlpha	= false;
	}
}


const bool CFrobHelper::IsEntityIgnored(idEntity* pEntity)
{
	if (!pEntity)
		return true;

	if (cv_frobhelper_ignore_size.GetFloat() < FLT_EPSILON)
		// Ignore size is disabled
		return false;

	// If the entity cannot be picked up, check the whole entity team because
	// the entity might be part of a big object that is supposed to be ignored
	if (!pEntity->CanBePickedUp() && pEntity->GetTeamMaster() != NULL)
	{
		for (idEntity* pEntityIt = pEntity->GetTeamMaster(); pEntityIt != NULL; pEntityIt = pEntityIt->GetNextTeamEntity())
		{
			if (IsEntityTooBig(pEntityIt))
				return true;
		}
		return false;
	}

	// Entity is not a team member or can be picked up. Just check its size
	return IsEntityTooBig(pEntity);
}


const float CFrobHelper::GetAlpha()
{
	if (!IsActive())
		return 0.0f;

	// Daft Mugi #6318: Ensure frob helper is always visible when
	// both tdm_frobhelper_active and tdm_frobhelper_alwaysVisible are true.
	if (cv_frobhelper_alwaysVisible.GetBool())
	{
		m_fCurrentAlpha = cv_frobhelper_alpha.GetFloat();
		m_bReachedTargetAlpha = true;
		return m_fCurrentAlpha;
	}

	CheckCvars();

	if (m_bReachedTargetAlpha)
		// Early return for higher performance
		return m_fCurrentAlpha;

	const int iTime = gameLocal.time;

	// Calculate current FrobHelper alpha based on delay and fade-in / fade-out
	if (!cv_frobhelper_alwaysVisible.GetBool() && m_bShouldBeDisplayed 
		|| cv_frobhelper_alwaysVisible.GetBool() && cv_frobhelper_active.GetBool())
	{		
		// Skip the fade delay if a fade was already active
		const bool bPreviousFadeoutNotCompleted = m_fLastStateChangeAlpha > 0.0f;
		const int iFadeStart = m_iLastStateChangeTime 			
			+ (bPreviousFadeoutNotCompleted ? 0 : cv_frobhelper_fadein_delay.GetInteger());
		if (iTime < iFadeStart)
			return 0.0f;

		// Calculate fade end time
		// > If there was a previous unfinished fade, reduce the fade end time to the needed value
		const float fFadeDurationFactor =
			fabs(cv_frobhelper_alpha.GetFloat() - m_fLastStateChangeAlpha) / cv_frobhelper_alpha.GetFloat();
		const int iFadeEnd = iFadeStart 
			+ static_cast<int>(fFadeDurationFactor * static_cast<float>(cv_frobhelper_fadein_duration.GetInteger()));

		if (iTime < iFadeEnd)
		{
			const float fFadeTimeTotal = static_cast<float>(iFadeEnd - iFadeStart);
			m_fCurrentAlpha = m_fLastStateChangeAlpha + abs(cv_frobhelper_alpha.GetFloat() - m_fLastStateChangeAlpha)
				* static_cast<float>(iTime - iFadeStart)
				/ fFadeTimeTotal;
		}
		else if (!m_bReachedTargetAlpha)
		{
			m_fCurrentAlpha = cv_frobhelper_alpha.GetFloat();
			m_bReachedTargetAlpha = true;
		}
	} else // Should not be displayed
	{	
		// Calculate fade ent time
		// > If there was a previous unfinished fade, reduce the fade end time to the needed value
		const float fFadeDurationFactor =
			m_fLastStateChangeAlpha / cv_frobhelper_alpha.GetFloat();
		const int iFadeEnd = m_iLastStateChangeTime 
			+ static_cast<int>(fFadeDurationFactor*static_cast<float>(cv_frobhelper_fadeout_duration.GetInteger()));

		// Calculate FrobHelper alpha based on fade out time
		if (iTime < iFadeEnd)
		{
			m_fCurrentAlpha = m_fLastStateChangeAlpha *
				(1.0f - static_cast<float>(iTime - m_iLastStateChangeTime)
					    / static_cast<float>(cv_frobhelper_fadeout_duration.GetInteger()) );
		}
		else if (!m_bReachedTargetAlpha)
		{
			m_fCurrentAlpha = 0.0;
			m_bReachedTargetAlpha = true;
		}
	}

	return m_fCurrentAlpha;
}
