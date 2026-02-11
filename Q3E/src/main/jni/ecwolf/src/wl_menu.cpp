////////////////////////////////////////////////////////////////////
//
// WL_MENU.C
// by John Romero (C) 1992 Id Software, Inc.
//
////////////////////////////////////////////////////////////////////

#include "m_classes.h"
#include "m_random.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "wl_iwad.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "language.h"
#include "w_wad.h"
#include "c_cvars.h"
#include "g_mapinfo.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_inter.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_net.h"
#include "wl_play.h"
#include "wl_text.h"
#include "v_palette.h"
#include "colormatcher.h"
#include "v_font.h"
#include "templates.h"
#include "thingdef/thingdef.h"
#include "wl_loadsave.h"
#include "am_map.h"

#include <climits>

static int	lastgamemusicoffset;
static FName playerClass = NAME_None;
EpisodeInfo	*episode = 0;
int BORDCOLOR, BORD2COLOR, BORD3COLOR, BKGDCOLOR, STRIPE, STRIPEBG,
	MENUWIN_BACKGROUND, MENUWIN_TOPBORDER, MENUWIN_BOTBORDER,
	MENUWINHGLT_BACKGROUND, MENUWINHGLT_TOPBORDER, MENUWINHGLT_BOTBORDER;
static MenuItem	*readThis;
// Android version reads this elsewhere so non-static.
bool menusAreFaded = true;

EMenuStyle MenuStyle = MENUSTYLE_Wolf;

MENU_LISTENER(EnterControlBase);
MENU_LISTENER(JoinNetGame);

Menu mainMenu(MENU_X, MENU_Y, MENU_W, 24);
Menu optionsMenu(80, 80, 190, 28);
Menu soundBase(24, 45, 284, 24);
Menu controlBase(CTL_X, CTL_Y, CTL_W, 56, EnterControlBase);
Menu displayMenu(20, 75, 285, 56);
Menu automapMenu(40, 55, 260, 56);
Menu mouseSensitivity(20, 50, 300, 24);
Menu joySensitivity(20, 30, 300, 24);
Menu playerClasses(NM_X, NM_Y, NM_W, 24);
Menu episodes(NE_X+4, NE_Y-1, NE_W+7, 83);
Menu skills(NM_X, NM_Y, NM_W, 24);
Menu controls(15, 70, 310, 24);
Menu resolutionMenu(90, 25, 150, 24);

MENU_LISTENER(PlayDemosOrReturnToGame)
{
	Menu::closeMenus();
	if (!ingame)
		StartCPMusic(gameinfo.TitleMusic);
	return true;
}
MENU_LISTENER(ViewScoresOrEndGame)
{
	if (ingame)
	{
		if(CP_EndGame(0))
			Menu::closeMenus();
	}
	else
	{
		if (gameinfo.TrackHighScores == true && Net::InitVars.mode == Net::MODE_SinglePlayer)
		{
			MenuFadeOut();

			StartCPMusic(gameinfo.ScoresMusic);

			DrawHighScores();
			VW_UpdateScreen();
			MenuFadeIn();

			IN_Ack(ACK_Local);

			StartCPMusic(gameinfo.MenuMusic);
			MenuFadeOut();
			mainMenu.draw();
			MenuFadeIn ();
		}
	}
	return true;
}
MENU_LISTENER(QuitGame)
{
	FString endString = gameinfo.QuitMessages[M_Random()%gameinfo.QuitMessages.Size()];
	if(endString[0] == '$')
		endString = language[endString.Mid(1)];

	if(Confirm(endString))
	{
		VW_UpdateScreen();
		SD_MusicOff();
		SD_StopSound();
		if(!menusAreFaded)
			MenuFadeOut();
		else
			VW_FadeOut();
		Quit();
	}

	// special case
	if(which != -1)
		mainMenu.draw();
	return false;
}
MENU_LISTENER(SetSoundEffects)
{
	SDMode modes[3] = { sdm_Off, sdm_PC, sdm_AdLib };
	if(SoundMode != modes[which])
	{
		SD_WaitSoundDone();
		SD_SetSoundMode(modes[which]);
	}
	return true;
}
MENU_LISTENER(SetDigitalSound)
{
	if(DigiMode != (which == 0 ? sds_Off : sds_SoundBlaster))
		SD_SetDigiDevice(which == 0 ? sds_Off : sds_SoundBlaster);
	return true;
}
MENU_LISTENER(SetMusic)
{
	if(MusicMode != (SMMode)which)
	{
		SD_SetMusicMode((SMMode)which);
		if(which != smm_Off)
			StartCPMusic(gameinfo.MenuMusic);
	}
	return true;
}
MENU_LISTENER(EnterControlBase)
{
	controlBase[2]->setEnabled(mouseenabled);
	controlBase[3]->setEnabled(mouseenabled);
	controlBase[4]->setEnabled(mouseenabled);
	controlBase[5]->setEnabled(IN_JoyPresent());
	controlBase[6]->setEnabled(IN_JoyPresent() && joystickenabled);
	controlBase.draw();

	IN_AdjustMouse();

	return true;
}

