// WL_MAIN.C

#ifdef _WIN32
//	#include <io.h>
#else
	#include <unistd.h>
#endif

#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"
#include "wl_atmos.h"
#include "m_classes.h"
#include "m_random.h"
#include "config.h"
#include "w_wad.h"
#include "language.h"
#include "textures/textures.h"
#include "c_cvars.h"
#include "thingdef/thingdef.h"
#include "v_font.h"
#include "v_palette.h"
#include "v_video.h"
#include "r_data/colormaps.h"
#include "wl_agent.h"
#include "doomerrors.h"
#include "lumpremap.h"
#include "scanner.h"
#include "g_shared/a_keys.h"
#include "g_mapinfo.h"
#include "wl_draw.h"
#include "wl_inter.h"
#include "wl_iwad.h"
#include "wl_play.h"
#include "wl_game.h"
#include "wl_loadsave.h"
#include "wl_net.h"
#include "dobject.h"
#include "colormatcher.h"
#include "version.h"
#include "r_2d/r_main.h"
#include "filesys.h"
#include "g_conversation.h"
#include "g_intermission.h"
#include "state_machine.h"
#include "am_map.h"

#include <clocale>

/*
=============================================================================

							WOLFENSTEIN 3-D

						An Id Software production

							by John Carmack

=============================================================================
*/

/*
=============================================================================

							LOCAL CONSTANTS

=============================================================================
*/


#define FOCALLENGTH     (0x5700l)               // in global coordinates

#define VIEWWIDTH       256                     // size of view window
#define VIEWHEIGHT      144

/*
=============================================================================

							GLOBAL VARIABLES

=============================================================================
*/

//
// proejection variables
//
fixed    focallength;
fixed    focallengthy;
fixed    r_depthvisibility;
unsigned screenofs;
int      viewscreenx, viewscreeny;
int      viewwidth;
int      viewheight;
int      statusbarx;
int      statusbary1, statusbary2;
short    centerx;
short    centerxwide;
int      shootdelta;           // pixels away from centerx a target can be
fixed    scale;
fixed    pspritexscale;
fixed    pspriteyscale;
fixed    yaspect;
int32_t  heightnumerator;
extern  bool menusAreFaded;

bool	startgame;
bool	loadedgame;
int		panxadjustment;
int     panyadjustment;

//
// Command line parameter variables
//
bool param_nowait = false;
int     param_difficulty = 1;           // default is "normal"
const char* param_tedlevel = NULL;            // default is not to start a level
int     param_joystickindex = 0;

int     param_joystickhat = -1;
int     param_samplerate = 44100;
int     param_audiobuffer = 2048 / (44100 / param_samplerate);
static 	int32_t lastBlinkTime;

//===========================================================================

void State_Fade(wl_state_t *state, int start, int end, int red, int green, int blue, int steps)
{
	state->isFading = true;
	end <<= FRACBITS;
	start <<= FRACBITS;

	state->fadeStep = (end-start)/steps;
	state->fadeStart = start - state->fadeStep;
	state->fadeEnd = end;
	state->fadeCur = start;
	state->fadeRed = red;
	state->fadeGreen = green;
	state->fadeBlue = blue;
}

void State_FadeOut(wl_state_t *state, int start, int end, int steps) {
	State_Fade(state, start, end, 0, 0, 0, steps);
}

void State_FadeIn(wl_state_t *state, int start, int end, int steps) {
	if(screenfaded)
		State_Fade(state, end, start, state->fadeRed, state->fadeGreen,
			   state->fadeBlue, steps);
}

void State_Ack(wl_state_t *state) {
	IN_StartAck ();
	state->isInWait = true;
	state->waitCanBeAcked = true;
	state->wasAcked = false;
	state->waitCanTimeout = false;
}

void State_UserInput(wl_state_t *state, longword delay) {
	IN_StartAck ();
	state->isInWait = true;
	state->waitCanBeAcked = true;
	state->wasAcked = false;
	state->waitCanTimeout = true;
	state->ackTimeout = GetTimeCount() + delay;
}

void State_Delay(wl_state_t *state, longword delay) {
	state->isInWait = true;
	state->wasAcked = false;
	state->waitCanTimeout = true;
	state->waitCanBeAcked = false;
	state->ackTimeout = GetTimeCount() + delay;
}

//===========================================================================

/*
==========================
=
= ShutdownId
=
= Shuts down all ID_?? managers
=
==========================
*/

void ShutdownId (void)
{
	SD_Shutdown ();
	IN_Shutdown ();
}


//===========================================================================

/*
==================
=
= BuildTables
=
= Calculates:
=
= scale                 projection constant
= sintable/costable     overlapping fractional tables
=
==================
*/

const double radtoint = (double)(FINEANGLES/2/PI);

