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
#pragma hdrstop



#include "DeviceContext.h"
#include "Window.h"
#include "UserInterfaceLocal.h"
#include "SimpleWindow.h"


idSimpleWindow::idSimpleWindow(idWindow *win) {
	gui = win->GetGui();
	dc = win->dc;
	drawRect = win->drawRect;
	clientRect = win->clientRect;
	textRect = win->textRect;
	origin = win->origin;
	fontNum = win->fontNum;
	name = win->name;
	matScalex = win->matScalex;
	matScaley = win->matScaley;
	borderSize = win->borderSize;
	textAlign = win->textAlign;
	textAlignx = win->textAlignx;
	textAligny = win->textAligny;
	background = win->background;
	flags = win->flags;
	textShadow = win->textShadow;

	visible = win->visible;
	text = win->text;
	rect = win->rect;
	backColor = win->backColor;
	matColor = win->matColor;
	foreColor = win->foreColor;
	borderColor = win->borderColor;
	textScale = win->textScale;
	rotate = win->rotate;
	shear = win->shear;
	backGroundName = win->backGroundName;
	if (backGroundName.Length()) {
		background = declManager->FindMaterial(backGroundName);
		background->SetSort( SS_GUI );
		background->SetImageClassifications( 1 );	// just for resource tracking
	}
	backGroundName.SetMaterialPtr(&background);

// 
//  added parent
	mParent = win->GetParent();
// 

	hideCursor = win->hideCursor;

	idWindow *parent = win->GetParent();
	if (parent) {
		if (text.NeedsUpdate()) {
			parent->AddUpdateVar(&text);
		}
		if (visible.NeedsUpdate()) {
			parent->AddUpdateVar(&visible);
		}
		if (rect.NeedsUpdate()) {
			parent->AddUpdateVar(&rect);
		}
		if (backColor.NeedsUpdate()) {
			parent->AddUpdateVar(&backColor);
		}
		if (matColor.NeedsUpdate()) {
			parent->AddUpdateVar(&matColor);
		}
		if (foreColor.NeedsUpdate()) {
			parent->AddUpdateVar(&foreColor);
		}
		if (borderColor.NeedsUpdate()) {
			parent->AddUpdateVar(&borderColor);
		}
		if (textScale.NeedsUpdate()) {
			parent->AddUpdateVar(&textScale);
		}
		if (rotate.NeedsUpdate()) {
			parent->AddUpdateVar(&rotate);
		}
		if (shear.NeedsUpdate()) {
			parent->AddUpdateVar(&shear);
		}
		if (backGroundName.NeedsUpdate()) {
			parent->AddUpdateVar(&backGroundName);
		}
	}

	srcLocation = win->srcLocation;
}

idSimpleWindow::~idSimpleWindow() {

}

void idSimpleWindow::StateChanged( bool redraw ) {
	if ( redraw && background && background->CinematicLength() ) { 
		background->UpdateCinematic( gui->GetTime() );
	}
}

void idSimpleWindow::SetupTransforms(float x, float y) {
	static idMat3 trans;
	static idVec3 org;

	trans.Identity();
	org.Set( origin.x + x, origin.y + y, 0 );
	if ( rotate ) {
		static idRotation rot;
		static idVec3 vec( 0, 0, 1 );
		rot.Set( org, vec, rotate );
		trans = rot.ToMat3();
	}

	static idMat3 smat;
	smat.Identity();
	if (shear.x() || shear.y()) {
		smat[0][1] = shear.x();
		smat[1][0] = shear.y();
		trans *= smat;
	}

	if ( !trans.IsIdentity() ) {
		dc->SetTransformInfo( org, trans );
	}
}

void idSimpleWindow::DrawBackground(const idRectangle &drawRect) {
	if (backColor.w() > 0) {
		dc->DrawFilledRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, backColor);
	}

	if (background) {
		if (matColor.w() > 0) {
			float scalex, scaley;
			if ( flags & WIN_NATURALMAT ) {
				scalex = drawRect.w / background->GetImageWidth();
				scaley = drawRect.h / background->GetImageHeight();
			} else {
				scalex = matScalex;
				scaley = matScaley;
			}
			dc->DrawMaterial(drawRect.x, drawRect.y, drawRect.w, drawRect.h, background, matColor, scalex, scaley);
		}
	}
}

void idSimpleWindow::DrawBorderAndCaption(const idRectangle &drawRect) {
	if (flags & WIN_BORDER) {
		if (borderSize) {
			dc->DrawRect(drawRect.x, drawRect.y, drawRect.w, drawRect.h, borderSize, borderColor);
		}
	}
}

