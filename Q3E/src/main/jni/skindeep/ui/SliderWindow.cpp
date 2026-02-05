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
#include "framework/KeyInput.h"
#include "ui/DeviceContext.h"
#include "ui/Window.h"
#include "ui/UserInterfaceLocal.h"

#include "ui/SliderWindow.h"


// blendo eric: usability cvars for scrollbar sliders
idCVar gui_slider_thumbDefWide( "gui_slider_thumbDefWide", "16", CVAR_SYSTEM | CVAR_FLOAT, "replaces the thumb/icon/nub default minimum width of the scrollbar/slider, must reload gui");
idCVar gui_slider_thumbDefLong( "gui_slider_thumbDefLong", "24", CVAR_SYSTEM | CVAR_FLOAT, "replaces the thumb/icon/nub default minimum length along the scrollbar/slider, must reload gui");
idCVar gui_slider_thumbLongMax( "gui_slider_thumbLongMax", "1.0f", CVAR_SYSTEM | CVAR_FLOAT, "max length of the thumb/icon/nub slider as a percentage of the slider bar length");

/*
============
idSliderWindow::CommonInit
============
*/
void idSliderWindow::CommonInit() {
	value = 0.0;
	low = 0.0;
	high = 100.0;
	stepSize = 1.0;
	thumbMat = declManager->FindMaterial("_default");
	buddyWin = NULL;

	cvar = NULL;
	cvar_init = false;
	liveUpdate = true;

	vertical = false;
	scrollbar = false;

	verticalFlip = false;

	//BC
	modifiedSum = 0;
	modifiedMultiplier = 1.0f;
	// blendo eric
	visiblePercent = 0.0f;
	thumbWidth = 0;
	thumbHeight = 0;
	thumbImgSize = idVec2(1.0f,1.0f);
	thumbFillAuto = false;
	thumbCapAuto = false;
	thumbCapRescale = 1.0f;
	thumbCapRect = idRectangle(0,0,1.0f,1.0f);
	sliderThumbLockColor = vec4_zero;
	sliderBarLockColor = vec4_zero;
	initThumbWidth = true;
	initThumbHeight = true;
	initThumbColor = true;
	initThumbAuto = true;
	initHoverColor = true;
	initBarColor = true;
}

idSliderWindow::idSliderWindow(idDeviceContext *d, idUserInterfaceLocal *g) : idWindow(d, g) {
	dc = d;
	gui = g;
	CommonInit();
}

idSliderWindow::idSliderWindow(idUserInterfaceLocal *g) : idWindow(g) {
	gui = g;
	CommonInit();
}

idSliderWindow::~idSliderWindow() {

}