void BuildTables (void)
{
	//
	// calculate fine tangents
	//

	int i;
	for(i=0;i<FINEANGLES/8;i++)
	{
		double tang=tan((i+0.5)/radtoint);
		finetangent[i + FINEANGLES/2] = finetangent[i]=(fixed)(tang*FRACUNIT);
		finetangent[FINEANGLES/4-1-i]=(fixed)((1/tang)*FRACUNIT);
		finetangent[FINEANGLES/4+i]=-finetangent[FINEANGLES/4-1-i];
		finetangent[FINEANGLES/2-1-i]=-finetangent[i];
	}
	memcpy(finetangent + FINEANGLES/2, finetangent, sizeof(fixed)*ANG180);

	//
	// costable overlays sintable with a quarter phase shift
	// ANGLES is assumed to be divisable by four
	//

	float angle = 0;
	float anglestep = (float)(PI/2/ANG90);
	for(i=0; i<FINEANGLES; i++)
	{
		finesine[i]=fixed(FRACUNIT*sin(angle));
		angle+=anglestep;
	}
	memcpy(&finesine[FINEANGLES], finesine, FINEANGLES*sizeof(fixed)/4);

#if defined(USE_STARSKY) || defined(USE_RAIN) || defined(USE_SNOW)
	Init3DPoints();
#endif
}

//===========================================================================

void CalcVisibility(fixed vis)
{
	r_depthvisibility = FixedDiv(FixedMul((160*FRACUNIT),vis),focallengthy<<16);
}

/*
====================
=
= CalcProjection
=
= Uses focallength
=
====================
*/

void CalcProjection (int32_t focal)
{
	int     i;
	int    intang;
	int     halfview;
	double  facedist;

	const fixed projectionFOV = static_cast<fixed>((players[ConsolePlayer].FOV / 90.0f)*AspectCorrection[r_ratio].viewGlobal);

	// 0xFD17 is a magic number to convert the player's radius 0x5800 to FOCALLENGTH (0x5700)
	focallength = FixedMul(focal, 0xFD17);
	facedist = 2*FOCALLENGTH+0x100; // Used to be MINDIST (0x5800) which was 0x100 then the FOCALLENGTH (0x5700)
	halfview = viewwidth/2;                                 // half view in pixels
	focallengthy = centerx*yaspect/finetangent[FINEANGLES/2+(ANGLE_45>>ANGLETOFINESHIFT)];

	//
	// calculate scale value for vertical height calculations
	// and sprite x calculations
	//
	scale = (fixed) (viewwidth*facedist/projectionFOV);

	//
	// divide heightnumerator by a posts distance to get the posts height for
	// the heightbuffer.  The pixel height is height>>2
	//
	heightnumerator = FixedMul(((TILEGLOBAL*scale)>>6), yaspect);

	//
	// calculate the angle offset from view angle of each pixel's ray
	//

	for (i=0;i<=halfview;i++)
	{
		// start 1/2 pixel over, so viewangle bisects two middle pixels
		double tang = (((double)i+0.5)*projectionFOV)/viewwidth/facedist;
		double angle = atan(tang);
		intang = (int) (angle*radtoint);
		pixelangle[halfview-i] = intang;
		pixelangle[halfview-1+i] = -intang;
	}
}

//===========================================================================

#ifdef TODO
Menu musicMenu(CTL_X, CTL_Y-6, 280, 32);
static TArray<FString> songList;

MENU_LISTENER(ChangeMusic)
{
	StartCPMusic(songList[which]);
	for(unsigned int i = 0;i < songList.Size();++i)
		musicMenu[i]->setHighlighted(i == (unsigned)which);
	musicMenu.draw();
	return true;
}

void DoJukebox(void)
{
	IN_ClearKeysDown();
	if (!AdLibPresent && !SoundBlasterPresent)
		return;

	VW_FadeOut ();

	ClearMScreen ();
	musicMenu.setHeadText(language["ROBSJUKEBOX"], true);
	for(unsigned int i = 0;i < (unsigned)Wads.GetNumLumps();++i)
	{
		if(Wads.GetLumpNamespace(i) != ns_music)
			continue;

		FString langString;
		langString.Format("MUS_%s", Wads.GetLumpFullName(i));
		const char* trackName = language[langString];
		if(trackName == langString.GetChars())
			musicMenu.addItem(new MenuItem(Wads.GetLumpFullName(i), ChangeMusic));
		else
			musicMenu.addItem(new MenuItem(language[langString], ChangeMusic));
		songList.Push(Wads.GetLumpFullName(i));

	}
	musicMenu.show();
	return;
}
#endif

/*
==========================
=
= InitGame
=
= Load a few things right away
=
==========================
*/

static void CollectGC()
{
	GC::FullGC();
	GC::DelSoftRootHead();
}

static bool DrawStartupConsole(FString statusStr)
{
	// Window for printing text to the screen is (12,76), (308, 182)
	const int textWindowTop = 76 + 2*ConFont->GetHeight();
	const int textWindowHeight = 182-textWindowTop;

	const bool hasSignon = !gameinfo.SignonLump.IsEmpty();
	if(hasSignon)
		CA_CacheScreen(TexMan(gameinfo.SignonLump));
	else
		screen->Clear(0, 0, SCREENWIDTH, SCREENHEIGHT, GPalette.BlackIndex, 0);

	word width, height;

	static const char* const engineVersion = GAMENAME " " DOTVERSIONSTR_NOREV;
	VW_MeasurePropString(ConFont, engineVersion, width, height);
	px = 160-width/2;
	py = 76;
	VWB_DrawPropString(ConFont, engineVersion, CR_GRAY);

	FString engineMode;
	switch(Net::InitVars.mode)
	{
	case Net::MODE_SinglePlayer:
		engineMode = "Single player";
		break;
	case Net::MODE_Host:
		engineMode.Format("Hosting %d players", Net::InitVars.numPlayers);
		break;
	case Net::MODE_Client:
		engineMode = "Joining multiplayer";
		break;
	}
	VW_MeasurePropString(ConFont, engineMode, width, height);
	px = 160-width/2;
	py += ConFont->GetHeight();
	VWB_DrawPropString(ConFont, engineMode, CR_GRAY);

	VW_MeasurePropString(ConFont, statusStr, width, height);
	px = 160-width/2;
	py = textWindowTop + (textWindowHeight-height)/2;
	VWB_DrawPropString(ConFont, statusStr, CR_GRAY);

	VH_UpdateScreen();

	return hasSignon;
}

