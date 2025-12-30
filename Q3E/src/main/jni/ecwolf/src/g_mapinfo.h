/*
** g_mapinfo.h
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

#ifndef __G_MAPINFO_H__
#define __G_MAPINFO_H__

#include "textures/textures.h"
#include "v_font.h"
#include "wl_text.h"
#include "zstring.h"

class ClassDef;
class GameMap;

extern class GameInfo
{
public:
	GameInfo();

	FString	SignonLump;
	int		MenuFadeColor;
	int		MenuColors[6];
	int		MessageColors[3];
	int		MenuWindowColors[6];
	int		AdvisoryColor;
	int		PsychedColors[2];
	int 	PsychedOffset;
	bool	DrawReadThis;
	bool	TrackHighScores;

	int		TitleTime;
	FString	BorderFlat;
	FString GameColormap;
	FString	GamePalette;
	FString	TitleMusic;
	FString	TitlePage;
	FString	TitlePalette;
	FString	MenuMusic;
	FString	ScoresMusic;
	FString	FinaleMusic;
	FString VictoryMusic;
	FString	IntermissionMusic;
	FString	HighScoresFont;
	FString	AdvisoryPic;
	FString FinaleFlat;
	FString GameOverPic;
	FString VictoryPic;
	FString PageIndexText;
	// Special stack for strings like the default translator.
	// This will allow the previous default to be included.
	class FStringStack
	{
	public:
		FStringStack() : next(NULL) {}
		~FStringStack() { delete next; }

		const FStringStack *Next() const { return next; }
		void Push(const FString &str)
		{
			if(!this->str.IsEmpty())
				next = new FStringStack(*this);
			this->str = str;
		}

		FString str;
	private:
		FStringStack *next;
	} Translator;
	FName	DoorSoundSequence;
	FName	PushwallSoundSequence;
	fixed	GibFactor;

	TArray<FName>	PlayerClasses;
	TArray<FString>	QuitMessages;

	enum EFontColors
	{
		MENU_TITLE,
		MENU_LABEL,
		MENU_SELECTION,
		MENU_DISABLED,
		MENU_INVALID,
		MENU_INVALIDSELECTION,
		MENU_HIGHLIGHT,
		MENU_HIGHLIGHTSELECTION,
		HIGHSCORES,
		PAGEINDEX,
		MESSAGEFONT,
		DIALOG,

		NUM_FONTCOLORS
	};
	EColorRange	FontColors[NUM_FONTCOLORS];

	enum ETransition
	{
		TRANSITION_Fizzle,
		TRANSITION_Fade
	};
	ETransition DeathTransition;

	struct BorderTextures
	{
		bool issolid;

		int topcolor;
		int bottomcolor;
		int highlightcolor;

		int offset;
		FString tl, t, tr;
		FString l, r;
		FString bl, b, br;
	} Border;

	struct AutomapInfo
	{
		EColorRange FontColor;
		int Background;
		int DoorColor;
		int FloorColor;
		int WallColor;
		int YourColor;
	} automap;
} gameinfo;

class LevelInfo
{
public:
	LevelInfo();
	FTextureID GetBorderTexture() const;
	FString GetMusic(const GameMap *gm) const;
	FString GetName(const GameMap *gm) const;

	char			MapName[9];
	FString			NextMap;
	FString			NextSecret;
	FString			NextVictory;
	FString			FloorNumber;
	FString			Music;
	unsigned int	Cluster;
	FString			Translator;
	FTextureID		TitlePatch;

	FTextureID		BorderTexture;
	FTextureID		DefaultTexture[2];
	int				DefaultLighting;
	fixed			DefaultVisibility;
	fixed			DefaultMaxLightVis;
	int				ExitFadeColor;
	unsigned int	ExitFadeDuration;
	unsigned int	Par;
	FString			CompletionString;
	FTextureID		HighScoresGraphic;
	int				LevelBonus;
	unsigned int	LevelNumber;
	bool			NoIntermission;

	bool			DeathCam;
	bool			SecretDeathSounds;
	bool			SpawnWithWeaponRaised;
	bool			ForceTally;
	bool			ResetHealth;
	bool			ResetInventory;

	TArray<const ClassDef *>	EnsureInventory;

	struct SpecialAction
	{
	public:
		const ClassDef	*Class;
		unsigned int	Special;
		int				Args[5];
	};
	TArray<SpecialAction>	SpecialActions;

	static LevelInfo &Find(const char* level);
	static LevelInfo &FindByNumber(unsigned int num);

protected:
	friend class LevelInfoBlockParser;
	friend void ParseMacMapList(int);

	bool			UseMapInfoName;
	FString			Name;
};

class EpisodeInfo
{
public:
	EpisodeInfo();

	FString		StartMap;
	FString		EpisodeName;
	FString		EpisodePicture;
	char		Shortcut;
	bool		NoSkill;

	static unsigned int GetNumEpisodes();
	static EpisodeInfo &GetEpisode(unsigned int index);
};

class ClusterInfo
{
public:
	enum ExitType
	{
		EXIT_STRING,
		EXIT_LUMP,
		EXIT_MESSAGE
	};

	ClusterInfo();

	FString		EnterSlideshow, ExitSlideshow;
	FString		EnterText, ExitText;
	ExitType	EnterTextType, ExitTextType;
	FString		Flat;
	FString		Music;
	FFont		*TextFont;
	ETSAlignment TextAlignment;
	ETSAnchor	TextAnchor;
	EColorRange	TextColor;

	static ClusterInfo &Find(unsigned int index);
};

class SkillInfo
{
public:
	SkillInfo();

	FString Name;
	FString SkillPicture;
	FString MustConfirm;
	fixed DamageFactor;
	fixed PlayerDamageFactor;
	unsigned int SpawnFilter;
	unsigned int MapFilter;
	bool FastMonsters;
	bool QuizHints;
	int LivesCount;
	fixed ScoreMultiplier;

	static unsigned int GetNumSkills();
	static unsigned int GetSkillIndex(const SkillInfo &skill);
	static SkillInfo &GetSkill(unsigned int index);
};

void G_ParseMapInfo(bool gameinfoPass);

#endif
