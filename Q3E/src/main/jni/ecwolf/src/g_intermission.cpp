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
#include "wl_net.h"
#include "wl_play.h"
#include "thingdef/thingdef.h"

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
static bool exitOnAck;

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
		return IN_UserInput(time, ACK_Any);
	}
	else
	{
		IN_ClearKeysDown ();
		IN_Ack (ACK_Any);
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
				for(unsigned int i = 0;i < Net::InitVars.numPlayers;++i)
					players[i].SetPSprite(NULL, player_t::ps_weapon);
				PreloadGraphics(true);
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
		if(Keyboard[sc_Space] || Keyboard[sc_Escape] || Keyboard[sc_Enter])
		{
			bool done = Keyboard[sc_Escape] || Keyboard[sc_Enter];
			Keyboard[sc_Space] = Keyboard[sc_Escape] = Keyboard[sc_Enter] = false;
			zoomer->Destroy();
			if(done)
				return true;
			break;
		}
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

static void ShowFader(FaderIntermissionAction *fader)
{
	switch(fader->Fade)
	{
		case FaderIntermissionAction::FADEIN:
			ShowImage(fader, true);
			VL_FadeIn(0, 255, fader->Time);
			break;
		case FaderIntermissionAction::FADEOUT:
			// We want to hold whatever may have been drawn in the previous page during the fade, so we don't need to draw.
			VL_FadeOut(0, 255, 0, 0, 0, fader->Time);
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

	VW_UpdateScreen();
	return WaitIntermission(textscreen->Time);
}

bool ShowIntermission(const IntermissionInfo *intermission, bool demoMode)
{
	exitOnAck = demoMode;
	bool gototitle = false;
	bool acked = false;

	// For a cast call we want the bar area to display the names
	if(viewsize > 20)
	{
		const int oldviewsize = viewsize;
		NewViewSize(20);
		viewsize = oldviewsize;
	}

	do
	{
		for(unsigned int i = 0;i < intermission->Actions.Size();++i)
		{
			switch(intermission->Actions[i].type)
			{
				default:
				case IntermissionInfo::IMAGE:
					acked = ShowImage(intermission->Actions[i].action.Get(), false);
					break;
				case IntermissionInfo::CAST:
					acked = gototitle = ShowCast((CastIntermissionAction*)intermission->Actions[i].action.Get());
					break;
				case IntermissionInfo::FADER:
					ShowFader((FaderIntermissionAction*)intermission->Actions[i].action.Get());
					break;
				case IntermissionInfo::GOTOTITLE:
					gototitle = true;
					break;
				case IntermissionInfo::TEXTSCREEN:
					acked = ShowTextScreen((TextScreenIntermissionAction*)intermission->Actions[i].action.Get(), demoMode);
					break;
				case IntermissionInfo::VICTORYSTATS:
					Victory(true);
					break;
			}

			if(demoMode ? acked : gototitle)
				goto EscSequence;
		}
		if(intermission->Link != NAME_None)
			intermission = IntermissionInfo::Find(intermission->Link);
		else
			break;
	}
	while(intermission);
EscSequence:

	// If we changed the view size, we should reset it now.
	if(viewsize > 20)
		NewViewSize(viewsize);

	if(!gototitle && !demoMode)
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

	if(demoMode)
	{
		if(acked)
		{
			VW_FadeOut();
			VL_ReadPalette(gameinfo.GamePalette);
			return false;
		}
		else
			return true;
	}
	else
	{
		VL_ReadPalette(gameinfo.GamePalette);
		return !gototitle;
	}
}
