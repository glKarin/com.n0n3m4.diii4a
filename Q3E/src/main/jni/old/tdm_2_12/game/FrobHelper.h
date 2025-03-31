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
#pragma once

// STiFU: Including SysCvar here will increase compile time, but it is necessary 
// for inlining below accessors
#include "gamesys/SysCvar.h"

typedef struct trace_s trace_t;

/** @brief	  	Handles visibility calculation of the FrobHelper cursor
  * @author		STiFU */
class CFrobHelper
{
public:
	
	CFrobHelper();
	~CFrobHelper();

	/** @brief	  	Hide the frob helper, skipping the fade-out
	  * @author		STiFU */
	void HideInstantly();
	
	/** @brief	  	Hide the FrobHelper with fade-out
	  * @author		STiFU */
	void Hide();
	
	/** @brief	  	Show the FrobHelper with fade-in and delay
	  * @author		STiFU */
	void Show();

	
	/** @brief	  	Check whether the passed entity is so big that it should be ignored
	  * @param 		pEntity	The entity that is supposed to be checked.
	  * @return   	True, if the passed entity should be ignored.
	  * @author		STiFU */
	const bool IsEntityIgnored(idEntity* pEntity);

	/** @brief	  	Retrieve the current visibility of the frob helper
	  * @return   	The visibility of the frob helper in the range [0,1]
	  * @author		STiFU */
	const float GetAlpha();

	/** @brief	  	Check whether the FrobHelper is supposed to used
	  * @remark		If the state changes, it will also reset all internal members
	  * @return   	True, if it is active
	  * @author		STiFU */
	inline const bool IsActive()
	{
		return cv_frobhelper_active.GetBool();
	}

private:

	inline const bool IsEntityTooBig(idEntity* pEntity)
	{
		const idVec3& entBounds = pEntity->GetPhysics()->GetBounds().GetSize();
		const float fMaxDim = Max3<float>(entBounds.x, entBounds.y, entBounds.z);
		return fMaxDim > cv_frobhelper_ignore_size.GetFloat();
	}
	
	inline void CheckCvars()
	{
		if (cv_frobhelper_alwaysVisible.GetBool() && cv_frobhelper_active.IsModified())
		{
			// stifu #4990: If always visible, players control frobhelper visibility 
			// via cv_frobhelper_active. So, remember current state whenever 
			// that cvar changes.
			m_iLastStateChangeTime = gameLocal.time;
			m_fLastStateChangeAlpha = m_fCurrentAlpha;
		}

		if (   cv_frobhelper_active.IsModified()
			|| cv_frobhelper_fadein_delay.IsModified()
			|| cv_frobhelper_fadein_duration.IsModified()
			|| cv_frobhelper_fadeout_duration.IsModified()
			|| cv_frobhelper_alpha.IsModified()
			|| cv_frobhelper_alwaysVisible.IsModified())
		{
			cv_frobhelper_active.ClearModified();
			cv_frobhelper_fadein_delay.ClearModified();
			cv_frobhelper_fadein_duration.ClearModified();
			cv_frobhelper_fadeout_duration.ClearModified();
			cv_frobhelper_alpha.ClearModified();
			cv_frobhelper_alwaysVisible.ClearModified();
			Reset();
		}
	}

	inline void Reset()
	{
		m_bShouldBeDisplayed = false;
		m_bReachedTargetAlpha = false;
		if (!cv_frobhelper_alwaysVisible.GetBool())
		{
			m_fCurrentAlpha = 0.0f;
			m_fLastStateChangeAlpha = 0.0f;
			m_iLastStateChangeTime = 0;
		}
	}
	
private:
	// State members
	// -> No locks required. All calls by same thread.
	bool	m_bShouldBeDisplayed;
	bool	m_bReachedTargetAlpha;
	float	m_fCurrentAlpha;
	float	m_fLastStateChangeAlpha;
	int		m_iLastStateChangeTime;
};