void I_ShutdownGraphics();
void UninitGame()
{
  R_DeinitColormaps();
  V_ClearFonts();
  // I_ShutdownGraphics needs to be run before the class definitions are unloaded.
  I_ShutdownGraphics();
  CollectGC();
  P_DeinitKeyMessages();
}

void InitGame()
{
	//
	// Mapinfo
	//

	V_InitFontColors();
	G_ParseMapInfo(true);

	//
	// Init texture manager
	//

	TexMan.Init();
	printf("VL_ReadPalette: Setting up the Palette...\n");
	VL_ReadPalette(gameinfo.GamePalette);
	GenerateLookupTables();
	atterm(R_DeinitColormaps);

	//
	// Fonts
	//
	V_InitFonts();
	atterm(V_ClearFonts);

//
// load in and lock down some basic chunks
//

	BuildTables ();          // trig tables

	// Setup a temporary window so if we have to terminate we don't do extra mode sets
	VL_SetVGAPlaneMode (true);
	DrawStartupConsole("Initializing game engine");

//
// Load Actors
//

	ClassDef::LoadActors();
	atterm(CollectGC);
	// I_ShutdownGraphics needs to be run before the class definitions are unloaded.
	atterm (I_ShutdownGraphics);

	// Parse non-gameinfo sections in MAPINFO
	G_ParseMapInfo(false);

//
// Fonts
//
	VH_Startup ();
	IN_Startup ();
	SD_Startup ();
	printf("US_Startup: Starting the User Manager.\n");

//
// Load Keys
//

	P_InitKeyMessages();
	atterm(P_DeinitKeyMessages);

//
// Finish with setting up through the config file.
//
	AM_UpdateFlags();

//
// Load the status bar
//
	CreateStatusBar();

//
// initialize the menusalcProjection
	printf("CreateMenus: Preparing the menu system...\n");
	CreateMenus();

//
// Load Noah's Ark quiz
//
	Dialog::LoadGlobalModule("NOAHQUIZ");
//
// Net game?
//
#ifdef TODO
	Net::Init(DrawStartupConsole);
#endif
//
// Finish signon screen
//
	VL_SetVGAPlaneMode();
	if(DrawStartupConsole("Initialization complete"))
	{
		if (!param_nowait)
			IN_UserInput(70*4);
	}
	else // Delay for a moment to allow the user to enter the jukebox if desired
		IN_UserInput(16);

//
// HOLDING DOWN 'M' KEY?
//
	IN_ProcessEvents();
#ifdef TODO
	if (Keyboard[sc_M])
		DoJukebox();
#endif
#ifdef NOTYET
	vdisp = (byte *) (0xa0000+PAGE1START);
	vbuf = (byte *) (0xa0000+PAGE2START);
#endif
}

//===========================================================================

/*
==========================
=
= SetViewSize
=
==========================
*/

static void SetViewSize (unsigned int screenWidth, unsigned int screenHeight)
{
	statusbarx = 0;
	if(AspectCorrection[r_ratio].isWide)
		statusbarx = screenWidth*(48-AspectCorrection[r_ratio].multiplier)/(48*2);

	if(StatusBar)
	{
		statusbary1 = StatusBar->GetHeight(true);
		statusbary2 = 200 - StatusBar->GetHeight(false);
	}
	else
	{
		statusbary1 = 0;
		statusbary2 = 200;
	}

	statusbary1 = statusbary1*screenHeight/200;
	if(AspectCorrection[r_ratio].tallscreen)
		statusbary2 = ((statusbary2 - 100)*screenHeight*3)/AspectCorrection[r_ratio].baseHeight + screenHeight/2
			+ (screenHeight - screenHeight*AspectCorrection[r_ratio].multiplier/48)/2;
	else
		statusbary2 = statusbary2*screenHeight/200;

	unsigned int width;
	unsigned int height;
	if(viewsize == 21)
	{
		width = screenWidth;
		height = screenHeight;
	}
	else if(viewsize == 20)
	{
		width = screenWidth;
		height = statusbary2-statusbary1;
	}
	else
	{
		width = screenWidth - (20-viewsize)*16*screenWidth/320;
		height = (statusbary2-statusbary1+1) - (20-viewsize)*8*screenHeight/200;
	}

	// Some code assumes these are even.
	viewwidth = width&~1;
	viewheight = height&~1;
	centerx = viewwidth/2-1;
	centerxwide = AspectCorrection[r_ratio].isWide ? CorrectWidthFactor(centerx) : centerx;
	// This should allow shooting within 9 degrees, but it's not perfect.
	shootdelta = ((viewwidth<<FRACBITS)/AspectCorrection[r_ratio].viewGlobal)/10;
	if((unsigned) viewheight == screenHeight)
		viewscreenx = viewscreeny = screenofs = 0;
	else
	{
		viewscreenx = (screenWidth-viewwidth) / 2;
		viewscreeny = (statusbary2+statusbary1-viewheight)/2;
		screenofs = viewscreeny*SCREENPITCH+viewscreenx;
	}

	int virtheight = screenHeight;
	int virtwidth = screenWidth;
	if(AspectCorrection[r_ratio].isWide)
		virtwidth = CorrectWidthFactor(virtwidth);
	else
		virtheight = CorrectWidthFactor(virtheight);
	yaspect = FixedMul((320<<FRACBITS)/200,(virtheight<<FRACBITS)/virtwidth);

	pspritexscale = (centerxwide<<FRACBITS)/160;
	pspriteyscale = FixedMul(pspritexscale, yaspect);

	//
	// calculate trace angles and projection constants
	//
	if(players[ConsolePlayer].mo)
		CalcProjection(players[ConsolePlayer].mo->radius);
	else
		CalcProjection (FOCALLENGTH);
}

