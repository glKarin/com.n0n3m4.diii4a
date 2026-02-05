/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "ui/DeviceContext.h"
#include "ui/Window.h"
#include "ui/UserInterfaceLocal.h"

#include "ui/SimpleWindow.h"

extern idCVar r_scaleMenusTo169;

idSimpleWindow::idSimpleWindow(idWindow *win) {
	gui = win->GetGui();
	dc = win->dc;
	drawRect = win->drawRect;
	clientRect = win->clientRect;
	textRect = win->textRect;
	origin = win->origin;
	font = win->font;
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

	letterSpacing = win->letterSpacing; //blendo eric

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

	// blendo eric: custom font material
	fontMaterialName = win->fontMaterialName;
	fontMaterial = win->fontMaterial;
	if (!fontMaterial && fontMaterialName.Length())
	{
		fontMaterial = declManager->FindMaterial(fontMaterialName, false);
	}

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

		//BC
		if (textAlign.NeedsUpdate()) {
			parent->AddUpdateVar(&textAlign);
		}

		// blendo eric
		if (letterSpacing.NeedsUpdate()) {
			parent->AddUpdateVar(&letterSpacing);
		}
		if (fontMaterialName.NeedsUpdate()) {
			parent->AddUpdateVar(&fontMaterialName);
		}
		if (shear.NeedsUpdate()) {
			parent->AddUpdateVar(&shear);
		}
		if (textShadow.NeedsUpdate()) {
			parent->AddUpdateVar(&textShadow);
		}

	}
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

	bool allowFixup = (flags & WIN_RATIOCORRECT) && !(flags & WIN_NORATIOCORRECT);
	bool fixupFor43 = false;
	if (allowFixup && r_scaleMenusTo169.GetBool()) {
		fixupFor43 = true;
		dc->SetMenuScaleFix(true);
	}

	CalcClientRect(0, 0);
	dc->SetFont(font,fontMaterial); // blendo eric: supply font mat
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
		shadowRect.x += .6f; //BC was 1. NOTE: also change the 'shadowRect.x +=' line in window.cpp
		shadowRect.y += .6f; //BC was 1.

		dc->DrawText( shadowText, textScale, textAlign, textShadow == 1 ? colorBlack : idVec4(0,0,0,.5f) /*bc light shadow*/, shadowRect, !( flags & WIN_NOWRAP ), -1, false, NULL, 0, letterSpacing);
	}
	dc->DrawText(text, textScale, textAlign, foreColor, textRect, !( flags & WIN_NOWRAP ), -1, false, NULL, 0, letterSpacing);
	dc->SetTransformInfo(vec3_origin, mat3_identity);
	if ( flags & WIN_NOCLIP ) {
		dc->EnableClipping( true );
	}
	drawRect.Offset(-x, -y);
	clientRect.Offset(-x, -y);
	textRect.Offset(-x, -y);

	if (fixupFor43) { // DG: gotta reset that before returning this function
		dc->SetMenuScaleFix(false);
	}
}

intptr_t idSimpleWindow::GetWinVarOffset( idWinVar *wv, drawWin_t* owner) {
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

	//BC
	if (wv == &textAlign) {
		ret = (ptrdiff_t)&this->textAlign - (ptrdiff_t)this;
	}
	// blendo eric
	if (wv == &letterSpacing) {
		ret = (ptrdiff_t)&this->textAlign - (ptrdiff_t)this;
	}

	if ( ret != -1 ) {
		owner->simp = this;
	}
	return ret;
}

idWinVar *idSimpleWindow::GetWinVarByName(const char *_name) {
	idWinVar *retVar = NULL;
	if (idStr::Icmp(_name, "background") == 0) {
		retVar = &backGroundName;
	}
	if (idStr::Icmp(_name, "visible") == 0) {
		retVar = &visible;
	}
	if (idStr::Icmp(_name, "rect") == 0) {
		retVar = &rect;
	}
	if (idStr::Icmp(_name, "backColor") == 0) {
		retVar = &backColor;
	}
	if (idStr::Icmp(_name, "matColor") == 0) {
		retVar = &matColor;
	}
	if (idStr::Icmp(_name, "foreColor") == 0) {
		retVar = &foreColor;
	}
	if (idStr::Icmp(_name, "borderColor") == 0) {
		retVar = &borderColor;
	}
	if (idStr::Icmp(_name, "textScale") == 0) {
		retVar = &textScale;
	}
	if (idStr::Icmp(_name, "rotate") == 0) {
		retVar = &rotate;
	}
	if (idStr::Icmp(_name, "shear") == 0) {
		retVar = &shear;
	}
	if (idStr::Icmp(_name, "text") == 0) {
		retVar = &text;
	}

	//BC
	if (idStr::Icmp(_name, "textalign") == 0) {
		retVar = &textAlign;
	}
	// blendo eric
	if (idStr::Icmp(_name, "letterspacing") == 0) {
		retVar = &letterSpacing;
	}

	// blendo eric
	if (idStr::Icmp(_name, "fontmaterial") == 0) {
		retVar = &fontMaterialName;
	}
	if (idStr::Icmp(_name, "shear") == 0) {
		retVar = &shear;
	}
	if (idStr::Icmp(_name, "shadow") == 0) {
		retVar = &textShadow;
	}

	return retVar;
}

