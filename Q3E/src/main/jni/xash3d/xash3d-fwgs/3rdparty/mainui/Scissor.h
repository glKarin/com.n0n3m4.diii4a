/*
Scissor.h -- scissor utils
Copyright (C) 2017 mittorn

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
#pragma once
#ifndef SCISSOR_H
#define SCISSOR_H

#include "BaseMenu.h"

namespace UI
{
namespace Scissor
{
void PushScissor( int x, int y, int w, int h );
inline void PushScissor( Point pt, Size sz ) { PushScissor( pt.x, pt.y, sz.w, sz.h ); }

void PopScissor();
}
}

#endif // SCISSOR_H
