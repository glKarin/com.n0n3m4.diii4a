/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include "Engine/StdH.h"

#include <Engine/Base/Updateable.h>
#include <Engine/Base/UpdateableRT.h>
#include <Engine/Base/Timer.h>

/*
 * Constructor.
 */
CUpdateable::CUpdateable(void)
{
  ud_LastUpdateTime = TIME(-1);
}

/*
 * Mark that the object has been updated.
 */
void CUpdateable::MarkUpdated(void)
{
  ud_LastUpdateTime = _pTimer->CurrentTick();
}

/*
 * Get time when last updated.
 */
TIME CUpdateable::LastUpdateTime(void) const
{
  return ud_LastUpdateTime;
}

/* Mark that the object has become invalid in spite of its time stamp. */
void CUpdateable::Invalidate(void)
{
  ud_LastUpdateTime = TIME(-1);
}

/*
 * Constructor.
 */
CUpdateableRT::CUpdateableRT(void)
{
  ud_LastUpdateTime = TIME(-1);
}

/*
 * Mark that the object has been updated.
 */
void CUpdateableRT::MarkUpdated(void)
{
  ud_LastUpdateTime = _pTimer->GetRealTimeTick();
}

/*
 * Get time when last updated.
 */
TIME CUpdateableRT::LastUpdateTime(void) const
{
  return ud_LastUpdateTime;
}

/* Mark that the object has become invalid in spite of its time stamp. */
void CUpdateableRT::Invalidate(void)
{
  ud_LastUpdateTime = TIME(-1);
}