/*
========================
idSimpleWindow::WriteToSaveGame
========================
*/
void idSimpleWindow::WriteToSaveGame( idSaveGame *savefile ) const {
	savefile->WriteCheckType(SG_CHECK_UI);
	savefile->WriteCheckSizeMarker();

	savefile->WriteString( name ); // idString name

	//idUserInterfaceLocal *gui; // idUserInterfaceLocal * gui // initialized in constructor
	//idDeviceContext *dc; // idDeviceContext * dc // initialized in constructor

	savefile->WriteUInt( flags ); // unsigned int flags
	savefile->WriteRect( drawRect ); // idRectangle drawRect
	savefile->WriteRect( clientRect ); // idRectangle clientRect
	savefile->WriteRect( textRect ); // idRectangle textRect
	savefile->WriteVec2( origin ); // idVec2 origin

	savefile->WriteFloat( matScalex ); // float matScalex
	savefile->WriteFloat( matScaley ); // float matScaley
	savefile->WriteFloat( borderSize ); // float borderSize

	savefile->WriteFloat( textAlignx ); // float textAlignx
	savefile->WriteFloat( textAligny ); // float textAligny
	textShadow.WriteToSaveGame( savefile ); // idWinInt textShadow

	savefile->WriteCheckSizeMarker();

	text.WriteToSaveGame( savefile ); // idWinStr text
	visible.WriteToSaveGame( savefile ); // idWinBool visible
	rect.WriteToSaveGame( savefile ); // idWinRectangle rect
	backColor.WriteToSaveGame( savefile ); // idWinVec4 backColor
	matColor.WriteToSaveGame( savefile ); // idWinVec4 matColor
	foreColor.WriteToSaveGame( savefile ); // idWinVec4 foreColor
	borderColor.WriteToSaveGame( savefile ); // idWinVec4 borderColor
	textScale.WriteToSaveGame( savefile ); // idWinFloat textScale
	rotate.WriteToSaveGame( savefile ); // idWinFloat rotate
	shear.WriteToSaveGame( savefile ); // idWinVec2 shear
	textAlign.WriteToSaveGame( savefile ); // idWinInt textAlign
	letterSpacing.WriteToSaveGame( savefile ); // idWinFloat letterSpacing

	savefile->WriteString( font->GetName() ); // class idFont * font 

	fontMaterialName.WriteToSaveGame( savefile ); // idWinStr fontMaterialName
	savefile->WriteMaterial( fontMaterial );  // const idMaterial * fontMaterial

	backGroundName.WriteToSaveGame( savefile ); // idWinBackground backGroundName
	savefile->WriteMaterial( background ); // const idMaterial* background

	//savefile->WriteMiscPtr( CastWriteVoidPtrPtr(mParent) ); // idWindow * mParent

	hideCursor.WriteToSaveGame( savefile ); // idWinBool hideCursor

	savefile->WriteCheckSizeMarker();
	savefile->WriteCheckType(SG_CHECK_UI);
}

/*
========================
idSimpleWindow::ReadFromSaveGame
========================
*/
void idSimpleWindow::ReadFromSaveGame( idRestoreGame *savefile ) {
	savefile->ReadCheckType(SG_CHECK_UI);
	savefile->ReadCheckSizeMarker();

	savefile->ReadString( name ); // idString name

	//idUserInterfaceLocal *gui; // idUserInterfaceLocal * gui // initialized in constructor
	//idDeviceContext *dc; // idDeviceContext * dc // initialized in constructor

	savefile->ReadUInt( flags ); // unsigned int flags
	savefile->ReadRect( drawRect ); // idRectangle drawRect
	savefile->ReadRect( clientRect ); // idRectangle clientRect
	savefile->ReadRect( textRect ); // idRectangle textRect
	savefile->ReadVec2( origin ); // idVec2 origin

	savefile->ReadFloat( matScalex ); // float matScalex
	savefile->ReadFloat( matScaley ); // float matScaley
	savefile->ReadFloat( borderSize ); // float borderSize

	savefile->ReadFloat( textAlignx ); // float textAlignx
	savefile->ReadFloat( textAligny ); // float textAligny
	textShadow.ReadFromSaveGame( savefile ); // idWinInt textShadow

	savefile->ReadCheckSizeMarker();

	text.ReadFromSaveGame( savefile ); // idWinStr text
	visible.ReadFromSaveGame( savefile ); // idWinBool visible
	rect.ReadFromSaveGame( savefile ); // idWinRectangle rect
	backColor.ReadFromSaveGame( savefile ); // idWinVec4 backColor
	matColor.ReadFromSaveGame( savefile ); // idWinVec4 matColor
	foreColor.ReadFromSaveGame( savefile ); // idWinVec4 foreColor
	borderColor.ReadFromSaveGame( savefile ); // idWinVec4 borderColor
	textScale.ReadFromSaveGame( savefile ); // idWinFloat textScale
	rotate.ReadFromSaveGame( savefile ); // idWinFloat rotate
	shear.ReadFromSaveGame( savefile ); // idWinVec2 shear
	textAlign.ReadFromSaveGame( savefile ); // idWinInt textAlign
	letterSpacing.ReadFromSaveGame( savefile ); // idWinFloat letterSpacing

	// SM: From BFG
	idStr fontName;
	savefile->ReadString( fontName );
	font = renderSystem->RegisterFont( fontName ); // class idFont * font

	fontMaterialName.ReadFromSaveGame( savefile );  // idWinStr fontMaterialName
	savefile->ReadMaterial(fontMaterial);// const idMaterial * fontMaterial

	backGroundName.ReadFromSaveGame( savefile ); // idWinBackground backGroundName
	savefile->ReadMaterial( background ); // const idMaterial* background

	//savefile->ReadMiscPtr( CastReadVoidPtrPtr( mParent ) ); // idWindow * mParent

	hideCursor.ReadFromSaveGame( savefile ); // idWinBool hideCursor

	savefile->ReadCheckSizeMarker();
	savefile->ReadCheckType(SG_CHECK_UI);
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