void idSliderWindow::WriteToSaveGame( idSaveGame *savefile ) const
{
	idWindow::WriteToSaveGame( savefile );

	value.WriteToSaveGame( savefile ); // idWinFloat value
	savefile->WriteFloat( low ); // float low
	savefile->WriteFloat( high ); // float high
	savefile->WriteFloat( visiblePercent ); // float visiblePercent
	savefile->WriteFloat( thumbWidth ); // float thumbWidth
	savefile->WriteFloat( thumbHeight ); // float thumbHeight
	savefile->WriteFloat( stepSize ); // float stepSize
	savefile->WriteFloat( lastValue ); // float lastValue
	savefile->WriteRect( thumbRect ); // idRectangle thumbRect

	savefile->WriteString( thumbShader ); // idString thumbShader
	// thumbMat = declManager->FindMaterial(thumbShader); // const idMaterial * thumbMat

	savefile->WriteBool( vertical ); // bool vertical
	savefile->WriteBool( verticalFlip ); // bool verticalFlip
	savefile->WriteBool( scrollbar ); // bool scrollbar
	
	//savefile->WriteSGPtr( buddyWin ); // idWindow * buddyWin // blendo eric: set by parent

	cvarStr.WriteToSaveGame( savefile ); // idWinStr cvarStr
	// InitCvar() // idCVar * cvar
	savefile->WriteBool( cvar_init ); // bool cvar_init
	liveUpdate.WriteToSaveGame( savefile ); // idWinBool liveUpdate
	cvarGroup.WriteToSaveGame( savefile ); // idWinStr cvarGroup
	savefile->WriteFloat( modifiedSum ); // float modifiedSum
	savefile->WriteFloat( modifiedMultiplier ); // float modifiedMultiplier
	savefile->WriteBool( initThumbWidth ); // bool initThumbWidth
	savefile->WriteBool( initThumbHeight ); // bool initThumbHeight
	savefile->WriteBool( initThumbColor ); // bool initThumbColor
	savefile->WriteBool( initThumbAuto ); // bool initThumbAuto
	savefile->WriteBool( initHoverColor ); // bool initHoverColor
	savefile->WriteBool( initBarColor ); // bool initBarColor
	savefile->WriteVec4( sliderThumbLockColor ); // idVec4 sliderThumbLockColor
	savefile->WriteVec4( sliderBarLockColor ); // idVec4 sliderBarLockColor

	savefile->WriteVec2( thumbImgSize ); // idVec2 thumbImgSize
	savefile->WriteBool( thumbFillAuto ); // bool thumbFillAuto
	savefile->WriteBool( thumbCapAuto ); // bool thumbCapAuto
	savefile->WriteFloat( thumbCapRescale ); // float thumbCapRescale
	savefile->WriteRect( thumbCapRect ); // idRectangle thumbCapRect
}

void idSliderWindow::ReadFromSaveGame( idRestoreGame *savefile )
{
	idWindow::ReadFromSaveGame( savefile );

	value.ReadFromSaveGame( savefile ); // idWinFloat value
	savefile->ReadFloat( low ); // float low
	savefile->ReadFloat( high ); // float high
	savefile->ReadFloat( visiblePercent ); // float visiblePercent
	savefile->ReadFloat( thumbWidth ); // float thumbWidth
	savefile->ReadFloat( thumbHeight ); // float thumbHeight
	savefile->ReadFloat( stepSize ); // float stepSize
	savefile->ReadFloat( lastValue ); // float lastValue
	savefile->ReadRect( thumbRect ); // idRectangle thumbRect

	savefile->ReadString( thumbShader ); // idString thumbShader
	thumbMat = declManager->FindMaterial(thumbShader); // const idMaterial * thumbMat

	savefile->ReadBool( vertical ); // bool vertical
	savefile->ReadBool( verticalFlip ); // bool verticalFlip
	savefile->ReadBool( scrollbar ); // bool scrollbar

	//savefile->ReadSGPtr( (idSaveGamePtr**)&buddyWin ); // idWindow * buddyWin // blendo eric: set by parent

	cvarStr.ReadFromSaveGame( savefile ); // idWinStr cvarStr
	InitCvar(); // idCVar * cvar
	savefile->ReadBool( cvar_init ); // bool cvar_init
	liveUpdate.ReadFromSaveGame( savefile ); // idWinBool liveUpdate
	cvarGroup.ReadFromSaveGame( savefile ); // idWinStr cvarGroup
	savefile->ReadFloat( modifiedSum ); // float modifiedSum
	savefile->ReadFloat( modifiedMultiplier ); // float modifiedMultiplier
	savefile->ReadBool( initThumbWidth ); // bool initThumbWidth
	savefile->ReadBool( initThumbHeight ); // bool initThumbHeight
	savefile->ReadBool( initThumbColor ); // bool initThumbColor
	savefile->ReadBool( initThumbAuto ); // bool initThumbAuto
	savefile->ReadBool( initHoverColor ); // bool initHoverColor
	savefile->ReadBool( initBarColor ); // bool initBarColor
	savefile->ReadVec4( sliderThumbLockColor ); // idVec4 sliderThumbLockColor
	savefile->ReadVec4( sliderBarLockColor ); // idVec4 sliderBarLockColor

	savefile->ReadVec2( thumbImgSize ); // idVec2 thumbImgSize
	savefile->ReadBool( thumbFillAuto ); // bool thumbFillAuto
	savefile->ReadBool( thumbCapAuto ); // bool thumbCapAuto
	savefile->ReadFloat( thumbCapRescale ); // float thumbCapRescale
	savefile->ReadRect( thumbCapRect ); // idRectangle thumbCapRect
}

