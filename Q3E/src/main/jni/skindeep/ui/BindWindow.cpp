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
#include "idlib/LangDict.h"
#include "framework/KeyInput.h"
#include "ui/DeviceContext.h"
#include "ui/Window.h"
#include "ui/UserInterfaceLocal.h"

#include "ui/BindWindow.h"
#include "Game_local.h"

extern idCVar in_forceButtonPrompts;

void idBindWindow::CommonInit() {
	bindName = "";
	waitingOnKey = false;
	isGamepad = false;
}

idBindWindow::idBindWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g) {
	dc = d;
	gui = g;
	CommonInit();
}

idBindWindow::idBindWindow(idUserInterfaceLocal *g) : idWindow(g) {
	gui = g;
	CommonInit();
}

idBindWindow::~idBindWindow() {

}

void idBindWindow::WriteToSaveGame( idSaveGame *savefile ) const
{
	idWindow::WriteToSaveGame( savefile );

	savefile->WriteBool( waitingOnKey ); // bool waitingOnKey
	savefile->WriteBool( isGamepad ); // bool isGamepad

	savefile->WriteCheckSizeMarker();
}

void idBindWindow::ReadFromSaveGame( idRestoreGame *savefile )
{
	idWindow::ReadFromSaveGame( savefile );

	savefile->ReadBool( waitingOnKey ); // bool waitingOnKey
	savefile->ReadBool( isGamepad ); // bool isGamepad

	savefile->ReadCheckSizeMarker();
}

const char *idBindWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {
	static char ret[ 256 ];

	if (!(event->evType == SE_KEY && event->evValue2)) {
		return "";
	}

	int key = event->evValue;

	if (waitingOnKey) {
		waitingOnKey = false;
		if (key == K_ESCAPE) {
			idStr::snPrintf(ret, sizeof(ret), "play cancel");
		} else if (isGamepad) {
			if (key >= K_JOY1 && key <= K_JOY_DPAD_RIGHT) {
				idStr::snPrintf(ret, sizeof(ret), "play click1; rebindGamepad %i \"%s\"", key, bindName.c_str());
			} else {
				waitingOnKey = true;
			}
		} else if (!isGamepad) {
			if (key < K_JOY1 || key > K_JOY_DPAD_RIGHT) {
				idStr::snPrintf(ret, sizeof(ret), "play click1; rebindKeyboard %i \"%s\"", key, bindName.c_str());
			} else {
				waitingOnKey = true;
			}
		}

		// Still waiting for a valid key press so dont' do anything
		if (waitingOnKey) {
			return "";
		}

		return ret;
	} else {
		if (key == K_MOUSE1 || JoyEmulateMouseClick(event)) {
			waitingOnKey = true;
			gui->SetBindHandler(this);
			return "play click1";
		} else if ((key == K_BACKSPACE || key == K_DEL)) {
			if (isGamepad)
			{
				//BC allow deleting keybinds.
				idStr::snPrintf(ret, sizeof(ret), "play cancel; clearGamepadBind \"%s\"", bindName.c_str());
			}
			else
			{
				//BC allow deleting keybinds.
				idStr::snPrintf(ret, sizeof(ret), "play cancel; clearKeyboardBind \"%s\"", bindName.c_str());
			}
			return ret;
		}
	}

	return "";
}

idWinVar *idBindWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner) {
	return idWindow::GetWinVarByName(_name, fixup,owner);
}

void idBindWindow::PostParse() {
	idWindow::PostParse();
	flags |= (WIN_HOLDCAPTURE | WIN_CANFOCUS);
}

