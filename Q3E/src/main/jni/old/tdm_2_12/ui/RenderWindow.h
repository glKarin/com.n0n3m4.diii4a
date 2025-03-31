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
#ifndef __RENDERWINDOW_H
#define __RENDERWINDOW_H

class idUserInterfaceLocal;
class idRenderWindow : public idWindow {
public:
	idRenderWindow(idUserInterfaceLocal *gui);
	idRenderWindow(idDeviceContext *d, idUserInterfaceLocal *gui);
	virtual ~idRenderWindow() override;

	virtual void PostParse() override;
	virtual void Draw(int time, float x, float y) override;
	virtual size_t Allocated() override{ return idWindow::Allocated(); }
// 
//  
	virtual idWinVar *GetThisWinVarByName(const char *varname) override;
// 
	
private:
	void CommonInit();
	virtual bool ParseInternalVar(const char *name, idParser *src) override;
	void Render(int time);
	void PreRender();
	void BuildAnimation(int time);
	renderView_t refdef;
	idRenderWorld *world;
	renderEntity_t worldEntity;
	renderLight_t rLight;
	const idMD5Anim *modelAnim;

	qhandle_t	worldModelDef;
	qhandle_t	lightDef;
	qhandle_t   modelDef;
	idWinStr modelName;
	idWinStr animName;
	idStr	 animClass;
	idWinVec4 lightOrigin;
	idWinVec4 lightColor;
	idWinVec4 modelOrigin;
	idWinVec4 modelRotate;
	idWinVec4 viewOffset;
	idWinBool needsRender;
	int animLength;
	int animEndTime;
	bool updateAnimation;
};

#endif // __RENDERWINDOW_H