bool idSliderWindow::ParseInternalVar(const char *_name, idParser *src) {
	if (idStr::Icmp(_name, "stepsize") == 0 || idStr::Icmp(_name, "step") == 0) {
		stepSize = src->ParseFloat();
		return true;
	}
	if (idStr::Icmp(_name, "low") == 0) {
		low = src->ParseFloat();
		return true;
	}
	if (idStr::Icmp(_name, "high") == 0) {
		high = src->ParseFloat();
		return true;
	}
	if (idStr::Icmp(_name, "vertical") == 0) {
		vertical = src->ParseBool();
		return true;
	}
	if (idStr::Icmp(_name, "verticalflip") == 0) {
		verticalFlip = src->ParseBool();
		return true;
	}
	if (idStr::Icmp(_name, "scrollbar") == 0) {
		scrollbar = src->ParseBool();
		return true;
	}
	if (idStr::Icmp(_name, "thumbshader") == 0) {
		ParseString(src, thumbShader);
		thumbMat = declManager->FindMaterial(thumbShader);
		thumbMat->SetSort( SS_GUI );
		return true;
	}


	//bc
	if (idStr::Icmp(_name, "modifiedSum") == 0)
	{
		modifiedSum = src->ParseFloat();
		return true;
	}

	if (idStr::Icmp(_name, "modifiedMultiplier") == 0)
	{
		modifiedMultiplier = src->ParseFloat();
		return true;
	}

	// blendo eric:
	// skip code initialized definitions when parsed vars are used
	// options can come from both slider gui defs and buddy window gui defs

	if (idStr::Icmp(_name, "thumbsize") == 0
		|| idStr::Icmp(_name, "sliderThumbsize") == 0)
	{
		thumbWidth = src->ParseInt();
		thumbHeight = thumbWidth;
		initThumbHeight = false;
		initThumbWidth = false;
		return true;
	}
	if (idStr::Icmp(_name, "thumbWidth") == 0
		|| idStr::Icmp(_name, "sliderThumbWidth") == 0)
	{
		thumbWidth = src->ParseInt();
		initThumbWidth = false;
		return true;
	}
	if (idStr::Icmp(_name, "thumbHeight") == 0
		|| idStr::Icmp(_name, "sliderThumbHeight") == 0)
	{
		thumbHeight = src->ParseInt();
		initThumbHeight = false;
		return true;
	}

	if ( idStr::Icmp(_name, "thumbFillAuto") == 0
		|| idStr::Icmp(_name, "sliderThumbFillAuto") == 0)
	{
		thumbFillAuto = src->ParseBool();
		initThumbAuto = false;
		return true;
	}
	if ( idStr::Icmp(_name, "thumbCapAuto") == 0
		|| idStr::Icmp(_name, "sliderThumbCapAuto") == 0)
	{
		thumbCapAuto = src->ParseBool();
		initThumbAuto = false;
		return true;
	}

	if( idStr::Icmp(_name, "forecolor") == 0)
	{
		initThumbColor = false;
	}
	else if (idStr::Icmp(_name, "sliderThumbColor") == 0)
	{
		idVec4 outColor;
		ParseVec4(src,outColor);
		foreColor = outColor;
		initThumbColor = false;
		return true;
	}

	if( idStr::Icmp(_name, "hovercolor") == 0)
	{
		initHoverColor = false;
	}
	else if (idStr::Icmp(_name, "sliderHoverColor") == 0)
	{
		idVec4 outColor;
		ParseVec4(src,outColor);
		hoverColor = outColor;
		initHoverColor = false;
		return true;
	}

	if( idStr::Icmp(_name, "matcolor") == 0 || idStr::Icmp(_name, "backcolor") == 0 )
	{
		initHoverColor = false;
	}
	else if (idStr::Icmp(_name, "sliderBarColor") == 0)
	{
		idVec4 outColor;
		ParseVec4(src,outColor);
		matColor = outColor;
		initBarColor = false;
		return true;
	}

	if (idStr::Icmp(_name, "sliderThumbLockColor") == 0)
	{
		ParseVec4(src,sliderThumbLockColor);
		return true;
	}
	if (idStr::Icmp(_name, "sliderBarLockColor") == 0)
	{
		ParseVec4(src,sliderBarLockColor);
		return true;
	}

	return idWindow::ParseInternalVar(_name, src);
}

