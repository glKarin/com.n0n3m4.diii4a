/*
** blake_sbar.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
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
#include "a_inventory.h"
#include "a_keys.h"
#include "colormatcher.h"
#include "id_ca.h"
#include "id_us.h"
#include "id_vh.h"
#include "g_mapinfo.h"
#include "v_font.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_def.h"
#include "wl_play.h"
#include "xs_Float.h"
#include "thingdef/thingdef.h"

enum
{
	STATUSLINES = 48,
	STATUSTOPLINES = 16
};

class BlakeStatusBar : public DBaseStatusBar
{
public:
	BlakeStatusBar() : CurrentScore(0) {}

	void DrawStatusBar();
	unsigned int GetHeight(bool top)
	{
		if(viewsize == 21)
			return 0;
		return top ? STATUSTOPLINES : STATUSLINES;
	}

	void NewGame()
	{
		CurrentScore = players[0].score;
	}

	void Tick();

protected:
	void DrawLed(double percent, double x, double y) const;
	void DrawString(FFont *font, const char* string, double x, double y, bool shadow, EColorRange color=CR_UNTRANSLATED, bool center=false) const;

private:
	int CurrentScore;
};

DBaseStatusBar *CreateStatusBar_Blake() { return new BlakeStatusBar(); }

void BlakeStatusBar::DrawLed(double percent, double x, double y) const
{
	static FTextureID LED[2][3] = {
		{TexMan.GetTexture("STLEDDR", FTexture::TEX_Any), TexMan.GetTexture("STLEDDY", FTexture::TEX_Any), TexMan.GetTexture("STLEDDG", FTexture::TEX_Any)},
		{TexMan.GetTexture("STLEDLR", FTexture::TEX_Any), TexMan.GetTexture("STLEDLY", FTexture::TEX_Any), TexMan.GetTexture("STLEDLG", FTexture::TEX_Any)}
	};

	unsigned int which = 0;
	if(percent > 0.71)
		which = 2;
	else if(percent > 0.38)
		which = 1;
	FTexture *dim = TexMan(LED[0][which]);
	FTexture *light = TexMan(LED[1][which]);

	double w = dim->GetScaledWidthDouble();
	double h = dim->GetScaledHeightDouble();

	screen->VirtualToRealCoords(x, y, w, h, 320, 200, true, true);

	int lightclip = xs_ToInt(y + h*(1-percent));
	screen->DrawTexture(dim, x, y,
		DTA_DestWidthF, w,
		DTA_DestHeightF, h,
		DTA_ClipBottom, lightclip,
		TAG_DONE);
	screen->DrawTexture(light, x, y,
		DTA_DestWidthF, w,
		DTA_DestHeightF, h,
		DTA_ClipTop, lightclip,
		TAG_DONE);
}

void BlakeStatusBar::DrawStatusBar()
{
	if(viewsize == 21 && ingame)
		return;

	static FFont *IndexFont = V_GetFont("INDEXFON");
	static FFont *HealthFont = V_GetFont("BlakeHealthFont");
	static FFont *ScoreFont = V_GetFont("BlakeScoreFont");

	static FTextureID STBar = TexMan.GetTexture("STBAR", FTexture::TEX_Any);
	static FTextureID STBarTop = TexMan.GetTexture("STTOP", FTexture::TEX_Any);

	double stx = 0;
	double sty = 200-STATUSLINES;
	double stw = 320;
	double sth = STATUSLINES;
	screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
	int boty = xs_ToInt(sty);

	screen->DrawTexture(TexMan(STBar), stx, sty,
		DTA_DestWidthF, stw,
		DTA_DestHeightF, sth,
		TAG_DONE);

	stx = 0;
	sty = 0;
	stw = 320;
	sth = STATUSTOPLINES;
	screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
	int topy = xs_ToInt(sth);

	screen->DrawTexture(TexMan(STBarTop), stx, 0.0,
		DTA_DestWidthF, stw,
		DTA_DestHeightF, sth,
		TAG_DONE);

	if(viewsize < 20)
	{
		// Draw outset border
		static byte colors[3] =
		{
			ColorMatcher.Pick(RPART(gameinfo.Border.topcolor), GPART(gameinfo.Border.topcolor), BPART(gameinfo.Border.topcolor)),
			ColorMatcher.Pick(RPART(gameinfo.Border.bottomcolor), GPART(gameinfo.Border.bottomcolor), BPART(gameinfo.Border.bottomcolor)),
			ColorMatcher.Pick(RPART(gameinfo.Border.highlightcolor), GPART(gameinfo.Border.highlightcolor), BPART(gameinfo.Border.highlightcolor))
		};

		VWB_Clear(colors[1], 0, topy, screenWidth-scaleFactorX, topy+scaleFactorY);
		VWB_Clear(colors[1], 0, topy+scaleFactorY, scaleFactorX, boty);
		VWB_Clear(colors[0], scaleFactorX, boty-scaleFactorY, screenWidth, boty);
		VWB_Clear(colors[0], screenWidth-scaleFactorX, topy, screenWidth, static_cast<int>(boty-scaleFactorY));
	}

	// Draw the top information
	FString lives, area;
	// TODO: Don't depend on LevelNumber for this switch
	if(levelInfo->LevelNumber > 20)
		area = "SECRET";
	else
		area.Format("AREA: %d", levelInfo->LevelNumber);
	lives.Format("LIVES: %d", players[0].lives);
	DrawString(IndexFont, area, 18, 5, true, CR_WHITE);
	DrawString(IndexFont, levelInfo->GetName(map), 160, 5, true, CR_WHITE, true);
	DrawString(IndexFont, lives, 267, 5, true, CR_WHITE);

	// Draw bottom information
	FString health;
	health.Format("%3d", players[0].health);
	DrawString(HealthFont, health, 128, 162, false);

	FString score;
	score.Format("%7d", CurrentScore);
	DrawString(ScoreFont, score, 256, 155, false);

	if(players[0].ReadyWeapon)
	{
		FTexture *weapon = TexMan(players[0].ReadyWeapon->icon);
		if(weapon)
		{
			stx = 248;
			sty = 176;
			stw = weapon->GetScaledWidthDouble();
			sth = weapon->GetScaledHeightDouble();
			screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
			screen->DrawTexture(weapon, stx, sty,
				DTA_DestWidthF, stw,
				DTA_DestHeightF, sth,
				TAG_DONE);
		}

		// TODO: Fix color
		unsigned int amount = players[0].ReadyWeapon->ammo[AWeapon::PrimaryFire]->amount;
		DrawLed(static_cast<double>(amount)/static_cast<double>(players[0].ReadyWeapon->ammo[AWeapon::PrimaryFire]->maxamount), 243, 155);

		FString ammo;
		ammo.Format("%3d%%", amount);
		DrawString(IndexFont, ammo, 252, 190, false, CR_LIGHTBLUE);
	}

	if(players[0].mo)
	{
		static const ClassDef * const radarPackCls = ClassDef::FindClass("RadarPack");
		AInventory *radarPack = players[0].mo->FindInventory(radarPackCls);
		if(radarPack)
			DrawLed(static_cast<double>(radarPack->amount)/static_cast<double>(radarPack->maxamount), 235, 155);
		else
			DrawLed(0, 235, 155);
	}

	// Find keys in inventory
	int presentKeys = 0;
	if(players[0].mo)
	{
		for(AInventory *item = players[0].mo->inventory;item != NULL;item = item->inventory)
		{
			if(item->IsKindOf(NATIVE_CLASS(Key)))
			{
				int slot = static_cast<AKey *>(item)->KeyNumber;
				if(slot <= 3)
					presentKeys |= 1<<(slot-1);
				if(presentKeys == 0x7)
					break;
			}
		}
	}

	static FTextureID Keys[4] = {
		TexMan.GetTexture("STKEYS0", FTexture::TEX_Any),
		TexMan.GetTexture("STKEYS1", FTexture::TEX_Any),
		TexMan.GetTexture("STKEYS2", FTexture::TEX_Any),
		TexMan.GetTexture("STKEYS3", FTexture::TEX_Any)
	};
	for(unsigned int i = 0;i < 3;++i)
	{
		FTexture *tex;
		if(presentKeys & (1<<i))
			tex = TexMan(Keys[i+1]);
		else
			tex = TexMan(Keys[0]);

		stx = 120+16*i;
		sty = 179;
		stw = tex->GetScaledWidthDouble();
		sth = tex->GetScaledHeightDouble();
		screen->VirtualToRealCoords(stx, sty, stw, sth, 320, 200, true, true);
		screen->DrawTexture(tex, stx, sty,
			DTA_DestWidthF, stw,
			DTA_DestHeightF, sth,
			TAG_DONE);
	}
}

void BlakeStatusBar::DrawString(FFont *font, const char* string, double x, double y, bool shadow, EColorRange color, bool center) const
{
	word strWidth, strHeight;
	VW_MeasurePropString(font, string, strWidth, strHeight);

	if(center)
		x -= strWidth/2.0;

	const double startX = x;
	FRemapTable *remap = font->GetColorTranslation(color);

	while(*string != '\0')
	{
		char ch = *string++;
		if(ch == '\n')
		{
			y += font->GetHeight();
			x = startX;
			continue;
		}

		int chWidth;
		FTexture *tex = font->GetChar(ch, &chWidth);
		if(tex)
		{
			double tx, ty, tw, th;

			if(shadow)
			{
				tx = x + 1, ty = y + 1, tw = tex->GetScaledWidthDouble(), th = tex->GetScaledHeightDouble();
				screen->VirtualToRealCoords(tx, ty, tw, th, 320, 200, true, true);
				screen->DrawTexture(tex, tx, ty,
					DTA_DestWidthF, tw,
					DTA_DestHeightF, th,
					DTA_FillColor, GPalette.BlackIndex,
					TAG_DONE);
			}

			tx = x, ty = y, tw = tex->GetScaledWidthDouble(), th = tex->GetScaledHeightDouble();
			screen->VirtualToRealCoords(tx, ty, tw, th, 320, 200, true, true);
			screen->DrawTexture(tex, tx, ty,
				DTA_DestWidthF, tw,
				DTA_DestHeightF, th,
				DTA_Translation, remap,
				TAG_DONE);
		}
		x += chWidth;
	}
}

void BlakeStatusBar::Tick()
{
	int scoreDelta = players[0].score - CurrentScore;
	if(scoreDelta > 1500)
		CurrentScore += scoreDelta/4;
	else
		CurrentScore += clamp<int>(scoreDelta, 0, 8);
}