void NewViewSize (int width, unsigned int scrWidth, unsigned int scrHeight)
{
	if(width < 4 || width > 21)
		return;

	viewsize = width;
	SetViewSize(scrWidth, scrHeight);
}



//===========================================================================


void I_Error(const char* format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	FString error;
	error.VFormat(format, vlist);
	va_end(vlist);

	throw CRecoverableError(error);
}

//==========================================================================

static bool DebugNetwork = false;

void NetDPrintf(const char* format, ...)
{
	if(!DebugNetwork)
		return;

	va_list vlist;
	va_start(vlist, format);
	vprintf(format, vlist);
	va_end(vlist);
}

//==========================================================================

/*
==================
=
= PG13
=
==================
*/

static void PG13 (void)
{
	if(gameinfo.AdvisoryPic.IsEmpty())
		return;

	BYTE color = ColorMatcher.Pick(RPART(gameinfo.AdvisoryColor), GPART(gameinfo.AdvisoryColor), BPART(gameinfo.AdvisoryColor));

	VWB_Clear(color, 0, 0, screenWidth, screenHeight);
	FTexture *tex = TexMan(gameinfo.AdvisoryPic);
	if(tex->GetScaledWidth() == 320)
		VWB_DrawGraphic(tex, 0, 100-tex->GetScaledHeight()/2);
	else
		VWB_DrawGraphic(tex, 304-tex->GetScaledWidth(), 174-tex->GetScaledHeight());
}

//===========================================================================

////////////////////////////////////////////////////////
//
// NON-SHAREWARE NOTICE
//
////////////////////////////////////////////////////////
static void NonShareware (void)
{
	ClearMScreen ();
	DrawStripes (10);

	PrintX = 110;
	PrintY = 15;

	pa = MENU_TOP;
	US_Print (BigFont, language["REGNOTICE_TITLE"], gameinfo.FontColors[GameInfo::MENU_HIGHLIGHTSELECTION]);
	pa = MENU_CENTER;

	WindowX = PrintX = 40;
	PrintY = 60;
	US_Print (BigFont, language["REGNOTICE_MESSAGE"], gameinfo.FontColors[GameInfo::MENU_SELECTION]);
}

//===========================================================================


/*
=====================
=
= DemoLoop
=
=====================
*/

//===========================================================================

// CheckRatio -- From ZDoom
//
// Tries to guess the physical dimensions of the screen based on the
// screen's pixel dimensions.
int CheckRatio (int width, int height, int *trueratio)
{
	int fakeratio = -1;
	Aspect ratio;

	if (vid_aspect != ASPECT_NONE)
	{
		// [SP] User wants to force aspect ratio; let them.
		fakeratio = vid_aspect;
	}
	/*if (vid_nowidescreen)
	{
		if (!vid_tft)
		{
			fakeratio = 0;
		}
		else
		{
			fakeratio = (height * 5/4 == width) ? 4 : 0;
		}
	}*/
	if (abs (height * 64/27 - width) < 5 || abs (height * 43/18 - width) < 5)
	{
		ratio = ASPECT_64_27;
	}
	else if (abs (height * 16/9 - width) < 10) // If the size is approximately 16:9, consider it so.
	{
		ratio = ASPECT_16_9;
	}
	// Consider 17:10 as well.
	else if (abs (height * 17/10 - width) < 10)
	{
		ratio = ASPECT_17_10;
	}
	// 16:10 has more variance in the pixel dimensions. Grr.
	else if (abs (height * 16/10 - width) < 60)
	{
		// 320x200 and 640x400 are always 4:3, not 16:10
		if ((width == 320 && height == 200) || (width == 640 && height == 400))
		{
			ratio = ASPECT_NONE;
		}
		else
		{
			ratio = ASPECT_16_10;
		}
	}
	// Unless vid_tft is set, 1280x1024 is 4:3, not 5:4.
	else if (height * 5/4 == width)// && vid_tft)
	{
		ratio = ASPECT_5_4;
	}
	// Assume anything else is 4:3.
	else
	{
		ratio = ASPECT_4_3;
	}

	if (trueratio != NULL)
	{
		*trueratio = ratio;
	}
	return (fakeratio >= 0) ? fakeratio : ratio;
}