idWinVar *idSliderWindow::GetWinVarByName(const char *_name, bool fixup, drawWin_t** owner) {

	if (idStr::Icmp(_name, "value") == 0) {
		return &value;
	}
	if (idStr::Icmp(_name, "cvar") == 0) {
		return &cvarStr;
	}
	if ( idStr::Icmp( _name, "liveUpdate" ) == 0 ) {
		return &liveUpdate;
	}
	if ( idStr::Icmp( _name, "cvarGroup" ) == 0 ) {
		return &cvarGroup;
	}

	return idWindow::GetWinVarByName(_name, fixup, owner);
}

const char *idSliderWindow::HandleEvent(const sysEvent_t *event, bool *updateVisuals) {

	if (!(event->evType == SE_KEY && event->evValue2)) {
		return "";
	}
	if( SliderHidden() ){ // blendo eric
		return "";
	}

	int key = event->evValue;

	float origValue = value;

	if ( event->evValue2 && key == K_MOUSE1 )
	{
		SetCapture(this);		
		return RouteMouseCoords(0.0f, 0.0f);
	}

	if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW  /*|| ( key == K_MOUSE2 && gui->CursorY() > thumbRect.y )*/ )  {
		value = value + stepSize;
	}

	if ( key == K_LEFTARROW || key == K_KP_LEFTARROW /*|| ( key == K_MOUSE2 && gui->CursorY() < thumbRect.y )*/ ) {
		value = value - stepSize;
	}

	if (buddyWin) {
		buddyWin->HandleBuddyUpdate(this);
	} else {
		gui->SetStateFloat( cvarStr, value );
		UpdateCvar( false );
	}

	if (origValue != value)
	{
		RunScript(ON_ACTION); //BC
	}

	return origValue != value ? "play click1" : "";
}

void idSliderWindow::SetBuddy(idWindow *buddy) {
	buddyWin = buddy;
}

void idSliderWindow::PostParse() {
	idWindow::PostParse();
	value = 0.0;

	// blendo eric
	SetupThumb(thumbShader);
	hoverColor = initHoverColor ? foreColor : hoverColor;
	 
	//vertical = state.GetBool("vertical");
	//scrollbar = state.GetBool("scrollbar");
	flags |= (WIN_HOLDCAPTURE | WIN_CANFOCUS);
	InitCvar();
}

void idSliderWindow::InitWithDefaults(const char *_name, const idRectangle &_rect, const idVec4 &_foreColor, const idVec4 &_matColor, const char *_background, const char *_thumbShader, bool _vertical,
										bool _scrollbar, bool _thumbAuto) {
	SetInitialState(_name);
	rect = _rect;

	// blendo eric: already parsed vars should be skipped here
	foreColor = initThumbColor ? _foreColor : foreColor;
	matColor = initBarColor ? _matColor : matColor;

	background = declManager->FindMaterial(_background);
	background->SetSort( SS_GUI );
	vertical = _vertical;
	scrollbar = _scrollbar;

	thumbCapAuto = initThumbAuto ? _thumbAuto : thumbCapAuto;
	thumbFillAuto = initThumbAuto ? _thumbAuto : thumbFillAuto;

	SetupThumb(_thumbShader); // blendo eric: setup last

	flags |= WIN_HOLDCAPTURE;
}

