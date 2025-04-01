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
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../../ui/EditWindow.h"
#include "../../ui/ListWindow.h"
#include "../../ui/BindWindow.h"
#include "../../ui/RenderWindow.h"
#include "../../ui/ChoiceWindow.h"

#include "GEWindowWrapper.h"

static rvGEWindowWrapper stub_wrap( NULL, rvGEWindowWrapper::WT_UNKNOWN );

rvGEWindowWrapper::rvGEWindowWrapper( idWindow* window, EWindowType type ) { }

rvGEWindowWrapper* rvGEWindowWrapper::GetWrapper ( idWindow* window ) { return &stub_wrap; }

void rvGEWindowWrapper::SetStateKey( const char*, const char*, bool ) { }

void rvGEWindowWrapper::Finish() { }
