/*
PlayerModelView.cpp -- player model view
Copyright (C) 2018 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#ifndef CMENUPLAYERMODELVIEW_H
#define CMENUPLAYERMODELVIEW_H

// HLSDK includes
#include "mathlib.h"
#include "const.h"
#include "keydefs.h"
#include "ref_params.h"
#include "cl_entity.h"
#include "com_model.h"
#include "entity_types.h"
// HLSDK includes end
#include "BaseItem.h"

class CMenuPlayerModelView : public CMenuBaseItem
{
public:
	CMenuPlayerModelView();
	void VidInit() override;
	void Draw() override;
	bool KeyDown( int key ) override;
	bool KeyUp( int key ) override;
	void CalcFov();

	HIMAGE hPlayerImage;

	ref_viewpass_t refdef;
	cl_entity_t *ent;

	bool bDrawAsPlayer;

	enum
	{
		PMV_DONTCARE = 0,
		PMV_SHOWMODEL,
		PMV_SHOWIMAGE
	} eOverrideMode;


	CColor backgroundColor;
	CColor outlineFocusColor;
private:
	bool mouseYawControl;

	int prevCursorX, prevCursorY;
};

#endif // CMENUPLAYERMODELVIEW_H