void idSliderWindow::SetupThumb(const char *_thumbShader) {
	// recalc all if thumb img or when auto filling / resizing 
	bool recalc = thumbFillAuto;

	// set base texture and recalc sizes
	if( _thumbShader && thumbShader.Icmp(_thumbShader) != 0 ) {
		thumbMat = declManager->FindMaterial(_thumbShader);
		thumbMat->SetSort( SS_GUI );
		thumbShader = _thumbShader;
		thumbImgSize = idVec2( thumbMat->GetImageWidth(), thumbMat->GetImageHeight() );
		thumbImgSize = idVec2( Max( thumbImgSize.x, 1.0f ),  Max( thumbImgSize.y, 1.0f ) ); // avoid div by 0
		recalc = true;
	}

	// set thumb size to default minimum only if init needed

	bool setWidth = thumbWidth < 1.0f || (recalc && initThumbWidth);
	bool setHeight = thumbHeight < 1.0f || (recalc && initThumbHeight);

	if( recalc || setWidth || setHeight ) {

		// calc best thumb size, when uninited

		float thumbMinSide = Min( rect.w(), rect.h() ); // the shorter side of the slider

		idVec2 thumbMinCvar;
		if( vertical ) {
			thumbMinCvar = idVec2( gui_slider_thumbDefWide.GetFloat(), gui_slider_thumbDefLong.GetFloat() );

		} else {
			thumbMinCvar = idVec2( gui_slider_thumbDefLong.GetFloat(), gui_slider_thumbDefWide.GetFloat() );
		}

		// resize thumb to lengthen and shorten depending on the visible percentage of lines in the list
		// keep a bit of the bar visible, at least as big as the short side of the scroll bar (sliderRectMin)
		idVec2 thumbMinFill = idVec2(0.0f,0.0f);
		if( thumbFillAuto ) {
			float fillPercent = visiblePercent * gui_slider_thumbLongMax.GetFloat();
			if( vertical ) {
				thumbMinFill.y = Min( fillPercent * rect.h(), rect.h() - thumbMinSide );
			} else {
				thumbMinFill.x = Min( fillPercent * rect.w(), rect.w() - thumbMinSide );
			}
		}

		// skip if parsed var, otherwise set as default minimum value
		if( setWidth ) {
			float newAutoSize =  Max(Max(Max( thumbImgSize.x, thumbMinSide ), thumbMinCvar.x ), thumbMinFill.x);
			thumbWidth = idMath::Floor( newAutoSize + 0.5f); // round
		}
		if( setHeight ) {
			float newMinSize =  Max(Max(Max( thumbImgSize.y, thumbMinSide ), thumbMinCvar.y ), thumbMinFill.y);
			thumbHeight = idMath::Floor( newMinSize + 0.5f); // round
		}

		assert( thumbImgSize.y > 0.0f && thumbHeight > 0.0f );
		float aspectImg = thumbImgSize.x / thumbImgSize.y;
		float aspectThumb = (thumbWidth / thumbHeight) / aspectImg;

		// correct the aspect ratio, then split in half (half cap at both ends), and truncate value
		if (!vertical) {
			thumbCapRescale = 1.0f/ aspectThumb;
			thumbCapRect.w = idMath::Floor( 0.5f * thumbWidth * thumbCapRescale );
			thumbCapRect.h = thumbHeight;
		} else {
			thumbCapRescale = aspectThumb; // inv horizontal aspect
			thumbCapRect.w = thumbWidth;
			thumbCapRect.h = idMath::Floor( 0.5f * thumbHeight * thumbCapRescale );
		}
	}
}