MENU_LISTENER(SetPlayerClassAndSwitch)
{
	playerClass = gameinfo.PlayerClasses[which];

	return true;
}
MENU_LISTENER(SetPlayerClassAndJoin)
{
	SetPlayerClassAndSwitch(which);
	return JoinNetGame(which);
}
MENU_LISTENER(SetEpisodeAndSwitchToSkill)
{
	EpisodeInfo &ep = EpisodeInfo::GetEpisode(which);

	if(!GameMap::CheckMapExists(ep.StartMap))
	{
		SD_PlaySound("player/usefail");
		Message("Please select \"Read This!\"\n"
				"from the Options menu to\n"
				"find out how to order this\n" "episode from Apogee.");
		IN_ClearKeysDown();
		IN_Ack(ACK_Local);
		episodes.draw();
		return false;
	}

	if(ingame)
	{
		if(!Confirm(language["CURGAME"]))
		{
			episodes.draw();
			return false;
		}
	}

	episode = &ep;
	return true;
}
MENU_LISTENER(StartNewGame)
{
	const SkillInfo &si = SkillInfo::GetSkill(which);
	if(si.MustConfirm.IsNotEmpty())
	{
		if(!Confirm(si.MustConfirm))
			return false;
	}

	if(episode == NULL)
		episode = &EpisodeInfo::GetEpisode(0);

	Menu::closeMenus();
	NewGame(which, episode->StartMap, true, playerClass);

	//
	// CHANGE "READ THIS!" TO NORMAL COLOR
	//
	readThis->setHighlighted(false);

	return true;
}
MENU_LISTENER(JoinNetGame)
{
	Menu::closeMenus();
	NewGame(0, "", true);

	//
	// CHANGE "READ THIS!" TO NORMAL COLOR
	//
	readThis->setHighlighted(false);

	return true;
}
MENU_LISTENER(ReadThis)
{
	MenuFadeOut();
	StartCPMusic(gameinfo.FinaleMusic);
	HelpScreens();
	StartCPMusic(gameinfo.MenuMusic);
	mainMenu.draw();
	MenuFadeIn();
	return true;
}
MENU_LISTENER(ToggleFullscreen)
{
	VL_SetFullscreen(vid_fullscreen);
	displayMenu.draw();

	IN_AdjustMouse();

	return true;
}
MENU_LISTENER(ToggleVsync)
{
	screen->SetVSync(vid_vsync);
	return true;
}
MENU_LISTENER(SetAspectRatio)
{
	vid_aspect = static_cast<Aspect>(which);
	r_ratio = static_cast<Aspect>(CheckRatio(screenWidth, screenHeight));
	NewViewSize(viewsize);
	displayMenu.draw();
	return true;
}

// Dummy screen sizes to pass when windowed
MENU_LISTENER(EnterResolutionSelection);
MENU_LISTENER(SetResolution)
{
	MenuFadeOut();

	{
		int width, height;
		bool lb;
		Video->StartModeIterator(DisplayBits, vid_fullscreen);
		for(int i = 0;i <= which;++i)
			Video->NextMode(&width, &height, &lb);
		screenWidth = width;
		screenHeight = height;

		if(vid_fullscreen)
		{
			fullScreenWidth = screenWidth;
			fullScreenHeight = screenHeight;
		}
		else
		{
			windowedScreenWidth = screenWidth;
			windowedScreenHeight = screenHeight;
		}
	}

	r_ratio = static_cast<Aspect>(CheckRatio(screenWidth, screenHeight));
	VH_Startup(); // Recalculate fizzlefade stuff.
	VL_SetVGAPlaneMode();
	EnterResolutionSelection(which);
	resolutionMenu.draw();
	MenuFadeIn();
	return true;
}
MENU_LISTENER(EnterResolutionSelection)
{
	int selected = 0;
	resolutionMenu.clear();
	FString resolution;

	{
		int width, height;
		bool lb;
		Video->StartModeIterator(DisplayBits, vid_fullscreen);
		while(Video->NextMode(&width, &height, &lb))
		{
			resolution.Format("%dx%d", width, height);
			MenuItem *item = new MenuItem(resolution, SetResolution);
			resolutionMenu.addItem(item);

			if(width == SCREENWIDTH && height == SCREENHEIGHT)
			{
				selected = resolutionMenu.countItems()-1;
				item->setHighlighted(true);
			}
		}
	}

	resolutionMenu.setCurrentPosition(selected);
	return true;
}

MENU_LISTENER(ChangeAutomapFlag)
{
	AM_UpdateFlags();
	return true;
}
MENU_LISTENER(ChangeAMOverlay)
{
	am_overlay = which;
	AM_UpdateFlags();
	return true;
}
MENU_LISTENER(ChangeAMRotate)
{
	am_rotate = which;
	AM_UpdateFlags();
	return true;
}
MENU_LISTENER(AdjustViewSize)
{
	NewViewSize(viewsize);
	return true;
}