#define IFARG(str) if(!strcmp(arg, (str)))

#ifndef _WIN32
// I_MakeRNGSeed is from ZDoom
#include <time.h>

// Return a random seed, preferably one with lots of entropy.
unsigned int I_MakeRNGSeed()
{
	unsigned int seed;
	int file;

	// Try reading from /dev/urandom first, then /dev/random, then
	// if all else fails, use a crappy seed from time().
	seed = time(NULL);
	file = open("/dev/urandom", O_RDONLY);
	if (file < 0)
	{
		file = open("/dev/random", O_RDONLY);
	}
	if (file >= 0)
	{
		read(file, &seed, sizeof(seed));
		close(file);
	}
	return seed;
}
#else
unsigned int I_MakeRNGSeed();
#endif

/*
==========================
=
= main
=
==========================
*/

extern Menu episodes;
extern Menu skills;

static bool
popMenu(wl_state_t *state)
{
	if (--state->menuLevel == 0)
	{
		//
		// DEALLOCATE EVERYTHING
		//
		CleanupControlPanel ();
		if (param_tedlevel || startgame || loadedgame)
			state->stage = START_GAME;
		else
			state->stage = START_DEMO_INTERMISSION;
		return false;
	}

	return false;
}

void
pushMenu(wl_state_t *state, StateMenuType menu)
{
	state->menuStack[state->menuLevel++] = menu;
}

void PrepareMainMenu (wl_state_t *state)
{
	bool idEasterEgg = Wads.CheckNumForName("IDGUYPAL") != -1;

	StartCPMusic (gameinfo.MenuMusic);
	SetupControlPanel ();

	Menu::closeMenus(false);

	StateMenuType menu;

	if(EpisodeInfo::GetNumEpisodes() > 1)
		menu = EPISODE_MENU;
	else
		menu = SKILL_MENU;
#ifdef TODO
	if (gameinfo.TrackHighScores == true)
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
	mainMenu[mainMenu.countItems()-2]->setHighlighted(false);
	mainMenu[3]->setEnabled(false);
#endif
	pushMenu(state, menu);
	state->currentMenu()->validateCurPos();
	state->currentMenu()->draw();
	Menu::closeMenus(false);
}

static int redrawitem = 1, lastitem = -1;

Menu *
wl_state_t::currentMenu() {
	if (menuLevel == 0)
		return NULL;
	switch (menuStack[menuLevel-1]) {
	case MAIN_MENU:
		return &mainMenu;
	case EPISODE_MENU:
		return &episodes;
	case SKILL_MENU:
		return &skills;
	}
	return NULL;
}


void
Menu::prepareMenu()
{
	if (redrawitem)
	{
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(curPos);
		getIndex(curPos)->draw();
	}
	VW_UpdateScreen ();

	lastBlinkTime = GetTimeCount ();
}


bool
Menu::handleStep(wl_state_t *state, const wl_input_state_t *input)
{
	//
	// CHANGE GUN SHAPE
	//
	if (getIndent() != 0)
	{
		TexMan.UpdateAnimations((GetTimeCount() - 1) * 14);

		if(MenuStyle != MENUSTYLE_Blake)
			cursor = TexMan("M_CURS1");
		draw();
	}

	CheckPause ();

#ifdef TODO
	//
	// SEE IF ANY KEYS ARE PRESSED FOR INITIAL CHAR FINDING
	//
	char key = LastASCII;
	if (key)
	{
		int ok = 0;

		if (key >= 'a')
			key -= 'a' - 'A';

		for (unsigned int i = curPos + 1; i < countItems(); i++)
			if (getIndex(i)->isEnabled() && getIndex(i)->getString()[0] == key)
			{
				curPos = i;
				ok = 1;
				SD_PlaySound("menu/move1");
				IN_ClearKeysDown ();
				break;
			}

		//
		// DIDN'T FIND A MATCH FIRST TIME THRU. CHECK AGAIN.
		//
		if (!ok)
		{
			for (int i = 0; i < curPos; i++)
			{
				if (getIndex(i)->isEnabled() && getIndex(i)->getString()[0] == key)
				{
					curPos = i;
					SD_PlaySound("menu/move1");
					IN_ClearKeysDown ();
					break;
				}
			}
		}
	}

	if(LastScan == SDLx_SCANCODE(DELETE))
	{
		handleDelete();
		LastScan = 0;

		// Leave menu if we delete everything.
		if(countItems() == 0)
		{
			lastitem = curPos; // Prevent redrawing
			exit = 2;
		}
	}

	//
	// GET INPUT
	//
	ControlInfo ci;
	ReadAnyControl (&ci);
#endif
	switch (input->menuDir)
	{
	default:
		break;

		////////////////////////////////////////////////
		//
		// MOVE UP
		//
	case dir_North:
	{
		if(countItems() <= 1)
			break;

		//
		// MOVE TO NEXT AVAILABLE SPOT
		//
		int oldPos = curPos;
		do
		{
			if (curPos == 0)
			{
				curPos = countItems() - 1;
				itemOffset = curPos - lastIndexDrawn;
			}
			else if (itemOffset > 0 && (unsigned)curPos == itemOffset+1)
			{
				--itemOffset;
				--curPos;
			}
			else
				--curPos;
		}
		while (!getIndex(curPos)->isEnabled());

#ifdef TODO
		if(oldPos - curPos == 1)
		{
			animating = true;
			draw();
			drawGunHalfStep(x, getY() + getHeight(oldPos) - 6);
			animating = false;
		}
#endif
		draw();
		SD_PlaySound("menu/move2");
		break;
	}

	////////////////////////////////////////////////
	//
	// MOVE DOWN
	//
	case dir_South:
	{
		if(countItems() <= 1)
			break;

		int oldPos = curPos;
		do
		{
			unsigned int lastPos = countItems() - 1;
			if ((unsigned)curPos == lastPos)
			{
				curPos = 0;
				itemOffset = 0;
			}
			else if (lastIndexDrawn != lastPos && (unsigned)curPos >= lastIndexDrawn-1)
			{
				++itemOffset;
				++curPos;
			}
			else
				++curPos;
		}
		while (!getIndex(curPos)->isEnabled());

#ifdef TODO
		if(oldPos - curPos == -1)
		{
			animating = true;
			draw();
			drawGunHalfStep(x, getY() + getHeight(oldPos) + 6);
			animating = false;
		}
#endif
		draw();
		SD_PlaySound("menu/move2");
		break;
	}
	case dir_West:
		getIndex(curPos)->left();
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(curPos);
		getIndex(curPos)->draw();
		break;
	case dir_East:
		getIndex(curPos)->right();
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(curPos);
		getIndex(curPos)->draw();
		break;
	}

	if (input->menuEnter)
		state->stage = MENU_EXITING_ENTER_1;
	else if (input->menuBack)
		state->stage = MENU_EXITING_ESCAPE_1;
	else
		State_FadeIn (state);
	VW_UpdateScreen();
	return true;
}

