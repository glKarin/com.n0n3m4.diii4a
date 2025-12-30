/*
** g_intermission.cpp
**
**---------------------------------------------------------------------------
** Copyright 2012 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "wl_def.h"
#include "id_ca.h"
#include "id_in.h"
#include "id_sd.h"
#include "id_vh.h"
#include "g_intermission.h"
#include "language.h"
#include "r_sprites.h"
#include "tarray.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_inter.h"
#include "wl_menu.h"
#include "wl_play.h"
#include "thingdef/thingdef.h"
#include "state_machine.h"

static TMap<FName, IntermissionInfo> intermissions;

IntermissionInfo::~IntermissionInfo()
{
}

IntermissionInfo *IntermissionInfo::Find(const FName &name)
{
	return &intermissions[name];
}

void IntermissionInfo::Clear()
{
	Actions.Clear();
}

////////////////////////////////////////////////////////////////////////////////

void StartTravel ();
void FinishTravel ();

static bool intermissionMapLoaded = false;

static void ClearStatusbar()
{
	DrawPlayScreen();

	FTexture *borderTex = TexMan(levelInfo->GetBorderTexture());
	VWB_DrawFill(borderTex, statusbarx, statusbary2+CleanYfac, screenWidth-statusbarx, screenHeight);
}

static bool WaitIntermission(unsigned int time)
{
	if(time)
	{
		return IN_UserInput(time);
	}
	else
	{
		IN_ClearKeysDown ();
		IN_Ack ();
		return true;
	}
}

static bool ShowImage(IntermissionAction *image, bool drawonly)
{
	if(!image->Music.IsEmpty())
		StartCPMusic(image->Music);

	if(!image->Palette.IsEmpty())
	{
		if(image->Palette.CompareNoCase("$GamePalette") == 0)
			VL_ReadPalette(gameinfo.GamePalette);
		else
			VL_ReadPalette(image->Palette);
	}

	static FTextureID background;
	static bool tileBackground = false;
	static IntermissionAction::BackgroundType type = IntermissionAction::NORMAL;

	// High Scores and such need special handling
	if(image->Type != IntermissionAction::UNSET)
	{
		type = image->Type;
	}
	if(type == IntermissionAction::NORMAL && image->Background.isValid())
	{
		background = image->Background;
		tileBackground = image->BackgroundTile;
	}

	intermissionMapLoaded = false;
	switch(type)
	{
		default:
			if(!tileBackground)
				CA_CacheScreen(TexMan(background));
			else
				VWB_DrawFill(TexMan(background), 0, 0, screenWidth, screenHeight);
			break;
		case IntermissionAction::HIGHSCORES:
			DrawHighScores();
			break;
		case IntermissionAction::TITLEPAGE:
			background = TexMan.CheckForTexture(gameinfo.TitlePage, FTexture::TEX_Any);
			if(!gameinfo.TitlePalette.IsEmpty())
				VL_ReadPalette(gameinfo.TitlePalette);
			CA_CacheScreen(TexMan(background));
			break;
		case IntermissionAction::LOADMAP:
			if(image->MapName.IsNotEmpty())
			{
				strncpy(gamestate.mapname, image->MapName, 8);
				StartTravel();
				SetupGameLevel();
				FinishTravel();
				AActor::FinishSpawningActors();
				// Drop weapon
				players[0].SetPSprite(NULL, player_t::ps_weapon);
				PreloadGraphics(false);
				gamestate.victoryflag = true;
			}
			intermissionMapLoaded = true;
			ThreeDRefresh();
			ClearStatusbar();
			break;
	}

	for(unsigned int i = 0;i < image->Draw.Size();++i)
	{
		VWB_DrawGraphic(TexMan(image->Draw[i].Image), image->Draw[i].X, image->Draw[i].Y);
	}

	if(!drawonly)
	{
		VW_UpdateScreen();
		return WaitIntermission(image->Time);
	}
	return false;
}


static void DrawCastName(CastIntermissionAction *cast)
{
	int width = BigFont->StringWidth(cast->Name);
	screen->DrawText(BigFont, gameinfo.FontColors[GameInfo::DIALOG],
		(screenWidth - width*CleanXfac)/2, statusbary2 + (screenHeight - statusbary2 - BigFont->GetHeight())/2,
		cast->Name,
		DTA_CleanNoMove, true,
		TAG_DONE
	);
}
static bool R_CastZoomer(const Frame *frame, CastIntermissionAction *cast)
{
	// This may appear to animate faster than vanilla, but I'm fairly sure
	// that's because while the time on screen is adaptive, the frame durations
	// were decremented by one each frame.
	TObjPtr<SpriteZoomer> zoomer = new SpriteZoomer(frame, 224);
	do
	{
		for(unsigned int t = tics;zoomer && t-- > 0;)
			zoomer->Tick();
		if(!zoomer)
			break;

		if(intermissionMapLoaded)
			ThreeDRefresh();
		else
		{
			// Unlike a 3D view, we will overwrite the whole screen here
			ShowImage(cast, true);
			DrawCastName(cast);
		}
		zoomer->Draw();
		VH_UpdateScreen();
		IN_ProcessEvents();
#ifdef TODO
		if(Keyboard[sc_Space] || Keyboard[sc_Escape] || Keyboard[sc_Enter])
		{
			bool done = Keyboard[sc_Escape] || Keyboard[sc_Enter];
			Keyboard[sc_Space] = Keyboard[sc_Escape] = Keyboard[sc_Enter] = false;
			zoomer->Destroy();
			if(done)
				return true;
			break;
		}
#endif
		CalcTics();
	}
	while(true);
	return false;
}
static bool ShowCast(CastIntermissionAction *cast)
{
	ClearStatusbar();
	DrawCastName(cast);

	SD_PlaySound(cast->Class->GetDefault()->seesound);
	const Frame *frame = cast->Class->FindState(NAME_See);
	return R_CastZoomer(frame, cast);
}

static void ShowFader(struct IntermissionGState *state, FaderIntermissionAction *fader)
{
	switch(fader->Fade)
	{
		case FaderIntermissionAction::FADEIN:
			ShowImage(fader, true);
			state->image_ready = true;
			state->fade_in = true;
			state->fade_steps = fader->Time;
			break;
		case FaderIntermissionAction::FADEOUT:
			// We want to hold whatever may have been drawn in the previous page during the fade, so we don't need to draw.
			state->fade_steps = fader->Time;
			state->fade_out = true;
			break;
	}
}

static bool ShowTextScreen(TextScreenIntermissionAction *textscreen, bool demoMode)
{
	if(textscreen->TextSpeed)
		Printf("Warning: Text screen has a non-zero textspeed which isn't supported at this time.\n");

	ShowImage(textscreen, true);

	if(textscreen->TextDelay)
	{
		if(WaitIntermission(textscreen->TextDelay) && demoMode)
			return true;
	}

	py = textscreen->PrintY;
	for(unsigned int i = 0;i < textscreen->Text.Size();++i)
	{
		px = textscreen->PrintX;

		FString str = textscreen->Text[i];
		if(str[0] == '$')
			str = language[str.Mid(1)];

		DrawMultiLineText(str, textscreen->TextFont, textscreen->TextColor, textscreen->Alignment, textscreen->Anchor);
	}

	// This really only makes sense to use if trying to display text immediately.
	if(textscreen->FadeTime)
	{
		VL_FadeIn(0, 255, textscreen->FadeTime);
	}

	return false;
}

void StateWaitIntermission(struct IntermissionGState *state, int time)
{
	state->wait = time;
}

void AdvanceIntermission(wl_state_t *wlstate, struct IntermissionGState *state)
{
	state->image_ready = false;
	state->wait = -1;
	
	if(state->gototitle)
		state->finishing = true;

	if (state->finishing) {
			// If we changed the view size, we should reset it now.
		if(viewsize > 20)
			NewViewSize(viewsize);
#ifdef TODO
		if(!state->gototitle && !state->demoMode)
		{
			// Hold at the final page until esc is pressed
			IN_ClearKeysDown();

			ControlInfo ci;
			do
			{
				LastScan = 0;
				ReadAnyControl(&ci);
			}
			while(LastScan != sc_Escape && LastScan != sc_Enter && !ci.button1);
		}
#endif

		VL_ReadPalette(gameinfo.GamePalette);
		state->ret_value = !state->gototitle;

		state->finished = true;
		return;
	}
	if (state->step < 0) {		
		// For a cast call we want the bar area to display the names
		if(viewsize > 20)
		{
			const int oldviewsize = viewsize;
			NewViewSize(20);
			viewsize = oldviewsize;
		}
		state->step = 0;
		return;
	}
	if (state->step >= (int) state->intermission->Actions.Size()) {
		if(state->intermission->Link != NAME_None) {
			state->intermission_name = state->intermission->Link;
			state->intermission = IntermissionInfo::Find(state->intermission->Link);
			state->step = 0;
			state->finishing = state->intermission != NULL;
		} else {
			state->finishing = true;
		}
		return;
	}

	int i = state->step;
	state->step++;

	switch(state->intermission->Actions[i].type)
	{
	default:
	case IntermissionInfo::IMAGE:
	{
		IntermissionAction *image = state->intermission->Actions[i].action.Get();
		ShowImage(image, true);
		state->image_ready = true;
		StateWaitIntermission(state, image->Time);
	}
		break;
	case IntermissionInfo::CAST:
		state->gototitle = ShowCast((CastIntermissionAction*)state->intermission->Actions[i].action.Get());
		break;
	case IntermissionInfo::FADER:
		ShowFader(state, (FaderIntermissionAction*)state->intermission->Actions[i].action.Get());
		break;
	case IntermissionInfo::GOTOTITLE:
		state->gototitle = true;
		break;
	case IntermissionInfo::TEXTSCREEN:
	{
		TextScreenIntermissionAction *textscreen = (TextScreenIntermissionAction*)state->intermission->Actions[i].action.Get();
		ShowTextScreen(textscreen, state->demoMode);
		state->image_ready = true;
		StateWaitIntermission(state, textscreen->Time);
		break;
	}
	case IntermissionInfo::VICTORYSTATS:
		DrawVictory(true);
		State_FadeIn (wlstate);
		State_Ack(wlstate);
#ifdef TODO
		EndText (levelInfo->Cluster);
		VW_FadeOut();
#endif
		break;
	}

	if(state->gototitle)
		state->finishing = true;
}

void InitIntermission(struct IntermissionGState *state, const FName &intermission, bool demoMode)
{
	state->step = -1;
	state->demoMode = demoMode;
	state->intermission_name = intermission;
	state->intermission = IntermissionInfo::Find(intermission);
	state->gototitle = false;
	state->finishing = false;
	state->finished = false;
	state->ret_value = false;
	state->image_ready = false;
	state->wait = -1;
}