void idSliderWindow::SetRange(float _low, float _high, float _step) {
	low = _low;
	high = _high;
	stepSize = _step;
}

void idSliderWindow::SetValue(float _value) {
	value = _value;
}

void idSliderWindow::Draw(int time, float x, float y) {
	idVec4 color = foreColor;

	//BC allow no cvar
	//if ( !cvar && !buddyWin ) {
	//	return;
	//}

	if ( SliderHidden() ) { // blendo eric
		return;
	}
	
	UpdateCvar( true );

	if (OverClientRect()) {
		color = hoverColor;
	} else {
		hover = false;
	}
	if ( flags & WIN_CAPTURE ) {
		color = hoverColor;
		hover = true;
	}

	// blendo eric
	if( SliderLocked() ) {
		color = sliderThumbLockColor;
	}

	if ( value > high ) {
		value = high;
	} else if ( value < low ) {
		value = low;
	}

	SetupThumb();

	float range = high - low;

	float thumbPos = (range) ? (value - low) / range : 0.0;
	if (vertical) {
		if ( verticalFlip ) {
			thumbPos = 1.f - thumbPos;
		}
		thumbPos *= drawRect.h - thumbHeight;
		thumbPos += drawRect.y;
		thumbRect.y = thumbPos;
		thumbRect.x = drawRect.x;
	} else {
		thumbPos *= drawRect.w - thumbWidth;
		thumbPos += drawRect.x;
		thumbRect.x = thumbPos;
		thumbRect.y = drawRect.y;
	}
	thumbRect.w = thumbWidth;
	thumbRect.h = thumbHeight;

	bool useEndCaps = thumbCapAuto && thumbCapRescale < 1.0f; // only use caps if they're smaller than stretched thumb bar

	if( !useEndCaps ) {

		// normal thumb draw
		dc->DrawMaterial(thumbRect.x, thumbRect.y, thumbRect.w, thumbRect.h, thumbMat, color);

	} else {
		// draw bar and end caps

		thumbCapRect.x = thumbRect.x;
		thumbCapRect.y = thumbRect.y;

		idRectangle thumbMid = thumbRect;
		idRectangle thumbCapA = thumbCapRect;
		idRectangle thumbCapB = thumbCapRect;

		// shift cap B to the end
		thumbCapB.x = thumbCapRect.x + (thumbRect.w - thumbCapRect.w);
		thumbCapB.y = thumbCapRect.y + (thumbRect.h - thumbCapRect.h);

		idVec2 thumbMidTexScale; // readjust middle thumb texture to fill in space between caps using its middle pixels
		idVec2 thumbCapTexScale; // rescale cap texture to fix half size cap rect

		if( !vertical ) {
			// horizontal slider
			float betweenCapSpace = Max( thumbRect.w - (thumbCapRect.w * 2.0f), 1.0f );
			thumbMidTexScale = idVec2( (2.0f/thumbImgSize.x) / betweenCapSpace , 1.0f );
			thumbCapTexScale = idVec2( 0.5f , 1.0f );

			// shift fill to middle
			thumbMid.x = thumbMid.x + thumbCapA.w;
			thumbMid.w = betweenCapSpace;

		} else {
			// vertical slider
			float betweenCapSpace =  Max( thumbRect.h - (thumbCapRect.h * 2.0f), 1.0f );
			thumbMidTexScale = idVec2( 1.0f , (2.0f/thumbImgSize.y) / betweenCapSpace );
			thumbCapTexScale = idVec2( 1.0f , 0.5f );

			// shift fill to middle
			thumbMid.y = thumbMid.y + thumbCapA.h;
			thumbMid.h = betweenCapSpace;
		}
#if 0
	#define DEBUG_SLIDER_THUMB_CAP_COLOR(_inDbgColor) color = _inDbgColor
#else
	#define DEBUG_SLIDER_THUMB_CAP_COLOR(_inDbgColor)
#endif

		DEBUG_SLIDER_THUMB_CAP_COLOR( idVec4(1.0,1.0,1.0,1.0) );
		idVec2 midTexOffset = idVec2(0.5f,0.5f) - thumbMidTexScale*0.5f; //center
		// draw thumb in middle, but stretch so middle pixels of texture fill it up
		dc->DrawMaterial(thumbMid.x, thumbMid.y, thumbMid.w, thumbMid.h, thumbMat, color, thumbMidTexScale.x, thumbMidTexScale.y, midTexOffset.x, midTexOffset.y);

		DEBUG_SLIDER_THUMB_CAP_COLOR( idVec4(0.0,1.0,0.0,0.5) );
		dc->DrawMaterial(thumbCapA.x, thumbCapA.y, thumbCapA.w, thumbCapA.h, thumbMat, color, thumbCapTexScale.x, thumbCapTexScale.y);
		
		DEBUG_SLIDER_THUMB_CAP_COLOR( idVec4(0.0,0.0,1.0,0.5) );
		dc->DrawMaterial(thumbCapB.x, thumbCapB.y, thumbCapB.w, -thumbCapB.h, thumbMat, color, thumbCapTexScale.x, thumbCapTexScale.y); // flip end cap
	}
}