bool Menu::ClearMenu()
{
	//
	// ERASE EVERYTHING
	//
	if (lastitem != curPos)
	{
		PrintX = getX() + getIndent();
		PrintY = getY() + getHeight(curPos);
		getIndex(curPos)->draw();
		redrawitem = 1;
	}
	else
		redrawitem = 0;
	lastitem = curPos;

	VW_UpdateScreen ();
	return true;
}

static bool handleChoice(wl_state_t *state, int pos)
{
	switch(state->menuStack[state->menuLevel - 1])
	{
	case EPISODE_MENU:
	{
		EpisodeInfo &ep = EpisodeInfo::GetEpisode(pos);

		if(!GameMap::CheckMapExists(ep.StartMap))
		{
			SD_PlaySound("player/usefail");
			Message("Please select \"Read This!\"\n"
				"from the Options menu to\n"
				"find out how to order this\n" "episode from Apogee.");
			State_Ack(state);
			state->stage = MENU_PREPARE;
			return true;
		}
		state->episode_num = pos;
		pushMenu(state, SKILL_MENU);
		state->stage = MENU_PREPARE;
		State_FadeOut(state);
	}
		break;
	case SKILL_MENU:
		state->skill_num = pos;
		state->stage = START_GAME;
		state->menuLevel = 0;
		// TODO: confirmation
		break;
	}
	return false;
}

static bool exitMenuEnter(wl_state_t *state)
{
	Menu *menu = state->currentMenu();
	int curPos = menu->getCurrentPosition();
	if(menu->getIndex(curPos)->playActivateSound())
		SD_PlaySound (menu->getIndex(curPos)->getActivateSound());
	PrintX = menu->getX() + menu->getIndent();
	PrintY = menu->getY() + menu->getHeight(curPos);
	return handleChoice(state, curPos);
}

static bool exitMenuEscape(wl_state_t *state)
{
	SD_PlaySound("menu/escape");

	return popMenu(state);
}

static bool EndSequence1(wl_state_t *state)
{
	FString next = state->nextMap;
	State_Fade(state, 0, 255, RPART(levelInfo->ExitFadeColor), GPART(levelInfo->ExitFadeColor), BPART(levelInfo->ExitFadeColor), levelInfo->ExitFadeDuration);
	if (state->dointermission && levelInfo->ForceTally) {
		state->stage = MAP_CHANGE_2;
	} else {
		state->stage = END_SEQUENCE_2;
	}
	return false;
}

static bool EndSequence2(wl_state_t *state)
{
	if (state->dointermission) {
		DrawVictory(false);
		State_FadeIn (state);
		State_Ack(state);
		state->stage = END_SEQUENCE_3;
		return true;
	}
	state->stage = END_SEQUENCE_3;
	return false;
}

static bool EndSequence3(wl_state_t *state)
{
	if(state->dointermission) {
			state->stageAfterIntermission = END_SEQUENCE_4;
			return TransitionText(state, levelInfo->Cluster, -1);
	}
	state->stage = END_SEQUENCE_5;
	return false;
}

static bool EndSequence4(wl_state_t *state)
{
	state->stage = END_SEQUENCE_5;
	State_FadeOut(state);
	return false;
}

static bool EndSequence5(wl_state_t *state)
{
	FString seqName = levelInfo->NextVictory;
	if(seqName.IsEmpty() || seqName.IndexOf("EndSequence:") != 0)
	{
		state->stage = END_SEQUENCE_6;
		return false;
	}

	state->stage = PLAY_INTERMISSION;
	state->stageAfterIntermission = END_SEQUENCE_6;
	InitIntermission(&state->intermission, seqName.Mid(12), false);
	return true;
}

