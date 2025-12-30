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

bool	startgame;
bool	loadedgame;
int		mousexadjustment;
int     mouseyadjustment;
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
#ifdef _DIII4A //karin: main loop control
extern bool q3e_initialized;
extern volatile bool q3e_running;
extern bool GLimp_CheckGLInitialized(void);
#endif

//===========================================================================

/*
=====================
=
= NewGame
=
= Set up new game to start from the beginning
=
=====================
*/

void NewGame (int difficulty, const FString &map, bool displayBriefing, const ClassDef *playerClass)
{
	if(!playerClass)
		playerClass = ClassDef::FindClass(gameinfo.PlayerClasses[0]);

	// void cast can be removed when we move to C++11
	memset ((void*)&gamestate,0,sizeof(gamestate));
	gamestate.difficulty = &SkillInfo::GetSkill(difficulty);
	strncpy(gamestate.mapname, map, 8);
	gamestate.mapname[8] = 0;
	gamestate.playerClass = playerClass;
	levelInfo = &LevelInfo::Find(map);

	if(displayBriefing)
		EnterText(levelInfo->Cluster);

	// Clear LevelRatios
	LevelRatios.killratio = LevelRatios.secretsratio = LevelRatios.treasureratio =
		LevelRatios.numLevels = LevelRatios.time = 0;

	players[0].state = player_t::PST_ENTER;

	Dialog::ClearConversations();

	startgame = true;
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

static void ShutdownId (void)
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
static void InitGame()
{
	// initialize SDL
#if SDL_VERSION_ATLEAST(2,0,0)
	{
		SDL_version ver;
		SDL_GetVersion(&ver);
		printf("SDL_Init: Using SDL %d.%d.%d\n", ver.major, ver.minor, ver.patch);
	}
#else
	printf("SDL_Init: Using SDL 1.2\n");
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
	if(SDL_Init(0) < 0)
#else
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
#endif
	{
		I_FatalError("Unable to init SDL: %s", SDL_GetError());
	}

	SDL_ShowCursor(SDL_DISABLE);

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
	atterm(R_DeinitColormaps);
	GenerateLookupTables();

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

//
// Load Keys
//

	P_InitKeyMessages();
	atterm(P_DeinitKeyMessages);

//
// Finish with setting up through the config file.
//
	FinalReadConfig();

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
	Net::Init(DrawStartupConsole);

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

	if (Keyboard[sc_M])
		DoJukebox();

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

/*
==========================
=
= Quit
=
==========================
*/

void Quit ()
{
	throw CNoRunExit();
}

void I_FatalError (const char *format, ...)
{
	va_list vlist;
	va_start(vlist, format);
	FString error;
	error.VFormat(format, vlist);
	va_end(vlist);

	throw CFatalError(error);
}

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
	VW_FadeOut ();

	if(gameinfo.AdvisoryPic.IsEmpty())
		return;

	BYTE color = ColorMatcher.Pick(RPART(gameinfo.AdvisoryColor), GPART(gameinfo.AdvisoryColor), BPART(gameinfo.AdvisoryColor));

	VWB_Clear(color, 0, 0, screenWidth, screenHeight);
	FTexture *tex = TexMan(gameinfo.AdvisoryPic);
	if(tex->GetScaledWidth() == 320)
		VWB_DrawGraphic(tex, 0, 100-tex->GetScaledHeight()/2);
	else
		VWB_DrawGraphic(tex, 304-tex->GetScaledWidth(), 174-tex->GetScaledHeight());
	VW_UpdateScreen ();

	VW_FadeIn ();
	IN_UserInput (TICRATE * 7);

	VW_FadeOut ();
}

//===========================================================================

////////////////////////////////////////////////////////
//
// NON-SHAREWARE NOTICE
//
////////////////////////////////////////////////////////
static void NonShareware (void)
{
	if(strlen(language["REGNOTICE_TITLE"]) == 0)
		return;

	VW_FadeOut ();

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

	VW_UpdateScreen ();
	VW_FadeIn ();
	IN_Ack ();
}

//===========================================================================


/*
=====================
=
= DemoLoop
=
=====================
*/

static void DemoLoop()
{
//
// check for launch from ted
//
	if (param_tedlevel)
	{
		param_nowait = true;
		NewGame(param_difficulty,param_tedlevel,false);
	}


//
// main game cycle
//

	if (!param_nowait && (IWad::GetGame().Flags & IWad::REGISTERED))
		NonShareware();

	StartCPMusic(gameinfo.TitleMusic);

	if (!param_nowait)
		PG13 ();

	IntermissionInfo *demoLoop = IntermissionInfo::Find("DemoLoop");
	bool gotoMenu = false;
	while (1)
	{
#ifdef _DIII4A //karin: control main loop running
		if(!q3e_running)
			break;
		if(!GLimp_CheckGLInitialized())
			continue;
#endif
		while(!param_nowait && ShowIntermission(demoLoop, true))
		{
		}

		if(!param_tedlevel)
		{
			gotoMenu = false;

			if (Keyboard[sc_Tab])
				RecordDemo ();
			else
				US_ControlPanel (0);
		}

		if (param_tedlevel || startgame || loadedgame)
		{
			param_tedlevel = NULL;
			if(GameLoop ())
				gotoMenu = true;

			if(!param_nowait && !gotoMenu)
			{
				StartCPMusic(gameinfo.TitleMusic);
			}
		}
	}
}


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

static const char* CheckParameters(int argc, char *argv[], TArray<FString> &files)
{
	const char* extension = NULL;
	bool hasError = false, showHelp = false;
	bool sampleRateGiven = false, audioBufferGiven = false;
	int defaultSampleRate = param_samplerate;

	fullscreen = vid_fullscreen;

	for(int i = 1; i < argc; i++)
	{
		char *arg = argv[i];
		IFARG("--baby")
			param_difficulty = 0;
		else IFARG("--easy")
			param_difficulty = 1;
		else IFARG("--normal")
			param_difficulty = 2;
		else IFARG("--hard")
			param_difficulty = 3;
		else IFARG("--skill")
		{
			if(++i >= argc)
			{
				printf("The skill option is missing an argument!\n");
				hasError = true;
			}
			param_difficulty = atoi(argv[i])-1; // 1-based indexing
		}
		else IFARG("--nowait")
			param_nowait = true;
		else IFARG("--tedlevel")
		{
			if(++i >= argc)
			{
				printf("The tedlevel option is missing the level argument!\n");
				hasError = true;
			}
			else param_tedlevel = argv[i];
		}
		else IFARG("--fullscreen")
			fullscreen = true;
		else IFARG("--res")
		{
			if(i + 2 >= argc)
			{
				printf("The res option needs the width and/or the height argument!\n");
				hasError = true;
			}
			else
			{
				screenWidth = atoi(argv[++i]);
				screenHeight = atoi(argv[++i]);
				if(screenWidth < 320)
					printf("Screen width must be at least 320!\n"), hasError = true;
				if(screenHeight < 200)
					printf("Screen height must be at least 200!\n"), hasError = true;
			}
		}
		else IFARG("--aspect")
		{
			const char* ratio = argv[++i];
			if(strcmp(ratio, "4:3") == 0)
				vid_aspect = ASPECT_4_3;
			else if(strcmp(ratio, "16:10") == 0)
				vid_aspect = ASPECT_16_10;
			else if(strcmp(ratio, "17:10") == 0)
				vid_aspect = ASPECT_17_10;
			else if(strcmp(ratio, "16:9") == 0)
				vid_aspect = ASPECT_16_9;
			else if(strcmp(ratio, "5:4") == 0)
				vid_aspect = ASPECT_5_4;
			else if(strcmp(ratio, "21:9") == 0)
				vid_aspect = ASPECT_64_27;
			else
			{
				printf("Unknown aspect ratio %s!\n", ratio);
				hasError = true;
			}
		}
		else IFARG("--bits")
		{
			if(++i >= argc)
			{
				printf("The bits option is missing the color depth argument!\n");
				hasError = true;
			}
			else
			{
				screenBits = atoi(argv[i]);
				switch(screenBits)
				{
					case 8:
					case 16:
					case 24:
					case 32:
						break;

					default:
						printf("Screen color depth must be 8, 16, 24, or 32!\n");
						hasError = true;
						break;
				}
			}
		}
		else IFARG("--noadaptive")
			noadaptive = true;
		else IFARG("--nodblbuf")
			usedoublebuffering = false;
		else IFARG("--extravbls")
		{
			if(++i >= argc)
			{
				printf("The extravbls option is missing the vbls argument!\n");
				hasError = true;
			}
			else
			{
				extravbls = atoi(argv[i]);
				if((signed)extravbls < 0)
				{
					printf("Extravbls must be positive!\n");
					hasError = true;
				}
			}
		}
		else IFARG("--joystick")
		{
			if(++i >= argc)
			{
				printf("The joystick option is missing the index argument!\n");
				hasError = true;
			}
			else param_joystickindex = atoi(argv[i]);   // index is checked in InitGame
		}
		else IFARG("--joystickhat")
		{
			if(++i >= argc)
			{
				printf("The joystickhat option is missing the index argument!\n");
				hasError = true;
			}
			else param_joystickhat = atoi(argv[i]);
		}
		else IFARG("--samplerate")
		{
			if(++i >= argc)
			{
				printf("The samplerate option is missing the rate argument!\n");
				hasError = true;
			}
			else param_samplerate = atoi(argv[i]);
			sampleRateGiven = true;
		}
		else IFARG("--audiobuffer")
		{
			if(++i >= argc)
			{
				printf("The audiobuffer option is missing the size argument!\n");
				hasError = true;
			}
			else param_audiobuffer = atoi(argv[i]);
			audioBufferGiven = true;
		}
		else IFARG("--help")
			showHelp = true;
		else IFARG("--data")
			if(++i >= argc)
			{
				printf("Expected main data extension!\n");
				hasError = true;
			}
			else
				extension = argv[i];
		else IFARG("--file")
		{
			if(++i < argc)
				files.Push(argv[i]);
		}
		else IFARG("--config")
		{
			// The config code will handle this itself, so ignore it here.
			++i;
		}
		else IFARG("--console") {} // Windows always create console parameter
		else IFARG("--savedir")
		{
			if(++i < argc)
				FileSys::SetDirectoryPath(FileSys::DIR_Saves, argv[i]);
		}
		else IFARG("--port")
		{
			if(++i < argc)
				Net::InitVars.port = atoi(argv[i]);
		}
		else IFARG("--host")
		{
			if(++i < argc)
			{
				Net::InitVars.mode = Net::MODE_Host;
				Net::InitVars.numPlayers = atoi(argv[i]);
			}
		}
		else IFARG("--join")
		{
			if(++i < argc)
			{
				Net::InitVars.mode = Net::MODE_Client;
				Net::InitVars.joinAddress = argv[i];
			}
		}
		else IFARG("--debugnet")
		{
			DebugNetwork = true;
		}
		else IFARG("--foreignsave")
		{
			GameSave::param_foreginsave = true;
		}
		else
			files.Push(argv[i]);
	}
	if(hasError || showHelp)
	{
		if(hasError) printf("\n");
		printf(
			"%s\n"
			"http://maniacsvault.net/ecwolf/\n"
			"Based on Wolf4SDL v1.7\n"
			"Ported by Chaos-Software (http://www.chaos-software.de.vu)\n"
			"Original Wolfenstein 3D by id Software\n\n"
			"Usage: " BINNAME " [options]\n"
			"Options:\n"
			" --help                 This help page\n"
#ifdef _WIN32
			" --console              Display a console window\n"
#endif
			" --config <file>        Use an explicit location for the config file\n"
			" --savedir <dir>        Use an explicit location for save games\n"
			" --file <file>          Loads an extra data file\n"
			" --data <extension>     Selects the given game data set skipping the dialog\n"
			" --tedlevel <level>     Starts the game in the given level\n"
			" --skill <#>            Sets the difficulty for tedlevel\n"
			" --baby                 Sets the difficulty to baby for tedlevel\n"
			" --easy                 Sets the difficulty to easy for tedlevel\n"
			" --normal               Sets the difficulty to normal for tedlevel\n"
			" --hard                 Sets the difficulty to hard for tedlevel\n"
			" --nowait               Skips intro screens\n"
			" --fullscreen           Starts the game in fullscreen mode\n"
			" --res <width> <height> Sets the screen resolution\n"
			" --aspect <aspect>      Sets the aspect ratio.\n"
			" --noadaptive           Disables adaptive tics.\n"
			" --bits <b>             Sets the screen color depth\n"
			"                        (use this when you have palette/fading problems\n"
			"                        allowed: 8, 16, 24, 32, default: \"best\" depth)\n"
			" --nodblbuf             Don't use SDL's double buffering\n"
			" --extravbls <vbls>     Sets a delay after each frame, which may help to\n"
			"                        reduce flickering (unit is currently 8 ms, default: 0)\n"
			" --joystick <index>     Use the index-th joystick if available\n"
			"                        (-1 to disable joystick, default: 0)\n"
			" --joystickhat <index>  Enables movement with the given coolie hat\n"
			" --samplerate <rate>    Sets the sound sample rate (given in Hz, default: %i)\n"
			" --audiobuffer <size>   Sets the size of the audio buffer (-> sound latency)\n"
			"                        (given in bytes, default: 2048 / (44100 / samplerate))\n"
			" --host <number>        Sets up a network game with the given number of players.\n"
			" --connect <address>    Connects to the given host.\n"
			" --port <number>        Port number to use for network communications.\n"
			" --debugnet             Enable network debugging messages.\n"
			" --foreignsave          Disable save game validity checking.\n"
			, GetGameCaption(), defaultSampleRate
		);
		Quit();
	}

	r_ratio = static_cast<Aspect>(CheckRatio(screenWidth, screenHeight));

	if(sampleRateGiven && !audioBufferGiven)
		param_audiobuffer = 2048 / (44100 / param_samplerate);

#ifdef __ANDROID__
	param_audiobuffer = (2048*2) / (44100 / param_samplerate);
#endif

	return extension;
}

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

static void ScannerMessageHandler(Scanner::MessageLevel level, const char *error, va_list list)
{
	FString errorMessage;
	errorMessage.VFormat(error, list);

	if(level == Scanner::ERROR)
		throw CRecoverableError(errorMessage);
	else
		Printf("%s", errorMessage.GetChars());
}

// Basically from ZDoom
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

static void CallTerminateFunctions()
{
	ShutdownId();
	WriteConfig();

	while(NumTerms > 0)
		TermFuncs[--NumTerms]();

	SDL_Quit();
}

#ifdef _WIN32
void I_AcknowledgeError();
#endif

int WL_Main (int argc, char *argv[])
{
	try
	{
		// Stop the C library from screwing around with its functions according
		// to the system locale.
		setlocale(LC_ALL, "C");

		FileSys::SetupPaths(argc, argv);

		// Find the program directory.
		FString progdir(FileSys::GetDirectoryPath(FileSys::DIR_Program));

		Scanner::SetMessageHandler(ScannerMessageHandler);

		printf("ReadConfig: Reading the Configuration.\n");
		config.LocateConfigFile(argc, argv);
		ReadConfig();

		{
			TArray<FString> wadfiles, files;

			Printf("IWad: Selecting base game data.\n");
			const char* extension = CheckParameters(argc, argv, wadfiles);
			IWad::SelectGame(files, extension, MAIN_PK3, progdir);

			for(unsigned int i = 0;i < wadfiles.Size();++i)
				files.Push(wadfiles[i]);

			printf("W_Init: Init WADfiles.\n");
			Wads.InitMultipleFiles(files);
			LumpRemapper::RemapAll();
			language.SetupStrings();
		}

		R_InitRenderer();

		printf("InitGame: Setting up the game...\n");
		rngseed = I_MakeRNGSeed(); // May change after initializing a net game
		InitGame();

		FRandom::StaticClearRandom();

#ifdef _DIII4A //karin: setup has initialized
		q3e_initialized = true;
#endif
		printf("DemoLoop: Starting the game loop...\n");
		DemoLoop();

		I_FatalError("Demo loop exited???");
	}
	catch(CNoRunExit) // Normal exit from deep code
	{
		CallTerminateFunctions();
		return 0;
	}
	catch(CDoomError &error)
	{
		CallTerminateFunctions();

#ifdef __ANDROID__
		Printf("%s\n", error.GetMessage());
#else
		fprintf(stderr, "%s\n", error.GetMessage());
#endif

#ifdef _WIN32
		I_AcknowledgeError();
#endif

		return 1;
	}
	return 1;
}
