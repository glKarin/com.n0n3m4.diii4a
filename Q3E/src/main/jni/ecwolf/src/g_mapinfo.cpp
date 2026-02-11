/*
** g_mapinfo.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#include "gamemap.h"
#include "g_intermission.h"
#include "g_mapinfo.h"
#include "language.h"
#include "lnspec.h"
#include "tarray.h"
#include "scanner.h"
#include "w_wad.h"
#include "wl_iwad.h"
#include "wl_shade.h"
#include "v_video.h"
#include "g_shared/a_inventory.h"
#include "g_shared/a_playerpawn.h"
#include "thingdef/thingdef.h"

////////////////////////////////////////////////////////////////////////////////
// MapInfoBlockParser
//
// Base class to handle syntax of mapinfo lump for common types.

class MapInfoBlockParser
{
public:
	MapInfoBlockParser(Scanner &sc, const char* block) : sc(sc), block(block)
	{
	}

	virtual ~MapInfoBlockParser() {}

	void Parse()
	{
		ParseHeader();
		ParseBlock(block);
	}

protected:
	virtual bool CheckKey(FString key)=0;
	virtual void ParseHeader() {};

	void ParseBoolAssignment(bool &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_BoolConst);
		dest = sc->boolean;
	}

	void ParseColorAssignment(int &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		dest = V_GetColorFromString(NULL, sc->str);
	}

	void ParseColorArrayAssignment(TArray<int> &dest)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			dest.Push(V_GetColorFromString(NULL, sc->str));
		}
		while(sc.CheckToken(','));
	}

	void ParseColorArrayAssignment(int *dest, unsigned int max)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			*dest++ = V_GetColorFromString(NULL, sc->str);

			if(--max == 0)
				break;
		}
		while(sc.CheckToken(','));
	}

	void ParseFixedAssignment(fixed &dest)
	{
		sc.MustGetToken('=');
		bool negative = sc.CheckToken('-');
		sc.MustGetToken(TK_FloatConst);
		dest = negative ? -FLOAT2FIXED(sc->decimal) : FLOAT2FIXED(sc->decimal);
	}

	void ParseIntAssignment(int &dest)
	{
		sc.MustGetToken('=');
		bool negative = sc.CheckToken('-');
		sc.MustGetToken(TK_IntConst);
		dest = negative ? -sc->number : sc->number;
	}

	void ParseIntAssignment(unsigned int &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_IntConst);
		dest = sc->number;
	}

	void ParseTicAssignment(unsigned int &tics)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_FloatConst);
		if(!CheckTicsValid(sc->decimal))
			sc.ScriptMessage(Scanner::ERROR, "Invalid tic duration.");

		tics = static_cast<unsigned int>(sc->decimal*2);
	}

	void ParseIntArrayAssignment(TArray<int> &dest)
	{
		sc.MustGetToken('=');
		do
		{
			bool negative = sc.CheckToken('-');
			sc.MustGetToken(TK_IntConst);
			dest.Push(negative ? -sc->number : sc->number);
		}
		while(sc.CheckToken(','));
	}

	void ParseIntArrayAssignment(TArray<unsigned int> &dest)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_IntConst);
			dest.Push(sc->number);
		}
		while(sc.CheckToken(','));
	}

	void ParseIntArrayAssignment(int *dest, unsigned int max)
	{
		sc.MustGetToken('=');
		do
		{
			bool negative = sc.CheckToken('-');
			sc.MustGetToken(TK_IntConst);
			*dest++ = negative ? -sc->number : sc->number;

			if(--max == 0)
				break;
		}
		while(sc.CheckToken(','));
	}

	void ParseIntArrayAssignment(unsigned int *dest, unsigned int max)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_IntConst);
			*dest++ = sc->number;

			if(--max == 0)
				break;
		}
		while(sc.CheckToken(','));
	}

	void ParseStringAssignment(FString &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		dest = sc->str;
	}

	void ParseNameAssignment(FName &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		dest = sc->str;
	}

	void ParseStringArrayAssignment(TArray<FString> &dest)
	{
		dest.Clear();
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			dest.Push(sc->str);
		}
		while(sc.CheckToken(','));
	}

	void ParseStringArrayAssignment(FString *dest, unsigned int max)
	{
		sc.MustGetToken('=');
		do
		{
			sc.MustGetToken(TK_StringConst);
			*dest++ = sc->str;

			if(--max == 0)
				break;
		}
		while(sc.CheckToken(','));
	}

	void ParseFontColorAssignment(EColorRange &dest)
	{
		sc.MustGetToken('=');
		sc.MustGetToken(TK_StringConst);
		dest = V_FindFontColor(sc->str);
	}

	Scanner &sc;
	const char* const block;

private:
	void ParseBlock(const char* block)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			if(!CheckKey(sc->str))
			{
				sc.ScriptMessage(Scanner::WARNING, "Unknown %s property '%s'.", block, sc->str.GetChars());
				if(sc.CheckToken('='))
				{
					do
					{
						sc.GetNextToken();
					}
					while(sc.CheckToken(','));
				}
			}
		}
	}
};

////////////////////////////////////////////////////////////////////////////////

static LevelInfo defaultMap;
static TArray<LevelInfo> levelInfos;

LevelInfo::LevelInfo() : ResetHealth(false), ResetInventory(false),
	UseMapInfoName(false)
{
	MapName[0] = 0;
	TitlePatch.SetInvalid();
	BorderTexture.SetInvalid();
	DefaultTexture[0].SetInvalid();
	DefaultTexture[1].SetInvalid();
	DefaultLighting = LIGHTLEVEL_DEFAULT;
	DefaultVisibility = VISIBILITY_DEFAULT;
	DefaultMaxLightVis = MAXLIGHTVIS_DEFAULT;
	DeathCam = false;
	ExitFadeColor = 0;
	ExitFadeDuration = 30;
	FloorNumber = "1";
	Par = 0;
	LevelBonus = -1;
	LevelNumber = 0;
	Cluster = 0;
	NoIntermission = false;
	SecretDeathSounds = false;
	SpawnWithWeaponRaised = false;
	ForceTally = false;
	HighScoresGraphic.SetInvalid();
	Sky.SetInvalid();
	SkyScrollSpeed = 0.0;
	SkyHorizonOffset = 0;
}

FTextureID LevelInfo::GetBorderTexture() const
{
	static FTextureID BorderFlat = TexMan.GetTexture(gameinfo.BorderFlat, FTexture::TEX_Flat);
	if(!BorderTexture.isValid())
		return BorderFlat;
	return BorderTexture;
}

FString LevelInfo::GetMusic(const GameMap *gm) const
{
	if(gm->GetHeader().music.IsNotEmpty())
		return gm->GetHeader().music;
	return Music;
}

FString LevelInfo::GetName(const GameMap *gm) const
{
	if(UseMapInfoName)
		return Name;
	return gm->GetHeader().name;
}

LevelInfo &LevelInfo::Find(const char* level)
{
	for(unsigned int i = 0;i < levelInfos.Size();++i)
	{
		if(stricmp(levelInfos[i].MapName, level) == 0)
			return levelInfos[i];
	}
	return defaultMap;
}

LevelInfo &LevelInfo::FindByNumber(unsigned int num)
{
	for(unsigned int i = 0;i < levelInfos.Size();++i)
	{
		if(levelInfos[i].LevelNumber == num)
			return levelInfos[i];
	}
	return defaultMap;
}

class LevelInfoBlockParser : public MapInfoBlockParser
{
private:
	bool parseHeader;
	LevelInfo &mapInfo;

public:
	LevelInfoBlockParser(Scanner &sc, LevelInfo &mapInfo, bool parseHeader) :
		MapInfoBlockParser(sc, "map"), parseHeader(parseHeader), mapInfo(mapInfo) {}

protected:
	void ParseHeader()
	{
		if(!parseHeader)
			return;

		sc.MustGetToken(TK_StringConst);
		strncpy(mapInfo.MapName, sc->str, 8);
		mapInfo.MapName[8] = 0;

		if(strnicmp(mapInfo.MapName, "MAP", 3) == 0)
		{
			int num = atoi(mapInfo.MapName+3);
			if(num > 0) // Zero is the default so no need to do anything.
			{
				ClearLevelNumber(num);
				mapInfo.LevelNumber = num;
			}
		}

		bool useLanguage = false;
		if(sc.CheckToken(TK_Identifier))
		{
			if(sc->str.CompareNoCase("lookup") != 0)
				sc.ScriptMessage(Scanner::ERROR, "Expected lookup keyword but got '%s' instead.", sc->str.GetChars());
			else
				useLanguage = true;
		}
		if(sc.CheckToken(TK_StringConst))
		{
			mapInfo.UseMapInfoName = true;
			if(useLanguage)
				mapInfo.Name = language[sc->str];
			else
				mapInfo.Name = sc->str;
		}
	}

	void ClearLevelNumber(unsigned int num)
	{
		LevelInfo &other = LevelInfo::FindByNumber(num);
		if(other.MapName[0] != 0)
			other.LevelNumber = 0;
	}

	void ParseNext(FString &next)
	{
		sc.MustGetToken('=');
		if(sc.CheckToken(TK_Identifier))
		{
			if(sc->str.CompareNoCase("EndSequence") == 0)
			{
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				next.Format("EndSequence:%s", sc->str.GetChars());
			}
			else
				sc.ScriptMessage(Scanner::ERROR, "Expected EndSequence.");
		}
		else
		{
			sc.MustGetToken(TK_StringConst);
			next = sc->str;
		}
	}

	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("next") == 0)
			ParseNext(mapInfo.NextMap);
		else if(key.CompareNoCase("secretnext") == 0)
			ParseNext(mapInfo.NextSecret);
		else if(key.CompareNoCase("victorynext") == 0)
			ParseNext(mapInfo.NextVictory);
		else if(key.CompareNoCase("bordertexture") == 0)
		{
			FString textureName;
			ParseStringAssignment(textureName);
			mapInfo.BorderTexture = TexMan.GetTexture(textureName, FTexture::TEX_Flat);
		}
		else if(key.CompareNoCase("defaultfloor") == 0)
		{
			FString textureName;
			ParseStringAssignment(textureName);
			mapInfo.DefaultTexture[MapSector::Floor] = TexMan.GetTexture(textureName, FTexture::TEX_Flat);
		}
		else if(key.CompareNoCase("defaultceiling") == 0)
		{
			FString textureName;
			ParseStringAssignment(textureName);
			mapInfo.DefaultTexture[MapSector::Ceiling] = TexMan.GetTexture(textureName, FTexture::TEX_Flat);
		}
		else if(key.CompareNoCase("DefaultLighting") == 0)
			ParseIntAssignment(mapInfo.DefaultLighting);
		else if(key.CompareNoCase("DefaultVisibility") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_FloatConst);
			mapInfo.DefaultVisibility = static_cast<fixed>(sc->decimal*LIGHTVISIBILITY_FACTOR*65536.);
		}
		else if(key.CompareNoCase("DefaultMaxLightVis") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_FloatConst);
			mapInfo.DefaultMaxLightVis = static_cast<fixed>(sc->decimal*LIGHTVISIBILITY_FACTOR*65536.);
		}
		else if(key.CompareNoCase("SpecialAction") == 0)
		{
			LevelInfo::SpecialAction action;
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			action.Class = ClassDef::FindClassTentative(sc->str, NATIVE_CLASS(Actor));
			sc.MustGetToken(',');
			sc.MustGetToken(TK_StringConst);
			action.Special = Specials::LookupFunctionNum(sc->str);

			unsigned int i;
			for(i = 0;i < 5 && sc.CheckToken(',');++i)
			{
				sc.MustGetToken(TK_IntConst);
				action.Args[i] = sc->number;
			}
			while(i < 5)
				action.Args[i++] = 0;

			mapInfo.SpecialActions.Push(action);
		}
		else if(key.CompareNoCase("Cluster") == 0)
			ParseIntAssignment(mapInfo.Cluster);
		else if(key.CompareNoCase("CompletionString") == 0)
			ParseStringAssignment(mapInfo.CompletionString);
		else if(key.CompareNoCase("EnsureInventory") == 0)
		{
			TArray<FString> classNames;
			ParseStringArrayAssignment(classNames);

			for(unsigned int i = 0;i < classNames.Size();++i)
			{
				const ClassDef *cls = ClassDef::FindClass(classNames[i]);
				if(!cls || !cls->IsDescendantOf(NATIVE_CLASS(Inventory)))
					sc.ScriptMessage(Scanner::ERROR, "Class %s doesn't appear to be a kind of Inventory.", classNames[i].GetChars());
				mapInfo.EnsureInventory.Push(cls);
			}
		}
		else if(key.CompareNoCase("ExitFade") == 0)
		{
			ParseColorAssignment(mapInfo.ExitFadeColor);
			sc.MustGetToken(',');
			sc.MustGetToken(TK_FloatConst);
			if(!CheckTicsValid(sc->decimal))
				sc.ScriptMessage(Scanner::ERROR, "Invalid tic duration.");
			mapInfo.ExitFadeDuration = static_cast<unsigned int>(sc->decimal*2);
		}
		else if(key.CompareNoCase("DeathCam") == 0)
			ParseBoolAssignment(mapInfo.DeathCam);
		else if(key.CompareNoCase("FloorNumber") == 0)
		{
			sc.MustGetToken('=');
			if(sc.CheckToken(TK_StringConst))
				mapInfo.FloorNumber = sc->str;
			else
			{
				sc.MustGetToken(TK_IntConst);
				mapInfo.FloorNumber.Format("%d", sc->number);
			}
		}
		else if(key.CompareNoCase("ForceTally") == 0)
			ParseBoolAssignment(mapInfo.ForceTally);
		else if(key.CompareNoCase("HighScoresGraphic") == 0)
		{
			FString texName;
			ParseStringAssignment(texName);
			mapInfo.HighScoresGraphic = TexMan.CheckForTexture(texName, FTexture::TEX_Any);
		}
		else if(key.CompareNoCase("LevelBonus") == 0)
			ParseIntAssignment(mapInfo.LevelBonus);
		else if(key.CompareNoCase("LevelNum") == 0)
		{
			// Get the number and then remove any other map from this slot.
			unsigned int num;
			ParseIntAssignment(num);
			ClearLevelNumber(num);
			mapInfo.LevelNumber = num;
		}
		else if(key.CompareNoCase("Music") == 0)
			ParseStringAssignment(mapInfo.Music);
		else if(key.CompareNoCase("NoIntermission") == 0)
			mapInfo.NoIntermission = true;
		else if(key.CompareNoCase("Par") == 0)
			ParseIntAssignment(mapInfo.Par);
		else if(key.CompareNoCase("ResetHealth") == 0)
			mapInfo.ResetHealth = true;
		else if(key.CompareNoCase("ResetInventory") == 0)
			mapInfo.ResetInventory = true;
		else if(key.CompareNoCase("SecretDeathSounds") == 0)
			ParseBoolAssignment(mapInfo.SecretDeathSounds);
		else if(key.CompareNoCase("SpawnWithWeaponRaised") == 0)
			mapInfo.SpawnWithWeaponRaised = true;
		else if(key.CompareNoCase("TitlePatch") == 0)
		{
			FString textureName;
			ParseStringAssignment(textureName);
			mapInfo.TitlePatch = TexMan.GetTexture(textureName, FTexture::TEX_Any);
		}
		else if(key.CompareNoCase("Translator") == 0)
			ParseStringAssignment(mapInfo.Translator);
		else if(key.CompareNoCase("Sky1") == 0)
		{
			FString texName;
			ParseStringAssignment(texName);
			mapInfo.Sky = TexMan.CheckForTexture(texName, FTexture::TEX_Wall);

			if(sc.CheckToken(','))
			{
				bool negative = sc.CheckToken('-');
				sc.MustGetToken(TK_FloatConst);
				mapInfo.SkyScrollSpeed = negative ? -sc->decimal : sc->decimal;

				if(sc.CheckToken(','))
				{
					negative = sc.CheckToken('-');
					sc.MustGetToken(TK_IntConst);
					mapInfo.SkyHorizonOffset = negative ? -sc->number : sc->number;
				}
			}
		}
		else
			return false;
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

GameInfo gameinfo;

GameInfo::GameInfo() : PageIndexText("pg %d of %d")
{
}

class GameInfoBlockParser : public MapInfoBlockParser
{
public:
	GameInfoBlockParser(Scanner &sc) : MapInfoBlockParser(sc, "gameinfo") {}

protected:
	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("advisorycolor") == 0)
			ParseColorAssignment(gameinfo.AdvisoryColor);
		else if(key.CompareNoCase("advisorypic") == 0)
			ParseStringAssignment(gameinfo.AdvisoryPic);
		else if(key.CompareNoCase("border") == 0)
		{
			sc.MustGetToken('=');
			if(sc.CheckToken(TK_Identifier))
			{
				gameinfo.Border.issolid = true;
				if(sc->str.CompareNoCase("inset") != 0)
					sc.ScriptMessage(Scanner::ERROR, "Expected 'inset' got '%s' instead.", sc->str.GetChars());
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.topcolor = V_GetColorFromString(NULL, sc->str);
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.bottomcolor = V_GetColorFromString(NULL, sc->str);
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.highlightcolor = V_GetColorFromString(NULL, sc->str);
			}
			else
			{
				gameinfo.Border.issolid = false;
				sc.MustGetToken(TK_IntConst);
				gameinfo.Border.offset = sc->number;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_IntConst); // Unused
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.tl = sc->str;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.t = sc->str;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.tr = sc->str;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.l = sc->str;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.r = sc->str;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.bl = sc->str;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.b = sc->str;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_StringConst);
				gameinfo.Border.br = sc->str;
			}
		}
		else if(key.CompareNoCase("borderflat") == 0)
			ParseStringAssignment(gameinfo.BorderFlat);
		else if(key.CompareNoCase("deathtransition") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			if(sc->str.CompareNoCase("fizzle") == 0)
				gameinfo.DeathTransition = GameInfo::TRANSITION_Fizzle;
			else if(sc->str.CompareNoCase("fade") == 0)
				gameinfo.DeathTransition = GameInfo::TRANSITION_Fade;
			else
				sc.ScriptMessage(Scanner::ERROR, "Unknown transition type '%s'.", sc->str.GetChars());
		}
		else if(key.CompareNoCase("dialogcolor") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::DIALOG]);
		else if(key.CompareNoCase("doorsoundsequence") == 0)
			ParseNameAssignment(gameinfo.DoorSoundSequence);
		else if(key.CompareNoCase("drawreadthis") == 0)
			ParseBoolAssignment(gameinfo.DrawReadThis);
		else if(key.CompareNoCase("trackhighscores") == 0)
			ParseBoolAssignment(gameinfo.TrackHighScores);
		else if(key.CompareNoCase("gamecolormap") == 0)
			ParseStringAssignment(gameinfo.GameColormap);
		else if(key.CompareNoCase("gameoverpic") == 0)
			ParseStringAssignment(gameinfo.GameOverPic);
		else if(key.CompareNoCase("victorypic") == 0)
			ParseStringAssignment(gameinfo.VictoryPic);
		else if(key.CompareNoCase("gamepalette") == 0)
			ParseStringAssignment(gameinfo.GamePalette);
		else if(key.CompareNoCase("gibfactor") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_FloatConst);
			gameinfo.GibFactor = static_cast<fixed>(sc->decimal*FRACUNIT);
		}
		else if(key.CompareNoCase("signon") == 0)
			ParseStringAssignment(gameinfo.SignonLump);
		else if(key.CompareNoCase("menufade") == 0)
			ParseColorAssignment(gameinfo.MenuFadeColor);
		else if(key.CompareNoCase("menucolors") == 0)
			// Border1, Border2, Border3, Background, Stripe, StripeBG
			ParseColorArrayAssignment(gameinfo.MenuColors, 6);
		else if(key.CompareNoCase("messagecolors") == 0)
			// Background, Top Color, Bottom Color
			ParseColorArrayAssignment(gameinfo.MessageColors, 3);
		else if(key.CompareNoCase("messagefontcolor") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MESSAGEFONT]);
		else if(key.CompareNoCase("titlemusic") == 0)
			ParseStringAssignment(gameinfo.TitleMusic);
		else if(key.CompareNoCase("titlepalette") == 0)
			ParseStringAssignment(gameinfo.TitlePalette);
		else if(key.CompareNoCase("titlepage") == 0)
			ParseStringAssignment(gameinfo.TitlePage);
		else if(key.CompareNoCase("titletime") == 0)
			ParseIntAssignment(gameinfo.TitleTime);
		else if(key.CompareNoCase("translator") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			gameinfo.Translator.Push(sc->str);
		}
		else if(key.CompareNoCase("menumusic") == 0)
			ParseStringAssignment(gameinfo.MenuMusic);
		else if(key.CompareNoCase("pushwallsoundsequence") == 0)
			ParseNameAssignment(gameinfo.PushwallSoundSequence);
		else if(key.CompareNoCase("scoresmusic") == 0)
			ParseStringAssignment(gameinfo.ScoresMusic);
		else if(key.CompareNoCase("menuwindowcolors") == 0)
			// Background, Top color, bottom color, Index background, Index top, Index bottom
			ParseColorArrayAssignment(gameinfo.MenuWindowColors, 6);
		else if(key.CompareNoCase("finaleflat") == 0)
			ParseStringAssignment(gameinfo.FinaleFlat);
		else if(key.CompareNoCase("finalemusic") == 0)
			ParseStringAssignment(gameinfo.FinaleMusic);
		else if(key.CompareNoCase("victorymusic") == 0)
			ParseStringAssignment(gameinfo.VictoryMusic);
		else if(key.CompareNoCase("intermissionmusic") == 0)
			ParseStringAssignment(gameinfo.IntermissionMusic);
		else if(key.CompareNoCase("menufontcolor_title") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_TITLE]);
		else if(key.CompareNoCase("menufontcolor_label") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_LABEL]);
		else if(key.CompareNoCase("menufontcolor_selection") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_SELECTION]);
		else if(key.CompareNoCase("menufontcolor_disabled") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_DISABLED]);
		else if(key.CompareNoCase("menufontcolor_invalid") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_INVALID]);
		else if(key.CompareNoCase("menufontcolor_invalidselection") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_INVALIDSELECTION]);
		else if(key.CompareNoCase("menufontcolor_highlight") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_HIGHLIGHT]);
		else if(key.CompareNoCase("menufontcolor_highlightselection") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::MENU_HIGHLIGHTSELECTION]);
		else if(key.CompareNoCase("highscoresfont") == 0)
			ParseStringAssignment(gameinfo.HighScoresFont);
		else if(key.CompareNoCase("highscoresfontcolor") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::HIGHSCORES]);
		else if(key.CompareNoCase("pageindexfontcolor") == 0)
			ParseFontColorAssignment(gameinfo.FontColors[GameInfo::PAGEINDEX]);
		else if(key.CompareNoCase("pageindextext") == 0)
			ParseStringAssignment(gameinfo.PageIndexText);
		else if(key.CompareNoCase("psyched") == 0)
		{
			ParseColorArrayAssignment(gameinfo.PsychedColors, 2);
			if(sc.CheckToken(','))
			{
				bool negative = sc.CheckToken('-');
				sc.MustGetToken(TK_IntConst);
				gameinfo.PsychedOffset = negative ? -sc->number : sc->number;
			}
			else
				gameinfo.PsychedOffset = 0;
		}
		else if(key.CompareNoCase("playerclasses") == 0)
		{
			sc.MustGetToken('=');
			gameinfo.PlayerClasses.Clear();
			do
			{
				sc.MustGetToken(TK_StringConst);
				gameinfo.PlayerClasses.Push(sc->str);

			}
			while(sc.CheckToken(','));
		}
		else if(key.CompareNoCase("quitmessages") == 0)
			ParseStringArrayAssignment(gameinfo.QuitMessages);
		else
			return false;
		return true;
	}
};

class AutomapBlockParser : public MapInfoBlockParser
{
public:
	AutomapBlockParser(Scanner &sc) : MapInfoBlockParser(sc, "automap") {}

protected:
	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("Background") == 0)
			ParseColorAssignment(gameinfo.automap.Background);
		else if(key.CompareNoCase("DoorColor") == 0)
			ParseColorAssignment(gameinfo.automap.DoorColor);
		else if(key.CompareNoCase("FloorColor") == 0)
			ParseColorAssignment(gameinfo.automap.FloorColor);
		else if(key.CompareNoCase("FontColor") == 0)
			ParseFontColorAssignment(gameinfo.automap.FontColor);
		else if(key.CompareNoCase("WallColor") == 0)
			ParseColorAssignment(gameinfo.automap.WallColor);
		else if(key.CompareNoCase("YourColor") == 0)
			ParseColorAssignment(gameinfo.automap.YourColor);
		else
			return false;
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

static TArray<EpisodeInfo> episodes;

EpisodeInfo::EpisodeInfo() : Shortcut(0), NoSkill(false)
{
}

unsigned int EpisodeInfo::GetNumEpisodes()
{
	return episodes.Size();
}

EpisodeInfo &EpisodeInfo::GetEpisode(unsigned int index)
{
	return episodes[index];
}

class EpisodeBlockParser : public MapInfoBlockParser
{
private:
	EpisodeInfo &episode;
	bool useEpisode;

public:
	EpisodeBlockParser(Scanner &sc, EpisodeInfo &episode) :
		MapInfoBlockParser(sc, "episode"), episode(episode), useEpisode(true)
		{}

	bool UseEpisode() const { return useEpisode; }

protected:
	void ParseHeader()
	{
		sc.MustGetToken(TK_StringConst);
		episode.StartMap = sc->str;
	}

	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("name") == 0)
			ParseStringAssignment(episode.EpisodeName);
		else if(key.CompareNoCase("lookup") == 0)
		{
			ParseStringAssignment(episode.EpisodeName);
			episode.EpisodeName = language[episode.EpisodeName];
		}
		else if(key.CompareNoCase("picname") == 0)
			ParseStringAssignment(episode.EpisodePicture);
		else if(key.CompareNoCase("key") == 0)
		{
			FString tmp;
			ParseStringAssignment(tmp);
			episode.Shortcut = tmp[0];
		}
		else if(key.CompareNoCase("remove") == 0)
			useEpisode = false;
		else if(key.CompareNoCase("noskillmenu") == 0)
			episode.NoSkill = true;
		else if(key.CompareNoCase("optional") == 0)
		{
			if(Wads.CheckNumForName(episode.StartMap) == -1)
				useEpisode = false;
		}
		else
			return false;
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

static TMap<unsigned int, ClusterInfo> clusters;

ClusterInfo::ClusterInfo() : EnterTextType(ClusterInfo::EXIT_STRING),
	ExitTextType(ClusterInfo::EXIT_STRING), TextFont(SmallFont),
	TextAlignment(TS_Left), TextAnchor(TS_Middle), TextColor(CR_UNTRANSLATED)
{
}

ClusterInfo &ClusterInfo::Find(unsigned int index)
{
	return clusters[index];
}

class ClusterBlockParser : public MapInfoBlockParser
{
private:
	ClusterInfo *cluster;

public:
	ClusterBlockParser(Scanner &sc) :
		MapInfoBlockParser(sc, "cluster")
		{}
protected:
	void ParseHeader()
	{
		sc.MustGetToken(TK_IntConst);
		cluster = &ClusterInfo::Find(sc->number);
	}

	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("enterslideshow") == 0)
			ParseStringAssignment(cluster->EnterSlideshow);
		else if(key.CompareNoCase("exitslideshow") == 0)
			ParseStringAssignment(cluster->ExitSlideshow);
		else if(key.CompareNoCase("exittext") == 0 || key.CompareNoCase("entertext") == 0)
		{
			sc.MustGetToken('=');
			bool lookup = false;
			if(sc.CheckToken(TK_Identifier))
			{
				if(sc->str.CompareNoCase("lookup") != 0)
					sc.ScriptMessage(Scanner::ERROR, "Expected lookup but got '%s' instead.", sc->str.GetChars());
				sc.MustGetToken(',');
				lookup = true;
			}
			sc.MustGetToken(TK_StringConst);

			FString &text = key.CompareNoCase("exittext") == 0 ? cluster->ExitText : cluster->EnterText;
			text = lookup ? FString(language[sc->str]) : sc->str;
		}
		else if(key.CompareNoCase("entertextislump") == 0)
			cluster->EnterTextType = ClusterInfo::EXIT_LUMP;
		else if(key.CompareNoCase("exittextislump") == 0)
			cluster->ExitTextType = ClusterInfo::EXIT_LUMP;
		else if(key.CompareNoCase("entertextismessage") == 0)
			cluster->EnterTextType = ClusterInfo::EXIT_MESSAGE;
		else if(key.CompareNoCase("exittextismessage") == 0)
			cluster->ExitTextType = ClusterInfo::EXIT_MESSAGE;
		else if(key.CompareNoCase("flat") == 0)
			ParseStringAssignment(cluster->Flat);
		else if(key.CompareNoCase("music") == 0)
			ParseStringAssignment(cluster->Music);
		else if(sc->str.CompareNoCase("textalignment") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_Identifier);
			if(sc->str.CompareNoCase("left") == 0)
				cluster->TextAlignment = TS_Left;
			else if(sc->str.CompareNoCase("center") == 0)
				cluster->TextAlignment = TS_Center;
			else if(sc->str.CompareNoCase("right") == 0)
				cluster->TextAlignment = TS_Right;
			else
				sc.ScriptMessage(Scanner::ERROR, "Unknown alignment.");
		}
		else if(sc->str.CompareNoCase("textanchor") == 0)
		{
			sc.MustGetToken('=');
			sc.MustGetToken(TK_Identifier);
			if(sc->str.CompareNoCase("top") == 0)
				cluster->TextAnchor = TS_Top;
			else if(sc->str.CompareNoCase("middle") == 0)
				cluster->TextAnchor = TS_Middle;
			else if(sc->str.CompareNoCase("bottom") == 0)
				cluster->TextAnchor = TS_Bottom;
			else
				sc.ScriptMessage(Scanner::ERROR, "Unknown anchor.");
		}
		else if(sc->str.CompareNoCase("textcolor") == 0)
			ParseFontColorAssignment(cluster->TextColor);
		else if(sc->str.CompareNoCase("textfont") == 0)
		{
			FString fontName;
			ParseStringAssignment(fontName);
			cluster->TextFont = V_GetFont(fontName);
		}
		else
			return false;
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

static TArray<SkillInfo> skills;
static TMap<FName, unsigned int> skillIds;

SkillInfo::SkillInfo() : DamageFactor(FRACUNIT), PlayerDamageFactor(FRACUNIT),
	SpawnFilter(0), MapFilter(0), FastMonsters(false), QuizHints(false), LivesCount(3),
	ScoreMultiplier(FRACUNIT)
{
}

unsigned int SkillInfo::GetNumSkills()
{
	return skills.Size();
}

unsigned int SkillInfo::GetSkillIndex(const SkillInfo &skill)
{
	return (unsigned int)(&skill - &skills[0]);
}

SkillInfo &SkillInfo::GetSkill(unsigned int index)
{
	return skills[MIN(index, skills.Size()-1)];
}

class SkillInfoBlockParser : public MapInfoBlockParser
{
private:
	SkillInfo *skill;

public:
	SkillInfoBlockParser(Scanner &sc) : MapInfoBlockParser(sc, "skill")
	{
	}

protected:
	void ParseHeader()
	{
		sc.MustGetToken(TK_Identifier);
		const unsigned int *id = skillIds.CheckKey(sc->str);
		if(id)
			skill = &skills[*id];
		else
		{
			unsigned int newId = skills.Push(SkillInfo());
			skill = &skills[newId];
			skillIds.Insert(sc->str, newId);
		}
	}

	bool CheckKey(FString key)
	{
		if(key.CompareNoCase("damagefactor") == 0)
			ParseFixedAssignment(skill->DamageFactor);
		else if(key.CompareNoCase("fastmonsters") == 0)
			skill->FastMonsters = true;
		else if(key.CompareNoCase("name") == 0)
		{
			ParseStringAssignment(skill->Name);
			if(skill->Name[0] == '$')
				skill->Name = language[skill->Name.Mid(1)];
		}
		else if(key.CompareNoCase("picname") == 0)
			ParseStringAssignment(skill->SkillPicture);
		else if(key.CompareNoCase("playerdamagefactor") == 0)
			ParseFixedAssignment(skill->PlayerDamageFactor);
		else if(key.CompareNoCase("spawnfilter") == 0)
		{
			ParseIntAssignment(skill->SpawnFilter);
			--skill->SpawnFilter;
		}
		else if(key.CompareNoCase("mapfilter") == 0)
			ParseIntAssignment(skill->MapFilter);
		else if(key.CompareNoCase("mustconfirm") == 0)
		{
			ParseStringAssignment(skill->MustConfirm);
			if(skill->MustConfirm[0] == '$')
				skill->MustConfirm = language[skill->MustConfirm.Mid(1)];
		}
		else if(key.CompareNoCase("quizhints") == 0)
			ParseBoolAssignment(skill->QuizHints);
        else if(key.CompareNoCase("lives") == 0)
            ParseIntAssignment(skill->LivesCount);
        else if (key.CompareNoCase("scoremultiplier") == 0)
            ParseFixedAssignment(skill->ScoreMultiplier);
		else
			return false;
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

class IntermissionBlockParser : public MapInfoBlockParser
{
private:
	IntermissionInfo *intermission;

public:
	IntermissionBlockParser(Scanner &sc) : MapInfoBlockParser(sc, "intermission")
	{
	}

protected:
	void ParseHeader()
	{
		sc.MustGetToken(TK_Identifier);

		intermission = IntermissionInfo::Find(sc->str);
		intermission->Clear();
	}

	void ParseTimeAssignment(unsigned int &time)
	{
		sc.MustGetToken('=');

		if(sc.CheckToken(TK_Identifier))
		{
			if(sc->str.CompareNoCase("titletime") == 0)
				time = gameinfo.TitleTime*TICRATE;
			else
				sc.ScriptMessage(Scanner::ERROR, "Invalid special time %s.\n", sc->str.GetChars());
		}
		else
		{
			bool inSeconds = sc.CheckToken('-');
			sc.MustGetToken(TK_FloatConst);
			if(!CheckTicsValid(sc->decimal))
				sc.ScriptMessage(Scanner::ERROR, "Invalid tic duration.");

			time = static_cast<unsigned int>(sc->decimal*2);
			if(inSeconds)
				time *= 35;
		}
	}

	bool CheckKey(FString key)
	{
		IntermissionInfo::Action action;
		if(key.CompareNoCase("Cast") == 0)
		{
			CastIntermissionAction *cast = new CastIntermissionAction();

			action.type = IntermissionInfo::CAST;
			action.action = cast;

			if(!ParseCast(cast))
				return false;
		}
		else if(key.CompareNoCase("Fader") == 0)
		{
			FaderIntermissionAction *fader = new FaderIntermissionAction();

			action.type = IntermissionInfo::FADER;
			action.action = fader;

			if(!ParseFader(fader))
				return false;
		}
		else if(key.CompareNoCase("GotoTitle") == 0)
		{
			action.type = IntermissionInfo::GOTOTITLE;
			action.action = new IntermissionAction();
			sc.MustGetToken('{');
			sc.MustGetToken('}');
		}
		else if(key.CompareNoCase("Image") == 0)
		{
			action.type = IntermissionInfo::IMAGE;
			action.action = new IntermissionAction();

			sc.MustGetToken('{');
			while(!sc.CheckToken('}'))
			{
				sc.MustGetToken(TK_Identifier);
				if(!CheckStandardKey(action.action, sc->str))
					return false;
			}
		}
		else if(key.CompareNoCase("Link") == 0)
		{
			ParseNameAssignment(intermission->Link);
			return true;
		}
		else if(key.CompareNoCase("TextScreen") == 0)
		{
			TextScreenIntermissionAction *textscreen = new TextScreenIntermissionAction();

			action.type = IntermissionInfo::TEXTSCREEN;
			action.action = textscreen;

			if(!ParseTextScreen(textscreen))
				return false;
		}
		else if(key.CompareNoCase("VictoryStats") == 0)
		{
			action.type = IntermissionInfo::VICTORYSTATS;
			action.action = new IntermissionAction();
			sc.MustGetToken('{');
			sc.MustGetToken('}');
		}
		else
			return false;

		intermission->Actions.Push(action);
		return true;
	}

	bool CheckStandardKey(IntermissionAction *action, const FString &key)
	{
		if(key.CompareNoCase("Draw") == 0)
		{
			IntermissionAction::DrawData data;

			sc.MustGetToken('=');
			sc.MustGetToken(TK_StringConst);
			data.Image = TexMan.CheckForTexture(sc->str, FTexture::TEX_Any);
			sc.MustGetToken(',');
			sc.MustGetToken(TK_IntConst);
			data.X = sc->number;
			sc.MustGetToken(',');
			sc.MustGetToken(TK_IntConst);
			data.Y = sc->number;

			action->Draw.Push(data);
		}
		else if(key.CompareNoCase("Background") == 0)
		{
			sc.MustGetToken('=');
			if(sc.CheckToken(TK_Identifier))
			{
				if(sc->str.CompareNoCase("HighScores") == 0)
				{
					action->Type = IntermissionAction::HIGHSCORES;
				}
				else if(sc->str.CompareNoCase("TitlePage") == 0)
				{
					action->Type = IntermissionAction::TITLEPAGE;
				}
				else if(sc->str.CompareNoCase("LoadMap") == 0)
				{
					action->Type = IntermissionAction::LOADMAP;
					sc.MustGetToken(',');
					sc.MustGetToken(TK_StringConst);
					action->MapName = sc->str;
				}
				else
				{
					sc.ScriptMessage(Scanner::ERROR, "Unknown background type %s. Use quotes for static image.", sc->str.GetChars());
				}
			}
			else
			{
				sc.MustGetToken(TK_StringConst);
				FString tex = sc->str;
				action->Background = TexMan.CheckForTexture(tex, FTexture::TEX_Any);
				action->Type = IntermissionAction::NORMAL;
				if(sc.CheckToken(','))
				{
					if(!sc.CheckToken(TK_BoolConst))
						sc.MustGetToken(TK_IntConst);
					action->BackgroundTile = sc->boolean;
					if(sc.CheckToken(','))
					{
						sc.MustGetToken(TK_StringConst);
						action->Palette = sc->str;
					}
				}
			}
		}
		else if(key.CompareNoCase("Music") == 0)
			ParseStringAssignment(action->Music);
		else if(key.CompareNoCase("Time") == 0)
			ParseTimeAssignment(action->Time);
		else
			return false;
		return true;
	}

	bool ParseCast(CastIntermissionAction *cast)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			if(CheckStandardKey(cast, sc->str))
				continue;

			if(sc->str.CompareNoCase("CastClass") == 0)
			{
				sc.MustGetToken('=');
				sc.MustGetToken(TK_StringConst);
				cast->Class = ClassDef::FindClass(sc->str);
			}
			else if(sc->str.CompareNoCase("CastName") == 0)
			{
				ParseStringAssignment(cast->Name);
				if(cast->Name[0] == '$')
					cast->Name = language[cast->Name.Mid(1)];
			}
			else
				return false;
		}
		return true;
	}

	bool ParseFader(FaderIntermissionAction *fader)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			if(CheckStandardKey(fader, sc->str))
				continue;

			if(sc->str.CompareNoCase("FadeType") == 0)
			{
				sc.MustGetToken('=');
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("FadeIn") == 0)
					fader->Fade = FaderIntermissionAction::FADEIN;
				else if(sc->str.CompareNoCase("FadeOut") == 0)
					fader->Fade = FaderIntermissionAction::FADEOUT;
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown fade type.");
			}
			else
				return false;
		}
		return true;
	}

	bool ParseTextScreen(TextScreenIntermissionAction *textscreen)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			if(CheckStandardKey(textscreen, sc->str))
				continue;

			if(sc->str.CompareNoCase("FadeTime") == 0)
				ParseTimeAssignment(textscreen->FadeTime);
			else if(sc->str.CompareNoCase("Text") == 0)
				ParseStringArrayAssignment(textscreen->Text);
			else if(sc->str.CompareNoCase("TextAlignment") == 0)
			{
				sc.MustGetToken('=');
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("left") == 0)
					textscreen->Alignment = TS_Left;
				else if(sc->str.CompareNoCase("center") == 0)
					textscreen->Alignment = TS_Center;
				else if(sc->str.CompareNoCase("right") == 0)
					textscreen->Alignment = TS_Right;
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown alignment.");
			}
			else if(sc->str.CompareNoCase("TextAnchor") == 0)
			{
				sc.MustGetToken('=');
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("top") == 0)
					textscreen->Anchor = TS_Top;
				else if(sc->str.CompareNoCase("middle") == 0)
					textscreen->Anchor = TS_Middle;
				else if(sc->str.CompareNoCase("bottom") == 0)
					textscreen->Anchor = TS_Bottom;
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown anchor.");
			}
			else if(sc->str.CompareNoCase("TextColor") == 0)
				ParseFontColorAssignment(textscreen->TextColor);
			else if(sc->str.CompareNoCase("TextDelay") == 0)
				ParseTicAssignment(textscreen->TextDelay);
			else if(sc->str.CompareNoCase("TextFont") == 0)
			{
				FString fontName;
				ParseStringAssignment(fontName);
				textscreen->TextFont = V_GetFont(fontName);
			}
			else if(sc->str.CompareNoCase("TextSpeed") == 0)
				ParseTicAssignment(textscreen->TextSpeed);
			else if(sc->str.CompareNoCase("Position") == 0)
			{
				sc.MustGetToken('=');
				sc.MustGetToken(TK_IntConst);
				textscreen->PrintX = sc->number;
				sc.MustGetToken(',');
				sc.MustGetToken(TK_IntConst);
				textscreen->PrintY = sc->number;
			}
			else
				return false;
		}
		return true;
	}
};

////////////////////////////////////////////////////////////////////////////////

static void SkipBlock(Scanner &sc)
{
	// Skip header
	while(sc.GetNextToken() && sc->token != '{') {}
	// Skip content
	unsigned int level = 0;
	while(sc.GetNextToken() && (level != 0 || sc->token != '}'))
	{
		if(sc->token == '{')
			++level;
		else if(sc->token == '}')
			--level;
	}
}

static void ParseMapInfoLump(int lump, bool gameinfoPass)
{
	Scanner sc(lump);

	while(sc.TokensLeft())
	{
		sc.MustGetToken(TK_Identifier);
		if(sc->str.CompareNoCase("include") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			int includeLump = Wads.GetNumForFullName(sc->str);
			if(includeLump != -1)
				ParseMapInfoLump(includeLump, gameinfoPass);

			continue;
		}

		if(!gameinfoPass)
		{
			if(sc->str.CompareNoCase("defaultmap") == 0)
			{
				defaultMap = LevelInfo();
				LevelInfoBlockParser(sc, defaultMap, false).Parse();
			}
			else if(sc->str.CompareNoCase("adddefaultmap") == 0)
			{
				LevelInfoBlockParser(sc, defaultMap, false).Parse();
			}
			else if(sc->str.CompareNoCase("automap") == 0)
			{
				AutomapBlockParser(sc).Parse();
			}
			else if(sc->str.CompareNoCase("clearepisodes") == 0)
			{
				episodes.Clear();
			}
			else if(sc->str.CompareNoCase("clearskills") == 0)
			{
				skills.Clear();
				skillIds.Clear();
			}
			else if(sc->str.CompareNoCase("cluster") == 0)
			{
				ClusterBlockParser(sc).Parse();
			}
			else if(sc->str.CompareNoCase("episode") == 0)
			{
				EpisodeInfo episode;
				EpisodeBlockParser parser(sc, episode);
				parser.Parse();
				if(parser.UseEpisode())
					episodes.Push(episode);
			}
			else if(sc->str.CompareNoCase("intermission") == 0)
			{
				IntermissionBlockParser(sc).Parse();
			}
			else if(sc->str.CompareNoCase("map") == 0)
			{
				LevelInfo newMap = defaultMap;
				LevelInfoBlockParser(sc, newMap, true).Parse();

				LevelInfo &existing = LevelInfo::Find(newMap.MapName);
				if(&existing != &defaultMap)
					existing = newMap;
				else
					levelInfos.Push(newMap);
			}
			else if(sc->str.CompareNoCase("skill") == 0)
			{
				SkillInfoBlockParser(sc).Parse();
			}
			else
				SkipBlock(sc);
		}
		else
		{
			if(sc->str.CompareNoCase("gameinfo") == 0)
			{
				GameInfoBlockParser(sc).Parse();
			}
			// Regular key words
			else if(sc->str.CompareNoCase("clearepisodes") == 0) {}
			else
				SkipBlock(sc);
		}
	}
}

// The Mac version of Wolf3D had a primitive scenario definition chunk.
void ParseMacMapList(int lumpnum)
{
	int songlumpnum = Wads.CheckNumForName("SONGLIST");
	TArray<WORD> songs;
	if(songlumpnum != -1)
	{
		FWadLump songlump = Wads.OpenLumpNum(songlumpnum);
		songs.Resize(songlump.GetLength()/2);
		songlump.Read(&songs[0], songs.Size()*2);
		for(unsigned int i = 0;i < songs.Size();++i)
			songs[i] = BigShort(songs[i]);

		gameinfo.TitleMusic.Format("MUS_%04X", songs[0]);
		gameinfo.MenuMusic = gameinfo.TitleMusic;
		gameinfo.IntermissionMusic.Format("MUS_%04X", songs[1]);
	}

	FWadLump lump = Wads.OpenLumpNum(lumpnum);

	WORD numMaps;
	lump.Read(&numMaps, sizeof(numMaps));
	lump.Seek(2, SEEK_CUR);
	numMaps = BigShort(numMaps);

	for(unsigned int i = 0;i < numMaps;++i)
	{
		WORD nextLevel, nextSecret, parTime, scenarioNum, floorNum;
		lump.Read(&nextLevel, sizeof(nextLevel));
		lump.Read(&nextSecret, sizeof(nextSecret));
		lump.Read(&parTime, sizeof(parTime));
		lump.Read(&scenarioNum, sizeof(scenarioNum));
		lump.Read(&floorNum, sizeof(floorNum));

		nextLevel = BigShort(nextLevel);
		nextSecret = BigShort(nextSecret);
		parTime = BigShort(parTime);
		scenarioNum = BigShort(scenarioNum);
		floorNum = BigShort(floorNum);

		LevelInfo info = defaultMap;
		mysnprintf(info.MapName, countof(info.MapName), "MAP%02d", i+1);
		info.NextMap.Format("MAP%02d", nextLevel+1);
		info.NextSecret.Format("MAP%02d", nextSecret+1);
		info.Par = parTime;
		info.FloorNumber.Format("%-2d-%d", scenarioNum, floorNum);
		info.UseMapInfoName = true;
		info.Name = info.FloorNumber;

		if(songs.Size() > 0)
			info.Music.Format("MUS_%04X", songs[(i+2)%songs.Size()]);

		LevelInfo &existing = LevelInfo::Find(info.MapName);
		if(&existing != &defaultMap)
			existing = info;
		else
			levelInfos.Push(info);
	}
}

void G_ParseMapInfo(bool gameinfoPass)
{
	int lastlump = 0;
	int lump;

	if((lump = Wads.GetNumForFullName(IWad::GetGame().Mapinfo)) != -1)
		ParseMapInfoLump(lump, gameinfoPass);

	if(!gameinfoPass && (lump = Wads.CheckNumForName("MAPLIST")) != -1)
		ParseMacMapList(lump);

	while((lump = Wads.FindLump("MAPINFO", &lastlump)) != -1)
		ParseMapInfoLump(lump, gameinfoPass);

	while((lump = Wads.FindLump("ZMAPINFO", &lastlump)) != -1)
		ParseMapInfoLump(lump, gameinfoPass);

	// Sanity checks!
	if(!gameinfoPass)
	{
		if(episodes.Size() == 0)
			I_FatalError("At least 1 episode must be defined.");

		for(unsigned int i = 0;i < gameinfo.PlayerClasses.Size();++i)
		{
			const ClassDef *cls = ClassDef::FindClass(gameinfo.PlayerClasses[i]);
			if(!cls || !cls->IsDescendantOf(NATIVE_CLASS(PlayerPawn)))
				I_FatalError("'%s' is not a valid player class!", gameinfo.PlayerClasses[i].GetChars());
		}
	}
}