static bool EndSequence6(wl_state_t *state)
{
	CheckHighScore (players[0].score,levelInfo);
	state->stage = START_DEMO_INTERMISSION;
	return true;
}

void InitThinkerList();

bool TopLoopStep(wl_state_t *state, const wl_input_state_t *input) {
	if (state->isFading) {
		state->fadeCur += state->fadeStep * tics;
		if (state->fadeStep < 0 ? state->fadeCur > state->fadeEnd : state->fadeCur < state->fadeEnd) {
                         // fade through intermediate frames
			V_SetBlend(state->fadeRed, state->fadeGreen, state->fadeBlue, state->fadeCur >> FRACBITS);
			VH_UpdateScreen();
			return true;
		}
		state->isFading = false;
		//
                // final color
                //
		V_SetBlend (state->fadeRed, state->fadeGreen, state->fadeBlue, state->fadeEnd>>FRACBITS);
		screenfaded = state->fadeEnd != 0;
		VH_UpdateScreen();
		return true;
	}
	if (state->isInWait) {
		if (state->waitCanBeAcked && input->screenAcked) {
			state->wasAcked = true;
			state->isInWait = false;
		}
		if (state->waitCanTimeout && state->ackTimeout < (longword) GetTimeCount())
			state->isInWait = false;
		VH_UpdateScreen();
		return true;
	}
	if (state->isInQuiz) {
		return Dialog::quizHandle(state, input);
	}
	switch (state->stage) {
	case START:
		if (!param_nowait && (IWad::GetGame().Flags & IWad::REGISTERED)
			&& strlen(language["REGNOTICE_TITLE"]) != 0) {
			state->stage = BEFORE_NON_SHAREWARE;
			State_FadeOut(state);
			return false;
		}
		state->stage = AFTER_NON_SHAREWARE;
		return false;
	case BEFORE_NON_SHAREWARE:
		NonShareware();
		VH_UpdateScreen ();
		state->stage = NON_SHAREWARE_WAIT;
		State_FadeIn(state);
		return true;

	case NON_SHAREWARE_WAIT:
		State_Ack(state);
		state->stage = AFTER_NON_SHAREWARE;
		return false;

	case AFTER_NON_SHAREWARE:
		StartCPMusic(gameinfo.TitleMusic);
		state->playing_title_music = true;
		if (param_nowait)
			state->stage = param_tedlevel ? START_GAME : MAIN_MENU_PREPARE;
		else {
			State_FadeOut (state);
			state->stage = BEFORE_PG13;
		}
		return false;

	case BEFORE_PG13:
		if (gameinfo.AdvisoryPic.IsEmpty()) {
			state->stage = START_DEMO_INTERMISSION;
			return false;
		}
		PG13();
		State_FadeIn(state);
		state->stage = PG13_WAIT;
		return false;
	case PG13_WAIT:
		State_UserInput(state, TICRATE * 7);
		state->stage = AFTER_PG13;
		return false;
	case AFTER_PG13:
		State_FadeOut (state);
		state->stage = START_DEMO_INTERMISSION;
		return false;
	case START_DEMO_INTERMISSION:
		state->wasAcked = false;
		if (param_nowait) {
			state->stage = AFTER_DEMO_INTERMISSION;
			return false;
		}
		if (!state->playing_title_music) {
			StartCPMusic(gameinfo.TitleMusic);
			state->playing_title_music = true;
		}
		{
			state->stage = PLAY_INTERMISSION;
			state->stageAfterIntermission = AFTER_DEMO_INTERMISSION;
			InitIntermission(&state->intermission, "DemoLoop", true);
		}
		return false;
	case PLAY_INTERMISSION:
		if (state->wasAcked) {
			State_FadeOut (state);
			state->stage = state->stageAfterIntermission;
			return false;
		}

		state->intermission.image_ready = false;
		state->intermission.fade_out = false;
		state->intermission.fade_in = false;

		while (!state->intermission.finished && !state->intermission.image_ready
		       && !state->intermission.fade_out && !state->intermission.fade_in) {
			AdvanceIntermission(state, &state->intermission);
		}

		if (state->intermission.fade_out)
			State_FadeOut(state, 0, 255, state->intermission.fade_steps);
		if (state->intermission.fade_in)
			State_FadeIn(state, 0, 255, state->intermission.fade_steps);

		if (state->intermission.finished) {
			State_FadeOut (state);
			state->stage = START_DEMO_INTERMISSION;
		}
		if (state->intermission.image_ready) {
			VW_UpdateScreen();
		}
		if (state->intermission.wait > 0) {
			State_UserInput(state, state->intermission.wait);
		}
		if (state->intermission.wait == 0) {
			State_Ack(state);
		}
		return state->intermission.image_ready;
	case AFTER_DEMO_INTERMISSION:
		VL_ReadPalette(gameinfo.GamePalette);
		state->playing_title_music = false;
		state->stage = param_tedlevel ? START_GAME : MAIN_MENU_PREPARE;
		return false;
	case MAIN_MENU_PREPARE:
#ifdef TODO
		if (Keyboard[sc_Tab])
			RecordDemo ();
#endif
		PrepareMainMenu (state);
		menusAreFaded = false;
		state->stage = MENU_PREPARE;
		return false;
	case MENU_PREPARE:
	{
		if(state->currentMenu()->areMenusClosed())
			return popMenu(state);

		state->currentMenu()->prepareMenu();
		state->stage = MENU_RUN;
		return true;
	}
	case MENU_RUN:
		return state->currentMenu()->handleStep(state, input);
	case MENU_EXITING_ENTER_1:
		state->stage = MENU_EXITING_ENTER_2;
		return state->currentMenu()->ClearMenu();
	case MENU_EXITING_ENTER_2:
		return exitMenuEnter(state);
	case MENU_EXITING_ESCAPE_1:
		state->stage = MENU_EXITING_ESCAPE_2;
		return state->currentMenu()->ClearMenu();
	case MENU_EXITING_ESCAPE_2:
		return exitMenuEscape(state);
	case START_GAME:
		return GameLoopInit(state);
	case GAME_DRAW_PLAY_SCREEN:
		DrawPlayScreen ();
		state->stage = GAME_LOAD_MAP;
		return false;
	case GAME_LOAD_MAP:
		return GameMapStart (state);
	case PLAY_START:
		PlayInit ();
		state->stage = PLAY_STEP_A;
		return false;
	case PLAY_STEP_A:
		PlayLoopA();
		if (screenfaded)
		{
				State_FadeIn (state);
				ResetTimeCount();
		}
		state->stage = PLAY_STEP_B;
		return false;
	case PLAY_STEP_B:
		return PlayLoopB (state);
	case GAME_END_MAP:
		return GameMapEnd (state);
	case END_SEQUENCE_1:
		return EndSequence1 (state);
	case END_SEQUENCE_2:
		return EndSequence2 (state);
	case END_SEQUENCE_3:
		return EndSequence3 (state);
	case END_SEQUENCE_4:
		return EndSequence4 (state);
	case END_SEQUENCE_5:
		return EndSequence5 (state);
	case END_SEQUENCE_6:
		return EndSequence6 (state);
	case MAP_CHANGE_1:
		return MapChange1 (state);
	case MAP_CHANGE_2:
		return MapChange2 (state);
	case MAP_CHANGE_3:
		if (playstate == ex_victorious) {
			State_FadeOut(state);
			state->stage = END_SEQUENCE_2;
			return false;
		} else
			return MapChange3 (state);
	case MAP_CHANGE_4:
		return MapChange4 (state);
	case MAP_CHANGE_5:
	  return MapChange5 (state);
	case LEVEL_INTERMISSION:
		return LevelIntermission(state);
	case LEVEL_INTERMISSION_CONTINUE:
		return LevelIntermissionContinue(state, input->screenAcked);
	case LEVEL_INTERMISSION_COUNT_KR:
		return LevelIntermissionCount1(state, input->screenAcked);
	case LEVEL_INTERMISSION_COUNT_2:
		return LevelIntermissionCount2(state, input->screenAcked);
	case LEVEL_INTERMISSION_COUNT_3:
		return LevelIntermissionCount3(state, input->screenAcked);
	case LEVEL_INTERMISSION_COUNT_BONUS:
		return LevelIntermissionCountBonus(state, input->screenAcked);
	case LEVEL_INTERMISSION_WAIT:
		return LevelIntermissionWait(state, input->screenAcked);
	case PSYCH_1:
		return updatePsych1(state);
	case PSYCH_2:
		return updatePsych2(state);
	case PSYCH_3:
		return updatePsych3(state);
	case PSYCH_4:
		return updatePsych4(state);
	case LEVEL_ENTER_TEXT:
		return LevelEnterText(state);
	case LEVEL_TRANSITION_INTERMISSION_START:
		return LevelEnterIntermissionStart(state);
	case DIED1:
		return Died1(state);
	case DIED2:
		return Died2(state);
	case DIED3:
		return Died3(state);
	case DIED4:
		return Died4(state);
	case DIED5:
		return Died5(state);
	case DIED6:
		return Died6(state);
	case DEATH_FIZZLE:
		return DeathFizzle(state);
	case DEATH_ROTATE:
		return DeathRotate(state);
	case DEATH_WEAPON_DROP:
		return DeathDropWeapon(state);
	case VICTORY_ZOOMER_START:
		return VictoryZoomerStart(state);
	case VICTORY_ZOOMER_STEP:
		return VictoryZoomerStep(state);
	case TEXT_READER_STEP:
		return TextReaderStep(state, input);
	}
	assert(false);
	return true;
}

// We are definting an atterm function so that we can control the exit behavior.
static const unsigned int MAX_TERMS = 32;
static void (*TermFuncs[MAX_TERMS])(void);
static unsigned int NumTerms;
void atterm(void (*func)(void))
{
       for(unsigned int i = 0;i < NumTerms;++i)
       {
               if(TermFuncs[i] == func)
                       return;
       }

       if(NumTerms < MAX_TERMS)
               TermFuncs[NumTerms++] = func;
       else
               fprintf(stderr, "Failed to register atterm function!\n");
}

void CallTerminateFunctions()
{
       while(NumTerms > 0)
               TermFuncs[--NumTerms]();
}