void CreateMenus()
{
	// HACK: Determine menu style by IWAD
	if(IWad::CheckGameFilter("Blake"))
		MenuStyle = MENUSTYLE_Blake;

	// Extract the palette
	BORDCOLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[0]), GPART(gameinfo.MenuColors[0]), BPART(gameinfo.MenuColors[0]));
	BORD2COLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[1]), GPART(gameinfo.MenuColors[1]), BPART(gameinfo.MenuColors[1]));
	BORD3COLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[2]), GPART(gameinfo.MenuColors[2]), BPART(gameinfo.MenuColors[2]));
	BKGDCOLOR = ColorMatcher.Pick(RPART(gameinfo.MenuColors[3]), GPART(gameinfo.MenuColors[3]), BPART(gameinfo.MenuColors[3]));
	STRIPE = ColorMatcher.Pick(RPART(gameinfo.MenuColors[4]), GPART(gameinfo.MenuColors[4]), BPART(gameinfo.MenuColors[4]));
	STRIPEBG = ColorMatcher.Pick(RPART(gameinfo.MenuColors[5]), GPART(gameinfo.MenuColors[5]), BPART(gameinfo.MenuColors[5]));
	MENUWIN_BACKGROUND = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[0]), GPART(gameinfo.MenuWindowColors[0]), BPART(gameinfo.MenuWindowColors[0])),
	MENUWIN_TOPBORDER = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[1]), GPART(gameinfo.MenuWindowColors[1]), BPART(gameinfo.MenuWindowColors[1])),
	MENUWIN_BOTBORDER = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[2]), GPART(gameinfo.MenuWindowColors[2]), BPART(gameinfo.MenuWindowColors[2])),
	MENUWINHGLT_BACKGROUND = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[3]), GPART(gameinfo.MenuWindowColors[3]), BPART(gameinfo.MenuWindowColors[3])),
	MENUWINHGLT_TOPBORDER = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[4]), GPART(gameinfo.MenuWindowColors[4]), BPART(gameinfo.MenuWindowColors[4])),
	MENUWINHGLT_BOTBORDER = ColorMatcher.Pick(RPART(gameinfo.MenuWindowColors[5]), GPART(gameinfo.MenuWindowColors[5]), BPART(gameinfo.MenuWindowColors[5]));

	// Actually initialize the menus
	GameSave::InitMenus();

	mainMenu.setHeadPicture("M_OPTION");

	const bool useEpisodeMenu = EpisodeInfo::GetNumEpisodes() > 1;
	if(gameinfo.PlayerClasses.Size() > 1)
		mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_NG"], playerClasses));
	else if(!Net::IsArbiter())
		mainMenu.addItem(new MenuItem(language["STR_NG"], JoinNetGame));
	else if(useEpisodeMenu)
		mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_NG"], episodes));
	else
		mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_NG"], skills));

	mainMenu.addItem(new MenuSwitcherMenuItem(language["STR_OPTIONS"], optionsMenu));
	mainMenu.addItem(GameSave::GetLoadMenuItem());
	mainMenu.addItem(GameSave::GetSaveMenuItem());
	readThis = new MenuItem(language["STR_RT"], ReadThis);
	readThis->setVisible(gameinfo.DrawReadThis);
	readThis->setHighlighted(true);
	mainMenu.addItem(readThis);
	mainMenu.addItem(new MenuItem(language["STR_VS"], ViewScoresOrEndGame));
	mainMenu.addItem(new MenuItem(language["STR_BD"], PlayDemosOrReturnToGame));
	mainMenu.addItem(new MenuItem(language["STR_QT"], QuitGame));

	playerClasses.setHeadText(language["STR_PLAYERCLASS"]);
	for(unsigned int i = 0;i < gameinfo.PlayerClasses.Size();++i)
	{
		const ClassDef *cls = ClassDef::FindClass(gameinfo.PlayerClasses[i]);
		const char* displayName = cls->Meta.GetMetaString(APMETA_DisplayName);
		if(!displayName)
			I_FatalError("Player class %s has no display name.", cls->GetName().GetChars());
		if(Net::IsArbiter())
			playerClasses.addItem(new MenuSwitcherMenuItem(displayName, useEpisodeMenu ? episodes : skills, SetPlayerClassAndSwitch));
		else
			playerClasses.addItem(new MenuItem(displayName, SetPlayerClassAndJoin));
	}

	episodes.setHeadText(language["STR_WHICHEPISODE"]);
	for(unsigned int i = 0;i < EpisodeInfo::GetNumEpisodes();++i)
	{
		EpisodeInfo &episode = EpisodeInfo::GetEpisode(i);
		MenuItem *tmp = new MenuSwitcherMenuItem(episode.EpisodeName, skills, SetEpisodeAndSwitchToSkill);
		if(!episode.EpisodePicture.IsEmpty())
			tmp->setPicture(episode.EpisodePicture);
		if(!GameMap::CheckMapExists(episode.StartMap))
			tmp->setHighlighted(2);
		episodes.addItem(tmp);
	}

	skills.setHeadText(language["STR_HOWTOUGH"]);
	skills.setHeadPicture("M_HOWTGH", true);
	for(unsigned int i = 0;i < SkillInfo::GetNumSkills();++i)
	{
		SkillInfo &skill = SkillInfo::GetSkill(i);
		MenuItem *tmp = new MenuItem(skill.Name, StartNewGame);
		if(!skill.SkillPicture.IsEmpty())
			tmp->setPicture(skill.SkillPicture, NM_X + 185, NM_Y + 7);
		skills.addItem(tmp);
	}
	skills.setCurrentPosition(2);

	optionsMenu.setHeadPicture("M_OPTION");
	optionsMenu.addItem(new MenuSwitcherMenuItem(language["STR_CL"], controlBase));
	optionsMenu.addItem(new MenuSwitcherMenuItem(language["STR_SD"], soundBase));
	optionsMenu.addItem(new MenuSwitcherMenuItem(language["STR_DISPLAY"], displayMenu));
	optionsMenu.addItem(new MenuSwitcherMenuItem(language["STR_AMOPTIONS"], automapMenu));

	// Collect options and defaults
	const char* soundEffectsOptions[] = {language["STR_NONE"], language["STR_PC"], language["STR_ALSB"] };
	const char* digitizedOptions[] = {language["STR_NONE"], language["STR_SB"] };
	const char* musicOptions[] = { language["STR_NONE"], language["STR_ALSB"], language["STR_MIDI"] };
	if(!AdLibPresent && !SoundBlasterPresent)
	{
		soundEffectsOptions[2] = NULL;
		musicOptions[1] = NULL;
	}
	if(!SoundBlasterPresent)
		digitizedOptions[1] = NULL;
	int soundEffectsMode = 0;
	int digitizedMode = 0;
	int musicMode = 0;
	switch(SoundMode)
	{
		default: soundEffectsMode = 0; break;
		case sdm_PC: soundEffectsMode = 1; break;
		case sdm_AdLib: soundEffectsMode = 2; break;
	}
	switch(DigiMode)
	{
		default: digitizedMode = 0; break;
		case sds_SoundBlaster: digitizedMode = 1; break;
	}
	switch(MusicMode)
	{
		default: musicMode = 0; break;
		case smm_AdLib: musicMode = 1; break;
		case smm_Midi: musicMode = 2; break;
	}
	soundBase.setHeadText(language["STR_SOUNDCONFIG"]);
	soundBase.addItem(new LabelMenuItem(language["STR_DIGITALDEVICE"]));
	soundBase.addItem(new MultipleChoiceMenuItem(SetDigitalSound, digitizedOptions, 2, digitizedMode));
	soundBase.addItem(new SliderMenuItem(SoundVolume, 150, MAX_VOLUME, language["STR_SOFT"], language["STR_LOUD"]));
	soundBase.addItem(new LabelMenuItem(language["STR_ADLIBDEVICE"]));
	soundBase.addItem(new MultipleChoiceMenuItem(SetSoundEffects, soundEffectsOptions, 3, soundEffectsMode));
	soundBase.addItem(new SliderMenuItem(AdlibVolume, 150, MAX_VOLUME, language["STR_SOFT"], language["STR_LOUD"], SD_UpdatePCSpeakerVolume));
	soundBase.addItem(new LabelMenuItem(language["STR_MUSICDEVICE"]));
	soundBase.addItem(new MultipleChoiceMenuItem(SetMusic, musicOptions, 3, musicMode));
	soundBase.addItem(new SliderMenuItem(MusicVolume, 150, MAX_VOLUME, language["STR_SOFT"], language["STR_LOUD"], SD_UpdateMusicVolume));

	controlBase.setHeadPicture("M_CONTRL");
	controlBase.addItem(new BooleanMenuItem(language["STR_ALWAYSRUN"], alwaysrun, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(language["STR_MOUSEEN"], mouseenabled, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(language["STR_WINDOWEDMOUSE"], forcegrabmouse, EnterControlBase));
	controlBase.addItem(new BooleanMenuItem(language["STR_DISABLEYAXIS"], mouseyaxisdisabled, EnterControlBase));
	controlBase.addItem(new MenuSwitcherMenuItem(language["STR_SENS"], mouseSensitivity));
	controlBase.addItem(new BooleanMenuItem(language["STR_JOYEN"], joystickenabled, EnterControlBase));
	controlBase.addItem(new MenuSwitcherMenuItem(language["STR_JOYSENS"], joySensitivity));
	controlBase.addItem(new MenuSwitcherMenuItem(language["STR_CUSTOM"], controls));
	controlBase.addItem(new BooleanMenuItem(language["STR_ESCQUIT"], quitonescape));

	joySensitivity.setHeadText(language["STR_JOYSENS"]);
	for(int i = 0;i < JoyNumAxes;++i)
	{
		FString label;
		if(i < 4)
		{
			static const char AxisNames[4] = { 'X', 'Y', 'Z', 'R' };
			label.Format("%c Axis", AxisNames[i]);
		}
		else
			label.Format("Axis %d", i+1);

		joySensitivity.addItem(new LabelMenuItem(label));
		joySensitivity.addItem(new SliderMenuItem(JoySensitivity[i].sensitivity, 164, 30, language["STR_SLOW"], language["STR_FAST"]));
		joySensitivity.addItem(new SliderMenuItem(JoySensitivity[i].deadzone, 150, 20, language["STR_SMALL"], language["STR_LARGE"]));
	}

	const char* aspectOptions[] = {"Aspect: Auto", "Aspect: 16:9", "Aspect: 16:10", "Aspect: 17:10", "Aspect: 4:3", "Aspect: 5:4", "Aspect: 21:9", "Aspect: 32:9"};
	displayMenu.setHeadText(language["STR_DISPLAY"]);
#ifndef __ANDROID__
	displayMenu.addItem(new BooleanMenuItem(language["STR_FULLSCREEN"], vid_fullscreen, ToggleFullscreen));
#endif
#if SDL_VERSION_ATLEAST(2,0,0)
	displayMenu.addItem(new BooleanMenuItem(language["STR_VSYNC"], vid_vsync, ToggleVsync));
#endif
	displayMenu.addItem(new MultipleChoiceMenuItem(SetAspectRatio, aspectOptions, 8, vid_aspect));
	displayMenu.addItem(new MenuSwitcherMenuItem(language["STR_SELECTRES"], resolutionMenu, EnterResolutionSelection));
	displayMenu.addItem(new LabelMenuItem(language["STR_SCREENSIZE"]));
	displayMenu.addItem(new SliderMenuItem(viewsize, 110, 21, language["STR_SMALL"], language["STR_LARGE"], AdjustViewSize));

	resolutionMenu.setHeadText(language["STR_SELECTRES"]);

	mouseSensitivity.setHeadText(language["STR_MOUSEADJ"]);
	mouseSensitivity.addItem(new LabelMenuItem(language["STR_MOUSEXADJ"]));
	mouseSensitivity.addItem(new SliderMenuItem(mousexadjustment, 173, 20, language["STR_SLOW"], language["STR_FAST"]));
	mouseSensitivity.addItem(new LabelMenuItem(language["STR_MOUSEYADJ"]));
	mouseSensitivity.addItem(new SliderMenuItem(mouseyadjustment, 173, 20, language["STR_SLOW"], language["STR_FAST"]));

	mouseSensitivity.addItem(new LabelMenuItem(language["STR_PANXADJ"]));
	mouseSensitivity.addItem(new SliderMenuItem(panxadjustment, 173, 20, language["STR_SLOW"], language["STR_FAST"]));
	mouseSensitivity.addItem(new LabelMenuItem(language["STR_PANYADJ"]));
	mouseSensitivity.addItem(new SliderMenuItem(panyadjustment, 173, 20, language["STR_SLOW"], language["STR_FAST"]));


	controls.setHeadPicture("M_CUSTOM");
	controls.showControlHeaders(true);
	for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
	{
		controls.addItem(new ControlMenuItem(controlScheme[i]));
	}

	const char* rotateOptions[] = { language["STR_AMROTATEOFF"], language["STR_AMROTATEON"], language["STR_AMROTATEOVERLAY"] };
	const char* overlayOptions[] = { language["STR_AMOVERLAYOFF"], language["STR_AMOVERLAYON"], language["STR_AMOVERLAYBOTH"] };
	automapMenu.setHeadText(language["STR_AMOPTIONS"]);
	automapMenu.addItem(new MultipleChoiceMenuItem(ChangeAMOverlay, overlayOptions, 3, am_overlay));
	automapMenu.addItem(new MultipleChoiceMenuItem(ChangeAMRotate, rotateOptions, 3, am_rotate));
	automapMenu.addItem(new BooleanMenuItem(language["STR_AMTEXTURES"], am_drawtexturedwalls, ChangeAutomapFlag));
	automapMenu.addItem(new BooleanMenuItem(language["STR_AMFLOORS"], am_drawfloors, ChangeAutomapFlag));
	automapMenu.addItem(new BooleanMenuItem(language["STR_AMTEXTUREDOVERLAY"], am_overlaytextured, ChangeAutomapFlag));
	automapMenu.addItem(new BooleanMenuItem(language["STR_AMRATIOS"], am_showratios, ChangeAutomapFlag));
	automapMenu.addItem(new BooleanMenuItem(language["STR_AMPAUSE"], am_pause, ChangeAutomapFlag));
}

////////////////////////////////////////////////////////////////////
//
// Wolfenstein Control Panel!  Ta Da!
//
////////////////////////////////////////////////////////////////////
void US_ControlPanel (ScanCode scancode)
{
	int which;
	bool idEasterEgg = Wads.CheckNumForName("IDGUYPAL") != -1;

	if (!Net::IsArbiter())
	{
		// Disable functions that should only be available to arbiter
		switch(scancode)
		{
			case sc_F2:
			case sc_F3:
			case sc_F7:
			case sc_F8:
			case sc_F9:
				return;
			default:
				break;
		}
	}

	if (Net::InitVars.mode != Net::MODE_SinglePlayer)
	{
		// At this time we don't support saves in multiplayer
		switch(scancode)
		{
			case sc_F2:
			case sc_F3:
			case sc_F8:
			case sc_F9:
				return;
			default:
				break;
		}
	}

	if (ingame)
	{
		if (CP_CheckQuick (scancode))
			return;
		lastgamemusicoffset = StartCPMusic (gameinfo.MenuMusic);

		Net::BlockPlaysim();

		VW_FadeOut();
	}
	else
		StartCPMusic (gameinfo.MenuMusic);
	SetupControlPanel ();

	//
	// F-KEYS FROM WITHIN GAME
	//
	Menu::closeMenus(false);
	switch (scancode)
	{
		case sc_F1:
			HelpScreens ();
			goto finishup;

		case sc_F2:
			GameSave::GetSaveMenu().show();
			goto finishup;

		case sc_F3:
			GameSave::GetLoadMenu().show();
			goto finishup;

		case sc_F4:
			soundBase.show();
			goto finishup;

		case sc_F5:
			displayMenu.show();
			goto finishup;

		case sc_F6:
			controlBase.show ();
			goto finishup;

		finishup:
			CleanupControlPanel ();
			return;

		default:
			break;
	}

	if(ingame)
	{
		mainMenu[0]->setEnabled(Net::InitVars.mode == Net::MODE_SinglePlayer); // Require explicit end game for net games
		mainMenu[mainMenu.countItems()-3]->setText(language["STR_EG"]);
		mainMenu[mainMenu.countItems()-3]->setEnabled(Net::IsArbiter());
		mainMenu[mainMenu.countItems()-2]->setText(language["STR_BG"]);
		mainMenu[mainMenu.countItems()-2]->setEnabled(true);
		mainMenu[mainMenu.countItems()-2]->setHighlighted(true);
		mainMenu[3]->setEnabled(Net::InitVars.mode == Net::MODE_SinglePlayer && players[ConsolePlayer].state != player_t::PST_DEAD);
	}
	else
	{
		mainMenu[0]->setEnabled(true);
		if (gameinfo.TrackHighScores == true && Net::InitVars.mode == Net::MODE_SinglePlayer)
		{
			mainMenu[mainMenu.countItems()-3]->setText(language["STR_VS"]);
			mainMenu[mainMenu.countItems()-3]->setEnabled(true);
		}
		else
		{
			mainMenu[mainMenu.countItems()-3]->setText(language["STR_EG"]);
			mainMenu[mainMenu.countItems()-3]->setEnabled(false);
		}
		mainMenu[mainMenu.countItems()-2]->setText(language["STR_BD"]);
		mainMenu[mainMenu.countItems()-2]->setEnabled(Net::InitVars.mode == Net::MODE_SinglePlayer);
		mainMenu[mainMenu.countItems()-2]->setHighlighted(false);
		mainMenu[3]->setEnabled(false);
	}
	mainMenu.validateCurPos();
	mainMenu.draw();
	MenuFadeIn ();
	Menu::closeMenus(false);

	//
	// MAIN MENU LOOP
	//
	do
	{
		which = mainMenu.handle();

		if(idEasterEgg)
		{
			IN_ProcessEvents();

			//
			// EASTER EGG FOR SPEAR OF DESTINY!
			//
			if (Keyboard[sc_I] && Keyboard[sc_D])
			{
				MenuFadeOut ();
				StartCPMusic ("XJAZNAZI");
				VL_ReadPalette("IDGUYPAL");

				CA_CacheScreen(TexMan("IDGUYS"));

				VW_UpdateScreen ();

				VW_FadeIn();

				while (Keyboard[sc_I] || Keyboard[sc_D])
					IN_WaitAndProcessEvents();
				IN_ClearKeysDown ();
				IN_Ack (ACK_Local);

				VW_FadeOut ();
				VL_ReadPalette(gameinfo.GamePalette);

				mainMenu.draw();
				StartCPMusic (gameinfo.MenuMusic);
				MenuFadeIn ();
			}
		}

		switch (which)
		{
			case -1:
				if(!ingame || quitonescape)
					QuitGame(0);
				else
					PlayDemosOrReturnToGame(0);
				break;
			default:
				break;
		}

		//
		// "EXIT OPTIONS" OR "NEW GAME" EXITS
		//
	}
	while (!Menu::areMenusClosed());

	//
	// DEALLOCATE EVERYTHING
	//
	CleanupControlPanel ();

	// RETURN/START GAME EXECUTION
}

////////////////////////////////////////////////////////////////////
//
// CHECK QUICK-KEYS & QUIT (WHILE IN A GAME)
//
////////////////////////////////////////////////////////////////////
int CP_CheckQuick (ScanCode scancode)
{
	switch (scancode)
	{
		// Check to see if we have anything to open
		case sc_F1:
			if(Wads.CheckNumForName("HELPART", ns_global) == -1)
				return 1;
			break;

		// Disable save if dead
		case sc_F2:
			if(players[ConsolePlayer].state == player_t::PST_DEAD)
				return 1;
			break;

		//
		// END GAME
		//
		case sc_F7:
			WindowH = 160;
			CP_EndGame(0);

			DrawPlayScreen();
			WindowH = 200;
			return 1;

		//
		// QUICKSAVE
		//
		case sc_F8:
			if(players[ConsolePlayer].state != player_t::PST_DEAD)
				GameSave::QuickLoadOrSave(false);
			return 1;

		//
		// QUICKLOAD
		//
		case sc_F9:
			GameSave::QuickLoadOrSave(true);
			return 1;

		//
		// QUIT
		//
		case sc_F10:
			WindowX = WindowY = 0;
			WindowW = 320;
			WindowH = 160;
			QuitGame(-1);

			DrawPlayScreen ();
			WindowH = 200;
			return 1;
	}

	return 0;
}


////////////////////////////////////////////////////////////////////
//
// END THE CURRENT GAME
//
////////////////////////////////////////////////////////////////////
int CP_EndGame (int)
{
	int res;
	res = Confirm (language["ENDGAMESTR"]);
	if (!ingame)
		mainMenu.draw();
	if(!res) return 0;

	Net::EndGame();
	return 1;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
//
// SUPPORT ROUTINES
//
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////
//
// Clear Menu screens to dark red
//
////////////////////////////////////////////////////////////////////
void ClearMScreen (void)
{
	static FTextureID backdropID = TexMan.CheckForTexture("BACKDROP", FTexture::TEX_Any);
	if(!backdropID.isValid())
		VWB_Clear (BORDCOLOR, 0, 0, screenWidth, screenHeight);
	else
		CA_CacheScreen(TexMan(backdropID), true);
}


////////////////////////////////////////////////////////////////////
//
// Draw a window for a menu
//
////////////////////////////////////////////////////////////////////
void DrawWindow (int x, int y, int w, int h, int wcolor, int color1, int color2)
{
	int wx = x, wy = y, ww = w, wh = h;
	MenuToRealCoords(wx, wy, ww, wh, MENU_CENTER);

	VWB_Clear (wcolor, wx, wy, wx+ww, wy+wh);
	DrawOutline (x, y, w, h, color1, color2);
}

void DrawOutline (int x, int y, int w, int h, int color1, int color2)
{
	MenuToRealCoords(x, y, w, h, MENU_CENTER);

	VWB_Clear(color2, x-scaleFactorX, y, x+w+scaleFactorX, y+scaleFactorY);
	VWB_Clear(color2, x-scaleFactorX, y, x, y+h);
	VWB_Clear(color1, x-scaleFactorX, y+h, x+w+scaleFactorX, y+h+scaleFactorY);
	VWB_Clear(color1, x+w, y, x+w+scaleFactorX, y+h);
}

////////////////////////////////////////////////////////////////////
//
// Setup Control Panel stuff - graphics, etc.
//
////////////////////////////////////////////////////////////////////
void SetupControlPanel (void)
{
	WindowH = 200;
	if(screenHeight % 200 != 0)
		VL_ClearScreen(0);

	//
	// CENTER MOUSE
	//
	if(IN_IsInputGrabbed())
		IN_CenterMouse();
}

////////////////////////////////////////////////////////////////////
//
// Clean up all the Control Panel stuff
//
////////////////////////////////////////////////////////////////////
void CleanupControlPanel (void)
{
	VWB_Clear(ColorMatcher.Pick(RPART(gameinfo.MenuFadeColor), GPART(gameinfo.MenuFadeColor), BPART(gameinfo.MenuFadeColor)),
		0, 0, screenWidth, screenHeight);
}

////////////////////////////////////////////////////////////////////
//
// DELAY FOR AN AMOUNT OF TICS OR UNTIL CONTROLS ARE INACTIVE
//
////////////////////////////////////////////////////////////////////
void TicDelay (int count)
{
	ControlInfo ci;

	int32_t startTime = GetTimeCount ();
	do
	{
		SDL_Delay(5);
		ReadAnyControl (&ci);
	}
	while ((int32_t) GetTimeCount () - startTime < count && ci.dir != dir_None);
}

////////////////////////////////////////////////////////////////////
//
// WAIT FOR CTRLKEY-UP OR BUTTON-UP
//
////////////////////////////////////////////////////////////////////
void WaitKeyUp (void)
{
	ControlInfo ci;
	while (ReadAnyControl (&ci), ci.button0 |
		ci.button1 |
		ci.button2 | ci.button3 | Keyboard[sc_Space] | Keyboard[sc_Enter] | Keyboard[sc_Escape])
	{
		IN_WaitAndProcessEvents();
	}
}


////////////////////////////////////////////////////////////////////
//
// READ KEYBOARD, JOYSTICK AND MOUSE FOR INPUT
//
////////////////////////////////////////////////////////////////////

// Store relative mouse movement until menu changes.
static int menumousex, menumousey;

void ReadAnyControl (ControlInfo * ci)
{
	int mouseactive = 0;

	IN_ReadControl (0, ci);

	if (mouseenabled && IN_IsInputGrabbed())
	{
		int mousex, mousey, buttons;
		buttons = SDL_GetRelativeMouseState(&mousex, &mousey);
		menumousex += mousex;
		menumousey += mousey;

		int middlePressed = buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE);
		int rightPressed = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
		buttons &= ~(SDL_BUTTON(SDL_BUTTON_MIDDLE) | SDL_BUTTON(SDL_BUTTON_RIGHT));
		if(middlePressed) buttons |= 1 << 2;
		if(rightPressed) buttons |= 1 << 1;

		if(menumousey < -SENSITIVE)
		{
			ci->dir = dir_North;
			mouseactive = 1;
		}
		else if(menumousey > SENSITIVE)
		{
			ci->dir = dir_South;
			mouseactive = 1;
		}

		if(menumousex < -SENSITIVE)
		{
			ci->dir = dir_West;
			mouseactive = 1;
		}
		else if(menumousex > SENSITIVE)
		{
			ci->dir = dir_East;
			mouseactive = 1;
		}

		if(mouseactive)
			menumousex = menumousey = 0;

		if (buttons)
		{
			ci->button0 = !!(buttons & 1);
			ci->button1 = !!(buttons & 2);
			ci->button2 = !!(buttons & 4);
			ci->button3 = false;
			mouseactive = 1;
		}
	}

	if (joystickenabled && !mouseactive)
	{
		int jx, jy, jb;

		IN_GetJoyDelta (&jx, &jy);
		if (jy < -SENSITIVE)
			ci->dir = dir_North;
		else if (jy > SENSITIVE)
			ci->dir = dir_South;

		if (jx < -SENSITIVE)
			ci->dir = dir_West;
		else if (jx > SENSITIVE)
			ci->dir = dir_East;

		jb = IN_JoyButtons ();
		if (jb)
		{
			ci->button0 = !!(jb & 1);
			ci->button1 = !!(jb & 2);
			ci->button2 = !!(jb & 4);
			ci->button3 = !!(jb & 8);
		}
	}
}

#ifdef __ANDROID__
extern  bool inConfirm;
#endif
////////////////////////////////////////////////////////////////////
//
// DRAW DIALOG AND CONFIRM YES OR NO TO QUESTION
//
////////////////////////////////////////////////////////////////////
bool Confirm (const char *string)
{
	bool xit = false;
	int x, y, tick = 0, lastBlinkTime;
	const char* whichsnd[2] = { "menu/escape", "menu/activate" };
	ControlInfo ci;

#ifdef __ANDROID__
	inConfirm = true;
#endif

	Message (string);
	IN_ClearKeysDown ();
	WaitKeyUp ();

	//
	// BLINK CURSOR
	//
	x = PrintX;
	y = PrintY;
	lastBlinkTime = GetTimeCount();

	do
	{
		ReadAnyControl(&ci);

		if (GetTimeCount() - lastBlinkTime >= 10)
		{
			switch (tick)
			{
				case 0:
				{
					double dx = x;
					double dy = y;
					double dw = 8;
					double dh = 13;
					MenuToRealCoords(dx, dy, dw, dh, MENU_CENTER);
					VWB_Clear(MENUWIN_BACKGROUND, (int)dx, (int)dy, (int)(dx+dw), (int)(dy+dh));
					break;
				}
				case 1:
					PrintX = x;
					PrintY = y;
					US_Print (BigFont, "_", gameinfo.FontColors[GameInfo::MESSAGEFONT]);
			}
			VW_UpdateScreen ();
			tick ^= 1;
			lastBlinkTime = GetTimeCount();
		}
		else SDL_Delay(5);

	}
	while (!Keyboard[sc_Y] && !Keyboard[sc_S] && !Keyboard[sc_N] && !Keyboard[sc_Escape] && !Keyboard[sc_Return] && !ci.button0 && !ci.button1);

	if (Keyboard[sc_S] || Keyboard[sc_Y] || Keyboard[sc_Return] || ci.button0)
	{
		xit = true;
		ShootSnd ();
	}

	IN_ClearKeysDown ();
	WaitKeyUp ();

	SD_PlaySound (whichsnd[xit]);

#ifdef __ANDROID__
	inConfirm = false;
#endif

	return xit;
}

////////////////////////////////////////////////////////////////////
//
// PRINT A MESSAGE IN A WINDOW
//
////////////////////////////////////////////////////////////////////
void Message (const char *string)
{
	static const int
		MESSAGE_BG = ColorMatcher.Pick(RPART(gameinfo.MessageColors[0]), GPART(gameinfo.MessageColors[0]), BPART(gameinfo.MessageColors[0])),
		TOPBRDR = ColorMatcher.Pick(RPART(gameinfo.MessageColors[1]), GPART(gameinfo.MessageColors[1]), BPART(gameinfo.MessageColors[1])),
		BOTBRDR = ColorMatcher.Pick(RPART(gameinfo.MessageColors[2]), GPART(gameinfo.MessageColors[2]), BPART(gameinfo.MessageColors[2]));

	word width, height;

	FString measureString;
	measureString.Format("%s_", string);
	VW_MeasurePropString(BigFont, measureString, width, height);
	width = MIN<int>(width, 320 - 10);
	height = MIN<int>(height, 200 - 10);

	PrintY = (WindowH / 2) - height / 2;
	PrintX = WindowX = 160 - width / 2;

	DrawWindow (WindowX - 5, PrintY - 5, width + 10, height + 10, MESSAGE_BG);
	DrawOutline (WindowX - 5, PrintY - 5, width + 10, height + 10, BOTBRDR, TOPBRDR);
	US_Print (BigFont, string, gameinfo.FontColors[GameInfo::MESSAGEFONT]);
	VW_UpdateScreen ();
}

////////////////////////////////////////////////////////////////////
//
// THIS MAY BE FIXED A LITTLE LATER...
//
////////////////////////////////////////////////////////////////////

int StartCPMusic (const char* song)
{
	int lastoffs;

	//lastmusic = song;
	lastoffs = SD_MusicOff ();

	SD_StartMusic(song);
	return lastoffs;
}

///////////////////////////////////////////////////////////////////////////
//
// CHECK FOR PAUSE KEY (FOR MUSIC ONLY)
//
///////////////////////////////////////////////////////////////////////////
void CheckPause (void)
{
	static int SoundStatus = 1;
	static int pauseofs = 0;
	if (LastScan == sc_Pause)
	{
		switch (SoundStatus)
		{
			case 0:
				SD_ContinueMusic(gameinfo.MenuMusic, pauseofs);
				break;
			case 1:
				pauseofs = SD_MusicOff();
				break;
		}

		SoundStatus ^= 1;
		VW_WaitVBL (3);
		IN_ClearKeysDown ();
	}
}

///////////////////////////////////////////////////////////////////////////
//
// DRAW SCREEN TITLE STRIPES
//
///////////////////////////////////////////////////////////////////////////
void DrawStripes (int y)
{
	static unsigned int calcStripes = INT_MAX;
	static unsigned int sy, sh;
	static unsigned int ly, lh;
	if(calcStripes != scaleFactorY)
	{
		unsigned int dummyx = 0, dummyw = 320;
		sy = y;
		sh = 24;
		ly = y+22;
		lh = 1;
		calcStripes = scaleFactorY;

		MenuToRealCoords(dummyx, sy, dummyw, sh, MENU_TOP);
		MenuToRealCoords(dummyx, ly, dummyw, lh, MENU_TOP);
	}

	VWB_Clear(STRIPEBG, 0, sy, screenWidth, sy+sh);
	VWB_Clear(STRIPE, 0, ly, screenWidth, ly+lh);
}

void ShootSnd (void)
{
	SD_PlaySound ("menu/activate");
}

void MenuFadeOut()
{
	assert(!menusAreFaded);
	menusAreFaded = true;

	VL_FadeOut(0, 255,
		RPART(gameinfo.MenuFadeColor), GPART(gameinfo.MenuFadeColor), BPART(gameinfo.MenuFadeColor),
		10);
}

void MenuFadeIn()
{
	assert(menusAreFaded);
	menusAreFaded = false;

	VL_FadeIn(0, 255, 10);
}

void ShowMenu(Menu &menu)
{
	// Clear out any residual mouse movement.
	menumousex = menumousey = 0;

	VW_FadeOut ();
	if(screenHeight % 200 != 0)
		VL_ClearScreen(0);

	lastgamemusicoffset = StartCPMusic (gameinfo.MenuMusic);
	Menu::closeMenus(false);
	menu.show();

	CleanupControlPanel();
	IN_ClearKeysDown ();
	VW_FadeOut();
	if(viewsize != 21)
		DrawPlayScreen ();

	if (!startgame && !loadedgame)
		ContinueMusic (lastgamemusicoffset);

	if (loadedgame)
		playstate = ex_abort;

	ResetTimeCount();

	if (MousePresent && IN_IsInputGrabbed())
		IN_CenterMouse();     // Clear accumulated mouse movement
}