void idSliderWindow::DrawBackground(const idRectangle &_drawRect) {
	
	//BC allow no cvar.
	//if ( !cvar && !buddyWin ) {
	//	return;
	//}

	// blendo eric
	if( SliderHidden() ) { return; }
	idVec4 origColor = matColor; // save for restore
	if( SliderLocked() ) {
		matColor = sliderBarLockColor;
	}

	idRectangle r = _drawRect;
	if(ThumbHidden()) {
		// blendo eric: let bar take over full length when thumb is hidden
	} else if (!scrollbar) {
		if ( vertical ) {
			r.y += thumbHeight / 2.f;
			r.h -= thumbHeight;
		} else {
			r.x += thumbWidth / 2.0;
			r.w -= thumbWidth;
		}
	} else if ( SliderLocked() ) {
		// blendo eric: prevent bar from drawing over locked thumb with alpha
		if ( vertical ) {
			r.y += thumbHeight;
			r.h -= thumbHeight / 2.0f;
		} else {
			r.x += thumbWidth;
			r.w -= thumbWidth / 2.0f;
		}
	}

	idWindow::DrawBackground(r);

	matColor = origColor;
}

const char *idSliderWindow::RouteMouseCoords(float xd, float yd) {
	float pct;

	if (!(flags & WIN_CAPTURE)) {
		return "";
	}

	if( NoSlide() ) { // blendo eric
		return "";
	}

	float origValue = value;

	idRectangle r = drawRect;
	r.x = actualX;
	r.y = actualY;
	r.x += thumbWidth / 2.0;
	r.w -= thumbWidth;
	if (vertical) {
		r.y += thumbHeight / 2;
		r.h -= thumbHeight;
		if (gui->CursorY() >= r.y && gui->CursorY() <= r.Bottom()) {
			pct = (gui->CursorY() - r.y) / r.h;
			if ( verticalFlip ) {
				pct = 1.f - pct;
			}
			value = low + (high - low) * pct;
		} else if (gui->CursorY() < r.y) {
			if ( verticalFlip ) {
				value = high;
			} else {
				value = low;
			}
		} else {
			if ( verticalFlip ) {
				value = low;
			} else {
				value = high;
			}
		}
	} else {
		r.x += thumbWidth / 2;
		r.w -= thumbWidth;
		if (gui->CursorX() >= r.x && gui->CursorX() <= r.Right())
		{
			float adjustedValue;//bc

			pct = (gui->CursorX() - r.x) / r.w;

			//value = low + (high - low) * pct;

			//BC snap to the closest stepsize.
			adjustedValue = low + (high - low) * pct;
			adjustedValue = (int)(adjustedValue / stepSize);			
			value = adjustedValue * stepSize;

		}
		else if (gui->CursorX() < r.x) {
			value = low;
		} else {
			value = high;
		}
	}

	if (buddyWin) {
		buddyWin->HandleBuddyUpdate(this);
	} else {
		gui->SetStateFloat( cvarStr, value );
	}
	UpdateCvar( false );

	if (origValue != value)
	{
		RunScript(ON_ACTION); //BC
	}

	return origValue != value ? "play click1" : "";
}


