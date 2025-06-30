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

#include <Engine/Base/Changeable.h>
#include <Engine/Base/ChangeableRT.h>
#include <Engine/Base/Updateable.h>
#include <Engine/Base/UpdateableRT.h>
#include <Engine/Base/Timer.h>

/*
 * Constructor.
 */
CChangeable::CChangeable(void)
{
  ch_LastChangeTime = TIME(-1);
}

/*
 * Mark that something has changed in this object.
 */
void CChangeable::MarkChanged(void)
{
  ch_LastChangeTime = _pTimer->CurrentTick();
}

/*
 * Test if some updateable object is up to date with this changeable.
 */
BOOL CChangeable::IsUpToDate(const CUpdateable &ud) const
{
  return ch_LastChangeTime < ud.LastUpdateTime();
}

/*
 * Constructor.
 */
CChangeableRT::CChangeableRT(void)
{
  ch_LastChangeTime = TIME(-1);
}

/*
 * Mark that something has changed in this object.
 */
void CChangeableRT::MarkChanged(void)
{
  ch_LastChangeTime = _pTimer->GetRealTimeTick();
}

/*
 * Test if some updateable object is up to date with this changeable.
 */
BOOL CChangeableRT::IsUpToDate(const CUpdateableRT &ud) const
{
  return ch_LastChangeTime < ud.LastUpdateTime();
}