void idBindWindow::Draw(int time, float x, float y) {
	idVec4 color = foreColor;

	bool forceShowText = false;
	idStr str;
	if ( waitingOnKey ) {
		str = common->GetLanguageDict()->GetString( "#str_07000" ); //"Press a key..."
		forceShowText = true;
	} else if ( bindName.Length() ) {
		str = gameLocal.GetKeyFromBinding(bindName.c_str(), controllerBinding);
	} else {
		str = common->GetLanguageDict()->GetString( "#str_07001" ); //"???"
		forceShowText = true;
	}

	if (str[0] == '<') // "<EMPTY>"
	{
		forceShowText = true;
	}

	if ( waitingOnKey || OverClientRect() ) {
		color = hoverColor;
	} else {
		hover = false;
	}

	idStr btnName;
	if (controllerBinding)
	{
		int controllerType = Sys_GetLastControllerType();
		if (in_forceButtonPrompts.GetInteger() > 0)
		{
			controllerType = in_forceButtonPrompts.GetInteger() - 1;
		}

		btnName = gameLocal.controllerButtonDicts[controllerType].GetString(str);
	}
	else
	{
		btnName = gameLocal.mouseButtonDict.GetString(str);
	}

	if ((!controllerBinding && btnName.IsEmpty()) || forceShowText || btnName.IsEmpty())
	{
		dc->DrawText(str, textScale, textAlign, color, textRect, false, -1);
	}
	else
	{
		//Draw gamepad button.
		const idMaterial* material = declManager->FindMaterial(btnName);
		float lineSkip = dc->MaxCharHeight(textScale);
		float imgSize = lineSkip * 1.1f * buttonScale;
		dc->EnableClipping(false);
		idVec4 btnColor = colorWhite;
		btnColor.w = foreColor.w();

		int alignment = buttonAlignOverride != -1 ? buttonAlignOverride : textAlign;
		float x = textRect.x + buttonOffsetx;
		if (alignment == idDeviceContext::ALIGN_RIGHT) {
			x = textRect.x + textRect.w - imgSize + buttonOffsetx;
		}
		else if (alignment == idDeviceContext::ALIGN_CENTER) {
			x = textRect.x + (textRect.w - imgSize) / 2 + buttonOffsetx;
		}

		dc->DrawMaterial(x, textRect.y + buttonOffsety, imgSize, imgSize * 1.33f, material, btnColor);
		dc->EnableClipping(true);
	}
}

void idBindWindow::Activate( bool activate, idStr &act ) {
	idWindow::Activate( activate, act );
	bindName.Update();
}


bool idBindWindow::ParseInternalVar(const char* name, idParser* src)
{
	if (idWindow::ParseInternalVar(name, src))
	{
		return true;
	}

	if (idStr::Icmp(name, "gamepad") == 0) {
		isGamepad = src->ParseBool();
		return true;
	}

	return false;
}

void idBindDisplayWindow::CommonInit() {
	bindName = "";
	cursorAttachActive = false;
}

idBindDisplayWindow::idBindDisplayWindow(idDeviceContext* d, idUserInterfaceLocal* g) : idWindow(d, g) {
	dc = d;
	gui = g;
	CommonInit();
}

idBindDisplayWindow::idBindDisplayWindow(idUserInterfaceLocal* g) : idWindow(g) {
	gui = g;
	CommonInit();
}

idBindDisplayWindow::~idBindDisplayWindow() {

}

void idBindDisplayWindow::WriteToSaveGame( idSaveGame *savefile ) const
{
	idWindow::WriteToSaveGame( savefile );

	savefile->WriteBool(cursorAttachActive); // bool cursorAttachActive

	savefile->WriteCheckSizeMarker();
}
void idBindDisplayWindow::ReadFromSaveGame( idRestoreGame *savefile )
{
	idWindow::ReadFromSaveGame( savefile );

	savefile->ReadBool(cursorAttachActive); // bool cursorAttachActive

	savefile->ReadCheckSizeMarker();
}

idWinVar* idBindDisplayWindow::GetWinVarByName(const char* _name, bool fixup, drawWin_t** owner) {

	if (idStr::Icmp(_name, "bind") == 0) {
		return &bindName;
	}

	return idWindow::GetWinVarByName(_name, fixup, owner);
}

void idBindDisplayWindow::PostParse() {
	idWindow::PostParse();
	bindName.Update();
}