void idSliderWindow::Activate(bool activate, idStr &act) {
	idWindow::Activate(activate, act);
	if ( activate ) {
		UpdateCvar( true, true );
	}
}

/*
============
idSliderWindow::InitCvar
============
*/
void idSliderWindow::InitCvar( ) {
	

	//BC Allow slider to NOT reference a cvar.

	if ( cvarStr[0] == '\0' ) {
		//if ( !buddyWin )
		//{
		//	common->Warning( "idSliderWindow::InitCvar: gui '%s' window '%s' has an empty cvar string", gui->GetSourceFile(), name.c_str() );
		//}
		cvar_init = true;
		cvar = NULL;
		return;
	}
	
	cvar = cvarSystem->Find( cvarStr );
	if ( !cvar )
	{
		//common->Warning( "idSliderWindow::InitCvar: gui '%s' window '%s' references undefined cvar '%s'", gui->GetSourceFile(), name.c_str(), cvarStr.c_str() );
		cvar_init = true;
		return;
	}
}

/*
============
idSliderWindow::UpdateCvar
============
*/
void idSliderWindow::UpdateCvar( bool read, bool force ) {

	//BC allow no cvar.
	//if ( buddyWin || !cvar ) {
	//	return;
	//}

	if (cvar)
	{
		if (force || liveUpdate) {
			value = cvar->GetFloat();
			if (value != gui->State().GetFloat(cvarStr)) {
				if (read) {
					gui->SetStateFloat(cvarStr, value);
				}
				else {
					value = gui->State().GetFloat(cvarStr);
					cvar->SetFloat(value);

					//BC TODO: make a click sound play here.
				}
			}
		}
	}

	//BC display slider value. Allow numeric display of slider value.	
	int integerOutput = round((value + modifiedSum) * modifiedMultiplier); //If you don't use round(), it defaults to just rounding down, which looks weird

	gui->SetStateInt(va("%s_value", name.c_str()), integerOutput);							//integer.
	gui->SetStateString(va("%s_valueFloat", name.c_str()), va("%.1f", (value + modifiedSum) * modifiedMultiplier));		//decimal with 1 place.
	gui->SetStateString(va("%s_valueFloat2", name.c_str()), va("%.3f", (value + modifiedSum) * modifiedMultiplier));	//decimal with 3 places.
	gui->SetStateString(va("%s_valueFloat3", name.c_str()), va("%.2f", (value + modifiedSum) * modifiedMultiplier));	//decimal with 2 places.
	gui->SetStateString(va("%s_percent", name.c_str()), idStr::Format("%.0f%%",(value + modifiedSum) * modifiedMultiplier).c_str());//adds percentage sign to value.
}

/*
============
idSliderWindow::RunNamedEvent
============
*/
void idSliderWindow::RunNamedEvent( const char* eventName ) {
	idStr event, group;

	if ( !idStr::Cmpn( eventName, "cvar read ", 10 ) ) {
		event = eventName;
		group = event.Mid( 10, event.Length() - 10 );
		if ( !group.Cmp( cvarGroup ) ) {
			UpdateCvar( true, true );
		}
	} else if ( !idStr::Cmpn( eventName, "cvar write ", 11 ) ) {
		event = eventName;
		group = event.Mid( 11, event.Length() - 11 );
		if ( !group.Cmp( cvarGroup ) ) {
			UpdateCvar( false, true );
		}
	}
}