void idSimpleWindow::CalcClientRect(float xofs, float yofs) {

	drawRect = rect;

	if ( flags & WIN_INVERTRECT ) {
		drawRect.x = rect.x() - rect.w();
		drawRect.y = rect.y() - rect.h();
	}
	
	drawRect.x += xofs;
	drawRect.y += yofs;

	clientRect = drawRect;
	if (rect.h() > 0.0 && rect.w() > 0.0) {

		if (flags & WIN_BORDER && borderSize != 0.0) {
			clientRect.x += borderSize;
			clientRect.y += borderSize;
			clientRect.w -= borderSize;
			clientRect.h -= borderSize;
		}

		textRect = clientRect;
		textRect.x += 2.0;
	 	textRect.w -= 2.0;
		textRect.y += 2.0;
		textRect.h -= 2.0;
		textRect.x += textAlignx;
		textRect.y += textAligny;

	}
	origin.Set( rect.x() + ( rect.w() / 2 ), rect.y() + ( rect.h() / 2 ) );

}


void idSimpleWindow::Redraw(float x, float y) {
	
	if (!visible) {
		return;
	}

	CalcClientRect(0, 0);
	dc->SetFont(fontNum);
	drawRect.Offset(x, y);
	clientRect.Offset(x, y);
	textRect.Offset(x, y);
	SetupTransforms(x, y);
	if ( flags & WIN_NOCLIP ) {
		dc->EnableClipping( false );
	}
	DrawBackground(drawRect);
	DrawBorderAndCaption(drawRect);
	if ( textShadow ) {
		idStr shadowText = text;
		idRectangle shadowRect = textRect;

		shadowText.RemoveColors();
		shadowRect.x += textShadow;
		shadowRect.y += textShadow;

		dc->DrawText( shadowText, textScale, textAlign, colorBlack, shadowRect, !( flags & WIN_NOWRAP ), -1 );
	}
	dc->DrawText(text, textScale, textAlign, foreColor, textRect, !( flags & WIN_NOWRAP ), -1);
	dc->SetTransformInfo(vec3_origin, mat3_identity);
	if ( flags & WIN_NOCLIP ) {
		dc->EnableClipping( true );
	}
	drawRect.Offset(-x, -y);
	clientRect.Offset(-x, -y);
	textRect.Offset(-x, -y);
}

intptr_t idSimpleWindow::GetWinVarOffset(idWinVar *wv, drawWin_t* owner) {
    intptr_t ret = -1;

	if ( wv == &rect ) {
		ret = (ptrdiff_t)&this->rect - (ptrdiff_t)this;
	}

	if ( wv == &backColor ) {
		ret = (ptrdiff_t)&this->backColor - (ptrdiff_t)this;
	}

	if ( wv == &matColor ) {
		ret = (ptrdiff_t)&this->matColor - (ptrdiff_t)this;
	}

	if ( wv == &foreColor ) {
		ret = (ptrdiff_t)&this->foreColor - (ptrdiff_t)this;
	}

	if ( wv == &borderColor ) {
		ret = (ptrdiff_t)&this->borderColor - (ptrdiff_t)this;
	}

	if ( wv == &textScale ) {
		ret = (ptrdiff_t)&this->textScale - (ptrdiff_t)this;
	}

	if ( wv == &rotate ) {
		ret = (ptrdiff_t)&this->rotate - (ptrdiff_t)this;
	}

	if ( ret != -1 ) {
		owner->simp = this;
	}
	return ret;
}

idWinVar *idSimpleWindow::GetThisWinVarByName(const char *varname) {
	idWinVar *retVar = NULL;
	if (idStr::Icmp(varname, "background") == 0) {
		retVar = &backGroundName;
	}
	if (idStr::Icmp(varname, "visible") == 0) {
		retVar = &visible;
	}
	if (idStr::Icmp(varname, "rect") == 0) {
		retVar = &rect;
	}
	if (idStr::Icmp(varname, "backColor") == 0) {
		retVar = &backColor;
	}
	if (idStr::Icmp(varname, "matColor") == 0) {
		retVar = &matColor;
	}
	if (idStr::Icmp(varname, "foreColor") == 0) {
		retVar = &foreColor;
	}
	if (idStr::Icmp(varname, "borderColor") == 0) {
		retVar = &borderColor;
	}
	if (idStr::Icmp(varname, "textScale") == 0) {
		retVar = &textScale;
	}
	if (idStr::Icmp(varname, "rotate") == 0) {
		retVar = &rotate;
	}
	if (idStr::Icmp(varname, "shear") == 0) {
		retVar = &shear;
	}
	if (idStr::Icmp(varname, "text") == 0) {
		retVar = &text;
	}
	return retVar;
}