void idBindDisplayWindow::Draw(int time, float x, float y)
{
	if(cursorBound)
	{ // check for changes in cursor type and activeness, and call related scripted gui events for animation
		bool cursorActive = gui->Active() && gui->IsInteractive() && (dc->GetCursor() == idDeviceContext::CURSOR_HAND);

		if(cursorActive != cursorAttachActive)
		{
			if(cursorAttachActive)
			{
				gui->HandleNamedEvent("cursorBindInactive");
			}
			else
			{
				gui->HandleNamedEvent("cursorBindActive");
			}
			cursorAttachActive = cursorActive;
		}

		if( foreColor.w() <= 0.0 ) return;
	}

	idStr bindDisplay = bindName.c_str();
	if (bindDisplay.Length() == 0) {
		// If the string is empty, don't draw anything
		return;
	}

	bool usingJoystick = usercmdGen->IsUsingJoystick();
	if (in_forceButtonPrompts.GetInteger() == 0)
	{
		usingJoystick = false;
	}
	else if (in_forceButtonPrompts.GetInteger() > 0)
	{
		usingJoystick = true;
	}

	idStr rawkey = gameLocal.GetKeyFromBinding(bindDisplay.c_str(), usingJoystick);
	idStr btnName;

	if (usingJoystick)
	{
		int controllerType = Sys_GetLastControllerType();
		if (in_forceButtonPrompts.GetInteger() > 0)
		{
			controllerType = in_forceButtonPrompts.GetInteger() - 1;
		}

		btnName = gameLocal.controllerButtonDicts[controllerType].GetString(rawkey);
	}
	else
	{
		btnName = gameLocal.mouseButtonDict.GetString(rawkey);
	}

	if ((!usingJoystick && btnName.IsEmpty()) || rawkey[0] == '<' || btnName.IsEmpty())
	{
#if 0 // attempt at scaling text to window, busted due to textscale not linear scaling
		float origTextScale = textScale;
		float newTextScale = textScale;
		float rectWidth = drawRect.w;

		// shrink for long keynames
		newTextScale = newTextScale*idMath::Fabs(rectWidth/(dc->TextWidth( text, newTextScale, 10 )+0.0001f));

		textScale = newTextScale;

		//Draw keyboard/mouse button.
		text = rawkey;
		idWindow::Draw(time, x, y);

		textScale = origTextScale;
#else
		text = rawkey;
		idWindow::Draw(time, x, y);
#endif
	}
	else
	{
		//Draw gamepad button.
		const idMaterial* material = declManager->FindMaterial(btnName);
		float lineSkip = dc->MaxCharHeight(textScale);
		float imgSize = lineSkip * 1.1f * buttonScale;
		dc->EnableClipping(false);
		idVec4 btnColor = colorWhite;
		btnColor.w = foreColor.w();

		int alignment = buttonAlignOverride != -1 ? buttonAlignOverride : textAlign;
		float x = textRect.x + buttonOffsetx;
		if (alignment == idDeviceContext::ALIGN_RIGHT) {
			x = textRect.x + textRect.w - imgSize + buttonOffsetx;
		}
		else if (alignment == idDeviceContext::ALIGN_CENTER) {
			x = textRect.x + (textRect.w - imgSize) / 2 + buttonOffsetx;
		}

		dc->DrawMaterial(x, textRect.y + buttonOffsety, imgSize, imgSize * 1.33f, material, btnColor);
		dc->EnableClipping(true);
	}
}

void idBindDisplayWindow::DrawBackground(const idRectangle& drawRect)
{
	if( matColor.w() <= 0.0f && backColor.w() <= 0.0f ) return;

	bool usingJoystick = usercmdGen->IsUsingJoystick();
	if (in_forceButtonPrompts.GetInteger() == 0)
	{
		usingJoystick = false;
	}
	else if (in_forceButtonPrompts.GetInteger() > 0)
	{
		usingJoystick = true;
	}

	idStr rawkey = gameLocal.GetKeyFromBinding(bindName.c_str(), usingJoystick);
	idStr btnName;
	if (!usingJoystick)
	{
		btnName = gameLocal.mouseButtonDict.GetString(rawkey);
	}

	if (!usingJoystick && strlen(bindName.c_str()) != 0 && btnName.Length() == 0)
	{
		idWindow::DrawBackground(drawRect);
	}
}

void idBindDisplayWindow::CalcClientRect(float xofs, float yofs)
{
	if(cursorBound)
	{
		idRectangle origRect = rect;
		rect = rect.ToVec4() + idVec4(gui->CursorX(),gui->CursorY(),0,0);
		idWindow::CalcClientRect( xofs, yofs );
		rect = origRect;
	}
	else
	{
		idWindow::CalcClientRect( xofs, yofs );
	}
}