/*
========================
idSimpleWindow::WriteToSaveGame
========================
*/
void idSimpleWindow::WriteToSaveGame( idFile *savefile ) {

	savefile->Write( &flags, sizeof( flags ) );
	savefile->Write( &drawRect, sizeof( drawRect ) );
	savefile->Write( &clientRect, sizeof( clientRect ) );
	savefile->Write( &textRect, sizeof( textRect ) );
	savefile->Write( &origin, sizeof( origin ) );
	savefile->Write( &fontNum, sizeof( fontNum ) );
	savefile->Write( &matScalex, sizeof( matScalex ) );
	savefile->Write( &matScaley, sizeof( matScaley ) );
	savefile->Write( &borderSize, sizeof( borderSize ) );
	savefile->Write( &textAlign, sizeof( textAlign ) );
	savefile->Write( &textAlignx, sizeof( textAlignx ) );
	savefile->Write( &textAligny, sizeof( textAligny ) );
	savefile->Write( &textShadow, sizeof( textShadow ) );

	text.WriteToSaveGame( savefile );
	visible.WriteToSaveGame( savefile );
	rect.WriteToSaveGame( savefile );
	backColor.WriteToSaveGame( savefile );
	matColor.WriteToSaveGame( savefile );
	foreColor.WriteToSaveGame( savefile );
	borderColor.WriteToSaveGame( savefile );
	textScale.WriteToSaveGame( savefile );
	rotate.WriteToSaveGame( savefile );
	shear.WriteToSaveGame( savefile );
	backGroundName.WriteToSaveGame( savefile );

	int stringLen;

	if ( background ) {
        stringLen = static_cast<int>(strlen(background->GetName()));
		savefile->Write( &stringLen, sizeof( stringLen ) );
		savefile->Write( background->GetName(), stringLen );
	} else {
		stringLen = 0;
		savefile->Write( &stringLen, sizeof( stringLen ) );
	}

}

/*
========================
idSimpleWindow::ReadFromSaveGame
========================
*/
void idSimpleWindow::ReadFromSaveGame( idFile *savefile ) {

	savefile->Read( &flags, sizeof( flags ) );
	savefile->Read( &drawRect, sizeof( drawRect ) );
	savefile->Read( &clientRect, sizeof( clientRect ) );
	savefile->Read( &textRect, sizeof( textRect ) );
	savefile->Read( &origin, sizeof( origin ) );
	savefile->Read( &fontNum, sizeof( fontNum ) );
	savefile->Read( &matScalex, sizeof( matScalex ) );
	savefile->Read( &matScaley, sizeof( matScaley ) );
	savefile->Read( &borderSize, sizeof( borderSize ) );
	savefile->Read( &textAlign, sizeof( textAlign ) );
	savefile->Read( &textAlignx, sizeof( textAlignx ) );
	savefile->Read( &textAligny, sizeof( textAligny ) );
	savefile->Read( &textShadow, sizeof( textShadow ) );

	text.ReadFromSaveGame( savefile );
	visible.ReadFromSaveGame( savefile );
	rect.ReadFromSaveGame( savefile );
	backColor.ReadFromSaveGame( savefile );
	matColor.ReadFromSaveGame( savefile );
	foreColor.ReadFromSaveGame( savefile );
	borderColor.ReadFromSaveGame( savefile );
	textScale.ReadFromSaveGame( savefile );
	rotate.ReadFromSaveGame( savefile );
	shear.ReadFromSaveGame( savefile );
	backGroundName.ReadFromSaveGame( savefile );

	int stringLen;

	savefile->Read( &stringLen, sizeof( stringLen ) );
	if ( stringLen > 0 ) {
		idStr backName;

		backName.Fill( ' ', stringLen );
		savefile->Read( &(backName)[0], stringLen );

		background = declManager->FindMaterial( backName );
		background->SetSort( SS_GUI );
	} else {
		background = NULL;
	}

}


/*
===============================
*/

size_t idSimpleWindow::Size() {
	size_t sz = sizeof(*this);
	sz += name.Size();
	sz += text.Size();
	sz += backGroundName.Size();
	return sz;
}
